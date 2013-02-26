#ifndef SECURITYS_H
#define SECURITYS_H

#include <QString>
#include <QStringList>
#include <QSet>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QSqlQuery>
#include <QVariant>

//权限类
class Right
{
public:
    //权限代码
    enum RightCode{
    //1、软件管理配置类（执行与会计业务本身无关的软件配置任务所需的权限）(1-50)
    //2、账户管理类：（51-100）
    //    账户生命期管理：（51-60）

    CreateAccount   = 51,       //创建账户（51）
    DelAccount      = 52,       //删除账户（52）
    ImportAccount   = 53,       //导入账户（53）
    ExportAccount   = 54,       //导出账户（54）
    RefreshAccount  = 55,       //刷新账户列表（55）

    //账户配置：（61-80）
    SetAccCommonInfo =    61,  //设置账户一般信息（61--账户名、全名，全局账户代码）
    SetAccSensitiveInfo = 62,  //设置账户敏感信息（62--比如开户行帐号等）；
    SetUsedSubject      = 63,  //所用科目系统的设定（63--使用新/旧科目系统）；
    ConfigFstSubject    = 64,  //一级科目的配置（64--比如配置实际使用的一级科目，过滤掉从不使用的科目）；
    ConfigSndSubject    = 65,  //二级科目的配置（65--增、删、改二级科目以及建立与一级科目的属从关系）；

    //配置账户的基准数据：（81-89）
    ConfigAccStandandTime = 81,// 配置账户的基准时间（81）
    ConfigAccPeriodBegin  = 82,//配置账户的期初数（82）

    //3：账户操作类：（101 - 150）
    OpenAccount     = 101,     //打开账户（101）
    CloseAccount    = 102,     //关闭账户（102）

    //4、业务操作类：（151 - 250）
    //    会计业务类：
    //        凭证集操作类：
    OpenPzs      = 151,        //打开凭证集（151）
    ClosePzs     = 152,        //关闭凭证集（152）

    //        一般凭证操作类：
    AddPz        = 155,        //新增凭证（155）
    DelPz        = 156,        //删除凭证（156）
    EditPz       = 157,        //修改凭证（157）
    ViewPz       = 158,        //查看凭证（158）

//            高级凭证操作类：
    RepealPz     = 201,        //凭证作废（201）
    InstatPz     = 202,        //凭证入账（202）
    VerifyPz     = 211,        //审核凭证（211）
    //VerifyPz2    = 221,        //审核凭证2（审核软件自动生成的结转凭证）（221）
//                结账
//                反审核1
//                反审核2
//                反结账
//                保存期末余额；

    //5、统计类：（250 - 300）
    //    查看明细/总分类帐；
    //    查看统计表；

    //6、打印类： （301 - 350）
    PrintPz           = 301,    //打印凭证（301）；
    PrintDetialTable  = 302,    //打印明细帐和总分类帐（302）
    PrintStatTable    = 303,    //打印统计表（303）

    //7、数据导出类：（351 - 400）
    //    导出明细帐和总分类帐数据；
    //    导出统计表数据；

    };

    Right(int code, int type, QString name, QString explain = "");
    void setType(int t);
    int getType();
    void setCode(int c);
    int getCode();
    void setName(QString name);
    QString getName();
    void setExplain(QString explain);
    QString getExplain();

private:
    int code;              //权限代码
    int type;              //权限类别代码
    QString name, explain; //权限名称和解释
};

inline bool operator==(Right &e1, Right &e2)
{
    return (e1.getCode() == e2.getCode());
}

//用户组类
class UserGroup
{
public:
    UserGroup(int code, QString name, QSet<Right*> haveRights = QSet<Right*>());
    QString getName();
    void setName(QString name);
    QSet<Right*> getHaveRights();
    void setHaveRights(QSet<Right*> rights);
    void addRight(Right* right);
    void delRight(Right* right);

private:
    int code;
    QString name;
    QSet<Right*> rights;
};

//用户类
class User
{
public:
    User(int id, QString name, QString password = "", QSet<UserGroup*> ownerGroups = QSet<UserGroup*>());
    int getUserId(){return id;}
    QString getName();
    void setName(QString name);
    void setPassword(QString password);
    bool verifyPw(QString password);
    QSet<UserGroup*> getOwnerGroups();
    void setOwnerGroups(QSet<UserGroup*> ownerGroups);
    void addGroup(UserGroup* group);
    void delGroup(UserGroup* group);
    QSet<Right*> getAllRight();
    bool haveRight(Right* right);
    bool haveRights(QSet<Right*> rights);

    static QString encryptPw(QString pw){return pw;} //默认实现不对密码进行加密

private:
    void refreshRights();

    int id;
    QString name;
    QString password;
    QSet<UserGroup*> groups;   //用户所属组
    QSet<Right*> rights;
};


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

extern QHash<int,User*> allUsers;
extern QHash<int,QString> allRightTypes; //权限类别
extern QHash<int,Right*> allRights;
extern QHash<int,UserGroup*> allGroups;
extern QHash<int,Operate*> allOperates;
//extern QSqlDatabase bdb;  //基本数据库连接

extern bool initSecurity();

#endif // SECURITYS_H
