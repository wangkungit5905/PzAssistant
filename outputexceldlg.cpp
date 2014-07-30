#include "outputexceldlg.h"
#ifdef Q_OS_WIN
#include "ui_outpuexceldlg.h"
#include "excel/ExcelUtil.h"

#include <QFileDialog>
#include <QMessageBox>

OutpuExcelDlg::OutpuExcelDlg(QString title, QAbstractItemModel* headModel, QAbstractItemModel* dataModel, QWidget *parent) :
    QDialog(parent),ui(new Ui::OutpuExcelDlg),title(title),headModel(headModel),dataModel(dataModel)
{
    ui->setupUi(this);
    excel = new ExcelUtil(this);
    headStartRowIndex = 3;
    bodyStartRowIndex = 4;
}

OutpuExcelDlg::~OutpuExcelDlg()
{
    delete ui;
}

void OutpuExcelDlg::setColWidthes(QList<int> cols)
{
    colWidthes=cols;
}

void OutpuExcelDlg::setColTextAligns(QList<int> aligns)
{
    colTextAligns = aligns;
}

/**
 * @brief 设置保存输出数据的表单名
 * @param sheetName
 */
void OutpuExcelDlg::setSheetName(QString sheetName)
{
    ui->edtSheetName->setText(sheetName);
}

void OutpuExcelDlg::on_btnBrowse_clicked()
{
    QString fileName;
    if(ui->rdoNew->isChecked()){
        fileName = QFileDialog::getSaveFileName(this,tr("请输入新建Excel的文件名！"),".","*.xls");
        if(fileName.isEmpty())
            return;
        fileName.replace("/",QDir::separator());//如果使用Qt内置的路径表达格式，则Excel不认，但奇怪的是打开文件时却无需替换路径分隔符
        if(!excel->createNew(fileName)){
            QMessageBox::critical(this,"",tr("创建Excel文件“%1”出错！").arg(fileName));
            return;
        }
    }
    else{
        fileName = QFileDialog::getOpenFileName(this,tr("请选择要插入表单的Excel文件！"),".","*.xls");
        if(fileName.isEmpty())
            return;
        ui->lwSheets->clear();
        ui->edtFilename->setText(fileName);
        if(!excel->open(fileName)){
            QMessageBox::critical(this,"",tr("打开Excel文件“%1”出错！").arg(fileName));
            return;
        }
        QAxObject *sheets = excel->getWorkBooks();
        if(!sheets){
            return;
        }
        int sheetNums = excel->getSheetsCount();
        for(int i = 0; i < sheetNums; ++i){
            QString sheetName = excel->getSheetName(i+1);
            ui->lwSheets->addItem(new QListWidgetItem(sheetName));
        }
    }
    ui->edtFilename->setText(fileName);
}

void OutpuExcelDlg::on_btnOk_clicked()
{
    if(ui->edtSheetName->text().isEmpty()){
        QMessageBox::warning(this,"",tr("请输入插入的表单名！"));
        return;
    }
    QString sheetName = ui->edtSheetName->text();
    if(!ui->lwSheets->findItems(sheetName,Qt::MatchExactly).isEmpty()){
        QMessageBox::warning(this,"",tr("表单名有冲突！"));
        return;
    }
    if(ui->rdoNew->isChecked()/* || ui->lwSheets->count() == 0*/){
        excel->insertSheet(sheetName);
    }
    else{
        int index = ui->lwSheets->currentRow();
        if(index == -1)
            index = 0;
        index++;
        if(ui->rdoAfter->isChecked())
            index++;
        excel->insertSheet(sheetName,index);
    }
    excel->selectSheet(sheetName);
    genTableHead();
    genTableBody();
    excel->save();
    accept();
}


void OutpuExcelDlg::genTableHead()
{
    if(!headModel)
        return;
    bool isSpanRow = false;
    int c = 1,span_c;
    for(int  i = 0; i < headModel->columnCount(); ++i){
        QModelIndex index = headModel->index(0,i);
        span_c = headModel->columnCount(index);
        if(span_c > 1){
            isSpanRow = true;
            if(bodyStartRowIndex - headStartRowIndex != 2)
                bodyStartRowIndex++;
            excel->mergeCells(headStartRowIndex,c,headStartRowIndex,c + span_c-1);
            excel->setCellString(headStartRowIndex,c,headModel->data(index).toString());
            for(int j = 0; j < span_c; ++j){
                QModelIndex inner_index = headModel->index(0,j,index);
                int w = colWidthes.at(c+j-1);
                excel->setColumnWidth(c+j,w);
                if(w != 0){
                    excel->setCellString(headStartRowIndex+1,c+j, headModel->data(inner_index).toString());
                    excel->setCellTextCenter(headStartRowIndex+1,c+j);
                    excel->setCellFontBold(headStartRowIndex+1,c+j,true);
                }
            }
        }
        else{
            if(isSpanRow)
                excel->mergeCells(headStartRowIndex,c,headStartRowIndex+1,c);
            int w = colWidthes.at(c-1);
            excel->setColumnWidth(c,w);
            if(w != 0)
                excel->setCellString(headStartRowIndex,c,headModel->data(index).toString());
        }
        excel->setCellTextCenter(headStartRowIndex,c);
        excel->setCellFontBold(headStartRowIndex,c,true);
        if(span_c == 0)
            c++;
        else
            c += span_c;
    }
    excel->mergeCells(1,1,1,c-1);
    excel->setRowHeight(1,20);
    excel->setCellTextCenter(1,1);
    excel->setCellFontBold(1,1,true);
    excel->setCellString(1,1,title);
}

void OutpuExcelDlg::genTableBody()
{
    int colCount = dataModel->columnCount();
    for(int r = 0; r < dataModel->rowCount(); ++r){
        for(int c = 0; c < dataModel->columnCount(); ++c){
            QModelIndex index = dataModel->index(r,c);
            if(colWidthes.at(c) != 0){
                if(colTextAligns.at(c))
                    excel->setCellTextCenter(bodyStartRowIndex+r,c+1);
                excel->setCellString(bodyStartRowIndex+r,c+1,dataModel->data(index).toString());
            }
        }
    }
}

#endif
