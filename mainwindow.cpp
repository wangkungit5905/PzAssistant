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
#include <QProgressBar>
//#include <QToolTip>

#include <QBuffer>

#include "mainwindow.h"
#include "tables.h"
#include "ui_mainwindow.h"
#include "connection.h"
#include "global.h"
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
#include "showdzdialog.h"
#include "pzdialog.h"
#include "statements.h"
#include "accountpropertyconfig.h"
#include "suiteswitchpanel.h"
#include "nabaseinfodialog.h"
#include "commands.h"
#include "importovaccdlg.h"
#include "optionform.h"
#include "taxescomparisonform.h"
#include "seccondialog.h"
#include "tools/notemgrform.h"
#include "tools/externaltoolconfigform.h"
#include "myhelper.h"

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

/////////////////////////////////PaStatusBar////////////////////////////////
PaStatusBar::PaStatusBar(QWidget *parent):QStatusBar(parent)
{
    timer = NULL;
    QLabel *l = new QLabel(tr("当前帐套:"),this);
    curSuite.setText("         ");
    curSuite.setObjectName("InfoSecInStatus");
    QHBoxLayout* hl1 = new QHBoxLayout(this);
    hl1->addWidget(l);
    hl1->addWidget(&curSuite);
    l = new QLabel(tr("日期:"),this);
    pzSetDate.setObjectName("InfoSecInStatus");
    pzSetDate.setText("                       ");    
    hl1->addWidget(l);
    hl1->addWidget(&pzSetDate);
    l = new QLabel(tr("凭证集状态："),this);
    pzSetState.setObjectName("InfoSecInStatus");
    pzSetState.setText("              ");

    QHBoxLayout* hl2 = new QHBoxLayout(this);
    hl2->addWidget(l);    hl2->addWidget(&pzSetState);
    l = new QLabel(tr("余额状态："),this);
    extraState.setObjectName("InfoSecInStatus");
    hl2->addWidget(l);    hl2->addWidget(&extraState);

    l = new QLabel(tr("凭证总数:"),this);
    pzCount.setAttribute(Qt::WA_AlwaysShowToolTips,true);
    pzCount.setObjectName("InfoSecInStatus");
    pzCount.setText("   ");
    QHBoxLayout* hl3 = new QHBoxLayout(this);
    hl3->addWidget(l);
    hl3->addWidget(&pzCount);

    l = new QLabel(tr("登录用户:"),this);
    lblUser.setObjectName("InfoSecInStatus");
    lblUser.setText("           ");
    QHBoxLayout* hl4 = new QHBoxLayout(this);
    hl4->addWidget(l);   hl4->addWidget(&lblUser);

    QHBoxLayout* ml = new QHBoxLayout(this);
    ml->addLayout(hl1);ml->addLayout(hl2);
    ml->addLayout(hl3);ml->addLayout(hl4);
    QFrame *f = new QFrame(this);
    f->setObjectName("FrameInStatus");
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

    AppConfig::getInstance()->getCfgVar(AppConfig::CVC_TimeoutOfTemInfo,_timeout);
}

PaStatusBar::~PaStatusBar()
{
    delete pBar;
}

void PaStatusBar::setCurSuite(QString suiteName)
{
    curSuite.setText(suiteName);
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
    QString ds = tr("%1月1日——%1月%2日").arg(m).arg(dend);
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
    num_rec = recording;
    num_ver = verify;
    num_ins = instat;
    num_rep = repeal;
    setPzAmount(amount);
    adjustPzCountTip();

}

void PaStatusBar::resetPzCounts()
{
    setPzAmount(0);
    adjustPzCountTip();
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
        timer->setInterval(_timeout);
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
 * @brief PaStatusBar::adjustPzCountTip
 *  调整凭证集内各类凭证数的提示信息
 * @param rec
 * @param ver
 * @param ins
 * @param rep
 */
void PaStatusBar::adjustPzCountTip()
{
    QString tip;
    if(num_rec==0 && num_ver==0 && num_ins==0 && num_rep==0)
        tip = tr("没有任何凭证");
    else{
        if(num_rec > 0)
            tip.append(tr("录入：%1\n").arg(num_rec));
        if(num_ver > 0)
            tip.append(tr("审核：%1\n").arg(num_ver));
        if(num_ins > 0)
            tip.append(tr("入账：%1\n").arg(num_ins));
        if(num_rep>0)
            tip.append(tr("作废：%1").arg(num_rep));
    }
    pzCount.setToolTip(tip);
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

void SubWinGroupMgr::closeAll()
{
    if(subWinHashs.isEmpty())
        return;
    QHashIterator<subWindowType,MyMdiSubWindow*> it(subWinHashs);
    while(it.hasNext()){
        it.next();
        it.value()->close();
        parent->removeSubWindow(it.value());
        delete it.value();
    }
    subWinHashs.clear();
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
    if(t == SUBWIN_PZEDIT){
        PzDialog* w = static_cast<PzDialog*>(subWin->widget());
        state = w->getState();
        disconnect(w,SIGNAL(showMessage(QString,AppErrorLevel)),this,SLOT(showRuntimeInfo(QString,AppErrorLevel)));
        disconnect(w,SIGNAL(selectedBaChanged(QList<int>,bool)),this,SLOT(baSelectChanged(QList<int>,bool)));
        disconnect(w,SIGNAL(rateChanged(int)),this,SLOT(rateChanged(int)));
        delete w;
    }
    else if(t == SUBWIN_HISTORYVIEW){
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWin->widget());
        state = w->getState();
        delete w;
    }
    else if(t == SUBWIN_DETAILSVIEW){
        ShowDZDialog* w = static_cast<ShowDZDialog*>(subWin->widget());
        state = w->getState();
        disconnect(w,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
        delete w;
    }
    else if(t == SUBWIN_PZSTAT){
        CurStatDialog* w = static_cast<CurStatDialog*>(subWin->widget());
        state = w->getState();
        delete w;
    }
    else if(t == SUBWIN_VIEWPZSETERROR){
        ViewPzSetErrorForm* w = static_cast<ViewPzSetErrorForm*>(subWin->widget());
        state = w->getState();
        disconnect(w,SIGNAL(reqLoation(int,int)),this,SLOT(openSpecPz(int,int)));
        delete w;
    }
    if(curAccount)
        curAccount->getDbUtil()->saveSubWinInfo(t,dim,state);
    subWinHashs.remove(subWin->getWindowType());
    delete dim;
    if(state)
        delete state;    
    parent->removeSubWindow(subWin);
    emit specSubWinClosed(t);
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
    sortBy = true; //默认按凭证号进行排序
    dbUtil = NULL;
    undoStack = NULL;
    undoView = NULL;
    curSSPanel = NULL;
    etMapper = NULL;

    //setStyleSheet("QMainWindow::separator:hover {background: red;}");


    appCon = AppConfig::getInstance();
    initActions();
    initToolBar();
    initTvActions();
    initExternalTools();

    mdiAreaWidth = ui->mdiArea->width();
    mdiAreaHeight = ui->mdiArea->height();


    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)),
            this, SLOT(setActiveSubWindow(QWidget*)));
    connect(ui->mdiArea,SIGNAL(subWindowActivated(QMdiSubWindow*)),this,SLOT(subWindowActivated(QMdiSubWindow*)));


    on_actLogin_triggered(); //显示登录窗口
    if(!curUser)
        return;

    bool ok = true;
    AppConfig* appCfg = AppConfig::getInstance();
    AccountCacheItem* ci = appCfg->getRecendOpenAccount();
    if(ci){
        if(!AccountVersionMaintain(ci->fileName)){
            setWindowTitle(QString("%1---%2").arg(appTitle)
                           .arg(tr("无账户被打开")));
            return;
        }        
        curAccount = new Account(ci->fileName);
        if(!curAccount->isValid()){
            QMessageBox::warning(this,"",tr("账户文件无效，请检查账户文件内信息是否齐全！！"));
            ok = false;
        }
        else if(!curAccount->canAccess(curUser)){
            QMessageBox::warning(this,"",tr("当前登录用户不能访问账户（%1），请以合适的用户登录！").arg(curAccount->getSName()));
            ok = false;
        }
    }
    else
        ok = false;
    if(!ok){
        if(curAccount)
            delete curAccount;
        curAccount = NULL;
        setWindowTitle(QString("%1---%2").arg(appTitle)
                       .arg(tr("无账户被打开")));
        return;
    }
    accountInit(ci);
    setWindowTitle(QString("%1---%2").arg(appTitle).arg(curAccount->getLName()));
    rfMainAct();
    refreshShowPzsState();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//获取主窗口的Mdi区域的大小（必须在主窗口显示后调用）
void MainWindow::getMdiAreaSize(int &mdiAreaWidth, int &mdiAreaHeight)
{
    mdiAreaWidth = ui->mdiArea->width();
    mdiAreaHeight = ui->mdiArea->height();
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
    undoAction = ui->actUndo;
    redoAction = ui->actRedo;
    //文件菜单
    connect(ui->actCrtAccount, SIGNAL(triggered()), this, SLOT(newAccount()));
    connect(ui->actOpenAccount, SIGNAL(triggered()), this, SLOT(openAccount()));
    connect(ui->actCloseAccount, SIGNAL(triggered()), this, SLOT(closeAccount()));

    connect(ui->actExit,SIGNAL(triggered()),this,SLOT(exit()));

    //数据菜单
    connect(ui->actSubExtra, SIGNAL(triggered()), this, SLOT(viewSubjectExtra()));

    //报表
    connect(ui->actBalanceSheet, SIGNAL(triggered()), this, SLOT(generateBalanceSheet()));
    connect(ui->actIncomeStatements, SIGNAL(triggered()), this, SLOT(generateIncomeStatements()));

    //帐务处理

    //工具菜单
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

    btnPrint = new QToolButton;
    btnPrint->setPopupMode(QToolButton::InstantPopup);
    QIcon icon(":/images/printer.png");
    btnPrint->setIcon(icon);
    btnPrint->setEnabled(false);
    actPrintToPrinter = new QAction(tr("打印"),this);
    connect(actPrintToPrinter,SIGNAL(triggered()),this,SLOT(printProcess()));
    actPrintPreview = new QAction(tr("打印预览"),this);
    connect(actPrintPreview,SIGNAL(triggered()),this,SLOT(printProcess()));
    actPrintToPDF = new QAction(tr("打印到PDF文件"),this);
    connect(actPrintToPDF,SIGNAL(triggered()),this,SLOT(printProcess()));
    actOutputToExcel = new QAction(tr("输出到Excel文件"),this);
    connect(actOutputToExcel,SIGNAL(triggered()),this,SLOT(printProcess()));
    btnPrint->addAction(actPrintToPrinter);
    btnPrint->addAction(actPrintPreview);
    btnPrint->addAction(actPrintToPDF);
    btnPrint->addAction(actOutputToExcel);
    ui->tbrMain->addWidget(btnPrint);

    //ui->tbrMain->setStyleSheet("{background: red;spacing: 20px;}");
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

    QDockWidget* dw = new QDockWidget(tr("帐套切换视图"), this);
    addDockWidget(Qt::LeftDockWidgetArea, dw);
    dockWindows[TV_SUITESWITCH] = dw;
    dw->hide();
}

void MainWindow::initExternalTools()
{
    if(!etMapper){
        etMapper = new QSignalMapper(this);
        connect(etMapper,SIGNAL(mapped(int)),this,SLOT(startExternalTool(int)));
    }

    QList<QAction*> actions = ui->mnuExternalTools->actions();
    if(actions.count() > 1){
        for(int i = actions.count()-1; i > 0 ; i--){
            delete actions.at(i);
            actions.removeLast();
        }
    }
    if(eTools.isEmpty())
        appCon->readAllExternalTools(eTools);
    int i = 0;
    foreach(ExternalToolCfgItem* tool,eTools){
        if(tool->id == 0){
            eTools.removeOne(tool);
            continue;
        }
        QAction* act = new QAction(eTools.at(i)->name,this);
        etMapper->setMapping(act,i);
        connect(act,SIGNAL(triggered()),etMapper,SLOT(map()));
        actions<<act;
        i++;
    }
    ui->mnuExternalTools->addActions(actions);
}



//打开账户时的初始化设置工作
void MainWindow::accountInit(AccountCacheItem* ci)
{
    dbUtil = curAccount->getDbUtil();
    curSuiteMgr = curAccount->getSuiteMgr();
    if(ci->tState != ATS_TRANSINDES)
        curAccount->setReadOnly(true);
    if(!curSuiteMgr)
        QMessageBox::warning(this,tr("友情提示"),tr("本账户还没有设置任何帐套，请在账户属性设置窗口的帐套页添加一个帐套以供帐务处理！"));
    else
        suiteViewSwitched(NULL,curSuiteMgr);    
    undoStack = curSuiteMgr?curSuiteMgr->getUndoStack():NULL;
    if(!BusiUtil::init(curAccount->getDbUtil()->getDb()))
        QMessageBox::critical(this,tr("错误信息"),tr("BusiUtil对象初始化阶段发生错误！"));
    AccountSuiteRecord* curSuite = curAccount->getCurSuiteRecord();
    if(curSuite && !subWinGroups.contains(curSuite->id)){
        SubWinGroupMgr* grpMgr = new SubWinGroupMgr(curSuite->id,ui->mdiArea);
        subWinGroups[curSuite->id] = grpMgr;
        connect(grpMgr,SIGNAL(specSubWinClosed(subWindowType)),this,SLOT(specSubWinClosed(subWindowType)));
    }

    curSSPanel = new SuiteSwitchPanel(curAccount);
    connect(curSSPanel,SIGNAL(selectedSuiteChanged(AccountSuiteManager*,AccountSuiteManager*)),
            this,SLOT(suiteViewSwitched(AccountSuiteManager*,AccountSuiteManager*)));
    connect(curSSPanel,SIGNAL(viewPzSet(AccountSuiteManager*,int)),this,SLOT(viewOrEditPzSet(AccountSuiteManager*,int)));
    connect(curSSPanel,SIGNAL(pzSetOpened(AccountSuiteManager*,int)),this,SLOT(pzSetOpen(AccountSuiteManager*,int)));
    connect(curSSPanel,SIGNAL(prepareClosePzSet(AccountSuiteManager*,int)),this,SLOT(prepareClosePzSet(AccountSuiteManager*,int)));
    connect(curSSPanel,SIGNAL(pzsetClosed(AccountSuiteManager*,int)),this,SLOT(pzSetClosed(AccountSuiteManager*,int)));
    dockWindows.value(TV_SUITESWITCH)->setWidget(curSSPanel);
}

//动态更新窗口菜单
void MainWindow::updateWindowMenu()
{
    ui->mnuWindow->clear();
    ui->mnuWindow->addAction(ui->actCloseCurWindow);
    ui->mnuWindow->addAction(ui->actCloseAllWindow);
    bool nonSubWin = true;
    if(curSuiteMgr){
        int key = curSuiteMgr->getSuiteRecord()->id;
        if(subWinGroups.contains(key) && subWinGroups.value(key)->count()>0){
            addSubWindowMenuItem(subWinGroups.value(key)->getAllSubWindows());
            nonSubWin = false;
        }
    }
    if(!commonGroups.isEmpty()){
        addSubWindowMenuItem(commonGroups.values());
        nonSubWin = false;
    }
    if(!commonGroups_multi.isEmpty()){
        addSubWindowMenuItem(commonGroups_multi.values());
        nonSubWin = false;
    }
    ui->actCloseAllWindow->setEnabled(!nonSubWin);
    ui->actCloseCurWindow->setEnabled(!nonSubWin);

}

//返回当前活动的子窗口的类型
subWindowType MainWindow::activedMdiChild()
{
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(ui->mdiArea->activeSubWindow());
    if(w)
        return w->getWindowType();
    else
        return SUBWIN_NONE;
}

void MainWindow::addSubWindowMenuItem(QList<MyMdiSubWindow *> windows)
{
    ui->mnuWindow->addSeparator();
    foreach(MyMdiSubWindow* w, windows){
        QAction* action = ui->mnuWindow->addAction(w->windowTitle());
        connect(action,SIGNAL(triggered()),windowMapper,SLOT(map()));
        windowMapper->setMapping(action,w);
    }
}

/**
 * @brief 当前帐套是否可编辑
 * @return
 */
bool MainWindow::isSuiteEditable()
{
    if(!curAccount)
        return false;
    if(!curSuiteMgr)
        return false;
    bool readOnly = curAccount->isReadOnly() || curSuiteMgr->isSuiteClosed();
    return !readOnly;
}

/**
 * @brief 凭证集是否可编辑
 * @return
 */
bool MainWindow::isPzSetEditable()
{
    if(!isSuiteEditable())
        return false;
    PzsState state = curSuiteMgr->getState();
    return (state == Ps_Rec) || (state == Ps_AllVerified);
}

/**
 * @brief 当前凭证是否可编辑
 * @return
 */
bool MainWindow::isPzEditable()
{
    if(!isPzSetEditable())
        return false;
    PingZheng* pz = curSuiteMgr->getCurPz();
    return pz && (pz->getPzState() == Pzs_Recording);
}


/**
 * @brief MainWindow::rfLogin
 *  控制与用户登录有关的菜单项可用性
 * @param login
 */
void MainWindow::rfLogin()
{
    bool login = curUser?true:false;
    ui->actLogin->setEnabled(!login);
    ui->actLogout->setEnabled(login);
    ui->actShiftUser->setEnabled(login);
    //如果当前登录的root用户，则启用安全配置菜单项，目前未实现，暂不启用
    bool r = login && curUser->isSuperUser();
    ui->actSecCon->setEnabled(r);
    ui->actSqlTool->setEnabled(r);
    rfMainAct();
}

/**
 * @brief MainWindow::rfMainAct
 *  控制账户相关的命令的启用性（在账户打开或关闭时刻调用）
 */
void MainWindow::rfMainAct()
{
    bool open = curAccount?true:false;
    ui->actCrtAccount->setEnabled(!open);
    ui->actOpenAccount->setEnabled(!open);
    ui->actCloseAccount->setEnabled(open);
    ui->actAccProperty->setEnabled(open);
    ui->actRefreshActInfo->setEnabled(!open);
    ui->actDelAcc->setEnabled(!open);
    ui->actInAccount->setEnabled(!open);
    ui->actEmpAccount->setEnabled(!open);
    rfSuiteAct();
}

/**
 * @brief 控制当帐套切换后，菜单项的启用性
 */
void MainWindow::rfSuiteAct()
{
    bool open = curSuiteMgr?true:false;
    ui->actSuite->setEnabled(open);         //帐套切换面板
    ui->actSearch->setEnabled(open);        //凭证搜索
    ui->actSubExtra->setEnabled(open);      //科目余额
    ui->actDetailView->setEnabled(open);    //明细账
    ui->actShowTotal->setEnabled(open);     //总账
    rfPzSetOpenAct();
}

/**
 * @brief 控制凭证集打开/或关闭时，菜单项的启用性
 * @param open
 */
void MainWindow::rfPzSetOpenAct()
{

    bool open = (curSuiteMgr && curSuiteMgr->isPzSetOpened())?true:false;
    ui->actNaviToPz->setEnabled(open);      //凭证定位
    ui->actCurStatNew->setEnabled(open);    //本期统计
    ui->actPzErrorInspect->setEnabled(open);//凭证集错误检测
    rfPzSetStateAct();
}

/**
 * @brief 按凭证集的当前状态来控制与其相关的菜单项的启用性
 */
void MainWindow::rfPzSetStateAct()
{
    PzsState state = curSuiteMgr?curSuiteMgr->getState():Ps_NoOpen;
    bool open = curSuiteMgr?curSuiteMgr->isPzSetOpened():false;
    bool editable = isSuiteEditable() && open && (state != Ps_Jzed);
    bool r = editable && (state == Ps_Rec);
    ui->actAllVerify->setEnabled(r);     //全部审核
    ui->actAllInstat->setEnabled(r);     //全部入账
    ui->actReassignPzNum->setEnabled(r); //重置凭证号

    r = editable && (state == Ps_AllVerified) && (curSuiteMgr->getExtraState());
    ui->actFordEx->setEnabled(r);   //结转汇兑损益
    ui->actFordPl->setEnabled(r);   //结转损益
    ui->actJzbnlr->setEnabled(r);   //结转本年利润
    ui->actEndAcc->setEnabled(r);   //结账
    ui->actAntiJz->setEnabled(r);   //反结转
    ui->actImpOtherPz->setEnabled(r); //引入其他凭证
    ui->actAntiImp->setEnabled(r);  //反引用
    ui->actAntiJz->setEnabled(r);   //反结转

    r = (isSuiteEditable() && open && (state == Ps_Jzed));
    ui->actAntiEndAcc->setEnabled(r);    //反结账

    rfPzSetEditAct(editable);
}

/**
 * @brief 控制对凭证集内凭证进行增、删、改等操作菜单项的启用性
 *  当凭证的状态发生改变时，要调用它
 * @param editable  凭证集是否可编辑
 */
void MainWindow::rfPzSetEditAct(bool editable)
{
    bool en= editable && (activedMdiChild() == SUBWIN_PZEDIT);
    ui->actAddPz->setEnabled(en);                                   //添加凭证
    ui->actInsertPz->setEnabled(en);                                //插入凭证
    PingZheng* curPz = curSuiteMgr?curSuiteMgr->getCurPz():NULL;
    ui->actDelPz->setEnabled(en && curPz);                          //删除凭证
    PzState pzState = curPz?curPz->getPzState():Pzs_NULL;
    rfPzStateEditAct(en,pzState);
//    ui->actRepealPz->setEnabled(en && (pzState == Pzs_Recording));  //作废凭证
//    ui->actAntiRepeat->setEnabled(en && (pzState == Pzs_Repeal));	//取消作废
//    ui->actVerifyPz->setEnabled(en && (pzState == Pzs_Recording));  //审核凭证
//    ui->actInStatPz->setEnabled(en && (pzState == Pzs_Verify));     //凭证入账
    ui->actAntiVerify->setEnabled((pzState == Pzs_Verify) || (pzState == Pzs_Instat));//取消审核
    rfBaEditAct();
    rfPzNaviAct();
}

/**
 * @brief 控制改变凭证状态的按钮启用性
 * @param Editable  凭证集是否可编辑
 * @param state     当前凭证状态
 */
void MainWindow::rfPzStateEditAct(bool Editable, PzState state)
{
    ui->actRepealPz->setEnabled(Editable && (state == Pzs_Recording));  //作废凭证
    ui->actAntiRepeat->setEnabled(Editable && (state == Pzs_Repeal));   //取消作废
    ui->actVerifyPz->setEnabled(Editable && (state == Pzs_Recording));  //审核凭证
    ui->actInStatPz->setEnabled(Editable && (state == Pzs_Verify));     //凭证入账
    ui->actAntiVerify->setEnabled(Editable && (state == Pzs_Verify || state == Pzs_Instat));
}

/**
 * @brief 控制凭证导航按钮的启用性
 */
void MainWindow::rfPzNaviAct()
{
    subWindowType type = activedMdiChild();
    bool open = curSuiteMgr && curSuiteMgr->isPzSetOpened();
    bool en = false;
    bool isEmpty = false;
    bool isFirst = false;
    bool isLast = false;
    //bool isPrint = false;
    if(type == SUBWIN_PZEDIT){
        //isPrint = true;
        en = open;
        isEmpty = (!open || curSuiteMgr->getPzCount() == 0);
        isFirst = open && curSuiteMgr->isFirst();
        isLast = open && curSuiteMgr->isLast();
    }
    else if(type == SUBWIN_HISTORYVIEW){
        en = true;
        //isPrint = true;
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        isEmpty = historyPzSet.value(suiteId).isEmpty();
        isFirst = historyPzSetIndex.value(suiteId) == 0;
        isLast = historyPzSetIndex.value(suiteId) == historyPzSet.value(suiteId).count()-1;
    }
    ui->actGoFirst->setEnabled(en && !isEmpty && !isFirst);
    ui->actGoLast->setEnabled(en && !isEmpty && !isLast);
    ui->actGoNext->setEnabled(en && !(isEmpty || isLast));
    ui->actGoPrev->setEnabled(en && !(isEmpty || isFirst));
    //btnPrint->setEnabled(isPrint);
}

/**
 * @brief 控制分录编辑操作的启用性
 */
void MainWindow::rfBaEditAct()
{
    bool en = isPzEditable()  && (activedMdiChild() == SUBWIN_PZEDIT);
    if(!en){
        ui->actAddAction->setEnabled(false);
        ui->actInsertBa->setEnabled(false);
        ui->actDelAction->setEnabled(false);
        ui->actMvUpAction->setEnabled(false);
        ui->actMvDownAction->setEnabled(false);
    }
    else{
        ui->actAddAction->setEnabled(true);
        PingZheng* pz = curSuiteMgr->getCurPz();
        bool isSel = pz->getCurBa()?true:false;
        ui->actInsertBa->setEnabled(isSel);
        ui->actDelAction->setEnabled(isSel);
        ui->actMvUpAction->setEnabled(isSel && !pz->isFirst());
        ui->actMvDownAction->setEnabled(isSel && !pz->isLast());
    }
}

/**
 * @brief 控制保存按钮的启用性
 */
void MainWindow::rfSaveBtn()
{
    //bool en = (curSuiteMgr && !curSuiteMgr->getUndoStack()->isClean())?true:false;
    bool en = (curSuiteMgr && curSuiteMgr->isDirty())?true:false;
    ui->actSave->setEnabled(en);
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
    NABaseInfoDialog dlg(this);
    if(dlg.exec() == QDialog::Rejected)
        return;

}

void MainWindow::openAccount()
{
    OpenAccountDialog* dlg = new OpenAccountDialog(this);
    if(dlg->exec() != QDialog::Accepted)
        return;

    AccountCacheItem* ci =dlg->getAccountCacheItem();
    if(!ci || !AccountVersionMaintain(ci->fileName)){
        setWindowTitle(QString("%1---%2").arg(appTitle)
                       .arg(tr("无账户被打开")));
        rfMainAct();
        return;
    }
    if(curAccount){
        delete curAccount;
        curAccount = NULL;
    }
    bool ok = true;
    curAccount = new Account(ci->fileName);
    if(!curAccount->isValid()){
        QMessageBox::warning(this,"",tr("账户文件无效，请检查账户文件内信息是否齐全！！"));
        ok = false;
    }
    else if(!curAccount->canAccess(curUser)){
        QMessageBox::warning(this,"",tr("当前登录用户不能访问该账户（%1），请以合适的用户登录！").arg(curAccount->getSName()));
        ok = false;
    }
    if(!ok){
        if(curAccount)
            delete curAccount;
        curAccount = NULL;
        setWindowTitle(QString("%1---%2").arg(appTitle).arg(tr("无账户被打开")));
        rfMainAct();
        return;
    }
    setWindowTitle(QString("%1---%2").arg(appTitle).arg(curAccount->getLName()));
    AppConfig::getInstance()->setRecentOpenAccount(ci->code);
    accountInit(ci);
    rfMainAct();
}

void MainWindow::closeAccount()
{
    //检查是否有未保存的修改
    if(!subWinGroups.isEmpty()){
        foreach(int asrId, subWinGroups.keys()){
            AccountSuiteManager* sm = curAccount->getSuiteMgr(asrId);
            if(sm->isDirty()){
                if(QMessageBox::Yes == QMessageBox::warning(this,tr("保存提醒"),tr("帐套“%1”已被修改，需要保存吗？").arg(sm->getSuiteRecord()->name),
                                                            QMessageBox::Yes|QMessageBox::No))
                    sm->save();
            }
        }
    }
    //关闭所有属于该账户的子窗口
    if(!commonGroups.isEmpty()){
        QHashIterator<subWindowType,MyMdiSubWindow*> it(commonGroups);
        while(it.hasNext()){
            it.next();
            it.value()->close();
        }
        commonGroups.clear();
    }
    if(!commonGroups_multi.isEmpty()){
        QHashIterator<subWindowType,MyMdiSubWindow*> it(commonGroups_multi);
        while(it.hasNext()){
            it.next();
            it.value()->close();
        }
        commonGroups_multi.clear();
    }
    if(!subWinGroups.isEmpty()){
        QHashIterator<int,SubWinGroupMgr*> it(subWinGroups);
        while(it.hasNext()){
            it.next();
            it.value()->closeAll();
            disconnect(it.value(),SIGNAL(specSubWinClosed(subWindowType)),this,SLOT(specSubWinClosed(subWindowType)));
            delete it.value();
        }
        subWinGroups.clear();
    }

    //释放所有与该账户相关的资源
    if(!historyPzSet.isEmpty()){
        QHashIterator<int,QList<PingZheng*> > it(historyPzSet);
        while(it.hasNext()){
            it.next();
            qDeleteAll(it.value());
        }
        historyPzSet.clear();
    }
    if(!historyPzSetIndex.isEmpty())
        historyPzSetIndex.clear();
    if(!historyPzMonth.isEmpty())
        historyPzMonth.clear();

    //如果帐套切换面板正显示，则关闭
    if(dockWindows.value(TV_SUITESWITCH)->isVisible())
        dockWindows.value(TV_SUITESWITCH)->hide();
    if(curSSPanel){
        disconnect(curSSPanel,SIGNAL(selectedSuiteChanged(AccountSuiteManager*,AccountSuiteManager*)),
                this,SLOT(suiteViewSwitched(AccountSuiteManager*,AccountSuiteManager*)));
        disconnect(curSSPanel,SIGNAL(viewPzSet(AccountSuiteManager*,int)),this,SLOT(viewOrEditPzSet(AccountSuiteManager*,int)));
        disconnect(curSSPanel,SIGNAL(pzSetOpened(AccountSuiteManager*,int)),this,SLOT(pzSetOpen(AccountSuiteManager*,int)));
        disconnect(curSSPanel,SIGNAL(prepareClosePzSet(AccountSuiteManager*,int)),this,SLOT(prepareClosePzSet(AccountSuiteManager*,int)));
        disconnect(curSSPanel,SIGNAL(pzsetClosed(AccountSuiteManager*,int)),this,SLOT(pzSetClosed(AccountSuiteManager*,int)));
        delete curSSPanel;
        curSSPanel = NULL;
    }

    if(curSuiteMgr && curSuiteMgr->isPzSetOpened())
        curSuiteMgr->closePzSet();

    curAccount->setLastAccessTime(QDateTime::currentDateTime());
    curAccount->close();

    undoStack = NULL;
    curSuiteMgr = NULL;
    QAction* action = qobject_cast<QAction*>(sender());
    if(action && action == ui->actCloseAccount)
        AppConfig::getInstance()->clearRecentOpenAccount();
    delete curAccount;
    curAccount = NULL;
    setWindowTitle(tr("会计凭证处理系统---无账户被打开"));
    rfMainAct();
}

/**
 * @brief 移除账户
 */
void MainWindow::on_actDelAcc_triggered()
{
    //显示当前账户列表供用户选择,将选择的账户从账户缓存中删除
    AppConfig* appCfg = AppConfig::getInstance();
    QList<AccountCacheItem*> accItems = appCfg->getAllCachedAccounts();
    QStringList items;
    for(int i = 0; i < accItems.count(); ++i)
        items<<accItems.at(i)->accName;
    bool ok;
    QString name = QInputDialog::getItem(this, tr("删除账户"),tr("请选择要删除的账户名"), items, 0, false, &ok);
    if (ok && !name.isEmpty()){
        int index = -1;
        for(int i = 0; i < accItems.count(); ++i){
            if(accItems.at(i)->accName == name){
                index = i;
                break;
            }
        }
        if(index == -1)
            return;
        AccountCacheItem* accItem = accItems.at(index);
        if(!appCfg->removeAccountCache(accItem)){
            QMessageBox::critical(this,tr("出错信息"),tr("在删除该账户的缓存记录时发生错误！"));
            return;
        }
        //将账户文件重命名后拷贝到备份目录中
        BackupUtil backup;
        if(!backup.backup(accItem->fileName,BackupUtil::BR_REMOVE))
            QMessageBox::warning(this,"",tr("将账户转移至备份目录时发生错误！"));
        QString sname = QString("%1%2").arg(DATABASE_PATH).arg(accItem->fileName);
        QFile::remove(sname);

    }

}

/**
 * @brief 从老版账户中导入指定年月的凭证
 *  这主要是为了在新旧版本过渡期间验证新版与旧版在统计功能、自动创建凭证功能、余额计算功能上的准确性
 */
void MainWindow::on_actImpPzSet_triggered()
{
    if(!curAccount)
        return;
    ImportOVAccDlg dlg(curAccount,curSSPanel, ui->mdiArea);
    dlg.exec();

}

/**
 * @brief 更新账户数据库表格创建Sql语句
 */
void MainWindow::on_actUpdateSql_triggered()
{
    QSqlDatabase db = curAccount->getDbUtil()->getDb();
    QString s = QString("select name,sql from sqlite_master");
    QSqlQuery q(db);
    if(!q.exec(s))
        return;
    QStringList names,sqls;
    while(q.next()){
        QString name = q.value(0).toString();
        QString sql = q.value(1).toString();
        if(name == "sqlite_sequence")   //这个是用来记录表格自增类型主键的
            continue;
        if(name.indexOf(tbl_sndsub_join_pre) != -1) //二级科目对接表
            continue;
        if(name.indexOf(tbl_fsub_prefix) != -1){    //一级科目表只记录一次
            QString tem = name;
            name.chop(1);
            if(names.contains(name))
                continue;
            sql.replace(tem,name);
        }
        names<< name;
        sqls<< sql;
    }
    AppConfig::getInstance()->updateTableCreateStatment(names,sqls);
}

/**
 * @brief MainWindow::on_actExtComSndSub_triggered
 */
void MainWindow::on_actExtComSndSub_triggered()
{
    if(!exportCommonSubject())
        QMessageBox::warning(this,"",tr("导出过程出错，请查看日志！"));
}

/**
 * @brief 打开选项配置窗口
 */
void MainWindow::on_actOption_triggered()
{
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    ConfigPanels* form = NULL;
    if(!commonGroups.contains(SUBWIN_OPTION)){
        dbUtil->getSubWinInfo(SUBWIN_OPTION,winfo,sinfo);
        form = new ConfigPanels();
        AppCommCfgPanel* comPanel = new AppCommCfgPanel(form);
        form->addPanel(comPanel,QIcon(":/images/Options/common.png"));
        PzTemplateOptionForm* panel = new PzTemplateOptionForm(form);
        form->addPanel(panel,QIcon(":/images/Options/pzTemplate.png"));        
        //SpecSubCodeCfgform* ssccPanel = new SpecSubCodeCfgform(form);
        //form->addPanel(ssccPanel,QIcon(":/images/Options/test1.png"));
        TestPanel* testPanel = new TestPanel(form);
        form->addPanel(testPanel,QIcon(":/images/Options/test1.png"));

    }
    showCommonSubWin(SUBWIN_OPTION,form,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

/**
 * @brief 管理外部实用工具
 */
#include <QProcess>
void MainWindow::on_actManageExternalTool_triggered()
{
    //任务：
    //1在配置文件中添加外部工具设置段
    //2在AppConfig类中添加读取和保存方法
    //3创建一个管理界面，可以浏览、添加、删除外部工具
    //4、启动时如果外部工具为空，则根据运行的操作系统平台类型添加默认的计算器工具软件

//    QProcess* p = new QProcess(this);
//    p->start("gedit",QStringList());
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    ExternalToolConfigForm* form = NULL;
    if(!commonGroups.contains(SUBWIN_EXTERNALTOOLS)){
        dbUtil->getSubWinInfo(SUBWIN_EXTERNALTOOLS,winfo,sinfo);
        form = new ExternalToolConfigForm(&eTools);
    }
    showCommonSubWin(SUBWIN_EXTERNALTOOLS,form,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;


}

/**
 * @brief 税金比对
 */
void MainWindow::on_actTaxCompare_triggered()
{
#ifdef Q_OS_WIN
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    TaxesComparisonForm* form = NULL;
    if(!commonGroups.contains(SUBWIN_TAXCOMPARE)){
        dbUtil->getSubWinInfo(SUBWIN_TAXCOMPARE,winfo,sinfo);
        form = new TaxesComparisonForm(curSuiteMgr);
        connect(form,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    showCommonSubWin(SUBWIN_TAXCOMPARE,form,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
#else
    QMessageBox::warning(this,"",tr("此功能目前仅在Windows平台下可用！"));
#endif
}

void MainWindow::on_actNoteMgr_triggered()
{
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    NoteMgrForm* form = NULL;
    if(!commonGroups.contains(SUBWIN_NOTEMGR)){
        dbUtil->getSubWinInfo(SUBWIN_NOTEMGR,winfo,sinfo);
        form = new NoteMgrForm(curAccount);
    }
    showCommonSubWin(SUBWIN_NOTEMGR,form,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}



//退出应用
void MainWindow::closeEvent(QCloseEvent *event)
{
    ui->mdiArea->closeAllSubWindows();
    if(curAccount)       //为了在下次打开应用时自动打开最后打开的账户
        closeAccount();
    if (ui->mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
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

///////////////////////数据菜单处理槽部分/////////////////////////////////////
/**
 * @brief MainWindow::viewSubjectExtra
 *  显示科目余额
 */
void MainWindow::viewSubjectExtra()
{

    if(curSuiteMgr->isDirty())
        curSuiteMgr->save();

    ApcData* w = NULL;
    SubWindowDim* winfo = NULL;
    QByteArray* sinfo = NULL;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_LOOKUPSUBEXTRA)){
        dbUtil->getSubWinInfo(SUBWIN_LOOKUPSUBEXTRA,winfo,sinfo);
        w = new ApcData(curAccount,false,this);
        w->setWindowTitle(tr("科目余额"));
    }
    else{
        w = static_cast<ApcData*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_LOOKUPSUBEXTRA));
        if(w){
            int y = curSuiteMgr->getSuiteRecord()->year;
            int m = curSuiteMgr->isPzSetOpened()?curSuiteMgr->month():curSuiteMgr->getSuiteRecord()->startMonth;
            w->setYM(y,m);
        }

    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_LOOKUPSUBEXTRA,w,winfo);
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
        if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZEDIT)){
            dbUtil->getSubWinInfo(SUBWIN_PZEDIT,winfo,sinfo);
            w = new PzDialog(month,curSuiteMgr,sinfo);
            w->setWindowTitle(tr("凭证窗口（新）"));
            connect(w,SIGNAL(showMessage(QString,AppErrorLevel)),this,SLOT(showRuntimeInfo(QString,AppErrorLevel)));
            connect(w,SIGNAL(selectedBaChanged(QList<int>,bool)),this,SLOT(baSelectChanged(QList<int>,bool)));
            connect(w,SIGNAL(rateChanged(int)),this,SLOT(rateChanged(int)));
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZEDIT,w,winfo);
        }
        else{
            w = static_cast<PzDialog*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_PZEDIT));
            w->setMonth(month);
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZEDIT,NULL,winfo);
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


////////////////////////帮助菜单处理槽部分////////////////////////////////////////
void MainWindow::about()
{
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

void MainWindow::showTemInfo(QString info)
{
    int timeout;
    AppConfig::getInstance()->getCfgVar(AppConfig::CVC_TimeoutOfTemInfo,timeout);
    ui->statusbar->showMessage(info, timeout);
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

/**
 * @brief MainWindow::undoCleanChanged
 *  当undo框架进入或退出干净状态时，实时启用或禁用保存按钮
 * @param clean
 */
void MainWindow::undoCleanChanged(bool clean)
{
    //如果当前激活的子窗口是凭证编辑窗口，则
    //if(ui->mdiArea->activeSubWindow() == subWindows.value(SUBWIN_PZEDIT_new))
    //    ui->actSave->setEnabled(!clean);
    if(activedMdiChild() == SUBWIN_PZEDIT)
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
    static int oriIdx = -1;      //undo栈的初始索引
    int newSize = undoStack->count();

    //这种情况是在凭证集没有进行undo操作，由用户在凭证编辑界面直接修改引起
    //或在进行undo操作后，用户要在编辑界面进行了修改，这样undo栈的尺寸就要增长或剪短
    //这不需要更新界面
    if(newSize != oriSize){
        oriSize = newSize;
        //LOG_INFO(QString("oriSize change to %1").arg(oriSize));
        return;
    }
    if(oriIdx != idx){
        PzDialog* pd = qobject_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
        if(pd)
            pd->updateContent();
    }
    oriIdx = idx;


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
 * @brief MainWindow::curPzChanged
 *  当前凭证发生改变时，调整会计分录索引改变的监视对象
 * @param newPz
 * @param oldPz
 */
void MainWindow::curPzChanged(PingZheng *newPz, PingZheng *oldPz)
{
    if(!newPz)
        rfPzStateEditAct(false,Pzs_NULL);
    else{
        bool editable = curSuiteMgr->getState() != Ps_Jzed;
        rfPzStateEditAct(editable,newPz->getPzState());
    }
//    if(oldPz)
//        disconnect(oldPz,SIGNAL(indexBoundaryChanged(bool,bool)),this,SLOT(baIndexBoundaryChanged(bool,bool)));
//    if(newPz){
//        connect(newPz,SIGNAL(indexBoundaryChanged(bool,bool)),this,SLOT(baIndexBoundaryChanged(bool,bool)));
//        rfEditInPzAct(newPz);
//    }
//    else{
//        ui->actInsertPz->setEnabled(false);
//        ui->actDelPz->setEnabled(false);
//    }
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
    rfBaEditAct();
//    if(!curSuiteMgr->getCurPz()){
//        ui->actMvUpAction->setEnabled(false);
//        ui->actMvDownAction->setEnabled(false);
//        return;
//    }
//    bool first,last;
//    if(rows.isEmpty() || !conti || (rows.count()==curSuiteMgr->getCurPz()->baCount())){
//        first=true;last=true;
//    }
//    else{
//        first = (rows.first() == 0);
//        last = (rows.last() == curSuiteMgr->getCurPz()->baCount()-1);
//    }

//    if(!first && !last){  //当前分录处于中间位置
//        ui->actMvUpAction->setEnabled(true);
//        ui->actMvDownAction->setEnabled(true);
//    }
//    else if(first && last){ //分录为空或只有一条分录
//        ui->actMvUpAction->setEnabled(false);
//        ui->actMvDownAction->setEnabled(false);
//    }
//    else{
//        ui->actMvUpAction->setEnabled(!first);
//        ui->actMvDownAction->setEnabled(!last);
//    }
//    bool r = (curSuiteMgr->getState()!=Ps_NoOpen) || (curSuiteMgr->getState()!= Ps_Jzed);
//    r = r && (curSuiteMgr->getCurPz()->getPzState() == Pzs_Recording);
//    ui->actDelAction->setEnabled(r && !rows.isEmpty());
//    ui->actInsertBa->setEnabled(r && (rows.count() == 1));
    //    ui->actAddAction->setEnabled(r);
}

void MainWindow::rateChanged(int month)
{
    ui->actSave->setEnabled(true);
}

/**
 * @brief 启动指定索引的外部工具
 * @param index
 */
void MainWindow::startExternalTool(int index)
{
    ExternalToolCfgItem* tool = eTools.at(index);
    QString commandline = tool->commandLine;
    if(!tool->parameter.isEmpty())
        commandline.append(" ").append(tool->parameter);
    QProcess::startDetached(commandline);
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
            SubWinGroupMgr* grpMgr = new SubWinGroupMgr(key,ui->mdiArea);
            subWinGroups[key] = grpMgr;
            connect(grpMgr,SIGNAL(specSubWinClosed(subWindowType)),this,SLOT(specSubWinClosed(subWindowType)));
            //connect(subWinGroups.value(key),SIGNAL(saveSubWinState(subWindowType,QByteArray*,SubWindowDim*)),
            //        this,SLOT(saveSubWindowState(subWindowType,QByteArray*,SubWindowDim*)));
        }
        adjustEditMenus(UT_PZ,true);
        subWinGroups.value(key)->show();
        ui->statusbar->setCurSuite(current->getSuiteRecord()->name);
        //if(commonGroups && commonGroups->isShow())
        //    commonGroups->hide();
    }
    if(curSuiteMgr){
        disconnect(curSuiteMgr,SIGNAL(pzCountChanged(int)),this,SLOT(pzCountChanged(int)));
        disconnect(curSuiteMgr,SIGNAL(currentPzChanged(PingZheng*,PingZheng*)),this,SLOT(curPzChanged(PingZheng*,PingZheng*)));
        disconnect(curSuiteMgr,SIGNAL(pzSetStateChanged(PzsState)),this,SLOT(pzSetStateChanged(PzsState)));
        disconnect(curSuiteMgr,SIGNAL(pzExtraStateChanged(bool)),this,SLOT(pzSetExtraStateChanged(bool)));
    }
    curSuiteMgr = current;
    connect(curSuiteMgr,SIGNAL(pzCountChanged(int)),this,SLOT(pzCountChanged(int)));
    connect(curSuiteMgr,SIGNAL(currentPzChanged(PingZheng*,PingZheng*)),this,SLOT(curPzChanged(PingZheng*,PingZheng*)));
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
    undoAction = undoStack->createUndoAction(this);
    redoAction = undoStack->createRedoAction(this);
    undoAction->setIcon(QIcon(":/images/edit-undo.png"));
    redoAction->setIcon(QIcon(":/images/edit-redo.png"));
    MyMdiSubWindow* subWin = qobject_cast<MyMdiSubWindow*>(ui->mdiArea->activeSubWindow());
    if(subWin){
        if(subWin->getWindowType() == SUBWIN_PZEDIT){
            adjustEditMenus(UT_PZ,false);
        }
    }
    refreshShowPzsState();
    rfPzNaviAct();
    rfBaEditAct();
    rfSaveBtn();
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
    bool editable = false;

    if(accSmg->getSuiteRecord()->isClosed || accSmg->getState(month) == Ps_Jzed){
        historyPzSet[suiteId] = accSmg->getHistoryPzSet(month);
        if(historyPzSet.value(suiteId).isEmpty()){
            QMessageBox::warning(this,tr("提示信息"),tr("没有任何凭证可显示！"));
            historyPzSetIndex[suiteId] = -1;
        }
        else{
            historyPzMonth[suiteId] = month;
            historyPzSetIndex[suiteId] = 0;
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
        if((curSuiteMgr != accSmg) || ((curSuiteMgr == accSmg) && (curSuiteMgr->month() != month)))
            curSuiteMgr = accSmg;
        if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZEDIT)){
            dbUtil->getSubWinInfo(SUBWIN_PZEDIT,winfo,sinfo);
            PzDialog* w = new PzDialog(month,curSuiteMgr,sinfo);
            w->setWindowTitle(tr("凭证编辑窗口"));
            connect(w,SIGNAL(showMessage(QString,AppErrorLevel)),this,SLOT(showRuntimeInfo(QString,AppErrorLevel)));
            connect(w,SIGNAL(selectedBaChanged(QList<int>,bool)),this,SLOT(baSelectChanged(QList<int>,bool)));
            connect(w,SIGNAL(rateChanged(int)),this,SLOT(rateChanged(int)));
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZEDIT,w,winfo);
        }
        else{
            w = static_cast<PzDialog*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_PZEDIT));
            if(w)
                w->setMonth(month);
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZEDIT,NULL,winfo);
        }
        PingZheng* curPz = curSuiteMgr->getCurPz();
        editable = (curSuiteMgr->getState() != Ps_Jzed) && curPz &&
                (curPz->getPzState() == Pzs_Recording);
        //undoView->setStack(curSuiteMgr->getUndoStack());
        //adjustEditMenus(UT_PZ,true);
        rfPzSetEditAct(true);
    }
    //调整界面部件的启用性，以使用户可以导航凭证集和进行其他帐务处理工作
    //导航按钮控制

    rfPzNaviAct();
    rfBaEditAct();
    refreshShowPzsState();
    //如果凭证集是以编辑模式打开，则还要控制其他高级帐务处理动作

}

/**
 * @brief 凭证集刚被打开，对界面做些调整
 * @param accSmg
 * @param month
 */
void MainWindow::pzSetOpen(AccountSuiteManager *accSmg, int month)
{
    refreshShowPzsState();
    ui->statusbar->setPzSetDate(accSmg->getSuiteRecord()->year,month);
    rfPzSetOpenAct();
    //switchUndoAction(false);
}

/**
 * @brief 凭证集将被关闭，在关闭前，可能需要保存凭证集相关的数据和信息，并调整界面
 * @param accSmg
 * @param month
 */
void MainWindow::prepareClosePzSet(AccountSuiteManager *accSmg, int month)
{
    //在关闭凭证集前，要检测是否有未保存的修改
//    if(curSuiteMgr->isDirty()){
//        if(QMessageBox::Accepted ==
//                QMessageBox::warning(0,tr("提示信息"),tr("凭证集已被改变，要保存吗？"),
//                                     QMessageBox::Yes|QMessageBox::No))
//            if(!curSuiteMgr->save())
//                QMessageBox::critical(0,tr("错误信息"),tr("保存凭证集时发生错误！"));

//    }
    //关闭凭证集时，要先关闭所有与该凭证集相关的子窗口
    SubWinGroupMgr* gm = subWinGroups.value(accSmg->getSuiteRecord()->id);
    if(gm){
        gm->closeSubWindow(SUBWIN_PZEDIT);      //凭证编辑窗口
        gm->closeSubWindow(SUBWIN_PZSTAT);      //本期统计的窗口
        //gm->closeSubWindow(SUBWIN_DETAILSVIEW); //明细账窗口
        //gm->closeSubWindow(TOTALDAILY);         //总分类账窗口
    }

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

    rfPzSetOpenAct();
    //rfAct();
    //rfTbrVisble();
    //refreshActEnanble();
    //adjustEditMenus(UT_PZ,true);
    //switchUndoAction(true);
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
                w->closeAllPage();
                delete w;
            }
        }
        else if(winType == SUBWIN_EXTERNALTOOLS){
            ExternalToolConfigForm* w = static_cast<ExternalToolConfigForm*>(subWin->widget());
            if(w){
                if(w->maybeSave() && QMessageBox::warning(this,"",tr("工具配置已改变，需要保存吗？"),
                                                          QMessageBox::Yes|QMessageBox::No,
                                                          QMessageBox::Yes)==QMessageBox::Yes)
                    w->save();
                if(w->isUpdateMenuItem()){
                    //因为有可能有效项目是新加入的但最后被取消了，因此不能作为正确项目
                    initExternalTools();
                }
                delete w;
            }
        }
        else if(winType == SUBWIN_SECURITY){
            SecConDialog* w = static_cast<SecConDialog*>(subWin->widget());
            if(w)
                state = w->getState();
        }
#ifdef Q_OS_WIN
        else if(winType == SUBWIN_TAXCOMPARE){

            TaxesComparisonForm* w = static_cast<TaxesComparisonForm*>(subWin->widget());
            if(w){
                disconnect(w,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
            }
        }
#endif
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

/**
 * @brief 根据激活的子窗口类型，来控制相应菜单项的启用性
 * @param window
 */
void MainWindow::subWindowActivated(QMdiSubWindow *window)
{
    MyMdiSubWindow* win = qobject_cast<MyMdiSubWindow*>(window);
    if(win && win->getWindowType() == SUBWIN_PZEDIT){
        rfSaveBtn();
        adjustEditMenus(UT_PZ,false);
    }
    else{
        adjustEditMenus(UT_PZ,true);
    }
    rfBaEditAct();
    rfPzNaviAct();
    if(win){
        subWindowType wType = win->getWindowType();
        if(wType == SUBWIN_PZEDIT || wType == SUBWIN_HISTORYVIEW ||
                wType == SUBWIN_PZSTAT || wType == SUBWIN_DETAILSVIEW)
            btnPrint->setEnabled(true);
        else
            btnPrint->setEnabled(false);
    }
}

/**
 * @brief 监视一些子窗口的关闭事件，以实时更新相应菜单项的启用性
 * @param winType
 */
void MainWindow::specSubWinClosed(subWindowType winType)
{
    if(winType == SUBWIN_PZEDIT){
        if(curSuiteMgr->isDirty()){
            if(QMessageBox::Yes == QMessageBox::warning(this,tr("提示信息"),
                                                        tr("凭证集已被修改，需要保存吗？"),
                                                        QMessageBox::Yes|QMessageBox::No))
                curSuiteMgr->save();
            else{
                curSuiteMgr->rollback();
            }
        }
        ui->actSave->setEnabled(false);
    }
    //adjustEditMenus(UT_PZ,false);
    rfPzSetEditAct(false);
    rfBaEditAct();
    rfPzNaviAct();
    if(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->count() == 0)
        btnPrint->setEnabled(false);
}

void MainWindow::printProcess()
{
    PrintActionClass pac = PAC_NONE;
    QAction* act = qobject_cast<QAction*>(sender());
    if(!act)
        return;
    if(act == actPrintToPrinter)
        pac = PAC_TOPRINTER;
    else if(act == actPrintPreview)
        pac = PAC_PREVIEW;
    else if(act == actPrintToPDF)
        pac = PAC_TOPDF;
    else if(act == actOutputToExcel)
        pac = PAC_TOEXCEL;
    else
        return;
    MyMdiSubWindow* subWin = qobject_cast<MyMdiSubWindow*>(ui->mdiArea->activeSubWindow());
    if(!subWin)
        return;
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT || winType == SUBWIN_HISTORYVIEW){
        QList<PingZheng*> pzSet;
        PingZheng* curPz;
        if(winType == SUBWIN_PZEDIT){
            curPz = curSuiteMgr->getCurPz();
            pzSet = curSuiteMgr->getHistoryPzSet(curSuiteMgr->month());
        }
        else{
            int id = curSuiteMgr->getSuiteRecord()->id;
            pzSet = historyPzSet.value(id);
            curPz = pzSet.at(historyPzSetIndex.value(id));
        }
        PrintSelectDialog* psDlg = new PrintSelectDialog(pzSet,curPz,this);
        if(psDlg->exec() == QDialog::Accepted){
            QList<PingZheng*> pzs;
            if(!psDlg->getSelectedPzs(pzs))//获取打印范围
                return;
            QPrinter printer;
            QPrintDialog* dlg = new QPrintDialog(&printer,this); //获取所选的打印机
            if(dlg->exec() == QDialog::Accepted){
                if(printer.pageSize() != QPrinter::A4){
                    QMessageBox::warning(this,tr("打印纸张出错"),tr("打印凭证只支持A4纸打印，一张A4纸可以打印两张凭证！"));
                    return;
                }
                QPrintPreviewDialog* preview = NULL;
                printer.setOrientation(QPrinter::Portrait);                              
                PrintPzUtils* view = new PrintPzUtils(curAccount,&printer);
                view->setPzs(pzs);
                switch(pac){
                case PAC_TOPRINTER: //输出到打印机
                    view->print(&printer);
                    break;
                case PAC_PREVIEW: //打印预览
                    preview = new QPrintPreviewDialog(&printer);
                    connect(preview, SIGNAL(paintRequested(QPrinter*)),
                            view, SLOT(print(QPrinter*)));
                    preview->exec();
                    break;
                case PAC_TOPDF: //输出到PDF
                    printer.setOutputFormat(QPrinter::PdfFormat);
                    QString fname = QFileDialog::getSaveFileName(this,tr("请输入文件名"),"./outPdfs","*.pdf");
                    if(fname != ""){
                        printer.setOutputFileName(fname);
                        view->print(&printer);
                    }
                    break;
                }
                if(preview)
                    delete preview;
                delete view;
            }
            delete dlg;
        }
        delete psDlg;
    }
    else if(winType == SUBWIN_PZSTAT || winType == SUBWIN_DETAILSVIEW ||
            winType == SUBWIN_TOTALVIEW){
        DialogWithPrint* dlg = qobject_cast<DialogWithPrint*>(subWin->widget());
        if(!dlg)
            return;
        dlg->print(pac);
    }
}

//处理添加凭证动作事件
void MainWindow::on_actAddPz_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && curSuiteMgr->getState() != Ps_Jzed){
        w->addPz();
        refreshShowPzsState();
        if(!ui->actDelPz->isEnabled())
            ui->actDelPz->setEnabled(true);
        //refreshActEnanble();
    }
}

/**
 * @brief MainWindow::on_actInsertPz_triggered
 *  插入凭证
 */
void MainWindow::on_actInsertPz_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && curSuiteMgr->getState() != Ps_Jzed){
        w->insertPz();
        refreshShowPzsState();
        if(!ui->actDelPz->isEnabled())
            ui->actDelPz->setEnabled(true);
    }
}

//删除凭证（综合考虑凭证集状态和凭证类别，来决定新的凭证集状态）
void MainWindow::on_actDelPz_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && curSuiteMgr->getState() != Ps_Jzed){
        w->removePz();
        if(!curSuiteMgr->getCurPz())
            ui->actDelPz->setEnabled(false);
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
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && curSuiteMgr->getState() != Ps_Jzed)
        w->addBa();
}

/**
 * @brief MainWindow::on_actInsertBa_triggered
 *  插入会计分录
 */
void MainWindow::on_actInsertBa_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && curSuiteMgr->getState() != Ps_Jzed)
        w->insertBa();
}

/**
 * @brief MainWindow::on_actDelAction_triggered
 *  删除分录
 */
void MainWindow::on_actDelAction_triggered()
{
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
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
    if(winType == SUBWIN_PZEDIT){
        PingZheng* oldPz = curSuiteMgr->getCurPz();
        PingZheng* newPz = curSuiteMgr->first();

        curPzChanged(newPz,oldPz);
    }
    else if(winType == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        historyPzSetIndex[suiteId] = 0;
        w->setPz(historyPzSet.value(suiteId).first());
    }
    rfPzNaviAct();
}

/**
 * @brief MainWindow::on_actGoPrev_triggered
 *  导航到前一张凭证
 */
void MainWindow::on_actGoPrev_triggered()
{
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT){
        PingZheng* oldPz = curSuiteMgr->getCurPz();
        PingZheng* newPz = curSuiteMgr->previou();
        curPzChanged(newPz,oldPz);
    }
    else if(winType == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        if(historyPzSetIndex.value(suiteId) > 0){
            historyPzSetIndex[suiteId] -= 1;
            w->setPz(historyPzSet.value(suiteId).at(historyPzSetIndex.value(suiteId)));

        }
    }
    rfPzNaviAct();
}

/**
 * @brief MainWindow::on_actGoNext_triggered
 *  导航到后一张凭证
 */
void MainWindow::on_actGoNext_triggered()
{
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT){
        PingZheng* oldPz = curSuiteMgr->getCurPz();
        PingZheng* newPz = curSuiteMgr->next();
        curPzChanged(newPz,oldPz);
    }
    else if(winType == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        if(historyPzSetIndex.value(suiteId) < historyPzSet.value(suiteId).count()-1){
            historyPzSetIndex[suiteId] += 1;
            w->setPz(historyPzSet.value(suiteId).at(historyPzSetIndex.value(suiteId)));

        }
    }
    rfPzNaviAct();
}

/**
 * @brief MainWindow::on_actGoLast_triggered
 *  导航到最后一张凭证
 */
void MainWindow::on_actGoLast_triggered()
{
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT){
        PingZheng* oldPz = curSuiteMgr->getCurPz();
        PingZheng* newPz = curSuiteMgr->last();
        curPzChanged(newPz,oldPz);
    }
    else if(winType == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        historyPzSetIndex[suiteId] = historyPzSet.value(suiteId).count()-1;
        w->setPz(historyPzSet.value(suiteId).last());
    }
    rfPzNaviAct();
}

/**
 * @brief MainWindow::on_actSavePz_triggered
 *  保存对当前凭证集的更改
 */
void MainWindow::on_actSave_triggered()
{
    subWindowType winType = activedMdiChild();
    if(winType == SUBWIN_PZEDIT){
        PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
        if(w)
            w->save();
    }
    ui->actSave->setEnabled(false);
    rfPzSetStateAct();
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



/**
 * @brief MainWindow::refreshShowPzsState
 *  在状态条上显示当前打开的凭证集的信息（时间范围、状态、凭证数、余额有效性等）
 */
void MainWindow::refreshShowPzsState()
{
    //如果存在以编辑模式打开的凭证集，则显示该凭证集的信息，否者如果存在打开的历史凭证集，则显示该历史凭证集信息
    int y=0,m=0;
    int repealNum=0,recordingNum=0,verifyNum=0,instatNum=0,amount=0;
    PzsState state = Ps_NoOpen;
    bool extraState = false;
    if(curSuiteMgr){
        y = curSuiteMgr->year();
        if(curSuiteMgr->isPzSetOpened()){
            m = curSuiteMgr->month();
            repealNum = curSuiteMgr->getRepealCount();
            recordingNum = curSuiteMgr->getRecordingCount();
            verifyNum = curSuiteMgr->getVerifyCount();
            instatNum = curSuiteMgr->getInstatCount();
            state = curSuiteMgr->getState();
            extraState = curSuiteMgr->getExtraState();
        }
        else{
            m = historyPzMonth.value(curSuiteMgr->getSuiteRecord()->id);
            if(m > 0 && m < 13){
                curSuiteMgr->getPzCountForMonth(m,repealNum,recordingNum,verifyNum,instatNum);
                state = curSuiteMgr->getState(m);
                extraState = curSuiteMgr->getExtraState(m);
            }
        }
        amount = repealNum+recordingNum+verifyNum+instatNum;
    }
    ui->statusbar->setPzSetDate(y,m);
    ui->statusbar->setPzCounts(repealNum,recordingNum,verifyNum,instatNum,amount);
    ui->statusbar->setPzSetState(state);
    ui->statusbar->setExtraState(extraState);
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
    rfPzSetStateAct();
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
    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
        return;
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT))
        return;
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && !w->crtJzhdPz())
        QMessageBox::critical(0,tr("错误信息"),tr("在创建结转汇兑损益的凭证时发生错误!"));
}

//结转损益
void MainWindow::on_actFordPl_triggered()
{
    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
        return;
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT)){
        QMessageBox::warning(this,"",tr("请先打开凭证编辑窗口，再执行结转损益操作！"));
        return;
    }
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && !w->crtJzsyPz())
        QMessageBox::critical(0,tr("错误信息"),tr("在创建结转损益的凭证时发生错误!"));
}



/**
 * @brief MainWindow::on_actCurStatNew_triggered
 *  统计本期发生额，并计算期末余额
 */
void MainWindow::on_actCurStatNew_triggered()
{
    if(!curSuiteMgr->isPzSetOpened()){
        pzsWarning();
        return;
    }
    if(!curSuiteMgr->isDirty())
        curSuiteMgr->save();

    CurStatDialog* dlg = NULL;
    SubWindowDim* winfo = NULL;
    QByteArray* sinfo = NULL;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZSTAT)){
        dbUtil->getSubWinInfo(SUBWIN_PZSTAT,winfo,sinfo);
        dlg = new CurStatDialog(curSuiteMgr->getStatUtil(), sinfo, this);
        connect(dlg,SIGNAL(infomation(QString)),this,SLOT(showTemInfo(QString)));
        connect(dlg,SIGNAL(extraValided()),this,SLOT(extraValid()));
    }
    else{
        dlg = static_cast<CurStatDialog*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_PZSTAT));
        if(dlg)
            dlg->stat();
    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZSTAT,dlg,winfo);
    if(winfo)
        delete winfo;
    if(sinfo)
        delete sinfo;
}

//登录
void MainWindow::on_actLogin_triggered()
{
    int recentUserId;
    AppConfig::getInstance()->getCfgVar(AppConfig::CVC_ResentLoginUser,recentUserId);
    LoginDialog* dlg = new LoginDialog;
    if(dlg->exec() == QDialog::Accepted){
        curUser = dlg->getLoginUser();
        recentUserId = curUser->getUserId();        
        ui->statusbar->setUser(curUser);
    }
    rfLogin();;
}

//登出
void MainWindow::on_actLogout_triggered()
{
    curUser = NULL;
    ui->statusbar->setUser(curUser);
    rfLogin();
}


//切换用户
void MainWindow::on_actShiftUser_triggered()
{
    LoginDialog* dlg = new LoginDialog;
    if(dlg->exec() == QDialog::Accepted){
        curUser = dlg->getLoginUser();
        ui->statusbar->setUser(curUser);
    }
    rfLogin();
}

//显示安全配置对话框
void MainWindow::on_actSecCon_triggered()
{
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    SecConDialog* dlg = NULL;
    if(!commonGroups.contains(SUBWIN_SECURITY)){
        dbUtil->getSubWinInfo(SUBWIN_SECURITY,winfo,sinfo);
        dlg = new SecConDialog(sinfo,this);
    }
    showCommonSubWin(SUBWIN_SECURITY,dlg,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}


//向上移动分录
void MainWindow::on_actMvUpAction_triggered()
{
    if(activedMdiChild() == SUBWIN_PZEDIT){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
        pzEdit->moveUpBa();
    }

}

//向下移动分录
void MainWindow::on_actMvDownAction_triggered()
{
    if(activedMdiChild() == SUBWIN_PZEDIT){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
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
    int num = spnNaviTo->value();
    if(curSuiteMgr->isPzSetOpened())
        viewOrEditPzSet(curSuiteMgr, curSuiteMgr->month());

    if(activedMdiChild() == SUBWIN_PZEDIT){
        if(!curSuiteMgr->seek(num)){
            QMessageBox::question(this,tr("错误提示"),
                                  tr("没有凭证号为%1的凭证").arg(num));

        }
    }
    else if(activedMdiChild() == SUBWIN_HISTORYVIEW){
        int suiteId = curSuiteMgr->getSuiteRecord()->id;
        HistoryPzForm* win = qobject_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
        PingZheng* pz = NULL;
        foreach (PingZheng* p, historyPzSet.value(suiteId)) {
            if(p->number() == num){
                pz = p;
                break;
            }
        }
        if(pz)
            win->setPz(pz);
    }
}

//全部手工凭证通过审核
void MainWindow::on_actAllVerify_triggered()
{
    if(!curUser->haveRight(allRights.value(Right::Pz_Advanced_Verify))){
        rightWarnning(Right::Pz_Advanced_Verify);
        return;
    }
    if(!curSuiteMgr->isPzSetOpened()){
        pzsWarning();
        return;
    }
    int affected = curSuiteMgr->verifyAll(curUser);
    showPzNumsAffected(affected);
    //刷新凭证集状态、各种状态的凭证数和余额状态

    //如果当前显示在凭证编辑窗口的凭证受到影响、也刷新之
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT))
        return;
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w){
        w->setPzState(Pzs_Instat);
        ui->actVerifyPz->setEnabled(false);
        ui->actInStatPz->setEnabled(true);
        ui->actAntiVerify->setEnabled(true);
    }
}

//全部凭证入账
void MainWindow::on_actAllInstat_triggered()
{
    //应该利用undo框架完成该动作，而不是直接操纵数据库
    if(!curUser->haveRight(allRights.value(Right::Pz_Advanced_Instat))){
        rightWarnning(Right::Pz_Advanced_Instat);
        return;
    }
    if(!curSuiteMgr->isPzSetOpened()){
        pzsWarning();
        return;
    }
    int affected = curSuiteMgr->instatAll(curUser);
    showPzNumsAffected(affected);
    //刷新凭证集状态、各种状态的凭证数和余额状态
    //如果当前显示在凭证编辑窗口的凭证受到影响、也刷新之
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT))
        return;
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w){
        w->setPzState(Pzs_Instat);
        ui->actInStatPz->setEnabled(false);
        ui->actVerifyPz->setEnabled(false);
        ui->actAntiVerify->setEnabled(true);
    }
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

/**
 * @brief MainWindow::on_actEndAcc_triggered
 *  结账
 */
void MainWindow::on_actEndAcc_triggered()
{
    if(!curSuiteMgr->isPzSetOpened()){
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
        curSuiteMgr->setState(Ps_Jzed);
        curSuiteMgr->save();
        if(dockWindows.contains(TV_SUITESWITCH)){
            SuiteSwitchPanel* w = static_cast<SuiteSwitchPanel*>(dockWindows.value(TV_SUITESWITCH)->widget());
            w->setJzState(curSuiteMgr,curSuiteMgr->month());
        }
        rfPzSetOpenAct();
        refreshShowPzsState();
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

/**
 * @brief MainWindow::on_actAntiEndAcc_triggered
 *  反结账
 */
void MainWindow::on_actAntiEndAcc_triggered()
{
    //打破结账限制，
    if(!curSuiteMgr->isPzSetOpened()){
        pzsWarning();
        return;
    }
    if(curSuiteMgr->getState() != Ps_Jzed)
        return;
    curSuiteMgr->setState(Ps_AllVerified);
    rfPzSetOpenAct();

    if(dockWindows.contains(TV_SUITESWITCH)){
        SuiteSwitchPanel* w = static_cast<SuiteSwitchPanel*>(dockWindows.value(TV_SUITESWITCH)->widget());
        w->setJzState(curSuiteMgr,curSuiteMgr->month(),false);
    }
    refreshShowPzsState();
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
    SubWindowDim dim;
    dim.w = 1000;
    dim.h = 600;
    dim.x = 10 * winNumber;
    dim.y = 10 * winNumber;
    showCommonSubWin(SUBWIN_SQL,dlg,&dim);
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
    //要重新实现显示总账的视图类，以适应应用结构的改变
//    QByteArray* sinfo;
//    SubWindowDim* winfo;
//    dbUtil->getSubWinInfo(TOTALVIEW,winfo,sinfo);
//    ShowTZDialog* dlg = new ShowTZDialog(curSuiteMgr->year(),curSuiteMgr->month(),sinfo);
//    showSubWindow(TOTALVIEW,winfo,dlg);
//    if(sinfo)
//        delete sinfo;
//    if(winfo)
//        delete winfo;
}



/**
 * @brief MainWindow::on_actDetailView_triggered
 *  查看明细账（新）
 */
void MainWindow::on_actDetailView_triggered()
{
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    ShowDZDialog* dlg = NULL;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_DETAILSVIEW)){
        dbUtil->getSubWinInfo(SUBWIN_DETAILSVIEW,winfo,sinfo);
        dlg = new ShowDZDialog(curAccount,sinfo);
        connect(dlg,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_DETAILSVIEW,dlg,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

/**
 * @brief MainWindow::on_actInStatPz_triggered
 *  使当前凭证入账
 */
void MainWindow::on_actInStatPz_triggered()
{
    if(!curSuiteMgr->getCurPz())
        return ;
    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
        return;
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT))
        return;
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w){
        w->setPzState(Pzs_Instat);
        ui->actVerifyPz->setEnabled(false);
        ui->actInStatPz->setEnabled(false);
        ui->actAntiVerify->setEnabled(true);
    }
}

/**
 * @brief MainWindow::on_actVerifyPz_triggered
 *  当前凭证审核通过
 */
void MainWindow::on_actVerifyPz_triggered()
{
    if(!curSuiteMgr->getCurPz())
        return ;
    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
        return;
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT))
        return;
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w){
        w->setPzState(Pzs_Verify);
        ui->actVerifyPz->setEnabled(false);
        ui->actInStatPz->setEnabled(true);
        ui->actAntiVerify->setEnabled(true);
    }
}


/**
 * @brief MainWindow::on_actAntiVerify_triggered
 *  取消已审核或已入账的凭证，使凭证回到初始状态
 */
void MainWindow::on_actAntiVerify_triggered()
{
    if(!curSuiteMgr->getCurPz())
        return ;
    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
        return;
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT))
        return;
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w){
        w->setPzState(Pzs_Recording);
        ui->actVerifyPz->setEnabled(true);
        ui->actInStatPz->setEnabled(false);
        ui->actAntiVerify->setEnabled(false);
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
    //AppConfig::getInstance()->clearRecentOpenAccount();
    int count=0;
    if(!AppConfig::getInstance()->refreshLocalAccount(count)){
        QMessageBox::critical(this,tr("错误信息"),tr("在扫描工作目录下的账户时发生错误！"));
        return;
    }
    //报告发现的账户
    QMessageBox::information(this,tr("提示信息"),tr("本次扫描共发现%1个账户！").arg(count));
}

/**
 * @brief 切换帐套视图
 */
void MainWindow::on_actSuite_triggered()
{
//    QDockWidget* dw;
//    if(!dockWindows.contains(TV_SUITESWITCH)){
//        dw = new QDockWidget(tr("帐套切换视图"), this);
//        addDockWidget(Qt::LeftDockWidgetArea, dw);
//        dockWindows[TV_SUITESWITCH] = dw;
//    }
//    else
//        dw = dockWindows.value(TV_SUITESWITCH);
//    if(curSSPanel){
//        disconnect(curSSPanel,SIGNAL(selectedSuiteChanged(AccountSuiteManager*,AccountSuiteManager*)),
//                this,SLOT(suiteViewSwitched(AccountSuiteManager*,AccountSuiteManager*)));
//        disconnect(curSSPanel,SIGNAL(viewPzSet(AccountSuiteManager*,int)),this,SLOT(viewOrEditPzSet(AccountSuiteManager*,int)));
//        disconnect(curSSPanel,SIGNAL(pzSetOpened(AccountSuiteManager*,int)),this,SLOT(pzSetOpen(AccountSuiteManager*,int)));
//        disconnect(curSSPanel,SIGNAL(prepareClosePzSet(AccountSuiteManager*,int)),this,SLOT(prepareClosePzSet(AccountSuiteManager*,int)));
//        disconnect(curSSPanel,SIGNAL(pzsetClosed(AccountSuiteManager*,int)),this,SLOT(pzSetClosed(AccountSuiteManager*,int)));
//        delete curSSPanel;
//    }
//    curSSPanel = new SuiteSwitchPanel(curAccount);
//    connect(curSSPanel,SIGNAL(selectedSuiteChanged(AccountSuiteManager*,AccountSuiteManager*)),
//            this,SLOT(suiteViewSwitched(AccountSuiteManager*,AccountSuiteManager*)));
//    connect(curSSPanel,SIGNAL(viewPzSet(AccountSuiteManager*,int)),this,SLOT(viewOrEditPzSet(AccountSuiteManager*,int)));
//    connect(curSSPanel,SIGNAL(pzSetOpened(AccountSuiteManager*,int)),this,SLOT(pzSetOpen(AccountSuiteManager*,int)));
//    connect(curSSPanel,SIGNAL(prepareClosePzSet(AccountSuiteManager*,int)),this,SLOT(prepareClosePzSet(AccountSuiteManager*,int)));
//    connect(curSSPanel,SIGNAL(pzsetClosed(AccountSuiteManager*,int)),this,SLOT(pzSetClosed(AccountSuiteManager*,int)));
//    if(!ssPanels.contains(curAccount->getCode())){
//        pn =
//        ssPanels[curAccount->getCode()] = pn;
//        dw->setWidget(pn);

//    }
//    else{
//        pn = ssPanels.value(curAccount->getCode());
//        if(dw->widget() != pn)
//            dw->setWidget(pn);
//    }

    dockWindows.value(TV_SUITESWITCH)->show();
}

/**
 * @brief MainWindow::on_actEmpAccount_triggered
 *  转出账户
 */
void MainWindow::on_actEmpAccount_triggered()
{
    TransferOutDialog dlg;
    dlg.exec();
}

/**
 * @brief MainWindow::on_actInAccount_triggered
 *  转入账户
 */
void MainWindow::on_actInAccount_triggered()
{
    TransferInDialog dlg;
    dlg.exec();
}


void MainWindow::on_actCloseCurWindow_triggered()
{
    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if(!w)
        return;
    w->close();
}

void MainWindow::on_actCloseAllWindow_triggered()
{
    if(!commonGroups.isEmpty()){
        QHashIterator<subWindowType,MyMdiSubWindow*> it(commonGroups);
        while(it.hasNext()){
            it.next();
            it.value()->close();
        }
        commonGroups.clear();
    }
    if(!commonGroups_multi.isEmpty()){
        QHashIterator<subWindowType,MyMdiSubWindow*> it(commonGroups_multi);
        while(it.hasNext()){
            it.next();
            it.value()->close();
        }
        commonGroups_multi.clear();
    }
    if(curSuiteMgr){
        int key = curSuiteMgr->getSuiteRecord()->id;
        if(subWinGroups.value(key)->count() != 0){
            subWinGroups.value(key)->closeAll();
        }
    }
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
        if(dim){
            w->resize(dim->w,dim->h);
            w->move(dim->x,dim->y);
        }
        commonGroups_multi.insert(winType,w);
    }
    if(w){
        connect(w,SIGNAL(windowClosed(MyMdiSubWindow*)),this,SLOT(commonSubWindowClosed(MyMdiSubWindow*)));
        w->show();
    }
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
//void MainWindow::clearUndo()
//{
//    if(!undoView)
//        return;
//    disconnect(undoView,SIGNAL(pressed(QModelIndex)),this,SLOT(undoViewItemClicked(QModelIndex)));
//    ui->menuEdit->removeAction(undoAction);
//    ui->menuEdit->removeAction(redoAction);
//    ui->tbrEdit->removeAction(undoAction);
//    ui->tbrEdit->removeAction(redoAction);
//    removeDockWidget(dockWindows.value(TV_UNDO));
//    delete dockWindows[TV_UNDO];
//    dockWindows.remove(TV_UNDO);
//    //delete undoView;
//    undoView = NULL;
//    adjustViewMenus(TV_UNDO,true);
//}

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
 * @param   ut
 * @param   restore true：恢复为ui文件中定义的，false：替换为Undo栈产生的
 */
void MainWindow::adjustEditMenus(UndoType ut, bool restore)
{
    QAction *undo,*redo;
    if(restore){
        undo = ui->actUndo;
        redo = ui->actRedo;
    }
    else{
        undo = undoAction;
        redo = redoAction;
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
 * @brief 从当前账户中导出常用科目到基本库
 * （这些信息用来在建立新账户时可以快速建立常用科目，立即使账户投入使用）
 * 信息将被保存在基本库的表SecondSubs的BelongTo字段内，用逗号分割，
 * 每个项用科目系统代码前导后跟科目一级代码，中间用连字符“-”连接。
 */
bool MainWindow::exportCommonSubject()
{
    if(!curAccount)
        return false;
    QSqlDatabase bdb = AppConfig::getBaseDbConnect();
    if(!bdb.transaction())
        return false;

    QSqlQuery q(bdb),q2(bdb);
    QString s = QString("update %1 set %2=''").arg(tbl_base_ni).arg(fld_base_ni_belongto);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("update %1 set %2=:codes where %3=:str").arg(tbl_base_ni)
            .arg(fld_base_ni_belongto).arg(fld_base_ni_name);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5,%6) values(:name,:lname,:remcode,:cls,:belongto)")
            .arg(tbl_base_ni).arg(fld_base_ni_name).arg(fld_base_ni_lname)
            .arg(fld_base_ni_remcode).arg(fld_base_ni_clsid).arg(fld_base_ni_belongto);
    if(!q2.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    SubjectManager* smg;
    QHash<int,QSet<QString> > maps;  //键为名称条目id
    foreach (SubSysNameItem* item, curAccount->getSupportSubSys()) {
        if(!item->isImport)
            continue;        
        smg = curAccount->getSubjectManager(item->code);
        FSubItrator* it = smg->getFstSubItrator();
        while(it->hasNext()){
            it->next();
            foreach(SecondSubject* sub, it->value()->getChildSubs()){
                int clsID = sub->getNameItem()->getClassId();
                if(clsID == 2 || clsID == 3) //跳过金融机构和业务客户
                    continue;
                int niId = sub->getNameItem()->getId();
                if(!maps.contains(niId))
                    maps[niId] = QSet<QString>();
                maps[niId].insert(QString("%1-%2").arg(item->code).arg(sub->getParent()->getCode()));
            }
        }        
    }
    QHashIterator<int,QSet<QString> > ie(maps);
    while(ie.hasNext()){
        ie.next();
        QStringList codes;
        QSetIterator<QString> is(ie.value());
        while(is.hasNext()){
            codes<<is.next();
        }
        SubjectNameItem* ni = smg->getNameItem(ie.key());
        q.bindValue(":str",ni->getShortName());
        q.bindValue(":codes",codes.isEmpty()?"":codes.join(","));
        if(!q.exec())
            return false;
        if(q.numRowsAffected() == 0){
            q2.bindValue(":name",ni->getShortName());
            q2.bindValue(":lname",ni->getLongName());
            q2.bindValue(":remcode",ni->getRemCode());
            q2.bindValue(":cls",ni->getClassId());
            q2.bindValue(":belongto",codes.join(","));
            if(!q2.exec())
                return false;
        }
    }
    if(!bdb.commit()){
        bdb.rollback();
        return false;
    }
    return true;
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
//void MainWindow::on_actSetPzCls_triggered()
//{
//    if(!curSuiteMgr->getCurPz())
//        return;
//    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
//        return;
//    int key = curSuiteMgr->getSuiteRecord()->id;
//    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT))
//        return;
//    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
//    if(!w)
//        return;

//    QDialog* dlg = new QDialog;
//    QLabel* lbl = new QLabel(tr("凭证类别"),dlg);
//    QComboBox* cmb = new QComboBox(dlg);
//    QList<PzClass> codes = pzClasses.keys();
//    qSort(codes.begin(),codes.end());
//    foreach(PzClass c, codes)
//        cmb->addItem(pzClasses.value(c),(int)c);
//    int index = cmb->findData(curSuiteMgr->getCurPz()->getPzClass());
//    cmb->setCurrentIndex(index);
//    QHBoxLayout* lh = new QHBoxLayout;
//    lh->addWidget(lbl);
//    lh->addWidget(cmb);
//    QPushButton* btnOk = new QPushButton(tr("确定"));
//    QPushButton* btnCancel = new QPushButton(tr("取消"));
//    QHBoxLayout* lb = new QHBoxLayout;
//    lb->addWidget(btnOk);
//    lb->addWidget(btnCancel);
//    QVBoxLayout* lm = new QVBoxLayout;
//    lm->addLayout(lh);
//    lm->addLayout(lb);
//    dlg->setLayout(lm);
//    dlg->resize(200,300);
//    connect(btnOk,SIGNAL(clicked()),dlg,SLOT(accept()));
//    connect(btnCancel,SIGNAL(clicked()),dlg,SLOT(reject()));
//    if(dlg->exec() == QDialog::Rejected){
//        delete dlg;
//        return;
//    }
//    PzClass c = (PzClass)cmb->itemData(cmb->currentIndex()).toInt();
//    curSuiteMgr->getCurPz()->setPzClass(c);  //这里避开了undo框架，这是个缺陷
//    delete dlg;
//}

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
            dim = new SubWindowDim;
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
//    if(!curSuiteMgr->isPzSetOpened()){
//        pzsWarning();
//        return;
//    }
//    QDialog* dlg = new QDialog(this);
//    QLabel* ly = new QLabel(tr("凭证所属年月"),dlg);
//    QDateEdit* date = new QDateEdit(dlg);
//    date->setDate(QDate(curSuiteMgr->year(),curSuiteMgr->month(),1));
//    date->setDisplayFormat("yyyy-M");
//    date->setReadOnly(false);
//    QLabel* ln = new QLabel(tr("凭证号"),dlg);
//    QLineEdit* edtPzNums = new QLineEdit(dlg);
//    QPushButton* btnOk = new QPushButton(tr("确定"));
//    QPushButton* btnCancel = new QPushButton(tr("取消"));
//    QHBoxLayout* ld = new QHBoxLayout;
//    QHBoxLayout* lb = new QHBoxLayout;
//    QHBoxLayout* ls = new QHBoxLayout;
//    QVBoxLayout* lm = new QVBoxLayout;
//    ld->addWidget(ly);
//    ld->addWidget(date);
//    ls->addWidget(ln);
//    ls->addWidget(edtPzNums);
//    lb->addWidget(btnOk);
//    lb->addWidget(btnCancel);
//    lm->addLayout(ld);
//    lm->addLayout(ls);
//    lm->addLayout(lb);
//    dlg->setLayout(lm);
//    connect(btnOk,SIGNAL(clicked()),dlg,SLOT(accept()));
//    connect(btnCancel, SIGNAL(clicked()),dlg, SLOT(reject()));
//    if(QDialog::Accepted != dlg->exec())
//        return;
//    QString ds = date->date().toString(Qt::ISODate);
//    ds.chop(3);
//    QStringList pzs = edtPzNums->text().split(",");
//    if(pzs.empty())
//        return;

//    QList<int> pzNums;
//    bool ok = false;
//    int numStart,numEnd;
//    foreach(QString s, pzs){
//        s =  s.trimmed();
//        numStart = s.toInt(&ok);
//        if(ok)
//            pzNums << numStart;
//        else{
//            int index = s.indexOf("-");
//            if(index != -1){
//                numStart = s.left(index).toInt(&ok);
//                if(!ok){
//                    qDebug()<<QString("String '%1' transform to number failed!").arg(s.left(index+1));
//                    return;
//                }
//                numEnd = s.right(s.size()-index-1).toInt(&ok);
//                if(!ok){
//                    qDebug()<<QString("String '%1' transform to number failed!").arg(s.right(s.size()-index-1));
//                    return;
//                }
//                for(int i = numStart; i <= numEnd; ++i)
//                    pzNums<<i;
//            }
//        }
//    }


//    delete dlg;

//    QString pzNumStr;
//    foreach(int n,pzNums)
//        pzNumStr.append(QString::number(n)).append(",");
//    pzNumStr.chop(1);
//    if(QMessageBox::No == QMessageBox::warning(this,
//           tr("警告信息"),tr("确定要删除 %1 %2号凭证吗？").arg(ds).arg(pzNumStr),
//           QMessageBox::Yes|QMessageBox::No))
//        return;

//    QSqlQuery qp,qb;
//    QString sp,sb;
//    foreach(int pzNum,pzNums){
//        sp = QString("select id from PingZhengs where date like '%1%' and number = %2")
//                .arg(ds).arg(pzNum);
//        if(!qp.exec(sp)){
//            qDebug()<<QString("Excute stationment '%1' failed!").arg(sp);
//            return;
//        }
//        int pid;
//        while(qp.next()){
//            pid = qp.value(0).toInt();
//            sb = QString("delete from BusiActions where pid = %1").arg(pid);
//            if(!qb.exec(sb)){
//                qDebug()<<QString("Excute stationment '%1' failed!").arg(sb);
//                return;
//            }
//            sb = QString("delete from PingZhengs where id = %1").arg(pid);
//            if(!qb.exec(sb)){
//                qDebug()<<QString("Excute stationment '%1' failed!").arg(sb);
//                return;
//            }
//        }
//    }
//    QMessageBox::information(0,tr("提示信息"),tr("成功删除凭证！"));
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
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    LogView* form = NULL;
    if(!commonGroups.contains(SUBWIN_LOGVIEW)){
        dbUtil->getSubWinInfo(SUBWIN_LOGVIEW,winfo,sinfo);
        form = new LogView;
    }
    showCommonSubWin(SUBWIN_NOTEMGR,form,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

//#include <QToolTip>
#include "widgets/subjectselectorcombobox.h"
#include "testform.h"
#include "excel/ExcelUtil.h"
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

//    QString info = tr("科目“%1-%2”的余额发生异常！\n一级科目余额：%3（%4）\n二级科目余额：%5（%6）");
//    QToolTip::showText(QPoint(10,10),info,0);
//    BackupUtil bu;
    //QString fn = bu._fondLastFile(tr("宁波苏航.dat"),BackupUtil::BR_TRANSFERIN);
    //bu.backup(tr("宁波苏航.dat"),BackupUtil::BR_TRANSFERIN);
    //bu.restore(tr("宁波苏航.dat"),BackupUtil::BR_TRANSFERIN);
    //bu.clear();

    //QHash<int,int> fMaps, sMaps;
    //QStringList errors;
    //if(!curAccount->getSubSysJoinMaps(1,2,fMaps,sMaps))
    //    return false;
    //if(!curAccount->getDbUtil()->convertExtraInYear(2013,fMaps,sMaps,errors))
    //    return false;
    //
    //curAccount->getDbUtil()->convertPzInYear(2013,fMaps,sMaps,errors);

//    errors<<"11111"<<"22222"<<"33333";
//    QFile logFile(LOGS_PATH + "subSysUpgrade.log");
//    if(!logFile.open(QIODevice::WriteOnly | QIODevice::Text)){
//        QMessageBox::critical(this,tr("错误提示"),tr("无法将升级日志写入到日志文件"));
//        return false;
//    }
//    QTextStream ds(&logFile);
//    foreach (QString s, errors){
//        ds<<s<<"\n";
//    }
//    ds.flush();

//    QDialog dlg(this);
//    SubjectManager* subMgr = curAccount->getSubjectManager(1);
//    //SubjectSelectorComboBox cmb(subMgr,subMgr->getFstSubject("1131"),
//    //                            SubjectSelectorComboBox::SC_SND,&dlg);
//    SubjectSelectorComboBox cmb(subMgr,subMgr->getFstSubject("1002"),
//                                SubjectSelectorComboBox::SC_FST,&dlg);

//    QHBoxLayout* l = new QHBoxLayout;
//    l->addWidget(&cmb);
//    dlg.setLayout(l);
//    dlg.resize(300,200);
//    dlg.exec();



    //导出常用科目到基本库
    //bool r = exportCommonSubject();
//    QString errors;
//    QSqlQuery qb(AppConfig::getBaseDbConnect());
//    QSqlQuery q(curAccount->getDbUtil()->getDb());
//    QSqlQuery qa(curAccount->getDbUtil()->getDb());

    //将默认科目系统中启用的一级科目配置信息写入到基本库中
//    QStringList codes;
//    FSubItrator* it = curAccount->getSubjectManager(1)->getFstSubItrator();
//    while(it->hasNext()){
//        it->next();
//        if(it->value()->isEnabled()){
//            codes<<it->value()->getCode();
//        }
//    }

//    while(it->hasNext()){
//        it->next();
//        if(it->value()->getJdDir())
//            codes<<it->value()->getCode();
//    }
    //bool r = AppConfig::getInstance()->setEnabledFstSubs(1,codes);
//    bool r = AppConfig::getInstance()->setSubjectJdDirs(1,codes);

//    QHash<QString, QString> codeMaps,maps;
//    AppConfig::getInstance()->getSubSysMaps2(1,2,codeMaps,maps);
//    QList<QString> codes = maps.values("1123");

//    SubjectManager* sm = curAccount->getSubjectManager(1);
//    FirstSubject* fsub = sm->getFstSubject("1151");
//    bool b = curAccount->getDbUtil()->verifyExtraForFsub(2013,12,fsub);

//    PzTemplateParameter pt;
//    AppConfig* config = AppConfig::getInstance();
//    bool r = config->getPzTemplateParameter(&pt);
//    pt.baRowHeight = 8;
//    pt.factor[4] = 0.11;
//    r = config->savePzTemplateParameter(&pt);

    Double v1(6.16,4),v2(6.1525,4);
    Double v(14128.01);
    v = (v1-v2)*v;
    int i = 0;
}






















