#include "cal.h"

Money* MT_NULL;
Money* MT_ALL;

Double::Double()
{
    digs = 2;
    digRate = 100;
    lv = 0;
}

/**
 * @brief Double::Double
 *  精度只能是2位或4位
 * @param v
 * @param digit
 */
Double::Double(double v, int digit):digs(digit)
{
    if(digit == 4)
        digRate = 10000;
    else{
        digs=2;
        digRate = 100;
    }
    QString s;
    s.setNum(v*digRate,'f',0);
    lv = s.toLongLong();
}

Double::Double(qint64 v, int digit):lv(v),digs(digit)
{
    if(digit == 4)
        digRate = 10000;
    else{
        digs=2;
        digRate = 100;
    }
}

//Double::Double(const Double &other)
//{
//    digs = other.digs;
//    digRate = other.digRate;
//    lv = other.lv;
//}


/**
 * @brief Double::toString
 * @param zero：true：当值为零时输出“0”，否则用0填补不足的小数位
 *              false：当值为零时输出空字符串，否则截断末尾的零
 * @return
 */
QString Double::toString(bool zero) const
{

    if(lv == 0)
        return zero?"0":"";
    else{
        double v = (double)lv / digRate;
        if(zero)
            return QString::number(v,'f',digs);
        else{
            int num=0;
            for(int rate = digRate; rate > 1; rate/=10){
                if(lv % rate == 0)
                    return QString::number(v,'f',num);
                num++;
            }
            return QString::number(v,'f',num);
        }
    }
}

QString Double::toString2() const
{
    return toString(true);
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
    QString s;
    s.setNum(v*digRate,'f',0);
    lv = s.toLongLong();
}

Double Double::operator =(int v)
{
    digs = 2;
    digRate = 100;
    lv = v*100;
}

Double Double::operator +(const Double &other) const
{
    qint64 v;
    int rate = digs - other.getDig();
    if(rate == 0){
        v = getlv()+other.getlv();
        if(digs > 2)
            v = reduce(v,digs-2);
        return Double(v,digs);
    }
    else{
        int rates=1;
        int c = abs(rate);
        for(int i = 0; i < c; ++i)
            rates *= 10;
        if(rate > 0)
            v = getlv() + other.getlv()*rates;
        else
            v = lv*rates+other.getlv();
        v = reduce(v,c);
        return Double(v,2);
    }
}

Double Double::operator -(const Double &other) const
{
    qint64 v;
    int rate = digs - other.getDig();
    if(rate == 0){
        v = getlv()-other.getlv();
        if(digs > 2)
            v = reduce(v,digs-2);
        return Double(v,digs);
    }
    else{
        int rates=1;
        int c = abs(rate);
        for(int i = 0; i < c; ++i)
            rates *= 10;
        if(rate > 0)
            v = getlv() - other.getlv()*rates;
        else
            v = lv*rates - other.getlv();
        v = reduce(v,c);
        return Double(v,2);
    }
}

Double Double::operator *(const Double &other) const
{
    qint64 v;
    int rate = digs - other.getDig();
    if(rate == 0){
        v = getlv() * other.getlv();
    }
    else{
        int rates=1;
        int c = abs(rate);
        for(int i = 0; i < c; ++i)
            rates *= 10;
        if(rate > 0)
            v = getlv() * (other.getlv()*rates);
        else
            v = (lv*rates) * other.getlv();
    }

    v = reduce(v,max(digs,other.getDig())*2-2);
    return Double(v,2);

//    if(digs != other.getDig())
//        return Double((qint64)0,digs);
//    qint64 v = getlv() * other.getlv();
//    v = v/(digRate/10);
//    if((v % 10) > 4)
//        v = v/10 + 1;
//    else
//        v = v / 10;
//    return Double(v,digs);
}

/**
 * @brief Double::operator /
 *  为保留精度，被除数的小数位数必须比除数多3以上，这里为了尽可能保留精度，
 *  将被除数相对于除数扩大10000倍，这样可以将商保留4位小数
 * @param other
 * @return
 */
Double Double::operator /(const Double &other) const
{
    qint64 v,v1,v2;
    int rate = digs-other.getDig();
    v1=lv; v2=other.getlv();
    if(rate<4){
        for(int i = rate; i<4; ++i)
            v1*=10;
    }
    v = v1/v2;
    v = reduce(v,2);
    return Double(v,2);

//    if(digs != other.getDig())
//        return Double((qint64)0,digs);
//    double v = (double)getlv()/(double)other.getlv();
//    qint64 iv = v * digRate * 10;
//    if(iv%10>4)
//        iv = iv/10+1;
//    else
//        iv = iv/10;
//    return Double(iv,digs);
}

void Double::operator +=(const Double &other)
{
    int rate = digs - other.getDig();
    if(rate == 0){
        lv += other.getlv();
        if(digs > 2){
            lv = reduce(lv,digs-2);
            digs=2;
        }
    }
    else{
        int rates=1;
        int c = abs(rate);
        for(int i = 0; i < c; ++i)
            rates *= 10;
        if(rate > 0)
            lv += other.getlv()*rates;
        else{
            lv *= rates;
            lv += other.getlv();
        }
        lv = reduce(lv,c);
        digs = 2;
    }
//    if(digs != other.getDig())
//        lv = 0;
//    else
//        lv += other.getlv();
}

void Double::operator -=(const Double other)
{
    int rate = digs - other.getDig();
    if(rate == 0){
        lv -= other.getlv();
        if(digs > 2){
            lv = reduce(lv,digs-2);
            digs=2;
        }
    }
    else{
        int rates=1;
        int c = abs(rate);
        for(int i = 0; i < c; ++i)
            rates *= 10;
        if(rate > 0)
            lv -= other.getlv()*rates;
        else{
            lv *= rates;
            lv -= other.getlv();
        }
        lv = reduce(lv,c);
        digs = 2;
    }
//    if(digs != other.getDig())
//        lv = 0;
//    else
//        lv -= other.getlv();
}

void Double::operator *=(const Double other)
{

    lv *= other.getlv();
    lv = reduce(lv,digs+other.getDig()-2);
    digs = 2;
//    if(digs != other.getDig())
//        lv = 0;
//    else{
//        lv *= other.getlv();
//        lv = lv/(digRate/10);
//        if((lv % 10) > 4)
//            lv = lv/10 + 1;
//        else
//            lv = lv / 10;
//    }
}

void Double::operator /=(const Double other)
{
    int rate = digs-other.getDig();
    if(rate<4){
        for(int i = rate; i<4; ++i)
            lv*=10;
    }
    lv /= other.getlv();
    lv = reduce(lv,2);
    digs=2;
//    if(digs != other.getDig())
//        lv = 0;
//    else{
//        double v = (double)getlv()/(double)other.getlv();
//        lv = v * digRate * 10;
//        if(lv%10 > 4)
//            lv = lv/10+1;
//        else
//            lv = lv/10;
//    }
}

bool Double::operator ==(const Double &other) const
{
    int rate = digs - other.getDig();
    if(rate == 0)
        return lv == other.getlv();
    qint64 v1=lv,v2=other.getlv();
    adjustValue(rate,v1,v2);
    return v1==v2;
}

bool Double::operator ==(const int v) const
{
    return lv == v * digRate;
}

bool Double::operator !=(const Double &other) const
{
    int rate = digs - other.getDig();
    if(rate == 0)
        return lv != other.getlv();
    qint64 v1=lv,v2=other.getlv();
    adjustValue(rate,v1,v2);
    return v1!=v2;
}

bool Double::operator !=(const int v) const
{
    return lv != v * digRate;
}

bool Double::operator >(const Double &other)
{
    int rate = digs - other.getDig();
    if(rate == 0)
        return lv > other.getlv();
    qint64 v1=lv,v2=other.getlv();
    adjustValue(rate,v1,v2);
    return v1>v2;
}

bool Double::operator >(const int v) const
{
    return lv > v * digRate;
}

bool Double::operator <(const Double &other) const
{
    int rate = digs - other.getDig();
    if(rate == 0)
        return lv < other.getlv();
    qint64 v1=lv,v2=other.getlv();
    adjustValue(rate,v1,v2);
    return v1<v2;
}
bool Double::operator <(const int v)
{
    return lv < v * digRate;
}

bool Double::operator >=(const Double &other) const
{
    int rate = digs - other.getDig();
    if(rate == 0)
        return lv >= other.getlv();
    qint64 v1=lv,v2=other.getlv();
    adjustValue(rate,v1,v2);
    return v1>=v2;
}

bool Double::operator >=(const int v) const
{
    return lv >= v * digRate;
}

bool Double::operator <=(const Double &other) const
{
    int rate = digs - other.getDig();
    if(rate == 0)
        return lv <= other.getlv();
    qint64 v1=lv,v2=other.getlv();
    adjustValue(rate,v1,v2);
    return v1<=v2;
}

bool Double::operator <=(const int v) const
{
    return lv <= v * digRate;
}

/**
 * @brief Double::reduce
 *  按四舍五入的规则，缩小sv的值（小数点向左移动rate位）
 * @param sv
 * @param rate
 * @return
 */
qint64 Double::reduce(qint64 sv, int rate)
{

    if(sv == 0 || rate < 0)
        return sv;
    int yd,rates=1;
    for(int i = 1; i <= rate; ++i)
        rates *= 10;
    int rates2 = rates/10;
    if(rates2==0)
        rates2=1;
    qint64 v = sv/rates;
    yd = abs((sv/rates2)%10);
    if(yd > 4){
        if(v>=0)
            v++;
        else
            v--;
    }
    return v;
}

/**
 * @brief Double::adjustValue
 *  将值调整到相同的精度表示
 * @param rate：精度相差位数（正表示前一个数的精度高于后一个数）
 * @param v1
 * @param v2
 */
void Double::adjustValue(int rate, qint64 &v1, qint64 &v2) const
{
    int rates=1;
    for(int i = 0; i < abs(rate); ++i)
        rates *= 10;
    if(rate>0)
        v2 *= rates;
    else
        v1 *= rates;
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

