#ifndef DBUTIL_H
#define DBUTIL_H

#include <QSqlDatabase>

#include "account.h"


/**
 * @brief The DbUtil class
 *  提供对账户数据库的直接访问支持，所有其他的类要访问账户数据库，必须通过此类
 */

const QString AccConnName = "Account";

class SubjectManager;
class FirstSubject;

class DbUtil
{
public:
    enum InfoField{
        ACODE     = 1,           //账户代码
        SNAME     = 2,           //账户简称
        LNAME     = 3,           //账户全称
        //RPTTYPE   = 6,           //报表类型
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

    DbUtil();
    ~DbUtil();
    bool setFilename(QString fname);
    void close();

    //账户信息相关
    bool readAccBriefInfo(AccountBriefInfo& info);
    bool initAccount(Account::AccountInfo& infos);
    bool saveAccountInfo(Account::AccountInfo &infos);

    //科目相关
    bool initNameItems();
    bool initSubjects(SubjectManager* smg, int subSys);
    bool saveNameItem(SubjectNameItem* ni);
    bool saveSndSubject(SecondSubject* sub);
    bool savefstSubject(FirstSubject* fsub);

    //货币相关
    bool initMoneys(QHash<int,Money*> &moneys);
    bool initBanks(Account* account);

    //余额相关
    bool readExtraForPm(int y,int m, QHash<int,Double>& fsums,
                                     QHash<int,MoneyDirection>& fdirs,
                                     QHash<int,Double>& ssums,
                                     QHash<int,MoneyDirection>& sdirs);
    bool readExtraForMm(int y,int m, QHash<int,Double>& fsums,
                                     QHash<int,Double>& ssums);
    bool saveExtraForPm(int y, int m, const QHash<int, Double>& fsums,
                                      const QHash<int, MoneyDirection>& fdirs,
                                      const QHash<int, Double>& ssums,
                                      const QHash<int, MoneyDirection>& sdirs);
    bool saveExtraForMm(int y, int m, const QHash<int, Double>& fsums,
                                      const QHash<int, Double>& ssums);

    //提供给SubjectComplete类的数据模型所用的查询对象
    QSqlQuery* getQuery(){/*QSqlQuery q(db); return q;*/return new QSqlQuery(db);}

private:
    bool saveAccInfoPiece(InfoField code, QString value);
    bool readAccountSuites(QList<Account::AccountSuite*>& suites);
    bool saveAccountSuites(QList<Account::AccountSuite*> suites);

    //余额相关辅助函数
    bool _readExtraPoint(int y, int m, QHash<int, int> &mtHashs);
    bool _readExtraForPm(int y, int m, QHash<int,Double> &sums, QHash<int,MoneyDirection> &dirs, bool isFst = true);
    bool _readExtraForMm(int y, int m, QHash<int,Double> &sums, bool isFst = true);
    bool _crtExtraPoint(int y, int m, int mt, int& pid);
    bool _saveExtrasForPm(int y, int m, const QHash<int,Double> &sums, const QHash<int,MoneyDirection> &dirs, bool isFst = true);
    bool _saveExtrasForMm(int y, int m, const QHash<int,Double> &sums, bool isFst = true);

    //表格创建函数
    void crtGdzcTable();

private:
    QSqlDatabase db;
    QString fileName;   //账户文件名
    QHash<InfoField,QString> pNames; //账户信息片段名称表
};

#endif // DBUTIL_H
