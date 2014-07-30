#ifndef OUTPUEXCELDLG_H
#define OUTPUEXCELDLG_H

#include <QtGlobal>
#ifdef Q_OS_WIN
#include <QDialog>

namespace Ui {
class OutpuExcelDlg;
}

class QAbstractItemModel;
class ExcelUtil;

class OutpuExcelDlg : public QDialog
{
    Q_OBJECT

public:
    explicit OutpuExcelDlg(QString title, QAbstractItemModel* headModel, QAbstractItemModel* dataModel, QWidget *parent = 0);
    ~OutpuExcelDlg();
    void setColWidthes(QList<int> cols);
    void setColTextAligns(QList<int> aligns);
    void setSheetName(QString sheetName);

private slots:
    void on_btnBrowse_clicked();

    void on_btnOk_clicked();

private:
    void genTableHead();
    void genTableBody();

    Ui::OutpuExcelDlg *ui;
    ExcelUtil* excel;
    QString title;
    QAbstractItemModel* headModel;  //表格标题所用模型
    QAbstractItemModel* dataModel;  //表格内容所用模型

    int headStartRowIndex;//表格头起始行
    int bodyStartRowIndex;//表格体起始行

    QList<int> colWidthes;   //列宽
    QList<int> colTextAligns;//列的文本排布方向（Qt::AlignLeft，Qt::AlignRight、Qt::AlignHCenter）
};

#endif // Q_OS_WIN
#endif // OUTPUEXCELDLG_H
