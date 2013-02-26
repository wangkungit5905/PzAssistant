#ifndef CAL_H
#define CAL_H

#include <QString>


class Double
{
public:
    Double();
    Double(double v, int digit = 2);
    Double(qint64 v, int digit = 2);
    //Double(const Double &other);

    QString toString() const;
    int getDig() const {return digs;}
    int getDigRate() const {return digRate;}
    double getv() const {return (double)lv / digRate;}
    qint64 getlv() const {return lv;}
    void changeSign();
    bool isZero() const {return lv == 0;}

    Double operator =(double v);
    Double operator =(int v);
    Double operator +(const Double &other) const;
    Double operator -(const Double &other) const;
    Double operator *(const Double &other) const;
    Double operator /(const Double &other) const;
    void operator +=(const Double &other);
    void operator -=(const Double other);
    void operator *=(const Double other);
    void operator /=(const Double other);

    bool operator ==(const Double &other) const;
    bool operator ==(const int v) const;
    bool operator !=(const Double &other) const;
    bool operator !=(const int v) const;
    bool operator >(const Double &other);
    bool operator >(const int v) const;
    bool operator <(const Double &other) const;
    bool operator <(const int v);
    bool operator >=(const Double &other) const;
    bool operator >=(const int v) const;
    bool operator <=(const Double &other) const;
    bool operator <=(const int v) const;

private:
    qint64 lv;
    int digs; //小数位数
    int digRate;  //与小数位数对应的10的倍数（比如2位即100）
};

#endif // CAL_H
