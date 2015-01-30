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
const int BA_TABLE_MAXROWS = 50;    //会计分录表格预留的最大行数

#define PZEW_DEF_WIDTH  1000 //默认宽度
#define PZEW_DEF_HEIGHT 600  //默认高度

namespace Ui {
    class pzDialog;
    class HistoryPzForm;
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
    void clearSum();
    void setLongName(QString name);
    void switchRow(int r1,int r2);
    bool isHasSelectedRows();
    void selectedRows(QList<int>& selRows, bool& isContinuous);
    void updateSubTableGeometry();
    void setRowChangedTags(QList<int> rowIndex,QList<CommonItemEditState> states);
    void setRowChangedTag(int row, CommonItemEditState state);
public slots:
    void setBalance(bool isBalance);
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
    QString getModifyTag(CommonItemEditState state);

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



class PzDialog : public QDialog
{
    Q_OBJECT

public:
    struct StateInfo{
        bool isValid;        //是否有效
        qint8 rowHeight;       //表格行高
        qint16 colSummaryWidth; //摘要列列宽
        qint16 colFstSubWidth;  //一级科目列列宽
        qint16 colSndSubWidth;  //二级科目列列宽
        qint16 colMtWidth;      //币种列列宽
        qint16 colValueWidth;   //金额列列宽
    };

    explicit PzDialog(int month, AccountSuiteManager* psm, QByteArray* sinfo, QWidget *parent = NULL);
    ~PzDialog();
    void setCommonState(QByteArray* info);
    QByteArray* getCommonState();
    void setMonth(int month);

    void adjustViewReadonly();
    bool isDirty();

    //状态获取和恢复方法
    void restoreStateInfo();
    const StateInfo &getStateInfo(){return states;}

    //凭证导航方法
    void updateContent();
    //这些方法应该可以去除了
    void moveToFirst();
    void moveToPrev();
    void moveToNext();
    void moveToLast();
    void seek(int num);
    void seek(PingZheng* pz, BusiAction *ba = NULL);

    //凭证增删方法
    void addPz();
    void insertPz();
    void removePz();
    bool crtJzhdPz();
    bool crtJzsyPz();
    bool crtJzbnlr();

    //会计分录操作方法
    void moveUpBa();
    void moveDownBa();
    void addBa();
    void insertBa(BusiAction *ba=NULL);
    void removeBa();
    void getBaSelectedCase(QList<int> rows, bool& conti);

    //由ModifySndSubEnableProperty对象回调以记录这些子目被修改了需要保存
    void addChangedSSub(SecondSubject* ssub){needSaveSSubs.insert(ssub);}
public slots:
    void save();
    void setPzState(PzState state);
private slots:
    void updatePzCount(int count);
    void curPzChanged(PingZheng* newPz, PingZheng* oldPz);
    void msgTimeout();
    void moneyTypeChanged(int index);
    void processShortcut();

    void moveToNextBa(int row);
    void selectedRowChanged();

    //凭证内容编辑监视槽
    void currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void pzDateChanged(const QDate& date);
    void pzZbNumChanged(int num);
    void pzEncNumChanged(int num);
    void BaDataChanged(QTableWidgetItem *item);

    void creatNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssub);
    void creatNewSndSubject(int row, int col, FirstSubject* fsub, SecondSubject*& ssub, QString name);

    void copyPrewAction(int row);

    void tabColWidthResized(int index, int oldSize, int newSize);
    void tabRowHeightResized(int index, int oldSize, int newSize);

    void extraException(BusiAction* ba,Double fv, MoneyDirection fd, Double sv, MoneyDirection sd);
    void modifyRate();
    void pzCommentChanged();
    void pzMemInfoModified(bool changed);
    void pzBalanceStateChanged(bool isBalance);
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

signals:
    void showMessage(QString info, AppErrorLevel levelS=AE_OK);
    void selectedBaChanged(QList<int> rows, bool conti);
    void rateChanged(int month);
private:
    void adjustTableSize();
    void initResources();
    void initBlankBa(int row);
    void refreshPzContent();
    void refreshActions();
    void refreshSingleBa(int row, BusiAction *ba);
    void updateBas(int row, int rows, BaUpdateColumns col);
    void showInfomation(QString info,AppErrorLevel level = AE_OK);
    bool isBlankLastRow();
    void installInfoWatch(bool install = true);



    Ui::pzDialog *ui;
    Account* account;            //账户对象
    SubjectManager* subMgr;      //科目管理对象
    AccountSuiteManager* pzMgr;             //凭证集
    PingZheng* curPz;            //当前凭证
    BusiAction* curBa;           //当前会计分录
    QSet<SecondSubject*> needSaveSSubs;   //需要保存的二级科目对象（这些子目对象是在做帐过程中改变的，
                                          //比如改变了子目的启用性，或新建了子目等），目前仅用于放置
                                          //被修改的子目对象。

    int curRow;                  //当前选定的会计分录行号
    bool isInteracting;          //是否在与用户的交互中（当需要用户确认创建新名称映射或二级科目时）
    bool isPzMemModified;        //凭证备注内容是否被修改的标记
    QHash<int,Double> rates;     //当前凭证集对应月份的汇率

    QList<bool> rowSelStates;  //行选择状态列表

    QHash<PzClass,QPixmap> icons_pzcls;      //各种凭证类别对应的图标
    QHash<PzState,QPixmap> icons_pzstate;    //各种凭证状态对应的图标

    //状态信息
    QList<int> baColWidths;   //显示会计分录的表格列宽（6个元素）
    int        baRowHeight;   //显示会计分录的表格行高

    QTimer* msgTimer;   //显示状态信息的定时器
    StateInfo states;   //状态信息

    ActionEditItemDelegate* delegate;

    //快捷键
    QShortcut* sc_copyprev;  //复制上一条会计分录
    QShortcut* sc_save;      //保存凭证集
    QShortcut* sc_copy;      //拷贝会计分录
    QShortcut* sc_cut;       //剪切会计分录
    QShortcut* sc_paster;    //粘贴会计分录

    //上下文菜单
    QAction* actModifyRate;
};

class HistoryPzForm : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryPzForm(PingZheng* pz, QByteArray* sinfo, QWidget *parent = 0);
    ~HistoryPzForm();
    void setPz(PingZheng* pz);
    void setCurBa(int bid);
    void setCommonState(QByteArray* info);
    QByteArray* getCommonState();
private slots:
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);
private:
    void viewPzContent();
    void viewBusiactions();
    void refreshSingleBa(int row, BusiAction *ba);
    void adjustTableSize();
    void viewRates(int y, int m);

    Ui::HistoryPzForm *ui;
    PingZheng* pz;
    int curY,curM;  //当前显示的凭证所处年月
    QList<int> colWidths;  //表格列宽
    QHash<int,QHash<Money*,Double> > rates;//汇率表，键为年月组成的混合键高四位表示年份
};

class ReadOnlyItemDelegate : public QItemDelegate{
    Q_OBJECT
public:
    ReadOnlyItemDelegate(QObject* parent = 0):QItemDelegate(parent){}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const{return 0;}
};

#endif // PZDIALOG_H
