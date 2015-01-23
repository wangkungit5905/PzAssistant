#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QSqlError>
#include <QTableWidget>

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

/**
 * @brief 序列化对象到文本
 * @return
 */
QString Machine::serialToText()
{
    QStringList ls;
    for(int i = 0; i < 6; ++i)
        ls<<"";
    ls[SOFI_WS_MID] = QString::number(mid);
    ls[SOFI_WS_TYPE] = QString::number((int)type);
    ls[SOFI_WS_ISLOCAL] = (isLocal?"1":"0");
    ls[SOFI_WS_NAME] = sname;
    ls[SOFI_WS_DESC] = desc;
    ls[SOFI_WS_OSTYPE] = QString::number(_osType);
    return ls.join("||");
}

Machine *Machine::serialFromText(QString serialText)
{
    QStringList ls = serialText.split("||");
    if(ls.count() != 6)
        return 0;
    bool ok;
    int mid = ls.at(SOFI_WS_MID).toInt(&ok);
    if(!ok)
        return 0;
    MachineType type = (MachineType)ls.at(SOFI_WS_TYPE).toInt(&ok);
    if(!ok)
        return 0;
    if(type != MT_COMPUTER && type != MT_COMPUTER){
        LOG_ERROR(QString("Create Workstation Object failed! Reason is type code invalid! text is '%1'").arg(serialText));
        return 0;
    }
    bool isLocal = (ls.at(SOFI_WS_ISLOCAL) == "0")?false:true;
    int osType = ls.at(SOFI_WS_OSTYPE).toInt(&ok);
    if(!ok)
        LOG_ERROR(QString("Create Workstation Object have Warning! Reason is Operate System code invalid! text is '%1'").arg(serialText));
    Machine* mac = new Machine(UNID,type,mid,isLocal,ls.at(SOFI_WS_NAME),ls.at(SOFI_WS_DESC),osType);
    return mac;
}

void Machine::serialAllToBinary(int mv, int sv, QByteArray *ds)
{
    QList<Machine*> macs = AppConfig::getInstance()->getAllMachines().values();
    qSort(macs.begin(),macs.end(),byMacMID);
    QBuffer bf(ds);
    QTextStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    out<<QString("version=%1.%2\n").arg(mv).arg(sv);
    foreach(Machine* m, macs)
        out<<m->serialToText()<<"\n";
    bf.close();
}

bool Machine::serialAllFromBinary(QList<Machine *> &macs, int &mv, int &sv, QByteArray *ds)
{
    QBuffer bf(ds);
    QTextStream in(&bf);
    bf.open(QIODevice::ReadOnly);
    QStringList sl = in.readLine().split("=");
    if(sl.count() != 2)
        return false;
    sl = sl.at(1).split(".");
    if(sl.count() != 2)
        return false;
    bool ok;
    mv = sl.at(0).toInt(&ok);
    if(!ok)
        return false;
    sv = sl.at(1).toInt(&ok);
    if(!ok)
        return false;

    while(!in.atEnd()){
        Machine* m = serialFromText(in.readLine());
        if(!m)
            return false;
        macs<<m;
    }
    return true;
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
    isExistTemAppCfgTable = false;
    macUpdated = false;
    isTranOut = (p->objectName() == "TransferOutDialog");
    conf = AppConfig::getInstance();
    localMacs = conf->getAllMachines();
    setFilename(filename);
}

TransferRecordManager::~TransferRecordManager()
{
    if(db.isOpen())
        db.close();
    QSqlDatabase::removeDatabase(TRANSFER_MANAGER_CONNSTR);
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
    QString s = QString("select count() from sqlite_master where type='table' and name='%1'").arg(tbl_tem_appcfg);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.first();
    int c = q.value(0).toInt();
    isExistTemAppCfgTable = (c == 1);
    trRec = getLastTransRec();
    q.finish();
    //如果是执行转入操作，且账户捎带了应用配置信息，则与本站进行版本比较只有在高于本站版本时才可以应用
    if(isExistTemAppCfgTable && !isTranOut && !upgradeAppCfg()){
        myHelper::ShowMessageBoxError(tr("应用配置信息更新过程发生错误，可能使应用无法处于最新有效的运行状态！"));
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
 *  让转出的账户文件捎带执行转出操作的主机上的应用配置信息，将在账户文件内创建临时保存配置信息的表
 *  此方法只在执行转出操作时调用
 * @return
 */
bool TransferRecordManager::attechAppCfgInfo(QList<BaseDbVersionEnum> &verTypes, QStringList &verNames, QList<int> &mvs, QList<int> &svs, QList<QByteArray*> &infos)
{
    if(!connected)
        return false;
    if(trRec && trRec->tState != ATS_TRANSINDES) //只有在账户已转入目标主机的情况下，才可以执行转出操作
        return true;
    if(verTypes.isEmpty())
        return true;
    QSqlQuery q(db);
    QString s;
    if(!isExistTemAppCfgTable){
        s = QString("create table %1(id INTEGER PRIMARY KEY, %2 INTEGER, %3 TEXT, %4 INTEGER, %5 INTEGER, %6 BLOB)")
                .arg(tbl_tem_appcfg).arg(fld_base_version_typeEnum).arg(fld_base_version_typeName)
                .arg(fld_base_version_master).arg(fld_base_version_second).arg(fld_tem_appcfg_obj);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            myHelper::ShowMessageBoxError(tr("创建临时应用配置表失败！"));
            return false;
        }
    }
    else{
        s = QString("delete from %1").arg(tbl_tem_appcfg);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    };
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save Application config infomation!");
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5,%6) values(:type,:name,:mv,:sv,:infos)")
            .arg(tbl_tem_appcfg).arg(fld_base_version_typeEnum).arg(fld_base_version_typeName)
            .arg(fld_base_version_master).arg(fld_base_version_second).arg(fld_tem_appcfg_obj);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    for(int i = 0; i < verTypes.count(); ++i){
        q.bindValue(":type",verTypes.at(i));
        q.bindValue(":name",verNames.at(i));
        q.bindValue(":mv",mvs.at(i));
        q.bindValue(":sv",svs.at(i));
        q.bindValue(":infos",*infos.at(i));
        if(!q.exec()){
            LOG_SQLERROR(q.lastQuery());
            return false;
        }
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on save Application config infomation!");
        return false;
    }
    return true;
}

/**
 * @brief TransferRecordManager::updateMachines
 *  升级应用配置信息
 *  此方法只在执行转入操作时调用
 * @return
 */
bool TransferRecordManager::upgradeAppCfg()
{
    if(!connected)
        return false;
    QSqlQuery q(db);
    if(trRec && trRec->tState != ATS_TRANSOUTED)
        return true;
    if(!isExistTemAppCfgTable)
        return true;
    QString s = QString("select * from %1").arg(tbl_tem_appcfg);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    //比较版本，以决定升级哪些配置信息
    QList<BaseDbVersionEnum> verTypes;
    QList<QByteArray*> objs;
    QList<QString> names;
    while(q.next()){
        int bmv,bsv,smv,ssv; //本站或转入捎带的主次版本号
        BaseDbVersionEnum verType = (BaseDbVersionEnum)q.value(FI_TEM_APPCFG_VERTYPE).toInt();
        conf->getAppCfgVersion(bmv,bsv,verType);
        smv = q.value(FI_TEM_APPCFG_MASTER).toInt();
        ssv = q.value(FI_TEM_APPCFG_SECOND).toInt();
        if((bmv > smv) || ((bmv == smv) && (bsv >= ssv)))
            continue;
        verTypes<<verType;
        QString name;
        switch(verType){
        case BDVE_RIGHTTYPE:
            name = tr("权限类型");
            break;
        case BDVE_RIGHT:
            name = tr("权限系统");
            break;
        case BDVE_GROUP:
            name = tr("组");
            break;
        case BDVE_USER:
            name = tr("用户");
            break;
        case BDVE_WORKSTATION:
            name = tr("工作站");
            break;
        case BDVE_COMMONPHRASE:
            name = tr("常用提示短语");
            break;
        }
        names<<name;
        objs<<new QByteArray(q.value(FI_TEM_APPCFG_OBJECT).toByteArray());
    }
    //显示给用户哪些配置信息需升级
    if(verTypes.empty()){
        myHelper::ShowMessageBoxInfo(tr("导入文件附带应用配置升级信息，但版本不高于本站配置，因此无须升级！"));
        q.finish();
        clearTemAppCfgTable();
        return true;
    }
    QString info = tr("如下应用配置信息需要升级：\n");
    foreach(QString s,names)
        info.append(s).append("\n");
    myHelper::ShowMessageBoxInfo(info);

    //升级
    int mv,sv;
    BaseDbVersionEnum verType;
    for(int i=0; i<verTypes.count(); ++i){
        bool ok1=false,ok2=false;
        verType = verTypes.at(i);
        if(verType == BDVE_RIGHTTYPE){
            QList<RightType*> rts;
            ok1 = RightType::serialAllFromBinary(rts,mv,sv,objs.at(i));
            if(!ok1)
                myHelper::ShowMessageBoxError(tr("升级“%1”配置信息时发生错误！").arg(names.at(i)));
            else{
                ok2 = conf->clearAndSaveRightTypes(rts,mv,sv);
                if(!ok2)
                    myHelper::ShowMessageBoxError(tr("保存“%1”配置信息时发生错误！").arg(names.at(i)));
            }
            if(!ok1 || !ok2){
                if(!rts.isEmpty())
                    qDeleteAll(rts);
                return false;
            }
            else{
                allRightTypes.clear();
                foreach(RightType* rt, rts)
                    allRightTypes[rt->code] = rt;
            }
        }
        else if(verType == BDVE_RIGHT){
            QList<Right*> rs;
            ok1 = Right::serialAllFromBinary(allRightTypes,rs,mv,sv,objs.at(i));
            if(!ok1)
                myHelper::ShowMessageBoxError(tr("升级“%1”配置信息时发生错误！").arg(names.at(i)));
            else{
                ok2 = conf->clearAndSaveRights(rs,mv,sv);
                if(!ok2)
                    myHelper::ShowMessageBoxError(tr("保存“%1”配置信息时发生错误！").arg(names.at(i)));
            }
            if(!ok1 || !ok2){
                if(!rs.isEmpty())
                    qDeleteAll(rs);
                return false;
            }
            else{
                allRights.clear();
                foreach(Right* r,rs)
                    allRights[r->getCode()] = r;
            }
        }
        else if(verType == BDVE_GROUP){
            QList<UserGroup*> gs;
            ok1 = UserGroup::serialAllFromBinary(gs,mv,sv,objs.at(i),allRights);
            if(!ok1)
                myHelper::ShowMessageBoxError(tr("升级“%1”配置信息时发生错误！").arg(names.at(i)));
            else{
                ok2 = conf->clearAndSaveGroups(gs,mv,sv);
                if(!ok2)
                    myHelper::ShowMessageBoxError(tr("保存“%1”配置信息时发生错误！").arg(names.at(i)));
            }
            if(!ok1 || !ok2){
                if(!gs.isEmpty())
                    qDeleteAll(gs);
                return false;
            }
            else{
                allGroups.clear();
                foreach(UserGroup* g, gs)
                    allGroups[g->getGroupCode()] = g;
            }
        }
        else if(verType == BDVE_USER){
            QList<User*> us;
            ok1 = User::serialAllFromBinary(us,mv,sv,objs.at(i),allRights,allGroups);
            if(!ok1)
                myHelper::ShowMessageBoxError(tr("升级“%1”配置信息时发生错误！").arg(names.at(i)));
            else{
                ok2 = conf->clearAndSaveUsers(us,mv,sv);
                if(!ok2)
                    myHelper::ShowMessageBoxError(tr("保存“%1”配置信息时发生错误！").arg(names.at(i)));
            }
            if(!ok1 || !ok2){
                if(!us.isEmpty())
                    qDeleteAll(us);
                return false;
            }
            else{
                allUsers.clear();
                foreach(User* u, us)
                    allUsers[u->getUserId()] = u;
            }
        }
        else if(verType == BDVE_WORKSTATION){
            QList<Machine*> macs;
            ok1 = Machine::serialAllFromBinary(macs,mv,sv,objs.at(i));
            if(!ok1)
                myHelper::ShowMessageBoxError(tr("升级“%1”配置信息时发生错误！").arg(names.at(i)));
            else{
                int localId = conf->getLocalStationId();
                foreach(Machine* m, macs){
                    if(m->getMID() == localId)
                        m->setLocalMachine(true);
                    else
                        m->setLocalMachine(false);
                }
                ok2 = conf->clearAndSaveMacs(macs,mv,sv);
                if(!ok2)
                    myHelper::ShowMessageBoxError(tr("保存“%1”配置信息时发生错误！").arg(names.at(i)));
            }
            if(!ok1 || !ok2){
                if(!macs.isEmpty())
                    qDeleteAll(macs);
                return false;
            }
            else{
                conf->refreshMachines();
                localMacs.clear();
                localMacs = conf->getAllMachines();
            }
        }
        else if(verType == BDVE_COMMONPHRASE){
            if(!conf->serialCommonPhraseFromBinary(objs.at(i)))
                myHelper::ShowMessageBoxError(tr("升级常用提示短语时发生错误！"));
        }
    }
    q.finish();
    clearTemAppCfgTable();
    myHelper::ShowMessageBoxInfo(tr("升级操作成功完成，需要重新启动应用以使配置生效！"));
    return true;
}

/**
 * @brief TransferRecordManager::clearTemMachineTable
 *  删除捎带应用配置信息的临时表
 * @return
 */
bool TransferRecordManager::clearTemAppCfgTable()
{
    QSqlQuery q(db);
    QString s = QString("drop table if exists %1").arg(tbl_tem_appcfg);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    isExistTemAppCfgTable = false;
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
    trRec = new AccontTranferInfo;
    trRec->tState = (AccountTransferState)q.value(TRANS_STATE).toInt();
    smid = q.value(TRANS_SMID).toInt();
    dmid = q.value(TRANS_DMID).toInt();
    //检测源主机和目标主机是否在本机上存在，如果不存在，则视为异常
    if(!localMacs.contains(smid) || !localMacs.contains(dmid)){
        if(isTranOut){
            myHelper::ShowMessageBoxError(tr("转出账户的最后转移记录中包含未知的源或目标主机，请先纠正之！"));
            return false;
        }
        else
            myHelper::ShowMessageBoxWarning(tr("转入账户的最后转移记录中包含未知的源或目标主机！"));
    }
    trRec->id = q.value(0).toInt();
    trRec->m_in = localMacs.value(dmid);
    trRec->m_out = localMacs.value(smid);
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

///////////////////////////TransferOutDialog//////////////////////////////////////////

TransferOutDialog::TransferOutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::TransferOutDialog)
{
    ui->setupUi(this);

    conf = AppConfig::getInstance();
    tranStates = conf->getAccTranStates();
    lm = conf->getLocalStation();
    if(lm){
        ui->edtLocalMac->setText(tr("%1（%2）").arg(lm->name()).arg(lm->getMID()));
        Machine* ms = conf->getMasterStation();
        if(ms && (lm->getMID() == ms->getMID()) && curUser->isSuperUser()) //主站
            ui->gbIsTake->setChecked(true);
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

    QHash<int,QString> phrases;
    conf->readPhases(CPPC_TRAN_OUT,phrases);
    QList<int> keys = phrases.keys();
    qSort(keys.begin(),keys.end());
    for(int i = 0; i < keys.count(); ++i){
        int key = keys.at(i);
        ui->cmbComDesc->addItem(phrases.value(key),key);
    }
    ui->cmbComDesc->setCurrentIndex(-1);
    connect(ui->cmbComDesc,SIGNAL(currentIndexChanged(QString)),this,SLOT(transInPhraseSelected(QString)));
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

/**
 * @brief 是否捎带应用配置信息（比如安全模块配置信息等）
 * @return
 */
bool TransferOutDialog::isTakeAppConInfo()
{
    return ui->chkRightType->isChecked()||ui->chkRight->isChecked()||ui->chkUser->isChecked()||
           ui->chkWS->isChecked()||ui->chkGroup->isChecked()||ui->chkPhrases->isChecked();
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

void TransferOutDialog::transInPhraseSelected(QString text)
{
    ui->edtDesc->setPlainText(text);
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
    dlg.setFileMode(QFileDialog::DirectoryOnly);
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
    bool isTake = isTakeAppConInfo();
    if(isTake){
        QList<BaseDbVersionEnum> verTypes;
        QStringList verNames;
        QList<int> mvs, svs;
        QList<QByteArray*> infos;
        if(ui->chkRightType->isChecked()){
            verTypes<<BDVE_RIGHTTYPE;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_RIGHTTYPE);
            RightType::serialAllToBinary(mv,sv,ba);
            infos<<ba;
        }
        if(ui->chkRight->isChecked()){
            verTypes<<BDVE_RIGHT;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_RIGHT);
            Right::serialAllToBinary(mv,sv,ba);
            infos<<ba;
        }
        if(ui->chkGroup->isChecked()){
            verTypes<<BDVE_GROUP;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_GROUP);
            UserGroup::serialAllToBinary(mv,sv,ba);
            infos<<ba;
        }
        if(ui->chkUser->isChecked()){
            verTypes<<BDVE_USER;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_USER);
            User::serialAllToBinary(mv,sv,ba);
            infos<<ba;
        }
        if(ui->chkWS->isChecked()){
            verTypes<<BDVE_WORKSTATION;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_WORKSTATION);
            Machine::serialAllToBinary(mv,sv,ba);
            infos<<ba;
        }
        if(ui->chkPhrases->isChecked()){
            verTypes<<BDVE_COMMONPHRASE;
            QByteArray* ba = new QByteArray;
            if(!conf->serialCommonPhraseToBinary(ba)){
                myHelper::ShowMessageBoxError(tr("在捎带常用提示短语配置信息时发生错误！"));

            }
            infos<<ba;
        }
        conf->getAppCfgVersions(verTypes,verNames,mvs,svs);
        if(!trMgr.attechAppCfgInfo(verTypes,verNames,mvs,svs,infos))
            myHelper::ShowMessageBoxError(tr("写入捎带站点信息时出错，无法捎带站点信息！"));
    }

    //2、修改本地的账户缓存记录
    AccountCacheItem* acc = accCacheItems.at(ui->cmbAccount->currentIndex());
    acc->outTime = QDateTime::currentDateTime();
    acc->s_ws = rec.m_out;
    acc->d_ws = rec.m_in;
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

    //4、移除本地账户文件中创建的临时表（因为捎带信息只保存在承载的账户文件上，而在本地文件上不需要此）
    if(isTake)
        trMgr.clearTemAppCfgTable();
    accept();
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
    conf = AppConfig::getInstance();
    QHash<int,QString> phrases;
    conf->readPhases(CPPC_TRAN_IN,phrases);
    QList<int> keys = phrases.keys();
    qSort(keys.begin(),keys.end());
    for(int i = 0; i < keys.count(); ++i){
        int key = keys.at(i);
        ui->cmbComDesc->addItem(phrases.value(key),key);
    }
    ui->cmbComDesc->setCurrentIndex(-1);
    connect(ui->cmbComDesc,SIGNAL(currentIndexChanged(QString)),this,SLOT(transInPhraseSelected(QString)));

}

TransferInDialog::~TransferInDialog()
{
    delete ui;
}

void TransferInDialog::transInPhraseSelected(QString text)
{
    ui->edtInDesc->setPlainText(text);
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
    ci->s_ws = tranRec->m_out;
    ci->d_ws = tranRec->m_in;
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
    //此账户的转移状态异常，原因可能是手工人为地修改了账户的转移状态，使其在整个工作组集内的转移状态不一致
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
        tw.setItem(1,1,new QTableWidgetItem(acItem->s_ws?QString::number(acItem->s_ws->getMID()):""));
        tw.setItem(1,2,new QTableWidgetItem(acItem->s_ws?acItem->s_ws->name():tr("未知站")));
        tw.setItem(1,3,new QTableWidgetItem(acItem->inTime.toString(Qt::ISODate)));
        QVBoxLayout* lm = new QVBoxLayout;
        lm->addWidget(&title);
        lm->addWidget(&tw);
        dlg.setLayout(lm);
        dlg.resize(600,300);
        dlg.exec();
        return;
    }

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
    ui->edtMType->setText(macTypes.value(trInfo->m_out->getType()));
    ui->edtMID->setText(QString::number(trInfo->m_out->getMID()));
    ui->edtOsType->setText(osTypes.value(trInfo->m_out->osType(),tr("未知系统")));
    ui->edtMDesc->setText(trInfo->m_out->description());
    ui->btnOk->setEnabled(true);
}
