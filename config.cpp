#include <QSettings>
#include <QStringList>
#include <QTextCodec>
#include <QInputDialog>

#include "global.h"
#include "config.h"
#include "tables.h"
#include "version.h"

QSettings *AppConfig::appIni = new QSettings("./config/app/appSetting.ini", QSettings::IniFormat);
AppConfig* AppConfig::instance = 0;
QSqlDatabase AppConfig::db;

//////////////////////////////////AppConfig//////////////////////////////////////
AppConfig::AppConfig()
{
    appIni->setIniCodec(QTextCodec::codecForTr());
}

AppConfig::~AppConfig()
{
    db.close();
    appIni->sync();
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
    if(!getConVar("RecentOpenAccId",curAccountId))
        curAccountId = 0;
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
    r = setConVar("RecentOpenAccId", curAccountId);
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

//清除账户信息
bool AppConfig::clear()
{
    QSqlQuery q(db);
    QString s = "delete from AccountInfos";
    bool r = q.exec(s);
    return r;
}

//测试是否存在指定代码的账户
bool AppConfig::isExist(QString code)
{
    QSqlQuery q(db);
    QString s = QString("select id from AccountInfos where code='%1'").arg(code);
    if(q.exec(s) && q.first())
        return true;
    else
        return false;
}

//保存账户简要信息
bool AppConfig::saveAccInfo(AccountBriefInfo accInfo)
{
    QSqlQuery q(db);
    QString s = QString("select id from AccountInfos where code = '%1'")
            .arg(accInfo.code);
//    if(q->exec(s) && q->first()){
//        int id = q->value(0).toInt();
//        s = QString("update AccountInfos set code='%1',baseTime='%2',usedSubSys=%3,"
//                    "usedRptType=%4,filename='%5',name='%6',lname='%7',lastTime='%8',"
//                    "desc='%9' where id=%10").arg(accInfo->code).arg(accInfo->baseTime)
//                .arg(accInfo->usedSubSys).arg(accInfo->usedRptType).arg(accInfo->fileName)
//                .arg(accInfo->accName).arg(accInfo->accLName).arg(accInfo->lastTime)
//                .arg(accInfo->desc).arg(id);
//    }
//    else{
//        s = QString("insert into AccountInfos(code,baseTime,usedSubSys,usedRptType,filename,name,"
//                    "lname,lastTime,desc) values('%1','%2',%3,%4,'%5','%6','%7','%8','%9')")
//                .arg(accInfo->code).arg(accInfo->baseTime).arg(accInfo->usedSubSys)
//                .arg(accInfo->usedRptType).arg(accInfo->fileName).arg(accInfo->accName)
//                .arg(accInfo->accLName).arg(accInfo->lastTime).arg(accInfo->desc);
//    }

    if(q.exec(s) && q.first()){
        int id = q.value(0).toInt();
        s = QString("update AccountInfos set code='%1',filename='%2',name='%3',"
                    "lname='%4' where id=%5").arg(accInfo.code).arg(accInfo.fname)
                .arg(accInfo.sname).arg(accInfo.lname).arg(id);
    }
    else{
        s = QString("insert into AccountInfos(code,filename,name,"
                    "lname) values('%1','%2','%3','%4')")
                .arg(accInfo.code).arg(accInfo.fname).arg(accInfo.sname)
                .arg(accInfo.lname);
    }
    bool r = q.exec(s);
    return r;
}

//读取账户信息
bool AppConfig::getAccInfo(int id, AccountBriefInfo& accInfo)
{
    QSqlQuery q(db);
    QString s = QString("select * from AccountInfos where id=%1").arg(id);
    if(q.exec(s) && q.first()){
        accInfo.id = q.value(0).toInt();
        accInfo.code = q.value(ACCIN_CODE).toString();
        accInfo.fname = q.value(ACCIN_FNAME).toString();
        accInfo.sname = q.value(ACCIN_NAME).toString();
        accInfo.lname = q.value(ACCIN_LNAME).toString();
        return true;
    }
    else
        return false;
}

bool AppConfig::getAccInfo(QString code, AccountBriefInfo accInfo)
{
    QSqlQuery q(db);
    QString s = QString("select * from AccountInfos where code=%1").arg(code);
    if(q.exec(s) && q.first()){
        accInfo.id = q.value(0).toInt();
        accInfo.code = q.value(ACCIN_CODE).toString();
        accInfo.fname = q.value(ACCIN_FNAME).toString();
        accInfo.sname = q.value(ACCIN_NAME).toString();
        accInfo.lname = q.value(ACCIN_LNAME).toString();
        return true;
    }
    else
        return false;
}

//读取所有账户信息
bool AppConfig::readAccountInfos(QList<AccountBriefInfo*>& accs)
{
    QSqlQuery q(db);
    QString s = "select * from AccountInfos";
    bool r = q.exec(s);
    if(r){
        AccountBriefInfo* accInfo;
        while(q.next()){
            accInfo = new AccountBriefInfo;
            accInfo->id = q.value(0).toInt();
            accInfo->code = q.value(ACCIN_CODE).toString();
            //accInfo->baseTime = q->value(ACCIN_BASETIME).toString();
            //accInfo->usedSubSys = q->value(ACCIN_USS).toInt();
            //accInfo->usedRptType = q->value(ACCIN_USRPT).toInt();
            accInfo->fname = q.value(ACCIN_FNAME).toString();
            accInfo->sname = q.value(ACCIN_NAME).toString();
            accInfo->lname = q.value(ACCIN_LNAME).toString();
            //accInfo->lastTime = q->value(ACCIN_LASTTIME).toString();
            //accInfo->desc = q->value(ACCIN_DESC).toString();
            accs.append(accInfo);
        }
    }
    return r;
}

//保存最近打开账户的id到配置变量中
bool AppConfig::setRecentOpenAccount(int id)
{
    return setConVar("RecentOpenAccId", id);
}

//读取最近打开账户的id
bool AppConfig::getRecentOpenAccount(int& curAccId)
{
    return getConVar("RecentOpenAccId", curAccId);
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
