#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QString>
#include <QDate>
#include <QStringList>
#include <QHash>
#include <QSqlQuery>

#include "commdatastruct.h"
#include "securitys.h"
#include "common.h"

class AccountSuiteManager;
class DbUtil;
class SubjectManager;
class WorkStation;

/**
 * @brief The AccontTranferInfo struct
 *  账户转移记录
 */
struct AccountTranferInfo{
    int id;
    AccountTransferState tState;    //转移状态
    WorkStation *m_out, *m_in;          //转出（源）/转入（目的）工作站
    QDateTime t_out, t_in;          //转出/转入时间
    QString desc_out,desc_in;       //转出/转入描述
};



class Account : public QObject
{
public:
    enum InfoField{
        ACODE     = 1,           //账户代码
        SNAME     = 2,           //账户简称
        LNAME     = 3,           //账户全称
        FNAME     = 4,           //账户工作数据库文件名（废弃）
        SUBTYPE   = 5,           //账户所采用的科目类型（废弃）
        RPTTYPE   = 6,           //报表类型
        MASTERMT  = 7,           //本币代码
        WAIMT     = 8,           //外币代码列表
        STIME     = 9,           //账户记账起始时间
        ENDTIME   = 10,          //账户记账终止时间（当前账户最后记账时间）
        CSUITE    = 11,          //账户当前帐套年份
        SUITENAME = 12,          //帐套名列表
        LASTACCESS= 13,          //账户最后访问时间
        LOGFILE   = 50,          //与该账户相关的日志文件名
        DBVERSION = 51           //账户文件的版本号（用来表示数据库内表格的变动）
    };

    //报表类型
    enum ReportType{
        RPT_OLD = 1,
        RPT_NEW = 2
    };


    /**
     * @brief The AccountInfo struct
     *  账户信息结构
     */
    struct AccountInfo{
        QString code,sname,lname;           //账户代码、简称和全称
        Money* masterMt;                    //本币代码
        QList<Money*> waiMts;               //外币代码表
        QString lastAccessTime;             //账户最后访问时间
        QString dbVersion;                  //账户文件版本号
        QString logFileName;                //账户日志文件
        QString fileName;                   //账户文件名
        AccountTranferInfo* transInfo;      //账户转移信息
        bool isJxTaxManaged;                //是否需要进项税管理
    };


    static bool createNewAccount(QString fileName,QString code,QString name, QString lname, SubSysNameItem* subSys, int startYear, int startMonth, QString& error);
    static bool createTableForNewAccount(QSqlDatabase db, QString &errors);
    static bool importSubjectForNewAccount(int subSys, QSqlDatabase db, QString &errors);

    Account(QString fname, QObject* parent=0);
    ~Account();
    bool isValid();
    bool canAccess(User* u);
    void close();
    AccountTranferInfo* getRecentTransferInfo(){return accInfos.transInfo;}
    bool saveAccountInfo();
    bool isOpen(){return isOpened;}
    DbUtil* getDbUtil(){return dbUtil;}
    QString getSName(){return accInfos.sname;}
    void setSName(QString name){accInfos.sname = name;}
    QString getLName(){return accInfos.lname;}
    void setLName(QString name){accInfos.lname = name;}
    QString getCode(){return accInfos.code;}
    void setCode(QString code){accInfos.code = code;}
    QString getFileName(){return accInfos.fileName;}
    void setFileName(QString fname){accInfos.fileName = fname;}
    int getSubType(){return subType;}
    void setSubType(int type){subType = type;}
    //ReportType getReportType(){return reportType;}
    //void setReportType(ReportType type){reportType = type; savePiece(RPTTYPE,QString::number(type));}
    QHash<int,Money*>& getAllMoneys(){return moneys;}
    Money* getMasterMt(){return accInfos.masterMt;}
    void setMasterMt(Money* mt){accInfos.masterMt = mt;}
    QList<Money*> getWaiMt(){return accInfos.waiMts;}
    void setWaiMts(QList<Money*> mts){accInfos.waiMts = mts;}
    void addWaiMt(Money *mt);
    void delWaiMt(Money *mt);
    bool isUsedWb(){return (moneys.count() > 1);}
    QString getWaiMtStr();
    QDate getStartDate();/*{return QDate::fromString(accInfos.startDate,Qt::ISODate);}*/
    //void setStartTime(QDate date);/*{accInfos.startDate = date.toString(Qt::ISODate);}*/
    QDate getEndDate();/*{return QDate::fromString(accInfos.endDate,Qt::ISODate);}*/
    void setEndTime(QDate date);/*{accInfos.endDate = date.toString(Qt::ISODate);}*/

    //日志相关
    QString getAllLogs();
    QString getLog(QDateTime time);
    void appendLog(QDateTime time, QString log);
    void delAllLogs();
    void delLogs(QDateTime start, QDateTime end);

    //帐套相关
    AccountSuiteRecord *appendSuiteRecord(int y, QString name, int subSys = 1);
    void addSuite(AccountSuiteRecord* as){suiteRecords.append(as);}
    void delSuite(int y);
    QString getSuiteName(int y);
    void setSuiteName(int y, QString name);
    QList<int> getSuiteYears();
    QList<AccountSuiteRecord*> getAllSuiteRecords(){return suiteRecords;}
    AccountSuiteRecord* getStartSuiteRecord(){return suiteRecords.isEmpty()?NULL:suiteRecords.first();}
    AccountSuiteRecord* getEndSuiteRecord(){return suiteRecords.isEmpty()?NULL:suiteRecords.last();}
    AccountSuiteRecord* getCurSuiteRecord();
    AccountSuiteRecord* getSuiteRecord(int y);
    void setCurSuite(int y);
    bool getSuiteMonthRange(int y, int& sm,int& em);
    bool containSuite(int y);
    int getSuiteFirstMonth(int y);
    int getSuiteLastMonth(int y);
    void setCurMonth(int m, int y=0);
    int getCurMonth(int y=0);
    bool saveSuite(AccountSuiteRecord* as);

    int getBaseYear();
    int getBaseMonth();
    void getVersion(int &mv,int &sv);

    AccountSuiteManager* getSuiteMgr(int suiteId = 0);
    SubjectManager* getSubjectManager(int subSys = 0);
    QList<SubSysNameItem*> getSupportSubSys();
    bool importNewSubSys(int code);

    bool getRates(int y, int m, QHash<int, Double> &rates);
    bool getRates(int y, int m, QHash<Money*, Double> &rates);
    bool setRates(int y, int m, QHash<int, Double> &rates);

    QDateTime getLastAccessTime(){return QDateTime::fromString(accInfos.lastAccessTime,Qt::ISODate);}
    void setLastAccessTime(QDateTime time){accInfos.lastAccessTime = time.toString(Qt::ISODate);}

    void setReadOnly(bool readOnly){this->readOnly=readOnly;}
    bool isReadOnly(){return readOnly;}
    QList<BankAccount*> getAllBankAccount();
    QList<Bank*> getAllBank(){return banks;}
    bool saveBank(Bank* bank);
    bool removeBank(Bank* bank);
    bool addBank(Bank* bank);
    static void setDatabase(QSqlDatabase* db);

    bool getSubSysJoinCfgInfo(int src, int des, QList<SubSysJoinItem2*>& cfgs);
    bool saveSubSysJoinCfgInfo(int src, int des, QList<SubSysJoinItem2*> cfgs);
    bool isImportSubSys(int code);
    bool isSubSysConfiged(int code);

    bool isJxTaxManaged(){return accInfos.isJxTaxManaged;}
    void setJxTaxManaged(bool ok){accInfos.isJxTaxManaged=ok;}
    bool isConvertExtra(int year);
    //bool convertExtra(QHash<int,Double>& sums, QHash<int,MoneyDirection>& dirs, const QHash<int, int> maps);
    bool convertExtra2(int year, QHash<int,Double>& fsums, QHash<int,MoneyDirection>& fdirs, QHash<int, Double> &dsums, QHash<int, MoneyDirection> &ddirs);

private:
    bool init();


    static QSqlDatabase* db;
    int subType; //账户所用的科目类型（科目系统由帐套来定）
    QList<Bank*> banks;
    QList<SubSysNameItem*> subSysLst; //账户支持的科目系统
    QHash<int,SubjectManager*> smgs;  //科目管理对象（键为科目系统代码）
    QHash<int,AccountSuiteManager*> suiteHash; //帐套管理对象表（键为帐套记录的id）
    QList<AccountSuiteRecord*> suiteRecords;         //帐套记录结构列表
    bool readOnly;         //是否只读模式
    DbUtil* dbUtil; //直接访问账户文件的数据库访问对象
    AccountInfo accInfos;   //账户信息
    QHash<int,Money*> moneys; //账户所使用的所有货币对象
    bool isOpened;  //账户是否打开

    friend class DbUtil;
};

//帐套结构比较函数
bool byAccountSuiteThan(AccountSuiteRecord *as1, AccountSuiteRecord *as2);

#endif // ACCOUNT_H
