#ifndef LOOKYSYFITEMFORM_H
#define LOOKYSYFITEMFORM_H

#include <QWidget>
#include <QSqlDatabase>

namespace Ui {
class LookYsYfItemForm;
}

class Account;
class PzDialog;
class FirstSubject;
class SecondSubject;
class Money;

class LookYsYfItemForm : public QWidget
{
    Q_OBJECT

    enum TableIndex{
        TI_DATE = 0,
        TI_IVNUM = 1,
        TI_MONEY = 2,
        TI_VALUE = 3,
        TI_WVALUE = 4
    };

public:
    explicit LookYsYfItemForm(Account* account, QWidget *parent=0);
    ~LookYsYfItemForm();

protected:
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent * event);

public slots:
    void show();
    void findItem(FirstSubject* fsub, SecondSubject* ssub, QHash<int,QList<int> >timeRange, QList<QStringList> invoiceNums);

private slots:
    void closeWindow();
    void yearChanged(int index);
    void monthChanged(int m);

    void on_btnMin_clicked();

    void on_btnSearch_clicked();

private:
    void _search();
    void _turnOn(bool on = true);

    Ui::LookYsYfItemForm *ui;
    QPoint mousePoint;              //鼠标拖动自定义标题栏时的坐标
    bool mousePressed;              //鼠标是否按下
    QAction* actClose;      //
    bool isNormal;          //
    PzDialog* parent;       //
    Account* account;
    QSqlDatabase db;
    FirstSubject* _fsub;
    SecondSubject* _ssub;
    QList<int> _range;   //搜索时间范围（3位一组，依次是年份，开始月份，结束月份）
    QStringList _invoiceNums;
    QHash<int,Money*> mtTypes;
};

#endif // LOOKYSYFITEMFORM_H
