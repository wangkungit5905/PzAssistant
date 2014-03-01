#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QSignalMapper>
#include <QSqlTableModel>
#include <QSpinBox>
#include <QDockWidget>
#include <QStatusBar>


#include "dialogs.h"
#include "dialog3.h"
#include "widgets.h"
#include "subjectsearchform.h"
#include "suiteswitchpanel.h"

class QUndoStack;
class QUndoView;
class QProgressBar;

namespace Ui {
    class MainWindow;
}

//class MouseHoverEventFilter : public QObject
// {
//     Q_OBJECT
//public:
//    MouseHoverEventFilter(QObject* parent=0):QObject(parent){}
// protected:
//     bool eventFilter(QObject *obj, QEvent *event);
// };


class PaStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    PaStatusBar(QWidget* parent=0);
    ~PaStatusBar();
    void setPzSetDate(int y, int m);
    void setPzSetState(PzsState state);
    void setExtraState(bool isValid){extraState.setText(isValid?tr("有效"):tr("无效"));}
    void setPzAmount(int c){pzCount.setText(c!=0?QString::number(c):"");}
    void setRepealCount(int c){num_rep=c;adjustPzCountTip();}
    void setRecordingCount(int c){num_rec=c;adjustPzCountTip();}
    void setVerifyCount(int c){num_ver=c;adjustPzCountTip();}
    void setInstantCount(int c){num_ins=c;adjustPzCountTip();}
    void setPzCounts(int repeal,int recording,int verify,int instat,int amount);
    void resetPzCounts();
    void setUser(User* user);
    void showRuntimeMessage(QString info, AppErrorLevel level);

public slots:
    void notificationProgress(int amount, int curp);
private slots:
    void timeout();
private:
    void adjustPzCountTip();
    void startProgress(int amount);
    void endProgress();
    void adjustProgress(int value);

    QLabel pzSetDate,pzSetState,extraState,lblUser,pzCount/*,pzRepeal,pzRecording,pzVerify,pzInstat*/;
    QProgressBar* pBar;
    QLabel* lblRuntime;
    int pAmount;  //进度指示器的最大值
    int num_rec,num_ver,num_ins,num_rep;    //录入、审核、入账、作废凭证数
    QHash<AppErrorLevel,QString> colors;     //运行时信息各级别所使用的颜色在样式表中的表示串
    QTimer* timer;
};



/**
 * @brief 子窗口组管理类
 */
class SubWinGroupMgr : public QObject
{
    Q_OBJECT
public:
    SubWinGroupMgr(int gid,QMdiArea* parent):groupId(gid),parent(parent){}
    ~SubWinGroupMgr(){}
    void show();
    void hide();
    bool isShow(){return isShowed;}
    MyMdiSubWindow* showSubWindow(subWindowType winType, QWidget* widget, SubWindowDim* winfo);
    bool isSpecSubOpened(subWindowType winType){return subWinHashs.contains(winType);}
    QWidget* getSubWinWidget(subWindowType winType);
    void closeSubWindow(subWindowType winType);
    void closeAll();

private slots:
    void subWindowClosed(MyMdiSubWindow *subWin);

//signals:
//    void saveSubWinState(subWindowType winType,QByteArray* state,SubWindowDim* dim);

private:
    int groupId;
    QMdiArea* parent;
    QHash<subWindowType,MyMdiSubWindow*> subWinHashs; //唯一性子窗口映射表
    //QList<QMdiSubWindow*> subWindows;                //所有子窗口列表
    bool isShowed;                                   //当前子窗口组是否处于显示状态
    //DbUtil* dbUtil;
};


extern int mdiAreaWidth;       //主窗口Mdi区域宽度
extern int mdiAreaHeight;      //主窗口Mdi区域高度

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:


    //工具视图类别枚举
    enum ToolViewType{
        TV_UNDO         = 1,    //UndoView工具视图
        TV_SEARCHCLIENT = 2,    //搜索客户
        TV_SUITESWITCH  = 3     //帐套切换视图
    };

    //undo框架类别
    enum UndoType{
        UT_PZ  = 1,      //对凭证集的编辑
        UT_ACCOUNT = 2,  //对账户信息的编辑
        UT_SUBJECT = 3   //对科目系统的编辑
    };

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void getMdiAreaSize(int &mdiAreaWidth, int &mdiAreaHeight);
    void hideDockWindows();
    bool AccountVersionMaintain(QString fname);
protected:
    void closeEvent(QCloseEvent *event);

private slots:
    //文件菜单
    void newAccount();
    void openAccount();
    void closeAccount();
    void exit();

    //数据菜单
    void viewSubjectExtra();      //显示科目余额
    void generateBalanceSheet();  //生成资产负债表
    void generateIncomeStatements(); //生成利润表

    //工具菜单
    void showSqlTool();

    //选项菜单

    //窗口菜单
    void updateWindowMenu();
    void setActiveSubWindow(QWidget *window);

    //帮助菜单
    void about();

    //处理创建新账户的向导
    void toCrtAccNextStep(int curStep, int nextStep);

    void openSpecPz(int pid,int bid);
    void showTemInfo(QString info);
    void showRuntimeInfo(QString info, AppErrorLevel level);
    void refreshShowPzsState();
    //void extraChanged(){isExtraVolid = false;} //由于凭证集发生了影响统计余额的改变，导致当前余额失效，
                                               //目前主要由凭证编辑窗口的编辑动作引起，反馈给主窗口
    void extraValid();
    void pzSetStateChanged(PzsState newState);
    void pzSetExtraStateChanged(bool valid);

    //Undo框架相关槽
    void undoViewItemClicked(const QModelIndex &indexes);
    //void canRedoChanged(bool canRedo);
    //void canUndoChanged(bool canUndo);
    void undoCleanChanged(bool clean);
    void UndoIndexChanged(int idx);
    void redoTextChanged(const QString& redoText);
    void undoTextChanged(const QString& undoText);

    void showAndHideToolView(int vtype);
    void DockWindowVisibilityChanged(bool visible);
    void pzCountChanged(int count);
    void rfNaviBtn();
    void curPzChanged(PingZheng* newPz=NULL, PingZheng* oldPz=NULL);
    void baIndexBoundaryChanged(bool first, bool last);
    void baSelectChanged(QList<int> rows, bool conti);
    //void PzChangedInSet();

    /////////////////////////////////////////////////////////////////
    void suiteViewSwitched(AccountSuiteManager* previous, AccountSuiteManager* current);
    void viewOrEditPzSet(AccountSuiteManager* accSmg, int month);
    void pzSetOpen(AccountSuiteManager* accSmg, int month);
    void prepareClosePzSet(AccountSuiteManager* accSmg, int month);
    void pzSetClosed(AccountSuiteManager* accSmg, int month);
    void commonSubWindowClosed(MyMdiSubWindow *subWin);

    void on_actAddPz_triggered();

    void on_actInsertPz_triggered();

    void on_actDelPz_triggered();

    void on_actAddAction_triggered();

    void on_actInsertBa_triggered();

    void on_actDelAction_triggered();

    void on_actGoFirst_triggered();

    void on_actGoPrev_triggered();

    void on_actGoNext_triggered();

    void on_actGoLast_triggered();

    void on_actSave_triggered();

    void on_actFordPl_triggered();

    void on_actFordEx_triggered();

    void on_actPrint_triggered();

    void on_actPrintAdd_triggered();

    void on_actPrintDel_triggered();

    void on_actLogin_triggered();

    void on_actLogout_triggered();

    void on_actShiftUser_triggered();

    void on_actSecCon_triggered();    

    void on_actMvUpAction_triggered();

    void on_actMvDownAction_triggered();

    void on_actSearch_triggered();

    void on_actNaviToPz_triggered();

    void on_actAllVerify_triggered();

    void on_actAllInstat_triggered();

    void on_actReassignPzNum_triggered();

    void on_actImpOtherPz_triggered();

    void on_actJzbnlr_triggered();

    void on_actEndAcc_triggered();

    void on_actAntiJz_triggered();

    void on_actAntiEndAcc_triggered();

    void on_actAntiImp_triggered();

    void on_actGdzcAdmin_triggered();

    void on_actDtfyAdmin_triggered();

    void on_actSetPzCls_triggered();

    void on_actShowTotal_triggered();

    void on_actAccProperty_triggered();

    void on_actPzErrorInspect_triggered();

    void on_actForceDelPz_triggered();


    bool impTestDatas();
    void on_actViewLog_triggered();

    void on_actCurStatNew_triggered();

    void on_actDetailView_triggered();

    void on_actInStatPz_triggered();

    void on_actVerifyPz_triggered();

    void on_actAntiVerify_triggered();

    void on_actRefreshActInfo_triggered();

    void on_actSuite_triggered();

    void on_actEmpAccount_triggered();

    void on_actInAccount_triggered();

private:
    bool isOnlyCommonSubWin(subWindowType winType);
    void showCommonSubWin(subWindowType winType, QWidget* widget, SubWindowDim* dim = NULL);
    void allPzToRecording(int year, int month);
    void initActions();
    void initToolBar();
    void initSearchClientToolView();
    void initTvActions();
    void accountInit(AccountCacheItem *ci);
    subWindowType activedMdiChild();
    void rfOther();
    void rfLogin(bool login = true);
    void rfMainAct(bool open = true);
    void rfPzSetAct(bool open = true);


    void rfPzAct(bool enable);
    void rfAdvancedAct();
    void rfEditInPzAct(PingZheng *pz);

    /////////////////////////////
    void rfAct();



    /////////////////////////////////////////

    void rightWarnning(int right);
    void pzsWarning();
    void sqlWarning();    
    void showPzNumsAffected(int num);




    void initUndoView();
    void clearUndo();
    void adjustViewMenus(ToolViewType t, bool isRestore = false);
    void adjustEditMenus(UndoType ut=UT_PZ, bool restore = false);

    /////////////////////////////////////////////////////////////////
    //void switchSubWindowGroup(int suiteId);

    /////////////////////////////////////////////////////////////////



    Ui::MainWindow *ui;

    //ToolBars
    QToolBar *fileToolBar;

    //创建新账户的4个步骤的对话框
    //CreateAccountDialog* dlgAcc;
    //SetupBankDialog* dlgBank;

    QSignalMapper *windowMapper; //用于处理从窗口菜单中选择显示的窗口
    QSignalMapper* tvMapper;     //用于处理从视图菜单中选择显示的工具视图

    QSet<int> PrintPznSet; //欲打印的凭证号集合
    bool sortBy; //凭证集的排序方式（true：按凭证号，false：按自编号）

    QHash<ToolViewType,QDockWidget*> dockWindows;     //工具视图窗口集
    QHash<ToolViewType,QAction*> tvActions;           //与工具视图类型对应的QAction对象表

    //工具条上的部件
    CustomSpinBox* spnNaviTo;

    AccountSuiteManager* curSuiteMgr;
    DbUtil* dbUtil;

    QUndoStack* undoStack;     //Undo命令栈
    QUndoView* undoView;       //Undo视图
    QAction *undoAction, *redoAction; //执行undo，redo操作

    //QHash<QString,SuiteSwitchPanel*> ssPanels;              //
    //bool isAccountChanged;                              //账户是否已变更的标记
    SuiteSwitchPanel* curSSPanel;                       //当前帐套切换面板对象
    QHash<subWindowType,MyMdiSubWindow*> commonGroups; //公共类（唯一性子窗口）
    QMultiHash<subWindowType,MyMdiSubWindow*> commonGroups_multi; //公共类（多子窗口共存）
    QHash<int,SubWinGroupMgr*> subWinGroups;       //帐套视图子窗口组表（键为帐套id）
    QHash<int,QList<PingZheng*> > historyPzSet;    //每个帐套视图当前正浏览的历史凭证列表
    QHash<int,int> historyPzSetIndex;              //每个帐套视图当前正浏览的历史凭证集的当前索引
    QHash<int,int> historyPzMonth;                 //每个账套视图当前装载的历史凭证的月份数
 };
#endif // MAINWINDOW_H
