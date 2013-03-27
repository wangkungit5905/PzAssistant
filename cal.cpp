#include "cal.h"

Money* MT_NULL;
Money* MT_ALL;

Double::Double()
{
    digs = 2;
    digRate = 100;
    lv = 0;
}

Double::Double(double v, int digit):digs(digit)
{
    digRate = 1;
    for(int i = 0; i < digit; ++i)
        digRate *= 10;
    QString s;
    s.setNum(v*digRate,'f',0);
    lv = s.toLongLong();
}

Double::Double(qint64 v, int digit):lv(v),digs(digit)
{
    digRate = 1;
    for(int i = 0; i < digit; ++i)
        digRate *= 10;
}

//Double::Double(const Double &other)
//{
//    digs = other.getDig();
//    digRate = other.getDigRate();
//    lv = other.getlv();
//}

QString Double::toString() const
{
    if(lv == 0)
        return "";
    double v = (double)lv / digRate;
    if(lv % 100 == 0)
        return QString::number(v,'f',0);
    else if(lv % 10 == 0)
        return QString::number(v,'f',1);
    return QString::number(v,'f',2);
}

//变更符号
void Double::changeSign()
{
    lv = -lv;
}

Double Double::operator =(double v)
{
    digs = 2;
    digRate = 100;
    lv = v*1000;
    if(lv%10 > 4){
        lv/=10;
        lv++;
    }
    else
        lv/=10;
}

Double Double::operator =(int v)
{
    digs = 2;
    digRate = 100;
    lv = v*100;
}

Double Double::operator +(const Double &other) const
{
    if(digs != other.getDig())
        return Double((qint64)0,digs);
    qint64 v = getlv()+other.getlv();
    return Double(v,digs);
}

Double Double::operator -(const Double &other) const
{
    if(digs != other.getDig())
        return Double((qint64)0,digs);
    return Double(getlv()-other.getlv(),digs);
}

Double Double::operator *(const Double &other) const
{
    if(digs != other.getDig())
        return Double((qint64)0,digs);
    qint64 v = getlv() * other.getlv();
    v = v/(digRate/10);
    if((v % 10) > 4)
        v = v/10 + 1;
    else
        v = v / 10;
    return Double(v,digs);
}

Double Double::operator /(const Double &other) const
{
    if(digs != other.getDig())
        return Double((qint64)0,digs);
    double v = (double)getlv()/(double)other.getlv();
    qint64 iv = v * digRate * 10;
    if(iv%10>4)
        iv = iv/10+1;
    else
        iv = iv/10;
    return Double(iv,digs);
}

void Double::operator +=(const Double &other)
{
    if(digs != other.getDig())
        lv = 0;
    else
        lv += other.getlv();
}

void Double::operator -=(const Double other)
{
    if(digs != other.getDig())
        lv = 0;
    else
        lv -= other.getlv();
}

void Double::operator *=(const Double other)
{
    if(digs != other.getDig())
        lv = 0;
    else{
        lv *= other.getlv();
        lv = lv/(digRate/10);
        if((lv % 10) > 4)
            lv = lv/10 + 1;
        else
            lv = lv / 10;
    }
}

void Double::operator /=(const Double other)
{
    if(digs != other.getDig())
        lv = 0;
    else{
        double v = (double)getlv()/(double)other.getlv();
        lv = v * digRate * 10;
        if(lv%10 > 4)
            lv = lv/10+1;
        else
            lv = lv/10;
    }
}

bool Double::operator ==(const Double &other) const
{
    return (digs == other.getDig()) &&
           (lv == other.getlv());
}

bool Double::operator ==(const int v) const
{
    return lv == v * digRate;
}

bool Double::operator !=(const Double &other) const
{
    return (digs == other.getDig()) &&
            (lv != other.getlv());
}

bool Double::operator !=(const int v) const
{
    return lv != v * digRate;
}

bool Double::operator >(const Double &other)
{
    if(digs != other.getDig())
        return false;
    return lv > other.getlv();
}

bool Double::operator >(const int v) const
{
    return lv > v * digRate;
}

bool Double::operator <(const Double &other) const
{
    if(digs != other.getDig())
        return false;
    return lv < other.getlv();
}
bool Double::operator <(const int v)
{
    return lv < v * digRate;
}

bool Double::operator >=(const Double &other) const
{
    if(digs != other.getDig())
        return false;
    return lv >= other.getlv();
}

bool Double::operator >=(const int v) const
{
    return lv >= v * digRate;
}

bool Double::operator <=(const Double &other) const
{
    if(digs != other.getDig())
        return false;
    return lv <= other.getlv();
}

bool Double::operator <=(const int v) const
{
    return lv <= v * digRate;
}


///////////////////////Money class/////////////////////////////////////
Money::Money(const Money &other)
{
    m_code = other.m_code;
    m_name = other.m_name;
    m_sign = other.m_sign;
}

bool byMoneyCodeGreatthan(Money* m1, Money* m2)
{
    return m1->code() > m2->code();
}


//bool Money::getRate(int y,int m, QHash<int,Double> &rates, QSqlDatabase db)
//{
//    QSqlQuery q(db);
//    QString s;

//    s = QString("select * from %1 where code!=%2")
//            .arg(tbl_mt).arg(RMB);
//    if(!q.exec(s))
//        return false;
//    QHash<int,QString> msHash; //货币代码到货币符号的映射
//    while(q.next()){
//        msHash[q.value(MT_CODE).toInt()] = q.value(MT_SIGN).toString();
//    }

//    s = QString("select * from %1 where (%2=%3) and (%4=%5)")
//            .arg(tbl_exchangeRate).arg(fld_exrate_year).arg(y)
//            .arg(fld_exrate_month).arg(m);
//    if(!q.exec(s) || !q.first())
//        return false;
//    QSqlRecord record = q.record();
//    QHashIterator<int,QString> it(msHash);
//    while(it.hasNext()){
//        it.next();
//        QString fldName = it.value()+"2rmb";
//        int idx = record.indexOf(fldName);
//        Double rate = Double(q.value(idx).toDouble());
//        rates[it.key()] = rate;
//    }
//    return true;
//}

//bool Money::saveRate(int y, int m, QHash<int, Double> rates, QSqlDatabase db)
//{
//    QSqlQuery q(db);
//    QString s;

//    s = QString("select * from %1 where code!=%2")
//            .arg(tbl_mt).arg(RMB);
//    if(!q.exec(s))
//        return false;
//    QHash<int,QString> msHash; //货币代码到货币符号的映射
//    while(q.next())
//        msHash[q.value(MT_CODE).toInt()] = q.value(MT_SIGN).toString();

//    s = QString("update %1 set ").arg(tbl_exchangeRate);
//    QHashIterator<int,QString> it(msHash);
//    while(it.hasNext()){
//        it.next();
//        Double rate = rates.value(it.key());
//        QString fldName = it.value()+"2rmb";
//        s.append(fldName.append(QString("=%1").arg(rate.getv())));
//        s.append(QString(" where (%1=%2) and (%3=%4)").arg(fld_exrate_year)
//                 .arg(y).arg(fld_exrate_month).arg(m));
//        if(!q.exec(s))
//            return false;
//    }
//    return true;
//}

//获取所有应用支持的币种（账户数据库表“MoneyTypes”中的每种货币都是使用的）
//参数db是基本库连接
//bool Money::getAllMts(QHash<int, Money *> &mts, QSqlDatabase db)
//{
//    QSqlQuery q(db);
//    QString s;

//    s = QString("select * from MoneyTypes");
//    if(!q.exec(s))
//        return false;
//    while(q.next()){
//        int code = q.value(MT_CODE).toInt();
//        QString sign = q.value(MT_SIGN).toString();
//        QString name = q.value(MT_NAME).toString();
//        mts[code] = new Money(code,name,sign);
//    }
//    return true;
//}

//获取指定币种代码的币种名称
//bool Money::getMtName(int code, QString &name, QSqlDatabase db)
//{
//    QSqlQuery q(db);
//    QString s;

//    s = QString("select %1 from %2 where %3=%4").arg(fld_mt_name)
//            .arg(tbl_mt).arg(fld_mt_code).arg(code);
//    if(!q.exec(s))
//        return false;
//    if(!q.first())
//        return false;
//    name = q.value(0).toString();
//    return true;
//}

