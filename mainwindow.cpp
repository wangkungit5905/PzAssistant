#include <QDir>
#include <QDebug>
#include <QPrinter>
#include <QPrintDialog>
#include <QDialog>
#include <QPageSetupDialog>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QVariant>
#include <QMdiSubWindow>
#include <QInputDialog>
#include <QUndoCommand>
#include <QUndoView>

#include <QBuffer>

#include "mainwindow.h"
#include "tables.h"
#include "ui_mainwindow.h"
#include "connection.h"
#include "global.h"
#include "subjectConfigDialog.h"
#include "sqltooldialog.h"
#include "databaseaccessform.h"
#include "utils.h"
#include "dialog2.h"
#include "dialog3.h"
#include "printUtils.h"
#include "securitys.h"
#include "commdatastruct.h"
#include "viewpzseterrorform.h"
#include "aboutform.h"
#include "dbutil.h"
#include "logs/logview.h"
#include "version.h"
#include "subject.h"
#include "PzSet.h"
#include "curstatdialog.h"
#include "statutil.h"
#include "showdzdialog2.h"
#include "pzdialog.h"
#include "statements.h"
#include "accountpropertyconfig.h"
#include "suiteswitchpanel.h"

#include "completsubinfodialog.h"


//#include "variousutils.h"

//#include "printtemplate.h"

#include "ui_mainwindow.h"

//临时
#include "printtemplate.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include "previewdialog.h"
#include "account.h"
#include "cal.h"
#include "tem.h"


/////////////////////////////////PaStatusBar////////////////////////////////
PaStatusBar::PaStatusBar(QWidget *parent):QStatusBar(parent)
{
    timer = NULL;
    QLabel *l = new QLabel(tr("日期:"),this);
    pzSetDate.setFrameShape(QFrame::StyledPanel);
    pzSetDate.setFrameShadow(QFrame::Sunken);
    pzSetDate.setText("                       ");    
    QHBoxLayout* hl1 = new QHBoxLayout(this);
    hl1->addWidget(l);
    hl1->addWidget(&pzSetDate);

    l = new QLabel(tr("凭证集状态："),this);
    pzSetState.setFrameShape(QFrame::StyledPanel);
    pzSetState.setFrameShadow(QFrame::Sunken);
    pzSetState.setText("              ");

    QHBoxLayout* hl2 = new QHBoxLayout(this);
    hl2->addWidget(l);    hl2->addWidget(&pzSetState);
    l = new QLabel(tr("余额状态："),this);
    extraState.setFrameShape(QFrame::StyledPanel);
    extraState.setFrameShadow(QFrame::Sunken);
    hl2->addWidget(l);    hl2->addWidget(&extraState);

    l = new QLabel(tr("凭证总数:"),this);
    pzCount.setFrameShadow(QFrame::Sunken);
    pzCount.setFrameShape(QFrame::StyledPanel);
    pzCount.setText("   ");
    QHBoxLayout* hl3 = new QHBoxLayout(this);
    hl3->addWidget(l);
    hl3->addWidget(&pzCount);
    l = new QLabel(tr("作废："));
    pzRepeal.setFrameShadow(QFrame::Sunken);
    pzRepeal.setFrameShape(QFrame::StyledPanel);
    pzRepeal.setText(" ");
    hl3->addWidget(l);hl3->addWidget(&pzRepeal);
    l = new QLabel(tr("录入："));
    pzRecording.setFrameShadow(QFrame::Sunken);
    pzRecording.setFrameShape(QFrame::StyledPanel);
    pzRecording.setText("   ");
    hl3->addWidget(l);hl3->addWidget(&pzRecording);
    l = new QLabel(tr("审核："));
    pzVerify.setFrameShadow(QFrame::Sunken);
    pzVerify.setFrameShape(QFrame::StyledPanel);
    pzVerify.setText(" ");
    hl3->addWidget(l);hl3->addWidget(&pzVerify);
    l = new QLabel(tr("入账："));
    pzInstat.setFrameShadow(QFrame::Sunken);
    pzInstat.setFrameShape(QFrame::StyledPanel);
    pzInstat.setText(" ");
    hl3->addWidget(l);hl3->addWidget(&pzInstat);

    l = new QLabel(tr("用户:"),this);
    lblUser.setFrameShadow(QFrame::Sunken);
    lblUser.setFrameShape(QFrame::StyledPanel);
    lblUser.setText("           ");
    QHBoxLayout* hl4 = new QHBoxLayout(this);
    hl4->addWidget(l);   hl4->addWidget(&lblUser);

    QHBoxLayout* ml = new QHBoxLayout(this);
    ml->addLayout(hl1);ml->addLayout(hl2);
    ml->addLayout(hl3);ml->addLayout(hl4);
    QFrame *f = new QFrame(this);
    f->setFrameShape(QFrame::StyledPanel);
    f->setLayout(ml);
    f->setFrameShadow(QFrame::Sunken);

    lblRuntime = new QLabel;
    lblRuntime->setFrameShadow(QFrame::Sunken);
    lblRuntime->setFrameShape(QFrame::StyledPanel);
    addPermanentWidget(lblRuntime,2);
    addPermanentWidget(f,1);

    pBar = new QProgressBar;

    //初始化不同级别的状态信息所采用的样式表颜色
    colors[AE_OK] = "color: rgb(0, 85, 0)";
    colors[AE_WARNING] = "color: rgb(255, 0, 127)";
    colors[AE_CRITICAL] = "color: rgb(85, 0, 0)";
    colors[AE_ERROR] = "color: rgb(255, 0, 0)";

}

PaStatusBar::~PaStatusBar()
{
    delete pBar;
}

/**
 * @brief PaStatusBar::setDate
 * 设置凭证集日期信息
 * @param date
 */
void PaStatusBar::setPzSetDate(int y, int m)
{
    if(y==0 or m==0){
        pzSetDate.setText("");
        return;
    }
    QDate d(y,m,1);
    int dend = d.daysInMonth();
    QString ds = tr("%1年%2月1日——%1年%2月%3日").arg(y).arg(m).arg(dend);
    pzSetDate.setText(ds);
}

/**
 * @brief PaStatusBar::setPzSetState
 * 设置凭证集状态信息
 * @param state
 */
void PaStatusBar::setPzSetState(PzsState state)
{
    pzSetState.setText(pzsStates.value(state));
    if(state == Ps_NoOpen){
        setPzCounts(0,0,0,0,0);
        setExtraState(false);
    }
}

void PaStatusBar::setPzCounts(int repeal, int recording, int verify, int instat, int amount)
{
    setRepealCount(repeal);
    setRecordingCount(recording);
    setVerifyCount(verify);
    setInstantCount(instat);
    setPzAmount(amount);
}

void PaStatusBar::resetPzCounts()
{
    setRepealCount(0);
    setRecordingCount(0);
    setVerifyCount(0);
    setInstantCount(0);
    setPzAmount(0);
}


/**
 * @brief PaStatusBar::setUser
 * 设置当前登入用户
 * @param user
 */
void PaStatusBar::setUser(User *user)
{
    if(user)
        lblUser.setText(user->getName());
    else
        lblUser.setText("");
}

void PaStatusBar::showRuntimeMessage(QString info, AppErrorLevel level)
{
    if(!timer){
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(timeoutOfTemInfo);
        connect(timer,SIGNAL(timeout()),this,SLOT(timeout()));
    }
    lblRuntime->setStyleSheet(colors.value(level));
    lblRuntime->setText(info);
    timer->start();
}

/**
 * @brief PaStatusBar::startProgress
 * 开始一个进度条指示
 * @param amount
 */
void PaStatusBar::startProgress(int amount)
{
    pBar->setRange(0,amount);
    pBar->setValue(0);
    addWidget(pBar,1);
}

/**
 * @brief PaStatusBar::adjustProgress
 * 调整进度指示
 * @param value
 */
void PaStatusBar::adjustProgress(int value)
{
    pBar->setValue(value);
}

/**
 * @brief PaStatusBar::notificationProgress
 * 进度通知
 * @param amount：进度最大值
 * @param curp：进度当前值（如果为0，则启动进度，如果是0到amount的值，则显示进度，其他值，则终止进度）
 */
void PaStatusBar::notificationProgress(int amount, int curp)
{
    if(curp == 0){
        pAmount = amount;
        startProgress(amount);
    }
    else if(curp > 0 && curp < amount)
        adjustProgress(curp);
    else
        endProgress();
}

/**
 * @brief PaStatusBar::timeout
 *  在显示运行时信息时，当时间超期后，清除信息
 */
void PaStatusBar::timeout()
{
    lblRuntime->clear();
}

/**
 * @brief PaStatusBar::endProgress
 * 终止进度条指示
 */
void PaStatusBar::endProgress()
{
    removeWidget(pBar);
}


////////////////////////////SubWinGroupMgr//////////////////////////////////////////////////////////////////////////////////////////
void SubWinGroupMgr::show()
{
    QHashIterator<subWindowType,MyMdiSubWindow*> it(subWinHashs);
    while(it.hasNext()){
        it.next();
        it.value()->show();
    }
    isShowed = true;
}

void SubWinGroupMgr::hide()
{
    QHashIterator<subWindowType,MyMdiSubWindow*> it(subWinHashs);
    while(it.hasNext()){
        it.next();
        it.value()->hide();
    }
    isShowed = false;
}

/**
 * @brief 显示指定类型的子窗口，如果不存在，则窗口
 * @param winType   子窗口类型
 * @param widget    子窗口的中心部件
 * @return
 */
MyMdiSubWindow* SubWinGroupMgr::showSubWindow(subWindowType winType, QWidget *widget, SubWindowDim* winfo)
{
    if(winType == SUBWIN_NONE)
        return NULL;
    else if(subWinHashs.contains(winType)){
        parent->setActiveSubWindow(subWinHashs.value(winType));
        return subWinHashs.value(winType);
    }

//    QByteArray* state = NULL;
//    SubWindowDim* dim = NULL;
//    if(curAccount)
//        curAccount->getDbUtil()->getSubWinInfo(winType,dim,state);

//    if(winType == SUBWIN_PZEDIT_new){
//        PzDialog* w = static_cast<PzDialog*>(widget);
//        if(w)
//            w->setState(state);
//    }
//    else if(winType == SUBWIN_HISTORYVIEW){
//        HistoryPzForm* w = static_cast<HistoryPzForm*>(widget);
//        if(w)
//            w->setState(state);
//    }
//    else if(winType == SUBWIN_DETAILSVIEW2){
//        ShowDZDialog2* w = static_cast<ShowDZDialog2*>(widget);
//        if(w)
//            w->setState(state);
//    }

    MyMdiSubWindow* sw = new MyMdiSubWindow(groupId,winType);
    sw->setWidget(widget);
    sw->setAttribute(Qt::WA_DeleteOnClose);
    parent->addSubWindow(sw);
    subWinHashs[winType] = sw;
    connect(sw,SIGNAL(windowClosed(MyMdiSubWindow*)),this,SLOT(subWindowClosed(MyMdiSubWindow*)));
    if(winfo){
        sw->resize(winfo->w,winfo->h);
        sw->move(winfo->x,winfo->y);
    }
    sw->show();
    return sw;
}

QWidget *SubWinGroupMgr::getSubWinWidget(subWindowType winType)
{
    if(!subWinHashs.contains(winType))
        return NULL;
    return subWinHashs.value(winType)->widget();
}

void SubWinGroupMgr::closeSubWindow(subWindowType winType)
{
    if(!subWinHashs.contains(winType))
        return;
    parent->removeSubWindow(subWinHashs.value(winType));
    delete subWinHashs.value(winType);
    subWinHashs.remove(winType);
}

void SubWinGroupMgr::subWindowClosed(MyMdiSubWindow *subWin)
{
    subWindowType t = subWin->getWindowType();
    QByteArray* state = NULL;
    SubWindowDim* dim = new SubWindowDim;
    dim->x = subWin->x();
    dim->y = subWin->y();
    dim->w = subWin->width();
    dim->h = subWin->height();
    if(t == SUBWIN_PZEDIT_new){
        PzDialog* w = static_cast<PzDialog*>(subWin->widget());
        state = w->getState();
        delete w;
        disconnect(w,SIGNAL(showMessage(QString,AppErrorLevel)),this,SLOT(showRuntimeInfo(QString,AppErrorLevel)));
        disconnect(w,SIGNAL(selectedBaChanged(QList<int>,bool)),this,SLOT(baSelectChanged(QList<int>,bool)));
    }
    else if(t == SUBWIN_HISTORYVIEW){
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWin->widget());
        state = w->getState();
        delete w;
    }
    else if(t == SUBWIN_DETAILSVIEW2){
        ShowDZDialog2* w = static_cast<ShowDZDialog2*>(subWin->widget());
        state = w->getState();
        delete w;
        disconnect(w,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    else if(t == SUBWIN_VIEWPZSETERROR){
        ViewPzSetErrorForm* w = static_cast<ViewPzSetErrorForm*>(subWin->widget());
        state = w->getState();
        delete w;
        disconnect(w,SIGNAL(reqLoation(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    //emit saveSubWinState(t,state,dim);
    if(curAccount)
        curAccount->getDbUtil()->saveSubWinInfo(t,dim,state);
    subWinHashs.remove(subWin->getWindowType());
    delete dim;
    if(state)
        delete state;
}


///////////////////////////////////////MainWindow////////////////////////////////

int mdiAreaWidth;
int mdiAreaHeight;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setCentralWidget(ui->mdiArea);

    curSuiteMgr = NULL;
    //commonGroups = NULL;

    //curPzSetState = Ps_NoOpen;
    sortBy = true; //默认按凭证号进行排序
    dbUtil = NULL;
    undoStack = NULL;
    undoView = NULL;




    cursy = 0;
    cursm = 0;
    cursd = 0;
    curey = 0;
    curem = 0;
    cured = 0;

    //初始化对话框窗口指针为NULL
    //dlgAcc = NULL;
    dlgBank = NULL;
    dlgData = NULL;

    //curPzn = 0;

    initActions();
    initToolBar();
    initTvActions();


    mdiAreaWidth = ui->mdiArea->width();
    mdiAreaHeight = ui->mdiArea->height();


    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)),
            this, SLOT(setActiveSubWindow(QWidget*)));


    on_actLogin_triggered(); //显示登录窗口
    if(!curUser)
        return;

    AppConfig* appCfg = AppConfig::getInstance();
    AccountCacheItem recentAcc;
    recentAcc.lastOpened = false;
    if(appCfg->getRecendOpenAccount(recentAcc) && recentAcc.lastOpened){
        if(!AccountVersionMaintain(recentAcc.fileName)){
            setWindowTitle(QString("%1---%2").arg(appTitle)
                           .arg(tr("无账户被打开")));
            rfMainAct(false);
            return;
        }

        curAccount = new Account(recentAcc.fileName);
        if(!curAccount->isValid()){
            showTemInfo(tr("账户文件无效，请检查账户文件内信息是否齐全！！"));
            delete curAccount;
            curAccount = NULL;
            setWindowTitle(QString("%1---%2").arg(appTitle)
                           .arg(tr("无账户被打开")));
            rfMainAct(false);
            return;
        }
        accountInit();
        rfMainAct();
        setWindowTitle(QString("%1---%2").arg(appTitle).arg(curAccount->getLName()));
    }
    else {//禁用只有在账户打开的情况下才可以的部件
        setWindowTitle(QString("%1---%2").arg(appTitle)
                       .arg(tr("无账户被打开")));
        rfMainAct(false);
    }

    refreshShowPzsState();
    //rfTbrVisble();
    //refreshActEnanble();
    rfPzSetAct(false);
    rfNaveBtn(NULL,NULL);
    //rfAct();
}

MainWindow::~MainWindow()
{
    delete ui;
    //初始化对话框窗口指针为NULL
    //if(dlgAcc != NULL)
    //    delete dlgAcc;
    if(dlgBank != NULL)
        delete dlgBank;
    if(dlgData != NULL)
        delete dlgData;
}

//获取主窗口的Mdi区域的大小（必须在主窗口显示后调用）
void MainWindow::getMdiAreaSize(int &mdiAreaWidth, int &mdiAreaHeight)
{
    mdiAreaWidth = ui->mdiArea->width();
    mdiAreaHeight = ui->mdiArea->height();
}

void MainWindow::hideDockWindows()
{
//    QAction* act = subjectSearchDock->toggleViewAction();
//    act->trigger();
}

/**
 * @brief MainWindow::AccountVersionMaintain
 *  执行账户的文件版本升级服务
 * @param fname 账户文件名
 * @return true：成功升级或无须升级，false：升级出错或被用户取消，或应用版本相对于账户版本过低、或账户版本无法归集到初始版本等
 */
bool MainWindow::AccountVersionMaintain(QString fname)
{
    bool cancel = false;
    VersionManager vm(VersionManager::MT_ACC,fname);
    VersionUpgradeInspectResult result = vm.isMustUpgrade();
    bool exec = false;
    switch(result){
    case VUIR_CANT:
        QMessageBox::warning(this,tr("出错信息"),
                             tr("该账户数据库版本不能归集到初始版本！"));
        return false;
    case VUIR_DONT:
        exec = false;
        break;
    case VUIR_LOW:
        QMessageBox::warning(this,tr("出错信息"),
                             tr("当前程序版本太低，必须要用更新版本的程序打开此账户！"));
        return false;
    case VUIR_MUST:
        exec = true;
        break;
    }
    if(exec){
        if(vm.exec() == QDialog::Rejected){
            QMessageBox::warning(this,tr("出错信息"),
                                      tr("该账户数据库版本过低，必须先升级！"));
            return false;
        }
        else if(!vm.getUpgradeResult())
            return false;
    }
    return true;
}

void MainWindow::initActions()
{
    //文件菜单
    connect(ui->actCrtAccount, SIGNAL(triggered()), this, SLOT(newAccount()));
    connect(ui->actOpenAccount, SIGNAL(triggered()), this, SLOT(openAccount()));
    connect(ui->actCloseAccount, SIGNAL(triggered()), this, SLOT(closeAccount()));

    connect(ui->actImpActFromFile, SIGNAL(triggered()), this, SLOT(attachAccount()));
    connect(ui->actEmpActToFile, SIGNAL(triggered()), this, SLOT(detachAccount()));
    connect(ui->actExit,SIGNAL(triggered()),this,SLOT(exit()));    

    //数据菜单
    connect(ui->actSubExtra, SIGNAL(triggered()), this, SLOT(viewSubjectExtra()));

    //报表
    connect(ui->actBalanceSheet, SIGNAL(triggered()), this, SLOT(generateBalanceSheet()));
    connect(ui->actIncomeStatements, SIGNAL(triggered()), this, SLOT(generateIncomeStatements()));

    //帐务处理
    connect(ui->actOpenPzs, SIGNAL(triggered()), this, SLOT(openPzs())); //打开凭证集
    connect(ui->actClosePzs, SIGNAL(triggered()), this, SLOT(closePzs())); //关闭凭证集
    connect(ui->actEditPzs, SIGNAL(triggered()), this, SLOT(editPzs())); //编辑凭证

    //选项菜单
    connect(ui->actSubConfig, SIGNAL(triggered()), this, SLOT(subjectConfig()));//科目配置

    //工具菜单
    connect(ui->actSetupAccInfos, SIGNAL(triggered()), this, SLOT(setupAccInfos()));  //设置账户信息
    connect(ui->actImpBasicDatas, SIGNAL(triggered()), this, SLOT(impBasicDatas()));  //导入基础数据
    connect(ui->actSetupBase, SIGNAL(triggered()), this, SLOT(setupBase()));          //设置记账基点
    connect(ui->actSetupBankInfos, SIGNAL(triggered()), this, SLOT(setupBankInfos()));//设置开户行信息
    connect(ui->actImpTestDatas, SIGNAL(triggered()), this, SLOT(impTestDatas()));    //导入测试数据
    connect(ui->actSqlTool, SIGNAL(triggered()), this, SLOT(showSqlTool()));          //Sql工具

    //窗口菜单
    connect(ui->mnuWindow, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

    //帮助菜单
    connect(ui->actAbout, SIGNAL(triggered()), this, SLOT(about()));
}

//初始化工具条上的某些不能在设计器中直接添加的组件
void MainWindow::initToolBar()
{
    spnNaviTo = new CustomSpinBox;
    spnNaviTo->setEnabled(false);
    connect(spnNaviTo,SIGNAL(userEditingFinished()),
            this,SLOT(on_actNaviToPz_triggered()));
    ui->tbrPzs->insertWidget(ui->actNaviToPz, spnNaviTo);

//    QActionGroup* g = new QActionGroup(this);
//    g->addAction(ui->actRecordPz);
//    g->addAction(ui->actVerifyPz);
//    g->addAction(ui->actInStatPz);

    //connect(ui->actRecordPz,SIGNAL(triggered(bool)),this,SLOT(userModifyPzState(bool)));
    //connect(ui->actVerifyPz,SIGNAL(triggered(bool)),this,SLOT(userModifyPzState(bool)));
    //connect(ui->actInStatPz,SIGNAL(triggered(bool)),this,SLOT(userModifyPzState(bool)));

//    QGroupBox* pzsBox = new QGroupBox;
//    rdoRecording = new QRadioButton(tr("录入态"),pzsBox);
//    rdoRepealPz = new QRadioButton(tr("已作废"),pzsBox);
//    rdoInstatPz = new QRadioButton(tr("已入账"),pzsBox);
//    rdoVerifyPz = new QRadioButton(tr("已审核"),pzsBox);
//    actRecording =  ui->tbrAdvanced->addWidget(rdoRecording);
//    actRecording->setVisible(false);
//    ui->tbrAdvanced->addWidget(rdoRepealPz);
//    ui->tbrAdvanced->addWidget(rdoVerifyPz);
//    ui->tbrAdvanced->addWidget(rdoInstatPz);
//    connect(rdoRepealPz, SIGNAL(clicked(bool)), this, SLOT(userModifyPzState(bool)));
//    connect(rdoVerifyPz, SIGNAL(clicked(bool)), this, SLOT(userModifyPzState(bool)));
//    connect(rdoInstatPz, SIGNAL(clicked(bool)), this, SLOT(userModifyPzState(bool)));

}

/**
 * @brief MainWindow::initSearchClientToolView
 *  初始化搜索客户的工具视图
 */
void MainWindow::initSearchClientToolView()
{
    if(!dockWindows.contains(TV_SEARCHCLIENT)){
        QDockWidget* dw = new QDockWidget(tr("搜索客户名"), this);
        dw->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        SubjectSearchForm* sfm = new SubjectSearchForm(dw);
        sfm->attachDb(&curAccount->getDbUtil()->getDb());
        dw->setWidget(sfm);
        addDockWidget(Qt::LeftDockWidgetArea, dw);
        dockWindows[TV_SEARCHCLIENT] = dw;
        tvMapper->removeMappings(tvActions.value(TV_SEARCHCLIENT));
    }
    adjustViewMenus(TV_SEARCHCLIENT);
//    QList<QAction*> actions;
//    actions = ui->mnuView->actions();
//    actions.replace(1,dockWindows.value(TV_SEARCHCLIENT)->toggleViewAction());
//    ui->mnuView->clear();
//    ui->mnuView->addActions(actions);
}

/**
 * @brief MainWindow::initTvActions
 *  初始化工具视图菜单
 */
void MainWindow::initTvActions()
{
    QList<QAction*> actions;
    QAction* act;

    tvMapper = new QSignalMapper(this);
    connect(tvMapper,SIGNAL(mapped(int)),this,SLOT(showAndHideToolView(int)));

    act = new QAction(tr("Undo视图"),this);
    tvActions[TV_UNDO] = act;
    tvMapper->setMapping(act,TV_UNDO);
    actions<<act;
    act = new QAction(tr("搜索客户"),this);
    tvActions[TV_SEARCHCLIENT] = act;
    tvMapper->setMapping(act,TV_SEARCHCLIENT);
    actions<<act;

    ui->mnuView->addActions(actions);
    for(int i = 0; i < actions.count(); ++i){
        actions.at(i)->setCheckable(true);
        connect(actions.at(i),SIGNAL(triggered()),tvMapper,SLOT(map()));
    }
}



//打开账户时的初始化设置工作
void MainWindow::accountInit()
{
    dbUtil = curAccount->getDbUtil();
    curSuiteMgr = curAccount->getPzSet();
    if(!curSuiteMgr)
        QMessageBox::warning(this,tr("友情提示"),tr("本账户还没有设置任何帐套，请在账户属性设置窗口的帐套页添加一个帐套以供帐务处理！"));
    else
        connect(curSuiteMgr,SIGNAL(pzCountChanged(int)),this,SLOT(pzCountChanged(int)));
    undoStack = curSuiteMgr?curSuiteMgr->getUndoStack():NULL;
    //connect(pzSetMgr,SIGNAL(pzSetChanged()),this, SLOT(PzChangedInSet()));
    if(!BusiUtil::init(curAccount->getDbUtil()->getDb()))
        QMessageBox::critical(this,tr("错误信息"),tr("BusiUtil对象初始化阶段发生错误！"));
    //initSearchClientToolView();
    AccountSuiteRecord* curSuite = curAccount->getCurSuite();
    if(curSuite && !subWinGroups.contains(curSuite->id))
        subWinGroups[curSuite->id] = new SubWinGroupMgr(curSuite->id,ui->mdiArea);
}

//动态更新窗口菜单
void MainWindow::updateWindowMenu()
{
    ui->mnuWindow->clear();
    //ui->mnuWindow->addAction(subjectSearchDock->toggleViewAction());
    ui->mnuWindow->addAction(ui->actCloseCurWindow);
    ui->mnuWindow->addAction(ui->actCloseAllWindow);
    ui->mnuWindow->addSeparator();
    ui->mnuWindow->addAction(ui->actTileWindow);
    ui->mnuWindow->addAction(ui->actCascadeWindow);
    ui->mnuWindow->addSeparator();
    ui->mnuWindow->addAction(ui->actPrevWindow);
    ui->mnuWindow->addAction(ui->actNextWindow);
    QAction *separatorAct = new QAction(this);
    separatorAct->setSeparator(true);
    ui->mnuWindow->addAction(separatorAct);

    QList<QMdiSubWindow *> windows = ui->mdiArea->subWindowList();
    separatorAct->setVisible(!windows.isEmpty());

    for(int i = 0; i < windows.size(); ++i){
        QMdiSubWindow* w = windows.at(i);
        QString text;
        if (i < 9)
            text = tr("&%1 %2").arg(i + 1).arg(w->windowTitle());
        else
            text = tr("%1 %2").arg(i + 1).arg(w->windowTitle());
        QAction *action  = ui->mnuWindow->addAction(text);
        action->setChecked(w == ui->mdiArea->activeSubWindow());
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, windows.at(i));
    }
//    for(int i = 0; i < windows.size(); ++i){
//        QDialog* child = qobject_cast<QDialog*>(windows.at(i)->widget());

//        QString text;
//        if (i < 9) {
//            text = tr("&%1 %2").arg(i + 1)
//                               .arg(child->windowTitle());
//        } else {
//            text = tr("%1 %2").arg(i + 1)
//                              .arg(child->windowTitle());
//        }
//        QAction *action  = ui->mnuWindow->addAction(text);
//        action->setCheckable(true);
//        action->setChecked(child == activeMdiChild());
//        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
//        windowMapper->setMapping(action, windows.at(i));
//    }
}

//返回当前活动的窗口
subWindowType MainWindow::activedMdiChild()
{
//    if(subWinActStates.isEmpty())
//        return SUBWIN_NONE;
//    QHashIterator<subWindowType,bool> it(subWinActStates);
//    while(it.hasNext()){
//        it.next();
//        if(it.value())
//            return it.key();
//    }
//    return SUBWIN_NONE;
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(ui->mdiArea->activeSubWindow());
    if(w)
        return w->getWindowType();
    else
        return SUBWIN_NONE;
}

/**
 * @brief MainWindow::rfOther
 *  控制其他按钮的启用性
 */
void MainWindow::rfOther()
{
    //保存按钮

}

/**
 * @brief MainWindow::rfLogin
 *  控制与用户登录有关的菜单项可用性
 * @param login
 */
void MainWindow::rfLogin(bool login)
{
    ui->actLogin->setEnabled(!login);
    ui->actLogout->setEnabled(login);
    ui->actShiftUser->setEnabled(login);
    //如果当前登录的root用户，则启用安全配置菜单项，目前未实现，暂不启用
    //ui->actSecCon->setEnabled(true);

    //如果用户没有登录，则禁用所有与账户有关的所有菜单项
    if(!login){
        ui->actCrtAccount->setEnabled(false);
        ui->actOpenAccount->setEnabled(false);
        ui->actCloseAccount->setEnabled(false);
        ui->actAccProperty->setEnabled(false);
        ui->actSave->setEnabled(false);
        ui->actPrint->setEnabled(false);
        ui->actOpenPzs->setEnabled(false);
        ui->actClosePzs->setEnabled(false);
        ui->actEditPzs->setEnabled(false);
        ui->actCurStat->setEnabled(false);
        ui->actCurStatNew->setEnabled(false);
        ui->actDetailView->setEnabled(false);
        ui->actAddPz->setEnabled(false);
        ui->actInsertPz->setEnabled(false);
        ui->actDelPz->setEnabled(false);
        ui->actGoFirst->setEnabled(false);
        ui->actGoNext->setEnabled(false);
        ui->actGoPrev->setEnabled(false);
        ui->actGoLast->setEnabled(false);
        ui->actAddAction->setEnabled(false);
        ui->actInsertBa->setEnabled(false);
        ui->actDelAction->setEnabled(false);
        ui->actMvUpAction->setEnabled(false);
        ui->actMvDownAction->setEnabled(false);

        //......
    }

}

/**
 * @brief MainWindow::rfMainAct
 *  控制账户相关的命令的启用性
 */
void MainWindow::rfMainAct(bool open)
{
    ui->actCrtAccount->setEnabled(!open);
    ui->actOpenAccount->setEnabled(!open);
    ui->actCloseAccount->setEnabled(open);
    ui->actAccProperty->setEnabled(open);
    ui->actRefreshActInfo->setEnabled(!open);
    ui->actDelAcc->setEnabled(!open);
    rfPzSetAct(false);
    ui->actOpenPzs->setEnabled(open);
    rfNaveBtn(0,0);
}

/**
 * @brief MainWindow::rfPzSetAct
 *  控制凭证集相关命令的启用性
 */
void MainWindow::rfPzSetAct(bool open)
{
    ui->actOpenPzs->setEnabled(!open);
    ui->actClosePzs->setEnabled(open);
    ui->actEditPzs->setEnabled(open);
    ui->actSearch->setEnabled(open);
    ui->actNaviToPz->setEnabled(open);
    ui->actCurStat->setEnabled(open);
    ui->actCurStatNew->setEnabled(true);
    ui->actPzErrorInspect->setEnabled(open);
    //ui->actDetailView->setEnabled(true);
    bool r = (curSuiteMgr && (curSuiteMgr->getState() != Ps_Jzed));
    //ui->actJzHdsy->setEnabled(open && r);
    ui->actFordEx->setEnabled(open && r);
    ui->actFordPl->setEnabled(open && r);
    ui->actEndAcc->setEnabled(open && r);
    ui->actJzbnlr->setEnabled(open && r);

}

/**
 * @brief MainWindow::rfPzAct
 *  控制凭证相关命令的启用性
 */
void MainWindow::rfPzAct(bool enable)
{
//    ui->actAddPz->setEnabled(enable);
//    ui->actInsertPz->setEnabled(enable);
//    ui->actDelPz->setEnabled(enable);
//    ui->actGoFirst->setEnabled(enable);
//    ui->actGoLast->setEnabled(enable);
//    ui->actGoPrev->setEnabled(enable);
//    ui->actGoNext->setEnabled(enable);
//    ui->actMvUpAction->setEnabled(enable);
//    ui->actMvDownAction->setEnabled(enable);
//    ui->actAddAction->setEnabled(enable);
//    ui->actInsertBa->setEnabled(enable);
//    ui->actDelAction->setEnabled(enable);
}

/**
 * @brief MainWindow::rfAdvancedAct
 *  控制高级命令的启用性
 */
void MainWindow::rfAdvancedAct()
{
    //控制凭证状态的命令
    //控制凭证集结转、结账及其反过程的命令
}

/**
 * @brief MainWindow::rfEditAct
 *  控制编辑命令的启用性
 */
void MainWindow::rfEditAct()
{
}

/**
 * @brief MainWindow::rfAct
 *  控制所有命令的启用性一级工具条的可见性
 */
void MainWindow::rfAct()
{
    //rfMainAct(curAccount != NULL);
    //rfPzSetAct();
    //rfPzAct();
    //rfAdvancedAct();
    //rfNaveBtn();
    //rfTbrVisble();
}

//设置当前活动的窗口
void MainWindow::setActiveSubWindow(QWidget *window)
{
    if (!window)
        return;
    ui->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}

/////////////////////////文件菜单处理槽部分/////////////////////////////////////////
void MainWindow::newAccount()
{
//    dlgAcc = new CreateAccountDialog(this);
//    connect(dlgAcc, SIGNAL(toNextStep(int,int)), this, SLOT(toCrtAccNextStep(int,int)));
//    dlgAcc->show();
}

void MainWindow::openAccount()
{
    OpenAccountDialog* dlg = new OpenAccountDialog;
    if(dlg->exec() != QDialog::Accepted)
        return;

    //ui->tbrPzs->setVisible(true);
    AccountCacheItem* aci =dlg->getAccountCacheItem();
    if(!aci || !AccountVersionMaintain(aci->fileName)){
        setWindowTitle(QString("%1---%2").arg(appTitle)
                       .arg(tr("无账户被打开")));
        rfMainAct(false);
        return;
    }
    if(curAccount){
        delete curAccount;
        curAccount = NULL;
    }
    curAccount = new Account(aci->fileName);
    if(!curAccount->isValid()){
        showTemInfo(tr("账户文件无效，请检查账户文件内信息是否齐全！！"));
        delete curAccount;
        curAccount = NULL;
        setWindowTitle(QString("%1---%2").arg(appTitle)
                       .arg(tr("无账户被打开")));
        rfMainAct(false);
        return;
    }
    setWindowTitle(tr("会计凭证处理系统---") + curAccount->getLName());
    AppConfig::getInstance()->setRecentOpenAccount(aci->code);
    rfMainAct();
    //rfTbrVisble();
    //refreshActEnanble();
    accountInit();
    //initUndo();
}

void MainWindow::closeAccount()
{
    QString state;
    bool ok;
//    state = QInputDialog::getText(this,tr("账户关闭提示"),
//        tr("请输入表示账户最后关闭前的状态说明语"),QLineEdit::Normal,state,&ok);
//    if(!ok)
//        return;

    if(curSuiteMgr->isOpened())
        closePzs();

    disconnect(curSuiteMgr,SIGNAL(pzCountChanged(int)),this,SLOT(pzCountChanged(int)));
    //disconnect(pzSetMgr,SIGNAL(pzSetChanged()),this, SLOT(PzChangedInSet()));
    //关闭账户前，还应关闭与此账户有关的其他子窗口
    //if(subWindows.contains(SETUPBANK)){  //设置银行账户窗口
    //    subWindows.value(SETUPBANK)->close();
    //}

    //设置期初余额的窗口
    //科目配置窗口
    //......

    curAccount->setLastAccessTime(QDateTime::currentDateTime());
    curAccount->close();
    rfMainAct(false);
    undoStack = NULL;
    curSuiteMgr = NULL;
//    if(state == "")
//        state = tr("账户最后由用户%1于%2关闭！").arg(curUser->getName())
//                .arg(curAccount->getLastAccessTime().toString(Qt::ISODate));
//    curAccount->appendLog(QDateTime::currentDateTime(),state);

    delete curAccount;
    curAccount = NULL;
    //ui->tbrPzs->setVisible(false);
    setWindowTitle(tr("会计凭证处理系统---无账户被打开"));
    //rfAct();
    //rfTbrVisble();
    //refreshActEnanble();
}

//退出应用
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(curAccount)       //为了在下次打开应用时自动打开最后打开的账户
        closeAccount();
    ui->mdiArea->closeAllSubWindows();
    //appExit();
    if (ui->mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        //writeSettings();
        event->accept();
    }
}

void MainWindow::exit()
{
    if(curAccount)
        closeAccount();
    ui->mdiArea->closeAllSubWindows();
    close();
}

////////////////////帐务处理菜单处理槽部分////////////////////////////////
//打开凭证集
void MainWindow::openPzs()
{
    OpenPzDialog* dlg = new OpenPzDialog(curAccount);
    if(dlg->exec() == QDialog::Accepted){
        QDate d = dlg->getDate();
        cursy = d.year();
        cursm = d.month();
        curey = d.year();
        curem = d.month();
        cursd = 1;
        cured = d.daysInMonth();

        if(!curSuiteMgr->open(cursm)){
           QMessageBox::critical(this, tr("错误提示"),tr("打开%1年%2月的凭证集时出错！").arg(curSuiteMgr->year()).arg(cursm));
           return;
        }


        if(undoView)
            undoView->setStack(undoStack);
        connect(undoStack,SIGNAL(cleanChanged(bool)),this,SLOT(undoCleanChanged(bool)));
        connect(undoStack,SIGNAL(indexChanged(int)),this,SLOT(UndoIndexChanged(int)));
        connect(undoStack,SIGNAL(redoTextChanged(QString)),this,SLOT(redoTextChanged(QString)));
        connect(undoStack,SIGNAL(undoTextChanged(QString)),this,SLOT(undoTextChanged(QString)));
        connect(curSuiteMgr,SIGNAL(currentPzChanged(PingZheng*,PingZheng*)),this,SLOT(rfNaveBtn(PingZheng*,PingZheng*)));
        refreshShowPzsState();
        ui->statusbar->setPzSetDate(cursy,cursm);
        connect(curSuiteMgr,SIGNAL(pzSetStateChanged(PzsState)),this,SLOT(pzSetStateChanged(PzsState)));
        connect(curSuiteMgr,SIGNAL(pzExtraStateChanged(bool)),this,SLOT(pzSetExtraStateChanged(bool)));
        rfPzSetAct();
        //rfAct();
        //rfTbrVisble();
        //refreshActEnanble();
    }
}


void MainWindow::closePzs()
{
    //在关闭凭证集前，要检测是否有未保存的修改
    if(curSuiteMgr->isDirty()){
        if(QMessageBox::Accepted ==
                QMessageBox::warning(0,tr("提示信息"),tr("凭证集已被改变，要保存吗？"),
                                     QMessageBox::Yes|QMessageBox::No))
            if(!curSuiteMgr->save())
                QMessageBox::critical(0,tr("错误信息"),tr("保存凭证集时发生错误！"));

    }
    //关闭凭证集时，要先关闭所有与该凭证集相关的子窗口
    if(subWindows.contains(SUBWIN_PZEDIT_new))             //凭证编辑窗口（新）
        subWindows.value(SUBWIN_PZEDIT_new)->close();
    if(subWindows.contains(SUBWIN_PZSTAT))                 //显示本期统计的窗口
        subWindows.value(SUBWIN_PZSTAT)->close();
    if(subWindows.contains(SUBWIN_PZSTAT2))                //显示本期统计的窗口（新）
        subWindows.value(SUBWIN_PZSTAT2)->close();
    if(subWindows.contains(DETAILSVIEW))            //明细账窗口
        subWindows.value(DETAILSVIEW)->close();
    if(subWindows.contains(SUBWIN_DETAILSVIEW2))           //明细账窗口（新）
        subWindows.value(SUBWIN_DETAILSVIEW2)->close();
    if(subWindows.contains(TOTALDAILY))             //总分类账窗口
        subWindows.value(TOTALDAILY)->close();

    curSuiteMgr->close();
    rfPzSetAct(false);
    //clearUndo();
    if(undoView)
        undoView->setStack(NULL);
    disconnect(undoStack,SIGNAL(cleanChanged(bool)),this,SLOT(undoCleanChanged(bool)));
    disconnect(undoStack,SIGNAL(indexChanged(int)),this,SLOT(UndoIndexChanged(int)));
    disconnect(undoStack,SIGNAL(redoTextChanged(QString)),this,SLOT(redoTextChanged(QString)));
    disconnect(undoStack,SIGNAL(undoTextChanged(QString)),this,SLOT(undoTextChanged(QString)));
    disconnect(curSuiteMgr,SIGNAL(currentPzChanged(PingZheng*,PingZheng*)),this,SLOT(rfNaveBtn(PingZheng*,PingZheng*)));
    disconnect(curSuiteMgr,SIGNAL(pzSetStateChanged(PzsState)),this,SLOT(pzSetStateChanged(PzsState)));
    disconnect(curSuiteMgr,SIGNAL(pzExtraStateChanged(bool)),this,SLOT(pzSetExtraStateChanged(bool)));
    cursy = 0;cursm = 0;cursd = 0;curey = 0;curem = 0;cured = 0;

    ui->statusbar->setPzSetDate(0,0);
    ui->statusbar->setPzSetState(Ps_NoOpen);
    ui->statusbar->resetPzCounts();

    //rfAct();
    //rfTbrVisble();
    //refreshActEnanble();
    adjustEditMenus(UT_PZ,true);
}

//在当前打开的凭证集中进行录入、修改凭证等编辑操作
void MainWindow::editPzs()
{
    //如果已经打开了此窗口则退出
    if(subWindows.contains(SUBWIN_PZEDIT_new)){
        ui->mdiArea->setActiveSubWindow(subWindows.value(SUBWIN_PZEDIT_new));
        adjustEditMenus();
        return;
    }

    //检测当前用户权限，以决定是否允许打开此窗口，是以只读方式还是以可编辑方式打开



    QByteArray* sinfo;
    SubWindowDim* winfo;
    dbUtil->getSubWinInfo(SUBWIN_PZEDIT_new,winfo,sinfo);
    PzDialog* dlg = new PzDialog(cursm,curSuiteMgr,sinfo,this);
    dlg->setWindowTitle(tr("凭证窗口（新）"));
    connect(dlg,SIGNAL(showMessage(QString,AppErrorLevel)),this,SLOT(showRuntimeInfo(QString,AppErrorLevel)));
    connect(dlg,SIGNAL(selectedBaChanged(QList<int>,bool)),this,SLOT(baSelectChanged(QList<int>,bool)));
//    connect(dlg, SIGNAL(canMoveUpAction(bool)), this, SLOT(canMoveUp(bool)));
//    connect(dlg, SIGNAL(canMoveDownAction(bool)), this, SLOT(canMoveDown(bool)));
//    connect(dlg, SIGNAL(curPzNumChanged(int)), this, SLOT(curPzNumChanged(int)));
//    connect(dlg, SIGNAL(pzStateChanged(int)), this, SLOT(pzStateChange(int)));
//    connect(dlg, SIGNAL(pzContentChanged(bool)), this, SLOT(pzContentChanged(bool)));
//    connect(dlg, SIGNAL(curIndexChanged(int,int)),this, SLOT(curPzIndexChanged(int,int)));
//    connect(dlg, SIGNAL(selectedBaAction(bool)), this, SLOT(userSelBaAction(bool)));
//    connect(dlg, SIGNAL(saveCompleted()), this, SLOT(pzContentSaved()));
//    connect(dlg, SIGNAL(pzsStateChanged()), this, SLOT(refreshShowPzsState()));
//    connect(dlg,SIGNAL(mustRestat()),this,SLOT(extraChanged()));

    //设置当前打开凭证可选的时间范围
    //QDate start = QDate(cursy,cursm,cursd);
    //QDate end = QDate(cursy,cursm, cured);
    //dlg->setDateRange(start, end);

    //if(curPzSetState == Ps_Jzed)
    //    dlg->setReadOnly(true);


    showSubWindow(SUBWIN_PZEDIT_new,winfo,dlg);
    if(winfo)
        delete winfo;
    if(sinfo)
        delete sinfo;
    //rfTbrVisble();
    //refreshActEnanble();
    adjustEditMenus();


    //    QRect rect;
    //    PzDialog::StateInfo states;
    //    appSetting.readPzEditWindowState(rect,states);
    //    pd = new PzDialog(cy,cm,curPzSet,states,this);
    //    PAMdiSubWindow* sw = new PAMdiSubWindow(SWT_PZEDIT, ui->mdiArea);
    //    sw->setWidget(pd);
    //    sw->setAttribute(Qt::WA_DeleteOnClose);
    //    connect(sw,SIGNAL(windowClosed(SubWindowType,QMdiSubWindow*)),
    //            this,SLOT(onSubWindowClosed(SubWindowType,QMdiSubWindow*)));
    //    sw->resize(rect.width(),rect.height());
    //    pd->moveToFirst();
    //    sw->show();

}

//自动分配凭证号
void MainWindow::autoAssignPzNum()
{

}

//手动指定凭证号
void MainWindow::handAssignPzNum(){

}

///////////////////////数据菜单处理槽部分/////////////////////////////////////
//显示科目余额
void MainWindow::viewSubjectExtra()
{
    LookupSubjectExtraDialog* dlg;
    if(subWindows.contains(SUBWIN_LOOKUPSUBEXTRA)){
        dlg = static_cast<LookupSubjectExtraDialog*>(subWindows.value(SUBWIN_LOOKUPSUBEXTRA)->widget());
        //dlg->setDate(cursy,cursm);
        //激活此子窗口
        ui->mdiArea->setActiveSubWindow(subWindows.value(SUBWIN_LOOKUPSUBEXTRA));
        return;
    }

    SubWindowDim* winfo;
    QByteArray* sinfo;
    bool exist = dbUtil->getSubWinInfo(SUBWIN_LOOKUPSUBEXTRA,winfo,sinfo);
    dlg = new LookupSubjectExtraDialog(curAccount);

    showSubWindow(SUBWIN_LOOKUPSUBEXTRA,winfo,dlg);
    if(winfo)
        delete winfo;
    if(sinfo)
        delete sinfo;



}

/**
 * @brief MainWindow::openSpecPz
 *  打开指定id值的凭证（接受外部对话框窗口的请求，比如显示明细账或凭证检错对话框）
 *  如果指定了bid，则选中该id代表的会计分录所在行
 * @param pid
 * @param bid
 */
void MainWindow::openSpecPz(int pid,int bid)
{
    //根据凭证是否属于当前月份来决定是否用凭证编辑窗口还是用历史凭证显示窗口打开    
    bool isIn;
    PingZheng* pz = curSuiteMgr->readPz(pid,isIn);
    if(!pz)
        return;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    int month = pz->getDate2().month();
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    if(isIn){
        PzDialog* w = NULL;
        if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZEDIT_new)){
            dbUtil->getSubWinInfo(SUBWIN_PZEDIT_new,winfo,sinfo);
            w = new PzDialog(month,curSuiteMgr,sinfo);
            w->setWindowTitle(tr("凭证窗口（新）"));
            connect(w,SIGNAL(showMessage(QString,AppErrorLevel)),this,SLOT(showRuntimeInfo(QString,AppErrorLevel)));
            connect(w,SIGNAL(selectedBaChanged(QList<int>,bool)),this,SLOT(baSelectChanged(QList<int>,bool)));
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZEDIT_new,w,winfo);
        }
        else{
            w = static_cast<PzDialog*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_PZEDIT_new));
            w->setMonth(month);
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZEDIT_new,NULL,winfo);
        }
        BusiAction* ba = NULL;
        foreach(BusiAction* b, pz->baList()){
            if(b->getId() == bid){
                ba = b;
                break;
            }
        }
        w->seek(pz,ba);
    }
    else{
        //如果按照周全的考虑，应该读取整个月份的凭证，这里为了简化也是为了提高响应速度（因为通常用户只会调取一张凭证）
        historyPzSet[suiteId].clear();
        historyPzSet[suiteId]<<pz;
        historyPzMonth[suiteId] = month;
        historyPzSetIndex[suiteId] = 0;
        HistoryPzForm* w;
        if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_HISTORYVIEW)){
            dbUtil->getSubWinInfo(SUBWIN_HISTORYVIEW,winfo,sinfo);
            w = new HistoryPzForm(pz,sinfo);
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_HISTORYVIEW,w,winfo);
        }
        else{
            w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
            w->setPz(pz);
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_HISTORYVIEW,NULL,NULL);
        }
        w->setCurBa(bid);
        w->setWindowTitle(tr("历史凭证（%1年%2月）").arg(curSuiteMgr->year()).arg(month));
        if(sinfo)
            delete sinfo;
        if(winfo)
            delete winfo;
    }
        //        HistoryPzForm* form;
        //        if(!subWindows.contains(SUBWIN_HISTORYVIEW)){
        //            QByteArray* sinfo;
        //            SubWindowDim* winfo;
        //            dbUtil->getSubWinInfo(SUBWIN_HISTORYVIEW,winfo,sinfo);
        //            form = new HistoryPzForm(pz,sinfo);
        //            showSubWindow(SUBWIN_HISTORYVIEW,winfo,form);
        //            if(sinfo)
        //                delete sinfo;
        //            if(winfo)
        //                delete winfo;
        //        }
        //        else{
        //            form  = static_cast<HistoryPzForm*>(subWindows.value(SUBWIN_HISTORYVIEW)->widget());
        //            form->setPz(pz);
        //        }
        //        form->setCurBa(bid);

}

//生成利润表
void MainWindow::generateIncomeStatements()
{
    ReportDialog* dlg = new ReportDialog;
    dlg->setReportType(2);
    dlg->setWindowTitle(tr("利润表"));
    ui->mdiArea->addSubWindow(dlg);
    dlg->show();
}

//生成资产负债表
void MainWindow::generateBalanceSheet()
{

}


///////////////////////////工具菜单处理槽部分//////////////////////////////////
//设置账户信息
void MainWindow::setupAccInfos()
{
//    CreateAccountDialog* dlg = new CreateAccountDialog(false, this);
//    dlg->setWindowTitle(tr("设置账户信息"));
//    ui->mdiArea->addSubWindow(dlg);
//    dlg->show();
}

//导入基本数据
bool MainWindow::impBasicDatas()
{
//    BasicDataDialog* dlg = new BasicDataDialog(false);
//    dlg->setWindowTitle(tr("导入基础数据"));
//    ui->mdiArea->addSubWindow(dlg);
//    dlg->exec();
}

//设置记账基点
void MainWindow::setupBase()
{
    showSubWindow(SUBWIN_SETUPBASE);
}

//设置开户银行账户信息
void MainWindow::setupBankInfos()
{
    //showSubWindow(SETUPBANK);
}



///////////////////////选项菜单处理槽部分//////////////////////////////////////////
void MainWindow::subjectConfig()
{
    SubjectConfigDialog* subConfigDlg = new SubjectConfigDialog(this);
    subConfigDlg->setWindowTitle(tr("科目配置"));
    ui->mdiArea->addSubWindow(subConfigDlg);
    subConfigDlg->show();
}

////////////////////////帮助菜单处理槽部分////////////////////////////////////////
void MainWindow::about()
{
    //QMessageBox::about(this, tr("版权声明"), aboutStr);
        AboutForm* form = new AboutForm(aboutStr);
        form->show();
}



//处理向导式的新账户创建的4个步骤
void MainWindow::toCrtAccNextStep(int curStep, int nextStep)
{
    //表现不理想，暂且不用
//    if((curStep == 1) && (nextStep == 2)){ //结束账户一般信息的输入
//        QString code = dlgAcc->getCode();
//        QString sname = dlgAcc->getSName();
//        QString lname = dlgAcc->getLName();
//        QString filename = dlgAcc->getFileName();
//        int reportType = dlgAcc->getReportType();

//        //创建该账户对应的数据库，并建立基本数据表和导入基本数据,必要情况下显示创建进度
//        //检查是否存在同名文件，如有，则恢复刚才的对话框的内容允许用户进行修改
//        QString qn = QString("./datas/databases/%1").arg(filename);

//        if(!QFile::exists(qn + ".dat") ||   //如果文件不存在或
//           (QFile::exists(qn + ".dat") &&   //文件存在且要求覆盖
//           (QMessageBox::Yes == QMessageBox::critical(0, qApp->tr("文件名称冲突"),
//                qApp->tr("数据库文件名已存在，要覆盖吗？\n"
//                "文件覆盖后将导致先前文件的数据全部丢失"),
//                QMessageBox::Yes | QMessageBox::No)))){
//            //创建数据库文件并打开连接
//            ConnectionManager::openConnection(filename);
//            adb = ConnectionManager::getConnect();
//            createBasicTable(); //创建基本表

//            //将账户信息添加到数据库文件中
//            QSqlQuery query;
//            query.prepare("INSERT INTO AccountInfos(code, sname, lname) "
//                          "VALUES(:code, :sname, :lname)");
//            query.bindValue(":code", code);
//            query.bindValue(":sname", sname);
//            query.bindValue(":lname", lname);
//            query.exec();

//            //将该账户添加到应用程序的配置信息中（帐户简称，数据库文件名对）并将该账户设置为当前账户
//            AppConfig* appSetting = AppConfig::getInstance();
//            curAccountId = appSetting->addAccountInfo(code, sname, lname, filename);
//            appSetting->setUsedReportType(curAccountId, reportType);

//            appSetting->setRecentOpenAccount(curAccountId);
//            setWindowTitle(tr("会计凭证处理系统---") + lname);
//            //enaActOnAccOpened(true);
//            refreshTbrVisble();
//            refreshActEnanble();
//            dlgAcc->close();

//            dlgBank = new SetupBankDialog;
//            connect(dlgBank, SIGNAL(toNextStep(int,int)), this, SLOT(toCrtAccNextStep(int,int)));
//            dlgBank->show();
//        }
//    }
//    else if((curStep == 2) && (nextStep == 3)){
//        dlgBank->close();
//        dlgData = new BasicDataDialog;
//        connect(dlgData, SIGNAL(toNextStep(int,int)), this, SLOT(toCrtAccNextStep(int,int)));
//        dlgData->show();
//    }
//    else if((curStep == 3) && (nextStep == 4)){
//        dlgData->close();
//        dlgBase = new SetupBaseDialog;
//        connect(dlgBase, SIGNAL(toNextStep(int,int)), this, SLOT(toCrtAccNextStep(int,int)));
//        dlgBase->show();
//    }
//    else if((curStep == 4) && (nextStep == 4)){
//        dlgBase->close();

//        delete dlgAcc;
//        delete dlgBank;
//        delete dlgData;
//        delete dlgBase;
//    }
//    else if(nextStep == 0){
//        delete dlgAcc;
//        delete dlgBank;
//        delete dlgData;
//        delete dlgBase;
//    }
}

//刷新工具条的可见性
void MainWindow::rfTbrVisble()
{
    bool isPzEdit = subWindows.contains(SUBWIN_PZEDIT);
    if(curUser == NULL){
        ui->tbrMain->setVisible(false);
        ui->tbrPzs->setVisible(false);
        ui->tbrPzEdit->setVisible(false);
        return;
    }
    //如果当前登录用户有账户生命期管理权限，则显示与此相关的工具条
    if(curUser->haveRight(allRights.value(Right::CreateAccount)) ||
       curUser->haveRight(allRights.value(Right::OpenAccount)) ||
       curUser->haveRight(allRights.value(Right::CloseAccount)) ||
       curUser->haveRight(allRights.value(Right::DelAccount)) ||
       curUser->haveRight(allRights.value(Right::RefreshAccount)) ||
       curUser->haveRight(allRights.value(Right::ImportAccount)) ||
       curUser->haveRight(allRights.value(Right::ExportAccount)))
        ui->tbrMain->setVisible(true);
    else
        ui->tbrMain->setVisible(false);

    if(curAccount){ //如果账户已经打开
        ui->tbrPzs->setVisible(true);
    }
    else{
        ui->tbrPzs->setVisible(false);
    }

//    if(isPzEdit && subWinActStates.value(PZEDIT)){ //凭证编辑窗口已经打开并处于激活状态
//        ui->tbrPzEdit->setVisible(true);
//        ui->tbrAdvanced->setVisible(true);
//    }
//    else{
//        ui->tbrPzEdit->setVisible(false);
//        ui->tbrAdvanced->setVisible(false);
//    }

    //新凭证编辑窗口有关
    bool r = (subWindows.contains(SUBWIN_PZEDIT_new) ||subWindows.contains(SUBWIN_PZEDIT)) &&
            (subWinActStates.value(SUBWIN_PZEDIT_new) || subWinActStates.value(SUBWIN_PZEDIT));
    ui->tbrPzEdit->setVisible(r);
    ui->tbrAdvanced->setVisible(r);
}

//刷新Action的可用性
void MainWindow::refreshActEnanble()
{
    //    bool isPzEdit = subWindows.contains(PZEDIT);
    //    bool isPzEditNew = subWindows.contains(PZEDIT_new);

    //    //登录动作管理
    //    bool r = (curUser == NULL);
    //    ui->actLogin->setEnabled(r);
    //    ui->actLogout->setEnabled(!r);
    //    ui->actShiftUser->setEnabled(!r);
    //    if(r)
    //        return;

    //    //账户生命期管理动作
    //    if(curUser == NULL){
    //        ui->actCrtAccount->setEnabled(false);
    //        ui->actCloseAccount->setEnabled(false);
    //        ui->actOpenAccount->setEnabled(false);
    //        //ui->actDelAcc->setEnabled(false);
    //        ui->actRefreshActInfo->setEnabled(false);
    //        ui->actImpActFromFile->setEnabled(false);
    //        ui->actEmpActToFile->setEnabled(false);
    //        ui->actAccProperty->setEnabled(false);
    //    }
    //    else{
    //        r = curUser->haveRight(allRights.value(Right::CreateAccount)) && (curAccountId == 0);
    //        ui->actCrtAccount->setEnabled(r);
    //        r = curUser->haveRight(allRights.value(Right::CloseAccount)) &&
    //                (curAccountId != 0);
    //        ui->actCloseAccount->setEnabled(r);
    //        r = curUser->haveRight(allRights.value(Right::OpenAccount)) && (curAccountId == 0);
    //        ui->actOpenAccount->setEnabled(r);
    //        //r = curUser->haveRight(allRights.value(Right::DelAccount)) &&
    //        //        (curAccount != 0);
    //        //ui->actDelAcc->setEnabled(r);
    //        r = curUser->haveRight(allRights.value(Right::RefreshAccount)) && (curAccountId == 0);
    //        ui->actRefreshActInfo->setEnabled(r);
    //        r = curUser->haveRight(allRights.value(Right::ImportAccount)) && (curAccountId == 0);
    //        ui->actImpActFromFile->setEnabled(r);
    //        r = curUser->haveRight(allRights.value(Right::ExportAccount)) && (curAccountId == 0);
    //        ui->actEmpActToFile->setEnabled(r);
    //        ui->actAccProperty->setEnabled(curAccountId != 0);
    //    }


    //    //凭证集操作动作
    //    if(curUser == NULL){
    //        ui->actOpenPzs->setEnabled(false);
    //        ui->actClosePzs->setEnabled(false);
    //        ui->actEditPzs->setEnabled(false);
    //        ui->mnuAdvOper->setEnabled(false);
    //        ui->mnuEndProcess->setEnabled(false);
    //    }
    //    else{
    //        r = (curAccountId != 0) && !pzSetMgr->isOpened() &&
    //                curUser->haveRight(allRights.value(Right::OpenPzs));
    //        ui->actOpenPzs->setEnabled(r);
    //        r = curUser->haveRight(allRights.value(Right::ClosePzs)) && pzSetMgr->isOpened();
    //        ui->actClosePzs->setEnabled(r);

    //        r = (curUser->haveRight(allRights.value(Right::AddPz)) ||
    //            curUser->haveRight(allRights.value(Right::DelPz))  ||
    //            curUser->haveRight(allRights.value(Right::EditPz)) ||
    //            curUser->haveRight(allRights.value(Right::ViewPz))) && pzSetMgr->isOpened();
    //        ui->actEditPzs->setEnabled(r);

    //        //检测用户的高级凭证操作权限，并视情启用菜单（未来实现！！）
    //        ui->mnuAdvOper->setEnabled(pzSetMgr->isOpened());
    //        ui->mnuEndProcess->setEnabled(pzSetMgr->isOpened());
    //    }

    //    //期末处理
    //    ui->actImpOtherPz->setEnabled(false);    //引入其他凭证
    //    r = ((curPzSetState == Ps_AllVerified) && !ui->actSave->isEnabled());
    //    ui->actFordEx->setEnabled(r);   //结转汇兑损益
    //    ui->actFordPl->setEnabled(r);   //结转损益
    //    ui->actJzbnlr->setEnabled(r);   //结转本年利润
    //    ui->actEndAcc->setEnabled(r && isExtraVolid); //结转（全部审核了且余额是有效的）
    //    ui->actAntiJz->setEnabled(curPzSetState != Ps_Jzed);   //反结转（未结账便可以）
    //    //反引入
    //    //r = (curPzSetState >= Ps_ImpOther) && (curPzSetState <= Ps_JzsyPre);
    //    ui->actAntiImp->setEnabled(false);  //当前因为还未实现，所有先禁用
    //    ui->actAntiEndAcc->setEnabled(curPzSetState == Ps_Jzed);    //反结账


    //    //高级凭证操作
    //    if(curUser == NULL){
    //        ui->actVerifyPz->setEnabled(false);
    //        ui->actInStatPz->setEnabled(false);
    //        ui->actAntiVerify->setEnabled(false);
    //        ui->actRepealPz->setEnabled(false);
    //        ui->actAntiRepeat->setEnabled(false);
    //        ui->actAllVerify->setEnabled(false);
    //        ui->actAllInstat->setEnabled(false);
    //    }
    //    else{
    //        if(curAccount && curAccount->getReadOnly()){
    //            ui->mnuAdvOper->setEnabled(false);
    //            ui->mnuEndProcess->setEnabled(false);

    //            ui->actSubConfig->setEnabled(false);
    //        }
    //        else{
    //            //审核凭证
    //            r = curUser->haveRight(allRights.value(Right::VerifyPz)) &&
    //                (curPzState == Pzs_Recording);
    //            ui->actVerifyPz->setEnabled(r);
    //            //凭证入账
    //            r = curUser->haveRight(allRights.value(Right::InstatPz)) &&
    //                (curPzState == Pzs_Verify);
    //            ui->actInStatPz->setEnabled(r);
    //            //取消审核
    //            r = curUser->haveRight(allRights.value(Right::VerifyPz)) &&
    //                    ((curPzState == Pzs_Verify) || (curPzState == Pzs_Instat));
    //            ui->actAntiVerify->setEnabled(r);
    //            //作废
    //            r = curUser->haveRight(allRights.value(Right::RepealPz)) &&
    //                (curPzState == Pzs_Recording);
    //            ui->actRepealPz->setEnabled(r);
    //            //取消作废
    //            r = curUser->haveRight(allRights.value(Right::RepealPz)) &&
    //                (curPzState == Pzs_Repeal);
    //            ui->actAntiRepeat->setEnabled(r);
    //            //全部审核（或许还应参考凭证集状态）
    //            r = curUser->haveRight(allRights.value(Right::VerifyPz));
    //            ui->actAllVerify->setEnabled(r);
    //            //全部入账
    //            r = curUser->haveRight(allRights.value(Right::InstatPz));
    //            ui->actAllInstat->setEnabled(r);
    //        }

    //    }

    //    //一般凭证操作动作
    //    if(curUser == NULL){
    //        ui->actAddPz->setEnabled(false);
    //        ui->actDelPz->setEnabled(false);
    //    }
    //    else{
    //        if(curAccount && curAccount->getReadOnly()){
    //            ui->actAddPz->setEnabled(false);
    //            ui->actInsertPz->setEnabled(false);
    //            ui->actSave->setEnabled(false);
    //            ui->actDelPz->setEnabled(false);
    //            ui->actAddAction->setEnabled(false);
    //            ui->actDelAction->setEnabled(false);
    //            ui->actMvDownAction->setEnabled(false);
    //            ui->actMvUpAction->setEnabled(false);
    //            ui->actRepealPz->setEnabled(false);
    //            ui->actVerifyPz->setEnabled(false);
    //            ui->actInStatPz->setEnabled(false);
    //            ui->actAllInstat->setEnabled(false);
    //            ui->actAllVerify->setEnabled(false);
    //            ui->actReassignPzNum->setEnabled(false);
    //        }
    //        else{
    //            //添加、插入凭证（这些操作都是针对手工录入凭证）
    //            r = curUser->haveRight(allRights.value(Right::AddPz)) && (isPzEdit||isPzEditNew)
    //                    && (curPzSetState != Ps_Jzed);
    //            ui->actAddPz->setEnabled(r);
    //            ui->actInsertPz->setEnabled(r);
    //            //删除凭证
    //            //r = curUser->haveRight(allRights.value(Right::DelPz)) && (isPzEdit||isPzEditNew)
    //            //        && (curPzModel->rowCount() > 0) && (curPzState != Ps_Jzed);
    //            ui->actDelPz->setEnabled(r);
    //        }

    //    }

    //    //打印动作
    //    if(curUser == NULL){
    //        ui->actPrint->setEnabled(false);
    //        ui->actPrintAdd->setEnabled(false);
    //        ui->actPrintDel->setEnabled(false);
    //    }
    //    else{
    //        r = (curUser->haveRight(allRights.value(Right::PrintPz)) ||
    //            curUser->haveRight(allRights.value(Right::PrintDetialTable)) ||
    //            curUser->haveRight(allRights.value(Right::PrintStatTable))) &&
    //                pzSetMgr->isOpened();
    //        ui->actPrint->setEnabled(r);

    //        r = (isPzEdit||isPzEditNew) && (curPzn != 0) && curUser->haveRight(allRights.value(Right::PrintPz));
    //        ui->actPrintAdd->setEnabled(r);
    //        ui->actPrintDel->setEnabled(r);
    //    }

    //    if(curUser == NULL){
    //        ui->actReassignPzNum->setEnabled(false);
    //        ui->actSave->setEnabled(false);
    //        ui->actAddAction->setEnabled(false);
    //    }
    //    else{
    //        //重置凭证号
    //        r = pzSetMgr->isOpened() && curUser->haveRight(allRights.value(Right::EditPz))
    //                && (curPzSetState == Ps_Rec);
    //        ui->actReassignPzNum->setEnabled(r);

    //        //保存动作
    //        PzDialog2* pzEdit;
    //        if(isPzEdit){
    //            pzEdit = qobject_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    //            r = isPzEdit && pzEdit->isDirty();
    //        }
    //        else
    //            r = false;
    //        ui->actSave->setEnabled(r);

    //        //增加业务按钮（还未考虑凭证集状态）
    //        r = (curPzState == Pzs_Repeal) || (curPzState == Pzs_Recording);
    //        ui->actAddAction->setEnabled(r);
    //    }



    //    //数据菜单处理
    //    if(curUser == NULL){
    //        ui->actSubExtra->setEnabled(false);
    //        //ui->actCollCal->setEnabled(false);  //该菜单项已弃用
    //        ui->actCurStat->setEnabled(false);
    //        ui->actShowDetail->setEnabled(false);
    //        ui->actShowTotal->setEnabled(false);
    //        //ui->mnuAccBook->setEnabled(false);
    //        //ui->mnuReport->setEnabled(false);
    //    }
    //    else{
    //        ui->actSubExtra->setEnabled(pzSetMgr->isOpened());
    //        r = pzSetMgr->isOpened() && !ui->actSave->isEnabled();
    //        ui->actCurStat->setEnabled(r);      //本期统计
    //        ui->actShowDetail->setEnabled(r);   //显示明细账
    //        ui->actShowTotal->setEnabled(r);    //显示总账
    //        //ui->mnuAccBook->setEnabled(isOpenPzSet);
    //        //ui->mnuReport->setEnabled(isOpenPzSet);
    //    }

    //    //工具菜单处理
    //    if(curUser == NULL){
    //        ui->actSetupAccInfos->setEnabled(false);
    //        ui->actSetupBankInfos->setEnabled(false);
    //        ui->actSetupBase->setEnabled(false);
    //        ui->actSetupBankInfos->setEnabled(false);
    //        ui->actSqlTool->setEnabled(false);
    //        ui->actSecCon->setEnabled(false);

    //    }
    //    else{
    //        if(curAccount && curAccount->getReadOnly()){
    //            //ui->mnuHandyTools->setEnabled(false);
    //            //ui->mnuAdminTools->setEnabled(false);
    //            ui->actSetupAccInfos->setEnabled(false);
    //            ui->actSetupBase->setEnabled(false);
    //            ui->actSetupBankInfos->setEnabled(false);
    //            ui->actGdzcAdmin->setEnabled(false);
    //            ui->actDtfyAdmin->setEnabled(false);
    //        }
    //        else{
    //            //这里进行了简化，其实应该考虑操作权限
    //            r = (curAccountId != 0);
    //            ui->actSetupAccInfos->setEnabled(r);
    //            ui->actSetupBankInfos->setEnabled(r);
    //            ui->actSetupBase->setEnabled(r);
    //            ui->actSetupBankInfos->setEnabled(r);
    //            ui->actSqlTool->setEnabled(r);
    //            ui->actSecCon->setEnabled(r);
    //        }
    //    }
    ////ui->actGoFirst
    //    //选项菜单处理
    //    if(curUser == NULL){
    //        ui->actSubConfig->setEnabled(false);
    //    }
    //    else{
    //        if(curAccount && !curAccount->getReadOnly())
    //            ui->actSubConfig->setEnabled(true);
    //        else
    //            ui->actSubConfig->setEnabled(false);
    //    }

    //    //凭证编辑动作


    //    //r = r && (((curPzSetState == PS_ORI) && (curPzState == PZS_RECORDING))
    //    //          ||(curPzSetState == PS_ORI && curPzModel->rowCount() == 0));


    //    //删除凭证


    ////
    ////    r = curUser->haveRight(allRights.value(Right::EditPz)) && dlgEditPz;
    ////    ui->actAddAction->setEnabled(r);

    ////    //ui->actDelAction->setEnabled(isEnabled);
    ////    ui->actSavePz->setVisible(isEnabled);

    //    //r = (dlgPzEdit != NULL);
    //    //ui->actClosePz->setEnabled(r);



}


//刷新高级凭证操作按钮的启用状态(应综合考虑用户权限和凭证的当前状态)
void MainWindow::refreshAdvancPzOperBtn()
{
//    bool r;
//    r = curUser->haveRight(allRights.value(Right::RepealPz));
//    ui->actRepealPz->setEnabled(r);
//    rdoRepealPz->setEnabled(r);
//    r = curUser->haveRight(allRights.value(Right::VerifyPz));
//    r = r && pzSetMgr && (pzSetMgr->getCurPz()->getPzState() == Pzs_Recording);
//    ui->actVerifyPz->setEnabled(r);
//    rdoVerifyPz->setEnabled(r);
//    r = curUser->haveRight(allRights.value(Right::InstatPz));
//    r = r && pzSetMgr && (pzSetMgr->getCurPz()->getPzState() == Pzs_Verify);
//    ui->actInStatPz->setEnabled(r);
//    rdoInstatPz->setEnabled(r);

}

//已废弃
//启用在用户登录后的Action(参数isEnabled，ture：表示登录，false：表示登出)
void MainWindow::enaActOnLogin(bool isEnabled)
{
    if(!isEnabled){  //登出
        ui->actLogin->setEnabled(true);
        ui->actLogout->setEnabled(false);
        ui->actShiftUser->setEnabled(false);
        ui->tbrMain->setVisible(false);
        ui->tbrPzEdit->setVisible(false);
        ui->tbrPzs->setVisible(false);
    }
    else{
        ui->actLogin->setEnabled(false);
        ui->actLogout->setEnabled(true);
        ui->actShiftUser->setEnabled(true);
        //检测当前登录用户是否具有打开、关闭账户的权限，并启用或禁用相应菜单项

    }
}

//凭证状态改变处理槽（用以在用户通过凭证编辑窗口导航凭证时正确显示和设置凭证的当前状态）
void MainWindow::pzStateChange(int scode)
{
//    curPzState = scode;
//    switch(scode){
//    case Pzs_Recording:   //录入态
//        rdoRecording->setChecked(true);
//        ui->actRepealPz->setChecked(false);
//        ui->actVerifyPz->setChecked(false);
//        ui->actInStatPz->setChecked(false);
//        break;
//    case Pzs_Repeal:      //作废
//        rdoRepealPz->setChecked(true);
//        ui->actRepealPz->setChecked(true);
//        ui->actVerifyPz->setChecked(false);
//        ui->actInStatPz->setChecked(false);
//        break;
//    case Pzs_Verify:    //已审核
//        rdoVerifyPz->setChecked(true);
//        ui->actVerifyPz->setChecked(true);
//        ui->actRepealPz->setChecked(false);
//        ui->actInStatPz->setChecked(false);
//        break;
//    case Pzs_Instat:      //已记账
//        rdoInstatPz->setChecked(true);
//        ui->actInStatPz->setChecked(true);
//        ui->actRepealPz->setChecked(false);
//        ui->actVerifyPz->setChecked(false);
//        break;
//    }
//    //refreshActEnanble();
//    refreshAdvancPzOperBtn();
}

//当前凭证的内容发生了改变
//void MainWindow::pzContentChanged(bool isChanged)
//{
//    ui->actSave->setEnabled(isChanged);
//}

//凭证内容已经保存
//void MainWindow::pzContentSaved()
//{
//    ui->actSave->setEnabled(false);
//}

//用户通过工具条上改变了当前凭证的状态，通过此函数告诉凭证编辑窗口，使它能保存
void MainWindow::userModifyPzState(bool checked)
{
//    QRadioButton* send = qobject_cast<QRadioButton*>(sender());
//    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
//    PzState old = (PzState)pzEdit->getPzState();
//    switch(old){
//    case Pzs_Repeal:
//        pzRepeal--;
//        break;
//    case Pzs_Recording:
//        pzRecording--;
//        break;
//    case Pzs_Verify:
//        pzVerify--;
//        break;
//    case Pzs_Instat:
//        pzInstat--;
//        break;
//    }

//    if(send == rdoVerifyPz){
//        curPzState = Pzs_Verify;
//        pzVerify++;
//        pzEdit->setPzState(Pzs_Verify,curUser);
//    }
//    else if(send == rdoInstatPz){
//        curPzState = Pzs_Instat;
//        pzInstat++;
//        pzEdit->setPzState(Pzs_Instat,curUser);
//    }
//    else if(send == rdoRepealPz){
//        curPzState = Pzs_Repeal;
//        pzRepeal++;
//        pzEdit->setPzState(Pzs_Repeal,curUser);
//    }
//    else{
//        curPzState = Pzs_Recording;
//        pzRecording++;
//        pzEdit->setPzState(Pzs_Recording,curUser);
//    }
//    refreshShowPzsState();
//    refreshAdvancPzOperBtn();
//    ui->actSavePz->setEnabled(true);
//    isExtraVolid = false;
}

//根据用户所选的业务活动的状况决定移动和删除业务活动按钮的启用状态
void MainWindow::userSelBaAction(bool isSel)
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(SUBWIN_PZEDIT)->widget());
    if(!isSel){
        ui->actMvUpAction->setEnabled(false);
        ui->actMvDownAction->setEnabled(false);
    }
    else{
        bool r = pzEdit->canMoveBaUp() && (curPzState == Pzs_Recording)
                && curUser->haveRight(allRights.value(Right::EditPz));
        ui->actMvUpAction->setEnabled(r);
        r = pzEdit->canMoveBaDown() && (curPzState == Pzs_Recording)
                && curUser->haveRight(allRights.value(Right::EditPz));
        ui->actMvDownAction->setEnabled(r);
    }
    ui->actDelAction->setEnabled(isSel && (curPzState == Pzs_Recording));
}

void MainWindow::showTemInfo(QString info)
{
    ui->statusbar->showMessage(info, timeoutOfTemInfo);
}

/**
 * @brief MainWindow::showRuntimeInfo
 *  显示应用运行期间的信息（正常、警告、错误等级别）
 * @param info
 * @param level
 */
void MainWindow::showRuntimeInfo(QString info, AppErrorLevel level)
{
    //基本的要求是根据错误级别，用不同的颜色来显示信息
    QString s;
    switch(level){
    case AE_OK:
        s = info;
        break;
    case AE_WARNING:
        s = tr("警告--%1").arg(info);
        break;
    case AE_CRITICAL:
        s = tr("一般性错误--%1").arg(info);
        break;
    case AE_ERROR:
        s = tr("致命性错误--%1").arg(info);
        break;
    }
    ui->statusbar->showRuntimeMessage(s,level);
}

//void MainWindow::canUndoChanged(bool canUndo)
//{
//    LOG_INFO(QString("enter canUndoChanged(%1)").arg(canUndo));
//    undoAction->setEnabled(canUndo);
//}


void MainWindow::undoCleanChanged(bool clean)
{
    //    if(clean)
    //        LOG_INFO("enter clean state!");
    //    else
    //        LOG_INFO("leave clean state!");
    //如果当前激活的子窗口是凭证编辑窗口，则
    if(ui->mdiArea->activeSubWindow() == subWindows.value(SUBWIN_PZEDIT_new))
        ui->actSave->setEnabled(!clean);
}

/**
 * @brief MainWindow::UndoIndexChanged
 *  起点的空白位置索引是0，第一次修改命令的索引是1
 * @param idx
 */
void MainWindow::UndoIndexChanged(int idx)
{
    //LOG_INFO(QString("undoStack index change to %1,stact size is %2")
    //         .arg(idx).arg(undoStack->count()));

    static int oriSize = 0;    //undo栈的原始尺寸
    int newSize = undoStack->count();

    //这种情况是在凭证集没有进行undo操作，由用户在凭证编辑界面直接修改引起
    //或在进行undo操作后，用户要在编辑界面进行了修改，这样undo栈的尺寸就要增长或剪短
    //这不需要更新界面
    if(newSize != oriSize){
        oriSize = newSize;
        //LOG_INFO(QString("oriSize change to %1").arg(oriSize));
        return;
    }
    if(subWindows.contains(SUBWIN_PZEDIT_new)){
        PzDialog* pd = qobject_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget());
        pd->updateContent();
    }




    //用户在凭证编辑窗口的编辑动作，一方面界面的内容已经更新，这时会产生一个编辑命令并
    //压入undo栈，从而引起栈索引的改变，进而会调用凭证编辑窗口的updateContent()方法
    //这有多余之嫌。而正常的undo或rudo操作引起的索引改变，必须调用updateContent()
    //方法来是凭证编辑界面及时更新

    //利用QUndoStack::command(int index)方法，可以获取当前应用的命令，这样可以缩小
    //凭证内容的更新范围，如果凭证内容的更新比较耗时，则可以考虑
}

void MainWindow::redoTextChanged(const QString &redoText)
{
    //LOG_INFO(QString("redo text  change to %1").arg(redoText));
}

void MainWindow::undoTextChanged(const QString &undoText)
{
    //LOG_INFO(QString("undo text  change to %1").arg(undoText));
}

/**
 * @brief MainWindow::showAndHideToolView
 *  显示或隐藏工具指定类型的视图
 * @param vtype
 */
void MainWindow::showAndHideToolView(int vtype)
{
    ToolViewType t = (ToolViewType)vtype;
    switch(t){
    case TV_UNDO:
        initUndoView();
        break;
    case TV_SEARCHCLIENT:
        initSearchClientToolView();
        break;
    }
}

/**
 * @brief MainWindow::DockWindowVisibilityChanged
 *  当Dock窗口的可见性改变时，调整对应菜单项的选取性
 * @param visible
 */
void MainWindow::DockWindowVisibilityChanged(bool visible)
{
    QDockWidget* dw = qobject_cast<QDockWidget*>(sender());
    ToolViewType t;
    QHashIterator<ToolViewType,QDockWidget*> it(dockWindows);
    while(it.hasNext()){
        it.next();
        if(dw == it.value())
            t = it.key();
    }
    tvActions.value(t)->setChecked(visible);
}

/**
 * @brief MainWindow::pzCountChanged
 *  当凭证数改变时，调整凭证号的选择范围和相应控件的启用性
 * @param count
 */
void MainWindow::pzCountChanged(int count)
{
    ui->actNaviToPz->setEnabled(count != 0);
    spnNaviTo->setEnabled(count != 0);
    spnNaviTo->setMinimum(1);
    spnNaviTo->setMaximum(count);
    ui->statusbar->setPzCounts(curSuiteMgr->getRepealCount(),curSuiteMgr->getRecordingCount(),
                               curSuiteMgr->getVerifyCount(),curSuiteMgr->getInstatCount(),
                               curSuiteMgr->getPzCount());
}

/**
 * @brief MainWindow::refreshPzNaveBtnEnable
 *  刷新凭证任务相关Action的可用性（导航按钮，凭证编辑按钮（添加、插入、删除、上移、下移等）
 */
void MainWindow::rfNaveBtn(PingZheng *newPz, PingZheng *oldPz)
{
    subWindowType wtype = activedMdiChild();
    if(!curSuiteMgr || (wtype != SUBWIN_PZEDIT_new && wtype != SUBWIN_HISTORYVIEW)){
        ui->actAddPz->setEnabled(false);
        ui->actInsertPz->setEnabled(false);
        ui->actDelPz->setEnabled(false);
        ui->actGoFirst->setEnabled(false);
        ui->actGoLast->setEnabled(false);
        ui->actGoPrev->setEnabled(false);
        ui->actGoNext->setEnabled(false);
        ui->actMvUpAction->setEnabled(false);
        ui->actMvDownAction->setEnabled(false);
        ui->actAddAction->setEnabled(false);
        ui->actInsertBa->setEnabled(false);
        ui->actDelAction->setEnabled(false);
    }
    else{
        if(wtype == SUBWIN_PZEDIT_new){
            //启用这些按钮的先决条件是凭证集打开且未结账
            ui->actPrint->setEnabled(true);
            bool r = (curSuiteMgr->getState()!=Ps_NoOpen) || (curSuiteMgr->getState()!= Ps_Jzed);
            ui->actAddPz->setEnabled(r);
            ui->actInsertPz->setEnabled(r);
            r = r && curSuiteMgr->getCurPz();
            ui->actDelPz->setEnabled(r);
            bool isEmpty = (curSuiteMgr->getPzCount() == 0);
            bool isFirst = curSuiteMgr->isFirst();
            bool isLast = curSuiteMgr->isLast();
            ui->actGoFirst->setEnabled(!isEmpty && !isFirst);
            ui->actGoLast->setEnabled(!isEmpty && !isLast);
            ui->actGoNext->setEnabled(!(isEmpty || isLast));
            ui->actGoPrev->setEnabled(!(isEmpty || isFirst));
            //根据当前的分录选择情况，控制移动分录按钮的可用性
            QList<int> rows; bool conti;
            //PzDialog* pd = static_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget());
            PzDialog* pd = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
            if(pd){
                pd->getBaSelectedCase(rows,conti);
                baSelectChanged(rows,conti);
            }
        }
        else{
            int suiteId = curSuiteMgr->getSuiteRecord()->id;
            bool isEmpty = historyPzSet.value(suiteId).isEmpty();
            bool isFirst = historyPzSetIndex.value(suiteId) == 0;
            bool isLast = (historyPzSetIndex.value(suiteId) == historyPzSet.value(suiteId).count()-1);
            ui->actGoFirst->setEnabled(!isEmpty && !isFirst);
            ui->actGoLast->setEnabled(!isEmpty && !isLast);
            ui->actGoNext->setEnabled(!(isEmpty || isLast));
            ui->actGoPrev->setEnabled(!(isEmpty || isFirst));
        }

    }
    if(oldPz)
        disconnect(oldPz,SIGNAL(indexBoundaryChanged(bool,bool)),this,SLOT(baIndexBoundaryChanged(bool,bool)));
    if(newPz)
        connect(newPz,SIGNAL(indexBoundaryChanged(bool,bool)),this,SLOT(baIndexBoundaryChanged(bool,bool)));
}

/**
 * @brief MainWindow::baBoundaryChanged
 *  当前凭证的当前选择的会计分录的索引位置改变有必要调整编辑分录的按钮的可用性
 * @param first
 * @param last
 */
void MainWindow::baIndexBoundaryChanged(bool first, bool last)
{
    if(!first && !last){  //当前分录处于中间位置
        ui->actMvUpAction->setEnabled(true);
        ui->actMvDownAction->setEnabled(true);
    }
    else if(first && last){ //分录为空或只有一条分录
        ui->actMvUpAction->setEnabled(false);
        ui->actMvDownAction->setEnabled(false);
    }
    else{
        ui->actMvUpAction->setEnabled(!first);
        ui->actMvDownAction->setEnabled(!last);
    }
}

/**
 * @brief MainWindow::baSelectChanged
 * @param rows      选择的分录对应的行号
 * @param conti     选择的分录是否连续
 */
void MainWindow::baSelectChanged(QList<int> rows, bool conti)
{
    if(!curSuiteMgr->getCurPz()){
        ui->actMvUpAction->setEnabled(false);
        ui->actMvDownAction->setEnabled(false);
        return;
    }
    bool first,last;
    if(rows.isEmpty() || !conti || (rows.count()==curSuiteMgr->getCurPz()->baCount())){
        first=true;last=true;
    }
    else{
        first = (rows.first() == 0);
        last = (rows.last() == curSuiteMgr->getCurPz()->baCount()-1);
    }

    if(!first && !last){  //当前分录处于中间位置
        ui->actMvUpAction->setEnabled(true);
        ui->actMvDownAction->setEnabled(true);
    }
    else if(first && last){ //分录为空或只有一条分录
        ui->actMvUpAction->setEnabled(false);
        ui->actMvDownAction->setEnabled(false);
    }
    else{
        ui->actMvUpAction->setEnabled(!first);
        ui->actMvDownAction->setEnabled(!last);
    }
    bool r = (curSuiteMgr->getState()!=Ps_NoOpen) || (curSuiteMgr->getState()!= Ps_Jzed);
    r = r && (curSuiteMgr->getCurPz()->getPzState() == Pzs_Recording);
    ui->actDelAction->setEnabled(r && !rows.isEmpty());
    ui->actInsertBa->setEnabled(r && (rows.count() == 1));
    ui->actAddAction->setEnabled(r);
}

/**
 * @brief 用户请求切换到新的帐套视图
 * @param previous
 * @param current
 */
void MainWindow::suiteViewSwitched(AccountSuiteManager *previous, AccountSuiteManager *current)
{
    if(previous && subWinGroups.contains(previous->getSuiteRecord()->id))
        subWinGroups.value(previous->getSuiteRecord()->id)->hide();
    if(current){
        int key = current->getSuiteRecord()->id;
        if(!subWinGroups.contains(key)){
            subWinGroups[key] = new SubWinGroupMgr(key,ui->mdiArea);
            //connect(subWinGroups.value(key),SIGNAL(saveSubWinState(subWindowType,QByteArray*,SubWindowDim*)),
            //        this,SLOT(saveSubWindowState(subWindowType,QByteArray*,SubWindowDim*)));
        }
        subWinGroups.value(key)->show();
        //if(commonGroups && commonGroups->isShow())
        //    commonGroups->hide();
    }
    if(curSuiteMgr){
        disconnect(curSuiteMgr,SIGNAL(currentPzChanged(PingZheng*,PingZheng*)),this,SLOT(rfNaveBtn(PingZheng*,PingZheng*)));
        disconnect(curSuiteMgr,SIGNAL(pzSetStateChanged(PzsState)),this,SLOT(pzSetStateChanged(PzsState)));
        disconnect(curSuiteMgr,SIGNAL(pzExtraStateChanged(bool)),this,SLOT(pzSetExtraStateChanged(bool)));
    }
    curSuiteMgr = current;
    connect(curSuiteMgr,SIGNAL(currentPzChanged(PingZheng*,PingZheng*)),this,SLOT(rfNaveBtn(PingZheng*,PingZheng*)));
    connect(curSuiteMgr,SIGNAL(pzSetStateChanged(PzsState)),this,SLOT(pzSetStateChanged(PzsState)));
    connect(curSuiteMgr,SIGNAL(pzExtraStateChanged(bool)),this,SLOT(pzSetExtraStateChanged(bool)));
    if(undoStack){
        disconnect(undoStack,SIGNAL(cleanChanged(bool)),this,SLOT(undoCleanChanged(bool)));
        disconnect(undoStack,SIGNAL(indexChanged(int)),this,SLOT(UndoIndexChanged(int)));
        disconnect(undoStack,SIGNAL(redoTextChanged(QString)),this,SLOT(redoTextChanged(QString)));
        disconnect(undoStack,SIGNAL(undoTextChanged(QString)),this,SLOT(undoTextChanged(QString)));
    }
    undoStack = curSuiteMgr->getUndoStack();
    connect(undoStack,SIGNAL(cleanChanged(bool)),this,SLOT(undoCleanChanged(bool)));
    connect(undoStack,SIGNAL(indexChanged(int)),this,SLOT(UndoIndexChanged(int)));
    connect(undoStack,SIGNAL(redoTextChanged(QString)),this,SLOT(redoTextChanged(QString)));
    connect(undoStack,SIGNAL(undoTextChanged(QString)),this,SLOT(undoTextChanged(QString)));
    if(undoView)
        undoView->setStack(undoStack);
    //调整其他界面元素的可用性等......
}

/**
 * @brief 显示或编辑凭证集
 * @param accSmg
 * @param month
 */
void MainWindow::viewOrEditPzSet(AccountSuiteManager *accSmg, int month)
{
    //如果帐套已关闭，或指定月份已结账，则利用历史凭证显示窗口显示
    int suiteId = accSmg->getSuiteRecord()->id;
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;

    if(accSmg->getSuiteRecord()->isClosed || accSmg->getState(month) == Ps_Jzed){
        historyPzSet[suiteId] = accSmg->getHistoryPzSet(month);
        if(historyPzSet.value(suiteId).isEmpty()){
            QMessageBox::warning(this,tr("提示信息"),tr("没有任何凭证可显示！"));
            historyPzSetIndex[suiteId] = -1;
        }
        else{
            historyPzMonth[suiteId] = month;
            historyPzSetIndex[suiteId] = 0;
            //if(!subWinGroups.contains(suiteId))
            //    subWinGroups[suiteId] = new SubWinGroupMgr(suiteId,ui->mdiArea);
            HistoryPzForm* w;
            if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_HISTORYVIEW)){
                dbUtil->getSubWinInfo(SUBWIN_HISTORYVIEW,winfo,sinfo);
                w = new HistoryPzForm(historyPzSet.value(suiteId).first(),sinfo);
                subWinGroups.value(suiteId)->showSubWindow(SUBWIN_HISTORYVIEW,w,winfo);
            }
            else{
                w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
                w->setPz(historyPzSet.value(suiteId).first());
                subWinGroups.value(suiteId)->showSubWindow(SUBWIN_HISTORYVIEW,NULL,NULL);
            }
            w->setWindowTitle(tr("历史凭证（%1年%2月）").arg(accSmg->year()).arg(month));
        }
    }
    //否则利用凭证编辑窗口显示
    else{
        PzDialog* w = NULL;
        if((curSuiteMgr != accSmg) || ((curSuiteMgr == accSmg) && (curSuiteMgr->month() != month))){
            //因为在调用此函数之前，已经打开了要编辑的凭证集，所有这里可以略去检测代码
            //if(curSuiteMgr->isDirty())
            //    curSuiteMgr->save();
            //if(curSuiteMgr->isOpened())
            //    curSuiteMgr->close();
            curSuiteMgr = accSmg;
            //if(!curSuiteMgr->open(month)){
            //    QMessageBox::critical(this,tr("出错信息"),tr("打开%1年%2月凭证集出错！").arg(curSuiteMgr->year()).arg(month));
            //    return;
            //}
        }
        if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZEDIT_new)){
            dbUtil->getSubWinInfo(SUBWIN_PZEDIT_new,winfo,sinfo);
            PzDialog* w = new PzDialog(month,curSuiteMgr,sinfo);
            w->setWindowTitle(tr("凭证窗口（新）"));
            connect(w,SIGNAL(showMessage(QString,AppErrorLevel)),this,SLOT(showRuntimeInfo(QString,AppErrorLevel)));
            connect(w,SIGNAL(selectedBaChanged(QList<int>,bool)),this,SLOT(baSelectChanged(QList<int>,bool)));
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZEDIT_new,w,winfo);
        }
        else{
            w = static_cast<PzDialog*>(subWinGroups.value(SUBWIN_PZEDIT_new)->getSubWinWidget(SUBWIN_PZEDIT_new));
            w->setMonth(month);
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZEDIT_new,NULL,winfo);
        }
    }
}

/**
 * @brief 凭证集刚被打开，对界面做些调整
 * @param accSmg
 * @param month
 */
void MainWindow::pzSetOpen(AccountSuiteManager *accSmg, int month)
{
    refreshShowPzsState();
    ui->statusbar->setPzSetDate(cursy,cursm);
    rfPzSetAct();
}

/**
 * @brief 凭证集将被关闭，在关闭前，可能需要保存凭证集相关的数据和信息，并调整界面
 * @param accSmg
 * @param month
 */
void MainWindow::prepareClosePzSet(AccountSuiteManager *accSmg, int month)
{
    //在关闭凭证集前，要检测是否有未保存的修改
    if(curSuiteMgr->isDirty()){
        if(QMessageBox::Accepted ==
                QMessageBox::warning(0,tr("提示信息"),tr("凭证集已被改变，要保存吗？"),
                                     QMessageBox::Yes|QMessageBox::No))
            if(!curSuiteMgr->save())
                QMessageBox::critical(0,tr("错误信息"),tr("保存凭证集时发生错误！"));

    }
    //关闭凭证集时，要先关闭所有与该凭证集相关的子窗口
    SubWinGroupMgr* gm = subWinGroups.value(accSmg->getSuiteRecord()->id);
    gm->closeSubWindow(SUBWIN_PZEDIT_new);  //凭证编辑窗口（新）
    gm->closeSubWindow(SUBWIN_PZSTAT2);     //本期统计的窗口（新）
//    if(subWindows.contains(SUBWIN_PZEDIT_new))             //凭证编辑窗口（新）
//        subWindows.value(SUBWIN_PZEDIT_new)->close();
//    if(subWindows.contains(SUBWIN_PZSTAT))                 //显示本期统计的窗口
//        subWindows.value(SUBWIN_PZSTAT)->close();
//    if(subWindows.contains(SUBWIN_PZSTAT2))                //显示本期统计的窗口（新）
//        subWindows.value(SUBWIN_PZSTAT2)->close();
//    if(subWindows.contains(DETAILSVIEW))            //明细账窗口
//        subWindows.value(DETAILSVIEW)->close();
//    if(subWindows.contains(SUBWIN_DETAILSVIEW2))           //明细账窗口（新）
//        subWindows.value(SUBWIN_DETAILSVIEW2)->close();
//    if(subWindows.contains(TOTALDAILY))             //总分类账窗口
//        subWindows.value(TOTALDAILY)->close();
}

/**
 * @brief 凭证集已被关闭，调整界面元素
 * @param accSmg
 * @param month
 */
void MainWindow::pzSetClosed(AccountSuiteManager *accSmg, int month)
{
//    if(undoView)
//        undoView->setStack(NULL);
//    disconnect(undoStack,SIGNAL(cleanChanged(bool)),this,SLOT(undoCleanChanged(bool)));
//    disconnect(undoStack,SIGNAL(indexChanged(int)),this,SLOT(UndoIndexChanged(int)));
//    disconnect(undoStack,SIGNAL(redoTextChanged(QString)),this,SLOT(redoTextChanged(QString)));
//    disconnect(undoStack,SIGNAL(undoTextChanged(QString)),this,SLOT(undoTextChanged(QString)));
//    disconnect(curSuiteMgr,SIGNAL(currentPzChanged(PingZheng*,PingZheng*)),this,SLOT(rfNaveBtn(PingZheng*,PingZheng*)));
//    disconnect(curSuiteMgr,SIGNAL(pzSetStateChanged(PzsState)),this,SLOT(pzSetStateChanged(PzsState)));
//    disconnect(curSuiteMgr,SIGNAL(pzExtraStateChanged(bool)),this,SLOT(pzSetExtraStateChanged(bool)));

    ui->statusbar->setPzSetDate(0,0);
    ui->statusbar->setPzSetState(Ps_NoOpen);
    ui->statusbar->resetPzCounts();

    rfPzSetAct(false);
    //rfAct();
    //rfTbrVisble();
    //refreshActEnanble();
    adjustEditMenus(UT_PZ,true);
}

/**
 * @brief 监视公共子窗口的关闭，以便及时将其从哈希表中删除
 * @param winType
 * @param subWin
 */
void MainWindow::commonSubWindowClosed(MyMdiSubWindow *subWin)
{
    //确定是唯一性还是多个同类型子窗口可以同时存在的子窗口
    QByteArray* state = NULL;
    SubWindowDim* dim = NULL;
    disconnect(subWin,SIGNAL(windowClosed(MyMdiSubWindow*)),this,SLOT(commonSubWindowClosed(MyMdiSubWindow*)));
    subWindowType winType = subWin->getWindowType();
    bool only = isOnlyCommonSubWin(winType);
    if(only){
        dim = new SubWindowDim;
        dim->x=subWin->x();dim->y=subWin->y();dim->w=subWin->width();dim->h=subWin->height();
        if(winType == SUBWIN_ACCOUNTPROPERTY){
            AccountPropertyConfig* w = static_cast<AccountPropertyConfig*>(subWin->widget());
            if(w){
                state = w->getState();
                delete w;
            }
        }
        commonGroups.remove(winType);
    }
    else{
        if(winType == SUBWIN_SQL){
            DatabaseAccessForm* w = static_cast<DatabaseAccessForm*>(subWin->widget());
            if(w)
                delete w;
        }
        commonGroups_multi.remove(winType,subWin);
    }
    if(dim || state)
        dbUtil->saveSubWinInfo(winType,dim,state);
    if(dim)
        delete dim;
    if(state)
        delete state;
}

///**
// * @brief 当子窗口关闭时，保存子窗口的位置、尺寸以及内部状态信息
// * @param state
// * @param dim
// */
//void MainWindow::saveSubWindowState(subWindowType winType, QByteArray *state, SubWindowDim *dim)
//{
//    curAccount->getDbUtil()->saveSubWinInfo(winType,dim,state);
//}


/**
 * @brief MainWindow::PzChangedInSet
 *  由于凭证集内凭证内容的任何改变，将需要启用保存按钮以使用户可以执行保存操作
 *  监视凭证内容的改变，可以通过监视undoStack的干净状态改变信号而得
 */
//void MainWindow::PzChangedInSet()
//{
//    ui->actSave->setEnabled(true);
//}



//处理添加凭证动作事件
void MainWindow::on_actAddPz_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
    if(w && curSuiteMgr->getState() != Ps_Jzed){
        w->addPz();
        refreshShowPzsState();
        //refreshActEnanble();
    }
}

/**
 * @brief MainWindow::on_actInsertPz_triggered
 *  插入凭证
 */
void MainWindow::on_actInsertPz_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
    if(w && curSuiteMgr->getState() != Ps_Jzed){
        w->insertPz();
        refreshShowPzsState();
        //refreshActEnanble();
    }
}

//删除凭证（综合考虑凭证集状态和凭证类别，来决定新的凭证集状态）
void MainWindow::on_actDelPz_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
    if(w && curSuiteMgr->getState() != Ps_Jzed){
        w->removePz();
        //isExtraVolid = false; //删除包含有会计分录（且金额非零）的凭证将导致余额的失效。这里不做过于细致的检测
        refreshShowPzsState();
        //refreshActEnanble();
    }
}

/**
 * @brief MainWindow::on_actAddAction_triggered
 *  新增会计分录
 */
void MainWindow::on_actAddAction_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
    if(w && curSuiteMgr->getState() != Ps_Jzed)
        w->addBa();
}

/**
 * @brief MainWindow::on_actInsertBa_triggered
 *  插入会计分录
 */
void MainWindow::on_actInsertBa_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
    if(w && curSuiteMgr->getState() != Ps_Jzed)
        w->insertBa();
}

/**
 * @brief MainWindow::on_actDelAction_triggered
 *  删除业务活动
 */
void MainWindow::on_actDelAction_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
    if(w && curSuiteMgr->getState() != Ps_Jzed)
        w->removeBa();
}

/**
 * @brief MainWindow::on_actGoFirst_triggered
 *  导航到第一张凭证
 */
void MainWindow::on_actGoFirst_triggered()
{
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT_new){
        PingZheng* oldPz = curSuiteMgr->getCurPz();
        PingZheng* newPz = curSuiteMgr->first();
        rfNaveBtn(newPz,oldPz);
    }
    else if(winType == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        historyPzSetIndex[suiteId] = 0;
        w->setPz(historyPzSet.value(suiteId).first());
        rfNaveBtn();
    }
//    if(subWindows.contains(SUBWIN_PZEDIT_new)){
//        curSuiteMgr->first();
//        //PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(PZEDIT_new)->widget());
//        //pzEdit->moveToFirst();
//        //rfNaveBtn();
//    }
}

/**
 * @brief MainWindow::on_actGoPrev_triggered
 *  导航到前一张凭证
 */
void MainWindow::on_actGoPrev_triggered()
{
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT_new){
        PingZheng* oldPz = curSuiteMgr->getCurPz();
        PingZheng* newPz = curSuiteMgr->previou();
        rfNaveBtn(newPz,oldPz);
    }
    else if(winType == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        if(historyPzSetIndex.value(suiteId) > 0){
            historyPzSetIndex[suiteId] -= 1;
            w->setPz(historyPzSet.value(suiteId).at(historyPzSetIndex.value(suiteId)));
            rfNaveBtn();
        }
    }
//    if(subWindows.contains(SUBWIN_PZEDIT_new)){
//        curSuiteMgr->previou();
//        //PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(PZEDIT_new)->widget());
//        //pzEdit->moveToPrev();
//        //rfNaveBtn();
//    }
}

/**
 * @brief MainWindow::on_actGoNext_triggered
 *  导航到后一张凭证
 */
void MainWindow::on_actGoNext_triggered()
{
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT_new){
        PingZheng* oldPz = curSuiteMgr->getCurPz();
        PingZheng* newPz = curSuiteMgr->next();
        rfNaveBtn(newPz,oldPz);
    }
    else if(winType == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        if(historyPzSetIndex.value(suiteId) < historyPzSet.value(suiteId).count()-1){
            historyPzSetIndex[suiteId] += 1;
            w->setPz(historyPzSet.value(suiteId).at(historyPzSetIndex.value(suiteId)));
            rfNaveBtn();
        }
    }

//    if(subWindows.contains(SUBWIN_PZEDIT_new)){
//        curSuiteMgr->next();
////        PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(PZEDIT_new)->widget());
////        pzEdit->moveToNext();
//        //rfNaveBtn();
//    }
}

/**
 * @brief MainWindow::on_actGoLast_triggered
 *  导航到最后一张凭证
 */
void MainWindow::on_actGoLast_triggered()
{
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT_new){
        PingZheng* oldPz = curSuiteMgr->getCurPz();
        PingZheng* newPz = curSuiteMgr->last();
        rfNaveBtn(newPz,oldPz);
    }
    else if(winType == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        historyPzSetIndex[suiteId] = historyPzSet.value(suiteId).count()-1;
        w->setPz(historyPzSet.value(suiteId).last());
        rfNaveBtn();
    }
//    if(subWindows.contains(SUBWIN_PZEDIT_new)){
//        curSuiteMgr->last();
//        //PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(PZEDIT_new)->widget());
//        //pzEdit->moveToLast();
//        //rfNaveBtn();
//    }
}

/**
 * @brief MainWindow::on_actSavePz_triggered
 */
void MainWindow::on_actSave_triggered()
{
    //根据激活的不同窗口，调用对应窗口的保存方法
    //账户属性设置窗口
    //科目设置窗口
    //本期统计窗口
    if(subWinActStates.value(SUBWIN_PZEDIT_new)){
        PzDialog* dlg = qobject_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget());
        dlg->save();
    }

    //通过凭证集对象的保存方法来保存与凭证集相关的项目（比如凭证对象、余额状态、凭证集状态等）
//    if(subWindows.contains(PZEDIT_new)){
//        PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(PZEDIT_new)->widget());
//        pzEdit->save();
//    }
//    //保存凭证集相关的状态
//    refreshShowPzsState();
//    //refreshActEnanble();
    ui->actSave->setEnabled(false);

}

//显示权限不足警告窗口
void MainWindow::rightWarnning(int right)
{
    QMessageBox::warning(this,tr("权限缺失警告"),
                         tr("由于缺失%1权限，此操作被拒绝！").arg(allRights.value(right)->getName()));
}

//未打开凭证集警告
void MainWindow::pzsWarning()
{
    QMessageBox::warning(this,tr("凭证集未打开警告"),
                         tr("凭证集还未打开。在执行任何账户操作前，请先打开凭证集"));
}

/**
 * @brief MainWindow::sqlWarning
 *  数据库访问发生错误警告
 */
void MainWindow::sqlWarning()
{
    QMessageBox::critical(0,tr("错误提示"),tr("在访问账户数据库文件时，发生错误！"));
}

//显示受影响的凭证数
void MainWindow::showPzNumsAffected(int num)
{
    if(num == 0)
        QMessageBox::information(this,tr("操作成功完成"),
                                     tr("本次操作成功完成，但没有凭证受此影响。"));
    else
        QMessageBox::information(this,tr("操作成功完成"),
                             tr("本次操作总共修改%1张凭证。").arg(num));
}

//保存子窗口信息（子窗口的位置，大小及其由参数sinfo指定的其他额外状态信息）
void MainWindow::saveSubWinInfo(subWindowType winEnum, QByteArray* sinfo)
{
    //QByteArray* oinfo = NULL;
    SubWindowDim* info = new SubWindowDim;
    info->x = subWindows.value(winEnum)->x();
    info->y = subWindows.value(winEnum)->y();
    info->w = subWindows.value(winEnum)->width();
    info->h = subWindows.value(winEnum)->height();
    dbUtil->saveSubWinInfo(winEnum,info,sinfo);
    delete info;
}

/**
 * @brief MainWindow::refreshShowPzsState
 *  保存当前凭证集的状态信息，并在状态条上显示
 */
void MainWindow::refreshShowPzsState()
{
    //显示各类凭证的数目
    if(!curSuiteMgr)
        return;
    ui->statusbar->setPzCounts(curSuiteMgr->getRepealCount(),curSuiteMgr->getRecordingCount(),
                               curSuiteMgr->getVerifyCount(),curSuiteMgr->getInstatCount(),curSuiteMgr->getPzCount());
//    PzsState state;
//    if(!dbUtil)
//        state = Ps_NoOpen;
//    else
//        dbUtil->getPzsState(cursy,cursm,state);
//    //如果凭证集原先未结账，则根据实际的凭证审核状态来决定凭证集的状态
//    if(!pzSetMgr->isOpened())
//        state = Ps_NoOpen;
//    else if(state == Ps_Jzed)
//        curPzSetState = Ps_Jzed;
//    else{
//        if(pzSetMgr->getRecordingCount() > 0)
//            curPzSetState = Ps_Rec;
//        else
//            curPzSetState = Ps_AllVerified;
//    }
//    if(cursy != 0 && cursy !=0){
//        dbUtil->setPzsState(cursy,cursm,curPzSetState);   //保存凭证集状态
//        dbUtil->setExtraState(cursy,cursm,isExtraVolid);  //保存余额状态
//    }
    ui->statusbar->setPzSetState(curSuiteMgr->getState());
    ui->statusbar->setExtraState(curSuiteMgr->getExtraState());
}

/**
 * @brief MainWindow::extraValid
 *  余额已经更新为有效（由本期统计窗口反馈给主窗口）
 *  待老的本期统计窗口被移除后，此槽可以移除
 */
void MainWindow::extraValid()
{
    //isExtraVolid = true;
    refreshShowPzsState();
    //refreshActEnanble();
}

/**
 * @brief MainWindow::pzSetStateChanged
 *  实时显示凭证集状态的改变
 * @param newState
 */
void MainWindow::pzSetStateChanged(PzsState newState)
{
    ui->statusbar->setPzSetState(newState);
}

/**
 * @brief MainWindow::pzSetExtraStateChanged
 *  实时更新凭证集余额的有效性
 * @param valid
 */
void MainWindow::pzSetExtraStateChanged(bool valid)
{
    ui->statusbar->setExtraState(valid);
}

/**
 * @brief MainWindow::undoViewItemClicked
 *  测试用户单击UndoView中的某个项目时，是先引发QUndoStack.setIndex()还是clicked()
 * @param indexes
 */
void MainWindow::undoViewItemClicked(const QModelIndex &indexes)
{
    int i = 0;

    LOG_INFO("enter undoViewItemClicked() slot!");
}

//结转汇兑损益
void MainWindow::on_actFordEx_triggered()
{
    if(subWindows.contains(SUBWIN_PZEDIT_new)){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget());
        if(!pzEdit->crtJzhdPz())
            QMessageBox::critical(0,tr("错误信息"),tr("在创建结转汇兑损益的凭证时发生错误!"));
    }
}

//结转损益
void MainWindow::on_actFordPl_triggered()
{
    if(subWindows.contains(SUBWIN_PZEDIT_new)){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget());
        if(!pzEdit->crtJzsyPz())
            QMessageBox::critical(0,tr("错误信息"),tr("在创建结转损益的凭证时发生错误!"));
    }
}

//统计本期发生额及其科目余额
void MainWindow::on_actCurStat_triggered()
{
    if(!curSuiteMgr->isOpened()){
        pzsWarning();
        return;
    }
    //为了使本期统计得以正确执行，必须将主窗口记录的凭证集状态保存到账户中
    dbUtil->setPzsState(cursy,cursm,curSuiteMgr->getState());
    ViewExtraDialog* dlg;
    if(subWindows.contains(SUBWIN_PZSTAT)){
        dlg = static_cast<ViewExtraDialog*>(subWindows.value(SUBWIN_PZSTAT)->widget());
        dlg->setDate(cursy,cursm);
        //激活此子窗口
        ui->mdiArea->setActiveSubWindow(subWindows.value(SUBWIN_PZSTAT));
        return;
    }

    SubWindowDim* winfo;
    QByteArray* sinfo;
    dbUtil->getSubWinInfo(SUBWIN_PZSTAT,winfo,sinfo);
    dlg = new ViewExtraDialog(curAccount, cursy, cursm, sinfo, this );
    connect(dlg,SIGNAL(infomation(QString)),this,SLOT(showTemInfo(QString)));
    connect(dlg,SIGNAL(pzsExtraSaved()),this,SLOT(extraValid()));

    showSubWindow(SUBWIN_PZSTAT,winfo,dlg);
    if(winfo)
        delete winfo;
    if(sinfo)
        delete sinfo;
}

/**
 * @brief MainWindow::on_actCurStatNew_triggered
 *  统计本期发生额，并计算期末余额
 */
void MainWindow::on_actCurStatNew_triggered()
{
    if(!curSuiteMgr->isOpened()){
        pzsWarning();
        return;
    }
    if(!curSuiteMgr->isDirty())
        curSuiteMgr->save();

    //为了使本期统计得以正确执行，必须将主窗口记录的凭证集状态保存到账户中
    //dbUtil->setPzsState(cursy,cursm,curSuiteMgr->getState());
    CurStatDialog* dlg = NULL;
//    if(subWindows.contains(SUBWIN_PZSTAT2)){
//        dlg = static_cast<CurStatDialog*>(subWindows.value(SUBWIN_PZSTAT2)->widget());
//        //dlg->stat();
//        //激活此子窗口
//        ui->mdiArea->setActiveSubWindow(subWindows.value(SUBWIN_PZSTAT2));
//        return;
//    }
    SubWindowDim* winfo = NULL;
    QByteArray* sinfo = NULL;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZSTAT2)){
        dbUtil->getSubWinInfo(SUBWIN_PZSTAT2,winfo,sinfo);
        dlg = new CurStatDialog(&curSuiteMgr->getStatObj(), sinfo, this);
        connect(dlg,SIGNAL(infomation(QString)),this,SLOT(showTemInfo(QString)));
    }
    else{
        dlg = static_cast<CurStatDialog*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_PZSTAT2));
        if(dlg)
            dlg->stat(&curSuiteMgr->getStatObj());
    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZSTAT2,dlg,winfo);




    //connect(dlg,SIGNAL(pzsExtraSaved()),this,SLOT(extraValid()));

    //showSubWindow(SUBWIN_PZSTAT2,winfo,dlg);
    if(winfo)
        delete winfo;
    if(sinfo)
        delete sinfo;
}

//打印
void MainWindow::on_actPrint_triggered()
{
    PrintSelectDialog* psDlg = new PrintSelectDialog(curSuiteMgr, this);
    if(!PrintPznSet.isEmpty())
        psDlg->setPzSet(PrintPznSet);
    psDlg->setCurPzn(curSuiteMgr->getCurPz()->number());
    if(psDlg->exec() == QDialog::Accepted){
        QList<PingZheng*> pzs;
        if(!psDlg->getSelectedPzs(pzs))//获取打印范围
            return;
        QPrinter printer;
        QPrintDialog* dlg = new QPrintDialog(&printer); //获取所选的打印机
        if(dlg->exec() == QDialog::Accepted){
            QPrintPreviewDialog* preview;
            printer.setOrientation(QPrinter::Portrait);
            PrintPzUtils* view = new PrintPzUtils(curAccount,&printer);
            view->setPzs(pzs);
            switch(psDlg->getPrintMode()){
            case 1: //输出到打印机
                view->print(&printer);
                break;
            case 2: //打印预览
                preview = new QPrintPreviewDialog(&printer);
                connect(preview, SIGNAL(paintRequested(QPrinter*)),
                        view, SLOT(print(QPrinter*)));
                preview->exec();
                break;
            case 3: //输出到PDF
                printer.setOutputFormat(QPrinter::PdfFormat);
                QString fname = QFileDialog::getSaveFileName(this,tr("请输入文件名"),"./outPdfs","*.pdf");
                if(fname != ""){
                    printer.setOutputFileName(fname);
                    view->print(&printer);
                }
                break;
            }
        }
        delete dlg;
    }
    delete psDlg;
}

//接收当前显示的凭证号
//void MainWindow::curPzNumChanged(int pzNum)
//{
//    curPzn = pzNum;
//}

//在子窗口被关闭后，删除对应的子窗口对象实例
void MainWindow::subWindowClosed(QMdiSubWindow* subWin)
{
    QByteArray* sinfo = NULL;
    subWindowType winEnum = SUBWIN_NONE;
    if(subWin == subWindows.value((SUBWIN_PZEDIT_new))){
        winEnum = SUBWIN_PZEDIT_new;
        if(!undoStack->isClean()){
            if(QMessageBox::Yes == QMessageBox::warning(this,msgTitle_warning,tr("凭证内容已改变，要保存吗？"),
                                 QMessageBox::Yes|QMessageBox::No)){
                curSuiteMgr->save(AccountSuiteManager::SW_PZS);
                undoStack->setClean();
            }
            else{
                int cleanIndex = undoStack->cleanIndex();
                undoStack->setIndex(cleanIndex);
            }
        }
        PzDialog* dlg = static_cast<PzDialog*>(subWin->widget());
        sinfo = dlg->getState();
        disconnect(dlg,SIGNAL(showMessage(QString,AppErrorLevel)),this,SLOT(showRuntimeInfo(QString,AppErrorLevel)));
        disconnect(dlg,SIGNAL(selectedBaChanged(QList<int>,bool)),this,SLOT(baSelectChanged(QList<int>,bool)));
        delete dlg;
        //curPzn = 0;
    }
    else if(subWin == subWindows.value(SUBWIN_PZSTAT)){
        winEnum = SUBWIN_PZSTAT;
        ViewExtraDialog* dlg = static_cast<ViewExtraDialog*>(subWin->widget());
        sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(SUBWIN_PZSTAT2)){
        winEnum = SUBWIN_PZSTAT2;
        CurStatDialog* dlg = static_cast<CurStatDialog*>(subWin->widget());
        sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(SUBWIN_SETUPBASE)){
        winEnum = SUBWIN_SETUPBASE;
        SetupBaseDialog2* dlg = static_cast<SetupBaseDialog2*>(subWin->widget());
        delete dlg;
    }
//    else if(subWin == subWindows.value(SETUPBANK)){
//        winEnum = SETUPBANK;
//        SetupBankDialog* dlg = static_cast<SetupBankDialog*>(subWin->widget());
//        delete dlg;
//    }
    else if(subWin == subWindows.value(SUBWIN_BASEDATAEDIT)){
        winEnum = SUBWIN_BASEDATAEDIT;
        BaseDataEditDialog* dlg = static_cast<BaseDataEditDialog*>(subWin->widget());
        delete dlg;
    }
    else if(subWin == subWindows.value(SUBWIN_GDZCADMIN)){
        winEnum = SUBWIN_GDZCADMIN;
        GdzcAdminDialog* dlg = static_cast<GdzcAdminDialog*>(subWin->widget());
        dlg->save();
        sinfo = dlg->getState();
        delete dlg;
    }
//    else if(subWin == subWindows.value(DTFYADMIN)){
//        winEnum = DTFYADMIN;
//        DtfyAdminDialog* dlg = static_cast<DtfyAdminDialog*>(subWin->widget());
//        dlg->save();
//        sinfo = dlg->getState();
//        delete dlg;
//    }
    else if(subWin == subWindows.value(TOTALVIEW)){
        winEnum = TOTALVIEW;
        ShowTZDialog* dlg = static_cast<ShowTZDialog*>(subWin->widget());
        sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(DETAILSVIEW)){
        winEnum = DETAILSVIEW;
        ShowDZDialog* dlg = static_cast<ShowDZDialog*>(subWin->widget());
        sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(SUBWIN_DETAILSVIEW2)){
        winEnum = SUBWIN_DETAILSVIEW2;
        ShowDZDialog2* dlg = static_cast<ShowDZDialog2*>(subWin->widget());
        sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(SUBWIN_HISTORYVIEW)){
        winEnum = SUBWIN_HISTORYVIEW;
        HistoryPzForm* dlg = static_cast<HistoryPzForm*>(subWin->widget());
        sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(SUBWIN_LOOKUPSUBEXTRA)){
        winEnum = SUBWIN_LOOKUPSUBEXTRA;
        LookupSubjectExtraDialog* dlg =
                static_cast<LookupSubjectExtraDialog*>(subWin->widget());
        //sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(SUBWIN_ACCOUNTPROPERTY)){
        winEnum = SUBWIN_ACCOUNTPROPERTY;
        AccountPropertyConfig* dlg =
                static_cast<AccountPropertyConfig*>(subWin->widget());
        //dlg->save(true);
        delete dlg;
    }
    else if(subWin == subWindows.value(SUBWIN_VIEWPZSETERROR)){
        winEnum = SUBWIN_VIEWPZSETERROR;
        ViewPzSetErrorForm* form = static_cast<ViewPzSetErrorForm*>(subWin->widget());
        disconnect(form,SIGNAL(reqLoation(int,int)),
                   this,SLOT(openSpecPz(int,int)));
        delete form;
    }

    if(winEnum != SUBWIN_NONE){
        saveSubWinInfo(winEnum,sinfo);
        subWindows.remove(winEnum);
    }
    ui->mdiArea->removeSubWindow(subWin);
    //rfAct();
    //rfTbrVisble();
}

//当子窗口被激活时，更新内部记录子窗口激活状态的变量 subWinActStates
void MainWindow::subWindowActivated(QMdiSubWindow *window)
{
    //找到激活的子窗口类型
    QHashIterator<subWindowType, MyMdiSubWindow*> it(subWindows);
    subWindowType winType = SUBWIN_NONE;
    while(it.hasNext()){
        it.next();
        if(window == it.value()){
            winType = it.key();
            break;
        }
    }
    QHashIterator<subWindowType,bool> i(subWinActStates);
    while(i.hasNext()){
        i.next();
        if(winType == SUBWIN_NONE)
            subWinActStates[i.key()] = false;
        else if(i.key() == winType)
            subWinActStates[i.key()] = true;
        else
            subWinActStates[i.key()] = false;
    }

    //根据激活的子窗口是否有未执行的保存来确定保存按钮的启用性
    bool isDirty = false;
    switch(winType){
    case SUBWIN_PZEDIT_new:
        isDirty = static_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget())->isDirty();
        rfNaveBtn(curSuiteMgr->getCurPz(),curSuiteMgr->getCurPz());
        break;
    }
    ui->actSave->setEnabled(isDirty);
    if(winType != SUBWIN_PZEDIT_new){
        rfNaveBtn(0,0);  //当凭证编辑窗口不被激活时，禁用禁用相关action
    }

    //rfAct();
    //rfTbrVisble(); //刷新工具条
//    if(winType == PZEDIT_new){
//        rfNaveBtn();
//    }

    /////////////////////////////
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(window);
    if(w){
        subWindowType winType = w->getWindowType();
        if(winType == SUBWIN_PZEDIT_new){
            rfNaveBtn(curSuiteMgr->getCurPz(),curSuiteMgr->getCurPz());
        }
    }

    ///////////////////////////////
}

//将当前显示的凭证号加入到打印队列中
void MainWindow::on_actPrintAdd_triggered()
{
    if(curSuiteMgr->isOpened() && curSuiteMgr->getCurPz() && subWinActStates.value(SUBWIN_PZEDIT_new))
        PrintPznSet.insert(curSuiteMgr->getCurPz()->number());

}

//将当前显示的凭证号从打印队列中移除
void MainWindow::on_actPrintDel_triggered()
{
    if(curSuiteMgr->isOpened() && curSuiteMgr->getCurPz() && subWinActStates.value(SUBWIN_PZEDIT_new))
        PrintPznSet.remove(curSuiteMgr->getCurPz()->number());
}

//登录
void MainWindow::on_actLogin_triggered()
{
    LoginDialog* dlg = new LoginDialog;
    if(dlg->exec() == QDialog::Accepted){
        curUser = dlg->getLoginUser();
        rfLogin();
        ui->statusbar->setUser(curUser);
    }
    else
        rfLogin(false);
}

//登出
void MainWindow::on_actLogout_triggered()
{
    curUser = NULL;
    ui->statusbar->setUser(curUser);
    //enaActOnLogin(false);
    //rfTbrVisble();
    //refreshActEnanble();
}


//切换用户
void MainWindow::on_actShiftUser_triggered()
{
    LoginDialog* dlg = new LoginDialog;
    if(dlg->exec() == QDialog::Accepted){
        curUser = dlg->getLoginUser();
        ui->statusbar->setUser(curUser);
    }
    //rfTbrVisble();
    //refreshActEnanble();
}

//显示安全配置对话框
void MainWindow::on_actSecCon_triggered()
{
    SecConDialog* dlg = new SecConDialog(this);
    ui->mdiArea->addSubWindow(dlg);
    dlg->exec();
}


//向上移动业务活动
void MainWindow::on_actMvUpAction_triggered()
{
    if(activedMdiChild() == SUBWIN_PZEDIT_new){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
        pzEdit->moveUpBa();
    }

}

//向下移动业务活动
void MainWindow::on_actMvDownAction_triggered()
{
    if(activedMdiChild() == SUBWIN_PZEDIT_new){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT_new));
        pzEdit->moveDownBa();
    }
}

//搜索凭证
void MainWindow::on_actSearch_triggered()
{
    SearchDialog* dlg = new SearchDialog;
    dlg->exec();
}

//转到指定号码的凭证
void MainWindow::on_actNaviToPz_triggered()
{
    editPzs();
    int num = spnNaviTo->value();
    if(!curSuiteMgr->seek(num)){
        QMessageBox::question(this,tr("错误提示"),
                              tr("没有凭证号为%1的凭证").arg(num));

    }
    //rfNaveBtn();
}

//全部手工凭证通过审核
void MainWindow::on_actAllVerify_triggered()
{
//    if(!curUser->haveRight(allRights.value(Right::VerifyPz))){
//        rightWarnning(Right::VerifyPz);
//        return;
//    }
//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }

//    int affectedRows;
//    dbUtil->setAllPzState(cursy,cursm,Pzs_Verify,Pzs_Recording,affectedRows,curUser);
//    dbUtil->scanPzSetCount(cursy,cursm,pzRepeal,pzRecording,pzVerify,pzInstat,pzAmount);
//    refreshShowPzsState();
//    showPzNumsAffected(affectedRows);
//    if(subWindows.contains(PZEDIT)){
//        PzDialog2* dlg = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
//        int pid = dlg->getCurPzId();
//        curPzModel->select();
//        dlg->naviTo(pid);
//    }
//    isExtraVolid = false;
}

//全部凭证入账
void MainWindow::on_actAllInstat_triggered()
{
    //应该利用undo框架完成该动作，而不是直接操纵数据库
//    if(!curUser->haveRight(allRights.value(Right::InstatPz))){
//        rightWarnning(Right::InstatPz);
//        return;
//    }
//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }

//    int affectedRows;
//    dbUtil->setAllPzState(cursy,cursm,Pzs_Instat,Pzs_Verify,affectedRows,curUser);
//    dbUtil->scanPzSetCount(cursy,cursm,pzRepeal,pzRecording,pzVerify,pzInstat,pzAmount);
//    refreshShowPzsState();
//    showPzNumsAffected(affectedRows);
//    if(subWindows.contains(PZEDIT)){
//        PzDialog2* dlg = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
//        int pid = dlg->getCurPzId();
//        curPzModel->select();
//        dlg->naviTo(pid);
//    }
//    isExtraVolid = false;
}



//重新分派凭证号
void MainWindow::on_actReassignPzNum_triggered()
{
//    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
//    if(pzEdit){
//        pzEdit->reAssignPzNum();
//    }
}

//引入其他模块自动产生的凭证（比如固定资产折旧、待摊费用等）
void MainWindow::on_actImpOtherPz_triggered()
{
//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }

//    QSet<OtherModCode> mods;
//    ImpOthModDialog* dlg = new ImpOthModDialog(1,this);

//    //如果用户选择取消，则什么也不做
//    if(QDialog::Rejected == dlg->exec()){
//        delete dlg;
//        return;
//    }

//    //用户选择跳过
//    if(dlg->isSelSkip()){
//        bool req;
//        BusiUtil::reqGenJzHdsyPz(cursy,cursm,req);
//        if(req)
//            BusiUtil::setPzsState(cursy, cursm, Ps_Stat2);
//        else  //如果同时也不需要结转汇兑损益，则直接准备结转损益
//            BusiUtil::setPzsState(cursy, cursm, Ps_JzsyPre);
//        delete dlg;
//        return;
//    }

//    //用户选择确定
//    QSet<OtherModCode> selMods;
//    dlg->selModules(selMods);
//    if(!BusiUtil::impPzFromOther(cursy,cursm,selMods))
//        showTemInfo(tr("在引入由其他模块创建的凭证时出错！"));
//    else if(selMods.empty())
//        showTemInfo(tr("你未选择模块，本次操作什么也不做！"));
//    else{
//        showTemInfo(tr("成功引入由其他模块创建的凭证！"));
//        refreshShowPzsState();
//        refreshActEnanble();
//    }

//    delete dlg;
}

//结转本年利润
void MainWindow::on_actJzbnlr_triggered()
{
//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }
//    if(curPzSetState == Ps_Jzed){
//        QMessageBox::information(this, tr("提示信息"), tr("已结账，不能添加任何凭证！"));
//        return;
//    }
//    if(curPzSetState != Ps_AllVerified){
//        QMessageBox::warning(this, tr("提示信息"), tr("当前凭证集内存在未审核凭证，不能结转！"));
//        return;
//    }
//    if(!isExtraVolid){
//        QMessageBox::warning(this, tr("提示信息"), tr("当前余额无效，请重新统计并保存余额！"));
//        return;
//    }
//    //损益类凭证必须已经结转了
//    int count;
//    dbUtil->inspectJzPzExist(cursy,cursm,Pzd_Jzsy,count);
//    if(count > 0){
//        QMessageBox::warning(this, tr("提示信息"), tr("在结转本年利润前，必须先结转损益类科目到本年利润！"));
//        return;
//    }
//    //创建一个结转本年利润到利润分配的特种类别空白凭证，由用户手动编辑此凭证
//    PzData d;
//    QDate date(cursy,cursm,1);
//    date.setDate(cursy,cursm,date.daysInMonth());
//    d.date = date.toString(Qt::ISODate);
//    d.attNums = 0;
//    d.pzNum = pzAmount+1;
//    d.pzZbNum = pzAmount+1;
//    d.state = Pzs_Recording;
//    d.pzClass = Pzc_Jzlr;
//    d.jsum = 0; d.dsum = 0;
//    d.producer = curUser;
//    d.verify = NULL;
//    d.bookKeeper = NULL;

//    if(dbUtil->crtNewPz(&d)){
//        showTemInfo(tr("成功创建结转本年利润凭证！"));
//        pzAmount++;
//        pzRecording++;
//        isExtraVolid = false;
//        refreshShowPzsState();
//        //refreshActEnanble();
//        //如果凭证编辑窗口已打开，则定位到刚创建的凭证
//        if(subWindows.contains(PZEDIT)){
//            curPzModel->select();
//            PzDialog2* dlg = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
//            dlg->naviLast();
//        }
//    }
//    else
//        showTemInfo(tr("在创建结转本年利润凭证时出错！"));
}

//结账
void MainWindow::on_actEndAcc_triggered()
{
    if(!curSuiteMgr->isOpened()){
        pzsWarning();
        return;
    }
    if(curSuiteMgr->getState() == Ps_Jzed){
        QMessageBox::information(this, tr("提示信息"), tr("已经结账"));
        return;
    }
    if(curSuiteMgr->getState() != Ps_AllVerified){
        QMessageBox::warning(this, tr("提示信息"), tr("凭证集内存在未审核凭证，不能结账！"));
        return;
    }
    if(!curSuiteMgr->getExtraState()){
        QMessageBox::warning(this, tr("提示信息"), tr("当前的余额无效，不能结账！"));
        return;
    }
    if(QMessageBox::Yes == QMessageBox::information(this,tr("提示信息"),
                                                    tr("结账后，将不能再次对凭证集进行修改，确认要结账吗？"),
                                                    QMessageBox::Yes | QMessageBox::No)){
        //curPzSetState = Ps_Jzed;
        dbUtil->setPzsState(cursy,cursm,Ps_Jzed);
        refreshShowPzsState();
        //refreshActEnanble();
        return;
    }
}

//反结转
void MainWindow::on_actAntiJz_triggered()
{
    //提供3种结转凭证类型（按优先级从高到低依次为结转利润、结转损益、结转汇兑损益）
    //的取消功能，由用户来选择。

    //功能需求说明：
    //1：只要凭证集未结账，就允许用户执行反结转功能，这个可以通过控制此菜单项的启用性来完成。
    //2：可选性限制，根据当前凭证集实际存在的结转凭证状况调整可以反结转的凭证项
    //3：结转凭证的产生有一个顺序的要求，从前往后依次是结转汇兑损益、结转损益、结转利润。
    //当反结转时也应该以反序操作。

//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }
//    QHash<PzdClass,bool> isExists;
//    dbUtil->haveSpecClsPz(cursy,cursm,isExists);
//    AntiJzDialog dlg(isExists,this);
//    bool isAnti = false;
//    if(QDialog::Accepted == dlg.exec()){
//        isExists.clear();
//        isExists = dlg.selected();
//        int affeced = 0;
//        if(isExists.value(Pzd_Jzlr)){
//            isAnti = true;
//            dbUtil->delSpecPz(cursy,cursm,Pzd_Jzlr,affeced);
//        }
//        if(isExists.value(Pzd_Jzsy)){
//            isAnti = true;
//            dbUtil->delSpecPz(cursy,cursm,Pzd_Jzsy,affeced);
//        }
//        if(isExists.value(Pzd_Jzhd)){
//            isAnti = true;
//            dbUtil->delSpecPz(cursy,cursm,Pzd_Jzhd,affeced);
//        }
//        if(affeced > 0){
//            isExtraVolid = false;
//            dbUtil->scanPzSetCount(cursy,cursm,pzRepeal,pzRecording,pzVerify,pzInstat,pzAmount);
//        }
//        refreshShowPzsState();
//    }
}

//反结账
void MainWindow::on_actAntiEndAcc_triggered()
{
    //打破结账限制，
//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }
//    if(curPzSetState != Ps_Jzed){
//        QMessageBox::information(this, tr("提示信息"), tr("还未结账"));
//        return;
//    }
//    dbUtil->setPzsState(cursy,cursm,/*Ps_Stat5*/Ps_Rec);
//    allPzToRecording(cursy,cursm);
//    isExtraVolid = false;
//    dbUtil->scanPzSetCount(cursy,cursm,pzRepeal,pzRecording,pzVerify,pzInstat,pzAmount);
//    refreshShowPzsState();
//    //refreshActEnanble();
}

//反引用
void MainWindow::on_actAntiImp_triggered()
{
//    //取消由其他模块产生的引入凭证（实现上是否可以简单地使此类凭证取消审核）
//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }
//    ImpOthModDialog dlg(2, this);
//    if(QDialog::Accepted == dlg.exec()){
//        QSet<OtherModCode> mods;
//        if(dlg.selModules(mods) && BusiUtil::antiImpPzFromOther(cursy,cursm,mods))
//            showTemInfo(tr("已删除选定模块的引入凭证！"));
//    }
}

//打开Sql工具
void MainWindow::showSqlTool()
{
    DatabaseAccessForm* dlg = new DatabaseAccessForm(curAccount, AppConfig::getInstance());
    int winNumber = 1;
    if(commonGroups_multi.count(SUBWIN_SQL) > 0){
        QList<int> winNums;
        foreach(MyMdiSubWindow* sub, commonGroups_multi.values(SUBWIN_SQL)){
            QString title = sub->windowTitle();
            int index = title.indexOf("-");
            if(index != -1){
                QString numStr = title.right(title.count()-index-1);
                winNums<<numStr.toInt();
            }
        }
        qSort(winNums);
        for(int i = 0; i < winNums.count(); ++i){
            if(winNumber < winNums.at(i))
                break;
            else
                winNumber++;
        }
    }
    dlg->setWindowTitle(tr("SQL工具窗-%1").arg(winNumber));
    showCommonSubWin(SUBWIN_SQL,dlg,NULL);
}

//显示基础数据库访问窗口
void MainWindow::on_actBasicDB_triggered()
{
    showSubWindow(SUBWIN_BASEDATAEDIT);
}

//固定资产管理
void MainWindow::on_actGdzcAdmin_triggered()
{
//    QByteArray* sinfo;
//    SubWindowDim* winfo;
//    VariousUtils::getSubWinInfo(GDZCADMIN,winfo,sinfo);
//    GdzcAdminDialog* dlg = new GdzcAdminDialog(sinfo);
//    showSubWindow(GDZCADMIN,winfo,dlg);
//    if(sinfo)
//        delete sinfo;
//    if(winfo)
//        delete winfo;
}

//待摊费用管理
void MainWindow::on_actDtfyAdmin_triggered()
{
//    QByteArray* sinfo;
//    SubWindowDim* winfo;
//    VariousUtils::getSubWinInfo(DTFYADMIN,winfo,sinfo);
//    DtfyAdminDialog* dlg = new DtfyAdminDialog(sinfo,adb);
//    showSubWindow(DTFYADMIN,winfo,dlg);
//    if(sinfo)
//        delete sinfo;
//    if(winfo)
//        delete winfo;
}

//查看总账
void MainWindow::on_actShowTotal_triggered()
{
    QByteArray* sinfo;
    SubWindowDim* winfo;
    dbUtil->getSubWinInfo(TOTALVIEW,winfo,sinfo);
    ShowTZDialog* dlg = new ShowTZDialog(cursy,cursm,sinfo);
    showSubWindow(TOTALVIEW,winfo,dlg);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

//查看明细账
void MainWindow::on_actShowDetail_triggered()
{

    HappenSubSelDialog hdlg(cursy,cursm);
    if(QDialog::Accepted == hdlg.exec()){
        int sm,em;
        hdlg.getTimeRange(sm,em);
        int witch;
        QList<int> fids;
        QHash<int,QList<int> > sids;
        double gv=0,lv=0;
        bool inc;
        hdlg.getSubRange(witch,fids,sids,gv,lv,inc);

        QByteArray* sinfo;
        SubWindowDim* winfo;
        dbUtil->getSubWinInfo(DETAILSVIEW,winfo,sinfo);
        ShowDZDialog* dlg = new ShowDZDialog(curAccount,sinfo);
        connect(dlg,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
        dlg->setSubRange(witch,fids,sids,gv,lv,inc);
        dlg->setDateRange(sm,em,cursy);   //暂且用cursy来代表年份，未来要修改

        showSubWindow(DETAILSVIEW, winfo, dlg);
        if(sinfo)
            delete sinfo;
        if(winfo)
            delete winfo;
    }

}

/**
 * @brief MainWindow::on_actDetailView_triggered
 *  查看明细账（新）
 */
void MainWindow::on_actDetailView_triggered()
{
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    ShowDZDialog2* dlg = NULL;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_DETAILSVIEW2)){
        dbUtil->getSubWinInfo(SUBWIN_DETAILSVIEW2,winfo,sinfo);
        dlg = new ShowDZDialog2(curAccount,sinfo);
        connect(dlg,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_DETAILSVIEW2,dlg,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

void MainWindow::on_actInStatPz_triggered()
{
    if(!curSuiteMgr->isOpened())
        return;
    if(!curSuiteMgr->getCurPz())
        return ;
    if(subWindows.contains(SUBWIN_PZEDIT_new)){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget());
        pzEdit->setPzState(Pzs_Instat);
    }
}

void MainWindow::on_actVerifyPz_triggered()
{
    if(!curSuiteMgr->isOpened())
        return;
    if(!curSuiteMgr->getCurPz())
        return ;
    if(subWindows.contains(SUBWIN_PZEDIT_new)){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget());
        pzEdit->setPzState(Pzs_Verify);
    }
}


//取消已审核或已入账的凭证，使凭证回到初始状态
void MainWindow::on_actAntiVerify_triggered()
{
    if(!curSuiteMgr->isOpened())
        return;
    if(!curSuiteMgr->getCurPz())
        return ;
    if(subWindows.contains(SUBWIN_PZEDIT_new)){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWindows.value(SUBWIN_PZEDIT_new)->widget());
        pzEdit->setPzState(Pzs_Recording);
    }
}

/**
 * @brief MainWindow::on_actRefreshActInfo_triggered
 *  刷新工作目录内的账户
 */
void MainWindow::on_actRefreshActInfo_triggered()
{
    //刷新前要关闭当前打开的账户
    if(curAccount)
        closeAccount();
    QList<AccountCacheItem*> accLst;
    if(!AppConfig::getInstance()->initAccountCache(accLst)){
        QMessageBox::critical(this,tr("错误信息"),tr("在扫描工作目录下的账户时发生错误！"));
        return;
    }
    //报告发现的账户
    QMessageBox::information(this,tr("提示信息"),tr("本次扫描共发现%1个账户！").arg(accLst.count()));
    qDeleteAll(accLst);


    //清除已有的账户信息
//    AppConfig* appCfg = AppConfig::getInstance();
//    QDir dir(DatabasePath /*"./datas/databases"*/);
//    QStringList filters, filelist;
//    filters << "*.dat";
//    dir.setNameFilters(filters);
//    filelist = dir.entryList(filters, QDir::Files);
//    int fondCount = 0;
//    if(filelist.count() == 0)
//        QMessageBox::information(this, tr("一般信息"),
//                                 tr("当前没有可用的帐户数据库文件"));
//    else{
//        appCfg->clear();
//        foreach(QString fname, filelist){
//            DbUtil du;
//            if(!du.setFilename(fname))
//                continue;
//            AccountBriefInfo accInfo;
//            if(!du.readAccBriefInfo(accInfo))
//                continue;
//            appCfg->saveAccInfo(accInfo);
//            fondCount++;
//        }
//        //报告查找结果
//        if(fondCount == 0)
//            QMessageBox::information(this, tr("搜寻账户"), tr("没有发现账户文件"));
//        else
//            QMessageBox::information(this, tr("搜寻账户"),
//                                     QString(tr("共发现%1个账户文件")).arg(fondCount));

//    }
}

/**
 * @brief 切换帐套视图
 */
void MainWindow::on_actSuite_triggered()
{
    if(!dockWindows.contains(TV_SUITESWITCH)){
        SuiteSwitchPanel* pn = new SuiteSwitchPanel(curAccount);
        QDockWidget* dw = new QDockWidget(tr("帐套切换视图"), this);
        dw->setWidget(pn);
        addDockWidget(Qt::LeftDockWidgetArea, dw);
        dockWindows[TV_SUITESWITCH] = dw;
        connect(pn,SIGNAL(selectedSuiteChanged(AccountSuiteManager*,AccountSuiteManager*)),
                this,SLOT(suiteViewSwitched(AccountSuiteManager*,AccountSuiteManager*)));
        connect(pn,SIGNAL(viewPzSet(AccountSuiteManager*,int)),this,SLOT(viewOrEditPzSet(AccountSuiteManager*,int)));
        connect(pn,SIGNAL(pzSetOpened(AccountSuiteManager*,int)),this,SLOT(pzSetOpen(AccountSuiteManager*,int)));
        connect(pn,SIGNAL(prepareClosePzSet(AccountSuiteManager*,int)),this,SLOT(prepareClosePzSet(AccountSuiteManager*,int)));
        connect(pn,SIGNAL(pzsetClosed(AccountSuiteManager*,int)),this,SLOT(pzSetClosed(AccountSuiteManager*,int)));
    }
    dockWindows.value(TV_SUITESWITCH)->show();
}

/**
 * @brief 判断公共类窗口类型是否是唯一性子窗口
 * @param winType
 */
bool MainWindow::isOnlyCommonSubWin(subWindowType winType)
{
    if(winType == SUBWIN_SQL)
        return false;
    else
        return true;
}

/**
 * @brief 显示公共类子窗口
 * @param winType
 * @param widget
 * @param dim
 */
void MainWindow::showCommonSubWin(subWindowType winType, QWidget* widget, SubWindowDim *dim)
{
    MyMdiSubWindow* w = NULL;
    if(isOnlyCommonSubWin(winType)){
        if(widget){
            w = new MyMdiSubWindow(0,winType);
            w->setWidget(widget);
            w->setAttribute(Qt::WA_DeleteOnClose);
            ui->mdiArea->addSubWindow(w);
            if(dim){
                w->resize(dim->w,dim->h);
                w->move(dim->x,dim->y);
            }
            commonGroups[winType] = w;            
        }
        else{
            ui->mdiArea->setActiveSubWindow(commonGroups.value(winType));
            return;
        }
    }
    else{
        w = new MyMdiSubWindow(0,winType);
        w->setWidget(widget);
        w->setAttribute(Qt::WA_DeleteOnClose);
        ui->mdiArea->addSubWindow(w);
        commonGroups_multi.insert(winType,w);
    }
    if(w){
        connect(w,SIGNAL(windowClosed(MyMdiSubWindow*)),this,SLOT(commonSubWindowClosed(MyMdiSubWindow*)));
        w->show();
    }
}

//显示只能有一个实例的对话框窗口（参数w是位于mdi子窗口内部的中心部件，仅对于部分子窗口有效，比如凭证编辑窗口）
void MainWindow::showSubWindow(subWindowType winType, SubWindowDim* winfo, QDialog* w)
{
    if(subWindows.contains(winType)){
        ui->mdiArea->setActiveSubWindow(subWindows.value(winType));
    }
    else{
        QDialog* dlg;
        //ShowDZDialog* dzd;
        subWindows[winType] = new MyMdiSubWindow;
        switch(winType){
        case SUBWIN_PZEDIT_new:
        case SUBWIN_PZSTAT:
        case SUBWIN_PZSTAT2:
            dlg = w;
            //在linux平台上测试，如果设置最大或最小化按钮，则同时出现关闭按钮，我晕
            //subWindows.value(winType)->setWindowFlags(Qt::CustomizeWindowHint);
            break;
        case TOTALVIEW:
            dlg = w;
            break;
        case DETAILSVIEW:
        case SUBWIN_DETAILSVIEW2:
            dlg = w;
            //dzd = qobject_cast<ShowDZDialog*>(dlg);
            //connect(dzd,SIGNAL(closeWidget()) ,subWindows[winType],SLOT(close()));
            break;
//        case SETUPBANK:
//            dlg = new SetupBankDialog(false, this);
//            break;
        case SUBWIN_SETUPBASE:
            dlg = new SetupBaseDialog2(curAccount);
            break;
        case SUBWIN_BASEDATAEDIT:
            dlg = new BaseDataEditDialog;
            break;
        case SUBWIN_GDZCADMIN:
            dlg = w;
            break;
//        case DTFYADMIN:
//            dlg = w;
//            break;
        case SUBWIN_HISTORYVIEW:
            dlg = w;
            break;
        case SUBWIN_LOOKUPSUBEXTRA:
            dlg = w;
            break;
        case SUBWIN_ACCOUNTPROPERTY:
            dlg = w;
            break;
        }

        connect(dlg,SIGNAL(accepted()) ,subWindows[winType],SLOT(close()));
        connect(dlg,SIGNAL(rejected()) ,subWindows[winType],SLOT(close()));
        subWindows[winType]->setWidget(dlg);
        subWinActStates[winType] = true; //设置窗口的激活状态
        ui->mdiArea->addSubWindow(subWindows[winType]);
        connect(subWindows[winType], SIGNAL(windowClosed(QMdiSubWindow*)),
                this, SLOT(subWindowClosed(QMdiSubWindow*)));

        //对于凭证编辑窗口，因为它重载了基类的show函数，因此必须显示调用子类的方法。
        if(winType == SUBWIN_PZEDIT){
            PzDialog2* pzDlg = static_cast<PzDialog2*>(w);
            pzDlg->show();
        }
        else
            dlg->show();

        //SubWindowDim* info = new SubWindowDim;
        //QByteArray* oinfo;
        if(winfo){
            subWindows.value(winType)->move(winfo->x,winfo->y);
            subWindows.value(winType)->resize(winfo->w,winfo->h);
        }
        else{
            subWindows.value(winType)->move(10,10);
            subWindows.value(winType)->resize(mdiAreaWidth * 0.8, mdiAreaHeight * 0.8);
        }


        //恢复子窗口的特定状态信息
        //QVariant* vdInfo;
        //switch(winType){
        //case PZSTAT:
        //    //vdInfo = oInfo->value<ViewExtraDialog::StateInfo>();
        //    qobject_cast<ViewExtraDialog*>(dlg)->setState(oinfo);
        //    break;
        //}
    }
}


/**
 * @brief MainWindow::jzsy
 *  结转损益科目到本年利润
 * @return
 */
bool MainWindow::jzsy()
{
//    if(!isOpenPzSet){
//        pzsWarning();
//        return true;
//    }
//    if(curPzSetState != Ps_AllVerified){
//        QMessageBox::warning(0,tr("警告信息"),tr("凭证集内存在未审核凭证，不能结转！"));
//        return true;
//    }
//    if(!isExtraVolid){
//        QMessageBox::warning(0,tr("警告信息"),tr("余额无效，请重新进行统计并保存正确余额！"));
//        return true;
//    }
//    //1、检测本期和下期汇率是否有变动，如果有，则检测是否执行了结转汇兑损益，如果没有，则退出
//    QHash<int,Double> sRates,eRates;
//    int y=cursy,m=cursm;
//    curAccount->getRates(y,m,sRates);
//    if(m == 12){
//        y++;
//        m = 1;
//    }
//    else{
//        m++;
//    }
//    curAccount->getRates(y,m,eRates);

//    if(eRates.empty()){
//        QString tip = tr("下期汇率未设置，请先设置：%1年%2月美金汇率：").arg(y).arg(m);
//        bool ok;
//        double rate = QInputDialog::getDouble(0,tr("信息输入"),tip,0,0,100,2,&ok);
//        if(!ok)
//            return true;
//        eRates[USD] = Double(rate);
//        curAccount->setRates(y,m,eRates);
//    }
//    //汇率不等，则检查是否执行了结转汇兑损益
//    if(sRates.value(USD) != eRates.value(USD)){
//        int count;
//        dbUtil->inspectJzPzExist(cursy,cursm,Pzd_Jzhd,count);
//        if(count != 0){
//            QMessageBox::warning(0,tr("警告信息"),tr("未结转汇兑损益或结转汇兑损益凭证有误！"));
//            return true;
//        }
//    }

//    //删除先前存在的结转凭证，这是因为要取得结转前的正确余额
//    int count;
//    dbUtil->inspectJzPzExist(cursy,cursm,Pzd_Jzsy,count);
//    if(count < 2){
//        dbUtil->delSpecPz(cursy,cursm,Pzd_Jzsy,count);
//        dbUtil->scanPzSetCount(cursy,cursm,pzRepeal,pzRecording,pzVerify,pzInstat,pzAmount);
//        isExtraVolid = false;
//        refreshShowPzsState();
//    }

//    if(!BusiUtil::genForwordPl2(cursy,cursm,curUser)) //创建结转凭证
//        return false;
//    dbUtil->specPzClsInstat(cursy,cursm,Pzd_Jzsy,count); //将结转凭证入账
//    pzInstat+=count;
//    pzAmount+=count;
//    isExtraVolid = false;
//    refreshShowPzsState();
//    ViewExtraDialog* vdlg = new ViewExtraDialog(curAccount,cursy,cursm); //再次统计本期发生额并保存余额
//    connect(vdlg,SIGNAL(pzsExtraSaved()),this,SLOT(extraValid()));
//    if(vdlg->exec() == QDialog::Rejected){
//        disconnect(vdlg,SIGNAL(pzsExtraSaved()),this,SLOT(extraValid()));
//        delete vdlg;
//        return false;
//    }
//    disconnect(vdlg,SIGNAL(pzsExtraSaved()),this,SLOT(extraValid()));
//    delete vdlg;
//    QMessageBox::information(0,tr("提示信息"),tr("成功创建结转损益凭证！"));
//    return true;
}


/**
 * @brief MainWindow::initUndo
 *  初始化Undo服务相关的组件
 */
void MainWindow::initUndoView()
{
    if(!undoView)
        undoView = new QUndoView(this);
    if(undoStack)
        undoView->setStack(undoStack);
    if(!dockWindows.contains(TV_UNDO)){
        connect(undoView,SIGNAL(pressed(QModelIndex)),this,SLOT(undoViewItemClicked(QModelIndex)));
        QDockWidget* dw;
        dw = new QDockWidget(tr("Undo视图"),this);
        dw->setWidget(undoView);
        dw->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        addDockWidget(Qt::LeftDockWidgetArea, dw);
        dw->resize(150,200);
        dockWindows[TV_UNDO] = dw;
        tvMapper->removeMappings(tvActions.value(TV_UNDO));
        adjustViewMenus(TV_UNDO);
    }
}

/**
 * @brief MainWindow::clearUndo
 *  清除Undo相关的对象（在关闭凭证集时调用）
 */
void MainWindow::clearUndo()
{
    if(!undoView)
        return;
    disconnect(undoView,SIGNAL(pressed(QModelIndex)),this,SLOT(undoViewItemClicked(QModelIndex)));
    ui->menuEdit->removeAction(undoAction);
    ui->menuEdit->removeAction(redoAction);
    ui->tbrEdit->removeAction(undoAction);
    ui->tbrEdit->removeAction(redoAction);
    removeDockWidget(dockWindows.value(TV_UNDO));
    delete dockWindows[TV_UNDO];
    dockWindows.remove(TV_UNDO);
    //delete undoView;
    undoView = NULL;
    adjustViewMenus(TV_UNDO,true);
}

/**
 * @brief MainWindow::adjustViewMenus
 *  调整视图菜单项，用实际控制该视图的Action来代替原先用来创建该视图的Action
 * @param t         要显示的视图
 */
void MainWindow::adjustViewMenus(MainWindow::ToolViewType t, bool isRestore)
{
    int index;
    switch(t){
    case TV_UNDO:
        index = 0;
        break;
    case TV_SEARCHCLIENT:
        index = 1;
        break;
    }
    QList<QAction*> actions;
    actions = ui->mnuView->actions();
    if(isRestore){
        actions.replace(index,tvActions.value(t));
        tvMapper->setMapping(tvActions.value(t),t);
    }
    else
        actions.replace(index,dockWindows.value(t)->toggleViewAction());
    ui->mnuView->clear();
    ui->mnuView->addActions(actions);
}

/**
 * @brief MainWindow::adjustEditMenus
 *  调整编辑菜单下的Undo、Redo菜单项的实际对应的QAction
 */
void MainWindow::adjustEditMenus(UndoType ut, bool restore)
{
    QAction *undo,*redo;
    if(restore){
        undo = ui->actUndo;
        redo = ui->actRedo;
    }
    else{
        switch(ut){
        case UT_PZ:
            undo = undoStack->createUndoAction(this);
            redo = undoStack->createRedoAction(this);
            break;
        }
        undo->setIcon(QIcon(":/images/edit-undo.png"));
        redo->setIcon(QIcon(":/images/edit-redo.png"));
    }
    QList<QAction*> actions;
    actions = ui->menuEdit->actions();
    actions.replace(0,undo);
    actions.replace(1,redo);
    ui->menuEdit->clear();
    ui->menuEdit->addActions(actions);
    actions.clear();
    actions = ui->tbrEdit->actions();
    actions.replace(0,undo);
    actions.replace(1,redo);
    ui->tbrEdit->clear();
    ui->tbrEdit->addActions(actions);
}

/**
 * @brief 切换到与指定帐套对应的帐套视图
 * @param suiteId
 */
void MainWindow::switchSubWindowGroup(int suiteId)
{
    if(suiteId != 0){
        //隐藏当前帐套对应的所有子窗口

        //显示新帐套的所有已打开的子窗口
    }
}

//显示账户属性对话框
void MainWindow::on_actAccProperty_triggered()
{
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    AccountPropertyConfig* dlg = NULL;
    if(!commonGroups.contains(SUBWIN_ACCOUNTPROPERTY)){
        dbUtil->getSubWinInfo(SUBWIN_ACCOUNTPROPERTY,winfo,sinfo);
        dlg = new AccountPropertyConfig(curAccount);
    }
    showCommonSubWin(SUBWIN_ACCOUNTPROPERTY,dlg,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

/**
 * @brief MainWindow::on_actSetPzCls_triggered
 *  设置凭证类别
 */
void MainWindow::on_actSetPzCls_triggered()
{
    if(!curSuiteMgr->isOpened()){
        pzsWarning();
        return;
    }
    if(!subWindows.contains(SUBWIN_PZEDIT))
        return;

    QSqlQuery q;
    PzDialog2* pd = static_cast<PzDialog2*>(subWindows.value(SUBWIN_PZEDIT)->widget());
    int pid = pd->getCurPzId();
    QString s = QString("select %1 from %2 where id = %3")
            .arg(fld_pz_class).arg(tbl_pz).arg(pid);
    if(!q.exec(s)){
        qDebug()<<QString("Failed exec sql: %1").arg(s);
        return;
    }
    if(!q.first()){
        qDebug()<<QString("Don't find PingZheng(id = %1)").arg(pid);
        return;
    }
    PzClass c = (PzClass)q.value(0).toInt();

    QDialog* dlg = new QDialog;
    QLabel* lbl = new QLabel(tr("凭证类别"),dlg);
    QComboBox* cmb = new QComboBox(dlg);
    QList<PzClass> codes = pzClasses.keys();
    qSort(codes.begin(),codes.end());
    foreach(PzClass c, codes)
        cmb->addItem(pzClasses.value(c),(int)c);
    int index = cmb->findData(c);
    cmb->setCurrentIndex(index);
    QHBoxLayout* lh = new QHBoxLayout;
    lh->addWidget(lbl);
    lh->addWidget(cmb);
    QPushButton* btnOk = new QPushButton(tr("确定"));
    QPushButton* btnCancel = new QPushButton(tr("取消"));
    QHBoxLayout* lb = new QHBoxLayout;
    lb->addWidget(btnOk);
    lb->addWidget(btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addLayout(lh);
    lm->addLayout(lb);
    dlg->setLayout(lm);
    dlg->resize(200,300);
    connect(btnOk,SIGNAL(clicked()),dlg,SLOT(accept()));
    connect(btnCancel,SIGNAL(clicked()),dlg,SLOT(reject()));
    if(dlg->exec() == QDialog::Rejected){
        delete dlg;
        return;
    }
    c = (PzClass)cmb->itemData(cmb->currentIndex()).toInt();
    s = QString("update %1 set %2=%3 where id=%4")
            .arg(tbl_pz).arg(fld_pz_class).arg(c).arg(pid);
    if(!q.exec(s))
        qDebug()<<QString("Failed exec sql: %1").arg(s);
    delete dlg;
}

/**
 * @brief MainWindow::on_actPzErrorInspect_triggered
 *  凭证检错
 */
void MainWindow::on_actPzErrorInspect_triggered()
{
    ViewPzSetErrorForm* w = NULL;
    QByteArray* state = NULL;
    SubWindowDim* dim = NULL;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_VIEWPZSETERROR)){
        dbUtil->getSubWinInfo(SUBWIN_VIEWPZSETERROR,dim,state);
        w = new ViewPzSetErrorForm(curSuiteMgr,state);
        if(!dim){
            dim->x = 10; dim->y = 10; dim->w = 600; dim->h = 400;
        }
        connect(w,SIGNAL(reqLoation(int,int)),this,SLOT(openSpecPz(int,int)));
        subWinGroups.value(suiteId)->showSubWindow(SUBWIN_VIEWPZSETERROR,w,dim);
    }
    else{
        w = static_cast<ViewPzSetErrorForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_VIEWPZSETERROR));
        w->inspect();
        subWinGroups.value(suiteId)->showSubWindow(SUBWIN_VIEWPZSETERROR,w,dim);
    }
}

//强制删除锁定凭证（批量删除，凭证号之间用逗号隔开，连续的凭证号可以用连字符“-”连接，比如“2-4，7，8”）
//实现中，在删除后未考虑对凭证号进行重置。
void MainWindow::on_actForceDelPz_triggered()
{
    if(!curSuiteMgr->isOpened()){
        pzsWarning();
        return;
    }
    QDialog* dlg = new QDialog(this);
    QLabel* ly = new QLabel(tr("凭证所属年月"),dlg);
    QDateEdit* date = new QDateEdit(dlg);
    date->setDate(QDate(cursy,cursm,1));
    date->setDisplayFormat("yyyy-M");
    date->setReadOnly(false);
    QLabel* ln = new QLabel(tr("凭证号"),dlg);
    QLineEdit* edtPzNums = new QLineEdit(dlg);
    QPushButton* btnOk = new QPushButton(tr("确定"));
    QPushButton* btnCancel = new QPushButton(tr("取消"));
    QHBoxLayout* ld = new QHBoxLayout;
    QHBoxLayout* lb = new QHBoxLayout;
    QHBoxLayout* ls = new QHBoxLayout;
    QVBoxLayout* lm = new QVBoxLayout;
    ld->addWidget(ly);
    ld->addWidget(date);
    ls->addWidget(ln);
    ls->addWidget(edtPzNums);
    lb->addWidget(btnOk);
    lb->addWidget(btnCancel);
    lm->addLayout(ld);
    lm->addLayout(ls);
    lm->addLayout(lb);
    dlg->setLayout(lm);
    connect(btnOk,SIGNAL(clicked()),dlg,SLOT(accept()));
    connect(btnCancel, SIGNAL(clicked()),dlg, SLOT(reject()));
    if(QDialog::Accepted != dlg->exec())
        return;
    QString ds = date->date().toString(Qt::ISODate);
    ds.chop(3);
    QStringList pzs = edtPzNums->text().split(",");
    if(pzs.empty())
        return;

    QList<int> pzNums;
    bool ok = false;
    int numStart,numEnd;
    foreach(QString s, pzs){
        s =  s.trimmed();
        numStart = s.toInt(&ok);
        if(ok)
            pzNums << numStart;
        else{
            int index = s.indexOf("-");
            if(index != -1){
                numStart = s.left(index).toInt(&ok);
                if(!ok){
                    qDebug()<<QString("String '%1' transform to number failed!").arg(s.left(index+1));
                    return;
                }
                numEnd = s.right(s.size()-index-1).toInt(&ok);
                if(!ok){
                    qDebug()<<QString("String '%1' transform to number failed!").arg(s.right(s.size()-index-1));
                    return;
                }
                for(int i = numStart; i <= numEnd; ++i)
                    pzNums<<i;
            }
        }
    }


    delete dlg;

    QString pzNumStr;
    foreach(int n,pzNums)
        pzNumStr.append(QString::number(n)).append(",");
    pzNumStr.chop(1);
    if(QMessageBox::No == QMessageBox::warning(this,
           tr("警告信息"),tr("确定要删除 %1 %2号凭证吗？").arg(ds).arg(pzNumStr),
           QMessageBox::Yes|QMessageBox::No))
        return;

    QSqlQuery qp,qb;
    QString sp,sb;
    foreach(int pzNum,pzNums){
        sp = QString("select id from PingZhengs where date like '%1%' and number = %2")
                .arg(ds).arg(pzNum);
        if(!qp.exec(sp)){
            qDebug()<<QString("Excute stationment '%1' failed!").arg(sp);
            return;
        }
        int pid;
        while(qp.next()){
            pid = qp.value(0).toInt();
            sb = QString("delete from BusiActions where pid = %1").arg(pid);
            if(!qb.exec(sb)){
                qDebug()<<QString("Excute stationment '%1' failed!").arg(sb);
                return;
            }
            sb = QString("delete from PingZhengs where id = %1").arg(pid);
            if(!qb.exec(sb)){
                qDebug()<<QString("Excute stationment '%1' failed!").arg(sb);
                return;
            }
        }
    }
    //isExtraVolid = false;
    QMessageBox::information(0,tr("提示信息"),tr("成功删除凭证！"));
}

/**
 * @brief MainWindow::allPzToRecording
 *  所有凭证回到录入态
 * @param year
 * @param month
 */
void MainWindow::allPzToRecording(int year, int month)
{
    QSqlQuery q;
    QString s;

    QString ds = QDate(year,month,1).toString(Qt::ISODate);
    ds.chop(3);
    s = QString("update PingZhengs set pzState=%1 where pzState != %2"
                " and date like '%3%'")
            .arg(Pzs_Recording).arg(Pzs_Repeal).arg(ds);
    if(!q.exec(s))
        return;
    //BusiUtil::setPzsState(y,m,Ps_JzhdV);
}

void MainWindow::on_actViewLog_triggered()
{
    LogView* lv = new LogView(this);
    QMdiSubWindow* sw = ui->mdiArea->addSubWindow(lv);
    sw->setAttribute(Qt::WA_DeleteOnClose);
    connect(lv,SIGNAL(onClose()),sw,SLOT(close()));
    sw->show();
}


bool MainWindow::impTestDatas()
{
//    SubjectManager* subMgr = curAccount->getSubjectManager();
//    //SubjectNameItem* ni = new SubjectNameItem(0,2,"TestNI","Test Name Item","ti",QDateTime::currentDateTime(),curUser);
//    SubjectNameItem* ni = subMgr->getNameItem(583);
//    ni->setShortName("TestNI_changed");
//    ni->setClassId(5);
//    ni->setRemCode("tttt");
//    ni->setLongName("test name item changed");
//    //SecondSubject* ssub = new SecondSubject(subMgr->getBankSub(),0,ni,"01",100,true,QDateTime::currentDateTime(),QDateTime::currentDateTime(),curUser);
//    SecondSubject* ssub = subMgr->getBankSub()->getChildSub(ni);
//    ssub->setParent(subMgr->getCashSub());
//    ssub->setCode("22");
//    ssub->setEnabled(false);
//    ssub->setDisableTime(QDateTime::currentDateTime());
//    dbUtil->saveSndSubject(ssub);
//    QVariant v;
    //v.setValue(false);
    //curAccount->getDbUtil()->setCfgVariable("boolValue",v);
    //v.setValue(455);
    //curAccount->getDbUtil()->setCfgVariable("integerValue",v);
    //v.setValue(4.666);
    //curAccount->getDbUtil()->setCfgVariable("floatValue",v);
    //v.setValue(QString("test string modify!"));
    //curAccount->getDbUtil()->setCfgVariable("stringValue",v);
//    curAccount->getDbUtil()->getCfgVariable("boolValue",v);
//    curAccount->getDbUtil()->getCfgVariable("integerValue",v);
//    curAccount->getDbUtil()->getCfgVariable("floatValue",v);
//    curAccount->getDbUtil()->getCfgVariable("stringValue",v);
//    bool completed,subCloned;
//    curAccount->isCompleteSubSysCfg(1,2,completed,subCloned);
//    curAccount->setCompleteSubSysCfg(1,2,completed,subCloned);

//    DatabaseAccessForm* dlg = new DatabaseAccessForm(curAccount,AppConfig::getInstance());
//    dlg->show();
    int i = 0;
}






