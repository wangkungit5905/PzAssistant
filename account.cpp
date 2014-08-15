#include <QVariant>

#include "account.h"
#include "common.h"
#include "global.h"
#include "utils.h"
#include "pz.h"
#include "tables.h"
#include "dbutil.h"
#include "logs/Logger.h"
#include "version.h"
#include "subject.h"
#include "PzSet.h"
#include "configvariablenames.h"

QSqlDatabase* Account::db;

/**
 * @brief 创建新账户的数据库文件
 * @param fileName      账户数据库文件名
 * @param code          账户代码
 * @param name          账户简称
 * @param lname         账户全称
 * @param subSys        所用科目系统代码
 * @param startYear     起账年份
 * @param startMonth    起账月份
 * @return
 */
bool Account::createNewAccount(QString fileName, QString code, QString name, QString lname, SubSysNameItem* subSys, int startYear, int startMonth, QString& error)
{
    //1、检查文件名是否冲突
    QString fname = DATABASE_PATH + fileName;
    if(QFile::exists(fname)){
        error = tr("文件名冲突！");
        return false;
    }
    //2、连接数据库
    QString connName = "CreateNewAccount";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",connName);
    db.setDatabaseName(fname);
    if(!db.open()){
        error = tr("不能打开数据库连接！");
        return false;
    }

    //3、创建所有必要的表
    if(!Account::createTableForNewAccount(db,error)){
        error = tr("创建表格失败！\n\n") + error;
        return false;
    }
    //4、填写基本属性（编码、名称、全称、科目系统、-报表类型、-最后访问时间、-文件名、版本号）
    if(!db.transaction()){
        error = tr("为设置账户基本属性，启动事务失败！");
        return false;
    }
    QSqlQuery q(db);
    QString s = QString("insert into %1(%2,%3,%4) values(:code,:name,:value)").arg(tbl_accInfo)
            .arg(fld_acci_code).arg(fld_acci_name).arg(fld_acci_value);
    if(!q.prepare(s)){
        error = tr("执行SQL语句失败：%1").arg(s);
        return false;
    }
    q.bindValue(":code", ACODE);
    q.bindValue(":name",ACCINFO_VNAME_CODE);
    q.bindValue(":value",code);
    if(!q.exec()){
        error = tr("设置账户代码出错！");
        return false;
    }
    q.bindValue(":code", SNAME);
    q.bindValue(":name",ACCINFO_VNAME_SNAME);
    q.bindValue(":value",name);
    if(!q.exec()){
        error = tr("设置账户简称出错！");
        return false;
    }
    q.bindValue(":code", LNAME);
    q.bindValue(":name",ACCINFO_VNAME_LNAME);
    q.bindValue(":value",lname);
    if(!q.exec()){
        error = tr("设置账户全称出错！");
        return false;
    }
    q.bindValue(":code", DBVERSION);
    q.bindValue(":name",ACCINFO_VNAME_DBVERSION);
    int mv = 0,sv = 0;
    VMAccount::getAppSupportVersion(mv,sv);
    QString verStr = QString("%1.%2").arg(mv).arg(sv);
    q.bindValue(":value",verStr);
    if(!q.exec()){
        error = tr("设置账户采用的科目系统代码出错！");
        return false;
    }
    if(!db.commit()){
        error = tr("为设置账户基本属性，提交事务失败！");
        return false;
    }

    //5、导入科目系统
    if(!importSubjectForNewAccount(subSys->code,db,error)){
        error = tr("导入科目系统时出错！\n\n") + error;
        return false;
    }

    //6、初始化帐套记录
    s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9) values(%10,%11,%12,%13,'%14',%15,%16,0)")
            .arg(tbl_accSuites).arg(fld_accs_year).arg(fld_accs_subSys).arg(fld_accs_isCur)
            .arg(fld_accs_recentMonth).arg(fld_accs_name).arg(fld_accs_startMonth)
            .arg(fld_accs_endMonth).arg(fld_accs_isClosed).arg(startYear).arg(subSys->code).arg(1)
            .arg(startMonth).arg(tr("%1年").arg(startYear)).arg(startMonth).arg(startMonth);
    if(!q.exec(s)){
        error = tr("在初始化帐套记录时发生错误！");
        return false;
    }

    //7、创建首条转移记录
    int mid = AppConfig::getInstance()->getLocalMachine()->getMID();
    QString curTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,%8,%9,'%10','%11')")
            .arg(tbl_transfer).arg(fld_trans_smid).arg(fld_trans_dmid).arg(fld_trans_state)
            .arg(fld_trans_outTime).arg(fld_trans_inTime)
            .arg(mid).arg(mid).arg(ATS_TRANSINDES).arg(curTime).arg(curTime);
    if(!q.exec(s)){
        error = tr("在初始化首条转移记录时出错！");
        return false;
    }
    s = "select last_insert_rowid()";
    if(!q.exec(s) || !q.first())
        return false;
    int tid = q.value(0).toInt();
    QString outDesc = tr("默认初始由本机转出");
    QString inDesc = tr("默认初始由本机转入");
    s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')")
            .arg(tbl_transferDesc).arg(fld_transDesc_tid).arg(fld_transDesc_out)
            .arg(fld_transDesc_in).arg(tid).arg(outDesc).arg(inDesc);
    if(!q.exec(s)){
        error = tr("在记录首条转移记录描述信息时出错！");
        return false;
    }
    return true;
}

bool Account::createTableForNewAccount(QSqlDatabase db, QString& errors)
{
    if(!db.transaction()){
        errors = tr("为新账户创建表，启动事务失败！");
        return false;
    }
    QStringList names,sqls;
    if(!AppConfig::getInstance()->getUpdateTableCreateStatment(names,sqls)){
        errors = tr("无法获取表格创建语句！");
        //db.rollback();
        return false;
    }
    QSqlQuery q(db);
    for(int i = 0; i < names.count(); ++i){
        if(!q.exec(sqls.at(i))){
            errors = tr("在创建表“%1”时发生错误！\nsql：“%2”").arg(names.at(i).arg(sqls.at(i)));
            //db.rollback();
            return false;
        }
    }
    if(!db.commit()){
        errors = tr("为新账户创建表，提交事务失败！");
        return false;
    }
    return true;
}

/**
 * @brief 为新账户导入科目系统
 * @param subSys    待导入的科目系统代码
 * @param db        账户数据库连接对象
 * @param errors    错误信息
 * @return
 */
bool Account::importSubjectForNewAccount(int subSys,QSqlDatabase db, QString& errors)
{
    //1、导入一级科目及其类别
    if(!db.transaction()){
        errors = tr("为新账户导入科目系统，启动事务失败！");
        return false;
    }
    QSqlQuery qb(AppConfig::getBaseDbConnect());
    QString s = QString("select * from %1 where %2=%3").arg(tbl_base_fsub_cls)
            .arg(fld_base_fsub_cls_subSys).arg(subSys);
    if(!qb.exec(s)){
        errors = tr("无法读取一级科目类别表！");
        return false;
    }
    QSqlQuery qa1(db),qa2(db),q(db);
    s = QString("insert into %1(%2,%3,%4) values(%5,:code,:name)").arg(tbl_fsclass)
            .arg(fld_fsc_subSys).arg(fld_fsc_code).arg(fld_fsc_name).arg(subSys);
    if(!qa1.prepare(s)){
        errors = tr("错误执行sql语句（%1）").arg(s);
        return false;
    }
    while(qb.next()){
         int code = qb.value(FI_BASE_FSUB_CLS_CODE).toInt();
         QString name  = qb.value(FI_BASE_FSUB_CLS_NAME).toString();
         qa1.bindValue(":code",code);
         qa1.bindValue(":name",name);
         if(!qa1.exec()){
             errors = tr("插入一级科目类别时发生错误！");
             return false;
         }
    }
    s = QString("select * from %1 where %2=%3").arg(tbl_base_fsub)
            .arg(fld_base_fsub_subsys).arg(subSys);
    if(!qb.exec(s)){
        errors = tr("无法读取一级科目！");
        return false;
    }
    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
    //因为新建的一级科目表表名还没有添加后缀，因此，在实际使用前要改名为与账户所使用的科目系统对应的
    s = QString("alter table %1 rename to %2").arg(tbl_fsub_prefix).arg(tname);
    if(!qa2.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QString s2 = QString("insert into %1 default values").arg(tbl_fsub_ids);
    s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) "
                "values(:fid,:subcode,:remcode,:subCls,:jddir,:isview,:isusedwb,:weight,:name)")
            .arg(tname).arg(fld_fsub_fid).arg(fld_fsub_subcode).arg(fld_fsub_remcode)
            .arg(fld_fsub_class).arg(fld_fsub_jddir).arg(fld_fsub_isEnalbed)
            .arg(fld_fsub_isUseWb).arg(fld_fsub_weight).arg(fld_fsub_name);
    if(!qa1.prepare(s)){
        errors = tr("错误执行sql语句（%1）").arg(s);
        return false;
    }
    while(qb.next()){
        QString code = qb.value(FI_BASE_FSUB_SUBCODE).toString();
        QString remcode = qb.value(FI_BASE_FSUB_REMCODE).toString();
        int cls = qb.value(FI_BASE_FSUB_CLS).toInt();
        int jddir = qb.value(FI_BASE_FSUB_JDDIR).toInt();
        int isview = qb.value(FI_BASE_FSUB_ENABLE).toInt();
        int isUsedWb = qb.value(FI_BASE_FSUB_USEDWB).toInt();
        int weight = qb.value(FI_BASE_FSUB_WEIGHT).toInt();
        QString name = qb.value(FI_BASE_FSUB_SUBNAME).toString();
        if(!qa2.exec(s2)){
            LOG_SQLERROR(s);
            return false;
        }
        qa2.exec("select last_insert_rowid()");
        qa2.first();
        int fid = qa2.value(0).toInt();
        qa1.bindValue(":fid",fid);
        qa1.bindValue(":subcode",code);
        qa1.bindValue(":remcode",remcode);
        qa1.bindValue(":subCls",cls);
        qa1.bindValue(":jddir",jddir);
        qa1.bindValue(":isview",isview);
        qa1.bindValue(":isusedwb",isUsedWb);
        qa1.bindValue(":weight",weight);
        qa1.bindValue(":name",name);
        if(!qa1.exec()){
            errors = tr("插入一级科目时发生错误！");
            return false;
        }
    }
    //2、导入常用名称条目及其类别
    s = QString("select * from %1").arg(tbl_base_nic);
    if(!qb.exec(s)){
        errors = tr("无法读取名称条目类别表！");
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5) values(:order,:code,:name,:explain)")
            .arg(tbl_nameItemCls).arg(fld_nic_order).arg(fld_nic_clscode)
            .arg(fld_nic_name).arg(fld_nic_explain);
    if(!qa1.prepare(s)){
        errors = tr("错误执行sql语句（%1）").arg(s);
        return false;
    }
    while(qb.next()){
        int order = qb.value(FI_BASE_NIC_ORDER).toInt();
        int code = qb.value(FI_BASE_NIC_CODE).toInt();
        QString name = qb.value(FI_BASE_NIC_NAME).toString();
        QString explain = qb.value(FI_BASE_NIC_EXPLAIN).toString();
        qa1.bindValue(":order",order);
        qa1.bindValue(":code",code);
        qa1.bindValue(":name",name);
        qa1.bindValue(":explain",explain);
        if(!qa1.exec()){
            errors = tr("插入名称条目类别时发生错误！");
            return false;
        }
    }
    s = QString("select * from %1").arg(tbl_base_ni);
    if(!qb.exec(s)){
        errors = tr("无法读取名称条目表！");
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values(:sname,:lname,:remcode,:cls,:ctime,:creator)")
            .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_lname).arg(fld_ni_remcode)
            .arg(fld_ni_class).arg(fld_ni_crtTime).arg(fld_ni_creator);
    if(!qa1.prepare(s)){
        errors = tr("错误执行sql语句（%1）").arg(s);
        return false;
    }
    while(qb.next()){
        QString sname = qb.value(FI_BASE_NI_SNAME).toString();
        QString lname = qb.value(FI_BASE_NI_LNAME).toString();
        if(lname.isEmpty())
            lname=sname;
        QString remCode = qb.value(FI_BASE_NI_REMCODE).toString();
        int cls = qb.value(FI_BASE_NI_CLASS).toInt();
        QString ctime = QDateTime::currentDateTime().toString(Qt::ISODate);
        int userId = curUser->getUserId();
        qa1.bindValue(":sname",sname);
        qa1.bindValue(":lname",lname);
        qa1.bindValue(":remcode",remCode);
        qa1.bindValue(":cls",cls);
        qa1.bindValue(":ctime",ctime);
        qa1.bindValue(":creator",userId);
        if(!qa1.exec()){
            errors = tr("插入名称条目时发生错误！");
            return false;
        }
    }


    //3、设置本币
    s = QString("insert into %1(%2,%3,%4,%5) values(:code,:name,:sign,0)")
            .arg(tbl_moneyType).arg(fld_mt_code).arg(fld_mt_name).arg(fld_mt_sign)
            .arg(fld_mt_isMaster);
    if(!qa1.prepare(s)){
        errors = tr("在设置可用货币时发送错误！");
        return false;
    }
    s = QString("select * from %1 where %2=%3 or %2=%4").arg(tbl_base_mt).arg(fld_base_mt_code)
            .arg(RMB).arg(USD);
    if(!qb.exec(s)){
        errors = tr("无法读取本币（人民币）！");
        return false;
    }
    while(qb.next()){
        QString mtName = qb.value(fld_base_mt_name).toString();
        QString mtSign = qb.value(fld_base_mt_sign).toString();
        int mtCode = qb.value(fld_base_mt_code).toInt();
        qa1.bindValue(":code",mtCode);
        qa1.bindValue(":name",mtName);
        qa1.bindValue(":sign",mtSign);
        if(!qa1.exec()){
            errors = tr("在设置可用货币时发送错误！");
            return false;
        }
    }
    //设置本币
    s = QString("update %1 set %2=1 where %3=%4").arg(tbl_moneyType)
            .arg(fld_mt_isMaster).arg(fld_mt_code).arg(RMB);
    if(!qa1.exec(s)){
        errors = tr("无法设置本币（人民币）！");
        return false;
    }



    //4、设置科目系统已导入配置变量值
//    QString vname = QString("%1_%2").arg(CFG_SUBSYS_IMPORT_PRE).arg(subSys);
//    s = QString("insert into %1(%2,%3) values('%4',1)").arg(tbl_cfgVariable)
//            .arg(fld_cfgv_name).arg(fld_cfgv_value).arg(vname);
//    if(!q.exec(s)){
//        errors = tr("在设置科目系统已导入配置变量时发生错误！");
//        return false;
//    }

    //5、建立常用二级科目
    s = QString("select %1,%2 from %3").arg(fld_base_ni_name).arg(fld_base_ni_belongto)
            .arg(tbl_base_ni);
    if(!qb.exec(s)){
        errors = tr("错误执行sql语句（%1）").arg(s);
        return false;
    }
    QHash<int,QStringList> maps;
    while(qb.next()){
        QString name = qb.value(0).toString();
        QStringList codes = qb.value(1).toString().split(",");
        if(codes.isEmpty())
            continue;
        s = QString("select id from %1 where %2='%3'").arg(tbl_nameItem)
                .arg(fld_ni_name).arg(name);
        if(!q.exec(s)){
            errors = tr("错误执行sql语句（%1）").arg(s);
            return false;
        }
        if(!q.first())
            continue;
        int niId = q.value(0).toInt();
        maps[niId] = codes;
    }
    s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values(:fid,:nid,1,:time,:user,'%8')")
            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_enable)
            .arg(fld_ssub_crtTime).arg(fld_ssub_creator).arg(fld_ssub_subsys)
            .arg(QString::number(subSys));
    if(!qa1.prepare(s)){
        errors = tr("错误执行sql语句（%1）").arg(s);
        return false;
    }

    //QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
    QHashIterator<int,QStringList> it(maps);
    while(it.hasNext()){
        it.next();
        foreach(QString str, it.value()){
            if(str.left(1).toInt() != subSys)
                continue;
            QString code = str.right(4);
            s = QString("select %1 from %2 where %3='%4'").arg(fld_fsub_fid).arg(tname)
                    .arg(fld_fsub_subcode).arg(code);
            if(!q.exec(s)){
                errors = tr("错误执行sql语句（%1）").arg(s);
                return false;
            }
            if(!q.first())
                continue;
            int fid = q.value(0).toInt();
            qa1.bindValue(":fid",fid);
            qa1.bindValue(":nid",it.key());
            qa1.bindValue(":time",QDateTime::currentDateTime().toString(Qt::ISODate));
            qa1.bindValue(":user",curUser->getUserId());
            if(!qa1.exec()){
                errors = tr("在插入常用科目时出错！");
                return false;
            }
        }
    }
    if(!db.commit()){
        errors = tr("为新账户导入科目系统，提交事务失败！");
        return false;
    }
    return true;
}

Account::Account(QString fname, QObject *parent):QObject(parent)
{
    isOpened = false;
    readOnly = false;
    accInfos.fileName = fname;
    if(!init())
        QMessageBox::critical(0,QObject::tr("错误信息"),QObject::tr("账户在初始化时发生错误！"));    
}

Account::~Account()
{
    qDeleteAll(suiteHash);
    qDeleteAll(suiteRecords);
    qDeleteAll(smgs);
    qDeleteAll(subSysLst);
    qDeleteAll(banks);
}

bool Account::isValid()
{
    if(!isOpened || accInfos.code.isEmpty() ||
       accInfos.sname.isEmpty() ||
       accInfos.lname.isEmpty())
        return false;
    else
        return true;
}

/**
 * @brief 指定用户是否可以访问该账户
 * @param u
 * @return
 */
bool Account::canAccess(User *u)
{
    if(u->isSuperUser() || u->isAdmin())
        return true;
    return accInfos.exclusiveUsers.contains(u);
}

void Account::close()
{
    dbUtil->saveAccountInfo(accInfos);
    dbUtil->close();
    isOpened = false;
}

bool Account::saveAccountInfo()
{
    return dbUtil->saveAccountInfo(accInfos);
}

void Account::addWaiMt(Money* mt)
{
    if(!accInfos.waiMts.contains(mt)){
        accInfos.waiMts<<mt;
        dbUtil->addMoney(mt);
    }
    if(!moneys.contains(mt->code()))
        moneys[mt->code()] = mt;
}

void Account::delWaiMt(Money* mt)
{
    //if(accInfos.waiMts.contains(mt))
        accInfos.waiMts.removeOne(mt);
    //if(moneys.contains(mt->code()))
        moneys.remove(mt->code());
        dbUtil->removeMoney(mt);
}

//获取所有外币的名称列表，用逗号分隔
QString Account::getWaiMtStr()
{
    QString t;
    foreach(Money* mt, accInfos.waiMts)
        t.append(mt->name()).append(",");
    if(!t.isEmpty())
        t.chop(1);
    return t;
}

/**
 * @brief Account::getStartTime
 *  获取账户开始记账时间
 * @return
 */
QDate Account::getStartDate()
{
    if(suiteRecords.isEmpty())
        return QDate();
    AccountSuiteRecord* asr = suiteRecords.first();
    if(asr)
        return QDate(asr->year,asr->startMonth,1);
    else
        return QDate();
}

/**
 * @brief Account::getEndTime
 *  获取账户结束记账时间
 * @return
 */
QDate Account::getEndDate()
{
    if(suiteRecords.isEmpty())
        return QDate();
    AccountSuiteRecord* asr = suiteRecords.last();
    if(asr){
        QDate d = QDate(asr->year,asr->endMonth,1);
        d.setDate(asr->year,asr->endMonth,d.daysInMonth());
        return d;
    }
    else
        return QDate();
}

/**
 * @brief Account::setEndTime
 *  设置账户结束记账时间
 * @param date
 */
void Account::setEndTime(QDate date)
{
    AccountSuiteRecord* asr = suiteRecords.last();
    if(asr->year == date.year())
        asr->endMonth = date.month();
}


/**
 * @brief Account::appendSuite
 *  添加帐套
 * @param y         帐套年份
 * @param name      帐套名
 * @param curMonth  帐套最近打开凭证集所属月份
 * @param subSys    帐套采用的科目系统代码
 */
AccountSuiteRecord* Account::appendSuiteRecord(int y, QString name, int subSys)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            return NULL;
    }
    AccountSuiteRecord* as = new AccountSuiteRecord;
    as->id = 0; as->year = y; as->name = name;
    as->isClosed = false; as->recentMonth = 1;
    as->startMonth = 1; as->endMonth = 1;as->subSys = subSys;
    as->isCur = false;
    int i = 0;
    while(i < suiteRecords.count() && y > suiteRecords.at(i)->year)
        i++;
    suiteRecords.insert(i,as);
    return as;
}

void Account::setSuiteName(int y, QString name)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            as->name = name;
    }
}

//考虑移除（只作为打开凭证集对话框类和老的账户属性配置中被使用）
QList<int> Account::getSuiteYears()
{
    QList<int> ys;
    for(int i = 0; i < suiteRecords.count(); ++i)
        ys<<suiteRecords.at(i)->year;
    return ys;
}

/**
 * @brief Account::getCurSuite
 *  获取当前帐套
 * @return
 */
AccountSuiteRecord* Account::getCurSuiteRecord()
 {
     foreach(AccountSuiteRecord* as, suiteRecords)
         if(as->isCur)
             return as;
     if(!suiteRecords.empty()){
         suiteRecords.last()->isCur = true;
         return suiteRecords.last();
     }
     return NULL;
 }

 /**
  * @brief Account::getSuite
  * 获取指定年份的帐套记录对象
  * @param y
  * @return
  */
 AccountSuiteRecord *Account::getSuiteRecord(int y)
 {
     foreach(AccountSuiteRecord* as, suiteRecords){
         if(as->year == y){
             return as;
         }
     }
     return NULL;
 }

/**
 * @brief Account::setCurSuite
 *  设置最近打开的帐套
 * @param y
 */
void Account::setCurSuite(int y)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            as->isCur = true;
        else
            as->isCur = false;
    }
}

/**
 * @brief 删除账套亦将账套的凭证及其其他相关的信息都删除，而且与其承上启下的账套都将受到影响，
 *  因此，必须严格斟酌是否有必要提供此函数
 * @param y
 */
void Account::delSuite(int y)
{
//    foreach(AccountSuite* as, accInfos.suites){
//        if(as->year == y){
//            delete as;
//            accInfos.suites.removeOne(as);
//        }
//    }
//    if(!accInfos.suites.isEmpty()){
//        accInfos.startSuite = accInfos.suites.first()->year;
//        accInfos.endSuite = accInfos.suites.last()->year;
//    }
//    else{
//        accInfos.startSuite = 0;
//        accInfos.endSuite = 0;
//    }
}

/**
 * @brief Account::getSuiteName
 *  获取帐套名
 * @param y
 * @return
 */
QString Account::getSuiteName(int y)
{
    for(int i = 0; i < suiteRecords.count(); ++i){
        if(y == suiteRecords.at(i)->year)
            return suiteRecords.at(i)->name;
    }
}

//获取当前帐套的起始月份
int Account::getSuiteFirstMonth(int y)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            return as->startMonth;
    }
    return 0;
}

//获取当前帐套的结束月份
int Account::getSuiteLastMonth(int y)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            return as->endMonth;
    }
    return 0;
}

/**
 * @brief Account::setCurMonth
 *  设置当前帐套的当前打开的凭证集的月份
 * @param m
 * @param y
 */
void Account::setCurMonth(int m, int y)
{
    if(y == 0){
        foreach(AccountSuiteRecord* as, suiteRecords){
            if(as->isCur)
                as->recentMonth = m;
        }
    }
    else{
        foreach(AccountSuiteRecord* as, suiteRecords){
            if(as->year == y)
                as->recentMonth = m;
        }
    }

}

/**
 * @brief Account::getCurMonth
 *  获取当前帐套的最近打开凭证集的月份
 * @return
 */
int Account::getCurMonth(int y)
{
    if(y == 0){
        foreach(AccountSuiteRecord* as, suiteRecords){
            if(as->isCur)
                return as->recentMonth;
        }
    }
    else{
        foreach(AccountSuiteRecord* as, suiteRecords){
            if(as->year == y)
                return as->recentMonth;
        }
    }
    return 0;
}

bool Account::saveSuite(AccountSuiteRecord *as)
{
    return dbUtil->saveSuite(as);
}

//获取账户期初年份（即开始记账的前一个月所处的年份）
int Account::getBaseYear()
{
    if(suiteRecords.isEmpty())
        return 0;
    AccountSuiteRecord* asr = suiteRecords.first();
    if(asr->startMonth == 1)
        return asr->year-1;
    else
        return asr->year;
}

//获取账户期初月份
int Account::getBaseMonth()
{
    if(suiteRecords.isEmpty())
        return 0;
    AccountSuiteRecord* asr = suiteRecords.last();
    if(asr->startMonth == 1)
        return 12;
    else
        return asr->startMonth - 1;
}

/**
 * @brief Account::getVersion
 *  获取账户文件的版本号
 * @param mv
 * @param sv
 */
void Account::getVersion(int &mv, int &sv)
{
    QStringList sl = accInfos.dbVersion.split(".");
    mv = sl.first().toInt();
    sv = sl.last().toInt();
}

/**
 * @brief 返回指定的帐套管理对象
 * @param suiteId   帐套记录id，如果为0，则返回当前帐套
 * @return
 */
AccountSuiteManager *Account::getSuiteMgr(int suiteId)
{
    if(suiteRecords.isEmpty())
        return NULL;
    if(suiteId != 0 && suiteHash.contains(suiteId))
        return suiteHash.value(suiteId);
    int key = 0;
    AccountSuiteRecord* record = NULL;
    if(suiteId == 0){
        foreach(AccountSuiteRecord* asr, suiteRecords){
            if(asr->isCur){
                key = asr->id;
                record = asr;
                break;
            }
        }
    }
    else{
        key = suiteId;
        foreach(AccountSuiteRecord* asr, suiteRecords){
            if(asr->id == key){
                record = asr;
                break;
            }
        }
    }
    if(key == 0)
        return NULL;
    if(!suiteHash.contains(key))
        suiteHash[key] = new AccountSuiteManager(record,this);
    return suiteHash.value(key);
}

/**
 * @brief Account::getSubjectManager
 *  返回指定科目系统的科目管理器对象，如果未指定科目系统代码，
 *  则返回账户最新的科目系统所对应的科目管理器对象
 * @param subSys
 * @return
 */
SubjectManager *Account::getSubjectManager(int subSys)
{
    int ssCode;
    if(subSys == 0){
        if(suiteRecords.isEmpty())
            ssCode = subSysLst.last()->code;
        else
            ssCode = suiteRecords.last()->subSys;
    }
    else
        ssCode = subSys;
    if(!smgs.contains(ssCode))
        smgs[ssCode] = new SubjectManager(this,ssCode);
    return smgs.value(ssCode);
}

/**
 * @brief 获取账户支持的科目系统
 * @return
 */
QList<SubSysNameItem *> Account::getSupportSubSys()
{
    if(subSysLst.isEmpty()){
        QList<SubSysNameItem*> items;
        AppConfig::getInstance()->getSubSysItems(items);
        SubSysNameItem* item;
        bool fonded = false;
        for(int i = 0; i < items.count(); ++i){
            item = items.at(i);
            item->isImport = isImportSubSys(item->code);
            if(!fonded && !item->isImport)
                continue;
            if(item->isImport)
                fonded = true;
            subSysLst<<item;
        }
    }
    return subSysLst;
}

/**
 * @brief Account::importNewSubSys
 *  导入新科目系统
 * @param code      科目系统代码
 * @param fname     放置新科目系统的数据库文件名
 * @return
 */
bool Account::importNewSubSys(int code)
{
    if(!dbUtil->importFstSubjects(code))
        return false;
    if(!getSubjectManager(code)->loadAfterImport())
        return false;
    foreach(SubSysNameItem* item, subSysLst){
        if(item->code == code){
            item->isImport = true;
            return true;
        }
    }
    return true;
}

bool Account::getRates(int y, int m, QHash<int, Double> &rates)
{
    return dbUtil->getRates(y,m,rates);
}

bool Account::getRates(int y, int m, QHash<Money*, Double> &rates)
{
    QHash<int,Double> rs;
    if(!dbUtil->getRates(y,m,rs))
        return false;
    QHashIterator<int,Double> it(rs);
    while(it.hasNext()){
        it.next();
        rates[moneys.value(it.key())] = it.value();
    }
    return true;
}

bool Account::setRates(int y, int m, QHash<int, Double> &rates)
{
    return dbUtil->saveRates(y,m,rates);
}

/**
 * @brief Account::getAllBankAccount
 * @return
 */
QList<BankAccount *> Account::getAllBankAccount()
{
    QList<BankAccount*> bas;
    foreach(Bank* bank, banks)
        bas<<bank->bas;
    return bas;
}

bool Account::saveBank(Bank *bank)
{
    return dbUtil->saveBankInfo(bank);
}

bool Account::removeBank(Bank *bank)
{
    banks.removeOne(bank);
    return dbUtil->saveBankInfo(bank,true);
}

bool Account::addBank(Bank *bank)
{
    banks<<bank;
    return dbUtil->saveBankInfo(bank);
}

void Account::setDatabase(QSqlDatabase *db)
{Account::db=db;}

/**
 * @brief 获取账户的科目系统对接配置信息
 * @param src   源科目系统代码
 * @param des   目的科目系统代码
 * @param cfgs  科目的对接配置列表
 * @return
 */
bool Account::getSubSysJoinCfgInfo(int src, int des, QList<SubSysJoinItem2*>& cfgs)
{
    return AppConfig::getInstance()->getSubSysMaps(src,des,cfgs);
}

bool Account::saveSubSysJoinCfgInfo(int src, int des, QList<SubSysJoinItem2 *> cfgs)
{
    return AppConfig::getInstance()->saveSubSysMaps(src,des,cfgs);
}



/**
 * @brief Account::isImportSubSys
 *  是否已经导入指定的科目系统
 * @param code  科目系统代码
 * @return
 */
bool Account::isImportSubSys(int code)
{
    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(code);
    return dbUtil->tableExist(tname);
}

/**
 * @brief 指定代码的科目系统是否已与前一个科目系统对接配置完成
 * @param code
 * @return
 */
bool Account::isSubSysConfiged(int code)
{
    if(code == DEFAULT_SUBSYS_CODE)
        return true;
    QList<SubSysNameItem *> subSyses = getSupportSubSys();
    if(subSyses.count() < 2)
        return false;
    for(int i = 1; i < subSyses.count(); ++i){
        if(subSyses.at(i)->code == code){
            bool ok;
            if(!AppConfig::getInstance()->getSubSysMapConfiged(subSyses.at(i-1)->code,code,ok))
                return false;
            return ok;
        }
    }
    return false;
}


/**
 * @brief Account::isConvertExtra
 *  在指定年份内读取前一年份的余额时，是否需要进行转换（科目id的替换）
 *  仅在跨年份读取余额时进行判断
 * @param year
 * @return
 */
bool Account::isConvertExtra(int year)
{
    AccountSuiteRecord* sc, *dc;
    sc = getSuiteRecord(year-1);
    dc = getSuiteRecord(year);
    if(!sc || !dc)
        return false;
    if(sc->subSys == dc->subSys)
        return false;
    return true;
}

/**
 * @brief Account::convertExtra
 *  用指定的科目映射表转换余额
 *  在执行过科目合并后，可能存在这样一种情况：即被合并科目在转换前如果存在余额，在合并后，要考虑将这些
 *  转换前的合并科目汇总到一个保留科目上（不能简单地替换，这样会导致余额覆盖）
 * @param sums  余额
 * @param maps  科目映射表
 * @return
 */
//bool Account::convertExtra(QHash<int, Double> &sums, QHash<int,MoneyDirection>& dirs,const QHash<int, int> maps)
//{
//    QHashIterator<int, Double> it(sums);
//    int key, id, mt;
//    Double v;
//    MoneyDirection d;
//    while(it.hasNext()){
//        it.next();
//        v = it.value();
//        d = dirs.value(it.key());
//        id = it.key()/10;
//        mt = it.key()%10;
//        if(!maps.contains(id))
//            return false;
//        key = maps.value(id) * 10 + mt;
//        sums.remove(it.key());
//        dirs.remove(it.key());
//        if(!sums.contains(key)){
//            sums[key] = v;
//            dirs[key] = d;
//        }
//        else{
//            if(dirs.value(key) == d)
//                sums[key] += v;
//            else{
//                Double v1 = sums.value(key);
//                v.changeSign();
//                v1 += v;
//                if(v1==0){
//                    sums.remove(key);
//                    dirs.remove(key);
//                }
//                else if(v1 > 0){
//                    sums[key] = v1;
//                    dirs[key] = MDIR_J;
//                }
//                else{
//                    v1.changeSign();
//                    sums[key] = v1;
//                    dirs[key] = MDIR_D;
//                }
//            }
//        }

//    }
//    return true;
//}

/**
 * @brief 转换余额，当跨帐套读取期初余额，如果恰好发生了科目系统的升级，则必须对余额进行转换
 *        因为，有些科目被取消并合并到新科目上。这样旧的科目余额也要合并到新科目上
 * @param year  余额所在年份
 * @param fsums 主目余额
 * @param fdirs 主目余额方向
 * @param dsums 子目余额
 * @param ddirs 子目余额方向
 * @return
 */
bool Account::convertExtra2(int year, QHash<int, Double> &fsums, QHash<int, MoneyDirection> &fdirs, QHash<int, Double> &dsums, QHash<int, MoneyDirection> &ddirs)
{
    int sc = getSuiteRecord(year)->subSys;
    int dc = getSuiteRecord(year+1)->subSys;
    if(sc==dc)
        return true;
    QList<MixedJoinCfg*> cfgs;
    if(!dbUtil->getMixJoinInfo(sc,dc,cfgs)){
        LOG_ERROR(tr("在转换余额时，无法获取混合对接配置信息！"));
        return false;
    }
    //因为有确信的把握这些混合对接科目不使用外币，因此余额转换只涉及到本币余额项
    int fid = 0,key1,key2;
    int mmt = getMasterMt()->code();
    for(int i = 0; i < cfgs.count(); ++i){
        MixedJoinCfg* item = cfgs.at(i);
        //转换主目的余额
        if(fid != item->s_fsubId){
            fid = item->s_fsubId;
            key1 = fid * 10 + mmt;
            if(fsums.contains(key1)){
                Double v = fsums.value(key1);
                if(fdirs.value(key1,MDIR_P) == MDIR_D)
                    v.changeSign();
                key2 = item->d_fsubId * 10 + mmt;
                Double sum = fsums.value(key2);
                if(fdirs.value(key2,MDIR_P) == MDIR_D)
                    sum.changeSign();
                sum += v;
                if(v == 0){
                    fsums.remove(key2);
                    fdirs.remove(key2);
                }
                else if(v < 0){
                    sum.changeSign();
                    fdirs[key2] = MDIR_D;
                }
                else
                    fdirs[key2] = MDIR_J;
                fsums[key2] = sum;
                fsums.remove(key1);
                fdirs.remove(key1);
            }
        }
        //转换子目余额
        key1 = item->s_ssubId * 10 + mmt;
        if(dsums.contains(key1)){
            Double v = dsums.value(key1);
            if(ddirs.value(key1,MDIR_P) == MDIR_D)
                v.changeSign();
            key2 = item->d_ssubId * 10 + mmt;
            Double sum = dsums.value(key2);
            if(ddirs.value(key2,MDIR_P) == MDIR_D)
                sum.changeSign();
            sum += v;
            if(v == 0){
                dsums.remove(key2);
                ddirs.remove(key2);
            }
            else if(v < 0){
                sum.changeSign();
                ddirs[key2] = MDIR_D;
            }
            else
                ddirs[key2] = MDIR_J;
            dsums[key2] = sum;
            dsums.remove(key1);
            ddirs.remove(key1);
        }
    }

//    QHash<QString,QString> maps;
//    if(!AppConfig::getInstance()->getNotDefSubSysMaps(sc,dc,maps))
//        return false;
//    if(maps.isEmpty())
//        return true;
//    QList<int> mtIds = getAllMoneys().keys();
//    SubjectManager* sm1 = getSubjectManager(sc);
//    SubjectManager* sm2 = getSubjectManager(dc);
//    QHashIterator<QString,QString> it(maps);
//    while(it.hasNext()){
//        it.next();
//        FirstSubject* fsub1 = sm1->getFstSubject(it.key());
//        if(!fsub1){
//            LOG_ERROR(tr("在转换余额时，发现源科目（%）对象不能找到！").arg(it.key()));
//            return false;
//        }
//        FirstSubject* fsub2 = sm2->getFstSubject(it.value());
//        if(!fsub2){
//            LOG_ERROR(tr("在转换余额时，发现对接科目（%）对象不能找到！").arg(it.key()));
//            return false;
//        }
//        foreach(int mt, mtIds){
//            int key1 = fsub1->getId() * 10 + mt;
//            if(fsums.contains(key1)){
//                Double v = fsums.value(key1);
//                if(fdirs.value(key1,MDIR_P) == MDIR_D)
//                    v.changeSign();
//                int key2 = fsub2->getId() * 10 + mt;
//                Double sum = fsums.value(key2);
//                if(fdirs.value(key2,MDIR_P) == MDIR_D)
//                    sum.changeSign();
//                sum += v;
//                if(v == 0){
//                    fsums.remove(key2);
//                    fdirs.remove(key2);
//                }
//                else if(v < 0){
//                    sum.changeSign();
//                    fdirs[key2] = MDIR_D;
//                }
//                else
//                    fdirs[key2] = MDIR_J;
//                fsums[key2] = sum;
//                fsums.remove(key1);
//                fdirs.remove(key2);
//            }
//        }
//    }
    return true;
}

/**
 * @brief Account::convertExtra
 *  对余额表进行科目id的替换
 * @param year
 * @param fsums
 * @param fdirs
 * @param ssums
 * @param sdirs
 * @return
 */
//bool Account::convertExtra(int year, QHash<int, Double> &fsums, QHash<int, MoneyDirection> &fdirs,
//                           QHash<int, Double> &ssums, QHash<int, MoneyDirection> &sdirs)
//{
//    AccountSuiteRecord* sc, dc;
//    sc = getSuite(year-1);
//    dc = getSuite(year);
//    if(!sc || !dc)
//        return false;
//    if(sc->subSys == dc.subSys)
//        return false;
//    QList<SubSysJoinItem*> jItems;
//    if(!account->getSubSysJoinCfgInfo(sc,dc,jItems))
//        return false;

//    //建立主目和子目的id映射表
//    QHash<int,int> fsubMaps,ssubMaps;
//    getSubSysJoinMaps(sc->subSys,dc.subSys,fsubMaps,ssubMaps);
//    foreach(SubSysJoinItem* item,jItems){

//    //处理主目余额及其方向

//    int id,mt,key;
//    Double v;
//    MoneyDirection d;
//    if(!fsums.isEmpty()){
//        QHashIterator<int,Double> it(fsums);
//        while(it.hasNext()){
//            it.next();
//            v = it.value();
//            d = fdirs.value(it.key());
//            id = it.key()/10;
//            mt = it.key()%10;
//            if(!fsubMaps.contains(id)){
//                FirstSubject* fsub = getSubjectManager(sc)->getFstSubject(id);
//                QMessageBox::warning(this,tr("警告信息"),tr("在衔接原币余额时，发现一个未建立衔接映射的一级科目（%1），操作无法继续！").arg(fsub->getName()));
//                return false;
//            }
//            key = fsubMaps.value(id) * 10 + mt;
//            //注意：如果有多个源科目被映射到同一个目的科目，则要考虑汇总余额，并最终确定方向，但不知是否会出现此种情形
//            fsums.remove(it.key());
//            fdirs.remove(it.key());
//            fsums[key] = v;
//            fdirs[key] = d;
//        }
//    }

//    if(!ssums.isEmpty()){
//        QHashIterator<int,Double> it(ssums);
//        while(it.hasNext()){
//            it.next();
//            v = it.value();
//            d = fdirs.value(it.key());
//            id = it.key()/10;
//            mt = it.key()%10;
//            if(!ssubMaps.contains(id)){
//                SecondSubject* ssub = getSubjectManager(sc)->getSndSubject(id);
//                QMessageBox::warning(this,tr("警告信息"),tr("在衔接原币余额时，发现一个未建立衔接映射的一级科目（%1），操作无法继续！").arg(ssub->getName()));
//                return false;
//            }
//            key = ssubMaps.value(id) * 10 + mt;
//            //注意：如果有多个源科目被映射到同一个目的科目，则要考虑汇总余额，并最终确定方向，但不知是否会出现此种情形
//            ssums.remove(it.key());
//            sdirs.remove(it.key());
//            ssums[key] = v;
//            sdirs[key] = d;
//        }
//    }
//    return true;
//}



/**
 * @brief Account::init
 *  账户对象初始化
 * @return
 */
bool Account::init()
{
    accInfos.masterMt = NULL;

    dbUtil = new DbUtil;
    bool ok = true;
    if(!dbUtil->setFilename(accInfos.fileName)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Can't connect to account file!"));
        ok = false;
    }
    if(ok && !dbUtil->initMoneys(this)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Money init happen error!"));
        ok = false;
    }
    if(ok && !dbUtil->initAccount(accInfos)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Account info init happen error!"));
        ok = false;
    }
    if(ok && !dbUtil->initSuites(suiteRecords))    {
        LOG_ERROR("Initial account suite happen error!");
        ok = false;
    }
    if(ok && !dbUtil->initNameItems()){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Name items init happen error!"));
        ok = false;
    }
    if(ok && !dbUtil->initBanks(this)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Bank init happen error!"));
        ok = false;
    }

    if(ok){
        isOpened = true;
        return true;
    }
    else{
        dbUtil->close();
        return false;
    }
}


//获取帐套内凭证集的开始、结束月份
bool Account::getSuiteMonthRange(int y,int& sm, int &em)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y){
            sm = as->startMonth;
            em = as->endMonth;
            return true;
        }
    }
    return false;
}

bool Account::containSuite(int y)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            return true;
    }
    return false;
}



QString Account::getAllLogs()
{

}

QString Account::getLog(QDateTime time)
{

}

void Account::appendLog(QDateTime time, QString log)
{

}

void Account::delAllLogs()
{

}

void Account::delLogs(QDateTime start, QDateTime end)
{

}

////////////////////////////////////////////////////////////////
bool byAccountSuiteThan(AccountSuiteRecord *as1, AccountSuiteRecord *as2)
{
    return as1->year < as2->year;
}


bool AccountSuiteRecord::operator !=(const AccountSuiteRecord& other)
{
    if(year != other.year)
        return true;
    else if(recentMonth != other.recentMonth)
        return true;
    else if(subSys != other.subSys)
        return true;
    else if(name != other.name)
        return true;
    else if(isClosed != other.isClosed)
        return true;
    return false;
}
