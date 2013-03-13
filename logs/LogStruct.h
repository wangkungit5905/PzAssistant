#ifndef LOGSTRUCT_H
#define LOGSTRUCT_H

#include <QDateTime>

class QString;
//class QDateTime;
class Logger;
//enum Logger::logLevel;

//应用程序日志结构
struct LogStruct{
    QDateTime time;
    //Logger::LogLevel level;
    int level;
    QString file;
    int line;
    QString funName;
    QString message;
};

//账户日志结构

#endif // LOGSTRUCT_H
