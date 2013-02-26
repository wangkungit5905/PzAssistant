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
#include "subjectmanager.h"
//#include "global.h"

class PzSetMgr;

typedef bool (*UpdateAccVerFun)();

class Account
{
public:
    enum InfoField{
        ACODE     = 1,           //账户代码
        SNAME     = 2,           //账户简称
        LNAME     = 3,           //账户全称
        FNAME     = 4,           //账户工作数据库文件名
        SUBTYPE   = 5,           //账户所采用的科目类型
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

    Account(QString fname/*, QSqlDatabase db = QSqlDatabase::database()*/);
    ~Account();
    //void saveAll();
    void savePiece(InfoField f, QString v);
    bool isValid();
    //QString getVersion(){return db_version;}
    //void setVersion(QString vs){db_version = vs;}
    QString getSName(){return sname;}
    void setSName(QString name){sname = name;savePiece(SNAME,name);}
    QString getLName(){return lname;}
    void setLName(QString name){lname = name;savePiece(LNAME,name);}
    QString getCode(){return accCode;}
    void setCode(QString code){accCode = code;savePiece(ACODE,code);}
    QString getFileName(){return fileName;}
    void setFileName(QString fname){fileName = fname;savePiece(FNAME,fname);}
    SubjectManager::SubjectSysType getSubType(){return subType;}
    void setSubType(SubjectManager::SubjectSysType type){subType = type;savePiece(SUBTYPE,QString::number(type));}
    ReportType getReportType(){return reportType;}
    void setReportType(ReportType type){reportType = type; savePiece(RPTTYPE,QString::number(type));}
    int getMasterMt(){return masterMt;}
    void setMasterMt(int mt){masterMt = mt;savePiece(MASTERMT,QString::number(mt));}
    QList<int> getWaiMt(){return waiMt;}
    void setWaiMt(QList<int> mts);
    void addWaiMt(int mt);
    void delWaiMt(int mt);
    QString getWaiMtStr();
    QDate getStartTime(){return QDate::fromString(startTime,Qt::ISODate);}
    void setStartTime(QDate date){startTime = date.toString(Qt::ISODate);
                                  savePiece(STIME,startTime);
                                 }
    QDate getEndTime(){return QDate::fromString(endTime,Qt::ISODate);}
    void setEndTime(QDate date){endTime = date.toString(Qt::ISODate);
                               savePiece(ETIME,endTime);}

    QString getAllLogs();
    QString getLog(QDateTime time);
    void appendLog(QDateTime time, QString log);
    void delAllLogs();
    void delLogs(QDateTime start, QDateTime end);

    void appendSuite(int y, QString name);
    void delSuite(int y);
    QString getSuiteName(int y){return suiteNames.value(y);}
    void setSuiteName(int y, QString name);
    QList<int> getSuites(){return suiteNames.keys();}
    int getStartSuite(){return startSuite;}
    int getEndSuite(){return endSuite;}
    int getCurSuite(){return curSuite;}
    void setCurSuite(int y){curSuite = y;
                            savePiece(CSUITE,QString::number(curSuite));}
    bool getSuiteMonthRange(int y, int& sm,int& em);
    bool containSuite(int y){return suiteNames.contains(y);}
    int getSuiteFirstMonth(int y);
    int getSuiteLastMonth(int y);
    void setCurMonth(int m){curMonth=m;}
    int getCurMonth(){return curMonth;}

    int getBaseYear();
    int getBaseMonth();

    PzSetMgr* getPzSet();
    void colsePzSet();
    SubjectManager* getSubjectManager(){return subMng;}

    QDateTime getLastAccessTime(){return QDateTime::fromString(lastAccessTime,Qt::ISODate);}
    void setLastAccessTime(QDateTime time){lastAccessTime = time.toString(Qt::ISODate);
                                          savePiece(LASTACCESS,lastAccessTime);}

	void setReadOnly(bool readOnly){isReadOnly=readOnly;}
	bool getReadOnly(){return isReadOnly;}
	static void setDatabase(QSqlDatabase* db);
	static bool versionMaintain(bool &cancel);
	
	//账户库版本维护相关函数
    static bool getVersion(int& mv, int &sv);
    static bool setVersion(int mv, int sv);
    static bool perfectVersion(){return true;}  //归集到初始版本，在此只是占位函数
    static bool updateTo1_3();
    static bool updateTo1_4(){return true;}
    static bool updateTo1_5(){return true;}

private:
    QString assembleSuiteNames();
    void crtAccountTable();
    void crtGdzcTable();
    //void insertVersion();

	static QSqlDatabase* db;
    User* user;            //操作此账户的用户
    QString db_version;    //账户文件版本号
    QString sname,lname;   //账户简称和全称
    QString accCode;       //账户代码
    QString fileName;      //账户文件名
    SubjectManager::SubjectSysType subType; //账户所用的科目类型
    ReportType reportType; //账户所用的报表类型
    int masterMt;          //本币代码
    QList<int> waiMt;      //外币代码列表
    QString startTime,endTime;      //记账开始日期与结束日期
    QString logFileName;   //账户的日志文件名（默认同工作数据库文件名，后缀为“.log”）
    QStringList logs;      //账户的日志信息（共2列，时间和信息）
    int startSuite,endSuite,curSuite; //起始、结束和当前帐套年份
    int curMonth;                     //当前操作的凭证集月份
    QHash<int,QString> suiteNames;    //帐套名
    QString lastAccessTime;  //账户最后访问时间

    QList<BankAccount*> bankAccounts;
    PzSetMgr* curPzSet;      //当前凭证集
    SubjectManager* subMng;  //科目管理对象
	bool isReadOnly;         //是否只读模式

	QList<QString> versionHistorys;  //账户库历史版本号
    QHash<QString,UpdateAccVerFun> updateFuns; //账户库更新函数列表
};



#endif // ACCOUNT_H
