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
#include "delegates.h"
#include "account.h"
#include "tables.h"
//#include "ExcelFormat.h"

//using namespace ExcelFormat;

//#define	FW_NORMAL	400
//#define	FW_BOLD		700

CreateAccountDialog::CreateAccountDialog(bool isWizarded, QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->isWizared = isWizarded;

    //如果不是在向导中而是单独打开设置账户信息对话框
    if(!isWizarded){
        ui.lblFileName->setVisible(false);
        ui.filename->setVisible(false);
        ui.lblStep->setVisible(false);
        ui.btnNext->setText(tr("保存"));
        model = new QSqlTableModel;
        model->setTable("AccountInfos");
        mapper = new QDataWidgetMapper;
        mapper->setModel(model);
        model->select();

        mapper->addMapping(ui.edtAccCode, ACCOUNT_CODE);
        mapper->addMapping(ui.sname, ACCOUNT_SNAME);
        mapper->addMapping(ui.lname, ACCOUNT_LNAME);
        mapper->toFirst();
    }
}

//CreateAccountDialog::CreateAccountDialog(QString sname, QString lname,
//                    QString filename, QWidget* parent)  : QDialog(parent)
//{
//    ui.setupUi(this);
//    ui.sname->setText(sname);
//    ui.lname->setText(lname);
//    ui.filename->setText(filename);
//}

QString CreateAccountDialog::getCode()
{
    return ui.edtAccCode->text();
}

QString CreateAccountDialog::getSName()
{
    return ui.sname->text();
}

QString CreateAccountDialog::getLName()
{
    return ui.lname->text();
}

QString CreateAccountDialog::getFileName()
{
    return ui.filename->text();
}

int CreateAccountDialog::getReportType()
{
    if(ui.rdoOld->isChecked())
        return Account::RPT_OLD;
    else
        return Account::RPT_NEW;
}

void CreateAccountDialog::nextStep()
{
    if(isWizared){
        emit toNextStep(1, 2);  //到第二步（设置开户行）
    }
    else{
       mapper->submit();
       close();
   }
}

/////////////////////////////////////////////////////////////////////////////////

OpenAccountDialog::OpenAccountDialog(QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
    //从应用程序的配置信息中读取已导入的账户
    QStringList accountLst;
    AppConfig::getInstance()->readAccountInfos(accInfoLst);
    for(int i = 0; i < accInfoLst.count(); ++i)
        accountLst << accInfoLst[i]->accName;
    model = new QStringListModel(accountLst);
    ui.accountList->setModel(model);
    connect(ui.accountList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(itemClicked(QModelIndex)));
    connect(ui.accountList, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(doubleClicked(QModelIndex)));
}

QString OpenAccountDialog::getSName()
{
    return accInfoLst[selAcc]->accName;
}

QString OpenAccountDialog::getLName()
{
    return accInfoLst[selAcc]->accLName;
}

QString OpenAccountDialog::getFileName()
{
    return accInfoLst[selAcc]->fileName;
}

void OpenAccountDialog::itemClicked(const QModelIndex &index)
{
    selAcc = index.row();
    ui.lblFName->setText(accInfoLst[selAcc]->fileName);
    ui.lblCode->setText(accInfoLst[selAcc]->code);
    ui.lname->setText(accInfoLst[selAcc]->accLName);
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
    y = account->getCurSuite();
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
        year = account->getCurSuite();
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
        QHash<int,double> rates;
        if(m == 1)
            BusiUtil::getRates(y-1,12,rates);
        else
            BusiUtil::getRates(y,m-1,rates);
        BusiUtil::saveRates(y,m,rates);
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

CollectPzDialog::CollectPzDialog(QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);
    setLayout(ui.mLayout);

    //初始化凭证分册类型选择框
    QString s = "select code,name from AccountBookGroups";
    QSqlQuery q;
    bool r = q.exec(s);
    ui.cmbBookType->addItem(tr("所有"), 0);
    while(q.next()){
        int code = q.value(0).toInt();
        QString name = q.value(1).toString();
        ui.cmbBookType->addItem(name,code);
    }

    //model1 = new QSqlQueryModel;
    model2 = new QSqlQueryModel;
    model = new QSqlQueryModel;
    fmodel = new QStandardItemModel;
    smodel = new QStandardItemModel;
    smmodel = new QStandardItemModel;

    //初始化日期为当前日期
    ui.dateEdit->setDate(QDate::currentDate());
    int year = QDate::currentDate().year();
    int month = QDate::currentDate().month();

    //初始化汇率表
    //initRates(year,month);
}

//将指定年月的汇率保存到汇率表中
void CollectPzDialog::initRates(int year,int month)
{
    QSqlQuery q;
    bool r;

    rates.clear();

    QString s = QString("select code,sign,name from MoneyTypes where "
                        "code != 1");
    r = q.exec(s);
    QHash<int,QString> msHash; //货币代码到货币符号的映射
    while(q.next()){
        msHash[q.value(0).toInt()] = q.value(1).toString();
    }

    QList<int> mtCodes = msHash.keys();
    s = QString("select ");
    for(int i = 0; i<mtCodes.count(); ++i){
        s.append(msHash[mtCodes[i]]).append("2rmb,");
    }
    s.chop(1);
    s.append(" from ExchangeRates ");
    s.append(QString("where (year = %1) and (month = %2)").arg(year).arg(month));
    r = q.exec(s);
    if(!q.first()){
        qDebug() << tr("没有指定汇率！");
        return;
        //这里也可以在没有指定汇率的情况下读取默认汇率，即年月数为0的条目
    }
    for(int i = 0;i<mtCodes.count();++i){
        rates[mtCodes[i]] = q.value(i).toDouble();
    }
    rates[RMB] = 1.00;

    //如有需要，在界面上显示所有币种的汇率，而不是现在的只显示单一的美元汇率
}

void CollectPzDialog::dateChanged(const QDate &date)
{
    int year = date.year();
    int month = date.month();
//    QSqlQuery query;
//    QString s = QString("select usd2rmb from ExchangeRates where (year = %1) and (month = %2)").arg(year).arg(month);
//    if(query.exec(s) && query.first()){
//        rate = query.value(0).toDouble();
//        ui.lblRate->setText(QString("%1").arg(rate));
//    }
    initRates(year,month);
    fmodel->clear();
    smodel->clear();
    smmodel->clear();

    ui.btnSave->setEnabled(false);

}

//初始化在汇总计算时要用到的一些哈希表
void CollectPzDialog::initHashs()
{
    bool r;
    QSqlQuery q;
    QString s;

    //初始化一级科目ID到名称/科目代码的映射表和需要进行明细核算的id集合
    r = q.exec("select id,subCode,subName,isReqDet from FirSubjects");
    chash.clear();
    fhash.clear();
    detSubSet.clear();
    while(q.next()){
        int id = q.value(0).toInt();
        chash[id] = q.value(1).toString();
        fhash[id] = q.value(2).toString();
        bool isReq = q.value(3).toBool();
        if(isReq) //如果此科目需要进行明细核算则加入集合
            detSubSet.insert(id);
    }

    //初始化明细科目id值到科目名称的映射表
    //仅对那些需要今年进行明细核算的一级科目下的二级科目
    QList<int> idLst = detSubSet.toList();
    qSort(idLst.begin(),idLst.end());  //仅仅是为了调试方便
    for(int i = 0; i < idLst.count(); ++i){
        s = QString("select FSAgent.id,SecSubjects.subName from FSAgent "
                    "join SecSubjects where (FSAgent.sid = SecSubjects.id) "
                    "and (FSAgent.fid = %1)").arg(idLst[i]);
        r = q.exec(s);
        while(q.next()){
            int id = q.value(0).toInt();
            QString name = q.value(1).toString();
            shash[id] = name;
        }
    }

    //初始化币种代码到名称的映射表
    s = "select code,name from MoneyTypes";
    r = q.exec(s);
    while(q.next()){
        mtMap[q.value(0).toInt()] = q.value(1).toString();
    }



}

//当在总账科目合计列表中单击科目名称时，显示该科目的明细科目的合计信息
void CollectPzDialog::selSubject(const QModelIndex &index)
{
    QString s;
    QSqlQuery q;
    bool r;

    int row = index.row();
    int col = index.column();
    //单击科目名列但排除最后一行的合计值
    if((col == 0) && (row != fmodel->rowCount() -1)){
        int fid = fmodel->data(fmodel->index(row,5)).toInt();
        //仅对需要进行明细核算的科目进行处理
        if(detSubSet.contains(fid)){
            smodel->clear();
            smmodel->clear();
            //获取该科目下的所有明细科目id值集合
            QSet<int> ids;
            s = QString("select id from FSAgent where fid = %1").arg(fid);
            r = q.exec(s);
            while(q.next())
                ids.insert(q.value(0).toInt());

            //遍历明细科目合计值表，将属于当前一级科目下的明细科目金额按币种
            //和明细科目Id显示出来，并按币种进行合计
            QStandardItem *itemName,*itemMt,*itemSum;
            QList<QStandardItem*> items;
            QHash<int,double> sumByMt;  //按币种存放合计值
            QHashIterator<int,double> it(subSums);
            while(it.hasNext()){
                it.next();
                int key = it.key();
                int id = key / 10;
                if(ids.contains(id)){
                    items.clear();
                    int mcode = key % 10;
                    itemName = new QStandardItem(shash[id]);
                    itemMt = new QStandardItem(QString::number(mcode));
                    itemSum = new QStandardItem(QString::number(it.value(),'f',2));
                    items<<itemName<<itemMt<<itemSum;
                    smodel->appendRow(items);
                    sumByMt[mcode] +=it.value();
                }
            }
            smodel->setHeaderData(0,Qt::Horizontal,tr("科目名"));
            smodel->setHeaderData(1,Qt::Horizontal,tr("币种"));
            smodel->setHeaderData(2,Qt::Horizontal,tr("合计金额"));
            ui.tvDetails->setModel(smodel);
            ui.tvDetails->setColumnWidth(0,140);

            iTosItemDelegate* mtDelegate = new iTosItemDelegate(mtMap);
            ui.tvDetails->setItemDelegateForColumn(1,mtDelegate);

            //对明细科目按币种显示合计值
            QHashIterator<int,double> its(sumByMt);
            while(its.hasNext()){
                its.next();
                items.clear();
                itemMt = new QStandardItem(QString::number(its.key()));
                itemSum = new QStandardItem(QString::number(its.value(),'f',2));
                items<<itemMt<<itemSum;
                smmodel->appendRow(items);
            }
            smmodel->setHeaderData(0,Qt::Horizontal,tr("币种"));
            smmodel->setHeaderData(1,Qt::Horizontal,tr("合计金额"));
            ui.tvSums->setModel(smmodel);
            ui.tvSums->setItemDelegateForColumn(0,mtDelegate);
        }

    }
}

//计算本期发生的所有科目的合计值
void CollectPzDialog::calBtnClicked()
{
    //计算任务：汇总所有总账科目、明细科目，并分门别类地显示。左边为总账科目，
    //右边为明细科目。单击总账科目（如果该科目需要进行明细核算），则相应显示明细科目

    //每次计算前都必须初始化汇率表，要确保计算时使用的汇率与指定的月份对应
    int year = ui.dateEdit->date().year();
    int month = ui.dateEdit->date().month();
    initRates(year,month);

    int bt = ui.cmbBookType->itemData(ui.cmbBookType->currentIndex()).toInt();

    QString d = ui.dateEdit->date().toString(Qt::ISODate);
    d.chop(3);
    QString s;

    initHashs();
    if(bt == 0)  //所有凭证
        s = QString("select * from PingZhengs where date like "
                          "'%1%' order by date").arg(d);
    else
        s = QString("select * from PingZhengs where (accBookGroup "
                          "= %1) and (date like '%2%')  order by date")
                .arg(bt).arg(d);
    model->setQuery(s);
    int c = model->rowCount();
    if(c > 0){
        //一级科目借贷合计值表
        QHash<int,double> fjsums;  //key为fid
        QHash<int,double> fdsums;

        int pid,fid,sid,ac,mtype;
        double jv,dv;  //业务活动的借贷金额
        int dir; //业务活动借贷方向

        //遍历所有凭证
        for(int i = 0; i < c; ++i){
            pid = model->data(model->index(i, PZ_ID)).toInt();
            s = QString("select * from BusiActions where pid = %1").arg(pid);
            model2->setQuery(s);
            ac = model2->rowCount(); //业务活动总数
            if(ac > 0){                
                //遍历该凭证下的所有的业务活动
                for(int j = 0; j < ac; ++j){
                    fid = model2->data(model2->index(j, BACTION_FID)).toInt();
                    sid = model2->data(model2->index(j, BACTION_SID)).toInt();
                    mtype = model2->data(model2->index(j, BACTION_MTYPE)).toInt();
                    dir = model2->data(model2->index(j, BACTION_DIR)).toInt();
                    if(dir == DIR_J){  //发生在借方
                        jv = model2->data(model2->index(j, BACTION_JMONEY)).toDouble();
                        fjsums[fid] += jv * rates[mtype];
                        //这是为了确保两个表的key集合包含等同的元素，以方便后续的显示操作
                        if(!fdsums.contains(fid))
                            fdsums[fid] = 0;
                        //仅对需要进行明细核算的科目进行明细科目的合计计算
                        if(detSubSet.contains(fid))
                            subSums[genKey(sid,mtype)] += jv;
                        //subSums[CompositeKeyType(sid,mtype)] += jv;
                    }
                    else{     //发生在贷方
                        dv = model2->data(model2->index(j, BACTION_DMONEY)).toDouble();
                        fdsums[fid] += dv * rates[mtype];
                        if(!fjsums.contains(fid))
                            fjsums[fid] = 0;
                        if(detSubSet.contains(fid))
                            subSums[genKey(sid,mtype)] -= dv;
                    }

                    //name = fhash.value(fid);
                    //pzcp.setValue(fid, name, jv, dv);
                }                
            }
        }

        //将总账科目的汇总数据装配到模型中以显示在表格中
        fmodel->clear();
        //QList<int> fidLst = fjsums.keys();
        //要求是按科目类别进行排序，这样做的前提是科目id的大小顺序与科目代码一致
        //qSort(fidLst.begin(), fidLst.end());

        //一种更健壮的方法是根据科目代码对id值进行排序
        QList<int> fidLst;
        s = QString("select id,subCode from FirSubjects order by subCode");
        QSqlQuery q;
        bool r = q.exec(s);
        while(q.next()){
            int id = q.value(0).toInt();
            QString code = q.value(1).toString();
            if(fjsums.contains(id))
                fidLst.append(id);
        }

        QList<QStandardItem*> itemLst;
        double jsum=0;//借贷合计数
        double dsum=0;
        QStandardItem *itemSName,*itemCode,*itemJSum,*itemDSum,*itemSum,*itemId;
        //fsums.clear();
        for(int i = 0; i < fidLst.count(); ++i){
            itemLst.clear();
            int id = fidLst[i];
            //fsums[id] = fjsums[id]-fdsums[id];
            itemSName = new QStandardItem(fhash[id]);
            itemCode = new QStandardItem(chash[id]);
            itemJSum = new QStandardItem(QString::number(fjsums[id],'f',2));
            itemDSum = new QStandardItem(QString::number(fdsums[id],'f',2));
            itemSum = new QStandardItem(QString::number(fjsums[id]-fdsums[id],'f',2));
            itemId = new QStandardItem(QString::number(id));
            itemLst<<itemSName<<itemCode<<itemJSum<<itemDSum<<itemSum<<itemId;
            fmodel->appendRow(itemLst);
            jsum+=fjsums[id];
            dsum+=fdsums[id];
        }

        //加入合计行
        itemLst.clear();
        itemSName = new  QStandardItem(tr("合计"));
        itemCode = new  QStandardItem();
        itemJSum = new  QStandardItem(QString::number(jsum,'f',2));
        itemDSum = new  QStandardItem(QString::number(dsum,'f',2));
        itemSum = new  QStandardItem(QString::number(jsum-dsum,'f',2));
        itemLst<<itemSName<<itemCode<<itemJSum<<itemDSum<<itemSum;
        fmodel->appendRow(itemLst);

        fmodel->setHeaderData(0,Qt::Horizontal,tr("科目名"));
        fmodel->setHeaderData(1,Qt::Horizontal,tr("科目代码"));
        fmodel->setHeaderData(2,Qt::Horizontal,tr("借方合计"));
        fmodel->setHeaderData(3,Qt::Horizontal,tr("贷方合计"));
        fmodel->setHeaderData(4,Qt::Horizontal,tr("合计"));
        ui.tvTSubs->setModel(fmodel);
    }
    else{
        QMessageBox::information(0, tr("一般信息"), tr("还没有任何凭证"));
    }

    //仅在汇总所有凭证时，该按钮才有效
    if(bt == 0)
        ui.btnSave->setEnabled(true);

}

//将汇总计算的结果导出到Excel文件中
void CollectPzDialog::toExcel()
{
//    QFileDialog* dlg = new QFileDialog(this);
//    dlg->setConfirmOverwrite(false);
//    dlg->setAcceptMode(QFileDialog::AcceptSave);
//    dlg->setDefaultSuffix("xls");
//    QString curPath = QDir::currentPath();
//    curPath.append("/").append("outExcels");
//    curPath = QDir::toNativeSeparators(curPath);
//    dlg->setDirectory(curPath);
//    QString fname;
//    if(dlg->exec() == QDialog::Accepted){
//        QStringList flst = dlg->selectedFiles();
//        if(flst.count()==1){
//            fname = QDir::toNativeSeparators(flst[0]);
//            fname = fname.replace("\\", "\\\\");
//            //fname = tr(fname.toLocal8Bit());
//            //fname = tr("D:\\AccTest\\AccTest1-build-desktop\\outExcels\\汇总数据.xls");
//        }
//        else
//            return;
//    }

//    //将模板文件拷贝到用户指定的目录中
//    QString sf = QDir::currentPath().append("/").append("templates/subjectCollection.xls");

//    bool r = QFile::copy(sf, fname);
//    if(!r){
//        if(QMessageBox::Yes == QMessageBox::information(this,
//                                tr("确认信息"), tr("文件已存在，覆盖吗？"),
//                                 QMessageBox::Yes | QMessageBox::No)){
//            QFile::remove(fname);
//            QFile::copy(sf, fname);
//        }
//        else
//            return;
//    }


//#ifdef Q_OS_WIN32
//    QAxObject* excel = new QAxObject("Excel.Application", this); //获取Excel对象
//    excel->dynamicCall("SetVisible(bool)", TRUE); //设置为可见
//    //得到Workbooks集合的指针
//    QAxObject *workbooks = excel->querySubObject( "Workbooks" );

//    //创建一个新的工作簿
//    //QAxObject *workbook = workbooks->querySubObject( "Add()", QVariant());

//    //打开模板文档文档
//    //QString fname = "d:\\c.xls";
//    //QString fname = tr("D:\\AccTest\\AccTest1-build-desktop\\outExcels\\汇总数据.xls");
//    QAxObject *workbook = workbooks->querySubObject( "Open(const QString&)", fname);
//    //得到第一个工作表对象指针
//    //QAxObject *sheets = workbook->querySubObject( "Sheets" );
//    QAxObject *sheet = workbook->querySubObject("Worksheets(int)", 1);
//    sheet->setProperty("Name", QString(tr("总账科目")));


//    //得到名为 stat的一个sheet的指针
//    //QAxObject *StatSheet = sheets->querySubObject( "Item(const QVariant&)", QVariant("stat") );
//    //StatSheet->dynamicCall( "Select()" ); //选择名为 stat的sheet，使之可见

//    //导出本期发生的总账科目的金额数
//    QAxObject *cell;
//    QString cs;
//    for(int r = 0; r < fmodel->rowCount(); ++r)
//        for(int c = 0; c < 5; ++c){
//            cs = QString("Cells(%1,%2)").arg(r+2).arg(c+1);
//            cell = sheet->querySubObject(cs.toLatin1());
//            cell->setProperty("Value", fmodel->data(fmodel->index(r,c)));
//        }

//    //导出本期发生的明细科目的金额数
//    //仅将所有需要明细核算的科目按序分别输出，第一行为总账科目名，第二行为表的标题，
//    //从第三行开始，是所属总账科目下的明细科目的发生金额合计
//    sheet = workbook->querySubObject("Worksheets(int)", 2);
//    sheet->setProperty("Name", QString(tr("明细科目")));

//    int row = 1;
//    QList<int> fidLst = detSubSet.toList();
//    for(int i = 0; i < fidLst.count(); ++i){
//        QString subName = fhash.value(fidLst[i]);
//        cs = QString("Cells(%1,1)").arg(row);
//        cell = sheet->querySubObject(cs.toLatin1());
//        cell->setProperty("Value", subName);
//        //设置为粗体（办法还未知）

//        //设置行标题
//        row++;
//        cell = sheet->querySubObject(QString("Cells(%1,1)").arg(row).toLatin1());
//        cell->setProperty("Value", tr("科目名"));
//        cell = sheet->querySubObject(QString("Cells(%1,2)").arg(row).toLatin1());
//        cell->setProperty("Value", tr("币种"));
//        cell = sheet->querySubObject(QString("Cells(%1,3)").arg(row).toLatin1());
//        cell->setProperty("Value", tr("合计金额"));

//        row++;

//        //输出数据
//        //获取该科目下的所有明细科目id值列表
//        QList<int> ids;
//        QString s = QString("select id from FSAgent where fid = %1").arg(fidLst[i]);
//        QSqlQuery q;
//        r = q.exec(s);
//        while(q.next())
//            ids.append(q.value(0).toInt());
//        QList<int> mts = mtMap.keys(); //币种代码的列表
//        for(int j = 0; j < ids.count(); ++j)
//            for(int k = 0; k < mts.count(); ++k){
//                int key = ids[j] * 10 + mts[k];
//                if(subSums.contains(key)){
//                    cell = sheet->querySubObject(QString("Cells(%1,1)").arg(row).toLatin1());
//                    cell->setProperty("Value", shash.value(ids[j]));
//                    cell = sheet->querySubObject(QString("Cells(%1,2)").arg(row).toLatin1());
//                    cell->setProperty("Value", mtMap.value(mts[k]));
//                    cell = sheet->querySubObject(QString("Cells(%1,3)").arg(row).toLatin1());
//                    cell->setProperty("Value", subSums.value(key));
//                    row++;
//                }
//            }
//        row += 2;  //空二行
//    }



//    //range->setProperty("Value", 200);
////    QAxObject *range = StatSheet->querySubObject( "Range(const QVariant&)",
////    QVariant( QString("A1:A1")));  //选择A1:A1 这个 range 对象
////    range->dynamicCall( "Clear()" ); //  清除 range对象
////    range->dynamicCall( "SetValue(const QVariant&)", QVariant(5) ); //将该range对象的值设为5
//#endif
}

//保存当前会计期间计算的科目余额到表SubjectExtras中
void CollectPzDialog::saveExtras()
{
    //任务需要：将前期的余额加上当期发生的合计值，并保存到两个余额表中
    QSqlQuery q, q2;
    QString s;
    bool r;

    //首先检测是否存在前期的余额，如果没有则报告并退出
    int year = ui.dateEdit->date().year();
    int month = ui.dateEdit->date().month();
    int y,m;
    if(month == 1){
        m = 12;
        y = year - 1;
    }
    else{
        m = month - 1;
        y = year;
    }

    s = QString("select * from SubjectExtras where (year = %1)"
                " and (month = %2)").arg(y).arg(m);
    r = q.exec(s);

    bool isNew = false;  //是否保存的是第一个月的余额数据
    if(!q.first()){
        QString info = QString(tr("数据库中没有%1年%2月的余额记录！\n"))
                       .arg(year).arg(month);
        info.append(tr("如果是第一个月的数据则选yes，否则说明数据库记录不完整\n"
                       "缺少前一个月的数据，请选择no"));
        if(QMessageBox::No == QMessageBox::question(this,
                        tr("询问信息"),info,
                        QMessageBox::Yes | QMessageBox::No))
            return;
        else
            isNew = true;
    }

    //提取保存前期总账科目余额的记录id值，在后面保存明细科目余额是有用
    int preId;
    if(!isNew)
        preId = q.value(0).toInt();

    //读取前期总账余额值保存到prevExtras
    QHash<QString,double> prevExtras; //键为一级科目类别的字母代码加科目代码
    QSqlRecord recInfo = q.record();
    if(!isNew){
        double v = 0;
        for(int i = SE_SUBSTART; i < recInfo.count(); i++){
            QString name = recInfo.fieldName(i);
            v = q.value(i).toDouble();
            prevExtras[name] = v;
        }
    }
    else
        for(int i = SE_SUBSTART; i < recInfo.count(); i++){
            QString name = recInfo.fieldName(i);
            prevExtras[name] = 0;
        }


    //从fmodel中读取当期总账科目发生额并累加到前期总账余额prevExtras
    //QHash<QString,double> curExtras;
    for(int i = 0; i < fmodel->rowCount() - 1; ++i){
        QString code = fmodel->data(fmodel->index(i, 1)).toString();
        //int t = code.left(1).toInt();
        char t = 'A' + code.left(1).toInt() - 1;
        QString name = QString("%1%2").arg(t).arg(code);
        double v = fmodel->data(fmodel->index(i,4)).toDouble();
        prevExtras[name] += v;
    }

    //保存当期总账余额到SubjectExtras表中
    //首先检测是否已经有当期的余额记录，如有则删除
    r = q.exec(QString("select id from SubjectExtras where (year = %1)"
                   " and (month = %2)").arg(year).arg(month));
    if(r){
        while(q.next()){
            int id = q.value(0).toInt();
            r = q2.exec(QString("delete from SubjectExtras where id = %1").arg(id));
        }
    }

    //插入新的当前总账余额记录
    //获取余额表的字段信息
    QList<QString> fnList; //要绑定值的字段名列表（有insert语句中由冒号前导的符号）
    QString fnStr = "(year, month, state,";           //SQL 的insert语句中的表字段部分
    QString vStr = "values(:year, :month, :state, ";  //values 部分
    QList<QString> keys = prevExtras.keys();
    qSort(keys.begin(),keys.end());
    for(int i = 0; i < keys.count(); ++i){
        QString fname = keys[i];
        fnStr.append(fname);
        fnStr.append(", ");
        vStr.append(":");
        vStr.append(fname);
        vStr.append(", ");
        fnList.append(fname);
    }
    fnStr.chop(2);
    fnStr.append(") ");
    vStr.chop(2);
    vStr.append(")");
    s = "insert into SubjectExtras ";
    s.append(fnStr);
    s.append(vStr);
    r = q.prepare(s);

    //绑定年月值
    q.bindValue(":year", year);
    q.bindValue(":month", month);
    q.bindValue(":state", 1);  //结转状态有待考虑
    //绑定总账科目余额值
    for(int i = 0; i < fnList.count(); ++i)
        q.bindValue(":"+fnList[i], prevExtras.value(fnList[i]));

    r = q.exec();

    //提取保存当期总账科目余额的记录id值
    s = QString("select id from SubjectExtras where (year = %1) and "
                "(month = %2)").arg(year).arg(month);
    r = q.exec(s);
    r = q.first();
    int curId = q.value(0).toInt();



    //处理明细科目
    //读取前期的明细科目余额并保存在prevSubSums
    QHash<int,double> prevSubSums; //键 = 明细科目id x 10 + 币种代码

    if(!isNew){
        s = QString("select fsid,mt,value from detailExtras where "
                    "seid = %1").arg(preId);
        r = q.exec(s);
        while(q.next()){
            int fsid = q.value(0).toInt();
            int mt = q.value(1).toInt();
            double v = q.value(2).toDouble();
            int key = fsid*10+mt;
            prevSubSums[key] = v;
        }
    }


    //遍历明细科目合计值表subSums，将当前发生额加入到prevsubSums表中
    QHashIterator<int,double> it(subSums);
    while(it.hasNext()){
        it.next();
        prevSubSums[it.key()] += it.value();
    }

    //将当期的明细余额值保存到detailExtras中
    QHashIterator<int,double> i(prevSubSums);
    while(i.hasNext()){
        i.next();
        bool isExist = false;  //是否存在当期对应的明细科目余额记录
        int id = i.key() / 10;
        int mt = i.key() % 10;
        s = QString("select id from detailExtras where (seid = %1) "
                    "and (fsid = %2) and (mt = %3)")
                .arg(curId).arg(id).arg(mt);
        r = q.exec(s);
        int eid; //detailExtras表的id列
        if(q.first()){
            isExist = true;
            eid = q.value(0).toInt();
        }
        //存在则更新
        if(isExist){
            s = QString("update detailExtras set value = %1 where "
                        "id = %2").arg(i.value()).arg(eid);
        }
        else{//不存在则插入
            s = QString("insert into detailExtras(seid,fsid,mt,value) "
                        "values(%1,%2,%3,%4)")
                    .arg(curId).arg(id).arg(mt).arg(i.value());
            r = q.exec(s);
        }

    }


    ui.btnSave->setEnabled(false);
}

void CollectPzDialog::setDateLimit(int sy,int sm,int ey,int em)
{
    QDate sd(sy,sm,1);
    QDate ed(ey,em,1);
    ed.setDate(ey,em,ed.daysInMonth());
    ui.dateEdit->setDateRange(sd, ed);
}

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
    bool r;
    QString s;
    QSqlQuery q;
    int c;  //tem

    demandAttachDatabase(true);//附加基础数据库到当前连接上

    //删除原来表中的数据
    s = "delete from FstSubClasses";
    r = q.exec(s);
    s = "delete from FirSubjects";
    r = q.exec(s);

    //从基础数据库中导入一级科目类别表
    s = QString("insert into FstSubClasses(code,name) select code,name "
                "from basic.FirstSubCls where subCls = %1").arg(subCls);
    r = q.exec(s);
    c = q.numRowsAffected();

    //从基础数据库中导入一级科目表
    s = QString("insert into FirSubjects(subCode,remCode,belongTo,isView,isReqDet,weight,"
                "subName,description,utils) select subCode,remCode,belongTo,isView,"
                "isReqDet,weight,subName,description,utils from basic.FirstSubs "
                "where subCls = %1").arg(subCls);
    r = q.exec(s);
    c = q.numRowsAffected();

    demandAttachDatabase(false);
}

//从基础数据库中导入二级科目及其类别到相应表中，
//对于二级科目暂不做考虑新旧科目系统的区别
void BasicDataDialog::impSndSubFromBasic()
{
    bool r;
    QString s;
    QSqlQuery q,q2;
    int c;  //tem

    demandAttachDatabase(true);//附加基础数据库到当前连接上

    //删除原来表中的数据
    s = "delete from SndSubClass";
    r = q.exec(s);
    s = "delete from SecSubjects";
    r = q.exec(s);

//    //在FSAgent表中删除除了银行存款和现金以外的所有一级科目下的二级科目的所属关系
    s = QString("select id from FirSubjects where subCode = '1002'");
    r = q.exec(s);
    r = q.first();
    int bankId = q.value(0).toInt();
    s = QString("select id from FirSubjects where subCode = '1001'");
    r = q.exec(s);
    r = q.first();
    int cashId = q.value(0).toInt();
    s = QString("delete from FSAgent where (fid != %1) and "
                "(fid != %2)").arg(cashId).arg(bankId);
    r = q.exec(s);


    //从基础数据库中导入二级科目类别表
    s = QString("insert into SndSubClass(clsCode,name,explain) select clsCode,name,explain "
                "from basic.SecondSubCls");
    r = q.exec(s);
    //c = q.numRowsAffected();

    //从基础数据库中导入二级科目表
    s = QString("insert into SecSubjects(subName,subLName,remCode,classId) "
                "select subName,subLName,remCode,classId "
                "from basic.SecondSubs");
    r = q.exec(s);
    //c = q.numRowsAffected();

    //建立一二级科目的所属关系
    s = QString("select subName,belongTo,subCode from basic.SecondSubs "
                "where subCls = %1").arg(usedSubCls);
    r = q.exec(s);
    while(q.next()){
        QString ownerStr = q.value(1).toString();
        if(ownerStr != ""){
            QString subName = q.value(0).toString();
            QStringList ownerLst = ownerStr.split(",");
            QStringList codeLst = q.value(2).toString().split(",");
            s = QString("select id from SecSubjects where subName = '%1'")
                .arg(subName);
            r = q2.exec(s);
            r = q2.first();
            int sid = q2.value(0).toInt();
            for(int i = 0; i < ownerLst.count(); ++i){
                s = QString("select id from FirSubjects where "
                            "subCode = %1").arg(ownerLst[i]);
                r = q2.exec(s);
                r = q2.first();
                int fid = q2.value(0).toInt();

                //插入到FSAgent表中
                s = QString("insert into FSAgent(fid,sid,subCode) "
                            "values(%1,%2,'%3')").arg(fid).arg(sid)
                        .arg(codeLst[i]);
                r = q2.exec(s);
            }
        }
    }

    demandAttachDatabase(false);
}

//从基础数据库中导入客户数据到SecSubjects表中
void BasicDataDialog::impCliFromBasic()
{
    bool r;
    QString s;
    QSqlQuery q;
    int c;  //tem

    //获取业务客户类别的ID值
    s = QString("select clsCode from SndSubClass where name = '%1'").arg(tr("业务客户"));
    r = q.exec(s);
    r = q.first();
    int clsId = q.value(0).toInt();

    //删除所有原先已有的业务客户
    s = QString("delete from SecSubjects where classId = %1").arg(clsId);
    r = q.exec(s);
    r = q.first();
    c = q.numRowsAffected();

    demandAttachDatabase(true);//附加基础数据库到当前连接上

    s = QString("insert into SecSubjects(subName,subLName,remCode,classId) "
                "select subName,subLName,remCode,%1 from basic.Clients").arg(clsId);
    r = q.exec(s);
    c = q.numRowsAffected();

    demandAttachDatabase(false);
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
        QString code = q.value(FSTSUB_SUBCODE).toString();
        QString remCode = q.value(FSTSUB_REMCODE).toString();
        int belongTo = q.value(FSTSUB_BELONGTO).toInt();
        int isView = q.value(FSTSUB_ISVIEW).toInt();
        int isReqDet = q.value(FSTSUB_ISREQDET).toInt();
        int weight = q.value(FSTSUB_WEIGHT).toInt();
        QString subName = q.value(FSTSUB_SUBNAME).toString();
        QString description  = q.value(FSTSUB_DESC).toString();
        QString utils = q.value(FSTSUB_UTILS).toString();

        if(codes.contains(code)){
            //更新
            s = QString("update basic.FirstSubs set remCode = '%1', "
                        "belongTo = %2, isView = %3, isReqDet = %4, "
                        "weight = %5, subName = '%6', description = '%7', "
                        "utils = '%8' "
                        "where (subCls = %9) and (subCode = '%10')")
                    .arg(remCode).arg(belongTo).arg(isView).arg(isReqDet)
                    .arg(weight).arg(subName).arg(description).arg(utils)
                    .arg(usedSubCls).arg(code);
            r = q2.exec(s);
        }
        else{
            s = QString("insert into basic.FirstSubs(subCls,subCode,"
                        "remCode,belongTo,isView,isReqDet,weight,subName,description,"
                        "utils) values(%1,'%2','%3',%4,%5,%6,%7,'%8','%9','%10')")
                    .arg(usedSubCls).arg(code).arg(remCode).arg(belongTo)
                    .arg(isView).arg(isReqDet).arg(weight).arg(subName)
                    .arg(description).arg(utils);
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
    s = QString("select clsCode from SndSubClass where name = '%1'")
        .arg(tr("金融机构"));
    int bcode;
    if(q.exec(s) && q.first())
        bcode = q.value(0).toInt();
    else{
        qDebug() << tr("没有金融机构二级科目类别");
        return;
    }

    int ccode;
    s = QString("select clsCode from SndSubClass where name = '%1'")
        .arg(tr("业务客户"));
    if(q.exec(s) && q.first())
        ccode = q.value(0).toInt();
    else{
        qDebug() << tr("没有业务客户二级科目类别");
        return;
    }


    s = QString("select * from SecSubjects where (classId != %1) "
                "and (classId != %2)").arg(bcode).arg(ccode);
    r = q.exec(s);
    int num = 0; //完成的记录数
    while(q.next()){
        QString subName = q.value(SNDSUB_SUBNAME).toString();
        QString subLName = q.value(SNDSUB_SUBLONGNAME).toString();
        QString remCode = q.value(SNDSUB_REMCODE).toString();
        int clsId = q.value(SNDSUB_CALSS).toInt();
        int sid = q.value(0).toInt();
        //获取此二级科目被哪些一级科目包含了
        s = QString("select FirSubjects.subCode, FSAgent.subCode from "
                    "FSAgent join FirSubjects where "
            "(FSAgent.fid = FirSubjects.id) and (FSAgent.sid = %1)")
                .arg(sid);
        QStringList fclst,sclst;
        r = q2.exec(s);
        while(q2.next()){
            fclst.append(q2.value(0).toString());//一级科目代码列表
            sclst.append(q2.value(1).toString());//二级科目代码列表
        }
        QString fcodes = fclst.join(","); //包含此二级科目的一级科目代码
        QString scodes = sclst.join(",");

        //更新
        if(names.contains(subName)){
            s = QString("update basic.SecondSubs set subName = '%1',"
                        "subLName = '%2', remCode = '%3', classId = %4, "
                        "belongTo = '%5', subCode = '%6' where (subCls = %7) "
                        "and (subName = '%8')")
                    .arg(subName).arg(subLName).arg(remCode).arg(clsId)
                    .arg(fcodes).arg(scodes).arg(usedSubCls).arg(subName);
            r = q2.exec(s);
        }
        else{ //插入
            s = QString("insert into basic.SecondSubs(subCls, subName, "
                        "subLName, remCode, classId,belongTo,subCode) "
                        "values(%1,'%2','%3','%4',%5,'%6','%7')")
                    .arg(usedSubCls).arg(subName).arg(subLName).arg(remCode)
                    .arg(clsId).arg(fcodes).arg(scodes);
            r = q2.exec(s);
        }
        num++;
        ui.progressBar->setValue(num);
    }
    ui.progressBar->setValue(0);
    demandAttachDatabase(false);
}

////////////////////////////////////////////////////////////////
SubjectExtraDialog::SubjectExtraDialog(QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);

    QSqlQuery q;
    QString name,code, oname;
    QString s;
    bool r;
    int pt = 1;  //因为查询必须严格按科目类别的顺序，当此二者不同时，
    int t;       //程序知道遇到了新的科目类别，必须重置row和col变量
    int row = 0;
    int col = 0;    

    s = "select code, name from FstSubClasses";
    r = q.exec(s);
    if(r){
        int c = 0;
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            ui.tabWidget->setTabText(c, name);
            QGridLayout* lyt = new QGridLayout;
            lytHash[code] = lyt;
            ui.tabWidget->widget(c++)->setLayout(lyt);
        }
        if(c == 5)  //界面上预设了6个页面，但老科目系统只需要5个
            ui.tabWidget->removeTab(c);
    }

    //创建显示科目余额的部件
    s = QString("select id, subCode, subName,belongTo ,isReqDet from "
                "FirSubjects where isView = 1 order by subCode");
    bool result = q.exec(s);
    if(result){
        int id;
        while(q.next()){
            id = q.value(0).toInt();
            QString oname;
            name = q.value(2).toString();  //科目名称
            code = q.value(1).toString();  //科目代码
            t = q.value(3).toInt();        //科目类别
            bool isReqDet = q.value(4).toBool(); //是否需要子目的余额支持

            QChar strCode;   //用大写字符（A-F）表示的科目类别代码
            switch(t){
            case 1:
                strCode = 'A';
                break;
            case 2:
                strCode = 'B';
                break;
            case 3:
                strCode = 'C';
                break;
            case 4:
                strCode = 'D';
                break;
            case 5:
                strCode = 'E';
                break;
            case 6:
                strCode = 'F';
                break;
            }

            if(pt != t){
                row = 0;
                col = 0;
                pt = t;
            }
            QLabel* label;
            QLineEdit* edt = new QLineEdit;
            idHash[id] = edt;

            oname = QString("%1%2").arg(strCode).arg(code);  //对象名（类别代码字母+科目代码）
            edt->setObjectName(oname);
            if(isReqDet){
                label = new ClickAbleLabel(name,code);
                ClickAbleLabel* lp = (ClickAbleLabel*)label;
                connect(lp, SIGNAL(viewSubExtra(QString,QPoint))
                        , this, SLOT(viewSubExtra(QString,QPoint)));
            }
            else{
                label = new QLabel(name);
            }
            lytHash[t]->addWidget(label, row/3, col++ % 6);
            lytHash[t]->addWidget(edt, row/3, col++ % 6);
            edtHash[oname] = edt;
            row++;
        }
    }



    //model = new QSqlTableModel;
    //model->setTable("SubjectExtras");
    //mapper = new QDataWidgetMapper;
    //mapper->setModel(model);
    connect(ui.btnView, SIGNAL(clicked()), this, SLOT(viewExtra()));
    connect(ui.btnEdit, SIGNAL(clicked()), this, SLOT(btnEditClicked()));
    connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(btnSaveClicked()));
    ui.dateEdit->setDate(QDate::currentDate());
    y = ui.dateEdit->date().year();
    m = ui.dateEdit->date().month();

//    //建立表字段与显示部件的关联对应
//    int fi = SE_SUBSTART;
//    int cls;
//    QString fname;

//    r = q.exec("select subCode, belongTo from FirSubjects where isView = 1");
//    if(r){
//        while(q.next()){
//            code = q.value(0).toString(); //科目代码
//            cls = q.value(1).toInt();     //科目类别代码
//            switch ( cls ){
//            case 1:
//                fname = QString("A%1").arg(code);
//                break;
//            case 2:
//                fname = QString("B%1").arg(code);
//                break;
//            case 3:
//                fname = QString("C%1").arg(code);
//                break;
//            case 4:
//                fname = QString("D%1").arg(code);
//                break;
//            case 5:
//                fname = QString("E%1").arg(code);
//                break;
//            case 6:
//                fname = QString("F%1").arg(code);
//                break;
//            }
//            //QLineEdit* edt = this->findChild<QLineEdit* >(fname);
//            QLineEdit* edt = edtHash.value(fname);
//            mapper->addMapping(edt, fi++);
//        }
//    }

}

void SubjectExtraDialog::dateChanged(const QDate &date)
{
    y = date.year();
    m = date.month();
}

void SubjectExtraDialog::setDate(int y, int m)
{
    QDate d(y,m,1);
    ui.dateEdit->setDate(d);
    this->y = y;
    this->m = m;
    viewExtra();
}

//显示明细科目的余额
void SubjectExtraDialog::viewSubExtra(QString code, const QPoint& pos)
{
//    int year = ui.dateEdit->date().year();
//    int month = ui.dateEdit->date().month();

//    QSqlQuery q;
//    QString s = QString("select id from SubjectExtras where (year = %1) "
//                        "and (month = %2)").arg(year).arg(month);
//    if(q.exec(s) && q.first()){
//        DetailExtraDialog* view = new DetailExtraDialog(code, year, month);
//        view->setOnlyView();
//        view->move(pos);
//        view->exec();
//    }
//    else{
//        QString info = QString(tr("数据库中没有%1年&2月的余额值！"))
//                       .arg(year).arg(month);
//        QMessageBox::information(this, tr("提醒信息"),info);
//    }

}

void SubjectExtraDialog::viewExtra()
{
    QHash<int,double> rates;
    if(!BusiUtil::getRates(y,m,rates))
        return;
    rates[RMB] = 1;
    QHash<int,double> pExtra,sExtra; //主、子科目余额值表(还未使用)
    QHash<int,int> pExtDir,sExtDir;  //主、子科目余额方向表
    if(!BusiUtil::readExtraByMonth(y,m,pExtra,pExtDir,sExtra,sExtDir))
        return;

    //计算主科目各币种的合计值
    QHash<int,double> psums;
    QHashIterator<int,double>* ip = new QHashIterator<int,double>(pExtra);
    int id,mt;
    while(ip->hasNext()){
        ip->next();
        id = ip->key() / 10;
        mt = ip->key() % 10;
        psums[id] += (ip->value() * rates.value(mt));
    }

    //将数据输出到部件中显示
    ip = new QHashIterator<int,double>(psums);
    while(ip->hasNext()){
        ip->next();
        idHash.value(ip->key())->setText(QString::number(ip->value(),'f',2));
    }
//    int year = ui.dateEdit->date().year();
//    int month = ui.dateEdit->date().month();
//    QString s = QString("(year = %1) and (month = %2)").arg(year).arg(month);
//    //QString s = QString("select * from SubjectExtras where "
//    //                    "(year = %1) and (month = %2)").arg(year).arg(month);
//    model->setFilter(s);
//    //model->setQuery(s);
//    //mapper->setModel(model);
//    model->select();
//    //int r = model->rowCount();
//    //double v = model->data(model->index(0,SE_SUBSTART)).toDouble();
//    //如果没有对应记录，亚清空显示余额的编辑框（不知道为什么QDataWidgetMapper不能自动在空记录时清空）
//    if(model->rowCount() == 0){
//        QHashIterator<QString, QLineEdit*> i(edtHash);
//        while (i.hasNext()){
//             i.next();
//             i.value()->setText("");
//        }
//    }
//    else
//        mapper->toFirst();
}

void SubjectExtraDialog::btnEditClicked()
{
//    bool state = ui.btnSave->isEnabled();
//    ui.btnSave->setEnabled(!state);
//    if(state)
//        ui.btnEdit->setText(tr("编辑模式"));
//    else
//        ui.btnEdit->setText(tr("显示模式"));

//    QHashIterator<QString, QLineEdit*> i(edtHash);
//    while (i.hasNext()){
//         i.next();
//         i.value()->setReadOnly(state);
//    }
}

void SubjectExtraDialog::btnSaveClicked()
{
    //mapper->submit();
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
    QString s;
    bool r;
    int cid;

    //获取金融机构类客户类别的代码
    s = QString("select clsCode from SndSubClass where name = '%1'")
        .arg(tr("金融机构"));
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
            s = QString("select id from SecSubjects where "
                         "subName = '%1'").arg(sname);

            //如果不存在，才可以进行后续的二次插入操作
            if(q2.exec(s) && !q2.first()){
                s = QString("insert into SecSubjects(subName,subLName,"
                            "classId) values('%1','%2',%3)")
                            .arg(sname).arg(slname).arg(cid);
                r = q2.exec(s);
                //回读刚插入的记录的id
                s = QString("select id from SecSubjects where "
                            "subName = '%1'").arg(sname);
                r = q2.exec(s);
                r = q2.first();
                int sid = q2.value(0).toInt();

                //建立一二级科目的对应关系
                s = QString("insert into FSAgent(fid,sid) "
                            "values(%1,%2)").arg(fid).arg(sid);
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


////////////////////////////////////////////////////////////////
//显示子目余额对话框类
DetailExtraDialog::DetailExtraDialog(QString subCode, int year,
                                     int month, QWidget *parent) : QDialog(parent),
    ui(new Ui::DetailExtraDialog)
{
    ui->setupUi(this);

    this->subCode = subCode;
    this->year = year;
    this->month = month;

    QString s;
    QSqlQuery q, q2;
    bool isBank = false; //是否是银行存款科目
    bool r;
    int seid;  //SubjectExtras表记录的id值
    int fsid;  //在表FSAgent表记录的id值

    //初始化汇率哈希表rates
    s = QString("select code,sign,name from MoneyTypes where code != 1");
    r = q.exec(s);
    QHash<int,QString> msHash; //货币代码到货币符号的映射
    while(q.next()){
        msHash[q.value(0).toInt()] = q.value(1).toString();
    }

    QList<int> mtCodes = msHash.keys();
    s = QString("select ");
    for(int i = 0; i<mtCodes.count(); ++i){
        s.append(msHash[mtCodes[i]]).append("2rmb,");
    }
    s.chop(1);
    s.append(" from ExchangeRates ");
    s.append(QString("where (year = %1) and (month = %2)").arg(year).arg(month));
    r = q.exec(s);
    if(!q.first()){
        qDebug() << tr("没有指定汇率！");
        return;
        //这里也可以在没有指定汇率的情况下读取默认汇率，即年月数为0的条目
    }
    for(int i = 0;i<mtCodes.count();++i){
        rates[mtCodes[i]] = q.value(i).toDouble();
    }

    //获取一级科目的ID值
    s = QString("select id from FirSubjects where subCode = %1")
        .arg(subCode);
    r = q.exec(s);
    r = q.first();
    int fid = q.value(0).toInt();

    //对银行存款下的二级科目需作特别处理
    QHash<int,int> bmHash; //银行存款下二级科目id(FSAgent)到币种代码的映射
    if(subCode == "1002"){
        isBank = true;

        //初始化货币名称到货币代码的映射表
        QHash<QString,int> ntHash;
        s = QString("select code,name from MoneyTypes");
        r = q.exec(s);
        while(q.next()){
            ntHash[q.value(1).toString()] = q.value(0).toInt();
        }

//        //获取银行存款科目的Id值
//        s = "select id from FirSubjects where subCode = '1002'";
//        r = q.exec(s);r = q.first();
//        int bankId = q.value(0).toInt();

//        //获取金融结构类别代码
//        s = QString("select clsCode from SndSubClass where (name = '%1')")
//            .arg(tr("金融机构"));
//        r = q.exec(s);
//        r = q.first();
//        int jrClsId = q.value(0).toInt();

        //初始化银行存款下的所有明细科目的id值到对应币种代码的映射表
        s = QString("select FSAgent.id,SecSubjects.subName from FSAgent "
                    "join SecSubjects where (FSAgent.sid = SecSubjects.id) "
                    "and (FSAgent.fid = %1)").arg(fid);
        r = q.exec(s);
        while(q.next()){
            QString name = q.value(1).toString();
            QString mname = name.mid(name.indexOf("-")+1, name.count()-1);
            int id = q.value(0).toInt();
            bmHash[id] = ntHash.value(mname);
        }
    }





    //初始化总账科目余额在SubjectExtras表中的对应字段名称
    //QString fn;  //一级科目余额字段名
    int t = subCode.left(1).toInt(); //一级科目类别代码
    char ts = 'A' + t -1;
    tsName.append(ts).append(subCode);

    //获取在SubjectExtras表中保存指定年月一级科目余额的记录ID值
    s = QString("select id, %1 from SubjectExtras where (year = %2) "
                "and (month = %3)").arg(tsName).arg(year).arg(month);
    if(!q.exec(s) || !q.first()){
        qDebug() << tr("查询总帐科目余额时失败！");
        return;
    }

    QList<int> alst; //仅包含单一币种代码的列表和包含所有币种代码的列表
    s = "select code from MoneyTypes";
    r = q2.exec(s);
    while(q2.next())
        alst.append(q2.value(0).toInt());
    //slst.append(1);  //人民币的代码永为1

    model = new QStandardItemModel;
    QList<QStandardItem*> items;

    //获取指定年月的总账科目余额值在SubjectExtras表中的记录的id值
    seid = q.value(0).toInt();
    oldValue = q.value(1).toDouble();
    ui->edtTotal->setText(QString::number(oldValue, 'f', 2));



    //获取该一级科目下所属的所有二级科目
    s = QString("select  FSAgent.id, FSAgent.isDetByMt, SecSubjects.subName "
                "from FSAgent join SecSubjects where (SecSubjects.id = "
                "FSAgent.sid) and (FSAgent.fid = %1)").arg(fid);
    r = q.exec(s);
    double sum = 0;    
    while(q.next()){
        bool isNew;  //是否在detailExtras中有对应的子目余额记录条目
        //mtlst.clear();

        fsid = q.value(0).toInt();
        bool isDetByName = q.value(1).toBool();  //是否按币种分子目
        QString name = q.value(2).toString();

        QList<int> mtlst;
        if(isDetByName)
            mtlst.append(alst);
        else if(isBank)
            mtlst.append(bmHash.value(fsid));
        else
            mtlst.append(1);  //对于其他科目，只考虑人民币

        for(int i = 0; i < mtlst.count(); ++i){
            items.clear();
            //从detailExtras表中读取余额值
            double value = 0;
            //int mt; //币种代码
            s = QString("select mt,value from detailExtras where "
                        "(seid = %1) and (fsid = %2) and (mt = %3)")
                    .arg(seid).arg(fsid).arg(mtlst[i]);
            if(q2.exec(s) && q2.first()){
                isNew = false;
                //mt = q2.value(0).toInt();
                value = q2.value(1).toDouble();
            }
            else{
                isNew = true;
                //mt = 1;  //默认为人民币
            }

            if(mtlst[i] == 1)
                sum += value;
            else
                sum += (value * rates.value(mtlst[i]));

            QStandardItem* itemName = new QStandardItem(name);
            QStandardItem* itemmt = new QStandardItem(QString::number(mtlst[i]));
            QStandardItem* itemv;
            if(value == 0)
                itemv = new QStandardItem;
            else
                itemv = new QStandardItem(QString::number(value, 'f', 2));
            QStandardItem* itemIsNew;
            if(isNew)
                itemIsNew = new QStandardItem("true");
            else
                itemIsNew = new QStandardItem("false");
            QStandardItem* itemSeid = new QStandardItem(QString::number(seid));
            QStandardItem* itemFsid = new QStandardItem(QString::number(fsid));
            items << itemName << itemmt  << itemv << itemIsNew
                    << itemSeid << itemFsid;
            model->appendRow(items);
        }
    }
    //增加合计行
    items.clear();
    QStandardItem* itemNull = new QStandardItem;
    QStandardItem* itemSumTitle = new QStandardItem(tr("合计"));
    QStandardItem* itemSum = new QStandardItem(QString::number(sum, 'f', 2));
    items <<itemSumTitle << new QStandardItem(QString::number(1))
          << itemSum <<itemNull <<itemNull << itemNull;
    model->appendRow(items);
    items.clear();

    model->setHeaderData(0, Qt::Horizontal, tr("科目名"));
    model->setHeaderData(1, Qt::Horizontal, tr("币种"));
    model->setHeaderData(2, Qt::Horizontal, tr("余额值"));
    ui->tview->setModel(model);

    ui->tview->setColumnWidth(0, 170);
    ui->tview->setColumnWidth(1, 80);
    ui->tview->setColumnWidth(2, 100);

    ui->tview->hideColumn(3);
    ui->tview->hideColumn(4);
    ui->tview->hideColumn(5);

    s = "select * from MoneyTypes";
    r = q.exec(s);
    QMap<int,QString> mtMap;
    while(q.next()){
        mtMap[q.value(MT_CODE).toInt()] = q.value(MT_NAME).toString();

    }

    //应该使用一种非选择性的代理类
    iTosItemDelegate* mtDelegate = new iTosItemDelegate(mtMap);
    ui->tview->setItemDelegateForColumn(1, mtDelegate);

    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));
}

DetailExtraDialog::~DetailExtraDialog()
{
    delete ui;
}

//当子目余额对话框仅用于显示子目余额时，应调整其中的某些界面元素
void DetailExtraDialog::setOnlyView()
{
    ui->lbl1->setText(tr("明细科目余额"));
    ui->lbl2->setVisible(false);
    ui->edtTotal->setVisible(false);
    ui->btnSave->setVisible(false);
    ui->btnCancel->setText(tr("关闭"));
}

//保存子目余额
void DetailExtraDialog::saveDetailExtra()
{
    QSqlQuery q;
    QString s;
    bool r;

    double sum = 0;
    for(int i = 0; i < model->rowCount()-1; ++i){
        bool isNew = model->data(model->index(i, 3)).toBool();
        int seid = model->data(model->index(i, 4)).toInt();
        int fsid = model->data(model->index(i, 5)).toInt();
        int mt = model->data(model->index(i,1)).toInt();
        double v = model->data(model->index(i, 2)).toDouble();
        //仅对有数值的明细科目创建新记录，这样可以减少存储量
        if(isNew && (v != 0)){
            s = QString("insert into detailExtras(seid,fsid,mt,value) "
                        "values(%1,%2,%3,%4)")
                    .arg(seid).arg(fsid).arg(mt).arg(v);
            r = q.exec(s);
        }
        else{
            s = QString("update detailExtras set value = %1 where "
                        "(seid = %2) and (fsid = %3) and (mt = %4)")
                    .arg(v).arg(seid).arg(fsid).arg(mt);
            r = q.exec(s);
        }

        //求所有明细科目的合计值
        if(mt == 1)
            sum += v;
        else
            sum += (v * rates.value(mt));
    }

    //更新总账科目的余额
    double zv = ui->edtTotal->text().toDouble();
    if(zv != sum){
        s = QString("update SubjectExtras set %1 = %2 where "
                    "(year = %3) and (month = %4)")
                .arg(tsName).arg(sum).arg(year).arg(month);
        if(!q.exec(s))
            qDebug() << tr("更新总账科目余额时失败！");
        emit extraValueChanged();
    }
    close();
}

//当明细科目的余额值改变时重新计算合计值
void DetailExtraDialog::dataChanged(const QModelIndex &topLeft,
                                    const QModelIndex &bottomRight)
{
    int row = topLeft.row();
    int col = topLeft.column();
    int rows = model->rowCount();
    if((col == 2) and (row < rows-1)){
        double sum = 0;
        for(int i = 0; i < rows - 1; ++i){
            int mt = model->data(model->index(i, 1)).toInt();
            if(mt == 1)
                sum += model->data(model->index(i, 2)).toDouble();
            else  //如果是外币，则要转换为人民币后再累加
                sum += (model->data(model->index(i, 2)).toDouble() * rates[mt]);
        }

        model->setData(model->index(rows - 1, 2), sum);

    }
}






