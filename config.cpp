#include <QSettings>
#include <QStringList>
#include <QTextCodec>
#include <QInputDialog>
#include <QFileInfo>
#include <QDir>
#include <QBuffer>
#include <QTextStream>

#include "global.h"
#include "config.h"
#include "tables.h"
#include "version.h"
#include "globalVarNames.h"
#include "subject.h"
#include "myhelper.h"
#include "otherModule.h"


QSettings* AppConfig::appIni=0;
AppConfig* AppConfig::instance = 0;
QSqlDatabase AppConfig::db;

//////////////////////////////////AppConfig//////////////////////////////////////
AppConfig::AppConfig()
{

    bool b = true;
    b &= _initCfgVars();
    b &= _initMachines();
    b &= _initAccountCaches();
    b &= _initSubSysNames();
    b &=_initSpecSubCodes();
    b &=_initSpecNameItemClses();
    if(!b)
        myHelper::ShowMessageBoxError(QObject::tr("应用配置对象初始化期间发生错误！"));
}

AppConfig::~AppConfig()
{
    exit();    
    qDeleteAll(subSysNames);
    qDeleteAll(moneyTypes);
    qDeleteAll(machines);
    qDeleteAll(accountCaches);
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

QString AppConfig::getAppStyleName()
{
    appIni->beginGroup("AppStyle");
    QString name = appIni->value("styleName").toString();
    appIni->endGroup();
    if(name.isEmpty()){
        name = "navy";
        appIni->setValue("AppStyle/styleName",name);
        appIni->sync();
    }
    return name;
}

void AppConfig::setAppStyleName(QString styleName)
{
    appIni->beginGroup("AppStyle");
    appIni->setValue("styleName",styleName);
    appIni->endGroup();
    appIni->sync();
}

/**
 * @brief AppConfig::getStyleFrom
 * 样式表文件来自于哪里
 * @return true：从应用程序的资源中获取，否则从应用的子目录“styles”获取
 */
bool AppConfig::getStyleFrom()
{
    appIni->beginGroup("AppStyle");
    bool fromRes = true;
    QVariant v = appIni->value("styleFrom");
    appIni->endGroup();
    if(v.isValid())
        fromRes = v.toBool();
    else{
        appIni->setValue("AppStyle/styleFrom",true);
        appIni->sync();
    }
    return fromRes;
}

void AppConfig::setStyleFrom(bool fromRes)
{
    appIni->beginGroup("AppStyle");
    appIni->setValue("styleFrom",fromRes);
    appIni->endGroup();
    appIni->sync();
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



//初始化全局配置变量
bool AppConfig::_initCfgVars()
{
    _initCfgVarDefs();
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_base_Cfg);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        CfgValueCode code = (CfgValueCode)q.value(BCONF_ENUM).toInt();
        VarType type = (VarType)q.value(BCONF_TYPE).toInt();

        switch(type){
        case BOOL:
            if(code >= CFGV_BOOL_NUM){
                LOG_ERROR(QString("Global config variable value is error! (type=%1,code=%2)")
                          .arg(type).arg(code));
                continue;
            }
            boolCfgs[code] = q.value(BCONF_VALUE).toBool();
            break;
        case INT:
            if(code >= CFGV_INT_NUM){
                LOG_ERROR(QString("Global config variable value is error! (type=%1,code=%2)")
                          .arg(type).arg(code));
                continue;
            }
            intCfgs[code] = q.value(BCONF_VALUE).toInt();
            break;
        case DOUBLE:
            if(code >= CFGV_DOUBLE_NUM){
                LOG_ERROR(QString("Global config variable value is error! (type=%1,code=%2)")
                          .arg(type).arg(code));
                continue;
            }
            doubleCfgs[code] = Double(q.value(BCONF_VALUE).toDouble());
            break;
        }
    }
    return true;
}

//保存全局配置变量到基础库
bool AppConfig::saveGlobalVar()
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save global config variable!");
        return false;
    }
    QSqlQuery q1(db),q2(db);
    QString s = QString("update %1 set %2=:value where %3=:code and %4=:type")
            .arg(tbl_base_Cfg).arg(fld_bconf_value).arg(fld_bconf_enum)
            .arg(fld_bconf_type);
    if(!q1.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4) values(:type,:code,:value)")
            .arg(tbl_base_Cfg).arg(fld_bconf_type).arg(fld_bconf_enum)
            .arg(fld_bconf_value);
    if(!q2.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    //保存布尔类型配置变量
    for(int i = 0; i < CFGV_BOOL_NUM; ++i){
        q1.bindValue(":value",boolCfgs[i]?1:0);
        q1.bindValue(":code",i);
        q1.bindValue(":type",BOOL);
        if(!q1.exec()){
            LOG_SQLERROR(q1.lastQuery());
            return false;
        }
        int nums = q1.numRowsAffected();
        if(nums == 0){
            q2.bindValue(":type",BOOL);
            q2.bindValue(":code",i);
            q2.bindValue(":value",boolCfgs[i]?1:0);
            if(!q2.exec()){
                LOG_SQLERROR(q2.lastQuery());
                return false;
            }
        }
    }
    //保存整形配置变量
    for(int i = 0; i < CFGV_INT_NUM; ++i){
        q1.bindValue(":value",intCfgs[i]);
        q1.bindValue(":code",i);
        q1.bindValue(":type",INT);
        if(!q1.exec()){
            LOG_SQLERROR(q1.lastQuery());
            return false;
        }
        int nums = q1.numRowsAffected();
        if(nums == 0){
            q2.bindValue(":type",INT);
            q2.bindValue(":code",i);
            q2.bindValue(":value",intCfgs[i]);
            if(!q2.exec()){
                LOG_SQLERROR(q2.lastQuery());
                return false;
            }
        }
    }
    //保存双精度型配置变量
    for(int i = 0; i < CFGV_DOUBLE_NUM; ++i){
        q1.bindValue(":value",doubleCfgs[i].toString2());
        q1.bindValue(":code",i);
        q1.bindValue(":type",DOUBLE);
        if(!q1.exec()){
            LOG_SQLERROR(q1.lastQuery());
            return false;
        }
        int nums = q1.numRowsAffected();
        if(nums == 0){
            q2.bindValue(":type",DOUBLE);
            q2.bindValue(":code",i);
            q2.bindValue(":value",doubleCfgs[i].toString2());
            if(!q2.exec()){
                LOG_SQLERROR(q2.lastQuery());
                return false;
            }
        }
    }

    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on save global config variable!");
        db.rollback();
        return false;
    }
    return true;
}

void AppConfig::getCfgVar(AppConfig::CfgValueCode varCode, Double &v)
{
    if(varCode >= CFGV_DOUBLE_NUM)
        return;
    v = doubleCfgs[varCode];
}

void AppConfig::setCfgVar(AppConfig::CfgValueCode varCode, bool v)
{
    if(varCode >= CFGV_BOOL_NUM)
        return;
    boolCfgs[varCode] = v;
}

void AppConfig::setCfgVar(AppConfig::CfgValueCode varCode, int v)
{
    if(varCode >= CFGV_INT_NUM)
        return;
    intCfgs[varCode] = v;
}

void AppConfig::setCfgVar(AppConfig::CfgValueCode varCode, double v)
{
    if(varCode >= CFGV_DOUBLE_NUM)
        return;
    doubleCfgs[varCode] = v;
}

void AppConfig::getCfgVar(CfgValueCode varCode, int &v)
{
    if(varCode >= CFGV_INT_NUM)
        return;
    v = intCfgs[varCode];
}

void AppConfig::getCfgVar(CfgValueCode varCode, bool &v)
{
    if(varCode >= CFGV_BOOL_NUM)
        return;
    v = boolCfgs[varCode];
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

//bool AppConfig::addAccountCacheItem(AccountCacheItem *accItem)
//{
//    if(!accItem)
//        return false;
//    if(isExist(accItem->code))
//        return false;
//    if(!_saveAccountCacheItem(accItem))
//        return false;
//    accountCaches<<accItem;
//    return true;
//}

bool AppConfig::saveAccountCacheItem(AccountCacheItem *accInfo)
{
    if(!_isValidAccountCode(accInfo->code)){
        LOG_WARNING(QString("Invalid account code(%1)").arg(accInfo->code));
        return false;
    }
    if(!_saveAccountCacheItem(accInfo))
        return false;
    if(!isExist(accInfo->code))
        accountCaches<<accInfo;
    return true;
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
    states[ATS_TRANSINDES] = QObject::tr("已转入到目的站");
    states[ATS_TRANSINOTHER] = QObject::tr("已转入到非目标站");
    states[ATS_TRANSOUTED] = QObject::tr("已转出本站");
    return states;
}

/**
 * @brief 读取所有应用支持的科目系统
 * @param items
 * @return
 */
void AppConfig::getSubSysItems(QList<SubSysNameItem *>& items)
{
    QList<int> codes = subSysNames.keys();
    qSort(codes.begin(),codes.end());
    for(int i = 0;i<codes.count();++i)
        items<<subSysNames.value(codes.at(i));
}

/**
 * @brief 返回指定科目系统名称项目
 * @param subSysCode    科目系统代码
 * @return
 */
SubSysNameItem *AppConfig::getSpecSubSysItem(int subSysCode)
{
    return subSysNames.value(subSysCode);
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
        QString name = names.at(i);
        if(name.indexOf(tbl_sndsub_join_pre) > -1)
            continue;
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
 * @brief 设置指定科目系统的启用的科目（这是一个临时性函数，用来一次性将在基本库中设置好启用的科目）
 * @param subSys    科目系统代码
 * @param codes     启用科目的科目代码列表
 * @return
 */
bool AppConfig::setEnabledFstSubs(int subSys, QStringList codes)
{
    QSqlQuery q(db);
    if(!db.transaction())
        return false;
    QString s = QString("update %1 set %2=0 where %3=%4").arg(tbl_base_fsub)
            .arg(fld_base_fsub_isenabled).arg(fld_base_fsub_subsys).arg(subSys);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("update %1 set %2=1 where %3=%4 and %5=:code").arg(tbl_base_fsub)
            .arg(fld_base_fsub_isenabled).arg(fld_base_fsub_subsys).arg(subSys)
            .arg(fld_base_fsub_subcode);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    foreach(QString code, codes){
        q.bindValue(":code",code);
        if(!q.exec())
            return false;
    }
    if(!db.commit()){
        db.rollback();
        return false;
    }
    return true;
}

/**
 * @brief 设置指定科目系统科目的记账方向
 * @param subSys    科目系统代码
 * @param codes     记账方向为正向的科目代码类别
 * @return
 */
bool AppConfig::setSubjectJdDirs(int subSys, QStringList codes)
{
    if(!db.transaction())
        return false;
    QSqlQuery q(db);
    QString s = QString("update %1 set %2=0 where %3=%4").arg(tbl_base_fsub)
            .arg(fld_base_fsub_jddir).arg(fld_base_fsub_subsys).arg(subSys);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("update %1 set %2=1 where %3=%4 and %5=:code").arg(tbl_base_fsub)
            .arg(fld_base_fsub_jddir).arg(fld_base_fsub_subsys).arg(subSys)
            .arg(fld_base_fsub_subcode);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    foreach(QString code, codes){
        q.bindValue(":code",code);
        if(!q.exec())
            return false;
    }

    if(!db.commit()){
        db.rollback();
       return false;
    }
    return true;
}



/**
 * @brief AppConfig::getSubSysMaps2
 * @param scode     源科目系统代码
 * @param dcode     目的科目系统代码
 * @param defMaps   所有默认的对接科目配置项
 * @param multiMaps 混合对接科目的配置项（键为目的科目代码，值是源科目系统代码列表，它是多值哈希表）
 * @return
 */
bool AppConfig::getSubSysMaps2(int scode, int dcode, QHash<QString, QString> &defMaps, QHash<QString, QString> &multiMaps)
{
    QSqlQuery q(db);
    QString tname = QString("%1_%2_%3").arg(tbl_base_subsysjion_pre).arg(scode).arg(dcode);
    QString s = QString("select * from %1").arg(tname);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QString sc,dc;
    while(q.next()){
        sc = q.value(FI_BASE_SSJC_SCODE).toString();
        dc = q.value(FI_BASE_SSJC_DCODE).toString();
        bool isDef = q.value(FI_BASE_SSJC_ISDEF).toBool();
        if(isDef)
            defMaps[sc] = dc;
        else
            multiMaps.insertMulti(dc,sc);
    }
    if(!multiMaps.isEmpty()){
        foreach(QString d, multiMaps.keys()){
            QHashIterator<QString, QString> it(defMaps);
            bool fonded = false;
            while(it.hasNext()){
                it.next();
                if(it.value() == d){
                    fonded = true;
                    multiMaps.insertMulti(d,it.key());
                }
            }
            if(!fonded){
                LOG_ERROR(QObject::tr("科目对接配置错误，目的科目（%1）的默认对接不存在！").arg(d));
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 获取指定科目系统之间的科目对接配置信息
 * @param scode     旧科目系统代码
 * @param dcode     新科目系统代码
 * @param cfgs      源科目代码列表
 * @return
 */
bool AppConfig::getSubSysMaps(int scode, int dcode, QList<SubSysJoinItem2*>& cfgs)
{
    QSqlQuery q(db);
    QString tname = QString("%1_%2_%3").arg(tbl_base_subsysjion_pre).arg(scode).arg(dcode);
    QString s = QString("select * from %1 where %2 != '0000' order by %2")
            .arg(tname).arg(fld_base_ssjc_scode);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        SubSysJoinItem2* item = new SubSysJoinItem2;
        item->id = q.value(0).toInt();
        item->scode=q.value(FI_BASE_SSJC_SCODE).toString();
        item->dcode=q.value(FI_BASE_SSJC_DCODE).toString();
        item->isDef=q.value(FI_BASE_SSJC_ISDEF).toBool();
        cfgs<<item;
    }
    return true;
}

/**
 * @brief AppConfig::saveSubSysMaps
 * @param scode
 * @param dcode
 * @param codeMaps
 * @return
 */
bool AppConfig::saveSubSysMaps(int scode, int dcode, QList<SubSysJoinItem2 *> cfgs)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transform failed in AppConfig::saveSubSysMaps()");
        return false;
    }
    QString tname = QString("%1_%2_%3").arg(tbl_base_subsysjion_pre).arg(scode).arg(dcode);
    QString s;
    QSqlQuery q1(db),q2(db);
    s = QString("insert into %1(%2,%3,%4) values(:scode,:dcode,:isDef)").arg(tname)
            .arg(fld_base_ssjc_scode).arg(fld_base_ssjc_dcode).arg(fld_base_ssjc_isDef);
    if(!q1.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("update %1 set %2=:scode,%3=:dcode,%4=:isDef where id=:id").arg(tname)
            .arg(fld_base_ssjc_scode).arg(fld_base_ssjc_dcode).arg(fld_base_ssjc_isDef);
    if(!q2.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    for(int i=0; i<cfgs.count(); ++i){
        SubSysJoinItem2* item = cfgs.at(i);
        if(item->id==0){
            q1.bindValue(":scode",item->scode);
            q1.bindValue(":dcode",item->dcode);
            q1.bindValue(":isDef",item->isDef?1:0);
            if(!q1.exec()){
                LOG_SQLERROR(q1.lastQuery());
                return false;
            }
        }
        else{
            q2.bindValue(":id",item->id);
            q2.bindValue(":scode",item->scode);
            q2.bindValue(":dcode",item->dcode);
            q2.bindValue(":isDef",item->isDef?1:0);
            if(!q2.exec()){
                LOG_SQLERROR(q2.lastQuery());
                return false;
            }
        }
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transform failed in AppConfig::saveSubSysMaps()");
        if(!db.rollback())
            LOG_SQLERROR("Rollback transform failed in AppConfig::saveSubSysMaps()");
        return false;
    }
    return true;
}

/**
 * @brief 获取所有非默认（混合）对接科目的对接配置
 * @param scode
 * @param dcode
 * @param codeMaps  键为源一级科目代码，值为混合对接到目的一级科目代码
 * @return
 */
bool AppConfig::getNotDefSubSysMaps(int scode, int dcode, QHash<QString, QString> &codeMaps)
{
    QSqlQuery q(db);
    QString tname = QString("%1_%2_%3").arg(tbl_base_subsysjion_pre).arg(scode).arg(dcode);
    QString s = QString("select * from %1 where %2=0").arg(tname).arg(fld_base_ssjc_isDef);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QString sc,dc;
    QSet<QString> sets;
    while(q.next()){
        sc = q.value(FI_BASE_SSJC_SCODE).toString();
        dc = q.value(FI_BASE_SSJC_DCODE).toString();
        codeMaps[sc] = dc;
    }
    return true;
}

/**
 * @brief 获取科目系统对接配置是否完成
 * @param scode
 * @param dcode
 * @param ok
 * @return
 */
bool AppConfig::getSubSysMapConfiged(int scode, int dcode, bool &ok)
{
    QSqlQuery q(db);
    QString tname = QString("%1_%2_%3").arg(tbl_base_subsysjion_pre).arg(scode).arg(dcode);
    QString specCode = "0000";
    QString s = QString("select %1 from %2 where %3='%4' and %5='%4'").arg(fld_base_ssjc_isDef)
            .arg(tname).arg(fld_base_ssjc_scode).arg(specCode).arg(fld_base_ssjc_dcode);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        ok = false;
        return true;
    }
    ok = q.value(0).toBool();
    return true;
}

/**
 * @brief 设置科目系统对接配置是否完成
 * @param scode
 * @param dcode
 * @param ok
 * @return
 */
bool AppConfig::setSubSysMapConfiged(int scode, int dcode, bool ok)
{
    QSqlQuery q(db);
    QString tname = QString("%1_%2_%3").arg(tbl_base_subsysjion_pre).arg(scode).arg(dcode);
    QString specCode = "0000";
    QString s = QString("update %1 set %2=%3 where %4='%6' and %5='%6'")
            .arg(tname).arg(fld_base_ssjc_isDef).arg(ok?1:0)
            .arg(fld_base_ssjc_scode).arg(fld_base_ssjc_dcode).arg(specCode);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int num = q.numRowsAffected();
    if(num != 1){
        s = QString("insert into %1(%2,%3,%4) values('%5','%5','%6')")
                .arg(tname).arg(fld_base_ssjc_scode).arg(fld_base_ssjc_dcode)
                .arg(fld_base_ssjc_isDef).arg(specCode).arg(ok?1:0);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    return true;
}

/**
 * @brief 获取指定科目系统的科目代码到科目名称的哈希表
 * 这个主要是为了在导入新科目系统前查看或配置与老科目系统的对接配置信息
 * @param subSys
 * @param subNames
 * @return
 */
bool AppConfig::getSubCodeToNameHash(int subSys, QHash<QString, QString> &subNames)
{
    QSqlQuery q(db);
    QString s = QString("select %1,%2 from %3 where %4=%5")
            .arg(fld_base_fsub_subcode).arg(fld_base_fsub_subname)
            .arg(tbl_base_fsub).arg(fld_base_fsub_subsys).arg(subSys);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        QString code = q.value(0).toString();
        QString name = q.value(1).toString();
        subNames[code]=name;
    }
    return true;
}

/**
 * @brief 读取所有配置的外部工具配置项
 * @param items
 */
bool AppConfig::readAllExternalTools(QList<ExternalToolCfgItem *> &items)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_base_external_tools);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        ExternalToolCfgItem* item = new ExternalToolCfgItem;
        item->id = q.value(0).toInt();
        item->name = q.value(FI_BASE_ET_NAME).toString();
        item->commandLine = q.value(FI_BASE_ET_COMMANDLINE).toString();
        item->parameter = q.value(FI_BASE_ET_PARAMETER).toString();
        items<<item;
    }
    return true;
}

/**
 * @brief 保存配置的外部工具配置项
 * @param item
 */
bool AppConfig::saveExternalTool(ExternalToolCfgItem *item, bool isDelete)
{
    QSqlQuery q(db);
    QString s;
    if(isDelete){
        s = QString("delete from %1 where id=%2").arg(tbl_base_external_tools).arg(item->id);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    else{
        if(item->id==0)
            s = QString("insert into %1(%2,%3,%4) values('%5','%6','%7')").arg(tbl_base_external_tools)
                    .arg(fld_base_et_name).arg(fld_base_et_commandline).arg(fld_base_et_parameter)
                    .arg(item->name).arg(item->commandLine).arg(item->parameter);
        else
            s = QString("update %1 set %2='%5',%3='%6',%4='%7' where id=%8").arg(tbl_base_external_tools)
                    .arg(fld_base_et_name).arg(fld_base_et_commandline).arg(fld_base_et_parameter)
                    .arg(item->name).arg(item->commandLine).arg(item->parameter).arg(item->id);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(item->id == 0){
            q.exec("select last_insert_rowid()");
            q.first();
            item->id = q.value(0).toInt();
        }
    }
    return true;
}

bool AppConfig::initRights(QHash<int, Right *> &rights)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_base_rights);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        int code = q.value(FI_BASE_R_CODE).toInt();
        QString name = q.value(FI_BASE_R_NAME).toString();
        int rtCode = q.value(FI_BASE_R_TYPE).toInt();
        RightType* type = allRightTypes.value(rtCode);
        if(!type)
            LOG_ERROR(QString("Fonded a invalid Right Type code(rt_code=%1,rcode=%2)").arg(rtCode).arg(code));
        QString explain = q.value(FI_BASE_R_EXPLAIN).toString();
        Right* right = new Right(code,type,name,explain);
        rights[code] = right;
    }
    return true;
}

bool AppConfig::initUsers(QHash<int, User *> &users)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_base_users);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        int id = q.value(0).toInt();
        bool isEnabled = q.value(FI_BASE_U_ISENABLED).toBool();
        QString name = q.value(FI_BASE_U_NAME).toString();
        QString pw = q.value(FI_BASE_U_PASSWORD).toString();
        pw = User::encryptPw(pw);
        QString gs = q.value(FI_BASE_U_GROUPS).toString();      //所属组
        QSet<UserGroup*> groups;
        if(!gs.isEmpty()){
            QStringList gr = gs.split(",");            
            for(int i = 0; i < gr.count(); ++i){
                UserGroup* g = allGroups.value(gr[i].toInt());
                if(g)
                    groups.insert(g);
            }            
        }
        User* user = new User(id, name, pw, groups);
        if(!isEnabled)
            user->setEnabled(false);
        users[id] = user;
        gs = q.value(FI_BASE_U_ACCOUNTS).toString();        //专属账户
        if(!gs.isEmpty()){
            foreach(QString code, gs.split(",")){
                if(!code.isEmpty())
                    user->addExclusiveAccount(code);
            }
        }        
        gs = q.value(FI_BASE_U_DISABLED_RIGHTS).toString();  //禁用权限
        if(!gs.isEmpty()){
            foreach(QString code, gs.split(",")){
                bool ok = false;
                int c = code.toInt(&ok);
                Right* r = allRights.value(c);
                if(!ok || !r){
                    LOG_ERROR(QString("User(uid=%1) disabled right code(rid=%2) is invalid")
                              .arg(id).arg(code));
                    continue;
                }
                user->addDisRight(r);
            }
        }
    }
    return true;
}

bool AppConfig::saveUser(User *u, bool isDelete)
{
    QSqlQuery q(db);
    QString s;
    if(isDelete){
        s = QString("delete from %1 where id=%2").arg(tbl_base_users).arg(u->getUserId());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        return true;
    }
    if(u->getUserId() != UNID){
        s = QString("update %1 set %2='%8',%3='%9',%4='%10',%5='%11',%6='%12',%7=%13 where id=%14")
                .arg(tbl_base_users).arg(fld_base_u_name).arg(fld_base_u_password)
                .arg(fld_base_u_groups).arg(fld_base_u_accounts).arg(fld_base_u_disabled_rights)
                .arg(fld_base_u_isenabled).arg(u->getName()).arg(User::decryptPw(u->getPassword()))
                .arg(u->getOwnerGroupCodeList()).arg(u->getExclusiveAccounts().join(",")).arg(u->getDisRightCodes())
                .arg(u->isEnabled()?1:0).arg(u->getUserId());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(q.numRowsAffected() == 0){
            s = QString("insert into %1(id,%2,%3,%4,%5,%6,%7) values(%14,'%8','%9','%10','%11','%12',%13)")
                    .arg(tbl_base_users).arg(fld_base_u_name).arg(fld_base_u_password)
                    .arg(fld_base_u_groups).arg(fld_base_u_accounts).arg(fld_base_u_disabled_rights)
                    .arg(fld_base_u_isenabled).arg(u->getName()).arg(User::decryptPw(u->getPassword()))
                    .arg(u->getOwnerGroupCodeList()).arg(u->getExclusiveAccounts().join(","))
                    .arg(u->getDisRightCodes()).arg(u->isEnabled()?1:0).arg(u->getUserId());
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            allUsers[u->getUserId()] = u;
        }
    }
    else{
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values('%8','%9','%10','%11','%12',%13)")
                .arg(tbl_base_users).arg(fld_base_u_name).arg(fld_base_u_password)
                .arg(fld_base_u_groups).arg(fld_base_u_accounts).arg(fld_base_u_disabled_rights)
                .arg(fld_base_u_isenabled).arg(u->getName()).arg(User::decryptPw(u->getPassword()))
                .arg(u->getOwnerGroupCodeList()).arg(u->getExclusiveAccounts().join(","))
                .arg(u->getDisRightCodes()).arg(u->isEnabled()?1:0);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.exec("select last_insert_rowid()");
        q.first();
        int id = q.value(0).toInt();
        u->id = id;
        allUsers[id] = u;
    }
    return true;
}

/**
 * @brief 从数据库中恢复用户对象
 * @param u
 * @return
 */
bool AppConfig::restorUser(User *u)
{
    if(!u)
        return false;
    QSqlQuery q(db);
    QString s = QString("select * from %1 where id=%2").arg(tbl_base_users).arg(u->getUserId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first())
        return false;
    QString name = q.value(FI_BASE_U_NAME).toString();
    u->setName(name);
    QString pw = q.value(FI_BASE_U_PASSWORD).toString();
    pw = User::decryptPw(pw);
    u->setPassword(pw);
    QString gs = q.value(FI_BASE_U_GROUPS).toString();
    QSet<UserGroup*> groups;
    if(!gs.isEmpty()){
        QStringList gr = gs.split(",");
        for(int i = 0; i < gr.count(); ++i){
            UserGroup* g = allGroups.value(gr[i].toInt());
            if(g)
                groups.insert(g);
        }
    }
    u->setOwnerGroups(groups);
    gs = q.value(FI_BASE_U_ACCOUNTS).toString();
    if(!gs.isEmpty()){
        foreach(QString code, gs.split(",")){
            if(!code.isEmpty())
                u->addExclusiveAccount(code);
        }
    }
    gs = q.value(FI_BASE_U_DISABLED_RIGHTS).toString();
    u->clearDisRights();
    if(!gs.isEmpty()){
        foreach(QString code, gs.split(",")){
            Right* r = allRights.value(code.toInt());
            if(r)
                u->addDisRight(r);
        }
    }
    return true;
}

bool AppConfig::initRightTypes(QHash<int, RightType*> &types)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2")
            .arg(tbl_base_righttypes).arg(fld_base_rt_pcode);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        bool ok = false;
        int rowId = q.value(0).toInt();
        int c = q.value(FI_BASE_RT_CODE).toInt(&ok);
        if(!ok)
            LOG_ERROR(QString("Fonded a invalid right type code in rightTypes table(rowId=%1)!").arg(rowId));
        int pc = q.value(FI_BASE_RT_PCODE).toInt(&ok);
        RightType* p = types.value(pc);
        if(!ok || ok && !p && pc!=0)
            LOG_ERROR(QString("Fonded a invalid parent right type code (in rightTypes table(rowId=%1)!").arg(rowId));
        RightType* t = new RightType;
        t->code = c; t->pType = p;
        t->name = q.value(FI_BASE_RT_NAME).toString();
        t->explain = q.value(FI_BASE_RT_EXPLAIN).toString();
        types[c] = t;
    }
    return true;
}

bool AppConfig::initUserGroups(QHash<int, UserGroup *> &groups)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_base_usergroups);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QHash<int,QList<int> > gcs;  //临时保存组的所属组代码，待所有组对象初始化完成后再添加所属组
    while(q.next()){
        int id = q.value(0).toInt();
        int code = q.value(FI_BASE_G_CODE).toInt();
        QString name = q.value(FI_BASE_G_NAME).toString();
        QString explain = q.value(FI_BASE_G_EXPLAIN).toString();
        QString rs = q.value(FI_BASE_G_RIGHTS).toString();
        QString gs = q.value(FI_BASE_G_GROUPS).toString();
        QSet<Right*> haveRights;
        if(!rs.isEmpty()){
            QStringList rl = rs.split(",");            
            for(int i = 0; i < rl.count(); ++i){
                Right* r = allRights.value(rl[i].toInt());
                if(!r)
                    LOG_ERROR(QString("Fonded a not exist right(right code=%1,group code=%2)")
                              .arg(rl[i].toInt()).arg(code));
                else
                    haveRights.insert(r);
            }            
        }
        if(!gs.isEmpty()){
            QStringList gl = gs.split(",");
            gcs[code] = QList<int>();
            for(int i = 0; i < gl.count(); ++i){
                gcs[code]<<gl[i].toInt();
            }
        }
        UserGroup* group = new UserGroup(id,code, name, haveRights);
        group->setExplain(explain);
        groups[code] = group;
    }
    //初始化组的所属组成员
    foreach(UserGroup* g, groups.values()){
        foreach(int gc, gcs.value(g->getGroupCode())){
            UserGroup* og = groups.value(gc);
            if(!og){
                LOG_ERROR(QString("Fonded a not exist group code(code=%1,groupCode=%2)")
                          .arg(gc).arg(g->getGroupCode()));
                return false;
            }
            g->addGroup(og);
        }
    }
    return true;
}

bool AppConfig::saveUserGroup(UserGroup *g, bool isDelete)
{
    QSqlQuery q(db);
    QString s;
    if(isDelete){
        s = QString("delete from %1 where %2=%3").arg(tbl_base_usergroups)
                .arg(fld_base_g_code).arg(g->getGroupCode());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        allGroups.remove(g->getGroupCode());
        delete g;
        return true;
    }
    s = QString("update %1 set %3='%7',%4='%8',%5='%9',%6='%10' where %2=%11")
            .arg(tbl_base_usergroups).arg(fld_base_g_code).arg(fld_base_g_name)
            .arg(fld_base_g_rights).arg(fld_base_g_groups).arg(fld_base_g_explain)
            .arg(g->getName()).arg(g->getRightCodeList()).arg(g->getOwnerCodeList())
            .arg(g->getExplain()).arg(g->getGroupCode());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int nums = q.numRowsAffected();
    if(nums == 0){
        s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,'%8','%9','%10','%11')").arg(tbl_base_usergroups)
                .arg(fld_base_g_code).arg(fld_base_g_name).arg(fld_base_g_explain)
                .arg(fld_base_g_rights).arg(fld_base_g_groups).arg(g->getGroupCode())
                .arg(g->getName()).arg(g->getExplain()).arg(g->getRightCodeList())
                .arg(g->getOwnerCodeList());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        allGroups[g->getGroupCode()] = g;
    }
    return true;
}

/**
 * @brief 从数据库中恢复用户组对象
 * @param g
 * @return
 */
bool AppConfig::restoreUserGroup(UserGroup *g)
{
    if(!g)
        return false;
    QSqlQuery q(db);
    QString s = QString("select * from %1 where id=%2").arg(tbl_base_usergroups)
            .arg(g->getGroupId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first())
        return false;
    int code = q.value(FI_BASE_G_CODE).toInt();
    QString name = q.value(FI_BASE_G_NAME).toString();
    g->setName(name);
    QString explain = q.value(FI_BASE_G_EXPLAIN).toString();
    g->setExplain(explain);
    QString rs = q.value(FI_BASE_G_RIGHTS).toString();
    QSet<Right*> haveRights;
    if(!rs.isEmpty()){
        QStringList rl = rs.split(",");
        for(int i = 0; i < rl.count(); ++i){
            Right* r = allRights.value(rl[i].toInt());
            if(!r)
                LOG_ERROR(QString("Fonded a not exist right(right code=%1,group code=%2)")
                          .arg(rl[i].toInt()).arg(code));
            else
                haveRights.insert(r);
        }
    }
    g->setHaveRights(haveRights);
    rs = q.value(FI_BASE_G_GROUPS).toString();
    QSet<UserGroup*> gs;
    if(!rs.isEmpty()){
        QStringList sl = rs.split(",");
        for(int i = 0; i < sl.count(); ++i){
            UserGroup* g1 = allGroups.value(sl.at(i).toInt());
            if(!g1)
                LOG_SQLERROR(QString("Fonded a not exist owner group(group code=%1,owner group code=%2")
                             .arg(g->getGroupCode()).arg(sl.at(i)));
            else
                gs.insert(g1);
        }
        g->setOwnerGroups(gs);
    }
    return true;
}

/**
 * @brief 返回应用配置类型名称
 * @return
 */
void AppConfig::getAppCfgTypeNames(QHash<BaseDbVersionEnum, QString> &names)
{
    names[BDVE_DB] = "bdb_version";
    names[BDVE_RIGHTTYPE] = "RightType";
    names[BDVE_RIGHT] = "Right";
    names[BDVE_GROUP] = "Group";
    names[BDVE_USER] = "User";
    names[BDVE_WORKSTATION] = "WorkStation";
}

/**
 * @brief 设置指定应用配置的版本号
 * @param verType
 * @param mv
 * @param sv
 * @return
 */
bool AppConfig::setAppCfgVersion(BaseDbVersionEnum verType, int mv, int sv)
{
    QSqlQuery q(db);
    QString s = QString("update %7 set %1=%2,%3=%4 where %5=%6").arg(fld_base_version_master)
            .arg(mv).arg(fld_base_version_second).arg(sv).
            arg(fld_base_version_typeEnum).arg((int)verType).arg(tbl_base_version);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.numRowsAffected() != 1){
        QHash<BaseDbVersionEnum, QString> names;
        s = QString("insert into %9(%1,%2,%3,%4) values(%5,%6,%7,%8)")
                .arg(fld_base_version_typeEnum).arg(fld_base_version_typeName)
                .arg(fld_base_version_master).arg(fld_base_version_second)
                .arg(tbl_base_version).arg(verType).arg(names.value(verType))
                .arg(mv).arg(sv);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    return true;
}

/**
 * @brief 获取基本库内可转移的应用配置的版本号
 * 这些配置信息可以通过转换转移操作在各个工作站之间进行转移
 * @param vType
 * @return
 */
bool AppConfig::getAppCfgVersion(int &mv, int &sv, BaseDbVersionEnum vType)
{
    QSqlQuery q(db);
    QString s = QString("select %1,%2 from %3 where %4=%5")
            .arg(fld_base_version_master).arg(fld_base_version_second).arg(tbl_base_version)
            .arg(fld_base_version_typeEnum).arg(vType);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        mv = 1; sv = 0;
        return false;
    }
    if(!q.first()){
        mv = 1; sv = 0;
    }
    else{
        mv = q.value(0).toInt();
        sv = q.value(1).toInt();
    }
    return true;
}

/**
 * @brief 获取指定的可转移的应用配置的版本信息（包括版本类型、版本号、版本名等）
 * @param verTypes  指定哪些应用配置版本信息（输入参数）
 * @param verNames  版本名称（下面都是输出参数）
 * @param mvs       主版本号
 * @param svs       次版本号
 * @return
 */
bool AppConfig::getAppCfgVersions(QList<BaseDbVersionEnum> &verTypes, QStringList &verNames, QList<int> &mvs, QList<int> &svs)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2").arg(tbl_base_version).arg(fld_base_version_typeEnum);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        BaseDbVersionEnum verType = (BaseDbVersionEnum)q.value(FI_BASE_VER_TYPEENUM).toInt();
        if(!verTypes.contains(verType))
            continue;
        verNames<<q.value(FI_BASE_VER_TYPENAME).toString();
        mvs<<q.value(FI_BASE_VER_MASTER).toInt();
        svs<<q.value(FI_BASE_VER_SECOND).toInt();
    }
    return true;
}

bool AppConfig::clearAndSaveUsers(QList<User *> users, int mv, int sv)
{
    QSqlQuery q(db);
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on clear and save users");
        return false;
    }
    QString s = QString("delete from %1").arg(tbl_base_users);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    foreach(User* u, users){
        if(!saveUser(u)){
            db.rollback();
            return false;
        }
    }
    if(!setAppCfgVersion(BDVE_USER,mv,sv)){
        db.rollback();
        return false;
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on clear and save users");
        return false;
    }
    return true;
}

bool AppConfig::clearAndSaveGroups(QList<UserGroup *> groups,int mv,int sv)
{
    QSqlQuery q(db);
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on clear and save groups");
        return false;
    }
    QString s = QString("delete from %1").arg(tbl_base_usergroups);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    foreach(UserGroup* g, groups){
        if(!saveUserGroup(g)){
            db.rollback();
            return false;
        }
    }
    if(!setAppCfgVersion(BDVE_GROUP,mv,sv)){
        db.rollback();
        return false;
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on clear and save groups");
        return false;
    }
    return true;
}

bool AppConfig::clearAndSaveMacs(QList<WorkStation *> macs,int mv, int sv)
{
    QSqlQuery q(db);
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on clear and save Workstations");
        return false;
    }
    QString s = QString("delete from %1").arg(tbl_machines);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        db.rollback();
        return false;
    }
    foreach(WorkStation* mac, macs){
        if(!_saveMachine(mac)){
            db.rollback();
            return false;
        }
    }
    if(!setAppCfgVersion(BDVE_WORKSTATION,mv,sv)){
        db.rollback();
        return false;
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on clear and save Workstations");
        return false;
    }
    return true;
}

bool AppConfig::clearAndSaveRights(QList<Right *> rights,int mv, int sv)
{
    QSqlQuery q(db);
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on clear and save rights");
        return false;
    }
    QString s = QString("delete from %1").arg(tbl_base_rights);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5) values(:c,:t,:name,:desc)")
            .arg(tbl_base_rights).arg(fld_base_r_code).arg(fld_base_r_type)
            .arg(fld_base_r_name).arg(fld_base_r_explain);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        db.rollback();
        return false;
    }
    foreach(Right* r, rights){
        q.bindValue(":c",r->getCode());
        q.bindValue(":t",r->getType()->code);
        q.bindValue(":name",r->getName());
        q.bindValue(":desc",r->getExplain());
        if(!q.exec()){
            db.rollback();
            return false;
        }
    }
    if(!setAppCfgVersion(BDVE_RIGHT,mv,sv)){
        db.rollback();
        return false;
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on clear and save groups");
        return false;
    }
    return true;
}

bool AppConfig::clearAndSaveRightTypes(QList<RightType *> rightTypes, int mv, int sv)
{
    QSqlQuery q(db);
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on clear and save rightTypes");
        return false;
    }
    QString s = QString("delete from %1").arg(tbl_base_righttypes);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5) values(:p,:c,:name,:desc)").arg(tbl_base_righttypes)
            .arg(fld_base_rt_pcode).arg(fld_base_rt_code).arg(fld_base_rt_name)
            .arg(fld_base_rt_explain);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        db.rollback();
        return false;
    }
    foreach(RightType* rt, rightTypes){
        q.bindValue(":p",rt->pType?rt->pType->code:0);
        q.bindValue(":c",rt->code);
        q.bindValue(":name",rt->name);
        q.bindValue(":desc",rt->explain);
        if(!q.exec()){
            db.rollback();
            return false;
        }
    }
    if(!setAppCfgVersion(BDVE_RIGHTTYPE,mv,sv)){
        db.rollback();
        return false;
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on clear and save rightTypes");
        return false;
    }
    return true;
}

/**
 * @brief 解析版本号
 * 版本字符串形如：“version=1.0”
 * @param text
 * @param mv
 * @param sv
 * @return
 */
bool AppConfig::parseVersionFromText(QString text, int &mv, int &sv)
{
    QStringList sl = text.split("=");
    if(sl.count() != 2)
        return false;
    if(sl.at(0) != "version")
        return false;
    sl = sl.at(1).split(".");
    if(sl.count() != 2)
        return false;
    bool ok;
    mv = sl.at(0).toInt(&ok);
    if(!ok)
        return false;
    sv = sl.at(1).toInt(&ok);
    if(!ok)
        return false;
    return true;
}

bool AppConfig::savePhases(QList<CommonPromptPhraseClass> cs, QList<int> nums, QStringList ps)
{
    QSqlQuery q(db);
    if(!db.transaction()){
        LOG_SQLERROR("Start Transaction failed on save common prompt phrase!");
        return false;
    }

    QString s = QString("insert into %1(%2,%3,%4) values(:c,:n,:p)").arg(tbl_base_commonPromptPhrase)
            .arg(fld_base_cpp_class).arg(fld_base_cpp_number).arg(fld_base_cpp_phrase);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    for(int i = 0; i < cs.count(); ++i){
        q.bindValue(":c",cs.at(i));
        q.bindValue(":n",nums.at(i));
        q.bindValue(":p",ps.at(i));
        if(!q.exec()){
            LOG_SQLERROR(q.lastQuery());
            return false;
        }
    }

    if(!db.commit()){
        LOG_SQLERROR("Commit Transaction failed on save common prompt phrase!");
        return false;
    }
    return true;
}

/**
 * @brief 读取指定类别的常用提示短语
 * @param pClass
 * @param phrases
 * @return
 */
bool AppConfig::readPhases(CommonPromptPhraseClass pClass, QHash<int, QString>& phrases)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2=%3 order by %4").arg(tbl_base_commonPromptPhrase)
            .arg(fld_base_cpp_class).arg(pClass).arg(fld_base_cpp_number);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        int num = q.value(FI_BASE_CPP_NUMBER).toInt();
        phrases[num] = q.value(FI_BASE_CPP_PHRASE).toString();
    }
    return true;
}

/**
 * @brief 序列化常用提示短语配置到字节数组
 * @param ba
 * @return
 */
bool AppConfig::serialCommonPhraseToBinary(QByteArray *ba)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2,%3").arg(tbl_base_commonPromptPhrase)
            .arg(fld_base_cpp_class).arg(fld_base_cpp_number);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int mv,sv;
    getAppCfgVersion(mv,sv,BDVE_COMMONPHRASE);
    QBuffer bf(ba);
    QTextStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    out<<QString("version=%1.%2\n").arg(mv).arg(sv);
    while(q.next()){
        out<<q.value(FI_BASE_CPP_CLASS).toInt()<<"||";
        out<<q.value(FI_BASE_CPP_NUMBER).toInt()<<"||";
        out<<q.value(FI_BASE_CPP_PHRASE).toString()<<"\n";
    }
    bf.close();
    return true;
}

/**
 * @brief 从字节数组中恢复常用短语配置信息
 * @param ba
 * @return
 */
bool AppConfig::serialCommonPhraseFromBinary(QByteArray *ba)
{
    QBuffer bf(ba);
    QTextStream in(&bf);
    bf.open(QIODevice::ReadOnly);
    int mv,sv;
    if(!parseVersionFromText(in.readLine(),mv,sv))
        return false;
    QList<CommonPromptPhraseClass> cs;
    QList<int> ns; QStringList ps;
    while(!in.atEnd()){
        QString s = in.readLine();
        QStringList sl = s.split("||");
        if(sl.count() != 3){
            LOG_ERROR(QString("Common phrase format error! text is '%1'").arg(s));
            return false;
        }
        bool ok = true, ok1;
        cs<<(CommonPromptPhraseClass)sl.at(0).toInt(&ok1);
        ok &= ok1;
        ns<<sl.at(1).toInt(&ok1);
        ok &= ok1;
        ps<<sl.at(2);
        if(!ok){
            LOG_ERROR(QString("Common phrase format error! text is '%1'").arg(s));
            return false;
        }
    }
    QSqlQuery q(db);
    QString s = QString("delete from %1").arg(tbl_base_commonPromptPhrase);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!savePhases(cs,ns,ps))
        return false;
    if(!setAppCfgVersion(BDVE_COMMONPHRASE,mv,sv))
        return false;
    bf.close();
    return true;
}

/**
 * @brief 返回表示指定科目系统的指定代码的一级科目对象
 * 该对象不能实际使用于账户中，仅用于临时使用（比如查看科目系统配置信息窗口使用）
 * @param subSysCode
 * @param subCode
 * @return
 */
FirstSubject *AppConfig::getFirstSubject(int subSysCode, QString subCode)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2=%3 and %4=%5").arg(tbl_base_fsub)
            .arg(fld_base_fsub_subsys).arg(subSysCode).arg(fld_base_fsub_subcode)
            .arg(subCode);
    if(!q.exec(s) || !q.first())
        return 0;
    QString name = q.value(FI_BASE_FSUB_SUBNAME).toString();
    QString remCode = q.value(FI_BASE_FSUB_REMCODE).toString();
    SubjectClass cls = (SubjectClass)q.value(FI_BASE_FSUB_CLS).toInt();
    bool jdDir = q.value(FI_BASE_FSUB_JDDIR).toBool();
    return new FirstSubject(0,0,cls,name,subCode,remCode,0,true,jdDir,false,"","",subSysCode);
}

/**
 * @brief 返回是否开启智能子目设置功能
 * @return
 */
bool AppConfig::isOnSmartSSubSet()
{
    appIni->beginGroup(SEGMENT_SMARTSSUB);
    bool on = appIni->value("on").toBool();
    appIni->endGroup();
    return on;
}

/**
 * @brief 返回与指定科目相关的智能子目设置相关的设置信息
 * @param subCode
 * @param witch
 * @return
 */
QString AppConfig::getSmartSSubFix(QString subCode, AppConfig::SmartSSubFix witch)
{
    appIni->beginGroup(SEGMENT_SMARTSSUB);
    QString ws;
    switch(witch){
    case SSF_PREFIXE:
        ws = KEY_SMART_PREFIXE;
        break;
    case SSF_SUFFIXE:
        ws = KEY_SMART_SUFFIXE;
        break;
    }
    QString key = QString("%1_%2").arg(ws).arg(subCode);
    QString str = appIni->value(key).toString();
    appIni->endGroup();
    return str;
}

/**
 * @brief 获取是否自动隐藏左侧停靠面板（帐套切换）的设置信息
 * @return
 */
bool AppConfig::isAutoHideLeftDock()
{
    appIni->beginGroup(SEGMENT_USER_INTFACE);
    bool r = appIni->value(KEY_INTERFACE_AUTOHIDELEFTPANEL,false).toBool();
    appIni->endGroup();
    return r;
}

/**
 * @brief 设置是否自动隐藏左侧停靠面板（帐套切换）的设置信息
 * @param on
 * @return
 */
void AppConfig::setAutoHideLeftDock(bool on)
{
    appIni->beginGroup(SEGMENT_USER_INTFACE);
    appIni->setValue(KEY_INTERFACE_AUTOHIDELEFTPANEL,on);
    appIni->endGroup();
}

/**
 * @brief 关闭主窗口时是否最小化到系统托盘区
 * @return
 */
bool AppConfig::minToTrayClose()
{
    appIni->beginGroup(SEGMENT_USER_INTFACE);
    bool r = appIni->value(KEY_INTERFACE_MIN_TO_TRAY,false).toBool();
    appIni->endGroup();
    return r;
}

void AppConfig::setMinToTrayClose(bool on)
{
    appIni->beginGroup(SEGMENT_USER_INTFACE);
    appIni->setValue(KEY_INTERFACE_MIN_TO_TRAY,on);
    appIni->endGroup();
}

/**
 * @brief AppConfig::_isValidAccountCode
 *  判断账户代码是否有效
 *  账户代码必须由大于1000的四位数组成，且不能重复
 * @param code
 * @return
 */
bool AppConfig::_isValidAccountCode(QString code)
{
    if(code.count() != 4)
        return false;
    bool ok = false;
    int v = code.toInt(&ok);
    if(!ok || v < 1000)
        return false;
//    foreach(AccountCacheItem* item, accountCaches){
//        if(item->code == code)
//            return false;
//    }
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
        s = QString("update %1 set %2='%3',%4='%5',%6='%7',%8=%9,%10=%11,%12='%13',%14=%15,%16='%17',%18=%19 where id=%20")
                .arg(tbl_localAccountCache).arg(fld_lac_name).arg(accInfo->accName)
                .arg(fld_lac_lname).arg(accInfo->accLName).arg(fld_lac_filename)
                .arg(accInfo->fileName).arg(fld_lac_isLastOpen).arg(accInfo->lastOpened?1:0)
                .arg(fld_lac_tranState).arg(accInfo->tState).arg(fld_lac_tranInTime)
                .arg(accInfo->inTime.toString(Qt::ISODate)).arg(fld_lac_tranSrcMid)
                .arg(accInfo->s_ws->getMID()).arg(fld_lac_tranOutTime)
                .arg(accInfo->outTime.toString(Qt::ISODate)).arg(fld_lac_tranDesMid)
                .arg(accInfo->d_ws->getMID()).arg(accInfo->id);

    }
    else{
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10,%11) values('%12','%13','%14','%15',0,%16,'%17',%18,'%19',%20)")
                .arg(tbl_localAccountCache).arg(fld_lac_code).arg(fld_lac_name)
                .arg(fld_lac_lname).arg(fld_lac_filename).arg(fld_lac_isLastOpen)
                .arg(fld_lac_tranState).arg(fld_lac_tranInTime).arg(fld_lac_tranSrcMid)
                .arg(fld_lac_tranOutTime).arg(fld_lac_tranDesMid).arg(accInfo->code).arg(accInfo->accName)
                .arg(accInfo->accLName).arg(accInfo->fileName)/*.arg(accInfo->lastOpened)*/
                .arg(accInfo->tState).arg(accInfo->inTime.toString(Qt::ISODate))
                .arg(accInfo->s_ws->getMID()).arg(accInfo->outTime.toString(Qt::ISODate))
                .arg(accInfo->d_ws->getMID());

    }
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(isNew){
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
    WorkStation* locMac = getLocalStation();
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
            accItem->s_ws = getLocalStation();
            if(!accItem->s_ws){
                myHelper::ShowMessageBoxWarning(QObject::tr("账户文件“%1”不存在转移记录，且未配置本站信息，无法初始化转移记录，忽略此账户！"));
                delete accItem;
                continue;
            }
            accItem->outTime = QDateTime::currentDateTime();
            accItem->inTime = QDateTime::currentDateTime();
            accItem->tState = ATS_TRANSINDES;
        }
        else{
            accItem->tState = (AccountTransferState)q.value(TRANS_STATE).toInt();
            accItem->s_ws = machines.value(q.value(TRANS_SMID).toInt());
            accItem->d_ws = machines.value(q.value(TRANS_DMID).toInt());
            if(!accItem->s_ws){
                myHelper::ShowMessageBoxWarning(QObject::tr("账户文件“%1”来源站不明，忽略！").arg(accItem->fileName));
                delete accItem;
                continue;
            }
            if(!accItem->d_ws){
                myHelper::ShowMessageBoxWarning(QObject::tr("账户文件“%1”去向站不明，忽略！").arg(accItem->fileName));
                delete accItem;
                continue;
            }            
            accItem->outTime = QDateTime::fromString(q.value(TRANS_OUTTIME).toString(),Qt::ISODate);
            accItem->inTime = QDateTime::fromString(q.value(TRANS_INTIME).toString(),Qt::ISODate);
            if(accItem->tState == ATS_TRANSINDES && accItem->d_ws != locMac){
                accItem->tState = ATS_TRANSINOTHER;
            }
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
 * @brief 初始化应用程序配置变量的默认值
 */
void AppConfig::_initCfgVarDefs()
{
    boolCfgs[CVC_RuntimeUpdateExtra]=true;
    boolCfgs[CVC_ExtraUnityInspectAfterRead] = true;
    boolCfgs[CVC_ExtraUnityInspectBeforeSave] = true;
    boolCfgs[CVC_ViewHideColInDailyAcc1] = false;
    boolCfgs[CVC_ViewHideColInDailyAcc2] = false;
    boolCfgs[CVC_IsByMtForOppoBa] = false;
    boolCfgs[CVC_IsCollapseJz] = true;
    boolCfgs[CVC_JzlrByYear] = true;
    intCfgs[CVC_ResentLoginUser] = 1;
    intCfgs[CVC_AutoSaveInterval] = 600000;
    intCfgs[CVC_TimeoutOfTemInfo] = 10000;
    doubleCfgs[CVC_GdzcCzRate] = 0;
}

/**
 * @brief AppConfig::_initAccountCaches
 *  读取本地账户缓存表，并初始化accountCaches列表
 * @return
 */
bool AppConfig::_initAccountCaches()
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2").arg(tbl_localAccountCache)
            .arg(fld_lac_code);
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
        item->s_ws = machines.value(q.value(LAC_SMAC).toInt());
        item->d_ws = machines.value(q.value(LAC_DMAC).toInt());
        item->outTime = q.value(LAC_OUTTIME).toDateTime();
        item->inTime = q.value(LAC_INTIME).toDateTime();
        item->tState = (AccountTransferState)q.value(LAC_TSTATE).toInt();
        accountCaches<<item;
    }
    init_accCache = true;
    return true;
}

/**
 * @brief AppConfig::_initMachines
 *  读取主机条目，并初始化machines表
 */
bool AppConfig::_initMachines()
{
    //读取主站标识
    appIni->beginGroup(SEGMENT_STATIONS);
    msId = appIni->value(KEY_STATION_MSID,101).toInt();
    localId = appIni->value(KEY_STATION_LOID,101).toInt();
    appIni->endGroup();

    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_machines);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        QMessageBox::critical(0,QObject::tr("出错信息"),QObject::tr("无法读取机器信息，请检查账户数据库相应表格是否有误！"));
        return false;
    }
    machines.clear();
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
        int osType = q.value(MACS_OSTYPE).toInt();
        WorkStation* m = new WorkStation(id,mtype,mid,isLocal,name,desc,osType);
        machines[m->getMID()] = m;
    }
    if(!machines.contains(localId)){
        LOG_ERROR(QString("Station Exception! Workstation list not contain (local ID: %1)").arg(localId));
        return false;
    }
    else if(!machines.value(localId)->isLocalStation()){
        foreach(WorkStation*m, machines){
            if(m->getMID() == localId)
                m->setLocalMachine(true);
            else
                m->setLocalMachine(false);
        }
    }
    return true;
}

/**
 * @brief 初始化科目系统哈希表
 */
bool AppConfig::_initSubSysNames()
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2").arg(tbl_subSys).arg(fld_ss_code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QList<int> codes;
    while(q.next()){
        SubSysNameItem* si = new SubSysNameItem;
        si->code = q.value(SS_CODE).toInt();
        codes<<si->code;
        si->name = q.value(SS_NAME).toString();
        si->startTime = q.value(SS_TIME).toDate();
        si->explain = q.value(SS_EXPLAIN).toString();
        si->isImport = false;
        si->isConfiged = false;
        subSysNames[si->code] = si;
    }
    if(codes.count()>1){
        for(int i = 1; i< codes.count(); ++i){
            SubSysNameItem *preItem, *curItem;
            preItem = subSysNames.value(codes.at(i-1));
            curItem = subSysNames.value(codes.at(i));
            getSubSysMapConfiged(preItem->code,curItem->code,curItem->isConfiged);
        }
    }
    return true;
}

/**
 * @brief 初始化特定科目代码表
 */
bool AppConfig::_initSpecSubCodes()
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_base_sscc);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        QMessageBox::critical(0,QObject::tr("出错信息"),QObject::tr("无法读取特定科目配置代码表，请检查基本库相应表格是否有误！"));
        return false;
    }
    int subSys;
    SpecSubCode subEnum;
    QString code;
    while(q.next()){
        subSys = q.value(FI_BASE_SSCC_SUBSYS).toInt();
        subEnum = (SpecSubCode)q.value(FI_BASE_SSCC_ENUM).toInt();
        code = q.value(FI_BASE_SSCC_CODE).toString();
        specCodes[subSys][subEnum] = code;
    }
    return true;
}

/**
 * @brief 初始化特定名称类别代码表
 */
bool AppConfig::_initSpecNameItemClses()
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
        return false;
    }
    q.first();
    int maxCode = q.value(0).toInt();

    s = QString("select %1 from %2 where %3=:name")
            .arg(fld_base_nic_code).arg(tbl_base_nic).arg(fld_base_nic_name);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4) values(:code,:name,:explain)")
            .arg(tbl_base_nic).arg(fld_base_nic_code).arg(fld_base_nic_name)
            .arg(fld_base_nic_explain);
    if(!q2.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.bindValue(":name",common_client);
    if(q.exec() && q.first())
        specNICs[SNIC_COMMON_CLIENT] = q.value(0).toInt();
    else{
        q2.bindValue("code",++maxCode);
        q2.bindValue(":name",common_client);
        q2.bindValue(":explain",QObject::tr("没有物流能力的通用企业用户"));
        if(!q2.exec())
            return false;
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
            return false;
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
            return false;
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
            return false;
        specNICs[SNIC_GDZC] = maxCode;
    }
    return true;
}

bool AppConfig::_saveMachine(WorkStation *mac)
{
    if(!mac)
        return false;
    QSqlQuery q(db);
    QString s;
    if(mac->getId() == UNID)
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values(%8,%9,%10,'%11','%12',%13)")
                .arg(tbl_machines).arg(fld_mac_mid).arg(fld_mac_type).arg(fld_mac_islocal)
                .arg(fld_mac_sname).arg(fld_mac_desc).arg(fld_mac_ostype).arg(mac->getMID())
                .arg(mac->getType()).arg(mac->isLocalStation()?1:0).arg(mac->name())
                .arg(mac->description()).arg(mac->osType());
    else
        s = QString("update %1 set %2=%3,%4=%5,%6=%7,%8='%9',%10='%11',%12=%13 where id=%14")
                .arg(tbl_machines).arg(fld_mac_mid).arg(mac->getMID()).arg(fld_mac_type)
                .arg(mac->getType()).arg(fld_mac_islocal).arg(mac->isLocalStation()?1:0)
                .arg(fld_mac_sname).arg(mac->name()).arg(fld_mac_desc).arg(mac->description())
                .arg(fld_mac_ostype).arg(mac->osType()).arg(mac->getId());
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
    if(mac->isLocalStation()){
        QString s = QString("update %1 set %2=0 where %3!=%4").arg(tbl_machines)
                .arg(fld_mac_islocal).arg(fld_mac_mid).arg(mac->getMID());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        appIni->beginGroup(SEGMENT_STATIONS);
        appIni->setValue(KEY_STATION_LOID,mac->getMID());
        appIni->endGroup();
    }
    return true;
}

/**
 * @brief 返回存放各种目录所使用的键名
 * @param witch
 * @return
 */
QString AppConfig::_getKeyNameForDir(AppConfig::DirectoryName witch)
{
    switch (witch) {
    case DIR_TRANSOUT:
        return "TransOutDir";
    case DIR_TRANSIN:
        return "TransInDir";
    default:
        return "";
    }
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
 * @brief 读取所有固定资产类别
 * @param gcs
 * @return
 */
bool AppConfig::readAllGdzcClasses(QHash<int, GdzcClass *> gcs)
{
    QSqlQuery q(db);
    QString s = QString("select code,name,zjMonths from gdzc_classes");
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        int code = q.value(0).toInt();
        int zjMonths = q.value(2).toInt();
        QString name = q.value(1).toString();
        allGdzcProductCls[code] = new GdzcClass(code,name,zjMonths);
    }
    return true;
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
            .arg(tbl_base_sscc).arg(subSys);
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
            .arg(tbl_base_sscc).arg(fld_base_sscc_subSys).arg(subSys);
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
    return specCodes.value(subSys).value(witch);
}

/**
 * @brief 返回指定科目系统的所有特定科目配置项
 * @param subSys
 * @return
 */
QHash<AppConfig::SpecSubCode, QString> AppConfig::getAllSpecSubCodeForSubSys(int subSys)
{
    return specCodes.value(subSys);
}

/**
 * @brief 获取所有特定科目在系统中的泛称
 * @param witchs
 * @param names
 * @return
 */
void AppConfig::getSpecSubGenricNames(QList<SpecSubCode> &gcodes, QStringList &gnames)
{
    gcodes<<SSC_CASH<<SSC_BANK<<SSC_GDZC<<SSC_CWFY<<SSC_BNLR<<SSC_LRFP<<SSC_YS
            <<SSC_YF<<SSC_YJSJ<<SSC_ZYSR<<SSC_ZYCB;
    gnames<<QObject::tr("现金")<<QObject::tr("银行")<<QObject::tr("固定资产")<<QObject::tr("财务费用")
        <<QObject::tr("本年利润")<<QObject::tr("利润分配")<<QObject::tr("应收账款")<<QObject::tr("应付账款")
        <<QObject::tr("应交税金")<<QObject::tr("主营业务收入")<<QObject::tr("主营业务成本");
}

/**
 * @brief 获取指定科目系统的所有特定科目配置
 * @param subSys    科目系统代码
 * @param codes     特定科目代码
 * @param names     特定科目名称
 * @return
 */
bool AppConfig::getAllSpecSubNameForSubSys(int subSys, QStringList &codes, QStringList &names)
{
    QSqlQuery q(db);
//    "select specSubCodeConfig.code,FirstSubs.subName from specSubCodeConfig "
//    "join FirstSubs on specSubCodeConfig.subSys=FirstSubs.subCls  and "
//    "specSubCodeConfig.code=FirstSubs.subCode where specSubCodeConfig.subSys=2 "
//    "order by specSubCodeConfig.subEnum"
    QString s = QString("select %1.%3,%2.%4 from %1 join %2 on %1.%5=%2.%6 and "
                        "%1.%3=%2.%7 where %1.%5=%8 order by %1.%9")
            .arg(tbl_base_sscc).arg(tbl_base_fsub).arg(fld_base_sscc_code)
            .arg(fld_base_fsub_subname).arg(fld_base_sscc_subSys)
            .arg(fld_base_fsub_subsys).arg(fld_base_fsub_subcode).arg(subSys)
            .arg(fld_base_sscc_enum);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        codes<<q.value(0).toString();
        names<<q.value(1).toString();
    }
    return true;
}

/**
 * @brief AppConfig::setSpecSubCode
 * @param subSys
 * @param witch
 * @param code
 */
bool AppConfig::setSpecSubCode(int subSys, AppConfig::SpecSubCode witch, QString code)
{
    specCodes[subSys][witch] = code;
    QSqlQuery q(db);
    QString s = QString("update %1 set %2='%3' where %4=%5 and %6=%7")
            .arg(tbl_base_sscc).arg(fld_base_sscc_subSys).arg(subSys)
            .arg(fld_base_sscc_enum).arg(witch).arg(fld_base_sscc_code)
            .arg(code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int num = q.numRowsAffected();
    if(num == 0){
        s = QString("insert into %1(%2,%3,%4) values(%5,%6,'%7')").arg(tbl_base_sscc)
                .arg(fld_base_sscc_subSys).arg(fld_base_sscc_enum).arg(fld_base_sscc_code)
                .arg(subSys).arg(witch).arg(code);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    return true;
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
 * @brief 保存针对指定科目系统的特定科目设置
 * @param subSys
 * @param codes
 * @param names
 * @return
 */
bool AppConfig::saveSpecSubNameForSysSys(int subSys, const QList<SpecSubCode> &gcodes, const QStringList &codes, const QStringList &names)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save special subject config for one subsys");
        return false;
    }
    QSqlQuery q(db);
    QString s = QString("delete from %1 where %2=%3").arg(tbl_base_sscc)
            .arg(fld_base_sscc_subSys).arg(subSys);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    //中间好像还要检查科目代码对应的科目名称是否和基本库中的一级科目表一致？
    s = QString("insert into %1(%2,%3,%4) values(%5,:gcode,:code)").arg(tbl_base_sscc)
            .arg(fld_base_sscc_subSys).arg(fld_base_sscc_enum).arg(fld_base_sscc_code)
            .arg(subSys);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }

    for(int i = 0; i < gcodes.count(); ++i){
        q.bindValue(":gcode",gcodes.at(i));
        q.bindValue(":code",codes.at(i));
        if(!q.exec()){
            LOG_SQLERROR(q.lastQuery());
            return false;
        }
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on save special subject config for one subsys");
        return false;
    }
    return true;
}

/**
 * @brief AppConfig::getMasterStation
 * 返回主站对象
 * @return
 */
WorkStation *AppConfig::getMasterStation()
{
    foreach(WorkStation* m, machines){
        if(m->getMID() == msId)
            return m;
    }
    return 0;
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
WorkStation *AppConfig::getLocalStation()
{
    QHashIterator<int,WorkStation*> it(machines);
    while(it.hasNext()){
        it.next();
        if(it.value()->isLocalStation())
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
bool AppConfig::saveMachine(WorkStation *mac)
{
    return _saveMachine(mac);
}

bool AppConfig::saveMachines(QList<WorkStation *> macs)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save machines list!");
        return false;
    }
    foreach(WorkStation* mac, macs){
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

bool AppConfig::removeMachine(WorkStation *mac)
{
    if(!mac)
        return false;
    if(mac->getId() != UNID){
        QSqlQuery q(db);
        QString s = QString("delete from %1 where id=%2").arg(tbl_machines).arg(mac->getId());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    machines.remove(mac->getMID());
    delete mac;
    return true;
}

bool AppConfig::getOsTypes(QHash<int, QString>& types)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2").arg(tbl_base_osTypes).arg(fld_base_osTypes_code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        int code = q.value(FI_BASE_OSTYPES_CODE).toInt();
        QString mName = q.value(FI_BASE_OSTYPES_MN).toString();
        QString sName = q.value(FI_BASE_OSTYPES_SN).toString();
        types[code] = mName + " " + sName;
    }
    return true;
}

/**
 * @brief 读取凭证模板配置参数
 * @param parameter
 * @return
 */
bool AppConfig::getPzTemplateParameter(PzTemplateParameter *parameter)
{
    if(!parameter)
        return false;
    appIni->beginGroup(SEGMENT_PZ_TEMPLATE);
    parameter->baRowHeight = appIni->value(KEY_PZT_BAROWHEIGHT,10.5).toDouble();
    parameter->titleHeight = appIni->value(KEY_PZT_BATITLEHEIGHT,10).toDouble();
    parameter->baRows = appIni->value(KEY_PZT_BAROWNUM,8).toInt();
    parameter->cutAreaHeight = appIni->value(KEY_PZT_CUTAREA,14).toInt();
    parameter->leftRightMargin = appIni->value(KEY_PZT_LR_MARGIN,8).toInt();
    parameter->topBottonMargin = appIni->value(KEY_PZT_TB_MARGIN,8).toInt();
    parameter->fontSize = appIni->value(KEY_PZT_FONTSIZE,8).toInt();
    parameter->isPrintCutLine = appIni->value(KEY_PZT_ISPRINTCUTLINE,true).toBool();
    parameter->isPrintMidLine = appIni->value(KEY_PZT_ISPRINTMIDLINE,true).toBool();
    bool result = true;
    QString vs = "0.27,0.26,0.12,0.10,0.05";
    QStringList factors = appIni->value(KEY_PZT_BATABLE_FACTOR,vs).toString().split(",");
    if(factors.count() != 5){
        LOG_ERROR("PingZheng template parameter error(allocate factor error)");
        factors = vs.split(",");
    }
    for(int i = 0; i < factors.count(); ++i){
        QString factor = factors.at(i);
        bool ok = true;
        double f = factor.toDouble(&ok);
        if(!ok){
            result = false;
            break;
        }
        else
            parameter->factor[i] = f;
    }
    appIni->endGroup();
    if(!result)
        return false;
    return true;
}

/**
 * @brief 保存凭证模板配置参数
 * @param parameter
 * @return
 */
bool AppConfig::savePzTemplateParameter(PzTemplateParameter *parameter)
{
    if(!parameter)
        return false;
    appIni->beginGroup(SEGMENT_PZ_TEMPLATE);
    appIni->setValue(KEY_PZT_BATITLEHEIGHT,parameter->titleHeight);
    appIni->setValue(KEY_PZT_BAROWHEIGHT,parameter->baRowHeight);
    appIni->setValue(KEY_PZT_BAROWNUM,parameter->baRows);
    appIni->setValue(KEY_PZT_CUTAREA,parameter->cutAreaHeight);
    appIni->setValue(KEY_PZT_LR_MARGIN,parameter->leftRightMargin);
    appIni->setValue(KEY_PZT_TB_MARGIN,parameter->topBottonMargin);
    appIni->setValue(KEY_PZT_FONTSIZE,parameter->fontSize);
    appIni->setValue(KEY_PZT_ISPRINTCUTLINE,parameter->isPrintCutLine);
    appIni->setValue(KEY_PZT_ISPRINTMIDLINE,parameter->isPrintMidLine);
    QStringList factors;
    for(int i = 0; i < 5; ++i){
        factors<<QString::number(parameter->factor[i],'f',3);
    }
    appIni->setValue(KEY_PZT_BATABLE_FACTOR,factors.join(","));
    appIni->endGroup();
    appIni->sync();
    return true;
}

QString AppConfig::getDirName(DirectoryName witch)
{
    QString key = _getKeyNameForDir(witch);
    if(key.isEmpty()){
        LOG_ERROR(QString("Key name is illegal in segment '%1'").arg(SEGMENT_DIR));
        return "";
    }
    appIni->beginGroup(SEGMENT_DIR);
    QString dir = appIni->value(key).toString();
    if(dir.isEmpty())
        dir = QDir::home().absolutePath();
    appIni->endGroup();
    return dir;
}

void AppConfig::saveDirName(DirectoryName witch, QString dir)
{
    QString key = _getKeyNameForDir(witch);
    if(key.isEmpty()){
        LOG_ERROR(QString("Key name is illegal in segment '%1'").arg(SEGMENT_DIR));
        return;
    }
    appIni->beginGroup(SEGMENT_DIR);
    appIni->setValue(key,dir);
    appIni->endGroup();
    appIni->sync();
}

/**
 * @brief AppConfig::getSubWinInfo
 *  读取子窗口的各种几何尺寸信息
 * @param winEnum
 * @param info  位置和大小信息
 * @param sinfo 其他公共信息（比如表格列宽等）
 * @return
 */
bool AppConfig::getSubWinInfo(int winEnum, SubWindowDim *&info, QByteArray* sinfo)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2 = %3")
           .arg(tbl_base_subWinInfo).arg(fld_base_swi_enum).arg(winEnum);
    if(!q.exec(s)){
       LOG_SQLERROR(s);
       return false;
    }
    if(q.first()){
        info = new SubWindowDim;
        info->x = q.value(FI_BASE_SWI_X).toInt();
        info->y = q.value(FI_BASE_SWI_Y).toInt();
        info->w = q.value(FI_BASE_SWI_W).toInt();
        info->h = q.value(FI_BASE_SWI_H).toInt();
        if(sinfo)
            *sinfo = q.value(FI_BASE_SWI_TBL).toByteArray();
    }
    return true;
}

/**
 * @brief AppConfig::saveSubWinInfo
 *  保存子窗口的各种几何尺寸信息
 * @param winEnum
 * @param info
 * @param otherInfo
 * @return
 */
bool AppConfig::saveSubWinInfo(int winEnum, SubWindowDim *info, QByteArray* sinfo)
{
    QSqlQuery q(db);
    QString s;
    s = QString("update %1 set %2=:x,%3=:y,%4=:w,%5=:h")
            .arg(tbl_base_subWinInfo).arg(fld_base_swi_x).arg(fld_base_swi_y)
            .arg(fld_base_swi_width).arg(fld_base_swi_height);
    if(sinfo)
        s.append(QString(",%1=:state").arg(fld_base_swi_stateInfo));
    s.append(QString(" where %1=:enum").arg(fld_base_swi_enum));
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.bindValue(":enum",winEnum);
    q.bindValue(":x",info->x);
    q.bindValue(":y",info->y);
    q.bindValue(":w",info->w);
    q.bindValue(":h",info->h);
    if(sinfo)
        q.bindValue(":state",*sinfo);
    if(!q.exec()){
        LOG_SQLERROR(q.lastQuery());
        return false;
    }
    if(q.numRowsAffected() == 0){
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7) "
                    "values(:enum,:x,:y,:w,:h,:state)")
                .arg(tbl_base_subWinInfo).arg(fld_base_swi_enum).arg(fld_base_swi_x).arg(fld_base_swi_y)
                .arg(fld_base_swi_width).arg(fld_base_swi_height).arg(fld_base_swi_stateInfo);
        if(!q.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.bindValue(":enum",winEnum);
        q.bindValue(":x",info->x);
        q.bindValue(":y",info->y);
        q.bindValue(":w",info->w);
        q.bindValue(":h",info->h);
        if(sinfo)
            q.bindValue(":state",*sinfo);
        else
            q.bindValue(":state",QByteArray());
        if(!q.exec()){
            LOG_SQLERROR(s);
            return false;
        }
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

