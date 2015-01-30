#ifndef SECURITYS_H
#define SECURITYS_H

#include <QString>
#include <QStringList>
#include <QSet>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QSqlQuery>
#include <QVariant>

#include "commdatastruct.h"

static int USER_GROUP_ROOT_ID = 1;        //超级用户组ID
static int USER_GROUP_ADMIN_ID = 2;       //管理员组ID

class Account;

struct RightType;
//权限类别
struct RightType{
    int code;               //类别代码
    RightType* pType;        //父类别
    QString name,explain;   //名称和简介

    static RightType* serialFromText(QString serialText, const QHash<int, RightType *> &rightTypes);
    QString serialToText();
    static void serialAllToBinary(int mv, int sv, QByteArray* ds);
    static bool serialAllFromBinary(QList<RightType*> &rts, int &mv, int &sv, QByteArray* ds);
};
bool rightTypeByCode(RightType* rt1, RightType* rt2);

//权限类
class Right
{
public:
    //权限代码
    enum RightCode{
    //1、软件管理配置类（执行与会计业务本身无关的软件配置任务所需的权限）(1-40)

    //2、数据库访问类（以数据库的方式访问账户文件）（8）（41）
    Database_Access  = 41,

    //2、账户管理类：（51-100）
    //账户生命期管理（2-21）：（51-60）
    Account_Create   = 51,       //创建账户（51）
    Account_Remove   = 52,       //移除账户（52）
    Account_Import   = 53,       //导入账户（53）
    Account_Export   = 54,       //导出账户（54）
    Account_Refresh  = 55,       //刷新账户列表（55）

    //账户配置（2-22）：（61-80）
    Account_Config_SetCommonInfo    = 61,  //设置账户一般信息（61--账户名、全名，全局账户代码）
    Account_Config_SetSensitiveInfo = 62,  //设置账户敏感信息（62--比如开户行帐号等）；
    Account_Config_SetUsedSubSys    = 63,  //所用科目系统的设定（63--使用新/旧科目系统）；
    Account_Config_SetFstSubject    = 64,  //一级科目的配置（64--比如配置实际使用的一级科目，过滤掉从不使用的科目）；
    Account_Config_SetSndSubject    = 65,  //二级科目的配置（65--增、删、改二级科目以及建立与一级科目的属从关系）；

    //配置账户的基准数据（2-23）：（81-89）
    Account_Config_SetBaseTime      = 81,  // 配置账户的基准时间（81）
    Account_Config_SetPeriodBegin   = 82,  //配置账户的期初数（82）

    //3：账户操作类（3）：（101 - 150）
    Account_Common_Open     = 101,     //打开账户（101）
    Account_Common_Close    = 102,     //关闭账户（102）

    //4、业务操作类：（151 - 250）
    //    会计业务类：
    //凭证集基本操作类（41）：
    PzSet_Common_Open    = 151,        //打开凭证集（151）
    PzSet_Common_Close   = 152,        //关闭凭证集（152）

    //一般凭证操作类（42）：
    Pz_common_Add        = 155,        //新增凭证（155）
    Pz_Common_Del        = 156,        //删除凭证（156）
    Pz_Common_Edit       = 157,        //修改凭证（157）
    Pz_Common_Show       = 158,        //查看凭证（158）

    //高级凭证操作类（43）：（201-210）
    Pz_Advanced_Repeal     = 201,        //凭证作废（201）
    Pz_Advanced_Instat     = 202,        //凭证入账（202）
    Pz_Advanced_Verify     = 203,        //审核凭证（203）
    Pz_Advanced_AntiVerify = 204,        //取消审核凭证（204）
    Pz_Advanced_JzHdsy     = 205,        //创建结转汇兑损益凭证（205）
    Pz_Advanced_JzSy       = 206,        //创建结转损益凭证（206）
    Pz_Advanced_jzlr       = 207,        //创建结转利润凭证（207）


    //高级凭证集操作（44）
    PzSet_Advance_ShowExtra     = 211,  //查看余额（211）
    PzSet_Advance_SaveExtra     = 212,  //保存统计值（212）
    PzSet_Advance_EndSet        = 213,  //凭证集结账（213）
    PzSet_Advance_AntiEndSet    = 214,  //反结账（214）

    //帐套操作（47）
    Suite_EndSuite      = 241,  //关账（帐套关账）（241）
    Suite_AntiEndSuite  = 242,  //反关账（242）
    Suite_New           = 243,  //新开帐套（243）
    Suite_Edit          = 244,  //编辑帐套(244)

    //5、统计类（45）：（251 - 300）
    PzSet_ShowStat_Details  = 251,      //查看明细账
    PzSet_ShowStat_Totals   = 252,      //查看总分类账
    PzSet_ShowStat_Current  = 253,      //查看本期统计

    //6、打印类（46）： （301 - 350）
    Print_Pz           = 301,    //打印凭证（301）；
    Print_DetialTable  = 302,    //打印明细帐和总分类帐（302）
    Print_StatTable    = 303,    //打印统计表（303）

    //7、数据导出类：（351 - 400）
    //    导出明细帐和总分类帐数据；
    //    导出统计表数据；

    };

    Right(int code, RightType* type, QString name, QString explain = "");
    void setType(RightType* t);
    RightType *getType();
    void setCode(int c);
    int getCode();
    void setName(QString name);
    QString getName();
    void setExplain(QString explain);
    QString getExplain();

    QString serialToText();
    static Right* serialFromText(QString serialText, const QHash<int, RightType *> &rightTypes);
    static void serialAllToBinary(int mv, int sv, QByteArray* ds);
    static bool serialAllFromBinary(const QHash<int, RightType *> &rightTypes, QList<Right*> &rs, int &mv, int &sv, QByteArray* ds);

private:
    int code;              //权限代码
    RightType* type;              //权限类别代码
    QString name, explain; //权限名称和解释
};
Q_DECLARE_METATYPE(Right*)
bool rightByCode(Right* r1, Right *r2);


inline bool operator==(Right &e1, Right &e2)
{
    return (e1.getCode() == e2.getCode());
}

//用户组类
class UserGroup
{
public:
    UserGroup(int id, int code, QString name, QSet<Right*> haveRights = QSet<Right*>());
    int getGroupId(){return id;}
    QString getName();
    void setName(QString name);
    QString getExplain(){return explain;}
    void setExplain(QString explain){this->explain=explain;}
    QSet<Right*> getExtraRights();
    QSet<Right*> getAllRights();
    QString getRightCodeList();
    void setHaveRights(QSet<Right*> rights);
    void addRight(Right* right);
    void removeRight(Right* right);
    void addGroup(UserGroup* g){ownerGroups.insert(g);}
    void removeGroup(UserGroup* g){ownerGroups.remove(g);}
    QSet<UserGroup*> getOwnerGroups(){return ownerGroups;}
    void setOwnerGroups(QSet<UserGroup*> gs){ownerGroups = gs;}
    QString getOwnerCodeList();
    bool isGroupRight(Right* r);
    int getGroupCode();
    bool hasRight(Right* r);

    QString serialToText();
    static UserGroup* serialFromText(QString serialText,const QHash<int, Right*> &rights, const QHash<int,UserGroup*> &groups);
    static void serialAllToBinary(int mv, int sv, QByteArray* ds);
    static bool serialAllFromBinary(QList<UserGroup*> &groups, int &mv, int &sv, QByteArray* ds,const QHash<int, Right*> &rights);

private:
    int id;
    int code;                       //组代码
    QString name,explain;           //组名，组说明信息
    QSet<Right*> rights;            //所拥有的权限集（不包括其所属的其他组所拥有的权限，即组的额外）
    QSet<UserGroup*> ownerGroups;   //所属组

    friend class AppConfig;
};
Q_DECLARE_METATYPE(UserGroup*)
bool groupByCode(UserGroup* g1, UserGroup* g2);

//用户类
class User
{
public:
    User(int id, QString name, QString password = "", QSet<UserGroup*> ownerGroups = QSet<UserGroup*>());
    int getUserId(){return id;}
    bool isEnabled(){return enabled;}
    void setEnabled(bool en){enabled=en;}
    QString getName();
    void setName(QString name);
    void setPassword(QString password);
    QString getPassword(){return password;}
    bool verifyPw(QString password);
    QSet<UserGroup*> getOwnerGroups();
    void setOwnerGroups(QSet<UserGroup*> ownerGroups);
    QString getOwnerGroupCodeList();
    void addGroup(UserGroup* group){groups.insert(group);}
    void delGroup(UserGroup* group){groups.remove(group);}
    bool isDisabledRight(Right* r){return disRights.contains(r);}
    QSet<Right*> getAllDisRights(){return disRights;}
    void setAllDisRights(QSet<Right*> rs){disRights=rs;}
    void addDisRight(Right* r){disRights.insert(r);}
    void removeDisRight(Right* r){disRights.remove(r);}
    void clearDisRights(){disRights.clear();}
    QString getDisRightCodes();
    QSet<Right*> getAllRights();
    bool haveRight(Right* right);
    bool haveRights(QSet<Right*> rights);
    bool isSuperUser();
    bool isAdmin();
    bool canAccessAccount(Account* account);
    void addExclusiveAccount(QString accountCode){accountCodes.insert(accountCode);}
    void removeExclusiveAccount(QString accountCode){accountCodes.remove(accountCode);}
    QStringList getExclusiveAccounts();
    void setExclusiveAccounts(QStringList codes);

    QString serialToText();
    static User* serialFromText(QString serialText,const QHash<int,Right*> &rights, const QHash<int,UserGroup*> &groups);
    static void serialAllToBinary(int mv, int sv, QByteArray* ds);
    static bool serialAllFromBinary(QList<User*> &users, int &mv, int &sv, QByteArray* ds,const QHash<int,Right*> &rights, const QHash<int,UserGroup*> &groups);

    static QString encryptPw(QString pw){return pw;} //默认实现不对密码进行加密
    static QString decryptPw(QString pw){return pw;}

private:
    int id;
    bool enabled;              //是否启用该用户
    QString name;
    QString password;
    QSet<UserGroup*> groups;   //用户所属组
    QSet<Right*> disRights;       //用户拥有的所有权限
    //QSet<Right*> extraRights;   //额外权限
    QSet<QString> accountCodes;//专属账户代码集合

    friend class AppConfig;
};
Q_DECLARE_METATYPE(User*)
bool userByCode(User* u1, User* u2);

////////////////////////////////////////////////////////////////////////////


//操作类
class Operate
{
public:
    Operate(int code, QString name, QSet<Right*> rights = QSet<Right*>());
    void setCode(int c);
    int getCode();
    void setName(QString name);
    QString getName();
    QSet<Right*> getRights();
    void setRight(QSet<Right*> rights);

    bool isPermition(User* user);

private:
    int code;
    QString name;
    QSet<Right*> rights;
};

//工作站类
class WorkStation
{
public:
    WorkStation(int id, MachineType type,int mid,bool isLocal,QString name,QString desc,int osType=1);
    WorkStation(WorkStation& other);
    int getId(){return id;}
    int getMID(){return mid;}
    void setMID(int id){mid=id;}
    MachineType getType(){return type;}
    void setType(MachineType type){this->type=type;}
    bool isLocalStation(){return isLocal;}
    void setLocalMachine(bool local){isLocal=local;}
    QString name(){return sname;}
    void setName(QString name){sname=name;}
    QString description(){return desc;}
    void setDescription(QString desc){this->desc=desc;}
    int osType(){return _osType;}
    void setOsType(int type){_osType = type;}

    QString serialToText();
    static WorkStation* serialFromText(QString serialText);
    static void serialAllToBinary(int mv, int sv, QByteArray* ds);
    static bool serialAllFromBinary(QList<WorkStation*> &macs, int &mv, int &sv, QByteArray* ds);

    bool operator ==(const WorkStation &other) const;
    bool operator !=(const WorkStation &other) const;
private:
    int id;
    MachineType type;   //主机类型（电脑(1)、云(2)）
    int mid;            //主机标识
    bool isLocal;       //是否是本机
    QString sname;      //主机简称
    QString desc;       //主机全称（或描述信息）
    int _osType;         //宿主操作系统类型

    friend class AppConfig;
};
Q_DECLARE_METATYPE(WorkStation*)
bool byMacMID(WorkStation *mac1, WorkStation *mac2);

extern QHash<int,User*> allUsers;
extern QHash<int,RightType*> allRightTypes; //权限类别
extern QHash<int,Right*> allRights;
extern QHash<int,UserGroup*> allGroups;
extern QHash<int,Operate*> allOperates;

extern bool initSecurity();

#endif // SECURITYS_H
