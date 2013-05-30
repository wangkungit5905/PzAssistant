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
class QUndoStack;

const int MAXUNDOSTACK = 100;    //Undo栈的最大容量

//凭证集管理类
class PzSetMgr : public QObject
{
    Q_OBJECT
public:
    //保存凭证集的哪个部分
    enum SaveWitch{
        SW_ALL   = 1,   //所有
        SW_PZS   = 2,   //仅凭证
        SW_STATE = 3    //凭证集状态和余额状态
    };

    PzSetMgr(Account* account,User* user = NULL,QObject* parent = 0);
    ~PzSetMgr();
    Account* getAccount(){return account;}
    bool open(int y, int m);
    bool isOpened();
    bool isDirty();
    void close();
    StatUtil &getStatObj();
    QUndoStack* getUndoStack(){return undoStack;}
    PingZheng* getCurPz(){return curPz;}

    int year(){return curY;}
    int month(){return curM;}
    int getMaxZbNum(){return maxZbNum;}
    bool resetPzNum(int by = 1);
    PzsState getState(int curY=0, int curM=0);
    void setState(PzsState state, int y=0, int m=0);
    bool getExtraState(int y=0,int m=0);
    void setExtraState(bool state, int y=0,int m=0);
    bool getPzSet(int y, int m, QList<PingZheng *> &pzs);
    QList<PingZheng*> getPzSpecRange(int y ,int m, QSet<int> nums);
    bool contains(int y, int m, int pid);
    int getStatePzCount(PzState state);

    bool saveExtra();
    bool readExtra();
    bool readPreExtra();
    QHash<int, Double> getRates();

    //导航方法
    PingZheng* first();
    PingZheng* next();
    PingZheng* previou();
    PingZheng* last();
    PingZheng* seek(int num);
    bool isFirst(){return curIndex == 0;}
    bool isLast(){return !pzs || (curIndex == pzs->count()-1);}

    //返回凭证数的方法
    int getPzCount();
    int getRecordingCount(){return c_recording;}
    int getVerifyCount(){return c_verify;}
    int getInstatCount(){return c_instat;}
    int getRepealCount(){return c_repeal;}

    PingZheng* appendPz(PzClass pzCls=Pzc_Hand);
    bool append(PingZheng* pz);
    bool insert(PingZheng* pz);
    bool insert(int index, PingZheng* pz);
    bool remove(PingZheng* pz);
    bool restorePz(PingZheng *pz);
    PingZheng* getPz(int num);
    void setCurPz(PingZheng *pz);
    bool savePz(PingZheng* pz);
    bool savePzSet();
    bool save(SaveWitch witch=SW_ALL);

    bool crtGdzcPz();
    bool crtDtfyImpPz(int y, int m, QList<PzData *> pzds);
    bool crtDtfyTxPz();
    bool delDtfyPz();
    bool crtJzhdsyPz();
    bool crtJzsyPz();
    bool crtJzlyPz();

    void finishAccount();

    bool stat();
    static bool stat(int y, int m);
    bool getDetList();
    static bool getDetList(int y, int sm, int em);
    bool getTotalList();
    static bool getTotalList(int y,int sm, int em);
    bool find();
    static bool find(int y, int sm, int em);


private slots:
    void needRestat();
    void pzChangedInSet(PingZheng* pz);
signals:
    void currentPzChanged(PingZheng* newPz, PingZheng* oldPz);
    void pzCountChanged(int count);
    void pzSetChanged();//包括凭证集内凭证数和凭证内容的改变，用于通知其他窗口有未保存的修改
private:
    void cachePz(PingZheng* pz);
    bool isZbNumConflict(int num);
    void scanPzCount();
    void _determinePzSetState(PzsState& state);
    void _determineCurPzChanged(PingZheng* oldPz);

    int genKey(int y, int m);

private:
    QString errorStr;
    //与本期统计相关
    QHash<int,Double> preExtra,preDetExtra; //期初主目和子目余额
    QHash<int,MoneyDirection> preDir,preDetDir;     //期初主目和子目余额方向
    QHash<int,Double> curHpJ,curHpD;        //当期借方和贷方发生额
    QHash<int,Double> endExtra,endDetExtra; //期末主目和子目余额
    QHash<int,MoneyDirection>    endDir,endDetDir;     //期末主目和子目余额方向
    bool isReStat;                          //是否需要重新进行统计标志
    bool isReSave;                          //是否需要保存余额


    QHash<int,QList<PingZheng*> > pzSetHash;  //保存所有已经装载的凭证集（键为年月所构成的整数高4位表示年，低2为表示月）
    QHash<int,PzsState> states; //凭证集状态（键同上）
    QHash<int,bool> extraStates;//凭证集余额状态（键同上）
    QList<PingZheng*>* pzs;      //当前打开的凭证集对象列表
    QList<PingZheng*> pz_dels;  //已被删除的凭证对象列表

    QList<PingZheng*> cachedPzs; //保存被删除后执行了保存操作的凭证对象（用以支持恢复任何情况下被删除的凭证对象）

    int curY, curM;               //当前以只读方式打开的凭证集所属年月
    PingZheng* curPz;             //当前显示在凭证编辑窗口内的凭证对象
    int curIndex;                 //当前凭证索引
    //PzsState state;                         //凭证集状态
    int maxPzNum;                           //最大可用凭证号
    int maxZbNum;                           //最大可用自编号
    int c_recording,c_verify,c_instat,c_repeal;      //录入态、审核态、入账态和作废的凭证数
    bool dirty;                   //只记录除凭证外的所有对凭证集的更改（凭证集状态、余额状态等）
    Account* account;
    DbUtil* dbUtil;
    StatUtil* statUtil;
    User* user;
    QUndoStack* undoStack;
};

#endif // PZSET_H
