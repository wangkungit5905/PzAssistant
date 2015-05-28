#ifndef YSYFINVOICESTATFORM_H
#define YSYFINVOICESTATFORM_H

#include <QWidget>
#include <QItemDelegate>
#include <QComboBox>
#include <QTableWidgetItem>
#include "commdatastruct.h"

namespace Ui {
class YsYfInvoiceStatForm;
}

class AccountSuiteManager;
struct InvoiceRecord;

class InvoiceStateWidget : public QTableWidgetItem
{
public:
    InvoiceStateWidget(CancelAccountState state, int type = QTableWidgetItem::UserType);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    QString getStateText(CancelAccountState state) const;
    CancelAccountState _state;
};

class InvoiceStateEditor : public QComboBox
{
    Q_OBJECT
public:
    InvoiceStateEditor(QWidget* parent=0);
    void setState(CancelAccountState state);
    CancelAccountState state(){return _state;}
private slots:
    void stateChanged(int index);
private:
    CancelAccountState _state;
};

class YsYfInvoiceTableDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    YsYfInvoiceTableDelegate(QWidget* parent=0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
};

class YsYfInvoiceStatForm : public QWidget
{
    Q_OBJECT
public:
    enum ColumnIndex {
        CI_MONTH = 0,
        CI_INVOICE = 1,
        CI_MONEY = 2,
        CI_TAXMONEY = 3,
        CI_WMONEY = 4,
        CI_STATE = 5,
        CI_CUSTOME = 6,
        CI_PZNUMBER = 0,
        CI_BANUMBER = 1,
        CI_ERROR = 2
    };

    explicit YsYfInvoiceStatForm(AccountSuiteManager* amgr,bool init = false,QWidget *parent = 0);
    ~YsYfInvoiceStatForm();

private slots:
    void contextMenuRequested(const QPoint &pos);
    void doubleClicked(const QModelIndex &index);
    void dataChanged(QTableWidgetItem *item);
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_actDel_triggered();

signals:
    void openSpecPz(int pid, int bid); //打开指定id的凭证

private:
    void init();
    void initCurMonth();
    void inspectDataChanged(bool on = true);
    bool exist(InvoiceRecord* r, bool &changed, bool isYs=true);
    bool isCancel(InvoiceRecord* r,QStringList &errors, bool isYs=true);
    void clear();
    void viewSingleRow(int row,InvoiceRecord* r,bool isYs=true);
    void viewRecords();
    void viewErrors();
    void extractPzNum(QString errorInfos, int &pzNum, int &baNum);


    Ui::YsYfInvoiceStatForm *ui;
    AccountSuiteManager* amgr;
    QList<InvoiceRecord *> incomes;
    QList<InvoiceRecord *> costs;
    QList<InvoiceRecord *> curIncomes,curCosts; //保存本月的记录,尽在初始化期间使用
    QList<InvoiceRecord *> dels;    //保存已不存在的记录
    QList<InvoiceRecord *> temrs;   //临时保存被添加的记录
    QStringList errors;
    YsYfInvoiceTableDelegate* delegate;
};

bool invoiceRecordCompareByDate(InvoiceRecord* r1, InvoiceRecord* r2);
bool invoiceRecordCompareByNumber(InvoiceRecord* r1, InvoiceRecord* r2);
bool invoiceRecordCompareByCustomer(InvoiceRecord* r1, InvoiceRecord* r2);

#endif // YSYFINVOICESTATFORM_H
