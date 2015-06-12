#include "fstsubeditcombobox.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QVBoxLayout>

FstSubEditComboBox::FstSubEditComboBox(SubjectManager *subMgr, QWidget *parent):
    QWidget(parent),subMgr(subMgr)
{
    sortBy = SORTMODE_CODE;
    com = new QComboBox(this);
    com->setEditable(true);
    connect(com,SIGNAL(currentIndexChanged(int)),this,SLOT(subjectIndexChanged(int)));
    tv = new QTreeView(this);
    tv->setRootIsDecorated(false);
    tv->header()->hide();
    tv->setSelectionBehavior(QAbstractItemView::SelectRows);
    tv->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(tv,SIGNAL(clicked(QModelIndex)),this,SLOT(completed(QModelIndex)));
    QVBoxLayout* lm = new QVBoxLayout;
    lm->setSpacing(0);
    lm->setContentsMargins(0,0,0,0);
    lm->addWidget(com);
    lm->addWidget(tv);
    tv->setHidden(true);
    setLayout(lm);
    setMaximumHeight(200);
    loadSubs();
    installEventFilter(this);
    com->installEventFilter(this);
    connect(com->lineEdit(),SIGNAL(textChanged(QString)),this,SLOT(nameChanged(QString)));
}

void FstSubEditComboBox::setSubject(FirstSubject *fsub)
{
    QVariant v;
    v.setValue(fsub);
    int index = com->findData(v,Qt::UserRole);
    if(index == -1)
        this->fsub = NULL;
    else
        this->fsub = fsub;
    com->setCurrentIndex(index);
}

void FstSubEditComboBox::setCurrentIndex(int index)
{
    if(index < 0 || index >= com->count())
        fsub = NULL;
    else
        fsub = com->itemData(index,Qt::UserRole).value<FirstSubject*>();
    com->setCurrentIndex(index);
}

bool FstSubEditComboBox::eventFilter(QObject *obj, QEvent *ev)
{
    if(ev->type() != QEvent::KeyPress && ev->type() != QEvent::MouseButtonPress)
        return QWidget::eventFilter(obj, ev);
    QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
    FstSubEditComboBox* w = static_cast<FstSubEditComboBox*>(obj);
    if(ke && w && w == this){
        int key = ke->key();
        if(tv->isHidden()){
            if(key == Qt::Key_Enter || key == Qt::Key_Return){
                emit dataEditCompleted(1,true);
                return true;
            }
            else if(key >= Qt::Key_0 && key <= Qt::Key_9){
                sortBy = SORTMODE_CODE;
            }
            else if(key >= Qt::Key_A && key <= Qt::Key_Z){
                sortBy = SORTMODE_REMCODE;
            }
            else{
                QWidget::eventFilter(obj,ev);
                return false;
            }
            keys.clear();
            keys.append(ke->text());
            refreshModel();
            hideTView(model.rowCount()==0);
            return true;
        }
        else{
            if(key >= Qt::Key_0 && key <= Qt::Key_9){
                if(sortBy != SORTMODE_CODE){
                    sortBy = SORTMODE_CODE;
                    keys.clear();
                }
                keys.append(ke->text());
                refreshModel();
            }
            else if(key >= Qt::Key_A && key <= Qt::Key_Z){
                if(sortBy != SORTMODE_REMCODE){
                    sortBy = SORTMODE_REMCODE;
                    keys.clear();
                }
                keys.append(ke->text());
                refreshModel();
            }
            else if(key == Qt::Key_Enter || key == Qt::Key_Return){
                int row = tv->currentIndex().row();
                if(row >= 0)
                    completed(tv->currentIndex());
            }
            else if(key == Qt::Key_Up){
                int row = tv->currentIndex().row();
                if(row > 0){
                    QModelIndex index = model.index(row-1,0);
                    tv->setCurrentIndex(index);
                    FirstSubject* fsub = model.data(index,Qt::UserRole).value<FirstSubject*>();
                    QVariant v;
                    v.setValue<FirstSubject*>(fsub);
                    int idx = com->findData(v);
                    setCurrentIndex(idx);
                }
            }
            else if(key == Qt::Key_Down){
                int row = tv->currentIndex().row();
                if(row < model.rowCount()-1){
                    QModelIndex index = model.index(row+1,0);
                    tv->setCurrentIndex(index);
                    FirstSubject* fsub = model.data(index,Qt::UserRole).value<FirstSubject*>();
                    QVariant v;
                    v.setValue<FirstSubject*>(fsub);
                    int idx = com->findData(v);
                    setCurrentIndex(idx);
                }
            }
            else if(key == Qt::Key_Backspace){
                if(keys.count() == 1){
                    keys.clear();
                    tv->hide();
                }
                else{
                    keys.chop(1);
                }
                if(sortBy == SORTMODE_NAME)
                    return com->eventFilter(obj,ev);
                else
                    refreshModel();
            }
            else if(key == Qt::Key_Escape){
                keys.clear();
                tv->hide();
            }
            hideTView(model.rowCount()==0);
            return true;
        }
    }

    QComboBox* c = static_cast<QComboBox*>(obj);
    if(c && ke && c==com){
        int key = ke->key();
        if(key == Qt::Key_Up || key == Qt::Key_Down)
            return eventFilter(this,ev);
    }
    return QWidget::eventFilter(obj, ev);
}


void FstSubEditComboBox::completed(QModelIndex index)
{
    if(tv->isHidden() || !index.isValid())
        return;
    fsub = model.data(index,Qt::UserRole).value<FirstSubject*>();
    QVariant v;
    v.setValue<FirstSubject*>(fsub);
    int idx = com->findData(v);
    setCurrentIndex(idx);
    tv->hide();
    emit dataEditCompleted(1,true);
}

void FstSubEditComboBox::nameChanged(QString text)
{
    if(!com->lineEdit()->isModified()) //也可以用是否具有输入焦点来判断
        return;
    if(sortBy != SORTMODE_NAME){
        sortBy = SORTMODE_NAME;
    }
    keys = text;
    if(tv->isHidden())
        hideTView(model.rowCount()==0);
    refreshModel();
}

void FstSubEditComboBox::subjectIndexChanged(int index)
{
    if(index < 0 || index >= com->count())
        return;
    fsub = com->itemData(index).value<FirstSubject*>();
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
    com->clear();
    switchModel(false);
    sourceModel.setColumnCount(3);
    while(it->hasNext()){
        it->next();
        sub = it->value();
        if(!sub->isEnabled())
            continue;
        row++;
        v.setValue<FirstSubject*>(sub);
        com->addItem(sub->getName(),v);
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
        tv->setModel(&model);
        tv->header()->setStretchLastSection(false);
        tv->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        tv->showColumn(SORTMODE_CODE-1);
        tv->hideColumn(SORTMODE_REMCODE-1);
        tv->setColumnWidth(SORTMODE_CODE-1,50);
    }
    else{
        tv->setModel(0);
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
        tv->hide();
    }
    else {
        tv->setCurrentIndex(model.index(0,0));
    }
}

void FstSubEditComboBox::hideTView(bool isHide)
{
    tv->setHidden(isHide);
    if(isHide)
        resize(width(),com->height());
    else
        resize(width(),200);
}
