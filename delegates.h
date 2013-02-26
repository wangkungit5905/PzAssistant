#ifndef DELEGATES_H
#define DELEGATES_H

#include <QItemDelegate>
#include <QModelIndex>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QCalendarWidget>
#include <QListView>
#include <QComboBox>
#include <QKeyEvent>
#include <QStringListModel>
#include <QString>

/**
    为列表项提供整数到字符串的映射转换
 */
class iTosItemDelegate : public QItemDelegate{
    Q_OBJECT

public:
    iTosItemDelegate(QMap<int, QString> map, QObject *parent = 0);

    iTosItemDelegate(QHash<int, QString> map, QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                   const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void paint ( QPainter * painter, const QStyleOptionViewItem & option,
                 const QModelIndex & index ) const;

//    QSize QItemDelegate::sizeHint( const QStyleOptionViewItem & option,
//                                   const QModelIndex & index ) const;

    void setBoxEnanbled(bool isEnabled);
private:
    QMap<int, QString> innerMap;
    bool isEnabled;
};


//////////////////////////////////////////////////////////////////////////////
/**
    用来显示和编辑业务活动列表中的摘要部分内容的代理
*/
class SummaryDelegate : public QItemDelegate{
    Q_OBJECT

public:
    SummaryDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                   const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void paint ( QPainter * painter, const QStyleOptionViewItem & option,
                 const QModelIndex & index ) const;

//    QSize QItemDelegate::sizeHint( const QStyleOptionViewItem & option,
//                                   const QModelIndex & index ) const;

    //bool eventFilter(QObject *editor, QEvent *event);

private:
    QMap<int, QString> innerMap;
};

///////////////////////////////////////////////////////////////////////////////////
//保存科目信息的结构
struct SubjectInfo{
    int id;           //科目ID
    int weight;       //科目使用的权重值
    QString subCode;  //科目代码
    QString remCode;  //科目助记符
    QString subName;  //科目名

};


//////////////////////////////////////////////////////////////////////////////////

//具有智能输入提示的comboBox
class SmartComboBox : public QComboBox{
    Q_OBJECT

public:
    SmartComboBox(int witch, QSqlQueryModel* model, int curFid = 0, QWidget* parent = 0);


//public slots:
//    void editTextChanged(const QString &text);

protected:
    void focusOutEvent(QFocusEvent* e);
    void keyPressEvent(QKeyEvent* e );

signals:
    void newMappingItem(int fid, int sid);
    void newSndSubject(int fid, QString name);

private:
    bool findSubMapper(int fin, int sid, int *index);
    bool findSubName(QString name, int& sid);

    QListView* listview;   //智能提示列表框，用来供用户选择科目
    QSqlQueryModel* model; //提取科目的数据模型（FirSubjects或Fsagent和SecSubjects的连接查询）
    QSqlTableModel* smodel; //从SecSubjects表提取，方便后续添加新科目
    QString* keys;   //接收到的字母或数字键（数字表示科目代码，字母表示科目助记符）
    int witch;       //1：一级科目表， 2：二级科目表
    int curFid;      //与当前二级科目同处一行的左边的一级科目的ID
};

//在主表上显示子表内容的代理类(主要用于在业务活动列表中显示科目的名称而不是代码)
class RelationMappingDelegate : public QItemDelegate{
    Q_OBJECT
public:
    RelationMappingDelegate(QSqlQueryModel* model, int witch, int idCol, int dispCol, int itemClassCol = -1, QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                   const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void paint ( QPainter * painter, const QStyleOptionViewItem & option,
                 const QModelIndex & index ) const;

    void setItemClassIcon(QMap<int, QIcon> map, int col);

    void setCurFstId(int id);
    void setCurCellIndex(int row, int col);

public slots:
    void newMappingItem(int fid, int sid);
    void newSndSubject(int fid, QString name);

signals:
    void updateCell(int row, int col);
    void updateSndSub();   //这个信号用在设置账户基点窗口中每次新增一个新的明细科目时触发此信号

private:
    void refreshModel(int fid, int sid);


    QSqlQueryModel* model;  //如果显示一级科目（FstSujects），二级科目（Fsagent和SecSujects表的连接查询）

    int idCol, dispCol;
    QMap<int, QString> map; //保存科目ID与科目名称之间的对应关系，在显示项目而不是编辑时使用

    //对项目进行分类,整数表示分类，图标用于可视化的区别
    int itemClassCol;
    QMap<int, QIcon> iconMap;

    int witch;  //显示的是一级科目（1）还是二级科目（2）
    int curFid; //当前的一级科目ID（当当前的一级科目下没有对应的二级科目时可用）

    //QModelIndex curMIndex;  //当前代理类所代理的模型索引，在刷新显示新加入的一二级科目映射条目时使用
    int curRow;
    int curCol;
};

//以自定义的方式显示浮点数
class ViewDoubleDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    ViewDoubleDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                   const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void paint ( QPainter * painter, const QStyleOptionViewItem & option,
                 const QModelIndex & index ) const;

//    QSize QItemDelegate::sizeHint( const QStyleOptionViewItem & option,
//                                   const QModelIndex & index ) const;

private:
    //QCalendarWidget* calendar;
};


//class PZFormItemDelegate : public QItemDelegate{
//    Q_OBJECT

//public:
//    PZFormItemDelegate();

//    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
//                               const QModelIndex &index) const;

//    void setEditorData(QWidget *editor, const QModelIndex &index) const;
//    void setModelData(QWidget *editor, QAbstractItemModel *model,
//                   const QModelIndex &index) const;

//    void updateEditorGeometry(QWidget *editor,
//        const QStyleOptionViewItem &option, const QModelIndex &index) const;

////    void paint ( QPainter * painter, const QStyleOptionViewItem & option,
////                 const QModelIndex & index ) const;

////    QSize QItemDelegate::sizeHint( const QStyleOptionViewItem & option,
////                                   const QModelIndex & index ) const;

//private:
//    //QCalendarWidget* calendar;
//};

#endif // DELEGATES_H
