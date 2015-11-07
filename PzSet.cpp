#include "global.h"
#include "tables.h"
#include "PzSet.h"
#include "pz.h"
#include "dbutil.h"
#include "statutil.h"
#include "commands.h"
#include "myhelper.h"
#include "utils.h"



/////////////////PzSetMgr///////////////////////////////////////////
AccountSuiteManager::AccountSuiteManager(AccountSuiteRecord* as, Account *account, /*User *user, */QObject *parent):QObject(parent),
    suiteRecord(as),account(account),curM(0),isYsYfLoaded(false),isICLoader(false)
{
    dbUtil = account->getDbUtil();
    undoStack = new QUndoStack(this);
    undoStack->setUndoLimit(MAXUNDOSTACK);
    statUtil = NULL;
    c_recording=0;c_verify=0;c_instat=0;c_repeal=0;
    maxPzNum = 0;
    maxZbNum = 0;
    curPz=NULL;
    curIndex = -1;
    curM=0;
    pzs=NULL;
    dirty = false;
}

AccountSuiteManager::~AccountSuiteManager()
{
    delete undoStack;
    if(statUtil)
        delete statUtil;
    qDeleteAll(incomes);
    qDeleteAll(costs);
    incomes.clear();costs.clear();
    qDeleteAll(ysInvoices);
    qDeleteAll(yfInvoices);
    ysInvoices.clear();yfInvoices.clear();
    foreach(QList<PingZheng*> pzs, pzSetHash){
        qDeleteAll(pzs);
        pzs.clear();
    }
    pzSetHash.clear();
    if(pzs)
        pzs=NULL;
    extraStates.clear();
    states.clear();
    qDeleteAll(historyPzs);historyPzs.clear();
}

/**
 * @brief 关闭帐套（关账）
 * @return
 */
bool AccountSuiteManager::closeSuite()
{
    //关账前置条件（帐套中最后一月的凭证集必须已结账）
    if(isPzSetOpened()){
        myHelper::ShowMessageBoxWarning(tr("凭证集未关闭，不能关账！"));
        return true;
    }
    if(getState(12) != Ps_Jzed){
        myHelper::ShowMessageBoxWarning(tr("帐套内最后月份的凭证集未结账，不能关账！"));
        return true;
    }
    suiteRecord->isClosed = true;
    return dbUtil->saveSuite(suiteRecord);
}

SubjectManager *AccountSuiteManager::getSubjectManager()
{
    return account->getSubjectManager(suiteRecord->subSys);
}

/**
 * @brief AccountSuiteManager::open
 *  以编辑模式打开凭证集
 * @param m 凭证集在账套中所处月份
 * @return
 */
bool AccountSuiteManager::open(int m)
{
    if(m>=suiteRecord->startMonth && m<=suiteRecord->endMonth && curM == m)
        return true;
    if(isPzSetOpened())    //同时只能打开一个凭证集用以编辑
        closePzSet();    
    if(!pzSetHash.contains(m)){
        if(!dbUtil->loadPzSet(suiteRecord->year,m,pzSetHash[m],this))
            return false;
        if(!dbUtil->getPzsState(suiteRecord->year,m,states[m]))
            return false;        
    }    
    curM=m;
    suiteRecord->recentMonth = m;
    pzs = &pzSetHash[m];
    scanPzCount(c_repeal,c_recording,c_verify,c_instat,pzs);
    if(!statUtil)
        statUtil = new StatUtil(pzs,this);
    else
        statUtil->setPzSet(pzs);
    //凭证集的状态以实际凭证为准
    PzsState state;
    _determinePzSetState(state);
    if(state != states.value(m)){
        states[m] = state;
        dirty = true;
    }
    extraStates[m] = dbUtil->getExtraState(suiteRecord->year,m);
    for(int i = 0; i < pzs->count(); ++i){
        PingZheng* pz = pzs->at(i);
        watchPz(pz);
    }

    maxPzNum = pzSetHash.value(m).count() + 1;
    maxZbNum = 0;
    foreach(PingZheng* pz, pzSetHash.value(m)){
        if(maxZbNum < pz->number())
            maxZbNum = pz->number();
    }
    maxZbNum++;
    statUtil->stat();
    if(!pzs->isEmpty()){
        PingZheng* oldPz = curPz;
        curPz = pzs->first();
        curIndex = 0;
        emit currentPzChanged(curPz,oldPz);
    }
    emit pzCountChanged(pzs->count());
    return true;
}

/**
 * @brief AccountSuiteManager::isSuiteEditable
 *  帐套是否可编辑
 * @return
 */
bool AccountSuiteManager::isSuiteEditable()
{
    if(account->isReadOnly() || suiteRecord->isClosed)
        return false;
    return true;
}

/**
 * @brief AccountSuiteManager::isEditable
 *  凭证集是否可编辑
 * @param m
 * @return
 */
bool AccountSuiteManager::isPzSetEditable(int m)
{
    if(account->isReadOnly() || isSuiteClosed())
        return false;
    if(m<0 || m>12)
        return false;
    if(curM == 0 || m!=0 && curM != m)
        return false;
    return !(getState() == Ps_Jzed);
}

/**
 * @brief PzSetMgr::isOpen
 *  凭证集是否已被打开
 * @return
 */
bool AccountSuiteManager::isPzSetOpened()
{
    return curM != 0;
}

/**
 * @brief PzSetMgr::isDirty
 *  凭证集是否有未保存的更改
 * @return
 */
bool AccountSuiteManager::isDirty()
{
    //要考虑的方面包括（凭证集内的凭证、余额状态、凭证集状态）
    return (dirty || !undoStack->isClean());
}


void AccountSuiteManager::closePzSet()
{
    if(isDirty()){
        if(QMessageBox::Yes == QMessageBox::warning(0,tr("提示信息"),
                                                    tr("凭证集已被编辑，需要保存吗？"),
                                                    QMessageBox::Yes|QMessageBox::No))
            save();
        else{
            //如果凭证发生了编辑，则恢复到初始状态
            rollback();
        }
    }

    statUtil->clear();
    undoStack->clear();
    for(int i = 0; i < pzs->count(); ++i){
        PingZheng* pz = pzs->at(i);
        watchPz(pz,false);
    }
    qDeleteAll(pz_dels);
    pz_dels.clear();
    qDeleteAll(cachedPzs);
    cachedPzs.clear();
    curM=0;
    c_recording=0;c_verify=0;c_instat=0;c_repeal=0;
    maxPzNum = 0;
    maxZbNum = 0;
    pzs=NULL;
    PingZheng* oldPz = curPz;
    curPz=NULL;
    curIndex = -1;
    _determineCurPzChanged(oldPz);
    emit pzCountChanged(0);
}

/**
 * @brief 回滚凭证集到初始状态
 */
void AccountSuiteManager::rollback()
{
    if(curM == 0)
        return;
    if(!undoStack->isClean()){
        int index = undoStack->cleanIndex();
        undoStack->setIndex(index);
    }
    undoStack->clear();
    //恢复余额状态和凭证集的状态
    dbUtil->getPzsState(suiteRecord->year,curM,states[curM]);
    extraStates[curM] = dbUtil->getExtraState(suiteRecord->year,curM);
}

/**
 * @brief 新建凭证集
 * @param month 新凭证集的月份
 * @return
 */
int AccountSuiteManager::newPzSet()
{
    if(suiteRecord->isClosed || suiteRecord->endMonth == 12)
        return 0;
    suiteRecord->endMonth++;
    account->saveSuite(suiteRecord);
    return suiteRecord->endMonth;
}

/**
 * @brief AccountSuiteManager::clearPzSetCaches
 */
void AccountSuiteManager::clearPzSetCaches()
{
    if(isPzSetOpened())
        closePzSet();
    QHashIterator<int,QList<PingZheng*> > it(pzSetHash);
    while(it.hasNext()){
        it.next();
        qDeleteAll(it.value());
        pzSetHash[it.key()].clear();
        pzSetHash.remove(it.key());
    }
}

//获取凭证总数（也即已用的最大凭证号）
int AccountSuiteManager::getPzCount()
{
    if(!isPzSetOpened())
        return 0;
    return maxPzNum-1;
}

/**
 * @brief PzSetMgr::getAllJzhdPzs
 *  返回所有结转汇兑损益的凭证对象列表
 * @return
 */
QList<PingZheng *> AccountSuiteManager::getAllJzhdPzs()
{
    QList<PingZheng*> pzLst;
    if(!isPzSetOpened())
        return pzLst;
    foreach(PingZheng* pz, *pzs){
        if(pz->getPzClass() == Pzc_Jzhd)
            pzLst<<pz;
    }
    return pzLst;
}

//重置凭证号
bool AccountSuiteManager::resetPzNum(int by)
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
PzsState AccountSuiteManager::getState(int m)
{
    int mm;
    if(m == 0){
        if(curM == 0)
            return Ps_NoOpen;
        mm = curM;
    }
    else
        mm = m;
    if(!states.contains(mm))
       dbUtil->getPzsState(suiteRecord->year,mm,states[mm]);
     return states.value(mm,Ps_NoOpen);
}

//设置凭证集状态
void AccountSuiteManager::setState(PzsState state,int m )
{
    int mm;
    if(m==0)
        mm=curM;
    else
        mm=m;
    if(!states.contains(mm))
        dbUtil->getPzsState(suiteRecord->year,mm,state);
    if(state == states.value(mm))
        return;
    states[mm] = state;
    dirty = true;
}

/**
 * @brief PzSetMgr::getExtraState
 *  获取余额状态（如果年月都为0，则获取当前打开凭证集的余额状态）
 * @param y
 * @param m
 * @return
 */
bool AccountSuiteManager::getExtraState(int m)
{
    int mm;
    if(m==0)
        mm=curM;
    else
        mm = m;
    if(!extraStates.contains(mm))
        extraStates[mm] = dbUtil->getExtraState(suiteRecord->year,mm);
    return extraStates.value(mm);
}

/**
 * @brief PzSetMgr::setExtraState
 *  设置余额状态（如果年月都为0，则设置当前打开凭证集的余额状态）
 * @param state
 * @param y
 * @param m
 */
void AccountSuiteManager::setExtraState(bool state, int m)
{
    int mm;
    if(m==0)
        mm=curM;
    else
        mm = m;
    if(!extraStates.contains(mm))
        extraStates[mm] = dbUtil->getExtraState(suiteRecord->year,mm);
    if(state == extraStates.value(mm))
        return;
    extraStates[mm] = state;
    dirty = true;
    emit pzExtraStateChanged(state);
}

/**
 * @brief PzSetMgr::getPzSet
 *  获取指定年月的凭证集
 * @param y
 * @param m
 */
bool AccountSuiteManager::getPzSet(int m, QList<PingZheng *> &pzs)
{
    if(!pzSetHash.contains(m)){
        if(!dbUtil->loadPzSet(suiteRecord->year,m,pzSetHash[m],this))
            return false;
        if(!dbUtil->getPzsState(suiteRecord->year,m,states[m]))
            return false;
        extraStates[m] = dbUtil->getExtraState(suiteRecord->year,m);
    }
    pzs = pzSetHash.value(m);
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
QList<PingZheng *> AccountSuiteManager::getPzSpecRange(QSet<int> nums)
{
    QList<PingZheng*> pzLst;
    if(!isPzSetOpened())
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
bool AccountSuiteManager::contains(int pid, int y, int m)
{
    //这个函数要重新实现，要先查找指定月份的凭证集是否已装载
    if(isPzSetOpened() && ((m == 0) || (m == curM))){
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
PingZheng *AccountSuiteManager::readPz(int pid, bool& in)
{
    if(pzs){
        foreach(PingZheng* pz, *pzs){
            if(pz->id() == pid){
                in = true;
                return pz;
            }
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
int AccountSuiteManager::getStatePzCount(PzState state)
{
    int c = 0;
    foreach(PingZheng* pz, *pzs)
        if(pz->getPzState() == state)
            c++;
    return c;
}

/**
 * @brief AccountSuiteManager::isAllInstat
 *  打开的凭证集内的所有凭证是否都已入账（除作废凭证外）
 * @return
 */
bool AccountSuiteManager::isAllInstat()
{
    if(!isPzSetOpened())
        return true;
    bool r = true;
    foreach(PingZheng* pz, *pzs){
        PzState state = pz->getPzState();
        if(state == Pzs_Instat || state == Pzs_Repeal)
            continue;
        else{
            r = false;
            break;
        }
    }
    return r;
}

/**
 * @brief 所有录入态凭证审核通过
 * @return  受影响的凭证数
 */
int AccountSuiteManager::verifyAll(User *user)
{
    if(!isPzSetOpened() || getState() == Ps_Jzed)
        return 0;
    int affected = 0;
    QUndoCommand* mainCmd = new QUndoCommand(tr("全部审核"));
    foreach(PingZheng* pz, *pzs){
        if(pz->getPzState() == Pzs_Recording){
            affected++;
            ModifyPzVStateCmd* cmd1 = new ModifyPzVStateCmd(this,pz,Pzs_Verify,mainCmd);
            ModifyPzVUserCmd* cmd2 = new ModifyPzVUserCmd(this,pz,user,mainCmd);
        }
    }
    undoStack->push(mainCmd);
    return affected;
}

/**
 * @brief 所有已审核凭证入账
 * @param user
 * @return  受影响的凭证数
 */
int AccountSuiteManager::instatAll(User *user)
{
    if(!isPzSetOpened() || getState() == Ps_Jzed)
        return 0;
    int affected = 0;
    QUndoCommand* mainCmd = new QUndoCommand(tr("全部入账"));
    foreach(PingZheng* pz, *pzs){
        if(pz->getPzState() == Pzs_Verify){
            affected++;
            ModifyPzVStateCmd* cmd1 = new ModifyPzVStateCmd(this,pz,Pzs_Instat,mainCmd);
            ModifyPzBUserCmd* cmd2 = new ModifyPzBUserCmd(this,pz,user,mainCmd);
        }
    }
    undoStack->push(mainCmd);
    return affected;
}

/**
 * @brief PzSetMgr::inspectPzError
 *  凭证集检错
 *  （1）凭证号连续性
 *  （2）自编号是否为空
 *  （3）自编号是否重复
 *  （4）借贷是否平衡
 *  （5）摘要栏是否为空
 *  （6）科目是否未设值（是否为“幽灵”科目）
 *  （7）金额是否未设值
 *  （8）借贷方向是否违反约定性
 *  （9）币种设置是否与科目的货币使用属性相符
 * @return
 */
bool AccountSuiteManager::inspectPzError(QList<PingZhengError*>& errors)
{
    if(!isPzSetOpened())
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
        if(pz->baCount() == 0){
            e = new PingZhengError;
            e->errorLevel = PZE_WARNING;
            e->errorType = 0;
            e->pz = pz;
            e->ba = NULL;
            errors<<e;
            pzNum = pz->number();
            continue;
        }
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
            if(ba->getFirstSubject() || (!ba->getFirstSubject() && ba->getSecondSubject())){
                SecondSubject* ssub = ba->getSecondSubject();
                if(ssub  && ba->getFirstSubject() != ssub->getParent()){
                    e = new PingZhengError;
                    e->errorLevel = PZE_ERROR;
                    e->errorType = 2;
                    e->pz = pz;
                    e->ba = ba;
                    e->explain = tr("%1号凭证第%2条会计分录发现“幽灵”子目！").arg(pz->number()).arg(ba->getNumber());
                    errors<<e;
                }
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
//bool AccountSuiteManager::AccountSuiteManager::saveExtra()
//{
//    if(!isPzSetOpened())
//        return false;
//    if(getExtraState())
//        return true;
//    if(!statUtil->save())
//        return false;
//    //setExtraState(true);
//    //if(!dbUtil->setExtraState(curY,curM,true))
//    //    return false;
//    return true;
//}

//读取当期余额（读取的是最近一次保存到余额表中的数据）
//bool AccountSuiteManager::readExtra()
//{
//    endExtra.clear();
//    endDetExtra.clear();
//    endDir.clear();
//    endDetDir.clear();

//    //if(!BusiUtil::readExtraByMonth(y,m,endExtra,endDir,endDetExtra,endDetDir))
//    //    return false;

//}

//读取期初（前期）余额
//bool AccountSuiteManager::readPreExtra()
//{
//    preExtra.clear();
//    preDetExtra.clear();
//    int yy,mm;
//    if(curM == 1){
//        yy = suiteRecord->year - 1;
//        mm = 12;
//    }
//    else{
//        yy = suiteRecord->year;
//        mm = curM - 1;
//    }
//    return dbUtil->readExtraForPm(yy,mm,preExtra,preDir,preDetExtra,preDetDir);
//}

/**
 * @brief PzSetMgr::getRates
 *  获取当前打开凭证集所使用的汇率表
 * @return
 */
bool AccountSuiteManager::getRates(QHash<int, Double>& rates, int m)
{
    //if(curY==0 && curM==0)
    //    return false;
    if(m == 0)
        m = curM;
    if(!account->getRates(suiteRecord->year,m,rates)){
        QMessageBox::critical(0,tr("错误信息"),tr("在读取%1年%2月的汇率时出错！").arg(suiteRecord->year).arg(m));
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
bool AccountSuiteManager::setRates(QHash<int, Double> rates, int m)
{
    //if(curY == 0 && curM == 0)
    //    return false;
    if(m == 0)
        m = curM;
    if(!account->setRates(suiteRecord->year,m,rates)){
        QMessageBox::critical(0,tr("错误信息"),tr("在保存%1年%月的汇率时出错！").arg(suiteRecord->year).arg(m));
        return false;
    }
    return true;
}

/**
 * @brief PzSetMgr::first 定位到第一个凭证对象
 * @return
 */
PingZheng *AccountSuiteManager::first()
{
    if(!isPzSetOpened()){
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
PingZheng *AccountSuiteManager::next()
{
    if(!isPzSetOpened()){
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
PingZheng *AccountSuiteManager::previou()
{
    if(!isPzSetOpened()){
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
PingZheng *AccountSuiteManager::last()
{
    if(!isPzSetOpened()){
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
PingZheng *AccountSuiteManager::seek(int num)
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
bool AccountSuiteManager::seek(PingZheng *pz)
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

/**
 * @brief 返回指定月份凭证集内各类凭证的数目
 * @param m             凭证集所处月份
 * @param repealNum     作废凭证数
 * @param recordingNum  录入凭证数
 * @param verifyNum     审核凭证数
 * @param instatNum     入账凭证数
 */
void AccountSuiteManager::getPzCountForMonth(int m, int &repealNum, int &recordingNum, int &verifyNum, int &instatNum)
{
    if(!pzSetHash.contains(m)){
        repealNum=0;
        recordingNum=0;
        verifyNum=0;
        instatNum=0;
        return;
    }
    scanPzCount(repealNum,recordingNum,verifyNum,instatNum,&pzSetHash[m]);
}

//添加空白凭证
PingZheng* AccountSuiteManager::appendPz(PzClass pzCls)
{
    if(!isPzSetOpened())
        return NULL;
    PingZheng* oldPz = curPz;
    QString ds = QDate(suiteRecord->year,curM,1).toString(Qt::ISODate);
    curPz =  new PingZheng(this,0,ds,maxPzNum++,maxZbNum++,0.0,0.0,pzCls,0,Pzs_Recording);
    c_recording++;
    setState(Ps_Rec);
    *pzs<<curPz;
    watchPz(curPz);
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
bool AccountSuiteManager::append(PingZheng *pz)
{
    if(!pz)
        return false;
    if(!isPzSetOpened()){
        LOG_ERROR(QObject::tr("pzSet not opened,don't' append pingzheng"));
        return false;
    }
    if(restorePz(pz))
        return true;
    *pzs<<pz;
    watchPz(pz);
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
    _determinePzSetState(states[curM]);
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
bool AccountSuiteManager::insert(PingZheng* pz)
{
    //插入凭证要保证凭证集内凭证号和自编号的连贯性要求,参数ecode错误代码（1：凭证号越界，2：自编号冲突）
    //凭证号必须从1开始，顺序增加，中间不能间断，自编号必须保证唯一性

    if(!pz)
        return false;
    if(!isPzSetOpened()){
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
        watchPz(pz);
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
    _determinePzSetState(states[curM]);
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
bool AccountSuiteManager::insert(int index, PingZheng *pz)
{
    if(!isPzSetOpened()){
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
    watchPz(pz);
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
    _determinePzSetState(states[curM]);
    emit pzCountChanged(pzs->count());
    emit pzSetChanged();
    return true;
}

/**
 * @brief AccountSuiteManager::watchPz
 *  监视凭证内容的改变，以便向外界通知必要的信息
 * @param pz
 * @param en
 */
void AccountSuiteManager::watchPz(PingZheng *pz, bool en)
{
    bool isRuntimeUpdateExtra;
    AppConfig::getInstance()->getCfgVar(AppConfig::CVC_RuntimeUpdateExtra,isRuntimeUpdateExtra);
    if(en){
        connect(pz,SIGNAL(mustRestat()),this,SLOT(needRestat()));
        connect(pz,SIGNAL(pzContentChanged(PingZheng*)),this,SLOT(pzChangedInSet(PingZheng*)));
        connect(pz,SIGNAL(pzStateChanged(PzState,PzState)),this,SLOT(pzStateChanged(PzState,PzState)));

        if(isRuntimeUpdateExtra){
            connect(pz,SIGNAL(addOrDelBa(BusiAction*,bool)),statUtil,SLOT(addOrDelBa(BusiAction*,bool)));
            //connect(pz,SIGNAL(dirChangedOnBa(BusiAction*,MoneyDirection)),statUtil,SLOT(dirChangedOnBa(BusiAction*,MoneyDirection)));
            //connect(pz,SIGNAL(mtChangedOnBa(BusiAction*,Money*)),statUtil,SLOT(mtChangedOnBa(BusiAction*,Money*)));
            connect(pz,SIGNAL(valueChangedOnBa(BusiAction*,Money*,Double&,MoneyDirection))
                    ,statUtil,SLOT(valueChangedOnBa(BusiAction*,Money*,Double&,MoneyDirection)));
            connect(pz,SIGNAL(subChangedOnBa(BusiAction*,FirstSubject*,SecondSubject*,Money*,Double)),
                    statUtil,SLOT(subChangedOnBa(BusiAction*,FirstSubject*,SecondSubject*,Money*,Double)));
        }
    }
    else{
        disconnect(pz,SIGNAL(mustRestat()),this,SLOT(needRestat()));
        disconnect(pz,SIGNAL(pzContentChanged(PingZheng*)),this,SLOT(pzChangedInSet(PingZheng*)));
        disconnect(pz,SIGNAL(pzStateChanged(PzState,PzState)),this,SLOT(pzStateChanged(PzState,PzState)));

        if(isRuntimeUpdateExtra){
            disconnect(pz,SIGNAL(addOrDelBa(BusiAction*,bool)),statUtil,SLOT(addOrDelBa(BusiAction*,bool)));
            //disconnect(pz,SIGNAL(dirChangedOnBa(BusiAction*,MoneyDirection)),statUtil,SLOT(dirChangedOnBa(BusiAction*,MoneyDirection)));
            //disconnect(pz,SIGNAL(mtChangedOnBa(BusiAction*,Money*)),statUtil,SLOT(mtChangedOnBa(BusiAction*,Money*)));
            disconnect(pz,SIGNAL(valueChangedOnBa(BusiAction*,Money*,Double&,MoneyDirection))
                       ,statUtil,SLOT(valueChangedOnBa(BusiAction*,Money*,Double&,MoneyDirection)));
            disconnect(pz,SIGNAL(subChangedOnBa(BusiAction*,FirstSubject*,SecondSubject*,Money*,Double)),
                    statUtil,SLOT(subChangedOnBa(BusiAction*,FirstSubject*,SecondSubject*,Money*,Double)));
        }
    }

}

/**
 * @brief PzSetMgr::cachePz
 *  缓存指定凭证对象，以便以后在撤销删除该凭证又执行了保存操作时恢复
 * @param pz
 */
void AccountSuiteManager::cachePz(PingZheng *pz)
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
bool AccountSuiteManager::isZbNumConflict(int num)
{
    if(!isPzSetOpened())
        return false;
    foreach(PingZheng* pz, *pzs)
        if(num == pz->zbNumber())
            return true;
    return false;
}

/**
 * @brief PzSetMgr::scanPzCount
 *  扫描指定凭证集内各种状态的凭证数
 */
void AccountSuiteManager::scanPzCount(int& repealNum, int& recordingNum, int& verifyNum, int& instatNum, QList<PingZheng*>* pzLst)
{
    recordingNum=0;verifyNum=0;instatNum=0;repealNum=0;
    foreach(PingZheng* pz, *pzLst){
        switch(pz->getPzState()){
        case Pzs_Recording:
            recordingNum++;
            break;
        case Pzs_Verify:
            verifyNum++;
            break;
        case Pzs_Instat:
            instatNum++;
            break;
        case Pzs_Repeal:
            repealNum++;
            break;
        }
    }
}

/**
 * @brief PzSetMgr::_confirmPzSetState
 *  根据当前打开凭证集内凭证的状态确定凭证集的状态
 * @param state
 */
void AccountSuiteManager::_determinePzSetState(PzsState &state)
{
    PzsState oldState = state;
    if(!isPzSetOpened())
        state = Ps_NoOpen;
    else if(getState() == Ps_Jzed)
        state = Ps_Jzed;
    else{
        scanPzCount(c_repeal,c_recording,c_verify,c_instat,pzs);
        if(c_recording > 0 || (c_recording==0 && c_verify==0 && c_instat==0))
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
void AccountSuiteManager::_determineCurPzChanged(PingZheng *oldPz)
{
    if(oldPz)
        oldPz->setCurBa(NULL);
    if((!curPz && oldPz) || (curPz && !oldPz) || (curPz && oldPz && *curPz != *oldPz))
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
bool AccountSuiteManager::_inspectDirEngageError(FirstSubject *fsub, MoneyDirection dir, PzClass pzc, QString &eStr)
{
    if(!fsub)
        return true;
    SubjectManager* sm = account->getSubjectManager(account->getCurSuiteRecord()->subSys);
    if(pzc == Pzc_Hand){
        if(fsub->getSubClass() == SC_SY){
            if(fsub->getJdDir() && (dir == MDIR_D)){
                eStr = tr("手工凭证中，费用类科目必须在借方");
                return false;
            }
            else if(!fsub->getJdDir() && (dir == MDIR_J)){
                eStr = tr("手工凭证中，收入类科目必须在贷方");
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
//int AccountSuiteManager::genKey(int y, int m)
//{
//    if(m < 1 || m > 12)
//        return 0;
//    return y * 100 + m;
//}

/**
 * @brief PzSetMgr::pzNotOpenWarning
 */
void AccountSuiteManager::pzsNotOpenWarning()
{
    QMessageBox::warning(0,tr("警告信息"),tr("凭证集未打开！"));
}



//移除凭证
bool AccountSuiteManager::remove(PingZheng *pz)
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
            watchPz(pz,false);
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
    }
    else{
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
    _determinePzSetState(states[curM]);
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
bool AccountSuiteManager::restorePz(PingZheng *pz)
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
PingZheng *AccountSuiteManager::getPz(int num)
{
    if(!isPzSetOpened() || num < 1 || num > pzs->count())
        return NULL;
    return pzs->at(num-1);
}

/**
 * @brief PzSetMgr::setCurPz
 * 设置当前凭证
 * @param pz
 */
void AccountSuiteManager::setCurPz(PingZheng *pz)
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
bool AccountSuiteManager::savePz(PingZheng *pz)
{
    return dbUtil->savePingZheng(pz);
}

/**
 * @brief PzSetMgr::savePz
 *  保存打开凭证集内的所有被修改和删除的凭证
 *  对于被删除的凭证，其对象仍将保存在缓存队列（pz_saveAfterDels）中，且对象ID复位
 *  当这些被删除且已实际删除的凭证恢复时，将以新凭证的面目出现
 * @param pzs
 * @return
 */
bool AccountSuiteManager::_savePzSet()
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
bool AccountSuiteManager::save(SaveWitch witch)
{
    switch(witch){
    case SW_ALL:
        if(!_savePzSet())
            return false;
        if(!dbUtil->setPzsState(suiteRecord->year,curM,states.value(curM)))
            return false;
        if(!dbUtil->setExtraState(suiteRecord->year,curM,extraStates.value(curM)))
            return false;
        undoStack->setClean();
        dirty = false;
    case SW_PZS:
        if(!_savePzSet())
            return false;
        undoStack->setClean();
        break;
    case SW_STATE:
        if(!dbUtil->setPzsState(suiteRecord->year,curM,getState()))
            return false;
        if(!dbUtil->setExtraState(suiteRecord->year,curM,getExtraState()))
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
bool AccountSuiteManager::crtJzhdsyPz(int y, int m, QList<PingZheng *> &createdPzs, QHash<int, Double> sRate, QHash<int, Double> eRate, User *user)
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
    SubjectManager* subMgr=account->getSubjectManager(account->getSuiteRecord(y)->subSys);
    FirstSubject* cwfySub=subMgr->getCwfySub();
    SecondSubject* hdsySub=subMgr->getHdsySSub();
    if(!hdsySub){
        QMessageBox::critical(0,tr("错误信息"),tr("不能获取到财务费用下的汇兑损益科目！"));
        return false;
    }
    PingZheng* pz=0;
    BusiAction* ba=0;
    QHash<int,Double> vs,wvs;
    QHash<int,MoneyDirection> dirs;
    QList<FirstSubject*> fsubs;
    Double sum,v;
    subMgr->getUseWbSubs(fsubs);
    int num=0;
    QDate d(year(),month(),1);
    d.setDate(year(),month(),d.daysInMonth());
    QString ds = d.toString(Qt::ISODate);
    QList<int> mtCodes;
    foreach(Money* mt, account->getAllMoneys()){
        if(mt != account->getMasterMt())
            mtCodes<<mt->code();
    }
    if(diffRates.count() == 0){   //如果所有的外币汇率都没有变化，则无须进行汇兑损益的结转
        if(!AppConfig::getInstance()->remainForeignCurrencyUnity())
            return true;
        //调整外币余额原币与本币币值的一致性
        vs = statUtil->getEndValueSPm();
        wvs = statUtil->getEndValueSMm();
        dirs = statUtil->getEndDirS();
        foreach(FirstSubject* fsub, fsubs){
            foreach(SecondSubject* ssub, fsub->getChildSubs()){
                foreach(int mt,mtCodes){
                    int key = ssub->getId() * 10 + mt;
                    Double pv = vs.value(key);
                    Double mv = wvs.value(key);
                    Double dv = mv - pv * eRate.value(mt);
                    if(dv != 0){
                        if(!pz){
                            pz = new PingZheng(this);
                            pz->setDate(ds);
                            pz->setEncNumber(0);
                            pz->setPzClass(Pzc_Jzhd);
                            pz->setRecordUser(user);
                            pz->setPzState(Pzs_Recording);
                        }
                        if(dirs.value(key) == MDIR_D)
                            dv.changeSign();
                        ba = pz->appendBlank();
                        ba->setSummary(tr("调整外币余额的本币值"));
                        ba->setFirstSubject(fsub);
                        ba->setSecondSubject(ssub);
                        ba->setDir(MDIR_D);
                        ba->setMt(account->getMasterMt(),dv);
                        sum += dv;
                    }
                }
            }
            if(pz){
                ba = pz->appendBlank();
                ba->setSummary(tr("调整外币余额的本币值"));
                ba->setFirstSubject(cwfySub);
                ba->setSecondSubject(hdsySub);
                ba->setMt(account->getMasterMt(),sum);
                ba->setDir(MDIR_J);
                createdPzs<<pz;
            }
            pz=0;
            sum=0.0;
        }
        return true;
    }
    foreach(FirstSubject* fsub, fsubs){
        vs.clear();wvs.clear();dirs.clear();
        if(!dbUtil->readAllWbExtraForFSub(y,m,fsub->getAllSSubIds(),mtCodes,vs,wvs,dirs))
            return false;
        if(vs.isEmpty() && wvs.isEmpty())
            continue;
        num++;
        pz = new PingZheng(this);
        pz->setDate(ds);
        pz->setEncNumber(0);
        pz->setPzClass(Pzc_Jzhd);
        pz->setRecordUser(user);
        pz->setPzState(Pzs_Recording);
        QList<SecondSubject*> ssubObjs;
        foreach(int key, wvs.keys()){
            int sid = key/10;
            SecondSubject* ssubObj = subMgr->getSndSubject(sid);
            if(!ssubObjs.contains(ssubObj))
                ssubObjs<<ssubObj;
        }
        qSort(ssubObjs.begin(),ssubObjs.end(),bySubNameThan_ss);
        sum = 0.0;
        int key;
        for(int i = 0; i < ssubObjs.count(); ++i){
            for(int j = 0; j < mtCodes.count(); ++j){
                SecondSubject* ssub = ssubObjs.at(i);
                int mCode = mtCodes.at(j);
                ba = pz->appendBlank();
                ba->setFirstSubject(fsub);
                ba->setSecondSubject(ssub);
                ba->setDir(MDIR_D);
                key = ssub->getId() * 10 + mCode;
                v = diffRates.value(mCode) * vs.value(key);
                //调整本币形式的值与原币的折算值一致
                if(AppConfig::getInstance()->remainForeignCurrencyUnity()){
                    Double dv = wvs.value(key) - v - vs.value(key) * eRate.value(mCode);
                    if(dv != 0)
                        v += dv;
                }
                if(dirs.value(key) == MDIR_D)
                    v.changeSign();
                ba->setMt(account->getMasterMt(),v);
                ba->setSummary(tr("结转汇兑损益"));
                sum += ba->getValue();
            }
        }
        ba = pz->appendBlank();
        ba->setFirstSubject(subMgr->getCwfySub());
        ba->setSecondSubject(hdsySub);
        ba->setMt(account->getMasterMt(),sum);
        ba->setDir(MDIR_J);
        ba->setSummary(tr("结转自%1的汇兑损益").arg(fsub->getName()));
        createdPzs<<pz;
    }
    return true;
}

/**
 * @brief PzSetMgr::getJzhdsyPz
 *  获取打开凭证集内的结转汇兑损益的凭证对象
 * @param pzLst
 */
void AccountSuiteManager::getJzhdsyPz(QList<PingZheng *> &pzLst)
{
    if(!isPzSetOpened())
        return;
    foreach(PingZheng* pz, *pzs){
        if(pz->getPzClass() == Pzc_Jzhd)
            pzLst<<pz;
    }
}

/**
 * @brief 获取当前打开凭证集中，必须进行结转汇兑损益的凭证数
 *  要进行汇兑损益的结转，必须满足2个条件
 *  1、期初汇率和期末汇率不等；
 *  2、使用外币的科目的外币原币余额不为0；
 *  其次，如果汇率未变，且原币余额折算为本币后与本币余额不一致，
 *  为调整也需要增加一个结转汇兑损益的凭证来存放这些调整分录
 * @return
 */
int AccountSuiteManager::getJzhdsyMustPzNums()
{
    if(!isPzSetOpened())
        return 0;
    int yy,mm;
    if(curM == 12){
        yy = suiteRecord->year + 1;
        mm = 1;
    }
    else{
        yy = suiteRecord->year;
        mm = curM + 1;
    }
    SubjectManager* sm = getSubjectManager();
    QList<FirstSubject*> fsubs;
    sm->getUseWbSubs(fsubs);
    QHash<int,Double> sRates,eRates;
    if(!account->getRates(suiteRecord->year, curM, sRates) ||
       !account->getRates(yy, mm, eRates)){
        return fsubs.count() + 1; //返回这个数将会提示用户结转损益操作无法执行。
    }
    QList<Money*> wbMts = account->getWaiMt();
    if(wbMts.isEmpty())
        return 0;
    bool rateChanged = false;
    foreach(Money* mt, wbMts){
        if(sRates.value(mt->code()) != eRates.value(mt->code())){
            rateChanged = true;
            break;
        }
    }
    int pzNums = 0;
    if(!rateChanged){
        if(!AppConfig::getInstance()->remainForeignCurrencyUnity())
            return 0;
        QHash<int,Double> vs,wvs;
        vs = statUtil->getEndValueSPm();
        wvs = statUtil->getEndValueSMm();
        foreach(Money* mt, wbMts){
            foreach(FirstSubject* fsub, fsubs){
                foreach(SecondSubject* ssub, fsub->getChildSubs()){
                    int key = ssub->getId()*10+mt->code();
                    Double pv = vs.value(key,0.0);
                    Double mv = wvs.value(key,0.0);
                    if(pv == 0 && mv == 0)
                        continue;
                    if(pv*eRates.value(mt->code()) != mv){
                        pzNums++;
                        break;
                    }
                }
            }
        }
        return pzNums;
    }

    Double v,wv; MoneyDirection dir=MDIR_P;
    //只要某个科目的某个外币汇率发生了改变且其余额不为0，则必须进行汇兑损益的结转
    foreach(FirstSubject* fsub, fsubs){
        foreach(Money* mt, wbMts){
            account->getDbUtil()->readExtraForMF(suiteRecord->year,curM,mt->code(),fsub->getId(),v,wv,dir);
            if(dir != MDIR_P){
                if(sRates.value(mt->code()) != eRates.value(mt->code())){
                    pzNums++;
                    break;
                }
            }
        }
    }
    return pzNums;
}

/**
 * @brief PzSetMgr::crtJzsyPz
 *  创建结转汇兑损益凭证
 * @param y
 * @param m
 * @param createdPzs
 * @return
 */
bool AccountSuiteManager::crtJzsyPz(int y, int m, QList<PingZheng *> &createdPzs)
{
    //读取余额
    QList<int> in_sids, fei_sids; //分别是收入类和费用类损益类二级科目的id集合
    QList<SecondSubject*> in_ssubs,fei_ssubs;
    SubjectManager* subMgr = account->getSubjectManager(suiteRecord->subSys);
    foreach(FirstSubject* fsub, subMgr->getSyClsSubs()){
        in_ssubs<<fsub->getChildSubs(SORTMODE_NAME);
    }
    foreach(FirstSubject* fsub, subMgr->getSyClsSubs(false))
        fei_ssubs<<fsub->getChildSubs(SORTMODE_NAME);
    QHash<int,Double> vs;
    QHash<int,MoneyDirection> dirs;
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
    foreach(SecondSubject* ssub, in_ssubs)
        in_sids<<ssub->getId();
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
    pz->setRecordUser(curUser);
    SecondSubject* ssub;
    BusiAction* ba;
    Double sum = 0.0;
    for(int i = 0; i < in_ssubs.count(); ++i){
        ssub = in_ssubs.at(i);
        if(!vs.contains(ssub->getId()))
            continue;
        ba = pz->appendBlank();
        ba->setSummary(tr("结转（%1-%2）至本年利润").arg(ssub->getParent()->getName()).arg(ssub->getName()));
        ba->setFirstSubject(ssub->getParent());
        ba->setSecondSubject(ssub);
        ba->setMt(account->getMasterMt(),vs.value(ssub->getId()));
        //结转收入类到本年利润，一般是损益类科目放在借方，本年利润放在贷方
        //而且，损益类中收入类科目余额约定是贷方，故不做方向检测
        ba->setDir(MDIR_J);
        sum += ba->getValue();
    }
    ba = pz->appendBlank();
    ba->setSummary(tr("结转收入至本年利润"));
    ba->setFirstSubject(bnlrFSub);
    ba->setSecondSubject(jzSSub);
    ba->setMt(account->getMasterMt(),sum);
    ba->setDir(MDIR_D);
    createdPzs<<pz;

    //创建结转费用类的结转凭证
    foreach(SecondSubject* ssub,fei_ssubs)
        fei_sids<<ssub->getId();
    if(!dbUtil->readAllExtraForSSubMMt(y,m,account->getMasterMt()->code(),fei_sids,vs,dirs))
        return false;
    sum = 0.0;
    pz = new PingZheng(this);
    pz->setDate(ds);
    pz->setEncNumber(0);
    pz->setPzClass(Pzc_JzsyFei);
    pz->setPzState(Pzs_Recording);
    pz->setRecordUser(curUser);
    for(int i = 0; i < fei_ssubs.count(); ++i){
        ssub = fei_ssubs.at(i);
        if(!vs.contains(ssub->getId()))
            continue;
        ba = pz->appendBlank();
        ba->setSummary(tr("结转（%1-%2）至本年利润").arg(ssub->getParent()->getName()).arg(ssub->getName()));
        ba->setFirstSubject(ssub->getParent());
        ba->setSecondSubject(ssub);
        ba->setMt(account->getMasterMt(),vs.value(ssub->getId()));
        //结转费用类到本年利润，一般是损益类科目放在贷方，本年利润方在借方
        //而且，损益类中费用类科目余额是约定在借方，故不做方向检测
        ba->setDir(MDIR_D);
        sum += ba->getValue();
    }
    ba = pz->appendBlank();
    ba->setSummary(tr("结转费用至本年利润"));
    ba->setFirstSubject(bnlrFSub);
    ba->setSecondSubject(jzSSub);
    ba->setMt(account->getMasterMt(),sum);
    ba->setDir(MDIR_J);
    createdPzs<<pz;
    return true;
}

/**
 * @brief PzSetMgr::getJzsyPz
 *  获取打开凭证集内的结转损益凭证对象
 * @param pzLst
 */
void AccountSuiteManager::getJzsyPz(QList<PingZheng *> &pzLst)
{
    if(!isPzSetOpened())
        return;
    foreach(PingZheng* pz, *pzs){
        PzClass pzCls = pz->getPzClass();
        if(pzCls == Pzc_JzsyIn || pzCls == Pzc_JzsyFei)
            pzLst<<pz;
    }
}


//创建当期固定资产折旧凭证
bool AccountSuiteManager::crtGdzcPz()
{

}

//创建在指定年月中引入待摊费用的凭证
bool AccountSuiteManager::crtDtfyImpPz(int y,int m,QList<PzData*> pzds)
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
bool AccountSuiteManager::crtDtfyTxPz()
{
    return Dtfy::createTxPz(suiteRecord->year,curM,curUser);
}

//删除当期计提待摊费用凭证
bool AccountSuiteManager::delDtfyPz()
{
    return Dtfy::repealTxPz(suiteRecord->year,curM);
}

//创建当期结转利润凭证（将本年利润科目的余额结转至利润分配）
bool AccountSuiteManager::crtJzlyPz(int y, int m, PingZheng *pz)
{

}

//结账
void AccountSuiteManager::finishAccount()
{

}

//统计本期发生额
bool AccountSuiteManager::stat()
{

}

//统计指定年月凭证集的本期发生额
bool AccountSuiteManager::stat(int y, int m)
{

}

//获取当期明细账数据
bool AccountSuiteManager::getDetList()
{

}

//获取指定月份区间的明细账数据
bool AccountSuiteManager::getDetList(int y, int sm, int em)
{

}

//获取当期总账数据
bool AccountSuiteManager::getTotalList()
{
    return true;
}

//获取指定月份区间的总账数据
bool AccountSuiteManager::getTotalList(int y,int sm, int em)
{
    return true;
}

//查找凭证
bool AccountSuiteManager::find()
{

}

//在指定月份区间内查找凭证
bool AccountSuiteManager::find(int y, int sm, int em)
{

}

/**
 * @brief 返回指定月份的凭证列表（用于历史凭证浏览）
 * @param m 月份
 * @return
 */
QList<PingZheng *> AccountSuiteManager::getHistoryPzSet(int m)
{
    if(!pzSetHash.contains(m)){
        if(!dbUtil->loadPzSet(suiteRecord->year,m,pzSetHash[m],this)){
            QMessageBox::critical(0,tr("出错信息"),tr("在装载%1年%2月的凭证集时出错！").arg(suiteRecord->year).arg(m));
            //return NULL;
        }
        if(!dbUtil->getPzsState(suiteRecord->year,m,states[m])){
            QMessageBox::critical(0,tr("出错信息"),tr("在读取%1年%2月的凭证集状态时出错！").arg(suiteRecord->year).arg(m));
            //return NULL;
        }
    }
    return pzSetHash.value(m);
}

/**
 * @brief 统计指定月份的应收应付发票增减情况
 * @param month
 * @param incomes
 * @param costs
 * @param errors
 * @param scanXf    //是否扫描销方（即应收应付的消减项-贷方的应收、借方的应付）
 * @param reserved  //是否保留已销账的记录
 */
void AccountSuiteManager::scanYsYfForMonth(int month, QList<InvoiceRecord *> &incomes, QList<InvoiceRecord *> &costs, QStringList &errors, bool scanXf, bool reserved)
{
    if(!open(month)){
        errors<<tr("打开%1月凭证集时发生错误！").arg(month);
        return;
    }
    SubjectManager* sm = account->getSubjectManager(suiteRecord->subSys);
    FirstSubject* ysFSub = sm->getYsSub();
    FirstSubject* yfFSub = sm->getYfSub();
    SecondSubject* xxSSub = sm->getXxseSSub();
    SecondSubject* jxSSub = sm->getJxseSSub();
    InvoiceRecord* r;
    QList<InvoiceRecord *> dels;    //不必在保存的记录
    for(int i = 0; i < pzs->count(); ++i){
        PingZheng* pz = pzs->at(i);
        for(int j = 0; j < pz->baCount(); ++j){
            BusiAction* ba = pz->getBusiAction(j);
            //如果是应收的借方，则提取客户、发票号和金额
            QString inum; Double wbMoney; SecondSubject* ssub;
            if(ba->getFirstSubject() == ysFSub && ba->getDir() == MDIR_J &&
                    PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wbMoney)){
                r = new InvoiceRecord;
                r->invoiceNumber = inum;
                r->customer = ba->getSecondSubject()->getNameItem();
                r->baRID = ba->getId();
                r->money = ba->getValue();
                r->wmoney = wbMoney;
                r->pzNumber = pz->number();
                r->wmt = ba->getMt();
                r->year = year();
                r->month = month;
                incomes<<r;
                //如果下一条分录是该发票对应的销项税，则在已添加的列表中查找对应发票号
                BusiAction* ba1 = pz->getBusiAction(j+1);
                if(!ba1)
                    continue;
                if(ba1->getSecondSubject() == xxSSub && ba1->getDir() == MDIR_D &&
                        PaUtils::extractOnlyInvoiceNum(ba1->getSummary(),inum,wbMoney)){
                    if(r->invoiceNumber == inum){
                        r->taxMoney = ba1->getValue();
                        r->isCommon = false;
                    }
                    else
                        errors<<tr("（%1#%2*）遇到一个销项税分录的发票号（%3）无配对应收项")
                                .arg(pz->number()).arg(j+1).arg(inum);
                }
            }
            //如果是应收的贷方，则根据提取到的客户和发票号，看是否可以在已存列表中销账
            else if(scanXf && ba->getFirstSubject() == ysFSub && ba->getDir() == MDIR_D){
                QList<int> months; QList<QStringList> invoiceNums;
                PaUtils::extractInvoiceNum(ba->getSummary(),months,invoiceNums);
                if(invoiceNums.isEmpty())
                    continue;
                QSet<QString> inums;
                foreach(QStringList nums, invoiceNums)
                    inums += nums.toSet();
                for(int k = 0; k < incomes.count(); ++k){
                    InvoiceRecord* rd = incomes.at(k);                    
                    if(inums.contains(rd->invoiceNumber)){
                        if(rd->customer == ba->getSecondSubject()->getNameItem()){
                            inums.remove(rd->invoiceNumber);
                            if(!reserved){
                                dels<<incomes.takeAt(k);
                                k--;
                            }
                            else{
                                //大多数情况下，可能一收就销账，但个别情况不是，在程序中很难判断
                                //比如，一次收多张发票的应收款，而且这些票可能有本币和外币的不同混合情形存在
                                rd->state = CAS_OK;
                            }
                        }
                        else{
                            errors<<tr("（%1#%2*）遇到一个应收发票号（%3）有匹配但客户不匹配的分录")
                                    .arg(pz->number()).arg(j+1).arg(inum);
                        }
                    }
                }
            }
            //如果是应付的贷方，则提取客户、发票号和金额
            else if(ba->getFirstSubject() == yfFSub && ba->getDir() == MDIR_D &&
                    PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wbMoney)){
                r = new InvoiceRecord;
                r->invoiceNumber = inum;
                r->customer = ba->getSecondSubject()->getNameItem();
                r->baRID = ba->getId();
                r->money = ba->getValue();
                r->wmoney = wbMoney;
                r->pzNumber = pz->number();
                r->wmt = ba->getMt();
                r->year = year();
                r->month = month;
                r->isIncome = false;
                costs<<r;
                //如果上一条分录是该发票对应的进项税，则在提取税金
                BusiAction* ba1 = pz->getBusiAction(j-1);
                if(!ba1)
                    continue;
                if(ba1->getSecondSubject() == jxSSub && ba1->getDir() == MDIR_J &&
                        PaUtils::extractOnlyInvoiceNum(ba1->getSummary(),inum,wbMoney)){
                    if(r->invoiceNumber == inum){
                        r->taxMoney = ba1->getValue();
                        r->isCommon = false;
                    }
                    else
                        errors<<tr("（%1#%2*）遇到一个进项税分录的发票号（%3）无配对应付项")
                                .arg(pz->number()).arg(j+1).arg(inum);
                }
            }
            //如果是应付的借方，则根据提取到的客户和发票号，看是否可以在已存列表中销账
            else if(scanXf && ba->getFirstSubject() == yfFSub && ba->getDir() == MDIR_J){
                QList<int> months; QList<QStringList> invoiceNums;
                PaUtils::extractInvoiceNum(ba->getSummary(),months,invoiceNums);
                if(invoiceNums.isEmpty())
                    continue;
                QSet<QString> inums;
                foreach(QStringList nums, invoiceNums)
                    inums += nums.toSet();
                for(int k = 0; k < costs.count(); ++k){
                    InvoiceRecord* rd = costs.at(k);
                    if(inums.contains(rd->invoiceNumber)){
                        if(rd->customer == ba->getSecondSubject()->getNameItem()){
                            inums.remove(rd->invoiceNumber);
                            if(!reserved){
                                dels<<costs.takeAt(k);
                                k--;
                            }
                            else{
                                rd->state = CAS_OK;
                            }
                        }
                        else{
                            errors<<tr("（%1#%2*）遇到一个应付发票号（%3）有匹配但客户不匹配的分录")
                                    .arg(pz->number()).arg(j+1).arg(inum);
                        }
                    }
                }
            }
        }
    }
    account->getDbUtil()->removeInvoiceRecords(dels);
    qDeleteAll(dels);
    dels.clear();
    closePzSet();
}

/**
 * @brief 扫描指定月份凭证集，分离出应收应付的增加项和销账项
 * @param month
 * @param incomeAdds    应收增加项列表
 * @param incomeCancels 应收销账项列表
 * @param costAdds      应付增加项列表
 * @param costCancels   应付销账项列表
 * @param errors
 */
void AccountSuiteManager::scanYsYfForMonth2(int month, QList<InvoiceRecord *> &incomeAdds, QList<InvoiceRecord *> &incomeCancels, QList<InvoiceRecord *> &costAdds, QList<InvoiceRecord *> &costCancels, QStringList &errors)
{
    if(!open(month)){
        errors<<tr("打开%1月凭证集时发生错误！").arg(month);
        return;
    }
    SubjectManager* sm = account->getSubjectManager(suiteRecord->subSys);
    FirstSubject* ysFSub = sm->getYsSub();
    FirstSubject* yfFSub = sm->getYfSub();
    SecondSubject* xxSSub = sm->getXxseSSub();
    SecondSubject* jxSSub = sm->getJxseSSub();
    InvoiceRecord* r;
    for(int i = 0; i < pzs->count(); ++i){
        PingZheng* pz = pzs->at(i);
        for(int j = 0; j < pz->baCount(); ++j){
            BusiAction* ba = pz->getBusiAction(j);
            //如果是应收的借方，则提取客户、发票号和金额
            QString inum; Double wbMoney;
            if(ba->getFirstSubject() == ysFSub && ba->getDir() == MDIR_J &&
                    PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wbMoney)){
                r = new InvoiceRecord;
                r->invoiceNumber = inum;
                r->customer = ba->getSecondSubject()->getNameItem();
                r->baRID = ba->getId();
                r->money = ba->getValue();
                r->wmoney = wbMoney;
                r->pzNumber = pz->number();
                r->wmt = ba->getMt();
                r->year = year();
                r->month = month;
                incomeAdds<<r;
                //如果下一条分录是该发票对应的销项税，则在已添加的列表中查找对应发票号
                BusiAction* ba1 = pz->getBusiAction(j+1);
                if(!ba1)
                    continue;
                if(ba1->getSecondSubject() == xxSSub && ba1->getDir() == MDIR_D &&
                        PaUtils::extractOnlyInvoiceNum(ba1->getSummary(),inum,wbMoney)){
                    if(r->invoiceNumber == inum){
                        r->taxMoney = ba1->getValue();
                        r->isCommon = false;
                    }
                    else
                        errors<<tr("（%1#%2*）遇到一个销项税分录的发票号（%3）无配对应收项")
                                .arg(pz->number()).arg(j+1).arg(inum);
                }
            }
            //如果是应收的贷方，则根据提取到的客户和发票号，看是否可以在已存列表中销账
            else if(ba->getFirstSubject() == ysFSub && ba->getDir() == MDIR_D){
                QList<int> months; QList<QStringList> invoiceNums;
                PaUtils::extractInvoiceNum(ba->getSummary(),months,invoiceNums);
                if(invoiceNums.isEmpty())
                    continue;
                foreach(QStringList nums, invoiceNums){
                    foreach(QString num, nums){
                        InvoiceRecord* r = new InvoiceRecord;
                        r->baRID = ba->getId();
                        r->pzNumber = pz->number();
                        r->invoiceNumber = num;
                        r->customer = ba->getSecondSubject()->getNameItem();
                        r->year = year();
                        r->month = month;
                        incomeCancels<<r;
                    }
                }
            }
            //如果是应付的贷方，则提取客户、发票号和金额
            else if(ba->getFirstSubject() == yfFSub && ba->getDir() == MDIR_D &&
                    PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wbMoney)){
                r = new InvoiceRecord;
                r->invoiceNumber = inum;
                r->customer = ba->getSecondSubject()->getNameItem();
                r->baRID = ba->getId();
                r->money = ba->getValue();
                r->wmoney = wbMoney;
                r->pzNumber = pz->number();
                r->wmt = ba->getMt();
                r->year = year();
                r->month = month;
                r->isIncome = false;
                costAdds<<r;
                //如果上一条分录是该发票对应的进项税，则在提取税金
                BusiAction* ba1 = pz->getBusiAction(j-1);
                if(!ba1)
                    continue;
                if(ba1->getSecondSubject() == jxSSub && ba1->getDir() == MDIR_J &&
                        PaUtils::extractOnlyInvoiceNum(ba1->getSummary(),inum,wbMoney)){
                    if(r->invoiceNumber == inum){
                        r->taxMoney = ba1->getValue();
                        r->isCommon = false;
                    }
                    else
                        errors<<tr("（%1#%2*）遇到一个进项税分录的发票号（%3）无配对应付项")
                                .arg(pz->number()).arg(j+1).arg(inum);
                }
            }
            //如果是应付的借方，则根据提取到的客户和发票号，看是否可以在已存列表中销账
            else if(ba->getFirstSubject() == yfFSub && ba->getDir() == MDIR_J){
                QList<int> months; QList<QStringList> invoiceNums;
                PaUtils::extractInvoiceNum(ba->getSummary(),months,invoiceNums);
                if(invoiceNums.isEmpty())
                    continue;
                foreach(QStringList nums, invoiceNums){
                    foreach(QString num, nums){
                        InvoiceRecord* r = new InvoiceRecord;
                        r->baRID = ba->getId();
                        r->pzNumber = pz->number();
                        r->isIncome = false;
                        r->invoiceNumber = num;
                        r->customer = ba->getSecondSubject()->getNameItem();
                        r->year = year();
                        r->month = month;
                        costCancels<<r;
                    }
                }
            }
        }
    }
}

/**
 * @brief 统计未销账的历史应收应付发票情况
 * 每个账户只需执行一次，用来初始化账户的最后帐套内现存未销账应收应付发票
 * 以辅助分录模板的正常运作（即在创建应收应付分录时可以自动按发票号填写相关金额）
 * @param incomes
 * @param costs
 * @param errors
 */
void AccountSuiteManager::scanYsYfForInit(QList<InvoiceRecord *> &incomes, QList<InvoiceRecord *> &costs, QStringList &errors)
{
    //前一个月增加应收应付发票，从次月开始同时增加和抵消发票
    //为简单起见，只在最新帐套的开始月份扫描
    if(isPzSetOpened())
        closePzSet();
    int em = suiteRecord->startMonth;
    PzsState state = getState(em);
    while(state == Ps_Jzed){
        em++;
        state = getState(em);
    }
    for(int m = suiteRecord->startMonth; m < em; ++m)
        scanYsYfForMonth(m,incomes,costs,errors,m>suiteRecord->startMonth,false);
}

/**
 * @brief 扫描当前凭证集的发票使用情况
 */
void AccountSuiteManager::scanInvoice(QList<InvoiceRecord*> &incomes, QList<InvoiceRecord*> &costs, QStringList &errors)
{
//    （1）主营业务收入在贷方，且摘要中可以提取一个发票号，则认为是一个有效的收入发票
//    （2）与此相对，主营业务在借方，且摘要中可以提取一个发票号，则认为是一个有效的成本发票

//    （3）如果存在主营业务收入在贷方，但摘要中无法提取发票号，且其他分录仅包含在借方的应收和在贷方的销项税，
//    则可以认为是应收发票的聚集凭证，则每个应收对应一张发票，如果有销项税，则是专票。

//    (4)与此相反的是成本聚集凭证

//    对于收入成本抵扣型的凭证，只能出现在非聚集凭证中，且有如下特征：
//    有与发票号相连的收入/成本项，且存在包含银行存款的分录


//    扫描算法：
//    如果遇到一个符合第（1）或（2）的分录，则可以直接提取发票号
//    如果凭证的第一条分录是包含发票号的应收，且在借方，且最后一条分录是没有发票号的主营业务收入在贷方，则视为收入聚集凭证
//    如果凭证的第一条分录是没有发票号的成本在借方，则视为成本聚集凭证。
    if(!isPzSetOpened() || pzs->isEmpty())
        return;
    SubjectManager* sm = getSubjectManager();
    FirstSubject* fs_income = sm->getZysrSub();
    FirstSubject* fs_cost = sm->getZycbSub();
    SecondSubject* ss_xx = sm->getXxseSSub();
    SecondSubject* ss_jx = sm->getJxseSSub();
    foreach(PingZheng* pz, *pzs){
        bool ok = false;
        if(!isGatherIncomePz(pz,ok)){
            LOG_WARNING(QString("Don't distinguish ping zheng class on scan invoice number(pz number is %1)").arg(pz->number()));
            continue;
        }
        if(ok){
            scanInvoiceGatherIncome(pz,incomes,errors);
            continue;
        }
        ok = false;
        if(!isGatherCostPz(pz,ok)){
            LOG_WARNING(QString("Don't distinguish ping zheng class on scan invoice number(pz number is %1)").arg(pz->number()));
            continue;
        }
        if(ok){
            scanInvoiceGatherCost(pz,costs,errors);
            continue;
        }
        //为了查找的方便，这里对于分录出现的顺序有个约定：对于同一张发票，收入/成本在前，税金在后
        for(int i = 0; i < pz->baCount(); ++i){
            BusiAction* ba = pz->getBusiAction(i);
            QString invoiceNumber;
            Double wMoney;
            //主营业务收入            
            if(ba->getFirstSubject() == fs_income && ba->getDir() == MDIR_D &&
                    PaUtils::extractOnlyInvoiceNum(ba->getSummary(),invoiceNumber,wMoney)){
                InvoiceRecord* r = new InvoiceRecord;
                r->year = suiteRecord->year;
                r->month = curM;
                r->invoiceNumber = invoiceNumber;
                r->money = ba->getValue();
                r->pzNumber = pz->number();
                r->baRID = ba->getId();
                r->state = CAS_OK;
                //提取客户名简称
                QString customerName;
                if(!PaUtils::extractCustomerName(ba->getSummary(),customerName))
                    errors<<tr("（%1#%2*）从摘要中提取客户名失败！").arg(pz->number()).arg(i+1);
                if(customerName.isEmpty() || !(r->customer=sm->getNameItem(customerName)))
                    errors<<tr("（%1#%2*）从摘要中提取到一个账户中还不存在的客户名“%3”！")
                            .arg(pz->number()).arg(i+1).arg(customerName);
                //如果下一条分录是与其对应的税额则合并处理
                if(i < pz->baCount()-1){
                    i++;
                    BusiAction* ba1 = pz->getBusiAction(i);
                    QString inum; Double v;
                    if(ba1->getSecondSubject() == ss_xx && ba1->getDir() == MDIR_D &&
                           PaUtils::extractOnlyInvoiceNum(ba1->getSummary(),inum,v) &&
                            inum == invoiceNumber){
                        r->isCommon = false;
                        r->money += ba1->getValue();
                        r->taxMoney = ba1->getValue();
                    }
                }
                incomes<<r;
            }
            else if(ba->getFirstSubject() == fs_cost && ba->getDir() == MDIR_J &&
                    PaUtils::extractOnlyInvoiceNum(ba->getSummary(),invoiceNumber,wMoney)){
                InvoiceRecord* r = new InvoiceRecord;
                r->year = suiteRecord->year;
                r->month = curM;
                r->isIncome = false;
                r->invoiceNumber = invoiceNumber;
                r->money = ba->getValue();
                r->pzNumber = pz->number();
                r->baRID = ba->getId();
                r->state = CAS_OK;
                QString customerName;
                if(!PaUtils::extractCustomerName(ba->getSummary(),customerName))
                    errors<<tr("（%1#%2*）从摘要中提取客户名失败！").arg(pz->number()).arg(i+1);
                if(customerName.isEmpty() || !(r->customer=sm->getNameItem(customerName)))
                    errors<<tr("（%1#%2*）从摘要中提取到一个账户中还不存在的客户名“%3”！")
                            .arg(pz->number()).arg(i+1).arg(customerName);
                if(i < pz->baCount()-1){
                    i++;
                    BusiAction* ba1 = pz->getBusiAction(i);
                    QString inum; Double v;
                    if(ba1->getSecondSubject() == ss_jx && ba1->getDir() == MDIR_J &&
                           PaUtils::extractOnlyInvoiceNum(ba1->getSummary(),inum,v) &&
                            inum == invoiceNumber){
                        r->isCommon = false;
                        r->money += ba1->getValue();
                        r->taxMoney = ba1->getValue();
                    }
                }
                costs<<r;
            }
        }
    }
}

//扫描聚合收入凭证
void AccountSuiteManager::scanInvoiceGatherIncome(PingZheng *pz, QList<InvoiceRecord *> &incomes,QStringList &errors)
{
    //按约定，最后一条分录是收入的汇总，摘要中一般不出现发票号，
    //对于每张发票，第一条分录是应收在借方，如果后面跟了相同发票号码的销项税分录，则此发票为专票
    //否者是普票
    SubjectManager* sm = getSubjectManager();
    FirstSubject* fs_ys = sm->getYsSub();
    SecondSubject* ss_xx = sm->getXxseSSub();
    Money* usd = account->getAllMoneys().value(USD);
    for(int i = 0; i < pz->baCount()-1; ++i){
        BusiAction* ba = pz->getBusiAction(i);
        QString invoiceNumber;
        Double wMoney;
        if(ba->getFirstSubject() == fs_ys && ba->getDir() == MDIR_J &&
                PaUtils::extractOnlyInvoiceNum(ba->getSummary(),invoiceNumber,wMoney)){
            InvoiceRecord* r = new InvoiceRecord;
            r->year = suiteRecord->year;
            r->month = curM;
            r->pzNumber = pz->number();
            r->baRID = ba->getId();
            r->invoiceNumber = invoiceNumber;
            r->customer = ba->getSecondSubject()->getNameItem();
            r->money = ba->getValue();
            if(wMoney != 0){
                r->wmt = usd;
                r->wmoney = wMoney;
            }
            incomes<<r;
        }
        else if(ba->getSecondSubject() == ss_xx && ba->getDir() == MDIR_D &&
                PaUtils::extractOnlyInvoiceNum(ba->getSummary(),invoiceNumber,wMoney)){
            if(!incomes.isEmpty() && incomes.last()->invoiceNumber == invoiceNumber){
                incomes.last()->isCommon = false;
                incomes.last()->taxMoney = ba->getValue();
            }
            else
                errors<<tr("（%1#%2*）遇到一个发票号（%3）无配对应收项的销项税分录")
                        .arg(pz->number()).arg(i+1).arg(invoiceNumber);
        }
    }
}

//扫描聚合成本凭证
void AccountSuiteManager::scanInvoiceGatherCost(PingZheng *pz, QList<InvoiceRecord *> &costs, QStringList &errors)
{
    //按约定，第一条分录是成本的汇总，摘要中一般不出现发票号，
    //对于每张发票，第一条分录如果是进项税在借方，则是专票，否者是普票
    //后面跟了相同发票号码的应付在贷方

    SubjectManager* sm = getSubjectManager();
    FirstSubject* fs_yf = sm->getYfSub();
    SecondSubject* ss_jx = sm->getJxseSSub();
    Money* usd = account->getAllMoneys().value(USD);
    QString preInvoiceNumber;   //记录先前发现的发票号（从进项税额所在分录提取到的）
    Double preTaxMoney;         //税额
    for(int i = 1; i < pz->baCount(); ++i){
        BusiAction* ba = pz->getBusiAction(i);
        QString invoiceNumber;
        Double wMoney;
        if(ba->getSecondSubject() == ss_jx && ba->getDir() == MDIR_J &&
                PaUtils::extractOnlyInvoiceNum(ba->getSummary(),invoiceNumber,wMoney)){
            preInvoiceNumber = invoiceNumber;
            preTaxMoney = ba->getValue();
        }
        else if(ba->getFirstSubject() == fs_yf && ba->getDir() == MDIR_D &&
                PaUtils::extractOnlyInvoiceNum(ba->getSummary(),invoiceNumber,wMoney)){
            InvoiceRecord* r = new InvoiceRecord;
            r->year = suiteRecord->year;
            r->month = curM;
            r->pzNumber = pz->number();
            r->baRID = ba->getId();
            r->isIncome = false;
            if(!preInvoiceNumber.isEmpty()){
                if(preInvoiceNumber == invoiceNumber){
                    r->isIncome = false;
                    r->taxMoney = preTaxMoney;
                }
                else{
                    errors<<tr("（%1#%2*）遇到一个发票号（%3）无配对应付项的进项税分录")
                            .arg(pz->number()).arg(i+1).arg(preInvoiceNumber);
                }
                preInvoiceNumber.clear();
                preTaxMoney = 0;
            }
            r->invoiceNumber = invoiceNumber;
            r->customer = ba->getSecondSubject()->getNameItem();
            r->money = ba->getValue();
            if(wMoney != 0){
                r->wmt = usd;
                r->wmoney = wMoney;
            }
            costs<<r;
        }
    }
}

/**
 * @brief //是否是聚合收入凭证
 * @param pz
 * @param ok    true：是，false：不是
 * @return false：无法识别
 */
bool AccountSuiteManager::isGatherIncomePz(PingZheng *pz, bool &ok)
{
    int baNums = pz->baCount();
    if(baNums <= 1)
        return false;    
    SubjectManager* sm = getSubjectManager();
    FirstSubject* fs_ys = sm->getYsSub();
    FirstSubject* fs_sr = sm->getZysrSub();
    SecondSubject* ss_xx = sm->getXxseSSub();
    for(int i = 0; i < pz->baCount(); ++i){
        BusiAction* ba = pz->getBusiAction(i);
        if(ba->getFirstSubject() == fs_ys && ba->getDir() == MDIR_J)
            continue;
        else if(ba->getFirstSubject() == fs_sr && ba->getDir() == MDIR_D)
            continue;
        else if(ba->getSecondSubject() == ss_xx && ba->getDir() == MDIR_D)
            continue;
        else{
            ok = false;
            return true;
        }
    }
    ok = true;
    return true;
}

//是否是聚合成本凭证
bool AccountSuiteManager::isGatherCostPz(PingZheng *pz, bool &ok)
{
    if(pz->baCount() <= 1)
        return false;
    SubjectManager* sm = getSubjectManager();
    FirstSubject* fs_yf = sm->getYfSub();
    FirstSubject* fs_cb = sm->getZycbSub();
    SecondSubject* ss_jx = sm->getJxseSSub();
    for(int i = 0; i < pz->baCount(); ++i){
        BusiAction* ba = pz->getBusiAction(i);
        if(ba->getFirstSubject() == fs_yf && ba->getDir() == MDIR_D)
            continue;
        else if(ba->getFirstSubject() == fs_cb && ba->getDir() == MDIR_J)
            continue;
        else if(ba->getSecondSubject() == ss_jx && ba->getDir() == MDIR_J)
            continue;
        else{
            ok = false;
            return true;
        }
    }
    ok = true;
    return true;
}

/**
 * @brief 装载本帐套内的应收应付发票缓存
 */
void AccountSuiteManager::loadYsYf()
{
    account->getDbUtil()->getInvoiceRecordsForYear(this,ysInvoices,yfInvoices);
    isYsYfLoaded = true;
}

/**
 * @brief 保存本月收入/成本发票
 * @return
 */
bool AccountSuiteManager::saveInCost()
{
    if(!account->getDbUtil()->saveCurInvoice(year(),curM,incomes) ||
            !account->getDbUtil()->saveCurInvoice(year(),curM,costs)){
        return false;
    }
    return true;
}

/**
 * @brief 装载本月收入/成本发票
 */
void AccountSuiteManager::loadInCost()
{
    if(!isPzSetOpened())
        return;
    if(!account->getDbUtil()->loadCurInvoice(year(),curM,incomes) ||
       !account->getDbUtil()->loadCurInvoice(year(),curM,costs,false)){
        myHelper::ShowMessageBoxError(tr("读取本地发票记录时出错！"));
        return;
    }
    //开始一次孤立别名的匹配（有些发票可能前次已经通过创建孤立别名-即创建新名称对象，来匹配客户，但由于表格内没有与孤立别名连接的字段，所以在这里进行处理）
    QList<NameItemAlias*> isolatedAlias;
    isolatedAlias = getSubjectManager()->getAllIsolatedAlias();
    QHash<QString,NameItemAlias*> matched;
    CurInvoiceRecord* r;
    for(int i = 0; i < incomes.count(); ++i){
        r = incomes.at(i);
        if(r->ni)
            continue;
        r->alias = matched.value(r->client);
        if(!r->alias){
            foreach(NameItemAlias* nia, isolatedAlias){
                if(nia->longName() == r->client){
                    r->alias = nia;
                    break;
                }
            }
        }
        if(!r->alias)
            continue;
        r->ni = new SubjectNameItem(r->alias);
    }
    for(int i = 0; i < costs.count(); ++i){
        r = costs.at(i);
        if(r->ni)
            continue;
        r->alias = matched.value(r->client);
        if(!r->alias){
            foreach(NameItemAlias* nia, isolatedAlias){
                if(nia->longName() == r->client){
                    r->alias = nia;
                    break;
                }
            }
        }
        if(!r->alias)
            continue;
        r->ni = new SubjectNameItem(r->alias);
    }
    isICLoader = true;
}

/**
 * @brief 清空本地已导入的发票
 * @param scope
 * @return
 */
bool AccountSuiteManager::clearInCost(int scope)
{
    if(!isPzSetOpened())
        return false;
    if(!account->getDbUtil()->clearCurInvoice(year(),curM,scope))
        return false;
    if(scope == 1 || scope == 0){
        qDeleteAll(incomes);
        incomes.clear();
    }
    else if(scope == 2 || scope == 0){
        qDeleteAll(costs);
        costs.clear();
    }
    return true;
}

/**
 * @brief 返回当前月份收入/成本发票记录列表
 * @param isIncome
 * @return
 */
QList<CurInvoiceRecord *> *AccountSuiteManager::getCurInvoiceRecords(bool isIncome)
{
    if(!isICLoader)
        loadInCost();
    if(isIncome)
        return &incomes;
    else
        return &costs;
}

/**
 * @brief 验证指定发票在凭证分录中的处理是否正确
 * @param invoiceNumber 发票号码
 * @param wbMoney       外币金额
 * @param ba            分录对象
 * @param isGather      是否聚合凭证
 * @param isIncome      收入/成本
 * @return 错误类型：二进制4位到0位如果置位，则依次表示：外币错误、方向错误、科目设置错误、数据错误、不存在
 */
int AccountSuiteManager::verifyCurInvoice(QString invoiceNumber, Double wbMoney, BusiAction *ba, bool isGather, bool isIncome)
{
    SubjectManager* sm = getSubjectManager();
    QList<CurInvoiceRecord *> *rs;
    int EC_NOT = 1,EC_DATA = 2,EC_SUB = 4,EC_DIR = 8,EC_WBDATA=16;
    if(isIncome)
        rs = &incomes;
    else
        rs = &costs;
    if(!ba->getFirstSubject() || !ba->getSecondSubject())
        return EC_SUB;
    int result = EC_NOT;
    foreach(CurInvoiceRecord* r, *rs){
        if(r->inum != invoiceNumber)
            continue;
        r->ba = ba; r->pz = ba->getParent();
        result &= 0;
        if(isIncome){   //收入发票
            if(isGather){ //收入聚合凭证，应收在借方，数据是发票金额
                if(ba->getFirstSubject() == sm->getYsSub()){
                    if(ba->getDir() != MDIR_J)
                        result |= EC_DIR;
                    if(ba->getValue() != r->money)
                        result |= EC_DATA;
                    if(wbMoney != r->wbMoney)
                        result |= EC_WBDATA;
                }
                else if(ba->getFirstSubject() == sm->getYjsjSub()){
                    if(ba->getDir() != MDIR_D)
                        result |= EC_DIR;
                    if(ba->getValue() != r->taxMoney)
                        result |= EC_DATA;
                    if(ba->getSecondSubject() != sm->getXxseSSub())
                        result |= EC_SUB;
                }
                else if(ba->getFirstSubject() == sm->getZysrSub())
                    continue;
                else
                    result |= EC_SUB;
            }
            else{
                if(ba->getFirstSubject() == sm->getYjsjSub()){ //应交税金-销项在贷方，金额等于税额
                    if(ba->getSecondSubject() != sm->getXxseSSub())
                        result |= EC_SUB;
                    if(ba->getDir() != MDIR_D)
                        result |= EC_DIR;
                    if(ba->getValue() != r->taxMoney)
                        result |= EC_DATA;
                }
                else if(ba->getFirstSubject() == sm->getZysrSub()){ //主营收入在贷方，金额等于发票金额- 发票税额
                    if(ba->getDir() != MDIR_D)
                        result |= EC_DIR;
                    if(ba->getValue() != r->money - r->taxMoney)
                        result |= EC_DATA;
                }
                else
                    result |= EC_SUB;
            }
        }
        else{   //成本发票
            if(isGather){ //成本聚合凭证，应付在贷方，数据是发票金额
                if(ba->getFirstSubject() == sm->getYfSub()){
                    if(ba->getDir() != MDIR_D)
                        result |= EC_DIR;
                    if(ba->getValue() != r->money)
                        result |= EC_DATA;
                    if(wbMoney != r->wbMoney)
                        result |= EC_WBDATA;
                }
                else if(ba->getFirstSubject() == sm->getYjsjSub()){
                    if(ba->getSecondSubject() != sm->getJxseSSub())
                        result |= EC_SUB;
                    if(ba->getDir() != MDIR_J)
                        result |= EC_DIR;
                    if(ba->getValue() != r->taxMoney)
                        result |= EC_DATA;
                }
                else if(ba->getFirstSubject() == sm->getZycbSub())
                    continue;
                else
                    result |= EC_SUB;
            }
            else{
                if(ba->getFirstSubject() == sm->getYjsjSub()){ //应交税金-进项在借方，金额等于税额
                    if(ba->getSecondSubject() != sm->getJxseSSub())
                        result |= EC_SUB;
                    if(ba->getDir() != MDIR_J)
                        result |= EC_DIR;
                    if(ba->getValue() != r->taxMoney)
                        result |= EC_DATA;
                }
                else if(ba->getFirstSubject() == sm->getZycbSub()){ //主营成本在借方，金额等于发票金额- 发票税额
                    if(ba->getDir() != MDIR_J)
                        result |= EC_DIR;
                    if(ba->getValue() != r->money - r->taxMoney)
                        result |= EC_DATA;
                }
                else
                    result |= EC_SUB;
            }
        }
        if(result == 0){
            r->errors.clear();
            r->processState = 1;
        }
        else{
            if((result & 2) != 0)
                r->errors.append(tr("发票金额错误\n"));
            if((result & 4) != 0)
                r->errors.append(tr("科目设置错误\n"));
            if((result & 8) != 0)
                r->errors.append(tr("方向错误\n"));
            if((result & 16) != 0)
                r->errors.append(tr("外币金额错误\n"));
            if((result & 30) != 0)
                r->processState = 2;
            else if((result & 1) != 0)
                r->processState = 0;
        }
        break;
    }
    return result;
}

/**
 * @brief 验证本期所有收入/成本发票的账务处理是否有效（金额、适用科目和方向）
 * @param   errInfo
 * @return
 */
bool AccountSuiteManager::verifyCurInvoices(QString &errInfo)
{
    bool ok = true;
    SubjectManager* sm = getSubjectManager();
    QSet<FirstSubject*> fsubs;
    fsubs.insert(sm->getYsSub());
    fsubs.insert(sm->getYfSub());
    fsubs.insert(sm->getYjsjSub());
    fsubs.insert(sm->getZysrSub());
    fsubs.insert(sm->getZycbSub());
    foreach(PingZheng* pz, *pzs){
        bool isGather = false;
        if(!isGatherIncomePz(pz,isGather) && !isGatherCostPz(pz,isGather)){
            errInfo.append(tr("无法识别是否是聚合凭证（凭证号：“%1”）！\n").arg(pz->number()));
            continue;
        }
        foreach(BusiAction* ba, pz->baList()){
            FirstSubject* fsub = ba->getFirstSubject();
            if(!fsubs.contains(fsub))
                continue;
            QString inum; Double wbMoney;
            if(!PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wbMoney))
                continue;            
            if(fsub == sm->getYjsjSub() && ba->getSecondSubject() != sm->getJxseSSub() &&
                ba->getSecondSubject() != sm->getXxseSSub()){
                errInfo.append(tr("凭证（%1#）第%2条分录无法判定是收入或成本相关！").arg(pz->number()).arg(ba->getNumber()));
                continue;
            }
            bool isIncome = fsub == sm->getYsSub() || fsub == sm->getZysrSub() ||
                    fsub == sm->getYjsjSub() && ba->getSecondSubject() == sm->getXxseSSub();
            if(verifyCurInvoice(inum,wbMoney,ba,isGather,isIncome) != 0)
                ok = false;
        }
    }
    return ok;
}

QList<InvoiceRecord *> AccountSuiteManager::getYsInvoiceStats()
{
    if(!isYsYfLoaded)
        loadYsYf();
    return ysInvoices;
}

QList<InvoiceRecord *> AccountSuiteManager::getYfInvoiceStats()
{
    if(!isYsYfLoaded)
        loadYsYf();
    return yfInvoices;
}

//QList<InvoiceRecord *> AccountSuiteManager::getYsInvoices()
//{
//    if(!isYsYfLoaded)
//}

//QList<InvoiceRecord *> AccountSuiteManager::getYfInvoices()
//{

//}

/**
 * @brief 查找应收应付发票号
 * @param isYs  true：应收，false：应付
 * @param inum  发票号
 * @return 与发票号对应的记录项
 */
InvoiceRecord *AccountSuiteManager::searchYsYfInvoice(bool isYs, QString inum)
{
    if(!isYsYfLoaded)
        loadYsYf();
    QList<InvoiceRecord*> *pi;
    if(isYs)
        pi = &ysInvoices;
    else
        pi = &yfInvoices;
    for(int i = 0; i < pi->count(); ++i){
        InvoiceRecord* r = pi->at(i);
        if(r->invoiceNumber == inum)
            return r;
    }
    return 0;
}

/**
 * @brief 查找收入/成本发票记录
 * @param isIncome
 * @param inum
 * @return
 */
CurInvoiceRecord *AccountSuiteManager::searchICInvoice(bool isIncome, QString inum)
{
    if(!isICLoader)
        loadInCost();
    QList<CurInvoiceRecord*> *rs;
    if(isIncome)
        rs = &incomes;
    else
        rs = &costs;
    for(int i = 0; i < rs->count(); ++i){
        CurInvoiceRecord* r = rs->at(i);
        if(r->inum == inum)
            return r;
    }
    return 0;
}

/**
 * @brief 将应收应付发票统计缓存中的内容保存到数据库中
 * @return
 */
bool AccountSuiteManager::saveYsYf()
{
    if(!account->getDbUtil()->saveInvoiceRecords(ysInvoices))
        return false;
    return account->getDbUtil()->saveInvoiceRecords(yfInvoices);
}

/**
 * @brief 指定月份的汇率发生了改变，需要重新统计（凭证集的统计和凭证集内每个凭证的借贷方合计值）
 * @param month
 */
void AccountSuiteManager::rateChanged(int month)
{
    if((month==0 && curM != 0) || (month!=0 && curM == month)){
        if(pzs->isEmpty())
            return;
        foreach(PingZheng* pz, pzSetHash[curM])
            pz->reCalSums();
        statUtil->stat();
        dirty = true;
        emit pzSetChanged();
    }
}

/**
 * @brief PzSetMgr::needRestat
 *  由于凭证集内的凭证数值的改变，需要进行重新统计来得到正确的余额
 */
void AccountSuiteManager::needRestat()
{
    setExtraState(false);
    emit pzExtraStateChanged(false);
}

/**
 * @brief PzSetMgr::pzChangedInSet
 *  打开的凭证集内的一个凭证的内容发生了改变，必须通知给凭证集对象
 * @param pz
 */
void AccountSuiteManager::pzChangedInSet(PingZheng *pz)
{
    //目前的实现只触发凭证集改变信号以启用主窗口的保存按钮
    emit pzSetChanged();
}

void AccountSuiteManager::pzStateChanged(PzState oldState, PzState newState)
{
    _determinePzSetState(states[curM]);
    emit pzCountChanged(pzs->count());
}





