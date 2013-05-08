#ifndef DBUTIL_H
#define DBUTIL_H

#include <QSqlDatabase>


#include "account.h"



/**
 * @brief The DbUtil class
 *  提供对账户数据库的直接访问支持，所有其他的类要访问账户数据库，必须通过此类
 */

const QString AccConnName = "Account";
const int nmv = 1;  //这是新老余额存储机制转换的衔接版本号
const int nsv = 6;  //此版本号前使用老存储机制，此版本后（包括此版本）使用新存储机制

class SubjectManager;
class FirstSubject;
class PingZheng;

class DbUtil
{
public:
    enum InfoField{
        ACODE     = 1,           //账户代码
        SNAME     = 2,           //账户简称
        LNAME     = 3,           //账户全称
        //RPTTYPE   = 6,           //报表类型
        MASTERMT  = 7,           //本币代码
        WAIMT     = 8,           //外币代码列表
        STIME     = 9,           //账户记账起始时间
        ETIME     = 10,          //账户记账终止时间（当前账户最后记账时间）
        CSUITE    = 11,          //账户当前帐套年份
        SUITENAME = 12,          //帐套名列表
        LASTACCESS= 13,          //账户最后访问时间
        LOGFILE   = 50,          //与该账户相关的日志文件名
        DBVERSION = 51           //账户文件的版本号（用来表示数据库内表格的变动）
    };

    //数据库操作错误代码
    enum ErrorCode{
        Transaction_open    = 1,    //打开事务
        Transaction_commit  = 2,    //提交事务
        Transaction_rollback  = 3     //回滚事务
    };

    DbUtil();
    ~DbUtil();
    bool setFilename(QString fname);
    bool init();
    void close();

    //账户信息相关
    bool readAccBriefInfo(AccountBriefInfo& info);
    bool initAccount(Account::AccountInfo &infos);
    bool saveAccountInfo(Account::AccountInfo &infos);

    //科目相关
    bool initNameItems();
    bool initSubjects(SubjectManager* smg, int subSys);
    bool saveNameItem(SubjectNameItem* ni);
    bool saveSndSubject(SecondSubject* sub);
    bool savefstSubject(FirstSubject* fsub);

    //货币相关
    bool initMoneys(Account* account);
    bool initBanks(Account* account);

    //凭证数统计
    bool scanPzSetCount(int y, int m, int &repeal, int &recording, int &verify, int &instat, int &amount);

    //余额相关
    bool readExtraForPm(int y,int m, QHash<int,Double>& fsums,
                                     QHash<int,MoneyDirection>& fdirs,
                                     QHash<int,Double>& ssums,
                                     QHash<int,MoneyDirection>& sdirs);
    bool readExtraForMm(int y,int m, QHash<int,Double>& fsums,
                                     QHash<int,Double>& ssums);
    bool saveExtraForPm(int y, int m, const QHash<int, Double>& fsums,
                                      const QHash<int, MoneyDirection>& fdirs,
                                      const QHash<int, Double>& ssums,
                                      const QHash<int, MoneyDirection>& sdirs);
    bool saveExtraForMm(int y, int m, const QHash<int, Double>& fsums,
                                      const QHash<int, Double>& ssums);

    //日记账
    bool getDetViewFilters(QList<DVFilterRecord*>& rs);
    bool saveDetViewFilter(const QList<DVFilterRecord *> &dvf);
    bool getDailyAccount2(QHash<int,SubjectManager*> smgs, QDate sd, QDate ed, int fid, int sid, int mt,
                            Double& prev, int& preDir,
                            QList<DailyAccountData2*>& datas,
                            QHash<int,Double>& preExtra,
                            QHash<int,Double>& preExtraR,
                            QHash<int,int>& preExtraDir,
                            QHash<int, Double>& rates,
                            QList<int> subIds = QList<int>(),
                            //QHash<int,QList<int> > sids = QHash<int,QList<int> >(),
                            Double gv = 0.00,
                            Double lv = 0.00,
                            bool inc = false);

    //提供给SubjectComplete类的数据模型所用的查询对象
    QSqlQuery* getQuery(){/*QSqlQuery q(db); return q;*/return new QSqlQuery(db);}
    QSqlDatabase& getDb(){return db;}

    //服务函数
    bool getFS_Id_name(QList<int> &ids, QList<QString> &names, int subSys = 1);

    //凭证集相关
    bool getPzsState(int y,int m,PzsState& state);
    bool setPzsState(int y,int m,PzsState state);
    bool setExtraState(int y, int m, bool isVolid);
    bool getExtraState(int y, int m);
    bool getRates(int y, int m, QHash<int,Double>& rates);
    bool saveRates(int y,int m, QHash<int,Double>& rates);
    bool loadPzSet(int y, int m, QList<PingZheng*> &pzs, PzSetMgr *parent);
    bool isContainPz(int y, int m, int pid);
    bool inspectJzPzExist(int y, int m, PzdClass pzCls, int& count);

    bool clearPzSet(int y, int m);
    bool clearRates(int y, int m);
    bool clearExtras(int y, int m);

    //凭证相关
    bool savePingZhengs(QList<PingZheng*> pzs);
    bool savePingZheng(PingZheng* pz);
    bool delPingZhengs(QList<PingZheng*> pzs);
    bool delPingZheng(PingZheng* pz);

    ////////////////////////////////////////////////////////////////////
    bool assignPzNum(int y, int m);
    bool crtNewPz(PzData* pz);
    bool delActionsInPz(int pzId);
    bool getActionsInPz(int pid, QList<BusiActionData2*>& busiActions);
    bool saveActionsInPz(int pid, QList<BusiActionData2*>& busiActions,
                                    QList<BusiActionData2*> dels = QList<BusiActionData2*>());
    bool delSpecPz(int y, int m, PzdClass pzCls, int &affected);
    QList<PzClass> getSpecClsPzCode(PzdClass cls);
    bool haveSpecClsPz(int y, int m, QHash<PzdClass,bool>& isExist);
    bool specPzClsInstat(int y, int m, PzdClass cls, int &affected);
    bool setAllPzState(int y, int m, PzState state, PzState includeState,
                                  int &affected, User* user);
    //////////////////////////////////////////////////////////////////

    //访问子窗口的位置、大小等信息
    bool getSubWinInfo(int winEnum, SubWindowDim* &info, QByteArray* &otherInfo);
    bool saveSubWinInfo(int winEnum, SubWindowDim* info, QByteArray* otherInfo = NULL);

private:
    bool saveAccInfoPiece(InfoField code, QString value);
    bool _readAccountSuites(QList<Account::AccountSuiteRecord*>& suites);
    bool _saveAccountSuites(QList<Account::AccountSuiteRecord*>& suites);

    bool _savePingZheng(PingZheng* pz);
    bool _saveBusiactionsInPz(PingZheng* pz);
    bool _delPingZheng(PingZheng* pz);

    //余额相关辅助函数
    bool _readExtraPoint(int y, int m, QHash<int, int> &mtHashs);
    bool _readExtraForPm(int y, int m, QHash<int,Double> &sums, QHash<int,MoneyDirection> &dirs, bool isFst = true);
    bool _readExtraForMm(int y, int m, QHash<int,Double> &sums, bool isFst = true);
    void _replaeAccurateExtra(QHash<int,Double> &sums, QHash<int,Double> &asums);
    bool _crtExtraPoint(int y, int m, int mt, int& pid);
    bool _saveExtrasForPm(int y, int m, const QHash<int,Double> &sums, const QHash<int,MoneyDirection> &dirs, bool isFst = true);
    bool _saveExtrasForMm(int y, int m, const QHash<int,Double> &sums, bool isFst = true);
    bool _readExtraForFSub(int y,int m, int fid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,MoneyDirection>& dir);
    bool _readExtraForSSub(int y,int m, int sid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,MoneyDirection>& dir);
    //
    int _genKeyForExtraPoint(int y, int m, int mt);

    //表格创建函数
    void crtGdzcTable();

    //
    void warn_transaction(ErrorCode witch, QString context);
    void errorNotify(QString info);
    bool isNewExtraAccess(){return (mv==nmv && sv>=nsv) || (mv>nmv);}//是否采用新余额存取机制

    void _getPreYM(int y, int m, int& yy, int& mm);
    void _getNextYM(int y, int m, int& yy, int& mm);

private:
    QSqlDatabase db;
    int masterMt;   //母币代码（在访问余额状态时要用，因为该状态只保存在母币余额中）
    QString fileName;   //账户文件名
    QHash<InfoField,QString> pNames; //账户信息片段名称表
    QHash<int,int> extraPoints;  //余额指针缓存表（键为余额的年、月和币种所构成的复合键（高4位是年份，中2位是月份，低1位是币种），值为SE_Point表的id）
                                 //使用此变量是为了减少读取指针表的次数，现在暂缓实施
    int mv,sv;  //账户文件版本号（临时保存，用于那些代理函数执行前的版本比较）
};

#endif // DBUTIL_H
