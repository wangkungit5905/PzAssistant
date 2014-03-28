#include <QLineEdit>
#include <QDomDocument>
#include <QInputDialog>
#include <QCompleter>
#include <QPainter>
#include <QKeyEvent>

#include "tables.h"
#include "delegates2.h"
#include "utils.h"
#include "dialog2.h"
#include "global.h"
#include "subject.h"
#include "dbutil.h"


//////////////////////////////iTosItemDelegate/////////////////////////////
iTosItemDelegate::iTosItemDelegate(QMap<int, QString> map, QObject *parent) : QItemDelegate(parent)
{
    isEnabled = true;
    QMapIterator<int, QString> i(map);
    while (i.hasNext()) {
        i.next();
        innerMap.insert(i.key(),i.value());
    }

}

iTosItemDelegate::iTosItemDelegate(QHash<int, QString> map, QObject *parent) : QItemDelegate(parent)
{
    isEnabled = true;
    QHashIterator<int, QString> i(map);
    while (i.hasNext()) {
        i.next();
        innerMap.insert(i.key(),i.value());
    }
}


QWidget* iTosItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const{
    QComboBox *editor = new QComboBox(parent);
    editor->setEnabled(isEnabled);
    QMapIterator<int, QString> i(innerMap);
    while (i.hasNext()) {
        i.next();
        editor->addItem(i.value(), i.key());
    }
    return editor;
}

void iTosItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int value = index.model()->data(index, Qt::EditRole).toInt();
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    int idx = comboBox->findData(value);
    comboBox->setCurrentIndex(idx);
}

void iTosItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
               const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    //comboBox->interpretText();
    int idx = comboBox->currentIndex();
    int value = comboBox->itemData(idx).toInt();

    model->setData(index, value, Qt::EditRole);

}

void iTosItemDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);

}

void iTosItemDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option,
             const QModelIndex & index ) const
{
    int value = index.model()->data(index, Qt::EditRole).toInt();
    painter->drawText(option.rect, Qt::AlignCenter, innerMap[value]);
}

//只是想使货币类型列不可编辑的权益之计
void iTosItemDelegate::setBoxEnanbled(bool isEnabled)
{
    this->isEnabled = isEnabled;
}




