#ifndef VERSION_H
#define VERSION_H


#include <QStringList>
#include <QHash>
#include <QSqlDatabase>

class QSettings;

const QString VM_BASIC = "vm_basic";
const QString VM_ACCOUNT = "vm_account";


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
    VersionManager(ModuleType moduleType, QString fname = "");
    bool versionMaintain(bool &cancel);
    void appendVersion(int mv, int sv, UpdateVersionFun fun);

private:
    void initConf();
    void initAcc();
    void close();

    bool inspectVerBeforeUpdate(int mv, int sv);
    bool updateVersion(int startMv, int startSv);

    ModuleType mt;
    QStringList versionHistorys;  //历史版本号（按约定：由主版本号和次版本号组成，中间用下划线隔开）
    QHash<QString,UpdateVersionFun> updateFuns; //更新函数指针表（键为版本号，值为更新到由键指定的版本所使用的更新函数指针）
    PerfectVersionFun pvFun;  //归集到初始版本的函数
    GetVersionFun gvFun;      //获取当前版本号的函数
    SetVersionFun svFun;      //设置当前版本号的函数

    QString moduleName;       //
    QString fileName;         //数据库文件名
};

class VMBase{
public:
    VMBase(){}
    static bool perfectVersion(){return true;}
    static bool getVersion(int& mv, int &sv){return true;}
    static bool setVersion(int mv, int sv){return true;}
};

class VMAccount : public VMBase
{
public:
    VMAccount(QString filename);
    static bool perfectVersion();
    static bool getVersion(int& mv, int &sv);
    static bool setVersion(int mv, int sv);

    static bool updateTo1_3();
    static bool updateTo1_4();
    static bool updateTo1_5();
    static bool updateTo1_6();

    static QSqlDatabase db;
};

class VMAppConfig : public VMBase
{
public:
    VMAppConfig();
    static bool perfectVersion();
    static bool getVersion(int& mv, int &sv);
    static bool setVersion(int mv, int sv);

    static bool updateTo1_1();
    static bool updateTo1_2();
    static bool updateTo1_3();
    static bool updateTo2_0();

    static QSqlDatabase db;
    static QSettings *appIni;
};


#endif // VERSION_H
