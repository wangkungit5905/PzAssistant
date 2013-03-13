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
#include "FileAppender.h"

// STL
#include <iostream>


FileAppender::FileAppender(const QString& fileName)
{
  setFileName(fileName);
  QString fmtStr = QString(QLatin1String("%t{%1yyyy-MM-ddTHH:mm:ss%2} %1%l%2 %1%F%2 %1%i%2 %1%C%2 %1%m%2\n"))
          .arg(leftChar).arg(rightChar);
  setFormat(fmtStr);
}


QString FileAppender::fileName() const
{
  QMutexLocker locker(&m_logFileMutex);
  return m_logFile.fileName();
}


void FileAppender::setFileName(const QString& s)
{
  QMutexLocker locker(&m_logFileMutex);
  if (m_logFile.isOpen())
    m_logFile.close();

  m_logFile.setFileName(s);
}

QList<LogStruct*> FileAppender::getLogs()
{
    QList<LogStruct*> logs;
    QMutexLocker locker(&m_logFileMutex);
    QIODevice::OpenMode mode = m_logFile.openMode();

    if(!m_logFile.isOpen()){   //如果没有打开
        if (m_logFile.open(QIODevice::ReadOnly | QIODevice::Text))
                m_logStream.setDevice(&m_logFile);
        else
        {
            std::cerr << "<FileAppender::append> Cannot open the log file " << qPrintable(m_logFile.fileName()) << std::endl;
            return logs;
        }

    }
    else if((m_logFile.openMode() & QIODevice::ReadOnly) == 0){  //如果没有以只读模式打开
        m_logFile.close();
        if (m_logFile.open(QIODevice::ReadOnly | QIODevice::Text))
                m_logStream.setDevice(&m_logFile);
        else
        {
            std::cerr << "<FileAppender::append> Cannot open the log file " << qPrintable(m_logFile.fileName()) << std::endl;
            return logs;
        }
    }

    QString l;
    LogStruct* logItem;
    while(!m_logStream.atEnd()){
        l = m_logStream.readLine();
        logItem = parseLogItem(l);
        if(logItem)
            logs<<logItem;
    }
    m_logFile.close();
    return logs;
}

void FileAppender::clear()
{
    QMutexLocker locker(&m_logFileMutex);
    QIODevice::OpenMode mode = m_logFile.openMode();

    if(!m_logFile.isOpen()){   //如果没有打开
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text))
                m_logFile.resize(0);
        else
            std::cerr << "<FileAppender::append> Cannot open the log file "
                      << qPrintable(m_logFile.fileName()) << std::endl;
    }
    else if((m_logFile.openMode() & QIODevice::WriteOnly) == 0){  //如果没有以只读模式打开
        m_logFile.close();
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text))
                m_logFile.resize(0);
        else
            std::cerr << "<FileAppender::append> Cannot open the log file "
                      << qPrintable(m_logFile.fileName()) << std::endl;
    }
    m_logFile.close();
}


void FileAppender::append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                          const char* function, const QString& message)
{
  QMutexLocker locker(&m_logFileMutex);

  if (!m_logFile.isOpen())
  {
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
      m_logStream.setDevice(&m_logFile);
    }
    else
    {
      std::cerr << "<FileAppender::append> Cannot open the log file " << qPrintable(m_logFile.fileName()) << std::endl;
      return;
    }
  }

  //如果输出消息为空，则用换行代替
  if(message.isEmpty())
      m_logStream << QString("\n");
  else
      m_logStream << formattedString(timeStamp, logLevel, file, line, function, message);
  m_logStream.flush();
}

LogStruct *FileAppender::parseLogItem(QString logItemStr)
{
    if(logItemStr.isEmpty() || logItemStr.isNull())
        return NULL;
    //每一个合格的日志记录，都由6个字段组成
    QList<int> leftIndexes,rightIndexes;
    int index = 0;
    while(index != -1){
        index = logItemStr.indexOf(leftChar,index);
        if(index != -1){
            leftIndexes<<index;
            index = logItemStr.indexOf(rightChar, ++index);
            if(index != -1)
                rightIndexes<<index;
            else
                break;
        }
        else
            break;
    }
    if(leftIndexes.count() != 6 || rightIndexes.count() != 6)
        return NULL;

    QString s,fileName,funName,message;
    int level,line;
    QDateTime time;
    //第1个字段为时间戳
    s = logItemStr.mid(leftIndexes[0]+1,rightIndexes[0]-leftIndexes[0]-1);
    time = QDateTime::fromString(s,Qt::ISODate);
    if(!time.isValid())
        return NULL;

    //第2个字段是日志级别
    s = logItemStr.mid(leftIndexes[1]+1,rightIndexes[1]-leftIndexes[1]-1);
    if(s == QLatin1String("Debug"))
        level = 1;
    else if (s == QLatin1String("Info"))
        level = 2;
    else if (s == QLatin1String("Warning"))
        level = 3;
    else if (s == QLatin1String("Error"))
        level = 4;
    else if (s == QLatin1String("Fatal"))
        level = 5;
    else if (s == QLatin1String("Must"))
        level = 6;
    else
        return NULL;

    //第3个字段为文件名
    s = logItemStr.mid(leftIndexes[2]+1,rightIndexes[2]-leftIndexes[2]-1);
    fileName = s;

    //第4个字段为行号
    s = logItemStr.mid(leftIndexes[3]+1,rightIndexes[3]-leftIndexes[3]-1);
    line = s.toInt();
    if(!fileName.isEmpty() && line == 0) //如果日志条目中包含了文件名，则也必须包含行号
        return NULL;

    //第5个字段为函数名
    s = logItemStr.mid(leftIndexes[4]+1,rightIndexes[4]-leftIndexes[4]-1);
    funName = s;

    //第6个字段为日志消息
    s = logItemStr.mid(leftIndexes[5]+1,rightIndexes[5]-leftIndexes[5]-1);
    message = s;


    LogStruct* logItem = new LogStruct;
    logItem->time = time;
    logItem->level = level;
    logItem->file = fileName;
    logItem->line = line;
    logItem->funName = funName;
    logItem->message = message;
    return logItem;
}
