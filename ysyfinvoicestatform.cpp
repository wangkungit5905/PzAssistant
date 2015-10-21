#include "ysyfinvoicestatform.h"
#include "ui_ysyfinvoicestatform.h"

#include "PzSet.h"
#include "subject.h"
#include "myhelper.h"
#include "dbutil.h"
#include "widgets.h"

#include <QMenu>

////////////////////////////////////////////////////////////
InvoiceStateWidget::InvoiceStateWidget(CancelAccountState state, int type)
    :QTableWidgetItem(type),_state(state)
{

}

QVariant InvoiceStateWidget::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole)
        return getStateText(_state);
    if(role == Qt::EditRole)
        return (int)_state;
    return QTableWidgetItem::data(role);
}

void InvoiceStateWidget::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        _state = (CancelAccountState)value.toInt();
    QTableWidgetItem::setData(role, value);
}

QString InvoiceStateWidget::getStateText(CancelAccountState state) const
{
    switch (state) {
    case CAS_NONE:
        return QObject::tr("未销");
    case CAS_OK:
        return QObject::tr("已销");
    case CAS_PARTLY:
        return QObject::tr("部分");
    default:
        break;
    }
}

////////////////////////////////////////////////////////////
InvoiceStateEditor::InvoiceStateEditor(QWidget *parent):QComboBox(parent)
{
    addItem(tr("已销"),CAS_OK);
    addItem(tr("未销"),CAS_NONE);
    addItem(tr("部分"),CAS_PARTLY);
    setCurrentIndex(-1);
    connect(this,SIGNAL(currentIndexChanged(int)),this,SLOT(stateChanged(int)));
}

void InvoiceStateEditor::setState(CancelAccountState state)
{
    int index = findData((int)state);
    setCurrentIndex(index);
}

void InvoiceStateEditor::stateChanged(int index)
{
    _state = (CancelAccountState)itemData(index).toInt();
}
///////////////////////////////////////////////////////////
YsYfInvoiceTableDelegate::YsYfInvoiceTableDelegate(QWidget *parent):QItemDelegate(parent)
{

}

QWidget *YsYfInvoiceTableDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.column() != YsYfInvoiceStatForm::CI_STATE)
        return 0;
    InvoiceStateEditor*  editor = new InvoiceStateEditor(parent);
    return editor;
}

void YsYfInvoiceTableDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    InvoiceStateEditor* edt = qobject_cast<InvoiceStateEditor*>(editor);
    if(edt)
        edt->setState((CancelAccountState)index.model()->data(index,Qt::EditRole).toInt());
}

void YsYfInvoiceTableDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    InvoiceStateEditor* edt = qobject_cast<InvoiceStateEditor*>(editor);
    if(edt){
        CancelAccountState state = edt->state();
        model->setData(index,(int)state);
    }
}

void YsYfInvoiceTableDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    editor->setGeometry(rect);
}


////////////////////////////////////////////////////////////
YsYfInvoiceStatForm::YsYfInvoiceStatForm(AccountSuiteManager *amgr, bool init, QWidget *parent) :
    QWidget(parent),ui(new Ui::YsYfInvoiceStatForm),amgr(amgr)
{
    ui->setupUi(this);
    int moneyWidth = 80;
    ui->twIncome->setColumnWidth(CI_MONTH,40);
    ui->twIncome->setColumnWidth(CI_INVOICE,80);
    ui->twIncome->setColumnWidth(CI_MONEY,moneyWidth);
    ui->twIncome->setColumnWidth(CI_TAXMONEY,moneyWidth);
    ui->twIncome->setColumnWidth(CI_WMONEY,moneyWidth);
    ui->twIncome->setColumnWidth(CI_STATE,80);
    ui->twCost->setColumnWidth(CI_MONTH,40);
    ui->twCost->setColumnWidth(CI_INVOICE,80);
    ui->twCost->setColumnWidth(CI_MONEY,moneyWidth);
    ui->twCost->setColumnWidth(CI_TAXMONEY,moneyWidth);
    ui->twCost->setColumnWidth(CI_WMONEY,moneyWidth);
    ui->twCost->setColumnWidth(CI_STATE,50);
    ui->twError->setColumnWidth(CI_PZNUMBER,50);
    ui->twError->setColumnWidth(CI_BANUMBER,50);
    ui->tabWidget->setCurrentIndex(0);
    delegate = new YsYfInvoiceTableDelegate(this);
    ui->twIncome->setItemDelegate(delegate);
    ui->twCost->setItemDelegate(delegate);
    if(init)
        this->init();
    else{
        ui->lblTitle->setText(tr("%1月应收应付发票增减情况统计").arg(amgr->month()));
        initCurMonth();
        ui->btnOk->setEnabled(amgr->getState() == Ps_Jzed);
    }
}

YsYfInvoiceStatForm::~YsYfInvoiceStatForm()
{
    delete ui;
    clear();
}

void YsYfInvoiceStatForm::contextMenuRequested(const QPoint &pos)
{
    QTableWidget* obj = qobject_cast<QTableWidget*>(sender());
    if(obj == ui->twIncome && ui->twIncome->currentRow() != -1 ||
            obj == ui->twCost && ui->twCost->currentRow() != -1){
        QMenu m;
        m.addAction(ui->actDel);
        m.exec(obj->mapToGlobal(pos));
    }
}

/**
 * @brief 打算实现双击记录显示对应凭证的分录
 * @param index
 */
void YsYfInvoiceStatForm::doubleClicked(const QModelIndex &index)
{

}

void YsYfInvoiceStatForm::dataChanged(QTableWidgetItem *item)
{
    int col = item->column();
    if(col != CI_STATE)
        return;
    int row = item->row();
    CancelAccountState state = (CancelAccountState)item->data(Qt::EditRole).toInt();
    QTableWidget* t = item->tableWidget();
    bool changed = false;
    if(t == ui->twIncome){
        if(incomes.at(row)->state != state){
            incomes[row]->state = state;
            changed = true;
        }
    }
    else if(t = ui->twCost){
        if(costs.at(row)->state != state){
            costs[row]->state = state;
            changed = true;
        }
    }
    if(changed)
        ui->btnOk->setEnabled(amgr->getState() == Ps_Jzed);
}

//扫描本帐套内的应收应付发票增减情况
void YsYfInvoiceStatForm::init()
{
    ui->lblTitle->setText(tr("%1年度应收应付发票增减情况统计").arg(amgr->year()));
    amgr->scanYsYfForInit(incomes,costs,errors);
    viewRecords();
    viewErrors();
}

//扫描本月应收应付发票增减情况并综合先前历史记录，判定哪些发票增加了、销账了或部分销账
void YsYfInvoiceStatForm::initCurMonth()
{
    //首先读取保存在表中的记录，然后扫描当前月份并比较判定增加项、不存在项（可以删除）、销账项
    incomes = amgr->getYsInvoiceStats();
    costs = amgr->getYfInvoiceStats();
    //将发票记录分离为两部分，当月的记录与先前月份的记录
    int m = amgr->month(); int y = amgr->year();
    InvoiceRecord* r;
    for(int i = incomes.count()-1; i >= 0; i--){
        r = incomes.at(i);
        if(r->year == y && r->month == m)
            curIncomes<<incomes.takeAt(i);
    }
    for(int i = costs.count()-1; i >= 0; i--){
        r = costs.at(i);
        if(r->year == y && r->month == m)
            curCosts<<costs.takeAt(i);
    }
    QList<InvoiceRecord *> incomeAdds;
    QList<InvoiceRecord *> incomeCancels;
    QList<InvoiceRecord *> costAdds;
    QList<InvoiceRecord *> costCancels;
    QList<InvoiceRecord *> tems;
    amgr->scanYsYfForMonth2(amgr->month(),incomeAdds,incomeCancels,costAdds,costCancels,errors);
    bool changed = false;
    //首先查找有没有已经不存在的记录（即先前已经保存到表中的记录在当前月份凭证集中没有扫描到）
    tems = curIncomes;
    for(int i = incomeAdds.count()-1; i >= 0; i--){
        InvoiceRecord* r = incomeAdds.at(i);
        for(int j = 0; j<tems.count(); ++j){
            InvoiceRecord* r1 = tems.at(j);
            if(r->invoiceNumber == r1->invoiceNumber && r->customer == r1->customer){
                tems.removeAt(j);
                break;
            }
        }
    }
    foreach(InvoiceRecord* r, tems)
        curIncomes.removeOne(r);
    dels<<tems;tems.clear();
    tems = curCosts;
    for(int i = costAdds.count()-1; i >= 0; i--){
        InvoiceRecord* r = costAdds.at(i);
        for(int j = 0; j<tems.count(); ++j){
            InvoiceRecord* r1 = tems.at(j);
            if(r->invoiceNumber == r1->invoiceNumber && r->customer == r1->customer){
                tems.removeAt(j);
                break;
            }
        }
    }
    foreach (InvoiceRecord* r, tems)
        curCosts.removeOne(r);
    dels<<tems;tems.clear();
    changed = !dels.isEmpty();
    //将新扫描到的记录插入或更新
    for(int i = incomeAdds.count()-1; i >= 0; i--){
        InvoiceRecord* r = incomeAdds.at(i);
        bool c;
        if(!exist(r,c)){
            InvoiceRecord* ir = incomeAdds.takeAt(i);
            curIncomes<<ir;
            temrs<<ir;
        }
        if(!changed && c)
            changed = true;
    }
    qDeleteAll(incomeAdds); incomeAdds.clear();
    for(int i = 0; i < incomeCancels.count(); ++i){
        InvoiceRecord* r = incomeCancels.at(i);
        bool c = isCancel(r,errors);
        if(!changed && c)
            changed = true;
    }
    qDeleteAll(incomeCancels); incomeCancels.clear();
    for(int i = costAdds.count()-1; i >= 0; i--){
        InvoiceRecord* r = costAdds.at(i);
        bool c;
        if(!exist(r,c,false)){
            InvoiceRecord* ir = costAdds.takeAt(i);
            curCosts<<ir;
            temrs<<ir;
        }
        if(!changed && c)
            changed = true;
    }
    qDeleteAll(costAdds); costAdds.clear();
    for(int i = 0; i < costCancels.count(); ++i){
        InvoiceRecord* r = costCancels.at(i);
        changed = isCancel(r,errors,false);
    }
    qDeleteAll(costCancels); costCancels.clear();
    //重新归并一处
    incomes<<curIncomes; curIncomes.clear();
    costs<<curCosts;curCosts.clear();
    //ui->btnOk->setEnabled(amgr->getState() == Ps_Jzed && changed);
    qSort(incomes.begin(),incomes.end(),invoiceRecordCompareByCustomer);
    qSort(costs.begin(),costs.end(),invoiceRecordCompareByCustomer);
    viewRecords();
    viewErrors();
}

void YsYfInvoiceStatForm::inspectDataChanged(bool on)
{
    if(on){
        connect(ui->twIncome,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(dataChanged(QTableWidgetItem*)));
        connect(ui->twCost,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(dataChanged(QTableWidgetItem*)));
    }
    else{
        disconnect(ui->twIncome,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(dataChanged(QTableWidgetItem*)));
        disconnect(ui->twCost,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(dataChanged(QTableWidgetItem*)));
    }
}

/**
 * @brief 是否已经存在发票号和客户相同的记录
 * 如果存在，则比较它们的金额是否相同，如有不同则更新
 * @param r         新扫描到的记录
 * @param changed   金额是否被更新
 * @param isYs      true：应收，false：应付
 * @return
 */
bool YsYfInvoiceStatForm::exist(InvoiceRecord *r, bool &changed, bool isYs)
{
    QList<InvoiceRecord *> *p;
    if(isYs)
        p = &curIncomes;
    else
        p = &curCosts;
    foreach (InvoiceRecord* ir, *p) {
        if(ir->invoiceNumber == r->invoiceNumber && ir->customer == r->customer){
            if(ir->money != r->money){
                ir->money = r->money;
                changed = true;
            }
            if(ir->taxMoney != r->taxMoney){
                ir->taxMoney = r->taxMoney;
                changed = true;
            }
            if(ir->wmoney != r->wmoney){
                ir->wmoney = r->wmoney;
                changed = true;
            }
            return true;
        }
    }
    return false;
}

/**
 * @brief 当前月份记录中是否存在与指定的销账记录对应的记录
 * 如果存在，则改变记录的状态为销账态
 * @param r     销账记录
 * @param errors
 * @param isYs  true：应收，false：应付
 * @return
 */
bool YsYfInvoiceStatForm::isCancel(InvoiceRecord *r, QStringList &errors, bool isYs)
{
    QList<InvoiceRecord *> *p;
    if(isYs)
        p = &curIncomes;
    else
        p = &curCosts;
    foreach (InvoiceRecord* ir, *p) {
        if(ir->invoiceNumber == r->invoiceNumber && ir->customer == r->customer){
            if(ir->state != CAS_OK){
                ir->state = CAS_OK;
                return true;
            }
        }
    }
    if(isYs)
        p = &incomes;
    else
        p = &costs;
    foreach (InvoiceRecord* ir, *p) {
        if(ir->invoiceNumber == r->invoiceNumber && ir->customer == r->customer){
            if(ir->state != CAS_OK){
                ir->state = CAS_OK;
                return true;
            }
        }
    }
    errors<<tr("（%1#%2*）遇到一个销账记录的发票号（%3）无配对%4项")
            .arg(r->pzNumber).arg(r->baRID).arg(r->invoiceNumber).arg(isYs?tr("应收"):tr("应付"));
    return false;
}

void YsYfInvoiceStatForm::clear()
{
    incomes.clear();
    costs.clear();
    errors.clear();
}

void YsYfInvoiceStatForm::viewSingleRow(int row, InvoiceRecord *r, bool isYs)
{
    QTableWidget* t;
    if(isYs)
        t = ui->twIncome;
    else
        t = ui->twCost;
    t->insertRow(row);
    t->setItem(row,CI_MONTH,new QTableWidgetItem(QString::number(r->month)));
    t->setItem(row,CI_INVOICE,new QTableWidgetItem(r->invoiceNumber));
    t->setItem(row,CI_MONEY,new QTableWidgetItem(r->money.toString()));
    t->setItem(row,CI_TAXMONEY,new QTableWidgetItem(r->taxMoney.toString()));
    t->setItem(row,CI_WMONEY,new QTableWidgetItem(r->wmoney.toString()));
    t->setItem(row,CI_STATE,new InvoiceStateWidget(r->state));
    t->setItem(row,CI_CUSTOME,new QTableWidgetItem(r->customer->getLongName()));
}

void YsYfInvoiceStatForm::viewRecords()
{
    inspectDataChanged(false);
    ui->twIncome->setRowCount(0);
    ui->twCost->setRowCount(0);
    for(int i = 0; i < incomes.count(); ++i)
        viewSingleRow(i,incomes.at(i));
    for(int i = 0; i < costs.count(); ++i)
        viewSingleRow(i,costs.at(i),false);
    inspectDataChanged();
}

void YsYfInvoiceStatForm::viewErrors()
{
    ui->twError->setRowCount(0);
    for(int i = 0; i < errors.count(); ++i){
        QString err = errors.at(i);
        int pzNum=0,baNum=0;
        extractPzNum(err,pzNum,baNum);
        ui->twError->insertRow(i);
        ui->twError->setItem(i,CI_PZNUMBER,new QTableWidgetItem(QString::number(pzNum)));
        ui->twError->setItem(i,CI_BANUMBER,new QTableWidgetItem(QString::number(baNum)));
        ui->twError->setItem(i,CI_ERROR,new QTableWidgetItem(err));
    }
}

/**
 * @brief 从错误信息中提取该错误相关的凭证号和分录序号
 * @param errorInfos
 */
void YsYfInvoiceStatForm::extractPzNum(QString errorInfos,int &pzNum, int &baNum)
{
    QRegExp re(tr("^（(\\d+)#{1}(\\d+)\\*{1}）"));
    int pos = re.indexIn(errorInfos);
    if(pos != -1){
        pzNum = re.cap(1).toInt();
        baNum = re.cap(2).toInt();
    }
}



void YsYfInvoiceStatForm::on_btnOk_clicked()
{
    DbUtil* du = amgr->getAccount()->getDbUtil();
    if(!du->saveInvoiceRecords(incomes) || !du->saveInvoiceRecords(costs))
        myHelper::ShowMessageBoxError(tr("保存发票统计记录时发生错误！"));
    else{
        //将销账项销毁
        foreach (InvoiceRecord* r, incomes) {
            if(r->state == CAS_OK){
                incomes.removeOne(r);
                delete r;
            }
        }
        foreach (InvoiceRecord* r, costs) {
            if(r->state == CAS_OK){
                costs.removeOne(r);
                delete r;
            }
        }
        amgr->setYsInvoiceStats(incomes);
        amgr->setYfInvoiceStats(costs);
        ui->btnOk->setEnabled(false);
    }
    if(!dels.isEmpty() && !du->removeInvoiceRecords(dels))
        myHelper::ShowMessageBoxError(tr("在移除已不存在的发票记录时发生错误！"));
    clear();
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}

void YsYfInvoiceStatForm::on_btnCancel_clicked()
{
    clear();
    //因为新扫描添加的记录项，如果不保存，则要将其移除以免内存泄漏
    if(!temrs.isEmpty()){
        qDeleteAll(temrs);
        temrs.clear();
    }
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}

void YsYfInvoiceStatForm::on_actDel_triggered()
{
    if(ui->tabWidget->currentIndex() == 0){
        int row = ui->twIncome->currentRow();
        ui->twIncome->removeRow(row);
        dels<<incomes.takeAt(row);
        ui->btnOk->setEnabled(amgr->getState() == Ps_Jzed);
    }
    else if(ui->tabWidget->currentIndex() == 1){
        int row = ui->twCost->currentRow();
        ui->twCost->removeRow(row);
        dels<<costs.takeAt(row);
        ui->btnOk->setEnabled(amgr->getState() == Ps_Jzed);
    }
}

bool invoiceRecordCompareByDate(InvoiceRecord *r1, InvoiceRecord *r2)
{
    return r1->year<r2->year || r1->year==r2->year && r1->month<r2->month;

}

//发票记录比较函数，用于对应收记录排序（按年月、发票号、客户名）
bool invoiceRecordCompareByNumber(InvoiceRecord *r1, InvoiceRecord *r2)
{
    return r1->invoiceNumber < r2->invoiceNumber;
//    if(r1->year < r2->year)
//        return true;
//    else if(r1->month < r2->month)
//        return true;
//    else if(r1->invoiceNumber < r2->invoiceNumber)
//        return true;
//    else
//        return r1->customer->getShortName() < r2->customer->getShortName();
}

//发票记录比较函数，用于对应付记录排序（按年月、客户名、发票号）
bool invoiceRecordCompareByCustomer(InvoiceRecord *r1, InvoiceRecord *r2)
{
    return r1->customer->getShortName() < r2->customer->getShortName();
//    if(r1->year < r2->year)
//        return true;
//    else if(r1->month < r2->month)
//        return true;
//    else if(r1->customer->getShortName() < r2->customer->getShortName())
//        return true;
//    else if(r1->invoiceNumber < r2->invoiceNumber)
//        return true;
//    else
//        return false;
}


