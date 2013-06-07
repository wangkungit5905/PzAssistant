#ifndef DIALOGS_H
#define DIALOGS_H

#include <QStringListModel>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QDataWidgetMapper>

#include "ui_createaccountdialog.h"
#include "ui_openaccountdialog.h"
#include "ui_openpzdialog.h"
#include "ui_collectpzdialog.h"
#include "ui_basicdatadialog.h"
#include "ui_subjectextradialog.h"
#include "ui_reportdialog.h"
#include "ui_setupbankdialog.h"
#include "ui_detailsviewdialog.h"
#include "ui_detailextradialog.h"


#include "config.h"
#include "appmodel.h"
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
};

class OpenPzDialog : public QDialog
{
    Q_OBJECT

public:
    OpenPzDialog(Account* account, QWidget* parent = 0);
    QDate getDate();

private slots:
    void suiteChanged(int index);
    void monthChanged(int month);

    void on_chkNew_clicked(bool checked);

    void on_btnOk_clicked();

private:
    Ui::openPzDialog ui;
    int y;  //选择的帐套年份
    int m;  //选择的凭证集月份
    Account* account;
};

struct PZCollect{
    int id;          //科目ID
    QString name;    //科目名称
    double jsum;     //借方合计值
    double dsum;     //贷方合计值
public:
    bool operator<( const PZCollect &other ) const;
//    void setName(QString name);
//    void addJ(double value);
//    void addD(double value);
//    QString getName();
//    double getJ();
//    double getD();
};

//处理所有一级科目的汇总值
class PzCollectProcess
{
public:
//    void addJ(int id, double v);
//    void addD(int id, double v);
//    void setName(int id, QString name);
    void setValue(int id, QString name, double jsum, double dsum);
    double getJ(int id);
    double getD(int id);
    QString getName(int id);
    int getCount();
    QStandardItemModel* toModel();
    void clear();

private:

    QList<PZCollect> plist;
};

////作为哈希表的复合键的结构类型
//class CompKey{
//public:
//    CompKey();
//    CompKey(int id,int code){subId = id;mtCode = code;}
//    int getId(){return subId;}
//    int getCode(){return mtCode;}

//private:
//    int subId;   //子目id值（在FSAgent表中的id列）
//    int mtCode;  //币种代码
//};



class BasicDataDialog : public QDialog
{
    Q_OBJECT

public:
    BasicDataDialog(bool isWizard = true, QWidget* parent = 0);

signals:
    void toNextStep(int curStep, int nextStep);

public slots:
    void btnImpOkClicked();
    void btnImpCancelClicked();
    void btnImpBowClicked();
    void btnExpOkClicked();
    void btnEXpCancelClicked();
    void btnExpBowClicked();
    //void chkImpSpecStateChanged(int state);
    void chkExpSpecStateChanged(int state);
    void nextStep();
    void saveFstToBase();
    void saveSndToBase();

private:
    void crtSubExtraTable(bool isTem); //创建余额表    
    void crtReportStructTable(int clsid);
    void demandAttachDatabase(bool isAttach);
    void impFstSubFromBasic(int subCls);
    void impSndSubFromBasic();
    void impCliFromBasic();

    Ui::basicDataDialog ui;
    int usedSubCls;  //所使用的科目系统类别
    bool isWizard;   //是否在向导中打开此对话框
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
//struct BankInfo{
//    bool isMain;
//    QString name;
//    QString lname;
//    QString rmbAcc;
//    QString usdAcc;
//};

//银行账户设置对话框
class SetupBankDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetupBankDialog(bool isWizared = true, QWidget *parent = 0);
    ~SetupBankDialog();

public slots:
    void newBank();    
    void newAcc();
    void delBank();
    void delAcc();
    void save();
    void selectedBank(const QModelIndex &index);
    void crtSndSubject();
    void selBankAccNum(const QModelIndex &index);

signals:
    void toNextStep(int curStep, int nextStep);

private:
    Ui::SetupBankDialog *ui;
    QSqlTableModel model; //指向Banks表
    QDataWidgetMapper mapper;
    QSqlRelationalTableModel accModel;

    bool isWizared;   //对话框是否是在向党中打开
    int curBankId;    //当前选中的银行ID值
};



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
