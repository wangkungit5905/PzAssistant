#include "pzdialog3.h"
#include "ui_pzdialog3.h"

PzDialog3::PzDialog3(PzSetMgr *pzSet, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::pzDialog3)
{
    ui->setupUi(this);
}

PzDialog3::~PzDialog3()
{
    delete ui;
}
