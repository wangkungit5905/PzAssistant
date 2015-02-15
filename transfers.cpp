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
#include "securitys.h"

#include "ui_transferoutdialog.h"

/////////////////////////AccountTransferUtil/////////////////////////////////////////////
AccountTransferUtil::AccountTransferUtil(bool out, QObject *parent):QObject(parent),trMode(out)
{
    appCfg = AppConfig::getInstance();
    trRec = NULL;
    db = QSqlDatabase::addDatabase("QSQLITE",TRANSFER_MANAGER_CONNSTR);
    bkDirName = DATABASE_PATH + TEM_BACKUP_DIRNAME + "/";
    if(!trMode)
        localMacs = appCfg->getAllMachines();
}

AccountTransferUtil::~AccountTransferUtil()
{
    if(db.isOpen())
        db.close();
    QSqlDatabase::removeDatabase(TRANSFER_MANAGER_CONNSTR);
    QDir bkDir(bkDirName);
    if(bkDir.exists())
        bkDir.removeRecursively();
}

/**
 * @brief 转出账户
 * @param fileName      账户文件名
 * @param desDirName    转出目录名
 * @param intent        转出意图
 * @param info          操作信息
 * @param desWS         转出目的站点
 * @param isTake        是否捎带应用配置信息
 * @param cfgs          应用配置信息
 * @return
 */
bool AccountTransferUtil::transferOut(QString fileName, QString desDirName, QString intent, QString &info,WorkStation* desWS,bool isTake,TakeAppCfgInfos* cfgs)
{
    if(!trMode)
        return false;
    //1、备份
    if(!backup(fileName)){
        info.append(tr("转出前，备份账户文件时发生错误！"));
        return false;
    }
    //2、建立连接并读取账户代码
    QString lFileName = DATABASE_PATH + fileName;
    db.setDatabaseName(lFileName);
    if(!db.open()){
        info.append(tr("转出操作时无法建立与本地账户“%1”的数据库连接！\n").arg(fileName));
        return false;
    }
    QString aCode;
    if(!getAccountCode(aCode)){
        info.append(tr("转出操作时无法读取本地账户“%1”的账户代码！\n").arg(fileName));
        return false;
    }
    //3、新建转出记录、修改本地账户缓存记录
    AccountTranferInfo trRec;
    trRec.id = 0;
    trRec.tState = ATS_TRANSOUTED;
    trRec.m_out = appCfg->getLocalStation();
    trRec.m_in = desWS;
    trRec.desc_out = intent;
    trRec.t_out = QDateTime::currentDateTime();
    if(!saveTransferRecord(&trRec)){
        info.append(tr("在往转出账户“%1”加入新转移记录时发生错误！\n").arg(fileName));
        restore(fileName);
        return false;
    }
    AccountCacheItem* aci = appCfg->getAccountCacheItem(aCode);
    aci->tState = ATS_TRANSOUTED;
    aci->s_ws=appCfg->getLocalStation();
    aci->d_ws=desWS;
    aci->outTime=QDateTime::currentDateTime();
    if(!appCfg->saveAccountCacheItem(aci)){
        info.append(tr("在为账户“%1”保存本地账户缓存记录时发生错误！").arg(aci->fileName));
        restore(aci->fileName);
        return false;
    }
    //4、捎带应用配置信息
    if(isTake && !attechAppCfgInfo(cfgs)){
        info.append(tr("捎带应用配置信息时发生错误！"));
        restore(aci->fileName);
        return false;
    }
    //5、拷贝文件
    QString sf = DATABASE_PATH + fileName;
    QString df;
    if(!desDirName.endsWith("/"))
        desDirName.append("/");
    df = desDirName + fileName;
    if(QFile::exists(df)){
        if(!myHelper::ShowMessageBoxQuesion(tr("转出目录存在同名文件“%1”，是否覆盖？").arg(fileName)) == QDialog::Rejected){
            QFile file(df);
            file.rename(df+".bak");
        }
        else
            QFile::remove(df);
    }
    if(!QFile::copy(sf,df)){
        info.append(tr("在转出操作时将账户文件“%1”拷贝到转出目录时发生错误！\n").arg(fileName));
        info.append(tr("请手工拷贝账户文件“%1”到转出目录！\n").arg(fileName));
        return false;
    }
    //6、删除备份
    QDir dir(bkDirName);
    dir.remove(fileName);
    return true;
}

/**
 * @brief 转入账户
 * @param fileName  转入账户文件名（路径名）
 * @param reason    转入说明
 * @param info      出错信息
 * @return
 */
bool AccountTransferUtil::transferIn(QString fileName, QString reason, QString &info)
{
    if(trMode)
        return false;
    //1、建立连接
    if(!QFile::exists(fileName)){
        info.append(tr("文件“%1”不存在\n").arg(fileName));
        return false;
    }
    if(!setAccontFile(fileName)){
        info.append(tr("与文件“%1”的数据库连接无法打开！\n").arg(fileName));
        return false;
    }
    //2、判断是否可以转入
    getLastTransRec();
    if(!trRec){
        info.append(tr("从文件“%1”中读取账户最近转移记录时发生错误！\n").arg(fileName));
        return false;
    }
    if(trRec->tState != ATS_TRANSOUTED){
        info.append(tr("账户“%1”未执行转出操作！\n").arg(fileName));
        return false;
    }
    //如果执行转入操作的工作站内存在对应的缓存账户，且缓存项的转移状态是已转入的目的站，则表示
    //此账户的转移状态异常，原因可能是手工人为地修改了账户的转移状态，使其在整个工作组集内的转移状态不一致
    QString accCode,accName,accLName;
    if(!getAccountBaseInfos(accCode,accName,accLName)){
        info.append(tr("转入操作时无法读取账户“%1”的账户基本信息（账户代码，简称及全称）！\n").arg(fileName));
        return false;
    }
    AccountCacheItem* acItem = appCfg->getAccountCacheItem(accCode);
    QString err;
    if(!canTransferIn(acItem,err)){
        info.append(err);
        return false;
    }
    WorkStation* locWs = appCfg->getLocalStation();
    if(trRec->m_in != locWs)
        info.append(tr("账户“%1”预定转入“%2”而不是本站，账户将不能修改！\n")
                    .arg(accName).arg(trRec->m_in->name()));
    //3、如果本地存在同名账户，则备份本地账户
    QFileInfo fi(fileName);
    QString fn = fi.fileName();
    QString lf = DATABASE_PATH + fn;
    if(QFile::exists(lf)){
        if(!backup(fn,true)){
            info.append(tr("备份文件“%1”到主备份目录时发生错误！").arg(fn));
            return false;
        }
        QDir dir(DATABASE_PATH);
        dir.remove(fn);
    }
    //4、拷贝文件到账户目录
    if(!QFile::copy(fileName,lf)){
        info.append(tr("将转入账户文件“%1”拷贝到账户目录时发生错误！").arg(fn));
        if(!restore(fn,true))
            info.append(tr("恢复原始账户文件“%1”出错！").arg(fn));
        return false;
    }
    //5、再次建立连接
    if(!setAccontFile(lf)){
        info.append(tr("与转入账户文件“%1”再次建立连接时，发生无法打开的错误！").arg(fn));
        if(!restore(fn,true))
            info.append(tr("恢复原始账户文件“%1”出错！").arg(fn));
        return false;
    }
    getLastTransRec();
    //6、如果账户带有升级配置，则执行升级
    if(!upgradeAppCfg()){
        info.append(tr("在从转入账户“%1”升级应用配置信息时，发生错误！").arg(fn));
        if(!restore(fn,true))
            info.append(tr("恢复原始账户文件“%1”出错！").arg(fn));
        return false;
    }
    //7、更改账户转移记录，和本地账户缓存记录
    if(!acItem){
        acItem = new AccountCacheItem;
        acItem->id = UNID;
        acItem->code = accCode;
        acItem->accName = accName;
        acItem->accLName = accLName;
        acItem->fileName = QFileInfo(fileName).fileName();
        acItem->lastOpened = false;
    }
    acItem->s_ws = trRec->m_out;
    acItem->d_ws = trRec->m_in;
    acItem->inTime = QDateTime::currentDateTime();
    acItem->outTime = trRec->t_out;
    if(trRec->m_in->getMID() == locWs->getMID())
        acItem->tState = ATS_TRANSINDES;
    else
        acItem->tState = ATS_TRANSINOTHER;
    if(!appCfg->saveAccountCacheItem(acItem)){
        info.append(tr("在保存账户“%1”的缓存账户条目时出错！\n").arg(accName));
        if(!restore(fn,true))
            info.append(tr("恢复原始账户文件“%1”出错！").arg(fn));
        return false;
    }
    //8、修改账户文件的转移记录
    trRec->tState=acItem->tState;
    trRec->t_in=acItem->inTime;
    trRec->desc_in=reason;
    if(!saveTransferRecord(trRec)){
        myHelper::ShowMessageBoxError(tr("在修改并保存转入账户文件的转移条目时发生错误！"));
        return false;
    }
    return true;
}

bool AccountTransferUtil::setAccontFile(QString fname)
{
    if(trMode)//仅在转入操作时调用
        return false;
    if(db.isOpen()){
        db.close();
        if(trRec){
            delete trRec;
            trRec = 0;
        }
    }
    db.setDatabaseName(fname);
    return db.open();
}

/**
 * @brief 转移账户是否存在捎带应用配置信息的临时表
 * @return
 */
bool AccountTransferUtil::isExistAppCfgInfo()
{
    QSqlQuery q(db);
    QString s = QString("select count() from sqlite_master where type='table' and name='%1'").arg(tbl_tem_appcfg);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.first();
    int c = q.value(0).toInt();
    return (c == 1);
}

/**
 * @brief 让转出的账户文件捎带执行转出操作的主机上的应用配置信息，
 *        这将在账户文件内创建临时保存配置信息的表，此方法只在执行转出操作时调用
 * @param verTypes
 * @param verNames
 * @param mvs
 * @param svs
 * @param infos
 * @return
 */
bool AccountTransferUtil::attechAppCfgInfo(TakeAppCfgInfos* cfgs)
{
    if(!trMode)
        return true;
    if(cfgs->verTypes.isEmpty())
        return true;

    bool isExistTemAppCfgTable = isExistAppCfgInfo();
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
    for(int i = 0; i < cfgs->verTypes.count(); ++i){
        q.bindValue(":type",cfgs->verTypes.at(i));
        q.bindValue(":name",cfgs->verNames.at(i));
        q.bindValue(":mv",cfgs->mvs.at(i));
        q.bindValue(":sv",cfgs->svs.at(i));
        q.bindValue(":infos",*cfgs->infos.at(i));
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
 * @brief 备份账户目录下的指定账户文件到主备份目录或临时备份目录
 * @param filename  不带路径的文件名
 * @return
 */
bool AccountTransferUtil::backup(QString filename, bool isMasteBD)
{
    if(isMasteBD){
        BackupUtil bu;
        return bu.backup(filename,BackupUtil::BR_TRANSFERIN);
    }
    else{
        QDir bkDir(bkDirName);
        if(!bkDir.exists())
            bkDir.mkpath(bkDirName);
        QString fname = DATABASE_PATH + filename;
        QString bkname = bkDirName + filename;
        if(bkDir.exists(filename))
            bkDir.remove(filename);
        return QFile::copy(fname,bkname);
    }
}

/**
 * @brief 从主备份目录或临时备份目录中恢复指定账户文件
 * @param filename
 * @return
 */
bool AccountTransferUtil::restore(QString filename, bool isMasteBD)
{
    if(isMasteBD){
        BackupUtil bu;
        QString err;
        return bu.restore(filename,BackupUtil::BR_TRANSFERIN,err);
    }
    else{
        QDir dir(DATABASE_PATH);
        if(dir.exists(filename))
            dir.remove(filename);
        QString sname = bkDirName + filename;
        QString dname = DATABASE_PATH + filename;
        return QFile::copy(dname,sname);
    }
}

bool AccountTransferUtil::saveTransferRecord(AccountTranferInfo *rec)
{
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
 * @brief 升级应用配置信息
 * @return
 */
bool AccountTransferUtil::upgradeAppCfg()
{
    if(!isExistAppCfgInfo())
        return true;
    QSqlQuery q(db);
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
        appCfg->getAppCfgVersion(bmv,bsv,verType);
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
        myHelper::ShowMessageBoxInfo(tr("导入文件“%1”附带应用配置升级信息，但版本不高于本站配置，因此无须升级！").arg(db.databaseName()));
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
                ok2 = appCfg->clearAndSaveRightTypes(rts,mv,sv);
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
                ok2 = appCfg->clearAndSaveRights(rs,mv,sv);
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
                ok2 = appCfg->clearAndSaveGroups(gs,mv,sv);
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
                ok2 = appCfg->clearAndSaveUsers(us,mv,sv);
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
            QList<WorkStation*> macs;
            ok1 = WorkStation::serialAllFromBinary(macs,mv,sv,objs.at(i));
            if(!ok1)
                myHelper::ShowMessageBoxError(tr("升级“%1”配置信息时发生错误！").arg(names.at(i)));
            else{
                int localId = appCfg->getLocalStationId();
                foreach(WorkStation* m, macs){
                    if(m->getMID() == localId)
                        m->setLocalMachine(true);
                    else
                        m->setLocalMachine(false);
                }
                ok2 = appCfg->clearAndSaveMacs(macs,mv,sv);
                if(!ok2)
                    myHelper::ShowMessageBoxError(tr("保存“%1”配置信息时发生错误！").arg(names.at(i)));
            }
            if(!ok1 || !ok2){
                if(!macs.isEmpty())
                    qDeleteAll(macs);
                return false;
            }
            else{
                appCfg->refreshMachines();
                localMacs.clear();
                localMacs = appCfg->getAllMachines();
            }
        }
        else if(verType == BDVE_COMMONPHRASE){
            if(!appCfg->serialCommonPhraseFromBinary(objs.at(i)))
                myHelper::ShowMessageBoxError(tr("升级常用提示短语时发生错误！"));
        }
    }
    q.finish();
    clearTemAppCfgTable();
    myHelper::ShowMessageBoxInfo(tr("升级操作成功完成，需要重新启动应用以使配置生效！"));
    return true;
}

/**
 * @brief 删除捎带应用配置信息的表格
 * @return
 */
bool AccountTransferUtil::clearTemAppCfgTable()
{
    QSqlQuery q(db);
    QString s = QString("drop table if exists %1").arg(tbl_tem_appcfg);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief 获取转入账户的最近的转移记录
 * @return
 */
AccountTranferInfo* AccountTransferUtil::getLastTransRec()
{
    if(trRec)
        return trRec;
    if(!db.isOpen()){
        LOG_SQLERROR("Database base connect don't opened!");
        return NULL;
    }
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_transfer);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return NULL;
    }
    if(!q.last())
        return NULL;
    trRec = new AccountTranferInfo;
    trRec->tState = (AccountTransferState)q.value(TRANS_STATE).toInt();
    int smid = q.value(TRANS_SMID).toInt();
    int dmid = q.value(TRANS_DMID).toInt();
    //检测源主机和目标主机是否在本机上存在，如果不存在，则视为异常
    if(!localMacs.contains(smid) || !localMacs.contains(dmid)){
        myHelper::ShowMessageBoxWarning(tr("转入账户的最后转移记录中包含未知的源或目标站点，请先纠正之！"));
        return NULL;
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
        return NULL;
    }
    if(q.first()){
        trRec->desc_in = q.value(TRANSDESC_IN).toString();
        trRec->desc_out = q.value(TRANSDESC_OUT).toString();
    }
    return trRec;
}

/**
 * @brief 获取账户代码
 * @param aCode
 * @return
 */
bool AccountTransferUtil::getAccountCode(QString &aCode)
{
    QString s = QString("select %1 from %2 where %3=%4").arg(fld_acci_value)
            .arg(tbl_accInfo).arg(fld_acci_code).arg(Account::ACODE);
    QSqlQuery q(db);
    if(!q.exec(s) || !q.first())
        return false;
    aCode = q.value(0).toString();
    return true;
}

bool AccountTransferUtil::getAccountBaseInfos(QString &acode, QString &sname, QString &lname)
{
    if(!db.isOpen()){
        LOG_SQLERROR("Database base connect don't opened!");
        return false;
    }
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
            acode = q.value(ACCINFO_VALUE).toString();
            break;
        case Account::SNAME:
            sname = q.value(ACCINFO_VALUE).toString();
            break;
        case Account::LNAME:
            lname = q.value(ACCINFO_VALUE).toString();
        }
    }
    return true;
}

/**
 * @brief 基于账户的最近转移记录和本地缓存条目，判断该账户是否可以转入
 * @param trRec
 * @param acItem
 * @return
 */
bool AccountTransferUtil::canTransferIn(AccountCacheItem *acItem, QString &infos)
{
    if(trMode)
        return false;
    QString name,lname;
    if(acItem){
        name = acItem->accName;
        lname = acItem->accLName;
    }
    if(!trRec){
        infos.append(tr("最近转移记录为空！\n"));
        return false;
    }
    if(trRec->tState != ATS_TRANSOUTED){
        infos.append(tr("账户未执行转出操作，不能转入！\n"));
        return false;
    }
    //为了确保账户编辑的连续性，账户的转移必须按时间顺序，以直线的方式转移
    //即账户A从站点A转出到目的站B，如果要回到站点A，必须从站点B转出，再转入站点A
    //在此期间，站点B不能接受以站点B为目的账的账户A的转入（也就是说，站点B将不再接受
    //账户A的转入，因为如果站点B对账户A做出了修改，此时再转入则将冲掉修改数据，
    //这就确保了账户A编辑动作的连续性），当然此时其他站点转入账户A只能以只读的形式。
    if(acItem && acItem->tState == ATS_TRANSINDES){
        QDialog dlg;
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
        tw.setItem(0,1,new QTableWidgetItem(trRec->m_out?QString::number(trRec->m_out->getMID()):""));
        tw.setItem(0,2,new QTableWidgetItem(trRec->m_out?trRec->m_out->name():tr("未知站")));
        tw.setItem(0,3,new QTableWidgetItem(trRec->t_out.toString(Qt::ISODate)));
        tw.setItem(1,1,new QTableWidgetItem(acItem->s_ws?QString::number(acItem->s_ws->getMID()):""));
        tw.setItem(1,2,new QTableWidgetItem(acItem->s_ws?acItem->s_ws->name():tr("未知站")));
        tw.setItem(1,3,new QTableWidgetItem(acItem->inTime.toString(Qt::ISODate)));
        QVBoxLayout* lm = new QVBoxLayout;
        lm->addWidget(&title);
        lm->addWidget(&tw);
        dlg.setLayout(lm);
        dlg.resize(600,300);
        dlg.exec();
        QHash<AccountTransferState, QString> trStateNames = appCfg->getAccTranStates();
        infos.append(tr("转入账户“%1”的转移记录异常！\n").arg(acItem->accName));
        infos.append(tr("本地账户缓存记录如下：\n"));
        infos.append(tr("源站：%1，转出时间：%2，目的站：%3，转入时间：%4，转移状态：%5\n")
                     .arg(acItem->s_ws->name()).arg(acItem->outTime.toString(Qt::ISODate))
                     .arg(acItem->d_ws->name()).arg(acItem->inTime.toString(Qt::ISODate))
                     .arg(trStateNames.value(acItem->tState)));
        infos.append(tr("账户最近转移记录如下：\n"));
        infos.append(tr("源站：%1，转出时间：%2，目的站：%3，转移状态：%4\n")
                     .arg(trRec->m_out->name()).arg(trRec->t_out.toString(Qt::ISODate))
                     .arg(trRec->m_in->name()).arg(trStateNames.value(trRec->tState)));
        return false;
    }
    return true;
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
        WorkStation* ms = conf->getMasterStation();
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
    foreach(WorkStation* mac, localMacs)
        ui->cmbMachines->addItem(mac->name());
    QString path = conf->getDirName(AppConfig::DIR_TRANSOUT);
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


WorkStation* TransferOutDialog::getDestiMachine()
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
    WorkStation* mac = localMacs.at(index);
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
    dlg.setDirectory(conf->getDirName(AppConfig::DIR_TRANSOUT));
    if(dlg.exec() == QDialog::Rejected)
        return;
    QStringList files = dlg.selectedFiles();
    if(files.empty())
        return;
    ui->edtDir->setText(files.first());
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
    AccountTransferUtil trUtil;
    AccountCacheItem* aci = accCacheItems.at(ui->cmbAccount->currentIndex());
    QString infos;
    QString desDir = ui->edtDir->text();
    WorkStation* desWs = localMacs.at(ui->cmbMachines->currentIndex());
    bool isTake = isTakeAppConInfo();
    TakeAppCfgInfos cfgs;
    if(isTake){
        if(ui->chkRightType->isChecked()){
            cfgs.verTypes<<BDVE_RIGHTTYPE;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_RIGHTTYPE);
            RightType::serialAllToBinary(mv,sv,ba);
            cfgs.infos<<ba;
        }
        if(ui->chkRight->isChecked()){
            cfgs.verTypes<<BDVE_RIGHT;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_RIGHT);
            Right::serialAllToBinary(mv,sv,ba);
            cfgs.infos<<ba;
        }
        if(ui->chkGroup->isChecked()){
            cfgs.verTypes<<BDVE_GROUP;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_GROUP);
            UserGroup::serialAllToBinary(mv,sv,ba);
            cfgs.infos<<ba;
        }
        if(ui->chkUser->isChecked()){
            cfgs.verTypes<<BDVE_USER;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_USER);
            User::serialAllToBinary(mv,sv,ba);
            cfgs.infos<<ba;
        }
        if(ui->chkWS->isChecked()){
            cfgs.verTypes<<BDVE_WORKSTATION;
            QByteArray* ba = new QByteArray;
            int mv,sv;
            conf->getAppCfgVersion(mv,sv,BDVE_WORKSTATION);
            WorkStation::serialAllToBinary(mv,sv,ba);
            cfgs.infos<<ba;
        }
        if(ui->chkPhrases->isChecked()){
            cfgs.verTypes<<BDVE_COMMONPHRASE;
            QByteArray* ba = new QByteArray;
            if(!conf->serialCommonPhraseToBinary(ba)){
                myHelper::ShowMessageBoxError(tr("在捎带常用提示短语配置信息时发生错误！"));

            }
            cfgs.infos<<ba;
        }
        conf->getAppCfgVersions(cfgs.verTypes,cfgs.verNames,cfgs.mvs,cfgs.svs);
    }
    if(!trUtil.transferOut(aci->fileName,desDir,ui->edtDesc->toPlainText(),infos,desWs,isTake,&cfgs)){
        myHelper::ShowMessageBoxError(infos);
        return;
    }
    conf->saveDirName(AppConfig::DIR_TRANSOUT,desDir);
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
    trUtil = NULL;
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
    if(trUtil)
        delete trUtil;
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
    if(!trUtil)
        return;
    if(ui->edtInDesc->toPlainText().isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("未填写转入意图！"));
        return;
    }
    QString infos;
    if(!trUtil->transferIn(fileName,ui->edtInDesc->toPlainText(),infos)){
        ui->stateTxt->appendPlainText(infos);
        ui->stateTxt->appendPlainText(tr("转入账户失败！\n"));
        ui->btnOk->setEnabled(false);
    }
    else{
        myHelper::ShowMessageBoxInfo(tr("成功转入账户！\n") + infos);
        QFileInfo fi(fileName);
        conf->saveDirName(AppConfig::DIR_TRANSIN,fi.absolutePath());
        accept();
    }
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
    trUtil = new AccountTransferUtil(false,this);
    trUtil->setAccontFile(fileName);
    QString accCode,accName,accLName;
    if(!trUtil->getAccountBaseInfos(accCode,accName,accLName)){
        myHelper::ShowMessageBoxError(tr("获取该账户基本信息，可能是无效的账户文件！"));
        return;
    }
    AccountTranferInfo* trInfo = trUtil->getLastTransRec();
    if(!trInfo){
        myHelper::ShowMessageBoxError(tr("未能获取该账户的最近转移记录！"));
        return;
    }
    if(trInfo->tState != ATS_TRANSOUTED){
        myHelper::ShowMessageBoxWarning(tr("该账户没有执行转出操作，不能转入本工作站！"));
        return;
    }
    AccountCacheItem* acItem = AppConfig::getInstance()->getAccountCacheItem(accCode);
    QString infos;
    if(!trUtil->canTransferIn(acItem,infos))
        return;
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


//////////////////////////////BatchOutputDialog////////////////////////////////
BatchOutputDialog::BatchOutputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatchOutputDialog)
{
    ui->setupUi(this);
    appCfg = AppConfig::getInstance();
    macTypes = appCfg->getMachineTypes();
    appCfg->getOsTypes(osTypes);
    accounts = appCfg->getAllCachedAccounts();
    ui->twAccounts->setRowCount(accounts.count());
    WorkStation* lm = appCfg->getLocalStation();
    AccountCacheItem* ai;
    for(int row = 0; row < accounts.count(); ++row){
        ai = accounts.at(row);
        if(ai->tState == ATS_TRANSINDES && ai->d_ws == lm){
            QCheckBox* btn = new QCheckBox(ui->twAccounts);
            ui->twAccounts->setCellWidget(row,CI_SEL,btn);
        }
        else{
            QHash<AccountTransferState, QString> states = appCfg->getAccTranStates();
            ui->twAccounts->setItem(row,CI_SEL,new QTableWidgetItem(states.value(ai->tState)));
        }
        ui->twAccounts->setItem(row,CI_CODE,new QTableWidgetItem(ai->code));
        ui->twAccounts->setItem(row,CI_NAME,new QTableWidgetItem(ai->accName));
        ui->twAccounts->setItem(row,CI_INTENT,new QTableWidgetItem(tr("转出做帐")));
    }
    QList<WorkStation*> macs = appCfg->getAllMachines().values();
    qSort(macs.begin(),macs.end(),byMacMID);
    WorkStation* locMac = appCfg->getLocalStation();
    foreach(WorkStation* mac, macs){
        if(mac == locMac)
            continue;
        QVariant v;
        v.setValue<WorkStation*>(mac);
        ui->cmbMacs->addItem(mac->name(),v);
    }
    ui->cmbMacs->setCurrentIndex(-1);
    connect(ui->cmbMacs,SIGNAL(currentIndexChanged(int)),this,SLOT(selectMacChanged(int)));
}

BatchOutputDialog::~BatchOutputDialog()
{
    delete ui;
}

void BatchOutputDialog::selectMacChanged(int index)
{
    desWs = ui->cmbMacs->currentData().value<WorkStation*>();
    ui->macId->setText(QString::number(desWs->getMID()));
    ui->macType->setText(macTypes.value(desWs->getType()));
    ui->macOsType->setText(osTypes.value(desWs->osType()));
    ui->macDesc->setText(desWs->description());
}

void BatchOutputDialog::on_btnSelDir_clicked()
{
    QFileDialog dlg;
    dlg.setFileMode(QFileDialog::DirectoryOnly);
    dlg.setNameFilter("Sqlite files(*.dat)");
    QString dirName = appCfg->getDirName(AppConfig::DIR_TRANSOUT);
    dlg.setDirectory(dirName);
    if(dlg.exec() == QDialog::Rejected)
        return;
    QStringList files = dlg.selectedFiles();
    if(files.empty())
        return;
    ui->edtDir->setText(files.first());
}

/**
 * @brief 批量转出
 */
void BatchOutputDialog::on_btnOk_clicked()
{
    QList<int> sels;
    int row = 0;
    for(int i = 0; i < accounts.count(); ++i){
        QCheckBox* btn = qobject_cast<QCheckBox*>(ui->twAccounts->cellWidget(row,CI_SEL));
        if(!btn || (btn && btn->checkState() == Qt::Unchecked)){
            row++;
            continue;
        }
        sels<<i;
        row++;
    }
    if(sels.isEmpty()){
        if(!accounts.isEmpty())
            myHelper::ShowMessageBoxInfo(tr("未选择任何账户！"));
        return;
    }
    if(ui->cmbMacs->currentIndex() == -1){
        myHelper::ShowMessageBoxWarning(tr("请选择转出目的工作站！"));
        return;
    }
    QString desDir = ui->edtDir->text();
    if(desDir.isEmpty()){
        desDir = appCfg->getDirName(AppConfig::DIR_TRANSOUT);
        ui->edtDir->setText(desDir);
    }
    ui->stateTxt->appendPlainText(tr("您选择了 %1 个账户，批量转出至文件夹“%2”\n")
                                  .arg(sels.count()).arg(desDir));
    AccountTransferUtil trUtil;
    QString infos;
    foreach(int i, sels){
        AccountCacheItem* aci = accounts.at(i);
        if(!trUtil.transferOut(aci->fileName,desDir,ui->twAccounts->item(i,CI_INTENT)->text(),infos,desWs)){
            ui->stateTxt->appendPlainText(infos);
            ui->stateTxt->appendPlainText(tr("账户“%1”转出失败！\n").arg(aci->fileName));
        }
        else
            ui->stateTxt->appendPlainText(tr("账户“%1”成功转出！\n").arg(aci->fileName));
    }
    appCfg->saveDirName(AppConfig::DIR_TRANSOUT,desDir);
    ui->btnOk->setEnabled(false);
    ui->btnCancel->setText(tr("关闭"));
}

////////////////////////////BatchImportDialog///////////////////////////////////////////
BatchImportDialog::BatchImportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatchImportDialog)
{
    ui->setupUi(this);
}

BatchImportDialog::~BatchImportDialog()
{
    delete ui;
}

void BatchImportDialog::on_btnSel_clicked()
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    dlg.setNameFilter("Sqlite files(*.dat)");
    QString path = AppConfig::getInstance()->getDirName(AppConfig::DIR_TRANSIN);
    if(path.isEmpty() || !QFileInfo::exists(path))
        path = QDir::homePath();
    dlg.setDirectory(path);
    if(dlg.exec() == QDialog::Rejected)
        return;
    ui->edtDir->setText(path);
    files = dlg.selectedFiles();
    for(int i = 0; i < files.count(); ++i){
        QFileInfo fi(files.at(i));
        QString fn = fi.baseName();
        QListWidgetItem* item = new QListWidgetItem(fn,ui->lwAccs);
    }
}

void BatchImportDialog::on_btnImp_clicked()
{
    if(files.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("您还未选择任何要导入的账户文件！"));
        return;
    }
    AccountTransferUtil trUtil(false,this);
    for(int i = 0; i < files.count(); ++i){
        QString infos;
        bool r = trUtil.transferIn(files.at(i),"",infos);
        ui->stateTxt->appendPlainText(infos);
        if(!r)
            ui->stateTxt->appendPlainText(tr("导入账户“%1”失败！\n").arg(ui->lwAccs->item(i)->text()));
        else
            ui->stateTxt->appendPlainText(tr("成功导入账户“%1”！\n").arg(ui->lwAccs->item(i)->text()));
        QApplication::processEvents();
    }
    ui->btnImp->setEnabled(false);
}
