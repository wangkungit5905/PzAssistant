#include <QVariant>

#include "account.h"
#include "common.h"
#include "global.h"
#include "utils.h"
#include "pz.h"
#include "tables.h"
#include "dbutil.h"
#include "logs/Logger.h"
#include "version.h"

QSqlDatabase* Account::db;

Account::Account(QString fname)
{
    curPzSet = NULL;
	isReadOnly = false;
    accInfos.fileName = fname;
    dbUtil = new DbUtil;
    if(!dbUtil->setFilename(fname)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Can't connect to account file!"));
        return;
    }
    if(!dbUtil->initAccount(accInfos)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Account init happen error!"));
        return;
    }
    subMng = new SubjectManager(subType,*db);


}

Account::~Account()
{
    delete subMng;
}

bool Account::isValid()
{
    if(accInfos.code.isEmpty() ||
       accInfos.sname.isEmpty() ||
       accInfos.lname.isEmpty())
        return false;
    else
        return true;
}

void Account::close()
{
    dbUtil->saveAccountInfo(accInfos);
    dbUtil->close();
}

void Account::addWaiMt(int mt)
{
    if(!accInfos.waiMts.contains(mt))
        accInfos.waiMts<<mt;
}

void Account::delWaiMt(int mt)
{
    if(accInfos.waiMts.contains(mt))
        accInfos.waiMts.removeOne(mt);
}

//获取所有外币的名称列表，用逗号分隔
QString Account::getWaiMtStr()
{
    QString t;
    foreach(int mt, accInfos.waiMts)
        t.append(MTS.value(mt)).append(",");
    if(!t.isEmpty())
        t.chop(1);
    return t;
}

/**
 * @brief Account::appendSuite
 *  添加帐套
 * @param y         帐套年份
 * @param name      帐套名
 * @param curMonth  帐套最近打开凭证集所属月份
 * @param subSys    帐套采用的科目系统代码
 */
void Account::appendSuite(int y, QString name, int curMonth,int subSys)
{
    foreach(AccountSuite* as, accInfos.suites){
        if(as->year == y){
            as->name = name;
            as->lastMonth = curMonth;
            as->subSys = subSys;
            as->isCur = true;
            return;
        }
        else
            as->isCur = false;
    }
    AccountSuite* as = new AccountSuite;
    as->id = 0; as->year = y; as->name = name;
    as->isCur = true; as->lastMonth = curMonth;
    as->startMonth = 1; as->endMonth = 1;as->subSys = subSys;
    int i = 0;
    while(i < accInfos.suites.count() && y > accInfos.suites.at(i)->year)
        i++;
    accInfos.suites.insert(i,as);
}

void Account::setSuiteName(int y, QString name)
{
    foreach(AccountSuite* as, accInfos.suites){
        if(as->year == y)
            as->name = name;
    }
}

QList<int> Account::getSuites()
{
    QList<int> ys;
    for(int i = 0; i < accInfos.suites.count(); ++i)
        ys<<accInfos.suites.at(i)->year;
    return ys;
}

/**
 * @brief Account::getCurSuite
 *  获取最近打开的帐套
 * @return
 */
 Account::AccountSuite* Account::getCurSuite()
 {
     foreach(AccountSuite* as, accInfos.suites)
         if(as->isCur)
             return as;
     if(!accInfos.suites.empty()){
         accInfos.suites.last()->isCur = true;
         return accInfos.suites.last();
     }
     return NULL;
 }

/**
 * @brief Account::setCurSuite
 *  设置最近打开的帐套
 * @param y
 */
void Account::setCurSuite(int y)
{
    foreach(AccountSuite* as, accInfos.suites)
        if(as->year == y)
            as->isCur = true;
}

void Account::delSuite(int y)
{
//    foreach(AccountSuite* as, accInfos.suites){
//        if(as->year == y){
//            delete as;
//            accInfos.suites.removeOne(as);
//        }
//    }
//    if(!accInfos.suites.isEmpty()){
//        accInfos.startSuite = accInfos.suites.first()->year;
//        accInfos.endSuite = accInfos.suites.last()->year;
//    }
//    else{
//        accInfos.startSuite = 0;
//        accInfos.endSuite = 0;
//    }
}

/**
 * @brief Account::getSuiteName
 *  获取帐套名
 * @param y
 * @return
 */
QString Account::getSuiteName(int y)
{
    for(int i = 0; i < accInfos.suites.count(); ++i){
        if(y == accInfos.suites.at(i)->year)
            return accInfos.suites.at(i)->name;
    }
}

//获取当前帐套的起始月份
int Account::getSuiteFirstMonth(int y)
{
    foreach(AccountSuite* as, accInfos.suites){
        if(as->year == y)
            return as->startMonth;
    }
    return 0;
}

//获取当前帐套的结束月份
int Account::getSuiteLastMonth(int y)
{
    foreach(AccountSuite* as, accInfos.suites){
        if(as->year == y)
            return as->endMonth;
    }
    return 0;
}

/**
 * @brief Account::setCurMonth
 *  设置当前帐套的当前打开的凭证集的月份
 * @param m
 */
void Account::setCurMonth(int m)
{
    foreach(AccountSuite* as, accInfos.suites){
        if(as->isCur)
            as->lastMonth = m;
    }
}

/**
 * @brief Account::getCurMonth
 *  获取当前帐套的最近打开凭证集的月份
 * @return
 */
int Account::getCurMonth()
{
    foreach(AccountSuite* as, accInfos.suites){
        if(as->isCur)
            return as->lastMonth;
    }
    return 0;
}

//获取账户期初年份（即开始记账的前一个月所处的年份）
int Account::getBaseYear()
{
    QDate d = QDate::fromString(accInfos.startDate,Qt::ISODate);
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
    QDate d = QDate::fromString(accInfos.startDate,Qt::ISODate);
    int m = d.month();
    if(m == 1)
        return 12;
    else
        return m-1;
}


//获取凭证集对象
PzSetMgr* Account::getPzSet()
{
    //if(accInfos.suiteLastMonths.value(accInfos.curSuite) == 0)
    //    return NULL;
    //if(!curPzSet)
    //    curPzSet = new PzSetMgr(accInfos.curSuite,accInfos.suiteLastMonths.value(accInfos.curSuite),user,*db);
    //return curPzSet;
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
 * @brief Account::init
 *  账户对象初始化
 * @return
 */
bool Account::init()
{
}


//获取帐套内凭证集的开始、结束月份
bool Account::getSuiteMonthRange(int y,int& sm, int &em)
{
    foreach(AccountSuite* as, accInfos.suites){
        if(as->year == y){
            sm = as->startMonth;
            em = as->endMonth;
            return true;
        }
    }
    return false;
}

bool Account::containSuite(int y)
{
    foreach(AccountSuite* as, accInfos.suites){
        if(as->year == y)
            return true;
    }
    return false;
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

////////////////////////////////////////////////////////////////
bool byAccountSuiteThan(Account::AccountSuite *as1, Account::AccountSuite *as2)
{
    return as1->year < as2->year;
}


bool Account::AccountSuite::operator !=(const AccountSuite& other)
{
    if(year != other.year)
        return true;
    else if(lastMonth != other.lastMonth)
        return true;
    else if(subSys != other.subSys)
        return true;
    else if(name != other.name)
        return true;
    else if(isCur != other.isCur)
        return true;
    return false;
}
