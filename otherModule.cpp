//#include <QVariant>

//#include "account.h"
#include "common.h"

#include "utils.h"
#include "otherModule.h"

//新建固定资产条目时调用的构造函数
Gdzc::Gdzc()
{
    QSqlQuery q;
    bool r;
    QString s = QString("select max(code) from gdzcs");
    r = q.exec(s);
    r = q.first();
    code = q.value(0).toInt() + 1;
    s = QString("insert into gdzcs(code) values(%1)").arg(code);
    r = q.exec(s);
    s = QString("select id from gdzcs where code=%1").arg(code);
    r = q.exec(s);
    q.first();
    id = q.value(0).toInt();
    primev = 0;
    remainv = 0;
    minv = 0;
}

//读取已有的固定资产条目时调用的构造函数
Gdzc::Gdzc(int id,int code,GdzcType* productClass,int subCls,QString name,QString model,QDate buyDate,
     double prime,double remain,double min,int zjMonths,QString otherInfo):
    id(id),code(code),productClass(productClass),subClass(subCls),name(name),model(model),
    buyDate(buyDate),primev(prime),remainv(remain),minv(min),otherInfo(otherInfo)
{

    //如果固定资产所属类别没有给出折旧时间，则使用参数zjMonths指定的值，否者使用类别指定的值
    this->zjMonths = productClass->getZjMonth();
    if(this->zjMonths == 0)
        this->zjMonths = zjMonths;
    loadZjInfos();
}

void Gdzc::setProductClass(GdzcType* type)
{
    this->productClass = type;
    if(type->getZjMonth() != 0)
        zjMonths = type->getZjMonth();
}

QList<Gdzc::GdzcZjInfoItem*> Gdzc::getZjInfos()
{
    QList<GdzcZjInfoItem*> items;
    foreach(GdzcZjInfoItem* item,zjInfos){
        if(item->state == INIT)
            items<<item;
    }
    return items;
}

//QList<Gdzc::GdzcZjInfoItem*> Gdzc::getAllZjInfos()
//{
//    QList<GdzcZjInfoItem*> items;
//    items = zjInfos + zjInfoDels;
//    return items;
//}

//添加折旧信息条目
bool Gdzc::addZjInfo(QDate d, double v, int pid, int bid)
{
    //对于某个固定资产，在某个月份只能折旧一次，即年月不能相重
    QString ds = d.toString(Qt::ISODate);
    ds.chop(3);
    for(int i = 0; i < zjInfos.count(); ++i){
        QString s = zjInfos[i]->date.toString(Qt::ISODate);
        s.chop(3);
        if(s == ds)
            return false;
    }
    GdzcZjInfoItem* item = new GdzcZjInfoItem(d,v,INIT,pid,bid);
    zjInfos<<item;
    remainv -= v;
    return true;
}

//添加折旧信息条目
bool Gdzc::addZjInfo(GdzcZjInfoItem* item)
{
    QString ds = item->date.toString(Qt::ISODate);
    ds.chop(3);
    for(int i = 0; i < zjInfos.count(); ++i){
        QString s = zjInfos[i]->date.toString(Qt::ISODate);
        s.chop(3);
        if(s == ds)
            return false;
    }
    zjInfos<<item;
    remainv -= item->v;
    return true;
}

//移除指定索引位置的折旧信息条目
bool Gdzc::removeZjInfo(int index)
{
    if(index >= zjInfos.count())
        return false;
    GdzcZjInfoItem* item = zjInfos.takeAt(index);
    item->state = DELETED;
    zjInfoDels<<item;
    remainv += item->v;
    return true;
}

//移除指定年月的折旧信息条目
bool Gdzc::removeZjInfo(QString ds)
{
    bool ok;
    int index;
    for(int i = 0; i < zjInfos.count(); ++i){
        QString s = zjInfos[i]->date.toString(Qt::ISODate);
        if(ds == s){
            ok = true;
            index = i;
            break;
        }
    }
    if(ok){
        GdzcZjInfoItem* item = zjInfos.takeAt(index);
        item->state = DELETED;
        zjInfoDels<<item;
        remainv += item->v;
        return true;
    }
    else
        return false;
}

//设置某个折旧信息条目的日期值
void Gdzc::setZjDate(QString ds, int index)
{
    if(index >= zjInfos.count())
        return;
    zjInfos[index]->date = QDate::fromString(ds,Qt::ISODate);
    if(zjInfos[index]->state == INIT)
        zjInfos[index]->state = CHANGED;
}

//设置某个折旧信息条目的折旧值
void Gdzc::setZjValue(double v, int index)
{
    if(index >= zjInfos.count())
        return;
    zjInfos[index]->v = v;
    if(zjInfos[index]->state == INIT)
        zjInfos[index]->state = CHANGED;
}

//是否折尽
bool Gdzc::isZjComplete()
{
    if(remainv <= minv)
        return false;
    else
        return true;
}


//装载所有此固定资产的折旧信息
void Gdzc::loadZjInfos()
{
    QSqlQuery q;
    QString s;
    bool r;
    s = QString("select * from gdzczjs where gid=%1 order by date").arg(id);
    q.exec(s);
    double sum = 0;  //已折旧值
    while(q.next()){
        GdzcZjInfoItem* item = new GdzcZjInfoItem;
        item->state = INIT;
        item->id = q.value(0).toInt();
        item->date = QDate::fromString(q.value(GZJ_DATE).toString(),Qt::ISODate);
        item->v = q.value(GZJ_PRICE).toDouble();
        item->pid = q.value(GZJ_PID).toInt();
        item->bid = q.value(GZJ_BID).toInt();
        zjInfos<<item;
        sum += item->v;
    }
    remainv = primev - sum;
}

void Gdzc::save()
{
    QSqlQuery q;
    QString s;
    bool r;

    //保存基本信息
    s = QString("update gdzcs set pcls=%1,scls = %2,name='%3',model='%4',buyDate='%5',"
                "prime=%6,remain=%7,min=%8,zjMonths=%9,desc='%10' where id=%11")
            .arg(productClass->getCode()).arg(subClass).arg(name).arg(model)
            .arg(buyDate.toString(Qt::ISODate)).arg(primev).arg(remainv)
            .arg(minv).arg(zjMonths).arg(otherInfo).arg(id);
    r = q.exec(s);

    //保存折旧信息
    foreach(GdzcZjInfoItem* item, zjInfos){
        if(item->state == NEW){
            s = QString("insert into gdzczjs(gid,pid,bid,date,price) values"
                        "(%1,%2,%3,'%4',%5)").arg(id).arg(item->pid).arg(item->bid)
                    .arg(item->date.toString(Qt::ISODate)).arg(item->v);
            r = q.exec(s);
            //回读以获取新记录的id
            s = QString("select id from gdzczjs where (gid=%1) and (date='%2')")
                    .arg(id).arg(item->date.toString(Qt::ISODate));
            r = q.exec(s);
            r = q.first();
            item->id = q.value(0).toInt();
            item->state = INIT;
        }
        else if(item->state == CHANGED){
            s = QString("update gdzczjs set pid=%1,bid=%2,date='%3',"
                        "price=%4 where id=%5").arg(item->pid).arg(item->bid)
                    .arg(item->date.toString(Qt::ISODate)).arg(item->v).arg(item->id);
            r = q.exec(s);
            item->state = INIT;
        }
    }

    //删除被移除的折旧信息
    foreach(GdzcZjInfoItem* item, zjInfoDels){
        if(item->state == NEW){
            delete item;
            continue;
        }
        s = QString("delete from gdzczjs where id=%1").arg(item->id);
        r = q.exec(s);
        delete item;
    }
    zjInfoDels.clear();
}

//从gdzcs表中移除此项固定资产
void Gdzc::remove()
{
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("delete from gdzcs where id = %1").arg(id);
    r = q.exec(s);
    s = QString("delete from gdzczjs where gid=%1").arg(id);
    r = q.exec(s);
    int i = 0;
}

//获取固定资产的科目类别id到名称的映射表（在SecSubjects表的id列）
bool Gdzc::getSubClasses(QHash<int, QString> &names)
{
    QSqlQuery q;
    QString s;

    s = QObject::tr("select clsCode from SndSubClass where name = '固定资产类'");
    if(!q.exec(s) || !q.first())
        return false;
    int code = q.value(0).toInt();

    s = QString("select id,subName from SecSubjects where classId=%1").arg(code);
    if(!q.exec(s))
        return false;
    while(q.next()){
        int id = q.value(0).toInt();
        QString name = q.value(1).toString();
        names[id] = name;
    }
    return true;
}

//获取固定资产类别id到固定资产子目id的映射
bool Gdzc::getSubClsToGs(QHash<int,int>& subIds)
{
    QSqlQuery q;
    QString s;
    int pid;  //固定资产科目id

    if(!BusiUtil::getIdByCode(pid, "1501"))
        return false;
    s = QString("select id,sid from FSAgent where fid = %1").arg(pid);
    if(!q.exec(s))
        return false;
    while(q.next()){
        int id = q.value(0).toInt();
        int sid = q.value(1).toInt();
        subIds[sid] = id;
    }
    return true;
}

//获取固定资产类别id到累计折旧子目id的映射
bool Gdzc::getSubClsToLs(QHash<int,int>& subIds)
{
    QSqlQuery q;
    QString s;
    int lid;  //固定资产科目id

    if(!BusiUtil::getIdByCode(lid, "1502"))
        return false;
    s = QString("select id,sid from FSAgent where fid = %1").arg(lid);
    if(!q.exec(s))
        return false;
    while(q.next()){
        int id = q.value(0).toInt();
        int sid = q.value(1).toInt();
        subIds[sid] = id;
    }
    return true;
}

//从数据库表gdzcs中装载所有固定资产
bool Gdzc::load(QList<Gdzc*>& accLst, bool isAll)
{
    QSqlQuery q;
    QString s;
    bool r;

    //装载所有固定资产
    if(isAll)
        s = "select * from gdzcs";
    else
        s = "select * from gdzcs where remain>min";
    if(!q.exec(s))
        return false;
    while(q.next()){
        int id = q.value(0).toInt();
        int code = q.value(GDZC_CODE).toInt();
        int gc = q.value(GDZC_PCLS).toInt();
        int sc = q.value(GDZC_SCLS).toInt();
        QString name = q.value(GDZC_NAME).toString();
        QString model = q.value(GDZC_MODEL).toString();
        QDate buyDate = QDate::fromString(q.value(GDZC_BUYDATE).toString(),Qt::ISODate);
        double primev = q.value(GDZC_PRIMEV).toDouble();
        double remainv = q.value(GDZC_REMAINV).toDouble();
        double minv = q.value(GDZC_MINV).toDouble();
        int zjMonths = q.value(GDZC_ZJM).toInt();
        QString desc = q.value(GDZC_DESC).toString();
        Gdzc* g = new Gdzc(id,code,allGdzcProductCls.value(gc),sc,name,model,buyDate,
                           primev,remainv,minv,zjMonths,desc);
        accLst<<g;
    }
    return true;
}

//移除指定年月的折旧信息（当gid=0时，表示不限于某个指定固定资产）
bool Gdzc::removeZjInfos(int y, int m, int gid)
{
    QString s;
    QSqlQuery q,q2;
    bool r = false;

    QDate d(y,m,1);
    d.setDate(y,m,d.daysInMonth());
    QString ds = d.toString(Qt::ISODate);
    if(gid == 0)
        s = QString("select * from gdzczjs where date like '%1%'")
                .arg(ds);
    else
        s = QString("select * from gdzczjs where (date like '%1%') "
                    "and (gid = %2)").arg(ds).arg(gid);
    r = q.exec(s);
    while(q.next()){
        int id = q.value(0).toInt();
        int gid = q.value(GZJ_GID).toInt();
        int pid = q.value(GZJ_PID).toInt();
        int bid = q.value(GZJ_BID).toInt();
        double v = q.value(GZJ_PRICE).toDouble();
        //if(pid != 0) //调整折旧凭证的借贷合计值和对应会计分录的金额
        //    adjustZjValue(pid,bid,v,false);
        s = QString("update gdzcs set remain=remain+%1 where gid=%2")
                .arg(v).arg(gid);
        r = q2.exec(s);
        s = QString("delete from gdzczjs where id = %1").arg(id);
        r = q2.exec(s);
    }
    return r;
}

//添加指定年月的折旧信息
bool Gdzc::createZjPz(int y,int m,User* user)
{
    QString s;
    QSqlQuery q,q2;
    bool r = false;

    QHash<int, int> subIds;  //固定资产科目类别id到累计折旧子目的映射表
    QHash<int,double> sums;  //按固定资产的科目类别汇总后的折旧值表（键为固定资产的科目类别）
    double sum;              //借或贷方的合计值
    QHash<int,int> bids;     //计提折旧的会计分录id（键为固定资产的科目类别）
    QHash<int,double> zjvs;  //每个固定资产的折旧值（键为固定资产的id）
    QHash<int,int> gidTosid; //固定资产id到固定资产科目类别id的映射表
    int pzNum = 0;           //固定资产折旧凭证的凭证号
    int pid = 0;             //固定资产折旧凭证的id


    QDate d(y,m,1);
    d.setDate(y,m,d.daysInMonth()); //折旧凭证的日期
    QString ds = d.toString(Qt::ISODate);

    //查找是否已有相关凭证
    s = QString("select id,number from PingZhengs where (date='%1') and (isForward=%2)")
            .arg(ds).arg(Pzc_GdzcZj);
    if(!q.exec(s))
        return false;

    if(q.first()){
        if(QMessageBox::Yes == QMessageBox::warning(0,QObject::tr("询问信息"),
            QObject::tr("%1年%2月的固定资产折旧凭证已存在，需要重新创建吗？").arg(y).arg(m),
            QMessageBox::Yes|QMessageBox::No)){
            //删除先前创建的折旧凭证所属的会计分录，凭证记录要重用
            pid = q.value(0).toInt();
            pzNum = q.value(1).toInt();
            s = QString("delete from BusiActions where pid=%1").arg(pid);
            r = q.exec(s);
            //删除gzzczjs表中的折旧记录
            //根据折旧记录重新计算gdzc表中的净值
            Gdzc::removeZjInfos(y,m);
        }
        else
            return true;
    }
    else{ //凭证记录不存在，则需新建
        //首先获取可用的最大凭证号
        QString sd(ds);
        sd.chop(3);
        s = QString("select max(number) from PingZhengs where (pzState != %1) "
                    "and (date like '%2%')").arg(Pzs_Repeal).arg(sd);
        r = q.exec(s); r = q.first();
        pzNum = q.value(0).toInt()+1;
        //创建折旧凭证对应的记录
        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
                    "values('%1',%2,%3,%4,%5)").arg(ds).arg(pzNum).arg(Pzc_GdzcZj)
                .arg(Pzs_Recording).arg(user->getUserId());
        r = q.exec(s);
        //回读凭证id
        s = QString("select id from PingZhengs where (date='%1') and (isForward=%2)")
                .arg(ds).arg(Pzc_GdzcZj);
        r = q.exec(s); r = q.first();
        pid = q.value(0).toInt();
    }


    //获取所有需要折旧的固定资产的折旧值，并根据其科目类别分别进行汇总
    s = QString("select * from gdzcs where remain>min");
    r = q.exec(s);
    while(q.next()){
        int gid = q.value(0).toInt();
        int sid = q.value(GDZC_SCLS).toInt();
        gidTosid[gid] = sid;
        double prime = q.value(GDZC_PRIMEV).toDouble();
        int zjMonths = q.value(GDZC_ZJM).toInt();
        double zjv = prime / zjMonths;
        zjvs[gid] = zjv;
        //int subClass = q.value(GDZC_SCLS).toInt();
        sums[sid] += zjv;
        sum += zjv;
    }

    //创建会计分录
    int lid; //累计折旧科目id
    int num = 0;
    BusiUtil::getIdByCode(lid,"1502");
    Gdzc::getSubClsToLs(subIds);
    QList<int> subs = sums.keys();
    qSort(subs.begin(),subs.end());
    for(int i = 0; i < subs.count(); ++i){
        num++;
        s = QString("insert into BusiActions(pid,summary,firSubID,secSubID,"
                    "moneyType,jMoney,dMoney,dir,NumInPz) values(%1,'%2',%3,"
                    "%4,%5,%6,%7,%8,%9)").arg(pid).arg(QObject::tr("计提折旧"))
                .arg(lid).arg(subIds.value(subs[i])).arg(RMB)
                .arg(0).arg(sums.value(subs[i])).arg(DIR_D).arg(num);
        r = q.exec(s);
        //回读bid
        s = QString("select id from BusiActions where (pid=%1) and (NumInPz=%2)")
                .arg(pid).arg(num);
        r = q.exec(s);
        r = q.first();
        bids[subs[i]] = q.value(0).toInt();
    }

    //在PingZhengs表中更新借贷合计值
    s = QString("update PingZhengs set jsum=%1,dsum=%1 where id=%2")
            .arg(sum).arg(pid);
    r = q.exec(s);

    //在gdzczjs表中添加记录
    QHashIterator<int,double> it(zjvs);
    while(it.hasNext()){
        it.next();
        s = QString("insert into gdzczjs(gid,pid,bid,date,price) values("
                    "%1,%2,%3,'%4',%5)").arg(it.key()).arg(pid)
                .arg(bids.value(gidTosid.value(it.key())))
                .arg(ds).arg(it.value());
        r = q.exec(s);
    }

    //更新gdzcs表
    it.toFront();
    while(it.hasNext()){
        it.next();
        s = QString("update gdzcs set remain=remain+%1 where id=%2")
                .arg(it.value()).arg(it.key());
        r = q.exec(s);
        int i = 0;
    }

    return r;
}


//撤销计提折旧凭证
bool Gdzc::repealZjPz(int y,int m)
{
    //删除凭证表中的折旧凭证及其会计分录
    //删除gdzczjs表中对应年月的折旧记录
    //更新gdzcs表的固定资产剩余值
    QSqlQuery q,q1;
    QString s;

    QDate d(y,m,1);
    QString ds = d.toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select gid,price from gdzczjs where date like '%1%'").arg(ds);
    if(!q.exec(s))
        return false;
    while(q.next()){
        int gid = q.value(0).toInt();
        double v = q.value(1).toDouble();
        s = QString("update gdzcs set remain=remain+%1 where id=%2")
                .arg(v).arg(gid);
        if(!q1.exec(s))
            return false;
    }
    s = QString("delete from gdzczjs where date like '%1%'").arg(ds);
    if(!q.exec(s))
        return false;
    s = QString("select id from PingZhengs where (isForward=%1) and "
                "(date like '%2%')").arg(Pzc_GdzcZj).arg(ds);
    if(!q.exec(s) || !q.first())
        return false;
    int pid = q.value(0).toInt();
    s = QString("delete from BusiActions where pid=%1").arg(pid);
    if(!q.exec(s))
        return false;
    s = QString("delete from PingZhengs where id=%1").arg(pid);
    if(!q.exec(s))
        return false;
    return true;
}

//在添加或删除折旧信息条目时，调用此函数以调整折旧凭证的借贷合计值和对应会计分录的金额
//bool Gdzc::adjustZjValue(int pid,int bid, double v, bool isAdd)
//{
//    QSqlQuery q;
//    QString s;
//    QChar sign;
//    bool r;

//    if(isAdd)
//        sign = '+';
//    else
//        sign = '-';

//    s = QString("update PingZhengs set jsum=jsum%3%1,dsum=dsum%3%1 where id=%2")
//            .arg(v).arg(pid).arg(sign);
//    r = q.exec(s);
//    s = QString("update BusiActions set dMoney=dMoney%3%1 where id=%2")
//            .arg(v).arg(bid).arg(sign);
//    r = q.exec(s);
//}

//////////////////////////////Dtfy class/////////////////////////////////////
//装载所有待摊费用类别
//1、首先试图从二级科目类别表中读取代表待摊费用类的记录，如果没有，则创建该记录
//2、试图从二级科目表中读取属于待摊费用类的条目，如果没有，则创建3个默认的记录
//  （办公室租金、网络维护费和办公室装修）
//3、在FSAgent表中创建映射条目

bool DtfyType::load(QHash<int,DtfyType*>& l, QSqlDatabase db)
{
    QSqlQuery q(db);
    QString s;
    //从二级科目类别表中获取待摊费用类的类别id（clsCode）
    int cid;
    s = QString("select clsCode from SndSubClass where name='%1'")
            .arg(QObject::tr("待摊费用类"));
    if(!q.exec(s))
        return false;
    if(q.first())
        cid = q.value(0).toInt();
    else{
        s = "select max(clsCode) from SndSubClass";
        if(!q.exec(s))
            return false;
        q.first();
        cid = q.value(0).toInt()+1;
        s = QString("insert into SndSubClass(clsCode,name,explain) values(%1,'%2','%3')")
                .arg(cid).arg(QObject::tr("待摊费用类"))
                .arg(QObject::tr("在摊销待摊费用时作为待摊费用的子目"));
        if(!q.exec(s))
            return false;
    }

    //从二级科目表中获取属于待摊费用类的记录
    s = QString("select id,subName from SecSubjects where classId=%1").arg(cid);
    if(!q.exec(s))
        return false;
    while(q.next()){
        int id = q.value(0).toInt();
        QString name = q.value(1).toString();
        DtfyType* t = new DtfyType(id,name);
        l[id] = t;
    }
    if(l.empty()){
        //获取待摊费用科目的id
        int subd;
        BusiUtil::getIdByCode(subd,"1301");
        QStringList subName;
        QStringList remCode;
        subName<<QObject::tr("办公室租金")
               <<QObject::tr("网络维护费")
               <<QObject::tr("办公室装修");
        remCode<<"bgszj"<<"wlwhf"<<"bgszx";
        for(int i = 0; i < subName.count(); ++i){
            s = QString("insert into SecSubjects(subName,subLName,remCode,classId) "
                        "values('%1','%2','%3',%4)").arg(subName[i])
                    .arg(subName[i]).arg(remCode[i]).arg(cid);
            bool r = q.exec(s);
            s = QString("select id from SecSubjects where (subName='%1') and "
                        "(classId=%2)").arg(subName[i]).arg(cid);
            r = q.exec(s);
            r = q.first();
            int id = q.value(0).toInt();
            DtfyType* t = new DtfyType(id,subName[i]);
            l[id] = t;
            //在FSAgent表中建立映射条目
            s = QString("insert into FSAgent(fid,sid) values(%1,%2)")
                    .arg(subd).arg(id);
            r = q.exec(s);
        }
    }
    return true;
}

//获取待摊费用科目id(fid)及其下的子目(sids,键为待摊费用类别，值为子目id（FSAgent表id列）)
bool Dtfy::getDtfySubId(int &fid,QHash<int,int>& sids, QSqlDatabase db)
{
    if(!BusiUtil::getIdByCode(fid,"1301"))
        return false;
    QHash<int,DtfyType*> dts;
    DtfyType::load(dts);
    QSqlQuery q(db);
    QString s;
    foreach(DtfyType* dt,dts){
        int sid = dt->getCode(); //SecSubjects表的id列
        s = QString("select id from FSAgent where (fid=%1) and (sid=%2)")
                .arg(fid).arg(sid);
        if(!q.exec(s))
            return false;
        if(!q.first())
            continue;
        int id = q.value(0).toInt();
        sids[sid] = id;
    }
}

bool Dtfy::load(QList<Dtfy*>& dl, bool all, QSqlDatabase db)
{
    QSqlQuery q(db),q1(db);
    QString s;

    if(all)
        s = "select * from dtfys";
    else
        s = "select * from dtfys where remain>0";
    if(!q.exec(s))
        return false;

    Dtfy* item;
    QString name,impDate,startDate,explain;
    double total,remain;
    int id,code,tid,months;
    int dfid,dsid;
    int pid,jbid,dbid;
    QList<FtItem*> ftlst;
    FtItem* fi;
    QHash<int,DtfyType*> dts;
    DtfyType::load(dts,db);
    while(q.next()){
        id = q.value(0).toInt();
        code = q.value(DTFY_CODE).toInt();
        tid = q.value(DTFY_TYPE).toInt();
        name = q.value(DTFY_NAME).toString();
        impDate = q.value(DTFY_IMPDATE).toString();
        total = q.value(DTFY_TOTAL).toDouble();
        remain = q.value(DTFY_REMAIN).toDouble();
        months = q.value(DTFY_MONTHS).toInt();
        startDate = q.value(DTFY_START).toString();
        dfid = q.value(DTFY_DFID).toInt();
        dsid = q.value(DTFY_DSID).toInt();
        pid = q.value(DTFY_PID).toInt();
        jbid = q.value(DTFY_JBID).toInt();
        dbid = q.value(DTFY_DBID).toInt();
        explain = q.value(DTFY_EXPLAIN).toString();
        s = QString("select * from ftqks where did=%1").arg(id);
        if(!q1.exec(s))
            return false;
        while(q1.next()){
            fi = new FtItem;
            fi->id = q1.value(0).toInt();
            fi->pid = q1.value(FT_PID).toInt();
            fi->bid = q1.value(FT_BID).toInt();
            fi->state = INIT;
            fi->date = q1.value(FT_DATE).toString();
            fi->v = q1.value(FT_VALUE).toDouble();
            ftlst<<fi;
        }
        item = new Dtfy(id,code,dts.value(tid),name,impDate,total,remain,months,
                        startDate,dfid,dsid,pid,jbid,dbid,explain,db);
        item->setFtItems(ftlst);
        ftlst.clear();
        dl<<item;
    }
    return true;
}

//创建引入待摊费用的凭证
bool Dtfy::genImpPzData(int y,int m,QList<PzData*> pzds, User* user, QSqlDatabase db)
{
    QSqlQuery q(db);
    QString s;
    int pid;

    //
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select * from dtfys where (impDate like '%1%') and "
                "(total=remain)").arg(ds);
    if(!q.exec(s))
        return false;

    //获取待摊费用类别到对应子目的id的映射
    SubjectManager* sm = curAccount->getSubjectManager();
    QHash<int,int> subMaps;
    sm->getDtfySubIds(subMaps);

    double sum = 0; //凭证的借贷方合计值
    int tid,dfid,dsid;      //待摊费用类别、贷方主子科目id
    QString name,impDate;   //名称、引入日期、
    double tv;      //总值
    int dnums = 0;  //引入的待摊费用数目
    while(q.next() && dnums < 5){
        dnums++;
        tid = q.value(DTFY_TYPE).toInt();
        name = q.value(DTFY_NAME).toString();
        dfid = q.value(DTFY_DFID).toInt();
        dsid = q.value(DTFY_DSID).toInt();
        impDate = q.value(DTFY_IMPDATE).toString();
        tv = q.value(DTFY_TOTAL).toDouble();

    }

}

//创建摊销凭证
bool Dtfy::createTxPz(int y,int m,User *user, QSqlDatabase db)
{

}

//删除摊销凭证
bool Dtfy::repealTxPz(int y,int m)
{

}

//获取指定年月的摊销清单列表
bool Dtfy::getFtManifest(int y,int m,QList<TxmItem*>& l, QSqlDatabase db)
{
    QSqlQuery q(db);
    QString s;
    QDate d(y,m,1);
    QString ds = d.toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select dtfys.name,dtfys.total,dtfys.total-dtfys.remain,"
                "dtfys.remain,ftqks.value from ftqks join dtfys on "
                "ftqks.did=dtfys.id where ftqks.date like '%1%'").arg(ds);
    if(!q.exec(s))
        return false;
    TxmItem* item;
    while(q.next()){
        item = new TxmItem;
        item->name = q.value(0).toString();
        item->total = q.value(1).toDouble();
        item->txSum = q.value(2).toDouble();
        item->remain = q.value(3).toDouble();
        item->curMonth = q.value(4).toDouble();
        l<<item;
    }
    return true;
}

//生成引入待摊费用的凭证的会计分录(参数jfid,jsid分别是引入待摊费用时贷方的科目id)
bool Dtfy::genDtfyPzDatas(PzData *data,QSqlDatabase db)
{
//    QSqlQuery q(db);
//    QString s;

//    //读取所有在指定年月引入的，但从未摊销过的待摊费用
//    QString ds = data->date;
//    ds.chop(3);
//    s = QString("select * from dtfys where (impDate like '%1%') and "
//                "(total=remain)").arg(ds);
//    if(!q.exec(s))
//        return false;
//    QList<Dtfy*> dtfys;
//    Dtfy* item;
//    QString name,impDate,startDate,explain;
//    double total,remain;
//    int id,code,tid,months;
//    int dfid,dsid;
//    int pid,jbid,dbid;
//    QHash<int,DtfyType*> dts;
//    DtfyType::load(dts);
//    while(q.next()){
//        id = q.value(0).toInt();
//        code = q.value(DTFY_CODE).toInt();
//        tid = q.value(DTFY_TYPE).toInt();
//        name = q.value(DTFY_NAME).toString();
//        impDate = q.value(DTFY_IMPDATE).toString();
//        total = q.value(DTFY_TOTAL).toDouble();
//        remain = q.value(DTFY_REMAIN).toDouble();
//        months = q.value(DTFY_MONTHS).toInt();
//        startDate = q.value(DTFY_START).toString();
//        dfid = q.value(DTFY_DFID).toInt();
//        dsid = q.value(DTFY_DSID).toInt();
//        pid = q.value(DTFY_PID).toInt();
//        jbid = q.value(DTFY_JBID).toInt();
//        dbid = q.value(DTFY_DBID).toInt();
//        explain = q.value(DTFY_EXPLAIN).toString();
//        item = new Dtfy(id,code,dts.value(tid),name,impDate,total,remain,months,
//                        startDate,dfid,dsid,pid,jbid,dbid,explain,db);
//        dtfys<<item;
//    }
//    if(dtfys.empty()){
//        QMessageBox::information(0,QObject::tr("提示信息"),
//                                 QObject::tr("所有待摊费用都已摊销完毕！"));
//        return false;
//    }
//    BaItemData* ba;
//    int fid;
//    QHash<int,int> sids;
//    getDtfySubId(fid,sids);
//    foreach(Dtfy* d,dtfys){
//        ba = new BaItemData;
//        //借方
//        ba->summary = d->name;
//        ba->fid = fid;
//        ba->sid = sids.value(d->getType()->getCode());
//        ba->mt = curAccount->getMasterMt();
//        ba->dir = DIR_J;
//        ba->v = d->total;
//        l<<ba;
//        //贷方
//        ba = new BaItemData;
//        ba->summary = d->name;
//        ba->fid = d->dfid;
//        ba->sid = d->dsid;
//        ba->mt = curAccount->getMasterMt();
//        ba->dir = DIR_D;
//        ba->v = d->total;
//        l<<ba;
//    }
//    return true;
}

//生成指定年月计提摊销凭证的会计分录列表
bool Dtfy::genPzBaDatas(int y,int m,QList<BaItemData*>& l,QSqlDatabase db )
{
    QSqlQuery q(db);
    QString s;

    QList<Dtfy*> dts;
    Dtfy::load(dts,false,db);
    if(dts.empty()){
        QMessageBox::information(0,QObject::tr("提示信息"),
                                 QObject::tr("所有待摊费用都已摊销完毕！"));
        return false;
    }
    //获取待摊费用及其下的子目id

    //获取管理费用-摊销的科目id
    foreach(Dtfy* d, dts){

    }
}

//保存分摊项（新建的插入，被改变的更新，被删除的移除）
void Dtfy::saveFtItems()
{
    QSqlQuery q(db);
    QString s;
    bool r;

    if(id == 0) //对于新建对象，数据库内还没有对应的记录，必须执行插入操作并获取id
        crtNewCode();

    for(int i = 0; i < txs.count(); ++i){
        if(txs[i]->state == CHANGED){
            s = QString("update ftqks set date='%1',value=%2,pid=%3,bid=%4 where id=%5")
                    .arg(txs[i]->date).arg(txs[i]->v).arg(txs[i]->pid)
                    .arg(txs[i]->bid).arg(txs[i]->id);
            r = q.exec(s);
            txs[i]->state = INIT;
        }
        else if(txs[i]->state == NEW){
            s = QString("insert into ftqks(did,date,value,pid,bid) values(%1,'%2',%3,%4,%5)")
                    .arg(id).arg(txs[i]->date).arg(txs[i]->v).arg(txs[i]->pid).arg(txs[i]->bid);
            r = q.exec(s);
            s = QString("select id from ftqks where (did=%1) and date='%2'")
                    .arg(id).arg(txs[i]->date);
            r = q.exec(s);
            r = q.first();
            txs[i]->id = q.value(0).toInt();
            txs[i]->state = INIT;
        }
    }

    for(int i = dels.count()-1; i >= 0; i--){
        if(dels[i]->id != 0){
            s = QString("delete from ftqks where id=%1").arg(dels[i]->id);
            r = q.exec(s);
        }
        delete dels[i];
    }
    dels.clear();
}

//添加摊销项目
bool Dtfy::insertTxInfo(QDate date, double v, int idx, int pid, int bid)
{
    if(idx > txs.count()+1)
        return false;
    QString ds = date.toString(Qt::ISODate);
    ds.chop(3);
    foreach(FtItem* item, txs){
        QString d = item->date;
        d.chop(3);
        if(d == ds)
            return false;
    }

    FtItem* item = new FtItem;
    item->date = date.toString(Qt::ISODate);
    item->v = v;
    item->pid = pid;
    item->bid = bid;
    item->state = NEW;
    txs.insert(idx,item);
    remain -=v;
    return true;
}

//移除指定位置的摊销项
bool  Dtfy::removeTxInfo(int idx)
{
    if(idx >= txs.count())
        return false;
    remain += txs[idx]->v;
    dels<<txs.takeAt(idx);
    return true;
}

//设置指定摊销项目的日期
void Dtfy::setTxDate(QString date, int index)
{
    if(index >= txs.count())
        return;
    txs[index]->date = date;
}

//设置指定摊销项目的值
void Dtfy::setTxValue(double v, int index)
{
    if(index >= txs.count())
        return;
    txs[index]->v = v;
}

bool Dtfy::save()
{
    if(id == 0)
        crtNewCode();
    saveFtItems();
    return update();
}

bool Dtfy::update()
{
    QSqlQuery q(db);
    if(id == 0)
        return false;
    QString s = QString("update dtfys set code=%1,tid=%2,name='%3',impDate='%4',"
                        "total=%5,remain=%6,months=%7,startDate='%8',dfid=%9,"
                        "dsid=%10,pid=%11,jbid=%12,dbid=%13,explain='%14' where id=%15")
            .arg(code).arg(t->getCode()).arg(name).arg(iDate).arg(total).arg(remain)
            .arg(months).arg(sDate).arg(dfid).arg(dsid).arg(pid).arg(jbid)
            .arg(dbid).arg(explain).arg(id);
    if(!q.exec(s))
        return false;
    return true;
}

//从数据库中移除该对象对应的记录
bool Dtfy::remove()
{
    QSqlQuery q(db);
    QString s;

    if(id == 0) //新对象，在数据库中没有对应记录
        return true;
    s = QString("delete from ftqks where did=%1").arg(id);
    if(!q.exec(s))
        return false;
    s = QString("delete from dtfys where id=%1").arg(id);
    if(!q.exec(s))
        return false;
    return true;
}

//为新待摊费用对象分配一个新的代码，在dtfys表新建记录，并获取新记录的id
void Dtfy::crtNewCode()
{
    QSqlQuery q(db);
    QString s;
    bool r = q.exec("select max(code) from dtfys");
    r=q.first();
    code = q.value(0).toInt() + 1;
    s = QString("insert into dtfys(code) values(%1)").arg(code);
    r = q.exec(s);
    s = QString("select id from dtfys where code=%1").arg(code);
    r = q.exec(s);
    r = q.first();
    id = q.value(0).toInt();
}
