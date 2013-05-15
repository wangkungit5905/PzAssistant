#ifndef PZDIALOG_H
#define PZDIALOG_H

#include <QTableWidget>
#include <QTimer>

#include "commdatastruct.h"
#include "pz.h"
#include "account.h"
#include "widgets/bawidgets.h"
#include "delegates.h"

#define INFO_TIMEOUT  5000   //状态信息显示的超时时间
#define MAXROWS       50    //会计分录表格预留的最大行数

//#define PZEW_SD_COUNT 9      //状态信息个数
#define PZEW_DEF_WIDTH  1000 //默认宽度
#define PZEW_DEF_HEIGHT 600  //默认高度
#define PZEW_DEFROWHEIGHT    30   //默认表格行高
#define PZEW_DEFCW_SUMMARY   400  //摘要列列宽
#define PZEW_DEFCW_FS        80   //一级科目列列宽
#define PZEW_DEFCW_SS        100  //而积极科目列列宽
#define PZEW_DEFCW_MT        80   //币种列列宽
#define PZEW_DEFCW_V         150  //金额列列宽

namespace Ui {
class pzDialog;
}

//编辑和显示凭证的会计分录的类
class BaTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    //会计分录表格列索引
    enum BaTabColIndex{
        SUMMARY   = 0,   //摘要
        FSTSUB    = 1,   //一级科目
        SNDSUB    = 2,   //二级科目
        MONEYTYPE = 3,   //币种
        JVALUE    = 4,   //借方
        DVALUE    = 5    //贷方
    };

    BaTableWidget(QWidget* parent = 0);
    void setValidRows(int rows);
    int getValidRows(){return validRows;}
    void setJSum(Double v);
    void setDSum(Double v);
    void setBalance(bool isBalance = true);
    void setLongName(QString name);
    void switchRow(int r1,int r2);
    bool isHasSelectedRows();
    void selectedRows(QList<int>& selRows, bool& isContinuous);


    void updateSubTableGeometry();
protected:
    virtual void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent* e);
    void mousePressEvent(QMouseEvent* event);

private slots:
    //void processShortcut();

signals:
    void requestCrtNewOppoBa();            //请求创建新的合计对冲会计分录
    void reqAutoCopyToCurBa();             //请求复制上一条会计分录并插入到当前行
    void requestAppendNewBa(int row);      //请求添加新的空会计分录
    void requestContextMenu(int row, int col); //请求对上下文菜单进行刷新

private:
    QTableWidget* sumTable;                 //显示会计分录合计栏的表格
    QTableWidgetItem *lnItem;               //显示二级科目全名
    BAMoneyValueItem_new *jSumItem, *dSumItem;  //显示借贷合计值
    int validRows;                          //有效行数（包括显示会计分录的行和备用行）
    //bool readonly;                          //只读属性
    //QShortcut* shortCut;                    //复制上一条会计分录的快捷键
};


enum BaUpdateColumn{
    BUC_SUMMARY     = 0x01,
    BUC_FSTSUB      = 0x02,
    BUC_SNDSUB      = 0x04,
    BUC_MTYPE       = 0x08,
    BUC_VALUE       = 0x10,
    BUC_ALL         = 0x12
};
Q_DECLARE_FLAGS(BaUpdateColumns, BaUpdateColumn)
Q_DECLARE_OPERATORS_FOR_FLAGS(BaUpdateColumns)



class PzDialog : public QWidget
{
    Q_OBJECT

public:
    struct StateInfo{
        bool isValid;        //是否有效
        int rowHeight;       //表格行高
        int colSummaryWidth; //摘要列列宽
        int colFstSubWidth;  //一级科目列列宽
        int colSndSubWidth;  //二级科目列列宽
        int colMtWidth;      //币种列列宽
        int colValueWidth;   //金额列列宽
    };

    explicit PzDialog(int y, int m, PzSetMgr* psm, const StateInfo states, QWidget *parent = NULL);
    ~PzDialog();

    void setReadonly();

    //状态获取和恢复方法
    void restoreState();
    const StateInfo &getState(){return states;}

    //凭证导航方法
    void updateContent();
    void moveToFirst();
    void moveToPrev();
    void moveToNext();
    void moveToLast();
    void seek(int num);

    //凭证增删方法
    void addPz();
    void insertPz();
    void removePz();

    //会计分录操作方法
    void moveUpBa();
    void moveDownBa();
    void addBa();
    void insertBa();
    void removeBa();

    //bool hasSelectedRow();
    //void selectedRows(QList<int>& rows);

private slots:
    void msgTimeout();
    void moneyTypeChanged(int index);
    //void updateSndSubject(int row, int col, SecondSubject* ssub);
    void processShortcut();
    void save();
    void moveToNextBa(int row);

    //凭证内容编辑监视槽
    void currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void pzDateChanged(const QDate& date);
    void pzZbNumChanged(int num);
    void pzEncNumChanged(int num);
    void BaDataChanged(QTableWidgetItem *item);

    void creatNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssub);
    void creatNewSndSubject(int row, int col, FirstSubject* fsub, SecondSubject*& ssub, QString name);

//    void actionDataItemChanged(QTableWidgetItem *item);
//    //void currentCellChanged(int currentRow, int currentColumn,
//    //                        int previousRow, int previousColumn);
//    void currentChanged(const QModelIndex& current, const QModelIndex& previous);
//    void customContextMenuRequested(const QPoint &pos);

//    //监视凭证账面信息部分和状态改变的槽
//    void dateChanged(const QDate& date);
//    void zbNumChanged(int i);
//    void encNumChanged(int i);
//    void pzStateChanged(bool checked);


//    //凭证导航和编辑方法
//    void on_btnNext_clicked();
//    void on_btnPrev_clicked();
//    void on_btnFisrt_clicked();
//    void on_btnLast_clicked();
//    void on_btnAddPz_clicked();
//    void on_btnInsertPz_clicked();
//    void on_btnDelPz_clicked();
//    void on_btnSave_clicked();

//    //会计分录处理
//    void on_btnAddBa_clicked();
//    void on_btnInsertBa_clicked();
//    void on_btnDelBa_clicked();
//    void on_btnMoveUp_clicked();
//    void on_btnMoveDown_clicked();
//    //void rowSelected(int logicalIndex);
//    void selectionChanged(const QItemSelection & selected,
//                          const QItemSelection & deselected);

//    //上下文菜单处理
//    void copyAction();
//    void cutAction();
//    void pasterAction();
//    void deleteAction();
//    void addNewAction();
//    void addCopyPrewAction();
//    void insertAction();
//    void insertOppoAction();
//    void collapsActions();
//    void mergeActions();

//    void demandAppendNewAction(int row);

    void tabColWidthResized(int index, int oldSize, int newSize);
    void tabRowHeightResized(int index, int oldSize, int newSize);

//signals:

private:
    void adjustTableSize();
    void initResources();
    void initBlankBa(int row);
//    void connectTableSignals();
//    void connectContextMenu();
    void refreshPzContent();
    void refreshActions();
    void refreshSingleBa(int row, BusiAction *ba);
    void updateBas(int row, int rows, BaUpdateColumns col);
    void showInfomation(QString info,AppErrorLevel level = AE_OK);
    bool isBlankLastRow();
    //
    void copyPreviouBa();
    void copyCutPaste(ClipboardOperate witch);

//    void enableWidget();
//    void installDataWatch(bool install = true);
    void installInfoWatch(bool install = true);
//    void adjustTableGeometry();
//    void initBankAction(int row);
//    void reCalSums();
//    void refreshVHeaderView();
//    bool isContinuous(int& top,int& tail);

//    void refreshContextMenu(int row, int col);



    Ui::pzDialog *ui;
    Account* account;            //账户对象
    SubjectManager* subMgr;      //科目管理对象
    PzSetMgr* pmg;             //凭证集
    PingZheng* curPz;            //当前凭证
    BusiAction* curBa;           //当前会计分录
    int curRow;                  //当前选定的会计分录行号
    bool isInteracting;          //是否在与用户的交互中（当需要用户确认创建新名称映射或二级科目时）
//    int curIdx;                  //当前凭证索引号
//    int validBas;                //当前凭证的实际有效会计分录数
    QHash<int,Double> rates;     //当前凭证集对应月份的汇率

//    //int selRows;                 //
    QList<bool> rowSelStates;  //行选择状态列表

    QHash<AppErrorLevel,QString> colors;     //信息级别所使用的颜色在样式表中的表示串
    QHash<PzClass,QPixmap> icons_pzcls;      //各种凭证类别对应的图标
    QHash<PzState,QPixmap> icons_pzstate;    //各种凭证状态对应的图标

    //状态信息
    QList<int> baColWidths;   //显示会计分录的表格列宽（6个元素）
    int        baRowHeight;   //显示会计分录的表格行高
//    QStringList vheadLst; //会计分录表格的垂直行标题（用于显示业务活动的序号，和编辑状态）

    QTimer* msgTimer;   //显示状态信息的定时器
    StateInfo states;   //状态信息

    ActionEditItemDelegate* delegate;
//    QMenu* contextMenu;     //会计分录表格内的上下文菜单

    //快捷键
    QShortcut* sc_copyprev;  //复制上一条会计分录
    QShortcut* sc_save;      //保存凭证集
    QShortcut* sc_copy;      //拷贝会计分录
    QShortcut* sc_cut;       //剪切会计分录
    QShortcut* sc_paster;    //粘贴会计分录
};



#endif // PZDIALOG_H
