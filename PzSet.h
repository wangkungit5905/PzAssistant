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
class FirstSubject;
class SubjectManager;

const int MAXUNDOSTACK = 100;    //Undo栈的最大容量

/**
 * @brief 帐套管理类，用于管理该帐套内所有的凭证集
 */
class AccountSuiteManager : public QObject
{
    Q_OBJECT
public:
    //保存凭证集的哪个部分
    enum SaveWitch{
        SW_ALL   = 1,   //所有
        SW_PZS   = 2,   //仅凭证
        SW_STATE = 3    //凭证集状态和余额状态
    };

    AccountSuiteManager(AccountSuiteRecord* as, Account* account,User* user = NULL,QObject* parent = 0);
    ~AccountSuiteManager();
    Account* getAccount(){return account;}
    AccountSuiteRecord* getSuiteRecord(){return suiteRecord;}
    bool isSuiteClosed(){return suiteRecord->isClosed;} //是否已关账
    SubjectManager* getSubjectManager();
    int getSubSysCode(){return suiteRecord->subSys;}
    bool open(int m);
    bool isSuiteEditable();
    bool isPzSetEditable(int m=0);
    bool isPzSetOpened();
    bool isDirty();
    void closePzSet();
    void rollback();
    int newPzSet();
    void clearPzSetCaches();
    //StatUtil &getStatObj();
    StatUtil* getStatUtil(){return statUtil;}
    QUndoStack* getUndoStack(){return undoStack;}
    PingZheng* getCurPz(){return curPz;}

    int year(){return suiteRecord->year;}
    int month(){return curM/*suiteRecord->recentMonth*/;}
    int getMaxZbNum(){return maxZbNum;}
    bool resetPzNum(int by = 1);
    PzsState getState(int curM=0);
    void setState(PzsState state, int m=0);
    bool getExtraState(int m=0);
    void setExtraState(bool state, int m=0);
    bool getPzSet(int m, QList<PingZheng *> &pzs);
    QList<PingZheng*> getPzSpecRange(QSet<int> nums);
    bool contains(int pid, int y=0, int m=0);
    PingZheng* readPz(int pid, bool &in);
    int getStatePzCount(PzState state);
    bool isAllInstat();

    int verifyAll(User* user);
    int instatAll(User* user);
    bool inspectPzError(QList<PingZhengError *> &errors);

    //bool saveExtra();
    //bool readExtra();
    //bool readPreExtra();
    bool getRates(QHash<int, Double> &rates, int m=0);
    bool setRates(QHash<int,Double> rates, int m=0);

    //导航方法
    PingZheng* first();
    PingZheng* next();
    PingZheng* previou();
    PingZheng* last();
    PingZheng* seek(int num);
    bool seek(PingZheng* pz);
    bool isFirst(){return curIndex == 0;}
    bool isLast(){return !pzs || (curIndex == pzs->count()-1);}

    //返回凭证数的方法
    void getPzCountForMonth(int m,int& repealNum, int& recordingNum, int& verifyNum, int& instatNum);
    int getPzCount();
    int getRecordingCount(){return c_recording;}
    int getVerifyCount(){return c_verify;}
    int getInstatCount(){return c_instat;}
    int getRepealCount(){return c_repeal;}
    QList<PingZheng*> getAllJzhdPzs();

    PingZheng* appendPz(PzClass pzCls=Pzc_Hand);
    bool append(PingZheng* pz);
    bool insert(PingZheng* pz);
    bool insert(int index, PingZheng* pz);
    bool remove(PingZheng* pz);
    bool restorePz(PingZheng *pz);
    PingZheng* getPz(int num);
    void setCurPz(PingZheng *pz);
    bool savePz(PingZheng* pz);

    bool save(SaveWitch witch=SW_ALL);

    //期末处理方法
    bool crtJzhdsyPz(int y, int m, QList<PingZheng*>& createdPzs,
                      QHash<int,Double> sRate,QHash<int,Double> erate, User* user);
    void getJzhdsyPz(QList<PingZheng*>& pzLst);
    int  getJzhdsyMustPzNums();
    bool crtJzsyPz(int y, int m, QList<PingZheng*>& createdPzs);
    void getJzsyPz(QList<PingZheng*>& pzLst);
    bool crtJzlyPz(int y, int m, PingZheng* pz);

    bool crtGdzcPz();
    bool crtDtfyImpPz(int y, int m, QList<PzData *> pzds);
    bool crtDtfyTxPz();
    bool delDtfyPz();


    void finishAccount();

    bool stat();
    static bool stat(int y, int m);
    bool getDetList();
    static bool getDetList(int y, int sm, int em);
    bool getTotalList();
    static bool getTotalList(int y,int sm, int em);
    bool find();
    static bool find(int y, int sm, int em);

    QList<PingZheng *> getHistoryPzSet(int m);

public slots:
    void rateChanged(int month=0);
private slots:
    void needRestat();
    void pzChangedInSet(PingZheng* pz);
    void pzStateChanged(PzState oldState, PzState newState);
signals:
    void currentPzChanged(PingZheng* newPz, PingZheng* oldPz);
    void pzCountChanged(int count); //凭证总数或处于不同状态的凭证数的改变
    void pzSetChanged();//包括凭证集内凭证数和凭证内容的改变，用于通知其他窗口有未保存的修改
    void pzSetStateChanged(PzsState newState);
    void pzExtraStateChanged(bool newState);
private:
    bool _savePzSet();
    void watchPz(PingZheng* pz, bool en=true);
    void cachePz(PingZheng* pz);
    bool isZbNumConflict(int num);
    void scanPzCount(int& repealNum, int& recordingNum, int& verifyNum, int& instatNum, QList<PingZheng*>* pzLst);
    void _determinePzSetState(PzsState& state);
    void _determineCurPzChanged(PingZheng* oldPz);
    bool _inspectDirEngageError(FirstSubject* fsud, MoneyDirection dir, PzClass pzc, QString& eStr);

    void pzsNotOpenWarning();

private:
    QString errorStr;
    //与本期统计相关
    QHash<int,Double> preExtra,preDetExtra; //期初主目和子目余额
    QHash<int,MoneyDirection> preDir,preDetDir;     //期初主目和子目余额方向
    QHash<int,Double> curHpJ,curHpD;        //当期借方和贷方发生额
    QHash<int,Double> endExtra,endDetExtra; //期末主目和子目余额
    QHash<int,MoneyDirection>    endDir,endDetDir;     //期末主目和子目余额方向


    QHash<int,QList<PingZheng*> > pzSetHash;  //保存所有已经装载的凭证集（键为月份）
    QHash<int,PzsState> states; //凭证集状态（键同上）
    QHash<int,bool> extraStates;//凭证集余额状态（键同）
    QList<PingZheng*>* pzs;      //当前打开的凭证集对象列表
    QList<PingZheng*> pz_dels;  //已被删除的凭证对象列表
    QList<PingZheng*> cachedPzs; //保存被删除后执行了保存操作的凭证对象（用以支持恢复任何情况下被删除的凭证对象）
    QList<PingZheng*> historyPzs;//历史凭证

    int curM;                     //当前以编辑方式打开的凭证集所属月份
    PingZheng* curPz;             //当前显示在凭证编辑窗口内的凭证对象
    int curIndex;                 //当前凭证索引
    int maxPzNum;                           //最大可用凭证号
    int maxZbNum;                           //最大可用自编号
    int c_recording,c_verify,c_instat,c_repeal;      //录入态、审核态、入账态和作废的凭证数
    bool dirty;                   //只记录除凭证外的所有对凭证集的更改（凭证集状态、余额状态等）
    Account* account;
    AccountSuiteRecord* suiteRecord;
    DbUtil* dbUtil;
    StatUtil* statUtil;
    User* user;
    QUndoStack* undoStack;
};

#endif // PZSET_H
