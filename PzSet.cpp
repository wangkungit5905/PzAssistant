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
    isReStat = false;
    isReSave = false;
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
    scanPzCount();
    extraStates[key] = dbUtil->getExtraState(y,m);
    pzs = &pzSetHash[key];
    for(int i = 0; i < pzs->count(); ++i){
        PingZheng* pz = pzs->at(i);
        connect(pz,SIGNAL(mustRestat()),this,SLOT(needRestat()));
        connect(pz,SIGNAL(pzContentChanged(PingZheng*)),this,SLOT(pzChangedInSet(PingZheng*)));
    }

    maxPzNum = pzSetHash.value(key).count() + 1;
    maxZbNum = 0;
    foreach(PingZheng* pz, pzSetHash.value(key)){
        if(maxZbNum < pz->number())
            maxZbNum = pz->number();
    }
    maxZbNum++;
    curY=y,curM=m;
    if(!statUtil)
        delete statUtil;
    statUtil = new StatUtil(pzSetHash.value(key),account);
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
    }
    qDeleteAll(pz_dels);
    pz_dels.clear();
    qDeleteAll(cachedPzs);
    cachedPzs.clear();
    curY=0;curM=0;
    c_recording=0;c_verify=0;c_instat=0;c_repeal=0;
    maxPzNum = 0;
    maxZbNum = 0;
    isReStat = false;
    isReSave = false;
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
QList<PingZheng *> PzSetMgr::getPzSpecRange(int y, int m, QSet<int> nums)
{
    QList<PingZheng*> pzs;
    int key = genKey(y,m);
    if(!pzSetHash.contains(key)){
        if(!dbUtil->loadPzSet(y,m,pzSetHash[key],this))
            return pzs;
        if(!dbUtil->getPzsState(y,m,states[key]))
            return pzs;
        extraStates[key] = dbUtil->getExtraState(y,m);
    }
    if(nums.isEmpty())
        return pzSetHash.value(key);
    foreach(PingZheng* pz, pzSetHash.value(key)){
        if(nums.contains(pz->number()))
            pzs<<pz;
    }
    return pzs;
}

/**
 * @brief PzSetMgr::contains
 *  在指定年月的凭证集内是否包含了指定id的凭证
 * @param y
 * @param m
 * @param pid
 * @return
 */
bool PzSetMgr::contains(int y, int m, int pid)
{
    return dbUtil->isContainPz(y,m,pid);
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
//保存当期余额
bool PzSetMgr::PzSetMgr::saveExtra()
{
    //保存当期余额之前，需要判断是否需要再次计算当期余额
    //（比如在凭证集打开后，进行了会影响当期余额的编辑操作）
    //return BusiUtil::savePeriodBeginValues(y,m,endExtra,endDir,
    //                                       endDetExtra,endDetDir,state,false);
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
QHash<int, Double> PzSetMgr::getRates()
{
    QHash<int,Double> rates;
    if(curY==0 && curM==0)
        return rates;
    account->getRates(curY,curM,rates);
    return rates;
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
        else
            curPz = pzs->at(--curIndex);
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
    pz->setNumber(pzs->count());
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
        return true;
    }
    //调整移除凭证后的凭证号
    for(i;i<pzs->count();++i)
        pzs->at(i)->setNumber(i+1);
    if(pz->number() == pzs->count()+1){
        curPz = pzs->last();
        curIndex = pzs->count()-1;
    }
    else{
        curPz = pzs->at(pz->number()-1);
        curIndex = pz->number() - 1;
    }
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
    PzsState state;
    _determinePzSetState(state);
    if(state != getState())
        setState(state);
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
    if(num < 1 || num > pzs->count())
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

//创建当期结转汇兑损益凭证
bool PzSetMgr::crtJzhdsyPz()
{

}

//创建当期结转损益凭证（将损益类科目余额结转至本年利润）
bool PzSetMgr::crtJzsyPz()
{

}

//创建当期结转利润凭证（将本年利润科目的余额结转至利润分配）
bool PzSetMgr::crtJzlyPz()
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



