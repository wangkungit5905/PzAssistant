/*
 * Table View Print & Preview dialog
 * Copyright (C) 2004-2008 by Gordos Kund / QnD Co Bt.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact gordos.kund@gmail.com with any questions on this license.
 */

#include "tdpreviewdialog.h"
#include <QtCore/QDebug>
#include <QtGui/QTextDocument>
#include <QtGui/QPageSetupDialog>
#include <QtGui/QGraphicsTextItem>
#include <QtGui/QHeaderView>
#include <QtGui/QPainter>
#include <QtCore/QTime>
#include <QtGui/QPrintDialog>
#include <QtCore/QRectF>


TDPreviewDialog::TDPreviewDialog(QTableView *p_tableView, QPrinter * p_printer, QWidget *parent) : QDialog(parent)
{
    //variables
    tableView=p_tableView;
    leftMargin=80;
    rightMargin=80;
    topMargin=40;
    bottomMargin=40;
    spacing=4;
    headerSize=60;
    footerSize=60;
    sceneZoomFactor=100;
    columnZoomFactor=0.65;
    gridMode=NormalGrid;

    model= tableView->model();

    //setup printer
    printer=p_printer;
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


    setHeaderText(tr("文档"));
    footerText=trUtf8("页：");


    //get column widths
    for (int i=0; i<model->columnCount(); i++) {
        int colWidth=tableView->columnWidth(i);
        //表格列以像素计的宽度值
        colSizes.append(QTextLength(QTextLength::FixedLength,colWidth));
    }

    //title font
    titleFont=tableView->font();
    titleFont.setPointSize(24);
    titleFont.setBold(false);
    titleFont.setUnderline(false);
    titleFmt =new QFontMetrics(titleFont);


    //header font
    headerFont=tableView->font();
    headerFont.setBold(true);
    headerFont.setUnderline(true);
    headerFont.setPointSize(9);
    headerFmt =new QFontMetrics(headerFont);

    //normal font
    font=tableView->font();
    font.setPointSize(9);
    fmt =new  QFontMetrics(font);

    //默认的headerStdText文本为当前时间
    QString date=QDate::currentDate().toString(QLocale().dateFormat());
    QString time=QTime::currentTime().toString(QLocale().timeFormat(QLocale::ShortFormat));
    headerStdText = date+"  "+time;

    setupPage();
    columnMultiplier=pageScene.width()/tableView->width()*columnZoomFactor;
}

//=================================================================

int TDPreviewDialog::exec()
{
    //GUI
    ui.setupUi(this);
    connect(ui.horizontalSlider, SIGNAL(valueChanged(int)), ui.pageSpinBox, SLOT(setValue(int)));
    connect(ui.prevToolButton, SIGNAL(clicked()), ui.pageSpinBox, SLOT(stepDown()));
    connect(ui.nextToolButton, SIGNAL(clicked()), ui.pageSpinBox, SLOT(stepUp()));

    ui.graphicsView->setScene(&pageScene);
    ui.graphicsView->ensureVisible(0,0,10,10);
    setupSpinBox();
    on_pageSpinBox_valueChanged(1);
    return QDialog::exec();
}

//=================================================================

TDPreviewDialog::~TDPreviewDialog()
{}

//==========================设置打印机=======================================
void TDPreviewDialog::on_setupToolButton_clicked()
{
    QPageSetupDialog *dialog;
    dialog = new QPageSetupDialog(printer, this);
    if (dialog->exec() == QDialog::Rejected) {
            return;
    }
    setupPage();
    on_pageSpinBox_valueChanged(1);
    setupSpinBox();
}

//=============================放大====================================
void TDPreviewDialog::on_zoomInToolButton_clicked()
{
    ui.graphicsView->scale(1.5,1.5);
}

//==========================缩小=======================================
void TDPreviewDialog::on_zoomOutToolButton_clicked()
{
    ui.graphicsView->scale(1 / 1.5, 1 / 1.5);
}

//============================页号改变=====================================
void TDPreviewDialog::on_pageSpinBox_valueChanged(int value)
{
    paintPage(value);
    pageScene.addRect(0,0,pageScene.width(),pageScene.height(),QPen(Qt::black, 2.0));
}

//=================================================================
void TDPreviewDialog::setupSpinBox()
{
    ui.pageSpinBox->setPrefix(QString::number(pages)+" / ");
    ui.pageSpinBox->setMaximum(pages);
    ui.horizontalSlider->setMaximum(pages);
}

//===========================计算打印页数======================================
void TDPreviewDialog::setupPage()
{
    rowHeight=font.pointSize()+spacing;
    QRectF rect=printer->paperRect();
    QRectF rectNew = QRectF(0,0,rect.width() / printer->logicalDpiX() * sceneZoomFactor, rect.height() / printer->logicalDpiY() * sceneZoomFactor);
    pageScene.setSceneRect(rectNew);//设置页面渲染场景的矩形区域大小

    //count lines 计算可打印的表格行数（排除了headerStdText、表格标题行和横线）
    lines = ((pageScene.height()-topMargin-bottomMargin-headerSize-footerSize)/rowHeight)-3;

    //count pages 计算需要打印的页数
    QAbstractItemModel *model= tableView->model();
    int rowCount=model->rowCount();
    double div = rowCount / lines;
    int modulo = rowCount % lines;
    if (modulo == 0 ) {
        pages = QVariant(div).toInt();
    } else {
        div = div+1.0;
        pages = QVariant(div).toInt();
    }
}

//=================================================================
void TDPreviewDialog::setHeaderText(const QString &text)
{
    headerText=text;
}

void TDPreviewDialog::setHeaderStdText(const QString &text)
{
    headerStdText = text;
}

//=================================================================
void TDPreviewDialog::print()
{
    //printDialog
    printer->setFromTo(1,pages);
    printer->setOutputFormat(QPrinter::NativeFormat);
    printer->setOutputFileName("");
    QPrintDialog dialog(printer, this);
    dialog.setWindowTitle(tr("打印文档"));
    if (dialog.exec() == QDialog::Rejected) {
        return;
    }
    setupPage();


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
            if (pagenum == lastpage)
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

//=================================================================
void TDPreviewDialog::exportPdf(const QString &filename)
{
    // file save dialog
    QString dialogcaption = "Export PDF";
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

//=================================================================

void TDPreviewDialog::paintPage(int pagenum)
{
    //Delete all
    QList<QGraphicsItem*> L = pageScene.items();
    while ( ! L.empty() ) {
        pageScene.removeItem( L.first() );
        delete L.first();
        L.removeFirst();
    }

    //绘制表格标题行
    int csize=0;
    for (int i=0; i<model->columnCount(); i++) {
        int logicalIndex=tableView->horizontalHeader()->logicalIndex(i);
        QString txt = model->headerData(logicalIndex,Qt::Horizontal,Qt::DisplayRole).toString();
        QGraphicsTextItem *item=new QGraphicsTextItem();//每个标题文本作为文本项目看待
        item->setFont(headerFont);
        //如果表格标题文本太长，则作揭短省略处理
        txt=headerFmt->elidedText(txt,Qt::ElideRight,QVariant(colSizes[logicalIndex].rawValue()).toInt()-8);
        item->setPlainText(txt);
        //item->moveBy((csize*columnMultiplier)+leftMargin,topMargin+headerSize+(spacing*2));
        item->moveBy(csize+leftMargin,topMargin+headerSize+(spacing*2));
        csize=csize+QVariant(colSizes[logicalIndex].rawValue()).toInt();
        pageScene.addItem(item);
    }

    //绘制表格行及其数据
    qreal dx,dy;
    QPen pen(Qt::gray, 1);
    QBrush brush(Qt::gray,Qt::SolidPattern);
    //int borderAdjust=rowHeight / 5;  //原始数据，但文本与表格行下边框有重叠
    int borderAdjust=rowHeight / 3;    //表格行文本垂直向下方向的偏移调节量，使文本刚好在表格行下边框之上

    for (int i=0; i<lines; i++){
        csize=0;
        dy=(i*rowHeight)+topMargin+headerSize+(rowHeight*2);
        int modelIdxY=(pagenum-1)*lines+i; //表格行在模型中的对应序号
        if (modelIdxY>=model->rowCount()){
            break;
        }

        for (int j=0; j<model->columnCount(); j++){
            int logicalIndex=tableView->horizontalHeader()->logicalIndex(j);
            int actColSize=QVariant(colSizes[logicalIndex].rawValue()).toInt();
            QString txt = model->data(model->index(modelIdxY,logicalIndex)).toString();
            QGraphicsTextItem *item=new QGraphicsTextItem();
            item->setFont(font);
            txt=fmt->elidedText(txt,Qt::ElideRight,actColSize-8);
            item->setPlainText(txt);
            //qreal dx=(csize*columnMultiplier)+leftMargin;
            dx=csize+leftMargin;
            item->moveBy(dx,dy);

            //绘制表格框线
            if (gridMode==NormalGrid || gridMode==AlternateWithGrid) {
                pageScene.addRect(dx,dy+borderAdjust,actColSize,rowHeight);
            }
            pageScene.addItem(item);
            csize=csize+actColSize;
        }

        //表格行采用交替颜色
        if (gridMode==AlternateColor || gridMode==AlternateWithGrid) {
            int modulo=i % 2;
            if (modulo==0) {
                //rectangle grey
                QGraphicsRectItem *rItem=pageScene.addRect(leftMargin,dy+borderAdjust,csize,rowHeight,pen,brush);
                rItem->setZValue(-1);
            }
        }
    }

    //绘制页面标题区域
    if (headerSize > 0) {
        //在页面标题文本和headerStdText文本之间绘制直线
        pageScene.addLine(leftMargin,headerSize+topMargin,pageScene.width()-rightMargin, headerSize+topMargin,QPen(Qt::black, 1.0));

        //绘制页标题文本
        QGraphicsTextItem *titleItem=new QGraphicsTextItem();
        titleItem->setFont(titleFont);
        titleItem->setPlainText(headerText);
        int titleWidth=titleFmt->width(headerText);
        double xpos=(pageScene.width()/2)-(titleWidth/2); //水平居中
        double ypos=(headerSize/2)-(titleFmt->height()/2);//垂直居中
        titleItem->moveBy(xpos,ypos+topMargin);
        pageScene.addItem(titleItem);

        //绘制headerStdText文本
        QGraphicsTextItem *item=new QGraphicsTextItem();
        item->setFont(font);
        item->setPlainText(headerStdText);
        item->moveBy(leftMargin,headerSize-rowHeight-spacing+topMargin);
        pageScene.addItem(item);
    }

    // 绘制页脚文本
    if (footerSize > 0) {
        pageScene.addLine(leftMargin,pageScene.height()-footerSize-bottomMargin,pageScene.width()-rightMargin, pageScene.height()-footerSize-bottomMargin,QPen(Qt::black, 1.0));
        QGraphicsTextItem *item=new QGraphicsTextItem();
        item->setFont(font);
        item->setPlainText(footerText+QString::number(pages)+" / "+QString::number(pagenum));
        item->moveBy(leftMargin,pageScene.height()-footerSize-bottomMargin+spacing);
        pageScene.addItem(item);
    }
    //pageScene.update();
}

//========================设置表格线类型=========================================
void TDPreviewDialog::setGridMode(Grids _gridMode)
{
     gridMode=_gridMode;
}
