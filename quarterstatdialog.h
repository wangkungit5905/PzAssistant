#ifndef QUARTERSTATDIALOG_H
#define QUARTERSTATDIALOG_H

#include <QMetaType>
#include <QDialog>
#include <QHash>

#include "common.h"
#include "commdatastruct.h"
#include "widgets.h"


namespace Ui {
class QuarterStatDialog;
}

class StatUtil;
class Account;
class AccountSuiteManager;
class HierarchicalHeaderView;
class MyWithHeaderModels;

class QuarterStatDialog : public DialogWithPrint
{
    Q_OBJECT
    
public:
    enum TableFormat{
        COMMON = 1,    //通用金额式
        THREERAIL = 2  //三栏式
    };

    enum TableRowType{
        TRT_FSUB    = 1,    //主科目行
        TRT_SSUB    = 2,    //子科目行
        TRT_SUM     = 3     //合计行
    };


    explicit QuarterStatDialog(StatUtil* statUtil, QByteArray* cinfo, QByteArray* pinfo, bool isQuarterStat=true, QWidget *parent = 0);
    ~QuarterStatDialog();
    void print(PrintActionClass action = PAC_TOPRINTER);
    
private slots:
    void onSelFstSub(int index);
    void onSelSndSub(int index);
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);

private:
    void setCommonState(QByteArray* info);
    void setProperState(QByteArray* info);
    void init();
    void initHashs();
    void viewRates();
    void viewTable();
    void genHeaderDatas();
    void genDatas();
    void printCommon(PrintTask task, QPrinter* printer);
    void setTableRowBackground(TableRowType rt, const QList<QStandardItem*> l);
    void setTableRowTextColor(TableRowType rt, const QList<QStandardItem*> l);


    Ui::QuarterStatDialog *ui;
    Account* account;
    StatUtil* statUtil;
    AccountSuiteManager* suiteMgr;
    SubjectManager* smg;
    FirstSubject* fsub;     //当前选择的一二级科目对象
    SecondSubject* ssub;
    bool isQuarterStat;     //季度统计（true）/或年度累计（false）
    QPrinter* printer;

    //与表格有关的数据成员
    HierarchicalHeaderView* hv;
    QStandardItemModel* headerModel;    //表头数据模型
    MyWithHeaderModels* dataModel;

    QHash<int,Double> sRates,eRates; //期初、期末汇率表
    QHash<int,Money*> allMts;      //所有币种代码到币种名称的映射
    QList<int> mts; //外币代码列表（用于保持外币金额显示的一致顺序）取代上面4个列表

    
    //数据表（键为科目id * 10 + 币种代码）
    QHash<int,Double> preExa, preDetExa;                    //期初余额（以原币计）
    QHash<int,Double> preExaR, preDetExaR;                  //期初余额（以本币计）
    QHash<int,MoneyDirection>    preExaDir,preDetExaDir;               //期初余额方向（以原币计）
    QHash<int,MoneyDirection>    preExaDirR,preDetExaDirR;             //期初余额方向（以本币计）
    QHash<int,Double> curJHpn, curJDHpn, curDHpn, curDDHpn; //当期借贷发生额（以原币计）
    QHash<int,Double> curJHpnR, curJDHpnR, curDHpnR, curDDHpnR; //当期借贷发生额（以本币计）
    QHash<int,Double> endExa, endDetExa;                    //期末余额（以原币计）
    QHash<int,Double> endExaR, endDetExaR;                  //期末余额（以本币计）
    QHash<int,MoneyDirection>    endExaDir,endDetExaDir;               //期末余额方向（以原币计）
    QHash<int,MoneyDirection>    endExaDirR,endDetExaDirR;             //期末余额方向（以本币计）

    //表格行背景色
    QColor row_bk_ssub;    //子目行
    QColor row_bk_fsub;    //总目行
    QColor row_bk_sum;     //合计行
};
//Q_DECLARE_METATYPE(QuarterStatDialog::StateInfo)

#endif // QUARTERSTATDIALOG_H
