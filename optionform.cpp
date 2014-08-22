#include "optionform.h"
#include "widgets.h"
//#include "global.h"
#include "subject.h"
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

///////////////////////SpecSubCodeCfgform//////////////////////////////////


//SpecSubCodeCfgform::SpecSubCodeCfgform(QWidget *parent) :
//    ConfigPanelBase(parent),ui(new Ui::SpecSubCodeCfgform)
//{
//    ui->setupUi(this);
//    appCon = AppConfig::getInstance();
//    init();
//}

//SpecSubCodeCfgform::~SpecSubCodeCfgform()
//{
//    delete ui;
//}

//bool SpecSubCodeCfgform::isDirty()
//{
//    return false;
//}

//bool SpecSubCodeCfgform::save()
//{
//    return true;
//}

//void SpecSubCodeCfgform::init()
//{
//    names[AppConfig::SSC_CASH ] = tr("现金");
//    names[AppConfig::SSC_BANK ] = tr("银行存款");
//    names[AppConfig::SSC_GDZC] = tr("固定资产");
//    names[AppConfig::SSC_CWFY] = tr("财务费用");
//    names[AppConfig::SSC_BNLR] = tr("本年利润");
//    names[AppConfig::SSC_LRFP] = tr("利润分配");
//    names[AppConfig::SSC_YS] = tr("应收账款");
//    names[AppConfig::SSC_YF] = tr("应付账款");
//    names[AppConfig::SSC_YJSJ] = tr("应交税金");
//    names[AppConfig::SSC_ZYSR] = tr("主营业务收入");
//    names[AppConfig::SSC_ZYCB] = tr("主营业务成本");
//    QList<SubSysNameItem*> items;
//    appCon->getSubSysItems(items);
////    foreach(SubSysNameItem* item, items){
////        ui->tabWidget->addTab(crtTab(item->code),item->name);
////    }
//    ui->tabWidget->addTab(crtTab(2),items.at(1)->name);
//}

///**
// * @brief 创建对应与指定科目系统的特定科目配置页
// * @param subSys
// * @return
// */
//QWidget *SpecSubCodeCfgform::crtTab(int subSys)
//{
//    SubjectManager* sm = curAccount->getSubjectManager(subSys);
//    QWidget* w = new QWidget(this);
//    QHash<AppConfig::SpecSubCode,QString> items = appCon->getAllSpecSubCodeForSubSys(subSys);
//    QList<AppConfig::SpecSubCode> es = items.keys();
//    qSort(es.begin(),es.end());
//    QGridLayout* gl = new QGridLayout();
//    for(int r = 0; r < es.count(); ++r){
//        AppConfig::SpecSubCode e = es.at(r);
//        QLabel* lblName = new QLabel(items.value(e),w);
//        QString code = items.value(e);
//        FirstSubject* fsub = sm->getFstSubject(code);
//        QLabel* lblSub = new QLabel(fsub->getName());
//        gl->addWidget(lblName,r,0,1,1);
//        gl->addWidget(lblSub,r,1,1,1);
//    }
//    w->setLayout(gl);
//    return w;
//}

/////////////////////////////

TestPanel::TestPanel(QWidget *parent):ConfigPanelBase(parent)
{
    QLabel* l = new QLabel(tr("测试页"),this);
    QHBoxLayout* ly = new QHBoxLayout;
    ly->addWidget(l);
    setLayout(ly);
}
