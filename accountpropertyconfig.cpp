#include "accountpropertyconfig.h"
#include "account.h"
#include "config.h"
#include "subject.h"
#include "dbutil.h"
#include "widgets.h"
#include "delegates.h"


#include <QListWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>

ApcBase::ApcBase(Account *account, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcBase)
{
    ui->setupUi(this);
}

ApcBase::~ApcBase()
{
    delete ui;
}

//////////////////////////////ApcSuite////////////////////////////////////////////////
ApcSuite::ApcSuite(Account *account, QWidget *parent) :
    QWidget(parent),ui(new Ui::ApcSuite),account(account)
{
    ui->setupUi(this);
    iniTag = false;
    editAction = EA_NONE;
    connect(ui->lw,SIGNAL(currentRowChanged(int)),this,SLOT(curSuiteChanged(int)));
    connect(ui->lw,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(suiteDbClicked(QListWidgetItem*)));

}

ApcSuite::~ApcSuite()
{
    delete ui;
}

void ApcSuite::init()
{
    if(iniTag)
        return;
    suites = account->getAllSuites();
    foreach(SubSysNameItem* sni, account->getSupportSubSys()){
        if(sni->isImport)
            ui->subSys->addItem(sni->name,sni->code);
    }

    QListWidgetItem* item;
    foreach(AccountSuiteRecord* as, suites){
        item = new QListWidgetItem(as->name);
        ui->lw->addItem(item);
    }
    iniTag = true;
    curSuiteChanged(-1);
}

/**
 * @brief 显示选中帐套的信息或清除
 * @param index
 */
void ApcSuite::curSuiteChanged(int index)
{
    if(index > -1){
        AccountSuiteRecord* as = suites.at(index);
        ui->name->setText(as->name);
        ui->year->setValue(as->year);
        ui->smonth->setValue(as->startMonth);
        ui->emonth->setValue(as->endMonth);
        ui->rmonth->setValue(as->recentMonth);
        ui->isCur->setChecked(as->isClosed);
        ui->isUsed->setChecked(as->isUsed);
        ui->subSys->setCurrentIndex(ui->subSys->findData(as->subSys));
        ui->btnEdit->setEnabled(true);
        ui->btnUsed->setEnabled(!as->isUsed);
    }
    else{
        ui->name->clear();
        ui->year->clear();
        ui->smonth->clear();
        ui->emonth->clear();
        ui->rmonth->clear();
        ui->subSys->setCurrentIndex(-1);
        ui->isCur->setChecked(false);
        ui->isUsed->setChecked(false);
        ui->btnEdit->setEnabled(false);
        ui->btnUsed->setEnabled(false);
    }
}

void ApcSuite::suiteDbClicked(QListWidgetItem *item)
{
    on_btnEdit_clicked();
}

void ApcSuite::on_btnNew_clicked()
{
    AccountSuiteRecord* as = new AccountSuiteRecord;
    as->id = UNID;
    if(suites.isEmpty()){
        as->year = account->getStartDate().year();
        as->subSys = 0;
    }
    else{
        as->year = suites.last()->year + 1;
        as->subSys = suites.last()->subSys;
    }
    as->name = tr("%1年").arg(as->year);
    as->startMonth = 1;
    as->endMonth = 1;
    as->recentMonth = 1;
    as->isClosed = false;
    as->isUsed = false;
    suites<<as;
    QListWidgetItem* item = new QListWidgetItem(as->name);
    ui->lw->addItem(item);
    ui->lw->setCurrentRow(ui->lw->count()-1);
    editAction = EA_NEW;
    enWidget(true);
}

void ApcSuite::on_btnEdit_clicked()
{
    //启动编辑操作
    if(editAction == EA_NONE){
        stack_s.push(ui->name->text());
        stack_i.push(ui->subSys->itemData(ui->subSys->currentIndex()).toInt());
        stack_i.push(ui->isUsed->isChecked()?1:0);
        editAction = EA_EDIT;
        enWidget(true);
    }
    else{ //取消编辑或新建操作
        if(editAction == EA_EDIT){
            ui->name->setText(stack_s.pop());
            ui->isUsed->setChecked((stack_i.pop()==1)?true:false);
            ui->subSys->setCurrentIndex(ui->subSys->findData(stack_i.pop()));
        }
        else if(editAction == EA_NEW){
            delete ui->lw->takeItem(ui->lw->count()-1);
            delete suites.takeAt(suites.count()-1);
            ui->lw->setCurrentRow(ui->lw->count()-1);
        }
        editAction = EA_NONE;
        enWidget(false);
    }
}

/**
 * @brief 启用帐套
 */
void ApcSuite::on_btnUsed_clicked()
{
    int row = ui->lw->currentRow();
    AccountSuiteRecord* as = suites.at(row);
    int dc = as->subSys;
    int sc = dc;
    if(row > 1)
        sc = suites.at(row-1)->subSys;
    //要启用帐套，且该帐套使用了与前一个帐套不同的科目系统，则必须满足两个条件
    //1、科目系统的衔接配置（包括科目的克隆）必须完成；
    //2、科目余额的衔接必须完成；
    //以下代码必须在确认如何正确进行衔接配置后再做测试
    if(dc != sc){
        bool isCompletedSubSys = false;
        bool isCompletedSubCloned = false;
        bool isCompletedExtraJoined = false;
        if(!account->isCompleteSubSysCfg(sc,dc,isCompletedSubSys,isCompletedSubCloned)){
            QMessageBox::critical(this,tr("出错信息"),tr("在读取配置变量时发生错误"));
            return;
        }
        if(!isCompletedSubSys || !isCompletedSubCloned){
            QMessageBox::warning(this,tr("警告提示"),tr("科目衔接或科目克隆没有完成，无法启用该帐套！"));
            return;
        }
        if(!account->isCompletedExtraJoin(sc,dc,isCompletedExtraJoined)){
            QMessageBox::critical(this,tr("出错信息"),tr("在读取配置变量时发生错误"));
            return;
        }
        if(!isCompletedExtraJoined &&
                QMessageBox::information(this,tr("提示信息"),
                                         tr("要启用使用了新科目系统的帐套，必须要衔接好余额。按确定，则系统自动为你衔接余额。"),
                                         QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Cancel)
            return;
        if(!joinExtra(as->year,sc,dc)){
            QMessageBox::critical(this,tr("出错信息"),tr("在衔接余额时发生错误！"));
            return;
        }
    }

    as->isUsed = true;
    if(!account->saveSuite(as))
        QMessageBox::critical(this,tr("出错信息"),tr("在保存帐套时发生错误！"));
    ui->btnUsed->setEnabled(false);
    ui->isUsed->setChecked(true);
}

/**
 * @brief 提交对帐套的更改
 */
void ApcSuite::on_btnCommit_clicked()
{
    if(editAction == EA_NONE)
        return;
    AccountSuiteRecord* as = suites.at(ui->lw->currentRow());
    if(as->name != ui->name->text())
        ui->lw->currentItem()->setText(ui->name->text());
    as->name = ui->name->text();
    as->subSys = ui->subSys->itemData(ui->subSys->currentIndex()).toInt();
    as->isUsed = ui->isUsed->isChecked();
    if(editAction == EA_NEW)
        account->addSuite(as);
    if(!account->saveSuite(as))
        QMessageBox::critical(this,tr("出错信息"),tr("在保存帐套时发生错误！"));
    stack_i.clear();
    stack_s.clear();
    editAction = EA_NONE;
    enWidget(false);
}



void ApcSuite::enWidget(bool en)
{
    //ui->year->setReadOnly(!en);
    //ui->smonth->setReadOnly(!en);
    //ui->emonth->setReadOnly(!en);
    //ui->rmonth->setReadOnly(!en);
    ui->name->setReadOnly(!en);
    ui->subSys->setEnabled(en);
    ui->btnCommit->setEnabled(en);
    ui->btnEdit->setText(en?tr("取消"):tr("编辑"));
}

/**
 * @brief 余额衔接
 * @param year  新帐套的年份
 * @param sc    新帐套使用的科目系统代码
 * @param dc    前一个帐套使用的科目系统代码
 * @return
 */
bool ApcSuite::joinExtra(int year, int sc, int dc)
{
    //读取科目映射条目配置项
    //读取前一个帐套的最后月份的余额
    //将余额表的键的科目id部分替换为新的科目
    QList<SubSysJoinItem*> jItems;
    if(!account->getSubSysJoinCfgInfo(sc,dc,jItems)){
        QMessageBox::critical(this,tr("出错信息"),tr("在读取科目衔接配置项时发生错误！"));
        return false;
    }
    QHash<int,Double> pvf,mvf,pvs,mvs;  //源科目系统的余额表
    QHash<int,MoneyDirection> pdf,pds;
    QHash<int,Double> pvf_n,mvf_n,pvs_n,mvs_n; //余额副本
    QHash<int,MoneyDirection> pdf_n,pds_n;
    if(!account->getDbUtil()->readExtraForPm(year-1,12,pvf,pdf,pvs,pds)){
        QMessageBox::critical(this,tr("出错信息"),tr("在读取%1年12月的科目原币余额时发生错误！").arg(year-1));
        return false;
    }
    if(!account->getDbUtil()->readExtraForMm(year-1,12,mvf,mvs)){
        QMessageBox::critical(this,tr("出错信息"),tr("在读取%1年12月的科目本币余额时发生错误！").arg(year-1));
        return false;
    }
    //建立主目和子目的id映射表
    QHash<int,int> fsubMaps,ssubMaps;
    foreach(SubSysJoinItem* item,jItems){
        fsubMaps[item->sFSub->getId()] = item->dFSub->getId();
        for(int i = 0; i < item->ssubMaps.count(); i+=2)
            ssubMaps[item->ssubMaps.at(i)] = item->ssubMaps.at(i+1);
    }
    //处理主目原币余额及其方向
    QHashIterator<int,Double>* it = new QHashIterator<int,Double>(pvf);
    int id,mt,key;
    while(it->hasNext()){
        it->next();
        id = it->key()/10;
        mt = it->key()%10;
        if(!fsubMaps.contains(id)){
            FirstSubject* fsub = account->getSubjectManager(sc)->getFstSubject(id);
            QMessageBox::warning(this,tr("警告信息"),tr("在衔接原币余额时，发现一个未建立衔接映射的一级科目（%1），操作无法继续！").arg(fsub->getName()));
            return false;
        }
        key = fsubMaps.value(id) * 10 + mt;
        //注意：如果有多个源科目被映射到同一个目的科目，则要考虑汇总余额，并最终确定方向，但不知是否会出现此种情形
        pvf_n[key] = it->value();
        pdf_n[key] = pdf.value(it->key());
    }
    //处理主目本币余额
    it = new QHashIterator<int,Double>(mvf);
    while(it->hasNext()){
        it->next();
        id = it->key()/10;
        mt = it->key()%10;
        if(!fsubMaps.contains(id)){
            FirstSubject* fsub = account->getSubjectManager(sc)->getFstSubject(id);
            QMessageBox::warning(this,tr("警告信息"),tr("在衔接本币余额时，发现一个未建立衔接映射的一级科目（%1），操作无法继续！").arg(fsub->getName()));
            return false;
        }
        key = fsubMaps.value(id) * 10 + mt;
        mvf_n[key ] = it->value();
    }
    //处理子目原币余额及其方向
    it = new QHashIterator<int,Double>(pvs);
    while(it->hasNext()){
        it->next();
        id = it->key()/10;
        mt = it->key()%10;
        if(!ssubMaps.contains(id)){
            SecondSubject* ssub = account->getSubjectManager(sc)->getSndSubject(id);
            QMessageBox::warning(this,tr("警告信息"),tr("在衔接本币余额时，发现一个未建立衔接映射的一级科目（%1），操作无法继续！").arg(ssub->getName()));
            return false;
        }
        key = ssubMaps.value(id) * 10 + mt;
        pvs_n[key] = it->value();
        pds_n[key] = pds.value(it->key());
    }
    //处理子目本币余额
    it = new QHashIterator<int,Double>(mvs);
    while(it->hasNext()){
        it->next();
        id = it->key()/10;
        mt = it->key()%10;
        if(!ssubMaps.contains(id)){
            SecondSubject* ssub = account->getSubjectManager(sc)->getSndSubject(id);
            QMessageBox::warning(this,tr("警告信息"),tr("在衔接本币余额时，发现一个未建立衔接映射的一级科目（%1），操作无法继续！").arg(ssub->getName()));
            return false;
        }
        key = ssubMaps.value(id) * 10 + mt;
        mvs_n[key] = it->value();
    }

    //保存余额的衔接副本
    if(!account->getDbUtil()->saveExtraForPm(year-1,12,pvf,pdf,pvs,pds)){
        QMessageBox::critical(this,tr("出错信息"),tr("在保存%1年12月的科目原币余额衔接副本时发生错误！").arg(year-1));
        return false;
    }
    if(!account->getDbUtil()->saveExtraForMm(year-1,12,mvf,mvs)){
        QMessageBox::critical(this,tr("出错信息"),tr("在保存%1年12月的科目本币余额衔接副本时发生错误！").arg(year-1));
        return false;
    }
    return true;
}


////////////////////////////ApcBank///////////////////////////////////////////////
ApcBank::ApcBank(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcBank)
{
    ui->setupUi(this);
}

ApcBank::~ApcBank()
{
    delete ui;
}

///////////////////////////ApcSubject////////////////////////////////////////////
ApcSubject::ApcSubject(Account *account, QWidget *parent) :
    QWidget(parent), ui(new Ui::ApcSubject),account(account)
{
    ui->setupUi(this);

    iniTag_subsys = false;
    iniTag_sub = false;
    iniTag_ni = false;
    editAction = APCEA_NONE;
    curFSub = NULL;
    curSSub = NULL;
    curNI = NULL;
    curNiCls = 0;
    subSysNames = account->getSupportSubSys();
}

ApcSubject::~ApcSubject()
{
    delete ui;
}

void ApcSubject::init()
{
//    APC_SUB_PAGEINDEX page = (APC_SUB_PAGEINDEX)ui->tw->currentIndex();
//    switch(page){
//    case APCS_SYS:
//        init_subsys();
//        break;
//    }
    on_tw_currentChanged(ui->tw->currentIndex());
}

void ApcSubject::save()
{
    foreach(SubSysNameItem* item, subSysNames){
        if(!account->getSubjectManager(item->code)->save()){
            QMessageBox::critical(this,tr("出错信息"),tr("在保存科目时发生错误！"));
            return;
        }
    }
}

/**
 * @brief 科目系统表的导入按钮被单击（导入指定科目系统的科目到当前账户）
 */
void ApcSubject::importBtnClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    for(int i = 0; i < ui->tv_subsys->rowCount(); ++i){
        QPushButton* w = qobject_cast<QPushButton*>(ui->tv_subsys->cellWidget(i,3));
        if(!w)
            continue;
        if(btn == w){
            int subSys = subSysNames.at(i)->code;
            QString fname = QFileDialog::getOpenFileName(this,tr("打开文件"),"./datas/basicdatas/","Sqlite (*.dat)");
            if(fname.isEmpty())
                return;
            if(!account->getDbUtil()->importFstSubjects(subSys,fname))
                QMessageBox::critical(this,tr("出错信息"),tr("在从文件“%1”导入科目系统代码为“%2”的一级科目时发生错误").arg(fname).arg(subSys));
            if(!account->getSubjectManager(subSys)->loadAfterImport()){
                QMessageBox::critical(this,tr("出错信息"),tr("将刚导入的科目装载到科目管理器对象期间发生错误"));
                return;
            }
            ui->tv_subsys->setCellWidget(i,3,NULL);
            ui->tv_subsys->setItem(i,3,new QTableWidgetItem(tr("已导入")));
            return;
        }
    }
}

/**
 * @brief 科目系统表的配置按钮被单击
 */
void ApcSubject::subSysCfgBtnClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    for(int i = 0; i < ui->tv_subsys->rowCount(); ++i){
        QPushButton* w = qobject_cast<QPushButton*>(ui->tv_subsys->cellWidget(i,4));
        if(!w)
            continue;
        if(btn == w){
            SubSysJoinCfgForm* form = new SubSysJoinCfgForm(subSysNames.at(i-1)->code,subSysNames.at(i)->code,account);
            if(form->exec() == QDialog::Accepted)
                form->save();
            return;
        }
    }
}

/**
 * @brief 当用户选择一个科目系统后，装载此科目系统所属的所有一级科目
 * @param checked
 */
void ApcSubject::selectedSubSys(bool checked)
{
    if(!checked)
        return;
    //科目配置清除界面
    //判断是选择了哪个科目系统再装载对应的科目
    QRadioButton* btn = qobject_cast<QRadioButton*>(sender());
    if(!btn)
        return;
    int subSys;
    foreach(SubSysNameItem* sn,subSysNames){
        if(sn->name == btn->text()){
            subSys = sn->code;
            curSubMgr = account->getSubjectManager(subSys);
            break;
        }
    }
    loadFSub(subSys);
}

/**
 * @brief 当前选择的要显示的一级科目的类别改变了（用来控制显示指定类别的一级科目）
 * @param index
 */
void ApcSubject::curFSubClsChanged(int index)
{
    SubjectClass sc = (SubjectClass)ui->cmbFSubCls->itemData(index).toInt();
    for(int i = 0; i < ui->lwFSub->count(); ++i){
        FirstSubject* fsub = ui->lwFSub->item(i)->data(Qt::UserRole).value<FirstSubject*>();
        if((sc == SC_ALL) || (sc == fsub->getSubClass()))
            ui->lwFSub->item(i)->setHidden(false);
        else
            ui->lwFSub->item(i)->setHidden(true);
    }
}

/**
 * @brief 双击一级科目启动编辑功能
 * @param item
 */
void ApcSubject::fsubDBClicked(QListWidgetItem *item)
{
    on_btnFSubEdit_clicked();
}

/**
 * @brief 选择的一级科目改变了
 * @param row
 */
void ApcSubject::curFSubChanged(int row)
{
    if(row == -1){
        curFSub = NULL;
        ui->btnFSubEdit->setEnabled(false);
        ui->btnFSubCommit->setEnabled(false);
    }
    else{
        curFSub = ui->lwFSub->currentItem()->data(Qt::UserRole).value<FirstSubject*>();
        ui->btnFSubEdit->setEnabled(true);
    }
    enFSubWidget(false);
    curSSub = NULL;
    viewFSub();
    viewSSub();
}

/**
 * @brief 选择的一级科目改变了
 * @param row
 */
void ApcSubject::curSSubChanged(int row)
{
    if(row == -1){
        curSSub = NULL;
        ui->btnSSubCommit->setEnabled(false);
        ui->btnSSubDel->setEnabled(false);
        ui->btnSSubEdit->setEnabled(false);
    }
    else{
        curSSub = ui->lwSSub->item(row)->data(Qt::UserRole).value<SecondSubject*>();
        ui->btnSSubDel->setEnabled(true);
        ui->btnSSubEdit->setEnabled(true);
    }
    enSSubWidget(false);
    viewSSub();
}

void ApcSubject::ssubDBClicked(QListWidgetItem *item)
{
    on_btnSSubEdit_clicked();
}

/**
 * @brief 跟踪科目配置选项页选择的变化，按需调用各自的初始化函数
 * @param index
 */
void ApcSubject::on_tw_currentChanged(int index)
{
    APC_SUB_PAGEINDEX page = (APC_SUB_PAGEINDEX)index;
    switch(page){
    case APCS_SYS:
        init_subsys();
        break;
    case APCS_NAME:
        init_NameItems();
        break;
    case APCS_SUB:
        init_subs();
        break;
    }
}

/**
 * @brief 跟踪用户对名称类别的选择，并显示名称类别的细节
 * @param curRow
 */
void ApcSubject::currentNiClsRowChanged(int curRow)
{
    QListWidgetItem* item = ui->lwNiCls->item(curRow);
    curNiCls = item?item->data(Qt::UserRole).toInt():0;
    ui->btnNiClsEdit->setEnabled(curNiCls);
    ui->btnDelNiCls->setEnabled(curNiCls);
    viewNiCls(curNiCls);
}

/**
 * @brief 双击允许对名称条目类别进行编辑
 * @param item
 */
void ApcSubject::niClsDoubleClicked(QListWidgetItem *item)
{
    //enNiClsWidget(true);
    on_btnNiClsEdit_clicked();
}

/**
 * @brief 跟踪用户对名称条目的选择，并显示名称条目的细节
 * @param curRow
 */
void ApcSubject::currentNiRowChanged(int curRow)
{
    QListWidgetItem* item = ui->lwNI->item(curRow);
    if(!item){
        ui->btnNiEdit->setEnabled(false);
        ui->btnNiCommit->setEnabled(false);
        ui->btnDelNI->setEnabled(false);
        viewNI(0);
    }
    else{
        ui->btnNiEdit->setEnabled(true);
        ui->btnDelNI->setEnabled(true);
        curNI = item->data(Qt::UserRole).value<SubjectNameItem*>();
        viewNI(curNI);
    }
}

/**
 * @brief 双击运行对名称条目进行编辑
 * @param item
 */
void ApcSubject::niDoubleClicked(QListWidgetItem *item)
{
    //curNI = item->data(Qt::UserRole).value<SubjectNameItem*>();
    on_btnNiEdit_clicked();
}

/**
 * @brief 初始化科目系统配置页
 */
void ApcSubject::init_subsys()
{
    ui->tv_subsys->setRowCount(subSysNames.count());
    QTableWidgetItem* item;
    for(int i = 0; i < subSysNames.count(); ++i){
        SubSysNameItem* si = subSysNames.at(i);
        item = new QTableWidgetItem(QString::number(si->code));
        ui->tv_subsys->setItem(i,0,item);
        item = new QTableWidgetItem(si->name);
        ui->tv_subsys->setItem(i,1,item);
        item = new QTableWidgetItem(si->explain);
        ui->tv_subsys->setItem(i,2,item);
        if(si->isImport){
            item = new QTableWidgetItem(tr("已导入"));
            ui->tv_subsys->setItem(i,3,item);
        }
        else{
            QPushButton* btn = new QPushButton(tr("导入"),this);
            ui->tv_subsys->setCellWidget(i,3,btn);
            connect(btn,SIGNAL(clicked()),this,SLOT(importBtnClicked()));
        }

        if(i > 0){
            QPushButton* btn = new QPushButton(tr("配置"),this);
            ui->tv_subsys->setCellWidget(i,4,btn);
            connect(btn,SIGNAL(clicked()),this,SLOT(subSysCfgBtnClicked()));
        }
    }
    iniTag_subsys = true;
}

/**
 * @brief ApcSubject::init_NameItems
 *  初始化名称条目配置页
 */
void ApcSubject::init_NameItems()
{
    if(iniTag_ni)
        return;
    QVariant v;
    ui->niClsView->addItem(tr("所有"),0);
    QListWidgetItem* item;
    NiClasses = SubjectManager::getAllNICls();
    QHashIterator<int,QStringList> it(NiClasses);
    while(it.hasNext()){
        it.next();
        QString clsName = it.value().at(0);
        ui->niCls->addItem(clsName,it.key());
        ui->niClsView->addItem(clsName,it.key());
        item = new QListWidgetItem(clsName);
        item->setData(Qt::UserRole,it.key());
        ui->lwNiCls->addItem(item);
    }
    loadNameItems();

    iniTag_ni = true;
    connect(ui->lwNiCls,SIGNAL(currentRowChanged(int)),this,SLOT(currentNiClsRowChanged(int)));
    connect(ui->lwNI,SIGNAL(currentRowChanged(int)),this,SLOT(currentNiRowChanged(int)));
    if(ui->lwNI->count())
        currentNiRowChanged(ui->lwNI->currentRow());
    connect(ui->niClsView,SIGNAL(currentIndexChanged(int)),this,SLOT(loadNameItems()));
    connect(ui->lwNI,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(niDoubleClicked(QListWidgetItem*)));
    connect(ui->lwNiCls,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(niClsDoubleClicked(QListWidgetItem*)));

}

/**
 * @brief ApcSubject::init_subs
 *  初始化科目配置页
 */
void ApcSubject::init_subs()
{
    if(iniTag_sub)
        return;
    //初始化科目系统的单选按钮列表
    QRadioButton* initRb;
    int row = 0;
    int rows = ui->gridLayout->rowCount();
    QVBoxLayout* l = new QVBoxLayout(this);
    foreach(SubSysNameItem* sn, subSysNames){
        row++;
        QRadioButton* r = new QRadioButton(sn->name,this);
        if(row == 1)
            initRb = r;
        //ui->gridLayout->addWidget(r,rows+row,1);
        l->addWidget(r);
        connect(r,SIGNAL(toggled(bool)),this,SLOT(selectedSubSys(bool)));
    }
    ui->gb_subSys->setLayout(l);
    iniTag_sub = true;
    if(initRb)
        initRb->setChecked(true);
    connect(ui->lwFSub,SIGNAL(currentRowChanged(int)),SLOT(curFSubChanged(int)));
    connect(ui->lwFSub,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(fsubDBClicked(QListWidgetItem*)));
    connect(ui->lwSSub,SIGNAL(currentRowChanged(int)),SLOT(curSSubChanged(int)));
    connect(ui->lwSSub,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(ssubDBClicked(QListWidgetItem*)));
    connect(ui->cmbFSubCls,SIGNAL(currentIndexChanged(int)),this,SLOT(curFSubClsChanged(int)));
}

/**
 * @brief 装载指定科目系统的一级科目
 * @param subSys    科目系统代码
 */
void ApcSubject::loadFSub(int subSys)
{
    ui->cmbFSubCls->clear();
    QHash<SubjectClass,QString> subClses = curSubMgr->getFstSubClass();
    ui->cmbFSubCls->addItem(tr("所有"),0);
    QHashIterator<SubjectClass,QString> ic(subClses);
    while(ic.hasNext()){
        ic.next();
        ui->FSubCls->addItem(ic.value(),(int)ic.key());
        ui->cmbFSubCls->addItem(ic.value(),(int)ic.key());
    }

    ui->lwFSub->clear();
    ui->lwSSub->clear();
    //SubjectManager* smg = account->getSubjectManager(subSys);
    QListWidgetItem* item;
    QVariant v;
    FirstSubject* fsub;
    FSubItrator* it = curSubMgr->getFstSubItrator();
    while(it->hasNext()){
        it->next();
        fsub = it->value();
        v.setValue<FirstSubject*>(fsub);
        item = new QListWidgetItem(fsub->getName());
        item->setData(Qt::UserRole,v);
        ui->lwFSub->addItem(item);
    }
}

/**
 * @brief 装载当前选择的一级科目的所有二级科目
 */
void ApcSubject::loadSSub()
{
    ui->lwSSub->clear();
    if(!curFSub)
        return;
    QVariant v;
    QListWidgetItem* item;
    foreach(SecondSubject* ssub, curFSub->getChildSubs()){
        v.setValue<SecondSubject*>(ssub);
        item = new QListWidgetItem(ssub->getName());
        item->setData(Qt::UserRole,v);
        ui->lwSSub->addItem(item);
    }
}

/**
 * @brief 显示当前选中的一级科目的信息或清屏
 * @param fsub
 */
void ApcSubject::viewFSub()
{
    ui->lwSSub->clear();
    if(curFSub){
        ui->fsubID->setText(QString::number(curFSub->getId()));
        int index = ui->FSubCls->findData((int)curFSub->getSubClass());
        ui->FSubCls->setCurrentIndex(index);
        ui->fsubCode->setText(curFSub->getCode());
        ui->fsubName->setText(curFSub->getName());
        ui->fsubRemCode->setText(curFSub->getRemCode());
        ui->fsubWeight->setText(QString::number(curFSub->getWeight()));
        ui->fsubIsEnable->setChecked(curFSub->isEnabled());
        ui->isUseWb->setChecked(curFSub->isUseForeignMoney());
        ui->jdDir_P->setChecked(curFSub->getJdDir());
        //ui->jdDir_N->setChecked(!curFSub->getJdDir());

        //装载二级科目
        QListWidgetItem* item;
        QVariant v;
        foreach(SecondSubject* ssub, curFSub->getChildSubs()){
            v.setValue<SecondSubject*>(ssub);
            item = new QListWidgetItem(ssub->getName());
            item->setData(Qt::UserRole,v);
            ui->lwSSub->addItem(item);
        }
    }
    else{
        ui->fsubID->clear();
        ui->FSubCls->setCurrentIndex(-1);
        ui->fsubName->clear();
        ui->fsubCode->clear();
        ui->fsubRemCode->clear();
        ui->fsubWeight->clear();
        ui->fsubIsEnable->setChecked(false);
        ui->isUseWb->setChecked(false);
        ui->jdDir_P->setChecked(false);
        ui->jdDir_N->setChecked(false);
    }
}

/**
 * @brief 显示当前选中的二级科目的信息或清屏
 * @param ssub
 */
void ApcSubject::viewSSub()
{
    if(curSSub){
        ui->ssubID->setText(QString::number(curSSub->getId()));
        ui->ssubCode->setText(curSSub->getCode());
        ui->ssubCreator->setText(curSSub->getCreator()->getName());
        ui->ssubCrtTime->setText(curSSub->getCreateTime().toString(Qt::ISODate));
        ui->ssubIsEnable->setChecked(curSSub->isEnabled());
        ui->ssubDisTime->setText(curSSub->getDisableTime().toString(Qt::ISODate));
        ui->ssubWeight->setText(QString::number(curSSub->getWeight()));
        ui->ssubName->setText(curSSub->getName());
        ui->ssubLName->setText(curSSub->getLName());
        ui->ssubRemCode->setText(curSSub->getRemCode());
    }
    else{
        ui->ssubID->clear();
        ui->ssubCode->clear();
        ui->ssubCreator->clear();
        ui->ssubCrtTime->clear();
        ui->ssubDisTime->clear();
        ui->ssubIsEnable->setChecked(false);
        ui->ssubWeight->clear();
        ui->ssubName->clear();
        ui->ssubLName->clear();
        ui->ssubRemCode->clear();
    }
}

/**
 * @brief 显示指定名称条目的信息
 */
void ApcSubject::viewNI(SubjectNameItem *ni)
{
    if(ni){
        ui->niID->setText(QString::number(curNI->getId()));
        int index = ui->niCls->findData(ni->getClassId());
        ui->niCls->setCurrentIndex(index);
        ui->niName->setText(ni->getShortName());
        ui->niLName->setText(ni->getLongName());
        ui->niRemCode->setText(ni->getRemCode());
        ui->niCreator->setText(ni->getCreator()?curNI->getCreator()->getName():"");
        ui->niCrtTime->setText(ni->getCreateTime().toString(Qt::ISODate));
    }
    else{
        ui->niID->clear();
        ui->niCls->setCurrentIndex(-1);
        ui->niName->clear();
        ui->niLName->clear();
        ui->niRemCode->clear();
        ui->niCreator->clear();
        ui->niCrtTime->clear();
    }
}

/**
 * @brief 显示名称条目类别的信息
 * @param cls
 */
void ApcSubject::viewNiCls(int cls)
{
    if(cls){
        ui->niClsCode->setText(QString::number(cls));
        ui->niClsName->setText(NiClasses.value(cls).at(0));
        ui->niClsExplain->setText(NiClasses.value(cls).at(1));
    }
    else{
        ui->niClsCode->clear();
        ui->niClsName->clear();
        ui->niClsExplain->clear();
    }
}

/**
 * @brief 控制一级科目编辑控件的可编辑性
 * @param en
 */
void ApcSubject::enFSubWidget(bool en)
{
    ui->lwFSub->setEnabled(!en);
    ui->lwSSub->setEnabled(!en);
    ui->FSubCls->setEnabled(en);
    ui->fsubCode->setReadOnly(!en);
    ui->fsubName->setReadOnly(!en);
    ui->fsubRemCode->setReadOnly(!en);
    ui->fsubWeight->setReadOnly(!en);
    ui->fsubIsEnable->setEnabled(en);
    ui->isUseWb->setEnabled(en);
    ui->jdDir_N->setEnabled(en);
    ui->jdDir_P->setEnabled(en);
    ui->btnFSubEdit->setText(en?tr("取消"):tr("编辑"));
    ui->btnFSubCommit->setEnabled(en);
}

/**
 * @brief 控制二级科目编辑控件的可编辑性
 * @param en
 */
void ApcSubject::enSSubWidget(bool en)
{
    ui->lwSSub->setEnabled(!en);
    ui->ssubCode->setReadOnly(!en);
    ui->ssubIsEnable->setEnabled(en);
    ui->ssubWeight->setReadOnly(!en);
    ui->btnSSubCommit->setEnabled(en);
    ui->btnSSubEdit->setText(en?tr("取消"):tr("编辑"));
    ui->btnSSubCommit->setEnabled(en);
}

/**
 * @brief 控制名称条目编辑控件的可编辑性
 * @param en
 */
void ApcSubject::enNiWidget(bool en)
{
    ui->lwNI->setEnabled(!en);
    ui->btnNiEdit->setText(en?tr("取消"):tr("编辑"));
    ui->btnNiCommit->setEnabled(en);
    ui->niCls->setEnabled(en);
    ui->niName->setReadOnly(!en);
    ui->niLName->setReadOnly(!en);
    ui->niRemCode->setReadOnly(!en);
    ui->btnNewNI->setEnabled(!en);
    ui->btnDelNI->setEnabled(!en);
    //ui->btnMergeSSub->setEnabled(!en);
}

/**
 * @brief 控制名称条目类别编辑控件的可编辑性
 * @param en
 */
void ApcSubject::enNiClsWidget(bool en)
{
    ui->lwNiCls->setEnabled(!en);
    ui->btnNiClsEdit->setText(en?tr("取消"):tr("编辑"));
    ui->btnNiClsCommit->setEnabled(en);
    //ui->niClsCode->setReadOnly(false);
    ui->niClsName->setReadOnly(!en);
    ui->niClsExplain->setReadOnly(!en);
    ui->btnNewNiCls->setEnabled(!en);
    ui->btnDelNiCls->setEnabled(!en);
}

/**
 * @brief 装载选择类别的名称条目到列表
 */
void ApcSubject::loadNameItems()
{
    ui->lwNI->clear();
    int curCls = ui->niClsView->itemData(ui->niClsView->currentIndex()).toInt();
    QVariant v; QListWidgetItem* item;
    foreach(SubjectNameItem* ni, SubjectManager::getAllNameItems()){
        if((curCls != 0) && (curCls != ni->getClassId()))
            continue;
        item = new QListWidgetItem(ni->getShortName());
        v.setValue<SubjectNameItem*>(ni);
        item->setData(Qt::UserRole,v);
        ui->lwNI->addItem(item);
    }
}

/**
 * @brief 启动或提交对名称条目的编辑动作
 */
void ApcSubject::on_btnNiEdit_clicked()
{
    //启动编辑
    if(editAction == APCEA_NONE){
        editAction = APCEA_EDIT_NI;
        stack_ints.clear();
        stack_strs.clear();
        stack_ints.push(curNI->getClassId());
        stack_strs.push(curNI->getShortName());
        stack_strs.push(curNI->getLongName());
        stack_strs.push(curNI->getRemCode());
        enNiWidget(true);
        ui->niName->setFocus();
    }//取消编辑
    else{
        if(editAction == APCEA_EDIT_NI){
            int index = ui->niCls->findData(stack_ints.pop());
            ui->niCls->setCurrentIndex(index);
            ui->niRemCode->setText(stack_strs.pop());
            ui->niLName->setText(stack_strs.pop());
            ui->niName->setText(stack_strs.pop());
        }
        else if(editAction == APCEA_NEW_NI){
            viewNI(curNI);
        }
        enNiWidget(false);
        editAction = APCEA_NONE;
    }
}

/**
 * @brief 保存新建的或被修改的名称条目
 */
void ApcSubject::on_btnNiCommit_clicked()
{
//    if(!curNI)
//        return;
    int cls = ui->niCls->itemData(ui->niCls->currentIndex()).toInt();
    QString name = ui->niName->text();
    QString lname = ui->niLName->text();
    QString remCode = ui->niRemCode->text();
    if(editAction == APCEA_EDIT_NI){
        curNI->setClassId(cls);
        curNI->setShortName(name);
        curNI->setLongName(lname);
        curNI->setRemCode(remCode);
        ui->lwNI->currentItem()->setText(curNI->getShortName());
    }
    else if(editAction == APCEA_NEW_NI){
        curNI = SubjectManager::addNameItem(name,lname,remCode,cls,QDateTime::currentDateTime(),curUser);
        QVariant v;
        v.setValue<SubjectNameItem*>(curNI);
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setData(Qt::UserRole,v);
        ui->lwNI->addItem(item);
        ui->lwNI->setCurrentItem(item);
    }
    if(!account->getDbUtil()->saveNameItem(curNI))
        QMessageBox::critical(this,tr("出错信息"),tr("在将名称条目保存到账户数据库中时出错！"));
    editAction = APCEA_NONE;
    enNiWidget(false);
}

/**
 * @brief 创建新的名称条目
 */
void ApcSubject::on_btnNewNI_clicked()
{
    if(!notCommitWarning())
        return;
    editAction = APCEA_NEW_NI;
    viewNI(0);
    ui->niCrtTime->setText(QDateTime::currentDateTime().toString(Qt::ISODate));
    ui->niCreator->setText(curUser->getName());
    enNiWidget(true);
    ui->niName->setFocus();
}

/**
 * @brief 移除选中的名称条目
 */
void ApcSubject::on_btnDelNI_clicked()
{
    if(!notCommitWarning())
        return;
    if(!curNI)
        return;
    if(account->getDbUtil()->nameItemIsUsed(curNI)){
        QMessageBox::warning(this,tr("警告信息"),tr("该名称条目已被某些二级科目采用，不能删除！"));
        return;
    }
    QListWidgetItem* item = ui->lwNI->takeItem(ui->lwNI->currentRow());
    delete item;
    SubjectManager::removeNameItem(curNI);
}

/**
 * @brief 创建新的名称条目类别
 */
void ApcSubject::on_btnNewNiCls_clicked()
{
    if(!notCommitWarning())
        return;
    editAction = APCEA_NEW_NICLS;
    viewNiCls(0);
    ui->niClsCode->setText(QString::number(SubjectManager::getNotUsedNiClsCode()));
    enNiClsWidget(true);
    ui->niClsName->setFocus();
}

/**
 * @brief 编辑选中的名称条目类别
 */
void ApcSubject::on_btnNiClsEdit_clicked()
{
    //启动编辑
    if(editAction == APCEA_NONE){
        editAction = APCEA_EDIT_NICLS;
        stack_strs.clear();
        viewNiCls(curNiCls);
        stack_strs.push(ui->niClsName->text());
        stack_strs.push(ui->niClsExplain->text());
        enNiClsWidget(true);
        ui->niClsName->setFocus();
    }
    else{//取消编辑
        if(editAction == APCEA_EDIT_NICLS){
            ui->niClsExplain->setText(stack_strs.pop());
            ui->niClsName->setText(stack_strs.pop());
        }
        else if(editAction == APCEA_NEW_NICLS)
            viewNiCls(curNiCls);
        editAction = APCEA_NONE;
        enNiClsWidget(false);
    }

}

/**
 * @brief ApcSubject::提交新建的或已修改的名称条目类别
 */
void ApcSubject::on_btnNiClsCommit_clicked()
{
    if(editAction == APCEA_NEW_NICLS){
        int code = ui->niClsCode->text().toInt();
        QString name = ui->niClsName->text();
        SubjectManager::addNiClass(code,name,ui->niClsExplain->text());
        NiClasses[code] = QStringList();
        NiClasses[code].append(name);
        NiClasses[code].append(ui->niClsExplain->text());
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setData(Qt::UserRole,code);
        ui->lwNiCls->addItem(item);
        ui->lwNiCls->setCurrentItem(item);
    }
    else{
        SubjectManager::modifyNiClass(curNiCls,ui->niClsName->text(),ui->niClsExplain->text());
        NiClasses[curNiCls][0] = ui->niClsName->text();
        NiClasses[curNiCls][1] = ui->niClsExplain->text();
        ui->lwNiCls->currentItem()->setText(ui->niClsName->text());
    }
    if(!account->getDbUtil()->saveNameItemClass(curNiCls,NiClasses.value(curNiCls).first(),NiClasses.value(curNiCls).last()))
        QMessageBox::critical(this, tr("出错信息"),tr("在将名称条目类别保存到账户数据库中时发生错误！"));
    editAction = APCEA_NONE;
    enNiClsWidget(false);
}

/**
 * @brief 删除名称条目类别
 */
void ApcSubject::on_btnDelNiCls_clicked()
{
    if(SubjectManager::isUsedNiCls(curNiCls)){
        QMessageBox::warning(this, tr("警告信息"),tr("该名称条目类别已被使用，不能删除！"));
        return ;
    }
    QListWidgetItem* item = ui->lwNiCls->takeItem(ui->lwNiCls->currentRow());
    SubjectManager::removeNiCls(curNiCls);
    delete item;
}

void ApcSubject::on_niClsBox_toggled(bool en)
{
    ui->niClsCode->setEnabled(en);
    ui->niClsName->setEnabled(en);
    ui->niClsExplain->setEnabled(en);
    ui->btnNewNiCls->setEnabled(en && editAction==APCEA_NONE);
    ui->btnNiClsEdit->setEnabled(en && curNiCls && editAction==APCEA_NONE);
    ui->btnNiClsCommit->setEnabled(en && (editAction==APCEA_EDIT_NICLS || editAction==APCEA_NEW_NICLS));
    ui->btnDelNiCls->setEnabled(en && curNiCls);
}

/**
 * @brief 启动或取消对当前选中的一级的编辑
 */
void ApcSubject::on_btnFSubEdit_clicked()
{    
    if(editAction == APCEA_NONE){ //启动编辑
        stack_ints.push(curFSub->getSubClass());
        stack_ints.push(curFSub->isEnabled()?1:0);
        stack_ints.push(curFSub->isUseForeignMoney()?1:0);
        stack_ints.push(curFSub->getJdDir()?1:0);
        stack_ints.push(curFSub->getWeight());
        stack_strs.push(curFSub->getCode());
        stack_strs.push(curFSub->getName());
        stack_strs.push(curFSub->getRemCode());
        editAction = APCEA_EDIT_FSUB;
        enFSubWidget(true);
        ui->fsubCode->setFocus();
    }
    else if(editAction == APCEA_EDIT_FSUB){  //取消编辑
        ui->fsubRemCode->setText(stack_strs.pop());
        ui->fsubName->setText(stack_strs.pop());
        ui->fsubCode->setText(stack_strs.pop());
        ui->fsubWeight->setText(QString::number(stack_ints.pop()));
        ui->jdDir_P->setChecked(stack_ints.pop());
        ui->jdDir_N->setChecked(!ui->jdDir_P->isChecked());
        ui->isUseWb->setChecked(stack_ints.pop());
        ui->fsubIsEnable->setChecked(stack_ints.pop());
        int index = ui->FSubCls->findData(stack_ints.pop());
        ui->FSubCls->setCurrentIndex(index);
        editAction = APCEA_NONE;
        enFSubWidget(false);
    }
}

/**
 * @brief 提交对一级科目的编辑结果
 */
void ApcSubject::on_btnFSubCommit_clicked()
{
    if(editAction != APCEA_EDIT_FSUB)
        return;
    curFSub->setSubClass((SubjectClass)ui->FSubCls->itemData(ui->FSubCls->currentIndex()).toInt());
    curFSub->setName(ui->fsubName->text());
    curFSub->setRemCode(ui->fsubRemCode->text());
    curFSub->setCode(ui->fsubCode->text());
    curFSub->setEnabled(ui->fsubIsEnable->isChecked());
    curFSub->setIsUseForeignMoney(ui->isUseWb->isChecked());
    curFSub->setJdDir(ui->jdDir_P->isChecked());
    curFSub->setWeight(ui->fsubWeight->text().toInt());
    if(stack_strs.at(1) != curFSub->getName())
        ui->lwFSub->currentItem()->setText(stack_strs.at(1));
    stack_ints.clear();
    stack_strs.clear();
    if(!account->getDbUtil()->savefstSubject(curFSub))
        QMessageBox::critical(this,tr("出错信息"),tr("在将一级科目保存当账户数据库中时发生错误！"));
    editAction = APCEA_NONE;
    enFSubWidget(false);
}

/**
 * 启动或取消对当前选中的二级的编辑
 */
void ApcSubject::on_btnSSubEdit_clicked()
{
    if(editAction == APCEA_NONE){
        stack_strs.push(curSSub->getCode());
        stack_ints.push(curSSub->getWeight());
        stack_ints.push(curSSub->isEnabled()?1:0);
        editAction = APCEA_EDIT_SSUB;
        enSSubWidget(true);
        ui->ssubCode->setFocus();
    }
    else if(editAction == APCEA_EDIT_SSUB){
        ui->ssubCode->setText(stack_strs.pop());
        ui->ssubWeight->setText(QString::number(stack_ints.pop()));
        ui->ssubIsEnable->setChecked((stack_ints.pop()==1)?true:false);
        editAction = APCEA_NONE;
        enSSubWidget(false);
    }
}

/**
 * @brief 提交对二级科目的编辑结果
 */
void ApcSubject::on_btnSSubCommit_clicked()
{
    if(editAction != APCEA_EDIT_SSUB)
        return;
    curSSub->setCode(ui->ssubCode->text());
    curSSub->setEnabled(ui->ssubIsEnable->isChecked());
    curSSub->setWeight(ui->ssubWeight->text().toInt());
    if(!account->getDbUtil()->saveSndSubject(curSSub))
        QMessageBox::critical(this,tr("出错信息"),tr("在将二级科目保存到账户数据库中时出错！"));
    editAction = APCEA_NONE;
    enSSubWidget(false);
}

/**
 * @brief 删除选中的二级科目
 */
void ApcSubject::on_btnSSubDel_clicked()
{
    if(curSubMgr->isUsedSSub(curSSub)){
        QMessageBox::warning(this,tr("警告信息"),tr("二级科目“%1”已在账户中被采用，不能删除！").arg(curSSub->getName()));
        return;
    }
    curFSub->removeChildSub(curSSub);
    delete ui->lwSSub->takeItem(ui->lwSSub->currentRow());
}

/**
 * @brief 科目系统衔接配置
 * @param sCode 源科目系统
 * @param dCode 目的科目系统
 */
void ApcSubject::subJoinConfig(int sCode, int dCode)
{
    int i = 0;
}

bool ApcSubject::notCommitWarning()
{
    QString info;
    switch(editAction){
    case APCEA_EDIT_NI:
    case APCEA_NEW_NI:
        info = tr("未提交名称条目的编辑结果！");
        break;
    case APCEA_EDIT_NICLS:
    case APCEA_NEW_NICLS:
        info = tr("未提交名称条目类别的编辑结果！");
        break;    
    case APCEA_EDIT_FSUB:
        info = tr("未提交一级科目的编辑结果！");
        break;
    }
    if(!info.isEmpty()){
        QMessageBox::warning(this,tr("警告信息"),info);
        return false;
    }
    return true;
}

//bool ApcSubject::testCommited()
//{
//    if(!stack_strs.isEmpty() || !stack_ints.isEmpty()){
//        QMessageBox::warning(this,tr("警告信息"),tr("有未提交的编辑结果！"));
//        return false;
//    }
//    return true;
//}

//////////////////////////SubSysJoinCfgForm////////////////////////////////////////
SubSysJoinCfgForm::SubSysJoinCfgForm(int src, int des, Account* account, QWidget *parent)
    :QDialog(parent),ui(new Ui::SubSysJoinCfgForm),account(account)
{
    ui->setupUi(this);
    sSmg = account->getSubjectManager(src);
    dSmg = account->getSubjectManager(des);
    init();
}

SubSysJoinCfgForm::~SubSysJoinCfgForm()
{
    delete ui;
}

/**
 * @brief 保存对配置的更改
 * @return
 */
bool SubSysJoinCfgForm::save()
{
    //如果用户已经完成配置，则请求用户确定是否终结配置，并进行二级科目的对应克隆
    if(!isCompleted && determineAllComplete()){
        QDialog* dlg = new QDialog;
        QLabel* title = new QLabel(tr("如果确定所有科目都已配置完成，则单击终结配置按钮！"),dlg);
        QCheckBox* chk1 = new QCheckBox(tr("克隆二级科目"),dlg);
        chk1->setEnabled(false);
        chk1->setChecked(true);
        //QCheckBox* chk2 = new QCheckBox(tr("克隆余额"),dlg);
        //chk2->setEnabled(false);
        //chk2->setChecked(true);
        QVBoxLayout* lm = new QVBoxLayout;
        lm->addWidget(title);
        lm->addWidget(chk1);
        //lm->addWidget(chk2);
        QPushButton* btnOk = new QPushButton(tr("终结配置"),dlg);
        QPushButton* btnCancel = new QPushButton(tr("稍后配置"),dlg);
        connect(btnOk,SIGNAL(clicked()),dlg,SLOT(accept()));
        connect(btnCancel,SIGNAL(clicked()),dlg,SLOT(reject()));
        QHBoxLayout* lb = new QHBoxLayout;
        lb->addStretch();
        lb->addWidget(btnOk);
        lb->addWidget(btnCancel);
        lm->addLayout(lb);
        dlg->setLayout(lm);
        if(dlg->exec() == QDialog::Accepted){
            cloneSndSubject();
            account->setCompleteSubSysCfg(sSmg->getCode(),dSmg->getCode(),true,true);
        }
        delete dlg;
    }

    QList<SubSysJoinItem*> editedItems;
    for(int i = 0; i < editTags.count(); ++i){
        if(!editTags.at(i))
            continue;
        editedItems<<ssjs.at(i);
    }
    if(!account->setSubSysJoinCfgInfo(sSmg->getCode(),dSmg->getCode(),editedItems))
        QMessageBox::critical(this,tr("出错信息"),tr("在保存科目系统衔接配置信息时出错！"));
}

/**
 * @brief 建立或取消映射
 */
void SubSysJoinCfgForm::mapBtnClicked()
{
    //首先定位是哪一行的按钮被单击了
    for(int i = 0; i < ui->tview->rowCount(); ++i){
        QPushButton* btn = qobject_cast<QPushButton*>(ui->tview->cellWidget(i,3));
        if(btn == qobject_cast<QPushButton*>(sender())){
            editTags[i] = true;
            ssjs.at(i)->isMap = btn->isChecked();
            if(btn->isChecked())
                btn->setText("==>");
            else
                btn->setText(tr("映射到"));
            //determineAllComplete();
            return;
        }
    }
}

/**
 * @brief 映射的目标科目的选择改变了
 * @param item
 */
void SubSysJoinCfgForm::destinationSubChanged(QTableWidgetItem *item)
{
    if(item->column() != 5)
        return;
    SubSysJoinItem* si = ssjs.at(item->row());
    if(!si->isMap)
        ui->tview->cellWidget(item->row(),3)->setEnabled(true);
    FirstSubject* fsub = item->data(Qt::EditRole).value<FirstSubject*>();
    ui->tview->item(item->row(),4)->setText(fsub->getCode());
    si->dFSub = fsub;
    editTags[item->row()] = true;
}


void SubSysJoinCfgForm::init()
{
    isCompleted = false;
    isCloned = false;
    QHash<int,QString> names;
    foreach(SubSysNameItem* item, account->getSupportSubSys())
        names[item->code] = item->name;
    ui->sSubSys->setText(names.value(sSmg->getCode()));
    ui->dSubSys->setText(names.value(dSmg->getCode()));

    if(!account->isCompleteSubSysCfg(sSmg->getCode(),dSmg->getCode(),isCompleted,isCloned)){
        QMessageBox::critical(this,tr("出错信息"),tr("在读取配置变量时发生错误"));
        ui->btnOk->setEnabled(false);
        return;
    }
    //ui->chkComplete->setEnabled(!isCompleted);
    //ui->chkComplete->setChecked(isCompleted);
    ui->tview->setColumnHidden(0,true);
    if(!account->getSubSysJoinCfgInfo(sSmg->getCode(),dSmg->getCode(),ssjs)){
        QMessageBox::critical(this,tr("出错信息"),tr("在读取科目系统衔接配置信息时出错！"));
        return;
    }
    //判断用户是否是第一次执行配置，如果是，则设置预配置科目
    bool initCfg = true;
    foreach(SubSysJoinItem* item,ssjs){
        if(item->dFSub){
            initCfg = false;
            break;
        }
    }
    for(int i = 0; i < ssjs.count(); ++i)
        editTags<<false;
    if(initCfg)
        preConfig();
    ui->tview->setRowCount(ssjs.count());
    SubSysJoinItem* item;
    QTableWidgetItem* ti;
    QPushButton* btn;
    bool allCfgComplete = true;  //是否所有必要的科目都配置完成
    for(int i = 0; i < ssjs.count(); ++i){
        item = ssjs.at(i);
        ti = new QTableWidgetItem(item->sFSub->getCode());
        ui->tview->setItem(i,1,ti);
        ti = new QTableWidgetItem(item->sFSub->getName());
        ui->tview->setItem(i,2,ti);
        btn = new QPushButton;
        btn->setCheckable(true);
        connect(btn,SIGNAL(clicked()),this,SLOT(mapBtnClicked()));
        ui->tview->setCellWidget(i,3,btn);
        if(isCompleted || !item->dFSub){
            allCfgComplete = false;
            btn->setEnabled(false);
        }
        if(item->isMap){
            btn->setChecked(true);
            btn->setText("==>");
        }
        else{
            btn->setChecked(false);
            btn->setText(tr("映射到"));
        }
        ti = new QTableWidgetItem(item->dFSub?item->dFSub->getCode():"");
        ui->tview->setItem(i,4,ti);
        ti = new BAFstSubItem2(item->dFSub);
        ui->tview->setItem(i,5,ti);
    }
    //if(!isCompleted)
    //    ui->chkComplete->setEnabled(!allCfgComplete);
    SubSysJoinCfgItemDelegate* delegate = new SubSysJoinCfgItemDelegate(dSmg);
    delegate->setReadOnly(isCompleted);
    ui->tview->setItemDelegate(delegate);
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(destinationSubChanged(QTableWidgetItem*)));

}

/**
 * @brief 扫描配置项列表中的所有条目，确定是否所有科目都完成了配置
 */
bool SubSysJoinCfgForm::determineAllComplete()
{
    foreach(SubSysJoinItem* item, ssjs){
        if(!item->isMap)
            return false;
    }
    return true;
}

/**
 * @brief 将配置项中源一级科目下的二级科目克隆到目的一级科目下
 */
void SubSysJoinCfgForm::cloneSndSubject()
{
    QList<SecondSubject*> ssubs;
    foreach(SubSysJoinItem* item, ssjs){
        for(int i = 0; i < item->sFSub->getChildSubs().count(); ++i){
            SecondSubject* sub = new SecondSubject(*(item->sFSub->getChildSub(i)));
            sub->setCreateTime(QDateTime::currentDateTime());
            sub->setCreator(curUser);
            item->dFSub->addChildSub(sub);
            ssubs<<sub;
        }
    }
    if(!account->getDbUtil()->saveSndSubjects(ssubs))
        QMessageBox::critical(this,tr("出错信息"),tr("在将克隆的二级科目保存到账户数据库中时发生错误！"));
    //建立子目的映射列表
    int j = 0;
    foreach(SubSysJoinItem* item, ssjs){
        for(int i = 0; i < item->sFSub->getChildSubs().count(); ++i){
            item->ssubMaps<<item->sFSub->getChildSub(i)->getId()
                          <<item->dFSub->getChildSub(i)->getId();
        }
        editTags[j++] = true;
    }

}

/**
 * @brief 为了加快用户的配置速度，预先配置好一些已知科目系统迁徙的科目衔接
 *  当前的实现仅支持在从科目系统1到科目系统2的迁徙时，做出一些预先配置
 */
void SubSysJoinCfgForm::preConfig()
{
    if((sSmg->getCode()==1) && (dSmg->getCode()==2)){
        QStringList sCodes,dCodes;
        sCodes<<"1001"<<"1002"<<"1131"<<"1133"<<"1151"<<"1301"<<"1501"<<"1502"<<"1801"<<"2121"<<"2131"<<"2151"<<"2171"<<"2176"<<"2181"<<"3101"<<"3131"<<"3141"<<"5101"<<"5301"<<"5401"<<"5402"<<"5501"<<"5502"<<"5503"<<"5601"<<"5701";
        dCodes<<"1001"<<"1002"<<"1122"<<"1221"<<"1123"<<"1123"<<"1601"<<"1602"<<"1701"<<"2202"<<"2203"<<"2211"<<"2221"<<"2241"<<"2241"<<"4001"<<"4103"<<"4104"<<"6001"<<"6301"<<"6401"<<"6403"<<"6601"<<"6602"<<"6603"<<"6711"<<"6801";
        QHash<int,FirstSubject*> subMaps;
        for(int i = 0; i < sCodes.count(); ++i)
            subMaps[sSmg->getFstSubject(sCodes.at(i))->getId()] = dSmg->getFstSubject(dCodes.at(i));

        int i = 0;
        foreach(SubSysJoinItem* item,ssjs){
            item->dFSub = subMaps.value(item->sFSub->getId());
            item->isMap = true;
            editTags[i++] = true;
        }
    }

}



///////////////////////////////ApcReport/////////////////////////////////////////
ApcReport::ApcReport(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcReport)
{
    ui->setupUi(this);
}

ApcReport::~ApcReport()
{
    delete ui;
}

//////////////////////////ApcLog/////////////////////////////////////////
ApcLog::ApcLog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApcLog)
{
    ui->setupUi(this);
}

ApcLog::~ApcLog()
{
    delete ui;
}

///////////////////////////////AccountPropertyConfig///////////////////////////////////////
AccountPropertyConfig::AccountPropertyConfig(Account* account, QWidget *parent) : QDialog(parent)
{
    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMaximumWidth(86);
    contentsWidget->setSpacing(12);

    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(new ApcBase(account,this));
    pagesWidget->addWidget(new ApcSuite(account,this));
    pagesWidget->addWidget(new ApcBank(this));
    pagesWidget->addWidget(new ApcSubject(account,this));
    pagesWidget->addWidget(new ApcReport(this));
    pagesWidget->addWidget(new ApcLog(this));

    QPushButton *closeButton = new QPushButton(tr("关闭"));

    createIcons();
    contentsWidget->setCurrentRow(0);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout,1);
    //mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("账户属性配置"));
}

AccountPropertyConfig::~AccountPropertyConfig()
{

}

void AccountPropertyConfig::pageChanged(int index)
{
    pagesWidget->setCurrentIndex(index);
    if(index == APC_SUBJECT){
        ApcSubject* w = static_cast<ApcSubject*>(pagesWidget->currentWidget());
        w->init();
    }
    else if(index == APC_SUITE){
        ApcSuite* w = static_cast<ApcSuite*>(pagesWidget->currentWidget());
        w->init();
    }

}

void AccountPropertyConfig::createIcons()
{
    QListWidgetItem *item;
    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/baseInfo.png"));
    item->setText(tr("基本"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/suiteInfo.png"));
    item->setText(tr("帐套"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/bankInfo.png"));
    item->setText(tr("开户行"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/subjectInfo.png"));
    item->setText(tr("科目"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/reportInfo.png"));
    item->setText(tr("报表"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item = new QListWidgetItem(contentsWidget);
    item->setIcon(QIcon(":/images/accProperty/logInfo.png"));
    item->setText(tr("日志"));
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(contentsWidget,SIGNAL(currentRowChanged(int)),
         this, SLOT(pageChanged(int)));
}

























