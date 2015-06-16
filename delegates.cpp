#include <QDomDocument>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "global.h"
#include "delegates.h"
#include "tables.h"
#include "dialogs.h"
#include "widgets/variousWidgets.h"
#include "widgets/fstsubeditcombobox.h"
#include "logs/Logger.h"
#include "subject.h"
#include "statements.h"
#include "keysequence.h"
#include "PzSet.h"
#include "statutil.h"
#include "myhelper.h"
#include "batemplateform.h"


////////////////////////////SummaryEdit/////////////////////////
SummaryEdit::SummaryEdit(int row,int col,QWidget* parent) : QLineEdit(parent)
{
    this->row = row;
    this->col = col;
    connect(this, SIGNAL(returnPressed()),
            this, SLOT(summaryEditingFinished())); //输入焦点自动转移到右边一列
    //installEventFilter(this);
    //经过测试发现，在这种临时部件上不能捕获快捷键，用alt键序列，在按下alt键后会失去光标而关闭编辑器，用ctrl键序列无反映
    //shortCut = new QShortcut(QKeySequence(PZEDIT_COPYPREVROW),this);
    //shortCut->setContext(Qt::WidgetShortcut);
    //shortCut = new QShortcut(QKeySequence(tr("Ctrl+=")),this,0,0,Qt::WidgetShortcut);
    //shortCut = new QShortcut(QKeySequence(tr("Ctrl+P")),this);
    //connect(shortCut,SIGNAL(activated()),this,SLOT(shortCutActivated()));
}



//摘要部分修改结束了
void SummaryEdit::summaryEditingFinished()
{
    emit dataEditCompleted(0,true);
}

//bool SummaryEdit::eventFilter(QObject *obj, QEvent *event)
//{
//    QKeyEvent* e = static_cast<QKeyEvent*>(event);
//    if(e){
//        int keyCode = e->key();
//        //为什么按一次组合键会连续产生4次相同的事件，且只有最后一次事件的时间戳是不同的？
//        if((e->modifiers() & Qt::ControlModifier) &&
//                keyCode == Qt::Key_Equal){
//            //LOG_INFO(QString("Ctrl+= at %1").arg(e->timestamp()));
//            emit copyPrevShortcutPressed(row,col);
//            return true;
//        }
//    }
//    return QLineEdit::eventFilter(obj,event);
//}

//捕获自动复制上一条会计分录的快捷方式
//void SummaryEdit::shortCutActivated()
//{
//    if(row > 0)
//        emit copyPrevShortcutPressed(row,col);
//}

//重载此函数的目的是为了捕获拷贝上一条分录的快捷键序列
void SummaryEdit::keyPressEvent(QKeyEvent *event)
{
    if(event->modifiers() == Qt::ControlModifier){
        int key = event->key();
        if(key == Qt::Key_Equal){
            emit copyPrevShortcutPressed(row,col);
        }
        else
            QLineEdit::keyPressEvent(event);//没有它，无法捕获拷贝、粘贴快捷键
    }
    else
        QLineEdit::keyPressEvent(event);


}

////////////////////////////SndSubComboBox//////////////////////////
SndSubComboBox::SndSubComboBox(SecondSubject* ssub, FirstSubject* fsub, SubjectManager *subMgr,
                               int row,int col,QWidget *parent):QWidget(parent),
    subMgr(subMgr),ssub(ssub),fsub(fsub),row(row),col(col),sortBy(SORTMODE_NAME)
{
    com = new QComboBox(this);
    com->setEditable(true);       //使其可以输入新的名称条目
    lw = new QListWidget(this);
    lw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(lw,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(itemSelected(QListWidgetItem*)));
    installEventFilter(this);
    com->installEventFilter(this);
    keys = new QString;
    lw->setHidden(true);
    QVBoxLayout* l = new QVBoxLayout;
    l->setSpacing(0);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(com);
    l->addWidget(lw);
    setLayout(l);

    //装载当前一级科目下的所有二级科目对象
    QVariant v;
    if(fsub){        
        foreach(SecondSubject* sub, fsub->getChildSubs()){
            v.setValue(sub);
            com->addItem(sub->getName(),v);
        }
    }
    if(ssub){
        v.setValue(ssub);
        com->setCurrentIndex(com->findData(v,Qt::UserRole));
    }
    else
        com->setCurrentIndex(-1);
    connect(com,SIGNAL(currentIndexChanged(int)),this,SLOT(subSelectChanged(int)));
    connect(com->lineEdit(),SIGNAL(textEdited(QString)),this,SLOT(nameTextChanged(QString)));
    connect(com->lineEdit(),SIGNAL(returnPressed()),this,SLOT(nameTexteditingFinished()));

    //装载所有名称条目
    allNIs = subMgr->getAllNameItems();
    qSort(allNIs.begin(),allNIs.end(),byNameThan_ni);
    QListWidgetItem* item;
    foreach(SubjectNameItem* ni, allNIs){
        v.setValue(ni);
        item = new QListWidgetItem(ni->getShortName());
        item->setData(Qt::UserRole, v);
        lw->addItem(item);
    }
    if(AppConfig::getInstance()->ssubFirstlyInputMothed())
        setFocusProxy(com);
}

/**
 * @brief SndSubComboBox::hideList 隐藏和显示智能提示列表框
 * @param isHide
 */
void SndSubComboBox::hideList(bool isHide)
{
    lw->setHidden(isHide);
    if(isHide)
        resize(width(),com->height());
    else
        resize(width(),200);
}

/**
 * @brief SndSubComboBox::setSndSub 设置二级科目对象
 * @param sub
 */
void SndSubComboBox::setSndSub(SecondSubject *ssub)
{
    if(ssub && ssub->getParent() == fsub){
        if(this->ssub != ssub){
            this->ssub = ssub;
            QVariant v;
            v.setValue(ssub);
            com->setCurrentIndex(com->findData(v,Qt::UserRole));
        }
    }
    else{
        this->ssub = 0;
        com->setCurrentIndex(-1);
    }
}



bool SndSubComboBox::eventFilter(QObject *obj, QEvent *event)
{
    static bool isDigit = true;  //true：输入的是科目的数字代码，false：科目的助记符
    QListWidgetItem* item;
    if(obj == this && event->type() == QEvent::KeyPress){
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        int keyCode = e->key();
        if((keyCode == Qt::Key_Return) || (keyCode == Qt::Key_Enter)){
            if(lw->isVisible()){
                item = lw->currentItem();
                if(item){
                    SubjectNameItem* ni = item->data(Qt::UserRole).value<SubjectNameItem*>();
                    ssub = fsub->getChildSub(ni);
                    if(!ssub){
                        emit newMappingItem(fsub,ni,ssub,row,col);
                    }
                }

            }
            emit dataEditCompleted(BT_SNDSUB,true);
            return true;
        }
        else if(keyCode == Qt::Key_Backspace){
            keys->chop(1);
            if(keys->size() == 0)
                hideList(true);
            else
                filterListItem();
        }
        else if(keyCode == Qt::Key_Up){
            if(processArrowKey(true))
                return true;
        }
        else if(keyCode == Qt::Key_Down){
            if(processArrowKey(false))
                return true;
        }
        else if(keyCode >= Qt::Key_A && keyCode <= Qt::Key_Z){
            keys->append(keyCode);
            isDigit = false;
            if(keys->size() == 1){ //接收到第一个字符，需要重新按科目助记符排序，并装载到列表框
                isDigit = false;
                sortBy = SORTMODE_REMCODE;
                qSort(allNIs.begin(),allNIs.end(),byRemCodeThan_ni);
                lw->clear();
                QVariant v;
                foreach(SubjectNameItem* ni, allNIs){
                    v.setValue(ni);
                    item = new QListWidgetItem(ni->getShortName());
                    item->setData(Qt::UserRole, v);
                    lw->addItem(item);
                }
                hideList(false);
            }
            filterListItem();
        }
        //如果是数字键则键入的是科目代码，则按科目代码快速定位
        else if((keyCode >= Qt::Key_0) && (keyCode <= Qt::Key_9)){
            keys->append(keyCode);
            if(keys->size() == 1){
                isDigit = true;
                sortBy = SORTMODE_CODE;
                hideList(false);
                //...
            }
        }
        return true;
    }
    //组合框事件处理
    QComboBox* cmb = qobject_cast<QComboBox*>(obj);
    if(!cmb || cmb != com)
        return QObject::eventFilter(obj, event);
    if(event->type() == QEvent::KeyPress){
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        int keyCode = e->key();
        if(lw->isHidden()){
            if((keyCode == Qt::Key_Return) || (keyCode == Qt::Key_Enter)){
                com->lineEdit()->clearFocus();
                this->setFocus();
                return false;
            }
        }
        else{
            if(keyCode == Qt::Key_Up){
                processArrowKey(true);
                return true;
            }
            else if(keyCode == Qt::Key_Down){
                processArrowKey(false);
                return true;
            }
            if(keyCode == Qt::Key_Return || keyCode == Qt::Key_Enter){
                itemSelected(lw->currentItem());
                com->lineEdit()->clearFocus();
                this->setFocus();
            }
        }
    }
    return com->eventFilter(obj, event);
}

/**
 * @brief SndSubComboBox::itemSelected 当用户在智能提示框中选择一个名称条目时
 * @param item
 */
void SndSubComboBox::itemSelected(QListWidgetItem *item)
{
    if(!item)
        return;
    SubjectNameItem* ni = item->data(Qt::UserRole).value<SubjectNameItem*>();
    if(!fsub)
        return;
    if(fsub->containChildSub(ni)){
        ssub = fsub->getChildSub(ni);
        QVariant v;
        v.setValue(ssub);
        int index = com->findData(v,Qt::UserRole);
        com->setCurrentIndex(index);
    }
    else if(subMgr->containNI(ni)){
        SecondSubject* ssub = NULL;
        emit newMappingItem(fsub,ni,ssub,row,col);
        if(ssub){
            this->ssub = ssub;
            QVariant v;
            v.setValue(ssub);
            com->addItem(ssub->getName(),v);
            com->setCurrentIndex(com->count()-1);
        }
    }
    hideList(true);
}

/**
 * @brief SndSubComboBox::nameItemTextChanged
 * 当组合框内的文本编辑区域的文本（用户输入名称条目时）发生改变
 * @param text
 */
void SndSubComboBox::nameTextChanged(const QString &text)
{
    if(sortBy != SORTMODE_NAME)
        return;
    filterListItem();
    hideList(false);
}

/**
 * @brief SndSubComboBox::nameTexteditingFinished
 * 用户提交输入的名称条目文本
 */
void SndSubComboBox::nameTexteditingFinished()
{
    QString editText = com->lineEdit()->text();
    if(editText.isEmpty()){
        ssub = 0;
        return;
    }

    //遍历智能提示列表框，找出名称完全匹配的条目
    QListWidgetItem* item;
    SubjectNameItem* ni;
    for(int i = 0; i < lw->count(); ++i){
        item = lw->item(i);
        if(item->isHidden())
            continue;
        ni = item->data(Qt::UserRole).value<SubjectNameItem*>();
        QString name = ni->getShortName();
        if(editText == name){
            itemSelected(item);
            return;
        }
    }
    //如果没有找到，则触发新名称条目信号
    SecondSubject* ssub = NULL;
    emit newSndSubject(fsub,ssub,editText,row,col);
    if(ssub){
        QVariant v;
        v.setValue(ssub);
        com->addItem(ssub->getName(),v);
        com->setCurrentIndex(com->count()-1);
    }
}

/**
 * @brief SndSubComboBox::subSelectChanged
 * 当用户通过从组合框的下拉列表中选择一个科目时
 * @param text
 */
void SndSubComboBox::subSelectChanged(const int index)
{
    ssub = com->itemData(index).value<SecondSubject*>();
}

/**
 * @brief 处理上下箭头键盘事件（以调整智能列表框的当前选择项）
 * @param up
 * @return true：列表已经到顶或到底，无法继续移动
 */
bool SndSubComboBox::processArrowKey(bool up)
{
    if(lw->isVisible()){
        int startRow;
        if(up){
            startRow = lw->currentRow();
            if(startRow == 0)
                return true;
            startRow--;
            for(startRow; startRow > -1; startRow--){
                if(!lw->item(startRow)->isHidden()){
                    lw->setCurrentRow(startRow);
                    lw->scrollToItem(lw->item(startRow),QAbstractItemView::PositionAtCenter);
                    break;
                }
            }
        }
        else{
            startRow = lw->currentRow();
            if(startRow == lw->count()-1)
                return true;
            startRow++;
            for(startRow; startRow < lw->count(); ++startRow){
                if(!lw->item(startRow)->isHidden()){
                    lw->setCurrentRow(startRow);
                    lw->scrollToItem(lw->item(startRow),QAbstractItemView::PositionAtCenter);
                    break;
                }
            }
        }
        QListWidgetItem* item = lw->currentItem();
        if(!item)
            return true;
        SubjectNameItem* ni = lw->currentItem()->data(Qt::UserRole).value<SubjectNameItem*>();
        SecondSubject* sub = fsub->getChildSub(ni);
        if(!ni)
            com->setCurrentIndex(-1);
        else{
            QVariant v; v.setValue<SecondSubject*>(sub);
            com->setCurrentIndex(com->findData(v));
        }
    }
    else{
        if(com->count()==0)
            return true;
        if(up){
            int ci = com->currentIndex();
            if(ci == 0)
                return true;
            com->setCurrentIndex(ci-1);
        }
        else{
            int ci = com->currentIndex();
            if(ci == com->count()-1)
                return true;
            com->setCurrentIndex(ci+1);
        }
    }
    return false;
}


/**
 * @brief SndSubComboBox::filteListItem 提示框列表项过滤
 * @param witch （0：按名称字符顺序，1：助记符，2：科目代码）
 * @param prefixStr：前缀字符串
 */
void SndSubComboBox::filterListItem()
{
    //隐藏所有助记符不是以指定字符串开始的名称条目
    if(sortBy == SORTMODE_NAME){
        QString namePre = com->lineEdit()->text().trimmed();
        for(int i = 0; i < allNIs.count(); ++i){
            if(allNIs.at(i)->getShortName().startsWith(namePre,Qt::CaseInsensitive))
                lw->item(i)->setHidden(false);
            else
                lw->item(i)->setHidden(true);
        }
    }
    else if(sortBy == SORTMODE_REMCODE){
        for(int i = 0; i < allNIs.count(); ++i){
            if(allNIs.at(i)->getRemCode().startsWith(*keys,Qt::CaseInsensitive))
                lw->item(i)->setHidden(false);
            else
                lw->item(i)->setHidden(true);
        }
    }
    //隐藏所有科目代码不是以指定串开始的二级科目
    else{
        int i = 0;
        //...
    }
}

///////////////////////////////MoneyTypeComboBox///////////////////////
MoneyTypeComboBox::MoneyTypeComboBox(QHash<int,Money*> mts,QWidget* parent):
    QComboBox(parent),mts(mts)
{
    QHashIterator<int,Money*> it(mts);
    while(it.hasNext()){
        it.next();
        addItem(it.value()->name(),it.key());
    }
}

void MoneyTypeComboBox::setCell(int row,int col)
{
    this->row = row;
    this->col = col;
}

/**
 * @brief MoneyTypeComboBox::getMoney
 * 获取当前选定的Money对象
 * @return
 */
Money* MoneyTypeComboBox::getMoney()
{
    return mts.value(itemData(currentIndex()).toInt());
}

//实现输入货币代码即定位到正确的货币索引
void MoneyTypeComboBox::keyPressEvent(QKeyEvent* e )
{
    int key = e->key();
    if((key >= Qt::Key_0) && (key <= Qt::Key_9)){
        int mcode = e->text().toInt();
        int idx = findData(mcode);
        if(idx != -1){
            setCurrentIndex(idx);
            e->accept();
        }
    }
    //还可以加入左右移动的箭头键，以使用户科目用键盘来定位到下一个单元格
    else if((key == Qt::Key_Return) || (key == Qt::Key_Enter)){
        emit dataEditCompleted(BT_MTYPE,true);
        emit editNextItem(row,col);
        e->accept();
    }
    else
        e->ignore();
    QComboBox::keyPressEvent(e);
}

///////////////////////////MoneyValueEdit/////////////////////////////////////
MoneyValueEdit::MoneyValueEdit(int row, int witch, Double v, QWidget *parent)
    :QLineEdit(parent),row(row),witch(witch)
{
    setValue(v);
    MyDoubleValidator *va = new MyDoubleValidator(this);
    va->setDecimals(2);
    setValidator(va);
    validator  = new QDoubleValidator(this);
    validator->setDecimals(2);
    installEventFilter(this);
}

void MoneyValueEdit::setValue(Double v)
{
    this->v = v;
    setText(v.toString());
}

Double MoneyValueEdit::getValue()
{
    v = text().toDouble();
    return v;
}

bool MoneyValueEdit::eventFilter(QObject *obj, QEvent *e)
{
    if(e->type() != QEvent::KeyPress)
        return QLineEdit::eventFilter(obj,e);
    QKeyEvent* ke = static_cast<QKeyEvent*>(e);
    int keyCode = ke->key();
    if((keyCode == Qt::Key_Return) || (keyCode == Qt::Key_Enter)){
        v = text().toDouble();
        if(witch == 1)  //借方金额栏
            emit dataEditCompleted(BT_JV, true);
        else
            emit dataEditCompleted(BT_DV, true);
        if(witch == 0)
            emit nextRow(row);  //传播给代理，代理再传播给凭证编辑窗
        emit editNextItem(row,col);
    }
    return QLineEdit::eventFilter(obj,e);
}



////////////////////////////////ActionEditItemDelegate/////////////////
ActionEditItemDelegate::ActionEditItemDelegate(SubjectManager *subMgr, QObject *parent):
    QItemDelegate(parent),subMgr(subMgr),canDestroy(true)
{
    isReadOnly = false;
    statUtil = NULL;
}

QWidget* ActionEditItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    if(isReadOnly)
        return 0;
    int col = index.column();
    int row = index.row();
    if(row <= validRows){
        if(col == BT_SUMMARY){ //摘要列
            SummaryEdit *editor = new SummaryEdit(row,col,parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            connect(editor, SIGNAL(copyPrevShortcutPressed(int,int)),
                    this, SLOT(catchCopyPrevShortcut(int,int)));
            return editor;
        }
        else if(col == BT_FSTSUB){ //总账科目列
            FstSubEditComboBox* editor = new FstSubEditComboBox(subMgr,parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            return editor;
        }
        else if(col == BT_SNDSUB){  //明细科目列
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
        else if(col == BT_MTYPE){ //币种列
            QHash<int, Money*> mts;
            FirstSubject* fsub = index.model()->data(index.model()->index(row,BT_FSTSUB),Qt::EditRole).value<FirstSubject*>();
            if(!fsub || !fsub->isUseForeignMoney()){
                Money* mmt = subMgr->getAccount()->getMasterMt();
                mts[mmt->code()] = mmt;
            }
            else if(fsub && fsub == subMgr->getBankSub()){
                SecondSubject* ssub = index.model()->data(index.model()->index(row,BT_SNDSUB),Qt::EditRole).value<SecondSubject*>();
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
        else if(col == BT_JV){ //借方金额列
            MoneyValueEdit *editor = new MoneyValueEdit(row,1,Double(),parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            return editor;
        }
        else{               //贷方金额列
            MoneyValueEdit *editor = new MoneyValueEdit(row,0,Double(),parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            connect(editor, SIGNAL(nextRow(int)), this, SLOT(nextRow(int)));
            return editor;
        }
    }
    else
        return 0;
}

void ActionEditItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    if(col == BT_SUMMARY){
       SummaryEdit* edit = qobject_cast<SummaryEdit*>(editor);
       if (edit) {
           edit->setText(index.model()->data(index, Qt::EditRole).toString());
       }
    }
    else if(col == BT_FSTSUB){
        FstSubEditComboBox* cmb  = qobject_cast<FstSubEditComboBox*>(editor);
        if(cmb){
            FirstSubject* fsub = index.model()->
                    data(index, Qt::EditRole).value<FirstSubject*>();
            cmb->setSubject(fsub);
        }
    }
    //这个好像有点多余，因为在创建编辑器时已经设置好了一二级科目
    else if(col == BT_SNDSUB){
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            SecondSubject* ssub = index.model()->data(index, Qt::EditRole)
                    .value<SecondSubject*>();
            cmb->setSndSub(ssub);
        }
    }
    else if(col == BT_MTYPE){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        Money* mt = index.model()->data(index, Qt::EditRole).value<Money*>();
        if(mt){
            int idx = cmb->findData(mt->code(), Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if((col == BT_JV) || (col == BT_DV)){
        MoneyValueEdit *edit = qobject_cast<MoneyValueEdit*>(editor);
        if (edit) {
            Double v = index.model()->data(index, Qt::EditRole).value<Double>();
            edit->setValue(v);
        }
    }
}

void ActionEditItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                  const QModelIndex &index) const
{
    int col = index.column();
    if(col == BT_SUMMARY){
        SummaryEdit* edit = qobject_cast<SummaryEdit*>(editor);
        if(edit){
            if(edit->text() != model->data(index,Qt::EditRole))
                model->setData(index, edit->text());
        }
    }
    else if(col == BT_FSTSUB){
        FstSubEditComboBox* cmb = qobject_cast<FstSubEditComboBox*>(editor);
        if(cmb){
            FirstSubject* fsub = cmb->getSubject();
            QVariant v; v.setValue(fsub);
            if(fsub != model->data(index,Qt::EditRole).value<FirstSubject*>())
                model->setData(index, v);
        }
    }
    else if(col == BT_SNDSUB){
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            SecondSubject* ssub = cmb->subject();
            if(ssub != model->data(index,Qt::EditRole).value<SecondSubject*>()){
                QVariant v; v.setValue<SecondSubject*>(ssub);
                model->setData(index,v);
            }


        }
    }
    else if(col == BT_MTYPE){
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
    else if((col == BT_JV) || (col == BT_DV)){
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

void ActionEditItemDelegate::commitAndCloseEditor(int colIndex, bool isMove)
{
    QWidget* editor;
    if(colIndex == BT_SUMMARY)
        editor = qobject_cast<SummaryEdit*>(sender());
    else if(colIndex == BT_FSTSUB)
        editor = qobject_cast<FstSubEditComboBox*>(sender());
    else if(colIndex == BT_SNDSUB){
        editor = qobject_cast<SndSubComboBox*>(sender());
    }
    else if(colIndex == BT_MTYPE)
        editor = qobject_cast<MoneyTypeComboBox*>(sender());
    else if(colIndex == BT_JV)
        editor = qobject_cast<MoneyValueEdit*>(sender());
    else if(colIndex == BT_DV)
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
void ActionEditItemDelegate::newNameItemMapping(FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssub,int row, int col)
{
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
void ActionEditItemDelegate::newSndSubject(FirstSubject *fsub, SecondSubject*& ssub, QString name, int row, int col)
{
    canDestroy = false;
    emit crtNewSndSubject(row,col,fsub,ssub,name);
}

//信号传播中介，在编辑器打开的情况下，当用户在贷方列按回车键时，会接收到此信号，并将此信号进一步传播给凭证编辑窗口
void ActionEditItemDelegate::nextRow(int row)
{
    emit moveNextRow(row);
}

//捕获编辑器触发的要求自动拷贝上一条分录的快捷方式
void ActionEditItemDelegate::catchCopyPrevShortcut(int row, int col)
{
    emit reqCopyPrevAction(row);
}



void ActionEditItemDelegate::updateEditorGeometry(QWidget* editor,
         const QStyleOptionViewItem &option, const QModelIndex& index) const
{
    QRect rect = option.rect;
    editor->setGeometry(rect);
}

void ActionEditItemDelegate::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
    if(index.column() != BT_SNDSUB)
        return QItemDelegate::destroyEditor(editor,index);
    if(canDestroy)
        editor->deleteLater();
}

///////////////////////////////FSubSelectCmb/////////////////////////////////////////////////////////
FSubSelectCmb::FSubSelectCmb(QHash<QString, QString> subNames, QWidget *parent):QComboBox(parent)
{
    QList<QString> codes = subNames.keys();
    qSort(codes.begin(),codes.end());
    foreach(QString code, codes){
        addItem(subNames.value(code),code);
    }
}

void FSubSelectCmb::setSubCode(QString code)
{
    int index = findData(code);
    setCurrentIndex(index);
}

QString FSubSelectCmb::getSubCode()
{
    if(currentIndex() == -1)
        return "";
    return itemData(currentIndex()).toString();
}


////////////////////////////BooleanSelectCmb////////////////////////////////
BooleanSelectCmb::BooleanSelectCmb(QStringList signs, QStringList displays, QWidget *parent):
    QComboBox(parent)
{
    if(signs.count() != 2 || displays.count() != 2){
        this->displays.clear();
        this->signs.clear();
        this->displays<<"true"<<"false";
        this->signs<<"+"<<"-";
    }
    else{
        this->displays = displays;
        this->signs = signs;
    }
    for(int i=0; i < 2; ++i)
        addItem(displays.at(i));
}

bool BooleanSelectCmb::getValue()
{
    if(currentIndex() == 0)
        return true;
    else
        return false;
}

QString BooleanSelectCmb::getDisplay()
{
    return displays.at(currentIndex());
}

QString BooleanSelectCmb::getSign()
{
    return signs.at(currentIndex());
}

void BooleanSelectCmb::setValue(bool v)
{
    if(v)
        setCurrentIndex(0);
    else
        setCurrentIndex(1);
}

////////////////////////////////SubSysJoinCfgItemDelegate/////////////////////////////////////////
SubSysJoinCfgItemDelegate::SubSysJoinCfgItemDelegate(QHash<QString, QString> subNames, QStringList dispStrs, QStringList signStrs, QObject *parent)
    :QItemDelegate(parent),subNames(subNames),readOnly(false),slSigns(signStrs),slDisps(dispStrs)
{
}

QWidget *SubSysJoinCfgItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //只为目标科目系统的科目名称列创建编辑器
    int col = index.column();
    if(readOnly || col != 2 && col != 4)
        return NULL;
    if(col == 4){
        FSubSelectCmb* cmb = new FSubSelectCmb(subNames,parent);
        return cmb;
    }
    else{
        BooleanSelectCmb* cmb = new BooleanSelectCmb(slSigns,slDisps,parent);
        return cmb;
    }
}

void SubSysJoinCfgItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    if(col == 4){
        FSubSelectCmb* cmb = qobject_cast<FSubSelectCmb*>(editor);
        if(!cmb)
            return;
        QString code = index.model()->data(index.model()->index(index.row(),index.column()-1),Qt::EditRole).toString();
        cmb->setSubCode(code);
    }
    else if(col == 2){
        BooleanSelectCmb* cmb = qobject_cast<BooleanSelectCmb*>(editor);
        if(!cmb)
            return;
        QString t = index.model()->data(index).toString();
        int idx = slSigns.indexOf(t);
        cmb->setValue(idx == 0);
    }
}

void SubSysJoinCfgItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    int col = index.column();
    if(col == 4){
        FSubSelectCmb* cmb = qobject_cast<FSubSelectCmb*>(editor);
        if(!cmb)
            return;
        QString code = cmb->getSubCode();
        model->setData(model->index(index.row(),index.column()-1),code);
        model->setData(index,subNames.value(code));
    }
    else if(col == 2){
        BooleanSelectCmb* cmb = qobject_cast<BooleanSelectCmb*>(editor);
        if(!cmb)
            return;
        bool v = cmb->getValue();
        model->setData(index,v?slSigns.at(0):slSigns.at(1));
    }
}

void SubSysJoinCfgItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}

