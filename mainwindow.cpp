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
#include <QBuffer>
#include <QShortcut>

#include "ui_mainwindow.h"
#include "mainapplication.h"
#include "mainwindow.h"
#include "tables.h"
#include "global.h"
#include "databaseaccessform.h"
#include "utils.h"
#include "dialog2.h"
#include "dialog3.h"
#include "printUtils.h"
#include "securitys.h"
#include "commdatastruct.h"
#include "viewpzseterrorform.h"
#include "aboutdialog.h"
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
#include "crtaccountfromolddlg.h"
#include "transfers.h"
#include "ysyfinvoicestatform.h"
#include "curinvoicestatform.h"
#include "searchdialog.h"
#include "jxtaxmgrform.h"




/////////////////////////////////PaStatusBar////////////////////////////////
PaStatusBar::PaStatusBar(QWidget *parent):QStatusBar(parent)
{
    timer = NULL;
    QLabel *l = new QLabel(tr("当前帐套:"),this);
    curSuite.setText("         ");
    curSuite.setObjectName("InfoSecInStatus");
    QHBoxLayout* hl1 = new QHBoxLayout;
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

    QHBoxLayout* hl2 = new QHBoxLayout;
    hl2->addWidget(l);    hl2->addWidget(&pzSetState);
    l = new QLabel(tr("余额状态："),this);
    extraState.setObjectName("InfoSecInStatus");
    hl2->addWidget(l);    hl2->addWidget(&extraState);

    l = new QLabel(tr("凭证总数："),this);
    pzCount.setAttribute(Qt::WA_AlwaysShowToolTips,true);
    pzCount.setObjectName("InfoSecInStatus");
    pzCount.setText("   ");
    QHBoxLayout* hl3 = new QHBoxLayout;
    hl3->addWidget(l);
    hl3->addWidget(&pzCount);

    l = new QLabel(tr("登录用户："),this);
    lblUser.setObjectName("InfoSecInStatus");
    lblUser.setText("           ");
    QHBoxLayout* hl4 = new QHBoxLayout;
    hl4->addWidget(l);   hl4->addWidget(&lblUser);

    l = new QLabel(tr("工作站："),this);
    lblWs.setObjectName("InfoSecInStatus");
    lblUser.setText("           ");
    QHBoxLayout* hl5 = new QHBoxLayout;
    hl5->addWidget(l);   hl5->addWidget(&lblWs);


    QFrame *f = new QFrame(this);
    f->setObjectName("FrameInStatus");
    f->setFrameShape(QFrame::StyledPanel);
    f->setFrameShadow(QFrame::Sunken);
    QHBoxLayout* ml = new QHBoxLayout(f);
    ml->addLayout(hl1);ml->addLayout(hl2);
    ml->addLayout(hl3);ml->addLayout(hl4);
    ml->addLayout(hl5);

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

void PaStatusBar::setWorkStation(WorkStation *mac)
{
    if(mac){
        lblWs.setText(mac->name());
        lblWs.setToolTip(mac->description());
    }
    else{
        lblWs.clear();
        lblWs.setToolTip("");
    }
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
    QByteArray* commonState=0,*properState=0;
    SubWindowDim* dim = new SubWindowDim;
    dim->x = subWin->x();
    dim->y = subWin->y();
    dim->w = subWin->width();
    dim->h = subWin->height();
    if(t == SUBWIN_PZEDIT){
        PzDialog* w = static_cast<PzDialog*>(subWin->widget());
        commonState = w->getCommonState();
        delete w;
    }
    else if(t == SUBWIN_HISTORYVIEW){
        HistoryPzForm* w = static_cast<HistoryPzForm*>(subWin->widget());
        commonState = w->getCommonState();
        delete w;
    }
    else if(t == SUBWIN_DETAILSVIEW){
        ShowDZDialog* w = static_cast<ShowDZDialog*>(subWin->widget());
        commonState = w->getCommonState();
        disconnect(w,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
        delete w;
    }
    else if(t == SUBWIN_PZSEARCH){
        PzSearchDialog* w = static_cast<PzSearchDialog*>(subWin->widget());
        commonState = w->getCommonState();
        disconnect(w,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
        delete w;
    }
    else if(t == SUBWIN_PZSTAT){
        CurStatDialog* w = static_cast<CurStatDialog*>(subWin->widget());
        commonState = w->getCommonState();
        properState = w->getProperState();
        delete w;
    }
    else if(t == SUBWIN_VIEWPZSETERROR){
        ViewPzSetErrorForm* w = static_cast<ViewPzSetErrorForm*>(subWin->widget());
        commonState = w->getState();
        disconnect(w,SIGNAL(reqLoation(int,int)),this,SLOT(openSpecPz(int,int)));
        delete w;
    }
    else if(t == SUBWIN_LOOKUPSUBEXTRA){
        ApcData* w = static_cast<ApcData*>(subWin->widget());
        if(w){
            properState = w->getProperState();
        }
    }
    else if(t == SUBWIN_INCOST){
        CurInvoiceStatForm* w = static_cast<CurInvoiceStatForm*>(subWin->widget());
        if(w)
            disconnect(w,SIGNAL(openRelatedPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    AppConfig::getInstance()->saveSubWinInfo(t,dim,commonState);
    if(curAccount)
        curAccount->getDbUtil()->saveSubWinInfo(t,properState);
    subWinHashs.remove(subWin->getWindowType());
    delete dim;
    if(commonState)
        delete commonState;
    parent->removeSubWindow(subWin);
    emit specSubWinClosed(t);
}


///////////////////////////////////////MainWindow////////////////////////////////

int mdiAreaWidth;
int mdiAreaHeight;

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent),ui(new Ui::MainWindow),
    isMinimizeToTray_(false),lockObj(0),tagLock(false)
{
    ui->setupUi(this);
    setCentralWidget(ui->mdiArea);
    appCon = AppConfig::getInstance();
    _catchTimer = 0;
    if(appCon->isAutoHideLeftDock()){
        ui->mdiArea->setMouseTracking(true);
        setMouseTracking(true);
        _catchTimer = new QTimer(this);
        _catchTimer->setSingleShot(true);
        connect(_catchTimer,SIGNAL(timeout()),this,SLOT(catchMouse()));
    }

    curSuiteMgr = NULL;
    sortBy = true; //默认按凭证号进行排序
    dbUtil = NULL;
    undoStack = NULL;
    undoView = NULL;
    curSSPanel = NULL;
    etMapper = NULL;

    initActions();
    initToolBar();
    initTvActions();
    initExternalTools();
    ui->statusbar->setWorkStation(appCon->getLocalStation());

    mdiAreaWidth = ui->mdiArea->width();
    mdiAreaHeight = ui->mdiArea->height();


    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)),
            this, SLOT(setActiveSubWindow(QWidget*)));
    connect(ui->mdiArea,SIGNAL(subWindowActivated(QMdiSubWindow*)),this,SLOT(subWindowActivated(QMdiSubWindow*)));

    loadSettings();
    sc_lock = new QShortcut(QKeySequence(tr("Ctrl+L")),this);
    connect(sc_lock,SIGNAL(activated()),this,SLOT(lockWindow()));
    rfLogin();
    ui->statusbar->setUser(curUser);
    if(!curUser)
        return;

    bool ok = true;
    createTray();
    AccountCacheItem* ci = appCon->getRecendOpenAccount();
    if(ci){
        if(!AccountVersionMaintain(ci->fileName)){
            setAppTitle();
            return;
        }        
        curAccount = new Account(ci->fileName);
        if(!curAccount->isValid()){
            myHelper::ShowMessageBoxQuesion(tr("账户文件无效，请检查账户文件内信息是否齐全！！"));
            ok = false;
        }
        else if(!curAccount->canAccess(curUser)){
            myHelper::ShowMessageBoxQuesion(tr("当前登录用户不能访问账户（%1），请以合适的用户登录！").arg(curAccount->getSName()));
            ok = false;
        }
    }
    else
        ok = false;
    if(!ok){
        if(curAccount)
            delete curAccount;
        curAccount = NULL;
        setAppTitle();
        return;
    }
    accountInit(ci);
    setAppTitle();
    rfMainAct(curUser);
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
    if(!QFile::exists(DATABASE_PATH+fname)){
        myHelper::ShowMessageBoxWarning(tr("账户文件“%1”不存在！").arg(fname));
        return false;
    }
    VersionManager vm(VersionManager::MT_ACC,fname);
    VersionUpgradeInspectResult result = vm.isMustUpgrade();
    bool exec = false;
    switch(result){
    case VUIR_CANT:
        myHelper::ShowMessageBoxWarning(tr("该账户数据库版本不能归集到初始版本！"));
        return false;
    case VUIR_DONT:
        exec = false;
        break;
    case VUIR_LOW:
        myHelper::ShowMessageBoxWarning(tr("当前程序版本太低，必须要用更新版本的程序打开此账户！"));
        return false;
    case VUIR_MUST:
        exec = true;
        break;
    }
    if(exec){
        if(vm.exec() == QDialog::Rejected){
            myHelper::ShowMessageBoxWarning(tr("该账户数据库版本过低，必须先升级！"));
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

    //分录模板工具条
    connect(ui->actBankIncome,SIGNAL(triggered()),this,SLOT(openBusiTemplate()));
    connect(ui->actYsIncome,SIGNAL(triggered()),this,SLOT(openBusiTemplate()));
    connect(ui->actBankCost,SIGNAL(triggered()),this,SLOT(openBusiTemplate()));
    connect(ui->actYfCost,SIGNAL(triggered()),this,SLOT(openBusiTemplate()));
    connect(ui->actYsGather,SIGNAL(triggered()),this,SLOT(openBusiTemplate()));
    connect(ui->actYfGather,SIGNAL(triggered()),this,SLOT(openBusiTemplate()));
    connect(ui->actInvoiceStat,SIGNAL(triggered()),this,SLOT(openBusiTemplate()));
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

    QToolButton* btn = qobject_cast<QToolButton*>(ui->tbrMain->widgetForAction(ui->actInAccount));
    if(btn){
        QMenu* m = new QMenu(this);
        m->addAction(ui->actBatchImport);
        btn->setMenu(m);
        //btn->setStyleSheet("QToolButton::menu-arrow {border: none;"
        //                   "image: url(:/styles/arrow-down.png);}");
    }
    btn = qobject_cast<QToolButton*>(ui->tbrMain->widgetForAction(ui->actEmpAccount));
    if(btn){
        QMenu* m = new QMenu(this);
        m->addAction(ui->actBatchExport);
        btn->setMenu(m);
    }
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
        addDockWidget(Qt::RightDockWidgetArea, dw);
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
    if(ci->tState != ATS_TRANSINDES){
        curAccount->setReadOnly(true);
        QString info;
        if(ci->tState == ATS_TRANSOUTED)
            info = tr("账户已转出至“%1”").arg(ci->d_ws->name());
        else if(ci->tState == ATS_TRANSINOTHER)
            info = tr("由“%1”转出，但不是转入至本站！").arg(ci->s_ws->name());
        myHelper::ShowMessageBoxInfo(tr("当前账户以只读模式打开！\n%1").arg(info));
    }
    if(!curSuiteMgr)
        myHelper::ShowMessageBoxInfo(tr("本账户还没有设置任何帐套，请在账户属性设置窗口的帐套页添加一个帐套以供帐务处理！"));
    else
        suiteViewSwitched(NULL,curSuiteMgr);    
    undoStack = curSuiteMgr?curSuiteMgr->getUndoStack():NULL;
    if(!BusiUtil::init(curAccount->getDbUtil()->getDb()))
        myHelper::ShowMessageBoxError(tr("BusiUtil对象初始化阶段发生错误！"));
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
    ui->actJxTaxMgr->setEnabled(curAccount->isJxTaxManaged());
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

bool MainWindow::isContainRight(Right::RightCode rc)
{
    if(!curUser)
        return false;
    if(curUser->isSuperUser())
        return true;
    Right* r = allRights.value(rc);
    if(!r)
        return false;
    if(!curUser->haveRight(r)){
        myHelper::ShowMessageBoxWarning(tr("当前用户不具有执行此操作的权限！"));
        return false;
    }
    return true;
}

/**
 * @brief MainWindow::isContainRights
 * 测试当前用户是否包含指定权限
 * @param rs
 * @return
 */
bool MainWindow::isContainRights(QSet<Right::RightCode> rcs)
{
    if(!curUser)
        return false;
    if(curUser->isSuperUser())
        return true;
    QSet<Right*> rs;
    foreach(Right::RightCode rc, rcs)
        rs.insert(allRights.value(rc));
    if(!curUser->haveRights(rs)){
        myHelper::ShowMessageBoxWarning(tr("当前用户不具有执行此操作的权限！"));
        return false;
    }
    return true;
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
    return curSuiteMgr->isSuiteEditable();
}

/**
 * @brief 凭证集是否可编辑
 * @return
 */
bool MainWindow::isPzSetEditable()
{
    if(!curSuiteMgr)
        return false;
    return curSuiteMgr->isPzSetEditable();
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
    ui->actOption->setEnabled(login);
    ui->actNoteMgr->setEnabled(login);
    //超级用户特权项
    bool r = login && curUser->isSuperUser();
    ui->actSecCon->setEnabled(r);
    ui->actSqlTool->setEnabled(r);
    ui->actManageExternalTool->setEnabled(r);
    ui->actUpdateSql->setEnabled(r);
    ui->actImpPzSet->setEnabled(r);
    ui->actExtComSndSub->setEnabled(r);
    //管理员特权项
    ui->actTaxCompare->setEnabled(login && curUser->isAdmin());
    rfMainAct(curUser);
}

/**
 * @brief MainWindow::rfMainAct
 *  控制账户相关的命令的启用性（在账户打开或关闭时刻调用）
 */
void MainWindow::rfMainAct(bool login)
{
    bool open = curAccount?true:false;
    ui->actCrtAccount->setEnabled(login && curUser->haveRight(allRights.value(Right::Account_Create)) && !open);
    ui->actOpenAccount->setEnabled(login && curUser->haveRight(allRights.value(Right::Account_Common_Open)) && !open);
    ui->actCloseAccount->setEnabled(login && curUser->haveRight(allRights.value(Right::Account_Common_Close)) && open);
    ui->actAccProperty->setEnabled(login && open && curUser->canAccessAccount(curAccount));
    ui->actRefreshActInfo->setEnabled(login && curUser->haveRight(allRights.value(Right::Account_Refresh)) && !open);
    ui->actDelAcc->setEnabled(login && curUser->haveRight(allRights.value(Right::Account_Remove)) && !open);
    ui->actInAccount->setEnabled(login && curUser->haveRight(allRights.value(Right::Account_Import)) && !open);
    ui->actEmpAccount->setEnabled(login && curUser->haveRight(allRights.value(Right::Account_Export)) && !open);
    rfSuiteAct();
}

/**
 * @brief 控制当帐套切换后，菜单项的启用性
 */
void MainWindow::rfSuiteAct()
{
    bool open = curUser && curSuiteMgr?true:false;
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

    bool open = (curUser && curSuiteMgr && curSuiteMgr->isPzSetOpened())?true:false;
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
    if(!isContainRight(Right::Account_Create))
        return;
    NABaseInfoDialog dlg(this);
    dlg.exec();
}

void MainWindow::openAccount()
{
    if(!isContainRight(Right::Account_Common_Open))
        return;
    OpenAccountDialog* dlg = new OpenAccountDialog(this);
    if(dlg->exec() != QDialog::Accepted)
        return;

    AccountCacheItem* ci =dlg->getAccountCacheItem();
    if(!ci || !AccountVersionMaintain(ci->fileName)){
        setAppTitle();
        rfMainAct(curUser);
        return;
    }
    if(curAccount){
        delete curAccount;
        curAccount = NULL;
    }
    bool ok = true;
    curAccount = new Account(ci->fileName);
    if(!curAccount->isValid()){
        myHelper::ShowMessageBoxWarning(tr("账户文件无效，请检查账户文件内信息是否齐全！！"));
        ok = false;
    }
    else if(!curAccount->canAccess(curUser)){
        myHelper::ShowMessageBoxWarning(tr("当前登录用户不能访问该账户（%1），请以合适的用户登录！").arg(curAccount->getSName()));
        ok = false;
    }
    if(!ok){
        if(curAccount)
            delete curAccount;
        curAccount = NULL;
        setAppTitle();
        rfMainAct(curUser);
        return;
    }
    setAppTitle();
    appCon->setRecentOpenAccount(ci->code);
    accountInit(ci);
    rfMainAct(curUser);
}

void MainWindow::closeAccount()
{
    if(!isContainRight(Right::Account_Common_Close))
        return;
    _closeAccount();
}

/**
 * @brief 移除账户
 */
void MainWindow::on_actDelAcc_triggered()
{
    //显示当前账户列表供用户选择,将选择的账户从账户缓存中删除
    if(!isContainRight(Right::Account_Remove))
        return;
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
            myHelper::ShowMessageBoxError(tr("在删除该账户的缓存记录时发生错误！"));
            return;
        }
        //将账户文件重命名后拷贝到备份目录中
        BackupUtil backup;
        if(!backup.backup(accItem->fileName,BackupUtil::BR_REMOVE))
            myHelper::ShowMessageBoxWarning(tr("将账户转移至备份目录时发生错误！"));
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
    if(!curUser || curUser && !curUser->isSuperUser())
        return;
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
    if(!curUser || curUser && !curUser->isSuperUser())
        return;
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
    if(!curUser || curUser && !curUser->isSuperUser())
        return;
    if(!exportCommonSubject())
        myHelper::ShowMessageBoxWarning(tr("导出过程出错，请查看日志！"));
}

/**
 * @brief 打开选项配置窗口
 */
void MainWindow::on_actOption_triggered()
{
    QByteArray* sinfo = new QByteArray;
    SubWindowDim* winfo = NULL;
    ConfigPanels* form = NULL;
    if(!commonGroups.contains(SUBWIN_OPTION)){
        appCon->getSubWinInfo(SUBWIN_OPTION,winfo,sinfo);
        form = new ConfigPanels(sinfo);
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
        appCon->getSubWinInfo(SUBWIN_EXTERNALTOOLS,winfo);
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
        appCon->getSubWinInfo(SUBWIN_TAXCOMPARE,winfo);
        form = new TaxesComparisonForm(curSuiteMgr);
        connect(form,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    showCommonSubWin(SUBWIN_TAXCOMPARE,form,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
#else
    myHelper::ShowMessageBoxInfo(tr("此功能目前仅在Windows平台下可用！"));
#endif
}

void MainWindow::on_actNoteMgr_triggered()
{
    QByteArray* sinfo = NULL;
    SubWindowDim* winfo = NULL;
    NoteMgrForm* form = NULL;
    if(!commonGroups.contains(SUBWIN_NOTEMGR)){
        appCon->getSubWinInfo(SUBWIN_NOTEMGR,winfo);
        form = new NoteMgrForm(curAccount);
    }
    showCommonSubWin(SUBWIN_NOTEMGR,form,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

/**
 * @brief 导出应用设置信息（包括工作站、组、用户、权限和权限类型等）
 */
void MainWindow::on_actExpAppCfg_triggered()
{
    if(!curUser || curUser && !curUser->isSuperUser())
        return;
    QDialog dlg(this);
    QLabel title(tr("选择要导出的部分："),&dlg);
    QCheckBox chkRightType(tr("权限类型"), &dlg);
    QCheckBox chkRight(tr("权限"), &dlg);
    QCheckBox chkMac(tr("工作站"), &dlg);
    QCheckBox chkGroup(tr("用户组"), &dlg);
    QCheckBox chkUser(tr("用户"), &dlg);
    QPushButton btnOk(tr("确定"), &dlg);
    QPushButton btnCancel(tr("取消"), &dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb; lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&title);lm->addWidget(&chkRightType);
    lm->addWidget(&chkRight); lm->addWidget(&chkMac);
    lm->addWidget(&chkGroup); lm->addWidget(&chkUser);
    lm->addLayout(&lb);
    dlg.setLayout(lm);
    if(dlg.exec() == QDialog::Rejected)
        return;
    if(!chkRight.isChecked() && !chkRightType.isChecked() && !chkMac.isChecked()
            && !chkGroup.isChecked() && !chkUser.isChecked())
        return;

    //默认导出目录
    QDir dir("./exports/securitys/");
    if(!dir.exists() && !dir.mkpath("."))
        dir.setPath(QApplication::applicationDirPath());
    QString dirName = QFileDialog::getExistingDirectory(this,tr("选择导出的目录"),dir.path());
    bool fileExist;
    if(chkRightType.isChecked()){
        QList<RightType*> rts;
        rts = allRightTypes.values();
        qSort(rts.begin(),rts.end(),rightTypeByCode);
        QString fileName = dirName + "/rightTypes.txt";
        fileExist = QFile::exists(fileName);
        if(!fileExist || fileExist && (QDialog::Accepted == myHelper::ShowMessageBoxQuesion(tr("文件“rightTypes.txt”已存在，要覆盖吗？")))){
            QFile file(fileName);
            if(!file.open(QFile::WriteOnly|QFile::Text)){
                myHelper::ShowMessageBoxError(tr("打开文件“rightTypes.txt”时出错！"));
                return;
            }
            QTextStream ts(&file);
            int mv, sv;
            appCon->getAppCfgVersion(mv,sv,BDVE_RIGHTTYPE);
            ts<<QString("version=%1.%2\n").arg(mv).arg(sv);
            foreach(RightType* rt, rts)
                ts<<rt->serialToText()<<"\n";
            ts.flush();
            file.close();
        }
    }
    if(chkRight.isChecked()){
        QList<Right*> rs;
        rs = allRights.values();
        qSort(rs.begin(),rs.end(),rightByCode);
        QString fileName = dirName + "/rights.txt";
        fileExist = QFile::exists(fileName);
        if(!fileExist || fileExist && (QDialog::Accepted == myHelper::ShowMessageBoxQuesion(tr("文件“rights.txt”已存在，要覆盖吗？")))){
            QFile file(fileName);
            if(!file.open(QFile::WriteOnly|QFile::Text)){
                myHelper::ShowMessageBoxError(tr("打开文件“rights.txt”时出错！"));
                return;
            }
            QTextStream ts(&file);
            int mv, sv;
            appCon->getAppCfgVersion(mv,sv,BDVE_RIGHT);
            ts<<QString("version=%1.%2\n").arg(mv).arg(sv);
            foreach(Right* r, rs)
                ts<<r->serialToText()<<"\n";
            ts.flush();
            file.close();
        }
    }
    if(chkMac.isChecked()){
        QList<WorkStation*> macs;
        macs = appCon->getAllMachines().values();
        qSort(macs.begin(),macs.end(),byMacMID);
        QString fileName = dirName + "/machines.txt";
        fileExist = QFile::exists(fileName);
        if(!fileExist || fileExist && (QDialog::Accepted == myHelper::ShowMessageBoxQuesion(tr("文件“machines.txt”已存在，要覆盖吗？")))){
            QFile file(fileName);
            if(!file.open(QFile::WriteOnly|QFile::Text)){
                myHelper::ShowMessageBoxError(tr("打开文件“machines.txt”时出错！"));
                return;
            }
            QTextStream ts(&file);
            int mv, sv;
            appCon->getAppCfgVersion(mv,sv,BDVE_WORKSTATION);
            ts<<QString("version=%1.%2\n").arg(mv).arg(sv);
            foreach(WorkStation* m, macs)
                ts<<m->serialToText()<<"\n";
            ts.flush();
            file.close();
        }
    }
    if(chkGroup.isChecked()){
        QList<UserGroup*> gs = allGroups.values();
        qSort(gs.begin(),gs.end(),groupByCode);
        QString fileName = dirName + "/groups.txt";
        fileExist = QFile::exists(fileName);
        if(!fileExist || fileExist && (QDialog::Accepted == myHelper::ShowMessageBoxQuesion(tr("文件“groups.txt”已存在，要覆盖吗？")))){
            QFile file(fileName);
            if(!file.open(QFile::WriteOnly|QFile::Text)){
                myHelper::ShowMessageBoxError(tr("打开文件“groups.txt”时出错！"));
                return;
            }
            QTextStream ts(&file);
            int mv, sv;
            appCon->getAppCfgVersion(mv,sv,BDVE_GROUP);
            ts<<QString("version=%1.%2\n").arg(mv).arg(sv);
            foreach(UserGroup* g, gs)
                ts<<g->serialToText()<<"\n";
            ts.flush();
            file.close();
        }
    }
    if(chkUser.isChecked()){
        QList<User*> us = allUsers.values();
        qSort(us.begin(),us.end(),userByCode);
        QString fileName = dirName + "/users.txt";
        fileExist = QFile::exists(fileName);
        if(!fileExist || fileExist && (QDialog::Accepted == myHelper::ShowMessageBoxQuesion(tr("文件“users.txt”已存在，要覆盖吗？")))){
            QFile file(fileName);
            if(!file.open(QFile::WriteOnly|QFile::Text)){
                myHelper::ShowMessageBoxError(tr("打开文件“users.txt”时出错！"));
                return;
            }
            QTextStream ts(&file);
            int mv, sv;
            appCon->getAppCfgVersion(mv,sv,BDVE_USER);
            ts<<QString("version=%1.%2\n").arg(mv).arg(sv);
            foreach(User* u, us)
                ts<<u->serialToText()<<"\n";
            ts.flush();
            file.close();
        }
    }
    myHelper::ShowMessageBoxInfo(tr("操作成功完成！"));
}

/**
 * @brief 导入应用配置信息（包括工作站、组、用户、权限和权限类型等）
 */
void MainWindow::on_actImpAppCfg_triggered()
{
    if(!curUser || curUser && !curUser->isSuperUser())
        return;
    QDialog dlg(this);
    QLabel title(tr("选择要导入的部分："),&dlg);
    QCheckBox chkRightType(tr("权限类型"), &dlg);
    QCheckBox chkRight(tr("权限"), &dlg);
    QCheckBox chkMac(tr("工作站"), &dlg);
    QCheckBox chkGroup(tr("用户组"), &dlg);
    QCheckBox chkUser(tr("用户"), &dlg);
    QCheckBox chkPhrase(tr("常用提示短语"),&dlg);
    QPushButton btnOk(tr("确定"), &dlg);
    QPushButton btnCancel(tr("取消"), &dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb; lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&title);lm->addWidget(&chkRightType);
    lm->addWidget(&chkRight); lm->addWidget(&chkMac);
    lm->addWidget(&chkGroup); lm->addWidget(&chkUser);
    lm->addWidget(&chkPhrase);
    lm->addLayout(&lb);
    dlg.setLayout(lm);
    if(dlg.exec() == QDialog::Rejected)
        return;
    if(!chkRight.isChecked() && !chkRightType.isChecked() && !chkMac.isChecked()
            && !chkGroup.isChecked() && !chkUser.isChecked() && !chkPhrase.isChecked())
        return;

    //默认导入目录
    QDir dir("./exports/securitys/");
    if(!dir.exists() && !dir.mkpath("."))
        dir.setPath(QApplication::applicationDirPath());
    QString dirName = QFileDialog::getExistingDirectory(this,tr("选择导出的目录"),dir.path());
    QString fileName;
    QFile file;  QTextStream ts;
    int mv,sv;
    bool needRestart = false;
    if(chkRightType.isChecked()){
        fileName = dirName + "/rightTypes.txt";
        if(!QFile::exists(fileName)){
            myHelper::ShowMessageBoxWarning(tr("导入文件“rightTypes.txt”不存在！"));
            return;
        }
        file.setFileName(fileName);
        if(!file.open(QFile::ReadOnly|QFile::Text)){
            myHelper::ShowMessageBoxError(tr("打开文件“rightTypes.txt”时出错！"));
            return;
        }
        ts.setDevice(&file);
        if(!inspectVersionBeforeImport(ts.readLine(),BDVE_RIGHTTYPE,"rightTypes.txt",mv,sv))
            return;
        QHash<int,RightType*> rts = allRightTypes;
        allRightTypes.clear();
        QList<RightType*> rightTypes;
        while(!ts.atEnd()){
            QString text = ts.readLine();
            RightType* rt = RightType::serialFromText(text,allRightTypes);
            if(!rt){
                myHelper::ShowMessageBoxWarning(tr("导入权限类型时出现错误，请查看日志！"));
                allRightTypes = rts;
                return;
            }
            rightTypes<<rt;
            allRightTypes[rt->code] = rt;
        }
        if(!appCon->clearAndSaveRightTypes(rightTypes,mv,sv)){
            myHelper::ShowMessageBoxError(tr("在保存导入的权限类型时发生错误！"));
            allRightTypes = rts;
            return;
        }
        needRestart = true;
    }
    if(chkRight.isChecked()){
        fileName = dirName + "/rights.txt";
        if(!QFile::exists(fileName)){
            myHelper::ShowMessageBoxWarning(tr("导入文件“rights.txt”不存在！"));
            return;
        }
        file.setFileName(fileName);
        if(!file.open(QFile::ReadOnly|QFile::Text)){
            myHelper::ShowMessageBoxError(tr("打开文件“rights.txt”时出错！"));
            return;
        }
        ts.setDevice(&file);
        if(!inspectVersionBeforeImport(ts.readLine(),BDVE_RIGHT,"rights.txt",mv,sv))
            return;
        QHash<int,Right*> rs = allRights;
        allRights.clear();
        QList<Right*> rights;
        while(!ts.atEnd()){
            QString text = ts.readLine();
            Right* r = Right::serialFromText(text,allRightTypes);
            if(!r){
                myHelper::ShowMessageBoxWarning(tr("导入权限类型时出现错误，请查看日志！"));
                allRights = rs;
                return;
            }
            rights<<r;
            allRights[r->getCode()] = r;
        }
        if(!appCon->clearAndSaveRights(rights,mv,sv)){
            myHelper::ShowMessageBoxError(tr("在保存导入的权限时发生错误！"));
            allRights = rs;
            return;
        }
        needRestart = true;
    }
    if(chkGroup.isChecked()){
        fileName = dirName + "/groups.txt";
        if(!QFile::exists(fileName)){
            myHelper::ShowMessageBoxWarning(tr("导入文件“groups.txt”不存在！"));
            return;
        }
        file.setFileName(fileName);
        if(!file.open(QFile::ReadOnly|QFile::Text)){
            myHelper::ShowMessageBoxError(tr("打开文件“groups.txt”时出错！"));
            return;
        }
        ts.setDevice(&file);
        if(!inspectVersionBeforeImport(ts.readLine(),BDVE_GROUP,"groups.txt",mv,sv))
            return;
        QHash<int,UserGroup*> gs = allGroups;
        allGroups.clear();
        QList<UserGroup*> groups;
        while(!ts.atEnd()){
            QString text = ts.readLine();
            UserGroup* g = UserGroup::serialFromText(text,allRights,allGroups);
            if(!g){
                myHelper::ShowMessageBoxWarning(tr("导入权限类型时出现错误，请查看日志！"));
                allGroups = gs;
                return;
            }
            groups<<g;
            allGroups[g->getGroupCode()] = g;
        }
        if(!appCon->clearAndSaveGroups(groups,mv,sv)){
            myHelper::ShowMessageBoxError(tr("在保存导入的组配置信息时发生错误！"));
            allGroups = gs;
            return;
        }
        needRestart = true;
    }
    if(chkUser.isChecked()){
        fileName = dirName + "/users.txt";
        if(!QFile::exists(fileName)){
            myHelper::ShowMessageBoxWarning(tr("导入文件“users.txt”不存在！"));
            return;
        }
        file.setFileName(fileName);
        if(!file.open(QFile::ReadOnly|QFile::Text)){
            myHelper::ShowMessageBoxError(tr("打开文件“users.txt”时出错！"));
            return;
        }
        ts.setDevice(&file);
        if(!inspectVersionBeforeImport(ts.readLine(),BDVE_USER,"users.txt",mv,sv))
            return;
        QHash<int,User*> us = allUsers; //备份全局用户表
        allUsers.clear();
        QList<User*> users;
        while(!ts.atEnd()){
            QString text = ts.readLine();
            User* u = User::serialFromText(text,allRights,allGroups);
            if(!u){
                myHelper::ShowMessageBoxWarning(tr("导入权限类型时出现错误，请查看日志！"));
                allUsers = us;  //恢复全局用户表
                return;
            }
            users<<u;
            allUsers[u->getUserId()] = u;
        }
        if(!appCon->clearAndSaveUsers(users,mv,sv)){
            myHelper::ShowMessageBoxError(tr("在保存导入的用户配置信息时发生错误！"));
            allUsers = us;
            return;
        }
        needRestart = true;
    }
    if(chkMac.isChecked()){
        fileName = dirName + "/machines.txt";
        if(!QFile::exists(fileName)){
            myHelper::ShowMessageBoxWarning(tr("导入文件“machines.txt”不存在！"));
            return;
        }
        file.setFileName(fileName);
        if(!file.open(QFile::ReadOnly|QFile::Text)){
            myHelper::ShowMessageBoxError(tr("打开文件“machines.txt”时出错！"));
            return;
        }
        ts.setDevice(&file);
        if(!inspectVersionBeforeImport(ts.readLine(),BDVE_WORKSTATION,"machines.txt",mv,sv))
            return;
        QList<WorkStation*> macs;
        while(!ts.atEnd()){
            QString text = ts.readLine();
            WorkStation* mac = WorkStation::serialFromText(text);
            if(!mac){
                myHelper::ShowMessageBoxWarning(tr("导入权限类型时出现错误，请查看日志！"));
                return;
            }
            macs<<mac;
        }
        if(!appCon->clearAndSaveMacs(macs,mv,sv)){
            myHelper::ShowMessageBoxError(tr("保存导入的工作站配置信息时发生错误！"));
            return;
        }
        appCon->refreshMachines();
        needRestart = true;
    }
    if(chkPhrase.isChecked()){
        fileName = dirName + "/phrases.txt";
        if(!QFile::exists(fileName)){
            myHelper::ShowMessageBoxWarning(tr("导入文件“phrases.txt”不存在！"));
            return;
        }
        file.setFileName(fileName);
        if(!file.open(QFile::ReadOnly|QFile::Text)){
            myHelper::ShowMessageBoxError(tr("打开文件“phrases.txt”时出错！"));
            return;
        }
        QString s(file.readLine());
        if(!inspectVersionBeforeImport(s,BDVE_COMMONPHRASE,"phrases.txt",mv,sv))
            return;
        file.reset();
        QByteArray ba = file.readAll();
        if(!appCon->serialCommonPhraseFromBinary(&ba)){
            myHelper::ShowMessageBoxError(tr("导入常用提示短语配置信息时发生错误！"));
            return;
        }
    }
    if(needRestart)
        myHelper::ShowMessageBoxInfo(tr("操作成功完成，需要重新启动应用以使新的设置生效！"));
    else
        myHelper::ShowMessageBoxInfo(tr("操作成功完成！"));
}

void MainWindow::_closeAccount()
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
            historyPzSet[it.key()].clear();
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
    setAppTitle();
    rfMainAct(curUser);
}



/**
 * @brief MainWindow::isExecAccountTransform
 *  是否运行执行账户转移操作
 * @return
 */
bool MainWindow::isExecAccountTransform()
{
    if(!appCon->getLocalStation()){
        myHelper::ShowMessageBoxWarning(tr("未配置本站，禁止执行转移操作！\n在“工具-选项-工作站配置”中设置本站！"));
        return false;
    }
    if(curUser && !(curUser->isAdmin() || curUser->isSuperUser())){
        myHelper::ShowMessageBoxWarning(tr("只有“管理员”或“超级用户”才能执行账户转移操作！"));
        return false;
    }
    return true;
}



//退出应用
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(tagLock){
        myHelper::ShowMessageBoxWarning(tr("应用已被锁定，请登录后再关闭！"));
        event->ignore();
        return;
    }
    else if(appCon->minToTrayClose()){
        hide();
        event->ignore();
        return;
    }
    exit();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(event->button() != Qt::NoButton)
        return;
    if(!curAccount || !appCon->isAutoHideLeftDock() || !_catchTimer)
        return;
    int x = event->x();
    QDockWidget* dw = dockWindows.value(TV_SUITESWITCH);
    if(dw->isHidden()){
        if(x > 10 && _catchTimer->isActive())
            _catchTimer->stop();
        else if(x < 10 && !_catchTimer->isActive())
            _catchTimer->start(500);
    }
    else{
        //这个实现在鼠标短期离开面板窗口还是会隐藏，不知何故？
        if(x < dw->width() && _catchTimer->isActive())
            _catchTimer->stop();
        else if(x > dw->width() && !_catchTimer->isActive())
            _catchTimer->start(500);
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::catchMouse()
{
    QDockWidget* dw = dockWindows.value(TV_SUITESWITCH);
    if(dw->isHidden())
        dw->show();
    else
        dw->hide();
}

void MainWindow::showMainWindow()
{
    if(isHidden())
        show();
//    if (!trayClick || isHidden()){
//        if (oldState & Qt::WindowFullScreen) {
//        show();
//        } else if (oldState & Qt::WindowMaximized) {
//        showMaximized();
//        } else {
//        showNormal();
//        Settings settings;
//        restoreGeometry(settings.value("GeometryState").toByteArray());
//        }
//        activateWindow();
//      } else {
//        if (minimizingTray_)
//            emit signalPlaceToTray();
//        else
//            showMinimized();
//      }
}

void MainWindow::exit()
{
    ui->mdiArea->closeAllSubWindows();
    if(curAccount)
        closeAccount();    
    close();
    qApp->quit();
}

/**
 * @brief 处理系统托盘图标的鼠标动作
 * @param reason
 */
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Context){
        _actShowMainWindow->setEnabled(isHidden());
    }
    else if(reason == QSystemTrayIcon::DoubleClick)
        showMaximized();
}



///////////////////////数据菜单处理槽部分/////////////////////////////////////
/**
 * @brief MainWindow::viewSubjectExtra
 *  显示科目余额
 */
void MainWindow::viewSubjectExtra()
{
    if(!isContainRight(Right::PzSet_Advance_ShowExtra))
        return;
    if(curSuiteMgr->isDirty())
        curSuiteMgr->save();

    ApcData* w = NULL;
    SubWindowDim* winfo = NULL;
    QByteArray* sinfo = new QByteArray;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_LOOKUPSUBEXTRA)){
        appCon->getSubWinInfo(SUBWIN_LOOKUPSUBEXTRA,winfo);
        dbUtil->getSubWinInfo(SUBWIN_LOOKUPSUBEXTRA,sinfo);
        w = new ApcData(curAccount,false,sinfo,this);
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
    if(!isContainRight(Right::Pz_Common_Show))
        return;
    bool isIn;
    PingZheng* pz = curSuiteMgr->readPz(pid,isIn);
    if(!pz)
        return;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    int month = pz->getDate2().month();
    QByteArray cinfo;
    SubWindowDim* winfo = NULL;
    if(isIn){
        PzDialog* w = NULL;
        if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZEDIT)){
            appCon->getSubWinInfo(SUBWIN_PZEDIT,winfo,&cinfo);
            w = new PzDialog(month,curSuiteMgr,&cinfo);
            w->setWindowTitle(tr("凭证窗口"));
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
            appCon->getSubWinInfo(SUBWIN_HISTORYVIEW,winfo,&cinfo);
            w = new HistoryPzForm(pz,&cinfo);
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_HISTORYVIEW,w,winfo);
        }
        else{
            w = static_cast<HistoryPzForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_HISTORYVIEW));
            w->setPz(pz);
            subWinGroups.value(suiteId)->showSubWindow(SUBWIN_HISTORYVIEW,NULL,NULL);
        }
        w->setCurBa(bid);
        w->setWindowTitle(tr("历史凭证（%1年%2月）").arg(curSuiteMgr->year()).arg(month));
        if(winfo)
            delete winfo;
    }
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
//    AboutForm* form = new AboutForm(aboutStr);
//    form->show();
    AboutDialog* dlg = new AboutDialog(this);
    dlg->show();
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
    QString commandline = tool->commandLine.trimmed();
    if(!tool->parameter.isEmpty())
        commandline.append(" ").append(tool->parameter);
    commandline = commandline.trimmed();
    if(commandline.contains(" ")){
        commandline.insert(0,"\"");
        commandline.append("\"");
    }
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
    if(!isContainRight(Right::Pz_Common_Show))
        return;
    int suiteId = accSmg->getSuiteRecord()->id;
    QByteArray cinfo;
    SubWindowDim* winfo = NULL;
    bool editable = false;

    if(curAccount->isReadOnly() || accSmg->getSuiteRecord()->isClosed || accSmg->getState(month) == Ps_Jzed){
        historyPzSet[suiteId] = accSmg->getHistoryPzSet(month);
        if(historyPzSet.value(suiteId).isEmpty()){
            myHelper::ShowMessageBoxInfo(tr("没有任何凭证可显示！"));
            historyPzSetIndex[suiteId] = -1;
        }
        else{
            historyPzMonth[suiteId] = month;
            historyPzSetIndex[suiteId] = 0;
            HistoryPzForm* w;
            if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_HISTORYVIEW)){
                appCon->getSubWinInfo(SUBWIN_HISTORYVIEW,winfo,&cinfo);
                w = new HistoryPzForm(historyPzSet.value(suiteId).first(),&cinfo);
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
            appCon->getSubWinInfo(SUBWIN_PZEDIT,winfo,&cinfo);
            PzDialog* w = new PzDialog(month,curSuiteMgr,&cinfo);
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
    if(!isContainRight(Right::PzSet_Common_Open))
        return;
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
    if(!isContainRight(Right::PzSet_Common_Close))
        return;
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

    if(!isContainRight(Right::PzSet_Common_Close))
        return;
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
    QByteArray* cinfo = NULL,*pinfo=NULL;
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
                cinfo = w->getCommonState();
                w->closeAllPage();
                if(curSSPanel)
                    disconnect(w,SIGNAL(suiteChanged()),curSSPanel,SLOT(suiteUpdated()));
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
                cinfo = w->getState();
        }
        else if(winType == SUBWIN_OPTION){
            ConfigPanels* w = static_cast<ConfigPanels*>(subWin->widget());
            if(w)
                cinfo = w->getState();
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
    if(dim){
        appCon->saveSubWinInfo(winType,dim,cinfo);
        delete dim;
        delete cinfo;
    }
    if(pinfo && curAccount){
        dbUtil->saveSubWinInfo(winType,pinfo);
        delete pinfo;
    }
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
        if(!isContainRight(Right::Print_Pz))
            return;
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
                    myHelper::ShowMessageBoxWarning(tr("打印凭证只支持A4纸打印，一张A4纸可以打印两张凭证！"));
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
        if(!isContainRight(Right::Print_DetialTable))
            return;
        dlg->print(pac);
    }
}

/**
 * @brief 本站定义改变
 * @param ws
 */
void MainWindow::localStationChanged(WorkStation *ws)
{
    if(ws)
        ui->statusbar->setWorkStation(ws);
}

/**
 * @brief 打开分录模板窗口
 */
void MainWindow::openBusiTemplate()
{
    int key = curSuiteMgr->getSuiteRecord()->id;
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(!w)
        return;
    BATemplateEnum type;
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    if(action == ui->actBankIncome)
        type = BATE_BANK_INCOME;
    else if(action == ui->actYsIncome)
        type = BATE_YS_INCOME;
    else if(action == ui->actBankCost)
        type = BATE_BANK_COST;
    else if(action == ui->actYfCost)
        type = BATE_YF_COST;
    else if(action == ui->actYsGather)
        type = BATE_YS_GATHER;
    else if(action == ui->actYfGather)
        type = BATE_YF_GATHER;
    else if(action == ui->actInvoiceStat){
        w->openInvoiceStatForm();
        return;
    }
    w->openBusiactionTemplate(type);
}

void MainWindow::lockWindow()
{
    if(!lockObj){
        lockObj = new LockApp(this);
        connect(lockObj,SIGNAL(unlock()),this,SLOT(unlockWindow()));
    }
    tagLock=true;
    adjustInterfaceForLock();
    this->setEnabled(false);
    QApplication::processEvents();
    installEventFilter(lockObj);
}

void MainWindow::unlockWindow()
{
    QDialog d;
    d.setWindowTitle(tr("登录系统"));
    QLabel lu(tr("用户"),&d);
    QLabel lp(tr("密码"),&d);
    QComboBox cmbUsers(&d);
    cmbUsers.addItem(curUser->getName(),curUser->getUserId());
    if(!curUser->isAdmin()){
        foreach(User* u, allUsers.values()){
            if(u->isAdmin())
                cmbUsers.addItem(u->getName(),u->getUserId());
        }
    }
    if(!curUser->isSuperUser()){
        foreach(User* u, allUsers.values()){
            if(u->isSuperUser())
                cmbUsers.addItem(u->getName(),u->getUserId());
        }
    }
    QLineEdit ep(&d);
    ep.setEchoMode(QLineEdit::Password);
    ep.setFocus();
    QGridLayout lg;
    lg.addWidget(&lu,0,0,1,1);
    lg.addWidget(&cmbUsers,0,1,1,1);
    lg.addWidget(&lp,1,0,1,1);
    lg.addWidget(&ep,1,1,1,1);
    QPushButton btnOk(tr("确定"),&d),btnCancel(tr("取消"),&d);
    QHBoxLayout lb;
    lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
    connect(&btnOk,SIGNAL(clicked()),&d,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&d,SLOT(reject()));
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addLayout(&lg); lm->addLayout(&lb);
    d.setLayout(lm);
    d.resize(300,200);
    if(d.exec() == QDialog::Rejected)
        return;
    int uid = cmbUsers.currentData().toInt();
    User* u = allUsers.value(uid);
    if(!u)
        return;
    if(!u->verifyPw(ep.text())){
        myHelper::ShowMessageBoxWarning(tr("密码不正确！"));
        return;
    }
    if(u != curUser){
        curUser=u;
        rfLogin();
    }
    removeEventFilter(lockObj);
    tagLock=false;
    adjustInterfaceForLock();
     this->setEnabled(true);
}

//处理添加凭证动作事件
void MainWindow::on_actAddPz_triggered()
{
    if(!isContainRight(Right::Pz_common_Add))
        return;
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
    if(!isContainRight(Right::Pz_common_Add))
        return;
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
    if(!isContainRight(Right::Pz_Common_Del))
        return;
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
    if(!isContainRight(Right::Pz_Common_Edit))
        return;
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
    if(!isContainRight(Right::Pz_Common_Edit))
        return;
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
    if(!isContainRight(Right::Pz_Common_Edit))
        return;
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
    //这个要如何进行权限限制？？？
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
    myHelper::ShowMessageBoxWarning(tr("由于缺失%1权限，此操作被拒绝！").arg(allRights.value(right)->getName()));
}

//未打开凭证集警告
void MainWindow::pzsWarning()
{
    myHelper::ShowMessageBoxWarning(tr("凭证集还未打开。在执行任何账户操作前，请先打开凭证集"));
}

/**
 * @brief MainWindow::sqlWarning
 *  数据库访问发生错误警告
 */
void MainWindow::sqlWarning()
{
    myHelper::ShowMessageBoxError(tr("在访问账户数据库文件时，发生错误！"));
}

//显示受影响的凭证数
void MainWindow::showPzNumsAffected(int num)
{
    if(num == 0)
        myHelper::ShowMessageBoxInfo(tr("本次操作成功完成，但没有凭证受此影响。"));
    else
        myHelper::ShowMessageBoxInfo(tr("本次操作总共修改%1张凭证。").arg(num));
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
    if(!isContainRight(Right::Pz_Advanced_JzHdsy))
        return;
    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
        return;
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT)){
        myHelper::ShowMessageBoxInfo(tr("请先打开凭证编辑窗口，再执行结转！"));
        return;
    }
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && !w->crtJzhdPz())
        myHelper::ShowMessageBoxWarning(tr("在创建结转汇兑损益的凭证时发生错误!"));
}

//结转损益
void MainWindow::on_actFordPl_triggered()
{
    if(!isContainRight(Right::Pz_Advanced_JzSy))
        return;
    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
        return;
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT)){
        myHelper::ShowMessageBoxInfo(tr("请先打开凭证编辑窗口，再执行结转操作！"));
        return;
    }
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && !w->crtJzsyPz())
        myHelper::ShowMessageBoxWarning(tr("在创建结转损益的凭证时发生错误!"));
}



/**
 * @brief MainWindow::on_actCurStatNew_triggered
 *  统计本期发生额，并计算期末余额
 */
void MainWindow::on_actCurStatNew_triggered()
{
    if(!isContainRight(Right::PzSet_ShowStat_Current))
        return;
    if(!curSuiteMgr->isPzSetOpened()){
        pzsWarning();
        return;
    }
    if(!curSuiteMgr->isDirty())
        curSuiteMgr->save();

    CurStatDialog* dlg = NULL;
    SubWindowDim* winfo = NULL;
    QByteArray cinfo,pinfo;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZSTAT)){
        appCon->getSubWinInfo(SUBWIN_PZSTAT,winfo,&cinfo);
        dbUtil->getSubWinInfo(SUBWIN_PZSTAT,&pinfo);
        dlg = new CurStatDialog(curSuiteMgr->getStatUtil(), &cinfo, &pinfo, this);
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
}

//登录
void MainWindow::on_actLogin_triggered()
{
    int recentUserId;
    AppConfig::getInstance()->getCfgVar(AppConfig::CVC_ResentLoginUser,recentUserId);
    LoginDialog* dlg = new LoginDialog;
    if(dlg->exec() == QDialog::Accepted){
        User* u = dlg->getLoginUser();
        if(curAccount && !u->canAccessAccount(curAccount))
            _closeAccount();
        curUser = u;
        recentUserId = curUser->getUserId();        
        ui->statusbar->setUser(curUser);
    }
    rfLogin();
}

//登出
void MainWindow::on_actLogout_triggered()
{
    if(curSuiteMgr && curSuiteMgr->isDirty()){
        if(QDialog::Accepted == myHelper::ShowMessageBoxQuesion(tr("当前帐套已改变，要保存吗？")))
            curSuiteMgr->save();
        else
            curSuiteMgr->rollback();
        curSuiteMgr->closePzSet();
    }
    //关闭所有打开的子窗口和面板窗口
    foreach(QMdiSubWindow* subWin, ui->mdiArea->subWindowList())
        subWin->close();
    foreach(QDockWidget* dw, dockWindows.values())
        dw->hide();
    curUser = NULL;
    ui->statusbar->setUser(curUser);
    rfLogin();
}


//切换用户
void MainWindow::on_actShiftUser_triggered()
{
    LoginDialog* dlg = new LoginDialog;
    if(dlg->exec() == QDialog::Accepted){
        User* u = dlg->getLoginUser();
        //如果切换用户前有打开的账户，且新登录的用户不是特权用户或他的专属账户中不包含当前打开的账户
        //则必须关闭当前打开的账户，否则登录无意义
        if(curAccount && !u->canAccessAccount(curAccount)){
            if(QDialog::Rejected == myHelper::ShowMessageBoxQuesion(
                        tr("当前登录用户不能访问账户（%1），确定用以此用户登录吗？")
                        .arg(curAccount->getSName())))
                return;
            _closeAccount();
        }
        curUser = u;
        ui->statusbar->setUser(curUser);
    }
    rfLogin();
}

//显示安全配置对话框
void MainWindow::on_actSecCon_triggered()
{
    QByteArray* sinfo = new QByteArray;
    SubWindowDim* winfo = NULL;
    SecConDialog* dlg = NULL;
    if(!commonGroups.contains(SUBWIN_SECURITY)){
        appCon->getSubWinInfo(SUBWIN_SECURITY,winfo,sinfo);
        if(dbUtil)
            dbUtil->getSubWinInfo(SUBWIN_SECURITY,sinfo);
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
    if(!isContainRight(Right::Pz_Common_Edit))
        return;
    if(activedMdiChild() == SUBWIN_PZEDIT){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
        pzEdit->moveUpBa();
    }

}

//向下移动分录
void MainWindow::on_actMvDownAction_triggered()
{
    if(!isContainRight(Right::Pz_Common_Edit))
        return;
    if(activedMdiChild() == SUBWIN_PZEDIT){
        PzDialog* pzEdit = static_cast<PzDialog*>(subWinGroups.value(curSuiteMgr->getSuiteRecord()->id)->getSubWinWidget(SUBWIN_PZEDIT));
        pzEdit->moveDownBa();
    }
}

//搜索凭证
void MainWindow::on_actSearch_triggered()
{
    QByteArray cinfo,pinfo;
    SubWindowDim* winfo = NULL;
    PzSearchDialog* dlg = NULL;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_PZSEARCH)){
        appCon->getSubWinInfo(SUBWIN_PZSEARCH,winfo,&cinfo);
        dbUtil->getSubWinInfo(SUBWIN_PZSEARCH,&pinfo);
        dlg = new PzSearchDialog(curAccount,&cinfo,&pinfo);
        connect(dlg,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_PZSEARCH,dlg,winfo);
    if(winfo)
        delete winfo;
}

//转到指定号码的凭证
void MainWindow::on_actNaviToPz_triggered()
{
    int num = spnNaviTo->value();
    if(curSuiteMgr->isPzSetOpened())
        viewOrEditPzSet(curSuiteMgr, curSuiteMgr->month());

    if(activedMdiChild() == SUBWIN_PZEDIT){
        if(!curSuiteMgr->seek(num))
            myHelper::ShowMessageBoxWarning(tr("没有凭证号为%1的凭证").arg(num));
    }
    else if(activedMdiChild() == SUBWIN_HISTORYVIEW){
        if(!isContainRight(Right::Pz_Common_Show))
            return;
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
    if(!isContainRight(Right::Pz_Advanced_Verify))
        return;
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
    if(!isContainRight(Right::Pz_Advanced_Instat))
        return;
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

/**
 * @brief 结转本年利润
 */
void MainWindow::on_actJzbnlr_triggered()
{
    if(!isContainRight(Right::Pz_Advanced_jzlr))
        return;
    if(!curSuiteMgr->isPzSetOpened() || curSuiteMgr->getState() == Ps_Jzed)
        return;
    int key = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(key)->isSpecSubOpened(SUBWIN_PZEDIT)){
        myHelper::ShowMessageBoxInfo(tr("请先打开凭证编辑窗口，再执行结转操作！"));
        return;
    }
    PzDialog* w = static_cast<PzDialog*>(subWinGroups.value(key)->getSubWinWidget(SUBWIN_PZEDIT));
    if(w && !w->crtJzbnlr())
        myHelper::ShowMessageBoxWarning(tr("在创建结转本年利润的凭证时发生错误!"));
}

/**
 * @brief MainWindow::on_actEndAcc_triggered
 *  结账
 */
void MainWindow::on_actEndAcc_triggered()
{
    if(!isContainRight(Right::PzSet_Advance_EndSet))
        return;
    if(!curSuiteMgr->isPzSetOpened()){
        pzsWarning();
        return;
    }
    if(curSuiteMgr->getState() == Ps_Jzed){
        myHelper::ShowMessageBoxInfo(tr("已经结账"));
        return;
    }
    if(curSuiteMgr->getState() != Ps_AllVerified){
        myHelper::ShowMessageBoxInfo(tr("凭证集内存在未审核凭证，不能结账！"));
        return;
    }
    if(!curSuiteMgr->getExtraState()){
        myHelper::ShowMessageBoxInfo(tr("当前的余额无效，不能结账！"));
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
    if(!isContainRight(Right::PzSet_Advance_AntiEndSet))
        return;
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
    if(!isContainRight(Right::Database_Access))
        return;
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
    if(!isContainRight(Right::PzSet_ShowStat_Totals))
        return;
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
    if(!isContainRight(Right::PzSet_ShowStat_Details))
        return;
    QByteArray cinfo,pinfo;
    SubWindowDim* winfo = NULL;
    ShowDZDialog* dlg = NULL;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_DETAILSVIEW)){
        appCon->getSubWinInfo(SUBWIN_DETAILSVIEW,winfo,&cinfo);
        dbUtil->getSubWinInfo(SUBWIN_DETAILSVIEW,&pinfo);
        dlg = new ShowDZDialog(curAccount,&cinfo,&pinfo);
        connect(dlg,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_DETAILSVIEW,dlg,winfo);
    if(winfo)
        delete winfo;
}

/**
 * @brief MainWindow::on_actInStatPz_triggered
 *  使当前凭证入账
 */
void MainWindow::on_actInStatPz_triggered()
{
    if(!isContainRight(Right::Pz_Advanced_Instat))
        return;
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
    if(!isContainRight(Right::Pz_Advanced_Verify))
        return;
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
    if(!isContainRight(Right::Pz_Advanced_AntiVerify))
        return;
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
    if(!isContainRight(Right::Account_Refresh))
        return;
    if(curAccount)
        closeAccount();
    //AppConfig::getInstance()->clearRecentOpenAccount();
    int count=0;
    if(!AppConfig::getInstance()->refreshLocalAccount(count)){
        myHelper::ShowMessageBoxError(tr("在扫描工作目录下的账户时发生错误！"));
        return;
    }
    //报告发现的账户
    myHelper::ShowMessageBoxInfo(tr("本次扫描共发现%1个账户！").arg(count));
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
    if(!isContainRight(Right::Account_Export))
        return;
    if(!isExecAccountTransform())
        return;
    TransferOutDialog dlg(this);
    dlg.exec();
}

/**
 * @brief MainWindow::on_actInAccount_triggered
 *  转入账户
 */
void MainWindow::on_actInAccount_triggered()
{
    if(!isContainRight(Right::Account_Import))
        return;
    if(!isExecAccountTransform())
        return;
    TransferInDialog dlg(this);
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
    if(!curUser || curUser && !curUser->isSuperUser())
        return false;
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



/**
 * @brief 在导入应用配置信息前执行版本检测
 * @param versionText   导入文件的第一行，表示待导入版本号
 * @param type          指定哪个应用配置类型
 * @param fileName      导入文件名
 * @param fmv           导入配置的主版本号
 * @param fsv           导入配置的次版本号
 * @return 如果版本号识别出错，或导入版本低于或等于当前版本且用户取消导入，则返回false，否则返回true
 */
bool MainWindow::inspectVersionBeforeImport(QString versionText, BaseDbVersionEnum type, QString fileName, int &fmv, int &fsv)
{
    if(!appCon->parseVersionFromText(versionText,fmv,fsv)){
        myHelper::ShowMessageBoxWarning(tr("文件“%1”无法识别版本号！").arg(fileName));
        return false;
    }
    int mv,sv;
    appCon->getAppCfgVersion(mv,sv,type);
    if((fmv < mv) || ((fmv == mv) && (fsv <= sv))){
        QString info;
        switch(type){
        case BDVE_RIGHTTYPE:
            info = tr("权限类型设置\n");
            break;
        case BDVE_RIGHT:
            info = tr("权限设置\n");
            break;
        case BDVE_GROUP:
            info = tr("组设置\n");
            break;
        case BDVE_USER:
            info = tr("用户设置\n");
            break;
        case BDVE_WORKSTATION:
            info = tr("工作站设置\n");
            break;
        case BDVE_COMMONPHRASE:
            info = tr("常用提示短语配置\n");
            break;
        }
        info.append(tr("当前版本：%1.%2\n导入版本：%3.%4\n版本不是最新的，确定需要导入吗？")
                .arg(mv).arg(sv).arg(fmv).arg(fsv));
        if(myHelper::ShowMessageBoxQuesion(info) == QDialog::Rejected)
            return false;
    }
    return true;
}

void MainWindow::loadSettings()
{
    showSplashScreen_ = true;
    showTrayIcon_ = false;
    minimizingTray_ = false;
}

/**
 * @brief 创建系统托盘图标
 */
void MainWindow::createTray()
{
    _traySystem = new QSystemTrayIcon(QIcon(":images/accSuite.png"), this);
    connect(_traySystem,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this,SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    _traySystem->setToolTip(tr("凭证助手"));
    trayMenu_ = new QMenu(this);
    _actShowMainWindow = new QAction(tr("显示主界面"),this);
    connect(_actShowMainWindow, SIGNAL(triggered()), this, SLOT(showMainWindow()));
    QFont font_ = _actShowMainWindow->font();
    font_.setBold(true);
    _actShowMainWindow->setFont(font_);
    trayMenu_->addAction(_actShowMainWindow);
    //trayMenu_->addAction(updateAllFeedsAct_);
    //trayMenu_->addAction(markAllFeedsRead_);
    //trayMenu_->addSeparator();

    //trayMenu_->addAction(optionsAct_);
    trayMenu_->addSeparator();

    trayMenu_->addAction(ui->actExit);
    _traySystem->setContextMenu(trayMenu_);
    _traySystem->show();
}

/*!
 * \brief 当应用锁定时，隐藏菜单、工具条、子窗口、浮动窗口、状态条，并在窗口标题中显示锁定状态
 */
void MainWindow::adjustInterfaceForLock()
{
    ui->menubar->setVisible(!tagLock);
    ui->statusbar->setVisible(!tagLock);
    ui->tbrMain->setVisible(!tagLock);
    ui->tbrAdvanced->setVisible(!tagLock);
    ui->tbrBaTemplate->setVisible(!tagLock);
    ui->tbrEdit->setVisible(!tagLock);
    ui->tbrPzEdit->setVisible(!tagLock);
    ui->tbrPzs->setVisible(!tagLock);
    ui->mdiArea->setVisible(!tagLock);
    foreach(QWidget* w, dockWindows)
        w->setVisible(!tagLock);
    setAppTitle();
}

/*!
 * \brief MainWindow::setAppTitle()
 */
void MainWindow::setAppTitle()
{
    if(tagLock)
        setWindowTitle(QString("%1---%2").arg(appTitle).arg(tr("应用被锁定")));
    else{
        if(curAccount)
            setWindowTitle(QString("%1---%2").arg(appTitle).arg(curAccount->getLName()));
        else
            setWindowTitle(QString("%1---%2").arg(appTitle).arg(tr("无账户被打开")));
    }
}

//显示账户属性对话框
void MainWindow::on_actAccProperty_triggered()
{
    QByteArray cinfo;
    SubWindowDim* winfo = NULL;
    AccountPropertyConfig* dlg = NULL;
    if(!commonGroups.contains(SUBWIN_ACCOUNTPROPERTY)){
        appCon->getSubWinInfo(SUBWIN_ACCOUNTPROPERTY,winfo,&cinfo);
        dlg = new AccountPropertyConfig(curAccount,&cinfo);
        if(curSSPanel)
            connect(dlg,SIGNAL(suiteChanged()),curSSPanel,SLOT(suiteUpdated()));
    }
    showCommonSubWin(SUBWIN_ACCOUNTPROPERTY,dlg,winfo);
    if(winfo)
        delete winfo;
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
        appCon->getSubWinInfo(SUBWIN_VIEWPZSETERROR,dim);
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
        appCon->getSubWinInfo(SUBWIN_LOGVIEW,winfo);
        form = new LogView;
    }
    showCommonSubWin(SUBWIN_NOTEMGR,form,winfo);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}



/**
 * @brief 从老账户创建新账户，即使用老账户的期末余额作为新账户的期初值来创建
 */
void MainWindow::on_actCrtAccoutFromOld_triggered()
{
    if(curUser && (curUser->isAdmin() || curUser->isSuperUser())){
        CrtAccountFromOldDlg* dlg = new CrtAccountFromOldDlg(this);
        dlg->exec();
    }
}

/**
 * @brief 关账
 */
void MainWindow::on_actCloseSuite_triggered()
{
    if(curSuiteMgr && curUser && (curUser->isAdmin() || curUser->isSuperUser())){
        if(!curSuiteMgr->closeSuite())
            myHelper::ShowMessageBoxError(tr("保存帐套记录时发生错误！"));
        else
            curSSPanel->suiteUpdated();
    }
}

/**
 * @brief 批量转出账户
 */
void MainWindow::on_actBatchExport_triggered()
{
    if(curAccount)
        return;
    if(curUser && (curUser->isAdmin() || curUser->isSuperUser())){
        BatchOutputDialog* dlg = new BatchOutputDialog(this);
        dlg->exec();
    }
}

/**
 * @brief 批量转入账户
 */
void MainWindow::on_actBatchImport_triggered()
{
    if(curAccount)
        return;
    if(curUser && (curUser->isAdmin() || curUser->isSuperUser())){
        BatchImportDialog* dlg = new BatchImportDialog(this);
        dlg->exec();
    }
}

bool MainWindow::impTestDatas()
{
    QString cName;
    PaUtils::extractCustomerName("应付宁波茗晗运费00590192",cName);
    int i = 0;
}

/**
 * @brief 初始化应收/应付发票统计记录（尽在第一次使用应收/应付发票信息前执行一次）
 */
void MainWindow::on_actInitInvoice_triggered()
{
    if(!curUser->isAdmin() && !curUser->isSuperUser()){
        myHelper::ShowMessageBoxWarning(tr("当前登录用户没有执行此功能的权限！"));
        return;
    }
    if(curSuiteMgr){
        YsYfInvoiceStatForm* form = new YsYfInvoiceStatForm(curSuiteMgr,true,this);
        //connect(form,SIGNAL(openSpecPz(int,int)),this,SLOT(openSpecPz(int,int)));
        QMdiSubWindow* w = ui->mdiArea->addSubWindow(form);
        w->resize(800,500);
        w->show();
    }
}

void MainWindow::on_actYsYfStat_triggered()
{
    if(!curUser->isAdmin() && !curUser->isSuperUser()){
        myHelper::ShowMessageBoxWarning(tr("当前登录用户没有执行此功能的权限！"));
        return;
    }
    if(!curSuiteMgr->isPzSetOpened()){
        pzsWarning();
        return;
    }
//    if(curSuiteMgr->getState() != Ps_Jzed){
//        myHelper::ShowMessageBoxWarning(tr("必须结账后才可以执行此功能！"));
//        return;
//    }

    YsYfInvoiceStatForm* dlg = NULL;
    SubWindowDim* winfo = NULL;
    QByteArray cinfo,pinfo;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_YSYFSTAT)){
        appCon->getSubWinInfo(SUBWIN_YSYFSTAT,winfo,&cinfo);
        dbUtil->getSubWinInfo(SUBWIN_YSYFSTAT,&pinfo);
        dlg = new YsYfInvoiceStatForm(curSuiteMgr, false, this);
    }
    else{
        dlg = static_cast<YsYfInvoiceStatForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_YSYFSTAT));

    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_YSYFSTAT,dlg,winfo);
    if(winfo)
        delete winfo;
}

void MainWindow::on_actICManage_triggered()
{
    if(!curSuiteMgr->isPzSetOpened()){
        pzsWarning();
        return;
    }

    CurInvoiceStatForm* dlg = new CurInvoiceStatForm(curAccount);
    SubWindowDim* winfo = NULL;
    QByteArray cinfo,pinfo;
    int suiteId = curSuiteMgr->getSuiteRecord()->id;
    if(!subWinGroups.value(suiteId)->isSpecSubOpened(SUBWIN_INCOST)){
        appCon->getSubWinInfo(SUBWIN_INCOST,winfo,&cinfo);
        dbUtil->getSubWinInfo(SUBWIN_INCOST,&pinfo);
        dlg = new CurInvoiceStatForm(curAccount);
        connect(dlg,SIGNAL(openRelatedPz(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    else{
        dlg = static_cast<CurInvoiceStatForm*>(subWinGroups.value(suiteId)->getSubWinWidget(SUBWIN_INCOST));

    }
    subWinGroups.value(suiteId)->showSubWindow(SUBWIN_INCOST,dlg,winfo);
    if(winfo)
        delete winfo;
}

/**
 * @brief MainWindow::on_actJxTaxMgr_triggered
 */
void MainWindow::on_actJxTaxMgr_triggered()
{
    JxTaxMgrDlg* dlg = new JxTaxMgrDlg(curAccount);
    dlg->show();
}

////////////////////////////////////////////LockApp///////////////////////////////////////
LockApp::LockApp(MainWindow *parent):QObject(parent),mainWin(parent)
{

}

bool LockApp::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == mainWin){
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if(!keyEvent->modifiers().testFlag(Qt::ControlModifier))
                return false;
            int key = keyEvent->key();
            if(key != Qt::Key_U)
                return false;
            emit unlock();
            return true;
        }
    }
}
