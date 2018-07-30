#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QSignalMapper>
#include <QSqlTableModel>
#include <QSpinBox>
#include <QDockWidget>
#include <QStatusBar>

#include <QSystemTrayIcon>

#include "dialogs.h"
#include "dialog3.h"
#include "widgets.h"
#include "subjectsearchform.h"
#include "suiteswitchpanel.h"

class QUndoStack;
class QUndoView;
class QProgressBar;
class QShortcut;

class WorkStation;
class PzSearchDialog;
class LockApp;

namespace Ui {
    class MainWindow;
}

class PaStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    PaStatusBar(QWidget* parent=0);
    ~PaStatusBar();
    void setCurSuite(QString suiteName);
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
    void setWorkStation(WorkStation* mac);
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

    QLabel curSuite,pzSetDate,pzSetState,extraState,lblUser,pzCount,lblWs/*,pzRecording,pzVerify,pzInstat*/;
    QProgressBar* pBar;
    QLabel* lblRuntime;
    int pAmount;  //进度指示器的最大值
    int num_rec,num_ver,num_ins,num_rep;    //录入、审核、入账、作废凭证数
    QHash<AppErrorLevel,QString> colors;     //运行时信息各级别所使用的颜色在样式表中的表示串
    QTimer* timer;
    int _timeout; //消息显示的超时时间
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
    int count(){return subWinHashs.count();}
    QList<MyMdiSubWindow*> getAllSubWindows(){return subWinHashs.values();}

private slots:
    void subWindowClosed(MyMdiSubWindow *subWin);

signals:
    void specSubWinClosed(subWindowType winType);

private:
    int groupId;
    QMdiArea* parent;
    QHash<subWindowType,MyMdiSubWindow*> subWinHashs; //唯一性子窗口映射表
    bool isShowed;                                   //当前子窗口组是否处于显示状态
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
    bool AccountVersionMaintain(QString fname);
protected:
    void closeEvent(QCloseEvent *event);
    void mouseMoveEvent(QMouseEvent * event);

public slots:
    void showMainWindow();
    void exit();

private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void catchMouse();
    //文件菜单
    void newAccount();
    void openAccount();
    void closeAccount();


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

    void openSpecPz(int pid,int bid);
    void showTemInfo(QString info);
    void showRuntimeInfo(QString info, AppErrorLevel level);
    void refreshShowPzsState();
    void extraValid();
    void pzSetStateChanged(PzsState newState);
    void pzSetExtraStateChanged(bool valid);

    //Undo框架相关槽
    void undoViewItemClicked(const QModelIndex &indexes);
    void undoCleanChanged(bool clean);
    void UndoIndexChanged(int idx);
    void redoTextChanged(const QString& redoText);
    void undoTextChanged(const QString& undoText);

    void showAndHideToolView(int vtype);
    void DockWindowVisibilityChanged(bool visible);
    void pzCountChanged(int count);
    void curPzChanged(PingZheng* newPz=NULL, PingZheng* oldPz=NULL);
    void baIndexBoundaryChanged(bool first, bool last);
    void baSelectChanged(QList<int> rows, bool conti);
    void rateChanged(int month);

    void startExternalTool(int index);

    /////////////////////////////////////////////////////////////////
    void suiteViewSwitched(AccountSuiteManager* previous, AccountSuiteManager* current);
    void viewOrEditPzSet(AccountSuiteManager* accSmg, int month);
    void pzSetOpen(AccountSuiteManager* accSmg, int month);
    void prepareClosePzSet(AccountSuiteManager* accSmg, int month);
    void pzSetClosed(AccountSuiteManager* accSmg, int month);
    void commonSubWindowClosed(MyMdiSubWindow *subWin);
    void subWindowActivated(QMdiSubWindow * window);
    void specSubWinClosed(subWindowType winType);
    void printProcess();
    void localStationChanged(WorkStation* ws);
    void openBusiTemplate();
    void lockWindow();
    void unlockWindow();

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

    void on_actShowTotal_triggered();

    void on_actAccProperty_triggered();

    void on_actPzErrorInspect_triggered();

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

    void on_actCloseCurWindow_triggered();

    void on_actCloseAllWindow_triggered();

    void on_actDelAcc_triggered();

    void on_actImpPzSet_triggered();

    void on_actUpdateSql_triggered();

    void on_actExtComSndSub_triggered();

    void on_actOption_triggered();

    void on_actManageExternalTool_triggered();

    void on_actTaxCompare_triggered();

    void on_actNoteMgr_triggered();

    void on_actExpAppCfg_triggered();

    void on_actImpAppCfg_triggered();

    void on_actCrtAccoutFromOld_triggered();

    //void on_actShowSusSybJoinCfg_triggered();

    void on_actCloseSuite_triggered();

    void on_actBatchExport_triggered();

    void on_actBatchImport_triggered();

    void on_actInitInvoice_triggered();

    void on_actYsYfStat_triggered();

    void on_actICManage_triggered();

    void on_actJxTaxMgr_triggered();

    void on_actQuarterStat_triggered();

    void on_actYearStat_triggered();

    void on_actCrtJtpz_triggered();

private:
    void _closeAccount();
    bool isExecAccountTransform();
    bool isOnlyCommonSubWin(subWindowType winType);
    void showCommonSubWin(subWindowType winType, QWidget* widget, SubWindowDim* dim = NULL);
    void allPzToRecording(int year, int month);
    void initActions();
    void initToolBar();
    void initSearchClientToolView();
    void initTvActions();
    void initExternalTools();
    void accountInit(AccountCacheItem *ci);
    subWindowType activedMdiChild();
    void addSubWindowMenuItem(QList<MyMdiSubWindow*> windows);
    void showStatWindow(subWindowType winType);

    //菜单项启用性控制
    bool isContainRight(Right::RightCode rc);
    bool isContainRights(QSet<Right::RightCode> rcs);
    bool isSuiteEditable();
    bool isPzSetEditable();
    bool isPzEditable();
    void rfLogin();
    void rfMainAct(bool login);
    void rfSuiteAct();
    void rfPzSetOpenAct();
    void rfPzSetStateAct();
    void rfPzSetEditAct(bool editable);
    void rfPzStateEditAct(bool Editable, PzState state);
    void rfPzNaviAct();
    void rfBaEditAct();
    void rfSaveBtn();

    void rightWarnning(int right);
    void pzsWarning();
    void sqlWarning();    
    void showPzNumsAffected(int num);

    void initUndoView();
    //void clearUndo();
    void adjustViewMenus(ToolViewType t, bool isRestore = false);
    void adjustEditMenus(UndoType ut=UT_PZ, bool restore = false);

    bool exportCommonSubject();
    bool inspectVersionBeforeImport(QString versionText, BaseDbVersionEnum type, QString fileName,int &mv, int &sv);

    void loadSettings();
    void createTray();
    void adjustInterfaceForLock();
    void setAppTitle();

    Ui::MainWindow *ui;

    //ToolBars
    QToolBar *fileToolBar;
    QTimer *_catchTimer;

    QSignalMapper *windowMapper; //用于处理从窗口菜单中选择显示的窗口
    QSignalMapper* tvMapper;     //用于处理从视图菜单中选择显示的工具视图
    QSignalMapper* etMapper;     //用于处理外部工具菜单

    bool sortBy; //凭证集的排序方式（true：按凭证号，false：按自编号）

    QHash<ToolViewType,QDockWidget*> dockWindows;     //工具视图窗口集
    QHash<ToolViewType,QAction*> tvActions;           //与工具视图类型对应的QAction对象表

    //工具条上的部件
    CustomSpinBox* spnNaviTo;
    QToolButton* btnPrint;
    //与打印任务相相关的动作对象
    QAction* actPrintPreview;
    QAction* actPrintToPDF;
    QAction* actPrintToPrinter;
    QAction* actOutputToExcel;

    AccountSuiteManager* curSuiteMgr;
    DbUtil* dbUtil;

    QUndoStack* undoStack;     //Undo命令栈
    QUndoView* undoView;       //Undo视图
    QAction *undoAction, *redoAction; //执行undo，redo操作

    SuiteSwitchPanel* curSSPanel;                       //当前帐套切换面板对象
    QHash<subWindowType,MyMdiSubWindow*> commonGroups; //公共类（唯一性子窗口）
    QMultiHash<subWindowType,MyMdiSubWindow*> commonGroups_multi; //公共类（多子窗口共存）
    QHash<int,SubWinGroupMgr*> subWinGroups;       //帐套视图子窗口组表（键为帐套记录id）
    QHash<int,QList<PingZheng*> > historyPzSet;    //每个帐套视图当前正浏览的历史凭证列表
    QHash<int,int> historyPzSetIndex;              //每个帐套视图当前正浏览的历史凭证集的当前索引
    QHash<int,int> historyPzMonth;                 //每个账套视图当前装载的历史凭证的月份数
    QList<ExternalToolCfgItem*> eTools;            //外部工具配置项列表
    AppConfig* appCon;
    LockApp* lockObj;
    bool tagLock;

    QList<PingZheng*> pzs;  //用于保存季度或年度统计时的凭证集

public:
    bool showSplashScreen_;
    bool showTrayIcon_;
    //bool startingTray_;
    bool isMinimizeToTray_;
    bool minimizingTray_ ;

    QMenu *trayMenu_;
    QAction *_actShowMainWindow;
    QSystemTrayIcon *_traySystem;
    QShortcut* sc_lock;
 };

/*!
 * \brief The LockApp class
 */
class LockApp : public QObject{
    Q_OBJECT
public:
    LockApp(MainWindow* parent);

protected:
    bool	eventFilter(QObject * obj, QEvent * event);
signals:
    void unlock();
private:
    MainWindow* mainWin;
};

#endif // MAINWINDOW_H
