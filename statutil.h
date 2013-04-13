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

    //获取期初值和方向
    //获取本期发生额
    //获取期末余额及其方向


private:
    void _clearDatas();
    void _statCurHappen();

    Account* account;
    DbUtil* dbUtil;
    SubjectManager* smg;
    QList<PingZheng*> pzs;  //凭证对象集合
    int y,m;                //凭证集所属年月
    Money* masterMt;

    QHash<int,Double> preExa, preDetExa;                     //期初余额（以原币计）
    QHash<int,Double> preExaR, preDetExaR;                   //期初余额（以本币计）
    QHash<int,MoneyDirection> preExaDir,preDetExaDir;        //期初余额方向（以原币计）
    //QHash<int,MoneyDirection> preExaDirR,preDetExaDirR;     //期初余额方向（以本币计）
    QHash<int,Double> curJHpn, curJDHpn, curDHpn, curDDHpn;  //当期借贷发生额（以原币计）
    QHash<int,Double> curJHpnR,curJDHpnR,curDHpnR,curDDHpnR; //当期借贷发生额（以本币计）
    QHash<int,Double> endExa, endDetExa;                     //期末余额（以原币计）
    QHash<int,Double> endExaR, endDetExaR;                   //期末余额（以本币计）
    QHash<int,MoneyDirection> endExaDir,endDetExaDir;        //期末余额方向（以原币计）
    //QHash<int,MoneyDirection> endExaDirR,endDetExaDirR;      //期末余额方向（以本币计）

};

#endif // STATUTIL_H
