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
#include "configvariablenames.h"

QSqlDatabase* Account::db;

Account::Account(QString fname, QObject *parent):QObject(parent)
{
    isOpened = false;
    isReadOnly = false;
    accInfos.fileName = fname;
    if(!init())
        QMessageBox::critical(0,QObject::tr("错误信息"),QObject::tr("账户在初始化时发生错误！"));
    //pzSetMgr = new PzSetMgr(this,curUser);
}

Account::~Account()
{
    qDeleteAll(smgs);
}

bool Account::isValid()
{
    if(!isOpened || accInfos.code.isEmpty() ||
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
    isOpened = false;
}

bool Account::saveAccountInfo()
{
    return dbUtil->saveAccountInfo(accInfos);
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
QDate Account::getStartDate()
{
    AccountSuiteRecord* asr = suiteRecords.first();
    if(asr)
        return QDate(asr->year,asr->startMonth,1);
    else
        return QDate();
}

/**
 * @brief Account::getEndTime
 *  获取账户结束记账时间
 * @return
 */
QDate Account::getEndDate()
{
    AccountSuiteRecord* asr = suiteRecords.last();
    if(asr){
        QDate d = QDate(asr->year,asr->endMonth,1);
        d.setDate(asr->year,asr->endMonth,d.daysInMonth());
        return d;
    }
    else
        return QDate();
}

/**
 * @brief Account::setEndTime
 *  设置账户结束记账时间
 * @param date
 */
void Account::setEndTime(QDate date)
{
    AccountSuiteRecord* asr = suiteRecords.last();
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
AccountSuiteRecord* Account::appendSuite(int y, QString name, int subSys)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            return NULL;
    }
    AccountSuiteRecord* as = new AccountSuiteRecord;
    as->id = 0; as->year = y; as->name = name;
    as->isClosed = false; as->recentMonth = 1;
    as->startMonth = 1; as->endMonth = 1;as->subSys = subSys;
    as->isCur = false;
    int i = 0;
    while(i < suiteRecords.count() && y > suiteRecords.at(i)->year)
        i++;
    suiteRecords.insert(i,as);
    return as;
}

void Account::setSuiteName(int y, QString name)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            as->name = name;
    }
}

//考虑移除（制作打开凭证集对话框类和老的账户属性配置中被使用）
QList<int> Account::getSuites()
{
    QList<int> ys;
    for(int i = 0; i < suiteRecords.count(); ++i)
        ys<<suiteRecords.at(i)->year;
    return ys;
}

/**
 * @brief Account::getCurSuite
 *  获取当前帐套
 * @return
 */
AccountSuiteRecord* Account::getCurSuite()
 {
     foreach(AccountSuiteRecord* as, suiteRecords)
         if(as->isCur)
             return as;
     if(!suiteRecords.empty()){
         suiteRecords.last()->isCur = true;
         return suiteRecords.last();
     }
     return NULL;
 }

 /**
  * @brief Account::getSuite
  * 获取指定年份的帐套对象
  * @param y
  * @return
  */
 AccountSuiteRecord *Account::getSuite(int y)
 {
     foreach(AccountSuiteRecord* as, suiteRecords){
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
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            as->isCur = true;
        else
            as->isCur = false;
    }
}

/**
 * @brief 删除账套亦将账套的凭证及其其他相关的信息都删除，而且与其承上启下的账套都将受到影响，
 *  因此，必须严格斟酌是否有必要提供此函数
 * @param y
 */
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
    for(int i = 0; i < suiteRecords.count(); ++i){
        if(y == suiteRecords.at(i)->year)
            return suiteRecords.at(i)->name;
    }
}

//获取当前帐套的起始月份
int Account::getSuiteFirstMonth(int y)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            return as->startMonth;
    }
    return 0;
}

//获取当前帐套的结束月份
int Account::getSuiteLastMonth(int y)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
        if(as->year == y)
            return as->endMonth;
    }
    return 0;
}

/**
 * @brief Account::setCurMonth
 *  设置当前帐套的当前打开的凭证集的月份
 * @param m
 * @param y
 */
void Account::setCurMonth(int m, int y)
{
    if(y == 0){
        foreach(AccountSuiteRecord* as, suiteRecords){
            if(as->isCur)
                as->recentMonth = m;
        }
    }
    else{
        foreach(AccountSuiteRecord* as, suiteRecords){
            if(as->year == y)
                as->recentMonth = m;
        }
    }

}

/**
 * @brief Account::getCurMonth
 *  获取当前帐套的最近打开凭证集的月份
 * @return
 */
int Account::getCurMonth(int y)
{
    if(y == 0){
        foreach(AccountSuiteRecord* as, suiteRecords){
            if(as->isCur)
                return as->recentMonth;
        }
    }
    else{
        foreach(AccountSuiteRecord* as, suiteRecords){
            if(as->year == y)
                return as->recentMonth;
        }
    }
    return 0;
}

bool Account::saveSuite(AccountSuiteRecord *as)
{
    return dbUtil->saveSuite(as);
}

//获取账户期初年份（即开始记账的前一个月所处的年份）
int Account::getBaseYear()
{
    if(suiteRecords.isEmpty())
        return 0;
    AccountSuiteRecord* asr = suiteRecords.first();
    if(asr->startMonth == 1)
        return asr->year-1;
    else
        return asr->year;
}

//获取账户期初月份
int Account::getBaseMonth()
{
    if(suiteRecords.isEmpty())
        return 0;
    AccountSuiteRecord* asr = suiteRecords.last();
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

/**
 * @brief 返回指定的帐套管理对象
 * @param suiteId   帐套id，如果为0，则返回当前帐套
 * @return
 */
AccountSuiteManager *Account::getPzSet(int suiteId)
{
    if(suiteRecords.isEmpty())
        return NULL;
    if(suiteId != 0 && suiteHash.contains(suiteId))
        return suiteHash.value(suiteId);
    int key = 0;
    AccountSuiteRecord* record = NULL;
    if(suiteId == 0){
        foreach(AccountSuiteRecord* asr, suiteRecords){
            if(asr->isCur){
                key = asr->id;
                record = asr;
                break;
            }
        }
    }
    else{
        key = suiteId;
        foreach(AccountSuiteRecord* asr, suiteRecords){
            if(asr->id == key){
                record = asr;
                break;
            }
        }
    }
    if(key == 0)
        return NULL;
    if(!suiteHash.contains(key))
        suiteHash[key] = new AccountSuiteManager(record,this);
    return suiteHash.value(key);
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
    //pzSetMgr->close();
    //pzSetMgr = NULL;
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

/**
 * @brief 获取账户支持的科目系统
 * @return
 */
QList<SubSysNameItem *> Account::getSupportSubSys()
{
    if(subSysLst.isEmpty()){
        AppConfig::getInstance()->getSubSysItems(subSysLst);
        for(int i = 0; i < subSysLst.count(); ++i){
            if(dbUtil->isSubSysImported(subSysLst.at(i)->code))
                subSysLst.at(i)->isImport = true;
        }
    }
    return subSysLst;
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
    QHash<int,Double> rs;
    if(!dbUtil->getRates(y,m,rs))
        return false;
    QHashIterator<int,Double> it(rs);
    while(it.hasNext()){
        it.next();
        rates[moneys.value(it.key())] = it.value();
    }
    return true;
}

bool Account::setRates(int y, int m, QHash<int, Double> &rates)
{
    return dbUtil->saveRates(y,m,rates);
}

/**
 * @brief Account::getAllBankAccount
 * @return
 */
QList<BankAccount *> Account::getAllBankAccount()
{
    QList<BankAccount*> bas;
    foreach(Bank* bank, banks)
        bas<<bank->bas;
    return bas;
}

bool Account::saveBank(Bank *bank)
{
    return dbUtil->saveBankInfo(bank);
}

void Account::setDatabase(QSqlDatabase *db)
{Account::db=db;}

/**
 * @brief 获取账户的科目系统衔接配置信息
 * @param src   源科目系统代码
 * @param des   目的科目系统代码
 * @param cfgs  科目的映射
 * @return
 */
bool Account::getSubSysJoinCfgInfo(int src, int des, QList<SubSysJoinItem *> &cfgs)
{
    return dbUtil->getSubSysJoinCfgInfo(getSubjectManager(src),getSubjectManager(des),cfgs);
}

bool Account::setSubSysJoinCfgInfo(int src, int des, QList<SubSysJoinItem *> &cfgs)
{
    return dbUtil->setSubSysJoinCfgInfo(getSubjectManager(src),getSubjectManager(des),cfgs);
}

/**
 * @brief 返回是否已完成从源科目系统（src）到目的科目系统（des）的配置
 * @param src
 * @param des
 * @param subCloned 二级科目是否已克隆
 * @return
 */
bool Account::isCompleteSubSysCfg(int src, int des, bool &completed, bool &subCloned)
{
    QString vname = QString("%1_%2_%3").arg(CFG_SUBSYS_COMPLETE_PRE).arg(src).arg(des);
    QVariant v;
    v.setValue(completed);
    if(!dbUtil->getCfgVariable(vname,v))
        return false;
    if(v.isValid())
        completed = v.toBool();
    else
        completed = false;
    vname = QString("%1_%2_%3").arg(CFG_SUBSYS_SUBCLONE_PRE).arg(src).arg(des);
    v.setValue(subCloned);
    if(!dbUtil->getCfgVariable(vname,v))
        return false;
    if(v.isValid())
        subCloned = v.toBool();
    else
        subCloned = false;
    return true;
}

/**
 * @brief 设置是否已完成从源科目系统（src）到目的科目系统（des）的配置
 * @param src
 * @param des
 * @param subCloned 二级科目是否已克隆
 */
bool Account::setCompleteSubSysCfg(int src, int des, bool completed, bool subCloned)
{
    QString vname = QString("%1_%2_%3").arg(CFG_SUBSYS_COMPLETE_PRE).arg(src).arg(des);
    QVariant v;
    v.setValue(completed);
    if(!dbUtil->setCfgVariable(vname,v))
        return false;
    vname = QString("%1_%2_%3").arg(CFG_SUBSYS_SUBCLONE_PRE).arg(src).arg(des);
    v.setValue(subCloned);
    if(!dbUtil->setCfgVariable(vname,v))
        return false;
    return true;
}

/**
 * @brief 获取是否完成余额的衔接
 * @param src
 * @param des
 * @param completed
 * @return
 */
bool Account::isCompletedExtraJoin(int src, int des, bool &completed)
{
    QString vname = QString("%1_%2_%3").arg(CFG_SUBSYS_EXTRA_JOIN).arg(src).arg(des);
    QVariant v;
    v.setValue(completed);
    if(!dbUtil->getCfgVariable(vname,v))
        return false;
    if(v.isValid())
        completed = v.toBool();
    else
        completed = false;
    return true;
}



/**
 * @brief Account::init
 *  账户对象初始化
 * @return
 */
bool Account::init()
{
    accInfos.masterMt = NULL;

    dbUtil = new DbUtil;
    bool ok = true;
    if(!dbUtil->setFilename(accInfos.fileName)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Can't connect to account file!"));
        ok = false;
    }
    if(ok && !dbUtil->initMoneys(this)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Money init happen error!"));
        ok = false;
    }
    if(ok && !dbUtil->initAccount(accInfos)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Account info init happen error!"));
        ok = false;
    }
    if(ok && !dbUtil->initSuites(suiteRecords))    {
        LOG_ERROR("Initial account suite happen error!");
        ok = false;
    }
    if(ok && !dbUtil->initNameItems()){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Name items init happen error!"));
        ok = false;
    }
    if(ok && !dbUtil->initBanks(this)){
        Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                      QObject::tr("Bank init happen error!"));
        ok = false;
    }
    if(ok){
        isOpened = true;
        return true;
    }
    else{
        dbUtil->close();
        return false;
    }
}


//获取帐套内凭证集的开始、结束月份
bool Account::getSuiteMonthRange(int y,int& sm, int &em)
{
    foreach(AccountSuiteRecord* as, suiteRecords){
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
    foreach(AccountSuiteRecord* as, suiteRecords){
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
bool byAccountSuiteThan(AccountSuiteRecord *as1, AccountSuiteRecord *as2)
{
    return as1->year < as2->year;
}


bool AccountSuiteRecord::operator !=(const AccountSuiteRecord& other)
{
    if(year != other.year)
        return true;
    else if(recentMonth != other.recentMonth)
        return true;
    else if(subSys != other.subSys)
        return true;
    else if(name != other.name)
        return true;
    else if(isClosed != other.isClosed)
        return true;
    return false;
}
