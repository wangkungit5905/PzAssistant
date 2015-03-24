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
    MainApplication app(argc,argv);
    if (app.isClosing())
        return 0;
    AppConfig* cfg = AppConfig::getInstance();
    FileAppender* logFile = new FileAppender(LOGS_PATH + "app.log");
    Logger::registerAppender(logFile);

    Logger::write(Logger::Must,"",0,"","");
    Logger::write(Logger::Must,"",0,"",
                  "************************************************************");
    Logger::write(QDateTime::currentDateTime(), Logger::Must,"",0,"",
                  QObject::tr("PzAssistant is starting......"));
    logLevel = cfg->getLogLevel();
    logFile->setDetailsLevel(logLevel);

    int exitCode = app.exec();

    Logger::write(QDateTime::currentDateTime(),Logger::Must,"",0,"", QObject::tr("Quit PzAssistant!"));
    cfg->setLogLevel(logLevel);
    return exitCode;
}
