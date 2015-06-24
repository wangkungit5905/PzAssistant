#ifndef SHOWDZDIALOG2_H
#define SHOWDZDIALOG2_H

#include "widgets.h"

#include <QDialog>

namespace Ui {
    class ShowDZDialog2;
    class SubjectRangeSelectDialog;
}

class QListWidgetItem;

class Account;
class FirstSubject;
class HierarchicalHeaderView;
class MyWithHeaderModels;
class PrintTemplateDz;
class PreviewDialog;

//表格列数
const int DV_COL_CNT_CASH = 12;
const int DV_COL_CNT_BANKRMB = 13;
const int DV_COL_CNT_COMMON = 11;

//明细账视图窗口类
class ShowDZDialog : public DialogWithPrint
{
    Q_OBJECT

public:
    //表格显示格式
    enum TableFormat{
        NONE         =0,   //
        CASHDAILY    =1,   //现金日记账格式
        BANKRMB      =2,   //银行日记账格式（人民币）
        BANKWB       =3,   //银行日记账格式（外币）
        COMMON       =4,   //通用金额式
        THREERAIL    =5    //三栏明细式（由应收/应付等使用）
    };

    enum TableRowType{
        TRT_DATA    = 1,    //数据行
        TRT_MONTH   = 2,    //月小计行
        TRT_YEAR    = 3     //年合计行
    };

    explicit ShowDZDialog(Account* account, QByteArray* cinfo, QByteArray* pinfo, QWidget *parent = 0);
    ~ShowDZDialog();
    void setCommonState(QByteArray* info);
    void setProperState(QByteArray* info);
    QByteArray* getCommonState();
    QByteArray* getProperState();
    void print(PrintActionClass pac = PAC_TOPRINTER);

private slots:
    void curFilterChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void startDateChanged(const QDate& date);
    void endDateChanged(const QDate& date);
    void onSelFstSub(int index);
    void onSelSndSub(int index);
    void onSelMt(int index);
    void moveTo();
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);
    void paging(int rowsInTable, int& pageNum);
    void renPageData(int pageNum, QList<int>*& colWidths, MyWithHeaderModels* pdModel);

    void on_actPrint_triggered();

    void on_actPreview_triggered();

    void on_actToPdf_triggered();

    void on_actToExcel_triggered();

    void on_btnSaveFilter_clicked();

    void on_btnSaveAs_clicked();

    void on_btnDelFilter_clicked();

    void on_btnSubRange_clicked();

    void on_btnRefresh_clicked();

    void on_lstSubs_itemDoubleClicked(QListWidgetItem *item);

signals:
    void openSpecPz(int pid, int bid); //打开指定id的凭证

private:
    void refreshTalbe();
    void readFilters();
    void initFilter();
    void initSubjectItems();
    void initSubjectList();
    void adjustSaveBtn();
    void setTableRowBackground(TableRowType rowType, const QList<QStandardItem*> cols);

    //生成指定格式表格数据的函数
    int genDataForCash( QList<DailyAccountData2*> datas,
                        QList<QList<QStandardItem*> >& pdatas,
                        Double prev, int preDir,
                        QHash<int,Double> preExtra,
                        QHash<int,int> preExtraDir,
                        QHash<int,Double> rates);
    int genDataForBankRMB(QList<DailyAccountData2 *> datas,
                           QList<QList<QStandardItem*> >& pdatas,
                           Double prev, int preDir,
                           QHash<int, Double> preExtra,
                           QHash<int,int> preExtraDir,
                           QHash<int, Double> rates);
    int genDataForBankWb(QList<DailyAccountData2*> datas,
                          QList<QList<QStandardItem*> >& pdatas,
                          Double prev, int preDir,
                          QHash<int,Double> preExtra,
                          QHash<int,int> preExtraDir,
                          QHash<int,Double> rates);
    int genDataForCommon(QList<DailyAccountData2*> datas,
                           QList<QList<QStandardItem*> >& pdatas,
                           Double prev, int preDir,
                           QHash<int,Double> preExtra,
                           QHash<int,int> preExtraDir,
                           QHash<int,Double> rates);
    int genDataForThreeRail(QList<DailyAccountData2*> datas,
                          QList<QList<QStandardItem*> >& pdatas,
                          Double prev, int preDir,
                          QHash<int,Double> preExtra,
                          QHash<int,int> preExtraDir,
                          QHash<int,Double> rates);

    //生成指定格式表头的函数
    void genThForCash(QStandardItemModel* model = NULL);
    void genThForBankRmb(QStandardItemModel* model = NULL);
    void genThForBankWb(QStandardItemModel* model = NULL);
    void genThForCommon(QStandardItemModel* model = NULL);
    void genThForThreeRail(QStandardItemModel* model = NULL);

    void printCommon(QPrinter* printer);
    TableFormat decideTableFormat(int fid,int sid,int mt);

private:
    Ui::ShowDZDialog2 *ui;
    QList<DVFilterRecord*> filters;  //历史过滤条件项目列表
    DVFilterRecord* curFilter;      //当前选中的过滤条件项
    SubjectComplete *fcom;
    int witch;                  //科目选择模式（1：所有科目，2：指定类型科目，3：指定范围科目）
    QList<int> subIds;            //当前要显示明细帐的科目id列表
    QHash<int,QList<QString> > sNames; //二级科目名列表
    double gv,lv;  //业务活动涉及的金额上下限
    bool inc;      //是否包含未入账凭证

    QHash<TableFormat,QList<int> > colWidths; //各种表格格式列宽度值
    QHash<TableFormat,QList<int> > colPrtWidths; //打印模板中的表格列宽度值
    QPrinter::Orientation pageOrientation;  //打印模板的页面方向
    PageMargin margins;  //页面边距
    QList<int> splitterSizes; //分裂器布局内两个视图部件的尺寸（左为表格、右为过滤条件窗口）

    //用户当前选择的科目币种状态
    FirstSubject* curFSub;
    SecondSubject* curSSub;
    Money* curMt;
    Money* mmtObj;  //账户所采用的本货币对象
    //与当前表格数据相对应的科目与币种的选择组合状态（通过与上面的状态相比较来决定更新表格数据）
    int tfid; int tsid; int tmt;

    //时间信息
    int cury;  //帐套年份

    //这两个可以合并
    QList<int> mts; //这是要在表格的借、贷和余额栏显示的外币代码列表

    TableFormat tf,otf;                     //当前表格格式

    //视图显示有关的数据成员
    HierarchicalHeaderView* hv;         //表头
    QStandardItemModel* headerModel;    //表头数据模型
    MyWithHeaderModels* dataModel;      //表格内容数据模型（可附带表头数据模型）

    //分页处理有关成员
    PrintTemplateDz* pt;                //打印模板类
    QList<QList<QStandardItem*> > pdatas; //打印页面行数据（分页的边界由pageIndexes的元素值指定）
    QList<int> pageIndexes;             //每页的表格最后一行在pdatas列表中的索引值
    QList<TableFormat> pageTfs;         //每页的表格格式（在涉及打印多个科目，且不同科目可能需要不同的表格形式，
                                        //比如同时打印银行存款下的人民币和美金账户，或同时打印现金和应收科目等情形）
    int maxRows;                        //每页的表格内最多可拥有的行数
    QList<int> pfids,psids,pmts;        //保存每页所属的一二级科目和币种
    QHash<int,QString> subPageNum;      //页号（第几页/总数）
    int pages;
    PrintTask curPrintTask;             //当前打印任务类别

    PreviewDialog* preview;

    QAction* actMoveTo;  //转到该凭证的QAction
    Account* account;
    AccountSuiteRecord* curSuite; //当前帐套
    SubjectManager* smg;
    QHash<int,Money*> allMts;

    //表格行背景色
    QBrush row_bk_data;     //数据行
    QBrush row_bk_month;    //月合计行
    QBrush row_bk_year;     //年合计行

    QString strMonthSum,strYearSum,strSum;
};


class SubjectRangeSelectDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SubjectRangeSelectDialog(SubjectManager* smg, const QList<int>& subIds, int fstSubId, QWidget *parent = 0);
    ~SubjectRangeSelectDialog();
    QList<int> getSelectedSubIds();
    bool isSelectedFst();
    FirstSubject* getSelectedFstSub();
    SecondSubject* getSelectedSndSub();

private slots:
    void onSubjectSelectModeChanged(bool checked);
    void curFstSubChanged(int currentRow);
    void itemCheckStateChanged(QListWidgetItem* item);

    void on_btnSelAllSnd_clicked();

    void on_btnSelAllSndNot_clicked();

    void on_btnSelAll_clicked();

    void on_btnSelAllNot_clicked();

private:
    void loadSubjects();
    void isAllSelected(bool isFst = true);
    
private:
    Ui::SubjectRangeSelectDialog *ui;
    SubjectManager* smg;
    FirstSubject* curFsub;
    QList<int> subIds;    
};

#endif // SHOWDZDIALOG2_H
