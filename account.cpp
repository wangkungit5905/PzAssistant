#include <QVariant>

#include "account.h"
#include "common.h"
#include "global.h"
#include "utils.h"
#include "pz.h"
#include "tables.h"

QSqlDatabase* Account::db;

Account::Account(QString fname/*, QSqlDatabase db*/):fileName(fname)/*,db(db)*/
{
    curSuite = 0;
    curMonth = 0;
    masterMt = 0;
    subType = SubjectManager::SUBT_OLD;
    curPzSet = NULL;
	isReadOnly = false;

//    //确定账户文件版本号，这些是临时代码，待所有账户文件都升级以后可以不要执行
//    //一致将版本号归为“1.2”或更高
//    //首先确定是否存在AccountInfo表
//    QString s;
//    QSqlQuery q(*db);
//    q.exec("select * from sqlite_master where tbl_name='AccountInfo'");
//    if(!q.first())
//        crtAccountTable();
//    q.exec("select * from sqlite_master where tbl_name='gdzcs'");
//    if(!q.first())
//        crtGdzcTable();
//    q.exec("select value from AccountInfo where name='db_version'");
//    if(!q.first()){
//        insertVersion();
//        db_version = "1.2";
//    }
//    else
//        db_version = q.value(0).toString();
    ///////////////////////////////////

    QSqlQuery q(*db);
    bool r = q.exec("select code,value from AccountInfo");
    if(r){
        InfoField code;
        QStringList sl;
        while(q.next()){
            code = (InfoField)q.value(0).toInt();
            switch(code){
            case ACODE:
                accCode = q.value(1).toString();
                break;
            case SNAME:
                sname = q.value(1).toString();
                break;
            case LNAME:
                lname = q.value(1).toString();
                break;
            case FNAME:
                fileName = q.value(1).toString();
                break;
            case SUBTYPE:
                subType = (SubjectManager::SubjectSysType)q.value(1).toInt();
                break;
            case MASTERMT:
                masterMt = q.value(1).toInt();
                break;
            case WAIMT:
                sl.clear();
                sl = q.value(1).toString().split(",");
                foreach(QString v, sl){
                    waiMt<<v.toInt();
                }
                break;
            case STIME:
                startTime = q.value(1).toString();
                break;
            case ETIME:
                endTime = q.value(1).toString();
                break;
            case CSUITE:
                curSuite = q.value(1).toInt();
                break;
            case SUITENAME:
                sl.clear();
                sl = q.value(1).toString().split(",");
                //每2个元素代表一个帐套年份与帐套名
                for(int i = 0; i < sl.count(); i+=2){
                    suiteNames[sl[i].toInt()] = sl[i+1];
                }
                break;
            case LOGFILE:
                logFileName = q.value(1).toString();
                break;
            }
        }

        //如果表内的信息字段内容不全，则需要提供默认值，以使对象的属性具有意义
        if(masterMt == 0)
            masterMt = RMB;
        //if(fileName.isEmpty())
        //    fileName = ; //此处应该用某种方法获取账户的文件名
        //默认，日志文件名同账户文件名同名，但扩展名不同
        if(logFileName.isEmpty()){
            logFileName = fileName;
            logFileName = logFileName.replace(".dat",".log");
        }
        if(subType == 0)
            subType = SubjectManager::SUBT_OLD;
        if(startTime.isEmpty()){
            QDate d = QDate::currentDate();
            int y = d.year();
            int m = d.month();
            int sd = 1;
            int ed = d.daysInMonth();
            startTime = QDate(y,m,sd).toString(Qt::ISODate);
            endTime = QDate(y,m,ed).toString(Qt::ISODate);
        }
        if(endTime.isEmpty()){
            QDate d = QDate::fromString(startTime,Qt::ISODate);
            int days = d.daysInMonth();
            d.setDate(d.year(),d.month(),days);
            endTime = d.toString(Qt::ISODate);
        }

        QList<int> suites = suiteNames.keys();
        if(!suites.empty()){
            qSort(suites.begin(),suites.end());
            startSuite = suites[0];
            endSuite = suites[suites.count() - 1];
            if(curSuite == 0)
                curSuite = endSuite;
        }
        else{
            startSuite = 0;
            endSuite = 0;
        }
    }

    subMng = new SubjectManager(subType,*db);
}

Account::~Account()
{
    delete subMng;
}

bool Account::isValid()
{
    if(accCode.isEmpty() ||
       sname.isEmpty() ||
       lname.isEmpty() ||
       fileName.isEmpty())
        return false;
    else
        return true;

//    if(accCode.isEmpty())
//        return false;
//    else if(sname.isEmpty())
//        return false;
//    else if(lname.isEmpty())
//        return false;
//    else if(fileName.isEmpty())
//        return false;
//    else
//        return true;
}

void Account::setWaiMt(QList<int> mts)
{
    QString s;
    waiMt = mts;
    for(int i = 0; i < waiMt.count(); ++i)
        s.append(QString::number(waiMt[i])).append(",");
    s.chop(1);
    savePiece(WAIMT,s);
}

void Account::addWaiMt(int mt)
{
    if(!waiMt.contains(mt))
        waiMt<<mt;
}

void Account::delWaiMt(int mt)
{
    if(waiMt.contains(mt))
        waiMt.removeOne(mt);
}

//获取所有外币的名称列表，用逗号分隔
QString Account::getWaiMtStr()
{
    QString t;
    foreach(int mt, waiMt)
        t.append(MTS.value(mt)).append(",");
    if(!t.isEmpty())
        t.chop(1);
    return t;
}

void Account::appendSuite(int y, QString name)
{
    if(!suiteNames.contains(y)){
        suiteNames[y] = name;
        savePiece(SUITENAME,assembleSuiteNames());
        if(y < startSuite)
            startSuite = y;
        if(y > endSuite)
            endSuite = y;
    }
}

void Account::setSuiteName(int y, QString name)
{
    if(suiteNames.contains(y)){
        suiteNames[y] = name;
        savePiece(SUITENAME,assembleSuiteNames());
    }
}

void Account::delSuite(int y)
{
    if(suiteNames.contains(y)){
        suiteNames.remove(y);
        savePiece(SUITENAME,assembleSuiteNames());
        QList<int> yl;
        yl = suiteNames.keys();
        qSort(yl.begin(),yl.end());
        startSuite = yl[0];
        endSuite = yl[yl.count()-1];
    }
}

//获取当前帐套的起始月份
int Account::getSuiteFirstMonth(int y)
{
    if(y < startSuite || y > endSuite)
        return 0;
    if(y == startSuite)
        return QDate::fromString(startTime,Qt::ISODate).month();
    else
        return 1;
}

//获取当前帐套的结束月份
int Account::getSuiteLastMonth(int y)
{
    if(y < startSuite || y > endSuite)
        return 0;
    if(y == endSuite)
        return QDate::fromString(endTime,Qt::ISODate).month();
    else
        return 12;
}

//获取账户期初年份（即开始记账的前一个月所处的年份）
int Account::getBaseYear()
{
    QDate d = QDate::fromString(startTime,Qt::ISODate);
    int m = d.month();
    int y = d.year();
    if(m == 1)
        return y-1;
    else
        return y;
}

//获取账户期初月份
int Account::getBaseMonth()
{
    QDate d = QDate::fromString(startTime,Qt::ISODate);
    int m = d.month();
    if(m == 1)
        return 12;
    else
        return m-1;
}


//获取凭证集对象
PzSetMgr* Account::getPzSet()
{
    if(curMonth == 0)
        return NULL;
    if(!curPzSet)
        curPzSet = new PzSetMgr(curSuite,curMonth,user,*db);
    return curPzSet;
}

//关闭凭证集
void Account::colsePzSet()
{
    curPzSet->close();
    delete curPzSet;
    curPzSet = NULL;
}

void Account::setDatabase(QSqlDatabase *db)
{Account::db=db;}

/**
 * @brief Account::versionMaintain
 *  账户模块版本维护
 * @param cancel
 * @return
 */
bool Account::versionMaintain(bool &cancel)
{
    VersionManager* vm = new VersionManager(VersionManager::MT_ACC);
    //每当有新的可用升级函数时，就在此添加
    vm->appendVersion(1,3,&Account::updateTo1_3);
    //vm->appendVersion(1,4,&Account::updateTo1_4);
    //vm->appendVersion(1,5,&Account::updateTo1_5);
    bool r = vm->versionMaintain(cancel);
    delete vm;
    return r;

}


/**
 * @brief Account::getVersion
 *  获取账户数据库版本
 * @param mv
 * @param sv
 * @return
 */
bool Account::getVersion(int &mv, int &sv)
{
    QSqlQuery q(*db);
    QString s = QString("select %1,%2 from %3 where %4=%5")
            .arg(fld_acc_name).arg(fld_acc_value).arg(tbl_account)
            .arg(fld_acc_code).arg(DBVERSION);
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
 * @brief Account::setVersion
 *  设置账户数据库版本
 * @param mv
 * @param sv
 * @return
 */
bool Account::setVersion(int mv, int sv)
{
    QSqlQuery q(*db);
    QString verStr = QString("%1.%2").arg(mv).arg(sv);
    QString s = QString("update %1 set %2=%3 where %4=%5").arg(tbl_account).arg(fld_acc_value)
            .arg(verStr).arg(fld_acc_code).arg(DBVERSION);
    if(!q.exec(s))
        return false;
    return true;
}

/**
 * @brief Account::updateTo1_3
 *  升级任务：
 *  1、创建名称条目表“nameItems”替换“SecSubjects”表，添加创建时间列，创建者
 *  2、修改FSAgent表，添加创建（启用）时间列、创建者、禁用时间列
 *  3、修改FirSubjects表，添加科目系统（subSys）
 *  4、修改一级科目类别表结构，增加科目系统类型字段“subSys”
 * @return
 */
bool Account::updateTo1_3()
{
    QSqlQuery q(*db);
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
    if(!db->transaction()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“SecSubjects”的数据时，启动事务失败！"));
        return false;
    }
    s = "insert into nameItems(id,sName,lName,remCode,classId,creator) "
            "select id,subName,subLName,remCode,classId,1 as user from old_SecSubjects";
    q.exec(s);
    if(!db->commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“SecSubjects”的数据时，提交事务失败！"));
        if(!db->rollback())
            QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“SecSubjects”的数据时，事务回滚失败！"));
        return false;
    }

    //进行校对
    QSqlQuery q2(*db);
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
    if(!db->transaction()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FSAgent”的数据时，启动事务失败！"));
        return false;
    }
    s = "insert into FSAgent(id,fid,sid,subCode,weight,isEnabled,creator) select id,fid,sid,subCode,FrequencyStat,isEnabled,1 as user from old_FSAgent";
    r = q.exec(s);
    s = "update FSAgent set isEnabled=1,weight=1";
    r = q.exec(s);
    if(!db->commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FSAgent”的数据时，提交事务失败！"));
        if(!db->rollback())
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
    if(!db->transaction()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FirSubjects”的数据时，启动事务失败！"));
        return false;
    }
    s = "insert into FirSubjects(id,subCode,remCode,belongTo,jdDir,isView,isUseWb,weight,subName) select id,subCode,remCode,belongTo,jdDir,isView,isReqDet,weight,subName from old_FirSubjects";
    q.exec(s);
    s = "update FirSubjects set subSys=1,isView=1";
    q.exec(s);
    if(!db->commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在转移表“FirSubjects”的数据时，提交事务失败！"));
        if(!db->rollback())
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

    QMessageBox::information(0,QObject::tr("更新成功"),
                                 QObject::tr("账户文件格式成功更新到1.3版本！"));
    return setVersion(1,3);
}

/**
 * @brief Account::updateTo1_4
 *  任务描述：
 *  1、创建帐套表accountSuites，从accountInfo表内读取有关帐套的数据进行初始化
 *  2、导入新科目系统的科目
 * @return
 */
bool Account::updateTo1_4()
{
    QSqlQuery q(*db);
    QString s;

    //1、创建帐套表accountSuites，从accountInfo表内读取有关帐套的数据进行初始化
    s = "create table accountSuites(id integer primary key, year integer, subSys integer, isCurrent integer, name text)";
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
    s = "insert into accountSuites(year,subSys,isCurrent,name) values(:year,1,0,:name)";
    bool r = q.prepare(s);
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
    r = db->transaction();
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
    if(!db->commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在导入新科目时，提交事务失败！"));
        return false;
    }
    if(!qm.exec("select * from FirstSubCls where subCls=2")){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在提取新科目系统科目类别的数据时出错"));
        return false;
    }
    s = "insert into FstSubClasses(subSys,code,name) values(2,:code,:name)";
    db->transaction();
    r = q.prepare(s);
    while(qm.next()){
        q.bindValue(":code",qm.value(2).toInt());
        q.bindValue(":name",qm.value(3).toString());
        q.exec();
    }
    if(!db->commit()){
        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在导入新科目类别时，提交事务失败！"));
        return false;
    }

    QMessageBox::information(0,QObject::tr("更新成功"),QObject::tr("账户文件格式成功更新到1.5版本！"));
    return setVersion(1,4);
}

bool Account::updateTo1_5()
{
    //1、创建转移记录表，转移记录描述表
    //2、为了使账户可以编辑，初始化一条转移记录（在系统当前时间，由本机转出并转入的转移记录）
    QSqlQuery q(*db);
    QString s = "create table transfers(id integer primary key, smid integer, dmid integer, state integer, outTime text, inTime text)";
    if(!q.exec(s))
        return false;
    s = "create table transferDescs(id integer primary key, tid integer, outDesc text, inDesc text)";
    if(!q.exec(s))
        return false;
    int mid = AppConfig::getInstance()->getLocalMid();
    QString curTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    s = QString("insert into transfers(smid,dmid,state,outTime,inTime) values(%1,%2,%3,'%4','%5')")
            .arg(mid).arg(mid).arg(ATS_TranInDes).arg(curTime).arg(curTime);
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

    return setVersion(1,5);
}

//将帐套名哈希表装配成一个字符串形式
QString Account::assembleSuiteNames()
{
    QString s;
    QList<int> yl = suiteNames.keys();
    qSort(yl.begin(),yl.end());
    for(int i = 0; i < yl.count(); ++i)
        s.append(QString::number(yl[i])).append(",")
                .append(suiteNames.value(yl[i]))
                .append(",");
    s.chop(1);
    return s;
}


//void Account::saveAll()
//{
//    QSqlQuery iq,uq; //分别用于插入和更新
//    bool r = q.exec("delete from AccountInfo");
//    r = q.prepare("insert into AccountInfo(code,name,value) values(:code,:name,:value)");
//    //uq.prepare("update AccountInfo set value = :value where code = :code");
//    q.bindValue(":code",ACODE);
//    q.bindValue(":name","accountCode");
//    q.bindValue(":value,", accCode);
//    q.exec();
//    q.bindValue(":code",SNAME);
//    q.bindValue(":name","shortName");
//    q.bindValue(":value,", sname);
//    q.exec();
//    q.bindValue(":code",LNAME);
//    q.bindValue(":name","longName");
//    q.bindValue(":value,", LNAME);
//    q.exec();
//    q.bindValue(":code",FNAME);
//    q.bindValue(":name","fileName");
//    q.bindValue(":value,", fileName);
//    q.exec();
//    q.bindValue(":code",SUBTYPE);
//    q.bindValue(":name","subType");
//    q.bindValue(":value,", QString::number(subType));
//    q.exec();
//    q.bindValue(":code",MASTERMT);
//    q.bindValue(":name","masterMt");
//    q.bindValue(":value,", QString::number(masterMt));
//    q.exec();
//    QString l;
//    for(int i = 0; i < waiMt.count(); ++i)
//        l.append(QString::number(waiMt[i])).append(",");
//    l.chop(1);
//    q.bindValue(":code",WAIMT);
//    q.bindValue(":name","WaiBiList");
//    q.bindValue(":value,", l);
//    q.exec();
//    q.bindValue(":code",STIME);
//    q.bindValue(":name","startTime");
//    q.bindValue(":value,", startTime);
//    q.exec();
//    q.bindValue(":code",ETIME);
//    q.bindValue(":name","endTime");
//    q.bindValue(":value,", endTime);
//    q.exec();
//    q.bindValue(":code",SSUITE);
//    q.bindValue(":name","startSuite");
//    q.bindValue(":value,", QString::number(startSuite));
//    q.exec();
//    q.bindValue(":code",ESUITE);
//    q.bindValue(":name","endSuite");
//    q.bindValue(":value,", QString::number(endSuite));
//    q.exec();
//    q.bindValue(":code",CSUITE);
//    q.bindValue(":name","currentSuite");
//    q.bindValue(":value,", QString::number(curSuite));
//    q.exec();
////    QList<int> sl = suiteNames.keys();
////    l.clear();
////    qSort(sl.begin(),sl.end());
////    for(int i = 0; i < sl.count(); ++i)
////        l.append(QString::number(sl[i])).append(",")
////        .append(suiteNames.value(sl[i]));
//    l = assembleSuiteNames();
//    q.bindValue(":code",SUITENAME);
//    q.bindValue(":name","suiteName");
//    q.bindValue(":value,", l);
//    q.exec();
//    q.bindValue(":code",LOGFILE);
//    q.bindValue(":name","logFileName");
//    q.bindValue(":value,", logFileName);
//    q.exec();

//}

//保存指定部分的账户信息
void Account::savePiece(InfoField f, QString v)
{
    QSqlQuery q(*db);
    QString s = QString("select id from AccountInfo where code=%1").arg(f);
    if(q.exec(s) && q.first())
        s = QString("update AccountInfo set value='%1' where code=%2")
                .arg(v).arg(f);
    else{
        QString name;
        switch(f){
        case ACODE:
            name = "accountCode";
            break;
        case SNAME:
            name = "shortName";
            break;
        case LNAME:
            name = "longName";
            break;
        case FNAME:
            name = "fileName";
            break;
        case SUBTYPE:
            name = "subType";
            break;
        case RPTTYPE:
            name = "reportType";
            break;
        case MASTERMT:
            name = "masterMt";
            break;
        case WAIMT:
            name = "WaiBiList";
            break;
        case STIME:
            name = "startTime";
            break;
        case ETIME:
            name = "endTime";
            break;
        case CSUITE:
            name = "currentSuite";
            break;
        case SUITENAME:
            name = "suiteNames";
            break;
        case LOGFILE:
            name = "logFileName";
            break;
        case LASTACCESS:
            name = "lastAccessTime";
            break;
        }
        s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
                .arg(f).arg(name).arg(v);

    }
    bool r = q.exec(s);
    int i = 0;
}

//获取帐套内凭证集的开始、结束月份
bool Account::getSuiteMonthRange(int y,int& sm, int &em)
{
    if(!suiteNames.contains(y))
        return false;

    QDate sd = QDate::fromString(startTime,Qt::ISODate);
    QDate ed = QDate::fromString(endTime,Qt::ISODate);

    if((y > sd.year()) && (y < ed.year())){
        sm = 1;em = 12;
    }
    else if((y == sd.year() && (y < ed.year()))){
        sm = sd.month(); em = 12;
    }
    else if((y > sd.year()) && (y == ed.year())){
        sm = 1; em = ed.month();
    }
    else{
        sm = sd.month(); em = ed.month();
    }
    return true;


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


//临时，创建账户信息表
void Account::crtAccountTable()
{
//    QSqlQuery q(*db),q1(*db);
//    bool r;
//    QString s;

//    //创建新的账户信息表，将老的账户信息表内的数据转入新表
//    r = q.exec("CREATE TABLE AccountInfo(id INTEGER PRIMARY KEY, code int, name TEXT, value TEXT)");
//    //转储老的账户信息表
//    r = q.exec("select * from AccountInfos");
//    r = q.first();
//    //账户代码
//    QString ts = q.value(ACCOUNT_CODE).toString();
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::ACODE).arg("accountCode").arg(ts);
//    r = q1.exec(s);
//    //账户开始记账时间
//    ts = q.value(ACCOUNT_BASETIME).toString().append("-01");
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::STIME).arg("startTime").arg(ts);
//    r = q1.exec(s);
//    //账户终止记账时间
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::ETIME).arg("endTime").arg("2011-11-30");
//    r = q1.exec(s);
//    //账户简称
//    ts = q.value(ACCOUNT_SNAME).toString();
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::SNAME).arg("shortName").arg(ts);
//    r = q1.exec(s);
//    //账户全称
//    ts = q.value(ACCOUNT_LNAME).toString();
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::LNAME).arg("longName").arg(ts);
//    r = q1.exec(s);
//    //账户所用科目类型
//    int v = q.value(ACCOUNT_USEDSUB).toInt();
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::SUBTYPE).arg("subType").arg(QString::number(v));
//    r = q1.exec(s);
//    //所用报表类型
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::RPTTYPE).arg("reportType").arg(QString::number(v));
//    r = q1.exec(s);
//    //本币
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::MASTERMT).arg("masterMt").arg("1");
//    r = q1.exec(s);
//    //外币
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::WAIMT).arg("WaiBiList").arg("2");
//    r = q1.exec(s);

//    //当前帐套
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::CSUITE).arg("currentSuite").arg("2011");
//    r = q1.exec(s);
//    //帐套名列表
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::SUITENAME).arg("suiteNames").arg(QObject::tr("2011,2011年"));
//    r = q1.exec(s);
//    //账户最后访问时间
//    ts = q.value(ACCOUNT_LASTTIME).toString();
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::LASTACCESS).arg("lastAccessTime").arg(ts);
//    r = q1.exec(s);
//    //日志文件名
//    //ts = curAccount->getFileName();
//    ts = fileName;
//    //ts.replace(".dat",".log");
//    ts.append(".log");
//    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
//            .arg(Account::LOGFILE).arg("logFileName").arg(ts);
//    r = q1.exec(s);


}

void Account::crtGdzcTable()
{
    QSqlQuery q1(*db);
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

//void Account::insertVersion()
//{
//    QSqlQuery q(db);
//    QString s;

//    s = QString("insert into AccountInfo(code,name,value) values("
//                "%1,'db_version','1.2')").arg(DBVERSION);
//    q.exec(s);
//}






////////////////////////////////////////////////////////////////

