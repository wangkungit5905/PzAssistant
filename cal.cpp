#include "cal.h"

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
