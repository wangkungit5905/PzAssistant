#include "impjournarydlg.h"
#include "ui_impjournarydlg.h"

#include "subject.h"
#include "xlsxworksheet.h"
#include "myhelper.h"
#include "commdatastruct.h"
#include "PzSet.h"
#include "dbutil.h"

#include <QMenu>
#include <QDir>
#include <QFileDialog>
#include <QTableWidget>
#include <QSettings>
#include <QRegExp>

ImpJournaryDlg::ImpJournaryDlg(SubjectManager* sm, QWidget *parent) :
    QDialog(parent),ui(new Ui::ImpJournaryDlg),sm(sm),excel(0)
{
    ui->setupUi(this);
    init();
    initJs();
}

ImpJournaryDlg::~ImpJournaryDlg()
{
    delete ui;
}


void ImpJournaryDlg::init()
{
    titles<<"日期"<<"摘要"<<"收入"<<"支出"<<"余额"<<"发票号"<<"备注"<<"审核";
    icon_bank = QIcon(":/images/bank.png");
    icon_cash = QIcon(":/images/cash.png");
    connect(ui->lwSheets,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(sheetListContextMeny(QPoint)));
    connect(ui->lwSheets,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(doubleItemReadSheet(QListWidgetItem*)));

    ag1 = new QActionGroup(this);
    QAction* act = new QAction(tr("现金-人民币"),this);
    ag1->addAction(act);
    SecondSubject* sub = sm->getCashSub()->getChildSub(0);
    actionMap[act] = sub;
    FirstSubject* bankFSub = sm->getBankSub();
    QList<SecondSubject*> bankSubs = bankFSub->getChildSubs();
    for(int i = 0; i < bankFSub->getChildCount(); ++i){
        sub = bankFSub->getChildSub(i);
        if(!sub->isEnabled())
            continue;
        act = new QAction(sub->getName(),this);
        ag1->addAction(act);
        actionMap[act] = sub;
    }
    foreach(QAction* act, ag1->actions()){
        act->setCheckable(true);
        connect(act,SIGNAL(toggled(bool)),this,SLOT(processSubChanged(bool)));
    }
    ui->lwSheets->addActions(ag1->actions());

    kwSetting = new QSettings("./config/app/keywords.ini", QSettings::IniFormat);
    kwSetting->setIniCodec("UTF-8");
    kwSetting->beginGroup("TitleKeywords");
    kwBeginBlas = kwSetting->value("BeginBalance").toStringList();
    kwSetting->endGroup();
}

/**
 * @brief 读取先前已导入的流水账数据
 */
void ImpJournaryDlg::initJs()
{
    AccountSuiteManager* smgr = sm->getAccount()->getSuiteMgr();
    DbUtil* dbt = sm->getAccount()->getDbUtil();
    int y = smgr->year();
    int m = smgr->month();
    QList<QList<Journal*>* > jjs;
    QList<SecondSubject* > subs;
    if(!dbt->readJournays(y,m,subs,jjs,sm)){
        myHelper::ShowMessageBoxWarning(tr("读取流水账出错!"));
        return;
    }
    if(subs.isEmpty())
        return;
    for(int i = ui->stackedWidget->count()-1; i >= 0; i--){
        QWidget* w = ui->stackedWidget->widget(i);
        ui->stackedWidget->removeWidget(w);
    }
    for(int t = 0; t < subs.count(); ++t){
        SecondSubject* sub = subs.at(t);
        QList<Journal* >* js = jjs.at(t);
        QListWidgetItem* li = new QListWidgetItem(sub->getName(),ui->lwSheets);
        li->setData(DR_READED,true);
        QVariant v;
        v.setValue<SecondSubject*>(sub);
        li->setData(DR_SUBJECT,v);
        li->setData(Qt::ToolTipRole,sub->getName());
        if(sm->isBankSndSub(sub))
            li->setData(Qt::DecorationRole,icon_bank);
        else
            li->setData(Qt::DecorationRole,icon_cash);
        QTableWidget* tw = new QTableWidget(this);
        tw->setColumnCount(titles.count());
        tw->setHorizontalHeaderLabels(titles);
        tw->setRowCount(js->count());
        tw->setSelectionBehavior(QAbstractItemView::SelectRows);
        for(int r = 0; r < js->count(); ++r)
            rendRow(r,js->at(r),tw);
        ui->stackedWidget->addWidget(tw);
    }
    connect(ui->lwSheets,SIGNAL(currentRowChanged(int)),this,SLOT(curSheetChanged(int)));
}

/**
 * 读取流水账
 * @param  index [description]
 * @param  tw    [description]
 * @return       [description]
 */
bool ImpJournaryDlg::readSheet(int index, QTableWidget* tw)
{
    QString sheetName = ui->lwSheets->item(index)->text();
    excel->selectSheet(sheetName);
    Worksheet* sheet = excel->currentWorksheet();
    int colDate = -1;
    int colSummary = -1;
    int colIncome = -1;
    int colPay = -1;
    int colBalance = -1;
    int colInvoice = -1;
    int colRemark = -1;
    QString strDate = tr("日期");
    QString strSummary = tr("摘要");
    QString strIncome = tr("收入（借方）");
    QString strPay = tr("付出（贷方）");
    QString strBalance = tr("余额");
    QString strInvoice = tr("发票号");
    QString strRemark = tr("备注");
    bool isCash = false;
    Cell* cell = sheet->cellAt(1,1);
    if(!cell){
        myHelper::ShowMessageBoxWarning(tr("报表的第一行必须是表示银行或现金的有效标题栏！"));
        return false;
    }
    QString t = cell->value().toString();
    t.replace(" ","");
    if(t.contains(tr("现金")))
        isCash = true;

    int titleRow = 2;    //标题栏行
    for(int c = 0; c < 10; ++c){
        cell = sheet->cellAt(titleRow,c);
        if(!cell)
            continue;
        QString v = cell->value().toString();
        v.replace('(',tr("（"));
        v.replace(')',tr("）"));
        if(v == strDate)
            colDate = c;
        else if(v == strSummary)
            colSummary = c;
        else if(v == strIncome)
            colIncome = c;
        else if(v == strPay)
            colPay = c;
        else if(v == strBalance)
            colBalance = c;
        else if(v == strInvoice)
            colInvoice = c;
        else if(v == strRemark)
            colRemark = c;
    }
    QStringList errors;
    if(colDate == -1)
        errors<<strDate;
    if(colSummary == -1)
        errors<<strSummary;
    if(colIncome == -1)
        errors<<strIncome;
    if(colPay == -1)
        errors<<strPay;
    if(colBalance == -1)
        errors<<strBalance;
    if(colInvoice == -1)
        errors<<strInvoice;
    if(colRemark == -1)
        errors<<strRemark;
    if(!errors.isEmpty()){
        QString e = tr("无法定位如下列：\n");
        foreach (QString s, errors)
            e.append(s+"\n");
        myHelper::ShowMessageBoxWarning(e);
        return false;
    }

    QDate primaryDay(1900,1,1);
    qint64 pdInt = primaryDay.toJulianDay()-2; //日期基数（Excel中的日期的内部是用一个数字，即从1900年1月1日开始到此日期的天数）
    QList<int> cols;
    cols<<colDate<<colSummary<<colIncome<<colPay<<colInvoice<<colRemark;
    Double v;
    QList<Journal*> js;

    //定位月度流水账的开始行和结束行，从第4行开始是有效的流水账数据行
    int curY = sm->getAccount()->getSuiteMgr()->year();
    int curM = sm->getAccount()->getSuiteMgr()->month();
    QDate sDay = QDate(curY,curM,1);
    QDate eDay = QDate(curY,curM,sDay.daysInMonth());
    int row=3,startRow=0,endRow=0;
    bool ok = false;
    while(!endRow){
        row++;
        cell = sheet->cellAt(row,colDate);
        if(!cell){
            endRow = row;
            break;
        }
        qint64 dv = cell->value().toLongLong(&ok);
        if(ok && dv != 0){
            dv += pdInt;
            QDate d = QDate::fromJulianDay(dv);
            if(d.isNull() || !d.isValid()){
                endRow = row;
                break;
            }
            if(d < sDay)
                continue;
            else if(!startRow && d >= sDay && d <= eDay){
                startRow = row;
                continue;
            }
            else if(d > eDay){
                endRow = row;
                break;
            }
        }
        else{
            myHelper::ShowMessageBoxWarning(tr("第%1行出现无效的日期格式！").arg(row));
            endRow = row;
            break;
        }
    }
    if(!startRow || !endRow){
        myHelper::ShowMessageBoxWarning(tr("无法定位%1年%2月的流水账的开始或结束行，请检查报表是否正确！").arg(curY).arg(curM));
        return false;
    }

    //读取期初余额
    cell = sheet->cellAt(startRow-1,colBalance);
    Journal* j = new Journal;
    j->id = 0;
    j->summary = tr("期初余额");
    j->balance = Double(cell->value().toDouble());
    j->priNum = 0;
    js<<j;
    //读取本月流水账
    for(int r = startRow; r < endRow; r++){
        j = new Journal;
        j->id = 0;
        j->priNum = r;
        foreach(int c,cols){            
            t="";
            cell = sheet->cellAt(r,c);
            if(!cell)
                continue;

            if(c == colDate){
                qint64 dv = cell->value().toLongLong(&ok);
                if(ok && dv != 0){
                    dv += pdInt;
                    QDate d = QDate::fromJulianDay(dv);
                    j->date = d.toString(Qt::ISODate);
                }
            }
            else if(c == colSummary || c == colInvoice || c == colRemark){
                t = cell->value().toString();
                if(c == colSummary)
                    j->summary = t;
                else if(c == colInvoice)
                    j->invoices = t;
                else
                    j->remark = t;
            }
            else if(c == colIncome || c == colPay){
                v = Double(cell->value().toDouble());
                if(v != 0){
                    j->value = v;
                    if(c == colIncome)
                        j->dir = MDIR_J;
                    else
                        j->dir = MDIR_D;
                }
            }
        }
        js<<j;
        int co = js.count();
        if(co>=2){
            Journal* jp = js.at(co-2);
            if(j->dir == MDIR_J)
                j->balance = jp->balance + j->value;
            else
                j->balance = jp->balance - j->value;
        }
    }

    //读取期末余额
    cell = sheet->cellAt(endRow-1,colBalance);
    if(!cell){
        myHelper::ShowMessageBoxWarning(tr("无法读取到期末余额！"));
        return false;
    }
    j = new Journal;
    j->id = 0;
    j->summary = tr("期末余额");
    j->balance = Double(cell->value().toDouble());
    j->priNum = row-1;
    js<<j;
    tw->setRowCount(js.count());
    for(int r=0; r < js.count(); ++r){
        j = js.at(r);
        rendRow(r,j,tw);
        if(r>0 && r<js.count()-1 && j->value == 0)
        	myHelper::ShowMessageBoxWarning(tr("第%1行 没有金额！").arg(j->priNum));
    }
    return true;
}

/**
 * @brief 在指定行渲染流水账
 * @param row
 * @param j
 * @param tw
 */
void ImpJournaryDlg::rendRow(int row, Journal *j, QTableWidget *tw)
{
    QTableWidgetItem* item = new QTableWidgetItem(j->date);
    QVariant v;
    v.setValue<Journal*>(j);
    item->setData(DR_OBJ,v);
    tw->setItem(row,TI_DATE,item);
    tw->setItem(row,TI_SUMMARY,new QTableWidgetItem(j->summary));
    if(j->dir == MDIR_J){
        tw->setItem(row,TI_INCOME,new QTableWidgetItem(j->value.toString()));
        tw->setItem(row,TI_PAY,new QTableWidgetItem);
    }
    else{
        tw->setItem(row,TI_INCOME,new QTableWidgetItem);
        tw->setItem(row,TI_PAY,new QTableWidgetItem(j->value.toString()));
    }
    tw->setItem(row,TI_BALANCE,new QTableWidgetItem(j->balance.toString()));
    tw->setItem(row,TI_INVOICE,new QTableWidgetItem(j->invoices));
    tw->setItem(row,TI_REMARK,new QTableWidgetItem(j->remark));
    tw->setItem(row,TI_VTAG,new QTableWidgetItem(j->vTag?"OK":"NOT"));
}

bool ImpJournaryDlg::isContainKeyword(QString t,QStringList kws)
{
    foreach(QString k, kws){
        if(t.contains(k))
            return true;
    }
    return false;
}

/**
 * @brief 文本t中是否包含发票号
 * @param t
 * @return
 */
bool ImpJournaryDlg::hasInvoice(QString t)
{
    QRegExp re("\\d{8}");
    return re.indexIn(t)!=-1;
}


void ImpJournaryDlg::sheetListContextMeny(const QPoint &pos)
{
    QListWidgetItem* li = ui->lwSheets->itemAt(pos);
    QMenu *m = new QMenu(this);
    m->addActions(ag1->actions());
    m->popup(ui->lwSheets->mapToGlobal(pos));
}

/**
 * 处理对银行/现金子目设定的改变
 * @param isChecked [description]
 */
void ImpJournaryDlg::processSubChanged(bool isChecked)
{
    if(!isChecked)
        return;
    QAction* act = qobject_cast<QAction*>(sender());
    SecondSubject* sub = actionMap.value(act);
    if(!sub)
        return;
    QListWidgetItem* item = ui->lwSheets->currentItem();
    QVariant v;
    v.setValue<SecondSubject*>(sub);
    item->setData(DR_SUBJECT,v);
    if(sm->isBankSndSub(sub)){
        item->setData(Qt::DecorationRole,icon_bank);
        //item->setData(Qt::ToolTipRole,sub->getLName());
    }
    else{
        item->setData(Qt::DecorationRole,icon_cash);
        //item->setData(Qt::ToolTipRole,sub->getName());
    }
    //item->setForeground(QBrush(QColor("blue")));
    item->setToolTip(sub->getName());
}

/**
 * 双击表单名，读取表单内容
 * @param item [description]
 */
void ImpJournaryDlg::doubleItemReadSheet(QListWidgetItem *item)
{
    if(item->data(DR_READED).toBool())
        return;
    int index = ui->lwSheets->row(item);
    ui->stackedWidget->setCurrentIndex(index);
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->widget(index));
    if(!readSheet(index,tw))
       myHelper::ShowMessageBoxError(tr("读取表单“%1”时出错！").arg(ui->lwSheets->item(index)->text()));
    else
       item->setData(DR_READED,true);
    ui->stackedWidget->setCurrentWidget(tw);
}

/**
 * @brief ImpJournaryDlg::curSheetChanged
 * @param index
 */
void ImpJournaryDlg::curSheetChanged(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
//    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->widget(index));
//    ui->stackedWidget->setCurrentWidget(tw);
}

/**
 * @brief 保存流水帳
 */
void ImpJournaryDlg::on_btnSave_clicked()
{
    QList<Journal* > js;
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    Journal* j;
    SecondSubject* ssub = ui->lwSheets->currentItem()->data(DR_SUBJECT).value<SecondSubject*>();
    if(!ssub){
        myHelper::ShowMessageBoxWarning(tr("未指定银行或现金科目！"));
        return;
    }
    for(int r = 1; r < tw->rowCount()-1; ++r){
        j = tw->item(r,TI_DATE)->data(DR_OBJ).value<Journal*>();
        if(j->id == 0){
            j->bankId = ssub->getId();
            js<<j;
        }
        else{
            bool edited = false;
            QString d = tw->item(r,TI_DATE)->text();
            if(d != j->date){
                j->date = d;
                edited = true;
            }
            d = tw->item(r,TI_SUMMARY)->text();
            if(d != j->summary){
                j->summary = d;
                edited = true;
            }
            Double vj = Double(tw->item(r,TI_INCOME)->text().toDouble());
            Double vd = Double(tw->item(r,TI_PAY)->text().toDouble());
            MoneyDirection dir = MDIR_J;
            Double v = vj;
            if(vd != 0){
                v = vd;
                dir = MDIR_D;
            }
            if(v != j->value){
                j->value = v;
                edited = true;
            }
            if(dir != j->dir){
                j->dir = dir;
                edited = true;
            }
            v = tw->item(r,TI_BALANCE)->text().toDouble();
            if(v != j->balance){
                j->balance = v;
                edited = true;
            }
            d = tw->item(r,TI_INVOICE)->text();
            if(d != j->invoices){
                j->invoices = d;
                edited = true;
            }
            d = tw->item(r,TI_REMARK)->text();
            if(d != j->remark){
                j->remark = d;
                edited = true;
            }
            if(edited){
                j->bankId = ssub->getId();
                js<<j;
            }
        }
    }
    if(js.isEmpty())
        return;
    DbUtil *dbUtil = sm->getAccount()->getDbUtil();
    if(!dbUtil->saveJournals(js))
        myHelper::ShowMessageBoxWarning(tr("保存流水账时出错！"));
}

void ImpJournaryDlg::on_btnBrower_clicked()
{
    QString dirName = QApplication::applicationDirPath()+"/files/"+sm->getAccount()->getSName()+"/";
    QDir d(dirName);
    if(!d.exists())
        d.mkpath(dirName);
    QString fileName = QFileDialog::getOpenFileName(this,tr("请选择记录流水账的Excel文件！"),dirName,"*.xlsx");
    if(fileName.isEmpty())
        return;
    ui->edtFilename->setText(fileName);
    if(excel){
        delete excel;
        excel = 0;
    }
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
    excel = new QXlsx::Document(fileName,this);
    ui->lwSheets->clear();
    for(int i = ui->stackedWidget->count()-1; i >= 0; i--){
        QWidget* w = ui->stackedWidget->widget(i);
        ui->stackedWidget->removeWidget(w);
    }
    QStringList sheets = excel->sheetNames();
    if(sheets.isEmpty())
        return;
    for(int i = 0; i < sheets.count(); ++i){
        QListWidgetItem* li = new QListWidgetItem(sheets.at(i),ui->lwSheets);
        li->setData(DR_READED,false);
        QTableWidget* tw = new QTableWidget(this);
        tw->setColumnCount(titles.count());
        tw->setHorizontalHeaderLabels(titles);
        tw->setSelectionBehavior(QAbstractItemView::SelectRows);
        //tw->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        //tw->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        // connect(tw->horizontalHeader(),SIGNAL(customContextMenuRequested(QPoint)),
        //         this,SLOT(tableHHeaderContextMenu(QPoint)));
        // connect(tw->verticalHeader(),SIGNAL(customContextMenuRequested(QPoint)),
        //         this,SLOT(tableVHeaderContextMenu(QPoint)));
        ui->stackedWidget->addWidget(tw);
    }
    qApp->restoreOverrideCursor();
}



