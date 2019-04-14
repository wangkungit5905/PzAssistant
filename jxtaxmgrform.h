#ifndef JXTAXMGRFORM_H
#define JXTAXMGRFORM_H

#include <QDialog>
#include "cal.h"

namespace Ui {
class JxTaxMgrForm;
}

class QTableWidgetItem;
class Account;
class AccountSuiteManager;
class SecondSubject;
class SubjectManager;
class SubjectNameItem;

/**
 * @brief 本月认证成本发票信息结构
 */
struct CurAuthCostInvoiceInfo{
    int id;
    bool edited;
    bool isCur;                 //true：本月计入，false：历史调整计入
    Double taxMoney,money;      //税额，发票金额
    QString inum,cName;         //发票号码，发票客户名
    SubjectNameItem* ni;        //和客户对应的名称条目
    CurAuthCostInvoiceInfo(){
        id = 0;
        isCur = true;
        edited = false;
    }
};

/**
 * @brief 历史未认证成本发票信息结构
 */
struct HisAuthCostInvoiceInfo{
    int id;
    QString pzNum,iNum,date;     //凭证号（形如xxxx年xx月xxx#），发票号，开票日期
    int baId;                    //对应分录id
    Double taxMoney,money;       //发票税额，发票金额
    SecondSubject* client;       //发票客户对应二级科目
    QString summary;             //暂存税金时使用的摘要信息
};

class JxTaxMgrDlg : public QDialog
{
    Q_OBJECT

    //历史未认证发票表格列索引
    enum ColumnIndexHistory {
        CIH_PZ               = 0,
        CIH_DATE          = 1,
        CIH_INVOICE    = 2,
        CIH_TAX            = 3,
        CIH_MONEY    = 4,
        CIH_CLIENT      = 5,
    };
    //本月新增成本专票表格列索引
    enum ColumnIndexCurrent {
        CIC_PZ         = 0,
        CIC_NUMBER     = 1,
        CIC_TAX       = 2,
        CIC_CLIENT = 3
    };
    //本月认证发票表格列索引
    enum ColumnIndexCurAuth{
        CICA_ISCUR  = 0,
        CICA_NUM    = 1,
        CICA_TAX    = 2,
        CICA_MONEY  = 3,
        CICA_CLIENT = 4
    };

    enum DataRole{
        DR_BAID = Qt::UserRole,                 //记载此发票税金的分录id
        DR_VERIFY_STATE = Qt::UserRole+1        //保存验证状态
    };

    //验证结果
    enum VerifyState{
        VS_OK   = 1,          //正确
        VS_ADJUST = 2,        //未认证但却被记录进项税
        VS_ERROR = 3,         //已认证，但金额或科目设置有误，或在历史表中存在，但金额或科目设置有误
        VS_NOEXIST = 4        //已认证，但本月未计入进项税，或在历史表中不存在
    };

public:
    explicit JxTaxMgrDlg(Account* account,QWidget *parent = 0);
    ~JxTaxMgrDlg();

private slots:
    void initHistoryDatas();
    void scanCurrentTax();
    void cusContextMenuRequested(const QPoint &pos);
    void curAuthMenyRequested(const QPoint &pos);
    void addCurAuthInvoice();
    void DataChanged(QTableWidgetItem *item);
    void on_btnApply_clicked();

    void on_btnVerify_clicked();

    void on_actAddHis_triggered();

    void on_actDelHis_triggered();

    void on_btnInitHis_clicked();

    void on_btnSaveHis_clicked();

    void on_btnSaveCur_clicked();

    void on_chkInited_clicked(bool checked);

    void on_actDelCur_triggered();

    void on_btnCrtPz_clicked();

    void on_actJrby_triggered();

private:
    void init();
    void showCaInvoices();
    void calCurAuthTaxSum();
    void showHaInvoices();
    //void VerifyCurInvoices();
    void turnDataInspect(bool on=true);
    VerifyState isInvoiceAuthed(QString inum, Double tax);
    VerifyState inHistory(QString num,SecondSubject* sub,Double tax);
    void setInvoiceColor(QTableWidgetItem* item,VerifyState state);

    Ui::JxTaxMgrForm *ui;
    QPushButton* btnAddCa;                                       //新增本月认证发票按钮
    Account* account;
    AccountSuiteManager* asMgr;
    SubjectManager* sm;
    QList<CurAuthCostInvoiceInfo*> caInvoices;       //本月认证发票
    QList<HisAuthCostInvoiceInfo*> haInvoices,haInvoices_del;       //历史未认证发票，被删除的记录缓存
    Double taxAmount;                                                  //本月认证发票税金总额

    QColor c_ok,c_adjust,c_error,c_noexist;
};

#endif // JXTAXMGRFORM_H
