#include <QDesktopWidget>
#include <QApplication>
#include <QDir>

#include "global.h"
#include "version.h"

const char* ObjEditState = "editState";
QString orgName = "SSC";
QString appName = "PingZheng Assistant";
QString appTitle;
QString versionStr;
QString aboutStr;
Logger::LogLevel logLevel;

int DEFAULT_SUBSYS_CODE = 1;

Account* curAccount = NULL;
int screenWidth;
int screenHeight;

User* curUser = NULL;
//int recentUserId = 1;

QSqlDatabase adb;
QSqlDatabase bdb;
QString hVersion = "1.2";

QString LOGS_PATH;
QString PATCHES_PATH;
QString DATABASE_PATH;
QString BASEDATA_PATH;
QString BACKUP_PATH;

QString lastModifyTime;

QHash<int,QString> MTS;
QHash<int,QString> allMts;

QHash<int,GdzcClass*> allGdzcProductCls;
QHash<int,QString> allGdzcSubjectCls;

QSet<int> pzClsImps;
QSet<PzClass> pzClsJzhds;
QSet<int> pzClsJzsys;
QHash<PzState,QString> pzStates;

QHash<PzsState,QString> pzsStates;
QHash<PzsState,QString> pzsStateDescs;
QHash<PzClass,QString> pzClasses;

//int subCashId = 0;
int subBankId = 0;
int subYsId = 0;
int subYfId = 0;
//int subCashRmbId = 0;


ClipboardOperate copyOrCut;
QList<BusiActionData2*> clbBaList;
QList<BusiAction*> clb_Bas;

//输出借贷方向的文字表达
QString dirStr(int dir)
{
    QString s;
    switch(dir){
    case DIR_J:
        s = QObject::tr("借");
        break;
    case DIR_D:
        s = QObject::tr("贷");
        break;
    case DIR_P:
        s = QObject::tr("平");
        break;
    }
    return s;
}

QString dirVStr(double v)
{
    if(v == 0)
        return QObject::tr("平");
    else if(v > 0)
        return QObject::tr("借");
    else
        return QObject::tr("贷");
}

QString removeRightZero(QString str)
{
    QString num = str;
    if(str.right(3) == ".00")
        num.chop(3);
    else if((str.indexOf(".") != -1) && (str.right(1) == "0"))
        num.chop(1);
    return num;
}

