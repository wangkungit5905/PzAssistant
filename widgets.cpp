#include <QKeyEvent>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QPrinter>
#include <QDomDocument>

#include "widgets.h"
#include "utils.h"
#include "global.h"
#include "completsubinfodialog.h"
#include "delegates2.h"
#include "tables.h"
#include "subject.h"
#include "logs/Logger.h"
#include "dbutil.h"


//////////////////////////////////////////////////////////////////////
CustomCheckBox::CustomCheckBox(QWidget* parent) :  QCheckBox(parent){}

CustomCheckBox::CustomCheckBox(const QString &text, QWidget *parent) : QCheckBox(text,parent){}

int CustomCheckBox::getState()
{
    if(isChecked())
        return 1;
    else
        return 0;
}



void CustomCheckBox::setState(int state)
{
    if(state == 1)
        setChecked(true);
    else
        setChecked(false);
}


//////////////////////////IntTableWidgetItem//////////////////////////////////////
ValidableTableWidgetItem::ValidableTableWidgetItem(QValidator* validator, int type)
    : QTableWidgetItem(QTableWidgetItem::UserType + 1)
{
    this->validator = validator;
}

ValidableTableWidgetItem::ValidableTableWidgetItem(const QString &text, QValidator* validator, int type)
    : QTableWidgetItem(QTableWidgetItem::UserType + 1)
{
    this->validator = validator;
    setText(text);
}

QVariant ValidableTableWidgetItem::data(int role) const
{
    if((role == Qt::EditRole) || (role == Qt::DisplayRole)){
        return QVariant(strData);
    }

    return QTableWidgetItem::data(role);
}

void ValidableTableWidgetItem::setData(int role, const QVariant& value)
{
    if((role == Qt::EditRole) || (role == Qt::DisplayRole)){
        QString input = value.toString();
        int pos;
        if(validator->validate(input, pos) == QValidator::Invalid)
            setText(strData);
        else
            strData = input;
    }
    QTableWidgetItem::setData(role, value);
}

//void ValidableTableWidgetItem::setText(const QString &text)
//{
//    int pos;
//    QString input = text;
//    if(validator->validate(input, pos) == QValidator::Invalid)
//        setText("");
//    else
//        QTableWidgetItem::setText(input);
//}

////////////////////////MyMdiSubWindow///////////////////////////////////////



//void MyMdiSubWindow::centrlWidgetClosed()
//{
//    //emit windowClosed(this);
//    close();
//}

void MyMdiSubWindow::closeEvent(QCloseEvent *closeEvent)
{
    emit windowClosed(this);
    //delete widget();
    QMdiSubWindow::closeEvent(closeEvent);
}


/////////////////////////BusinessActionItem/////////////////////////////////////////////////
BASummaryItem::BASummaryItem(const QString content,
                             SubjectManager* subMgr, int type) : QTableWidgetItem(type)
{
    oppoSubject = 0;
    subManager = subMgr;
    parse(content);
}


QTableWidgetItem* BASummaryItem::clone() const
{
    QString content = assembleContent();
    BASummaryItem *item = new BASummaryItem(content);
    *item = *this;
    return item;
}

//返回指定角色的数据内容
QVariant BASummaryItem::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole)
        return summary;
    if(role == Qt::EditRole)        
        return assembleContent();
    if(role == Qt::ToolTipRole)
        return genQuoteInfo();
    return QTableWidgetItem::data(role);

}


void BASummaryItem::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        parse(value.toString());
    QTableWidgetItem::setData(role, value);
}

//将内容解析分离为摘要体和引用体
void BASummaryItem::parse(QString content)
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

//装配内容
QString BASummaryItem::assembleContent() const
{
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

//生成摘要引用部分的信息以便在工具提示框中显示
QString BASummaryItem::genQuoteInfo() const
{
    QString s;
    if(fpNums.count() > 0){
        s = QObject::tr("发票号码：");
        for(int i = 0; i < fpNums.count(); ++i)
            s.append(fpNums[i]).append(",");
        s.chop(1);
    }
    if(bankNums != "")
        s.append("\n").append(QObject::tr("银行票据号："))
                .append(bankNums);
    if(oppoSubject != 0)
        s.append("\n").append(QObject::tr("对方科目："))
                .append(subManager->getFstSubject(oppoSubject)->getName());

    return s;
}


///////////////////////////////BAFstSubItem///////////////////////////////
BAFstSubItem::BAFstSubItem(int subId, SubjectManager* subMgr,int type) :
    QTableWidgetItem(type)
{
    this->subId = subId;
    subManager = subMgr;
}

QVariant BAFstSubItem::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole){
        FirstSubject* fsub = subManager->getFstSubject(subId);
        if(fsub)
            return fsub->getName();
        else
            return "";
    }
    if(role == Qt::EditRole)
        return subId;
    return QTableWidgetItem::data(role);
}

void BAFstSubItem::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        subId = value.toInt();
    QTableWidgetItem::setData(role, value);
}


///////////////////////////BASndSubItem//////////////////////////////
BASndSubItem::BASndSubItem(int subId, SubjectManager* smg,int type):
    subId(subId),smg(smg),QTableWidgetItem(type)
{

}

QVariant BASndSubItem::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::ToolTipRole){
        QString tip = smg->getSndSubject(subId)->getLName();
        //如果是银行科目，则显示银行账户信息
        BankAccount* ba=0; bool found = false;
        foreach(ba,smg->getBankAccounts()){
            if(ba->subObj->getId() == subId){
                found = true;
                break;
            }
        }
        if(found){
            tip.append("\n").append(QObject::tr("帐号：%1\n").arg(ba->accNumber));
            tip.append(QObject::tr("是否基本户："));
            if(ba->bank->isMain)
                tip.append(QObject::tr("是"));
            else
                tip.append(QObject::tr("否"));
        }
        return tip;
    }
    if(role == Qt::DisplayRole){
        SecondSubject* ssub = smg->getSndSubject(subId);
        if(ssub)
            return ssub->getName();
        else
            return "";
    }
    if(role == Qt::EditRole)
        return subId;
    return QTableWidgetItem::data(role);
}

void BASndSubItem::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        subId = value.toInt();
    QTableWidgetItem::setData(role, value);
}


///////////////////////////BAMoneyTypeItem/////////////////////////////
BAMoneyTypeItem::BAMoneyTypeItem(int mt, QHash<int, QString>* mts,
                                 int type) : QTableWidgetItem(type)
{
    this->mt = mt;
    this->mts = mts;
}

QVariant BAMoneyTypeItem::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole)
        return mts->value(mt);
    if(role == Qt::EditRole)
        return mt;
    return QTableWidgetItem::data(role);
}

void BAMoneyTypeItem::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        mt = value.toInt();
    QTableWidgetItem::setData(role, value);
}

//////////////////////////////////BAMoneyValueItem////////////////////////
//设置金额的借贷方向
void BAMoneyValueItem::setDir(int dir)
{
    if((dir == DIR_J) || (dir == DIR_P) || (dir == DIR_D))
        witch = dir;
    else
        qDebug() << "direction code is invalid!!";
}

QVariant BAMoneyValueItem::data(int role) const
{
    if(role == Qt::ForegroundRole){
        if(v < 0.00)             //负数用红色
            return qVariantFromValue(QColor(Qt::red));
        else if(witch == DIR_J)  //借方金额用蓝色
            return qVariantFromValue(QColor(Qt::blue));
        else if(witch == DIR_D)  //贷方金额用绿色
            return qVariantFromValue(QColor(Qt::green));
    }
    if(role == Qt::DisplayRole){
        return v.toString();
//        if(v == 0)
//            return "";
//        else{
//            QString s = QString::number(v, 'f', 2);
//            if(s.right(3) == ".00")
//                s.chop(3);
//            if((s.right(1) == "0") && (s.indexOf(".") != -1))
//                s.chop(1);
//            return  s;
//        }
    }
    if(role == Qt::EditRole)
        return v.getv();
    return QTableWidgetItem::data(role);
}

void BAMoneyValueItem::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        v = Double(value.toDouble());
    QTableWidgetItem::setData(role, value);
}

//void BAMoneyValueItem::setData(int role, const QVariant &value)
//{
//    if(role == Qt::EditRole)
//        v = value.toDouble();
//    QTableWidgetItem::setData(role, value);
//}

//////////////////////////tagItem//////////////////////////////////
tagItem::tagItem(bool isTag, int type) : QTableWidgetItem(type),tag(isTag)
{
    tagIcon = QIcon(":/images/tag.png");
    notTagIcon = QIcon(":/images/not_tag.png");
}

QVariant tagItem::data(int role) const
{
    if(role == Qt::DecorationRole){
        if(tag)
            return tagIcon;
        else
            return notTagIcon;
    }
    else if(role == Qt::DisplayRole)
        return desc;
    else if(role == Qt::EditRole)
        return tag;
    return QTableWidgetItem::data(role);
}

void tagItem::setData(int role, const QVariant &value)
{
    if(role == Qt::DisplayRole)
        desc = value.toString();
    else if(role == Qt::EditRole)
        tag = value.toBool();
    QTableWidgetItem::setData(role,value);
}


///////////////////////////ActionEditTableWidget////////////////////////
ActionEditTableWidget::ActionEditTableWidget(QWidget* parent):QTableWidget(parent)
{
    //connect(this,SIGNAL(cellClicked(int,int)),this,SLOT(cellClicked(int,int)));
    //setVerticalHeader();

    int subSys = curAccount->getCurSuite()->subSys;
    smg = curAccount->getSubjectManager(subSys);

    volidRows = 0;
    rowTag = false;
    //isReadOnly = false;  //默认表格可编辑
    setRowCount(INITROWS);
    setColumnCount(10);
    QStringList headers;  //设置表格水平标题
    headers << tr("摘   要") << tr("总账科目") << tr("明细科目")
            << tr("币种") << tr("借方金额") << tr("贷方金额")
            << "DIR" << "ID"<< "PID"<< "NUM";
    setHorizontalHeaderLabels(headers);

    connect(this, SIGNAL(currentCellChanged(int,int,int,int)),
            this,SLOT(currentCellChanged(int,int,int,int)));


}

//交换表格行
void ActionEditTableWidget::switchRow(int r1,int r2)
{
    if((r1 < 0) || (r2 < 0))
        return;
    if(r1 == r2)
        return;
    int ru,rd; //上面、下面一行的行序号
    if(r1 > r2){
        ru = r2; rd = r1;
    }
    else{
        ru = r1; rd = r2;
    }
    if(rd > rowCount()-1)
        return;

    QList<QTableWidgetItem*> items;
    //保存上面一行的表格项
    for(int i = 0; i < columnCount(); ++i)
        items.append(takeItem(ru,i));
    //将下面一行的表格项移到上面一行
    for(int i = 0; i < columnCount(); ++i)
        setItem(ru,i,takeItem(rd,i));
    //将保存的上面一行的表格项设置到下面一行
    for(int i = 0; i < items.count(); ++i)
        setItem(rd,i,items[i]);

}

//当前行是否是有效行
bool ActionEditTableWidget::isCurRowVolid()
{
    return currentRow() < volidRows;  //所有有效行与最下面的有效行的下一行
}

////当前行
//bool ActionEditTableWidget::isRailRow()
//{

//}

//是否有选择的行
bool ActionEditTableWidget::hasSelectedRow()
{
    bool found = false;
    QItemSelectionModel* smodel = selectionModel();
    if(smodel->hasSelection()){
        int i = 0;
        while((!found) && i < volidRows){
            if(smodel->isRowSelected(i, QModelIndex()))
                found = true;
            i++;
        }
    }
    return found;
}

//返回整行选择的行列表（只返回有效行）
void ActionEditTableWidget::selectedRows(QList<int>& rows, bool& isContinue)
{
    if(!rows.empty())
        rows.clear();
    QList<QTableWidgetSelectionRange> selRange = selectedRanges();

    bool contiTag = true;
    int row;
    for(int i = 0; i < selRange.count(); ++i){
        row = selRange[i].topRow();
        for(int j = 0; j < selRange[i].rowCount(); ++j){
            if((selRange[i].columnCount() > 6) && ((row+j) < volidRows))  //因为还有隐藏列，因此选择的列数可能大于可见到列数
                rows << row + j;
        }
    }
    qSort(rows.begin(),rows.end());  //一定要确保选择的行顺序，越上面的行越靠前，这是为了保证删除时的正确性
    for(int i = 1; i < rows.count(); ++i){
        if(rows[i] - rows[i-1] != 1){
            contiTag = false;
            break;
        }
    }
    if(!rows.empty())
        isContinue = contiTag;
    else
        isContinue = false;

    //还应考虑剔除无效行！！！！！
}

/**
 * @brief ActionEditTableWidget::newSndSubMapping
 *  创建新的二级科目
 * @param pid   一级科目id
 * @param nid   名称条目id
 * @param row
 * @param col
 * @param reqConfirm
 */
void ActionEditTableWidget::newSndSubMapping(int pid, int nid,
                                             int row, int col, bool reqConfirm)
{
    QString s;
    SubjectNameItem* ni = smg->getNameItem(nid);

    s = QString(tr("确定要在一级科目“%1”下使用已有名称“%2”创建二级科目吗？")
                           .arg(smg->getFstSubject(pid)->getName()).arg(ni->getShortName()));
    if(!reqConfirm || (QMessageBox::Yes == QMessageBox::question(0,
        QObject::tr("确认消息"), s, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))){
        SecondSubject* ssub = smg->addSndSubject(smg->getFstSubject(pid),ni);
        item(row,col)->setData(Qt::EditRole, ssub->getId()); //在表格项中设置新条目对应的id
    }
    else
        item(row,col)->setData(Qt::EditRole, 0);
}

/**
 * @brief ActionEditTableWidget::newSndSubAndMapping
 *  创建新的名称条目，并用此名称在指定一级科目下创建二级科目
 * @param fid
 * @param name
 * @param row
 * @param col
 */
void ActionEditTableWidget::newSndSubAndMapping(int fid, QString name, int row, int col)
{
    int subSys = curAccount->getCurSuite()->subSys;
    SubjectManager* sm = curAccount->getSubjectManager(subSys);
    FirstSubject* fsub = sm->getFstSubject(fid);
    QString s = tr("确定要创建新的名称条目“%1”，并利用新建名称在一级科目“%2”下创建二级科目吗？")
            .arg(name).arg(fsub->getName());
    if(QMessageBox::Yes ==
            QMessageBox::question(this,tr("确认消息"),s,QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes)){
        CompletSubInfoDialog* dlg = new CompletSubInfoDialog(fid,smg,this);
        dlg->setName(name);
        if(QDialog::Accepted == dlg->exec()){
            //在SecSubjects表中插入新的二级科目条目
            QString sname = dlg->getSName();
            QString lname = dlg->getLName();
            QString remCode = dlg->getRemCode();
            int cls = dlg->getSubCalss();
            SubjectNameItem* ni = sm->addNameItem(sname,lname,remCode,cls);
            SecondSubject* ssub = sm->addSndSubject(fsub,ni);
            item(row,col)->setData(Qt::EditRole, ssub->getId()); //在表格项中设置新条目对应的id
        }
    }
}

/**
 * @brief ActionEditTableWidget::sndSubjectDisabeld
 *  二级科目已被禁用
 * @param id
 */
void ActionEditTableWidget::sndSubjectDisabeld(int id)
{
    QMessageBox::warning(0,tr("警告信息"),tr("二级科目（%1）已被禁用！").arg(smg->getSndSubject(id)->getName()));
}

void ActionEditTableWidget::currentCellChanged (int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    //如果是从前一行的末列转到下一行的第一列（这个动作一般是由在贷方列按回车键后发生）
    if((previousColumn == ActionEditItemDelegate2::DV)
            && (currentColumn == ActionEditItemDelegate2::SUMMARY)
            && (currentRow == previousRow + 1)){
        rowTag = true;
    }
    else
        rowTag = false;
    curRow = currentRow;
    curCol = currentColumn;
}



//在最后一个有效行行添加数个新行
void ActionEditTableWidget::appendRow(int rowNums)
{
    int n = volidRows + rowNums- rowCount();
    if(n > 0)
        for(int i = 0; i < n; ++i)
            QTableWidget::insertRow(volidRows+i);
    volidRows += rowNums;
}

//void ActionEditTableWidget::cellClicked(int row, int column)
//{
//    int i = 0;
//}

//在指定位置插入数行
void ActionEditTableWidget::insertRows(int pos, int rowNums)
{
    if(pos >= volidRows) //必须在有效行数的范围内才可以执行插入，即中间不能留有无效行
        return;
    for(int i = 0; i < rowNums; ++i)
        insertRow(pos);
    volidRows += rowNums;
}

//在指定位置移除数行
void ActionEditTableWidget::removeRows(int pos, int rowNums)
{
    int n =(pos+ 1+rowNums)  - volidRows;
    n = (n>0)?n:rowNums;
    for(int i = 0; i < n; n--)
        removeRow(pos);
}

void ActionEditTableWidget::keyPressEvent(QKeyEvent* e)
{
    int key = e->key();
    int row = currentRow();    //在编辑器打开的时候，获取的行列号无效，不知何故？
    int col = currentColumn(); //但又不能避免在编辑器打开的时候处理此消息，这是一个潜在的bug


    //if(e->isAccepted()){
        //按下等号自动请求创建合计对等业务活动
        if(key == Qt::Key_Equal){
            if(e->modifiers() == Qt::ControlModifier){
                emit reqAutoCopyToCurAction();
            }
            else
                emit requestCrtNewOppoAction();

        }
        //else if(static_cast<CustomKeyEvent*>(e))
        //    emit requestAppendNewAction(row+1);
        //这个只能在还没有打开编辑器的时候，在贷方列按回车键时满足条件
        else if(((key == Qt::Key_Return) || (key == Qt::Key_Enter))
                && col == ActionEditItemDelegate2::DV){
            emit requestAppendNewAction(row+1);
        }
    //}
    QTableWidget::keyPressEvent(e);
}

void ActionEditTableWidget::mousePressEvent(QMouseEvent* event)
{
    if(Qt::RightButton == event->button()){
        int row = rowAt(event->y());
        int col = columnAt(event->x());
        emit requestContextMenu(row, col);
    }
    QTableWidget::mousePressEvent(event);
}


//////////////////////////////////DirItem///////////////////////////////
DirItem::DirItem(int dir, int type) : QTableWidgetItem(type)
{
    this->dir = dir;
}

QVariant DirItem::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole){
        if(dir == DIR_J)
            return QObject::tr("借");
        else if(dir == DIR_D)
            return QObject::tr("贷");
        else
            return QObject::tr("平");
    }
    if(role == Qt::EditRole)
        return dir;
    return QTableWidgetItem::data(role);
}

void DirItem::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        dir = value.toInt();
    QTableWidgetItem::setData(role, value);
}


/////////////////////////////CustomSpinBox/////////////////////////////////////////
CustomSpinBox::CustomSpinBox(QWidget* parent) : QSpinBox(parent){}

void CustomSpinBox::keyPressEvent(QKeyEvent * event)
{
    int key = event->key();
    if((key == Qt::Key_Return) || (key == Qt::Key_Enter))
        emit userEditingFinished();

    QSpinBox::keyPressEvent(event);
}


////////////////////////////SubjectComplete2//////////////////////////////////////////
SubjectComplete::SubjectComplete(SujectLevel witch, QObject *parent)
    : QCompleter(parent),witch(witch)
{
    dbUtil = curAccount->getDbUtil();
    q = dbUtil->getQuery();
    pid = 0; //如果服务于二级科目，则初始时默认是所有二级科目名列表
    tv.setRootIsDecorated(false);
    tv.header()->hide();
    tv.header()->setStretchLastSection(false);
    tv.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(&tv,SIGNAL(clicked(QModelIndex)),this,SLOT(clickedInList(QModelIndex)));
}

SubjectComplete::~SubjectComplete()
{
    delete q;
}

//设置一级科目的id以限制可选的二级科目范围
void SubjectComplete::setPid(int pid)
{
    //只有当此完成器服务于二级科目时，才有意义
    if(witch == 2){
        this->pid = pid;

    }
}

QString SubjectComplete::pathFromIndex(const QModelIndex &index) const
{
    const QAbstractItemModel* model = index.model();
    int id = model->data(model->index(index.row(),2)).toInt();
    QComboBox* w = static_cast<QComboBox*>(widget());
    QString subName;
    //如果从完成列表中选择的项目在组合框内没有，则返回空字符串
    if(w && w->findData(id) != -1)
        subName = model->data(model->index(index.row(),0)).toString();
    return subName;
}

bool SubjectComplete::eventFilter(QObject *obj, QEvent *e)
{
    if(obj == widget() && e->type() == QEvent::KeyPress){
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(e);
        int key = keyEvent->key();
        if((key >= Qt::Key_0) && key <= Qt::Key_9){
            if(keyBuf.count() == 0){
                keyBuf.append(key);
                if(witch == 1){
                    QString s = QString("select %1,%2,id from %3 where (%4=1) ")
                            .arg(fld_fsub_name).arg(fld_fsub_subcode).arg(tbl_fsub).arg(fld_fsub_isview);
                    if(!filter.isEmpty())
                        s.append(" and ").append(filter);
                    s.append(QString(" order by %1").arg(fld_fsub_subcode));
                    q->exec(s);
                    m.setQuery(*q);
                }
                else
                    return QCompleter::eventFilter(obj,e); //目前对二级科目不做代码处理
                setModel(&m); //只有在设置了模型后，对tv的调整才会有效
                setPopup(&tv);
                setCompletionColumn(1);
                tv.setColumnWidth(1,60);
                //tv.hideColumn(1);
                tv.hideColumn(2);
            }
            else
                keyBuf.append(key);
            return QCompleter::eventFilter(obj,e);
        }
        else if((key >= Qt::Key_A) && (key <= Qt::Key_Z)){
            if(keyBuf.count() == 0){
                QString s;
                keyBuf.append(key);
                if(witch == 1){
                    s = QString("select %1,%2,id,%3 from %4 where (%5=1)")
                            .arg(fld_fsub_name).arg(fld_fsub_remcode).arg(fld_fsub_subcode)
                            .arg(tbl_fsub).arg(fld_fsub_isview);
                    if(!filter.isEmpty())
                        s.append(" and ").append(filter);
                    s.append(QString(" order by %1").arg(fld_fsub_remcode));
                    q->exec(s);
                    m.setQuery(*q);
                    LOG_DEBUG(QString("model sql statement is '%1'").arg(s));
                }
                else{
                    if(pid != 0)
                        s = QString("select %1.%2,%1.%3,%4.id from %1 join %4 on "
                                "%1.id = %4.%5 where %6=%7")
                                .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_remcode)
                                .arg(tbl_ssub).arg(fld_ssub_nid).arg(fld_ssub_fid).arg(pid);
                    else
                        s = QString("select %1.%2,%1.%3,%4.id from %1 join %4 on %1.id = %4.%5")
                                .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_remcode)
                                .arg(tbl_ssub).arg(fld_ssub_fid);

                    //LOG_DEBUG(QString("model station is '%1'").arg(s));
                    q->exec(s);
                    m.setQuery(*q);
                }
                setModel(&m); //只有在设置了模型后，对tv的调整才会有效
                setPopup(&tv);
                setCompletionColumn(1);
                tv.setColumnWidth(1,60);
                if(witch == 1)
                    tv.hideColumn(1);
                tv.hideColumn(2);
            }
            else
                keyBuf.append(key);
            return QCompleter::eventFilter(obj,e);
        }
        else
            return QCompleter::eventFilter(obj, e);
    }
    else
        return QCompleter::eventFilter(obj, e);

}

//当用户用鼠标在完成列表中选择一个完成项时，将与完成器相连的组合框的当前项设置为用户选择的项
//很奇怪，用户通过键盘操作选择完成列表中的项时，能够正确设置与之相连的组合框的当前选择项？
void SubjectComplete::clickedInList(const QModelIndex &index)
{
    const QAbstractItemModel* m = index.model();
    int id = m->data(m->index(index.row(),2)).toInt();
    QComboBox* w = static_cast<QComboBox*>(widget());
    if(w){
        //如果完成列表中有，但组合框内没有，如何处理？
        int idx = w->findData(id);
        w->setCurrentIndex(idx);
    }
}


////////////////////////////MultiRowHeaderTableView//////////////////////////////////////////
MultiRowHeaderTableView::MultiRowHeaderTableView(QAbstractItemModel *model,
                                                 QAbstractItemModel *hmodel, QWidget *parent) : QTableView(parent)
{

    setModel(model);
    headView = new QTableView(this);
    headView->setModel(hmodel);
    headView->setFocusPolicy(Qt::NoFocus);
    headView->verticalHeader()->hide();
    headView->horizontalHeader()->hide();
    headView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    headView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    headView->horizontalHeader()->setResizeMode(QHeaderView::Fixed);
    viewport()->stackUnder(headView); //使作为标题的表格显示在数据表格之上
    //headView->setSelectionModel(selectionModel());
    for(int col=1; col<model->columnCount(); col++)
        headView->setColumnHidden(col, true);
    headView->show();

    init();
}

MultiRowHeaderTableView::~MultiRowHeaderTableView()
{
    delete headView;
}

void MultiRowHeaderTableView::init()
{

}

//更新标题表格的几何尺寸
void MultiRowHeaderTableView::updateHeadTableGeometry()
{
    headView->setGeometry(verticalHeader()->width()+frameWidth(),
                            frameWidth(), columnWidth(0),
                            viewport()->height()+horizontalHeader()->height());
}

 ///////////////////////////////////////////////////////////////////
ApStandardItem::ApStandardItem(bool editable):QStandardItem()
{
    setEditable(editable);
}

ApStandardItem::ApStandardItem(Double v, Qt::Alignment align, bool editable):QStandardItem(v.toString())
{
    setTextAlignment(align);
    setEditable(editable);
}

ApStandardItem::ApStandardItem(QString text, Qt::Alignment align,bool editable):QStandardItem(text)
{
    setTextAlignment(align);
    setEditable(editable);
}

ApStandardItem::ApStandardItem(int v, Qt::Alignment align,bool editable):QStandardItem()
{
    if(v != 0)
        setText(QString::number(v));
    setTextAlignment(align);
    setEditable(editable);

}

ApStandardItem::ApStandardItem(double v, int decimal, Qt::Alignment align,bool editable):QStandardItem()
{
    int factor=1;
    for(int i = 0; i < decimal; ++i)
        factor *=10;
    double d = 1 / factor;
    if(v > d){
        //截去小数部分多余的零
        QString s = QString::number(v,'f',decimal);
        while(true){
            if(s.indexOf('.') == -1)
                break;
            if(s.endsWith('0') || s.endsWith('.'))
                s.chop(1);
            else
                break;
        }
        setText(s);
    }
    setTextAlignment(align);
    setEditable(editable);
}


////////////////////////////////////////////////////////////////////////////
ApSpinBox::ApSpinBox(QWidget* parent) : QSpinBox(parent){}

void ApSpinBox::setDefaultValue(int v, QString t)
{
    if((v > minimum()) && v < maximum()){
        defValue = v;
        defText = t;
        setValue(v);
    }
}

QString ApSpinBox::textFromValue(int value) const
{
    if(value == defValue)
        return defText;
    else
        return QSpinBox::textFromValue(value);
}


//int ApSpinBox::valueFromText(const QString& text) const
//{

//}


////////////////////////////////////////////////
void IntentButton::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
        emit intentClicked(intent);
    QPushButton::mousePressEvent(event);
}
