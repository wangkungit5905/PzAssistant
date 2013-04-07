#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QString>
#include <QDate>
#include <QStringList>
#include <QHash>
#include <QSqlQuery>

#include "commdatastruct.h"
#include "securitys.h"
#include "appmodel.h"
#include "common.h"

class PzSetMgr;
class DbUtil;
class SubjectManager;


class Account
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
        ETIME     = 10,          //账户记账终止时间（当前账户最后记账时间）
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

	//账户转移状态
    enum AccountTransferState{
        ATS_TranOut     = 1,
        ATS_TranInDes   = 2,
        ATS_TranInOther = 3
    };

    /**
     * @brief The AccountSuite struct
     *  描述帐套的数据结构
     */
    struct AccountSuiteRecord{
        int id;
        int year,lastMonth; //帐套所属年份、最后打开月份
        int startMonth,endMonth; //开始月份和结束月份
        int subSys;         //帐套采用的科目系统代码
        QString name;       //帐套名
        bool isCur;         //是否当前打开帐套

        bool operator !=(const AccountSuiteRecord& other);
    };

    /**
     * @brief The AccountInfo struct
     *  账户信息结构
     */
    struct AccountInfo{
        QString code,sname,lname;           //账户代码、简称和全称
        Money* masterMt;                       //本币代码
        QList<Money*> waiMts;                  //外币代码表
        QString startDate,endDate;          //记账起止时间
        QList<AccountSuiteRecord*> suites;        //帐套列表
        QString lastAccessTime;             //账户最后访问时间
        QString dbVersion;                  //账户文件版本号
        QString logFileName;                //账户日志文件
        QString fileName;                   //账户文件名
    };


    Account(QString fname);
    ~Account();
    bool isValid();
    void close();
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
    void setWaiMt(QList<Money*> mts){accInfos.waiMts = mts;}
    void addWaiMt(Money *mt);
    void delWaiMt(Money *mt);
    QString getWaiMtStr();
    QDate getStartTime(){return QDate::fromString(accInfos.startDate,Qt::ISODate);}
    void setStartTime(QDate date){accInfos.startDate = date.toString(Qt::ISODate);}
    QDate getEndTime(){return QDate::fromString(accInfos.endDate,Qt::ISODate);}
    void setEndTime(QDate date){accInfos.endDate = date.toString(Qt::ISODate);}

    //日志相关
    QString getAllLogs();
    QString getLog(QDateTime time);
    void appendLog(QDateTime time, QString log);
    void delAllLogs();
    void delLogs(QDateTime start, QDateTime end);

    //帐套相关
    void appendSuite(int y, QString name, int curMonth = 1, int subSys = 1);
    void delSuite(int y);
    QString getSuiteName(int y);
    void setSuiteName(int y, QString name);
    QList<int> getSuites();
    AccountSuiteRecord* getStartSuite(){return accInfos.suites.first();}
    AccountSuiteRecord* getEndSuite(){return accInfos.suites.last();}
    AccountSuiteRecord* getCurSuite();
    AccountSuiteRecord* getSuite(int y);
    void setCurSuite(int y);
    bool getSuiteMonthRange(int y, int& sm,int& em);
    bool containSuite(int y);
    int getSuiteFirstMonth(int y);
    int getSuiteLastMonth(int y);
    void setCurMonth(int m);
    int getCurMonth();

    int getBaseYear();
    int getBaseMonth();

    PzSetMgr* getPzSet(){return pzSetMgr;}
    void colsePzSet();
    SubjectManager* getSubjectManager(int subSys = 0);
    //SubjectManager* getSubjectManager();

    bool getRates(int y, int m, QHash<int, Double> &rates);
    bool getRates(int y, int m, QHash<Money*, Double> &rates);
    bool setRates(int y, int m, QHash<int, Double> &rates);

    QDateTime getLastAccessTime(){return QDateTime::fromString(accInfos.lastAccessTime,Qt::ISODate);}
    void setLastAccessTime(QDateTime time){accInfos.lastAccessTime = time.toString(Qt::ISODate);}

	void setReadOnly(bool readOnly){isReadOnly=readOnly;}
	bool getReadOnly(){return isReadOnly;}
    QList<BankAccount*> getAllBankAccount(){return bankAccounts;}
    static void setDatabase(QSqlDatabase* db);

private:
    bool init();


	static QSqlDatabase* db;
    User* user;            //操作此账户的用户
    int subType; //账户所用的科目类型（科目系统由帐套来定）
    //ReportType reportType; //账户所用的报表类型

    QList<BankAccount*> bankAccounts;
    PzSetMgr* pzSetMgr;      //凭证集对象
    QHash<int,SubjectManager*> smgs; //科目管理对象（键为科目系统代码）
	bool isReadOnly;         //是否只读模式

    DbUtil* dbUtil; //直接访问账户文件的数据库访问对象
    AccountInfo accInfos;   //账户信息
    QHash<int,Money*> moneys; //账户所使用的所有货币对象

    friend class DbUtil;
};

//帐套结构比较函数
bool byAccountSuiteThan(Account::AccountSuiteRecord *as1, Account::AccountSuiteRecord *as2);

#endif // ACCOUNT_H
