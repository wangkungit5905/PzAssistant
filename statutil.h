#ifndef STATUTIL_H
#define STATUTIL_H

#include <QList>

#include "commdatastruct.h"

class PingZheng;
class Account;
class DbUtil;
class SubjectManager;


/**
 * @brief The StatUtil class
 *  负债对凭证集进行统计
 */
class StatUtil
{
public:
    StatUtil(QList<PingZheng*> &pzs,Account* account);
    bool stat();
    bool save();

    //获取期初值和方向
    QHash<int,Double>& getPreValueFPm(){return preFExa;}
    QHash<int,Double>& getPreValueFMm(){return preFExaR;}
    QHash<int,Double>& getPreValueSPm(){return preSExa;}
    QHash<int,Double>& getPreValueSMm(){return preSExaR;}
    QHash<int,MoneyDirection>& getPreDirF(){return preFDir;}
    QHash<int,MoneyDirection>& getPreDirS(){return preSDir;}

    //获取本期发生额
    QHash<int,Double>& getCurValueJFPm(){return curJF;}
    QHash<int,Double>& getCurValueJFMm(){return curJFR;}
    QHash<int,Double>& getCurValueDFPm(){return curDF;}
    QHash<int,Double>& getCurValueDFMm(){return curDFR;}
    QHash<int,Double>& getCurValueJSPm(){return curJS;}
    QHash<int,Double>& getCurValueJSMm(){return curJSR;}
    QHash<int,Double>& getCurValueDSPm(){return curDS;}
    QHash<int,Double>& getCurValueDSMm(){return curDSR;}

   //获取期末余额及其方向
    QHash<int,Double>& getEndValueFPm(){return endFExa;}
    QHash<int,Double>& getEndValueFMm(){return endFExaR;}
    QHash<int,Double>& getEndValueSPm(){return endSExa;}
    QHash<int,Double>& getEndValueSMm(){return endSExaR;}
    QHash<int,MoneyDirection>& getEndDirF(){return endFDir;}
    QHash<int,MoneyDirection>& getEndDirS(){return endSDir;}



private:
    void _clearDatas();
    bool _statCurHappen();
    bool _readPreExtra();
    void _calEndExtra(bool isFst=true);

    Account* account;
    DbUtil* dbUtil;
    SubjectManager* smg;
    QList<PingZheng*> pzs;  //凭证对象集合
    int y,m;                //凭证集所属年月
    Money* masterMt;
    QHash<int,Double> rates;


    //命名约定：J：借方，D：贷方，F：一级科目，S：二级科目，R：表示以本币计
    //pre：期初，end：期末，Exa：余额，Dir：访问
    QHash<int,Double> preFExa, preSExa;                     //期初余额（以原币计）
    QHash<int,Double> preFExaR, preSExaR;                   //期初余额（以本币计）
    QHash<int,MoneyDirection> preFDir,preSDir;        //期初余额方向（以原币计）
    //QHash<int,MoneyDirection> preExaDirR,preDetExaDirR;     //期初余额方向（以本币计）
    QHash<int,Double> curJF, curJS, curDF, curDS;  //当期借贷发生额（以原币计）
    QHash<int,Double> curJFR,curJSR,curDFR,curDSR; //当期借贷发生额（以本币计）（仅用来保存涉及到外币的发生额）
    QHash<int,Double> endFExa, endSExa;                     //期末余额（以原币计）
    QHash<int,Double> endFExaR, endSExaR;                   //期末余额（以本币计）
    QHash<int,MoneyDirection> endFDir,endSDir;        //期末余额方向（以原币计）
    //QHash<int,MoneyDirection> endExaDirR,endDetExaDirR;      //期末余额方向（以本币计）

};

#endif // STATUTIL_H
