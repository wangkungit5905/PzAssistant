#include <QtGui>


#include "connection.h"

QSqlDatabase ConnectionManager::db = QSqlDatabase::addDatabase("QSQLITE");
//QSqlDatabase ConnectionManager::db = NULL;

//ConnectionManager::ConnectionManager()
//{
////    if(db == NULL){
////        db = &QSqlDatabase::addDatabase("QSQLITE");
////    }
//}

//bool ConnectionManager::openConnection(QString filename)
//{
//    if(db == NULL){
//        db = &QSqlDatabase::addDatabase("QSQLITE");
//    }

//    if(db->isOpen())
//        db->close();

//    db->setDatabaseName(filename);
//    if (!db->open()) {
//        QMessageBox::critical(0, qApp->tr("不能打开数据库"),
//            qApp->tr("不能够建立与SQlite数据库的连接\n"
//                     "请检查data目录下是否存在相应的数据库文件及其名称是否正确 "), QMessageBox::Cancel);
//        return false;
//    }
//    return true;
//}

//void ConnectionManager::closeConnection()
//{
//    if(db != NULL)
//        db->close();
//}
