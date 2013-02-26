#include <QSqlQuery>
#include <QVariant>
#include <QDate>

#include "tem.h"
#include "cal.h"
#include "common.h"

//将指定年月的指定主科目下金额为美元的会计分录转换为人民币
void tranUsdToRbm(int y,int m,int fid,QSqlDatabase db)
{
    QSqlQuery q(db),q1(db),q2(db);
    Double rate;
    QString s = QString("select usd2rmb from ExchangeRates where (year=%1) and (month=%2)")
            .arg(y).arg(m);
    if(!q.exec(s))
        return;
    if(!q.first())
        return;
    rate = Double(q.value(0).toDouble());

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select id from PingZhengs where date like '%1%'").arg(ds);
    if(!q.exec(s))
        return;
    int pid;
    while(q.next()){
        pid = q.value(0).toInt();
        s = QString("select id,dir,jMoney,dMoney from BusiActions where (pid=%1) and (firSubID=%2) and (moneyType=2)")
                .arg(pid).arg(fid);
        if(!q1.exec(s))
            return;
        if(!q1.first())
            continue;
        int id = q1.value(0).toInt();
        int dir = q1.value(1).toInt();
        Double v;
        if(dir == DIR_J){
            v = Double(q1.value(2).toDouble()) * rate;
            s = QString("update BusiActions set jMoney=%1,moneyType=1 where id=%2")
                    .arg(v.toString()).arg(id);
            if(!q1.exec(s))
                return;

        }
        else if(dir == DIR_D){
            v = Double(q1.value(3).toDouble()) * rate;
            s = QString("update BusiActions set dMoney=%1,moneyType=1 where id=%2")
                    .arg(v.toString()).arg(id);
            if(!q1.exec(s))
                return;
        }
    }
}

void delSpecPidPz(int pid)
{

}
