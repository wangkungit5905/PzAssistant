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
#include "subject.h"

QSqlDatabase* Account::db;

Account::Account(QString fname)
{
    curPzSet = NULL;
	isReadOnly = false;
    accInfos.fileName = fname;
    if(!init())
        QMessageBox::critical(0,QObject::tr("错误信息"),QObject::tr("账户在初始化时发生错误！"));

}

Account::~Account()
{
    qDeleteAll(smgs);
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

void Account::addWaiMt(Money* mt)
{
    if(!accInfos.waiMts.contains(mt))
        accInfos.waiMts<<mt;
}

void Account::delWaiMt(Money* mt)
{
    if(accInfos.waiMts.contains(mt))
        accInfos.waiMts.removeOne(mt);
}

//获取所有外币的名称列表，用逗号分隔
QString Account::getWaiMtStr()
{
    QString t;
    foreach(Money* mt, accInfos.waiMts)
        t.append(mt->name()).append(",");
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
    foreach(AccountSuiteRecord* as, accInfos.suites){
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
    AccountSuiteRecord* as = new AccountSuiteRecord;
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
    foreach(AccountSuiteRecord* as, accInfos.suites){
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
 Account::AccountSuiteRecord* Account::getCurSuite()
 {
     foreach(AccountSuiteRecord* as, accInfos.suites)
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
    foreach(AccountSuiteRecord* as, accInfos.suites)
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
    foreach(AccountSuiteRecord* as, accInfos.suites){
        if(as->year == y)
            return as->startMonth;
    }
    return 0;
}

//获取当前帐套的结束月份
int Account::getSuiteLastMonth(int y)
{
    foreach(AccountSuiteRecord* as, accInfos.suites){
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
    foreach(AccountSuiteRecord* as, accInfos.suites){
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
    foreach(AccountSuiteRecord* as, accInfos.suites){
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

/**
 * @brief Account::getSubjectManager
 *  返回指定科目系统的科目管理器对象
 * @param subSys
 * @return
 */
SubjectManager *Account::getSubjectManager(int subSys)
{
    if(!smgs.contains(subSys))
        smgs[subSys] = new SubjectManager(this,subSys);
    return smgs.value(subSys);
}

bool Account::getRates(int y, int m, QHash<int, Double> &rates)
{
    return dbUtil->getRates(y,m,rates);
}

bool Account::setRates(int y, int m, QHash<int, Double> &rates)
{
    return dbUtil->saveRates(y,m,rates);
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
    accInfos.masterMt = NULL;

    dbUtil = new DbUtil;
    if(!dbUtil->setFilename(accInfos.fileName)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Can't connect to account file!"));
        return false;
    }
    if(!dbUtil->initMoneys(this)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Money init happen error!"));
        return false;
    }
    if(!dbUtil->initAccount(accInfos)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Account info init happen error!"));
        return false;
    }    
    if(!dbUtil->initNameItems()){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Name items init happen error!"));
        return false;
    }
    if(!dbUtil->initBanks(this)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Bank init happen error!"));
        return false;
    }
    return true;
}


//获取帐套内凭证集的开始、结束月份
bool Account::getSuiteMonthRange(int y,int& sm, int &em)
{
    foreach(AccountSuiteRecord* as, accInfos.suites){
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
    foreach(AccountSuiteRecord* as, accInfos.suites){
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
bool byAccountSuiteThan(Account::AccountSuiteRecord *as1, Account::AccountSuiteRecord *as2)
{
    return as1->year < as2->year;
}


bool Account::AccountSuiteRecord::operator !=(const AccountSuiteRecord& other)
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
