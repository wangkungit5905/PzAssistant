#include "jzhdsyinfoinputdlg.h"
#include "ui_jzhdsyinfoinputdlg.h"


JzHdsyInfoInputDlg::JzHdsyInfoInputDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JzHdsyInfoInputDlg)
{
    ui->setupUi(this);
    ui->dateEdit->setDate(QDate(2012,12,1));
    QDoubleValidator* v = new QDoubleValidator(this);
    ui->edtRate->setValidator(v);
}

JzHdsyInfoInputDlg::~JzHdsyInfoInputDlg()
{
    delete ui;
}

int JzHdsyInfoInputDlg::getYear()
{
    return ui->dateEdit->date().year();
}

int JzHdsyInfoInputDlg::getMonth()
{
    return ui->dateEdit->date().month();
}

Double JzHdsyInfoInputDlg::getRate()
{
    return Double(ui->edtRate->text().toDouble());
}

void JzHdsyInfoInputDlg::on_dateEdit_dateChanged(const QDate &date)
{
    QHash<int,Double> rates;
    rateExist = curAccount->getRates(ui->dateEdit->date().year(),ui->dateEdit->date().month(),rates);
    ui->edtRate->setReadOnly(rateExist);
}
