#include "global.h"
#include "tables.h"
#include "PzSet.h"
#include "pz.h"
#include "dbutil.h"
#include "statutil.h"



/////////////////PzSetMgr///////////////////////////////////////////
PzSetMgr::PzSetMgr(Account *account, User *user, QObject *parent):QObject(parent),
    account(account),user(user),curY(0),curM(0)
{
    dbUtil = account->getDbUtil();
    undoStack = new QUndoStack(this);
    undoStack->setUndoLimit(MAXUNDOSTACK);
    statUtil = NULL;
    c_recording=0;c_verify=0;c_instat=0;c_repeal=0;
    //isReStat = false;
    //isReSave = false;
    maxPzNum = 0;
    maxZbNum = 0;
    curPz=NULL;
    curIndex = -1;
    curY=0;curM=0;
    pzs=NULL;
    dirty = false;
    if(!user)
        user = curUser;
}

PzSetMgr::~PzSetMgr()
{
    delete undoStack;
}

//打开凭证集
bool PzSetMgr::open(int y, int m)
{
    if(curY!=0 && curM!=0)    //同时只能打开一个凭证集
        return false;

    int key = genKey(y,m);
    if(!pzSetHash.contains(key)){
        if(!dbUtil->loadPzSet(y,m,pzSetHash[key],this))
            return false;
        if(!dbUtil->getPzsState(y,m,states[key]))
            return false;        
    }    
    curY=y,curM=m;
    pzs = &pzSetHash[key];
    scanPzCount();
    //凭证集的状态以实际凭证为准
    PzsState state;
    _determinePzSetState(state);
    if(state != states.value(key)){
        states[key] = state;
        dirty = true;
    }
    extraStates[key] = dbUtil->getExtraState(y,m);    
    for(int i = 0; i < pzs->count(); ++i){
        PingZheng* pz = pzs->at(i);
        connect(pz,SIGNAL(mustRestat()),this,SLOT(needRestat()));
        connect(pz,SIGNAL(pzContentChanged(PingZheng*)),this,SLOT(pzChangedInSet(PingZheng*)));
        connect(pz,SIGNAL(pzStateChanged(PzState,PzState)),this,SLOT(pzStateChanged(PzState,PzState)));
    }

    maxPzNum = pzSetHash.value(key).count() + 1;
    maxZbNum = 0;
    foreach(PingZheng* pz, pzSetHash.value(key)){
        if(maxZbNum < pz->number())
            maxZbNum = pz->number();
    }
    maxZbNum++;
    if(!statUtil)
        delete statUtil;
    statUtil = new StatUtil(pzs,account);
    if(!pzs->isEmpty()){
        curPz = pzs->first();
        curIndex = 0;
        emit currentPzChanged(curPz,NULL);
    }
    emit pzCountChanged(pzs->count());
    return true;
}

/**
 * @brief PzSetMgr::isOpen
 *  凭证集是否已被打开
 * @return
 */
bool PzSetMgr::isOpened()
{
    return (curY!=0 && curM!=0);
}

/**
 * @brief PzSetMgr::isDirty
 *  凭证集是否有未保存的更改
 * @return
 */
bool PzSetMgr::isDirty()
{
    //要考虑的方面包括（凭证集内的凭证、余额状态、凭证集状态）
    return (dirty || !undoStack->isClean());
}


void PzSetMgr::close()
{
    //save();
    undoStack->clear();
    for(int i = 0; i < pzs->count(); ++i){
        PingZheng* pz = pzs->at(i);
        disconnect(pz,SIGNAL(mustRestat()),this,SLOT(needRestat()));
        disconnect(pz,SIGNAL(pzContentChanged(PingZheng*)),this,SLOT(pzChangedInSet(PingZheng*)));
        disconnect(pz,SIGNAL(pzStateChanged(PzState,PzState)),this,SLOT(pzStateChanged(PzState,PzState)));
    }
    qDeleteAll(pz_dels);
    pz_dels.clear();
    qDeleteAll(cachedPzs);
    cachedPzs.clear();
    curY=0;curM=0;
    c_recording=0;c_verify=0;c_instat=0;c_repeal=0;
    maxPzNum = 0;
    maxZbNum = 0;
    //isReStat = false;
    //isReSave = false;
    delete statUtil;
    pzs=NULL;
    PingZheng* oldPz = curPz;
    curPz=NULL;
    curIndex = -1;
    _determineCurPzChanged(oldPz);
    emit pzCountChanged(0);
}

/**
 * @brief PzSetMgr::getStatObj
 *  获取当前打开凭证集的本期统计对象的引用
 * @return
 */
StatUtil &PzSetMgr::getStatObj()
{
    return *statUtil;
}

//获取凭证总数（也即已用的最大凭证号）
int PzSetMgr::getPzCount()
{
    if(curY==0 && curM==0)
        return 0;
    return maxPzNum-1;
}

/**
 * @brief PzSetMgr::getAllJzhdPzs
 *  返回所有结转汇兑损益的凭证对象列表
 * @return
 */
QList<PingZheng *> PzSetMgr::getAllJzhdPzs()
{
    QList<PingZheng*> pzLst;
    if(curY==0 && curM==0)
        return pzLst;
    foreach(PingZheng* pz, *pzs){
        if(pz->getPzClass() == Pzc_Jzhd)
            pzLst<<pz;
    }
    return pzLst;
}

//重置凭证号
bool PzSetMgr::resetPzNum(int by)
{
//    if(state == Ps_NoOpen)
//        return true;

//    //1：表示按日期顺序，2：表示按自编号顺序
//    if(by == 1){
//        qSort(pds.begin(),pds.end(),byDateLessThan);
//        for(int i = 0; i < pds.count(); ++i){
//            pds[i]->setNumber(i+1);
//            //pds[i]->setEditState(PingZheng::INFOEDITED);
//        }
//        return true;
//    }
//    if(by == 2){
//        qSort(pds.begin(),pds.end(),byZbNumLessThan);
//        for(int i = 0; i < pds.count(); ++i){
//            pds[i]->setNumber(i+1);
//            //pds[i]->setEditState(PingZheng::INFOEDITED);
//        }
//        return true;
//    }
//    else
//        return false;
    return true;
}

/**
 * @brief PzSetMgr::getState
 *  返回凭证集状态，如果年月为0，则返回当前打开的凭证集的状态
 * @param y
 * @param m
 * @return
 */
PzsState PzSetMgr::getState(int y, int m)
{
    int yy,mm;
    if(y==0 && m == 0){
        yy = curY; mm = curM;
    }
    else{
        yy = y; mm = m;
    }
    int key = genKey(yy,mm);
    if(states.contains(key))
        return states.value(key);
    return Ps_NoOpen;
}

//设置凭证集状态
void PzSetMgr::setState(PzsState state,int y, int m )
{
    int yy,mm;
    if(y==0 && m==0){
        yy=curY;mm=curM;
    }
    else{
        yy=y;mm=m;
    }
    int key = genKey(yy,mm);
    if(!states.contains(key))
        dbUtil->getPzsState(y,m,state);
    if(state == states.value(key))
        return;
    states[key] = state;
    dirty = true;
}

/**
 * @brief PzSetMgr::getExtraState
 *  获取余额状态（如果年月都为0，则获取当前打开凭证集的余额状态）
 * @param y
 * @param m
 * @return
 */
bool PzSetMgr::getExtraState(int y, int m)
{
    int yy,mm;
    if(y==0 && m==0){
        yy=curY,mm=curM;
    }
    int key = genKey(yy,mm);
    if(!extraStates.contains(key))
        extraStates[key] = dbUtil->getExtraState(yy,mm);
    return extraStates.value(key);
}

/**
 * @brief PzSetMgr::setExtraState
 *  设置余额状态（如果年月都为0，则设置当前打开凭证集的余额状态）
 * @param state
 * @param y
 * @param m
 */
void PzSetMgr::setExtraState(bool state, int y, int m)
{
    int yy,mm;
    if(y==0 && m==0){
        yy=curY;mm=curM;
    }
    else{
        yy = y;mm = m;
    }
    int key = genKey(yy,mm);
    if(!extraStates.contains(key))
        extraStates[key] = dbUtil->getExtraState(yy,mm);
    if(state == extraStates.value(key))
        return;
    extraStates[key] = state;
    dirty = true;
    emit pzExtraStateChanged(state);
}

/**
 * @brief PzSetMgr::getPzSet
 *  获取指定年月的凭证集
 * @param y
 * @param m
 */
bool PzSetMgr::getPzSet(int y, int m, QList<PingZheng *> &pzs)
{
    int key = genKey(y,m);
    if(!pzSetHash.contains(key)){
        if(!dbUtil->loadPzSet(y,m,pzSetHash[key],this))
            return false;
        if(!dbUtil->getPzsState(y,m,states[key]))
            return false;
        extraStates[key] = dbUtil->getExtraState(y,m);
    }
    pzs = pzSetHash.value(key);
    return true;
}

/**
 * @brief PzSetMgr::getPzSpecRange
 *  获取指定凭证号集合的凭证对象列表
 * @param y
 * @param m
 * @param nums  凭证号集合（集合为空，则返回凭证集内的所有凭证）
 * @return
 */
QList<PingZheng *> PzSetMgr::getPzSpecRange(QSet<int> nums)
{
    QList<PingZheng*> pzLst;
    if(!isOpened())
        return pzLst;
    if(nums.isEmpty())
        pzLst = *pzs;
    else{
        foreach(PingZheng* pz, *pzs){
            if(nums.contains(pz->number()))
                pzLst<<pz;
        }
    }
    return pzLst;
}

/**
 * @brief PzSetMgr::contains
 *  在指定年月的凭证集内是否包含了指定id的凭证
 * @param pid
 * @param y
 * @param m 
 * @return
 */
bool PzSetMgr::contains(int pid, int y, int m)
{
    if(isOpened() && ((y == 0 && m == 0) || (y == curY && m == curM))){
        foreach(PingZheng* pz, *pzs){
            if(pid == pz->id())
                return true;
        }
        return false;
    }
    else
        return dbUtil->isContainPz(y,m,pid);
}

/**
 * @brief PzSetMgr::readPz
 *  获取指定id的凭证
 *  此方法可以用来得到任意时间点的凭证（比如在明细账视图的某个发生行上选择转到该凭证，则要调用该方法来获取历史凭证）
 * @param pid
 * @param in    如果凭证在当前打开的凭证集内，则为true，否则是历史凭证
 * @return  如果为空则表示凭证不存在
 */
PingZheng *PzSetMgr::readPz(int pid, bool& in)
{
    foreach(PingZheng* pz, *pzs){
        if(pz->id() == pid){
            in = true;
            return pz;
        }
    }

    foreach(PingZheng* pz, historyPzs){
        if(pz->id() == pid){
            in = false;
            return pz;
        }
    }
    PingZheng* pz;
    if(!dbUtil->getPz(pid,pz,this)){
        QMessageBox::critical(0,tr("错误信息"),tr("读取id=%1的凭证时发生错误！").arg(pid));
        return NULL;
    }
    in = false;
    return pz;
}

/**
 * @brief PzSetMgr::getStatePzCount 返回凭证集内指定状态凭证数
 * @param st
 * @return
 */
int PzSetMgr::getStatePzCount(PzState state)
{
    int c = 0;
    foreach(PingZheng* pz, *pzs)
        if(pz->getPzState() == state)
            c++;
    return c;
}

/**
 * @brief PzSetMgr::inspectPzError
 *  凭证集检错
 *  （1）凭证号连续性
 *  （2）自编号是否为空
 *  （3）自编号是否重复
 *  （4）借贷是否平衡
 *  （5）摘要栏是否为空
 *  （6）科目是否未设值
 *  （7）金额是否未设值
 *  （8）借贷方向是否违反约定性
 *  （9）币种设置是否与科目的货币使用属性相符
 * @return
 */
bool PzSetMgr::inspectPzError(QList<PingZhengError*>& errors)
{
    if(!isOpened())
        return false;
    errors.clear();

    PingZhengError* e;
    PingZheng* pz;
    BusiAction* ba;
    QSet<int> zbNums;
    int pzNum = 0;
    QString errStr;
    for(int i = 0; i < pzs->count(); ++i){
        pz = pzs->at(i);
        //（1）凭证号连续性
        if((pz->number() - pzNum) != 1){
            e = new PingZhengError;
            e->errorLevel = PZE_WARNING;
            e->errorType = 2;
            e->pz = pz;
            e->ba = NULL;
            e->explain = tr("在%1号凭证前存在不连续的凭证号！").arg(pz->number());
            errors<<e;
        }
        pzNum = pz->number();
        //（2）自编号是否为空
        if(pz->zbNumber() == 0){
            e = new PingZhengError;
            e->errorLevel = PZE_WARNING;
            e->errorType = 1;
            e->pz = pz;
            e->ba = NULL;
            e->explain = tr("%1号凭证的自编号未设置！").arg(pz->number());
            errors<<e;
        }
        else{
            //（3）自编号是否重复
            if(zbNums.contains(pz->zbNumber())){
                e = new PingZhengError;
                e->errorLevel = PZE_WARNING;
                e->errorType = 3;
                e->pz = pz;
                e->ba = NULL;
                e->explain = tr("%1号凭证的自编号与其他凭证重复！").arg(pz->number());
                errors<<e;
            }
            else
                zbNums.insert(pz->zbNumber());
        }
        //（4）借贷是否平衡
        if(!pz->isBalance()){
            e = new PingZhengError;
            e->errorLevel = PZE_ERROR;
            e->errorType = 1;
            e->pz = pz;
            e->ba = NULL;
            e->explain = tr("%1号凭证借贷不平衡！").arg(pz->number());
            errors<<e;
        }
        for(int j = 0; j < pz->baCount(); ++j){
            ba = pz->getBusiAction(j);
            //（5）摘要栏是否为空
            if(ba->getSummary().isEmpty()){
                e = new PingZhengError;
                e->errorLevel = PZE_WARNING;
                e->errorType = 4;
                e->pz = pz;
                e->ba = ba;
                e->explain = tr("%1号凭证第%2条会计分录没有设置摘要！").arg(pz->number()).arg(ba->getNumber());
                errors<<e;
            }
            //（6）科目是否未设值
            if(!ba->getFirstSubject() || !ba->getSecondSubject()){
                e = new PingZhengError;
                e->errorLevel = PZE_ERROR;
                e->errorType = 2;
                e->pz = pz;
                e->ba = ba;
                e->explain = tr("%1号凭证第%2条会计分录科目未设置！").arg(pz->number()).arg(ba->getNumber());
                errors<<e;
            }
            //（7）金额是否未设值
            if(ba->getValue() == 0.0){
                e = new PingZhengError;
                e->errorLevel = PZE_ERROR;
                e->errorType = 5;
                e->pz = pz;
                e->ba = ba;
                e->explain = tr("%1号凭证第%2条会计分录金额未设置！").arg(pz->number()).arg(ba->getNumber());
                errors<<e;
            }
            //（8）借贷方向是否违反约定性
            if(!_inspectDirEngageError(ba->getFirstSubject(),ba->getDir(),pz->getPzClass(),errStr)){
                e = new PingZhengError;
                e->errorLevel = PZE_ERROR;
                e->errorType = 4;
                e->pz = pz;
                e->ba = ba;
                e->explain = errStr;
                errors<<e;
            }
            //（9）币种设置是否与科目的货币使用属性相符
            if(ba->getFirstSubject() && !ba->getFirstSubject()->isUseForeignMoney() &&
                    (ba->getMt() != account->getMasterMt())){
                e = new PingZhengError;
                e->errorLevel = PZE_ERROR;
                e->errorType = 3;
                e->pz = pz;
                e->ba = ba;
                e->explain = tr("%1号凭证第%2条会计分录在不允许使用外币的科目上使用了外币").arg(pz->number()).arg(ba->getNumber());
                errors<<e;
            }
        }






    }
    return true;
}

//保存当期余额
bool PzSetMgr::PzSetMgr::saveExtra()
{
    if(!isOpened())
        return false;
    if(getExtraState())
        return true;
    if(!statUtil->save())
        return false;
    //setExtraState(true);
    //if(!dbUtil->setExtraState(curY,curM,true))
    //    return false;
    return true;
}

//读取当期余额（读取的是最近一次保存到余额表中的数据）
bool PzSetMgr::readExtra()
{
    endExtra.clear();
    endDetExtra.clear();
    endDir.clear();
    endDetDir.clear();

    //if(!BusiUtil::readExtraByMonth(y,m,endExtra,endDir,endDetExtra,endDetDir))
    //    return false;

}

//读取期初（前期）余额
bool PzSetMgr::readPreExtra()
{
    preExtra.clear();
    preDetExtra.clear();
    int yy,mm;
    if(curM == 1){
        yy = curY - 1;
        mm = 12;
    }
    else{
        yy = curY;
        mm = curM - 1;
    }
    return dbUtil->readExtraForPm(yy,mm,preExtra,preDir,preDetExtra,preDetDir);
}

/**
 * @brief PzSetMgr::getRates
 *  获取当前打开凭证集所使用的汇率表
 * @return
 */
bool PzSetMgr::getRates(QHash<int, Double>& rates, int y, int m)
{
    if(curY==0 && curM==0)
        return false;
    if(y==0 && m == 0){
        y = curY; m = curM;
    }
    if(!account->getRates(y,m,rates)){
        QMessageBox::critical(0,tr("错误信息"),tr("在读取%1年%月的汇率时出错！").arg(y).arg(m));
        return false;
    }
    return true;
}

/**
 * @brief PzSetMgr::setRates
 * @param rates
 * @param y
 * @param m
 * @return
 */
bool PzSetMgr::setRates(QHash<int, Double> rates, int y, int m)
{
    if(curY == 0 && curM == 0)
        return false;
    if(y == 0 && m == 0){
        y = curY; m = curM;
    }
    if(!account->setRates(y,m,rates)){
        QMessageBox::critical(0,tr("错误信息"),tr("在保存%1年%月的汇率时出错！").arg(y).arg(m));
        return false;
    }
    return true;
}

/**
 * @brief PzSetMgr::first 定位到第一个凭证对象
 * @return
 */
PingZheng *PzSetMgr::first()
{
    if(!isOpened()){
        LOG_ERROR(QObject::tr("pzSet not opened!"));
        return NULL;
    }
    PingZheng* oldPz = curPz;
    if(pzs->empty()){
        curPz = NULL;
        curIndex = -1;
    }
    else{
        curIndex = 0;
        curPz = pzs->first();
    }
    _determineCurPzChanged(oldPz);
    return curPz;
}

/**
 * @brief PzSetMgr::next 定位到下一个凭证对象
 * @return
 */
PingZheng *PzSetMgr::next()
{
    if(!isOpened()){
        LOG_ERROR(QObject::tr("pzSet not opened!"));
        return NULL;
    }
    PingZheng* oldPz = curPz;
    if(pzs->empty()){
        curPz = NULL;
        curIndex = -1;
    }
    else{
        if(curIndex == pzs->count()-1){
            curPz = NULL;
            curIndex = -1;
        }
        else
            curPz = pzs->at(++curIndex);
    }
    _determineCurPzChanged(oldPz);
    return curPz;
}

/**
 * @brief PzSetMgr::previou 定位到上一个凭证对象
 * @return
 */
PingZheng *PzSetMgr::previou()
{
    if(!isOpened()){
        LOG_ERROR(QObject::tr("pzSet not opened!"));
        return NULL;
    }
    PingZheng* oldPz = curPz;
    if(pzs->empty()){
        curPz = NULL;
        curIndex = -1;
    }
    else{
        if(curIndex == 0){
            curIndex = -1;
            curPz = NULL;
        }
        else{
            curIndex--;
            curPz = pzs->at(curIndex);
        }
    }
    _determineCurPzChanged(oldPz);
    return curPz;
}

/**
 * @brief PzSetMgr::last 定位到最后一个凭证对象
 * @return
 */
PingZheng *PzSetMgr::last()
{
    if(!isOpened()){
        LOG_ERROR(QObject::tr("pzSet not opened!"));
        return NULL;
    }
    PingZheng* oldPz = curPz;
    if(pzs->empty()){
        curIndex = -1;
        curPz = NULL;
    }
    else{
        curIndex = pzs->count() - 1;
        curPz = pzs->last();
    }
    _determineCurPzChanged(oldPz);
    return curPz;
}

/**
 * @brief PzSetMgr::seek 定位到指定凭证号的凭证对象
 * @param num
 * @return
 */
PingZheng *PzSetMgr::seek(int num)
{
    PingZheng* oldPz = curPz;
    if((num < 1) || num > pzs->count())
        return NULL;
    else{
        curPz = pzs->at(num-1);
        curIndex = num-1;
    }
    _determineCurPzChanged(oldPz);
    return curPz;
}

/**
 * @brief PzSetMgr::seek
 *  查找并定位到凭证集内的指定凭证
 * @param pz
 * @return 如果正确定位则返回true，反之返回false
 */
bool PzSetMgr::seek(PingZheng *pz)
{
    PingZheng* oldPz = curPz;
    PingZheng* p;
    for(int i = 0; i < pzs->count(); ++i){
        p = pzs->at(i);
        if(*p == *pz){
            curPz = pz;
            curIndex = i;
            _determineCurPzChanged(oldPz);
            return true;
        }
    }
    return false;
}

//添加空白凭证
PingZheng* PzSetMgr::appendPz(PzClass pzCls)
{
    if(curY==0 && curM==0)
        return NULL;
    PingZheng* oldPz = curPz;
    QString ds = QDate(curY,curM,1).toString(Qt::ISODate);
    curPz =  new PingZheng(this,0,ds,maxPzNum++,maxZbNum++,0.0,0.0,pzCls,0,Pzs_Recording);
    c_recording++;
    setState(Ps_Rec);
    *pzs<<curPz;
    curIndex = pzs->count()-1;
    emit currentPzChanged(curPz,oldPz);
    emit pzCountChanged(pzs->count());
    emit pzSetChanged();
    return curPz;
}

/**
 * @brief PzSetMgr::append
 *  添加指定凭证到当前打开的凭证集中
 * @param pz
 * @return
 */
bool PzSetMgr::append(PingZheng *pz)
{
    if(!pz)
        return false;
    if(!isOpened()){
        LOG_ERROR(QObject::tr("pzSet not opened,don't' append pingzheng"));
        return false;
    }
    if(restorePz(pz))
        return true;
    *pzs<<pz;
    pz->setNumber(maxPzNum++);
    pz->setZbNumber(maxZbNum++);
    PingZheng* oldPz = curPz;
    curPz = pz;
    curIndex = pz->number()-1;
    switch(pz->getPzState()){
    case Pzs_Recording:
        c_recording++;
        setState(Ps_Rec);
        break;
    case Pzs_Verify:
        c_verify++;
        break;
    case Pzs_Instat:
        c_instat++;
        break;
    case Pzs_Repeal:
        c_repeal++;
        break;
    }
    if(pz->baCount() > 0)
        setExtraState(false);
    _determineCurPzChanged(oldPz);
    _determinePzSetState(states[genKey(curY,curM)]);
    emit pzCountChanged(pzs->count());
    emit pzSetChanged();
    return true;
}

/**
 * @brief PzSetMgr::insert
 *  插入凭证
 * @param pd
 * @return
 */
bool PzSetMgr::insert(PingZheng* pz)
{
    //插入凭证要保证凭证集内凭证号和自编号的连贯性要求,参数ecode错误代码（1：凭证号越界，2：自编号冲突）
    //凭证号必须从1开始，顺序增加，中间不能间断，自编号必须保证唯一性

    if(!pz)
        return false;
    if(!isOpened()){
        LOG_ERROR(tr("pzSet not opened,don't' insert pingzheng"));
        return false;
    }
    //凭证集内不能存在同一个凭证对象的多个拷贝
    for(int i = 0; i < pzs->count(); ++i){
        if(*pzs->at(i) == *pz)
            return false;
    }
    int index = pz->number()-1;
    if(index > pzs->count()){
        //errorStr = tr("when insert pingzheng to happen error,pingzheng number overflow!");
        //LOG_ERROR(errorStr);
        //return false;
        index = pzs->count();
        pz->setNumber(index+1);
    }

//    if(restorePz(pz))
//        return true;
    pz->setParent(this);
    PingZheng* oldPz = curPz;
    if(index == pzs->count())
        pzs->append(pz);
    else{
        pzs->insert(index,pz);
        curPz = pz;
        curIndex = index;
        maxPzNum++;
        maxZbNum++;
        //调整插入凭证后的凭证号
        index++;
        while(index < pzs->count()){
            pzs->at(index)->setNumber(index+1);
            ++index;
        }
    }
    switch(pz->getPzState()){
    case Pzs_Recording:
        c_recording++;
        setState(Ps_Rec);
        break;
    case Pzs_Verify:
        c_verify++;
        break;
    case Pzs_Instat:
        c_instat++;
        break;
    case Pzs_Repeal:
        c_repeal++;
        break;
    }
    if(pz->baCount() > 0)
        setExtraState(false);
    _determineCurPzChanged(oldPz);
    _determinePzSetState(states[genKey(curY,curM)]);
    emit pzCountChanged(pzs->count());
    emit pzSetChanged();
    return true;
}

/**
 * @brief PzSetMgr::insert
 *  在指定位置插入凭证
 * @param index
 * @param pz
 * @return
 */
bool PzSetMgr::insert(int index, PingZheng *pz)
{
    if(!isOpened()){
        LOG_ERROR(tr("pzSet not opened,don't' insert pingzheng"));
        return false;
    }
    if(!pz || ((index < 0) && (index > pzs->count())))
        return false;
    //凭证集内不能存在同一个凭证对象的多个拷贝
    for(int i = 0; i < pzs->count(); ++i){
        if(*pzs->at(i) == *pz)
            return false;
    }

    PingZheng* oldPz = curPz;
    pz->setNumber(index+1);
    pzs->insert(index,pz);
    if(index < pzs->count()-1)
    for(int i = index+1; i < pzs->count(); ++i)
        pzs->at(i)->setNumber(i+1);
    curPz = pz;
    curIndex = index;
    maxPzNum++;
    maxZbNum++;
    switch(pz->getPzState()){
    case Pzs_Recording:
        c_recording++;
        setState(Ps_Rec);
        break;
    case Pzs_Verify:
        c_verify++;
        break;
    case Pzs_Instat:
        c_instat++;
        break;
    case Pzs_Repeal:
        c_repeal++;
        break;
    }
    if(pz->baCount() > 0)
        setExtraState(false);
    _determineCurPzChanged(oldPz);
    _determinePzSetState(states[genKey(curY,curM)]);
    emit pzCountChanged(pzs->count());
    emit pzSetChanged();
    return true;
}

/**
 * @brief PzSetMgr::cachePz
 *  缓存指定凭证对象，以便以后在撤销删除该凭证又执行了保存操作时恢复
 * @param pz
 */
void PzSetMgr::cachePz(PingZheng *pz)
{
    pz->ID = UNID;
    foreach(BusiAction* ba, pz->baLst){
        ba->id = UNID;
    }
    cachedPzs<<pz;
}

/**
 * @brief PzSetMgr::isZbNumConflict
 *  自编号是否冲突
 * @param num
 * @return true：冲突，false：不冲突
 */
bool PzSetMgr::isZbNumConflict(int num)
{
    if(curY==0 && curM==0)
        return false;
    foreach(PingZheng* pz, *pzs)
        if(num == pz->zbNumber())
            return true;
    return false;
}

/**
 * @brief PzSetMgr::scanPzCount
 *  扫描打开凭证集内各种状态的凭证数
 */
void PzSetMgr::scanPzCount()
{
    if(curY==0 && curM==0)
        return;
    c_recording=0;c_verify=0;c_instat=0;c_repeal=0;
    foreach(PingZheng* pz, *pzs){
        switch(pz->getPzState()){
        case Pzs_Recording:
            c_recording++;
            break;
        case Pzs_Verify:
            c_verify++;
            break;
        case Pzs_Instat:
            c_instat++;
            break;
        case Pzs_Repeal:
            c_repeal++;
            break;
        }
    }
}

/**
 * @brief PzSetMgr::_confirmPzSetState
 *  根据当前打开凭证集内凭证的状态确定凭证集的状态
 * @param state
 */
void PzSetMgr::_determinePzSetState(PzsState &state)
{
    PzsState oldState = state;
    if(curY==0 && curM==0)
        state = Ps_NoOpen;
    else if(getState() == Ps_Jzed)
        state = Ps_Jzed;
    else{
        scanPzCount();
        if(c_recording > 0)
            state = Ps_Rec;
        else
            state = Ps_AllVerified;
    }
    if(oldState != state)
        emit pzSetStateChanged(state);
}

/**
 * @brief PzSetMgr::_determineCurPzChanged
 *  判断当前凭证是否改变，如果改变则触发相应信号
 * @param oldPz
 */
void PzSetMgr::_determineCurPzChanged(PingZheng *oldPz)
{
    if((!curPz && oldPz) || (curPz && !oldPz) || (*curPz != *oldPz))
        emit currentPzChanged(curPz,oldPz);
}

/**
 * @brief PzSetMgr::_inspectDirEngageError
 *  借贷方向约定正确性检测：
 *  1、手工凭证：对于损益类的收入型科目只能出现在贷方，成本型科目只能出现在借方；
 *  2、特种凭证：
 *  （1）结转汇兑损益类凭证：
 *      结转的科目（银行、应收和应付）在贷方，财务费用科目在借方；
 *  （2）结转损益类凭证：
 *      结转收入时，收入在借方，本年利润在贷方
 *      结转成本时，成本在贷方，本年利润在借方
 *  （3）结转利润类凭证：
 *      本年利润在借方，利润分配在贷方
 * @param fsub  分录的一级科目
 * @param dir   金额方向
 * @param pzc   凭证的类别
 * @param eStr  错误说明
 * @return
 */
bool PzSetMgr::_inspectDirEngageError(FirstSubject *fsub, MoneyDirection dir, PzClass pzc, QString &eStr)
{
    SubjectManager* sm = account->getSubjectManager(account->getCurSuite()->subSys);
    if(pzc == Pzc_Hand){
        if(fsub->getSubClass() == SC_SY){
            if(!fsub->getJdDir() && (dir == MDIR_J)){
                eStr = tr("手工凭证中，收入类科目必须在贷方");
                return false;
            }
            else if(fsub->getJdDir() && (dir == MDIR_D)){
                eStr = tr("手工凭证中，费用类科目必须在借方");
                return false;
            }
            else
                return true;
        }
        else
            return true;
    }
    if(pzc == Pzc_Jzhd){
        if(fsub->isUseForeignMoney() && (dir == MDIR_J)){
            eStr = tr("结转汇兑损益类凭证中，拟结转科目必须在贷方");
            return false;
        }
        else if((fsub == sm->getCwfySub()) && (dir == MDIR_D)){
            eStr = tr("结转汇兑损益类凭证中，财务费用科目必须在借方");
            return false;
        }
        else
            return true;
    }
    else if(pzc == Pzc_JzsyIn){
        if((fsub != sm->getBnlrSub()) && (fsub->getSubClass() != SC_SY) &&
                ((fsub->getSubClass() == SC_SY && fsub->getJdDir()))){
            eStr = tr("在结转损益（收入类）的凭证中，只能存在损益类（收入）科目或本年利润科目");
            return false;
        }
        else if((fsub == sm->getBnlrSub()) && (dir == MDIR_J)){
            eStr = tr("结转收入的凭证中，本年利润必须在贷方");
            return false;
        }
        else if((fsub != sm->getBnlrSub()) && !fsub->getJdDir() && (dir == MDIR_D)){
            eStr = tr("结转收入的凭证中，收入类科目必须在借方");
            return false;
        }

        else
            return true;
    }
    else if(pzc == Pzc_JzsyFei){
        if(fsub != sm->getBnlrSub() && (fsub->getSubClass() == SC_SY) &&
                ((fsub->getSubClass() == SC_SY && !fsub->getJdDir()))){
            eStr = tr("在结转损益（费用类）的凭证中，只能存在损益类（费用）科目或本年利润科目");
            return false;
        }
        else if((fsub == sm->getBnlrSub()) && (dir == MDIR_D)){
            eStr = tr("结转费用的凭证中，本年利润必须在借方");
            return false;
        }
        else if((fsub != sm->getBnlrSub()) && fsub->getJdDir() && (dir == MDIR_J)){
            eStr = tr("结转费用的凭证中，费用类科目必须在贷方");
            return false;
        }        
        else
            return true;
    }
    else if(pzc == Pzc_Jzlr){
        if((fsub == sm->getBnlrSub()) && (dir == MDIR_D)){
            eStr = tr("结转利润的凭证中，本年利润必须在借方");
            return false;
        }
        else if((fsub == sm->getLrfpSub()) && (dir == MDIR_J)){
            return false;
            eStr = tr("结转利润的凭证中，利润分配必须在贷方");
        }
        else
            return true;
    }
}



/**
 * @brief PzSetMgr::genKey
 *  生成由年月构成的键，用于索引凭证集
 * @param y
 * @param m
 * @return
 */
int PzSetMgr::genKey(int y, int m)
{
    if(m < 1 || m > 12)
        return 0;
    return y * 100 + m;
}

/**
 * @brief PzSetMgr::pzNotOpenWarning
 */
void PzSetMgr::pzsNotOpenWarning()
{
    QMessageBox::warning(0,tr("警告信息"),tr("凭证集未打开！"));
}

//移除凭证
bool PzSetMgr::remove(PingZheng *pz)
{
    if(!pz)
        return false;
    if(!pzs){
        LOG_ERROR(QObject::tr("pzSet not opened,don't' remove pingzheng"));
        return false;
    }
    int fonded = false;
    int i = 0;
    while(!fonded && i < pzs->count()){
        if(*pzs->at(i) == *pz){
            pzs->removeAt(i);
            fonded = true;
            pz->setParent(NULL);
            pz->setDeleted(true);
            pz_dels<<pz;
            break;
        }
        i++;
    }
    if(!fonded)
        return false;
    PingZheng* oldPz = curPz;
    if(pzs->isEmpty()){
        curPz = NULL;
        curIndex = -1;
        maxPzNum=1;
        maxZbNum=1;
        return true;
    }
    //调整移除凭证后的凭证号
    for(int j = i;j<pzs->count();++j)
        pzs->at(j)->setNumber(j+1);
    if(pz->number() == pzs->count()+1){ //移除的是最后一张凭证
        curPz = pzs->last();
        curIndex = pzs->count()-1;
    }
    else{
        curPz = pzs->at(i);
        curIndex = i;
    }
    maxPzNum--;
    maxZbNum--;
    switch(pz->getPzState()){
    case Pzs_Recording:
        c_recording--;
        break;
    case Pzs_Verify:
        c_verify--;
        break;
    case Pzs_Instat:
        c_instat--;
        break;
    case Pzs_Repeal:
        c_repeal--;
        break;
    }
    _determinePzSetState(states[genKey(curY,curM)]);
    _determineCurPzChanged(oldPz);
    emit pzCountChanged(pzs->count());
    emit pzSetChanged();
    return true;
}

/**
 * @brief PzSetMgr::restorePz
 * 恢复被删除的凭证（只能恢复被删除后未执行保存操作的凭证）
 * @param pz
 * @return
 */
bool PzSetMgr::restorePz(PingZheng *pz)
{
    //bool fonded = false;
    for(int i = 0; i < pz_dels.count(); ++i){
        if(*pz_dels.at(i) == *pz){
            pz_dels.takeAt(i);
            pz->setDeleted(false);
            return insert(pz);
        }
    }    
    for(int i = 0; i < cachedPzs.count(); ++i){
        if(*cachedPzs.at(i) == *pz){
            cachedPzs.takeAt(i);
            pz->setDeleted(false);
            return insert(pz);
        }
    }
    return false;
//    if(fonded){
//        PingZheng* oldPz = curPz;
//        int index = pz->number()-1;
//        if(index >= pzs->count()){
//            pzs->append(pz);
//            pz->setNumber(index+1);
//            curPz = pz;
//            curIndex = pzs->count()-1;
//        }
//        else{
//            pzs->insert(index,pz);
//            curPz = pz;
//            curIndex = pz->number()-1;
//            //调整插入凭证后的凭证号
//            index++;
//            while(index < pzs->count()){
//                pzs->at(index)->setNumber(index);
//                ++index;
//            }
//        }
//        switch(pz->getPzState()){
//        case Pzs_Recording:
//            c_recording++;
//            setState(Ps_Rec);
//            break;
//        case Pzs_Verify:
//            c_verify++;
//            break;
//        case Pzs_Instat:
//            c_instat++;
//            break;
//        case Pzs_Repeal:
//            c_repeal++;
//            break;
//        }
//        if(pz->baCount() > 0)
//            setExtraState(false);
//        if(curPz != oldPz)
//            emit currentPzChanged(curPz,oldPz);
//        emit pzCountChanged(pzs->count());
//        emit pzSetChanged();
//        return true;
//    }
//    return false;
}

/**
 * @brief PzSetMgr::getPz
 * 获取指定号的凭证对象
 * @param num
 * @return
 */
PingZheng *PzSetMgr::getPz(int num)
{
    if(!isOpened() || num < 1 || num > pzs->count())
        return NULL;
    return pzs->at(num-1);
}

/**
 * @brief PzSetMgr::setCurPz
 * 设置当前凭证
 * @param pz
 */
void PzSetMgr::setCurPz(PingZheng *pz)
{
    if(curPz == pz)
        return;
    PingZheng* oldPz = curPz;
    bool fonded = false;
    for(int i = 0; i < pzs->count(); ++i){
        if(pzs->at(i) == pz){
            fonded = true;
            curIndex = i;
            curPz = pz;
            break;
        }
    }
    if(!fonded){
        curIndex = -1;
        curPz = NULL;
    }
    _determineCurPzChanged(oldPz);
}

/**
 * @brief PzSetMgr::savePz
 *  保存单一凭证
 * @param pz
 * @return
 */
bool PzSetMgr::savePz(PingZheng *pz)
{
    return dbUtil->savePingZheng(pz);
}

/**
 * @brief PzSetMgr::savePz
 *  保存打开凭证集内的所有被修改和删除的凭证
 *  对应被删除的凭证，其对象仍将保存在缓存队列（pz_saveAfterDels）中，且对象ID复位
 *  当这些被删除且已实际删除的凭证复时，将以新凭证的面目出现
 * @param pzs
 * @return
 */
bool PzSetMgr::savePzSet()
{
    if(!dbUtil->savePingZhengs(*pzs))
        return false;
    if(!dbUtil->delPingZhengs(pz_dels))
        return false;
    foreach(PingZheng* pz, pz_dels){
        cachePz(pz);
    }
    pz_dels.clear();
    return true;
}

/**
 * @brief PzSetMgr::save
 *  保存凭证集相关的数据和信息（凭证、余额状态、凭证集状态等）
 * @return
 */
bool PzSetMgr::save(SaveWitch witch)
{
    switch(witch){
    case SW_ALL:
        if(!savePzSet())
            return false;
        if(!dbUtil->setPzsState(curY,curM,states.value(genKey(curY,curM))))
            return false;
        if(!dbUtil->setExtraState(curY,curM,extraStates.value(genKey(curY,curM))))
            return false;
        undoStack->setClean();
        dirty = false;
    case SW_PZS:
        if(!savePzSet())
            return false;
        undoStack->setClean();
        break;
    case SW_STATE:
        if(!dbUtil->setPzsState(curY,curM,getState()))
            return false;
        if(!dbUtil->setExtraState(curY,curM,getExtraState()))
            return false;
        break;
        dirty = false;
    }
    return true;
}

/**
 * @brief PzSetMgr::_crtJzhdsyPz
 *  创建指定月份结转汇兑损益的凭证
 * @param y             年
 * @param m             月
 * @param createdPzs    创建的凭证对象列表
 * @param sRate         期初汇率
 * @param erate         期末汇率
 * @param user          执行此操作的用户
 * @return
 */\
bool PzSetMgr::crtJzhdsyPz(int y, int m, QList<PingZheng *> &createdPzs, QHash<int, Double> sRate, QHash<int, Double> eRate, User *user)
{
    //计算汇率差
    QHash<int,Double> diffRates;
    QHashIterator<int,Double> it(sRate);
    while(it.hasNext()){
        it.next();
        Double diff = it.value() - eRate.value(it.key()); //期初汇率 - 期末汇率
        if(diff != 0)
            diffRates[it.key()] = diff;
    }
    if(diffRates.count() == 0)   //如果所有的外币汇率都没有变化，则无须进行汇兑损益的结转
        return true;

    //获取财务费用、及其下的汇兑损益科目id
    SubjectManager* subMgr = account->getSubjectManager(account->getSuite(y)->subSys);
    FirstSubject* cwfySub = subMgr->getCwfySub();
    SecondSubject* hdsySub = NULL;
    foreach(SecondSubject* ssub, cwfySub->getChildSubs()){
        if(ssub->getName() == tr("汇兑损益")){
            hdsySub = ssub;
            break;
        }
    }
    if(!hdsySub){
        QMessageBox::critical(0,tr("错误信息"),tr("不能获取到财务费用下的汇兑损益科目！"));
        return false;
    }

    //QList<Money*> mts;              //要结转的外币对象列表
    //mts = account->getAllMoneys().values();
   // mts.removeOne(account->getMasterMt());
    //Double ev,wv,sum;  //原币和本币形式的余额，合计值
    //MoneyDirection dir;

    QList<int> mtCodes;
    foreach(Money* mt, account->getAllMoneys()){
        if(mt != account->getMasterMt())
            mtCodes<<mt->code();
    }

    QDate d(year(),month(),1);
    d.setDate(year(),month(),d.daysInMonth());
    QString ds = d.toString(Qt::ISODate);

    PingZheng* pz;
    BusiAction* ba;
    QHash<int,Double> vs;
    QHash<int,MoneyDirection> dirs;
    QList<FirstSubject*> fsubs;
    Double sum,v;
    subMgr->getUseWbSubs(fsubs);
    int num=0;
    foreach(FirstSubject* fsub, fsubs){
        vs.clear();
        dirs.clear();
        if(!dbUtil->readAllWbExtraForFSub(y,m,fsub->getAllSSubIds(),mtCodes,vs,dirs))
            return false;
        if(vs.isEmpty())
            continue;
        num++;
        pz = new PingZheng(this);
        //pz->setNumber(maxPzNum + num);
        //pz->setZbNumber(maxZbNum + num);
        pz->setDate(ds);
        pz->setEncNumber(0);
        pz->setPzClass(Pzc_Jzhd);
        pz->setRecordUser(user);
        pz->setPzState(Pzs_Recording);
        QList<int> subIds;
        foreach(int key, vs.keys())
            subIds<<key / 10;
        subIds = QList<int>::fromSet(subIds.toSet());
        qSort(subIds.begin(),subIds.end());
        sum = 0.0;
        int key;
        for(int i = 0; i < subIds.count(); ++i){            
            MoneyDirection dir = fsub->getJdDir()?MDIR_J:MDIR_D;
            for(int j = 0; j < mtCodes.count(); ++j){
                ba = pz->appendBlank();
                ba->setFirstSubject(fsub);
                ba->setSecondSubject(subMgr->getSndSubject(subIds.at(i)));
                ba->setMt(account->getMasterMt());
                ba->setDir(MDIR_D);
                key = subIds.at(i) * 10 + mtCodes.at(j);
                v = diffRates.value(mtCodes.at(j)) * vs.value(key);
                if(dirs.value(key) == MDIR_D)
                    v.changeSign();
                ba->setValue(v);
                ba->setSummary(tr("结转汇兑损益"));
                sum += ba->getValue();
            }
        }
        ba = pz->appendBlank();
        ba->setFirstSubject(subMgr->getCwfySub());
        ba->setSecondSubject(hdsySub);
        ba->setMt(account->getMasterMt());
        ba->setDir(MDIR_J);
        ba->setValue(sum);
        ba->setSummary(tr("结转自%1的汇兑损益").arg(fsub->getName()));
        createdPzs<<pz;
    }

//    //创建结转银行存款汇兑损益的凭证
//    PingZheng* bankPz = new PingZheng(this);
//    foreach(SecondSubject* ssub, subMgr->getBankSub()->getChildSubs()){
//        if(ssub->getName().indexOf(account->getMasterMt()->name()) != -1)
//            continue;
//        for(int i = 0; i < mts.count(); ++i){
//            if(!dbUtil->readExtraForMS(y,m,mts.at(i)->code(),ssub->getId(),ev,wv,dir))
//                return false;
//            if(dir == MDIR_P)
//                continue;
//            BusiAction* ba = bankPz->appendBlank();
//            ba->setFirstSubject(subMgr->getBankSub());
//            ba->setSecondSubject(ssub);
//            ba->setMt(mts.at(i));
//            ba->setSummary(tr("结转汇兑损益"));
//            ba->setDir(MDIR_D);
//            ba->setValue(diffRates.value(mts.at(i).code) * ev);
//            sum += ba->getValue();
//        }
//    }
//    if(sum != 0.0){
//        BusiAction* ba = bankPz->appendBlank();
//        ba->setFirstSubject(subMgr->getCwfySub());
//        ba->setSecondSubject(hdsySub);
//        ba->setMt(account->getMasterMt());
//        ba->setDir(MDIR_J);
//        ba->setSummary(tr("结转自银行存款的汇兑损益"));
//        ba->setValue(sum);
//        createdPzs<<bankPz;
//    }

    //创建结转应收账款汇兑损益的凭证
    //PingZheng* ysPz = new PingZheng(this);

    //创建结转应付账款汇兑损益的凭证
    //PingZheng* yfPz = new PingZheng(this);
    return true;
}

/**
 * @brief PzSetMgr::getJzhdsyPz
 *  获取打开凭证集内的结转汇兑损益的凭证对象
 * @param pzLst
 */
void PzSetMgr::getJzhdsyPz(QList<PingZheng *> &pzLst)
{
    if(!isOpened())
        return;
    foreach(PingZheng* pz, *pzs){
        if(pz->getPzClass() == Pzc_Jzhd)
            pzLst<<pz;
    }
}

/**
 * @brief PzSetMgr::crtJzsyPz
 *  创建结转汇兑损益凭证
 * @param y
 * @param m
 * @param createdPzs
 * @return
 */
bool PzSetMgr::crtJzsyPz(int y, int m, QList<PingZheng *> &createdPzs)
{
    //读取余额
    QList<int> in_sids, fei_sids; //分别是收入类和费用类损益类二级科目的id集合
    SubjectManager* subMgr = account->getSubjectManager(account->getSuite(y)->subSys);
    //QList<FirstSubject*> in_subs,fei_subs; //收入类和费用类的损益类一级科目
    //in_subs = subMgr->getSyClsSubs();
    foreach(FirstSubject* fsub, subMgr->getSyClsSubs()){
        in_sids<<fsub->getChildSubIds();
    }
    //fei_subs = subMgr->getSyClsSubs(false);
    foreach(FirstSubject* fsub, subMgr->getSyClsSubs(false))
        fei_sids<<fsub->getChildSubIds();
    QHash<int,Double> vs;
    QHash<int,MoneyDirection> dirs;
    //QList<int> sids = in_sids + fei_sids;
    FirstSubject* bnlrFSub = subMgr->getBnlrSub();  //本年利润一级科目
    SecondSubject* jzSSub = NULL;                   //本年利润——结转子目
    foreach(SecondSubject* ssub, bnlrFSub->getChildSubs()){
        if(ssub->getName() == tr("结转")){
            jzSSub = ssub;
            break;
        }
    }
    if(!jzSSub){
        QMessageBox::warning(0,tr("警告信息"),tr("不能获取本年利润——结转科目，结转不能继续不能"));
        return false;
    }

    //创建结转收入类的结转凭证
    if(!dbUtil->readAllExtraForSSubMMt(y,m,account->getMasterMt()->code(),in_sids,vs,dirs))
        return false;
    QDate d(year(),month(),1);
    d.setDate(year(),month(),d.daysInMonth());
    QString ds = d.toString(Qt::ISODate);

    PingZheng* pz = new PingZheng(this);
    pz->setDate(ds);
    pz->setEncNumber(0);
    pz->setPzClass(Pzc_JzsyIn);
    pz->setPzState(Pzs_Recording);
    pz->setRecordUser(user);
    SecondSubject* ssub;
    BusiAction* ba;
    Double sum = 0.0;
    for(int i = 0; i < in_sids.count(); ++i){
        ssub = subMgr->getSndSubject(in_sids.at(i));
        if(!vs.contains(ssub->getId()))
            continue;
        ba = pz->appendBlank();
        ba->setSummary(tr("结转（%1-%2）至本年利润").arg(ssub->getParent()->getName()).arg(ssub->getName()));
        ba->setFirstSubject(ssub->getParent());
        ba->setSecondSubject(ssub);
        ba->setMt(account->getMasterMt());
        //结转收入类到本年利润，一般是损益类科目放在借方，本年利润放在贷方
        //而且，损益类中收入类科目余额约定是贷方，故不做方向检测
        ba->setDir(MDIR_J);
        ba->setValue(vs.value(ssub->getId()));
        sum += ba->getValue();
    }
    ba = pz->appendBlank();
    ba->setSummary(tr("结转收入至本年利润"));
    ba->setFirstSubject(bnlrFSub);
    ba->setSecondSubject(jzSSub);
    ba->setMt(account->getMasterMt());
    ba->setDir(MDIR_D);
    ba->setValue(sum);
    createdPzs<<pz;

    //创建结转费用类的结转凭证
    if(!dbUtil->readAllExtraForSSubMMt(y,m,account->getMasterMt()->code(),fei_sids,vs,dirs))
        return false;
    sum = 0.0;
    pz = new PingZheng(this);
    pz->setDate(ds);
    pz->setEncNumber(0);
    pz->setPzClass(Pzc_JzsyFei);
    pz->setPzState(Pzs_Recording);
    pz->setRecordUser(user);
    for(int i = 0; i < fei_sids.count(); ++i){
        ssub = subMgr->getSndSubject(fei_sids.at(i));
        if(!vs.contains(ssub->getId()))
            continue;
        ba = pz->appendBlank();
        ba->setSummary(tr("结转（%1-%2）至本年利润").arg(ssub->getParent()->getName()).arg(ssub->getName()));
        ba->setFirstSubject(ssub->getParent());
        ba->setSecondSubject(ssub);
        ba->setMt(account->getMasterMt());
        //结转费用类到本年利润，一般是损益类科目放在贷方，本年利润方在借方
        //而且，损益类中费用类科目余额是约定在借方，故不做方向检测
        ba->setDir(MDIR_D);
        ba->setValue(vs.value(ssub->getId()));
        sum += ba->getValue();
    }
    ba = pz->appendBlank();
    ba->setSummary(tr("结转费用至本年利润"));
    ba->setFirstSubject(bnlrFSub);
    ba->setSecondSubject(jzSSub);
    ba->setMt(account->getMasterMt());
    ba->setDir(MDIR_J);
    ba->setValue(sum);
    createdPzs<<pz;
    return true;
}

/**
 * @brief PzSetMgr::getJzsyPz
 *  获取打开凭证集内的结转损益凭证对象
 * @param pzLst
 */
void PzSetMgr::getJzsyPz(QList<PingZheng *> &pzLst)
{
    if(!isOpened())
        return;
    foreach(PingZheng* pz, *pzs){
        PzClass pzCls = pz->getPzClass();
        if(pzCls == Pzc_JzsyIn || pzCls == Pzc_JzsyFei)
            pzLst<<pz;
    }
}


//创建当期固定资产折旧凭证
bool PzSetMgr::crtGdzcPz()
{

}

//创建在指定年月中引入待摊费用的凭证
bool PzSetMgr::crtDtfyImpPz(int y,int m,QList<PzData*> pzds)
{
    //前置条件判定
    //...

//    if(!Dtfy::genImpPzData(y,m,pzds,user)){
//        if(!pzds.empty()){
//            for(int i = 0; i < pzds.count(); ++i)
//                delete pzds[i];
//            pzds.clear();
//        }
//        return false;
//    }

//    if(pzds.empty()){
//        QMessageBox::information(0,QObject::tr("提示信息"),QObject::tr("未发现要引入的待摊费用"));
//        return true;
//    }
//    for(int i = 0; i < pzds.count(); ++i){
//        PingZheng pz(pzds[i],user,db);
//        pz.save();
//        delete pzds[i];
//    }
//    pzds.clear();
    return true;
}

//创建当期计提待摊费用凭证
bool PzSetMgr::crtDtfyTxPz()
{
    return Dtfy::createTxPz(curY,curM,user);
}

//删除当期计提待摊费用凭证
bool PzSetMgr::delDtfyPz()
{
    return Dtfy::repealTxPz(curY,curM);
}

//创建当期结转利润凭证（将本年利润科目的余额结转至利润分配）
bool PzSetMgr::crtJzlyPz(int y, int m, PingZheng *pz)
{

}

//结账
void PzSetMgr::finishAccount()
{

}

//统计本期发生额
bool PzSetMgr::stat()
{

}

//统计指定年月凭证集的本期发生额
bool PzSetMgr::stat(int y, int m)
{

}

//获取当期明细账数据
bool PzSetMgr::getDetList()
{

}

//获取指定月份区间的明细账数据
bool PzSetMgr::getDetList(int y, int sm, int em)
{

}

//获取当期总账数据
bool PzSetMgr::getTotalList()
{
    return true;
}

//获取指定月份区间的总账数据
bool PzSetMgr::getTotalList(int y,int sm, int em)
{
    return true;
}

//查找凭证
bool PzSetMgr::find()
{

}

//在指定月份区间内查找凭证
bool PzSetMgr::find(int y, int sm, int em)
{

}

/**
 * @brief PzSetMgr::needRestat
 *  由于凭证集内的凭证数值的改变，需要进行重新统计来得到正确的余额
 */
void PzSetMgr::needRestat()
{
    setExtraState(false);
    emit pzExtraStateChanged(false);
}

/**
 * @brief PzSetMgr::pzChangedInSet
 *  打开的凭证集内的一个凭证的内容发生了改变，必须通知给凭证集对象
 * @param pz
 */
void PzSetMgr::pzChangedInSet(PingZheng *pz)
{
    //目前的实现只触发凭证集改变信号以启用主窗口的保存按钮
    emit pzSetChanged();
}

void PzSetMgr::pzStateChanged(PzState oldState, PzState newState)
{
//    switch(oldState){
//    case Pzs_Recording:
//        c_recording--;
//        break;
//    case Pzs_Verify:
//        c_verify--;
//        break;
//    case Pzs_Instat:
//        c_instat--;
//        break;
//    case Pzs_Repeal:
//        c_repeal--;
//        break;
//    }
//    switch(newState){
//    case Pzs_Recording:
//        c_recording++;
//        break;
//    case Pzs_Verify:
//        c_verify++;
//        break;
//    case Pzs_Instat:
//        c_instat++;
//        break;
//    case Pzs_Repeal:
//        c_repeal++;
//        break;
//    }
    _determinePzSetState(states[genKey(curY,curM)]);
    emit pzCountChanged(pzs->count());
}



