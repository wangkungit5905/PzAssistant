#ifndef PRINTUTILS_H
#define PRINTUTILS_H

#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QTableView>
#include <QTableView>
#include <QDate>

#include "commdatastruct.h"



////利用QTableView.render()方法来完成实际的打印任务的代理类
//class PrintView : public QTableView
//{
//    Q_OBJECT

//public:
//    PrintView();

//public Q_SLOTS:
//    void print(QPrinter *printer);
//};


//完成打印表格任务的实用类
class PrintUtils : public QObject
{
    Q_OBJECT

public:
    PrintUtils();
    ~PrintUtils();
    PrintUtils(QPrinter* printer);

    //void printTable(QTableView* table, int hTitleNums, int vTitleNums);
    void setTable(QTableView* table);
    void setHTNums(int nums);
    void setVTNums(int nums);
    void paginate(QList<QTableView*> &tvLst);
    void printTables(QList<QTableView*> tvLst, QPainter* paint);

public slots:
    void print(QPrinter* printer);

private:
    void formatTableTitle(int rows, QTableView* tv);
    //void printTablePart(int from, int to);

    void printTable(QTableView* tv, QPainter* paint, bool newPage = false);

    void setTvFormat(QTableView* tv);

    QPrinter* printer;
    QTableView* tview; //外部欲打印的表格视图部件
    //QTableView tv;
    int hTitleNums;  //水平和垂直表头的行数或列数
    int vTitleNUms;

    int pageW,pageH; //可打印区域的宽和高
    QList<double> colWidths; //列宽与总宽的比率列表
};

//////////////////////////////////////////////////////////////

//完成打印凭证任务的实用类
class PrintPzUtils : public QObject{
    Q_OBJECT
public:
    PrintPzUtils();
    PrintPzUtils(QPrinter* printer);
    void setPzDatas(QList<PzPrintData2*> datas);
    void setCompanyName(QString name);
    void setRates(QHash<int,Double> rates);

public slots:
    void print(QPrinter* printer);

private:
    void printPage(QPainter* paint, int index, bool newPage = false);

    QPrinter* printer;
    int pageW,pageH;         //可打印区域的宽和高
    QList<PzPrintData2*> datas;    //凭证数据集合
    QString company;         //凭证的单位名称
    QHash<int,Double> rates; //汇率

    //qreal LEFTMARGIN,TOPMARGIN,RIGHTMARGIN,BUTTOMMARGIN;
};
#endif // PRINTUTILS_H
