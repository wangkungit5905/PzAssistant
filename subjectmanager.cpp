#include <QSqlQuery>
#include <QString>
#include <QVariant>

#include "subjectmanager.h"
#include "tables.h"

SubjectManager::SubjectManager(SubjectSysType subSysType,QSqlDatabase db):
    subType(subSysType),db(db)
{
    init();
}

//返回一级科目名
QString SubjectManager::getFstSubName(int id)
{
    return fstSubs.value(id);
}

QString SubjectManager::getFstSubCode(int id)
{
    return fstCodes.value(id);
}

//返回二级科目名（参数id为FSAgent表的id列）
QString SubjectManager::getSndSubName(int id)
{
    return sndNames[sndIdx.value(id)];
}

//返回二级科目全称
QString SubjectManager::getSndSubLName(int id)
{
    return sndLNames[sndIdx.value(id)];
}

//获取现金主目下的人民币子目（应该表达为现金主目下对应母币的子目）
int SubjectManager::getCashRmbSid()
{
    QSqlQuery q(db);
    QString s;
    //s = "select FSAgent.id from FSAgent join SecSubjects on FSAgent.sid=SecSubjects.id where SecSubjects.subName='人民币'";
    s = QString("select %1.id from %1 join %2 on %1.%3=%2.id where %2.%4='%5'")
            .arg(tbl_fsa).arg(tbl_ssub).arg(fld_fsa_sid).arg(fld_ssub_name)
            .arg(QObject::tr("人民币"));
    if(q.exec(s) && q.first())
        return q.value(0).toInt();
    else
        return 0;
}

bool SubjectManager::init()
{
    QSqlQuery q(db);
    QString s;

    //初始化一级科目类别表
    int c;
    QString name;
    s = QString("select %1,%2 from %3").arg(fld_fsc_code)
            .arg(fld_fsc_name).arg(tbl_fsclass);
    if(!q.exec(s))
        return false;
    while(q.next()){
        c = q.value(0).toInt();
        name = q.value(1).toString();
        fstSubCls[name] = c;
    }

    //初始化一级科目表
    s = QString("select id,%1,%2,%3 from %4").arg(fld_fsub_name)
            .arg(fld_fsub_subcode).arg(fld_fsub_class).arg(tbl_fsub);
    if(!q.exec(s))
        return false;

    int id,cls;
    QString code;
    int syId = fstSubCls.value(QObject::tr("损益类")); //损益类科目类别代码
    while(q.next()){
        id = q.value(0).toInt();
        name = q.value(1).toString();
        code = q.value(2).toString();
        cls = q.value(3).toInt();
        fstSubs[id] = name;
        fstCodes[id] = code;
        if(syId == cls)
            plIds<<id;
        if(code == "1001")
            cashId = id;
        else if(code == "1002")
            bankId = id;
        else if(code == "1131")
            ysId = id;
        else if(code == "2121")
            yfId = id;
        else if(code == "1501")
            gdzcId = id;
        else if(code == "1301")
            dtfyId == id;
        else if(code == "1502")
            ljzjId = id;
        else if(code == "3131")
            bnlrId = id;
        else if(code == "5503")
            cwfyId = id;
        else if(code == "3141")
            lrfpId = id;
    }

    //初始化二级科目表
    s = QString("select id,%1,%2 from %3").arg(fld_ssub_name)
            .arg(fld_ssub_lname).arg(tbl_ssub);
    if(!q.exec(s))
        return false;
    QString lname;
    int idx = 0;
    while(q.next()){
        id = q.value(0).toInt();
        name = q.value(1).toString();
        lname = q.value(2).toString();
        sndNames<<name;
        sndLNames<<lname;
        secIdx[id] = idx++;
    }

    //初始化二级科目映射表
    s = QString("select id,%1,%2 from %3").arg(fld_fsa_fid)
            .arg(fld_fsa_sid).arg(tbl_fsa);
    if(!q.exec(s))
        return false;
    int fid,sid;
    while(q.next()){
        id = q.value(0).toInt();
        fid = q.value(1).toInt();
        sid = q.value(2).toInt();
        //sndSubMap[id] = sid;
        sndIdx[id] = secIdx.value(sid);
        subBelongs[fid]<<id;
    }
}

//获取待摊费用类别id到对应子目id的映射
void SubjectManager::getDtfySubIds(QHash<int,int>& h)
{

}

void SubjectManager::getDtfySubNames()
{

}
