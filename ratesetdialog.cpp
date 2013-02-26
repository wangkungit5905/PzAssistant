#include "ratesetdialog.h"
#include "ui_ratesetdialog.h"

RateSetDialog::RateSetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RateSetDialog)
{
    ui->setupUi(this);
}

RateSetDialog::~RateSetDialog()
{
    delete ui;
}
