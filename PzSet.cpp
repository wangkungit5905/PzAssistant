#include "global.h"
#include "tables.h"
#include "PzSet.h"
#include "pz.h"
#include "dbutil.h"
#include "statutil.h"


/////////////////PzSetMgr///////////////////////////////////////////
PzSetMgr::PzSetMgr(Account *account, User *user):
    account(account),user(user),curY(0),curM(0)
{
    dbUtil = account->getDbUtil();
    statUtil = NULL;
    state = Ps_NoOpen;
    isReStat = false;
    isReSave = false;
    maxPzNum = 0;
    maxZbNum = 0;
    if(!user)
        user = curUser;
}

//打开凭证集
bool PzSetMgr::open(int y, int m)
{
    if(state != Ps_NoOpen)    //在打开指定年月的凭证集，必须显式调用close关闭先前已打开的凭证集
        return false;

    int key = genKey(y,m);
    if(!pzSetHash.contains(key)){
        if(!dbUtil->loadPzSet(y,m,pzSetHash[key],this))
            return false;
        if(!dbUtil->getPzsState(y,m,states[key]))
            return false;
        state=states.value(key);
        extraStates[key] = dbUtil->getExtraState(y,m);
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
    return true;
}

void PzSetMgr::close()
{
    save();
    curY=0;curM=0;
    state = Ps_NoOpen;
    maxPzNum = 0;
    maxZbNum = 0;
    isReStat = false;
    isReSave = false;
    delete statUtil;
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
    if(state == Ps_NoOpen)
        return 0;
    return maxPzNum-1;
}

//重置凭证号
bool PzSetMgr::resetPzNum(int by)
{
    if(state == Ps_NoOpen)
        return true;

    //1：表示按日期顺序，2：表示按自编号顺序
    if(by == 1){
        qSort(pds.begin(),pds.end(),byDateLessThan);
        for(int i = 0; i < pds.count(); ++i){
            pds[i]->setNumber(i+1);
            //pds[i]->setEditState(PingZheng::INFOEDITED);
        }
        return true;
    }
    if(by == 2){
        qSort(pds.begin(),pds.end(),byZbNumLessThan);
        for(int i = 0; i < pds.count(); ++i){
            pds[i]->setNumber(i+1);
            //pds[i]->setEditState(PingZheng::INFOEDITED);
        }
        return true;
    }
    else
        return false;

//    QSqlQuery q(db),q1(db);
//    QString s;

//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    if(by == 1) //按凭证日期
//        s = QString("select id from PingZhengs where "
//                    "date like '%1%' order by date").arg(ds);
//    else  if(by == 2)      //按自编号
//        s = QString("select id from PingZhengs where "
//                    "date like '%1%' order by zbNum").arg(ds);
//    else
//        return false;
//    if(!q.exec(s))
//        return false;
//    int id, num = 1;
//    while(q.next()){
//        id = q.value(0).toInt();
//        s = QString("update PingZhengs set number=%1 where id=%2").arg(num++).arg(id);
//        if(!q1.exec(s))
//            return false;
//    }
//    model->select();
//    return true;
}

//根据凭证集内的每个凭证的状态来确定凭证集的状态
bool PzSetMgr::determineState()
{
    if(state == Ps_NoOpen)
        return false;

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
void PzSetMgr::setstate(PzsState state,int y, int m )
{
    int yy,mm;
    if(y==0 && m==0){
        yy=curY;mm=curM;
    }
    int key = genKey(yy,mm);
    if(!states.contains(key))
        dbUtil->setPzsState(y,m,state);
    states[key] = state;

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
        yy=curY,mm=curM;
    }
    int key = genKey(yy,mm);
    if(!extraStates.contains(key))
        dbUtil->setExtraState(yy,mm,state);
    extraStates[key] = state;
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
QHash<int, Double> &PzSetMgr::getRates()
{
    QHash<int,Double> rates;
    if(curY==0 && curM==0)
        return rates;
    account->getRates(curY,curM,rates);
    return rates;
}

//添加空白凭证
PingZheng* PzSetMgr::appendPz(PzClass pzCls)
{
    if(state == Ps_NoOpen)
        return NULL;
    QString ds = QDate(curY,curM,1).toString(Qt::ISODate);
    return new PingZheng(this,0,ds,maxPzNum++,maxZbNum++,0.0,0.0,pzCls,0,Pzs_Recording);
}

//插入凭证，参数ecode错误代码（1：凭证号越界，2：自编号冲突）
bool PzSetMgr::insert(PingZheng* pd,int& ecode)
{
    //插入凭证要保证凭证集内凭证号和自编号的连贯性要求
    //凭证号必须从1开始，顺序增加，中间不能间断，自编号必须保证唯一性
    if(pd->number() > maxPzNum){
        ecode = 1;
        return false;
    }
    if(isZbNumConflict(pd->zbNumber())){
        ecode = 2;
        return false;
    }

}

/**
 * @brief PzSetMgr::isZbNumConflict
 *  自编号是否冲突
 * @param num
 * @return true：冲突，false：不冲突
 */
bool PzSetMgr::isZbNumConflict(int num)
{
    if(state == Ps_NoOpen)
        return false;
    foreach(PingZheng* pz, pzSetHash.value(genKey(curY,curM)))
        if(num == pz->zbNumber())
            return true;
    return false;
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
bool PzSetMgr::remove(int pzNum)
{

}

//保存凭证
bool PzSetMgr::savePz()
{

}

//保存凭证集
bool PzSetMgr::save()
{

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

//获取凭证集的数据模型，提供给凭证编辑窗口
CustomRelationTableModel* PzSetMgr::getModel()
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
