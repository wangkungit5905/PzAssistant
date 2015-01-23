#include "nabaseinfodialog.h"
#include "ui_nabaseinfodialog.h"
#include "account.h"
#include "config.h"
#include "myhelper.h"

#include <QFileDialog>


NABaseInfoDialog::NABaseInfoDialog(QWidget *parent) :
    QDialog(parent),ui(new Ui::NABaseInfoDialog)
{
    ui->setupUi(this);
    init();
}

NABaseInfoDialog::~NABaseInfoDialog()
{
    delete ui;
}


void NABaseInfoDialog::init()
{
    AppConfig* cfg = AppConfig::getInstance();
    cfg->getSubSysItems(supportSubSys);
    foreach(SubSysNameItem* item, supportSubSys)
        ui->cmbSubSys->addItem(item->name);
    ui->edtStartDate->setDateTime(QDateTime::currentDateTime());    
    ui->btnSelFile->setVisible(false);
    foreach (AccountCacheItem* item, cfg->getAllCachedAccounts())
        usedCodes<<item->code;
    qSort(usedCodes.begin(),usedCodes.end());
    int code = usedCodes.last().toInt()+1;
    ui->edtCode->setText(QString::number(code));
}

void NABaseInfoDialog::on_btnOk_clicked()
{
    QString code = ui->edtCode->text();
    if(code.isEmpty()){
        QMessageBox::warning(this,"",tr("必须为新账户提供唯一的代码！"));
        return;
    }
    foreach(QString c, usedCodes){
        if(code == c){
            QMessageBox::warning(this,"",tr("账户代码与现存账户有冲突！"));
            return;
        }
    }
    QString sname = ui->edtSName->text();
    if(sname.isEmpty()){
        QMessageBox::warning(this,"",tr("必须为新账户提供简称！"));
        return;
    }
    QString lname = ui->edtLName->text();
    if(lname.isEmpty()){
        QMessageBox::warning(this,"",tr("必须为新账户提供全称！"));
        return;
    }
    QString fileName = ui->edtFileName->text();
    if(fileName.isEmpty())
        fileName = sname + ".dat";
    else if(!fileName.endsWith(".dat",Qt::CaseInsensitive)){
        int index = fileName.lastIndexOf(".");
        if(index != -1)
            fileName = fileName.left(index);
        fileName = fileName + ".dat";
    }
    if(ui->cmbSubSys->currentIndex() == -1){
        QMessageBox::warning(this,"",tr("必须为新账户选择所采用的科目系统！"));
        return;
    }
    SubSysNameItem* subSys = supportSubSys.at(ui->cmbSubSys->currentIndex());
    int startYear = ui->edtStartDate->date().year();
    int startMonth = ui->edtStartDate->date().month();
    QString errorInfos;



    if(!Account::createNewAccount(fileName,code,sname,lname,subSys,startYear,startMonth,errorInfos)){
        QMessageBox::critical(this,"",tr("创建新账户失败！可能由于以下原因：\n%1").arg(errorInfos));
        return;
    }

    //将新账户导入到本地缓存
    AccountCacheItem* item= new AccountCacheItem;
    item->id = UNID;
    item->code = code;
    item->accName = sname;
    item->accLName = lname;
    item->fileName = fileName;
    item->lastOpened = true;
    item->tState = ATS_TRANSINDES;
    item->inTime = QDateTime::currentDateTime();
    item->outTime = QDateTime::currentDateTime();
    Machine* locMac = AppConfig::getInstance()->getLocalStation();
    item->s_ws = locMac;
    item->d_ws = locMac;
    if(!AppConfig::getInstance()->saveAccountCacheItem(item)){
        QMessageBox::critical(this,"",tr("将新账户保存到本地缓存时发生错误！"));
        return;
    }
    QMessageBox::information(this,tr("账户创建成功"),tr("在实际记账前，请先打开账户属性配置窗口，配置账户的其他属性，比如科目和期初数据！"));
    accept();
}
