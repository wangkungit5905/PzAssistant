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

    static QSqlDatabase db;     //为了能够让它暂时为我服务，给它一个数据连接对象而不是使用默认的连接

    //类的初始化函数
    static bool init(QSqlDatabase &db);

    //static bool getActionsInPz(int pid, QList<BusiActionData2*>& busiActions);

    //static bool saveActionsInPz2(int pid, QList<BusiActionData2*>& busiActions,
    //                            QList<BusiActionData2*> dels = QList<BusiActionData2*>());

    /**
        判断科目余额的借贷方向，1：借方，0：平，-1：贷方，-2：无定义（可能对于此科目类型还未作处理）
    */
//    static int getDirSubExa(double v, int pid)
//    {


//        if(v == 0)
//            return 0;
//        if(v > 0){
//            if(pset.contains(pid))
//                return 1;
//            else if(nset.contains(pid))
//                return -1;
//            else
//                return -2;
//        }
//        else{
//            if(pset.contains(pid))
//                return -1;
//            else if(nset.contains(pid))
//                return 1;
//            else
//                return -2;
//        }
//    }

//    /**
//        判断明细科目余额的借贷方向，1：借方，0：平，-1：贷方
//    */
//    static int getDirSubDetExa(double v, int sid)
//    {
//        if(v == 0)
//            return 0;
//        if(v > 0){
//            if(spset.contains(sid))
//                return 1;
//            else if(snset.contains(sid))
//                return -1;
//            else
//                return -2;
//        }
//        else{
//            if(spset.contains(sid))
//                return -1;
//            else if(snset.contains(sid))
//                return 1;
//            else
//                return -2;
//        }
//    }

//    /**
//        判断总账科目余额的借贷方向---文本版
//    */
//    static QString getSubExaDir(double v, int pid)
//    {
//        int dir = getDirSubExa(v,pid);
//        if(dir == 1)
//            return QString(QObject::tr("借"));
//        else if(dir == -1)
//            return QString(QObject::tr("贷"));
//        else if(dir == 0)
//            return QString(QObject::tr("平"));
//        else
//            return QString(QObject::tr("？"));
//    }

//    /**
//        判断明细科目余额的借贷方向---文本版
//    */
//    static QString getSubDetExaDir(double v, int sid)
//    {
//        int dir = getDirSubExa(v,sid);
//        if(dir == 1)
//            return QString(QObject::tr("借"));
//        else if(dir == -1)
//            return QString(QObject::tr("贷"));
//        else if(dir == 0)
//            return QString(QObject::tr("平"));
//        else
//            return QString(QObject::tr("？"));
//    }

//    /**
//        获取所有用户名
//    */
//    static void getAllUser(QHash<int,QString>& users)
//    {
//        //
//    }

    static bool getRates2(int y,int m, QHash<int,Double>& rates, int mainMt = RMB);
    static bool saveRates2(int y,int m, QHash<int,Double>& rates, int mainMt = RMB);


    //static bool getFstSubCls(QHash<int,QString>& clsNames,int subSys = 1);



    //    /**
    //        获取指定的一级科目类别代码
    //        此功能将并入科目管理器对象中
    //    */
    //    static bool getSubClsCode(int& code, QString name)
    //    {
    //        QSqlQuery q;
    //        QString s;

    //        s = QString("select code from FstSubClasses where name = '%1'")
    //                .arg(name);
    //        if(!q.exec() || !q.first()){
    //            QMessageBox::information(0,QObject::tr("提示信息"),
    //                                     QString(QObject::tr("未能找到%1的类别代码")).arg(name));
    //            return false;
    //        }
    //        code = q.value(0).toInt();
    //        return true;
    //    }


    static bool getMTName(QHash<int,QString>& names);


//    /**
//        获取指定币种的代码
//    */
//    static bool getMtCode(int& code, QString name)
//    {
//        QSqlQuery q;
//        QString s;

//        s = QString("select code from MoneyTypes where name = '%1'").arg(name);
//        if(q.exec(s) && q.first()){
//            code = q.value(0).toInt();
//            return true;
//        }
//        else{
//            QMessageBox::information(0,QObject::tr("提示信息"),
//                                     QString(QObject::tr("未能找到%1的代码")).arg(name));
//            return false;
//        }
//    }

    //static bool getSubCodeByName(QString& code, QString name, int subSys = 1);
    static bool getIdByCode(int& id, QString code, int subSys = 1);
    //static bool getSidByName(QString fname, QString sname, int& id, int subSys = 1);
    //static bool getIdByName(int& id, QString name, int subSys = 1);
    //static bool getIdsByCls(QList<int>& ids, int cls, bool isByView, int subSys = 1);
    static bool getSndSubInSpecFst(int pid, QList<int>& ids, QList<QString>& names, bool isAll = true, int subSys = 1);



    //static bool getOwnerSub(int oid, QHash<int,QString>& names);
    //static bool getDefaultSndSubs(QHash<int,int>& defSubs, int subSys = 1);
    //static bool getSidToFid(QHash<int,int>& sidToFids, int subSys = 1);


    /**
        获取所有指定类别的损益类总账和明细科目的id列表
    */
    //static bool getAllIdForSy(bool isIncome, QHash<int, QList<int> >& ids, int subSys = 1);

    /**
        获取所有总目id到总目名的哈希表
    */
    //static bool getAllSubFName(QHash<int,QString>& names, bool isByView = true);

    /**
        获取所有总目id到总目代码的哈希表
    */
    //static bool getAllSubFCode(QHash<int,QString>& codes, bool isByView = true);


    //static  bool getAllFstSub(QList<int>& ids, QList<QString>& names, bool isByView = true);

    //static bool getAllSubCode(QHash<int,QString>& codes, bool isByView = true);


    /**
        获取所有子目id到子目名的哈希表
    */
    //static bool getAllSubSName(QHash<int,QString>& names);


    /**
        获取所有子目id到子目全名的哈希表
    */
    //static bool getAllSubSLName(QHash<int,QString>& names);

    /**
        获取所有SecSubjects表中的二级科目名列表
    */
    //static bool getAllSndSubNameList(QStringList& names);

    //static bool getAllSubSCode(QHash<int,QString>& codes);


    //static bool getReqDetSubs(QList<int>& ids);

//    static bool calAmountByMonth2(int y, int m, QHash<int,Double>& jSums, QHash<int,Double>& dSums,
//                                 QHash<int,Double>& sjSums, QHash<int,Double>& sdSums,
//                                 bool& isSave, int& amount);

//    static bool calAmountByMonth3(int y, int m, QHash<int,Double>& jSums, QHash<int,Double>& dSums,
//                                 QHash<int,Double>& sjSums, QHash<int,Double>& sdSums,
//                                 bool& isSave, int& amount);


    /**
        构造统计查询语句（根据当前凭证集的状态来构造将不同类别的凭证纳入统计范围的SQL语句）
    */
    //static bool genStatSql(int y, int m, QString& s);
    //static bool genStatSql2(int y, int m, QString& s);

    /**
        计算本期余额
    */
//    static bool calCurExtraByMonth(int y,int m,
//       QHash<int,double> preExa, QHash<int,double> preDetExa,     //期初余额
//       QHash<int,int> preExaDir, QHash<int,int> preDetExaDir,     //期初余额方向
//       QHash<int,double> curJHpn, QHash<int,double> curJDHpn,     //当期借方发生额
//       QHash<int,double> curDHpn, QHash<int,double>curDDHpn,      //当期贷方发生额
//       QHash<int,double> &endExa, QHash<int,double>&endDetExa,    //期末余额
//       QHash<int,int> &endExaDir, QHash<int,int> &endDetExaDir);  //期末余额方向

//    static bool calCurExtraByMonth2(int y,int m,
//       QHash<int,Double> preExa, QHash<int,Double> preDetExa,     //期初余额
//       QHash<int,MoneyDirection> preExaDir, QHash<int,MoneyDirection> preDetExaDir,     //期初余额方向
//       QHash<int,Double> curJHpn, QHash<int,Double> curJDHpn,     //当期借方发生额
//       QHash<int,Double> curDHpn, QHash<int,Double>curDDHpn,      //当期贷方发生额
//       QHash<int,Double> &endExa, QHash<int,Double>&endDetExa,    //期末余额
//       QHash<int,MoneyDirection> &endExaDir, QHash<int,MoneyDirection> &endDetExaDir);  //期末余额方向

    /*static bool calCurExtraByMonth3(int y,int m,
       QHash<int,Double> preExaR, QHash<int,Double> preDetExaR,     //期初余额
       QHash<int,MoneyDirection> preExaDirR, QHash<int,MoneyDirection> preDetExaDirR,     //期初余额方向
       QHash<int,Double> curJHpnR, QHash<int,Double> curJDHpnR,     //当期借方发生额
       QHash<int,Double> curDHpnR, QHash<int,Double>curDDHpnR,      //当期贷方发生额
       QHash<int,Double> &endExaR, QHash<int,Double>&endDetExaR,    //期末余额
       QHash<int,MoneyDirection> &endExaDirR, QHash<int,MoneyDirection> &endDetExaDirR);*/  //期末余额方向

    /**
        计算科目各币种合计余额及其方向
    */
//    static bool calSumByMt(QHash<int,double> exas, QHash<int,int>exaDirs,
//                           QHash<int,double>& sums, QHash<int,int>& dirs,
//                           QHash<int,double> rates);

//    static bool calSumByMt2(QHash<int,Double> exas, QHash<int,int>exaDirs,
//                           QHash<int,Double>& sums, QHash<int,int>& dirs,
//                           QHash<int,Double> rates);

    static bool getFidToFldName(QHash<int,QString>& names);


    /**
        获取所有需要按币种分开核算的一级科目id到保存该科目余额的字段名称的映射
    */
    static bool getFidToFldName2(QHash<int,QString>& names)
    {
        //这只能作为临时性的解决方法，最终的解决方法是用新的保存余额机制。
        int id;
        getIdByCode(id,"1002");
        names[id] = "A1002";
        getIdByCode(id,"1131");
        names[id] = "A1131";
        getIdByCode(id,"2121");
        names[id] = "B2121";
        getIdByCode(id,"1151");
        names[id] = "A1151";
        getIdByCode(id,"2131");
        names[id] = "B2131";
        return true;
    }


    /**
        从表SubjectExtras读取指定月份的所有主、子科目余额
        参数 sums：一级科目余额，ssums：明细科目余额，key：id x 10 + 币种代码
            fdirs和sdirs为一二级科目余额的方向
        ????要不要考虑凭证集的状态？？？
    */

    static bool readExtraByMonth2(int y,int m, QHash<int,Double>& sums,
           QHash<int,MoneyDirection>& fdirs, QHash<int,Double>& ssums, QHash<int,MoneyDirection>& sdirs);
    static bool readExtraByMonth3(int y,int m, QHash<int,Double>& sumsR,
           QHash<int,MoneyDirection>& fdirsR, QHash<int,Double>& ssumsR, QHash<int,MoneyDirection>& sdirsR);
    static bool readExtraByMonth4(int y,int m, QHash<int,Double>& sumsR,
           QHash<int,Double>& ssumsR, bool &exist);

    static bool readExtraForSub2(int y,int m, int fid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,int>& dir);

    static bool readExtraForDetSub2(int y,int m, int sid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,int>& dir);

    //static bool readDetExtraForMt2(int y,int m, int sid, int mt, Double& v, int& dir);

    //static bool savePeriodBeginValues2(int y, int m, QHash<int, Double> newF, QHash<int, MoneyDirection> newFDir,
    //                                      QHash<int, Double> newS, QHash<int, MoneyDirection> newSDir,
    //                                      bool isSetup = true);
    //static bool savePeriodEndValues(int y, int m, QHash<int, Double> newF, QHash<int, Double> newS);


    //
    //static bool calExtraAndDir(QHash<int,double> extra,QHash<int,int> extraDir,
    //                           QHash<int,double> rate,double& mExtra,int& mDir);
    static bool calExtraAndDir2(QHash<int,Double> extra,QHash<int,int> extraDir,
                               QHash<int,Double> rate,Double& mExtra,int& mDir);

    //获取指定月份范围，指定科目的日记账/明细账数据
//    static bool getDailyAccount(int y, int sm, int em, int fid, int sid, int mt,
//                                double& prev, int& preDir,
//                                QList<DailyAccountData*>& datas,
//                                QHash<int,double>& preExtra,
//                                QHash<int,int>& preExtraDir,
//                                QHash<int, double>& rates,
//                                QList<int> fids = QList<int>(),
//                                QHash<int,QList<int> > sids = QHash<int,QList<int> >(),
//                                double gv = 0,
//                                double lv = 0,
//                                bool inc = false);
    //获取指定月份范围，指定科目的日记账/明细账数据
//    static bool getDailyAccount2(int y, int sm, int em, int fid, int sid, int mt,
//                                Double& prev, int& preDir,
//                                QList<DailyAccountData2*>& datas,
//                                QHash<int,Double>& preExtra,
//                                QHash<int,Double>& preExtraR,
//                                QHash<int,int>& preExtraDir,
//                                QHash<int, Double>& rates,
//                                QList<int> fids = QList<int>(),
//                                QHash<int,QList<int> > sids = QHash<int,QList<int> >(),
//                                Double gv = 0.00,
//                                Double lv = 0.00,
//                                bool inc = false);


    //获取指定月份范围，指定总账科目的总账数据
    static bool getTotalAccount(int y, int sm, int em, int fid,
                                QList<TotalAccountData2 *> &datas,
                                QHash<int, Double> &preExtra,
                                QHash<int,int>& preExtraDir,
                                QHash<int, Double> &rates);

    //生成欲打印凭证的数据集合
    //static bool genPzPrintDatas2(int y, int m, QList<PzPrintData2*> &datas, QSet<int> pznSet = QSet<int>());


    //获取指定年月指定类别的凭证id
    //static bool getPzIdForSpecCls(int y, int m, int cls, User* user, int& id);

    //
    //static bool delActionsInPz(int pzId);

    //在FSAgent表中创建新的一二级科目的映射条目
    //static bool newFstToSnd(int fid, int sid, int& id);

    /**
        创建新的二级科目名称，并建立与指定一级科目的对应关系
        参数 fid：所属的一级科目id，id 新的二级科目与一级科目的映射条目的id，name：二级科目名，
            lname：二级科目全称，remCode：科目助记符，clsCode：科目名称所属类别代码
    */
    //static bool newSndSubAndMapping(int fid, int& id, QString name, QString lname, QString remCode, int clsCode);

    //static bool getFstToSnd(int fid, int nid, int& id);
    //static bool isSndSubDisabled(int id, bool& enabled);

    //获取指定id（FSAgent表的id字段）的二级科目名称
    //static bool getSndSubNameForId(int id, QString& name, QString& lname);

    //获取凭证集内最大的可用凭证号
    //static int getMaxPzNum(int y, int m);

    //读取凭证集状态
    static bool getPzsState(int y,int m,PzsState& state);
    //设置凭证集状态
    static bool setPzsState(int y,int m,PzsState state);

    //获取银行存款下所有外币账户对应的明细科目id列表
    //static bool getOutMtInBank(QList<int>& ids, QList<int>& mt);

    //新建凭证
    //static bool crtNewPz(PzData* pz);

    //按凭证日期，重新设置凭证集内的凭证号
    //static bool assignPzNum(int y, int m);

    //static bool getSNameForId(int sid, QString& name, QString& lname);

    //保存账户信息到账户文件（中的AccountInfos表中）
    //static bool saveAccInfo(AccountBriefInfo* accInfo);

    //读取银行帐号
    //static bool readAllBankAccont(QHash<int,BankAccount*>& banks);

    //static bool scanPzSetCount(int y, int m, int &repeal, int &recording, int &verify, int &instat, int &amount);
    //static bool inspectJzPzExist(int y, int m, PzdClass pzCls, int& count);

    //引入其他模块产生的凭证
    //static bool impPzFromOther(int y,int m, QSet<OtherModCode> mods);
    //取消引入的由其他模块产生的凭证
    //static bool antiImpPzFromOther(int y, int m, QSet<OtherModCode> mods);
    //其他模块是否需要生成指定年月的引入类凭证
    //static bool reqGenImpOthPz(int y,int m, bool& req);

    //创建结转汇兑损益凭证
    //static bool genForwordEx2(int y, int m, User* user, int state = Pzs_Recording);
    //是否需要结转汇兑损益
    //static bool reqGenJzHdsyPz(int y, int m, bool& req);

    //创建结转损益类科目到本年利润的凭证
    //static bool genForwordPl2(int y, int m, User *user);

    //获取指定范围的科目id列表
    //static bool getSubRange(int sfid,int ssid,int efid,int esid,
    //                        QList<int>& fids,QHash<int,QList<int> >& sids);

    //指定id的凭证是否处于指定的年月内
    //static bool isPzInMonth(int y, int m, int pzid, bool& isIn);

    //获取指定一级科目是否需要按币种进行分别核算
    static bool isAccMt(int fid);
    //获取指定二级科目是否需要按币种进行分别核算
    static bool isAccMtS(int sid);

    //判断指定的科目id是否是损益类科目的费用类或收入类科目
    //static bool isFeiSub(int fid){return feiIds.contains(fid);}
    //static bool isInSub(int fid){return inIds.contains(fid);}
    //static bool isFeiSSub(int sid){return feiSIds.contains(sid);}
    //static bool isInSSub(int sid){return inSIds.contains(sid);}

    //获取所有在二级科目类别表中名为“固定资产类”的科目（已归并到Gdzc类）
    //static QList<PzClass> getSpecClsPzCode(PzdClass cls);
    //static bool delSpecPz(int y, int m, PzdClass pzCls, int &affected);
    //static bool haveSpecClsPz(int y, int m, QHash<PzdClass,bool>& isExist);
    static bool setExtraState(int y, int m, bool isVolid);
    static bool getExtraState(int y, int m);
    //static bool specPzClsInstat(int y, int m, PzdClass cls, int &affected);
    //static bool setAllPzState(int y, int m, PzState state, PzState includeState,
    //                          int &affected, User* user = curUser);

private:
    //为查询处于指定状态的某些类别的凭证生成过滤子句
    //static void genFiltState(QList<int> pzCls, PzState state, QString& s);
    //生成过滤出指定类别的凭证的条件语句
    //static QString genFiltStateForSpecPzCls(QList<int> pzClses);

    //固定资产管理模块是否需要产生凭证
    //static bool reqGenGdzcPz(int y, int m, bool& req);
    //生成固定资产折旧凭证
    //static bool genGdzcZjPz(int y,int m);
    //取消固定资产管理模块引入的凭证
    //static bool antiGdzcPz(int y, int m);

    //待摊费用管理模块是否需要产生凭证
    //static bool reqGenDtfyPz(int y, int m, bool& req);
    //生成待摊费用凭证
    //static bool genDtfyPz(int y,int m);
    //取消待摊费用管理模块引入的凭证
    //static bool antiDtfyPz(int y, int m);

    //判断凭证类别的方法
    //static bool isImpPzCls(PzClass pzc);     //是否是由其他模块引入的凭证类别
    //static bool isJzhdPzCls(PzClass pzc);    //是否是由系统自动产生的结转汇兑损益凭证类别
    //static bool isJzsyPzCls(PzClass pzc);    //是否是由系统自动产生的结转损益凭证类别
    //static bool isOtherPzCls(PzClass pzc);   //是否是其他由系统产生并允许用户修改的凭证类别


};




///////////////////////////////////////////////////////////////

//提供各种各样的使用函数的类(不能插入断点，郁闷中？)
class VariousUtils{
public:

    //将整数集合转为简写的文本形式（每个连续的数字区段用比如“4-8”的形式，多个区段用逗号分隔）
//    static QString IntSetToStr(QSet<int> set)
//    {
//        QString s;
//        if(set.count() > 0){
//            QList<int> pzs = set.toList();
//            qSort(pzs.begin(),pzs.end());
//            int prev = pzs[0],next = pzs[0];
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
//        return s;
//    }

    //将简写的文本格式转为整数集合
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

    //获取子窗口信息
    //static bool getSubWinInfo(int winEnum, SubWindowDim*& info, QByteArray*& otherInfo);

    //保存字窗口信息
    //static bool saveSubWinInfo(int winEnum, SubWindowDim* info, QByteArray* otherInfo = NULL);

    //获取子窗口信息
    static bool getSubWinInfo3(int winEnum, QByteArray*& ba);

    //保存字窗口信息
    static bool saveSubWinInfo3(int winEnum, QByteArray* otherInfo);
};

void transferDirection(const QHash<int, int> &sd, QHash<int, MoneyDirection> &dd);
void transferAntiDirection(const QHash<int, MoneyDirection> &sd, QHash<int, int> &dd);



/////////////////////////////////////////////////////////////
/**
 * @brief BackupUtil 账户文件备份实用类
 */
class BackupUtil
{
public:
    enum BackupReason{
        BR_UPGRADE = 1,     //账户升级
        BR_TRANSFERIN = 2   //账户转入
    };

    BackupUtil(QString srcDir="", QString bacDir="");
    bool backup(QString fileName, BackupReason reason);
    bool restore(QString& error);
    bool restore(QString fileName, BackupReason reason, QString& error);
    //QString _fondLastFile(QString fileName,BackupReason reason);
    QString _cutSuffix(QString fileName);
    void clear();
    void setBackupDirectory(QString path);
    void setSourceDirectory(QString path);
private:
    int _fondLastFile(QString fileName,BackupReason reason);
    void _loadBackupFiles();
    QString _getReasonTag(BackupReason reason);
    //QString _cutSuffix(QString fileName);
    //QString _getPrimaryFileName(QString fileName, QString suffix);
    bool _copyFile(QString sn,QString dn);

    QStringList files;
    QDir backDir,sorDir;
    QStack<QString> stk_sor,stk_back; //带路径的源文件目录、备份文件目录的堆栈
};

#endif // UTILS_H
