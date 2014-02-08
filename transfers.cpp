#include <QPushButton>
#include <QFileDialog>
//#include <QHash>

#include "global.h"
#include "config.h"
#include "tables.h"
#include "transfers.h"
#include "commdatastruct.h"
#include "utils.h"

#include "ui_transferoutdialog.h"


Machine::Machine(int id, MachineType type, int mid, bool isLocal, QString name, QString desc)
    :id(id),type(type),mid(mid),isLocal(isLocal),sname(name),desc(desc)
{
}

Machine::Machine(Machine &other)
{
    id = other.id;
    type = other.type;
    mid = other.mid;
    isLocal = other.isLocal;
    sname = other.sname;
    desc = other.desc;
}

bool Machine::operator ==(const Machine &other) const
{
    if(mid != other.mid || type != other.type)
        return false;
    if(sname != other.sname || desc != other.desc)
        return false;
    return true;
}

bool Machine::operator !=(const Machine &other) const
{
    return !(*this == other);
}

bool byMacMID(Machine *mac1, Machine *mac2)
{
    return mac1->getMID() < mac2->getMID();
}

////////////////////////////////////TransferRecordManager////////////////////////////

TransferRecordManager::TransferRecordManager(QString filename)
{
    trRec = NULL;
    db = QSqlDatabase::addDatabase("QSQLITE",TRANSFER_MANAGER_CONNSTR);
    connected = false;
    isExistTemMacTable = false;
    macUpdated = false;
    setFilename(filename);    
}

/**
 * @brief TransferRecordManager::setFilename
 * @param filename  账户文件名（路径名）
 * @return
 */
bool TransferRecordManager::setFilename(QString filename)
{
    if(filename.isEmpty())
        return false;
    if(trRec){
        delete trRec;
        trRec = NULL;
    }
    if(db.isOpen()){
        connected = false;
        db.close();
    }
    db.setDatabaseName(filename);
    if(!db.open()){
        QMessageBox::critical(0,tr("出错信息"),tr("无法建立与选定账户文件的数据库连接！"));
        connected = false;
    }
    connected = true;
    QSqlQuery q(db);
    QString s = QString("select count() from sqlite_master where type='table' and name='tem_%1'").arg(tbl_machines);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.first();
    int c = q.value(0).toInt();
    isExistTemMacTable = (c == 1);
    getMergeMacs();
    trRec = getLastTransRec();
}

/**
 * @brief TransferRecordManager::appendTransferRecord
 *  在转移记录表的最后添加或保存一条转移记录
 * @param rec
 * @return
 */
bool TransferRecordManager::saveTransferRecord(AccontTranferInfo *rec)
{
    if(!connected)
        return false;
    QSqlQuery q(db);
    QString s;
    bool newRec = rec->id == UNID;
    if(newRec)
        s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,%8,%9,'%10','%11')")
                .arg(tbl_transfer).arg(fld_trans_smid).arg(fld_trans_dmid)
                .arg(fld_trans_state).arg(fld_trans_outTime).arg(fld_trans_inTime)
                .arg(rec->m_out->getMID()).arg(rec->m_in->getMID()).arg(rec->tState)
                .arg(rec->t_out.isValid()?rec->t_out.toString(Qt::ISODate):"")
                .arg(rec->t_in.isValid()?rec->t_in.toString(Qt::ISODate):"");
    else
        s = QString("update %1 set %2=%3,%4=%5,%6=%7,%8='%9',%10='%11' where id=%12")
                .arg(tbl_transfer).arg(fld_trans_smid).arg(rec->m_out->getMID())
                .arg(fld_trans_dmid).arg(rec->m_in->getMID()).arg(fld_trans_state)
                .arg(rec->tState).arg(fld_trans_outTime)
                .arg(rec->t_out.isValid()?rec->t_out.toString(Qt::ISODate):"")
                .arg(fld_trans_inTime).arg(rec->t_in.isValid()?rec->t_in.toString(Qt::ISODate):"")
                .arg(rec->id);

    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(newRec){
        s = "select last_insert_rowid()";
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.first();
        rec->id = q.value(0).toInt();
    }
    if(!rec->desc_in.isEmpty() || !rec->desc_out.isEmpty()){
        if(newRec)
            s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')")
                    .arg(tbl_transferDesc).arg(fld_transDesc_tid).arg(fld_transDesc_out)
                    .arg(fld_transDesc_in).arg(rec->id).arg(rec->desc_out).arg(rec->desc_in);
        else
            s = QString("update %1 set %2='%3',%4='%5' where %6=%7").arg(tbl_transferDesc)
                    .arg(fld_transDesc_in).arg(rec->desc_in).arg(fld_transDesc_out).arg(rec->desc_out)
                    .arg(fld_transDesc_tid).arg(rec->id);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    return true;
}

/**
 * @brief TransferRecordManager::attechMachineInfo
 *  让转出的账户文件捎带执行转出操作的主机上的所有主机信息，将在账户文件内创建临时保存主机的表
 *  此方法只在执行转出操作时调用
 * @return
 */
bool TransferRecordManager::attechMachineInfo(QList<Machine*> macs)
{
    if(!connected)
        return false;
    if(trRec && trRec->tState != ATS_TRANSINDES) //只有在账户已转入目标主机的情况下，才可以执行转出操作
        return true;
    QSqlQuery q(db);
    QString s;
    if(!isExistTemMacTable){
        s = QString("create table tem_%1(id integer primary key, %2 integer, %3 integer, %4 text, %5 text)")
                .arg(tbl_machines).arg(fld_mac_mid).arg(fld_mac_type).arg(fld_mac_sname).arg(fld_mac_desc);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            QMessageBox::critical(0,tr("出错信息"),tr("创建临时主机表失败！"));
            return false;
        }
    }
    else{
        s = QString("delete from tem_%1").arg(tbl_machines);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    foreach(Machine* mac, macs){
        s = QString("insert into tem_%1(%2,%3,%4,%5) values(%6,%7,'%8','%9')")
                .arg(tbl_machines).arg(fld_mac_mid).arg(fld_mac_type).arg(fld_mac_sname).arg(fld_mac_desc)
                .arg(mac->getMID()).arg(mac->getType()).arg(mac->name()).arg(mac->description());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            QMessageBox::critical(0,tr("出错信息"),tr("往临时主机表插入主机信息时失败！"));
            return false;
        }
    }
    return true;
}

/**
 * @brief TransferRecordManager::updateMachines
 *  如果被转入的账户捎带了主机信息，则和本地主机上保存的主机列表比较，进行必要的更新、新增和删除操作
 *  用一个表格显示比较的结果，如果是要更新的主机，则第一列有一个检取框供用户选择是否更新，如果没有检取框，则表示
 *  此主机信息没有变化，或此主机在本机中存在但在来源主机中不存在（用灰色背景）
 *  此方法只在执行转入操作时调用
 * @return
 */
bool TransferRecordManager::updateMachines()
{
    if(!connected)
        return false;
    QSqlQuery q(db);
    if(trRec && trRec->tState != ATS_TRANSOUTED)
        return true;
    if(!isExistTemMacTable) //只有在捎带了主机信息的情况下，才执行此任务
        return true;
    AppConfig* conf = AppConfig::getInstance();

    if(upMacs.isEmpty() && newMacs.isEmpty())
        return true;

    QDialog dlg;
    QLabel* lblExplain = new QLabel(QObject::tr("说明：蓝色--新增，红色--不相同，黑色--相同，灰底--本机存在但未捎带"));
    QFont font = lblExplain->font();
    font.setBold(true);
    lblExplain->setFont(font);
    tab = new QTableWidget(&dlg);
    tab->setColumnCount(5);
    int rowCount = newMacs.count()+upMacs.count()+notMacs.count()+notExistMacs.count();
    tab->setRowCount(rowCount);
    tab->horizontalHeader()->setStretchLastSection(true);
    tab->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QStringList ht;
    ht<<tr("是否更新")<<tr("主机ID")<<tr("主机类型")<<tr("主机名")<<tr("主机描述");
    tab->setHorizontalHeaderLabels(ht);
    QHash<MachineType,QString> macTypes = conf->getMachineTypes();
    QPushButton* btnExec = new QPushButton(tr("更新本地主机"),&dlg);
    QPushButton* btnClose = new QPushButton(tr("关闭"),&dlg);
    QObject::connect(btnExec,SIGNAL(clicked()),this,SLOT(execUpdate()));
    QObject::connect(btnClose,SIGNAL(clicked()),&dlg,SLOT(close()));
    int row = 0;
    Machine* mac;
    QTableWidgetItem* item;
    QBrush color_blue(QColor(Qt::blue));    //新增主机
    QBrush color_red(QColor(Qt::red));      //更新主机
    QBrush color_grey(QColor(Qt::gray));    //
    QBrush color_green(QColor(Qt::green));  //未更新主机
    //装载新增主机
    for(int i = 0; i < newMacs.count(); ++i,++row){
        mac = newMacs.at(i);
        tab->setCellWidget(row,0,new QCheckBox());
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        item->setForeground(color_blue);
        tab->setItem(row,1,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        item->setForeground(color_blue);
        tab->setItem(row,2,item);
        item = new QTableWidgetItem(mac->name());
        item->setForeground(color_blue);
        tab->setItem(row,3,item);
        item = new QTableWidgetItem(mac->description());
        item->setForeground(color_blue);
        tab->setItem(row,4,item);
    }
    //装载信息更新的主机
    for(int i = 0; i < upMacs.count(); ++i,++row){
        mac = upMacs.at(i);
        tab->setCellWidget(row,0,new QCheckBox());
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        item->setForeground(color_red);
        tab->setItem(row,1,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        item->setForeground(color_red);
        tab->setItem(row,2,item);
        item = new QTableWidgetItem(mac->name());
        item->setForeground(color_red);
        tab->setItem(row,3,item);
        item = new QTableWidgetItem(mac->description());
        item->setForeground(color_red);
        tab->setItem(row,4,item);
    }
    //装载未变更的主机
    for(int i = 0; i < notMacs.count(); ++i,++row){
        mac = notMacs.at(i);
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        item->setForeground(color_green);
        tab->setItem(row,1,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        item->setForeground(color_green);
        tab->setItem(row,2,item);
        item = new QTableWidgetItem(mac->name());
        item->setForeground(color_green);
        tab->setItem(row,3,item);
        item = new QTableWidgetItem(mac->description());
        item->setForeground(color_green);
        tab->setItem(row,4,item);
    }
    //装载本机存在但未捎带的主机（可能这些主机已经被认为不实际存在了，可以清除）
    for(int i = 0; i < notExistMacs.count(); ++i,++row){
        mac = notExistMacs.at(i);
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        item->setBackground(color_grey);
        tab->setItem(row,1,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        item->setBackground(color_grey);
        tab->setItem(row,2,item);
        item = new QTableWidgetItem(mac->name());
        item->setBackground(color_grey);
        tab->setItem(row,3,item);
        item = new QTableWidgetItem(mac->description());
        item->setBackground(color_grey);
        tab->setItem(row,4,item);
    }
    QHBoxLayout* lb = new QHBoxLayout;
    lb->addStretch();
    lb->addWidget(btnExec);
    lb->addWidget(btnClose);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(lblExplain);
    lm->addWidget(tab);
    lm->addLayout(lb);
    dlg.setLayout(lm);
    dlg.resize(800,400);
    dlg.exec();
    return true;
}

/**
 * @brief TransferRecordManager::clearTemMachineTable
 *  删除捎带主机信息的临时表
 * @return
 */
bool TransferRecordManager::clearTemMachineTable()
{
    if(!isExistTemMacTable)
        return true;
    QSqlQuery q(db);
    QString s = QString("drop table tem_%1").arg(tbl_machines);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief TransferRecordManager::getAccountInfo
 *  获取账户的基本信息
 * @param accId
 * @param accName
 * @param accLName
 * @return
 */
bool TransferRecordManager::getAccountInfo(QString &accCode, QString &accName, QString &accLName)
{
    if(!connected)
        return false;
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2=%3 or %2=%4 or %2=%5").arg(tbl_accInfo)
            .arg(fld_acci_code).arg(Account::ACODE).arg(Account::SNAME).arg(Account::LNAME);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    Account::InfoField fld;
    while(q.next()){
        fld = (Account::InfoField)q.value(ACCINFO_CODE).toInt();
        switch(fld){
        case Account::ACODE:
            accCode = q.value(ACCINFO_VALUE ).toString();
            break;
        case Account::SNAME:
            accName = q.value(ACCINFO_VALUE).toString();
            break;
        case Account::LNAME:
            accLName = q.value(ACCINFO_VALUE).toString();
        }
    }
    return true;
}

/**
 * @brief TransferRecordManager::getLastTransRec
 *  返回账户最后一个转移记录
 * @return
 */
AccontTranferInfo *TransferRecordManager::getLastTransRec()
{
    if(!connected)
        return NULL;
    if(trRec)
        return trRec;
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_transfer);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return NULL;
    }
    if(!q.last())
        return NULL;
    AppConfig* conf = AppConfig::getInstance();
    trRec = new AccontTranferInfo;
    trRec->tState = (AccountTransferState)q.value(TRANS_STATE).toInt();
    int smid = q.value(TRANS_SMID).toInt();
    int dmid = q.value(TRANS_DMID).toInt();
    //检测源主机和目标主机是否在本机上存在，如果不存在，则检查是否捎带了主机信息，并更新主机信息，如果还是不存在，则视为异常
    if(!mergeMacs.contains(smid) || !mergeMacs.contains(dmid)){
            QMessageBox::warning(0,tr("出错信息"),tr("转移记录中包含未知的源或目标主机，这将使账户转移无法继续！"));
            return false;
    }
    trRec->id = q.value(0).toInt();
    trRec->m_in = mergeMacs.value(dmid);
    trRec->m_out = mergeMacs.value(smid);
    trRec->t_in = q.value(TRANS_INTIME).toDateTime();
    trRec->t_out = q.value(TRANS_OUTTIME).toDateTime();
    s = QString("select * from %1 where %2=%3").arg(tbl_transferDesc)
            .arg(fld_transDesc_tid).arg(trRec->id);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        delete trRec;
        return NULL;
    }
    if(q.first()){
        trRec->desc_in = q.value(TRANSDESC_IN).toString();
        trRec->desc_out = q.value(TRANSDESC_OUT).toString();
    }
    return trRec;
}

/**
 * @brief TransferRecordManager::getMergeMacs
 *  返回合并的主机信息（本地拥有的和引入文件捎带的）
 *  同时进行本地主机信息与捎带主机信息的比较，并初始化新增、更新主机列表成员
 * @return
 */
QHash<int,Machine*> TransferRecordManager::getMergeMacs()
{
    mergeMacs.clear();localMacs.clear();newMacs.clear();upMacs.clear();
    notMacs.clear();notExistMacs.clear();
    localMacs = AppConfig::getInstance()->getAllMachines();
    QList<int> localMids = localMacs.keys();
    if(!_getTakeMacs() || takeMacs.isEmpty())
        return localMacs;
    mergeMacs = localMacs;
    foreach(Machine* mac, takeMacs){
        int mid = mac->getMID();
        if(!localMacs.contains(mid)){
            mergeMacs[mid] = mac;
            newMacs<<mac;
        }
        else {
            if(*mac != *localMacs.value(mac->getMID())){
                mergeMacs[mid] = mac;    //主机信息以捎带的为准
                upMacs<<mac;
            }
            else
                notMacs<<localMacs.value(mid);
            localMids.removeOne(mid);
        }
    }
    if(!localMids.isEmpty()){
        foreach(int mid, localMids)
            notExistMacs<<localMacs.value(mid);
    }
    return mergeMacs;
}

/**
 * @brief TransferRecordManager::execUpdate
 *  执行更新主机操作
 */
void TransferRecordManager::execUpdate()
{
    if(!_inspectSelUpMacs()){
        QMessageBox::warning(0,tr("警告信息"),tr("来源主机或本机信息已更新，必须更新到本地，否者造成主机无法识别或无法显示最新的主机信息！"));
        return;
    }
    AppConfig* conf = AppConfig::getInstance();
    if(!conf->saveMachines(selUpMacs))
        QMessageBox::critical(0,tr("出错信息"),tr("在保存更新的主机信息时出错"));
    macUpdated = true;
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if(btn){
        QDialog* dlg = qobject_cast<QDialog*>(btn->parent());
        if(dlg)
            dlg->accept();
    }
}

/**
 * @brief TransferRecordManager::getTakeMacs
 *  读取捎带的主机信息
 */
bool TransferRecordManager::_getTakeMacs()
{
    if(!connected)
        return false;
    if(!takeMacs.isEmpty()){
        foreach(Machine* mac, takeMacs){
            if(mac->getId() == UNID)
                delete mac;
        }
        takeMacs.clear();
    }
    if(!isExistTemMacTable)
        return true;
    QSqlQuery q(db);
    QString s = QString("select * from tem_%1").arg(tbl_machines);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        int mid = q.value(MACS_MID).toInt();
        MachineType type = (MachineType)q.value(MACS_TYPE).toInt();
        QString name = q.value(MACS_NAME-1).toString();
        QString desc = q.value(MACS_DESC-1).toString();
        takeMacs[mid] = new Machine(UNID,type,mid,false,name,desc);
    }
    return true;
}

/**
 * @brief TransferRecordManager::_inspectSelUpMacs
 *  检查选择的更新主机（如果当前引入的账户涉及到的主机（来源机和目标机）发生了变更，则必须至少选择它们）
 * @return true：用户的选择满足最少更新条件，反之不满足
 */
bool TransferRecordManager::_inspectSelUpMacs()
{
    QList<Machine*> macs;
    macs = newMacs + upMacs;
    selUpMacs.clear();
    int newRows = newMacs.count();
    int rows = newRows + upMacs.count();
    for(int i = 0; i < rows; ++i){
        QCheckBox* btn = qobject_cast<QCheckBox*>(tab->cellWidget(i,0));
        Machine* mac = macs.at(i);
        if(btn && btn->isChecked()){
            if(i >= newRows){   //对于更新主机要将更新信息转移到本地表示该主机的对象上
                Machine* takeMac = mac;
                mac = localMacs.value(takeMac->getMID());
                mac->setType(takeMac->getType());
                mac->setName(takeMac->name());
                mac->setDescription(takeMac->description());
            }
            selUpMacs<<mac;
        }
    }
    if(!localMacs.contains(trRec->m_in->getMID()) && !selUpMacs.contains(trRec->m_in))
        return false;
    if(!localMacs.contains(trRec->m_out->getMID()) && !selUpMacs.contains(trRec->m_out))
        return false;
    else
        return true;
}

///////////////////////////TransferOutDialog//////////////////////////////////////////

TransferOutDialog::TransferOutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::TransferOutDialog)
{
    ui->setupUi(this);
    ui->tView->setColumnWidth(0,50);
    ui->tView->setColumnWidth(1,100);
    ui->tView->setColumnWidth(2,150);
    ui->tView->addAction(ui->actDel);
    ui->tView->addAction(ui->actReset);
    connect(ui->tView,SIGNAL(itemSelectionChanged()),this,SLOT(enAct()));

    conf = AppConfig::getInstance();
    tranStates = conf->getAccTranStates();
    lm = conf->getLocalMachine();
    if(lm)
        ui->edtLocalMac->setText(tr("%1（%2）").arg(lm->name()).arg(lm->getMID()));
    else
        ui->edtLocalMac->setText(tr("本机未知，这将影响转移操作！"));
    accCacheItems = conf->getAllCachedAccounts();
    if(accCacheItems.isEmpty()){
        QMessageBox::warning(this,tr("警告信息"),tr("没有任何本地账户！"));
        ui->btnOk->setEnabled(false);
        return;
    }
    localMacs = conf->getAllMachines().values();
    qSort(localMacs.begin(),localMacs.end(),byMacMID);
    if(localMacs.isEmpty()){
        QMessageBox::critical(this,tr("出错信息"),tr("无法读取主机列表"));
        ui->btnOk->setEnabled(false);
        return;
    }
    foreach(AccountCacheItem* acc, accCacheItems)
        ui->cmbAccount->addItem(acc->accName);
    foreach(Machine* mac, localMacs)
        ui->cmbMachines->addItem(mac->name());
    ui->edtDir->setText(QDir::homePath());
    connect(ui->cmbAccount,SIGNAL(currentIndexChanged(int)),this,SLOT(selectAccountChanged(int)));
    selectAccountChanged(ui->cmbAccount->currentIndex());
}

TransferOutDialog::~TransferOutDialog()
{
    accCacheItems.clear();
    delete ui;
}


QString TransferOutDialog::getAccountFileName()
{
    AccountCacheItem* acc = accCacheItems.at(ui->cmbAccount->currentIndex());
    if(!acc)
        return "";
    else
        return acc->fileName;
}


Machine* TransferOutDialog::getDestiMachine()
{
    int index = ui->cmbMachines->currentIndex();
    if(index == -1)
        return NULL;
    else
        return localMacs.at(index);
}

QString TransferOutDialog::getDescription()
{
    return ui->edtDesc->toPlainText();
}

bool TransferOutDialog::isTakeMachineInfo()
{
    return ui->chkTake->isChecked();
}

QDir TransferOutDialog::getDirection()
{
    return dir;
}

/**
 * @brief TransferOutDialog::selectAccountChanged
 *  用户选择的待转出的账户改变
 * @param index
 */
void TransferOutDialog::selectAccountChanged(int index)
{
    AccountCacheItem* acc = accCacheItems.at(index);
    if(acc){
        ui->edtAccID->setText(acc->code);
        ui->edtAccLName->setText(acc->accLName);
        ui->edtAccFileName->setText(acc->fileName);
        ui->edtState->setText(tranStates.value(acc->tState));
    }
}

/**
 * @brief TransferOutDialog::enAct
 *  控制重置、删除主机的菜单项的可用性
 */
void TransferOutDialog::enAct()
{
    ui->actReset->setEnabled(takeMacs.count() != localMacs.count());
    int row = ui->tView->currentRow();
    bool r = (row > -1) && (row < takeMacs.count());
    ui->actDel->setEnabled(r);
}


/**
 * @brief TransferOutDialog::on_cmbMachines_currentIndexChanged
 *  选择一个目标主机
 * @param index
 */
void TransferOutDialog::on_cmbMachines_currentIndexChanged(int index)
{
    Machine* mac = localMacs.at(index);
    if(mac){
        ui->edtMID->setText(QString::number(mac->getMID()));
        if(mac->getType() == MT_COMPUTER)
            ui->edtMType->setText(tr("电脑"));
        else
            ui->edtMType->setText(tr("云账户"));
        ui->edtMDesc->setText(mac->description());
    }

}

/**
 * @brief TransferOutDialog::on_btnBrowser_clicked
 *  选择一个保存转出账户文件的目录
 */
void TransferOutDialog::on_btnBrowser_clicked()
{
    QFileDialog dlg;
    dlg.setFileMode(QFileDialog::Directory);
    //dlg.setFilter("Sqlite files(*.dat)");
    dlg.setNameFilter("Sqlite files(*.dat)");
    dlg.setDirectory(".");
    if(dlg.exec() == QDialog::Rejected)
        return;
    QStringList files = dlg.selectedFiles();
    if(files.empty())
        return;
    ui->edtDir->setText(files.first());
    dir.setPath(files.first());
}

/**
 * @brief TransferOutDialog::on_chkTake_toggled
 *  选择或不选捎带主机信息
 * @param checked
 */
void TransferOutDialog::on_chkTake_toggled(bool checked)
{
    if(checked)
        on_actReset_triggered();
    else{
        ui->tView->clearContents();
        ui->tView->setRowCount(0);
    }
}

/**
 * @brief TransferOutDialog::on_btnOk_clicked
 *  执行转出操作
 */
void TransferOutDialog::on_btnOk_clicked()
{
    DontTranReason reason;
    if(!canTransferOut(reason))
        return;
    //1、在转出账户中添加一条转出记录
    QString sn = DatabasePath + getAccountFileName();
    TransferRecordManager trMgr(sn);
    AppConfig* conf = AppConfig::getInstance();
    AccontTranferInfo rec;
    rec.id = UNID;
    rec.desc_out = ui->edtDesc->toPlainText();
    rec.m_in = getDestiMachine();
    rec.m_out = lm;
    rec.tState = ATS_TRANSOUTED;
    rec.t_out = QDateTime::currentDateTime();
    if(!trMgr.saveTransferRecord(&rec)){
        QMessageBox::critical(0,QObject::tr("出错信息"),QObject::tr("在添加转移记录时出错！"));
        return;
    }
    if(isTakeMachineInfo())
        trMgr.attechMachineInfo(takeMacs);

    //2、修改本地的账户缓存记录
    AccountCacheItem* acc = accCacheItems.at(ui->cmbAccount->currentIndex());
    acc->outTime = QDateTime::currentDateTime();
    acc->mac = rec.m_in;
    acc->tState = ATS_TRANSOUTED;
    if(!conf->saveAccountCacheItem(acc)){
        QMessageBox::critical(0,tr("出错信息"),tr("在保存本地账户缓存项目时出错，无法执行转出操作！"));
        return;
    }

    //3、拷贝账户文件到指定位置
    QString dn = dir.absolutePath() + QDir::separator() + getAccountFileName();
    if(QFile::exists(dn)){
        if(QMessageBox::warning(this,tr("警告信息"),tr("选定的目录下存在同名文件，要覆盖吗？\n如果不覆盖，则老的文件将被添加后缀“bak”"),
                                QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            QFile::remove(dn);
        else
            QFile::rename(dn,dn+".bak");
    }
    if(!QFile::copy(sn,dn)){
        QMessageBox::critical(0,tr("出错信息"),tr("账户文件拷贝失败！"));
        return;
    }
    accept();
}

/**
 * @brief TransferOutDialog::on_actReset_triggered
 *  装载捎带的主机信息
 */
void TransferOutDialog::on_actReset_triggered()
{
    takeMacs = localMacs;
    ui->tView->setRowCount(localMacs.count());
    QTableWidgetItem* item;
    int i = -1;
    foreach(Machine* mac, localMacs){
        i++;
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        ui->tView->setItem(i,0,item);
        if(mac->getType() == MT_COMPUTER)
            item = new QTableWidgetItem(tr("物理电脑"));
        else
            item = new QTableWidgetItem(tr("云账户"));
        ui->tView->setItem(i,1,item);
        item = new QTableWidgetItem(mac->name());
        ui->tView->setItem(i,2,item);
        item = new QTableWidgetItem(mac->description());
        ui->tView->setItem(i,3,item);
        if(mac->isLocalMachine()){
            QBrush back(QColor("grey"));
            for(int j = 0; j < 4; ++j)
                ui->tView->item(i,j)->setBackground(back);
        }
    }
}

/**
 * @brief TransferOutDialog::on_actDel_triggered
 *  移除选定的主机，不捎带
 */
void TransferOutDialog::on_actDel_triggered()
{
    int row = ui->tView->currentRow();
    if(row < 0 || row >= takeMacs.count())
        return;
    ui->tView->removeRow(row);
    takeMacs.removeAt(row);
}

/**
 * @brief TransferOutDialog::canTransferOut
 *  基于当前用户选择的条件判断是否可以执行转出操作
 *  1、用户必须选择一个有效的账户、有效的目的机、填写转出理由
 *  2、选择的账户必须可以转出（即账户处于已转入到目的机的状态）
 * @return
 */
bool TransferOutDialog::canTransferOut(DontTranReason &reason)
{
    reason = DTR_OK;
    AccountCacheItem* acc = accCacheItems.at(ui->cmbAccount->currentIndex());
    if(!acc)
        reason = DTR_NULLACC;
    else if(acc->tState != ATS_TRANSINDES)
        reason = DTR_NOTSTATE;
    else if(!getDestiMachine())
        reason = DTR_NULLMACHINE;
    else if(ui->edtDesc->toPlainText().isEmpty())
        reason = DTR_NULLDESC;
    else if(!lm)
        reason = DTR_NOTLOCAL;
    if(reason != DTR_OK){
        QString info;
        switch(reason){
        case DTR_NULLACC:
            info = tr("未选择转出账户");
            break;
        case DTR_NULLDESC:
            info = tr("未填写转出说明");
            break;
        case DTR_NULLMACHINE:
            info = tr("未选择转出的目标主机");
            break;
        case DTR_NOTSTATE:
            info = tr("账户的转入目的主机不是本机");
            break;
        case DTR_NOTLOCAL:
            info = tr("本机未定义");
            break;
        }
        QMessageBox::warning(this,tr("警告信息"),tr("不能执行转出操作，原因：\n%1！").arg(info));
        return false;
    }
    else
        return true;
}



/////////////////////////////TransferInDialog//////////////////////////////
TransferInDialog::TransferInDialog(QWidget *parent) : QDialog(parent), ui(new Ui::TransferInDialog)
{
    ui->setupUi(this);
    trMgr = NULL;
    ui->twMachines->setColumnWidth(0,50);
    ui->twMachines->setColumnWidth(1,100);
    ui->twMachines->setColumnWidth(2,50);
    ui->twMachines->setColumnWidth(3,150);
    refreshMachines();
}

TransferInDialog::~TransferInDialog()
{
    delete ui;
}

/**
 * @brief TransferInDialog::refreshMachines
 *  刷新显示本地主机信息
 */
void TransferInDialog::refreshMachines()
{
    ui->twMachines->setRowCount(0);
    QTableWidgetItem* item;
    int row = 0;
    AppConfig* conf = AppConfig::getInstance();
    QHash<MachineType,QString> macTypes = conf->getMachineTypes();
    QBrush lmBack(QColor("grey"));
    foreach(Machine* mac, conf->getAllMachines()){
        ui->twMachines->insertRow(row);
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        ui->twMachines->setItem(row,0,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        ui->twMachines->setItem(row,1,item);
        item = new QTableWidgetItem(mac->isLocalMachine()?tr("是"):tr("否"));
        ui->twMachines->setItem(row,2,item);
        item = new QTableWidgetItem(mac->name());
        ui->twMachines->setItem(row,3,item);
        item = new QTableWidgetItem(mac->description());
        ui->twMachines->setItem(row,4,item);        
        if(mac->isLocalMachine()){
            for(int i = 0; i < 5; ++i)
                ui->twMachines->item(row,i)->setBackground(lmBack);
        }
        row++;
    }
}


/**
 * @brief TransferInDialog::on_btnOk_clicked
 *  执行转入操作
 */
void TransferInDialog::on_btnOk_clicked()
{    
    //1、拷贝账户文件到工作目录（最好在拷贝前备份账户文件）
    QDir dir(DatabasePath);
    QString fname = QFileInfo(fileName).fileName();
    BackupUtil bu;
    bool isBackup = false;
    if(dir.exists(fname)){
        if(!bu.backup(fname,BackupUtil::BR_TRANSFERIN)){
            QMessageBox::warning(this,tr("警告"),tr("在转入账户前执行备份操作出错！"));
            return;
        }
        isBackup = true;
        dir.remove(fname);
    }
    QString desFName = DatabasePath + fname;
    QString error;
    if(!QFile::copy(fileName,desFName)){
        QMessageBox::critical(this,tr("出错信息"),tr("文件拷贝操作出错，请手工将账户文件拷贝到工作目录"));
        if(isBackup && !bu.restore(fname,BackupUtil::BR_TRANSFERIN,error))
            QMessageBox::warning(this,tr("警告"),tr("无法恢复被覆盖的账户文件！"));
        return;
    }

    //2、修改账户的转移记录
    //（1）状态（转入目标机或转入其他机）
    //（2）设置用户输入的转入描述信息及其转入时间
    //（3）转出时间、转出源主机、转入目的主机、转出描述等内容不变    

    trMgr->setFilename(desFName);
    AccontTranferInfo* tranRec = trMgr->getLastTransRec();
    AppConfig* conf = AppConfig::getInstance();
    if(tranRec->m_in == conf->getLocalMachine())
        tranRec->tState = ATS_TRANSINDES;
    else
        tranRec->tState = ATS_TRANSINOTHER;
    tranRec->desc_in = ui->edtInDesc->toPlainText();
    tranRec->t_in = QDateTime::currentDateTime();
    if(!trMgr->saveTransferRecord(tranRec)){
        QMessageBox::critical(this,tr("出错信息"),tr("保存转移记录时出错！"));
        if(isBackup && !bu.restore(/*fname,BackupUtil::BR_TRANSFERIN,*/error))
            QMessageBox::warning(this,tr("警告"),tr("无法恢复被覆盖的账户文件！"));
        reject();
    }

    //3、修改本地账户缓存，对应于转入账户的条目（也有可能转入的是新账户，即在本地缓存中还没有）
    QString code,accName,accLName;
    trMgr->getAccountInfo(code,accName,accLName);
    AccountCacheItem* ci = conf->getAccountCacheItem(code);
    if(!ci){
        ci = new AccountCacheItem;
        ci->id = UNID;
        ci->code = code;
        ci->accName = accName;
        ci->accLName = accLName;
        ci->fileName = QFileInfo(fileName).fileName();
        ci->lastOpened = false;
    }
    ci->mac = tranRec->m_out;
    ci->inTime = QDateTime::currentDateTime();
    ci->outTime = tranRec->t_out;
    if(tranRec->m_in == conf->getLocalMachine())
        ci->tState = ATS_TRANSINDES;
    else
        ci->tState = ATS_TRANSINOTHER;
    if(!conf->saveAccountCacheItem(ci)){
        QMessageBox::critical(this,tr("出错信息"),tr("在保存缓存账户条目时出错！"));
        if(isBackup && !bu.restore(fname,BackupUtil::BR_TRANSFERIN,error))
            QMessageBox::warning(this,tr("警告"),tr("无法恢复被覆盖的账户文件！"));
        return;
    }
    if(ui->btnClear->isChecked())
        trMgr->clearTemMachineTable();
    accept();
}

/**
 * @brief TransferInDialog::on_btnSelectFile_clicked
 *  提供用户选择要转入的账户文件，并读取用户选择的账户的信息（包括账户基本信息和转移信息）
 */
void TransferInDialog::on_btnSelectFile_clicked()
{
    ui->btnOk->setEnabled(false);
    QFileDialog dlg;
    dlg.setFileMode(QFileDialog::ExistingFile);
    //dlg.setFilter("Sqlite files(*.dat)");
    dlg.setNameFilter("Sqlite files(*.dat)");
    dlg.setDirectory(".");
    if(dlg.exec() == QDialog::Rejected)
        return;
    QStringList files = dlg.selectedFiles();
    if(files.empty())
        return;
    fileName = files.first();
    ui->edtFileName->setText(fileName);
    if(!trMgr)
        trMgr = new TransferRecordManager(fileName);
    else
        trMgr->setFilename(files.first());
    QString accCode,accName,accLName;
    if(!trMgr->getAccountInfo(accCode,accName,accLName)){
        QMessageBox::critical(this,tr("出错信息"),tr("获取该账户基本信息，可能是无效的账户文件！"));
        return;
    }
    AccontTranferInfo* trInfo = trMgr->getLastTransRec();
    if(!trInfo){
        QMessageBox::critical(this,tr("出错信息"),tr("未能获取该账户的最近转移记录！"));
        return;
    }
    if(trInfo->tState != ATS_TRANSOUTED){
        QMessageBox::warning(this,tr("警告信息"),tr("该账户没有执行转出操作，不能转入本机！"));
        return;
    }
    AppConfig* conf = AppConfig::getInstance();
    if(trInfo->m_in != conf->getLocalMachine())
        QMessageBox::warning(this,tr("警告信息"),
                             tr("该账户预定转移到“%1”，如果要转移到本机则在本机只能以只读模式查看！").arg(trInfo->m_in->name()));

    ui->edtAccID->setText(accCode);
    ui->edtAccName->setText(accName);
    ui->edtAccLName->setText(accLName);
    ui->edtOutTime->setText(trInfo->t_out.toString(Qt::ISODate));
    ui->edtOutDesc->setPlainText(trInfo->desc_out);
    QHash<MachineType,QString> macTypes;
    macTypes = conf->getMachineTypes();
    ui->edtMType->setText(macTypes.value(trInfo->m_in->getType()));
    ui->edtMID->setText(QString::number(trInfo->m_in->getMID()));
    ui->edtMDesc->setText(trInfo->m_in->description());
    ui->btnOk->setEnabled(true);

    trMgr->updateMachines();
    if(trMgr->isMacUpdated())
        refreshMachines();
}
