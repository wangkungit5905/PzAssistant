#include <QObject>
#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>
#include <QTextCodec>

#include "version.h"
#include "global.h"
#include "tables.h"

QSqlDatabase VMAccount::db;
QSqlDatabase VMAppConfig::db;
QSettings *VMAppConfig::appIni;

/////////////////////////////VersionManager//////////////////////////////////
VersionManager::VersionManager(VersionManager::ModuleType moduleType, QString fname)
    :mt(moduleType),fileName(fname)
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
    close();
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
    VMAppConfig::db = QSqlDatabase::addDatabase("QSQLITE", VM_BASIC);
    fileName = BaseDataPath + "basicdata.dat";
    VMAppConfig::db.setDatabaseName(fileName);
    if (!VMAppConfig::db.open()){
        QMessageBox::critical(0, QObject::tr("不能打开基础数据库"),
            QObject::tr("不能够建立与基础数据库之间的连接\n"
                     "请检查data目录下是否存在basicdata.dat文件"), QMessageBox::Cancel);
        return;
    }

    VMAppConfig::appIni = new QSettings("./config/app/appSetting.ini", QSettings::IniFormat);
    VMAppConfig::appIni->setIniCodec(QTextCodec::codecForTr());

    versionHistorys<<"1_0";
    moduleName = QObject::tr("配置模块");
    gvFun = &VMAppConfig::getVersion;
    svFun = &VMAppConfig::setVersion;
    pvFun = &VMAppConfig::perfectVersion;

    appendVersion(1,1,&VMAppConfig::updateTo1_1);
    appendVersion(1,2,&VMAppConfig::updateTo1_2);
    //appendVersion(2,0,&VMAppConfig::updateTo2_0);
}

void VersionManager::initAcc()
{
    VMAccount::db = QSqlDatabase::addDatabase("QSQLITE", VM_ACCOUNT);
    VMAccount::db.setDatabaseName(DatabasePath+fileName);
    if(!VMAccount::db.open()){
        QMessageBox::critical(0, QObject::tr("不能打开基础数据库"),
            QObject::tr("不能够建立与账户数据库之间的连接\n"
                        "请检查data目录下是否存在%1文件").arg(fileName), QMessageBox::Cancel);
        return;
    }
    versionHistorys<<"1_2";    //添加初始版本号
    moduleName = QObject::tr("账户数据库");

    gvFun = &VMAccount::getVersion;
    svFun = &VMAccount::setVersion;
    pvFun = &VMAccount::perfectVersion;

    appendVersion(1,3,&VMAccount::updateTo1_3);
    appendVersion(1,4,&VMAccount::updateTo1_4);
    //appendVersion(1,5,&VMAccount::updateTo1_5);
}

/**
 * @brief VersionManager::close
 *  在更新操作完成后，关闭数据库连接和打开的配置文件。
 */
void VersionManager::close()
{
    if(mt == MT_CONF){
        VMAppConfig::db.close();
        QSqlDatabase::removeDatabase(VM_BASIC);
        VMAppConfig::appIni->sync();
        delete VMAppConfig::appIni;
    }
    else{
        VMAccount::db.close();
        QSqlDatabase::removeDatabase(VM_ACCOUNT);
    }
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


bool VMAccount::perfectVersion()
{
    return true;
}

/**
 * @brief VMAccount::getVersion
 *  获取账户数据库版本
 * @param mv
 * @param sv
 * @return
 */
bool VMAccount::getVersion(int &mv, int &sv)
{
    QSqlQuery q(db);
    QString s = QString("select %1,%2 from %3 where %4=%5")
            .arg(fld_acci_name).arg(fld_acci_value).arg(tbl_accInfo)
            .arg(fld_acci_code).arg(Account::DBVERSION);
    if(!q.exec(s))
        return false;
    if(!q.first())
        return false;
    QStringList ls = q.value(1).toString().split(".");
    if(ls.count() != 2)
        return false;
    bool ok;
    mv = ls.at(0).toInt(&ok);
    if(!ok)
        return false;
    sv = ls.at(1).toInt(&ok);
    if(!ok)
        return false;
    return true;
}

/**
 * @brief VMAccount::setVersion
 *  设置账户数据库版本
 * @param mv
 * @param sv
 * @return
 */
bool VMAccount::setVersion(int mv, int sv)
{
    QSqlQuery q(db);
    QString verStr = QString("%1.%2").arg(mv).arg(sv);
    QString s = QString("update %1 set %2=%3 where %4=%5")
            .arg(tbl_accInfo).arg(fld_acci_value)
            .arg(verStr).arg(fld_acci_code).arg(Account::DBVERSION);
    if(!q.exec(s))
        return false;
    return true;
}

/**
 * @brief VMAccount::updateTo1_3
 *  升级任务：
 *  1、创建名称条目表“nameItems”替换“SecSubjects”表，添加创建时间列，创建者
 *  2、修改FSAgent表，添加创建（启用）时间列、创建者、禁用时间列
 *  3、修改FirSubjects表，添加科目系统（subSys）
 *  4、修改一级科目类别表结构，增加科目系统类型字段“subSys”
 *  5、创建帐套表accountSuites，从accountInfo表内读取有关帐套的数据进行初始化
 * @return
 */
bool VMAccount::updateTo1_3()
{
    QSqlQuery q(db);
    QString s;
    bool r,ok;

    //1、修改SecSubjects表，添加创建时间列
    s = "alter table SecSubjects rename to old_SecSubjects";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在更改“SecSubjects”表名时发生错误！"));
        return false;
    }
    s = "CREATE TABLE nameItems(id INTEGER PRIMARY KEY, sName text, lName text, remCode text, classId integer, createdTime TimeStamp NOT NULL DEFAULT (datetime('now','localtime')), creator integer)";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在创建“SecSubjects”表时发生错误！"));
        return false;
    }
    if(!db.transaction()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“SecSubjects”的数据时，启动事务失败！"));
        return false;
    }
    s = "insert into nameItems(id,sName,lName,remCode,classId,creator) "
            "select id,subName,subLName,remCode,classId,1 as user from old_SecSubjects";
    q.exec(s);
    if(!db.commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“SecSubjects”的数据时，提交事务失败！"));
        if(!db.rollback())
            QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“SecSubjects”的数据时，事务回滚失败！"));
        return false;
    }

    //进行校对
    QSqlQuery q2(db);
    int id;
    QString name,newname;
    s = "select id, subName from old_SecSubjects";
    r = q.exec(s);
    r = q2.prepare("select sName from nameItems where id = :id");
    while(q.next()){
        ok = true;
        id = q.value(0).toInt();
        name = q.value(1).toString();
        q2.bindValue(":id",id);
        if(!q2.exec())
            ok = false;
        if(!q2.first())
            ok = false;
        newname = q2.value(0).toString();
        if(QString::compare(name,newname) != 0)
            ok = false;
        if(!ok){
            QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在校对表“SecSubjects”的数据时，发现数据的不一致！"));
            break;
        }
    }
    //在这里删除表，会出错，不知为啥？ 所有必须在第二次打开时删除
    s = "delete from old_SecSubjects";
    r = q.exec(s);
    s = "drop table old_SecSubjects";
    if(!q.exec(s))
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在删除表“old_SecSubjects”表时发生错误，请先退出应用，使用专门工具删除它！"));

     //2、修改FSAgent表，添加创建（启用）时间列、禁用时间列、创建者
    s = "alter table FSAgent rename to old_FSAgent";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在更改“FSAgent”表名时发生错误！"));
        return false;
    }
    s = "CREATE TABLE FSAgent(id INTEGER PRIMARY KEY, fid INTEGER, sid INTEGER, subCode varchar(5), weight INTEGER, isEnabled INTEGER,disabledTime TimeStamp, createdTime NOT NULL DEFAULT (datetime('now','localtime')),creator integer)";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在创建“FSAgent”表时发生错误！"));
        return false;
    }
    if(!db.transaction()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FSAgent”的数据时，启动事务失败！"));
        return false;
    }
    s = "insert into FSAgent(id,fid,sid,subCode,weight,isEnabled,creator) select id,fid,sid,subCode,FrequencyStat,isEnabled,1 as user from old_FSAgent";
    r = q.exec(s);
    s = "update FSAgent set isEnabled=1,weight=1";
    r = q.exec(s);
    if(!db.commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FSAgent”的数据时，提交事务失败！"));
        if(!db.rollback())
            QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FSAgent”的数据时，事务回滚失败！"));
        return false;
    }

    //进行校对
    int fid,nfid,sid,nsid;
    s = "select id,fid,sid from old_FSAgent";
    r = q.exec(s);
    s = "select fid,sid from FSAgent where id=:id";
    r = q2.prepare(s);
    while(q.next()){
        ok = true;
        id = q.value(0).toInt();
        fid = q.value(1).toInt();
        sid = q.value(2).toInt();
        q2.bindValue(":id",id);
        if(!q2.exec())
            ok = false;
        if(!q2.first())
            ok = false;
        nfid = q2.value(0).toInt();
        nsid = q2.value(1).toInt();
        if(fid != nfid || sid != nsid)
            ok = false;
        if(!ok){
            QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在校对表“FSAgent”的数据时，发现数据的不一致！"));
            break;
        }
    }
    s = "delete from old_FSAgent";
    r = q.exec(s);
    s = "drop table old_FSAgent";
    if(!q.exec(s))
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在删除表“old_FSAgent”表时发生错误，请先退出应用，使用专门工具删除它！"));

    //3、修改FirSubjects表，添加科目系统（subSys）、科目是否启用（enabled）字段
    s = "alter table FirSubjects rename to old_FirSubjects";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在更改“FirSubjects”表名时发生错误！"));
        return false;
    }
    s = "CREATE TABLE FirSubjects(id INTEGER PRIMARY KEY, subSys INTEGER, subCode varchar(4), remCode varchar(10), belongTo integer, jdDir integer, isView integer, isUseWb INTEGER, weight integer, subName varchar(10))";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在创建“FirSubjects”表时发生错误！"));
        return false;
    }
    if(!db.transaction()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FirSubjects”的数据时，启动事务失败！"));
        return false;
    }
    s = "insert into FirSubjects(id,subCode,remCode,belongTo,jdDir,isView,isUseWb,weight,subName) select id,subCode,remCode,belongTo,jdDir,isView,isReqDet,weight,subName from old_FirSubjects";
    q.exec(s);
    s = "update FirSubjects set subSys=1,isView=1";
    q.exec(s);
    if(!db.commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FirSubjects”的数据时，提交事务失败！"));
        if(!db.rollback())
            QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FirSubjects”的数据时，事务回滚失败！"));
        return false;
    }

    //校对
    QString code,ncode;
    s = "select id,subCode from old_FirSubjects";
    r = q.exec(s);
    s = "select subCode from FirSubjects where id=:id";
    r = q2.prepare(s);
    while(q.next()){
        ok = true;
        id = q.value(0).toInt();
        code = q.value(1).toString();
        q2.bindValue(":id",id);
        if(!q2.exec())
            ok = false;
        if(!q2.first())
            ok = false;
        ncode = q2.value(0).toString();
        if(QString::compare(code,ncode) != 0)
            ok = false;
        if(!ok){
            QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在校对表“FirSubjects”的数据时，发现数据的不一致！"));
            break;
        }
    }
    //设置哪些科目要使用外币
    QStringList codes;
    codes<<"1002"<<"1131"<<"2121"<<"1151"<<"2131";
    s = "update FirSubjects set isUseWb=1 where subCode=:code";
    r = q.prepare(s);
    for(int i = 0; i < codes.count(); ++i){
        q.bindValue(":code", codes.at(i));
        if(!q.exec())
            return false;
    }
    s = "update FirSubjects set isUseWb=0 where subCode!='1002' and subCode!='1131' "
            "and subCode!='2121' and subCode!='1151' and subCode!='2131'";
    r = q.exec(s);
    s = "update FirSubjects set weight=1";
    r = q.exec(s);

    //删除表
    s = "delete from old_FirSubjects";
    r = q.exec(s);
    s = "drop table old_FirSubjects";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在删除表“old_FirSubjects”表时发生错误，请先退出应用，使用专门工具删除它！"));
    }

    //4、修改一级科目类别表结构，增加科目系统类型字段“subSys”
    s = "alter table FstSubClasses rename to old_FstSubClasses";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在重命名表“FstSubClasses”时发生错误！"));
        return false;
    }
    s = "create table FstSubClasses(id integer primary key, subSys integer, code integer, name text)";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在创建表“FstSubClasses”时发生错误！"));
        return false;
    }
    s = "insert into FstSubClasses(subSys,code,name) select 1 as subSys,code,name from old_FstSubClasses";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FstSubClasses”的数据时发生错误！"));
        return false;
    }
    s = "delete from old_FstSubClasses";
    r = q.exec(s);
    s = "drop table old_FstSubClasses";
    if(!q.exec(s))
        QMessageBox::critical(0,QObject::tr("更新错误"),
                              QObject::tr("在删除表“old_FstSubClasses”表时发生错误，请先退出应用，使用专门工具删除它！"));


    //5、创建帐套表accountSuites，从accountInfo表内读取有关帐套的数据进行初始化
    s = "create table accountSuites(id integer primary key, year integer, subSys integer, isCurrent integer, lastMonth integer, name text)";
    if(!q.exec(s)){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("创建帐套表accountSuites时发生错误！"));
        return false;
    }
    //读取帐套
    s = "select value from AccountInfo where code=12";
    if(!q.exec(s))
        return false;
    if(!q.first())
        return false;
    QStringList sl = q.value(0).toString().split(",");
    //每2个元素代表一个帐套年份与帐套名
    QList<int> sYears; QList<QString> sNames;
    for(int i = 0; i < sl.count(); i+=2){
        sYears<<sl.at(i).toInt();
        sNames<<sl.at(i+1);
    }
    s = "insert into accountSuites(year,subSys,isCurrent,lastMonth,name) values(:year,1,0,1,:name)";
    r = q.prepare(s);
    for(int i = 0; i < sYears.count(); ++i){
        q.bindValue("year",sYears.at(i));
        q.bindValue("name",sNames.at(i));
        r = q.exec();
    }
    s = "select value from AccountInfo where code=11";
    r = q.exec(s);
    r = q.first();
    int curY = q.value(0).toInt();
    s = QString("update accountSuites set isCurrent=1 where year=%1").arg(curY);
    r = q.exec(s);
    s = QString("delete from AccountInfo where code=11 or code=12");
    r = q.exec(s);
    s = "drop table AccountInfos";
    r = q.exec(s);

    QMessageBox::information(0,QObject::tr("更新成功"),
                                 QObject::tr("账户文件格式成功更新到1.3版本！"));
    return setVersion(1,3);
}

/**
 * @brief VMAccount::updateTo1_4
 *  任务描述：
 *  1、删除无用表
 *
 *  2、创建新余额表
 *  创建保存余额相关的表，5个新表（SEPoint、SE_PM_F,SE_MM_F,SE_PM_S,SE_MM_S）
 *  （1）余额指针表（SEPoint）
 *      包含字段年、月、币种（id，year，month，mt），唯一地指出了所属凭证年月和币种
 *  （2）所有与新余额相关的表以SE开头，表示科目余额。
 *      其中：PM指代原币新式、MM指代本币形式、F指代一级科目、S指代二级科目
 * @return
 */
bool VMAccount::updateTo1_4()
{
    QSqlQuery q(db);

    //1、删除无用表
    QStringList tables;
    tables<<"AccountBookGroups"<<"ReportStructs"<<"ReportAdditionInfo"<<"CashDailys"
         <<"PzClasses"<<"AccountInfos"<<"old_SecSubjects"<<"old_FstSubClasses"
        <<"old_FSAgent"<<"old_FirSubjects";
    QString s;bool r;
    foreach(QString tname, tables){
        s = QString("drop table %1").arg(tname);
        r = q.exec(s);
    }

    //2、创建新余额表
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 integer)")
            .arg(tbl_nse_point).arg(fld_sep_year).arg(fld_sep_month).arg(fld_sep_mt);
    if(!q.exec(s))
        return false;
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 real,%5 integer)")
            .arg(tbl_nse_p_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value).arg(fld_nse_dir);
    if(!q.exec(s))
        return false;
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 real)")
            .arg(tbl_nse_m_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value);
    if(!q.exec(s))
        return false;
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 real,%5 integer)")
            .arg(tbl_nse_p_s).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value).arg(fld_nse_dir);
    if(!q.exec(s))
        return false;
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 real)")
            .arg(tbl_nse_m_s).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value);
    if(!q.exec(s))
        return false;
    QMessageBox::information(0,QObject::tr("更新成功"),
                                 QObject::tr("账户文件格式成功更新到1.4版本！"));
    return setVersion(1,4);
}

/**
 * @brief VMAccount::updateTo1_5
 *  任务描述：
 *  1、
 *  2、导入新科目系统的科目
 * @return
 */
bool VMAccount::updateTo1_5()
{
    QSqlQuery q(db);
    QString s;

    //2、导入新科目系统的科目
    QSqlDatabase ndb = QSqlDatabase::addDatabase("QSQLITE","importNewSub");
    ndb.setDatabaseName("./datas/basicdatas/firstSubjects_2.dat");
    if(!ndb.open()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("不能打开新科目系统2的数据源表！，请检查文件夹下“datas/basicdatas/”下是否存在“firstSubjects_2.dat”文件"));
        return false;
    }
    QSqlQuery qm(ndb);
    if(!qm.exec("select * from FirstSubs")){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在提取新科目系统的数据时出错"));
        return false;
    }
    //(id,subCode,remCode,belongTo,jdDir,isView,isReqDet,weight,subName)
    s = "insert into FirSubjects(subSys,subCode,remCode,belongTo,jdDir,isView,isUseWb,weight,subName) "
            "values(2,:code,:remCode,:belongTo,:jdDir,:isView,:isUseWb,:weight,:name)";
    bool r = db.transaction();
    r = q.prepare(s);
    while(qm.next()){
        q.bindValue(":code",qm.value(2).toString());
        q.bindValue("remCode",qm.value(3).toString());
        q.bindValue(":belongTo",qm.value(4).toInt());
        q.bindValue(":jdDir",qm.value(5).toInt());
        q.bindValue(":isView",qm.value(6).toInt());
        q.bindValue(":isUseWb",qm.value(7).toInt());
        q.bindValue(":weight",qm.value(8).toInt());
        q.bindValue(":name",qm.value(9).toString());
        q.exec();
    }
    if(!db.commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在导入新科目时，提交事务失败！"));
        return false;
    }
    if(!qm.exec("select * from FirstSubCls where subCls=2")){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在提取新科目系统科目类别的数据时出错"));
        return false;
    }
    s = "insert into FstSubClasses(subSys,code,name) values(2,:code,:name)";
    db.transaction();
    r = q.prepare(s);
    while(qm.next()){
        q.bindValue(":code",qm.value(2).toInt());
        q.bindValue(":name",qm.value(3).toString());
        q.exec();
    }
    if(!db.commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在导入新科目类别时，提交事务失败！"));
        return false;
    }

    QMessageBox::information(0,QObject::tr("更新成功"),QObject::tr("账户文件格式成功更新到1.5版本！"));
    return setVersion(1,5);
}

/**
 * @brief VMAccount::updateTo1_6
 * @return
 */
bool VMAccount::updateTo1_6()
{
    //1、创建转移记录表，转移记录描述表
    //2、为了使账户可以编辑，初始化一条转移记录（在系统当前时间，由本机转出并转入的转移记录）
    QSqlQuery q(db);
    QString s = "create table transfers(id integer primary key, smid integer, dmid integer, state integer, outTime text, inTime text)";
    if(!q.exec(s))
        return false;
    s = "create table transferDescs(id integer primary key, tid integer, outDesc text, inDesc text)";
    if(!q.exec(s))
        return false;
    int mid = AppConfig::getInstance()->getLocalMid();
    QString curTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    s = QString("insert into transfers(smid,dmid,state,outTime,inTime) values(%1,%2,%3,'%4','%5')")
            .arg(mid).arg(mid).arg(Account::ATS_TranInDes).arg(curTime).arg(curTime);
    if(!q.exec(s))
        return false;
    s = "select last_insert_rowid()";
    if(!q.exec(s) || !q.first())
        return false;
    int tid = q.value(0).toInt();
    QString outDesc = QObject::tr("默认初始由本机转出");
    QString inDesc = QObject::tr("默认初始由本机转入");
    s = QString("insert into transferDescs(tid,outDesc,inDesc) values(%1,'%2','%3')")
            .arg(tid).arg(outDesc).arg(inDesc);
    if(!q.exec(s))
        return false;
    QMessageBox::information(0,QObject::tr("更新成功"),
                                 QObject::tr("账户文件格式成功更新到1.6版本！"));
    return setVersion(1,6);
}


bool VMAppConfig::perfectVersion()
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

bool VMAppConfig::getVersion(int &mv, int &sv)
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

bool VMAppConfig::setVersion(int mv, int sv)
{
    QSqlQuery q(db);
    QString s = QString("update version set master=%1,second=%2").arg(mv).arg(sv);
    return q.exec(s);
}

/**
 * @brief VMAppConfig::updateTo1_1
 *  升级任务描述：
 *  1、用新的简化版凭证集状态替换原先相对复杂的
 * @return
 */
bool VMAppConfig::updateTo1_1()
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
 * @brief VMAppConfig::updateTo1_2
 *  添加日志级别的设置字段
 * @return
 */
bool VMAppConfig::updateTo1_2()
{
    appIni->beginGroup("Debug");
    appIni->setValue("loglevel", Logger::levelToString(Logger::Debug));
    appIni->endGroup();
    return setVersion(1,2);
}

bool VMAppConfig::updateTo1_3()
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

bool VMAppConfig::updateTo2_0()
{
    return true;
}
