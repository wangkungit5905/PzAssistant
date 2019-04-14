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
#include "widgets/fstsubeditcombobox.h"


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


////////////////////////////////JolItemDelegate/////////////////
JolItemDelegate::JolItemDelegate(SubjectManager *subMgr, QObject *parent):
    QItemDelegate(parent),subMgr(subMgr),canDestroy(true)
{
    
}

QWidget* JolItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    int col = index.column();
    if(col < JCI_SUMMARY)
        return 0;
    int row = index.row();
    if(col == JCI_SUMMARY){ //摘要列
        SummaryEdit *editor = new SummaryEdit(row,col,parent);
        connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                this, SLOT(commitAndCloseEditor(int,bool)));
        connect(editor, SIGNAL(copyPrevShortcutPressed(int,int)),
                this, SLOT(catchCopyPrevShortcut(int,int)));
        return editor;
    }
    else if(col == JCI_FSUB){ //主目
        FstSubEditComboBox* editor = new FstSubEditComboBox(subMgr,parent);
        connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                this, SLOT(commitAndCloseEditor(int,bool)));
        return editor;
    }
    else if(col == JCI_SSUB){  //子目
        SecondSubject* ssub = index.model()->data(index,Qt::EditRole).value<SecondSubject*>();
        FirstSubject* fsub = index.model()->data(index.sibling(index.row(),index.column()-1),Qt::EditRole).value<FirstSubject*>();
        SndSubComboBox* editor = new SndSubComboBox(ssub,fsub,subMgr,index.row(),index.column(),parent);
        connect(editor,SIGNAL(newMappingItem(FirstSubject*,SubjectNameItem*,SecondSubject*&,int,int)),
                this,SLOT(newNameItemMapping(FirstSubject*,SubjectNameItem*,SecondSubject*&,int,int)));
        connect(editor,SIGNAL(newSndSubject(FirstSubject*,SecondSubject*&,QString,int,int)),
                this,SLOT(newSndSubject(FirstSubject*,SecondSubject*&,QString,int,int)));
        connect(editor,SIGNAL(dataEditCompleted(int,bool)),
                this,SLOT(commitAndCloseEditor(int,bool)));
        return editor;
    }
    else if(col == JCI_MTYPE){ //币种列
        QHash<int, Money*> mts;
        FirstSubject* fsub = index.model()->data(index.model()->index(row,JCI_FSUB),Qt::EditRole).value<FirstSubject*>();
        if(!fsub || !fsub->isUseForeignMoney()){
            Money* mmt = subMgr->getAccount()->getMasterMt();
            mts[mmt->code()] = mmt;
        }
        else if(fsub && fsub == subMgr->getBankSub()){
            SecondSubject* ssub = index.model()->data(index.model()->index(row,JCI_SSUB),Qt::EditRole).value<SecondSubject*>();
            if(ssub){
                Money* mt = subMgr->getSubMatchMt(ssub);
                mts[mt->code()] = mt;
            }
        }
        else
            mts = subMgr->getAccount()->getAllMoneys();
        MoneyTypeComboBox* editor = new MoneyTypeComboBox(mts, parent);
        connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                this, SLOT(commitAndCloseEditor(int,bool)));
        return editor;
    }
    else if(col == JCI_JV){ //借方金额列
        MoneyValueEdit *editor = new MoneyValueEdit(row,1,Double(),parent);
        connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                this, SLOT(commitAndCloseEditor(int,bool)));
        return editor;
    }
    else if(col == JCI_DV){ //贷方金额列
        MoneyValueEdit *editor = new MoneyValueEdit(row,0,Double(),parent);
        connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                this, SLOT(commitAndCloseEditor(int,bool)));
        connect(editor, SIGNAL(nextRow(int)), this, SLOT(nextRow(int)));
        return editor;
    }else{
        return 0; //提供一个审核的编辑代理
    }
}

void JolItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    if(col == JCI_SUMMARY){
       SummaryEdit* edit = qobject_cast<SummaryEdit*>(editor);
       if (edit) {
           edit->setText(index.model()->data(index, Qt::EditRole).toString());
       }
    }
    else if(col == JCI_FSUB){
        FstSubEditComboBox* cmb  = qobject_cast<FstSubEditComboBox*>(editor);
        if(cmb){
            FirstSubject* fsub = index.model()->
                    data(index, Qt::EditRole).value<FirstSubject*>();
            cmb->setSubject(fsub);
        }
    }
    //这个好像有点多余，因为在创建编辑器时已经设置好了一二级科目
    else if(col == JCI_SSUB){
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            SecondSubject* ssub = index.model()->data(index, Qt::EditRole)
                    .value<SecondSubject*>();
            cmb->setSndSub(ssub);
        }
    }
    else if(col == JCI_MTYPE){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        Money* mt = index.model()->data(index, Qt::EditRole).value<Money*>();
        if(mt){
            int idx = cmb->findData(mt->code(), Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if((col == JCI_JV) || (col == JCI_DV)){
        MoneyValueEdit *edit = qobject_cast<MoneyValueEdit*>(editor);
        if (edit) {
            Double v = index.model()->data(index, Qt::EditRole).value<Double>();
            edit->setValue(v);
        }
    }
}

void JolItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                  const QModelIndex &index) const
{
    int col = index.column();
    if(col == JCI_SUMMARY){
        SummaryEdit* edit = qobject_cast<SummaryEdit*>(editor);
        if(edit){
            if(edit->text() != model->data(index,Qt::EditRole))
                model->setData(index, edit->text());
        }
    }
    else if(col == JCI_FSUB){
        FstSubEditComboBox* cmb = qobject_cast<FstSubEditComboBox*>(editor);
        if(cmb){
            FirstSubject* fsub = cmb->getSubject();
            QVariant v; v.setValue(fsub);
            if(fsub != model->data(index,Qt::EditRole).value<FirstSubject*>())
                model->setData(index, v);
        }
    }
    else if(col == JCI_SSUB){
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            SecondSubject* ssub = cmb->subject();
            if(ssub != model->data(index,Qt::EditRole).value<SecondSubject*>()){
                QVariant v; v.setValue<SecondSubject*>(ssub);
                model->setData(index,v);
            }


        }
    }
    else if(col == JCI_MTYPE){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        if(cmb){
            Money* mt = cmb->getMoney();
            if(mt != model->data(index,Qt::EditRole).value<Money*>()){
                QVariant v;
                v.setValue(mt);
                model->setData(index, v);
            }
        }
    }
    else if((col == JCI_JV) || (col == JCI_DV)){
        MoneyValueEdit* edit = qobject_cast<MoneyValueEdit*>(editor);
        if(edit){
            Double v = edit->getValue();
            if(v != model->data(index,Qt::EditRole).value<Double>()){
                QVariant va;
                va.setValue(Double(v));
                model->setData(index, va);
            }
        }
    }
}

void JolItemDelegate::  commitAndCloseEditor(int colIndex, bool isMove)
{
    QWidget* editor=0;
    colIndex += 2;
    if(colIndex == JCI_SUMMARY)
        editor = qobject_cast<SummaryEdit*>(sender());
    else if(colIndex == JCI_FSUB)
        editor = qobject_cast<FstSubEditComboBox*>(sender());
    else if(colIndex == JCI_SSUB){
        editor = qobject_cast<SndSubComboBox*>(sender());
    }
    else if(colIndex == JCI_MTYPE)
        editor = qobject_cast<MoneyTypeComboBox*>(sender());
    else if(colIndex == JCI_JV)
        editor = qobject_cast<MoneyValueEdit*>(sender());
    else if(colIndex == JCI_DV)
        editor = qobject_cast<MoneyValueEdit*>(sender());

    if(isMove){
        emit commitData(editor);
        emit closeEditor(editor, QAbstractItemDelegate::EditNextItem);
    }
    else{
        emit commitData(editor);
        emit closeEditor(editor);
    }
}

/**
 * @brief ActionEditItemDelegate::newNameItemMapping
 * 提示在指定的一级科目下利用指定的名称条目创建新的二级科目
 * @param fsub 一级科目
 * @param ni   使用的名称条目
 * @param row  所在行
 * @param col  所在列
 */
void JolItemDelegate::newNameItemMapping(FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssub,int row, int col)
{
    canDestroy = false;
    emit crtNewNameItemMapping(row,col,fsub,ni,ssub);
}

/**
 * @brief ActionEditItemDelegate::newSndSubject
 * 提示创建新的二级科目（二级科目所用的名称条目也要新建）
 * @param fsub 一级科目
 * @param name 名称条目的简短名称
 * @param row  所在行
 * @param col  所在列
 */
void JolItemDelegate::newSndSubject(FirstSubject *fsub, SecondSubject*& ssub, QString name, int row, int col)
{
    canDestroy = false;
    emit crtNewSndSubject(row,col,fsub,ssub,name);
}

//信号传播中介，在编辑器打开的情况下，当用户在贷方列按回车键时，会接收到此信号，并将此信号进一步传播给凭证编辑窗口
void JolItemDelegate::nextRow(int row)
{
    emit moveNextRow(row);
}

//捕获编辑器触发的要求自动拷贝上一条分录的快捷方式
void JolItemDelegate::catchCopyPrevShortcut(int row, int col)
{
    emit reqCopyPrevAction(row);
}



void JolItemDelegate::updateEditorGeometry(QWidget* editor,
         const QStyleOptionViewItem &option, const QModelIndex& index) const
{
    QRect rect = option.rect;
    editor->setGeometry(rect);
}

void JolItemDelegate::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
    if(index.column() != JCI_SSUB)
        return QItemDelegate::destroyEditor(editor,index);
    if(canDestroy)
        editor->deleteLater();
}

