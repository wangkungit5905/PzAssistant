#include <QGraphicsProxyWidget>
#include <QPageSetupDialog>
#include <QGraphicsAnchorLayout>


#include "previewdialog.h"
#include "ui_previewdialog.h"

/**
 * @brief PreviewDialog::PreviewDialog
 * @param templateWidget    打印模板对象
 * @param pageType          页面类型
 * @param printer           打印操作所使用的打印机
 * @param outPaging         是否由外部进行分页处理（true：是（默认））
 * @param parent
 */
PreviewDialog::PreviewDialog(PrintTemplateBase* templateWidget, PrintPageType pageType,
                             QPrinter* printer, bool outPaging, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreviewDialog)
{
    dataModel = NULL;
    headerModel = NULL;
    this->outPaging = outPaging;
    this->pageType = pageType;
    tWidget = templateWidget;
    tv = tWidget->findChild<QTableView*>("tview");

    if(pageType != COMMONPAGE){
        //获取表格的复合代理数据模型，并从中取出表头数据模型和表格内容数据模型
        MyWithHeaderModels* pmodel =
                qobject_cast<MyWithHeaderModels*>(tv->model());
        if(pmodel){
            dataModel = qobject_cast<QStandardItemModel*>(pmodel);
            //if(!outPaging)
                headerModel = pmodel->getHorizontalHeaderModel();
        }
    }
    else{
        //如果要打印通用表格，则保存表头数据
        QAbstractItemModel* model = tv->model();
        for(int i = 0; i < model->columnCount(); ++i){
            QString hd = model->headerData(i,Qt::Horizontal).toString();
            headDatas<<hd;
        }
        dataModel = qobject_cast<QStandardItemModel*>(model);
        if(!dataModel){ //如果转换不成功，则手工转储打印模板中表格的数据源
            dataModel = new QStandardItemModel;
            int rows = model->columnCount();
            int cols = model->columnCount();
            QList<QStandardItem*> l;
            QStandardItem* item;
            for(int i = 0; i < rows; ++i){
                for(int j = 0; j < cols; ++j){
                    item = new QStandardItem(model->data(model->index(i,j)).toString());
                    l<<item;
                }
                dataModel->appendRow(l);
                l.clear();
            }
        }
    }

    //初始化打印机设置
    this->printer = printer;
    printer->setFullPage(true);
    switch (QLocale::system().country()) {
    case QLocale::AnyCountry:
    case QLocale::Canada:
    case QLocale::UnitedStates:
    case QLocale::UnitedStatesMinorOutlyingIslands:
        printer->setPageSize(QPrinter::Letter);
        break;
    default:
        printer->setPageSize(QPrinter::A4);
        break;
    }

    //初始化页边距
    printer->getPageMargins(&leftMargin,&topMargin,&rightMargin,&bottomMargin,QPrinter::Didot);
    rowHeight = 30;
    sceneBgColor = Qt::lightGray;
    sceneZoomFactor = 100;

    w = NULL;
    proWidget = NULL;
    oldw=0;oldh=0;
    pageBack = NULL;
}

PreviewDialog::~PreviewDialog()
{
    delete ui;
}

void PreviewDialog::setupPage()
{
    QRectF rect = printer->paperRect();
    QRectF rectNew = QRectF(0,0,rect.width() / printer->logicalDpiX() * sceneZoomFactor, rect.height() / printer->logicalDpiY() * sceneZoomFactor);
    pageScene.setSceneRect(rectNew);//设置页面渲染场景的矩形区域大小

    //将模板放入场景中，拉伸和定位打印模板，以适合场景的大小
    if(!proWidget){
        proWidget = new QGraphicsProxyWidget;
        proWidget->setWidget(tWidget);
    }

    if(!w){
        l = new QGraphicsAnchorLayout;
        l->setSpacing(0);
        w = new QGraphicsWidget(0, Qt::Widget);
        w->setZValue(1);
        QPalette winPal = w->palette(); //改变背景色为白色
        winPal.setColor(QPalette::Window, Qt::white);
        w->setPalette(winPal);
        w->setPos(0, 0);
        w->setLayout(l);
        l->addAnchor(proWidget, Qt::AnchorTop, l, Qt::AnchorTop);
        l->addAnchor(proWidget, Qt::AnchorBottom, l, Qt::AnchorBottom);
        l->addAnchor(proWidget, Qt::AnchorLeft, l, Qt::AnchorLeft);
        l->addAnchor(proWidget, Qt::AnchorRight, l, Qt::AnchorRight);

        pageScene.addItem(w);
        w->moveBy(leftMargin,topMargin);
        pageScene.setBackgroundBrush(sceneBgColor);
    }
    if((w->x() != leftMargin) || (w->y() != topMargin))
        w->moveBy(leftMargin - w->x(),topMargin - w->y());
    w->resize(rectNew.width()-leftMargin-rightMargin,
              rectNew.height()-topMargin-bottomMargin);

    if(pageBack){
        pageScene.removeItem(pageBack);
        delete pageBack;
    }
    QPen pen;
    if(isPreview)
        pen.setColor(Qt::black);
    else
        pen.setColor(Qt::white);
    pen.setWidth(3);

    //无法使矩形的下边框显示出来
    //QRectF br(rectNew);
    //br.setHeight(rectNew.height() - 5);
    //pageBack = pageScene.addRect(br,pen,QBrush(Qt::white));
    pageBack = pageScene.addRect(rectNew,pen,QBrush(Qt::white));
    pageBack->setZValue(0);
    pageBack->setRect(rectNew);

    rect = proWidget->subWidgetRect(tv);
    rowPerPage = rect.height() / rowHeight-1;
    if(outPaging){
        emit reqPaging(rowPerPage, pages);
    }
    else{
        pages = dataModel->rowCount() / rowPerPage;
        if((dataModel->rowCount() % rowPerPage) != 0)
            pages++;        
    }

}

void PreviewDialog::paintPage(int pagenum)
{
    QList<int>* colWidths = NULL; //列宽，仅在由外部处理分页时使用
    QString pageNumber = QString("%1/%2").arg(pagenum).arg(pages);

    //为当前打印页表格创建一个新的数据源
    oPageModel.clear();
    if(outPaging) //如果由外部进行分页，则从调用方请求页面表格数据
        emit reqPageData(pagenum,colWidths,oPageModel);
    else{ //由预览框自己进行分页处理
        //确定起始边界
        int start,end;
        if(pagenum == 1)
            start = 0;
        else
            start = (pagenum-1)*rowPerPage;
        end = start + rowPerPage;
        if(end > dataModel->rowCount())
            end = dataModel->rowCount();

        //装载数据
        QList<QStandardItem*> l;
        QStandardItem* item;
        Qt::Alignment align;
        int cols = oPageModel.columnCount();
        for(int i = start; i < end; ++i){
            for(int j = 0; j < cols; ++j){
                item = dataModel->item(i,j);
                if(item){
                    align = item->textAlignment();
                    item = new QStandardItem(item->text());
                    item->setTextAlignment(align);
                    item->setEditable(false);
                }
                else
                    item = NULL;
                l<<item;
            }
            oPageModel.appendRow(l);
            l.clear();
        }
    }

    //将数据模型附加到表格视图
    if(pageType != COMMONPAGE){
        //构造复合模型
        //oPageModel.setHorizontalHeaderModel(headerModel);
        if(outPaging)
            tWidget->setColWidth(colWidths);
    }
    else{
        if(!outPaging){ //如果由预览框自己处理分页，则必须重新设置表头内容
            oPageModel.setHorizontalHeaderLabels(headDatas);
        }
    }
    tv->setModel(&oPageModel);
    if(outPaging){
        for(int i = 0; i < oPageModel.columnCount(); ++i)
            tv->setColumnWidth(i,colWidths->value(i));
        for(int i = 0; i < oPageModel.rowCount(); ++i)
            tv->setRowHeight(i,rowHeight);
    }
    else{
        for(int i = 0; i < oPageModel.rowCount(); ++i)
            tv->setRowHeight(i,rowHeight);
    }
    if(!outPaging)
        tWidget->setPageNum(pageNumber);
}

void PreviewDialog::setupSpinBox()
{
    ui->spnPage->setPrefix(QString::number(pages)+" / ");
    ui->spnPage->setMaximum(pages);
    ui->hSlider->setMaximum(pages);
}

//执行打印
void PreviewDialog::print()
{
    //printDialog
    isPreview = false;
    printer->setOutputFormat(QPrinter::NativeFormat);
    setupPage();
    printer->setFromTo(1,pages);
    printer->setOutputFileName("");
    QPrintDialog dialog(printer, this);
    dialog.setWindowTitle(tr("打印文档"));
    if (dialog.exec() == QDialog::Rejected) {
        return;
    }

    // get num copies
    int doccopies;
    int pagecopies;
    if (printer->collateCopies()) {
        doccopies = 1;
        pagecopies = printer->numCopies();
    } else {
        doccopies = printer->numCopies();
        pagecopies = 1;
    }

    // get page range
    int firstpage = printer->fromPage();
    int lastpage = printer->toPage();
    if (firstpage == 0 && lastpage == 0) { // all pages
        firstpage = 1;
        lastpage =pages;
    }

    // print order
    bool ascending = true;
    if (printer->pageOrder() == QPrinter::LastPageFirst) {
        int tmp = firstpage;
        firstpage = lastpage;
        lastpage = tmp;
        ascending = false;
    }

    // loop through and print pages
    QPainter painter(printer);
    for (int dc=0; dc<doccopies; dc++){
        int pagenum = firstpage;
        while (true){
            for (int pc=0; pc<pagecopies; pc++){
                if(printer->printerState() == QPrinter::Aborted ||
                   printer->printerState() == QPrinter::Error){
                    break;
                }
                // print page
                paintPage(pagenum);
                pageScene.render(&painter);
                if (pc < pagecopies-1)
                    printer->newPage();
            }
            if (pagenum == lastpage)  //打印完成
                break;
            if (ascending) {
                pagenum++;
            } else {
                 pagenum--;
            }
            printer->newPage();
        }

        if (dc < doccopies-1)
            printer->newPage();
    }
}

//打印预览
int PreviewDialog::exec()
{
    ui->setupUi(this);
    connect(ui->hSlider, SIGNAL(valueChanged(int)), ui->spnPage, SLOT(setValue(int)));
    connect(ui->btnPrev, SIGNAL(clicked()), ui->spnPage, SLOT(stepDown()));
    connect(ui->btnNext, SIGNAL(clicked()), ui->spnPage, SLOT(stepUp()));

    ui->gview->setScene(&pageScene);
    ui->gview->ensureVisible(0,0,10,10);    
    isPreview = true;
    setupPage();
    setupSpinBox();
    on_spnPage_valueChanged(1); //预览第一页
    return QDialog::exec();
}

//打印到pdf文件
void PreviewDialog::exportPdf(const QString &filename)
{
    isPreview = false;
    setupPage();
    QString dialogcaption = tr("输出到PDF格式文件");
    QString exportname = QFileDialog::getSaveFileName(this, dialogcaption, filename, "*.pdf");
    if (exportname.isEmpty()) return;
    if (QFileInfo(exportname).suffix().isEmpty())
        exportname.append(".pdf");

    // setup printer
    printer->setOutputFormat(QPrinter::PdfFormat);
    printer->setOutputFileName(exportname);

    // print pdf
    QPainter painter(printer);
    for (int pagenum=1; pagenum<=pages; pagenum++){
        paintPage(pagenum);
        pageScene.render(&painter);
        if (pagenum < pages) {
            printer->newPage();
        }
    }
}

//
void PreviewDialog::colWidthResized(int logicalIndex, int oldSize, int newSize)
{
    int i = 0;
}

//页号改变
void PreviewDialog::on_spnPage_valueChanged(int arg1)
{
    paintPage(arg1);
}

//放大
void PreviewDialog::on_btnZoomIn_clicked()
{
    ui->gview->scale(1.5,1.5);
}

//缩小
void PreviewDialog::on_btnZoomOut_clicked()
{
    ui->gview->scale(1/1.5,1/1.5);
}

//页面设置
void PreviewDialog::on_btnSet_clicked()
{
    QPageSetupDialog *dialog;
    qreal x = w->x();
    qreal y = w->y();
    printer->setPageMargins(leftMargin,topMargin,rightMargin,bottomMargin,QPrinter::Didot);
    dialog = new QPageSetupDialog(printer, this);    
    if (dialog->exec() == QDialog::Rejected) {
            return;
    }
    printer->getPageMargins(&leftMargin,&topMargin,&rightMargin,&bottomMargin,QPrinter::Didot);
    setupPage();
    setupSpinBox();
    on_spnPage_valueChanged(1);    
}


