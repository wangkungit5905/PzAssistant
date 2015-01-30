#include "crtaccountfromolddlg.h"
#include "ui_crtaccountfromolddlg.h"
#include "myhelper.h"
#include "account.h"
#include "tables.h"
#include "global.h"
#include "dbutil.h"

#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>


CrtAccountFromOldDlg::CrtAccountFromOldDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CrtAccountFromOldDlg)
{
    ui->setupUi(this);
    appCfg = AppConfig::getInstance();
    conn_o = "OldAccount";
    conn_n = "NewAccount";
}

CrtAccountFromOldDlg::~CrtAccountFromOldDlg()
{
    delete ui;
    if(QSqlDatabase::contains(conn_o))
        QSqlDatabase::removeDatabase(conn_o);
    if(QSqlDatabase::contains(conn_n))
        QSqlDatabase::removeDatabase(conn_n);
}

void CrtAccountFromOldDlg::on_btnSelect_clicked()
{
    oldFile = QFileDialog::getOpenFileName(this,tr(""),".","Sqlite (*.dat)");
    if(oldFile.isEmpty())
        return;
    ui->fileName->setText(oldFile);
    db_o = QSqlDatabase::addDatabase("QSQLITE",conn_o);
    db_o.setDatabaseName(oldFile);
    if(!db_o.open()){
        myHelper::ShowMessageBoxError(tr("不能打开账户文件“%1”").arg(oldFile));
        return;
    }
    //1、读取账户的账户基本信息（名称、编码、起账日期和结账日期等）
    QSqlQuery q(db_o);
    QString s = "select code,value from AccountInfo";
    if(!q.exec(s)){
        myHelper::ShowMessageBoxError(tr("读取账户基本信息时发生错误！"));
        return;
    }
    int code;
    QString str,accSName,accLName,accCode,startDate,endDate;
    //假定老账户使用科目系统1，外币是美金
    while(q.next()){
        code = q.value(0).toInt();
        str = q.value(1).toString();
        switch(code){
        case 1:
            accCode = str;
            break;
        case 2:
            accSName = str;
            break;
        case 3:
            accLName = str;
            break;
        case 9:
            startDate = str;
            break;
        case 10:
            endDate = str;
            break;
        }
    }
    if(accSName.isEmpty()||accLName.isEmpty()||accCode.isEmpty()||
            startDate.isEmpty()||endDate.isEmpty()){
        myHelper::ShowMessageBoxError(tr("账户的基本信息缺失（账户简称、全称、代码、起止账日期等）"));
        return;
    }
    ui->code->setText(accCode);
    ui->sName->setText(accSName);
    ui->lName->setText(accLName);
    ui->sDate->setText(startDate);
    ui->eDate->setText(endDate);
    QDate ed = QDate::fromString(endDate,Qt::ISODate);
    if(ed.month() != 12){
        myHelper::ShowMessageBoxError(tr("账户的最后凭证集不是在12月！"));
        return;
    }
    year = ed.year();
    s = QString("select state from PZSetStates where year=%1 and month=%2")
            .arg(ed.year()).arg(ed.month());
    if(!q.exec(s) || !q.first()){
        myHelper::ShowMessageBoxError(tr("查询最后凭证集状态出错！"));
        return;
    }
    int pzSetState = q.value(0).toInt();
    if(pzSetState != 100){
        myHelper::ShowMessageBoxError(tr("最后凭证集未结账，不能继续！"));
        return;
    }
    db_o.close();
}

void CrtAccountFromOldDlg::on_btnCreate_clicked()
{
    QList<SubSysNameItem*> supportSubSys;
    appCfg->getSubSysItems(supportSubSys);
    SubSysNameItem* subSys;
    foreach(SubSysNameItem* item, supportSubSys){
        if(item->code == 2){
            subSys = item;
            break;
        }
    }
    QString errorInfos;
    QFileInfo fi(oldFile);
    newFile = DATABASE_PATH + fi.fileName();

    //1、创建账户框架
    ui->stateText->appendPlainText(tr("***********第一步：创建账户框架***********\n"));
    if(!Account::createNewAccount(fi.fileName(),ui->code->text(),ui->sName->text(),ui->lName->text(),subSys,
                                  2015,1,errorInfos)){
        QMessageBox::critical(this,"",tr("创建新账户失败！"));
        ui->stateText->setPlainText(errorInfos);
        return;
    }
    QApplication::processEvents();
    if(!db_o.open()){
        ui->stateText->appendPlainText(tr("不能打开老账户数据库连接！\n"));
        return;
    }
    //2、连接到新账户
    ui->stateText->appendPlainText(tr("***********第二步：连接到新账户***********\n"));    
    QApplication::processEvents();
    db_n = QSqlDatabase::addDatabase("QSQLITE",conn_n);
    db_n.setDatabaseName(newFile);
    if(!db_n.open()){
        ui->stateText->appendPlainText(tr("不能打开新账户数据库连接！\n"));
        return;
    }
    if(!db_n.transaction()){
        ui->stateText->appendPlainText(tr("启动事务失败！"));
        return;
    }
    bool r = removeDefCreated();
    if(r)
        r = transferNames();
    if(r)
        r = transferBanks();
    if(r)
        r = transferRates();
    if(r)
        r = transferSubjects();
    if(r)
        r = transferFSubExtras();
    if(r)
        r = transferSSubExtras();
    if(r)
        r = editTransferRecord();
    if(r){
        r = db_n.commit();
        if(!r)
            ui->stateText->appendPlainText(tr("在提交事务时失败！\n"));
    }
    if(!r){
        ui->stateText->appendPlainText(tr("创建失败！\n"));
        return;
    }
    ui->stateText->appendPlainText(tr("创建成功！\n"));
    db_n.close();
    QSqlDatabase::removeDatabase(conn_n);
    QSqlDatabase::removeDatabase(conn_o);

    //写入本地账户缓存
    ui->stateText->appendPlainText(tr("**********最后：写入本地账户缓存***********\n"));
    QApplication::processEvents();
    AccountCacheItem* item= new AccountCacheItem;
    item->id = UNID;
    item->code = ui->code->text();
    item->accName = ui->sName->text();
    item->accLName = ui->lName->text();
    item->fileName = fi.fileName();
    item->lastOpened = false;
    item->tState = ATS_TRANSINDES;
    item->inTime = QDateTime::currentDateTime();
    item->outTime = QDateTime::currentDateTime();
    WorkStation* locMac = AppConfig::getInstance()->getLocalStation();
    item->s_ws = locMac;
    item->d_ws = locMac;
    if(!AppConfig::getInstance()->saveAccountCacheItem(item))
        QMessageBox::critical(this,"",tr("将新账户保存到本地缓存时发生错误！"));
}

/**
 * @brief 移除默认导入的名称类别、名称、子目等
 * @return
 */
bool CrtAccountFromOldDlg::removeDefCreated()
{
    ui->stateText->appendPlainText(tr("***********第三步：移除默认导入的名称类别、名称、子目等***********\n"));
    QApplication::processEvents();
    QSqlQuery q(db_n);
    QString s = QString("delete from %1").arg(tbl_nameItemCls);
    if(!q.exec(s)){
        ui->stateText->appendPlainText(tr("清空名称类别时发生错误！\n"));
        return false;
    }
    s = QString("delete from %1").arg(tbl_nameItem);
    if(!q.exec(s)){
        ui->stateText->appendPlainText(tr("清空名称时发生错误！\n"));
        return false;
    }
    s = QString("delete from %1").arg(tbl_ssub);
    if(!q.exec(s)){
        ui->stateText->appendPlainText(tr("清空子目时发生错误！\n"));
        return false;
    }
    return true;
}

/**
 * @brief 读取老账户的名称类别、名称条目并导入到新账户（期间作名称条目的重复性检测并输出结果）
 * @return
 */
bool CrtAccountFromOldDlg::transferNames()
{
    ui->stateText->appendPlainText(tr("***********第四步：转移名称类别、名称条目***********\n"));
    QApplication::processEvents();
    QSqlQuery qo(db_o),qn(db_n);
    //名称类别
    QString s = "select * from SndSubClass";
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("从老账户中读取名称类别时发生错误！\n"));
        return false;
    }
    s = QString("insert into %1(%2,%3,%4) values(:cls,:name,:desc)")
            .arg(tbl_nameItemCls).arg(fld_nic_clscode).arg(fld_nic_name)
            .arg(fld_nic_explain);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    while(qo.next()){
        int code = qo.value(1).toInt();
        QString name = qo.value(2).toString();
        QString desc = qo.value(3).toString();
        qn.bindValue(":cls",code);
        qn.bindValue(":name",name);
        qn.bindValue(":desc",desc);
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("在转移账户的名称类别时发生错误！\n"));
            return false;
        }
    }
    //名称条目
    s = "select * from SecSubjects";
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("从老账户中读取名称条目时发生错误！\n"));
        return false;
    }
    s = QString("insert into %1(id,%2,%3,%4,%5,%6,%7) values(:id,:name,:lname,:remCode,:cls,:crtTime,1)")
            .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_lname)
            .arg(fld_ni_remcode).arg(fld_ni_class).arg(fld_ni_crtTime)
            .arg(fld_ni_creator);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    QStringList names, dulNames;
    int count = 0;
    while(qo.next()){
        int id = qo.value(0).toInt();
        QString name = qo.value(1).toString();
        if(names.contains(name))
            dulNames<<QString("%1(%2)").arg(name).arg(id);
        else
            names<<name;
        QString lname = qo.value(2).toString();
        if(lname.isEmpty())
            lname = name;
        QString remCode = qo.value(3).toString();
        int clsCode = qo.value(4).toInt();
        QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
        qn.bindValue(":id",id);
        qn.bindValue(":name",name);
        qn.bindValue(":lname",lname);
        qn.bindValue(":remCode",remCode);
        qn.bindValue(":cls",clsCode);
        qn.bindValue(":crtTime",time);
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("在转移账户的名称条目时发生错误！sql: %1\n").arg(qn.lastQuery()));
            return false;
        }
        count++;
    }
    QString info = tr("本次共转移 %1 条名称条目！").arg(count);
    if(!dulNames.isEmpty()){
        info.append(tr("但发现如下名称条目重复：\n"));
        foreach(QString n, dulNames)
            info.append(n).append("\n");
    }
    info.append("\n");
    ui->stateText->appendPlainText(info);
    return true;
}

/**
 * @brief 转移银行账户
 * @return
 */
bool CrtAccountFromOldDlg::transferBanks()
{
    ui->stateText->appendPlainText(tr("***********第五步：转移银行账户设置信息***********\n"));
    QApplication::processEvents();
    QSqlQuery qo(db_o),qo_2(db_o),qn(db_n);
    QString s = "select * from Banks";
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    s = QString("insert into %1(id,%2,%3,%4) values(:id,:isMain,:name,:lname)")
            .arg(tbl_bank).arg(fld_bank_isMain).arg(fld_bank_name)
            .arg(fld_bank_lname);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    QHash<int,QString> bankNames;
    while(qo.next()){
        int bankId = qo.value(0).toInt();
        bool isMain = qo.value(1).toBool();
        QString name = qo.value(2).toString();
        QString lname = qo.value(3).toString();
        qn.bindValue(":id",bankId);
        qn.bindValue(":isMain",isMain?1:0);
        qn.bindValue(":name",name);
        qn.bindValue(":lname",lname);
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(qn.lastQuery()));
            return false;
        }
        bankNames[bankId] = name;
    }
    s = "select * from BankAccounts order by bankID,mtID";
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    s = QString("insert into %1(%2,%3,%4,%5) values(:bankId,:mt,:accNum,:nid)")
            .arg(tbl_bankAcc).arg(fld_bankAcc_bankId).arg(fld_bankAcc_mt)
            .arg(fld_bankAcc_accNum).arg(fld_bankAcc_nameId);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    s = "select code,name from MoneyTypes";
    if(!qo_2.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    QHash<int,QString> mtNames;
    while(qo_2.next()){
        int mt = qo_2.value(0).toInt();
        QString name = qo_2.value(1).toString();
        mtNames[mt] = name;
    }
    s = "select id from SecSubjects where subName = :name";
    if(!qo_2.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    while(qo.next()){
        int bankId = qo.value(1).toInt();
        int mt = qo.value(2).toInt();
        QString accNumer = qo.value(3).toString();
        QString subName = QString("%1-%2").arg(bankNames.value(bankId)).arg(mtNames.value(mt));
        qo_2.bindValue(":name",subName);
        if(!qo_2.exec()){
            ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(qo_2.lastQuery()));
            return false;
        }
        if(!qo_2.first()){
            ui->stateText->appendPlainText(tr("未能找到与银行“%1”的“%2”相关联的名称条目！")
                                           .arg(bankNames.value(bankId)).arg(mtNames.value(mt)));
            return false;
        }
        int nid = qo_2.value(0).toInt();
        qn.bindValue(":bankId",bankId);
        qn.bindValue(":mt",mt);
        qn.bindValue(":accNum",accNumer);
        qn.bindValue(":nid",nid);
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(qn.lastQuery()));
            return false;
        }
    }
    return true;
}

/**
 * @brief 转移汇率
 * @return
 */
bool CrtAccountFromOldDlg::transferRates()
{
    ui->stateText->appendPlainText(tr("***********第六步：转移汇率设置信息***********\n"));
    QApplication::processEvents();
    QSqlQuery qo(db_o),qn(db_n);
    QString s = "select usd2rmb from ExchangeRates where year=:y and month=:m";
    if(!qo.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行sql语句：%1\n").arg(s));
        return false;
    }
    s = QString("insert into %1(%2,%3,usd2rmb) values(:y,:m,:rate)").arg(tbl_rateTable)
            .arg(fld_rt_year).arg(fld_rt_month);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行sql语句：%1\n").arg(s));
        return false;
    }
    QList<int> years; years<<2014<<2015;
    foreach(int y,years){
        qo.bindValue(":y",y);
        int m;
        if(y == 2014)
            m = 12;
        else
            m = 1;
        qo.bindValue(":m",m);
        if(!qo.exec() || !qo.first()){
            ui->stateText->appendPlainText(tr("未能找到%1年%2月的汇率！\n").arg(y).arg(m));
            return false;
        }
        Double v = Double(qo.value(0).toDouble(),4);
        qn.bindValue(":y",y);
        qn.bindValue(":m",m);
        qn.bindValue(":rate",v.toString2());
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("在保存汇率时出错！\n"));
            return false;
        }
    }
    return true;
}

/**
 * @brief 转移科目
 * @return
 */
bool CrtAccountFromOldDlg::transferSubjects()
{
    ui->stateText->appendPlainText(tr("***********第七步：转移科目***********\n"));
    QApplication::processEvents();
    QSqlQuery qo(db_o),qn(db_n);
    //（1）、建立科目一级科目id映射表（键：老主目id，值：新主目id）
    QList<SubSysJoinItem2*> subMapLst;
    appCfg->getSubSysMaps(1,2,subMapLst);
    QString s = "select id,subCode from FirSubjects";
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    while(qo.next()){
        int id = qo.value(0).toInt();
        QString code = qo.value(1).toString();
        fidMaps_o[code] = id;
    }
    s = QString("select id,%1 from %2%3 where %4!=0 order by %1").arg(fld_fsub_subcode)
            .arg(tbl_fsub_prefix).arg(2).arg(fld_fsub_fid);
    if(!qn.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    while(qn.next()){
        int id = qn.value(0).toInt();
        QString code = qn.value(1).toString();
        fidMaps_n[code] = id;
    }
    foreach(SubSysJoinItem2* item, subMapLst){
        int oid = fidMaps_o.value(item->scode);
        int nid = fidMaps_n.value(item->dcode);
        if(oid == 0 || nid ==0){
            ui->stateText->appendPlainText(tr("科目映射有误！老科目（code=%1，id=%2），新科目（code=%3，id=%4）")
                                           .arg(item->scode).arg(oid).arg(item->dcode).arg(nid));
            return false;
        }
        fsubMaps[oid] = nid;
    }

    //（2）、按老主目的顺序，逐一读取其下子目，并根据映射表，最终将该主目的所有子目导入到新子目表
    //如果是混合对接科目，则必须将其子目的映射情况记录到子目映射表中，以便后来合并余额时使用
    //（2）确定混合对接主目
    QHash<QString,QString> mixedSubCodes;
    appCfg->getNotDefSubSysMaps(1,2,mixedSubCodes);
    QHashIterator<QString,QString> it(mixedSubCodes);
    while(it.hasNext()){
        it.next();
        int oid = fidMaps_o.value(it.key());
        int nid = fidMaps_n.value(it.value());
        if(oid == 0 || nid == 0){
            ui->stateText->appendPlainText(tr("混合对接科目映射有误！老科目（code=%1，id=%2），新科目（code=%3，id=%4）")
                                           .arg(it.key()).arg(oid).arg(it.value()).arg(nid));
            return false;
        }
        mixedSubFids[oid] = nid;
    }
    //（3）转移子目
    s = "select id,fid,sid from FSAgent order by fid,sid";
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    s = QString("insert into %1(id,%2,%3,%4,%5,%6,%7,%8) values(:id,:fid,:nid,1,1,:time,1,2)")
            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_weight)
            .arg(fld_ssub_enable).arg(fld_ssub_crtTime).arg(fld_ssub_creator).arg(fld_ssub_subsys);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    while(qo.next()){
        int id = qo.value(0).toInt();
        int sfid = qo.value(1).toInt();
        int dfid = fsubMaps.value(sfid);
        if(dfid == 0){
            bool isUsed = false; QString info;
            if(!subUsedInExtra(sfid,isUsed,info))
                return false;
            if(!isUsed){
                ui->stateText->appendPlainText(info);
                continue;
            }
            ui->stateText->appendPlainText(tr("在转移子目时发现一个子目的所属主目没有对应的新主目，（源主目id=%1，源子目id=%2\n")
                                           .arg(sfid).arg(id));
            return false;
        }
        int nid = qo.value(2).toInt();
        qn.bindValue(":id",id);
        qn.bindValue(":fid",dfid);
        qn.bindValue(":nid",nid);
        qn.bindValue(":time",QDateTime::currentDateTime().toString(Qt::ISODate));
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("在转移子目时发生错误！"));
            return false;
        }
    }
    return true;
}

/**
 * @brief 转移主目期末余额
 * @return
 */
bool CrtAccountFromOldDlg::transferFSubExtras()
{
    ui->stateText->appendPlainText(tr("***********第八步：转移主目期末余额***********\n"));
    QApplication::processEvents();
    QSqlQuery qo(db_o),qn(db_n),qn_2(db_n),qn_3(db_n);

    //在新账户中建立期初余额指针
    QString s = QString("insert into %1(%2,%3,%4,%5) values(%6,12,1,%7)").arg(tbl_nse_point)
            .arg(fld_nse_year).arg(fld_nse_month).arg(fld_nse_mt).arg(fld_nse_state)
            .arg(year).arg(Ps_Jzed);
    if(!qn.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    s = "select last_insert_rowid()";
    if(!qn.exec(s) || !qn.first()){
        ui->stateText->appendPlainText(tr("无法在新账户中获取期初余额指针值！"));
        return false;
    }
    pRMB_n = qn.value(0).toInt();
    s = QString("insert into %1(%2,%3,%4) values(%5,12,2)").arg(tbl_nse_point)
            .arg(fld_nse_year).arg(fld_nse_month).arg(fld_nse_mt).arg(year);
    if(!qn.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    s = "select last_insert_rowid()";
    if(!qn.exec(s) || !qn.first()){
        ui->stateText->appendPlainText(tr("无法在新账户中获取期初余额指针值！"));
        return false;
    }
    pUSD_n = qn.value(0).toInt();

    //建立插入到新账户的人民币余额语句
    s = QString("insert into %1(%2,%3,%4,%5) values(%6,:sid,:value,:dir)")
            .arg(tbl_nse_p_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value)
            .arg(fld_nse_dir).arg(pRMB_n);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    //建立插入到新账户的美金余额语句
    s = QString("insert into %1(%2,%3,%4,%5) values(%6,:sid,:value,:dir)")
            .arg(tbl_nse_p_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value)
            .arg(fld_nse_dir).arg(pUSD_n);
    if(!qn_2.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    //建立插入到新账户的美金本币余额语句
    s = QString("insert into %1(%2,%3,%4) values(:pid,:sid,:value)")
            .arg(tbl_nse_m_f).arg(fld_nse_pid).arg(fld_nse_sid).arg(fld_nse_value);

    if(!qn_3.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }

    //从老账户中读取人民币期末余额方向
    s = QString("select * from SubjectExtraDirs where year = %1 and "
                "month = 12 and mt = 1").arg(year);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    if(!qo.first()){
        ui->stateText->appendPlainText(tr("无法获取最后月份的人民币余额方向记录！\n"));
        return false;
    }
    QSqlRecord rec = qo.record();
    for(int i = 4; i < rec.count(); ++i){
        QSqlField field = rec.field(i);
        if(field.isNull())
            continue;
        QString filedName = field.name();
        QString subCode = filedName.right(4);
        int fid_o = fidMaps_o.value(subCode);
        if(fid_o == 0){
            ui->stateText->appendPlainText(tr("在读取期末余额方向时遇到一个未配置对接科目的老科目（科目代码 = %1）").arg(subCode));
            return false;
        }
        fsubDirsRMB[fid_o] = (MoneyDirection)(qo.value(i).toInt());
    }
    //从老账户中读取美金期末余额方向
    s = QString("select * from SubjectExtraDirs where year = %1 and "
                "month = 12 and mt = 2").arg(year);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    if(!qo.first()){
        ui->stateText->appendPlainText(tr("无法获取最后月份的美金余额方向记录！\n"));
        return false;
    }
    rec = qo.record();
    for(int i = 4; i < rec.count(); ++i){
        QSqlField field = rec.field(i);
        if(field.isNull())
            continue;
        QString filedName = rec.field(i).name();
        QString subCode = filedName.right(4);
        int fid_o = fidMaps_o.value(subCode);
        if(fid_o == 0){
            ui->stateText->appendPlainText(tr("在读取期末余额方向时遇到一个未配置对接科目的老科目（科目代码 = %1）").arg(subCode));
            return false;
        }
        fsubDirsUSD[fid_o] = (MoneyDirection)(qo.value(i).toInt());
    }

    //读取人民币余额并保存
    s = QString("select * from SubjectExtras where year = %1 and "
                    "month = 12 and mt = 1").arg(year);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    if(!qo.first()){
        ui->stateText->appendPlainText(tr("无法获取最后月份的人民币余额记录！\n"));
        return false;
    }
    pRMB_o = qo.value(0).toInt(); //保存人民币余额的记录指针
    rec = qo.record();
    for(int i = 5; i < rec.count(); ++i){
        QSqlField field = rec.field(i);
        if(field.isNull())
            continue;
        QString filedName = rec.field(i).name();
        QString subCode = filedName.right(4);
        int fid_o = fidMaps_o.value(subCode);
        if(fid_o == 0){
            ui->stateText->appendPlainText(tr("在读取期末余额时遇到一个未配置对接科目的老科目（科目代码 = %1）").arg(subCode));
            return false;
        }
        Double v = Double(qo.value(i).toDouble());
        fsubExtrasRMB[fid_o] = v;
        //qn.bindValue(":pid",pRMB_n);
        qn.bindValue(":sid",fsubMaps.value(fid_o));
        qn.bindValue(":value",v.toString2());
        qn.bindValue(":dir",fsubDirsRMB.value(fid_o));
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("在保存期初原币余额时发生错误！ sql：%1").arg(qn.lastQuery()));
            return false;
        }
    }
    //读取美金余额并保存
    s = QString("select * from SubjectExtras where year = %1 and "
                    "month = 12 and mt = 2").arg(year);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    if(!qo.first()){
        ui->stateText->appendPlainText(tr("无法获取最后月份的美金原币余额记录！\n"));
        return false;
    }
    pUSD_o = qo.value(0).toInt(); //保存人民币余额的记录指针
    rec = qo.record();
    for(int i = 5; i < rec.count(); ++i){
        QSqlField field = rec.field(i);
        if(field.isNull())
            continue;
        QString filedName = rec.field(i).name();
        QString subCode = filedName.right(4);
        int fid_o = fidMaps_o.value(subCode);
        if(fid_o == 0){
            ui->stateText->appendPlainText(tr("在读取期末余额时遇到一个未配置对接科目的老科目（科目代码 = %1）").arg(subCode));
            return false;
        }
        Double v = Double(qo.value(i).toDouble());
        fsubExtrasUSD[fid_o] = v;
        //qn_2.bindValue(":pid",pUSD_n);
        qn_2.bindValue(":sid",fsubMaps.value(fid_o));
        qn_2.bindValue(":value",v.toString2());
        qn_2.bindValue(":dir",fsubDirsUSD.value(fid_o));
        if(!qn_2.exec()){
            ui->stateText->appendPlainText(tr("在保存期初原币余额时发生错误！ sql：%1").arg(qn.lastQuery()));
            return false;
        }
    }

    //读取期末本币余额
    s = QString("select * from SubjectMmtExtras where year = %1 and "
                "month = 12 and mt = 2").arg(year);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1！\n").arg(s));
        return false;
    }
    if(!qo.first()){
        ui->stateText->appendPlainText(tr("无法获取最后月份的本币余额记录！\n"));
        return false;
    }
    rec = qo.record();
    for(int i = 4; i < rec.count(); ++i){
        QString filedName = rec.field(i).name();
        QString subCode = filedName.right(4);
        int fid_o = fidMaps_o.value(subCode);
        Double v = Double(qo.value(i).toDouble());
        fsubMExtras[fid_o] = v;
        qn_3.bindValue(":pid",pUSD_n);
        qn_3.bindValue(":sid",fsubMaps.value(fid_o));
        qn_3.bindValue(":value",v.toString2());
        if(!qn_3.exec()){
            ui->stateText->appendPlainText(tr("在保存期初本币余额时发生错误！ sql：%1\n").arg(qn_2.lastQuery()));
            return false;
        }
    }
    return true;
}

/**
 * @brief 转移子目期末余额
 * @return
 */
bool CrtAccountFromOldDlg::transferSSubExtras()
{
    ui->stateText->appendPlainText(tr("***********第九步：转移子目期末余额***********\n"));
    QApplication::processEvents();
    QSqlQuery qo(db_o),qn(db_n);

    //建立保存子目原币余额及其方向的sql语句
    QString s = QString("insert into %1(%2,%3,%4,%5) values(:pid,:sid,:value,:dir)")
            .arg(tbl_nse_p_s).arg(fld_nse_pid).arg(fld_nse_sid)
            .arg(fld_nse_value).arg(fld_nse_dir);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行sql语句，sql：%1\n").arg(s));
        return false;
    }

    //读取并保存子目的人民币余额及其方向
    s = QString("select fsid,dir,value from detailExtras where seid=%1").arg(pRMB_o);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行sql语句，sql：%1\n").arg(s));
        return false;
    }
    while(qo.next()){
        int sid = qo.value(0).toInt();
        int dir = qo.value(1).toInt();
        Double v = Double(qo.value(2).toDouble());
        qn.bindValue(":pid",pRMB_n);
        qn.bindValue(":sid",sid);
        qn.bindValue(":value",v.toString2());
        qn.bindValue(":dir",dir);
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("在保存人民币期初余额时发生错误，sql：%1\n"));
            return false;
        }
    }
    //读取并保存子目的美金余额及其方向
    s = QString("select fsid,dir,value from detailExtras where seid=%1").arg(pUSD_o);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行sql语句，sql：%1\n").arg(s));
        return false;
    }
    while(qo.next()){
        int sid = qo.value(0).toInt();
        int dir = qo.value(1).toInt();
        Double v = Double(qo.value(2).toDouble());
        qn.bindValue(":pid",pUSD_n);
        qn.bindValue(":sid",sid);
        qn.bindValue(":value",v.toString2());
        qn.bindValue(":dir",dir);
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("在保存美金期初余额时发生错误，sql：%1\n"));
            return false;
        }
    }
    //建立保存子目美金本币余额的sql语句
    s = QString("insert into %1(%2,%3,%4) values(%6,:sid,:value)")
            .arg(tbl_nse_m_s).arg(fld_nse_pid).arg(fld_nse_sid)
            .arg(fld_nse_value).arg(pUSD_n);
    if(!qn.prepare(s)){
        ui->stateText->appendPlainText(tr("错误执行sql语句，sql：%1\n").arg(s));
        return false;
    }
    //读取并保存子目的美金本币余额
    s = QString("select id from SubjectMmtExtras where year=%1 and month=12").arg(year);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("在读取美金本币余额指针时出错，sql：%1\n").arg(s));
        return false;
    }
    if(!qo.first()){
        ui->stateText->appendPlainText(tr("未发现美金本币余额指针\n"));
        return false;
    }
    int p = qo.value(0).toInt();
    s = QString("select fsid,value from detailMmtExtras where seid=%1").arg(p);
    if(!qo.exec(s)){
        ui->stateText->appendPlainText(tr("在读取美金本币余额时出错，sql：%1\n").arg(s));
        return false;
    }
    while(qo.next()){
        int sid = qo.value(0).toInt();
        Double v = Double(qo.value(1).toDouble());
        qn.bindValue(":sid",sid);
        qn.bindValue(":value",v.toString2());
        if(!qn.exec()){
            ui->stateText->appendPlainText(tr("在保存美金本币余额时出错！sql：%1\n").arg(qn.lastQuery()));
            return false;
        }
    }
    return true;
}

/**
 * @brief 修改转移记录的描述
 * @return
 */
bool CrtAccountFromOldDlg::editTransferRecord()
{
    ui->stateText->appendPlainText(tr("***********第十步：初始化账户转移记录***********\n"));
    QApplication::processEvents();
    QString info = tr("由工作站“%1”从老账户格式转移而来").arg(appCfg->getLocalStation()->name());
    QSqlQuery q(db_n);
    QString s = QString("update %1 set %2='%4',%3='%4' where %5=1").arg(tbl_transferDesc)
            .arg(fld_transDesc_in).arg(fld_transDesc_out).arg(info).arg(fld_transDesc_tid);
    if(!q.exec(s)){
        ui->stateText->appendPlainText(tr("在修改转移记录时发生错误！\n"));
        return false;
    }
    return true;
}

/**
 * @brief 余额一致性检测
 * @return
 */
bool CrtAccountFromOldDlg::extraUnityInspect()
{
    DbUtil du;
    du.setFilename(newFile);
}

/**
 * @brief 指定主目（老）是否在最后的余额记录中被引用
 * @param fid
 * @param isUsed
 * @param info
 * @return
 */
bool CrtAccountFromOldDlg::subUsedInExtra(int fid,bool &isUsed, QString &info)
{
    QString subCode;
    QHashIterator<QString,int> it(fidMaps_o);
    while(it.hasNext()){
        it.next();
        if(it.value() == fid){
            subCode = it.key();
            break;
        }
    }
    if(subCode.isEmpty()){
        ui->stateText->appendPlainText(tr("无法找到id为“%1”的老主目！\n").arg(fid));
        return false;
    }
    char sign = 'A'+subCode.left(1).toInt() - 1;
    QString fieldName = QString("%1%2").arg(sign).arg(subCode);
    QSqlQuery q(db_o);
    QString s = QString("select %1 from SubjectExtras where year = %2 and month = 12 and mt = 1")
            .arg(fieldName).arg(year);
    if(!q.exec(s)){
        ui->stateText->appendPlainText(tr("错误执行Sql语句：%1\n").arg(s));
        return false;
    }
    if(!q.first()){
        isUsed = false;
        return true;
    }
    isUsed = !q.record().field(0).isNull();
    if(!isUsed)
        info = tr("科目（%1）未配置对接，但在最后的余额记录中未被引用，可以不用转移到新账户！\n").arg(subCode);
    return true;
}
