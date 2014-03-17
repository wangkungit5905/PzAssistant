
#include <QDate>

#include "printtemplate.h"
#include "utils.h"
#include "common.h"
#include "pz.h"

int PzPrintTemplate::TvHeight = 0;  //初始的业务活动表格高度为0

PzPrintTemplate::PzPrintTemplate(QWidget *parent) :
    QWidget(parent),ui(new Ui::PzPrintTemplate)
{
    ui->setupUi(this);
    setLayout(ui->mLayout);
    //BusiUtil::getMTName(mtNames); //获取币种名称
    //调整业务活动表格的各列宽度

    wr.append(0.3);
    wr.append(0.2);
    wr.append(0.15);
    wr.append(0.15);
    wr.append(0.15);
    wr.append(0.05);
    //ui->tview->setColumnWidth(0.3 * pagew);
}

PzPrintTemplate::~PzPrintTemplate()
{
    delete ui;
}

void PzPrintTemplate::resize(const QSize& size)
{
    resize(size.width(), size.height());
}

void PzPrintTemplate::resize(int w, int h)
{
    QWidget::resize(w,h);
    //调整列宽
    int sums = 0; //列宽或行高的合计（因为整数可能除不尽，因此会在右边或下边产生一些误差--有多余的边框）
    for(int i = 0; i < wr.count(); ++i){
        ui->tview->setColumnWidth(i,wr[i] * width());
        sums += wr[i] * width();
    }
    if(sums < w){
        int aw = wr[wr.count()-1] * width() + w - sums -2; //调整后的最后一列的宽带
        ui->tview->setColumnWidth(wr.count()-1, aw);
    }


    //调整行高
//    ui->tview->setRowHeight(0,TITLEHEIGHT);
//    int th = ui->tview->height();
//    int height = (th - TITLEHEIGHT)/(MAXROWS - 1);
    sums = PZPRINTE_TITLEHEIGHT;
    if(TvHeight != 0){
        int rh = (TvHeight - PZPRINTE_TITLEHEIGHT) / (PZPRINTE_MAXROWS + 1);
        for(int i = 1; i < PZPRINTE_MAXROWS + 1; ++i){
            ui->tview->setRowHeight(i, rh);
            sums += rh;
        }
        ui->tview->setRowHeight(PZPRINTE_MAXROWS + 1, TvHeight - PZPRINTE_TITLEHEIGHT - sums - 2);
        //        if(sums < h){
        //           ui->tview->setRowHeight(MAXROWS + 1);
        //        }
    }
    //ui->tview->setProperty("x", 1);

}

//设置业务活动表格的高度静态变量
void PzPrintTemplate::setTvHeight()
{
    if(TvHeight == 0)
        TvHeight = ui->tview->height();
}

//设置汇率
void PzPrintTemplate::setRates(QHash<int, Double> &rates)
{
    this->rates = rates;
}

//设置单位名称
void PzPrintTemplate::setCompany(QString name)
{
    ui->lblCompName->setText(name);
}

//设置凭证日期
void PzPrintTemplate::setPzDate(QDate date)
{
    QString d = QString(tr("%1年%2月%3日")).arg(date.year())
            .arg(date.month()).arg(date.day());
    ui->lblDate->setText(d);
}

//设置凭证附件数
void PzPrintTemplate::setAttNums(int num)
{
    ui->lblAttach->setText(QString::number(num));
}

//设置凭证号
void PzPrintTemplate::setPzNum(QString num)
{
    ui->lblPzNum->setText(num);
}

//设置业务活动
void PzPrintTemplate::setBaList(QList<BusiAction*>& bas)
{
    QTableWidgetItem* item;
    Double v; //借贷金额及合计数

    //绘制表头
    item = new QTableWidgetItem(tr("摘    要"));
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(0,0,item);
    item = new QTableWidgetItem(tr("科    目"));
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(0,1,item);
    item = new QTableWidgetItem(tr("借  方"));
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(0,2,item);
    item = new QTableWidgetItem(tr("贷  方"));
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(0,3,item);
    item = new QTableWidgetItem(tr("外币金额"));
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(0,4,item);
    item = new QTableWidgetItem(tr("汇率"));
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(0,5,item);

    //填制业务活动数据
    BusiAction* ba;
    for(int i = 0; i < bas.count(); ++i){
        ba = bas.at(i);
        ui->tview->setItem(i+1,0,new QTableWidgetItem(ba->getSummary())); //摘要
        ui->tview->setItem(i+1,1,new QTableWidgetItem(tr("%1——%2")
                                                      .arg(ba->getFirstSubject()->getName())
                                                      .arg(ba->getSecondSubject()->getName()))); //科目

        if(ba->getMt() != mmt){
            ui->tview->setItem(i+1,4,new QTableWidgetItem(ba->getValue().toString())); //外币金额
            ui->tview->setItem(i+1,5,new QTableWidgetItem(rates.value(ba->getMt()->code()).toString())); //汇率
            v = ba->getValue() * rates.value(ba->getMt()->code());
        }
        else
            v = ba->getValue();

        if(ba->getDir() == MDIR_J) //借方
            ui->tview->setItem(i+1,2,new QTableWidgetItem(v.toString()));//借方金额
        else
            ui->tview->setItem(i+1,3,new QTableWidgetItem(v.toString()));//贷方金额

    }
}

//填制借贷合计行
void PzPrintTemplate::setJDSums(Double jsum, Double dsum)
{
    QTableWidgetItem* item;
    ui->tview->setSpan(PZPRINTE_MAXROWS+1,0,1,2);
    item = new QTableWidgetItem(tr("合   计"));
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(PZPRINTE_MAXROWS+1,0,item);
    item = new QTableWidgetItem(jsum.toString());
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(PZPRINTE_MAXROWS+1,2,item);//借方合计
    item = new QTableWidgetItem(dsum.toString());
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(PZPRINTE_MAXROWS+1,3,item);//贷方合计
}

//设置制单者
void PzPrintTemplate::setProducer(QString name)
{
    ui->lblProducer->setText(name);
}

//设置审核者
void PzPrintTemplate::setVerify(QString name)
{
    ui->lblVerify->setText(name);
}

//设置记账者
void PzPrintTemplate::setBookKeeper(QString name)
{
    ui->lblBookkeeper->setText(name);
}
/////////////////////////PrintTemplateBase///////////////////////////////////
PrintTemplateBase::PrintTemplateBase(QWidget* parent) : QWidget(parent){}

///////////////////////////PrintTemplateDz/////////////////////////////////////////////
PrintTemplateDz::PrintTemplateDz(MyWithHeaderModels* model,
                                 HierarchicalHeaderView* headView,
                                 QList<int>* colWidths,
                                 QWidget *parent) :
     PrintTemplateBase(parent), ui(new Ui::PrintTemplateDz)
{
    ui->setupUi(this);
    this->model = model;
    hv = headView;    
    this->colWidths = colWidths;
    ui->tview->setHorizontalHeader(hv);
    hv->setStyleSheet("QHeaderView {background-color:white;}"
                      "QHeaderView::section {background-color:white;}");//表头背景色为白色
    ui->tview->setModel(model);
    //设置列宽
    for(int i = 0; i < colWidths->count(); ++i)
        ui->tview->setColumnWidth(i,colWidths->value(i));
    connect(hv,SIGNAL(sectionResized(int,int,int)),this,SLOT(colWidthResized(int,int,int)));

}

PrintTemplateDz::~PrintTemplateDz()
{
    delete ui;
}

PrintPageType PrintTemplateDz::getPageType()
{
    return DETAILPAGE;
}

//设置打印的页号
void PrintTemplateDz::setPageNum(QString strNum)
{
    ui->lblPageNum->setText(strNum);
}

//设置列宽（此函数在打印指定范围的明细帐时，因为由打印预览对象的外部提供页面数据，而外部、
//的数据提供者会根据科目和币种自动选择所使用的表格格式，因此，模板类必须知道当前的表格格式
//以对用户改变列宽作成响应）
void PrintTemplateDz::setColWidth(QList<int>* colWidths)
{
    if(ui->tview->colorCount() != colWidths->count())
        return;
    this->colWidths = colWidths;
    for(int i = 0; i < colWidths->count(); ++i)
        ui->tview->setColumnWidth(i,colWidths->at(i));
}

//设置打印页标题
void PrintTemplateDz::setTitle(QString title)
{
    ui->lblTitle->setText(title);
}

//设置本币名称
void PrintTemplateDz::setMasteMt(QString mtName)
{
    ui->lblMasteMt->setText(mtName);
}

//设置明细账的日期范围
void PrintTemplateDz::setDateRange(int y, int sm, int em)
{
    QDate sd(y,sm,1);
    QString ss = sd.toString(Qt::ISODate);
    ss.chop(3);
    ss.append(tr("——"));
    sd = QDate(y,em,1);
    ss.append(sd.toString(Qt::ISODate));
    ss.chop(3);
    ui->lblDate->setText(ss);
}

void PrintTemplateDz::setDateRange2(const QDate& sd, const QDate& ed)
{
    QString s = tr("%1——%2").arg(sd.toString(Qt::ISODate)).arg(ed.toString(Qt::ISODate));
    ui->lblDate->setText(s);
}

//设置科目名
void PrintTemplateDz::setSubName(QString subName)
{
    ui->lblSubName->setText(subName);
}

//设置核算单位名
void PrintTemplateDz::setAccountName(QString name)
{
    ui->lblAccName->setText(name);
}

//设置制表者
void PrintTemplateDz::setCreator(QString name)
{
    ui->lblCreator->setText(name);
}

//设置打印日期
void PrintTemplateDz::setPrintDate(QDate date)
{
    ui->lblPrintDate->setText(date.toString(Qt::ISODate));
}

//
void PrintTemplateDz::colWidthResized(int logicalIndex, int oldSize, int newSize)
{
    (*colWidths)[logicalIndex] = newSize;
}


////////////////////////PrintTemplateTz::////////////////////////////////////////
PrintTemplateTz::PrintTemplateTz(MyWithHeaderModels *model,
                                 HierarchicalHeaderView* headView,
                                 QList<int>* colWidths,
                                 QWidget *parent) :
    PrintTemplateBase(parent), ui(new Ui::PrintTemplateTz)
{
    ui->setupUi(this);
    this->model = model;
    hv = headView;
    this->colWidths = colWidths;
    ui->tview->setHorizontalHeader(hv);
    hv->setStyleSheet("QHeaderView {background-color:white;}"
                      "QHeaderView::section {background-color:white;}");//表头背景色为白色
    ui->tview->setModel(model);
    //设置列宽
    for(int i = 0; i < colWidths->count(); ++i)
        ui->tview->setColumnWidth(i,colWidths->value(i));
    connect(hv,SIGNAL(sectionResized(int,int,int)),this,SLOT(colWidthResized(int,int,int)));
}

PrintTemplateTz::~PrintTemplateTz()
{
    delete ui;
}

PrintPageType PrintTemplateTz::getPageType()
{
    return TOTALPAGE;
}

void PrintTemplateTz::setPageNum(QString strNum)
{
    ui->lblPageNum->setText(strNum);
}

void PrintTemplateTz::setColWidth(QList<int>* colWidths)
{

}

void PrintTemplateTz::setTitle(QString title)
{
    ui->lblTitle->setText(title);
}

void PrintTemplateTz::setMasteMt(QString mtName)
{
    ui->lblMasterMt->setText(mtName);
}

void PrintTemplateTz::setSubName(QString subName)
{
    ui->lblSub->setText(subName);
}

void PrintTemplateTz::setAccountName(QString name)
{
    ui->lblAccName->setText(name);
}

void PrintTemplateTz::setCreator(QString name)
{
    ui->lblCreator->setText(name);
}

void PrintTemplateTz::setPrintDate(QDate date)
{
    ui->lblPrintDate->setText(date.toString(Qt::ISODate));
}

void PrintTemplateTz::colWidthResized(int logicalIndex, int oldSize, int newSize)
{
    (*colWidths)[logicalIndex] = newSize;
}

///////////////////////PrintTemplateStat///////////////////////////////
PrintTemplateStat::PrintTemplateStat(MyWithHeaderModels *model,
                                     HierarchicalHeaderView* headView,
                                     QList<int>* colWidths,
                                     QWidget *parent) :
    PrintTemplateBase(parent), ui(new Ui::PrintTemplateStat)
{
    ui->setupUi(this);
    this->model = model;
    hv = headView;
    this->colWidths = colWidths;
    ui->tview->setHorizontalHeader(hv);
    hv->setStyleSheet("QHeaderView {background-color:white;}"
                      "QHeaderView::section {background-color:white;}");//表头背景色为白色
    ui->tview->setModel(model);
    //设置列宽
    for(int i = 0; i < colWidths->count(); ++i)
        ui->tview->setColumnWidth(i,colWidths->value(i));
    connect(hv,SIGNAL(sectionResized(int,int,int)),this,SLOT(colWidthResized(int,int,int)));
}

PrintTemplateStat::~PrintTemplateStat()
{
    delete ui;
}

void PrintTemplateStat::setTitle(QString title)
{
    ui->lblTitle->setText(title);
}

PrintPageType PrintTemplateStat::getPageType()
{
    return STATPAGE;
}

void PrintTemplateStat::setPageNum(QString strNum)
{
    ui->lblPageNum->setText(strNum);
}

void PrintTemplateStat::setColWidth(QList<int>* colWidths)
{
    this->colWidths = colWidths;
}

void PrintTemplateStat::setAccountName(QString name)
{
    ui->lblAccName->setText(name);
}

void PrintTemplateStat::setCreator(QString name)
{
    ui->lblCrator->setText(name);
}

void PrintTemplateStat::setPrintDate(QDate date)
{
    ui->lblPrintDate->setText(date.toString(Qt::ISODate));
}

void PrintTemplateStat::colWidthResized(int logicalIndex, int oldSize, int newSize)
{
    (*colWidths)[logicalIndex] = newSize;
}


///////////////////////GdzcJtzjHzTable/////////////////////////////////////
GdzcJtzjHzTable::GdzcJtzjHzTable(QStandardItemModel *model,
                                 QList<int>* colWidths, QWidget *parent) :
    PrintTemplateBase(parent), ui(new Ui::GdzcJtzjHzTable),colWidths(colWidths)
{
    ui->setupUi(this);
    ui->tview->setModel(model);
    ui->tview->horizontalHeader()->
            setStyleSheet("QHeaderView {background-color:white;}"
                    "QHeaderView::section {background-color:white;}");
    ui->tview->verticalHeader()->
            setStyleSheet("QHeaderView {background-color:white;}"
                     "QHeaderView::section {background-color:white;}");
    for(int i = 0; i < colWidths->count(); ++i)
        ui->tview->setColumnWidth(i,colWidths->value(i));
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));
}

GdzcJtzjHzTable::~GdzcJtzjHzTable()
{
    delete ui;
}

void GdzcJtzjHzTable::setTitle(QString title)
{
    ui->lblTitle->setText(title);
}

PrintPageType GdzcJtzjHzTable::getPageType()
{
    return COMMONPAGE;
}

void GdzcJtzjHzTable::setPageNum(QString strNum)
{

}

void GdzcJtzjHzTable::setColWidth(QList<int> *colWidths)
{
    this->colWidths = colWidths;
}

void GdzcJtzjHzTable::setDate(int y, int m)
{
    ui->lblDate->setText(tr("%1年%2月").arg(y).arg(m));
}

void GdzcJtzjHzTable::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    (*colWidths)[logicalIndex] = newSize;
}

//////////////////////////////DtfyJttxHzTable////////////////////////////
DtfyJttxHzTable::DtfyJttxHzTable(QStandardItemModel *model, QList<int> *colWidths, QWidget *parent) :
    PrintTemplateBase(parent),ui(new Ui::DtfyJttxHzTable),colWidths(colWidths)
{
    ui->setupUi(this);
    ui->tview->setModel(model);
    ui->tview->horizontalHeader()->
            setStyleSheet("QHeaderView {background-color:white;}"
                    "QHeaderView::section {background-color:white;}");
    ui->tview->verticalHeader()->
            setStyleSheet("QHeaderView {background-color:white;}"
                     "QHeaderView::section {background-color:white;}");
    for(int i = 0; i < colWidths->count(); ++i)
        ui->tview->setColumnWidth(i,colWidths->value(i));
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));
}

DtfyJttxHzTable::~DtfyJttxHzTable()
{
    delete ui;
}

void DtfyJttxHzTable::setTitle(QString title)
{
    ui->lblTitle->setText(title);
}

PrintPageType DtfyJttxHzTable::getPageType()
{
    return COMMONPAGE;
}

void DtfyJttxHzTable::setPageNum(QString strNum)
{
}

void DtfyJttxHzTable::setColWidth(QList<int> *colWidths)
{
    this->colWidths = colWidths;
}

void DtfyJttxHzTable::setDate(int y, int m)
{
    ui->lblDate->setText(tr("%1年%2月").arg(y).arg(m));
}

void DtfyJttxHzTable::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    (*colWidths)[logicalIndex] = newSize;
}

