#ifndef WIDGETS_H
#define WIDGETS_H

#include <QTableView>
#include <QTreeView>
#include <QCheckBox>
#include <QTableWidgetItem>
#include <QMdiSubWindow>
#include <QSpinBox>
#include <QCompleter>
#include <QComboBox>
#include <QSqlQueryModel>
#include <QStandardItem>
#include <QPushButton>

#include "commdatastruct.h"
#include "common.h"

//#include "cal.h"

#define INITROWS 50  //显示业务活动的表格的初始空白行数


class SubjectManager;

//显示和编辑凭证内的业务活动列表的表格视图类
class ActionEditTableView : public QTableView
{
    Q_OBJECT
public:
    ActionEditTableView(QWidget* parent = 0);

protected:
    void keyPressEvent(QKeyEvent* event);
};

//以0和1表示选取状态的组合选取框类（QCheckBox类在用QDataWidgetMapper与数据库建立映射后，是用文本来表示真假值的）
class CustomCheckBox : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(int checkstate READ getState WRITE setState USER true)
public:
    CustomCheckBox(QWidget* parent = 0);
    CustomCheckBox(const QString &text, QWidget *parent = 0);
    int getState();
    void setState(int state);

};


//实现打印功能的表格视图类
class PrintView2 : public QTableView
{
    Q_OBJECT

public:
    PrintView2();

public Q_SLOTS:
    void print(QPrinter *printer);
};

//支持外部验证器的表格项目类
class ValidableTableWidgetItem : public QTableWidgetItem
{

public:
    ValidableTableWidgetItem(QValidator* validator, int type = Type);
    ValidableTableWidgetItem(const QString & text, QValidator* validator, int type = Type );
    //void setText(const QString &text);
    void setData(int role, const QVariant& value);
    QVariant data(int role) const;

private:
    QValidator* validator;
    QString strData;

};

//支持子窗口关闭事件信号MDI子窗口类
class MyMdiSubWindow : public QMdiSubWindow
{
    Q_OBJECT
public:
    MyMdiSubWindow(QWidget* parent = 0):QMdiSubWindow(parent){}
    //~MyMdiSubWindow();
public slots:
    //void centrlWidgetClosed();
signals:
    void windowClosed(QMdiSubWindow* subWin);
protected:
    void closeEvent(QCloseEvent *closeEvent);

};

//在QTableWidget中显示业务活动摘要部分内容的表格项
class BASummaryItem : public QTableWidgetItem
{
public:
    BASummaryItem(const QString data, SubjectManager* subMgr = NULL, int type = QTableWidgetItem::UserType);
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
class BAFstSubItem : public QTableWidgetItem
{
public:
    BAFstSubItem(int subId, SubjectManager* subMgr, int type = QTableWidgetItem::UserType + 1);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    int subId;
    SubjectManager* subManager;
};

//在QTableWidget中显示业务活动明细科目的表格项
class BASndSubItem : public QTableWidgetItem
{
public:
    BASndSubItem(int subId, QHash<int,QString>* subNames, QHash<int,QString>* subLNames,
                 int type = QTableWidgetItem::UserType + 2);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    int subId;
    QHash<int,QString>* subNames;
    QHash<int,QString>* subLNames;
    QHash<int,BankAccount*> bankAccounts;  //银行子目到账号，键为明细科目id
};

//在QTableWidget中显示币种的表格项
class BAMoneyTypeItem : public QTableWidgetItem
{
public:
    BAMoneyTypeItem(int mt, QHash<int, QString>* mts, int type = QTableWidgetItem::UserType + 3);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    int mt;
    QHash<int, QString>* mts; //币种的代码名称映射表
};


//在QTableWidget中显示借贷金额值的表格项
class BAMoneyValueItem : public QTableWidgetItem
{
public:
    BAMoneyValueItem(int witch, double v = 0, int type = QTableWidgetItem::UserType + 4):
        QTableWidgetItem(type),witch(witch),v(Double(v)){}
    BAMoneyValueItem(int witch, Double v = 0.00, int type = QTableWidgetItem::UserType + 4):
        QTableWidgetItem(type),witch(witch),v(v.getv()){}
    void setDir(int dir);
    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    int witch; //用于显示贷方还是借方（1：借，0：贷）
    Double v;
};

//用钩子显示是否打过标记的表格项目类
class tagItem : public QTableWidgetItem
{
public:
    tagItem(bool isTag = false, int type = QTableWidgetItem::UserType);
    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

    void setTagIcon(QIcon icon){tagIcon = icon;}
    void setNotTagIcon(QIcon icon){notTagIcon = icon;}

private:
    bool tag;
    QIcon tagIcon,notTagIcon;
    QString desc;
};

//编辑业务活动列表的自定义表格部件视图类
class ActionEditTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    ActionEditTableWidget(QWidget* parent = 0);    
    void switchRow(int r1,int r2);
    bool isCurRowVolid();
    //bool isRailRow();
    bool hasSelectedRow();
    void selectedRows(QList<int>& rows, bool& isContinue);
    void appendRow(int rowNums = 1);
    void insertRows(int pos, int rowNums = 1);
    void removeRows(int pos, int rowNums = 1);
protected:
    void keyPressEvent(QKeyEvent* e);
    void mousePressEvent(QMouseEvent* event);
signals:
    //void requestCrtNewAction(bool dir);
    void requestCrtNewOppoAction();            //请求创建新的合计对冲业务活动
    void reqAutoCopyToCurAction();              //请求复制当前的业务活动并插入到当前行的下一行
    void requestAppendNewAction(int row);      //请求添加新的空业务活动
    void requestContextMenu(int row, int col); //请求对上下文菜单进行刷新

private slots:
    void newSndSubMapping(int pid, int sid, int row, int col, bool reqConfirm = true);
    void newSndSubAndMapping(int fid, QString name, int row, int col);
    void sndSubjectDisabeld(int id);
    //void cellClicked(int row, int column);
    void currentCellChanged(int currentRow, int curCol, int previousRow, int previousColumn);


private:
    int volidRows;   //有效行数（那些处于表格上端的显示业务活动的行（包括表尾到备用空白行）才是有效行，其余空白的行都是无效的行）
    int curRow;     //当前单元格的行和列
    int curCol;
    bool rowTag;      //折行标志
};

//用于显示借贷方向的表格项类
class DirItem : public QTableWidgetItem
{
public:
    DirItem(int dir, int type = QTableWidgetItem::UserType + 6);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    int dir; //（1：借，-1：贷，0：平）
};


//只是为了捕获用户按下回车键时产生的编辑完成事件，而在鄙弃由于失去焦点时也产生编辑完成事件这个多余的动作
class CustomSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    CustomSpinBox(QWidget* parent = 0);

protected:
    void keyPressEvent(QKeyEvent * event);

signals:
    void userEditingFinished();
};



//专用于输入科目的完成器类（输入科目代码或科目名的助记符，弹出完成列表帮助用户完成科目名称的输入）
//自动根据第一个输入字符是数字还是英文字母，来建立对应的完成串列表。它专与QComboBox类一起使用。
//完成器内部是直接从表FirSubjects和SecSubjects表中提取数据，还可以为它们设置过滤条件
//为保证完成器选择列表与组合框的可选项的一致性，在装载组合框的选项时必须考虑。
class SubjectComplete  : public QCompleter
{
    Q_OBJECT
public:
    SubjectComplete(SujectLevel witch = FstSubject, QObject *parent = 0);
    void setPid(int pid);
    void setFilter(QString strFlt);

protected:
    QString pathFromIndex(const QModelIndex &index) const;
    bool eventFilter(QObject *obj, QEvent * e);

private slots:
    void clickedInList(const QModelIndex &index);

private:    
    QTreeView tv;       //用于显示候选词列表
    QSqlQueryModel m;   //完成器使用的模型
    SujectLevel witch;  //服务的科目级别（1：一级科目，2：二级科目）
    int pid;            //当服务于二级科目时，表示二级科目的父科目的id
    QString keyBuf;     //键入字符缓存
    QString filter;     //过滤子句（针对FirSubjects表）
};

//支持二行表格标题行的表格类（通常是第一行的某些列需要跨越多个原始的表格列）
//基本实现方法是在表格视图（数据表格）中内嵌一个表格（标题表格），且这两个表格具有相同的列数和列的可见性
class MultiRowHeaderTableView : public QTableView
{
public:
    MultiRowHeaderTableView(QAbstractItemModel *model, QAbstractItemModel *hmodel, QWidget *parent = 0);
    ~MultiRowHeaderTableView();

private:
    void init();
    void updateHeadTableGeometry();

    QTableView *headView; //作为数据表格的标题

};

//用于明细账/日记账表格内的数据项目类
class ApStandardItem : public QStandardItem
{
public:
    ApStandardItem(bool editable = false);
    ApStandardItem(Double v,Qt::Alignment align = Qt::AlignCenter,bool editable = false);
    ApStandardItem(QString text, Qt::Alignment align = Qt::AlignCenter,bool editable = false);
    ApStandardItem(int v, Qt::Alignment align = Qt::AlignCenter,bool editable = false);
    ApStandardItem(double v, int decimal = 2,
                   Qt::Alignment align = Qt::AlignRight|Qt::AlignVCenter,bool editable = false);
};


//支持默认值的特别显示，QSpinBox只支持最小值的特别显示
class ApSpinBox :public QSpinBox
{
    Q_OBJECT
public:
    ApSpinBox(QWidget* parent = 0);
    void setDefaultValue(int v, QString t);

protected:
    QString textFromValue(int value) const;
    //int valueFromText(const QString& text) const;


private:
    int defValue;  //默认值
    QString defText; //默认值对应的文本
};

//支持按钮动作意图信号的按钮类
class IntentButton : public QPushButton
{
    Q_OBJECT
public:
    IntentButton(int intent, QString text, QWidget* parent = 0)
        :QPushButton(text,parent),
        intent(intent){}

signals:
    void intentClicked(int intent);
protected:
    void mouseReleaseEvent(QMouseEvent* event);
private:
    int intent;

};

#endif // WIDGETS_H
