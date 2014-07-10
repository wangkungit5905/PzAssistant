#include <QMessageBox>
#include <QHeaderView>
#include <QScrollBar>
#include <QPainter>
#include <QStandardItemModel>

#include "printUtils.h"
#include "printtemplate.h"
#include "common.h"
#include "global.h"
#include "pz.h"
#include "account.h"

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


PrintPzUtils::PrintPzUtils(Account *account, QPrinter* printer)
    :account(account)
{
    this->printer = printer;
    parameter = new PzTemplateParameter;
    AppConfig::getInstance()->getPzTemplateParameter(parameter);
    //printer->setPageMargins(0,0,0,0,QPrinter::Millimeter);//设置页边距
    printer->setFullPage(true);
    pageW = printer->paperRect().width();
    pageH = printer->paperRect().height();
    int ps_h = pageW/210;
    int ps_v = pageH/297;
    parameter->baRowHeight = ps_v * parameter->baRowHeight;
    parameter->titleHeight = ps_v * parameter->titleHeight;
    parameter->leftRightMargin = ps_h * parameter->leftRightMargin;
    parameter->topBottonMargin = ps_v * parameter->topBottonMargin;
    parameter->cutAreaHeight = ps_v * parameter->cutAreaHeight;
    int tw = pageW - parameter->leftRightMargin*2;
    parameter->factor[0] = parameter->factor[0] * tw;
    parameter->factor[1] = parameter->factor[1] * tw;
    parameter->factor[2] = parameter->factor[2] * tw;
    parameter->factor[3] = parameter->factor[3] * tw;
    tp = new PzPrintTemplate(parameter/*,paint*/);
}

PrintPzUtils::~PrintPzUtils()
{
    delete parameter;
    delete tp;
}


void PrintPzUtils::print(QPrinter* printer)
{
    if(printer != NULL){
        int mapW = pageW - parameter->leftRightMargin*2;
        int mapH = pageH/2 - parameter->topBottonMargin*2 - parameter->cutAreaHeight/2;
        QPixmap pixmap(mapW,mapH);
        tp->render(&pixmap);
        double scaleX = mapW/(double(tp->width()));
        double scaleY = mapH/(double(tp->height())+2);
        QPainter paint(printer);
        if(datas.count() < 3)
            printPage(scaleX,scaleY,&paint,0);
        else{
            printPage(scaleX,scaleY,&paint,0);
            for(int i = 2; i < datas.count(); i+=2){
                printPage(scaleX,scaleY,&paint,i,true);
            }
        }
    }
}

void PrintPzUtils::printPage(double scaleX, double scaleY, QPainter* paint, int index, bool newPage)
{
    if(newPage)
        printer->newPage();
    //绘制中线和裁剪线
    paint->save();
    paint->setPen(Qt::DotLine);
    int y = pageH/2;
    int x1 = parameter->leftRightMargin;
    int x2 = pageW-parameter->leftRightMargin;
    paint->drawLine(QPoint(x1,y),QPoint(x2,y));
    y -= parameter->cutAreaHeight/2;
    paint->drawLine(QPoint(x1,y),QPoint(x2,y));
    y = pageH/2 + parameter->cutAreaHeight/2;
    paint->drawLine(QPoint(x1,y),QPoint(x2,y));

    PzPrintData* pd;
    for(int i = index; i < index+2; ++i){
        if(i < datas.count()){
            tp->setMasterMoneyType(account->getMasterMt());
            tp->setRates(rates);
            tp->setCompany(account->getLName());   //单位名称
            pd = datas.at(i);
            tp->setPzDate(pd->date);   //凭证日期
            tp->setAttNums(pd->attNums);     //附件数
            tp->setPzNum(pd->pzNum);         //凭证号
            tp->setProducer(pd->producer?pd->producer->getName():"");
            tp->setVerify(pd->verify?pd->verify->getName():"");
            tp->setBookKeeper(pd->bookKeeper?pd->bookKeeper->getName():"");
            tp->setBaList(pd->baLst);
            tp->setJDSums(pd->jsum, pd->dsum);
            if(i == index)
                paint->translate(printer->paperRect().x()+parameter->leftRightMargin+3,
                                 printer->paperRect().y()+parameter->topBottonMargin);

            else
                paint->translate(0,tp->height()*scaleY+parameter->topBottonMargin*2+parameter->cutAreaHeight);
            paint->save();
            paint->scale(scaleX,scaleY);
            tp->render(paint);
            paint->restore();
        }
    }
    paint->restore();
}

/**
 * @brief PrintPzUtils::setPzDatas
 *  设置要打印的凭证对象列表
 * @param pzs
 */
void PrintPzUtils::setPzs(QList<PingZheng*> pzs)
{
    this->pzs = pzs;
    if(!datas.isEmpty()){
        qDeleteAll(datas);
        datas.clear();
    }
    //这里为了简化，因为打印凭证一般都是在一个凭证集内的凭证，所有它们的汇率都是相同的
    if(pzs.isEmpty())
        return;
    PingZheng* pz = pzs.first();
    if(!pz)
        return;
    if(!account->getRates(pz->getDate2().year(),pz->getDate2().month(),rates)){
        QMessageBox::warning(0,tr("警告信息"),tr("无法获取待打印凭证所采用的汇率！"));
        return;
    }
    int pages;    //每个凭证对象需要几页
    int bac;
    int baIndex;
    BusiAction* ba;
    for(int i = 0; i < pzs.count(); ++i){
        pz = pzs.at(i);
        bac = pz->baCount();
        baIndex = 0;
        if((bac % parameter->baRows) == 0)
            pages = bac / parameter->baRows;
        else
            pages = bac / parameter->baRows + 1;

        Double jsum = 0.00,dsum = 0.00; //借贷合计值
        for(int i = 0; i < pages; ++i){
            PzPrintData* pd = new PzPrintData;
            pd->date = pz->getDate2();     //凭证日期
            pd->attNums = pz->encNumber(); //附件数
            if(pages == 1)
                pd->pzNum = QString::number(pz->number());
            else
                pd->pzNum = QString::number(pz->number()) + '-' + QString("%1/%2").arg(i+1).arg(pages);

            int num = 0; //已提取的会计分录数
            while((num < parameter->baRows) && (baIndex < bac)){
                ba = pz->getBusiAction(baIndex++);
                pd->baLst<<ba;
                num++;
                if(ba->getDir() == MDIR_J)
                    jsum += ba->getValue() * rates.value(ba->getMt()->code(), 1.0);
                else
                    dsum += ba->getValue() * rates.value(ba->getMt()->code(), 1.0);
            }
            pd->jsum = jsum;
            pd->dsum = dsum;
            pd->producer = pz->recordUser();    //制单者
            pd->verify = pz->verifyUser();      //审核者
            pd->bookKeeper = pz->bookKeeperUser();  //记账者
            datas<<pd;
        }
    }
}
