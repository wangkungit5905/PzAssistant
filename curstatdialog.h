#ifndef CURSTATDIALOG_H
#define CURSTATDIALOG_H

#include <QMetaType>
#include <QDialog>
#include <QHash>

#include "common.h"
#include "commdatastruct.h"
#include "widgets.h"


namespace Ui {
class CurStatDialog;
}

class QMenu;
class StatUtil;
class Account;
class QStandardItem;
class QStandardItemModel;
class HierarchicalHeaderView;
class MyWithHeaderModels;
class FirstSubject;
class SecondSubject;
class Money;
class Double;
class SubjectManager;



/**
 * @brief The CurStatDialog class
 *  本期统计对话框类（统计数据来自于StatUtil类）
 */
class CurStatDialog : public DialogWithPrint
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

    //状态信息结构
    struct StateInfo{
        TableFormat tFormat;  //最后关闭时，显示的表格格式
        bool viewDetails;     //最后关闭时，是否选择了显示明细选择框
        QPrinter::Orientation pageOrientation;  //打印模板的页面方向
        PageMargin margins;                        //页边距
        QHash<TableFormat, QList<int> > colWidths; //各种表格格式下的列宽
        QHash<TableFormat, QList<int> > colPriWidths; //打印各种表格格式时的列宽
    } stateInfo;

    explicit CurStatDialog(StatUtil* statUtil, QByteArray* sinfo = NULL, QWidget *parent = 0);
    ~CurStatDialog();
    void setState(QByteArray* info);
    QByteArray* getState();
    void stat();
    void print(PrintActionClass action = PAC_TOPRINTER);

public slots:
    void save();
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);

private slots:
    void onSelFstSub(int index);
    void onSelSndSub(int index);
    void onTableFormatChanged(bool checked);
    void onDetViewChanged(bool checked);
    
    void on_actPrint_triggered();

    void on_actPreview_triggered();

    void on_actToPDF_triggered();

    void on_actToExcel_triggered();

    void on_btnSave_clicked();

    void on_btnRefresh_clicked();

    void on_btnClose_clicked();

signals:
    void infomation(QString info);       //向主窗口发送要在状态条上显示的信息
    //void pzsExtraSaved();

private:
    void init(Account* acc);
    void initHashs();
    void viewRates();
    void viewTable();
    void genHeaderDatas();
    void genDatas();
    void printCommon(PrintTask task, QPrinter* printer);
    void setTableRowBackground(TableRowType rt, const QList<QStandardItem*> l);
    void setTableRowTextColor(TableRowType rt, const QList<QStandardItem*> l);

    Ui::CurStatDialog *ui;
    Account* account;
    StatUtil* statUtil;

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

    QMenu* mnuPrint; //附加在打印按钮上的菜单

    //SubjectComplete *fcom, *scom;  //一二级科目选择框使用的完成器
    FirstSubject* fsub;     //当前选择的一二级科目对象
    SecondSubject* ssub;
    bool isCanSave; //是否可以保存余额（基于当前的凭证集状态）

    SubjectManager* smg;

    //表格行文本色
    QColor row_tc_ssub;
    QColor row_tc_fsub;
    QColor row_tc_sum;
    //表格行背景色
    QBrush row_bk_ssub;    //子目行
    QBrush row_bk_fsub;    //总目行
    QBrush row_bk_sum;     //合计行
};
Q_DECLARE_METATYPE(CurStatDialog::StateInfo)

#endif // CURSTATDIALOG_H
