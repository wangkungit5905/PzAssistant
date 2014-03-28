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

class SubjectManager;
class FirstSubject;
class SecondSubject;
class SubjectNameItem;

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








#endif // DELEGATES2_H


