#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>

#include "global.h"
#include "config.h"
#include "tables.h"
#include "transfers.h"
#include "commdatastruct.h"
#include "utils.h"
#include "myhelper.h"

#include "ui_transferoutdialog.h"


Machine::Machine(int id, MachineType type, int mid, bool isLocal, QString name, QString desc,int osType)
    :id(id),type(type),mid(mid),isLocal(isLocal),sname(name),desc(desc),_osType(osType)
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
    _osType = other._osType;
}

bool Machine::operator ==(const Machine &other) const
{
    if(mid != other.mid)
        return false;
    if( type != other.type || _osType != other._osType ||
           sname != other.sname || desc != other.desc)
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

TransferRecordManager::TransferRecordManager(QString filename, QWidget *parent):QObject(parent),p(parent)
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
        myHelper::ShowMessageBoxError(tr("无法建立与选定账户文件的数据库连接！"));
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
    s = QString("select count() from sqlite_master where type='table' and name='tem_%1'").arg(tbl_base_users);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.first();
    c = q.value(0).toInt();
    isExistTemUserTable = (c == 1);
    genMergeMacs();
    trRec = getLastTransRec();
    //如果捎带了用户信息，则更新本站的用户信息表
    if(isExistTemUserTable && upUsers.isEmpty()){
        s = QString("select count() from tem_%1").arg(tbl_base_users);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.first();
        c = q.value(0).toInt();
        if(c == 0)
            return true;
        s = QString("select * from tem_%1").arg(tbl_base_users);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next()){
            int uid = q.value(0).toInt();
            QString name = q.value(FI_BASE_U_NAME).toString();
            QString password = q.value(FI_BASE_U_PASSWORD).toString();
            QString groups = q.value(FI_BASE_U_GROUPS).toString();
            QString accounts = q.value(FI_BASE_U_ACCOUNTS).toString();
            QString rights = q.value(FI_BASE_U_EXTRARIGHTS).toString();
            QSet<Right*> extraRights;
            bool ok = false;
            int rid;
            foreach(QString rd, rights.split(",")){
                rid = rd.toInt(&ok);
                if(!ok)
                    LOG_ERROR(QString("Invalid Right code '%1'").arg(rid));
                Right* r = allRights.value(rid);
                if(!r)
                    LOG_ERROR(QString("Invalid Right code '%1'").arg(rid));
                extraRights.insert(r);
            }
            //剔除无效组代码
            QSet<UserGroup*> gs;
            int gc = 0;
            foreach(QString cs, groups.split(",")){
                gc = cs.toInt(&ok);
                if(!ok){
                    LOG_ERROR(QString("Invalid group code: %1").arg(cs));
                    continue;
                }
                UserGroup* g = allGroups.value(gc);
                if(!g){
                    myHelper::ShowMessageBoxWarning(tr("捎带用户“%1”所属组在本站不存在，组代码为“%2”").arg(name).arg(gc));
                    continue;
                }
                gs.insert(g);
            }
            User* u = allUsers.value(uid);
            if(!u){
                u = new User(uid,name,password,gs);
                //allUsers[uid] = u;
            }
            else{
                if(u->getName() != name)
                    u->setName(name);
                if(u->getPassword() != password)
                    u->setPassword(password);
                if(u->getOwnerGroups() != gs)
                    u->setOwnerGroups(gs);
            }
            if(u->getExclusiveAccounts().join(",") != accounts)
                u->setExclusiveAccounts(accounts.split(","));
            if(u->getExtraRights() != extraRights){
                foreach(Right* r, extraRights)
                    u->addRight(r);
            }
            upUsers<<u;
        }
    }
    return true;
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
        s = QString("create table tem_%1(id integer primary key, %2 integer, %3 integer, %4 text, %5 text, %6 integer)")
                .arg(tbl_machines).arg(fld_mac_mid).arg(fld_mac_type).arg(fld_mac_sname).arg(fld_mac_desc).arg(fld_mac_ostype);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            myHelper::ShowMessageBoxError(tr("创建临时主机表失败！"));
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
        s = QString("insert into tem_%1(%2,%3,%4,%5,%6) values(%7,%8,'%9','%10',%11)")
                .arg(tbl_machines).arg(fld_mac_mid).arg(fld_mac_type).arg(fld_mac_sname)
                .arg(fld_mac_desc).arg(fld_mac_ostype).arg(mac->getMID()).arg(mac->getType())
                .arg(mac->name()).arg(mac->description()).arg(mac->osType());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            QMessageBox::critical(0,tr("出错信息"),tr("往临时主机表插入主机信息时失败！"));
            return false;
        }
    }
    return true;
}

/**
 * @brief TransferRecordManager::attechUserInfo
 *  捎带用户信息到转入站
 * @param users
 * @return
 */
bool TransferRecordManager::attechUserInfo(QList<User *> users)
{
    if(!connected)
        return false;
    if(trRec && trRec->tState != ATS_TRANSINDES) //只有在账户已转入目标主机的情况下，才可以执行转出操作
        return true;
    QSqlQuery q(db);
    QString s;
    if(!isExistTemUserTable){
        s = QString("create table tem_%1(id INTEGER PRIMARY KEY, %2 TEXT, %3 TEXT, %4 TEXT, %5 TEXT, %6 TEXT)")
                .arg(tbl_base_users).arg(fld_base_u_name).arg(fld_base_u_password)
                .arg(fld_base_u_groups).arg(fld_base_u_accounts).arg(fld_base_u_extra_rights);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }

    }else{
        s = QString("delete from tem_%1").arg(tbl_base_users);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    s = QString("insert into tem_%1(id,%2,%3,%4,%5,%6) values(:id,:name,:pw,:g,:a,:right)")
            .arg(tbl_base_users).arg(fld_base_u_name).arg(fld_base_u_password)
            .arg(fld_base_u_groups).arg(fld_base_u_accounts).arg(fld_base_u_extra_rights);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    foreach(User* u, users){
        q.bindValue(":id",u->getUserId());
        q.bindValue(":name",u->getName());
        q.bindValue(":pw",u->getPassword());
        q.bindValue(":g",u->getOwnerGroupCodeList());
        q.bindValue(":a",u->getExclusiveAccounts().join(","));
        q.bindValue(":right",u->getExtraRightCodes());
        if(!q.exec()){
            LOG_SQLERROR(q.lastQuery());
            return false;
        }
    }
    return true;
}

/**
 * @brief 移除捎带用户信息的临时表
 * @return
 */
bool TransferRecordManager::clearTemUserTable()
{
    if(!isExistTemUserTable)
        return true;
    QSqlQuery q(db);
    QString s = QString("drop table tem_%1").arg(tbl_base_users);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    isExistTemUserTable = false;
    return true;
}

/**
 * @brief 从临时用户表捎带的用户更新本站用户信息
 * @return
 */
bool TransferRecordManager::updateUsers()
{
    if(!connected)
        return false;
    if(upUsers.isEmpty())
        return true;
    AppConfig* conf = AppConfig::getInstance();
    foreach(User* u, upUsers){
        if(!conf->saveUser(u)){
            myHelper::ShowMessageBoxError(tr("在保存捎带用户“%1（%2）时发生错误！”")
                                                          .arg(u->getName().arg(u->getUserId())));
            return false;
        }
    }
    if(!clearTemUserTable())
        LOG_ERROR("Failed delete temporary user table!");
    return true;
}

/**
 * @brief TransferRecordManager::updateMachines
 *  如果被转入的账户捎带了工作站信息，则和本地工作站上保存的工作站列表比较，进行必要的更新、新增和删除操作
 *  用一个表格显示比较的结果，如果是要更新的工作站，则第一列有一个检取框供用户选择是否更新，如果没有检取框，
 *  则表示此工作站信息没有变化，或此工作站在本工作站中存在但在来源工作站中不存在（用灰色背景）
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

    QDialog dlg(p);
    QLabel* lblExplain = new QLabel(QObject::tr("说明：蓝色--新增，红色--不相同，黑色--相同，绿色--未更新，灰底--本机存在但未捎带"));
    QFont font = lblExplain->font();
    font.setBold(true);
    lblExplain->setFont(font);
    tab = new QTableWidget(&dlg);
    tab->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab->setColumnCount(6);
    int rowCount = newMacs.count()+upMacs.count()+notMacs.count()+notExistMacs.count();
    tab->setRowCount(rowCount);
    tab->horizontalHeader()->setStretchLastSection(true);
    tab->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QStringList ht;
    ht<<tr("是否更新")<<tr("主机ID")<<tr("主机类型")<<tr("宿主系统")<<tr("主机名")<<tr("主机描述");
    tab->setHorizontalHeaderLabels(ht);
    QHash<MachineType,QString> macTypes = conf->getMachineTypes();
    QPushButton* btnExec = new QPushButton(tr("更新本地主机"),&dlg);
    QPushButton* btnClose = new QPushButton(tr("关闭"),&dlg);
    QObject::connect(btnExec,SIGNAL(clicked()),this,SLOT(execStationUpdate()));
    QObject::connect(btnClose,SIGNAL(clicked()),&dlg,SLOT(close()));
    int row = 0;
    Machine* mac;
    QTableWidgetItem* item;
    QBrush color_blue(QColor(Qt::blue));    //新增主机
    QBrush color_red(QColor(Qt::red));      //更新主机
    QBrush color_grey(QColor(Qt::gray));    //
    QBrush color_green(QColor(Qt::darkGreen));  //未更新主机
    QHash<int,QString> osTypes;
    conf->getOsTypes(osTypes);
    //装载新增主机
    for(int i = 0; i < newMacs.count(); ++i,++row){
        mac = newMacs.at(i);
        tab->setCellWidget(row,0,new QCheckBox());
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        item->setForeground(color_blue);
        tab->setItem(row,MCI_MID+1,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        item->setForeground(color_blue);
        tab->setItem(row,MCI_TYPE+1,item);
        item = new QTableWidgetItem(osTypes.value(mac->osType(),tr("未知系统")));
        item->setForeground(color_blue);
        tab->setItem(row,MCI_OSTYPE+1,item);
        item = new QTableWidgetItem(mac->name());
        item->setForeground(color_blue);
        tab->setItem(row,MCI_NAME+1,item);
        item = new QTableWidgetItem(mac->description());
        item->setForeground(color_blue);
        tab->setItem(row,MCI_DESC+1,item);
    }
    //装载信息更新的主机
    for(int i = 0; i < upMacs.count(); ++i,++row){
        mac = upMacs.at(i);
        tab->setCellWidget(row,0,new QCheckBox());
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        item->setForeground(color_red);
        tab->setItem(row,MCI_MID+1,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        item->setForeground(color_red);
        tab->setItem(row,MCI_TYPE+1,item);
        item = new QTableWidgetItem(osTypes.value(mac->osType(),tr("未知系统")));
        item->setForeground(color_red);
        tab->setItem(row,MCI_OSTYPE+1,item);
        item = new QTableWidgetItem(mac->name());
        item->setForeground(color_red);
        tab->setItem(row,MCI_NAME+1,item);
        item = new QTableWidgetItem(mac->description());
        item->setForeground(color_red);
        tab->setItem(row,MCI_DESC+1,item);
    }
    //装载未变更的主机
    for(int i = 0; i < notMacs.count(); ++i,++row){
        mac = notMacs.at(i);
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        item->setForeground(color_green);
        tab->setItem(row,MCI_MID+1,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        item->setForeground(color_green);
        tab->setItem(row,MCI_TYPE+1,item);
        item = new QTableWidgetItem(osTypes.value(mac->osType(),tr("未知系统")));
        item->setForeground(color_green);
        tab->setItem(row,MCI_OSTYPE+1,item);
        item = new QTableWidgetItem(mac->name());
        item->setForeground(color_green);
        tab->setItem(row,MCI_NAME+1,item);
        item = new QTableWidgetItem(mac->description());
        item->setForeground(color_green);
        tab->setItem(row,MCI_DESC+1,item);
    }
    //装载本机存在但未捎带的主机（可能这些主机已经被认为不实际存在了，可以清除）
    for(int i = 0; i < notExistMacs.count(); ++i,++row){
        mac = notExistMacs.at(i);
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        item->setBackground(color_grey);
        tab->setItem(row,MCI_MID+1,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        item->setBackground(color_grey);
        tab->setItem(row,MCI_TYPE+1,item);
        item = new QTableWidgetItem(osTypes.value(mac->osType(),tr("未知系统")));
        item->setBackground(color_grey);
        tab->setItem(row,MCI_OSTYPE+1,item);
        item = new QTableWidgetItem(mac->name());
        item->setBackground(color_grey);
        tab->setItem(row,MCI_NAME+1,item);
        item = new QTableWidgetItem(mac->description());
        item->setBackground(color_grey);
        tab->setItem(row,MCI_DESC+1,item);
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
    isExistTemMacTable = false;
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
    smid = q.value(TRANS_SMID).toInt();
    dmid = q.value(TRANS_DMID).toInt();
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
 *  生成合并的工作站信息（本地拥有的和引入文件捎带的）
 *  同时进行本地工作站信息与捎带工作站信息的比较，并初始化新增、更新工作站列表成员
 * @return
 */
void TransferRecordManager::genMergeMacs()
{
    mergeMacs.clear();localMacs.clear();newMacs.clear();upMacs.clear();
    notMacs.clear();notExistMacs.clear();
    localMacs = AppConfig::getInstance()->getAllMachines();
    QList<int> localMids = localMacs.keys();
    mergeMacs = localMacs;
    if(!_getTakeMacs() || takeMacs.isEmpty())
        return;
    foreach(Machine* mac, takeMacs){
        int mid = mac->getMID();
        if(!localMacs.contains(mid)){
            mergeMacs[mid] = mac;
            newMacs<<mac;
        }
        else {
            if(*mac != *localMacs.value(mac->getMID())){
                mergeMacs[mid] = mac;    //工作站信息以捎带的为准
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
}

/**
 * @brief TransferRecordManager::execUpdate
 *  执行更新工作站操作
 */
void TransferRecordManager::execStationUpdate()
{
    if(!_inspectSelUpMacs()){
        myHelper::ShowMessageBoxWarning(tr("来源工作站或目的工作站信息已更新，必须更新到本地，否者造成工作站无法识别或无法显示最新的工作站信息！"));
        return;
    }
    AppConfig* conf = AppConfig::getInstance();
    if(!conf->saveMachines(selUpMacs))
        myHelper::ShowMessageBoxError(tr("在保存更新的工作站信息时出错"));
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
        int osType = q.value(MACS_OSTYPE-1).toInt();
        QString name = q.value(MACS_NAME-1).toString();
        QString desc = q.value(MACS_DESC-1).toString();
        takeMacs[mid] = new Machine(UNID,type,mid,false,name,desc,osType);
    }
    return true;
}

/**
 * @brief TransferRecordManager::_inspectSelUpMacs
 *  用户选择的更新工作站是否满足基本更新条件，即
 *  如果当前引入的账户涉及到的主机（来源机和目标机）发生了变更，则必须至少选择它们
 * @return true：满足
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
                mac->setOsType(takeMac->osType());
                mac->setName(takeMac->name());
                mac->setDescription(takeMac->description());
            }
            selUpMacs<<mac;
        }
    }
    //如果本地未包含源工作站，也未选择更新
    if(!localMacs.contains(trRec->m_in->getMID()) && !selUpMacs.contains(trRec->m_in))
        return false;
    //如果本地未包含目的工作站，也未选择更新
    if(!localMacs.contains(trRec->m_out->getMID()) && !selUpMacs.contains(trRec->m_out))
        return false;
    //如果本地包含了源工作站，且源工作站发生了更新但未选择更新
    if(!localMacs.contains(trRec->m_in->getMID()) && upMacs.contains(trRec->m_in) && !selUpMacs.contains(trRec->m_in))
        return false;
    //如果本地包含了目的工作站，且目的工作站发生了更新但未选择更新
    if(!localMacs.contains(trRec->m_out->getMID()) && upMacs.contains(trRec->m_out) && !selUpMacs.contains(trRec->m_out))
        return false;
    else
        return true;
}

///////////////////////////TransferOutDialog//////////////////////////////////////////

TransferOutDialog::TransferOutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::TransferOutDialog)
{
    ui->setupUi(this);
    ui->tView->setColumnWidth(MCI_MID,50);
    ui->tView->setColumnWidth(MCI_TYPE,100);
    ui->tView->setColumnWidth(MCI_OSTYPE,150);
    ui->tView->setColumnWidth(MCI_NAME,150);
    ui->tView->addAction(ui->actDel);
    ui->tView->addAction(ui->actReset);
    connect(ui->tView,SIGNAL(itemSelectionChanged()),this,SLOT(enAct()));

    conf = AppConfig::getInstance();
    tranStates = conf->getAccTranStates();
    lm = conf->getLocalStation();
    if(lm){
        ui->edtLocalMac->setText(tr("%1（%2）").arg(lm->name()).arg(lm->getMID()));
        Machine* ms = conf->getMasterStation();
        if(ms && lm->getMID() == ms->getMID()) //主站
            ui->btnTakeUser->setEnabled(true);
    }
    accCacheItems = conf->getAllCachedAccounts();
    if(accCacheItems.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("没有任何本地账户，无法执行转出操作！"));
        ui->btnOk->setEnabled(false);
        return;
    }
    localMacs = conf->getAllMachines().values();
    qSort(localMacs.begin(),localMacs.end(),byMacMID);
    if(localMacs.isEmpty()){
        myHelper::ShowMessageBoxError(tr("无法读取工作站列表"));
        ui->btnOk->setEnabled(false);
        return;
    }
    foreach(AccountCacheItem* acc, accCacheItems)
        ui->cmbAccount->addItem(acc->accName);
    foreach(Machine* mac, localMacs)
        ui->cmbMachines->addItem(mac->name());
    QString path = conf->getDirName(AppConfig::DIR_TRANSOUT);
    if(path.isEmpty())
        path = QDir::homePath();
    dir.setPath(path);
    if(!dir.exists())
        dir.setPath(QDir::homePath());
    ui->edtDir->setText(path);
    ui->cmbAccount->setCurrentIndex(-1);
    ui->cmbMachines->setCurrentIndex(-1);
    connect(ui->cmbAccount,SIGNAL(currentIndexChanged(int)),this,SLOT(selectAccountChanged(int)));
    ui->edtDesc->setPlainText(tr("转出做帐"));
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
        if(acc->tState != ATS_TRANSINDES)
            myHelper::ShowMessageBoxWarning(tr("所选账户上次转入时目的工作站不是本站"));
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
    if(index < 0 || index >= localMacs.count()){
        ui->edtMID->clear();
        ui->edtMType->clear();
        ui->edtMDesc->clear();
        return;
    }
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
    dlg.setNameFilter("Sqlite files(*.dat)");
    dlg.setDirectory(dir.absolutePath());
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
        //ui->tView->clearContents();
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
    QString sn = DATABASE_PATH + getAccountFileName();
    TransferRecordManager trMgr(sn,this);
    AppConfig* conf = AppConfig::getInstance();
    AccontTranferInfo rec;
    rec.id = UNID;
    rec.desc_out = ui->edtDesc->toPlainText();
    rec.m_in = getDestiMachine();
    rec.m_out = lm;
    rec.tState = ATS_TRANSOUTED;
    rec.t_out = QDateTime::currentDateTime();
    if(!trMgr.saveTransferRecord(&rec)){
        myHelper::ShowMessageBoxError(tr("在添加转移记录时出错！"));
        return;
    }
    if(isTakeMachineInfo() && !trMgr.attechMachineInfo(takeMacs))
        myHelper::ShowMessageBoxError(tr("写入捎带站点信息时出错，无法捎带站点信息！"));
    if(!upUsers.isEmpty() && !trMgr.attechUserInfo(upUsers))
        myHelper::ShowMessageBoxError(tr("写入捎带用户信息时出错，无法捎带用户信息！"));

    //2、修改本地的账户缓存记录
    AccountCacheItem* acc = accCacheItems.at(ui->cmbAccount->currentIndex());
    acc->outTime = QDateTime::currentDateTime();
    acc->mac = rec.m_in;
    acc->tState = ATS_TRANSOUTED;
    if(!conf->saveAccountCacheItem(acc)){
        myHelper::ShowMessageBoxError(tr("在保存本地账户缓存项目时出错，无法执行转出操作！"));
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
        myHelper::ShowMessageBoxError(tr("账户文件拷贝失败！"));
        return;
    }
    conf->saveDirName(AppConfig::DIR_TRANSOUT,dir.absolutePath());
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
    QHash<int,QString> osTypes;
    if(!AppConfig::getInstance()->getOsTypes(osTypes)){
        myHelper::ShowMessageBoxError(tr("无法获取宿主操作系统类型！"));
        return;
    }
    QTableWidgetItem* item;
    int i = -1;
    foreach(Machine* mac, localMacs){
        i++;
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        ui->tView->setItem(i,MCI_MID,item);
        if(mac->getType() == MT_COMPUTER)
            item = new QTableWidgetItem(tr("物理电脑"));
        else
            item = new QTableWidgetItem(tr("云账户"));
        ui->tView->setItem(i,MCI_TYPE,item);
        item = new QTableWidgetItem(osTypes.value(mac->osType(),tr("未知系统")));
        ui->tView->setItem(i,MCI_OSTYPE,item);
        item = new QTableWidgetItem(mac->name());
        ui->tView->setItem(i,MCI_NAME,item);
        item = new QTableWidgetItem(mac->description());
        ui->tView->setItem(i,MCI_DESC,item);
        if(mac->isLocalStation()){
            QBrush back(QColor("grey"));
            for(int j = 0; j < ui->tView->columnCount(); ++j)
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
            info = tr("未选择转出的目标工作站");
            break;
        case DTR_NOTSTATE:
            info = tr("所选账户上次转入时目的工作站不是本站");
            break;
        case DTR_NOTLOCAL:
            info = tr("本站未定义");
            break;
        }
        myHelper::ShowMessageBoxWarning(tr("不能执行转出操作，原因：\n%1！").arg(info));
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
    ui->twMachines->setColumnWidth(MCI_MID,50);
    ui->twMachines->setColumnWidth(MCI_TYPE,80);
    ui->twMachines->setColumnWidth(MCI_OSTYPE,100);
    ui->twMachines->setColumnWidth(MCI_NAME,150);
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
    QHash<int,QString> osTypes;
    conf->getOsTypes(osTypes);
    QBrush lmBack(QColor("grey"));
    QHash<int,Machine*> ms; ms = conf->getAllMachines();
    QList<int> mids; mids = ms.keys(); qSort(mids.begin(),mids.end());
    Machine* mac;
    for(int i=0; i< mids.count(); ++i){
        mac = ms.value(mids.at(i));
        ui->twMachines->insertRow(row);
        item = new QTableWidgetItem(QString::number(mac->getMID()));
        ui->twMachines->setItem(row,MCI_MID,item);
        item = new QTableWidgetItem(macTypes.value(mac->getType()));
        ui->twMachines->setItem(row,MCI_TYPE,item);
        item = new QTableWidgetItem(osTypes.value(mac->osType(),tr("未知系统")));
        ui->twMachines->setItem(row,MCI_OSTYPE,item);
        item = new QTableWidgetItem(mac->name());
        ui->twMachines->setItem(row,MCI_NAME,item);
        item = new QTableWidgetItem(mac->description());
        ui->twMachines->setItem(row,MCI_DESC,item);
        if(mac->isLocalStation()){
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
    QDir dir(DATABASE_PATH);
    QString fname = QFileInfo(fileName).fileName();
    BackupUtil bu;
    bool isBackup = false;
    if(dir.exists(fname)){
        if(myHelper::ShowMessageBoxQuesion(tr("本站拥有同名账户，要覆盖它吗？")) == QDialog::Rejected)
            return;
        if(!bu.backup(fname,BackupUtil::BR_TRANSFERIN)){
            myHelper::ShowMessageBoxWarning(tr("在转入账户前执行备份操作出错！"));
            return;
        }
        isBackup = true;
        dir.remove(fname);
    }
    QString desFName = DATABASE_PATH + fname;
    QString error;
    if(!QFile::copy(fileName,desFName)){
        myHelper::ShowMessageBoxError(tr("文件拷贝操作出错，请手工将账户文件拷贝到工作目录"));
        if(isBackup && !bu.restore(fname,BackupUtil::BR_TRANSFERIN,error))
            myHelper::ShowMessageBoxWarning(tr("无法恢复被覆盖的账户文件！"));
        return;
    }

    //2、修改账户的转移记录
    //（1）状态（转入目标机或转入其他机）
    //（2）设置用户输入的转入描述信息及其转入时间
    //（3）转出时间、转出源主机、转入目的主机、转出描述等内容不变    

    trMgr->setFilename(desFName);
    AccontTranferInfo* tranRec = trMgr->getLastTransRec();
    AppConfig* conf = AppConfig::getInstance();
    if(tranRec->m_in == conf->getLocalStation())
        tranRec->tState = ATS_TRANSINDES;
    else
        tranRec->tState = ATS_TRANSINOTHER;
    tranRec->desc_in = ui->edtInDesc->toPlainText();
    tranRec->t_in = QDateTime::currentDateTime();
    if(!trMgr->saveTransferRecord(tranRec)){
        myHelper::ShowMessageBoxError(tr("保存转移记录时出错！"));
        if(isBackup && !bu.restore(/*fname,BackupUtil::BR_TRANSFERIN,*/error))
            myHelper::ShowMessageBoxWarning(tr("无法恢复被覆盖的账户文件！"));
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
    if(tranRec->m_in->getMID() == conf->getLocalStation()->getMID())
        ci->tState = ATS_TRANSINDES;
    else
        ci->tState = ATS_TRANSINOTHER;
    if(!conf->saveAccountCacheItem(ci)){
        myHelper::ShowMessageBoxError(tr("在保存缓存账户条目时出错！"));
        if(isBackup && !bu.restore(fname,BackupUtil::BR_TRANSFERIN,error))
            myHelper::ShowMessageBoxWarning(tr("无法恢复被覆盖的账户文件！"));
        return;
    }
    if(ui->btnClear->isChecked())
        trMgr->clearTemMachineTable();
    if(trMgr->isReqUpdateUser() && !trMgr->updateUsers())
        myHelper::ShowMessageBoxError(tr("从捎带用户更新本地用户信息时发生错误，这可能将导致转入的账户无法正确操作！"));
    QString path = QFileInfo(fileName).path();
    conf->saveDirName(AppConfig::DIR_TRANSIN,path);
    accept();
}

/**
 * @brief TransferInDialog::on_btnSelectFile_clicked
 *  提供用户选择要转入的账户文件，并读取用户选择的账户的信息（包括账户基本信息和转移信息）
 */
void TransferInDialog::on_btnSelectFile_clicked()
{
    ui->btnOk->setEnabled(false);
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setNameFilter("Sqlite files(*.dat)");
    QString path = AppConfig::getInstance()->getDirName(AppConfig::DIR_TRANSIN);
    if(path.isEmpty() || !QFileInfo::exists(path))
        path = QDir::homePath();
    dlg.setDirectory(path);
    if(dlg.exec() == QDialog::Rejected)
        return;
    QStringList files = dlg.selectedFiles();
    if(files.empty())
        return;
    fileName = files.first();
    ui->edtFileName->setText(fileName);
    if(!trMgr)
        trMgr = new TransferRecordManager(fileName,this);
    else
        trMgr->setFilename(files.first());
    QString accCode,accName,accLName;
    if(!trMgr->getAccountInfo(accCode,accName,accLName)){
        myHelper::ShowMessageBoxError(tr("获取该账户基本信息，可能是无效的账户文件！"));
        return;
    }
    AccontTranferInfo* trInfo = trMgr->getLastTransRec();
    if(!trInfo){
        myHelper::ShowMessageBoxError(tr("未能获取该账户的最近转移记录！"));
        return;
    }
    if(trInfo->tState != ATS_TRANSOUTED){
        myHelper::ShowMessageBoxWarning(tr("该账户没有执行转出操作，不能转入本工作站！"));
        return;
    }
    //源站不明，也要忽略
    //if(trInfo->m_out)

    //如果执行转入操作的工作站内存在对应的缓存账户，且缓存项的转移状态是已转入的目的站，则表示
    //此账户的转移状态异常，原因可能是手工任务地修改了账户的转移状态，使其在整个工作组集内的转移状态不一致
    AccountCacheItem* acItem = AppConfig::getInstance()->getAccountCacheItem(accCode);
    if(acItem && acItem->tState == ATS_TRANSINDES){
        QDialog dlg(this);
        QLabel title(tr("转移状态异常"),&dlg);
        title.setAlignment(Qt::AlignCenter);
        QTableWidget tw(&dlg);
        tw.setColumnCount(4);
        tw.setRowCount(2);
        tw.horizontalHeader()->setStretchLastSection(true);
        QStringList titles;
        titles<<tr("账户")<<"MID"<<tr("工作站名")<<tr("转入/出时间");
        tw.setHorizontalHeaderLabels(titles);
        tw.setColumnWidth(0,80); tw.setColumnWidth(1,50);
        tw.setColumnWidth(2,80);
        tw.setItem(0,0,new QTableWidgetItem(tr("转入账户")));
        tw.setItem(1,0,new QTableWidgetItem(tr("缓存账户")));
        tw.setItem(0,1,new QTableWidgetItem(trInfo->m_out?QString::number(trInfo->m_out->getMID()):""));
        tw.setItem(0,2,new QTableWidgetItem(trInfo->m_out?trInfo->m_out->name():tr("未知站")));
        tw.setItem(0,3,new QTableWidgetItem(trInfo->t_out.toString(Qt::ISODate)));
        tw.setItem(1,1,new QTableWidgetItem(acItem->mac?QString::number(acItem->mac->getMID()):""));
        tw.setItem(1,2,new QTableWidgetItem(acItem->mac?acItem->mac->name():tr("未知站")));
        tw.setItem(1,3,new QTableWidgetItem(acItem->inTime.toString(Qt::ISODate)));
        QVBoxLayout* lm = new QVBoxLayout;
        lm->addWidget(&title);
        lm->addWidget(&tw);
        dlg.setLayout(lm);
        dlg.resize(600,300);
        dlg.exec();
        return;
    }

    AppConfig* conf = AppConfig::getInstance();
    if(trInfo->m_in->getMID() != conf->getLocalStation()->getMID())
        myHelper::ShowMessageBoxWarning(tr("该账户预定转移到“%1”，如果要转移到本工作站则在本工作站只能以只读模式查看！")
                                        .arg(trInfo->m_in->name()));

    ui->edtAccID->setText(accCode);
    ui->edtAccName->setText(accName);
    ui->edtAccLName->setText(accLName);
    ui->edtOutTime->setText(trInfo->t_out.toString(Qt::ISODate));
    ui->edtOutDesc->setPlainText(trInfo->desc_out);
    QHash<MachineType,QString> macTypes;
    macTypes = conf->getMachineTypes();
    QHash<int,QString> osTypes;
    conf->getOsTypes(osTypes);
    ui->edtMType->setText(macTypes.value(trInfo->m_in->getType()));
    ui->edtMID->setText(QString::number(trInfo->m_in->getMID()));
    ui->edtOsType->setText(osTypes.value(trInfo->m_in->osType(),tr("未知系统")));
    ui->edtMDesc->setText(trInfo->m_in->description());
    ui->btnOk->setEnabled(true);



    trMgr->updateMachines();
    if(trMgr->isMacUpdated()){
        refreshMachines();
        ui->btnClear->setChecked(true   );
    }
}

/**
 * @brief 编辑要捎带的用户信息
 */
void TransferOutDialog::on_btnTakeUser_clicked()
{
    QDialog dlg(this);
    QTableWidget tw(&dlg);
    tw.setColumnCount(2);
    tw.horizontalHeader()->setStretchLastSection(true);
    tw.setSelectionBehavior(QAbstractItemView::SelectRows);
    int row = 0;
    QList<User*> users = allUsers.values();
    qSort(users.begin(),users.end(),userByCode);
    foreach(User* u, users){
        tw.insertRow(row);
        QTableWidgetItem* item = new QTableWidgetItem(tr("%1(%2)").arg(u->getName()).arg(u->getUserId()));
        item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
        QVariant v; v.setValue<User*>(u);
        item->setData(Qt::UserRole,v);
        item->setCheckState(upUsers.contains(u)?Qt::Checked:Qt::Unchecked);
        tw.setItem(row,0,item);
        QStringList gs;
        foreach(UserGroup* g,u->getOwnerGroups())
            gs<<g->getName();
        item = new QTableWidgetItem(gs.join(","));
        tw.setItem(row,1,item);
    }
    QPushButton btnOk(tr("确定"),&dlg),btnCancel(tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb; lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&tw); lm->addLayout(&lb);
    dlg.setLayout(lm);
    if(dlg.exec() == QDialog::Rejected)
        return;
    if(!upUsers.isEmpty())
        upUsers.clear();
    for(int i = 0; i < tw.rowCount(); ++i){
        if(tw.item(i,0)->checkState() == Qt::Checked)
            upUsers<<tw.item(i,0)->data(Qt::UserRole).value<User*>();
    }

}
