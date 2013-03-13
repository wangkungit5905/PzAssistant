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
#include "sqltooldialog.h"
#include "setupbasedialog.h"
#include "pzdialog2.h"
#include "widgets.h"
#include "subjectsearchform.h"



namespace Ui {
    class MainWindow;
}

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
    void setRepealCount(int c){pzRepeal.setText(c!=0?QString::number(c):"");}
    void setRecordingCount(int c){pzRecording.setText(c!=0?QString::number(c):"");}
    void setVerifyCount(int c){pzVerify.setText(c!=0?QString::number(c):"");}
    void setInstantCount(int c){pzInstat.setText(c!=0?QString::number(c):"");}
    void setPzCounts(int repeal,int recording,int verify,int instat,int amount);
    void resetPzCounts();
    void setUser(User* user);

public slots:
    void notificationProgress(int amount, int curp);
private:
    void startProgress(int amount);
    void endProgress();
    void adjustProgress(int value);

    QLabel pzSetDate,pzSetState,extraState,lblUser,pzCount,pzRepeal,pzRecording,pzVerify,pzInstat;
    QProgressBar* pBar;
    int pAmount;  //进度指示器的最大值
};



extern int mdiAreaWidth;       //主窗口Mdi区域宽度
extern int mdiAreaHeight;      //主窗口Mdi区域高度

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //可以在MDI区域打开的子窗口类型代码
    enum subWindowType{
        NONE       = 0,    //不指代任何字窗口类型
        PZEDIT     = 1,    //凭证编辑窗口
        PZSTAT     = 2,    //本期统计窗口
        CASHDAILY  = 3,    //现金日记账窗口
        BANKDAILY  = 4,    //银行日记账窗口
        DETAILSDAILY=5,    //明细科目日记账窗口
        TOTALDAILY = 6,    //总分类账窗口
        SETUPBASE  = 7,    //设置账户期初余额窗口
        SETUPBANK  = 8,    //设置开户行信息
        BASEDATAEDIT = 9,  //基本数据库编辑窗口
        GDZCADMIN =  10,   //固定资产管理窗口
        DTFYADMIN = 11,    //待摊费用管理窗口
        TOTALVIEW = 12,    //总账视图
        DETAILSVIEW = 13,  //明细账视图
        HISTORYVIEW = 14,  //历史凭证
        LOOKUPSUBEXTRA =15,//查看科目余额
        ACCOUNTPROPERTY=16,//查看账户属性
        VIEWPZSETERROR=17  //查看凭证错误窗口
        //设置期初余额的窗口
        //科目配置窗口
    };

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void getMdiAreaSize(int &mdiAreaWidth, int &mdiAreaHeight);
    void hideDockWindows();
protected:
    void closeEvent(QCloseEvent *event);

private slots:
    //文件菜单
    void newAccount();
    void openAccount();
    void closeAccount();
    void refreshActInfo();
    void exit();

    //帐务处理
    void openPzs();
    void closePzs();
    void editPzs();
    void autoAssignPzNum();
    void handAssignPzNum();

    //数据菜单
    void viewSubjectExtra();      //显示科目余额
    void generateBalanceSheet();  //生成资产负债表
    void generateIncomeStatements(); //生成利润表

    //工具菜单
    void setupAccInfos();
    bool impBasicDatas();
    void setupBase();
    void setupBankInfos();
    void showSqlTool();

    //选项菜单
    void subjectConfig();

    //窗口菜单
    void updateWindowMenu();
    void setActiveSubWindow(QWidget *window);

    //帮助菜单
    void about();

    //处理创建新账户的向导
    void toCrtAccNextStep(int curStep, int nextStep);

    //处理移动业务活动按钮是否启用
    //void canMoveUp(bool isCan);
    //void canMoveDown(bool isCan);

    //打开指定id值的凭证（接受外部对话框窗口的请求，比如显示日记账或明细账的对话框）
    void openSpecPz(int pid,int bid);

    void curPzNumChanged(int pzNum);

    void subWindowClosed(QMdiSubWindow* subWin);
    void subWindowActivated(QMdiSubWindow *window);

    void pzStateChange(int scode);
    void pzContentChanged(bool isChanged = true);
    void curPzIndexChanged(int idx, int nums);
    void pzContentSaved();

    void userModifyPzState(bool checked);
    void userSelBaAction(bool isSel);
    void showTemInfo(QString info);
    void refreshShowPzsState();
    void extraChanged(){isExtraVolid = false;} //由于凭证集发生了影响统计余额的改变，导致当前余额失效，
                                               //目前主要由凭证编辑窗口的编辑动作引起，反馈给主窗口
    void extraValid();

    void on_actAddPz_triggered();

    void on_actDelPz_triggered();

    void on_actAddAction_triggered();

    void on_actDelAction_triggered();

    void on_actGoFirst_triggered();

    void on_actGoPrev_triggered();

    void on_actGoNext_triggered();

    void on_actGoLast_triggered();

    void on_actSavePz_triggered();

    void on_actFordPl_triggered();

    void on_actFordEx_triggered();

    void on_actCurStat_triggered();

    void on_actCashJournal_triggered();

    void on_actBankJournal_triggered();

    void on_actSubsidiaryLedger_triggered();

    void on_actLedger_triggered();    

    void on_actPrint_triggered();

    void on_actPrintAdd_triggered();

    void on_actPrintDel_triggered();

    void on_actSortByZb_triggered();

    void on_actSortByPz_triggered();

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

    void on_actAntiVerify_triggered();

    void on_actInsertPz_triggered();

    void on_actReassignPzNum_triggered();

    void on_actImpOtherPz_triggered();

    void on_actJzbnlr_triggered();

    void on_actEndAcc_triggered();

    void on_actAntiJz_triggered();

    void on_actAntiEndAcc_triggered();

    void on_actAntiImp_triggered();

    void on_actBasicDB_triggered();

    void on_actGdzcAdmin_triggered();

    void on_actDtfyAdmin_triggered();

    void on_actSetPzCls_triggered();

    void on_actShowTotal_triggered();

    void on_actShowDetail_triggered();

    void on_actAccProperty_triggered();

    void on_actJzHdsy_triggered();

    void on_actPzErrorInspect_triggered();

    void on_actForceDelPz_triggered();


    bool impTestDatas();
    void on_actViewLog_triggered();

private:
    void allPzToRecording(int year, int month);
    void initActions();
    void initToolBar();
    void initDockWidget();
    void accountInit();    
    QDialog* activeMdiChild();
    void refreshTbrVisble();
    void refreshActEnanble();
    void refreshAdvancPzOperBtn();
    void enaActOnLogin(bool isEnabled);    
    bool isIncoming(int id);
    void rightWarnning(int right);
    void pzsWarning();
    void sqlWarning();
    void showPzNumsAffected(int num);
    void saveSubWinInfo(subWindowType winEnum, QByteArray* sinfo = NULL);

    void showSubWindow(subWindowType winType, SubWindowDim* dinfo = NULL, QDialog* w = NULL);

    bool jzsy();
    bool jzhdsy();

    Ui::MainWindow *ui;

    //ToolBars
    QToolBar *fileToolBar;

    //创建新账户的4个步骤的对话框
    //CreateAccountDialog* dlgAcc;
    SetupBankDialog* dlgBank;
    BasicDataDialog* dlgData;
    SetupBaseDialog* dlgBase;

    QSignalMapper *windowMapper; //用于处理从窗口菜单中选择显示的窗口

    //各个子窗体内的中心部件指针

//    DetailsViewDialog2* dlgCashDaily;   //现金日记账窗口
//    DetailsViewDialog2* dlgBankDaily;   //银行日记账窗口
//    DetailsViewDialog2* dlgDetailDaily; //明细账窗口
//    DetailsViewDialog2* dlgTotalDaily;  //总分类账窗口

    //QSignalMapper* BasicDataTabMapper; //将组中的每个菜单项映射到同一个槽中
    CustomRelationTableModel* curPzModel; //当前打开的凭证集
    int cursy,cursm,curey,curem,cursd,cured; //当前打开的凭证集的起始年、月、日

    QSet<int> PrintPznSet; //欲打印的凭证号集合
    int curPzn; //当前凭证号（打印功能所需）
    bool sortBy; //凭证集的排序方式（true：按凭证号，false：按自编号）

    //QSet<subWindowType> subWinSet;  //需要保存子窗口信息的窗口枚举类型集合（这些子窗口都是单例的）
    QHash<subWindowType, MyMdiSubWindow*> subWindows; //已打开的子窗口（这些窗口只能有一个实例）
    QHash<subWindowType,bool> subWinActStates;   //子窗口的激活状态

    //状态条部件
    //QLabel *l1,*lblPzsDate;
    //QLabel *l2,*lblPzSetState;
    //QLabel *l3,*lblCurUser;


    //工具条上的部件
    CustomSpinBox* spnNaviTo;

    //可停靠窗口
    QDockWidget *subjectSearchDock;
    SubjectSearchForm* sfm;  //客户名查询

    bool isOpenPzSet;
    int curPzState;  //当前凭证的状态代码，用以确定高级凭证操作按钮的启用状态
    bool isExtraVolid; //当前凭证集的余额是否有效
    //bool isRefreshPzSetState;  //是否需要刷新凭证集状态
    int pzRepeal,pzRecording,pzVerify,pzInstat,pzAmount; //作废、录入、审核、入账凭证数，总数
    PzClass curPzCls; //当前凭证类别
    PzsState curPzSetState; //当前凭证集状态
    QRadioButton *rdoRecording, *rdoRepealPz, *rdoInstatPz, *rdoVerifyPz;
    QAction *actRecording;/*, *actRepeal, *actVerify, *actInstat;*/

 };
#endif // MAINWINDOW_H
