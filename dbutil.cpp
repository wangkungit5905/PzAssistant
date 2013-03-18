#include <QDebug>
#include <QSqlError>

#include "dbutil.h"
#include "global.h"
#include "tables.h"
#include "account.h"

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
        as->lastMonth = q.value(ACCS_LASTMONTH).toInt();
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
                    .arg(fld_accs_name).arg(fld_accs_lastMonth).arg(fld_accs_isCur)
                    .arg(as->year).arg(as->subSys).arg(as->name).arg(as->lastMonth)
                    .arg(as->isCur?1:0);
        else if(*as != *(oldHash.value(as->id)))
            s = QString("update %1 set %2=%3,%4=%5,%6='%7',%8=%9,%10=%11 where id=%12")
                    .arg(tbl_accSuites).arg(fld_accs_year).arg(as->year)
                    .arg(fld_accs_subSys).arg(as->subSys).arg(fld_accs_name)
                    .arg(as->name).arg(fld_accs_lastMonth).arg(as->lastMonth)
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
