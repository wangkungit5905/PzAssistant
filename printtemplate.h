#ifndef PRINTTEMPLATE_H
#define PRINTTEMPLATE_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QStandardItemModel>

#include "ui_pzprinttemplate.h"
#include "ui_printtemplatedz.h"
#include "ui_printtemplatetz.h"
#include "ui_printtemplatestat.h"
#include "ui_gdzcjtzjhztable.h"
#include "ui_dtfyjttxhztable.h"

#include "printUtils.h"
#include "HierarchicalHeaderView.h"
//#include "dialog3.h"



enum PrintPageType{
    COMMONPAGE = 1,    //通用表格模板，即其表格视图类使用默认的表头，数据使用标准的数据模型
    DETAILPAGE = 2,    //明细账或日记账
    TOTALPAGE  = 3,    //总账
    STATPAGE = 4       //本期统计
};

//凭证打印模板类
class PzPrintTemplate : public QWidget
{
    Q_OBJECT

public:
    explicit PzPrintTemplate(QWidget *parent = 0);
    ~PzPrintTemplate();

    void setCompany(QString name);
    void setPzDate(QDate date);
    void setAttNums(int num);
    void setPzNum(QString num);
    void setBaList(QList<BaData2*> baLst);
    void setJDSums(Double jsum, Double dsum);
    void setProducer(QString name);
    void setVerify(QString name);
    void setBookKeeper(QString name);
    void setRates(QHash<int, Double> rates);

    void resize(const QSize& size);
    void resize(int w, int h);
    void setTvHeight();

    static int TvHeight;

private:
    Ui::PzPrintTemplate *ui;
    //QHash<int,QString> mtNames; //币种代码到币种名称的映射
    QHash<int,Double> rates;    //汇率
    QList<double> wr; //列宽比率



};

//打印模板基类（所有需要用PrewViewDialog类来打印的共同接口方法）
class PrintTemplateBase : public QWidget
{
public:
    PrintTemplateBase(QWidget* parent = 0);
    virtual void setTitle(QString title) = 0;
    virtual void setPageNum(QString strNum) = 0; //设置页号
    virtual PrintPageType getPageType() = 0;     //返回打印页的类型
    virtual void setColWidth(QList<int>* colWidths) = 0;  //设置列宽
    //virtual void setPageOrientation(QPrinter::Orientation pageOri) = 0;
    //virtual QPrinter::Orientation getPageOrientation() = 0;

private:
    QPrinter::Orientation pageOrientation;  //页面方向
};

//明细账打印模板类
class PrintTemplateDz : public /*QWidget,*/PrintTemplateBase
{
    Q_OBJECT

public:
    explicit PrintTemplateDz(ProxyModelWithHeaderModels* model,
                             HierarchicalHeaderView* headView,
                             QList<int>* colWidths,
                             QWidget *parent = 0);
    ~PrintTemplateDz();

    //PrintTemplateBase
    void setTitle(QString title);
    PrintPageType getPageType();
    void setPageNum(QString strNum);
    void setColWidth(QList<int>* colWidths);

    void setMasteMt(QString mtName);
    void setDateRange(int y, int sm, int em);
    void setSubName(QString subName);
    void setAccountName(QString name);
    void setCreator(QString name);
    void setPrintDate(QDate date);

private slots:
    void colWidthResized(int logicalIndex, int oldSize, int newSize);

private:
    Ui::PrintTemplateDz *ui;
    ProxyModelWithHeaderModels* model;
    HierarchicalHeaderView* hv;
    QList<int>* colWidths; //表格列宽

    //PageMargin margins;
};

//打印总账的模板类
class PrintTemplateTz : public PrintTemplateBase
{
    Q_OBJECT

public:
    explicit PrintTemplateTz(ProxyModelWithHeaderModels* model,
                             HierarchicalHeaderView* headView,
                             QList<int>* colWidths,
                             QWidget *parent = 0);
    ~PrintTemplateTz();

    //PrintTemplateBase
    void setTitle(QString title);
    PrintPageType getPageType();
    void setPageNum(QString strNum);
    void setColWidth(QList<int>* colWidths);

    void setMasteMt(QString mtName);
    void setSubName(QString subName);
    void setAccountName(QString name);
    void setCreator(QString name);
    void setPrintDate(QDate date);

private slots:
    void colWidthResized(int logicalIndex, int oldSize, int newSize);


private:
    Ui::PrintTemplateTz *ui;
    ProxyModelWithHeaderModels* model;
    HierarchicalHeaderView* hv;
    QList<int>* colWidths; //表格列宽
};

//打印本期统计的模板类
class PrintTemplateStat: public PrintTemplateBase
{
    Q_OBJECT

public:
    explicit PrintTemplateStat(ProxyModelWithHeaderModels* model,
                               HierarchicalHeaderView* headView,
                               QList<int>* colWidths,
                               QWidget *parent = 0);
    ~PrintTemplateStat();

    //PrintTemplateBase
    void setTitle(QString title);
    PrintPageType getPageType();
    void setPageNum(QString strNum);
    void setColWidth(QList<int>* colWidths);

    void setAccountName(QString name);
    void setCreator(QString name);
    void setPrintDate(QDate date);

private slots:
    void colWidthResized(int logicalIndex, int oldSize, int newSize);

private:
    Ui::PrintTemplateStat *ui;
    ProxyModelWithHeaderModels* model;
    HierarchicalHeaderView* hv;
    QList<int>* colWidths; //表格列宽
};

///////////////////////固定资产计提折旧汇总表打印模板类/////////////////////////
class GdzcJtzjHzTable : public PrintTemplateBase
{
    Q_OBJECT

public:
    explicit GdzcJtzjHzTable(QStandardItemModel* model,
                             QList<int>* colWidths, QWidget *parent = 0);
    ~GdzcJtzjHzTable();

    //PrintTemplateBase
    void setTitle(QString title);
    PrintPageType getPageType();
    void setPageNum(QString strNum);
    void setColWidth(QList<int>* colWidths);

    void setDate(int y, int m);

private slots:
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);

private:
    Ui::GdzcJtzjHzTable *ui;
    //int pageNum; //页号
    QList<int>* colWidths; //表格列宽
};

///////////////////////待摊费用计提摊销汇总表打印模板类/////////////////////////
class DtfyJttxHzTable : public PrintTemplateBase
{
    Q_OBJECT

public:
    explicit DtfyJttxHzTable(QStandardItemModel* model,
                             QList<int>* colWidths, QWidget *parent = 0);
    ~DtfyJttxHzTable();

    //PrintTemplateBase
    void setTitle(QString title);
    PrintPageType getPageType();
    void setPageNum(QString strNum);
    void setColWidth(QList<int>* colWidths);

    void setDate(int y, int m);

private slots:
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);


private:
    Ui::DtfyJttxHzTable *ui;
    QList<int>* colWidths; //表格列宽
};


#endif // PRINTTEMPLATE_H
