#include "taxescomparisonform.h"
#ifdef Q_OS_WIN
#include "ui_taxescomparisonform.h"
#include "excel/ExcelUtil.h"
#include "tables.h"
#include "common.h"
#include "PzSet.h"
#include "account.h"
#include "dbutil.h"
#include "logs/Logger.h"
#include "subject.h"
#include "pz.h"

#include <QFileDialog>
#include <QSqlQuery>
//#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QListWidget>

TaxesExcelFileCfgDlg::TaxesExcelFileCfgDlg(QWidget *parent) :
    QDialog(parent),ui(new Ui::TaxesExcelFileCfgForm)
{
    ui->setupUi(this);
    QString fileName = QApplication::applicationDirPath()+QDir::separator()+"config/app/taxCfg.ini";
    cfgFile = new QSettings(fileName,QSettings::IniFormat);
    cfgFile->setIniCodec("utf-8");
    readCfgInfos();
    connect(ui->cmbCfgs,SIGNAL(currentIndexChanged(int)),this,SLOT(cfgGroupChanged(int)));
}

TaxesExcelFileCfgDlg::~TaxesExcelFileCfgDlg()
{
    delete ui;
}

void TaxesExcelFileCfgDlg::cfgGroupChanged(int index)
{
    TaxExcelCfgInfo* item = cfgGroups.at(index);
    viewCfgInfos(item);
}

void TaxesExcelFileCfgDlg::on_btnSaveAs_clicked()
{
    QDialog dlg(this);
    QLabel l1(tr("已有配置组："),&dlg);
    QListWidget lw(&dlg);
    int maxId = 0;
    foreach(TaxExcelCfgInfo* gitem, cfgGroups){
        QListWidgetItem* item = new QListWidgetItem(gitem->showName);
        item->setData(Qt::UserRole,gitem->id);
        lw.addItem(item);
        if(gitem->id > maxId)
            maxId = gitem->id;
    }
    maxId++;
    QLabel l2(tr("新建配置组："),&dlg);
    QLineEdit gName(&dlg);
    connect(&lw,SIGNAL(currentTextChanged(QString)),&gName,SLOT(setText(QString)));
    QPushButton btnOk(tr("确定"),&dlg);
    QPushButton btnCancel(tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb;
    lb.addWidget(&btnOk);
    lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&l1);lm->addWidget(&lw);
    lm->addWidget(&l2);lm->addWidget(&gName);
    lm->addLayout(&lb);
    dlg.setLayout(lm);
    if(dlg.exec() == QDialog::Accepted){
        if(gName.text().isEmpty()){
            QMessageBox::warning(this,"",tr("配置组名称不能为空！"));
            return;
        }
        TaxExcelCfgInfo* cfg;
        if(!lw.findItems(gName.text(),Qt::MatchExactly).isEmpty()){
            if(QMessageBox::No == QMessageBox::warning(this,"",tr("确定要覆盖当前的配置组（%1）吗？").arg(gName.text()),
                                                       QMessageBox::Yes|QMessageBox::No,QMessageBox::No))
                return;
            cfg = cfgGroups.at(lw.currentRow());
        }
        else{
            cfg = new TaxExcelCfgInfo;
            cfg->id = maxId;
            cfg->showName = gName.text();
            cfg->groupName = QString("Group_%1").arg(maxId);
            ui->cmbCfgs->addItem(cfg->showName,cfg->id);
            cfgGroups<<cfg;
        }
        cfg->nsrName = ui->edtNsrName->text();
        cfg->nsrTaxCode = ui->edtNsrCode->text();
        cfg->startDate = ui->edtStart->text();
        cfg->endDate = ui->edtEnd->text();
        cfg->startRow = ui->spnStartRow->value();
        cfg->colNumber = ui->spnNumber->textValue();
        cfg->colInvoiceCode = ui->spnInvoiceCode->textValue();
        cfg->colInvoiceNumber = ui->spnInvoiceNumber->textValue();
        cfg->colDate = ui->spnDate->textValue();
        cfg->colMoney = ui->spnMoney->textValue();
        cfg->colTaxMoney = ui->spnTaxMoney->textValue();
        cfg->colGfCode = ui->spnGfCode->textValue();
        cfg->colRepealTag = ui->spnRepealTag->textValue();
        cfg->colInvoiceType = ui->spnType->textValue();
        saveCfgInfos(cfg);
        ui->cmbCfgs->setCurrentIndex(ui->cmbCfgs->findData(cfg->id));
    }
}

void TaxesExcelFileCfgDlg::readCfgInfos()
{
    QStringList segments = cfgFile->childGroups();
    if(segments.isEmpty()){
        TaxExcelCfgInfo* item = new TaxExcelCfgInfo;
        item->id = 0;
        item->groupName = defGroupName;
        item->showName=tr("默认");
        item->nsrName = "K8";
        item->nsrTaxCode = "K7";
        item->startDate = "N7";
        item->endDate = "Q7";
        item->startRow = 12;
        item->colNumber = "F";
        item->colInvoiceNumber = "K";
        item->colDate = "L";
        item->colMoney = "O";
        item->colTaxMoney = "P";
        item->colInvoiceCode = "G";
        item->colInvoiceType = "S";
        item->colRepealTag = "R";
        item->colGfCode = "M";
        saveCfgInfos(item);
        cfgGroups<<item;
    }
    else{
        foreach(QString segment, segments){
            cfgFile->beginGroup(segment);
            TaxExcelCfgInfo* item = new TaxExcelCfgInfo;
            item->groupName = segment;
            item->id = cfgFile->value(key_Id).toInt();
            item->showName = cfgFile->value(key_showname).toString();
            item->nsrName = cfgFile->value(key_nsrName).toString();
            item->nsrTaxCode = cfgFile->value(key_TaxCode).toString();
            item->startDate = cfgFile->value(key_startdate).toString();
            item->endDate = cfgFile->value(key_enddate).toString();
            item->startRow = cfgFile->value(key_startRow).toInt();
            item->colNumber = cfgFile->value(key_col_number).toString();
            item->colInvoiceCode = cfgFile->value(key_col_invoiceCode).toString();
            item->colInvoiceNumber = cfgFile->value(key_col_invoiceNumber).toString();
            item->colDate = cfgFile->value(key_col_date).toString();
            item->colGfCode = cfgFile->value(key_col_gfCode).toString();
            item->colMoney = cfgFile->value(key_col_money).toString();
            item->colTaxMoney = cfgFile->value(key_col_taxMoney).toString();
            item->colRepealTag = cfgFile->value(key_col_repeatTag).toString();
            item->colInvoiceType = cfgFile->value(key_col_invoiceType).toString();
            cfgFile->endGroup();
            cfgGroups<<item;
        }
    }
    int defGroupIndex = 0,index=0;
    foreach(TaxExcelCfgInfo* item, cfgGroups){
        ui->cmbCfgs->addItem(item->showName,item->id);
        if(item->id == 0)
            defGroupIndex = index;
        index++;
    }
    ui->cmbCfgs->setCurrentIndex(defGroupIndex);
    viewCfgInfos(cfgGroups.at(defGroupIndex));
}

void TaxesExcelFileCfgDlg::saveCfgInfos(TaxExcelCfgInfo *item)
{
    cfgFile->beginGroup(item->groupName);
    cfgFile->setValue(key_Id,item->id);
    cfgFile->setValue(key_showname,item->showName);
    cfgFile->setValue(key_nsrName,item->nsrName);
    cfgFile->setValue(key_TaxCode,item->nsrTaxCode);
    cfgFile->setValue(key_startdate,item->startDate);
    cfgFile->setValue(key_enddate,item->endDate);
    cfgFile->setValue(key_startRow,item->startRow);
    cfgFile->setValue(key_col_number,item->colNumber);
    cfgFile->setValue(key_col_invoiceCode,item->colInvoiceCode);
    cfgFile->setValue(key_col_invoiceNumber,item->colInvoiceNumber);
    cfgFile->setValue(key_col_date,item->colDate);
    cfgFile->setValue(key_col_gfCode,item->colGfCode);
    cfgFile->setValue(key_col_money,item->colMoney);
    cfgFile->setValue(key_col_taxMoney,item->colTaxMoney);
    cfgFile->setValue(key_col_repeatTag,item->colRepealTag);
    cfgFile->setValue(key_col_invoiceType,item->colInvoiceType);
    cfgFile->sync();
}

void TaxesExcelFileCfgDlg::viewCfgInfos(TaxExcelCfgInfo *item)
{
    ui->edtNsrName->setText(item->nsrName);
    ui->edtNsrCode->setText(item->nsrTaxCode);
    ui->edtStart->setText(item->startDate);
    ui->edtEnd->setText(item->endDate);
    ui->spnDate->setTextValue(item->colDate);
    ui->spnGfCode->setTextValue(item->colGfCode);
    ui->spnInvoiceCode->setTextValue(item->colInvoiceCode);
    ui->spnInvoiceNumber->setTextValue(item->colInvoiceNumber);
    ui->spnMoney->setTextValue(item->colMoney);
    ui->spnNumber->setTextValue(item->colNumber);
    ui->spnRepealTag->setTextValue(item->colRepealTag);
    ui->spnStartRow->setValue(item->startRow);
    ui->spnTaxMoney->setTextValue(item->colTaxMoney);
    ui->spnType->setTextValue(item->colInvoiceType);

}

///////////////////////////TaxesComparisonForm//////////////////////////////////////////
TaxesComparisonForm::TaxesComparisonForm(AccountSuiteManager *asMgr, QWidget *parent) :
    QWidget(parent), ui(new Ui::TaxesComparisonForm),asMgr(asMgr)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);   //导入页
    ui->tabWidget_2->setCurrentIndex(0); //收入页
    excel = 0;
    account = asMgr->getAccount();
    ui->twResIncome->addAction(ui->actNaviToPz);
    ui->twResCost->addAction(ui->actNaviToPz);
    if(!initTable()){
        QMessageBox::warning(this,"",tr("不能创建导入表！"));
        return;
    }
    initColumnWidth();
    connect(ui->twResIncome,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(cellDoubleClicked(int,int)));
    connect(ui->twResCost,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(cellDoubleClicked(int,int)));
    readTable();
}

TaxesComparisonForm::~TaxesComparisonForm()
{
    delete ui;
}

void TaxesComparisonForm::sheetChanged(int index)
{
    if(excel)
        excel->selectSheet(index+1);
}

void TaxesComparisonForm::cellDoubleClicked(int row, int column)
{
    QTableWidget* tw = qobject_cast<QTableWidget*>(sender());
    if(tw){
        CompareResult cr = (CompareResult)tw->item(row,RTI_RESULT)->data(Qt::UserRole).toInt();
        if(cr == CR_NOT_FONDED || cr == CR_NOT_DISTINGUISH)
            return;
        QString num = tw->item(row,RTI_INVOICE_NUMBER)->text();
        QTableWidget* table;
        if(tw == ui->twResIncome){
            table = ui->twImpIncome;
        }
        else if(tw == ui->twResCost)
            table = ui->twImpCost;
        else
            return;
        for(int r = 0; r < table->rowCount(); r++){
            if(num == table->item(r,ITI_INVOICE_NUMBER)->text()){
                table->selectRow(r);
                ui->tabWidget->setCurrentIndex(0);
                ui->tabWidget_2->setCurrentIndex(tw == ui->twResIncome?0:1);
                return;
            }
        }
    }
}

void TaxesComparisonForm::on_btnBrowseExcel_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("请选择包含税金的Excel文件"),".","*.xls");
    if(fileName.isEmpty())
        return;
    disconnect(ui->cmbSheets,SIGNAL(currentIndexChanged(int)),this,SLOT(sheetChanged(int)));
    ui->cmbSheets->clear();
    ui->edtExcelFileName->setText(fileName);
    if(!excel)
        excel = new ExcelUtil(this);
    if(!excel->open(fileName)){
        QMessageBox::critical(this,"",tr("打开Excel文件“%1”出错！").arg(fileName));
        return;
    }

    int sheetNums = excel->getSheetsCount();
    if(sheetNums  == 0){
        QMessageBox::warning(this,"",tr("文件内没有保护任何表格"));
        return;
    }
    for(int i = 0; i < sheetNums; ++i){
        excel->selectSheet(i+1);
        QString sheetName = excel->getSheetName();
        ui->cmbSheets->addItem(sheetName);
    }
    excel->selectSheet(1);
    connect(ui->cmbSheets,SIGNAL(currentIndexChanged(int)),this,SLOT(sheetChanged(int)));
    ui->btnImpIncome->setEnabled(true);
    ui->btnImpCost->setEnabled(true);
}

void TaxesComparisonForm::on_btnConfig_clicked()
{

}

/**
 * @brief 创建税金导入表
 * @return
 */
bool TaxesComparisonForm::initTable()
{
    QSqlQuery q(account->getDbUtil()->getDb());
    QString s = QString("select name from sqlite_master where name='%1'").arg(tbl_taxes);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(!q.first()){
        s = QString("create table %1(id INTEGER PRIMARY KEY,%2 INTEGER, %3 INTEGER, %4 TEXT, "
                    "%5 TEXT, %6 TEXT, %7 TEXT, %8 REAL, %9 REAL, %10 INTEGER, %11 INTEGER)")
                .arg(tbl_taxes).arg(fld_tax_class).arg(fld_tax_index).arg(fld_tax_invoice_code)
                .arg(fld_tax_invoice_number).arg(fld_tax_date).arg(fld_tax_gfCode)
                .arg(fld_tax_money).arg(fld_tax_taxMoney).arg(fld_tax_repeat_tag)
                .arg(fld_tax_invoice_type);
        if(!q.exec(s)){
            LOG_SQLERROR(s);
            return false;
        }
    }
    return true;
}

/**
 * @brief 清除税金表内以前导入的数据
 * @param isIncome
 * @return
 */
bool TaxesComparisonForm::clearTable(bool isIncome)
{
    QSqlQuery q(account->getDbUtil()->getDb());
    if(isIncome)
        ui->twImpIncome->setRowCount(0);
    else
        ui->twImpCost->setRowCount(0);
    QString s = QString("delete from %1 where %2=%3").arg(tbl_taxes)
            .arg(fld_tax_class).arg(isIncome?1:0);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    if(isIncome)
        ui->btnClearIncome->setEnabled(false);
    else
        ui->btnClearCost->setEnabled(false);
    return true;
}

/**
 * @brief 读取数据库内缓存的记录并显示在对应表格内
 * @param isIncome
 * @return
 */
bool TaxesComparisonForm::readTable()
{
    QSqlQuery q(account->getDbUtil()->getDb());
    QString s = QString("select * from %1 order by %2,%3")
            .arg(tbl_taxes).arg(fld_tax_class).arg(fld_tax_index);
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QTableWidget* table;
    int row,row1=-1,row2=-1; //收入和成本表的当前行索引
    while(q.next()){
        bool isIncome = q.value(FI_TAX_CLASS).toBool();
        if(isIncome){
            table = ui->twImpIncome;
            row = row1++;
        }
        else{
            table = ui->twImpCost;
            row = row2++;
        }
        table->insertRow(row);
        int number = q.value(FI_TAX_INDEX).toInt();
        table->setItem(row,ITI_NUMBER,new QTableWidgetItem(QString::number(number)));
        QString strValue = q.value(FI_TAX_INVOICECODE).toString();
        table->setItem(row,ITI_INVOICE_CODE,new QTableWidgetItem(strValue));
        strValue = q.value(FI_TAX_INCOICENUMBER).toString();
        table->setItem(row,ITI_INVOICE_NUMBER,new QTableWidgetItem(strValue));
        strValue = q.value(FI_TAX_DATE).toString();
        table->setItem(row,ITI_DATE,new QTableWidgetItem(strValue));
        strValue = q.value(FI_TAX_GFCODE).toString();
        table->setItem(row,ITI_GFCODE,new QTableWidgetItem(strValue));
        double v = q.value(FI_TAX_MONEY).toDouble();
        table->setItem(row,ITI_MONEY,new QTableWidgetItem(QString::number(v,'f',2)));
        v = q.value(FI_TAX_TAXMONEY).toDouble();
        table->setItem(row,ITI_TAX_MONEY,new QTableWidgetItem(QString::number(v,'f',2)));
        bool repealTag = q.value(FI_TAX_REPEATTEG).toBool();
        if(repealTag)
            table->setItem(row,ITI_REPEAL_TAG,new QTableWidgetItem(repeal_explain));
        else
            table->setItem(row,ITI_REPEAL_TAG,new QTableWidgetItem());
        InvoiceType type = (InvoiceType)q.value(FI_TAX_INVOICETYPE).toInt();
        if(type == IT_DEDICATED)
            table->setItem(row,ITI_TYPE,new QTableWidgetItem(invoice_type_ded));
        else
            table->setItem(row,ITI_TYPE,new QTableWidgetItem(invoice_type_com));
    }
    return true;
}

/**
 * @brief 将导入的税金数据保存到数据库中
 * @param isIncome
 * @return
 */
bool TaxesComparisonForm::saveImport(bool isIncome)
{
    //保存前，先清除数据库中遗留的记录
    QSqlQuery q(account->getDbUtil()->getDb());
    QTableWidget* table;
    if(isIncome)
        table = ui->twImpIncome;
    else
        table = ui->twImpCost;
    if(isIncome && table->rowCount()==0)
        return true;
    QString s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10,%11) "
                "values(:isIncome,:index,:invoicecode,:invoicenumber,:date,:gfcode,"
                ":money,:taxmoney,:repealtag,:invoiceType)").arg(tbl_taxes)
            .arg(fld_tax_class).arg(fld_tax_index).arg(fld_tax_invoice_code)
            .arg(fld_tax_invoice_number).arg(fld_tax_date).arg(fld_tax_gfCode)
            .arg(fld_tax_money).arg(fld_tax_taxMoney).arg(fld_tax_repeat_tag)
            .arg(fld_tax_invoice_type);
    if(!q.prepare(s)){
        LOG_SQLERROR(s);
        return false;
    }
    QTableWidgetItem* item;
    QString strValue;
    for(int i = 0; i < table->rowCount(); ++i){
        q.bindValue(":isIncome",isIncome?1:0);
        int number = table->item(i,ITI_NUMBER)->text().toInt();
        q.bindValue(":index",number);
        item = table->item(i,ITI_INVOICE_CODE);
        if(item){
            strValue = item->text();
            q.bindValue(":invoicecode",strValue);
        }
        strValue = table->item(i,ITI_INVOICE_NUMBER)->text();
        q.bindValue(":invoicenumber",strValue);
        strValue = table->item(i,ITI_DATE)->text();
        q.bindValue(":date",strValue);
        item = table->item(i,ITI_GFCODE);
        if(item){
            strValue = item->text();
            q.bindValue(":gfcode",strValue);
        }
        strValue = table->item(i,ITI_MONEY)->text();
        q.bindValue(":money",strValue);
        strValue = table->item(i,ITI_TAX_MONEY)->text();
        q.bindValue(":taxmoney",strValue);
        item = table->item(i,ITI_REPEAL_TAG);
        if(item){
            strValue = item->text();
            q.bindValue(":repealtag",strValue.isEmpty()?0:1);
        }
        item = table->item(i,ITI_TYPE);
        if(item){
            strValue = item->text();
            q.bindValue(":invoiceType",(strValue == invoice_type_ded)?IT_DEDICATED:IT_COMMON);
        }
        if(!q.exec()){
            LOG_SQLERROR(q.lastQuery());
            return false;
        }
    }
    return true;
}

/**
 * @brief 从Excel文件中导入税金数据
 * @param isIncome
 * @return
 */
bool TaxesComparisonForm::import(bool isIncome)
{
    if(!excel)
        return false;    
    //显示配置窗口，以允许用户修改配置
    TaxesExcelFileCfgDlg dlg(this);
    if(dlg.exec() == QDialog::Rejected)
        return true;
    QApplication::processEvents();
    QTableWidget* table;
    if(isIncome)
        table = ui->twImpIncome;
    else
        table = ui->twImpCost;
    if(table->rowCount() > 0){
        if(QMessageBox::Yes == QMessageBox::warning(this,"",tr("导入前是否清除缓存的记录？"),
                                                    QMessageBox::Yes|QMessageBox::No))
            clearTable(isIncome);
    }
    QApplication::processEvents();
    bool completed = false;
    int startRow = dlg.getStartRow();
    int colNumber = dlg.getColNumber();
    int colInvoiceCode = dlg.getColInvoiceCode();
    int colInvoiceNumber = dlg.getColInvoiceNumber();
    int colDate = dlg.getColDate();
    int colGfCode = dlg.getColGfCode();
    int colMoney = dlg.getColMoney();
    int colTaxMoney = dlg.getColTaxMoney();
    int colRepeatTeg = dlg.getColRepealTag();
    int colInvoiceType = dlg.getColInvoiceType();
    //检查所有的必填项（开始行、序号、发票号码、发票金额、发票税额、开票日期）
    if(startRow==0 || colNumber==0 || colInvoiceNumber==0 ||
       colDate==0 || colMoney==0 || colTaxMoney==0){
        QMessageBox::warning(this,"",tr("必须设置开始行号、序号列、发票号码列、发票金额列、发票税额列和开票日期列！"));
        return true;
    }


    QDialog infoDlg(this);
    infoDlg.setModal(true);
    QLabel title(tr("正在导入，请稍候："),&infoDlg);
    QLineEdit edtRows("",&infoDlg);
    edtRows.setReadOnly(true);
    QHBoxLayout* l = new QHBoxLayout;
    l->addWidget(&title);
    l->addWidget(&edtRows);
    infoDlg.setLayout(l);
    infoDlg.resize(300,100);
    infoDlg.show();
    int row = 0;
    while(!completed){
        int r = startRow + row;
        QString strValue = excel->getCellValue(r,colNumber).toString();
        bool ok;
        int number = strValue.toInt(&ok);//序号
        if(!ok || number==0)
            break;
        QString invoiceNumber = excel->getCellValue(r,colInvoiceNumber).toString();//发票号
        if(invoiceNumber.length() < 2)
            break;
        table->insertRow(row);
        table->setItem(row,ITI_NUMBER,new QTableWidgetItem(QString::number(number)));
        table->setItem(row,ITI_INVOICE_NUMBER,new QTableWidgetItem(invoiceNumber));
        //发票类型
        if(colInvoiceType != 0){
            QString strInvoiceType = excel->getCellValue(r,colInvoiceType).toString();
            if(strInvoiceType == "YB" || strInvoiceType == invoice_type_ded){
                table->setItem(row,ITI_TYPE,new QTableWidgetItem(invoice_type_ded));
            }
            else{
                table->setItem(row,ITI_TYPE,new QTableWidgetItem(invoice_type_com));
            }
        }

        //发票代码
        if(colInvoiceCode != 0){
            QString invoiceCode = excel->getCellValue(r,colInvoiceCode).toString();
            table->setItem(row,ITI_INVOICE_CODE,new QTableWidgetItem(invoiceCode));
        }

        QString date = excel->getCellValue(r,colDate).toString();//开票日期
        table->setItem(row,ITI_DATE,new QTableWidgetItem(date));
        //购方税号
        if(colGfCode != 0){
            QString gfCode = excel->getCellValue(r,colGfCode).toString();
            table->setItem(row,ITI_GFCODE,new QTableWidgetItem(gfCode));
        }
        double money = excel->getCellValue(r,colMoney).toDouble();//发票金额
        table->setItem(row,ITI_MONEY,new QTableWidgetItem(QString::number(money,'f',2)));
        double taxMoney = excel->getCellValue(r,colTaxMoney).toDouble();//税额
        table->setItem(row,ITI_TAX_MONEY,new QTableWidgetItem(QString::number(taxMoney,'f',2)));
        //作废标记
        if(colRepeatTeg != 0){
            bool repeatTag = excel->getCellValue(r,colRepeatTeg).toBool();
            if(repeatTag)
                table->setItem(row,ITI_REPEAL_TAG,new QTableWidgetItem(repeal_explain));
            else
                table->setItem(row,ITI_REPEAL_TAG,new QTableWidgetItem());
        }
        edtRows.setText(tr("已导入 %1 行！").arg(row));
        QApplication::processEvents();
        row++;
    }
    infoDlg.close();
    return true;
}

/**
 * @brief 从摘要内容中提取发票号码
 * @param summary
 * @return
 */
QString TaxesComparisonForm::subInvoiceNumber(QString summary)
{
    QRegExp re("[0-9]{8,8}");
    int pos = re.indexIn(summary);
    if(pos != -1)
        return summary.mid(pos,8);
    return "";
}

//发票税额比对
CompareResult TaxesComparisonForm::compare(QString invoiceNumber, Double taxMoney, Double& correctValue, bool isIncome)
{
    QTableWidget* table;
    if(isIncome)
        table = ui->twImpIncome;
    else
        table = ui->twImpCost;
    CompareResult result = CR_NOT_FONDED;
    for(int i = 0; i < table->rowCount(); ++i){
        if(table->item(i,ITI_INVOICE_NUMBER)->text() == invoiceNumber){
            Double v = Double(table->item(i,ITI_TAX_MONEY)->text().toDouble());
            if(v == taxMoney)
                result = CR_OK;
            else{
                correctValue = v;
                result = CR_MONEY_NOTEQUAL;
            }
            break;
        }
    }
    return result;
}

/**
 * @brief 将发票号码从源列表转移到结果列表
 * @param invoiceNumber
 * @param isIncome
 */
void TaxesComparisonForm::transferNumber(QString invoiceNumber, bool isIncome)
{
    if(isIncome){
        if(sl_i.removeOne(invoiceNumber))
            rl_i<<invoiceNumber;
    }
    else{
        if(sl_c.removeOne(invoiceNumber))
            rl_c<<invoiceNumber;
    }
}

/**
 * @brief 执行导入收入发票税金数据
 */
void TaxesComparisonForm::on_btnImpIncome_clicked()
{
    import();
    if(ui->twImpIncome->rowCount() != 0)
        ui->btnSaveIncome->setEnabled(true);
}

/**
 * @brief 保存导入的收入发票税金数据到数据库
 */
void TaxesComparisonForm::on_btnSaveIncome_clicked()
{
    ui->btnSaveIncome->setEnabled(false);
    QApplication::processEvents();
    saveImport();

}

/**
 * @brief 保存导入的成本发票税金数据到数据库
 */
void TaxesComparisonForm::on_btnSaveCost_clicked()
{
    ui->btnSaveCost->setEnabled(false);
    QApplication::processEvents();
    saveImport(false);
}

/**
 * @brief 执行导入成本发票税金数据
 */
void TaxesComparisonForm::on_btnImpCost_clicked()
{
    import(false);
    if(ui->twImpCost->rowCount() != 0)
        ui->btnSaveCost->setEnabled(true);
}

/**
 * @brief 清除数据库内的收入发票税金数据
 */
void TaxesComparisonForm::on_btnClearIncome_clicked()
{
    clearTable();
}

/**
 * @brief 清除数据库内的成本发票税金数据
 */
void TaxesComparisonForm::on_btnClearCost_clicked()
{
    clearTable(false);
}

/**
 * @brief 执行比对
 */
void TaxesComparisonForm::on_btnExec_clicked()
{
    if(!asMgr->isPzSetOpened())
        return;
    ui->twResIncome->setRowCount(0);
    ui->twResCost->setRowCount(0);
    //1、装载源列表
    sl_i.clear(); sl_c.clear();rl_i.clear();rl_c.clear();
    for(int i = 0; i < ui->twImpIncome->rowCount(); ++i){
        QString number = ui->twImpIncome->item(i,ITI_INVOICE_NUMBER)->text();
        sl_i<<number;
    }
    for(int i = 0; i < ui->twImpCost->rowCount(); ++i){
        QString number = ui->twImpCost->item(i,ITI_INVOICE_NUMBER)->text();
        sl_c<<number;
    }

    //2、读取本期所有凭证内涉及到应交税金-进项和销项的分录    
    //获取应交税金科目及其应交增值税（销项）和（进项）主目和子目id
    SubjectManager* sm = account->getSubjectManager(asMgr->getSubSysCode());
    FirstSubject* fsub = sm->getYjsjSub();   //应交税金
    SubjectNameItem* ni_jx = sm->getNameItem(tr("应交增值税（进项）"));
    SubjectNameItem* ni_xx = sm->getNameItem(tr("应交增值税（销项）"));
    SecondSubject* ssub_jx = fsub->getChildSub(ni_jx);
    SecondSubject* ssub_xx = fsub->getChildSub(ni_xx);
    int row1=0,row2=0; //分别对应收入和成本发票比较表格的插入行号
    int row;
    bool isIncome = true;
    QTableWidget* table;
    QTableWidgetItem* item;
    QList<PingZheng*> pzs;
    if(!asMgr->getPzSet(asMgr->month(),pzs))
        return;
    foreach(PingZheng* pz, pzs){
        foreach(BusiAction* ba, pz->baList()){
            if(ba->getFirstSubject() != fsub)
                continue;
            SecondSubject* ssub = ba->getSecondSubject();
            if(ssub != ssub_jx && ssub != ssub_xx)
                continue;
            if(ssub == ssub_jx){
                table = ui->twResCost;
                isIncome = false;
                row = row2++;
            }
            else{
                table = ui->twResIncome;
                isIncome = true;
                row = row1++;
            }
            table->insertRow(row);
            table->setItem(row,RTI_NUMBER,new QTableWidgetItem(QString::number(row+1)));
            item = new QTableWidgetItem(QString("%1#").arg(pz->number()));
            item->setData(Qt::UserRole,pz->id());
            table->setItem(row,RTI_PZ_NUMBER,item);
            item = new QTableWidgetItem(QString::number(ba->getNumber()));
            item->setData(Qt::UserRole,ba->getId());
            table->setItem(row,RTI_BA_NUMBER,item);
            QString invoiceNumber = subInvoiceNumber(ba->getSummary());
            if(invoiceNumber.isEmpty()){
                table->setItem(row,RTI_INVOICE_NUMBER,new QTableWidgetItem(ba->getSummary()));
                item = new QTableWidgetItem(result_explain_not_distinguish);
                item->setData(Qt::UserRole,CR_NOT_DISTINGUISH);
                table->setItem(row,RTI_RESULT,item);
                continue;
            }
            table->setItem(row,RTI_INVOICE_NUMBER,new QTableWidgetItem(invoiceNumber));
            //判断记账方向是否正确
            if(isIncome && (ba->getDir() != MDIR_D) || !isIncome && (ba->getDir() != MDIR_J)){
                item = new QTableWidgetItem(result_explain_dir_error);
                item->setData(Qt::UserRole,CR_DIR_ERROR);
                table->setItem(row,RTI_RESULT,item);
                transferNumber(invoiceNumber,isIncome);
                continue;
            }
            //判断币种是否正确
            if(ba->getMt() != account->getMasterMt()){
                item = new QTableWidgetItem(result_explain_moneytype_error);
                item->setData(Qt::UserRole,CR_MONTYTYPE_ERROR);
                table->setItem(row,RTI_RESULT,item);
                transferNumber(invoiceNumber,isIncome);
                continue;
            }
            //判断金额是否正确
            Double correctValue;
            CompareResult result = compare(invoiceNumber,ba->getValue(),correctValue,isIncome);
            QString rInfo;
            switch(result){
            case CR_OK:
                rInfo = result_explain_ok;
                break;
            case CR_MONEY_NOTEQUAL:
                rInfo = result_explain_money_notequal;
                break;
            case CR_NOT_FONDED:
                rInfo = result_explain_not_fonded;
                break;
            }
            item = new QTableWidgetItem(rInfo);
            item->setData(Qt::UserRole,result);
            if(result == CR_MONEY_NOTEQUAL){
                item->setData(Qt::ToolTipRole,tr("正确值：%1，错误值：%2")
                              .arg(correctValue.toString2()).arg(ba->getValue().toString2()));

            }
            else if(result == CR_NOT_FONDED)
                item->setData(Qt::ToolTipRole,tr("税金为：%1").arg(ba->getValue().toString2()));
            table->setItem(row,RTI_RESULT,item);
            transferNumber(invoiceNumber,isIncome);
        }
    }
    //输出未入账发票号及金额等
    if(!sl_i.isEmpty()){
        ui->twResIncome->insertRow(row1++);
        foreach(QString num, sl_i){
            ui->twResIncome->insertRow(row1);
            ui->twImpIncome->setItem(row1,RTI_NUMBER,new QTableWidgetItem(QString::number(row1)));
            ui->twResIncome->setItem(row1,RTI_INVOICE_NUMBER,new QTableWidgetItem(num));
            ui->twResIncome->setItem(row1,RTI_RESULT,new QTableWidgetItem(result_explain_invoice_not_instant));
            row1++;
        }
    }
    if(!sl_c.isEmpty()){
        ui->twResIncome->insertRow(row2++);
        foreach(QString num, sl_c){
            ui->twResIncome->insertRow(row2);
            ui->twImpIncome->setItem(row2,RTI_NUMBER,new QTableWidgetItem(QString::number(row2)));
            ui->twResIncome->setItem(row2,RTI_INVOICE_NUMBER,new QTableWidgetItem(num));
            ui->twResIncome->setItem(row2,RTI_RESULT,new QTableWidgetItem(result_explain_invoice_not_instant));
            row1++;
        }
    }
}

/**
 * @brief 转到该凭证
 */
void TaxesComparisonForm::on_actNaviToPz_triggered()
{
    int pid = 0,bid=0,row;
    if(ui->tabInvoice->currentIndex() == 0){
        row = ui->twResIncome->currentRow();
        if(row == -1)
            return;
        pid = ui->twResIncome->item(row,RTI_PZ_NUMBER)->data(Qt::UserRole).toInt();
        bid = ui->twResIncome->item(row,RTI_BA_NUMBER)->data(Qt::UserRole).toInt();
        //在导入表中选中对应发票的行
        CompareResult rcode = (CompareResult)ui->twResIncome->item(row,RTI_RESULT)->data(Qt::UserRole).toInt();
        if(rcode != CR_NOT_FONDED || rcode != CR_NOT_DISTINGUISH){
            QString num = ui->twResIncome->item(row,RTI_INVOICE_NUMBER)->text();
            for(int i = 0; i < ui->twImpIncome->rowCount(); ++i){
                if(ui->twImpIncome->item(i,ITI_INVOICE_NUMBER)->text() == num){
                    ui->twImpIncome->selectRow(i);
                    break;
                }
            }
        }
    }
    else if(ui->tabInvoice->currentIndex() == 1){
        row = ui->twResCost->currentRow();
        if(row == -1)
            return;
        pid= ui->twResCost->item(row,RTI_PZ_NUMBER)->data(Qt::UserRole).toInt();
        bid = ui->twResCost->item(row,RTI_BA_NUMBER)->data(Qt::UserRole).toInt();
        CompareResult rcode = (CompareResult)ui->twResCost->item(row,RTI_RESULT)->data(Qt::UserRole).toInt();
        if(rcode != CR_NOT_FONDED || rcode != CR_NOT_DISTINGUISH){
            QString num = ui->twResCost->item(row,RTI_INVOICE_NUMBER)->text();
            for(int i = 0; i < ui->twImpCost->rowCount(); ++i){
                if(ui->twImpCost->item(i,ITI_INVOICE_NUMBER)->text() == num){
                    ui->twImpCost->selectRow(i);
                    break;
                }
            }
        }
    }
    if(pid != 0)
        emit openSpecPz(pid,bid);

}

void TaxesComparisonForm::initColumnWidth()
{
    QTableWidget* table;
    for(int i = 0; i < 2; ++i){
        if(i == 0)
            table = ui->twImpIncome;
        else
            table = ui->twImpCost;
        table->setColumnWidth(ITI_TYPE,70);
        table->setColumnWidth(ITI_NUMBER,30);
        table->setColumnWidth(ITI_INVOICE_CODE,100);
        table->setColumnWidth(ITI_INVOICE_NUMBER,80);
        table->setColumnWidth(ITI_DATE,80);
        table->setColumnWidth(ITI_GFCODE,120);
        table->setColumnWidth(ITI_MONEY,80);
        table->setColumnWidth(ITI_TAX_MONEY,70);
        table->setColumnWidth(ITI_REPEAL_TAG,80);
    }
    for(int i = 0; i < 2; ++i){
        if(i == 0)
            table = ui->twResIncome;
        else
            table = ui->twResCost;
        table->setColumnWidth(RTI_NUMBER,50);
        table->setColumnWidth(RTI_INVOICE_NUMBER,100);
        table->setColumnWidth(RTI_RESULT,150);
        table->setColumnWidth(RTI_PZ_NUMBER,50);
        table->setColumnWidth(RTI_BA_NUMBER,50);
    }
}


#endif
