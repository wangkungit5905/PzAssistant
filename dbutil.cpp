#include <QDebug>
#include <QSqlError>
#include <QVariant>

#include "dbutil.h"
#include "global.h"
#include "tables.h"
#include "logs/Logger.h"
#include "subject.h"
#include "pz.h"
#include "PzSet.h"
#include "utils.h"
//#include "account.h"

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
    return init();
}

bool DbUtil::init()
{
    //QSqlQuery q(db);

    //初始化余额指针索引表 （基于这样一个事实，通常不会读取多个年、月份的余额值，因此既然是缓存表，则有使用需求的才被缓存）
//    QString s = QString("select * from %1").arg(tbl_nse_point);
//    if(!q.exec(s)){
//        LOG_SQLERROR(s);
//        return false;
//    }
//    int id,y,m,mt,key;
//    while(q.next()){
//        id = q.value(0).toInt();
//        y = q.value(NSE_POINT_YEAR).toInt();
//        m = q.value(NSE_POINT_MONTH).toInt();
//        mt = q.value(NSE_POINT_MT).toInt();
//    }
//    key = _genKeyForExtraPoint(y,m,mt);
//    if(key == 0){
//        LOG_ERROR("extra point value have error!");
//        return false;
//    }
//    extraPoints[key] = id;
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
        //case MASTERMT:
            //infos->accInfos.masterMt = infos->moneys.value(q.value(1).toInt());
        //    break;
        //case WAIMT:
            //sl.clear();
            //sl = q.value(1).toString().split(",");
            //foreach(QString v, sl)
            //    infos->accInfos.waiMts<<infos->moneys.value(v.toInt());
            //break;
        //case STIME:
            //infos.startDate = q.value(1).toString();
            //break;
        //case ETIME:
            //infos.endDate = q.value(1).toString();
            //break;
        case LOGFILE:
            infos.logFileName = q.value(1).toString();
            break;
        case DBVERSION:
            infos.dbVersion = q.value(1).toString();
            sl = infos.dbVersion.split(".");
            mv = sl.first().toInt();
            sv = sl.last().toInt();
            break;
        }
    }

    //如果表内的信息字段内容不全，则需要提供默认值，以使对象的属性具有意义
    //默认，日志文件名同账户文件名同名，但扩展名不同
    if(infos.logFileName.isEmpty()){
        infos.logFileName = fileName;
        infos.logFileName = infos.logFileName.replace(".dat",".log");
    }
//    if(infos.endDate.isEmpty()){
//        QDate d = QDate::currentDate();
//        int y = d.year();
//        int m = d.month();
//        int sd = 1;
//        int ed = d.daysInMonth();
//        //infos.startDate = QDate(y,m,sd).toString(Qt::ISODate);
//        //infos.endDate = QDate(y,m,ed).toString(Qt::ISODate);
//    }
//    if(infos.endDate.isEmpty()){
//        QDate d = QDate::fromString(infos.startDate,Qt::ISODate);
//        int days = d.daysInMonth();
//        d.setDate(d.year(),d.month(),days);
//        infos.endDate = d.toString(Qt::ISODate);
//    }

    //读取账户的帐套信息
    if(!readAccountSuites(infos.suites))
        return false;
    //完善帐套的起止月份
//    Account::AccountSuiteRecord* asr;
//    asr = infos.suites.first();
//    asr->startMonth = QDate::fromString(infos.startDate,Qt::ISODate).month();
//    asr->endMonth = 12;
//    asr = infos.suites.last();
//    asr->startMonth = 1;
//    asr->endMonth = QDate::fromString(infos.endDate,Qt::ISODate).month();
//    if(infos.suites.count() > 2){   //中间年份的起止月份都是1-12月
//        for(int i = 1; i < infos.suites.count()-1; ++i){
//            infos.suites.at(i)->startMonth = 1;
//            infos.suites.at(i)->endMonth = 12;
//        }
//    }
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
    //未实现Money类的操作符重载
    //if(infos.masterMt != oldInfos.masterMt && !saveAccInfoPiece(MASTERMT,QString::number(infos.masterMt)))
    //    return false;
    //if(infos.startDate != oldInfos.startDate && !saveAccInfoPiece(STIME,infos.startDate))
    //    return false;
    //if(infos.endDate != oldInfos.endDate && !saveAccInfoPiece(ETIME,infos.endDate))
    //    return false;
    if(infos.lastAccessTime != oldInfos.lastAccessTime && !saveAccInfoPiece(LASTACCESS,infos.lastAccessTime))
        return false;
    if(infos.dbVersion != oldInfos.dbVersion && !saveAccInfoPiece(DBVERSION,infos.dbVersion))
        return false;
    if(infos.logFileName != oldInfos.logFileName && !saveAccInfoPiece(LOGFILE,infos.logFileName))
        return false;
    bool changed = false;
    //保存外币列表
//    if(infos.waiMts.count() != oldInfos.waiMts.count())
//        changed = true;
//    else{
//        for(int i = 0; i < infos.waiMts.count(); ++i){
//            if(infos.waiMts.at(i) != oldInfos.waiMts.at(i)){
//                changed = true;
//                break;
//            }
//        }
//    }
//    if(changed){
//        QString vs;
//        for(int i = 0; i < infos.waiMts.count(); ++i)
//            vs.append(QString("%1,").arg(infos.waiMts.at(i)));
//        vs.chop(1);
//        if(!saveAccInfoPiece(WAIMT,vs))
//            return false;
//        changed = false;
//    }
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
        fsub->addChildSub(ssub);
        if(ssub->getWeight() == DEFALUTSUBFS)
            fsub->setDefaultSubject(ssub);
    }
    //初始化银行账户信息
    //if(!getAllBankAccount(banks))
    //    return false;
    return true;
}

bool DbUtil::saveNameItem(SubjectNameItem *ni)
{
    QSqlQuery q(db);
    QString s;
    if(ni->getId() == UNID)
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values(%8,'%9','%10','%11','%12',%13)")
                .arg(tbl_nameItem).arg(fld_ni_class).arg(fld_ni_name).arg(fld_ni_lname)
                .arg(fld_ni_remcode).arg(fld_ni_crtTime).arg(fld_ni_creator)
                .arg(ni->getClassId()).arg(ni->getShortName()).arg(ni->getLongName())
                .arg(ni->getRemCode()).arg(ni->getCreateTime().toString(Qt::ISODate))
                .arg(ni->getCreator()->getUserId());
    else
        s = QString("update %1 set %2=%3,%4='%5',%6='%7',%8='%9',%10='%11',%12='%13',%14=%15 where id=%16")
                .arg(tbl_nameItem).arg(fld_ni_class).arg(ni->getClassId())
                .arg(fld_ni_name).arg(ni->getShortName()).arg(fld_ni_lname).arg(ni->getLongName())
                .arg(fld_ni_remcode).arg(ni->getRemCode()).arg(fld_ni_crtTime)
                .arg(ni->getCreateTime().toString(Qt::ISODate)).arg(fld_ni_creator)
                .arg(ni->getCreator()->getUserId()).arg(ni->getId());
    return q.exec(s);
}

bool DbUtil::saveSndSubject(SecondSubject *sub)
{
    QSqlQuery q(db);
    QString s;
    if(sub->getId() == UNID)
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8) "
                    "values(%9,%10,'%11',%12,%13,%14,%15)").arg(tbl_ssub)
                .arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_code)
                .arg(fld_ssub_weight).arg(fld_ssub_enable).arg(fld_ssub_crtTime)
                .arg(fld_ssub_creator).arg(sub->getParent()->getId()).arg(sub->getNameItem()->getId())
                .arg(sub->getCode()).arg(sub->getWeight()).arg(sub->isEnabled()?1:0)
                .arg(sub->getCreateTime().toString(Qt::ISODate)).arg(sub->getCreator()->getUserId());
    else
        s = QString("update %1 set %2=%3,%4=%5,%6=%7,%8=%9,%10='%11',%=%,%=%,%=% where id=%")
                .arg(tbl_ssub).arg(fld_ssub_fid).arg(sub->getParent()->getId())
                .arg(fld_ssub_nid).arg(sub->getNameItem()->getId())
                .arg(fld_ssub_weight).arg(sub->getWeight()).arg(fld_ssub_enable).arg(sub->isEnabled()?1:0)
                .arg(fld_ssub_code).arg(sub->getCode())
                .arg(fld_ssub_crtTime).arg(sub->getCreateTime().toString(Qt::ISODate))
                .arg(fld_ssub_disTime).arg(sub->getDiableTime().toString(Qt::ISODate))
                .arg(fld_ssub_creator).arg(sub->getCreator()->getUserId());
    return q.exec(s);
}

bool DbUtil::savefstSubject(FirstSubject *fsub)
{
}

/**
 * @brief DbUtil::initMoneys
 *  初始化当前账户所使用的所有货币对象
 * @param moneys
 * @return
 */
bool DbUtil::initMoneys(Account *account)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_moneyType);
    if(!q.exec(s))
        return false;
    int mmt = 0;
    while(q.next()){
        int code = q.value(MT_CODE).toInt();
        QString sign = q.value(MT_SIGN).toString();
        QString name = q.value(MT_NAME).toString();
        account->moneys[code] = new Money(code,name,sign);
        bool isMain = q.value(MT_MASTER).toBool();
        if(isMain){
            masterMt = code;
            mmt = code;
        }
    }
    //初始化账户信息结构中的母币和外币
    QHashIterator<int,Money*> it(account->moneys);
    while(it.hasNext()){
        it.next();
        if(mmt == it.key())
            account->accInfos.masterMt = it.value();
        else
            account->accInfos.waiMts<<it.value();
    }
    if(mmt == 0)
        QMessageBox::warning(0,QObject::tr("设置错误"),QObject::tr("没有设置本币！"));

    return true;
//    //获取账户所使用的母币
//    s = QString("select %1,%2 from %3 where %4=%5").arg(fld_acci_code)
//            .arg(fld_acci_value).arg(tbl_accInfo).arg(fld_acci_code).arg(MASTERMT);
//    if(!q.exec(s)){
//        LOG_SQLERROR(s);
//        return false;
//    }
//    if(!q.first()){
//        LOG_ERROR("Master money don't set!");
//        return false;
//    }
//    account->accInfos.masterMt = account->moneys.value(q.value(1).toInt());
//    //获取账户所使用的外币
//    s = QString("select %1,%2 from %3 where %4=%5").arg(fld_acci_code)
//            .arg(fld_acci_value).arg(tbl_accInfo).arg(fld_acci_code).arg(WAIMT);
//    if(!q.exec(s)){
//        LOG_SQLERROR(s);
//        return false;
//    }
//    if(!q.first())
//        return true;
//    QStringList sl = q.value(1).toString().split(",");
//    foreach(QString v, sl)
//        account->accInfos.waiMts<<account->moneys.value(v.toInt());
//    return true;
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
 * @brief DbUtil::scanPzSetCount
 *  统计凭证集内各类凭证的总数
 * @param y         年
 * @param m         月
 * @param repeal    作废凭证数
 * @param recording 正在录入的凭证数
 * @param verify    审核通过的凭证数
 * @param instat    已入账的凭证数
 * @param amount    凭证总数
 * @return
 */
bool DbUtil::scanPzSetCount(int y, int m, int &repeal, int &recording, int &verify, int &instat, int &amount)
{
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select %1 from %2 where %3 like '%4%'")
            .arg(fld_pz_state).arg(tbl_pz).arg(fld_pz_date).arg(ds);
    if(!q.exec(s))
        return false;
    repeal=0;recording=0;verify=0;instat=0;amount=0;
    PzState state;
    while(q.next()){
        amount++;
        state = (PzState)q.value(0).toInt();
        switch(state){
        case Pzs_Repeal:
            repeal++;
            break;
        case Pzs_Recording:
            recording++;
            break;
        case Pzs_Verify:
            verify++;
            break;
        case Pzs_Instat:
            instat++;
            break;
        }
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
    if(!isNewExtraAccess())
        BusiUtil::readExtraByMonth2(y,m,fsums,fdirs,ssums,sdirs);
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
    if(isNewExtraAccess()){
        QHash<int,MoneyDirection> fdirs,sdirs;
        //读取以本币计的余额（先从SubjectExtras和detailExtras表读取科目的原币余额值，将其中的外币乘以汇率转换为本币）
        if(!BusiUtil::readExtraByMonth3(y,m,fsums,fdirs,ssums,sdirs))
            return false;
        //从SubjectMmtExtras和detailMmtExtras表读取本币形式的余额，仅包含外币部分）
        bool exist;
        QHash<int,Double> fsumRs,ssumRs;
        if(!BusiUtil::readExtraByMonth4(y,m,fsumRs,ssumRs,exist))
            return false;
        //用精确值代替直接从原币币转换来的外币值
        if(exist){
            QHashIterator<int,Double>* it = new QHashIterator<int,Double>(fsumRs);
            while(it->hasNext()){
                it->next();
                fsums[it->key()] = it->value();
            }
            it = new QHashIterator<int,Double>(ssumRs);
            while(it->hasNext()){
                it->next();
                ssums[it->key()] = it->value();
            }
        }
        return true;
    }

    if(!_readExtraForMm(y,m,fsums))
        return false;
    if(!_readExtraForMm(y,m,ssums,false))
        return false;
    //_replaeAccurateExtra();
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
    if(isNewExtraAccess())
        return BusiUtil::savePeriodBeginValues2(y,m,fsums,fdirs,ssums,sdirs,false);

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
    if(!isNewExtraAccess())
        return BusiUtil::savePeriodEndValues(y,m,fsums,ssums);

    if(!_saveExtrasForMm(y,m,fsums))
        return false;
    if(!_saveExtrasForMm(y,m,ssums,false))
        return false;
    return true;
}



/**
 * @brief DbUtil::getFS_Id_name
 *  获取指定科目系统的一级科目的id和名称（一般用于一级科目选取组合框的初始化）
 * @param ids
 * @param names
 */
bool DbUtil::getFS_Id_name(QList<int> &ids, QList<QString> &names, int subSys)
{
    QSqlQuery q(db);
    QString s = QString("select id,%1 from %2 where %3=%4")
            .arg(fld_fsub_name).arg(tbl_fsub).arg(fld_fsub_subSys).arg(subSys);
    if(q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int id; QString name;
    while(q.next()){
        ids<<q.value(0).toInt();
        names<<q.value(1).toString();
    }
    return true;
}

/**
 * @brief DbUtil::getPzsState
 *  获取指定年月的凭证集状态
 * @param y
 * @param m
 * @param state
 * @return
 */
bool DbUtil::getPzsState(int y, int m, PzsState &state)
{
    if(!isNewExtraAccess())
        return BusiUtil::getPzsState(y,m,state);
    QSqlQuery q(db);
    if(y==0 && m==0){
        state = Ps_NoOpen;
        return true;
    }
    QString s = QString("select %1 from %2 where (%3=%4) and (%5=%6)")
            .arg(fld_pzss_state).arg(tbl_pzsStates).arg(fld_pzss_year)
            .arg(y).arg(fld_pzss_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first())  //还没有记录，则表示刚开始录入凭证
        state = Ps_Rec;
    else
        state = (PzsState)q.value(0).toInt();
    return true;
}

/**
 * @brief DbUtil::setPzsState
 *  保存指定年月的凭证集状态
 * @param y
 * @param m
 * @param state
 * @return
 */
bool DbUtil::setPzsState(int y, int m, PzsState state)
{
    if(!isNewExtraAccess())
        return BusiUtil::setPzsState(y,m,state);

    QSqlQuery q(db);
    //首先检测是否存在对应记录
    QString s = QString("select %1 from %2 where (%3=%4) and (%5=%6)")
            .arg(fld_pzss_state).arg(tbl_pzsStates).arg(fld_pzss_year)
            .arg(y).arg(fld_pzss_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first())
        s = QString("update %1 set %2=%3 where (%4=%5) and (%6=%7)")
                .arg(tbl_pzsStates).arg(fld_pzss_state).arg(state)
                .arg(fld_pzss_year).arg(y).arg(fld_pzss_month).arg(m);
    else
        s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)").arg(tbl_pzsStates)
                .arg(fld_pzss_year).arg(fld_pzss_month).arg(fld_pzss_state)
                .arg(y).arg(m).arg(state);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::setExtraState
 *  设置余额是否有效
 * @param y
 * @param m
 * @param isVolid
 * @return
 */
bool DbUtil::setExtraState(int y, int m, bool isVolid)
{
    if(!isNewExtraAccess())
        return BusiUtil::setExtraState(y,m,isVolid);

    //余额的有效性状态只记录在“余额指针表”中保存本币余额的那条记录里
    QSqlQuery q(db);
    QString s = QString("update %1 set %2=%3 where %4=%5 and %6=%7 and %8=%9")
            .arg(tbl_nse_point).arg(fld_nse_state).arg(isVolid?1:0).arg(fld_nse_year)
            .arg(y).arg(fld_nse_month).arg(m).arg(fld_nse_mt).arg(masterMt);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.numRowsAffected() == 1)
        return true;
    s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,%8,%9)")
            .arg(tbl_nse_point).arg(fld_nse_year).arg(fld_nse_month)
            .arg(fld_nse_mt).arg(fld_nse_state).arg(y).arg(m).arg(masterMt)
            .arg(isVolid?1:0);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::getExtraState
 *  获取余额是否有效
 * @param y
 * @param m
 * @return
 */
bool DbUtil::getExtraState(int y, int m)
{
    if(!isNewExtraAccess())
        return BusiUtil::getExtraState(y,m);

    QSqlQuery q(db);
    QString s = QString("select %1 from %2 where %3=%4 and %5=%6 and %7=%8")
            .arg(fld_nse_state).arg(tbl_nse_point).arg(fld_nse_year)
            .arg(y).arg(fld_nse_month).arg(m).arg(fld_nse_mt).arg(masterMt);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first())
        return false;
    return q.value(0).toBool();
}

/**
 * @brief DbUtil::getRates
 *  获取指定年月凭证集所采用的汇率
 * @param y
 * @param m
 * @param rates     汇率表（键为币种代码）
 * @param mainMt    本币代码
 * @return
 */
bool DbUtil::getRates(int y, int m, QHash<int, Double> &rates)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_moneyType);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QString mainSign; //母币符号
    QHash<int,QString> msHash; //货币代码到货币符号的映射
    int mt; QString mtSign;bool isMast;
    while(q.next()){
        isMast = q.value(MT_MASTER).toBool();
        mt = q.value(MT_CODE).toInt();
        mtSign = q.value(MT_SIGN).toString();
        if(!isMast)
            msHash[mt] = mtSign;
        else
            mainSign = mtSign;
    }

    QList<int> mtcs = msHash.keys();
    s = QString("select ");
    for(int i = 0; i<mtcs.count(); ++i){
        s.append(msHash.value(mtcs.at(i)));
        s.append(QString("2%1,").arg(mainSign));
    }
    s.chop(1);
    s.append(QString(" from %1 ").arg(tbl_rateTable));
    s.append(QString("where %1 = %2 and %3 = %4")
             .arg(fld_rt_year).arg(y).arg(fld_rt_month).arg(m));
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        LOG_DEBUG(QObject::tr("没有指定汇率！"));
        return false;
    }
    for(int i = 0;i<mtcs.count();++i)
        rates[mtcs.at(i)] = Double(q.value(i).toDouble());
    return true;
}

/**
 * @brief DbUtil::saveRates
 *  保存指定年月凭证集所采用的汇率
 * @param y
 * @param m
 * @param rates     汇率表（键为币种代码）
 * @param mainMt    本币代码
 * @return
 */
bool DbUtil::saveRates(int y, int m, QHash<int, Double> &rates)
{
    QSqlQuery q(db);
    QString s,vs;

    QList<int> wbCodes;     //外币币种代码
    QList<QString> wbSigns; //外币符号列表
    QList<QString> mtFields; //存放与外币币种对应汇率的字段名（按序号一一对应）
    s = QString("select %1,%2,%3 from %4").arg(fld_mt_code).arg(fld_mt_sign)
            .arg(fld_mt_isMaster).arg(tbl_moneyType);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QString mainSign,mtSign;
    int mt; bool isMast;
    while(q.next()){
        mt = q.value(0).toInt();
        mtSign = q.value(1).toString();
        isMast = q.value(2).toBool();
        if(isMast)
            mainSign = mtSign;
        else{
            wbCodes << mt;
            wbSigns << mtSign;
        }
    }
    for(int i = 0; i < wbCodes.count(); ++i)
        mtFields << wbSigns.at(i) + "2" + mainSign;
    s = QString("select id from %1 where %2 = %3 and %4 = %5")
            .arg(tbl_rateTable).arg(fld_rt_year).arg(y).arg(fld_rt_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first()){
        int id = q.value(0).toInt();
        s = QString("update %1 set ").arg(tbl_rateTable);
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(wbCodes.at(i)))
                s.append(QString("%1=%2,").arg(mtFields.at(i))
                         .arg(rates.value(wbCodes.at(i)).toString()));
        }

        s.chop(1);
        s.append(QString(" where id = %1").arg(id));
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    else{
        s = QString("insert into %1(%2,%3,").arg(tbl_rateTable).arg(fld_rt_year).arg(fld_rt_month);
        vs = QString("values(%1,%2,").arg(y).arg(m);
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(wbCodes.at(i))){
                s.append(mtFields.at(i)).append(",");
                vs.append(rates.value(wbCodes[i]).toString()).append(",");
            }
        }
        s.chop(1); vs.chop(1);
        s.append(") "); vs.append(")");
        s.append(vs);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    return true;
}

/**
 * @brief DbUtil::loadPzSet
 *  装载指定年月的凭证
 * @param y
 * @param m
 * @param pzs
 * @return
 */
bool DbUtil::loadPzSet(int y, int m, QList<PingZheng *> &pzs, PzSetMgr* parent)
{
    QSqlQuery q(db),q1(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select * from %1 where %2 like '%3%'")
            .arg(tbl_pz).arg(fld_pz_date).arg(ds);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }

    Account* account = parent->getAccount();
    int subSys = account->getSuite(y)->subSys;
    SubjectManager* smg = account->getSubjectManager(subSys);
    QHash<int,Money*> mts = account->getAllMoneys();
    PingZheng* pz;
    QList<BusiAction*> bs;
    BusiAction* ba;
    int id,pid,pnum,znum,encnum,num;
    MoneyDirection dir;
    QString d,summary;
    PzClass pzCls;
    PzState pzState;
    User *vu,*ru,*bu;
    Double jsum,dsum;
    FirstSubject* fsub;
    SecondSubject* ssub;
    Double v;
    Money* mt;
    while(q.next()){
        id = q.value(0).toInt();
        d = q.value(PZ_DATE).toString();
        pnum = q.value(PZ_NUMBER).toInt();
        znum = q.value(PZ_ZBNUM).toInt();
        jsum = q.value(PZ_JSUM).toDouble();
        dsum = q.value(PZ_DSUM).toDouble();
        pzCls = (PzClass)q.value(PZ_CLS).toInt();
        encnum = q.value(PZ_ENCNUM).toInt();
        pzState = (PzState)q.value(PZ_PZSTATE).toInt();
        vu = allUsers.value(q.value(PZ_VUSER).toInt());
        ru = allUsers.value(q.value(PZ_RUSER).toInt());
        bu = allUsers.value(q.value(PZ_BUSER).toInt());

        pz = new PingZheng(id,d,pnum,znum,jsum,dsum,pzCls,encnum,pzState,
                           vu,ru,bu,parent);

        QString as = QString("select * from %1 where %2=%3 order by %4")
                .arg(tbl_ba).arg(fld_ba_pid).arg(pz->id()).arg(fld_ba_number);
        if(!q1.exec(as)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q1.next()){
            id = q1.value(0).toInt();
            pid = q1.value(BACTION_PID).toInt();
            summary = q1.value(BACTION_SUMMARY).toString();
            fsub = smg->getFstSubject(q1.value(BACTION_FID).toInt());
            ssub = smg->getSndSubject(q1.value(BACTION_SID).toInt());
            mt = mts.value(q1.value(BACTION_MTYPE).toInt());
            dir = (MoneyDirection)q1.value(BACTION_DIR).toInt();
            if(dir == DIR_J)
                v = q1.value(BACTION_JMONEY).toDouble();
            else
                v = q1.value(BACTION_DMONEY).toDouble();
            num = q1.value(BACTION_NUMINPZ).toInt();
            //ba->state = BusiActionData::INIT;
            ba = new BusiAction(id,pz,summary,fsub,ssub,mt,dir,v,num);
            pz->baLst<<ba;
        }
        pzs<<pz;
    }
    return true;
}

/**
 * @brief DbUtil::isContainPz
 *  在指定年月的凭证集内是否包含了指定id的凭证
 * @param y
 * @param m
 * @param pid   凭证id
 * @return      true：包含，false：不包含
 */
bool DbUtil::isContainPz(int y, int m, int pid)
{
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select id from %1 where id=%2 and %3 like '%4%'")
            .arg(tbl_pz).arg(pid).arg(fld_pz_date).arg(ds);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return q.first();
}

/**
 * @brief DbUtil::inspectJzPzExist
 *  检查指定类别的凭证是否存在、是否齐全
 * @param y
 * @param m
 * @param pzCls 凭证大类别
 * @param count 指定的凭证大类必须具有的凭证数（返回值大于0，则表示凭证不齐全）
 * @return
 */
bool DbUtil::inspectJzPzExist(int y, int m, PzdClass pzCls, int &count)
{
    QSqlQuery q(db);
    QList<PzClass> pzClses;
    if(pzCls == Pzd_Jzhd)
        pzClses<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf;
    else if(pzCls == Pzd_Jzsy)
        pzClses<<Pzc_JzsyIn<<Pzc_JzsyFei;
    else if(pzCls == Pzd_Jzlr)
        pzClses<<Pzc_Jzlr;
    count = pzClses.count();
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select %1 from %2 where %3 like '%4%'")
            .arg(fld_pz_class).arg(tbl_pz).arg(fld_pz_date).arg(ds)/*.arg(fld_pz_number)*/;
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    PzClass cls;
    while(q.next()){
        cls = (PzClass)q.value(0).toInt();
        if(pzClses.contains(cls)){
            pzClses.removeOne(cls);
            count--;
        }
        if(count==0)
            break;
    }
    return true;
}


/**
 * @brief DbUtil::assignPzNum
 *  按凭证日期和录入顺序重置凭证号
 * @param y
 * @param m
 * @return
 */
bool DbUtil::assignPzNum(int y, int m)
{
    QSqlQuery q(db),q1(db);
    QString s;

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("重置凭证"));
        return false;
    }
    s = QString("select id from %1 where %2 like '%3%' order by %2")
            .arg(tbl_pz).arg(fld_pz_date).arg(ds);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int id, num = 1;
    while(q.next()){
        id = q.value(0).toInt();
        s = QString("update %1 set %2=%3 where id=%4")
                .arg(tbl_pz).arg(fld_pz_number).arg(num++).arg(id);
        if(!q1.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("重置凭证"));
        if(!db.rollback()){
            warn_transaction(Transaction_rollback,QObject::tr("重置凭证"));
            return false;
        }
        return false;
    }
    return true;
}

bool DbUtil::crtNewPz(PzData *pz)
{
    QSqlQuery q(db);
    QString s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12) "
                        "values('%13',%14,%15,%16,%17,%18,%19,%20,%21,%22,%23)").arg(tbl_pz)
            .arg(fld_pz_date).arg(fld_pz_number).arg(fld_pz_zbnum).arg(fld_pz_jsum)
            .arg(fld_pz_dsum).arg(fld_pz_class).arg(fld_pz_encnum).arg(fld_pz_state)
            .arg(fld_pz_vu).arg(fld_pz_ru).arg(fld_pz_bu)
            .arg(pz->date).arg(pz->pzNum).arg(pz->pzZbNum).arg(pz->jsum)
            .arg(pz->dsum).arg(pz->pzClass).arg(pz->attNums).arg(pz->state)
            .arg(pz->verify!=NULL?pz->verify->getUserId():NULL)
            .arg(pz->producer!=NULL?pz->producer->getUserId():NULL)
            .arg(pz->bookKeeper!=NULL?pz->bookKeeper->getUserId():NULL);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }

    s = QString("select last_insert_rowid()");
    if(q.exec(s) && q.first())
        pz->pzId = q.value(0).toInt();
    else
        return false;
    return true;
}

bool DbUtil::delActionsInPz(int pzId)
{
    QSqlQuery q(db);
    QString s = QString("delete from %1 where %2 = %3").arg(tbl_ba).arg(fld_ba_pid).arg(pzId);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    //int rows = q.numRowsAffected();
    return true;
}

/**
 * @brief DbUtil::getActionsInPz
 *  获取指定凭证内的所有会计分录（将来当引入凭证和会计分录类时，要修改）
 * @param pid
 * @param busiActions
 * @return
 */
bool DbUtil::getActionsInPz(int pid, QList<BusiActionData2 *> &busiActions)
{
    QSqlQuery q(db);

    if(busiActions.count() > 0){
        qDeleteAll(busiActions);
        busiActions.clear();
    }

    QString s = QString("select * from %1 where %2 = %3 order by %4")
            .arg(tbl_ba).arg(fld_ba_pid).arg(pid).arg(fld_ba_number);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        BusiActionData2* ba = new BusiActionData2;
        ba->id = q.value(0).toInt();
        ba->pid = pid;
        ba->summary = q.value(BACTION_SUMMARY).toString();
        ba->fid = q.value(BACTION_FID).toInt();
        ba->sid = q.value(BACTION_SID).toInt();
        ba->mt  = q.value(BACTION_MTYPE).toInt();
        ba->dir = q.value(BACTION_DIR).toInt();
        if(ba->dir == DIR_J)
            ba->v = Double(q.value(BACTION_JMONEY).toDouble());
        else
            ba->v = Double(q.value(BACTION_DMONEY).toDouble());
        ba->num = q.value(BACTION_NUMINPZ).toInt();
        ba->state = BusiActionData2::INIT;
        busiActions.append(ba);
    }
    return true;
}

/**
 * @brief DbUtil::saveActionsInPz
 *  保存指定凭证的所有会计分录（当引入凭证类后，可以使用一个凭证对象参数在一个事务中保存所有凭证相关的内容）
 * @param pid
 * @param busiActions
 * @param dels
 * @return
 */
bool DbUtil::saveActionsInPz(int pid, QList<BusiActionData2 *> &busiActions, QList<BusiActionData2 *> dels)
{
    QString s;
    QSqlQuery q1(db),q2(db),q3(db),q4(db);
    bool hasNew = false;

    if(!busiActions.isEmpty()){
        if(!db.transaction()){
            warn_transaction(Transaction_open,QObject::tr("保存凭证内的会计分录"));
            return false;
        }

        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) "
                    "values(:pid,:summary,:fid,:sid,:mt,:jv,:dv,:dir,:num)")
                .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary).arg(fld_ba_fid)
                .arg(fld_ba_sid).arg(fld_ba_mt).arg(fld_ba_jv).arg(fld_ba_dv)
                .arg(fld_ba_dir).arg(fld_ba_number);
        q1.prepare(s);
        s = QString("update %1 set %2=:summary,%3=:fid,%4=:sid,%5=:mt,"
                    "%6=:jv,%7=:dv,%8=:dir,%9=:num where id=:id")
                .arg(tbl_ba).arg(fld_ba_summary).arg(fld_ba_fid).arg(fld_ba_sid)
                .arg(fld_ba_mt).arg(fld_ba_jv).arg(fld_ba_dv).arg(fld_ba_dir)
                .arg(fld_ba_number);
        q2.prepare(s);
        s = QString("update %1 set %2=:num where id=:id").arg(tbl_ba).arg(fld_ba_number);
        q3.prepare(s);
        for(int i = 0; i < busiActions.count(); ++i){
            busiActions[i]->num = i + 1;  //在保存的同时，重新赋于顺序号
            switch(busiActions[i]->state){
            case BusiActionData2::INIT:
                break;
            case BusiActionData2::NEW:
                hasNew = true;
                q1.bindValue(":pid",busiActions[i]->pid);
                q1.bindValue(":summary", busiActions[i]->summary);
                q1.bindValue(":fid", busiActions[i]->fid);
                q1.bindValue(":sid", busiActions[i]->sid);
                q1.bindValue(":mt", busiActions[i]->mt);
                if(busiActions[i]->dir == DIR_J){
                    q1.bindValue(":jv", busiActions[i]->v.getv());
                    q1.bindValue(":dv",0);
                    q1.bindValue(":dir", DIR_J);
                }
                else{
                    q1.bindValue(":jv",0);
                    q1.bindValue(":dv", busiActions[i]->v.getv());
                    q1.bindValue(":dir", DIR_D);
                }
                q1.bindValue(":num", busiActions[i]->num);
                q1.exec();
                q4.exec("select last_insert_rowid()");
                q4.first();
                busiActions.at(i)->id = q4.value(0).toInt();
                break;
            case BusiActionData2::EDITED:
                q2.bindValue(":summary", busiActions[i]->summary);
                q2.bindValue(":fid", busiActions[i]->fid);
                q2.bindValue(":sid", busiActions[i]->sid);
                q2.bindValue(":mt", busiActions[i]->mt);
                if(busiActions[i]->dir == DIR_J){
                    q2.bindValue(":jv", busiActions[i]->v.getv());
                    q2.bindValue(":dv",0);
                    q2.bindValue(":dir", DIR_J);
                }
                else{
                    q2.bindValue(":jv",0);
                    q2.bindValue(":dv", busiActions[i]->v.getv());
                    q2.bindValue(":dir", DIR_D);
                }
                q2.bindValue(":num", busiActions[i]->num);
                q2.bindValue("id", busiActions[i]->id);
                q2.exec();
                break;
            case BusiActionData2::NUMCHANGED:
                q3.bindValue(":num", busiActions[i]->num);
                q3.bindValue(":id", busiActions[i]->id);
                q3.exec();
                break;
            }
            busiActions[i]->state = BusiActionData2::INIT;
        }
        if(!db.commit()){
            warn_transaction(Transaction_commit,QObject::tr("保存凭证内的会计分录"));
            return false;
        }
        //回读新增的业务活动的id(待需要时再使用此代码)
        //if(hasNew && !getActionsInPz(pid,busiActions))
        //    return false;
    }
    if(!dels.isEmpty()){
        if(!db.transaction()){
            warn_transaction(Transaction_open,QObject::tr("保存凭证内的会计分录"));
            return false;
        }
        s = "delete from BusiActions where id=:id";
        q4.prepare(s);
        for(int i = 0; i < dels.count(); ++i){
            q4.bindValue(":id", dels[i]->id);
            q4.exec();
        }
        if(!db.commit()){
            warn_transaction(Transaction_commit,QObject::tr("保存凭证内的会计分录"));
            return false;
        }
    }
    return true;
}
/**
 * @brief DbUtil:delSpecPz
 *  删除指定年月的指定大类别凭证
 * @param y
 * @param m
 * @param pzCls         凭证大类
 * @param pzCntAffected 删除的凭证数
 * @return
 */
bool DbUtil::delSpecPz(int y, int m, PzdClass pzCls, int &affected)
{
    QString s;
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QList<PzClass> codes = getSpecClsPzCode(pzCls);
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("删除大类别凭证"));
        return false;
    }
    QList<int> ids;
    for(int i = 0; i < codes.count(); ++i){
        s = QString("select id from %1 where (%2 like '%3%') and (%4=%5)")
                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class).arg(codes.at(i));
        q.exec(s);q.first();
        ids<<q.value(0).toInt();
    }
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("删除大类别凭证"));
        return false;
    }
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("删除大类别凭证"));
        return false;
    }
    affected = 0;
    for(int i = 0; i < ids.count(); ++i){
        s = QString("delete from %1 where %2=%3").arg(tbl_ba).arg(fld_ba_pid).arg(ids.at(i));
        q.exec(s);
        s = QString("delete from %1 where id=%2").arg(tbl_pz).arg(ids.at(i));
        q.exec(s);
        affected += q.numRowsAffected();
    }
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("删除大类别凭证"));
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::getSpecClsPzCode
 *  获取指定大类凭证的类别代码列表
 * @param cls
 * @return
 */
QList<PzClass> DbUtil::getSpecClsPzCode(PzdClass cls)
{
    QList<PzClass> codes;
    if(cls == Pzd_Jzhd)
        codes<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf;
    else if(cls == Pzd_Jzsy)
        codes<<Pzc_JzsyIn<<Pzc_JzsyFei;
    else if(cls == Pzd_Jzlr)
        codes<<Pzc_Jzlr;
    return codes;
}

/**
 * @brief DbUtil::haveSpecClsPz
 *  检测凭证集内各类特殊凭证是否存在
 * @param y
 * @param m
 * @param isExist
 * @return
 */
bool DbUtil::haveSpecClsPz(int y, int m, QHash<PzdClass, bool> &isExist)
{
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select id from %1 where (%2 like '%3%') and (%4=%5 or %4=%6 or %4=%7)")
            .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class).arg(Pzc_Jzhd_Bank)
            .arg(Pzc_Jzhd_Ys).arg(Pzc_Jzhd_Yf);
    if(!q.exec(s))
        return false;
    isExist[Pzd_Jzhd] = q.first();

    s = QString("select id from %1 where (%2 like '%3%') and (%4=%5 or %4=%6)")
                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class)
                .arg(Pzc_JzsyIn).arg(Pzc_JzsyFei);
    if(!q.exec(s))
        return false;
    isExist[Pzd_Jzsy] = q.first();
    s = QString("select id from %1 where (%2 like '%3%') and (%4=%5)")
                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class)
                .arg(Pzc_Jzlr);
    if(!q.exec(s))
        return false;
    isExist[Pzd_Jzlr] = q.first();
    return true;
}

/**
 * @brief DbUtil::specPzClsInstat
 *  将指定类别凭证全部入账
 * @param y
 * @param m
 * @param cls
 * @param affected  受影响的凭证条目
 * @return
 */
bool DbUtil::specPzClsInstat(int y, int m, PzdClass cls, int &affected)
{
    QSqlQuery q(db);
    QString s;
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QList<PzClass> codes = getSpecClsPzCode(cls);
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("将指定类别凭证全部入账"));
        return false;
    }
    affected = 0;
    for(int i = 0; i < codes.count(); ++i){
        s = QString("update %1 set %2=%3 where (%4 like '%5%') and %6=%7")
                .arg(tbl_pz).arg(fld_pz_state).arg(Pzs_Instat)
                .arg(fld_pz_date).arg(ds).arg(fld_pz_class).arg(codes.at(i));
        q.exec(s);
        affected += q.numRowsAffected();
    }
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("将指定类别凭证全部入账"));
        return false;
    }
}

/**
 * @brief DbUtil::setAllPzState
 *  设置凭证集内的所有具有指定状态的凭证到目的状态
 * @param y
 * @param m
 * @param state         目的状态
 * @param includeState  指定的凭证状态
 * @param affected      受影响的行数
 * @param user
 * @return
 */
bool DbUtil::setAllPzState(int y, int m, PzState state, PzState includeState, int &affected, User *user)
{
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString userField;
    if(state == Pzs_Recording)
        userField = fld_pz_ru;
    else if(state == Pzs_Verify)
        userField = fld_pz_vu;
    else if(state == Pzs_Instat)
        userField = fld_pz_bu;
    QString s = QString("update %1 set %2=%3,%4=%5 where (%6 like '%7%') and %2=%8")
            .arg(tbl_pz).arg(fld_pz_state).arg(state).arg(userField)
            .arg(user->getUserId()).arg(fld_pz_date).arg(ds).arg(includeState);
    affected = 0;
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    affected = q.numRowsAffected();
    return true;
}

/**
 * @brief DbUtil::getSubWinInfo
 *  读取子窗口的各种几何尺寸信息
 * @param winEnum
 * @param info
 * @param otherInfo
 * @return
 */
bool DbUtil::getSubWinInfo(int winEnum, SubWindowDim *&info, QByteArray *&otherInfo)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2 = %3")
            .arg(tbl_subWinInfo).arg(fld_swi_enum).arg(winEnum);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first()){
        info = new SubWindowDim;
        info->x = q.value(SWI_X).toInt();
        info->y = q.value(SWI_Y).toInt();
        info->w = q.value(SWI_W).toInt();
        info->h = q.value(SWI_H).toInt();
        otherInfo = new QByteArray(q.value(SWI_TBL).toByteArray());
    }
    else{
        info = NULL;
        otherInfo = NULL;
    }
    return true;
}

/**
 * @brief DbUtil::saveSubWinInfo
 *  保存子窗口的各种几何尺寸信息
 * @param winEnum
 * @param info
 * @param otherInfo
 * @return
 */
bool DbUtil::saveSubWinInfo(int winEnum, SubWindowDim *info, QByteArray *otherInfo)
{
    QSqlQuery q(db);
    QString s;

    if(otherInfo == NULL)
        otherInfo = new QByteArray;
    s = QString("select * from %1 where %2 = %3")
            .arg(tbl_subWinInfo).arg(fld_swi_enum).arg(winEnum);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first()){
        int id = q.value(0).toInt();
        s = QString("update %1 set %2=:enum,%3=:x,%4=:y,%5=:w,%6=:h"
                    ",%7=:info where id=:id")
                .arg(tbl_subWinInfo).arg(fld_swi_enum).arg(fld_swi_x).arg(fld_swi_y)
                .arg(fld_swi_width).arg(fld_swi_height).arg(fld_swi_tblInfo);
        if(!q.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.bindValue(":enum",winEnum);
        q.bindValue(":x",info->x);
        q.bindValue(":y",info->y);
        q.bindValue(":w",info->w);
        q.bindValue(":h",info->h);
        q.bindValue(":info",*otherInfo);
        q.bindValue(":id",id);
    }
    else{
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7) "
                    "values(:enum,:x,:y,:w,:h,:info)")
                .arg(tbl_subWinInfo).arg(fld_swi_enum).arg(fld_swi_x).arg(fld_swi_y)
                .arg(fld_swi_width).arg(fld_swi_height).arg(fld_swi_tblInfo);;
        if(!q.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.bindValue(":enum",winEnum);
        q.bindValue(":x",info->x);
        q.bindValue(":y",info->y);
        q.bindValue(":w",info->w);
        q.bindValue(":h",info->h);
        q.bindValue(":info",*otherInfo);
    }
    if(!q.exec()){
        LOG_SQLERROR(s);
        return false;
    }
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
bool DbUtil::readAccountSuites(QList<Account::AccountSuiteRecord *> &suites)
{
    //读取账户的帐套信息
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2").arg(tbl_accSuites).arg(fld_accs_year);
    if(!q.exec(s))
        return false;
    Account::AccountSuiteRecord* as;
    while(q.next()){
        as = new Account::AccountSuiteRecord;
        as->id = q.value(0).toInt();
        as->year = q.value(ACCS_YEAR).toInt();
        as->subSys = q.value(ACCS_SUBSYS).toInt();
        as->recentMonth = q.value(ACCS_RECENTMONTH).toInt();
        as->isCur = q.value(ACCS_ISCUR).toBool();
        as->name = q.value(ACCS_NAME).toString();
        as->startMonth = q.value(ACCS_STARTMONTH).toInt();
        as->endMonth = q.value(ACCS_ENDMONTH).toInt();
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
bool DbUtil::saveAccountSuites(QList<Account::AccountSuiteRecord *> &suites)
{
    QString s;
    QSqlQuery q(db);
    if(!db.transaction()){
        Logger::write(QDateTime::currentDateTime(),Logger::Error,"",0,"", QObject::tr("Start transaction failed!"));
        return false;
    }
    QList<Account::AccountSuiteRecord *> oldSuites;
    if(!readAccountSuites(oldSuites))
        return false;
    QHash<int,Account::AccountSuiteRecord*> oldHash;
    foreach(Account::AccountSuiteRecord* as, oldSuites)
        oldHash[as->id] = as;
    foreach(Account::AccountSuiteRecord* as, suites){
        if(as->id == 0)
            s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,%8,'%9',%10,%11)")
                    .arg(tbl_accSuites).arg(fld_accs_year).arg(fld_accs_subSys)
                    .arg(fld_accs_name).arg(fld_accs_recentMonth).arg(fld_accs_isCur)
                    .arg(as->year).arg(as->subSys).arg(as->name).arg(as->recentMonth)
                    .arg(as->isCur?1:0);
        else if(*as != *(oldHash.value(as->id)))
            s = QString("update %1 set %2=%3,%4=%5,%6='%7',%8=%9,%10=%11 where id=%12")
                    .arg(tbl_accSuites).arg(fld_accs_year).arg(as->year)
                    .arg(fld_accs_subSys).arg(as->subSys).arg(fld_accs_name)
                    .arg(as->name).arg(fld_accs_recentMonth).arg(as->recentMonth)
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
    QString s;

    //首先查找余额指针缓存表内是否存在，如果存在，则直接从缓存表中读取，否则从数据库的指针表中读取
    //因为传入的参数“mtHashs”是一个空表，不能对他迭代，但又不知道数据库中到底保存了哪些币种
    //对应的余额，所有由于还缺少一个币种的获取机制，且暂缓实施这一缓存机制
//    QHashIterator<int,int> it(mtHashs);
//    int key,id;
//    while(it.hasNext()){
//        it.next();
//        key = _genKeyForExtraPoint(y,m,it.key());
//        if(key == 0)
//            return false;
//        if(!extraPoints.contains(key)){
//            s = QString("select id from %1 where %2=%3 and %4=%5 and %6=%7")
//                    .arg(tbl_nse_point).arg(fld_nse_year).arg(y)
//                    .arg(fld_nse_month).arg(m).arg(fld_nse_mt).arg(it.key());
//            if(!q.exec(s))
//                return false;
//            if(!q.first()){
//                continue;
//            }
//            id = q.value(0).toInt();
//            mtHashs[it.key()] = id;
//            extraPoints[key] = id;
//        }
//        else
//            mtHashs[it.key()] = extraPoints.value(key);
//    }

    //1、首先取得保存指定年月余额值的指针id
    s = QString("select id,%1 from %2 where %3=%4 and %5=%6")
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
    mtHash.remove(curAccount->getMasterMt()->code());
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
 * @brief DbUtil::_replaeAccurateExtra
 *  用精确值替换直接从原币转换为本币的值
 *  此函数用于那些需要按币种进行分别核算的科目，这些科目的外币余额往往有原币和本币形式，
 *  但它们之间不是精确地符合当前的汇率比率（即不能简单地用原币余额值乘以汇率得到本币余额值）
 *  而是要用从本币余额表中读取值来替换转换而来的值
 * @param sums      本币余额值表
 * @param asums     替换后的余额值表
 */
void DbUtil::_replaeAccurateExtra(QHash<int, Double> &sums, QHash<int, Double> &asums)
{

    QHashIterator<int,Double> it(asums);
    while(it.hasNext()){
        it.next();
        sums[it.key()] = it.value();
    }
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

/**
 * @brief DbUtil::_genKeyForExtraPoint
 *  生成由年份、月份和币种构成的复合键，用来索引余额指针表
 * @param y
 * @param m
 * @param mt
 * @return
 */
int DbUtil::_genKeyForExtraPoint(int y, int m, int mt)
{
    if(m < 1 || m > 12 || mt < 1 || mt > 9)
        return 0;
    if(m < 10)
        return y*1000+m*100+mt;
    else if(m > 9 && m <= 12)
        return y*1000+m*10+mt;
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

void DbUtil::warn_transaction(ErrorCode witch, QString context)
{
    QString s;
    switch(witch){
    case Transaction_open:
        s = QObject::tr("启动事务");
        break;
    case Transaction_commit:
        s = QObject::tr("提交事务");
        break;
    case Transaction_rollback:
        s = QObject::tr("回滚事务");
        break;
    }
    QString ss = QObject::tr("在%1时，%2失败！").arg(context).arg(s);
    //LOG_SQLERROR(ss);
    QMessageBox::critical(0,QObject::tr("数据库访问出错"),ss);
}
