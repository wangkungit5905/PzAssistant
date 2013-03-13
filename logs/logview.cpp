#include "logs/logview.h"
#include "ui_logview.h"
#include "global.h"

LogView::LogView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogDialog)
{
    ui->setupUi(this);

    QList<Logger::LogLevel> logLevelEnums;
    QList<QString> logLevelNames;
    Logger::getLogLevelList(logLevelEnums,logLevelNames);
    ui->cmbViewLevel->addItem(tr("All"),0);
    for(int i = 0; i < logLevelEnums.count(); ++i){
        ui->cmbViewLevel->addItem(logLevelNames[i],logLevelEnums[i]);
        ui->cmbRecLevel->addItem(logLevelNames[i],logLevelEnums[i]);
        logNameHash[logLevelEnums[i]] = logLevelNames[i];
    }
    ui->cmbViewLevel->setCurrentIndex(0);
    int idx = ui->cmbRecLevel->findData(logLevel);
    ui->cmbRecLevel->setCurrentIndex(idx);

    readAllLogs();
    connect(ui->cmbRecLevel,SIGNAL(currentIndexChanged(int)),
            this, SLOT(curRecLogLevelChanged(int)));
    connect(ui->cmbViewLevel,SIGNAL(currentIndexChanged(int)),
            this,SLOT(curViewLogLevelChanged(int)));
    viewLogs(0);
}

LogView::~LogView()
{
    delete ui;
}

void LogView::curViewLogLevelChanged(int index)
{
    int level = ui->cmbViewLevel->itemData(index).toInt();
    viewLogs(level);
}

void LogView::curRecLogLevelChanged(int index)
{
    logLevel = (Logger::LogLevel)ui->cmbRecLevel->itemData(index).toInt();
    //appSetting.setLogLevel(logLevel);
}

void LogView::on_btnDump_clicked()
{

}

void LogView::on_btnClose_clicked()
{
    //close();
    emit onClose();
}

//读取所有日志
void LogView::readAllLogs()
{
    logs = Logger::read();
}

//显示指定级别的日志信息
void LogView::viewLogs(int level)
{
    ui->tw->clear();

    //QTreeWidgetItem* rootItem = ui->tw->topLevelItem(0);
    QStringList str;
    foreach(LogStruct* logItem, logs){
        if(level != 0 && logItem->level != level)
            continue;
        str<<logItem->time.toString(Qt::ISODate)
           <<logNameHash.value((Logger::LogLevel)logItem->level)
           <<logItem->file;
        if(logItem->line == 0)
            str<<"";
        else
            str<<QString::number(logItem->line);
        str<<logItem->funName<<logItem->message;

        QTreeWidgetItem* item = new QTreeWidgetItem(str);
        //rootItem->addChild(item);
        ui->tw->addTopLevelItem(item);
        str.clear();
    }
}

//清空日志文件
void LogView::on_BtnClear_clicked()
{
    Logger::clear();
    foreach(LogStruct* item, logs)
        delete item;
    logs.clear();
    ui->tw->clear();
}

//刷新日志
void LogView::on_btnRefresh_clicked()
{
    foreach(LogStruct* item, logs)
        delete item;
    logs.clear();
    ui->tw->clear();
    readAllLogs();
    int level = ui->cmbViewLevel->itemData(ui->cmbViewLevel->currentIndex())
            .toInt();
    viewLogs(level);
}
