#include "curinvoicestatform.h"
#include "ui_curinvoicestatform.h"
#include "myhelper.h"
#include "xlsxworksheet.h"
#include "subject.h"
#include "PzSet.h"
#include "pz.h"
#include "dbutil.h"
#include "validator.h"

#include <QFileDialog>
#include <QActionGroup>
#include <QMenu>
#include <QSpinBox>
#include <QDateEdit>
#include <QComboBox>

////////////////////////////InvoiceClientItem/////////////////////////////
InvoiceClientItem::InvoiceClientItem(QString name, int type)
    :QTableWidgetItem(name,type),name(name),ni(0),_matchState(CMS_NONE)
{    
    mc_precise = QColor(Qt::darkGreen);
    mc_alias = QColor(Qt::darkYellow);
    mc_new = QColor(Qt::darkMagenta);
    mc_none = QColor(Qt::red);
}

void InvoiceClientItem::setText(QString text)
{
    name = text;
    QTableWidgetItem::setText(text);
}

void InvoiceClientItem::setNameItem(SubjectNameItem *ni)
{
    this->ni=ni;
    if(!ni){
        _matchState = CMS_NONE;
        return;
    }
    if(ni->getId() == 0)
        _matchState = CMS_NEWCLIENT;
    else if(ni->getLongName() == name)
        _matchState = CMS_PRECISE;
    else
        _matchState = CMS_ALIAS;
}

QVariant InvoiceClientItem::data(int role) const
{
    if(role == Qt::DisplayRole)
        return name;
    else if(role == Qt::ToolTipRole){
        return ni?ni->getLongName():"";
    }
    else if(role == Qt::EditRole){
        QVariant v;
        v.setValue<SubjectNameItem*>(ni);
        return v;
    }
    else if(role == Qt::ForegroundRole){
        switch(_matchState){
        case CMS_NONE:
            return mc_none;
        case CMS_PRECISE:
            return mc_precise;
        case CMS_ALIAS:
            return mc_alias;
        case CMS_NEWCLIENT:
            return mc_new;
        }
    }
    else
        return QTableWidgetItem::data(role);
}

void InvoiceClientItem::setData(int role, const QVariant &value)
{
    if(role == Qt::DisplayRole)
        name = value.toString();
    else if(role == Qt::EditRole){
        SubjectNameItem* ni = value.value<SubjectNameItem*>();
        setNameItem(ni);
    }
    else
        return QTableWidgetItem::setData(role,value);
}

/////////////////////////InvoiceStateItem//////////////////////////////////
MultiStateItem::MultiStateItem(const QHash<int, QString> &stateNames, bool isForeground, int type)
    :QTableWidgetItem(type),stateCode(0),isForeground(isForeground)
{
    this->stateNames = stateNames;
    if(!stateNames.isEmpty()){
        QList<int> codes = stateNames.keys();
        qSort(codes.begin(),codes.end());
        stateCode = codes.first();
    }
}

QVariant MultiStateItem::data(int role) const
{
    if(role == Qt::DisplayRole)
        return stateNames.value(stateCode);
    if(role == Qt::EditRole)
        return stateCode;
    if(role == Qt::ForegroundRole && isForeground){
        if(stateColors.value(stateCode).isValid())
            return stateColors.value(stateCode);
    }
    if(role == Qt::BackgroundColorRole && !isForeground){
        if(stateColors.value(stateCode).isValid())
            return stateColors.value(stateCode);
    }
    return QTableWidgetItem::data(role);
}

void MultiStateItem::setData(int role, const QVariant &value)
{
    if(role == Qt::DisplayRole)
        return;
    if(role == Qt::EditRole){
        int c = value.toInt();
        if(stateNames.keys().contains(c))
            stateCode = c;
    }
    QTableWidgetItem::setData(role,value);
}

///////////////////////////InvoiceProcessItem////////////////////////////////
InvoiceProcessItem::InvoiceProcessItem(int stateCode, int type):QTableWidgetItem(type),stateCode(stateCode)
{
    icons[0] = QIcon("://images/error.png");    //表示不存在
    icons[1] = QIcon("://images/info.png");     //表示正确
    icons[2] = QIcon("://images/question.png"); //表示有问题
}

QVariant InvoiceProcessItem::data(int role) const
{
    if(role == Qt::DecorationRole)
        return icons.value(stateCode);
    if(role == Qt::ToolTipRole)
        return errInfos;
    return QTableWidgetItem::data(role);
}

void InvoiceProcessItem::setProcessState(int state)
{
    if(state>=0 && state <=2)
        stateCode = state;
}

////////////////////////////////HandMatchClientForm///////////////////////////////////////
HandMatchClientDialog::HandMatchClientDialog(SubjectNameItem *nameItem, SubjectManager *subMgr, QString name, QWidget *parent)
    :QDialog(parent),ni(nameItem),sm(subMgr),name(name)
{
    nameEdit = new QLineEdit(this);
    nameEdit->setText(name);
    nameEdit->setReadOnly(true);
    nameEdit->installEventFilter(this);
    connect(nameEdit,SIGNAL(editingFinished()),this,SLOT(nameEditFinished()));
    lwNames = new QListWidget(this);
    QList<SubjectNameItem*> nis = sm->getAllNameItems(SORTMODE_NAME);
    for(int i = 0; i < nis.count(); ++i){
        SubjectNameItem* ni = nis.at(i);
        QListWidgetItem* li = new QListWidgetItem(ni->getLongName(),lwNames);
        QVariant v; v.setValue<SubjectNameItem*>(ni);
        li->setData(Qt::UserRole,v);
    }
    refreshList();
    btnOk = new QPushButton(QIcon(":/images/btn_ok.png"),tr("确定"));
    btnCancel = new QPushButton(QIcon(":/images/btn_close.png"),tr("取消"));
    btnNewClient = new QPushButton(tr("作为新客户"));
    QSpacerItem* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(btnNewClient);
    btnLayout->addItem(horizontalSpacer);
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);
    QVBoxLayout* contentLayout = new QVBoxLayout;
    contentLayout->addWidget(nameEdit);
    contentLayout->addWidget(lwNames);
    QVBoxLayout* ml = new QVBoxLayout;
    ml->addLayout(contentLayout);
    ml->addLayout(btnLayout);
    setLayout(ml);
    setModal(true);
    connect(btnOk,SIGNAL(clicked()),this,SLOT(btnOkClicked()));
    connect(btnCancel,SIGNAL(clicked()),this,SLOT(reject()));
    connect(btnNewClient,SIGNAL(clicked()),this,SLOT(confirmNewClient()));
    connect(nameEdit,SIGNAL(selectionChanged()),this,SLOT(keysChanged()));
}

void HandMatchClientDialog::setClientName(SubjectNameItem *ni, QString name)
{
    this->ni = ni;
    this->name = name;
    nameEdit->setText(name);
    refreshList();
}

QString HandMatchClientDialog::clientName()
{
    return nameEdit->text();
}

/**
 * @brief 双击开启名称的编辑功能
 * @param obj
 * @param event
 * @return
 */
bool HandMatchClientDialog::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() != QEvent::MouseButtonDblClick)
        return nameEdit->eventFilter(obj,event);
    QLineEdit* w = qobject_cast<QLineEdit*>(obj);
    w->setReadOnly(false);
    return w->eventFilter(obj,event);
}

void HandMatchClientDialog::nameEditFinished()
{
    nameEdit->setReadOnly(true);
}

void HandMatchClientDialog::btnOkClicked()
{
    if(name != nameEdit->text())
        emit clinetNameChanged(name,nameEdit->text());
    if(lwNames->currentRow() == -1){
        close();
        return;
    }
    SubjectNameItem* nameObj = lwNames->currentItem()->data(Qt::UserRole).value<SubjectNameItem*>();
    if(nameObj != ni)
        emit clientMatchChanged(nameEdit->text(),nameObj);
    close();
}

/**
 * @brief 当选择的名称关键词改变时，重新刷新列表
 */
void HandMatchClientDialog::keysChanged()
{
    if(!nameEdit->hasFocus())
        return;
    QString keys = nameEdit->selectedText();
    refreshList(keys);
}

void HandMatchClientDialog::confirmNewClient()
{
    QDialog dlg;
    QLabel lblShorName(tr("简称"),&dlg);
    QLabel lblLongName(tr("全称"),&dlg);
    QLabel lblRemCode(tr("助记符"),&dlg);
    QLineEdit edtShortName("",&dlg);
    QLineEdit edtLongeName(name,&dlg);
    edtLongeName.setReadOnly(true);
    QLineEdit edtRemCode("",&dlg);
    QGridLayout gl;
    gl.addWidget(&lblShorName,0,0);
    gl.addWidget(&edtShortName,0,1);
    gl.addWidget(&lblLongName,1,0);
    gl.addWidget(&edtLongeName,1,1);
    gl.addWidget(&lblRemCode,2,0);
    gl.addWidget(&edtRemCode,2,1);
    QPushButton bo(QIcon(":/images/btn_ok.png"),tr("确定"),&dlg);
    QPushButton bc(QIcon(":/images/btn_close.png"),tr("取消"),&dlg);
    QSpacerItem* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(&bo,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&bc,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout bl;
    bl.addItem(horizontalSpacer);
    bl.addWidget(&bo);
    bl.addWidget(&bc);
    QVBoxLayout *lm = new QVBoxLayout;
    lm->addLayout(&gl);
    lm->addLayout(&bl);
    dlg.setLayout(lm);
    dlg.resize(600,200);
    if(dlg.exec() == QDialog::Rejected)
        return;
    if(edtShortName.text().isEmpty() || edtRemCode.text().isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("客户简称或助记符不能为空！"));
        return;
    }
    NameItemAlias* alias = new NameItemAlias(edtShortName.text(),edtLongeName.text(),
                                             edtRemCode.text(),QDateTime::currentDateTime());
    emit createNewClientAlias(alias);
    close();
}

void HandMatchClientDialog::refreshList(QString keys)
{
    QString filteWords;
    if(keys.isEmpty()){
        filteWords = name;
    }
    else{
        filteWords = keys;
    }
    for(int i = 0; i < lwNames->count(); ++i){
        QString t = lwNames->item(i)->text();
        lwNames->item(i)->setHidden(!t.contains(filteWords));
    }
    if(ni)
        filteWords = ni->getLongName();
    else
        filteWords = name;
    for(int i = 0; i < lwNames->count(); ++i){
        if(lwNames->item(i)->text() == filteWords){
            lwNames->setCurrentItem(lwNames->item(i),QItemSelectionModel::SelectCurrent);
            break;
        }
    }
}


//////////////////////////////////////////////////////
InvoiceInfoDelegate::InvoiceInfoDelegate(QWidget *parent):QItemDelegate(parent)
{

}

QWidget *InvoiceInfoDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int col = index.column();
    switch(col){
    case CurInvoiceStatForm::TI_NUMBER:{
        QSpinBox* editor = new QSpinBox(parent);
        editor->setMinimum(1);
        return editor;}
    case CurInvoiceStatForm::TI_DATE:{
        QString ds = index.model()->data(index).toString();
        QDate d = QDate::fromString(ds,Qt::ISODate);
        if(d.isValid()){
            QDateEdit* editor = new QDateEdit(parent);
            editor->setDisplayFormat("yyyy-M-d");
            return editor;
        }
        else{
            QLineEdit* editor = new QLineEdit(parent);
            return editor;
        }}
    case CurInvoiceStatForm::TI_INUMBER:{
        QLineEdit* editor = new QLineEdit(parent);
        editor->setValidator(new QRegExpValidator(QRegExp("^[0-9]{8}$"),editor));
        return editor;}
    case CurInvoiceStatForm::TI_MONEY:
    case CurInvoiceStatForm::TI_TAXMONEY:
    case CurInvoiceStatForm::TI_WBMONEY:{
        QLineEdit* editor = new QLineEdit(parent);
        editor->setValidator(new MyDoubleValidator(editor));
        return editor;}
    case CurInvoiceStatForm::TI_ITYPE:{
        QComboBox* editor = new QComboBox(parent);
        editor->addItem(tr("普票"),false);
        editor->addItem(tr("专票"),true);
        return editor;}
    case CurInvoiceStatForm::TI_STATE:{
        QComboBox* editor = new QComboBox(parent);        
        editor->addItem(tr("正常"),1);
        editor->addItem(tr("冲红"),2);
        editor->addItem(tr("作废"),3);
        return editor;}
    case CurInvoiceStatForm::TI_SFINFO:{
        QLineEdit* editor = new QLineEdit(parent);
        return editor;}
    case CurInvoiceStatForm::TI_ISPROCESS:
        return 0;
    case CurInvoiceStatForm::TI_CLIENT:
        return 0;
    }

}

void InvoiceInfoDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    CurInvoiceStatForm::TableIndex col = (CurInvoiceStatForm::TableIndex)index.column();
    switch(col){
    case CurInvoiceStatForm::TI_NUMBER:{
        QSpinBox* w = qobject_cast<QSpinBox*>(editor);
        if(w){
            int num = index.model()->data(index).toInt();
            w->setValue(num);
        }
        break;
    }
    case CurInvoiceStatForm::TI_DATE:{
        QDateEdit* de = qobject_cast<QDateEdit*>(editor);
        if(de){
            QDate d = QDate::fromString(index.model()->data(index).toString(),Qt::ISODate);
            de->setDate(d);
        }
        else{
            QLineEdit* edt = qobject_cast<QLineEdit*>(editor);
            if(edt)
                edt->setText(index.model()->data(index).toString());
        }
        break;
    }
    case CurInvoiceStatForm::TI_INUMBER:{
        QLineEdit* w = qobject_cast<QLineEdit*>(editor);
        if(w){
            QString inum = index.model()->data(index).toString();
            w->setText(inum);
        }
        break;
    }
    case CurInvoiceStatForm::TI_MONEY:
    case CurInvoiceStatForm::TI_TAXMONEY:
    case CurInvoiceStatForm::TI_WBMONEY:{
        QLineEdit* w = qobject_cast<QLineEdit*>(editor);
        if(w){
            QString v = index.model()->data(index).toString();
            w->setText(v);
        }
        break;
    }
    case CurInvoiceStatForm::TI_ITYPE:{
        QComboBox* w = qobject_cast<QComboBox*>(editor);
        if(w){
            bool type = index.model()->data(index,Qt::EditRole).toBool();
            w->setCurrentIndex(type?1:0);
        }
        break;
    }
    case CurInvoiceStatForm::TI_STATE:{
        QComboBox* w = qobject_cast<QComboBox*>(editor);
        if(w){
            int stateCode = index.model()->data(index,Qt::EditRole).toInt();
            int idx = w->findData(stateCode);
            w->setCurrentIndex(idx);
        }
        break;
    }
    case CurInvoiceStatForm::TI_SFINFO:{
        QLineEdit* w = qobject_cast<QLineEdit*>(editor);
        if(w){
            QString v = index.model()->data(index).toString();
            w->setText(v);
        }
        break;
    }
    }

}

void InvoiceInfoDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    CurInvoiceStatForm::TableIndex col = (CurInvoiceStatForm::TableIndex)index.column();
    switch(col){
    case CurInvoiceStatForm::TI_NUMBER:{
        QSpinBox* w = qobject_cast<QSpinBox*>(editor);
        if(w){
            int num = w->value();
            model->setData(index,QString::number(num),Qt::DisplayRole);
        }
        break;
    }
    case CurInvoiceStatForm::TI_DATE:{
        QDateEdit* w = qobject_cast<QDateEdit*>(editor);
        if(w){
            QString d = w->date().toString(Qt::ISODate);
            model->setData(index,d,Qt::DisplayRole);
        }
        else{
            QLineEdit* edt = qobject_cast<QLineEdit*>(editor);
            model->setData(index,edt->text(),Qt::DisplayRole);
        }
        break;
    }
    case CurInvoiceStatForm::TI_INUMBER:{
        QLineEdit* w = qobject_cast<QLineEdit*>(editor);
        if(w)
            model->setData(index,w->text(),Qt::DisplayRole);
        break;
    }
    case CurInvoiceStatForm::TI_MONEY:
    case CurInvoiceStatForm::TI_TAXMONEY:
    case CurInvoiceStatForm::TI_WBMONEY:{
        QLineEdit* w = qobject_cast<QLineEdit*>(editor);
        if(w){
            model->setData(index,w->text(),Qt::DisplayRole);
        }
        break;
    }
    case CurInvoiceStatForm::TI_ITYPE:{
        QComboBox* w = qobject_cast<QComboBox*>(editor);
        if(w){
            bool type = (w->currentIndex()==1);
            model->setData(index,type);
        }
        break;
    }
    case CurInvoiceStatForm::TI_STATE:{
        QComboBox* w = qobject_cast<QComboBox*>(editor);
        if(w){
            int state = w->currentData().toInt();
            model->setData(index,state);
        }
        break;
    }
    case CurInvoiceStatForm::TI_SFINFO:{
        QLineEdit* w = qobject_cast<QLineEdit*>(editor);
        if(w){
            model->setData(index,w->text(),Qt::DisplayRole);
        }
        break;
    }
    }
}

void InvoiceInfoDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    editor->setGeometry(rect);
}



/////////////////////////////CurInvoiceStatForm/////////////////////////////////
CurInvoiceStatForm::CurInvoiceStatForm(Account *account, QWidget *parent) :
    QWidget(parent),ui(new Ui::CurInvoiceStatForm),excel(0),handMatchDlg(0),
    account(account),sort_in(TSC_PRIMARY),sort_cost(TSC_PRIMARY),contextMenuSelectedCol(-1)
{
    ui->setupUi(this);
    suiteMgr = account->getSuiteMgr();
    sm = suiteMgr->getSubjectManager();
    init();
}

CurInvoiceStatForm::~CurInvoiceStatForm()
{
    delete ui;
}

/**
 * @brief 双击列表项读取表格
 * @param item
 */
void CurInvoiceStatForm::doubleItemReadSheet(QListWidgetItem *item)
{
    if(item->data(DR_READED).toBool())
        return;
    int index = ui->lwSheets->row(item);
    ui->stackedWidget->setCurrentIndex(index);
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->widget(index));
    if(!readSheet(index,tw))
       myHelper::ShowMessageBoxError(tr("读取表单“%1”时出错！").arg(ui->lwSheets->item(index)->text()));
    else
       item->setData(DR_READED,true);
    ui->stackedWidget->setCurrentWidget(tw);
}

void CurInvoiceStatForm::curSheetChanged(int index)
{
    if(index<0 || index >= ui->lwSheets->count())
        return;
//    bool readTag = ui->lwSheets->item(index)->data(DR_READED).toBool();
//    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->widget(index));
//    if(!readTag && !readSheet(index,tw))
//        myHelper::ShowMessageBoxError(tr("读取表单“%1”时出错！").arg(ui->lwSheets->item(index)->text()));
//    else
//        ui->lwSheets->item(index)->setData(DR_READED,true);
    ui->stackedWidget->setCurrentIndex(index);
}

/**
 * @brief 表单列表上下文菜单构建
 * @param pos
 */
void CurInvoiceStatForm::sheetListContextMeny(const QPoint &pos)
{
    //注意：只有在读取后未导入时，才提供菜单
    QListWidgetItem* li = ui->lwSheets->itemAt(pos);
    QMenu *m = new QMenu(this);
    //bool readTag = li->data(DR_READED).toBool();
    //if(!readTag)
    //    m->addAction(ui->actReadSheet);
    InvoiceType type = (InvoiceType)li->data(DR_ITYPE).toInt();
    ui->actYS->setChecked(type == IT_INCOME);
    ui->actYF->setChecked(type == IT_COST);
    m->addActions(ag3->actions());
    m->popup(ui->lwSheets->mapToGlobal(pos));
}

void CurInvoiceStatForm::tableHHeaderContextMenu(const QPoint &pos)
{
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    contextMenuSelectedCol = tw->horizontalHeader()->logicalIndexAt(pos);
    QTableWidgetItem* hi = tw->horizontalHeaderItem(contextMenuSelectedCol);
    CurInvoiceColumnType colType = CT_NONE;
    if(hi)
        colType = (CurInvoiceColumnType)hi->data(Qt::UserRole).toInt();
    switchHActions(false);
    switch(colType){
    case CT_NONE:
        ui->actNone->setChecked(true);
        break;
    case CT_NUMBER:
        ui->actNum->setChecked(true);
        break;
    case CT_DATE:
        ui->actDate->setChecked(true);
        break;
    case CT_INVOICE:
        ui->actInvoice->setChecked(true);
        break;
    case CT_CLIENT:
        ui->actClient->setChecked(true);
        break;
    case CT_MONEY:
        ui->actMoney->setChecked(true);
        break;
    case CT_TAXMONEY:
        ui->actTaxMoney->setChecked(true);
        break;
    case CT_WBMONEY:
        ui->actWbMoney->setChecked(true);
        break;
    case CT_SFINFO:
        ui->actsfInfo->setChecked(true);
        break;
    case CT_ICLASS:
        ui->actInvoiceCls->setChecked(true);
        break;
    }
    switchHActions();
    mnuColTypes->popup(tw->horizontalHeader()->mapToGlobal(pos));
}

void CurInvoiceStatForm::tableVHeaderContextMenu(const QPoint &pos)
{
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    int row = tw->verticalHeader()->logicalIndexAt(pos);
    int sr = ui->lwSheets->currentItem()->data(DR_STARTROW).toInt();
    int er = ui->lwSheets->currentItem()->data(DR_ENDROW).toInt();
    RowType rowType = RT_NONE;
    if(sr == row)
        rowType = RT_START;
    else if(row == er){
        rowType = RT_END;
    }
    switchVActions(false);
    switch(rowType){
    case RT_NONE:
        ui->actCommRow->setChecked(true);
        break;
    case RT_START:
        ui->actStartRow->setChecked(true);
        break;
    case RT_END:
        ui->actEndRow->setChecked(true);;
        break;
    }
    ui->actCommRow->setData(row);
    ui->actStartRow->setData(row);
    ui->actEndRow->setData(row);
    switchVActions();
    mnuRowTypes->popup(tw->verticalHeader()->mapToGlobal(pos));
}

void CurInvoiceStatForm::processYsYfSelected(bool checked)
{
    if(!checked)
        return;
    QAction* a = qobject_cast<QAction*>(sender());
    if(a == ui->actYS){
        ui->lwSheets->currentItem()->setData(DR_ITYPE,IT_INCOME);
        ui->lwSheets->currentItem()->setForeground(QBrush(QColor("blue")));
        ui->lwSheets->currentItem()->setToolTip(tr("应收发票"));
        ui->lwSheets->currentItem()->setData(Qt::DecorationRole,icon_income);
    }
    else{
        ui->lwSheets->currentItem()->setData(DR_ITYPE,IT_COST);
        ui->lwSheets->currentItem()->setForeground(QBrush(QColor("red")));
        ui->lwSheets->currentItem()->setToolTip(tr("应付发票"));
        ui->lwSheets->currentItem()->setData(Qt::DecorationRole,icon_cost);
    }
}

/**
 * @brief 处理用户对列类型的选择上下文菜单
 * @param checked
 */
void CurInvoiceStatForm::processColTypeSelected(bool checked)
{
    if(!checked){
        return;
    }
    CurInvoiceColumnType colType = CT_NONE;
    QAction* act = qobject_cast<QAction*>(sender());
    if(act == ui->actNum)
        colType = CT_NUMBER;
    else if(act == ui->actDate)
        colType = CT_DATE;
    else if(act == ui->actInvoice)
        colType = CT_INVOICE;
    else if(act == ui->actClient)
        colType = CT_CLIENT;
    else if(act == ui->actMoney)
        colType = CT_MONEY;
    else if(act == ui->actTaxMoney)
        colType = CT_TAXMONEY;
    else if(act == ui->actWbMoney)
        colType = CT_WBMONEY;
    else if(act == ui->actsfInfo)
        colType = CT_SFINFO;
    else if(act == ui->actInvoiceCls)
        colType = CT_ICLASS;

    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    QTableWidgetItem* hi = 0;
    if(colType != CT_NONE)
        resetTableHeadItem(colType,tw);
    hi = tw->horizontalHeaderItem(contextMenuSelectedCol);
    if(!hi)
        hi = new QTableWidgetItem;
    QString colTitle;
    if(colType == CT_NONE)
        colTitle = QString::number(contextMenuSelectedCol+1);
    else
        colTitle = getColTypeText(colType);
    hi->setData(Qt::UserRole,colType);
    hi->setText(colTitle);
    tw->setHorizontalHeaderItem(contextMenuSelectedCol,hi);
}

void CurInvoiceStatForm::processRowTypeSelected(bool checked)
{
    if(!checked){
        return;
    }
    RowType rowType = RT_NONE;
    int sr = ui->lwSheets->currentItem()->data(DR_STARTROW).toInt();
    int er = ui->lwSheets->currentItem()->data(DR_ENDROW).toInt();
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    QAction* act = qobject_cast<QAction*>(sender());
    int row = act->data().toInt();
    if(act == ui->actStartRow){
        rowType = RT_START;
        if(row != sr && sr != -1){
            ui->lwSheets->currentItem()->setData(DR_STARTROW,row);
            tw->setVerticalHeaderItem(sr,0);
            QTableWidgetItem* hi = new QTableWidgetItem(QString("%1+").arg(row+1));
            hi->setToolTip(getRowTypeText(RT_START));
            tw->setVerticalHeaderItem(row,hi);
        }
    }
    else if(act == ui->actEndRow){
        rowType == RT_END;
        if(row != er){
            ui->lwSheets->currentItem()->setData(DR_ENDROW,row);
            tw->setVerticalHeaderItem(er,0);
            QTableWidgetItem* hi = new QTableWidgetItem(QString("%1-").arg(row+1));
            hi->setToolTip(getRowTypeText(RT_END));
            tw->setVerticalHeaderItem(row,hi);
        }
    }
}

/**
 * @brief 导入的收入成本发票表格的表头上下文菜单
 * @param pos
 */
void CurInvoiceStatForm::InvoiceTableHeaderContextMenu(const QPoint &pos)
{
    QTableWidget* tw;
    if(ui->tabWidget->currentIndex() == 0)
        tw = ui->twIncome;
    else
        tw = ui->twCost;
    int col = tw->horizontalHeader()->logicalIndexAt(pos);
    if(col != TI_CLIENT)
        return;
    QMenu* m = new QMenu(this);
    m->addAction(ui->actAutoMatch);
    m->popup(tw->horizontalHeader()->mapToGlobal(pos));
}


/**
 * @brief 返回与列类型匹配的标题文本
 * @param colType
 * @return
 */
QString CurInvoiceStatForm::getColTypeText(CurInvoiceColumnType colType)
{
    QString t;
    switch (colType) {
    case CT_NUMBER:
        t = tr("序号");
        break;
    case CT_DATE:
        t = tr("开票日期");
        break;
    case CT_INVOICE:
        t = tr("发票号码");
        break;
    case CT_CLIENT:
        t = tr("客户名");
        break;
    case CT_MONEY:
        t = tr("发票金额");
        break;
    case CT_TAXMONEY:
        t = tr("税额");
        break;
    case CT_WBMONEY:
        t = tr("外币金额");
        break;
    case CT_SFINFO:
        t = tr("收付信息");
        break;
    case CT_ICLASS:
        t = tr("发票类别");
        break;
    }
    return t;
}

QString CurInvoiceStatForm::getRowTypeText(CurInvoiceStatForm::RowType rowType)
{
    switch(rowType){
    case RT_START:
        return tr("开始行");
    case RT_END:
        return tr("结束行");
    default:
        return "";
    }
}

/**
 * @brief 跟踪发票信息的改变
 * @param item
 */
void CurInvoiceStatForm::invoiceInfoChanged(QTableWidgetItem *item)
{
    int col = item->column();
    int row = item->row();
    CurInvoiceRecord* r = item->tableWidget()->item(row,TI_INUMBER)->data(Qt::UserRole).value<CurInvoiceRecord*>();
    switch (col){
    case TI_NUMBER:{
        int num = item->text().toInt();
        if(num != r->num){
            r->num = num;
            r->tags->setBit(CI_TAG_NUMBER,true);
        }
        break;}
    case TI_DATE:{
        QDate d = QDate::fromString(item->text(),Qt::ISODate);
        if(d.isValid() && r->date.isValid() && d != r->date){
            r->date = d;
            r->tags->setBit(CI_TAG_DATE,true);
        }
        else{
            if(r->dateStr != item->text()){
                r->dateStr = item->text();
                r->tags->setBit(CI_TAG_DATE,true);
            }
        }
        break;}
    case TI_INUMBER:{
        QString inum = item->text();
        if(inum != r->inum){
            r->inum = inum;
            r->tags->setBit(CI_TAG_INUMBER,true);
        }
        break;}
    case TI_MONEY:{
        Double v = item->text().toDouble();
        if(v != r->money){
            r->money = v;
            r->tags->setBit(CI_TAG_MONEY,true);
        }
        break;}
    case TI_WBMONEY:{
        Double v = item->text().toDouble();
        if(v != r->wbMoney){
            r->wbMoney = v;
            r->tags->setBit(CI_TAG_WBMONEY,true);
        }
        break;}
    case TI_TAXMONEY:{
        Double v = item->text().toDouble();
        if(v != r->taxMoney){
            r->taxMoney = v;
            r->tags->setBit(CI_TAG_TAXMONEY,true);
        }
        break;}
    case TI_SFINFO:{
        QString info = item->text();
        if(info != r->sfInfo){
            r->sfInfo = info;
            r->tags->setBit(CI_TAG_SFSTATE);
        }
        break;}
    case TI_STATE:{
        int state = item->data(Qt::EditRole).toInt();
        if(state != r->state){
            r->state = state;
            r->tags->setBit(CI_TAG_STATE,true);
        }
        break;}
    case TI_ITYPE:{
        bool type = item->data(Qt::EditRole).toBool();
        if(type ^ r->type){
            r->type = type;
            r->tags->setBit(CI_TAG_TYPE,true);
        }
        break;}
    }
}

/**
 * @brief 如果双击客户列，则打开人工介入匹配对话框
 *        如果双击处理列，则跳转到指定凭证
 * @param item
 */
void CurInvoiceStatForm::itemDoubleClicked(QTableWidgetItem *item)
{
    if(item->column() == TI_CLIENT){
        InvoiceClientItem* ti = static_cast<InvoiceClientItem*>(item);
        SubjectNameItem* ni = ti->nameObject();
        if(!handMatchDlg){
            handMatchDlg = new HandMatchClientDialog(ni,sm,ti->text(),this);
            connect(handMatchDlg,SIGNAL(clinetNameChanged(QString,QString)),this,SLOT(clientNameChanged(QString,QString)));
            connect(handMatchDlg,SIGNAL(clientMatchChanged(QString,SubjectNameItem*)),this,SLOT(clientMatchChanged(QString,SubjectNameItem*)));
            connect(handMatchDlg,SIGNAL(createNewClientAlias(NameItemAlias*)),this,SLOT(createNewClientAlias(NameItemAlias*)));
            handMatchDlg->resize(600,400);
        }
        else{
            handMatchDlg->setClientName(ni,ti->text());
        }
        handMatchDlg->show();
    }
    else if(item->column() == TI_ISPROCESS){
        int row = item->row();
        bool isIncome = ui->tabWidget->currentIndex() == 0;
        QList<CurInvoiceRecord*> *rs = isIncome?incomes:costs;
        CurInvoiceRecord* r = item->tableWidget()->item(row,TI_INUMBER)->data(Qt::UserRole).value<CurInvoiceRecord*>();
        BusiAction* ba = r->ba;
        PingZheng* pz=0;
        if(ba)
            pz = ba->getParent();
        if(pz && ba)
            emit openRelatedPz(pz->id(),ba->getId());
    }
}

void CurInvoiceStatForm::clientNameChanged(QString oldName, QString newName)
{
    QList<CurInvoiceRecord *> *rs;
    QTableWidget* tw;
    if(ui->tabWidget->currentIndex() == 0){
        rs = incomes;
        tw = ui->twIncome;
    }
    else{
        rs = costs;
        tw = ui->twCost;
    }
    for(int i = 0; i < rs->count(); ++i){
        CurInvoiceRecord *r = rs->at(i);
        if(r->client == oldName){
            r->client = newName;
            r->tags->setBit(CI_TAG_CLIENT,true);
        }
    }
    for(int i = 0; i < tw->rowCount(); ++i){
        InvoiceClientItem* ti = static_cast<InvoiceClientItem*>(tw->item(i,TI_CLIENT));
        if(ti->text() == oldName)
            ti->setText(newName);
    }
}

void CurInvoiceStatForm::clientMatchChanged(QString clientName, SubjectNameItem *ni)
{
    QList<CurInvoiceRecord *> *rs;
    QTableWidget* tw;
    if(ui->tabWidget->currentIndex() == 0){
        rs = incomes;
        tw = ui->twIncome;
    }
    else{
        rs = costs;
        tw = ui->twCost;
    }
    if(ni->getLongName() != clientName)
        ni->addAlias(new NameItemAlias(ni->getShortName(),clientName,ni->getRemCode(),QDateTime::currentDateTime()));
    for(int i = 0; i < rs->count(); ++i){
        CurInvoiceRecord *r = rs->at(i);
        if(r->client == clientName){
            r->ni = ni;
            r->tags->setBit(CI_TAG_NAMEITEM,true);
        }
    }
    for(int i = 0; i < tw->rowCount(); ++i){
        InvoiceClientItem* ti = static_cast<InvoiceClientItem*>(tw->item(i,TI_CLIENT));
        if(ti->text() == clientName)
            ti->setNameItem(ni);
    }
}

void CurInvoiceStatForm::createNewClientAlias(NameItemAlias *alias)
{
    QList<CurInvoiceRecord *> *rs;
    QTableWidget* tw;
    if(ui->tabWidget->currentIndex() == 0){
        rs = incomes;
        tw = ui->twIncome;
    }
    else{
        rs = costs;
        tw = ui->twCost;
    }
    SubjectNameItem* temNi = 0;
    for(int i = 0; i < rs->count(); ++i){
        CurInvoiceRecord *r = rs->at(i);
        if(r->client == alias->longName()){
            if(!temNi)
                temNi = new SubjectNameItem(0,clientClsId,alias->shortName(),alias->longName(),alias->rememberCode(),QDateTime::currentDateTime(),curUser);
            r->alias = alias;
            r->ni = temNi;
            r->tags->setBit(CI_TAG_NAMEITEM,true);
        }
    }
    for(int i = 0; i < tw->rowCount(); ++i){
        InvoiceClientItem* ti = static_cast<InvoiceClientItem*>(tw->item(i,TI_CLIENT));
        if(ti->text() == alias->longName())
            ti->setNameItem(temNi);
    }
    SubjectManager::addIsolatedAlias(alias);
}

/**
 * @brief 当前选择显示收入或成本表格
 * @param index
 */
void CurInvoiceStatForm::curTableChanged(int index)
{
    TableSortColumn col;
    if(index == 0)
        col = sort_in;
    else
        col = sort_cost;
    if(col == TSC_PRIMARY && ui->rdoPrimary->isChecked() ||
       col == TSC_NUMBER && ui->rdoNumber->isChecked() ||
       col == TSC_INUMBER && ui->rdoINum->isChecked() ||
       col == TSC_NAME && ui->rdoName->isChecked())
        return;
    switchSortChanged(false);
    switch(col){
    case TSC_PRIMARY:
        ui->rdoPrimary->setChecked(true);
        break;
    case TSC_NUMBER:
        ui->rdoNumber->setChecked(true);
        break;
    case TSC_INUMBER:
        ui->rdoINum->setChecked(true);
        break;
    case TSC_NAME:
        ui->rdoName->setChecked(true);
        break;
    }
    switchSortChanged();
}

/**
 * @brief 排序列的选择改变，重新排序后显示表格
 * @param on
 */
void CurInvoiceStatForm::sortColumnChanged(bool on)
{
    if(!on)
        return;
    QRadioButton* rb = qobject_cast<QRadioButton*>(sender());
    if(!rb)
        return;
    QTableWidget* tw = ui->tabWidget->currentIndex()==0?ui->twIncome:ui->twCost;
    TableSortColumn* col;
    if(ui->tabWidget->currentIndex() == 0)
        col = &sort_in;
    else
        col = &sort_cost;
    if(rb == ui->rdoPrimary && *col != TSC_PRIMARY){
        *col = TSC_PRIMARY;
        tw->sortByColumn(TI_SORT_PRIMARY,Qt::AscendingOrder);
    }
    else if(rb == ui->rdoNumber && *col != TSC_NUMBER){
        *col = TSC_NUMBER;
        tw->sortByColumn(TI_SORT_NUM,Qt::AscendingOrder);
    }
    else if(rb == ui->rdoINum && *col != TSC_INUMBER){
        *col = TSC_INUMBER;
        tw->sortByColumn(TI_INUMBER,Qt::AscendingOrder);
    }
    else if(rb == ui->rdoName && *col != TSC_NAME){
        *col = TSC_NAME;
        tw->sortByColumn(TI_CLIENT,Qt::AscendingOrder);
    }
}

/**
 * @brief 启用或禁用过滤器
 * @param on
 */
void CurInvoiceStatForm::enanbleFilter(bool on)
{
    if(on){
        connect(ui->edtFltText,SIGNAL(textChanged(QString)),this,SLOT(filteTextChanged(QString)));
        filteTextChanged(ui->edtFltText->text());
    }
    else{
        disconnect(ui->edtFltText,SIGNAL(textChanged(QString)),this,SLOT(filteTextChanged(QString)));
        for(int i = 0; i<ui->twIncome->rowCount(); ++i){
            if(ui->twIncome->isRowHidden(i))
                ui->twIncome->setRowHidden(i,false);
        }
        for(int i = 0; i<ui->twCost->rowCount(); ++i){
            if(ui->twCost->isRowHidden(i))
                ui->twCost->setRowHidden(i,false);
        }
    }
}

void CurInvoiceStatForm::filteTextChanged(QString text)
{
    QTableWidget* tw = ui->tabWidget->currentIndex()==0?ui->twIncome:ui->twCost;
    if(text.isEmpty()){
        for(int i = 0; i<tw->rowCount(); ++i){
            if(tw->isRowHidden(i))
                tw->setRowHidden(i,false);
        }
        return;
    }
    if(ui->rdoFltINum->isChecked()){
        for(int i = 0;i<tw->rowCount();++i)
            tw->setRowHidden(i,!tw->item(i,TI_INUMBER)->text().startsWith(text));
    }
    else{
        for(int i = 0;i<tw->rowCount();++i)
            tw->setRowHidden(i,!tw->item(i,TI_CLIENT)->text().contains(text));
    }
}

void CurInvoiceStatForm::init()
{    
    initKeys();
    icon_income = QIcon(":/images/invoiceType/i_income.png");
    icon_cost = QIcon(":/images/invoiceType/i_cost.png");
    invoiceStates[1] = QObject::tr("正常");
    invoiceStates[2] = QObject::tr("冲红");
    invoiceStates[3] = QObject::tr("作废");
    invoicesClasses[0] = QObject::tr("普票");
    invoicesClasses[1] = QObject::tr("专票");
    invoiceStateColors[1] = QColor(Qt::darkGreen);
    invoiceStateColors[2] = QColor(Qt::red);
    invoiceStateColors[3] = QColor(Qt::gray);
    invoiceClassColors[0] = QColor();
    invoiceClassColors[1] = QColor(Qt::magenta);
    mnuColTypes = new QMenu(this);
    mnuRowTypes = new QMenu(this);
    ag1 = new QActionGroup(this);
    ag2 = new QActionGroup(this);
    ag3 = new QActionGroup(this);
    ag1->addAction(ui->actNone);
    ag1->addAction(ui->actNum);
    ag1->addAction(ui->actDate);
    ag1->addAction(ui->actInvoice);
    ag1->addAction(ui->actClient);
    ag1->addAction(ui->actMoney);
    ag1->addAction(ui->actTaxMoney);
    ag1->addAction(ui->actWbMoney);
    ag1->addAction(ui->actsfInfo);
    ag1->addAction(ui->actInvoiceCls);
    ag1->addAction(ui->actResolveCol);
    mnuColTypes->addActions(ag1->actions());
    ag2->addAction(ui->actCommRow);
    ag2->addAction(ui->actStartRow);
    ag2->addAction(ui->actEndRow);
    mnuRowTypes->addActions(ag2->actions());
    ag3->addAction(ui->actYS);
    ag3->addAction(ui->actYF);

    switchHActions();
    switchVActions();
    for(int i = ui->stackedWidget->count()-1; i >= 0; i--){
        QWidget* w = ui->stackedWidget->widget(i);
        ui->stackedWidget->removeWidget(w);
    }
    expandPreview(false);
    connect(ui->lwSheets,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(sheetListContextMeny(QPoint)));
    connect(ui->lwSheets,SIGNAL(currentRowChanged(int)),this,SLOT(curSheetChanged(int)));
    connect(ui->lwSheets,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(doubleItemReadSheet(QListWidgetItem*)));
    connect(ui->actYS,SIGNAL(toggled(bool)),this,SLOT(processYsYfSelected(bool)));
    connect(ui->actYF,SIGNAL(toggled(bool)),this,SLOT(processYsYfSelected(bool)));
    initTable();
    connect(ui->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(curTableChanged(int)));
    connect(ui->gbxFilter,SIGNAL(toggled(bool)),this,SLOT(enanbleFilter(bool)));
    //读取本地记录
    switchSortChanged();
    incomes = suiteMgr->getCurInvoiceRecords();
    costs = suiteMgr->getCurInvoiceRecords(false);
    if(!incomes->isEmpty()){
        ui->twIncome->setRowCount(incomes->count());
        for(int i = 0;i < incomes->count(); ++i)
            showInvoiceInfo(i,incomes->at(i));
    }
    if(!costs->isEmpty())    {
        ui->twCost->setRowCount(costs->count());
        for(int i = 0;i < costs->count(); ++i)
            showInvoiceInfo(i,costs->at(i));
    }
    switchInvoiceInfo();
    delegate = new InvoiceInfoDelegate(this);
    ui->twIncome->setItemDelegate(delegate);
    ui->twCost->setItemDelegate(delegate);
    ui->twIncome->addAction(ui->actClearInvoice);
    ui->twCost->addAction(ui->actClearInvoice);
    ui->twIncome->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->twCost->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->twIncome->horizontalHeader(),SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(InvoiceTableHeaderContextMenu(QPoint)));
    connect(ui->twCost->horizontalHeader(),SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(InvoiceTableHeaderContextMenu(QPoint)));
    connect(ui->twIncome,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(itemDoubleClicked(QTableWidgetItem*)));
    connect(ui->twCost,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(itemDoubleClicked(QTableWidgetItem*)));
    clientClsId = AppConfig::getInstance()->getSpecNameItemCls(AppConfig::SNIC_COMMON_CLIENT);
}

/**
 * @brief 初始化敏感关键字所对应的列类型字典
 */
void CurInvoiceStatForm::initKeys()
{
    AppConfig* cfg = AppConfig::getInstance();
    colTitleKeys[CT_NUMBER] = cfg->getCurInvoiceColumnTitle(CT_NUMBER);
    colTitleKeys[CT_DATE] = cfg->getCurInvoiceColumnTitle(CT_DATE);
    colTitleKeys[CT_INVOICE] = cfg->getCurInvoiceColumnTitle(CT_INVOICE);
    colTitleKeys[CT_CLIENT] = cfg->getCurInvoiceColumnTitle(CT_CLIENT);
    colTitleKeys[CT_MONEY] = cfg->getCurInvoiceColumnTitle(CT_MONEY);
    colTitleKeys[CT_TAXMONEY] = cfg->getCurInvoiceColumnTitle(CT_TAXMONEY);
    colTitleKeys[CT_WBMONEY] = cfg->getCurInvoiceColumnTitle(CT_WBMONEY);
    colTitleKeys[CT_SFINFO] = cfg->getCurInvoiceColumnTitle(CT_SFINFO);
}

void CurInvoiceStatForm::initTable()
{
    QStringList titles;
    titles<<tr("序号")<<tr("开票日期")<<tr("发票号码")<<tr("发票金额")<<tr("税额")
          <<tr("外币金额")<<tr("发票类型")<<tr("发票属性")<<tr("收款情况")<<tr("处理情况")<<tr("客户名");
    ui->twIncome->setColumnCount(titles.count()+2);
    ui->twCost->setColumnCount(titles.count()+2);
    ui->twIncome->setHorizontalHeaderLabels(titles);
    ui->twCost->setHorizontalHeaderLabels(titles);
    ui->twIncome->setColumnWidth(TI_NUMBER,40);
    ui->twCost->setColumnWidth(TI_NUMBER,40);
    ui->twIncome->setColumnWidth(TI_DATE,100);
    ui->twCost->setColumnWidth(TI_DATE,100);
    ui->twIncome->setColumnWidth(TI_INUMBER,80);
    ui->twCost->setColumnWidth(TI_INUMBER,80);
    ui->twIncome->setColumnWidth(TI_MONEY,80);
    ui->twCost->setColumnWidth(TI_MONEY,80);
    ui->twIncome->setColumnWidth(TI_TAXMONEY,80);
    ui->twCost->setColumnWidth(TI_TAXMONEY,80);
    ui->twIncome->setColumnWidth(TI_WBMONEY,80);
    ui->twCost->setColumnWidth(TI_WBMONEY,80);
    ui->twIncome->setColumnWidth(TI_SFINFO,100);
    ui->twCost->setColumnWidth(TI_SFINFO,100);
    ui->twIncome->setColumnWidth(TI_ITYPE,60);
    ui->twCost->setColumnWidth(TI_ITYPE,60);
    ui->twIncome->setColumnWidth(TI_STATE,60);
    ui->twCost->setColumnWidth(TI_STATE,60);
    ui->twIncome->setColumnWidth(TI_ISPROCESS,80);
    ui->twCost->setColumnWidth(TI_ISPROCESS,80);
    ui->twIncome->setColumnHidden(TI_SORT_NUM,true);
    ui->twCost->setColumnHidden(TI_SORT_NUM,true);
    ui->twIncome->setColumnHidden(TI_SORT_PRIMARY,true);
    ui->twCost->setColumnHidden(TI_SORT_PRIMARY,true);
}

void CurInvoiceStatForm::switchHActions(bool on)
{
    if(on){
        connect(ui->actNone,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actNum,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actDate,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actInvoice,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actClient,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actMoney,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actTaxMoney,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actWbMoney,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actsfInfo,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        connect(ui->actInvoiceCls,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
    }
    else{
        disconnect(ui->actNone,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actNum,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actDate,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actInvoice,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actClient,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actMoney,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actTaxMoney,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actWbMoney,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actsfInfo,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
        disconnect(ui->actInvoiceCls,SIGNAL(toggled(bool)),this,SLOT(processColTypeSelected(bool)));
    }
}

void CurInvoiceStatForm::switchVActions(bool on)
{
    if(on){
        connect(ui->actCommRow,SIGNAL(toggled(bool)),this,SLOT(processRowTypeSelected(bool)));
        connect(ui->actStartRow,SIGNAL(toggled(bool)),this,SLOT(processRowTypeSelected(bool)));
        connect(ui->actEndRow,SIGNAL(toggled(bool)),this,SLOT(processRowTypeSelected(bool)));
    }
    else{
        disconnect(ui->actCommRow,SIGNAL(toggled(bool)),this,SLOT(processRowTypeSelected(bool)));
        disconnect(ui->actStartRow,SIGNAL(toggled(bool)),this,SLOT(processRowTypeSelected(bool)));
        disconnect(ui->actEndRow,SIGNAL(toggled(bool)),this,SLOT(processRowTypeSelected(bool)));
    }
}

void CurInvoiceStatForm::switchSortChanged(bool on)
{
    if(on){
        connect(ui->rdoPrimary,SIGNAL(toggled(bool)),this,SLOT(sortColumnChanged(bool)));
        connect(ui->rdoNumber,SIGNAL(toggled(bool)),this,SLOT(sortColumnChanged(bool)));
        connect(ui->rdoINum,SIGNAL(toggled(bool)),this,SLOT(sortColumnChanged(bool)));
        connect(ui->rdoName,SIGNAL(toggled(bool)),this,SLOT(sortColumnChanged(bool)));
    }
    else{
        disconnect(ui->rdoPrimary,SIGNAL(toggled(bool)),this,SLOT(sortColumnChanged(bool)));
        disconnect(ui->rdoNumber,SIGNAL(toggled(bool)),this,SLOT(sortColumnChanged(bool)));
        disconnect(ui->rdoINum,SIGNAL(toggled(bool)),this,SLOT(sortColumnChanged(bool)));
        disconnect(ui->rdoName,SIGNAL(toggled(bool)),this,SLOT(sortColumnChanged(bool)));
    }
}

void CurInvoiceStatForm::switchInvoiceInfo(bool on)
{
    if(on){
        connect(ui->twIncome,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(invoiceInfoChanged(QTableWidgetItem*)));
        connect(ui->twCost,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(invoiceInfoChanged(QTableWidgetItem*)));
    }
    else{
        disconnect(ui->twIncome,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(invoiceInfoChanged(QTableWidgetItem*)));
        disconnect(ui->twCost,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(invoiceInfoChanged(QTableWidgetItem*)));
    }
}

/**
 * @brief 重置有指定列属性的表头数据项为普通列
 * @param colType
 * @param tw
 */
void CurInvoiceStatForm::resetTableHeadItem(CurInvoiceColumnType colType, QTableWidget *tw)
{
    for(int i = 0; i < tw->columnCount(); ++i){
        QTableWidgetItem* ti = tw->horizontalHeaderItem(i);
        if(!ti)
            continue;
        CurInvoiceColumnType type = (CurInvoiceColumnType)ti->data(Qt::UserRole).toInt();
        if(type == colType){
            ti->setText(QString::number(i+1));
            ti->setData(Qt::UserRole,CT_NONE);
            return;
        }
    }
}

void CurInvoiceStatForm::expandPreview(bool on)
{
    if(on){
        ui->btnExpand->setText(tr("收缩"));
        ui->btnExpand->setIcon(QIcon(":/images/arrow-up"));
        ui->gbxImport->setFixedHeight(400);
    }
    else{
        ui->btnExpand->setText(tr("展开"));        
        ui->btnExpand->setIcon(QIcon(":/images/arrow-down"));
        ui->gbxImport->setFixedHeight(100);
    }
    ui->lblFileds->setVisible(on);
    ui->lwSheets->setVisible(on);
    ui->lblSheets->setVisible(on);
    ui->stackedWidget->setVisible(on);
}

bool CurInvoiceStatForm::readSheet(int index, QTableWidget *tw)
{
    QString sheetName = ui->lwSheets->item(index)->text();
    excel->selectSheet(sheetName);
    Worksheet* sheet = excel->currentWorksheet();
    CellRange range = sheet->dimension();
    int firstRow = range.firstRow();
    int lastRow = range.lastRow();
    int firstCol = range.firstColumn();
    int lastCol = range.lastColumn();

    QList<CellRange> mergedCells = sheet->mergedCells();
    //查找“发票号码”列所在单元格
    int r,c;
    for(r = firstRow; r <= lastRow; ++r){
        bool fonded = false;
        for(c = firstCol; c <= lastCol; ++c){
            QString str = sheet->read(r,c).toString().trimmed();
            if(str == tr("发票号码")){
                fonded = true;
                break;
            }
        }
        if(fonded)
            break;
    }
    if(r > lastRow && c > lastCol){
        myHelper::ShowMessageBoxWarning(tr("无法定位到“发票号码”所在单元格，请确认表格是否符合规定！"));
        return false;
    }
    //确定标题行跨越的行数
    int titleSpanRows = 1;
    foreach(CellRange rg, mergedCells){
        if(r>=rg.firstRow() && r <= rg.lastRow() && c>=rg.firstColumn() && c<=rg.lastColumn()){
            titleSpanRows += rg.rowCount()-1;
            break;
        }
    }
    QListWidgetItem* li = ui->lwSheets->currentItem();
    int startRow = r+titleSpanRows-1;   //数据开始行-基于0
    int endRow = -1;                    //数据结束行
    li->setData(DR_STARTROW,startRow);
    li->setData(DR_ENDROW,endRow);

    tw->clear();
    tw->setRowCount(range.rowCount());
    tw->setColumnCount(range.columnCount());
    QDate primaryDay(1900,1,1);
    qint64 pdInt = primaryDay.toJulianDay()-2; //日期基数（Excel中的日期的内部是用一个数字，即从1900年1月1日开始到此日期的天数）
    //读取所有合并过的单元格
    foreach(CellRange rg, mergedCells){
        tw->setSpan(rg.firstRow()-1,rg.firstColumn()-1,rg.rowCount(),rg.columnCount());
        CellReference tlCell = rg.topLeft();
        QString v = sheet->read(tlCell).toString().trimmed();
        tw->setItem(tlCell.row()-1,tlCell.column()-1,new QTableWidgetItem(v));
        //跨越行的标题
        if(rg.columnCount()==1){
            CurInvoiceColumnType colType = columnTypeForText(v);
            if(colType != CT_NONE){
                QTableWidgetItem* hi = new QTableWidgetItem(getColTypeText(colType));
                hi->setData(Qt::UserRole,colType);
                tw->setHorizontalHeaderItem(rg.firstColumn()-1,hi);
            }
        }
    }
    //读取其他标题区域未合并过的单元格
    for(int i = 1; i <= startRow+1; ++i){
        for(int j = 1; j <= tw->colorCount(); ++j){
            if(tw->rowSpan(i-1,j-1)>1 || tw->columnSpan(i-1,j-1) > 1)
                continue;
            QString v = sheet->read(i,j).toString();
            CurInvoiceColumnType colType = columnTypeForText(v);
            if(colType != CT_NONE){
                QTableWidgetItem* hi = new QTableWidgetItem(getColTypeText(colType));
                hi->setData(Qt::UserRole,colType);
                tw->setHorizontalHeaderItem(j-1,hi);
            }
            tw->setItem(i-1,j-1,new QTableWidgetItem(v));
        }
    }

    //如果标题行未跨越行，则再次根据标题的名称确定列类型
    if(titleSpanRows==1){
        for(int i = 0; i < tw->columnCount(); ++i){
            QTableWidgetItem* hi = tw->item(startRow-1,i);
            if(!hi)
                continue;
            CurInvoiceColumnType colType = columnTypeForText(hi->text());
            if(colType != CT_NONE){
                hi = new QTableWidgetItem(getColTypeText(colType));
                hi->setData(Qt::UserRole,colType);
                tw->setHorizontalHeaderItem(i,hi);
            }
        }
    }

    int dateCol = -1;   //确定日期列
    for(int col=0; col < tw->columnCount(); ++col){
        QTableWidgetItem* hi = tw->horizontalHeaderItem(col);
        if(!hi)
            continue;
        CurInvoiceColumnType coltype = (CurInvoiceColumnType)hi->data(Qt::UserRole).toInt();
        if(coltype == CT_DATE){
            dateCol = col+1;
            break;
        }
    }
    QList<int> formulaCols;
    QRegExp re("^[0-9]{8}$");
    for(int i = startRow+1; i <= tw->rowCount(); ++i){
        for(int j = 1; j <= tw->columnCount(); ++j){
            if(tw->rowSpan(i-1,j-1)>1 || tw->columnSpan(i-1,j-1) > 1)
                continue;
            Cell* cell = sheet->cellAt(i,j);
            if(!cell)
                continue;
            QString v;
            if(dateCol != -1 && j == dateCol){
                bool ok=false;
                qint64 dv = cell->value().toLongLong(&ok);
                if(ok && dv != 0){
                    dv += pdInt;
                    QDate d = QDate::fromJulianDay(dv);
                    v = d.toString(Qt::ISODate);
                }
                else
                    v = cell->value().toString();
            }
            else
                v = sheet->read(i,j).toString().trimmed();
            if(endRow == -1 && j == c && re.indexIn(v) == -1) //第一个空的或无效的发票号行的上一行作为结束行
                endRow = i-2;
            if(cell->dataType() == Cell::Formula && endRow == -1
                    && !formulaCols.contains(j)){
                formulaCols<<j;
            }
            if(v.isEmpty())
                continue;
            tw->setItem(i-1,j-1,new QTableWidgetItem(v));
        }
    }
    if(startRow != -1){
        QTableWidgetItem* ti =  new QTableWidgetItem(QString("%1+").arg(startRow+1));
        ti->setForeground(QBrush(QColor("red")));
        ti->setToolTip(getRowTypeText(RT_START));
        tw->setVerticalHeaderItem(startRow,ti);
    }
    if(endRow != -1){
        QTableWidgetItem* ti = new QTableWidgetItem(QString("%1-").arg(endRow+1));
        ti->setForeground(QBrush(QColor("red")));
        ti->setToolTip(getRowTypeText(RT_END));
        tw->setVerticalHeaderItem(endRow,ti);
        li->setData(DR_ENDROW,endRow);
    }
    //计算公式
    if(!formulaCols.isEmpty()){
        foreach(int col, formulaCols){
            for(int row = startRow; row <= endRow; ++row){
                QTableWidgetItem* ti = tw->item(row,col-1);
                if(!ti || ti->text().isEmpty())
                    continue;
                Double dv;
                if(!calFormula(ti->text(),dv,tw)){
                    ti->setForeground(QBrush(QColor("red")));
                }
                else{
                    ti->setText(dv.toString());
                }
            }
        }
    }
    return true;
}

/**
 * @brief 返回指定的列标题文本所对应的特定列类型
 * @param text
 * @return
 */
CurInvoiceColumnType CurInvoiceStatForm::columnTypeForText(QString text)
{
    if(colTitleKeys.isEmpty() || text.isEmpty())
        return CT_NONE;
    QHashIterator<CurInvoiceColumnType,QStringList> it(colTitleKeys);
    while(it.hasNext()){
        it.next();
        if(it.value().contains(text))
            return it.key();
    }
    return CT_NONE;
}

/**
 * @brief 计算公式的值
 * @param formula
 * @param v
 * @return 如果公式可以求值，则返回true，否则返回false
 */
bool CurInvoiceStatForm::calFormula(QString formula, Double &v, QTableWidget* tw)
{
    QRegExp re1("=(SUM)\\(([A-Z])(\\d{1,3}):([A-Z])(\\d{1,3})\\)");
    int pos = re1.indexIn(formula);
    int lc,lr,rc,rr;
    if(pos == 0){
        lc = re1.cap(2).at(0).toLatin1() - 'A';
        lr = re1.cap(3).toInt();
        rc = re1.cap(4).at(0).toLatin1() - 'A';
        rr = re1.cap(5).toInt();
        if(lr != rr)
            return false;
        if(re1.captureCount() != 5)
            return false;
        if(re1.cap(1) != "SUM")
            return false;
    }
    else{
        QRegExp re2("=([A-Z])(\\d{1,3})\\+([A-Z])(\\d{1,3})");
        pos = re2.indexIn(formula);
        if(pos == -1)
            return false;
        lc = re2.cap(1).at(0).toLatin1() - 'A';
        lr = re2.cap(2).toInt();
        rc = re2.cap(3).at(0).toLatin1() - 'A';
        rr = re2.cap(4).toInt();
    }
    lr--;
    Double d1,d2;
    bool ok = false;
    QTableWidgetItem* ti = tw->item(lr,lc);
    if(ti){
        if(ti->text().isEmpty())
            d1 = 0.0;
        else{
            d1 = ti->text().toDouble(&ok);
            if(!ok)
                return false;
        }
    }
    else
        d1 = 0.0;
    ti = tw->item(lr,rc);
    if(ti){
        if(ti->text().isEmpty())
            d2 = 0.0;
        else{
            d2 = ti->text().toDouble(&ok);
            if(!ok)
                return false;
        }
    }
    else
        d2 = 0.0;
    v = d1+d2;
    return true;
}

/**
* @brief 检查表格是否设置了必要的列类型
* @param colTypes
* @param info
* @return
*/
bool CurInvoiceStatForm::inspectColTypeSet(QList<CurInvoiceColumnType> colTypes, QString &info)
{
    QSet<CurInvoiceColumnType> types;
    types.insert(CT_INVOICE);
    types.insert(CT_CLIENT);
    types.insert(CT_MONEY);
    foreach (CurInvoiceColumnType type, colTypes) {
        types.remove(type);
    }
    if(!types.isEmpty()){
        foreach(CurInvoiceColumnType type, types)
            info.append(getColTypeText(type)+"\n");
        return false;
    }
    return true;
}

/**
 * @brief 数据校验
 * 日期是否有效，序号是否连续、金额数据是否有效、普票/专票设置、客户适配
 * @param isYs
 * @return
 */
bool CurInvoiceStatForm::inspectTableData(bool isYs)
{
    QList<CurInvoiceRecord *> *rs;
    QTableWidget* t=0;
    if(isYs){
        rs = incomes;
        t = ui->twIncome;
    }
    else{
        rs = costs;
        t = ui->twCost;
    }
    QStringList invoices;
    CurInvoiceRecord* r;
    //校验序号的连续性
    for(int i = 0; i < rs->count(); ++i){
        r = rs->at(i);
        if(r->num == 0){
            myHelper::ShowMessageBoxError(tr("第 %1 行出现空缺的序号！").arg(i+1));
            return false;
        }
        if(r->num != i+1){
            myHelper::ShowMessageBoxError(tr("第 %1 行出现不连续的序号！").arg(i+1));
            return false;
        }
        //发票金额是否缺失
        if(r->money == 0){
            myHelper::ShowMessageBoxError(tr("第 %1 行未设置发票金额！").arg(i+1));
            return false;
        }
        if(r->inum.isEmpty()){
            myHelper::ShowMessageBoxError(tr("第 %1 行发票号码异常！").arg(i+1));
            return false;
        }
        invoices<<r->inum;
    }
    //号码重复性检测
    if(rs->count() != invoices.count()){
        QStringList repeats;
        while(!invoices.isEmpty()){
            QString inum = invoices.takeFirst();
            if(invoices.contains(inum))
                repeats<<inum;
        }
        QString inums;
        foreach(QString num, repeats)
            inums.append(num+"\n");
        myHelper::ShowMessageBoxError(tr("存在重复发票号码！%1").arg(inums));
        return false;
    }
    return true;
}

/**
 * @brief 在导入表的指定行显示发票信息
 * @param row
 * @param r
 */
void CurInvoiceStatForm::showInvoiceInfo(int row, CurInvoiceRecord *r)
{
    QTableWidget* tw = r->isIncome?ui->twIncome:ui->twCost;
    tw->setItem(row,TI_NUMBER,new QTableWidgetItem(QString::number(r->num)));
    QTableWidgetItem* ti = new QTableWidgetItem(r->inum);
    QVariant v; v.setValue<CurInvoiceRecord*>(r);
    ti->setData(Qt::UserRole,v);
    tw->setItem(row,TI_INUMBER,ti);
    QString ds;
    if(r->date.isValid())
        ds =r->date.toString(Qt::ISODate);
    else
        ds = r->dateStr;
    tw->setItem(row,TI_DATE,new QTableWidgetItem(ds));
    tw->setItem(row,TI_MONEY,new QTableWidgetItem(r->money.toString()));
    tw->setItem(row,TI_TAXMONEY,new QTableWidgetItem(r->taxMoney.toString()));
    tw->setItem(row,TI_WBMONEY,new QTableWidgetItem(r->wbMoney.toString()));
    InvoiceClientItem* cItem = new InvoiceClientItem(r->client);
    cItem->setNameItem(r->ni);
    tw->setItem(row,TI_CLIENT,cItem);
    MultiStateItem* mi = new MultiStateItem(invoicesClasses,false);
    mi->setData(Qt::EditRole,r->type?1:0);
    mi->setStateColors(invoiceClassColors);
    tw->setItem(row,TI_ITYPE,mi);
    mi = new MultiStateItem(invoiceStates);
    mi->setData(Qt::EditRole,r->state);
    mi->setStateColors(invoiceStateColors);
    tw->setItem(row,TI_STATE,mi);
    tw->setItem(row,TI_SFINFO,new QTableWidgetItem(r->sfInfo));
    InvoiceProcessItem* pi = new InvoiceProcessItem(r->processState);
    pi->setErrorInfos(r->errors);
    tw->setItem(row,TI_ISPROCESS, pi);
    tw->setItem(row,TI_SORT_NUM,new QTableWidgetItem(padZero(r->num)));
    tw->setItem(row,TI_SORT_PRIMARY,new QTableWidgetItem(padZero(row)));
}

/**
 * @brief 系统默认表格列的排序是按文本顺序，因此对整数必须填充前导0
 * @param num
 * @return
 */
QString CurInvoiceStatForm::padZero(int num)
{
    if(num < 10)
        return "000" + QString::number(num);
    else if(num >= 10 && num < 100)
        return "00" + QString::number(num);
    else if(num >= 100 && num < 1000)
        return "0" + QString::number(num);
    else
        return QString::number(num);
}

void CurInvoiceStatForm::on_btnExpand_toggled(bool checked)
{
    expandPreview(checked);
}

//从缓存表中导入
void CurInvoiceStatForm::on_btnImport_clicked()
{
    //先决条件：正确设置了开始行、结束行、应收/应付属性、必要的列
    QString errors;
    QListWidgetItem* li = ui->lwSheets->currentItem();
    if(!li){
        myHelper::ShowMessageBoxWarning(tr("还未指定要导入哪个表格！"));
        return;
    }
    int sr = li->data(DR_STARTROW).toInt();
    int er = li->data(DR_ENDROW).toInt();
    InvoiceType icType = (InvoiceType)li->data(DR_ITYPE).toInt();
    if(sr == -1)
        errors = tr("未设置表格数据开始行；");
    if(er == -1)
        errors.append(tr("\n未设置表格数据结束行；"));
    if(icType == IT_NONE)
        errors.append(tr("\n未指定是收入/成本发票！"));
    if(!errors.isEmpty()){
        myHelper::ShowMessageBoxWarning(errors);
        return;
    }

    QList<CurInvoiceColumnType> colTypes;
    int invoiceColumn = -1;  //此行新增
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->widget(ui->lwSheets->currentRow()));
    for(int col = 0; col < tw->columnCount(); ++col){
        QTableWidgetItem* hi = tw->horizontalHeaderItem(col);
        if(!hi){
            colTypes<<CT_NONE;
            continue;
        }
        CurInvoiceColumnType colType = (CurInvoiceColumnType)hi->data(Qt::UserRole).toInt();
        if(colTypes.contains(colType)){
            myHelper::ShowMessageBoxWarning(tr("关键列类型有重复，请再次确认！"));
            return;
        }
        if(colType == CT_INVOICE)
            invoiceColumn = col;
        colTypes<<colType;
    }
    errors.clear();
    if(!inspectColTypeSet(colTypes,errors)){
        myHelper::ShowMessageBoxWarning(errors);
        return;
    }
    //检测是否缺必填项（发票号）
    QRegExp re("^\\d{8}$");
    for(int r = sr; r <= er; ++r){
        QString inum = tw->item(r,invoiceColumn)->text().trimmed();
        if(inum.isEmpty() || re.indexIn(inum) == -1){
            errors.append(tr("第%1行的发票号有误！\n").arg(r+1));
        }
    }
    if(!errors.isEmpty()){
        myHelper::ShowMessageBoxWarning(errors);
        return;
    }
    QTableWidget* t = 0;
    QList<CurInvoiceRecord *> *rs;
    if(icType == IT_INCOME){
        t = ui->twIncome;
        rs = incomes;
    }
    else{
        t = ui->twCost;
        rs = costs;
    }
    if(!rs->isEmpty()){
        QDialog dlg(this);
        QLabel title(tr("表格内已存在数据行，您可选择如下导入方式："),&dlg);
        QRadioButton rdoAppend(tr("追加导入"),&dlg);
        rdoAppend.setChecked(true);
        QRadioButton rdoClear(tr("清空后导入"),&dlg);
        QPushButton btnOk(QIcon(":/images/btn_ok.png"),tr("确定"),&dlg);
        QPushButton btnCancel(QIcon(":/images/btn_close.png"),tr("取消"),&dlg);
        connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
        connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
        QHBoxLayout lb; lb.addStretch();lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
        QVBoxLayout* lm = new QVBoxLayout;
        lm->addWidget(&title);
        lm->addWidget(&rdoAppend);
        lm->addWidget(&rdoClear);
        lm->addLayout(&lb);
        dlg.setLayout(lm);
        dlg.resize(300,200);
        if(dlg.exec() == QDialog::Rejected)
            return;
        if(rdoClear.isChecked())
            on_actClearInvoice_triggered();
    }

    int y = suiteMgr->year();
    int m = suiteMgr->month();
    int num = rs->count() + 1;
    for(int r = sr; r <= er; ++r,num++){
        CurInvoiceRecord* rc = new CurInvoiceRecord;
        rc->y=y;rc->m=m;
        rc->isIncome = (icType == IT_INCOME);
        rc->num = num;
        bool isCommon = false;
        for(int c = 0; c < tw->columnCount(); ++c){
            if(!tw->item(r,c))
                continue;
            CurInvoiceColumnType ct = colTypes.at(c);
            if(ct == CT_NONE)
                continue;
            switch(ct){
            case CT_NUMBER:
                rc->num = tw->item(r,c)->text().toInt();
                break;
            case CT_DATE:
                rc->date = QDate::fromString(tw->item(r,c)->text(),Qt::ISODate);
                if(!rc->date.isValid())
                    rc->dateStr = tw->item(r,c)->text();
                break;
            case CT_INVOICE:
                rc->inum = tw->item(r,c)->text();
                break;
            case CT_CLIENT:
                rc->client = tw->item(r,c)->text();
                break;
            case CT_MONEY:
                rc->money = tw->item(r,c)->text().toDouble();
                if(rc->money == 0)
                    myHelper::ShowMessageBoxWarning(tr("发票号“%1”未设置金额！").arg(rc->inum));
                break;
            case CT_TAXMONEY:
                rc->taxMoney = tw->item(r,c)->text().toDouble();
                break;
            case CT_WBMONEY:
                rc->wbMoney = tw->item(r,c)->text().toDouble();
                break;
            case CT_SFINFO:
                rc->sfInfo = tw->item(r,c)->text();
                if(rc->sfInfo.contains(tr("普票")))
                    isCommon = true;
            case CT_ICLASS:{
                QTableWidgetItem* ti = tw->item(r,c);
                if(ti && ti->text().contains(tr("普票")))
                    isCommon = true;}
            }
        }
        if(isCommon)
            rc->type = false;
        else
            rc->type = (rc->taxMoney != 0);
        rs->append(rc);
    }    
    //因为导入后是以原始顺序显示的，因此必须将排序方式复位，且禁用过滤
    if(!ui->rdoPrimary->isChecked()){
        switchSortChanged(false);
        ui->rdoPrimary->setChecked(true);
        switchSortChanged();
    }
    if(ui->gbxFilter->isChecked())
        ui->gbxFilter->setChecked(false);
    t->setRowCount(rs->count());
    switchInvoiceInfo(false);
    for(int i = 0; i < rs->count(); ++i)
        showInvoiceInfo(i,rs->at(i));
    switchInvoiceInfo();
 }

void CurInvoiceStatForm::on_btnBrowser_clicked()
{
    QString dirName = QApplication::applicationDirPath()+"/files/"+account->getSName()+"/";
    QDir d(dirName);
    if(!d.exists())
        d.mkpath(dirName);
    QString fileName = QFileDialog::getOpenFileName(this,tr("请选择作为应收应付发票数据源的Excel文件！"),dirName,"*.xlsx");
    if(fileName.isEmpty())
        return;
    ui->edtFilename->setText(fileName);
    if(excel){
        delete excel;
        excel = 0;
    }
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
    excel = new QXlsx::Document(fileName,this);
    ui->lwSheets->clear();
    for(int i = ui->stackedWidget->count()-1; i >= 0; i--){
        QWidget* w = ui->stackedWidget->widget(i);
        ui->stackedWidget->removeWidget(w);
    }
    QStringList sheets = excel->sheetNames();
    if(sheets.isEmpty())
        return;
    for(int i = 0; i < sheets.count(); ++i){
        QListWidgetItem* li = new QListWidgetItem(sheets.at(i),ui->lwSheets);
        li->setData(DR_STARTROW,-1);
        li->setData(DR_ENDROW,-1);
        li->setData(DR_ITYPE,IT_NONE);
        li->setData(DR_READED,false);  //表单未装载标记
        QTableWidget* tw = new QTableWidget(this);
        tw->setSelectionBehavior(QAbstractItemView::SelectRows);
        tw->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        tw->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tw->horizontalHeader(),SIGNAL(customContextMenuRequested(QPoint)),
                this,SLOT(tableHHeaderContextMenu(QPoint)));
        connect(tw->verticalHeader(),SIGNAL(customContextMenuRequested(QPoint)),
                this,SLOT(tableVHeaderContextMenu(QPoint)));
        ui->stackedWidget->addWidget(tw);
    }
    qApp->restoreOverrideCursor();
}

void CurInvoiceStatForm::on_btnSave_clicked()
{
    if(!suiteMgr->saveInCost())
        myHelper::ShowMessageBoxError(tr("保存操作出错！"));
}

/**
 * @brief 清空本地已导入的发票
 */
void CurInvoiceStatForm::on_actClearInvoice_triggered()
{
    int scope = (ui->tabWidget->currentIndex() == 0)?1:2;
    if(!suiteMgr->clearInCost(scope)){
        myHelper::ShowMessageBoxError(tr("清空操作发生错误！"));
        return;
    }
    if(scope == 1)
        ui->twIncome->setRowCount(0);
    else if(scope == 2 || scope == 0)
        ui->twCost->setRowCount(0);
}

/**
 * @brief 执行客户的自动匹配
 */
void CurInvoiceStatForm::on_actAutoMatch_triggered()
{
    QList<CurInvoiceRecord *> *rs;
    QTableWidget* t;
    if(ui->tabWidget->currentIndex() == 0){
        rs = incomes;
        t = ui->twIncome;
    }
    else{
        rs = costs;
        t = ui->twCost;
    }
    QHash<QString, SubjectNameItem*> matchedHashs;  //已找到匹配的缓存
    QList<SubjectNameItem*> nameItems;              //系统所有的名称对象
    nameItems = sm->getAllNameItems(SORTMODE_NAME);
    QList<NameItemAlias *> isolatedAlias;           //系统所有的孤立别名（由前几次匹配的新客户产生）
    isolatedAlias = sm->getAllIsolatedAlias();

    for(int i = 0; i < t->rowCount(); ++i){
        if(t->isRowHidden(i))
            continue;
        CurInvoiceRecord* r = t->item(i,TI_INUMBER)->data(Qt::UserRole).value<CurInvoiceRecord*>();
        if(r->ni)
            continue;
        InvoiceClientItem* ti = static_cast<InvoiceClientItem*>(t->item(i,TI_CLIENT)) ;
        SubjectNameItem* ni = matchedHashs.value(r->client);
        if(ni){
            r->ni = ni;
            r->tags->setBit(CI_TAG_NAMEITEM,true);
            ti->setNameItem(ni);
            continue;
        }
        foreach(SubjectNameItem* n, nameItems){
            if(n->matchName(r->client) == 0)
                continue;
            r->ni = n;
            r->tags->setBit(CI_TAG_NAMEITEM,true);
            ti->setNameItem(n);
            matchedHashs[r->client] = n;
            break;
        }
        if(!r->ni){
            foreach(NameItemAlias* alias, isolatedAlias){
                if(alias->longName() == r->client){
                    r->alias = alias;
                    r->ni = new SubjectNameItem(0,clientClsId,alias->shortName(),alias->longName(),alias->rememberCode(),alias->createdTime(),curUser);
                    r->tags->setBit(CI_TAG_NAMEITEM,true);
                    matchedHashs[r->client] = r->ni;
                    ti->setNameItem(r->ni);
                    break;
                }
            }
        }
    }
}

/**
 * @brief 遍历凭证集，验证发票在凭证中处理是否正确
 */
void CurInvoiceStatForm::on_btnVerify_clicked()
{
    QString errors;
    if(!suiteMgr->verifyCurInvoices(errors)){
        QString info = tr("验证发票时发现错误！");
        if(!errors.isEmpty())
            info.append("\n"+errors);
        else
            info.append(tr("\n发现有未在凭证集中出现的发票！"));
        myHelper::ShowMessageBoxWarning(info);
    }
    for(int i = 0; i < ui->twIncome->rowCount(); ++i){
        InvoiceProcessItem* ti = static_cast<InvoiceProcessItem*>(ui->twIncome->item(i,TI_ISPROCESS));
        CurInvoiceRecord* r = ui->twIncome->item(i,TI_INUMBER)->data(Qt::UserRole).value<CurInvoiceRecord*>();
        ti->setProcessState(r->processState);
        ti->setErrorInfos(r->errors);
    }
    for(int i = 0; i < ui->twCost->rowCount(); ++i){
        InvoiceProcessItem* ti = static_cast<InvoiceProcessItem*>(ui->twCost->item(i,TI_ISPROCESS));
        CurInvoiceRecord* r = ui->twCost->item(i,TI_INUMBER)->data(Qt::UserRole).value<CurInvoiceRecord*>();
        ti->setProcessState(r->processState);
        ti->setErrorInfos(r->errors);
    }
}

/**
 * @brief 重新根据列的类型解析列的内容
 */
void CurInvoiceStatForm::on_actResolveCol_triggered()
{
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    QPoint pos = tw->horizontalHeader()->mapFromGlobal(mapToGlobal(mnuColTypes->pos()));
    int col = tw->horizontalHeader()->logicalIndexAt(pos);
    QTableWidgetItem* hi = tw->horizontalHeaderItem(col);
    if(!hi)
        return;
    CurInvoiceColumnType colType = (CurInvoiceColumnType)hi->data(Qt::UserRole).toInt();
    int startRow = ui->lwSheets->currentItem()->data(DR_STARTROW).toInt();
    int endRow = ui->lwSheets->currentItem()->data(DR_ENDROW).toInt();
    if(startRow == 0 || endRow <= startRow ){
        myHelper::ShowMessageBoxWarning(tr("开始、结束行指定错误！"));
        return;
    }
    //以日期格式解析
    if(colType == CT_DATE || colType == CT_SFINFO){
        QDate primaryDay(1900,1,1);
        qint64 pdInt = primaryDay.toJulianDay()-2;
        for(int r = startRow; r <= endRow; ++r){
            QTableWidgetItem* ti = tw->item(r,col);
            if(!ti)
                continue;
            bool ok = false;
            qint64 dv = ti->text().toLongLong(&ok);
            if(!ok || dv == 0)
                continue;
            dv += pdInt;
            QDate d = QDate::fromJulianDay(dv);
            ti->setText(d.toString(Qt::ISODate));
        }
    }
    else if(colType == CT_MONEY){ //如果值为公式，则求解公式值
        for(int r = startRow; r <= endRow; ++r){
            QTableWidgetItem* ti = tw->item(r,col);
            if(!ti)
                continue;
            Double v;
            if(!calFormula(ti->text(),v,tw))
                continue;
            ti->setText(v.toString());
        }
    }
}
