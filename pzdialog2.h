#ifndef PZDIALOG2_H
#define PZDIALOG2_H

#include <QDialog>
#include <QDate>
#include <QDataWidgetMapper>
#include <QLabel>
#include <QTimer>

#include "appmodel.h"
#include "utils.h"
#include "commdatastruct.h"


namespace Ui {
    class PzDialog2;
}

//显示有一个hash表所表达的标识（实际应用于显示凭证状态，用户名等）
class MapLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(int key READ getValue WRITE setValue)

public:
    MapLabel(QWidget* parent = 0);
    void setMap(QHash<int,QString> labelHash);
    void setValue(int key);
    int getValue();

private:
    QHash<int,QString> inerHash;
    int curKey;
};

class PzDialog2 : public QDialog
{
    Q_OBJECT

public:
    explicit PzDialog2(Account* account, int year, int month, CustomRelationTableModel* model,
                       bool readOnly = false, QWidget *parent = 0);
    ~PzDialog2();

    void setDateRange(QDate start, QDate end);
    int getCurPzId();
    void setReadOnly(bool readOnly);
    int getPzState();
    void setPzState(int scode, User* user);
    bool isDirty();
    void reAssignPzNum();
    bool canMoveBaUp();    //是否可以向上移动业务活动
    bool canMoveBaDown();  //是否可以向下移动业务活动

    //void refreshInfo();

public slots:
    //处理凭证记录的导航按钮的槽
    void naviFst();
    void naviNext();
    void naviPrev();
    void naviLast();
    void naviTo(int pid, int bid = 0);
    bool naviTo2(int num);

    //增、删凭证按钮处理槽
    void addPz();
    void insertPz();
    void delPz();

    //增删业务活动槽
    void addBusiAct();
    void delBusiAct();

    //移动业务活动槽
    void moveUp();
    void moveDown();

    //处理确定、取消按钮
    void save(bool isForm = true);
    void closeDlg();    
    void requestCrtNewOppoAction();//请求创建新的合计对冲业务活动

protected:
    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent * event);

public slots:
    void show();

private slots:
    void currentMtChanged(int index);
    //void rateTextChanged();
    //void on_edtRate_editingFinished();
    void actionDataItemChanged(QTableWidgetItem *item);
    void crtNewSumOppoAction(int row);
    void autoCopyToCurAction();
    void autoCopyToCurAction2(int row,int col);
    void demandAppendNewAction(int row);
    void refreshContextMenu(int row, int col);
    bool calSums();
    void pzContentModify();
    void zbNumChanged();
    void encChanged();
    void pzDateChanged();
    void jdSumChanged();
    void autoSave();
    //void rowClicked(int logicalIndex);
    //void currentCellChanged(int currentRow, int currentColumn,
    //                        int previousRow, int previousColumn);
    void curItemSelectionChanged();
    //void cellClicked(int row, int column);

//    void pzDateChanged();
//    void pzEncNumChanged();
//    void pzZbNumChanged();

    void on_actAddNewAction_triggered();

    void on_actInsertNewAction_triggered();

    void on_actInsertOppoAction_triggered();

    void on_actDelAction_triggered();

    void on_actCopyAction_triggered();

    void on_actCutAction_triggered();

    void on_actPasteAction_triggered();

    void on_actCollaps_triggered();

    //void on_pzDate_editingFinished();

    //void on_spbEnc_editingFinished();

    void on_edtRate_returnPressed();

    void on_actInsSelOppoAction_triggered();

signals:
    void infomation(QString info);       //向主窗口发送要在状态条上显示的信息
    void pzsStateChanged();              //在调用save方法后，如果当前凭证集状态发生了更改，则触发此信号
    void canMoveUpAction(bool isCan);    //是否可以向上移动业务活动
    void canMoveDownAction(bool isCan);  //是否可以向下移动业务活动
    void curPzNumChanged(int pzNum); //当前凭证号改变了
    void recalSum(); //需重新计算合计值（在借贷金额列发生改变时）
    void pzStateChanged(int scode);  //用以通知主窗口当前凭证的状态
    void pzContentChanged(bool isChanged = true); //用以通知主窗口当前凭证的内容发生了改变，参数默认为真表示改变了
    void curIndexChanged(int idx, int nums);  //当前凭证改变了，参数idx：当前凭证在凭证集内的
                                              //序号（基于1），nums：凭证集内的凭证总数
    void selectedBaAction(bool isSel);   //用户选择了业务活动
    void saveCompleted();    //凭证自动保存完成，用于通知主窗口，使其对此做出反映    
    void mustRestat();      //凭证集的内容发生了影响统计余额结果的改变

private:
    void init();
    void initAction();
    void adjustTable();
    void adjustViewMode();
    bool canCrtOppoSumAction(int row, int col, QHash<int,Double>& sums,
                             Double& sum, int& dir, int& num);
    void reAllocActionNumber();
    void viewCurPzInfo();
    void refreshVHeaderView();

    void installDataWatch(bool install = true);
    void installDataWatch2(bool install = true);
    void appendBlankAction();
    void appentNewAction(BusiActionData2* ba);
    void appendNewAction(int pid, int num, int dir = DIR_J, Double v = Double(),int mt = RMB,
        QString summary = "", int fid = 0, int sid = 0,
        BusiActionData2::ActionState state = BusiActionData2::BLANK);
    void insertNewAction(int row,BusiActionData2* ba);
    void insertNewAction(int row, int pid, int num, int dir = 1, Double v = Double(),
         int mt = RMB, QString summary = "", int fid = 0, int sid = 0,
         BusiActionData2::ActionState  state = BusiActionData2::BLANK);
    void initNewAction(int row, int pid, int num, int dir = 1, Double v = Double(),int mt = RMB,
        QString summary = "", int fid = 0, int sid = 0,
        BusiActionData2::ActionState  state = BusiActionData2::BLANK);
    bool isEditable();

    Ui::PzDialog2 *ui;

    CustomRelationTableModel* model;  //读取凭证表
    QDataWidgetMapper* dataMapping;   //映射PingZhengs表的内容到显示部件
    ActionEditItemDelegate* delegate; //业务活动表格部件的项目代理

    QHash<int,Double> rates;  //当月汇率
    QHash<int,QString> mtNames; //币种名称表
    QHash<int,QString> userNames;  //用户名表
    QHash<int,QString> PzStates; //凭证状态表（因为和全局变量pzStates的键不同，它是枚举型）

    int maxPzNum;      //当前凭证集合已用的最大总号
    int maxPzZbNum;    //当前凭证集合已用的最大自编号
    int curPzId;       //当前凭证的ID
    PzClass curPzClass;    //当前凭证类别
    PzsState curPzSetState; //当前凭证集状态
    int cury,curm;     //凭证集所处年、月
    bool pzStateDirty; //当凭证状态在录入态和审核态之间的转变时为真
    bool pzDirty;      //凭证数据是否被修改
    bool acDirty;      //凭证业务活动数据是否被修改
    bool initStage;    //是否处于init()函数调用阶段
    bool readOnly;     //是否以只读方式打开
    QList<BusiActionData2*> busiActions; //当前凭证的业务活动数据
    QList<BusiActionData2*> delActions;  //要删除的业务活动的数据列表
    int numActions;  //当前凭证包含的业务活动数（包括自动添加的末尾空白行）
    int rowHeight;   //业务活动表行高
    int bankId,ysId,yfId;  //银行存款、应收账款和应付账款科目的id（在以简要方式显示结转汇兑损益的凭证时使用）

    QHash<int,Double>oSums;  //在创建合计对冲业务活动时，用以保存计算好的各币种分开计算的合计值
    Double oSum;             //用以保存计算好的各币种合起来的合计值
    int oDir;                //用于保存要创建的对冲业务活动的借贷方向
    int oNum;                //用于保存源对冲业务活动的条数

    QList<int> selRows;      //业务活动表中选择的行（整行选择的行）
    bool isContinue;         //选中的行是否是连续的
    bool isSameDir;          //选中的行到借贷方向是否相同
    QStringList vheadLst; //业务活动表格的垂直行标题（用于显示业务活动的序号，和编辑）

    QTimer* timer;
    Account* account;
    SubjectManager* smg;
    DbUtil* dbUtil;
};

#endif // PZDIALOG2_H
