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


////////////////////////////SummaryEdit/////////////////////////
SummaryEdit2::SummaryEdit2(int row,int col,QWidget* parent) : QLineEdit(parent)
{
    this->row = row;
    this->col = col;
    oppoSubject = 0;
    connect(this, SIGNAL(returnPressed()),
            this, SLOT(summaryEditingFinished())); //输入焦点自动转移到右边一列
    shortCut = new QShortcut(QKeySequence(tr("Ctrl+=")),this);
    connect(shortCut,SIGNAL(activated()),this,SLOT(shortCutActivated()));
}

//设置内容
void SummaryEdit2::setContent(QString content)
{
    parse(content);
    setText(summary);
}

//取得摘要的所有内容（包括摘要信息体，引用体）
QString SummaryEdit2::getContent()
{
    return assemble();
}

//解析内容
void SummaryEdit2::parse(QString content)
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
QString SummaryEdit2::assemble()
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
void SummaryEdit2::summaryEditingFinished()
{
    summary = text();
    emit dataEditCompleted(0,true);
}

//捕获自动复制前一条会计分录的快捷方式
void SummaryEdit2::shortCutActivated()
{
    emit copyPrevShortcutPressed(row,col);
}

//重载此函数的目的是创建一种输入摘要引用体内容的方法
void SummaryEdit2::keyPressEvent(QKeyEvent *event)
{
    //if(event->modifiers() == Qt::AltModifier){

    int key = event->key();
    bool alt = false;

    if(event->modifiers() == Qt::AltModifier)
        alt = true;

    //为简单起见，目前的实现仅可输入发票号
    if(alt){
        bool ok;
        QString s = QInputDialog::getText(this, tr("发票号输入"),
                                          tr("发票号："), QLineEdit::Normal,
                                          QString(), &ok);
        if(ok){
            fpNums.clear();
            fpNums = s.split(",");
        }
    }

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


    QLineEdit::keyPressEvent(event);
}

void SummaryEdit2::mouseDoubleClickEvent(QMouseEvent *e)
{
    int i = 0;
    e->ignore();
}

void SummaryEdit2::focusOutEvent(QFocusEvent* e)
{
    //QLineEdit::focusOutEvent(e);

    //这个函数在QItemDelegate::setModelData()之后得到调用，
    //已经失去了获取最新编辑数据的时机，不能达到在编辑器失去焦点后保存数据到模型的目的
//    if(e->lostFocus()){
//        summary = text();
//        emit dataEditCompleted(0, false);
//    }

    QLineEdit::focusOutEvent(e);
}




/////////////////////////////FstSubComboBox//////////////////////////

FstSubComboBox2::FstSubComboBox2(QWidget *parent) : QComboBox(parent)
{
    QList<int>ids;        //总账科目id（它是以科目代码顺序出现的）
    //QList<QString> names; //总账科目名
    //BusiUtil::getAllFstSub(ids,names);
    SubjectManager* smg = curAccount->getSubjectManager();
    ids = smg->getAllFstSubHash().keys();
    qSort(ids.begin(),ids.end());
    for(int i = 0; i < ids.count(); ++i)
        addItem(smg->getFstSubject(ids.at(i))->getName(), ids.at(i));

    db = curAccount->getDbUtil()->getDb();
    model = new QSqlQueryModel;
    model->setQuery(QString("select * from %1 where %2=1")
                    .arg(tbl_fsub).arg(fld_fsub_isview),db);
    keys = new QString;
    //listview = new QListView(this);//这样作将把listview限制在单元格内
    listview = new QListView(parent);
    listview->setModel(model);
    listview->setModelColumn(FSUB_SUBNAME);
    //connect(listview, SIGNAL(clicked(QModelIndex)),this,SLOT(completeText(QModelIndex)));


}

FstSubComboBox2::~FstSubComboBox2()
{
    delete listview;
    delete keys;
}

//当代理失去焦点时，由代理类的析构函数来完成此功能
//void FstSubComboBox::focusOutEvent(QFocusEvent* e)
//{
//    listview->hide();
//    QComboBox::focusOutEvent(e);
//}

void FstSubComboBox2::keyPressEvent(QKeyEvent* e)
{
    static int i = 0;
    static bool isDigit = true;  //true：输入的是科目的数字代码，false：科目的助记符
    int subId; //在FsAgent表中的ID
    int index;

    if((e->modifiers() != Qt::NoModifier) &&
      (e->modifiers() != Qt::KeypadModifier)){
        e->ignore();
        QComboBox::keyPressEvent(e);
        return;
    }

    int keyCode = e->key();

    //如果是字母键，则输入的是科目助记符，则按助记符快速定位
    if(((keyCode >= Qt::Key_A) && (keyCode <= Qt::Key_Z))){
        keys->append(keyCode);
        if(keys->size() == 1){ //接收到第一个字符，需要重新按科目助记符排序，并装载到列表框
            //model->sort(FSTSUB_REMCODE);
            isDigit = false;
            model->setQuery(QString("select * from %1 where %2=1 order by %3")
                            .arg(tbl_fsub).arg(fld_fsub_isview).arg(fld_fsub_remcode),db);
            i = 0;
            listview->setMinimumWidth(width());
            listview->setMaximumWidth(width());
            QPoint p(0, height());
            int x = mapToParent(p).x();
            int y = mapToParent(p).y() + 1;
            //int x = mapToGlobal(p).x();
            //int y = mapToGlobal(p).y() + 1;
            listview->move(x, y);
            listview->show();
        }
        //定位到最匹配的条目
        QString remCode = model->data(model->index(i, FSUB_REMCODE)).toString();
        int rows = model->rowCount();
        while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
            i++;
            remCode = model->data(model->index(i, FSUB_REMCODE)).toString();
        }
        if(i < rows)
            listview->setCurrentIndex(model->index(i, FSUB_SUBNAME));
    }
    //如果是数字键则键入的是科目代码，则按科目代码快速定位
    else if((keyCode >= Qt::Key_0) && (keyCode <= Qt::Key_9)){
        keys->append(keyCode);
        isDigit = true;
        if(keys->size() == 1){
            model->setQuery(QString("select * from %1 where %2=1 order by %3")
                            .arg(tbl_fsub).arg(fld_fsub_isview).arg(fld_fsub_subcode),db);
            i = 0;
            listview->setMinimumWidth(width());
            listview->setMaximumWidth(width());
            QPoint p(0, height());
            int x = mapToParent(p).x();
            int y = mapToParent(p).y() + 1;
            listview->move(x, y);
            listview->show();
        }
        QString subCode = model->data(model->index(i, FSUB_SUBCODE)).toString();
        int rows = model->rowCount();
        while((keys->compare(subCode) > 0) && (i < rows)){
            i++;
            subCode = model->data(model->index(i, FSUB_SUBCODE)).toString();
        }
        if(i < rows)
            listview->setCurrentIndex(model->index(i, FSUB_SUBNAME));
    }
    //如果是其他编辑键
    else if(listview->isVisible()){
        switch(keyCode){
        case Qt::Key_Backspace:  //退格键，则维护键盘字符缓存，并进行重新定位
            keys->chop(1);
            if(keys->size() == 0)
                listview->hide();
            else{ //用遗留的字符根据是数字还是字母再重新进行依据科目代码或助记符进行定位
                if(isDigit){
                    i = 0;
                    QString subCode = model->data(model->index(i, FSUB_SUBCODE)).toString();
                    int rows = model->rowCount();
                    while((keys->compare(subCode) > 0) && (i < rows)){
                        i++;
                        subCode = model->data(model->index(i, FSUB_SUBCODE)).toString();
                    }
                    if(i < rows)
                        listview->setCurrentIndex(model->index(i, FSUB_SUBNAME));
                }
                else{
                    i = 0;
                    QString remCode = model->data(model->index(i, FSUB_REMCODE)).toString();
                    int rows = model->rowCount();
                    while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
                        i++;
                        remCode = model->data(model->index(i, FSUB_REMCODE)).toString();
                    }
                    if(i < rows)
                        listview->setCurrentIndex(model->index(i, FSUB_SUBNAME));
                }
            }
            break;

        case Qt::Key_Up:
            if(listview->currentIndex().row() > 0){
                listview->setCurrentIndex(model->index(listview->currentIndex().row() - 1, FSUB_SUBNAME));
            }
            break;

        case Qt::Key_Down:
            if(listview->currentIndex().row() < model->rowCount() - 1){
                listview->setCurrentIndex(model->index(listview->currentIndex().row() + 1, FSUB_SUBNAME));
            }
            break;

        case Qt::Key_Return:  //回车键
        case Qt::Key_Enter:
            index = listview->currentIndex().row();
            subId = model->data(model->index(index, 0)).toInt();
            setCurrentIndex(findData(subId));
            listview->hide();
            keys->clear();
            emit dataEditCompleted(1, true);
            break;
        }
    }
    //增加这个检测是为了能够以一贯的方式（即用回车键完成输入焦点的移动）移动焦点到下一个表格项
    else if((keyCode == Qt::Key_Enter) || (keyCode == Qt::Key_Return))
        emit dataEditCompleted(1, true);
    else
        QComboBox::keyPressEvent(e);
}

//在点击智能提示框某个条目时（其实在你点击listview时，
//代理已经失去了焦点，它的生命也将终止，因此不能接收到此信号，这样的话，
//必须将listview从代理中脱离出来，即不作为代理类的成员。
//或者在创建listview时，使代理类作为它的父类，但这样必须解决listview的显示
//和定位问题，因为默认，它被限制在单元格的区域内）
//void FstSubComboBox::completeText(const QModelIndex &index)
//{
//    int idx = listview->currentIndex().row();
//    int subId = model->data(model->index(idx, FSTSUB_ID)).toInt();
//    setCurrentIndex(findData(subId));
//    listview->hide();
//    keys->clear();
//    emit dataEditCompleted(1, true);
//}


//////////////////////////SndSubComboBox//////////////////////////
SndSubComboBox2::SndSubComboBox2(int pid, SubjectManager *smg, QWidget *parent) :
    QComboBox(parent),pid(pid),smg(smg)
{
    db = smg->getAccount()->getDbUtil()->getDb();
    fsub = NULL;
    if(pid != 0){
        fsub = smg->getFstSubject(pid);
        foreach(SecondSubject* ssub, smg->getFstSubject(pid)->getChildSubs())
            addItem(ssub->getName(),ssub->getId());
    }
//显然，只有设置了一级科目，才能显示可用的二级科目
//    else{
//        QHashIterator<int,QString> it(allSndSubs);
//        while(it.hasNext()){
//            it.next();
//            addItem(it.value(), it.key());
//        }
//    }

    setEditable(true);  //使其可以输入新的二级科目名
    //BusiUtil::getAllSndSubNameList(snames);
    //lineEdit()->setCompleter(new QCompleter(snames, this));
    model = new QSqlQueryModel;
    //这里是否要过滤禁用的二级科目
    QString s = QString("select %1.id,%1.%2,%1.%3,%1.%4,%1.%5,%6.%7 "
                        "from %1 join %6 on %1.%3 = %6.id order by %1.%4")
            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_code)
            .arg(fld_ssub_weight).arg(tbl_nameItem).arg(fld_ni_name);
    model->setQuery(s,db);
    keys = new QString;
    listview = new QListView(parent);

    //应该使用取自SecSubjects表的model
    smodel = new QSqlTableModel(parent,db);
    smodel->setTable(tbl_nameItem);
    smodel->setSort(NI_REMCODE, Qt::AscendingOrder);
    listview->setModel(smodel);
    listview->setModelColumn(NI_NAME);
    smodel->select();

    //因为QSqlTableModel不会一次提取所有的记录，因此，必须用如下方法
    //获取真实的行数，但基于性能的原因，不建议使用如下的方法，而直接用一个
    //QSqlQuery来提取实际的行数
    //while (smodel->canFetchMore())
    //{
    //    smodel->fetchMore();
    //}
    //rows = smodel->rowCount();

//    QSqlQuery q(db);
//    bool r = q.exec(QString("select count() from %1").arg(tbl_nameItem));
//    r = q.first();
//    rows = q.value(0).toInt();

    rows = smg->getAllNI().count();

    listview->setFixedHeight(150); //最好是设置一个与父窗口的高度合适的尺寸
    //QPoint globalP = mapToGlobal(parent->pos());
    //listview->move(parent->x(),parent->y() + this->height());

}

SndSubComboBox2::~SndSubComboBox2()
{
    delete listview;
    delete keys;
}

//设置编辑器激活时所处的行列号
void SndSubComboBox2::setRowColNum(int row, int col)
{
    this->row = row;
    this->col = col;
}

//void SndSubComboBox::focusOutEvent(QFocusEvent* e)
//{
//    listview->hide();
//    QComboBox::focusOutEvent(e);
//}

void SndSubComboBox2::keyPressEvent(QKeyEvent* e)
{
    static int i = 0;
    static bool isDigit = true;  //true：输入的是科目的数字代码，false：科目的助记符    
    int nid;   //名称条目id
    int index;    
    int idx,c;

    if((e->modifiers() != Qt::NoModifier) &&
       (e->modifiers() != Qt::KeypadModifier)){
        e->ignore();
        QComboBox::keyPressEvent(e);
        return;
    }

    int keyCode = e->key();

    //如果是字母键，则输入的是科目助记符，则按助记符快速定位
    //字母键只有在输入法未打开的情况下才会接收到
    if(((keyCode >= Qt::Key_A) && (keyCode <= Qt::Key_Z))){
        keys->append(keyCode);
        if(keys->size() == 1){ //接收到第一个字符，需要重新按科目助记符排序，并装载到列表框
            isDigit = false;
            smodel->select();
            while (smodel->canFetchMore())
            {
                smodel->fetchMore();
            }
            i = 0;
            listview->setMinimumWidth(width());
            listview->setMaximumWidth(width());
            QPoint p(0, height());
            int x = mapToParent(p).x();
            int y = mapToParent(p).y() + 1;
            listview->move(x, y);
            listview->show();
        }
        //定位到最匹配的条目
        QString remCode = smodel->data(smodel->index(i, NI_REMCODE)).toString();
        while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
            i++;
            remCode = smodel->data(smodel->index(i, NI_REMCODE)).toString();
        }
        if(i < rows)
            listview->setCurrentIndex(smodel->index(i, NI_NAME));

        //e->accept();

    }
    //如果是数字键则键入的是科目代码，则按科目代码快速定位
    else if((keyCode >= Qt::Key_0) && (keyCode <= Qt::Key_9)){
//        keys->append(keyCode);
//        isDigit = true;
//        if(keys->size() == 1)
//            i = 0;
//        showPopup();
//        //定位到最匹配的科目
//        QString subCode = model->data(model->index(i, 3)).toString();
//        int rows = model->rowCount();
//        while((keys->compare(subCode) > 0) && (i < rows)){
//            i++;
//            subCode = model->data(model->index(i, 3)).toString();
//        }
//        if(i < rows)
//            setCurrentIndex(i);
//        e->accept();

    }
    //如果是其他编辑键
    else if(listview->isVisible() || (keys->size() > 0)){
        int sid; //子目id值（SndSubjects表id列）
        switch(keyCode){
        case Qt::Key_Backspace:  //退格键，则维护键盘字符缓存，并进行重新定位
            keys->chop(1);
            if(keys->size() == 0)
                listview->hide();
            if((keys->size() == 0) && isDigit)
                hidePopup();
            else{ //用遗留的字符根据是数字还是字母再重新进行依据科目代码或助记符进行定位
                if(isDigit){
                    i = 0;
                    QString subCode = model->data(model->index(i, 3)).toString();
                    int rows = model->rowCount();
                    while((keys->compare(subCode) > 0) && (i < rows)){
                        i++;
                        subCode = model->data(model->index(i, 3)).toString();
                    }
                    if(i < rows)
                        listview->setCurrentIndex(model->index(i, 3));
                }
                else{
                    i = 0;
                    QString remCode = smodel->data(smodel->index(i, NI_REMCODE)).toString();
                    //int rows = smodel->rowCount();
                    while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
                        i++;
                        remCode = smodel->data(smodel->index(i, NI_REMCODE)).toString();
                    }
                    if(i < rows)
                        listview->setCurrentIndex(smodel->index(i, NI_NAME));
                }
            }
            break;

        case Qt::Key_Up:
            if(listview->currentIndex().row() > 0){
                listview->setCurrentIndex(smodel->index(listview->currentIndex().row() - 1, NI_NAME));
            }
            break;

        case Qt::Key_Down:
            idx = listview->currentIndex().row();
            //c = smodel->rowCount();
            if( idx < rows - 1){
                listview->setCurrentIndex(smodel->index(idx + 1, NI_NAME));
            }
            break;

        case Qt::Key_Return:  //回车键
        case Qt::Key_Enter:
            index = listview->currentIndex().row();
            nid = smodel->data(smodel->index(index, 0)).toInt();
            ni = smg->getNameItem(nid);
            //如果在当前一级科目下没有使用当前选择的名称条目的二级科目，则触发信号用选择的名称创建二级科目
            //if(!BusiUtil::getFstToSnd(pid, nid, id)){
            if(!fsub->containChildSub(ni)){
                listview->hide();
                keys->clear();
                emit dataEditCompleted(2,true);
                emit newMappingItem(pid, nid, row, col);
            }
            else{
                ssub = fsub->getChildSub(ni);
                listview->hide();
                keys->clear();
                //bool dis = false;
                //BusiUtil::isSndSubDisabled(sid,dis);
                if(!ssub->isEnabled())
                    emit sndSubDisabled(sid);
                else{
                    setCurrentIndex(findData(sid));
                    emit dataEditCompleted(2,true);
                    emit editNextItem(row,col);
                }
            }
            break;
        }
        //e->accept();
    }
    //在智能提示框没有出现时输入的文本（中文文本），需要在SecSubjects表中进行查找，
    //如果不能找到一个匹配的，这说明是一个新的二级科目
    else if((keyCode == Qt::Key_Return) || (keyCode == Qt::Key_Enter)){
            QString name = currentText();
            name.simplified();
            if(name != ""){
                int sid;
                //如果名称条目是新的，则要在当前一级科目下创建采用该名称条目的二级科目
                //if(!findSubName(name,sid)){
                ni = SubjectManager::getNameItem(name);
                if(!ni){
                    emit dataEditCompleted(2,true);
                    emit newSndSubject(pid, name, row, col);
                }
                else{ //如果名称条目已存在，则检查是否在当前一级科目下有采用该名称的二级科目
                    //int id = 0;
                    //BusiUtil::getFstToSnd(pid,sid,id);
                    //if(id == 0){ //如果没有则采用该名称条目创建新的二级科目
                    ssub = fsub->getChildSub(ni);
                    if(!ssub){
                        emit dataEditCompleted(2,true);
                        emit newMappingItem(pid,ni->getId(),row,col);
                    }
                    else{
                        //bool dis = false;
                        //BusiUtil::isSndSubDisabled(id,dis);
                        //if(!dis)
                        if(!ssub->isEnabled())
                            emit sndSubDisabled(ssub->getId());
                        else{
                            setCurrentIndex(findData(ssub->getId()));
                            emit dataEditCompleted(2,true);
                        }

                    }
                }                
            }
            else     //明细科目不能为空（但这样处置，却不能阻止关闭编辑器）
                return;

            //e->accept();
    }
    else
        //e->ignore();
        QComboBox::keyPressEvent(e);
}

//在this.model中查找是否有此一级和二级科目的对应条目
//bool SndSubComboBox::findSubMapper(int fid, int sid, int& id)
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

//在二级科目表SecSubjects中查找是否存在名称为name的二级科目
//bool SndSubComboBox::findSubName(QString name, int& sid)
//{
//    QSqlQuery q;
//    QString s = QString("select id from %1 where %2 = '%3'")
//            .arg(tbl_nameItem).arg(fld_ni_name).arg(name);
//    if(q.exec(s) && q.first()){
//        sid = q.value(0).toInt();
//        return true;
//    }
//    else
//        return false;
//}


/////////////////////////////MoneyTypeComboBox///////////////////////
MoneyTypeComboBox2::MoneyTypeComboBox2(QHash<int,QString>* mts,
                                     QWidget* parent):QComboBox(parent)
{
    this->mts = mts;
    QHashIterator<int,QString>* it = new QHashIterator<int,QString>(*mts);
    while(it->hasNext()){
        it->next();
        addItem(it->value(), it->key());
    }
}

void MoneyTypeComboBox2::setCell(int row,int col)
{
    this->row = row;
    this->col = col;
}

void MoneyTypeComboBox2::focusOutEvent(QFocusEvent* e)
{
    QComboBox::focusOutEvent(e);
}

//实现输入货币代码即定位到正确的货币索引
void MoneyTypeComboBox2::keyPressEvent(QKeyEvent* e )
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
    else if((key == Qt::Key_Return) || (key == Qt::Key_Enter)){
        emit dataEditCompleted(3,true);
        emit editNextItem(row,col);
        e->accept();
    }
    else
        e->ignore();
    QComboBox::keyPressEvent(e);
}


////////////////////////////MoneyValueEdit////////////////////////
MoneyValueEdit2::MoneyValueEdit2(int row, int witch, double v, QWidget* parent)
    : QLineEdit(parent)
{
    this->row = row;
    this->witch = witch;
    setValue(v);

    QDoubleValidator* validator  = new QDoubleValidator(this);
    validator->setDecimals(2);
    setValidator(validator);    
}

//设置金额值
void MoneyValueEdit2::setValue(double v)
{
    //QString s = removeRightZero(QString::number(v,'f',2));
    this->v = v;
    if(v != 0){
        QString s = QString::number(v,'f',2);
        if(s.right(3) == ".00")
            s.chop(3);
        if((s.right(1) == "0") && (s.indexOf(".") != -1))
            s.chop(1);
        setText(s);
    }

}

//获取金额值
double MoneyValueEdit2::getValue()
{
    v = text().toDouble();
    return v;
}

void MoneyValueEdit2::setCell(int row,int col)
{
    this->row = row;
    this->col = col;
}

void MoneyValueEdit2::focusOutEvent(QFocusEvent* e)
{
    QLineEdit::focusOutEvent(e);
}

void MoneyValueEdit2::keyPressEvent(QKeyEvent* e )
{
    int key = e->key();
    if((key == Qt::Key_Return) || (key == Qt::Key_Enter)){
        v = text().toDouble();
        if(witch == 1)  //借方金额栏
            emit dataEditCompleted(4, true);
        else
            emit dataEditCompleted(5, true);
        emit editNextItem(row,col);
        if(witch == 0){
            emit nextRow(row+1);  //传播给代理，代理再传播给凭证编辑窗
            //CustomKeyEvent ke(e);
            //ke.setTag(true);
            //ke.accept();
            //QLineEdit::keyPressEvent(&ke);//事件不能向父对象传播
        }
        //QLineEdit::keyPressEvent(e);
    }
    else
        QLineEdit::keyPressEvent(e);
}

//////////////////////////////DirEdit///////////////////////////////////////
DirEdit::DirEdit(int dir, QWidget* parent) : QComboBox(parent)
{
    this->dir = dir;
    addItem(tr("借"), DIR_J);
    addItem(tr("贷"), DIR_D);
    addItem(tr("平"), DIR_D);
}

void DirEdit::setDir(int dir)
{
    this->dir = dir;
    int idx = findData(dir);
    setCurrentIndex(idx);
}

int DirEdit::getDir()
{
   dir = itemData(currentIndex(), Qt::UserRole).toInt();
   return dir;
}

void DirEdit::setCell(int row,int col)
{
    this->row = row;
    this->col = col;
}

////////////////////////TagEdit//////////////////////////////////////
TagEdit::TagEdit(bool tag, QString desc, QWidget* parent)
    :QWidget(parent),tag(tag),desc(desc)
{
    chkBox = new QCheckBox(this);
    chkBox->setChecked(tag);
    edtBox = new QLineEdit(desc);
    QHBoxLayout* l = new QHBoxLayout;
    l->addWidget(chkBox,0);
    l->addWidget(edtBox,1);
    setLayout(l);
    connect(chkBox,SIGNAL(toggled(bool)),this,SLOT(tagToggled(bool)));
    connect(edtBox,SIGNAL(textEdited(QString)),this,SLOT(DescEdited(QString)));
}

void TagEdit::tagToggled(bool checked)
{
    tag = checked;
}

void TagEdit::DescEdited(const QString &text)
{
    desc = text;
}


////////////////////////////////ActionEditItemDelegate/////////////////
ActionEditItemDelegate2::ActionEditItemDelegate2(SubjectManager *smg, QObject *parent):
    QItemDelegate(parent),smg(smg)
{
    isReadOnly = false;
}

//设置其代理的表格项的只读模式（这个函数用于支持表格的只读模式）
void ActionEditItemDelegate2::setReadOnly(bool readOnly)
{
    isReadOnly = readOnly;
}

//
void ActionEditItemDelegate2::setVolidRows(int rows)
{
    this->rows = rows;
}

QWidget* ActionEditItemDelegate2::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    if(isReadOnly)
        return 0;
    int col = index.column();
    int row = index.row();
    if(row < rows){
        if(col == SUMMARY){ //摘要列
            //QLineEdit* editor = new QLineEdit(parent);
            SummaryEdit2 *editor = new SummaryEdit2(row,col,parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            connect(editor, SIGNAL(copyPrevShortcutPressed(int,int)),
                    this, SLOT(catchCopyPrevShortcut(int,int)));
            //connect(editor, SIGNAL(editingFinished()),
            //        this, SLOT(commitAndCloseEditor(ColumnIndex)));
            return editor;
        }
        else if(col == FSTSUB){ //总账科目列
            FstSubComboBox2* editor = new FstSubComboBox2(parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            return editor;
        }
        else if(col == SNDSUB){ //明细科目列
            //首先要获取总账科目id
            int pid = index.model()->data(index.model()->index(index.row(),
                           col - 1), Qt::EditRole).toInt();
            if(pid != 0){
                SndSubComboBox2* editor = new SndSubComboBox2(pid,smg,parent);
                //editor->setCompleter(new QCompleter(names, editor));
                editor->setRowColNum(index.row(), col);
                connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                        this, SLOT(commitAndCloseEditor(int,bool)));
                connect(editor, SIGNAL(newMappingItem(int,int,int,int)),
                        this, SLOT(newMappingItem(int,int,int,int)));
                connect(editor,SIGNAL(newSndSubject(int,QString,int,int)),
                        this, SLOT(newSndSubject(int,QString,int,int)));
                connect(editor,SIGNAL(sndSubDisabled(int)),
                        this,SLOT(sndSubDisabled(int)));
                return editor;
            }
            else
                return new QComboBox(parent);
        }
        else if(col == MTYPE){ //币种列
            MoneyTypeComboBox2* editor = new MoneyTypeComboBox2(&allMts, parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            return editor;
        }
        else if(col == JV){ //借方金额列
            MoneyValueEdit2 *editor = new MoneyValueEdit2(row,1,0,parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            return editor;
        }
        else{               //贷方金额列
            MoneyValueEdit2 *editor = new MoneyValueEdit2(row,0,0,parent);
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

void ActionEditItemDelegate2::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    if(col == SUMMARY){
       SummaryEdit2* edit = qobject_cast<SummaryEdit2*>(editor);
       if (edit) {
           edit->setContent(index.model()->data(index, Qt::EditRole).toString());
       }
    }
    else if(col == FSTSUB){
        FstSubComboBox2* cmb = qobject_cast<FstSubComboBox2*>(editor);
        if(cmb){
            int fid = index.model()->data(index, Qt::EditRole).toInt();
            int idx = cmb->findData(fid, Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if(col == SNDSUB){
        SndSubComboBox2* cmb = qobject_cast<SndSubComboBox2*>(editor);
        if(cmb){
            int sid = index.model()->data(index, Qt::EditRole).toInt();
            int idx = cmb->findData(sid, Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if(col == MTYPE){
        MoneyTypeComboBox2* cmb = qobject_cast<MoneyTypeComboBox2*>(editor);
        int mt = index.model()->data(index, Qt::EditRole).toInt();
        int idx = cmb->findData(mt, Qt::UserRole);
        cmb->setCurrentIndex(idx);
    }
    else if((col == JV) || (col == DV)){
        MoneyValueEdit2 *edit = qobject_cast<MoneyValueEdit2*>(editor);
        if (edit) {
            double v = index.model()->data(index, Qt::EditRole).toDouble();
            edit->setValue(v);
        }
    }    
}

void ActionEditItemDelegate2::setModelData(QWidget *editor, QAbstractItemModel *model,
                  const QModelIndex &index) const
{
    int col = index.column();
    if(col == SUMMARY){
        SummaryEdit2* edit = qobject_cast<SummaryEdit2*>(editor);
        if (edit) {
            model->setData(index, edit->getContent());
        }
    }
    else if(col == FSTSUB){
        FstSubComboBox2* cmb = qobject_cast<FstSubComboBox2*>(editor);
        if(cmb){
            int fid = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(fid != 0)
                model->setData(index, fid);
        }
    }
    else if(col == SNDSUB){
        SndSubComboBox2* cmb = qobject_cast<SndSubComboBox2*>(editor);
        if(cmb){
            int sid = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();

            if(sid != 0)
                model->setData(index, sid);
        }
    }
    else if(col == MTYPE){
        MoneyTypeComboBox2* cmb = qobject_cast<MoneyTypeComboBox2*>(editor);
        if(cmb){
            int mt = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(mt != 0)
                model->setData(index, mt);
        }
    }
    else if((col == JV) || (col == DV)){
        MoneyValueEdit2* edit = qobject_cast<MoneyValueEdit2*>(editor);
        if(edit){
            double v = edit->getValue();
            model->setData(index, v);
        }
    }
}

void ActionEditItemDelegate2::commitAndCloseEditor(int colIndex, bool isMove)
{
    QWidget* editor;
    if(colIndex == SUMMARY)
        editor = qobject_cast<BASummaryForm*>(sender());
    else if(colIndex == FSTSUB)
        editor = qobject_cast<FstSubComboBox2*>(sender());
    else if(colIndex == SNDSUB){
        editor = qobject_cast<SndSubComboBox2*>(sender());
        //SndSubComboBox* combo = qobject_cast<SndSubComboBox*>(sender());
        //int idx = combo->currentIndex();
        //if(idx == -1)
        //    return;
    }
    else if(colIndex == MTYPE)
        editor = qobject_cast<MoneyTypeComboBox2*>(sender());
    else if(colIndex == JV)
        editor = qobject_cast<MoneyValueEdit2*>(sender());
    else if(colIndex == DV)
        editor = qobject_cast<MoneyValueEdit2*>(sender());

    //emit commitData(editor);
    if(isMove){
        emit commitData(editor);
        emit closeEditor(editor, QAbstractItemDelegate::EditNextItem);        
    }
    else{
        emit commitData(editor);
        emit closeEditor(editor);
    }
}

//创建新的一二级科目的映射关系
void ActionEditItemDelegate2::newMappingItem(int pid, int sid, int row, int col)
{    
    emit newSndSubMapping(pid,sid,row,col);
}

//创建新的二级科目，并建立与指定一级科目的映射关系
void ActionEditItemDelegate2::newSndSubject(int fid, QString name, int row, int col)
{
    emit newSndSubAndMapping(fid,name,row,col);
}

//二级科目已被禁用
void ActionEditItemDelegate2::sndSubDisabled(int id)
{
    emit sndSubjectDisabled(id);
}

//信号传播中介，在编辑器打开的情况下，当用户在贷方列按回车键时，会接收到此信号，并将此信号进一步传播给凭证编辑窗口
void ActionEditItemDelegate2::nextRow(int row)
{
    emit moveNextRow(row);
}

//捕获编辑器触发的要求自动快捷方式
void ActionEditItemDelegate2::catchCopyPrevShortcut(int row, int col)
{
    emit reqCopyPrevAction(row,col);
}

void ActionEditItemDelegate2::updateEditorGeometry(QWidget* editor,
         const QStyleOptionViewItem &option, const QModelIndex& index) const
{
//    if(index.column() == SUMMARY){
//        QRect rect = option.rect;
//        rect.setHeight(100);
//        editor->setGeometry(rect);
//    }
//    else
        editor->setGeometry(option.rect);

}


//////////////////////////////DetExtItemDelegate//////////////////////////
DetExtItemDelegate::DetExtItemDelegate(SubjectManager *smg, QObject *parent):
    QItemDelegate(parent),smg(smg)
{

}

QWidget* DetExtItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    int col = index.column();
    int row = index.row();
    if(col == SUB){
        SndSubComboBox2* editor = new SndSubComboBox2(fid,smg,parent);
        editor->setRowColNum(row,col);
        //connect(editor, SIGNAL(editNextItem(int,int)),
        //        this,SLOT(editNextItem(int,int)));
        connect(editor, SIGNAL(newMappingItem(int,int,int,int)),
                this,SLOT(newMappingItem(int,int,int,int)));
        connect(editor, SIGNAL(newSndSubject(int,QString,int,int)),
                this, SLOT(newSndSubject(int,QString,int,int)));
        return editor;
    }
    else if(col == MT){
        MoneyTypeComboBox2* editor = new MoneyTypeComboBox2(&allMts, parent);
        editor->setCell(row,col);
        connect(editor, SIGNAL(editNextItem(int,int)),this, SLOT(editNextItem(int,int)));
        return editor;
    }
    else if(col == MV){
        MoneyValueEdit2* editor;
        int dir = index.model()->data(index.model()->index(row,DIR),Qt::EditRole).toInt();
        if(dir == DIR_J)
            editor = new MoneyValueEdit2(row,1,0,parent);
        else
            editor = new MoneyValueEdit2(row,0,0,parent);
        connect(editor, SIGNAL(editNextItem(int,int)),this,SLOT(editNextItem(int,int)));
        editor->setCell(row,col);
        return editor;
    }
    else if(col == RV){ //只有在外币余额行，才提供编辑器
        int mt = index.model()->data(index.model()->index(row,MT),Qt::EditRole).toInt();
        if(mt == RMB)
            return NULL;
        MoneyValueEdit2* editor;
        int dir = index.model()->data(index.model()->index(row,DIR),Qt::EditRole).toInt();
        if(dir == DIR_J)
            editor = new MoneyValueEdit2(row,1,0,parent);
        else
            editor = new MoneyValueEdit2(row,0,0,parent);
        connect(editor, SIGNAL(editNextItem(int,int)),this,SLOT(editNextItem(int,int)));
        editor->setCell(row,col);
        return editor;
    }
    else if(col == DIR){
        DirEdit* editor = new DirEdit(DIR_J,parent);
        editor->setCell(row,col);
        connect(editor,SIGNAL(editNextItem(int,int)),this,SLOT(editNextItem(int,int)));
        return editor;
    }
    else if(col == TAG){
        bool tag = index.model()->data(index.model()->index(row,TAG),Qt::EditRole).toBool();
        QString desc = index.model()->data(index.model()->index(row,TAG)).toString();
        TagEdit* editor = new TagEdit(tag,desc,parent);
        //editor->resize(parent->size());
        return editor;
    }
}

void DetExtItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    int row = index.row();
    if(col == SUB){
        SndSubComboBox2* cmb = qobject_cast<SndSubComboBox2*>(editor);
        if(cmb){
            int sid = index.model()->data(index, Qt::EditRole).toInt();
            int idx = cmb->findData(sid, Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if(col == MT){
        MoneyTypeComboBox2* cmb = qobject_cast<MoneyTypeComboBox2*>(editor);
        int mt = index.model()->data(index, Qt::EditRole).toInt();
        int idx = cmb->findData(mt, Qt::UserRole);
        cmb->setCurrentIndex(idx);
    }
    else if(col == MV){
        MoneyValueEdit2 *edit = qobject_cast<MoneyValueEdit2*>(editor);
        if (edit) {
            double v = index.model()->data(index, Qt::EditRole).toDouble();
            edit->setValue(v);
        }
    }
    else if(col == RV){
        MoneyValueEdit2 *edit = qobject_cast<MoneyValueEdit2*>(editor);
        if (edit) {
            double v = index.model()->data(index, Qt::EditRole).toDouble();
            edit->setValue(v);
        }
    }
    else if(col == DIR){
        DirEdit* cmb = qobject_cast<DirEdit*>(editor);
        int dir = index.model()->data(index, Qt::EditRole).toInt();
        cmb->setDir(dir);
        //int idx = cmb->findData(dir, Qt::UserRole);
        //cmb->setCurrentIndex(idx);
    }
    else if(col == TAG){
        TagEdit* cmb = qobject_cast<TagEdit*>(editor);
        if(cmb){
            bool tag = index.model()->data(index.model()->index(row,TAG),Qt::EditRole).toBool();
            QString desc = index.model()->data(index.model()->index(row,TAG)).toString();
            cmb->setTag(tag);
            cmb->setDescription(desc);
        }
    }
}

void DetExtItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                  const QModelIndex &index) const
{
    int col = index.column();
    int row = index.row();
    if(col == SUB){
        SndSubComboBox2* cmb = qobject_cast<SndSubComboBox2*>(editor);
        if(cmb){
            int sid = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(sid != 0)
                model->setData(index, sid);
        }
    }
    else if(col == MT){
        MoneyTypeComboBox2* cmb = qobject_cast<MoneyTypeComboBox2*>(editor);
        if(cmb){
            int mt = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(mt != 0)
                model->setData(index, mt);
        }
    }
    else if(col == MV){
        MoneyValueEdit2* edit = qobject_cast<MoneyValueEdit2*>(editor);
        if(edit){
            double v = edit->getValue();
            model->setData(index, v);
        }
    }
    else if(col == RV){
        MoneyValueEdit2* edit = qobject_cast<MoneyValueEdit2*>(editor);
        if(edit){
            double v = edit->getValue();
            model->setData(index, v);
        }
    }
    else if(col == DIR){
        DirEdit* cmb = qobject_cast<DirEdit*>(editor);
        if(cmb){
            int dir = cmb->getDir();
            //if(dir != 0)
            model->setData(index, dir);
        }
    }
    else if(col == TAG){
        TagEdit* cmb = qobject_cast<TagEdit*>(editor);
        bool tag = cmb->getTag();
        QString desc = cmb->getDescription();
        model->setData(index,tag,Qt::EditRole);
        model->setData(index,desc,Qt::DisplayRole);
    }
}

void DetExtItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                          const QModelIndex& index) const
{
    editor->setGeometry(option.rect);
}

void DetExtItemDelegate::setFid(int fid)
{
    this->fid = fid;
}

void DetExtItemDelegate::commitAndCloseEditor(int colIndex, bool isMove)
{
//    QWidget* editor;
//    if(colIndex == ActionEditItemDelegate::SNDSUB)
//        emit editNext();
//    else if(colIndex == ActionEditItemDelegate::MTYPE)
//        editor = qobject_cast<MoneyTypeComboBox*>(sender());
//    else if((colIndex == ActionEditItemDelegate::JV) ||
//            (colIndex == ActionEditItemDelegate::DV))
//        editor = qobject_cast<MoneyValueEdit*>(sender());
//    else if(colIndex == DIR)
//        editor = qobject_cast<DirEdit*>(sender());

//    //emit commitData(editor);
//    if(isMove){
//        emit commitData(editor);
//        emit closeEditor(editor, QAbstractItemDelegate::EditNextItem);
//    }
//    else{
//        emit commitData(editor);
//        emit closeEditor(editor);
//    }
}

void DetExtItemDelegate::newMappingItem(int fid, int sid, int row, int col)
{
    emit newMapping(fid,sid,row,col);
}

void DetExtItemDelegate::newSndSubject(int fid, QString name, int row, int col)
{
    emit newSndSub(fid,name,row,col);
}

void DetExtItemDelegate::editNextItem(int row, int col)
{
    emit editNext(row,col);
}
