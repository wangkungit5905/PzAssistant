#ifndef OUTPUEXCELDLG_H
#define OUTPUEXCELDLG_H

#include <QtGlobal>
#include <QDialog>

#include "xlsxdocument.h"
#include "commdatastruct.h"

namespace Ui {
class OutpuExcelDlg;
}

class QAbstractItemModel;


class OutpuExcelDlg : public QDialog
{
    Q_OBJECT

public:
    explicit OutpuExcelDlg(QString title, QAbstractItemModel* headModel, QAbstractItemModel* dataModel, QWidget *parent = 0);
    ~OutpuExcelDlg();
    void setBoltRows(QList<int> rows);
    void setColWidthes(QList<int> cols);
    void setColTextAligns(QList<int> aligns);
    void setSheetName(QString sheetName);
    void setBodyRowHeight(int height){rowHeight_body=height;}
    void setHeadRowHeight(int height){rowHeight_head=height;}
    void setFooter(QString text){footer=text;}
    void setColumnTypes(QList<TableColValueType> types){colTypes=types;}

private slots:
    void outputModeChanged(bool checked);
    void on_btnBrowse_clicked();

    void on_btnOk_clicked();

private:
    void genTableHead();
    void genTableBody();

    Ui::OutpuExcelDlg *ui;
    QXlsx::Document *excel;
    QString title;
    QAbstractItemModel* headModel;  //表格标题所用模型
    QAbstractItemModel* dataModel;  //表格内容所用模型

    int headStartRowIndex;//表格头起始行
    int bodyStartRowIndex;//表格体起始行
    int rowHeight_body,rowHeight_head; //表体和表头高度
    QString footer; //表脚注解文本

    QList<int > rowBolts;    //需要突出显示的行
    QList<int> colWidthes;   //列宽
    QList<int> colTextAligns;//列的文本排布方向（Qt::AlignLeft，Qt::AlignRight、Qt::AlignHCenter）
    QList<TableColValueType> colTypes; //列的值类型
};

#endif // OUTPUEXCELDLG_H
