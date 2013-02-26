#ifndef CONFIG_H
#define CONFIG_H

#include <QMap>
#include <QSettings>
#include <QSqlQuery>

#include "commdatastruct.h"



class VersionManager;
//通过访问应用程序的基本库来存取配置信息
class AppConfig
{
public:
    enum VarType{
        BOOL    =  1,
        INT     =  2,
        DOUBLE  =  3,
        STRING  =  4
    };


    ~AppConfig();

    static bool versionMaintain(bool &cancel);
    static AppConfig* getInstance();
    static QSqlDatabase getBaseDbConnect();

    bool readPingzhenClass(QHash<PzClass,QString>& pzClasses);
    void setUsedReportType(int accId, int rt){}
    int addAccountInfo(QString code, QString aName, QString lName, QString filename);

    int getLocalMid();

    //获取或设置配置变量的值
    bool initGlobalVar();
    bool saveGlobalVar();
    bool getConVar(QString name, bool& v);
    bool getConVar(QString name, int& v);
    bool getConVar(QString name, double& v);
    bool getConVar(QString name, QString& v);
    bool setConVar(QString name, bool value);
    bool setConVar(QString name, int value);
    bool setConVar(QString name, double value);
    bool setConVar(QString name, QString value);

    //保存或读取账户信息
    bool clear();
    bool isExist(QString code);
    bool saveAccInfo(AccountBriefInfo* accInfo);
    bool getAccInfo(int id, AccountBriefInfo* accInfo);
    bool getAccInfo(QString code, AccountBriefInfo* accInfo);
    bool readAccountInfos(QList<AccountBriefInfo*>& accs);

    //读取或设置最近打开的账户id
    bool setRecentOpenAccount(int id);
    bool getRecentOpenAccount(int& curAccId);

    static bool perfectVersion();
    static bool getVersion(int& mv, int &sv);
    static bool setVersion(int mv, int sv);

    static bool updateTo1_1();
    static bool updateTo1_2();
    static bool updateTo2_0();

private:
    AppConfig();
    bool getConfigVar(QString name, int type);
    bool setConfigVar(QString name,int type);

    bool bv;       //分别用来保存4种类型的变量值
    int iv;
    double dv;
    QString sv;
    static AppConfig* instance;
    static QSqlDatabase db;
    static QSettings *appIni;
};

/**
 * @brief The VersionManager class
 *  执行版本管理任务
 */

typedef bool (*UpdateVersionFun)();
typedef bool (*PerfectVersionFun)();
typedef bool (*GetVersionFun)(int& mv, int &sv);
typedef bool (*SetVersionFun)(int mv, int sv);


class VersionManager{
public:
    //待管理的模块类型
    enum ModuleType{
        MT_CONF = 1,    //配置模块
        MT_ACC = 2      //账户模块
    };
    VersionManager(ModuleType moduleType);
    bool versionMaintain(bool &cancel);
    void appendVersion(int mv, int sv, UpdateVersionFun fun);

private:
    void initConf();
    void initAcc();

    bool inspectVerBeforeUpdate(int mv, int sv);
    bool updateVersion(int startMv, int startSv);

    ModuleType mt;
    QStringList versionHistorys;  //历史版本号（按约定：由主版本号和次版本号组成，中间用下划线隔开）
    QHash<QString,UpdateVersionFun> updateFuns; //更新函数指针表（键为版本号，值为更新到由键指定的版本所使用的更新函数指针）
    PerfectVersionFun pvFun;  //归集到初始版本的函数
    GetVersionFun gvFun;      //获取当前版本号的函数
    SetVersionFun svFun;      //设置当前版本号的函数

    QString moduleName;       //
};


#endif // CONFIG_H

