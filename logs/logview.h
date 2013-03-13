#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include <QWidget>
#include <QHash>

#include "Logger.h"

namespace Ui {
class LogDialog;
}

class LogView : public QWidget
{
    Q_OBJECT
    
public:
    explicit LogView(QWidget *parent = 0);
    ~LogView();
    
private slots:
    void curViewLogLevelChanged(int index);
    void curRecLogLevelChanged(int index);

    void on_btnDump_clicked();

    void on_btnClose_clicked();

    void on_BtnClear_clicked();

    void on_btnRefresh_clicked();

signals:
    void onClose();


private:
    //void init();
    void readAllLogs();
    void viewLogs(int level);

    Ui::LogDialog *ui;
    QHash<Logger::LogLevel,QString> logNameHash; //日志级别枚举值到级别名称的映射
    QList<LogStruct *> logs;  //从日志文件中读取的日志信息
};

#endif // LOGDIALOG_H
