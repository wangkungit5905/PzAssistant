#ifndef UTILS_H
#define UTILS_H

#include <QStack>
#include <QDebug>
#include <QSqlQuery>
#include <QVariant>
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QDate>

#include "cal.h"
#include "common.h"
#include "commdatastruct.h"
#include "dialog2.h"
#include "appmodel.h"
#include "global.h"


#define MaxOp 7

typedef struct{
    char ch;   //运算符
    int pri;   //优先级
}OpratorPri;


class ExpressParse{

public:
    ExpressParse();
    void trans(char *exp);
    double trans(QStack<QString> exp);
    double calValue(QString exp, QHash<QString, double>* vhash);

    QHash<QString, double>* vhash; //保存表达式中标量的对应值

private:
    int leftpri(char op);
    int rightpri(char op);
    int InOp(char ch);
    bool InOp(QString str);
    int Precede(char op1, char op2);
    //int Precede(QString op1, QString op2);
    void compValue(QStack<char> &op, QStack<double> &st);
    double replaceValue(QString op);

    OpratorPri lpri[7];
    OpratorPri rpri[7];



};


class CompositeKeyType
{
public:
    CompositeKeyType() {}
    CompositeKeyType(int id, int code){subId=id;mtCode=code;}
    int getSubId(){return subId;}
    int getMtCode(){return mtCode;}

private:
    int subId;
    int mtCode;
};

inline bool operator==(CompositeKeyType &e1, CompositeKeyType &e2)
{
    return (e1.getSubId() == e2.getSubId())
    && (e1.getMtCode() == e2.getMtCode());

}

inline uint qHash(CompositeKeyType &key)
{
    return qHash(key.getSubId() * MAXMT + key.getMtCode());

}

//////////////////////////////////////////////////////////////////////////




//与应用的业务逻辑有关的功能类
class BusiUtil{

public:
    static QSet<int> pset; //具有正借负贷特性的一级科目id集合
    static QSet<int> nset; //具有负借正贷特性的一级科目id集合
    static QSet<int> spset; //具有正借负贷特性的二级科目id集合
    static QSet<int> snset; //具有负借正贷特性的二级科目id集合

    static QSet<int> impPzCls; //由其他模块引入的凭证类别代码集合
    static QSet<int> jzhdPzCls;  //由系统自动产生的结转汇兑损益凭证类别代码集合
    static QSet<int> jzsyPzCls;  //由系统自动产生的结转损益凭证类别代码集合
    static QSet<int> otherPzCls; //其他由系统产生并允许用户修改的凭证类别代码集合

    static QSet<int> accToMt;    //需要按币种核算的科目id集合（这是临时代码，这个信息需要在一级科目表中反映）

    static QSet<int> inIds;  //损益类科目中的收入类科目id集合
    static QSet<int> feiIds; //损益类科目中的费用类科目id集合
    static QSet<int> inSIds;  //损益类科目中的收入类子目id集合
    static QSet<int> feiSIds; //损益类科目中的费用类子目id集合

    //类的初始化函数
    static void init();

    /**
        获取指定凭证的所有业务活动
    */
    static bool getActionsInPz(int pid, QList<BusiActionData*>& busiActions);
    static bool getActionsInPz2(int pid, QList<BusiActionData2*>& busiActions);

    /**
        保存指定凭证下的业务活动
    */
    static bool saveActionsInPz(int pid, QList<BusiActionData*>& busiActions,
                                QList<BusiActionData*> dels = QList<BusiActionData*>());

    static bool saveActionsInPz2(int pid, QList<BusiActionData2*>& busiActions,
                                QList<BusiActionData2*> dels = QList<BusiActionData2*>());

    /**
        判断科目余额的借贷方向，1：借方，0：平，-1：贷方，-2：无定义（可能对于此科目类型还未作处理）
    */
    static int getDirSubExa(double v, int pid)
    {


        if(v == 0)
            return 0;
        if(v > 0){
            if(pset.contains(pid))
                return 1;
            else if(nset.contains(pid))
                return -1;
            else
                return -2;
        }
        else{
            if(pset.contains(pid))
                return -1;
            else if(nset.contains(pid))
                return 1;
            else
                return -2;
        }
    }

    /**
        判断明细科目余额的借贷方向，1：借方，0：平，-1：贷方
    */
    static int getDirSubDetExa(double v, int sid)
    {
        if(v == 0)
            return 0;
        if(v > 0){
            if(spset.contains(sid))
                return 1;
            else if(snset.contains(sid))
                return -1;
            else
                return -2;
        }
        else{
            if(spset.contains(sid))
                return -1;
            else if(snset.contains(sid))
                return 1;
            else
                return -2;
        }
    }

    /**
        判断总账科目余额的借贷方向---文本版
    */
    static QString getSubExaDir(double v, int pid)
    {
        int dir = getDirSubExa(v,pid);
        if(dir == 1)
            return QString(QObject::tr("借"));
        else if(dir == -1)
            return QString(QObject::tr("贷"));
        else if(dir == 0)
            return QString(QObject::tr("平"));
        else
            return QString(QObject::tr("？"));
    }

    /**
        判断明细科目余额的借贷方向---文本版
    */
    static QString getSubDetExaDir(double v, int sid)
    {
        int dir = getDirSubExa(v,sid);
        if(dir == 1)
            return QString(QObject::tr("借"));
        else if(dir == -1)
            return QString(QObject::tr("贷"));
        else if(dir == 0)
            return QString(QObject::tr("平"));
        else
            return QString(QObject::tr("？"));
    }

    /**
        获取所有用户名
    */
    static void getAllUser(QHash<int,QString>& users)
    {
        //
    }

    /**
        获取公司名称
    */
    static bool getCompanyName(QString& sname, QString& lname)
    {
        QSqlQuery q;
        q.exec("select sname,lname from AccountInfos");
        if(!q.first()){
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("不能账户名称信息")));
            return false;
        }
        sname = q.value(0).toString();
        lname = q.value(1).toString();
        return true;
    }

    /**
        获取凭证类别哈希表
    */
    static bool getPzClass(QHash<int,QString>& pzCls)
    {
        QSqlQuery q;
        QString s;

        s = "select code,name from PzClasses";
        if(q.exec(s) && q.first()){
            q.seek(-1);
            while(q.next())
                pzCls[q.value(0).toInt()] = q.value(1).toString();
            return true;
        }
        else{
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取凭证类别")));
            return false;
        }
    }

    /**
        获取凭证类别简称哈希表
    */
    static bool getPzSClass(QHash<int,QString>& pzCls)
    {
        QSqlQuery q;
        QString s;

        s = "select code,sname from PzClasses";
        if(q.exec(s) && q.first()){
            q.seek(-1);
            while(q.next())
                pzCls[q.value(0).toInt()] = q.value(1).toString();
            return true;
        }
        else{
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取凭证类别简称")));
            return false;
        }
    }


    /**
        获取凭证分册类型哈希表
    */
    static bool getPzBgClass(QHash<int,QString>& bgCls)
    {
        QSqlQuery q;
        QString s;
        bool r;

        s = "select code,name from AccountBookGroups";
        if(q.exec(s) && q.first()){
            q.seek(-1);
            while(q.next())
                bgCls[q.value(0).toInt()] = q.value(1).toString();
            return true;
        }
        else{
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取凭证分册类型")));
            return false;
        }
    }

    /**
        读取指定年月的汇率
    */
    static bool getRates(int y,int m, QHash<int,double>& rates);

    static bool getRates2(int y,int m, QHash<int,Double>& rates);

    /**
        保存指定年月的汇率
    */
    static bool saveRates(int y,int m, QHash<int,double>& rates);
    static bool saveRates2(int y,int m, QHash<int,Double>& rates);


    /**
        获取一级科目类别代码
    */
    static bool getFstSubCls(QHash<int,QString>& clsNames)
    {
        QSqlQuery q;
        QString s = QString("select code,name from FstSubClasses");
        if(!(q.exec(s) && q.first())){
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("未能一级科目类别")));
            return false;
        }
        q.seek(-1);
        while(q.next())
            clsNames[q.value(0).toInt()] = q.value(1).toString();
    }


    /**
        获取指定的一级科目类别代码
    */
    static bool getSubClsCode(int& code, QString name)
    {
        QSqlQuery q;
        QString s;

        s = QString("select code from FstSubClasses where name = '%1'")
                .arg(name);
        if(!q.exec() || !q.first()){
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("未能找到%1的类别代码")).arg(name));
            return false;
        }
        code = q.value(0).toInt();
        return true;
    }

    /**
        获取币种代码表
    */
    static bool getMTName(QHash<int,QString>& names){
        QSqlQuery q;
        QString s;
        bool r;

        s = "select code,name from MoneyTypes";
        r = q.exec(s);
        while(q.next()){
            names[q.value(0).toInt()] = q.value(1).toString();
        }
        if(r)
            return true;
        else
            return false;
    }

    /**
        获取指定币种的代码
    */
    static bool getMtCode(int& code, QString name)
    {
        QSqlQuery q;
        QString s;

        s = QString("select code from MoneyTypes where name = '%1'").arg(name);
        if(q.exec(s) && q.first()){
            code = q.value(0).toInt();
            return true;
        }
        else{
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("未能找到%1的代码")).arg(name));
            return false;
        }
    }

    /**
        按科目名称获取科目代码（一级科目）
    */
    static bool getSubCodeByName(QString& code, QString name){
        QSqlQuery q;
        QString s;

        s = QString("select subCode from FirSubjects where subName = '%1'")
                .arg(name);
        if(!q.exec(s) || !q.first()){
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("未能找到科目%1")).arg(name));
            return false;
        }
        code = q.value(0).toString();
        return true;
    }

    /**
        按科目代码获取该科目在一级科目表中的id值
    */
    static bool getIdByCode(int& id, QString code){
        QSqlQuery q;
        QString s;

        s = QString("select id from FirSubjects where subCode = '%1'")
                .arg(code);
        if(!q.exec(s) || !q.first()){
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("未能找到科目代码%1")).arg(code));
            return false;
        }
        id = q.value(0).toInt();
        return true;
    }

    /**
        按科目名称获取该二级科目id
        参数 fname：一级科目名称，sname：二级科目名称，id：明细科目id
    */
    static bool getSidByName(QString fname, QString sname, int& id){
        QSqlQuery q;
        QString s;
        bool r;

        int fid;
        if(!getIdByName(fid,fname))
            return false;
        s = QString("select FSAgent.id from FSAgent join SecSubjects where "
                    "(FSAgent.sid=SecSubjects.id) and (FSAgent.fid=%1) "
                    "and (SecSubjects.subName='%2')").arg(fid).arg(sname);
        if(q.exec(s) && q.first()){
            id = q.value(0).toInt();
            return true;
        }
        else
            return false;

    }

    /**
        按科目名称获取该科目在一级科目表中的id值
    */
    static bool getIdByName(int& id, QString name){
        QSqlQuery q;
        QString s;

        s = QString("select id from FirSubjects where subName = '%1'")
                .arg(name);
        if(!q.exec(s) || !q.first()){
            QMessageBox::information(0,QObject::tr("提示信息"),
                                     QString(QObject::tr("未能找到科目：%1")).arg(name));
            return false;
        }
        id = q.value(0).toInt();
        return true;
    }

    /**
        获取指定科目类别下的所有一级科目的id
        参数cls：一级科目类别代码，isByView：是否只输出配置为显示的一级科目
    */
    static bool getIdsByCls(QList<int>& ids, int cls, bool isByView)
    {
        QSqlQuery q;
        QString s;
        bool r;

        s = QString("select id from FirSubjects where (belongTo = %1)")
                .arg(cls);
        if(isByView)
            s.append(" and (isView = 1)");
        if(!q.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取此类别的科目id列表")));
            return false;
        }
        while(q.next())
            ids.append(q.value(0).toInt());

        return true;
    }

    //获取所有指定总目下的子目（参数ids：子目id，names：子目名）
    static bool getSndSubInSpecFst(int pid, QList<int>& ids, QList<QString>& names);

    //获取指定总目下、指定子目子集的名称（参数sids：指定子目子集id，names：子目名）
    static bool getSubSetNameInSpecFst(int pid, QList<int> sids, QList<QString>& names);

    /**
        获取指定一级科目下的所有子科目（id到科目名的哈希表）
    */
    static bool getOwnerSub(int oid, QHash<int,QString>& names)
    {
        QSqlQuery q;
        QString s;

        s = QString("select FSAgent.id,SecSubjects.subName from FSAgent "
                    "join SecSubjects where (FSAgent.sid = SecSubjects.id) "
                    "and (fid = %1)").arg(oid);
        if(!q.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取科目id=%1的子科目id列表"))
                                     .arg(oid));
            return false;
        }
        while(q.next())
            names[q.value(0).toInt()] = q.value(1).toString();
        return true;
    }

    /**
        获取所有一级科目下的默认子科目（使用频度最高的子科目）
    */
    static bool getDefaultSndSubs(QHash<int,int>& defSubs);


    /**
        获取子目id所属的总目id反向映射表
    */
    static bool getSidToFid(QHash<int,int>& sidToFids)
    {
        QSqlQuery q;
        QString s;

        s = "select id,fid from FSAgent";
        bool r = q.exec(s);
        while(q.next()){
            sidToFids[q.value(0).toInt()] = q.value(1).toInt();
        }
        return true;
    }

    /**
        获取所有指定类别的损益类总账和明细科目的id列表
    */
    static bool getAllIdForSy(bool isIncome, QHash<int, QList<int> >& ids);

    /**
        获取所有总目id到总目名的哈希表
    */
    static bool getAllSubFName(QHash<int,QString>& names, bool isByView = true);

    /**
        获取所有总目id到总目代码的哈希表
    */
    static bool getAllSubFCode(QHash<int,QString>& codes, bool isByView = true);


    static  bool getAllFstSub(QList<int>& ids, QList<QString>& names, bool isByView = true);

    /**
        获取所有总目id到总目代码的哈希表
    */
    static bool getAllSubCode(QHash<int,QString>& codes, bool isByView = true)
    {
        QString s;
        QSqlQuery q;

        s = "select id,subCode from FirSubjects";
        if(isByView)
            s.append(" where isView = 1");
        bool r = q.exec(s);
        while(q.next())
            codes[q.value(0).toInt()] = q.value(1).toString();
    }

    /**
        获取所有子目id到子目名的哈希表
    */
    static bool getAllSubSName(QHash<int,QString>& names);


    /**
        获取所有子目id到子目全名的哈希表
    */
    static bool getAllSubSLName(QHash<int,QString>& names);

    /**
        获取所有SecSubjects表中的二级科目名列表
    */
    static bool getAllSndSubNameList(QStringList& names);


    /**
        获取所有子目id到子目代码的哈希表
    */
    static bool getAllSubSCode(QHash<int,QString>& codes)
    {
        QSqlQuery q;
        QString s;

        s = QString("select FSAgent.id,FSAgent.subCode from FSAgent "
                    "join SecSubjects where FSAgent.sid = SecSubjects.id");
        if(!q.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取所有子科目哈希表")));
            return false;
        }
        while(q.next())
            codes[q.value(0).toInt()] = q.value(1).toString();
        return true;
    }

    /**
        获取需要进行明细核算的id列表
    */
    static bool getReqDetSubs(QList<int>& ids)
    {
        QSqlQuery q;
        QString s;

        s = "select id from FirSubjects where isReqDet = 1";
        if(!q.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取需要明细支持的一级科目id列表")));
            return false;
        }
        while(q.next())
            ids.append(q.value(0).toInt());
        return true;
    }

//    /**
//        获取指定科目下的所有子科目的余额（直接从SubjectExtras和detailExtras读取）
//        参数ids：一级科目的id列表，values：余额表，isByMt：是否按币种区分
//        values的key：当isByMt为true时，key = 子科目id x 10 + 币种代码
//        当isByMt为false时，key = 子科目id
//    */
//    static bool getExtraBySub(QList<int>& ids, QHash<int,double>& values, bool isByMt)
//    {

//    }

    /**
        计算本期发生额，参数jSums：一级科目借方发生额，key = 一级科目id x 10 + 币种代码
                     参数dSums：一级科目贷方发生额，key = 一级科目id x 10 + 币种代码
                        sjSums：明细科目借方发生额，key = 明细科目id x 10 + 币种代码
                        sdSums：明细科目贷方发生额，key = 明细科目id x 10 + 币种代码
                        isSave：是否可以保存余额，amount：参予统计的凭证数
    */
    static bool calAmountByMonth(int y, int m, QHash<int,double>& jSums, QHash<int,double>& dSums,
                                 QHash<int,double>& sjSums, QHash<int,double>& sdSums,
                                 bool& isSave, int& amount);

    static bool calAmountByMonth2(int y, int m, QHash<int,Double>& jSums, QHash<int,Double>& dSums,
                                 QHash<int,Double>& sjSums, QHash<int,Double>& sdSums,
                                 bool& isSave, int& amount);

    static bool calAmountByMonth3(int y, int m, QHash<int,Double>& jSums, QHash<int,Double>& dSums,
                                 QHash<int,Double>& sjSums, QHash<int,Double>& sdSums,
                                 bool& isSave, int& amount);


    /**
        构造统计查询语句（根据当前凭证集的状态来构造将不同类别的凭证纳入统计范围的SQL语句）
    */
    static bool genStatSql(int y, int m, QString& s);

    /**
        计算本期余额
    */
    static bool calCurExtraByMonth(int y,int m,
       QHash<int,double> preExa, QHash<int,double> preDetExa,     //期初余额
       QHash<int,int> preExaDir, QHash<int,int> preDetExaDir,     //期初余额方向
       QHash<int,double> curJHpn, QHash<int,double> curJDHpn,     //当期借方发生额
       QHash<int,double> curDHpn, QHash<int,double>curDDHpn,      //当期贷方发生额
       QHash<int,double> &endExa, QHash<int,double>&endDetExa,    //期末余额
       QHash<int,int> &endExaDir, QHash<int,int> &endDetExaDir);  //期末余额方向

    static bool calCurExtraByMonth2(int y,int m,
       QHash<int,Double> preExa, QHash<int,Double> preDetExa,     //期初余额
       QHash<int,int> preExaDir, QHash<int,int> preDetExaDir,     //期初余额方向
       QHash<int,Double> curJHpn, QHash<int,Double> curJDHpn,     //当期借方发生额
       QHash<int,Double> curDHpn, QHash<int,Double>curDDHpn,      //当期贷方发生额
       QHash<int,Double> &endExa, QHash<int,Double>&endDetExa,    //期末余额
       QHash<int,int> &endExaDir, QHash<int,int> &endDetExaDir);  //期末余额方向

    static bool calCurExtraByMonth3(int y,int m,
       QHash<int,Double> preExaR, QHash<int,Double> preDetExaR,     //期初余额
       QHash<int,int> preExaDirR, QHash<int,int> preDetExaDirR,     //期初余额方向
       QHash<int,Double> curJHpnR, QHash<int,Double> curJDHpnR,     //当期借方发生额
       QHash<int,Double> curDHpnR, QHash<int,Double>curDDHpnR,      //当期贷方发生额
       QHash<int,Double> &endExaR, QHash<int,Double>&endDetExaR,    //期末余额
       QHash<int,int> &endExaDirR, QHash<int,int> &endDetExaDirR);  //期末余额方向

    /**
        计算科目各币种合计余额及其方向
    */
    static bool calSumByMt(QHash<int,double> exas, QHash<int,int>exaDirs,
                           QHash<int,double>& sums, QHash<int,int>& dirs,
                           QHash<int,double> rates);

    static bool calSumByMt2(QHash<int,Double> exas, QHash<int,int>exaDirs,
                           QHash<int,Double>& sums, QHash<int,int>& dirs,
                           QHash<int,Double> rates);

    /**
        获取所有一级科目id到保存科目余额的字段名称的映射
    */
    static bool getFidToFldName(QHash<int,QString>& names)
    {
        QSqlQuery q;
        QString s;

        s = "select id,subCode from FirSubjects";
        char c;
        if(!q.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取一级科目id到保存余额字段名称的映射")));
            return false;
        }
        while(q.next()){
            int id = q.value(0).toInt();
            QString code = q.value(1).toString();
            c = 'A' + code.left(1).toInt() -1;
            names[id] = QString(c).append(code);
        }
    }

    /**
        获取所有需要按币种分开核算的一级科目id到保存该科目余额的字段名称的映射
    */
    static bool getFidToFldName2(QHash<int,QString>& names)
    {
        QSqlQuery q;
        QString s;

        int id;
        getIdByCode(id,"1002");
        names[id] = "A1002";
        getIdByCode(id,"1131");
        names[id] = "A1131";
        getIdByCode(id,"2121");
        names[id] = "B2121";
        return true;
    }


    /**
        从表SubjectExtras读取指定月份的所有主、子科目余额
        参数 sums：一级科目余额，ssums：明细科目余额，key：id x 10 + 币种代码
            fdirs和sdirs为一二级科目余额的方向
        ????要不要考虑凭证集的状态？？？
    */
    static bool readExtraByMonth(int y,int m, QHash<int,double>& sums,
           QHash<int,int>& fdirs, QHash<int,double>& ssums, QHash<int,int>& sdirs);
    static bool readExtraByMonth2(int y,int m, QHash<int,Double>& sums,
           QHash<int,int>& fdirs, QHash<int,Double>& ssums, QHash<int,int>& sdirs);
    static bool readExtraByMonth3(int y,int m, QHash<int,Double>& sumsR,
           QHash<int,int>& fdirsR, QHash<int,Double>& ssumsR, QHash<int,int>& sdirsR);
    static bool readExtraByMonth4(int y,int m, QHash<int,Double>& sumsR,
           QHash<int,Double>& ssumsR, bool &exist);

    /**
        读取指定月份-指定总账科目的余额（参数fid：总账科目id，v是余额值，dir是方向，key为币种代码）
    */
    static bool readExtraForSub(int y,int m, int fid, QHash<int,double>& v, QHash<int,int>& dir);

    static bool readExtraForSub2(int y,int m, int fid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,int>& dir);

    /**
        读取指定月份-指定明细科目的余额（参数sid：明细科目id，v是余额值，dir是方向，key为币种代码）
    */
    static bool readExtraForDetSub(int y,int m, int sid, QHash<int,double>& v, QHash<int,int>& dir);

    static bool readExtraForDetSub2(int y,int m, int sid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,int>& dir);


    /**
        读取指定月份-指定明细科目-指定币种的余额（参数sid：明细科目id，mt：币种代码，v是余额值，dir是方向）
    */
    static bool readDetExtraForMt(int y,int m, int sid, int mt, double& v, int& dir);

    static bool readDetExtraForMt2(int y,int m, int sid, int mt, Double& v, int& dir);

    /**
        保存当期余额值。key为一二级科目的id * 10 + 币种代码
        带F的是总账科目，带S的是明细科目，dir代表借贷方向
        isSetup true：表示保存当期余额，false：保存设定的期初值。
    */
    static bool savePeriodBeginValues(int y, int m, QHash<int, double> newF, QHash<int, int> newFDir,
                                      QHash<int, double> newS, QHash<int, int> newSDir,
                                      PzsState& pzsState, bool isSetup = true);


    static bool savePeriodBeginValues2(int y, int m, QHash<int, Double> newF, QHash<int, int> newFDir,
                                          QHash<int, Double> newS, QHash<int, int> newSDir,
                                          PzsState& pzsState, bool isSetup = true);
    static bool savePeriodEndValues(int y, int m, QHash<int, Double> newF, QHash<int, Double> newS);


    /**
        保存指定时间的科目余额
         参数 sums：一级科目当期发生额，ssums：明细科目当期发生额，key同上
         ????要不要考虑凭证集的状态？？？
    */
    static bool saveExtryByMonth(int y,int m,QHash<int,double>& sums,
                                 QHash<int,double>ssums)
    {
        bool r;
        QSqlQuery q,q2;
        QString s;

        //首先检测是否存在前期的余额，如果没有要向用户报告，有用户决定是否保存
        QHash<int,double>preSums,preSSums; //前期余额值
        QHash<int,int> preFDirs,preSDirs;  //前期余额方向
        int yy,mm;
        if(m == 1){
            yy = y-1;
            mm = 12;
        }
        else{
            yy = y;
            mm = m -1;
        }

        bool isNew = false;  //是否保存的是第一个月的余额数据
        if(!readExtraByMonth(yy,mm,preSums,preFDirs,preSSums,preSDirs)){
            QString info = QString(QObject::tr("数据库中没有%1年%2月的余额记录！\n"))
                           .arg(y).arg(m);
            info.append(QObject::tr("如果是第一个月的数据则选yes，否则说明数据库记录不完整\n"
                           "缺少前一个月的数据，请选择no"));
            if(QMessageBox::No == QMessageBox::question(0,
                            QObject::tr("询问信息"),info,
                            QMessageBox::Yes | QMessageBox::No))
                return false;
            else
                isNew = true;
        }

        //////////////////////////////////////////////////
//        //计算当期发生额
//        QHash<int,double> curSums,curSSums; //当期发生额
//        if(!readExtraByMonth(y,m,curSums,curSSums))
//            return false;

//        //计算余额
//        QHash<int,double> exSums,exSSums;
//        QHashIterator<int,double> it(curSums);
//        while(it.hasNext()){
//            it.next();
//            exSums[it.key()] = preSums[it.key()] + curSums[it.key()];
//        }
//        QHashIterator<int,double> is(curSSums);
//        while(is.hasNext()){
//            is.next();
//            exSSums[is.key()] = preSSums[is.key()] + curSSums[is.key()];
//        }

        //构造余额数据
        QHashIterator<int,double> ips(preSums);
        while(ips.hasNext()){
            ips.next();
            sums[ips.key()] += ips.value();
        }
        QHashIterator<int,double> ipss(preSSums);
        while(ipss.hasNext()){
            ipss.next();
            ssums[ipss.key()] += ipss.value();
        }

        //保存余额到数据库中
        //首先检测是否已经有当期的余额记录，如有则删除
        r = q.exec(QString("select id from SubjectExtras where (year = %1)"
                       " and (month = %2)").arg(y).arg(m));
        if(r){
            while(q.next()){
                int id = q.value(0).toInt();
                r = q2.exec(QString("delete from detailExtras where seid = %1").arg(id));//删除明细余额
                r = q2.exec(QString("delete from SubjectExtras where id = %1").arg(id)); //删除总账余额
            }
        }

//        s = QString("delete from SubjectExtras where (year = %1) "
//                    "and (month = %2)").arg(y).arg(m);
//        r = q.exec(s);



        //构造插入语句
        QList<QString> fnList; //要绑定值的字段名列表（有insert语句中由冒号前导的符号）
        QString fnStr = "(year, month, state, mt, ";       //SQL 的insert语句中的表字段部分
        QString vStr = "values(:year, :month, :state, :mt, ";  //values 部分
        QHash<int,QString>fldNames; //一级科目id到保存余额的字段名的映射
        getFidToFldName(fldNames);
        QList<QString> fields = fldNames.values();
        qSort(fields.begin(),fields.end());
        for(int i = 0; i < fields.count(); ++i){
            QString fname = fields[i];
            fnStr.append(fname);
            fnStr.append(", ");
            vStr.append(":");
            vStr.append(fname);
            vStr.append(", ");
            fnList.append(fname);
        }
        fnStr.chop(2);
        fnStr.append(") ");
        vStr.chop(2);
        vStr.append(")");
        s = "insert into SubjectExtras ";
        s.append(fnStr);
        s.append(vStr);
        r = q.prepare(s);

        //按币种分别插入余额值
        QHash<int,QString> mts;
        getMTName(mts);
        QHashIterator<int,QString> mti(mts);
        while(mti.hasNext()){
            mti.next();
            //绑定年月值及币种代码
            q.bindValue(":year", y);
            q.bindValue(":month", m);
            q.bindValue(":state", 1);  //结转状态有待考虑
            q.bindValue(":mt", mti.key());

            //绑定总账科目余额值
            QHash<QString,int> names; //对fldNames进行倒置，从字段名映射到科目id
            QHashIterator<int,QString>  i(fldNames);
            while(i.hasNext()){
                i.next();
                names[i.value()] = i.key();
            }
            for(int i = 0; i < fnList.count(); ++i)
                q.bindValue(":"+fnList[i], sums.value(names[fnList[i]]*10+mti.key()));

            r = q.exec();
        }



///////////////////////////////////////////
        //提取保存当期总账科目余额的记录id集合
        s = QString("select id,mt from SubjectExtras where (year = %1) "
                    "and (month = %2)").arg(y).arg(m);
        r = q.exec(s);
        while(q.next()){
            int curId = q.value(0).toInt();//保存总账科目余额的记录id
            int mt = q.value(1).toInt(); //总账科目余额的币种代码
            //将当期的明细余额值保存到detailExtras中
            QHashIterator<int,double> ip(ssums);
            while(ip.hasNext()){
                ip.next();
                if((ip.key() % 10) == mt){ //对应币种保存明细科目余额
                    int id = ip.key() / 10;//明细科目id
                    s = QString("insert into detailExtras(seid,fsid,value) "
                                "values(%1,%2,%3)")
                            .arg(curId).arg(id).arg(ip.value());
                    r = q2.exec(s);
                }

            }
        }




//            s = QString("select id from detailExtras where (seid = %1) "
//                        "and (fsid = %2) and (mt = %3)")
//                    .arg(curId).arg(id).arg(mt);
//            r = q.exec(s);
//            int eid; //detailExtras表的id列
//            if(q.first()){
//                isExist = true;
//                eid = q.value(0).toInt();
//            }
//            //存在则更新
//            if(isExist){
//                s = QString("update detailExtras set value = %1 where "
//                            "id = %2").arg(ip.value()).arg(eid);
//            }
//            else{//不存在则插入

//            }
//        }
        return true;
    }

    //生成明细科目日记账数据列表（金额式）
    static bool getDailyForJe(int y,int m, int fid, int sid,
                              QList<RowTypeForJe*>& dlist, double& preExtra, int& preExtraDir);
    //生成明细科目日记账数据列表（外币金额式）
    static bool getDailyForWj(int y,int m, int fid, int sid,
                              QList<RowTypeForWj*>& dlist,
                              QHash<int,double>& preExtra,
                              QHash<int,int>& preExtraDir);

    //将各币种的余额汇总为用母币计的余额并确定余额方向
    static bool calExtraAndDir(QHash<int,double> extra,QHash<int,int> extraDir,
                               QHash<int,double> rate,double& mExtra,int& mDir);

    //获取指定月份范围，指定科目的日记账/明细账数据
    static bool getDailyAccount(int y, int sm, int em, int fid, int sid, int mt,
                                double& prev, int& preDir,
                                QList<DailyAccountData*>& datas,
                                QHash<int,double>& preExtra,
                                QHash<int,int>& preExtraDir,
                                QHash<int, double>& rates,
                                QList<int> fids = QList<int>(),
                                QHash<int,QList<int> > sids = QHash<int,QList<int> >(),
                                double gv = 0,
                                double lv = 0,
                                bool inc = false);
    //获取指定月份范围，指定科目的日记账/明细账数据
    static bool getDailyAccount2(int y, int sm, int em, int fid, int sid, int mt,
                                Double& prev, int& preDir,
                                QList<DailyAccountData2*>& datas,
                                QHash<int,Double>& preExtra,
                                QHash<int,Double>& preExtraR,
                                QHash<int,int>& preExtraDir,
                                QHash<int, Double>& rates,
                                QList<int> fids = QList<int>(),
                                QHash<int,QList<int> > sids = QHash<int,QList<int> >(),
                                Double gv = 0.00,
                                Double lv = 0.00,
                                bool inc = false);


    //获取指定月份范围，指定总账科目的总账数据
    static bool getTotalAccount(int y, int sm, int em, int fid,
                                QList<TotalAccountData*>& datas,
                                QHash<int,double>& preExtra,
                                QHash<int,int>& preExtraDir,
                                QHash<int, double>& rates);

    //生成欲打印凭证的数据集合
    static bool genPzPrintDatas(int y, int m, QList<PzPrintData*> &datas, QSet<int> pznSet = QSet<int>());
    static bool genPzPrintDatas2(int y, int m, QList<PzPrintData2*> &datas, QSet<int> pznSet = QSet<int>());


    //获取指定年月指定类别的凭证id
    static bool getPzIdForSpecCls(int y, int m, int cls, User* user, int& id);

    /////////////////////////////////////////////////////////////
    //将整数集合转为简写的文本形式（每个连续的数字区段用比如“4-8”的形式，多个区段用逗号分隔）
//    static void IntSetToStr(QSet<int> set, QString& s)
//    {
//        if(set.count() > 0){
//            QList<int> pzs = set.toList();
//            qSort(pzs.begin(),pzs.end());
//            int prev = pzs[0],next = pzs[0];
//            //QString s;
//            for(int i = 1; i < pzs.count(); ++i){
//                if((pzs[i] - next) == 1){
//                    next = pzs[i];
//                }
//                else{
//                    if(prev == next)
//                        s.append(QString::number(prev)).append(",");
//                    else
//                        s.append(QString("%1-%2").arg(prev).arg(next)).append(",");
//                    prev = next = pzs[i];
//                }
//            }
//            if(prev == next)
//                s.append(QString::number(prev));
//            else
//                s.append(QString("%1-%2").arg(prev).arg(pzs[pzs.count() - 1]));

//        }
//    }

//    //将简写的文本格式转为整数集合
//    static bool strToIntSet(QString s, QSet<int>& set)
//    {
//        //首先用规则表达式验证字符串中是否存在不可解析的字符，如有则返回false
//        if(false)
//            return false;

//        set.clear();
//        //对打印范围的编辑框文本进行解析，生成凭证号集合
//        QStringList sels = s.split(",");
//        for(int i = 0; i < sels.count(); ++i){
//            if(sels[i].indexOf('-') == -1)
//                set.insert(sels[i].toInt());
//            else{
//                QStringList ps = sels[i].split("-");
//                int start = ps[0].toInt();
//                int end = ps[1].toInt();
//                for(int j = start; j <= end; ++j)
//                    set.insert(j);
//            }
//        }
//        return true;
//    }

    //
    static bool delActionsInPz(int pzId);

    //在FSAgent表中创建新的一二级科目的映射条目
    static bool newFstToSnd(int fid, int sid, int& id);

    /**
        创建新的二级科目名称，并建立与指定一级科目的对应关系
        参数 fid：所属的一级科目id，id 新的二级科目与一级科目的映射条目的id，name：二级科目名，
            lname：二级科目全称，remCode：科目助记符，clsCode：科目名称所属类别代码
    */
    static bool newSndSubAndMapping(int fid, int& id, QString name, QString lname, QString remCode, int clsCode);

    //读取指定一级科目到指定二级科目的映射条目的id
    static bool getFstToSnd(int fid, int sid, int& id);

    //获取指定id（FSAgent表的id字段）的二级科目名称
    static bool getSndSubNameForId(int id, QString& name, QString& lname);

    //获取凭证集内最大的可用凭证号
    static int getMaxPzNum(int y, int m);

    //读取凭证集状态
    static bool getPzsState(int y,int m,PzsState& state);
    //设置凭证集状态
    static bool setPzsState(int y,int m,PzsState state);
    //设置凭证集状态，并根据设定的状态调整凭证集内凭证的审核入账状态
    static bool setPzsState2(int y,int m,PzsState state);

    //获取银行存款下所有外币账户对应的明细科目id列表
    static bool getOutMtInBank(QList<int>& ids, QList<int>& mt);

    //新建凭证
    static bool crtNewPz(PzData* pz);    

    //按凭证日期，重新设置凭证集内的凭证号
    static bool assignPzNum(int y, int m);

    //按二级科目id获取二级科目名（这里的sid是SecSubjects表的id）
    static bool getSNameForId(int sid, QString& name, QString& lname);

    //保存账户信息到账户文件（中的AccountInfos表中）
    static bool saveAccInfo(AccountBriefInfo* accInfo);

    //读取银行帐号
    static bool readAllBankAccont(QHash<int,BankAccount*>& banks);

    //刷新凭证集状态
    static bool refreshPzsState(int y,int m,CustomRelationTableModel* model/*,
                                QSet<int> pcImps = pzClsImps,
                                QSet<int> pcJzhds = pzClsJzhds,
                                QSet<int> pcJzsys = pzClsJzsys*/);
    //


    //引入其他模块产生的凭证
    static bool impPzFromOther(int y,int m, QSet<OtherModCode> mods);
    //取消引入的由其他模块产生的凭证
    static bool antiImpPzFromOther(int y, int m, QSet<OtherModCode> mods);
    //其他模块是否需要生成指定年月的引入类凭证
    static bool reqGenImpOthPz(int y,int m, bool& req);

    //创建结转汇兑损益凭证
    static bool genForwordEx(int y, int m, User* user, int state = Pzs_Recording);
    static bool genForwordEx2(int y, int m, User* user, int state = Pzs_Recording);
    //取消结转汇兑损益凭证
    static bool antiJzhdsyPz(int y, int m);
    //是否需要结转汇兑损益
    static bool reqGenJzHdsyPz(int y, int m, bool& req);

    //创建结转损益类科目到本年利润的凭证
    //static bool genForwordPl(int y, int m, QHash<int,QString>* fnames,
    //                         QHash<int,QString>* snames, User* user);
    static bool genForwordPl(int y, int m, User* user);
    static bool genForwordPl2(int y, int m, User *user);
    //取消结转损益类科目到本年利润的凭证
    static bool antiForwordPl(int y, int m);
    //是否需要创建结转损益类科目到本年利润的凭证
    static bool reqForwordPl(int y, int m, bool& req);

    //生成结转本年利润的凭证
    static bool genJzbnlr(int y, int m, PzData& d);
    //取消结转本年利润的凭证
    static bool antiJzbnlr(int y, int m);
    //是否需要创建结转本年利润的凭证
    static bool reqGenJzbnlr(int y, int m, bool& req);


    //获取指定范围的科目id列表
    static bool getSubRange(int sfid,int ssid,int efid,int esid,
                            QList<int>& fids,QHash<int,QList<int> >& sids);

    //指定id的凭证是否处于指定的年月内
    static bool isPzInMonth(int y, int m, int pzid, bool& isIn);

    //获取指定一级科目是否需要按币种进行分别核算
    static bool isAccMt(int fid);
    //获取指定二级科目是否需要按币种进行分别核算
    static bool isAccMtS(int sid);

    //判断指定的科目id是否是损益类科目的费用类或收入类科目
    static bool isFeiSub(int fid){return feiIds.contains(fid);}
    static bool isInSub(int fid){return inIds.contains(fid);}
    static bool isFeiSSub(int sid){return feiSIds.contains(sid);}
    static bool isInSSub(int sid){return inSIds.contains(sid);}

    //获取所有在二级科目类别表中名为“固定资产类”的科目（已归并到Gdzc类）
    //static bool getGdzcSubClass(QHash<int,QString>& names);
    static bool delSpecPz(int y, int m, PzClass pzCls);

private:
    //为查询处于指定状态的某些类别的凭证生成过滤子句
    static void genFiltState(QList<int> pzCls, PzState state, QString& s);
    //生成过滤出指定类别的凭证的条件语句
    static QString genFiltStateForSpecPzCls(QList<int> pzClses);
    //删除指定年月的指定类别凭证


    //固定资产管理模块是否需要产生凭证
    static bool reqGenGdzcPz(int y, int m, bool& req);
    //生成固定资产折旧凭证
    static bool genGdzcZjPz(int y,int m);
    //取消固定资产管理模块引入的凭证
    static bool antiGdzcPz(int y, int m);

    //待摊费用管理模块是否需要产生凭证
    static bool reqGenDtfyPz(int y, int m, bool& req);
    //生成待摊费用凭证
    static bool genDtfyPz(int y,int m);
    //取消待摊费用管理模块引入的凭证
    static bool antiDtfyPz(int y, int m);

    //判断凭证类别的方法
    static bool isImpPzCls(PzClass pzc);     //是否是由其他模块引入的凭证类别
    static bool isJzhdPzCls(PzClass pzc);    //是否是由系统自动产生的结转汇兑损益凭证类别
    static bool isJzsyPzCls(PzClass pzc);    //是否是由系统自动产生的结转损益凭证类别
    static bool isOtherPzCls(PzClass pzc);   //是否是其他由系统产生并允许用户修改的凭证类别
};




///////////////////////////////////////////////////////////////

//提供各种各样的使用函数的类(不能插入断点，郁闷中？)
class VariousUtils{
public:

    //将整数集合转为简写的文本形式（每个连续的数字区段用比如“4-8”的形式，多个区段用逗号分隔）
    static QString IntSetToStr(QSet<int> set)
    {
        QString s;
        if(set.count() > 0){
            QList<int> pzs = set.toList();
            qSort(pzs.begin(),pzs.end());
            int prev = pzs[0],next = pzs[0];
            for(int i = 1; i < pzs.count(); ++i){
                if((pzs[i] - next) == 1){
                    next = pzs[i];
                }
                else{
                    if(prev == next)
                        s.append(QString::number(prev)).append(",");
                    else
                        s.append(QString("%1-%2").arg(prev).arg(next)).append(",");
                    prev = next = pzs[i];
                }
            }
            if(prev == next)
                s.append(QString::number(prev));
            else
                s.append(QString("%1-%2").arg(prev).arg(pzs[pzs.count() - 1]));
        }
        return s;
    }

    //将简写的文本格式转为整数集合
    static bool strToIntSet(QString s, QSet<int>& set)
    {
        //首先用规则表达式验证字符串中是否存在不可解析的字符，如有则返回false
        if(false)
            return false;

        set.clear();
        //对打印范围的编辑框文本进行解析，生成凭证号集合
        QStringList sels = s.split(",");
        for(int i = 0; i < sels.count(); ++i){
            if(sels[i].indexOf('-') == -1)
                set.insert(sels[i].toInt());
            else{
                QStringList ps = sels[i].split("-");
                int start = ps[0].toInt();
                int end = ps[1].toInt();
                for(int j = start; j <= end; ++j)
                    set.insert(j);
            }
        }
        return true;
    }

    //获取子窗口信息
    static bool getSubWinInfo(int winEnum, SubWindowDim*& info, QByteArray*& otherInfo);

    //保存字窗口信息
    static bool saveSubWinInfo(int winEnum, SubWindowDim* info, QByteArray* otherInfo = NULL);

    //获取子窗口信息
    static bool getSubWinInfo3(int winEnum, QByteArray*& ba);

    //保存字窗口信息
    static bool saveSubWinInfo3(int winEnum, QByteArray* otherInfo);
};



#endif // UTILS_H
