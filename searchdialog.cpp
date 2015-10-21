#include "searchdialog.h"
#include "account.h"
#include "PzSet.h"
#include "subject.h"
#include "dbutil.h"
#include "myhelper.h"

#include <QBuffer>

PzSearchDialog::PzSearchDialog(Account *account, QByteArray *cinfo, QByteArray *pinfo, QWidget *parent) :
    QDialog(parent),ui(new Ui::SearchDialog),account(account),curPageNum(0),pageRowCount(MAXROWSPERPAGE)
{
    ui->setupUi(this);
    ui->gbCondition->setFixedHeight(200);
    sMgr = account->getSuiteMgr();
    if(!sMgr){
        ui->btnFind->setEnabled(false);
        return;
    }
    sm = sMgr->getSubjectManager();
    setCommonState(cinfo);
    setProperState(pinfo);
    mts = account->getAllMoneys();
    init();
}

PzSearchDialog::~PzSearchDialog()
{
    qDeleteAll(rs);
    rs.clear();
    delete ui;
}

QByteArray *PzSearchDialog::getCommonState()
{
    QByteArray* ba = new QByteArray;
    QBuffer bf(ba);
    QDataStream out(&bf);
    qint16 i16;
    bf.open(QIODevice::WriteOnly);
    for(int i = 0; i < ui->tw->columnCount(); ++i){
        i16 = ui->tw->columnWidth(i);
        out<<i16;
    }
    bf.close();
    return ba;
}

void PzSearchDialog::setCommonState(QByteArray *states)
{
    QList<int> colWidths;
    if(!states || states->isEmpty()){
        colWidths<<80<<50<<300<<80<<80<<50<<80<<80;
    }
    else{
        QBuffer bf(states);
        QDataStream in(&bf);
        qint16 i16;
        bf.open(QIODevice::ReadOnly);
        for(int i = 0; i < 8; ++i){
            in>>i16;
            colWidths<<i16;
        }
    }
    for(int i = 0; i < colWidths.count(); ++i)
        ui->tw->setColumnWidth(i,colWidths.at(i));
}

/**
 * @brief 返回窗口的专有状态信息（包括最后一次查找所使用的过滤条件）
 * 是否有必要保存最后一次查询条件？待实际需要是再实现
 * @return
 */
QByteArray *PzSearchDialog::getProperState()
{
    QByteArray* ba = new QByteArray;
//    QBuffer bf(ba);
//    QDataStream out(&bf);
//    qint8 i8;
//    qint16 i16;

//    bf.open(QIODevice::WriteOnly);

//     bf.close();
     return ba;
}

void PzSearchDialog::setProperState(QByteArray *states)
{

}

void PzSearchDialog::fsubSelectChanged(int index)
{
    if(index == 0){
        ui->cmbSSub->clear();
        ui->cmbSSub->addItem(tr("所有"),0);
        ui->cmbSSub->setEnabled(false);
    }
    else{
        FirstSubject* fsub = ui->cmbFSub->itemData(index).value<FirstSubject*>();
        ui->cmbSSub->setEnabled(true);
        ui->cmbSSub->setParentSubject(fsub);
        ui->cmbSSub->insertItem(0,tr("所有"),0);
    }
    ui->cmbSSub->setCurrentIndex(0);
}

void PzSearchDialog::startDateChanged(const QDate &date)
{
    ui->deEnd->setMinimumDate(date);
}

void PzSearchDialog::dateScopeChanged(bool on)
{
    if(!on)
        return;
    QRadioButton* b = qobject_cast<QRadioButton*>(sender());
    if(b == ui->rdoCurSuilte){
        setDateScopeForSuite();
        enDateScopeEdit(false);
    }
    else if(b == ui->rdoCurPzSet){
        int y = sMgr->getSuiteRecord()->year;
        int m = sMgr->month();
        QDate d(y,m,1);
        ui->deStart->setDate(d);
        d.setDate(y,m,d.daysInMonth());
        ui->deEnd->setDate(d);
        enDateScopeEdit(false);
    }
    else if(b == ui->rdoDateLimit)
        enDateScopeEdit(true);
}

void PzSearchDialog::moneyMatchChanged(bool on)
{
    if(on){
        ui->spnMinValue->setVisible(false);
        ui->lblMin->setVisible(false);
        ui->lblMax->setText(tr("金额"));
    }
    else{
        ui->spnMinValue->setVisible(true);
        ui->lblMin->setVisible(true);
        ui->lblMax->setText(tr("最大"));
    }
}

void PzSearchDialog::summaryFilteChanged(bool on)
{
    ui->edtInvoice->setEnabled(on);
    ui->edtContent->setEnabled(!on);
}

/**
 * @brief 双击凭证号单元格则打开对应凭证，并加亮对应分录
 * @param item
 */
void PzSearchDialog::positionPz(QTableWidgetItem *item)
{
    if(item->column() != TI_PZNUM)
        return;
    int pageIndex = (curPageNum-1) * pageRowCount + item->row();
    if(pageIndex >= rs.count())
        return;
    PzFindBaContent* c = rs.at(pageIndex);
    emit openSpecPz(c->pid,c->bid);
}


void PzSearchDialog::on_btnFind_clicked()
{
    PzFindFilteCondition filter;
    filter.startDate = ui->deStart->date();
    filter.endDate = ui->deEnd->date();
    filter.fsub = 0;
    filter.ssub = 0;
    if(ui->gbSub->isChecked()){
        filter.fsub = ui->cmbFSub->currentData().value<FirstSubject*>();
        filter.ssub = ui->cmbSSub->currentData().value<SecondSubject*>();
    }
    filter.isInvoiceNumInSummary=false;
    if(ui->gbSummary->isChecked()){
        if(ui->rdoInvoiceNum->isChecked()){
            if(ui->edtInvoice->text().length() != 8){
                myHelper::ShowMessageBoxWarning(tr("发票号不完整！"));
                return;
            }
            filter.isInvoiceNumInSummary = true;
            filter.summary = ui->edtInvoice->text();
        }
        else{
            filter.summary = ui->edtContent->text();
        }
    }
    filter.isPreciseMatch = ui->chkPreciseMatch->isChecked();
    filter.dir = MDIR_P;
    if(ui->gbMoney->isChecked()){
        filter.vMax = ui->spnMaxValue->value();
        if(!filter.isPreciseMatch)
            filter.vMin = ui->spnMinValue->value();
        if(ui->chkJDir->isChecked() ^ ui->chkDDir->isChecked()){
            if(ui->chkJDir->isChecked())
                filter.dir = MDIR_J;
            else
                filter.dir = MDIR_D;
        }
    }
    bool hasMore = false;
    if(!account->getDbUtil()->findPz(filter,temRs,hasMore,pageRowCount)){
        myHelper::ShowMessageBoxError(tr("查询出错！"));
        return;
    }
    qDeleteAll(rs);
    rs.clear();
    rs<<temRs;
    curPageNum=1;
    ui->btnPrePage->setEnabled(false);
    ui->btnNextPage->setEnabled(hasMore);
    temRs.clear();
    viewPage(curPageNum);
}

void PzSearchDialog::init()
{
    ui->cmbFSub->setSubjectManager(sm);
    ui->cmbFSub->setSubjectClass();
    ui->cmbFSub->insertItem(0,tr("所有"));
    ui->cmbSSub->setSubjectManager(sm);
    ui->cmbSSub->setSubjectClass(SubjectSelectorComboBox::SC_SND);
    connect(ui->cmbFSub,SIGNAL(currentIndexChanged(int)),this,SLOT(fsubSelectChanged(int)));
    ui->cmbFSub->setCurrentIndex(0);
    connect(ui->deStart,SIGNAL(dateChanged(QDate)),this,SLOT(startDateChanged(QDate)));
    setDateScopeForSuite();
    if(sMgr->month() == 0)
        ui->rdoCurPzSet->setEnabled(false);
    enDateScopeEdit(false);
    connect(ui->rdoCurSuilte,SIGNAL(toggled(bool)),this,SLOT(dateScopeChanged(bool)));
    connect(ui->rdoCurPzSet,SIGNAL(toggled(bool)),this,SLOT(dateScopeChanged(bool)));
    connect(ui->rdoDateLimit,SIGNAL(toggled(bool)),this,SLOT(dateScopeChanged(bool)));
    connect(ui->chkPreciseMatch,SIGNAL(toggled(bool)),this,SLOT(moneyMatchChanged(bool)));
    connect(ui->rdoInvoiceNum,SIGNAL(toggled(bool)),this,SLOT(summaryFilteChanged(bool)));
    connect(ui->tw,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(positionPz(QTableWidgetItem*)));
    ui->edtInvoice->setEnabled(false);
    ui->tw->setRowCount(pageRowCount);
    ui->edtInvoice->setValidator(new QRegExpValidator(QRegExp("^[0-9]{8}$"),this));
    ui->btnExpand->setIcon(QIcon(":/images/arrow-up"));
}

void PzSearchDialog::setDateScopeForSuite()
{
    AccountSuiteRecord* r = sMgr->getSuiteRecord();
    QDate d(r->year,r->startMonth,1);
    ui->deStart->setMinimumDate(d);
    ui->deStart->setDate(d);
    d.setDate(r->year,r->endMonth,1);
    d.setDate(r->year,r->endMonth,d.daysInMonth());
    ui->deStart->setMaximumDate(d);
    ui->deEnd->setMaximumDate(d);
    ui->deEnd->setDate(d);
}

void PzSearchDialog::enDateScopeEdit(bool en)
{
    ui->deStart->setReadOnly(!en);
    ui->deEnd->setReadOnly(!en);
}

void PzSearchDialog::viewPage(int pageNum)
{
    if((pageNum-1)*pageRowCount>rs.count())
        return;
    ui->tw->clearContents();
    for(int i=0; i < pageRowCount; ++i){
        int index = (curPageNum-1)*pageRowCount+i;
        if(index >= rs.count())
            break;
        PzFindBaContent* c = rs.at(index);
        ui->tw->setItem(i,TI_DATE,new QTableWidgetItem(c->date));
        QTableWidgetItem* ti = new QTableWidgetItem(QString::number(c->pzNum));
        ti->setData(DR_PID,c->pid);
        ti->setData(DR_BID,c->bid);
        ui->tw->setItem(i,TI_PZNUM,ti);
        ui->tw->setItem(i,TI_SUMMARY,new QTableWidgetItem(c->summary));
        FirstSubject* fsub = sm->getFstSubject(c->fid);
        ui->tw->setItem(i,TI_FSUB,new QTableWidgetItem(fsub?fsub->getName():QString::number(c->fid)));
        SecondSubject* ssub = sm->getSndSubject(c->sid);
        ui->tw->setItem(i,TI_SSUB,new QTableWidgetItem(ssub?ssub->getName():QString::number(c->sid)));
        Money* m = mts.value(c->mt);
        ui->tw->setItem(i,TI_MONEYTYPE,new QTableWidgetItem(m?m->name():QString::number(c->mt)));
        if(c->dir == MDIR_J)
            ui->tw->setItem(i,TI_JMONEY,new QTableWidgetItem(c->value.toString()));
        else
            ui->tw->setItem(i,TI_DMONEY,new QTableWidgetItem(c->value.toString()));
    }
    ui->tw->scrollToTop();
    ui->lblPageNum->setText(tr("第 %1 页").arg(curPageNum));
}

void PzSearchDialog::on_btnPrePage_clicked()
{
    curPageNum--;
    ui->btnPrePage->setEnabled(curPageNum>1);
    ui->btnNextPage->setEnabled(true);
    viewPage(curPageNum);
}

void PzSearchDialog::on_btnNextPage_clicked()
{
    curPageNum++;
    if((curPageNum-1)*pageRowCount>=rs.count()){
        bool hasMore = false;
        PzFindFilteCondition condition;
        account->getDbUtil()->findPz(condition,temRs,hasMore,pageRowCount,true);
        rs<<temRs;
        temRs.clear();
        ui->btnNextPage->setEnabled(hasMore);
    }
    else
        ui->btnNextPage->setEnabled(curPageNum*pageRowCount<rs.count());
    ui->btnPrePage->setEnabled(true);
    viewPage(curPageNum);
}

void PzSearchDialog::on_btnExpand_toggled(bool checked)
{
    if(checked){
        ui->gbCondition->setFixedHeight(200);
        ui->btnExpand->setIcon(QIcon(":/images/arrow-up"));
    }
    else{
        ui->gbCondition->setFixedHeight(50);
        ui->btnExpand->setIcon(QIcon(":/images/arrow-down"));
    }
}
