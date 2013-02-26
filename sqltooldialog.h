#ifndef SQLTOOLDIALOG_H
#define SQLTOOLDIALOG_H

#include <QDialog>
#include <QSqlQueryModel>
#include <QSqlTableModel>

#include "global.h"

#include "ui_basedataeditdialog.h"

namespace Ui {
    class SqlToolDialog;
}

class SqlToolDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SqlToolDialog(QWidget *parent = 0);
    ~SqlToolDialog();

public slots:
    void excuteSql();
    void selectedTable(const QString &text);
    void toggled(bool checked);
    void save();
    void contextMenuRequested(const QPoint &pos);
    void delRec();
    void clearAll();
    void addNewRec();
    void attachBasis(bool checked);
    void initTableList();

private slots:
    void on_chkImport_toggled(bool checked);

    void on_btnImport_clicked();

private:


    Ui::SqlToolDialog *ui;

    QSqlQueryModel* model;
    QSqlTableModel* tmodel;

    QAction* actDel;
    QAction* actClear;
    QAction* actAdd;

    int mc; //主数据库的表数目
    int bc; //基础数据库表数目

};

//基础数据库编辑对话框类
class BaseDataEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BaseDataEditDialog(QSqlDatabase* con = &bdb, QWidget *parent = 0);
    ~BaseDataEditDialog();

private slots:
    void excuteSql();
    void seledRow(int rowIndex);
    void on_cmbTables_currentIndexChanged(const QString &arg1);

    void on_btnSubmit_clicked();

    void on_btnRevert_clicked();

    void on_btnSwitch_clicked(bool checked);

    void on_btnAdd_clicked();

    void on_btnDel_clicked();

    void on_tview_clicked(const QModelIndex &index);

private:
    void init();

    QSqlDatabase* con;
    QSqlQueryModel* model;  //未来用一个自定义的model类，以支持对查询结果的编辑操作
    QSqlTableModel* tmodel;
    Ui::BaseDataEditDialog *ui;
    QHeaderView* vh;  //表格视图的行标题
    QSet<int> selRows; //选择的行集合
};

#endif // SQLTOOLDIALOG_H
