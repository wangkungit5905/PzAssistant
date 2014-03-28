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
#include "transfers.h"
#include "globalVarNames.h"


QSettings* AppConfig::appIni=0;
AppConfig* AppConfig::instance = 0;
QSqlDatabase AppConfig::db;

//////////////////////////////////AppConfig//////////////////////////////////////
AppConfig::AppConfig()
{
    //appIni->setIniCodec(QTextCodec::codecForTr());
    _initMachines();
    _initAccountCaches();
    _initSpecSubCodes();
    _initSpecNameItemClses();
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
    appIni->setIniCodec(QTextCodec::codecForLocale());

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
    if(!instance)
        getInstance();
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
    if(!getConVar(GVN_RECENT_LOGIN_USER, recentUserId))
        recentUserId = 1;
    if(!getConVar(GVN_IS_COLLAPSE_JZ,isCollapseJz))
        isCollapseJz = true;
    if(!getConVar(GVN_IS_BY_MT_FOR_OPPO_BA, isByMt))
        isByMt = false;
    if(!getConVar(GVN_AUTOSAVE_INTERVAL,autoSaveInterval))
        autoSaveInterval = 600000;
    if(!getConVar(GVN_JZLR_BY_YEAR, jzlrByYear))
        jzlrByYear = true;
    if(!getConVar(GVN_VIEWORHIDE_COL_IN_DAILY_1, viewHideColInDailyAcc1))
        viewHideColInDailyAcc1 = false;
    if(!getConVar(GVN_VIEWORHIDE_COL_IN_DAILY_2, viewHideColInDailyAcc2))
        viewHideColInDailyAcc2 = false;
    if(!getConVar(GVN_GDZC_CZ_RATE, czRate))
        czRate = 0;
    if(!getConVar(GVN_IS_RUNTIMNE_UPDATE_EXTRA, rt_update_extra))
        rt_update_extra = true;
}

//保存全局配置变量到基础库
bool AppConfig::saveGlobalVar()
{
    bool r;
    setConVar(GVN_RECENT_LOGIN_USER, recentUserId);
    r = setConVar(GVN_IS_COLLAPSE_JZ, isCollapseJz);
    r = setConVar(GVN_IS_BY_MT_FOR_OPPO_BA, isByMt);
    r = setConVar(GVN_AUTOSAVE_INTERVAL, autoSaveInterval);
    r = setConVar(GVN_JZLR_BY_YEAR, autoSaveInterval);
    r = setConVar(GVN_VIEWORHIDE_COL_IN_DAILY_1, viewHideColInDailyAcc1);
    r = setConVar(GVN_VIEWORHIDE_COL_IN_DAILY_2, viewHideColInDailyAcc2);
    r = setConVar(GVN_IS_RUNTIMNE_UPDATE_EXTRA, rt_update_extra);
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
    qDeleteAll(accountCaches);
    accountCaches.clear();
    init_accCache = false;
    return true;
}

/**
 * @brief 在本地缓存中是否存在指定代码的账户
 * @param code
 * @return
 */
bool AppConfig::isExist(QString code)
{
    if(!init_accCache)
        return false;
    if(accountCaches.isEmpty())
        return false;
    foreach(AccountCacheItem* item, accountCaches){
        if(code == item->code)
            return true;
    }
    return false;
}

/**
 * @brief AppConfig::refreshLocalAccount
 *  搜索当前工作目录下的所有有效账户文件，并将缓存账户保存在基本库中
 * @param count
 * @return
 */
bool AppConfig::refreshLocalAccount(int &count)
{
    if(!_searchAccount()){
        count = 0;
        return false;
    }
    count = accountCaches.count();
    return true;
}

bool AppConfig::addAccountCacheItem(AccountCacheItem *accItem)
{
    if(!accItem)
        return false;
    if(isExist(accItem->code))
        return false;
    accountCaches<<accItem;
    return _saveAccountCacheItem(accItem);
}

bool AppConfig::saveAccountCacheItem(AccountCacheItem *accInfo)
{
    if(!_isValidAccountCode(accInfo->code)){
        LOG_WARNING(QString("Invalid account code(%1)").arg(accInfo->code));
        return false;
    }
    return _saveAccountCacheItem(accInfo);
}

/**
 * @brief AppConfig::saveAllAccountCaches
 *  将缓存账户保存到本机基本库中
 * @return
 */
bool AppConfig::saveAllAccountCaches()
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save all cached account item!");
        return false;
    }
    foreach(AccountCacheItem* item, accountCaches){
        if(!_saveAccountCacheItem(item))
            return false;
    }
    if(!db.commit()){
        if(!db.rollback())
            LOG_SQLERROR("Rollback transaction failed on save all cached account item!");
        LOG_SQLERROR("Commit transaction failed on save all cached account item!");
    }
    return true;
}

/**
 * @brief 移除账户缓存
 * @param accInfo
 * @return
 */
bool AppConfig::removeAccountCache(AccountCacheItem *accInfo)
{
    QSqlQuery q(db);
    QString s = QString("delete from %1 where id = %2").arg(tbl_localAccountCache).arg(accInfo->id);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    accountCaches.removeOne(accInfo);
    return true;
}

/**
 * @brief 读取指定账户代码的账户缓存信息
 * @param code   账户代码
 * @return
 */
AccountCacheItem *AppConfig::getAccountCacheItem(QString code)
{
    if(!init_accCache)
        return NULL;
    foreach(AccountCacheItem* item, accountCaches){
        if(item->code == code)
            return item;
    }
    return NULL;
}

/**
 * @brief 读取所有账户缓存条目
 * @param accs
 * @return
 */
QList<AccountCacheItem *> AppConfig::getAllCachedAccounts()
{
    return accountCaches;
}

/**
 * @brief 获取最后一次关闭的账户缓存条目
 * @param accItem
 * @return
 */
AccountCacheItem* AppConfig::getRecendOpenAccount()
{
    if(!init_accCache)
        return NULL;
    foreach(AccountCacheItem* item, accountCaches){
        if(item->lastOpened)
            return item;
    }
    return NULL;
}

/**
 * @brief 设置最近打开账户
 * @param code
 * @return
 */
void AppConfig::setRecentOpenAccount(QString code)
{
    if(!init_accCache)
        return;
    if(!isExist(code))
        return;
    foreach(AccountCacheItem* item,accountCaches){
        if(code == item->code)
            item->lastOpened = true;
        else
            item->lastOpened = false;
    }
    saveAllAccountCaches();
}

/**
 * @brief 清除最近打开账户的标志
 */
void AppConfig::clearRecentOpenAccount()
{
    if(!init_accCache)
        return;
    foreach(AccountCacheItem* item,accountCaches){
        if(item->lastOpened){
            item->lastOpened = false;
            _saveAccountCacheItem(item);
            return;
        }
    }
}

/**
 * @brief AppConfig::getAccTranStates
 *  返回账户转移状态名称表
 * @return
 */
QHash<AccountTransferState, QString> AppConfig::getAccTranStates()
{
    QHash<AccountTransferState, QString> states;
    states[ATS_INVALID] = QObject::tr("无效状态");
    states[ATS_TRANSINDES] = QObject::tr("已转入到目的主机");
    states[ATS_TRANSINOTHER] = QObject::tr("已转入到非目标主机");
    states[ATS_TRANSOUTED] = QObject::tr("已转出本机");
    return states;
}

/**
 * @brief 读取所有应用支持的科目系统
 * @param items
 * @return
 */
bool AppConfig::getSubSysItems(QList<SubSysNameItem *>& items)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2").arg(tbl_subSys).arg(fld_ss_code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        SubSysNameItem* si = new SubSysNameItem;
        si->code = q.value(SS_CODE).toInt();
        si->name = q.value(SS_NAME).toString();
        si->explain = q.value(SS_EXPLAIN).toString();
        si->isImport = false;
        si->isConfiged = false;
        items<<si;
    }
    return true;
}

/**
 * @brief AppConfig::getSupportMoneyType
 *  获取系统支持的货币类型
 * @param moneys
 * @return
 */
bool AppConfig::getSupportMoneyType(QHash<int, Money*> &moneys)
{
    if(moneyTypes.isEmpty()){
        QSqlQuery q(db);
        QString s = QString("select * from %1").arg(tbl_base_mt);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        int code;
        QString sign,name;
        while(q.next()){
            code = q.value(BASE_MT_CODE).toInt();
            sign = q.value(BASE_MT_SIGN).toString();
            name = q.value(BASE_MT_NAME).toString();
            moneyTypes[code] = new Money(code,name,sign);
        }
    }
    moneys = moneyTypes;
    return true;
}

/**
 * @brief 更新账户数据库表格创建语句
 * @param names 表格名
 * @param sqls  创建表格的sql语句
 */
void AppConfig::updateTableCreateStatment(QStringList names, QStringList sqls)
{
    if(names.count() != sqls.count())
        return;
    if(names.isEmpty())
        return;
    QSqlQuery q(db);
    QString s = QString("select count() from sqlite_master where name='%1'")
            .arg(tbl_table_create_sqls);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return;
    }
    q.first();
    if(q.value(0).toInt() == 0){
        s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY,name TEXT,sql TEXT)")
                .arg(tbl_table_create_sqls);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return;
        }
    }
    s = QString("delete from %1").arg(tbl_table_create_sqls);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return;
    }
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on insert create table sql statment!");
        return;
    }
    s = QString("insert into %1(name,sql) values(:name,:sql)").arg(tbl_table_create_sqls);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return;
    }
    for(int i = 0; i < names.count(); ++i){
        q.bindValue(":name",names.at(i));
        q.bindValue(":sql",sqls.at(i));
        if(!q.exec()){
            LOG_SQLERROR(s);
            return;
        }
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on insert create table sql statment!");
        return;
    }
}

/**
 * @brief 获取账户数据库表格创建语句
 * @param names
 * @param sqls
 * @return
 */
bool AppConfig::getUpdateTableCreateStatment(QStringList& names, QStringList& sqls)
{
    QSqlQuery q(db);
    QString s = QString("select name,sql from %1").arg(tbl_table_create_sqls);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        names<<q.value(0).toString();
        sqls<<q.value(1).toString();
    }
    return true;
}

/**
 * @brief AppConfig::_isValidAccountCode
 *  判断账户代码是否有效
 *  代码不符合规定，代码为空，代码重复冲突等都视为无效
 * @param code
 * @return
 */
bool AppConfig::_isValidAccountCode(QString code)
{
    return true;
}

/**
 * @brief AppConfig::_saveAccountCacheItem
 *  实际地保存缓存账户到本机基本库中的方法
 * @param accInfo
 * @return
 */
bool AppConfig::_saveAccountCacheItem(AccountCacheItem *accInfo)
{
    QSqlQuery q(db);
    QString s;
    bool isNew = (accInfo->id == UNID);
    if(!isNew){
        s = QString("update %1 set %2='%3',%4='%5',%6='%7',%8=%9,%10=%11,%12='%13',%14=%15,%16='%17' where id=%18")
                .arg(tbl_localAccountCache).arg(fld_lac_name).arg(accInfo->accName)
                .arg(fld_lac_lname).arg(accInfo->accLName).arg(fld_lac_filename)
                .arg(accInfo->fileName).arg(fld_lac_isLastOpen).arg(accInfo->lastOpened?1:0)
                .arg(fld_lac_tranState).arg(accInfo->tState).arg(fld_lac_tranInTime)
                .arg(accInfo->inTime.toString(Qt::ISODate)).arg(fld_lac_tranOutMid)
                .arg(accInfo->mac->getMID()).arg(fld_lac_tranOutTime)
                .arg(accInfo->outTime.toString(Qt::ISODate)).arg(accInfo->id);

    }
    else{
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) values('%11','%12','%13','%14',0,%15,'%16',%17,'%18')")
                .arg(tbl_localAccountCache).arg(fld_lac_code).arg(fld_lac_name)
                .arg(fld_lac_lname).arg(fld_lac_filename).arg(fld_lac_isLastOpen)
                .arg(fld_lac_tranState).arg(fld_lac_tranInTime).arg(fld_lac_tranOutMid)
                .arg(fld_lac_tranOutTime).arg(accInfo->code).arg(accInfo->accName)
                .arg(accInfo->accLName).arg(accInfo->fileName)/*.arg(accInfo->lastOpened)*/
                .arg(accInfo->tState).arg(accInfo->inTime.toString(Qt::ISODate))
                .arg(accInfo->mac->getMID()).arg(accInfo->outTime.toString(Qt::ISODate));

    }
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(isNew){
        //accountCaches<<accInfo;
        s = "select last_insert_rowid()";
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.first();
        accInfo->id = q.value(0).toInt();
    }
    return true;
}

/**
 * @brief 扫描工作目录下的账户文件，将有效的账户读入账户缓存表
 * @return
 */
bool AppConfig::_searchAccount()
{
    clearAccountCache();
    QDir dir(DATABASE_PATH);
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
        //读取账户的转移信息
        s = QString("select * from %1").arg(tbl_transfer);
        AccountCacheItem *accItem = new AccountCacheItem;
        accItem->id = UNID;
        accItem->code = code;
        accItem->accName = name;
        accItem->accLName = lname;
        accItem->fileName = finfo.fileName();
        accItem->lastOpened = false;
        //如果账户不存在转移记录（未升级账户文件，没有相应的转移表），则先将它视作有一条初始的本机转入本机的转移记录
        //在下次打开该账户时，升级账户时将创建相应的转移表和转移记录
        if(!q.exec(s) || !q.last()){
            accItem->mac = getLocalMachine();
            accItem->outTime = QDateTime::currentDateTime();
            accItem->inTime = QDateTime::currentDateTime();
            accItem->tState = ATS_TRANSINDES;
        }
        else{
            accItem->mac = machines.value(q.value(TRANS_SMID).toInt());
            //accItem->outTime = q.value(TRANS_OUTTIME).toDateTime();
            //accItem->inTime = q.value(TRANS_INTIME).toDateTime();
            accItem->outTime = QDateTime::fromString(q.value(TRANS_OUTTIME).toString(),Qt::ISODate);
            accItem->inTime = QDateTime::fromString(q.value(TRANS_INTIME).toString(),Qt::ISODate);
            accItem->tState = (AccountTransferState)q.value(TRANS_STATE).toInt();
        }
        accountCaches<<accItem;
    }
    if(!saveAllAccountCaches()){
        LOG_ERROR("Save searched account failed on refresh loacal cached account!");
        result = false;
    }
    QSqlDatabase::removeDatabase("accountRefresh");
    if(result){
        init_accCache = true;
        return true;
    }
    else{
        init_accCache = false;
        return false;
    }
}

/**
 * @brief AppConfig::_initAccountCaches
 *  读取本地账户缓存表，并初始化accountCaches列表
 * @return
 */
bool AppConfig::_initAccountCaches()
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_localAccountCache);
    init_accCache = false;
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    AccountCacheItem* item;
    while(q.next()){
        item = new AccountCacheItem;
        item->id = q.value(0).toInt();
        item->code = q.value(LAC_CODE).toString();
        item->accName = q.value(LAC_NAEM).toString();
        item->accLName = q.value(LAC_LNAME).toString();
        item->fileName = q.value(LAC_FNAME).toString();
        item->lastOpened = q.value(LAC_ISLAST).toBool();
        item->mac = machines.value(q.value(LAC_MAC).toInt());
        item->outTime = q.value(LAC_OUTTIME).toDateTime();
        item->inTime = q.value(LAC_INTIME).toDateTime();
        item->tState = (AccountTransferState)q.value(LAC_TSTATE).toInt();
        accountCaches<<item;
    }
    init_accCache = true;
}

/**
 * @brief AppConfig::_initMachines
 *  读取主机条目，并初始化machines表
 */
void AppConfig::_initMachines()
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_machines);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        QMessageBox::critical(0,QObject::tr("出错信息"),QObject::tr("无法读取机器信息，请检查账户数据库相应表格是否有误！"));
        return;
    }
    int id,mid;
    QString name,desc;
    MachineType mtype;
    bool isLocal;
    while(q.next()){
        id = q.value(0).toInt();
        mid = q.value(MACS_MID).toInt();
        mtype = (MachineType)q.value(MACS_TYPE).toInt();
        isLocal = q.value(MACS_ISLOCAL).toBool();
        name = q.value(MACS_NAME).toString();
        desc = q.value(MACS_DESC).toString();
        Machine* m = new Machine(id,mtype,mid,isLocal,name,desc);
        machines[m->getMID()] = m;
    }
}

/**
 * @brief 初始化特定科目代码表
 */
void AppConfig::_initSpecSubCodes()
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_sscc);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        QMessageBox::critical(0,QObject::tr("出错信息"),QObject::tr("无法读取特定科目配置代码表，请检查基本库相应表格是否有误！"));
        return;
    }
    int subSys;
    SpecSubCode subEnum;
    QString code;
    while(q.next()){
        subSys = q.value(SSCC_SUBSYS).toInt();
        subEnum = (SpecSubCode)q.value(SSCC_ENUM).toInt();
        code = q.value(SSCC_CODE).toString();
        specCodes[subSys][subEnum] = code;
    }

}

/**
 * @brief 初始化特定名称类别代码表
 */
void AppConfig::_initSpecNameItemClses()
{
    QSqlQuery q(db),q2(db);
    QString common_client = QObject::tr("业务客户");
    QString logistics_client = QObject::tr("物流企业");
    QString bank_client = QObject::tr("金融机构");
    QString gdzc_class = QObject::tr("固定资产类");
    //先找出一个最大可用的类别代码
    QString s = QString("select max(%1) from %2").arg(fld_base_nic_code).arg(tbl_base_nic);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return;
    }
    q.first();
    int maxCode = q.value(0).toInt();

    s = QString("select %1 from %2 where %3=:name")
            .arg(fld_base_nic_code).arg(tbl_base_nic).arg(fld_base_nic_name);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return;
    }
    s = QString("insert into %1(%2,%3,%4) values(:code,:name,:explain)")
            .arg(tbl_base_nic).arg(fld_base_nic_code).arg(fld_base_nic_name)
            .arg(fld_base_nic_explain);
    if(!q2.prepare(s)){
        LOG_SQLERROR(s);
        return;
    }
    q.bindValue(":name",common_client);
    if(q.exec() && q.first())
        specNICs[SNIC_COMMON_CLIENT] = q.value(0).toInt();
    else{
        q2.bindValue("code",++maxCode);
        q2.bindValue(":name",common_client);
        q2.bindValue(":explain",QObject::tr("没有物流能力的通用企业用户"));
        if(!q2.exec())
            return;
        specNICs[SNIC_COMMON_CLIENT] = maxCode;
    }
    q.bindValue(":name",logistics_client);
    if(q.exec() && q.first())
        specNICs[SNIC_WL_CLIENT] = q.value(0).toInt();
    else{
        q2.bindValue(":code",++maxCode);
        q2.bindValue(":name",logistics_client);
        q2.bindValue(":explain",QObject::tr("本身有物流能力的物流企业用户"));
        if(!q2.exec())
            return;
        specNICs[SNIC_WL_CLIENT] = maxCode;
    }
    q.bindValue(":name",bank_client);
    if(q.exec() && q.first())
        specNICs[SNIC_BANK] = q.value(0).toInt();
    else{
        q2.bindValue("code",++maxCode);
        q2.bindValue(":name",bank_client);
        q2.bindValue(":explain",QObject::tr("有资金划转能力的机构，通常指银行"));
        if(!q2.exec())
            return;
        specNICs[SNIC_BANK] = maxCode;
    }
    q.bindValue(":name",gdzc_class);
    if(q.exec() && q.first())
        specNICs[SNIC_GDZC] = q.value(0).toInt();
    else{
        q2.bindValue("code",++maxCode);
        q2.bindValue(":name",bank_client);
        q2.bindValue(":explain",QObject::tr("可以作为固定资产的物品名称类"));
        if(!q2.exec())
            return;
        specNICs[SNIC_GDZC] = maxCode;
    }
}

bool AppConfig::_saveMachine(Machine *mac)
{
    if(!mac)
        return false;
    QSqlQuery q(db);
    QString s;
    if(mac->getId() == UNID)
        s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,%8,%9,'%10','%11')")
                .arg(tbl_machines).arg(fld_mac_mid).arg(fld_mac_type).arg(fld_mac_islocal)
                .arg(fld_mac_sname).arg(fld_mac_desc).arg(mac->getMID()).arg(mac->getType())
                .arg(mac->isLocalMachine()?1:0).arg(mac->name()).arg(mac->description());
    else
        s = QString("update %1 set %2=%3,%4=%5,%6=%7,%8='%9',%10='%11' where id=%12")
                .arg(tbl_machines).arg(fld_mac_mid).arg(mac->getMID()).arg(fld_mac_type)
                .arg(mac->getType()).arg(fld_mac_islocal).arg(mac->isLocalMachine()?1:0)
                .arg(fld_mac_sname).arg(mac->name()).arg(fld_mac_desc).arg(mac->description()).arg(mac->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(mac->getId() == UNID){
        s = "select last_insert_rowid()";
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.first();
        mac->id = q.value(0).toInt();
        machines[mac->getMID()] = mac;
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

/**
 * @brief AppConfig::getSpecNameItemCls
 *  返回特定名称条目类别的代码（因为这些代码将被硬编码到程序中，要是程序正常运行，必须获取正确的类别编码）
 * @param witch
 * @return
 */
int AppConfig::getSpecNameItemCls(AppConfig::SpecNameItemClass witch)
{
    return specNICs.value(witch);
//    switch(witch){
//    case SNIC_COMMON_CLIENT:
//        return 2;
//    case SNIC_WL_CLIENT:
//        return 29;
//    case SNIC_GDZC:
//        return 6;
//    case SNIC_BANK:
//        return 3;
//    }
}

/**
 * @brief 指定科目系统的特定科目是否已经配置完成
 * @param subSys
 * @return
 */
bool AppConfig::isSpecSubCodeConfiged(int subSys)
{
    QSqlQuery q(db);
    QString s = QString("select count() from %1 where subSys = %2")
            .arg(tbl_sscc).arg(subSys);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.first();
    int num = q.value(0).toInt();
    if(num == SPEC_SUBJECT_NUMS)
        return true;
    if(num == 0)
        return false;
    s = QString("delete from %1 where %2=%3")
            .arg(tbl_sscc).arg(fld_sscc_subSys).arg(subSys);
    if(!q.exec(s))
        LOG_SQLERROR(s);
    return false;
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
//    switch(witch){
//    case SSC_CASH:
//        return "1001";
//    case SSC_BANK:
//        return "1002";
//    case SSC_GDZC:
//        return "1501";
//    case SSC_CWFY:
//        return "5503";
//    case SSC_BNLR:
//        return "3131";
//    case SSC_LRFP:
//        return "3141";
//    case SSC_YS:
//        return "1131";
//    case SSC_YF:
//        return "2121";
//    }
    return specCodes.value(subSys).value(witch);
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
 * @brief AppConfig::getMachineTypes
 *  返回系统支持的主机类型（即保存账户文件处所）
 * @return
 */
QHash<MachineType, QString> AppConfig::getMachineTypes()
{
    QHash<MachineType, QString> mt;
    mt[MT_COMPUTER] = QObject::tr("物理电脑");
    mt[MT_CLOUDY] = QObject::tr("云账户");
    return mt;
}



/**
 * @brief AppConfig::getLocalMachine
 *  获取本机对象
 * @return
 */
Machine *AppConfig::getLocalMachine()
{
    QHashIterator<int,Machine*> it(machines);
    while(it.hasNext()){
        it.next();
        if(it.value()->isLocalMachine())
            return it.value();
    }
    return NULL;
}

/**
 * @brief AppConfig::saveMachine
 *  将指定的主机信息保存到本机基本库的主机表中
 * @param mac
 * @return
 */
bool AppConfig::saveMachine(Machine *mac)
{
    return _saveMachine(mac);
}

bool AppConfig::saveMachines(QList<Machine *> macs)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save machines list!");
        return false;
    }
    foreach(Machine* mac, macs){
        if(!_saveMachine(mac))
            return false;
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on save machines list!");
        if(!db.rollback())
            LOG_SQLERROR("Rollback transaction failed on save machines list!");
        return false;
    }
    return true;
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

