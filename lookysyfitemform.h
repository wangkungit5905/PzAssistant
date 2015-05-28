#ifndef LOOKYSYFITEMFORM_H
#define LOOKYSYFITEMFORM_H

#include <QWidget>
#include <QSqlDatabase>
#include <QTimer>
#include <QItemDelegate>

namespace Ui {
class LookYsYfItemForm;
}

class QShortcut;
class Account;
class PzDialog;
class FirstSubject;
class SecondSubject;
class Money;


class IntItemDelegate : public QItemDelegate
{
public:
    enum ColumnIndex{
        CI_JOIN =   0,
        CI_YEAR =   1,
        CI_SMONTH = 2,
        CI_EMONTH = 3
    };

    IntItemDelegate(Account* account, QWidget* parent=0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
private:
    Account *_account;
};

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
    void leaveEvent(QEvent * event);
    void enterEvent(QEvent * event);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

public slots:
    void show();
    void findItem(FirstSubject* fsub, SecondSubject* ssub, QHash<int,QList<int> >timeRange, QList<QStringList> invoiceNums);

private slots:
    void closeWindow();
    void flickerIcon();
    void catchMouse();

    void on_btnSearch_clicked();

    void on_btnQuit_clicked();

private:
    void _search();

    Ui::LookYsYfItemForm *ui;
    QPoint mousePoint;              //鼠标拖动自定义标题栏时的坐标
    bool mousePressed;              //鼠标是否按下
    QAction* actQuit;      //
    bool isNormal;          //
    PzDialog* parent;       //
    Account* account;
    QSqlDatabase db;
    FirstSubject* _fsub;
    SecondSubject* _ssub;
    QStringList _invoiceNums;
    QHash<int,Money*> mtTypes;
    QTimer _flickeTimer,_catchTimer;
    QPixmap _iconPix;
    QShortcut* sc_look;
};

#endif // LOOKYSYFITEMFORM_H
