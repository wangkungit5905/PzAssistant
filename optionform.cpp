#include "optionform.h"
#include "widgets.h"
#include "subject.h"
#include "myhelper.h"
#include "ui_appcommcfgpanel.h"
#include "ui_pztemplateoptionform.h"
#include "ui_stationcfgform.h"

#include <QHBoxLayout>
#include <QMenu>


////////////////////AppCommCfgPanel//////////////////////////////
AppCommCfgPanel::AppCommCfgPanel(QWidget *parent) :
    ConfigPanelBase(parent),
    ui(new Ui::AppCommCfgPanel)
{
    ui->setupUi(this);
    init();
}

AppCommCfgPanel::~AppCommCfgPanel()
{
    delete ui;
}

bool AppCommCfgPanel::isDirty()
{
    return false;
}

bool AppCommCfgPanel::save()
{
    return true;
}

void AppCommCfgPanel::styleChanged(bool checked)
{
    if(checked){
        QRadioButton* obj = qobject_cast<QRadioButton*>(sender());
        QString cssName;
        if(obj == ui->rdoNavy)
            cssName = "navy";
        else if(obj == ui->rdoBlack)
            cssName = "black";
        else
            cssName = "pink";
        myHelper::SetStyle(cssName);
        AppConfig::getInstance()->setAppStyleName(cssName);
    }
}

void AppCommCfgPanel::styleFromChanged(bool checked)
{
    AppConfig::getInstance()->setStyleFrom(checked);
}

void AppCommCfgPanel::init()
{
    AppConfig* appCfg = AppConfig::getInstance();
    QString styleName = appCfg->getAppStyleName();
    if(styleName == "navy")
        ui->rdoNavy->setChecked(true);
    else if(styleName == "black")
        ui->rdoBlack->setChecked(true);
    else
        ui->rdoPink->setChecked(true);
    if(appCfg->getStyleFrom())
        ui->rdoRes->setChecked(true);
    else
        ui->rdoDir->setChecked(true);
}

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

///////////////////////StationCfgForm////////////////////////////////////
StationCfgForm::StationCfgForm(QWidget *parent) : ConfigPanelBase(parent), ui(new Ui::StationCfgForm)
{
    ui->setupUi(this);
    readonly = true;
    loadStations();
    connect(ui->lwStations,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this,SLOT(currentStationChanged(QListWidgetItem*,QListWidgetItem*)));
    connect(ui->lwStations,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(editStation(QListWidgetItem*)));
    connect(ui->lwStations,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(customContextMenuRequested(QPoint)));
    setEditState();
}

StationCfgForm::~StationCfgForm()
{
    delete ui;
}

bool StationCfgForm::isDirty()
{
    if(ui->lwStations->currentItem())
        collectData(ui->lwStations->currentItem());
    if(!msDels.isEmpty())
        return true;
    for(int i = 0; i < ui->lwStations->count(); ++i){
        if(ui->lwStations->item(i)->data(DR_ES).toBool())
            return true;
    }
    return false;
}

bool StationCfgForm::save()
{
    QList<Machine*> ms;
    for(int i = 0; i < ui->lwStations->count(); ++i){
        QListWidgetItem* item = ui->lwStations->item(i);
        if(item->data(DR_ES).toBool())
            ms<<item->data(DR_OBJ).value<Machine*>();
    }
    AppConfig* acfg = AppConfig::getInstance();
    if(!acfg->saveMachines(ms)){
        myHelper::ShowMessageBoxError(tr("保存工作站信息时出错，请查看日志！"));
        return false;
    }
    if(!msDels.isEmpty()){
        foreach(Machine* mac, msDels){
            if(!acfg->removeMachine(mac)){
                myHelper::ShowMessageBoxError(tr("保存工作站信息时出错，请查看日志！"));
                return false;
            }
        }
        msDels.clear();
    }
    return true;
}

void StationCfgForm::customContextMenuRequested(const QPoint &pos)
{
    if(curUser->isSuperUser()){
        QMenu menu;
        QListWidgetItem* item = ui->lwStations->itemAt(pos);
        menu.addAction(ui->actAdd);
        if(item){
            menu.addAction(ui->actEdit);
            menu.addAction(ui->actDel);
        }
        menu.exec(ui->lwStations->mapToGlobal(pos));
    }
}

void StationCfgForm::currentStationChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if(previous)
        collectData(previous);
    Machine* m = current->data(DR_OBJ).value<Machine*>();
    showStation(m);
}

void StationCfgForm::editStation(QListWidgetItem *item)
{
    if(curUser->isSuperUser()){
        readonly = false;
        setEditState();
    }
}

void StationCfgForm::loadStations()
{
    AppConfig* acfg = AppConfig::getInstance();
    acfg->getOsTypes(osTypes);
    QList<int> codes = osTypes.keys();
    qSort(codes.begin(),codes.end());
    foreach(int code, codes)
        ui->cmbOsTypes->addItem(osTypes.value(code),code);
    ms = acfg->getAllMachines().values();
    qSort(ms.begin(),ms.end(),byMacMID);
    foreach(Machine* m, ms){
        QListWidgetItem* item = new QListWidgetItem(m->name(),ui->lwStations);
        QVariant v; v.setValue<Machine*>(m);
        item->setData(DR_OBJ,v);
        item->setData(DR_ES,false);
    }
}

void StationCfgForm::showStation(Machine* m)
{
    if(m){
        ui->edtMid->setText(QString::number(m->getMID()));
        ui->edtName->setText(m->name());
        ui->edtDesc->setText(m->description());
        ui->chkIsLocal->setChecked(m->isLocalStation());
        if(m->getType() == MT_COMPUTER)
            ui->rdoPc->setChecked(true);
        else
            ui->rdoCloud->setChecked(true);
        int index = ui->cmbOsTypes->findData(m->osType());
        ui->cmbOsTypes->setCurrentIndex(index);
    }
    else{
        ui->edtMid->clear();
        ui->edtName->clear();
        ui->edtDesc->clear();
        ui->chkIsLocal->setChecked(false);
        ui->rdoPc->setChecked(true);
        ui->cmbOsTypes->setCurrentIndex(-1);
    }
}

void StationCfgForm::setEditState()
{
    ui->edtName->setReadOnly(readonly);
    if(!readonly)
        ui->edtName->setFocus();
    ui->edtDesc->setReadOnly(readonly);
    ui->rdoPc->setEnabled(!readonly);
    ui->rdoCloud->setEnabled(!readonly);
    ui->chkIsLocal->setEnabled(!readonly);
    ui->cmbOsTypes->setEnabled(!readonly);
}

/**
 * @brief 收集当前编辑界面的数据，将改变部分保存到对象中
 */
void StationCfgForm::collectData(QListWidgetItem* item)
{
    if(item && !readonly){
        Machine* m = item->data(DR_OBJ).value<Machine*>();
        if(m->getId()==0)
            ui->edtMid->setReadOnly(true);
        bool isChanged = false;
        int mid = ui->edtMid->text().toInt();
        if(m->getMID() != mid){
            m->setMID(mid);
            isChanged = true;
        }
        if(m->name() != ui->edtName->text()){
            m->setName(ui->edtName->text());
            isChanged = true;
        }
        if(m->description() != ui->edtDesc->text()){
            m->setDescription(ui->edtDesc->text());
            isChanged = true;
        }
        if(m->isLocalStation() ^ ui->chkIsLocal->isChecked()){
            //本站设置是排他性的，要么都不设置，要么只能设置一个
            if(!m->isLocalStation() && ui->chkIsLocal->isChecked()){
                for(int i = 0; i < ui->lwStations->count(); ++i){
                    QListWidgetItem* li = ui->lwStations->item(i);
                    if(li == item)
                        continue;
                    if(ms.at(i)->isLocalStation()){
                        ms.at(i)->setLocalMachine(false);
                        li->setData(DR_ES,true);
                        break;
                    }
                }
            }
            m->setLocalMachine(ui->chkIsLocal->isChecked());
            isChanged = true;
        }
        if(m->getType() == MT_COMPUTER && !ui->rdoPc->isChecked()){
            m->setType(MT_CLOUDY);
            isChanged = true;
        }
        else if(m->getType() == MT_CLOUDY && !ui->rdoCloud->isChecked()){
            m->setType(MT_COMPUTER);
            isChanged = true;
        }
        int osType = ui->cmbOsTypes->currentData().toInt();
        if(osType != m->osType()){
            m->setOsType(osType);
            isChanged = true;
        }
        if(isChanged)
            item->setData(DR_ES,true);//需要保存
        readonly = true;
    }
}

void StationCfgForm::on_actAdd_triggered()
{
    int mid = 1;
    if(!ms.isEmpty())
        mid = ms.last()->getMID()+1;
    Machine* m = new Machine(UNID,MT_COMPUTER,mid,false,tr("新工作站"),"");
    ms<<m;
    QListWidgetItem* item = new QListWidgetItem(m->name(),ui->lwStations);
    QVariant v; v.setValue<Machine*>(m);
    item->setData(DR_OBJ,v);
    item->setData(DR_ES,true);
    ui->lwStations->setCurrentItem(item);
    showStation(m);
    readonly = false;
    setEditState();
    ui->edtName->setFocus();
}

void StationCfgForm::on_actEdit_triggered()
{
    editStation(ui->lwStations->currentItem());
}

void StationCfgForm::on_actDel_triggered()
{
    int index = ui->lwStations->currentRow();
    ui->lwStations->takeItem(index);
    msDels<<ms.takeAt(index);
    if(ui->lwStations->currentItem())
        showStation(ms.at(ui->lwStations->currentRow()));
    else
        showStation(0);
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




