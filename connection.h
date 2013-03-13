#ifndef CONNECTION_H
#define CONNECTION_H

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QFile>
#include <QTextStream>

#include "common.h"

class ConnectionManager
{
public:
    //ConnectionManager();
    static bool crtConnection(QString fname)
    {
        if(db.isOpen())
            db.close();
        db.setDatabaseName(fname);
        if(!db.open()){
            QString strErr = db.lastError().text();
            QMessageBox::warning(0, QObject::tr("Database Error"),strErr);
            return false;
        }
        return true;
    }

    static bool openConnection(QString filename)
    {
//        if(db == NULL){
//            db = &QSqlDatabase::addDatabase("QSQLITE");
//        }

        if(db.isOpen())
            db.close();

        //QString fullname = "./datas/databases/" + filename + ".dat";
        QString fullname = QString("./datas/databases/%1.dat").arg(filename);
        db.setDatabaseName(fullname);
        if (!db.open()) {
//            QMessageBox::critical(0, qApp->tr("不能打开数据库"),
//                qApp->tr("不能够建立与SQlite数据库的连接\n"
//                         "请检查data目录下是否存在相应的数据库文件及其名称是否正确 "), QMessageBox::Cancel);
//            return false;
            QString strErr = db.lastError().text();
            QMessageBox::warning(0, QObject::tr("Database Error"),strErr);
            return false;
        }
        else{
            qDebug() << "Driver name: " << db.driverName();
            qDebug() << "Database file name is: " << db.databaseName();
            return true;
        }
    }


    static void closeConnection()
    {
        db.close();
//        if(db != NULL){
//            try{
//                db->close();
//            }
//            catch(QString exception)
//            {
//                QString s = exception;
//                QMessageBox::about(0,"error",s);
//            }

//            //qDebug() << db->lastError().text();
//        }
    }

    static QSqlDatabase getConnect()
    {
        if(db.isOpen())
            return db;
        else
            QSqlDatabase::database();
    }

private:
    static QSqlDatabase db;

};





#endif // CONNECTION_H
