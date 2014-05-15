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

//void MyMdiSubWindow::close()
//{
//    if(isHideWhenColse)
//        hide();
//    else{
//        emit windowClosed(this);
//        QMdiSubWindow::close();
//    }
//}

void MyMdiSubWindow::closeEvent(QCloseEvent *closeEvent)
{
    if(isHideWhenColse)
        hide();
    else{
        emit windowClosed(this);
        //delete widget();
        QMdiSubWindow::closeEvent(closeEvent);
    }
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

//////////////////////////////////BAFstSubItem2//////////////////////////////////////////////////
BAFstSubItem2::BAFstSubItem2(FirstSubject *fsub, int type):QTableWidgetItem(type),fsub(fsub)
{}

QVariant BAFstSubItem2::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole){
        return fsub?fsub->getName():"";
    }
    if(role == Qt::EditRole){
        QVariant v;
        v.setValue<FirstSubject*>(fsub);
        return v;
    }
    return QTableWidgetItem::data(role);
}

void BAFstSubItem2::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        fsub = value.value<FirstSubject*>();
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
        SecondSubject* ssub = smg->getSndSubject(subId);
        QString tip = ssub->getLName();
        //如果是银行科目，则显示银行账户信息
        if(smg->isBankSndSub(ssub)){
            BankAccount* ba = smg->getBankAccount(ssub);
            tip.append("\n").append(QObject::tr("帐号：%1\n").arg(ba->accNumber));
            tip.append(QObject::tr("是否基本户："));
            if(ba->parent->isMain)
                tip.append(QObject::tr("是"));
            else
                tip.append(QObject::tr("否"));
        }
        return tip;

//        BankAccount* ba=0; bool found = false;
//        foreach(ba,smg->getBankAccounts()){
//            if(ba->subObj->getId() == subId){
//                found = true;
//                break;
//            }
//        }
//        if(found){
//            tip.append("\n").append(QObject::tr("帐号：%1\n").arg(ba->accNumber));
//            tip.append(QObject::tr("是否基本户："));
//            if(ba->bank->isMain)
//                tip.append(QObject::tr("是"));
//            else
//                tip.append(QObject::tr("否"));
//        }
//        return tip;
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
SubjectComplete::SubjectComplete(int subSys, SujectLevel witch, QObject *parent)
    : QCompleter(parent),witch(witch),subSys(subSys)
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
    //LOG_INFO("enter into SubjectComplete::pathFromIndex");
    const QAbstractItemModel* model = index.model();
    int id = model->data(model->index(index.row(),2)).toInt();
    QComboBox* w = static_cast<QComboBox*>(widget());
    QString subName;
    //如果从完成列表中选择的项目在组合框内没有，则返回空字符串
    if(w && findSubject(id) != -1){
        subName = model->data(model->index(index.row(),0)).toString();
        //LOG_INFO(QString("SubjectComplete::pathFromIndex: subName = %1").arg(subName));
    }
    else
        subName = tr("未包含");
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
                    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
                    QString s = QString("select %1,%2,%3 from %4 where (%5=1) ")
                            .arg(fld_fsub_name).arg(fld_fsub_subcode).arg(fld_fsub_fid)
                            .arg(tname).arg(fld_fsub_isEnalbed);
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
                    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
                    s = QString("select %1,%2,%3,%4 from %5 where (%6=1)")
                            .arg(fld_fsub_name).arg(fld_fsub_remcode).arg(fld_fsub_fid)
                            .arg(fld_fsub_subcode).arg(tname).arg(fld_fsub_isEnalbed);
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
//        else if(key == Qt::Key_Up || key == Qt::Key_Down){
//            LOG_INFO(QString("SubjectComplete.eventFilter(): arrow key"));
//        }
//        else if((key == Qt::Key_Enter) && (key == Qt::Key_Return)){
//            LOG_INFO(QString("SubjectComplete.eventFilter(): enter key"));
//        }
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
    //LOG_INFO(QString("SubjectComplete.clickedInList: subject id=%1").arg(id));
    QComboBox* w = static_cast<QComboBox*>(widget());
    if(w){
        //如果完成列表中有，但组合框内没有，如何处理？
        int idx = findSubject(id);
        //LOG_INFO(QString("SubjectComplete.clickedInList: Fonded subject index=%1").arg(idx));
        w->setCurrentIndex(idx);
    }
}

/**
 * @brief 查找指定id的科目在组合框中的索引位置
 * @param id
 * @return
 */
int SubjectComplete::findSubject(int id) const
{
    QComboBox* w = static_cast<QComboBox*>(widget());
    if(!w)
        return -1;
    if(w->count() == 0)
        return -1;
    //LOG_INFO(QString("Total have %1 items in ComboBox").arg(w->count()));
    for(int i = 0; i < w->count(); ++i){
        SubjectBase* sub;
        if(witch == 1)
            sub = w->itemData(i).value<FirstSubject*>();
        else
            sub = w->itemData(i).value<SecondSubject*>();
        if(!sub){
            //LOG_INFO(QString("subject don't fonded at index=%1!").arg(i));
            continue;
        }
        if(sub->getId() == id){
            //LOG_INFO(QString("Fonded subject(%1) at index = %2").arg(sub->getName()).arg(i));
            return i;
        }
        else
            continue;
    }
    return -1;
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
    headView->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
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



