#ifndef CAL_H
#define CAL_H

#include <QString>
#include <QMetaType>

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
Q_DECLARE_METATYPE(Double)

//货币类
class Money{
public:
    Money():m_code(0){}
    Money(const Money& other);
    Money(int mt, QString name, QString sign):
        m_code(mt),m_name(name),m_sign(sign){}
    ~Money(){}

    int code(){return m_code;}
    QString name(){return m_name;}
    void setName(QString name){m_name=name;}
    QString sign(){return m_sign;}
    void setSign(QString sign){m_sign=sign;}

    //下面这两个获取和保存汇率方法静态方法有多余之嫌，因为Account类已提供了相同的方法
//    static bool getRate(int y,int m, QHash<int, Double> &rates,
//                        QSqlDatabase db = QSqlDatabase::database());
//    static bool saveRate(int y,int m, QHash<int,Double> rates,
//                        QSqlDatabase db = QSqlDatabase::database());
//    static bool getAllMts(QHash<int, Money *> &mts,
//                          QSqlDatabase db = QSqlDatabase::database());
//    static bool getMtName(int code,QString &name,
//                             QSqlDatabase db = QSqlDatabase::database());

private:
    int  m_code;             //货币代码
    QString m_name;          //货币名称
    QString m_sign;          //货币符号
};

//Q_DECLARE_METATYPE(Money);
Q_DECLARE_METATYPE(Money*)

bool byMoneyCodeGreatthan(Money* m1, Money* m2);
extern Money* MT_NULL;     //空币种
extern Money* MT_ALL;      //所有币种

#endif // CAL_H
