#ifndef DELEGATES3_H
#define DELEGATES3_H

#include <QItemDelegate>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QListWidget>
#include <QComboBox>
#include <QToolButton>
#include <QLineEdit>
#include <QShortcut>
#include <QCheckBox>

#include "commdatastruct.h"
#include "cal.h"

class PAComboBox;
class FirstSubject;
class SecondSubject;
class SubjectManager;
class SubjectNameItem;
class Money;
class StatUtil;

enum BaTableColumnIndex{
    BT_SUMMARY = 0,   //摘要列
    BT_FSTSUB  = 1,   //总账科目列
    BT_SNDSUB  = 2,   //明细科目列
    BT_MTYPE   = 3,   //币种列
    BT_JV      = 4,   //借方金额列
    BT_DV      = 5,   //贷方金额列
    BT_DIR     = 6,   //借贷方向
    BT_ID      = 7,   //业务活动的ID列
    BT_PID     = 8,   //业务活动所属凭证ID列
    BT_NUM     = 9,   //业务活动在凭证中的序号列
    BT_ALL     = 10
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

//protected:
//    void keyPressEvent(QKeyEvent *event);
//    void mouseDoubleClickEvent(QMouseEvent *e);
//    void focusOutEvent(QFocusEvent* e);

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
class FstSubComboBox : public QWidget
{
    Q_OBJECT
public:
    FstSubComboBox(SubjectManager* subMgr, QWidget *parent = 0);
    ~FstSubComboBox();
    void setSubject(FirstSubject* fsub);
    FirstSubject* getSubject(){return fsub;}

    void addItem(const QString& text, const QVariant& userData = QVariant());
    int	currentIndex() const;
    void setCurrentIndex(int index);

    int	findData(const QVariant& data, int role = Qt::UserRole,
                 Qt::MatchFlags flags = static_cast<Qt::MatchFlags>
            ( Qt::MatchExactly | Qt::MatchCaseSensitive ) ) const;
    QVariant itemData (int index, int role = Qt::UserRole) const;
protected:
    void keyPressEvent(QKeyEvent* e );

private slots:
    void itemSelected(QListWidgetItem* item);
    void nameTextChanged(const QString& text);
    void nameTexteditingFinished();
    void subSelectChanged(const int index);
signals:
    void dataEditCompleted(int col, bool isMove);
private:
    void reloadFSubs(int witch, QString startStr);
    void hideList(bool isHide);

    QString* keys;
    SortByMode sortBy;
    int expandHeight;  //当出现智能提示框时要伸展的高度
    FirstSubject* fsub;              //
    SubjectManager *subMgr;          //
    QList<FirstSubject*> fsubs;  //所有二级科目

    bool textChangeReson; //组合框的文本是怎么改变的（true：鼠标选择组合框的下拉列表中的一个项目，false：用户输入到组合框的文本编辑区域）
    PAComboBox* com;       //显示当前一级科目下的可选的二级科目的组合框
    QListWidget* lw;      //智能提示列表框（显示所有带有指定前缀的名称条目）
};

//编辑和显示明细科目
class SndSubComboBox : public QWidget
{
    Q_OBJECT
public:
    SndSubComboBox(SecondSubject* ssub, FirstSubject* fsub, SubjectManager* subMgr, int row=0, int col=0, QWidget *parent = 0);
    void hideList(bool isHide);
    void setSndSub(SecondSubject* sub);
    //void setRowColNum(int row, int col);

    void addItem(const QString& text, const QVariant& userData = QVariant()){com->addItem(text,userData);}
    int	currentIndex() const{return com->currentIndex();}
    void setCurrentIndex(int index){com->setCurrentIndex(index);}
    int	findData(const QVariant& data, int role = Qt::UserRole,
                 Qt::MatchFlags flags = static_cast<Qt::MatchFlags>
            ( Qt::MatchExactly | Qt::MatchCaseSensitive ) ) const{return com->findData(data,role,flags);}
    QVariant itemData (int index, int role = Qt::UserRole) const{return com->itemData(index,role);}

protected:
    void keyPressEvent(QKeyEvent* e );

private slots:
    void itemSelected(QListWidgetItem* item);
    void nameTextChanged(const QString& text);
    void nameTexteditingFinished();
    void subSelectChanged(const int index);
signals:
    void newMappingItem(FirstSubject* fsub, SubjectNameItem* ni, SecondSubject*& ssub,int row, int col);
    void newSndSubject(FirstSubject* fsub, SecondSubject*& ssub, QString name, int row, int col);
    void dataEditCompleted(int col, bool isMove);
private:
    void filterListItem();
    void enterKeyWhenHide();

    int row,col;       //编辑器所处的行列位置
    QString* keys;     //接收到的字母或数字键（数字表示科目代码，字母表示科目助记符）
    SortByMode sortBy;
    int expandHeight;  //当出现智能提示框时要伸展的高度
    FirstSubject* fsub;              //二级科目所属的一级科目
    SecondSubject* ssub;             //当前选定的二级科目对象
    SubjectManager *subMgr;          //
    QList<SubjectNameItem*> allNIs;  //所有名称条目

    bool textChangeReson; //组合框的文本是怎么改变的（true：鼠标选择组合框的下拉列表中的一个项目，false：用户输入到组合框的文本编辑区域）
    QComboBox* com;       //显示当前一级科目下的可选的二级科目的组合框
    QListWidget* lw;      //智能提示列表框（显示所有带有指定前缀的名称条目）

    //bool editFinished;    //
};

////编辑和显示明细科目
//class SndSubComboBox2 : public QComboBox
//{
//    Q_OBJECT
//public:
//    SndSubComboBox2(int pid, SubjectManager* subMgr, QWidget *parent = 0);
//    ~SndSubComboBox2();
//    void setRowColNum(int row, int col);

//protected:
//    //void focusOutEvent(QFocusEvent* e);
//    void keyPressEvent(QKeyEvent* e );

//signals:
//    void newMappingItem(int fid, int sid, int row, int col);
//    void newSndSubject(int fid, QString name, int row, int col);
//    void dataEditCompleted(int col, bool isMove);
//    void editNextItem(int row, int col);   //这一信号仅用于设置明细科目余额值的表中

//private:
//    bool findSubMapper(int fid, int sid, int& id);
//    bool findSubName(QString name, int& sid);

//    int pid;   //所属的总账科目id
//    int row,col; //编辑器所处的行列位置
//    QListView* listview;   //智能提示列表框，用来供用户选择科目
//    QSqlQueryModel* model; //提取科目的数据模型（Fsagent和SecSubjects的连接查询）
//    QSqlTableModel* smodel; //从SecSubjects表提取，方便后续添加新科目（作为Listview的数据模型）
//    int rows;   //用以保存smodel的行数，因为smodel.rowCount()方法不一定返回正确的行数，因为模型类的实现一次不会返回所有行
//    QString* keys;   //接收到的字母或数字键（数字表示科目代码，字母表示科目助记符）
//    QStringList snames;//二级科目名称列表，用于输入二级科目名称时，提供一个输入完成器

//    SubjectManager* subMgr;
//};

//编辑和显示币种
class MoneyTypeComboBox : public QComboBox
{
    Q_OBJECT
public:
    MoneyTypeComboBox(QHash<int,Money*> mts, QWidget* parent = 0);
    void setCell(int row, int col);
    Money* getMoney();
protected:
    void keyPressEvent(QKeyEvent* e );
signals:
    void dataEditCompleted(int col, bool isMove);
    void editNextItem(int row, int col);   //这一信号仅用于设置明细科目余额值的表中
private:
    QHash<int,Money*> mts;
    int row,col;
};


//编辑和显示借贷金额
class MoneyValueEdit : public QLineEdit
{
    Q_OBJECT
public:
    MoneyValueEdit(int row, int witch = 0,Double v = Double(), QWidget* parent = 0);
    void setValue(Double v){this->v = v;setText(v.toString());}
    Double getValue(){return v;}
    void setCell(int row, int col){this->row = row;this->col = col;}
protected:
    void keyPressEvent(QKeyEvent* e );
private slots:
    void valueChanged(const QString & text);
    //void valueEdited();
signals:
    void dataEditCompleted(int col, bool isMove);
    void nextRow(int row);  //在贷方列按下回车键时触发此信号
    void editNextItem(int row, int col);   //这一信号仅用于设置明细科目余额值的表中
private:
    int witch; //用于编辑贷方还是借方（1：借，0：贷）
    Double v;
    int row,col;   //编辑器所处的行号和列
};


//提供编辑会计分录各项的项目代理类
class ActionEditItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    ActionEditItemDelegate(SubjectManager* subMgr, QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;

    void setReadOnly(bool readOnly){isReadOnly=readOnly;}
    void setVolidRows(int rows){validRows=rows;}
    int getVolidRows(){return validRows;}
    //void watchExtraException();

private slots:
    void commitAndCloseEditor(int colIndex, bool isMove);
    void newNameItemMapping(FirstSubject* fsub, SubjectNameItem* ni, SecondSubject*& ssub,int row, int col);
    void newSndSubject(FirstSubject* fsub, SecondSubject*& ssub, QString name, int row, int col);

    void nextRow(int row);
    void catchCopyPrevShortcut(int row, int col);

    //void cachedExtraException(BusiAction* ba,Double fv, MoneyDirection fd, Double sv, MoneyDirection sd);

signals:
    //void updateSndSubject(int row, int col, SecondSubject* ssub);
    void crtNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssbu);
    void crtNewSndSubject(int row, int col, FirstSubject* fsub, SecondSubject*& ssub, QString name);
    void moveNextRow(int row);
    void reqCopyPrevAction(int row);

    void extraException(BusiAction* ba,Double fv, MoneyDirection fd, Double sv, MoneyDirection sd);

private:
    //int witch;  //代理当前编辑的是业务活动的哪个列
    bool isReadOnly; //表格是否只读的
    int validRows;   //表格的有效行数，这个是为了对无效行不创建编辑器（不包含备用行)
    SubjectManager* subMgr;
    StatUtil* statUtil;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 仅用于简单的从一个下拉列表框中选择一个一级科目（它包含了一个空科目表示没有选取）
 */
class FSubSelectCmb : public QComboBox
{
    Q_OBJECT
public:
    FSubSelectCmb(SubjectManager* smg, QWidget* parent = 0);
    void setSubject(FirstSubject* fsub);
    FirstSubject* getSubject();
private:
    //FirstSubject* fsub;
};

/**
 * @brief 在科目系统衔接配置窗口内部表格使用的项目代理类
 */
class SubSysJoinCfgItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    SubSysJoinCfgItemDelegate(SubjectManager* subMgr, QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
    void setReadOnly(bool isReadonly){readOnly = isReadonly;}
private:
    SubjectManager* subMgr;
    bool readOnly;
};

#endif // DELEGATES3_H
