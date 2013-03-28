//#include <QSqlQuery>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextCodec>
#include <QSqlRecord>
#include <QSqlField>
#include <QMenu>
#include <QSqlRelationalDelegate>
#include <QMouseEvent>

//#ifdef Q_OS_WIN32
//#include <qaxobject.h>
//#endif




#include "dialogs.h"
#include "config.h"

#include "utils.h"
#include "global.h"
#include "connection.h"
#include "account.h"
#include "tables.h"
//#include "ExcelFormat.h"

//using namespace ExcelFormat;

//#define	FW_NORMAL	400
//#define	FW_BOLD		700



/////////////////////////////////////////////////////////////////////////////////

OpenAccountDialog::OpenAccountDialog(QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
    //从应用程序的配置信息中读取已导入的账户
    QStringList accountLst;
    AppConfig::getInstance()->readAccountInfos(accInfoLst);
    for(int i = 0; i < accInfoLst.count(); ++i)
        accountLst << accInfoLst[i]->sname;
    model = new QStringListModel(accountLst);
    ui.accountList->setModel(model);
    connect(ui.accountList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(itemClicked(QModelIndex)));
    connect(ui.accountList, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(doubleClicked(QModelIndex)));
}

QString OpenAccountDialog::getSName()
{
    return accInfoLst[selAcc]->sname;
}

QString OpenAccountDialog::getLName()
{
    return accInfoLst[selAcc]->lname;
}

QString OpenAccountDialog::getFileName()
{
    return accInfoLst[selAcc]->fname;
}

void OpenAccountDialog::itemClicked(const QModelIndex &index)
{
    selAcc = index.row();
    ui.lblFName->setText(accInfoLst[selAcc]->fname);
    ui.lblCode->setText(accInfoLst[selAcc]->code);
    ui.lname->setText(accInfoLst[selAcc]->lname);
    //ui.lblLastTime->setText(accInfoLst[selAcc]->lastTime);
    //ui.lblDesc->setText(accInfoLst[selAcc]->desc);
    //ui.buttonBox->button(QDialogButtonBox::Open)->setEnabled(true);
}

void OpenAccountDialog::doubleClicked(const QModelIndex &index)
{
    itemClicked(index);
    accept();
}

//获取打开的账户的序号
int OpenAccountDialog::getAccountId()
{
    return accInfoLst[selAcc]->id;
}

//获取打开的账户所采用的科目系统
//int OpenAccountDialog::getUsedSubSys()
//{
//    return accInfoLst[selAcc]->usedSubSys;
//}


////////////////////////////////////////////////////////////////////////////
OpenPzDialog::OpenPzDialog(Account *account, QWidget* parent) :
    account(account),QDialog(parent)
{
    ui.setupUi(this);

    ui.title->setText(QString("<H6><font color=green>%1</H6>")
                      .arg(account->getLName()));

    foreach(int y,account->getSuites())
        ui.cmbSuites->addItem(account->getSuiteName(y),y);
    if(account->getCurSuite()){
        y = account->getCurSuite()->year;
        int idx = ui.cmbSuites->findData(y);
        ui.cmbSuites->setCurrentIndex(idx);
        int sm,em;
        if(!account->getSuiteMonthRange(y,sm,em)){
            ui.btnOk->setEnabled(false);
            return;
        }
        ui.spnMonth->setMinimum(sm);
        ui.spnMonth->setMaximum(em);
        ui.spnMonth->setValue(em);
        m = em;
    }
    connect(ui.cmbSuites,SIGNAL(currentIndexChanged(int)),
               this,SLOT(suiteChanged(int)));
    connect(ui.spnMonth,SIGNAL(valueChanged(int)),this,SLOT(monthChanged(int)));


}

QDate OpenPzDialog::getDate()
{
    return QDate(y,m,1);
}


//
void OpenPzDialog::suiteChanged(int index)
{
    y = ui.cmbSuites->itemData(index).toInt();
    int sm,em;
    if(!account->getSuiteMonthRange(y,sm,em)){
        ui.btnOk->setEnabled(false);
        return;
    }
    ui.spnMonth->setMinimum(sm);
    ui.spnMonth->setMaximum(em);
    ui.spnMonth->setValue(em);
    ui.btnOk->setEnabled(true);
}

void OpenPzDialog::monthChanged(int month)
{
    m = month;
}

//创建新的凭证集
void OpenPzDialog::on_chkNew_clicked(bool checked)
{
    disconnect(ui.cmbSuites,SIGNAL(currentIndexChanged(int)),
               this,SLOT(suiteChanged(int)));
    disconnect(ui.spnMonth,SIGNAL(valueChanged(int)),this,SLOT(monthChanged(int)));

    int month = account->getEndTime().month();
    int year = account->getEndTime().year();
    if(checked){
        //如果账户的最后记账日期是12月，则还必须创建新的帐套
        ui.chkRate->setEnabled(true);
        ui.chkRate->setChecked(true);
        if(month == 12){
            year++;
            month = 1;
            ui.spnMonth->setMinimum(1);
            ui.spnMonth->setMaximum(1);
            ui.spnMonth->setValue(1);
        }
        else{
            month++;
            ui.spnMonth->setMinimum(month);
            ui.spnMonth->setMaximum(month);
            ui.spnMonth->setValue(month);
        }
        ui.cmbSuites->clear();
        ui.cmbSuites->addItem(tr("%1年").arg(year),year);
    }
    else{
        ui.chkRate->setEnabled(false);
        ui.chkRate->setChecked(false);
        ui.cmbSuites->clear();
        foreach(int y, account->getSuites())
            ui.cmbSuites->addItem(account->getSuiteName(y),y);
        year = account->getCurSuite()->year;
        ui.cmbSuites->setCurrentIndex(ui.cmbSuites->findData(year));
        int sm,em;
        account->getSuiteMonthRange(year,sm,em);
        ui.spnMonth->setMinimum(sm);
        ui.spnMonth->setMaximum(em);
        ui.spnMonth->setValue(em);
        month = em;
    }
    y = year;
    m = month;

    connect(ui.cmbSuites,SIGNAL(currentIndexChanged(int)),
               this,SLOT(suiteChanged(int)));
    connect(ui.spnMonth,SIGNAL(valueChanged(int)),this,SLOT(monthChanged(int)));
}

void OpenPzDialog::on_btnOk_clicked()
{
    if(ui.chkNew->isChecked()){
        if(!account->containSuite(y))
            account->appendSuite(y,ui.cmbSuites->currentText());
        QDate endDate = QDate(y,m,1);
        endDate.setYMD(y,m,endDate.daysInMonth());
        account->setEndTime(endDate);
    }
    if(ui.chkRate->isChecked()){
        //int year = account->getEndTime().year();
        //int month = account->getEndTime().month();
        QHash<int,Double> rates;
        if(m == 1)
            BusiUtil::getRates2(y-1,12,rates);
        else
            BusiUtil::getRates2(y,m-1,rates);
        BusiUtil::saveRates2(y,m,rates);
    }
    account->setCurSuite(y);
    accept();
}



////////////////////////////////////////////////////////////////////////

bool PZCollect::operator<( const PZCollect &other ) const
{
    return id < other.id;
}

//    QString name;    //科目名称
//    double jsum;     //借方合计值
//    double dsum;     //贷方合计值


//void PZCollect::setName(QString name)
//{
//    this->name = name;
//}

//void PZCollect::addJ(double value)
//{
//    jsum += value;
//}

//void PZCollect::addD(double value)
//{
//    dsum += value;
//}

//QString PZCollect::getName()
//{
//    return name;
//}

//double PZCollect::getJ()
//{
//    return jsum;
//}

//double PZCollect::getD()
//{
//    return dsum;
//}

//void PzCollectProcess::addJ(int id, double v)
//{
//    int size = plist.size();
//    if(size == 0){
//        PZCollect pc;
//        pc.id = id;
//        pc.dsum = 0;
//        pc.jsum = v;
//        pc.name = "";
//        plist.append(pc);
//    }
//    else{
//        int i = 0;
//        int fid = plist[0].id;
//        while((fid != id) && (i < size)){  //顺序查找对应的一级科目id
//            fid = plist[i].id;
//            i++;
//        }
//        if(i < size){ //找到了
//            plist[i].jsum += v;
//        }
//        else{ //没有找到
//            PZCollect pc;
//            pc.id = id;
//            pc.dsum = 0;
//            pc.jsum = v;
//            pc.name = "";
//            plist.append(pc);
//        }
//    }
//}

//void PzCollectProcess::addD(int id, double v)
//{
//    int size = plist.size();
//    if(size == 0){
//        PZCollect pc;
//        pc.id = id;
//        pc.dsum = v;
//        pc.jsum = 0;
//        pc.name = "";
//        plist.append(pc);
//    }
//    else{
//        int i = 0;
//        int fid = plist[0].id;
//        while((fid != id) && (i < size)){  //顺序查找对应的一级科目id
//            fid = plist[i].id;
//            i++;
//        }
//        if(i < size){ //找到了
//            plist[i].dsum += v;
//        }
//        else{ //没有找到
//            PZCollect pc;
//            pc.id = id;
//            pc.dsum = v;
//            pc.jsum = 0;
//            pc.name = "";
//            plist.append(pc);
//        }
//    }
//}

//void PzCollectProcess::setName(int id, QString name)
//{
//    int size = plist.size();
//    if(size == 0){
//        PZCollect pc;
//        pc.id = id;
//        pc.dsum = 0;
//        pc.jsum = 0;
//        pc.name = name;
//        plist.append(pc);
//    }
//    else{
//        int i = 0;
//        int fid = plist[0].id;
//        while((fid != id) && (i < size)){  //顺序查找对应的一级科目id
//            fid = plist[i].id;
//            i++;
//        }
//        if(i < size){ //找到了
//            plist[i].name = name;
//        }
//        else{ //没有找到
//            PZCollect pc;
//            pc.id = id;
//            pc.dsum = 0;
//            pc.jsum = 0;
//            pc.name = name;
//            plist.append(pc);
//        }
//    }
//}

void PzCollectProcess::setValue(int id, QString name, double jsum, double dsum)
{
    int size = plist.size();
    if(size == 0){
        PZCollect pc;
        pc.id = id;
        pc.dsum = dsum;
        pc.jsum = jsum;
        pc.name = name;
        plist.append(pc);
    }
    else{
        int i = 0;
        int fid = plist[0].id;
        while(fid != id){  //顺序查找对应的一级科目id
            i++;
            if(i == size)
                break;
            fid = plist[i].id;

        }
        if(i < size){ //找到了
            //plist[i].name = name;
            plist[i].dsum += dsum;
            plist[i].jsum += jsum;
        }
        else{ //没有找到
            PZCollect pc;
            pc.id = id;
            pc.dsum = dsum;
            pc.jsum = jsum;
            pc.name = name;
            plist.append(pc);
        }
    }
}

void PzCollectProcess::clear()
{
    plist.clear();
}

double PzCollectProcess::getJ(int id)
{
    int size = plist.size();
    if(size == 0){
        //错误情况
        qDebug() << "error" ;
    }
    else{
        int i = 0;
        int fid = plist[0].id;
        while((fid != id) && (i < size)){  //顺序查找对应的一级科目id
            i++;
            fid = plist[i].id;
        }
        if(i < size){ //找到了
            return plist[i].jsum;
        }
    }
}

double PzCollectProcess::getD(int id)
{
    int size = plist.size();
    if(size == 0){
        //错误情况
        qDebug() << "error" ;
    }
    else{
        int i = 0;
        int fid = plist[0].id;
        while((fid != id) && (i < size)){  //顺序查找对应的一级科目id
            i++;
            fid = plist[i].id;
        }
        if(i < size){ //找到了
            return plist[i].dsum;
        }
    }
}

QString PzCollectProcess::getName(int id)
{
    int size = plist.size();
    if(size == 0){
        //错误情况
        qDebug() << "error" ;
    }
    else{
        int i = 0;
        int fid = plist[0].id;
        while((fid != id) && (i < size)){  //顺序查找对应的一级科目id
            i++;
            fid = plist[i].id;

        }
        if(i < size){ //找到了
            return plist[i].name;
        }
    }
}

int PzCollectProcess::getCount()
{
    return plist.size();
}

QStandardItemModel* PzCollectProcess::toModel()
{
    double jsum, dsum;
    QHash<int, QString> codeHash;  //科目代码到科目余额字段名之间的映射
    QSqlQuery query;
    bool r = query.exec("select id, subCode from FirSubjects");
    if(r){
        while(query.next()){
            int id = query.value(0).toInt();
            QString code = query.value(1).toString();
            int t = code.left(1).toInt();
            switch(t){
            case 1:
                codeHash[id] = QString("A%1").arg(code);
                break;
            case 2:
                codeHash[id] = QString("B%1").arg(code);
                break;
            case 3:
                codeHash[id] = QString("C%1").arg(code);
                break;
            case 4:
                codeHash[id] = QString("D%1").arg(code);
                break;
            case 5:
                codeHash[id] = QString("E%1").arg(code);
                break;
            case 6:
                codeHash[id] = QString("F%1").arg(code);
                break;
            }
        }
    }
    QStandardItemModel* model = new QStandardItemModel;
    qSort(plist.begin(), plist.end());
    for(int i = 0; i < plist.size(); ++i){
        QStandardItem* nameitem = new QStandardItem(plist[i].name);
        QStandardItem* codeitem = new QStandardItem(codeHash[plist[i].id]);
        QStandardItem* jsumitem = new QStandardItem(QString("%1").arg(plist[i].jsum));
        QStandardItem* dsumitem = new QStandardItem(QString("%1").arg(plist[i].dsum));
        QStandardItem* extraitem = new QStandardItem(QString("%1").arg(plist[i].jsum - plist[i].dsum));
        QList<QStandardItem*> itemlist;
        itemlist.append(nameitem);
        itemlist.append(codeitem);
        itemlist.append(jsumitem);
        itemlist.append(dsumitem);
        itemlist.append(extraitem);
        model->appendRow(itemlist);
        jsum += plist[i].jsum;
        dsum += plist[i].dsum;
    }
    //加入合计行
    QStandardItem* nameitem = new QStandardItem(QObject::tr("合计"));
    QStandardItem* codeitem = new QStandardItem();
    QStandardItem* jsumitem = new QStandardItem(QString("%1").arg(jsum));
    QStandardItem* dsumitem = new QStandardItem(QString("%1").arg(dsum));
    QStandardItem* extraitem = new QStandardItem(QString("%1").arg(jsum - dsum));
    QList<QStandardItem*> itemlist;
    itemlist.append(nameitem);
    itemlist.append(codeitem);
    itemlist.append(jsumitem);
    itemlist.append(dsumitem);
    itemlist.append(extraitem);
    model->appendRow(itemlist);

    //设置表格的标题
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("科目名"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("科目代码"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("借方合计"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("贷方合计"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("余额"));
    return model;
}

////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
BasicDataDialog::BasicDataDialog(bool isWizard, QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->isWizard = isWizard;
    if(!isWizard){
        ui.btnNext->setText(tr("关闭"));
        ui.lblStep->setVisible(false);
    }

    connect(ui.btnImpOk, SIGNAL(clicked()), this, SLOT(btnImpOkClicked()));
    connect(ui.btnImpCancel, SIGNAL(clicked()), this, SLOT(btnImpCancelClicked()));
    connect(ui.btnImpBow, SIGNAL(clicked()), this, SLOT(btnImpBowClicked()));
    connect(ui.btnExpBow, SIGNAL(clicked()), this, SLOT(btnExpBowClicked()));
    connect(ui.btnExpCancel, SIGNAL(clicked()), this, SLOT(btnEXpCancelClicked()));
    connect(ui.btnExpOk, SIGNAL(clicked()), this, SLOT(btnExpOkClicked()));
    //connect(ui.chkImpSndSpec, SIGNAL(stateChanged(int)), this, SLOT(chkImpSpecStateChanged(int)));
    connect(ui.chkExpSndSpec, SIGNAL(stateChanged(int)), this, SLOT(chkExpSpecStateChanged(int)));

    //ui.btnImpBow->setEnabled(ui.chkImpSndSpec->isChecked());
    //ui.edtImpFile->setEnabled(ui.chkImpSndSpec->isChecked());
    ui.btnExpBow->setEnabled(ui.chkExpSndSpec->isChecked());
    ui.edtExpFile->setEnabled(ui.chkExpSndSpec->isChecked());

    //当前使用的科目系统类别
    //PSetting* setting = PSetting::getInstance();
    //usedSubCls = setting->readUsedSubCls(setting->getRecentOpenAccount());
}

void BasicDataDialog::btnImpOkClicked()
{
    QSqlQuery q, q2;
    QString s;
    bool r;
    int c;

    //导入一级科目及其类别到FirSubjects、FstSubClasses表中
    if(ui.chxImpFst->isChecked()){
        impFstSubFromBasic(usedSubCls);

        //一级科目改变以后将要重新创建科目余额表
        //（首先要检测余额表是否存在，如存在，则询问是否保存数据待创建后导入）
        //这个暂不做考虑，因为新旧科目系统的代码不同，如果导入前后的科目系统不同，则保存此前的余额数据是无意义的
        crtSubExtraTable(false); //新建余额表
    }




    int count;
    QString prefix;

    //二级科目及其类别到SecSubjects、SndSubClass表中
    if(ui.chkImpSndCls->isChecked()){
        ui.lblStage->setText(tr("正在导入二级科目类别："));
        impSndSubFromBasic();

        ui.lblStage->setText("");
    }

    //导入常用二级科目
//    if(ui.chxImpSndCmn->isChecked()){
//        ui.lblStage->setText(tr("正在导入常用二级科目："));

//        //插入到表中前先清空二级科目表和一、二级科目映射表
//        r = q2.exec("delete from SecSubjects");
//        r = q2.exec("delete from FSAgent");

//        QSettings setting("./config/SndSubjects.ini", QSettings::IniFormat);
//        setting.setIniCodec(QTextCodec::codecForTr());
//        setting.beginGroup("main");
//        count = setting.value("count").toInt();
//        prefix = setting.value("prefix").toString();
//        setting.endGroup();

//        ui.progressBar->setMinimum(0);
//        ui.progressBar->setMaximum(count);

//        QString sname, lname, remcode, owner;
//        int cid;

//        q.prepare("insert into SecSubjects(subName, subLName, remCode, classId)"
//                      "values(:sname, :lname, :remcode, :cid)");
//        for(int i = 1; i <= count; ++i){
//            setting.beginGroup(QString("%1-%2").arg(prefix).arg(i));
//            sname = setting.value("sname").toString();
//            lname = setting.value("lname").toString();
//            remcode = setting.value("remcode").toString();
//            cid = setting.value("class").toInt();
//            owner = setting.value("owner").toString();
//            setting.endGroup();
//            q.bindValue(":sname", sname);
//            q.bindValue(":lname", lname);
//            q.bindValue(":remcode", remcode);
//            q.bindValue(":cid", cid);
//            r = q.exec();

//            //建立一、二级科目的对应关系
//            QStringList olist = owner.split(" ");
//            if(olist.count() > 0){
//                //获取刚刚插入的二级科目的id值
//                QString s = QString("select id from SecSubjects where subName = '%1'").arg(sname);
//                if(q2.exec(s) && q2.first()){ //其实还应该检测是否只返回一条记录，否则将产生重名
//                    int sid = q2.value(0).toInt();
//                    for(int i = 0; i < olist.count(); ++i){
//                        //获取所属的一级科目的ID值
//                        QString s = QString("select id from FirSubjects where subCode = %1").arg(olist[i]);
//                        if(q2.exec(s) && q2.first()){
//                            int fid = q2.value(0).toInt();
//                            r = q2.exec(QString("insert into FSAgent(fid, sid) values(%1, %2)").arg(fid).arg(sid));
//                        }

//                    }
//                }

//            }
//            ui.progressBar->setValue(i);
//        }
//        ui.lblStage->setText("");
//    }

    //导入业务客户数据
    if(ui.chkImpClients->isChecked()){

//        if(!QFile::exists(ui.edtImpFile->text())){
//            QMessageBox::warning(this, tr("警告信息"), tr("文件不存在"));
//            return;
//        }

        ui.lblStage->setText(tr("正在导入专有二级科目："));
        impCliFromBasic();
//        QSettings setting(ui.edtImpFile->text(), QSettings::IniFormat);
//        setting.setIniCodec(QTextCodec::codecForTr());
//        setting.beginGroup("main");
//        count = setting.value("count").toInt();
//        prefix = setting.value("prefix").toString();
//        setting.endGroup();

//        ui.progressBar->setMinimum(0);
//        ui.progressBar->setMaximum(count);

//        QString sname, lname, remcode, owner;
//        int cid;
//        query.prepare("insert into SecSubjects(subName, subLName, remCode, classId)"
//                      "values(:sname, :lname, :remcode, :cid)");
//        for(int i = 1; i <= count; ++i){
//            setting.beginGroup(QString("%1-%2").arg(prefix).arg(i));
//            sname = setting.value("sname").toString();
//            lname = setting.value("lname").toString();
//            remcode = setting.value("remcode").toString();
//            cid = setting.value("class").toInt();
//            owner = setting.value("owner").toString();
//            setting.endGroup();
//            query.bindValue(":sname", sname);
//            query.bindValue(":lname", lname);
//            query.bindValue(":remcode", remcode);
//            query.bindValue(":cid", cid);
//            query.exec();

//            //建立一、二级科目的对应关系
//            QStringList olist = owner.split(" ");
//            if(olist.count() > 0){
//                //获取刚刚插入的二级科目的id值
//                QString s = QString("select id from SecSubjects where subName = '%1'").arg(sname);
//                if(query2.exec(s) && query2.first()){ //其实还应该检测是否只返回一条记录，否则将产生重名
//                    int sid = query2.value(0).toInt();
//                    for(int i = 0; i < olist.count(); ++i){
//                        //获取所属的一级科目的ID值
//                        QString s = QString("select id from FirSubjects where subCode = %1").arg(olist[i]);
//                        if(query2.exec(s) && query2.first()){
//                            int fid = query2.value(0).toInt();
//                            result = query2.exec(QString("insert into FSAgent(fid, sid) values(%1, %2)").arg(fid).arg(sid));
//                        }

//                    }
//                }

//            }
//            ui.progressBar->setValue(i);
//        }
    }

    //导入报表结构信息并建立对应数据库表格
    if(ui.chkAssets->isChecked())
        crtReportStructTable(RPT_BALANCE);
    if(ui.chkProfit->isChecked())
        crtReportStructTable(RPT_PROFIT);
    if(ui.chkCash->isChecked())
        crtReportStructTable(RPT_CASH);
    if(ui.chkOwner->isChecked())
        crtReportStructTable(RPT_OWNER);



    demandAttachDatabase(true);//附加基础数据库到当前连接上

    //导入凭证分册类型
    if(ui.chkBgCls->isChecked()){        
        //删除原来表中的数据
        s = "delete from AccountBookGroups";
        r = q.exec(s);

        //从基础数据库中导入币种表内容
        s = QString("insert into AccountBookGroups(code,name,explain) select code,name,explain "
                    "from basic.AccountBookGroups");
        r = q.exec(s);
        c = q.numRowsAffected();

//        QMap<int, QString> map;
//        PSetting s;
//        map = s.readBookGroupClass();
//        bool r = query2.exec("delete from AccountBookGroups");
//        query.prepare("insert into AccountBookGroups(code, name) "
//                      "values(:code, :name)");
//        QMapIterator<int, QString> i(map);
//        while(i.hasNext()){
//            i.next();
//            query.bindValue(":code", i.key());
//            query.bindValue(":name", i.value());
//            r = query.exec();
//        }
    }

    //导入货币类型
    if(ui.chkMt->isChecked()){
        //删除原来表中的数据
        s = "delete from MoneyTypes";
        r = q.exec(s);

        //从基础数据库中导入币种表内容
        s = QString("insert into MoneyTypes(code,sign,name) select code,sign,name "
                    "from basic.MoneyTypes");
        r = q.exec(s);
        c = q.numRowsAffected();
    }

    //导入凭证类别
    if(ui.chkPzCls->isChecked()){
        //删除原来表中的数据
        s = "delete from PzClasses";
        r = q.exec(s);

        //从基础数据库中导入币种表内容
        s = QString("insert into PzClasses(code,name,sname,explain) select code,name,sname,explain "
                    "from basic.PzClasses");
        r = q.exec(s);
        c = q.numRowsAffected();

    }

    demandAttachDatabase(false);

    if(!isWizard)
        close();
}

//从对应的报表结构定义文件中读取报表的结构定义，并导入到ReportStructs表，并根据当前
//账户所采用的报表类型（新式或老式）来创建保存报表数据的数据库表
void BasicDataDialog::crtReportStructTable(int clsid)
{
    bool r;
    QString fileName;
    QString tableName;
    switch(clsid){
    case 1:
        fileName = "./config/report/balance.ini";
        tableName = tr("资产负债表");
        break;
    case 2:
        fileName = "./config/report/profit.ini";
        tableName = tr("利润表");
        break;
    case 3:    //现金流量表
        return;
    case 4:    //所有者权益变动表
        return;
    }

    ui.lblStage->setText(QString(tr("正在导入%1结构：")).arg(tableName));

    QSettings setting(fileName, QSettings::IniFormat);
    setting.setIniCodec(QTextCodec::codecForTr());

    //读取报表类型的前缀作为键
    QList<QString> prefixLst;
    setting.beginGroup("main");
    int typeCount = setting.value("typeCount").toInt();
    for(int i = 1; i <= typeCount; ++i)
        prefixLst.append(setting.value(QString("prefix-%1").arg(i)).toString());
    setting.endGroup();

    //
    QSqlQuery q, q2;
    for(int i = 0; i < prefixLst.count(); ++i){
        QString prefix = prefixLst[i];
        setting.beginGroup(prefix);
        int count = setting.value("itemCount").toInt();  //报表字段数
        int typeCode = setting.value("typeCode").toInt();//报表类型代码

        //将报表标题等信息保存到报表附加信息表（ReportAdditionInfo）中
        QString tname = setting.value("tabName").toString(); //在数据库中保存报表数据的表名
        QString title = setting.value("title").toString();  //报表标题
        setting.endGroup();
        //首先要检测是否存在，如有则删除之
        QSqlQuery q;
        QString s = QString("select id from ReportAdditionInfo "
                            "where (clsid = %1) and (tid = %2)")
                .arg(clsid).arg(typeCode);
        if(q.exec(s) && q.first()){
            s = QString("delete from ReportAdditionInfo where "
                        "(clsid = %1) and (tid = %2)")
                    .arg(clsid).arg(typeCode);
            r = q.exec(s);
            int n = q.numRowsAffected();
            int j = 0;
        }
        s = QString("insert into ReportAdditionInfo(clsid, tid, tname, title) "
                    "values(%1, %2, '%3', '%4')").arg(clsid).arg(typeCode)
                    .arg(tname).arg(title);
        r = q.exec(s);

        ui.lblStage->setText(QString(tr("正在导入%1（%2）结构："))
                             .arg(tableName).arg(tname));
        ui.progressBar->setMinimum(0);
        ui.progressBar->setMaximum(count);

        QString name,ftitle, calMethod;
        int vo, co, row;
        QList<QString> flst;  //保存报表数据的数据库表的字段名列表

        //在加入报表信息前先删除已有的结构数据
        s = QString("delete from ReportStructs where "
                            "(clsid = %1) and (tid = %2)")
                .arg(clsid).arg(typeCode);
        r = q2.exec(s);
        int c = q2.numRowsAffected();

        //将报表结构信息保存到表ReportStructs中
        s = QString("insert into ReportStructs(clsid, tid, viewOrder, "
                    "calOrder, rowNum, fname, ftitle, fformula) values("
                    ":clsid, :tid, :viewOrder, :calOrder, :rowNum, :fname, "
                    ":ftitle, :fformula)");

        r = q.prepare(s);
        for(int j = 1; j <= count; ++j){
            QString ps = QString("%1-f%2").arg(prefix).arg(j); //报表字段的键名
            setting.beginGroup(ps);
            vo = setting.value("viewOrder").toInt();  //报表字段的显示顺序
            co = setting.value("calOrder").toInt();   //报表字段的计算顺序
            row = setting.value("row").toInt();       //报表字段的行号
            name = setting.value("name").toString();  //报表字段的内部名称
            if(co > 0)  //只有具有数值的报表字段才有对应的数据库表字段
                flst.append(name);
            ftitle = setting.value("fTitle").toString(); //报表字段的标题
            calMethod = setting.value("calMethod").toString(); //报表字段的计算公式
            setting.endGroup();
            q.bindValue(":clsid", clsid);
            q.bindValue(":tid", typeCode);
            q.bindValue(":viewOrder", vo);
            q.bindValue(":calOrder", co);
            q.bindValue(":rowNum", row);
            q.bindValue(":fname", name);
            q.bindValue(":ftitle", ftitle);
            q.bindValue(":fformula", calMethod);
            r = q.exec();
            ui.progressBar->setValue(j);
        }

        //创建保存报表数据的数据库表
        //如果要完善一点的话，必须检测对应的表是否存在，以及是否需要保存数据等
        //为了是创建表得以成功，必须首先删除表
        s = QString("DROP TABLE IF EXISTS %1").arg(tname);
        r = q2.exec(s);
        s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY, "
                    "year INTEGER, month INTEGER, ").arg(tname);
        for(int k = 0; k < flst.count(); ++k){
            s.append(flst[k]);
            s.append(" REAL, ");
        }
        s.chop(2);
        s.append(")");
        r = q2.exec(s);
        flst.clear();
    }
}

//创建余额表（参数isTem--true：创建临时余额表用来保存先前表的数据，false：新建余额表）
void BasicDataDialog::crtSubExtraTable(bool isTem)
{
    bool r;
    QSqlQuery q, q2;
    QString tname,tname2;

    if(isTem){
        tname = "tem1";
        tname2 = "tem2";
    }
    else{
        tname = "SubjectExtras";
        tname2 = "SubjectExtraDirs";
    }

    QString s = QString("DELETE FROM %1").arg(tname);
    QString s2 = QString("DELETE FROM %1").arg(tname2);
    r = q2.exec(s);
    r = q2.exec(s2);

    //s = QString("DROP TABLE IF EXISTS %1").arg(tname); //调用这句返回false，搞不懂？
    s = QString("DROP TABLE %1").arg(tname);
    s2 = QString("DROP TABLE %1").arg(tname2);
    r = q2.exec(s);
    r = q2.exec(s2);

    s = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY, "
        "year INTEGER, month INTEGER, state INTEGER, mt INTEGER, ").arg(tname);
    s2 = QString("CREATE TABLE %1(id INTEGER PRIMARY KEY, "
        "year INTEGER, month INTEGER, mt INTEGER, ").arg(tname2);

    //读取一级科目代码，并据此构造科目余额表的各个科目的对应字段
    r = q2.exec("select subCode from FirSubjects order by subCode");

    QString fname; //字段名
    while(q2.next()){
        QString code = q2.value(0).toString();
        int t = code.left(1).toInt();
        switch(t){
        case 1:
            fname = QString("A").append(code);
            break;
        case 2:
            fname = QString("B").append(code);
            break;
        case 3:
            fname = QString("C").append(code);
            break;
        case 4:
            fname = QString("D").append(code);
            break;
        case 5:
            fname = QString("E").append(code);
            break;
        case 6:
            fname = QString("F").append(code);
            break;
        }
        s.append(fname);
        s2.append(fname);
        s.append(" ");
        s2.append(" ");
        s.append("REAL");
        s2.append("INTEGER");
        s.append(", ");
        s2.append(", ");
    }
    s.chop(2);
    s2.chop(2);
    s.append(")");
    s2.append(")");

    r = q.exec(s);
    r = q.exec(s2);
    int i = 0;
}

void BasicDataDialog::btnImpCancelClicked()
{
    close();
}

void BasicDataDialog::btnImpBowClicked()
{
    QFileDialog* fdlg = new QFileDialog;
    fdlg->setDirectory("./config");
    fdlg->setFilter("*.ini");
    fdlg->setFileMode(QFileDialog::ExistingFile);
    if(QDialog::Accepted == fdlg->exec()){
        QStringList files = fdlg->selectedFiles();
        if(files.count() == 1)
            ui.edtImpFile->setText(files[0]);
    }
}

void BasicDataDialog::btnExpOkClicked()
{

}

void BasicDataDialog::btnEXpCancelClicked()
{

}

void BasicDataDialog::btnExpBowClicked()
{
    QFileDialog* fdlg = new QFileDialog;
    fdlg->setDirectory("./config");
    fdlg->setFilter("*.ini");
    fdlg->setFileMode(QFileDialog::AnyFile);
    if(QDialog::Accepted == fdlg->exec()){
        QStringList files = fdlg->selectedFiles();
        if(files.count() == 1)
            ui.edtExpFile->setText(files[0]);
    }
}


//void BasicDataDialog::chkImpSpecStateChanged(int state)
//{
//    ui.btnImpBow->setEnabled(ui.chkImpSndSpec->isChecked());
//    ui.edtImpFile->setEnabled(ui.chkImpSndSpec->isChecked());
//}

void BasicDataDialog::chkExpSpecStateChanged(int state)
{
    ui.btnExpBow->setEnabled(ui.chkExpSndSpec->isChecked());
    ui.edtExpFile->setEnabled(ui.chkExpSndSpec->isChecked());
}

//导航到创建账户向导的第四步或如果不在向导中则关闭对话框
void BasicDataDialog::nextStep()
{
    if(isWizard)
        emit toNextStep(3, 4);
    else
        close();
}

//按需附加基础数据库到当前连接上(参数isAttach为true：要求附加，false：要求分离)
void BasicDataDialog::demandAttachDatabase(bool isAttach)
{
    static bool preState = false;//记录附加前的基础数据库的附加状态
    //（true：已经附加，要求分离时不做分离；false：没有附加，也就是说要求分离时，必须分离）
    bool r;
    QString s;
    QSqlQuery q;

    if(isAttach){
        //首先判断先前的附加状态
        s = "select * from basic.sqlite_master";
        if(q.exec())
            preState = true;  //先前已附加
        else{
            preState = false;
            QString filename = "./datas/basicdatas/basicdata.dat";
            s = QString("attach database '%1' as basic").arg(filename);
            r = q.exec(s);
        }

    }
    else{ //要求分离
        if(!preState){ //如果先前没有附加，则要分离
            s = "detach database basic";
            r = q.exec(s);
        }
    }
}

//从基础数据库中导入一级科目及其类别到相应表中，
//参数subCls表示所采用的科目系统类别（1：老式，2：新式）
void BasicDataDialog::impFstSubFromBasic(int subCls)
{
    //因账户数据库格式已被修改，这个函数已不适合
//    bool r;
//    QSqlQuery q;
//    int c;  //tem

//    demandAttachDatabase(true);//附加基础数据库到当前连接上

//    //删除原来表中的数据
//    QString s = QString("delete from %1").arg(tbl_fsclass);
//    r = q.exec(s);
//    s = QString("delete from %1").arg(tbl_fsub);
//    r = q.exec(s);

//    //从基础数据库中导入一级科目类别表
//    s = QString("insert into FstSubClasses(code,name) select code,name "
//                "from basic.FirstSubCls where subCls = %1").arg(subCls);
//    r = q.exec(s);
//    c = q.numRowsAffected();

//    //从基础数据库中导入一级科目表
//    s = QString("insert into FirSubjects(subCode,remCode,belongTo,isView,isReqDet,weight,"
//                "subName,description,utils) select subCode,remCode,belongTo,isView,"
//                "isReqDet,weight,subName,description,utils from basic.FirstSubs "
//                "where subCls = %1").arg(subCls);
//    r = q.exec(s);
//    c = q.numRowsAffected();

//    demandAttachDatabase(false);
}

//从基础数据库中导入二级科目及其类别到相应表中，
//对于二级科目暂不做考虑新旧科目系统的区别
void BasicDataDialog::impSndSubFromBasic()
{
//    bool r;
//    QString s;
//    QSqlQuery q,q2;
//    int c;  //tem

//    demandAttachDatabase(true);//附加基础数据库到当前连接上

//    //删除原来表中的数据
//    s = QString("delete from %1").arg(tbl_ssclass);
//    r = q.exec(s);
//    s = QString("delete from %1").arg(tbl_ssub);
//    r = q.exec(s);

////    //在FSAgent表中删除除了银行存款和现金以外的所有一级科目下的二级科目的所属关系
//    s = QString("select id from FirSubjects where subCode = '1002'");
//    r = q.exec(s);
//    r = q.first();
//    int bankId = q.value(0).toInt();
//    s = QString("select id from FirSubjects where subCode = '1001'");
//    r = q.exec(s);
//    r = q.first();
//    int cashId = q.value(0).toInt();
//    s = QString("delete from FSAgent where (fid != %1) and "
//                "(fid != %2)").arg(cashId).arg(bankId);
//    r = q.exec(s);


//    //从基础数据库中导入二级科目类别表
//    s = QString("insert into SndSubClass(clsCode,name,explain) select clsCode,name,explain "
//                "from basic.SecondSubCls");
//    r = q.exec(s);
//    //c = q.numRowsAffected();

//    //从基础数据库中导入二级科目表
//    s = QString("insert into SecSubjects(subName,subLName,remCode,classId) "
//                "select subName,subLName,remCode,classId "
//                "from basic.SecondSubs");
//    r = q.exec(s);
//    //c = q.numRowsAffected();

//    //建立一二级科目的所属关系
//    s = QString("select subName,belongTo,subCode from basic.SecondSubs "
//                "where subCls = %1").arg(usedSubCls);
//    r = q.exec(s);
//    while(q.next()){
//        QString ownerStr = q.value(1).toString();
//        if(ownerStr != ""){
//            QString subName = q.value(0).toString();
//            QStringList ownerLst = ownerStr.split(",");
//            QStringList codeLst = q.value(2).toString().split(",");
//            s = QString("select id from SecSubjects where subName = '%1'")
//                .arg(subName);
//            r = q2.exec(s);
//            r = q2.first();
//            int sid = q2.value(0).toInt();
//            for(int i = 0; i < ownerLst.count(); ++i){
//                s = QString("select id from FirSubjects where "
//                            "subCode = %1").arg(ownerLst[i]);
//                r = q2.exec(s);
//                r = q2.first();
//                int fid = q2.value(0).toInt();

//                //插入到FSAgent表中
//                s = QString("insert into FSAgent(fid,sid,subCode) "
//                            "values(%1,%2,'%3')").arg(fid).arg(sid)
//                        .arg(codeLst[i]);
//                r = q2.exec(s);
//            }
//        }
//    }

//    demandAttachDatabase(false);
}

//从基础数据库中导入客户数据到SecSubjects表中
void BasicDataDialog::impCliFromBasic()
{
//    bool r;
//    QString s;
//    QSqlQuery q;
//    int c;  //tem

//    //获取业务客户类别的ID值
//    s = QString("select clsCode from SndSubClass where name = '%1'").arg(tr("业务客户"));
//    r = q.exec(s);
//    r = q.first();
//    int clsId = q.value(0).toInt();

//    //删除所有原先已有的业务客户
//    s = QString("delete from SecSubjects where classId = %1").arg(clsId);
//    r = q.exec(s);
//    r = q.first();
//    c = q.numRowsAffected();

//    demandAttachDatabase(true);//附加基础数据库到当前连接上

//    s = QString("insert into SecSubjects(subName,subLName,remCode,classId) "
//                "select subName,subLName,remCode,%1 from basic.Clients").arg(clsId);
//    r = q.exec(s);
//    c = q.numRowsAffected();

//    demandAttachDatabase(false);
}

//将当前的一级科目配置保存到基础数据库中
void BasicDataDialog::saveFstToBase()
{
    QSqlQuery q,q2;
    QString s,s2;
    bool r;

    QSet<QString> codes; //获取基础库中已有的一级科目代码集合
    demandAttachDatabase(true);
    s = QString("select subCode from basic.FirstSubs where subCls = %1")
        .arg(usedSubCls);
    r = q.exec(s);
    //QSqlRecord rec = q.record();
    while(q.next()){
        codes.insert(q.value(0).toString());
    }


    s = "select * from FirSubjects";
    r = q.exec(s);
    int num = 0; //完成的记录数
    while(q.next()){
        QString code = q.value(FSUB_SUBCODE).toString();
        QString remCode = q.value(FSUB_REMCODE).toString();
        int belongTo = q.value(FSUB_CLASS).toInt();
        int isView = q.value(FSUB_ISVIEW).toInt();
        int isUseWb = q.value(FSUB_ISUSEWB).toInt();
        int weight = q.value(FSUB_WEIGHT).toInt();
        QString subName = q.value(FSUB_SUBNAME).toString();
        //QString description  = q.value(FSTSUB_DESC).toString();
        //QString utils = q.value(FSTSUB_UTILS).toString();

        if(codes.contains(code)){
            //更新
            s = QString("update basic.FirstSubs set remCode = '%1', "
                        "belongTo = %2, isView = %3, isReqDet = %4, "
                        "weight = %5, subName = '%6' where (subCls = %7) and (subCode = '%8')")
                    .arg(remCode).arg(belongTo).arg(isView).arg(isUseWb)
                    .arg(weight).arg(subName).arg(usedSubCls).arg(code);
            r = q2.exec(s);
        }
        else{
            s = QString("insert into basic.FirstSubs(subCls,subCode,"
                        "remCode,belongTo,isView,isReqDet,weight,subName) "
                        "values(%1,'%2','%3',%4,%5,%6,%7,'%8')")
                    .arg(usedSubCls).arg(code).arg(remCode).arg(belongTo)
                    .arg(isView).arg(isUseWb).arg(weight).arg(subName);
            r = q2.exec(s);
        }
        num++;
        ui.progressBar->setValue(num);
    }
    ui.progressBar->setValue(0);
    demandAttachDatabase(false);
}

//将当前的二级科目配置保存到基础数据库中
void BasicDataDialog::saveSndToBase()
{
    QSqlQuery q,q2;
    QString s,s2;
    bool r;

    //测试
//    QSet<QString> set;
//    set.insert(tr("社会保险个人"));
//    r = set.contains(tr("社会保险个人测试"));


    QSet<QString> names; //获取基础库中已有的二级科目代码集合
    demandAttachDatabase(true);
    s = QString("select subName from basic.SecondSubs where subCls = %1")
        .arg(usedSubCls);
    r = q.exec(s);
    while(q.next()){
        names.insert(q.value(0).toString());
    }


    //应该还要考虑保存二级科目类别表！！！！！！！！！！！！！！
    //代目前还没有在用户界面上进行添加、删除科目类别的功能，因此无需考虑

    //金融机构类的二级科目不需要保存，客户类二级科目另作处理
//    s = QString("select clsCode from SndSubClass where name = '%1'")
//        .arg(tr("金融机构"));
//    int bcode;
//    if(q.exec(s) && q.first())
//        bcode = q.value(0).toInt();
//    else{
//        qDebug() << tr("没有金融机构二级科目类别");
//        return;
//    }

//    int ccode;
//    s = QString("select clsCode from SndSubClass where name = '%1'")
//        .arg(tr("业务客户"));
//    if(q.exec(s) && q.first())
//        ccode = q.value(0).toInt();
//    else{
//        qDebug() << tr("没有业务客户二级科目类别");
//        return;
//    }


//    s = QString("select * from SecSubjects where (classId != %1) "
//                "and (classId != %2)").arg(bcode).arg(ccode);
//    r = q.exec(s);
//    int num = 0; //完成的记录数
//    while(q.next()){
//        QString subName = q.value(SNDSUB_SUBNAME).toString();
//        QString subLName = q.value(SNDSUB_SUBLONGNAME).toString();
//        QString remCode = q.value(SNDSUB_REMCODE).toString();
//        int clsId = q.value(SNDSUB_CALSS).toInt();
//        int sid = q.value(0).toInt();
//        //获取此二级科目被哪些一级科目包含了
//        s = QString("select FirSubjects.subCode, FSAgent.subCode from "
//                    "FSAgent join FirSubjects where "
//            "(FSAgent.fid = FirSubjects.id) and (FSAgent.sid = %1)")
//                .arg(sid);
//        QStringList fclst,sclst;
//        r = q2.exec(s);
//        while(q2.next()){
//            fclst.append(q2.value(0).toString());//一级科目代码列表
//            sclst.append(q2.value(1).toString());//二级科目代码列表
//        }
//        QString fcodes = fclst.join(","); //包含此二级科目的一级科目代码
//        QString scodes = sclst.join(",");

//        //更新
//        if(names.contains(subName)){
//            s = QString("update basic.SecondSubs set subName = '%1',"
//                        "subLName = '%2', remCode = '%3', classId = %4, "
//                        "belongTo = '%5', subCode = '%6' where (subCls = %7) "
//                        "and (subName = '%8')")
//                    .arg(subName).arg(subLName).arg(remCode).arg(clsId)
//                    .arg(fcodes).arg(scodes).arg(usedSubCls).arg(subName);
//            r = q2.exec(s);
//        }
//        else{ //插入
//            s = QString("insert into basic.SecondSubs(subCls, subName, "
//                        "subLName, remCode, classId,belongTo,subCode) "
//                        "values(%1,'%2','%3','%4',%5,'%6','%7')")
//                    .arg(usedSubCls).arg(subName).arg(subLName).arg(remCode)
//                    .arg(clsId).arg(fcodes).arg(scodes);
//            r = q2.exec(s);
//        }
//        num++;
//        ui.progressBar->setValue(num);
//    }
//    ui.progressBar->setValue(0);
//    demandAttachDatabase(false);
}


/////////////////////////////////////////////////////////////////

ReportDialog::ReportDialog(QWidget* parent)
{
    ui.setupUi(this);

    ui.dateEdit->setDate(QDate::currentDate());
    year = QDate::currentDate().year();
    month = QDate::currentDate().month();
    model = new QStandardItemModel;
    ui.tview->setModel(model);

    //设置报表类型代码
    if(ui.rdoAssets->isChecked())
        usedReporCid = 1;
    else if(ui.rdoProfits->isChecked())
        usedReporCid = 2;
    else if(ui.rdoCash->isChecked())
        usedReporCid = 3;
    else
        usedReporCid = 4;

    //应该从配置文件中读取系统使用什么样的报表分类代码，这里为简单期间，假定使用老式格式
    usedReportTid = 1;
    //因为默认选择是老式利润表
    tableName = readTableName(usedReporCid, usedReportTid);
    //从ReportAdditionInfo表中读取报表标题和表名
    readReportAddInfo();

    connect(ui.btnCreate, SIGNAL(clicked()), this, SLOT(btnCreateClicked()));
    connect(ui.btnView, SIGNAL(clicked()), this, SLOT(viewReport()));
    connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(btnSaveClicked()));
    connect(ui.btnToExcel, SIGNAL(clicked()), this, SLOT(btnToExcelClicked()));
    connect(ui.dateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(dateChanged(QDate)));
    connect(ui.rdoProfits, SIGNAL(toggled(bool)), this, SLOT(reportClassChanged()));
    connect(ui.rdoAssets, SIGNAL(toggled(bool)), this, SLOT(reportClassChanged()));
    connect(ui.rdoCash, SIGNAL(toggled(bool)), this, SLOT(reportClassChanged()));
    connect(ui.rdoOwner, SIGNAL(toggled(bool)), this, SLOT(reportClassChanged()));
}

//读取当前选择的报表所对应的数据库表名
QString ReportDialog::readTableName(int clsid, int tid)
{
    QString s;
    QSqlQuery q;

    s = QString("select tname from ReportAdditionInfo");
    if(q.exec(s) && q.first())
        return q.value(0).toString();
    else
        return "";
}

//从ReportAdditionInfo表中读取报表标题和表名
void ReportDialog::readReportAddInfo()
{
    QSqlQuery q;
    QString s;
    if(ui.rdoProfits->isChecked()){
        s = QString("select tname title from ReportAdditionInfo"
                    "where (clsid = %1) and tid = %2")
                .arg(usedReporCid).arg(usedReportTid);
    }
    bool r = q.exec(s);
    if(r && q.first()){
        tableName = q.value(0).toString();
        reportTitle = q.value(1).toString();
    }
}

void ReportDialog::dateChanged(const QDate &date)
{
    year = date.year();
    month = date.month();
    ui.btnSave->setEnabled(false);
    ui.btnSave->setEnabled(false);
}

//设置需要生成的报表类型
void ReportDialog::setReportType(int witch){
    switch(witch){
    case 1:
        ui.rdoAssets->setChecked(true);
        break;
    case 2:
        ui.rdoProfits->setChecked(true);
        break;
    case 3:
        ui.rdoCash->setChecked(true);
        break;
    case 4:
        ui.rdoOwner->setChecked(true);
    }
}

//选择的报表类别改变了
void ReportDialog::reportClassChanged()
{
    //设置报表类型代码
    if(ui.rdoAssets->isChecked())
        usedReporCid = 1;
    else if(ui.rdoProfits->isChecked())
        usedReporCid = 2;
    else if(ui.rdoCash->isChecked())
        usedReporCid = 3;
    else
        usedReporCid = 4;

    tableName = readTableName(usedReporCid, usedReportTid);
}

//根据报表数据的生成规则产生报表数据
void ReportDialog::btnCreateClicked()
{
//    if(ui.rdoAssets->isChecked())
//        generateAssets();
//    else if(ui.rdoProfits->isChecked())
//        generateProfits();
//    else if(ui.rdoCash)
//        generateCashs();
//    else
//        generateOwner();
    genReportDatas(usedReporCid, usedReportTid);
    ui.btnView->setEnabled(false);
}

//显示报表数据，数据来源于与报表对应的表中
void ReportDialog::btnViewCliecked()
{
    if(ui.rdoYear)
        viewReport(year, 0);
    else
        viewReport(year, month);

    ui.btnView->setEnabled(true);
}


//保存生成的报表数据到数据库对应表中
void ReportDialog::btnSaveClicked()
{
    //首先要检测是否存在与当前选择的报表年月相对应的记录，如有，删除之
    //更合理的是应该提示用户是否覆盖表中的记录，由用户来做出决定
    QSqlQuery q, q2;
    QString s;
    bool r;
    int m;

    if(ui.rdoYear->isChecked()){ //年报
        m = 0;
    }
    else  //月报
        m = month;

    s = QString("select id from %1 where (year = %2) and (month = %3)")
        .arg(tableName).arg(year).arg(m);
    if(q.exec(s) && q.first() &&
       (QMessageBox::Yes == QMessageBox::question(this, tr("确认消息"),
        tr("要覆盖已有的数据吗？"), QMessageBox::Yes | QMessageBox::No))){
        for(int i = 0; i < model->rowCount(); ++i){
            QString fname = model->data(model->index(i, 0)).toString();
            double v = model->data(model->index(i, 2)).toDouble();
            s = QString("update %1 set %2 = %3 where (year = %4) and (month = %5)")
                .arg(tableName).arg(fname).arg(v).arg(year).arg(m);
            r = q.exec(s);
        }

    }
    else{
        s = QString("insert into %1(year, month, ").arg(tableName);
        QString sf;
        QString sv = QString("values(%1, %2, ").arg(year).arg(m);
        for(int i = 0; i < model->rowCount(); ++i){
            sf.append(model->data(model->index(i, 0)).toString());
            sf.append(", ");
            double v = model->data(model->index(i, 2)).toDouble();
            sv.append(QString::number(v));
            sv.append(", ");
        }
        sf.chop(2);
        sf.append(") ");
        sv.chop(2);
        sv.append(")");
        s.append(sf);
        s.append(sv);
        r = q.exec(s);
        //int i = 0;
    }
    ui.btnView->setEnabled(true);
}

void ReportDialog::btnToExcelClicked()
{

}

//生成利润表数据
void ReportDialog::generateProfits()
{    
    QSqlQuery q;
    QString s;    
    bool r;

    QList<QString> nlist;         //按报表字段显示的顺序保存报表字段名
    QHash<QString, bool> fthash;   //报表字段的类型(1：数值型，0：非数值型，-1：空行)
    QHash<QString, double> vhash; //保存当月报表字段值，以及计算报表所需要的其他
                                  //参考值（比如科目余额和其他必要的参考项）

    QHash<QString, QString> thash;//保存报表字段名和报表字段标题的对应


    QHash<QString, double>  ahash; //保存报表字段名和累计值/年报期初值的对应

    //设置各个科目的余额数值，以便计算报表各字段时作参考    
    if(ui.rdoYear->isChecked())  //年报
        readSubjectsExtra(year, 12, &vhash);
    else //月报
        readSubjectsExtra(year, month, &vhash);

    //设置计算报表所需的其他参考项（目前，暂时还不知道需要哪些项目）

    //按报表字段的显示顺序读取报表字段标题
    nlist.clear();
    thash.clear();
    readReportTitle(usedReporCid, usedReportTid, &nlist, &fthash, &thash);

    //按报表结构信息表中的报表字段计算顺序，计算各个报表字段值并保存到vhash中
    calReportField(usedReporCid, usedReportTid, &vhash);

    //计算累计值(月报)/读取上年数（年报）
    if(ui.rdoMonth->isChecked()) //月报
        calAddValue(year, month, &nlist, &ahash);
    else if(ui.rdoYear->isChecked())//{ //年报
        calAddValue(year, 0, &nlist, &ahash);

    //装配模型
    createModel(&nlist, &thash, &vhash, &ahash);

    ui.btnSave->setEnabled(true);
    ui.btnToExcel->setEnabled(true);
}

//生成指定类别和类型的报表数据
void ReportDialog::genReportDatas(int clsid, int tid)
{
    QSqlQuery q;
    QString s;
    bool r;

    QList<QString> nlist;         //按报表字段显示的顺序保存报表字段名
    QHash<QString, bool> fthash;   //报表字段的类型(1：数值型，0：非数值型，-1：空行)
    QHash<QString, double> vhash; //保存当月报表字段值，以及计算报表所需要的其他
                                  //参考值（比如科目余额和其他必要的参考项）

    QHash<QString, QString> thash;//保存报表字段名和报表字段标题的对应


    QHash<QString, double>  ahash; //保存报表字段名和累计值/年报期初值的对应

    //设置各个科目的余额数值，以便计算报表各字段时作参考
    if(ui.rdoYear->isChecked())  //年报
        readSubjectsExtra(year, 12, &vhash);
    else //月报
        readSubjectsExtra(year, month, &vhash);

    //设置计算报表所需的其他参考项（目前，暂时还不知道需要哪些项目）

    //按报表字段的显示顺序读取报表字段标题
    nlist.clear();
    thash.clear();
    readReportTitle(clsid, tid, &nlist, &fthash, &thash);

    //按报表结构信息表中的报表字段计算顺序，计算各个报表字段值并保存到vhash中
    calReportField(clsid, tid, &vhash);

    //计算累计值(月报)/读取上年数（年报）
    if(ui.rdoMonth->isChecked()) //月报
        calAddValue(year, month, &nlist, &ahash);
    else if(ui.rdoYear->isChecked())//{ //年报
        calAddValue(year, 0, &nlist, &ahash);

    //装配模型
    createModel(&nlist, &thash, &vhash, &ahash);

    ui.btnSave->setEnabled(true);
    ui.btnToExcel->setEnabled(true);
}

void ReportDialog::generateAssets()
{
    QMessageBox::information(this, tr("一般信息"), tr("此功能还未实现"));
}

void ReportDialog::generateCashs()
{
    QMessageBox::information(this, tr("一般信息"), tr("此功能还未实现"));
}

void ReportDialog::generateOwner()
{
    QMessageBox::information(this, tr("一般信息"), tr("此功能还未实现"));
}

//创建显示报表所用的模型数据
//参数nlist：报表字段名，titles：报表字段标题，
//valuse：科目余额和报表字段值，avalues：累计值/上年数
void ReportDialog::createModel(QList<QString>* nlist, QHash<QString, QString>* titles,
                 QHash<QString, double>* values, QHash<QString, double>* avalues)
{
    //装配模型
    model->clear();

    for(int i = 0; i < nlist->count(); ++i){
        QList<QStandardItem*> items;
        QStandardItem* itemName = new QStandardItem(nlist->at(i));
        QStandardItem* itemTitle = new QStandardItem(titles->value(nlist->at(i)));
        double cur = values->value(nlist->at(i));
        QStandardItem* itemCur = new QStandardItem(QString::number(cur));
        double av = cur + avalues->value(nlist->at(i));
        QStandardItem* itemAv = new QStandardItem(QString::number(av));
        items.append(itemName);
        items.append(itemTitle);
        items.append(itemCur);
        items.append(itemAv);
        model->appendRow(items);
    }

    if(ui.rdoMonth->isChecked()){
        model->setHeaderData(0, Qt::Horizontal, tr("段名"));
        model->setHeaderData(1, Qt::Horizontal, tr("项目"));
        model->setHeaderData(2, Qt::Horizontal, tr("本月数"));
        model->setHeaderData(3, Qt::Horizontal, tr("本年累计数"));
    }
    else if(ui.rdoYear->isChecked()){
        model->setHeaderData(0, Qt::Horizontal, tr("段名"));
        model->setHeaderData(1, Qt::Horizontal, tr("项目"));
        model->setHeaderData(2, Qt::Horizontal, tr("本年金额"));
        model->setHeaderData(3, Qt::Horizontal, tr("上年金额"));
    }

    ui.tview->setColumnWidth(0, 40);
    ui.tview->setColumnWidth(1, 200);
}

//按报表字段的显示顺序读取报表字段的名称和标题
void ReportDialog::readReportTitle(int clsid, int tid,QList<QString>* nlist,
               QHash<QString, bool>* fthash, QHash<QString, QString>* titles)
{
    bool r;
    QSqlQuery q;
    QString s = QString("select fname, ftitle from ReportStructs "
                "where (clsid = %1) and (tid = %2) order by viewOrder")
            .arg(clsid).arg(tid);
    r = q.exec(s);
    if(r){
        while(q.next()){
            QString fname = q.value(0).toString();
            QString title = q.value(1).toString();
            (*titles)[fname] = title;
            nlist->append(fname);
            if(title == "")  //报表标题为空，这个报表字段要么是文本型要么是空行
                (*fthash)[fname] = false;
        }

    }
}

//按报表结构信息表中的报表字段计算顺序，计算各个报表字段值并保存到vhash中
void ReportDialog::calReportField(int clsid, int tid,
                    QHash<QString, double>* values)
{
    bool r;
    QSqlQuery q;
    QString s = QString("select fname, fformula from ReportStructs where "
                "(clsid = %1) and (tid = %2) and (calOrder > 0) order by calOrder asc")
            .arg(clsid).arg(tid);
    r = q.exec(s);
    if(r){
        ExpressParse ep;
        while(q.next()){
            QString fname = q.value(0).toString();
            QString formula = q.value(1).toString();
            //计算报表字段的值并加入到vhash中
            double v = ep.calValue(formula, values);
            (*values)[fname] = v;
        }
    }
}

//计算月报的累计值/读取年报上年数据
void ReportDialog::calAddValue(int year, int month, QList<QString>* names,
                               QHash<QString, double>* values)
{
    QString s;
    bool r;
    QSqlRecord rec;
    QSqlQuery q;

    //月报计算累计值
    //Sqlite内部没有内置sum()函数，因此必须自己实现
    if(month != 0){
        s = QString("select * from %1 where "
                    "(year = %2) and (month < %3)")
                .arg(tableName).arg(year).arg(month);
        r = q.exec(s);
        rec = q.record();
        if(r){
            while(q.next()){
                //对所有报表字段进行累加
                for(int i = 0; i < names->count(); ++i){
                    int idx = rec.indexOf(names->at(i));
                    double v = q.value(idx).toDouble();
                    (*values)[names->at(i)] += v;
                }
            }
        }
    }
    else{ //年报则读取上年数据
        s = QString("select * from %1 where "
                    "(year = %2) and (month = 0)")
                .arg(tableName).arg(year - 1);
        r = q.exec(s);
        if(r){
            q.first();
            rec = q.record();
            //QList<QString> fnameLst = thash.keys();  //报表字段名
            for(int i = 0; i < names->count(); ++i){
                int idx = rec.indexOf(names->at(i));
                double v = q.value(idx).toDouble();
                (*values)[names->at(i)] = v;
            }
        }
    }
}

//读取科目余额数据
void ReportDialog::readSubjectsExtra(int year, int month, QHash<QString, double>* values)
{
    QString s;
    bool r;
    QSqlQuery q;
    QSqlRecord rec;

    s = QString("select * from SubjectExtras where "
                "(year = %1) and (month = %2)").arg(year).arg(month);
    r = q.exec(s);
    if(r && q.first()){
        rec = q.record();
        for(int i = SE_SUBSTART; i < rec.count(); ++i){
            QString fname = rec.fieldName(i);
            double v = q.value(i).toDouble();
            (*values)[fname] = v;
        }
    }
}


void ReportDialog::viewReport()
{
    viewReport(year, month);
}

//要改变代码，增加对报表字段类型的处理逻辑...
void ReportDialog::viewReport(int year, int month)
{
    QString s;
    QSqlQuery q;
    bool r;
    QSqlRecord rec;

    QList<QString> names; //报表字段名
    QHash<QString, bool> fthash;   //报表字段的类型(1：数值型，0：非数值型，-1：空行)
    QHash<QString, QString>titles; //报表字段标题名
    QHash<QString, double>values;  //报表字段值
    QHash<QString, double>values2; //报表中的累计值（月报）

    readReportTitle(usedReporCid, usedReportTid, &names, &fthash, &titles);
    calAddValue(year, month, &names, &values2);
    s = QString("select * from %1 where (year = %2) and (month = %3)")
        .arg(tableName).arg(year).arg(month);
    if(q.exec(s) && q.first()){
        rec = q.record();
        for(int i = 0; i< names.count(); ++i){
            int idx = rec.indexOf(names.at(i));
            double v = q.value(idx).toDouble();
            values[names.at(i)] = v;
        }
    }

    //装配模型
    createModel(&names, &titles, &values, &values2);
}


//////////////////////银行账户设置对话框////////////////////////////////////////
SetupBankDialog::SetupBankDialog(bool isWizared, QWidget *parent) : QDialog(parent),
    ui(new Ui::SetupBankDialog)
{
    ui->setupUi(this);
    this->isWizared = isWizared;
    if(!isWizared){
        ui->btnCrtSub->setText(tr("创建二级科目"));
        ui->lblStep->setVisible(false);
    }

    model.setTable("Banks");
    model.select();
    ui->lstBank->setModel(&model);
    ui->lstBank->setModelColumn(BANK_NAME);
    mapper.setModel(&model);
    mapper.addMapping(ui->chkIsMain, BANK_ISMAIN);
    mapper.addMapping(ui->edtBankName, BANK_NAME);
    mapper.addMapping(ui->edtBankLName, BANK_LNAME);
    //mapper.addMapping(ui->edtRmbAccNum, BANK_RMB);
    //mapper.addMapping(ui->edtUsdAccNum, BANK_USD);

    mapper.toFirst();

    ui->tvAccList->setModel(&accModel);

}

SetupBankDialog::~SetupBankDialog()
{
    delete ui;
}

void SetupBankDialog::newBank()
{
    int r = model.rowCount();
    model.insertRow(r);
    model.setData(model.index(r, BANK_ISMAIN), false);
    model.submit();

    int newBankId = model.data(model.index(r,0)).toInt();
    accModel.setFilter(QString("bankID = %1").arg(newBankId));
    mapper.toLast();

    ui->chkIsMain->setEnabled(true);
    ui->edtBankName->setEnabled(true);
    ui->edtBankLName->setEnabled(true);
    ui->tvAccList->setEnabled(true);
    ui->btnNewAcc->setEnabled(true);

    int j = 0;
}

//新增银行帐号
void SetupBankDialog::newAcc()
{
    int row = accModel.rowCount();
    accModel.insertRow(row);
    accModel.setData(accModel.index(row, BA_BANKID), curBankId);
    accModel.setData(accModel.index(row, BA_MT), 1); //默认为人民币账户
    accModel.submit();
}

void SetupBankDialog::delBank()
{
    int row = ui->lstBank->currentIndex().row();
    model.removeRow(row);
    int c = accModel.rowCount();
    accModel.removeRows(0, c);

    if(row > 0){
        mapper.setCurrentIndex(--row);
        ui->lstBank->setCurrentIndex(model.index(row, MT_NAME));
    }

}

void SetupBankDialog::delAcc()
{
    int row = ui->tvAccList->currentIndex().row();
    accModel.removeRow(row);
}

void SetupBankDialog::save()
{
    model.submit();
    accModel.submit();
}

//在列表中选择了一个银行
void SetupBankDialog::selectedBank(const QModelIndex &index)
{
    int idx = index.row();
    mapper.setCurrentIndex(idx);

    //读取在该开户行中所设的银行账户信息
    curBankId = model.data(model.index(index.row(), 0)).toInt(); //银行ID
    QSqlQuery q;
    bool r;

    accModel.setTable("BankAccounts");
    accModel.setRelation(BA_MT, QSqlRelation("MoneyTypes", "code", "name"));
    accModel.setFilter(QString("bankID = %1").arg(curBankId));
    accModel.select();
    accModel.setHeaderData(2, Qt::Horizontal, tr("账户名"));
    accModel.setHeaderData(3, Qt::Horizontal, tr("帐号"));
    ui->tvAccList->setItemDelegate(new QSqlRelationalDelegate);
    ui->tvAccList->hideColumn(0);
    ui->tvAccList->hideColumn(BA_BANKID);
    ui->tvAccList->setColumnWidth(BA_ACCNUM, 200);

    ui->btnDelAcc->setEnabled(false);
    ui->btnDelBank->setEnabled(true);

    ui->chkIsMain->setEnabled(true);
    ui->edtBankName->setEnabled(true);
    ui->edtBankLName->setEnabled(true);
    ui->tvAccList->setEnabled(true);
    ui->btnNewAcc->setEnabled(true);
}

//选择了一个银行帐号
void SetupBankDialog::selBankAccNum(const QModelIndex &index)
{
    ui->btnDelAcc->setEnabled(true);

}

//为每个开户行下的货币类型账户的组合在SecSubjects表中创建一个二级科目
void SetupBankDialog::crtSndSubject()
{
    QSqlQuery q, q2;
    bool r;
    int cid;

    //获取金融机构类客户类别的代码
    QString s = QString("select %1 from %2 where %3='%1'")
            .arg(fld_nic_clscode).arg(tbl_nameItemCls).arg(fld_nic_name).arg(tr("金融机构"));
    if(q.exec(s) && q.first())
        cid = q.value(0).toInt();
    else{
        qDebug() << tr("不能找到金融机构类的代码");
        return;
    }


    //获取一级科目银行存款的id
    s = QString("select id from FirSubjects where "
                "subCode = '%1'").arg("1002");
    r = q.exec(s);
    r = q.first();
    int fid = q.value(0).toInt();

    //为所有银行下的各个账户在银行存款一级科目下设立二级科目
    for(int i = 0; i < model.rowCount(); ++i){
        QString bName = model.data(model.index(i, BANK_NAME)).toString();
        QString bLName = model.data(model.index(i, BANK_LNAME)).toString();
        int bankId = model.data(model.index(i, 0)).toInt();
        s = QString("select MoneyTypes.name from BankAccounts join MoneyTypes "
                    "where (BankAccounts.mtID = MoneyTypes.code) and "
                    "(BankAccounts.bankID = %1)").arg(bankId);
        r = q.exec(s);
        while(q.next()){
            QString sname(bName);
            QString slname(bLName);
            sname.append("-");
            sname.append(q.value(0).toString());
            slname.append("-");
            slname.append(q.value(0).toString());

            //在插入前首先检测是否已存在
            s = QString("select id from %1 where %2='%3'")
                    .arg(tbl_nameItem).arg(fld_ni_name).arg(sname);

            //如果不存在，才可以进行后续的二次插入操作
            if(q2.exec(s) && !q2.first()){
                s = QString("insert into %1(%2,%3,%4,%5) values('%6','%7',%8,%9)")
                        .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_lname)
                        .arg(fld_ni_class).arg(fld_ni_creator).arg(sname)
                        .arg(slname).arg(cid).arg(curUser->getUserId());
                r = q2.exec(s);
                //回读刚插入的记录的id
                s = QString("select last_insert_rowid()");
                r = q2.exec(s);
                r = q2.first();
                int sid = q2.value(0).toInt();

                //建立一二级科目的对应关系
                s = QString("insert into FSAgent(fid,sid) "
                            "values(%1,%2)").arg(fid).arg(sid);
                s = QString("insert into %1(%2,%3,%4,%5,%6) values(%7,%8,1,1,%9)")
                        .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_weight)
                        .arg(fld_ssub_enable).arg(fld_ssub_creator).arg(fid).arg(sid)
                        .arg(curUser->getUserId());
                r = q2.exec(s);
            }
        }
    }


    if(isWizared)
        emit toNextStep(2, 3);
    else
        close();
}


//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//可接受鼠标单击事件的标签类
ClickAbleLabel::ClickAbleLabel(QString title, QString code, QWidget* parent):QLabel(parent)
{
    //this->setStyleSheet("foreground-color:rgba(255,0,0,255)");
//    QPalette pal = this->palette();
//    pal.setColor(QPalette::Text,QColor(255,0,0));
//    setPalette(pal);
    this->code = code;
    QString s = QString("<H3><font color = green>%1</font></H3>").arg(title);
    setText(s);
}

void ClickAbleLabel::mousePressEvent (QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        emit viewSubExtra(code,event->globalPos());
    }
}


