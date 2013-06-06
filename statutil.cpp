#include <QDate>
#include <QDebug>
//#include <QObject>

#include "statutil.h"
#include "account.h"
#include "pz.h"
#include "dbutil.h"
#include "PzSet.h"

StatUtil::StatUtil(QList<PingZheng *> *pzs, Account *account):pzs(pzs),account(account)
{
    dbUtil = account->getDbUtil();
    if(pzs->isEmpty())
        return;
    PingZheng* pz = pzs->first();
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
    _clearDatas();
    if(!_readPreExtra()){
        QMessageBox::critical(0,QObject::tr("错误提示"),QObject::tr("在读取%1年%2的前期余额时发生错误！").arg(y).arg(m));
        return false;
    }
    if(!_statCurHappen()){
        QMessageBox::warning(0,QObject::tr("错误提示"),QObject::tr("在统计%1年%2的本期发生额时，发现凭证有误，请使用凭证集检错工具查看或调阅日志信息！").arg(y).arg(m));
        return false;
    }
    //qDebug()<<QString("cwfy-j:%1").arg(curJF.value(821).toString());
    //qDebug()<<QString("cwfy-hdsy-j:%1").arg(curJS.value(601).toString());

    //输出佳利的贷方数值
    qDebug()<<QString("(new)YS-nbjl-D: rmb=%1, usd=%2, usdR=%3")
              .arg(curDS.value(961).toString()).arg(curDS.value(962).toString())
              .arg(curDSM.value(962).toString());

    _calEndExtra();
    _calEndExtra(false);


    //计算各币种合计后各个科目的余额合计及其方向
    _calSumValue(true);
    _calSumValue(true,false);
    _calSumValue(false);
    _calSumValue(false,false);
    _calCurSumValue(true);
    _calCurSumValue(true,false);
    _calCurSumValue(false);
    _calCurSumValue(false,false);
    return true;
}

/**
 * @brief StatUtil::save
 *  保存本期余额
 * @return
 */
bool StatUtil::save()
{
    if(!dbUtil->saveExtraForPm(y,m,endFExa,endFDir,endSExa,endSDir))
        return false;
    if(!dbUtil->saveExtraForMm(y,m,endFExaM,endSExaM))
        return false;
    PzSetMgr* pzMgr = account->getPzSet();
    pzMgr->setExtraState(true);
    if(!dbUtil->setExtraState(y,m,true))
        return false;
    return true;
}



/**
 * @brief StatUtil::_clearDatas
 *  清除所有hash表内的与统计相关的数据
 */
void StatUtil::_clearDatas()
{
    preFExa.clear();preFDir.clear();
    preSExa.clear();preSDir.clear();
    preFExaM.clear();preSExaM.clear();
    sumPreFV.clear();sumPreFD.clear();
    sumPreSV.clear();sumPreSD.clear();

    curJF.clear();curJS.clear();
    curJFM.clear();curJSM.clear();
    curDF.clear();curDS.clear();
    curDFM.clear();curDSM.clear();
    sumCurJF.clear();sumCurJS.clear();
    sumCurDF.clear();sumCurDS.clear();

    endFExa.clear();endSExa.clear();
    endFExaM.clear();endSExaM.clear();
    endFDir.clear();endSDir.clear();
    sumEndFV.clear();sumEndFD.clear();
    sumEndSV.clear();sumEndSD.clear();
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

    FirstSubject* fsub;
    SecondSubject* ssub;
    QHash<int,Double> rates;
    account->getRates(y,m,rates);

    //Debug
    //FirstSubject* cwfySub = smg->getCwfySub();
    //SecondSubject* hdsySub = cwfySub->getChildSub(smg->getNameItem(QObject::tr("汇兑损益")));


    for(int i = 0; i < pzs->count(); ++i){
        pz = pzs->at(i);
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
            keyF = fsub->getId()*10+mt; /*qDebug()<<QString("keyF=%1").arg(keyF);*/
            keyS = ssub->getId()*10+mt;
            //如果是结转汇兑损益的凭证，则要进行专门的处理 则跳过非财务费用方的会计分录，因为这些要计入到外币部分
            if((pz->getPzClass() == Pzc_Jzhd) /*&& (fsub->isSameSub(cwfySub))*/){
                //对结转方即非财务费用方的会计分录，其值要计入对应科目的对应外币上（因为调整的是该科目对应外币的本币值）
                //如果要支持多个外币，则必须有一个机制来辨识该会计分录结转的是对应哪个外币
                //目前为了简化，始终认为是美金
                if(fsub->isUseForeignMoney()){
                    int key = fsub->getId()*10 + USD;
                    curDFM[key] += ba->getValue();
                    if(!curJFM.contains(key))
                        curJFM[key] = Double(0.0);
                    //这样做是为了记录此科目的外币本期发生了（尽管发生的不是外币的原币形式）
                    //因为在计算期末余额时，是以本期发生值表来迭代的，如果不这样做，将会漏掉本期只发生了汇兑损益结转的科目的统计
                    //比如对于应收账户的某个客户对应的科目，本期没有发生业务往来，但期末汇率发生了变化，必须
                    //进行结转，这是下面的就是为了避免这种遗漏情况的发生
                    if(!curDF.contains(key))
                        curDF[key] = 0.0;
                    if(!curJF.contains(key))
                        curJF[key] = 0.0;

                    key = ssub->getId()*10+USD;
                    curDSM[key] += ba->getValue();
                    if(!curJSM.contains(key))
                        curJSM[key] = Double(0.0);
                    if(!curDS.contains(key))
                        curDS[key] = 0.0;
                    if(!curJS.contains(key))
                        curJS[key] = 0.0;
                }
                else{ //财务费用方（最好能够检测是否是财务费用科目）
                    curJF[keyF] += ba->getValue();
                    if(!curDF.contains(keyF))
                        curDF[keyF] = Double(0.00);
                    curJS[keyS] += ba->getValue();
                    if(!curDS.contains(keyS))
                        curDS[keyS] = Double(0.00);
                    //qDebug()<<QString("cwfy-hdsy-j:%1").arg(curJS.value(601).toString());
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
                        curJFM[keyF] += v;
                        if(!curDFM.contains(keyF))
                            curDFM[keyF] = Double(0.0);
                        curJSM[keyS] += v;
                        if(!curDSM.contains(keyS))
                            curDSM[keyS] = Double(0.0);
                    }
                    //debug
                    //if(ssub->getId() == hdsySub->getId())
                    //    qDebug()<<QString("cwfy-hdsy-j:%1").arg(ba->getValue().toString());
                }
                else{       //发生在贷方
                    curDF[keyF] += ba->getValue();
                    if(!curJF.contains(keyF)) //这是为了确保jSums和dSums的key集合相同
                        curJF[keyF] = Double(0.00);
                    curDS[keyS] += ba->getValue();
                    if(!curJS.contains(keyS))
                        curJS[keyS] = Double(0.00);
                    if(mt != masterMt->code()){
                        v = ba->getValue() * rates.value(mt);
                        curDFM[keyF] += v;
                        if(!curJFM.contains(keyF))
                            curJFM[keyF] = Double(0.0);
                        curDSM[keyS] += v;
                        if(!curJSM.contains(keyS))
                            curJSM[keyS] =  Double(0.0);
                    }
                    //debug
                    //if(ssub->getId() == hdsySub->getId())
                    //    qDebug()<<QString("cwfy-hdsy-j:%1").arg(ba->getValue().toString());
                }
            }
            //qDebug()<<QString("cwfy-j:%1").arg(curJF.value(821).toString());
            //qDebug()<<QString("cwfy-hdsy-j:%1").arg(curJS.value(601).toString());
        }
    }
    return true;
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

    if(!dbUtil->readExtraForPm(yy,mm,preFExa,preFDir,preSExa,preSDir))
        return false;
    if(!dbUtil->readExtraForMm(yy,mm,preFExaM,preSExaM))
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

    //变量命名约定（p：期初，v：金额，j：借方，d：贷方或方向，c：本期发生，e：期末，M：本币值）
    QHash<int,Double> *pvs,*pvMs,*cjvs,*cjMvs,*cdvs,*cdMvs,*evs,*evMs;
    QHash<int, MoneyDirection> *pds,*eds;
    if(isFst){
        pvs = &preFExa;
        pvMs = &preFExaM;
        pds = &preFDir;
        cjvs = &curJF;
        cjMvs = &curJFM;
        cdvs = &curDF;
        cdMvs = &curDFM;
        evs = &endFExa;
        evMs = &endFExaM;
        eds = &endFDir;
    }
    else{
        pvs = &preSExa;
        pvMs = &preSExaM;
        pds = &preSDir;
        cjvs = &curJS;
        cjMvs = &curJSM;
        cdvs = &curDS;
        cdMvs = &curDSM;
        evs = &endSExa;
        evMs = &endSExaM;
        eds = &endSDir;
    }

//    evs->clear(); eds->clear(); evMs->clear();

    Double vp,vm;   //原币金额、本币金额
    MoneyDirection dir;
    int sid,key,mt;
    int mmt = masterMt->code();
    QHashIterator<int,Double> it(*cjvs);
    while(it.hasNext()){
        it.next();
        key = it.key();
        if(key == 962)
            int ii = 0;
        sid = key/10;
        mt = key%10;
        vp = cjvs->value(key) - cdvs->value(key);
        if(mt != mmt)
            vm = cjMvs->value(key) - cdMvs->value(key);
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
            (*evs)[key] = pvs->value(key);
            (*eds)[key] = pds->value(key);
            if(mt != mmt){
                (*evMs)[key] = pvMs->value(key)+vm;
                //if(isFst)
                //    evRs[key] = pvRs.value(key);
                //else
                //    endSExaR = preSExaR;
            }
        }
        else if(pds->value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
            (*evs)[key] = pvs->value(key) + vp;
            (*eds)[key] = pds->value(key);
            if(mt != mmt){
                (*evMs)[key] = pvMs->value(key) + vm;
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
                tvp = vp - pvs->value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
                if(mt != mmt){
                    tvm = vm - pvMs->value(key);
                    //if(isFst)
                    //    tvm = vm - preFExaR;
                    //else
                    //    tvm = vm -preSExaR;
                }

            }
            else{
                tvp = pvs->value(key) - vp; //借方（期初余额） - 贷方（当前发生借贷相抵后）
                if(mt != mmt){
                    tvm = pvMs->value(key) - vm;
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
                    (*evs)[key] = tvp;
                    (*eds)[key] = MDIR_D;
                }
                else{
                    (*evs)[key] = tvp;
                    (*eds)[key] = MDIR_J;
                    if(mt != mmt){
                        (*evMs)[key] = tvm;
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
                    (*evs)[key] = tvp;
                    (*eds)[key] = MDIR_J;
                }
                else{
                    tvp.changeSign();
                    tvm.changeSign();
                    (*evs)[key] = tvp;
                    (*eds)[key] = MDIR_D;
                    if(mt != mmt){
                        (*evMs)[key] = tvm;
                        //if(isFst)
                        //    endFExaR[key] = tvm;
                        //else
                        //    endSExaR[key] = tvm;
                    }
                }
            }
            else{
                (*evs)[key] = 0;
                (*eds)[key] = MDIR_P;
                if(mt != mmt){
                    (*evMs)[key] = tvm;
                    //if(isFst)
                    //    endFExaR[key] = tvm;    //因为原币余额为0，并不意味着本币余额也为0
                    //else
                    //    endSExaR[key] = tvm;
                }
            }
        }
    }

    if(!isFst)
        qDebug()<<QString("StatUtil::_calEndExtra===> %1").arg(evMs->value(1122).toString());

    //将存在期初值但本期未发生的科目余额拷贝到期末余额
    QHashIterator<int,Double>* ip = new QHashIterator<int,Double>(*pvs);
    while(ip->hasNext()){
        ip->next();
        int key = ip->key();
        if(!evs->contains(key)){
            (*evs)[key] = pvs->value(key);
            (*eds)[key] = pds->value(key);
        }
    }

}

/**
 * @brief StatUtil::_calSumValue
 *  计算将各个科目下的各个币种合计后的余额及其方向
 * @param isPre true：期初，false：期末
 * @param isfst true：一级科目，false：二级科目
 */
void StatUtil::_calSumValue(bool isPre, bool isfst)
{
    QHash<int,Double> *vs,*vms,*sumvs; //未合计前的值表（其中：vs为原币值表，vms为本币值表），合计后的值表
    QHash<int,MoneyDirection> *ds,*sumds; //未合计前的方向表，合计后的方向表
    if(isPre){
        if(isfst){
            vs = &preFExa;
            vms= &preFExaM;
            ds = &preFDir;
            sumvs = &sumPreFV;
            sumds = &sumPreFD;
        }
        else{
            vs = &preSExa;
            vms= &preSExaM;
            ds = &preSDir;
            sumvs = &sumPreSV;
            sumds = &sumPreSD;
        }
    }
    else{
        if(isfst){
            vs = &endFExa;
            vms= &endFExaM;
            ds = &endFDir;
            sumvs = &sumEndFV;
            sumds = &sumEndFD;
        }
        else{
            vs = &endSExa;
            vms= &endSExaM;
            ds = &endSDir;
            sumvs = &sumEndSV;
            sumds = &sumEndSD;
        }
    }

    //基本思路是借方　－　贷方，并根据差值的符号来判断余额方向
    QHashIterator<int,Double>* it = new QHashIterator<int,Double>(*vs);
    int mt,sid;
    int mmt = masterMt->code();
    Double v;
    while(it->hasNext()){
        it->next();
        sid = it->key()/10;
        mt = it->key()%10;
        if(mt == mmt)
            v = it->value();
        else
            v = vms->value(it->key());
        if(ds->value(it->key()) == MDIR_P){
            if(sumvs->contains(sid))
                continue;
            else
                (*sumvs)[sid] = 0;
        }
        else if(ds->value(it->key()) == MDIR_J)
            (*sumvs)[sid] += v;
        else
            (*sumvs)[sid] -= v;
    }

    bool isIn ;  //是否是损益类凭证的收入类科目
    bool isFei;  //是否是损益类凭证的费用类科目
    FirstSubject* fsub; SecondSubject* ssub;
    it = new QHashIterator<int,Double>(*sumvs);
    while(it->hasNext()){
        it->next();
        sid = it->key();
        if(isfst){
            fsub = smg->getFstSubject(sid);
            ssub = NULL;
        }
        else{
            ssub = smg->getSndSubject(sid);
            fsub = ssub->getParent();
        }
        if(it->value() == 0)
            (*sumds)[sid] = MDIR_P;
        else if(it->value() > 0){
            isIn = (smg->isSySubject(fsub->getId()) && !fsub->getJdDir());
            //如果是收入类科目，要将它固定为贷方
            if(isIn){
                (*sumvs)[sid].changeSign();
                (*sumds)[sid] = MDIR_D;
            }
            else{
                (*sumds)[sid] = MDIR_J;
            }
        }
        else{
            isFei = (smg->isSySubject(fsub->getId()) && fsub->getJdDir());
            //如果是费用类科目，要将它固定为借方
            if(isFei){
                (*sumds)[sid] = MDIR_J;
                //为什么不需要更改金额符号，是因为对于一般的科目是根据符号来判断科目的余额方向
                //但对费用类科目，我们认为即使是负数也将其定为借方（因为运算法则是借方-贷方，方向仍为借方）
                //sumsR[id].changeSign();
            }
            else{
                (*sumvs)[sid].changeSign();
                (*sumds)[sid] = MDIR_D;
            }
        }
    }
}

/**
 * @brief StatUtil::_calCurSumValue
 *  计算本期发生额各个科目下的各币种合计值
 * @param isJ
 * @param isFst
 */
void StatUtil::_calCurSumValue(bool isJ, bool isFst)
{
    QHash<int,Double> *vs,*vms,*sumvs;
    if(isJ){
        if(isFst){
            vs = &curJF;
            vms = &curJFM;
            sumvs = &sumCurJF;
        }
        else{
            vs = &curJS;
            vms = &curJSM;
            sumvs = &sumCurJS;
        }
    }
    else{
        if(isFst){
            vs = &curDF;
            vms = &curDFM;
            sumvs = &sumCurDF;
        }
        else{
            vs = &curDS;
            vms = &curDSM;
            sumvs = &sumCurDS;
        }
    }

    int sid,mt;
    int mmt = masterMt->code();
    Double v;
    QHashIterator<int,Double> it(*vs);
    while(it.hasNext()){
        it.next();
        sid = it.key()/10;
        mt = it.key()%10;
        if(mt == mmt)
            v = it.value();
        else
            v = vms->value(it.key());
        (*sumvs)[sid] += v;
    }
}
