#include <QDate>

#include "statutil.h"
#include "account.h"
#include "pz.h"
#include "dbutil.h"

StatUtil::StatUtil(QList<PingZheng *> &pzs, Account *account):pzs(pzs),account(account)
{
    dbUtil = account->getDbUtil();
    if(pzs.isEmpty())
        return;
    PingZheng* pz = pzs.first();
    y = pz->getDate2().year();
    m = pz->getDate2().month();
    smg = account->getSubjectManager(account->getSuite(y)->subSys);
    masterMt = account->getMasterMt();
    account->getRates(y,m,rates);
}

/**
 * @brief StatUtil::stat
 *  统计本期发生额，并依据前期余额计算本期余额
 * @return
 */
bool StatUtil::stat()
{
    if(_readPreExtra()){
        QMessageBox::critical(0,QObject::tr("错误提示"),QObject::tr("在读取%1年%2的前期余额时发生错误！"));
        return false;
    }
    if(!_statCurHappen()){
        QMessageBox::warning(0,QObject::tr("错误提示"),QObject::tr("在统计%1年%2的本期发生额时，发现凭证有误，请使用凭证集检错工具查看或调阅日志信息！"));
        return false;
    }
    _calEndExtra();
    _calEndExtra(false);
    return true;
}

/**
 * @brief StatUtil::save
 *  保存本期余额
 * @return
 */
bool StatUtil::save()
{
    return true;
}



/**
 * @brief StatUtil::_clearDatas
 *  清除所有hash表内的数据
 */
void StatUtil::_clearDatas()
{
    preFExa.clear(); preSExa.clear();
    preFExaR.clear();preSExaR.clear();
    preFDir.clear(); preSDir.clear();

    curJF.clear();curJS.clear();
    curJFR.clear();curJSR.clear();
    curDF.clear();curDS.clear();
    curDFR.clear();curDSR.clear();

    endFExa.clear();endSExa.clear();
    endFExaR.clear();endSExaR.clear();
    endFDir.clear();endSDir.clear();
}

/**
 * @brief StatUtil::_statCurHappen
 *  统计本期发生额
 */
bool StatUtil::_statCurHappen()
{
    PingZheng* pz;
    BusiAction* ba;
    int mt,keyF,keyS;
    Double v;
    //FirstSubject* cwfySub = smg->getCwfySub();
    FirstSubject* fsub;
    SecondSubject* ssub;
    QHash<int,Double> rates;
    account->getRates(y,m,rates);

    curJF.clear(); curJS.clear();/*curJFR.clear();curJSR.clear();*/
    curDF.clear(); curDS.clear();/*curDFR.clear();curDSR.clear();*/

    for(int i = 0; i < pzs.count(); ++i){
        pz = pzs.at(i);
        for(int j = 0; j < pz->baCount(); ++j){
            ba = pz->getBusiAction(j);
            fsub = ba->getFirstSubject();
            if(!fsub){
                LOG_WARNING(QObject::tr("First subject is null in index busiAction(%1) of PingZheng(%2,%3,%4)")
                            .arg(ba->getNumber()).arg(y).arg(m).arg(pz->number()));
                return false;
            }
            ssub = ba->getSecondSubject();
            if(!ssub){
                LOG_WARNING(QObject::tr("Secound subject is null in index busiAction(%1) of PingZheng(%2,%3,%4)")
                            .arg(ba->getNumber()).arg(y).arg(m).arg(pz->number()));
                return false;
            }
            mt = ba->getMt()->code();
            keyF = fsub->getId()*10+mt;
            keyS = ssub->getId()*10+mt;
            //如果是结转汇兑损益的凭证，则要进行专门的处理 则跳过非财务费用方的会计分录，因为这些要计入到外币部分
            if((pz->getPzClass() == Pzc_Jzhd) /*&& (fsub->isSameSub(cwfySub))*/){
                //对结转方即非财务费用方的会计分录，其值要计入对应科目的对应外币上（因为调整的是该科目对应外币的本币值）
                //如果要支持多个外币，则必须有一个机制来辨识该会计分录结转的是对应哪个外币
                //目前为了简化，始终认为是美金
                if(fsub->isUseForeignMoney()){
                    int key = fsub->getId()*10 + USD;
                    curDFR[key] += ba->getValue();
                    if(!curJFR.contains(key))
                        curJFR[key] = Double(0.0);
                    key = ssub->getId()*10+USD;
                    curDSR[key] += ba->getValue();
                    if(!curJSR.contains(key))
                        curJSR[key] = Double(0.0);
                }
                else{ //财务费用方
                    curJF[keyF] += ba->getValue();
                    if(!curDF.contains(keyF))
                        curDF[keyF] = Double(0.00);
                    curJS[keyS] += ba->getValue();
                    if(!curDS.contains(keyS))
                        curDS[keyS] = Double(0.00);
                }
            }
            else{
                if(ba->getDir() == MDIR_J){//发生在借方
                    curJF[keyF] += ba->getValue();
                    if(!curDF.contains(keyF)) //这是为了确保jSums和dSums的key集合相同
                        curDF[keyF] = Double(0.00);
                    curJS[keyS] += ba->getValue();
                    if(!curDS.contains(keyS))
                        curDS[keyS] = Double(0.00);
                    if(mt != masterMt->code()){
                        v = ba->getValue() * rates.value(mt);
                        curJFR[keyF] += v;
                        if(!curDFR.contains(keyF))
                            curDFR[keyF] = Double(0.0);
                        curJSR[keyS] += v;
                        if(!curDSR.contains(keyS))
                            curDSR[keyS] = Double(0.0);
                    }
                }
                else{
                    curJF[keyF] += ba->getValue();
                    if(!curDF.contains(keyF)) //这是为了确保jSums和dSums的key集合相同
                        curDF[keyF] = Double(0.00);
                    curDS[keyS] += ba->getValue();
                    if(!curJS.contains(keyS))
                        curJS[keyS] = Double(0.00);
                    if(mt != masterMt->code()){
                        v = ba->getValue() * rates.value(mt);
                        curDFR[keyF] += v;
                        if(!curJFR.contains(keyF))
                            curJFR[keyF] = Double(0.0);
                        curDSR[keyS] += v;
                        if(!curJSR.contains(keyS))
                            curJSR[keyS] =  Double(0.0);
                    }
                }
            }
        }
    }

}

/**
 * @brief StatUtil::_readPreExtra
 *  读取前期余额及其方向
 */
bool StatUtil::_readPreExtra()
{
    int yy,mm;
    if(m == 1){
        yy = y-1;
        mm = 12;
    }
    else{
        yy = y;
        mm = m - 1;
    }
    preFExa.clear();preFDir.clear();
    preSExa.clear();preSDir.clear();
    preFExaR.clear();preSExaR.clear();
    if(!dbUtil->readExtraForPm(yy,mm,preFExa,preFDir,preSExa,preSDir))
        return false;
    if(!dbUtil->readExtraForMm(yy,mm,preFExaR,preSExaR))
        return false;
    return true;
}

/**
 * @brief StatUtil::_calEndExtra
 * @param isFst true：统计的是一级科目，false：二级科目
 */
void StatUtil::_calEndExtra(bool isFst)
{
    //期末余额的计算及其方向的确定：
    //要遵循的基本原则：
    //（1）同向计算，即只有在方向相同的情况下，才进行金额的加减计算，否则在计算前要进行调整
    //（2）始终用“借方-贷方”后所得差额值的符号，来确定方向
    //（3）只有当涉及到外币项目时，才进行本币值的计算

    //变量命名约定（p：期初，v：金额，j：借方，d：贷方或方向，c：本期发生，e：期末，R：本币值）
    QHash<int,Double> pvs,pvRs,cjvs,cjRvs,cdvs,cdRvs,evs,evRs;
    QHash<int, MoneyDirection> pds,eds;
    if(isFst){
        pvs = preFExa;
        pvRs = preFExaR;
        pds = preFDir;
        cjvs = curJF;
        cjRvs = curJFR;
        cdvs = curDF;
        cdRvs = curDFR;
        evs = endFExa;
        evRs = endFExaR;
        eds = endFDir;
    }
    else{
        pvs = preSExa;
        pvRs = preSExaR;
        pds = preSDir;
        cjvs = curJS;
        cjRvs = curJSR;
        cdvs = curDS;
        cdRvs = curDSR;
        evs = endSExa;
        evRs = endSExaR;
        eds = endSDir;
    }

    evs.clear(); eds.clear(); evRs.clear();

    Double vp,vm;   //原币金额、本币金额
    MoneyDirection dir;
    int sid,key,mt;
    int mmt = masterMt->code();
    QHashIterator<int,Double> it(cjvs);
    while(it.hasNext()){
        key = it.key();
        sid = key/10;
        mt = key%10;
        vp = cjvs.value(key) - cdvs.value(key);
        if(mt != mmt)
            vm = cjRvs.value(key) - cdRvs.value(key);
        if(vp > 0)
            dir = MDIR_J;
        else if(vp < 0){
            dir = MDIR_D;
            vp.changeSign();
            vm.changeSign();
        }
        else
            dir = MDIR_P;
        if(dir == MDIR_P){ //本期借贷相抵（平），则余额值和方向同期初
            evs[key] = pvs.value(key);
            eds[key] = pds.value(key);
            if(mt != mmt){
                evRs[key] = pvRs.value(key);
                //if(isFst)
                //    evRs[key] = pvRs.value(key);
                //else
                //    endSExaR = preSExaR;
            }
        }
        else if(pds.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
            evs[key] = pvs.value(key) + vp;
            eds[key] = pds.value(key);
            if(mt != mmt){
                evRs[key] = pvRs.value(key) + vm;
                //if(isFst)
                //    endFExaR = preFExaR + vm;
                //else
                //    endSExaR = preSExaR + vm;
            }
        }
        else{
            Double tvp,tvm;
            bool isInSub;
            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
            if(dir == MDIR_J){
                tvp = vp - pvs.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
                if(mt != mmt){
                    tvm = vm - pvRs.value(key);
                    //if(isFst)
                    //    tvm = vm - preFExaR;
                    //else
                    //    tvm = vm -preSExaR;
                }

            }
            else{
                tvp = pvRs.value(key) - vp; //借方（期初余额） - 贷方（当前发生借贷相抵后）
                if(mt != mmt){
                    tvm = pvRs.value(key) - vm;
                    //if(isFst)
                    //    tvm = preFExaR - vm;
                    //else
                    //    tvm = preSExaR - vm;
                }
            }
            if(tvp > 0){ //余额在借方
                //如果是收入类科目，要将它固定为贷方
                if(smg->isSyClsSubject(sid,isInSub,isFst) && isInSub){
                    tvp.changeSign();
                    evs[key] = tvp;
                    eds[key] = MDIR_D;
                }
                else{
                    evs[key] = tvp;
                    eds[key] = MDIR_J;
                    if(mt != mmt){
                        evRs[key] = tvm;
                        //if(isFst)
                        //    endFExaR[key] = tvm;
                        //else
                        //    endSExaR[key] = tvm;
                    }
                }
            }
            else if(tvp < 0){ //余额在贷方
                //如果是费用类科目，要将它固定为借方
                if(smg->isSyClsSubject(sid,isInSub,isFst) && !isInSub){
                    evs[key] = tvp;
                    eds[key] = MDIR_J;
                }
                else{
                    tvp.changeSign();
                    evs[key] = tvp;
                    eds[key] = MDIR_D;
                    if(mt != mmt){
                        evRs[key] = tvm;
                        //if(isFst)
                        //    endFExaR[key] = tvm;
                        //else
                        //    endSExaR[key] = tvm;
                    }
                }
            }
            else{
                evs[key] = 0;
                eds[key] = MDIR_P;
                if(mt != mmt){
                    evRs[key] = tvm;
                    //if(isFst)
                    //    endFExaR[key] = tvm;    //因为原币余额为0，并不意味着本币余额也为0
                    //else
                    //    endSExaR[key] = tvm;
                }
            }
        }
    }
}
