#ifndef DIALOGS_H
#define DIALOGS_H

#include <QStringListModel>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QDataWidgetMapper>

#include "ui_createaccountdialog.h"
#include "ui_openaccountdialog.h"
#include "ui_collectpzdialog.h"
#include "ui_reportdialog.h"


#include "config.h"
#include "common.h"
#include "account.h"



class OpenAccountDialog : public QDialog
{
    Q_OBJECT

public:
    OpenAccountDialog(QWidget* parent = 0);
    AccountCacheItem* getAccountCacheItem();

public slots:
    void itemClicked(const QModelIndex &index);
    void doubleClicked(const QModelIndex & index);
private:
    Ui::OpenAccountDialog ui;
    QStringListModel* model;
    QList<AccountCacheItem*> accList;
    int selAcc;   //选择的账户的序号
    QHash<AccountTransferState,QString> states;
};











class ReportDialog : public QDialog
{
    Q_OBJECT
public:
    ReportDialog(QWidget* parent = 0);
    void setReportType(int witch); //设置需要生成的报表类型

public slots:
    void btnCreateClicked();
    void btnSaveClicked();
    void btnToExcelClicked();
    void btnViewCliecked();
    void dateChanged(const QDate &date);
    void reportClassChanged();
    void viewReport();

private:
    void genReportDatas(int clsid, int tid); //生成指定类别和类型的报表数据，建议使用此函数来取代下面的四个函数
    void generateProfits();
    void generateAssets();
    void generateCashs();
    void generateOwner();
    void readReportAddInfo();
    void createModel(QList<QString>* nlist, QHash<QString, QString>* titles,
                     QHash<QString, double>* values, QHash<QString, double>* avalues);
    void readReportTitle(int clsid, int tid, QList<QString>* nlist,
                    QHash<QString, bool>* fthash, QHash<QString, QString>* titles);
    void calReportField(int clsid, int tid,
                        QHash<QString, double>* values);
    void calAddValue(int year, int month, QList<QString>* names, QHash<QString, double>* values);
    void readSubjectsExtra(int year, int month, QHash<QString, double>* values);
    void viewReport(int year, int month);
    QString readTableName(int clsid, int tid);

    Ui::reportDialog ui;
    QStandardItemModel* model;
    int usedReportTid; //所使用的报表分类代码（老式还是新式）
    int usedReporCid; //所使用的报表类型代码
    int year, month;    //报表所属的年月
    QString tableName;  //在数据库内的利润表名称
    QString reportTitle; //报表标题
};


////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////


//显示日记账、明细账的对话框


////////////////////////////////////////////////////////////
//可接受鼠标单击事件的标签类
class ClickAbleLabel : public QLabel
{
    Q_OBJECT

public:
    ClickAbleLabel(QString title, QString code, QWidget* parent = 0);
    //void setSubCode(QString code);

//public slots:

protected:
    void mousePressEvent (QMouseEvent *event);

signals:
    void viewSubExtra(QString code, const QPoint& pos); //要求显示子目余额的信号

private:
    QString code; //一级科目代码
};

////////////////////////////////////////////////////////////////////


#endif // DIALOGS_H
