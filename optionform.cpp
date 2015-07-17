#include "optionform.h"
#include "widgets.h"
#include "subject.h"
#include "myhelper.h"
#include "mainwindow.h"
#include "delegates.h"
#include "ui_appcommcfgpanel.h"
#include "ui_pztemplateoptionform.h"
#include "ui_stationcfgform.h"
#include "ui_specsubcfgform.h"
#include "ui_subsysjoincfgform.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenu>
#include <QStackedWidget>
#include <QBuffer>
#include <QDir>
#include <QTextStream>

//////////////////////////////ConfigPanels//////////////////////////////////////////
ConfigPanels::ConfigPanels(QByteArray* state, QWidget *parent) : QWidget(parent)
{
    reason = R_UNCERTAIN;
    setWindowTitle(tr("应用选项配置"));
    contentsWidget = new QListWidget(this);
    contentsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMinimumWidth(96);
    contentsWidget->setMaximumWidth(96);
    contentsWidget->setSpacing(12);

    pagesWidget = new QStackedWidget(this);
    connect(contentsWidget,SIGNAL(currentRowChanged(int)),pagesWidget,SLOT(setCurrentIndex(int)));

    QHBoxLayout* contentLayout = new QHBoxLayout;
    contentLayout->addWidget(contentsWidget);
    contentLayout->addWidget(pagesWidget);
    QPushButton* btnOk = new QPushButton(tr("确定"),this);
    connect(btnOk,SIGNAL(clicked()),this,SLOT(btnOkClicked()));
    QPushButton* btnCancel = new QPushButton(tr("取消"),this);
    connect(btnCancel,SIGNAL(clicked()),this,SLOT(btnCancelClicked()));
    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(btnOk);
    buttonsLayout->addWidget(btnCancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(contentLayout,1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    //
    appCon = AppConfig::getInstance();
    panels<<new AppCommCfgPanel(this);            //通用面板
    icons<<QIcon(":/images/Options/common.png");
    if(curUser->isSuperUser()){
        panels<<new PzTemplateOptionForm;   //凭证模板参数面板
        icons<<QIcon(":/images/Options/pzTemplate.png");
        StationCfgForm* sf = new StationCfgForm; //工作站面板
        panels<<sf;
        icons<<QIcon(":/images/Options/test1.png");
        panels<<new SpecSubCfgForm(this);
        icons<<QIcon(":/images/Options/test1.png");
    }
    QList<SubSysNameItem *> subSysLst;
    appCon->getSubSysItems(subSysLst);
    int count = subSysLst.count();
    if(count > 1){
        int preCode = subSysLst.at(count -2)->code;
        int curCode = subSysLst.at(count -1)->code;
        panels<<new SubSysJoinCfgForm(preCode,curCode,curAccount,this);
        icons<<QIcon(":/images/Options/test1.png");
    }    
    panels<<new TestPanel(this);            //测试面板
    icons<<QIcon(":/images/Options/test1.png");

    for(int i = 0; i < panels.count(); ++i)
        addPanel(panels.at(i),icons.at(i));
    qint8 curIndex = 0;
    if(state && !state->isEmpty()){
        QBuffer bf(state);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        in>>curIndex;
        bf.close();
    }
    if(curIndex >=0 && curIndex < panels.count()){
        contentsWidget->setCurrentRow(curIndex);
        pagesWidget->setCurrentIndex(curIndex);
    }


}

QByteArray *ConfigPanels::getState()
{
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 curIndex = contentsWidget->currentRow();
    out<<curIndex;
    bf.close();
    return info;
}

void ConfigPanels::addPanel(ConfigPanelBase *panel, QIcon icon)
{
    panel->setParent(this);
    QListWidgetItem* item = new QListWidgetItem(contentsWidget);
    item->setIcon(icon);
    item->setText(panel->panelName());
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    pagesWidget->addWidget(panel);
}


void ConfigPanels::closeEvent(QCloseEvent *event)
{
    if((reason == R_UNCERTAIN) && isDirty()){
        if(QMessageBox::warning(this,"",tr("保存已做出的更改吗？"),
                                QMessageBox::Yes|QMessageBox::No,
                                QMessageBox::Yes)){
            save();
        }
    }
    else if(reason == R_OK && isDirty())
        save();
    event->accept();
}

void ConfigPanels::btnOkClicked()
{
    reason = R_OK;
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}

void ConfigPanels::btnCancelClicked()
{
    reason = R_CANCEL;
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}

/**
 * @brief 轮询所有的配置面板是否配置数据被修改而未保存
 * @return
 */
bool ConfigPanels::isDirty()
{
    isChangedIndexes.clear();
    for(int i = 0; i < pagesWidget->count(); ++i){
        ConfigPanelBase* panel = qobject_cast<ConfigPanelBase*>(pagesWidget->widget(i));
        if(panel && panel->isDirty())
            isChangedIndexes<<i;
    }
    return !isChangedIndexes.isEmpty();
}

bool ConfigPanels::save()
{
    if(isChangedIndexes.isEmpty())
        return true;
    bool ok = true;
    foreach(int i, isChangedIndexes){
        ConfigPanelBase* panel = qobject_cast<ConfigPanelBase*>(pagesWidget->widget(i));
        if(panel && !panel->save()){
            ok = false;
            QMessageBox::critical(this,tr("保存出错"),tr("在保存“%1”时发生错误！").arg(panel->panelName()));
        }
    }
    return ok;
}

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
    if(ui->chkAutoHideLeftPanel->isChecked() ^ _autoHideLeftPanel){
        return true;
    }
    if(ui->chkMinToTray->isChecked() ^ _minToTray)
        return true;
    if(ui->rdoSubName->isChecked() ^ _ssubFirstlyName)
        return true;
    if(ui->chkRemain->isChecked() ^ _remainForeignUnity)
        return true;
    return false;
}

bool AppCommCfgPanel::save()
{
    if(!isDirty())
        return true;
    if(ui->chkAutoHideLeftPanel->isChecked() ^ _autoHideLeftPanel){
        _autoHideLeftPanel = ui->chkAutoHideLeftPanel->isChecked();
        _appCfg->setAutoHideLeftDock(_autoHideLeftPanel);
    }
    if(ui->chkMinToTray->isChecked() ^ _minToTray){
        _minToTray = ui->chkMinToTray->isChecked();
        _appCfg->setMinToTrayClose(_minToTray);
    }
    if(ui->rdoSubName->isChecked() ^ _ssubFirstlyName){
        _ssubFirstlyName = ui->rdoSubName->isChecked();
        _appCfg->setSSubFirstlyInputMothed(_ssubFirstlyName,true);
    }
    if(ui->chkRemain->isChecked() ^ _remainForeignUnity){
        _remainForeignUnity = ui->chkRemain->isChecked();
        _appCfg->setRemainForeignCurrencyUnity(_remainForeignUnity,true);
    }
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
        _appCfg->setAppStyleName(cssName);
    }
}

void AppCommCfgPanel::styleFromChanged(bool checked)
{
    AppConfig::getInstance()->setStyleFrom(checked);
}

void AppCommCfgPanel::init()
{
    _appCfg = AppConfig::getInstance();
    QString styleName = _appCfg->getAppStyleName();
    if(styleName == "navy")
        ui->rdoNavy->setChecked(true);
    else if(styleName == "black")
        ui->rdoBlack->setChecked(true);
    else
        ui->rdoPink->setChecked(true);
    if(_appCfg->getStyleFrom())
        ui->rdoRes->setChecked(true);
    else
        ui->rdoDir->setChecked(true);
    _ssubFirstlyName = _appCfg->ssubFirstlyInputMothed();
    _remainForeignUnity = _appCfg->remainForeignCurrencyUnity();
    if(_ssubFirstlyName)
        ui->rdoSubName->setChecked(true);
    else
        ui->rdoSubRemCode->setChecked(true);
    ui->chkRemain->setChecked(_remainForeignUnity);
    _autoHideLeftPanel = _appCfg->isAutoHideLeftDock();
    ui->chkAutoHideLeftPanel->setChecked(_autoHideLeftPanel);
    _minToTray = _appCfg->minToTrayClose();
    ui->chkMinToTray->setChecked(_minToTray);
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

void StationCfgForm::setListener(MainWindow *listener)
{
    if(listener)
        conn = connect(this,SIGNAL(localStationChanged(WorkStation*)),
                       listener,SLOT(localStationChanged(WorkStation*)));
}

StationCfgForm::~StationCfgForm()
{
    delete ui;
    disconnect(conn);
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
    AppConfig* acfg = AppConfig::getInstance();
    WorkStation* lm;
    bool lmChanged = false;
    QList<WorkStation*> ms;
    for(int i = 0; i < ui->lwStations->count(); ++i){
        QListWidgetItem* item = ui->lwStations->item(i);
        if(item->data(DR_ES).toBool()){
            WorkStation* m = item->data(DR_OBJ).value<WorkStation*>();
            ms<<m;
            if(m->isLocalStation() && (localMID != m->getMID() ||
                                       lName != m->name() ||
                                       lDesc != m->description())){
                lm = m;
                lmChanged = true;
            }
        }
    }

    if(!acfg->saveMachines(ms)){
        myHelper::ShowMessageBoxError(tr("保存工作站信息时出错，请查看日志！"));
        return false;
    }
    if(lmChanged)
        emit localStationChanged(lm);
    if(!msDels.isEmpty()){
        foreach(WorkStation* mac, msDels){
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
    WorkStation* m = current->data(DR_OBJ).value<WorkStation*>();
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
    foreach(WorkStation* m, ms){
        QListWidgetItem* item = new QListWidgetItem(m->name(),ui->lwStations);
        QVariant v; v.setValue<WorkStation*>(m);
        item->setData(DR_OBJ,v);
        item->setData(DR_ES,false);
        if(m->isLocalStation()){
            localMID = m->getMID();
            lName = m->name();
            lDesc = m->description();
        }
    }
}

void StationCfgForm::showStation(WorkStation* m)
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
        WorkStation* m = item->data(DR_OBJ).value<WorkStation*>();
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
    WorkStation* m = new WorkStation(UNID,MT_COMPUTER,mid,false,tr("新工作站"),"");
    ms<<m;
    QListWidgetItem* item = new QListWidgetItem(m->name(),ui->lwStations);
    QVariant v; v.setValue<WorkStation*>(m);
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


//////////////////////////SubSysJoinCfgForm////////////////////////////////////////
SubSysJoinCfgForm::SubSysJoinCfgForm(int src, int des, Account* account, QWidget *parent)
    :ConfigPanelBase(parent),ui(new Ui::SubSysJoinCfgForm),account(account),pre_subSys(src),subSys(des)
{
    ui->setupUi(this);
    m_loop = 0;
    ui->btnOk->setVisible(false);
    ui->btnCancel->setVisible(false);
    appCfg = AppConfig::getInstance();
    if(account)
        sSmg = account->getSubjectManager(src);
    else
        sSmg = 0;
    init();
}

SubSysJoinCfgForm::~SubSysJoinCfgForm()
{
    delete ui;
    qDeleteAll(temFstSubs);
}

/**
 * @brief 保存对配置的更改
 * @return
 */
bool SubSysJoinCfgForm::save()
{
    //如果用户已经完成配置，则请求用户确定是否终结配置，并进行二级科目的对应克隆
    if(!isCompleted){
        QList<SubSysJoinItem2*> editedItems;
        for(int i = 0; i < editTags.count(); ++i){
            if(!editTags.at(i))
                continue;
            editedItems<<ssjs.at(i);
        }
        if(!appCfg->getSubSysMaps(pre_subSys,subSys,editedItems))
            myHelper::ShowMessageBoxError(tr("在保存科目系统衔接配置信息时出错！"));
        if(determineAllComplete() && (QMessageBox::Yes ==
                QMessageBox::warning(this,"",
                                     tr("如果确定所有科目都已正确对接，则单击是"),
                                     QMessageBox::Yes|QMessageBox::No))){
            if(!appCfg->setSubSysMapConfiged(pre_subSys,subSys))
                return false;
        }
    }
}

bool SubSysJoinCfgForm::isDirty()
{
    return false;
}

int SubSysJoinCfgForm::exec()
{
    setWindowFlags(Qt::Dialog);
    setWindowModified(true);
    ui->btnOk->setVisible(true);
    ui->btnCancel->setVisible(true);
    show();
    QEventLoop loop;
    m_loop = &loop;
    loop.exec();
    m_loop = 0;
    return m_result;
}

void SubSysJoinCfgForm::setVisible(bool visible)
{
    if(!visible && m_loop)
        m_loop->exit(m_result);
    ConfigPanelBase::setVisible(visible);
}

void SubSysJoinCfgForm::closeEvent(QCloseEvent *e)
{
    hide();
    m_result = QDialog::Rejected;
    e->accept();
}

/**
 * @brief 对接的目标科目改变了
 * @param item
 */
void SubSysJoinCfgForm::destinationSubChanged(QTableWidgetItem *item)
{
    int col = item->column();
    if(col == COL_INDEX_NEWSUBNAME){
        SubSysJoinItem2* si = ssjs.at(item->row());
        QString code = ui->tview->item(item->row(),COL_INDEX_NEWSUBCODE)->text();
        if(code != si->dcode){
            si->dcode = code;
            editTags[item->row()] = true;
        }
    }
    else if(col == COL_INDEX_SUBJOIN){
        SubSysJoinItem2* si = ssjs.at(item->row());
        bool v = item->text() == defJoinStr;
        if((v && !si->isDef) || (!v && si->isDef)){
            si->isDef = v;
            editTags[item->row()] = true;
        }
    }
}

void SubSysJoinCfgForm::on_btnOk_clicked()
{
    m_result = QDialog::Accepted;
    hide();
}

void SubSysJoinCfgForm::on_btnCancel_clicked()
{
    close();
}



void SubSysJoinCfgForm::init()
{
    defJoinStr = "===>";
    mixedJoinStr = "----->";
    QHash<int,QString> names;
    QList<SubSysNameItem *> subSysLst;
    if(account)
        subSysLst = account->getSupportSubSys();
    else
        appCfg->getSubSysItems(subSysLst);
    foreach(SubSysNameItem* item, subSysLst)
        names[item->code] = item->name;
    ui->sSubSys->setText(names.value(pre_subSys));
    ui->dSubSys->setText(names.value(subSys));
    if(!appCfg->getSubSysMaps(pre_subSys,subSys,ssjs)){
        myHelper::ShowMessageBoxError(tr("在读取科目系统衔接配置信息时出错！"));
        return;
    }
    if(!appCfg->getSubCodeToNameHash(subSys,subNames)){
        myHelper::ShowMessageBoxError(tr("在读取对接的科目系统的科目时发生错误！"));
        return;
    }
    for(int i = 0; i < ssjs.count(); ++i)
        editTags<<false;
    ui->tview->setRowCount(ssjs.count());
    SubSysJoinItem2* item;
    QTableWidgetItem* ti;
    FirstSubject* fsub;
    for(int i = 0; i < ssjs.count(); ++i){
        item = ssjs.at(i);
        ti = new QTableWidgetItem(item->scode);
        ui->tview->setItem(i,COL_INDEX_SUBCODE,ti);
        if(sSmg)
            fsub = sSmg->getFstSubject(item->scode);
        else{
            fsub = appCfg->getFirstSubject(pre_subSys,item->scode);
            temFstSubs<<fsub;
        }
        ti = new QTableWidgetItem(fsub?fsub->getName():"");
        ui->tview->setItem(i,COL_INDEX_SUBNAME,ti);
        ti = new QTableWidgetItem(item->isDef?defJoinStr:mixedJoinStr);
        ti->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        ui->tview->setItem(i,COL_INDEX_SUBJOIN,ti);
        ti = new QTableWidgetItem(item->dcode);
        ui->tview->setItem(i,COL_INDEX_NEWSUBCODE,ti);
        ti = new QTableWidgetItem(subNames.value(item->dcode));
        ui->tview->setItem(i,COL_INDEX_NEWSUBNAME,ti);
    }
    QStringList slSigns,slDisplays;
    slSigns<<defJoinStr<<mixedJoinStr;
    slDisplays<<tr("默认对接")<<tr("混合对接");
    if(account)
        isCompleted = account->isSubSysConfiged(subSys);
    else
        appCfg->getSubSysMapConfiged(pre_subSys,subSys,isCompleted);
    if(!curAccount && (curUser->isAdmin() || curUser->isSuperUser())){
        SubSysJoinCfgItemDelegate* delegate = new SubSysJoinCfgItemDelegate(subNames,slDisplays,slSigns);
        delegate->setReadOnly(isCompleted);
        ui->tview->setItemDelegate(delegate);
        connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(destinationSubChanged(QTableWidgetItem*)));
    }
}

/**
 * @brief 扫描配置项列表中的所有条目，确定是否所有科目都完成了配置
 */
bool SubSysJoinCfgForm::determineAllComplete()
{
    for(int i = 0; i < ui->tview->rowCount(); ++i){
        QString code = ui->tview->item(i,COL_INDEX_NEWSUBCODE)->text();
        if(code.isEmpty())
            return false;
    }
    return true;
}


////////////////////////SpecSubCfgForm/////////////////////////////////////
SpecSubCfgForm::SpecSubCfgForm(QWidget *parent) :
    ConfigPanelBase(parent),
    ui(new Ui::SpecSubCfgForm)
{
    ui->setupUi(this);
    appCfg = AppConfig::getInstance();
    QList<AppConfig::SpecSubCode> gcodes;
    QStringList gnames;
    appCfg->getSpecSubGenricNames(gcodes,gnames);
    ui->tw->setRowCount(gcodes.count());
    for(int r = 0; r < gcodes.count(); ++r){
        QTableWidgetItem* item = new QTableWidgetItem(gnames.at(r));
        item->setData(Qt::UserRole,gcodes.at(r));
        ui->tw->setItem(r,0,item);
    }
    QList<SubSysNameItem *> items;
    appCfg->getSubSysItems(items);
    for(int i = 0; i < items.count(); ++i){
        SubSysNameItem* si = items.at(i);
        ui->cmbSubSys->addItem(si->name,si->code);
        codes[si->code] = QStringList();
        names[si->code] = QStringList();
        if(!appCfg->getAllSpecSubNameForSubSys(si->code,codes[si->code],names[si->code])){
            myHelper::ShowMessageBoxError(tr("获取特定科目代码和名称时发生错误！"));
            return;
        }
        if(codes.value(si->code).count() != ui->tw->rowCount()){
            myHelper::ShowMessageBoxWarning(tr("特定科目配置数量异常！\n系统要求 %1 个，但实际有 %2 个！")
                                            .arg(ui->tw->rowCount()).arg(codes.value(si->code).count()));
            return;
        }
    }
    if(!items.isEmpty())
        subSysChanged(0);
    connect(ui->cmbSubSys,SIGNAL(currentIndexChanged(int)),this,SLOT(subSysChanged(int)));
}

SpecSubCfgForm::~SpecSubCfgForm()
{
    delete ui;
}

bool SpecSubCfgForm::isDirty()
{
    return false;
}

bool SpecSubCfgForm::save()
{
    return true;
}

void SpecSubCfgForm::subSysChanged(int index)
{
    int sysCode = ui->cmbSubSys->itemData(index).toInt();
    for(int r=0; r<ui->tw->rowCount(); ++r){
        QTableWidgetItem* item = ui->tw->item(r,1);
        if(!item)
            item = new QTableWidgetItem;
        item->setText(codes[sysCode].at(r));
        ui->tw->setItem(r,1,item);
        item = ui->tw->item(r,2);
        if(!item)
            item = new QTableWidgetItem;
        item->setText(names[sysCode].at(r));
        ui->tw->setItem(r,2,item);
    }

}

/**
 * @brief 导出特定科目配置信息
 */
void SpecSubCfgForm::on_btnExport_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存文件"),
                                QDir::homePath(),
                                tr("Application Config File(*.cfg)"));
    if(fileName.isEmpty())
        return;
    if(!fileName.contains("."))
        fileName = fileName + ".cfg";
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly|QFile::Text)){
        myHelper::ShowMessageBoxError(tr("打开文件时出错！"));
        return;
    }
    QTextStream ts(&file);
    ts<<QString("config=%1\n").arg(CFG_FILE_TAG);
    int mv, sv;
    appCfg->getAppCfgVersion(mv,sv,BDVE_RIGHTTYPE);
    ts<<QString("version=%1.%2\n").arg(mv).arg(sv);
    ts<<QString("SubjectSystemCount=%1\n").arg(codes.count());
    QList<int> sysCodes = codes.keys();
    qSort(sysCodes.begin(),sysCodes.end());
    for(int i = 0; i < sysCodes.count(); ++i){
        int sysCode = sysCodes.at(i);
        int subNumbers = codes.value(sysCode).count();
        ts<<QString("%1=%2\n").arg(CFG_FILE_KEY_SUBSYS).arg(sysCode);//每个科目系统配置的科目数
        ts<<QString("%1=%2\n").arg(CFG_FILE_KEY_SYSNUM).arg(subNumbers);
        for(int j = 0; j < subNumbers; ++j){
            ts<<QString("%1||%2||%3\n")
                .arg(ui->tw->item(j,0)->data(Qt::UserRole).toInt())
                .arg(codes.value(sysCode).at(j)).arg(names.value(sysCode).at(j));
        }
    }
    ts.flush();
    file.close();
}

/**
 * @brief 导入特定科目配置信息
 */
void SpecSubCfgForm::on_btnImport_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"),
                                                    QDir::homePath(),
                                                    tr("Application Config File(*.cfg)"));
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    if(!file.open(QFile::ReadOnly|QFile::Text)){
        myHelper::ShowMessageBoxError(tr("打开文件时出错！"));
        return;
    }
    QTextStream ts(&file);
    QString line = ts.readLine();
    QStringList ls = line.split("=");
    if(ls.count() != 2 || ls.at(0) != "config"){
        myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
        return;
    }
    if(ls.at(1) != CFG_FILE_TAG){
        myHelper::ShowMessageBoxError(tr("该文件不是特定科目配置文件，“%1”").arg(line));
       return;
    }
    line = ts.readLine();
    int fmv=0,fsv=0;
    if(!appCfg->parseVersionFromText(line,fmv,fsv)){
        myHelper::ShowMessageBoxWarning(tr("文件“%1”无法识别版本号！").arg(fileName));
        return;
    }
    int mv=0,sv=0;
    appCfg->getAppCfgVersion(mv,sv,BDVE_SPECSUBJECT);
    if(fmv < mv || (fmv == mv && fsv < sv)){
        myHelper::ShowMessageBoxWarning(tr("配置文件的版本（%1.%2）低于系统版本（%3.%4）")
                                        .arg(fmv).arg(fsv).arg(mv).arg(sv));
        return;
    }
    if(fmv==mv && fsv==sv){
        myHelper::ShowMessageBoxWarning(tr("配置文件的版本与系统版本相同（%1.%2）")
                                        .arg(fmv).arg(fsv));
        return;
    }
    QString info;
    line = ts.readLine();
    ls = line.split("=");
    if(ls.count() != 2 || ls.at(0) != CFG_FILE_KEY_SYSNUM){
        myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
        return;
    }
    bool ok;
    int sysNum = ls.at(1).toInt(&ok);
    if(!ok){
        myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
        return;
    }
    for(int i = 0; i < sysNum; ++i){
        line = ts.readLine();
        ls = line.split("=");
        if(ls.count() != 2 || ls.at(0) != CFG_FILE_KEY_SUBSYS){
            myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
            return;
        }
        int sysCode = ls.at(1).toInt(&ok);
        if(!ok){
            myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
            return;
        }
        line = ts.readLine();
        ls = line.split("=");
        if(ls.count() != 2 || ls.at(0) != CFG_FILE_KEY_SYSNUM){
            myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
            return;
        }
        int subNum = ls.at(1).toInt(&ok);
        if(!ok){
            myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
            return;
        }
        if(subNum != ui->tw->rowCount()){
            myHelper::ShowMessageBoxWarning(tr("导入的科目配置数（%1）与系统要求的（%2）不一致！")
                                            .arg(subNum).arg(ui->tw->rowCount()));
            return;
        }
        QList<AppConfig::SpecSubCode> gcodes;
        QStringList codes,names;
        for(int j = 0; j < subNum; ++j){
            line = ts.readLine();
            if(line.isEmpty()){
                myHelper::ShowMessageBoxError(tr("特定科目配置数不足，科目系统代码是“%1”").arg(sysCode));
                return;
            }
            ls = line.split("||");
            if(ls.count() != 3){
                myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
                return;
            }
            int gcode = ls.at(0).toInt(&ok);
            if(!ok){
                myHelper::ShowMessageBoxError(tr("文件格式错误，“%1”").arg(line));
                return;
            }
            gcodes<<(AppConfig::SpecSubCode)gcode;
            codes<<ls.at(1);
            names<<ls.at(2);
        }        
        if(!appCfg->saveSpecSubNameForSysSys(sysCode,gcodes,codes,names)){
            myHelper::ShowMessageBoxError(tr("保存科目系统%1时发生错误！").arg(sysCode));
            return;
        }
        this->codes[sysCode] = codes;
        this->names[sysCode] = names;
    }
    if(sysNum > 0){
        subSysChanged(0);
        myHelper::ShowMessageBoxInfo(tr("导入成功！"));
    }
}



/////////////////////////////

TestPanel::TestPanel(QWidget *parent):ConfigPanelBase(parent)
{
    QLabel* l = new QLabel(tr("测试页"),this);
    QHBoxLayout* ly = new QHBoxLayout;
    ly->addWidget(l);
    setLayout(ly);
}






