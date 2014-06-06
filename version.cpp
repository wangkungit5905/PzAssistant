#include <QObject>
#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>
#include <QTextCodec>
#include <QDir>

#include "version.h"
#include "global.h"
#include "tables.h"
#include "utils.h"
#include "dbutil.h"
#include "configvariablenames.h"
#include "subject.h"

#include "ui_versionmanager.h"


/////////////////////////////VMBase/////////////////////////////////////////////////////
/**
 * @brief VMBase::getUpgradeVersion
 *  获取从当前账户版本升级到系统支持版本要经历的版本列表
 * @return
 */
QList<int> VMBase::getUpgradeVersion()
{
    int index = 0;
    int curVer = curMv * 100 + curSv;
    int count = versions.count();
    while(index < count){
        if(versions.at(index) == curVer)
            break;
        index++;
    }
    QList<int> verNums;
    if(index < count - 1){
        for(int i = index+1; i < count; ++i){
            verNums<< versions.at(i);
        }
    }
    return verNums;
}

/**
 * @brief VMBase::inspectVersion
 *  比较系统支持版本和当前模块版本，以判断是否需要升级
 * @return
 */
VersionUpgradeInspectResult VMBase::inspectVersion()
{
    if(!canUpgrade)
        return VUIR_CANT;
    else if(curMv == sysMv && curSv == sysSv)
        return VUIR_DONT;
    else if(curMv < sysMv || (curMv == sysMv && curSv < sysSv))
        return VUIR_MUST;
    else
        return VUIR_LOW;
}

/**
 * @brief VMBase::appendVersion
 *  添加新版本（注意：为简化，调用这个函数要保证添加的版本号顺序，从低到高依次加入）
 * @param mv
 * @param sv
 */
//void VMBase::appendVersion(int mv, int sv, UpgradeFunc upFun)
//{
//    int verNum = mv * 100 + sv;
//    versions<<verNum;
//    upgradeFuns[verNum] = upFun;
//}

/**
 * @brief VMBase::_getSysVersion
 *  获取当前系统支持的最高版本号
 */
void VMBase::_getSysVersion()
{
    int ver = versions.last();
    sysMv = ver / 100;
    sysSv = ver % 100;
}

/**
 * @brief VMBase::_inspectVerBeforeUpdate
 *  执行升级操作前的前置版本检测
 * @param mv
 * @param sv
 * @return true：前置版本匹配，false：不匹配
 */
bool VMBase::_inspectVerBeforeUpdate(int mv, int sv)
{
    if(curMv != mv && curSv != sv)
        return false;
    return true;
}



///////////////////////////////VMAccount//////////////////////////////////////////////////
/**
 * @brief VMAccount::VMAccount
 * @param filename  账户文件名
 */
void VMAccount::getAppSupportVersion(int &mv, int &sv)
{
    mv = 1;
    sv = 7;
}

VMAccount::VMAccount(QString filename):fileName(filename)
{
    if(!restoreConnect())
        return;
    //appendVersion(1,1,VMAccount::up);
    //注意：每添加一个新版本，则修改getAppSupportVersion()返回的版本号，要对应。
    appendVersion(1,2,NULL);
    appendVersion(1,3,&VMAccount::updateTo1_3);
    appendVersion(1,4,&VMAccount::updateTo1_4);
    appendVersion(1,5,&VMAccount::updateTo1_5);
    appendVersion(1,6,&VMAccount::updateTo1_6);
    appendVersion(1,7,&VMAccount::updateTo1_7);
    _getSysVersion();
    if(!_getCurVersion()){
        if(!perfectVersion()){
            LOG_ERROR(tr("在升级账户“%1”时，不能获取当前账户版本号，也不能进行版本号的归集！").arg(filename));
            canUpgrade = false;
            return;
        }
    }
    canUpgrade = true;
}

VMAccount::~VMAccount()
{
    closeConnect();
}

/**
 * @brief VMAccount::restoreConnect
 *  建立与账户数据库的连接
 * @return
 */
bool VMAccount::restoreConnect()
{
    db = QSqlDatabase::addDatabase("QSQLITE", VM_ACCOUNT);
    db.setDatabaseName(DATABASE_PATH+fileName);
    if(!db.open()){
        LOG_ERROR(tr("在升级账户“%1”时，不能打开数据库连接！").arg(fileName));
        canUpgrade = false;
        return false;
    }
    return true;
}

/**
 * @brief VMAccount::closeConnect
 *  关闭与账户数据库的连接
 */
void VMAccount::closeConnect()
{
    db.close();
    QSqlDatabase::removeDatabase(VM_ACCOUNT);
}


/**
 * @brief VMAccount::backup
 * @param fname 账户文件名
 * @return
 */
bool VMAccount::backup(QString fname)
{
    //备份文件放置在源文件的“backupAccount目录下”
    QDir backDir(DATABASE_PATH + VM_ACC_BACKDIR + "/");
    if(!backDir.exists())
        QDir(DATABASE_PATH).mkdir(VM_ACC_BACKDIR);

    QString ds = fname + VM_ACC_BACKSUFFIX;
    if(backDir.exists(ds))
        backDir.remove(ds);

    QString sfile,dfile;
    sfile = DATABASE_PATH + fname;
    dfile = DATABASE_PATH + VM_ACC_BACKDIR +"/" + ds;
    return QFile::copy(sfile,dfile);
}

/**
 * @brief VMAccount::restore
 * @param fname 账户文件名
 * @return
 */
bool VMAccount::restore(QString fname)
{
    QDir backDir(DATABASE_PATH + VM_ACC_BACKDIR + "/");
    if(!backDir.exists())
        return false;
    QString sf = fname + VM_ACC_BACKSUFFIX;
    if(!backDir.exists(sf))
        return false;
    QDir(DATABASE_PATH).remove(fname);
    sf = backDir.absoluteFilePath(sf);
    QString df = DATABASE_PATH + fname;
    if(!QFile::copy(sf,df))
        return false;
    return backDir.remove(sf);
}


bool VMAccount::perfectVersion()
{
    return true;
}

/**
 * @brief VMAccount::getSysVersion
 *  获取系统当前支持的最大版本号
 * @param mv    主版本号
 * @param sv    此版本号
 */
void VMAccount::getSysVersion(int &mv, int &sv)
{
    mv = sysMv; sv = sysSv;
}

/**
 * @brief VMAccount::getCurVersion
 *  获取当前账户的版本号
 * @param mv
 * @param sv
 * @return
 */
void VMAccount::getCurVersion(int &mv, int &sv)
{
    mv = curMv; sv = curSv;
}

/**
 * @brief VMAccount::setCurVersion
 *  设置账户的当前版本
 * @param mv
 * @param sv
 * @return
 */
bool VMAccount::setCurVersion(int mv, int sv)
{
    QSqlQuery q(db);
    QString verStr = QString("%1.%2").arg(mv).arg(sv);
    QString s = QString("update %1 set %2=%3 where %4=%5")
            .arg(tbl_accInfo).arg(fld_acci_value)
            .arg(verStr).arg(fld_acci_code).arg(Account::DBVERSION);
    if(!q.exec(s))
        return false;
    curMv = mv; curSv = sv;
    return true;
}

/**
 * @brief VMAccount::execUpgrade
 *  执行更新到指定版本的升级操作
 * @param verNum
 * @return
 */
bool VMAccount::execUpgrade(int verNum)
{
    if(!upgradeFuns.contains(verNum)){
        LOG_ERROR(tr("版本：%1 的升级函数不存在！").arg(verNum));
        return false;
    }
    UpgradeFun_Acc fun = upgradeFuns.value(verNum);
    if(!fun){
        LOG_ERROR(tr("版本：%1 是初始版本，不需要升级！").arg(verNum));
        return false;
    }
    return (this->*fun)();
}

/**
 * @brief VMAccount::appendVersion
 *  添加新版本及其对应的升级函数
 * @param mv    主版本号
 * @param sv    次版本号
 * @param upFun 升级函数
 */
void VMAccount::appendVersion(int mv, int sv, UpgradeFun_Acc upFun)
{
    int verNum = mv * 100 + sv;
    versions<<verNum;
    upgradeFuns[verNum] = upFun;
}

/**
 * @brief VMAccount::updateTo1_3
 *  升级任务：
 *  1、修改名称条目类别表“SndSubClass”，添加显示顺序列“order”，并重命名为“NameItemClass”
 *  2、创建名称条目表“nameItems”替换“SecSubjects”表，添加创建时间列，创建者
 *  3、创建新版“SndSubject”替换“FSAgent”表，添加创建（启用）时间列、创建者、禁用时间列
 *  4、新建一级科目id表（FstSubIDs）和一级科目表“FstSubs_1”，并导入默认科目系统
 *  5、添加pzMemInfos表（保存凭证的备注信息）
 *  6、修改一级科目类别表结构，增加科目系统类型字段“subSys”
 *  7、创建帐套表accountSuites，从accountInfo表内读取有关帐套的数据进行初始化
 *
 * @return
 */
bool VMAccount::updateTo1_3()
{
    int verNum = 103;
    QSqlQuery q(db);
    QString s;
    bool r,ok;

    emit startUpgrade(verNum, tr("开始更新到版本“1.3”..."));
    if(!db.transaction()){
        emit upgradeStep(verNum,tr("启动事务失败！"),VUR_ERROR);
        return false;
    }

    //1、修改名称条目类别表“SndSubClass”，添加显示顺序列“order”，并重命名为“NameItemClass”
    emit upgradeStep(verNum,tr("第一步：修改名称条目类别表“SndSubClass”，添加显示顺序列“order”，并重命名为“NameItemClass”"),VUR_OK);
    s = QString("create table %1(id INTEGER PRIMARY KEY, %2 INTEGER, %3 INTEGER, %4 TEXT, %5 TEXT)")
            .arg(tbl_nameItemCls).arg(fld_nic_order).arg(fld_nic_clscode)
            .arg(fld_nic_name).arg(fld_nic_explain);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建名称类别表“%1”时发生错误！").arg(tbl_nameItemCls),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4) select %2,%3,%4 from SndSubClass order by %2")
            .arg(tbl_nameItemCls).arg(fld_nic_clscode).arg(fld_nic_name)
            .arg(fld_nic_explain);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在转移名称类别表内容时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }
    QSqlQuery qb(AppConfig::getBaseDbConnect());
    s = QString("select * from %1 where %2<29").arg(tbl_base_nic).arg(fld_base_nic_code);
    if(!qb.exec(s)){
        emit upgradeStep(verNum,tr("从基本库中读取名称类别的显示顺序时发生错误！"),VUR_ERROR);
        return false;
    }
    s = QString("update %1 set %2=:order where %3=:code").arg(tbl_nameItemCls)
            .arg(fld_nic_order).arg(fld_nic_clscode);
    if(!q.prepare(s)){
        emit upgradeStep(verNum,tr("错误地执行插入语句“%1”").arg(s),VUR_ERROR);
        return false;
    }
    while(qb.next()){
        int code = qb.value(FI_BASE_NIC_CODE).toInt();
        int order = qb.value(FI_BASE_NIC_ORDER).toInt();
        q.bindValue(":order",order);
        q.bindValue(":code",code);
        if(!q.exec()){
            emit upgradeStep(verNum,tr("在设置名称类别的显示顺序时发生错误！"),VUR_ERROR);
            return false;
        }
    }
    s = QString("select * from %1 where %2>28").arg(tbl_base_nic).arg(fld_base_nic_code);
    if(!qb.exec(s)){
        emit upgradeStep(verNum,tr("从基本库中读取名称类别的显示顺序时发生错误！"),VUR_ERROR);
        return false;
    }

    s = QString("insert into %1(%2,%3,%4,%5) values(:order,:code,:name,:explain)")
            .arg(tbl_nameItemCls).arg(fld_nic_order).arg(fld_nic_clscode)
            .arg(fld_nic_name).arg(fld_nic_explain);
    if(!q.prepare(s)){
        emit upgradeStep(verNum,tr("错误地执行插入语句“%1”").arg(s),VUR_ERROR);
        return false;
    }
    while(qb.next()){
        int code = qb.value(FI_BASE_NIC_CODE).toInt();
        int order = qb.value(FI_BASE_NIC_ORDER).toInt();
        QString name = qb.value(FI_BASE_NIC_NAME).toString();
        QString explain = qb.value(FI_BASE_NIC_EXPLAIN).toString();
        q.bindValue(":order",order);
        q.bindValue(":code",code);
        q.bindValue(":name",name);
        q.bindValue(":explain",explain);
        if(!q.exec()){
            emit upgradeStep(verNum,tr("在设置名称类别的显示顺序时发生错误！"),VUR_ERROR);
            return false;
        }
    }
    s = "drop table SndSubClass";
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("在删除老的名称类别表时发生错误！"),VUR_ERROR);


    //2、创建名称条目表“nameItems”替换“SecSubjects”表，添加创建时间列，创建者

    emit upgradeStep(verNum,tr("第二步：创建名称条目表“nameItems”替换“SecSubjects”表，添加创建时间列，创建者"),VUR_OK);
    s = "alter table SecSubjects rename to old_SecSubjects";
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在更改“SecSubjects”表名时发生错误！"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("更改“SecSubjects”表名为“old_SecSubjects”"),VUR_OK);
    s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY, %2 text, %3 text, %4 text, "
                "%5 integer, %6 TimeStamp NOT NULL DEFAULT (datetime('now','localtime')), "
                "%7 integer)")
            .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_lname).arg(fld_ni_remcode)
            .arg(fld_ni_class).arg(fld_ni_crtTime).arg(fld_ni_creator);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("创建表“%1”失败！").arg(tbl_nameItem),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功创建表“%1”").arg(tbl_nameItem),VUR_OK);
    s = QString("insert into %1(id,sName,lName,remCode,classId,creator) "
            "select id,subName,subLName,remCode,classId,1 as user from old_SecSubjects")
            .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_lname).arg(fld_ni_remcode)
            .arg(fld_ni_class).arg(fld_ni_creator);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在从表“old_SecSubjects”转移数据到表“%1”时发生错误")
                         .arg(tbl_nameItem),VUR_ERROR);
        return false;
    }

    emit upgradeStep(verNum,tr("成功将表“old_SecSubjects”中的数据转移至表“%1”").arg(tbl_nameItem),VUR_OK);

    //进行校对
    QSqlQuery q2(db);
    int id;
    QString name,newname;
    s = "select id, subName from old_SecSubjects";
    r = q.exec(s);
    r = q2.prepare(QString("select %1 from %2 where id = :id").arg(fld_ni_name).arg(tbl_nameItem));
    while(q.next()){
        ok = true;
        id = q.value(0).toInt();
        name = q.value(1).toString();
        q2.bindValue(":id",id);
        if(!q2.exec())
            ok = false;
        if(!q2.first())
            ok = false;
        newname = q2.value(0).toString();
        if(QString::compare(name,newname) != 0)
            ok = false;
        if(!ok){
            emit upgradeStep(verNum,tr("校对表“old_SecSubjects”和“%1”数据，发现不一致！").arg(tbl_nameItem),VUR_ERROR);
            break;
        }
    }
    emit upgradeStep(verNum,tr("校对表“old_SecSubjects”和“%1”数据，数据一致！").arg(tbl_nameItem),VUR_OK);

    //在这里删除表，会出错，不知为啥？ 所有必须在第二次打开时删除
    s = "delete from old_SecSubjects";
    r = q.exec(s);
    s = "drop table old_SecSubjects";
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("不能删除表“old_SecSubjects”"),VUR_WARNING);

    //3、创建新表“SndSubject”替换“FSAgent”表，添加创建（启用）时间列、禁用时间列、创建者
    emit upgradeStep(verNum,tr("第三步：创建新表“SndSubject”替换“FSAgent”表，添加创建（启用）时间列、禁用时间列、创建者"),VUR_OK);
    s = "alter table FSAgent rename to old_FSAgent";
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在更改“FSAgent”表名时发生错误！"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("将表“FSAgent”改名为“old_FSAgent”！"),VUR_OK);

    s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY,%2 INTEGER, "
                "%3 INTEGER, %4 varchar(5),%5 INTEGER,%6 INTEGER,%7 TimeStamp, "
                "%8 TimeStamp NOT NULL DEFAULT (datetime('now','localtime')),"
                "%9 INTEGER,%10 INTEGER)")
            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid)
            .arg(fld_ssub_code).arg(fld_ssub_weight).arg(fld_ssub_enable)
            .arg(fld_ssub_disTime).arg(fld_ssub_crtTime).arg(fld_ssub_creator)
            .arg(fld_ssub_subsys);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建“%1”表时发生错误！").arg(tbl_ssub),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功创建“%1”表！").arg(tbl_ssub),VUR_OK);


    s = QString("insert into %1(id,%2,%3,%4,%5,%6,%7,%8,%9) select id,fid,sid,subCode,"
                "FrequencyStat,isEnabled,'2013-12-31' as createTime,1 as user,1 as subsys"
                " from old_FSAgent")
            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid)
            .arg(fld_ssub_code).arg(fld_ssub_weight).arg(fld_ssub_enable)
            .arg(fld_ssub_crtTime).arg(fld_ssub_creator).arg(fld_ssub_subsys);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在从表“old_FSAgent”转移数据到表“%1”时发生错误！").arg(tbl_ssub),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功将表“old_FSAgent”数据转移到表“%1”中！").arg(tbl_ssub),VUR_OK);

    //QString dateStr = AppConfig::getInstance()->getSpecSubSysItem(2)->startTime.toString(Qt::ISODate);
    //QDate date = QDate::fromString(dateStr,Qt::ISODate);
    //date.setDate(date.year()-1,12,31);
    s = QString("update %1 set %2=1,%3=1,%4='%5'").arg(tbl_ssub)
            .arg(fld_ssub_enable).arg(fld_ssub_weight).arg(fld_ssub_crtTime)
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
            //.arg(date.toString(Qt::ISODate));
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在更新表“%1”启用、权重字段时发生错误！").arg(tbl_ssub),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("更新表“%1”启用、权重字段！").arg(tbl_ssub),VUR_OK);

    //进行校对
    int fid,nfid,sid,nsid;
    s = "select id,fid,sid from old_FSAgent";
    r = q.exec(s);
    s = QString("select %1,%2 from %3 where id=:id")
            .arg(fld_ssub_fid).arg(fld_ssub_nid).arg(tbl_ssub);
    r = q2.prepare(s);
    while(q.next()){
        ok = true;
        id = q.value(0).toInt();
        fid = q.value(1).toInt();
        sid = q.value(2).toInt();
        q2.bindValue(":id",id);
        if(!q2.exec())
            ok = false;
        if(!q2.first())
            ok = false;
        nfid = q2.value(0).toInt();
        nsid = q2.value(1).toInt();
        if(fid != nfid || sid != nsid)
            ok = false;
        if(!ok){
            emit upgradeStep(verNum,tr("在比较表“old_FSAgent”和表“%1”数据时，发现不一致！").arg(tbl_ssub),VUR_ERROR);
            return false;
        }
    }
    emit upgradeStep(verNum,tr("比较表“old_FSAgent”和表“%1”，数据一致！").arg(tbl_ssub),VUR_OK);

    s = "delete from old_FSAgent";
    r = q.exec(s);
    s = "drop table old_FSAgent";
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("在删除表“old_FSAgent”表时发生错误!"),VUR_WARNING);



    //4、新建一级科目id表（FstSubIDs）和一级科目表“FstSubs_1”，并导入默认科目系统
    QString defFsubName = QString("%1%2").arg(tbl_fsub_prefix).arg(DEFAULT_SUBSYS_CODE);
    emit upgradeStep(verNum,tr("第四步：新建一级科目id表（%1）和一级科目表“%2”，并导入默认科目系统")
                     .arg(tbl_fsub_ids).arg(defFsubName),VUR_OK);
    s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY AUTOINCREMENT)").arg(tbl_fsub_ids);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建“%1”表时发生错误！").arg(tbl_fsub_ids),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("4-2 创建默认科目系统表（%1）！").arg(defFsubName),VUR_OK);
    s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY,%2 INTEGER,%3 varchar(4),"
                "%4 varchar(10), %5 integer, %6 integer, %7 integer, %8 INTEGER, "
                "%9 integer, %10 varchar(10))")
            .arg(defFsubName).arg(fld_fsub_fid).arg(fld_fsub_subcode).arg(fld_fsub_remcode)
            .arg(fld_fsub_class).arg(fld_fsub_jddir).arg(fld_fsub_isEnalbed)
            .arg(fld_fsub_isUseWb).arg(fld_fsub_weight).arg(fld_fsub_name);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建“%1”表时发生错误！").arg(defFsubName),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功创建“%1”表！").arg(defFsubName),VUR_OK);
    emit upgradeStep(verNum,tr("4-3 导入默认科目系统！"),VUR_OK);
    s = QString("select count() from FirSubjects");
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在获取默认一级科目数目时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }
    q.first();
    int fsubCount = q.value(0).toInt();
    s = QString("insert into %1 default values").arg(tbl_fsub_ids);
    for(int i = 0; i < fsubCount; ++i){
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            emit upgradeStep(verNum,tr("在插入一级科目id行时发生错误！"),VUR_ERROR);
            return false;
        }
    }
    s = QString("select * from %1").arg(tbl_fsub_ids);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在读取一级科目id时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10) "
                "values(:fid,:subCode,:remCode,:clsId,:jdDir,1,0,"
                ":weight,:subName)")
            .arg(defFsubName).arg(fld_fsub_fid).arg(fld_fsub_subcode)
            .arg(fld_fsub_remcode).arg(fld_fsub_class).arg(fld_fsub_jddir)
            .arg(fld_fsub_isEnalbed).arg(fld_fsub_isUseWb).arg(fld_fsub_weight)
            .arg(fld_fsub_name);
    if(!q2.prepare(s)){
        emit upgradeStep(verNum,tr("在导入默认一级科目时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("select * from FirSubjects where id=:id");
    QSqlQuery q3(db);
    if(!q3.prepare(s)){
        emit upgradeStep(verNum,tr("在读取默认一级科目表时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        int fid = q.value(0).toInt();
        q3.bindValue(":id",fid);
        if(!q3.exec()){
            emit upgradeStep(verNum,tr("在读取默认一级科目表时发生错误！"),VUR_ERROR);
            LOG_SQLERROR(q3.lastQuery());
            return false;
        }
        if(!q3.first()){
            emit upgradeStep(verNum,tr("在未能找到“id=%1”的默认一级科目！").arg(fid),VUR_ERROR);
            return false;
        }
        q2.bindValue(":fid",fid);
        QString subCode = q3.value(1).toString();
        q2.bindValue(":subCode",subCode);
        QString remCode = q3.value(2).toString();
        q2.bindValue(":remCode",remCode);
        int clsId = q3.value(3).toInt();
        q2.bindValue(":clsId",clsId);
        int jdDir = q3.value(4).toInt();
        q2.bindValue(":jdDir",jdDir);
        int isUseWb = q3.value(6).toInt();
        q2.bindValue(":isUseWb",isUseWb);
        int weight = q3.value(7).toInt();
        q2.bindValue(":weight",weight);
        QString name = q3.value(8).toString();
        q2.bindValue(":subName",name);
        if(!q2.exec()){
            emit upgradeStep(verNum,tr("在导入默认一级科目时发生错误！"),VUR_ERROR);
            LOG_SQLERROR(s);
            return false;
        }
    }
    emit upgradeStep(verNum, tr("成功导入默认科目系统！").arg(defFsubName),VUR_OK);

    //设置哪些科目要使用外币
    QStringList codes;    
    codes<<"1002"<<"1131"<<"2121"<<"1151"<<"2131";
    s = QString("update %1 set %2=1 where %3=:code")
            .arg(defFsubName).arg(fld_fsub_isUseWb).arg(fld_fsub_subcode);
    r = q.prepare(s);
    for(int i = 0; i < codes.count(); ++i){
        q.bindValue(":code", codes.at(i));
        if(!q.exec()){
            emit upgradeStep(verNum, tr("在初始化使用外币的科目时，发生错误！"),VUR_ERROR);
            return false;
        }
    }
    emit upgradeStep(verNum, tr("成功初始化使用外币的科目！"),VUR_OK);

    s = QString("update %1 set %2=1").arg(defFsubName).arg(fld_fsub_weight);
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("在复位一级科目的权重字段时发生错误！"),VUR_WARNING);


    //删除表
    s = "delete from FirSubjects";
    r = q.exec(s);
    s = "drop table FirSubjects";
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("在删除表“FirSubjects”表时发生错误"),VUR_WARNING);

    //5、添加pzMemInfos表（保存凭证的备注信息）
    emit upgradeStep(verNum,tr("第五步：创建凭证备注信息表！"),VUR_OK);
    s = QString("create table %1(id INTEGER PRIMARY KEY, %2 INTEGER NOT NULL, %3 TEXT)")
            .arg(tbl_pz_meminfos).arg(fld_pzmi_pid).arg(fld_pzmi_info);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建凭证备注信息表时发生错误！"),VUR_WARNING);
        return false;
    }

    //6、修改一级科目类别表结构，增加科目系统类型字段“subSys”
    emit upgradeStep(verNum,tr("第六步：修改一级科目类别表结构，增加科目系统类型字段“subSys”"),VUR_OK);
    s = "alter table FstSubClasses rename to old_FstSubClasses";
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在重命名表“FstSubClasses”时发生错误!"),VUR_WARNING);
        return false;
    }
    emit upgradeStep(verNum,tr("重命名表“FstSubClasses”到“old_FstSubClasses”！"),VUR_OK);

    s = QString("create table %1(id integer primary key, %2 integer, %3 integer, %4 text)")
            .arg(tbl_fsclass).arg(fld_fsc_subSys).arg(fld_fsc_code).arg(fld_fsc_name);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时发生错误!").arg(tbl_fsclass),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功创建表“%1”!").arg(tbl_fsclass),VUR_OK);

    s = QString("insert into %1(%2,%3,%4) select 1 as subSys,code,name from old_FstSubClasses")
            .arg(tbl_fsclass).arg(fld_fsc_subSys).arg(fld_fsc_code).arg(fld_fsc_name);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在转移表“FstSubClasses”的数据时发生错误！!"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功将表“old_FstSubClasses”数据转移到表“%1”").arg(tbl_fsclass),VUR_OK);
    s = "delete from old_FstSubClasses";
    r = q.exec(s);
    s = "drop table old_FstSubClasses";
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("在删除表“old_FstSubClasses”表时发生错误!"),VUR_WARNING);


    //7、创建帐套表accountSuites，从accountInfo表内读取有关帐套的数据进行初始化
    emit upgradeStep(verNum,tr("第七步：创建帐套表accountSuites，从accountInfo表内读取有关帐套的数据进行初始化"),VUR_OK);
    s = QString("create table %1(id integer primary key, %2 integer, %3 integer, "
                "%4 integer, %5 integer, %6 text, %7 integer, %8 integer, %9 integer)")
            .arg(tbl_accSuites).arg(fld_accs_year).arg(fld_accs_subSys)
            .arg(fld_accs_isCur).arg(fld_accs_recentMonth).arg(fld_accs_name)
            .arg(fld_accs_startMonth).arg(fld_accs_endMonth).arg(fld_accs_isClosed);

    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("创建帐套表“%1”时发生错误！").arg(tbl_accSuites),VUR_ERROR);
        return false;
    }
    //读取帐套
    s = QString("select %1 from %2 where %3=%4")
            .arg(fld_acci_value).arg(tbl_accInfo).arg(fld_acci_code).arg(Account::SUITENAME);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在从表“AccountInfo”读取帐套信息时发生错误！"),VUR_ERROR);
        return false;
    }
    if(!q.first())
        emit upgradeStep(verNum,tr("从表“AccountInfo”中未能读取帐套信息！"),VUR_WARNING);

    QStringList sl = q.value(0).toString().split(",");
    //每2个元素代表一个帐套年份与帐套名
    QList<int> sYears; QList<QString> sNames;
    for(int i = 0; i < sl.count(); i+=2){
        sYears<<sl.at(i).toInt();
        sNames<<sl.at(i+1);
    }
    s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values(:year,1,0,1,:name,1)")
            .arg(tbl_accSuites).arg(fld_accs_year).arg(fld_accs_subSys)
            .arg(fld_accs_isCur).arg(fld_accs_recentMonth).arg(fld_accs_name).arg(fld_accs_isClosed);
    r = q.prepare(s);
    for(int i = 0; i < sYears.count(); ++i){
        q.bindValue(":year",sYears.at(i));
        q.bindValue(":name",sNames.at(i));
        if(!q.exec()){
            emit upgradeStep(verNum,tr("转移帐套数据时发生错误！"),VUR_ERROR);
            return false;
        }
    }
    emit upgradeStep(verNum,tr("成功转移帐套数据！"),VUR_OK);

    s = QString("select %1 from %2 where %3=%4")
            .arg(fld_acci_value).arg(tbl_accInfo).arg(fld_acci_code).arg(Account::CSUITE);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在读取最近打开帐套数据时发生错误！"),VUR_ERROR);
        return false;
    }
    if(!q.first())
        emit upgradeStep(verNum,tr("未能读取最近打开帐套数据！"),VUR_WARNING);

    int curY = q.value(0).toInt();
    s = QString("update %1 set %2=1 where %3=%4")
            .arg(tbl_accSuites).arg(fld_accs_isCur).arg(fld_accs_year).arg(curY);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在更新最近打开帐套时发生错误！"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功更新最近打开帐套数据！"),VUR_OK);    

    s = QString("delete from %1 where %2=%3 or %2=%4").arg(tbl_accInfo)
            .arg(fld_acci_code).arg(Account::CSUITE).arg(Account::SUITENAME);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在从表“%1”中删除帐套信息时，发生错误！").arg(tbl_accInfo),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("从表“%1”中删除帐套信息！").arg(tbl_accInfo),VUR_OK);
    s = "drop table AccountInfos";
    r = q.exec(s);

    //将账户的起止时间也并入到帐套表
    int sy,sm,ey,em;
    ok = true;
    s = QString("select %1 from %2 where %3=%4")
            .arg(fld_acci_value).arg(tbl_accInfo).arg(fld_acci_code).arg(Account::STIME);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("读取账户开始时间时，发生错误！"),VUR_ERROR);
        return false;
    }
    if(!q.first()){
        emit upgradeStep(verNum,tr("遗失账户开始时间信息！"),VUR_ERROR);
        return false;
    }
    QDate d = QDate::fromString(q.value(0).toString(),Qt::ISODate);
    sy = d.year(),sm = d.month();

    s = QString("select %1 from %2 where %3=%4")
            .arg(fld_acci_value).arg(tbl_accInfo).arg(fld_acci_code).arg(Account::ETIME);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("读取账户结束时间时，发生错误！"),VUR_ERROR);
        return false;
    }
    if(!q.first()){
        emit upgradeStep(verNum,tr("遗失账户结束时间信息！"),VUR_ERROR);
        return false;
    }
    d = QDate::fromString(q.value(0).toString(),Qt::ISODate);
    ey = d.year(),em = d.month();
    //将最后一个账套设置为未关账
    s = QString("update %1 set %2=0 where %3=%4").arg(tbl_accSuites).arg(fld_accs_isClosed)
            .arg(fld_accs_year).arg(ey);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在将最后一个帐套设置为未关账时发生错误！"),VUR_ERROR);
        return false;
    }
    //只有一个帐套
    if(sy == ey){
        s = QString("update %1 set %2=%3,%4=%5 where %6=%7")
                .arg(tbl_accSuites).arg(fld_accs_startMonth).arg(sm)
                .arg(fld_accs_endMonth).arg(em).arg(fld_accs_year).arg(sy);
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("更新帐套起止月份时发生错误！"),VUR_ERROR);
            return false;
        }
        if(q.numRowsAffected() != 1){
            emit upgradeStep(verNum,tr("没有更新帐套起止月份，请查看数据库表！"),VUR_WARNING);
            ok = false;
        }
    }
    //两个或以上帐套
    else {
        s = QString("update %1 set %2=%3,%4=12 where %5=%6")
                .arg(tbl_accSuites).arg(fld_accs_startMonth).arg(sm)
                .arg(fld_accs_endMonth).arg(fld_accs_year).arg(sy);
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("更新帐套起始月份时发生错误！"),VUR_ERROR);
            return false;
        }
        if(q.numRowsAffected() != 1){
            emit upgradeStep(verNum,tr("没有更新帐套起始月份，请查看数据库表！"),VUR_WARNING);
            ok = false;
        }

        s = QString("update %1 set %2=1,%3=%4 where %5=%6")
                .arg(tbl_accSuites).arg(fld_accs_startMonth)
                .arg(fld_accs_endMonth).arg(em).arg(fld_accs_year).arg(ey);
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("更新帐套结束月份时发生错误！"),VUR_ERROR);
            return false;
        }
        if(q.numRowsAffected() != 1){
            emit upgradeStep(verNum,tr("没有更新帐套结束月份，请查看数据库表！"),VUR_WARNING);
            ok = false;
        }
    }
    //2个以上的帐套
    if(ey - sy > 1){
        s = QString("update %1 set %2=1,%3=12 where %4!=%5 and %4!=%6")
                .arg(tbl_accSuites).arg(fld_accs_startMonth)
                .arg(fld_accs_endMonth).arg(fld_accs_year).arg(sy).arg(ey);
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("更新中间帐套起止月份时发生错误！"),VUR_ERROR);
            return false;
        }
        if(q.numRowsAffected() == 0){
            emit upgradeStep(verNum,tr("没有更新中间帐套起止月份，请查看数据库表！"),VUR_WARNING);
            ok = false;
        }
    }

    //从账户信息表中移除账户起止信息
    s = QString("delete from %1 where %2=%3 or %2=%4")
            .arg(tbl_accInfo).arg(fld_acci_code).arg(Account::STIME).arg(Account::ETIME);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("从账户信息表中移除账户起止信息时发生错误"),VUR_ERROR);
        return false;
    }

    s = QString("delete from %1").arg(tbl_subWinInfo);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在删除子窗口信息表内容时发送错误"),VUR_ERROR);
        return false;
    }
    if(!db.commit()){
        emit endUpgrade(verNum,tr("提交事务失败！"),VUR_OK);
        return false;
    }
    if(setCurVersion(1,3)){
        emit endUpgrade(verNum,tr("账户文件格式成功更新到1.3版本！"),VUR_OK);
        return true;
    }
    else{
        emit endUpgrade(verNum,tr("升级过程顺利，但未能正确设置版本号！"),VUR_WARNING);
        return false;
    }
}

/**
 * @brief VMAccount::updateTo1_4
 *  任务描述：
 *  1、删除无用表
 *
 *  2、修改BankAccounts表
 *      （1）添加1个字段：nameId（银行账户对应的名称条目id）
 *      （2）并读取已有的银行账户下对应的名称信息，补齐空白
 *
 *  3、创建新余额表
 *  创建保存余额相关的表，5个新表（SEPoint、SE_PM_F,SE_MM_F,SE_PM_S,SE_MM_S）
 *  （1）余额指针表（SEPoint）
 *      包含字段年、月、币种（id，year，month，mt），唯一地指出了所属凭证年月和币种
 *  （2）所有与新余额相关的表以SE开头，表示科目余额。
 *      其中：PM指代原币新式、MM指代本币形式、F指代一级科目、S指代二级科目
 *
 *  4、修改币种表，添加字段“是否是母币”，并用账户信息表中的相应记录初始化后移除
 *
 *  5、修改Busiactions表的结构，将jMoney、dMoney字段合并为value字段
 *
 *  6、设置常用一级科目下的默认二级科目
 * @return
 */
bool VMAccount::updateTo1_4()
{
    QSqlQuery q(db);
    int verNum = 104;
    emit startUpgrade(verNum,tr("开始更新到版本“1.4”..."));
    emit upgradeStep(verNum,tr("第一步：删除无用表"),VUR_OK);
    //1、删除无用表
    QStringList tables;
    tables<<"AccountBookGroups"<<"ReportStructs"<<"ReportAdditionInfo"<<"CashDailys"
         <<"PzClasses"<<"AccountInfos"<<"old_SecSubjects"<<"old_FstSubClasses"
        <<"old_FSAgent"<<"old_FirSubjects";
    QString s;bool r;
    foreach(QString tname, tables){
        s = QString("drop table %1").arg(tname);
        if(!q.exec(s))
            emit upgradeStep(verNum,tr("无法删除表“%1”").arg(tname),VUR_WARNING);
    }

    //2、修改BankAccounts表
    //（1）添加1个字段：nameId（银行账户对应的名称条目id）
    //（2）并读取已有的银行账户下对应的名称信息，补齐空白
    emit upgradeStep(verNum,tr("第二步：修改BankAccounts表，添加1个字段：nameId"),VUR_OK);
    s = QString("alter table %1 add column %2 integer").arg(tbl_bankAcc).arg(fld_bankAcc_nameId);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在修改表“%1”时发生错误！").arg(tbl_bankAcc),VUR_ERROR);
        return false;
    }
    QHash<int,QString> bankNames;
    s = QString("select id,%1 from %2").arg(fld_bank_name).arg(tbl_bank);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在读取银行表时发生错误！"),VUR_ERROR);
        return false;
    }
    while(q.next())
        bankNames[q.value(0).toInt()] = q.value(1).toString();

    QHash<int,QString> mtNames;
    s = QString("select id,%1 from %2").arg(fld_mt_name).arg(tbl_moneyType);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在读取币种表时发生错误！"),VUR_ERROR);
        return false;
    }
    while(q.next())
        mtNames[q.value(0).toInt()] = q.value(1).toString();

    QHashIterator<int,QString> ib(bankNames),im(mtNames);
    while(ib.hasNext()){
        ib.next();
        im.toFront();
        while(im.hasNext()){
            im.next();
            s = QString("select id from %1 where %2='%3'")
                    .arg(tbl_nameItem).arg(fld_ni_name)
                    .arg(QString("%1-%2").arg(ib.value()).arg(im.value()));
            if(!q.exec(s)){
                emit upgradeStep(verNum,tr("在读取银行账户表时发生错误！"),VUR_ERROR);
                return false;
            }
            if(!q.first()){
                LOG_DEBUG(tr("未找到“%1-%2”的名称条目！").arg(ib.value()).arg(im.value()));
                emit upgradeStep(verNum,tr("未能读取到与银行账号“%1（%2）”对应的名称条目记录！").arg(ib.value()).arg(im.value()),VUR_WARNING);
                continue;
            }
            int nid = q.value(0).toInt();
            s = QString("update %1 set %2=%3 where %4=%5 and %6=%7").arg(tbl_bankAcc)
                    .arg(fld_bankAcc_nameId).arg(nid).arg(fld_bankAcc_bankId).arg(ib.key())
                    .arg(fld_bankAcc_mt).arg(im.key());
            if(!q.exec(s))
                emit upgradeStep(verNum,tr("未能更新银行账户表的名称条目字段！"),VUR_WARNING);
        }
    }


    //3、创建新余额表
    emit upgradeStep(verNum,tr("第三步：创建新余额表"),VUR_OK);
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 integer,%5 integer)")
            .arg(tbl_nse_point).arg(fld_nse_year).arg(fld_nse_month).arg(fld_nse_mt).arg(fld_nse_state);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时发生错误！").arg(tbl_nse_point),VUR_ERROR);
        return false;
    }
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 real,%5 integer)")
            .arg(tbl_nse_p_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value).arg(fld_nse_dir);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时发生错误！").arg(tbl_nse_p_f),VUR_ERROR);
        return false;
    }
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 real)")
            .arg(tbl_nse_m_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时发生错误！").arg(tbl_nse_m_f),VUR_ERROR);
        return false;
    }
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 real,%5 integer)")
            .arg(tbl_nse_p_s).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value).arg(fld_nse_dir);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时发生错误！").arg(tbl_nse_p_s),VUR_ERROR);
        return false;
    }
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 real)")
            .arg(tbl_nse_m_s).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时发生错误！").arg(tbl_nse_m_s),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功创建新余额表！"),VUR_OK);

    //4、修改币种表，添加字段“是否是母币”，并用账户信息表中的相应记录初始化后移除
    emit upgradeStep(verNum,tr("第四步：修改币种表，将本外币信息放置在此"),VUR_OK);
    s = QString("alter table %1 rename to old_%1").arg(tbl_moneyType);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在更改表“%1”名称时发生错误"),VUR_ERROR);
        return false;
    }
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 text,%5 text)")
            .arg(tbl_moneyType).arg(fld_mt_isMaster).arg(fld_mt_code)
            .arg(fld_mt_name).arg(fld_mt_sign);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("创建表“%1”失败！").arg(tbl_moneyType),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功创建表“%1”！").arg(tbl_moneyType),VUR_OK);
    s = QString("insert into %1(id,%2,%3,%4) select id,%2,%3,%4 from old_%1")
            .arg(tbl_moneyType).arg(fld_mt_code).arg(fld_mt_name).arg(fld_mt_sign);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在转移币种表数据时发生错误！"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功转移币种表数据！"),VUR_OK);
    s = QString("select id,%1 from %2 where %3=%4").arg(fld_acci_value)
            .arg(tbl_accInfo).arg(fld_acci_code).arg(Account::MASTERMT);
    if(!q.exec(s) || !q.first()){
        emit upgradeStep(verNum,tr("在从账户信息表中读取本币代码时发生错误！"),VUR_ERROR);
        return false;
    }
    int id = q.value(0).toInt();
    int mt = q.value(1).toInt();
    s = QString("update %1 set %2=1 where %3=%4").arg(tbl_moneyType)
            .arg(fld_mt_isMaster).arg(fld_mt_code).arg(mt);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在设置本币代码时发生错误！"),VUR_ERROR);
        return false;
    }
    s = QString("update %1 set %2=0 where %3!=%4").arg(tbl_moneyType)
                .arg(fld_mt_isMaster).arg(fld_mt_code).arg(mt);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在设置外币代码时发生错误！"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("正确设置了本外币设置信息"),VUR_OK);
    s = QString("delete from %1 where id=%2").arg(tbl_accInfo).arg(id);
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("在从账户信息表中移除本币设置信息时发生错误！"),VUR_WARNING);
    s = QString("delete from %1 where %2=%3").arg(tbl_accInfo).arg(fld_acci_code).arg(Account::WAIMT);
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("在从账户信息表中移除外币设置信息时发生错误！"),VUR_WARNING);
    s = QString("drop table old_%1").arg(tbl_moneyType);
    if(!q.exec(s))
        emit upgradeStep(verNum,tr("在删除表“old_%1”时发生错误！").arg(tbl_moneyType),VUR_WARNING);

    //5、修改Busiactions表的结构，将jMoney、dMoney字段合并为value字段
    s = QString("alter table %1 rename to old_%1").arg(tbl_ba);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在修改表“%1”的名字时发生错误！").arg(tbl_ba),VUR_ERROR);
        return false;
    }
    s = QString("create table %1(id integer primary key,%2 integer,%3 text,%4 integer,%5 integer,%6 integer,%7 real,%8 integer,%9 integer)")
            .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary).arg(fld_ba_fid).arg(fld_ba_sid).arg(fld_ba_mt).arg(fld_ba_value).arg(fld_ba_dir).arg(fld_ba_number);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建新的会计分录表时发生错误！"),VUR_ERROR);
        return false;
    }
    QSqlQuery q2(db);
    if(!db.transaction()){
        emit upgradeStep(verNum,tr("在转移会计分录表的数据时，启动事务失败！"),VUR_ERROR);
        return false;
    }
    s = QString("select * from old_%1").arg(tbl_ba);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在试图读取老会计分录表数据时发生错误！"),VUR_ERROR);
        return false;
    }
    int pid,fid,sid,dir,num;
    QString summary;
    Double v;
    while(q.next()){
        id = q.value(0).toInt();
        pid = q.value(1).toInt();
        if(pid == 0)
            continue;
        summary = q.value(2).toString();
        fid = q.value(3).toInt();
        sid = q.value(4).toInt();
        mt = q.value(5).toInt();
        dir = q.value(8).toInt();
        if(dir == 1)
            v = Double(q.value(6).toDouble());
        else
            v = Double(q.value(7).toDouble());
        num = q.value(9).toInt();
        s = QString("insert into %1(id,%2,%3,%4,%5,%6,%7,%8,%9) "
                    "values(%10,%11,'%12',%13,%14,%15,%16,%17,%18)")
                .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary).arg(fld_ba_fid)
                .arg(fld_ba_sid).arg(fld_ba_mt).arg(fld_ba_value).arg(fld_ba_dir)
                .arg(fld_ba_number).arg(id).arg(pid).arg(summary).arg(fid).arg(sid).arg(mt)
                .arg(v.toString2()).arg(dir).arg(num);
        if(!q2.exec(s)){
            emit upgradeStep(verNum,tr("在试图插入id=%1的记录到“%2”时发生错误！").arg(id).arg(tbl_ba),VUR_ERROR);
            LOG_SQLERROR(s);
            db.commit();
            return false;
        }
    }
    if(!q.exec(QString("delete from old_%1").arg(tbl_ba)))
        emit upgradeStep(verNum,tr("试图删除老会计分录表内记录时发生错误！"),VUR_WARNING);
    if(!q.exec(QString("drop table old_%1").arg(tbl_ba)))
        emit upgradeStep(verNum,tr("试图删除老会计分录表时发生错误！"),VUR_WARNING);

    if(!db.commit()){
        emit upgradeStep(verNum,tr("在转移会计分录表的数据时，提交事务失败"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功修改会计分录表结构！"),VUR_OK);

    //6、设置常用一级科目下的默认二级科目
    s = QString("update %1 set %2=%3 where %4=:fid and %5=:nid").arg(tbl_ssub)
            .arg(fld_ssub_weight).arg(DEFALUT_SUB_WEIGHT).arg(fld_ssub_fid).arg(fld_ssub_nid);
    if(!q.prepare(s)){
        emit upgradeStep(verNum,tr("未能执行设置常用一级科目下的默认科目!"),VUR_ERROR);
        return false;
    }
    //现金-本币
    QStringList subNames;
    QString fname,sname;
    subNames<<"现金"<<"银行存款"<<"财务费用"<<"应交税金"<<" 主营业务收入"<<"主营业务成本"
           <<"应付工资"<<"管理费用";
    int nid=0,nums=0;
    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(DEFAULT_SUBSYS_CODE);
    for(int i = 0; i < subNames.count(); ++i){
        fname = subNames.at(i);
        s = QString("select %1 from %2 where %3='%4'").arg(fld_fsub_fid).arg(tname)
                .arg(fld_fsub_name).arg(fname);
        if(!q2.exec(s) || !q2.first())
            emit upgradeStep(verNum,tr("未能找到 %1 科目").arg(fname),VUR_ERROR);
        fid = q2.value(0).toInt();
        switch(i){
        case 0:
            sname = "人民币";
            break;
        case 2:
            sname = "汇兑损益";
            break;
        case 3:
            sname = "应交增值税（进项）";
            break;
        case 4:
        case 5:
            sname = "包干费等";
            break;
        case 6:
            sname = "工资";
            break;
        case 7:
            sname = "快件费";
            break;
        }
        if(i == 1){
            s = QString("select %1.%4 from %1 join %2 on %1.%5=%2.id join %3 on %1.%6=%3.id "
                        "where %2.%7='true' and %3.%8=1").arg(tbl_bankAcc).arg(tbl_bank).arg(tbl_moneyType)
                    .arg(fld_bankAcc_nameId).arg(fld_bankAcc_bankId).arg(fld_bankAcc_mt)
                    .arg(fld_bank_isMain).arg(fld_mt_isMaster);
            if(!q2.exec(s)){
                emit upgradeStep(verNum,tr("在查找与基本户（本币）的银行账户对应的名称条目时发生错误!"),VUR_ERROR);
                continue;
            }
            if(!q2.first()){
                emit upgradeStep(verNum,tr("未找到与基本户（本币）的银行账户对应的名称条目!"),VUR_ERROR);
                continue;
            }
            nid = q2.value(0).toInt();
        }
        else{
            s = QString("select id from %1 where %2='%3'").arg(tbl_nameItem)
                    .arg(fld_ni_name).arg(sname);
            if(!q2.exec(s)){
                emit upgradeStep(verNum,tr("在查找名称条目（%1）时发生错误！").arg(sname),VUR_ERROR);
                continue;
            }
            if(!q2.first()){
                emit upgradeStep(verNum,tr("未能找到名称条目（%1）用作“%2”科目下的默认科目！")
                                 .arg(sname).arg(subNames.at(i)),VUR_WARNING);
                continue;
            }
            nid = q2.value(0).toInt();
        }
        q.bindValue(":fid", fid);
        q.bindValue(":nid", nid);
        if(!q.exec()){
            emit upgradeStep(verNum,tr("在设置%1的默认科目时发生错误!").arg(fname),VUR_ERROR);
            continue;
        }
        nums = q.numRowsAffected();
        if(nums == 0)
            emit upgradeStep(verNum,tr("未找到%1科目下的默认科目!").arg(fname),VUR_ERROR);
        if(nums > 1)
            emit upgradeStep(verNum,tr("%1科目下存在多个默认科目!").arg(fname),VUR_ERROR);
    }

    if(setCurVersion(1,4)){
        emit endUpgrade(verNum,tr("账户文件格式成功更新到1.4版本！"),VUR_OK);
        return true;
    }
    else{
        emit endUpgrade(verNum,tr("账户文件格式已更新到1.4版本，但不能正确设置版本号！"),VUR_WARNING);
        return false;
    }
}

/**
 * @brief VMAccount::updateTo1_7
 *  任务描述：
 *  1、创建转移记录表，转移记录描述表
 *  2、为了使账户可以编辑，初始化一条转移记录（在系统当前时间，由本机转出并转入到本机的转移记录）
 * @return
 */
bool VMAccount::updateTo1_7()
{
    int verNum = 107;
    QSqlQuery q(db);
    emit upgradeStep(verNum,tr("第一步：创建转移记录表，转移记录描述表！"),VUR_OK);
    QString s = QString("create table %1(id integer primary key, %2 integer, %3 integer, %4 integer, %5 text, %6 text)")
            .arg(tbl_transfer).arg(fld_trans_smid).arg(fld_trans_dmid).arg(fld_trans_state).arg(fld_trans_outTime).arg(fld_trans_inTime);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时出错！").arg(tbl_transfer),VUR_ERROR);
        return false;
    }
    s = QString("create table %1(id integer primary key, %2 integer, %3 text, %4 text)")
            .arg(tbl_transferDesc).arg(fld_transDesc_tid).arg(fld_transDesc_out).arg(fld_transDesc_in);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时出错！").arg(tbl_transferDesc),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("第二步：初始化首条转移记录！"),VUR_OK);
    int mid = AppConfig::getInstance()->getLocalMachine()->getMID();
    QString curTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,%8,%9,'%10','%11')")
            .arg(tbl_transfer).arg(fld_trans_smid).arg(fld_trans_dmid).arg(fld_trans_state)
            .arg(fld_trans_outTime).arg(fld_trans_inTime)
            .arg(mid).arg(mid).arg(ATS_TRANSINDES).arg(curTime).arg(curTime);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在初始化首条转移记录时出错！"),VUR_ERROR);
        return false;
    }
    s = "select last_insert_rowid()";
    if(!q.exec(s) || !q.first())
        return false;
    int tid = q.value(0).toInt();
    QString outDesc = QObject::tr("默认初始由本机转出");
    QString inDesc = QObject::tr("默认初始由本机转入");
    s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')")
            .arg(tbl_transferDesc).arg(fld_transDesc_tid).arg(fld_transDesc_out)
            .arg(fld_transDesc_in).arg(tid).arg(outDesc).arg(inDesc);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在记录首条转移记录描述信息时出错！"),VUR_ERROR);
        return false;
    }
    emit endUpgrade(verNum,tr("账户文件格式成功更新到1.7版本！"),VUR_OK);
    return setCurVersion(1,7);
}

/**
 * @brief VMAccount::updateTo1_8
 *  任务描述：
 *  1、将选项表示结转银行存款、应收账款和应付账款的结转凭证归为一个结转汇兑损益类别
 * @return
 */
bool VMAccount::updateTo1_8()
{
//    QSqlQuery q(db);
//    QString s = QString("update %1 set %2=%3 where %2=%4 or %2=%5 or %2=%6")
//            .arg(tbl_pz).arg(fld_pz_class).arg(Pzc_Jzhd).arg(Pzc_Jzhd_Bank)
//            .arg(Pzc_Jzhd_Ys).arg(Pzc_Jzhd_Yf);
//    int verNum = 1.5;
//    if(!db.transaction()){
//        emit upgradeStep(verNum,tr("在更新到1.5版本时，启动事务失败！"),VUR_ERROR);
//        return false;
//    }
//    q.exec(s);
//    int nums = q.numRowsAffected();
//    if(!db.commit()){
//        emit upgradeStep(verNum,tr("在更新到1.5版本时，提交事务失败！"),VUR_ERROR);
//        return false;
//    }
//    emit upgradeStep(verNum,tr("共更改 % 张凭证！").arg(nums),VUR_OK);
//    if(setCurVersion(1,6)){
//        emit upgradeStep(verNum,tr("成功更新到版本%1").arg(verNum),VUR_OK);
//        return true;
//    }
//    else{
//        emit upgradeStep(verNum,tr("成功更新到版本%1，但不能正确设置版本号！").arg(verNum),VUR_WARNING);
//        return false;
//    }
}

/**
 * @brief VMAccount::updateTo1_5
 *  任务描述：
 *  1、将余额转移到新表系中
 *  2、添加明细账视图过滤条件表（DVFilters）
 *  3、设置启用的一级科目
 *  4、删除老的余额表系
 * @return
 */
bool VMAccount::updateTo1_5()
{
    //因为读取余额的操作要分别使用由BusiUtil和DbUtil类提供的函数，所以必须关闭由VMAccount类使用的连接
    //并在操作完成后恢复
    int verNum = 105;
    emit startUpgrade(verNum,tr("开始更新到版本“1.5”..."));


    //1、将余额转移到新表系中
    closeConnect();
    DbUtil* dbUtil = new DbUtil;
    if(!dbUtil->setFilename(fileName)){
        emit upgradeStep(verNum,tr("无法建立与账户文件的数据库连接！"),VUR_ERROR);
        return false;
    }
    db = dbUtil->getDb();
    BusiUtil::init(db);

    //（1）、创建一个需要进行转移的凭证集月份列表
//    if(!db.transaction()){
//        emit upgradeStep(verNum,tr("启动事务失败！"),VUR_ERROR);
//        return false;
//    }

    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_accSuites);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在读取账户帐套时发生错误！"),VUR_ERROR);
        return false;
    }
    int year,sm,em;
    QList<int> suiteMonthes;
    while(q.next()){
        year = q.value(ACCS_YEAR).toInt();
        sm = q.value(ACCS_STARTMONTH).toInt();
        em = q.value(ACCS_ENDMONTH).toInt();
        for(;sm<=em;++sm)
            suiteMonthes<<year*100+sm;
    }
    year = suiteMonthes.first() / 100;
    sm = suiteMonthes.first() % 100;
    if(sm == 1){
        year--;
        sm = 12;
    }
    else{
        sm--;
    }
    suiteMonthes.push_front(year*100+sm); //账户的期初余额的年月
    emit upgradeStep(verNum,tr("共需要转移 %1 个凭证集的余额数据！").arg(suiteMonthes.count()),VUR_OK);

    //（2）、读取并转储
    QHash<int,Double> fes,des/*,feRs,deRs*/; //一二级科目的余额
    QHash<int,MoneyDirection> feDirs,deDirs; //一二级科目的余额方向
    int y,m;
    bool state;
    for(int i = 0; i < suiteMonthes.count();++i){
        year = suiteMonthes.at(i);
        y = year/100;
        m = year%100;
        if(!BusiUtil::readExtraByMonth2(y,m,fes,feDirs,des,deDirs)){
            emit upgradeStep(verNum,tr("在读取“%1年%2月”的原币余额时，发生错误！").arg(y).arg(m),VUR_ERROR);
            return false;
        }
        if(!dbUtil->saveExtraForPm(y,m,fes,feDirs,des,deDirs)){
            emit upgradeStep(verNum,tr("在保存“%1年%2月”的原币余额时，发生错误！").arg(y).arg(m),VUR_ERROR);
            return false;
        }
        emit upgradeStep(verNum,tr("成功转移“%1年%2月”的原币余额！").arg(y).arg(m),VUR_OK);
        fes.clear();des.clear();
        bool exist;
        if(!BusiUtil::readExtraByMonth4(y,m,fes,des,exist)){
            emit upgradeStep(verNum,tr("在读取“%1年%2月”的本币余额时，发生错误！").arg(y).arg(m),VUR_ERROR);
            return false;
        }
        if(!dbUtil->saveExtraForMm(y,m,fes,des)){
            emit upgradeStep(verNum,tr("在保存“%1年%2月”的本币余额时，发生错误！").arg(y).arg(m),VUR_ERROR);
            return false;
        }
        emit upgradeStep(verNum,tr("成功转移“%1年%2月”的本币余额！").arg(y).arg(m),VUR_OK);
        fes.clear();des.clear();feDirs.clear();deDirs.clear();
    }
    emit upgradeStep(verNum,tr("成功转移所有的余额数据！").arg(y).arg(m),VUR_OK);
    suiteMonthes.clear();

    //（3）、转移余额状态信息
    //首先依据凭证集的状态如果是结账的，则余额肯定有效，如果没有结账，
    //则再从余额表中读取余额状态，因为记录余额状态的字段是后来才添加的，先前年月的凭证集
    //不能正确反映其真实的余额状态
    //（1、处理结账的凭证集状态
    s = QString("select %1,%2 from %3 where %4=%5")
            .arg(fld_pzss_year).arg(fld_pzss_month).arg(tbl_pzsStates)
            .arg(fld_pzss_state).arg(Ps_Jzed);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("从凭证集状态表中读取结账状态的凭证集时发生错误！"),VUR_ERROR);
        return false;
    }
    while(q.next())
        suiteMonthes<<q.value(0).toInt()*100+q.value(1).toInt();

    if(!db.transaction()){
        emit upgradeStep(verNum,tr("转移余额状态时，启动事务失败！"),VUR_ERROR);
        return false;
    }
    for(int i = 0; i < suiteMonthes.count(); ++i){
        year = suiteMonthes.at(i);
        y = year/100;
        m = year%100;
        s = QString("update %1 set %2=1 where %3=%4 and %5=%6 and %7=%8")
                .arg(tbl_nse_point).arg(fld_nse_state)
                .arg(fld_nse_year).arg(y).arg(fld_nse_month)
                .arg(m).arg(fld_nse_mt).arg(1);
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("在转储%1年%2月的余额状态时发生错误！").arg(y).arg(m),VUR_ERROR);
            return false;
        }
    }
    if(!db.commit()){
        emit upgradeStep(verNum,tr("转移余额状态时，提交事务失败！"),VUR_ERROR);
        if(!db.rollback())
            emit upgradeStep(verNum,tr("转移余额状态时，回滚事务失败！"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum, tr("成功转移已结账凭证集的余额状态！"),VUR_OK);
    suiteMonthes.clear();

    //（2、处理未结账的凭证集状态
    s = QString("select %1,%2 from %3 where %4!=%5")
            .arg(fld_pzss_year).arg(fld_pzss_month).arg(tbl_pzsStates)
            .arg(fld_pzss_state).arg(Ps_Jzed);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("从凭证集状态表中读取非结账状态的凭证集时发生错误！"),VUR_ERROR);
        return false;
    }
    while(q.next())
        suiteMonthes<<q.value(0).toInt()*100+q.value(1).toInt();
    for(int i = 0; i < suiteMonthes.count(); ++i){
        year = suiteMonthes.at(i);
        y = year/100;
        m = year%100;
        s = QString("select %1 from %2 where %3=%4 and %5=%6 and %7=%8")
                .arg(fld_se_state).arg(tbl_se).arg(fld_se_year).arg(y)
                .arg(fld_se_month).arg(m).arg(fld_se_mt).arg(1);
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("读取%1年%2月的余额状态失败！").arg(y).arg(m),VUR_ERROR);
            return false;
        }
        if(!q.first())
            continue;
        state = q.value(0).toBool();
        s = QString("update %1 set %2=%3 where %4=%5 and %6=%7 and %8=%9")
                .arg(tbl_nse_point).arg(fld_nse_state).arg(state?1:0)
                .arg(fld_nse_year).arg(y).arg(fld_nse_month)
                .arg(m).arg(fld_nse_mt).arg(1);

        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("在转储%1年%2月的余额状态时发生错误！").arg(y).arg(m),VUR_ERROR);
            return false;
        }
        if(q.numRowsAffected() !=1)
            emit upgradeStep(verNum,tr("在转储%1年%2月的余额状态时，发现没有正确保存！").arg(y).arg(m),VUR_WARNING);
        upgradeStep(verNum,tr("成功转储%1年%2月的余额状态！").arg(y).arg(m),VUR_OK);
    }

    //2、添加明细账视图过滤条件表（DVFilters）
    s = QString("create table %1(id integer primary key,%2 integer,%3 integer,%4 integer,%5 integer,"
                "%6 integer,%7 integer,%8 integer, %9 text,%10 text,%11 text,%12 text)").arg(tbl_dvfilters)
            .arg(fld_dvfs_suite).arg(fld_dvfs_isDef).arg(fld_dvfs_isCur).arg(fld_dvfs_isFstSub)
            .arg(fld_dvfs_curFSub).arg(fld_dvfs_curSSub).arg(fld_dvfs_mt).arg(fld_dvfs_name)
            .arg(fld_dvfs_startDate).arg(fld_dvfs_endDate).arg(fld_dvfs_subIds);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("创建表“%1”失败").arg(tbl_dvfilters),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功创建表“%1”").arg(tbl_dvfilters),VUR_OK);


//    if(!db.commit()){
//        emit upgradeStep(verNum,tr("提交事务失败！"),VUR_ERROR);
//        if(!db.rollback())
//            emit upgradeStep(verNum,tr("回滚事务失败！"),VUR_ERROR);
//        return false;
//    }

    //3、设置启用的一级科目及其记账方向
    QStringList codes;
    codes<<"1001"<<"1002"<<"1131"<<"1133"<<"1151"<<"1301"<<"1501"<<"1502"<<"1801"
          <<"2121"<<"2131"<<"2151"<<"2171"<<"2176"<<"2181"<<"3101"<<"3131"<<"3141"
          <<"5101"<<"5301"<<"5401"<<"5402"<<"5501"<<"5502"<<"5503"<<"5601"<<"5701";
    if(!db.transaction()){
        emit upgradeStep(verNum,tr("设置启用的一级科目时，启动事务失败！"),VUR_ERROR);
        return false;
    }
    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(DEFAULT_SUBSYS_CODE);
    s = QString("update %1 set %2=0,%3=0").arg(tname).arg(fld_fsub_isEnalbed).arg(fld_fsub_jddir);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("重置表“%1”的“%2”字段失败").arg(tname).arg(fld_fsub_isEnalbed),VUR_ERROR);
        return false;
    }
    foreach(QString code, codes){
        s = QString("update %1 set %2=1 where %4='%5'")
                .arg(tname).arg(fld_fsub_isEnalbed).arg(fld_fsub_subcode).arg(code);
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("更新表“%1”的“%2”字段失败").arg(tname).arg(fld_fsub_isEnalbed),VUR_ERROR);
            return false;
        }
    }
    codes.clear();
    codes<<"1001"<<"1002"<<"1131"<<"1133"<<"1301"<<"1501"<<"1502"<<"1801"<<"2131"<<"2151"
         <<"2171"<<"3101"<<"3141"<<"5401"<<"5402"<<"5501"<<"5502"<<"5503"<<"5601"<<"5701";
    foreach(QString code, codes){
        s = QString("update %1 set %2=1 where %4='%5'")
                .arg(tname).arg(fld_fsub_jddir).arg(fld_fsub_subcode).arg(code);
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("更新表“%1”的“%2”字段失败").arg(tname).arg(fld_fsub_jddir),VUR_ERROR);
            return false;
        }
    }
    if(!db.commit()){
        emit upgradeStep(verNum,tr("设置启用的一级科目时，提交事务失败！"),VUR_ERROR);
        return false;
    }
    emit upgradeStep(verNum,tr("成功设置启用的一级科目！"),VUR_OK);

    //4、删除老的余额表系
    QStringList statments;
    statments<<"SubjectExtras"<<"SubjectExtraDirs"
               <<"detailExtras"<<"SubjectMmtExtras"
               <<"detailMmtExtras";
    foreach (QString tableName, statments){
        s = QString("delete from %1").arg(tableName);
        if(!q.exec(s))
            emit upgradeStep(verNum,tr("在清空表“%1”的数据时发生错误！").arg(tableName),VUR_ERROR);
        s = QString("drop table %1").arg(tableName);
        if(!q.exec(s))
            emit upgradeStep(verNum,tr("在删除表“%1”的数据时发生错误！").arg(tableName),VUR_ERROR);

    }

    delete dbUtil;
    //恢复连接
    if(!restoreConnect()){
        emit upgradeStep(verNum,tr("在升级到版本 %1 后无法恢复数据库连接，如果后续有升级将无法完成！").arg(verNum),VUR_ERROR);
        return false;
    }

    if(setCurVersion(1,5)){
        emit endUpgrade(verNum,"",VUR_OK);
        return true;
    }
    else{
        emit endUpgrade(verNum,tr("但无法正确设置版本号！"),VUR_OK);
        return false;
    }


}

/**
 * @brief VMAccount::updateTo1_6
 *  任务描述：
 *  1、添加配置变量表
 *  2、清空子窗口状态信息表
 * @return
 */
bool VMAccount::updateTo1_6()
{
    QSqlQuery q(db);
    int verNum = 106;
    emit startUpgrade(verNum,tr("开始更新到版本“1.6”..."));

    //1、添加配置变量表
    emit upgradeStep(verNum,tr("创建配置变量表（%1）").arg(tbl_cfgVariable),VUR_OK);
    QString s = QString("create table %1(id integer primary key, %2 text, %3 text)")
            .arg(tbl_cfgVariable).arg(fld_cfgv_name).arg(fld_cfgv_value);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("创建配置变量表（%1）时出错！").arg(tbl_cfgVariable),VUR_ERROR);
        return false;
    }
    //为默认科目系统设置该科目系统已导入的配置变量（subSysImport_1）
//    s = QString("insert into %1(%2,%3) values('%4',1)").arg(tbl_cfgVariable)
//            .arg(fld_cfgv_name).arg(fld_cfgv_value).arg(QString("%1_%2")
//            .arg(CFG_SUBSYS_IMPORT_PRE).arg(DEFAULT_SUBSYS_CODE));
//    if(!q.exec(s)){
//        emit upgradeStep(verNum,tr("为默认科目系统设置该科目系统已导入的配置变量时出错！"),VUR_ERROR);
//        return false;
//    }

    //初始化账户配置变量
//    QString vname = QString("%1_%2").arg(CFG_SUBSYS_IMPORT_PRE).arg(DEFAULT_SUBSYS_CODE);
//    s = QString("insert into %1(%2,%3) values('%4',1)").arg(tbl_cfgVariable)
//            .arg(fld_cfgv_name).arg(fld_cfgv_value).arg(vname);
//    if(!q.exec(s)){
//        emit upgradeStep(verNum,tr("在初始化默认科目系统的导入状态时出错"),VUR_ERROR);
//        return false;
//    }

    //2、清空子窗口状态信息表
    s = QString("delete from %1").arg(tbl_subWinInfo);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("清空子窗口状态信息表（%1）时出错！").arg(tbl_subWinInfo),VUR_ERROR);
        return false;
    }
    endUpgrade(verNum,tr("成功升级到版本1.6"),VUR_OK);
    return setCurVersion(1,6);
}

/**
 * @brief VMAccount::updateTo2_0
 *  任务描述：
 *  1、
 *  2、导入新科目系统的科目（此升级任务已作为科目配置功能的一部分，将其数据库操作部分归并到dbutil类中）
 * @return
 */
bool VMAccount::updateTo2_0()
{
//    QSqlQuery q(db);
//    QString s;

//    //2、导入新科目系统的科目
//    QSqlDatabase ndb = QSqlDatabase::addDatabase("QSQLITE","importNewSub");
//    ndb.setDatabaseName("./datas/basicdatas/firstSubjects_2.dat");
//    if(!ndb.open()){
//        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("不能打开新科目系统2的数据源表！，请检查文件夹下“datas/basicdatas/”下是否存在“firstSubjects_2.dat”文件"));
//        return false;
//    }
//    QSqlQuery qm(ndb);
//    if(!qm.exec("select * from FirstSubs")){
//        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在提取新科目系统的数据时出错"));
//        return false;
//    }
//    //(id,subCode,remCode,belongTo,jdDir,isView,isReqDet,weight,subName)
//    s = "insert into FirSubjects(subSys,subCode,remCode,belongTo,jdDir,isView,isUseWb,weight,subName) "
//            "values(2,:code,:remCode,:belongTo,:jdDir,:isView,:isUseWb,:weight,:name)";
//    bool r = db.transaction();
//    r = q.prepare(s);
//    while(qm.next()){
//        q.bindValue(":code",qm.value(2).toString());
//        q.bindValue("remCode",qm.value(3).toString());
//        q.bindValue(":belongTo",qm.value(4).toInt());
//        q.bindValue(":jdDir",qm.value(5).toInt());
//        q.bindValue(":isView",qm.value(6).toInt());
//        q.bindValue(":isUseWb",qm.value(7).toInt());
//        q.bindValue(":weight",qm.value(8).toInt());
//        q.bindValue(":name",qm.value(9).toString());
//        q.exec();
//    }
//    if(!db.commit()){
//        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在导入新科目时，提交事务失败！"));
//        return false;
//    }
//    if(!qm.exec("select * from FirstSubCls where subCls=2")){
//        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在提取新科目系统科目类别的数据时出错"));
//        return false;
//    }
//    s = "insert into FstSubClasses(subSys,code,name) values(2,:code,:name)";
//    db.transaction();
//    r = q.prepare(s);
//    while(qm.next()){
//        q.bindValue(":code",qm.value(2).toInt());
//        q.bindValue(":name",qm.value(3).toString());
//        q.exec();
//    }
//    if(!db.commit()){
//        QMessageBox::critical(0,QObject::tr("更新错误"),QObject::tr("在导入新科目类别时，提交事务失败！"));
//        return false;
//    }

//    QMessageBox::information(0,QObject::tr("更新成功"),QObject::tr("账户文件格式成功更新到1.5版本！"));
//    return setCurVersion(1,5);
}




/////////////////////////////////VMAppConfig//////////////////////////////////////////
VMAppConfig::VMAppConfig(QString fileName)
{
    db = QSqlDatabase::addDatabase("QSQLITE", VM_BASIC);
    db.setDatabaseName(BASEDATA_PATH + fileName);
    if(!db.open()){
        LOG_ERROR(tr("在升级配置模块时，不能打开与基本库的数据库连接！"));
        canUpgrade = false;
        return;
    }
    appendVersion(1,0,NULL);
    appendVersion(1,1,&VMAppConfig::updateTo1_1);
    appendVersion(1,2,&VMAppConfig::updateTo1_2);
    appendVersion(1,3,&VMAppConfig::updateTo1_3);
    appendVersion(1,4,&VMAppConfig::updateTo1_4);
    appendVersion(1,5,&VMAppConfig::updateTo1_5);
    appendVersion(1,6,&VMAppConfig::updateTo1_6);
    _getSysVersion();
    if(!_getCurVersion()){
        if(!perfectVersion()){
            LOG_ERROR(tr("在升级配置模块时，不能获取基本库的当前版本号，也不能归集版本到初始版本！"));
            canUpgrade = false;
            return;
        }
    }
    appIni = new QSettings("./config/app/appSetting.ini", QSettings::IniFormat);
    //appIni->setIniCodec(QTextCodec::codecForTr());
    appIni->setIniCodec(QTextCodec::codecForLocale());
    if(!appIni->isWritable()){
        LOG_ERROR(tr("在升级配置模块时，系统配置文件“config/app/appSetting.ini”不可写！"));
        canUpgrade = false;
        return;
    }
    canUpgrade = true;
}

VMAppConfig::~VMAppConfig()
{
    db.close();
    QSqlDatabase::removeDatabase(VM_BASIC);
    appIni->sync();
    delete appIni;
}

bool VMAppConfig::backup(QString fname)
{
    return true;
}

bool VMAppConfig::restore(QString fname)
{
    return true;
}

/**
 * @brief VMAppConfig::perfectVersion
 *  归集到初始版本 1.0
 * @return
 */
bool VMAppConfig::perfectVersion()
{
    QSqlQuery q(db);
    //检测是否存在version表（这是初始版本的标记）
    QString s = "select name from sqlite_master where name='version'";
    if(!q.exec(s))
        return false;
    if(q.first())
        return true;

    //添加version表，并插入一条初始版本号记录
    s = "create table version(id INTEGER PRIMARY KEY, master INTEGER, second INTGER)";
    if(!q.exec(s))
        return false;
    s = "insert into version(master,second) values(1,0)";
    if(!q.exec(s))
        return false;
    return true;
}

void VMAppConfig::getSysVersion(int &mv, int &sv)
{
    mv = sysMv; sv = sysSv;
}

void VMAppConfig::getCurVersion(int &mv, int &sv)
{
    mv = curMv; sv = curSv;
}

bool VMAppConfig::setCurVersion(int mv, int sv)
{
    QSqlQuery q(db);
    QString s = QString("update version set master=%1,second=%2").arg(mv).arg(sv);
    return q.exec(s);
}

/**
 * @brief VMAppConfig::execUpgrade
 *  执行更新到指定版本的升级操作
 * @param verNum
 * @return
 */
bool VMAppConfig::execUpgrade(int verNum)
{
    if(!upgradeFuns.contains(verNum)){
        LOG_ERROR(tr("版本：%1 的升级函数不存在！").arg(verNum));
        return false;
    }
    UpgradeFun_Config fun = upgradeFuns.value(verNum);
    if(!fun){
        LOG_ERROR(tr("版本：%1 是初始版本，不需要升级！").arg(verNum));
        return false;
    }
    return (this->*fun)();
}

/**
 * @brief VMAppConfig::appendVersion
 *  添加新版本及其对应的升级函数
 * @param mv    主版本号
 * @param sv    次版本号
 * @param upFun 升级函数
 */
void VMAppConfig::appendVersion(int mv, int sv, UpgradeFun_Config upFun)
{
    int verNum = mv * 100 + sv;
    versions<<verNum;
    upgradeFuns[verNum] = upFun;
}

/**
 * @brief VMAppConfig::_getCurVersion
 *  读取当前版本
 * @return
 */
bool VMAppConfig::_getCurVersion()
{
    QSqlQuery q(db);
    QString s = "select master, second from version";
    if(!q.exec(s))
        return false;
    if(!q.first())
        return false;
    curMv = q.value(0).toInt();
    curSv = q.value(1).toInt();
    return true;
}

/**
 * @brief VMAppConfig::updateTo1_1
 *  升级任务描述：
 *  1、用新的简化版凭证集状态替换原先相对复杂的
 * @return
 */
bool VMAppConfig::updateTo1_1()
{
    QSqlQuery q(db);
    QString s = QString("delete from %1").arg(tbl_pzsStateNames);
    if(!q.exec(s))
        return false;

    QList<PzsState> codes;
    QHash<PzsState,QString> snames,lnames;
    codes<<Ps_NoOpen<<Ps_Rec<<Ps_AllVerified<<Ps_Jzed;
    snames[Ps_NoOpen] = QObject::tr("未打开");
    snames[Ps_Rec] = QObject::tr("录入态");
    snames[Ps_AllVerified] = QObject::tr("入账态");
    snames[Ps_Jzed] = QObject::tr("结转");
    lnames[Ps_NoOpen] = QObject::tr("未打开凭证集");
    lnames[Ps_Rec] = QObject::tr("正在录入凭证");
    lnames[Ps_AllVerified] = QObject::tr("凭证都已审核，可以进行统计并保存余额");
    lnames[Ps_Jzed] = QObject::tr("已结账，不能做出任何影响凭证集数据的操作");

    if(!db.transaction())
        return false;
    for(int i = 0; i < codes.count(); ++i){
        PzsState code = codes.at(i);
        s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')")
                .arg(tbl_pzsStateNames).arg(fld_pzssn_code).arg(fld_pzssn_sname)
                .arg(fld_pzssn_lname).arg(code).arg(snames.value(code))
                .arg(lnames.value(code));
        q.exec(s);
    }
    if(!db.commit())
        return true;
    return setCurVersion(1,1);

}

/**
 * @brief VMAppConfig::updateTo1_2
 *  添加日志级别的设置字段
 * @return
 */
bool VMAppConfig::updateTo1_2()
{
    appIni->beginGroup("Debug");
    appIni->setValue("loglevel", Logger::levelToString(Logger::Debug));
    appIni->endGroup();
    return setCurVersion(1,2);
}

/**
 * @brief VMAppConfig::updateTo1_4
 *  1、在基本库中添加machines表，并初始化5个默认主机
 *  2、根据用户的选择设置isLacal字段（配置本机是哪个主机标识）
 * @return
 */
bool VMAppConfig::updateTo1_4()
{
    int verNum = 140;
    QSqlQuery q(db);
    QHash<int,QString> snames;
    QHash<int,QString> lnames;
    snames[101] = QObject::tr("本家PC");
    lnames[101] = QObject::tr("家里的桌面电脑");
    snames[102] = QObject::tr("章莹笔记本");
    lnames[102] = QObject::tr("章莹的联想笔记本");
    snames[103] = QObject::tr("单位电脑");
    lnames[103] = QObject::tr("单位的桌面电脑");
    snames[104] = QObject::tr("我的笔记本（linux）");
    lnames[104] = QObject::tr("我的笔记本上的linux系统");
    snames[105] = QObject::tr("我的笔记本（Windows XP）");
    lnames[105] = QObject::tr("我的笔记本上的Windows XP系统");

    QList<int> mids = snames.keys();
    QStringList names;
    for(int i = 0; i < mids.count(); ++i)
        names<<snames.value(mids.at(i));
    bool ok;
    QString name = QInputDialog::getItem(0,QObject::tr("信息输入框"),
                          QObject::tr("选择一个匹配你当前运行主机的主机名："),
                          names,0,false,&ok);
    if(!ok)
        return false;

    int index = names.indexOf(name);
    int mid = mids.at(index);        //选择的主机标识
    QHashIterator<int,QString> it(snames);
    QString s = QString("create table %1(id integer primary key, %2 integer, %3 integer, %4 integer, %5 text, %6 text)")
            .arg(tbl_machines).arg(fld_mac_mid).arg(fld_mac_type).arg(fld_mac_islocal)
            .arg(fld_mac_sname).arg(fld_mac_desc);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时出错！").arg(tbl_machines),VUR_ERROR);
        return false;
    }
    it.toFront();
    while(it.hasNext()){
        it.next();
        s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,1,0,'%8','%9')")
                .arg(tbl_machines).arg(fld_mac_mid).arg(fld_mac_type).arg(fld_mac_islocal)
                .arg(fld_mac_sname).arg(fld_mac_desc).arg(it.key())
                .arg(it.value()).arg(lnames.value(it.key()));
        if(!q.exec(s)){
            emit upgradeStep(verNum,tr("在将初始的机器列表插入到“%1”时出错！").arg(tbl_machines),VUR_ERROR);
            return false;
        }
    }
    s = QString("update %1 set %2=1 where %3=%4").arg(tbl_machines)
            .arg(fld_mac_islocal).arg(fld_mac_mid).arg(mid);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在设置本机标识时出错！"),VUR_ERROR);
        return false;
    }
    endUpgrade(verNum,"",VUR_OK);
    return setCurVersion(1,4);
}

/**
 * @brief VMAppConfig::updateTo1_5
 *  1、创建特定科目配置表（特定科目代码配置表），并初始化默认科目系统的特定科目配置
 * @return
 */
bool VMAppConfig::updateTo1_5()
{
    int verNum = 150;
    QSqlQuery q(db);
    QString s = QString("create table %1(id INTEGER PRIMARY KEY, %2 INTEGER,%3 INTEGER,%4 TEXT)")
            .arg(tbl_base_sscc).arg(fld_base_sscc_subSys).arg(fld_base_sscc_enum).arg(fld_base_sscc_code);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在创建表“%1”时发生错误！").arg(tbl_base_sscc),VUR_ERROR);
        return false;
    }
    if(!db.transaction()){
        emit upgradeStep(verNum,tr("启动事务失败"),VUR_ERROR);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4) values(:subsys,:enum,:code)")
            .arg(tbl_base_sscc).arg(fld_base_sscc_subSys).arg(fld_base_sscc_enum).arg(fld_base_sscc_code);
    if(!q.prepare(s)){
        emit upgradeStep(verNum,tr("错误执行SQL语句：“%1”！").arg(s),VUR_ERROR);
        return false;
    }
    QStringList codes;
    codes<<"1001"<<"1002"<<"1501"<<"5503"<<"3131"<<"3141"<<"1131"<<"2121";
    for(int i = 1; i < 9; ++i){
        q.bindValue(":subsys", DEFAULT_SUBSYS_CODE);
        q.bindValue(":enum",i);
        q.bindValue(":code", codes.at(i-1));
        if(!q.exec()){
            emit upgradeStep(verNum,tr("在初始化默认科目系统的特定科目配置信息时发生错误！"),VUR_ERROR);
            return false;
        }
    }
    if(!db.commit()){
        emit upgradeStep(verNum,tr("提交事务失败"),VUR_ERROR);
        db.rollback();
    }
    return setCurVersion(1,5);
}

/**
 * @brief
 * 1、移除本地缓存中科目系统2的所有科目，并从指定文件中更新科目系统2
 * 2、设置老科目系统的记账方向配置信息
 * 3、建立从默认科目系统到新科目系统的对接科目表并初始化
 * @return
 */
bool VMAppConfig::updateTo1_6()
{
    QSqlQuery q(db);
    int verNum = 106;
    emit startUpgrade(verNum, tr("开始更新到版本“1.6”..."));

    //1、移除本地缓存中科目系统2的所有科目，并从指定文件中更新科目系统2
    //（1）、移除原先的科目系统2的所有信息
    QString s = QString("delete from %1 where %2=2").arg(tbl_base_fsub_cls)
            .arg(fld_base_fsub_cls_subSys);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在从本地科目缓存中移除新科目系统类别时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }

    s = QString("delete from %1 where %2=2").arg(tbl_base_fsub)
                .arg(fld_base_fsub_subsys);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在从本地科目缓存中移除新科目系统时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }

    s = QString("delete from %1 where %2=2").arg(tbl_base_sscc).arg(fld_base_sscc_subSys);
    if(!q.exec(s)){
        emit upgradeStep(verNum,tr("在从本地科目缓存中移除新科目系统的特定科目配置信息时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }

    //（2）、从数据库文件“firstSubjects_2.dat”中读取新科目系统2，并导入到本地缓存
    QString connName = "importNewSub";
    QSqlDatabase ndb = QSqlDatabase::addDatabase("QSQLITE",connName);
    QString fileName = BASEDATA_PATH + "firstSubjects_2.dat";
    ndb.setDatabaseName(fileName);
    if(!ndb.open()){
        emit upgradeStep(verNum,tr("无法打开包含新科目系统的数据库连接，文件名为“%1”！").arg(fileName),VUR_ERROR);
        return false;
    }

    //（1、导入科目类别表
    QSqlQuery qm(ndb);
    s = QString("select * from FirstSubCls where subCls=2");
    if(!qm.exec(s)){
        upgradeStep(verNum,tr("在提取新科目系统的数据时出错"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }

    if(!db.transaction()){
        upgradeStep(verNum,tr("启动事务出错"),VUR_ERROR);
        return false;
    }
    s= QString("insert into %1(%2,%3,%4) values(2,:code,:name)").arg(tbl_base_fsub_cls)
            .arg(fld_base_fsub_cls_subSys).arg(fld_base_fsub_cls_clsCode)
            .arg(fld_base_fsub_cls_name);

    if(!q.prepare(s)){
        upgradeStep(verNum,tr("错误地执行Sql语句（%1）").arg(s),VUR_ERROR);
        return false;
    }
    while(qm.next()){
        q.bindValue(":code",qm.value(FI_BASE_FSUB_CLS_CODE).toInt());
        q.bindValue(":name",qm.value(FI_BASE_FSUB_CLS_NAME).toString());
        if(!q.exec()){
            if(!qm.exec(s)){
                upgradeStep(verNum,tr("在导入科目类别表时发生错误！"),VUR_ERROR);
                return false;
            }
        }
    }

    //（2、导入一级科目表
    s = QString("select * from FirstSubs where subCls=2 order by subCode");
    if(!qm.exec(s)){
        upgradeStep(verNum,tr("在读取一级科目表时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12) "
                "values(2,:subCode,:remCode,:cls,:jddir,:isview,:isUseWb,:weight,:subname,:desc,:util)")
            .arg(tbl_base_fsub).arg(fld_base_fsub_subsys).arg(fld_base_fsub_subcode)
            .arg(fld_base_fsub_remcode).arg(fld_base_fsub_subcls).arg(fld_base_fsub_jddir)
            .arg(fld_base_fsub_isenabled).arg(fld_base_fsub_isUseWb).arg(fld_base_fsub_weight)
            .arg(fld_base_fsub_subname).arg(fld_base_fsub_desc).arg(fld_base_fsub_util);
    if(!q.prepare(s)){
        upgradeStep(verNum,tr("错误地执行Sql语句（%1）").arg(s),VUR_ERROR);
        return false;
    }

    while(qm.next()){
        q.bindValue(":subCode",qm.value(FI_BASE_FSUB_SUBCODE).toString());
        q.bindValue(":remCode",qm.value(FI_BASE_FSUB_REMCODE).toString());
        q.bindValue(":cls",qm.value(FI_BASE_FSUB_CLS).toInt());
        q.bindValue(":jddir",qm.value(FI_BASE_FSUB_JDDIR).toInt());
        q.bindValue(":isview",qm.value(FI_BASE_FSUB_ENABLE).toInt());
        q.bindValue(":isUseWb",qm.value(FI_BASE_FSUB_USEDWB).toInt());
        q.bindValue(":weight",qm.value(FI_BASE_FSUB_WEIGHT).toInt());
        q.bindValue(":subname",qm.value(FI_BASE_FSUB_SUBNAME).toString());
        q.bindValue(":desc",qm.value(FI_BASE_FSUB_DESC).toString());
        q.bindValue(":util",qm.value(FI_BASE_FSUB_UTILS).toString());
        if(!q.exec()){
            upgradeStep(verNum,tr("在导入一级科目表时发生错误！"),VUR_ERROR);
            return false;
        }
    }

    //（3、导入特定科目配置信息
    s = QString("select * from %1 where %2=2 order by %3")
            .arg(tbl_base_sscc).arg(fld_base_sscc_subSys).arg(fld_base_sscc_enum);
    if(!qm.exec(s)){
        upgradeStep(verNum,tr("在读取特定科目配置表时发生错误！"),VUR_ERROR);
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4) values(2,:subEnum,:code)")
            .arg(tbl_base_sscc).arg(fld_base_sscc_subSys).arg(fld_base_sscc_enum)
            .arg(fld_base_sscc_code);
    if(!q.prepare(s)){
        upgradeStep(verNum,tr("错误地执行Sql语句（%1）").arg(s),VUR_ERROR);
        return false;
    }
    while(qm.next()){
        AppConfig::SpecSubCode subEnum = (AppConfig::SpecSubCode)qm.value(FI_BASE_SSCC_ENUM).toInt();
        QString code = qm.value(FI_BASE_SSCC_CODE).toString();
        q.bindValue(":subEnum", subEnum);
        q.bindValue(":code", code);
        if(!q.exec()){
            upgradeStep(verNum,tr("在导入特定科目配置表时发生错误！"),VUR_ERROR);
            return false;
        }
    }

    //2、设置老科目系统的记账方向配置信息

    //3、建立从默认科目系统到新科目系统的对接科目表并初始化
    QString tableName = QString("%1_1_2").arg(tbl_base_subsysjion_pre);
    s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY, %2 TEXT, %3 TEXT, %4 INTEGER)")
            .arg(tableName).arg(fld_base_ssjc_scode).arg(fld_base_ssjc_dcode).arg(fld_base_ssjc_isDef);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        upgradeStep(verNum,tr("在创建科目系统对接科目表时发生错误！"),VUR_ERROR);
        return false;
    }
    QStringList sCodes,dCodes;
    QList<int> isDefs;
    sCodes<<"1001"<<"1002"<<"1131"<<"1133"<<"1151"<<"1301"<<"1501"<<"1502"<<"1801"<<"2121"<<"2131"<<"2151"<<"2171"<<"2176"<<"2181"<<"3101"<<"3131"<<"3141"<<"5101"<<"5301"<<"5401"<<"5402"<<"5501"<<"5502"<<"5503"<<"5601"<<"5701";
    dCodes<<"1001"<<"1002"<<"1122"<<"1221"<<"1123"<<"1123"<<"1601"<<"1602"<<"1701"<<"2202"<<"2203"<<"2211"<<"2221"<<"2241"<<"2241"<<"4001"<<"4103"<<"4104"<<"6001"<<"6301"<<"6401"<<"6403"<<"6601"<<"6602"<<"6603"<<"6711"<<"6801";
    s = QString("insert into %1(%2,%3,%4) values(:scode,:dcode,:isDef)")
            .arg(tableName).arg(fld_base_ssjc_scode).arg(fld_base_ssjc_dcode)
            .arg(fld_base_ssjc_isDef);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        upgradeStep(verNum,tr("在初始化科目系统对接科目表时发生错误！"),VUR_ERROR);
        return false;
    }
    for(int i = 0; i < sCodes.count(); ++i){
        QString sc = sCodes.at(i);
        QString dc = dCodes.at(i);
        q.bindValue(":scode",sc);
        q.bindValue(":dcode",dc);
        //（1151（预付）-1301（待摊费用））->1123预付账款
        //（2176(其他应交款)-2181（其他应付款））->2241其他应付款
        if(((sc == "1301") && (dc == "1123")) || ((sc == "2176") && (dc == "2241")))
            q.bindValue(":isDef",0);
        else
            q.bindValue(":isDef",1);
        if(!q.exec()){
            LOG_SQLERROR(s);
            upgradeStep(verNum,tr("在初始化科目系统对接科目表时发生错误！"),VUR_ERROR);
            return false;
        }
    }
    if(!db.commit()){
        upgradeStep(verNum,tr("在导入新科目系统时的提交事务时发生错误！"),VUR_ERROR);
        return false;
    }
    endUpgrade(verNum,"",VUR_OK);
    QSqlDatabase::removeDatabase(connName);
    return setCurVersion(1,6);
}

bool VMAppConfig::updateTo1_7()
{

}

/**
 * @brief VMAppConfig::updateTo1_3
 *  1、在基本库中添加本地账户缓存表，替换原先的AccountInfos表
 *  2、删除原先的本地账户缓存信息表“AccountInfos”
 *  3、移除全局变量“RecentOpenAccId”
 *  4、添加科目系统名称表
 * @return
 */
bool VMAppConfig::updateTo1_3()
{
    QSqlQuery q(db);
    int verNum = 103;
    emit startUpgrade(verNum, tr("开始更新到版本“1.3”..."));
    emit upgradeStep(verNum,tr("第一步：在基本库中添加本地账户缓存表"),VUR_OK);
    QString s = QString("create table %1(id integer primary key,%2 text,%3 text,%4 text,"
                "%5 text,%6 integer,%7 integer,%8 text,%9 integer,%10 text)")
            .arg(tbl_localAccountCache).arg(fld_lac_code).arg(fld_lac_name)
            .arg(fld_lac_lname).arg(fld_lac_filename).arg(fld_lac_isLastOpen)
            .arg(fld_lac_tranState).arg(fld_lac_tranInTime).arg(fld_lac_tranOutMid)
            .arg(fld_lac_tranOutTime);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        emit upgradeStep(verNum,tr("创建本地账户缓存表“%1”时发生错误！").arg(tbl_localAccountCache),VUR_ERROR);
        return false;
    }

    emit upgradeStep(verNum,tr("第二步：删除原先的本地账户缓存信息表“AccountInfos”"),VUR_OK);
    s = QString("drop table AccountInfos");
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        emit upgradeStep(verNum,tr("在删除原先的本地账户缓存信息表“AccountInfos”时发生错误！"),VUR_ERROR);
        return false;
    }

    //应用配置变量表已经改变，以下代码不能执行
//    emit upgradeStep(verNum,tr("第三步：移除全局变量“RecentOpenAccId”"),VUR_OK);
//    s = QString("delete from %1 where %2='RecentOpenAccId'")
//            .arg(tbl_base_Cfg).arg(fld_bconf_name);
//    if(!q.exec(s)){
//        LOG_SQLERROR(s);
//        emit upgradeStep(verNum,tr("在移除全局变量“RecentOpenAccId”时发生错误"),VUR_ERROR);
//        return false;
//    }

    emit upgradeStep(verNum,tr("第三步：添加科目系统名称表"),VUR_OK);
    s = QString("create table %1(id integer primary key,%2 integer,%3 text,%4 text,%5 TEXT)")
            .arg(tbl_subSys).arg(fld_ss_code).arg(fld_ss_name).arg(fld_ss_startTime)
            .arg(fld_ss_explain);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        emit upgradeStep(verNum,tr("在创建科目系统名称表“%1”时发生错误！").arg(tbl_subSys),VUR_ERROR);
        return false;
    }
    QStringList statments;
    statments<<QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')").arg(tbl_subSys)
               .arg(fld_ss_code).arg(fld_ss_name).arg(fld_ss_explain).arg(DEFAULT_SUBSYS_CODE)
               .arg(tr("科目系统1")).arg(tr("2013年前使用的老科目系统"))
               <<QString("insert into %1(%2,%3,%4,%5) values(%6,'%6','%7','*8')").arg(tbl_subSys)
                 .arg(fld_ss_code).arg(fld_ss_name).arg(fld_ss_startTime).arg(fld_ss_explain)
                 .arg(2).arg(tr("科目系统2")).arg("2014-01-01").arg(tr("2013年后使用的新科目系统"));
    for(int i = 0; i < statments.count(); ++i){
        if(!q.exec(statments.at(i))){
            LOG_SQLERROR(statments.at(i));
            emit upgradeStep(verNum,tr("在初始化科目系统名称表时发生错误！"),VUR_ERROR);
            return false;
        }
    }
    endUpgrade(verNum,tr("成功升级到版本1.3"),true);
    return setCurVersion(1,3);
}

bool VMAppConfig::updateTo2_0()
{
    return true;
}

/**
 * @brief VMAccount::_getCurVersion
 *  读取当前版本
 * @return
 */
bool VMAccount::_getCurVersion()
{
    QSqlQuery q(db);
    QString s = QString("select %1,%2 from %3 where %4=%5")
            .arg(fld_acci_name).arg(fld_acci_value).arg(tbl_accInfo)
            .arg(fld_acci_code).arg(Account::DBVERSION);
    if(!q.exec(s))
        return false;
    if(!q.first())
        return false;
    QStringList ls = q.value(1).toString().split(".");
    if(ls.count() != 2)
        return false;
    bool ok;
    curMv = ls.at(0).toInt(&ok);
    if(!ok)
        return false;
    curSv = ls.at(1).toInt(&ok);
    if(!ok)
        return false;
    return true;
}


/////////////////////////////VersionManager//////////////////////////////////
VersionManager::VersionManager(VersionManager::ModuleType moduleType, QString fname, QWidget *parent)
    :QDialog(parent),ui(new Ui::VersionManager),mt(moduleType),fileName(fname)
{
    ui->setupUi(this);
    ui->lblDBFile->setText(fname);
    closeBtnState = false;
    switch(moduleType){
    case MT_CONF:
        initConf();
        break;
    case MT_ACC:
        initAcc();
        break;
    }
}

VersionManager::~VersionManager()
{
    delete vmObj;
    delete ui;
}

/**
 * @brief VersionManager::compareVersion
 *  比较版本，如果当前版本低于最高可用版本，则提示升级，如果用户同意则执行升级操作
 * @param cancel：在返回为false时，其值为true，则表示用户取消了升级操作
 * @return true：升级成功，false：升级失败或用户取消升级
 */
bool VersionManager::versionMaintain()
{
    if(upVers.isEmpty())
        return true;
    if(!vmObj->backup(fileName))
        return false;
    foreach(int verNum, upVers){
        if(!vmObj->execUpgrade(verNum)){
            if(!vmObj->restore(fileName))
                QMessageBox::critical(0,tr("升级出错"),tr("在恢复文件“%1”时出错！").arg(fileName));

            return false;
        }
    }
    return true;
}

/**
 * @brief VersionManager::startUpgrade
 *  开始升级到指定版本
 * @param verNum    版本号
 * @param infos     额外信息
 */
void VersionManager::startUpgrade(int verNum, const QString &infos)
{
    if(!upgradeInfos.contains(verNum))
        return;
    QString info = tr("开始升级到版本：%1.%2 >>>>>>>>>>>>>>>>>>>>>>>>>>>>> %3")
            .arg(verNum/100).arg(verNum%100).arg(infos);
    upgradeInfos[verNum].append(info);
    ui->edtInfos->appendPlainText(info);
    QCoreApplication::processEvents();
}

/**
 * @brief VersionManager::endUpgrade
 *  版本升级结束
 * @param verNum    升级的版本号
 * @param infos     升级结束后的额外信息
 * @param ok        升级是否成功
 */
void VersionManager::endUpgrade(int verNum, const QString &infos, bool ok)
{
    if(!upgradeInfos.contains(verNum))
        return;
    QString info;
    if(ok)
        info = tr("成功升级到版本：%1.%2 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<%3")
                .arg(verNum/100).arg(verNum%100).arg(infos);
    else
        info = tr("升级到版本：%1.%2 失败 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<%3")
                .arg(verNum/100).arg(verNum%100).arg(infos);
    upgradeInfos[verNum].append(info);
    ui->edtInfos->appendPlainText(info);
    QCoreApplication::processEvents();
}

/**
 * @brief VersionManager::upgradeStepInform
 *  升级进度通知
 * @param verNum    正在升级的版本号
 * @param infos     进度信息
 * @param result    分步进度结果
 */
void VersionManager::upgradeStepInform(int verNum, const QString &infos, VersionUpgradeResult result)
{
    if(!upgradeInfos.contains(verNum))
        return;
    QString resultStr;
    switch(result){
    case VUR_OK:
        resultStr = tr("成功");
        break;
    case VUR_WARNING:
        resultStr = tr("警告");
        break;
    case VUR_ERROR:
        resultStr = tr("失败");
        break;
    }
    QString info = tr("（%1）升级到版本“%2.%3”进度： %4")
            .arg(resultStr).arg(verNum/100).arg(verNum%100).arg(infos);
    upgradeInfos[verNum].append(info);
    ui->edtInfos->appendPlainText(info);
    QCoreApplication::processEvents();
}

void VersionManager::on_btnStart_clicked()
{
    int curm,curs;
    vmObj->getCurVersion(curm,curs);
    oldVersion = QString("%1.%2").arg(curm).arg(curs);
    ui->btnClose->setEnabled(false);
    closeBtnState = true;
    ui->btnStart->setEnabled(false);
    upgradeResult = versionMaintain();    
    ui->btnClose->setText(tr("关闭"));
    ui->btnClose->setEnabled(true);
    ui->btnSave->setEnabled(true);
}

void VersionManager::initConf()
{
    ui->lblTitle->setText(tr("配置模块版本升级服务"));
    fileName = "basicdata.dat";
    vmObj = new VMAppConfig(fileName);
    init();
}

void VersionManager::initAcc()
{
    ui->lblTitle->setText(tr("账户数据库升级服务"));
    vmObj = new VMAccount(fileName);
    init();

}


void VersionManager::init()
{
    connect(vmObj, SIGNAL(startUpgrade(int,QString)),
            this, SLOT(startUpgrade(int,QString)));
    connect(vmObj, SIGNAL(upgradeStep(int,QString,VersionUpgradeResult)),
            this, SLOT(upgradeStepInform(int,QString,VersionUpgradeResult)));
    connect(vmObj, SIGNAL(endUpgrade(int,QString,bool)),
            this, SLOT(endUpgrade(int,QString,bool)));

    int curMv, curSv, sysMv, sysSv;
    vmObj->getSysVersion(sysMv,sysSv);
    vmObj->getCurVersion(curMv,curSv);
    ui->edtSysVer->setText(QString("%1.%2").arg(sysMv).arg(sysSv));
    ui->edtCurVer->setText(QString("%1.%2").arg(curMv).arg(curSv));

    //填充可升级版本
    upVers = vmObj->getUpgradeVersion();
    QListWidgetItem* item;
    int verNum;
    for(int i = 0; i < upVers.count(); ++i){
        verNum = upVers.at(i);
        item = new QListWidgetItem(tr("版本：%1.%2").arg(verNum/100).arg(verNum%100));
        ui->lstVersion->addItem(item);
        upgradeInfos[verNum] = QStringList();
    }
}

/**
 * @brief VersionManager::close
 *  在更新操作完成后，关闭数据库连接和打开的配置文件。
 */
void VersionManager::close()
{
    on_btnClose_clicked();
}

void VersionManager::on_btnClose_clicked()
{
    if(closeBtnState)
        accept();
    else
        reject();
}


//保存升级日志
void VersionManager::on_btnSave_clicked()
{
    QString fname = fileName;
    int index = fname.indexOf('.');
    if(index != -1)
        fname.chop(fname.length()-index);
    int curm,curs;
    vmObj->getCurVersion(curm,curs);
    QString curVersion = QString("%1.%2").arg(curm).arg(curs);
    if(mt == MT_ACC)
        fname = QString("%1%2（从%3-%4）升级日志.log")
                .arg(LOGS_PATH).arg(fname).arg(oldVersion).arg(curVersion);
    else
        fname = QString("%1基本库从（%2-%3）升级日志.log").arg(LOGS_PATH)
                .arg(oldVersion).arg(curVersion);
    QFile logFile(fname);
    if(!logFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::warning(this,tr("警告信息"),tr("无法将日志信息写入文件"));
        return;
    }
    QTextStream ds(&logFile);
    QList<int> vers = upgradeInfos.keys();
    qSort(vers);
    for(int i = 0; i < vers.count(); ++i){
        foreach (QString info, upgradeInfos.value(vers.at(i))) {
            ds<<info<<"\n";
        }
    }
    ds.flush();
    ui->btnSave->setEnabled(false);
}
