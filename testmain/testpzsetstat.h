#ifndef TESTPZSETSTAT_H
#define TESTPZSETSTAT_H

#include <QObject>

#include "cal.h"
#include "commdatastruct.h"

class Account;
class SubjectManager;
class PzSetMgr;
class FirstSubject;
class SecondSubject;
class PingZheng;
class StatUtil;

class TestPzSetStat : public QObject
{
    Q_OBJECT
public:
    explicit TestPzSetStat(Account *account);
    ~TestPzSetStat();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testPreExtra();
    void testCurHappen();
    void testendExtra();
private:
    void getPreYM(int& yy,int& mm);
    void getNextYM(int& yy, int& mm);
    void resetPreExtras();
    void initSubjects();
    bool initRates();
    void clearRates();
    bool initPreExtra();
    void clearExtra();
    bool initPzSet();
    void clearPzSet();

    bool verifyValueHash(QHash<int,Double>& v1,QHash<int,Double>& v2);
    bool verifyDirHash(QHash<int,MoneyDirection>& d1, QHash<int,MoneyDirection>& d2);
    //void adjustValueTables(PingZheng* pz);

//    void verifyValues(QHash<int,Double>& vs1,QHash<int,Double>& vs2);
//    void v

    Account* account;
    SubjectManager* smg;
    PzSetMgr* psMgr;
    StatUtil* statUtil;
    int y,m;
    QHash<int,Double> rates;
    QHash<int,Money*> moneys;
    QList<PingZheng*> pzs;

    //F：一级科目，S：打头表示源值表，其他位置表示二级科目，M：本币形式，D：方向，R：表示读取值
    QHash<int,Double> preF,preMF,preS,preMS;   //期初余额
    QHash<int,MoneyDirection> preFD,preSD;

    QHash<int,Double> curJF,curJS,curJMF,curJMS,;   //本期借方发生额
    QHash<int,Double> curDF,curDS,curDMF,curDMS,;   //本期贷方发生额

    QHash<int,Double> endF,endMF,endS,endMS;   //期末余额
    QHash<int,MoneyDirection> endFD,endSD;

    //一些要涉及到的科目
    FirstSubject *cashSub,*bankSub,*ysSub,*yfSub,*inSub,*outSub,*cwfySub,*yufSub;
    SecondSubject *cash_rmb_sub,*bank_rmb_sub,*bank_usd_sub;
    SecondSubject *ys_tp_sub,*ys_jl_sub; //应收下的宁波太平和宁波中包
    SecondSubject *yf_ky_sub,*yf_ms_sub; //应付下的宁波开源和宁波外运
    SecondSubject *yuf_jbxa_sub;         //预付下的北京金科信安
    SecondSubject *in_bgf_sub,*out_bgf_sub; //主营收入和成本下的包干费
    SecondSubject *cwfy_hdyi_sub;
    
};

#endif // TESTPZSETSTAT_H
