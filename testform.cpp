#include "testform.h"
#include "ui_testform.h"
#include "global.h"
#include "subject.h"

TestForm::TestForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TestForm)
{
    ui->setupUi(this);
    SubjectManager* subMgr = curAccount->getSubjectManager(1);
    ui->cmbFst->setSubjectManager(subMgr);
    ui->cmbFst->setSubjectClass();
    ui->cmbSnd->setSubjectManager(subMgr);
    ui->cmbSnd->setSubjectClass(SubjectSelectorComboBox::SC_SND);
    connect(ui->cmbFst,SIGNAL(currentIndexChanged(int)),SLOT(fstSubChanged(int)));
    //    //SubjectSelectorComboBox cmb(subMgr,subMgr->getFstSubject("1131"),
    //    //                            SubjectSelectorComboBox::SC_SND,&dlg);
}

TestForm::~TestForm()
{
    delete ui;
}

void TestForm::fstSubChanged(int index)
{
    fsub = ui->cmbFst->itemData(index).value<FirstSubject*>();
    if(fsub){
        ui->edtName->setText(fsub->getName());
        ui->cmbSnd->setParentSubject(fsub);
    }
    else
        ui->edtName->clear();
}
