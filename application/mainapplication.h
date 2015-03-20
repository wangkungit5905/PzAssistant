#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#include "ui_logindialog.h"
#include "splashscreen.h"
#include "mainwindow.h"
#include "paapplock.h"

/**
 * @brief 登录对话框类
 */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();
    User* getLoginUser();

private slots:
    void on_btnLogin_clicked();

    void on_btnCancel_clicked();

private:
    void init();

    Ui::LoginDialog *ui;
};

class MainApplication : public QApplication
{
    Q_OBJECT
public:
    enum AppErrorCode{
        AE_OK =                     0,  //正常退出
        AE_READ_SETTINGS_FAILED =   1,  //读取应用配置出错
        AE_SETTING_NOT_UPGRADE =    2,  //应用配置必须升级
        AE_SETTING_UPGRADE_FAILED = 3,  //应用配置升级失败
        AE_BDATA_CONN =             4,  //基础库连接错误
        //AE_BDATA_NOT_UPGRADE =      4,  //用户取消了基本库版本的升级
        //AE_BDATA_UPGRADE_FAILED =   5,  //基本库版本升级失败
        AE_SECURIT_MODULE_INIT =    5,  //安全模块初始化错误
        AE_GLOBAL_INIT =            6   //全局变量初始化错误
        //AE_OK = 8,读取凭证类型出错
    };

    explicit MainApplication(int &argc, char** argv);
    ~MainApplication();    
    bool isClosing() const;
    void showErrorInfo(AppErrorCode errCode, QString details=QString());

    static MainApplication *getInstance();

public slots:
  void quitApplication();

private:
    AppErrorCode init();
    void loadSettings();
    void showSplashScreen();
    void closeSplashScreen();
    void setProgressSplashScreen(int value);
    void setStyleApplication();

private:
    AppConfig* _appCfg;
    PaAppLock* appLock;
    QWidget *closingWidget_;
    MainWindow* mainWindow_;
    SplashScreen *splashScreen_;

    bool _isClosing;
    bool showSplashScreen_;
    QString _initErrorDetails;  //应用初始化出错时的详情
};

#endif // MAINAPPLICATION_H
