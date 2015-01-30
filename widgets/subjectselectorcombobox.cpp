#include "subjectselectorcombobox.h"
#include "subject.h"

#include <QTreeView>
#include <QWidget>
#include <QLineEdit>
#include <QHeaderView>

SubjectSelectorComboBox::SubjectSelectorComboBox(QWidget *parent):QComboBox(parent)
{
    subMgr = NULL;
    fsub = NULL;
    ssub = NULL;
    which = SC_FST;
    sortBy = SORTMODE_CODE;
    init();
}

SubjectSelectorComboBox::SubjectSelectorComboBox(SubjectManager* subMgr, FirstSubject* fsub,
                                                 SUBJECTCATALOG which, QWidget *parent) :
    QComboBox(parent),subMgr(subMgr),which(which),fsub(fsub)
{
    ssub = NULL;
    if(which == SC_FST)
        sortBy = SORTMODE_CODE;
    else
        sortBy = SORTMODE_REMCODE;
    tv.setWindowFlags(Qt::ToolTip);

    if(!subMgr || !fsub)
        return;
    if(which == SC_FST){
        loadFstSubs();
    }
    else{
        model.setDynamicSortFilter(true);
        if(!fsub)
            return;
        loadSndSubs();
    }
    init();
}

void SubjectSelectorComboBox::setSubjectManager(SubjectManager *smg)
{
    if(subMgr == smg)
        return;
    subMgr = smg;
    fsub = NULL;
    ssub = NULL;
    sortBy = SORTMODE_CODE;
}

void SubjectSelectorComboBox::setSubjectClass(SubjectSelectorComboBox::SUBJECTCATALOG subClass)
{
    which = subClass;
    if(which == SC_FST)
        loadFstSubs();
    else
        loadSndSubs();


}

void SubjectSelectorComboBox::setSubject(SubjectBase *fsub)
{
    setCurrentIndex(findSubject(fsub));
}

void SubjectSelectorComboBox::setParentSubject(FirstSubject *fsub)
{
    if(which == SC_FST)
        return;
    this->fsub = fsub;
    loadSndSubs();
}

void SubjectSelectorComboBox::keyPressEvent(QKeyEvent *event)
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

void SubjectSelectorComboBox::focusOutEvent(QFocusEvent *event)
{
    tv.hide();
}

void SubjectSelectorComboBox::completed(const QModelIndex &index)
{
    if(tv.isHidden() || !index.isValid())
        return;
    if(which == SC_FST){
        fsub = model.data(index,Qt::UserRole).value<FirstSubject*>();
        int idx = findSubject(fsub);
        setCurrentIndex(idx);
    }
    else{
        ssub = model.data(index,Qt::UserRole).value<SecondSubject*>();
        int idx = findSubject(ssub);
        setCurrentIndex(idx);
    }
    tv.hide();
}

void SubjectSelectorComboBox::nameChanged(const QString &text)
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

void SubjectSelectorComboBox::init()
{
    setEditable(true);
    tv.setWindowFlags(Qt::ToolTip);
    tv.setRootIsDecorated(false);
    tv.header()->hide();
    connect(&tv,SIGNAL(clicked(QModelIndex)),this,SLOT(completed(QModelIndex)));
    connect(this->lineEdit(),SIGNAL(textChanged(QString)),this,SLOT(nameChanged(QString)));
}

void SubjectSelectorComboBox::switchModel(bool on)
{
    if(on){
        model.setSourceModel(&sourceModel);
        model.setSortCaseSensitivity(Qt::CaseInsensitive);
        model.setFilterKeyColumn(sortBy-1);
        tv.setModel(&model);
        tv.header()->setStretchLastSection(false);
        tv.header()->setSectionResizeMode(0, QHeaderView::Stretch);
        if(which == SC_FST){
            tv.showColumn(SORTMODE_CODE-1);
            tv.hideColumn(SORTMODE_REMCODE-1);
            tv.setColumnWidth(SORTMODE_CODE-1,50);
        }
        else{
            tv.showColumn(SORTMODE_REMCODE-1);
            tv.hideColumn(SORTMODE_CODE-1);
            tv.setColumnWidth(SORTMODE_REMCODE-1,50);
        }
    }
    else{
        tv.setModel(0);
        //model.setSourceModel(0); //好像不重新设置数据源的情况下，重新设置模型的列数也不会出现段错误
    }

}

/**
 * @brief 装载所有一级科目
 */
void SubjectSelectorComboBox::loadFstSubs()
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

/**
 * @brief 装载当前一级科目下的所有二级科目
 */
void SubjectSelectorComboBox::loadSndSubs()
{
    if(!fsub)
        return;
    sourceModel.clear();
    clear();
    switchModel(false);
    sourceModel.setColumnCount(3);
    QStandardItem* item;
    QList<QStandardItem*> items;
    QVariant v;
    foreach (SecondSubject* sub, fsub->getChildSubs(SORTMODE_NAME)) {
        item = new QStandardItem(sub->getName());
        v.setValue<SecondSubject*>(sub);
        item->setData(v,Qt::UserRole);
        addItem(sub->getName(),v);
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

void SubjectSelectorComboBox::refreshModel()
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

void SubjectSelectorComboBox::showCompleteList()
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

int SubjectSelectorComboBox::findSubject(SubjectBase *sub)
{
    if(which == SC_FST){
        FirstSubject* fsub = static_cast<FirstSubject*>(sub);
        for(int i = 0; i < count(); ++i){
            FirstSubject* subject = itemData(i).value<FirstSubject*>();
            if(fsub == subject)
                return i;
        }
    }
    else{
        SecondSubject* ssub = static_cast<SecondSubject*>(sub);
        for(int i = 0; i < count(); ++i){
            SecondSubject* subject = itemData(i).value<SecondSubject*>();
            if(ssub == subject)
                return i;
        }
    }
    return -1;
}


