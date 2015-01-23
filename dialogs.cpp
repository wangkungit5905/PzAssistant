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
    AppConfig* conf = AppConfig::getInstance();
    accList = conf->getAllCachedAccounts();
    states = conf->getAccTranStates();
    for(int i = 0; i < accList.count(); ++i)
        accountLst << accList.at(i)->accName;
    model = new QStringListModel(accountLst);
    ui.accountList->setModel(model);
    connect(ui.accountList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(itemClicked(QModelIndex)));
    connect(ui.accountList, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(doubleClicked(QModelIndex)));
}

void OpenAccountDialog::itemClicked(const QModelIndex &index)
{
    ui.buttonBox->button(QDialogButtonBox::Open)->setEnabled(true);
    selAcc = index.row();
    AccountCacheItem* ci = accList.at(selAcc);
    ui.lblFName->setText(ci->fileName);
    ui.lblCode->setText(ci->code);
    ui.lname->setText(ci->accLName);
    ui.edtOutTime->setText(ci->outTime.toString(Qt::ISODate));
    if(ci->tState == ATS_TRANSOUTED){
        ui.lblWS->setText(tr("转出站名"));
        ui.edtMac->setText(ci->s_ws->name());
        ui.edtTranState->setText(tr("已转出至“%1”").arg(ci->d_ws->name()));
    }
    else{
        ui.lblWS->setText(tr("来源站名"));
        ui.edtMac->setText(ci->s_ws->name());
        if(ci->tState == ATS_TRANSINDES)
            ui.edtTranState->setText(tr("已转入目的站"));
        else
            ui.edtTranState->setText(tr("已转入非目的站"));
    }
    //ui.edtTranState->setText(states.value(ci->tState));
    ui.edtInTime->setText(ci->inTime.toString(Qt::ISODate));
    ui.buttonBox->button(QDialogButtonBox::Open)->setEnabled(true);
}

void OpenAccountDialog::doubleClicked(const QModelIndex &index)
{
    itemClicked(index);
    accept();
}

AccountCacheItem *OpenAccountDialog::getAccountCacheItem()
{
    if(selAcc >= 0 && selAcc < accList.count())
        return accList.at(selAcc);
    else
        return NULL;
}

////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////





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


