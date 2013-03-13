#include <QDebug>
#include <QSqlError>

#include "dbutil.h"
#include "global.h"
#include "tables.h"

DbUtil::DbUtil()
{
}

DbUtil::~DbUtil()
{
    if(db.isOpen())
        close();
}

/**
 * @brief DbUtil::setFilename
 *  设置账户文件名，并打开该连接
 * @param fname
 * @return
 */
bool DbUtil::setFilename(QString fname)
{
    if(db.isOpen())
        close();

    QString name = DatabasePath+fname;
    db = QSqlDatabase::addDatabase("QSQLITE",AccConnName);
    db.setDatabaseName(name);
    //return db.open();
    if(!db.open())
        qDebug()<<db.lastError()/*.text()*/;
}

/**
 * @brief DbUtil::colse
 * 关闭并移除账户数据库的连接
 */
void DbUtil::close()
{
    db.close();
    QSqlDatabase::removeDatabase(AccConnName);
}

/**
 * @brief DbUtil::upTo1_4

 * @return
 */
bool DbUtil::upTo1_4()
{
    return createSETable();
}

/**
 * @brief DbUtil::createSETable
 *  创建保存余额相关的表，5个新表（SEPoint、SE_PM_F,SE_MM_F,SE_PM_S,SE_MM_S）
 *  （1）余额指针表（SEPoint）
 *      包含字段年、月、币种（id，year，month，mt），唯一地指出了所属凭证年月和币种
 *  （2）所有与新余额相关的表以SE开头，表示科目余额。
 *      其中：PM指代原币新式、MM指代本币形式、F指代一级科目、S指代二级科目
 * @return
 */
bool DbUtil::createSETable()
{
    QSqlQuery q(db);
    QString s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 integer)")
            .arg(tbl_nse_point).arg(fld_sep_year).arg(fld_sep_month).arg(fld_sep_mt);
    if(!q.exec(s))
        return false;
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%5 integer,%4 real)")
            .arg(tbl_nse_p_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_dir).arg(fld_nse_value);
    if(!q.exec(s))
        return false;
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%5 integer,%4 real)")
            .arg(tbl_nse_m_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_dir).arg(fld_nse_value);
    if(!q.exec(s))
        return false;
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%5 integer,%4 real)")
            .arg(tbl_nse_p_s).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_dir).arg(fld_nse_value);
    if(!q.exec(s))
        return false;
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%5 integer,%4 real)")
            .arg(tbl_nse_m_s).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_dir).arg(fld_nse_value);
    if(!q.exec(s))
        return false;
    return true;
}
