#include "pz.h"
#include "tables.h"
#include "global.h"
#include "utils.h"
#include "otherModule.h"

/////////////////PingZheng/////////////////////////////////////////
PingZheng::PingZheng(User* user, QSqlDatabase db):user(user),db(db)
{
    eState = NEW;
}

PingZheng::PingZheng(int id,QString date,int pnum,int znum,double js,double ds,
          PzClass pcls,int encnum,PzState state,User* vu,User* ru, User* bu,
                     User* user,QSqlDatabase db)
    :ID(id),date(date),pnum(pnum),znum(znum),js(js),ds(ds),pzCls(pcls),
      encNum(encnum),state(state),vu(vu),ru(ru),bu(bu),user(user),db(db)
{
    eState = INIT;
}

PingZheng::PingZheng(PzData* data,User* puser,QSqlDatabase db):
    ID(data->pzId),date(data->date),pnum(data->pzNum),znum(data->pzZbNum),
    js(data->jsum),ds(data->dsum),pzCls(data->pzClass),encNum(data->attNums),
    state(data->state),vu(data->verify),ru(data->producer),bu(data->bookKeeper),
    db(db)
{
//    ID = data->pzId;
//    date = data->date;
//    pnum = data->pzNum;
//    znum = data->pzZbNum;
//    js = data->jsum;
//    ds = data->dsum;
//    pzCls = data->pzClass;
//    encNum = data->attNums;
//    state = data->state;
//    vu = data->verify;
//    ru = data->producer;
//    bu = data->bookKeeper;
//    user = puser;
//    this->db = db;
}

bool PingZheng::save()
{
    QSqlQuery q(db);
    QString s;

    if(ID == 0){
        s = "insert into PingZhengs() values()";
        if(!q.exec(s))
            return false;
        s = "select id from PingZhengs";
        if(!q.exec(s))
            return false;
        if(!q.last())
            return false;
        ID = q.value(0).toInt();
    }
    return update();

}

bool PingZheng::update()
{
    QSqlQuery q(db);
    QString s;

    //保存凭证数据
    s = QString("update PingZhengs(date,number,zbNum,jsum,dsum,isForward,"
                "encNum,pzState,vuid,ruid,buid) values('%1',%2,"
                "%3,%4,%5,%6,%7,%8,%9,%10,%11) where id=%12")
            .arg(date).arg(pnum).arg(znum).arg(js).arg(ds).arg(pzCls)
            .arg(encNum).arg(state).arg(vu->getUserId())
            .arg(ru->getUserId()).arg(bu->getUserId()).arg(ID);
    if(!q.exec(s))
        return false;

    //保存凭证的会计分录
    BusiActionData* ba;
    //保存有效的会计分录
    for(int i = 0; i < baLst.count(); ++i){
        ba = baLst[i];
        if(ba->state == BusiActionData::NEW){
            if(ba->dir == DIR_J)
                s = QString("insert into BusiActions(pid,summary,firSubID,"
                            "secSubID,moneyType,jMoney,dMoney,dir,NumInPz) "
                            "values(%1,'%2',%3,%4,%5,%6,0,%7,%8)").arg(ID)
                        .arg(ba->summary).arg(ba->fid).arg(ba->sid).arg(ba->mt)
                        .arg(ba->v).arg(ba->dir).arg(i);
            else
                s = QString("insert into BusiActions(pid,summary,firSubID,"
                            "secSubID,moneyType,jMoney,dMoney,dir,NumInPz) "
                            "values(%1,'%2',%3,%4,%5,0,%6,%7,%8)").arg(ID)
                        .arg(ba->summary).arg(ba->fid).arg(ba->sid).arg(ba->mt)
                        .arg(ba->v).arg(ba->dir).arg(i);
        }
        else if(ba->state == BusiActionData::EDITED){
            if(ba->dir == DIR_J)
                s = QString("update BusiActions set summary='%1',firSubID=%2,secSubID=%3,"
                            "moneyType=%4,jMoney=%5,dMoney=0,dir=%6,NumInPz=%7 where id=%8")
                        .arg(ba->summary).arg(ba->fid).arg(ba->sid).arg(ba->mt)
                        .arg(ba->v).arg(DIR_J).arg(i).arg(ba->id);
            else
                s = QString("update BusiActions set summary='%1',firSubID=%2,secSubID=%3,"
                            "moneyType=%4,jMoney=0,dMoney=%5,dir=%6,NumInPz=%7 where id=%8")
                        .arg(ba->summary).arg(ba->fid).arg(ba->sid).arg(ba->mt)
                        .arg(ba->v).arg(DIR_D).arg(i).arg(ba->id);
        }
        else if(ba->state == BusiActionData::NUMCHANGED)
            s = QString("update BusiActions set NumInPz=%1 where id=%2").arg(i).arg(ba->id);
        if(!q.exec(s))
            return false;
    }

    //移除被删除的会计分录
    for(int i = 0; i < delLst.count(); ++i){
        s = QString("delete from BusiActions where id=%1").arg(delLst[i]->id);
        if(!q.exec(s))
            return false;
    }
    return true;
}

PingZheng* load(int id,QSqlDatabase db)
{

}

PingZheng* create(User* user,QSqlDatabase db)
{

}

PingZheng* create(QString date,int pnum,int znum,double js,double ds,
                             PzClass pcls,int encnum,PzState state,User* vu,
                             User* ru, User* bu,User* user,QSqlDatabase db)
{

}


//凭证排序函数
bool byDateLessThan(PingZheng* p1, PingZheng* p2)
{
    return p1->getDate() < p2->getDate();
}

bool byPzNumLessThan(PingZheng* p1, PingZheng* p2)
{
    return p1->number() < p2->number();
}

bool byZbNumLessThan(PingZheng* p1, PingZheng* p2)
{
    return p1->ZbNumber() < p2->ZbNumber();
}

/////////////////PzSetMgr///////////////////////////////////////////
PzSetMgr::PzSetMgr(int y, int m, User* user, QSqlDatabase db):
    y(y),m(m),user(user),db(db)
{
    isOpened = false;
    isReStat = false;
    isReSave = false;
    maxPzNum = 0;
    maxZbNum = 0;
}

//打开凭证集
bool PzSetMgr::open()
{
    QSqlQuery q(db),q1(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select * from PingZhengs where date like '%1%'").arg(ds);
    isOpened = q.exec(s);
    if(!isOpened){
        state = Ps_NoOpen;
        return false;
    }

    PingZheng* pz;
    QList<BusiActionData*> bs;
    BusiActionData* bd;
    int id,pnum,znum,encnum;
    QString d;
    PzClass pzCls;
    PzState pzState;
    User *vu,*ru,*bu;
    double jsum,dsum;
    while(q.next()){
        id = q.value(0).toInt();
        d = q.value(PZ_DATE).toString();
        pnum = q.value(PZ_NUMBER).toInt();
        znum = q.value(PZ_ZBNUM).toInt();
        jsum = q.value(PZ_JSUM).toDouble();
        dsum = q.value(PZ_DSUM).toDouble();
        pzCls = (PzClass)q.value(PZ_CLS).toInt();
        encnum = q.value(PZ_ENCNUM).toInt();
        pzState = (PzState)q.value(PZ_PZSTATE).toInt();
        vu = allUsers.value(q.value(PZ_VUSER).toInt());
        ru = allUsers.value(q.value(PZ_RUSER).toInt());
        bu = allUsers.value(q.value(PZ_BUSER).toInt());

        pz = new PingZheng(id,d,pnum,znum,jsum,dsum,pzCls,encnum,pzState,
                           vu,ru,bu,curUser,db);

        QString as = QString("select * from BusiActions where pid=%1").arg(pz->id());
        if(!q1.exec(as)){
            state = Ps_NoOpen;
            return false;
        }
        while(q1.next()){
            bd = new BusiActionData;
            bd->id = q1.value(0).toInt();
            bd->pid = q1.value(BACTION_PID).toInt();
            bd->summary = q1.value(BACTION_SUMMARY).toString();
            bd->fid = q1.value(BACTION_FID).toInt();
            bd->sid = q1.value(BACTION_SID).toInt();
            bd->mt = q1.value(BACTION_MTYPE).toInt();
            bd->dir = q1.value(BACTION_DIR).toInt();
            if(bd->dir == DIR_J)
                bd->v = q1.value(BACTION_JMONEY).toDouble();
            else
                bd->v = q1.value(BACTION_DMONEY).toDouble();
            bd->state = BusiActionData::INIT;
            bs<<bd;
        }
        pz->setBaList(bs);
        pz->setEditState(PingZheng::INIT);
        bs.clear();
        pds<<pz;
    }

    if(!BusiUtil::getPzsState(y,m,state)){
        state = Ps_NoOpen;
        if(!BusiUtil::setPzsState(y,m,state))
            return false;
    }
    maxPzNum = pds.count() + 1;
    maxZbNum = 0;
    for(int i = 0; i < pds.count(); ++i)
        if(maxZbNum < pds[i]->number())
            maxZbNum = pds[i]->number();
    maxZbNum++;

    return true;
}

void PzSetMgr::close()
{
    save();
    state = Ps_NoOpen;
    maxPzNum = 0;
    maxZbNum = 0;
}

bool PzSetMgr::isOpen()
{
    return isOpened;
}

//获取凭证总数（也即已用的最大凭证号）
int PzSetMgr::getPzCount()
{
    return maxPzNum-1;
}

//获取最大可用凭证自编号
int PzSetMgr::getMaxZbNum()
{
//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    QString fs = QString("date like '%1%'").arg(ds);
//    QSqlQuery q(db);
//    QString s = QString("select max(number) from PingZhengs where ")
//            .append(fs);
//    q.exec(s);
//    if(q.first())
//        return q.value(0).toInt() + 1;
//    else
//        return 1;
    return maxZbNum;
}

//重置凭证号
bool PzSetMgr::resetPzNum(int by)
{
    //1：表示按日期顺序，2：表示按自编号顺序
    if(by == 1){
        qSort(pds.begin(),pds.end(),byDateLessThan);
        for(int i = 0; i < pds.count(); ++i){
            pds[i]->setNumber(i+1);
            pds[i]->setEditState(PingZheng::INFOEDITED);
        }
        return true;
    }
    if(by == 2){
        qSort(pds.begin(),pds.end(),byZbNumLessThan);
        for(int i = 0; i < pds.count(); ++i){
            pds[i]->setNumber(i+1);
            pds[i]->setEditState(PingZheng::INFOEDITED);
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
//    //（此刷新动作主要使凭证状态从录入态转到审核态）
//    //执行此函数的时机是，用户修改了凭证状态（而非凭证的其他数据）
//    bool ori = false;  //初始态（任一手工录入凭证处于录入状态时，它为真）
//    bool handV = true; //所有手工录入凭证处于审核或入账状态时，它为真
//    bool handE = false;//手工录入凭证是否存在

//    bool imp = false;  //引入态（任一自动引入的凭证还未审核或入账时，它为真）
//    bool impV = true;  //引入审核态（所有自动引入的凭证都已审核或入账时，它为真）
//    bool impE = false; //引入凭证是否存在

//    bool jzhd = false;  //结转汇兑态（任一结转汇兑损益的凭证处于录入态时，它为真）
//    bool jzhdV = true;  //结转汇兑审核态（所有结转汇兑损益的凭证已审核或入账时，它为真）
//    bool jzhdE = false; //结转汇兑损益的凭证是否存在


//    bool jzsy = false; //结转损益态（任一结转损益的凭证处于录入态时，它为真）
//    bool jzsyV = true; //结转损益审核态（所有结转损益的凭证处于已审核或入账时，它为真）
//    bool jzsyE = false;//结转损益的凭证是否存在

//    bool jzlr = false; //结转本年利润态（结转本年利润的凭证处于录入态时，它为真）
//    bool jzlrV = true; //结转本年利润态（结转本年利润的凭证已审核或入账时，它为真）
//    bool jzlrE = false;//结转本年利润的凭证是否存在

//    PzsState oldPs, newPs = Ps_Rec;  //原先保存的和新的凭证集状态
//    if(!state(oldPs)){
//        qDebug() << "Don't get current pingzheng set state!!";
//        return false;
//    }
//    if(oldPs == Ps_Jzed)  //如果已结账，则不用继续
//        return true;

//    int state;
//    int pzCls;
//    for(int i = 0; i < model->rowCount(); ++i){
//        state = model->data(model->index(i,PZ_PZSTATE)).toInt();
//        pzCls = model->data(model->index(i,PZ_CLS )).toInt();

//        if(state == Pzs_Repeal)
//            continue;
//        //如果是手工录入凭证
//        if(pzCls == Pzc_Hand){
//            handE = true;
//            if(state == Pzs_Recording){
//                ori = true;
//                handV = false;
//                break;
//            }
//        }
//        //如果是其他模块引入的凭证
//        else if(BusiUtil::isImpPzCls(pzCls)){
//            impE = true;
//            if(state == Pzs_Recording){
//                imp = true;
//                impV = false;
//                break;
//            }
//        }
//        //如果是结转汇兑损益的凭证
//        else if(BusiUtil::isJzhdPzCls(pzCls)){
//            jzhdE = true;
//            if(state == Pzs_Recording){
//                jzhd = true;
//                jzhdV = false;
//                break;
//            }
//        }
//        //如果是结转损益的凭证
//        else if(BusiUtil::isJzsyPzCls(pzCls)){
//            jzsyE = true;
//            if(state == Pzs_Recording){
//                jzsy = true;
//                jzsyV = false;
//                break;
//            }
//        }
//        //如果是结转本年利润的凭证
//        else if((pzCls == Pzc_Jzlr)){
//            jzlrE = true;
//            if(state == Pzs_Recording){
//                jzlr = true;
//                jzlrV = false;
//                break;
//            }
//        }
//    }

//    //根据凭证的扫描情况以及凭证集的先前状态，决定新状态
//    if(ori)
//        newPs = Ps_Rec;
//    else if(imp)
//        newPs = Ps_ImpOther;
//    else if(jzhd)
//        newPs = Ps_Jzhd;
//    else if(jzsy)
//        newPs = Ps_Jzsy;
//    else if(jzlr)
//        newPs = Ps_Jzbnlr;

//    else if((oldPs == Ps_Jzbnlr)  && jzlrV && jzlrE)   //如果已经审核了结转本年利润凭证
//        newPs = Ps_JzbnlrV;
//    else if((oldPs == Ps_Jzsy) && jzsyV && jzsyE) //如果结转损益凭证都进行了审核
//        newPs = Ps_JzsyV;
//    else if((oldPs == Ps_Jzhd) && jzhdV && jzhdE) //如果结转汇兑损益凭证都进行了审核
//        newPs = Ps_JzhdV;
//    else if((oldPs == Ps_ImpOther) && impV && impE) //如果引入的其他凭证都审核通过
//        newPs = Ps_ImpV;
//    else if((oldPs == Ps_Rec) && handV && handE) //如果所有手工录入的凭证都审核通过
//        newPs = Ps_HandV;

//    setstate(newPs);
}

//返回凭证集状态
PzsState PzSetMgr::getState()
{
    return state;
}

//设置凭证集状态
void PzSetMgr::setstate(PzsState state)
{
    this->state = state;
    //return BusiUtil::setPzsState(y,m,state);
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
    if(m == 1){
        yy = y - 1;
        mm = 12;
    }
    else{
        yy = y;
        mm = m - 1;
    }
    return BusiUtil::readExtraByMonth2(yy,mm,preExtra,preDir,preDetExtra,preDetDir);
}

//添加空白凭证
bool PzSetMgr::appendBlankPz(PzData* pd)
{
    if(!isOpened)
        return false;

}

//插入凭证，参数ecode错误代码（1：凭证号越界，2：自编号冲突）
bool PzSetMgr::insert(PzData* pd,int& ecode)
{
    //插入凭证要保证凭证集内凭证号和自编号的连贯性要求
    //凭证号必须从1开始，顺序增加，中间不能间断，自编号必须保证唯一性
    if(pd->pzNum > maxPzNum){
        ecode = 1;
        return false;
    }
    if(isZbNumConflict(pd->pzZbNum)){
        ecode = 2;
        return false;
    }

}

//自编号是否冲突
bool PzSetMgr::isZbNumConflict(int num)
{
    for(int i = 0; i < pds.count(); ++i){
        int zn = pds[i]->ZbNumber();
        if(num == zn)
            return true;
    }
    return false;
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

    if(!Dtfy::genImpPzData(y,m,pzds,user)){
        if(!pzds.empty()){
            for(int i = 0; i < pzds.count(); ++i)
                delete pzds[i];
            pzds.clear();
        }
        return false;
    }

    if(pzds.empty()){
        QMessageBox::information(0,QObject::tr("提示信息"),QObject::tr("未发现要引入的待摊费用"));
        return true;
    }
    for(int i = 0; i < pzds.count(); ++i){
        PingZheng pz(pzds[i],user,db);
        pz.save();
        delete pzds[i];
    }
    pzds.clear();
    return true;
}

//创建当期计提待摊费用凭证
bool PzSetMgr::crtDtfyTxPz()
{
    return Dtfy::createTxPz(y,m,user);
}

//删除当期计提待摊费用凭证
bool PzSetMgr::delDtfyPz()
{
    return Dtfy::repealTxPz(y,m);
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

