#include <QDebug>
#include <QTimer>
#include <QSystemTrayIcon>

#include "mainapplication.h"
#include "myhelper.h"
#include "version.h"

//////////////////////////LoginDialog///////////////////////////////////////
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    init();
}

void LoginDialog::init()
{
    QHashIterator<int,User*> it(allUsers);
    int ruIndex = 0, index = 0;
    int recentUserId;
    AppConfig::getInstance()->getCfgVar(AppConfig::CVC_ResentLoginUser,recentUserId);
    while(it.hasNext()){
        it.next();
        if(!it.value()->isEnabled())
            continue;
        if(it.value()->getUserId() == recentUserId)
            ruIndex = index;
        ui->cmbUsers->addItem(it.value()->getName(),it.value()->getUserId());
        index++;
    }
    ui->cmbUsers->setCurrentIndex(ruIndex);
    ui->edtPw->setFocus();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

User* LoginDialog::getLoginUser()
{
    int userId = ui->cmbUsers->itemData(ui->cmbUsers->currentIndex()).toInt();
    return allUsers.value(userId);
}

//登录
void LoginDialog::on_btnLogin_clicked()
{

    int userId =ui->cmbUsers->itemData(ui->cmbUsers->currentIndex()).toInt();
    User* u = allUsers.value(userId);
    if(!u){
        myHelper::ShowMessageBoxWarning(tr("请以确定的用户身份登录系统！"));
        return;
    }
    if(u->verifyPw(ui->edtPw->text())){
        curUser = u;
        accept();
    }
    else
        myHelper::ShowMessageBoxWarning(tr("密码不正确"));
}

//取消登录
void LoginDialog::on_btnCancel_clicked()
{
    QDialog::reject();
}

/////////////////MainApplication/////////////////////////////////////////
MainApplication::MainApplication(int &argc, char** argv): QApplication(argc, argv),
    _appCfg(0),_isClosing(false),mainWindow_(0)
{
    appLock = new PaAppLock(this);
    if(appLock->existInstance()){
        _isClosing = true;
        return;
    }

    _appCfg = AppConfig::getInstance();
    QString style = _appCfg->getAppStyleName();
    if(style.isEmpty()){
        style = "navy";
        _appCfg->setAppStyleName(style);
    }
    myHelper::SetStyle(style);
    myHelper::SetChinese();
    if(!initSecurity()){
        _isClosing = true;
        showErrorInfo(AE_SECURIT_MODULE_INIT);
        return;
    }
    int recentUserId;
    _appCfg->getCfgVar(AppConfig::CVC_ResentLoginUser,recentUserId);
    LoginDialog dlg;
    if(dlg.exec() == QDialog::Rejected){
        //_appCfg->exit();
        _isClosing = true;
        return;
    }
    recentUserId = curUser->getUserId();
    _appCfg->setCfgVar(AppConfig::CVC_ResentLoginUser,recentUserId);
    showSplashScreen();
    myHelper::SetUTF8Code();
    AppErrorCode result = init();
    if(result != AE_OK){
        showErrorInfo(result,_initErrorDetails);
        _isClosing = true;
        return;
    }
    setProgressSplashScreen(30);
    qWarning() << "Run application 2";
    mainWindow_ = new MainWindow();
    qWarning() << "Run application 3";
    setProgressSplashScreen(60);
    //loadSettings();
    qWarning() << "Run application 4";
    //updateFeeds_ = new UpdateFeeds(mainWindow_);
    setProgressSplashScreen(90);
    qWarning() << "Run application 5";
    //mainWindow_->restoreFeedsOnStartUp();
    setProgressSplashScreen(100);
    qWarning() << "Run application 6";
//    if(!mainWindow_->showTrayIcon_){
    mainWindow_->showMaximized();
//    }
    mainWindow_->isMinimizeToTray_ = false;
    closeSplashScreen();

//    if (mainWindow_->showTrayIcon_) {
//        QTimer::singleShot(0, mainWindow_->traySystem, SLOT(show()));
//    }
    mainWindow_->_traySystem->show();
}

MainApplication::~MainApplication()
{
    quitApplication();
}

bool MainApplication::isClosing() const
{
    return _isClosing;
}

void MainApplication::showErrorInfo(AppErrorCode errCode, QString details)
{
    QString info;
    switch(errCode){
    case AE_OK:
        return;
    case AE_READ_SETTINGS_FAILED:
        info = QObject::tr("读取应用配置时出错！");
        break;
    case AE_SETTING_NOT_UPGRADE:
        info = QObject::tr("要正常运行当前版本程序，必须更新配置，或当前版本无法确定，或当前程序的配置版本低于当前版本");
        break;
    case AE_BDATA_CONN:
        info = QObject::tr("基础库连接错误！");
        break;
    case AE_SECURIT_MODULE_INIT:
        info = QObject::tr("安全模块初始化错误！");
        break;
    case AE_GLOBAL_INIT:
        info = QObject::tr("全局变量初始化错误！");
        break;
    }
    if(errCode != AE_OK){
        QString err = tr("应用初始化时遇到错误：\n%1").arg(info);
        if(!details.isEmpty())
            err += QString("\n%1").arg(details);
        myHelper::ShowMessageBoxError(err);
    }
}

void MainApplication::quitApplication()
{
    //qWarning() << "quitApplication 1";
    if(mainWindow_)
        delete mainWindow_;
    //qWarning() << "quitApplication 2";
    _appCfg->saveGlobalVar();
    _appCfg->exit();
}

MainApplication::AppErrorCode MainApplication::init()
{
    appTitle = QObject::tr("凭证辅助处理系统");
    setApplicationName(appName);
    setOrganizationName("SSC");
    setApplicationVersion(versionStr);
    setWindowIcon(QIcon(":/images/appIcon.ico"));
    setQuitOnLastWindowClosed(false);

    //初始化路径信息
    LOGS_PATH = QDir::toNativeSeparators(QDir::currentPath().append("/logs/"));
    PATCHES_PATH = QDir::toNativeSeparators(QDir::currentPath().append("/patches/"));
    DATABASE_PATH = QDir::toNativeSeparators(QDir::currentPath().append("/datas/databases/"));
    BASEDATA_PATH = QDir::toNativeSeparators(QDir::currentPath().append("/datas/basicdatas/"));
    BACKUP_PATH = QDir::toNativeSeparators(QDir::currentPath().append("/datas/backups/"));
    VersionManager vm(VersionManager::MT_CONF);
    VersionUpgradeInspectResult result = vm.isMustUpgrade();
    bool exec = false;
    switch(result){
    case VUIR_CANT:
        return AE_SETTING_NOT_UPGRADE;
    case VUIR_DONT:
        exec = false;
        break;
    case VUIR_LOW:
        return AE_SETTING_NOT_UPGRADE;
    case VUIR_MUST:
        exec = true;
        break;
    }
    if(exec){
        if(vm.exec() == QDialog::Rejected){
            return AE_SETTING_NOT_UPGRADE;
        }
        else if(!vm.getUpgradeResult())
            return AE_SETTING_UPGRADE_FAILED;
    }

    if(!_appCfg)
        return AE_BDATA_CONN;

    //设置应用程序的版本号
    int master = 1;
    int second = 0;



    //获取可用屏幕尺寸
    QDesktopWidget desktop;
    screenWidth = desktop.availableGeometry().width();
    screenHeight = desktop.availableGeometry().height();

    bdb = _appCfg->getBaseDbConnect();
    if(!_appCfg->readPzSetStates(pzsStates,pzsStateDescs))
        _initErrorDetails += tr("读取凭证集状态名时");
    if(!_appCfg->readPingzhenClass(pzClasses))
        _initErrorDetails += tr("读取凭证类别名时");
    if(!_appCfg->readPzStates(pzStates))
        _initErrorDetails += tr("读取凭证状态名时");
    if(!_appCfg->readAllGdzcClasses(allGdzcProductCls))
        _initErrorDetails += tr("\n读取固定资财类别时");
    if(!_initErrorDetails.isEmpty())
        return AE_GLOBAL_INIT;

    pzClsImps.insert(Pzc_GdzcZj);
    pzClsImps.insert(Pzc_DtfyTx);

    pzClsJzhds.insert(Pzc_Jzhd_Bank);
    pzClsJzhds.insert(Pzc_Jzhd_Ys);
    pzClsJzhds.insert(Pzc_Jzhd_Yf);
    pzClsJzhds.insert(Pzc_Jzhd_Yuf );
    pzClsJzhds.insert(Pzc_Jzhd_Yus);
    pzClsJzhds.insert(Pzc_Jzhd);

    pzClsJzsys.insert(Pzc_JzsyIn);
    pzClsJzsys.insert(Pzc_JzsyFei);
    return AE_OK;
}

MainApplication *MainApplication::getInstance()
{
    return static_cast<MainApplication*>(QCoreApplication::instance());
}

/**
 * @brief 返回应用所使用的设置文件名
 * @return
 */
QString MainApplication::settingFile()
{
    return _appCfg->getSettingFileName();
}

void MainApplication::showSplashScreen()
{
    if(showSplashScreen_){
        splashScreen_ = new SplashScreen(QPixmap(":images/accProperty/suiteInfo.png"));
        splashScreen_->show();
        processEvents();
    }
}

void MainApplication::closeSplashScreen()
{
    if(showSplashScreen_){
        splashScreen_->finish(mainWindow_);
        splashScreen_->deleteLater();
    }
}

void MainApplication::setProgressSplashScreen(int value)
{
    if(showSplashScreen_)
        splashScreen_->setProgress(value);
}

void MainApplication::setStyleApplication()
{
    AppConfig* cfg = AppConfig::getInstance();
    QString style = cfg->getAppStyleName();
    if(style.isEmpty()){
        style = "navy";
        cfg->setAppStyleName(style);
    }
    myHelper::SetStyle(style);
    myHelper::SetChinese();
}
