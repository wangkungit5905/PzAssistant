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


////////////////////////////SummaryEdit/////////////////////////
SummaryEdit::SummaryEdit(int row,int col,QWidget* parent) : QLineEdit(parent)
{
    this->row = row;
    this->col = col;
    oppoSubject = 0;
    connect(this, SIGNAL(returnPressed()),
            this, SLOT(summaryEditingFinished())); //输入焦点自动转移到右边一列
    //经过测试发现，在这种临时部件上不能捕获快捷键，用alt键序列，在按下alt键后会失去光标而关闭编辑器，用ctrl键序列无反映
    //shortCut = new QShortcut(QKeySequence(PZEDIT_COPYPREVROW),this);
    //shortCut->setContext(Qt::WidgetShortcut);
    //shortCut = new QShortcut(QKeySequence(tr("Ctrl+=")),this,0,0,Qt::WidgetShortcut);
    //shortCut = new QShortcut(QKeySequence(tr("Ctrl+P")),this);
    //connect(shortCut,SIGNAL(activated()),this,SLOT(shortCutActivated()));
}

//设置内容
void SummaryEdit::setContent(QString content)
{
    parse(content);
    setText(summary);
}

//取得摘要的所有内容（包括摘要信息体，引用体）
QString SummaryEdit::getContent()
{
    return assemble();
}

//解析内容
void SummaryEdit::parse(QString content)
{
    QString summ,xmlStr;
    int start = content.indexOf("<");
    int end   = content.lastIndexOf(">");
    if((start > 0) && (end > start)){
        summ = content.left(start);               //摘要体部分
        summary = summ;

        xmlStr = content.right(content.count() - start); //引用体部分
        QDomDocument dom;
        bool r = dom.setContent(xmlStr);
        //发票号部分
        if(dom.elementsByTagName("fp").count() > 0){
            QDomNode node = dom.elementsByTagName("fp").at(0);
            fpNums = node.childNodes().at(0).nodeValue().split(",");
        }
        //银行票据号部分
        if(dom.elementsByTagName("bp").count() > 0){
            QDomNode node = dom.elementsByTagName("bp").at(0);
            bankNums = node.childNodes().at(0).nodeValue();
        }
        //对方科目部分
        if(dom.elementsByTagName("op").count() > 0){
            QDomNode node = dom.elementsByTagName("op").at(0);
            oppoSubject = node.childNodes().at(0).nodeValue().toInt();
        }
    }
    else{
        summary = content;
    }
}

//装配内容（包括摘要信息体，引用体）
QString SummaryEdit::assemble()
{
    summary = text();
    bool isHave = false;
    QDomDocument dom;
    QDomElement root = dom.createElement("rp");
    QString fpStr = fpNums.join(",");
    //QString bpStr = ui->edtBankNum->text();

    //发票号部分
    if(fpStr != ""){
        isHave = true;
        QDomText fpText = dom.createTextNode(fpStr);
        QDomElement fpEle = dom.createElement("fp");
        fpEle.appendChild(fpText);
        root.appendChild(fpEle);
    }
    //银行票据号部分
    if(bankNums != ""){
        isHave = true;
        QDomText bpText = dom.createTextNode(bankNums);
        QDomElement bpEle = dom.createElement("bp");
        bpEle.appendChild(bpText);
        root.appendChild(bpEle);
    }
    //对方科目部分
    if(oppoSubject != 0){
        isHave = true;
        QDomText opText = dom.createTextNode(QString::number(oppoSubject));
        QDomElement opEle = dom.createElement("op");
        opEle.appendChild(opText);
        root.appendChild(opEle);
    }

    //QString s;
    if(isHave){
        dom.appendChild(root);
        QString s = QString("%1%2").arg(summary).arg(dom.toString());
        s.chop(1);
        return s;
    }
    else
        return summary;
}

//摘要部分修改结束了
void SummaryEdit::summaryEditingFinished()
{
    summary = text();
    emit dataEditCompleted(0,true);
}

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
    }
    else
        QLineEdit::keyPressEvent(event);

//    int key = event->key();
//    bool alt = false;

//    if(event->modifiers() == Qt::AltModifier)
//        alt = true;

//    //为简单起见，目前的实现仅可输入发票号
//    if(alt){
//        bool ok;
//        QString s = QInputDialog::getText(this, tr("input invoice number"),
//                                          tr("invoice number:"), QLineEdit::Normal,
//                                          QString(), &ok);
//        if(ok){
//            fpNums.clear();
//            fpNums = s.split(",");
//        }
//    }

//    if(alt && (key == Qt::Key_1)){ //打开发票号输入窗口

//       event->accept();
//    }
//    else if(alt && (key == Qt::Key_2)){ //打开银行票据号输入窗口

//        event->accept();
//    }
//    else if(alt && (key == Qt::Key_3)){ //打开对方科目输入窗口

//        event->accept();
//    }
//    else
//        event->ignore();


//    QLineEdit::keyPressEvent(event);
}

//void SummaryEdit::mouseDoubleClickEvent(QMouseEvent *e)
//{
//    int i = 0;
//    e->ignore();
//}

//void SummaryEdit::focusOutEvent(QFocusEvent* e)
//{
    //QLineEdit::focusOutEvent(e);

    //这个函数在QItemDelegate::setModelData()之后得到调用，
    //已经失去了获取最新编辑数据的时机，不能达到在编辑器失去焦点后保存数据到模型的目的
//    if(e->lostFocus()){
//        summary = text();
//        emit dataEditCompleted(0, false);
//    }

//    QLineEdit::focusOutEvent(e);
//}


////////////////////////////SndSubComboBox//////////////////////////
SndSubComboBox::SndSubComboBox(SecondSubject* ssub, FirstSubject* fsub, SubjectManager *subMgr,
                               int row,int col,QWidget *parent):
    QWidget(parent),subMgr(subMgr),ssub(ssub),fsub(fsub),row(row),col(col),
    sortBy(SORTMODE_NAME),textChangeReson(true)/*,editFinished(false)*/
{
    com = new QComboBox(this);
    com->setEditable(true);       //使其可以输入新的名称条目
    lw = new QListWidget(this);
    lw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(lw,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(itemSelected(QListWidgetItem*)));

    keys = new QString;
    expandHeight = 200;
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
            addItem(sub->getName(),v);
        }
    }
    if(ssub){
        v.setValue(ssub);
        com->setCurrentIndex(com->findData(v,Qt::UserRole));
    }
    connect(com,SIGNAL(activated(int)),this,SLOT(subSelectChanged(int)));
    connect(com->lineEdit(),SIGNAL(textEdited(QString)),this,SLOT(nameTextChanged(QString)));
    connect(com->lineEdit(),SIGNAL(returnPressed()),this,SLOT(nameTexteditingFinished()));

    //装载所有名称条目
    //foreach(SubjectNameItem* ni, subMgr->getAllNameItems())
    //    allNIs<<ni;
    allNIs = subMgr->getAllNameItems();
    qSort(allNIs.begin(),allNIs.end(),byNameThan_ni);
    QListWidgetItem* item;
    foreach(SubjectNameItem* ni, allNIs){
        v.setValue(ni);
        item = new QListWidgetItem(ni->getShortName());
        item->setData(Qt::UserRole, v);
        lw->addItem(item);
    }
}

/**
 * @brief SndSubComboBox::hideList 隐藏和显示智能提示列表框
 * @param isHide
 */
void SndSubComboBox::hideList(bool isHide)
{
    lw->setHidden(isHide);
    if(isHide)
        resize(width(),height() - expandHeight);
    else
        resize(width(),height() + expandHeight);
}

/**
 * @brief SndSubComboBox::setSndSub 设置二级科目对象
 * @param sub
 */
void SndSubComboBox::setSndSub(SecondSubject *sub)
{
    ssub = sub;
    QVariant v;
    v.setValue(ssub);
    com->setCurrentIndex(com->findData(v,Qt::UserRole));
}

///**
// * @brief SndSubComboBox::setRowColNum 设置编辑器激活时所处的行列号
// * @param row
// * @param col
// */
//void SndSubComboBox::setRowColNum(int row, int col)
//{
//    this->row = row;
//    this->col = col;
//}



void SndSubComboBox::keyPressEvent(QKeyEvent *e)
{
    static bool isDigit = true;  //true：输入的是科目的数字代码，false：科目的助记符

    if((e->modifiers() != Qt::NoModifier) &&
       (e->modifiers() != Qt::KeypadModifier)){
        e->ignore();
        QWidget::keyPressEvent(e);
        return;
    }

    int keyCode = e->key();
    //QKeyEvent* keyEvent;

    //如果是字母键，则输入的是科目助记符，则按助记符快速定位
    //字母键只有在输入法未打开的情况下才会接收到
    QListWidgetItem* item;
    if(((keyCode >= Qt::Key_A) && (keyCode <= Qt::Key_Z))){
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
    else if(lw->isVisible() && (keys->size() > 0)){
        int startRow;
        switch(keyCode){
        case Qt::Key_Backspace:  //退格键，则维护键盘字符缓存，并进行重新定位
            keys->chop(1);
            if(keys->size() == 0){
                hideList(true);
            }
            else{
                if(isDigit)
                    filterListItem();
                else
                    filterListItem();
            }
            break;
        //因为在提示列表框内装载了隐藏的项目，所以在用上下箭头导航时，会有跳跃、定位不准等不足，
        //拟采用在每次输入一个数字或字母后，重新装载的方式，先在一级科目编辑器中实验
        case Qt::Key_Up:
            //LOG_INFO("press up arrow!!!");
            startRow = lw->currentRow();
            if(startRow == 0)
                break;
            startRow--;
            for(startRow; startRow > -1; startRow--){
                if(!lw->item(startRow)->isHidden()){
                    lw->setCurrentRow(startRow);
                    lw->scrollToItem(lw->item(startRow),QAbstractItemView::PositionAtCenter);
                    break;
                }
            }
            break;
        case Qt::Key_Down:
            //LOG_INFO("press down arrow!!!");
            startRow = lw->currentRow();
            if(startRow == lw->count()-1)
                break;
            startRow++;
            for(startRow; startRow < lw->count(); ++startRow){
                if(!lw->item(startRow)->isHidden()){
                    lw->setCurrentRow(startRow);
                    lw->scrollToItem(lw->item(startRow),QAbstractItemView::PositionAtCenter);
                    break;
                }
            }
            break;
        case Qt::Key_Return:  //回车键
        case Qt::Key_Enter:
            LOG_INFO("when smart list vistble press enter key! emit dataEditCompleted() signal!");
            enterKeyWhenHide();
            LOG_INFO("enter key processed !");
            //itemSelected(lw->currentItem());
            //emit dataEditCompleted(BT_SNDSUB,true);
            e->accept();
            //e->ignore();
            break;
        }
    }
    else if(!lw->isVisible() && keyCode != Qt::Key_Return && keyCode != Qt::Key_Enter){
        int index,ItemCount;
        switch(keyCode){
        case Qt::Key_Up:
            //LOG_INFO("press up arrow!!!");
            index = currentIndex();
            ItemCount = com->count();
            if(index > 0)
                setCurrentIndex(index-1);
            break;
        case Qt::Key_Down:
            //LOG_INFO("press down arrow!!!");
            index = currentIndex();
            ItemCount = com->count();
            if(index < ItemCount-1)
                setCurrentIndex(index+1);
            break;
        }
    }

    //在智能提示框没有出现时
    else if((keyCode == Qt::Key_Return) || (keyCode == Qt::Key_Enter)){
        LOG_INFO("when smart list hide press enter key! emit dataEditCompleted() signal!");

        //if(!editFinished){
        //    LOG_INFO(QString("editFinished: %1").arg(editFinished));
            emit dataEditCompleted(BT_SNDSUB,true);
        //}
        //e->accept();
    }
    else
        QWidget::keyPressEvent(e);
}

/**
 * @brief SndSubComboBox::itemSelected 当用户在智能提示框中选择一个名称条目时
 * @param item
 */
void SndSubComboBox::itemSelected(QListWidgetItem *item)
{
    LOG_INFO("enter SndSubComboBox::itemSelected() function!");
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
        LOG_INFO("after emit newMappingItem signal!");
        if(ssub){
            LOG_INFO("create new sndsub after emit newMappingItem signal!");
            QVariant v;
            v.setValue(ssub);
            com->addItem(ssub->getName(),v);
            com->setCurrentIndex(com->count()-1);
        }
        //emit dataEditCompleted(BT_SNDSUB,false);
    }
    LOG_INFO("before hideList()");
    hideList(true);
    LOG_INFO("after hideList()");
}

/**
 * @brief SndSubComboBox::nameItemTextChanged
 * 当组合框内的文本编辑区域的文本（用户输入名称条目时）发生改变
 * @param text
 */
void SndSubComboBox::nameTextChanged(const QString &text)
{
    //这种方式是用户手动输入科目文本，则智能提示列表框按名称条目的字符顺序来显示
    //LOG_INFO(QString("enter nameItemTextChanged()! Changed text %1").arg(text));

    if(sortBy != SORTMODE_NAME)
        return;
    filterListItem();
    hideList(false);
    textChangeReson = false;
}

/**
 * @brief SndSubComboBox::nameTexteditingFinished
 * 用户提交输入的名称条目文本
 */
void SndSubComboBox::nameTexteditingFinished()
{
    //LOG_INFO("enter nameTexteditingFinished()");
    if(textChangeReson)
        return;

    //遍历智能提示列表框，找出名称完全匹配的条目
    //LOG_INFO(QString("start look up name(%1)").arg(com->lineEdit()->text()));
    QListWidgetItem* item;
    SubjectNameItem* ni;
    QString editText = com->lineEdit()->text();
    for(int i = 0; i < lw->count(); ++i){
        item = lw->item(i);
        if(item->isHidden())
            continue;
        ni = item->data(Qt::UserRole).value<SubjectNameItem*>();
        QString name = ni->getShortName();
        if(editText == name){
            itemSelected(item);
            LOG_INFO(QString("Fonded name item: %1").arg(name));
            return;
        }
    }
    //如果没有找到，则触发新名称条目信号
    //LOG_INFO(QString("emit newSndSubject(%1,%2,%3,%4) signal!")
    //         .arg(fsub->getName()).arg(editText).arg(row).arg(col));
    SecondSubject* ssub = NULL;
    emit newSndSubject(fsub,ssub,editText,row,col);
    //editFinished = true;
    LOG_INFO(QString("after emit newSndSubject() signal!"));
    //LOG_INFO(QString("create new Second Subject(%) !").arg(ssub->getName()));
    //由于编辑器已经关闭，后续处理已经没有意义
    if(ssub){
        LOG_INFO(QString("new second subject has created(name=%1)").arg(ssub->getName()));
        QVariant v;
        v.setValue(ssub);
        com->addItem(ssub->getName(),v);
        com->setCurrentIndex(com->count()-1);
    }
    //emit dataEditCompleted(BT_SNDSUB,false);
}

/**
 * @brief SndSubComboBox::subSelectChanged
 * 当用户通过从组合框的下拉列表中选择一个科目时
 * @param text
 */
void SndSubComboBox::subSelectChanged(const int index)
{
    //LOG_INFO(QString("User select subject(index=%1,name=%2,count=%3) ")
    //         .arg(index).arg(com->currentText()).arg(com->count()));
    //当在组合框上的编辑框内输入当前不存在的项目时，在按回车键后，它会自动将此新的项目加入到
    //组合框中，引起组合框的项目增加，因此有必要要排除此新输入的项（因为，在按回车键时，也会
    //触发activated或currentIndexChanged信号）
    if(index > -1 && index < com->count()-1)
        textChangeReson = true;
}


//void SndSubComboBox::subSelectChanged(const QString &text)
//{
//    LOG_INFO(QString("User select subject %1").arg(text));
//    textChangeReson = true;
//}

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
    //定位到第一条记录
    for(int i = 0; i < allNIs.count(); ++i){
        if(!lw->item(i)->isHidden()){
            lw->setCurrentRow(i);
            //LOG_INFO("Fonded first match name !!!");
            return;
        }
    }
    //选中状态在UI中没有反馈？？？
    //if(lw->count()>0){
    //    LOG_INFO("set current row to top after filter!");
    //    lw->setCurrentRow(0);
        //lw->setCurrentRow(0,QItemSelectionModel::ClearAndSelect); //选中第一行
    //}
}

void SndSubComboBox::enterKeyWhenHide()
{
    //LOG_INFO("enter SndSubComboBox::enterKeyWhenHide()");
    //LOG_INFO(QString("fsub=%1").arg(fsub->getName()));
    if(!fsub)
        return;
    SubjectNameItem* ni = lw->currentItem()->data(Qt::UserRole).value<SubjectNameItem*>();
    //LOG_INFO(QString("ni=%1").arg(ni->getShortName()));
    if(!ni)
        return;
    //LOG_INFO("test fsub->containChildSub(ni)");
    if(fsub->containChildSub(ni)){
        //LOG_INFO("contained");
        ssub = fsub->getChildSub(ni);
        QVariant v;
        v.setValue(ssub);
        int index = com->findData(v,Qt::UserRole);
        com->setCurrentIndex(index);
    }
    else if(subMgr->containNI(ni)){
        //LOG_INFO("subMgr has name item");
        SecondSubject* ssub = NULL;
        emit newMappingItem(fsub,ni,ssub,row,col);
        //LOG_INFO("after emit newMappingItem signal!");

        //在触发此信号后，由于处理此信号要弹出一个消息确认框，将导致编辑器失去焦点而不可用
        //因此，后续的操作不能再访问编辑器（比如编辑器内的可见部件等）
        //处理此信号的槽（pzdialog::creatNewNameItemMapping）会通过Undo栈修改当前会计分录的二级科目并刷新
        //因为编辑器的提早关闭，也因此无法将最新创建的映射通过编辑器反馈到PzDialog::pzDateChanged()函数
        //也由于编辑器已经关闭，因此也无法触发dataEditCompleted信号达到将输入焦点移动到下一列
        //if(ssub){
        //    LOG_INFO("create new sndsub after emit newMappingItem signal!");
        //    QVariant v;
        //    v.setValue(ssub);
        //    com->addItem(ssub->getName(),v);
        //    com->setCurrentIndex(com->count()-1);
        //}
        //emit dataEditCompleted(BT_SNDSUB,false);
    }
    //LOG_INFO("before hideList()");
    //hideList(true);
    //LOG_INFO("after hideList()");
}

//    : QComboBox(parent),pid(pid),subMgr(subMgr)
//SndSubComboBox2::SndSubComboBox2(int pid, SubjectManager *subMgr, QWidget *parent)
//    : QComboBox(parent),pid(pid),subMgr(subMgr)
//{
//    QList<int> ids;
//    QList<QString> names;
//    if(pid != 0){
//        subMgr->getSndSubInSpecFst(pid, ids, names);
//        for(int i = 0; i < ids.count(); ++i)
//            addItem(names[i], ids[i]);
//    }
//    else{
//        QHashIterator<int,QString> it(allSndSubs);
//        while(it.hasNext()){
//            it.next();
//            addItem(it.value(), it.key());
//        }
//    }

//    setEditable(true);  //使其可以输入新的二级科目名
//    //BusiUtil::getAllSndSubNameList(snames);
//    //lineEdit()->setCompleter(new QCompleter(snames, this));
//    model = new QSqlQueryModel;
//    model->setQuery("select FSAgent.id,FSAgent.fid,FSAgent.sid,"
//                    "FSAgent.subCode,FSAgent.FrequencyStat,SecSubjects.subName "
//                    "from FSAgent join SecSubjects on FSAgent.sid = "
//                    "SecSubjects.id order by FSAgent.subCode");
//    keys = new QString;
//    listview = new QListView(parent);

//    //应该使用取自SecSubjects表的model
//    smodel = new QSqlTableModel;
//    smodel->setTable("SecSubjects");
//    smodel->setSort(SNDSUB_REMCODE, Qt::AscendingOrder);
//    listview->setModel(smodel);
//    listview->setModelColumn(SNDSUB_SUBNAME);
//    smodel->select();

//    //因为QSqlTableModel不会一次提取所有的记录，因此，必须用如下方法
//    //获取真实的行数，但基于性能的原因，不建议使用如下的方法，而直接用一个
//    //QSqlQuery来提取实际的行数
//    //while (smodel->canFetchMore())
//    //{
//    //    smodel->fetchMore();
//    //}
//    //rows = smodel->rowCount();

//    QSqlQuery q;
//    bool r = q.exec("select count() from SecSubjects");
//    r = q.first();
//    rows = q.value(0).toInt();

//    listview->setFixedHeight(150); //最好是设置一个与父窗口的高度合适的尺寸
//    //QPoint globalP = mapToGlobal(parent->pos());
//    //listview->move(parent->x(),parent->y() + this->height());

//}

//SndSubComboBox2::~SndSubComboBox2()
//{
//    delete listview;
//    delete keys;
//}

////设置编辑器激活时所处的行列号
//void SndSubComboBox2::setRowColNum(int row, int col)
//{
//    this->row = row;
//    this->col = col;
//}

////void SndSubComboBox::focusOutEvent(QFocusEvent* e)
////{
////    listview->hide();
////    QComboBox::focusOutEvent(e);
////}

//void SndSubComboBox2::keyPressEvent(QKeyEvent* e)
//{
//    static int i = 0;
//    static bool isDigit = true;  //true：输入的是科目的数字代码，false：科目的助记符
//    int sid;   //在SndSubClass表中的ID
//    int index;
//    int idx,c;

//    if((e->modifiers() != Qt::NoModifier) &&
//       (e->modifiers() != Qt::KeypadModifier)){
//        e->ignore();
//        QComboBox::keyPressEvent(e);
//        return;
//    }

//    int keyCode = e->key();

//    //如果是字母键，则输入的是科目助记符，则按助记符快速定位
//    //字母键只有在输入法未打开的情况下才会接收到
//    if(((keyCode >= Qt::Key_A) && (keyCode <= Qt::Key_Z))){
//        keys->append(keyCode);
//        if(keys->size() == 1){ //接收到第一个字符，需要重新按科目助记符排序，并装载到列表框
//            isDigit = false;
//            smodel->select();
//            while (smodel->canFetchMore())
//            {
//                smodel->fetchMore();
//            }
//            i = 0;
//            listview->setMinimumWidth(width());
//            listview->setMaximumWidth(width());
//            QPoint p(0, height());
//            int x = mapToParent(p).x();
//            int y = mapToParent(p).y() + 1;
//            listview->move(x, y);
//            listview->show();
//        }
//        //定位到最匹配的条目
//        QString remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
//        while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
//            i++;
//            remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
//        }
//        if(i < rows)
//            listview->setCurrentIndex(smodel->index(i, SNDSUB_SUBNAME));

//        //e->accept();

//    }
//    //如果是数字键则键入的是科目代码，则按科目代码快速定位
//    else if((keyCode >= Qt::Key_0) && (keyCode <= Qt::Key_9)){
////        keys->append(keyCode);
////        isDigit = true;
////        if(keys->size() == 1)
////            i = 0;
////        showPopup();
////        //定位到最匹配的科目
////        QString subCode = model->data(model->index(i, 3)).toString();
////        int rows = model->rowCount();
////        while((keys->compare(subCode) > 0) && (i < rows)){
////            i++;
////            subCode = model->data(model->index(i, 3)).toString();
////        }
////        if(i < rows)
////            setCurrentIndex(i);
////        e->accept();

//    }
//    //如果是其他编辑键
//    else if(listview->isVisible() || (keys->size() > 0)){
//        int id; //FSAgent表的id值
//        switch(keyCode){
//        case Qt::Key_Backspace:  //退格键，则维护键盘字符缓存，并进行重新定位
//            keys->chop(1);
//            if(keys->size() == 0)
//                listview->hide();
//            if((keys->size() == 0) && isDigit)
//                hidePopup();
//            else{ //用遗留的字符根据是数字还是字母再重新进行依据科目代码或助记符进行定位
//                if(isDigit){
//                    i = 0;
//                    QString subCode = model->data(model->index(i, 3)).toString();
//                    int rows = model->rowCount();
//                    while((keys->compare(subCode) > 0) && (i < rows)){
//                        i++;
//                        subCode = model->data(model->index(i, 3)).toString();
//                    }
//                    if(i < rows)
//                        listview->setCurrentIndex(model->index(i, 3));
//                }
//                else{
//                    i = 0;
//                    QString remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
//                    //int rows = smodel->rowCount();
//                    while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
//                        i++;
//                        remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
//                    }
//                    if(i < rows)
//                        listview->setCurrentIndex(smodel->index(i, SNDSUB_SUBNAME));
//                }
//            }
//            break;

//        case Qt::Key_Up:
//            if(listview->currentIndex().row() > 0){
//                listview->setCurrentIndex(smodel->index(listview->currentIndex().row() - 1, SNDSUB_SUBNAME));
//            }
//            break;

//        case Qt::Key_Down:
//            idx = listview->currentIndex().row();
//            //c = smodel->rowCount();
//            if( idx < rows - 1){
//                listview->setCurrentIndex(smodel->index(idx + 1, SNDSUB_SUBNAME));
//            }
//            break;

//        case Qt::Key_Return:  //回车键
//        case Qt::Key_Enter:
//            index = listview->currentIndex().row();
//            sid = smodel->data(smodel->index(index, SNDSUB_ID)).toInt();

//            //在当前一级科目下没有任何二级科目的映射，或者如果在当前的smodel中
//            //找不到此sid值，说明当前一级科目和选择的二级科目没有对应的映射条目

//            if(!findSubMapper(pid, sid, id)){
//                listview->hide();
//                keys->clear();
//                emit dataEditCompleted(2,true);
//                emit newMappingItem(pid, sid, row, col);
//            }
//            else{
//                listview->hide();
//                keys->clear();
//                setCurrentIndex(findData(id));
//                emit dataEditCompleted(2,true);
//                emit editNextItem(row,col);
//            }
//            break;
//        }
//        //e->accept();
//    }
//    //在智能提示框没有出现时输入的文本（中文文本），需要在SecSubjects表中进行查找，
//    //如果不能找到一个匹配的，这说明是一个新的二级科目
//    else if((keyCode == Qt::Key_Return) || (keyCode == Qt::Key_Enter)){
//            QString name = currentText();
//            name.simplified();
//            if(name != ""){
//                int sid;
//                if(!findSubName(name,sid)){
//                    emit dataEditCompleted(2,true);
//                    emit newSndSubject(pid, name, row, col);
//                }
//                else{ //如果找到，还要检测是否存在一二级科目的对应映射关系
//                    int id = 0;
//                    subMgr->getFstToSnd(pid,sid,id);
//                    if(id == 0){ //不存在对应映射关系
//                        emit dataEditCompleted(2,true);
//                        emit newMappingItem(pid,sid,row,col);
//                    }
//                    else{
//                        setCurrentIndex(findData(id));
//                        emit dataEditCompleted(2,true);
//                    }
//                }
//            }
//            else     //明细科目不能为空（但这样处置，却不能阻止关闭编辑器）
//                return;

//            //e->accept();
//    }
//    else
//        //e->ignore();
//        QComboBox::keyPressEvent(e);
//}

////在this.model中查找是否有此一级和二级科目的对应条目
//bool SndSubComboBox2::findSubMapper(int fid, int sid, int& id)
//{
//    int c = model->rowCount();
//    bool founded = false;

//    if(c > 0){
//        int i = 0;
//        while((i < c) && !founded){
//            int fv = model->data(model->index(i,1)).toInt();
//            int sv = model->data(model->index(i, 2)).toInt();
//            if((fv == fid) && (sv == sid)){
//                founded = true;
//                id = model->data(model->index(i,0)).toInt();
//            }
//            i++;
//        }
//    }
//    return founded;
//}

////在二级科目表SecSubjects中查找是否存在名称为name的二级科目
//bool SndSubComboBox2::findSubName(QString name, int& sid)
//{
//    QSqlQuery q;
//    QString s = QString("select id from SecSubjects "
//                        "where subName = '%1'").arg(name);
//    if(q.exec(s) && q.first()){
//        sid = q.value(0).toInt();
//        return true;
//    }
//    else
//        return false;
//}



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

//void MoneyTypeComboBox::focusOutEvent(QFocusEvent* e)
//{
//    QComboBox::focusOutEvent(e);
//}

//实现输入货币代码即定位到正确的货币索引
void MoneyTypeComboBox::keyPressEvent(QKeyEvent* e )
{
    int key = e->key();
    if((key >= Qt::Key_0) && (key <= Qt::Key_9)){
        int mcode = e->text().toInt();
        int idx = findData(mcode);
        if(idx != -1){
            setCurrentIndex(idx);
            //emit dataEditCompleted(3,true);
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
    validator  = new QDoubleValidator(this);
    validator->setDecimals(2);
    //setValidator(validator);
    connect(this,SIGNAL(textChanged(QString)),this,SLOT(valueChanged(QString)));
    //connect(this,SIGNAL(editingFinished()),this,SLOT(valueEdited()));
}

void MoneyValueEdit::setValue(Double v)
{
    this->v = v;
    setText(v.toString());
}

void MoneyValueEdit::keyPressEvent(QKeyEvent *e)
{
    //在Qt5下，如果编辑器被设置了验证器，则在文本为空时，将接收不到任何按键信息，不知为何？
    //因此，这里的实现只能自己利用浮点数验证器对象进行手工验证
    int key = e->key();
    if((key == Qt::Key_Return) || (key == Qt::Key_Enter)){
        v = text().toDouble();
        if(witch == 1)  //借方金额栏
            emit dataEditCompleted(BT_JV, true);
        else
            emit dataEditCompleted(BT_DV, true);
        if(witch == 0)
            emit nextRow(row);  //传播给代理，代理再传播给凭证编辑窗
        emit editNextItem(row,col);
    }
    else if((key >= Qt::Key_0 && key <= Qt::Key_9) || key == Qt::Key_Period){
        QString t = text()+ e->text();
        int i = 0;
        QValidator::State state = validator->validate(t,i);
        if(state == QValidator::Acceptable)
            QLineEdit::keyPressEvent(e);
    }
    else if(key == Qt::Key_Minus && cursorPosition() == 0)
        QLineEdit::keyPressEvent(e);
    else if(key == Qt::Key_Backspace)
        QLineEdit::keyPressEvent(e);
    else if(key == Qt::Key_Up || key == Qt::Key_Down)
        QLineEdit::keyPressEvent(e);
}

void MoneyValueEdit::valueChanged(const QString &text)
{
    v = text.toDouble();
    //qDebug()<<QString("value is %1").arg(v.toString());
}


//void MoneyValueEdit::valueEdited()
//{
//    //v = text().toDouble();
//    if(witch == 1)  //借方金额栏
//        emit dataEditCompleted(BT_JV, true);
//    else
//        emit dataEditCompleted(BT_DV, true);
//    if(witch == 0)
//        emit nextRow(row);  //传播给代理，代理再传播给凭证编辑窗
//    emit editNextItem(row,col);
//}


////////////////////////////////ActionEditItemDelegate2/////////////////
ActionEditItemDelegate::ActionEditItemDelegate(SubjectManager *subMgr, QObject *parent):
    QItemDelegate(parent),subMgr(subMgr)
{
    isReadOnly = false;
    statUtil = NULL;
}

//设置其代理的表格项的只读模式（这个函数用于支持表格的只读模式）
//void ActionEditItemDelegate::setReadOnly(bool readOnly)
//{
//    isReadOnly = readOnly;
//}

//
//void ActionEditItemDelegate::setVolidRows(int rows)
//{
//    this->validRows = rows;
//}

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
            //FstSubComboBox* editor = new FstSubComboBox(subMgr, parent);
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
        //return QItemDelegate::createEditor(parent,option,index);
        return 0;
}

void ActionEditItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    if(col == BT_SUMMARY){
       SummaryEdit* edit = qobject_cast<SummaryEdit*>(editor);
       if (edit) {
           edit->setContent(index.model()->data(index, Qt::EditRole).toString());
       }
    }
    else if(col == BT_FSTSUB){
        //FstSubComboBox* cmb = qobject_cast<FstSubComboBox*>(editor);
        FstSubEditComboBox* cmb  = qobject_cast<FstSubEditComboBox*>(editor);
        if(cmb){
            //FirstSubject* fsub = index.model()->
            //        data(index, Qt::EditRole).value<FirstSubject*>();
            int idx = cmb->findData(index.model()->
                                    data(index, Qt::EditRole), Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if(col == BT_SNDSUB){
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            SecondSubject* ssub = index.model()->data(index, Qt::EditRole)
                    .value<SecondSubject*>();
            cmb->setSndSub(ssub);
            FirstSubject* fsub = NULL;
            if(!ssub){  //如果未设置二级科目则通过模型获取一级科目
                fsub = index.model()->data(index.sibling(index.row(),index.column()-1), Qt::EditRole).value<FirstSubject*>();
            }
            else
                fsub = ssub->getParent();
            if(!fsub)
                return;
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
            if(edit->getContent() != model->data(index,Qt::EditRole))
                model->setData(index, edit->getContent());
        }
    }
    else if(col == BT_FSTSUB){
        //FstSubComboBox* cmb = qobject_cast<FstSubComboBox*>(editor);
        FstSubEditComboBox* cmb = qobject_cast<FstSubEditComboBox*>(editor);
        if(cmb){
            FirstSubject* fsub = cmb->itemData(cmb->currentIndex(), Qt::UserRole).value<FirstSubject*>();
            if(fsub != model->data(index,Qt::EditRole).value<FirstSubject*>())
                model->setData(index, cmb->itemData(cmb->currentIndex()));
        }
    }
    else if(col == BT_SNDSUB){
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            //int idx = cmb->currentIndex();
            //LOG_INFO(QString("Delegate::setModelData,ssub index = %1").arg(idx));
            SecondSubject* ssub = cmb->itemData(cmb->currentIndex(), Qt::UserRole).value<SecondSubject*>();
            if(ssub != model->data(index,Qt::EditRole).value<SecondSubject*>())
                model->setData(index,cmb->itemData(cmb->currentIndex()));


        }
    }
    else if(col == BT_MTYPE){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        if(cmb){
            Money* mt = cmb->getMoney();
            Money* mmt = model->data(index,Qt::EditRole).value<Money*>();
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
        //editor = qobject_cast<FstSubComboBox*>(sender());
        editor = qobject_cast<FstSubEditComboBox*>(sender());
    else if(colIndex == BT_SNDSUB){
        editor = qobject_cast<SndSubComboBox*>(sender());
        //LOG_INFO("SndSubComboBox commit data!");
        //LOG_INFO(QString("SndSub editor is %1").arg(editor?"NoNUll":NULL));
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
    //LOG_INFO(QString("Request create new name item mapping!(fsub=%1,name=%2) signal").arg(fsub->getName()).arg(ni->getShortName()));
    LOG_INFO("enter ActionEditItemDelegate::newNameItemMapping()");
    emit crtNewNameItemMapping(row,col,fsub,ni,ssub);

//    if(QMessageBox::information(0,msgTitle_info,tr("Confirm request create new second subject mapping item?\n"
//                                                "first subject:%1\nsecond subject:%2").arg(fsub->getName()).arg(ni->getShortName()),
//                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes){
//        //ssub = fsub->addChildSub(ni);
//        //emit updateSndSubject(row,col,ssub);
//        emit crtNewNameItemMapping(row,col,fsub,ni,ssub);
//    }
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
    if(QMessageBox::information(0,msgTitle_info,tr("确定要用新的名称条目“%1”在一级科目“%2”下创建二级科目吗？")
                                .arg(name).arg(fsub->getName()),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
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

//void ActionEditItemDelegate::cachedExtraException(BusiAction *ba, Double fv, MoneyDirection fd, Double sv, MoneyDirection sd)
//{
//    emit extraException(ba,fv,fd,sv,sd);
//}

void ActionEditItemDelegate::updateEditorGeometry(QWidget* editor,
         const QStyleOptionViewItem &option, const QModelIndex& index) const
{
    //editor->setGeometry(option.rect);

    QRect rect = option.rect;
    //if(index.column() == 2)
    //    rect.setHeight(100);

    editor->setGeometry(rect);
}

//void ActionEditItemDelegate::watchExtraException()
//{
//    if(!statUtil){
//        AccountSuiteManager* pzMgr = subMgr->getAccount()->getPzSet();
//        statUtil = pzMgr->getStatUtil();
//        connect(statUtil,SIGNAL(extraException(BusiAction*,Double,MoneyDirection,Double,MoneyDirection)),
//                this,SLOT(cachedExtraException(BusiAction*,Double,MoneyDirection,Double,MoneyDirection)));
//    }
//}

///////////////////////////////FSubSelectCmb/////////////////////////////////////////////////////////
FSubSelectCmb::FSubSelectCmb(SubjectManager *smg, QWidget *parent):QComboBox(parent)
{
    FirstSubject* fsub;
    fsub = smg->getNullFSub();
    QVariant v;
    v.setValue<FirstSubject*>(fsub);
    addItem(fsub->getName(),v);
    FSubItrator* it = smg->getFstSubItrator();
    while(it->hasNext()){
        it->next();
        fsub = it->value();
        if(fsub->isEnabled()){
            v.setValue<FirstSubject*>(fsub);
            addItem(fsub->getName(),v);
        }
    }
}

void FSubSelectCmb::setSubject(FirstSubject *fsub)
{
    QVariant v;
    v.setValue<FirstSubject*>(fsub);
    setCurrentIndex(findData(v));
}

FirstSubject *FSubSelectCmb::getSubject()
{
    if(currentIndex() == -1)
        return NULL;
    return itemData(currentIndex()).value<FirstSubject*>();
}


////////////////////////////////SubSysJoinCfgItemDelegate/////////////////////////////////////////
SubSysJoinCfgItemDelegate::SubSysJoinCfgItemDelegate(SubjectManager *subMgr, QObject *parent)
    :QItemDelegate(parent),subMgr(subMgr),readOnly(false)
{
}

QWidget *SubSysJoinCfgItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //只为目标科目系统的科目名称列创建编辑器
    if(readOnly || index.column() != 5)
        return NULL;
    FSubSelectCmb* cmb = new FSubSelectCmb(subMgr,parent);
    return cmb;
}

void SubSysJoinCfgItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    FSubSelectCmb* cmb = qobject_cast<FSubSelectCmb*>(editor);
    if(!cmb)
        return;
    FirstSubject* fsub = index.model()->data(index,Qt::EditRole).value<FirstSubject*>();
    if(!fsub)
        return;
    cmb->setSubject(fsub);
//    QVariant v;
//    v.setValue<FirstSubject*>(fsub);
//    int idx = cmb->findData(v);
//    cmb->setCurrentIndex(idx);
}

void SubSysJoinCfgItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    FSubSelectCmb* cmb = qobject_cast<FSubSelectCmb*>(editor);
    if(!cmb)
        return;
    QVariant v;
    FirstSubject* fsub = cmb->getSubject();
    v.setValue<FirstSubject*>(fsub);
    model->setData(index,v);
}

void SubSysJoinCfgItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}






