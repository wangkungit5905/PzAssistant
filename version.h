#ifndef VERSION_H
#define VERSION_H

#include <QStringList>
#include <QHash>
#include <QSqlDatabase>
#include <QDialog>

class QSettings;

namespace Ui {
class VersionManager;
}

class VMAccount;
class VMAppConfig;
typedef bool (VMAccount::*UpgradeFun_Acc)();
typedef bool (VMAppConfig::*UpgradeFun_Config)();


const QString VM_BASIC = "vm_basic";
const QString VM_ACCOUNT = "vm_account";
const QString VM_ACC_BACKDIR = "backupAccount";
const QString VM_ACC_BACKSUFFIX = ".BAK";

/**
 * @brief The VersionUpgradeResult enum
 *  更新进度的结果
 */
enum VersionUpgradeResult{
    VUR_OK      = 1,    //更新成功
    VUR_WARNING = 2,    //更新成功，但有未处理完的缺陷
    VUR_ERROR   = 3     //更新失败
};

enum VersionUpgradeInspectResult{
    VUIR_MUST   =   1,  //系统版本高于当前账户版本，需要升级
    VUIR_DONT   =   0,  //版本相等，不需要升级
    VUIR_LOW    =   -1, //系统版本低于当前账户版本，程序版本太低，不能打开当前账户
    VUIR_CANT   =   -2  //不能获取当前账户的版本号，因此不能确定是否可以升级，也不能打开该账户
};

class VMBase : public QObject
{
    Q_OBJECT
public:
    VMBase(){}

    virtual bool backup(QString fname){return true;}
    virtual bool restore(QString fname){return true;}
    virtual bool perfectVersion(){return true;}
    virtual void getSysVersion(int &mv, int &sv){mv =sysMv;sv=sysSv;}
    virtual void getCurVersion(int &mv, int &sv){}
    virtual bool setCurVersion(int mv, int sv){return true;}
    virtual QList<int> getUpgradeVersion();
    virtual VersionUpgradeInspectResult inspectVersion();
    virtual bool execUpgrade(int verNum){return true;}

protected:
    virtual void _getSysVersion();
    bool _inspectVerBeforeUpdate(int mv, int sv);

    int curMv,curSv,sysMv,sysSv;    //当前和系统支持的主、次版本
    bool canUpgrade;    //是否可以执行升级服务的标志（如果数据库不能连接或不能获取当前版本则不能执行升级操作）
    QList<int> versions;//系统支持的版本号（4位整数，高两位主版本号，低两位次表版本号）
    QSqlDatabase db;

signals:
    void startUpgrade(int verNum, const QString &infos);
    void endUpgrade(int verNum, const QString &infos, bool ok);
    void upgradeStep(int verNum, const QString &infos, VersionUpgradeResult result);
};

class VMAccount : public VMBase
{
    Q_OBJECT
public:
    static void getAppSupportVersion(int& mv, int& sv);

    VMAccount(QString filename);
    ~VMAccount();
    bool restoreConnect();
    void closeConnect();
    bool backup(QString fname);
    bool restore(QString fname);
    bool perfectVersion();
    void getSysVersion(int &mv, int &sv);
    void getCurVersion(int &mv, int &sv);
    bool setCurVersion(int mv, int sv);
    bool execUpgrade(int verNum);

private:
    void appendVersion(int mv, int sv, UpgradeFun_Acc upFun);
    bool _getCurVersion();    

    bool updateTo1_3();
    bool updateTo1_4();
    bool updateTo1_5();
    bool updateTo1_6();
    bool updateTo1_7();
    bool updateTo1_8();
    bool updateTo2_0();


    QHash<int,UpgradeFun_Acc> upgradeFuns;
    QString fileName;
};

class VMAppConfig : public VMBase
{
    Q_OBJECT
public:
    VMAppConfig(QString fileName);
    ~VMAppConfig();

    bool backup(QString fname);
    bool restore(QString fname);
    bool perfectVersion();
    void getSysVersion(int &mv, int &sv);
    void getCurVersion(int &mv, int &sv);
    bool setCurVersion(int mv, int sv);
    bool execUpgrade(int verNum);
private:
    void appendVersion(int mv, int sv, UpgradeFun_Config upFun);
    bool _getCurVersion();
    bool updateTo1_1();
    bool updateTo1_2();
    bool updateTo1_3();
    bool updateTo1_4();
    bool updateTo1_5();
    bool updateTo1_6();
    bool updateTo2_0();

    QHash<int,UpgradeFun_Config> upgradeFuns;
    QSettings *appIni;
};


/**
 * @brief The VersionManager class
 *  执行版本管理任务
 */

class VersionManager : public QDialog
{
    Q_OBJECT
public:
    //待管理的模块类型
    enum ModuleType{
        MT_CONF = 1,    //配置模块
        MT_ACC = 2      //账户模块
    };
    explicit VersionManager(ModuleType moduleType, QString fname = "", QWidget* parent = 0);
    ~VersionManager();
    bool versionMaintain();
    VersionUpgradeInspectResult isMustUpgrade(){return vmObj->inspectVersion();}
    bool getUpgradeResult(){return upgradeResult;}
private slots:
    void startUpgrade(int verNum, const QString &infos);
    void endUpgrade(int verNum, const QString &infos, bool ok);
    void upgradeStepInform(int verNum, const QString &infos, VersionUpgradeResult result);

    void on_btnStart_clicked();

    void on_btnClose_clicked();

    void on_btnSave_clicked();

private:
    void initConf();
    void initAcc();
    void init();
    void close();

    bool closeBtnState; //关闭按钮状态（初始时，它显示为“取消”对应值为false，当更新完成后显示为关闭，对应值为true）
    ModuleType mt;
    QString fileName;   //数据库文件名
    bool upgradeResult; //升级结果（true：成功，flase：失败）
    VMBase* vmObj;      //具体执行对应模块升级服务的对象
    QList<int> upVers;  //待升级的版本号列表
    QHash<int,QStringList> upgradeInfos; //每个版本的升级过程信息
    QString oldVersion; //升级前的版本号

    Ui::VersionManager *ui;
};

#endif // VERSION_H
