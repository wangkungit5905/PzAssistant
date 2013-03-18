#ifndef DBUTIL_H
#define DBUTIL_H

#include <QSqlDatabase>

#include "account.h"


/**
 * @brief The DbUtil class
 *  提供对账户数据库的直接访问支持，所有其他的类要访问账户数据库，必须通过此类
 */

#define AccConnName "Account"

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

    bool readAccBriefInfo(AccountBriefInfo& info);
    bool initAccount(Account::AccountInfo& infos);
    bool saveAccountInfo(Account::AccountInfo &infos);
private:
    bool saveAccInfoPiece(InfoField code, QString value);
    bool readAccountSuites(QList<Account::AccountSuite*>& suites);
    bool saveAccountSuites(QList<Account::AccountSuite*> suites);
    void crtGdzcTable();

private:
    QSqlDatabase db;
    QString fileName;   //账户文件名
    QHash<InfoField,QString> pNames; //账户信息片段名称表
};

#endif // DBUTIL_H
