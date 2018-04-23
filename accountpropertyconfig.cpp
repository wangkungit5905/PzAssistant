#include "accountpropertyconfig.h"
#include "account.h"
#include "config.h"
#include "subject.h"
#include "dbutil.h"
#include "widgets.h"
#include "delegates.h"
#include "widgets/bawidgets.h"
#include "version.h"
#include "newsndsubdialog.h"
#include "PzSet.h"
#include "myhelper.h"
#include "optionform.h"


#include <QListWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QBuffer>
#include <QTextStream>
#include <QTreeWidget>
#include <QMenu>


ApcBase::ApcBase(Account *account, QWidget *parent) :
    QWidget(parent),ui(new Ui::ApcBase),account(account)
{
    ui->setupUi(this);
    iniTag = false;
    changed = false;
}

ApcBase::~ApcBase()
{
    delete ui;
}

void ApcBase::init()
{
    if(iniTag)
        return;
    wbs = account->getWaiMt();
    ui->edtName->setText(account->getSName());
    ui->edtLName->setText(account->getLName());
    ui->edtCode->setText(account->getCode());
    ui->edtFName ->setText(account->getFileName());
    ui->startDate->setDate(account->getStartDate());
    ui->endDate->setDate(account->getEndDate());
    ui->edtMMt->setText(account->getMasterMt()->name());
    ui->chkJxTaxMgr->setChecked(account->isJxTaxManaged());
    foreach(Money* mt, wbs){
        QListWidgetItem* item = new QListWidgetItem(mt->name());
        QVariant v; v.setValue<Money*>(mt);
        item->setData(Qt::UserRole,v);
        ui->lstWMt->addItem(item);
    }
    foreach(User* u, allUsers.values()){
        if(u->isAdmin() || u->isSuperUser())
            continue;
        if(u->canAccessAccount(account)){
            QListWidgetItem* item = new QListWidgetItem(u->getName());
            item->setData(Qt::UserRole,u->getUserId());
            ui->lstUsers->addItem(item);
        }
    }
    if(!account->isReadOnly()){
        connect(ui->lstUsers,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this,SLOT(currentUserChanged(QListWidgetItem*,QListWidgetItem*)));
        connect(ui->edtName,SIGNAL(textEdited(QString)),this,SLOT(textEdited()));
        connect(ui->edtLName,SIGNAL(textEdited(QString)),this,SLOT(textEdited()));
        if(curUser->isSuperUser() || curUser->isAdmin()){
            ui->addUser->setEnabled(true);
            if(ui->lstUsers->currentRow()>=0)
                ui->removeUser->setEnabled(true);
            else
                ui->removeUser->setEnabled(false);
        }
        else{
            ui->addUser->setEnabled(false);
            ui->removeUser->setEnabled(false);
        }
    }
    else{
        ui->addWb->setEnabled(false);
        ui->delWb->setEnabled(false);
        ui->edtName->setReadOnly(true);
        ui->edtLName->setReadOnly(true);
        ui->addUser->setEnabled(false);
        ui->removeUser->setEnabled(false);
    }
    iniTag = true;
}

/**
 * @brief ApcBase::windowShallClosed
 *  关闭前的保存操作
 */
void ApcBase::windowShallClosed()
{
    if(account->isReadOnly())
        return;
    bool tag = false;
    if(ui->edtName->text() != account->getSName()){
        tag = true;
        account->setSName(ui->edtName->text());
    }
    if(ui->edtLName->text() != account->getLName()){
        tag = true;
        account->setLName(ui->edtLName->text());
    }
    if(ui->chkJxTaxMgr->isChecked() ^ account->isJxTaxManaged()){
        tag = true;
        account->setJxTaxManaged(ui->chkJxTaxMgr->isChecked());
    }
    //如果外币数目不同或相同但币种不同，则认为外币发生了改变
//    if(wbs.count() != account->getWaiMt().count()){
//        account->setWaiMts(wbs);
//        tag = true;
//    }
//    else{
//        qSort(wbs.begin(),wbs.end());
//        QList<Money*> mts;
//        mts = account->getWaiMt();
//        qSort(mts.begin(),mts.end());
//        foreach(Money* mt, mts){
//            if(!wbs.contains(mt)){
//                tag = true;
//                break;
//            }
//        }
//    }
    if(tag){
        account->saveAccountInfo();
        if(account->isJxTaxManaged())
            account->getDbUtil()->crtJxTaxTable();
    }
}

void ApcBase::textEdited()
{
    changed = true;
}

void ApcBase::currentUserChanged(QListWidgetItem * current, QListWidgetItem * previous)
{
    if(!curUser->isSuperUser() && !curUser->isAdmin())
        return;
    if(current && !previous)
        ui->removeUser->setEnabled(true);
    else if(!current && previous)
        ui->removeUser->setEnabled(false);
}

void ApcBase::on_addWb_clicked()
{
    QList<Money*> mts;
    QHash<int,Money*> moneys;
    if(!AppConfig::getInstance()->getSupportMoneyType(moneys))
        return;
    mts = moneys.values();
    mts.removeOne(account->getMasterMt());
    Money* mt;
    foreach(Money* mt, wbs){
        mts.removeOne(mt);
    }

    if(mts.isEmpty()){
        myHelper::ShowMessageBoxInfo(tr("应用支持的所有货币类型已全部被账户利用，已没有新的可用的外币类型！"));
        return;
    }
    QDialog dlg;
    QListWidget lstMt(&dlg);
    QVariant v; QListWidgetItem* item;

    for(int i = 0; i < mts.count(); ++i){
        mt = mts.at(i);
        v.setValue<Money*>(mt);
        item = new QListWidgetItem(mt->name(),&lstMt);
        item->setData(Qt::UserRole,v);
    }
    QVBoxLayout lm;
    lm.addWidget(&lstMt);
    QPushButton btnOk(QIcon(":/images/btn_ok.png"),tr("确定")),btnCancel(QIcon(":/images/btn_close.png"),tr("取消"));
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb;
    lb.addWidget(&btnOk);
    lb.addWidget(&btnCancel);
    lm.addLayout(&lb);
    dlg.setLayout(&lm);
    if(dlg.exec() == QDialog::Accepted){
        if(lstMt.currentRow() > -1){
            mt = lstMt.currentItem()->data(Qt::UserRole).value<Money*>();
            QListWidgetItem* item = new QListWidgetItem(mt->name());
            v.setValue<Money*>(mt);
            item->setData(Qt::UserRole,v);
            ui->lstWMt->addItem(item);
            wbs.append(mt);
            account->addWaiMt(mt);
        }
    }
}


void ApcBase::on_delWb_clicked()
{
    if(!ui->lstWMt->currentItem())
        return;
    Money* mt = ui->lstWMt->currentItem()->data(Qt::UserRole).value<Money*>();
    bool used;
    account->getDbUtil()->moneyIsUsed(mt,used);
    if(used){
        myHelper::ShowMessageBoxWarning(tr("该货币已被账户使用了，不能删除！"));
        return;
    }
    ui->lstWMt->takeItem(ui->lstWMt->currentRow());
    wbs.removeOne(mt);
    account->delWaiMt(mt);
}

/**
 * @brief 添加专属用户
 */
void ApcBase::on_addUser_clicked()
{
    QList<User*> users = allUsers.values();
    foreach(User* u, users){
        if(u->isSuperUser() || u->isAdmin())
            users.removeOne(u);
    }
    for(int i = 0; i < ui->lstUsers->count(); ++i){
        User* u = allUsers.value(ui->lstUsers->item(i)->data(Qt::UserRole).toInt());
        users.removeOne(u);
    }
    if(users.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("没有可作为专属用户的用户！"));
        return;
    }
    QDialog dlg(this);
    QListWidget lw(&dlg);
    foreach(User* u, users){
        QListWidgetItem* item = new QListWidgetItem(u->getName());
        item->setData(Qt::UserRole,u->getUserId());
        lw.addItem(item);
    }
    lw.setCurrentRow(0);
    QPushButton btnOk(QIcon(":/images/btn_ok.png"),tr("确定"),&dlg),btnCancel(QIcon(":/images/btn_close.png"),tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb;
    lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&lw);
    lm->addLayout(&lb);
    dlg.setLayout(lm);
    dlg.resize(200,200);
    if(QDialog::Rejected == dlg.exec())
        return;
    User* u = allUsers.value(lw.currentItem()->data(Qt::UserRole).toInt());
    QListWidgetItem* item = new QListWidgetItem(u->getName());
    item->setData(Qt::UserRole,u->getUserId());
    ui->lstUsers->addItem(item);
    u->addExclusiveAccount(account->getCode());
    AppConfig::getInstance()->saveUser(u);
}

/**
 * @brief 移除专属用户
 */
void ApcBase::on_removeUser_clicked()
{
    if(ui->lstUsers->currentRow() == -1)
        return;
    if(QMessageBox::Yes == QMessageBox::warning(this,"",tr("如果移除此用户，则该用户此后将不能访问当前账户。确定要这样做吗？"),
                                    QMessageBox::Yes|QMessageBox::No,QMessageBox::No)){
        User* u = allUsers.value(ui->lstUsers->currentItem()->data(Qt::UserRole).toInt());
        u->removeExclusiveAccount(account->getCode());
        delete ui->lstUsers->takeItem(ui->lstUsers->currentRow());
        AppConfig::getInstance()->saveUser(u);
    }
}


//////////////////////////////ApcSuite////////////////////////////////////////////////
ApcSuite::ApcSuite(Account *account, QWidget *parent) :
    QWidget(parent),ui(new Ui::ApcSuite),account(account)
{
    ui->setupUi(this);
    iniTag = false;
    editAction = EA_NONE;
    ui->btnNew->setEnabled(!account->isReadOnly() && curUser->haveRight(allRights.value(Right::Suite_New)));
    if(!account->isReadOnly()){
        connect(ui->lw,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(suiteDbClicked(QListWidgetItem*)));
    }
    connect(ui->lw,SIGNAL(currentRowChanged(int)),this,SLOT(curSuiteChanged(int)));
}

ApcSuite::~ApcSuite()
{
    delete ui;
}

void ApcSuite::init()
{
    if(iniTag)
        return;
    suites = account->getAllSuiteRecords();
    foreach(SubSysNameItem* sni, account->getSupportSubSys()){
        subSystems[sni->code] = sni;
    }

    QListWidgetItem* item;
    foreach(AccountSuiteRecord* as, suites){
        item = new QListWidgetItem(as->name);
        ui->lw->addItem(item);
    }
    iniTag = true;
    curSuiteChanged(-1);
}

void ApcSuite::windowShallClosed()
{
    if(account->isReadOnly())
        return;
    if(editAction != EA_NONE){
        if(QMessageBox::warning(this,tr("警告信息"),tr("账套设置信息被修改，但未保存！"),QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            on_btnCommit_clicked();
    }
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
        ui->isClosed->setChecked(as->isClosed);
        ui->isUsed->setChecked(as->isUsed);
        ui->lblSubSys->setText(subSystems.value(as->subSys)->name);
        ui->btnEdit->setEnabled(!curAccount->isReadOnly() && curUser->haveRight(allRights.value(Right::Suite_Edit)));
        ui->btnUsed->setEnabled(!as->isUsed);
    }
    else{
        ui->name->clear();
        ui->year->clear();
        ui->smonth->clear();
        ui->emonth->clear();
        ui->rmonth->clear();
        ui->lblSubSys->clear();
        ui->isClosed->setChecked(false);
        ui->isUsed->setChecked(false);
        ui->btnEdit->setEnabled(false);
        ui->btnUsed->setEnabled(false);
        ui->btnUpgrade->setEnabled(false);
    }
}

void ApcSuite::suiteDbClicked(QListWidgetItem *item)
{
    on_btnEdit_clicked();
}

/**
 * @brief 创建新帐套 *
 */
void ApcSuite::on_btnNew_clicked()
{
    AccountSuiteRecord* as = new AccountSuiteRecord;
    as->id = UNID;
    if(suites.isEmpty()){
        as->year = account->getStartDate().year();
        as->subSys = DEFAULT_SUBSYS_CODE;
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
        //stack_i.push(ui->subSys->itemData(ui->subSys->currentIndex()).toInt());
        stack_i.push(ui->isUsed->isChecked()?1:0);
        editAction = EA_EDIT;
        enWidget(true);
    }
    else{ //取消编辑或新建操作
        if(editAction == EA_EDIT){
            ui->name->setText(stack_s.pop());
            ui->isUsed->setChecked((stack_i.pop()==1)?true:false);
            //ui->subSys->setCurrentIndex(ui->subSys->findData(stack_i.pop()));
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
    as->isUsed = true;
    //if(!account->saveSuite(as))
    //    QMessageBox::critical(this,tr("出错信息"),tr("在保存帐套时发生错误！"));
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
    bool update = false;
    AccountSuiteRecord* as = suites.at(ui->lw->currentRow());
    if(as->name != ui->name->text()){
        update = true;
        ui->lw->currentItem()->setText(ui->name->text());
    }
    as->name = ui->name->text();
    as->isUsed = ui->isUsed->isChecked();
    if(editAction == EA_NEW){
        update = true;
        account->addSuite(as);
    }
    if(!account->saveSuite(as))
        myHelper::ShowMessageBoxError(tr("在保存帐套时发生错误！"));
    SubjectManager* sm = account->getSubjectManager(as->subSys);
    sm->setEndDate(QDate(as->year,12,31));
    stack_i.clear();
    stack_s.clear();
    if(update)
        emit suiteUpdated();
    editAction = EA_NONE;
    enWidget(false);
}

/**
 * @brief 升级帐套所采用的科目系统
 */
void ApcSuite::on_btnUpgrade_clicked()
{
    //QMessageBox::warning(this,"",tr("实现要做调整，暂不支持！"));
    if(QMessageBox::No == QMessageBox::information(this,tr("提示信息"),
                                                   tr("确定要升级科目系统吗？\n升级后将无法逆转！"),
                                                   QMessageBox::Yes|QMessageBox::No))
        return;


    AccountSuiteRecord* as = suites.at(ui->lw->currentRow());
    QList<SubSysNameItem*> items = account->getSupportSubSys();
    int index = -1;
    for(int i = 0; i < items.count(); ++i){
        if(items.at(i)->code > as->subSys){
            if(!items.at(i)->isImport || !items.at(i)->isConfiged)
                continue;
            index = i;
            break;
        }
    }
    if(index == -1)
        return;

    QString fname = account->getFileName();
    QString sf = DATABASE_PATH + fname;
    fname.chop(4);
    fname = QString("%1_%2_%3%4").arg(fname).arg("SUP")
            .arg(QDateTime::currentDateTime().toString())
            .arg(VM_ACC_BACKSUFFIX);
    QString df = QString("%1/%2/%3")
            .arg(DATABASE_PATH).arg(VM_ACC_BACKDIR).arg(fname);
    if(!QFile::copy(sf,df)){
        myHelper::ShowMessageBoxError(tr("在升级科目系统前，执行账户文件的备份时出错"));
        return;
    }

    int sc = as->subSys;
    int dc = items.at(index)->code;
    QList<MixedJoinCfg*> cfgs;
    if(!account->getDbUtil()->getMixJoinInfo(sc,dc,cfgs)){
        myHelper::ShowMessageBoxError(tr("在升级科目系统前，获取混合科目对接条目时出错"));
    }
    QHash<int,int> fMaps,sMaps;//主目和子目id对接映射表
    int fid = 0;
    foreach(MixedJoinCfg* item, cfgs){
        if(fid != item->s_fsubId){
            fid = item->s_fsubId;
            fMaps[fid] = item->d_fsubId;
        }
        sMaps[item->s_ssubId] = item->d_ssubId;
    }

    if(!account->getDbUtil()->convertExtraInYear(as->year,fMaps)){
        myHelper::ShowMessageBoxError(tr("在转换帐套（%1年）内的一级科目余额表项时出错").arg(as->year));
        return;
    }
    if(!account->getDbUtil()->convertExtraInYear(as->year,sMaps,false)){
        myHelper::ShowMessageBoxError(tr("在转换帐套（%1年）内的二级科目余额表项时出错").arg(as->year));
        return;
    }
    if(!account->getDbUtil()->convertPzInYear(as->year,fMaps,sMaps)){
        myHelper::ShowMessageBoxError(tr("在转换帐套（%1年）内的会计分录时出错").arg(as->year));
        return;
    }

    as->subSys = dc;
    if(!account->saveSuite(as))
        myHelper::ShowMessageBoxError(tr("在保存升级后的帐套时发生错误"));
    SubjectManager* sm = account->getSubjectManager(dc);
    sm->setStartDate(QDate(as->year,1,1));
    ui->lblSubSys->setText(subSystems.value(dc)->name);
    ui->btnUpgrade->setEnabled(false);

//    if(errors.count() > 2){
//        QFile logFile(LOGS_PATH + "subSysUpgrade.log");
//        if(!logFile.open(QIODevice::WriteOnly)){
//            QMessageBox::critical(this,tr("错误提示"),tr("无法将升级日志写入到日志文件"));
//            return;
//        }
//        QTextStream ds(&logFile);
//        foreach (QString s, errors){
//            ds<<s<<"\n";
//        }
//        ds.flush();
//        QMessageBox::warning(this,tr("警告信息"),tr("在转换当前帐套内的余额项或会计分录时，遇到系统无法处理的条目，具体请查看升级日志文件"));
//    }
}

void ApcSuite::enWidget(bool en)
{
    bool isRO = account->isReadOnly();
    ui->lw->setEnabled(isRO || !en);
    ui->name->setReadOnly(!isRO && en && curUser->haveRight(allRights.value(Right::Suite_Edit)));
    ui->btnCommit->setEnabled(!isRO && en);
    ui->btnEdit->setText(en?tr("取消"):tr("编辑"));
    if(isRO || !en){
        ui->btnUpgrade->setEnabled(false);
        return;
    }
    //决定科目系统升级按钮的启用性
    //判断当前帐套的科目系统是否可以升级
    //首先存在一个新的科目系统，且该科目系统必须已经导入，并正确地配置了新老科目之间的映射
    int row = ui->lw->currentIndex().row();
    AccountSuiteRecord* as = suites.at(row);
    QList<SubSysNameItem*> items = account->getSupportSubSys();
    int idx = -1;
    for(int i = 0; i < items.count(); ++i){
        if(as->subSys == items.at(i)->code){
            idx = i;
            break;
        }
    }
    //如果没有找到，或当前帐套使用的科目系统是最新的，则不能升级
    if(idx == -1 || idx == (items.count()-1)){
        ui->btnUpgrade->setEnabled(false);
    }
    //下一个新的科目系统还没有导入或还没有配置完成，也不能升级
    else if(!items.at(idx+1)->isImport || !items.at(idx+1)->isConfiged){
        ui->btnUpgrade->setEnabled(false);
    }
    //如果当前帐套不是最后一个帐套且其下一个帐套采用的科目系统与当前帐套的科目系统相同，也不能升级
    else if(row < (suites.count()-1) && suites.at(row+1)->subSys == as->subSys)
        ui->btnUpgrade->setEnabled(false);
    else
        ui->btnUpgrade->setEnabled(true);
}


//////////////////////////BankCfgNiCellWidget/////////////////////////////////////////////
BankCfgNiCellWidget::BankCfgNiCellWidget(SubjectNameItem *ni):QTableWidgetItem(),ni(ni)
{
}

QVariant BankCfgNiCellWidget::data(int role) const
{
    if(role == Qt::DisplayRole){
        if(!ni)
            return "";
        else
            return ni->getShortName();
    }
    if(role == Qt::EditRole){
        QVariant v;
        v.setValue(ni);
        return v;
    }
    return QTableWidgetItem::data(role);
}

void BankCfgNiCellWidget::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole){
        ni = value.value<SubjectNameItem*>();
        QTableWidgetItem::setData(Qt::DisplayRole,ni?ni->getShortName():"");
    }
    else
        QTableWidgetItem::setData(role, value);
}


////////////////////////BankCfgItemDelegate/////////////////////////////////////////////
BankCfgItemDelegate::BankCfgItemDelegate(Account *account, QObject *parent)
    : QItemDelegate(parent),account(account)
{
}

QWidget *BankCfgItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(readOnly)
        return NULL;
    int col = index.column();
    int row = index.row();
    if(col == ApcBank::CI_MONEY){
        SubjectNameItem* ni = index.model()->data(index.model()->index(row,ApcBank::CI_NAME),Qt::EditRole).value<SubjectNameItem*>();
        if(ni)
            return 0;
        QComboBox* cmb = new QComboBox(parent);
        QVariant v;
        foreach(Money* mt, account->getAllMoneys().values()){
            v.setValue<Money*>(mt);
            cmb->addItem(mt->name(),v);
        }
        return cmb;
    }
    else if(col == ApcBank::CI_ACCOUNTNUM){
        QLineEdit* edt = new QLineEdit(parent);
        return edt;
    }
}

void BankCfgItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    if(col == ApcBank::CI_MONEY){
        QComboBox* cmb = qobject_cast<QComboBox*>(editor);
        if(cmb){
            Money* mt = index.model()->data(index,Qt::EditRole).value<Money*>();
            QVariant v;
            v.setValue<Money*>(mt);
            int index = cmb->findData(v);
            cmb->setCurrentIndex(index);
        }
    }
    else if(col == ApcBank::CI_ACCOUNTNUM){
        QLineEdit* edt = qobject_cast<QLineEdit*>(editor);
        if(edt)
            edt->setText(index.model()->data(index).toString());
    }
}

void BankCfgItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    int col = index.column();
    if(col == ApcBank::CI_MONEY){
        QComboBox* cmb = qobject_cast<QComboBox*>(editor);
        if(cmb){
            Money* mt = cmb->itemData(cmb->currentIndex()).value<Money*>();
            QVariant v; v.setValue<Money*>(mt);
            model->setData(index,v,Qt::EditRole);
            //model->setData(index,cmb->currentText(),Qt::DisplayRole);
        }
    }
    else if(col == ApcBank::CI_ACCOUNTNUM){
        QLineEdit* edt = qobject_cast<QLineEdit*>(editor);
        if(edt)
            model->setData(index,edt->text(),Qt::DisplayRole);
    }
}

void BankCfgItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}


////////////////////////////ApcBank///////////////////////////////////////////////
ApcBank::ApcBank(Account* account, QWidget *parent) : QWidget(parent), ui(new Ui::ApcBank),account(account)
{
    ui->setupUi(this);
    isEdit = !account->isReadOnly() && curUser->haveRight(allRights.value(Right::Account_Config_SetSensitiveInfo));
    iniTag = false;
    curBank = NULL;
    editAction = EA_NONE;
    delegate = NULL;
    if(!account->isReadOnly() && curUser->haveRight(allRights.value(Right::Account_Config_SetSensitiveInfo))){
        delegate = new BankCfgItemDelegate(account,this);
        ui->tvAccList->setItemDelegate(delegate);
    }
    else{
        ui->newBank->setEnabled(false);        
        ui->tvAccList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    ui->delBank->setEnabled(false);
}

ApcBank::~ApcBank()
{
    delete ui;
}

void ApcBank::init()
{
    if(iniTag)
        return;
    ui->tvAccList->setColumnHidden(CI_ID,true);
    banks = account->getAllBank();
    QListWidgetItem* item;
    foreach(Bank* bank, banks){
        editTags<<false;
        item = new QListWidgetItem(bank->name);
        ui->lstBank->addItem(item);
    }
    if(isEdit){
        connect(ui->lstBank,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(bankDbClicked()));
    }
    connect(ui->lstBank,SIGNAL(currentRowChanged(int)),this,SLOT(curBankChanged(int)));

    if(!banks.isEmpty())
        ui->lstBank->setCurrentRow(0);
    iniTag = true;
    enWidget(false);
}

void ApcBank::windowShallClosed()
{
    if(editAction != EA_NONE){
        if(QMessageBox::warning(this,tr("警告信息"),tr("银行设置信息已被修改，但未保存！"),QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            on_submit_clicked();
    }
}

/**
 * @brief 当前选择的银行改变
 * @param index
 */
void ApcBank::curBankChanged(int index)
{
    if(index < 0 || index >= ui->lstBank->count()){
        curBank = NULL;
        ui->delBank->setEnabled(false);
        return;
    }
    curBank = banks.at(index);
    viewBankAccounts();
    ui->delBank->setEnabled(isEdit);
}

/**
 * @brief 跟踪银行帐号选择的改变
 * @param currentRow
 * @param currentColumn
 * @param previousRow
 * @param previousColumn
 */
void ApcBank::curBankAccountChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if(currentRow != previousRow)
        ui->delAcc->setEnabled((isEdit && editAction != EA_NONE) && (currentRow != -1));
}

/**
 * @brief 为银行账户创建对应的名称条目
 */
void ApcBank::crtNameBtnClicked()
{
    if(editAction == EA_NONE)
        return;
    int row = -1;
    for(int i = 0; i < ui->tvAccList->rowCount(); ++i){
        QPushButton* btn = qobject_cast<QPushButton*>(ui->tvAccList->cellWidget(i,CI_NAME));
        if(btn && btn == qobject_cast<QPushButton*>(sender())){
            row = i;
            break;
        }
    }
    Money* mt = ui->tvAccList->item(row,CI_MONEY)->data(Qt::EditRole).value<Money*>();
    QString suggestName = QString("%1-%2").arg(ui->bankName->text()).arg(mt->name());
    QString suggestLName = QString("%1-%2").arg(ui->bankLName->text()).arg(mt->name());
    bool ok;
    QInputDialog::getText(this,tr("信息获取"),tr("建议的名称"),QLineEdit::Normal,suggestName,&ok);
    if(ok){
        int cls = AppConfig::getInstance()->getSpecNameItemCls(AppConfig::SNIC_BANK);
        SubjectNameItem* ni = new SubjectNameItem(UNID,cls,suggestName,suggestLName,"",
                                                  QDateTime::currentDateTime(),curUser);
        ui->tvAccList->setCellWidget(row,CI_NAME,NULL);
        BankCfgNiCellWidget* item = new BankCfgNiCellWidget(ni);
        ui->tvAccList->setItem(row,CI_NAME,item);
        //QTableWidgetItem* mtItem = new QTableWidgetItem(mt->name());
        //ui->tvAccList->setItem(row,CI_MONEY,mtItem);
    }
}

/**
 * @brief ApcBank::bankDbClicked
 *  双击银行名，启动编辑
 */
void ApcBank::bankDbClicked()
{
    if(editAction == EA_NONE)
        on_editBank_clicked();
}

void ApcBank::on_editBank_clicked()
{
    //开始编辑
    if(editAction == EA_NONE){
        editAction = EA_EDIT;
        enWidget(true);
    }
    else{ //取消编辑
        if(editAction == EA_NEW){
            delete curBank;
            ui->lstBank->setCurrentRow(-1);
            //curBank = NULL;
            //if(!banks.isEmpty())
            //    ui->lstBank->setCurrentRow(0);
            ui->newBank->setEnabled(true);
        }
        viewBankAccounts();
        editAction = EA_NONE;
        enWidget(false);
    }
}

/**
 * @brief ApcBank::on_submit_clicked
 */
void ApcBank::on_submit_clicked()
{
    if(editAction == EA_NONE)
        return;
    if(editAction == EA_EDIT){
        curBank->name = ui->bankName->text();
        curBank->lname = ui->bankLName->text();
        curBank->isMain = ui->chkIsMain->isChecked();
        if(curBank->isMain){
            foreach(Bank* bank, banks){
                if(bank != curBank && bank->isMain){
                    bank->isMain = false;
                    account->saveBank(bank);
                }
            }
        }
        for(int i = 0; i < ui->tvAccList->rowCount(); ++i){
            int id = ui->tvAccList->item(i,CI_ID)->data(Qt::EditRole).toInt();
            BankAccount* ba;
            if(id == 0){
                ba = new BankAccount;
                ba->id = UNID;
                ba->parent = curBank;
                ba->accNumber = ui->tvAccList->item(i,CI_ACCOUNTNUM)->text();
                ba->mt = ui->tvAccList->item(i,CI_MONEY)->data(Qt::EditRole).value<Money*>();
                curBank->bas<<ba;
            }
            else{
                ba = fondBankAccount(id);
                ba->accNumber = ui->tvAccList->item(i,CI_ACCOUNTNUM)->text();
                ba->mt = ui->tvAccList->item(i,CI_MONEY)->data(Qt::EditRole).value<Money*>();
            }
            if(ui->tvAccList->item(i,CI_NAME)){
                ba->niObj = ui->tvAccList->item(i,CI_NAME)->data(Qt::EditRole).value<SubjectNameItem*>();
                if(ba->niObj->getId() == NULL){
                    SubjectManager* sm = account->getSubjectManager();
                    sm->addNameItem(ba->niObj);
                    //创建二级科目
                    sm->addSndSubject(sm->getBankSub(),ba->niObj,"");
                }
            }
            else
                ba->niObj = NULL;
        }
        account->saveBank(curBank);
    }
    else if(editAction == EA_NEW){
        curBank->isMain = ui->chkIsMain->isChecked();
        if(curBank->isMain){
            foreach(Bank* bank, banks){
                if(bank->isMain){
                    bank->isMain = false;
                    account->saveBank(bank);
                }
            }
        }
        curBank->name = ui->bankName->text();
        curBank->lname = ui->bankLName->text();
        for(int i = 0; i < ui->tvAccList->rowCount(); ++i){
            BankAccount* ba  = new BankAccount;
            ba->id = 0;
            ba->niObj = NULL;
            ba->parent = curBank;
            ba->accNumber = ui->tvAccList->item(i,CI_ACCOUNTNUM)->text();
            ba->mt = ui->tvAccList->item(i,CI_MONEY)->data(Qt::EditRole).value<Money*>();
            if(ui->tvAccList->item(i,CI_NAME)){
                SubjectNameItem* ni = ui->tvAccList->item(i,CI_NAME)->data(Qt::EditRole).value<SubjectNameItem*>();
                if(ba->niObj == NULL){
                    ba->niObj = ni;
                    SubjectManager* sm = account->getSubjectManager();
                    sm->addNameItem(ba->niObj);
                    sm->addSndSubject(sm->getBankSub(),ba->niObj,"");
                }
            }
            else
                ba->niObj = NULL;
            curBank->bas<<ba;
        }
        banks<<curBank;
        account->addBank(curBank);
        QListWidgetItem* item  = new QListWidgetItem(curBank->name);
        ui->lstBank->addItem(item);
        ui->lstBank->setCurrentRow(banks.count()-1);        
    }
    editAction = EA_NONE;
    enWidget(false);
    viewBankAccounts();
}

/**
 * @brief 新建开户行
 */
void ApcBank::on_newBank_clicked()
{
    curBank = new Bank;
    curBank->id = UNID;
    curBank->isMain = false;
    editAction = EA_NEW;
    enWidget(true);
    ui->newBank->setEnabled(false);
    viewBankAccounts();
    ui->bankName->setFocus();
}


void ApcBank::on_delBank_clicked()
{
    int row  = ui->lstBank->currentRow();
    if(row == -1)
        return;
    if(curBank->id != UNID){
        foreach(BankAccount* ba, curBank->bas){
            if(ba->niObj && account->getDbUtil()->nameItemIsUsed(ba->niObj)){
                myHelper::ShowMessageBoxWarning(tr("该银行关联的科目已被采用，不能删除！"));
                return;
            }
        }
    }
    account->removeBank(curBank);
    delete ui->lstBank->takeItem(row);
    ui->tvAccList->setRowCount(0);
}

void ApcBank::on_newAcc_clicked()
{
    int row = ui->tvAccList->rowCount();
    ui->tvAccList->insertRow(row);
    QTableWidgetItem* item = new QTableWidgetItem;
    item->setData(Qt::EditRole,0);
    ui->tvAccList->setItem(row,CI_ID,item);
    BAMoneyTypeItem_new* cell = new BAMoneyTypeItem_new(account->getMasterMt());
    ui->tvAccList->setItem(row,CI_MONEY,cell);
    item = new QTableWidgetItem;
    ui->tvAccList->setItem(row,CI_ACCOUNTNUM,item);
    QPushButton* btn = new QPushButton(tr("创建关联科目"),ui->tvAccList);
    connect(btn,SIGNAL(clicked()),this,SLOT(crtNameBtnClicked()));
    ui->tvAccList->setCellWidget(row,CI_NAME,btn);
}

void ApcBank::on_delAcc_clicked()
{
    int row = ui->tvAccList->currentRow();
    int id = ui->tvAccList->item(row,CI_ID)->data(Qt::EditRole).toInt();
    if(id != UNID){
        BankAccount* ba = fondBankAccount(id);
        if(ba->niObj && account->getDbUtil()->nameItemIsUsed(ba->niObj)){
            myHelper::ShowMessageBoxWarning(tr("该银行帐号关联的科目已被采用，不能删除！"));
            return;
        }
        curBank->bas.removeOne(ba);
        account->getSubjectManager()->removeNameItem(ba->niObj,true);
        delete ba;
    }
    ui->tvAccList->removeRow(row);
}


/**
 * @brief 显示当前开户行的所有信息（名称及其其下的各个账户信息）
 */
void ApcBank::viewBankAccounts()
{
    disconnect(ui->tvAccList,SIGNAL(currentCellChanged(int,int,int,int)),
               this,SLOT(curBankAccountChanged(int,int,int,int)));
    if(!curBank){
        ui->editBank->setEnabled(false);
        ui->chkIsMain->setChecked(false);
        ui->bankName->clear();
        ui->bankLName->clear();
        ui->tvAccList->setRowCount(0);
        return;
    }
    else
        ui->editBank->setEnabled(isEdit);
    ui->chkIsMain->setChecked(curBank->isMain);
    ui->bankName->setText(curBank->name);
    ui->bankLName->setText(curBank->lname);
    ui->tvAccList->setRowCount(0);
    QTableWidgetItem* item;
    int row = -1;
    foreach(BankAccount* ba, curBank->bas){
        row++;
        ui->tvAccList->insertRow(row);
        item = new QTableWidgetItem;
        item->setData(Qt::EditRole,ba->id);
        ui->tvAccList->setItem(row,CI_ID,item);
        BAMoneyTypeItem_new* cell = new BAMoneyTypeItem_new(ba->mt);
        ui->tvAccList->setItem(row,CI_MONEY,cell);
        item = new QTableWidgetItem(ba->accNumber);
        ui->tvAccList->setItem(row,CI_ACCOUNTNUM,item);
        if(ba->niObj){            
            BankCfgNiCellWidget* ni = new BankCfgNiCellWidget(ba->niObj);
            ui->tvAccList->setItem(row,CI_NAME,ni);
        }
        else{            
            QPushButton* btn = new QPushButton(tr("创建关联科目"),ui->tvAccList);
            btn->setEnabled(false);
            connect(btn,SIGNAL(clicked()),this,SLOT(crtNameBtnClicked()));
            ui->tvAccList->setCellWidget(row,CI_NAME,btn);
        }
    }
    if(isEdit)
        connect(ui->tvAccList,SIGNAL(currentCellChanged(int,int,int,int)),
                       this,SLOT(curBankAccountChanged(int,int,int,int)));
    ui->delAcc->setEnabled(false);
}

/**
 * @brief 对控件的编辑性进行控制
 * @param en    true：编辑模式，false：只读模式
 */
void ApcBank::enWidget(bool en)
{
    ui->lstBank->setEnabled(!isEdit || !en);
    if(delegate)
        delegate->setReadOnly(!en);
    ui->chkIsMain->setEnabled(isEdit && en);
    ui->bankName->setReadOnly(!isEdit && !en);
    ui->bankLName->setReadOnly(!isEdit && !en);
    ui->newAcc->setEnabled(isEdit && en);
    ui->delAcc->setEnabled(isEdit && en && ui->tvAccList->currentRow() != -1);
    ui->submit->setEnabled(isEdit && en);
    ui->editBank->setText(en?tr("取消"):tr("编辑"));
    ui->newBank->setEnabled(isEdit && !en);
    if(!isEdit || en)
        ui->delBank->setEnabled(false);
    else
        ui->delBank->setEnabled(ui->lstBank->currentRow() != -1);
    if(curBank && ui->tvAccList->rowCount() > 0){
        for(int i = 0; i < curBank->bas.count(); ++i){
            if(curBank->bas.at(i)->niObj == UNID){
                QPushButton* btn = qobject_cast<QPushButton*>(ui->tvAccList->cellWidget(i,CI_NAME));
                if(btn)
                    btn->setEnabled(en);
            }
        }
    }
}

/**
 * @brief ApcBank::fondBankAccount
 *  找到指定id的银行帐号
 * @param id
 * @return
 */
BankAccount *ApcBank::fondBankAccount(int id)
{
    foreach(BankAccount* ba, curBank->bas){
        if(ba->id == id)
            return ba;
    }
    return NULL;
}

//////////////////////////////SmartSSubAdapteEditDlg/////////////////////////////////////
SmartSSubAdapteEditDlg::SmartSSubAdapteEditDlg(SubjectManager *sm, bool isEdit, QWidget *parent)
    :QDialog(parent),_isEdit(isEdit),_sm(sm)
{
    _fsub=0; _ssub=0;
    setWindowTitle(tr("配置适配子目"));
    lab1.setText(tr("一级科目"));
    lab2.setText(tr("关键字"));
    FSubItrator* it = sm->getFstSubItrator();
    while(it->hasNext()){
        it->next();
        FirstSubject* fsub = it->value();
        if(!fsub->isEnabled())
            continue;
        QVariant v;
        v.setValue<FirstSubject*>(fsub);
        cmbFSubs.addItem(fsub->getName(),v);
    }
    cmbFSubs.setCurrentIndex(-1);
    btnOk.setText(tr("确定"));
    btnCancel.setText((_isEdit)?tr("取消"):tr("忽略"));
    h1.addWidget(&lab1); h1.addWidget(&cmbFSubs);
    h2.addWidget(&lab2); h2.addWidget(&edtKeys);
    h3.addWidget(&btnOk); h3.addWidget(&btnCancel);
    lm = new QVBoxLayout;
    lm->addLayout(&h1);lm->addLayout(&h2);
    lm->addWidget(&lw);lm->addLayout(&h3);
    connect(&btnOk,SIGNAL(clicked()),this,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),this,SLOT(reject()));
    setLayout(lm);
    resize(300,400);
    connect(&lw,SIGNAL(currentRowChanged(int)),this,SLOT(currentAdapteItemChanged(int)));
    connect(&cmbFSubs,SIGNAL(currentIndexChanged(int)),this,SLOT(firstSubjectChanged(int)));
}

void SmartSSubAdapteEditDlg::setFirstSubject(FirstSubject *fsub)
{
    _fsub=fsub;
    int index = cmbFSubs.findText(fsub->getName());
    cmbFSubs.setCurrentIndex(index);
}

void SmartSSubAdapteEditDlg::setSecondSubject(SecondSubject *ssub)
{
    _ssub=ssub;
    for(int i = 0; i < lw.count(); ++i){
        SecondSubject* sub = lw.item(i)->data(Qt::UserRole).value<SecondSubject*>();
        if(ssub == sub){
            lw.setCurrentRow(i,QItemSelectionModel::Select);
            return;
        }
    }
}

void SmartSSubAdapteEditDlg::setKeys(QString keys)
{
    edtKeys.setText(keys);
}

void SmartSSubAdapteEditDlg::currentAdapteItemChanged(int currentRow)
{
    if(currentRow == -1)
        return;
    _ssub = lw.item(currentRow)->data(Qt::UserRole).value<SecondSubject*>();
}

void SmartSSubAdapteEditDlg::firstSubjectChanged(int index)
{
    _fsub = cmbFSubs.currentData().value<FirstSubject*>();
    loadSSubs();
}

void SmartSSubAdapteEditDlg::loadSSubs()
{
    lw.clear();
    foreach(SecondSubject* ssub, _fsub->getChildSubs(SORTMODE_NAME)){
        QListWidgetItem* item = new QListWidgetItem(ssub->getName(),&lw);
        QVariant v; v.setValue<SecondSubject*>(ssub);
        item->setData(Qt::UserRole,v);
    }
}

///////////////////////////ApcSubject////////////////////////////////////////////
ApcSubject::ApcSubject(Account *account, QWidget *parent) :
    QWidget(parent), ui(new Ui::ApcSubject),account(account)
{
    ui->setupUi(this);

    iniTag_subsys = false;
    iniTag_sub = false;
    iniTag_ni = false;
    iniTag_alias = false;
    iniTag_smart = false;
    editAction = APCEA_NONE;
    curFSub = NULL;
    curSSub = NULL;
    curNI = NULL;
    curNiCls = 0;
    curSubMgr = NULL;
    subSysNames = account->getSupportSubSys();    
    color_enabledSub.setColor(Qt::black);
    color_disabledSub.setColor(Qt::darkGray);
}

ApcSubject::~ApcSubject()
{
    if(!SmartAdaptes.isEmpty())
        qDeleteAll(SmartAdaptes);
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
            myHelper::ShowMessageBoxError(tr("在保存科目时发生错误！"));
            return;
        }
    }
}

void ApcSubject::windowShallClosed()
{
    if(account->isReadOnly())
        return;
    if(editAction != APCEA_NONE){
        if(QMessageBox::warning(this,tr("警告信息"),tr("科目设置信息已被修改，但未保存！"),QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes){
            switch(editAction){
            case APCEA_NEW_NICLS:
            case APCEA_EDIT_NICLS:
                on_btnNiClsCommit_clicked();
                break;
            case APCEA_NEW_NI:
            case APCEA_EDIT_NI:
                on_btnNiCommit_clicked();
                break;
            case APCEA_NEW_SSUB:
            case APCEA_EDIT_SSUB:
                on_btnSSubCommit_clicked();
                break;
            case APCEA_EDIT_FSUB:
                on_btnFSubCommit_clicked();
                break;
            }
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
            if(!account->importNewSubSys(subSys)){
                myHelper::ShowMessageBoxError(tr("在导入新科目系统到当前账户时发生错误"));
                return;
            }
            ui->tv_subsys->setCellWidget(i,3,NULL);
            ui->tv_subsys->setItem(i,3,new QTableWidgetItem(tr("已导入")));
            return;
        }
    }
}

/**
 * @brief 查看科目系统对接配置信息
 */
void ApcSubject::subSysCfgBtnClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    for(int i = 0; i < ui->tv_subsys->rowCount(); ++i){
        QPushButton* w = qobject_cast<QPushButton*>(ui->tv_subsys->cellWidget(i,4));
        if(!w)
            continue;
        if(btn == w){
            SubSysJoinCfgForm* form = new SubSysJoinCfgForm(subSysNames.at(i-1)->code,subSysNames.at(i)->code,account,this);
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
    if(editAction == APCEA_EDIT_FSUB)
        on_btnFSubCommit_clicked();
    if(row == -1){
        curFSub = NULL;
        ui->btnFSubEdit->setEnabled(false);
        ui->btnFSubCommit->setEnabled(false);
    }
    else{
        curFSub = ui->lwFSub->currentItem()->data(Qt::UserRole).value<FirstSubject*>();
        ui->btnFSubEdit->setEnabled(!account->isReadOnly() && (isFSubSetRight || isSSubSetRight));
    }
    enFSubWidget(false);
    curSSub = NULL;
    viewFSub();
    viewSSub();
    ui->btnInspectDup->setEnabled(!account->isReadOnly() && isPrivilegeUser);
}

/**
 * @brief 选择的二级科目改变了
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
        bool readonly = account->isReadOnly();
        ui->btnSSubDel->setEnabled(!readonly && (editAction == APCEA_EDIT_FSUB));
        ui->btnSSubEdit->setEnabled(!readonly && (editAction != APCEA_EDIT_FSUB));
    }
    enSSubWidget(false);
    viewSSub();
}

/**
 * @brief 当选择的子目改变时（用来执行子目的合并）
 */
void ApcSubject::SelectedSSubChanged()
{
    if(editAction != APCEA_EDIT_FSUB)
        return;
    ui->btnSSubMerge->setEnabled(ui->lwSSub->selectedItems().count() > 1);
}

void ApcSubject::ssubDBClicked(QListWidgetItem *item)
{
    on_btnSSubEdit_clicked();
}

/**
 * @brief ApcSubject::defSubCfgChanged
 *  默认科目设置改变
 * @param checked
 */
void ApcSubject::defSubCfgChanged(bool checked)
{
    if(editAction != APCEA_EDIT_SSUB && editAction != APCEA_NEW_SSUB)
        return;
    if(checked)
        ui->ssubWeight->setText(QString::number(DEFALUT_SUB_WEIGHT));
    else{
        if(editAction == APCEA_EDIT_SSUB)
            ui->ssubWeight->setText(QString::number(stack_ints.at(1)));
        else
            ui->ssubWeight->setText(QString::number(1));
    }
    ui->ssubWeight->setReadOnly(checked);
}

/**
 * @brief 跟踪科目配置选项页选择的变化，按需调用各自的初始化函数
 * @param index
 */
void ApcSubject::on_tw_currentChanged(int index)
{

    isPrivilegeUser = curUser->isAdmin() || curUser->isSuperUser();
    isFSubSetRight = curUser->haveRight(allRights.value(Right::Account_Config_SetFstSubject));
    isSSubSetRight = curUser->haveRight(allRights.value(Right::Account_Config_SetSndSubject));
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
    case APCS_ALIAS:
        init_alias();
        break;
    case APCS_SMARTADAPTE:
        init_smarts();
    }
}

/**
 * @brief 跟踪用户对名称类别的选择，并显示名称类别的细节
 * @param curRow
 */
void ApcSubject::currentNiClsRowChanged(int curRow)
{
    bool readonly = account->isReadOnly();
    QListWidgetItem* item = ui->lwNiCls->item(curRow);
    curNiCls = item?item->data(Qt::UserRole).toInt():0;
    ui->btnNiClsEdit->setEnabled(isPrivilegeUser && !readonly && curNiCls);
    ui->btnDelNiCls->setEnabled(isPrivilegeUser && !readonly && curNiCls);
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
        bool readonly = account->isReadOnly();
        ui->btnNiEdit->setEnabled(!readonly && isPrivilegeUser);
        ui->btnDelNI->setEnabled(!readonly && isPrivilegeUser);
        curNI = item->data(Qt::UserRole).value<SubjectNameItem*>();
        viewNI(curNI);
    }
}

/**
 * @brief 选择的名称条目改变（用来进行名称条目的合并）
 */
void ApcSubject::selectedNIChanged()
{
    if(ui->lwNI->selectedItems().count() > 1)
        ui->btnNIMerge->setEnabled(true);
    else
        ui->btnNIMerge->setEnabled(false);
}

/**
 * @brief 双击对名称条目进行编辑
 * @param item
 */
void ApcSubject::niDoubleClicked(QListWidgetItem *item)
{
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
            if(!account->isReadOnly() && account->isSubSysConfiged(si->code) &&
                    curUser->haveRight(allRights.value(Right::Account_Config_SetUsedSubSys)))
                connect(btn,SIGNAL(clicked()),this,SLOT(importBtnClicked()));
            else
                btn->setEnabled(false);
        }
        if(i > 0){
            QPushButton* btn = new QPushButton(tr("查看对接配置"),this);
            ui->tv_subsys->setCellWidget(i,4,btn);
            if(curUser->isSuperUser() || curUser->isAdmin())
                connect(btn,SIGNAL(clicked()),this,SLOT(subSysCfgBtnClicked()));
            else
                btn->setEnabled(false);
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
    if(!account->isReadOnly() && isPrivilegeUser){
        connect(ui->lwNI,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(niDoubleClicked(QListWidgetItem*)));
        connect(ui->lwNiCls,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(niClsDoubleClicked(QListWidgetItem*)));

    }
    if(isPrivilegeUser)
        connect(ui->lwNI,SIGNAL(itemSelectionChanged()),SLOT(selectedNIChanged()));
    curSubMgr = account->getSubjectManager();
    if(!isPrivilegeUser)
        ui->btnNIMerge->setEnabled(false);    
    ui->btnInspectNameConflit->setEnabled(isPrivilegeUser);
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
    QRadioButton* initRb=NULL;
    int row = 0;
    QVBoxLayout* l = new QVBoxLayout(this);
    foreach(SubSysNameItem* sn, subSysNames){
        row++;
        QRadioButton* r = new QRadioButton(sn->name,this);
        if(row == 1)
            initRb = r;
        l->addWidget(r);
        connect(r,SIGNAL(toggled(bool)),this,SLOT(selectedSubSys(bool)));
    }
    ui->gb_subSys->setLayout(l);
    iniTag_sub = true;
    if(initRb)
        initRb->setChecked(true);
    connect(ui->lwFSub,SIGNAL(currentRowChanged(int)),SLOT(curFSubChanged(int)));
    if(isFSubSetRight && !account->isReadOnly()){
        connect(ui->lwFSub,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(fsubDBClicked(QListWidgetItem*)));
        connect(ui->lwSSub,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(ssubDBClicked(QListWidgetItem*)));
        connect(ui->ssubIsDef,SIGNAL(clicked(bool)),this,SLOT(defSubCfgChanged(bool)));
    }
    connect(ui->lwSSub,SIGNAL(currentRowChanged(int)),SLOT(curSSubChanged(int)));    
    connect(ui->cmbFSubCls,SIGNAL(currentIndexChanged(int)),this,SLOT(curFSubClsChanged(int)));    

    if(account->isReadOnly()){
        ui->btnInspectDup->setEnabled(false);
        ui->btnInspectNameConflit->setEnabled(false);
        ui->btnSSubAdd->setEnabled(false);
    }
    else{
        if(isPrivilegeUser){
            connect(ui->lwSSub,SIGNAL(itemSelectionChanged()),this,SLOT(SelectedSSubChanged()));
            ui->btnInspectDup->setEnabled(isPrivilegeUser);
        }
        ui->btnSSubAdd->setEnabled(isSSubSetRight);
    }
}

void ApcSubject::init_alias()
{
    if(iniTag_alias)
        return;
    ui->tabAlias->setLayout(ui->layAlias);
    foreach(SubjectNameItem* ni, SubjectManager::getAllNI().values()){
        if(ni->haveAlias()){
            QVariant v;v.setValue<SubjectNameItem*>(ni);
            QListWidgetItem* item = new QListWidgetItem(ni->getShortName(),ui->lwAliasNames);
            item->setData(Qt::UserRole,v);
        }
    }
    connect(ui->lwAliasNames,SIGNAL(currentRowChanged(int)),this,SLOT(curNameObjChanged(int)));
    connect(ui->lwAlias,SIGNAL(currentRowChanged(int)),this,SLOT(curAliasChanged(int)));
    connect(ui->lwAlias,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(aliasListContextMenuRequest(QPoint)));
    connect(ui->chkIsoAlias,SIGNAL(toggled(bool)),this,SLOT(showIsolatedAlias(bool)));
    ui->lwAlias->addAction(ui->actDelAlias);
    iniTag_alias = true;
}

void ApcSubject::init_smarts()
{
    if(iniTag_smart)
        return;
    foreach(SubSysNameItem* item, subSysNames){
        ui->cmbSubSys->addItem(item->name,item->code);
    }
    if(!subSysNames.isEmpty()){
        ui->cmbSubSys->setCurrentIndex(0);
        subjectSystemChanged(0);
    }
    connect(ui->cmbSubSys,SIGNAL(currentIndexChanged(int)),this,SLOT(subjectSystemChanged(int)));
    connect(ui->twSmarts,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(SmartTableMenuRequested(QPoint)));
    iniTag_smart = true;
}

/**
 * @brief  将指定科目系统中所有的智能子目适配配置项装载到表中
 * @param sm
 */
void ApcSubject::loadSmartItems(SubjectManager *sm)
{
    account->getDbUtil()->loadSmartSSubAdaptes(sm,SmartAdaptes);
    ui->twSmarts->setRowCount(SmartAdaptes.count());
    QVariant v;
    for(int i = 0; i < SmartAdaptes.count(); ++i){
        SmartSSubAdapteItem* ai = SmartAdaptes.at(i);
        QTableWidgetItem* item = new QTableWidgetItem(ai->fsub->getName());
        v.setValue<FirstSubject*>(ai->fsub);
        item->setData(Qt::UserRole,v);
        ui->twSmarts->setItem(i,0,item);
        item = new QTableWidgetItem(ai->ssub->getName());
        v.setValue<SecondSubject*>(ai->ssub);
        item->setData(Qt::UserRole,v);
        ui->twSmarts->setItem(i,1,item);
        ui->twSmarts->setItem(i,2,new QTableWidgetItem(ai->keys));
    }
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
        if(!fsub->isEnabled())
            item->setForeground(color_disabledSub);
        item->setData(Qt::UserRole,v);
        ui->lwFSub->addItem(item);
    }
}

/**
 * @brief 装载当前选择的一级科目的所有二级科目
 */
void ApcSubject::loadSSub(SortByMode sortBy)
{
    ui->lwSSub->clear();
    if(!curFSub)
        return;
    QVariant v;
    QListWidgetItem* item;
    QList<int> usedNiIds;
    SubjectNameItem* ni = NULL;
//    bool fonded = false;    //是否找到第一个冲突科目
    QList<SecondSubject*> childs = curFSub->getChildSubs();
    if(sortBy == SORTMODE_CRT_TIME)
        qSort(childs.begin(),childs.end(),byCreateTimeThan_ss);
    else
        qSort(childs.begin(),childs.end(),bySubNameThan_ss);
    foreach(SecondSubject* ssub, childs){
//        if(!fonded){
//            int nid = ssub->getNameItem()->getId();
//            if(!usedNiIds.contains(nid))
//                usedNiIds<<nid;
//            else{
//                fonded = true;
//                ni = ssub->getNameItem();
//            }
//        }
        v.setValue<SecondSubject*>(ssub);
        item = new QListWidgetItem(ssub->getName());
        item->setData(Qt::UserRole,v);
        if(!ssub->isEnabled())
            item->setForeground(color_disabledSub);
        ui->lwSSub->addItem(item);
    }
//    if(ni){
//        QList<QListWidgetItem*> items = ui->lwSSub->findItems(ni->getShortName(),Qt::MatchExactly);
//        QList<SecondSubject*> subjects;
//        foreach(QListWidgetItem* item, items){
//            item->setSelected(true);
//            SecondSubject* ssub = item->data(Qt::UserRole).value<SecondSubject*>();
//            subjects<<ssub;
//        }
//        ui->lwSSub->scrollToItem(items.first());
//        QMessageBox::warning(this,tr("二级科目名称冲突")
//                             ,tr("发现%1处使用名称“%2”的二级科目").arg(items.count()).arg(ni->getShortName()));
//        if(!account->isReadOnly())
//            ui->btnSSubMerge->setEnabled(true);
//    }

}

void ApcSubject::loadIsolatedAlias()
{
    ui->lwAlias->clear();
    foreach(NameItemAlias* alias, SubjectManager::getAllIsolatedAlias()){
        QListWidgetItem* item = new QListWidgetItem(alias->longName(),ui->lwAlias);
        QVariant v; v.setValue<NameItemAlias*>(alias);
        item->setData(Qt::UserRole,v);
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
    loadSSub();
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
        ui->ssubNIID->setText(QString::number(curSSub->getNameItem()->getId()));
        ui->ssubName->setText(curSSub->getName());
        ui->ssubLName->setText(curSSub->getLName());
        ui->ssubRemCode->setText(curSSub->getRemCode());
        ui->ssubIsDef->setChecked(curFSub->getDefaultSubject() == curSSub);
    }
    else{
        ui->ssubID->clear();
        ui->ssubCode->clear();
        ui->ssubCreator->clear();
        ui->ssubCrtTime->clear();
        ui->ssubDisTime->clear();
        ui->ssubIsEnable->setChecked(false);
        ui->ssubWeight->clear();
        ui->ssubNIID->clear();
        ui->ssubName->clear();
        ui->ssubLName->clear();
        ui->ssubRemCode->clear();
        ui->ssubIsDef->setChecked(false);
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
    ui->lwFSub->setEnabled(!isFSubSetRight || !en);
    ui->FSubCls->setEnabled(isFSubSetRight && en);
    ui->fsubCode->setReadOnly(!isFSubSetRight || !en);
    ui->fsubName->setReadOnly(!isFSubSetRight || !en);
    ui->fsubRemCode->setReadOnly(!isFSubSetRight || !en);
    ui->fsubWeight->setReadOnly(!isFSubSetRight || !en);
    ui->fsubIsEnable->setEnabled(isFSubSetRight && en);
    ui->isUseWb->setEnabled(isFSubSetRight && en);
    ui->jdDir_N->setEnabled(isFSubSetRight && en);
    ui->jdDir_P->setEnabled(isFSubSetRight && en);
    ui->btnFSubEdit->setText(en?tr("取消"):tr("编辑"));
    ui->btnFSubCommit->setEnabled((isFSubSetRight||isSSubSetRight) && en);
    ui->btnSSubAdd->setEnabled((isFSubSetRight||isSSubSetRight) && en && !account->isReadOnly());
    ui->btnSSubDel->setEnabled((isFSubSetRight||isSSubSetRight) && en && (ui->lwSSub->currentRow() != -1));
    QApplication::processEvents();
}

/**
 * @brief 控制二级科目编辑控件的可编辑性
 * @param en
 */
void ApcSubject::enSSubWidget(bool en)
{
    ui->lwSSub->setEnabled(!isSSubSetRight || !en);
    ui->ssubCode->setReadOnly(!isSSubSetRight || !en);
    ui->ssubIsEnable->setEnabled(isSSubSetRight && en);
    ui->ssubWeight->setReadOnly(!isSSubSetRight || !en);
    ui->btnSSubCommit->setEnabled(isSSubSetRight && en);
    ui->btnSSubEdit->setText(en?tr("取消"):tr("编辑"));
    ui->ssubIsDef->setEnabled(isSSubSetRight && en);
}

/**
 * @brief 控制名称条目编辑控件的可编辑性
 * @param en
 */
void ApcSubject::enNiWidget(bool en)
{
    ui->lwNI->setEnabled(isPrivilegeUser && !en);
    ui->btnNiEdit->setText(en?tr("取消"):tr("编辑"));
    ui->btnNiCommit->setEnabled(isPrivilegeUser && en);
    ui->niCls->setEnabled(isPrivilegeUser && en);
    ui->niName->setReadOnly(!isPrivilegeUser || !en);
    ui->niLName->setReadOnly(!isPrivilegeUser || !en);
    ui->niRemCode->setReadOnly(!isPrivilegeUser || !en);
    ui->btnNewNI->setEnabled(isPrivilegeUser && !en);
    ui->btnDelNI->setEnabled(isPrivilegeUser && !en);
}

/**
 * @brief 控制名称条目类别编辑控件的可编辑性
 * @param en
 */
void ApcSubject::enNiClsWidget(bool en)
{
    ui->lwNiCls->setEnabled(isPrivilegeUser && !en);
    ui->btnNiClsEdit->setText(en?tr("取消"):tr("编辑"));
    ui->btnNiClsCommit->setEnabled(isPrivilegeUser && en);
    //ui->niClsCode->setReadOnly(false);
    ui->niClsName->setReadOnly(!isPrivilegeUser || !en);
    ui->niClsExplain->setReadOnly(!isPrivilegeUser ||!en);
    ui->btnNewNiCls->setEnabled(isPrivilegeUser && !en);
    ui->btnDelNiCls->setEnabled(isPrivilegeUser && !en);
}

/**
 * @brief 装载选择类别的名称条目到列表
 */
void ApcSubject::loadNameItems()
{
    ui->lwNI->clear();
    int curCls = ui->niClsView->itemData(ui->niClsView->currentIndex()).toInt();
    QVariant v; QListWidgetItem* item;
    QList<SubjectNameItem*> nis = SubjectManager::getAllNameItems(SORTMODE_NAME);
    foreach(SubjectNameItem* ni, nis){
        if((curCls != 0) && (curCls != ni->getClassId()))
            continue;
        item = new QListWidgetItem(ni->getShortName());
        v.setValue<SubjectNameItem*>(ni);
        item->setData(Qt::UserRole,v);
        ui->lwNI->addItem(item);
    }
}

/**
 * @brief 在智能适配页面选择一个科目系统
 * @param index
 */
void ApcSubject::subjectSystemChanged(int index)
{
    int subSysCode = ui->cmbSubSys->itemData(index).toInt();
    SubjectManager* sm = account->getSubjectManager(subSysCode);
    //视情况保存当前表格中显示的内容
    qDeleteAll(SmartAdaptes);
    ui->twSmarts->clearContents();
    loadSmartItems(sm);
}

/**
 * @brief 创建子目智能适配表格的上下文菜单
 * @param pos
 */
void ApcSubject::SmartTableMenuRequested(const QPoint &pos)
{
    QMenu m;
    m.addAction(ui->actAddSmartItem);
    int row = ui->twSmarts->rowAt(pos.y());
    if(row >= 0 && row <= ui->twSmarts->rowCount()){
        m.addAction(ui->actEditSmartItem);
        m.addAction(ui->actRemoveSmartItem);
    }
    m.exec(ui->twSmarts->mapToGlobal(pos));
}

void ApcSubject::curNameObjChanged(int index)
{
    if(ui->chkIsoAlias->isChecked())
        return;
    if(index <0 || index >= ui->lwAliasNames->count())
        return;
    ui->lwAlias->clear();
    SubjectNameItem* ni = ui->lwAliasNames->currentItem()->data(Qt::UserRole).value<SubjectNameItem*>();
    foreach (NameItemAlias* alias, ni->getAliases()) {
        QListWidgetItem* item = new QListWidgetItem(alias->longName(),ui->lwAlias);
        QVariant v; v.setValue<NameItemAlias*>(alias);
        item->setData(Qt::UserRole,v);
    }
}

void ApcSubject::curAliasChanged(int index)
{
    if(index < 0 || index >= ui->lwAlias->count())
        return;
    NameItemAlias* alias = ui->lwAlias->currentItem()->data(Qt::UserRole).value<NameItemAlias*>();
    ui->edtAliasSName->setText(alias->shortName());
    ui->edtAliasRemCode->setText(alias->rememberCode());
    if(alias->createdTime().isValid())
        ui->edtAliasCrtTime->setText(alias->createdTime().toString(Qt::ISODate));
    else
        ui->edtAliasCrtTime->clear();
}

void ApcSubject::showIsolatedAlias(bool checked)
{
    ui->lwAliasNames->setEnabled(!checked);
    if(checked)
        loadIsolatedAlias();
    else{
        ui->lwAlias->clear();
        if(ui->lwAliasNames->currentItem()){
            SubjectNameItem* ni = ui->lwAliasNames->currentItem()->data(Qt::UserRole).value<SubjectNameItem*>();
            foreach (NameItemAlias* alias, ni->getAliases()) {
                QListWidgetItem* item = new QListWidgetItem(alias->longName(),ui->lwAlias);
                QVariant v; v.setValue<NameItemAlias*>(alias);
                item->setData(Qt::UserRole,v);
            }
        }
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
        curNI = curSubMgr->addNameItem(name,lname,remCode,cls,QDateTime::currentDateTime(),curUser);
        QVariant v;
        v.setValue<SubjectNameItem*>(curNI);
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setData(Qt::UserRole,v);
        ui->lwNI->addItem(item);
        ui->lwNI->setCurrentItem(item);
    }
    if(!account->getDbUtil()->saveNameItem(curNI))
        myHelper::ShowMessageBoxError(tr("在将名称条目保存到账户数据库中时出错！"));
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
    if(curSubMgr->nameItemIsUsed(curNI)){
        myHelper::ShowMessageBoxWarning(tr("该名称条目已被某些二级科目引用，不能删除！"));
        return;
    }
    SubjectNameItem* ni = curNI;
    QListWidgetItem* item = ui->lwNI->takeItem(ui->lwNI->currentRow());
    delete item;
    curSubMgr->removeNameItem(ni,true);
    if(ui->lwNI->currentRow() != -1)
        curNI = ui->lwNI->currentItem()->data(Qt::UserRole).value<SubjectNameItem*>();
    else{
        curNI = NULL;
        ui->btnDelNI->setEnabled(false);
    }
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
        myHelper::ShowMessageBoxError(tr("在将名称条目类别保存到账户数据库中时发生错误！"));
    editAction = APCEA_NONE;
    enNiClsWidget(false);
}

/**
 * @brief 删除名称条目类别
 */
void ApcSubject::on_btnDelNiCls_clicked()
{
    if(SubjectManager::isUsedNiCls(curNiCls)){
        myHelper::ShowMessageBoxWarning(tr("该名称条目类别已被使用，不能删除！"));
        return ;
    }
    QListWidgetItem* item = ui->lwNiCls->takeItem(ui->lwNiCls->currentRow());
    int clsCode = item->data(Qt::UserRole).toInt();
    account->getDbUtil()->removeNameItemCls(clsCode);
    delete item;
}

void ApcSubject::on_niClsBox_toggled(bool en)
{
    bool readonly = account->isReadOnly();
    ui->niClsCode->setEnabled(en);
    ui->niClsName->setEnabled(en);
    ui->niClsExplain->setEnabled(en);
    ui->btnNewNiCls->setEnabled(isPrivilegeUser && !readonly && en && editAction==APCEA_NONE);
    ui->btnNiClsEdit->setEnabled(isPrivilegeUser && !readonly && en && curNiCls && editAction==APCEA_NONE);
    ui->btnNiClsCommit->setEnabled(isPrivilegeUser && !readonly && en && (editAction==APCEA_EDIT_NICLS || editAction==APCEA_NEW_NICLS));
    ui->btnDelNiCls->setEnabled(isPrivilegeUser && !readonly && en && curNiCls);
}

/**
 * @brief 启动或取消对当前选中的一级科目的编辑
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
        if(isPrivilegeUser && ui->lwSSub->selectedItems().count() > 1)
            ui->btnSSubMerge->setEnabled(true);
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
        //如果有任何新建的不想提交的二级科目对象，则要删除否则造成内存泄漏
        for(int i = 0; i < ui->lwSSub->count(); ++ i){
            SecondSubject* sub = ui->lwSSub->item(i)->data(Qt::UserRole).value<SecondSubject*>();
            if(sub->getId() == 0)
                delete sub;
        }
        loadSSub();
        editAction = APCEA_NONE;
        enFSubWidget(false);
        if(ui->btnSSubMerge->isEnabled())
            ui->btnSSubMerge->setEnabled(false);
    }
}

/**
 * @brief 提交对一级科目的编辑结果
 */
void ApcSubject::on_btnFSubCommit_clicked()
{
    if(editAction != APCEA_EDIT_FSUB)
        return;
    //保存科目的属性
    curFSub->setSubClass((SubjectClass)ui->FSubCls->itemData(ui->FSubCls->currentIndex()).toInt());
    curFSub->setName(ui->fsubName->text());
    curFSub->setRemCode(ui->fsubRemCode->text());
    curFSub->setCode(ui->fsubCode->text());
    bool enableChanged = (curFSub->isEnabled() && !ui->fsubIsEnable->isChecked()) ||
            (!curFSub->isEnabled() && ui->fsubIsEnable->isChecked());
    curFSub->setEnabled(ui->fsubIsEnable->isChecked());
    if(curFSub->isUseForeignMoney() ^ ui->isUseWb->isChecked()){
        if(!ui->isUseWb->isChecked()){
            bool isExist = true;
            account->getDbUtil()->isUsedWbForFSub(curFSub,isExist);
            if(isExist){
                myHelper::ShowMessageBoxWarning(tr("科目“%1”先前曾使用过外币且余额不为0，不能去除该科目使用外币的属性！").arg(curFSub->getName()));
                ui->isUseWb->setChecked(true);
            }
            else
                curFSub->setIsUseForeignMoney(false);
        }
        else
            curFSub->setIsUseForeignMoney(true);
    }

    curFSub->setJdDir(ui->jdDir_P->isChecked());
    curFSub->setWeight(ui->fsubWeight->text().toInt());
    if(stack_strs.at(1) != curFSub->getName())
        ui->lwFSub->currentItem()->setText(stack_strs.at(1));
    stack_ints.clear();
    stack_strs.clear();
    //保存二级科目
    QList<int> ids = curFSub->getAllSSubIds();
    for(int i = 0; i < ui->lwSSub->count(); ++i){
        SecondSubject* sub = ui->lwSSub->item(i)->data(Qt::UserRole).value<SecondSubject*>();
        if(curFSub->containChildSub(sub))
            ids.removeOne(sub->getId());
        else
            curFSub->addChildSub(sub);
    }
        if(!ids.isEmpty()){
        foreach(int id, ids)
            curFSub->removeChildSubForId(id);
    }
    if(!curSubMgr->saveFS(curFSub))
        myHelper::ShowMessageBoxError(tr("在保存一级科目时发生错误！"));
    editAction = APCEA_NONE;
    if(enableChanged){
        if(curFSub->isEnabled())
            ui->lwFSub->currentItem()->setForeground(color_enabledSub);
        else
            ui->lwFSub->currentItem()->setForeground(color_disabledSub);
    }
    enFSubWidget(false);
}

/**
 * 启动或取消对当前选中的二级科目的编辑
 */
void ApcSubject::on_btnSSubEdit_clicked()
{
    if(editAction == APCEA_NONE){
        stack_strs.push(curSSub->getCode());
        stack_ints.push(curSSub->getWeight());
        stack_ints.push(curSSub->isEnabled()?1:0);
        stack_ints.push((curFSub->getDefaultSubject() == curSSub)?1:0);
        editAction = APCEA_EDIT_SSUB;
        enSSubWidget(true);
        ui->ssubCode->setFocus();
    }
    else if(editAction == APCEA_EDIT_SSUB){
        ui->ssubCode->setText(stack_strs.pop());
        ui->ssubWeight->setText(QString::number(stack_ints.pop()));
        ui->ssubIsEnable->setChecked((stack_ints.pop()==1)?true:false);
        ui->ssubIsDef->setChecked(stack_ints.pop()==1?true:false);
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
    bool enableChanged = (curSSub->isEnabled() && !ui->ssubIsEnable->isChecked()) ||
            (!curSSub->isEnabled() && ui->ssubIsEnable->isChecked());
    curSSub->setEnabled(ui->ssubIsEnable->isChecked());
    curSSub->setWeight(ui->ssubWeight->text().toInt());
    //如果用户将本不是默认的科目设置为默认了
    if(ui->ssubIsDef->isChecked() ^ stack_ints.at(2)){
        if(ui->ssubIsDef->isChecked())
            curFSub->setDefaultSubject(curSSub);
        else
            curFSub->setDefaultSubject(0);
        if(!curSubMgr->saveFS(curFSub))
            myHelper::ShowMessageBoxError(tr("在当前一级科目的默认科目时出错！"));
    }
    if(!curSubMgr->saveSS(curSSub))
        myHelper::ShowMessageBoxError(tr("在将二级科目保存到账户数据库中时出错！"));
    editAction = APCEA_NONE;
    if(enableChanged){
        if(curSSub->isEnabled())
            ui->lwSSub->currentItem()->setForeground(color_enabledSub);
        else
            ui->lwSSub->currentItem()->setForeground(color_disabledSub);
    }
    enSSubWidget(false);
    stack_ints.clear();
    stack_strs.clear();
}

/**
 * @brief 删除选中的二级科目
 */
void ApcSubject::on_btnSSubDel_clicked()
{
    if(curSubMgr->isUsedSSub(curSSub)){
        myHelper::ShowMessageBoxWarning(tr("二级科目“%1”已在账户中被采用，不能删除！").arg(curSSub->getName()));
        return;
    }
    //curFSub->removeChildSub(curSSub);
    delete ui->lwSSub->takeItem(ui->lwSSub->currentRow());
    //curSubMgr->saveFS(curFSub);
}

/**
 * @brief 新建二级科目
 */
void ApcSubject::on_btnSSubAdd_clicked()
{
    if(!curFSub)
        return;
    NewSndSubDialog dlg(curFSub, this);
    if(dlg.exec() == QDialog::Accepted){
        SecondSubject* sub = dlg.getCreatedSubject();
        QListWidgetItem* item = new QListWidgetItem(sub->getName());
        QVariant v; v.setValue<SecondSubject*>(sub);
        item->setData(Qt::UserRole,v);
        ui->lwSSub->addItem(item);
        ui->lwSSub->setCurrentItem(item);
        //viewFSub();
        //on_btnFSubCommit_clicked();
    }

}

/**
 * @brief 检测名称条目是否有重复性的冲突
 */
void ApcSubject::on_btnInspectNameConflit_clicked()
{
    QList<SubjectNameItem*> nis,allNis;
    QList<QString> names;
    allNis = SubjectManager::getAllNameItems();
    for(int i = 0; i < allNis.count(); ++i){
        SubjectNameItem* ni = allNis.at(i);
        if(names.contains(ni->getShortName())){
            QList<QListWidgetItem*> items = ui->lwNI->findItems(ni->getShortName(),Qt::MatchExactly);
            if(!items.isEmpty()){
                foreach(QListWidgetItem* item, items)
                    item->setSelected(true);
                myHelper::ShowMessageBoxWarning(tr("名称条目“%1”有重复！").arg(ni->getShortName()));
                if(!account->isReadOnly())
                    ui->btnNIMerge->setEnabled(true);
                else
                    ui->btnNIMerge->setEnabled(false);
                ui->lwNI->scrollToItem(items.first());
            }
            return;
        }
        else{
            nis<<ni;
            names<<ni->getShortName();
        }
    }
    myHelper::ShowMessageBoxInfo(tr("未发现重复的名称条目！"));
    ui->btnInspectNameConflit->setEnabled(false);
    ui->btnNIMerge->setEnabled(false);
}

/**
 * @brief 合并选定的名称条目
 */
void ApcSubject::on_btnNIMerge_clicked()
{
    QList<QListWidgetItem*> items = ui->lwNI->selectedItems();
    if(items.count() < 2)
        return;
    QList<SubjectNameItem*> nameItems;
    foreach(QListWidgetItem* item, items){
        SubjectNameItem* ni = item->data(Qt::UserRole).value<SubjectNameItem*>();
        if(ni)
            nameItems<<ni;
    }
    //选择保留的名称条目
    QDialog dlg(this);
    QTreeWidget tv(&dlg);
    tv.setColumnCount(3);
    QVariant v;
    foreach(SubjectNameItem* ni, nameItems){
        QStringList sl;
        sl.append(QString::number(ni->getId()));
        sl.append(ni->getShortName());
        sl.append(ni->getCreateTime().toString(Qt::ISODate));
        QTreeWidgetItem* item = new QTreeWidgetItem(sl);
        v.setValue<SubjectNameItem*>(ni);
        item->setData(0,Qt::UserRole,v);
        tv.addTopLevelItem(item);
    }
    QPushButton btnOk(QIcon(":/images/btn_ok.png"),tr("确定"), &dlg);
    QPushButton btnCancel(QIcon(":/images/btn_close.png"),tr("取消"), &dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb;
    lb.addWidget(&btnOk);
    lb.addWidget(&btnCancel);
    QLabel title(tr("请选择一个保留的名称条目："),&dlg);
    QVBoxLayout* l = new QVBoxLayout;
    l->addWidget(&title);
    l->addWidget(&tv);
    l->addLayout(&lb);
    dlg.setLayout(l);
    if(dlg.exec() == QDialog::Rejected)
        return;
    if(!tv.currentItem())
        return;
    SubjectNameItem* preNI = tv.currentItem()->data(0,Qt::UserRole).value<SubjectNameItem*>();
    nameItems.removeOne(preNI);
    if(!mergeNameItem(preNI,nameItems))
        myHelper::ShowMessageBoxError(tr("合并过程发生错误，请查看日志！"));
    foreach(QListWidgetItem* item, items)
        delete ui->lwNI->takeItem(ui->lwNI->row(item));
}

/**
 * @brief 按指定的排序方式重新装载当前一级科目下的二级科目
 * @param checked
 */
void ApcSubject::on_rdoSortbyName_toggled(bool checked)
{
    if(checked)
        loadSSub();
    else
        loadSSub(SORTMODE_CRT_TIME);
}

/**
 * @brief 合并选定的二级科目
 */
void ApcSubject::on_btnSSubMerge_clicked()
{
    QList<QListWidgetItem*> items = ui->lwSSub->selectedItems();
    if(items.count() < 2)
        return;
    QList<SecondSubject*> subjects;
    foreach(QListWidgetItem* item, items){
        SecondSubject* ssub = item->data(Qt::UserRole).value<SecondSubject*>();
        subjects<<ssub;
    }
    int preSubIndex = -1;
    bool isCancel = false;
    if(!mergeSndSubject(subjects,preSubIndex,isCancel))
        myHelper::ShowMessageBoxError(tr("合并科目过程出错！"));
    if(isCancel)
        return;
    QListWidgetItem* preItem = items.at(preSubIndex);
    foreach(QListWidgetItem* item, items){
        if(item == preItem)
            continue;
        delete ui->lwSSub->takeItem(ui->lwSSub->row(item));
    }
    on_btnFSubCommit_clicked();
    preSubIndex = ui->lwSSub->row(preItem);
    curSSubChanged(preSubIndex);
}

/**
 * @brief 检查当前一级科目下使用同一个名称条目的重复子目
 */
void ApcSubject::on_btnInspectDup_clicked()
{
    QList<int> usedNiIds;
    SubjectNameItem* ni=NULL;
    for(int i = 0; i < ui->lwSSub->count(); ++i){
        SecondSubject* ssub = ui->lwSSub->item(i)->data(Qt::UserRole).value<SecondSubject*>();
        int nid = ssub->getNameItem()->getId();
        if(!usedNiIds.contains(nid))
            usedNiIds<<nid;
        else{
            ni = ssub->getNameItem();
            break;
        }
    }
    if(ni){
        ui->lwSSub->setCurrentRow(-1,QItemSelectionModel::Clear);
        QList<QListWidgetItem*> items = ui->lwSSub->findItems(ni->getShortName(),Qt::MatchExactly);
        QList<SecondSubject*> subjects;
        foreach(QListWidgetItem* item, items){
            item->setSelected(true);
            SecondSubject* ssub = item->data(Qt::UserRole).value<SecondSubject*>();
            subjects<<ssub;
        }
        ui->lwSSub->scrollToItem(items.first());
        myHelper::ShowMessageBoxWarning(tr("发现%1处使用名称“%2”的二级科目").arg(items.count()).arg(ni->getShortName()));
        if(!account->isReadOnly() && editAction == APCEA_EDIT_FSUB)
            ui->btnSSubMerge->setEnabled(true);
    }
    else{
        myHelper::ShowMessageBoxInfo(tr("未发现重复子目！"));
        ui->btnSSubMerge->setEnabled(false);
        ui->btnInspectDup->setEnabled(false);
    }
}

/**
 * @brief 由默认的配置模板初始化智能适配配置项并装入表格
 */
void ApcSubject::on_btnLoadDefs_clicked()
{
    QString fname = QFileDialog::getOpenFileName(this,tr("选择模板文件"),".");
    if(fname.isEmpty())
        return;
    QFile file(fname);
    if(!file.open(QFile::ReadOnly|QFile::Text)){
        myHelper::ShowMessageBoxError(tr("文件“%1”打开失败！").arg(fname));
        return;
    }
    qDeleteAll(SmartAdaptes);
    SmartAdaptes.clear();
    int subSysCode = ui->cmbSubSys->itemData(ui->cmbSubSys->currentIndex()).toInt();
    SubjectManager* sm = account->getSubjectManager(subSysCode);
    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    int row = 0;
    while(!ts.atEnd()){
        QString line = ts.readLine();
        QStringList ls = line.split("||");
        if(ls.count() != 4){
            myHelper::ShowMessageBoxError(tr("文件格式错误：%1").arg(line));
            return;
        }
        FirstSubject* fsub = sm->getFstSubject(ls.at(1));
        if(!fsub){
            myHelper::ShowMessageBoxError(tr("无效的一级科目代码：%1").arg(line));
            return;
        }
        SecondSubject* ssub = fsub->getChildSub(ls.at(3));
        if(!ssub){
            //显示一个对话框由用户来抉择采用哪个二级科目来适配当前的关键字
            SmartSSubAdapteEditDlg dlg(sm,false,this);
            dlg.setFirstSubject(fsub);
            dlg.setKeys(ls.at(2));
            if(dlg.exec() == QDialog::Rejected)
                continue;
            ssub = dlg.getSecondSubject();
            if(!ssub){
                myHelper::ShowMessageBoxWarning(tr("您未选择适配的二级科目，该条将忽略！"));
                continue;
            }
        }
        //fsub->addSmartAdapteSSub(ls.at(2),ssub);
        SmartSSubAdapteItem* ai = new SmartSSubAdapteItem;
        ai->id = 0;
        ai->subSys = subSysCode;
        ai->fsub = fsub;
        ai->ssub = ssub;
        ai->keys = ls.at(2);
        SmartAdaptes<<ai;
        ui->twSmarts->insertRow(row);
        QVariant v;
        QTableWidgetItem* item = new QTableWidgetItem(fsub->getName());
        v.setValue<FirstSubject*>(fsub);
        item->setData(Qt::UserRole,v);
        ui->twSmarts->setItem(row,0,item);
        item = new QTableWidgetItem(ssub->getName());
        v.setValue<SecondSubject*>(ssub);
        item->setData(Qt::UserRole,v);
        ui->twSmarts->setItem(row,1,item);
        ui->twSmarts->setItem(row,2,new QTableWidgetItem(ls.at(2)));
        row++;
    }
}

void ApcSubject::on_btnSaveSmart_clicked()
{
    if(!SmartAdaptes_del.isEmpty()){
        if(!account->getDbUtil()->saveSmartSSubAdapters(SmartAdaptes_del,true)){
            myHelper::ShowMessageBoxError(tr("在保存智能适配子目配置项时发生错误！"));
            return;
        }
        foreach(SmartSSubAdapteItem* ai, SmartAdaptes_del){
            QSet<QString> set = QSet<QString>::fromList(ai->keys.split(tr("，")));
            ai->fsub->removeSmartAdapteSSub(set,ai->ssub);
        }
        qDeleteAll(SmartAdaptes_del);
        SmartAdaptes_del.clear();
    }
    QList<SmartSSubAdapteItem*> items;
    for(int i = 0; i < ui->twSmarts->rowCount(); ++i){
        FirstSubject* fsub = ui->twSmarts->item(i,0)->data(Qt::UserRole).value<FirstSubject*>();
        SecondSubject* ssub = ui->twSmarts->item(i,1)->data(Qt::UserRole).value<SecondSubject*>();
        QString keys = ui->twSmarts->item(i,2)->text();
        SmartSSubAdapteItem* ai = SmartAdaptes.at(i);
        if(ai->id == 0){
            items<<ai;
            continue;
        }
        bool changed = false;
        if(ai->fsub != fsub){
            changed = true;
            ai->fsub = fsub;
        }
        if(ai->ssub != ssub){
            changed = true;
            ai->ssub = ssub;
        }
        if(ai->keys != keys){
            changed = true;
            ai->keys = keys;
        }
        if(changed)
            items<<ai;
    }
    if(items.isEmpty())
        return;
    if(!account->getDbUtil()->saveSmartSSubAdapters(items))
        myHelper::ShowMessageBoxError(tr("在保存智能适配子目配置项时发生错误！"));
    //内存对象更新
    QHash<FirstSubject*, QList<int> > ps;
    for(int i = 0; i < SmartAdaptes.count(); ++i){
        SmartSSubAdapteItem* as = SmartAdaptes.at(i);
        if(!ps.contains(as->fsub))
            ps[as->fsub]=QList<int>();
        ps[as->fsub]<<i;
    }
    QHashIterator<FirstSubject*, QList<int> > it(ps);
    while(it.hasNext()){
        it.next();
        FirstSubject* fsub = it.key();
        fsub->clearSmartAdaptes();
        foreach(int i, it.value()){
            SmartSSubAdapteItem* as = SmartAdaptes.at(i);
            QSet<QString> set = QSet<QString>::fromList(as->keys.split(tr("，")));
            fsub->addSmartAdapteSSub(set,as->ssub);
        }
    }
}

/**
 * @brief 添加包含型智能子目配置项
 */
void ApcSubject::on_actAddSmartItem_triggered()
{
    int subSysCode = ui->cmbSubSys->itemData(ui->cmbSubSys->currentIndex()).toInt();
    SubjectManager* sm = account->getSubjectManager(subSysCode);
    SmartSSubAdapteEditDlg dlg(sm,true,this);
    if(dlg.exec() == QDialog::Rejected)
        return;
    FirstSubject* fsub = dlg.getFirstSubject();
    SecondSubject* ssub = dlg.getSecondSubject();
    QString keys = dlg.getKeys();
    if(!fsub || !ssub || keys.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("配置项目不全！\n可能未选项一级科目、二级科目或关键字为空！"));
        return;
    }
    int row = ui->twSmarts->currentRow();
    if(row == -1)
        row = ui->twSmarts->rowCount();
    SmartSSubAdapteItem* ai = new SmartSSubAdapteItem;
    ai->id=0;
    ai->subSys=subSysCode;
    ai->fsub=fsub;
    ai->ssub=ssub;
    ai->keys = keys;
    SmartAdaptes.insert(row,ai);
    ui->twSmarts->insertRow(row);
    QTableWidgetItem* item = new QTableWidgetItem(fsub->getName());
    QVariant v; v.setValue<FirstSubject*>(fsub);
    item->setData(Qt::UserRole,v);
    ui->twSmarts->setItem(row,0,item);
    item = new QTableWidgetItem(ssub->getName());
    v.setValue<SecondSubject*>(ssub);
    item->setData(Qt::UserRole,v);
    ui->twSmarts->setItem(row,1,item);
    item = new QTableWidgetItem(keys);
    ui->twSmarts->setItem(row,2,item);
}

/**
 * @brief 编辑包含型智能子目配置项
 */
void ApcSubject::on_actEditSmartItem_triggered()
{
    int subSysCode = ui->cmbSubSys->itemData(ui->cmbSubSys->currentIndex()).toInt();
    SubjectManager* sm = account->getSubjectManager(subSysCode);
    int row = ui->twSmarts->currentRow();
    SmartSSubAdapteEditDlg dlg(sm,true,this);
    FirstSubject* fsub1 = ui->twSmarts->item(row,0)->data(Qt::UserRole).value<FirstSubject*>();
    dlg.setFirstSubject(fsub1);
    SecondSubject* ssub1 = ui->twSmarts->item(row,1)->data(Qt::UserRole).value<SecondSubject*>();
    dlg.setSecondSubject(ssub1);
    QString keys1 = ui->twSmarts->item(row,2)->text();
    dlg.setKeys(keys1);
    if(dlg.exec() == QDialog::Rejected)
        return;

    FirstSubject* fsub2 = dlg.getFirstSubject();
    if(fsub1 != fsub2){
        QVariant v; v.setValue<FirstSubject*>(fsub2);
        ui->twSmarts->item(row,0)->setData(Qt::UserRole,v);
        ui->twSmarts->item(row,0)->setText(fsub2->getName());
    }
    SecondSubject* ssub2 = dlg.getSecondSubject();
    if(ssub1 != ssub2){
        QVariant v; v.setValue<SecondSubject*>(ssub2);
        ui->twSmarts->item(row,1)->setData(Qt::UserRole,v);
        ui->twSmarts->item(row,1)->setText(ssub2->getName());
    }
    QString keys2 = dlg.getKeys();
    if(keys1 != keys2)
        ui->twSmarts->item(row,2)->setText(keys2);
}

/**
 * @brief 移除包含型智能子目配置项
 */
void ApcSubject::on_actRemoveSmartItem_triggered()
{
    int row = ui->twSmarts->currentRow();
    ui->twSmarts->removeRow(row);
    SmartAdaptes_del<<SmartAdaptes.takeAt(row);
}

/**
 * @brief 名称条目配置页面中模糊定位文本改变
 * @param arg1
 */
void ApcSubject::on_edtNI_NampInput_textEdited(const QString &arg1)
{
    if(!ni_fuzzyNameIndexes.isEmpty()){
        foreach(int row, ni_fuzzyNameIndexes){
            QListWidgetItem* item = ui->lwNI->item(row);
            if(item)
                item->setBackground(QBrush());
        }
    }
    ni_fuzzyNameIndexes.clear();
    if(ui->edtNI_NampInput->text().isEmpty())
        return;
    for(int i = 0; i < ui->lwNI->count(); ++i){
        QListWidgetItem* item = ui->lwNI->item(i);
        if(item->text().contains(ui->edtNI_NampInput->text())){
            item->setBackground(Qt::lightGray);
            ni_fuzzyNameIndexes<<i;
        }
    }
    if(!ni_fuzzyNameIndexes.isEmpty())
        ui->lwNI->scrollToItem(ui->lwNI->item(ni_fuzzyNameIndexes.first()),QAbstractItemView::PositionAtTop);
}

/**
 * @brief 科目配置页面，在二级科目列表上的模糊定位文本改变
 */
void ApcSubject::on_edtSSubNameInput_textEdited()
{
    if(!ssub_fuzzyNameIndexes.isEmpty()){
        foreach(int row, ssub_fuzzyNameIndexes){
            QListWidgetItem* item = ui->lwSSub->item(row);
            item->setBackground(QBrush());
        }
    }
    ssub_fuzzyNameIndexes.clear();
    if(ui->edtSSubNameInput->text().isEmpty())
        return;
    for(int i = 0; i < ui->lwSSub->count(); ++i){
        QListWidgetItem* item = ui->lwSSub->item(i);
        if(item->text().contains(ui->edtSSubNameInput->text())){
            item->setBackground(Qt::lightGray);
            ssub_fuzzyNameIndexes<<i;
        }
    }
    if(!ssub_fuzzyNameIndexes.isEmpty())
        ui->lwSSub->scrollToItem(ui->lwSSub->item(ssub_fuzzyNameIndexes.first()),QAbstractItemView::PositionAtTop);
}

/**
 * @brief 孤立别名列表上下文菜单请求
 * @param pos
 */
void ApcSubject::aliasListContextMenuRequest(const QPoint &pos)
{
    QListWidgetItem* item = ui->lwAlias->itemAt(pos);
    if(item){
        QMenu* m = new QMenu(this);
        m->addAction(ui->actDelAlias);
        m->popup(ui->lwAlias->mapToGlobal(pos));
    }
}

/**
 * @brief 移除选中的别名
 */
void ApcSubject::on_actDelAlias_triggered()
{
    if(subSysNames.isEmpty())
        return;
    NameItemAlias* alias = ui->lwAlias->currentItem()->data(Qt::UserRole).value<NameItemAlias*>();
    SubjectManager* sm = account->getSubjectManager(subSysNames.last()->code);
    if(alias->getParent()){
        QDialog dlg(this);
        QRadioButton rdoIso(tr("作为孤立别名"),&dlg);
        rdoIso.setChecked(true);
        QRadioButton rdoDel(tr("永久删除"),&dlg);
        QVBoxLayout lb;
        lb.addWidget(&rdoIso);
        lb.addWidget(&rdoDel);
        QPushButton btnOk(QIcon(":/images/btn_ok.png"),tr("确定"),&dlg);
        QPushButton btnCancel(QIcon(":/images/btn_close.png"),tr("取消"),&dlg);
        connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
        connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
        QHBoxLayout lh;lh.addWidget(&btnOk);lh.addWidget(&btnCancel);
        QVBoxLayout* lm = new QVBoxLayout;
        lm->addLayout(&lb);lm->addLayout(&lh);
        dlg.setLayout(lm);
        dlg.resize(300,200);
        if(dlg.exec() == QDialog::Rejected)
            return;
        SubjectNameItem* ni = ui->lwAliasNames->currentItem()->data(Qt::UserRole).value<SubjectNameItem*>();
        ni->removeAlias(alias);
        sm->saveNI(ni);
        if(rdoDel.isChecked())
            sm->removeNameAlias(alias);
    }
    else
        sm->removeNameAlias(alias);
    ui->lwAlias->takeItem(ui->lwAlias->currentRow());
}



/**
 * @brief 合并列表内的名称条目
 * @param preNI     保留的名称条目
 * @param nameItems 待合并的名称条目
 * @return
 */
bool ApcSubject::mergeNameItem(SubjectNameItem* preNI, QList<SubjectNameItem *> nameItems)
{
    if(!preNI || nameItems.isEmpty())
        return true;
    foreach(SubjectNameItem* ni, nameItems){
        if(curSubMgr->nameItemIsUsed(ni)){
            for(int i = 0; i < subSysNames.count(); ++i){
                SubSysNameItem* ssni = subSysNames.at(i);
                SubjectManager* sm = account->getSubjectManager(ssni->code);
                QList<SecondSubject*> subjects = sm->getSubSubjectUseNI(ni);
                if(subjects.isEmpty())
                    continue;
                foreach(SecondSubject* sub, subjects){
                    sub->setNameItem(preNI);
                    if(!sm->saveSS(sub))
                        return false;
                }
            }
        }
        curSubMgr->removeNameItem(ni,true);
        nameItems.removeOne(ni);
    }

    return true;
}

/**
 * @brief 合并列表内的二级科目（这些二级科目都属于同一个一级科目）
 * @param subjects
 * @return
 */
bool ApcSubject::mergeSndSubject(QList<SecondSubject *> subjects, int& preSubIndex, bool &isCancel)
{
    if(subjects.count() < 2)
        return true;
    //1、确定保留的二级科目，应提示用户选择，默认选择创建时间最早的
    SecondSubject* preserveSub = subjects.first();
    for(int i = 1; i < subjects.count(); ++i){
        if(subjects.at(i)->getCreateTime() < preserveSub->getCreateTime())
            preserveSub = subjects.at(i);
    }
    preSubIndex = subjects.indexOf(preserveSub);
    QDialog dlg(this);
    QListWidget lw(&dlg);
    QVariant v;
    SecondSubject* ssub;
    for(int i = 0; i < subjects.count(); ++i){
        ssub = subjects.at(i);
        QListWidgetItem* item = new QListWidgetItem(QString("%1(%2)").arg(ssub->getName()).arg(ssub->getId()));
        v.setValue<SecondSubject*>(ssub);
        item->setData(Qt::UserRole,v);
        lw.addItem(item);
    }
    lw.setCurrentRow(0);
    QLabel title(tr("请选择保留的二级科目："),&dlg);
    QPushButton btnOk(QIcon(":/images/btn_ok.png"),tr("确定"),&dlg);
    QPushButton btnCancel(QIcon(":/images/btn_close.png"),tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lh;
    lh.addWidget(&btnOk);
    lh.addWidget(&btnCancel);;
    QVBoxLayout* lv = new QVBoxLayout(&dlg);
    lv->addWidget(&title);
    lv->addWidget(&lw);
    lv->addLayout(&lh);
    dlg.setLayout(lv);
    if(dlg.exec() == QDialog::Rejected){
        isCancel = true;
        return true;
    }
    preserveSub = lw.currentItem()->data(Qt::UserRole).value<SecondSubject*>();
    preSubIndex = lw.currentRow();
    subjects.removeOne(preserveSub);
    SubjectManager* sm = preserveSub->getParent()->parent();
    QList<SecondSubject*> replaceSubs;
    //如果是新科目系统，则还要在科目衔接映射表中将所有待合并子目的id替换为保留科目
    //这样在跨年度读取期初余额时才能映射到正确的保留科目上
    if(sm->getSubSysCode() != DEFAULT_SUBSYS_CODE){
        replaceSubs = subjects;
    }
    QList<SecondSubject *> canRemovedSubs; //可以直接移除的科目（就是那些还没有被引用的科目）
    foreach(SecondSubject* sub, subjects){
        if(!sm->isUsedSSub(sub)){
            canRemovedSubs<<sub;
            subjects.removeOne(sub);
        }
    }
    //所有科目都未被引用，可以直接移除
    if(!canRemovedSubs.isEmpty()){
        FirstSubject* fsub = preserveSub->getParent();
        foreach(SecondSubject* sub, canRemovedSubs)
            fsub->removeChildSub(sub);
        if(!sm->saveFS(fsub))
            return false;
        if(!replaceSubs.isEmpty() && !account->getDbUtil()->replaceMapSidWithReserved(preserveSub,replaceSubs))
            return false;
    }
    if(subjects.isEmpty())
        return true;

    //2、如果某个待合并的子目被引用了，则必须调整分录表或余额表中被引用的子目id值
    //确定要处理的时间范围（从最开始被引用的年月到最后被引用的年月）
    int subSysCode = sm->getSubSysCode();
    int startYear=0,startMonth=0,endYear=0,endMonth=0;
    QList<AccountSuiteRecord*> suites = account->getAllSuiteRecords();
    if(suites.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("该账户没有任何帐套记录！"));
        return false;
    }
    for(int i = 0; i < suites.count(); ++i){        
        AccountSuiteRecord* asr = suites.at(i);
        if(asr->subSys != subSysCode)
            continue;
        if(startYear == 0){
            startYear = asr->year;
            startMonth = asr->startMonth;
        }
        endYear = asr->year;
        endMonth = asr->endMonth;
    }
    if(startYear == 0 || endYear == 0){
        myHelper::ShowMessageBoxWarning(tr("无法界定时间范围！"));
        return false;
    }
    int isInclued = (startYear == suites.first()->year); //是否需要包含起账时的期初余额范围
    if(!account->getDbUtil()->mergeSecondSubject(startYear,startMonth,endYear,endMonth,preserveSub,subjects,isInclued)){
        myHelper::ShowMessageBoxError(tr("合并过程发生错误，请查看日志！"));
        return false;
    }
    if(!replaceSubs.isEmpty() &&
            !account->getDbUtil()->replaceMapSidWithReserved(preserveSub,replaceSubs)){
        myHelper::ShowMessageBoxError(tr("合并过程中在科目衔接配置表中替换科目id时发生错误，请查看日志！"));
        return false;
    }

    //6、将多个重复科目合并为一个（科目管理器对象内部和数据库内部）
    FirstSubject* fsub = preserveSub->getParent();
    foreach(SecondSubject* sub, subjects){
        fsub->removeChildSub(sub);
    }
    if(!sm->saveFS(fsub))
        return false;
    return true;
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
        myHelper::ShowMessageBoxWarning(info);
        return false;
    }
    return true;
}



/////////////////////////////DirView////////////////////////////////////////////////
DirView::DirView(MoneyDirection dir, int type):QTableWidgetItem(type),dir(dir)
{
}

QVariant DirView::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole){
        switch(dir){
        case MDIR_J:
            return QObject::tr("借");
        case MDIR_D:
            return QObject::tr("贷");
        case MDIR_P:
            return QObject::tr("平");
        }
    }
    if(role == Qt::EditRole)
        return dir;
    return QTableWidgetItem::data(role);
}

void DirView::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        dir = (MoneyDirection)value.toInt();
    QTableWidgetItem::setData(role,value);
}


///////////////////////////DirEdit_new////////////////////////////////////////////////////////////
DirEdit_new::DirEdit_new(MoneyDirection dir, QWidget *parent):QComboBox(parent),dir(dir)
{
    addItem(tr("借"), MDIR_J);
    addItem(tr("贷"), MDIR_D);
    //addItem(tr("平"), MDIR_P);
}

void DirEdit_new::setDir(MoneyDirection dir)
{
    this->dir = dir;
    int idx = findData(dir);
    setCurrentIndex(idx);
}

MoneyDirection DirEdit_new::getDir()
{
    dir = (MoneyDirection)itemData(currentIndex(), Qt::UserRole).toInt();
    return dir;
}

void DirEdit_new::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();
    if((key == Qt::Key_Return) || (key == Qt::Key_Enter)){
        emit dataEditCompleted(BT_MTYPE,true);
        emit editNextItem(row,col);
        e->accept();
    }
    else
        e->ignore();
    QComboBox::keyPressEvent(e);
}


////////////////////////////BeginCfgItemDelegate///////////////////////////////////////////////
BeginCfgItemDelegate::BeginCfgItemDelegate(Account *account, bool readOnly, bool isFSub, QObject *parent)
    :QItemDelegate(parent),account(account),readOnly(readOnly),isFSub(isFSub)
{
    mts = account->getAllMoneys();

}

QWidget *BeginCfgItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(readOnly)
        return NULL;
    if(isFSub)
        return NULL;
    int col = index.column();
    int row = index.row();
    if(col == ApcData::CI_MONEY){
        MoneyTypeComboBox* cmb = new MoneyTypeComboBox(mts,parent);
        return cmb;
    }
    else if(col == ApcData::CI_DIR){
        DirEdit_new* cmb = new DirEdit_new(MDIR_J,parent);
        return cmb;
    }
    else{
        MoneyValueEdit* editor = NULL;
        //如果是币种是本币，则不能也无须输入本币形式的余额
        Money* mt = index.model()->data(index.model()->index(row,ApcData::CI_MONEY),Qt::EditRole).value<Money*>();
        if(col == ApcData::CI_MV && mt == account->getMasterMt())
            return editor;
        MoneyDirection dir = (MoneyDirection)index.model()->data(index.model()->index(row,ApcData::CI_DIR),Qt::EditRole).toInt();
        if(dir == MDIR_J)
            editor = new MoneyValueEdit(row,1,Double(),parent);
        else
            editor = new MoneyValueEdit(row,0,Double(),parent);
        return editor;
    }
}

void BeginCfgItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col = index.column();
    if(col == ApcData::CI_MONEY){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        Money* mt = index.model()->data(index, Qt::EditRole).value<Money*>();
        if(mt){
            int idx = cmb->findData(mt->code(), Qt::UserRole);
            cmb->setCurrentIndex(idx);
        }

    }
    else if(col == ApcData::CI_DIR){
        DirEdit_new* cmb = qobject_cast<DirEdit_new*>(editor);
        MoneyDirection dir = (MoneyDirection)index.model()->data(index, Qt::EditRole).toInt();
        cmb->setDir(dir);
    }
    else{
        MoneyValueEdit *edit = qobject_cast<MoneyValueEdit*>(editor);
        if (edit) {
            Double v;
            if(col == ApcData::CI_PV){
                v = index.model()->data(index, Qt::EditRole).value<Double>();
                edit->setValue(v);
            }
            else{
                v = index.model()->data(index, Qt::EditRole).value<Double>();
                edit->setValue(v);
            }
        }
    }
}

void BeginCfgItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    int col = index.column();
    if(col == ApcData::CI_MONEY){
        MoneyTypeComboBox* cmb = qobject_cast<MoneyTypeComboBox*>(editor);
        if(cmb){
            int newMt = cmb->itemData(cmb->currentIndex(), Qt::UserRole).toInt();
            if(newMt != 0){
                int oldMt = 0;
                Money* mo = model->data(index).value<Money*>();
                if(mo)
                   oldMt = mo->code();
                QVariant v;
                v.setValue<Money*>(mts.value(newMt));
                model->setData(index, v);
                //emit moneyChanged(oldMt,newMt);//直接触发信号，编译通不过
            }
        }
    }
    else if(col == ApcData::CI_DIR){
        DirEdit_new* cmb = qobject_cast<DirEdit_new*>(editor);
        if(cmb){
            MoneyDirection dir = cmb->getDir();
            model->setData(index, dir);
        }
    }
    else{
        MoneyValueEdit* edit = qobject_cast<MoneyValueEdit*>(editor);
        if(edit){
            Double v = edit->getValue();
            QVariant va; va.setValue(v);
            model->setData(index, va);
        }
    }
}

void BeginCfgItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}

void BeginCfgItemDelegate::commitAndCloseEditor(int colIndex, bool isMove)
{
    int i = 0;
}


///////////////////////////ApcData///////////////////////////////////////////
ApcData::ApcData(Account *account, bool isCfg, QByteArray* state, QWidget *parent) :
    QWidget(parent),ui(new Ui::ApcData),account(account),extraCfg(isCfg)
{
    ui->setupUi(this);
    iniTag = false;
    b_fsubId=0; b_ssubId=0;e_fsubId=0;e_ssubId=0;e_y=0;e_m=0;
    curFSub=NULL;curSSub=NULL;
    readOnly = account->isReadOnly();
    boldFont = ui->ssubs->font();
    boldFont.setBold(true);
    if(!isCfg)
        init(state);
    connect(ui->nameInput,SIGNAL(textEdited(QString)),this,SLOT(nameChanged(QString)));
    connect(ui->rdoPrefixe,SIGNAL(toggled(bool)),this,SLOT(searchModeChanged(bool)));
}

ApcData::~ApcData()
{
    delete ui;
    smg = NULL;
}

void ApcData::init(QByteArray *state)
{
    if(iniTag)
        return;
    if(state && !state->isEmpty()){
        QBuffer bf(state);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        in>>e_y;
        in>>e_m;
        in>>e_fsubId;
        in>>e_ssubId;
        in>>b_fsubId;
        in>>b_ssubId;
    }

    AccountSuiteRecord* asr;
    if(extraCfg){  //期初余额配置模式
        asr = account->getStartSuiteRecord();
        if(!asr && !readOnly){
            myHelper::ShowMessageBoxWarning(tr("该账户还没有设置任何帐套，无法设置期初值"));
            return;
        }
        if(asr->startMonth == 1){
            y = asr->year - 1;
            m = 12;
        }
        else{
            y = asr->year;
            m = asr->startMonth - 1;
        }

        ui->year->setText(QString::number(y));
        ui->month->setValue(m);
        readOnly |= !curUser->haveRight(allRights.value(Right::Account_Config_SetPeriodBegin));
        if(!readOnly){
            //确定期初余额是否可编辑（这里如果账户的第一个月份还未结账，则视为可编辑）
            PzsState state;
            if(!account->getDbUtil()->getPzsState(asr->year,asr->startMonth,state)){
                myHelper::ShowMessageBoxError(tr("在读取首期凭证集状态时发生错误"));
                return;
            }
            readOnly = readOnly || (state == Ps_Jzed);
            if(!readOnly)
                ui->edtERate->addAction(ui->actSetRate,QLineEdit::TrailingPosition);
        }
        ui->month->setReadOnly(true);        
    }
    else{   //余额显示模式
        asr = account->getCurSuiteRecord();
        if(!asr){
            myHelper::ShowMessageBoxWarning(tr("没有任何账套！"));
            return;
        }
        y = asr->year; m = asr->startMonth;
        if(e_y == y && m != e_m)
            m = e_m;
        readOnly = true;
        ui->month->setMaximum(asr->endMonth);
        ui->month->setMinimum(asr->startMonth);
        ui->month->setValue(m);
        ui->year->setText(QString::number(y));
        ui->add->setVisible(false);
        ui->save->setVisible(false);
        connect(ui->month,SIGNAL(valueChanged(int)),this,SLOT(monthChanged(int)));
    }

    mts = account->getAllMoneys();
    mtSorts = mts.keys();
    if(mtSorts.count() > 1){
        qSort(mtSorts.begin(),mtSorts.end());
        //这里应加入将本币代码移到首位的代码，但考虑到实际使用时，本币为人民币且其代码最小
    }
    viewRates();
    smg = account->getSubjectManager(asr->subSys);
    int fsubId = extraCfg?b_fsubId:e_fsubId;
    int ssubId = extraCfg?b_ssubId:e_ssubId;
    curFSub = smg->getFstSubject(fsubId);
    curSSub = smg->getSndSubject(ssubId);
    FSubItrator* it = smg->getFstSubItrator();
    QListWidgetItem* item;
    QVariant v; int index = -1,fsubIndex = 0;
    while(it->hasNext()){
        it->next();
        if(!it->value()->isEnabled())
            continue;
        index++;
        item = new QListWidgetItem(it->value()->getName());
        v.setValue<FirstSubject*>(it->value());
        item->setData(Qt::UserRole,v);
        ui->fsubs->addItem(item);
        if(curFSub && curFSub == it->value())
            fsubIndex = index;
    }

    delegate = new BeginCfgItemDelegate(account,readOnly,false,this);
    delegate_fsub = new BeginCfgItemDelegate(account,readOnly,true,this);
    ui->etables->setItemDelegate(delegate);
    connect(ui->fsubs,SIGNAL(currentRowChanged(int)),this,SLOT(curFSubChanged(int)));
    ui->ftables->setColumnCount(4);
    ui->ftables->setItemDelegate(delegate_fsub);
    ui->ftables->setColumnWidth(CI_MONEY, ui->etables->columnWidth(CI_MONEY));
    ui->ftables->setColumnWidth(CI_DIR, ui->etables->columnWidth(CI_DIR));
    ui->ftables->setColumnWidth(CI_PV, ui->etables->columnWidth(CI_PV));
    connect(ui->etables->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),this,SLOT(adjustColWidth(int,int,int)));
    if(fsubIndex >= 0){
        ui->fsubs->setCurrentRow(fsubIndex);
        if(ssubId){
            SecondSubject* ssub = smg->getSndSubject(ssubId);
            if(ssub){
                for(int r=0; r < ui->ssubs->count(); ++r){
                    if(ssub == ui->ssubs->item(r)->data(Qt::UserRole).value<SecondSubject*>()){
                        ui->ssubs->setCurrentRow(r);
                        break;
                    }
                }
            }

        }
    }
    iniTag = true;
}

/**
 * @brief ApcData::setMonth
 *  在余额显示模式下，设置要显示的余额的月份
 * @param month
 */
void ApcData::setYM(int year, int month)
{
    if(extraCfg)
        return;
    AccountSuiteRecord* asr = account->getSuiteRecord(y);
    if(!asr)
        return;
    if(m < asr->startMonth || m > asr->endMonth)
        return;
    y = year;
    m = month;
    if(ui->fsubs->currentRow() != -1)
        curFSubChanged(ui->fsubs->currentRow());
}

QByteArray *ApcData::getProperState()
{
    if(!extraCfg){
        e_y = y; e_m = m;
        e_fsubId = curFSub?curFSub->getId():0;
        e_ssubId = curSSub?curSSub->getId():0;
    }
    else{
        b_fsubId = curFSub?curFSub->getId():0;
        b_ssubId = curSSub?curSSub->getId():0;
    }
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    out<<e_y;
    out<<e_m;
    out<<e_fsubId;
    out<<e_ssubId;
    out<<b_fsubId;
    out<<b_ssubId;
    bf.close();
    return info;
}

void ApcData::curFSubChanged(int index)
{
    //读取当前一级科目下所有二级科目的余额
    if(index < 0 || index >= ui->fsubs->count()){
        curFSub = NULL;
        return;
    }
    if(ui->save->isEnabled()){
        if(QMessageBox::warning(this,tr("警告信息"),tr("一级科目“%1”的期初余额已被修改，要保存吗？").arg(curFSub->getName()),QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            on_save_clicked();
        else
            ui->save->setEnabled(false);
    }
    pvs.clear(); mvs.clear(); dirs.clear();
    curFSub = ui->fsubs->currentItem()->data(Qt::UserRole).value<FirstSubject*>();
    if(!account->getDbUtil()->readExtraForAllSSubInFSub(y,m,curFSub,pvs,dirs,mvs)){
        myHelper::ShowMessageBoxError(tr("在读取一级科目“%1”的期初余额是发生错误（%2年%3月）！")
                              .arg(curFSub->getName()).arg(y).arg(m));
        return;
    }

    disconnect(ui->ssubs,SIGNAL(currentRowChanged(int)),this,SLOT(curSSubChanged(int)));
    ui->ssubs->clear();
    QVariant v;
    QListWidgetItem* item;
    foreach(SecondSubject* ssub, curFSub->getChildSubs(SORTMODE_NAME)){
        v.setValue<SecondSubject*>(ssub);
        //item = new QListWidgetItem(ssub->getName());
        item = new QListWidgetItem(QString("%1(%2)").arg(ssub->getName()).arg(ssub->getId()) );
        item->setData(Qt::UserRole,v);
        if(exist(ssub->getId()))
            item->setFont(boldFont);
        ui->ssubs->addItem(item);
    }    
    connect(ui->ssubs,SIGNAL(currentRowChanged(int)),this,SLOT(curSSubChanged(int)));
    if(ui->ssubs->count()>0)
        ui->ssubs->setCurrentRow(0);
    else
        curSSubChanged(-1);
    viewCollectData();
}

void ApcData::curSSubChanged(int index)
{
    ui->etables->setRowCount(0);
    if(index < 0 || index >= ui->ssubs->count()){
        curSSub = NULL;
        ui->add->setEnabled(false);
        return;
    }
    curSSub = ui->ssubs->currentItem()->data(Qt::UserRole).value<SecondSubject*>();
    //显示当前二级科目的期初余额
    watchDataChanged(false);
    int mmt = account->getMasterMt()->code();
    int row = -1;
    int key = curSSub->getId() * 10 + mmt;
    if(pvs.contains(key)){
        row++;
        ui->etables->insertRow(row);
        BAMoneyTypeItem_new* mtItem = new BAMoneyTypeItem_new(mts.value(mmt));
        ui->etables->setItem(row,CI_MONEY,mtItem);
        DirView* dirItem = new DirView(dirs.value(key,MDIR_P));
        ui->etables->setItem(row,CI_DIR,dirItem);
        BAMoneyValueItem_new* vItem = new BAMoneyValueItem_new(dirs.value(key),pvs.value(key));
        ui->etables->setItem(row,CI_PV,vItem);
        vItem = new BAMoneyValueItem_new(MDIR_P,0.0);
        ui->etables->setItem(row,CI_MV,vItem);
    }
    if(mtSorts.count()>1 && curFSub->isUseForeignMoney()){
        for(int i = 1; i < mtSorts.count(); ++i){
            int mt = mtSorts.at(i);
            key = curSSub->getId() * 10 + mt;
            if(!pvs.contains(key))
                continue;
            row++;
            ui->etables->insertRow(row);
            BAMoneyTypeItem_new* mtItem = new BAMoneyTypeItem_new(mts.value(mt));
            ui->etables->setItem(row,CI_MONEY,mtItem);
            DirView* dirItem = new DirView(dirs.value(key,MDIR_P));
            ui->etables->setItem(row,CI_DIR,dirItem);
            BAMoneyValueItem_new* vItem = new BAMoneyValueItem_new(dirs.value(key),pvs.value(key));
            ui->etables->setItem(row,CI_PV,vItem);
            vItem = new BAMoneyValueItem_new(dirs.value(mt),mvs.value(key));
            ui->etables->setItem(row,CI_MV,vItem);
        }
    }
    enAddBtn();
    watchDataChanged();

}

void ApcData::curMtChanged(int index)
{
    int mt = ui->cmbRate->itemData(index).toInt();
    ui->edtSRate->setText(srates.value(mt).toString());
    ui->edtERate->setText(erates.value(mt).toString());
}

void ApcData::adjustColWidth(int col, int oldSize, int newSize)
{
    ui->ftables->setColumnWidth(col,newSize);
}

void ApcData::dataChanged(QTableWidgetItem *item)
{
    if(!curSSub)
        return;
    bool dirty = false;
    int col = item->column();
    int key;
    if(col == CI_MONEY){ //移除余额表中当前二级科目相关项
        foreach(int mt, mtSorts){
            key = curSSub->getId() * 10 + mt;
            pvs.remove(key);
            dirs.remove(key);
            if(curFSub->isUseForeignMoney())
                mvs.remove(key);
        }
        //重建当前二级科目的余额值表项（从表中读取）
        for(int i = 0; i < ui->etables->rowCount(); ++i){
            int mt = ui->etables->item(i,CI_MONEY)->data(Qt::EditRole).value<Money*>()->code();
            MoneyDirection dir = (MoneyDirection)ui->etables->item(i,CI_DIR)->data(Qt::EditRole).toInt();
            Double pv = ui->etables->item(i,CI_PV)->data(Qt::EditRole).value<Double>();
            Double mv = ui->etables->item(i,CI_MV)->data(Qt::EditRole).value<Double>();
            key = curSSub->getId() * 10 + mt;
            pvs[key] = pv;
            dirs[key] = dir;
            if(curFSub->isUseForeignMoney())
                mvs[key] = mv;
        }
        dirty = true;
    }
    else{
        int row = item->row();
        int mt = ui->etables->item(row,CI_MONEY)->data(Qt::EditRole).value<Money*>()->code();
        key = curSSub->getId() * 10 + mt;        
        if(col == CI_DIR){
            MoneyDirection oldDir = dirs.value(key);
            MoneyDirection newDir = (MoneyDirection)ui->etables->item(row,CI_DIR)->data(Qt::EditRole).toInt();
            if(oldDir != newDir){
                dirs[key] = newDir;
                dirty = true;
            }
        }
        else if(col == CI_PV){
            Double oldValue = pvs.value(key);
            Double newValue = ui->etables->item(row,CI_PV)->data(Qt::EditRole).value<Double>();
            if(newValue != oldValue){
                QVariant v;
                pvs[key] = newValue;
                //如果是外币金额发生改变，则自动根据汇率调整本币金额
                if(mt != account->getMasterMt()->code()){
                    Double mv = newValue * erates.value(mt);
                    v.setValue<Double>(mv);
                    mvs[key] = mv;
                    ui->etables->item(row,CI_MV)->setData(Qt::EditRole,v);
                }
                if(newValue == 0.0){
                    dirs[key] = MDIR_P;
                    ui->etables->item(row,CI_DIR)->setData(Qt::EditRole,MDIR_P);
                }
                dirty = true;
            }
        }
        else{
            Double oldValue = mvs.value(key);
            Double newValue = ui->etables->item(row,CI_MV)->data(Qt::EditRole).value<Double>();
            if(newValue != oldValue){
                mvs[key] = newValue;
                dirty = true;
            }
        }
    }
    viewCollectData();
    ui->save->setEnabled(dirty);
}

/**
 * @brief ApcData::monthChanged
 *  在余额显示模式下，当月份改变时显示指定月份的余额
 * @param date
 */
void ApcData::monthChanged(int m)
{
    this->m = m;
    viewRates();
    if(ui->fsubs->currentRow() != -1)
        curFSubChanged(ui->fsubs->currentRow());
}

/**
 * @brief 输入名称快速模糊定位
 * @param arg1
 */
void ApcData::nameChanged(const QString &name)
{
    fuzzySearch(ui->rdoPrefixe->isChecked());
}

/**
 * @brief 名称模糊搜索模式改变
 * @param isPre
 */
void ApcData::searchModeChanged(bool isPre)
{
    fuzzySearch(isPre);
}

/**
 * @brief ApcData::windowShallClosed
 *  容器窗口将关闭，在这里执行一些未保存的更改
 */
void ApcData::windowShallClosed()
{
    if(ui->save->isEnabled())
        on_save_clicked();
    QByteArray* state = getProperState();
    account->getDbUtil()->saveSubWinInfo(SUBWIN_LOOKUPSUBEXTRA,state);
}

void ApcData::on_add_clicked()
{
    if(readOnly)
        return;    
    int row = ui->etables->rowCount();
    if(row > 0 && row == mtSorts.count())  //所有的币种都已输入
        return;
    //如果是银行存款，则只能添加其子目所属的币种余额条目
    Money* mt = NULL;
    if(curSSub && curFSub->parent()->isBankSndSub(curSSub)){
        if(row == 1)
            return;
        mt = curFSub->parent()->getSubMatchMt(curSSub);
    }
    if(row == 0)
        ui->ssubs->currentItem()->setFont(boldFont);
    watchDataChanged(false);
    ui->etables->insertRow(row);
    if(!mt)
        mt = mts.value(mtSorts.at(row));
    BAMoneyTypeItem_new* mtItem = new BAMoneyTypeItem_new(mt);
    ui->etables->setItem(row,CI_MONEY,mtItem);
    MoneyDirection dir = curFSub->getJdDir()?MDIR_J:MDIR_D;
    DirView* dirItem = new DirView(dir);
    ui->etables->setItem(row,CI_DIR,dirItem);
    BAMoneyValueItem_new* vItem = new BAMoneyValueItem_new(dir,Double());
    ui->etables->setItem(row,CI_PV,vItem);
    BAMoneyValueItem_new* vItem2 = new BAMoneyValueItem_new(dir,Double());
    ui->etables->setItem(row,CI_MV,vItem2);
    ui->etables->editItem(vItem);
    int key = curSSub->getId() * 10 + mt->code();
    pvs[key] = 0.0;
    dirs[key] = dir;
    mvs[key] = 0.0;
    watchDataChanged();
    enAddBtn();
}

void ApcData::on_save_clicked()
{
    if(readOnly)
        return;
    if(!curFSub)
        return;
    if(!account->getDbUtil()->saveExtraForAllSSubInFSub(y,m,curFSub,pvs_f,mvs_f,dir_f,pvs,mvs,dirs))
        myHelper::ShowMessageBoxError(tr("在保存科目“%1”的期初余额时出错！").arg(curSSub->getName()));
    ui->save->setEnabled(false);
}

/**
 * @brief 更改起账汇率
 */
void ApcData::on_actSetRate_triggered()
{
    QDialog dlg(this);
    QTableWidget tw(&dlg);
    tw.setRowCount(erates.count());
    tw.setColumnCount(2);
    QStringList titles;
    titles<<tr("币种")<<tr("汇率");
    tw.setHorizontalHeaderLabels(titles);
    QTableWidgetItem* item;
    QHashIterator<int,Double> it(erates);
    int i = 0;
    while(it.hasNext()){
        it.next();
        item = new QTableWidgetItem(mts.value(it.key())->name());
        item->setData(Qt::UserRole,it.key());
        tw.setItem(i,0,item);
        item = new QTableWidgetItem(erates.value(it.key()).toString());
        tw.setItem(i,1,item);
        i++;
    }
    QPushButton btnOk(QIcon(":/images/btn_ok.png"),tr("确定"),&dlg),btnCancel(QIcon(":/images/btn_close.png"),tr("取消"),&dlg);
    QHBoxLayout lb;
    lb.addWidget(&btnOk);
    lb.addWidget(&btnCancel);
    QVBoxLayout* l = new QVBoxLayout;
    l->addWidget(&tw);
    l->addLayout(&lb);
    dlg.setLayout(l);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    if(dlg.exec() == QDialog::Accepted){
        bool changed = false;
        for(int i = 0; i < tw.rowCount(); ++i){
            int mtCode = tw.item(i,0)->data(Qt::UserRole).toInt();
            Double rate = Double(tw.item(i,1)->text().toDouble(),4);
            if(rate != erates.value(mtCode)){
                erates[mtCode] = rate;
                changed = true;
            }
        }
        if(changed){
            int yy,mm;
            getNextMonth(y,m,yy,mm);
            if(!account->setRates(yy,mm,erates)){
                myHelper::ShowMessageBoxError(tr("保存起账汇率时出错！"));
                return;
            }
            ui->cmbRate->clear();
            QHashIterator<int,Double> it(erates);
            while(it.hasNext()){
                it.next();
                ui->cmbRate->addItem(mts.value(it.key())->name(),it.key());
            }
            if(ui->cmbRate->count() > 0)
                curMtChanged(0);
            //如果当前显示的一级科目是使用外币的科目，则要相应更新外币余额折算成本币后的值
            //这有待考虑，因为与外币对应的本币值不是严格按汇率换算的，它可能由用户自己输入的值为准
            //而且，如果要自动换算，则换算后的本币值，还必须保存到余额表中（否者，显示值与保存值不等同）
            //if(curFSub && curFSub->isUseForeignMoney()){

            //}
        }
    }
}




/**
 * @brief 是否存在未设置汇率的外币
 * 如果是起账数据设置界面，则要求期末汇率必须完备，如果是余额显示，则期初和期末都必须完备
 * @return true：汇率设置有遗漏
 */
bool ApcData::isRateNull()
{
    QList<Money*> mts = account->getWaiMt();
    if(mts.isEmpty())
        return false;
    bool isLack = false;
    //首先检测期末汇率设置是否必须完备
    foreach(Money* m, mts){
        if(erates.contains(m->code()))
            continue;
        erates[m->code()] = 0;
        isLack = true;
    }
    if(isLack || extraCfg)
        return isLack;
    else{   //如果是余额显示，则还要检测期初汇率是否设置完备
        foreach(Money* m, mts){
            if(srates.contains(m->code()))
                continue;
            srates[m->code()] = 0;
            isLack = true;
        }
    }
    return isLack;
}



/**
 * @brief ApcData::viewRates
 *  读取并显示指定年月的汇率
 * @return
 */
bool ApcData::viewRates()
{
    disconnect(ui->cmbRate,SIGNAL(currentIndexChanged(int)),this,SLOT(curMtChanged(int)));
    srates.clear();
    erates.clear();
    ui->cmbRate->clear();
    ui->edtSRate->clear();
    ui->edtERate->clear();
    if(!account->getRates(y,m,srates)){
        myHelper::ShowMessageBoxError(tr("在读取汇率时出错（%1年%2月）").arg(y).arg(m));
        return false;
    }
    int yy,mm;
    getNextMonth(y,m,yy,mm);
    if(!account->getRates(yy,mm,erates)){
        myHelper::ShowMessageBoxError(tr("在读取汇率时出错（%1年%2月）").arg(yy).arg(mm));
        return false;
    }
    if(isRateNull() && !readOnly){
        myHelper::ShowMessageBoxWarning(tr("在配置期初数据前，如果要涉及到外币，则必须首先设置起账汇率！"));
        bool ok;
        double rate;        
        int yy,mm;
        getNextMonth(y,m,yy,mm);
        QHashIterator<int,Double> it(erates);
        while(it.hasNext()){
            it.next();
            if(it.value() != 0)
                continue;
            rate = QInputDialog::getDouble(this,tr("汇率输入"),
                                           tr("请输入起账%1（%2年%3月）汇率：")
                                           .arg(mts.value(it.key())->name())
                                           .arg(yy).arg(mm),1,0,100,4,&ok);
            if(!ok){
                readOnly = true;
                return false;
            }
            erates[it.key()] = Double(rate,4);
        }
        if(!account->setRates(yy,mm,erates)){
            myHelper::ShowMessageBoxError(tr("在保存起账汇率时出错（%1年%2月）").arg(yy).arg(mm));
            readOnly = true;
            return false;
        }
        readOnly = false;
    }    
    else if(erates.isEmpty() && readOnly)
        myHelper::ShowMessageBoxWarning(tr("起账汇率未设置！"));
    QHashIterator<int,Double> it(erates);
    while(it.hasNext()){
        it.next();
        ui->cmbRate->addItem(mts.value(it.key())->name(),it.key());
    }
    connect(ui->cmbRate,SIGNAL(currentIndexChanged(int)),this,SLOT(curMtChanged(int)));
    curMtChanged(0);
    return true;
}

/**
 * @brief ApcData::collect
 *  汇总当前一级科目下的二级科目的期初余额
 *  各子目的期初余额按币种汇总并最终确定主目的期初余额及其方向
 */
void ApcData::collect()
{
    pvs_f.clear();mvs_f.clear();dir_f.clear();
    QHashIterator<int,Double>* it = new QHashIterator<int,Double>(pvs);
    int mt;
    Double v;
    while(it->hasNext()){
        it->next();
        mt = it->key() % 10;
        if(dirs.value(it->key()) == MDIR_D){
            v = it->value();
            v.changeSign();
        }
        else
            v = it->value();
        pvs_f[mt] += v;
    }
    it = new QHashIterator<int,Double>(pvs_f);
    while(it->hasNext()){
        it->next();
        if(it->value() > 0.0)
            dir_f[it->key()] = MDIR_J;
        else if(it->value() < 0.0){
            dir_f[it->key()] = MDIR_D;
            pvs_f[it->key()].changeSign();
        }
        else
            continue;
    }
    it = new QHashIterator<int,Double>(mvs);
    while(it->hasNext()){
        it->next();
        mt = it->key() % 10;
        if(dirs.value(it->key()) == MDIR_D){
            v = it->value();
            v.changeSign();
        }
        else
            v = it->value();
        mvs_f[mt] += v;
    }
    it = new QHashIterator<int,Double>(mvs_f);
    while(it->hasNext()){
        it->next();
        if((it->value() < 0.0) && (dir_f.value(it->key()) == MDIR_D)){
            mvs_f[it->key()].changeSign();
        }
    }
}

/**
 * @brief ApcData::exist
 *  指定二级科目是否存在于期初值表中
 * @param sid
 */
bool ApcData::exist(int sid)
{
    QHashIterator<int,Double> it(pvs);
    int id;
    while(it.hasNext()){
        it.next();
        if(sid == it.key() / 10)
            return true;
    }
    return false;
}

/**
 * @brief ApcData::viewCollectData
 *  显示汇总后的主目余额
 */
void ApcData::viewCollectData()
{
    if(!curFSub){
        ui->ftables->setRowCount(0);
        return;
    }
    ui->ftables->setRowCount(0);
    if(curFSub->isUseForeignMoney())
        ui->ftables->setRowCount(2);
    else
        ui->ftables->setRowCount(1);

    collect();

    int mmt = account->getMasterMt()->code();
    QTableWidgetItem* ti = new BAMoneyTypeItem_new(account->getMasterMt());
    ui->ftables->setItem(0,CI_MONEY,ti);
    DirView* di = new DirView(dir_f.value(mmt,MDIR_P));
    ui->ftables->setItem(0,CI_DIR,di);
    BAMoneyValueItem_new *vi = new BAMoneyValueItem_new(dir_f.value(mmt,MDIR_P),pvs_f.value(mmt));
    ui->ftables->setItem(0,CI_PV,vi);
    vi = new BAMoneyValueItem_new(dir_f.value(mmt,MDIR_P),mvs_f.value(mmt));
    ui->ftables->setItem(0,CI_MV,vi);
    if(curFSub->isUseForeignMoney() && mtSorts.count() > 1){
        int row = 0;
        for(int i = 1; i < mtSorts.count(); ++i){
            row++;
            ti = new BAMoneyTypeItem_new(mts.value(mtSorts.at(i)));
            ui->ftables->setItem(row,CI_MONEY,ti);
            int mt = mtSorts.at(i);
            ui->ftables->setItem(row,CI_DIR,new DirView(dir_f.value(mt,MDIR_P)));
            ui->ftables->setItem(row,CI_PV,new BAMoneyValueItem_new(dir_f.value(mt,MDIR_P),pvs_f.value(mt)));
            ui->ftables->setItem(row,CI_MV,new BAMoneyValueItem_new(dir_f.value(mt,MDIR_P),mvs_f.value(mt)));
        }
    }
}

void ApcData::watchDataChanged(bool en)
{
    if(en)
        connect(ui->etables,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(dataChanged(QTableWidgetItem*)));
    else
        disconnect(ui->etables,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(dataChanged(QTableWidgetItem*)));
}

/**
 * @brief 决定添加按钮的启用性
 */
void ApcData::enAddBtn()
{
   if(readOnly || !curFSub || !curSSub){
        ui->add->setEnabled(false);
        return;
    }
    int eitem = 1;
    if(curFSub->isUseForeignMoney())
        eitem = mtSorts.count();
    ui->add->setEnabled(eitem > ui->etables->rowCount());
}

void ApcData::getNextMonth(int y, int m, int &yy, int &mm)
{
    if(m == 12){
        yy = y+1;
        mm = 1;
    }
    else{
        yy = y;
        mm = m+1;
    }
}

void ApcData::fuzzySearch(bool isPre)
{
    if(!fuzzyNameIndexes.isEmpty()){
        for(int i = 0; i < fuzzyNameIndexes.count(); ++i){
            QListWidgetItem* item = ui->ssubs->item(fuzzyNameIndexes.at(i));
            item->setBackground(QBrush());
        }
    }
    fuzzyNameIndexes.clear();
    if(ui->nameInput->text().isEmpty())
        return;
    if(isPre){
        for(int i = 0; i < ui->ssubs->count(); ++i){
            QListWidgetItem* item = ui->ssubs->item(i);
            if(item->text().startsWith(ui->nameInput->text())){
                item->setBackground(QBrush(Qt::lightGray));
                fuzzyNameIndexes<<i;
            }
        }
    }
    else{
        for(int i = 0; i < ui->ssubs->count(); ++i){
            QListWidgetItem* item = ui->ssubs->item(i);
            if(item->text().contains(ui->nameInput->text())){
                item->setBackground(QBrush(Qt::lightGray));
                fuzzyNameIndexes<<i;
            }
        }
    }
    if(!fuzzyNameIndexes.isEmpty())
        ui->ssubs->scrollToItem(ui->ssubs->item(fuzzyNameIndexes.first()),QAbstractItemView::PositionAtTop);

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
AccountPropertyConfig::AccountPropertyConfig(Account* account, QByteArray* cinfo, QWidget *parent)
    : QDialog(parent),account(account)
{
    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMaximumWidth(86);
    contentsWidget->setSpacing(12);

    pagesWidget = new QStackedWidget;
    ApcBase* baseCfg = new ApcBase(account,this);
    connect(this,SIGNAL(windowShallClosed()),baseCfg,SLOT(windowShallClosed()));
    pagesWidget->addWidget(baseCfg);
    ApcSuite* suiteCfg = new ApcSuite(account,this);
    connect(suiteCfg,SIGNAL(suiteUpdated()),this,SLOT(suiteUpdated()));
    connect(this,SIGNAL(windowShallClosed()),suiteCfg,SLOT(windowShallClosed()));
    pagesWidget->addWidget(suiteCfg);
    ApcBank* bankCfg = new ApcBank(account,this);
    connect(this,SIGNAL(windowShallClosed()),bankCfg,SLOT(windowShallClosed()));
    pagesWidget->addWidget(bankCfg);
    ApcSubject* subjectCfg = new ApcSubject(account,this);
    connect(this,SIGNAL(windowShallClosed()),subjectCfg,SLOT(windowShallClosed()));
    pagesWidget->addWidget(subjectCfg);
    QByteArray* state = new QByteArray;
    account->getDbUtil()->getSubWinInfo(SUBWIN_LOOKUPSUBEXTRA,state);
    ApcData* dataCfg = new ApcData(account,true,state,this);
    delete state;
    connect(this,SIGNAL(windowShallClosed()),dataCfg,SLOT(windowShallClosed()));
    pagesWidget->addWidget(dataCfg);
    pagesWidget->addWidget(new ApcReport(this));
    pagesWidget->addWidget(new ApcLog(this));

    QPushButton *closeButton = new QPushButton(QIcon(":/images/btn_close.png"),tr("关闭"));

    createIcons();
    contentsWidget->setCurrentRow(0);

    //connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(closeButton,SIGNAL(clicked()),this,SLOT(closeAllPage()));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout,1);
    //mainLayout->addStretch(1);
    //mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("账户属性配置"));
    setCommonState(cinfo);
}

AccountPropertyConfig::~AccountPropertyConfig()
{

}

QByteArray *AccountPropertyConfig::getCommonState()
{
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 i8 = contentsWidget->currentRow();
    out<<i8;
    bf.close();
    return info;
}

void AccountPropertyConfig::setCommonState(QByteArray *state)
{
    if(!state || state->isEmpty()){
        contentsWidget->setCurrentRow(0);
        return;
    }
    QBuffer bf(state);
    QDataStream in(&bf);
    bf.open(QIODevice::ReadOnly);
    qint8 i8; in>>i8;
    contentsWidget->setCurrentRow(i8);
    bf.close();
}

//void AccountPropertyConfig::closeEvent(QCloseEvent *event)
//{
//    emit windowShallClosed();
//    event->accept();
////    if (maybeSave()) {
////         writeSettings();
////         event->accept();
////     } else {
////         event->ignore();
////     }
//}

void AccountPropertyConfig::pageChanged(int index)
{
    pagesWidget->setCurrentIndex(index);
    if(index == APC_BASE){
        ApcBase* w = static_cast<ApcBase*>(pagesWidget->currentWidget());
        w->init();
    }
    else if(index == APC_BANK){
        ApcBank* w = static_cast<ApcBank*>(pagesWidget->currentWidget());
        w->init();
    }
    else if(index == APC_SUBJECT){
        ApcSubject* w = static_cast<ApcSubject*>(pagesWidget->currentWidget());
        w->init();
    }
    else if(index == APC_SUITE){
        ApcSuite* w = static_cast<ApcSuite*>(pagesWidget->currentWidget());
        w->init();
    }
    else if(index == APC_DATA){
        ApcData* w = static_cast<ApcData*>(pagesWidget->currentWidget());
        QByteArray* state = new QByteArray;
        account->getDbUtil()->getSubWinInfo(SUBWIN_LOOKUPSUBEXTRA,state);
        w->init(state);
    }

}

void AccountPropertyConfig::closeAllPage()
{
    emit windowShallClosed();
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();

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
    item->setIcon(QIcon(":/images/accProperty/basedata.png"));
    item->setText(tr("期初数据"));
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





