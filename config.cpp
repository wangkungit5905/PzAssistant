#include <QSettings>
#include <QStringList>
#include <QTextCodec>
#include <QInputDialog>
#include <QFileInfo>
#include <QDir>

#include "global.h"
#include "config.h"
#include "tables.h"
#include "version.h"

QSettings* AppConfig::appIni;
AppConfig* AppConfig::instance = 0;
QSqlDatabase AppConfig::db;

//////////////////////////////////AppConfig//////////////////////////////////////
AppConfig::AppConfig()
{
    appIni->setIniCodec(QTextCodec::codecForTr());
}

AppConfig::~AppConfig()
{
    exit();
}



/**
 * @brief AppConfig::getInstance
 *  获取应用配置对象的实例
 * @return
 */
AppConfig *AppConfig::getInstance()
{
    if(instance)
        return instance;
    appIni = new QSettings("./config/app/appSetting.ini", QSettings::IniFormat);

    db = QSqlDatabase::addDatabase("QSQLITE", "basic");
    QString fname = "./datas/basicdatas/basicdata.dat";
    db.setDatabaseName(fname);
    if (!db.open()){
        QMessageBox::critical(0, QObject::tr("不能打开基础数据库"),
            QObject::tr("不能够建立与基础数据库之间的连接\n"
                     "请检查data目录下是否存在basicdata.dat文件"), QMessageBox::Cancel);
        return NULL;
    }    
    instance = new AppConfig;
    return instance;
}

//读取应用程序的日志级别
Logger::LogLevel AppConfig::getLogLevel()
{
    appIni->beginGroup("Debug");
    Logger::LogLevel l = Logger::levelFromString(appIni->
                       value("loglevel", Logger::Error).toString());
    appIni->endGroup();
    return l;
}

//设置应用程序的日志级别
void AppConfig::setLogLevel(Logger::LogLevel level)
{
    appIni->beginGroup("Debug");
    appIni->setValue("loglevel", Logger::levelToString(level));
    appIni->endGroup();
}


/**
 * @brief AppConfig::getBaseDbConnect
 *  获取基本库的数据库连接
 * @return
 */
QSqlDatabase AppConfig::getBaseDbConnect()
{
    if(instance)
        return db;
}

void AppConfig::exit()
{
    if(instance){
        db.close();
        QSqlDatabase::removeDatabase("basic");
        appIni->sync();
        delete appIni;
    }

}

/**
 * @brief AppConfig::readPingzhenClass
 *  读取凭证类别
 * @param pzClasses
 * @return
 */
bool AppConfig::readPingzhenClass(QHash<PzClass, QString> &pzClasses)
{
    QString key = "pingzhenClass";
    appIni->beginGroup(key);
    QStringList keys = appIni->childKeys();
    int code; bool ok;
    foreach(key, keys){
        code = key.toInt(&ok);
        if(!ok)
            return false;
        pzClasses[(PzClass)code] = appIni->value(key).toString();
    }
    appIni->endGroup();
    return true;
}

/**
 * @brief AppConfig::readPzStates
 *  获取凭证状态名集合
 * @param names
 * @return
 */
bool AppConfig::readPzStates(QHash<PzState, QString>& names)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_pzStateName);
    if(!q.exec(s))
        return false;
    while(q.next()){
        names[(PzState)q.value(PZSN_CODE).toInt()] = q.value(PZSN_NAME).toString();
    }
    return true;
}

/**
 * @brief AppConfig::readPzSetStates
 *  读取凭证集状态名
 * @param snames
 * @param lnames
 * @return
 */
bool AppConfig::readPzSetStates(QHash<PzsState, QString> &snames, QHash<PzsState, QString> &lnames)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_pzsStateNames);
    if(!q.exec(s))
        return false;
    PzsState code;
    QString sn,ln;
    while(q.next()){
        code = (PzsState)q.value(PZSSN_CODE).toInt();
        snames[code] = q.value(PZSSN_SNAME).toString();
        lnames[code] = q.value(PZSSN_LNAME).toString();
    }
    return true;
}

bool AppConfig::getConfigVar(QString name, int type)
{
    QSqlQuery q(db);
    QString s = QString("select value from configs where (name = '%1') and (type = %2)")
            .arg(name).arg(type);
    if(q.exec(s) && q.first()){
        switch(type){
        case BOOL:
            bv = q.value(0).toBool();
        case INT:
            iv = q.value(0).toInt();
            break;
        case DOUBLE:
            dv = q.value(0).toDouble();
            break;
        case STRING:
            sv = q.value(0).toString();
            break;
        }
        return true;
    }
    else
        return false;
}

//初始化全局配置变量
bool AppConfig::initGlobalVar()
{
    //if(!getConVar("RecentOpenAccId",curAccountId))
    //    curAccountId = 0;
    if(!getConVar("isCollapseJz",isCollapseJz))
        isCollapseJz = true;
    if(!getConVar("isByMtForOppoBa", isByMt))
        isByMt = false;
    if(!getConVar("autoSaveInterval",autoSaveInterval))
        autoSaveInterval = 600000;
    if(!getConVar("jzlrByYear", jzlrByYear))
        jzlrByYear = true;
    if(!getConVar("viewHideColInDailyAcc1", viewHideColInDailyAcc1))
        viewHideColInDailyAcc1 = false;
    if(!getConVar("viewHideColInDailyAcc2", viewHideColInDailyAcc2))
        viewHideColInDailyAcc2 = false;
    if(!getConVar("canZhiRate", czRate))
        czRate = 0;

}

//保存全局配置变量到基础库
bool AppConfig::saveGlobalVar()
{
    bool r;
    //r = setConVar("RecentOpenAccId", curAccountId);
    r = setConVar("isCollapseJz", isCollapseJz);
    r = setConVar("isByMtForOppoBa", isByMt);
    r = setConVar("autoSaveInterval", autoSaveInterval);
    r = setConVar("zlrByYear", autoSaveInterval);
    r = setConVar("viewHideColInDailyAcc1", viewHideColInDailyAcc1);
    r = setConVar("viewHideColInDailyAcc2", viewHideColInDailyAcc2);
    //r = setConVar("viewHideColInDailyAcc1", false);
    //r = setConVar("viewHideColInDailyAcc2", false);
    return r;
}

bool AppConfig::getConVar(QString name, bool& v)
{
    if(getConfigVar(name,BOOL)){
        v = bv;
        return true;
    }
    else
        return false;
}

//获取或设置配置变量的值
bool AppConfig::getConVar(QString name, int& v)
{
   if(getConfigVar(name,INT)){
       v = iv;
       return true;
   }
   else
       return false;
}



bool AppConfig::getConVar(QString name, double& v)
{
    if(getConfigVar(name,DOUBLE)){
        v = dv;
        return true;
    }
    else
        return false;
}

bool AppConfig::getConVar(QString name, QString& v)
{
    if(getConfigVar(name,STRING)){
        v = sv;
        return true;
    }
    else
        return false;
}

bool AppConfig::setConfigVar(QString name,int type)
{
    QSqlQuery q(db);
    QString s = QString("select id from configs where (name='%1') and (type=%2)")
            .arg(name).arg(type);
    if(q.exec(s) && q.first()){
        int id = q.value(0).toInt();
        switch(type){
        case BOOL:
            if(bv)
                s = QString("update configs set value = 1 where id=%2").arg(id);
            else
                s = QString("update configs set value = 0 where id=%2").arg(id);
            break;
        case INT:
            s = QString("update configs set value = %1 where id=%2").arg(iv).arg(id);
            break;
        case DOUBLE:
            s = QString("update configs set value = %1 where id=%2").arg(dv).arg(id);
            break;
        case STRING:
            s = QString("update configs set value = '%1' where id=%2").arg(sv).arg(id);
            break;
        }
    }
    else{
        switch(type){
        case BOOL:
            if(bv)
                s = QString("insert into configs(name,type,value) values('%1',%2,1)")
                        .arg(name).arg(type);
            else
                s = QString("insert into configs(name,type,value) values('%1',%2,0)")
                        .arg(name).arg(type);
        case INT:
            s = QString("insert into configs(name,type,value) values('%1',%2,%3)")
                    .arg(name).arg(type).arg(iv);
            break;
        case DOUBLE:
            s = QString("insert into configs(name,type,value) values('%1',%2,%3)")
                    .arg(name).arg(type).arg(dv);
            break;
        case STRING:
            s = QString("insert into configs(name,type,value) values('%1',%2,'%3')")
                    .arg(name).arg(type).arg(sv);
            break;
        }
    }
    return q.exec(s);
}

bool AppConfig::setConVar(QString name, bool value)
{
    bv = value;
    return setConfigVar(name,BOOL);
}

bool AppConfig::setConVar(QString name, int value)
{
    iv = value;
    return setConfigVar(name,INT);
}

bool AppConfig::setConVar(QString name, double value)
{
    dv = value;
    return setConfigVar(name,DOUBLE);
}

bool AppConfig::setConVar(QString name, QString value)
{
    sv = value;
    return setConfigVar(name,STRING);
}

/**
 * @brief 扫描工作目录下的账户文件，将有效的账户读入账户缓存表
 * @return
 */
bool AppConfig::initAccountCache(QList<AccountCacheItem *> &accCaches)
{
    clearAccountCache();
    QDir dir(DatabasePath);
    QStringList filters;
    filters << "*.dat";
    dir.setNameFilters(filters);
    QFileInfoList filelist = dir.entryInfoList(filters, QDir::Files);
    QList<AccountCacheItem*> accItems;  //扫描到的账户
    if(filelist.empty())
        return true;

    bool result = true;
    QStringList invalidFiles;
    QSqlDatabase adb = QSqlDatabase::addDatabase("QSQLITE","accountRefresh");
    //QSqlQuery q(adb);
    foreach(QFileInfo finfo, filelist){
        QString fileName = finfo.absoluteFilePath();
        adb.setDatabaseName(fileName);
        if(!adb.open()){
            invalidFiles<<finfo.fileName();
            continue;
        }
        QSqlQuery q(adb);
        //读取账户版本信息，以进一步确定文件是否有效
        QString s = QString("select %1,%2 from %3").arg(fld_acci_code)
                .arg(fld_acci_value).arg(tbl_accInfo);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            result = false;
            break;
        }
        QString name,lname,fname,code,version;
        while(q.next()){
            Account::InfoField fi = (Account::InfoField)q.value(0).toInt();
            switch(fi){
            case Account::ACODE:
                code = q.value(1).toString();
                break;
            case Account::SNAME:
                name = q.value(1).toString();
                break;
            case Account::LNAME:
                lname = q.value(1).toString();
                break;
            case Account::FNAME:
                fname = q.value(1).toString();
                break;
            case Account::DBVERSION:
                version = q.value(1).toString();
                break;
            }
        }
        if(version.isEmpty()){
            invalidFiles<<finfo.fileName();
            continue;
        }
        //读取账户的转移信息（以后实现）

        AccountCacheItem *accItem = new AccountCacheItem;
        accItem->code = code;
        accItem->accName = name;
        accItem->accLName = lname;
        accItem->fileName = finfo.fileName();
        //一些转移信息...
        accItem->lastOpened = false;
        if(!saveAccountCacheItem(*accItem)){
            result = false;
            break;
        }
        accCaches<<accItem;
    }
    QSqlDatabase::removeDatabase("accountRefresh");
    return result;
}

/**
 * @brief 清除账户缓存
 * @return
 */
bool AppConfig::clearAccountCache()
{
    QSqlQuery q(db);
    QString s = QString("delete from %1").arg(tbl_localAccountCache);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief 在本地缓存中是否存在指定代码的账户
 * @param code
 * @return
 */
bool AppConfig::isExist(QString code)
{
    QSqlQuery q(db);
    QString s = QString("select id from %1 where %2='%3'")
            .arg(tbl_localAccountCache).arg(fld_lac_code).arg(code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first())
        return true;
    else
        return false;
}

bool AppConfig::saveAccountCacheItem(AccountCacheItem &accInfo)
{
    QSqlQuery q(db);
    QString s;
    if(accInfo.code.isEmpty() || accInfo.code == "0000")
        return true;
    if(isExist(accInfo.code)){
        s = QString("update %1(%2,%3,%4,%5,%6,%7,%8,%9) values('%10','%10','%11',%12,%13,'%14',%15,'%16')")
                .arg(tbl_localAccountCache).arg(fld_lac_name).arg(fld_lac_lname)
                .arg(fld_lac_filename).arg(fld_lac_isLastOpen).arg(fld_lac_tranState)
                .arg(fld_lac_tranInTime).arg(fld_lac_tranOutMid).arg(fld_lac_tranOutTime)
                .arg(accInfo.accName).arg(accInfo.accLName).arg(accInfo.fileName)
                .arg(accInfo.lastOpened).arg(accInfo.tState).arg(accInfo.inTime.toString(Qt::ISODate))
                .arg(accInfo.outMid).arg(accInfo.outTime.toString(Qt::ISODate));

    }
    else{
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) values('%11','%12','%13','%14',0,%15,'%16',%17,'%18')")
                .arg(tbl_localAccountCache).arg(fld_lac_code).arg(fld_lac_name)
                .arg(fld_lac_lname).arg(fld_lac_filename).arg(fld_lac_isLastOpen)
                .arg(fld_lac_tranState).arg(fld_lac_tranInTime).arg(fld_lac_tranOutMid)
                .arg(fld_lac_tranOutTime).arg(accInfo.code).arg(accInfo.accName)
                .arg(accInfo.accLName).arg(accInfo.fileName)/*.arg(accInfo.lastOpened)*/
                .arg(accInfo.tState).arg(accInfo.inTime.toString(Qt::ISODate))
                .arg(accInfo.outMid).arg(accInfo.outTime.toString(Qt::ISODate));

    }
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief 读取指定账户代码的账户缓存信息
 * @param accInfo   此参数必须先设置好账户的代码
 * @return
 */
bool AppConfig::getAccountCacheItem(AccountCacheItem &accInfo)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2=%3")
            .arg(tbl_localAccountCache).arg(fld_lac_code).arg(accInfo.code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        accInfo.code.clear();
        return true;
    }
    accInfo.accName = q.value(LAC_CODE).toString();
    accInfo.accLName = q.value(LAC_LNAME).toString();
    accInfo.fileName = q.value(LAC_FNAME).toString();
    accInfo.lastOpened = q.value(LAC_ISLAST).toBool();
    accInfo.tState = (AccountTransferState)q.value(LAC_TSTATE).toInt();
    accInfo.inTime = QDateTime::fromString(q.value(LAC_INTIME).toString(),Qt::ISODate);
    accInfo.outMid = q.value(LAC_OUTMID).toInt();
    accInfo.outTime = QDateTime::fromString(q.value(LAC_OUTTIME).toString(),Qt::ISODate);
    return true;
}

/**
 * @brief 读取所有账户缓存条目
 * @param accs
 * @return
 */
bool AppConfig::readAccountCaches(QList<AccountCacheItem *> &accs)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_localAccountCache);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    AccountCacheItem* accInfo;
    while(q.next()){
        accInfo = new AccountCacheItem;
        accInfo->code = q.value(LAC_CODE).toString();
        accInfo->accName = q.value(LAC_NAEM).toString();
        accInfo->accLName = q.value(LAC_LNAME).toString();
        accInfo->fileName = q.value(LAC_FNAME).toString();
        accInfo->lastOpened = q.value(LAC_ISLAST).toBool();
        accInfo->tState = (AccountTransferState)q.value(LAC_TSTATE).toInt();
        accInfo->inTime = QDateTime::fromString(q.value(LAC_INTIME).toString(),Qt::ISODate);
        accInfo->outMid = q.value(LAC_OUTMID).toInt();
        accInfo->outTime = QDateTime::fromString(q.value(LAC_OUTTIME).toString(),Qt::ISODate);
        accs.append(accInfo);
    }
    return true;
}

/**
 * @brief 获取最后一次关闭的账户缓存条目
 * @param accItem
 * @return
 */
bool AppConfig::getRecendOpenAccount(AccountCacheItem &accItem)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2=1")
            .arg(tbl_localAccountCache).arg(fld_lac_isLastOpen);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        //accItem.code = "0000";
        return false;
    }
    accItem.code = q.value(LAC_CODE).toString();
    accItem.accName = q.value(LAC_NAEM).toString();
    accItem.accLName = q.value(LAC_LNAME).toString();
    accItem.fileName = q.value(LAC_FNAME).toString();
    accItem.lastOpened = q.value(LAC_ISLAST).toBool();
    accItem.tState = (AccountTransferState)q.value(LAC_TSTATE).toInt();
    accItem.inTime = QDateTime::fromString(q.value(LAC_INTIME).toString(),Qt::ISODate);
    accItem.outMid = q.value(LAC_OUTMID).toInt();
    accItem.outTime = QDateTime::fromString(q.value(LAC_OUTTIME).toString(),Qt::ISODate);
    return true;
}

/**
 * @brief 设置最近打开账户
 * @param code
 * @return
 */
bool AppConfig::setRecentOpenAccount(QString code)
{
    QSqlQuery q(db);
    QString s = QString("update %1 set %2=0 where %2=1")
            .arg(tbl_localAccountCache).arg(fld_lac_isLastOpen);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("update %1 set %2=1 where %3='%4'").arg(tbl_localAccountCache)
                .arg(fld_lac_isLastOpen).arg(fld_lac_code).arg(code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief AppConfig::addAccountInfo
 *  添加账户信息
 * @param code
 * @param aName
 * @param lName
 * @param filename
 * @return
 */
int AppConfig::addAccountInfo(QString code, QString aName, QString lName, QString filename)
{
    appIni->beginGroup("accounts");
    int c = appIni->value("count").toInt();
    c++;
    appIni->setValue("count", c);
    appIni->endGroup();

    QString key = QString("account-%1").arg(c);
    appIni->beginGroup(key);
    appIni->setValue("code", code);
    appIni->setValue("name", aName);
    appIni->setValue("lname", lName);
    appIni->setValue("filename", filename);
    appIni->endGroup();

    return c;
}

int AppConfig::getSpecNameItemCls(AppConfig::SpecNameItemClass witch)
{
    switch(witch){
    case SNIC_CLIENT:
        return 2;
    case SNIC_GDZC:
        return 6;
    }
}

/**
 * @brief AppConfig::getSpecSubCode
 *  获取程序中要特别辨识的科目代码（因为在程序中对这些科目有专门的处理功能）
 * @param subSys    科目系统代码
 * @param witch     指代哪个科目
 * @return
 */
QString AppConfig::getSpecSubCode(int subSys, AppConfig::SpecSubCode witch)
{
    //目前为了简化，将硬编码实现
    switch(witch){
    case SSC_CASH:
        return "1001";
    case SSC_BANK:
        return "1002";
    case SSC_GDZC:
        return "1501";
    case SSC_CWFY:
        return "5503";
    case SSC_BNLR:
        return "3131";
    case SSC_LRFP:
        return "3141";
    case SSC_YS:
        return "1131";
    case SSC_YF:
        return "2121";
    }
}

/**
 * @brief AppConfig::setSpecSubCode
 * @param subSys
 * @param witch
 * @param code
 */
void AppConfig::setSpecSubCode(int subSys, AppConfig::SpecSubCode witch, QString code)
{
    //待以后决定了如何保存这些特别科目的机制后，再实现
}

/**
 * @brief AppConfig::getSubjectClassMaps
 *  获取指定科目系统的科目类别映射表（将实际的科目类别代码映射到内置科目类别代码）
 * @param subSys
 * @return
 */
QHash<int, SubjectClass> AppConfig::getSubjectClassMaps(int subSys)
{
    QHash<int, SubjectClass> maps;
    if(subSys == 1){
        maps[1] = SC_ZC;
        maps[2] = SC_FZ;
        maps[3] = SC_QY;
        maps[4] = SC_CB;
        maps[5] = SC_SY;
    }
    else{
        maps[1] = SC_ZC;
        maps[2] = SC_FZ;
        maps[3] = SC_GT;
        maps[4] = SC_QY;
        maps[5] = SC_CB;
        maps[6] = SC_SY;
    }
    return maps;
}



/**
 * @brief AppConfig::getLocalMid
 *  获取本机ID标识
 * @return 0：出错，比如没有找到isLocal字段为1的记录
 */
int AppConfig::getLocalMid()
{
    QSqlQuery q(db);
    QString s = "select mid from machines where isLocal=1";
    if(!q.exec(s) || !q.first())
        return 0;
    return q.value(0).toInt();
}

/**
 * @brief AppConfig::readPzEwTableState
 *  读取凭证编辑窗口的默认窗口尺寸、表格行高和列宽等信息
 * @param dim
 * @param infos
 * @return
 */
bool AppConfig::readPzEwTableState(QList<int>& infos)
{
//    dim.setWidth(1000);
//    dim.setHeight(600);
//    dim.setTop(10);
//    dim.setLeft(10);
    infos<<30<<400<<80<<100<<80<<150;//默认表格行高、摘要列、一二级科目、币种、金额列宽
    return true;
}

