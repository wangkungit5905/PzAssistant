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
#include "mainapplication.h"

int main(int argc, char *argv[])
{
    //测试新的提交（20190406）
    MainApplication app(argc,argv);
    if (app.isClosing()){
        myHelper::ShowMessageBoxWarning(app.tr("已有一个应用实例正在运行，如需重新启动应用，则先退出前一个应用实例后再启动！"));
        return 0;
    }
    AppConfig* cfg = AppConfig::getInstance();
    FileAppender* logFile = new FileAppender(LOGS_PATH + "app.log");
    Logger::registerAppender(logFile);

    Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                  QObject::tr("PzAssistant start runing......"));
    logLevel = cfg->getLogLevel();
    logFile->setDetailsLevel(logLevel);

    int exitCode = app.exec();

    Logger::write(QDateTime::currentDateTime(),Logger::Must,"",0,"", QObject::tr("Quit PzAssistant!"));
    cfg->setLogLevel(logLevel);
    return exitCode;
}
