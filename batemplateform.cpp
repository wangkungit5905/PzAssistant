#include "batemplateform.h"
#include "ui_batemplateform.h"
#include "PzSet.h"
#include "pzdialog.h"
#include "subject.h"
#include "myhelper.h"
#include "completsubinfodialog.h"
#include "commands.h"
#include "utils.h"
#include "widgets/bawidgets.h"
#include "dbutil.h"

#include <QInputDialog>
#include <QListWidget>
#include <QKeyEvent>
#include <QDebug>
#include <QMenu>
#include <QSpinBox>

#include "widgets.h"


InvoiceNumberEdit::InvoiceNumberEdit(QWidget *parent):QLineEdit(parent)
{
    re.setPattern("\\s{0,}(\\d{8})(((/\\d{2,4}){0,})|((-\\d{2,2}){0,1}))");
    connect(this,SIGNAL(returnPressed()),this,SLOT(invoiceEditCompleted()));
}

void InvoiceNumberEdit::invoiceEditCompleted()
{
    QStringList invoices;
    int pos = 0;
    while(true){
        pos = re.indexIn(text(),pos);
        if(pos == -1)
            break;
        invoices<<re.cap(1);
        if(!re.cap(2).isEmpty()){
            //断续型
            if(re.cap(2).contains('/')){
                QString ss = re.cap(2).right(re.cap(2).count()-1);
                QStringList suffs = ss.split('/');
                foreach(QString suff, suffs){
                    QString preStr = re.cap(1).left(8-suff.count());
                    invoices<<preStr + suff;
                }
            }
            else{ //连续型
                int len = re.cap(2).count() - 1;
                QString preStr = re.cap(1).left(8-len);
                int start = re.cap(1).right(len).toInt();
                int end = re.cap(2).right(len).toInt();
                if(abs(start-end)>0){
                    if(start > end){
                        int tem = start;
                        start = end;
                        end = tem;
                    }
                    for(int i = start+1; i <= end; ++i){
                        QString suffix;
                        if(i < 10)
                            suffix = "0" + QString::number(i);
                        else
                            suffix = QString::number(i);
                        QString ins = preStr+suffix;
                        invoices<<ins;
                    }
                }
            }
        }
        pos += re.matchedLength();
    }
    if(invoices.count() > 1){
        setText(invoices.first());
        emit inputedMultiInvoices(invoices);
    }
    if(invoices.isEmpty() && !text().isEmpty()){
        emit dataEditCompleted(BaTemplateForm::CI_INVOICE,false);
        QMessageBox::warning(0,"",tr("发票号输入为空或包含非法字符或格式不符合规范！"));        
    }
    else
        emit dataEditCompleted(BaTemplateForm::CI_INVOICE,true);
}

////////////////////////MoneyEdit///////////////////////////////////
QValidator::State MyDoubleValidator::validate(QString &input, int &pos) const
{
    if(input.isEmpty())
        return QValidator::Acceptable;
    else
        return QDoubleValidator::validate(input,pos);
}



MoneyEdit::MoneyEdit(QWidget *parent):QLineEdit(parent),isRightBottomCell(false)
{
    MyDoubleValidator* validator = new MyDoubleValidator(this);
    validator->setDecimals(2);
    setValidator(validator);
    connect(this,SIGNAL(editingFinished()),this,SLOT(moneyEditCompleated()));
}

void MoneyEdit::moneyEditCompleated()
{
    emit dataEditCompleted(colIndex,!isRightBottomCell);
}

//////////////////////////////CustomerNameEdit/////////////////////////////////
CustomerNameEdit::CustomerNameEdit(SubjectManager *subMgr, QWidget *parent)
    :QWidget(parent),sm(subMgr),fsub(0),ssub(0),sortBy(SORTMODE_NAME)
{
    com = new QComboBox(this);
    com->setEditable(true);       //使其可以输入新的名称条目
    com->installEventFilter(this);
    installEventFilter(this);
    lw = new QListWidget(this);
    lw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(lw,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(itemSelected(QListWidgetItem*)));

    lw->setHidden(true);
    QVBoxLayout* l = new QVBoxLayout;
    l->setSpacing(0);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(com);
    l->addWidget(lw);
    setMaximumHeight(200);
    setLayout(l);

    connect(com,SIGNAL(currentIndexChanged(int)),this,SLOT(subSelectChanged(int)));
    connect(com->lineEdit(),SIGNAL(textEdited(QString)),this,SLOT(nameTextChanged(QString)));
    connect(com->lineEdit(),SIGNAL(returnPressed()),this,SLOT(nameTexteditingFinished()));

    //装载所有名称条目
    allNIs = sm->getAllNameItems();
    qSort(allNIs.begin(),allNIs.end(),byNameThan_ni);
    QListWidgetItem* item;
    QVariant v;
    foreach(SubjectNameItem* ni, allNIs){
        v.setValue(ni);
        item = new QListWidgetItem(ni->getShortName());
        item->setData(Qt::UserRole, v);
        lw->addItem(item);
    }
    if(AppConfig::getInstance()->ssubFirstlyInputMothed())
        this->setFocusProxy(com);
}

void CustomerNameEdit::setCustomerName(QString name)
{
    ssub = fsub->getChildSub(name);
}

QString CustomerNameEdit::customerName()
{
    if(ssub)
        return ssub->getName();
    else
        return "";
}

void CustomerNameEdit::setFSub(FirstSubject *fsub, const QList<SecondSubject*> &extraSubs)
{
    if(this->fsub != fsub){
        this->fsub = fsub;
        QList<SecondSubject *> subs = fsub->getChildSubs();
        QVariant v;
        if(!extraSubs.isEmpty()){
            this->extraSSubs = extraSubs;
            subs += extraSubs;
            foreach(SecondSubject* sub, subs){
                if(sub->getNameItem()->getId() == 0){
                    int row = insertExtraNameItem(sub->getNameItem());
                    v.setValue(sub->getNameItem());
                    QListWidgetItem *item = new QListWidgetItem(sub->getNameItem()->getShortName());
                    item->setData(Qt::UserRole, v);
                    lw->insertItem(row,item);
                }
            }
        }
        foreach(SecondSubject* sub, subs){
            v.setValue(sub);
            com->addItem(sub->getName(),v);
        }
        com->setCurrentIndex(-1);
    }
}

void CustomerNameEdit::setSSub(SecondSubject *ssub)
{
    if(ssub->getParent() == fsub || extraSSubs.contains(ssub)){
        if(this->ssub != ssub){
            this->ssub = ssub;
            QVariant v;
            v.setValue(ssub);
            com->setCurrentIndex(com->findData(v,Qt::UserRole));
        }
    }
}

void CustomerNameEdit::hideList(bool isHide)
{
    lw->setHidden(isHide);
    if(isHide)
        resize(width(),com->height());
    else
        resize(width(),200);
}

void CustomerNameEdit::itemSelected(QListWidgetItem *item)
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
    else if(ssub = getMatchExtraSub(ni)){
        QVariant v;
        v.setValue(ssub);
        int index = com->findData(v,Qt::UserRole);
        com->setCurrentIndex(index);
    }
    else if(sm->containNI(ni)){
        SecondSubject* ssub = NULL;
        emit newMappingItem(fsub,ni,ssub,row,BaTemplateForm::CI_CUSTOMER);
        if(ssub){
            this->ssub = ssub;
            extraSSubs<<ssub;
            QVariant v;
            v.setValue(ssub);
            com->addItem(ssub->getName(),v);
            com->setCurrentIndex(com->count()-1);
        }
    }
    hideList(true);
}

void CustomerNameEdit::nameTextChanged(const QString &text)
{
    if(sortBy != SORTMODE_NAME)
        return;
    filterListItem();
    hideList(false);
}

void CustomerNameEdit::nameTexteditingFinished()
{    
    QString editText = com->lineEdit()->text().trimmed();
    if(editText.isEmpty())
        return;
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
    emit newSndSubject(fsub,ssub,editText,row,BaTemplateForm::CI_CUSTOMER);
    if(ssub){
        this->ssub = ssub;
        extraSSubs<<ssub;
        QVariant v;
        v.setValue(ssub);
        com->addItem(ssub->getName(),v);
        com->setCurrentIndex(com->count()-1);
    }
}

void CustomerNameEdit::subSelectChanged(int index)
{
    ssub = com->itemData(index).value<SecondSubject*>();
}

/**
 * @brief CustomerNameEdit::eventFilter
 * @param obj
 * @param event
 * @return
 */
bool CustomerNameEdit::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == this && event->type() == QEvent::KeyPress){
        //当编辑器刚打开（通过双击单元格），光标还未在组合框的编辑框中出现时，
        //如果按回车键，则无法捕获组合框对象上捕获，而是在编辑器对象本身上，
        //因此，必须在类对象自身安装事件过滤器
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        int keyCode = e->key();
        if((keyCode == Qt::Key_Return) || (keyCode == Qt::Key_Enter)){
            if(lw->isVisible()){
                QListWidgetItem* item = lw->currentItem();
                if(item){
                    SubjectNameItem* ni = item->data(Qt::UserRole).value<SubjectNameItem*>();
                    ssub = fsub->getChildSub(ni);
                    if(!ssub)
                        emit newMappingItem(fsub,ni,ssub,row,BaTemplateForm::CI_CUSTOMER);
                }
            }
            emit dataEditCompleted(BaTemplateForm::CI_CUSTOMER,true);
        }
        else if(keyCode == Qt::Key_Backspace){
            keys.chop(1);
            if(keys.size() == 0)
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
            keys.append(keyCode);
            if(keys.size() == 1){
                sortBy = SORTMODE_REMCODE;
                qSort(allNIs.begin(),allNIs.end(),byRemCodeThan_ni);
                lw->clear();
                QVariant v;
                foreach(SubjectNameItem* ni, allNIs){
                    v.setValue(ni);
                    QListWidgetItem* item = new QListWidgetItem(ni->getShortName());
                    item->setData(Qt::UserRole, v);
                    lw->addItem(item);
                }
                hideList(false);
            }
            filterListItem();
        }
        return true;
    }

    QComboBox* cmb = qobject_cast<QComboBox*>(obj);
    if(!cmb || cmb != com)
        return QObject::eventFilter(obj, event);
    if(event->type() == QEvent::KeyPress){
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        int keyCode = e->key();
        if(lw->isHidden()){
            if((keyCode == Qt::Key_Return) || (keyCode == Qt::Key_Enter)){
                emit dataEditCompleted(BaTemplateForm::CI_CUSTOMER,true/*!isLastRow*/);
                return true;
            }
        }
        else{
            if(keyCode == Qt::Key_Up){
                processArrowKey(true);
                return true;
            }
            if(keyCode == Qt::Key_Down){
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
 * @brief 处理上下箭头键盘事件（以调整智能列表框的当前选择项）
 * @param up
 * @return true：列表已经到顶或到底，无法继续移动
 */
bool CustomerNameEdit::processArrowKey(bool up)
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
 * @brief 仅显示以输入的文本开头的列表项
 */
void CustomerNameEdit::filterListItem()
{
    QString namePre = com->lineEdit()->text().trimmed();
    if(sortBy == SORTMODE_NAME){
        for(int i = 0; i < allNIs.count(); ++i){
            if(allNIs.at(i)->getShortName().startsWith(namePre,Qt::CaseInsensitive))
                lw->item(i)->setHidden(false);
            else
                lw->item(i)->setHidden(true);
        }
    }
    else if(sortBy == SORTMODE_REMCODE){
        for(int i = 0; i < allNIs.count(); ++i){
            if(allNIs.at(i)->getRemCode().startsWith(keys,Qt::CaseInsensitive))
                lw->item(i)->setHidden(false);
            else
                lw->item(i)->setHidden(true);
        }
    }
}

/**
 * @brief 返回额外科目列表中包含的与指定名称条目匹配的额外二级科目
 * @param ni
 * @return
 */
SecondSubject *CustomerNameEdit::getMatchExtraSub(SubjectNameItem *ni)
{
    if(extraSSubs.isEmpty())
        return 0;
    for(int i = 0; i < extraSSubs.count(); ++i){
        if(ni == extraSSubs.at(i)->getNameItem())
            return extraSSubs.at(i);
    }
    return 0;
}

/**
 * @brief 将额外科目所引用的新名称条目插入到已有的名称条目列表中
 * 以使新名称条目可以在智能提示列表中出现
 * @param ni
 * @return 插入的位置
 */
int CustomerNameEdit::insertExtraNameItem(SubjectNameItem *ni)
{
    for(int i = 0; i < allNIs.count(); ++i){
        if(ni->getShortName() >= allNIs.at(i)->getShortName()){
            allNIs.insert(i,ni);
            return i;
        }
    }
}
///////////////////////////InvoiceInputDelegate////////////////////////////////////////
InvoiceInputDelegate::InvoiceInputDelegate(SubjectManager *subMgr, QWidget *parent)
    :QItemDelegate(parent),sm(subMgr)
{
    p = static_cast<BaTemplateForm*>(parent);
}

void InvoiceInputDelegate::setTemplateType(BATemplateEnum type)
{
    templae=type;
    if(type == BATE_YS_INCOME || type == BATE_BANK_INCOME || type == BATE_YS_GATHER)
        fsub = sm->getYsSub();
    else
        fsub = sm->getYfSub();
}

QWidget *InvoiceInputDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int rows = index.model()->rowCount();
    if(index.row() == rows-1)
        return 0;
    int col = index.column();
    int row = index.row();
    if(col == BaTemplateForm::CI_MONTH){
        QSpinBox* edt = new QSpinBox(parent);
        edt->setMaximum(12);
        edt->setMinimum(1);
        return edt;
    }
    else if(col == BaTemplateForm::CI_INVOICE){
        InvoiceNumberEdit* edt = new InvoiceNumberEdit(parent);
        connect(edt,SIGNAL(dataEditCompleted(int,bool)),this,SLOT(commitAndCloseEditor(int,bool)));
        connect(edt,SIGNAL(inputedMultiInvoices(QStringList)),this,SLOT(multiInvoices(QStringList)));
        return edt;
    }
    else if(isDoubleColumn(col)){
        MoneyEdit* edt = new MoneyEdit(parent);
        edt->setColIndex(col);
        if((row == rows-2) && (col == BaTemplateForm::CI_WMONEY && validColumns == 5 ||
           col == BaTemplateForm::CI_CUSTOMER && validColumns == 6))
            edt->setLastCell(true);
        connect(edt,SIGNAL(dataEditCompleted(int,bool)),this,SLOT(commitAndCloseEditor(int,bool)));
        return edt;
    }
    else if(col == BaTemplateForm::CI_CUSTOMER){
        CustomerNameEdit* edt = new CustomerNameEdit(sm,parent);
        edt->setRowNum(row);
        edt->setFSub(fsub,p->getExtraSSubs());
        connect(edt,SIGNAL(dataEditCompleted(int,bool)),this,SLOT(commitAndCloseEditor(int,bool)));
        connect(edt,SIGNAL(newMappingItem(FirstSubject*,SubjectNameItem*,SecondSubject*&,int,int)),
                this,SLOT(newNameItemMapping(FirstSubject*,SubjectNameItem*,SecondSubject*&,int,int)));
        connect(edt,SIGNAL(newSndSubject(FirstSubject*,SecondSubject*&,QString,int,int)),
                this,SLOT(newSndSubject(FirstSubject*,SecondSubject*&,QString,int,int)));
        return edt;
    }
    return 0;
}

void InvoiceInputDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    if(col == BaTemplateForm::CI_MONTH){
        QSpinBox* edt = qobject_cast<QSpinBox*>(editor);
        if(edt)
            edt->setValue(index.model()->data(index).toInt());
    }
    else if(col == BaTemplateForm::CI_INVOICE){
        InvoiceNumberEdit* edt = qobject_cast<InvoiceNumberEdit*>(editor);
        if(edt)
            edt->setText(index.model()->data(index).toString());
    }
    else if(isDoubleColumn(col)){
        MoneyEdit* edt = qobject_cast<MoneyEdit*>(editor);
        if(edt){
            edt->setText(index.model()->data(index).toString());
        }
    }
    else if(col == BaTemplateForm::CI_CUSTOMER){
        CustomerNameEdit* edt = qobject_cast<CustomerNameEdit*>(editor);
        if(edt){
            SecondSubject* ssub = index.model()->data(index,Qt::EditRole).value<SecondSubject*>();
            if(ssub)
                edt->setSSub(ssub);
        }
    }
}

void InvoiceInputDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    int col = index.column();
    if(col == BaTemplateForm::CI_MONTH){
        QSpinBox* edt = qobject_cast<QSpinBox*>(editor);
        if(edt)
            model->setData(index,edt->value());
    }
    else if(col == BaTemplateForm::CI_INVOICE){
        InvoiceNumberEdit* edt = qobject_cast<InvoiceNumberEdit*>(editor);
        if(edt){
            model->setData(index,edt->text());
        }
    }
    else if(isDoubleColumn(col)){
        MoneyEdit* edt = qobject_cast<MoneyEdit*>(editor);
        if(edt){
            model->setData(index,edt->text(),Qt::DisplayRole);
        }
    }
    else if(col == BaTemplateForm::CI_CUSTOMER){
        CustomerNameEdit* edt = qobject_cast<CustomerNameEdit*>(editor);
        if(edt){
            if(edt->subject()){
                SecondSubject* ssub = edt->subject();
                QVariant v; v.setValue<SecondSubject*>(ssub);
                model->setData(index,v);
            }
        }
    }
}

void InvoiceInputDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    editor->setGeometry(rect);
}

void InvoiceInputDelegate::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
    if(index.column() != BaTemplateForm::CI_CUSTOMER)
        return QItemDelegate::destroyEditor(editor,index);
    if(canDestroy)
        editor->deleteLater();
}

void InvoiceInputDelegate::commitAndCloseEditor(int colIndex, bool isMove)
{
    QWidget* editor;
    if(colIndex == BaTemplateForm::CI_INVOICE)
        editor = qobject_cast<InvoiceNumberEdit*>(sender());
    else if(isDoubleColumn(colIndex))
        editor = qobject_cast<MoneyEdit*>(sender());
    else if(colIndex == BaTemplateForm::CI_CUSTOMER)
        editor = qobject_cast<CustomerNameEdit*>(sender());

    if(isMove){
        emit commitData(editor);
        emit closeEditor(editor, QAbstractItemDelegate::EditNextItem);
    }
    else{
        emit commitData(editor);
        emit closeEditor(editor);
    }
}

void InvoiceInputDelegate::multiInvoices(QStringList invoices)
{
    emit reqCrtMultiInvoices(invoices);
}

void InvoiceInputDelegate::newNameItemMapping(FirstSubject *fsub, SubjectNameItem *ni, SecondSubject *&ssub, int row, int col)
{
    canDestroy = false;
    emit crtNewNameItemMapping(row,col,fsub,ni,ssub);
}

void InvoiceInputDelegate::newSndSubject(FirstSubject *fsub, SecondSubject *&ssub, QString name, int row, int col)
{
    canDestroy = false;
    if(!curUser->haveRight(allRights.value(Right::Account_Config_SetSndSubject))){
        myHelper::ShowMessageBoxWarning(tr("您没有创建新二级科目的权限！"));
        return;
    }
    if(QMessageBox::information(0,"",tr("确定要用新的名称条目“%1”在一级科目“%2”下创建二级科目吗？")
                                .arg(name).arg(fsub->getName()),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            emit crtNewSndSubject(row,col,fsub,ssub,name);
}

/**
 * @brief 判断给定的列是否是要输入数据的列
 * @param col
 * @return
 */
bool InvoiceInputDelegate::isDoubleColumn(int col) const
{
    if(col == BaTemplateForm::CI_MONEY || col == BaTemplateForm::CI_TAXMONEY ||
            col == BaTemplateForm::CI_WMONEY)
        return true;
    return false;
}

//////////////////////////////////////////////////////////////////
BaTemplateForm::BaTemplateForm(AccountSuiteManager *suiterMgr, QWidget *parent) :
    QWidget(parent),ui(new Ui::BaTemplateForm),amgr(suiterMgr),bankFSub(0),
    ysFSub(0),yfFSub(0),bankSSub(0),curCusSSub(0),ok(false)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::Dialog);
    sm = amgr->getSubjectManager();
    pz = amgr->getCurPz();
    pzDlg = qobject_cast<PzDialog*>(parent);
    cw_invoice = 150;
    cw_money = 100;
    ui->cmbCustomer->setEditable(true);
    ui->cmbCustomer->setSubjectManager(sm);
    ui->cmbCustomer->setSubjectClass(SubjectSelectorComboBox::SC_SND);
    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setDecimals(2);
    dv->setBottom(0.01);
    ui->edtBankMoney->setValidator(dv);
    init();
}

BaTemplateForm::~BaTemplateForm()
{
    delete ui;
}

void BaTemplateForm::setTemplateType(BATemplateEnum type)
{
    switch (type) {
    case BATE_BANK_INCOME:
        ui->rdoBankIncome->setChecked(true);
        break;
    case BATE_YS_INCOME:
        ui->rdoYsIncome->setChecked(true);
        break;
    case BATE_BANK_COST:
        ui->rdoBankCost->setChecked(true);
        break;
    case BATE_YF_COST:
        ui->rdoYfCost->setChecked(true);
        break;
    case BATE_YS_GATHER:
        ui->rdoYsGather->setChecked(true);
        break;
    case BATE_YF_GATHER:
        ui->rdoYfGather->setChecked(true);
        break;
    default:
        break;
    }
    delegate->setTemplateType(type);
    if(type == BATE_YS_INCOME || type ==  BATE_YF_COST){
            ui->rdoSingle->setChecked(true);
            changeCustomerType(CT_SINGLE);
            ui->tw->setColumnHidden(CI_MONTH,false);
            ui->cmbCustomer->setFocus();
            ui->cmbBank->setEnabled(true);
            ui->edtBankMoney->setEnabled(true);
            ui->btnSave->setVisible(false);
            ui->btnLoad->setVisible(false);
    }
    else{
        if(type == BATE_BANK_INCOME || type == BATE_BANK_COST){
            ui->rdoSingle->setChecked(true);
            changeCustomerType(CT_SINGLE);
            ui->cmbCustomer->setFocus();
            ui->cmbBank->setEnabled(true);
            ui->edtBankMoney->setEnabled(true);
            ui->btnSave->setVisible(false);
            ui->btnLoad->setVisible(false);
        }
        else{
            ui->rdoMulti->setChecked(true);
            changeCustomerType(CT_MULTI);
            ui->cmbBank->setEnabled(false);
            ui->edtBankMoney->setEnabled(false);
            BaTemplateType type;
            if(ui->rdoYsGather->isChecked())
                type = BTT_YS_GATHER;
            else
                type = BTT_YF_GATHER;
            bool r = amgr->getAccount()->getDbUtil()->existBaTemlateDatas(type);
            ui->btnSave->setVisible(true);
            ui->btnLoad->setVisible(true);
            ui->btnLoad->setEnabled(r);
        }
        ui->tw->setColumnHidden(CI_MONTH,true);
    }
}

void BaTemplateForm::clear()
{
    if(ui->tw->rowCount() <= 1)
        return;
    int rows = ui->tw->rowCount()-1;
    for(int i = 0; i < rows; ++i){
        ui->tw->removeRow(0);
    }
    for(int i = CI_MONEY; i <= CI_CUSTOMER; ++i)
        ui->tw->item(0,i)->setText("");
    if(ui->rdoBankIncome->isChecked() || ui->rdoBankCost->isChecked())
        qDeleteAll(extraSSubs);
    extraSSubs.clear();
    qDeleteAll(buffers);
    buffers.clear();
    ui->cmbBank->setCurrentIndex(-1);
    ui->edtBankMoney->clear();
    ui->edtBankAccount->clear();
    ui->cmbCustomer->setCurrentIndex(-1);
    ok = false;
}

void BaTemplateForm::moneyTypeChanged(int index)
{
    if(index < 0 || index >= ui->cmbBank->count())
        return;
    bankSSub = ui->cmbBank->itemData(ui->cmbBank->currentIndex()).value<SecondSubject*>();
    mt = sm->getBankAccount(bankSSub)->mt;
    ui->edtBankAccount->setText(sm->getBankAccount(bankSSub)->accNumber);
}


void BaTemplateForm::customerChanged(int index)
{
    if(index == -1){
        ui->edtName->clear();
        curCusSSub = 0;
        return;
    }    
    QString name = ui->cmbCustomer->currentText();
    curCusSSub = ui->cmbCustomer->itemData(index).value<SecondSubject*>();    
    if(!curCusSSub){
        if(ui->rdoYsIncome->isChecked() || ui->rdoYfCost->isChecked()){
            myHelper::ShowMessageBoxWarning(tr("该客户不存在！"));
            ui->cmbCustomer->setCurrentIndex(-1);
            return;
        }
        //查看临时科目中有没有
        if(!extraSSubs.isEmpty()){
            for(int i = 0; i < extraSSubs.count(); ++i){
                if(extraSSubs.at(i)->getName() == name){
                    curCusSSub = extraSSubs.at(i);
                    return;
                }
            }
        }
        FirstSubject* fsub = 0;
        if(ui->rdoBankIncome->isChecked())
            fsub = ysFSub;
        else if(ui->rdoBankCost->isChecked())
            fsub = yfFSub;
        if(!fsub)
            return;
        SubjectNameItem* ni = sm->getNameItem(name);
        if(ni){
            if(QMessageBox::No == myHelper::ShowMessageBoxQuesion(tr("“%1”科目不存在，要在“%2”下创建临时二级科目吗？")
                                                                  .arg(name).arg(fsub->getName())))
                return;
        }
        else{
            if(QMessageBox::No == myHelper::ShowMessageBoxQuesion(tr("“%1”科目不存在，要创建临时客户名并在“%2”下创建对应临时二级科目吗？")
                                                                  .arg(name).arg(fsub->getName())))
                return;
            ni = new SubjectNameItem(0,1,name,name,"",QDateTime(),curUser);
        }
        curCusSSub = new SecondSubject(0,0,ni,"",0,true,QDateTime(),QDateTime(),curUser);
        extraSSubs<<curCusSSub;
        //ui->cmbCustomer->addTemSndSub(curCusSSub);
    }
    ui->edtName->setText(curCusSSub->getLName());
}

/**
 * @brief 监视输入发票或金额的变化，以实时计算合计金额，如果是应收应付，
 * 则实时从缓存中读取发票各种金额信息
 * @param item
 */
void BaTemplateForm::DataChanged(QTableWidgetItem *item)
{
    int row = item->row();
    if(row == ui->tw->rowCount()-1)
        return;
    int col = item->column();
    if(col >= CI_MONEY && col <= CI_WMONEY)
        reCalSum((ColumnIndex)col);
    else if(col == CI_INVOICE && (ui->rdoYsIncome->isChecked() || ui->rdoYfCost->isChecked())){
        if(!invoiceQualified(item->text()))
            return;
        autoSetYsYf(row,item->text());
    }
}

/**
 *
 */
void BaTemplateForm::addNewInvoice()
{
    int row = ui->tw->rowCount()-1;
    turnDataInspect(false);
    ui->tw->insertRow(row);
    initRow(row);
    ui->tw->selectRow(row);
    ui->tw->edit(ui->tw->model()->index(row,CI_INVOICE));
    turnDataInspect();
}

/**
 * @brief 在当前行插入多个发票行
 * @param invoices
 */
void BaTemplateForm::createMultiInvoice(QStringList invoices)
{
    int row = ui->tw->currentRow();
    turnDataInspect(false);
    for(int i = 1; i < invoices.count(); ++i){
        ui->tw->insertRow(row+i);
        ui->tw->setItem(row+i,CI_MONTH,new QTableWidgetItem);
        ui->tw->setItem(row+i,CI_INVOICE,new QTableWidgetItem(invoices.at(i)));
        ui->tw->setItem(row+i,CI_MONEY,new QTableWidgetItem);
        ui->tw->setItem(row+i,CI_TAXMONEY,new QTableWidgetItem);
        ui->tw->setItem(row+i,CI_WMONEY,new QTableWidgetItem);        
    }
    if(ui->rdoYsGather->isChecked() || ui->rdoYfGather->isChecked()){
        for(int i = 1; i < invoices.count(); ++i)
            ui->tw->setItem(row+i,CI_CUSTOMER,new BASndSubItem_new(0,sm));
    }
    //如果是应收应付，则自动搜索发票对应的金额信息
    if(ui->rdoYsIncome->isChecked() || ui->rdoYfCost->isChecked()){
        for(int i = row; i < row+invoices.count(); ++i)
            autoSetYsYf(i,ui->tw->item(i,CI_INVOICE)->text());
    }
    reCalAllSum();
    turnDataInspect();
}

void BaTemplateForm::creatNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject *&ssub)
{
    if(!curUser->haveRight(allRights.value(Right::Account_Config_SetSndSubject))){
        myHelper::ShowMessageBoxWarning(tr("您没有创建新二级科目的权限！"));
        delegate->userConfirmed();
        return;
    }
    if(QMessageBox::information(0,"",tr("确定要使用已有的名称条目“%1”在一级科目“%2”下创建二级科目吗？")
                                .arg(ni->getShortName()).arg(fsub->getName()),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::No){
        delegate->userConfirmed();
        return;
    }
    ssub = new SecondSubject(fsub,0,ni,"",1,true,QDateTime::currentDateTime(),QDateTime(),curUser);
    extraSSubs<<ssub;
    QVariant v;
    v.setValue<SecondSubject*>(ssub);
    ui->tw->item(row,col)->setData(Qt::EditRole,v);
    delegate->userConfirmed();
}

void BaTemplateForm::creatNewSndSubject(int row, int col, FirstSubject *fsub, SecondSubject *&ssub, QString name)
{    
    CompletSubInfoDialog dlg(0,sm,0);
    dlg.setName(name);
    if(QDialog::Accepted == dlg.exec()){
        SubjectNameItem* ni = new SubjectNameItem(0,dlg.getSubCalss(),dlg.getSName(),
                                                  dlg.getLName(),dlg.getRemCode(),QDateTime::currentDateTime(),curUser);
        ssub = new SecondSubject(fsub,0,ni,"",1,true,QDateTime::currentDateTime(),QDateTime(),curUser);
        extraSSubs<<ssub;
        QVariant v;
        v.setValue<SecondSubject*>(ssub);
        ui->tw->item(row,col)->setData(Qt::EditRole,v);
    }
    delegate->userConfirmed();
}

/**
 * @brief 当双击金额合计栏（账面金额或外币金额）时，自动将当前金额填写到银行金额域内
 * @param index
 */
void BaTemplateForm::doubleClickedCell(const QModelIndex &index)
{
    if(ui->rdoYsGather->isChecked() || ui->rdoYfGather->isChecked() || ui->tw->rowCount()==1)
        return;
    int row = index.row();
    if(row != ui->tw->rowCount()-1)
        return;
    int col = index.column();
    ui->edtBankMoney->setText(ui->tw->item(row,col)->text());
    QList<SecondSubject*> bankSubs;
    Money* mt=0;
    if(col == CI_MONEY)
        mt = mmt;
    else if(col == CI_WMONEY)
        mt = wmt;
    for(int i = 0; i < bankFSub->getChildCount(); ++i){
        SecondSubject* ssub = bankFSub->getChildSub(i);
        if(ssub->isEnabled() && sm->getSubMatchMt(ssub) == mt)
            bankSubs<<ssub;
    }
    if(bankSubs.isEmpty())
        return;
    SecondSubject* bsub = bankSubs.first();
    if(bankSubs.count() > 1){
        QDialog dlg(this);
        QLabel t(tr("选择银行账户"),this);
        QTableWidget tw(this);
        tw.horizontalHeader()->setStretchLastSection(true);
        tw.setSelectionBehavior(QAbstractItemView::SelectRows);
        tw.setColumnCount(2);
        QStringList titles;
        titles<<tr("名称")<<tr("帐号");
        tw.setHorizontalHeaderLabels(titles);
        for(int i = 0; i < bankSubs.count(); ++i){
            tw.insertRow(i);
            SecondSubject* ssub = bankSubs.at(i);
            tw.setItem(i,0,new QTableWidgetItem(ssub->getName()));
            tw.setItem(i,1,new QTableWidgetItem(sm->getBankAccount(ssub)->accNumber));
        }
        tw.setCurrentCell(0,0);
        QPushButton btnOk(tr("确定"),this);
        connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
        QVBoxLayout* lo = new QVBoxLayout;
        lo->addWidget(&t);
        lo->addWidget(&tw);
        lo->addWidget(&btnOk);
        dlg.setLayout(lo);
        dlg.resize(300,200);
        dlg.exec();
        int row = tw.currentRow();
        bsub = bankSubs.at(row);
    }
    for(int i = 0; i < ui->cmbBank->count(); ++i){
        SecondSubject* ssub = ui->cmbBank->itemData(i).value<SecondSubject*>();
        if(ssub == bsub){
            ui->cmbBank->setCurrentIndex(i);
            return;
        }
    }
}

void BaTemplateForm::contextMenuRequested(const QPoint &pos)
{
    QTableWidgetItem* item = ui->tw->itemAt(pos);
    if(!item || item->row() == ui->tw->rowCount()-1)
        return;
    QMenu m;
    m.addAction(ui->actCopy);
    m.addAction(ui->actCut);
    QList<int> rows = selectedRows();
    if(!buffers.isEmpty()){
        m.addAction(ui->actPaste);
        if(rows.count() != buffers.count())
            ui->actPaste->setEnabled(false);
        else{
            if(rows.count() == 1)
                ui->actPaste->setEnabled(true);
            else{
                bool rowContinue = true;
                for(int i = 0; i < rows.count()-1; ++i){
                    if(rows.at(i)+1 != rows.at(i+1)){
                        rowContinue = false;
                        break;
                    }
                }
                ui->actPaste->setEnabled(rowContinue);
            }
        }
    }
    if(rows.count() == 1)
        m.addAction(ui->actInsert);
    m.addAction(ui->actRemove);
    if(ui->rdoYsGather->isChecked() || ui->rdoYfGather->isChecked()){
        m.addAction(ui->actMergerCustomer);
        ui->actMergerCustomer->setEnabled(rows.count()>1);
    }
    m.exec(ui->tw->mapToGlobal(pos));
}

void BaTemplateForm::processContextMenu()
{
    QAction* a = qobject_cast<QAction*>(sender());
    if(!a)
        return;
    if(a == ui->actCopy){
        qDeleteAll(buffers);
        buffers.clear();
        foreach (int row, selectedRows())
            copyRow(row);
    }
    else if(a == ui->actCut){
        qDeleteAll(buffers);
        buffers.clear();
        QList<int> rows = selectedRows();
        foreach (int row, rows)
            copyRow(row);
        turnDataInspect(false);
        for(int i = rows.count() -1; i >= 0; i--)
            ui->tw->removeRow(rows.at(i));
        turnDataInspect();
        reCalAllSum();
    }else if(a == ui->actInsert){
        int row = ui->tw->currentRow();
        ui->tw->insertRow(row);
        initRow(row);
    }else if(a == ui->actPaste){
        QList<int> rows = selectedRows();
        int row = rows.first();
        bool isCustomer = false;
        if(ui->rdoYsGather->isChecked() || ui->rdoYfGather->isChecked())
            isCustomer = true;
        turnDataInspect(false);
        for(int i = 0; i < buffers.count(); ++i){
            //ui->tw->insertRow(row+i);
            //initRow(row+i);
            QString ms;
            if(buffers.at(i)->month != 0)
                QString::number(buffers.at(i)->month);
            ui->tw->item(row+i,CI_MONTH)->setText(ms);
            ui->tw->item(row+i,CI_INVOICE)->setText(buffers.at(i)->inum);
            ui->tw->item(row+i,CI_MONEY)->setText(buffers.at(i)->money.toString());
            ui->tw->item(row+i,CI_TAXMONEY)->setText(buffers.at(i)->taxMoney.toString());
            ui->tw->item(row+i,CI_WMONEY)->setText(buffers.at(i)->wMoney.toString());
            if(isCustomer){
                QVariant v; v.setValue<SecondSubject*>(buffers.at(i)->ssub);
                ui->tw->item(row+i,CI_CUSTOMER)->setData(Qt::EditRole,v);
            }
        }
        turnDataInspect();
        reCalAllSum();
    }else if(a == ui->actRemove){
        QList<int> rows = selectedRows();
        turnDataInspect(false);
        for(int i = rows.count()-1; i >= 0; i--)
            ui->tw->removeRow(rows.at(i));
        turnDataInspect();
    }
    else if(a == ui->actMergerCustomer){
        QList<int> rows = selectedRows();
        QList<SecondSubject*> ssubs;
        for(int i = 0; i < rows.count(); ++i){
            SecondSubject* ssub = ui->tw->item(rows.at(i),CI_CUSTOMER)->data(Qt::EditRole).value<SecondSubject*>();
            if(!ssub)
                continue;
            if(ssubs.contains(ssub))
                continue;
            ssubs<<ssub;
        }
        if(ssubs.isEmpty()){
            myHelper::ShowMessageBoxWarning(tr("请至少在一行中设置归并的客户名！"));
            return;
        }
        SecondSubject* ssub = 0;
        if(ssubs.count() > 1){
            QDialog d;
            QLabel info(tr("所选行包含多个不同的客户，请选择一个正确的归并客户"),&d);
            QListWidget lw(&d);
            for(int i = 0; i < ssubs.count(); ++i){
                QListWidgetItem* item = new QListWidgetItem(ssubs.at(i)->getName(),&lw);
                QVariant v; v.setValue<SecondSubject*>(ssubs.at(i));
                item->setData(Qt::UserRole,v);
            }
            lw.setCurrentRow(0);
            QPushButton btnOk(tr("确定"),&d);
            QPushButton btnCancel(tr("取消"),&d);
            connect(&btnOk,SIGNAL(clicked()),&d,SLOT(accept()));
            connect(&btnCancel,SIGNAL(clicked()),&d,SLOT(reject()));
            QHBoxLayout lb;
            lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
            QVBoxLayout* lm = new QVBoxLayout;
            lm->addWidget(&info);
            lm->addWidget(&lw);
            lm->addLayout(&lb);
            d.setLayout(lm);
            d.resize(200,300);
            if(QDialog::Rejected == d.exec())
                return;
            ssub = lw.currentItem()->data(Qt::UserRole).value<SecondSubject*>();
        }
        else
            ssub = ssubs.first();
        for(int i = 0; i < rows.count(); ++i){
            QVariant v;
            v.setValue<SecondSubject*>(ssub);
            ui->tw->item(rows.at(i),CI_CUSTOMER)->setData(Qt::EditRole,v);
        }
    }
}



void BaTemplateForm::init()
{
    mmt = amgr->getAccount()->getMasterMt();
    wmt = amgr->getAccount()->getAllMoneys().value(USD);
    yfFSub = sm->getYfSub();
    ysFSub = sm->getYsSub();
    bankFSub = sm->getBankSub();
    if(!ysFSub || !yfFSub || !bankFSub){
        myHelper::ShowMessageBoxWarning(tr("应收、应付或银行等特定科目未做配置，无法正常工作！"));
        return;
    }
    srFSub = sm->getZysrSub();
    cbFSub = sm->getZycbSub();
    sjFSub = sm->getYjsjSub();
    cwFSub = sm->getCwfySub();
    if(!srFSub || !cbFSub || !sjFSub || !cwFSub){
        myHelper::ShowMessageBoxWarning(tr("主营业务收入、成本、应交税金或财务费用等特定科目未做配置，无法正常工作！"));
        return;
    }
    xxSSub = sm->getXxseSSub();
    jxSSub = sm->getJxseSSub();
    if(!xxSSub || !jxSSub){
        myHelper::ShowMessageBoxWarning(tr("%1下的销项税额或进行税额等特定子目未做配置，无法正常工作！").arg(sjFSub->getName()));
        return;
    }
    hdsySSub = cwFSub->getDefaultSubject();
    srDefSSub = srFSub->getDefaultSubject();
    cbDefSSub = cbFSub->getDefaultSubject();
    if(!hdsySSub || !srDefSSub || !cbDefSSub){
        myHelper::ShowMessageBoxWarning(tr("%1、%2或%3下未配置默认子目，它们应该分别对应汇兑损益、包干费等或代理运费等！")
                                        .arg(cwFSub->getName()).arg(srFSub->getName()).arg(cbFSub->getName()));
        return;
    }
    sxfSSub = sm->getSxfSSub();
    if(!sxfSSub){
        myHelper::ShowMessageBoxWarning(tr("财务费用科目下不能找到“金融机构手续费等”子目，无法正常工作！"));
        return;
    }
    delegate = new InvoiceInputDelegate(sm,this);
    delegate->setTemplateType(BATE_BANK_INCOME);
    connect(delegate,SIGNAL(reqCrtMultiInvoices(QStringList)),SLOT(createMultiInvoice(QStringList)));
    connect(delegate,SIGNAL(crtNewNameItemMapping(int,int,FirstSubject*,SubjectNameItem*,SecondSubject*&)),
            this,SLOT(creatNewNameItemMapping(int,int,FirstSubject*,SubjectNameItem*,SecondSubject*&)));
    connect(delegate,SIGNAL(crtNewSndSubject(int,int,FirstSubject*,SecondSubject*&,QString)),
            this,SLOT(creatNewSndSubject(int,int,FirstSubject*,SecondSubject*&,QString)));
    ui->tw->setItemDelegate(delegate);
    btnAdd = new QPushButton(tr("新增"),this);
    connect(btnAdd,SIGNAL(clicked()),this,SLOT(addNewInvoice()));
    ui->tw->setRowCount(1);
    ui->tw->setCellWidget(0,CI_INVOICE,btnAdd);
    ui->tw->setItem(0,CI_MONEY,new QTableWidgetItem);
    ui->tw->setItem(0,CI_TAXMONEY,new QTableWidgetItem);
    ui->tw->setItem(0,CI_WMONEY,new QTableWidgetItem);
    ui->tw->setItem(0,CI_CUSTOMER,new QTableWidgetItem);

    //初始化银行客户下拉列表
    for(int i = 0; i < bankFSub->getChildCount(); ++i){
        SecondSubject* ssub = bankFSub->getChildSub(i);
        if(!ssub->isEnabled())
            continue;
        QVariant v; v.setValue<SecondSubject*>(ssub);
        ui->cmbBank->addItem(ssub->getName(),v);
    }
    bankSSub = ui->cmbBank->itemData(ui->cmbBank->currentIndex()).value<SecondSubject*>();
    ui->edtBankAccount->setText(sm->getBankAccount(bankSSub)->accNumber);
    mt = sm->getBankAccount(bankSSub)->mt;
    connect(ui->cmbBank,SIGNAL(currentIndexChanged(int)),this,SLOT(moneyTypeChanged(int)));
    //初始化客户下拉列表
    connect(ui->cmbCustomer,SIGNAL(currentIndexChanged(int)),this,SLOT(customerChanged(int)));
    changeCustomerType(CT_SINGLE);
    delegate->setvalidColumns(4);

    ui->tw->setColumnWidth(CI_MONTH,50);
    ui->tw->setColumnWidth(CI_INVOICE,cw_invoice);
    ui->tw->setColumnWidth(CI_MONEY,cw_money);
    ui->tw->setColumnWidth(CI_TAXMONEY,cw_money);
    ui->tw->setColumnWidth(CI_WMONEY,cw_money);
    turnDataInspect();
    connect(ui->tw,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(doubleClickedCell(QModelIndex)));
    connect(ui->tw,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenuRequested(QPoint)));
    connect(ui->actCopy,SIGNAL(triggered()),this,SLOT(processContextMenu()));
    connect(ui->actCut,SIGNAL(triggered()),this,SLOT(processContextMenu()));
    connect(ui->actInsert,SIGNAL(triggered()),this,SLOT(processContextMenu()));
    connect(ui->actPaste,SIGNAL(triggered()),this,SLOT(processContextMenu()));
    connect(ui->actMergerCustomer,SIGNAL(triggered()),this,SLOT(processContextMenu()));
    connect(ui->actRemove,SIGNAL(triggered()),this,SLOT(processContextMenu()));
    ui->btnSave->setVisible(false);
    ui->btnLoad->setVisible(false);
}

void BaTemplateForm::initRow(int row)
{
    ui->tw->setItem(row,CI_MONTH,new QTableWidgetItem);
    ui->tw->setItem(row,CI_INVOICE,new QTableWidgetItem);
    ui->tw->setItem(row,CI_MONEY,new QTableWidgetItem);
    ui->tw->setItem(row,CI_TAXMONEY,new QTableWidgetItem);
    ui->tw->setItem(row,CI_WMONEY,new QTableWidgetItem);
    ui->tw->setItem(row,CI_CUSTOMER,new BASndSubItem_new(0,sm));
    ui->tw->selectRow(row);
}

/**
 * @brief 改变客户类型（一对一或一对多）
 * @param type
 */
void BaTemplateForm::changeCustomerType(BaTemplateForm::CustomerType type)
{
    if(type == CT_SINGLE){
        ui->tw->setColumnHidden(CI_CUSTOMER,true);
        ui->cmbCustomer->setEnabled(true);
        ui->cmbCustomer->clear();
        FirstSubject* fsub = 0;
        if(ui->rdoYsIncome->isChecked() || ui->rdoBankIncome->isChecked()){
            fsub = ysFSub;
        }
        else if(ui->rdoYfCost->isChecked() || ui->rdoBankCost->isChecked()){
            fsub = yfFSub;
        }
        ui->cmbCustomer->setParentSubject(fsub);
        ui->cmbCustomer->setCurrentIndex(-1);
    }
    else{
        ui->tw->setColumnHidden(CI_CUSTOMER,false);
        ui->tw->setColumnWidth(CI_WMONEY, cw_money);
        ui->cmbCustomer->setEnabled(false);
    }

}

/**
 * @brief 创建当月收入分录
 * @return
 */
void BaTemplateForm::createBankIncomeBas()
{
    if(!ui->rdoBankIncome->isChecked())
        return;
    if(ui->tw->rowCount()==1)
        return;
    if(!curCusSSub){
        myHelper::ShowMessageBoxWarning(tr("拜托，收了谁的钱啊？"));
        return;
    }
    if(ui->edtBankMoney->text().isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("拜托，银行收了多少钱啊？"));
        return;
    }
    int row = invoiceQualifieds();
    if(row != -1)
        return;
    QString dupliNum =  dupliInvoice();
    if(!dupliNum.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("发票号“%1”有重复！").arg(dupliNum));
        return;
    }
    QList<BusiAction*> bas;
    Double bv = Double(ui->edtBankMoney->text().toDouble());
    Double sumZm = ui->tw->item(ui->tw->rowCount()-1,CI_MONEY)->data(Qt::DisplayRole).toString().toDouble();
    Double sumWm = ui->tw->item(ui->tw->rowCount()-1,CI_WMONEY)->data(Qt::DisplayRole).toString().toDouble();
    Double diff;
    if(mt == mmt){
        diff = sumZm - bv;
        if(diff != 0 && sumWm == 0){
            diff = 0.0;
            myHelper::ShowMessageBoxWarning(tr("发票合计金额与银行所收金额不相等！"));
        }
    }
    else{
        if(sumWm != bv){
            diff = sumWm - bv;
            QDialog dlg(this);
            QLabel l(tr("%1差额：").arg(wmt->name()),this);
            QDoubleSpinBox vBox(this);
            vBox.setMinimum(0);
            vBox.setMaximum(diff.getv());
            vBox.setValue(diff.getv());
            QHBoxLayout lh;
            lh.addWidget(&l);
            lh.addWidget(&vBox);
            QPushButton btnOk(tr("确定"),this);
            QPushButton btnCancel(tr("取消"),this);
            connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
            connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
            QHBoxLayout lb;
            lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
            QLabel t(tr("是否将%1差额作手续费").arg(wmt->name()));
            QVBoxLayout* lm = new QVBoxLayout;
            lm->addWidget(&t);
            lm->addLayout(&lh);lm->addLayout(&lb);
            dlg.setLayout(lm);
            dlg.resize(200,100);
            QHash<int,Double> rates;
            amgr->getRates(rates);
            if(dlg.exec() == QDialog::Accepted){
                diff = vBox.value();
                QString s = tr("手续费（%1%2）").arg(wmt->simpleSign()).arg(diff.toString());
                diff = diff * rates.value(wmt->code());
                bas<<new BusiAction(0,pz,s,cwFSub,sxfSSub,mmt,MDIR_J,diff,0);
                diff = sumZm - (bv*rates.value(wmt->code()) + diff);
            }
            else
                diff = sumZm - bv*rates.value(wmt->code());
        }
    }
    if(diff != 0)
        bas<<new BusiAction(0,pz,tr("汇兑损益"),cwFSub,hdsySSub,mmt,MDIR_J,diff,0);
    QString summary = QString("收%1运费").arg(curCusSSub->getName());
    bas<<new BusiAction(0,pz,summary,bankFSub,bankSSub,mt,MDIR_J,bv,0);
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        QString invoice = ui->tw->item(i,CI_INVOICE)->text();
        Double zmMoney = ui->tw->item(i,CI_MONEY)->text().toDouble();
        Double taxMoney = ui->tw->item(i,CI_TAXMONEY)->text().toDouble();
        Double srMoney;
        if(taxMoney == 0)
            srMoney = zmMoney;
        else
            srMoney = zmMoney - taxMoney;
        summary = tr("收%1运费 %2").arg(curCusSSub->getName()).arg(invoice);
        bas<<new BusiAction(0,pz,summary,srFSub,srDefSSub,mmt,MDIR_D,srMoney,0);
        if(taxMoney != 0)
            bas<<new BusiAction(0,pz,summary,sjFSub,xxSSub,mmt,MDIR_D,taxMoney,0);
    }
    pzDlg->insertBas(bas);
    ok = true;
}

/**
 * @brief 创建当月成本分录
 */
void BaTemplateForm::createBankCostBas()
{
    if(!ui->rdoBankCost->isChecked())
        return;
    if(ui->tw->rowCount()==1)
        return;
    if(!curCusSSub){
        myHelper::ShowMessageBoxWarning(tr("拜托，钱付给谁了呀？"));
        return;
    }
    if(ui->edtBankMoney->text().isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("拜托，银行付了多少钱呀？"));
        return;
    }
    int row = invoiceQualifieds();
    if(row != -1)
        return;
    QString dupliNum =  dupliInvoice();
    if(!dupliNum.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("发票号“%1”有重复！").arg(dupliNum));
        return;
    }
    Double bv = Double(ui->edtBankMoney->text().toDouble());
    Double sumZm = ui->tw->item(ui->tw->rowCount()-1,CI_MONEY)->data(Qt::DisplayRole).toString().toDouble();
    Double sumWm = ui->tw->item(ui->tw->rowCount()-1,CI_WMONEY)->data(Qt::DisplayRole).toString().toDouble();
    QList<BusiAction*> bas;
    Double diff;
    if(mt == mmt){
        diff = bv - sumZm;
        if(sumWm == 0 && diff != 0){
            diff = 0;
            myHelper::ShowMessageBoxWarning(tr("发票合计金额与银行所付金额不相等！"));
        }
    }
    else{
        QHash<int,Double> rates;
        amgr->getRates(rates);
        diff = bv * rates.value(wmt->code()) - sumZm;
    }
    if(diff != 0)
        bas<<new BusiAction(0,pz,tr("汇兑损益"),cwFSub,hdsySSub,mmt,MDIR_J,diff,0);
    QString summary;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        QString invoice = ui->tw->item(i,CI_INVOICE)->text();
        Double zmMoney = ui->tw->item(i,CI_MONEY)->text().toDouble();
        Double taxMoney = ui->tw->item(i,CI_TAXMONEY)->text().toDouble();
        Double cbMoney;
        if(taxMoney == 0)
            cbMoney = zmMoney;
        else
            cbMoney = zmMoney - taxMoney;
        summary = tr("付%1运费 %2").arg(curCusSSub->getName()).arg(invoice);
        bas<<new BusiAction(0,pz,summary,cbFSub,cbDefSSub,mmt,MDIR_J,cbMoney,0);
        if(taxMoney != 0)
            bas<<new BusiAction(0,pz,summary,sjFSub,jxSSub,mmt,MDIR_J,taxMoney,0);
    }
    summary = QString("付%1运费").arg(curCusSSub->getName());
    bas<<new BusiAction(0,pz,summary,bankFSub,bankSSub,mt,MDIR_D,bv,0);
    pzDlg->insertBas(bas);
    ok = true;
}

/**
 * @brief 创建银收-应收分录
 */
void BaTemplateForm::createYsBas()
{
    if(!ui->rdoYsIncome->isChecked())
        return;
    if(ui->tw->rowCount()==1)
        return;
    if(!curCusSSub){
        myHelper::ShowMessageBoxWarning(tr("拜托，收了谁的钱啊？"));
        return;
    }
    if(ui->edtBankMoney->text().isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("拜托，银行收了多少钱啊？"));
        return;
    }
    int row = invoiceQualifieds();
    if(row != -1)
        return;
    QString dupliNum =  dupliInvoice();
    if(!dupliNum.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("发票号“%1”有重复！").arg(dupliNum));
        return;
    }

    //生成发票号的简写形式
    QHash<int,QStringList> invoices;
    bool exist = false;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        QString inum = ui->tw->item(i,CI_INVOICE)->text();
        if(!invoiceQualified(inum)){
            myHelper::ShowMessageBoxWarning(tr("第%1行发票号无效！").arg(i+1));
            return;
        }
        int month = ui->tw->item(i,CI_MONTH)->text().toInt();
        if(!invoices.contains(month))
            invoices[month] = QStringList();
        if(!exist){
            Double money = ui->tw->item(i,CI_MONEY)->text().toDouble();
            if(money == 0){
                exist = true;
                myHelper::ShowMessageBoxWarning(tr("存在未经确认的应收项！"));
            }
        }        
        invoices[month]<<inum;
    }
    QList<int> months;
    QList<QStringList> inums;
    months = invoices.keys();
    qSort(months.begin(),months.end());
    for(int i = 0; i < months.count(); ++i)
        inums<<invoices.value(months.at(i));
    QString compactNums = PaUtils::terseInvoiceNumsWithMonth(months,inums);

    Double bv = ui->edtBankMoney->text().toDouble();
    int rows = ui->tw->rowCount()-1;
    Double sumZm = ui->tw->item(rows,CI_MONEY)->text().toDouble();//应收合计金额
    Double sumWb = ui->tw->item(rows,CI_WMONEY)->text().toDouble();//应收美金合计
    QList<BusiAction*> bas;
    //如果金额不符，则考虑加入汇兑损益或手续费之类的分录
    Double diff;
    if(mt == mmt){
        diff = sumZm - bv;
        if(diff != 0 && sumWb == 0){
            diff = 0;
            myHelper::ShowMessageBoxWarning(tr("发票合计金额与银行所收金额不相等！"));
        }
    }
    else if(mt == wmt){
        QHash<int,Double> rates;
        amgr->getRates(rates);
        diff = sumWb - bv;
        if(diff != 0){
            QDialog dlg(this);
            QLabel l(tr("%1差额：").arg(wmt->name()),this);
            QDoubleSpinBox vBox(this);
            vBox.setMinimum(0);
            vBox.setMaximum(diff.getv());
            vBox.setValue(diff.getv());
            QHBoxLayout lh;
            lh.addWidget(&l);
            lh.addWidget(&vBox);
            QPushButton btnOk(tr("确定"),this);
            QPushButton btnCancel(tr("取消"),this);
            connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
            connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
            QHBoxLayout lb;
            lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
            QLabel t(tr("是否将%1差额作手续费").arg(wmt->name()));
            QVBoxLayout* lm = new QVBoxLayout;
            lm->addWidget(&t);
            lm->addLayout(&lh);lm->addLayout(&lb);
            dlg.setLayout(lm);
            dlg.resize(200,100);
            if(dlg.exec() == QDialog::Accepted){
                diff = vBox.value();
                QString s = tr("手续费（%1%2）").arg(wmt->simpleSign()).arg(diff.toString());
                diff = diff * rates.value(wmt->code());
                bas<<new BusiAction(0,pz,s,cwFSub,sxfSSub,mmt,MDIR_J,diff,0);
                diff = sumZm - (bv*rates.value(wmt->code()) + diff);
            }
            else
                diff = sumZm - bv*rates.value(wmt->code());
        }
        else{
            diff = sumZm - bv*rates.value(wmt->code());
        }
    }
    if(diff != 0)
        bas<<new BusiAction(0,pz,tr("汇兑损益"),cwFSub,hdsySSub,mmt,MDIR_J,diff,0);

    QString summary = tr("收%1运费 %2").arg(curCusSSub->getName()).arg(compactNums);
    bas<<new BusiAction(0,pz,summary,bankFSub,bankSSub,mt,MDIR_J,bv,0);
    bas<<new BusiAction(0,pz,summary,ysFSub,curCusSSub,mmt,MDIR_D,sumZm,0);
    pzDlg->insertBas(bas);
    ok = true;
}


/**
 * @brief 创建银付-应付分录
 */
void BaTemplateForm::createYfBas()
{
    if(!ui->rdoYfCost->isChecked())
        return;
    if(ui->tw->rowCount()==1)
        return;
    if(!curCusSSub){
        myHelper::ShowMessageBoxWarning(tr("拜托，钱付给谁了呀？"));
        return;
    }
    if(ui->edtBankMoney->text().isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("拜托，从银行付了多少钱呀？"));
        return;
    }    
    int row = invoiceQualifieds();
    if(row != -1)
        return;
    QString dupliNum =  dupliInvoice();
    if(!dupliNum.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("发票号“%1”有重复！").arg(dupliNum));
        return;
    }

    //生成发票号的简写形式
    QHash<int,QStringList> invoices;
    bool exist = false;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        QString inum = ui->tw->item(i,CI_INVOICE)->text();
        if(!invoiceQualified(inum)){
            myHelper::ShowMessageBoxWarning(tr("第%1行发票号无效！").arg(i+1));
            return;
        }
        int month = ui->tw->item(i,CI_MONTH)->text().toInt();
        if(!invoices.contains(month))
            invoices[month] = QStringList();
        if(!exist){
            Double money = ui->tw->item(i,CI_MONEY)->text().toDouble();
            if(money == 0){
                exist = true;
                myHelper::ShowMessageBoxWarning(tr("存在未经确认的应付项！"));
            }
        }
        invoices[month]<<inum;
    }
    QList<int> months;
    QList<QStringList> inums;
    months = invoices.keys();
    qSort(months.begin(),months.end());
    for(int i = 0; i < months.count(); ++i)
        inums<<invoices.value(months.at(i));
    QString compactNums = PaUtils::terseInvoiceNumsWithMonth(months,inums);

    QList<BusiAction*> bas;
    Double bv = ui->edtBankMoney->text().toDouble();
    int rows = ui->tw->rowCount()-1;
    Double sumZm = ui->tw->item(rows,CI_MONEY)->text().toDouble();//应付合计金额
    Double sumWb = ui->tw->item(rows,CI_WMONEY)->text().toDouble();//应付美金合计
    Double diff;
    if(mt == mmt){
        diff = bv - sumZm; //用本币折换外币的差额
        if(diff != 0 && sumWb == 0){
            diff = 0;
            myHelper::ShowMessageBoxWarning(tr("发票合计金额与银行所付金额不相等！"));
        }
    }
    else{
        QHash<int,Double> rates;
        amgr->getRates(rates);
        diff = bv * rates.value(wmt->code()) - sumZm;
    }
    if(diff != 0)
        bas<<new BusiAction(0,pz,tr("汇兑损益"),cwFSub,hdsySSub,mmt,MDIR_J,diff,0);
    QString summary = tr("付%1运费 %2").arg(curCusSSub->getName()).arg(compactNums);
    bas<<new BusiAction(0,pz,summary,yfFSub,curCusSSub,mmt,MDIR_J,sumZm,0);
    bas<<new BusiAction(0,pz,summary,bankFSub,bankSSub,mt,MDIR_D,bv,0);
    pzDlg->insertBas(bas);
    ok = true;
}

/**
 * @brief 创建应收聚合分录
 */
void BaTemplateForm::createYsGatherBas()
{
    if(!ui->rdoYsGather->isChecked())
        return;
    if(ui->tw->rowCount()==1)
        return;
    //检测遗漏项（发票号、账面金额、客户）
    QStringList invoices;
    QList<Double> zmMoneys;
    QList<SecondSubject*> ssubs;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        QString invoice = ui->tw->item(i,CI_INVOICE)->text();
        if(invoice.isEmpty() || !invoiceQualified(invoice)){
            myHelper::ShowMessageBoxWarning(tr("第%1行没有输入发票号或无效！").arg(i+1));
            return;
        }
        invoices<<invoice;
        Double zmMoney = ui->tw->item(i,CI_MONEY)->text().toDouble();
        if(zmMoney == 0){
            myHelper::ShowMessageBoxWarning(tr("第%1行没有输入发票账面金额！").arg(i+1));
            return;
        }
        zmMoneys<<zmMoney;
        SecondSubject* ssub = ui->tw->item(i,CI_CUSTOMER)->data(Qt::EditRole).value<SecondSubject*>();
        if(!ssub){
            myHelper::ShowMessageBoxWarning(tr("第%1行（%2）没有设置客户！")
                                            .arg(i+1).arg(invoice));
            return;
        }
        ssubs<<ssub;
    }
    QString dupliNum =  dupliInvoice();
    if(!dupliNum.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("发票号“%1”有重复！").arg(dupliNum));
        return;
    }
    QList<BusiAction*> bas;
    QString summary;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        Double taxMoney = ui->tw->item(i,CI_TAXMONEY)->text().toDouble();
        Double wbMoney = ui->tw->item(i,CI_WMONEY)->text().toDouble();
        summary = tr("应收%1运费 %2").arg(ssubs.at(i)->getName()).arg(invoices.at(i));
        QString ysSummary = summary;
        if(wbMoney != 0)
            ysSummary.append(tr("（%1%2）").arg(wmt->simpleSign()).arg(wbMoney.toString()));
        bas<<new BusiAction(0,pz,ysSummary,ysFSub,ssubs.at(i),mmt,MDIR_J,zmMoneys.at(i),0);
        if(taxMoney != 0)
            bas<<new BusiAction(0,pz,summary,sjFSub,xxSSub,mmt,MDIR_D,taxMoney,0);
    }
    int sumRow = ui->tw->rowCount()-1;
    Double sumZm = ui->tw->item(sumRow,CI_MONEY)->text().toDouble();
    Double sumTax = ui->tw->item(sumRow,CI_TAXMONEY)->text().toDouble();
    bas<<new BusiAction(0,pz,tr("应收运费"),srFSub,srDefSSub,mmt,MDIR_D,sumZm-sumTax,0);
    pzDlg->insertBas(bas);
    //剔除未被最终使用的临时科目（可能由于移除、剪切行等行为引起）
    foreach (SecondSubject* ssub, extraSSubs) {
        if(!ssubs.contains(ssub))
            extraSSubs.removeOne(ssub);
    }
    //将临时科目转正
    if(!extraSSubs.isEmpty()){
        foreach(SecondSubject* ssub, extraSSubs)
            ysFSub->addChildSub(ssub);
    }
    ok = true;
}

/**
 * @brief 创建应付聚合分录
 */
void BaTemplateForm::createYfGatherBas()
{
    if(!ui->rdoYfGather->isChecked())
        return;
    if(ui->tw->rowCount()==1)
        return;
    //检测遗漏项（发票号、账面金额、客户）
    QStringList invoices;
    QList<Double> zmMoneys;
    QList<SecondSubject*> ssubs;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        QString invoice = ui->tw->item(i,CI_INVOICE)->text();
        if(invoice.isEmpty() || !invoiceQualified(invoice)){
            myHelper::ShowMessageBoxWarning(tr("第%1行没有输入发票号或无效！").arg(i+1));
            return;
        }
        invoices<<invoice;
        Double zmMoney = ui->tw->item(i,CI_MONEY)->text().toDouble();
        if(zmMoney == 0){
            myHelper::ShowMessageBoxWarning(tr("第%1行没有输入发票账面金额！").arg(i+1));
            return;
        }
        zmMoneys<<zmMoney;
        SecondSubject* ssub = ui->tw->item(i,CI_CUSTOMER)->data(Qt::EditRole).value<SecondSubject*>();
        if(!ssub){
            myHelper::ShowMessageBoxWarning(tr("第%1行（%2）没有设置客户！")
                                            .arg(i+1).arg(invoice));
            return;
        }
        ssubs<<ssub;
    }
    QString dupliNum =  dupliInvoice();
    if(!dupliNum.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("发票号“%1”有重复！").arg(dupliNum));
        return;
    }

    QList<BusiAction*> bas;
    int sumRow = ui->tw->rowCount()-1;
    Double sumZm = ui->tw->item(sumRow,CI_MONEY)->text().toDouble();
    Double sumTax = ui->tw->item(sumRow,CI_TAXMONEY)->text().toDouble();
    bas<<new BusiAction(0,pz,tr("应付运费"),cbFSub,cbDefSSub,mmt,MDIR_J,sumZm-sumTax,0);

    QString summary;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        Double taxMoney = ui->tw->item(i,CI_TAXMONEY)->text().toDouble();
        Double wbMoney = ui->tw->item(i,CI_WMONEY)->text().toDouble();
        summary = tr("应付%1运费 %2").arg(ssubs.at(i)->getName()).arg(invoices.at(i));
        if(taxMoney != 0)
            bas<<new BusiAction(0,pz,summary,sjFSub,jxSSub,mmt,MDIR_J,taxMoney,0);
        QString yfSummary = summary;
        if(wbMoney != 0)
            yfSummary.append(tr("（%1%2）").arg(wmt->simpleSign()).arg(wbMoney.toString()));
        bas<<new BusiAction(0,pz,yfSummary,yfFSub,ssubs.at(i),mmt,MDIR_D,zmMoneys.at(i),0);
    }
    pzDlg->insertBas(bas);
    foreach (SecondSubject* ssub, extraSSubs) {
        if(!ssubs.contains(ssub))
            extraSSubs.removeOne(ssub);
    }
    if(!extraSSubs.isEmpty()){
        foreach(SecondSubject* ssub, extraSSubs)
            yfFSub->addChildSub(ssub);
    }
    ok = true;
}

QList<int> BaTemplateForm::selectedRows()
{
    QSet<int> rowSets;
    QList<QTableWidgetSelectionRange> ranges = ui->tw->selectedRanges();
    foreach (QTableWidgetSelectionRange range, ranges) {
        for(int i = range.topRow(); i <= range.bottomRow(); ++i)
            rowSets.insert(i);
    }
    QList<int> rows;
    rows = rowSets.toList();
    qSort(rows.begin(),rows.end());
    return rows;
}

void BaTemplateForm::copyRow(int row)
{
    bool isCustomer = false;
    if(ui->rdoYsGather->isChecked() || ui->rdoYfGather->isChecked())
        isCustomer = true;
    InvoiceRowStruct* r = new InvoiceRowStruct;
    r->money = ui->tw->item(row,CI_MONTH)->text().toInt();
    r->inum = ui->tw->item(row,CI_INVOICE)->text();
    r->money = ui->tw->item(row,CI_MONEY)->text().toDouble();
    r->taxMoney = ui->tw->item(row,CI_TAXMONEY)->text().toDouble();
    r->wMoney = ui->tw->item(row,CI_WMONEY)->text().toDouble();
    if(isCustomer)
        r->ssub = ui->tw->item(row,CI_CUSTOMER)->data(Qt::EditRole).value<SecondSubject*>();
    else
        r->ssub = 0;
    buffers<<r;
}

/**
 * @检测表格中的发票号是否有重复
 * @return 返回发现的第一个重复的发票号
 */
QString BaTemplateForm::dupliInvoice()
{
    QSet<QString> inums;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        QString inum = ui->tw->item(i,CI_INVOICE)->text();
        if(inums.contains(inum))
            return inum;
        inums.insert(inum);
    }
    return "";
}

/**
 * @brief 计算指定金额列的合计值
 * @param col
 */
void BaTemplateForm::reCalSum(BaTemplateForm::ColumnIndex col)
{
    if(col < CI_MONEY || col > CI_WMONEY || ui->tw->rowCount() == 1)
        return;
    Double sum;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        Double v = ui->tw->item(i,col)->text().toDouble();
        sum += v;
    }
    turnDataInspect(false);
    ui->tw->item(ui->tw->rowCount()-1,col)->setText(sum.toString());
    turnDataInspect();
}

void BaTemplateForm::reCalAllSum()
{
    reCalSum(CI_MONEY);
    reCalSum(CI_TAXMONEY);
    reCalSum(CI_WMONEY);
}

void BaTemplateForm::turnDataInspect(bool on)
{
    if(on)
        connect(ui->tw,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(DataChanged(QTableWidgetItem*)));
    else
        disconnect(ui->tw,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(DataChanged(QTableWidgetItem*)));
}

bool BaTemplateForm::invoiceQualified(QString inum)
{
    QRegExp re("^\\d{8}$");
    if(re.indexIn(inum) == -1)
        return false;
    return true;
}

/**
 * @brief 检测所有表格行上的发票号是否有效
 * @return 如果都有效，则返回-1，否则返回第一个无效的行号
 */
int BaTemplateForm::invoiceQualifieds()
{
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        if(!invoiceQualified(ui->tw->item(i,CI_INVOICE)->text())){
            myHelper::ShowMessageBoxWarning(tr("第%1行发票号无效！").arg(i+1));
            return i+1;
        }
    }
    return -1;
}

/**
 * @brief 在指定行设置与指定发票号对应的应收/应付发票的金额信息，并比较对应的客户是否一致
 * 调用此函数，前提是在应收应付模式下，且行号是一个发票有效行
 * @param row
 * @param inum
 */
void BaTemplateForm::autoSetYsYf(int row, QString inum)
{
    InvoiceRecord* r = amgr->searchInvoice(ui->rdoYsIncome->isChecked(),inum);
    if(!r)
        return;
    FirstSubject* fsub;
    if(ui->rdoYsIncome->isChecked())
        fsub = ysFSub;
    else
        fsub = yfFSub;
    SecondSubject* ssub = fsub->getChildSub(r->customer);
    if(curCusSSub != ssub){
        //myHelper::ShowMessageBoxWarning(tr("有点问题哦！该发票（%1）对应客户（%2）与当前客户不一致！")
        //                                .arg(inum).arg(r->customer?r->customer->getLongName():tr("空")));
        QTableWidgetItem* it = ui->tw->item(row,CI_INVOICE);
        it->setForeground(QBrush(Qt::red));
    }
    int month = (amgr->year() == r->year)?r->month:-r->month;
    ui->tw->item(row,CI_MONTH)->setText(QString::number(month));
    ui->tw->item(row,CI_MONEY)->setText(r->money.toString());
    if(!r->isCommon)
        ui->tw->item(row,CI_TAXMONEY)->setText(r->taxMoney.toString());
    if(r->wmoney != 0)
        ui->tw->item(row,CI_WMONEY)->setText(r->wmoney.toString());
}

//测试函数，创建银行收入的测试数据
//void BaTemplateForm::initBankIncomeTestData()
//{
//    addNewInvoice();
//    SecondSubject* ssub = ysFSub->getChildSub(tr("上海亚恩"));
//    ui->cmbCustomer->setSubject(ssub);
//     //收人民币带税金
////    ui->edtBankMoney->setText("3432");
////    ui->tw->item(0,CI_INVOICE)->setText("01340554");
////    Double dv(3432.0);
////    ui->tw->item(0,CI_MONEY)->setText(dv.toString());
////    dv = 194.26;
////    ui->tw->item(0,CI_TAXMONEY)->setText(dv.toString());

//    //收美金不带税金
//    ui->cmbBank->setCurrentIndex(1);
//    ui->edtBankMoney->setText("558.76");
//    ui->tw->item(0,CI_INVOICE)->setText("01340554");
//    Double dv(3432.0);
//    ui->tw->item(0,CI_MONEY)->setText(dv.toString());
//    //dv = 194.26;
//    //ui->tw->item(0,CI_TAXMONEY)->setText(dv.toString());
//    dv = 558.76;
//    ui->tw->item(0,CI_WMONEY)->setText(dv.toString());
//}

//void BaTemplateForm::initBankCostTestData()
//{
//    SecondSubject* ssub = yfFSub->getChildSub(tr("上海世航"));
//    ui->cmbCustomer->setSubject(ssub);
//    ui->edtBankMoney->setText("1823.45");
//    ui->cmbBank->setCurrentIndex(1);
//    addNewInvoice();
//    ui->tw->item(0,CI_INVOICE)->setText("11592451");
//    Double dv(8000.0);
//    ui->tw->item(0,CI_MONEY)->setText(dv.toString());
//    dv = 480.0;
//    ui->tw->item(0,CI_TAXMONEY)->setText(dv.toString());
//    dv = 1302.46;
//    ui->tw->item(0,CI_WMONEY)->setText(dv.toString());
//    addNewInvoice();
//    ui->tw->item(1,CI_INVOICE)->setText("11592500");
//    dv = 3200.0;
//    ui->tw->item(1,CI_MONEY)->setText(dv.toString());
//    dv = 192.0;
//    ui->tw->item(1,CI_TAXMONEY)->setText(dv.toString());
//    dv = 520.99;
//    ui->tw->item(1,CI_WMONEY)->setText(dv.toString());

//}

//void BaTemplateForm::initYsTestData()
//{

//    SecondSubject* ssub = ysFSub->getChildSub(tr("杰埃伊（上海）"));
//    ui->cmbCustomer->setSubject(ssub);
//    QStringList invoices;
//    invoices<<"01285082"<<"01285084"<<"01285086";
//    QList<int> months;
//    months<<2<<3<<3;
//    QList<Double> zmMoneys;
//    zmMoneys<<6190.0<<14485.0<<4035.0;
//    ui->edtBankMoney->setText("24710");

//    //invoices<<"01285144";
//    //zmMoneys<<11249.93;
//    //QList<Double> wbMoneys;
//    //wbMoneys<<1830.0;
//    //ui->edtBankMoney->setText("1830");
//    //ui->cmbBank->setCurrentIndex(1);

//    for(int i = 0; i < invoices.count(); ++i){
//        addNewInvoice();
//        ui->tw->item(i,CI_INVOICE)->setText(invoices.at(i));
//        ui->tw->item(i,CI_MONTH)->setText(QString::number(months.at(i)));
//        ui->tw->item(i,CI_MONEY)->setText(zmMoneys.at(i).toString());
//        //ui->tw->item(i,CI_WMONEY)->setText(wbMoneys.at(i).toString());
//    }

//}

//void BaTemplateForm::initYfTestData()
//{
//    //应付宁波中远运费 01369752（$4380）26801.22
//    //应付宁波中远运费 01364834（$12250）74957.75
//    //付宁波中远运费 2月01369752/4834 $16630
//    SecondSubject* ssub = yfFSub->getChildSub(tr("宁波中远"));
//    ui->cmbCustomer->setSubject(ssub);
//    QStringList invoices;
//    invoices<<"01369752"<<"01364834";
//    QList<int> months;
//    months<<2<<2;
//    QList<Double> zmMoneys;
//    zmMoneys<<26801.22<<74957.75;
//    ui->edtBankMoney->setText("16630");
//    ui->cmbBank->setCurrentIndex(1);
//    QList<Double> wbMoneys;
//    wbMoneys<<4380.0<<12250.0;

//    for(int i = 0; i < invoices.count(); ++i){
//        addNewInvoice();
//        ui->tw->item(i,CI_INVOICE)->setText(invoices.at(i));
//        ui->tw->item(i,CI_MONTH)->setText(QString::number(months.at(i)));
//        ui->tw->item(i,CI_MONEY)->setText(zmMoneys.at(i).toString());
//        ui->tw->item(i,CI_WMONEY)->setText(wbMoneys.at(i).toString());
//    }

//}

//void BaTemplateForm::initYsGatherTestData()
//{
//    QStringList invoices;
//    invoices<<"00237752"<<"00237756"<<"00237758"<<"01340580";
//    QList<Double> zmMoneys;
//    zmMoneys<<2965.0<<4676.0<<1382.27<<790.0;
//    QList<Double> taxMoneys;
//    taxMoneys<<0.0<<0.0<<0.0<<44.72;
//    QList<Double>wbMoneys;
//    wbMoneys<<0.0<<0.0<<225.0<<0.0;
//    QList<SecondSubject*> ssubs;
//    ssubs<<ysFSub->getChildSub(tr("繁昌沃泰科"))
//         <<ysFSub->getChildSub(tr("鄞州祥宏"))
//         <<ysFSub->getChildSub(tr("慈溪骏马"))
//         <<ysFSub->getChildSub(tr("宁波世贸通"));
//    for(int i = 0; i < invoices.count(); ++i){
//        addNewInvoice();
//        ui->tw->item(i,CI_INVOICE)->setText(invoices.at(i));
//        ui->tw->item(i,CI_MONEY)->setText(zmMoneys.at(i).toString());
//        ui->tw->item(i,CI_TAXMONEY)->setText(taxMoneys.at(i).toString());
//        ui->tw->item(i,CI_WMONEY)->setText(wbMoneys.at(i).toString());
//        QVariant v; v.setValue<SecondSubject*>(ssubs.at(i));
//        ui->tw->item(i,CI_CUSTOMER)->setData(Qt::EditRole,v);
//    }
//}

//void BaTemplateForm::initYfGatherTextData()
//{
//    QStringList invoices;
//    invoices<<"01226118"<<"02491161"<<"00092286";
//    QList<Double> zmMoneys;
//    zmMoneys<<6245.0<<11979.63<<10852.24;
//    QList<Double> taxMoneys;
//    taxMoneys<<353.49<<0.0<<0.0;
//    QList<Double>wbMoneys;
//    wbMoneys<<0.0<<0.0<<1950.0;
//    QList<SecondSubject*> ssubs;
//    ssubs<<yfFSub->getChildSub(tr("宁波国际物流"))
//         <<yfFSub->getChildSub(tr("宁波西瑞"))
//         <<yfFSub->getChildSub(tr("北京中翼腾飞"));
//    for(int i = 0; i < invoices.count(); ++i){
//        addNewInvoice();
//        ui->tw->item(i,CI_INVOICE)->setText(invoices.at(i));
//        ui->tw->item(i,CI_MONEY)->setText(zmMoneys.at(i).toString());
//        ui->tw->item(i,CI_TAXMONEY)->setText(taxMoneys.at(i).toString());
//        ui->tw->item(i,CI_WMONEY)->setText(wbMoneys.at(i).toString());
//        QVariant v; v.setValue<SecondSubject*>(ssubs.at(i));
//        ui->tw->item(i,CI_CUSTOMER)->setData(Qt::EditRole,v);
//    }
//}

//测试函数，测试创建多条分录功能
//void BaTemplateForm::testInsertMultiBAs()
//{
//    FirstSubject* zysrFSub = sm->getZysrSub();
//    QList<BusiAction*> bas;
//    Money* mt = amgr->getAccount()->getAllMoneys().value(RMB);
//    QString summary = "test summary";
//    BusiAction* ba1 = new BusiAction(0,pz,summary,bankFSub,bankFSub->getDefaultSubject(),
//                                     mt,MDIR_J,1000.0,0);
//    BusiAction* ba2 = new BusiAction(0,pz,summary,zysrFSub,zysrFSub->getDefaultSubject(),
//                                     mt,MDIR_D,1000.0,0);
//    bas<<ba1<<ba2;
//    pzDlg->insertBas(bas);
//}

void BaTemplateForm::on_btnOk_clicked()
{
    if(ui->rdoBankIncome->isChecked())
        createBankIncomeBas();
    else if(ui->rdoBankCost->isChecked())
        createBankCostBas();
    else if(ui->rdoYsIncome->isChecked())
        createYsBas();
    else if(ui->rdoYfCost->isChecked())
        createYfBas();
    else if(ui->rdoYsGather->isChecked())
        createYsGatherBas();
    else
        createYfGatherBas();
    if(ok){
        clear();
        close();
    }
}

//void BaTemplateForm::on_btnTest_clicked()
//{
//    if(ui->rdoBankIncome->isChecked())
//        initBankIncomeTestData();
//    else if(ui->rdoBankCost->isChecked())
//        initBankCostTestData();
//    else if(ui->rdoYsIncome->isChecked())
//        initYsTestData();
//    else if(ui->rdoYfCost->isChecked())
//        initYfTestData();
//    else if(ui->rdoYsGather->isChecked())
//        initYsGatherTestData();
//    else
//        initYfGatherTextData();

//}

void BaTemplateForm::on_btnCancel_clicked()
{
    clear();
    close();
}


/**
 * @brief 保存模板数据并关闭窗口
 */
void BaTemplateForm::on_btnSave_clicked()
{
    if(ui->tw->rowCount() == 1)
        return;
    BaTemplateType type;
    if(ui->rdoYsGather->isChecked())
        type = BTT_YS_GATHER;
    else if(ui->rdoYfGather->isChecked())
        type = BTT_YF_GATHER;
    else
        return;
    QList<InvoiceRowStruct*> datas;
    for(int i = 0; i < ui->tw->rowCount()-1; ++i){
        InvoiceRowStruct* r = new InvoiceRowStruct;
        r->month = 0;
        r->inum = ui->tw->item(i,CI_INVOICE)->text();
        r->money = ui->tw->item(i,CI_MONEY)->text().toDouble();
        r->taxMoney = ui->tw->item(i,CI_TAXMONEY)->text().toDouble();
        r->wMoney = ui->tw->item(i,CI_WMONEY)->text().toDouble();
        r->ssub = ui->tw->item(i,CI_CUSTOMER)->data(Qt::EditRole).value<SecondSubject*>();
        if(r->ssub){
            r->sname = r->ssub->getName();
            if(extraSSubs.contains(r->ssub)){
                r->lname = r->ssub->getLName();
                r->remCode = r->ssub->getRemCode();
                r->ssub = 0;
            }
        }
        datas<<r;
    }
    if(!amgr->getAccount()->getDbUtil()->saveBaTemplateDatas(type,datas)){
        myHelper::ShowMessageBoxError(tr("保存模板数据出错！"));
    }
    qDeleteAll(datas);
    datas.clear();
    clear();
    close();
}

void BaTemplateForm::on_btnLoad_clicked()
{
    if(ui->tw->rowCount()>1){
        if(QDialog::Rejected == myHelper::ShowMessageBoxQuesion(tr("装载后将覆盖当前已录入的数据，是否继续？")))
            return;
        clear();
    }
    QList<InvoiceRowStruct*> datas;
    BaTemplateType type;
    FirstSubject* fsub;
    if(ui->rdoYsGather->isChecked()){
        fsub = ysFSub;
        type = BTT_YS_GATHER;
    }
    else if(ui->rdoYfGather->isChecked()){
        fsub = yfFSub;
        type = BTT_YF_GATHER;
    }
    else
        return;
    if(!amgr->getAccount()->getDbUtil()->readBaTemplateDatas(type,datas)){
        myHelper::ShowMessageBoxError(tr("读取模板数据发生错误！"));
        return;
    }
    if(datas.isEmpty())
        return;
    turnDataInspect(false);
    for(int i = 0; i < datas.count(); ++i){
        InvoiceRowStruct* r = datas.at(i);
        ui->tw->insertRow(i);
        initRow(i);
        ui->tw->item(i,CI_INVOICE)->setText(r->inum);
        ui->tw->item(i,CI_MONEY)->setText(r->money.toString());
        ui->tw->item(i,CI_TAXMONEY)->setText(r->taxMoney.toString());
        ui->tw->item(i,CI_WMONEY)->setText(r->wMoney.toString());
        if(!r->ssub && !r->sname.isEmpty())
            r->ssub = fsub->getChildSub(r->sname);
        if(!r->ssub){
            SubjectNameItem* ni = sm->getNameItem(r->sname);
            if(!ni){
                if(!r->sname.isEmpty())
                    ni = new SubjectNameItem(0,2,r->sname,r->lname,r->remCode,QDateTime(),curUser);
                else
                    continue;
            }
            r->ssub = new SecondSubject(fsub,0,ni,"",1,true,QDateTime(),QDateTime(),curUser);
            extraSSubs<<r->ssub;
        }
        QVariant v; v.setValue<SecondSubject*>(r->ssub);
        ui->tw->item(i,CI_CUSTOMER)->setData(Qt::EditRole,v);
    }
    turnDataInspect();
    reCalAllSum();
    qDeleteAll(datas);
    datas.clear();
}
