#ifndef BATEMPLATEFORM_H
#define BATEMPLATEFORM_H

#include <QWidget>
#include <QItemDelegate>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidgetItem>
#include "commdatastruct.h"


namespace Ui {
class BaTemplateForm;
}

class QPushButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QTableWidgetItem;
class QActionGroup;

class SubjectManager;
class AccountSuiteManager;
class PzDialog;
class BaTemplateForm;

/**
 * @brief 保存分录模板窗口中的表格数据（兼具暂存区数据保存）
 */
struct InvoiceRowStruct{
    int month;
    Double money,taxMoney,wMoney;
    SecondSubject* ssub;
    QString inum;
    QString sname,lname,remCode;  //这些仅在临时科目时使用
};

/**
 * @brief 模板类型枚举
 */
enum BaTemplateType{
    BTT_YS_GATHER = 1,
    BTT_YF_GATHER = 2
};

/**
 * @brief 抵扣发票类型
 * 收入：应付抵扣收入，成本抵扣收入
 * 应收：应付抵扣应收，成本抵扣应收
 * 成本：应收抵扣成本，收入抵扣成本
 * 应付：应收抵扣应付，收入抵扣应付
 */
enum InvoiceItemType{
    IIT_COMMON  = 0,    //不作为抵扣发票
    IIT_YF2IN   = 1,    //应付抵扣收入
    IIT_COST2IN = 2,    //成本抵扣收入
    IIT_YF2YS   = 3,    //应付抵扣应收
    IIT_COST2YS = 4,    //成本抵扣应收
    IIT_YS2COST = 5,    //应收抵扣成本
    IIT_IN2COST = 6,    //收入抵扣成本
    IIT_YS2YF   = 7,    //应收抵扣应付
    IIT_IN2YF   = 8,    //收入抵扣应付
    IIT_YS      = 9,    //作为应收
    IIT_YF      = 10    //作为应付
};

/**
 * @brief 用于输入发票号，可以识别断续型或连续型的发票号输入方式或两者的混合
 */
class InvoiceNumberEdit : public QLineEdit
{
    Q_OBJECT
public:
    InvoiceNumberEdit(QWidget* parent=0);

private slots:
    void invoiceEditCompleted();

signals:
    //参数col表示编辑器所在列索引，isMove指示是否将输入焦点移动到下一个项目
    void dataEditCompleted(int col, bool isMove);
    //当输入多张发票时触发此信号，以自动插入多行
    void inputedMultiInvoices(QStringList invoices);

private:
    QRegExp re;
};

/**
 * @brief 用于编辑金额
 */
class MoneyEdit : public QLineEdit
{
    Q_OBJECT
public:
    MoneyEdit(QWidget* parent=0);
    void setColIndex(int index){colIndex=index;}
    void setLastCell(bool yes){isRightBottomCell=yes;}

private slots:
    void moneyEditCompleated();
signals:
    void dataEditCompleted(int col, bool isMove);

private:
    Double v;
    int colIndex;
    bool isRightBottomCell;
};

class CustomerNameEdit : public QWidget
{
    Q_OBJECT
public:
    CustomerNameEdit(SubjectManager* subMgr, QWidget* parent=0);
    void setRowNum(int rowNum){row=rowNum;}
    void setCustomerName(QString name);
    QString customerName();
    SecondSubject* subject(){return ssub;}
    void setFSub(FirstSubject* fsub, const QList<SecondSubject *> &extraSubs);
    void setSSub(SecondSubject* ssub);
    void hideList(bool isHide);
    //void setLastRow(bool yes){isLastRow=yes;}
    void setEditText(QString t);

private slots:
    void itemSelected(QListWidgetItem* item);
    void nameTextChanged(const QString& text);
    void nameTexteditingFinished();
    void subSelectChanged(int index);

signals:
    void newMappingItem(FirstSubject* fsub, SubjectNameItem* ni, SecondSubject*& ssub,int row, int col);
    void newSndSubject(FirstSubject* fsub, SecondSubject*& ssub, QString name, int row, int col);
    void dataEditCompleted(int col, bool isMove);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    bool processArrowKey(bool up);
    void filterListItem();
    SecondSubject *getMatchExtraSub(SubjectNameItem* ni);
    int insertExtraNameItem(SubjectNameItem* ni);

    FirstSubject* fsub;
    SecondSubject* ssub;
    SubjectManager* sm;
    SortByMode sortBy;
    QString keys;
    QList<SubjectNameItem*> allNIs;  //所有名称条目
    QList<SecondSubject*> extraSSubs;//额外的二级科目
    QComboBox* com;       //显示当前一级科目下的可选的二级科目的组合框
    QListWidget* lw;      //智能提示列表框（显示所有带有指定前缀的名称条目）
    int row;              //编辑器所在行号（基于0）
};

class InvoiceInputDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    InvoiceInputDelegate(SubjectManager* subMgr,QWidget* parent=0);
    void setTemplateType(BATemplateEnum type);
    void setvalidColumns(int colNums){validColumns=colNums;}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
    void destroyEditor(QWidget * editor, const QModelIndex & index) const;
    void userConfirmed(){canDestroy=true;}
    void laterDestroyEditor(){canDestroy=false;}

private slots:
    void commitAndCloseEditor(int colIndex, bool isMove);
    void multiInvoices(QStringList invoices);
    void newNameItemMapping(FirstSubject* fsub, SubjectNameItem* ni, SecondSubject*& ssub,int row, int col);
    void newSndSubject(FirstSubject* fsub, SecondSubject*& ssub, QString name, int row, int col);

signals:
    void reqCrtMultiInvoices(QStringList invoices);
    void crtNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssbu);
    void crtNewSndSubject(int row, int col, FirstSubject* fsub, SecondSubject*& ssub, QString name);
private:
    bool isDoubleColumn(int col) const;

    BaTemplateForm* p;
    FirstSubject* fsub;
    BATemplateEnum templae;   //当前处理的模板类型
    SubjectManager* sm;
    int validColumns;
    bool canDestroy;      //对象是否可以销毁（当创建新科目时，利用此标记延迟对象的销毁）
};

class InvoiceTableItem : public QTableWidgetItem
{
public:

    InvoiceTableItem(QString text="", InvoiceItemType itype=IIT_COMMON, int type=UserType+100);
    QVariant data(int role) const;
    void setType(InvoiceItemType type);
    InvoiceItemType invoiceType(){return _type;}

private:
    InvoiceItemType _type;
    QIcon bankIcon,ysyfIcon;
};

/**
 * @brief 分录模板类
 */
class BaTemplateForm : public QWidget
{
    Q_OBJECT

public:
    enum ColumnIndex{
        CI_MONTH = 0,       //发票所属月份
        CI_INVOICE = 1,     //发票号
        CI_MONEY = 2,       //账面金额
        CI_TAXMONEY = 3,    //税额
        CI_WMONEY = 4,      //外币金额
        CI_CUSTOMER = 5     //关联客户
    };

    enum CustomerType {
        CT_SINGLE = 1,  //单客户
        CT_MULTI = 2    //多客户
    };



    explicit BaTemplateForm(AccountSuiteManager* suiterMgr, QWidget *parent = 0);
    ~BaTemplateForm();
    void setTemplateType(BATemplateEnum type);
    void clear();
    QList<SecondSubject*> getExtraSSubs(){return extraSSubs;}

private slots:
    void moneyTypeChanged(int index);
    void customerChanged(int index);
    void DataChanged(QTableWidgetItem *item);
    void addNewInvoice();
    void createMultiInvoice(QStringList invoices);
    void creatNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssub);
    void creatNewSndSubject(int row, int col, FirstSubject* fsub, SecondSubject*& ssub, QString name);
    void doubleClickedCell(const QModelIndex& index);
    void contextMenuRequested(const QPoint &pos);
    void processContextMenu();
    void dkProcess();

    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_btnSave_clicked();

    void on_btnLoad_clicked();

private:
    void init();
    void initRow(int row);
    void changeCustomerType(CustomerType type);
    void reCalSum(ColumnIndex col);
    void reCalAllSum();
    void turnDataInspect(bool on=true);
    bool invoiceQualified(QString inum);
    int invoiceQualifieds();
    void autoSetYsYf(int row, QString inum, bool isYs=true);
    void autoSetInCost(int row, QString inum, bool isIncome=true);
    void setYsYfMoney(int row, SecondSubject* ssub);
    void createBankIncomeBas();
    void createBankCostBas();
    void createYsBas();
    void createYfBas();
    void createYsGatherBas();
    void createYfGatherBas();
    QList<int> selectedRows();
    void copyRow(int row);
    QString dupliInvoice();


    void initBankIncomeTestData();
//    void initBankCostTestData();
//    void initYsTestData();
//    void initYfTestData();
//    void initYsGatherTestData();
//    void initYfGatherTextData();
    //void testInsertMultiBAs();

    Ui::BaTemplateForm *ui;
    SubjectManager* sm;
    AccountSuiteManager* amgr;
    PingZheng* pz;
    PzDialog* pzDlg;
    InvoiceInputDelegate* delegate;
    FirstSubject* bankFSub,*ysFSub,*yfFSub,*sjFSub,*srFSub,*cbFSub,*cwFSub;
    SecondSubject *xxSSub,*jxSSub,*hdsySSub,*srDefSSub,*cbDefSSub,*sxfSSub;
    SecondSubject *bankSSub,*curCusSSub; //当前选择的银行子目，当前选择的客户子目（在单客户类型即银行收入或成本类型模板）
    Money *mmt,*mt,*wmt;     //本币，当前银行科目涉及的币种、外币（美金）
    QPushButton* btnAdd;
    int cw_invoice,cw_money/*,cw_customer*/; //列宽
    QList<SecondSubject*> extraSSubs;   //额外二级科目（如果模板类型是应收应付型则这些科目
                                        //在创建凭证前需要真正创建，否则只作为临时科目用于
                                        //后续的选择，不至于多次提示，这些科目没有设置其所属主科目）
    bool ok;    //如果创建过程一切OK，则在按确定按钮后关闭窗口，否则不关闭
    QList<InvoiceRowStruct*> buffers; //表格行的拷贝缓冲区
    QActionGroup *ag_in, *ag_ys, *ag_cost,*ag_yf;
    QIcon icon_question;
};

#endif // BATEMPLATEFORM_H
