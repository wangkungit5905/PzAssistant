#include <QLineEdit>
#include <QDomDocument>
#include <QInputDialog>
#include <QCompleter>

#include "tables.h"
#include "delegates2.h"
#include "utils.h"
#include "dialog2.h"
#include "global.h"


////////////////////////////SummaryEdit/////////////////////////
SummaryEdit::SummaryEdit(int row,int col,QWidget* parent) : QLineEdit(parent)
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

//捕获自动复制前一条会计分录的快捷方式
void SummaryEdit::shortCutActivated()
{
    emit copyPrevShortcutPressed(row,col);
}

//重载此函数的目的是创建一种输入摘要引用体内容的方法
void SummaryEdit::keyPressEvent(QKeyEvent *event)
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

void SummaryEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
    int i = 0;
    e->ignore();
}

void SummaryEdit::focusOutEvent(QFocusEvent* e)
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

FstSubComboBox::FstSubComboBox(/*QHash<int,QString>* subNames,*/ QWidget *parent) : QComboBox(parent)
{
    QList<int>ids;        //总账科目id（它是以科目代码顺序出现的）
    QList<QString> names; //总账科目名
    BusiUtil::getAllFstSub(ids,names);
    for(int i = 0; i < ids.count(); ++i)
        addItem(names[i], ids[i]);

    model = new QSqlQueryModel;
    model->setQuery("select * from FirSubjects where isView = 1");
    keys = new QString;
    //listview = new QListView(this);//这样作将把listview限制在单元格内
    listview = new QListView(parent);
    listview->setModel(model);
    listview->setModelColumn(FSTSUB_SUBNAME);
    //connect(listview, SIGNAL(clicked(QModelIndex)),this,SLOT(completeText(QModelIndex)));


}

FstSubComboBox::~FstSubComboBox()
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

void FstSubComboBox::keyPressEvent(QKeyEvent* e)
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
            model->setQuery("select * from FirSubjects where isView = 1 order by remCode");
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
        QString remCode = model->data(model->index(i, FSTSUB_REMCODE)).toString();
        int rows = model->rowCount();
        while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
            i++;
            remCode = model->data(model->index(i, FSTSUB_REMCODE)).toString();
        }
        if(i < rows)
            listview->setCurrentIndex(model->index(i, FSTSUB_SUBNAME));
    }
    //如果是数字键则键入的是科目代码，则按科目代码快速定位
    else if((keyCode >= Qt::Key_0) && (keyCode <= Qt::Key_9)){
        keys->append(keyCode);
        isDigit = true;
        if(keys->size() == 1){
            model->setQuery("select * from FirSubjects where isView = 1 order by subCode");
            i = 0;
            listview->setMinimumWidth(width());
            listview->setMaximumWidth(width());
            QPoint p(0, height());
            int x = mapToParent(p).x();
            int y = mapToParent(p).y() + 1;
            listview->move(x, y);
            listview->show();
        }
        QString subCode = model->data(model->index(i, FSTSUB_SUBCODE)).toString();
        int rows = model->rowCount();
        while((keys->compare(subCode) > 0) && (i < rows)){
            i++;
            subCode = model->data(model->index(i, FSTSUB_SUBCODE)).toString();
        }
        if(i < rows)
            listview->setCurrentIndex(model->index(i, FSTSUB_SUBNAME));
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
                    QString subCode = model->data(model->index(i, FSTSUB_SUBCODE)).toString();
                    int rows = model->rowCount();
                    while((keys->compare(subCode) > 0) && (i < rows)){
                        i++;
                        subCode = model->data(model->index(i, FSTSUB_SUBCODE)).toString();
                    }
                    if(i < rows)
                        listview->setCurrentIndex(model->index(i, FSTSUB_SUBNAME));
                }
                else{
                    i = 0;
                    QString remCode = model->data(model->index(i, FSTSUB_REMCODE)).toString();
                    int rows = model->rowCount();
                    while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
                        i++;
                        remCode = model->data(model->index(i, FSTSUB_REMCODE)).toString();
                    }
                    if(i < rows)
                        listview->setCurrentIndex(model->index(i, FSTSUB_SUBNAME));
                }
            }
            break;

        case Qt::Key_Up:
            if(listview->currentIndex().row() > 0){
                listview->setCurrentIndex(model->index(listview->currentIndex().row() - 1, FSTSUB_SUBNAME));
            }
            break;

        case Qt::Key_Down:
            if(listview->currentIndex().row() < model->rowCount() - 1){
                listview->setCurrentIndex(model->index(listview->currentIndex().row() + 1, FSTSUB_SUBNAME));
            }
            break;

        case Qt::Key_Return:  //回车键
        case Qt::Key_Enter:
            index = listview->currentIndex().row();
            subId = model->data(model->index(index, FSTSUB_ID)).toInt();
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
SndSubComboBox::SndSubComboBox(int pid, QWidget *parent) : QComboBox(parent)
{
    this->pid = pid;
    QList<int> ids;
    QList<QString> names;
    if(pid != 0){
        BusiUtil::getSndSubInSpecFst(pid, ids, names);
        for(int i = 0; i < ids.count(); ++i)
            addItem(names[i], ids[i]);
    }
    else{
        QHashIterator<int,QString> it(allSndSubs);
        while(it.hasNext()){
            it.next();
            addItem(it.value(), it.key());
        }
    }

    setEditable(true);  //使其可以输入新的二级科目名
    //BusiUtil::getAllSndSubNameList(snames);
    //lineEdit()->setCompleter(new QCompleter(snames, this));
    model = new QSqlQueryModel;
    model->setQuery("select FSAgent.id,FSAgent.fid,FSAgent.sid,"
                    "FSAgent.subCode,FSAgent.FrequencyStat,SecSubjects.subName "
                    "from FSAgent join SecSubjects on FSAgent.sid = "
                    "SecSubjects.id order by FSAgent.subCode");
    keys = new QString;
    listview = new QListView(parent);

    //应该使用取自SecSubjects表的model
    smodel = new QSqlTableModel;
    smodel->setTable("SecSubjects");
    smodel->setSort(SNDSUB_REMCODE, Qt::AscendingOrder);
    listview->setModel(smodel);
    listview->setModelColumn(SNDSUB_SUBNAME);
    smodel->select();

    //因为QSqlTableModel不会一次提取所有的记录，因此，必须用如下方法
    //获取真实的行数，但基于性能的原因，不建议使用如下的方法，而直接用一个
    //QSqlQuery来提取实际的行数
    //while (smodel->canFetchMore())
    //{
    //    smodel->fetchMore();
    //}
    //rows = smodel->rowCount();

    QSqlQuery q;
    bool r = q.exec("select count() from SecSubjects");
    r = q.first();
    rows = q.value(0).toInt();

    listview->setFixedHeight(150); //最好是设置一个与父窗口的高度合适的尺寸
    //QPoint globalP = mapToGlobal(parent->pos());
    //listview->move(parent->x(),parent->y() + this->height());

}

SndSubComboBox::~SndSubComboBox()
{
    delete listview;
    delete keys;
}

//设置编辑器激活时所处的行列号
void SndSubComboBox::setRowColNum(int row, int col)
{
    this->row = row;
    this->col = col;
}

//void SndSubComboBox::focusOutEvent(QFocusEvent* e)
//{
//    listview->hide();
//    QComboBox::focusOutEvent(e);
//}

void SndSubComboBox::keyPressEvent(QKeyEvent* e)
{
    static int i = 0;
    static bool isDigit = true;  //true：输入的是科目的数字代码，false：科目的助记符    
    int sid;   //在SndSubClass表中的ID
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
        QString remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
        while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
            i++;
            remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
        }
        if(i < rows)
            listview->setCurrentIndex(smodel->index(i, SNDSUB_SUBNAME));

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
        int id; //FSAgent表的id值
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
                    QString remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
                    //int rows = smodel->rowCount();
                    while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
                        i++;
                        remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
                    }
                    if(i < rows)
                        listview->setCurrentIndex(smodel->index(i, SNDSUB_SUBNAME));
                }
            }
            break;

        case Qt::Key_Up:
            if(listview->currentIndex().row() > 0){
                listview->setCurrentIndex(smodel->index(listview->currentIndex().row() - 1, SNDSUB_SUBNAME));
            }
            break;

        case Qt::Key_Down:
            idx = listview->currentIndex().row();
            //c = smodel->rowCount();
            if( idx < rows - 1){
                listview->setCurrentIndex(smodel->index(idx + 1, SNDSUB_SUBNAME));
            }
            break;

        case Qt::Key_Return:  //回车键
        case Qt::Key_Enter:
            index = listview->currentIndex().row();
            sid = smodel->data(smodel->index(index, SNDSUB_ID)).toInt();

            //在当前一级科目下没有任何二级科目的映射，或者如果在当前的smodel中
            //找不到此sid值，说明当前一级科目和选择的二级科目没有对应的映射条目

            if(!BusiUtil::getFstToSnd(pid, sid, id)){
                listview->hide();
                keys->clear();
                emit dataEditCompleted(2,true);
                emit newMappingItem(pid, sid, row, col);                
            }
            else{
                listview->hide();
                keys->clear();
                setCurrentIndex(findData(id));
                emit dataEditCompleted(2,true);
                emit editNextItem(row,col);
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
                if(!findSubName(name,sid)){
                    emit dataEditCompleted(2,true);
                    emit newSndSubject(pid, name, row, col);
                }
                else{ //如果找到，还要检测是否存在一二级科目的对应映射关系
                    int id = 0;
                    BusiUtil::getFstToSnd(pid,sid,id);
                    if(id == 0){ //不存在对应映射关系
                        emit dataEditCompleted(2,true);
                        emit newMappingItem(pid,sid,row,col);
                    }
                    else{
                        setCurrentIndex(findData(id));
                        emit dataEditCompleted(2,true);
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
bool SndSubComboBox::findSubMapper(int fid, int sid, int& id)
{
    int c = model->rowCount();
    bool founded = false;

    if(c > 0){
        int i = 0;
        while((i < c) && !founded){
            int fv = model->data(model->index(i,1)).toInt();
            int sv = model->data(model->index(i, 2)).toInt();
            if((fv == fid) && (sv == sid)){
                founded = true;
                id = model->data(model->index(i,0)).toInt();
            }
            i++;
        }
    }
    return founded;
}

//在二级科目表SecSubjects中查找是否存在名称为name的二级科目
bool SndSubComboBox::findSubName(QString name, int& sid)
{
    QSqlQuery q;
    QString s = QString("select id from SecSubjects "
                        "where subName = '%1'").arg(name);
    if(q.exec(s) && q.first()){
        sid = q.value(0).toInt();
        return true;
    }
    else
        return false;
}


/////////////////////////////MoneyTypeComboBox///////////////////////
MoneyTypeComboBox::MoneyTypeComboBox(QHash<int,QString>* mts,
                                     QWidget* parent):QComboBox(parent)
{
    this->mts = mts;
    QHashIterator<int,QString>* it = new QHashIterator<int,QString>(*mts);
    while(it->hasNext()){
        it->next();
        addItem(it->value(), it->key());
    }
}

void MoneyTypeComboBox::setCell(int row,int col)
{
    this->row = row;
    this->col = col;
}

void MoneyTypeComboBox::focusOutEvent(QFocusEvent* e)
{
    QComboBox::focusOutEvent(e);
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
MoneyValueEdit::MoneyValueEdit(int row, int witch, double v, QWidget* parent)
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
void MoneyValueEdit::setValue(double v)
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
double MoneyValueEdit::getValue()
{
    v = text().toDouble();
    return v;
}

void MoneyValueEdit::setCell(int row,int col)
{
    this->row = row;
    this->col = col;
}

void MoneyValueEdit::focusOutEvent(QFocusEvent* e)
{
    QLineEdit::focusOutEvent(e);
}

void MoneyValueEdit::keyPressEvent(QKeyEvent* e )
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
ActionEditItemDelegate::ActionEditItemDelegate(QObject *parent):
    QItemDelegate(parent)
{
    isReadOnly = false;
}

//设置其代理的表格项的只读模式（这个函数用于支持表格的只读模式）
void ActionEditItemDelegate::setReadOnly(bool readOnly)
{
    isReadOnly = readOnly;
}

//
void ActionEditItemDelegate::setVolidRows(int rows)
{
    this->rows = rows;
}

QWidget* ActionEditItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    if(isReadOnly)
        return 0;
    int col = index.column();
    int row = index.row();
    if(row < rows){
        if(col == SUMMARY){ //摘要列
            //QLineEdit* editor = new QLineEdit(parent);
            SummaryEdit *editor = new SummaryEdit(row,col,parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            connect(editor, SIGNAL(copyPrevShortcutPressed(int,int)),
                    this, SLOT(catchCopyPrevShortcut(int,int)));
            //connect(editor, SIGNAL(editingFinished()),
            //        this, SLOT(commitAndCloseEditor(ColumnIndex)));
            return editor;
        }
        else if(col == FSTSUB){ //总账科目列
            FstSubComboBox* editor = new FstSubComboBox(parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            return editor;
        }
        else if(col == SNDSUB){ //明细科目列
            //首先要获取总账科目id
            int pid = index.model()->data(index.model()->index(index.row(),
                           col - 1), Qt::EditRole).toInt();
            if(pid != 0){
                SndSubComboBox* editor = new SndSubComboBox(pid, parent);
                //editor->setCompleter(new QCompleter(names, editor));
                editor->setRowColNum(index.row(), col);
                connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                        this, SLOT(commitAndCloseEditor(int,bool)));
                connect(editor, SIGNAL(newMappingItem(int,int,int,int)),
                        this, SLOT(newMappingItem(int,int,int,int)));
                connect(editor,SIGNAL(newSndSubject(int,QString,int,int)),
                        this, SLOT(newSndSubject(int,QString,int,int)));
                return editor;
            }
            else
                return new QComboBox(parent);
        }
        else if(col == MTYPE){ //币种列
            MoneyTypeComboBox* editor = new MoneyTypeComboBox(&allMts, parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            return editor;
        }
        else if(col == JV){ //借方金额列
            MoneyValueEdit *editor = new MoneyValueEdit(row,1,0,parent);
            connect(editor, SIGNAL(dataEditCompleted(int,bool)),
                    this, SLOT(commitAndCloseEditor(int,bool)));
            return editor;
        }
        else{               //贷方金额列
            MoneyValueEdit *editor = new MoneyValueEdit(row,0,0,parent);
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
    if(col == SUMMARY){
       SummaryEdit* edit = qobject_cast<SummaryEdit*>(editor);
       if (edit) {
           edit->setContent(index.model()->data(index, Qt::EditRole).toString());
       }
    }
    else if(col == FSTSUB){
        FstSubComboBox* cmb = qobject_cast<FstSubComboBox*>(editor);
        if(cmb){
            int fid = index.model()->data(index, Qt::EditRole).toInt();
            int idx = cmb->findData(fid, Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if(col == SNDSUB){
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            int sid = index.model()->data(index, Qt::EditRole).toInt();
            int idx = cmb->findData(sid, Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if(col == MTYPE){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        int mt = index.model()->data(index, Qt::EditRole).toInt();
        int idx = cmb->findData(mt, Qt::UserRole);
        cmb->setCurrentIndex(idx);
    }
    else if((col == JV) || (col == DV)){
        MoneyValueEdit *edit = qobject_cast<MoneyValueEdit*>(editor);
        if (edit) {
            double v = index.model()->data(index, Qt::EditRole).toDouble();
            edit->setValue(v);
        }
    }    
}

void ActionEditItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                  const QModelIndex &index) const
{
    int col = index.column();
    if(col == SUMMARY){
        SummaryEdit* edit = qobject_cast<SummaryEdit*>(editor);
        if (edit) {
            model->setData(index, edit->getContent());
        }
    }
    else if(col == FSTSUB){
        FstSubComboBox* cmb = qobject_cast<FstSubComboBox*>(editor);
        if(cmb){
            int fid = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(fid != 0)
                model->setData(index, fid);
        }
    }
    else if(col == SNDSUB){
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            int sid = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();

            if(sid != 0)
                model->setData(index, sid);
        }
    }
    else if(col == MTYPE){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        if(cmb){
            int mt = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(mt != 0)
                model->setData(index, mt);
        }
    }
    else if((col == JV) || (col == DV)){
        MoneyValueEdit* edit = qobject_cast<MoneyValueEdit*>(editor);
        if(edit){
            double v = edit->getValue();
            model->setData(index, v);
        }
    }
}

void ActionEditItemDelegate::commitAndCloseEditor(int colIndex, bool isMove)
{
    QWidget* editor;
    if(colIndex == SUMMARY)
        editor = qobject_cast<BASummaryForm*>(sender());
    else if(colIndex == FSTSUB)
        editor = qobject_cast<FstSubComboBox*>(sender());
    else if(colIndex == SNDSUB){
        editor = qobject_cast<SndSubComboBox*>(sender());
        //SndSubComboBox* combo = qobject_cast<SndSubComboBox*>(sender());
        //int idx = combo->currentIndex();
        //if(idx == -1)
        //    return;
    }
    else if(colIndex == MTYPE)
        editor = qobject_cast<MoneyTypeComboBox*>(sender());
    else if(colIndex == JV)
        editor = qobject_cast<MoneyValueEdit*>(sender());
    else if(colIndex == DV)
        editor = qobject_cast<MoneyValueEdit*>(sender());

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
void ActionEditItemDelegate::newMappingItem(int pid, int sid, int row, int col)
{    
    emit newSndSubMapping(pid,sid,row,col);
}

//创建新的二级科目，并建立与指定一级科目的映射关系
void ActionEditItemDelegate::newSndSubject(int fid, QString name, int row, int col)
{
    emit newSndSubAndMapping(fid,name,row,col);
}

//信号传播中介，在编辑器打开的情况下，当用户在贷方列按回车键时，会接收到此信号，并将此信号进一步传播给凭证编辑窗口
void ActionEditItemDelegate::nextRow(int row)
{
    emit moveNextRow(row);
}

//捕获编辑器触发的要求自动快捷方式
void ActionEditItemDelegate::catchCopyPrevShortcut(int row, int col)
{
    emit reqCopyPrevAction(row,col);
}

void ActionEditItemDelegate::updateEditorGeometry(QWidget* editor,
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
DetExtItemDelegate::DetExtItemDelegate(QObject *parent):QItemDelegate(parent)
{

}

QWidget* DetExtItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    int col = index.column();
    int row = index.row();
    if(col == SUB){
        SndSubComboBox* editor = new SndSubComboBox(fid, parent);
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
        MoneyTypeComboBox* editor = new MoneyTypeComboBox(&allMts, parent);
        editor->setCell(row,col);
        connect(editor, SIGNAL(editNextItem(int,int)),this, SLOT(editNextItem(int,int)));
        return editor;
    }
    else if(col == MV){
        MoneyValueEdit* editor;
        int dir = index.model()->data(index.model()->index(row,DIR),Qt::EditRole).toInt();
        if(dir == DIR_J)
            editor = new MoneyValueEdit(row,1,0,parent);
        else
            editor = new MoneyValueEdit(row,0,0,parent);
        connect(editor, SIGNAL(editNextItem(int,int)),this,SLOT(editNextItem(int,int)));
        editor->setCell(row,col);
        return editor;
    }
    else if(col == RV){ //只有在外币余额行，才提供编辑器
        int mt = index.model()->data(index.model()->index(row,MT),Qt::EditRole).toInt();
        if(mt == RMB)
            return NULL;
        MoneyValueEdit* editor;
        int dir = index.model()->data(index.model()->index(row,DIR),Qt::EditRole).toInt();
        if(dir == DIR_J)
            editor = new MoneyValueEdit(row,1,0,parent);
        else
            editor = new MoneyValueEdit(row,0,0,parent);
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
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            int sid = index.model()->data(index, Qt::EditRole).toInt();
            int idx = cmb->findData(sid, Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }
    }
    else if(col == MT){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        int mt = index.model()->data(index, Qt::EditRole).toInt();
        int idx = cmb->findData(mt, Qt::UserRole);
        cmb->setCurrentIndex(idx);
    }
    else if(col == MV){
        MoneyValueEdit *edit = qobject_cast<MoneyValueEdit*>(editor);
        if (edit) {
            double v = index.model()->data(index, Qt::EditRole).toDouble();
            edit->setValue(v);
        }
    }
    else if(col == RV){
        MoneyValueEdit *edit = qobject_cast<MoneyValueEdit*>(editor);
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
        SndSubComboBox* cmb = qobject_cast<SndSubComboBox*>(editor);
        if(cmb){
            int sid = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(sid != 0)
                model->setData(index, sid);
        }
    }
    else if(col == MT){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        if(cmb){
            int mt = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(mt != 0)
                model->setData(index, mt);
        }
    }
    else if(col == MV){
        MoneyValueEdit* edit = qobject_cast<MoneyValueEdit*>(editor);
        if(edit){
            double v = edit->getValue();
            model->setData(index, v);
        }
    }
    else if(col == RV){
        MoneyValueEdit* edit = qobject_cast<MoneyValueEdit*>(editor);
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
