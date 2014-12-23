#ifndef CONFIG_H
#define CONFIG_H

#include <QMap>
#include <QSqlQuery>

#include "commdatastruct.h"
#include "common.h"
#include "logs/Logger.h"

class QSettings;
class VersionManager;
class Machine;
struct ExternalToolCfgItem;

//struct ExternalToolCfgItem{
//    QString name;       //外部工具名称
//    QString cmd;        //启动命令
//    QStringList paras;  //命令的参数列表
//};

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
        SSC_YF      = 8,    //应付账款
        SSC_YJSJ    = 9,    //应交税金
        SSC_ZYSR    = 10,   //主营业务收入
        SSC_ZYCB    = 11    //主营业务成本
    };

    /**
     * @brief 在程序内部有特殊处理的名称类别枚举
     */
    enum SpecNameItemClass{
        SNIC_COMMON_CLIENT = 1,    //普通业务客户类
        SNIC_WL_CLIENT     = 2,    //物流企业
        SNIC_GDZC          = 3,    //固定资产类
        SNIC_BANK          = 4     //金融机构（常指银行）
    };

    /**
     * @brief 配置变量对应的枚举代码
     * 并作为存放变量值的数组索引
     */
    enum CfgValueCode{
        CVC_RuntimeUpdateExtra = 0,     //是否实时更新余额
        CVC_ExtraUnityInspectAfterRead=1, //是否在读取余额后进行一致性检测
        CVC_ExtraUnityInspectBeforeSave=2, //是否在保存余额前进行一致性检测
        CVC_ViewHideColInDailyAcc1 = 3, //是否在日记账表格中显示隐藏列（包括对方科目、结算号等）
        CVC_ViewHideColInDailyAcc2 = 4, //是否在日记账表格中显示隐藏列（包括凭证id、会计分录id等）
        CVC_IsByMtForOppoBa = 5,        //在按下等号键建立新的合计对冲业务活动时，是否按币种分开建立
        CVC_IsCollapseJz = 6,           //是否展开结转凭证中的业务活动表项的明细，默认展开。
        CVC_JzlrByYear = 7,             //是否每年年底执行一次本年利润的结转
        //整形
        CVC_ResentLoginUser = 0,        //最近登录用户id
        CVC_AutoSaveInterval = 1,       //凭证自动保存的时间间隔
        CVC_TimeoutOfTemInfo = 2,       //在状态条上显示临时信息的超时时间（以毫秒计）
        //双精度型
        CVC_GdzcCzRate = 0,            //固定资产折旧残值率
    };

    //凭证打印模板参数名
    const QString SEGMENT_DEBUG = "Debug";
    const QString SEGMENT_PZ_TEMPLATE = "PzTemplate";
    const QString KEY_PZT_BAROWHEIGHT = "BaRowHeight";
    const QString KEY_PZT_BATITLEHEIGHT = "BaTitleHeight";
    const QString KEY_PZT_BAROWNUM = "BaRowNum";
    const QString KEY_PZT_CUTAREA = "CutAreaHeight";
    const QString KEY_PZT_LR_MARGIN = "LeftRightWidth";
    const QString KEY_PZT_TB_MARGIN = "TopBottonHeight";
    const QString KEY_PZT_BATABLE_FACTOR = "AllocateFactor";
    const QString KEY_PZT_FONTSIZE = "FontSize";
    const QString KEY_PZT_ISPRINTCUTLINE = "IsPrintCutLine";
    const QString KEY_PZT_ISPRINTMIDLINE = "IsPrintMidLine";

    //各种目录记录键名
    const QString SEGMENT_DIR = "Directorys";
    enum DirectoryName{
        DIR_TRANSOUT    = 1,    //最近转出账户操作时所选择的目录
        DIR_TRANSIN     = 2     //最近转入账户操作时所选择的目录
    };

    //记录站点信息
    const QString SEGMENT_STATIONS = "Stations";
    const QString KEY_STATION_MSID = "masterStationId";
    const QString KEY_STATION_LOID = "localStationId";

    ~AppConfig();

    static AppConfig* getInstance();
    static QSqlDatabase getBaseDbConnect();

    void exit();
    Logger::LogLevel getLogLevel();
    void setLogLevel(Logger::LogLevel level);
    QString getAppStyleName();
    void setAppStyleName(QString styleName);
    bool getStyleFrom();
    void setStyleFrom(bool fromRes=true);
    bool readPingzhenClass(QHash<PzClass,QString>& pzClasses);
    bool readPzStates(QHash<PzState, QString> &names);
    bool readPzSetStates(QHash<PzsState,QString>& snames, QHash<PzsState,QString>& lnames);
    void setUsedReportType(int accId, int rt){}
    int addAccountInfo(QString code, QString aName, QString lName, QString filename);

    int getSpecNameItemCls(SpecNameItemClass witch);
    void setSpecNameItemcls(SpecNameItemClass witch, int code);
    bool isSpecSubCodeConfiged(int subSys);
    QString getSpecSubCode(int subSys, SpecSubCode witch);
    QHash<SpecSubCode,QString> getAllSpecSubCodeForSubSys(int subSys);
    bool setSpecSubCode(int subSys, SpecSubCode witch, QString code);
    QHash<int,SubjectClass> getSubjectClassMaps(int subSys);

    Machine* getMasterStation();
    QHash<MachineType,QString> getMachineTypes();
    Machine* getLocalStation();
    int getLocalStationId(){return localId;}
    Machine* getMachine(int id){return machines.value(id);}
    QHash<int,Machine*> getAllMachines(){return machines;}
    bool refreshMachines(){return _initMachines();}
    bool saveMachine(Machine* mac);
    bool saveMachines(QList<Machine*> macs);
    bool removeMachine(Machine* mac);
    bool getOsTypes(QHash<int, QString> &types);

    bool getPzTemplateParameter(PzTemplateParameter* parameter);
    bool savePzTemplateParameter(PzTemplateParameter* parameter);

    QString getDirName(DirectoryName witch);
    void saveDirName(DirectoryName witch,QString dir);

    //子窗口状态信息存取方法
    bool getSubWinInfo(int winEnum, SubWindowDim *&info, QByteArray *sinfo=0);
    bool saveSubWinInfo(int winEnum, SubWindowDim *info, QByteArray *sinfo=0);
    bool readPzEwTableState(QList<int> &infos);

    //获取或设置配置变量的值

    bool saveGlobalVar();
    void getCfgVar(CfgValueCode varCode, bool& v);
    void getCfgVar(CfgValueCode varCode, int& v);
    void getCfgVar(CfgValueCode varCode, Double &v);
    void setCfgVar(CfgValueCode varCode, bool v);
    void setCfgVar(CfgValueCode varCode, int v);
    void setCfgVar(CfgValueCode varCode, double v);

    //保存或读取本地账户缓存信息
    bool clearAccountCache();
    bool isExist(QString code);
    bool refreshLocalAccount(int& count);
    //bool addAccountCacheItem(AccountCacheItem* accItem);
    bool saveAccountCacheItem(AccountCacheItem* accInfo);
    bool saveAllAccountCaches();
    bool removeAccountCache(AccountCacheItem* accInfo);
    AccountCacheItem* getAccountCacheItem(QString code);
    QList<AccountCacheItem *> getAllCachedAccounts();
    AccountCacheItem *getRecendOpenAccount();
    void setRecentOpenAccount(QString code);
    void clearRecentOpenAccount();
    QHash<AccountTransferState,QString> getAccTranStates();

    void getSubSysItems(QList<SubSysNameItem *> &items);
    SubSysNameItem* getSpecSubSysItem(int subSysCode);

    bool getSupportMoneyType(QHash<int, Money *> &moneys);

    void updateTableCreateStatment(QStringList names, QStringList sqls);
    bool getUpdateTableCreateStatment(QStringList &names, QStringList &sqls);
    bool setEnabledFstSubs(int subSys, QStringList codes);
    bool setSubjectJdDirs(int subSys, QStringList codes);    
    bool getSubSysMaps2(int scode, int dcode, QHash<QString,QString> &defMaps, QHash<QString, QString> &multiMaps);
    bool getSubSysMaps(int scode, int dcode, QList<SubSysJoinItem2*>& cfgs);
    bool saveSubSysMaps(int scode, int dcode, QList<SubSysJoinItem2*> cfgs);
    bool getNotDefSubSysMaps(int scode, int dcode, QHash<QString,QString>& codeMaps);
    bool getSubSysMapConfiged(int scode,int dcode, bool &ok);
    bool setSubSysMapConfiged(int scode,int dcode, bool ok = true);
    bool getSubCodeToNameHash(int subSys, QHash<QString,QString>& subNames);

    bool readAllExternalTools(QList<ExternalToolCfgItem*> &items);
    bool saveExternalTool(ExternalToolCfgItem* item, bool isDelete=false);

    //安全模块需要的方法
    bool initRights(QHash<int,Right*>& rights);
    bool initUsers(QHash<int,User*>& users);
    bool saveUser(User* u, bool isDelete=false);
    bool restorUser(User* u);
    bool initRightTypes(QHash<int, RightType *> &types);
    bool initUserGroups(QHash<int,UserGroup*>& groups);
    bool saveUserGroup(UserGroup* g, bool isDelete=false);
    bool restoreUserGroup(UserGroup* g);
    void getAppCfgTypeNames(QHash<BaseDbVersionEnum, QString> &names);
    bool setAppCfgVersion(BaseDbVersionEnum verType, int mv, int sv);
    bool getAppCfgVersion(int &mv, int &sv, BaseDbVersionEnum vType);
    bool getAppCfgVersions(QList<BaseDbVersionEnum> &verTypes, QStringList &verNames,
                          QList<int> &mvs, QList<int> &svs);

    //批量保存安全模块设置信息方法
    bool clearAndSaveUsers(QList<User*> users,int mv,int sv);
    bool clearAndSaveGroups(QList<UserGroup*> groups,int mv,int sv);
    bool clearAndSaveMacs(QList<Machine *> macs,int mv,int sv);
    bool clearAndSaveRights(QList<Right*> rights,int mv,int sv);
    bool clearAndSaveRightTypes(QList<RightType*> rightTypes,int mv,int sv);
private:
    bool _isValidAccountCode(QString code);
    bool _saveAccountCacheItem(AccountCacheItem* accInfo);
    bool _searchAccount();
    bool _initCfgVars();
    void _initCfgVarDefs();
    bool _initAccountCaches();
    bool _initMachines();
    bool _initSubSysNames();
    bool _initSpecSubCodes();
    bool _initSpecNameItemClses();
    bool _saveMachine(Machine* mac);
    QString _getKeyNameForDir(DirectoryName witch);
    AppConfig();

    QHash<int, Money*> moneyTypes;
    QHash<int, Machine*> machines;
    int msId;   //主站标识
    int localId; //本站标识
    QHash<int, SubSysNameItem*> subSysNames;
    QList<AccountCacheItem*> accountCaches;
    QHash<int, QHash<SpecSubCode,QString> > specCodes; //特定科目代码表
    bool init_accCache; //本地账户缓存条目是否已从缓存表中读取的标志

    //特定名称类别的代码
    QHash<SpecNameItemClass,int> specNICs;

    //应用行为配置变量
    static const int CFGV_BOOL_NUM = 8;     //布尔型配置变量个数
    static const int CFGV_INT_NUM = 3;      //整形配置变量个数
    static const int CFGV_DOUBLE_NUM = 1;   //双精度型配置变量个数
    bool boolCfgs[CFGV_BOOL_NUM];
    int intCfgs[CFGV_INT_NUM];
    Double doubleCfgs[CFGV_DOUBLE_NUM];

    static AppConfig* instance;
    static QSqlDatabase db;
    static QSettings *appIni;
    static const int SPEC_SUBJECT_NUMS = 8; //特定科目数目


};



#endif // CONFIG_H

