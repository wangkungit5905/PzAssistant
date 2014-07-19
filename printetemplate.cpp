
#include <QDate>

#include "printtemplate.h"
#include "utils.h"
#include "common.h"
#include "pz.h"

//int PzPrintTemplate::TvHeight = 0;  //初始的业务活动表格高度为0

PzPrintTemplate::PzPrintTemplate(PzTemplateParameter* parameter, /*QPainter* painter, */QWidget *parent) :
    QWidget(parent),ui(new Ui::PzPrintTemplate),parameter(parameter)/*,painter(painter)*/
{
    ui->setupUi(this);
    ui->tview->setWordWrap(true);//搞不懂，在ui文件中设置此属性为真，却不会自动折行？
            //而且折行还需字体大小与行高的配合，如果垂直方向上没有足够的空间同样也不会折行
    adjustTableRow();
    QFontDatabase fd;
    QFont pFont = ui->tview->font();
    pointSizes = fd.pointSizes(pFont.family(),pFont.styleName());
    pFont.setPointSize(parameter->fontSize);
    ui->tview->setFont(pFont);
}

PzPrintTemplate::~PzPrintTemplate()
{
    delete ui;
}

//设置业务活动表格的高度静态变量
void PzPrintTemplate::adjustTableRow()
{
    ui->tview->setRowCount(parameter->baRows+2);
    ui->tview->setColumnWidth(0,parameter->factor[0]);
    ui->tview->setColumnWidth(1,parameter->factor[1]);
    ui->tview->setColumnWidth(2,parameter->factor[2]);
    ui->tview->setColumnWidth(3,parameter->factor[2]);
    ui->tview->setColumnWidth(4,parameter->factor[3]);
    ui->tview->setColumnWidth(5,parameter->factor[4]);
    ui->tview->setRowHeight(0,parameter->titleHeight);
    for(int i = 1; i < parameter->baRows+1; ++i)
        ui->tview->setRowHeight(i,parameter->baRowHeight);
}

/**
 * @brief 判断是否需要缩小字体尺寸以使给定字符串能够放置在给定的表格列（最多折行为2行后）
 * @param colIndex
 * @param str
 * @return
 */
//bool PzPrintTemplate::isReduceFontSize(int colIndex, QTableWidgetItem* item)
//{
//    QString str = item->text();
//    int colWidget = ui->tview->columnWidth(colIndex);
//    QFont font(item->font());
//    QFontMetrics fm = QFontMetrics(font);
//    int w = fm.width(str);
//    int w1 = fm.boundingRect(str).width();
//    int h = fm.height();
//    int h1 = fm.boundingRect(str).height();

//    //需要折行处理
//    if(w > colWidget){
//        //如果两行的高度超出了行高，则尝试缩小字体尺寸以使它可以容纳
//        if(h1*2 > parameter->baRowHeight){
//            int oldPointSize = font.pointSize();
//            int size = oldPointSize;
//            int index = pointSizes.indexOf(oldPointSize);
//            while(h1*2 > parameter->baRowHeight && index > 0){
//                size = pointSizes.at(index-1);
//                font.setPointSize(size);
//                QFontMetrics fm1 = QFontMetrics(font);
//                h1 = fm1.boundingRect(str).height();
//            }
//            if(size != oldPointSize)
//                item->setFont(font);
//            return true;
//        }
//        else{
//        }
//    }
//    return false;
//}

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
    ui->tview->clearContents();

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
    QString str;
    //恢复分录的字体尺寸到配置的尺寸
    QFont font = ui->tview->font();
    if(ui->tview->font().pointSize() != parameter->fontSize){
        font.setPointSize(parameter->fontSize);
        ui->tview->setFont(font);
    }
    //找出分录中最长的字符串，当前的行高和字体尺寸是否可以满足其折行要求，如果不行，则尝试缩小字体直至满足或到最小字体为止
    int maxSize =0;
    QString maxStr;
    int maxCol=0;  //最大长度的串是出现在摘要栏还是科目栏(1：摘要，2：科目栏)
    for(int i = 0; i < bas.count(); ++i){
        ba = bas.at(i);
        int size = ba->getSummary().size();
        if(maxSize < size){
            maxCol = 1;
            maxStr = ba->getSummary();
            maxSize = size;
        }
        QString str = tr("%1——%2")
                .arg(ba->getFirstSubject()->getName())
                .arg(ba->getSecondSubject()->getLName());
        size = str.size();
        if(maxSize < size){
            maxStr = str;
            maxSize = size;
            maxCol = 2;
        }
    }
    //如果需要折行以显示所有文本，则尝试缩小字体直至满足当前行高可以容纳两行文本或到最小字体为止
    //求出的宽度尺寸好像偏大，以至于有些不需要折行的文本也被缩小字体来折行，但实际可以不用缩小字
    //体也可以折行显示，因此目前无法正确把握何时需要缩小字体以折行的时机？
//    QFontMetrics fm(font);
//    int width = fm.boundingRect(maxStr).width()+2;
//    if(((maxCol==1) && (width > parameter->factor[0])) ||
//            ((maxCol == 2) && (width > parameter->factor[1]))){
//        int h = fm.boundingRect(maxStr).height()+1;
//        int index = pointSizes.indexOf(parameter->fontSize);
//        int size = parameter->fontSize;
//        while(h*2 > parameter->baRowHeight && index > 0){
//            index--;
//            size = pointSizes.at(index);
//            font.setPointSize(size);
//            QFontMetrics fm1(font);
//            h = fm.boundingRect(maxStr).height();
//        }
//        if(size != ui->tview->font().pointSize())
//            ui->tview->setFont(font);
//    }
    for(int i = 0; i < bas.count(); ++i){
        ba = bas.at(i);
        str = ba->getSummary();
        item = new QTableWidgetItem(str);
        ui->tview->setItem(i+1,0,item); //摘要
        str = tr("%1——%2")
                .arg(ba->getFirstSubject()->getName())
                .arg(ba->getSecondSubject()->getLName());
        item = new QTableWidgetItem(str);
        ui->tview->setItem(i+1,1,item); //科目
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
    ui->tview->setSpan(parameter->baRows+1,0,1,2);
    item = new QTableWidgetItem(tr("合   计"));
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(parameter->baRows+1,0,item);
    item = new QTableWidgetItem(jsum.toString());
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(parameter->baRows+1,2,item);//借方合计
    item = new QTableWidgetItem(dsum.toString());
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(parameter->baRows+1,3,item);//贷方合计
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

