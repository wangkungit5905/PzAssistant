/*
  Copyright (c) 2010 Boris Moiseev (cyberbobs at gmail dot com)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1
  as published by the Free Software Foundation and appearing in the file
  LICENSE.LGPL included in the packaging of this file.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.
*/
// Local
#include "Logger.h"
#include "AbstractAppender.h"
#include "FileAppender.h"

// Qt
#include <QCoreApplication>
#include <QReadWriteLock>
#include <QDateTime>
#include <QIODevice>
#include <QDebug>

// STL
#include <iostream>


class LogDevice : public QIODevice
{
  public:
    LogDevice()
    {}

    void lock(Logger::LogLevel logLevel, const char* file, int line, const char* function)
    {
      m_selfMutex.lock();

      if (!isOpen())
        open(QIODevice::WriteOnly);

      m_logLevel = logLevel;
      m_file = file;
      m_line = line;
      m_function = function;
    }

  protected:
    qint64 readData(char*, qint64)
    {
      return 0;
    }

    qint64 writeData(const char* data, qint64 maxSize)
    {
      if (maxSize > 0)
        Logger::write(m_logLevel, m_file, m_line, m_function, QString::fromLocal8Bit(QByteArray(data, maxSize)));

      m_selfMutex.unlock();
      return maxSize;
    }

  private:
    QMutex m_selfMutex;
    Logger::LogLevel m_logLevel;
    const char* m_file;
    int m_line;
    const char* m_function;
};


// Forward declarations
static void cleanupLoggerPrivate();
static void qtLoggerMessageHandler(QtMsgType type, const char* msg);


/**
 * LoggerPrivate类用线程安全的方式实现了单例模式。它通过使用一个指向自身的静态指针，
 * 并通过QReadWriteLock来保护。
 *
 * LoggerPrivate类内部维护着一个appender列表，并利用QReadWriteLock来保护，因此，
 * 该类在多线程应用内使用也是安全的。
 */
class LoggerPrivate
{
  public:
    static LoggerPrivate* m_self;
    static QReadWriteLock m_selfLock;

    //返回自身的单一实例
    static LoggerPrivate* instance()
    {
      LoggerPrivate* result = 0;
      {
        QReadLocker locker(&m_selfLock);
        result = m_self;
      }

      if (!result)
      {
        QWriteLocker locker(&m_selfLock);
        m_self = new LoggerPrivate;
        qInstallMsgHandler(qtLoggerMessageHandler);
        qAddPostRoutine(cleanupLoggerPrivate);
        result = m_self;
      }

      return result;
    }


    LoggerPrivate()
      : m_logDevice(0)
    {}


    ~LoggerPrivate()
    {
      // 在析构函数内会清除注册的appender，因此，应用程序无须考虑
      QReadLocker appendersLocker(&m_appendersLock);
      foreach (AbstractAppender* appender, m_appenders)
        delete appender;

      // Cleanup device
      QReadLocker deviceLocker(&m_logDeviceLock);
      delete m_logDevice;
    }

    //注册一个appender，用于调试信息的输出
    void registerAppender(AbstractAppender* appender)
    {
      QWriteLocker locker(&m_appendersLock);

      if (!m_appenders.contains(appender))
        m_appenders.append(appender);
      else
        std::cerr << "Trying to register appender that was already registered" << std::endl;
    }

    //获取一个QIODevice设备，以使我们可以像使用QDebug类一样使用它
    LogDevice* logDevice()
    {
      LogDevice* result = 0;
      {
        QReadLocker locker(&m_logDeviceLock);
        result = m_logDevice;
      }

      if (!result)
      {
        QWriteLocker locker(&m_logDeviceLock);
        m_logDevice = new LogDevice;
        result = m_logDevice;
      }

      return result;
    }

    //日志输出函数，日志信息会输出到任何注册的appender中，如果未注册appender，则默认输出到标准错误流
    void write(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line, const char* function,
               const QString& message)
    {
      QReadLocker locker(&m_appendersLock);

      if (!m_appenders.isEmpty())
      {
        foreach (AbstractAppender* appender, m_appenders)
          appender->write(timeStamp, logLevel, file, line, function, message);
      }
      else
      {
        // Fallback
        QString result = QString(QLatin1String("[%1] <%2> %3")).arg(Logger::levelToString(logLevel), -7)
                                                               .arg(function).arg(message);

        std::cerr << qPrintable(result) << std::endl;
      }

      if (logLevel == Logger::Fatal)
        abort();
    }


    void write(Logger::LogLevel logLevel, const char* file, int line, const char* function, const QString& message)
    {
      write(QDateTime::currentDateTime(), logLevel, file, line, function, message);
    }


    void write(Logger::LogLevel logLevel, const char* file, int line, const char* function, const char* message)
    {
      write(logLevel, file, line, function, QString::fromAscii(message));
    }


    QDebug write(Logger::LogLevel logLevel, const char* file, int line, const char* function)
    {
      LogDevice* d = logDevice();
      d->lock(logLevel, file, line, function);
      return QDebug(d);
    }


    void writeAssert(const char* file, int line, const char* function, const char* condition)
    {
      write(Logger::Fatal, file, line, function, QString("ASSERT: \"%1\"").arg(condition));
    }

    //读取指定日志输出设备上的日志信息（目前，只支持从文件日志中读取日志）
    QList<LogStruct*> read(Logger::LogOutDevice d)
    {
        QList<LogStruct*> logs;
        if(d = Logger::File){
            QReadLocker locker(&m_appendersLock);
            bool found = false;
            int i = 0;
            FileAppender* fa = 0;
            while(!found && i < m_appenders.count()){
                fa = static_cast<FileAppender*>(m_appenders[i]);
                if(fa)
                    found = true;
                ++i;
            }
            if(found)
                logs = fa->getLogs();
        }
        return logs;
    }

    //清空指定日志输出设备上的日志信息（目前仅支持文件日志）
    QList<LogStruct*> clear(Logger::LogOutDevice d)
    {
        QList<LogStruct*> logs;
        if(d = Logger::File){
            QReadLocker locker(&m_appendersLock);
            bool found = false;
            int i = 0;
            FileAppender* fa = 0;
            while(!found && i < m_appenders.count()){
                fa = static_cast<FileAppender*>(m_appenders[i]);
                if(fa)
                    found = true;
                ++i;
            }
            if(found)
                fa->clear();
        }
        return logs;
    }



  private:
    QList<AbstractAppender*> m_appenders;
    QReadWriteLock m_appendersLock;

    LogDevice* m_logDevice;
    QReadWriteLock m_logDeviceLock;
};

// Static fields initialization
LoggerPrivate* LoggerPrivate::m_self = 0;
QReadWriteLock LoggerPrivate::m_selfLock;

//清除例程，清除LoggerPrivate实例
static void cleanupLoggerPrivate()
{
  QWriteLocker locker(&LoggerPrivate::m_selfLock);

  delete LoggerPrivate::m_self;
  LoggerPrivate::m_self = 0;
}

//捕获Qt的各种调试信息输出（主要是通过QDebug类）到自己的日志输出中
static void qtLoggerMessageHandler(QtMsgType type, const char* msg)
{
  switch (type)
  {
    case QtDebugMsg:
      LOG_DEBUG(msg);
      break;
    case QtWarningMsg:
      LOG_WARNING(msg);
      break;
    case QtCriticalMsg:
      LOG_ERROR(msg);
      break;
    case QtFatalMsg:
      LOG_FATAL(msg);
      break;
  }
}

void Logger::getLogLevelList(QList<Logger::LogLevel> &elst, QList<QString> &names)
{
    elst<<Debug<<Info<<Warning<<Error<<Fatal<<Must;
    names<<QObject::tr("Debug")<<QObject::tr("Info")<<QObject::tr("Warning")
         <<QObject::tr("Error")<<QObject::tr("Fatal")<<QObject::tr("Must");
}

QString Logger::levelToString(Logger::LogLevel logLevel)
{
  switch (logLevel)
  {
    case Debug:
      return QLatin1String("Debug");
    case Info:
      return QLatin1String("Info");
    case Warning:
      return QLatin1String("Warning");
    case Error:
      return QLatin1String("Error");
    case Fatal:
      return QLatin1String("Fatal");
    case Must:
      return QLatin1String("Must");
  }

  return QString();
}


Logger::LogLevel Logger::levelFromString(const QString& s)
{
  QString str = s.trimmed();

  LogLevel result = Debug;

  if (str == QLatin1String("Debug"))
    result = Debug;
  else if (str == QLatin1String("Info"))
    result = Info;
  else if (str == QLatin1String("Warning"))
    result = Warning;
  else if (str == QLatin1String("Error"))
    result = Error;
  else if (str == QLatin1String("Fatal"))
    result = Fatal;
  else if (str == QLatin1String("Must"))
    result = Must;
  return result;
}


void Logger::registerAppender(AbstractAppender* appender)
{
  LoggerPrivate::instance()->registerAppender(appender);
}


void Logger::write(const QDateTime& timeStamp, LogLevel logLevel, const char* file, int line, const char* function,
                   const QString& message)
{
  LoggerPrivate::instance()->write(timeStamp, logLevel, file, line, function, message);
}


void Logger::write(LogLevel logLevel, const char* file, int line, const char* function, const QString& message)
{
  LoggerPrivate::instance()->write(logLevel, file, line, function, message);
}


void Logger::write(LogLevel logLevel, const char* file, int line, const char* function, const char* message)
{
  LoggerPrivate::instance()->write(logLevel, file, line, function, message);
}


QDebug Logger::write(LogLevel logLevel, const char* file, int line, const char* function)
{
  return LoggerPrivate::instance()->write(logLevel, file, line, function);
}


void Logger::writeAssert(const char* file, int line, const char* function, const char* condition)
{
    LoggerPrivate::instance()->writeAssert(file, line, function, condition);
}


QList<LogStruct *> Logger::read(LogOutDevice device)
{
    return LoggerPrivate::instance()->read(device);
}

QList<LogStruct *> Logger::clear(Logger::LogOutDevice device)
{
     return LoggerPrivate::instance()->clear(device);
}
