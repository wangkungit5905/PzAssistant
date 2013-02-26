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

#include <QBuffer>

#include "mainwindow.h"
#include "tables.h"
#include "ui_mainwindow.h"
#include "connection.h"
#include "global.h"
#include "subjectConfigDialog.h"
#include "sqltooldialog.h"
#include "utils.h"
#include "dialog2.h"
#include "dialog3.h"
#include "printUtils.h"
#include "securitys.h"
#include "commdatastruct.h"
#include "pzdialog3.h"
#include "viewpzseterrorform.h"
#include "aboutform.h"

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


int mdiAreaWidth;
int mdiAreaHeight;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setCentralWidget(ui->mdiArea);
    //mdiArea = new MyMdiArea;
    //setCentralWidget(mdiArea);

    curPzModel = NULL;  //初始时凭证集未打开
    curPzSetState = Ps_NoOpen;
    sortBy = true; //默认按凭证号进行排序
    isOpenPzSet = false;

    cursy = 0;
    cursm = 0;
    cursd = 0;
    curey = 0;
    curem = 0;
    cured = 0;

    //初始化对话框窗口指针为NULL
    dlgAcc = NULL;
    dlgBank = NULL;
    dlgData = NULL;
    dlgBase = NULL;    
    dlgCashDaily = NULL;
    dlgBankDaily = NULL;
    dlgDetailDaily = NULL;
    dlgTotalDaily = NULL;    

//    subWinSet.insert(PZEDIT);
//    subWinSet.insert(PZSTAT);
//    subWinSet.insert(CASHDAILY);
//    subWinSet.insert(BANKDAILY);
//    subWinSet.insert(DETAILSDAILY);
//    subWinSet.insert(TOTALDAILY);

    curPzn = 0;

    initActions();
    initToolBar();
    initDockWidget();

    mdiAreaWidth = ui->mdiArea->width();
    mdiAreaHeight = ui->mdiArea->height();

    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)),
            this, SLOT(setActiveSubWindow(QWidget*)));

    //初始化状态条
    //起止日期
    l1 = new QLabel;
    ui->statusbar->addPermanentWidget(l1);
    lblPzsDate = new QLabel;
    ui->statusbar->addPermanentWidget(lblPzsDate);

    //凭证集状态
    l2 = new QLabel(tr("凭证集状态："));
    ui->statusbar->addPermanentWidget(l2);
    lblPzSetState = new QLabel(pzsStates.value(curPzSetState));
    ui->statusbar->addPermanentWidget(lblPzSetState);

    //登录用户
    l3 = new QLabel(tr("当前登录用户："));
    ui->statusbar->addPermanentWidget(l3);
    lblCurUser = new QLabel(tr("未登录"));
    ui->statusbar->addPermanentWidget(lblCurUser);

    on_actLogin_triggered(); //显示登录窗口

    AppConfig* appCfg = AppConfig::getInstance();
    if(appCfg->getRecentOpenAccount(curAccountId) && (curAccountId != 0)){
        AccountBriefInfo* curAccInfo = new AccountBriefInfo;
        //QString fname = appSetting.readAccountFileName(curAccount);//获取账户数据库文本名
        appCfg->getAccInfo(curAccountId, curAccInfo);
        ConnectionManager::openConnection(curAccInfo->fileName);
        adb = ConnectionManager::getConnect();
        curAccount = new Account(curAccInfo->fileName);
        curAccount->setDatabase(&adb);
        bool cancel = false;
        if(!Account::versionMaintain(cancel)){
            if(cancel){
                QMessageBox::warning(this,tr("出错信息"),
                                     tr("该账户数据库版本过低，必须先升级！"));
            }
            else
            {
                QMessageBox::critical(this,tr("出错信息"),
                                      tr("数据库版本升级过程出错！"));
            }
            return;
        }
        //这个主要用于过度阶段（从老账户信息表过度到新账户信息表时）
        if(curAccount->getFileName().isEmpty()){
            curAccount->setFileName(curAccInfo->fileName);
            curAccount->setSName(curAccInfo->accName);
            curAccount->setLName(curAccInfo->accLName);
            curAccount->setCode(curAccInfo->code);
        }
        if(!curAccount->isValid()){
            showTemInfo(tr("账户文件无效，请检查账户文件内信息是否齐全！！"));
            return;
        }
        //curUsedSubSys = curAccInfo->usedSubSys;
        //usedRptType = curAccInfo->usedSubSys;
        accountInit();
        QString accSName, accLName;
        BusiUtil::getCompanyName(accSName, accLName);
        setWindowTitle(QString("%1---%2").arg(appTitle).arg(accLName));
        sfm->attachDb(&adb);
    }
    else {//禁用只有在账户打开的情况下才可以的部件
        setWindowTitle(QString("%1---%2").arg(appTitle)
                       .arg(tr("无账户被打开")));
    }


    //ui->actClosePzs->setEnabled(false); //刚打开账户还未打开凭证集，因此必须禁用
    ui->tbrPzEdit->setVisible(false);    
    refreshTbrVisble();
    refreshActEnanble();
}

MainWindow::~MainWindow()
{
    delete ui;
    //初始化对话框窗口指针为NULL
    if(dlgAcc != NULL)
        delete dlgAcc;
    if(dlgBank != NULL)
        delete dlgBank;
    if(dlgData != NULL)
        delete dlgData;
    if(dlgBase != NULL)
        delete dlgBase;
    if(dlgCashDaily != NULL)
        delete dlgCashDaily;
    if(dlgBankDaily != NULL)
        delete dlgBankDaily;
    if(dlgDetailDaily != NULL)
        delete dlgDetailDaily;
    if(dlgTotalDaily != NULL)
        delete dlgTotalDaily;

}

//获取主窗口的Mdi区域的大小（必须在主窗口显示后调用）
void MainWindow::getMdiAreaSize(int &mdiAreaWidth, int &mdiAreaHeight)
{
    mdiAreaWidth = ui->mdiArea->width();
    mdiAreaHeight = ui->mdiArea->height();
}

void MainWindow::hideDockWindows()
{
    QAction* act = subjectSearchDock->toggleViewAction();
    act->trigger();
}

void MainWindow::initActions()
{
    //文件菜单
    connect(ui->actCrtAccount, SIGNAL(triggered()), this, SLOT(newAccount()));
    connect(ui->actOpenAccount, SIGNAL(triggered()), this, SLOT(openAccount()));
    connect(ui->actCloseAccount, SIGNAL(triggered()), this, SLOT(closeAccount()));
    connect(ui->actRefreshActInfo, SIGNAL(triggered()), this, SLOT(refreshActInfo()));
    connect(ui->actImpActFromFile, SIGNAL(triggered()), this, SLOT(attachAccount()));
    connect(ui->actEmpActToFile, SIGNAL(triggered()), this, SLOT(detachAccount()));
    connect(ui->actExit,SIGNAL(triggered()),this,SLOT(exit()));    

    //数据菜单
    connect(ui->actCollCal, SIGNAL(triggered()), this, SLOT(collectCalculate()));
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
    spnNaviTo->setMaximum(MAXPZNUMS);
    connect(spnNaviTo,SIGNAL(userEditingFinished()),
            this,SLOT(on_actNaviToPz_triggered()));
    ui->tbrPzs->insertWidget(ui->actNaviToPz, spnNaviTo);

    QGroupBox* pzsBox = new QGroupBox;
    rdoRecording = new QRadioButton(tr("录入态"),pzsBox);
    rdoRepealPz = new QRadioButton(tr("已作废"),pzsBox);
    rdoInstatPz = new QRadioButton(tr("已入账"),pzsBox);
    rdoVerifyPz = new QRadioButton(tr("已审核"),pzsBox);
    actRecording =  ui->tbrAdvanced->addWidget(rdoRecording);
    actRecording->setVisible(false);
    ui->tbrAdvanced->addWidget(rdoRepealPz);
    ui->tbrAdvanced->addWidget(rdoVerifyPz);
    ui->tbrAdvanced->addWidget(rdoInstatPz);
    connect(rdoRepealPz, SIGNAL(clicked(bool)), this, SLOT(userModifyPzState(bool)));
    connect(rdoVerifyPz, SIGNAL(clicked(bool)), this, SLOT(userModifyPzState(bool)));
    connect(rdoInstatPz, SIGNAL(clicked(bool)), this, SLOT(userModifyPzState(bool)));

}

void MainWindow::initDockWidget()
{
    subjectSearchDock = new QDockWidget(tr("搜索客户名"), this);
    subjectSearchDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    sfm = new SubjectSearchForm(subjectSearchDock);
    subjectSearchDock->setWidget(sfm);
    addDockWidget(Qt::LeftDockWidgetArea, subjectSearchDock);
}



//打开账户时的初始化设置工作
void MainWindow::accountInit()
{
    //设置账户内使用的币种
    allMts.clear();
    allMts[curAccount->getMasterMt()] = MTS.value(curAccount->getMasterMt());
    foreach(int mt, curAccount->getWaiMt())
        allMts[mt] = MTS.value(mt);
    //BusiUtil::getAllSubFName(allFstSubs);//初始化一二级科目名表
    //BusiUtil::getAllSubFCode(allFSCodes);
    BusiUtil::getAllSubSName(allSndSubs);
    BusiUtil::getAllSubSLName(allSndSubLNames);
    BusiUtil::getDefaultSndSubs(defaultSndSubs);

    //初始化需特别处理到科目id
    BusiUtil::getIdByCode(subCashId, "1001");
    BusiUtil::getIdByCode(subBankId, "1002");
    BusiUtil::getIdByCode(subYsId, "1131");
    BusiUtil::getIdByCode(subYfId, "2121");
    SubjectManager* sm = curAccount->getSubjectManager();
    subCashRmbId = sm->getCashRmbSid();
    //BusiUtil::getSidByName(allFstSubs.value(subCashId), tr("人民币"), subCashRmbId);
    //BusiUtil::getGdzcSubClass(allGdzcSubjectCls);
    Gdzc::getSubClasses(allGdzcSubjectCls);
    BusiUtil::init();
}

//动态更新窗口菜单
void MainWindow::updateWindowMenu()
{
    ui->mnuWindow->clear();
    ui->mnuWindow->addAction(subjectSearchDock->toggleViewAction());
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
        QDialog* child = qobject_cast<QDialog*>(windows.at(i)->widget());

        QString text;
        if (i < 9) {
            text = tr("&%1 %2").arg(i + 1)
                               .arg(child->windowTitle());
        } else {
            text = tr("%1 %2").arg(i + 1)
                              .arg(child->windowTitle());
        }
        QAction *action  = ui->mnuWindow->addAction(text);
        action->setCheckable(true);
        action->setChecked(child == activeMdiChild());
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, windows.at(i));
    }
}

//返回当前活动的窗口
QDialog* MainWindow::activeMdiChild()
{
    if (QMdiSubWindow *activeSubWindow = ui->mdiArea->activeSubWindow())
        return qobject_cast<QDialog*>(activeSubWindow->widget());
    return 0;
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
    dlgAcc = new CreateAccountDialog(this);
    connect(dlgAcc, SIGNAL(toNextStep(int,int)), this, SLOT(toCrtAccNextStep(int,int)));
    dlgAcc->show();
}

void MainWindow::openAccount()
{
    OpenAccountDialog* dlg = new OpenAccountDialog;
    if(dlg->exec() == QDialog::Accepted){
         ui->tbrPzs->setVisible(true);
        curAccountId = dlg->getAccountId();
        //if(curAccInfo)
        //    delete curAccInfo;

        AppConfig* appCfg = AppConfig::getInstance();
        AccountBriefInfo* curAccInfo = new AccountBriefInfo;
        appCfg->getAccInfo(curAccountId,curAccInfo);
        //showTemInfo(curAccInfo->desc);
        //curUsedSubSys = curAccInfo->usedSubSys;  //读取打开的账户所使用的科目系统
        //usedRptType = curAccInfo->usedRptType;   //读取打开的账户所使用的报表类型
        ConnectionManager::openConnection(dlg->getFileName());
        adb = ConnectionManager::getConnect();
        if(curAccount){
            delete curAccount;
            curAccount = NULL;
        }
        bool cancel;
        if(!Account::versionMaintain(cancel)){
            if(cancel){
                QMessageBox::warning(this,tr("出错信息"),
                                     tr("该账户数据库版本过低，必须先升级！"));
            }
            else
            {
                QMessageBox::critical(this,tr("出错信息"),
                                      tr("数据库版本升级过程出错！"));
            }
            return;
        }
        curAccount = new Account(curAccInfo->fileName/*,adb*/);
        //这个主要用于过度阶段（从老账户信息表过度到新账户信息表时）
        if(curAccount->getFileName().isEmpty()){
            curAccount->setFileName(curAccInfo->fileName);
            curAccount->setSName(curAccInfo->accName);
            curAccount->setLName(curAccInfo->accLName);
            curAccount->setCode(curAccInfo->code);
        }
        if(!curAccount->isValid()){
            showTemInfo(tr("账户文件无效，请检查账户文件内信息是否齐全！！"));
            return;
        }
        //setWindowTitle(tr("会计凭证处理系统---") + dlg->getLName());
        setWindowTitle(tr("会计凭证处理系统---") + curAccount->getLName());
        appCfg->setRecentOpenAccount(curAccountId);
        refreshTbrVisble();
        refreshActEnanble();
        sfm->attachDb(&adb);
        accountInit();
    }
}

void MainWindow::closeAccount()
{
    QString state;
    bool ok;
    state = QInputDialog::getText(this,tr("账户关闭提示"),
        tr("请输入表示账户最后关闭前的状态说明语"),QLineEdit::Normal,state,&ok);
    if(!ok)
        return;

    if(isOpenPzSet)
        closePzs();

    //关闭账户前，还应关闭与此账户有关的其他子窗口
    if(subWindows.contains(SETUPBANK)){  //设置银行账户窗口
        subWindows.value(SETUPBANK)->close();
        //ui->mdiArea->removeSubWindow(subWindows.value(SETUPBANK));
        //delete dlgSetupBank;
    }

    //设置期初余额的窗口
    //科目配置窗口
    //......

    curAccount->setLastAccessTime(QDateTime::currentDateTime());
    if(state == "")
        state = tr("账户最后由用户%1于%2关闭！").arg(curUser->getName())
                .arg(curAccount->getLastAccessTime().toString(Qt::ISODate));
    curAccount->appendLog(QDateTime::currentDateTime(),state);
    //appCfg.saveAccInfo(curAccInfo);
    //BusiUtil::saveAccInfo(curAccInfo);

    delete curAccount;
    curAccount = NULL;
    ConnectionManager::closeConnection();
    ui->tbrPzs->setVisible(false);
    curAccountId = 0;
    //delete curAccInfo;
    //curAccInfo = NULL;
    AppConfig::getInstance()->setRecentOpenAccount(0);
    setWindowTitle(tr("会计凭证处理系统---无账户被打开"));    
    refreshTbrVisble();
    refreshActEnanble();
}

//刷新账户信息
void MainWindow::refreshActInfo()
{
    //刷新前要关闭当前打开的账户
    if(curAccountId != 0)
        closeAccount();

    //清除已有的账户信息
    AppConfig::getInstance()->clear();

    //在应用程序的账户目录下扫描所有文件，判断是否是有效的账户数据库文件，如是，则读取
    //账户信息并导入到应用程序中，将账户信息写入到配置中

    QDir dir("./datas/databases");
    QStringList filters, filelist;
    filters << "*.dat";
    dir.setNameFilters(filters);
    filelist = dir.entryList(filters, QDir::Files);

    bool r;
    int fondCount = 0;
    int c;

    AppConfig* appCfg = AppConfig::getInstance();
    if(filelist.count() == 0)
        QMessageBox::information(this, tr("一般信息"),
                                 tr("当前没有可用的帐户数据库文件"));
    else{
        appCfg->clear();
        for(int i = 0; i < filelist.count(); ++i){
            filelist[i].chop(4);
            QString fname = filelist[i];
            if(ConnectionManager::openConnection(fname)){ //成功打开数据库文件
                Account* acc = new Account(fname/*,ConnectionManager::getConnect()*/);//读取数据库文件中的账户信息
                if(acc->getFileName().isEmpty())
                    acc->setFileName(fname);
                if(acc->isValid()){
                    AccountBriefInfo* accInfo = new AccountBriefInfo;
                    accInfo->code = acc->getCode();
                    accInfo->fileName = fname;
                    accInfo->accName = acc->getSName();
                    accInfo->accLName = acc->getLName();
                    appCfg->saveAccInfo(accInfo);

                    fondCount++;
                    ConnectionManager::closeConnection(); //关闭刚刚打开的数据库
                }
            }
        }
        //报告查找结果
        if(fondCount == 0)
            QMessageBox::information(this, tr("搜寻账户"), tr("没有发现账户文件"));
        else
            QMessageBox::information(this, tr("搜寻账户"),
                                     QString(tr("共发现%1个账户文件")).arg(fondCount));

    }
}


//退出应用
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(curAccountId != 0){       //为了在下次打开应用时自动打开最后打开的账户
        int id = curAccountId;
        closeAccount();
        curAccountId = id;
    }
    ui->mdiArea->closeAllSubWindows();
    appExit();
    if (ui->mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        //writeSettings();
        event->accept();
    }
}

void MainWindow::exit()
{
    if(curAccountId != 0)
        closeAccount();
    ui->mdiArea->closeAllSubWindows();

    close();
}

////////////////////帐务处理菜单处理槽部分////////////////////////////////
//打开凭证集
void MainWindow::openPzs()
{
    //PSetting setting;
    //QString lname = setting.readAccountFullName(curAccount);
    QString sname,lname; //帐户名
    BusiUtil::getCompanyName(sname,lname);
    OpenPzDialog* dlg = new OpenPzDialog(curAccount);
    if(dlg->exec() == QDialog::Accepted){       
       isOpenPzSet = true;
       QDate d = dlg->getDate();
       curAccount->setCurMonth(d.month());
       cursy = d.year();
       cursm = d.month();
       curey = d.year();
       curem = d.month();
       cursd = 1;
       cured = d.daysInMonth();

       QString dateStr = dlg->getDate().toString(Qt::ISODate);
       dateStr.chop(3); //去掉日部分

       curPzModel = new CustomRelationTableModel;
       curPzModel->setTable("PingZhengs");       

       //过滤模型数据
       QString fileStr = QString("date like '%1%'").arg(dateStr);
       curPzModel->setFilter(fileStr);
       curPzModel->select();       
       if(!BusiUtil::getPzsState(cursy,cursm,curPzSetState)){
           curPzSetState = Ps_Rec;
           BusiUtil::setPzsState(cursy,cursm,Ps_Rec);
       }

       //在状态条中显示打开的凭证集的起止日期和状态
       QString sd = QString(tr("(%1年%2月%3日——%4年%5月%6日)")).arg(cursy)
               .arg(cursm).arg(cursd).arg(curey).arg(curem).arg(cured);
       l1->setText(tr("起止日期："));
       lblPzsDate->setText(sd);
       lblPzSetState->setText(pzsStates.value(curPzSetState));

       //在状态条中显示凭证集中各种状态的凭证数
       int amount = curPzModel->rowCount();
       QHash<PzState,int> counts;
       PzState pzs;
       for(int i = 0; i < amount; ++i){
           pzs = (PzState )curPzModel->data(curPzModel->index(i,PZ_PZSTATE)).toInt();
           counts[pzs]++;
       }
       if(amount > 0){
           QString info = tr("本期共有%1张凭证，其中（").arg(amount);
           QHashIterator<PzState,int> it(counts);
           while(it.hasNext()){
               it.next();
               info.append(tr("%1：%2，").arg(pzStates.value(it.key())).arg(it.value()));
           }
           info.chop(1); info.append(tr("）"));
           showTemInfo(info);
       }
       else
           showTemInfo(tr("本期还没有任何凭证。"));


       BusiUtil::getRates(cursy,cursm,curRates); //初始化全局汇率表
       curRates[RMB] = 1.00;
       refreshTbrVisble();
       refreshActEnanble();
    }
}


void MainWindow::closePzs()
{
    //关闭凭证集时，要先关闭所有与该凭证集相关的子窗口
    if(subWindows.contains(PZEDIT))
        subWindows.value(PZEDIT)->close();
    if(subWindows.contains(PZSTAT))                  //显示本期统计的窗口
        subWindows.value(PZSTAT)->close();

    if(subWindows.contains(CASHDAILY))               //现金日记账窗口
        subWindows.value(CASHDAILY)->close();
    if(subWindows.contains(BANKDAILY))               //银行日记账窗口
        subWindows.value(BANKDAILY)->close();
    if(subWindows.contains(DETAILSDAILY))            //明细账窗口
        subWindows.value(DETAILSDAILY)->close();
    if(subWindows.contains(TOTALDAILY))              //总分类账窗口
        subWindows.value(TOTALDAILY)->close();

    if(curPzModel)
        delete curPzModel;

    BusiUtil::setPzsState(cursy,cursm,curPzSetState);
    curPzModel = NULL;
    isOpenPzSet = false;
    cursy = 0;
    cursm = 0;
    cursd = 0;
    curey = 0;
    curem = 0;
    cured = 0;

    l1->setText("");
    lblPzsDate->setText("");
    lblPzSetState->setText(pzsStates.value(Ps_NoOpen));

    refreshTbrVisble();
    refreshActEnanble();    


}

//在当前打开的凭证集中进行录入、修改凭证等编辑操作
void MainWindow::editPzs()
{
    //如果已经打开了此窗口则退出
    if(subWindows.contains(PZEDIT)){
        ui->mdiArea->setActiveSubWindow(subWindows.value(PZEDIT));
        return;
    }

    //检测当前用户权限，以决定是否允许打开此窗口，是以只读方式还是以可编辑方式打开



    //指定对模型数据如何排序
    QList<int> sortClos; //模型的排序列
    if(sortBy)
        sortClos << PZ_NUMBER << Qt::AscendingOrder; //按凭证总号的升序
    else
        sortClos << PZ_ZBNUM << Qt::AscendingOrder; //按凭证自编号的升序
    curPzModel->setSort(sortClos);

    PzDialog2* dlg = new PzDialog2(cursy, cursm, curPzModel, false, this);
    dlg->setWindowTitle(tr("凭证窗口"));
    connect(dlg, SIGNAL(canMoveUpAction(bool)), this, SLOT(canMoveUp(bool)));
    connect(dlg, SIGNAL(canMoveDownAction(bool)), this, SLOT(canMoveDown(bool)));
    connect(dlg, SIGNAL(curPzNumChanged(int)), this, SLOT(curPzNumChanged(int)));
    connect(dlg, SIGNAL(pzStateChanged(int)), this, SLOT(pzStateChange(int)));
    connect(dlg, SIGNAL(pzContentChanged(bool)), this, SLOT(pzContentChanged(bool)));
    connect(dlg, SIGNAL(curIndexChanged(int,int)),this, SLOT(curPzIndexChanged(int,int)));
    connect(dlg, SIGNAL(selectedBaAction(bool)), this, SLOT(userSelBaAction(bool)));
    connect(dlg, SIGNAL(saveCompleted()), this, SLOT(pzContentSaved()));
    connect(dlg, SIGNAL(pzsStateChanged()), this, SLOT(refreshShowPzsState()));

    //设置当前打开凭证可选的时间范围
    QDate start = QDate(cursy,cursm,cursd);
    QDate end = QDate(cursy,cursm, cured);
    dlg->setDateRange(start, end);

    //根据凭证集的状态，设置对话框的只读状态（有待推敲）
    if(curPzSetState != Ps_Rec){
        dlg->setReadOnly(true);
    }

    QByteArray* sinfo;
    SubWindowDim* winfo;
    VariousUtils::getSubWinInfo(PZEDIT,winfo,sinfo);
    showSubWindow(PZEDIT,winfo,dlg);
    if(winfo)
        delete winfo;
    if(sinfo)
        delete sinfo;

    //获取凭证集合所属月份的汇率(这里要判断会计期间处于一个月份以内才有意义，如果跨越月份，则必须在
    //编辑凭证的对话框内增加实时获取当前凭证所使用的汇率的机制，这可以通过捕获凭证日期的改变来实时
    //更新当前汇率。)
//    QSqlQuery query;
//    QString qs = QString("select usd2rmb from ExchangeRates where (year = %1) and (month = %2)")
//                 .arg(cursy).arg(cursm);
//    if((query.exec(qs)) && (query.first()))
//        dlgEditPz->setRate(query.value(0).toDouble());

    //QHash<int,double> rates;
    //BusiUtil::getRates(cursy, cursm, rates);
    //dlgEditPz->initRate(rates);

    //pdlg->exec();
    //pdlg->show();



    refreshTbrVisble();
    refreshActEnanble();
}

//自动分配凭证号
void MainWindow::autoAssignPzNum()
{

}

//手动指定凭证号
void MainWindow::handAssignPzNum(){

}

///////////////////////数据菜单处理槽部分/////////////////////////////////////
//汇总计算
void MainWindow::collectCalculate()
{
    CollectPzDialog* dlg = new CollectPzDialog;
    dlg->setDateLimit(cursy,cursm,curey,curem);
    dlg->setWindowTitle(tr("汇总窗口"));
    ui->mdiArea->addSubWindow(dlg);
    dlg->show();
}

//显示科目余额
void MainWindow::viewSubjectExtra()
{
//    SubjectExtraDialog* dlg = new SubjectExtraDialog;
//    dlg->setWindowTitle(tr("科目余额窗口"));
//    if(isOpenPzSet)  //在一级打开凭证集的情况下，直接显示该时间段的余额
//        dlg->setDate(cursy,cursm);
//    ui->mdiArea->addSubWindow(dlg);
//    dlg->show();

//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }
    LookupSubjectExtraDialog* dlg;
    if(subWindows.contains(LOOKUPSUBEXTRA)){
        dlg = static_cast<LookupSubjectExtraDialog*>(subWindows.value(LOOKUPSUBEXTRA)->widget());
        //dlg->setDate(cursy,cursm);
        //激活此子窗口
        ui->mdiArea->setActiveSubWindow(subWindows.value(LOOKUPSUBEXTRA));
        return;
    }

    SubWindowDim* winfo;
    QByteArray* sinfo;
    bool exist = VariousUtils::getSubWinInfo(LOOKUPSUBEXTRA,winfo,sinfo);
    dlg = new LookupSubjectExtraDialog(curAccount);

    showSubWindow(LOOKUPSUBEXTRA,winfo,dlg);
    if(winfo)
        delete winfo;
    if(sinfo)
        delete sinfo;



}

//打开指定id值的凭证（接受外部对话框窗口的请求，比如显示日记账或明细账的对话框）
//并选择id值为bid的会计分录
void MainWindow::openSpecPz(int pid,int bid)
{
    //根据凭证是否属于当前月份来决定是否用凭证编辑窗口还是用历史凭证显示窗口打开
    bool isIn;
    if(!BusiUtil::isPzInMonth(cursy,cursm,pid,isIn))
        return;
    if(isIn){
        if(!subWindows.contains(PZEDIT)) //如果凭证编辑窗口还未打开，则先打开
            editPzs();
        PzDialog2* dlg = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
        dlg->naviTo(pid,bid);
        ui->mdiArea->setActiveSubWindow(subWindows.value(PZEDIT));
    }
    else{
        QByteArray* sinfo;
        SubWindowDim* winfo;
        VariousUtils::getSubWinInfo(HISTORYVIEW,winfo,sinfo);
        HistoryPzDialog* dlg = new HistoryPzDialog(pid,bid,sinfo);
        showSubWindow(HISTORYVIEW,winfo,dlg);
        if(sinfo)
            delete sinfo;
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
//设置账户信息
void MainWindow::setupAccInfos()
{
    CreateAccountDialog* dlg = new CreateAccountDialog(false, this);
    dlg->setWindowTitle(tr("设置账户信息"));
    ui->mdiArea->addSubWindow(dlg);
    //dlg->exec();
    dlg->show();
}

//导入基本数据
bool MainWindow::impBasicDatas()
{
    BasicDataDialog* dlg = new BasicDataDialog(false);
    dlg->setWindowTitle(tr("导入基础数据"));
    ui->mdiArea->addSubWindow(dlg);
    dlg->exec();
}

//设置记账基点
void MainWindow::setupBase()
{
    showSubWindow(SETUPBASE);
}

//设置开户银行账户信息
void MainWindow::setupBankInfos()
{
    showSubWindow(SETUPBANK);
}

bool MainWindow::impTestDatas()
{
//    SubjectManager* sm = curAccount->getSubjectManager();
//    int id;
//    id = sm->getCashRmbSid();
//    int i = 0;


//    QString s;
//    Double d1(1.12);
//    Double d2(2.32);
//    Double d3 = d1+d2;
//    s = d3.toString();
//    d3  = d1-d2;
//    s = d3.toString();
//    d3 = d1*d2;
//    s = d3.toString();
//    d3 = d1/d2;
//    s = d3.toString();
//    d1+=d2;
//    s = d1.toString();
//    d1-=d2;
//    s = d1.toString();
//    d1*=d2;
//    s = d1.toString();
//    d1/=d2;
//    s = d1.toString();
//    int i = 0;


    //计算当期发生额
//    QHash<int,double> jsum,jdsum,dsum,ddsum;
//    QHash<int,Double> d_jsum,d_jdsum,d_dsum,d_ddsum;
//    bool isSave;
//    int amount;
//    BusiUtil::calAmountByMonth(2011,5,jsum,dsum,jdsum,ddsum,isSave,amount);
//    BusiUtil::calAmountByMonth2(2011,6,d_jsum,d_dsum,d_jdsum,d_ddsum,isSave,amount);
//    int i = 0;

    //将主营业务收入和成本下币种为美元的会计分录转换为人民币
    //tranUsdToRbm(2011,5,72,ConnectionManager::getConnect());
    //tranUsdToRbm(2011,5,77,ConnectionManager::getConnect());

    //结转汇兑损益
    //BusiUtil::genForwordEx2(2011,12,curUser);

    //结转损益类科目到本年利润（有提示窗，供用户输入要结转的年月）
    //jzsy();

    //删除结转汇兑损益凭证Pzc_Jzhd_Bank
//    BusiUtil::delSpecPz(2011,12,Pzc_Jzhd_Bank);
//    BusiUtil::delSpecPz(2011,12,Pzc_Jzhd_Ys);
//    BusiUtil::delSpecPz(2011,12,Pzc_Jzhd_Yf);

    //结转2011年5月的损益类科目至本年利润
    //BusiUtil::genForwordPl2(2011,12,curUser);

    // 删除结转损益（收入和费用两张）的凭证
    BusiUtil::delSpecPz(2011,12,Pzc_JzsyIn);
    BusiUtil::delSpecPz(2011,12,Pzc_JzsyFei);
    BusiUtil::setPzsState(2011,12,Ps_JzhdV);
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
    if((curStep == 1) && (nextStep == 2)){ //结束账户一般信息的输入
        QString code = dlgAcc->getCode();
        QString sname = dlgAcc->getSName();
        QString lname = dlgAcc->getLName();
        QString filename = dlgAcc->getFileName();
        int reportType = dlgAcc->getReportType();

        //创建该账户对应的数据库，并建立基本数据表和导入基本数据,必要情况下显示创建进度
        //检查是否存在同名文件，如有，则恢复刚才的对话框的内容允许用户进行修改
        QString qn = QString("./datas/databases/%1").arg(filename);

        if(!QFile::exists(qn + ".dat") ||   //如果文件不存在或
           (QFile::exists(qn + ".dat") &&   //文件存在且要求覆盖
           (QMessageBox::Yes == QMessageBox::critical(0, qApp->tr("文件名称冲突"),
                qApp->tr("数据库文件名已存在，要覆盖吗？\n"
                "文件覆盖后将导致先前文件的数据全部丢失"),
                QMessageBox::Yes | QMessageBox::No)))){
            //创建数据库文件并打开连接
            ConnectionManager::openConnection(filename);
            adb = ConnectionManager::getConnect();
            createBasicTable(); //创建基本表

            //将账户信息添加到数据库文件中
            QSqlQuery query;
            query.prepare("INSERT INTO AccountInfos(code, sname, lname) "
                          "VALUES(:code, :sname, :lname)");
            query.bindValue(":code", code);
            query.bindValue(":sname", sname);
            query.bindValue(":lname", lname);
            query.exec();

            //将该账户添加到应用程序的配置信息中（帐户简称，数据库文件名对）并将该账户设置为当前账户
            AppConfig* appSetting = AppConfig::getInstance();
            curAccountId = appSetting->addAccountInfo(code, sname, lname, filename);
            appSetting->setUsedReportType(curAccountId, reportType);

            appSetting->setRecentOpenAccount(curAccountId);
            setWindowTitle(tr("会计凭证处理系统---") + lname);
            //enaActOnAccOpened(true);
            refreshTbrVisble();
            refreshActEnanble();
            dlgAcc->close();

            dlgBank = new SetupBankDialog;
            connect(dlgBank, SIGNAL(toNextStep(int,int)), this, SLOT(toCrtAccNextStep(int,int)));
            dlgBank->show();
        }
    }
    else if((curStep == 2) && (nextStep == 3)){
        dlgBank->close();
        dlgData = new BasicDataDialog;
        connect(dlgData, SIGNAL(toNextStep(int,int)), this, SLOT(toCrtAccNextStep(int,int)));
        dlgData->show();
    }
    else if((curStep == 3) && (nextStep == 4)){
        dlgData->close();
        dlgBase = new SetupBaseDialog;
        connect(dlgBase, SIGNAL(toNextStep(int,int)), this, SLOT(toCrtAccNextStep(int,int)));
        dlgBase->show();
    }
    else if((curStep == 4) && (nextStep == 4)){
        dlgBase->close();

        delete dlgAcc;
        delete dlgBank;
        delete dlgData;
        delete dlgBase;
    }
    else if(nextStep == 0){
        delete dlgAcc;
        delete dlgBank;
        delete dlgData;
        delete dlgBase;
    }
}

//刷新工具条的可见性
void MainWindow::refreshTbrVisble()
{
    bool isPzEdit = subWindows.contains(PZEDIT);
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

    if(curAccountId != 0){ //如果账户已经打开
        ui->tbrPzs->setVisible(true);
    }
    else{
        ui->tbrPzs->setVisible(false);
    }

    if(isPzEdit && subWinActStates.value(PZEDIT)){ //凭证编辑窗口已经打开并处于激活状态
        ui->tbrPzEdit->setVisible(true);
        ui->tbrAdvanced->setVisible(true);
    }
    else{
        ui->tbrPzEdit->setVisible(false);
        ui->tbrAdvanced->setVisible(false);
    }
}

//刷新Action的可用性
void MainWindow::refreshActEnanble()
{
    bool isPzEdit = subWindows.contains(PZEDIT);

    //登录动作管理
    bool r = (curUser == NULL);
    ui->actLogin->setEnabled(r);
    ui->actLogout->setEnabled(!r);
    ui->actShiftUser->setEnabled(!r);
    if(r)
        return;

    //账户生命期管理动作
    if(curUser == NULL){
        ui->actCrtAccount->setEnabled(false);
        ui->actCloseAccount->setEnabled(false);
        ui->actOpenAccount->setEnabled(false);
        //ui->actDelAcc->setEnabled(false);
        ui->actRefreshActInfo->setEnabled(false);
        ui->actImpActFromFile->setEnabled(false);
        ui->actEmpActToFile->setEnabled(false);
        ui->actAccProperty->setEnabled(false);
    }
    else{
        r = curUser->haveRight(allRights.value(Right::CreateAccount)) && (curAccountId == 0);
        ui->actCrtAccount->setEnabled(r);
        r = curUser->haveRight(allRights.value(Right::CloseAccount)) &&
                (curAccountId != 0);
        ui->actCloseAccount->setEnabled(r);
        r = curUser->haveRight(allRights.value(Right::OpenAccount)) && (curAccountId == 0);
        ui->actOpenAccount->setEnabled(r);
        //r = curUser->haveRight(allRights.value(Right::DelAccount)) &&
        //        (curAccount != 0);
        //ui->actDelAcc->setEnabled(r);
        r = curUser->haveRight(allRights.value(Right::RefreshAccount)) && (curAccountId == 0);
        ui->actRefreshActInfo->setEnabled(r);
        r = curUser->haveRight(allRights.value(Right::ImportAccount)) && (curAccountId == 0);
        ui->actImpActFromFile->setEnabled(r);
        r = curUser->haveRight(allRights.value(Right::ExportAccount)) && (curAccountId == 0);
        ui->actEmpActToFile->setEnabled(r);
        ui->actAccProperty->setEnabled(curAccountId != 0);
    }


    //凭证集操作动作
    if(curUser == NULL){
        ui->actOpenPzs->setEnabled(false);
        ui->actClosePzs->setEnabled(false);
        ui->actEditPzs->setEnabled(false);
        ui->mnuAdvOper->setEnabled(false);
        ui->mnuEndProcess->setEnabled(false);
    }
    else{
        r = (curAccountId != 0) && !isOpenPzSet &&
                curUser->haveRight(allRights.value(Right::OpenPzs));
        ui->actOpenPzs->setEnabled(r);
        r = curUser->haveRight(allRights.value(Right::ClosePzs)) && isOpenPzSet;
        ui->actClosePzs->setEnabled(r);

        r = (curUser->haveRight(allRights.value(Right::AddPz)) ||
            curUser->haveRight(allRights.value(Right::DelPz))  ||
            curUser->haveRight(allRights.value(Right::EditPz)) ||
            curUser->haveRight(allRights.value(Right::ViewPz))) && isOpenPzSet;
        ui->actEditPzs->setEnabled(r);

        //检测用户的高级凭证操作权限，并视情启用菜单（未来实现！！）
        ui->mnuAdvOper->setEnabled(isOpenPzSet);
        ui->mnuEndProcess->setEnabled(isOpenPzSet);
    }

    //期末处理
    //引入其他凭证
    r = (curPzSetState == Ps_Stat1) || (curPzSetState == Ps_Stat3);
    ui->actImpOtherPz->setEnabled(r);
    //结转汇兑损益
    r = (curPzSetState == Ps_Stat1) || (curPzSetState == Ps_Stat2);
    ui->actFordEx->setEnabled(r);
    //结转损益
    //r = (curPzSetState == Ps_Stat2) || (curPzSetState == Ps_Stat3);
    ui->actFordPl->setEnabled(curPzSetState == Ps_JzsyPre);
    //结转本年利润
    ui->actJzbnlr->setEnabled(curPzSetState == Ps_Stat4);
    //结账
    //r = (curPzSetState == Ps_Stat5) ||
    //    (jzlrByYear && (cursm != 12) && (curPzSetState == Ps_Stat4));
    //ui->actEndAcc->setEnabled(r);
    //反结转
    r = (curPzSetState >= Ps_ImpOther) && (curPzSetState <= Ps_Stat5);
    ui->actAntiJz->setEnabled(r);
    //反引入
    r = (curPzSetState >= Ps_ImpOther) && (curPzSetState <= Ps_JzsyPre);
    ui->actAntiImp->setEnabled(r);
    //反结账
    ui->actAntiEndAcc->setEnabled(curPzSetState == Ps_Jzed);


    //高级凭证操作
    if(curUser == NULL){
        ui->actVerifyPz->setEnabled(false);
        ui->actInStatPz->setEnabled(false);
        ui->actAntiVerify->setEnabled(false);
        ui->actRepealPz->setEnabled(false);
        ui->actAntiRepeat->setEnabled(false);
        ui->actAllVerify->setEnabled(false);
        ui->actAllInstat->setEnabled(false);
    }
    else{
        if(curAccount && curAccount->getReadOnly()){
            ui->mnuAdvOper->setEnabled(false);
            ui->mnuEndProcess->setEnabled(false);

            ui->actSubConfig->setEnabled(false);
        }
        else{
            //审核凭证
            r = curUser->haveRight(allRights.value(Right::VerifyPz)) &&
                (curPzState == Pzs_Recording);
            ui->actVerifyPz->setEnabled(r);
            //凭证入账
            r = curUser->haveRight(allRights.value(Right::InstatPz)) &&
                (curPzState == Pzs_Verify);
            ui->actInStatPz->setEnabled(r);
            //取消审核
            r = curUser->haveRight(allRights.value(Right::VerifyPz)) &&
                    ((curPzState == Pzs_Verify) || (curPzState == Pzs_Instat));
            ui->actAntiVerify->setEnabled(r);
            //作废
            r = curUser->haveRight(allRights.value(Right::RepealPz)) &&
                (curPzState == Pzs_Recording);
            ui->actRepealPz->setEnabled(r);
            //取消作废
            r = curUser->haveRight(allRights.value(Right::RepealPz)) &&
                (curPzState == Pzs_Repeal);
            ui->actAntiRepeat->setEnabled(r);
            //全部审核（或许还应参考凭证集状态）
            r = curUser->haveRight(allRights.value(Right::VerifyPz));
            ui->actAllVerify->setEnabled(r);
            //全部入账
            r = curUser->haveRight(allRights.value(Right::InstatPz));
            ui->actAllInstat->setEnabled(r);
        }

    }

    //一般凭证操作动作
    if(curUser == NULL){        
        ui->actAddPz->setEnabled(false);
        ui->actDelPz->setEnabled(false);        
    }
    else{
        if(curAccount && curAccount->getReadOnly()){
            ui->actAddPz->setEnabled(false);
            ui->actInsertPz->setEnabled(false);
            ui->actSavePz->setEnabled(false);
            ui->actDelPz->setEnabled(false);
            ui->actAddAction->setEnabled(false);
            ui->actDelAction->setEnabled(false);
            ui->actMvDownAction->setEnabled(false);
            ui->actMvUpAction->setEnabled(false);
            ui->actRepealPz->setEnabled(false);
            ui->actVerifyPz->setEnabled(false);
            ui->actInStatPz->setEnabled(false);
            ui->actAllInstat->setEnabled(false);
            ui->actAllVerify->setEnabled(false);
            ui->actReassignPzNum->setEnabled(false);
        }
        else{
            //添加、插入凭证（这些操作都是针对手工录入凭证）
            r = curUser->haveRight(allRights.value(Right::AddPz)) && isPzEdit
                    && (curPzSetState != Ps_Jzed)/*(curPzSetState < Ps_Stat1+1)*/;
            ui->actAddPz->setEnabled(r);
            ui->actInsertPz->setEnabled(r);
            //删除凭证
            r = curUser->haveRight(allRights.value(Right::DelPz)) && isPzEdit
                    && (curPzModel->rowCount() > 0)
                    && (curPzState == Pzs_Recording); //为简单起见，规定只有处于录入态的凭证可以删除
            ui->actDelPz->setEnabled(r);
        }

    }

    //打印动作
    if(curUser == NULL){
        ui->actPrint->setEnabled(false);
        ui->actPrintAdd->setEnabled(false);
        ui->actPrintDel->setEnabled(false);
    }
    else{
        r = (curUser->haveRight(allRights.value(Right::PrintPz)) ||
            curUser->haveRight(allRights.value(Right::PrintDetialTable)) ||
            curUser->haveRight(allRights.value(Right::PrintStatTable))) &&
                isOpenPzSet;
        ui->actPrint->setEnabled(r);

        r = isPzEdit && (curPzn != 0) && curUser->haveRight(allRights.value(Right::PrintPz));
        ui->actPrintAdd->setEnabled(r);
        ui->actPrintDel->setEnabled(r);
    }

    if(curUser == NULL){
        ui->actReassignPzNum->setEnabled(false);
        ui->actSavePz->setEnabled(false);
        ui->actAddAction->setEnabled(false);
    }
    else{
        //重置凭证号
        r = isOpenPzSet && curUser->haveRight(allRights.value(Right::EditPz))
                && (curPzSetState == Ps_Rec);
        ui->actReassignPzNum->setEnabled(r);

        //保存动作
        PzDialog2* pzEdit;
        if(isPzEdit){
            pzEdit = qobject_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
            r = isPzEdit && pzEdit->isDirty();
        }
        else
            r = false;
        ui->actSavePz->setEnabled(r);

        //增加业务按钮（还未考虑凭证集状态）
        r = (curPzState == Pzs_Repeal) || (curPzState == Pzs_Recording);
        ui->actAddAction->setEnabled(r);
    }



    //数据菜单处理
    if(curUser == NULL){
        ui->actSubExtra->setEnabled(false);
        //ui->actCollCal->setEnabled(false);  //该菜单项已弃用
        ui->actCurStat->setEnabled(false);
        ui->mnuAccBook->setEnabled(false);
        ui->mnuReport->setEnabled(false);
    }
    else{
        ui->actSubExtra->setEnabled(isOpenPzSet);
        //ui->actCollCal->setEnabled(isOpenPzSet);
        ui->actCurStat->setEnabled(isOpenPzSet);
        ui->mnuAccBook->setEnabled(isOpenPzSet);
        ui->mnuReport->setEnabled(isOpenPzSet);
    }

    //工具菜单处理
    if(curUser == NULL){
        ui->actSetupAccInfos->setEnabled(false);
        ui->actSetupBankInfos->setEnabled(false);
        ui->actSetupBase->setEnabled(false);
        ui->actSetupBankInfos->setEnabled(false);
        ui->actSqlTool->setEnabled(false);
        ui->actSecCon->setEnabled(false);

    }
    else{
        if(curAccount && curAccount->getReadOnly()){
            //ui->mnuHandyTools->setEnabled(false);
            //ui->mnuAdminTools->setEnabled(false);
            ui->actSetupAccInfos->setEnabled(false);
            ui->actSetupBase->setEnabled(false);
            ui->actSetupBankInfos->setEnabled(false);
            ui->actGdzcAdmin->setEnabled(false);
            ui->actDtfyAdmin->setEnabled(false);
        }
        else{
            //这里进行了简化，其实应该考虑操作权限
            r = (curAccountId != 0);
            ui->actSetupAccInfos->setEnabled(r);
            ui->actSetupBankInfos->setEnabled(r);
            ui->actSetupBase->setEnabled(r);
            ui->actSetupBankInfos->setEnabled(r);
            ui->actSqlTool->setEnabled(r);
            ui->actSecCon->setEnabled(r);
        }
    }

    //选项菜单处理
    if(curUser == NULL){
        ui->actSubConfig->setEnabled(false);
    }
    else{
        if(curAccount && !curAccount->getReadOnly())
            ui->actSubConfig->setEnabled(true);
        else
            ui->actSubConfig->setEnabled(false);
    }

    //凭证编辑动作


    //r = r && (((curPzSetState == PS_ORI) && (curPzState == PZS_RECORDING))
    //          ||(curPzSetState == PS_ORI && curPzModel->rowCount() == 0));


    //删除凭证


//
//    r = curUser->haveRight(allRights.value(Right::EditPz)) && dlgEditPz;
//    ui->actAddAction->setEnabled(r);

//    //ui->actDelAction->setEnabled(isEnabled);
//    ui->actSavePz->setVisible(isEnabled);

    //r = (dlgPzEdit != NULL);
    //ui->actClosePz->setEnabled(r);



}

//刷新高级凭证操作按钮的启用状态(应综合考虑用户权限和凭证的当前状态)
void MainWindow::refreshAdvancPzOperBtn()
{
    bool r;
    r = curUser->haveRight(allRights.value(Right::RepealPz));
    ui->actRepealPz->setEnabled(r);
    rdoRepealPz->setEnabled(r);
    r = curUser->haveRight(allRights.value(Right::VerifyPz));
    r = r && (curPzState == Pzs_Recording);
    ui->actVerifyPz->setEnabled(r);
    rdoVerifyPz->setEnabled(r);
    r = curUser->haveRight(allRights.value(Right::InstatPz));
    r = r && (curPzState == Pzs_Verify);
    ui->actInStatPz->setEnabled(r);
    rdoInstatPz->setEnabled(r);

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

QSqlTableModel* MainWindow::initSingleTableModel(int witch)
{
    QSqlTableModel* model;
    QSqlRelationalTableModel* rmodel;
    CustomRelationTableModel* crmodel;
    QList<int> sortClos;

    CustomRelationModel* cmodel;
    QMap<int, QVariant> map;
    //int row;

    switch(witch){
    case 1:                     //一级科目表
        rmodel = new QSqlRelationalTableModel;
        rmodel->setTable("FirSubjects");
        rmodel->setRelation(FSTSUB_BELONGTO, QSqlRelation("FstSubClasses", "id", "name"));
        rmodel->setHeaderData(FSTSUB_ID, Qt::Horizontal, QObject::tr("ID"));
        rmodel->setHeaderData(FSTSUB_SUBCODE, Qt::Horizontal, QSqlQueryModel::tr("科目代码"));
        rmodel->setHeaderData(FSTSUB_REMCODE, Qt::Horizontal, QObject::tr("助记符"));
        rmodel->setHeaderData(FSTSUB_BELONGTO, Qt::Horizontal, QObject::tr("科目所属类别"));
        rmodel->setHeaderData(FSTSUB_WEIGHT, Qt::Horizontal, QObject::tr("科目使用权重"));
        rmodel->setHeaderData(FSTSUB_SUBNAME, Qt::Horizontal, QObject::tr("科目名称"));
        rmodel->setHeaderData(FSTSUB_DESC, Qt::Horizontal, QObject::tr("科目简单描述"));
        rmodel->setHeaderData(FSTSUB_UTILS, Qt::Horizontal, QObject::tr("用法举例"));
        rmodel->select();
        model = rmodel;
        break;
    case 2:                     //二级科目信息表
        //rmodel = new QSqlRelationalTableModel;
        model = new QSqlTableModel;
        model->setTable("SecSubjects");
        //rmodel->setRelation(SNDSUB_PID, QSqlRelation("FirSubjects", "id", "subName"));
        model->setHeaderData(SNDSUB_ID, Qt::Horizontal, QObject::tr("ID"));
        //rmodel->setHeaderData(SNDSUB_PID, Qt::Horizontal, QSqlQueryModel::tr("所属一级科目"));
        model->setHeaderData(SNDSUB_SUBNAME, Qt::Horizontal, QObject::tr("科目名称"));
        model->setHeaderData(SNDSUB_SUBLONGNAME, Qt::Horizontal, QObject::tr("科目详名"));
        //rmodel->setHeaderData(SNDSUB_SUBCODE, Qt::Horizontal, QObject::tr("科目代码"));
        model->setHeaderData(SNDSUB_REMCODE, Qt::Horizontal, QObject::tr("科目助记符"));
        model->setHeaderData(SNDSUB_CALSS, Qt::Horizontal, QObject::tr("科目所属类别"));
        //rmodel->setHeaderData(SNDSUB_FS, Qt::Horizontal, QObject::tr("使用频度统计值"));
        model->select();
        //model = rmodel;
        break;
    case 3:                     //业务活动表
        model = new QSqlTableModel;
        model->setTable("BusiActions");
        model->setHeaderData(2, Qt::Horizontal, QSqlQueryModel::tr("摘要"));
        model->setHeaderData(3, Qt::Horizontal, QSqlQueryModel::tr("一级科目"));
        model->setHeaderData(4, Qt::Horizontal, QSqlQueryModel::tr("二级科目"));
        model->setHeaderData(5, Qt::Horizontal, QSqlQueryModel::tr("借方金额"));
        model->setHeaderData(6, Qt::Horizontal, QSqlQueryModel::tr("贷方金额"));
        model->setHeaderData(7, Qt::Horizontal, QSqlQueryModel::tr("货币类型"));
        model->select();
        break;
    case 4:                   //凭证表
        crmodel = new CustomRelationTableModel;
        crmodel->setTable("PingZhengs");
        //crmodel->setRelation(PZ_ACCOUNTBOOKGROUP, QSqlRelation("AccountBookGroups", "code", "name"));

        //sortClos << PZ_ACCOUNTBOOKGROUP << Qt::AscendingOrder; //按凭证分册类别的升序
        sortClos << PZ_DATE << Qt::AscendingOrder; //按日期的升序
        sortClos << PZ_NUMBER << Qt::AscendingOrder; //按凭证总号的升序
        //sortClos << PZ_NUMBERSUB << Qt::AscendingOrder; //按凭证分号的升序
        crmodel->setSort(sortClos);
        //rmodel->setSort(PZ_NUMBER, Qt::AscendingOrder);
        model = crmodel;
        model->select();
        break;
    case 5:                   //一级科目类别表
        model = new QSqlTableModel;
        model->setTable("FstSubClasses");
        model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
        model->setHeaderData(1, Qt::Horizontal, QObject::tr("类别名称"));
        model->select();
        break;

    case FSAGENTTABLE:  //一级科目到二级科目的映射表
//        model = new QSqlTableModel;
//        model->setTable("fsagent");
//        model->setHeaderData(FSAGENT_ID, Qt::Horizontal, QObject::tr("ID"));
//        model->setHeaderData(FSAGENT_FID, Qt::Horizontal, QObject::tr("一级科目"));
//        model->setHeaderData(FSAGENT_SID, Qt::Horizontal, QObject::tr("二级科目"));
//        model->setHeaderData(FSAGENT_SUBCODE, Qt::Horizontal, QObject::tr("科目代码"));
//        model->setHeaderData(FSAGENT_FS, Qt::Horizontal, QObject::tr("使用频度统计值"));
//        model->select();
        rmodel = new QSqlRelationalTableModel;
        rmodel->setTable("FSAgent");
        rmodel->setRelation(FSAGENT_FID, QSqlRelation("FirSubjects", "id", "subName"));
        rmodel->setRelation(FSAGENT_SID, QSqlRelation("SecSubjects", "id", "subName"));
        rmodel->setHeaderData(FSAGENT_ID, Qt::Horizontal, QObject::tr("ID"));
        rmodel->setHeaderData(FSAGENT_FID, Qt::Horizontal, QObject::tr("一级科目"));
        rmodel->setHeaderData(FSAGENT_SID, Qt::Horizontal, QObject::tr("二级科目"));
        rmodel->setHeaderData(FSAGENT_SUBCODE, Qt::Horizontal, QObject::tr("科目代码"));
        rmodel->setHeaderData(FSAGENT_FS, Qt::Horizontal, QObject::tr("使用频度统计值"));

        rmodel->select();
        model = rmodel;
        break;

    case SNDSUBCLSTABLE:                   //二级科目类别表
        model = new QSqlTableModel;
        model->setTable("SndSubClass");
        model->setHeaderData(SNDSUBCLASS_ID, Qt::Horizontal, QObject::tr("ID"));
        model->setHeaderData(SNDSUBCLASS_CODE, Qt::Horizontal, QObject::tr("类别代码"));
        model->setHeaderData(SNDSUBCLASS_NAME, Qt::Horizontal, QObject::tr("类别名称"));
        model->setHeaderData(SNDSUBCLASS_EXPLAIN, Qt::Horizontal, QObject::tr("简要说明"));
        model->select();
        break;

    case ACCOUNTINFO:                 //账户信息表
        model = new QSqlTableModel;
        model->setTable("AccountInfos");
        model->setHeaderData(ACCOUNTINFO_ID, Qt::Horizontal, QObject::tr("ID"));
        model->setHeaderData(ACCOUNT_SNAME, Qt::Horizontal, QObject::tr("账户简称"));
        model->setHeaderData(ACCOUNT_LNAME, Qt::Horizontal, QObject::tr("账户全称"));
        model->select();
        break;

    case EXCHANGERATE:    //汇率表
        model = new QSqlTableModel;
        model->setTable("ExchangeRates");
        model->setHeaderData(EXCHANGERATE_ID, Qt::Horizontal, QObject::tr("ID"));
        model->setHeaderData(EXCHANGERATE_YEAR, Qt::Horizontal, QObject::tr("年份"));
        model->setHeaderData(EXCHANGERATE_MONTH, Qt::Horizontal, QObject::tr("月份"));
        model->setHeaderData(EXCHANGERATE_USDTORMB, Qt::Horizontal, QObject::tr("美元兑rmb"));
        model->select();
        break;

    case ACCOUNTBOOKGROUP:      //账簿分类表
        model = new QSqlTableModel;
        model->setTable("AccountBookGroups");
        model->setHeaderData(ACCBOOKGROUP_ID, Qt::Horizontal, QObject::tr("ID"));
        model->setHeaderData(ACCBOOKGROUP_NAME, Qt::Horizontal, QObject::tr("账簿名称"));
        model->setHeaderData(ACCBOOKGROUP_EXPLAIN, Qt::Horizontal, QObject::tr("对归属于此账簿的凭证说明"));
        model->select();
        break;

    case SUJECTEXTRA:   //科目余额表
        model = new QSqlTableModel;
        model->setTable("SubjectExtras");
        model->select();
        break;

    case REPORTSTRUCTS:   //报表结构信息表
        model = new QSqlTableModel;
        model->setTable("ReportStructs");
        model->setHeaderData(RS_CLSID, Qt::Horizontal, QObject::tr("报表类别代码D"));
        model->setHeaderData(RS_TYPE, Qt::Horizontal, QObject::tr("报表分类代码"));
        model->setHeaderData(RS_VIEWORDER, Qt::Horizontal, QObject::tr("显示顺序"));
        model->setHeaderData(RS_CALORDER, Qt::Horizontal, QObject::tr("计算顺序"));
        model->setHeaderData(RS_FIELDNAME, Qt::Horizontal, QObject::tr("报表字段名"));
        model->setHeaderData(RS_FIELDTITLE, Qt::Horizontal, QObject::tr("报表字段标题"));
        model->setHeaderData(RS_FIELDFORMULA, Qt::Horizontal, QObject::tr("报表字段的计算公式"));
        model->select();
        break;


    case REPORTADDITIONINFO:   //报表其他信息表
        model = new QSqlTableModel;
        model->setTable("ReportAdditionInfo");
        model->setHeaderData(RAI_CLSID, Qt::Horizontal, QObject::tr("报表类别代码D"));
        model->setHeaderData(RAI_TYPE, Qt::Horizontal, QObject::tr("报表分类代码"));
        model->setHeaderData(RAI_TABLENAME, Qt::Horizontal, QObject::tr("表名"));
        model->setHeaderData(RAI_TITLE, Qt::Horizontal, QObject::tr("报表标题"));
        model->select();
        break;

    case INCOMESTATMENTS_OLD:   //利润表（老）
        model = new QSqlTableModel;
        model->setTable("OldProfits");
        model->setHeaderData(PROFITS_YEAR, Qt::Horizontal, QObject::tr("年份"));
        model->setHeaderData(PROFITS_MONTH, Qt::Horizontal, QObject::tr("月份"));
        //建议从报表信息结构表中读取报表字段标题
        //model->setHeaderData(PROFITS_START+1, Qt::Horizontal, QObject::tr("显示顺序"));
        //model->setHeaderData(PROFITS_START+2, Qt::Horizontal, QObject::tr("计算顺序"));
        //model->setHeaderData(PROFITS_START+3, Qt::Horizontal, QObject::tr("报表字段名"));

        model->select();
        break;

    case INCOMESTATMENTS_NEW:   //利润表（新）
        model = new QSqlTableModel;
        model->setTable("NewProfits");
        model->setHeaderData(PROFITS_YEAR, Qt::Horizontal, QObject::tr("年份"));
        model->setHeaderData(PROFITS_MONTH, Qt::Horizontal, QObject::tr("月份"));
        //建议从报表信息结构表中读取报表字段标题
        //model->setHeaderData(PROFITS_START+1, Qt::Horizontal, QObject::tr("显示顺序"));
        //model->setHeaderData(PROFITS_START+2, Qt::Horizontal, QObject::tr("计算顺序"));
        //model->setHeaderData(PROFITS_START+3, Qt::Horizontal, QObject::tr("报表字段名"));

        model->select();
        break;

    case 40:  //原始的凭证表（显示表的原始字段的数据）
        model = new QSqlTableModel;
        model->setTable("PingZhengs");
        model->select();
        break;
    }



    return model;
}

//凭证状态改变处理槽（用以在用户导航凭证时正确显示和设置凭证的当前状态）
void MainWindow::pzStateChange(int scode)
{
    curPzState = scode;
    switch(scode){
    case Pzs_Recording:   //录入态
        rdoRecording->setChecked(true);
        ui->actRepealPz->setChecked(false);
        ui->actVerifyPz->setChecked(false);
        ui->actInStatPz->setChecked(false);        
        break;
    case Pzs_Repeal:      //作废
        rdoRepealPz->setChecked(true);
        ui->actRepealPz->setChecked(true);
        ui->actVerifyPz->setChecked(false);
        ui->actInStatPz->setChecked(false);        
        break;
    case Pzs_Verify:    //已审核
        rdoVerifyPz->setChecked(true);
        ui->actVerifyPz->setChecked(true);
        ui->actRepealPz->setChecked(false);
        ui->actInStatPz->setChecked(false);        
        break;
    case Pzs_Instat:      //已记账
        rdoInstatPz->setChecked(true);
        ui->actInStatPz->setChecked(true);
        ui->actRepealPz->setChecked(false);
        ui->actVerifyPz->setChecked(false);        
        break;
    }
    //refreshActEnanble();

    refreshAdvancPzOperBtn();
}

//当前凭证的内容发生了改变
void MainWindow::pzContentChanged(bool isChanged)
{    
    ui->actSavePz->setEnabled(isChanged);
}

//当前凭证改变了，主要用来控制凭证导航按钮的有效性，并获取凭证类别等其他信息
//参数idx：当前凭证在凭证集内的序号（基于1），nums：凭证集内的凭证总数
void MainWindow::curPzIndexChanged(int idx, int nums)
{
    curPzCls = (PzClass)curPzModel->data(curPzModel->index(idx,PZ_CLS)).toInt();
    bool first=true,last=true,next=true,prev=true;
    if((nums == 0) || (nums == 1)){
        first=last=next=prev=false;
    }
    else if((nums > 1) && (idx == 1)){  //第一张
        first= prev=false;
    }
    else if((nums > 1) && (idx == nums)){ //最后一张
        next=last=false;
    }

    ui->actGoFirst->setEnabled(first);
    ui->actGoLast->setEnabled(last);
    ui->actGoNext->setEnabled(next);
    ui->actGoPrev->setEnabled(prev);
}

//凭证内容已经保存
void MainWindow::pzContentSaved()
{
    ui->actSavePz->setEnabled(false);
}

//用户通过工具条上改变了当前凭证的状态，通过此函数告诉凭证编辑窗口，使它能保存
void MainWindow::userModifyPzState(bool checked)
{
    QRadioButton* send = qobject_cast<QRadioButton*>(sender());
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    if(send == rdoVerifyPz){
        curPzState = Pzs_Verify;
        pzEdit->setPzState(Pzs_Verify,curUser);
    }
    else if(send == rdoInstatPz){
        curPzState = Pzs_Instat;
        pzEdit->setPzState(Pzs_Instat,curUser);
    }
    else if(send == rdoRepealPz){
        curPzState = Pzs_Repeal;
        pzEdit->setPzState(Pzs_Repeal,curUser);
    }
    else{
        curPzState = Pzs_Recording;
        pzEdit->setPzState(Pzs_Recording,curUser);
    }
    refreshAdvancPzOperBtn();
    ui->actSavePz->setEnabled(true);    
}

//根据用户所选的业务活动的状况决定移动和删除业务活动按钮的启用状态
void MainWindow::userSelBaAction(bool isSel)
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
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

//处理添加凭证动作事件
void MainWindow::on_actAddPz_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    if(curPzSetState != Ps_Jzed){
        pzEdit->addPz();
        //BusiUtil::setPzsState(cursy,cursm,Ps_Rec);
        refreshShowPzsState();
        refreshActEnanble();
    }
}

//插入凭证
void MainWindow::on_actInsertPz_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    if(curPzSetState < Ps_Stat1+1){
        pzEdit->insertPz();
        BusiUtil::setPzsState(cursy,cursm,Ps_Rec);
        refreshShowPzsState();
        refreshActEnanble();
    }
}

//删除凭证（综合考虑凭证集状态和凭证类别，来决定新的凭证集状态）
void MainWindow::on_actDelPz_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->delPz();
    PzsState newState;
    if((curPzSetState >= Ps_Rec) && (curPzSetState <= Ps_Stat1))
        newState = Ps_Rec;
    else if((curPzSetState >= Ps_ImpOther) && (curPzSetState <= Ps_Stat2))
        newState = Ps_Stat1;
    else if((curPzSetState >= Ps_Jzhd) && (curPzSetState <= Ps_Stat3))
        newState = Ps_Stat1;
    else if((curPzSetState >= Ps_Jzsy) && (curPzSetState <= Ps_Stat4)){
        bool req1,req2;
        BusiUtil::reqGenImpOthPz(cursy,cursm,req1);
        BusiUtil::reqGenJzHdsyPz(cursy,cursm,req2);
        if(req1 && req2)
            newState = Ps_Stat1;
        else if(req1 && !req2)
            newState = Ps_Stat3;
        else if(!req1 && req2)
            newState = Ps_Stat2;
        else
            newState = Ps_JzsyPre;
    }

}

//新增业务活动
void MainWindow::on_actAddAction_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->addBusiAct();
}



//删除业务活动
void MainWindow::on_actDelAction_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->delBusiAct();
    //因为只能删除录入态的凭证，因此不会引起凭证集状态的改变
    //BusiUtil::refreshPzsState(cursy,cursm,curPzModel);
    //refreshPzSetState();
    //refreshActEnanble();
}

//导航到第一张凭证
void MainWindow::on_actGoFirst_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->naviFst();
}

//导航到前一张凭证
void MainWindow::on_actGoPrev_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->naviPrev();
}

//导航到后一张凭证
void MainWindow::on_actGoNext_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->naviNext();
}

//导航到最后一张凭证
void MainWindow::on_actGoLast_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->naviLast();
}

//提交对凭证的改变
void MainWindow::on_actSavePz_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->save();
    ui->actSavePz->setEnabled(false);
    refreshShowPzsState();
}

//判定损益类科目是收入类还是费用类（临时代码）
bool MainWindow::isIncoming(int id)
{
    QSet<int> in; //收入类科目id
    QSet<int> fei; //费用类科目id
    in << 72 << 73 << 76;
    fei <<77<<78<<79<<80<<81<<82<<83;

    if(in.contains(id))
        return true;
    else
        return false;
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
    VariousUtils::saveSubWinInfo(winEnum,info,sinfo);
    delete info;
}

//读取当前凭证集的最新状态，并设置对应变量和状态条信息
void MainWindow::refreshShowPzsState()
{
    PzsState old = curPzSetState;
    BusiUtil::getPzsState(cursy,cursm,curPzSetState);
    if(old != curPzSetState){
        lblPzSetState->setText(pzsStates.value(curPzSetState));
        refreshActEnanble();

        //通知其他相关子窗口（某些子窗口可能需要重新进行计算，比如本期统计等）
    }
}

//结转汇兑损益
void MainWindow::on_actFordEx_triggered()
{
//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }

//    //给用户一个选项，是否真的需要执行汇兑损益的结转
//    QMessageBox* dlg = new 	QMessageBox(this);
//    QAbstractButton* btnOk = dlg->addButton(tr("确定"),QMessageBox::AcceptRole);
//    QAbstractButton* btnSkit = dlg->addButton(tr("跳过"),QMessageBox::AcceptRole);
//    QAbstractButton* btnCancel = dlg->addButton(tr("取消"),QMessageBox::RejectRole);
//    dlg->exec();
//    if(btnCancel == dlg->clickedButton()){
//        int i = 0;
//        return;
//    }
//    else if(btnOk == dlg->clickedButton()){
//        if(BusiUtil::genForwordEx(cursy,cursm,curUser)){
//            showTemInfo(tr("成功创建结转汇兑损益凭证"));
//            refreshShowPzsState();
//            refreshActEnanble();
//        }
//        else
//            showTemInfo(tr("在创建结转汇兑损益凭证时出错"));
//        return;
//    }
//    else{ //跳过结转时，设置下月的汇率为本月汇率，且设置凭证集状态
//        int y,m;
//        if(cursm == 12){
//            y = cursy + 1;
//            m = 1;
//        }
//        else{
//            y = cursy;
//            m = cursm + 1;
//        }
//        BusiUtil::saveRates(y,m,curRates);
//        bool req;
//        BusiUtil::reqGenImpOthPz(cursy,cursm,req);
//        if(req)
//            BusiUtil::setPzsState(cursy,cursm,Ps_Stat3);
//        else  //如果同时也不需要引入其他凭证，则直接准备结转损益
//            BusiUtil::setPzsState(cursy,cursm,Ps_JzsyPre);
//        refreshShowPzsState();
//        refreshActEnanble();
//        return;
//    }
}

//结转损益
void MainWindow::on_actFordPl_triggered()
{
//    if(!isOpenPzSet){
//        pzsWarning();
//        return;
//    }

//    PzsState state;
//    if(!BusiUtil::getPzsState(cursy,cursm,state)){
//        showTemInfo(tr("不能读取凭证集状态，操作无法完成！"));
//        return;
//    }
//    //进行结转损益的先决状态是Ps_JzsyPre
//    if(state != Ps_JzsyPre){
//        QMessageBox::information(this, tr("提示信息"),
//                                 tr("当前凭证集还未做好结转损益前的准备，\n"
//                                    "这通常是由于还未执行引入其他凭证或结转汇兑损益操作引起的！"));
//        return;
//    }
//    if(state >= Ps_JzsyV){
//        QMessageBox::information(this, tr("提示信息"),
//                                 tr("你已经审核了结转损益凭证，必须先取消审核才能重新执行！"));
//        return;
//    }

/////////////这些代码原先就已经注释掉的////////////////////////
//        bool req = false;

//    //如果执行了引入其他凭证，但没有执行结转汇兑损益操作
//    if(!BusiUtil::reqGenJzHdsyPz(cursy,cursm,req)){
//        QMessageBox::information(this, tr("错误信息"),
//                                 tr("在请求是否需要生成结转汇兑损益凭证时出错"));
//        return;
//    }
//    if((state == Ps_Stat2) && req){
//        QMessageBox::information(this, tr("提示信息"),
//                                 tr("你还没有执行结转汇兑损益操作！"));
//        return;
//    }

//    //如果执行了结转汇兑损益但没有执行引入其他凭证操作
//    if(!BusiUtil::reqGenImpOthPz(cursy,cursm,req)){
//        QMessageBox::information(this, tr("错误信息"),
//                                 tr("在请求是否需要引入其他凭证时出错"));
//        return;
//    }
//    if((state == Ps_Stat3) && req){
//        QMessageBox::information(this, tr("提示信息"),
//                                 tr("你还没有执行引入其他凭证操作！"));
//        return;
//    }
///////////////////////////////////////////////////////////////////////////////

//    if(BusiUtil::genForwordPl(cursy,cursm,curUser)){
//        showTemInfo(tr("成功创建结转损益凭证！"));
//        refreshShowPzsState();
//        refreshActEnanble();
//    }
//    else
//        showTemInfo(tr("在创建结转损益凭证的过程中发生了错误！"));

}

//统计本期发生额及其科目余额
void MainWindow::on_actCurStat_triggered()
{
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }
    ViewExtraDialog* dlg;
    if(subWindows.contains(PZSTAT)){
        dlg = static_cast<ViewExtraDialog*>(subWindows.value(PZSTAT)->widget());
        dlg->setDate(cursy,cursm);
        //激活此子窗口
        ui->mdiArea->setActiveSubWindow(subWindows.value(PZSTAT));
        return;
    }

    SubWindowDim* winfo;
    QByteArray* sinfo;
    bool exist = VariousUtils::getSubWinInfo(PZSTAT,winfo,sinfo);
    dlg = new ViewExtraDialog(cursy, cursm, sinfo, this );
    connect(dlg,SIGNAL(infomation(QString)),this,SLOT(showTemInfo(QString)));
    connect(dlg,SIGNAL(pzsStateChanged()),this,SLOT(refreshShowPzsState()));
    //dlg->setDate(cursy,cursm);    

    showSubWindow(PZSTAT,winfo,dlg);
    //if(exist)
    //    showSubWindow(PZSTAT,&winfo,dlg);
    //else
    //    showSubWindow(PZSTAT,NULL,dlg);
    if(winfo)
        delete winfo;
    if(sinfo)
        delete sinfo;

//    subWindows[PZSTAT] = new MyMdiSubWindow;
//    subWindows[PZSTAT]->setWidget(dlgPzStat);
//    //设置凭证本期统计窗口的激活状态（必须在子窗口加入到mdi区域之前设置，精确地说应该在show之前）
//    if(!subWinActStates.contains(PZSTAT))
//        subWinActStates[PZSTAT] = true;
//    ui->mdiArea->addSubWindow(subWindows[PZSTAT]);
//    connect(subWindows[PZSTAT], SIGNAL(windowClosed(QMdiSubWindow*)),
//            this, SLOT(subWindowClosed(QMdiSubWindow*)));
//    //在此之前一定要设置子窗口的激活状态
//    dlgPzStat->show();

//    SubWindowInfo* info = new SubWindowInfo;
//    if(VariousUtils::getSubWinInfo(PZSTAT,info)){
//        subWindows.value(PZSTAT)->move(info->x,info->y);
//        subWindows.value(PZSTAT)->resize(info->w,info->h);
//    }
//    else{
//        int w = 1322; //对话框的首选尺寸
//        int h = 650;
//        w = (w>(screenWidth*0.8))? w:(screenWidth * 0.8);
//        h = (h>(screenHeight*0.9))? h:(screenHeight * 0.9);
//        subWindows.value(PZSTAT)->move(10,10);
//        subWindows.value(PZSTAT)->resize(w, h);
//    }
}

//显示现金日记账
void MainWindow::on_actCashJournal_triggered()
{
    if(subWindows.contains(CASHDAILY)){
        dlgCashDaily->refresh();
        ui->mdiArea->setActiveSubWindow(subWindows.value(CASHDAILY));
        return;
    }
    dlgCashDaily = new DetailsViewDialog2;
    dlgCashDaily->setDateLimit(cursy, cursm, curey, curem);
    dlgCashDaily->setWindowTitle(tr("现金日记账"));    
    subWindows[CASHDAILY] = new MyMdiSubWindow;
    subWindows[CASHDAILY]->setWidget(dlgCashDaily);
    ui->mdiArea->addSubWindow(subWindows[CASHDAILY]);
    connect(subWindows[CASHDAILY], SIGNAL(windowClosed(QMdiSubWindow*)),
            this, SLOT(subWindowClosed(QMdiSubWindow*)));
    //设置凭证本期统计窗口的激活状态
    if(!subWinActStates.contains(CASHDAILY))
        subWinActStates[CASHDAILY] = true;
    dlgCashDaily->show();
    SubWindowDim* info = new SubWindowDim;
    QByteArray* oinfo;
    if(VariousUtils::getSubWinInfo(CASHDAILY,info,oinfo)){
        subWindows.value(CASHDAILY)->move(info->x,info->y);
        subWindows.value(CASHDAILY)->resize(info->w,info->h);
    }
    else{
        int w = 1322; //对话框的首选尺寸
        int h = 650;
        w = (w>(screenWidth*0.8))? w:(screenWidth * 0.8);
        h = (h>(screenHeight*0.9))? h:(screenHeight * 0.9);
        subWindows.value(CASHDAILY)->move(10,10);
        subWindows.value(CASHDAILY)->resize(w, h);
    }
    delete info;
    connect(dlgCashDaily, SIGNAL(openSpecPz(int,int)), this, SLOT(openSpecPz(int,int)));

}

//显示银行日记帐
void MainWindow::on_actBankJournal_triggered()
{
    if(dlgBankDaily){
        dlgBankDaily->refresh();
        ui->mdiArea->setActiveSubWindow(subWindows.value(BANKDAILY));
    }
    dlgBankDaily = new DetailsViewDialog2(2);
    dlgBankDaily->setDateLimit(cursy, cursm, curey, curem);
    dlgBankDaily->setWindowTitle(tr("银行日记账"));
    subWindows[BANKDAILY] = new MyMdiSubWindow;
    subWindows[BANKDAILY]->setWidget(dlgBankDaily);
    ui->mdiArea->addSubWindow(subWindows[BANKDAILY]);
    connect(subWindows[BANKDAILY], SIGNAL(windowClosed(QMdiSubWindow*)),
            this, SLOT(subWindowClosed(QMdiSubWindow*)));
    //设置凭证银行日记账窗口的激活状态
    if(!subWinActStates.contains(BANKDAILY))
        subWinActStates[BANKDAILY] = true;
    dlgBankDaily->show();
    SubWindowDim* info = new SubWindowDim;
    QByteArray* oinfo;
    if(VariousUtils::getSubWinInfo(BANKDAILY,info,oinfo)){
        subWindows.value(BANKDAILY)->move(info->x,info->y);
        subWindows.value(BANKDAILY)->resize(info->w,info->h);
    }
    else{
        int w = 1322; //对话框的首选尺寸
        int h = 650;
        w = (w>(screenWidth*0.8))? w:(screenWidth * 0.8);
        h = (h>(screenHeight*0.9))? h:(screenHeight * 0.9);
        subWindows.value(BANKDAILY)->move(10,10);
        subWindows.value(BANKDAILY)->resize(w, h);
    }
    delete info;
    connect(dlgBankDaily, SIGNAL(openSpecPz(int,int)), this, SLOT(openSpecPz(int,int)));
}

//显示明细帐
void MainWindow::on_actSubsidiaryLedger_triggered()
{
    if(dlgDetailDaily){
        dlgDetailDaily->refresh();
        ui->mdiArea->setActiveSubWindow(subWindows.value(DETAILSDAILY));
        return;
    }
    dlgDetailDaily = new DetailsViewDialog2(3);
    dlgDetailDaily->setDateLimit(cursy, cursm, curey, curem);
    dlgDetailDaily->setWindowTitle(tr("明细账"));
    connect(dlgDetailDaily, SIGNAL(openSpecPz(int,int)), this, SLOT(openSpecPz(int,int)));
    subWindows[DETAILSDAILY] = new MyMdiSubWindow;
    subWindows[DETAILSDAILY]->setWidget(dlgDetailDaily);
    ui->mdiArea->addSubWindow(subWindows[DETAILSDAILY]);
    connect(subWindows[DETAILSDAILY], SIGNAL(windowClosed(QMdiSubWindow*)),
            this, SLOT(subWindowClosed(QMdiSubWindow*)));
    if(!subWinActStates.contains(DETAILSDAILY))
        subWinActStates[DETAILSDAILY] = true;
    dlgDetailDaily->show();
    SubWindowDim* info = new SubWindowDim;
    QByteArray* oinfo;
    if(VariousUtils::getSubWinInfo(CASHDAILY,info,oinfo)){
        subWindows.value(DETAILSDAILY)->move(info->x,info->y);
        subWindows.value(DETAILSDAILY)->resize(info->w,info->h);
    }
    else{
        int w = 1322; //对话框的首选尺寸
        int h = 650;
        w = (w>(screenWidth*0.8))? w:(screenWidth * 0.8);
        h = (h>(screenHeight*0.9))? h:(screenHeight * 0.9);
        subWindows.value(DETAILSDAILY)->move(10,10);
        subWindows.value(DETAILSDAILY)->resize(w, h);
    }
    delete info;
}

//显示总分类帐
void MainWindow::on_actLedger_triggered()
{
    if(dlgTotalDaily){
        dlgTotalDaily->refresh();
        ui->mdiArea->setActiveSubWindow(subWindows.value(TOTALDAILY));
        return;
    }
    dlgTotalDaily = new DetailsViewDialog2(4);
    dlgTotalDaily->setDateLimit(cursy, cursm, curey, curem);
    dlgTotalDaily->setWindowTitle(tr("总分类账"));
    connect(dlgTotalDaily, SIGNAL(openSpecPz(int,int)), this, SLOT(openSpecPz(int,int)));

    subWindows[TOTALDAILY] = new MyMdiSubWindow;
    subWindows[TOTALDAILY]->setWidget(dlgTotalDaily);
    ui->mdiArea->addSubWindow(subWindows[TOTALDAILY]);
    connect(subWindows[TOTALDAILY], SIGNAL(windowClosed(QMdiSubWindow*)),
            this, SLOT(subWindowClosed(QMdiSubWindow*)));
    if(!subWinActStates.contains(TOTALDAILY))
        subWinActStates[TOTALDAILY] = true;
    dlgTotalDaily->show();
    SubWindowDim* info = new SubWindowDim;
    QByteArray* oinfo;
    if(VariousUtils::getSubWinInfo(TOTALDAILY,info,oinfo)){
        subWindows.value(TOTALDAILY)->move(info->x,info->y);
        subWindows.value(TOTALDAILY)->resize(info->w,info->h);
    }
    else{
        int w = 1322; //对话框的首选尺寸
        int h = 650;
        w = (w>(screenWidth*0.8))? w:(screenWidth * 0.8);
        h = (h>(screenHeight*0.9))? h:(screenHeight * 0.9);
        subWindows.value(TOTALDAILY)->move(10,10);
        subWindows.value(TOTALDAILY)->resize(w, h);
    }
    delete info;
}

//打印
void MainWindow::on_actPrint_triggered()
{
    PrintSelectDialog* psDlg = new PrintSelectDialog(this);

    psDlg->setPzSet(PrintPznSet);
    psDlg->setCurPzn(curPzn);
    if(psDlg->exec() == QDialog::Accepted){
        QSet<int> pznSet;
        int range = psDlg->getPrintPzSet(pznSet);//获取打印范围
        int mode = psDlg->getPrintMode();        //获取打印模式
        QHash<int,Double> rates;
        BusiUtil::getRates2(cursy,cursm,rates);        //获取汇率
        QString sname,lname;
        BusiUtil::getCompanyName(sname,lname);   //获取账户名

        QPrinter printer;
        QPrintDialog* dlg = new QPrintDialog(&printer); //获取所选的打印机
        if(dlg->exec() == QDialog::Accepted){
            QPrintPreviewDialog* preview;
            printer.setOrientation(QPrinter::Portrait);
            QList<PzPrintData2*> datas; //获取凭证数据
            if(range == 0)
                BusiUtil::genPzPrintDatas2(cursy,cursm,datas);
            else{
                if(range == 1){//当前凭证
                    if(curPzn == 0)
                        return;
                    QSet<int> set;
                    set.insert(curPzn);
                    BusiUtil::genPzPrintDatas2(cursy,cursm,datas,set);
                }
                else{ //自选凭证
                    BusiUtil::genPzPrintDatas2(cursy,cursm,datas,pznSet);
                }
            }

            PrintPzUtils* view = new PrintPzUtils(&printer);
            view->setRates(rates);
            view->setPzDatas(datas);
            //去除账户名称中的括弧及其内部的内容
            int idx = lname.indexOf(tr("（"));
            if(idx != -1)
                lname.chop(lname.count() - idx);
            view->setCompanyName(lname);

            switch(mode){
            case 1: //输出地打印机
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
    }
    delete psDlg;
}

//接收当前显示的凭证号
void MainWindow::curPzNumChanged(int pzNum)
{
    curPzn = pzNum;
}

//在子窗口被关闭后，删除对应的子窗口对象实例
void MainWindow::subWindowClosed(QMdiSubWindow* subWin)
{
    QByteArray* sinfo = NULL;
    subWindowType winEnum = NONE;
    if(subWin == subWindows.value(PZEDIT)){
        winEnum = PZEDIT;
        PzDialog2* dlg = static_cast<PzDialog2*>(subWin->widget());
        dlg->save(false);
        delete dlg;
        curPzn = 0;
    }
    else if(subWin == subWindows.value(PZSTAT)){
        winEnum = PZSTAT;
        ViewExtraDialog* dlg = static_cast<ViewExtraDialog*>(subWin->widget());
        sinfo = dlg->getState();
        delete dlg;
    }
    ///////////////////////////这些处理在将来可能不需要了，因为由其他的对话框来代替/////////////////////////////////
    else if(subWin == subWindows.value(CASHDAILY)){
        winEnum = CASHDAILY;
        delete dlgCashDaily;
        dlgCashDaily = NULL;
        //DetailsViewDialog2* dlg = static_cast<DetailsViewDialog2*>(subWin->widget());
        //delete dlg;
    }
    else if(subWin == subWindows.value(BANKDAILY)){
        winEnum = BANKDAILY;
        delete dlgBankDaily;
        dlgBankDaily = NULL;
        //DetailsViewDialog2* dlg = static_cast<DetailsViewDialog2*>(subWin->widget());
        //delete dlg;
    }
    else if(subWin == subWindows.value(DETAILSDAILY)){
        winEnum = DETAILSDAILY;
        delete dlgDetailDaily;
        dlgDetailDaily = NULL;
        //DetailsViewDialog2* dlg = static_cast<DetailsViewDialog2*>(subWin->widget());
        //delete dlg;
    }
    else if(subWin == subWindows.value(TOTALDAILY)){
        winEnum = TOTALDAILY;
        delete dlgTotalDaily;
        dlgTotalDaily = NULL;
        //DetailsViewDialog2* dlg = static_cast<DetailsViewDialog2*>(subWin->widget());
        //delete dlg;
    }
    ////////////////////////////////////////////////////////////////////
    else if(subWin == subWindows.value(SETUPBASE)){
        winEnum = SETUPBASE;
        SetupBaseDialog2* dlg = static_cast<SetupBaseDialog2*>(subWin->widget());
        delete dlg;
    }
    else if(subWin == subWindows.value(SETUPBANK)){
        winEnum = SETUPBANK;
        SetupBankDialog* dlg = static_cast<SetupBankDialog*>(subWin->widget());
        delete dlg;
    }
    else if(subWin == subWindows.value(BASEDATAEDIT)){
        winEnum = BASEDATAEDIT;
        BaseDataEditDialog* dlg = static_cast<BaseDataEditDialog*>(subWin->widget());
        delete dlg;
    }
    else if(subWin == subWindows.value(GDZCADMIN)){
        winEnum = GDZCADMIN;
        GdzcAdminDialog* dlg = static_cast<GdzcAdminDialog*>(subWin->widget());
        dlg->save();
        sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(DTFYADMIN)){
        winEnum = DTFYADMIN;
        DtfyAdminDialog* dlg = static_cast<DtfyAdminDialog*>(subWin->widget());
        dlg->save();
        sinfo = dlg->getState();
        delete dlg;
    }
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
    else if(subWin == subWindows.value(HISTORYVIEW)){
        winEnum = HISTORYVIEW;
        HistoryPzDialog* dlg = static_cast<HistoryPzDialog*>(subWin->widget());
        sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(LOOKUPSUBEXTRA)){
        winEnum = LOOKUPSUBEXTRA;
        LookupSubjectExtraDialog* dlg =
                static_cast<LookupSubjectExtraDialog*>(subWin->widget());
        //sinfo = dlg->getState();
        delete dlg;
    }
    else if(subWin == subWindows.value(ACCOUNTPROPERTY)){
        winEnum = ACCOUNTPROPERTY;
        AccountPropertyDialog* dlg =
                static_cast<AccountPropertyDialog*>(subWin->widget());
        dlg->save(true);
        delete dlg;
    }
    else if(subWin == subWindows.value(VIEWPZSETERROR)){
        winEnum = VIEWPZSETERROR;
        ViewPzSetErrorForm* form = static_cast<ViewPzSetErrorForm*>(subWin->widget());
        delete form;
    }

    if(winEnum != NONE){
        saveSubWinInfo(winEnum,sinfo);
        subWindows.remove(winEnum);
    }
    ui->mdiArea->removeSubWindow(subWin);
    refreshTbrVisble();
}

//当子窗口被激活时，更新内部记录字窗口激活状态的变量 subWinActStates
void MainWindow::subWindowActivated(QMdiSubWindow *window)
{
    //找到激活的子窗口类型
    QHashIterator<subWindowType, MyMdiSubWindow*> it(subWindows);
    subWindowType winType = NONE;
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
        if(winType == NONE)
            subWinActStates[i.key()] = false;
        else if(i.key() == winType)
            subWinActStates[i.key()] = true;
        else
            subWinActStates[i.key()] = false;
    }

    refreshTbrVisble(); //刷新工具条
}

//将当前显示的凭证号加入到打印队列中
void MainWindow::on_actPrintAdd_triggered()
{
    if(curPzn != 0){
        PrintPznSet.insert(curPzn);
        //QString ps = IntSetToStr();
        QString ps = VariousUtils::IntSetToStr(PrintPznSet);
        QString msg = tr("当前选择的欲打印的凭证号： %1").arg(ps);
        ui->statusbar->showMessage(msg);
    }
}

//将当前显示的凭证号从打印队列中移除
void MainWindow::on_actPrintDel_triggered()
{
    if(curPzn != 0){
        PrintPznSet.remove(curPzn);
        QString ps;
        //ps = IntSetToStr();
        ps = VariousUtils::IntSetToStr(PrintPznSet);
        if(ps == "")
            ps = tr("无");
        //VariousUtils::IntSetToStr(PrintPznSet, ps);
        QString msg = tr("当前选择的欲打印的凭证号： %1").arg(ps);
        ui->statusbar->showMessage(msg);
    }
}

//按自编号对凭证集中的凭证进行排序
void MainWindow::on_actSortByZb_triggered()
{
//    QList<int> sortClos; //模型的排序列
//    sortClos << PZ_ZBNUM << Qt::AscendingOrder; //按凭证自编号的升序
//    curPzModel->setSort(sortClos);
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    sortBy = false;
    if(pzEdit){
        ui->actSortByPz->setEnabled(true);
        ui->actSortByZb->setEnabled(false);
        int curId = pzEdit->getCurPzId();
        pzEdit->save();
        pzEdit->closeDlg();
        subWindows.value(PZEDIT)->close();
        editPzs();
        pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
        pzEdit->naviTo(curId);
    }

}

//按凭证号对凭证集中的凭证进行排序
void MainWindow::on_actSortByPz_triggered()
{
//    QList<int> sortClos; //模型的排序列
//    sortClos << PZ_NUMBER << Qt::AscendingOrder; //按凭证总号的升序
//    curPzModel->setSort(sortClos);
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    sortBy = true;
    if(pzEdit){
        ui->actSortByPz->setEnabled(false);
        ui->actSortByZb->setEnabled(true);
        int curId = pzEdit->getCurPzId();
        pzEdit->save();
        pzEdit->closeDlg();
        subWindows.value(PZEDIT)->close();
        editPzs();
        pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
        pzEdit->naviTo(curId);
    }

}

//登录
void MainWindow::on_actLogin_triggered()
{
    LoginDialog* dlg = new LoginDialog;
    if(dlg->exec() == QDialog::Accepted){
        curUser = dlg->getLoginUser();
        lblCurUser->setText(curUser->getName());
        //enaActOnLogin(true);
        //ui->actLogin->setEnabled(false);
        //ui->actLogout->setEnabled(true);
        //ui->actShiftUser->setEnabled(true);
    }
    refreshTbrVisble();
    refreshActEnanble();
}

//登出
void MainWindow::on_actLogout_triggered()
{
    curUser = NULL;
    lblCurUser->setText(tr("未登录"));
    //enaActOnLogin(false);
    refreshTbrVisble();
    refreshActEnanble();
}


//切换用户
void MainWindow::on_actShiftUser_triggered()
{
    LoginDialog* dlg = new LoginDialog;
    if(dlg->exec() == QDialog::Accepted){
        curUser = dlg->getLoginUser();
        lblCurUser->setText(curUser->getName());
    }
    refreshTbrVisble();
    refreshActEnanble();
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
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->moveUp();
}

//向下移动业务活动
void MainWindow::on_actMvDownAction_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    pzEdit->moveDown();
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
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    int num = spnNaviTo->value();
    if(!pzEdit->naviTo2(num))
        QMessageBox::question(this,tr("错误提示"),
                              tr("没有凭证号为%1的凭证").arg(num));

}

//全部手工凭证通过审核
void MainWindow::on_actAllVerify_triggered()
{
    if(!curUser->haveRight(allRights.value(Right::VerifyPz))){
        rightWarnning(Right::VerifyPz);
        return;
    }
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }
    int pzState, rows = 0, num = 0;
    QProgressBar pb;
    ui->statusbar->addWidget(&pb);
    pb.setMinimum(0);
    pb.setMaximum(curPzModel->rowCount());
    pb.setValue(num);
    for(int i = 0; i < curPzModel->rowCount(); i++){
        pzState = curPzModel->data(curPzModel->index(i,PZ_PZSTATE)).toInt();
        if(pzState == Pzs_Recording){
            curPzModel->setData(curPzModel->index(i,PZ_PZSTATE),Pzs_Verify);
            curPzModel->setData(curPzModel->index(i,PZ_VUSER),curUser->getUserId());
            rows++;
        }
        pb.setValue(++num);
    }
    curPzModel->submitAll();
    //BusiUtil::refreshPzsState(cursy,cursm,curPzModel);
    curPzSetState = Ps_HandV;
    BusiUtil::setPzsState(cursy,cursm,Ps_HandV);
    refreshShowPzsState();
    showPzNumsAffected(rows);
}

//全部手工凭证入账
void MainWindow::on_actAllInstat_triggered()
{
    if(!curUser->haveRight(allRights.value(Right::InstatPz))){
        rightWarnning(Right::InstatPz);
        return;
    }
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }
    int pzState, rows = 0, num = 0;
    QProgressBar pb;
    ui->statusbar->addWidget(&pb);
    pb.setMinimum(0);
    pb.setMaximum(curPzModel->rowCount());
    pb.setValue(num);
    for(int i = 0; i < curPzModel->rowCount(); i++){
        pzState = curPzModel->data(curPzModel->index(i,PZ_PZSTATE)).toInt();
        if(pzState == Pzs_Verify){
            curPzModel->setData(curPzModel->index(i,PZ_PZSTATE),Pzs_Instat);
            curPzModel->setData(curPzModel->index(i,PZ_BUSER),curUser->getUserId());
            rows++;
        }
        pb.setValue(num);
    }
    curPzModel->submitAll();    
    //入账操作对凭证集的状态没有影响
    //BusiUtil::refreshPzsState(cursy,cursm,curPzModel);
    //refreshShowPzsState();
    showPzNumsAffected(rows);

}

//取消已审核或已入账的凭证，使凭证回到初始状态
void MainWindow::on_actAntiVerify_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    if(pzEdit){
        if((pzEdit->getPzState() == Pzs_Verify)
                || (pzEdit->getPzState() == Pzs_Instat)){
            pzEdit->setPzState(Pzs_Recording, curUser);
            curPzState = Pzs_Recording;
            pzStateChange(Pzs_Recording);
            refreshAdvancPzOperBtn();
            //ui->actSavePz->setEnabled(true);
        }
    }
}

//重新分派凭证号
void MainWindow::on_actReassignPzNum_triggered()
{
    PzDialog2* pzEdit = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
    if(pzEdit){
        pzEdit->reAssignPzNum();
    }
}

//引入其他模块自动产生的凭证（比如固定资产折旧、待摊费用等）
void MainWindow::on_actImpOtherPz_triggered()
{
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }

    QSet<OtherModCode> mods;
    ImpOthModDialog* dlg = new ImpOthModDialog(1,this);

    //如果用户选择取消，则什么也不做
    if(QDialog::Rejected == dlg->exec()){
        delete dlg;
        return;
    }

    //用户选择跳过
    if(dlg->isSelSkip()){
        bool req;
        BusiUtil::reqGenJzHdsyPz(cursy,cursm,req);
        if(req)
            BusiUtil::setPzsState(cursy, cursm, Ps_Stat2);
        else  //如果同时也不需要结转汇兑损益，则直接准备结转损益
            BusiUtil::setPzsState(cursy, cursm, Ps_JzsyPre);
        delete dlg;
        return;
    }

    //用户选择确定
    QSet<OtherModCode> selMods;
    dlg->selModules(selMods);
    if(!BusiUtil::impPzFromOther(cursy,cursm,selMods))
        showTemInfo(tr("在引入由其他模块创建的凭证时出错！"));
    else if(selMods.empty())
        showTemInfo(tr("你未选择模块，本次操作什么也不做！"));
    else{
        showTemInfo(tr("成功引入由其他模块创建的凭证！"));
        refreshShowPzsState();
        refreshActEnanble();
    }

    delete dlg;
}

//结转本年利润
void MainWindow::on_actJzbnlr_triggered()
{
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }
    if(curPzSetState == Ps_Jzed){
        QMessageBox::information(this, tr("提示信息"), tr("已结账，不能添加任何凭证"));
        return;
    }
    //创建一个结转本年利润到利润分配的特种类别凭证，允许用户编辑此凭证
    PzData d;
    QDate date(cursy,cursm,1);
    date.setDate(cursy,cursm,date.daysInMonth());
    d.date = date.toString(Qt::ISODate);
    d.attNums = 0;
    d.pzNum = curPzModel->rowCount()+1;
    d.pzZbNum = curPzModel->rowCount()+1;
    d.state = Pzs_Recording;
    d.pzClass = Pzc_Jzlr;
    d.jsum = 0; d.dsum = 0;
    d.producer = curUser;
    d.verify = NULL;
    d.bookKeeper = NULL;

    if(BusiUtil::genJzbnlr(cursy,cursm,d)){
        showTemInfo(tr("成功创建结转本年利润凭证！"));
        refreshShowPzsState();
        refreshActEnanble();
        //如果凭证编辑窗口已打开，则定位到刚创建的凭证
        if(subWindows.contains(PZEDIT)){
            curPzModel->select();
            PzDialog2* dlg = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
            dlg->naviLast();
        }
    }
    else
        showTemInfo(tr("在创建结转本年利润凭证时出错！"));
}

//结账
void MainWindow::on_actEndAcc_triggered()
{
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }
    if(curPzSetState == Ps_Jzed){
        QMessageBox::information(this, tr("提示信息"), tr("已经结账"));
        return;
    }
    if(QMessageBox::Yes == QMessageBox::information(this,tr("提示信息"),
                                                    tr("结账后，将不能再次对凭证集进行修改，确认要结账吗？"),
                                                    QMessageBox::Yes | QMessageBox::No)){
        BusiUtil::setPzsState(cursy,cursm,Ps_Jzed);
        showTemInfo(tr("当前凭证集已结账！"));
        //要刷新界面，以反映凭证集状态的变化        
        refreshShowPzsState();
        refreshActEnanble();
        //refreshTbrVisble();
        //refreshAdvancPzOperBtn();
        return;
    }
}

//反结转
void MainWindow::on_actAntiJz_triggered()
{
//    //提供3种结转凭证类型（按优先级从高到低依次为结转利润、结转损益、结转汇兑损益）
//    //的取消功能，由用户来选择。

//    //功能需求说明：
//    //1：应允许用户在任意状态点（在执行了任意结转操作后的状态点）执行反结转功能
//    //这个可以通过控制此菜单项的启用性来完成。

//    //1：可选性限制，即是否可选择，还要取决于当前凭证集的状态。
//    //比如当前凭证集状态是结转损益（Ps_Stat4），则只有结转损益、引入和结转汇兑是可以选的，结转利润不能选

//    //2：自动选择限制
//    //如果已经执行了结转损益操作，选择取消引入时，还必须自动选中取消结转汇兑损益和取消结转损益，并使其
//    //不可更改（即对应的复选框不可用）

//    //在反结转后执行的操作，应能够自动地处理后续动作

//    bool req;
//    QHash<PzdClass,bool> reqs;
//    BusiUtil::reqGenJzHdsyPz(cursy,cursm,req);
//    reqs[Pzd_Jzhd] = !req;
//    BusiUtil::reqForwordPl(cursy,cursm,req);
//    reqs[Pzd_Jzsy] = !req;
//    BusiUtil::reqGenJzbnlr(cursy,cursm,req);
//    reqs[Pzd_Jzlr] = !req;
//    PzsState state;
//    AntiJzDialog dlg(reqs,state,this);
//    if(QDialog::Accepted == dlg.exec()){
//        QHash<PzdClass,bool> sels;
//        if(dlg.selected(sels)){
//            if(sels.value(Pzd_Jzlr))
//                BusiUtil::antiJzbnlr(cursy,cursm);
//            if(sels.value(Pzd_Jzsy))
//                BusiUtil::antiForwordPl(cursy,cursm);
//            if(sels.value(Pzd_Jzhd))
//                BusiUtil::antiJzhdsyPz(cursy,cursm);
//        }

//    }
}

//反结账
void MainWindow::on_actAntiEndAcc_triggered()
{
    //打破结账限制，
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }
    if(curPzSetState != Ps_Jzed){
        QMessageBox::information(this, tr("提示信息"), tr("还未结账"));
        return;
    }
    BusiUtil::setPzsState(cursy,cursm,/*Ps_Stat5*/Ps_Rec);
    allPzToRecording(cursy,cursm);
    curPzSetState = Ps_Rec;
    refreshShowPzsState();
    refreshActEnanble();
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
    SqlToolDialog* dlg = new SqlToolDialog;
    dlg->setWindowTitle(tr("SQL工具窗"));
    ui->mdiArea->addSubWindow(dlg);
    dlg->show();
}

//显示基础数据库访问窗口
void MainWindow::on_actBasicDB_triggered()
{
    showSubWindow(BASEDATAEDIT);
}

//固定资产管理
void MainWindow::on_actGdzcAdmin_triggered()
{
    QByteArray* sinfo;
    SubWindowDim* winfo;
    VariousUtils::getSubWinInfo(GDZCADMIN,winfo,sinfo);
    GdzcAdminDialog* dlg = new GdzcAdminDialog(sinfo);
    showSubWindow(GDZCADMIN,winfo,dlg);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

//待摊费用管理
void MainWindow::on_actDtfyAdmin_triggered()
{
    QByteArray* sinfo;
    SubWindowDim* winfo;
    VariousUtils::getSubWinInfo(DTFYADMIN,winfo,sinfo);
    DtfyAdminDialog* dlg = new DtfyAdminDialog(sinfo,adb);
    showSubWindow(DTFYADMIN,winfo,dlg);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

//查看总账
void MainWindow::on_actShowTotal_triggered()
{
    QByteArray* sinfo;
    SubWindowDim* winfo;
    VariousUtils::getSubWinInfo(TOTALVIEW,winfo,sinfo);
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
        VariousUtils::getSubWinInfo(DETAILSVIEW,winfo,sinfo);
        ShowDZDialog* dlg = new ShowDZDialog(sinfo);
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
        case PZEDIT:
        case PZSTAT:
            dlg = w;
            break;
        case TOTALVIEW:
            dlg = w;
            break;
        case DETAILSVIEW:
            dlg = w;
            //dzd = qobject_cast<ShowDZDialog*>(dlg);
            //connect(dzd,SIGNAL(closeWidget()) ,subWindows[winType],SLOT(close()));
            break;
        case SETUPBANK:
            dlg = new SetupBankDialog(false, this);
            break;
        case SETUPBASE:
            dlg = new SetupBaseDialog2(curAccount);
            break;
        case BASEDATAEDIT:
            dlg = new BaseDataEditDialog;
            break;
        case GDZCADMIN:
            dlg = w;
            break;
        case DTFYADMIN:
            dlg = w;
            break;
        case HISTORYVIEW:
            dlg = w;
            break;
        case LOOKUPSUBEXTRA:
            dlg = w;
            break;
        case ACCOUNTPROPERTY:
            dlg = w;
            break;
        }

        connect(dlg,SIGNAL(accepted()) ,subWindows[winType],SLOT(close()));
        connect(dlg,SIGNAL(rejected()) ,subWindows[winType],SLOT(close()));
        subWindows[winType]->setWidget(dlg);
        ui->mdiArea->addSubWindow(subWindows[winType]);
        connect(subWindows[winType], SIGNAL(windowClosed(QMdiSubWindow*)),
                this, SLOT(subWindowClosed(QMdiSubWindow*)));

        //对于凭证编辑窗口，因为它重载了基类的show函数，因此必须显示调用子类的方法。
        if(winType == PZEDIT){
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
        subWinActStates[winType] = true; //设置窗口的激活状态

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

//将指定的凭证集内的所有凭证入账
bool MainWindow::allInstat(int y, int m)
{
    QSqlQuery q;
    QString s;

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    s = QString("update PingZhengs set pzState=%1 where pzState != %2"
                " and date like '%3%'")
            .arg(Pzs_Instat).arg(Pzs_Repeal).arg(ds);
    if(!q.exec(s))
        return false;
    BusiUtil::setPzsState(y,m,Ps_JzhdV);
    return true;
}

//审核结转损益类科目的凭证
bool MainWindow::instatJzsyPz(int y, int m)
{
    QSqlQuery q;
    QString s;

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    s = QString("update PingZhengs set pzState=%1 where (isForward = %2 "
                "or isForward = %3) and date like '%4%'")
            .arg(Pzs_Instat).arg(Pzc_JzsyIn).arg(Pzc_JzsyFei).arg(ds);
    if(!q.exec(s))
        return false;
    return true;
}

bool MainWindow::jzsy()
{
    //结转损益科目到本年利润
        bool r;
        QDialog dlg;
        QDateEdit* date = new QDateEdit(QDate::currentDate());
        date->setDisplayFormat("yyyy-M");
        QLabel* lab = new QLabel(tr("请选择要结转损益的年月："));
        QHBoxLayout* l1 = new QHBoxLayout;
        l1->addWidget(lab);
        l1->addWidget(date);
        QPushButton* btnOk = new QPushButton(tr("确定"));
        QPushButton* btnCancel = new QPushButton(tr("取消"));
        QHBoxLayout* l2 = new QHBoxLayout;
        l2->addWidget(btnOk);
        l2->addWidget(btnCancel);
        QVBoxLayout* l = new QVBoxLayout;
        l->addLayout(l1);
        l->addLayout(l2);
        dlg.setLayout(l);
        connect(btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
        connect(btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));


        if(dlg.exec() == QDialog::Rejected)
            return false;

        int y = date->date().year();
        int m = date->date().month();
        //删除先前存在的结转凭证，这是因为要取得结转前的正确余额
        BusiUtil::delSpecPz(y,m,Pzc_JzsyIn);
        BusiUtil::delSpecPz(y,m,Pzc_JzsyFei);
        BusiUtil::setPzsState2(y,m,Ps_JzhdV);
        ViewExtraDialog* vdlg = new ViewExtraDialog(y,m);//统计本期发生额并保存余额
        if(vdlg->exec() == QDialog::Rejected)
            return false;
        delete vdlg;
        BusiUtil::setPzsState(y,m,Ps_JzsyPre);
        BusiUtil::genForwordPl2(y,m,curUser); //创建结转凭证
        instatJzsyPz(y,m);  //将结转凭证入账
        BusiUtil::setPzsState(y,m,Ps_JzsyV);
        vdlg = new ViewExtraDialog(y,m); //再次统计本期发生额并保存余额
        if(vdlg->exec() == QDialog::Rejected)
            return false;
        delete vdlg;
        //r = BusiUtil::setPzsState(y,m,Ps_Jzed);
        QMessageBox::information(0,tr("提示信息"),tr("成功创建结转损益凭证！"));
}

//在执行了结转汇兑损益后，执行的结转损益类科目到本年利润
bool MainWindow::jzsy2()
{
    if(QMessageBox::Yes != QMessageBox::warning(this,"warning",
                        tr("执行本操作前，要确保已经执行了结转汇兑损益操作，并统计后保存余额。"
                           "继续：点击是，取消：点击否 ！！"),
                         QMessageBox::Yes|QMessageBox::No,
                         QMessageBox::Yes))
        return true;

    //结转损益科目到本年利润
    bool r;
    QDialog dlg;
    QDateEdit* date = new QDateEdit(QDate::currentDate());
    date->setDisplayFormat("yyyy-M");
    QLabel* lab = new QLabel(tr("请选择要结转损益的年月："));
    QHBoxLayout* l1 = new QHBoxLayout;
    l1->addWidget(lab);
    l1->addWidget(date);
    QPushButton* btnOk = new QPushButton(tr("确定"));
    QPushButton* btnCancel = new QPushButton(tr("取消"));
    QHBoxLayout* l2 = new QHBoxLayout;
    l2->addWidget(btnOk);
    l2->addWidget(btnCancel);
    QVBoxLayout* l = new QVBoxLayout;
    l->addLayout(l1);
    l->addLayout(l2);
    dlg.setLayout(l);
    connect(btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));


    if(dlg.exec() == QDialog::Rejected)
        return false;

    int y = date->date().year();
    int m = date->date().month();

    //删除先前存在的结转凭证，这是因为要取得结转前的正确余额
    BusiUtil::delSpecPz(y,m,Pzc_JzsyIn);
    BusiUtil::delSpecPz(y,m,Pzc_JzsyFei);
    r = allInstat(y,m);

    BusiUtil::setPzsState(y,m,Ps_JzsyPre);
    BusiUtil::genForwordPl2(y,m,curUser); //创建结转凭证
    instatJzsyPz(y,m);  //将结转凭证入账
    BusiUtil::setPzsState(y,m,Ps_JzsyV);
    ViewExtraDialog* vdlg = new ViewExtraDialog(y,m); //再次统计本期发生额并保存余额
    if(vdlg->exec() == QDialog::Rejected)
        return false;
    delete vdlg;
    //r = BusiUtil::setPzsState(y,m,Ps_Jzed);
    QMessageBox::information(0,tr("提示信息"),tr("成功创建结转损益凭证！"));
}


//结转汇兑损益
bool MainWindow::jzhdsy()
{
    //结转汇兑损益到财务费用
        QDialog dlg;
        QDateEdit* date = new QDateEdit(QDate::currentDate());
        date->setDisplayFormat("yyyy-M");
        QLabel* lab = new QLabel(tr("请选择要结转汇兑损益的年月："));
        QHBoxLayout* l1 = new QHBoxLayout;
        l1->addWidget(lab);
        l1->addWidget(date);
        QPushButton* btnOk = new QPushButton(tr("确定"));
        QPushButton* btnCancel = new QPushButton(tr("取消"));
        QHBoxLayout* l2 = new QHBoxLayout;
        l2->addWidget(btnOk);
        l2->addWidget(btnCancel);
        QVBoxLayout* l = new QVBoxLayout;
        l->addLayout(l1);
        l->addLayout(l2);
        dlg.setLayout(l);
        connect(btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
        connect(btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));


        if(dlg.exec() == QDialog::Rejected)
            return false;

        int y = date->date().year();
        int m = date->date().month();

        //因为汇兑损益的结转要涉及到期末汇率，即下期的汇率，要求用户确认汇率是否正确
        QHash<int,Double> rates;
        int yy,mm;
        if(m == 12){
            yy = y+1;
            mm = 1;
        }
        else{
            yy = y;
            mm = m+1;
        }
        BusiUtil::getRates2(yy,mm,rates);
        QString tip = tr("%1年%2月美金汇率：").arg(yy).arg(mm);
        bool ok;
        double rate = QInputDialog::getDouble(0,tr("信息输入"),tip,rates.value(USD).getv(),0,100,2,&ok);
        if(!ok)
            return false;
        rates[USD] = Double(rate);
        BusiUtil::saveRates2(yy,mm,rates);

        BusiUtil::delSpecPz(y,m,Pzc_Jzhd_Bank);
        BusiUtil::delSpecPz(y,m,Pzc_Jzhd_Ys);
        BusiUtil::delSpecPz(y,m,Pzc_Jzhd_Yf);
        BusiUtil::setPzsState2(y,m,Ps_HandV);

        ViewExtraDialog* vdlg = new ViewExtraDialog(y,m);//统计本期发生额并保存余额
        if(vdlg->exec() == QDialog::Rejected)
            return false;
        delete vdlg;

        BusiUtil::setPzsState(y,m,Ps_Stat1);
        BusiUtil::genForwordEx2(y,m,curUser);
        BusiUtil::setPzsState2(y,m,Ps_JzhdV);

        vdlg = new ViewExtraDialog(y,m);     //统计本期发生额并保存余额
        if(vdlg->exec() == QDialog::Rejected)
            return false;
        delete vdlg;

        showTemInfo(tr("成功创建结转汇兑损益凭证！"));
        QMessageBox::information(0,tr("提示信息"),tr("成功创建结转汇兑损益凭证！"));
}


//显示账户属性对话框
void MainWindow::on_actAccProperty_triggered()
{
    QByteArray* sinfo;
    SubWindowDim* winfo;
    VariousUtils::getSubWinInfo(ACCOUNTPROPERTY,winfo,sinfo);
    AccountPropertyDialog* dlg = new AccountPropertyDialog(curAccount);
    showSubWindow(ACCOUNTPROPERTY,winfo,dlg);
    if(sinfo)
        delete sinfo;
    if(winfo)
        delete winfo;
}

/**
 * @brief MainWindow::on_aactTemTask_triggered
 *  结转损益至本年利润
 */
void MainWindow::on_aactTemTask_triggered()
{
    jzsy();
}

/**
 * @brief MainWindow::on_actJzHdsy_triggered
 *  结转汇兑损益
 */
void MainWindow::on_actJzHdsy_triggered()
{
    jzhdsy();
}

/**
 * @brief MainWindow::on_actJzsyAfterJzhd_triggered
 *  结转汇兑后结转损益至本年利润
 */
void MainWindow::on_actJzsyAfterJzhd_triggered()
{
    jzsy2();
}

/**
 * @brief MainWindow::on_actForcePzsState_triggered
 *  强制设置凭证集状态
 */
void MainWindow::on_actForcePzsState_triggered()
{
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }
    QDialog* dlg = new QDialog;
    QLabel* lb1 = new QLabel(tr("当前凭证集状态："),dlg);
    QLabel* lbCur = new QLabel(pzsStates.value(curPzSetState),dlg);
    QLabel* lb2 = new QLabel(tr("目标凭证集状态："));
    QComboBox* cmb = new QComboBox(dlg);
    cmb->addItem(pzsStates.value(Ps_Rec),Ps_Rec);
    cmb->addItem(pzsStates.value(Ps_HandV),Ps_HandV);
    cmb->addItem(pzsStates.value(Ps_Jzhd),Ps_Jzhd);
    cmb->addItem(pzsStates.value(Ps_JzhdV),Ps_JzhdV);
    cmb->addItem(pzsStates.value(Ps_Jzsy),Ps_Jzsy);
    cmb->addItem(pzsStates.value(Ps_JzsyV),Ps_JzsyV);
    cmb->addItem(pzsStates.value(Ps_Jzbnlr),Ps_Jzbnlr);
    cmb->addItem(pzsStates.value(Ps_JzbnlrV),Ps_JzbnlrV);
    cmb->addItem(pzsStates.value(Ps_Jzed),Ps_Jzed);

    int index = cmb->findData(curPzSetState);
    cmb->setCurrentIndex(index);
    QVBoxLayout* lm = new QVBoxLayout;
    QGridLayout* l1 = new QGridLayout;
    QHBoxLayout* l2 = new QHBoxLayout;
    l1->addWidget(lb1,0,0);
    l1->addWidget(lbCur,0,1);
    l1->addWidget(lb2,1,0);
    l1->addWidget(cmb,1,1);
    QPushButton* btnOk = new QPushButton(tr("确定"));
    QPushButton* btnCancel = new QPushButton(tr("取消"));
    connect(btnOk, SIGNAL(clicked()), dlg, SLOT(accept()));
    connect(btnCancel, SIGNAL(clicked()), dlg, SLOT(reject()));
    l2->addWidget(btnOk);
    l2->addWidget(btnCancel);
    lm->addLayout(l1);
    lm->addLayout(l2);
    dlg->setLayout(lm);
    dlg->resize(300,200);
    if(dlg->exec() == QDialog::Rejected)
        return;
    PzsState ps = (PzsState)cmb->itemData(cmb->currentIndex()).toInt();
    BusiUtil::setPzsState2(cursy,cursm,ps);
    refreshShowPzsState();
}

/**
 * @brief MainWindow::on_actSetPzCls_triggered
 *  设置凭证类别
 */
void MainWindow::on_actSetPzCls_triggered()
{
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }
    if(!subWindows.contains(PZEDIT))
        return;

    QSqlQuery q;
    PzDialog2* pd = static_cast<PzDialog2*>(subWindows.value(PZEDIT)->widget());
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
    if(!isOpenPzSet){
        pzsWarning();
        return;
    }

    ViewPzSetErrorForm* w;
    if(subWindows.contains(VIEWPZSETERROR)){
        ui->mdiArea->setActiveSubWindow(subWindows.value(VIEWPZSETERROR));
        w = static_cast<ViewPzSetErrorForm*>(subWindows.value(VIEWPZSETERROR)->widget());
    }
    else{
        w = new ViewPzSetErrorForm(cursy,cursm);
        MyMdiSubWindow* sw = new MyMdiSubWindow;
        sw->setWidget(w);
        ui->mdiArea->addSubWindow(sw);
        subWindows[VIEWPZSETERROR] = sw;
        connect(sw,SIGNAL(windowClosed(QMdiSubWindow*)),
                this,SLOT(subWindowClosed(QMdiSubWindow*)));
        connect(w,SIGNAL(reqLoation(int,int)),this,SLOT(openSpecPz(int,int)));
    }
    subWindows.value(VIEWPZSETERROR)->resize(600,400);
    subWindows.value(VIEWPZSETERROR)->show();
}

//强制删除锁定凭证（批量删除，凭证号之间用逗号隔开，连续的凭证号可以用连字符“-”连接，比如“2-4，7，8”）
//实现中，在删除后未考虑对凭证号进行重置。
void MainWindow::on_actForceDelPz_triggered()
{
    QDialog* dlg = new QDialog(this);
    QLabel* ly = new QLabel(tr("凭证所属年月"),dlg);
    QDateEdit* date = new QDateEdit(dlg);
    date->setDate(QDate(2011,1,1));
    date->setDisplayFormat("yyyy-M");
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
