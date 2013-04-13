#include <QDate>

#include "statutil.h"
#include "account.h"
#include "pz.h"

StatUtil::StatUtil(QList<PingZheng *> &pzs, Account *account):pzs(pzs),account(account)
{
    dbUtil = account->getDbUtil();
    if(pzs.isEmpty())
        return;
    PingZheng* pz = pzs.first();
    y = pz->getDate2().year();
    m = pz->getDate2().month();
    smg = account->getSubjectManager(account->getSuite(y)->subSys);
    masterMt = account->getMasterMt();
}

/**
 * @brief StatUtil::_clearDatas
 *  清除所有hash表内的数据
 */
void StatUtil::_clearDatas()
{
    preExa.clear(); preDetExa.clear();
    preExaR.clear();preDetExaR.clear();
    preExaDir.clear(); preDetExaDir.clear();

    curJHpn.clear();curJDHpn.clear();
    curJHpnR.clear();curJDHpnR.clear();
    curDHpn.clear();curDDHpn.clear();
    curDHpnR.clear();curDDHpnR.clear();

    endExa.clear();endDetExa.clear();
    endExaR.clear();endDetExaR.clear();
    endExaDir.clear();endDetExaDir.clear();
}

/**
 * @brief StatUtil::_statCurHappen
 *  统计本期发生额
 */
void StatUtil::_statCurHappen()
{
    PingZheng* pz;
    BusiAction* ba;

    for(int i = 0; i < pzs.count(); ++i){
        pz = pzs.at(i);

        for(int j = 0; j < pz->baCount(); ++j){
            ba = pz->getBusiAction(j);

        }
    }
}
