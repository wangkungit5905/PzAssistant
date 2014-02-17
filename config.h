#ifndef CONFIG_H
#define CONFIG_H

#include <QMap>
#include <QSettings>
#include <QSqlQuery>

#include "commdatastruct.h"
#include "common.h"
#include "logs/Logger.h"


class VersionManager;
class Machine;
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

    /**
     * @brief 在程序内部有特殊处理的科目
     *
     */
    enum SpecSubCode{
        SSC_CASH    = 1,    //现金
        SSC_BANK    = 2,    //银行
        SSC_GDZC    = 3,    //固定资产
        SSC_CWFY    = 4,    //财务费用
        SSC_BNLR    = 5,    //本年利润
        SSC_LRFP    = 6,    //利润分配
        SSC_YS      = 7,    //应收账款
        SSC_YF      = 8     //应付账款
    };

    /**
     * @brief 在程序内部有特殊处理的名称类别枚举
     */
    enum SpecNameItemClass{
        SNIC_CLIENT = 1,    //业务客户类
        SNIC_GDZC   = 2,    //固定资产类
        SNIC_BANK   = 3     //金融机构（常指银行）
    };




    ~AppConfig();

    static AppConfig* getInstance();
    static QSqlDatabase getBaseDbConnect();

    void exit();
    Logger::LogLevel getLogLevel();
    void setLogLevel(Logger::LogLevel level);
    bool readPingzhenClass(QHash<PzClass,QString>& pzClasses);
    bool readPzStates(QHash<PzState, QString> &names);
    bool readPzSetStates(QHash<PzsState,QString>& snames, QHash<PzsState,QString>& lnames);
    void setUsedReportType(int accId, int rt){}
    int addAccountInfo(QString code, QString aName, QString lName, QString filename);

    int getSpecNameItemCls(SpecNameItemClass witch);
    void setSpecNameItemcls(SpecNameItemClass witch, int code);
    bool isSpecSubCodeConfiged(int subSys);
    QString getSpecSubCode(int subSys, SpecSubCode witch);
    void setSpecSubCode(int subSys, SpecSubCode witch, QString code);
    QHash<int,SubjectClass> getSubjectClassMaps(int subSys);

    QHash<MachineType,QString> getMachineTypes();
    Machine* getLocalMachine();
    Machine* getMachine(int id){return machines.value(id);}
    QHash<int,Machine*> getAllMachines(){return machines;}
    bool saveMachine(Machine* mac);
    bool saveMachines(QList<Machine*> macs);

    //子窗口状态信息存取方法
    bool readPzEwTableState(QList<int> &infos);

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

    //保存或读取本地账户缓存信息
    bool clearAccountCache();
    bool isExist(QString code);
    bool refreshLocalAccount(int& count);
    bool saveAccountCacheItem(AccountCacheItem* accInfo);
    bool saveAllAccountCaches();
    AccountCacheItem* getAccountCacheItem(QString code);
    QList<AccountCacheItem *> getAllCachedAccounts();
    AccountCacheItem *getRecendOpenAccount();
    void setRecentOpenAccount(QString code);
    QHash<AccountTransferState,QString> getAccTranStates();

    bool getSubSysItems(QList<SubSysNameItem *> &items);

    bool getSupportMoneyType(QHash<int, Money *> &moneys);

private:
    bool _isValidAccountCode(QString code);
    bool _saveAccountCacheItem(AccountCacheItem* accInfo);
    bool _searchAccount();
    bool _initAccountCaches();
    void _initMachines();
    void _initSpecSubCodes();
    bool _saveMachine(Machine* mac);
    AppConfig();
    bool getConfigVar(QString name, int type);
    bool setConfigVar(QString name,int type);

    QHash<int, Money*> moneyTypes;
    QHash<int, Machine*> machines;
    QList<AccountCacheItem*> accountCaches;
    QHash<int, QHash<SpecSubCode,QString> > specCodes; //特定科目代码表
    bool init_accCache; //本地账户缓存条目是否已从缓存表中读取的标志
    bool bv;       //分别用来保存4种类型的变量值
    int iv;
    double dv;
    QString sv;

    static AppConfig* instance;
    static QSqlDatabase db;
    static QSettings *appIni;
    static const int SPEC_SUBJECT_NUMS = 8; //特定科目数目
};



#endif // CONFIG_H

