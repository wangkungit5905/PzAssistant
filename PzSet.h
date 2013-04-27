#ifndef PZSET_H
#define PZSET_H

#include "QList"

#include "common.h"
#include "commdatastruct.h"

class DbUtil;
class CustomRelationTableModel;
class PingZheng;
class Account;
class StatUtil;



//凭证集管理类
class PzSetMgr
{
public:

    PzSetMgr(Account* account,User* user = NULL);
    Account* getAccount(){return account;}
    bool open(int y, int m);
    void close();
    StatUtil &getStatObj();

    int year(){return curY;}
    int month(){return curM;}
    int getPzCount();
    int getMaxZbNum(){return maxZbNum;}
    bool resetPzNum(int by = 1);
    bool determineState();
    PzsState getState(int curY=0, int curM=0);
    void setstate(PzsState state, int y=0, int m=0);
    bool getExtraState(int y=0,int m=0);
    void setExtraState(bool state, int y=0,int m=0);
    bool getPzSet(int y, int m, QList<PingZheng *> &pzs);
    QList<PingZheng*> getPzSpecRange(int y ,int m, QSet<int> nums);
    bool contains(int y, int m, int pid);

    bool saveExtra();
    bool readExtra();
    bool readPreExtra();
    QHash<int,Double>& getRates();

    PingZheng* appendPz(PzClass pzCls=Pzc_Hand);
    bool insert(PingZheng* pd, int &ecode);
    bool remove(int pzNum);
    bool savePz();
    bool save();

    bool crtGdzcPz();
    bool crtDtfyImpPz(int y, int m, QList<PzData *> pzds);
    bool crtDtfyTxPz();
    bool delDtfyPz();
    bool crtJzhdsyPz();
    bool crtJzsyPz();
    bool crtJzlyPz();

    void finishAccount();

    CustomRelationTableModel* getModel();
    bool stat();
    static bool stat(int y, int m);
    bool getDetList();
    static bool getDetList(int y, int sm, int em);
    bool getTotalList();
    static bool getTotalList(int y,int sm, int em);
    bool find();
    static bool find(int y, int sm, int em);

private:
    bool isZbNumConflict(int num);

    bool isOpen();
    int genKey(int y, int m);

    //CustomRelationTableModel* model;


    QList<PingZheng*> pds;                  //凭证对象列表
    QList<PingZheng*> dpds;                 //已被删除的凭证对象列表

    QHash<int,Double> preExtra,preDetExtra; //期初主目和子目余额
    QHash<int,MoneyDirection> preDir,preDetDir;     //期初主目和子目余额方向
    QHash<int,Double> curHpJ,curHpD;        //当期借方和贷方发生额
    QHash<int,Double> endExtra,endDetExtra; //期末主目和子目余额
    QHash<int,MoneyDirection>    endDir,endDetDir;     //期末主目和子目余额方向

    bool isReStat;                          //是否需要重新进行统计标志
    bool isReSave;                          //是否需要保存余额


    ///////////////////////////////////////////////////////////////////////
    int curY, curM;               //当前以只读方式打开的凭证集所属年月
    PzsState state;                         //凭证集状态

    int maxPzNum;                           //最大可用凭证号
    int maxZbNum;                           //最大可用自编号

    QHash<int,QList<PingZheng*> > pzSetHash;  //保存所有已经装载的凭证集（键为年月所构成的整数高4位表示年，低2为表示月）
    QHash<int,PzsState> states; //凭证集状态（键同上）
    QHash<int,bool> extraStates;//凭证集余额状态（键同上）
    Account* account;
    DbUtil* dbUtil;
    StatUtil* statUtil;
    User* user;
    QSqlDatabase db;
};

#endif // PZSET_H
