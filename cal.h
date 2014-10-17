#ifndef CAL_H
#define CAL_H

#include <QString>
#include <QMetaType>

/**
 * @brief The Double class
 * 满足了会计数值计算所要求的精度的实用类
 * 实现思路是使用大整数来模拟浮点数（小数位在2位（表示普通数值或计算结果）或4位（表示汇率））
 * Double类对于运算符有如下约定：
 * 参予操作的两个数，精度（即保留小数位,约定是2位或4位）可以不同，对于乘除运算的结果值将只保留2位小数，
 * 加减运算，结果值的精度是两个操作数中的较大者。
 * 这些约定是因为在实际的会计运算中要求，汇率要求用4位精度，为了准确计算汇差，加减运算的结果必须是4位。
 * 而币值的转换是2位的原币值与4位的汇率的乘除运算结果，为了保证运算准确性的一致，要求中间计算结果必须
 * 使用2位精度。因为，在日常求合计值时，参予合计的各项数据的精度都要求与最终的结果精度相一致。
 */
class Double
{
public:
    Double();
    Double(double v, int digit = 2);
    Double(qint64 v, int digit = 2);
    //Double(const Double &other);

    QString toString(bool zero=false) const;
    QString toString2() const;
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
    static qint64 reduce(qint64 sv, int rate);
private:
    int max(const int x, const int y) const {return x>=y?x:y;}
    void adjustValue(int rate, qint64 &v1, qint64 &v2) const;

    qint64 lv;    //用于表示浮点数的大整数
    int digs;     //小数位数
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
