#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include "ui_pzsearchdialog.h"

class Account;
class SubjectManager;
class AccountSuiteManager;

/**
 * @brief 凭证查找过滤条件结构体
 */
struct PzFindFilteCondition{
    QDate startDate,endDate;    //时间范围
    bool isInvoiceNumInSummary; //摘要中是否包含的是发票号
    QString summary;            //摘要包含的内容
    FirstSubject* fsub;         //一级科目
    SecondSubject* ssub;        //二级科目
    bool isCheckValue;          //是否检测金额
    bool isPreciseMatch;        //金额是否精确匹配
    Double vMax,vMin;           //金额区间（当精确匹配时，使用vMax表示匹配的金额）
    MoneyDirection dir;         //金额的借贷方向（平表示不考虑方向，或两个方向都考虑）
};

#define MAXROWSPERPAGE  50     //搜索结果的每页最大行数

/**
 * @brief 凭证查找分录内容结构体
 */
struct PzFindBaContent{
    QString date;
    int pzNum;
    QString summary;
    int fid;
    int sid;
    Double value;
    MoneyDirection dir;
    int mt;
    int pid,bid;    //凭证、分录的id
};

//凭证搜索对话框类
class PzSearchDialog : public QDialog
{
    Q_OBJECT

    enum TableColumnIndex{
        TI_DATE     = 0,
        TI_PZNUM    = 1,
        TI_SUMMARY  = 2,
        TI_FSUB     = 3,
        TI_SSUB     = 4,
        TI_MONEYTYPE= 5,
        TI_JMONEY   = 6,
        TI_DMONEY   = 7
    };

    enum DataRole{
        DR_PID  = Qt::UserRole+1,
        DR_BID  = Qt::UserRole+2
    };

public:
    explicit PzSearchDialog(Account* account,QByteArray* cinfo, QByteArray* pinfo, QWidget *parent = 0);
    ~PzSearchDialog();
    QByteArray* getCommonState();
    void setCommonState(QByteArray* states);
    QByteArray* getProperState();
    void setProperState(QByteArray* states);

private slots:
    void fsubSelectChanged(int index);
    void startDateChanged(const QDate &date);
    void dateScopeChanged(bool on);
    void moneyMatchChanged(bool on);
    void summaryFilteChanged(bool on);
    void positionPz(QTableWidgetItem * item);
    void on_btnFind_clicked();

    void on_btnPrePage_clicked();

    void on_btnNextPage_clicked();

    void on_btnExpand_toggled(bool checked);

signals:
    void openSpecPz(int pid, int bid); //打开指定id的凭证

private:
    void init();
    void setDateScopeForSuite();
    void enDateScopeEdit(bool en);
    void viewPage(int pageNum);

    Ui::SearchDialog *ui;
    Account* account;
    AccountSuiteManager* sMgr;
    SubjectManager* sm;
    QHash<int,Money*> mts;
    QList<PzFindBaContent*> rs,temRs;
    int curPageNum;     //当前页号（基于1）
    int pageRowCount;   //每页的行数
};

#endif // SEARCHDIALOG_H

