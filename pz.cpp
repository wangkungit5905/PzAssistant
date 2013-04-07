#include <QHash>
#include <QSqlRecord>

#include "pz.h"
#include "tables.h"
#include "global.h"
#include "PzSet.h"


//////////////////////////BusiAction////////////////////////////////////
void BusiAction::setParent(PingZheng *p)
{
    if(parent != p){
        parent = p;
        witchEdited |= ES_BA_PARENT;
        parent->witchEdited |= ES_PZ_BACTION;
    }
}

void BusiAction::setSummary(QString s)
{
    QString su = s.trimmed();
    if(summary != su){
        summary = su;
        witchEdited |= ES_BA_SUMMARY;
        parent->witchEdited |= ES_PZ_BACTION;
    }
}

void BusiAction::setFirstSubject(FirstSubject *fsub)
{
    if(this->fsub != fsub){
        this->fsub = fsub;
        witchEdited |= ES_BA_FSUB;
        parent->witchEdited |= ES_PZ_BACTION;
    }
}

void BusiAction::setSecondSubject(SecondSubject *ssub)
{
    if(this->ssub != ssub){
        this->ssub = ssub;
        witchEdited |= ES_BA_SSUB;
        parent->witchEdited |= ES_PZ_BACTION;
    }
}

void BusiAction::setMt(Money *mt)
{
    if(this->mt != mt){
        this->mt = mt;
        witchEdited |= ES_BA_MT;
        if(mt)
            parent->calSum();
        parent->witchEdited |= ES_PZ_BACTION;
        /*
        //因为币种变了，所有要调整币值
        QHash<int,Double> rates;
        rates = parent->parent()->getRates(parent->parent());*/
    }
}

void BusiAction::setValue(Double value)
{
    if(v != value){
        v = value;
        witchEdited |= ES_BA_VALUE;
        parent->calSum();
        parent->witchEdited |= ES_PZ_BACTION;
    }
}

void BusiAction::setDir(MoneyDirection direct)
{
    if(dir != direct){
        dir = direct;
        witchEdited |= ES_BA_DIR | ES_BA_VALUE;
        parent->calSum();
        parent->witchEdited |= ES_PZ_BACTION;
    }
}

void BusiAction::setNumber(int number)
{
    if(num != number){
        num = number;
        witchEdited |= ES_BA_NUMBER;
        parent->witchEdited |= ES_PZ_BACTION;
    }
}

//bool BusiAction::operator ==(const BusiAction other)
//{
//    if(md == other.md)
//        return true;
//    else
//        return false;
//}


/////////////////PingZheng/////////////////////////////////////////
PingZheng::PingZheng(PzSetMgr *parent):ID(0),p(parent),witchEdited(ES_PZ_INIT),
    isDeleted(false),encNum(0),ru(NULL),vu(NULL),bu(NULL){md=PZMD++;}

PingZheng::PingZheng(int id, QString date, int pnum, int znum, Double js, Double ds,
          PzClass pcls, int encnum, PzState state, User* vu, User* ru, User* bu, PzSetMgr* parent)
    :ID(id),date(date),pnum(pnum),znum(znum),js(js),ds(ds),pzCls(pcls),
      encNum(encnum),state(state),vu(vu),ru(ru),bu(bu),witchEdited(ES_PZ_INIT),
      isDeleted(false),p(parent)
{
    md=PZMD++;
}

//PingZheng::PingZheng(PzData2* data,User* puser,QSqlDatabase db):
//    ID(data->pzId),date(data->date),pnum(data->pzNum),znum(data->pzZbNum),
//    js(data->jsum),ds(data->dsum),pzCls(data->pzClass),encNum(data->attNums),
//    state(data->state),vu(data->verify),ru(data->producer),bu(data->bookKeeper),
//    witchEdited(ES_PZ_INIT),isDeleted(false)
//{
//}



//保存凭证的会计分录的顺序
//bool PingZheng::saveBaOrder()
//{
//    QSqlQuery q(db);
//    QString s;

//    for(int i = 0; i < baLst.count(); ++i){
//        s = QString("update %1 set %2=%3 where id=%4")
//                .arg(tbl_ba).arg(fld_ba_number).arg(i+1).arg(baLst[i]->id);
//        if(!q.exec(s))
//            return false;
//    }
//    return true;
//}

//保存全新的凭证（已经有了对应的记录，只是未保存凭证的账面信息和会计分录部分）
//bool PingZheng::saveNewPz()
//{
//    QSqlQuery q(db);
//    QString s;

//    s = QString("update %1 set %2='%3',%4=%5,%6=%7,%8=%9,%10=%11,%12=%13,"
//                "%14=%15,%16=%17,%18=%19,")
//            .arg(tbl_pz).arg(fld_pz_date).arg(date).arg(fld_pz_number).arg(pnum)
//            .arg(fld_pz_zbnum).arg(znum).arg(fld_pz_jsum).arg(js.getv())
//            .arg(fld_pz_dsum).arg(ds.getv()).arg(fld_pz_class).arg(pzCls)
//            .arg(fld_pz_encnum).arg(encNum).arg(fld_pz_state).arg(state)
//            .arg(fld_pz_ru).arg(ru->getUserId());
//    if(vu == NULL)
//        s.append(QString("%1=0,").arg(fld_pz_vu));
//    else
//        s.append(QString("%1=%2,").arg(fld_pz_vu).arg(vu->getUserId()));
//    if(bu == NULL)
//        s.append(QString("%1=0,").arg(fld_pz_bu));
//    else
//        s.append(QString("%1=%2,").arg(fld_pz_bu).arg(bu->getUserId()));
//    s.chop(1);
//    s.append(QString(" where id=%1").arg(ID));
//    if(!q.exec(s))
//        return false;

//    //保存会计分录
//    BusiActionData2* ba;
//    int num = 0;
//    for(int i = 0; i < baLst.count(); ++i){
//        ba = baLst[i];
//        if(ba->state == BusiActionData2::BLANK)
//            continue;
//        ++num;
//        if(ba->dir == DIR_J)
//            s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) "
//                        "values(%11,'%12',%13,%14,%15,%16,0,%17,%18)")
//                    .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary)
//                    .arg(fld_ba_fid).arg(fld_ba_sid).arg(fld_ba_mt)
//                    .arg(fld_ba_jv).arg(fld_ba_dv).arg(fld_ba_dir).arg(fld_ba_number)
//                    .arg(ID).arg(ba->summary).arg(ba->fid).arg(ba->sid)
//                    .arg(ba->mt).arg(ba->v.getv()).arg(ba->dir).arg(num);
//        else
//            s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) "
//                        "values(%11,'%12',%13,%14,%15,0,%16,%17,%18)")
//                    .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary)
//                    .arg(fld_ba_fid).arg(fld_ba_sid).arg(fld_ba_mt)
//                    .arg(fld_ba_jv).arg(fld_ba_dv).arg(fld_ba_dir).arg(fld_ba_number)
//                    .arg(ID).arg(ba->summary).arg(ba->fid).arg(ba->sid)
//                    .arg(ba->mt).arg(ba->v.getv()).arg(ba->dir).arg(num);
//        if(!q.exec(s))
//            return false;
//        ba->state == BusiActionData2::INIT;
//    }
//    eState = PZINIT;
//    editStates.fill(false,PzEditBitNum);
//    return true;
//}

//保存凭证的信息数据部分
//bool PingZheng::saveInfoPart()
//{
//    QSqlQuery q(db);
//    QString s;

//    s = QString("update %1 set %2='%3',%4=%5,%6=%7,%8=%9,"
//                "%10=%11,%12=%13,%14=%15,")
//            .arg(tbl_pz).arg(fld_pz_date).arg(date).arg(fld_pz_number).arg(pnum)
//            .arg(fld_pz_zbnum).arg(znum).arg(fld_pz_class).arg(pzCls)
//            .arg(fld_pz_encnum).arg(encNum).arg(fld_pz_state).arg(state)
//            .arg(fld_pz_ru).arg(ru->getUserId());
//    if(vu == NULL)
//        s.append(QString("%1=0,").arg(fld_pz_vu));
//    else
//        s.append(QString("%1=%2,").arg(fld_pz_vu).arg(vu->getUserId()));
//    if(bu == NULL)
//        s.append(QString("%1=0,").arg(fld_pz_bu));
//    else
//        s.append(QString("%1=%2,").arg(fld_pz_bu).arg(bu->getUserId()));
//    s.chop(1);
//    s.append(QString(" where id=%1").arg(ID));
//    if(!q.exec(s))
//        return false;

//    //保存会计分类的摘要部分（凭证的信息部分，在会计分录中只涉及到会计分录的摘要）
//    BusiActionData2* ba;
//    for(int i = 0; i < baLst.count(); ++i){
//        ba = baLst[i];
//        if(ba->state == BusiActionData2::EDITED){
//            s = QString("update %1 set %2='%3' where id=%4")
//                    .arg(tbl_ba).arg(fld_ba_summary).arg(ba->summary).arg(ba->id);
//            if(!q.exec(s))
//                return false;
//            ba->state = BusiActionData2::INIT;
//        }
//    }
//    return true;
//}

//保存凭证的金额部分（会计分录的添加、移除，改变会计分录的科目、币种、方向和金额）
//也包含了对凭证的信息部分内容的保存
//bool PingZheng::saveContent()
//{
//    QSqlQuery q(db);
//    QString s;

//    s = QString("update %1 set ").arg(tbl_pz);
//    if(editStates.testBit(DATE))
//        s.append(QString("%1='%2',").arg(fld_pz_date).arg(date));
//    if(editStates.testBit(PZNUM))
//        s.append(QString("%1=%2,").arg(fld_pz_number).arg(pnum));
//    if(editStates.testBit(ZBNUM))
//        s.append(QString("%1=%2,").arg(fld_pz_zbnum).arg(znum));
//    if(editStates.testBit(ENCNUM))
//        s.append(QString("%1=%2,").arg(fld_pz_encnum).arg(encNum));
//    if(editStates.testBit(JSUM))
//        s.append(QString("%1=%2,").arg(fld_pz_jsum).arg(js.getv()));
//    if(editStates.testBit(DSUM))
//        s.append(QString("%1=%2,").arg(fld_pz_dsum).arg(ds.getv()));
//    if(editStates.testBit(PZSTATE))
//        s.append(QString("%1=%2,").arg(fld_pz_state).arg(state));
//    if(editStates.testBit(RUSER))
//        s.append(QString("%1=%2,").arg(fld_pz_ru).arg(ru->getUserId()));
//    if(editStates.testBit(VUSER)){
//        if(vu == NULL)
//            s.append(QString("%1=0,").arg(fld_pz_vu));
//        else
//            s.append(QString("%1=%2,").arg(fld_pz_vu).arg(vu->getUserId()));
//    }
//    if(editStates.testBit(BUSER)){
//        if(bu == NULL)
//            s.append(QString("%1=0,").arg(fld_pz_bu));
//        else
//            s.append(QString("%1=%2,").arg(fld_pz_bu).arg(bu->getUserId()));
//    }
//    s.chop(1);
//    s.append(QString(" where id=%1").arg(ID));
//    if(!q.exec(s))
//        return false;
//    //保存凭证的会计分录
//    BusiActionData2* ba;
//    int num=0;
//    for(int i = 0; i < baLst.count(); ++i){
//        ba = baLst[i];
//        if(ba->state == BusiActionData2::BLANK) //忽略空白行
//            continue;
//        if(ba->state == BusiActionData2::INIT){  //初始行不用保存
//            ++num;
//            continue;
//        }
//        else{
//            ++num;
//            if(ba->state == BusiActionData2::NEW){
//                if(ba->dir == DIR_J)
//                    s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) "
//                                "values(%11,'%12',%13,%14,%15,%16,0,%17,%18)")
//                            .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary)
//                            .arg(fld_ba_fid).arg(fld_ba_sid).arg(fld_ba_mt)
//                            .arg(fld_ba_jv).arg(fld_ba_dv).arg(fld_ba_dir).arg(fld_ba_number)
//                            .arg(ID).arg(ba->summary).arg(ba->fid).arg(ba->sid)
//                            .arg(ba->mt).arg(ba->v.getv()).arg(ba->dir).arg(num);
//                else
//                    s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) "
//                                "values(%11,'%12',%13,%14,%15,0,%16,%17,%18)")
//                            .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary)
//                            .arg(fld_ba_fid).arg(fld_ba_sid).arg(fld_ba_mt)
//                            .arg(fld_ba_jv).arg(fld_ba_dv).arg(fld_ba_dir).arg(fld_ba_number)
//                            .arg(ID).arg(ba->summary).arg(ba->fid).arg(ba->sid)
//                            .arg(ba->mt).arg(ba->v.getv()).arg(ba->dir).arg(num);
//            }
//            else if(ba->state == BusiActionData2::EDITED){
//                s = QString("update %1 set ").arg(tbl_ba);
//                if(ba->editStates.testBit(BusiActionData2::SUMMARY))
//                    s.append(QString("%1='%2',").arg(fld_ba_summary).arg(ba->summary));
//                if(ba->editStates.testBit(BusiActionData2::FSUB))
//                    s.append(QString("%1=%2,").arg(fld_ba_fid).arg(ba->fid));
//                if(ba->editStates.testBit(BusiActionData2::SSUB))
//                    s.append(QString("%1=%2,").arg(fld_ba_sid).arg(ba->sid));
//                if(ba->editStates.testBit(BusiActionData2::MT))
//                    s.append(QString("%1=%2,").arg(fld_ba_mt).arg(ba->mt));
//                if(ba->editStates.testBit(BusiActionData2::VALUE)){
//                    if(ba->dir == DIR_J)
//                        s.append(QString("%1=%2,%3=0,").arg(fld_ba_jv).arg(ba->v.getv())
//                                 .arg(fld_ba_dv));
//                    else
//                        s.append(QString("%1=0,%2=%3,").arg(fld_ba_jv).arg(fld_ba_dv)
//                                 .arg(ba->v.getv()));
//                }
//                if(ba->editStates.testBit(BusiActionData2::NUMBER))
//                    s.append(QString("%1=%2,").arg(fld_ba_number).arg(num));
//                s.chop(1);
//                s.append(QString(" where id=%1").arg(ba->id));
//            }
//            if(!q.exec(s))
//                return false;
//            ba->state = BusiActionData2::INIT;
//        }
//    }

//    //移除被删除的会计分录
//    for(int i = 0; i < delLst.count(); ++i){
//        s = QString("delete from %1 where id=%2")
//                .arg(tbl_ba).arg(delLst[i]->id);
//        if(!q.exec(s))
//            return false;
//    }
//    eState = PZINIT;
//    editStates.fill(false,PzEditBitNum);
//    return true;
//}

//bool PingZheng::update()
//{
//    //如果是全新的凭证
//    if(eState == PZNEW)
//        return saveNewPz();
//    //如果是金额部分发生了改变，则要保存所有两部分
//    else if(eState == PZEDITED)
//        return saveContent();
//    //如果只是凭证的信息字段内容发生了改变，则无需保存会计分录的数据
//    //else if(eState == PZINFOEDITED)
//    //    return saveInfoPart();
//    //如果只是会计分录的顺序发生改变，则只需重新设置会计分录的顺序
//    //if(eState == PZORDERCHANGED)
//    //    return saveBaOrder();
//}

/**
 * @brief PingZheng::appendBlank 添加空白分录
 * @return
 */
BusiAction* PingZheng::appendBlank()
{
    BusiAction* ba = new BusiAction;
    ba->setParent(this);
    baLst<<ba;
    ba->setNumber(baLst.count());
    witchEdited |= ES_PZ_BACTION;
    curBa = ba;
    return ba;
}

/**
 * @brief PingZheng::append
 * @param ba
 * @param isUpdate：当加入会计分录后，是否更新借贷合计值（js,ds），默认为更新
 *        此参数为假，往往使用在凭证集的打开方法内部，凭证对象的初始化阶段
 * @return
 * 添加会计分录
 */
bool PingZheng::append(BusiAction *ba, bool isUpdate)
{
    if(!ba){
        LOG_ERROR(QObject::tr("BusiAction object is NULL!"));
        return false;
    }
    if(hasBusiAction(ba))
        return false;

    ba->setParent(this);
    baLst<<ba;
    if(isUpdate){
        if(ba->dir == DIR_J){
            js += ba->v;
            witchEdited |= ES_PZ_JSUM;
        }
        else{
            ds += ba->v;
            witchEdited |= ES_PZ_DSUM;
        }
        witchEdited |= ES_PZ_BACTION;
        curBa = ba;
    }
    ba->setNumber(baLst.count());
    return true;
}


/**
 * @brief PingZheng::insert
 * @param index
 * @param bd
 * 插入会计分录
 */
bool PingZheng::insert(int index,BusiAction *ba)
{
    if(!ba){
        LOG_ERROR(QObject::tr("BusiAction object is NULL!"));
        return false;
    }
    if(hasBusiAction(ba))
        return false;
    if(baDels.contains(ba)){
        baDels.removeOne(ba);
        ba->setDelete(false);
    }
    else
        ba->setParent(this);

    int idx;
    if(index >= baLst.count()){
        idx = baLst.count();
    }
    else if(index < 0)
        idx = 0;
    else
        idx = index;

    ba->setNumber(idx+1);
    baLst.insert(idx,ba);
    if(ba->dir == DIR_J){
        js += ba->v;
        witchEdited |= ES_PZ_JSUM;
    }
    else{
        ds += ba->v;
        witchEdited |= ES_PZ_DSUM;
    }
    for(int i = idx; i < baLst.count(); ++i)
        baLst.at(i)->setNumber(i + 1);
    witchEdited |= ES_PZ_BACTION;
    return true;
}

/**
 * @brief PingZheng::remove  移除会计分录
 * @param index
 */
bool PingZheng::remove(int index)
{
    if(index >= baLst.count() || index < 0){
        LOG_ERROR(QObject::tr("when remove BusiAction object in PingZheng object(id:%1,pnum:%2) index overflow!")
                  .arg(ID).arg(pnum));
        return false;
    }
    BusiAction *ba = baLst.takeAt(index);
    if(index < baLst.count()-1)
        for(int i = index; i < baLst.count(); ++i)
            baLst.at(i)->setNumber(index+1);
    if(ba->dir == DIR_J){
        js -= ba->v;
        witchEdited |= ES_PZ_JSUM;
    }
    else{
        ds -= ba->v;
        witchEdited |= ES_PZ_DSUM;
    }
    witchEdited |= ES_PZ_BACTION;

    ba->setDelete(true);
    baDels<<ba;
    return true;
}

/**
 * @brief PingZheng::remove 移除指定会计分录对象
 * @param ba
 * @return
 */
bool PingZheng::remove(BusiAction *ba)
{
    int idx;
    for(idx = 0; idx < baLst.count(); ++idx){
        if(baLst.at(idx) == ba)
            break;
    }
    if(idx == baLst.count()) //未找到
        return false;

    baLst.takeAt(idx);
    ba->setDelete(true);
    baDels<<ba;

    if(baList().empty())
        curBa = NULL;
    else if( baLst.count() == 1)
        curBa = baLst.at(0);
    else if(idx == baLst.count())
        curBa = baLst.at(idx-1);
    else
        curBa = baLst.at(idx);

    if(idx < baLst.count()-1)
        for(int i = idx; i < baLst.count(); ++i)
            baLst.at(i)->setNumber(idx+1);
    if(ba->dir == DIR_J){
        js -= ba->v;
        witchEdited |= ES_PZ_JSUM;
    }
    else{
        ds -= ba->v;
        witchEdited |= ES_PZ_DSUM;
    }
    witchEdited |= ES_PZ_BACTION;
    return true;
}

/**
 * @brief PingZheng::restore
 * 恢复被删除的会计分录对象（会计分录被删除后未执行保存操作，则此对象仍保留在删除队列中）
 * @param ba
 * @return true：成功，false：对象不存在
 */
bool PingZheng::restore(BusiAction *ba)
{
    for(int i = 0; i < baDels.count(); ++i){
        if(*baDels.at(i) == *ba){
            baDels.takeAt(i);
            insert(ba->getNumber()-1,ba);
            return true;
        }
    }
    curBa = ba;
    return false;
}

/**
 * @brief PingZheng::take
 * 提取会计分录对象
 * @param index
 * @return
 */
BusiAction *PingZheng::take(int index)
{
    if(index >= baLst.count() || index < 0){
        LOG_ERROR(QObject::tr("when remove BusiAction object in PingZheng object(id:%1,pnum:%2) index overflow!")
                  .arg(ID).arg(pnum));
        return NULL;
    }
    BusiAction *ba = baLst.takeAt(index);
    if(index < baLst.count()-1)
        for(int i = index; i < baLst.count(); ++i)
            baLst.at(i)->setNumber(index+1);
    if(ba->dir == DIR_J){
        js -= ba->v;
        witchEdited |= ES_PZ_JSUM;
    }
    else{
        ds -= ba->v;
        witchEdited |= ES_PZ_DSUM;
    }
    witchEdited |= ES_PZ_BACTION;

    return ba;
}

//（参数：row ，nums：）
/**
 * @brief PingZheng::moveUp 向上移动会计分录
 * @param row  要移动的会计分录的起始行号
 * @param nums 要移动的行数
 * @return
 */
bool PingZheng::moveUp(int row, int nums)
{
    if(row < 0 || row >= baLst.count()){
        LOG_ERROR(QObject::tr("when move up BusiAction object in PingZheng object(id:%1,pnum:%2) start row index overflow!")
                  .arg(ID).arg(pnum));
        return false;
    }
    if(row-nums < 0){
        LOG_ERROR(QObject::tr("when move up BusiAction object in PingZheng object(id:%1,pnum:%2) end row index overflow!")
                  .arg(ID).arg(pnum));
        return false;
    }
    if(nums < 1){
        LOG_ERROR(QObject::tr("when move up BusiAction object in PingZheng object(id:%1,pnum:%2) rows less than 1!")
                  .arg(ID).arg(pnum));
        return false;
    }

    int i;
    for(i = row; i > row-nums; i--){
        baLst.at(i-1)->setNumber(i+1);
        baLst.swap(i-1,i);
    }
    baLst.at(i)->setNumber(row-nums+1);
    witchEdited |= ES_PZ_BACTION;
    return true;
}

//
/**
 * @brief PingZheng::moveDown 向下移动会计分录
 * @param row   要移动的会计分录的起始行号
 * @param nums  要移动的行数
 * @return
 */
bool PingZheng::moveDown(int row, int nums)
{
    if(row < 0 || row >= baLst.count()){
        LOG_ERROR(QObject::tr("when move down BusiAction object in PingZheng object(id:%1,pnum:%2) start row index overflow!")
                  .arg(ID).arg(pnum));
        return false;
    }
    if(row+nums > baLst.count()+1){
        LOG_ERROR(QObject::tr("when move down BusiAction object in PingZheng object(id:%1,pnum:%2) end row index overflow!")
                  .arg(ID).arg(pnum));
        return false;
    }
    if(nums < 1){
        LOG_ERROR(QObject::tr("when move down BusiAction object in PingZheng object(id:%1,pnum:%2) rows less than 1!")
                  .arg(ID).arg(pnum));
        return false;
    }

    for(int i = row; i < row+nums; ++i){
        baLst.at(i+1)->setNumber(i+1);
        baLst.swap(i,i+1);
    }
    baLst.at(row+nums)->setNumber(row+nums+1);
    return true;
}

//移除会计分录列表中末尾的连续空白会计分录
//void PingZheng::removeTailBlank()
//{
//    bool con = true;
//    int i = baLst.count()-1;
//    while(con && i>-1){
//        if(baLst[i]->state == BusiActionData2::BLANK){
//            baLst.removeAt(i);
//            i--;
//        }
//        else
//            con = false;
//    }
//}

/**
 * @brief PingZheng::hasBusiAction
 * 是否存在同一个BusiAction对象
 * @param ba
 * @return
 */
bool PingZheng::hasBusiAction(BusiAction *ba)
{
    //避免同一个分录对象多次加入到同一个凭证对象内
    foreach(BusiAction* b,baLst){
        if(*b == *ba)
            return true;
//        if(b->getId() == ba->getId()){
//            LOG_ERROR(QObject::tr("The PingZheng object(id:%1,pnum:%2) has same BusiAction object(id:%3)")
//                      .arg(ID).arg(pnum).arg(ba->getId()));
//            return true;
//        }
    }
    return false;
}

/**
 * @brief PingZheng::calSum 求借贷双方的合计值
 */
void PingZheng::calSum()
{
    if(!p)
        return;
    Money* mmt = p->getAccount()->getMasterMt();
    QDate d = QDate::fromString(date,Qt::ISODate);
    int y = d.year(); int m = d.month();
    QHash<Money*,Double> rates;
    p->getAccount()->getRates(y,m,rates);
    Double jv = 0.0, dv = 0.0;
    foreach(BusiAction* ba, baLst){
        if(ba->getDir() == DIR_J){
            if(ba->getMt() == mmt)
                jv += ba->getValue();
            else
                jv += ba->getValue() * rates.value(ba->getMt());
        }
        else{
            if(ba->getMt() == mmt)
                dv += ba->getValue();
            else
                dv += ba->getValue() * rates.value(ba->getMt());
        }
    }
    if(js != jv){
        js = jv;
        witchEdited |= ES_PZ_JSUM;
    }
    if(ds != dv){
        ds = dv;
        witchEdited |= ES_PZ_DSUM;
    }
}

//PingZheng* PingZheng::load(int id,QSqlDatabase db)
//{
//    QSqlQuery q(db);
//    QString s;

//    //读取凭证信息数据
//    s = QString("select * from %1 where id=%2").arg(tbl_pz).arg(id);
//    if(!q.exec(s) || !q.first())
//        return NULL;
//    PzData2 pd;
//    pd.pzId = id;
//    pd.date = q.value(PZ_DATE).toString();
//    pd.pzNum = q.value(PZ_NUMBER).toInt();
//    pd.pzZbNum = q.value(PZ_ZBNUM).toInt();
//    pd.jsum = Double(q.value(PZ_JSUM).toDouble());
//    pd.dsum = Double(q.value(PZ_DSUM).toDouble());
//    pd.pzClass = (PzClass)q.value(PZ_CLS).toInt();
//    pd.attNums = q.value(PZ_ENCNUM).toInt();
//    pd.state = (PzState)q.value(PZ_PZSTATE).toInt();
//    pd.verify = allUsers.value(q.value(PZ_VUSER).toInt());
//    pd.producer = allUsers.value(q.value(PZ_RUSER).toInt());
//    pd.bookKeeper = allUsers.value(q.value(PZ_BUSER).toInt());

//    //读取凭证所处月份的汇率
//    QHash<int,Double> rates;
//    QDate d = QDate::fromString(pd.date,Qt::ISODate);
//    int y = d.year();
//    int m = d.month();
//    if(!Money::getRate(y,m,rates,db))
//        return NULL;

//    //读取凭证所属会计分录
//    s = QString("select * from %1 where %2=%3 order by %4")
//            .arg(tbl_ba).arg(fld_ba_pid).arg(id).arg(fld_ba_number);
//    if(!q.exec(s))
//        return NULL;
//    QList<BusiActionData2*> baLst;
//    BusiActionData2* bd;
//    Double js,ds;
//    int mmt = curAccount->getMasterMt(); //母币代码
//    while(q.next()){
//        bd = new BusiActionData2;
//        bd->summary = q.value(BACTION_SUMMARY).toString();
//        bd->fid = q.value(BACTION_FID).toInt();
//        bd->sid = q.value(BACTION_SID).toInt();
//        bd->mt = q.value(BACTION_MTYPE).toInt();
//        bd->dir = q.value(BACTION_DIR).toInt();
//        if(bd->dir == DIR_J){
//            bd->v = Double(q.value(BACTION_JMONEY).toDouble());
//            if(bd->mt == mmt)
//                js += bd->v;
//            else
//                js += (bd->v * rates.value(bd->mt));
//        }
//        else{
//            bd->v = Double(q.value(BACTION_DMONEY).toDouble());
//            if(bd->mt == mmt)
//                ds += bd->v;
//            else
//                ds += (bd->v * rates.value(bd->mt));
//        }
//        bd->num = q.value(BACTION_NUMINPZ).toInt();
//        baLst<<bd;
//    }
//    //如果统计的借贷合计值与表中读取的值不同，要考虑将其报告给用户，并提供纠错功能
//    if(pd.jsum != js){
//        pd.jsum = js;
//        pd.editState = PZEDITED;
//    }
//    if(pd.dsum != ds){
//        pd.dsum = ds;
//        pd.editState = PZEDITED;
//    }
//    PingZheng* pz = new PingZheng(&pd,curUser,db);
//    pz->setBaList(baLst);
//    return pz;
//}

//PingZheng* create(User* user,QSqlDatabase db)
//{

//}

//PingZheng* create(QString date,int pnum,int znum,double js,double ds,
//                             PzClass pcls,int encnum,PzState state,User* vu,
//                             User* ru, User* bu,User* user,QSqlDatabase db)
//{

//}


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
