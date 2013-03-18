#ifndef CONFIG_H
#define CONFIG_H

#include <QMap>
#include <QSettings>
#include <QSqlQuery>

#include "commdatastruct.h"
#include "common.h"
#include "logs/Logger.h"


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

    static AppConfig* getInstance();
    static QSqlDatabase getBaseDbConnect();

    Logger::LogLevel getLogLevel();
    void setLogLevel(Logger::LogLevel level);
    bool readPingzhenClass(QHash<PzClass,QString>& pzClasses);
    bool readPzStates(QHash<PzState, QString> &names);
    bool readPzSetStates(QHash<PzsState,QString>& snames, QHash<PzsState,QString>& lnames);
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
    bool saveAccInfo(AccountBriefInfo accInfo);
    bool getAccInfo(int id, AccountBriefInfo &accInfo);
    bool getAccInfo(QString code, AccountBriefInfo accInfo);
    bool readAccountInfos(QList<AccountBriefInfo*>& accs);

    //读取或设置最近打开的账户id
    bool setRecentOpenAccount(int id);
    bool getRecentOpenAccount(int& curAccId);

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



#endif // CONFIG_H

