#ifndef DELEGATES2_H
#define DELEGATES2_H

#include <QItemDelegate>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QListView>
#include <QComboBox>
#include <QToolButton>
#include <QLineEdit>
#include <QShortcut>
#include <QCheckBox>

#include "common.h"

//提供编辑明细科目余额的代理类（用于设置明细科目余额）
class DetExtItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    enum ColumnIndex{
        SUB  = 0,   //明细科目列
        MT   = 1,   //币种列
        MV   = 2,   //金额列
        RV   = 3,   //本币金额列
        DIR  = 4,   //借贷方向
        INDEX= 5,   //
        TAG  = 6    //
    };

    DetExtItemDelegate(QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
    void setFid(int fid);

private slots:
    void commitAndCloseEditor(int colIndex, bool isMove);
    void newMappingItem(int fid, int sid, int row, int col);
    void newSndSubject(int fid, QString name, int row, int col);
    void editNextItem(int row, int col);

signals:
    void newMapping(int fid, int sid, int row, int col);
    void newSndSub(int fid, QString name, int row, int col);
    void editNext(int row, int col);

private:
    int fid;   //当前选定的一级科目id
};



//提供编辑业务活动各项的项目代理类
class ActionEditItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    enum ColumnIndex{
        SUMMARY = 0,   //摘要列
        FSTSUB  = 1,   //总账科目列
        SNDSUB  = 2,   //明细科目列
        MTYPE   = 3,   //币种列
        JV      = 4,   //借方金额列
        DV      = 5,   //贷方金额列
        DIR     = 6,   //借贷方向
        ID      = 7,   //业务活动的ID列
        PID     = 8,   //业务活动所属凭证ID列
        NUM     = 9    //业务活动在凭证中的序号列
    };

    ActionEditItemDelegate(QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;

    void setReadOnly(bool readOnly);
    void setVolidRows(int rows);

private slots:
    void commitAndCloseEditor(int colIndex, bool isMove);
    void newMappingItem(int pid, int sid, int row, int col);
    void newSndSubject(int fid, QString name, int row, int col);
    void nextRow(int row);
    void catchCopyPrevShortcut(int row, int col);

signals:
    void newSndSubMapping(int pid, int sid, int row, int col, bool reqConfirm = true);
    void newSndSubAndMapping(int fid, QString name, int row, int col);
    void moveNextRow(int row);
    void reqCopyPrevAction(int row, int col);

private:
    int witch;  //代理当前编辑的是业务活动的哪个列
    bool isReadOnly; //表格是否只读的
    int rows;        //表格的有效行数，这个是为了对无效行不创建编辑器(应该包含最末尾到用于添加新业务活动到行)
    //QStringList names; //二级科目名称列表，用于输入二级科目名称时，提供一个输入完成器    
};


//显示和编辑业务活动的摘要
class SummaryEdit : public QLineEdit
{
    Q_OBJECT
public:
    SummaryEdit(int row,int col,QWidget* parent = 0);
    void setContent(QString content);
    QString getContent();

signals:
    //当编辑器的数据修改完成后，触发此信号，
    //参数col表示编辑器所在列索引，isMove指示是否将输入焦点移动到下一个项目
    void dataEditCompleted(int col, bool isMove);
    void copyPrevShortcutPressed(int r,int c); //向外传播请求复制上一条会计分录，参数r和c表示按下快捷方式时的单元格行列索引

private slots:
    void summaryEditingFinished();
    void shortCutActivated();
//public slots:


protected:
    void keyPressEvent(QKeyEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void focusOutEvent(QFocusEvent* e);

private:
    void parse(QString content);
    QString assemble();

    QString summary;     //摘要部分
    QStringList fpNums;  //发票号列表
    QString bankNums;    //银行票据号
    int oppoSubject;     //对方科目id
    int row,col;         //编辑器打开时所处的行列位置

    QShortcut* shortCut;
};

//显示和编辑总账科目
class FstSubComboBox : public QComboBox
{
    Q_OBJECT
public:
    FstSubComboBox(QWidget *parent = 0);
    ~FstSubComboBox();
protected:
    //void focusOutEvent(QFocusEvent* e);
    void keyPressEvent(QKeyEvent* e );

//private slots:
//    void completeText(const QModelIndex &index);
signals:
    void dataEditCompleted(int col, bool isMove);
private:
    QListView* listview;   //智能提示列表框，用来供用户选择科目
    QSqlQueryModel* model; //提取科目作为Listview的数据模型（FirSubjects表）
    QString* keys;   //接收到的字母或数字键（数字表示科目代码，字母表示科目助记符）

};

//编辑和显示明细科目
class SndSubComboBox : public QComboBox
{
    Q_OBJECT
public:
    SndSubComboBox(int pid, QWidget *parent = 0);
    ~SndSubComboBox();
    void setRowColNum(int row, int col);

protected:
    //void focusOutEvent(QFocusEvent* e);
    void keyPressEvent(QKeyEvent* e );

signals:
    void newMappingItem(int fid, int sid, int row, int col);
    void newSndSubject(int fid, QString name, int row, int col);
    void dataEditCompleted(int col, bool isMove);
    void editNextItem(int row, int col);   //这一信号仅用于设置明细科目余额值的表中

private:
    bool findSubMapper(int fid, int sid, int& id);
    bool findSubName(QString name, int& sid);

    int pid;   //所属的总账科目id
    int row,col; //编辑器所处的行列位置
    QListView* listview;   //智能提示列表框，用来供用户选择科目
    QSqlQueryModel* model; //提取科目的数据模型（Fsagent和SecSubjects的连接查询）
    QSqlTableModel* smodel; //从SecSubjects表提取，方便后续添加新科目（作为Listview的数据模型）
    int rows;   //用以保存smodel的行数，因为smodel.rowCount()方法不一定返回正确的行数，因为模型类的实现一次不会返回所有行
    QString* keys;   //接收到的字母或数字键（数字表示科目代码，字母表示科目助记符）
    QStringList snames;//二级科目名称列表，用于输入二级科目名称时，提供一个输入完成器
};

//编辑和显示币种
class MoneyTypeComboBox : public QComboBox
{
    Q_OBJECT
public:
    MoneyTypeComboBox(QHash<int,QString>* mts, QWidget* parent = 0);
    void setCell(int row, int col);
protected:
    void focusOutEvent(QFocusEvent* e);
    void keyPressEvent(QKeyEvent* e );
signals:
    void dataEditCompleted(int col, bool isMove);
    void editNextItem(int row, int col);   //这一信号仅用于设置明细科目余额值的表中
private:
    QHash<int,QString>* mts;
    int row,col;
};

//编辑和显示借贷金额
class MoneyValueEdit : public QLineEdit
{
    Q_OBJECT
public:
    MoneyValueEdit(int row, int witch = 0,double v = 0, QWidget* parent = 0);
    void setValue(double v);
    double getValue();
    void setCell(int row, int col);
protected:
    void focusOutEvent(QFocusEvent* e);
    void keyPressEvent(QKeyEvent* e );
signals:
    void dataEditCompleted(int col, bool isMove);
    void nextRow(int row);  //在贷方列按下回车键时触发此信号
    void editNextItem(int row, int col);   //这一信号仅用于设置明细科目余额值的表中
private:
    int witch; //用于编辑贷方还是借方（1：借，0：贷）
    double v;
    int row,col;   //编辑器所处的行号和列
};

//编辑和显示借贷方向
class DirEdit : public QComboBox
{
    Q_OBJECT;
public:
    DirEdit(int dir = DIR_J, QWidget* parent = 0);
    void setDir(int dir);
    int getDir();
    void setCell(int row,int col);
signals:
    void editNextItem(int row, int col);   //这一信号仅用于设置明细科目余额值的表中

private:
    int dir;
    int row,col;
};

//编辑带描述信息的标记列
class TagEdit : public QWidget
{
    Q_OBJECT
public:
    TagEdit(bool tag = false, QString desc = "", QWidget* parent = 0);
    void setTag(bool tag){this->tag=tag;chkBox->setChecked(tag);}
    bool getTag(){return tag;}
    void setDescription(QString desc){this->desc=desc;edtBox->setText(desc);}
    QString getDescription(){return desc;}
    //void setCell(int row,int col);
//signals:
//    void editNextItem(int row, int col);   //这一信号仅用于设置明细科目余额值的表中

private slots:
    void tagToggled(bool checked);
    void DescEdited(const QString &text);
private:
    bool tag;
    QString desc;
    QCheckBox* chkBox;
    QLineEdit* edtBox;
    //int row,col;
};


#endif // DELEGATES2_H


