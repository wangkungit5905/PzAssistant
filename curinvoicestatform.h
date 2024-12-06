#ifndef CURINVOICESTATFORM_H
#define CURINVOICESTATFORM_H


#include "xlsxdocument.h"
#include "cal.h"
#include "commdatastruct.h"

#include <QWidget>
#include <QTableWidgetItem>
#include <QItemDelegate>
#include <QDialog>

namespace Ui {
class CurInvoiceStatForm;
}

using namespace QXlsx;

class QTableWidget;
class QListWidgetItem;
class QActionGroup;
class QMenu;
class QPushButton;
class QListWidget;
class SubjectManager;
class AccountSuiteManager;
class Account;
class SubjectNameItem;

enum ClientMatchState{
    CMS_NONE        = 0,    //未匹配
    CMS_PRECISE     = 1,    //精确匹配（客户名与名称对象的全称一致）
    CMS_ALIAS       = 2,    //别名匹配（客户名与名称对象的某个别名一致）
    CMS_NEWCLIENT   = 3     //新客户（无法找到与之匹配的名称对象）
};

/**
 * @brief 显示发票客户的表格项类
 */
class InvoiceClientItem : public QTableWidgetItem
{
public:
    InvoiceClientItem(QString name="", int type = UserType);
    void setText(QString text);
    void setNameItem(SubjectNameItem* ni);
    SubjectNameItem* nameObject(){return ni;}
    QVariant data(int role) const;
    void setData(int role, const QVariant &value);
    //ClientMatchState matchState(){return _matchState;}
    //void setMatchState(ClientMatchState state){_matchState=state;}
private:
    QString name;
    SubjectNameItem* ni;
    ClientMatchState _matchState;
    QColor mc_precise,mc_alias,mc_new,mc_none;
};

/**
 * @brief 多状态表格项类（用于显示发票状态或发票类型）
 */
class MultiStateItem : public QTableWidgetItem
{
public:
    MultiStateItem(const QHash<int,QString> &stateNames, bool isForeground=true, int type = UserType+1);
    void setStateColors(const QHash<int,QColor> &colors){stateColors=colors;}
    QVariant data(int role) const;
    void setData(int role, const QVariant &value);
private:
    int stateCode;
    bool isForeground;              //通过前景色/背景色表达不同的状态
    QHash<int,QString> stateNames;  //状态名
    QHash<int,QColor> stateColors;  //状态前景色
};


/**
 * @brief 显示发票处理情况的表格项类
 */
class InvoiceProcessItem : public QTableWidgetItem
{
public:
    InvoiceProcessItem(int stateCode,int type = UserType+2);
    QVariant data(int role) const;
    void setProcessState(int state);
    int processState(){return stateCode;}
    void setErrorInfos(QString infos){errInfos=infos;}
private:
    int stateCode;
    QString errInfos;
    QHash<int,QIcon> icons;
};

/**
 * @brief 人工介入客户匹配窗口类
 */
class HandMatchClientDialog : public QDialog
{
    Q_OBJECT
public:
    HandMatchClientDialog(SubjectNameItem* nameItem, SubjectManager* subMgr, QString name, QWidget* parent=0);
    void setClientName(SubjectNameItem* ni, QString name="");
    QString clientName();
protected:
    bool eventFilter(QObject *obj, QEvent *event);
private slots:
    void nameEditFinished();
    void btnOkClicked();
    void keysChanged();
    void confirmNewClient();
signals:
    void clinetNameChanged(QString oldName,QString newName);   //用户改变了客户的名称，但没有改变匹配状态
    void clientMatchChanged(QString clientName, SubjectNameItem* ni); //客户名匹配到现存的一个名称对象
    void createNewClientAlias(NameItemAlias* alias); //为客户名创建一个新的孤立别名来匹配
private:
    void refreshList(QString keys="");

    QLineEdit* nameEdit;
    QListWidget* lwNames;
    QPushButton *btnOk,*btnCancel,*btnNewClient;
    QString name;
    SubjectNameItem* ni;
    SubjectManager* sm;

};

class InvoiceInfoDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    InvoiceInfoDelegate(QWidget* parent=0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
};



class CurInvoiceStatForm : public QWidget
{
    Q_OBJECT

public:
    enum ItemDataRole{
        DR_READED   = Qt::UserRole,     //表单是否已读取（bool）
        DR_ITYPE    = Qt::UserRole + 1, //发票类型（0:未指定，1：应收普票，2：应收专票，3：应付）
        DR_STARTROW = Qt::UserRole + 2, //表单数据开始行，基于0
        DR_ENDROW   = Qt::UserRole + 3  //表单数据结束行
    };

    enum InvoiceType{
        IT_NONE   = 0,    //未指定
        IT_INCOME_COMMON = 1,    //收入（普票）
        IT_INCOME_SPECIAL = 2,    //收入（专票）
        IT_COST   = 3     //成本
    };

    enum RowType{
        RT_NONE     = 0,    //普通行
        RT_START    = 1,    //数据开始行
        RT_END      = 2     //数据结束行
    };

    enum TableIndex{
        TI_NUMBER   = 0,    //序号
        TI_DATE     = 1,    //开票日期
        TI_INUMBER  = 2,    //发票号码
        TI_MONEY    = 3,    //发票金额
        TI_TAXMONEY = 4,    //税额
        TI_WBMONEY  = 5,    //外币金额
        TI_ITYPE   = 6,    //发票类型（专/普）
        TI_STATE    = 7,    //发票属性（正常、冲红、作废）
        TI_SFINFO   = 8,    //收款情况
        TI_ISPROCESS= 9,    //本期是否已处理
        TI_CLIENT   = 10,   //客户名
        TI_SORT_NUM = 11,   //仅用于按序号排序的隐藏列
        TI_SORT_PRIMARY = 12 //原始导入顺序隐藏列
    };

    enum TableSortColumn{
        TSC_PRIMARY = 1,    //原始导入顺序
        TSC_NUMBER  = 2,    //按序号
        TSC_INUMBER = 3,    //按发票号
        TSC_NAME    = 4     //按客户名
    };

    explicit CurInvoiceStatForm(Account* account,QWidget *parent = 0);
    ~CurInvoiceStatForm();

private slots:
    void doubleItemReadSheet(QListWidgetItem *item);
    void curSheetChanged(int index);
    void sheetListContextMeny(const QPoint &pos);
    void tableHHeaderContextMenu(const QPoint &pos);
    void tableVHeaderContextMenu(const QPoint &pos);
    void processYsYfSelected(bool checked);
    void processColTypeSelected(bool checked);
    void processRowTypeSelected(bool checked);
    void InvoiceTableHeaderContextMenu(const QPoint &pos);
    QString getColTypeText(CurInvoiceColumnType colType);
    QString getRowTypeText(RowType rowType);
    void invoiceInfoChanged(QTableWidgetItem * item);
    void itemDoubleClicked(QTableWidgetItem * item);
    void clientNameChanged(QString oldName,QString newName);
    void clientMatchChanged(QString clientName,SubjectNameItem* ni);
    void createNewClientAlias(NameItemAlias* alias);
    void curTableChanged(int index);
    void sortColumnChanged(bool on);
    void enanbleFilter(bool on);
    void filteTextChanged(QString text);
    void on_btnExpand_toggled(bool checked);

    void on_btnImport_clicked();

    void on_btnBrowser_clicked();

    void on_btnSave_clicked();

    void on_actClearInvoice_triggered();

    void on_actAutoMatch_triggered();

    void on_btnVerify_clicked();

    void on_actResolveCol_triggered();

signals:
    void openRelatedPz(int pid,int bid);

private:
    void init();
    void initKeys();
    void initColMaps();
    void initTable();
    void switchHActions(bool on=true);
    void switchVActions(bool on=true);
    void switchSortChanged(bool on=true);
    void switchInvoiceInfo(bool on=true);
    void resetTableHeadItem(CurInvoiceColumnType colType, QTableWidget* tw);
    void expandPreview(bool on = true);
    bool readSheet(int index, QTableWidget* tw);
    CurInvoiceColumnType columnTypeForText(QString text);
    bool calFormula(QString formula, Double &v, QTableWidget *tw);
    bool inspectColTypeSet(QList<CurInvoiceColumnType> colTypes,QString &info);
    bool inspectTableData(bool isYs);
    void showInvoiceInfo(int row,CurInvoiceRecord* r);
    QString padZero(int num);

    Ui::CurInvoiceStatForm *ui;
    QXlsx::Document *excel;
    QActionGroup *ag1,*ag2,*ag3; //组合管理列类型设置
    QMenu *mnuRowTypes, *mnuColTypes; //设置行、列类型的上下文菜单
    QHash<CurInvoiceColumnType,QStringList> colTitleKeys;     //特定列所对应的敏感名称
    Account* account;
    SubjectManager* sm;
    AccountSuiteManager* suiteMgr;
    InvoiceInfoDelegate* delegate;
    QList<CurInvoiceRecord *> *incomes,*costs;  //本地保存的发票记录
    TableSortColumn sort_in,sort_cost;          //收入/成本表格当前的排序列
    HandMatchClientDialog* handMatchDlg;
    int clientClsId;      //业务客户类别id

    int contextMenuSelectedCol;     //记录表格的水平表头最近一次上下文菜单所选择的列
    QIcon icon_income,icon_cost;
    QHash<int,QString> invoiceStates,invoicesClasses; //发票状态和类别名表
    QHash<int,QColor> invoiceStateColors;
    QHash<int,QColor> invoiceClassColors;  //表达普票/专票的颜色
};

#endif // CURINVOICESTATFORM_H
