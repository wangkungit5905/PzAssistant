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

//static bool createConnection()
//{
//    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
//    db.setDatabaseName("test.db");
//    if (!db.open()) {
//        QMessageBox::critical(0, qApp->tr("不能打开数据库"),
//            qApp->tr("不能够建立与SQlite数据库的连接\n"
//                     "请检查data目录下是否存在相应的数据库文件及其名称是否正确 "), QMessageBox::Cancel);
//        return false;
//    }

//    return true;
//}

//static bool createConnection(QString filename)
//{
//    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
//    QString fullname = "./datas/databases/" + filename + ".dat";
//    if(QFile::exists(fullname)){
//        if(QMessageBox::Cancel == QMessageBox::critical(0, qApp->tr("文件名称冲突"),
//                              qApp->tr("数据库文件名已存在，要覆盖吗？\n"
//                                 "文件覆盖后将导致先前文件的数据全部丢失"),
//                              QMessageBox::Ok | QMessageBox::Cancel)){
//            return false;

//        }
//    }

//    db.setDatabaseName(fullname);
//    if (!db.open()) {
//        QMessageBox::critical(0, qApp->tr("不能打开数据库"),
//            qApp->tr("不能够建立与SQlite数据库的连接\n"
//                     "请检查data目录下是否存在相应的数据库文件及其名称是否正确 "), QMessageBox::Cancel);
//        return false;
//    }
//    return true;
//}

//static bool openConnection(QString filename)
//{
//    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
//    QString fullname = "./datas/databases/" + filename + ".dat";
//    db.setDatabaseName(fullname);
//    if (!db.open()) {
//        QMessageBox::critical(0, qApp->tr("不能打开数据库"),
//            qApp->tr("不能够建立与SQlite数据库的连接\n"
//                     "请检查data目录下是否存在相应的数据库文件及其名称是否正确 "), QMessageBox::Cancel);
//        return false;
//    }
//    return true;
//}

//static bool closeConnection(QString connectionName = "")
//{
//    if(connectionName == ""){
//        //关闭默认连接
//        QSqlDatabase::removeDatabase(QSqlDatabase::database().connectionName());

//    }
//    else{
//        QSqlDatabase::removeDatabase(connectionName);
//    }
//}

//bool isTableExist(QString tname){
//    QSqlQuery query;
//    bool result;

//    result = query.exec("");
//}

//创建基本表格
static bool createBasicTable(int witch){
    QSqlQuery query;
    bool result;

    switch(witch){
    case FSTSUBTABLE:
        //一级科目表（FirSubjects）
        result = query.exec("DROP TABLE IF EXISTS FirSubjects");
        result = query.exec("CREATE TABLE FirSubjects(id INTEGER PRIMARY KEY, "
                            "subCode varchar(4), remCode varchar(10), "
                            "belongTo integer, isView integer, isReqDet INTEGER, weight integer, "
                            "subName varchar(10), description TEXT, utils TEXT)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建一级科目表失败"),
                                  qApp->tr("在创建一级科目表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;
    case SNDSUBTABLE:
        //二级科目表（SecSubjects）
        result = query.exec("DROP TABLE IF EXISTS SecSubjects");
        result = query.exec("CREATE TABLE SecSubjects(id INTEGER PRIMARY KEY, subName VERCHAR(10), subLName TEXT, remCode varchar(10), classId INTEGER)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建二级科目表失败"),
                                  qApp->tr("在创建二级科目表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;
    case ACTIONSTABLE:
        //业务活动表（BusiActions）
        result = query.exec("DROP TABLE IF EXISTS BusiActions");
        result = query.exec("CREATE TABLE BusiActions(id INTEGER PRIMARY KEY, "
                            "pid INTEGER, summary TEXT, firSubID INTEGER, "
                            "secSubID INTEGER, moneyType INTEGER, jMoney REAL, "
                            "dMoney REAL, dir BOOL, toTotal BOOL, toDetails BOOL)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建业务活动表失败"),
                                  qApp->tr("在创建业务活动表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;
    case PINGZHENGTABLE:
        //凭证表（PingZhengs）
        result = query.exec("DROP TABLE IF EXISTS PingZhengs");
        result = query.exec("CREATE TABLE PingZhengs(id INTEGER PRIMARY KEY, date TEXT, "
                            "number INTEGER, numberSub INTEGER, class INTEGER, jsum REAL, "
                            "dsum REAL, accBookGroup INTEGER, isForward INTEGER, encNum INTEGER)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建凭证表失败"),
                                  qApp->tr("在创建凭证表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;
    case FSTSUBCLASSTABLE:
        //一级科目类别表（FstSubClasses）
        result = query.exec("DROP TABLE IF EXISTS FstSubClasses");
        result = query.exec("CREATE TABLE FstSubClasses(id INTEGER PRIMARY KEY, code INTEGER, name TEXT)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建一级科目类别表失败"),
                                  qApp->tr("在创建一级科目类别表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;
    case FSAGENTTABLE:
        //一级科目到二级科目的代理映射表 fsagent
        result = query.exec("DROP TABLE IF EXISTS FSAgent");
        result = query.exec("CREATE TABLE FSAgent(id INTEGER PRIMARY KEY, "
                            "fid INTEGER, sid INTEGER, subCode varchar(5), "
                            "isDetByMt INTEGER, FrequencyStat INTEGER)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建一级科目到二级科目的代理映射表失败"),
                                  qApp->tr("在创建一级科目到二级科目的代理映射表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;

    case SNDSUBCLSTABLE:
        //二级科目类别表 SndSubClass
        result = query.exec("DROP TABLE IF EXISTS SndSubClass");
        result = query.exec("CREATE TABLE SndSubClass(id INTEGER PRIMARY KEY, "
                            "clsCode INTEGER, name TEXT, explain TEXT)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建二级科目类别表失败"),
                                  qApp->tr("在创建二级科目类别表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;

    case ACCOUNTINFO:
        //账户信息表
        result = query.exec("DROP TABLE IF EXISTS AccountInfos");
        result = query.exec("CREATE TABLE AccountInfos(id INTEGER PRIMARY KEY, "
                            "code TEXT, sname TEXT, lname TEXT, usedSubId INTEGER)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建账户信息表失败"),
                                  qApp->tr("在创建账户信息表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;


    case ACCOUNTBOOKGROUP:
        //账簿分类表
        result = query.exec("DROP TABLE IF EXISTS AccountBookGroups");
        result = query.exec("CREATE TABLE AccountBookGroups(id INTEGER PRIMARY KEY, "
                            "code INTEGER, name TEXT, explain TEXT)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建账簿分类表失败"),
                                  qApp->tr("在创建账簿分类表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;


    //科目余额表(此表的创建在导入一级科目以后自动创建)
//    case SUJECTEXTRA:
//        //科目余额表（各科目的字段名由代表科目类别的字母代码加上科目国标代码组成）
//        //A（资产类）B（负债类）C（共同类）D（所有者权益类）E（成本类）F（损益类）
//        result = query.exec("DROP TABLE IF EXISTS SubjectExtras");
//        result = query.exec("CREATE TABLE SubjectExtras("
//        "id INTEGER PRIMARY KEY, year INTEGER, month INTEGER, state INTEGER, "
//        "A1001 REAL, A1002 REAL, A1012 REAL, "  //库存现金,银行存款,其他货币资金
//        "A1101 REAL, A1121 REAL, A1122 REAL, "  //交易性金融资产,应收票据,应收账款
//        "A1123 REAL, A1131 REAL, A1132 REAL, "  //预付账款,应收股利,应收利息
//        "A1221 REAL, A1231 REAL, A1401 REAL, "  //其他应收款,坏账准备,材料采购
//        "A1402 REAL, A1403 REAL, A1404 REAL, "  //在途物资,原材料,材料成本差异
//        "A1405 REAL, A1406 REAL, A1407 REAL, "  //库存商品,发出商品,商品进销差价
//        "A1408 REAL, A1411 REAL, A1471 REAL, "  //委托加工物资,周转材料,存货跌价准备
//        "A1501 REAL, A1503 REAL, A1511 REAL, "  //持有至到期投资,可供出售金融资产,长期股权投资
//        "A1521 REAL, A1601 REAL, A1602 REAL, "  //投资性房地产,固定资产,累计折旧
//        "A1604 REAL, A1605 REAL, A1606 REAL, "  //在建工程,工程物资,固定资产清理
//        "A1701 REAL, A1702 REAL, A1801 REAL, "  //无形资产,累计摊销,长期待摊费用
//        "A1901 REAL, B2001 REAL, B2201 REAL, "  //待处理资产损溢,短期借款,应付票据
//        "B2202 REAL, B2203 REAL, B2211 REAL, "  //应付账款,预收账款,应付职工薪酬
//        "B2221 REAL, B2231 REAL, B2232 REAL, "  //应交税费,应付利息,应付股利/利润
//        "B2241 REAL, B2501 REAL, B2502 REAL, "  //其他应付款,长期借款,应付债券
//        "C3103 REAL, C3201 REAL, D4001 REAL, "  //衍生工具,套期工具,实收资本
//        "D4002 REAL, D4101 REAL, D4103 REAL, "  //资本公积,盈余公积,本年利润
//        "D4104 REAL, E5001 REAL, E5101 REAL, "  //利润分配,生产成本,制造费用
//        "E5201 REAL, E5301 REAL, F6001 REAL, "  //劳务成本,研发支出,主营业务收入
//        "F6051 REAL, F6101 REAL, F6111 REAL, "  //其他业务收入,公允价值变动损益,投资损益
//        "F6301 REAL, F6401 REAL, F6402 REAL, "  //营业外收入,主营业务成本,其他业务成本
//        "F6403 REAL, F6601 REAL, F6602 REAL, "  //营业税金及附加,销售费用,管理费用
//        "F6603 REAL, F6701 REAL, F6711 REAL, "  //财务费用,资产减值损失,营业外支出
//        "F6801 REAL)");                         //所得税费用

//        if(!result){
//            QMessageBox::critical(0, qApp->tr("创建科目余额表失败"),
//                                  qApp->tr("在创建科目余额表时发生错误"), QMessageBox::Cancel);
//            return false;
//        }
//        break;

    //报表结构信息表
    case REPORTSTRUCTS:
        result = query.exec("DROP TABLE IF EXISTS ReportStructs");
        result = query.exec("CREATE TABLE ReportStructs(id INTEGER PRIMARY KEY, "
                            "clsid INTEGER, tid INTEGER, viewOrder INTERGER, "
                            "calOrder INTEGER, rowNum INTEGER, fname TEXT, "
                            "ftitle TEXT, fformula TEXT)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建报表结构信息表失败"),
                                  qApp->tr("在创建报表结构信息表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;

    //报表其他信息表
    case REPORTADDITIONINFO:
        result = query.exec("DROP TABLE IF EXISTS ReportAdditionInfo");
        result = query.exec("CREATE TABLE ReportAdditionInfo(id INTEGER PRIMARY KEY, "
                            "clsid INTEGER, tid INTEGER, tname TEXT, title TEXT)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建报表结构信息表失败"),
                                  qApp->tr("在创建报表结构信息表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;

    //凭证集状态表（PZSetStates）
    case PZSETSTATE:
        result = query.exec("DROP TABLE IF EXISTS PZSetStates");
        result = query.exec("CREATE TABLE PZSetStates(id INTEGER PRIMARY KEY, "
                            "year INTEGER, month INTEGER, state INTEGER)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建凭证集状态表失败"),
                                  qApp->tr("在创建凭证集状态表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;

    //开户行账户信息表
    case BANKS:
        result = query.exec("DROP TABLE IF EXISTS Banks");
        result = query.exec("CREATE TABLE Banks(id INTEGER PRIMARY KEY, "
                            "isMain BOOL, name TEXT, lname TEXT, "
                            "RMB TEXT, USD TEXT)");
        if(!result){
            QMessageBox::critical(0, qApp->tr("创建开户行帐号信息表失败"),
                                  qApp->tr("在创建开户行帐号信息表时发生错误"), QMessageBox::Cancel);
            return false;
        }
        break;

    //资产负债表
    case BALANCESHEET:

        break;


    }

    return result;
}

//创建基本表格
static bool createBasicTable(){
    QSqlQuery query;
    bool result;

    //一级科目表（FirSubjects）
    result = query.exec("DROP TABLE IF EXISTS FirSubjects");
    result = query.exec("CREATE TABLE FirSubjects(id INTEGER PRIMARY KEY, "
                        "subCode varchar(4), remCode varchar(10), belongTo "
                        "integer, jdDir integer, isView integer, isReqDet "
                        "INTEGER, weight integer, subName varchar(10), "
                        "description TEXT, utils TEXT)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建一级科目表失败"),
                              qApp->tr("在创建一级科目表时发生错误"), QMessageBox::Cancel);
        return false;
    }

//    //凭证类别表（PzClasses）
//    result = query.exec("DROP TABLE IF EXISTS PzClasses");
//    result = query.exec("CREATE TABLE PzClasses(id INTEGER PRIMARY KEY, code INTEGER, "
//                        "name TEXT,sname TEXT, explain TEXT)");
//    if(!result){
//        QMessageBox::critical(0, qApp->tr("创建凭证类别表失败"),
//                              qApp->tr("在创建凭证类别表时发生错误"), QMessageBox::Cancel);
//        return false;
//    }

    //二级科目表（SecSubjects）
    result = query.exec("DROP TABLE IF EXISTS SecSubjects");
    result = query.exec("CREATE TABLE SecSubjects(id INTEGER PRIMARY KEY, subName VERCHAR(10), subLName TEXT, remCode varchar(10), classId INTEGER)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建二级科目表失败"),
                              qApp->tr("在创建二级科目表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    //业务活动表（BusiActions）
    result = query.exec("DROP TABLE IF EXISTS BusiActions");
    result = query.exec("CREATE TABLE BusiActions(id INTEGER PRIMARY KEY, pid INTEGER, "
                        "summary TEXT, firSubID INTEGER, secSubID INTEGER, moneyType INTEGER, "
                        "jMoney REAL, dMoney REAL, dir BOOL, NumInPz INTEGER)");

    if(!result){
        QMessageBox::critical(0, qApp->tr("创建业务活动表失败"),
                              qApp->tr("在创建业务活动表时发生错误"), QMessageBox::Cancel);
        return false;
    }


    //凭证表（PingZhengs）
//    result = query.exec("DROP TABLE IF EXISTS PingZhengs");
//    result = query.exec("CREATE TABLE PingZhengs(id INTEGER PRIMARY KEY, date TEXT, "
//                        "number INTEGER, numberSub INTEGER, class INTEGER, jsum REAL, "
//                        "dsum REAL, accBookGroup INTEGER, isForward INTEGER, encNum INTEGER)");
//    if(!result){
//        QMessageBox::critical(0, qApp->tr("创建凭证表失败"),
//                              qApp->tr("在创建凭证表时发生错误"), QMessageBox::Cancel);
//        return false;
//    }
    result = query.exec("DROP TABLE IF EXISTS PingZhengs");
    result = query.exec("CREATE TABLE PingZhengs(id INTEGER PRIMARY KEY, date TEXT, "
                        "number INTEGER, zbNum INTEGER, jsum REAL, dsum REAL, "
                        "isForward INTEGER, encNum INTEGER, pzState INTEGER, vuid "
                        "INTEGER, ruid INTEGER, buid INTEGER)");


    //一级科目类别表（FstSubClasses）
    result = query.exec("DROP TABLE IF EXISTS FstSubClasses");
    result = query.exec("CREATE TABLE FstSubClasses(id INTEGER PRIMARY KEY, code INTEGER, name TEXT)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建一级科目类别表失败"),
                              qApp->tr("在创建一级科目类别表时发生错误"), QMessageBox::Cancel);
        return false;
    }


    //一级科目到二级科目的代理映射表 fsagent
    result = query.exec("DROP TABLE IF EXISTS FSAgent");
    result = query.exec("CREATE TABLE FSAgent(id INTEGER PRIMARY KEY, "
                        "fid INTEGER, sid INTEGER, subCode varchar(5), "
                        "isDetByMt INTEGER, FrequencyStat INTEGER, isEnabled INTEGER)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建一级科目到二级科目的代理映射表失败"),
                              qApp->tr("在创建一级科目到二级科目的代理映射表时发生错误"), QMessageBox::Cancel);
        return false;
    }



    //二级科目类别表 SndSubClass
    result = query.exec("DROP TABLE IF EXISTS SndSubClass");
    result = query.exec("CREATE TABLE SndSubClass(id INTEGER PRIMARY KEY, "
                        "clsCode INTEGER, name TEXT, explain TEXT)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建二级科目类别表失败"),
                              qApp->tr("在创建二级科目类别表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    //账户信息表  (AccountInfos)
    result = query.exec("DROP TABLE IF EXISTS AccountInfos");
    result = query.exec("CREATE TABLE AccountInfos(id INTEGER PRIMARY KEY, code TEXT, sname TEXT, lname TEXT, usedSubId INTEGER)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建账户信息表失败"),
                              qApp->tr("在创建账户信息表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    //汇率表 (ExchangeRates)
    result = query.exec("DROP TABLE IF EXISTS ExchangeRates");
    result = query.exec("CREATE TABLE ExchangeRates(id INTEGER PRIMARY KEY, year INTEGER, month INTEGER, usd2rmb REAL)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建汇率表失败"),
                              qApp->tr("在创建汇率表时发生错误"), QMessageBox::Cancel);
        return false;
    }

//    //凭证分册类别表
//    result = query.exec("DROP TABLE IF EXISTS AccountBookGroups");
//    result = query.exec("CREATE TABLE AccountBookGroups(id INTEGER PRIMARY KEY, "
//                        "code INTEGER, name TEXT, explain TEXT)");
//    if(!result){
//        QMessageBox::critical(0, qApp->tr("创建账簿分类表失败"),
//                              qApp->tr("在账簿分类表时发生错误"), QMessageBox::Cancel);
//        return false;
//    }

//    //科目余额表（各科目的字段名由代表科目类别的字母代码加上科目国标代码组成）
//    //A（资产类）B（负债类）C（共同类）D（所有者权益类）E（成本类）F（损益类）
//    //建议从一级科目表中读取并建立相应的余额字段名
//    result = query.exec("DROP TABLE IF EXISTS SubjectExtras");
//    result = query.exec("CREATE TABLE SubjectExtras("
//    "id INTEGER PRIMARY KEY, year INTEGER, month INTEGER, state INTEGER, "
//    "A1001 REAL, A1002 REAL, A1012 REAL, "  //库存现金,银行存款,其他货币资金
//    "A1101 REAL, A1121 REAL, A1122 REAL, "  //交易性金融资产,应收票据,应收账款
//    "A1123 REAL, A1131 REAL, A1132 REAL, "  //预付账款,应收股利,应收利息
//    "A1221 REAL, A1231 REAL, A1401 REAL, "  //其他应收款,坏账准备,材料采购
//    "A1402 REAL, A1403 REAL, A1404 REAL, "  //在途物资,原材料,材料成本差异
//    "A1405 REAL, A1406 REAL, A1407 REAL, "  //库存商品,发出商品,商品进销差价
//    "A1408 REAL, A1411 REAL, A1471 REAL, "  //委托加工物资,周转材料,存货跌价准备
//    "A1501 REAL, A1503 REAL, A1511 REAL, "  //持有至到期投资,可供出售金融资产,长期股权投资
//    "A1521 REAL, A1601 REAL, A1602 REAL, "  //投资性房地产,固定资产,累计折旧
//    "A1604 REAL, A1605 REAL, A1606 REAL, "  //在建工程,工程物资,固定资产清理
//    "A1701 REAL, A1702 REAL, A1801 REAL, "  //无形资产,累计摊销,长期待摊费用
//    "A1901 REAL, B2001 REAL, B2201 REAL, "  //待处理资产损溢,短期借款,应付票据
//    "B2202 REAL, B2203 REAL, B2211 REAL, "  //应付账款,预收账款,应付职工薪酬
//    "B2221 REAL, B2231 REAL, B2232 REAL, "  //应交税费,应付利息,应付股利/利润
//    "B2241 REAL, B2501 REAL, B2502 REAL, "  //其他应付款,长期借款,应付债券
//    "C3103 REAL, C3201 REAL, D4001 REAL, "  //衍生工具,套期工具,实收资本
//    "D4002 REAL, D4101 REAL, D4103 REAL, "  //资本公积,盈余公积,本年利润
//    "D4104 REAL, E5001 REAL, E5101 REAL, "  //利润分配,生产成本,制造费用
//    "E5201 REAL, E5301 REAL, F6001 REAL, "  //劳务成本,研发支出,主营业务收入
//    "F6051 REAL, F6101 REAL, F6111 REAL, "  //其他业务收入,公允价值变动损益,投资损益
//    "F6301 REAL, F6401 REAL, F6402 REAL, "  //营业外收入,主营业务成本,其他业务成本
//    "F6403 REAL, F6601 REAL, F6602 REAL, "  //营业税金及附加,销售费用,管理费用
//    "F6603 REAL, F6701 REAL, F6711 REAL, "  //财务费用,资产减值损失,营业外支出
//    "F6801 REAL)");                         //所得税费用

//    if(!result){
//        QMessageBox::critical(0, qApp->tr("创建科目余额表失败"),
//                              qApp->tr("在创建科目余额表时发生错误"), QMessageBox::Cancel);
//        return false;
//    }

    //报表结构信息表

    result = query.exec("DROP TABLE IF EXISTS ReportStructs");
    result = query.exec("CREATE TABLE ReportStructs(id INTEGER PRIMARY KEY, "
                        "clsid INTEGER, tid INTEGER, viewOrder INTERGER, "
                        "calOrder INTEGER, rowNum INTEGER, fname TEXT, "
                        "ftitle TEXT, fformula TEXT)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建报表结构信息表失败"),
                              qApp->tr("在创建报表结构信息表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    //报表其他信息表
    result = query.exec("DROP TABLE IF EXISTS ReportAdditionInfo");
    result = query.exec("CREATE TABLE ReportAdditionInfo(id INTEGER PRIMARY KEY, "
                        "clsid INTEGER, tid INTEGER, tname TEXT, title TEXT)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建报表结构信息表失败"),
                              qApp->tr("在创建报表结构信息表时发生错误"), QMessageBox::Cancel);
        return false;
    }


    //凭证集状态表（PZSetStates）
    result = query.exec("DROP TABLE IF EXISTS PZSetStates");
    result = query.exec("CREATE TABLE PZSetStates(id INTEGER PRIMARY KEY, "
                        "year INTEGER, month INTEGER, state INTEGER)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建凭证集状态表失败"),
                              qApp->tr("在创建凭证集状态表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    //开户行帐号信息表
    result = query.exec("DROP TABLE IF EXISTS Banks");
    result = query.exec("CREATE TABLE Banks(id INTEGER PRIMARY KEY, "
                        "isMain BOOL, name TEXT, lname TEXT)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建开户行帐号信息表失败"),
                              qApp->tr("在创建开户行帐号信息表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    //币种表
    result = query.exec("DROP TABLE IF EXISTS MoneyTypes");
    result = query.exec("CREATE TABLE MoneyTypes(id INTEGER PRIMARY KEY, "
                        "code INTEGER, sign TEXT, name TEXT)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建币种表失败"),
                              qApp->tr("在创建币种表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    //银行账户表（BankAccounts）
    result = query.exec("DROP TABLE IF EXISTS BankAccounts");
    result = query.exec("CREATE TABLE BankAccounts(id INTEGER PRIMARY KEY, "
                        "bankID INTEGER, mtID INTEGER, accNum TEXT)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建银行账户表失败"),
                              qApp->tr("在创建银行账户表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    //明细科目余额表（ detailExtras）
    result = query.exec("DROP TABLE IF EXISTS detailExtras");
    result = query.exec("CREATE TABLE detailExtras(id integer primary key,"
                        "seid integer,fsid integer,dir integer,value real)");
    if(!result){
        QMessageBox::critical(0, qApp->tr("创建明细科目余额表失败"),
                              qApp->tr("在创建明细科目余额表时发生错误"), QMessageBox::Cancel);
        return false;
    }

    return result;
}


//从文本文件中导入基本数据
//static bool impBasicDataFromTxtFile(){
//    bool result;
//    QStringList strList;
//    QFile* file;
//    QTextStream* in;
//    QSqlQuery query;

//    //一级科目
//    file = new QFile("datas/basicdatas/FirSubjects.txt");
//    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
//        return  false;
//    in = new QTextStream(file);
//    query.prepare("INSERT INTO FirSubjects(subCode,remCode,belongTo,weight,subName,description,utils) "
//                           "VALUES (:subCode, :remCode, :belongTo, :weight, :subName, :desc, :utils)");
//    while (!in->atEnd()) {
//        QString line = in->readLine();
//        strList = line.split(",");
//        if (strList.count() == 7){
//            query.bindValue(":subCode", strList[0]);
//            query.bindValue("remCode", strList[1]);
//            query.bindValue(":belosngTo", strList[2].toInt());
//            query.bindValue(":weight", strList[3].toInt());
//            query.bindValue(":subName", strList[4]);
//            query.bindValue(":desc", strList[5]);
//            query.bindValue(":utils", strList[6]);
//            result = query.exec();
//        }
//    }

//    //二级科目
//    delete file;
//    file = new QFile("datas/basicdatas/SecSubjects.txt");
//    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
//        return  false;
//    in = new QTextStream(file);
//    query.prepare("INSERT INTO SecSubjects(subName, subLName, remCode, classId) "
//                           "VALUES (:subName, :subLName, :remCode, :classId)");
//    while (!in->atEnd()) {
//        QString line = in->readLine();
//        strList = line.split(",");
//        if (strList.count() == 4){
//            query.bindValue(":subName", strList[SNDSUB_SUBNAME-1]);
//            query.bindValue(":subLName", strList[SNDSUB_SUBLONGNAME-1]);
//            query.bindValue(":remCode", strList[SNDSUB_REMCODE-1]);
//            query.bindValue(":classId", strList[SNDSUB_CALSS-1].toInt());
//            result = query.exec();
//        }
//    }

//    //一级科目类别表
//    delete file;
//    file = new QFile("datas/basicdatas/FstSubClasses.txt");
//    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
//        return  false;
//    in = new QTextStream(file);
//    query.prepare("INSERT INTO FstSubClasses(name) "
//                           "VALUES (:name)");
//    while (!in->atEnd()) {
//        QString line = in->readLine();
//        strList = line.split(",");
//        if (strList.count() > 0){
//            for(int i = 0; i < strList.count(); ++i){
//                query.bindValue(":name", strList[i]);
//                result = query.exec();
//            }

//        }
//    }

//    //一级科目到二级科目的代理映射表


//    //二级科目类别表
//    delete file;
//    file = new QFile("datas/basicdatas/SecSubClasses.txt");
//    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
//        return  false;
//    in = new QTextStream(file);
//    query.prepare("INSERT INTO SndSubClass(name, explain) "
//                           "VALUES (:name, :explain)");
//    while (!in->atEnd()) {
//        QString line = in->readLine();
//        strList = line.split(",");
//        if (strList.count() == 2){
//            query.bindValue(":name", strList[0]);
//            query.bindValue(":explain", strList[1]);

//            result = query.exec();
//        }
//    }
//    return result;
//}

//导入测试数据
//static bool importTestDatas(){
//    bool result;
//    QStringList strList;
//    QFile* file;
//    QTextStream* in;
//    QSqlQuery query;

//    //业务活动表数据
//    if(!createBasicTable(3))
//        return false;
//    file = new QFile("datas/testdatas/BusiActions.txt");
//    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
//        return  false;
//    in = new QTextStream(file);
//    query.prepare("INSERT INTO BusiActions(pid, summary, firSubID, secSubID, jMoney, dMoney, moneyType) "
//                           "VALUES (:pid, :summary, :firSubID, :secSubID, :jMoney, :dMoney, :moneyType)");
//    while (!in->atEnd()) {
//        QString line = in->readLine();
//        strList = line.split(",");
//        if (strList.count() == 7){
//            query.bindValue(":pid", strList[0].toInt());
//            query.bindValue(":summary", strList[1]);
//            query.bindValue(":firSubID", strList[2].toInt());
//            query.bindValue(":secSubID", strList[3].toInt());
//            query.bindValue(":jMoney", strList[4].toDouble());
//            query.bindValue(":dMoney", strList[5].toDouble());
//            query.bindValue(":moneyType", strList[6].toInt());
//            result = query.exec();
//        }
//    }

//    //凭证表数据
//    if(!createBasicTable(4))
//        return false;
//    delete file;

//    file = new QFile("datas/testdatas/PingZhengs.txt");
//    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
//        return  false;
//    in = new QTextStream(file);
//    query.prepare("INSERT INTO PingZhengs(date, number, numberSub, class, jsum, dsum) "
//                           "VALUES (:date, :number, :numberSub, :class, :jsum, :dsum)");
//    while (!in->atEnd()) {
//        QString line = in->readLine();
//        strList = line.split(",");
//        if (strList.count() == 4){
//            query.bindValue(":date", strList[0]);
//            query.bindValue(":number", strList[1].toInt());
//            query.bindValue(":numberSub", strList[2].toInt());
//            query.bindValue(":class", strList[3].toInt());
//            query.bindValue(":jsum", strList[4].toDouble());
//            query.bindValue(":jsum", strList[5].toDouble());
//            result = query.exec();
//        }
//    }

//    return result;
//}


#endif // CONNECTION_H
