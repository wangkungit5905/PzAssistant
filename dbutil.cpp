#include <QDebug>
#include <QSqlError>
#include <QVariant>

#include "dbutil.h"
#include "global.h"
#include "tables.h"
#include "logs/Logger.h"
#include "subject.h"
#include "pz.h"
#include "PzSet.h"
#include "utils.h"
#include "subject.h"
//#include "config.h"
//#include "account.h"

DbUtil::DbUtil()
{
    //初始化账户信息片段名称表
    pNames[ACODE] = "accountCode";
    pNames[SNAME] = "shortName";
    pNames[LNAME] = "longName";
    //pNames[RPTTYPE] = "reportType";
    pNames[MASTERMT] = "masterMt";
    pNames[WAIMT] = "WaiBiList";
    pNames[STIME] = "startTime";
    pNames[ETIME] = "endTime";
    pNames[CSUITE] = "currentSuite";
    pNames[SUITENAME] = "suiteNames";
    pNames[LASTACCESS] = "lastAccessTime";
    pNames[LOGFILE] = "logFileName";
    pNames[DBVERSION] = "db_version";

}

DbUtil::~DbUtil()
{
    if(db.isOpen())
        close();
}

/**
 * @brief DbUtil::setFilename
 *  设置账户文件名，并打开该连接
 * @param fname
 * @return
 */
bool DbUtil::setFilename(QString fname)
{
    if(db.isOpen())
        close();
    fileName = fname;
    QString name = DATABASE_PATH+fname;
    db = QSqlDatabase::addDatabase("QSQLITE",AccConnName);
    db.setDatabaseName(name);
    if(!db.open()){
        qDebug()<<db.lastError();
        return false;
    }
    return init();
}

bool DbUtil::init()
{
    QSqlQuery q(db);

    //1、设置本位币代码
    QString s = QString("select %1 from %2 where %3=1").arg(fld_mt_code)
            .arg(tbl_moneyType).arg(fld_mt_isMaster);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        LOG_WARNING(QObject::tr("Don't set master money type"));

    }
    return true;
}

/**
 * @brief DbUtil::colse
 * 关闭并移除账户数据库的连接
 */
void DbUtil::close()
{
    db.close();
    QSqlDatabase::removeDatabase(AccConnName);
}

bool DbUtil::getCfgVariable(QString name, QVariant &value)
{
    QSqlQuery q(db);
    QString s = QString("select %1 from %2 where %3='%4'").arg(fld_cfgv_value)
            .arg(tbl_cfgVariable).arg(fld_cfgv_name).arg(name);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        LOG_WARNING(QString("Don't exist config variable(%1)").arg(name));
        value.clear();
        return true;
    }
    value = q.value(0);
    if(q.next()){
        LOG_WARNING("config variable(%1) name conflict!");
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::setCfgVariableForBool
 * @param vtype
 * @param name
 * @param value
 * @return
 */
bool DbUtil::setCfgVariable(QString name, QVariant value)
{
    QSqlQuery q(db);
    if(!db.transaction()){
        LOG_SQLERROR(QString("start transaction failed on set config variable(%1)").arg(name));
        return false;
    }
    QString s = QString("update %1 set %2=:value where %4='%5'")
            .arg(tbl_cfgVariable).arg(fld_cfgv_value)
            .arg(fld_cfgv_name).arg(name);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.bindValue(":value", value);
    if(!q.exec()){
        LOG_SQLERROR(s);
        return false;
    }
    int rows = q.numRowsAffected();
    if(rows > 1){
        db.rollback();
        LOG_WARNING(QString("config variable(%1) name conflict!").arg(name));
        return false;
    }
    if(rows == 0){
        s = QString("insert into %1(%2,%3) values('%4',:value)")
                .arg(tbl_cfgVariable).arg(fld_cfgv_name)
                .arg(fld_cfgv_value).arg(name);
        if(!q.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.bindValue(":value",value);
        if(!q.exec()){
            LOG_SQLERROR(s);
            return false;
        }
    }
    if(!db.commit()){
        LOG_SQLERROR(QString("commit transaction failed on set config variable(%1)").arg(name));
        return false;
    }
    return true;
}

/**
 * @brief 从基本库中导入指定科目系统到账户数据库中
 * @param subSys    科目系统代码
 * @return
 */
bool DbUtil::importFstSubjects(int subSys)
{
    QSqlQuery q(db);
    QString s;

    //1、导入一级科目
    QSqlQuery qm(AppConfig::getBaseDbConnect());
    s = QString("select * from %1 where %2=%3 order by %4").arg(tbl_base_fsub)
            .arg(fld_base_fsub_subsys).arg(subSys).arg(fld_base_fsub_subcode);
    if(!qm.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
    if(!tableExist(tname)){
        s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY,%2 INTEGER,%3 varchar(4),"
                    "%4 varchar(10), %5 integer, %6 integer, %7 integer, %8 INTEGER, "
                    "%9 integer, %10 varchar(10))")
                .arg(tname).arg(fld_fsub_fid).arg(fld_fsub_subcode).arg(fld_fsub_remcode)
                .arg(fld_fsub_class).arg(fld_fsub_jddir).arg(fld_fsub_isEnalbed)
                .arg(fld_fsub_isUseWb).arg(fld_fsub_weight).arg(fld_fsub_name);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9) "
                "values(:code,:remCode,:clsId,:jdDir,:isView,:isUseWb,:weight,:name)")
            .arg(tname).arg(fld_fsub_subcode).arg(fld_fsub_remcode)
            .arg(fld_fsub_class).arg(fld_fsub_jddir).arg(fld_fsub_isEnalbed).arg(fld_fsub_isUseWb)
            .arg(fld_fsub_weight).arg(fld_fsub_name);

    if(!db.transaction()){
        LOG_ERROR("Start transaction failed on import first subject!");
        return false;
    }
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(qm.next()){
        q.bindValue(":code",qm.value(FI_BASE_FSUB_SUBCODE).toString());
        q.bindValue(":remCode",qm.value(FI_BASE_FSUB_REMCODE).toString());
        q.bindValue(":clsId",qm.value(FI_BASE_FSUB_CLS).toInt());
        q.bindValue(":jdDir",qm.value(FI_BASE_FSUB_JDDIR).toInt());
        q.bindValue(":isView",qm.value(FI_BASE_FSUB_ENABLE).toInt());
        q.bindValue(":isUseWb",qm.value(FI_BASE_FSUB_USEDWB).toInt());
        q.bindValue(":weight",qm.value(FI_BASE_FSUB_WEIGHT).toInt());
        q.bindValue(":name",qm.value(FI_BASE_FSUB_SUBNAME).toString());
        if(!q.exec()){
            LOG_SQLERROR("on exec insert into first subject failed!");
            return false;
        }
    }

    //2、导入新科目系统类别
    s = QString("select * from %1 where %2=%3 order by %4")
            .arg(tbl_base_fsub_cls).arg(fld_base_fsub_cls_subSys).arg(subSys)
            .arg(fld_base_fsub_cls_clsCode);
    if(!qm.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("insert into %1(%2,%3,%4) values(%5,:code,:name)")
            .arg(tbl_fsclass).arg(fld_fsc_subSys).arg(fld_fsc_code).arg(fld_fsc_name)
            .arg(subSys);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(qm.next()){
        q.bindValue(":code",qm.value(FI_BASE_FSUB_CLS_CODE).toInt());
        q.bindValue(":name",qm.value(FI_BASE_FSUB_CLS_NAME).toString());
        if(!q.exec()){
            LOG_SQLERROR("On insert into first subject class failed!");
            return false;
        }
    }

    //3、对接新老一级科目
    QHash<QString,QString> codeMaps;
    QHash<QString,QString> maps;
    if(!AppConfig::getInstance()->getSubSysMaps2(DEFAULT_SUBSYS_CODE,subSys,codeMaps,maps)){
        LOG_ERROR("Don't get subject system subject join items!");
        return false;
    }
    QString sTable = QString("%1%2").arg(tbl_fsub_prefix).arg(DEFAULT_SUBSYS_CODE);
    QString dTable = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
    QSqlQuery q1(db),q2(db);
    s = QString("select %1 from %2 where %3=:scode").arg(fld_fsub_fid).arg(sTable)
            .arg(fld_fsub_subcode);
    if(!q1.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("update %1 set %2=:fid where %3=:dcode").arg(dTable).arg(fld_fsub_fid)
            .arg(fld_fsub_subcode);
    if(!q2.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }

    QHashIterator<QString,QString> it(codeMaps);
    int fid = 0;
    while(it.hasNext()){
        it.next();
        q1.bindValue(":scode",it.key());
        if(!q1.exec())
            return false;
        if(!q1.first()){
            s = QString("insert into FstSubIDs default values");
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            s = "select last_insert_rowid()";
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            q.first();
            fid = q.value(0).toInt();
        }
        else
            fid = q1.value(0).toInt();
        q2.bindValue(":fid",fid);
        q2.bindValue(":dcode",it.value());
        if(!q2.exec()){
            LOG_SQLERROR(q2.lastQuery());
            return false;
        }
    }

    //4、对接需混合并入的二级科目
    tname = QString("%1_%2_%3").arg(tbl_sndsub_join_pre).arg(DEFAULT_SUBSYS_CODE).arg(subSys);
    if(!tableExist(tname)){
        s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY, %2 INTEGER, %3 INTEGER,"
                    " %4 INTEGER, %5 INTEGER)").arg(tname).arg(fld_ssj_s_fsub)
                .arg(fld_ssj_s_ssub).arg(fld_ssj_d_fsub).arg(fld_ssj_d_ssub);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }

    codeMaps.clear();
    if(!AppConfig::getInstance()->getNotDefSubSysMaps(DEFAULT_SUBSYS_CODE,subSys,codeMaps)){
        return false;
    }
    QHashIterator<QString,QString> im(codeMaps);
    int s_fid,s_sid,s_nid,d_fid,d_sid;
    while(im.hasNext()){
        im.next();
        s = QString("select %1 from %2 where %3='%4'").arg(fld_fsub_fid).arg(sTable)
                .arg(fld_fsub_subcode).arg(im.key());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(!q.first()){
            LOG_ERROR(QObject::tr("导入新科目系统期间，无法找到源科目（%1）").arg(im.key()));
            return false;
        }
        s_fid = q.value(0).toInt();
        s = QString("select %1 from %2 where %3='%4'").arg(fld_fsub_fid).arg(dTable)
                .arg(fld_fsub_subcode).arg(im.value());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(!q.first()){
            LOG_ERROR(QObject::tr("导入新科目系统期间，无法找到对接科目（%1）").arg(im.value()));
            return false;
        }
        d_fid = q.value(0).toInt();
        s = QString("select id,%1 from %2 where %3=%4").arg(fld_ssub_nid).arg(tbl_ssub)
                .arg(fld_ssub_fid).arg(s_fid);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next()){
            s_sid = q.value(0).toInt();
            s_nid = q.value(1).toInt();
            s = QString("select id from %1 where %2=%3 and %4=%5").arg(tbl_ssub)
                    .arg(fld_ssub_fid).arg(d_fid).arg(fld_ssub_nid).arg(s_nid);
            if(!q1.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            if(!q1.first()){
                QString ds = AppConfig::getInstance()->getSpecSubSysItem(subSys)->startTime.toString(Qt::ISODate);
                s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values(%8,%9,1,1,'%10',%11)")
                            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_weight)
                        .arg(fld_ssub_enable).arg(fld_ssub_crtTime).arg(fld_ssub_creator)
                        .arg(d_fid).arg(s_nid).arg(ds).arg(curUser->getUserId());
                if(!q1.exec(s)){
                    LOG_SQLERROR(s);
                    return false;
                }
                s = "select last_insert_rowid()";
                if(!q1.exec(s)){
                    LOG_SQLERROR(s);
                    return false;
                }
                q1.first();
            }
            d_sid = q1.value(0).toInt();
            s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,%8,%9)").arg(tname)
                    .arg(fld_ssj_s_fsub).arg(fld_ssj_s_ssub).arg(fld_ssj_d_fsub)
                    .arg(fld_ssj_d_ssub).arg(s_fid).arg(s_sid).arg(d_fid).arg(d_sid);
            if(!q1.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }

    if(!db.commit()){
        LOG_ERROR("commit transaction failed on import first subject!");
        return false;
    }
    return true;
}

/**
 * @brief 根据指定的源和目标科目管理器，提取两个科目系统衔接的科目映射信息
 * @param src
 * @param des
 * @param cfgs
 * @return
 */
bool DbUtil::getSubSysJoinCfgInfo(SubjectManager* src, SubjectManager* des, QList<SubSysJoinItem *> &cfgs)
{
    //注意：衔接映射表的命名规则：subSysJoin_n1_n2，其中n1用源科目系统代码表示，n2用目的科目系统代码表示
//    QSqlQuery q(db);
//    QString tname = QString("%1_%2_%3").arg(tbl_ssjc_pre).arg(src->getSubSysCode()).arg(des->getSubSysCode());
//    QString s;
//    if(!_tableExist(tname)){
//        if(!db.transaction()){
//            LOG_SQLERROR("start transaction failed on read subject system join config infomation!");
//            return false;
//        }
//        s = QString("create table %1(id integer primary key, %2 integer, %3 integer, %4 integer, %5 text)")
//                .arg(tname).arg(fld_ssjc_sSub).arg(fld_ssjc_dSub).arg(fld_ssjc_isMap).arg(fld_ssjc_ssubMaps);
//        if(!q.exec(s)){
//            LOG_SQLERROR(s);
//            return false;
//        }
//        //将源科目系统中配置为启用的一级科目导入到配置表中
//        s = QString("insert into %1(%2,%3,%4,%5) values(:ssub,0,0,'')").arg(tname)
//                .arg(fld_ssjc_sSub).arg(fld_ssjc_dSub).arg(fld_ssjc_isMap).arg(fld_ssjc_ssubMaps);
//        if(!q.prepare(s)){
//            LOG_SQLERROR(s);
//            return false;
//        }
//        FSubItrator* it = src->getFstSubItrator();
//        while(it->hasNext()){
//            it->next();
//            FirstSubject* fsub = it->value();
//            if(fsub->isEnabled()){
//                q.bindValue(":ssub",fsub->getId());
//                if(!q.exec())
//                    return false;
//            }
//        }
//        if(!db.commit()){
//            LOG_SQLERROR("commit transaction failed on read subject system join config infomation!");
//            return false;
//        }
//    }

//    s = QString("select * from %1").arg(tname);
//    if(!q.exec(s)){
//        LOG_SQLERROR(s);
//        return false;
//    }
//    QStringList sl;
//    while(q.next()){
//        sl.clear();
//        SubSysJoinItem* item = new SubSysJoinItem;
//        item->sFSub = src->getFstSubject(q.value(SSJC_SSUB).toInt());
//        item->dFSub = des->getFstSubject(q.value(SSJC_DSUB).toInt());
//        item->isMap = q.value(SSJC_ISMAP).toBool();
//        sl = q.value(SSJC_SSUBMaps).toString().split(",",QString::SkipEmptyParts);
//        if(!sl.isEmpty()){
//            for(int i = 0; i < sl.count(); ++i)
//                item->ssubMaps<<sl.at(i).toInt();
//        }
//        cfgs<<item;
//    }
    return true;
}

/**
 * @brief 保存科目系统衔接的科目映射信息
 * @param src
 * @param des
 * @param cfgs
 * @return
 */
//bool DbUtil::setSubSysJoinCfgInfo(SubjectManager *src, SubjectManager *des, QList<SubSysJoinItem *> &cfgs)
//{
//    QSqlQuery q(db);
//    QString tname = QString("%1_%2_%3").arg(tbl_ssjc_pre).arg(src->getSubSysCode()).arg(des->getSubSysCode());
//    QString s = QString("update %1 set %2=:dSub,%3=:isMap,%4=:ssubMaps where %5=:sSub")
//            .arg(tname).arg(fld_ssjc_dSub).arg(fld_ssjc_isMap).arg(fld_ssjc_ssubMaps).arg(fld_ssjc_sSub);
//    if(!db.transaction()){
//        LOG_SQLERROR("start transaction failed on save subject system join config infomation!");
//        return false;
//    }
//    if(!q.prepare(s)){
//        LOG_SQLERROR(s);
//        return false;
//    }
//    foreach(SubSysJoinItem* item,cfgs){
//        q.bindValue(":dSub",item->dFSub->getId());
//        q.bindValue(":isMap", item->isMap?1:0);
//        q.bindValue(":sSub",item->sFSub->getId());
//        if(item->ssubMaps.isEmpty())
//            q.bindValue(":ssubMaps","");
//        else{
//            QString sl;
//            for(int i = 0; i < item->ssubMaps.count(); ++i)
//                sl.append(QString("%1,").arg(item->ssubMaps.at(i)));
//            sl.chop(1);
//            q.bindValue(":ssubMaps",sl);
//        }
//        if(!q.exec()){
//            LOG_SQLERROR("exec prepare sql failed on save subject system join config infomation!");
//            return false;
//        }
//    }
//    if(!db.commit()){
//        LOG_SQLERROR("commit transaction failed on save subject system join config infomation!");
//        return false;
//    }
//    return true;
//}

/**
 * @brief 指定名称的表格是否存在
 * @param tableName
 * @return
 */
bool DbUtil::tableExist(QString tableName)
{
    QSqlQuery q(db);
    QString s = QString("select name from sqlite_master where type='table' and tbl_name='%1'").arg(tableName);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return q.first();
}

/**
 * @brief DbUtil::readAccBriefInfo
 *  读取账户的简要信息
 * @param info
 * @return
 */
//bool DbUtil::readAccBriefInfo(AccountBriefInfo &info)
//{
//    QSqlQuery q(db);
//    QString s = QString("select %1,%2 from %3")
//            .arg(fld_acci_code).arg(fld_acci_value).arg(tbl_accInfo);
//    if(!q.exec(s))
//        return false;
//    InfoField code;
//    while(q.next()){
//        code = (InfoField)q.value(0).toInt();
//        switch(code){
//        case ACODE:
//            info.code = q.value(1).toString();
//            break;
//        case SNAME:
//            info.sname = q.value(1).toString();
//            break;
//        case LNAME:
//            info.lname = q.value(1).toString();
//            break;
//        }
//    }
//    info.isRecent = false;
//    info.fname = fileName;
//    return true;
//}

/**
 * @brief DbUtil::initAccount
 *  从账户数据库文件中读取账户信息到infos结构中
 * @param infos
 * @return
 */
bool DbUtil::initAccount(Account::AccountInfo &infos)
{
    QSqlQuery q(db);
    QString s = QString("select %1,%2 from %3")
            .arg(fld_acci_code).arg(fld_acci_value).arg(tbl_accInfo);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }

    InfoField code;
    QStringList sl;
    while(q.next()){
        code = (InfoField)q.value(0).toInt();
        switch(code){
        case ACODE:
            infos.code = q.value(1).toString();
            break;
        case SNAME:
            infos.sname = q.value(1).toString();
            break;
        case LNAME:
            infos.lname = q.value(1).toString();
            break;
        case LOGFILE:
            infos.logFileName = q.value(1).toString();
            break;
        case DBVERSION:
            infos.dbVersion = q.value(1).toString();
            sl = infos.dbVersion.split(".");
            mv = sl.first().toInt();
            sv = sl.last().toInt();
            break;
        }
    }

    //如果表内的信息字段内容不全，则需要提供默认值，以使对象的属性具有意义
    //默认，日志文件名同账户文件名同名，但扩展名不同
    if(infos.logFileName.isEmpty()){
        infos.logFileName = fileName;
        infos.logFileName = infos.logFileName.replace(".dat",".log");
    }

    //读取账户的最近的转移记录
    s = QString("select * from %1").arg(tbl_transfer);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.last()){
        LOG_ERROR("Don't fonded recent account transfer record!");
        return false;
    }
    //infos.transInfo = new Account::AccontTranferInfo;

    return true;
}

/**
 * @brief 初始化帐套信息
 * @param suites
 * @return
 */
bool DbUtil::initSuites(QList<AccountSuiteRecord *> &suites)
{
    return _readAccountSuites(suites);
}

/**
 * @brief DbUtil::saveAccountInfo
 *  将账户信息保存的账户文件中
 * @param infos
 * @return
 */
bool DbUtil::saveAccountInfo(Account::AccountInfo &infos)
{
    //基本实现思路先读取一份账户文件中的的信息，与传入的参数进行比较，在发生变化的情况下进行更新或插入操作
    Account::AccountInfo oldInfos;
    if(!initAccount(oldInfos))
        return false;

    if(infos.code != oldInfos.code && !saveAccInfoPiece(ACODE,infos.code))
        return false;
    if(infos.sname != oldInfos.sname && !saveAccInfoPiece(SNAME,infos.sname))
        return false;
    if(infos.lname != oldInfos.lname && !saveAccInfoPiece(LNAME,infos.lname))
        return false;
    if(infos.lastAccessTime != oldInfos.lastAccessTime && !saveAccInfoPiece(LASTACCESS,infos.lastAccessTime))
        return false;
    if(infos.dbVersion != oldInfos.dbVersion && !saveAccInfoPiece(DBVERSION,infos.dbVersion))
        return false;
    if(infos.logFileName != oldInfos.logFileName && !saveAccInfoPiece(LOGFILE,infos.logFileName))
        return false;
    //保存外币列表
    bool changed = false;
    if(infos.waiMts.count() != oldInfos.waiMts.count())
        changed = true;
    else{
        foreach(Money* mt, infos.waiMts){
            if(!oldInfos.waiMts.contains(mt)){
                changed = true;
                break;
            }
        }
    }
    if(changed && !saveMoneys(infos.waiMts))
        return false;
    return true;
}

/**
 * @brief DbUtil::saveBankInfo
 *  保存开户行信息
 * @param ba
 * @return
 */
bool DbUtil::saveBankInfo(Bank *ba, bool isDel)
{
    QSqlQuery q(db);
    QString s;
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save bank!");
        return false;
    }

    if(isDel){
        foreach(BankAccount* acc, ba->bas){
            s = QString("delete from %1 where id=%2").arg(tbl_bankAcc).arg(acc->id);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        s = QString("delete from %1 where id=%2").arg(tbl_bank).arg(ba->id);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(!db.commit()){
            LOG_SQLERROR("Commit transaction failed on save bank!");
            return false;
        }
        return true;
    }
    if(ba->id == UNID){
        s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')").arg(tbl_bank)
                .arg(fld_bank_isMain).arg(fld_bank_name).arg(fld_bank_lname)
                .arg(ba->isMain?1:0).arg(ba->name).arg(ba->lname);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        s = "select last_insert_rowid()";
        q.exec(s); q.first();
        ba->id = q.value(0).toInt();
        foreach(BankAccount* acc, ba->bas){
            if(acc->niObj && (acc->niObj->getId() == 0) && !saveNameItem(acc->niObj)){
                LOG_ERROR("Save name item failed on save bank object!");
                return false;
            }
            s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,'%8',%9)").arg(tbl_bankAcc)
                    .arg(fld_bankAcc_bankId).arg(fld_bankAcc_mt).arg(fld_bankAcc_accNum)
                    .arg(fld_bankAcc_nameId).arg(acc->parent->id).arg(acc->mt->code())
                    .arg(acc->accNumber).arg(acc->niObj?acc->niObj->getId():0);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            s = "select last_insert_rowid()";
            q.exec(s); q.first();
            acc->id = q.value(0).toInt();
        }
    }
    else{
        s = QString("update %1 set %2=%3,%4='%5',%6='%7' where id=%8").arg(tbl_bank)
                .arg(fld_bank_isMain).arg(ba->isMain?1:0).arg(fld_bank_name)
                .arg(ba->name).arg(fld_bank_lname).arg(ba->lname).arg(ba->id);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        QList<int> accIds;
        s = QString("select id from %1 where %2=%3").arg(tbl_bankAcc).arg(fld_bankAcc_bankId).arg(ba->id);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next())
            accIds<<q.value(0).toInt();
        foreach(BankAccount* acc, ba->bas){
            if(acc->niObj && (acc->niObj->getId() == 0) && !saveNameItem(acc->niObj)){
                LOG_ERROR("Save name item failed on save bank object!");
                return false;
            }
            if(acc->id == UNID){
                s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,'%8',%9)").arg(tbl_bankAcc)
                        .arg(fld_bankAcc_bankId).arg(fld_bankAcc_mt).arg(fld_bankAcc_accNum)
                        .arg(fld_bankAcc_nameId).arg(acc->parent->id).arg(acc->mt->code())
                        .arg(acc->accNumber).arg(acc->niObj?acc->niObj->getId():0);
                if(!q.exec(s)){
                    LOG_SQLERROR(s);
                    return false;
                }
                s = "select last_insert_rowid()";
                q.exec(s); q.first();
                acc->id = q.value(0).toInt();
            }
            else{
                s = QString("update %1 set %2=%3,%4='%5',%6=%7 where id=%8")
                        .arg(tbl_bankAcc).arg(fld_bankAcc_mt).arg(acc->mt->code())
                        .arg(fld_bankAcc_accNum).arg(acc->accNumber).arg(fld_bankAcc_nameId)
                        .arg(acc->niObj?acc->niObj->getId():0).arg(acc->id);
                if(!q.exec(s)){
                    LOG_SQLERROR(s);
                    return false;
                }
                accIds.removeOne(acc->id);
            }
        }
        if(!accIds.isEmpty()){
            foreach(int id, accIds){
                s = QString("delete from %1 where id=%2").arg(tbl_bankAcc).arg(id);
                if(!q.exec(s)){
                    LOG_SQLERROR(s);
                    return false;
                }
            }
        }
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on save bank!");
        return false;
    }
    return true;
}

bool DbUtil::saveSuites(QList<AccountSuiteRecord *> &suites)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save Account suites!");
        return false;
    }
    foreach(AccountSuiteRecord* as, suites){
        if(!_saveAccountSuite(as))
            return false;
    }

    if(!db.commit()){
        if(!db.rollback())
            LOG_SQLERROR("Rollback transaction failed on save Account suites!");
        LOG_SQLERROR("Commit transaction failed on save Account suites!");
        return false;
    }
}

bool DbUtil::saveSuite(AccountSuiteRecord *suite)
{
    return _saveAccountSuite(suite);
}

/**
 * @brief DbUtil::initNameItems
 *  初始化科目管理器使用的名称条目
 * @return
 */
bool DbUtil::initNameItems()
{
    QSqlQuery q(db);
    QString s;

    //1、装载名称条目类别（有关名称条目的信息只需装载一次）
    s = QString("select * from %1").arg(tbl_nameItemCls);
    if(!q.exec(s))
        return false;
    while(q.next()){
        int code = q.value(NICLASS_CODE).toInt();
        SubjectManager::nameItemCls[code] = QStringList();
        SubjectManager::nameItemCls[code].append(q.value(NICLASS_NAME).toString());
        SubjectManager::nameItemCls[code].append(q.value(NICLASS_EXPLAIN).toString());
    }


    //4、装载所有名称条目
    s = QString("select * from %1").arg(tbl_nameItem);
    if(!q.exec(s))
        return false;

    SubjectNameItem* item;
    while(q.next()){
        int id = q.value(0).toInt();
        int clsId = q.value(NI_CALSS).toInt();
        QString sname = q.value(NI_NAME).toString();
        QString lname = q.value(NI_LNAME).toString();
        QString remCode = q.value(NI_REMCODE).toString();
        QDateTime crtTime = QDateTime::fromString(q.value(NI_CREATERTIME).toString(),Qt::ISODate);
        int uid = q.value(NI_CREATOR).toInt();
        item = new SubjectNameItem(id,clsId,sname,lname,remCode,crtTime,allUsers.value(uid));
        SubjectManager::nameItems[id]=item;
    }
    return true;
}

/**
 * @brief DbUtil::initSubjects
 *  初始化科目管理器对象
 * @param smg       科目管理器对象指针
 * @param subSys    科目系统代码
 * @return
 */
bool DbUtil::initSubjects(SubjectManager *smg, int subSys)
{
    QSqlQuery q(db);
    QString s;

    int preSubSys = (subSys<=2)?DEFAULT_SUBSYS_CODE:(subSys-1);    //前一个科目系统的代码
    //1、装载一级科目类别
    s = QString("select %1,%2 from %3 where %4=%5").arg(fld_fsc_code)
            .arg(fld_fsc_name).arg(tbl_fsclass).arg(fld_fsc_subSys).arg(subSys);
    if(!q.exec(s))
        return false;
    int cls;
    QHash<int,SubjectClass> subClsMaps = AppConfig::getInstance()->getSubjectClassMaps(subSys);
    while(q.next()){
        cls = q.value(0).toInt();
        smg->fsClsNames[subClsMaps.value(cls)] = q.value(1).toString();
    }

    //2、装载所有一级科目
    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
    s = QString("select * from %1 order by %2")
            .arg(tname).arg(fld_fsub_subcode);
    if(!q.exec(s))
        return false;

    QString name,code,remCode,explain,usage;
    bool jdDir,isUseWb,isEnable;
    int id, weight;
    SubjectClass subCls;
    FirstSubject* fsub;

    while(q.next()){
        id = q.value(FSUB_FID).toInt();
        subCls = subClsMaps.value(q.value(FSUB_CLASS).toInt());
        name = q.value(FSUB_SUBNAME).toString();
        code = q.value(FSUB_SUBCODE).toString();
        remCode = q.value(FSUB_REMCODE).toString();
        weight = q.value(FSUB_WEIGHT).toInt();
        isEnable = q.value(FSUB_ISVIEW).toBool();
        jdDir = q.value(FSUB_DIR).toBool();
        isUseWb = q.value(FSUB_ISUSEWB).toBool();
        //读取explain和usage的内容，目前暂不支持（将来这两个内容将保存在另一个表中）
        //s = QString("select * from ")
        fsub = new FirstSubject(smg,id,subCls,name,code,remCode,weight,isEnable,jdDir,isUseWb,explain,usage,subSys);
        //smg->fstSubs<<fsub;
        smg->fstSubHash[id]=fsub;

        //设置特定科目对象
        AppConfig* conf = AppConfig::getInstance();
        if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_CASH))
            smg->cashSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_BANK))
            smg->bankSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_GDZC))\
            smg->gdzcSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_CWFY))
            smg->cwfySub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_BNLR))
            smg->bnlrSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_LRFP))
            smg->lrfpSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_YS))
            smg->ysSub = fsub;
        else if(code == conf->getSpecSubCode(subSys,AppConfig::SSC_YF))
            smg->yfSub = fsub;
    }

    //3、装载所有二级科目
    //首先要判定是否存在有采用比当前科目系统还要新的帐套，如果有则记录该帐套的时间点
    //这个主要是用来判定读取到的二级科目是否属于当前科目系统
    s = QString("select %1,%2 from %3 where %4>%5").arg(fld_accs_year)
            .arg(fld_accs_startMonth).arg(tbl_accSuites).arg(fld_accs_subSys)
            .arg(subSys);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    bool exist = q.first();
    QDate startDate;
    if(exist){
        int y = q.value(0).toInt();
        int m = q.value(1).toInt();
        startDate.setDate(y,m,1);
    }

    s = QString("select * from %1 join %2 on %1.%3=%2.%4").arg(tbl_ssub).arg(tname)
            .arg(fld_ssub_fid).arg(fld_fsub_fid);
    if(!q.exec(s))
        return false;

    SecondSubject* ssub;
    while(q.next()){
        QDateTime crtTime = q.value(SSUB_CREATETIME).toDateTime();
        if(exist && crtTime.date() >= startDate)
            continue;
        int id = q.value(0).toInt();
        int fid = q.value(SSUB_FID).toInt();
        fsub = smg->fstSubHash.value(fid);
        if(!fsub){
            LOG_INFO(QObject::tr("Find a second subject(id=%1) don't belong to any first subject!").arg(id));
            continue;
        }
        int sid = q.value(SSUB_NID).toInt();
        if(!smg->nameItems.contains(sid)){
            LOG_INFO(QObject::tr("Find a name item(id=%1,fid=%2 %3,sid=%4) don't exist!")
                     .arg(id).arg(fid).arg(fsub->getName()).arg(sid));
            continue;
        }
        code = q.value(SSUB_SUBCODE).toString();
        weight = q.value(SSUB_WEIGHT).toInt();
        bool isEnable = q.value(SSUB_ENABLED).toBool();

        QDateTime disTime = QDateTime::fromString(q.value(SSUB_DISABLETIME).toString(),Qt::ISODate);
        int uid = q.value(SSUB_CREATOR).toInt();
        ssub = new SecondSubject(fsub,id,smg->nameItems.value(sid),code,weight,isEnable,crtTime,disTime,allUsers.value(uid));
        smg->sndSubs[id] = ssub;
        fsub->addChildSub(ssub);
        if(ssub->getWeight() == DEFALUT_SUB_WEIGHT)
            fsub->setDefaultSubject(ssub);
    }
    //如果是新科目系统，则有些一级科目可能有多个老的一级科目对接到它，因此要把这些二级科目移植到新科目
    //比如，老科目系统的预付账款和待摊费用都对接到新科目系统的预付账款，这样就必须把待摊费用下的二级科目
    //移植到预付账款

//    if(subSys != preSubSys){
//        QHash<QString,QString> maps;
//        if(!AppConfig::getInstance()->getNotDefSubSysMaps(preSubSys,subSys,maps))
//            return false;
//        if(maps.isEmpty())
//            return true;
//        QString sTable = QString("%1%2").arg(tbl_fsub_prefix).arg(preSubSys);
//        QHashIterator<QString,QString> it(maps);
//        while(it.hasNext()){
//            it.next();
//            s = QString("select %1 from %2 where %3='%4'").arg(fld_fsub_fid)
//                    .arg(sTable).arg(fld_fsub_subcode).arg(it.key());
//            if(!q.exec(s)){
//                LOG_SQLERROR(s);
//                return false;
//            }
//            if(!q.first()){
//                LOG_ERROR(QObject::tr("未找到非默认的对接源科目（%1）").arg(it.key()));
//                return false;
//            }
//            int fid = q.value(0).toInt();
//            s = QString("select * from %1 where %2=%3").arg(tbl_ssub)
//                    .arg(fld_ssub_fid).arg(fid);
//            if(!q.exec(s)){
//                LOG_SQLERROR(s);
//                return false;
//            }
//            FirstSubject* fsub = smg->getFstSubject(it.value());
//            if(!fsub){
//                LOG_ERROR(QObject::tr("未找到非默认的对接目标科目（%1）").arg(it.value()));
//                return false;
//            }
//            while(q.next()){
//                int id = q.value(0).toInt();
//                int nid = q.value(SSUB_NID).toInt();
//                code = q.value(SSUB_SUBCODE).toString();
//                weight = q.value(SSUB_WEIGHT).toInt();
//                bool isEnable = q.value(SSUB_ENABLED).toBool();
//                QDateTime crtTime = QDateTime::fromString(q.value(SSUB_CREATETIME).toString(),Qt::ISODate);
//                QDateTime disTime = QDateTime::fromString(q.value(SSUB_DISABLETIME).toString(),Qt::ISODate);
//                int uid = q.value(SSUB_CREATOR).toInt();
//                ssub = new SecondSubject(fsub,id,smg->nameItems.value(nid),code,weight,isEnable,crtTime,disTime,allUsers.value(uid));
//                smg->sndSubs[id] = ssub;
//                fsub->addChildSub(ssub);
//            }
//        }
//    }
    return true;
}

/**
 * @brief 保存名称条目类别
 * @param code
 * @param name
 * @param explain
 * @return
 */
bool DbUtil::saveNameItemClass(int code, QString name, QString explain)
{
    QSqlQuery q(db);
    QString s = QString("update %1 set %2='%3',%4='%5' where %6=%7")
            .arg(tbl_nameItemCls).arg(fld_nic_name).arg(name)
            .arg(fld_nic_explain).arg(explain).arg(fld_nic_clscode).arg(code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.numRowsAffected() == 0){
        s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')")
                .arg(tbl_nameItemCls).arg(fld_nic_clscode).arg(fld_nic_name)
                .arg(fld_nic_explain).arg(code).arg(name).arg(explain);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    return true;
}

bool DbUtil::saveNameItem(SubjectNameItem *ni)
{
    return _saveNameItem(ni);
}

/**
 * @brief DbUtil::removeNameItem
 * @param ni
 * @return
 */
bool DbUtil::removeNameItem(SubjectNameItem *ni)
{
    return _removeNameItem(ni);
}

/**
 * @brief 从数据库中移除指定代码的名称条目类别
 * @param code
 * @return
 */
bool DbUtil::removeNameItemCls(int code)
{
    QSqlQuery q(db);
    QString s = QString("delete from %1 where %2=%3").arg(tbl_nameItemCls)
            .arg(fld_nic_clscode).arg(code);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief 从数据库中移除二级科目
 * @param subs
 * @return
 */
bool DbUtil::removeSndSubjects(QList<SecondSubject *> subs)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on banch delete second subject!");
        return false;
    }
    foreach(SecondSubject* sub, subs){
        if(!_removeSecondSubject(sub)){
            if(!db.rollback())
                LOG_SQLERROR("Rollback transaction failed on banch delete second subject!");
            return false;
        }
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on banch delete second subject!");
        return false;
    }
    return true;
}

bool DbUtil::saveSndSubject(SecondSubject *sub)
{
    return _saveSecondSubject(sub);
}

/**
 * @brief 批量保存二级科目
 * @param subs
 * @return
 */
bool DbUtil::saveSndSubjects(QList<SecondSubject *> subs)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on banch save second subject!");
        return false;
    }
    foreach(SecondSubject* ssub, subs){
        if(!_saveSecondSubject(ssub)){
            if(!db.rollback())
                LOG_SQLERROR("Rollback transaction failed on banch save second subject!");
            return false;
        }
    }
    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on banch save second subject!");
        return false;
    }
    return true;
}

bool DbUtil::savefstSubject(FirstSubject *fsub)
{
    return _saveFirstSubject(fsub);
}

/**
 * @brief DbUtil::getBankSubMatchMoney
 *  返回与银行科目的子目对应的货币代码
 * @param sub
 * @return
 */
int DbUtil::getBankSubMatchMoney(SecondSubject *sub)
{
    if(!sub)
        return 0;
    QSqlQuery q(db);
    if(!db.transaction()){
        LOG_ERROR(QObject::tr("Start transaction failed!"));
        return 0;
    }

    QString s = QString("select %1 from %2 where %3=%4").arg(fld_bankAcc_mt).arg(tbl_bankAcc)
            .arg(fld_bankAcc_nameId).arg(sub->getNameItem()->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return 0;
    }
    if(!q.first())
        return 0;
    int rowId = q.value(0).toInt();
    s = QString("select %1 from %2 where id=%3")
            .arg(fld_mt_code).arg(tbl_moneyType).arg(rowId);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return 0;
    }
    if(!q.first())
        return 0;
    int mt = q.value(0).toInt();

    if(!db.commit()){
        LOG_ERROR(QObject::tr("commit transaction failed!"));
        return 0;
    }
    return mt;
}

/**
 * @brief 查询数据库以判断指定的名称条目是否已被使用了，这是删除名称条目前的检查手段
 * @param ni
 * @return true：已被使用，false：未被使用或出错
 */
bool DbUtil::nameItemIsUsed(SubjectNameItem *ni)
{
    QSqlQuery q(db);
    QString s = QString("select count() from %1 where %2=%3").arg(tbl_ssub).arg(fld_ssub_nid).arg(ni->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.first();
    int c = q.value(0).toInt();
    return c != 0;
}

/**
 * @brief 查找会计分录表和余额表，是否指定的二级科目已被采用
 * @param ssub
 * @return true：被采用了，false：未被采用
 */
bool DbUtil::ssubIsUsed(SecondSubject *ssub)
{
    QSqlQuery q(db);
    QString s = QString("select id from %1 where %2=%3").arg(tbl_ba).arg(fld_ba_sid).arg(ssub->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    bool result = q.first();
    if(result)
        return true;
    return ssubIsUsedInExtraTable(ssub);
}

/**
 * @brief 指定子目是否在余额表中被引用
 * @param ssub
 * @return
 */
bool DbUtil::ssubIsUsedInExtraTable(SecondSubject *ssub)
{
    QSqlQuery q(db);
    QString s = QString("select id from %1 where %2=%3").arg(tbl_nse_p_s).arg(fld_nse_sid).arg(ssub->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first())
        return true;
    if(ssub->isUseForeignMoney()){
        s = QString("select id from %1 where %2=%3").arg(tbl_nse_m_s).arg(fld_nse_sid).arg(ssub->getId());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        return q.first();
    }
    return false;
}



/**
 * @brief 返回指定的科目系统下的科目是否已被导入一级科目表
 * @param subSys
 * @return
 */
//bool DbUtil::isSubSysImported(int subSys)
//{
//    QSqlQuery q(db);
//    QString s = QString("select id from %1 where %2=%3").arg(tbl_fsub).arg(fld_fsub_subSys).arg(subSys);
//    if(!q.exec(s)){
//        LOG_SQLERROR(s);
//        return false;
//    }
//    return q.first();
//}

///**
// * @brief DbUtil::isSubSysJoinConfiged
// *  获取从源科目系统到目的科目系统的配置是否完成
// * @param source    源科目系统代码
// * @param destinate 目的科目系统代码
// * @return
// */
//bool DbUtil::isSubSysJoinConfiged(int source, int destinate)
//{
//    QSqlQuery q(db);
//    QString table = QString("%1_%2_%3").arg(tbl_ssjc_pre).arg(source).arg(destinate);
//    QString s = QString("select %4 from %1 where %2=0 and %3=0").arg(table)
//            .arg(fld_ssjc_sSub).arg(fld_ssjc_dSub).arg(fld_ssjc_isMap);
//    if(q.exec(s)){
//        LOG_SQLERROR(s);
//        return false;
//    }
//    if(!q.next()){
//        return false;
//    }
//    int v = q.value(0).toBool();
//    return v;
//}

/**
 * @brief 将指定时间范围内的被合并科目的余额并入保留科目，
 *        并将此范围内的分录中引用的被合并科目替换为保留科目
 * @param startYear     起始年份
 * @param startMonth    起始月份
 * @param endYear       结束年份
 * @param endMonth      结束月份
 * @param preSub        保留科目
 * @param mergedSubs    待合并科目
 * @return
 */
bool DbUtil::mergeSecondSubject(int startYear, int startMonth, int endYear, int endMonth, SecondSubject *preSub, QList<SecondSubject *> mergedSubs)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on merge second subject!");
        return false;
    }
    if(!_replaceSidWithResorved2(startYear,startMonth,endYear,endMonth,preSub,mergedSubs)){
        if(!db.rollback())
            LOG_SQLERROR("Rollback transaction failed on merge second subject!");
        LOG_ERROR("Replace second subject failed on merge second subject!");
        return false;
    }
    if(!_mergeExtraWithinRange(startYear,startMonth,endYear,endMonth,preSub,mergedSubs)){
        if(!db.rollback())
            LOG_SQLERROR("Rollback transaction failed on merge second subject!");
        LOG_ERROR("Merge second subject extra failed on merge second subject!");
        return false;
    }    
    if(!db.commit()){
        if(!db.rollback())
            LOG_SQLERROR("Rollback transaction failed on merge second subject!");
        LOG_SQLERROR("Commit transaction failed on merge second subject!");
        return false;
    }
    return true;
}

/**
 * @brief 在科目系统衔接映射表中的子目映射项中的待合并子目的id替换为保留子目的id
 * @param preSub        保留子目
 * @param mergedSubs    待合并子目
 * @return
 */
bool DbUtil::replaceMapSidWithReserved(SecondSubject *preSub, QList<SecondSubject *> mergedSubs)
{
    QSqlQuery q(db);
    QString tableName = QString("%1_%2_%3").arg(tbl_ssjc_pre).arg(DEFAULT_SUBSYS_CODE)
            .arg(preSub->getParent()->parent()->getSubSysCode());
    QString s = QString("select id,%1 from %2 where %3=%4").arg(fld_ssjc_ssubMaps)
            .arg(tableName).arg(fld_ssjc_dSub).arg(preSub->getParent()->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first())
        return true;
    QList<int> ids;
    foreach(SecondSubject* sub, mergedSubs)
        ids<<sub->getId();
    int rid = q.value(0).toInt();
    QString str = q.value(1).toString();
    QStringList maps = str.split(",");
    for(int i = 1; i < maps.count(); i+=2){
        int id = maps.at(i).toInt();
        if(ids.contains(id))
            maps[i] = QString::number(preSub->getId());
    }
    str = maps.join(",");
    s = QString("update %1 set %2='%3' where id=%4").arg(tableName)
            .arg(fld_ssjc_ssubMaps).arg(str).arg(rid);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::initMoneys
 *  初始化当前账户所使用的所有货币对象
 * @param moneys
 * @return
 */
bool DbUtil::initMoneys(Account *account)
{
    QSqlQuery q(db);
    QHash<int,Money*> mts;
    if(!AppConfig::getInstance()->getSupportMoneyType(mts)){
        LOG_ERROR("Don't get application support money type!");
        return false;
    }
    QString s = QString("select %1,%2 from %3").arg(fld_mt_code).arg(fld_mt_isMaster).arg(tbl_moneyType);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int mmt = 0;
    while(q.next()){
        int code = q.value(0).toInt();
        if(!mts.contains(code)){
            LOG_ERROR("fonded a unknowed money type on initail moneys!");
            return false;
        }
        account->moneys[code] = mts.value(code);
        bool isMain = q.value(1).toBool();
        if(isMain){
            masterMt = code;
            mmt = code;
        }
    }
    //初始化账户信息结构中的母币和外币
    QHashIterator<int,Money*> it(account->moneys);
    while(it.hasNext()){
        it.next();
        if(mmt == it.key())
            account->accInfos.masterMt = it.value();
        else
            account->accInfos.waiMts<<it.value();
    }
    if(mmt == 0)
        QMessageBox::warning(0,QObject::tr("设置错误"),QObject::tr("没有设置本币！"));

    return true;
}

/**
 * @brief DbUtil::initBanks
 *  初始化账户使用的银行账户及其对应的科目
 * @param banks
 * @return
 */
bool DbUtil::initBanks(Account *account)
{
    if(!db.transaction()){
        LOG_SQLERROR("start transaction failed on init bank!");
        return false;
    }
    QSqlQuery q(db), q2(db);
    QString s = QString("select * from %1").arg(tbl_bank);
    if(!q.exec(s))
        return false;
    while(q.next()){
        Bank* bank = new Bank;
        bank->id = q.value(0).toInt();
        bank->isMain = q.value(BANK_ISMAIN).toBool();
        bank->name = q.value(BANK_NAME).toString();
        bank->lname = q.value(BANK_LNAME).toString();
        account->banks<<bank;
        s = QString("select * from %1 where %2=%3")
                .arg(tbl_bankAcc).arg(fld_bankAcc_bankId).arg(bank->id);
        if(!q2.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q2.next()){
            BankAccount* ba = new BankAccount;
            ba->parent = bank;
            ba->id = q2.value(0).toInt();
            int mt = q2.value(BA_MT).toInt();
            int nid = q2.value(BA_ACCNAME).toInt();
            ba->accNumber = q2.value(BA_ACCNUM).toString();
            ba->mt = account->moneys.value(mt);
            ba->niObj = SubjectManager::getNameItem(nid);
            bank->bas<<ba;
        }
    }

//    s = QString("select * from %1").arg(tbl_bankAcc);
//    if(!q.exec(s))
//        return false;
//    while(q.next()){
//        BankAccount* ba = new BankAccount;
//        ba->id = q.value(0).toInt();
//        int bankId = q.value(BA_BANKID).toInt();
//        int mt = q.value(BA_MT).toInt();
//        int nid = q.value(BA_ACCNAME).toInt();
//        ba->bank = banks.value(bankId);
//        ba->accNumber = q.value(BA_ACCNUM).toString();
//        ba->mt = account->moneys.value(mt);
//        ba->niObj = SubjectManager::getNameItem(nid);
//        account->bankAccounts<<ba;
//    }
    if(!db.commit()){
        LOG_SQLERROR("commit transaction failed on init bank!");
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::moneyIsUsed
 *  指定货币对象是否已被账户使用
 *  检测依据定为（余额指针表、分录表和银行账户是否有指定货币）
 * @param mt
 * @return
 */
bool DbUtil::moneyIsUsed(Money *mt, bool &used)
{
    QSqlQuery q(db);
    used = false;
    QString s = QString("select id from %1 where %2=%3")
            .arg(tbl_nse_point).arg(fld_nse_mt).arg(mt->code());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first()){
        used = true;
        return true;
    }
    s = QString("select id from %1 where %2=%3").arg(tbl_ba).arg(fld_ba_mt).arg(mt->code());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first()){
        used = true;
        return true;
    }
    s = QString("select id from %1 where %2=%3").arg(tbl_bankAcc).arg(fld_bankAcc_mt).arg(mt->code());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first())
        used = true;
    return true;

}

/**
 * @brief DbUtil::saveMoneys
 *  保存账户所使用的外币
 * @param moneys
 * @return
 */
bool DbUtil::saveMoneys(QList<Money *> moneys)
{
    QSqlQuery q(db);
    QString s = QString("select %1 from %2 where %3=%4").arg(fld_mt_code)
            .arg(tbl_moneyType).arg(fld_mt_isMaster).arg(0);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QList<int> mtCodes;
    while(q.next())
        mtCodes<<q.value(0).toInt();
    foreach(Money* mt, moneys){
        if(!mtCodes.contains(mt->code())){
            s = QString("insert into %1(%2,%3,%4,%5) values(%6,'%7','%8',0)")
                    .arg(tbl_moneyType).arg(fld_mt_code).arg(fld_mt_name)
                    .arg(fld_mt_sign).arg(fld_mt_isMaster).arg(mt->code())
                    .arg(mt->name()).arg(mt->sign());
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        else
            mtCodes.removeOne(mt->code());
        moneys.removeOne(mt);
    }
    if(!mtCodes.isEmpty()){
        foreach(int code, mtCodes){
            s = QString("delete from %1 where %2=%3").arg(tbl_moneyType).arg(fld_mt_code).arg(code);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 从货币表中移除指定货币
 * @param mt
 * @return
 */
bool DbUtil::removeMoney(Money *mt)
{
    if(!mt)
        return false;
    QSqlQuery q(db);
    QString s = QString("delete from %1 where %2=%3").arg(tbl_moneyType)
            .arg(fld_mt_code).arg(mt->code());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief 加入指定货币到货币表
 * @param mt
 * @return
 */
bool DbUtil::addMoney(Money *mt)
{
    if(!mt)
        return false;
    QSqlQuery q(db);
    QString s = QString("select count() from %1 where %2=%3").arg(tbl_moneyType)
            .arg(fld_mt_code).arg(mt->code());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    q.first();
    if(q.value(0).toInt() > 0)
        return true;
    s = QString("insert into %1(%2,%3,%4,%5) values(%6,'%7','%8',0)").arg(tbl_moneyType)
            .arg(fld_mt_code).arg(fld_mt_sign).arg(fld_mt_name).arg(fld_mt_isMaster)
            .arg(mt->code()).arg(mt->sign()).arg(mt->name());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::scanPzSetCount
 *  统计凭证集内各类凭证的总数
 * @param y         年
 * @param m         月
 * @param repeal    作废凭证数
 * @param recording 正在录入的凭证数
 * @param verify    审核通过的凭证数
 * @param instat    已入账的凭证数
 * @param amount    凭证总数
 * @return
 */
bool DbUtil::scanPzSetCount(int y, int m, int &repeal, int &recording, int &verify, int &instat, int &amount)
{
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select %1 from %2 where %3 like '%4%'")
            .arg(fld_pz_state).arg(tbl_pz).arg(fld_pz_date).arg(ds);
    if(!q.exec(s))
        return false;
    repeal=0;recording=0;verify=0;instat=0;amount=0;
    PzState state;
    while(q.next()){
        amount++;
        state = (PzState)q.value(0).toInt();
        switch(state){
        case Pzs_Repeal:
            repeal++;
            break;
        case Pzs_Recording:
            recording++;
            break;
        case Pzs_Verify:
            verify++;
            break;
        case Pzs_Instat:
            instat++;
            break;
        }
    }
    return true;
}

/**
 * @brief DbUtil::readAllExtraForSSubMMt
 *  读取指定年月，指定二级科目集合的原币余额
 *  此方法主要用于在创建结转损益凭证时，一次性读取所有损益类二级科目的余额
 * @param y
 * @param m
 * @param mt    币种
 * @param sids  二级科目id集合
 * @param vs    余额（键为二级科目的id）
 * @param dirs  余额方向
 * @return
 */
bool DbUtil::readAllExtraForSSubMMt(int y, int m, int mt, QList<int> sids, QHash<int, Double> &vs, QHash<int, MoneyDirection> &dirs)
{
    QSqlQuery q(db);
    QString s;
    int pid;
    if(!_readExtraPoint(y,m,mt,pid))
        return false;
    s = QString("select %1,%2,%3 from %4 where %5=%6")
            .arg(fld_nse_value).arg(fld_nse_dir).arg(fld_nse_sid)
            .arg(tbl_nse_p_s).arg(fld_nse_pid).arg(pid);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int sid;
    MoneyDirection dir;
    Double v;
    while(q.next()){
        sid = q.value(2).toInt();
        if(!sids.contains(sid))
            continue;
        dir = (MoneyDirection)q.value(1).toInt();
        v = Double(q.value(0).toDouble());
        vs[sid] = v;
        dirs[sid] = dir;
    }
    return true;
}

/**
 * @brief DbUtil::readAllWbExtraForFSub
 *  读取指定年、月、一级科目下的所有指定币种的二级科目原币形式的余额
 *  此方法用来在结转汇兑损益时，为了提高读取余额的性能而一次性读取指定一级科目下的所有二级科目余额
 * @param y
 * @param m
 * @param sids  指定一级科目下的所有二级科目的id
 * @param mts   要读取余额的币种代码列表
 * @param vs    余额（原币形式）（键为二级科目id * 10 + 币种代码）
 * @param dirs  余额方向
 * @return
 */
bool DbUtil::readAllWbExtraForFSub(int y, int m, QList<int> sids, QList<int> mts, QHash<int, Double> &vs, QHash<int, MoneyDirection> &dirs)
{
    QSqlQuery q(db);
    QString s;
    QHash<int,int> pids;
    int pid;
    foreach(int mt, mts){
        if(!_readExtraPoint(y,m,mt,pid))
            return false;
        pids[mt] = pid;
    }
    QHashIterator<int,int> it(pids);
    while(it.hasNext()){
        it.next();
        s = QString("select %1,%2,%3 from %4 where %5=%6")
                .arg(fld_nse_value).arg(fld_nse_dir).arg(fld_nse_sid)
                .arg(tbl_nse_p_s).arg(fld_nse_pid).arg(it.value());
        if(!q.exec(s))
            return false;
        int sid,key;
        while(q.next()){
            sid = q.value(2).toInt();
            if(!sids.contains(sid))
                continue;
            key = sid*10+it.key();
            vs[key] = Double(q.value(0).toDouble());
            dirs[key] = (MoneyDirection)q.value(1).toInt();
        }
    }
    return true;
}

/**
 * @brief DbUtil::readExtraForMF
 *  读取指定年、月、币种、一级科目的余额
 * @param y
 * @param m
 * @param mt    币种代码
 * @param fid   一级科目id
 * @param v     余额（原币形式）
 * @param wv    余额（本币形式）
 * @param dir   方向
 * @return
 */
bool DbUtil::readExtraForMF(int y, int m, int mt, int fid, Double& v, Double& wv, MoneyDirection& dir)
{
    return _readExtraForSubMoney(y,m,mt,fid,v,wv,dir);
}

/**
 * @brief DbUtil::readExtraForMS
 *  读取指定年、月、币种、二级科目的余额
 * @param y
 * @param m
 * @param mt    币种代码
 * @param sid   二级科目id
 * @param v     余额（原币形式）
 * @param wv    余额（本币形式）
 * @param dir   方向
 * @return
 */
bool DbUtil::readExtraForMS(int y, int m, int mt, int sid, Double &v, Double &wv, MoneyDirection& dir)
{
    return _readExtraForSubMoney(y,m,mt,sid,v,wv,dir,false);
}

/**
 * @brief DbUtil::readExtraForFSub
 *  读取指定一级科目在指定年月的各币种余额
 * @param y     年
 * @param m     月
 * @param fid   一级科目id
 * @param v     余额（原币形式）
 * @param wv    余额（本币形式）
 * @param dir   余额方向
 * @return
 */
bool DbUtil::readExtraForFSub(int y, int m, int fid, QHash<int, Double> &v, QHash<int, Double> &wv, QHash<int, MoneyDirection> &dir)
{
    return _readExtraForFSub(y,m,fid,v,wv,dir);
}

/**
 * @brief DbUtil::readExtraForSSub
 *  读取指定一级科目在指定年月的各币种余额
 * @param y     年
 * @param m     月
 * @param sid   二级科目id
 * @param v     余额（原币形式）
 * @param wv    余额（本币形式）
 * @param dir   余额方向
 * @return
 */
bool DbUtil::readExtraForSSub(int y, int m, int sid, QHash<int, Double> &v, QHash<int, Double> &wv, QHash<int, MoneyDirection> &dir)
{
    return _readExtraForSSub(y,m,sid,v,wv,dir);
}



//bool DbUtil::saveExtraForSSub(int y, int m, int fid, const QHash<int, Double> &v, const QHash<int, Double> &wv, const QHash<int, MoneyDirection> &dir)
//{
//    return _saveExtraForSub(y,m,fid,v,wv,dir,false);
//}

/**
 * @brief DbUtil::readExtraForPm
 *  读取指定年月所有科目（包括二级科目）的余额值（原币形式）
 * @param y     年
 * @param m     月
 * @param fsums 一级科目余额值（注意：hash表的键是 “科目代码 * 10 + 币种代码”）
 * @param fdirs 一级科目余额方向
 * @param ssums 二级科目余额值
 * @param sdirs 二级科目方向
 * @return
 */
bool DbUtil::readExtraForPm(int y, int m, QHash<int, Double> &fsums, QHash<int, MoneyDirection> &fdirs,
                            QHash<int, Double> &ssums, QHash<int, MoneyDirection> &sdirs)
{
    fsums.clear();fdirs.clear();ssums.clear();sdirs.clear();
    if(!_readExtraForPm(y,m,fsums,fdirs))
        return false;
    if(!_readExtraForPm(y,m,ssums,sdirs,false))
        return false;
    return true;
}

/**
 * @brief DbUtil::readExtraForMm
 *  读取指定年月的余额值（本币形式）
 * @param y     年
 * @param m     月
 * @param fsums 一级科目余额值（注意：hash表的键是 “科目代码 * 10 + 币种代码”）
 * @param fdirs 一级科目余额方向
 * @param ssums 二级科目余额值
 * @param sdirs 二级科目方向
 * @param m
 * @param fsums
 * @param fdirs
 * @param ssums
 * @param sdirs
 * @return
 */
bool DbUtil::readExtraForMm(int y, int m, QHash<int, Double> &fsums, QHash<int, Double> &ssums)
{
    //    if(isNewExtraAccess()){
    //        QHash<int,MoneyDirection> fdirs,sdirs;
    //        //读取以本币计的余额（先从SubjectExtras和detailExtras表读取科目的原币余额值，将其中的外币乘以汇率转换为本币）
    //        if(!BusiUtil::readExtraByMonth3(y,m,fsums,fdirs,ssums,sdirs))
    //            return false;
    //        //从SubjectMmtExtras和detailMmtExtras表读取本币形式的余额，仅包含外币部分）
    //        bool exist;
    //        QHash<int,Double> fsumRs,ssumRs;
    //        if(!BusiUtil::readExtraByMonth4(y,m,fsumRs,ssumRs,exist))
    //            return false;
    //        //用精确值代替直接从原币币转换来的外币值
    //        if(exist){
    //            QHashIterator<int,Double>* it = new QHashIterator<int,Double>(fsumRs);
    //            while(it->hasNext()){
    //                it->next();
    //                fsums[it->key()] = it->value();
    //            }
    //            it = new QHashIterator<int,Double>(ssumRs);
    //            while(it->hasNext()){
    //                it->next();
    //                ssums[it->key()] = it->value();
    //            }
    //        }
    //        return true;
    //    }

    if(!_readExtraForMm(y,m,fsums))
        return false;
    if(!_readExtraForMm(y,m,ssums,false))
        return false;
    //_replaeAccurateExtra();
    return true;
}

/**
 * @brief DbUtil::saveExtraForPm
 *  保存指定年月的期末余额（原币形式）
 * @param y     年
 * @param m     月
 * @param fsums 一级科目余额表（注意：键为“科目id * 10 + 币种代码”）
 * @param fdirs 一级科目余额方向
 * @param ssums 二级科目余额表
 * @param sdirs 二级科目余额方向
 * @return
 */
bool DbUtil::saveExtraForPm(int y, int m, const QHash<int, Double> &fsums, const QHash<int, MoneyDirection> &fdirs, const QHash<int, Double> &ssums, const QHash<int, MoneyDirection> &sdirs)
{
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("When save extra for primary money, open database transaction failed!"));
        return false;
    }
    if(!_saveExtrasForPm(y,m,fsums,fdirs))
        return false;
    if(!_saveExtrasForPm(y,m,ssums,sdirs,false))
        return false;
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("When save extra for primary money, transaction commit failed!"));
        if(!db.rollback())
            warn_transaction(Transaction_rollback,QObject::tr("When save extra for primary money,Database transaction roll back failed!"));
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::saveExtraForMm
 *  保存指定年月的期末余额（本币形式）
 * @param y
 * @param m
 * @param fsums 一级科目余额表（注意：键为“科目id * 10 + 币种代码”）
 * @param ssums 二级科目余额表
 * @return
 */
bool DbUtil::saveExtraForMm(int y, int m, const QHash<int, Double> &fsums, const QHash<int, Double> &ssums)
{
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("When save extra for master money, open database transaction failed!"));
        return false;
    }
    if(!_saveExtrasForMm(y,m,fsums))
        return false;
    if(!_saveExtrasForMm(y,m,ssums,false))
        return false;
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("When save extra for master money, transaction commit failed!"));
        if(!db.rollback())
            warn_transaction(Transaction_rollback,QObject::tr("When save extra for master money,Database transaction roll back failed!"));
        return false;
    }
    return true;
}

/**
 * @brief 验证子目的汇总余额是否和主目的余额一致
 * @param y
 * @param m
 * @param fsub
 * @return
 */
bool DbUtil::verifyExtraForFsub(int y, int m, FirstSubject *fsub)
{
    QHash<int,Double> pvs,mvs;
    QHash<int,MoneyDirection> dirs;
    readExtraForAllSSubInFSub(y,m,fsub,pvs,dirs,mvs);
    QHashIterator<int,Double> it(pvs);
    Double sum,v;
    MoneyDirection dir=MDIR_P;
    while(it.hasNext()){
        it.next();
        v = it.value();
        if(dirs.value(it.key()) == DIR_D)
            v.changeSign();
        sum += v;
    }


    QSqlQuery q(db);
    int pid;
    _readExtraPoint(y,m,1,pid);
    QString s = QString("select %1,%2 from %3 where %4=%5 and %6=%7")
            .arg(fld_nse_value).arg(fld_nse_dir).arg(tbl_nse_p_f)
            .arg(fld_nse_pid).arg(pid).arg(fld_nse_sid).arg(fsub->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    MoneyDirection dir2 = MDIR_P;
    Double sum2;
    if(q.first()){
        dir2 = (MoneyDirection)q.value(1).toInt();
        sum2 = Double(q.value(0).toDouble());
    }
    if(sum == 0){
        dir = MDIR_P;
        if(dir2 == MDIR_P)
            return true;
    }
    else if(dir == dir2 && sum == sum2){
        return true;
    }
    else if((int)dir + (int)dir2 == 0){
        sum.changeSign();
        if(sum == sum2)
            return true;
        else
            return false;
    }
    return false;
}

/**
 * @brief DbUtil::readExtraForAllSSubInFSub
 *  读取指定年月，指定一级科目下的所有二级科目的余额（主要用于设置账户期初余额时使用）
 * @param y
 * @param m
 * @param fid   一级科目id
 * @param pvs   余额（原币）（键为二级科目id + 币种代码）
 * @param dirs  余额方向
 * @param mvs   余额（本币）
 * @return
 */
bool DbUtil::readExtraForAllSSubInFSub(int y, int m, FirstSubject* fsub, QHash<int, Double> &pvs, QHash<int, MoneyDirection> &dirs, QHash<int, Double> &mvs)
{
    QSqlQuery q(db),q2(db);
    QHash<int, int> pids;
    if(!fsub)
        return false;
    if(!_readExtraPoint(y,m,pids))
        return false;
    QList<int> sids;
    foreach(SecondSubject* ssub, fsub->getChildSubs())
        sids<<ssub->getId();
    QString s;

    QHashIterator<int,int> it(pids);
    MoneyDirection dir; int sid,key; Double v;
    int mmt = fsub->parent()->getAccount()->getMasterMt()->code();
    while(it.hasNext()){
        it.next();
        s = QString("select %1,%2,%3 from %4 where %5=%6").arg(fld_nse_dir).arg(fld_nse_value)
                .arg(fld_nse_sid).arg(tbl_nse_p_s).arg(fld_nse_pid).arg(it.value());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next()){
            sid = q.value(2).toInt();
            if(!sids.contains(sid))
                continue;
            dir = (MoneyDirection)q.value(0).toInt();
            v = Double(q.value(1).toDouble());
            key = sid * 10 + it.key();
            pvs[key] = v;
            dirs[key] = dir;
            if(it.key() != mmt){
                s = QString("select %1,%2 from %3 where %4=%5").arg(fld_nse_sid).arg(fld_nse_value)
                        .arg(tbl_nse_m_s).arg(fld_nse_pid).arg(it.value());
                if(!q2.exec(s)){
                    LOG_SQLERROR(s);
                    return false;
                }
                while(q2.next()){
                    sid = q2.value(0).toInt();
                    if(!sids.contains(sid))
                        continue;
                    v = Double(q2.value(1).toDouble());
                    key = sid * 10 + it.key();
                    mvs[key] = v;
                }
            }
        }
    }
    return true;
}

/**
 * @brief DbUtil::saveExtraForAllSSubInFSub
 *  保存指定年月、指定一级科目的余额（包括其属下的所有二级科目余额）
 * @param y
 * @param m
 * @param fid   一级科目id
 * @param fpvs  一级科目余额（原币）（键为币种代码）
 * @param fmvs  一级科目余额（本币）（键为币种代码）
 * @param fdirs 一级科目余额方向（键为币种代码）
 * @param v     二级科目余额（原币）（键为科目id*10+币种代码）
 * @param wv    二级科目余额（外币）（键为科目id*10+币种代码）
 * @param dir   二级科目余额方向（键为科目id*10+币种代码）
 * @return
 * @return
 */
bool DbUtil::saveExtraForAllSSubInFSub(int y, int m, FirstSubject* fsub,
                              const QHash<int, Double> fpvs, const QHash<int, Double> fmvs,
                              QHash<int, MoneyDirection> fdirs, const QHash<int, Double> &v,
                              const QHash<int, Double> &wv, const QHash<int, MoneyDirection> &dir)
{
    if(!db.transaction()){
        LOG_SQLERROR("Start transaction failed on save first subject extra!");
        return false;
    }

    QHash<int, Double> pvs,mvs;
    QHash<int,MoneyDirection> dirs;
    int key;

    //保存一级科目余额
    QHashIterator<int,Double>* it = new QHashIterator<int,Double>(fpvs);    
    while(it->hasNext()){
        it->next();
        if(fdirs.value(it->key() == MDIR_P))
            continue;
        key = fsub->getId() * 10 + it->key();
        pvs[key] = it->value();
        dirs[key] = fdirs.value(it->key());
    }
    it = new QHashIterator<int,Double>(fmvs);
    while(it->hasNext()){
        it->next();
        if(it->value() == 0)
            continue;
        key = fsub->getId() * 10 + it->key();
        mvs[key] = it->value();
    }

    QList<int> ids;
    ids<<fsub->getId();
    if(!_saveExtrasForSubLst(y,m,ids,pvs,mvs,dirs))
        return false;

    //保存二级科目余额
    ids.clear();
    pvs.clear(); mvs.clear(); dirs.clear();
    foreach(SecondSubject* ssub, fsub->getChildSubs())
        ids<<ssub->getId();
    it = new QHashIterator<int,Double>(v);
    while(it->hasNext()){
        it->next();
        if(dir.value(it->key() == MDIR_P))
            continue;
        pvs[it->key()] = it->value();
        dirs[it->key()] = dir.value(it->key());
    }
    it = new QHashIterator<int,Double>(wv);
    while(it->hasNext()){
        it->next();
        if(it->value() == 0)
            continue;
        mvs[it->key()] = it->value();
    }
    if(!_saveExtrasForSubLst(y,m,ids,pvs,mvs,dirs,false))
        return false;

    if(!db.commit()){
        LOG_SQLERROR("Commit transaction failed on save first subject extra!");
        if(!db.rollback())
            LOG_SQLERROR("Rollback transaction failed on save first subject extra!");
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::convertExtraInYear
 *  转换指定年份内的余额，即将混合对接科目的余额并入对接科目上后删除原余额条目
 * @param year
 * @param maps 科目映射表（键为源科目id，值为混合对接科目id）
 * @param isFst true：主目，false：子目
 * @return
 */
bool DbUtil::convertExtraInYear(int year, const QHash<int, int> maps,bool isFst)
{

    if(!db.transaction()){
        LOG_SQLERROR(QString("Start transaction failed on convert extra in %1 year!").arg(year));
        return false;
    }

    if(!_convertExtraInYear(year,maps,isFst)){
        db.rollback();
        return false;
    }

    if(!db.commit()){
        LOG_SQLERROR(QString("Commit transaction failed on convert extra in %1 year!").arg(year));
        if(!db.rollback())
            LOG_SQLERROR(QString("Roolback transaction failed on convert extra in %1 year!").arg(year));
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::convertPzInYear
 *  转换指定年份内的所有凭证
 *  即将凭证内的会计分录中的那些混合对接科目id替换为对应科目所使用的科目id
 * @param year  帐套年份
 * @param fMaps 主目id映射表（这些科目的对接关系都保存在子目对接表“sndSubJoin"_1_2”中）
 * @param sMaps 子目id映射表
 * @return
 */
bool DbUtil::convertPzInYear(int year, const QHash<int, int> fMaps,
                             const QHash<int, int> sMaps)
{
    QSqlQuery q1(db),q2(db);
    QString s1, s2;
    QList<int> ps;
    if(!db.transaction()){
        LOG_SQLERROR(QString("Start transaction failed on convert PingZheng in %1 year!").arg(year));
        return false;
    }

    //select BusiActions.id,BusiActions.fid,BusiActions.sid,PingZhengs.date,PingZhengs.number,BusiActions.numInPz from PingZhengs join BusiActions on PingZhengs.id=BusiActions.pid where PingZhengs.date like '2013%'

    s1 = QString("select %1.id,%1.%6,%1.%7,%2.%8,%2.%9,%1.%10 from %2 join %1 on %2.id=%1.%3 where %2.%4 like '%5%'")
            .arg(tbl_ba).arg(tbl_pz).arg(fld_ba_pid).arg(fld_pz_date).arg(year)
            .arg(fld_ba_fid).arg(fld_ba_sid).arg(fld_pz_date).arg(fld_pz_number)
            .arg(fld_ba_number);
    if(!fMaps.isEmpty()){
        s1.append(QString(" and ("));
        foreach(int fid, fMaps.keys()){
            s1.append(QString("%1=%2 or ").arg(fld_ba_fid).arg(fid));
        }
        s1.chop(4);
        s1.append(")");
    }
    if(!q1.exec(s1)){
        LOG_SQLERROR(s1);
        return false;
    }
    s2 = QString("update %1 set %2=:fid,%3=:sid where id=:id").arg(tbl_ba)
            .arg(fld_ba_fid).arg(fld_ba_sid);
    if(!q2.prepare(s2)){
        LOG_SQLERROR(s2);
        return false;
    }
    QString date;
    int pzNum,baNum;
    int id,fid,sid;
    int nums;
    while(q1.next()){
        fid = q1.value(1).toInt();
        id = q1.value(0).toInt();        
        sid = q1.value(2).toInt();        
        date = q1.value(3).toString();
        pzNum = q1.value(4).toInt();
        baNum = q1.value(5).toInt();
        if(!sMaps.contains(sid)){
            LOG_ERROR(QObject::tr("在升级科目系统期间，发现一个未配置的二级科目，在%1，%2#凭证的第%3条分录（id=%4，）包含无效的科目（fid=%5，sid=%6）")
                                          .arg(date).arg(pzNum).arg(baNum).arg(id).arg(fid).arg(sid));
            db.rollback();
            return false;
        }
        q2.bindValue(":fid", fMaps.value(fid));
        q2.bindValue(":sid", sMaps.value(sid));
        q2.bindValue(":id", id);
        if(!q2.exec())
            return false;
        nums = q2.numRowsAffected();
    }

    if(!db.commit()){
        LOG_SQLERROR(QString("Commit transaction failed on convert PingZheng in %1 year!").arg(year));
        if(!db.rollback())
            LOG_SQLERROR(QString("Roolback transaction failed on convert PingZheng in %1 year!").arg(year));
        return false;
    }
    return true;
}

/**
 * @brief 指定一级科目的外币余额是否为零
 * 这个要参考最后两个月的凭证集，后一个月未结账，前一个月已结账，且前一个月没有外币余额，而后一个月
 * 凭证集内没有包含使用此科目和外币的分录存在。只有这样才能视为可以调整为不使用外币。
 * 但这样还有一个问题，如果要查看前面凭证集，比如统计或明细账，则会因为使用调整为不使用外币而无法显示
 * 涉及到外币的分录。
 * @param ssub
 * @return
 */
bool DbUtil::lastWbExtraIsZeroForFSub(FirstSubject *ssub)
{
    return true;
}

/**
 * @brief 获取混合对接科目配置信息
 * @param sc
 * @param dc
 * @param cfgInfos
 * @return
 */
bool DbUtil::getMixJoinInfo(int sc, int dc, QList<MixedJoinCfg *>& cfgInfos)
{
    QSqlQuery q(db);
    QString tname = QString("%1_%2_%3").arg(tbl_sndsub_join_pre).arg(sc).arg(dc);
    QString s = QString("select * from %1 order by %2").arg(tname).arg(fld_ssj_s_fsub);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        MixedJoinCfg* item = new MixedJoinCfg;
        item->s_fsubId = q.value(SSJ_SF_SUB).toInt();
        item->s_ssubId = q.value(SSJ_SS_SUB).toInt();
        item->d_fsubId = q.value(SSJ_DF_SUB).toInt();
        item->d_ssubId = q.value(SSJ_DS_SUB).toInt();
        cfgInfos<<item;
    }
    return true;
}

/**
 * @brief 添加二级混合对接配置信息
 * @param sc
 * @param dc
 * @param cfgInfos
 * @return
 */
bool DbUtil::appendMixJoinInfo(int sc, int dc, QList<MixedJoinCfg *> cfgInfos)
{
    QSqlQuery q(db);
    QString tname = QString("%1_%2_%3").arg(tbl_sndsub_join_pre).arg(sc).arg(dc);
    QString s = QString("insert into %1(%1,%2,%3,%4) values(:sfid,:ssid,:dfid,:dsid)")
            .arg(tname).arg(fld_ssj_s_fsub).arg(fld_ssjc_sSub).arg(fld_ssj_s_fsub)
            .arg(fld_ssjc_sSub);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    foreach(MixedJoinCfg* item,cfgInfos){
        q.bindValue(":sfid",item->s_fsubId);
        q.bindValue(":ssid",item->s_ssubId);
        q.bindValue(":dfid",item->d_fsubId);
        q.bindValue(":dsid",item->d_ssubId);
        if(!q.exec()){
            LOG_SQLERROR(q.lastQuery());
            return false;
        }
    }
    return true;
}

/**
 * @brief DbUtil::getDetViewFilters
 *  获取明细账视图的历史过滤条件记录
 * @param rs
 * @return
 */
bool DbUtil::getDetViewFilters(int suiteId, QList<DVFilterRecord *> &rs)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2=%3").arg(tbl_dvfilters)
            .arg(fld_dvfs_suite).arg(suiteId);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    DVFilterRecord* r;
    QStringList subs;
    while(q.next()){
        r = new DVFilterRecord;
        r->editState = CIES_INIT;
        r->id = q.value(0).toInt();
        r->suiteId = q.value(DVFS_SUITEID).toInt();
        r->isDef = q.value(DVFS_ISDEF).toBool();
        r->isCur = q.value(DVFS_ISCUR).toBool();
        r->isFst = q.value(DVFS_ISFST).toBool();
        r->curFSub = q.value(DVFS_CURFSUB).toInt();
        r->curSSub = q.value(DVFS_CURSSUB).toInt();
        r->curMt =  q.value(DVFS_MONEYTYPE).toInt();
        r->name = q.value(DVFS_NAME).toString();
        r->startDate = QDate::fromString(q.value(DVFS_STARTDATE).toString(),Qt::ISODate);
        r->endDate = QDate::fromString(q.value(DVFS_ENDDATE).toString(),Qt::ISODate);
        s = q.value(DVFS_SUBIDS).toString();
        if(!s.isEmpty()){
            subs = s.split(",");
            foreach(QString sid, subs)
                r->subIds<<sid.toInt();
        }
        rs<<r;
    }
    return true;
}

/**
 * @brief DbUtil::saveDetViewFilter
 *  保存一个明细账视图的过滤条件设置
 * @param dvf
 * @return
 */
bool DbUtil::saveDetViewFilter(const QList<DVFilterRecord*>& dvfs)
{
    QSqlQuery q(db);
    QString s;
    QStringList subIds;

    foreach(DVFilterRecord* dvf, dvfs){
        if(dvf->editState == CIES_INIT)
            continue;
        subIds.clear();
        for(int i = 0; i < dvf->subIds.count(); ++i)
            subIds.append(QString::number(dvf->subIds.at(i)));
        if(dvf->editState == CIES_NEW)
            s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12) "
                        "values(%13,%14,%15,%16,%17,%18,%19,'%20','%21','%22','%23')")
                    .arg(tbl_dvfilters).arg(fld_dvfs_suite).arg(fld_dvfs_isDef).arg(fld_dvfs_isCur)
                    .arg(fld_dvfs_isFstSub).arg(fld_dvfs_curFSub).arg(fld_dvfs_curSSub).arg(fld_dvfs_mt)
                    .arg(fld_dvfs_name).arg(fld_dvfs_startDate).arg(fld_dvfs_endDate)
                    .arg(fld_dvfs_subIds).arg(dvf->suiteId).arg(dvf->isDef?1:0).arg(dvf->isCur?1:0)
                    .arg(dvf->isFst?1:0).arg(dvf->curFSub).arg(dvf->curSSub)
                    .arg(dvf->curMt).arg(dvf->name).arg(dvf->startDate.toString(Qt::ISODate))
                    .arg(dvf->endDate.toString(Qt::ISODate)).arg(subIds.join(","));
        else if(dvf->editState == CIES_CHANGED)
            s = QString("update %1 set %2=%3,%4=%5,%6=%7,%8=%9,%10=%11,%12=%13,%14=%15,"
                        "%16='%17',%18='%19',%20='%21',%22='%23' where id=%24")
                    .arg(tbl_dvfilters).arg(fld_dvfs_suite).arg(dvf->suiteId).arg(fld_dvfs_isDef).arg(dvf->isDef?1:0)
                    .arg(fld_dvfs_isCur).arg(dvf->isCur?1:0).arg(fld_dvfs_isFstSub)
                    .arg(dvf->isFst?1:0).arg(fld_dvfs_curFSub).arg(dvf->curFSub)
                    .arg(fld_dvfs_curSSub).arg(dvf->curSSub)
                    .arg(fld_dvfs_mt).arg(dvf->curMt).arg(fld_dvfs_name).arg(dvf->name)
                    .arg(fld_dvfs_startDate).arg(dvf->startDate.toString(Qt::ISODate))
                    .arg(fld_dvfs_endDate).arg(dvf->endDate.toString(Qt::ISODate))
                    .arg(fld_dvfs_subIds).arg(subIds.join(",")).arg(dvf->id);
        else if(dvf->editState == CIES_DELETED)
            s = QString("delete from %1 where id=%2").arg(tbl_dvfilters).arg(dvf->id);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(dvf->editState == CIES_NEW){
            if(!q.exec("select last_insert_rowid()")){
                LOG_SQLERROR(s);
                return false;
            }
            q.first();
            dvf->id = q.value(0).toInt();
        }
        dvf->editState = CIES_INIT;
    }
    return true;
}

/**
 * @brief DbUtil::getDailyAccount2
 *  获取指定时间范围内、指定科目范围、符合指定条件的日记账数据
 * @param smgs          每个帐套年份对应的科目管理器对象
 * @param sd            搜索的开始日期
 * @param ed            搜索的结束日期
 * @param fid           一级科目id（0表示所有一级科目）
 * @param sid           二级科目id（0表示指定一级科目下的所有二级科目）
 * @param mt            币种代码
 * @param prev          期初总余额（各币种合计值）（键是科目代码）
 * @param preDir        期初方向
 * @param datas         发生项数据
 * @param preExtra      期初值（原币形式）（这些键是科目代码和币种构成的复合键）
 * @param preExtraR     期初值（本币形式）
 * @param preExtraDir   期初余额方向
 * @param rates         每月的汇率，键为年月和币种的复合键（高4位年，中2位月，低1位币种，其中，期初余额是个位数，用币种代码表示）
 * @param subIds        指定的科目代码（当fid=0时包含一级科目代码，反之包含有fid指定的一级科目下的二级科目的id）
 //* @param sids          所有在fids中指定的总账科目所属的明细科目代码总集合
 * @param gv            要提取的会计分录的值的上界
 * @param lv            要提取的会计分录的值的下界
 * @param inc           是否包含未记账凭证
 * @return
 */
bool DbUtil::getDailyAccount2(QHash<int, SubjectManager*> smgs, QDate sd, QDate ed, int fid, int sid, int mt, Double &prev, int &preDir, QList<DailyAccountData2 *> &datas, QHash<int, Double> &preExtra, QHash<int, Double> &preExtraR, QHash<int, int> &preExtraDir, QHash<int, Double> &rates, QList<int> subIds, /*QHash<int, QList<int> > sids,*/ Double gv, Double lv, bool inc)
{
    QSqlQuery q(db);
    QString s;

    //读取前期余额
    int yy,mm;
    _getPreYM(sd.year(),sd.month(),yy,mm);

    QHash<int,Double> ra;
    QHashIterator<int,Double>* it;

    //获取期初汇率并加入汇率表
    if(!getRates(yy,mm,ra))
        return false;
    //ra[RMB] = 1.00;
    it = new QHashIterator<int,Double>(ra);
    while(it->hasNext()){
        it->next();
        rates[it->key()] = it->value();  //期初汇率的键直接是货币代码
    }

    //只有提取指定总账科目的明细发生项的情况下，读取余额才有意义
    if(fid != 0){
        QHash<int,MoneyDirection>dirs;        
        bool isTrans;
        QHash<int,int> fMaps,sMaps;
        if(sd.month() > 1)
            isTrans = false;
        else{
            if(!_isTransformExtra(sd.year(),isTrans,fMaps,sMaps))
                return false;
        }
        int pre_fid=fid,pre_sid=sid;
        if(isTrans){
            QHashIterator<int,int> it(fMaps);
            while(it.hasNext()){
                it.next();
                if(fid == it.value()){
                    pre_fid = it.key();
                    break;
                }
            }
            if(sid != 0){
                QHashIterator<int,int> it(sMaps);
                while(it.hasNext()){
                    it.next();
                    if(it.value() == sid){
                        pre_sid = it.key();
                        break;
                    }
                }
            }
        }

        //读取总账科目或明细科目的余额
        if(sid == 0){
            if(!_readExtraForFSub(yy,mm,pre_fid,preExtra,preExtraR,dirs))
                return false;
        }
        else{
            if(!_readExtraForSSub(yy,mm,pre_sid,preExtra,preExtraR,dirs))
                return false;
        }


        //因为期初余额值表的键为币种代码，因此，无须进行转换。。。。。
//        if(isTrans){
//            if(sid == 0 && !_transformExtra(fMaps,preExtra,preExtraR,dirs))
//                return false;
//            else if(sid != 0 && !_transformExtra(sMaps,preExtra,preExtraR,dirs))
//                return false;
//        }

        //原先方向是用整形来表示的，而新实现是用枚举类型来实现的，因此先进行转换以使用原先的数据生成代码
        //待验证明细帐功能正确后可以移除
        QHashIterator<int,MoneyDirection> id(dirs);
        while(id.hasNext()){
            id.next();
            preExtraDir[id.key()] = id.value();
        }

        //如果还指定了币种，则只保留该币种的余额
        if(mt != ALLMT){
            QHashIterator<int,Double> it(preExtra);
            while(it.hasNext()){
                it.next();
                if(mt != it.key()){
                    preExtra.remove(it.key());
                    preExtraDir.remove(it.key());
                }
            }
        }
    }


    //以原币形式保存每次发生业务活动后的各币种余额，初值就是前期余额（还要根据前期余额的方向调整符号）
    //将期初余额以统一的方向来表示，以便后期的累加
    QHash<int,Double> esums;
    QHash<int,Double> emsums;
    it = new QHashIterator<int,Double>(preExtra);
    while(it->hasNext()){
        it->next();
        Double v = it->value();
        if(preExtraDir.value(it->key()) == DIR_D)
            v.changeSign();
        esums[it->key()] = v;
    }

    //计算期初总余额
    Double tsums = 0.00;  //保存按母币计的期初总余额及其每次发生后的总余额（将各币种用母币合计）
    QHashIterator<int,Double> i(preExtra);
    while(i.hasNext()){ //计算期初总余额
        i.next();
        int mt = i.key();
        if(preExtraDir.value(mt) == DIR_P)
            continue;
        else if(preExtraDir.value(mt) == DIR_J){
            if(mt == masterMt)
                tsums += i.value();
            else{
                tsums += preExtraR.value(mt);
                emsums[mt] = preExtraR.value(i.key());
            }
        }
        else{
            if(mt == masterMt)
                tsums -= i.value();
            else{
                tsums -= preExtraR.value(mt);
                emsums[mt] = preExtraR.value(i.key());
                emsums[mt].changeSign();
            }
        }
    }
    if(tsums == 0){
        prev = 0.00; preDir = DIR_P;
    }
    else if(tsums > 0){
        prev = tsums; preDir = DIR_J;
    }
    else{
        prev = tsums;
        prev.changeSign();
        preDir = DIR_D;
    }

    //获取指定的开始和结束时间范围的汇率
    QList<int> ms; //在指定的时间范围内的月份列表，每个元素的高四位表示年份，第两位表示月份
    for(int y = sd.year(); y <= ed.year(); ++y){
        int sm,em;
        if(y == sd.year())
            sm = sd.month();
        else
            sm = 1;
        if(y == ed.year())
            em = ed.month();
        else
            em = 12;
        for(int m = sm; m <= em; ++m)
            ms<<y *100 + m;
    }
    ra.clear();
    for(int i = 0; i < ms.count(); ++i){
        int ym = ms.at(i);
        if(!getRates(ym/100,ym%100,ra))
            return false;
        //ra[RMB] = 1.00;
        it = new QHashIterator<int,Double>(ra);
        while(it->hasNext()){
            it->next();
            rates[ym*10+it->key()] = it->value();
        }
        ra.clear();
    }


    //构造查询语句
    QString sdStr = sd.toString(Qt::ISODate);
    QString edStr = ed.toString(Qt::ISODate);
    s = QString("select %1.%2,%1.%3,%4.%5,"
                "%4.id,%4.%6,%4.%7,%4.%8,"
                "%4.%9,%4.%10,"
                "%4.%11,%1.%12 "
                "from %1 join %4 on %4.%6 = %1.id "
                "where (%1.%2 >= '%13') and (%1.%2 <= '%14')")
            .arg(tbl_pz).arg(fld_pz_date).arg(fld_pz_number).arg(tbl_ba).arg(fld_ba_summary)
            .arg(fld_ba_pid).arg(fld_ba_value).arg(fld_ba_mt).arg(fld_ba_dir).arg(fld_ba_fid)
            .arg(fld_ba_sid).arg(fld_pz_class).arg(sdStr).arg(edStr);
    if(fid != 0)
        s.append(QString(" and (%1.%2 = %3)").arg(tbl_ba).arg(fld_ba_fid).arg(fid));
    if(sid != 0)
        s.append(QString(" and (%1.%2 = %3)").arg(tbl_ba).arg(fld_ba_sid).arg(sid));
    if(gv != 0)
        s.append(QString(" and (%1.%2 > %3)")
                 .arg(tbl_ba).arg(fld_ba_value).arg(gv.toString()));
    if(lv != 0)
        s.append(QString(" and (%1.%2 < %3)")
                 .arg(tbl_ba).arg(fld_ba_value).arg(lv.toString()));
    if(!inc) //将已入账的凭证纳入统计范围
        s.append(QString(" and (%1.%2 = %3)").arg(tbl_pz).arg(fld_pz_state).arg(Pzs_Instat));
    else     //将未审核、已审核、已入账的凭证纳入统计范围
        s.append(QString(" and (%1.%2 != %3)").arg(tbl_pz).arg(fld_pz_state).arg(Pzs_Repeal));
    s.append(QString(" order by %1.%2,%1.%3").arg(tbl_pz).arg(fld_pz_date).arg(fld_pz_number));

    if(!q.exec(s))
        return false;

    int mType,fsubId,ssubId;
    PzClass pzCls;
    int cwfyId; //财务费用的科目id

    while(q.next()){
        //id = q.value(8).toInt();  //fid
        mType = q.value(6).toInt();  //业务发生的币种
        pzCls = (PzClass)q.value(10).toInt();    //凭证类别
        fsubId = q.value(8).toInt();    //会计分录所涉及的一级科目id
        //如果要提取所有选定主目，则跳过所有未选定主目
        if(fid == 0){
            if(!subIds.contains(fsubId))
                continue;
        }
        //如果选择了某个主目并选择所有子目，则过滤掉所有未选定子目
        if((fid != 0) && (sid == 0)){
            ssubId = q.value(9).toInt();
            if(!subIds.contains(ssubId))
                continue;
        }

        QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        cwfyId = smgs.value(d.year())->getCwfySub()->getId();
        //当前凭证是否是结转汇兑损益的凭证
        bool isJzhdPz = pzClsJzhds.contains(pzCls);
        //如果是结转汇兑损益的凭证作特别处理
        if(isJzhdPz){
            if(mt == RMB && fid != cwfyId) //如果指定的是人民币，则跳过非财务费用方的会计分录
                continue;
        }

        //对于非结转汇兑损益的凭证，如果指定了币种，则跳过非此币种的会计分录
        if((mt != 0 && mt != mType && !isJzhdPz))
            continue;

        DailyAccountData2* item = new DailyAccountData2;
        //凭证日期

        item->y = d.year();
        item->m = d.month();
        item->d = d.day();
        //凭证号
        int num = q.value(1).toInt(); //凭证号
        item->pzNum = QObject::tr("计%1").arg(num);
        //凭证摘要
        QString summary = q.value(2).toString();
        //如果是现金、银行科目
        //结算号
        //item->jsNum =
        //对方科目
        //item->oppoSub =
        int idx = summary.indexOf("<");
        if(idx != -1){
            summary = summary.left(idx);
        }
        item->summary = summary;
        //借、贷方金额
        item->mt = q.value(6).toInt();  //业务发生的币种
        item->dh = q.value(7).toInt();  //业务发生的借贷方向
        item->v = Double(q.value(5).toDouble());
//        if(item->dh == DIR_J)
//            item->v = Double(q.value(5).toDouble()); //发生在借方
//        else
//            item->v = Double(q.value(6).toDouble()); //发生在贷方

        //余额
        int key = item->y*1000+item->m*10+item->mt;
        Double mv;//发生额的美金原币值的本币值
        if(item->mt == USD)
            mv = item->v * rates.value(key);
        else
            mv = item->v;
        if(item->dh == DIR_J){
            tsums += mv;
            if(item->mt != masterMt){
                emsums[item->mt] += mv;
                esums[item->mt] += item->v;
            }
            else if(isJzhdPz && item->mt == masterMt && fid != cwfyId)
                emsums[item->mt] += item->v;
            else
                esums[item->mt] += item->v;
        }
        else{
            tsums -= mv;
            if(item->mt != masterMt){
                emsums[item->mt] -= mv;
                esums[item->mt] -= item->v;
            }
            else if(isJzhdPz && item->mt == masterMt && fid != cwfyId){
                emsums[item->mt] -= item->v;
            }
            else
                esums[item->mt] -= item->v;
        }

        //保存分币种的余额及其方向
        //item->em = esums;  //分币种余额
        it = new QHashIterator<int,Double>(esums);
        while(it->hasNext()){
            it->next();
            if(it->value()>0){
                item->em[it->key()] = it->value();
                item->dirs[it->key()] = DIR_J;
                if(it->key() != masterMt)
                    item->mm[it->key()] = emsums.value(it->key());
            }
            else if(it->value() < 0.00){
                item->em[it->key()] = it->value();
                item->em[it->key()].changeSign();
                item->dirs[it->key()] = DIR_D;
                if(it->key() != masterMt){
                    item->mm[it->key()] = emsums.value(it->key());
                    item->mm[it->key()].changeSign();
                }
            }
            else{
                item->em[it->key()] = 0;
                item->dirs[it->key()] = DIR_P;
                if(it->key() != masterMt)
                    item->mm[it->key()] = 0;
            }
        }

        //确定总余额的方向和值（余额值始终用正数表示，而在计算时，借方用正数，贷方用负数）
        if(tsums > 0){
            item->etm = tsums; //各币种合计余额
            item->dir = DIR_J;
        }
        else if(tsums < 0){
            item->etm = tsums;
            item->etm.changeSign();
            item->dir = DIR_D;
        }
        else{
            item->etm = 0;
            item->dir = DIR_P;
        }
        item->pid = q.value(4).toInt();
        item->bid = q.value(3).toInt(); //这个？？测试目的
        datas.append(item);
    }
    return true;
}



/**
 * @brief DbUtil::getFS_Id_name
 *  获取指定科目系统的一级科目的id和名称（一般用于一级科目选取组合框的初始化）
 * @param ids
 * @param names
 */
bool DbUtil::getFS_Id_name(QList<int> &ids, QList<QString> &names, int subSys)
{
    QSqlQuery q(db);
    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
    QString s = QString("select %1,%2 from %3").arg(fld_fsub_fid)
            .arg(fld_fsub_name).arg(tname);
    if(q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        ids<<q.value(0).toInt();
        names<<q.value(1).toString();
    }
    return true;
}

/**
 * @brief DbUtil::getPzsState
 *  获取指定年月的凭证集状态
 * @param y
 * @param m
 * @param state
 * @return
 */
bool DbUtil::getPzsState(int y, int m, PzsState &state)
{
    //if(!isNewExtraAccess())
    //    return BusiUtil::getPzsState(y,m,state);
    QSqlQuery q(db);
    if(y==0 && m==0){
        state = Ps_NoOpen;
        return true;
    }
    QString s = QString("select %1 from %2 where (%3=%4) and (%5=%6)")
            .arg(fld_pzss_state).arg(tbl_pzsStates).arg(fld_pzss_year)
            .arg(y).arg(fld_pzss_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first())  //还没有记录，则表示刚开始录入凭证
        state = Ps_Rec;
    else
        state = (PzsState)q.value(0).toInt();
    return true;
}

/**
 * @brief DbUtil::setPzsState
 *  保存指定年月的凭证集状态
 * @param y
 * @param m
 * @param state
 * @return
 */
bool DbUtil::setPzsState(int y, int m, PzsState state)
{
    //if(!isNewExtraAccess())
    //    return BusiUtil::setPzsState(y,m,state);

    QSqlQuery q(db);
    //首先检测是否存在对应记录
    QString s = QString("select %1 from %2 where (%3=%4) and (%5=%6)")
            .arg(fld_pzss_state).arg(tbl_pzsStates).arg(fld_pzss_year)
            .arg(y).arg(fld_pzss_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first())
        s = QString("update %1 set %2=%3 where (%4=%5) and (%6=%7)")
                .arg(tbl_pzsStates).arg(fld_pzss_state).arg(state)
                .arg(fld_pzss_year).arg(y).arg(fld_pzss_month).arg(m);
    else
        s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)").arg(tbl_pzsStates)
                .arg(fld_pzss_year).arg(fld_pzss_month).arg(fld_pzss_state)
                .arg(y).arg(m).arg(state);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::setExtraState
 *  设置余额是否有效
 * @param y
 * @param m
 * @param isVolid
 * @return
 */
bool DbUtil::setExtraState(int y, int m, bool isVolid)
{
//    if(!isNewExtraAccess())
//        return BusiUtil::setExtraState(y,m,isVolid);

    //余额的有效性状态只记录在“余额指针表”中保存本币余额的那条记录里
    QSqlQuery q(db);
    QString s = QString("update %1 set %2=%3 where %4=%5 and %6=%7 and %8=%9")
            .arg(tbl_nse_point).arg(fld_nse_state).arg(isVolid?1:0).arg(fld_nse_year)
            .arg(y).arg(fld_nse_month).arg(m).arg(fld_nse_mt).arg(masterMt);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.numRowsAffected() == 1)
        return true;
    s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,%8,%9)")
            .arg(tbl_nse_point).arg(fld_nse_year).arg(fld_nse_month)
            .arg(fld_nse_mt).arg(fld_nse_state).arg(y).arg(m).arg(masterMt)
            .arg(isVolid?1:0);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::getExtraState
 *  获取余额是否有效
 * @param y
 * @param m
 * @return
 */
bool DbUtil::getExtraState(int y, int m)
{
    //if(!isNewExtraAccess())
    //    return BusiUtil::getExtraState(y,m);

    QSqlQuery q(db);
    QString s = QString("select %1 from %2 where %3=%4 and %5=%6 and %7=%8")
            .arg(fld_nse_state).arg(tbl_nse_point).arg(fld_nse_year)
            .arg(y).arg(fld_nse_month).arg(m).arg(fld_nse_mt).arg(masterMt);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first())
        return false;
    return q.value(0).toBool();
}

/**
 * @brief DbUtil::getRates
 *  获取指定年月凭证集所采用的汇率
 * @param y
 * @param m
 * @param rates     汇率表（键为币种代码）
 * @param mainMt    本币代码
 * @return
 */
bool DbUtil::getRates(int y, int m, QHash<int, Double> &rates)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_moneyType);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QString mainSign; //母币符号
    QHash<int,QString> msHash; //货币代码到货币符号的映射
    int mt; QString mtSign;bool isMast;
    while(q.next()){
        isMast = q.value(MT_MASTER).toBool();
        mt = q.value(MT_CODE).toInt();
        mtSign = q.value(MT_SIGN).toString();
        if(!isMast)
            msHash[mt] = mtSign;
        else
            mainSign = mtSign;
    }

    QList<int> mtcs = msHash.keys();
    if(mtcs.isEmpty())  //账户没有设置外币，则汇率也无意义
        return true;
    s = QString("select ");
    for(int i = 0; i<mtcs.count(); ++i){
        s.append(msHash.value(mtcs.at(i)));
        s.append(QString("2%1,").arg(mainSign));
    }
    s.chop(1);
    s.append(QString(" from %1 ").arg(tbl_rateTable));
    s.append(QString("where %1 = %2 and %3 = %4")
             .arg(fld_rt_year).arg(y).arg(fld_rt_month).arg(m));
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        LOG_DEBUG(QObject::tr("没有指定汇率！"));
        return true;
    }
    for(int i = 0;i<mtcs.count();++i)
        rates[mtcs.at(i)] = Double(q.value(i).toDouble());
    return true;
}

/**
 * @brief DbUtil::saveRates
 *  保存指定年月凭证集所采用的汇率
 * @param y
 * @param m
 * @param rates     汇率表（键为币种代码）
 * @param mainMt    本币代码
 * @return
 */
bool DbUtil::saveRates(int y, int m, QHash<int, Double> &rates)
{
    QSqlQuery q(db);
    QString s,vs;

    QList<int> wbCodes;     //外币币种代码
    QList<QString> wbSigns; //外币符号列表
    QList<QString> mtFields; //存放与外币币种对应汇率的字段名（按序号一一对应）
    s = QString("select %1,%2,%3 from %4").arg(fld_mt_code).arg(fld_mt_sign)
            .arg(fld_mt_isMaster).arg(tbl_moneyType);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QString mainSign,mtSign;
    int mt; bool isMast;
    while(q.next()){
        mt = q.value(0).toInt();
        mtSign = q.value(1).toString();
        isMast = q.value(2).toBool();
        if(isMast)
            mainSign = mtSign;
        else{
            wbCodes << mt;
            wbSigns << mtSign;
        }
    }
    for(int i = 0; i < wbCodes.count(); ++i)
        mtFields << wbSigns.at(i) + "2" + mainSign;
    s = QString("select id from %1 where %2 = %3 and %4 = %5")
            .arg(tbl_rateTable).arg(fld_rt_year).arg(y).arg(fld_rt_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first()){
        int id = q.value(0).toInt();
        s = QString("update %1 set ").arg(tbl_rateTable);
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(wbCodes.at(i)))
                s.append(QString("%1=%2,").arg(mtFields.at(i))
                         .arg(rates.value(wbCodes.at(i)).toString2()));
        }

        s.chop(1);
        s.append(QString(" where id = %1").arg(id));
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    else{
        s = QString("insert into %1(%2,%3,").arg(tbl_rateTable).arg(fld_rt_year).arg(fld_rt_month);
        vs = QString("values(%1,%2,").arg(y).arg(m);
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(wbCodes.at(i))){
                s.append(mtFields.at(i)).append(",");
                vs.append(rates.value(wbCodes[i]).toString2()).append(",");
            }
        }
        s.chop(1); vs.chop(1);
        s.append(") "); vs.append(")");
        s.append(vs);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    return true;
}

/**
 * @brief DbUtil::loadPzSet
 *  装载指定年月的凭证
 * @param y
 * @param m
 * @param pzs
 * @return
 */
bool DbUtil::loadPzSet(int y, int m, QList<PingZheng *> &pzs, AccountSuiteManager* parent)
{
    QSqlQuery q(db),q1(db);

    //发现在事务中装载凭证集，也没有提升多少速度，不知道瓶颈在那里
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("When load PingZheng set(%1-%2) failed!"));
        return false;
    }

    QHash<int,Double> rates;
    if(!getRates(y,m,rates)){
        LOG_WARNING(QString("Don't read rates(y=%1,m=%2)").arg(y).arg(m));
    }

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select * from %1 where %2 like '%3%' order by %4")
            .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_number);

    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }

    pzs.clear();
    Account* account = parent->getAccount();
    int subSys = account->getSuiteRecord(y)->subSys;
    SubjectManager* smg = account->getSubjectManager(subSys);
    QHash<int,Money*> mts = account->getAllMoneys();
    PingZheng* pz;
    BusiAction* ba;
    int id,pid,pnum,znum,encnum,num;
    MoneyDirection dir;
    QString d,summary;
    PzClass pzCls;
    PzState pzState;
    User *vu,*ru,*bu;
    Double jsum,dsum;
    FirstSubject* fsub;
    SecondSubject* ssub;
    Double v;
    Money* mt;
    while(q.next()){
        id = q.value(0).toInt();
        d = q.value(PZ_DATE).toString();
        pnum = q.value(PZ_NUMBER).toInt();
        znum = q.value(PZ_ZBNUM).toInt();
        jsum = q.value(PZ_JSUM).toDouble();
        dsum = q.value(PZ_DSUM).toDouble();
        pzCls = (PzClass)q.value(PZ_CLS).toInt();
        encnum = q.value(PZ_ENCNUM).toInt();
        pzState = (PzState)q.value(PZ_PZSTATE).toInt();
        vu = allUsers.value(q.value(PZ_VUSER).toInt());
        ru = allUsers.value(q.value(PZ_RUSER).toInt());
        bu = allUsers.value(q.value(PZ_BUSER).toInt());

        pz = new PingZheng(parent,id,d,pnum,znum,jsum,dsum,pzCls,encnum,pzState,
                           vu,ru,bu);
        //如果是结转汇兑损益类凭证，则要根据其包含的会计分录中的对方科目来确定其是结转哪个科目的
        bool isJzhdPz = pzClsJzhds.contains(pzCls);
        QString as = QString("select * from %1 where %2=%3 order by %4")
                .arg(tbl_ba).arg(fld_ba_pid).arg(pz->id()).arg(fld_ba_number);
        if(!q1.exec(as)){
            LOG_SQLERROR(s);
            return false;
        }
        Double js = 0.0,ds = 0.0;
        while(q1.next()){
            id = q1.value(0).toInt();
            pid = q1.value(BACTION_PID).toInt();
            summary = q1.value(BACTION_SUMMARY).toString();
            fsub = smg->getFstSubject(q1.value(BACTION_FID).toInt());
            ssub = smg->getSndSubject(q1.value(BACTION_SID).toInt());
            mt = mts.value(q1.value(BACTION_MTYPE).toInt());
            dir = (MoneyDirection)q1.value(BACTION_DIR).toInt();
            v = Double(q1.value(BACTION_VALUE).toDouble());
            num = q1.value(BACTION_NUMINPZ).toInt();
            //ba->state = BusiActionData::INIT;
            ba = new BusiAction(id,pz,summary,fsub,ssub,mt,dir,v,num);
            pz->baLst<<ba;
            pz->watchBusiaction(ba);
            if(isJzhdPz && fsub->isUseForeignMoney() && dir == MDIR_D){
                pz->setOppoSubject(fsub);
                isJzhdPz=false; //后续的判断都是多余的
            }
            if(mt && v != 0.0){
                if(dir == MDIR_J)
                    js += v*rates.value(mt->code(),1.0);
                else
                    ds += v*rates.value(mt->code(),1.0);
            }

        }
        pzs<<pz;
        //纠正凭证表中的借贷合计值
        if(js != pz->jsum())
            pz->js = js;
        if(ds != pz->dsum())
            pz->ds = ds;
        js=0.0;ds=0.0;
    }
    //装载凭证备注信息
    foreach(PingZheng* pz, pzs){
        s = QString("select %1 from %2 where %3=%4").arg(fld_pzmi_info)
                .arg(tbl_pz_meminfos).arg(fld_pzmi_pid).arg(pz->id());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(!q.first())
            continue;
        QString info = q.value(0).toString();
        pz->setMemInfo(info);
    }

    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("When load PingZheng set(%1-%2) failed!").arg(y).arg(m));
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::isContainPz
 *  在指定年月的凭证集内是否包含了指定id的凭证
 * @param y
 * @param m
 * @param pid   凭证id
 * @return      true：包含，false：不包含
 */
bool DbUtil::isContainPz(int y, int m, int pid)
{
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select id from %1 where id=%2 and %3 like '%4%'")
            .arg(tbl_pz).arg(pid).arg(fld_pz_date).arg(ds);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return q.first();
}

/**
 * @brief DbUtil::inspectJzPzExist
 *  检查指定类别的凭证是否存在、是否齐全
 * @param y
 * @param m
 * @param pzCls 凭证大类别
 * @param count 指定的凭证大类必须具有的凭证数（返回值大于0，则表示凭证不齐全）
 * @return
 */
bool DbUtil::inspectJzPzExist(int y, int m, PzdClass pzCls, int &count)
{
    QSqlQuery q(db);
    QString s;
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    //1、结转汇兑损益凭证，要根据凭证集的需要使用外币的科目的余额状况来定
    if(pzCls == Pzd_Jzhd){
        //1、获取所指年份使用的科目系统代码
        s = QString("select %1 from %2 where %3=%4")
                .arg(fld_accs_subSys).arg(tbl_accSuites)
                .arg(fld_accs_year).arg(y);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(!q.first()){
            LOG_ERROR(QObject::tr("Don't read subject system code for %1").arg(y));
            return false;
        }
        int subSys = q.value(0).toInt();
        //2、获取需要使用外币的科目
        QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(subSys);
        QList<int> subIds;
        s = QString("select %1 from %2 where %3=1").arg(fld_fsub_fid)
                .arg(tname).arg(fld_fsub_isUseWb);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next())
            subIds<<q.value(0).toInt();
        //3、检查这些科目的余额，如果不为0，则必须结转，即要对这个科目进行计数
        //select SE_PM_F.sid, SE_PM_F.dir from SE_PM_F join SE_Point on  SE_PM_F.pid=SE_Point.id where SE_Point.year=2012 and SE_Point.month=12 and SE_Point.mt!=1
        //4、然后，查询结转汇兑损益凭证数目
    }
    else if(pzCls == Pzd_Jzsy){ //2、结转损益凭证，固定为2
        count = 2;
        s = QString("select count() from %1 where (%2 like '%3%') and (%4=%5 or %4=%6")
                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class)
                .arg(Pzc_JzsyIn).arg(Pzc_JzsyFei);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.first();
        count -= q.value(0).toInt();
    }
    else{ //3、结转利润凭证，固定为1
        count = 1;
        s = QString("select count() from %1 where (%2 like '%3%') and (%4=%5")
                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class)
                .arg(Pzc_Jzlr);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.first();
        count -= q.value(0).toInt();
    }
    return true;
}

/**
 * @brief DbUtil::clearPzSet
 *  清除数据库中指定年月凭证集内的所有凭证
 *  （凭证表内的凭证记录及在会计分录表内与这些凭证相关的所有会计分录，如果将来有其他表格的扩展，也要加以考虑）
 * @param y
 * @param m
 * @return
 */
bool DbUtil::clearPzSet(int y, int m)
{
    QSqlQuery q(db),q2(db);
    QString s;
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("when clear PingZheng set of year(%1) month(%2)"));
        return false;
    }
    s = QString("select id from %1 where %2 like '%3%'").arg(tbl_pz)
            .arg(fld_pz_date).arg(ds);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int pid;
    while(q.next()){
        pid = q.value(0).toInt();
        s = QString("delete from %1 where %2=%3").arg(tbl_ba).arg(fld_ba_pid).arg(pid);
        if(!q2.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    s = QString("delete from %1 where %2 like '%3%'").arg(tbl_pz)
            .arg(fld_pz_date).arg(ds);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("delete from %1 where %2=%3 and %4=%5")
            .arg(tbl_pzsStates).arg(fld_pzss_year).arg(y).arg(fld_pzss_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!db.commit()){
        if(!db.rollback())
            warn_transaction(Transaction_rollback,QObject::tr("when clear Ping Zheng in the year(%1) mont(%2)").arg(y).arg(m));
        warn_transaction(Transaction_commit,QObject::tr("when clear Ping Zheng in the year(%1) mont(%2)").arg(y).arg(m));
    }
    return true;
}

/**
 * @brief DbUtil::clearRates
 *  清除指定年月的汇率记录
 * @param y
 * @param m
 * @return
 */
bool DbUtil::clearRates(int y, int m)
{
    QSqlQuery q(db);
    QString s = QString("delete from %1 where %2=%3 and %4=%5")
            .arg(tbl_rateTable).arg(fld_rt_year).arg(y).arg(fld_rt_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::clearExtras
 *  清除指定年月的余额记录
 * @param y
 * @param m
 * @return
 */
bool DbUtil::clearExtras(int y, int m)
{
    QSqlQuery q(db),q2(db);
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("when clear extra values of year(%1) month(%2)"));
        return false;
    }
    QString s = QString("select id,%1 from %2 where %3=%4 and %5=%6").arg(fld_nse_mt)
            .arg(tbl_nse_point).arg(fld_nse_year).arg(y).arg(fld_nse_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int pid,mt;
    while(q.next()){
        pid = q.value(0).toInt();
        mt = q.value(1).toInt();
        s = QString("delete from %1 where %2=%3").arg(tbl_nse_p_f)
                .arg(fld_nse_pid).arg(pid);
        if(!q2.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        s = QString("delete from %1 where %2=%3").arg(tbl_nse_p_s)
                .arg(fld_nse_pid).arg(pid);
        if(!q2.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(mt != masterMt){
            s = QString("delete from %1 where %2=%3").arg(tbl_nse_m_f)
                    .arg(fld_nse_pid).arg(pid);
            if(!q2.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            s = QString("delete from %1 where %2=%3").arg(tbl_nse_m_s)
                    .arg(fld_nse_pid).arg(pid);
            if(!q2.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }
    s = QString("delete from %1 where %2=%3 and %4=%5").arg(tbl_nse_point)
            .arg(fld_nse_year).arg(y).arg(fld_nse_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }

    if(!db.commit()){
        if(!db.rollback())
            warn_transaction(Transaction_rollback,QObject::tr("when clear extra values of year(%1) mont(%2)").arg(y).arg(m));
        warn_transaction(Transaction_commit,QObject::tr("when clear extra values of year(%1) mont(%2)").arg(y).arg(m));
    }
    return true;
}

/**
 * @brief DbUtil::getPz
 *  读取指定id的凭证内容并创建凭证对象
 * @param pid
 * @param pz
 * @return
 */
bool DbUtil::getPz(int pid, PingZheng *&pz, AccountSuiteManager* parent)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where id=%2").arg(tbl_pz).arg(pid);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        pz = NULL;
        return true;
    }
    Account* account = parent->getAccount();
    QHash<int,Money*> mts = account->getAllMoneys();
    BusiAction* ba;
    int num;
    MoneyDirection dir;
    QString summary;
    FirstSubject* fsub;
    SecondSubject* ssub;
    Double v;
    Money* mt;
    int id = q.value(0).toInt();
    QString d = q.value(PZ_DATE).toString();
    int pnum = q.value(PZ_NUMBER).toInt();
    int znum = q.value(PZ_ZBNUM).toInt();
    Double jsum = q.value(PZ_JSUM).toDouble();
    Double dsum = q.value(PZ_DSUM).toDouble();
    PzClass pzCls = (PzClass)q.value(PZ_CLS).toInt();
    int encnum = q.value(PZ_ENCNUM).toInt();
    PzState pzState = (PzState)q.value(PZ_PZSTATE).toInt();
    User* vu = allUsers.value(q.value(PZ_VUSER).toInt());
    User* ru = allUsers.value(q.value(PZ_RUSER).toInt());
    User* bu = allUsers.value(q.value(PZ_BUSER).toInt());

    pz = new PingZheng(parent,id,d,pnum,znum,jsum,dsum,pzCls,encnum,pzState,
                       vu,ru,bu);
    SubjectManager* smg = account->getSubjectManager(account->getSuiteRecord(pz->getDate2().year())->subSys);
    QHash<int,Double> rates;
    account->getRates(pz->getDate2().year(),pz->getDate2().month(),rates);
    s = QString("select * from %1 where %2=%3 order by %4")
            .arg(tbl_ba).arg(fld_ba_pid).arg(pz->id()).arg(fld_ba_number);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    Double js = 0.0,ds = 0.0;
    while(q.next()){
        id = q.value(0).toInt();
        summary = q.value(BACTION_SUMMARY).toString();
        fsub = smg->getFstSubject(q.value(BACTION_FID).toInt());
        ssub = smg->getSndSubject(q.value(BACTION_SID).toInt());
        mt = mts.value(q.value(BACTION_MTYPE).toInt());
        dir = (MoneyDirection)q.value(BACTION_DIR).toInt();
        v = Double(q.value(BACTION_VALUE).toDouble());
        num = q.value(BACTION_NUMINPZ).toInt();
        ba = new BusiAction(id,pz,summary,fsub,ssub,mt,dir,v,num);
        pz->baLst<<ba;
        if(dir == MDIR_J)
            js += v*rates.value(mt->code(),1.0);
        else
            ds += v*rates.value(mt->code(),1.0);
    }
    //纠正凭证表中的借贷合计值
    if(js != pz->jsum())
        pz->js = js;
    if(ds != pz->dsum())
        pz->ds = ds;
    js=0.0;ds=0.0;
    return true;
}

/**
 * @brief DbUtil::savePingZhengs
 *  保存一组凭证
 * @param   pzs
 * @param   dels
 * @param   catched
 * @return
 */
bool DbUtil::savePingZhengs(QList<PingZheng *> pzs)
{
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("when save a Ping Zheng start transaction failed!"));
        return false;
    }

    PingZheng* pz;
    for(int i = 0; i < pzs.count(); ++i){
        pz = pzs.at(i);
        if(!_savePingZheng(pz)){
            LOG_SQLERROR(QObject::tr("保存凭证时发生错误！"));
            errorNotify(QObject::tr("保存凭证时发生错误！"));
            //db.commit();
            return false;
        }
    }
    if(!db.commit()){
        if(!db.rollback()){
            warn_transaction(Transaction_rollback,QObject::tr("when save a Ping Zheng rollback transaction failed"));
            return false;
        }
        warn_transaction(Transaction_commit,QObject::tr("when save a Ping Zheng commit transaction failed"));
        return false;
    }
    foreach(pz,pzs)
        pz->resetEditState();
    return true;
}

/**
 * @brief DbUtil::savePingZheng
 *  保存一张凭证
 * @param pz
 * @return
 */
bool DbUtil::savePingZheng(PingZheng *pz)
{
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("when save a Ping Zheng failed!"));
        return false;
    }
    if(!_savePingZheng(pz)){
        LOG_ERROR(QObject::tr("保存凭证时发生错误！"));
        errorNotify(QObject::tr("保存凭证时发生错误！"));
        db.commit();
        return false;
    }
    if(!db.commit()){
        if(!db.rollback()){
            warn_transaction(Transaction_rollback,QObject::tr("when save a Ping Zheng rollback transaction failed"));
            return false;
        }
        warn_transaction(Transaction_commit,QObject::tr("when save a Ping Zheng commit transaction failed"));
        return false;
    }
    pz->resetEditState();
    return true;
}

/**
 * @brief DbUtil::delPingZhengs
 *  从数据库中删除一组凭证
 * @param pzs
 * @return
 */
bool DbUtil::delPingZhengs(QList<PingZheng *> pzs)
{
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("when delete a Ping Zheng failed!"));
        return false;
    }
    for(int i = 0; i < pzs.count(); ++i){
        if(!_delPingZheng(pzs.at(i))){
            errorNotify(QObject::tr("Delete ping zheng operate failed!"));
            return false;
        }
        //delete pzs.at(i);
    }
    if(!db.commit()){
        if(!db.rollback()){
            warn_transaction(Transaction_rollback,QObject::tr("when delete a Ping Zheng rollback transaction failed"));
            return false;
        }
        warn_transaction(Transaction_commit,QObject::tr("when delete a Ping Zheng commit transaction failed"));
        return false;
    }
    //pzs.clear();
    return true;
}

/**
 * @brief DbUtil::delPingZheng
 *  删除单一凭证
 * @param pz
 * @return
 */
bool DbUtil::delPingZheng(PingZheng *pz)
{

    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("when delete a Ping Zheng failed!"));
        return false;
    }
    if(!_delPingZheng(pz)){
        errorNotify(QObject::tr("Delete ping zheng operate failed!"));
        return false;
    }
    if(!db.commit()){
        if(!db.rollback()){
            warn_transaction(Transaction_rollback,QObject::tr("when delete a Ping Zheng rollback transaction failed"));
            return false;
        }
        warn_transaction(Transaction_commit,QObject::tr("when delete a Ping Zheng commit transaction failed"));
        return false;
    }
    delete pz; pz = NULL;
    return true;
}


/**
 * @brief DbUtil::assignPzNum
 *  按凭证日期和录入顺序重置凭证号
 * @param y
 * @param m
 * @return
 */
bool DbUtil::assignPzNum(int y, int m)
{
    QSqlQuery q(db),q1(db);
    QString s;

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("重置凭证"));
        return false;
    }
    s = QString("select id from %1 where %2 like '%3%' order by %2")
            .arg(tbl_pz).arg(fld_pz_date).arg(ds);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int id, num = 1;
    while(q.next()){
        id = q.value(0).toInt();
        s = QString("update %1 set %2=%3 where id=%4")
                .arg(tbl_pz).arg(fld_pz_number).arg(num++).arg(id);
        if(!q1.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("重置凭证"));
        if(!db.rollback()){
            warn_transaction(Transaction_rollback,QObject::tr("重置凭证"));
            return false;
        }
        return false;
    }
    return true;
}

bool DbUtil::crtNewPz(PzData *pz)
{
    QSqlQuery q(db);
    QString s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12) "
                        "values('%13',%14,%15,%16,%17,%18,%19,%20,%21,%22,%23)").arg(tbl_pz)
            .arg(fld_pz_date).arg(fld_pz_number).arg(fld_pz_zbnum).arg(fld_pz_jsum)
            .arg(fld_pz_dsum).arg(fld_pz_class).arg(fld_pz_encnum).arg(fld_pz_state)
            .arg(fld_pz_vu).arg(fld_pz_ru).arg(fld_pz_bu)
            .arg(pz->date).arg(pz->pzNum).arg(pz->pzZbNum).arg(pz->jsum)
            .arg(pz->dsum).arg(pz->pzClass).arg(pz->attNums).arg(pz->state)
            .arg(pz->verify!=NULL?pz->verify->getUserId():NULL)
            .arg(pz->producer!=NULL?pz->producer->getUserId():NULL)
            .arg(pz->bookKeeper!=NULL?pz->bookKeeper->getUserId():NULL);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }

    s = QString("select last_insert_rowid()");
    if(q.exec(s) && q.first())
        pz->pzId = q.value(0).toInt();
    else
        return false;
    return true;
}

bool DbUtil::delActionsInPz(int pzId)
{
    QSqlQuery q(db);
    QString s = QString("delete from %1 where %2 = %3").arg(tbl_ba).arg(fld_ba_pid).arg(pzId);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    //int rows = q.numRowsAffected();
    return true;
}

/**
 * @brief DbUtil::getActionsInPz
 *  获取指定凭证内的所有会计分录（将来当引入凭证和会计分录类时，要修改）
 * @param pid
 * @param busiActions
 * @return
 */
bool DbUtil::getActionsInPz(int pid, QList<BusiActionData2 *> &busiActions)
{
    QSqlQuery q(db);

    if(busiActions.count() > 0){
        qDeleteAll(busiActions);
        busiActions.clear();
    }

    QString s = QString("select * from %1 where %2 = %3 order by %4")
            .arg(tbl_ba).arg(fld_ba_pid).arg(pid).arg(fld_ba_number);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        BusiActionData2* ba = new BusiActionData2;
        ba->id = q.value(0).toInt();
        ba->pid = pid;
        ba->summary = q.value(BACTION_SUMMARY).toString();
        ba->fid = q.value(BACTION_FID).toInt();
        ba->sid = q.value(BACTION_SID).toInt();
        ba->mt  = q.value(BACTION_MTYPE).toInt();
        ba->dir = q.value(BACTION_DIR).toInt();
        ba->v = Double(q.value(BACTION_VALUE).toDouble());
//        if(ba->dir == DIR_J)
//            ba->v = Double(q.value(BACTION_VALUE).toDouble());
//        else
//            ba->v = Double(q.value(BACTION_DMONEY).toDouble());
        ba->num = q.value(BACTION_NUMINPZ).toInt();
        ba->state = BusiActionData2::INIT;
        busiActions.append(ba);
    }
    return true;
}

/**
 * @brief DbUtil::saveActionsInPz
 *  保存指定凭证的所有会计分录（当引入凭证类后，可以使用一个凭证对象参数在一个事务中保存所有凭证相关的内容）
 * @param pid
 * @param busiActions
 * @param dels
 * @return
 */
bool DbUtil::saveActionsInPz(int pid, QList<BusiActionData2 *> &busiActions, QList<BusiActionData2 *> dels)
{
    QString s;
    QSqlQuery q1(db),q2(db),q3(db),q4(db);
    bool hasNew = false;

    if(!busiActions.isEmpty()){
        if(!db.transaction()){
            warn_transaction(Transaction_open,QObject::tr("保存凭证内的会计分录"));
            return false;
        }

        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9) "
                    "values(:pid,:summary,:fid,:sid,:mt,:v,:dir,:num)")
                .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary).arg(fld_ba_fid)
                .arg(fld_ba_sid).arg(fld_ba_mt).arg(fld_ba_value)
                .arg(fld_ba_dir).arg(fld_ba_number);
        q1.prepare(s);
        s = QString("update %1 set %2=:summary,%3=:fid,%4=:sid,%5=:mt,"
                    "%6=:v,%7=:dir,%8=:num where id=:id")
                .arg(tbl_ba).arg(fld_ba_summary).arg(fld_ba_fid).arg(fld_ba_sid)
                .arg(fld_ba_mt).arg(fld_ba_value).arg(fld_ba_dir)
                .arg(fld_ba_number);
        q2.prepare(s);
        s = QString("update %1 set %2=:num where id=:id").arg(tbl_ba).arg(fld_ba_number);
        q3.prepare(s);
        for(int i = 0; i < busiActions.count(); ++i){
            busiActions[i]->num = i + 1;  //在保存的同时，重新赋于顺序号
            switch(busiActions[i]->state){
            case BusiActionData2::INIT:
                break;
            case BusiActionData2::NEW:
                hasNew = true;
                q1.bindValue(":pid",busiActions[i]->pid);
                q1.bindValue(":summary", busiActions[i]->summary);
                q1.bindValue(":fid", busiActions[i]->fid);
                q1.bindValue(":sid", busiActions[i]->sid);
                q1.bindValue(":mt", busiActions[i]->mt);
                //if(busiActions[i]->dir == DIR_J){
                    q1.bindValue(":v", busiActions[i]->v.getv());
                    q1.bindValue(":dv",0);
                    q1.bindValue(":dir", DIR_J);
                //}
//                else{
//                    q1.bindValue(":jv",0);
//                    q1.bindValue(":dv", busiActions[i]->v.getv());
//                    q1.bindValue(":dir", DIR_D);
//                }
                q1.bindValue(":num", busiActions[i]->num);
                q1.exec();
                q4.exec("select last_insert_rowid()");
                q4.first();
                busiActions.at(i)->id = q4.value(0).toInt();
                break;
            case BusiActionData2::EDITED:
                q2.bindValue(":summary", busiActions[i]->summary);
                q2.bindValue(":fid", busiActions[i]->fid);
                q2.bindValue(":sid", busiActions[i]->sid);
                q2.bindValue(":mt", busiActions[i]->mt);
                //if(busiActions[i]->dir == DIR_J){
                    q2.bindValue(":v", busiActions[i]->v.getv());
                    q2.bindValue(":dv",0);
                    q2.bindValue(":dir", DIR_J);
                //}
//                else{
//                    q2.bindValue(":jv",0);
//                    q2.bindValue(":dv", busiActions[i]->v.getv());
//                    q2.bindValue(":dir", DIR_D);
//                }
                q2.bindValue(":num", busiActions[i]->num);
                q2.bindValue("id", busiActions[i]->id);
                q2.exec();
                break;
            case BusiActionData2::NUMCHANGED:
                q3.bindValue(":num", busiActions[i]->num);
                q3.bindValue(":id", busiActions[i]->id);
                q3.exec();
                break;
            }
            busiActions[i]->state = BusiActionData2::INIT;
        }
        if(!db.commit()){
            warn_transaction(Transaction_commit,QObject::tr("保存凭证内的会计分录"));
            return false;
        }
        //回读新增的业务活动的id(待需要时再使用此代码)
        //if(hasNew && !getActionsInPz(pid,busiActions))
        //    return false;
    }
    if(!dels.isEmpty()){
        if(!db.transaction()){
            warn_transaction(Transaction_open,QObject::tr("保存凭证内的会计分录"));
            return false;
        }
        s = "delete from BusiActions where id=:id";
        q4.prepare(s);
        for(int i = 0; i < dels.count(); ++i){
            q4.bindValue(":id", dels[i]->id);
            q4.exec();
        }
        if(!db.commit()){
            warn_transaction(Transaction_commit,QObject::tr("保存凭证内的会计分录"));
            return false;
        }
    }
    return true;
}
/**
 * @brief DbUtil:delSpecPz
 *  删除指定年月的指定大类别凭证
 * @param y
 * @param m
 * @param pzCls         凭证大类
 * @param pzCntAffected 删除的凭证数
 * @return
 */
bool DbUtil::delSpecPz(int y, int m, PzdClass pzCls, int &affected)
{
    QString s;
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QList<PzClass> codes = getSpecClsPzCode(pzCls);
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("删除大类别凭证"));
        return false;
    }
    QList<int> ids;
    for(int i = 0; i < codes.count(); ++i){
        s = QString("select id from %1 where (%2 like '%3%') and (%4=%5)")
                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class).arg(codes.at(i));
        q.exec(s);q.first();
        ids<<q.value(0).toInt();
    }
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("删除大类别凭证"));
        return false;
    }
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("删除大类别凭证"));
        return false;
    }
    affected = 0;
    for(int i = 0; i < ids.count(); ++i){
        s = QString("delete from %1 where %2=%3").arg(tbl_ba).arg(fld_ba_pid).arg(ids.at(i));
        q.exec(s);
        s = QString("delete from %1 where id=%2").arg(tbl_pz).arg(ids.at(i));
        q.exec(s);
        affected += q.numRowsAffected();
    }
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("删除大类别凭证"));
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::getSpecClsPzCode
 *  获取指定大类凭证的类别代码列表
 * @param cls
 * @return
 */
QList<PzClass> DbUtil::getSpecClsPzCode(PzdClass cls)
{
    QList<PzClass> codes;
    if(cls == Pzd_Jzhd){
        codes=pzClsJzhds.toList();
        codes.removeOne(Pzc_Jzhd);
    }
    else if(cls == Pzd_Jzsy)
        codes<<Pzc_JzsyIn<<Pzc_JzsyFei;
    else if(cls == Pzd_Jzlr)
        codes<<Pzc_Jzlr;
    return codes;
}

/**
 * @brief DbUtil::haveSpecClsPz
 *  检测凭证集内各类特殊凭证是否存在
 * @param y
 * @param m
 * @param isExist
 * @return
 */
bool DbUtil::haveSpecClsPz(int y, int m, QHash<PzdClass, bool> &isExist)
{
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);

    //检测结转汇兑损益类凭证是否存在
    QString s = QString("select id from %1 where (%2 like '%3%') and (")
            .arg(tbl_pz).arg(fld_pz_date).arg(ds);
    foreach(PzClass pzCls, getSpecClsPzCode(Pzd_Jzhd))
        s.append(QString("%1=%2 or ").arg(fld_pz_class).arg(pzCls));
    s.chop(4); s.append(")");
    if(!q.exec(s))
        return false;
    isExist[Pzd_Jzhd] = q.first();

    //检测结转损益类凭证是否存在
    s = QString("select id from %1 where (%2 like '%3%') and (%4=%5 or %4=%6)")
                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class)
                .arg(Pzc_JzsyIn).arg(Pzc_JzsyFei);
    if(!q.exec(s))
        return false;
    isExist[Pzd_Jzsy] = q.first();
    //检测结转利润类凭证是否存在
    s = QString("select id from %1 where (%2 like '%3%') and (%4=%5)")
                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class)
                .arg(Pzc_Jzlr);
    if(!q.exec(s))
        return false;
    isExist[Pzd_Jzlr] = q.first();
    return true;
}

/**
 * @brief DbUtil::specPzClsInstat
 *  将指定类别凭证全部入账
 * @param y
 * @param m
 * @param cls
 * @param affected  受影响的凭证条目
 * @return
 */
bool DbUtil::specPzClsInstat(int y, int m, PzdClass cls, int &affected)
{
    QSqlQuery q(db);
    QString s;
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QList<PzClass> codes = getSpecClsPzCode(cls);
    if(!db.transaction()){
        warn_transaction(Transaction_open,QObject::tr("将指定类别凭证全部入账"));
        return false;
    }
    affected = 0;
    for(int i = 0; i < codes.count(); ++i){
        s = QString("update %1 set %2=%3 where (%4 like '%5%') and %6=%7")
                .arg(tbl_pz).arg(fld_pz_state).arg(Pzs_Instat)
                .arg(fld_pz_date).arg(ds).arg(fld_pz_class).arg(codes.at(i));
        q.exec(s);
        affected += q.numRowsAffected();
    }
    if(!db.commit()){
        warn_transaction(Transaction_commit,QObject::tr("将指定类别凭证全部入账"));
        return false;
    }
}

/**
 * @brief DbUtil::setAllPzState
 *  设置凭证集内的所有具有指定状态的凭证到目的状态
 * @param y
 * @param m
 * @param state         目的状态
 * @param includeState  指定的凭证状态
 * @param affected      受影响的行数
 * @param user
 * @return
 */
bool DbUtil::setAllPzState(int y, int m, PzState state, PzState includeState, int &affected, User *user)
{
    QSqlQuery q(db);
    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    QString userField;
    if(state == Pzs_Recording)
        userField = fld_pz_ru;
    else if(state == Pzs_Verify)
        userField = fld_pz_vu;
    else if(state == Pzs_Instat)
        userField = fld_pz_bu;
    QString s = QString("update %1 set %2=%3,%4=%5 where (%6 like '%7%') and %2=%8")
            .arg(tbl_pz).arg(fld_pz_state).arg(state).arg(userField)
            .arg(user->getUserId()).arg(fld_pz_date).arg(ds).arg(includeState);
    affected = 0;
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    affected = q.numRowsAffected();
    return true;
}

/**
 * @brief DbUtil::getSubWinInfo
 *  读取子窗口的各种几何尺寸信息
 * @param winEnum
 * @param info
 * @param otherInfo
 * @return
 */
bool DbUtil::getSubWinInfo(int winEnum, SubWindowDim *&info, QByteArray *&otherInfo)
{
    QSqlQuery q(db);
    QString s = QString("select * from %1 where %2 = %3")
            .arg(tbl_subWinInfo).arg(fld_swi_enum).arg(winEnum);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first()){
        info = new SubWindowDim;
        info->x = q.value(SWI_X).toInt();
        info->y = q.value(SWI_Y).toInt();
        info->w = q.value(SWI_W).toInt();
        info->h = q.value(SWI_H).toInt();
        otherInfo = new QByteArray(q.value(SWI_TBL).toByteArray());
    }
    else{
        info = NULL;
        otherInfo = NULL;
    }
    return true;
}

/**
 * @brief DbUtil::saveSubWinInfo
 *  保存子窗口的各种几何尺寸信息
 * @param winEnum
 * @param info
 * @param otherInfo
 * @return
 */
bool DbUtil::saveSubWinInfo(int winEnum, SubWindowDim *info, QByteArray *otherInfo)
{
    QSqlQuery q(db);
    QString s;

    if(otherInfo == NULL)
        otherInfo = new QByteArray;
    s = QString("select * from %1 where %2 = %3")
            .arg(tbl_subWinInfo).arg(fld_swi_enum).arg(winEnum);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(q.first()){
        int id = q.value(0).toInt();
        s = QString("update %1 set %2=:enum,%3=:x,%4=:y,%5=:w,%6=:h"
                    ",%7=:info where id=:id")
                .arg(tbl_subWinInfo).arg(fld_swi_enum).arg(fld_swi_x).arg(fld_swi_y)
                .arg(fld_swi_width).arg(fld_swi_height).arg(fld_swi_tblInfo);
        if(!q.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.bindValue(":enum",winEnum);
        q.bindValue(":x",info->x);
        q.bindValue(":y",info->y);
        q.bindValue(":w",info->w);
        q.bindValue(":h",info->h);
        q.bindValue(":info",*otherInfo);
        q.bindValue(":id",id);
    }
    else{
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7) "
                    "values(:enum,:x,:y,:w,:h,:info)")
                .arg(tbl_subWinInfo).arg(fld_swi_enum).arg(fld_swi_x).arg(fld_swi_y)
                .arg(fld_swi_width).arg(fld_swi_height).arg(fld_swi_tblInfo);;
        if(!q.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.bindValue(":enum",winEnum);
        q.bindValue(":x",info->x);
        q.bindValue(":y",info->y);
        q.bindValue(":w",info->w);
        q.bindValue(":h",info->h);
        q.bindValue(":info",*otherInfo);
    }
    if(!q.exec()){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::saveAccInfoPiece
 *  保存账户信息片段
 * @param code
 * @param value
 * @return
 */
bool DbUtil::saveAccInfoPiece(DbUtil::InfoField code, QString value)
{
    QSqlQuery q(db);
    QString s = QString("update %1 set %2='%3' where %4=%5")
            .arg(tbl_accInfo).arg(fld_acci_value).arg(value)
            .arg(fld_acci_code).arg(code);
    if(!q.exec(s))
        return false;
    if(q.numRowsAffected() == 0){
        s = QString("insert into %1(%2,%3,%4) values(%5,'%6','%7')")
                .arg(tbl_accInfo).arg(fld_acci_code).arg(fld_acci_name)
                .arg(fld_acci_value).arg(code).arg(pNames.value(code)).arg(value);
        if(!q.exec(s))
            return false;
    }
    return true;
}

/**
 * @brief DbUtil::readAccountSuites
 *  读取帐套信息
 * @param suites
 * @return
 */
bool DbUtil::_readAccountSuites(QList<AccountSuiteRecord *> &suites)
{
    //读取账户的帐套信息
    QSqlQuery q(db);
    QString s = QString("select * from %1 order by %2").arg(tbl_accSuites).arg(fld_accs_year);
    if(!q.exec(s))
        return false;
    AccountSuiteRecord* as;
    while(q.next()){
        as = new AccountSuiteRecord;
        as->isUsed = false;
        as->id = q.value(0).toInt();
        as->year = q.value(ACCS_YEAR).toInt();
        as->subSys = q.value(ACCS_SUBSYS).toInt();
        as->recentMonth = q.value(ACCS_RECENTMONTH).toInt();
        as->isCur = q.value(ACCS_ISCUR).toInt();
        as->isClosed = q.value(ACCS_ISCLOSED).toBool();
        as->name = q.value(ACCS_NAME).toString();
        as->startMonth = q.value(ACCS_STARTMONTH).toInt();
        as->endMonth = q.value(ACCS_ENDMONTH).toInt();
        suites<<as;
    }
    //以是否存在该帐套对应的凭证集状态记录来判断该帐套是否已启用
    foreach(AccountSuiteRecord* as, suites){
        s = QString("select id from %1 where %2 like '%3%'")
                .arg(tbl_pz).arg(fld_pz_date).arg(as->year);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        as->isUsed = q.first();
    }

    return true;
}

/**
 * @brief DbUtil::saveAccountSuites
 *  保存帐套信息
 * @param suites
 * @return
 */
bool DbUtil::_saveAccountSuite(AccountSuiteRecord *suite)
{
    QString s;
    QSqlQuery q(db);
    if(suite->id == 0)
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8) values(%9,%10,%11,%12,'%13',%14,%15)")
                .arg(tbl_accSuites).arg(fld_accs_year).arg(fld_accs_subSys).arg(fld_accs_isCur)
                .arg(fld_accs_recentMonth).arg(fld_accs_name).arg(fld_accs_startMonth)
                .arg(fld_accs_endMonth).arg(suite->year).arg(suite->subSys).arg(suite->isClosed?1:0)
                .arg(suite->recentMonth).arg(suite->name).arg(suite->startMonth).arg(suite->endMonth);
    else
        s = QString("update %1 set %2=%3,%4=%5,%6=%7,%8=%9,%10='%11',%12=%13,%14=%15,%16=%17 where id=%18")
                .arg(tbl_accSuites).arg(fld_accs_year).arg(suite->year)
                .arg(fld_accs_subSys).arg(suite->subSys).arg(fld_accs_isCur).arg(suite->isCur?1:0)
                .arg(fld_accs_recentMonth).arg(suite->recentMonth).arg(fld_accs_name).arg(suite->name)
                .arg(fld_accs_startMonth).arg(suite->startMonth).arg(fld_accs_endMonth).arg(suite->endMonth)
                .arg(fld_accs_isClosed).arg(suite->isClosed?1:0).arg(suite->id);

    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(suite->id == 0){
        s = "select last_insert_rowid()";
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.next();
        suite->id = q.value(0).toInt();
    }
    return true;
}

/**
 * @brief DbUtil::_saveFirstSubject
 * @param sub
 * @return
 */
bool DbUtil::_saveFirstSubject(FirstSubject *sub)
{
    QSqlQuery q(db);
    QString s;
    QString tname = QString("%1%2").arg(tbl_fsub_prefix).arg(sub->parent()->getSubSysCode());
    if(sub->getId() == UNID){
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9) values('%12','%13',%14,%15,%16,%17,%18,'%19')")
                .arg(tname).arg(fld_fsub_subcode).arg(fld_fsub_remcode)
                .arg(fld_fsub_class).arg(fld_fsub_jddir).arg(fld_fsub_isEnalbed).arg(fld_fsub_isUseWb)
                .arg(fld_fsub_weight).arg(fld_fsub_name).arg(sub->getCode()).
                arg(sub->getRemCode()).arg(sub->getSubClass())
                .arg(sub->getJdDir()?1:0).arg(sub->isEnabled()?1:0)
                .arg(sub->isUseForeignMoney()?1:0)
                .arg(sub->getWeight()).arg(sub->getName());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        //保存子目
        foreach(SecondSubject* ssub, sub->getChildSubs()){
            if(!_saveSecondSubject(ssub))
                return false;
        }
    }
    else{
        FirstSubjectEditStates estate = sub->getEditState();
        if(estate == ES_FS_INIT)
            return true;
        s = QString("update %1 set ").arg(tname);
        if(estate.testFlag(ES_FS_CLASS))
            s.append(QString("%1=%2,").arg(fld_fsub_class).arg(sub->getSubClass()));
        if(estate.testFlag(ES_FS_CODE))
            s.append(QString("%1='%2,'").arg(fld_fsub_subcode).arg(sub->getCode()));
        if(estate.testFlag(ES_FS_JDDIR))
            s.append(QString("%1=%2,").arg(fld_fsub_jddir).arg(sub->getJdDir()?1:0));
        if(estate.testFlag(ES_FS_ISENABLED))
            s.append(QString("%1=%2,").arg(fld_fsub_isEnalbed).arg(sub->isEnabled()?1:0));
        if(estate.testFlag(ES_FS_ISUSEWB))
            s.append(QString("%1=%2,").arg(fld_fsub_isUseWb).arg(sub->isUseForeignMoney()?1:0));
        if(estate.testFlag(ES_FS_WEIGHT))
            s.append(QString("%1=%2,").arg(fld_fsub_weight).arg(sub->getWeight()));
        if(estate.testFlag(ES_FS_NAME))
            s.append(QString("%1='%2',").arg(fld_fsub_name).arg(sub->getName()));
        if(s.endsWith(",")){
            s.chop(1);
            s.append(QString(" where id=%1").arg(sub->getId()));
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        //保存子目
        if(estate.testFlag(ES_FS_CHILD)){
            if(!saveSndSubjects(sub->getChildSubs()))
                return false;
        }
        //清除默认科目标记-即将在此一级科目下的已不是默认科目的二级科目的权重复位到初始权重
        //（最新的默认科目的保存在保存二级科目的权重时就完成了）
        if(estate.testFlag(ES_FS_DEFSUB)){
            s = QString("update %1 set %2=%3 where %4=%5 and id!=%6 and %2=%7").arg(tbl_ssub)
                    .arg(fld_ssub_weight).arg(INIT_WEIGHT).arg(fld_ssub_fid)
                    .arg(sub->getId()).arg(sub->getDefaultSubject()->getId())
                    .arg(DEFALUT_SUB_WEIGHT);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }
    sub->resetEditState();
    return true;
}

/**
 * @brief DbUtil::_saveSecondSubject
 * @param sub
 * @return
 */
bool DbUtil::_saveSecondSubject(SecondSubject *sub)
{
    QSqlQuery q(db);
    QString s;


    if(!_saveNameItem(sub->getNameItem()))
        return false;

    if(sub->getId() == UNID)
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8) "
                    "values(%9,%10,'%11',%12,%13,'%14',%15)").arg(tbl_ssub)
                .arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_code)
                .arg(fld_ssub_weight).arg(fld_ssub_enable).arg(fld_ssub_crtTime)
                .arg(fld_ssub_creator).arg(sub->getParent()->getId())
                .arg(sub->getNameItem()->getId()).arg(sub->getCode())
                .arg(sub->getWeight()).arg(sub->isEnabled()?1:0)
                .arg(sub->getCreateTime().toString(Qt::ISODate)).arg(sub->getCreator()->getUserId());
    else{
        SecondSubjectEditStates state = sub->getEditState();
        if(state == ES_SS_INIT)
            return true;
        s = QString("update %1 set ").arg(tbl_ssub);
        if(state.testFlag(ES_SS_CODE))
            s.append(QString("%1='%2',").arg(fld_ssub_code).arg(sub->getCode()));
        if(state.testFlag(ES_SS_FID))
            s.append(QString("%1=%2,").arg(fld_ssub_fid).arg(sub->getParent()->getId()));
        if(state.testFlag(ES_SS_NID))
            s.append(QString("%1=%2,").arg(fld_ssub_nid).arg(sub->getNameItem()->getId()));
        if(state.testFlag(ES_SS_WEIGHT))
            s.append(QString("%1=%2,").arg(fld_ssub_weight).arg(sub->getWeight()));
        if(state.testFlag(ES_SS_DISABLE))
            s.append(QString("%1='%2',").arg(fld_ssub_disTime).arg(sub->getDisableTime().toString(Qt::ISODate)));
        if(state.testFlag(ES_SS_ISENABLED))
            s.append(QString("%1=%2,").arg(fld_ssub_enable).arg(sub->isEnabled()?1:0));
        if(state.testFlag(ES_SS_CTIME))
            s.append(QString("%1=%2,").arg(fld_ssub_crtTime).arg(sub->getCreateTime().toString(Qt::ISODate)));
        if(state.testFlag(ES_SS_CUSER))
            s.append(QString("%1=%2,").arg(fld_ssub_creator).arg(sub->getCreator()?sub->getCreator()->getUserId():0));
        if(s.endsWith(","))
            s.chop(1);
        s.append(QString(" where id=%1").arg(sub->getId()));
    }
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(sub->getId() == 0){
        if(!q.exec("select last_insert_rowid()") || !q.first())
            return false;
        sub->id = q.value(0).toInt();
        SubjectManager* sm = sub->getParent()->parent();
        sm->sndSubs[sub->id] = sub;
    }
    //保存是否是默认科目的属性

    sub->resetEditState();
    return true;
}

bool DbUtil::_removeSecondSubject(SecondSubject *sub)
{
    QSqlQuery q(db);
    QString s = QString("delete from %1 where id=%2").arg(tbl_ssub).arg(sub->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    SubjectManager* sm = sub->getParent()->parent();
    sm->sndSubs.remove(sub->id);
    return true;
}

/**
 * @brief DbUtil::_saveNameItem
 * @param ni
 * @return
 */
bool DbUtil::_saveNameItem(SubjectNameItem *ni)
{
    QSqlQuery q(db);
    QString s;
    if(ni->getId() == UNID)
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values('%8','%9','%10',%11,'%12',%13)")
                .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_lname)
                .arg(fld_ni_remcode).arg(fld_ni_class).arg(fld_ni_crtTime)
                .arg(fld_ni_creator).arg(ni->getShortName()).arg(ni->getLongName())
                .arg(ni->getRemCode()).arg(ni->getClassId()).arg(ni->getCreateTime().toString(Qt::ISODate))
                .arg(ni->getCreator()?ni->getCreator()->getUserId():1);
    else{
        NameItemEditStates state = ni->getEditState();
        if(state == ES_NI_INIT)
            return true;
        s = QString("update %1 set ").arg(tbl_nameItem);
        if(state.testFlag(ES_NI_CLASS))
            s.append(QString("%1=%2,").arg(fld_ni_class).arg(ni->getClassId()));
        if(state.testFlag(ES_NI_SNAME))
            s.append(QString("%1='%2',").arg(fld_ni_name).arg(ni->getShortName()));
        if(state.testFlag(ES_NI_LNAME))
            s.append(QString("%1='%2',").arg(fld_ni_lname).arg(ni->getLongName()));
        if(state.testFlag(ES_NI_SYMBOL))
            s.append(QString("%1='%2',").arg(fld_ni_remcode).arg(ni->getRemCode()));
        if(s.endsWith(","))
            s.chop(1);
        s.append(QString(" where id=%1").arg(ni->getId()));
    }
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(ni->getId() == UNID){
        if(!q.exec("select last_insert_rowid()") || !q.first())
            return false;
        ni->id = q.value(0).toInt();
        SubjectManager::nameItems[ni->id] = ni;
    }
    ni->resetEditState();
    return true;
}

bool DbUtil::_removeNameItem(SubjectNameItem *ni)
{
    if(ni->getId() == UNID)
        return true;
    QSqlQuery q(db);
    QString s = QString("delete from %1 where id=%2").arg(tbl_nameItem).arg(ni->getId());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    SubjectManager::nameItems.remove(ni->id);
    return true;
}

/**
 * @brief 将指定时间范围内的待合并科目的余额按月合并到保留科目
 * @param startYear
 * @param startMonth
 * @param endYear
 * @param endMonth
 * @param preSub        保留科目
 * @param mergedSubs    待合并科目
 * @return
 */
bool DbUtil::_mergeExtraWithinRange(int startYear, int startMonth, int endYear, int endMonth, SecondSubject *preSub, QList<SecondSubject *> mergedSubs)
{
    for(int y = startYear; y < endYear; ++y){
        int sm,em;
        if(y == startYear)
            sm = startMonth;
        else
            sm = 1;
        if(y == endYear)
            em = endMonth;
        else
            em = 12;
        for(int m = sm; m <= em; ++m){
            if(!_mergeExtraWithinMonth(y,m,preSub,mergedSubs))
                return false;
        }
    }
    return true;
}

/**
 * @brief 将指定余额的待合并科目的余额合并到保留科目
 * @param year
 * @param month
 * @param preSub        保留科目
 * @param mergedSubs    待合并科目
 * @return
 */
bool DbUtil::_mergeExtraWithinMonth(int year, int month, SecondSubject *preSub, QList<SecondSubject *> mergedSubs)
{
    QHash<int,int> points;
    if(!_readExtraPoint(year,month,points))
        return false;
    if(points.isEmpty())
        return true;
    //先处理本币（人民币）余额
    int point  = points.value(masterMt);
    if(!_mergeExtra(point,preSub,mergedSubs))
        return false;
    points.remove(masterMt);
    if(points.isEmpty())
        return true;
    //再处理外币余额
    foreach(int point, points){
        //因为表SE_MM_S没有方向字段，它的方向要参考原币余额表SE_PM_S，
        //因此要先处理本币形式的余额表SE_MM_S
        if(!_mergeExtra(point,preSub,mergedSubs,false))
            return false;
        if(!_mergeExtra(point,preSub,mergedSubs))
            return false;
    }
    return true;
}

/**
 * @brief 合并指定月份和币种的科目余额到保留科目
 *  余额指针唯一确定了余额的月份和币种
 * @param point         余额指针
 * @param preSub        保留科目
 * @param mergedSubs    待合并科目
 * @param isPrimary     true：原币余额，false：本币余额
 * @return
 */
bool DbUtil::_mergeExtra(int point, SecondSubject *preSub, QList<SecondSubject *> mergedSubs, bool isPrimary)
{
    QSqlQuery q(db);
    QString s;
    QString subStr = QString("(%1=%2 or ").arg(fld_nse_sid).arg(preSub->getId());
    foreach(SecondSubject* sub, mergedSubs){
        subStr.append(QString("%1=%2 or ").arg(fld_nse_sid).arg(sub->getId()));
    }
    subStr.chop(4);
    subStr.append(")");
    QHash<int,MoneyDirection> ds;
    QHash<int,Double> vs;
    QList<int> rowIds;
    Double preV;            //保留科目的余额
    MoneyDirection preDir=MDIR_P;  //保留科目的余额方向
    bool d_exist=false,v_exist=false;   //余额及其方向记录是否存在
    //如果是处理原币余额，则可以一次性读取余额及其方向
    if(isPrimary){
        s = QString("select id,%1,%2,%3 from %4 where %5=%6 and ").arg(fld_nse_sid)
                .arg(fld_nse_value).arg(fld_nse_dir).arg(tbl_nse_p_s)
                .arg(fld_nse_pid).arg(point);
        s.append(subStr);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next()){
            int sid = q.value(1).toInt();
            if(sid == preSub->getId()){
                d_exist=true;v_exist=true;
                preV = Double(q.value(2).toDouble());
                preDir = (MoneyDirection)q.value(3).toInt();
            }
            else{
                rowIds<<q.value(0).toInt();
                ds[sid] = (MoneyDirection)q.value(3).toInt();
                vs[sid] = Double(q.value(2).toDouble());
            }
        }
    }
    else{ //否者，需要进行两次读取
        s = QString("select %1,%2 from %3 where %4=%5 and ").arg(fld_nse_sid)
                .arg(fld_nse_dir).arg(tbl_nse_p_s).arg(fld_nse_pid).arg(point);
        s.append(subStr);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next()){
            int sid = q.value(0).toInt();
            if(sid == preSub->getId()){
                d_exist = true;
                preDir = (MoneyDirection)q.value(1).toInt();
            }
            else
                ds[sid] = (MoneyDirection)q.value(1).toInt();
        }
        s = QString("select id,%1,%2 from %3 where %4=%5 and ").arg(fld_nse_sid)
                .arg(fld_nse_value).arg(tbl_nse_m_s).arg(fld_nse_pid).arg(point);
        s.append(subStr);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next()){
            int sid = q.value(1).toInt();
            if(sid == preSub->getId()){
                v_exist = true;
                preV = Double(q.value(2).toDouble());
            }
            else{
                rowIds<<q.value(0).toInt();
                vs[sid] = Double(q.value(2).toDouble());
            }
        }
        if((v_exist && !d_exist) || (!v_exist && d_exist)){
            LOG_ERROR("Extra value and direction not exist at same time!");
            return false;
        }
    }
    //计算待合并科目的汇总余额
    Double v,sum;
    if(ds.count() != vs.count()){
        LOG_ERROR("Extra items not equal extra direction items!");
        return false;
    }
    QHashIterator<int,MoneyDirection> it(ds);
    while(it.hasNext()){
        it.next();
        if(!vs.contains(it.key())){
            LOG_ERROR("Extra items not exist but extra direction items exist!");
            return false;
        }
        v = vs.value(it.key());
        if(it.value() == MDIR_D)
            v.changeSign();
        sum += v;
    }
    QString tableName = isPrimary?tbl_nse_p_s:tbl_nse_m_s;
    if(!rowIds.isEmpty()){
        foreach(int id, rowIds){
            s = QString("delete from %1 where id=%2").arg(tableName).arg(id);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }
    if(sum == 0) //汇总后的余额都相互抵消，则无须合并到保留科目
        return true;
    if(preDir == MDIR_D)
        preV.changeSign();
    preV += sum;
    if(preV == 0)
        preDir = MDIR_P;
    else if(preV < 0){
        preDir = MDIR_D;
        preV.changeSign();
    }
    else
        preDir = MDIR_J;
    if(isPrimary){
        if(d_exist)
            s = QString("update %1 set %2=%3,%4=%5 where %6=%7 and %8=%9").arg(tbl_nse_p_s)
                    .arg(fld_nse_value).arg(preV.toString2()).arg(fld_nse_dir).arg(preDir)
                    .arg(fld_nse_pid).arg(point).arg(fld_nse_sid).arg(preSub->getId());
        else
            s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,%8,%9)").arg(tbl_nse_p_s)
                    .arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value).arg(fld_nse_dir)
                    .arg(point).arg(preSub->getId()).arg(preV.toString2()).arg(preDir);

    }
    else{
        if(v_exist)
            s = QString("update %1 set %2=%3 where %4=%5 and %6=%7").arg(tbl_nse_m_s)
                    .arg(fld_nse_value).arg(preV.toString2()).arg(fld_nse_pid).arg(point)
                    .arg(fld_nse_sid).arg(preSub->getId());
        else
            s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)").arg(tbl_nse_m_s)
                    .arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value)
                    .arg(point).arg(preSub->getId()).arg(preV.toString2());
    }
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief 将指定时间范围内的凭证中包含的分录中的待合并科目用保留科目进行替换
 * @param startYear
 * @param startMonth
 * @param endYear
 * @param endMonth
 * @param preSub        保留科目
 * @param mergedSubs    待合并科目
 * @return
 */
bool DbUtil::_replaceSidWithResorved(int startYear, int startMonth, int endYear, int endMonth, SecondSubject *preSub, QList<SecondSubject *> mergedSubs)
{
    QSqlQuery q(db);
    QDate date;
    date.setDate(startYear,startMonth,1);
    QString strSD = date.toString(Qt::ISODate);
    date.setDate(endYear,endMonth,1);
    date.setDate(endYear,endMonth,date.daysInMonth());
    QString strED = date.toString(Qt::ISODate);
    QString s = QString("select id from %1 where (%2 >= '%3') and (%2 <= '%4') order by %2")
            .arg(tbl_pz).arg(fld_pz_date).arg(strSD).arg(strED);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QList<int> pids;
    while(q.next())
        pids<<q.value(0).toInt();
    FirstSubject* fsub = preSub->getParent();
    s = QString("update %1 set %2=%3 where %4=:pid and %5=%6 and ").arg(tbl_ba)
            .arg(fld_ba_sid).arg(preSub->getId()).arg(fld_ba_pid).arg(fld_ba_fid)
            .arg(fsub->getId());
    QString subStr = "(";
    foreach(SecondSubject* sub, mergedSubs){
        subStr.append(QString("%1=%2 or ").arg(fld_ba_sid).arg(sub->getId()));
    }
    subStr.chop(4);
    subStr.append(")");
    s.append(subStr);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int total = 0;
    foreach(int pid, pids){
        q.bindValue(":pid",pid);
        if(!q.exec()){
            LOG_ERROR("Failed -- On replace second subject with preserve subject in BusiAction");
            return false;
        }
        total += q.numRowsAffected();
    }
    LOG_INFO(QString("Total replace %1 rows on merge second subject!").arg(total));
    return true;
}

/**
 * @brief 将指定时间范围内的凭证中包含的分录中的待合并科目用保留科目进行替换
 * @param startYear
 * @param startMonth
 * @param endYear
 * @param endMonth
 * @param preSub        保留科目
 * @param mergedSubs    待合并科目
 * @return
 */
bool DbUtil::_replaceSidWithResorved2(int startYear, int startMonth, int endYear, int endMonth, SecondSubject *preSub, QList<SecondSubject *> mergedSubs)
{
    //反向解决方案，即首先读取所有相关分录（即分录所属凭证在参数指定的时间范围内，且其一级科目和二级科目匹配待合并科目）
    //的id。这可以在一个查询语句完成。然后用此id列表逐条更新科目。
    QSqlQuery q(db);
    QDate date;
    date.setDate(startYear,startMonth,1);
    QString strSD = date.toString(Qt::ISODate);
    date.setDate(endYear,endMonth,1);
    date.setDate(endYear,endMonth,date.daysInMonth());
    QString strED = date.toString(Qt::ISODate);
    QString subStr = "(";
    foreach(SecondSubject* sub, mergedSubs){
        subStr.append(QString("%1.%2=%3 or ").arg(tbl_ba).arg(fld_ba_sid).arg(sub->getId()));
    }
    subStr.chop(4);
    subStr.append(")");
    QString s = QString("select %1.id from %1 join %2 on %1.%3=%2.id where "
                        "%2.%4>='%5' and %2.%4<='%6' and %1.%7=%8 and %9")
            .arg(tbl_ba).arg(tbl_pz).arg(fld_ba_pid).arg(fld_pz_date)
            .arg(strSD).arg(strED).arg(fld_ba_fid).arg(preSub->getParent()->getId())
            .arg(subStr);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QList<int> ids;
    while(q.next())
        ids<<q.value(0).toInt();
    if(ids.isEmpty())
        return true;
    s = QString("update %1 set %2=%3 where id=:id ").arg(tbl_ba).arg(fld_ba_sid)
            .arg(preSub->getId());
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    int total = 0;
    foreach(int id, ids){
        q.bindValue(":id",id);
        if(!q.exec()){
            LOG_ERROR("Replace second subject with preserv second subject failed!");
            return false;
        }
        total++;
    }
    LOG_INFO(QString("Total replace %1 rows on merge second subject!").arg(total));
    return true;
}

/**
 * @brief DbUtil::_savePzForInfos
 *  保存凭证的信息部分内容
 * @param pz
 * @return
 */
bool DbUtil::_savePingZheng(PingZheng *pz)
{
    QSqlQuery q(db);
    QString s;
    PingZhengEditStates state = pz->getEditState();
    if(state == ES_PZ_INIT)
        return true;
    if(pz->id() == UNID){  //如果是新建凭证
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12) "
                    "values('%13',%14,%15,%16,%17,%18,%19,%20,%21,%22,%23)")
                .arg(tbl_pz).arg(fld_pz_date).arg(fld_pz_number).arg(fld_pz_zbnum)
                .arg(fld_pz_jsum).arg(fld_pz_dsum).arg(fld_pz_class).arg(fld_pz_encnum)
                .arg(fld_pz_state).arg(fld_pz_ru).arg(fld_pz_vu).arg(fld_pz_bu)
                .arg(pz->getDate()).arg(pz->number()).arg(pz->zbNumber())
                .arg(pz->jsum().toString2()).arg(pz->dsum().toString2()).arg(pz->getPzClass())
                .arg(pz->encNumber()).arg(pz->getPzState())
                .arg(pz->recordUser()?pz->recordUser()->getUserId():0)
                .arg(pz->verifyUser()?pz->verifyUser()->getUserId():0)
                .arg(pz->bookKeeperUser()?pz->bookKeeperUser()->getUserId():0);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        s = "select last_insert_rowid()";
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        q.first(); pz->ID = q.value(0).toInt();
        if(!pz->memInfo().isEmpty()){
            s = QString("insert into %1(%2,%3) values(%4,'%5')").arg(tbl_pz_meminfos)
                    .arg(fld_pzmi_pid).arg(fld_pzmi_info).arg(pz->id()).arg(pz->memInfo());
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        return _saveBusiactionsInPz(pz);
    }
    else{   //如果是原有凭证        
        s = QString("update %1 set ").arg(tbl_pz);
        if(state.testFlag(ES_PZ_DATE))
            s.append(QString("%1='%2',").arg(fld_pz_date).arg(pz->getDate()));
        if(state.testFlag(ES_PZ_PZNUM))
            s.append(QString("%1=%2,").arg(fld_pz_number).arg(pz->number()));
        if(state.testFlag(ES_PZ_ZBNUM))
            s.append(QString("%1=%2,").arg(fld_pz_zbnum).arg(pz->zbNumber()));
        if(state.testFlag(ES_PZ_ENCNUM))
            s.append(QString("%1=%2,").arg(fld_pz_encnum).arg(pz->encNumber()));
        if(state.testFlag(ES_PZ_PZSTATE))
            s.append(QString("%1=%2,").arg(fld_pz_state).arg(pz->getPzState()));
        if(state.testFlag(ES_PZ_JSUM))
            s.append(QString("%1=%2,").arg(fld_pz_jsum).arg(pz->jsum().toString2()));
        if(state.testFlag(ES_PZ_DSUM))
            s.append(QString("%1=%2,").arg(fld_pz_dsum).arg(pz->dsum().toString2()));
        if(state.testFlag(ES_PZ_CLASS))
            s.append(QString("%1=%2,").arg(fld_pz_class).arg(pz->getPzClass()));
        if(state.testFlag(ES_PZ_RUSER))
            s.append(QString("%1=%2,").arg(fld_pz_ru).arg(pz->recordUser()->getUserId()));
        if(state.testFlag(ES_PZ_VUSER))
            s.append(QString("%1=%2,").arg(fld_pz_vu).arg(pz->verifyUser()->getUserId()));
        if(state.testFlag(ES_PZ_BUSER))
            s.append(QString("%1=%2,").arg(fld_pz_bu).arg(pz->bookKeeperUser()->getUserId()));

        if(s.endsWith(',')){
            s.chop(1);
            s.append(QString(" where id=%1").arg(pz->id()));
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        if(state.testFlag(ES_PZ_MEMINFO)){
            s = QString("update %1 set %2='%3' where %4=%5").arg(tbl_pz_meminfos)
                    .arg(fld_pzmi_info).arg(pz->memInfo()).arg(fld_pzmi_pid).arg(pz->id());
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            if(q.numRowsAffected() == 0){
                s = QString("insert into %1(%2,%3) values(%4,'%5')").arg(tbl_pz_meminfos)
                        .arg(fld_pzmi_pid).arg(fld_pzmi_info).arg(pz->id()).arg(pz->memInfo());
                if(!q.exec(s)){
                    LOG_SQLERROR(s);
                    return false;
                }
            }
        }

        if(state.testFlag(ES_PZ_BACTION))
            return _saveBusiactionsInPz(pz);
    }

    return true;
}

/**
 * @brief DbUtil::_savePzForBas
 *  保存凭证的会计分录内容
 * @param pz
 * @return
 */
bool DbUtil::_saveBusiactionsInPz(PingZheng *pz)
{
    QSqlQuery q(db);
    QString s;
    BusiAction* ba;
    for(int i = 0; i < pz->baCount(); ++i){
        ba = pz->getBusiAction(i);
        BusiActionEditStates state = ba->getEditState();
        if(ba->getId() != UNID && state == ES_BA_INIT)
            continue;
        //会计分录可能引用了一个新建的二级科目，因此要先保存该二级科目
        if(ba->getSecondSubject() && ba->getSecondSubject()->getId() == 0){
            if(!_saveSecondSubject(ba->getSecondSubject()))
                return false;
        }
        if(ba->getId() == UNID){ //新会计分录
            s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9) "
                        "values(%10,'%11',%12,%13,%14,%15,%16,%17)")
                    .arg(tbl_ba).arg(fld_ba_pid).arg(fld_ba_summary).arg(fld_ba_fid)
                    .arg(fld_ba_sid).arg(fld_ba_mt).arg(fld_ba_value).arg(fld_ba_dir)
                    .arg(fld_ba_number).arg(pz->id()).arg(ba->getSummary())
                    .arg(ba->getFirstSubject()?ba->getFirstSubject()->getId():0)
                    .arg(ba->getSecondSubject()?ba->getSecondSubject()->getId():0)
                    .arg(ba->getMt()?ba->getMt()->code():0)
                    .arg(ba->getValue().toString2()).arg(ba->getDir())
                    .arg(i+1);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            s = "select last_insert_rowid()";
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            q.first();
            ba->id = q.value(0).toInt();
        }
        else{   //原有的已修改会计分录
            s = QString("update %1 set ").arg(tbl_ba);            
            if(state.testFlag(ES_BA_NUMBER))
                s.append(QString("%1=%2,").arg(fld_ba_number).arg(i+1));
            if(state.testFlag(ES_BA_PARENT))
                s.append(QString("%1=%2,").arg(fld_ba_pid).arg(pz->id()));
            if(state.testFlag(ES_BA_SUMMARY))
                s.append(QString("%1='%2',").arg(fld_ba_summary).arg(ba->getSummary()));
            if(state.testFlag(ES_BA_FSUB))
                s.append(QString("%1=%2,").arg(fld_ba_fid).arg(ba->getFirstSubject()?ba->getFirstSubject()->getId():0));
            if(state.testFlag(ES_BA_SSUB))
                s.append(QString("%1=%2,").arg(fld_ba_sid).arg(ba->getSecondSubject()?ba->getSecondSubject()->getId():0));
            if(state.testFlag(ES_BA_MT))
                s.append(QString("%1=%2,").arg(fld_ba_mt).arg(ba->getMt()->code()));
            if(state.testFlag(ES_BA_VALUE))
                s.append(QString("%1=%2,").arg(fld_ba_value).arg(ba->getValue().toString2()));
            if(state.testFlag(ES_BA_DIR))
                s.append(QString("%1=%2,").arg(fld_ba_dir).arg(ba->getDir()));
            if(s.endsWith(',')){
                s.chop(1);
                s.append(QString(" where id=%1").arg(ba->getId()));
                if(!q.exec(s)){
                    LOG_SQLERROR(s);
                    return false;
                }
            }
        }
        ba->setEditState(ES_BA_INIT);
    }
    for(int i = 0; i < pz->baDels.count(); ++i){
        ba = pz->baDels.takeAt(0);
        if(ba->getId() != UNID){
            s = QString("delete from %1 where id=%2").arg(tbl_ba).arg(ba->getId());
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        ba->id = UNID;
        pz->ba_saveAfterDels<<ba;
    }
    return true;
}

/**
 * @brief DbUtil::_delPingZheng
 * @param pz
 * @return
 */
bool DbUtil::_delPingZheng(PingZheng *pz)
{
    Q_ASSERT(pz);
    if(pz->id() == UNID)
        return true;

    QSqlQuery q(db);
    QString s = QString("delete from %1 where %2=%3").arg(tbl_ba).arg(fld_ba_pid).arg(pz->id());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    s = QString("delete from %1 where %2=%3").arg(tbl_pz_meminfos).arg(fld_pzmi_pid).arg(pz->id());
    if(!q.exec(s))
        LOG_SQLERROR(s);

    s = QString("delete from %1 where id=%2").arg(tbl_pz).arg(pz->id());
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    return true;
}

/**
 * @brief DbUtil::_readExtraPointInYear
 *  读取指定年份内的所有本币余额指针
 * @param y
 * @param points
 * @return
 */
bool DbUtil::_readExtraPointInYear(int y, QList<int>& points)
{
    QSqlQuery q(db);
    QString s = QString("select id from %1 where %2=%3 and %4=%5 order by %6")
            .arg(tbl_nse_point).arg(fld_nse_year).arg(y)
            .arg(fld_nse_mt).arg(masterMt).arg(fld_nse_month);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        points<<q.value(0).toInt();
    }
    return true;
}

/**
 * @brief DbUtil::readExtraPoint
 *  读取指定年月的余额指针Pid
 * @param y
 * @param m
 * @param mtHashs   键为币种代码，值为余额指针pid
 * @return
 */
bool DbUtil::_readExtraPoint(int y, int m, QHash<int, int>& mtHashs)
{
    QSqlQuery q(db);
    QString s;

    //首先查找余额指针缓存表内是否存在，如果存在，则直接从缓存表中读取，否则从数据库的指针表中读取
    //因为传入的参数“mtHashs”是一个空表，不能对他迭代，但又不知道数据库中到底保存了哪些币种
    //对应的余额，所有由于还缺少一个币种的获取机制，且暂缓实施这一缓存机制
//    QHashIterator<int,int> it(mtHashs);
//    int key,id;
//    while(it.hasNext()){
//        it.next();
//        key = _genKeyForExtraPoint(y,m,it.key());
//        if(key == 0)
//            return false;
//        if(!extraPoints.contains(key)){
//            s = QString("select id from %1 where %2=%3 and %4=%5 and %6=%7")
//                    .arg(tbl_nse_point).arg(fld_nse_year).arg(y)
//                    .arg(fld_nse_month).arg(m).arg(fld_nse_mt).arg(it.key());
//            if(!q.exec(s))
//                return false;
//            if(!q.first()){
//                continue;
//            }
//            id = q.value(0).toInt();
//            mtHashs[it.key()] = id;
//            extraPoints[key] = id;
//        }
//        else
//            mtHashs[it.key()] = extraPoints.value(key);
//    }

    //1、首先取得保存指定年月余额值的指针id
    s = QString("select id,%1 from %2 where %3=%4 and %5=%6")
            .arg(fld_nse_mt).arg(tbl_nse_point)
            .arg(fld_nse_year).arg(y).arg(fld_nse_month).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next())
        mtHashs[q.value(1).toInt()] = q.value(0).toInt();
    return true;
}

/**
 * @brief DbUtil::_readExtraPoint
 *  读取指定年、月、币种的指针
 * @param y
 * @param m
 * @param mt
 * @return
 */
bool DbUtil::_readExtraPoint(int y, int m, int mt, int& pid)
{
    QSqlQuery q(db);
    QString s;
    s = QString("select id from %1 where %2=%3 and %4=%5 and %6=%7")
            .arg(tbl_nse_point).arg(fld_nse_year).arg(y).arg(fld_nse_month).arg(m)
            .arg(fld_nse_mt).arg(mt);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        pid = 0;
        return false;
    }
    pid = q.value(0).toInt();
    return true;
}

/**
 * @brief DbUtil::_readExtraForPm
 *  读取指定年月的余额（原币形式）
 * @param y     年
 * @param m     月
 * @param sums  余额值表
 * @param dirs  余额方向表
 * @param isFst 是一级科目（true：默认值）还是二级科目
 * @return
 */
bool DbUtil::_readExtraForPm(int y, int m, QHash<int, Double> &sums, QHash<int, MoneyDirection> &dirs, bool isFst)
{
    QSqlQuery q(db);
    QHash<int,int> mtHash;  //键为币种代码，值为余额指针pid

    //1、首先取得保存指定年月余额值的指针id
    if(!_readExtraPoint(y,m,mtHash))
        return false;
    if(mtHash.isEmpty())
        return true;

    //2、读取各币种的余额值
    QString s;
    QHashIterator<int,int> it(mtHash);
    int sid,key; Double v;
    MoneyDirection dir;
    QString tname;
    if(isFst)
        tname = tbl_nse_p_f;
    else
        tname = tbl_nse_p_s;
    while(it.hasNext()){
        it.next();
        s = QString("select * from %1 where %2=%3")
                .arg(tname).arg(fld_nse_pid).arg(it.value());
        if(!q.exec(s))
            return false;
        while(q.next()){
            sid = q.value(NSE_E_SID).toInt();
            dir = (MoneyDirection)q.value(NSE_E_DIR).toInt();
            v = Double(q.value(NSE_E_VALUE).toDouble());
            key = sid * 10 + it.key();
            sums[key] = v;
            dirs[key] = dir;
        }
    }
    return true;
}

/**
 * @brief DbUtil::_readExtraForMm
 *  读取指定年月的余额（本币形式）
 * @param y
 * @param m
 * @param sums  余额值表
 * @param isFst 是一级科目（true：默认值）还是二级科目
 * @return
 */
bool DbUtil::_readExtraForMm(int y, int m, QHash<int, Double> &sums, bool isFst)
{
    QSqlQuery q(db);
    QHash<int,int> mtHash;  //键为币种代码，值为余额指针pid

    //1、首先取得保存指定年月余额值的指针id
    if(!_readExtraPoint(y,m,mtHash))
        return false;
    if(mtHash.isEmpty())
        return true;
    //因为只有需要外币的科目才会保存本币形式的余额，因此只有存在外币的本币余额项时才继续读取
    mtHash.remove(masterMt);
    if(mtHash.isEmpty())
        return true;

    //2、读取各币种的余额值
    QString s;
    QHashIterator<int,int> it(mtHash);
    int sid,key; Double v;
    QString tname;
    if(isFst)
        tname = tbl_nse_m_f;
    else
        tname = tbl_nse_m_s;
    while(it.hasNext()){
        it.next();
        s = QString("select * from %1 where %2=%3")
                .arg(tname).arg(fld_nse_pid).arg(it.value());
        if(!q.exec(s))
            return false;
        while(q.next()){
            sid = q.value(NSE_E_SID).toInt();
            v = Double(q.value(NSE_E_VALUE).toDouble());
            key = sid * 10 + it.key();
            sums[key] = v;
        }
    }
    return true;
}

/**
 * @brief DbUtil::_replaeAccurateExtra
 *  用精确值替换直接从原币转换为本币的值
 *  此函数用于那些需要按币种进行分别核算的科目，这些科目的外币余额往往有原币和本币形式，
 *  但它们之间不是精确地符合当前的汇率比率（即不能简单地用原币余额值乘以汇率得到本币余额值）
 *  而是要用从本币余额表中读取值来替换转换而来的值
 * @param sums      本币余额值表
 * @param asums     替换后的余额值表
 */
void DbUtil::_replaeAccurateExtra(QHash<int, Double> &sums, QHash<int, Double> &asums)
{

    QHashIterator<int,Double> it(asums);
    while(it.hasNext()){
        it.next();
        sums[it.key()] = it.value();
    }
}

/**
 * @brief DbUtil::_crtExtraPoint
 *  创建余额指针
 * @param y
 * @param m
 * @param mt    币种代码
 * @param pid   余额指针
 * @return
 */
bool DbUtil::_crtExtraPoint(int y, int m, int mt, int &pid)
{
    //如果此函数在一个数据库事务中被调用，是否要启动它自己的内嵌事务以取得记录id？
    QSqlQuery q(db);
    QString s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)")
            .arg(tbl_nse_point).arg(fld_nse_year).arg(fld_nse_month)
            .arg(fld_nse_mt).arg(y).arg(m).arg(mt);
    if(!q.exec(s))
        return false;
    s = "select last_insert_rowid()";
    if(!q.exec(s))
        return false;
    if(!q.first())
        return false;
    pid = q.value(0).toInt();
    return true;
}

/**
 * @brief DbUtil::_saveExtrasForPm
 *  保存指定年月的余额到数据库中（原币形式）
 * @param y     年
 * @param m     月
 * @param sums  余额值表
 * @param dirs  余额方向表
 * @param isFst 是一级科目（true：默认值）还是二级科目
 * @return
 */
bool DbUtil::_saveExtrasForPm(int y, int m, const QHash<int, Double> &sums, const QHash<int, MoneyDirection> &dirs, bool isFst)
{
    //操作思路
    //1、先读取保存在表中的余额
    //2、在新余额表上进行迭代操作：
    //（1）如果新值项方向为平，则先跳过（如果老值表中存在对应值项，在迭代结束后会从表中删除）
    //（2）如果新值在老值表中不存在，则执行插入操作（在插入操作前，可能还要执行余额指针的创建）
    //（3）如果新老值方向不同，则更新方向
    //（4）如果新老值不同，则更新值
    //（5）每次迭代的末尾，都从老值表中移除已执行的值项
    //3、迭代完成后，如果老值表不空，则说明遗留的值项都不再存在，可以从相应表中删除

    QSqlQuery q(db);
    QString s;
    QHash<int,int> mtHashs; //键为币种代码，值为余额指针

    if(!_readExtraPoint(y,m,mtHashs))
        return false;

    QHash<int, Double> oldSums;
    QHash<int, MoneyDirection> oldDirs;
    if(!_readExtraForPm(y,m,oldSums,oldDirs,isFst))
        return false;

    QHashIterator<int,Double> it(sums);
    int mt,sid;
    QString tname;
    if(isFst)
        tname = tbl_nse_p_f;
    else
        tname = tbl_nse_p_s;
    while(it.hasNext()){
        it.next();
        s.clear();
        mt = it.key() % 10;
        sid = it.key() / 10;
        //（1）如果新值项方向为平，则先跳过（如果老值表中存在对应值项，在迭代结束后遗留在老值表中的值项是新值为0的，所以后面要从数据库中移除这些值项占用的记录）
        if(dirs.value(it.key()) == MDIR_P)
            continue;
        //（2）如果新值在老值表中不存在，则执行插入操作
        else if(!oldSums.contains(it.key())){
            if(!mtHashs.contains(mt)){
                int pid;
                if(!_crtExtraPoint(y,m,mt,pid))
                    return false;
                mtHashs[mt] = pid;
            }
            s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,%8,%9)")
                    .arg(tname).arg(fld_nse_pid).arg(fld_nse_sid)
                    .arg(fld_nse_value).arg(fld_nse_dir).arg(mtHashs.value(mt))
                    .arg(sid).arg(it.value().toString2()).arg(dirs.value(it.key()));

        }
        //（3）、如果新老值或者方向不同，则更新值或方向
        else if((dirs.value(it.key()) != oldDirs.value(it.key())) ||
                (it.value() != oldSums.value(it.key()))){
            s = QString("update %1 set ").arg(tname);
            if(dirs.value(it.key()) != oldDirs.value(it.key())) //方向不同
                s.append(QString("%1=%2,").arg(fld_nse_dir).arg(dirs.value(it.key())));
            if(it.value() != oldSums.value(it.key())) //值不同
                s.append(QString("%1=%2,").arg(fld_nse_value).arg(it.value().toString2()));
            s.chop(1);
            s.append(QString(" where %1=%2 and %3=%4").arg(fld_nse_pid).arg(mtHashs.value(mt)).arg(fld_nse_sid).arg(sid));
        }
        //（5）每次迭代的末尾，都从老值表中移除已执行的值项
        if(!s.isEmpty() && !q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        oldSums.remove(it.key());
        oldDirs.remove(it.key());
    }
    //迭代完成后，如果老值表不空，则说明遗留的值项都不再存在，可以从相应表中删除对应记录
    if(!oldSums.isEmpty()){
        QHashIterator<int,Double> ii(oldSums);
        while(ii.hasNext()){
            ii.next();
            mt = ii.key() % 10;
            sid = ii.key() / 10;
            s = QString("delete from %1 where %2=%3 and %4=%5")
                    .arg(tname).arg(fld_nse_pid).arg(mtHashs.value(mt))
                    .arg(fld_nse_sid).arg(sid);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief DbUtil::_saveExtrasForMm
 *  保存指定年月的余额到数据库中（本币形式）
 * @param y     年
 * @param m     月
 * @param sums  余额值表
 * @param dirs  余额方向表
 * @param isFst 是一级科目（true：默认值）还是二级科目
 * @return
 */
bool DbUtil::_saveExtrasForMm(int y, int m, const QHash<int, Double> &sums, bool isFst)
{
    //假定余额值表中已经剔除了人民币科目的余额值（即只有那些需要外币的科目才需要保存）
    QSqlQuery q(db);
    QString s;
    QHash<int,int> mtHashs; //键为币种代码，值为余额指针


    if(!_readExtraPoint(y,m,mtHashs))
        return false;
    QHash<int, Double> oldSums;
    if(!_readExtraForMm(y,m,oldSums,isFst))
        return false;

    QHashIterator<int,Double> it(sums);
    int mt,sid;
    QString tname;
    if(isFst)
        tname = tbl_nse_m_f;
    else
        tname = tbl_nse_m_s;
    while(it.hasNext()){
        it.next();
        s.clear();
        mt = it.key() % 10;
        sid = it.key() / 10;
        //（1）
        if(sums.value(it.key()) == 0)
            continue;
        //（2）如果新值在老值表中不存在，则执行插入操作
        else if(!oldSums.contains(it.key())){
            if(!mtHashs.contains(mt)){
                int pid;
                if(!_crtExtraPoint(y,m,mt,pid))
                    return false;
                mtHashs[mt] = pid;
            }
            s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)")
                    .arg(tname).arg(fld_nse_pid).arg(fld_nse_sid)
                    .arg(fld_nse_value).arg(mtHashs.value(mt))
                    .arg(sid).arg(it.value().toString2());

        }
        //（3）、如果新老值不同，则更新值
        else if(it.value() != oldSums.value(it.key())){
            s = QString("update %1 set %2=%3 where %4=%5 and %6=%7").arg(tname)
                    .arg(fld_nse_value).arg(it.value().toString2())
                    .arg(fld_nse_pid).arg(mtHashs.value(mt)).arg(fld_nse_sid).arg(sid);

        }
        //（5）每次迭代的末尾，都从老值表中移除已执行的值项
        if(!s.isEmpty() && !q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        oldSums.remove(it.key());
    }
    //迭代完成后，如果老值表不空，则说明遗留的值项都不再存在，可以从相应表中删除
    if(!oldSums.isEmpty()){
        QHashIterator<int,Double> ii(oldSums);
        while(ii.hasNext()){
            ii.next();
            mt = ii.key() % 10;
            sid = ii.key() / 10;
            s = QString("delete from %1 where %2=%3 and %4=%5")
                    .arg(tname).arg(fld_nse_pid).arg(mtHashs.value(mt))
                    .arg(fld_nse_sid).arg(sid);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief DbUtil::_readExtraForSubMoney
 *  读取指定年、月、币种的科目余额及其方向
 * @param y
 * @param m
 * @param mt    币种代码
 * @param sid   科目id
 * @param v     余额（原币形式）
 * @param wv    余额（本币形式）
 * @param dir   余额方向
 * @param fst   一级科目（true）或二级科目（false）
 * @return
 */
bool DbUtil::_readExtraForSubMoney(int y, int m, int mt, int sid, Double &v, Double &wv, MoneyDirection &dir, bool fst)
{
    QSqlQuery q(db);
    QString s;
    int pid;
    if(!_readExtraPoint(y,m,mt,pid))
        return false;
    s = QString("select %1,%2 from %3 where %4=%5 and %6=%7")
            .arg(fld_nse_value).arg(fld_nse_dir).arg(fst?tbl_nse_p_f:tbl_nse_p_s).arg(fld_nse_pid)
            .arg(pid).arg(fld_nse_sid).arg(sid);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        v = 0.0;
        dir = MDIR_P;
    }
    v = Double(q.value(0).toDouble());
    dir = (MoneyDirection)q.value(1).toInt();

    s = QString("select %1 from %2 where %3=%4 and %5=%6")
            .arg(fld_nse_value).arg(fst?tbl_nse_m_f:tbl_nse_m_s).arg(fld_nse_pid)
            .arg(pid).arg(fld_nse_sid).arg(sid);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first())
        wv = 0.0;
    wv = Double(q.value(0).toDouble());
    return true;
}

/**
 * @brief DbUtil::_readExtraForFSub
 *  读取指定年月，指定一级科目的余额
 * @param y
 * @param m
 * @param fid   一级科目id
 * @param v     余额（原币形式，键为币种代码）
 * @param wv    余额（本币形式，键为币种代码）
 * @param dir   余额方向（键为币种代码）
 * @return
 */
bool DbUtil::_readExtraForFSub(int y, int m, int fid, QHash<int, Double> &v, QHash<int, Double> &wv, QHash<int,MoneyDirection> &dir)
{
    QSqlQuery q(db);
    QString s;
    QHash<int,int> pids; //余额指针表
    if(!_readExtraPoint(y,m,pids))
        return false;
    QHashIterator<int,int> it(pids);
    while(it.hasNext()){
        it.next();
        s = QString("select %1,%2 from %3 where %4=%5 and %6=%7")
                .arg(fld_nse_value).arg(fld_nse_dir).arg(tbl_nse_p_f)
                .arg(fld_nse_pid).arg(it.value()).arg(fld_nse_sid).arg(fid);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(q.first()){
            v[it.key()] = Double(q.value(0).toDouble());
            dir[it.key()] = (MoneyDirection)q.value(1).toInt();
        }
        s = QString("select %1 from %2 where %3=%4 and %5=%6")
                .arg(fld_nse_value).arg(tbl_nse_m_f)
                .arg(fld_nse_pid).arg(it.value()).arg(fld_nse_sid).arg(fid);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(q.first())
            wv[it.key()] = Double(q.value(0).toDouble());
    }
    return true;
}

/**
 * @brief DbUtil::_readExtraForSSub
 *  读取指定年月，指定二级科目的余额
 * @param y
 * @param m
 * @param sid   二级科目id
 * @param v     余额（原币形式，键为币种代码）
 * @param wv    余额（本币形式，键为币种代码）
 * @param dir   余额方向（键为币种代码）
 * @return
 */
bool DbUtil::_readExtraForSSub(int y, int m, int sid, QHash<int, Double> &v, QHash<int, Double> &wv, QHash<int, MoneyDirection> &dir)
{
    QSqlQuery q(db);
    QString s;
    QHash<int, int> pids; //余额指针表
    if(!_readExtraPoint(y,m,pids))
        return false;
    QHashIterator<int,int> it(pids);
    while(it.hasNext()){
        it.next();
        s = QString("select %1,%2 from %3 where %4=%5 and %6=%7")
                .arg(fld_nse_value).arg(fld_nse_dir).arg(tbl_nse_p_s)
                .arg(fld_nse_pid).arg(it.value()).arg(fld_nse_sid).arg(sid);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(q.first()){
            v[it.key()] = Double(q.value(0).toDouble());
            dir[it.key()] = (MoneyDirection)q.value(1).toInt();
        }
        s = QString("select %1 from %2 where %3=%4 and %5=%6")
                .arg(fld_nse_value).arg(tbl_nse_m_s)
                .arg(fld_nse_pid).arg(it.value()).arg(fld_nse_sid).arg(sid);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        if(q.first())
            wv[it.key()] = Double(q.value(0).toDouble());
    }
    return true;
}

/**
 * @brief DbUtil::_readExtrasForSubLst
 *  读取指定年月、指定科目集合、指定余额类型（原币或本币）的余额
 * @param y
 * @param m
 * @param sids  科目id列表
 * @param pvs   余额（原币）
 * @param mvs   余额（本币）
 * @param dirs  余额方向
 * @param isFst true：一级科目（默认），false：二级科目
 * @return
 */
bool DbUtil::_readExtrasForSubLst(int y, int m, const QList<int> sids, QHash<int, Double> &pvs, QHash<int, Double> &mvs, QHash<int, MoneyDirection> &dirs, bool isFst/*, bool isPv*/)
{
    QHash<int,int> mtHash;
    if(!_readExtraPoint(y,m,mtHash))
        return false;
    if(mtHash.isEmpty())
        return true;

    QString pt,mt;
    if(isFst){
        pt = tbl_nse_p_f;
        mt = tbl_nse_m_f;
    }
    else{
        pt = tbl_nse_p_s;
        mt = tbl_nse_m_s;
    }

    QHashIterator<int,int> it(mtHash);
    QSqlQuery q(db);
    QString s; int key,sid;
    while(it.hasNext()){
        it.next();
        s = QString("select %1,%2,%3 from %4 where %5=%6").arg(fld_nse_value).arg(fld_nse_dir)
                .arg(fld_nse_sid).arg(pt).arg(fld_nse_pid).arg(it.value());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next()){
            sid = q.value(2).toInt();
            if(!sids.contains(sid))
                continue;
            key = sid*10+it.key();
            pvs[key] = Double(q.value(0).toDouble());
            dirs[key] = (MoneyDirection)q.value(1).toInt();
        }
        if(it.key() == masterMt)
            continue;
        s = QString("select %1,%2 from %3 where %4=%5")
                .arg(fld_nse_value).arg(fld_nse_sid).arg(mt).arg(fld_nse_pid).arg(it.value());
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q.next()){
            sid = q.value(1).toInt();
            if(!sids.contains(sid))
                continue;
            key = sid*10+it.key();
            mvs[key] = Double(q.value(0).toDouble());
        }
    }
    return true;
}

/**
 * @brief DbUtil::_saveExtrasForSubLst
 *  保存指定年月、指定科目集合、指定余额类型（原币或本币）的余额
 * @param y
 * @param m
 * @param sids  科目id列表
 * @param pvs   余额（原币）
 * @param mvs   余额（本币）
 * @param dirs  余额方向
 * @param isFst true：一级科目（默认），false：二级科目
 * @return
 */
bool DbUtil::_saveExtrasForSubLst(int y, int m, const QList<int> sids, const QHash<int, Double> &pvs, const QHash<int, Double> &mvs, const QHash<int, MoneyDirection> &dirs, bool isFst)
{
    QHash<int, Double> old_pvs,old_mvs;
    QHash<int, MoneyDirection> old_dirs;
    if(!_readExtrasForSubLst(y,m,sids,old_pvs,old_mvs,old_dirs,isFst))
        return false;
    QHash<int,int> mtHash;
    if(!_readExtraPoint(y,m,mtHash))
        return false;

    QString tp,tm;
    if(isFst){
        tp = tbl_nse_p_f;
        tm = tbl_nse_m_f;
    }
    else{
        tp = tbl_nse_p_s;
        tm = tbl_nse_m_s;
    }

    QSqlQuery q(db);
    int mt,sid;
    QString s;
    QHashIterator<int, Double>* it = new QHashIterator<int, Double>(pvs);
    while(it->hasNext()){
        it->next();
        mt = it->key() % 10;
        sid = it->key() / 10;
        if(dirs.value(it->key()) == MDIR_P)  //余额为0，不需表保存
            continue;
        //原先表中不存在，则执行插入操作
        else if(!old_pvs.contains(it->key())){
            if(!mtHash.contains(mt) && !_crtExtraPoint(y,m,mt,mtHash[mt]))
                return false;
            s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,%8,%9)")
                    .arg(tp).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value).arg(fld_nse_dir)
                    .arg(mtHash.value(mt)).arg(sid).arg(it->value().toString2()).arg(dirs.value(it->key()));
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        //值或方向不同，则执行更新操作
        else if(it->value() != old_pvs.value(it->key()) || dirs.value(it->key()) != old_dirs.value(it->key())){
            s = QString("update %1 set ").arg(tp);
            if(it->value() != old_pvs.value(it->key()))
                s.append(QString("%1=%2,").arg(fld_nse_value).arg(it->value().toString2()));
            if(dirs.value(it->key()) != old_dirs.value(it->key()))
                s.append(QString("%1=%2,").arg(fld_nse_dir).arg(dirs.value(it->key())));
            s.chop(1);
            s.append(QString(" where %1=%2 and %3=%4").arg(fld_nse_pid).arg(mtHash.value(mt)).arg(fld_nse_sid).arg(sid));
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        old_pvs.remove(it->key());
    }
    //删除已不存在的余额项
    if(!old_pvs.isEmpty()){
        it  = new QHashIterator<int, Double>(old_pvs);
        while(it->hasNext()){
            it->next();
            mt = it->key() % 10;
            sid = it->key() / 10;
            s = QString("delete from %1 where %2=%3 and %4=%5")
                    .arg(tp).arg(fld_nse_pid).arg(mtHash.value(mt)).arg(fld_nse_sid).arg(sid);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }

    //处理本币余额
    it = new QHashIterator<int, Double>(mvs);
    while(it->hasNext()){
        it->next();
        mt = it->key() % 10;
        sid = it->key() / 10;
        if(it->value() == 0)
            continue;
        else if(!old_mvs.contains(it->key())){
            if(!mtHash.contains(mt) && !_crtExtraPoint(y,m,mt,mtHash[mt]))
                return false;
            s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)")
                    .arg(tm).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value)
                    .arg(mtHash.value(mt)).arg(sid).arg(it->value().toString2());
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        else if(it->value() != old_mvs.value(it->key())){
            s = QString("update %1 set %2=%3 where %4=%5 and %6=%7")
                    .arg(tm).arg(fld_nse_value).arg(it->value().toString2()).arg(fld_nse_pid)
                    .arg(mtHash.value(mt)).arg(fld_nse_sid).arg(sid);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
        old_mvs.remove(it->key());        
    }
    if(!old_mvs.isEmpty()){
        it = new QHashIterator<int, Double>(old_mvs);
        while(it->hasNext()){
            it->next();
            mt = it->key() % 10;
            sid = it->key() / 10;
            s = QString("delete from %1 where %2=%3 and %4=%5")
                    .arg(tm).arg(fld_nse_pid).arg(mtHash.value(mt)).arg(fld_nse_sid).arg(sid);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
        }
    }
    return true;
}

bool DbUtil::_convertExtraInYear(int year, const QHash<int, int> maps, bool isFst)
{
    QSqlQuery q1(db),q2(db),q3(db),q4(db);
    QString s;
    QList<int> ePoints;

    if(!_readExtraPointInYear(year,ePoints))
        return false;
    QString tname = isFst?tbl_nse_p_f:tbl_nse_p_s;
    foreach (int p, ePoints) {
        //1、查找源科目的余额是否存在，如果不存在，则不处理，否则，读取此余额以及对接科目的余额，
        //将两者汇总后保存，并移除源科目余额项
        QHashIterator<int,int> it(maps);
        s = QString("select id,%1,%2 from %4 where %5=%6 and %7=:sid")
                .arg(fld_nse_value).arg(fld_nse_dir).arg(tname)
                .arg(fld_nse_pid).arg(p).arg(fld_nse_sid);
        if(!q1.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        s = QString("update %1 set %2=:value,%3=:dir where %4=%5 and %6=:sid")
                .arg(tname).arg(fld_nse_value).arg(fld_nse_dir).arg(fld_nse_pid)
                .arg(p).arg(fld_nse_sid);
        if(!q2.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        s = QString("insert into %1(%2,%3,%4,%5) values(:pid,:sid,:value,:dir)").arg(tname)
                .arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value).arg(fld_nse_dir);
        if(!q3.prepare(s))
            return false;
        s = QString("delete from %1 where id=:id").arg(tname);
        if(!q4.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(it.hasNext()){
            it.next();
            //读取源科目余额
            q1.bindValue(":sid",it.key());
            if(!q1.exec()){
               LOG_SQLERROR(q1.lastQuery());
               return false;
            }
            if(!q1.first())
                continue;
            int id1 = q1.value(0).toInt();
            Double v = Double(q1.value(1).toDouble());
            MoneyDirection dir = (MoneyDirection)q1.value(2).toInt();
            if(dir == MDIR_P){
                q4.bindValue(":id",id1);
                if(!q4.exec())
                    return false;
                continue;
            }
            //读取对接科目余额
            q1.bindValue(":sid",it.value());
            if(!q1.exec()){
               LOG_SQLERROR(q1.lastQuery());
               return false;
            }
            Double sum;
            MoneyDirection d = MDIR_P;
            int id2 = 0;
            if(q1.first()){
                id2 = q1.value(0).toInt();
                sum = Double(q1.value(1).toDouble());
                d = (MoneyDirection)q1.value(2).toInt();
                if(d == MDIR_P)
                    d = dir;
            }
            //调整到与对接科目同样的余额方向
            if(d != dir){
                dir = d;
                v.changeSign();
            }
            sum += v;
            if(sum == 0 ){
                if(id2 != 0){
                    q4.bindValue(":id",id2);
                    if(!q4.exec()){
                        LOG_SQLERROR(q4.lastQuery());
                        return false;
                    }
                }
                else
                    continue;
            }
            q2.bindValue(":sid",it.value());
            q2.bindValue(":value",sum.toString2());
            q2.bindValue(":dir",d);
            if(!q2.exec()){
                LOG_SQLERROR(q2.lastQuery());
                return  false;
            }
            int numRows = q2.numRowsAffected();
            if(numRows != 1){
                q3.bindValue(":pid",p);
                q3.bindValue(":sid",it.value());
                q3.bindValue(":value",sum.toString2());
                q3.bindValue(":dir",d);
                if(!q3.exec())
                    return false;
            }
            q4.bindValue(":id",id1);
            if(!q4.exec()){
                LOG_SQLERROR(q4.lastQuery());
                return false;
            }
        }
    }
    return true;
}



/**
 * @brief DbUtil::_genKeyForExtraPoint
 *  生成由年份、月份和币种构成的复合键，用来索引余额指针表
 * @param y
 * @param m
 * @param mt
 * @return
 */
int DbUtil::_genKeyForExtraPoint(int y, int m, int mt)
{
    if(m < 1 || m > 12 || mt < 1 || mt > 9)
        return 0;
    if(m < 10)
        return y*1000+m*100+mt;
    else if(m > 9 && m <= 12)
        return y*1000+m*10+mt;
}

/**
 * @brief 判定指定年份与前一年份的帐套所使用的科目系统是否发生了变化
 * @param y
 * @param isTrans   是否需要转换
 * @param fMaps     如果发生了变化，则此参数返回一级科目映射表
 * @param sMaps     二级科目映射表
 * @return
 */
bool DbUtil::_isTransformExtra(int y, bool &isTrans, QHash<int, int> &fMaps, QHash<int, int> &sMaps)
{
    QSqlQuery q(db);
    QString s = QString("select %1 from %5 where %2=%3 or %2=%4 order by %2")
            .arg(fld_accs_subSys).arg(fld_accs_year).arg(y).arg(y-1).arg(tbl_accSuites);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        isTrans = false;
        return false;
    }
    if(!q.first()){
        LOG_ERROR("Account %1 is exist!");
        isTrans = false;
        return false;
    }
    int preSubSys = q.value(0).toInt();
    if(!q.next()){
        isTrans = false;
        return true;
    }
    int curSubSys = q.value(0).toInt();
    if(curSubSys == preSubSys){
        isTrans = false;
        return true;
    }
    isTrans = true;
    QString tName = QString("%1_%2_%3").arg(tbl_ssjc_pre).arg(preSubSys).arg(curSubSys);
    s = QString("select * from %1").arg(tName);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QStringList sl;
    int sFid,dFid,sSid,dSid;
    bool isMapping,ok;
    while(q.next()){
        sl.clear();
        isMapping = q.value(SSJC_ISMAP).toBool();
        if(!isMapping){
            errorNotify(QObject::tr("科目系统升级衔接配置不完整！"));
            return false;
        }
        sFid = q.value(SSJC_SSUB).toInt();
        dFid = q.value(SSJC_DSUB).toInt();
        sl = q.value(SSJC_SSUBMaps).toString().split(",",QString::SkipEmptyParts);
        if(!sl.isEmpty()){
            for(int i = 0; i < sl.count(); i+=2){
                sSid = sl.at(i).toInt(&ok);
                dSid = sl.at(i+1).toInt(&ok);
                if(!ok){
                    errorNotify(QObject::tr("二级科目衔接配置有问题（一级科目id=%1）").arg(sFid));
                    return false;
                }
                sMaps[sSid] = dSid;
            }
        }
        fMaps[sFid] = dFid;
    }
    return true;
}

/**
 * @brief 根据指定的科目映射表，转换余额值表
 * @param maps      科目映射表
 * @param ExtrasP   原币形式余额值
 * @param extrasM   本币形式余额值
 * @param dirs      余额方向
 * @return
 */
bool DbUtil::_transformExtra(QHash<int,int> maps, QHash<int,Double>& ExtrasP, QHash<int,Double>& extrasM, QHash<int,MoneyDirection>& dirs)
{
    QHashIterator<int,Double> it(ExtrasP);
    int sid,key,mt;
    while(it.hasNext()){
        it.next();
        sid = it.key()/10;
        mt = it.key()%10;
        if(!maps.contains(sid))
            return false;
        key = maps.value(sid) * 10 + mt;
        ExtrasP[key] = it.value();
        dirs[key] = dirs.value(it.key());
        if(extrasM.contains(it.key())){
            extrasM[key] = extrasM.value(it.key());
            extrasM.remove(it.key());
        }
        ExtrasP.remove(it.key());
        dirs.remove(it.key());
    }
    return true;
}

/**
 * @brief 检测指定一级科目某币种的余额与其子目的汇总余额是否一致
 * @param fid       主目id
 * @param mt        币种代码
 * @param sum       主目余额
 * @param dir       主目方向
 * @param values    子目余额表（键为子目id*10+币种代码）
 * @param dirs      子目余额方向表
 * @param ok        true：一致，false：不一致
 * @return
 */
bool DbUtil::_extraUnityInspectForFSub(int fid, int mt, Double sum, MoneyDirection dir, QHash<int, Double> values, QHash<int,MoneyDirection> dirs, bool& ok)
{

    //将子目余额进行汇总，汇总的余额方向归并到主目的余额方向
    //1、读取主目下的子目id，因为在这里无法辨别科目所属的科目系统，因此可能会读取到不属于当前主目的子目，
    //但因为这样的子目余额项不会出现在值表中，因此无碍。
    if(sum == 0 || dir == MDIR_P){
        ok = true;
        return true;
    }

    QList<int> sids;
    QSqlQuery q(db);
    QString s = QString("select id from %1 where %2=%3").arg(tbl_ssub)
            .arg(fld_ssub_fid).arg(fid);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next())
        sids<<q.value(0).toInt();
    int key;
    Double v,sv;
    MoneyDirection d;
    foreach(int sid, sids){
        key = sid*10 + mt;
        if(!values.contains(key))
            continue;
        d = dirs.value(key);
        if(d == MDIR_P)
            continue;
        v = values.value(key);
        if(d != dir)
            v.changeSign();
        sv += v;
    }
    ok = (sum == sv);
    return true;
}

void DbUtil::crtGdzcTable()
{
    QSqlQuery q1(db);
    QString s;
    bool r;
    ////////////////////////////////////////////////////////////////////
    //创建固定资产类别表
    //s = "CREATE TABLE gdzc_classes(id INTEGER PRIMARY KEY, code INTEGER, name TEXT, zjMonths INTEGER)";
    //r = q1.exec(s);
    //创建固定资产表
    s = "CREATE TABLE gdzcs(id INTEGER PRIMARY KEY,code INTEGER,pcls INTEGER,"
        "scls INTEGER,name TEXT,model TEXT,buyDate TEXT,prime DOUBLE,"
        "remain DOUBLE,min DOUBLE,zjMonths INTEGER,desc TEXT)";
    r = q1.exec(s);
    //创建固定资产折旧信息表
    s = "CREATE TABLE gdzczjs(id INTEGER PRIMARY KEY,gid INTEGER,pid INTEGER,"
        "bid INTEGER,date TEXT,price DOUBLE)";
    r = q1.exec(s);
}



void DbUtil::warn_transaction(ErrorCode witch, QString context)
{
    QString s;
    switch(witch){
    case Transaction_open:
        s = QObject::tr("启动事务");
        break;
    case Transaction_commit:
        s = QObject::tr("提交事务");
        break;
    case Transaction_rollback:
        s = QObject::tr("回滚事务");
        break;
    }
    QString ss = QObject::tr("在%1时，%2失败！").arg(context).arg(s);
    //LOG_SQLERROR(ss);
    QMessageBox::critical(0,QObject::tr("数据库访问出错"),ss);
}

void DbUtil::errorNotify(QString info)
{
    QMessageBox::critical(0,QObject::tr("operate error"),info);
}

/**
 * @brief DbUtil::_getPreYM
 * @param y
 * @param m
 * @param yy
 * @param mm
 */
void DbUtil::_getPreYM(int y, int m, int &yy, int &mm)
{
    if(m == 1){
        yy = y - 1;
        mm = 12;
    }
    else{
        yy = y;
        mm = m - 1;
    }
}

/**
 * @brief DbUtil::_getNextYM
 * @param y
 * @param m
 * @param yy
 * @param mm
 */
void DbUtil::_getNextYM(int y, int m, int &yy, int &mm)
{
    if(m == 12){
        yy = y + 1;
        mm = 1;
    }
    else{
        yy = y;
        mm = m + 1;
    }
}
