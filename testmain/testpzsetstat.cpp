#include <QtTest>

#include "testpzsetstat.h"
#include "pz.h"
#include "dbutil.h"
#include "statutil.h"
#include "PzSet.h"

TestPzSetStat::TestPzSetStat(Account* account) : account(account),y(2013),m(1)
{
    if(!account->isOpen()){
        qDebug()<<"Test Account open failed!";
        return;
    }
    smg = account->getSubjectManager(1);
    moneys = account->getAllMoneys();
    psMgr = account->getPzSet();
}

TestPzSetStat::~TestPzSetStat()
{
    delete statUtil;
}

void TestPzSetStat::initTestCase()
{
    initSubjects();
    QVERIFY2(initRates(),"Initial rates failed!");
    QVERIFY2(initPreExtra(),"Initial pre extra failed!");
    QVERIFY2(initPzSet(),"Initial PingZheng Set failed!");
    QVERIFY2(psMgr->open(y,m),"PengZheng set open failed!");
    qDeleteAll(pzs);
    pzs.clear();
    QVERIFY2(psMgr->getPzSet(y,m,pzs),"PingZheng set load failed!");
    statUtil = new StatUtil(pzs,account);
    QVERIFY2(statUtil->stat(),"Stat current happen extra failed!") ;
}

void TestPzSetStat::cleanupTestCase()
{
    clearRates();
    clearPzSet();
    clearExtra();
}

/**
 * @brief TestPzSetStat::testPreExtra
 *  验证期初余额是否正确
 */
void TestPzSetStat::testPreExtra()
{
    QHash<int,Double> vs;
    QHash<int,MoneyDirection> ds;
    vs = statUtil->getPreValueFPm();
    QVERIFY2(preF==vs, "First subject preview extra(primary) don't equal!");
    ds = statUtil->getPreDirF();
    QVERIFY2(preFD==ds,"First subject preview extra direction don't equal!");
    vs = statUtil->getPreValueSPm();
    QVERIFY2(preS==vs, "Second subject preview extra(primary) don't equal!");
    ds = statUtil->getPreDirS();
    QVERIFY2(preSD==ds,"Second subject preview extra direction don't equal!");
    vs = statUtil->getPreValueFMm();
    QVERIFY2(preMF==vs, "First subject preview extra(master) don't equal!");
    vs = statUtil->getPreValueSMm();
    QVERIFY2(preMS==vs, "Second subject preview extra(master) don't equal!");
}

/**
 * @brief TestPzSetStat::testCurHappen
 *  验证本期发生额是否正确
 */
void TestPzSetStat::testCurHappen()
{
    Double rate = rates.value(USD);

    curJF[bankSub->getId()*10+RMB] += 100.0;
    curJS[bank_rmb_sub->getId()*10+RMB] += 100.0;
    curJF[bankSub->getId()*10+USD] += 200.0;
    curJS[bank_usd_sub->getId()*10+USD] += 200.0;
    curJMF[bankSub->getId()*10+USD] += rate*200.0;
    curJMS[bank_usd_sub->getId()*10+USD] += rate*200.0;
    curDF[ysSub->getId()*10+RMB] += 100.0;
    curDS[ys_tp_sub->getId()*10+RMB] += 100.0;
    curDF[ysSub->getId()*10+USD] += 200.0;
    curDS[ys_tp_sub->getId()*10+USD] += 200.0;
    curDMF[ysSub->getId()*10+USD] += rate*200.0;
    curDMS[ys_tp_sub->getId()*10+USD] += rate*200.0;


    QVERIFY2(verifyValueHash(curJF,statUtil->getCurValueJFPm()),"Current happen J dir first subject primary don't equal!");
    QVERIFY2(verifyValueHash(curDF,statUtil->getCurValueDFPm()),"Current happen D dir first subject primary don't equal!");
    QVERIFY2(verifyValueHash(curJS,statUtil->getCurValueJSPm()),"Current happen J dir second subject primary don't equal!");
    QVERIFY2(verifyValueHash(curDS,statUtil->getCurValueDSPm()),"Current happen D dir second subject primary don't equal!");
    QVERIFY2(verifyValueHash(curJMF,statUtil->getCurValueJFMm()),"Current happen J dir first subject master don't equal!");
    QVERIFY2(verifyValueHash(curJMS,statUtil->getCurValueJSMm()),"Current happen J dir second subject master don't equal!");
    QVERIFY2(verifyValueHash(curDMF,statUtil->getCurValueDFMm()),"Current happen D dir first subject master don't equal!");
    QVERIFY2(verifyValueHash(curDMS,statUtil->getCurValueDSMm()),"Current happen D dir second subject master don't equal!");
}

/**
 * @brief TestPzSetStat::testendExtra
 *  验证期末余额是否正确
 */
void TestPzSetStat::testendExtra()
{
    //现金
    endF[cashSub->getId()*10+RMB] = 1000;
    endS[cash_rmb_sub->getId()*10+RMB] = 1000;
    endFD[cashSub->getId()*10+RMB] = MDIR_J;
    endSD[cash_rmb_sub->getId()*10+RMB] = MDIR_J;
    //银行
    endF[bankSub->getId()*10+RMB] = 10100;
    endS[bank_rmb_sub->getId()*10+RMB] = 10100;
    endF[bankSub->getId()*10+USD] = 2200;
    endS[bank_usd_sub->getId()*10+USD] = 2200;
    endFD[bankSub->getId()*10+RMB] = MDIR_J;
    endSD[bank_rmb_sub->getId()*10+RMB] = MDIR_J;
    endFD[bankSub->getId()*10+USD] = MDIR_J;
    endSD[bank_usd_sub->getId()*10+USD] = MDIR_J;
    //应收
    endF[ysSub->getId()*10+RMB] = 600;
    endF[ysSub->getId()*10+USD] = 100;
    endS[ys_tp_sub->getId()*10+RMB] = 100;
    endS[ys_tp_sub->getId()*10+USD] = 100;
    endS[ys_jl_sub->getId()*10+RMB] = 500;
    endFD[ysSub->getId()*10+RMB] = MDIR_J;
    endFD[ysSub->getId()*10+USD] = MDIR_J;
    endSD[ys_tp_sub->getId()*10+RMB] = MDIR_J;
    endSD[ys_tp_sub->getId()*10+USD] = MDIR_J;
    endSD[ys_jl_sub->getId()*10+RMB] = MDIR_J;
    //应付
    endF[yfSub->getId()*10+RMB] = 1100;
    endF[yfSub->getId()*10+USD] = 30;
    endS[yf_ky_sub->getId()*10+RMB] = 400;
    endS[yf_ky_sub->getId()*10+USD] = 30;
    endS[yf_ms_sub->getId()*10+RMB] = 700;
    endFD[yfSub->getId()*10+RMB] = MDIR_D;
    endFD[yfSub->getId()*10+USD] = MDIR_D;
    endSD[yf_ky_sub->getId()*10+RMB] = MDIR_D;
    endSD[yf_ky_sub->getId()*10+USD] = MDIR_D;
    endSD[yf_ms_sub->getId()*10+RMB] = MDIR_D;

    QVERIFY2(verifyValueHash(endF,statUtil->getEndValueFPm()),"End extra don't equl(first subject primary)!");
    QVERIFY2(verifyValueHash(endS,statUtil->getEndValueSPm()),"End extra don't equl(second subject primary)!");
    QVERIFY2(verifyDirHash(endFD,statUtil->getEndDirF()),"End direction don't equl(first subject)!");
    QVERIFY2(verifyDirHash(endSD,statUtil->getEndDirS()),"End direction don't equl(second subject)!");
}

void TestPzSetStat::getPreYM(int &yy, int &mm)
{
    if(m == 1){
        yy = y - 1;
        mm = 12;
    }
    else{
        yy = y;
        mm = m - 1;
    }
}

void TestPzSetStat::getNextYM(int &yy, int &mm)
{
    if(m==12){
        yy = y + 1;
        mm = 1;
    }
    else{
        yy = y;
        mm = m + 1;
    }
}

void TestPzSetStat::resetPreExtras()
{
    preF.clear();preFD.clear();
    preS.clear();preSD.clear();
    preMF.clear();preMS.clear();
}

void TestPzSetStat::initSubjects()
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

bool TestPzSetStat::initRates()
{
    int yy,mm;
    getPreYM(yy,mm);
    rates[USD]=6.29;    //上月汇率
    if(!account->setRates(yy,mm,rates))
        return false;
    rates[USD]=6.27;   //下月汇率
    getNextYM(yy,mm);
    if(!account->setRates(yy,mm,rates))
        return false;
    rates[USD]=6.28;    //本月汇率
    if(!account->setRates(y,m,rates))
        return false;
    return true;
}

void TestPzSetStat::clearRates()
{
    account->getDbUtil()->clearRates(y,m);
}

bool TestPzSetStat::initPreExtra()
{
    //生成原币形式的期初余额数据表及其方向
    //1、现金期初值为1000元人民币
    preF[cashSub->getId()*10+RMB]=1000;
    preFD[cashSub->getId()*10+RMB]=MDIR_J;
    preS[cash_rmb_sub->getId()*10+RMB]=1000;
    preSD[cash_rmb_sub->getId()*10+RMB]=MDIR_J;
    //2、银行存款（人民币：10000，美金：2000）
    preF[bankSub->getId()*10+RMB]=10000;
    preFD[bankSub->getId()*10+RMB]=MDIR_J;
    preS[bank_rmb_sub->getId()*10+RMB]=10000;
    preSD[bank_rmb_sub->getId()*10+RMB]=MDIR_J;
    preF[bankSub->getId()*10+USD]=2000;
    preFD[bankSub->getId()*10+USD]=MDIR_J;
    preS[bank_usd_sub->getId()*10+USD]=2000;
    preSD[bank_usd_sub->getId()*10+USD]=MDIR_J;
    //3、应收-宁波太平（人民币：200，美金：300），宁波佳利（人民币500）
    preF[ysSub->getId()*10+RMB]=700;
    preFD[ysSub->getId()*10+RMB]=MDIR_J;
    preS[ys_tp_sub->getId()*10+RMB]=200;
    preSD[ys_tp_sub->getId()*10+RMB]=MDIR_J;
    preS[ys_jl_sub->getId()*10+RMB]=500;
    preSD[ys_jl_sub->getId()*10+RMB]=MDIR_J;
    preF[ysSub->getId()*10+USD]=300;
    preFD[ysSub->getId()*10+USD]=MDIR_J;
    preS[ys_tp_sub->getId()*10+USD]=300;
    preSD[ys_tp_sub->getId()*10+USD]=MDIR_J;
    //4、应付-宁波开源（人民币：400，美金：30），宁波美设（人民币：700）
    preF[yfSub->getId()*10+RMB]=1100;
    preFD[yfSub->getId()*10+RMB]=MDIR_D;
    preS[yf_ky_sub->getId()*10+RMB]=400;
    preSD[yf_ky_sub->getId()*10+RMB]=MDIR_D;
    preS[yf_ms_sub->getId()*10+RMB]=700;
    preSD[yf_ms_sub->getId()*10+RMB]=MDIR_D;
    preF[yfSub->getId()*10+USD]=30;
    preFD[yfSub->getId()*10+USD]=MDIR_D;
    preS[yf_ky_sub->getId()*10+USD]=30;
    preSD[yf_ky_sub->getId()*10+USD]=MDIR_D;

    //生成本币形式的期初余额数据表
    Double rate = rates.value(USD);
    preMF[bankSub->getId()*10+USD]=preF.value(bankSub->getId()*10+USD)*rate;
    preMS[bank_usd_sub->getId()*10+USD]=preS.value(bank_usd_sub->getId()*10+USD)*rate;
    preMF[ysSub->getId()*10+USD]=preF.value(ysSub->getId()*10+USD)*rate;
    preMS[ys_tp_sub->getId()*10+USD]=preS.value(ys_tp_sub->getId()*10+USD)*rate;
    preMF[yfSub->getId()*10+USD]=preF.value(yfSub->getId()*10+USD)*rate;
    preMS[yf_ky_sub->getId()*10+USD]=preS.value(yf_ky_sub->getId()*10+USD)*rate;

    DbUtil* dbUtil = account->getDbUtil();
    int yy,mm;
    getPreYM(yy,mm);
    if(!dbUtil->saveExtraForPm(yy,mm,preF,preFD,preS,preSD))
        return false;
    if(!dbUtil->saveExtraForMm(yy,mm,preMF,preMS))
        return false;
    return true;
}

void TestPzSetStat::clearExtra()
{
    int yy,mm;
    getPreYM(yy,mm);
    account->getDbUtil()->clearExtras(yy,mm);
    account->getDbUtil()->clearExtras(y,m);
}

bool TestPzSetStat::initPzSet()
{
    //1#凭证（借银行人民币100，贷应收-宁波太平100）
    PingZheng* pz = new PingZheng(0,0,"2013-01-01",1,1,100.0,100.0,Pzc_Hand,2,Pzs_Recording,
                                  0,allUsers.value(1),0);
    pz->append(tr("收宁波太平运费 00001"),bankSub,bank_rmb_sub,moneys.value(RMB),MDIR_J,Double(100.0));
    pz->append(tr("收宁波太平运费 00001"),ysSub,ys_tp_sub,moneys.value(RMB),MDIR_D,Double(100.0));
    pzs<<pz;

    //2#瓶子（借银行美金200，贷应收-宁波太平200）
    pz = new PingZheng(0,0,"2013-01-02",2,2,1256.0,1256.0,Pzc_Hand,3,Pzs_Recording,
                                  0,allUsers.value(1),0);
    pz->append(tr("收宁波太平海运费 00002"),bankSub,bank_usd_sub,moneys.value(USD),MDIR_J,Double(200.0));
    pz->append(tr("收宁波太平海运费 00002"),ysSub,ys_tp_sub,moneys.value(USD),MDIR_D,Double(200.0));
    pzs<<pz;
    return account->getDbUtil()->savePingZhengs(pzs);
}

void TestPzSetStat::clearPzSet()
{
    qDeleteAll(pzs);
    account->getDbUtil()->clearPzSet(y,m);
}

/**
 * @brief TestPzSetStat::verifyHappenHash
 *  比较两个hash表是否相同（在比较前，将移除值为0的键值对）
 * @param v1
 * @param v2
 */
bool TestPzSetStat::verifyValueHash(QHash<int, Double> &v1, QHash<int, Double> &v2)
{
    QHashIterator<int,Double>* it = new QHashIterator<int,Double>(v1);
    while(it->hasNext()){
        it->next();
        if(it->value()==0)
            v1.remove(it->key());
    }
    it = new QHashIterator<int,Double>(v2);
    while(it->hasNext()){
        it->next();
        if(it->value()==0)
            v2.remove(it->key());
    }
    return v1==v2;
}

/**
 * @brief TestPzSetStat::verifyDirHash
 *  比较两个保存方向值的hash表是否相等
 * @param d1
 * @param d2
 * @return
 */
bool TestPzSetStat::verifyDirHash(QHash<int, MoneyDirection> &d1, QHash<int, MoneyDirection> &d2)
{
    QHashIterator<int,MoneyDirection>* it = new QHashIterator<int,MoneyDirection>(d1);
    while(it->hasNext()){
        it->next();
        if(it->value() == MDIR_P)
            d1.remove(it->key());
    }
    it = new QHashIterator<int,MoneyDirection>(d2);
    while(it->hasNext()){
        it->next();
        if(it->value() == MDIR_P)
            d2.remove(it->key());
    }
    return d1==d2;
}

///**
// * @brief TestPzSetStat::adjustValueTables
// *  每添加一个凭证，就调整本期发生额与
// * @param pz
// */
//void TestPzSetStat::adjustValueTables(PingZheng *pz)
//{
//}






