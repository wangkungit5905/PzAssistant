#ifndef DBUTIL_H
#define DBUTIL_H

#include <QSqlDatabase>
/**
 * @brief The DbUtil class
 *  提供对账户数据库的直接访问支持，所有其他的类要访问账户数据库，必须通过此类
 */

#define AccConnName "Account"

class DbUtil
{
public:
    DbUtil();
    ~DbUtil();
    bool setFilename(QString fname);
    void close();

    //数据库升级服务函数
    bool upTo1_4();
private:
    bool createSETable();

private:
    QSqlDatabase db;
};

#endif // DBUTIL_H
