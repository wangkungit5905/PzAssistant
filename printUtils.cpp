#include <QMessageBox>
#include <QHeaderView>
#include <QScrollBar>
#include <QPainter>
#include <QStandardItemModel>

#include "printUtils.h"
#include "printtemplate.h"
#include "common.h"
#include "global.h"


////////////PrintView/////////////////////////////////////////////
//PrintView::PrintView()
//{
//    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//}

//void PrintView::print(QPrinter *printer)
//{
//    resize(printer->width(), printer->height());
//    render(printer);
//}

////////////////PrintUtils////////////////////////////////////////

PrintUtils::PrintUtils(){
    QPrintDialog dlg;
    if(QMessageBox::Accepted == dlg.exec()){
        printer = dlg.printer();
    }
    else
       printer = NULL;
}

PrintUtils::~PrintUtils()
{
//    if(printer != NULL)
//        delete printer;
}

PrintUtils::PrintUtils(QPrinter* printer)
{
    this->printer = printer;
}

///**
//    打印表格，参数table：源表格部件，
//               hTitleNums：表格水平方向标题的行数
//               hTitleNums：表格垂直方向标题的行数
//*/
//void PrintUtils::printTable(QTableView* table, int hTitleNums, int vTitleNums)
//{

//}

//设置欲打印的表格视图部件，因为打印的数据都在表格视图部件中
void PrintUtils::setTable(QTableView* table)
{
    tview = table;
}

//设置水平表头的行数
void PrintUtils::setHTNums(int nums)
{
    hTitleNums = nums;
}

//设置垂直表头的行数
void PrintUtils::setVTNums(int nums)
{
    vTitleNUms = nums;
}

//响应打印请求，执行实际的打印任务
void PrintUtils::print(QPrinter* printer)
{
    if(printer != NULL){        
        this->printer = printer;
        pageW = printer->pageRect().width();
        pageH = printer->pageRect().height();

        //获取外部表格的列宽，并计算总宽
        int tw = 0;//列宽的总和        
        for(int i = 0; i < tview->model()->columnCount(); ++i){
            if(!tview->isColumnHidden(i)){
                colWidths.append(tview->columnWidth(i));
                tw += tview->columnWidth(i);
            }
        }
        //得到列宽与总宽的比率
        for(int i = 0; i < colWidths.count(); ++i){
            colWidths[i] = colWidths[i]/tw;
        }


        QList<QTableView*> tvs;
        QPainter paint(printer);
        paginate(tvs);
        printTables(tvs, &paint);
        tvs.clear();


//        QTableView tv;
//        QPixmap pixmap(pageW,pageH);
//        tv.setModel(tview->model());
//        setTvFormat(&tv);
//        tv.render(&pixmap);
//        paint.save();
//        paint.drawPixmap(QPoint(0,0), pixmap);
//        paint.restore();

//        bool r = printer->newPage();
//        tv.render(&pixmap);
//        paint.save();
//        paint.drawPixmap(QPoint(0,0), pixmap);
//        paint.restore();


    }
}

//设置表格的格式
void PrintUtils::setTvFormat(QTableView* tv)
{
    tv->resize(pageW, pageH);
    //去除滚动条及其控件所带的标题条
    tv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tv->horizontalHeader()->setVisible(false);
    tv->verticalHeader()->setVisible(false);
    //调整列宽和隐藏不可见列
    int i = 0;
    for(int j = 0; j < tview->model()->columnCount(); ++j){
        if(!tview->isColumnHidden(j))
            tv->setColumnWidth(j, pageW * colWidths[i++]);
        else
            tv->hideColumn(j);
    }
}

//分页处理
void PrintUtils::paginate(QList<QTableView*> &tvLst)
{
    QTableView* tv = new QTableView;
    tv->setModel(tview->model());
    setTvFormat(tv);

    //计算表格的总高，如果超出了可打印区域则需要进行分页处理
    int th = 0;
    for(int i = 0; i < tv->model()->rowCount(); ++i){
        th += tv->rowHeight(i);
    }
    if(th <= pageW){
        formatTableTitle(hTitleNums, tv);
        tvLst.append(tv);
    }
    else{
        //计算表头高度
        int tth = 0;
        for(int i = 0; i < hTitleNums; ++i)
            tth += tv->rowHeight(i);

        //分页
        QList<int> rowsInPage; //每页实际拥有的数据行数
        int h = tth; //每页都需要一个表头
        int rowNums = 0;
        for(int i = hTitleNums; i < tview->model()->rowCount(); ++i){
            h += tv->rowHeight(i);
            if(h > pageH){
                rowsInPage.append(rowNums);
                h = tth + tv->rowHeight(i); rowNums = 1;
            }
            else{
                rowNums++;
            }
        }
        rowsInPage.append(rowNums);

        //创建每页的表格
        QStandardItemModel* smodel = (QStandardItemModel*)tview->model();
        QStandardItemModel* model;
        QList<QStandardItem*> items;
        int rowIdx = hTitleNums;  //数据行的索引
        for(int i = 0; i < rowsInPage.count(); ++i){
            QTableView *ttv = new QTableView;
            int rows,cols;
            rows = hTitleNums;
            cols = tview->model()->columnCount();

            //设置表头行
            model = new QStandardItemModel;
            for(int r = 0; r < rows; ++r){
                for(int c = 0; c < cols; ++c){
                    if(smodel->item(r,c)){
                        QString v = smodel->item(r,c)->data(Qt::DisplayRole).toString();
                        QStandardItem* item = new QStandardItem(v);
                        items.append(item);
                    }
                    else
                        items.append(NULL);

                }
                model->appendRow(items);
                items.clear();
            }
            //设置表格数据行
            for(int r = 0; r < rowsInPage[i]; ++r){
                for(int c = 0; c < cols; ++c){
                    if(smodel->item(rowIdx,c)){
                        QString v = smodel->item(rowIdx,c)->data(Qt::DisplayRole).toString();
                        QStandardItem* item = new QStandardItem(v);
                        items.append(item);
                    }
                    else
                        items.append(NULL);
                }
                model->appendRow(items);
                rowIdx++;
                items.clear();
            }
            ttv->setModel(model);
            setTvFormat(ttv);
            tvLst.append(ttv);
        }
    }

}

//打印多页
void PrintUtils::printTables(QList<QTableView*> tvLst, QPainter* paint)
{
    if(tvLst.count() == 1)
        printTable(tvLst[0], paint);
    else{
        printTable(tvLst[0], paint);
        for(int i = 1; i < tvLst.count(); ++i){
            printTable(tvLst[i], paint, true);
        }
    }
}

//打印单页
void PrintUtils::printTable(QTableView* tv, QPainter* paint, bool newPage)
{
    formatTableTitle(hTitleNums, tv);
    if(newPage)
        printer->newPage();
    QPixmap pixmap(pageW, pageH);
    tv->render(&pixmap);
    paint->save();
    paint->drawPixmap(QPoint(0,0), pixmap);
    paint->restore();
}

//使打印的表格表头的格式与原始的表格视图一致
void PrintUtils::formatTableTitle(int rows, QTableView* tv)
{
    int span;
    for(int r = 0; r < rows; ++r)
        for(int c = 0; c < tv->model()->columnCount(); ++c){
            if(!tview->isColumnHidden(c)){
                span = tview->rowSpan(r,c);
                if(span > 1){
                    tv->setSpan(r,c,1,span);
                    c += (span-1);
                }
            }
        }
}

//void PrintUtils::printTablePart(int from, int to)
//{

//}

////////////////////////PrintPzUtils/////////////////////////////////////
PrintPzUtils::PrintPzUtils()
{
    QPrintDialog dlg;
    if(QMessageBox::Accepted == dlg.exec()){
        printer = dlg.printer();
    }
    else
       printer = NULL;


}

PrintPzUtils::PrintPzUtils(QPrinter* printer)
{
    this->printer = printer;
}


void PrintPzUtils::print(QPrinter* printer)
{
    if(printer != NULL){
        this->printer = printer;

        //设置页边距
        printer->setPageMargins(10,10,10,10,QPrinter::Millimeter);
        pageW = printer->pageRect().width();
        pageH = printer->pageRect().height();

//        PzPrintTemplate* p1 = new PzPrintTemplate;
//        PzPrintTemplate* p2 = new PzPrintTemplate;
//        QWidget* w = new QWidget;
//        QVBoxLayout* l = new QVBoxLayout;
//        l->addWidget(p1);
//        l->addWidget(p2);
//        w->setLayout(l);
//        w->resize(pageW,pageH);


        QPainter paint(printer);
        if(datas.count() < 3)
            printPage(&paint,0);
        else{
            printPage(&paint,0);
            for(int i = 2; i < datas.count(); i+=2){
                printPage(&paint,i,true);
            }
        }


//        QPixmap pixmap(pageW,pageH/2-10);
//        p1->render(&pixmap);
//        paint.save();
//        paint.drawPixmap(QPoint(0,0), pixmap);
//        paint.restore();
//        p2->render(&pixmap);
//        paint.save();
//        paint.drawPixmap(QPoint(0,pageH/2 + 10), pixmap);
//        paint.restore();

    }
}

void PrintPzUtils::printPage(QPainter* paint, int index, bool newPage)
{
    QPixmap pixmap(pageW,pageH/2-MIDGAP/2);
    if(newPage)
        printer->newPage();
    PzPrintTemplate* p1;
    int x=0;int y=0;
    QString name;
    for(int i = index; i < index+2; ++i){
        if(i < datas.count()){
            p1 = new PzPrintTemplate;
            p1->setRates(rates);
            p1->setCompany(company);               //单位名称
            p1->setPzDate(datas[i]->date);         //凭证日期
            p1->setAttNums(datas[i]->attNums);     //附件数
            p1->setPzNum(datas[i]->pzNum);         //凭证号
            if(datas[i]->producer != 0)
                p1->setProducer(allUsers.value(datas[i]->producer)->getName());   //制单者
            else
                p1->setProducer("");
            if(datas[i]->verify != 0)
                p1->setVerify(allUsers.value(datas[i]->verify)->getName());       //审核者
            else
                p1->setVerify("");
            if(datas[i]->bookKeeper != 0)
                p1->setBookKeeper(allUsers.value(datas[i]->bookKeeper)->getName());//记账者
            else
                p1->setBookKeeper("");
            p1->setBaList(datas[i]->baLst);
            p1->setJDSums(datas[i]->jsum, datas[i]->dsum);
            p1->resize(pageW,pageH/2-MIDGAP/2);
            if(PzPrintTemplate::TvHeight == 0){
                p1->render(&pixmap);
                p1->setTvHeight();
            }
            p1->resize(pageW,pageH/2-MIDGAP/2);
            p1->render(&pixmap);
            paint->save();
            paint->setPen(Qt::DotLine);
            paint->drawPixmap(QPoint(x,y),pixmap);            
            //y += (pageH/2 + MIDGAP/2);
            y += pageH/2;
            paint->drawLine(QPoint(0,y),QPoint(pageW,y));
            y += MIDGAP/2;
            paint->restore();

        }
    }

}

void PrintPzUtils::setPzDatas(QList<PzPrintData2 *> datas)
{
    this->datas = datas;
}

void PrintPzUtils::setCompanyName(QString name)
{
    company = name;
}

void PrintPzUtils::setRates(QHash<int, Double> rates)
{
    this->rates = rates;
}
