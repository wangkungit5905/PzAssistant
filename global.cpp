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

Account* curAccount = NULL;
int screenWidth;
int screenHeight;

User* curUser = NULL;

QSqlDatabase adb;
QSqlDatabase bdb;
QString hVersion = "1.2";

QString DatabasePath;
QString BaseDataPath;
QString BackupPath;

QString lastModifyTime;

QHash<int,QString> MTS;
QHash<int,QString> allMts;

QHash<int,GdzcType*> allGdzcProductCls;
QHash<int,QString> allGdzcSubjectCls;

QSet<int> pzClsImps;
QSet<PzClass> pzClsJzhds;
QSet<int> pzClsJzsys;
QHash<PzState,QString> pzStates;

QHash<PzsState,QString> pzsStates;
QHash<PzsState,QString> pzsStateDescs;
QHash<PzClass,QString> pzClasses;

int subCashId = 0;
int subBankId = 0;
int subYsId = 0;
int subYfId = 0;
int subCashRmbId = 0;

bool isByMt;
bool isCollapseJz;
int  autoSaveInterval;
bool jzlrByYear;
int  timeoutOfTemInfo = 10000;
bool viewHideColInDailyAcc1;
bool viewHideColInDailyAcc2;
ClipboardOperate copyOrCut;
QList<BusiActionData2*> clbBaList;
QList<BusiAction*> clb_Bas;
double czRate;
bool rt_update_extra;

/**
 * @brief appInit
 *  应用初始化例程
 * @return（要对出错代码按不同的模块或例程进行规划，预留一定区间的代码给指定例程或模块）
 *  0：正常退出
 *  1：升级配置模块时出错
 *  2：用户取消了配置模块的升级 *
 *  3：基础库连接错误
 *  4：
 *  5：
 *  6：安全模块初始化错误
 *  7：全局变量初始化错误
 *  8：读取凭证类型出错
 */
int appInit()
{
    //初始化路径信息
    DatabasePath = QDir::toNativeSeparators(QDir::currentPath().append("/datas/databases/"));
    BaseDataPath = QDir::toNativeSeparators(QDir::currentPath().append("/datas/basicdatas/"));
    BackupPath = QDir::toNativeSeparators(QDir::currentPath().append("/datas/backups/"));
    VersionManager vm(VersionManager::MT_CONF);
    VersionUpgradeInspectResult result = vm.isMustUpgrade();
    bool exec = false;
    switch(result){
    case VUIR_CANT:
        return 1;
    case VUIR_DONT:
        exec = false;
        break;
    case VUIR_LOW:
        return 1;
    case VUIR_MUST:
        exec = true;
        break;
    }
    if(exec){
        if(vm.exec() == QDialog::Rejected){
            return 2;
        }
        else if(!vm.getUpgradeResult())
            return 1;
    }

    AppConfig* appCfg = AppConfig::getInstance();
    if(!appCfg)
        return 3;

    //设置应用程序的版本号
    int master = 0;
    int second = 98;
    aboutStr = qApp->tr("凭证助手，版权属于小灵猫工作室所有。\n版本号：%1.%2").arg(master).arg(second);



    //获取可用屏幕尺寸
    QDesktopWidget desktop;
    screenWidth = desktop.availableGeometry().width();
    screenHeight = desktop.availableGeometry().height();

    bdb = appCfg->getBaseDbConnect();
    initSecurity();    
    appCfg->initGlobalVar();

    appCfg->readPzSetStates(pzsStates,pzsStateDescs);
    appCfg->readPingzhenClass(pzClasses);
    appCfg->readPzStates(pzStates);

    //初始化凭证状态名表
    QSqlQuery q(bdb);
    bool r;

    pzClsImps.insert(Pzc_GdzcZj);
    pzClsImps.insert(Pzc_DtfyTx);

    pzClsJzhds.insert(Pzc_Jzhd_Bank);
    pzClsJzhds.insert(Pzc_Jzhd_Ys);
    pzClsJzhds.insert(Pzc_Jzhd_Yf);
    pzClsJzhds.insert(Pzc_Jzhd_Yuf );
    pzClsJzhds.insert(Pzc_Jzhd_Yus);
    pzClsJzhds.insert(Pzc_Jzhd);

    pzClsJzsys.insert(Pzc_JzsyIn);
    pzClsJzsys.insert(Pzc_JzsyFei);

    //初始化币种表
    r = q.exec("select code,name from MoneyTypes");
    while(q.next())
        MTS[q.value(0).toInt()] = q.value(1).toString();

    //初始化固定资产产品类别表
    r = q.exec("select code,name,zjMonths from gdzc_classes");
    while(q.next()){
        int code = q.value(0).toInt();
        int zjMonths = q.value(2).toInt();
        QString name = q.value(1).toString();
        allGdzcProductCls[code] = new GdzcType(code,name,zjMonths);
    }
    return 0;
}

void appExit()
{
    AppConfig::getInstance()->saveGlobalVar();
    AppConfig::getInstance()->exit();
}

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

