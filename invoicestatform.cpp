#include "invoicestatform.h"
#include "ui_invoicestatform.h"

#include "PzSet.h"
#include "subject.h"
#include "pzdialog.h"


InvoiceStatForm::InvoiceStatForm(AccountSuiteManager *manager, QWidget *parent) :
    QWidget(parent),ui(new Ui::InvoiceStatForm),amgr(manager)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::Dialog);
    this->parent = qobject_cast<PzDialog*>(parent);
    init();
}

InvoiceStatForm::~InvoiceStatForm()
{
    delete ui;
}

void InvoiceStatForm::doubleRecord(const QModelIndex &index)
{
    QTableWidget* table = qobject_cast<QTableWidget*>(sender());
    if(table && table == ui->twIncome){
        InvoiceRecord* r = incomes.at(index.row());
        if(amgr->seek(r->pzNumber)){
            PingZheng* pz = amgr->getCurPz();
            for(int i = 0; i < pz->baCount(); ++i){
                if(r->baRID == pz->getBusiAction(i)->getId()){
                    parent->selectBa(i+1);
                    break;
                }
            }
        }
        return;
    }
    if(table && table == ui->twCost){
        InvoiceRecord* r = costs.at(index.row());
        if(amgr->seek(r->pzNumber)){
            PingZheng* pz = amgr->getCurPz();
            for(int i = 0; i < pz->baCount(); ++i){
                if(r->baRID == pz->getBusiAction(i)->getId()){
                    parent->selectBa(i+1);
                    break;
                }
            }
        }
        return;
    }
    QListWidget* lw = qobject_cast<QListWidget*>(sender());
    if(lw && lw == ui->lwErrors){
        QRegExp re(tr("^（(\\d+)#{1}(\\d+)\\*{1}）"));
        int pos = re.indexIn(ui->lwErrors->currentItem()->text());
        if(pos != -1){
            int pzNum = re.cap(1).toInt();
            int baNum = re.cap(2).toInt();
            amgr->seek(pzNum);
            parent->selectBa(baNum);
        }
    }
}

void InvoiceStatForm::init()
{
    ui->lblYear->setText(QString::number(amgr->year()));
    ui->lblMonth->setText(QString::number(amgr->month()));
    QStringList titles;
    titles<<tr("凭证号")<<tr("普票")<<tr("发票号")<<tr("账面金额")<<tr("外币金额")
         <<tr("税额")<<tr("销账状态")<<tr("关联客户");
    ui->twIncome->setColumnCount(titles.count());
    ui->twCost->setColumnCount(titles.count());
    ui->twIncome->setHorizontalHeaderLabels(titles);
    ui->twCost->setHorizontalHeaderLabels(titles);
    QList<int> widths;
    widths<<40<<30<<80<<100<<80<<80<<50;
    for(int i = 0; i < widths.count(); ++i){
        ui->twIncome->setColumnWidth(i,widths.at(i));
        ui->twCost->setColumnWidth(i,widths.at(i));
    }
    rescan();
}

void InvoiceStatForm::rescan()
{
    qDeleteAll(incomes);
    incomes.clear();
    qDeleteAll(costs);
    costs.clear();
    ui->twIncome->setRowCount(0);
    ui->twCost->setRowCount(0);
    ui->lwErrors->clear();
    QStringList errors;
    amgr->scanInvoice(incomes,costs,errors);
    ui->twIncome->setRowCount(incomes.count());
    ui->twCost->setRowCount(costs.count());
    for(int i = 0; i < incomes.count(); ++i)
        refreshRow(i);
    for(int i = 0; i < costs.count(); ++i)
        refreshRow(i,false);
    if(!errors.isEmpty()){
        for(int i = 0; i < errors.count(); ++i){
            QListWidgetItem* item = new QListWidgetItem(errors.at(i), ui->lwErrors);
        }
    }
}

void InvoiceStatForm::refreshRow(int row, bool isIncome)
{
    InvoiceRecord *r;
    QTableWidget* t;
    if(isIncome){
        t = ui->twIncome;
        r = incomes.at(row);
    }
    else{
        t = ui->twCost;
        r = costs.at(row);
    }
    QString ti ;
    t->setItem(row,CL_PZNUM,new QTableWidgetItem(tr("%1#").arg(r->pzNumber)));
    t->setItem(row,CL_COMMON,new QTableWidgetItem(r->isCommon?tr("是"):tr("否")));
    t->setItem(row,CL_INVOICE,new QTableWidgetItem(r->invoiceNumber));
    t->setItem(row,CL_MONEY,new QTableWidgetItem(r->money.toString()));
    t->setItem(row,CL_WMONEY,new QTableWidgetItem(r->wmoney.toString()));
    t->setItem(row,CL_TAXMONEY,new QTableWidgetItem(r->taxMoney.toString()));
    if(r->state == CAS_NONE)
        ti = tr("未销");
    else if(r->state == CAS_PARTLY)
        ti = tr("部分");
    else
        ti = tr("已销");
    t->setItem(row,CL_STATE,new QTableWidgetItem(ti));
    t->setItem(row,CL_CUSTOMER,new QTableWidgetItem(r->customer?r->customer->getLongName():""));
}

void InvoiceStatForm::on_btnScan_clicked()
{
    ui->btnScan->setEnabled(false);
    QApplication::processEvents();
    rescan();
    ui->btnScan->setEnabled(true);
}
