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
#include "connection.h"
#include "logs/FileAppender.h"

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

    //QTextCodec::setCodecForTr(QTextCodec::codecForName("utf-8"));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));
    appTitle = QObject::tr("凭证辅助处理系统");
    QTranslator translator; //汉化标准对话框、标准上下文菜单等
    translator.load("qt_zh_CN.qm","./translations");
    //translator.load("i18n_zh"); //这个不行
    app.installTranslator(&translator );

	int errNum = appInit();
    if(errNum != 0){
        showErrorInfo(errNum);
        return errNum;
    }

    FileAppender* logFile = new FileAppender(LOGS_PATH + "app.log");
    Logger::registerAppender(logFile);

    Logger::write(Logger::Must,"",0,"","");
    Logger::write(Logger::Must,"",0,"",
                  "************************************************************");
    Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                  QObject::tr("PzAssistant is starting......"));
    //qDebug()<<"qDebug export info!";
    logLevel = AppConfig::getInstance()->getLogLevel();
    logFile->setDetailsLevel(logLevel);

    MainWindow mainWin;
    mainWin.showMaximized();
    mainWin.hideDockWindows();
    mainWin.getMdiAreaSize(mdiAreaWidth, mdiAreaHeight);
    int exitCode = app.exec();

    Logger::write(QDateTime::currentDateTime(),Logger::Must,"",0,"", QObject::tr("Quit PzAssistant!"));
    AppConfig::getInstance()->setLogLevel(logLevel);
    appExit();
    return exitCode;
}
