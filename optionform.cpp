#include "optionform.h"
#include "widgets.h"
#include "config.h"
#include "ui_pztemplateoptionform.h"

#include <QHBoxLayout>

//////////////////////PzTemplateOptionForm//////////////////////////////////
PzTemplateOptionForm::PzTemplateOptionForm(QWidget *parent) :
    ConfigPanelBase(parent),ui(new Ui::PzTemplateOptionForm)
{
    ui->setupUi(this);
    AppConfig::getInstance()->getPzTemplateParameter(&parameter);
    ui->spnTitleHeight->setValue(parameter.titleHeight);
    ui->spnBaRowHeight->setValue(parameter.baRowHeight);
    ui->spnBaRowNum->setValue(parameter.baRows);
    ui->spnFontSize->setValue(parameter.fontSize);
    ui->spnLFMargin->setValue(parameter.leftRightMargin);
    ui->spnTBMargin->setValue(parameter.topBottonMargin);
    ui->spnCutArea->setValue(parameter.cutAreaHeight);
    ui->chkCutLine->setChecked(parameter.isPrintCutLine);
    ui->chkMidLine->setChecked(parameter.isPrintMidLine);
    ui->spnSummary->setValue(parameter.factor[0]);
    ui->spnSubject->setValue(parameter.factor[1]);
    ui->spnMoney->setValue(parameter.factor[2]);
    ui->spnWb->setValue(parameter.factor[3]);
    ui->spnRate->setValue(parameter.factor[4]);
}

PzTemplateOptionForm::~PzTemplateOptionForm()
{
    delete ui;
}

bool PzTemplateOptionForm::isDirty()
{
    if(ui->spnBaRowHeight->value() != parameter.baRowHeight)
        return true;
    if(parameter.baRows != ui->spnBaRowNum->value())
        return true;
    if(parameter.titleHeight != ui->spnTitleHeight->value())
        return true;
    if(parameter.fontSize != ui->spnFontSize->value())
        return true;
    if(parameter.cutAreaHeight != ui->spnCutArea->value())
        return true;
    if(parameter.leftRightMargin != ui->spnLFMargin->value())
        return true;
    if(parameter.topBottonMargin != ui->spnTBMargin->value())
        return true;
    if(parameter.isPrintCutLine != ui->chkCutLine->isChecked())
        return true;
    if(parameter.isPrintMidLine != ui->chkMidLine->isChecked())
        return true;
    if(parameter.factor[0] != ui->spnSummary->value())
        return true;
    if(parameter.factor[1] != ui->spnSubject->value())
        return true;
    if(parameter.factor[2] != ui->spnMoney->value())
        return true;
    if(parameter.factor[3] != ui->spnWb->value())
        return true;
    if(parameter.factor[4] != ui->spnRate->value())
        return true;
    return false;
}

bool PzTemplateOptionForm::save()
{
    if(!isDirty())
        return true;
    if(ui->spnBaRowHeight->value() != parameter.baRowHeight)
        parameter.baRowHeight = ui->spnBaRowHeight->value();
    if(parameter.baRows != ui->spnBaRowNum->value())
        parameter.baRows = ui->spnBaRowNum->value();
    if(parameter.titleHeight != ui->spnTitleHeight->value())
        parameter.titleHeight = ui->spnTitleHeight->value();
    if(parameter.fontSize != ui->spnFontSize->value())
        parameter.fontSize = ui->spnFontSize->value();
    if(parameter.cutAreaHeight != ui->spnCutArea->value())
        parameter.cutAreaHeight = ui->spnCutArea->value();
    if(parameter.leftRightMargin != ui->spnLFMargin->value())
        parameter.leftRightMargin = ui->spnLFMargin->value();
    if(parameter.topBottonMargin != ui->spnTBMargin->value())
        parameter.topBottonMargin = ui->spnTBMargin->value();
    if(parameter.isPrintCutLine != ui->chkCutLine->isChecked())
        parameter.isPrintCutLine = ui->chkCutLine->isChecked();
    if(parameter.isPrintMidLine != ui->chkMidLine->isChecked())
        parameter.isPrintMidLine = ui->chkMidLine->isChecked();
    if(parameter.factor[0] != ui->spnSummary->value())
        parameter.factor[0] = ui->spnSummary->value();
    if(parameter.factor[1] != ui->spnSubject->value())
        parameter.factor[1] = ui->spnSubject->value();
    if(parameter.factor[2] != ui->spnMoney->value())
        parameter.factor[2] = ui->spnMoney->value();
    if(parameter.factor[3] != ui->spnWb->value())
        parameter.factor[3] = ui->spnWb->value();
    if(parameter.factor[4] != ui->spnRate->value())
        parameter.factor[4] = ui->spnRate->value();
    return AppConfig::getInstance()->savePzTemplateParameter(&parameter);
}



/////////////////////////////

TestPanel::TestPanel(QWidget *parent):ConfigPanelBase(parent)
{
    QLabel* l = new QLabel(tr("测试页"),this);
    QHBoxLayout* ly = new QHBoxLayout;
    ly->addWidget(l);
    setLayout(ly);
}
