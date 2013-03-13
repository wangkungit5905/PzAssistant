#include <QtGui>


#include "connection.h"

QSqlDatabase ConnectionManager::db = QSqlDatabase::addDatabase("QSQLITE");
//QSqlDatabase ConnectionManager::db = NULL;


