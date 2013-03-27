#include <QDebug>
#include <QSqlError>

#include "dbutil.h"
#include "global.h"
#include "tables.h"
#include "account.h"
#include "logs/Logger.h"
#include "subject.h"

DbUtil::DbUtil()
{
    //初始化账户信息片段名称表
    pNames[ACODE] = "accountCode";
    pNames[SNAME] = "shortName";
    pNames[LNAME] = "longName";
    //pNames[RPTTYPE] = "reportType";
    pNames[MASTERMT] = "masterMt";
    pNames[WAIMT] = "WaiBiList";
    pNames[STIME] = "startTime";
    pNames[ETIME] = "endTime";
    pNames[CSUITE] = "currentSuite";
    pNames[SUITENAME] = "suiteNames";
    pNames[LASTACCESS] = "lastAccessTime";
    pNames[LOGFILE] = "logFileName";
    pNames[DBVERSION] = "db_version";

}

DbUtil::~DbUtil()
{
    if(db.isOpen())
        close();
}

/**
 * @brief DbUtil::setFilename
 *  设置账户文件名，并打开该连接
 * @param fname
 * @return
 */
bool DbUtil::setFilename(QString fname)
{
    if(db.isOpen())
        close();
    fileName = fname;
    QString name = DatabasePath+fname;
    db = QSqlDatabase::addDatabase("QSQLITE",AccConnName);
    db.setDatabaseName(name);
    if(!db.open()){
        qDebug()<<db.lastError();
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::colse
 * 关闭并移除账户数据库的连接
 */
void DbUtil::close()
{
    db.close();
    QSqlDatabase::removeDatabase(AccConnName);
}

/**
 * @brief DbUtil::readAccBriefInfo
 *  读取账户的简要信息
 * @param info
 * @return
 */
bool DbUtil::readAccBriefInfo(AccountBriefInfo &info)
{
    QSqlQuery q(db);
    QString s = QString("select %1,%2 from %3")
            .arg(fld_acci_code).arg(fld_acci_value).arg(tbl_accInfo);
    if(!q.exec(s))
        return false;
    InfoField code;
    while(q.next()){
        code = (InfoField)q.value(0).toInt();
        switch(code){
        case ACODE:
            info.code = q.value(1).toString();
            break;
        case SNAME:
            info.sname = q.value(1).toString();
            break;
        case LNAME:
            info.lname = q.value(1).toString();
            break;
        }
    }
    info.isRecent = false;
    info.fname = fileName;
    return true;
}

/**
 * @brief DbUtil::initAccount
 *  从账户数据库文件中读取账户信息到infos结构中
 * @param infos
 * @return
 */
bool DbUtil::initAccount(Account::AccountInfo &infos)
{
    QSqlQuery q(db);
    QString s = QString("select %1,%2 from %3")
            .arg(fld_acci_code).arg(fld_acci_value).arg(tbl_accInfo);
    if(!q.exec(s))
        return false;

    InfoField code;
    QStringList sl;
    while(q.next()){
        code = (InfoField)q.value(0).toInt();
        switch(code){
        case ACODE:
            infos.code = q.value(1).toString();
            break;
        case SNAME:
            infos.sname = q.value(1).toString();
            break;
        case LNAME:
            infos.lname = q.value(1).toString();
            break;
            break;
        case MASTERMT:
            infos.masterMt = q.value(1).toInt();
            break;
        case WAIMT:
            sl.clear();
            sl = q.value(1).toString().split(",");
            foreach(QString v, sl)
                infos.waiMts<<v.toInt();
            break;
        case STIME:
            infos.startDate = q.value(1).toString();
            break;
        case ETIME:
            infos.endDate = q.value(1).toString();
            break;
        case LOGFILE:
            infos.logFileName = q.value(1).toString();
            break;
        case DBVERSION:
            infos.dbVersion = q.value(1).toString();
            break;
        }
    }

    //如果表内的信息字段内容不全，则需要提供默认值，以使对象的属性具有意义
    if(infos.masterMt == 0)
        infos.masterMt = RMB;
    //默认，日志文件名同账户文件名同名，但扩展名不同
    if(infos.logFileName.isEmpty()){
        infos.logFileName = fileName;
        infos.logFileName = infos.logFileName.replace(".dat",".log");
    }
    if(infos.endDate.isEmpty()){
        QDate d = QDate::currentDate();
        int y = d.year();
        int m = d.month();
        int sd = 1;
        int ed = d.daysInMonth();
        infos.startDate = QDate(y,m,sd).toString(Qt::ISODate);
        infos.endDate = QDate(y,m,ed).toString(Qt::ISODate);
    }
    if(infos.endDate.isEmpty()){
        QDate d = QDate::fromString(infos.startDate,Qt::ISODate);
        int days = d.daysInMonth();
        d.setDate(d.year(),d.month(),days);
        infos.endDate = d.toString(Qt::ISODate);
    }

    //读取账户的帐套信息
    if(!readAccountSuites(infos.suites))
        return false;
    //完善帐套的起止月份
    infos.suites.first()->startMonth = QDate::fromString(infos.startDate).month();
    infos.suites.last()->endMonth = QDate::fromString(infos.endDate).month();
    if(infos.suites.count() > 2){   //中间年份的起止月份都是1-12月
        for(int i = 1; i < infos.suites.count()-1; ++i){
            infos.suites.at(i)->startMonth = 1;
            infos.suites.at(i)->endMonth = 12;
        }
    }
    return true;
}

/**
 * @brief DbUtil::saveAccountInfo
 *  将账户信息保存的账户文件中
 * @param infos
 * @return
 */
bool DbUtil::saveAccountInfo(Account::AccountInfo &infos)
{
    //基本实现思路先读取一份账户文件中的的信息，与传入的参数进行比较，在发生变化的情况下进行更新或插入操作
    Account::AccountInfo oldInfos;
    if(!initAccount(oldInfos))
        return false;

    if(infos.code != oldInfos.code && !saveAccInfoPiece(ACODE,infos.code))
        return false;
    if(infos.sname != oldInfos.sname && !saveAccInfoPiece(SNAME,infos.sname))
        return false;
    if(infos.lname != oldInfos.lname && !saveAccInfoPiece(LNAME,infos.lname))
        return false;
    if(infos.masterMt != oldInfos.masterMt && !saveAccInfoPiece(MASTERMT,QString::number(infos.masterMt)))
        return false;
    if(infos.startDate != oldInfos.startDate && !saveAccInfoPiece(STIME,infos.startDate))
        return false;
    if(infos.endDate != oldInfos.endDate && !saveAccInfoPiece(ETIME,infos.endDate))
        return false;
    if(infos.lastAccessTime != oldInfos.lastAccessTime && !saveAccInfoPiece(LASTACCESS,infos.lastAccessTime))
        return false;
    if(infos.dbVersion != oldInfos.dbVersion && !saveAccInfoPiece(DBVERSION,infos.dbVersion))
        return false;
    if(infos.logFileName != oldInfos.logFileName && !saveAccInfoPiece(LOGFILE,infos.logFileName))
        return false;
    bool changed = false;
    //保存外币列表
    if(infos.waiMts.count() != oldInfos.waiMts.count())
        changed = true;
    else{
        for(int i = 0; i < infos.waiMts.count(); ++i){
            if(infos.waiMts.at(i) != oldInfos.waiMts.at(i)){
                changed = true;
                break;
            }
        }
    }
    if(changed){
        QString vs;
        for(int i = 0; i < infos.waiMts.count(); ++i)
            vs.append(QString("%1,").arg(infos.waiMts.at(i)));
        vs.chop(1);
        if(!saveAccInfoPiece(WAIMT,vs))
            return false;
        changed = false;
    }
    //保存帐套名表
    if(!saveAccountSuites(infos.suites))
        return false;
    return true;
}

/**
 * @brief DbUtil::initNameItems
 *  初始化科目管理器使用的名称条目
 * @return
 */
bool DbUtil::initNameItems()
{
    QSqlQuery q(db);
    QString s;

    //1、装载名称条目类别（有关名称条目的信息只需装载一次）
    s = QString("select * from %1").arg(tbl_nameItemCls);
    if(!q.exec(s))
        return false;
    while(q.next()){
        int code = q.value(NICLASS_CODE).toInt();
        SubjectManager::nameItemCls[code] = QStringList();
        SubjectManager::nameItemCls[code].append(q.value(NICLASS_NAME).toString());
        SubjectManager::nameItemCls[code].append(q.value(NICLASS_EXPLAIN).toString());
    }


    //4、装载所有二级科目名称条目
    s = QString("select * from %1").arg(tbl_nameItem);
    if(!q.exec(s))
        return false;

    SubjectNameItem* item;
    while(q.next()){
        int id = q.value(0).toInt();
        int clsId = q.value(NI_CALSS).toInt();
        QString sname = q.value(NI_NAME).toString();
        QString lname = q.value(NI_LNAME).toString();
        QString remCode = q.value(NI_REMCODE).toString();
        QDateTime crtTime = QDateTime::fromString(q.value(NI_CREATERTIME).toString(),Qt::ISODate);
        int uid = q.value(NI_CREATOR).toInt();
        item = new SubjectNameItem(id,clsId,sname,lname,remCode,crtTime,allUsers.value(uid));
        SubjectManager::nameItems[id]=item;
    }
    return true;
}

/**
 * @brief DbUtil::initSubjects
 *  初始化科目管理器对象
 * @param smg       科目管理器对象指针
 * @param subSys    科目系统代码
 * @return
 */
bool DbUtil::initSubjects(SubjectManager *smg, int subSys)
{
    QSqlQuery q(db);
    QString s;

    //1、装载一级科目类别
    s = QString("select %1,%2 from %3 where %4=%5").arg(fld_fsc_code)
            .arg(fld_fsc_name).arg(tbl_fsclass).arg(fld_fsc_subSys).arg(subSys);
    if(!q.exec(s))
        return false;
    while(q.next())
        smg->fstSubCls[q.value(0).toInt()] = q.value(1).toString();

    //2、装载所有一级科目
    s = QString("select * from %1 where %2=%3 order by %4")
            .arg(tbl_fsub).arg(fld_fsub_subSys).arg(subSys).arg(fld_fsub_subcode);
    if(!q.exec(s))
        return false;

    QString name,code,remCode,explain,usage;
    bool jdDir,isUseWb;
    int id, subCls,weight;
    FirstSubject* fsub;

    while(q.next()){
        id = q.value(0).toInt();
        subCls = q.value(FSUB_CLASS).toInt();
        name = q.value(FSUB_SUBNAME).toString();
        code = q.value(FSUB_SUBCODE).toString();
        remCode = q.value(FSUB_REMCODE).toString();
        weight = q.value(FSUB_WEIGHT).toInt();
        jdDir = q.value(FSUB_DIR).toBool();
        isUseWb = q.value(FSUB_ISUSEWB).toBool();
        //读取explain和usage的内容，目前暂不支持（将来这两个内容将保存在另一个表中）
        //s = QString("select * from ")
        fsub = new FirstSubject(id,subCls,name,code,remCode,weight,jdDir,isUseWb,explain,usage,subSys);
        smg->fstSubs<<fsub;
        smg->fstSubHash[id]=fsub;

        //设置特定科目对象
        AppConfig* conf = AppConfig::getInstance();
        if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_CASH))
            smg->cashSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_BANK))
            smg->bankSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_GDZC))\
            smg->gdzcSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_CWFY))
            smg->cwfySub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_BNLR))
            smg->bnlrSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_LRFP))
            smg->lrfpSub = fsub;
    }

    //3、装载所有二级科目
    s = QString("select * from %1").arg(tbl_ssub);
    if(!q.exec(s))
        return false;

    SecondSubject* ssub;
    while(q.next()){
        int id = q.value(0).toInt();
        int fid = q.value(SSUB_FID).toInt();
        fsub = smg->fstSubHash.value(fid);
        if(!fsub){
            LOG_INFO(QObject::tr("Find a second subject(id=%1) don't belong to any first subject!").arg(id));
            continue;
            //return false;
        }
        int sid = q.value(SSUB_NID).toInt();
        if(!smg->nameItems.contains(sid)){
            LOG_INFO(QObject::tr("Find a name item(id=%1,fid=%2 %3,sid=%4) don't exist!")
                     .arg(id).arg(fid).arg(fsub->getName()).arg(sid));
            continue;
            //return false;
        }
        code = q.value(SSUB_SUBCODE).toString();
        weight = q.value(SSUB_WEIGHT).toInt();
        bool isEnable = q.value(SSUB_ENABLED).toBool();
        QDateTime crtTime = QDateTime::fromString(q.value(SSUB_CREATETIME).toString(),Qt::ISODate);
        QDateTime disTime = QDateTime::fromString(q.value(SSUB_DISABLETIME).toString(),Qt::ISODate);
        int uid = q.value(SSUB_CREATOR).toInt();
        ssub = new SecondSubject(fsub,id,smg->nameItems.value(sid),code,weight,isEnable,crtTime,disTime,allUsers.value(uid));
        smg->sndSubs[id] = ssub;
        if(ssub->getWeight() == DEFALUTSUBFS)
            fsub->setDefaultSubject(ssub);
    }
    //初始化银行账户信息
    //if(!getAllBankAccount(banks))
    //    return false;
    return true;
}

/**
 * @brief DbUtil::initMoneys
 *  初始化当前账户所使用的所有货币对象
 * @param moneys
 * @return
 */
bool DbUtil::initMoneys(QHash<int, Money *> &moneys)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_moneyType);
    if(!q.exec(s))
        return false;
    while(q.next()){
        int code = q.value(MT_CODE).toInt();
        QString sign = q.value(MT_SIGN).toString();
        QString name = q.value(MT_NAME).toString();
        moneys[code] = new Money(code,name,sign);
    }
    return true;
}

/**
 * @brief DbUtil::initBanks
 *  初始化账户使用的银行账户及其对应的科目
 * @param banks
 * @return
 */
bool DbUtil::initBanks(Account *account)
{
    QSqlQuery q(db);

    QHash<int,Bank*> banks;
    QString s = QString("select * from %1").arg(tbl_bank);
    if(!q.exec(s))
        return false;
    int id;
    while(q.next()){
        Bank* bank = new Bank;
        bank->id = q.value(0).toInt();
        bank->isMain = q.value(BANK_ISMAIN).toBool();
        bank->name = q.value(BANK_NAME).toString();
        bank->lname = q.value(BANK_LNAME).toString();
        banks[bank->id] = bank;
    }

    s = QString("select * from %1").arg(tbl_bankAcc);
    if(!q.exec(s))
        return false;
    while(q.next()){
        BankAccount* ba = new BankAccount;
        ba->id = q.value(0).toInt();
        int bankId = q.value(BA_BANKID).toInt();
        int mt = q.value(BA_MT).toInt();
        int nid = q.value(BA_ACCNAME).toInt();
        ba->bank = banks.value(bankId);
        ba->accNumber = q.value(BA_ACCNUM).toString();
        ba->mt = account->moneys.value(mt);
        ba->niObj = SubjectManager::getNameItem(nid);
        account->bankAccounts<<ba;
    }
    return true;
}

/**
 * @brief DbUtil::readExtraForPm
 *  读取指定年月的余额值（原币形式）
 * @param y     年
 * @param m     月
 * @param fsums 一级科目余额值（注意：hash表的键是 “科目代码 * 10 + 币种代码”）
 * @param fdirs 一级科目余额方向
 * @param ssums 二级科目余额值
 * @param sdirs 二级科目方向
 * @return
 */
bool DbUtil::readExtraForPm(int y, int m, QHash<int, Double> &fsums, QHash<int, MoneyDirection> &fdirs,
                            QHash<int, Double> &ssums, QHash<int, MoneyDirection> &sdirs)
{
    if(!_readExtraForPm(y,m,fsums,fdirs))
        return false;
    if(!_readExtraForPm(y,m,ssums,sdirs,false))
        return false;
    return true;
}

/**
 * @brief DbUtil::readExtraForMm
 *  读取指定年月的余额值（本币形式）
 * @param y     年
 * @param m     月
 * @param fsums 一级科目余额值（注意：hash表的键是 “科目代码 * 10 + 币种代码”）
 * @param fdirs 一级科目余额方向
 * @param ssums 二级科目余额值
 * @param sdirs 二级科目方向
 * @param m
 * @param fsums
 * @param fdirs
 * @param ssums
 * @param sdirs
 * @return
 */
bool DbUtil::readExtraForMm(int y, int m, QHash<int, Double> &fsums, QHash<int, Double> &ssums)
{
    if(!_readExtraForMm(y,m,fsums))
        return false;
    if(!_readExtraForMm(y,m,ssums,false))
        return false;
    return true;
}

/**
 * @brief DbUtil::saveExtraForPm
 *  保存指定年月的期末余额（原币形式）
 * @param y     年
 * @param m     月
 * @param fsums 一级科目余额表（注意：键为“科目id * 10 + 币种代码”）
 * @param fdirs 一级科目余额方向
 * @param ssums 二级科目余额表
 * @param sdirs 二级科目余额方向
 * @return
 */
bool DbUtil::saveExtraForPm(int y, int m, const QHash<int, Double> &fsums, const QHash<int, MoneyDirection> &fdirs, const QHash<int, Double> &ssums, const QHash<int, MoneyDirection> &sdirs)
{
    if(!_saveExtrasForPm(y,m,fsums,fdirs))
        return false;
    if(!_saveExtrasForPm(y,m,ssums,sdirs,false))
        return false;
    return true;
}

/**
 * @brief DbUtil::saveExtraForMm
 *  保存指定年月的期末余额（本币形式）
 * @param y
 * @param m
 * @param fsums 一级科目余额表（注意：键为“科目id * 10 + 币种代码”）
 * @param ssums 二级科目余额表
 * @return
 */
bool DbUtil::saveExtraForMm(int y, int m, const QHash<int, Double> &fsums, const QHash<int, Double> &ssums)
{
    if(!_saveExtrasForMm(y,m,fsums))
        return false;
    if(!_saveExtrasForMm(y,m,ssums,false))
        return false;
    return true;
}

/**
 * @brief DbUtil::saveAccInfoPiece
 *  保存账户信息片段
 * @param code
 * @param value
 * @return
 */
bool DbUtil::saveAccInfoPiece(DbUtil::InfoField code, QString value)
{
    QSqlQuery q(db);
    QString s = QString("update %1 set %2='%3' where %4=%5")
            .arg(tbl_accInfo).arg(fld_acci_value).arg(value)
            .arg(fld_acci_code).arg(code);
    if(!q.exec(s))
        return false;
    if(q.numRowsAffected() == 0){
        s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')")
                .arg(tbl_accInfo).arg(fld_acci_code).arg(fld_acci_name)
                .arg(fld_acci_value).arg(code).arg(pNames.value(code)).arg(value);
        if(!q.exec(s))
            return false;
    }
    return true;
}

/**
 * @brief DbUtil::readAccountSuites
 *  读取帐套信息
 * @param suites
 * @return
 */
bool DbUtil::readAccountSuites(QList<Account::AccountSuite *> &suites)
{
    //读取账户的帐套信息
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2").arg(tbl_accSuites).arg(fld_accs_year);
    if(!q.exec(s))
        return false;
    Account::AccountSuite* as;
    while(q.next()){
        as = new Account::AccountSuite;
        as->id = q.value(0).toInt();
        as->year = q.value(ACCS_YEAR).toInt();
        as->subSys = q.value(ACCS_SUBSYS).toInt();
        as->lastMonth = q.value(ACCS_RECENTMONTH).toInt();
        as->isCur = q.value(ACCS_ISCUR).toBool();
        as->name = q.value(ACCS_NAME).toString();
        suites<<as;
    }
    return true;
}

/**
 * @brief DbUtil::saveAccountSuites
 *  保存帐套信息
 * @param suites
 * @return
 */
bool DbUtil::saveAccountSuites(QList<Account::AccountSuite *> suites)
{
    QString s;
    QSqlQuery q(db);
    if(!db.transaction()){
        Logger::write(QDateTime::currentDateTime(),Logger::Error,"",0,"", QObject::tr("Start transaction failed!"));
        return false;
    }
    QList<Account::AccountSuite *> oldSuites;
    if(!readAccountSuites(oldSuites))
        return false;
    QHash<int,Account::AccountSuite*> oldHash;
    foreach(Account::AccountSuite* as, oldSuites)
        oldHash[as->id] = as;
    foreach(Account::AccountSuite* as, suites){
        if(as->id == 0)
            s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,%8,'%9',%10,%11)")
                    .arg(tbl_accSuites).arg(fld_accs_year).arg(fld_accs_subSys)
                    .arg(fld_accs_name).arg(fld_accs_recentMonth).arg(fld_accs_isCur)
                    .arg(as->year).arg(as->subSys).arg(as->name).arg(as->lastMonth)
                    .arg(as->isCur?1:0);
        else if(*as != *(oldHash.value(as->id)))
            s = QString("update %1 set %2=%3,%4=%5,%6='%7',%8=%9,%10=%11 where id=%12")
                    .arg(tbl_accSuites).arg(fld_accs_year).arg(as->year)
                    .arg(fld_accs_subSys).arg(as->subSys).arg(fld_accs_name)
                    .arg(as->name).arg(fld_accs_recentMonth).arg(as->lastMonth)
                    .arg(fld_accs_isCur).arg(as->isCur?1:0).arg(as->id);

        if(!s.isEmpty()){
            q.exec(s);
            s.clear();
        }
    }

    if(!db.commit()){
        Logger::write(QDateTime::currentDateTime(),Logger::Error,"",0,"", QObject::tr("Transaction commit failed!"));
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::readExtraPoint
 *  读取指定年月的余额指针Pid
 * @param y
 * @param m
 * @param mtHashs   键为币种代码，值为余额指针pid
 * @return
 */
bool DbUtil::_readExtraPoint(int y, int m, QHash<int, int>& mtHashs)
{
    QSqlQuery q(db);
    //1、首先取得保存指定年月余额值的指针id
    QString s = QString("select id,%1 from %2 where %3=%4 and %5=%6")
            .arg(fld_nse_mt).arg(tbl_nse_point)
            .arg(fld_nse_year).arg(y).arg(fld_nse_month).arg(m);
    if(!q.exec(s))
        return false;
    while(q.next())
        mtHashs[q.value(1).toInt()] = q.value(0).toInt();
    return true;
}

/**
 * @brief DbUtil::_readExtraForPm
 *  读取指定年月的余额（原币形式）
 * @param y     年
 * @param m     月
 * @param sums  余额值表
 * @param dirs  余额方向表
 * @param isFst 是一级科目（true：默认值）还是二级科目
 * @return
 */
bool DbUtil::_readExtraForPm(int y, int m, QHash<int, Double> &sums, QHash<int, MoneyDirection> &dirs, bool isFst)
{
    QSqlQuery q(db);
    QHash<int,int> mtHash;  //键为币种代码，值为余额指针pid

    //1、首先取得保存指定年月余额值的指针id
    if(!_readExtraPoint(y,m,mtHash))
        return false;
    if(mtHash.isEmpty())
        return true;

    //2、读取各币种的余额值
    QString s;
    QHashIterator<int,int> it(mtHash);
    int sid,key; Double v;
    MoneyDirection dir;
    QString tname;
    if(isFst)
        tname = tbl_nse_p_f;
    else
        tname = tbl_nse_p_s;
    while(it.hasNext()){
        it.next();
        s = QString("select * from %1 where %2=%3")
                .arg(tname).arg(fld_nse_pid).arg(it.value());
        if(!q.exec(s))
            return false;
        while(q.next()){
            sid = q.value(NSE_E_SID).toInt();
            dir = (MoneyDirection)q.value(NSE_E_DIR).toInt();
            v = Double(q.value(NSE_E_VALUE).toDouble());
            key = sid * 10 + it.key();
            sums[key] = v;
            dirs[key] = dir;
        }
    }
    return true;
}

/**
 * @brief DbUtil::_readExtraForMm
 *  读取指定年月的余额（本币形式）
 * @param y
 * @param m
 * @param sums  余额值表
 * @param isFst 是一级科目（true：默认值）还是二级科目
 * @return
 */
bool DbUtil::_readExtraForMm(int y, int m, QHash<int, Double> &sums, bool isFst)
{
    QSqlQuery q(db);
    QHash<int,int> mtHash;  //键为币种代码，值为余额指针pid

    //1、首先取得保存指定年月余额值的指针id
    if(!_readExtraPoint(y,m,mtHash))
        return false;
    if(mtHash.isEmpty())
        return true;
    //因为只有需要外币的科目才会保存本币形式的余额，因此只有存在外币的本币余额项时才继续读取
    mtHash.remove(curAccount->getMasterMt());
    if(mtHash.isEmpty())
        return true;

    //2、读取各币种的余额值
    QString s;
    QHashIterator<int,int> it(mtHash);
    int sid,key; Double v;
    QString tname;
    if(isFst)
        tname = tbl_nse_m_f;
    else
        tname = tbl_nse_m_s;
    while(it.hasNext()){
        it.next();
        s = QString("select * from %1 where %2=%3")
                .arg(tname).arg(fld_nse_pid).arg(it.value());
        if(!q.exec(s))
            return false;
        while(q.next()){
            sid = q.value(NSE_E_SID).toInt();
            v = Double(q.value(NSE_E_VALUE).toDouble());
            key = sid * 10 + it.key();
            sums[key] = v;
        }
    }
    return true;
}

/**
 * @brief DbUtil::_crtExtraPoint
 *  创建余额指针
 * @param y
 * @param m
 * @param mt    币种代码
 * @param pid   余额指针
 * @return
 */
bool DbUtil::_crtExtraPoint(int y, int m, int mt, int &pid)
{
    //如果此函数在一个数据库事务中被调用，是否要启动它自己的内嵌事务以取得记录id？
    QSqlQuery q(db);
    QString s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)")
            .arg(tbl_nse_point).arg(fld_nse_year).arg(fld_nse_month)
            .arg(fld_nse_mt).arg(y).arg(m).arg(mt);
    if(!q.exec(s))
        return false;
    s = "select last_insert_rowid()";
    if(!q.exec(s))
        return false;
    if(!q.first())
        return false;
    pid = q.value(0).toInt();
    return true;
}

/**
 * @brief DbUtil::_saveExtrasForPm
 *  保存指定年月的余额到数据库中（原币形式）
 * @param y     年
 * @param m     月
 * @param sums  余额值表
 * @param dirs  余额方向表
 * @param isFst 是一级科目（true：默认值）还是二级科目
 * @return
 */
bool DbUtil::_saveExtrasForPm(int y, int m, const QHash<int, Double> &sums, const QHash<int, MoneyDirection> &dirs, bool isFst)
{
    //操作思路
    //1、先读取保存在表中的余额
    //2、在一二级科目的新余额上进行分别进行迭代操作：
    //（1）如果新值项方向为平，则先跳过（如果老值表中存在对应值项，在迭代结束后会从标志删除）
    //（2）如果新值在老值表中不存在，则执行插入操作（在插入操作前，可能还要执行余额指针的创建）
    //（3）如果新老值方向不同，则更新方向
    //（4）如果新老值不同，则更新值
    //（5）每次迭代的末尾，都从老值表中移除已执行的值项
    //3、迭代完成后，如果老值表不空，则说明遗留的值项都不再存在，可以从相应表中删除
    QSqlQuery q(db);
    QString s;
    QHash<int,int> mtHashs; //键为币种代码，值为余额指针
    if(!_readExtraPoint(y,m,mtHashs))
        return false;
    QHash<int, Double> oldSums;
    QHash<int, MoneyDirection> oldDirs;
    if(!_readExtraForPm(y,m,oldSums,oldDirs,isFst))
        return false;
    if(!db.transaction()){
        LOG_ERROR("Database transaction start failed!");
        return false;
    }
    QHashIterator<int,Double> it(sums);
    int mt,sid;
    QString tname;
    if(isFst)
        tname = tbl_nse_p_f;
    else
        tname = tbl_nse_p_s;
    while(it.hasNext()){
        it.next();
        s.clear();
        mt = it.key() % 10;
        sid = it.key() / 10;
        if(dirs.value(it.key()) == MDIR_P)
            continue;
        //（）、如果新值在老值表中不存在，则执行插入操作
        else if(!oldSums.contains(it.key())){
            if(!mtHashs.contains(mt)){
                int pid;
                if(!_crtExtraPoint(y,m,mt,pid))
                    return false;
                mtHashs[mt] = pid;
            }
            s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,%8,%9)")
                    .arg(tname).arg(fld_nse_pid).arg(fld_nse_sid)
                    .arg(fld_nse_value).arg(fld_nse_dir).arg(mtHashs.value(mt))
                    .arg(sid).arg(it.value().getv()).arg(dirs.value(it.key()));

        }
        //（2）、如果新老值方向不同，则更新方向
        else if(dirs.value(it.key() != oldDirs.value(it.key()))){
            s = QString("update %1 set %2=%3 where %4=%5 and %6=%7").arg(tname)
                    .arg(fld_nse_dir).arg(dirs.value(it.key()))
                    .arg(fld_nse_pid).arg(mtHashs.value(mt)).arg(fld_nse_sid).arg(sid);

        }
        //（3）、如果新老值不同，则更新值
        else if(it.value() != oldSums.value(it.key())){
            s = QString("update %1 set %2=%3 where %4=%5 and %6=%7").arg(tname)
                    .arg(fld_nse_value).arg(it.value().getv())
                    .arg(fld_nse_pid).arg(mtHashs.value(mt)).arg(fld_nse_sid).arg(sid);

        }
        //（5）每次迭代的末尾，都从老值表中移除已执行的值项
        if(!s.isEmpty())
            q.exec(s);
        oldSums.remove(it.key());
        oldDirs.remove(it.key());
    }
    //迭代完成后，如果老值表不空，则说明遗留的值项都不再存在，可以从相应表中删除
    if(!oldSums.isEmpty()){
        QHashIterator<int,Double> ii(oldSums);
        while(ii.hasNext()){
            ii.next();
            mt = ii.key() % 10;
            sid = ii.key() / 10;
            s = QString("delete from %1 where %2=%3 and %4=%5")
                    .arg(tname).arg(fld_nse_pid).arg(mtHashs.value(mt))
                    .arg(fld_nse_sid).arg(sid);
            q.exec(s);
        }
    }

    if(!db.commit()){
        if(!db.rollback())
            LOG_ERROR("Database transaction roll back failed!");
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::_saveExtrasForMm
 *  保存指定年月的余额到数据库中（本币形式）
 * @param y     年
 * @param m     月
 * @param sums  余额值表
 * @param dirs  余额方向表
 * @param isFst 是一级科目（true：默认值）还是二级科目
 * @return
 */
bool DbUtil::_saveExtrasForMm(int y, int m, const QHash<int, Double> &sums, bool isFst)
{
    //假定余额值表中已经剔除了人民币科目的余额值（即只有那些需要外币的科目才需要保存）
    QSqlQuery q(db);
    QString s;
    QHash<int,int> mtHashs; //键为币种代码，值为余额指针
    if(!_readExtraPoint(y,m,mtHashs))
        return false;
    QHash<int, Double> oldSums;
    if(!_readExtraForMm(y,m,oldSums,isFst))
        return false;
    if(!db.transaction()){
        LOG_ERROR("Database transaction start failed!");
        return false;
    }
    QHashIterator<int,Double> it(sums);
    int mt,sid;
    QString tname;
    if(isFst)
        tname = tbl_nse_m_f;
    else
        tname = tbl_nse_m_s;
    while(it.hasNext()){
        it.next();
        s.clear();
        mt = it.key() % 10;
        sid = it.key() / 10;
        if(sums.value(it.key()) == 0)
            continue;
        //（）、如果新值在老值表中不存在，则执行插入操作
        else if(!oldSums.contains(it.key())){
            if(!mtHashs.contains(mt)){
                int pid;
                if(!_crtExtraPoint(y,m,mt,pid))
                    return false;
                mtHashs[mt] = pid;
            }
            s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)")
                    .arg(tname).arg(fld_nse_pid).arg(fld_nse_sid)
                    .arg(fld_nse_value).arg(mtHashs.value(mt))
                    .arg(sid).arg(it.value().getv());

        }
        //（3）、如果新老值不同，则更新值
        else if(it.value() != oldSums.value(it.key())){
            s = QString("update %1 set %2=%3 where %4=%5 and %6=%7").arg(tname)
                    .arg(fld_nse_value).arg(it.value().getv())
                    .arg(fld_nse_pid).arg(mtHashs.value(mt)).arg(fld_nse_sid).arg(sid);

        }
        //（5）每次迭代的末尾，都从老值表中移除已执行的值项
        if(!s.isEmpty())
            q.exec(s);
        oldSums.remove(it.key());
    }
    //迭代完成后，如果老值表不空，则说明遗留的值项都不再存在，可以从相应表中删除
    if(!oldSums.isEmpty()){
        QHashIterator<int,Double> ii(oldSums);
        while(ii.hasNext()){
            ii.next();
            mt = ii.key() % 10;
            sid = ii.key() / 10;
            s = QString("delete from %1 where %2=%3 and %4=%5")
                    .arg(tname).arg(fld_nse_pid).arg(mtHashs.value(mt))
                    .arg(fld_nse_sid).arg(sid);
            q.exec(s);
        }
    }

    if(!db.commit()){
        if(!db.rollback())
            LOG_ERROR("Database transaction roll back failed!");
        return false;
    }
    return true;
}

void DbUtil::crtGdzcTable()
{
    QSqlQuery q1(db);
    QString s;
    bool r;
    ////////////////////////////////////////////////////////////////////
    //创建固定资产类别表
    //s = "CREATE TABLE gdzc_classes(id INTEGER PRIMARY KEY, code INTEGER, name TEXT, zjMonths INTEGER)";
    //r = q1.exec(s);
    //创建固定资产表
    s = "CREATE TABLE gdzcs(id INTEGER PRIMARY KEY,code INTEGER,pcls INTEGER,"
        "scls INTEGER,name TEXT,model TEXT,buyDate TEXT,prime DOUBLE,"
        "remain DOUBLE,min DOUBLE,zjMonths INTEGER,desc TEXT)";
    r = q1.exec(s);
    //创建固定资产折旧信息表
    s = "CREATE TABLE gdzczjs(id INTEGER PRIMARY KEY,gid INTEGER,pid INTEGER,"
        "bid INTEGER,date TEXT,price DOUBLE)";
    r = q1.exec(s);
}
