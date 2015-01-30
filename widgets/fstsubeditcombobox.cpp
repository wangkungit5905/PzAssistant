#include "fstsubeditcombobox.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QKeyEvent>

FstSubEditComboBox::FstSubEditComboBox(SubjectManager *subMgr, QWidget *parent):
    QComboBox(parent),subMgr(subMgr)
{
    sortBy = SORTMODE_CODE;
    tv.setWindowFlags(Qt::ToolTip);
    loadSubs();
    setEditable(true);
    tv.setWindowFlags(Qt::ToolTip);
    tv.setRootIsDecorated(false);
    tv.header()->hide();
    connect(&tv,SIGNAL(clicked(QModelIndex)),this,SLOT(completed(QModelIndex)));
    connect(this->lineEdit(),SIGNAL(textChanged(QString)),this,SLOT(nameChanged(QString)));
}

void FstSubEditComboBox::setSubject(FirstSubject *fsub)
{
    QVariant v;
    v.setValue(fsub);
    int index = findData(v,Qt::EditRole);
    if(index == -1)
        fsub = NULL;
    else
        this->fsub = fsub;
    setCurrentIndex(index);
}

void FstSubEditComboBox::setCurrentIndex(int index)
{
    if(index < 0 || index >= count())
        fsub = NULL;
    else
        fsub = itemData(index,Qt::UserRole).value<FirstSubject*>();
    QComboBox::setCurrentIndex(index);
}

void FstSubEditComboBox::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if(tv.isHidden()){
        if(key >= Qt::Key_0 && key <= Qt::Key_9){
            sortBy = SORTMODE_CODE;
        }
        else if(key >= Qt::Key_A && key <= Qt::Key_Z){
            sortBy = SORTMODE_REMCODE;
        }
        else{
            QComboBox::keyPressEvent(event);
            return;
        }
        keys.clear();
        keys.append(event->text());
        showCompleteList();
        refreshModel();
    }
    else{
        if(key >= Qt::Key_0 && key <= Qt::Key_9){
            if(sortBy != SORTMODE_CODE){
                sortBy = SORTMODE_CODE;
                keys.clear();
            }
            keys.append(event->text());
            refreshModel();
        }
        else if(key >= Qt::Key_A && key <= Qt::Key_Z){
            if(sortBy != SORTMODE_REMCODE){
                sortBy = SORTMODE_REMCODE;
                keys.clear();
            }
            keys.append(event->text());
            refreshModel();
        }
        else if(key == Qt::Key_Enter || key == Qt::Key_Return){
            int row = tv.currentIndex().row();
            if(row >= 0)
                completed(tv.currentIndex());
        }
        else if(key == Qt::Key_Up){
            int row = tv.currentIndex().row();
            if(row > 0){
                QModelIndex index = model.index(row-1,0);
                tv.setCurrentIndex(index);
            }
        }
        else if(key == Qt::Key_Down){
            int row = tv.currentIndex().row();
            if(row < model.rowCount()-1){
                QModelIndex index = model.index(row+1,0);
                tv.setCurrentIndex(index);
            }
        }
        else if(key == Qt::Key_Backspace){
            if(keys.count() == 1){
                keys.clear();
                tv.hide();
            }
            else{
                keys.chop(1);
            }
            if(sortBy == SORTMODE_NAME)
                QComboBox::keyPressEvent(event);
            else
                refreshModel();
        }
        else if(key == Qt::Key_Escape){
            keys.clear();
            tv.hide();
        }
        else{
            QComboBox::keyPressEvent(event);
        }
    }
}

void FstSubEditComboBox::focusOutEvent(QFocusEvent *event)
{
    tv.hide();
}

void FstSubEditComboBox::completed(QModelIndex index)
{
    if(tv.isHidden() || !index.isValid())
        return;
    fsub = model.data(index,Qt::UserRole).value<FirstSubject*>();
    QVariant v;
    v.setValue<FirstSubject*>(fsub);
    int idx = findData(v);
    setCurrentIndex(idx);
    tv.hide();
    emit dataEditCompleted(1,true);
}

void FstSubEditComboBox::nameChanged(QString text)
{
    if(!lineEdit()->isModified()) //也可以用是否具有输入焦点来判断
        return;
    if(sortBy != SORTMODE_NAME){
        sortBy = SORTMODE_NAME;
    }
    keys = text;
    if(tv.isHidden())
        showCompleteList();
    refreshModel();
}

void FstSubEditComboBox::loadSubs()
{
    FirstSubject* sub;
    FSubItrator* it = subMgr->getFstSubItrator();
    QStandardItem* item;
    QList<QStandardItem*> items;
    QVariant v;
    int row = -1;
    sourceModel.clear();
    clear();
    switchModel(false);
    sourceModel.setColumnCount(3);
    while(it->hasNext()){
        it->next();
        sub = it->value();
        if(!sub->isEnabled())
            continue;
        row++;
        v.setValue<FirstSubject*>(sub);
        addItem(sub->getName(),v);
        item = new QStandardItem(sub->getName());
        QVariant v2; v2.setValue<FirstSubject*>(sub);
        item->setData(v2,Qt::UserRole);
        items<<item;
        item = new QStandardItem(sub->getCode());
        items<<item;
        item = new QStandardItem(sub->getRemCode());
        items<<item;
        sourceModel.appendRow(items);
        items.clear();
    }
    switchModel();
}

void FstSubEditComboBox::switchModel(bool on)
{
    if(on){
        model.setSourceModel(&sourceModel);
        model.setSortCaseSensitivity(Qt::CaseInsensitive);
        model.setFilterKeyColumn(sortBy-1);
        tv.setModel(&model);
        tv.header()->setStretchLastSection(false);
        tv.header()->setSectionResizeMode(0, QHeaderView::Stretch);
        tv.showColumn(SORTMODE_CODE-1);
        tv.hideColumn(SORTMODE_REMCODE-1);
        tv.setColumnWidth(SORTMODE_CODE-1,50);
    }
    else{
        tv.setModel(0);
    }
}

void FstSubEditComboBox::refreshModel()
{
    if(model.filterKeyColumn() != (sortBy-1)){
        model.setFilterKeyColumn(sortBy-1);
        model.sort(sortBy-1);
    }
    model.setFilterFixedString(keys);
    int count = model.rowCount();
    if(count == 0){
        tv.hide();
    }
    else {
        tv.setCurrentIndex(model.index(0,0));
    }
}

void FstSubEditComboBox::showCompleteList()
{
    int w = width();
    if(w < 200)
        w = 200;
    tv.setMaximumWidth(w);
    tv.setMinimumWidth(w);
    QPoint p(0,height());
    int x = mapToGlobal(p).x();
    int y = mapToGlobal(p).y()+1;
    tv.move(x,y);
    tv.show();
}
