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

class CreateAccountDialog : public QDialog
{
    Q_OBJECT

public:
    CreateAccountDialog(bool isWizarded = true, QWidget* parent = 0);
    //    CreateAccountDialog(QString sname, QString lname,
    //                        QString filename, QWidget* parent);

    QString getCode();
    QString getSName();
    QString getLName();
    QString getFileName();
    int getReportType();

signals:
    void toNextStep(int curStep, int nextStep);

public slots:
    void nextStep();

private:
    Ui::Dialog ui;
    bool isWizared;
    QSqlTableModel* model;
    QDataWidgetMapper* mapper;
};


class OpenAccountDialog : public QDialog
{
    Q_OBJECT

public:
    OpenAccountDialog(QWidget* parent = 0);
    QString getSName();
    QString getLName();
    QString getFileName();
    int getAccountId();
    //int getUsedSubSys();

public slots:
    void itemClicked(const QModelIndex &index);
    void doubleClicked(const QModelIndex & index);
private:
    //QString sname, lname, fname;
    Ui::OpenAccountDialog ui;
    QStringListModel* model;
    //PSetting setting;
    QList<AccountBriefInfo*> accInfoLst;
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

class CollectPzDialog : public QDialog
{
    Q_OBJECT

public:
    CollectPzDialog(QWidget* parent = 0);

public slots:
    void calBtnClicked();
    void dateChanged(const QDate &date);
    void saveExtras();
    void selSubject(const QModelIndex &index);
    void toExcel();
    void setDateLimit(int sy,int sm,int ey,int em);

private:
    void initRates(int year,int month);
    void initHashs();
    int genKey(int id, int code){return id * MAXMT + code;}

    QSqlQueryModel* model;  //获取凭证信息
    //QSqlQueryModel* model1; //获取凭证分册类别内容
    QSqlQueryModel* model2; //获取业务活动内容
    QStandardItemModel* fmodel; //一级科目的汇总数据
    QStandardItemModel* smodel; //明细科目的汇总数据(按币种和明细科目)
    QStandardItemModel* smmodel; //明细科目的汇总数据（按币种）

    QSqlQuery query;
    Ui::collectPzDialog ui;

    QHash<int, QString> fhash; //一级科目id到一级科目名称的映射
    QHash<int, QString> chash; //一级科目id到一级科目代码的映射
    QSet<int> detSubSet; //需要进行明细核算的一级科目id集合
    QHash<int,QString> shash; //明细科目Id（FSAgent表的id列）到科目名的映射
    //QHash<int,double> sumByMt; //币种代码到对应合计值的映射（在计算明细科目合计值时要根据币种来分开合计）
    QMap<int,QString> mtMap; //币种代码到名称的映射
    //QHash<int,double> fsums; //本期一级科目的合计值，保存科目余额时使用
    QHash<int,double> subSums; //明细科目合计值表(key = 明细科目id x 10 + 币种代码)

    QDate selDate;      //选择的日期
    int selBookType;    //选择的凭证分册类型
    double rate;        //汇率
    QHash<int,double> rates; //汇率表，币种代码到汇率的映射
    PzCollectProcess pzcp; //在一个类里面处理所有一级科目的汇总值
};

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

/**
    该类的设计目标是显示指定月份的总子科目的余额，月份可以选择，但余额数据不能编辑。
    它提供了与本月统计类似的功能，但它能任意选择某个时间点来显示，而本月统计一般用
    来显示当月余额数。
*/
class SubjectExtraDialog : public QDialog
{
    Q_OBJECT

public:
    SubjectExtraDialog(QWidget* parent = 0);
    void setDate(int y,int m);

public slots:
    void viewExtra();
    void btnEditClicked();
    void btnSaveClicked();
    void viewSubExtra(QString code, const QPoint& pos);
    void dateChanged(const QDate &date);


private:
    Ui::subjectExtraDialog ui;
    //QSqlTableModel* model;
    //QSqlQueryModel* model;
    //QDataWidgetMapper* mapper;
    QHash<QString, QLineEdit*> edtHash;  //对象名与部件指针的对应表
    QHash<int,QLineEdit*> idHash;        //科目id到显示该科目的余额值的部件指针的对应表
    QHash<int, QGridLayout*> lytHash;    //科目类别代码到布局容器对象指针的映射

    int y,m;

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
//显示子目余额值的对话框类
class DetailExtraDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DetailExtraDialog(QString subCode, int year,
                               int month, QWidget *parent = 0);
    void setOnlyView();
    ~DetailExtraDialog();

public slots:
    void saveDetailExtra();
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

signals:
    void extraValueChanged();

private:
    Ui::DetailExtraDialog *ui;
    int year, month;  //余额所属年月
    QString subCode;  //明细科目所属的一级科目的代码
    double oldValue;  //在SubjectExtras表中的老值(总账科目余额值)
    //bool isExist;     //是否在SubjectExtras表中存在对应年月的记录条目
    QStandardItemModel* model;
    QHash<int,double> rates;  //汇率表
    QString tsName; //总账科目余额在SubjectExtras表中的对应字段名
};


#endif // DIALOGS_H
