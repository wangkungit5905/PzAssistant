#ifndef JOURNALIZINGPREVIEWDLG_H
#define JOURNALIZINGPREVIEWDLG_H

#include <QDialog>
#include <QStringList>
#include <QTableWidgetItem>
#include <QList>
#include <QSettings>
#include <QShortcut>
#include <QLabel>
#include <QPixmap>

#include "cal.h"
#include "commdatastruct.h"

namespace Ui {
class JournalizingPreviewDlg;
}

class AppConfig;
class AccountSuiteManager;
class SecondSubject;
class SubjectManager;
class FirstSubject;
class SubjectNameItem;
class Journal;
class Journalizing;
class DbUtil;
class JolItemDelegate;
class BusiAction;
class Money;
struct CurInvoiceRecord;
struct InvoiceRecord;


class VerifyTagWidget : public QWidget
{
public:
    VerifyTagWidget(QWidget* parent=0);
    void setDate(QString date){lblDate->setText(date);}
    void setValue(Double v);


private:
    bool isOk;
    Double diffValue;
    QLabel *lblDate;
    QLabel *lblTag;
    QLabel *lblDiffValue;
    QPixmap pixOk,pixNot;
};


class JournalizingPreviewDlg : public QDialog
{
    Q_OBJECT

public:
    enum DataRole{
        DR_JOURNAL = Qt::UserRole,
        DR_JOURNALIZING = Qt::UserRole + 1
    };

    explicit JournalizingPreviewDlg(SubjectManager* sm, QWidget *parent = 0);
    ~JournalizingPreviewDlg();
    bool isDirty();

public slots:
    void save();

private slots:
    void moveNextRow(int row);
    void copyPrewAction(int row);
    void creatNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssub);
    void creatNewSndSubject(int row, int col, FirstSubject* fsub, SecondSubject*& ssub, QString name);

    void BaDataChanged(QTableWidgetItem *item);

    void twContextMenuRequested(const QPoint &pos);
    void currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    //menu item process slots
    void mnuCollectToPz();
    void mnuDissolvePz();
    void mnuInsertBa();
    void mnuRemoveBa();
    void mnuCopyBa();
    void mnuCutBa();
    void mnuPasteBa();
    void mnuMerge();
    void processShortcut();

    void on_btnGenrate_clicked();

    void on_btnClose_clicked();

    void on_tbUp_clicked();

    void on_tbDown_clicked();

    void on_tbGUp_clicked();

    void on_tbGDown_clicked();

private:
    void init();
    void initColors();
    void initSubs();
    void initKeywords();
    void turnWatchData(bool on=true);
    void loadJournals();
    void initBlankBa(int row);
    void rendRow(int row,Journalizing* jo);
    void mergeToGroup();
    void getCurrentGroupRowIndex(int &sr, int &er);
    void calBalanceInGroup(int sr,int er);
    void genarating();
    QString genTerseInvoiceNums(QList<InvoiceRecord*> invoices);
    QStringList extractInvoice(QString t);
    QStringList separateInvoices(QStringList invoices,QList<CurInvoiceRecord*> &incomes,
        QList<CurInvoiceRecord*> &costs, QList<InvoiceRecord*> &yss, QList<InvoiceRecord*> &yfs);
    CurInvoiceRecord* findIncomesOrCosts(QString inum, bool isIncome=true);
    InvoiceRecord* findInYsOrYf(QString inum, bool isYs=true);

    bool isContainKeyword(Journal* j,QStringList kws);
    QList<Journalizing*> processInvoices(int gnum,Journal* j);
    Journal* isBankTranMoney(Journal* j);
    QList<Journalizing*> processBankTranMoney(int gnum, Journal* js, Journal* jd);
    Journal* isWithdrawal(Journal* j);
    QList<Journalizing*> processCash(int gnum, Journal* js, Journal* jd);
    Journal* isBugForex(Journal* j);
    QList<Journalizing*> processBuyForex(int gnum, Journal* js, Journal* jd);
    SecondSubject* isFee(Journal* j);
    QList<Journalizing*> processFee(int gnum, Journal* jd, SecondSubject* ssub);
    SecondSubject* isTax(Journal* j);
    QList<Journalizing*> processTax(int gnum, Journal* jd, SecondSubject* ssub);
    SecondSubject* isSalary(Journal* j);
    QList<Journalizing*> processSalary(int gnum, Journal* jd, SecondSubject* ssub);
    bool isOversea(Journal* j);
    QList<Journalizing*> processOversea(int gnum, Journal* jd);
    QList<Journalizing*> processOther(int gnum, Journal* jd,bool &processed);
    Journalizing* processUnkownJournal(int gnum, Journal* jd);
    QList<Journalizing*> genBaForIncomeOrCost(Journal*j, int gnum, QList<CurInvoiceRecord*> ls);
    Journalizing *genBaForYsOrYf(Journal* jo, int gnum, QList<InvoiceRecord*> ls, bool isYs=true);
    QList<Journalizing*> genDeductionForIncome(Journal *jo, int gnum, QList<CurInvoiceRecord*> costs, QList<InvoiceRecord*> yfs, bool inOrYs=true);
    QList<Journalizing*> genDeductionForCost(Journal *jo, int gnum, QList<CurInvoiceRecord*> incomes, QList<InvoiceRecord*> yss, bool costOrYf=true);
    QList<Journalizing*> genGatherBas(int gnum, bool isIncome = true);

    bool createPzs();

    QString extracBankAccount(QString str){return "";}
    int indexOf(Journal* j);
    SecondSubject *getAdapterSSub(FirstSubject *fsub, QString summary, QString prefixe, QString suffix);

    void updateBas(int row, int rows, BaUpdateColumns col);

    void insertBa(int row, Journalizing *j);
    void moveGroupTo(int sg,int dg);
    void movePzTo(int sp, int dp);
    bool selectedGroups();
    bool isValidInvoiceNumber(QString t);

    Ui::JournalizingPreviewDlg *ui;
    QStringList titles;
    SubjectManager* sm;
    DbUtil* dbUtil;
    int year,month;
    Money* mmt;
    QList<Journal*> js; //
    QList<Journal*> ignoreJs;  //可忽略的流水账（比如：银行间划款等）
    QHash<int,Journal*> jMaps; 
    QList<Journalizing* > jls,doSaves,doRemoves;

    FirstSubject *bankSub,*cashSub,*ysSub,*yfSub,*zysrSub,*zycbSub;
    FirstSubject *cwfySub,*yjsjSub,*xsfySub,*glfySub,*gzSub; //财务费用、应交税金、销售费用、管理费用、应付职工薪酬
    FirstSubject *sszbSub;
    SecondSubject *jxseSSub,*xxseSSub,*hdsySSub,*sxfSSub; //进项、销项、汇兑损益、手续费
    QHash<int,SecondSubject*> bankSubs;
    AccountSuiteManager* smg;
    QList<CurInvoiceRecord *> incomes,costs; //当月收入成本发票记录列表
    QList<InvoiceRecord *> yss,yfs;  //应收应付发票记录列表
    JolItemDelegate* delegate;
    Journalizing* curBa;
    AppConfig* appCfg;
    bool smartSSubSet;      //是否开启智能子目设置功能的标记
    QHash<QString,QString> prefixes,suffixes; //包裹客户名的前缀和后缀，键为科目代码
    QHash<int,Double> rates;     //当前凭证集对应月份的汇率
    bool isInteracting;
    bool isCopy;                        //true：Copy，false：Cut
    QList<Journalizing*> copyOrCutLst;  //拷贝或剪切时临时存放的分录列表
    Journal *jsr_p,*jsr_z,*jcb_p,*jcb_z; //虚拟流水账，分别对应收入普票，收入专票，成本普票，成本专票
    QList<SubjectNameItem*> nameItems;   //系统所有的名称对象
    QList<NameItemAlias *> isolatedAlias;//系统所有的孤立别名

    QList<int> selectedGs;       //选择的组
    QList<int> selectedGroupRows; //选择的组的索引（开始行、结束行），每组占2个元素

    //QShortcut* sc_copyprev;  //复制上一条会计分录
    QShortcut* sc_save;      //保存
    QShortcut* sc_copy;      //拷贝会计分录
    QShortcut* sc_cut;       //剪切会计分录
    QShortcut* sc_paster;    //粘贴会计分录

    //关键字列表
    QSettings *kwSetting;
    QStringList kwCash;  //现金存取款
    QStringList kwBankTranMoney; //银行间划款
    QStringList kwBuyForex;  //购汇结汇
    QStringList kwOversea;   //国外账
    QStringList kwSalary;    //工资
    QStringList kwServiceFee;//金融机构各种服务费
    QStringList kwInvest;    //投资款
    QStringList kwPreInPay;  //预收预付
    //QStringList kwOthers;    //其他可以专门处理的

    //colors
    QColor color_pzNum1;
    QColor color_pzNum2;
    QColor color_gNum1;
    QColor color_gNum2;
};

bool journalThan(Journal *j1, Journal *j2);
bool incomeInvoiceThan(CurInvoiceRecord* i1, CurInvoiceRecord* i2);
bool costInvoiceThan(CurInvoiceRecord* i1, CurInvoiceRecord* i2);

//手续费, 
//工资
//车费, 餐费, 通信费, 吊机费, 住宿费, 物业, 顺丰, 申通, 办公用品, 电讯费（财务费用）
//税金, 公积金, 待报解预算收入（增值税, 个人所得税, 医疗保险, 养老金+失业险+工伤险+生育险, 地方教育附加+教育费附加+城建税, 残疾人就业保障金, ）
//国外账（发票号为00000000）

//如果摘要栏的客户名无法确定，则可以通过发票号推导

#endif // JOURNALIZINGPREVIEWDLG_H


