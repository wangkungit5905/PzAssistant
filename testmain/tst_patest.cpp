#include <QString>
#include <QtTest>

#include "tst_patest.h"
#include "account.h"
#include "subject.h"
#include "PzSet.h"
#include "dbutil.h"
#include "common.h"
#include "pz.h"
#include "statutil.h"
#include "cal.h"

///////////////////////////////TestPzObj///////////////////////////////////
TestPzObj::TestPzObj(Account *account):account(account),y(2013),m(1)
{
    if(!account->isOpen()){
        qDebug()<<"Test Account open failed!";
        return;
    }
    smg = account->getSubjectManager(1);
    moneys = account->getAllMoneys();
    psMgr = account->getPzSet();
    if(!psMgr->open(y,m))
        return;

}

void TestPzObj::initTestCase()
{
    initSubjects();
}

void TestPzObj::cleanupTestCase()
{
    account->getDbUtil()->delPingZhengs(pzs);
}

/**
 * @brief TestPzObj::testSaveNewPz
 *  测试新凭证的保存功能
 */
void TestPzObj::testSaveNewPz()
{
    crtBankInPz();
    crtBankOutPz();
    DbUtil* dbUtil = account->getDbUtil();
    dbUtil->savePingZhengs(pzs);
    QList<PingZheng*> savedPzs;
    savedPzs=pzs;
    dbUtil->loadPzSet(y,m,pzs,psMgr);
    verify(savedPzs,pzs);


    PingZheng* pz = savedPzs.last();
    pz->setDate("2013-01-12");
    pz->setNumber(3);
    pz->setZbNumber(5);
    pz->setPzState(Pzs_Verify);
    pz->setPzClass(Pzc_Jzhd);
    pz->setEncNumber(8);
    pz->setRecordUser(allUsers.value(2));
    pz->setVerifyUser(allUsers.value(3));
    pz->setBookKeeperUser(allUsers.value(4));
    dbUtil->savePingZheng(pz);
    dbUtil->loadPzSet(y,m,pzs,psMgr);
    verify(savedPzs,pzs);
}

void TestPzObj::initSubjects()
{
    cashSub = smg->getCashSub();
    bankSub = smg->getBankSub();
    ysSub = smg->getFstSubject("1131");
    yfSub = smg->getFstSubject("2121");

    cash_rmb_sub = cashSub->getChildSub(smg->getNameItem(tr("人民币")));
    bank_rmb_sub = bankSub->getChildSub(smg->getNameItem(tr("工行-人民币")));
    bank_usd_sub = bankSub->getChildSub(smg->getNameItem(tr("工行-美元")));
    ys_tp_sub = ysSub->getChildSub(smg->getNameItem(tr("宁波太平")));
    ys_jl_sub = ysSub->getChildSub(smg->getNameItem(tr("宁波佳利")));
    yf_ky_sub = yfSub->getChildSub(smg->getNameItem(tr("宁波开源")));
    yf_ms_sub = yfSub->getChildSub(smg->getNameItem(tr("宁波美设")));
}

/**
 * @brief TestPzObj::crtBankInPz
 *  创建一个银行收入凭证
 */
void TestPzObj::crtBankInPz()
{
    //1#凭证
    PingZheng* pz = psMgr->appendPz();
    pz->append(tr("收宁波太平运费 00001"),bankSub,bank_rmb_sub,moneys.value(RMB),MDIR_J,Double(100.0));
    pz->append(tr("收宁波太平运费 00001"),ysSub,ys_tp_sub,moneys.value(RMB),MDIR_D,Double(100.0));
    pz->setEncNumber(3);
    pzs<<pz;
}

/**
 * @brief TestPzObj::crtBankOutPz
 *  创建银行支出凭证
 */
void TestPzObj::crtBankOutPz()
{
    PingZheng* pz = psMgr->appendPz();
    pz->append(tr("付宁波开源运费 00002"),bankSub,bank_usd_sub,moneys.value(USD),MDIR_D,Double(20.0));
    pz->append(tr("付宁波开源运费 00002"),yfSub,yf_ky_sub,moneys.value(USD),MDIR_D,Double(20.0));
    pz->setEncNumber(1);
    pzs<<pz;
}

/**
 * @brief TestPzObj::verify
 *  验证两组凭证是否相等
 * @param ps
 * @param pr
 */
void TestPzObj::verify(QList<PingZheng *> ps, QList<PingZheng *> pr)
{
    PingZheng *pz_s, *pz_r;
    BusiAction *ba_s, *ba_r;
    QVERIFY(ps.count() == pr.count());
    for(int i = 0; i < pzs.count(); ++i){
        pz_s = ps.at(i);
        pz_r = pr.at(i);
        QVERIFY(pz_s->getDate()==pz_r->getDate());
        QVERIFY(pz_s->number()==pz_r->number());
        QVERIFY(pz_s->zbNumber()==pz_r->zbNumber());
        QVERIFY(pz_s->encNumber()==pz_r->encNumber());
        QVERIFY(pz_s->getPzClass()==pz_r->getPzClass());
        QVERIFY(pz_s->getPzState()==pz_r->getPzState());
        QVERIFY(pz_s->jsum()==pz_r->jsum());
        QVERIFY(pz_s->dsum()==pz_r->dsum());
        QVERIFY(pz_s->recordUser()==pz_r->recordUser());
        QVERIFY(pz_s->verifyUser()==pz_r->verifyUser());
        QVERIFY(pz_s->bookKeeperUser()==pz_r->bookKeeperUser());
        QVERIFY(pz_s->baCount()==pz_r->baCount());
        for(int j = 0; j < pz_s->baCount(); ++j){
            ba_s = pz_s->getBusiAction(j);
            ba_r = pz_r->getBusiAction(j);
            QVERIFY(ba_s->getParent()->id()==ba_r->getParent()->id());
            QVERIFY(ba_s->getSummary() == ba_r->getSummary());
            QVERIFY(ba_s->getFirstSubject()->getId()==ba_r->getFirstSubject()->getId());
            QVERIFY(ba_s->getSecondSubject()->getId()==ba_r->getSecondSubject()->getId());
            QVERIFY(ba_s->getMt()->code()==ba_r->getMt()->code());
            QVERIFY(ba_s->getValue()==ba_r->getValue());
            QVERIFY(ba_s->getDir()==ba_r->getDir());
            QVERIFY(ba_s->getNumber()==ba_r->getNumber());
        }
    }
}


///////////////////////////////////////////////////////////////////
TestExtraFun::TestExtraFun(Account *account):y(2013),m(1),account(account)
{
    if(!account->isOpen()){
        qDebug()<<"Test Account open failed!";
        return;
    }
    smg = account->getSubjectManager(1);
    moneys = account->getAllMoneys();
}

void TestExtraFun::initTestCase()
{
    initSubjects();
    rates[USD]=6.29;
}

void TestExtraFun::cleanupTestCase()
{
    clearExtra();
}

void TestExtraFun::init()
{
    initPreExtra();
    DbUtil* dbUtil = account->getDbUtil();
    QVERIFY2(dbUtil->saveExtraForPm(y,m,SF,SFD,SS,SSD),"Save primary extra oprate failed!");
    QVERIFY2(dbUtil->saveExtraForMm(y,m,SMF,SMS),"Save master extra oprate failed!");
}

void TestExtraFun::cleanup()
{
    SF.clear();SFD.clear();
    SS.clear();SSD.clear();
    SMF.clear();SMS.clear();
    RF.clear();RFD.clear();
    RS.clear();RSD.clear();
    RMF.clear();RMS.clear();
    clearExtra();
}

/**
 * @brief TestPzSetStat::testPreExtra
 *  测试保存一份新的余额值表的功能是否正确
 */
void TestExtraFun::testSaveNewSet()
{
    DbUtil* dbUtil = account->getDbUtil();
    QVERIFY2(dbUtil->readExtraForPm(y,m,RF,RFD,RS,RSD),"read primary extra failed!");
    QVERIFY2(dbUtil->readExtraForMm(y,m,RMF,RMS),"read master extra failed!");
    verifyValues(RF,SF);
    verifyValues(RS,SS);
    verifyValues(RMF,SMF);
    verifyValues(RMS,SMS);
    verifyDirections(RFD,SFD);
    verifyDirections(RSD,SSD);
}

/**
 * @brief TestExtraFun::testChangeItem
 *  测试修改一个值项后，保存是否正确
 */
void TestExtraFun::testChangeItem()
{
    SF[cashSub->getId()*10+RMB]=1100;
    SS[cash_rmb_sub->getId()*10+RMB] = 1100;
    SMF[bankSub->getId()*10+USD]=44444;
    SMS[bank_usd_sub->getId()*10+USD]=44444;
    verify();
}

/**
 * @brief TestExtraFun::testZeroItem
 *  测试对值为0的项目是否会删除
 */
void TestExtraFun::testZeroItem()
{
    //验证当源值表中出现0值项，更改值项时，是否会自动从数据库中移除
    SF[ysSub->getId()*10+RMB] = 0;
    SFD[ysSub->getId()*10+RMB] = MDIR_P;
    SS[ys_tp_sub->getId()*10+RMB] = 0;
    SSD[ys_tp_sub->getId()*10+RMB] = MDIR_P;
    SMF[ysSub->getId()*10+USD] = 0;
    SMS[ys_tp_sub->getId()*10+USD]=0;
    SF[yfSub->getId()*10+RMB]= 1200; //源值=1100
    SS[yf_ky_sub->getId()*10+RMB] = 500; //源值=400
    SMF[bankSub->getId()*10+USD] = 44444;
    SMS[bank_usd_sub->getId()*10+USD] = 44444;
    verify();
}

/**
 * @brief TestExtraFun::testNewItem
 *  测试是否能够保存新增值项
 */
void TestExtraFun::testNewItem()
{
    SF[cwfySub->getId()*10+RMB] = 333;
    SFD[cwfySub->getId()*10+RMB] = MDIR_J;
    SS[cwfy_hdyi_sub->getId()*10+RMB] = 333;
    SSD[cwfy_hdyi_sub->getId()*10+RMB] = MDIR_J;
    SMF[yufSub->getId()*10+USD] = 444;
    SMS[yuf_jbxa_sub->getId()*10+USD] = 444;
    verify();
}

/**
 * @brief TestExtraFun::testNotExistItem
 *  测试是否会删除不再存在的值项
 */
void TestExtraFun::testNotExistItem()
{
    SF.remove(cashSub->getId()*10+RMB);
    SFD.remove(cashSub->getId()*10+RMB);
    SS.remove(cash_rmb_sub->getId()*10+RMB);
    SSD.remove(cash_rmb_sub->getId()*10+RMB);
    SMF.remove(bankSub->getId()*10+USD);
    SMS.remove(bank_usd_sub->getId()*10+USD);
    verify();
}





void TestExtraFun::initSubjects()
{
    cashSub = smg->getCashSub();
    bankSub = smg->getBankSub();
    ysSub = smg->getFstSubject("1131");
    yfSub = smg->getFstSubject("2121");
    inSub = smg->getFstSubject("5101");
    outSub = smg->getFstSubject("5401");
    cwfySub = smg->getCwfySub();
    yufSub = smg->getFstSubject("1151");

    cash_rmb_sub = cashSub->getChildSub(smg->getNameItem(tr("人民币")));
    bank_rmb_sub = bankSub->getChildSub(smg->getNameItem(tr("工行-人民币")));
    bank_usd_sub = bankSub->getChildSub(smg->getNameItem(tr("工行-美元")));
    ys_tp_sub = ysSub->getChildSub(smg->getNameItem(tr("宁波太平")));
    ys_jl_sub = ysSub->getChildSub(smg->getNameItem(tr("宁波佳利")));
    yf_ky_sub = yfSub->getChildSub(smg->getNameItem(tr("宁波开源")));
    yf_ms_sub = yfSub->getChildSub(smg->getNameItem(tr("宁波美设")));
    in_bgf_sub = inSub->getChildSub(smg->getNameItem(tr("包干费等")));
    out_bgf_sub = outSub->getChildSub(smg->getNameItem(tr("包干费等")));
    cwfy_hdyi_sub = cwfySub->getChildSub(smg->getNameItem(tr("汇兑损益")));
    yuf_jbxa_sub = yufSub->getChildSub(smg->getNameItem(tr("北京金科信安")));
}



void TestExtraFun::initPreExtra()
{
    //生成原币形式的期初余额数据表及其方向
    //1、现金期初值为1000元人民币
    SF[cashSub->getId()*10+RMB]=1000;
    SFD[cashSub->getId()*10+RMB]=MDIR_J;
    SS[cash_rmb_sub->getId()*10+RMB]=1000;
    SSD[cash_rmb_sub->getId()*10+RMB]=MDIR_J;
    //2、银行存款（人民币：10000，美金：2000）
    SF[bankSub->getId()*10+RMB]=10000;
    SFD[bankSub->getId()*10+RMB]=MDIR_J;
    SS[bank_rmb_sub->getId()*10+RMB]=10000;
    SSD[bank_rmb_sub->getId()*10+RMB]=MDIR_J;
    SF[bankSub->getId()*10+USD]=2000;
    SFD[bankSub->getId()*10+USD]=MDIR_J;
    SS[bank_usd_sub->getId()*10+USD]=2000;
    SSD[bank_usd_sub->getId()*10+USD]=MDIR_J;
    //3、应收-宁波太平（人民币：200，美金：300），宁波佳利（人民币500）
    SF[ysSub->getId()*10+RMB]=700;
    SFD[ysSub->getId()*10+RMB]=MDIR_J;
    SS[ys_tp_sub->getId()*10+RMB]=200;
    SSD[ys_tp_sub->getId()*10+RMB]=MDIR_J;
    SS[ys_jl_sub->getId()*10+RMB]=500;
    SSD[ys_jl_sub->getId()*10+RMB]=MDIR_J;
    SF[ysSub->getId()*10+USD]=300;
    SFD[ysSub->getId()*10+USD]=MDIR_J;
    SS[ys_tp_sub->getId()*10+USD]=300;
    SSD[ys_tp_sub->getId()*10+USD]=MDIR_J;
    //4、应付-宁波开源（人民币：400，美金：30），宁波美设（人民币：700）
    SF[yfSub->getId()*10+RMB]=1100;
    SFD[yfSub->getId()*10+RMB]=MDIR_D;
    SS[yf_ky_sub->getId()*10+RMB]=400;
    SSD[yf_ky_sub->getId()*10+RMB]=MDIR_D;
    SS[yf_ms_sub->getId()*10+RMB]=700;
    SSD[yf_ms_sub->getId()*10+RMB]=MDIR_D;
    SF[yfSub->getId()*10+USD]=30;
    SFD[yfSub->getId()*10+USD]=MDIR_D;
    SS[yf_ky_sub->getId()*10+USD]=30;
    SSD[yf_ky_sub->getId()*10+USD]=MDIR_D;

    //生成本币形式的期初余额数据表
    Double rate = rates.value(USD);
    SMF[bankSub->getId()*10+USD]=SF.value(bankSub->getId()*10+USD)*rate;
    SMS[bank_usd_sub->getId()*10+USD]=SS.value(bank_usd_sub->getId()*10+USD)*rate;
    SMF[ysSub->getId()*10+USD]=SF.value(ysSub->getId()*10+USD)*rate;
    SMS[ys_tp_sub->getId()*10+USD]=SS.value(ys_tp_sub->getId()*10+USD)*rate;
    SMF[yfSub->getId()*10+USD]=SF.value(yfSub->getId()*10+USD)*rate;
    SMS[yf_ky_sub->getId()*10+USD]=SS.value(yf_ky_sub->getId()*10+USD)*rate;
}


void TestExtraFun::clearExtra()
{
   account->getDbUtil()->clearExtras(y,m);   //清除期末余额
}

/**
 * @brief TestExtraFun::verify
 *  验证源值表和读取值表是否一致
 */
void TestExtraFun::verify()
{
    DbUtil* dbUtil = account->getDbUtil();
    QVERIFY2(dbUtil->saveExtraForPm(y,m,SF,SFD,SS,SSD),"save primary extra failed!");
    QVERIFY2(dbUtil->saveExtraForMm(y,m,SMF,SMS),"save master extra failed!");
    QVERIFY2(dbUtil->readExtraForPm(y,m,RF,RFD,RS,RSD),"read primary extra failed!");
    QVERIFY2(dbUtil->readExtraForMm(y,m,RMF,RMS),"read master extra failed!");
    verifyValues(RF,SF);
    verifyValues(RS,SS);
    verifyValues(RMF,SMF);
    verifyValues(RMS,SMS);
    verifyDirections(RFD,SFD);
    verifyDirections(RSD,SSD);
}


/**
 * @brief TestPzSetStat::verifyValues
 *  校验两个hash值表是否相等
 * @param rv    读取的值表
 * @param sv    源值表
 */
void TestExtraFun::verifyValues(QHash<int, Double> rv, QHash<int, Double> sv)
{
    QList<int> keys;
    QHashIterator<int,Double> it(rv);
    keys=sv.keys();
    while(it.hasNext()){
        it.next();
        QVERIFY2(sv.contains(it.key()),"Find a item don't exist in the source hash!");
        QVERIFY2(it.value()==sv.value(it.key()), "Find a item is diffrence!");
        keys.removeOne(it.key());
    }
    if(!keys.isEmpty()){
        foreach(int key,keys){
            QVERIFY2(sv.value(key)==0,"Finde a non zero item don't save in the source hash!");
        }
    }
}

void TestExtraFun::verifyDirections(QHash<int, MoneyDirection> rd, QHash<int, MoneyDirection> sd)
{
    QList<int> keys;
    QHashIterator<int,MoneyDirection> it(rd);
    keys=sd.keys();
    while(it.hasNext()){
        it.next();
        QVERIFY2(sd.contains(it.key()),"Find a item don't exist in the source hash!");
        QVERIFY2(it.value()==sd.value(it.key()), "Find a item is diffrence!");
        keys.removeOne(it.key());
    }
    if(!keys.isEmpty()){
        foreach(int key,keys){
            QVERIFY2(sd.value(key)==0,"Finde a non zero item don't save in the source hash!");
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////

