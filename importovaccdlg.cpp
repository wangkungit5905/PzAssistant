#include "importovaccdlg.h"
#include "ui_importovaccdlg.h"
#include "account.h"
#include "PzSet.h"
#include "global.h"
#include "commands.h"
#include "subject.h"
#include "tables.h"
#include "pz.h"
#include "suiteswitchpanel.h"
#include "dbutil.h"

#include <QFileDialog>

ImportOVAccDlg::ImportOVAccDlg(Account *account, SuiteSwitchPanel* panel, QWidget *parent) :
    QDialog(parent),ui(new Ui::ImportOVAccDlg),account(account),panel(panel)
{
    ui->setupUi(this);
    connect(this,SIGNAL(switchSuiteTo(int)),panel,SLOT(switchToSuite(int)));
    init();
}

ImportOVAccDlg::~ImportOVAccDlg()
{
    if(!connName.isEmpty())
        QSqlDatabase::removeDatabase(connName);
    disconnect(this,SIGNAL(switchSuiteTo(int)),panel,SLOT(switchToSuite(int)));
    delete ui;
}

/**
 * @brief 改变导入凭证集的日期
 * @param date
 */
void ImportOVAccDlg::importDateChanged(const QDate &date)
{
    QSqlQuery q(sdb);
    QString ds = date.toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select count() from PingZhengs where date like '%1%'").arg(ds);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return;
    }
    q.first();
    int pzCount = q.value(0).toInt();
    if(pzCount == 0){
        ui->spnStart->setMaximum(0);
        ui->spnStart->setMinimum(0);
        ui->spnStart->clear();
        ui->spnEnd->setMaximum(0);
        ui->spnEnd->setMinimum(0);
        ui->spnEnd->cleanText();
        return;
    }
    else{
        ui->spnStart->setMaximum(pzCount);
        ui->spnStart->setMinimum(1);
        ui->spnStart->setValue(1);
        ui->spnEnd->setMaximum(pzCount);
        ui->spnEnd->setMinimum(1);
        ui->spnEnd->setValue(pzCount);
    }
}

bool ImportOVAccDlg::init()
{
    ui->edtCurAcc->setText(tr("%1（%2）").arg(account->getLName()).arg(account->getCode()));
    AccountSuiteRecord* asr = account->getEndSuiteRecord();
    if(!asr){
        QMessageBox::warning(this,"",tr("当前账户没有任何帐套！"));
        return false;
    }
    ui->edtCurSuite->setText(asr->name);
    ui->edtLastMonth->setText(QString::number(asr->endMonth));
    curSuite = account->getSuiteMgr(asr->id);
    sm = curSuite->getSubjectManager();
    if(curSuite != account->getSuiteMgr())
        emit switchSuiteTo(asr->year);
    if(curSuite->isPzSetOpened())
        curSuite->closePzSet();
    curSuite->clearPzSetCaches();
    if(!curSuite->open(asr->endMonth)){
        QMessageBox::warning(this,"",tr("打开最后月份凭证集时出错！"));
        return false;
    }
    ui->edtMaxPzNum->setText(QString::number(curSuite->getPzCount()));
    return true;
}

/**
 * @brief 比较当前账户与导入账户在所选月份内的汇率是否一致
 * @return
 */
bool ImportOVAccDlg::compareRate()
{
    QSqlQuery q(sdb);
    int y = ui->edtDate->date().year();
    int m = ui->edtDate->date().month();
    QString s = QString("select usd2rmb from ExchangeRates where year=%1 and month=%2").arg(y).arg(m);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    bool sRateIsExist = false;
    sRateIsExist = q.first();
    if(!sRateIsExist)
        QMessageBox::warning(this,"",tr("导入账户没有设置%1年%月的汇率").arg(y).arg(m));

    Double srate = Double(q.value(0).toDouble());
    QHash<int,Double> rates;
    account->getDbUtil()->getRates(y,m,rates);
    bool isRateChanged = false;
    if(rates.isEmpty() && sRateIsExist){
        rates[USD] = srate;
        isRateChanged = true;
    }
    else if(sRateIsExist && (srate != rates.value(USD))){
        if(QMessageBox::Yes == QMessageBox::warning(this,"",tr("当前账户与导入账户的汇率设置不一致，如果要以导入账户的汇率设置为准则按是，否者按否！")
                                                    ,QMessageBox::Yes|QMessageBox::No,
                                                    QMessageBox::Yes)){
            rates[USD] = srate;
            isRateChanged = true;
        }
    }
    else if(!sRateIsExist && rates.isEmpty())
        QMessageBox::warning(this,"",tr("导入凭证后请设置正确的汇率！"));

    if(isRateChanged && !account->getDbUtil()->saveRates(y,m,rates)){
        QMessageBox::warning(this,"",tr("将汇率保存到当前账户时出错！"));
        return false;
    }
    return true;
}



/**
 * @brief 建立新旧科目之间的对接表
 * 只有在源帐套和目的帐套所采用的科目系统不同时，且只有那些混合对接科目才需要考虑真正的转换
 * @return
 */
bool ImportOVAccDlg::createMaps()
{
    //如果是从旧科目系统导入到新科目系统，则可以通过新旧科目对接配置信息先建立这些科目id到科目对象的映射
    //如果是从旧科目系统导入到旧科目系统，则不能建立这些映射，而只能遇到一个，然后在目的账户内查找是否存在
    //此科目    
    if(sm->getSubSysCode() != DEFAULT_SUBSYS_CODE){
        //初始化新旧科目系统之间的混合对接项
        QList<MixedJoinCfg*> cfgInfos;
        if(!account->getDbUtil()->getMixJoinInfo(DEFAULT_SUBSYS_CODE,sm->getSubSysCode(),cfgInfos)){
            LOG_ERROR(tr("无法获取新旧科目混合对接表信息！"));
            return false;
        }
        foreach(MixedJoinCfg* item, cfgInfos){
            FirstSubject* fsub = sm->getFstSubject(item->d_fsubId);
            if(!fsub){
                LOG_ERROR(tr("无法找到与旧主目（fid=%1）对应的新主目（fid=%2）").arg(item->s_fsubId).arg(item->d_fsubId));
                return false;
            }
            SecondSubject* ssub = sm->getSndSubject(item->d_ssubId);
            if(!ssub){
                LOG_ERROR(tr("无法找到与旧子目（fid=%1）对应的新子目（fid=%2）").arg(item->s_ssubId).arg(item->d_ssubId));
                return false;
            }
            MixedJoinSubObj* item1 = new MixedJoinSubObj;
            item1->isNew = false;
            item1->sFid = item->s_fsubId;
            item1->sSid = item->s_ssubId;
            item1->dFSub = fsub;
            item1->dSSub = ssub;
            mixedJoinItems<<item1;
        }
        //建立所有科目的对接关系
        QList<SubSysJoinItem2*> cfgs;
        if(!AppConfig::getInstance()->getSubSysMaps(DEFAULT_SUBSYS_CODE,sm->getSubSysCode(),cfgs)){
            QMessageBox::warning(this,"",tr("无法获取新旧科目系统对接配置信息！"));
            return false;
        }
        QSqlQuery q(sdb),q2(sdb);
        QString s = QString("select FSAgent.id,SecSubjects.subName,SecSubjects.subLName,"
                             "SecSubjects.remCode,SecSubjects.classId from FSAgent join SecSubjects on "
                             "FSAgent.sid = SecSubjects.id  where fid=:fid");
        if(!q2.prepare(s)){
            LOG_SQLERROR(s);
            return false;
        }
        FirstSubject* fsub;
        int fid;
        foreach(SubSysJoinItem2* item, cfgs){
            //处理主目映射
            s = QString("select id from firSubjects where subCode='%1'").arg(item->scode);
            if(!q.exec(s)){
                LOG_SQLERROR(s);
                return false;
            }
            if(!q.first()){
                QMessageBox::warning(this,"",tr("无法获取旧科目系统中代码为“%1”的科目！").arg(item->scode));
                return false;
            }
            fid = q.value(0).toInt();
            fsub = sm->getFstSubject(item->dcode);
            if(!fsub){
                QMessageBox::warning(this,"",tr("无法获取新科目系统中代码为“%1”的科目！").arg(item->dcode));
                return false;
            }
            fsubIdMaps[fid] = fsub;
            //处理子目映射
            if(!processSSubMaps(fid,q2))
                return false;
        }
    }
    return true;
}

/**
 * @brief 从源账户数据库中导入指定月份内的指定区间的凭证
 * @return
 */
bool ImportOVAccDlg::importPzSet()
{
    QSqlQuery q1(sdb),q2(sdb),q3(sdb);//q1查询凭证q2查询分录q3查询子目映射
    QString s = QString("select FSAgent.id,SecSubjects.subName,SecSubjects.subLName,"
                         "SecSubjects.remCode,SecSubjects.classId from FSAgent join SecSubjects on "
                         "FSAgent.sid = SecSubjects.id  where fid=:fid");
    if(!q3.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QHash<int,Money*> moneys = curAccount->getAllMoneys();
    QString ds = ui->edtDate->date().toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select * from PingZhengs where date like '%1%'").arg(ds);
    int startPzNum = ui->spnStart->value();
    int endPzNum = ui->spnEnd->value();
    if(startPzNum == endPzNum)
        s.append(QString(" and number=%1").arg(startPzNum));
    else
        s.append(QString(" and number>=%1 and number<=%2").arg(startPzNum).arg(endPzNum));
    s.append(" order by number");
    if(!q1.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QHash<int,QString> fsubIdToCode;//源账户的一级科目id到科目代码的映射
    bool isTran = curSuite->getSubjectManager()->getSubSysCode() != DEFAULT_SUBSYS_CODE;
    if(!isTran){
        s = QString("select id,subCode from firSubjects");
        if(!q2.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        while(q2.next())
            fsubIdToCode[q2.value(0).toInt()] = q2.value(1).toString();
    }
    s = QString("select * from BusiActions where pid=:pid order by NumInPz");
    if(!q2.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    ui->progress->setMaximum(endPzNum - startPzNum + 1);
    ui->progress->setMinimum(0);
    while(q1.next()){
        int pzId = q1.value(0).toInt();
        QString date = q1.value(1).toString();
        int pzNum = q1.value(2).toInt();
        ui->progress->setValue(pzNum);
        QApplication::processEvents();
        int zbNum = q1.value(3).toInt();
        Double jsum = Double(q1.value(4).toDouble());
        Double dsum = Double(q1.value(5).toDouble());
        PzClass pzCls = (PzClass)q1.value(6).toInt();
        int encNum = q1.value(7).toInt();
        PzState pzState = (PzState)q1.value(8).toInt();
        int vuId = q1.value(9).toInt();
        int ruId = q1.value(10).toInt();
        int buId = q1.value(11).toInt();
        PingZheng* pz = new PingZheng(curSuite,0,date,pzNum,zbNum,jsum,dsum,pzCls,encNum,pzState,allUsers.value(vuId),allUsers.value(ruId),allUsers.value(buId));
        q2.bindValue(":pid",pzId);
        if(!q2.exec()){
            LOG_SQLERROR(q2.lastQuery());
            return false;
        }
        int baNum = 0;
        while(q2.next()){
            baNum++;
            QString summary = q2.value(2).toString();
            int fid = q2.value(3).toInt();
            FirstSubject* fsub;
            if(!fsubIdMaps.contains(fid)){
                if(isTran){
                    QMessageBox::warning(this,"",tr("遇到一个一级科目（fid=%1）没有对接科目").arg(fid));
                    return false;
                }
                if(!fsubIdToCode.contains(fid)){
                    QMessageBox::warning(this,"",tr("在%1#凭证的第%2条分录上遇到一个非法一级科目（fid=%3）")
                                         .arg(pzNum).arg(baNum).arg(fid));
                    return false;
                }
                fsub = sm->getFstSubject(fsubIdToCode.value(fid));
                if(!fsub){
                    QMessageBox::warning(this,"",tr("在目的账户内不存在代码为“%1”的一级科目").arg(fsubIdToCode.value(fid)));
                    return false;
                }
                fsubIdMaps[fid] = fsub;
                if(!processSSubMaps(fid,q3))
                    return false;
            }
            else
                fsub = fsubIdMaps.value(fid);
            int sid = q2.value(4).toInt();
            if(!ssubIdMaps.contains(sid)){
                QMessageBox::warning(this,"",tr("遇到一个非法二级科目（sid=%1）").arg(sid));
                return false;
            }
            SecondSubject* ssub = ssubIdMaps.value(sid);
            Money* mt = moneys.value(q2.value(5).toInt());
            int dir = q2.value(8).toInt();
            double v;
            MoneyDirection d;
            if(dir == 1){
                d = MDIR_J;
                v = q2.value(6).toDouble();
            }
            else{
                d = MDIR_D;
                v = q2.value(7).toDouble();
            }
            BusiAction* ba = new BusiAction(0,pz,summary,fsub,ssub,mt,d,v,baNum);
            pz->append(ba,false);
        }
        AppendPzCmd* cmd = new AppendPzCmd(curSuite,pz);
        curSuite->getUndoStack()->push(cmd);
    }
    curSuite->save(AccountSuiteManager::SW_PZS);
    //保存新的混合二级科目配置项
    if(!mixedJoinItems.isEmpty()){
        QList<MixedJoinCfg*> cfgItems;
        foreach(MixedJoinSubObj* item, mixedJoinItems){
            if(!item->isNew)
                continue;
            MixedJoinCfg* m = new MixedJoinCfg;
            m->s_fsubId = item->sFid;
            m->s_ssubId = item->sSid;
            m->d_fsubId = item->dFSub->getId();
            m->d_ssubId = item->dSSub->getId();
            cfgItems<<m;
        }
        if(!account->getDbUtil()->appendMixJoinInfo(DEFAULT_SUBSYS_CODE,sm->getSubSysCode(),cfgItems))
            QMessageBox::critical(this,tr("保存出错"),tr("导入的凭证成功保存，但在保存混合对接科目信息时出错！"));
    }
    curSuite->closePzSet();
    q1.finish();q2.finish();
    return true;
}

/**
 * @brief 处理子目映射，即把源账户中的主目（fid）下的子目，通过子目id映射到目的账户内的子目对象
 * 这些映射保存在ssubIdMaps哈希表内
 * @param fid   主目id
 * @param q     查询子目的预编译sql查询对象
 * @return
 */
bool ImportOVAccDlg::processSSubMaps(int fid, QSqlQuery q)
{
    FirstSubject* fsub = fsubIdMaps.value(fid);
    SubjectNameItem* ni;
    SecondSubject* ssub;
    q.bindValue(":fid",fid);
    if(!q.exec()){
        LOG_SQLERROR(q.lastQuery());
        return false;
    }
    bool isMixedSub = false;
    if(sm->getSubSysCode() != DEFAULT_SUBSYS_CODE){
        foreach(MixedJoinSubObj* item, mixedJoinItems){
            if(item->sFid == fid){
                isMixedSub = true;
                break;
            }
        }
    }
    while(q.next()){
        int sid = q.value(0).toInt();
        QString name = q.value(1).toString();
        ni = sm->getNameItem(name);
        if(!ni){
            //创建名称条目
            QString lname = q.value(2).toString();
            QString remCode = q.value(3).toString();
            int clsId = q.value(4).toInt();
            CrtNameItemCmd* cmd = new CrtNameItemCmd(name,lname,remCode,clsId,QDateTime::currentDateTime(),curUser,sm);
            curSuite->getUndoStack()->push(cmd);
            ni = sm->getNameItem(name);
            LOG_INFO(tr("创建了名称条目：%1").arg(name));
        }
        ssub = fsub->getChildSub(ni);
        if(!ssub){
            //创建二级科目
            QString dateStr = AppConfig::getInstance()->getSpecSubSysItem(sm->getSubSysCode())->startTime.toString(Qt::ISODate);
            QDate endDate = QDate::fromString(dateStr,Qt::ISODate);//默认科目系统的截止日期
            endDate.setDate(endDate.year()-1,12,31);
            if(QDate::currentDate() <= endDate)
                endDate = QDate::currentDate();
            CrtSndSubUseNICmd* cmd = new CrtSndSubUseNICmd(sm,fsub,ni,1,QDateTime(endDate),curUser);
            curSuite->getUndoStack()->push(cmd);
            LOG_INFO(tr("创建了二级科目：%1--%2").arg(fsub->getName()).arg(ni->getShortName()));
            ssub = fsub->getChildSub(ni);
        }
        ssubIdMaps[sid] = ssub;
        //如果新老账户科目系统不同，主目是属于混合对接科目，且取得的二级科目对象不存在于对接表内，
        //则应记录此对接项，在导入成功后保存此对接项
        if(isMixedSub && !isExistCfg(fid,sid)){
            MixedJoinSubObj* item = new MixedJoinSubObj;
            item->isNew = true;
            item->sFid = fid;
            item->sSid = sid;
            item->dFSub = fsub;
            item->dSSub = ssub;
            mixedJoinItems<<item;
        }
    }
    return true;
}

/**
 * @brief 在混合对接二级科目配置列表中查找是否存在指定配置项
 * @param sfid  源一级科目id
 * @param ssid  源二级科目id
 * @return
 */
bool ImportOVAccDlg::isExistCfg(int sfid, int ssid)
{
    if(curSuite->getSubjectManager()->getSubSysCode() == DEFAULT_SUBSYS_CODE)
        return true;
    foreach(MixedJoinSubObj* item, mixedJoinItems){
        if(item->sFid==sfid && item->sSid == ssid)
            return true;
    }
    return false;
}



/**
 * @brief 选择导入账户文件
 */
void ImportOVAccDlg::on_btnFile_clicked()
{
    QString fname = QFileDialog::getOpenFileName(this,tr("请选择与当前账户等同的老版账户文件"));
    if(fname.isEmpty())
        return;

    //2、读取账户文件，判定是否是同一个账户
    ui->edtFileName->setText(fname);
    connName = "ImportPzSetFromOlder";
    sdb = QSqlDatabase::addDatabase("QSQLITE",connName);
    sdb.setDatabaseName(fname);
    if(!sdb.open()){
        QMessageBox::warning(this,"",tr("所选文件不是一个有效的账户文件！"));
        return;
    }
    QString s = QString("select %1 from %2 where %3=%4").arg(fld_acci_value)
            .arg(tbl_accInfo).arg(fld_acci_code).arg(Account::ACODE);
    QSqlQuery qs(sdb);
    if(!qs.exec(s) || !qs.first()){
        LOG_SQLERROR(s);
        QMessageBox::warning(this,"",tr("所选文件不是一个有效的账户文件！"));
        return;
    }

    QString accCode = qs.value(0).toString();
    if(accCode != curAccount->getCode()){
        QMessageBox::warning(this,"",tr("必须选择与当前账户等同的账户文件！"));
        return;
    }

    //判断是否是老版账户（即通过数据库版本号来判断）
    s = QString("select %1 from %2 where %3=%4").arg(fld_acci_value)
                .arg(tbl_accInfo).arg(fld_acci_code).arg(Account::DBVERSION);
    if(!qs.exec(s) || !qs.first()){
        LOG_SQLERROR(s);
        QMessageBox::warning(this,"",tr("无法获取账户数据库版本号！"));
        return;
    }
    QString dbVersion = qs.value(0).toString();
    if(dbVersion != "1.2"){
        QMessageBox::warning(this,"",tr("此账户文件不是老版账户，导入功能只支持从老版账户导入！"));
        return;
    }
    //判断是否存在比当前账户更新的凭证集
    s = QString("select %1 from %2 where %3=%4").arg(fld_acci_value)
                .arg(tbl_accInfo).arg(fld_acci_code).arg(Account::ETIME);
    QDateTime date;
    if(!qs.exec(s) || !qs.first()){
        QMessageBox::warning(this,"",tr("该账户文件缺失记账终止时间信息！"));
        return;
    }
    else
        date = QDateTime::fromString(qs.value(0).toString(),Qt::ISODate);
    int year = date.date().year();
    int month = date.date().month();
    int curYear = curSuite->year();
    int curEM = curSuite->getSuiteRecord()->endMonth;
    if((year < curYear) || ((year == curYear) and (month < curEM))){
        QMessageBox::warning(this,"",tr("该账户不存在比当前账户更新的凭证集可以导入！"));
        return;
    }
    //确定可导入凭证集的时间范围和凭证号范围
    QDate maxDate,minDate;
    maxDate.setDate(year,month,1);
    minDate.setDate(curYear,curEM,1);
    ui->edtDate->setMaximumDate(maxDate);
    ui->edtDate->setMinimumDate(minDate);
    ui->edtDate->setDate(minDate);
    connect(ui->edtDate,SIGNAL(dateChanged(QDate)),this,SLOT(importDateChanged(QDate)));
    importDateChanged(minDate);
}

void ImportOVAccDlg::on_btnImport_clicked()
{
    //首先要判定导入凭证集的时间和凭证号范围是否合适
    int curYear = curSuite->year();
    int curMonth = curSuite->getSuiteRecord()->endMonth;
    int year = ui->edtDate->date().year();
    int month = ui->edtDate->date().month();
    if(curYear != year || curMonth != month){
        QMessageBox::warning(this,"",tr("导入凭证集的时间范围选择不合适！"));
        return;
    }
    int startPzNum = ui->spnStart->value();
    int endPzNum = ui->spnEnd->value();
    int curPzNum = curSuite->getPzCount();
    if((startPzNum >= endPzNum) || (startPzNum-curPzNum != 1)){
        QMessageBox::warning(this,"",tr("凭证号范围设置不正确，可能的原因如下：\n1、起始凭证号大于结束凭证号\n2、起始凭证号与当前凭证号不连续"));
        return;
    }

    //其次要判定所选月份的汇率设置是否一致
    if(!compareRate())
        return;

    if(!createMaps()){
        curSuite->rollback();
        QMessageBox::critical(this,"",tr("在创建新旧科目映射阶段发生错误！"));
        return ;
    }
    if(!importPzSet()){
        QMessageBox::critical(this,"",tr("在导入凭证阶段发生错误！"));
        return ;
    }
    QMessageBox::information(this,"",tr("成功导入凭证（%1#-%2#）").arg(startPzNum).arg(endPzNum));
    ui->btnImport->setEnabled(false);
}
