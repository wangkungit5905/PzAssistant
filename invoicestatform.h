#ifndef INVOICESTATFORM_H
#define INVOICESTATFORM_H

#include <QWidget>

class AccountSuiteManager;
class InvoiceRecord;
class PzDialog;

namespace Ui {
class InvoiceStatForm;
}

class InvoiceStatForm : public QWidget
{
    Q_OBJECT

public:
    enum ColumnIndex {
        CL_PZNUM = 0,       //凭证号
        CL_COMMON = 1,      //普票
        CL_INVOICE = 2,     //发票号
        CL_MONEY = 3,       //账面金额
        CL_WMONEY = 4,      //外币金额
        CL_TAXMONEY = 5,    //税额
        CL_STATE = 6,       //销账状态
        CL_CUSTOMER = 7     //关联客户
    };

    explicit InvoiceStatForm(AccountSuiteManager* manager,QWidget *parent = 0);
    ~InvoiceStatForm();
    //实现发票统计记录的排序功能（按发票号、客户名、凭证号），点按列头进行排序

signals:
    void naviToPz(int pzNum, int baID);

private slots:
    void doubleRecord(const QModelIndex &index);


    void on_btnScan_clicked();

private:
    void init();
    void rescan();
    void refreshRow(int row, bool isIncome = true);

    Ui::InvoiceStatForm *ui;
    AccountSuiteManager* amgr;
    PzDialog* parent;
    QList<InvoiceRecord*> incomes,costs;
};

#endif // INVOICESTATFORM_H
