#ifndef BAWIDGETS_H
#define BAWIDGETS_H

#include <QTableWidgetItem>

#include "../subject.h"
#include "account.h"

//在QTableWidget中显示业务活动摘要部分内容的表格项
class BASummaryItem_new : public QTableWidgetItem
{
public:
    BASummaryItem_new(const QString data, SubjectManager* subMgr = NULL, int type = QTableWidgetItem::UserType);
    QTableWidgetItem *clone() const;
    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    void parse(QString data);
    QString assembleContent() const;
    QString genQuoteInfo() const;

    QString summary;
    QStringList fpNums; //发票号
    QString bankNums;   //银行票据号
    int oppoSubject;    //对方科目id

    SubjectManager* subManager;
};

//在QTableWidget中显示业务活动总账科目的表格项
class BAFstSubItem_new : public QTableWidgetItem
{
public:
    BAFstSubItem_new(FirstSubject* fsub, SubjectManager* subMgr, int type = QTableWidgetItem::UserType + 1);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    FirstSubject* fsub;
    SubjectManager* subManager;
};

//在QTableWidget中显示业务活动明细科目的表格项
class BASndSubItem_new : public QTableWidgetItem
{
public:
    BASndSubItem_new(SecondSubject* ssub, SubjectManager* subMgr, int type = QTableWidgetItem::UserType + 2);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    SecondSubject* ssub;
    SubjectManager* subMgr;
};

//在QTableWidget中显示币种的表格项
class BAMoneyTypeItem_new : public QTableWidgetItem
{
public:
    BAMoneyTypeItem_new(Money* mt, int type = QTableWidgetItem::UserType + 3);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    Money* mt;
};

//在QTableWidget中显示借贷金额值的表格项
class BAMoneyValueItem_new : public QTableWidgetItem
{
public:
    BAMoneyValueItem_new(int witch, double v = 0,
                     QColor forColor = QColor(Qt::black),
                     int type = QTableWidgetItem::UserType + 4):
        QTableWidgetItem(type),witch(witch),v(Double(v)),outCon(false),
        forColor(forColor){}
    BAMoneyValueItem_new(int witch, Double v = 0.00,
                     QColor forColor = QColor(Qt::black),
                     int type = QTableWidgetItem::UserType + 4):
        QTableWidgetItem(type),witch(witch),v(v),outCon(false),
        forColor(forColor){}
    void setOutControl(bool con){outCon = con;}
    void setForeColor(QColor color){forColor = color;}
    void setDir(int dir);
    QVariant data(int role) const;
    void setData(int role, const QVariant &value);
    void setData(int role, const Double &value);

private:
    int witch; //用于显示贷方还是借方（1：借，-1：贷）
    Double v;
    bool outCon;  //是否使用有外部设置的前景色
    QColor forColor;  //外部指定的前景色
};



#endif // BAWIDGETS_H
