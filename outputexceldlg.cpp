#include "outputexceldlg.h"
#include "myhelper.h"
#include "ui_outpuexceldlg.h"
#include <QFileDialog>
#include <QMessageBox>

QTXLSX_USE_NAMESPACE

OutpuExcelDlg::OutpuExcelDlg(QString title, QAbstractItemModel* headModel, QAbstractItemModel* dataModel, QWidget *parent) :
    QDialog(parent),ui(new Ui::OutpuExcelDlg),title(title),headModel(headModel),dataModel(dataModel),excel(0),
    rowHeight_head(40),rowHeight_body(20)
{
    ui->setupUi(this);
    headStartRowIndex = 2;
    ui->btnBrowse->setEnabled(false);
    connect(ui->rdoInsert,SIGNAL(toggled(bool)),this,SLOT(outputModeChanged(bool)));
}

OutpuExcelDlg::~OutpuExcelDlg()
{
    delete ui;
}

void OutpuExcelDlg::setBoltRows(QList<int> rows)
{
    rowBolts = rows;
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

void OutpuExcelDlg::outputModeChanged(bool checked)
{
    ui->btnBrowse->setEnabled(checked);
    if(!checked){ //new excel file
        ui->edtFilename->clear();
        ui->edtSheetName->clear();
        ui->lwSheets->clear();
    }
}

void OutpuExcelDlg::on_btnBrowse_clicked()
{
    if(excel){
        delete excel;
        excel = 0;
    }
    if(ui->rdoNew->isChecked())
        return;
    QString fileName = QFileDialog::getOpenFileName(this,tr("请选择要插入表单的Excel文件！"),".","*.xlsx");
    if(fileName.isEmpty())
        return;
    excel = new QXlsx::Document(fileName,this);
    ui->lwSheets->clear();
    ui->edtFilename->setText(fileName);
    QStringList sheets = excel->sheetNames();
    if(sheets.isEmpty())
        return;
    for(int i = 0; i < sheets.count(); ++i){
        ui->lwSheets->addItem(new QListWidgetItem(sheets.at(i)));
    }
    ui->edtFilename->setText(fileName);
}

void OutpuExcelDlg::on_btnOk_clicked()
{
    if(!excel){
        QString fileName = ui->edtFilename->text();
        if(ui->rdoInsert->isChecked() && fileName.isEmpty()){
            myHelper::ShowMessageBoxWarning(tr("请选择输出的Excel文件名！"));
            return;
        }
        if(ui->rdoNew->isChecked() && fileName.isEmpty()){
            fileName  = QFileDialog::getSaveFileName(this,tr("输出文件名"),QDir::homePath(),"*.xlsx");
            if(fileName.isEmpty()){
                myHelper::ShowMessageBoxWarning(tr("您未选择输出的Excel新建文件名！"));
                return;
            }
            if(!fileName.endsWith(".xlsx"))
                fileName.append(".xlsx");
            ui->edtFilename->setText(fileName);
        }
        excel = new QXlsx::Document(fileName,this);
    }
    QString sheetName = ui->edtSheetName->text();
    if(!ui->lwSheets->findItems(sheetName,Qt::MatchExactly).isEmpty()){
        QMessageBox::warning(this,"",tr("表单名有冲突！"));
        return;
    }
    int index = ui->lwSheets->currentRow();
    if(index == -1)
        index = 0;
    else{
        if(ui->rdoAfter->isChecked())
            index++;
    }
    excel->insertSheet(index,sheetName);

    excel->selectSheet(sheetName);
    genTableHead();
    genTableBody();    
    if(ui->chkBorder->isChecked()){
        for(int i = headStartRowIndex; i < bodyStartRowIndex; ++i){
            for(int j = 0; j < dataModel->columnCount(); ++j){
                if(colWidthes.at(j) == 0)
                    continue;
                Cell* cell = excel->cellAt(i,j+1);
                Format fmt = cell->format();
                fmt.setBorderStyle(Format::BorderMedium);
                excel->write(i,j+1,cell->value(),fmt);
            }
        }
    }
    if(!footer.isEmpty()){
        int row = bodyStartRowIndex + dataModel->rowCount();
        excel->mergeCells(CellRange(row,1,row,dataModel->columnCount()));
        Format fmt;
        fmt.setHorizontalAlignment(Format::AlignLeft);
        fmt.setFontBold(true);
        excel->write(row,1,footer,fmt);
    }
    if(ui->rdoNew->isChecked())
        excel->saveAs(ui->edtFilename->text());
    else
        excel->save();
    accept();
}


void OutpuExcelDlg::genTableHead()
{
    if(!headModel)
        return;
    bool isSpanRow = false;
    int c = 1,span_c;
    QXlsx::Format format;
    format.setFontBold(true);
    format.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    int cols = 0;
    if(!colWidthes.isEmpty() && colWidthes.count() < dataModel->columnCount())
        cols = colWidthes.count();
    else
        cols = dataModel->columnCount();
    if(headModel->columnCount() != dataModel->columnCount()){
        isSpanRow = true;
        bodyStartRowIndex = headStartRowIndex + 2;
    }
    else
        bodyStartRowIndex = headStartRowIndex + 1;
    for(int  i = 0; i < headModel->columnCount(); ++i){
        QModelIndex index = headModel->index(0,i);
        span_c = headModel->columnCount(index);
        if(span_c > 1){
            QXlsx::CellRange range(headStartRowIndex,c,headStartRowIndex,c + span_c-1);
            excel->mergeCells(range);
            excel->write(headStartRowIndex,c,headModel->data(index),format);
            for(int j = 0; j < span_c; ++j){
                QModelIndex inner_index = headModel->index(0,j,index);
                int w = colWidthes.at(c+j-1);
                if(w == 0)
                    excel->setColumnHidden(c,true);
                else{
                    excel->setColumnWidth(c+j,w);
                    excel->write(headStartRowIndex+1,c+j, headModel->data(inner_index),format);
                }
            }
        }
        else{
            if(isSpanRow)
                excel->mergeCells(QXlsx::CellRange(headStartRowIndex,c,headStartRowIndex+1,c));
            int w = colWidthes.at(c-1);

            if(w == 0)
                excel->setColumnHidden(c,true);
            else{
                excel->setColumnWidth(c,w);
                excel->write(headStartRowIndex,c,headModel->data(index),format);
            }
        }
        if(span_c == 0)
            c++;
        else
            c += span_c;
    }
    excel->mergeCells(QXlsx::CellRange(1,1,1,c-1));
    excel->setRowHeight(1,rowHeight_head);\
    format.setFontSize(16);
    format.setBorderStyle(Format::BorderNone);
    excel->write(1,1,title,format);
}

void OutpuExcelDlg::genTableBody()
{
    QList<Format> aligns;
    if(colTextAligns.isEmpty()){
        for(int i = 0; i < dataModel->columnCount(); ++i){
            Format fmt;
            fmt.setHorizontalAlignment(Format::AlignLeft);
            if(ui->chkBorder->isChecked())
                fmt.setBorderStyle(Format::BorderThin);
            aligns<<fmt;
        }
    }
    else{
        for(int i = 0; i < colTextAligns.count(); ++i){
            Format fmt;
            int a = colTextAligns.at(i);
            switch (a) {
            case Qt::AlignLeft:
                fmt.setHorizontalAlignment(Format::AlignLeft);
                break;
            case Qt::AlignHCenter:
                fmt.setHorizontalAlignment(Format::AlignHCenter);
                break;
            case Qt::AlignRight:
                fmt.setHorizontalAlignment(Format::AlignRight);
                break;
            default:
                fmt.setHorizontalAlignment(Format::AlignHCenter);
                break;
            }
            if(ui->chkBorder->isChecked())
                fmt.setBorderStyle(Format::BorderThin);
            aligns<<fmt;
        }
    }
    if(colTypes.isEmpty()){
        for(int i = 0; i < dataModel->columnCount(); ++i)
            colTypes<<TCVT_TEXT;
    }
    for(int r = 0; r < dataModel->rowCount(); ++r){
        for(int c = 0; c < dataModel->columnCount(); ++c){
            QModelIndex index = dataModel->index(r,c);
            if(colWidthes.at(c) != 0){
                Format fm = aligns.at(c);
                if(rowBolts.contains(r))
                    fm.setFontBold(true);
                TableColValueType type = colTypes.at(c);
                switch (type){
                case TCVT_TEXT:
                    excel->write(bodyStartRowIndex+r,c+1,dataModel->data(index),fm);
                    break;
                case TCVT_DOUBLE:
                    excel->write(bodyStartRowIndex+r,c+1,dataModel->data(index).toDouble(),fm);
                    break;
                case TCVT_INT:
                    excel->write(bodyStartRowIndex+r,c+1,dataModel->data(index).toInt(),fm);
                    break;
                case TCVT_BOOL:
                    excel->write(bodyStartRowIndex+r,c+1,dataModel->data(index).toBool()?tr("是"):tr("否"),fm);
                    break;
                }
            }
        }
    }
    excel->setRowHeight(bodyStartRowIndex,bodyStartRowIndex+dataModel->rowCount(),rowHeight_body);
}
