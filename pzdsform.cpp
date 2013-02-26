#include <QSqlQuery>

#include "pzdsform.h"
#include "ui_pzdsform.h"

PzDSForm::PzDSForm(int id, EnumPzDsType type, QWidget *parent)
    : QWidget(parent), ui(new Ui::PzDSForm),pzId(id),type(type)
{
    ui->setupUi(this);
    init();
}

PzDSForm::~PzDSForm()
{
    delete ui;
}

void PzDSForm::on_btnClose_clicked()
{
    close();
}

void PzDSForm::on_btnUpToDS_clicked()
{

}

void PzDSForm::on_btnUpToPZ_clicked()
{

}

void PzDSForm::init()
{
    ui->twClients->setColumnWidth(0,100);
    ui->twClients->setColumnWidth(0,600);
    ui->twClients->setColumnWidth(0,50);
}

/**
 * @brief PzDSForm::loadBills
 * 装载发票
 */
void PzDSForm::loadBills()
{
    QSqlQuery q;
    QString s = QString("select * from t_bills where PzId=%1 and IsIncome=1").arg(pzId);
    if(!q.exec(s))
        return;
    if(!q.first())
        return;
    QTableWidgetItem* item;
    QString ns;
    int r = 0;
    while(q.next()){
        ui->twClients->insertRow(r);
        ns = q.value(1).toString();
        item = new QTableWidgetItem(ns);
        ui->twClients->setItem(r,0,item);
    }
}

/**
 * @brief PzDSForm::loadNewClients
 * 装载新客户
 */
void PzDSForm::loadNewClients()
{
    QSqlQuery q;
    QString s = "select * from newClients";
    if(!q.exec(s))
        return;
    if(!q.first())
        return;
    QTableWidgetItem* item;
    QString ns;
    int r = 0;
    while(q.next()){
        ui->twClients->insertRow(r);
        ns = q.value(1).toString();
        item = new QTableWidgetItem(ns);
        ui->twClients->setItem(r,0,item);
        ns = q.value(2).toString();
        item = new QTableWidgetItem(ns);
        ui->twClients->setItem(r,1,item);
        ns = q.value(3).toString();
        item = new QTableWidgetItem(ns);
        ui->twClients->setItem(r,2,item);
        r++;
    }
}

void PzDSForm::on_btnAddBill_clicked()
{

}

void PzDSForm::on_btnDelBill_clicked()
{

}
