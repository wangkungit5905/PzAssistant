#ifndef SUBJECTMANAGER_H
#define SUBJECTMANAGER_H

#include <QSqlDatabase>
#include <QHash>
#include <QStringList>


//科目管理类-
class SubjectManager
{
public:
    enum SubjectSysType{
        SUBT_OLD = 1,
        SUBT_NEW = 2
    };

    SubjectManager(SubjectSysType subSysType = SUBT_OLD,
                   QSqlDatabase db = QSqlDatabase::database());

    //获取科目名的方法
    QString getFstSubName(int id);
    QString getFstSubCode(int id);
    QString getSndSubName(int id);
    QString getSndSubLName(int id);

    //获取要作特别处理的科目id的方法
    int getCashId(){return cashId;}
    int getBankId(){return bankId;}
    int getYsId(){return ysId;}
    int getYfId(){return yfId;}
    int getGdzcId(){return gdzcId;}
    int getDtfyId(){return dtfyId;}
    int getLjzjId(){return ljzjId;}
    int getBnlrId(){return bnlrId;}
    int getCashRmbSid();

    //待摊费用科目类
    void getDtfySubIds(QHash<int, int> &h);
    void getDtfySubNames();
    bool isSySubs(int id){return plIds.contains(id);} //是否损益类科目
    int getCwfyId(){return cwfyId;}
    int getLrfpId(){return lrfpId;}

private:
    bool init();

    QSqlDatabase db;
    SubjectSysType subType;   //科目系统的类型

    QHash<QString,int> fstSubCls; //一级科目类别表

    QHash<int,QString> fstSubs;  //所有一级科目id到科目名称的映射表
    QHash<int,QString> fstCodes;  //所有一级科目id到科目代码的映射表

    //从SecSubjects表读取
    QStringList sndNames,sndLNames; //所有二级科目名称和全称的列表
    QHash<int,int> secIdx;          //SecSubjects表id列到名称索引位置的映射
    QHash<int,int> sndIdx;          //所有二级科目id到科目名称（或全称）索引（上面两个列表的索引）的映射表
    //所有二级科目id(FSAgent表id列)到二级科目名称id(SecSubjects表id列)的映射
    //QHash<int,int> sndSubMap;
    QHash<int,QList<int> > subBelongs; //所有一级科目属下的二级科目

    QList<int> plIds;                   //损益类主科目id

    //一些特别科目id
    int cashId,bankId,ysId,yfId;  //现金、银行、应收和应付科目id
    int gdzcId,dtfyId,ljzjId,bnlrId;     //固定资产、待摊费用、累计折旧和本年利润科目id
    int cwfyId,lrfpId; //财务费用、利润分配
};

#endif // SUBJECTMANAGER_H
