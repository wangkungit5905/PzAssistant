#ifndef TST_PATEST_H
#define TST_PATEST_H

#include <QObject>
#include <QHash>

#include "cal.h"
#include "commdatastruct.h"

class Account;
class SubjectManager;
class PzSetMgr;
class FirstSubject;
class SecondSubject;
class PingZheng;

/**
 * @brief The TestPzObj class
 *  测试凭证集对象的保存和读取功能
 */
class TestPzObj : public QObject
{
    Q_OBJECT
public:
    TestPzObj(Account* account);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testSaveNewPz();

private:
    void initSubjects();
    void crtBankInPz();
    void crtBankOutPz();
    void verify(QList<PingZheng*> ps, QList<PingZheng*> pr);

    Account* account;
    FirstSubject *cashSub,*bankSub,*ysSub,*yfSub;
    SecondSubject *cash_rmb_sub,*bank_rmb_sub,*bank_usd_sub;
    SecondSubject *ys_tp_sub,*ys_jl_sub; //应收下的宁波太平和宁波中包
    SecondSubject *yf_ky_sub,*yf_ms_sub; //应付下的宁波开源和宁波外运

    SubjectManager* smg;
    PzSetMgr* psMgr;
    int y,m;
    QHash<int,Double> rates;
    QHash<int,Money*> moneys;
    QList<PingZheng*> pzs;
};


/**
 * @brief The TestPzSetStat class
 *  用来余额存取功能的正确性
 */
class TestExtraFun : public QObject
{
    Q_OBJECT

public:
    TestExtraFun(Account* account);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testSaveNewSet();
    void testChangeItem();
    void testZeroItem();
    void testNewItem();
    void testNotExistItem();

private:
    void initSubjects();
    void initPreExtra();
    void clearExtra();
    void verify();
    void verifyValues(QHash<int,Double> rv, QHash<int, Double> sv);
    void verifyDirections(QHash<int,MoneyDirection> rd, QHash<int,MoneyDirection> sd);

    Account* account;
    SubjectManager* smg;
    int y,m;
    QHash<int,Double> rates;
    QHash<int,Money*> moneys;
    QList<PingZheng*> pzs;

    //F：一级科目，S：打头表示源值表，其他位置表示二级科目，M：本币形式，D：方向，R：表示读取值
    QHash<int,Double> SF,SMF,SS,SMS;   //源值表
    QHash<int,MoneyDirection> SFD,SSD;
    QHash<int,Double> RF,RMF,RS,RMS;   //读取值表
    QHash<int,MoneyDirection> RFD,RSD;

    //一些要涉及到的科目
    FirstSubject *cashSub,*bankSub,*ysSub,*yfSub,*inSub,*outSub,*cwfySub,*yufSub;
    SecondSubject *cash_rmb_sub,*bank_rmb_sub,*bank_usd_sub;
    SecondSubject *ys_tp_sub,*ys_jl_sub; //应收下的宁波太平和宁波中包
    SecondSubject *yf_ky_sub,*yf_ms_sub; //应付下的宁波开源和宁波外运
    SecondSubject *yuf_jbxa_sub;         //预付下的北京金科信安
    SecondSubject *in_bgf_sub,*out_bgf_sub; //主营收入和成本下的包干费
    SecondSubject *cwfy_hdyi_sub;
};



#endif // TST_PATEST_H
