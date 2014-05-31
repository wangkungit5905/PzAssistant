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
    if(curSuite->getSubSysCode() == DEFAULT_SUBSYS_CODE){
        QMessageBox::warning(this,"",tr("当前账户的最后帐套所采用的科目系统未升级到新科目系统，不能导入！"));
        return false;
    }
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
 * 只有在源帐套和目的帐套所采用的科目系统不同时，才需要将科目进行转换
 * 科目的转换涉及到主目和子目，且只有那些混合对接科目才需要考虑转换
 * @return
 */
bool ImportOVAccDlg::createMaps()
{
    //1、从基本库中读取新旧科目系统之间的混合对接项
    sm = curSuite->getSubjectManager();
    if(sm->getSubSysCode() != DEFAULT_SUBSYS_CODE){
        QList<MixedJoinCfg*> cfgInfos;
        if(!account->getDbUtil()->getMixJoinInfo(DEFAULT_SUBSYS_CODE,sm->getSubSysCode(),cfgInfos)){
            LOG_ERROR(tr("无法获取新旧科目混合对接表信息！"));
            return false;
        }
        //2、读取新旧科目系统混合对接配置信息
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
            fsubIdMaps[item->s_fsubId] = fsub;
            ssubIdMaps[item->s_ssubId] = ssub;
        }
    }


    //3、补齐在源账户内新建但在目的账户内不存在的二级科目和名称条目
    //即查询在导入源账户内存在的二级科目和它所采用的名称条目是否存在，如果不存在，则创建，
    //还要考虑其所属的一级科目是否是混合对接科目。如果是，则还要在当前账户的二级科目混合对接表中新增该配置

    QString dateStr = AppConfig::getInstance()->getSpecSubSysItem(sm->getSubSysCode())->startTime.toString(Qt::ISODate);
    QDate endDate = QDate::fromString(dateStr,Qt::ISODate);//默认科目系统的截止日期
    endDate.setDate(endDate.year()-1,12,31);
    QString s = QString("select FSAgent.id,FSAgent.fid,SecSubjects.subName,SecSubjects.subLName,"
                "SecSubjects.remCode,SecSubjects.classId from FSAgent join SecSubjects on "
                "FSAgent.sid = SecSubjects.id order by FSAgent.fid");
    QSqlQuery q(sdb);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    while(q.next()){
        int sid = q.value(0).toInt();
        int fid = q.value(1).toInt();
        FirstSubject* fsub;
        if(fsubIdMaps.contains(fid))
            fsub = fsubIdMaps.value(fid);
        else
            fsub = sm->getFstSubject(fid);
        if(!fsub){
            LOG_ERROR(tr("遇到幽灵主目（fid=%1）").arg(fid));
            return false;
        }
        QString name = q.value(2).toString();
        SubjectNameItem* ni = sm->getNameItem(name);
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
        SecondSubject* ssub = fsub->getChildSub(ni);
        if(!ssub){
            //创建二级科目
            QDateTime crtTime = QDateTime::currentDateTime();
            if(sm->getSubSysCode() == DEFAULT_SUBSYS_CODE)
                crtTime = QDateTime(endDate);
            CrtSndSubUseNICmd* cmd = new CrtSndSubUseNICmd(sm,fsub,ni,1,crtTime,curUser);
            curSuite->getUndoStack()->push(cmd);
            LOG_INFO(tr("创建了二级科目：%1--%2").arg(fsub->getName()).arg(ni->getShortName()));
            ssub = fsub->getChildSub(ni);
        }
        //如果新建的二级科目属于混合对接科目，则必须记录在案。当导入成功后，将这些科目保存后得到的id信息写入到二级科目混合对接表中
        //还有一种情形是源和目的账户存在同名的混合对接二级科目，但在配置项中不存在，则必须添加。
        if(fsubIdMaps.contains(fid) && (ssub->getId() == 0 || !isExistCfg(fid,sid))){
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
 * @brief 从源账户数据库中导入指定月份内的指定区间的凭证
 * @return
 */
bool ImportOVAccDlg::importPzSet()
{
    QSqlQuery q(sdb),q2(sdb);
    QHash<int,Money*> moneys = curAccount->getAllMoneys();
    QString ds = ui->edtDate->date().toString(Qt::ISODate);
    ds.chop(3);
    QString s = QString("select * from PingZhengs where date like '%1%'").arg(ds);
    int startPzNum = ui->spnStart->value();
    int endPzNum = ui->spnEnd->value();
    if(startPzNum == endPzNum)
        s.append(QString(" and number=%1").arg(startPzNum));
    else
        s.append(QString(" and number>=%1 and number<=%2").arg(startPzNum).arg(endPzNum));
    s.append(" order by number");
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    bool isTran = curSuite->getSubjectManager()->getSubSysCode() != DEFAULT_SUBSYS_CODE;
    ui->progress->setMaximum(endPzNum - startPzNum + 1);
    ui->progress->setMinimum(0);
    while(q.next()){
        int pzId = q.value(0).toInt();
        QString date = q.value(1).toString();
        int pzNum = q.value(2).toInt();
        ui->progress->setValue(pzNum);
        QApplication::processEvents();
        int zbNum = q.value(3).toInt();
        Double jsum = Double(q.value(4).toDouble());
        Double dsum = Double(q.value(5).toDouble());
        PzClass pzCls = (PzClass)q.value(6).toInt();
        int encNum = q.value(7).toInt();
        PzState pzState = (PzState)q.value(8).toInt();
        int vuId = q.value(9).toInt();
        int ruId = q.value(10).toInt();
        int buId = q.value(11).toInt();
        PingZheng* pz = new PingZheng(curSuite,0,date,pzNum,zbNum,jsum,dsum,pzCls,encNum,pzState,allUsers.value(vuId),allUsers.value(ruId),allUsers.value(buId));
        s = QString("select * from BusiActions where pid=%1 order by NumInPz").arg(pzId);
        if(!q2.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
        int baNum = 0;
        while(q2.next()){
            baNum++;
            QString summary = q2.value(2).toString();
            int fid = q2.value(3).toInt();
            int sid = q2.value(4).toInt();
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
            //这里要检测是否存在科目？
            FirstSubject* fsub=NULL;
            SecondSubject* ssub=NULL;
            if(isTran){
                //导入到新科目系统，对于混合对接科目要进行科目转换，而对于二级科目如果有转换项存在，则使用转换项
                if(fsubIdMaps.contains(fid))
                    fsub = fsubIdMaps.value(fid);
                else
                    fsub = sm->getFstSubject(fid);
                if(ssubIdMaps.contains(sid))
                    ssub = ssubIdMaps.value(sid);
                else
                    ssub = sm->getSndSubject(sid);
            }
            else{
                //导入到旧科目系统，则一级科目一般是一一对应的，但二级科目有可能不是一一对应的
                fsub = sm->getFstSubject(fid);
                if(ssubIdMaps.contains(sid))
                    ssub = ssubIdMaps.value(sid);
                else
                    ssub = sm->getSndSubject(sid);
            }
            if(!fsub || !ssub){
                LOG_ERROR(tr("导入凭证期间，在%1#凭证的第%2条分录发现一个幽灵科目（fid=%3，sid=%4）")
                          .arg(pzNum).arg(baNum).arg(fid).arg(sid));
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
        }
        if(!account->getDbUtil()->appendMixJoinInfo(DEFAULT_SUBSYS_CODE,sm->getSubSysCode(),cfgItems))
            QMessageBox::critical(this,tr("保存出错"),tr("导入的凭证成功保存，但在保存混合对接科目信息时出错！"));
    }
    curSuite->closePzSet();
    q.finish();q2.finish();
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
