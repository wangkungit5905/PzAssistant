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
#include "PzSet.h"

QSqlDatabase* Account::db;

Account::Account(QString fname)
{
    isReadOnly = false;
    accInfos.fileName = fname;
    if(!init())
        QMessageBox::critical(0,QObject::tr("错误信息"),QObject::tr("账户在初始化时发生错误！"));
    pzSetMgr = new PzSetMgr(this);
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
 * @brief Account::getStartTime
 *  获取账户开始记账时间
 * @return
 */
QDate Account::getStartTime()
{
    AccountSuiteRecord* asr = accInfos.suites.first();
    return QDate(asr->year,asr->startMonth,1);
}

/**
 * @brief Account::getEndTime
 *  获取账户结束记账时间
 * @return
 */
QDate Account::getEndTime()
{
    AccountSuiteRecord* asr = accInfos.suites.last();
    QDate d = QDate(asr->year,asr->endMonth,1);
    d.setDate(asr->year,asr->endMonth,d.daysInMonth());
    return d;
}

/**
 * @brief Account::setEndTime
 *  设置账户结束记账时间
 * @param date
 */
void Account::setEndTime(QDate date)
{
    AccountSuiteRecord* asr = accInfos.suites.last();
    if(asr->year == date.year())
        asr->endMonth = date.month();
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
            as->recentMonth = curMonth;
            as->subSys = subSys;
            as->isCur = true;
            return;
        }
        else
            as->isCur = false;
    }
    AccountSuiteRecord* as = new AccountSuiteRecord;
    as->id = 0; as->year = y; as->name = name;
    as->isCur = true; as->recentMonth = curMonth;
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
  * @brief Account::getSuite
  * 获取指定年份的帐套对象
  * @param y
  * @return
  */
 Account::AccountSuiteRecord *Account::getSuite(int y)
 {
     foreach(AccountSuiteRecord* as, accInfos.suites){
         if(as->year == y){
             return as;
         }
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
            as->recentMonth = m;
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
            return as->recentMonth;
    }
    return 0;
}

//获取账户期初年份（即开始记账的前一个月所处的年份）
int Account::getBaseYear()
{
    if(accInfos.suites.isEmpty())
        return 0;
    AccountSuiteRecord* asr = accInfos.suites.first();
    if(asr->startMonth == 1)
        return asr->year-1;
    else
        return asr->year;
}

//获取账户期初月份
int Account::getBaseMonth()
{
    if(accInfos.suites.isEmpty())
        return 0;
    AccountSuiteRecord* asr = accInfos.suites.last();
    if(asr->startMonth == 1)
        return 12;
    else
        return asr->startMonth - 1;
}

/**
 * @brief Account::getVersion
 *  获取账户文件的版本号
 * @param mv
 * @param sv
 */
void Account::getVersion(int &mv, int &sv)
{
    QStringList sl = accInfos.dbVersion.split(".");
    mv = sl.first().toInt();
    sv = sl.last().toInt();
}


//获取凭证集对象
//PzSetMgr* Account::getPzSet()
//{
    //if(accInfos.suiteLastMonths.value(accInfos.curSuite) == 0)
    //    return NULL;
    //if(!curPzSet)
    //    curPzSet = new PzSetMgr(accInfos.curSuite,accInfos.suiteLastMonths.value(accInfos.curSuite),user,*db);
    //return curPzSet;
//}

//关闭凭证集
void Account::colsePzSet()
{
    pzSetMgr->close();
    pzSetMgr = NULL;
}

/**
 * @brief Account::getSubjectManager
 *  返回指定科目系统的科目管理器对象
 * @param subSys
 * @return
 */
SubjectManager *Account::getSubjectManager(int subSys)
{
    int ssCode;
    if(subSys == 0)
        ssCode = getCurSuite()->subSys;
    else
        ssCode = subSys;
    if(!smgs.contains(ssCode))
        smgs[ssCode] = new SubjectManager(this,ssCode);
    return smgs.value(ssCode);
}

//SubjectManager *Account::getSubjectManager()
//{
//    int subSys = getCurSuite()->subSys;
//    return getSubjectManager(subSys);
//}

bool Account::getRates(int y, int m, QHash<int, Double> &rates)
{
    return dbUtil->getRates(y,m,rates);
}

bool Account::getRates(int y, int m, QHash<Money*, Double> &rates)
{
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
    else if(recentMonth != other.recentMonth)
        return true;
    else if(subSys != other.subSys)
        return true;
    else if(name != other.name)
        return true;
    else if(isCur != other.isCur)
        return true;
    return false;
}
