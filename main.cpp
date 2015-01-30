#include <QApplication>
#include <QtCore/QTextCodec>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QTableView>
#include <QTranslator>
#include <QPluginLoader>
#include <QDir>
#include <QLibraryInfo>
#include <QDesktopWidget>
#include <QtDebug>

#include "global.h"
#include "config.h"
#include "mainwindow.h"
#include "logs/FileAppender.h"
#include "myhelper.h"

/**
 * @brief showErrorInfo
 *  显示应用打开阶段的出现信息
 * @param errNum
 *  0：正常退出
 *  1：读取配置文件appSettings.ini出错
 *  2：用户取消了配置文件的升级
 *  3：基础库连接错误
 *  4：用户取消了基本库版本的升级
 *  5：基本库版本升级失败
 *  6：安全模块初始化错误
 *  7：全局变量初始化错误
 *  8：读取凭证类型出错
 */
void showErrorInfo(int errNum)
{
    QString info;
    switch(errNum){
    case 1:
        info = QObject::tr("读取配置文件appSettings.ini出错");
        break;
    case 2:
        info = QObject::tr("要正常运行当前版本程序，必须更新配置文件");
        break;
    case 3:
        info = QObject::tr("基础库连接错误");
        break;
    case 4:
        info = QObject::tr("要正常运行当前版本程序，必须升级基本库版本！");
        break;
    case 5:
        info = QObject::tr("基本库版本升级失败");
        break;
    case 6:
        info = QObject::tr("安全模块初始化错误");
        break;
    case 7:
        info = QObject::tr("全局变量初始化错误");
        break;
    case 8:
        info = QObject::tr("读取凭证类型出错");
        break;
    }
    QMessageBox::warning(0, QObject::tr("出错信息"),
                         QObject::tr("应用初始化时遇到错误：\n%1").arg(info));
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));
    myHelper::SetUTF8Code();
    //myHelper::SetStyle("navy");//天蓝色风格
    //myHelper::SetStyle("black");
    int errNum = appInit();
    if(errNum != 0){
        showErrorInfo(errNum);
        return errNum;
    }
    AppConfig* cfg = AppConfig::getInstance();
    QString style = cfg->getAppStyleName();
    if(style.isEmpty()){
        style = "navy";
        cfg->setAppStyleName(style);
    }
    myHelper::SetStyle(style);
    myHelper::SetChinese();
    appTitle = QObject::tr("凭证辅助处理系统");
    FileAppender* logFile = new FileAppender(LOGS_PATH + "app.log");
    Logger::registerAppender(logFile);

    Logger::write(Logger::Must,"",0,"","");
    Logger::write(Logger::Must,"",0,"",
                  "************************************************************");
    Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                  QObject::tr("PzAssistant is starting......"));
    logLevel = cfg->getLogLevel();
    logFile->setDetailsLevel(logLevel);

    MainWindow mainWin;
    mainWin.showMaximized();
    mainWin.getMdiAreaSize(mdiAreaWidth, mdiAreaHeight);
    int exitCode = app.exec();

    Logger::write(QDateTime::currentDateTime(),Logger::Must,"",0,"", QObject::tr("Quit PzAssistant!"));
    cfg->setLogLevel(logLevel);
    appExit();
    return exitCode;
}
