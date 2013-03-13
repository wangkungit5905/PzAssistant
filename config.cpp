#include <QSettings>
#include <QStringList>
#include <QTextCodec>
#include <QInputDialog>

#include "global.h"
#include "config.h"
#include "tables.h"

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
 * @brief AppConfig::versionMaintain
 *  维护配置模块的版本
 * @return
 */
bool AppConfig::versionMaintain(bool& cancel)
{
    getInstance();
    VersionManager* vm = new VersionManager(VersionManager::MT_CONF);
    //每当有新的可用升级函数时，就在此添加
    vm->appendVersion(1,1,&AppConfig::updateTo1_1);
    vm->appendVersion(1,2,&AppConfig::updateTo1_2);
    //vm->appendVersion(2,0,&AppConfig::updateTo2_0);
    bool r = vm->versionMaintain(cancel);
    delete vm;
    return r;
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

/**
 * @brief AppConfig::perfectVersion
 *  完善基本库的结构，使其满足1.0版的要求
 *  配置模块的版本号基本库中的version表内
 * @return
 */
bool AppConfig::perfectVersion()
{
    QSqlQuery q(db);
    //检测是否存在version表（这是初始版本的标记）
    QString s = "select name from sqlite_master where name='version'";
    if(!q.exec(s))
        return false;
    if(q.first())
        return true;
		
    //添加version表，并插入一条初始版本号记录
    s = "create table version(id INTEGER PRIMARY KEY, master INTEGER, second INTGER)";
    if(!q.exec(s))
        return false;
    s = "insert into version(master,second) values(1,0)";
    if(!q.exec(s))
        return false;
    return true;
}

/**
 * @brief AppConfig::getVersion
 *  获取基本库的版本号
 * @param mv  主版本号
 * @param sv  次版本号
 * @return
 */
bool AppConfig::getVersion(int &mv, int &sv)
{
    QSqlQuery q(db);
    QString s = "select master, second from version";
    if(!q.exec(s))
        return false;
    if(!q.first())
        return false;
    mv = q.value(0).toInt();
    sv = q.value(1).toInt();
    return true;
}

/**
 * @brief AppConfig::setVersion
 *  设置基本库的版本号
 * @param mv
 * @param sv
 * @return
 */
bool AppConfig::setVersion(int mv, int sv)
{
    QSqlQuery q(db);
    QString s = QString("update version set master=%1,second=%2").arg(mv).arg(sv);
    return q.exec(s);
}


/**
 * @brief AppConfig::updateTo1_1
 *  升级任务描述：
 *  1、用新的简化版凭证集状态替换原先相对复杂的
 * @return
 */
bool AppConfig::updateTo1_1()
{
    QSqlQuery q(db);
    QString s = QString("delete from %1").arg(tbl_pzsStateNames);
    if(!q.exec(s))
        return false;

    QList<PzsState> codes;
    QHash<PzsState,QString> snames,lnames;
    codes<<Ps_NoOpen<<Ps_Rec<<Ps_AllVerified<<Ps_Jzed;
    snames[Ps_NoOpen] = QObject::tr("未打开");
    snames[Ps_Rec] = QObject::tr("录入态");
    snames[Ps_AllVerified] = QObject::tr("入账态");
    snames[Ps_Jzed] = QObject::tr("结转");
    lnames[Ps_NoOpen] = QObject::tr("未打开凭证集");
    lnames[Ps_Rec] = QObject::tr("正在录入凭证");
    lnames[Ps_AllVerified] = QObject::tr("凭证都已审核，可以进行统计并保存余额");
    lnames[Ps_Jzed] = QObject::tr("已结账，不能做出任何影响凭证集数据的操作");

    if(!db.transaction())
        return false;
    for(int i = 0; i < codes.count(); ++i){
        PzsState code = codes.at(i);
        s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')")
                .arg(tbl_pzsStateNames).arg(fld_pzssn_code).arg(fld_pzssn_sname)
                .arg(fld_pzssn_lname).arg(code).arg(snames.value(code))
                .arg(lnames.value(code));
        q.exec(s);
    }
    if(!db.commit())
        return true;
    return setVersion(1,1);

}

/**
 * @brief AppConfig::updateTo1_2
 *  添加日志级别的设置字段
 * @return
 */
bool AppConfig::updateTo1_2()
{
    appIni->beginGroup("Debug");
    appIni->setValue("loglevel", Logger::levelToString(Logger::Debug));
    appIni->endGroup();
    return setVersion(1,2);
}

bool AppConfig::updateTo1_3()
{
    //1、在基本库中添加machines表，并初始化4个默认主机
    //2、根据用户的选择设置isLacal字段（配置本机是哪个主机标识）
    QSqlQuery q(db);
    QHash<int,QString> snames;
    QHash<int,QString> lnames;
    snames[101] = QObject::tr("本家PC");
    lnames[101] = QObject::tr("家里的桌面电脑");
    snames[102] = QObject::tr("单位电脑");
    lnames[102] = QObject::tr("单位的桌面电脑");
    snames[103] = QObject::tr("我的笔记本（linux）");
    lnames[103] = QObject::tr("我的笔记本上的linux系统");
    snames[104] = QObject::tr("我的笔记本（Windows XP）");
    lnames[104] = QObject::tr("我的笔记本上的Windows XP系统");

    QList<int> mids = snames.keys();
    QStringList names;
    for(int i = 0; i < mids.count(); ++i)
        names<<snames.value(mids.at(i));
    bool ok;
    QString name = QInputDialog::getItem(0,QObject::tr("信息输入框"),
                          QObject::tr("选择一个匹配你当前运行主机的主机名："),
                          names,0,false,&ok);
    if(!ok)
        return false;

    int index = names.indexOf(name);
    int mid = mids.at(index);        //选择的主机标识
    QHashIterator<int,QString> it(snames);
    QString s = "create table machines(id integer primary key, type integer, mid integer, isLocal integer, sname text, lname text)";
    if(!q.exec(s))
        return false;
    it.toFront();
    while(it.hasNext()){
        it.next();
        s = QString("insert into machines(type,mid,isLocal,sname,lname) values(1,%1,%2,'%3','%4')")
                .arg(it.key()).arg(0).arg(it.value()).arg(lnames.value(it.key()));
        if(!q.exec(s))
            return false;
    }
    s = QString("update machines set isLocal=1 where mid=%1").arg(mid);
    if(!q.exec(s))
        return false;
    return setVersion(1,3);
}

bool AppConfig::updateTo2_0()
{
    return true;
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
bool AppConfig::saveAccInfo(AccountBriefInfo* accInfo)
{
    QSqlQuery q(db);
    QString s = QString("select id from AccountInfos where code = '%1'")
            .arg(accInfo->code);
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
                    "lname='%4' where id=%5").arg(accInfo->code).arg(accInfo->fileName)
                .arg(accInfo->accName).arg(accInfo->accLName).arg(id);
    }
    else{
        s = QString("insert into AccountInfos(code,filename,name,"
                    "lname) values('%1','%2','%3','%4')")
                .arg(accInfo->code).arg(accInfo->fileName).arg(accInfo->accName)
                .arg(accInfo->accLName);
    }
    bool r = q.exec(s);
    return r;
}

//读取账户信息
bool AppConfig::getAccInfo(int id, AccountBriefInfo* accInfo)
{
    QSqlQuery q(db);
    QString s = QString("select * from AccountInfos where id=%1").arg(id);
    if(q.exec(s) && q.first()){
        accInfo->id = q.value(0).toInt();
        accInfo->code = q.value(ACCIN_CODE).toString();
        //accInfo->baseTime = q->value(ACCIN_BASETIME).toString();
        //accInfo->usedSubSys = q->value(ACCIN_USS).toInt();
        //accInfo->usedRptType = q->value(ACCIN_USRPT).toInt();
        accInfo->fileName = q.value(ACCIN_FNAME).toString();
        accInfo->accName = q.value(ACCIN_NAME).toString();
        accInfo->accLName = q.value(ACCIN_LNAME).toString();
        //accInfo->lastTime = q->value(ACCIN_LASTTIME).toString();
        //accInfo->desc = q->value(ACCIN_DESC).toString();
        return true;
    }
    else
        return false;
}

bool AppConfig::getAccInfo(QString code, AccountBriefInfo* accInfo)
{
    QSqlQuery q(db);
    QString s = QString("select * from AccountInfos where code=%1").arg(code);
    if(q.exec(s) && q.first()){
        accInfo->id = q.value(0).toInt();
        accInfo->code = q.value(ACCIN_CODE).toString();
        //accInfo->baseTime = q->value(ACCIN_BASETIME).toString();
        //accInfo->usedSubSys = q->value(ACCIN_USS).toInt();
        //accInfo->usedRptType = q->value(ACCIN_USRPT).toInt();
        accInfo->fileName = q.value(ACCIN_FNAME).toString();
        accInfo->accName = q.value(ACCIN_NAME).toString();
        accInfo->accLName = q.value(ACCIN_LNAME).toString();
        //accInfo->lastTime = q->value(ACCIN_LASTTIME).toString();
        //accInfo->desc = q->value(ACCIN_DESC).toString();
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
            accInfo->fileName = q.value(ACCIN_FNAME).toString();
            accInfo->accName = q.value(ACCIN_NAME).toString();
            accInfo->accLName = q.value(ACCIN_LNAME).toString();
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


/////////////////////////////VersionManager//////////////////////////////////
VersionManager::VersionManager(VersionManager::ModuleType moduleType)
{
    switch(moduleType){
    case MT_CONF:
        initConf();
        break;
    case MT_ACC:
        initAcc();
        break;
    }
}

/**
 * @brief VersionManager::compareVersion
 *  比较版本，如果当前版本低于最高可用版本，则提示升级，如果用户同意则执行升级操作
 * @param cancel：在返回为false时，其值为true，则表示用户取消了升级操作
 * @return true：升级成功，false：升级失败或用户取消升级
 */
bool VersionManager::versionMaintain(bool& cancel)
{
    cancel = false;
    int curMv, curSv;
    int maxMv, maxSv;

    //按需进行初始版本归集
    if(!pvFun())
        return false;

    if(!(*gvFun)(curMv,curSv))
        return false;
    QStringList vs = versionHistorys.last().split("_");
    maxMv = vs.first().toInt();
    maxSv = vs.last().toInt();
    if(curMv < maxMv || curSv < maxSv){
        if(QMessageBox::Yes ==
            QMessageBox::information(0,QObject::tr("更新通知"),
                                     QObject::tr("%1需要更新").arg(moduleName),
                                     QMessageBox::Yes|QMessageBox::No)){
            if(!updateVersion(curMv,curSv))
                return false;
        }
        else{
            cancel = true;
            return false;
        }
    }
    return true;
}

/**
 * @brief VersionManager::appendVersion
 *  添加新版本
 * @param mv    主版本
 * @param sv    次版本
 * @param fun   升级函数
 */
void VersionManager::appendVersion(int mv, int sv, UpdateVersionFun fun)
{
    //必须确保添加的版本比最近添加的版本更新，为方便，这里没有执行检测
    QString vs = QString("%1_%2").arg(mv).arg(sv);
    versionHistorys<<vs;
    updateFuns[vs] = fun;
}

void VersionManager::initConf()
{
    versionHistorys<<"1_0";
    moduleName = QObject::tr("配置模块");
    gvFun = &AppConfig::getVersion;
    svFun = &AppConfig::setVersion;
    pvFun = &AppConfig::perfectVersion;
}

void VersionManager::initAcc()
{
    versionHistorys<<"1_2";    //添加初始版本号
    moduleName = QObject::tr("账户数据库");

    gvFun = &Account::getVersion;
    svFun = &Account::setVersion;
    pvFun = &Account::perfectVersion;
}

/**
 * @brief VersionManager::inspectVerBeforeUpdate
 *  更新到某个版本前调用本函数检测当前版本是否是指定的前置版本
 * @param mv
 * @param sv
 * @return
 */
bool VersionManager::inspectVerBeforeUpdate(int mv, int sv)
{
    int cmv,csv;
    if(!(*gvFun)(cmv,csv))
        return false;
    if(cmv != mv && csv != sv)
        return false;
    return true;
}

/**
 * @brief VersionManager::updateBase
 *  从起始版本更新到当前最新版本（即在历史版本号中的最高版本）
 * @param startMv
 * @param startSv
 * @return
 */
bool VersionManager::updateVersion(int startMv, int startSv)
{
    QString startVer = QString("%1_%2").arg(startMv).arg(startSv);
    QList<UpdateVersionFun> funs;
    int i;
    for(i = 0; i < versionHistorys.count(); ++i){
        if(startVer.compare(versionHistorys.at(i)) <= 0)
            break;
        else
            continue;
    }
    i++;
    int startIndex = i-1;            //更新的开始版本号所对应的索引
    if(i == versionHistorys.count()) //没有可用更新
        return true;
    //收集要执行的更新函数
    int pmv,psv,umv,usv; //前置版本和目的更新版本号
    for(i; i < versionHistorys.count(); ++i)
        funs<<updateFuns.value(versionHistorys.at(i));

    //执行更新函数
    for(int j=0; j < funs.count(); ++j){
        //每次执行更新函数前，要进行前置版本检测
        QStringList vs = versionHistorys.at(startIndex).split("_");
        pmv = vs.at(0).toInt();
        psv = vs.at(1).toInt();
        vs = versionHistorys.at(startIndex+1).split("_");
        umv = vs.at(0).toInt();
        usv = vs.at(1).toInt();
        if(!inspectVerBeforeUpdate(pmv,psv)){
            QMessageBox::critical(0,QObject::tr("更新出错"),
                                  QObject::tr("在更新至版本%1.%2时出错（前置版本不符，要求是%3.%4）")
                                  .arg(umv).arg(usv).arg(pmv).arg(psv));
            return false;
        }
        if(!(*funs.at(j))())
            return false;
        startIndex++;
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
