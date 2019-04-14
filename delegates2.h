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
#include "delegates.h"

class SubjectManager;
class FirstSubject;
class SecondSubject;
class SubjectNameItem;

/**
 * 流水分录表列索引
 */
enum JolColumnIndex{
    JCI_PNUM    = 0,   //凭证号
    JCI_GROUP   = 1,   //组序号
    JCI_SUMMARY = 2,   //摘要列
    JCI_FSUB  = 3,     //主目
    JCI_SSUB  = 4,     //子目
    JCI_MTYPE   = 5,   //币种
    JCI_JV      = 6,   //借方金额列
    JCI_DV      = 7,   //贷方金额列
    JCI_VTAG    = 8    //借贷平衡标志
};


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


//提供编辑流水分录各项的代理类
class JolItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    JolItemDelegate(SubjectManager* subMgr, QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
    void destroyEditor(QWidget * editor, const QModelIndex & index) const;

    //void setReadOnly(bool readOnly){isReadOnly=readOnly;}
    //void setVolidRows(int rows){validRows=rows;}
    //int getVolidRows(){return validRows;}
    //void watchExtraException();
    void userConfirmed(){canDestroy=true;}

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

    //void extraException(BusiAction* ba,Double fv, MoneyDirection fd, Double sv, MoneyDirection sd);

private:
    //int witch;  //代理当前编辑的是业务活动的哪个列
    //bool isReadOnly; //表格是否只读的
    //int validRows;   //表格的有效行数，这个是为了对无效行不创建编辑器（不包含备用行)
    SubjectManager* subMgr;
    //StatUtil* statUtil;    
    bool canDestroy;      //对象是否可以销毁（当创建新科目时，利用此标记延迟对象的销毁）
};





#endif // DELEGATES2_H


