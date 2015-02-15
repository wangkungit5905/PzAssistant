#include "lookysyfitemform.h"
#include "pzdialog.h"
#include "dbutil.h"
#include "tables.h"
#include "utils.h"
#include "ui_lookysyfitemform.h"

#include <QMouseEvent>
#include <QMenu>

LookYsYfItemForm::LookYsYfItemForm(Account *account, QWidget *parent) : QWidget(parent),
    ui(new Ui::LookYsYfItemForm),account(account)
{
    ui->setupUi(this);
    ui->tw->setColumnWidth(TI_MONEY,50);
    setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::Tool);
    //setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    mousePressed = false;
    actClose = new QAction(tr("关闭"),this);
    connect(actClose,SIGNAL(triggered()),this,SLOT(closeWindow()));
    isNormal = false;
    ui->detailWidget->setHidden(true);
    ui->icon->setHidden(false);

    db = account->getDbUtil()->getDb();
    mtTypes = account->getAllMoneys();
    _fsub = 0; _ssub = 0;
    this->parent = qobject_cast<PzDialog*>(parent);
    _turnOn();
    connect(ui->cmbYear,SIGNAL(currentIndexChanged(int)),this,SLOT(yearChanged(int)));
}

LookYsYfItemForm::~LookYsYfItemForm()
{
    delete ui;
}

void LookYsYfItemForm::mouseMoveEvent(QMouseEvent *e)
{
    if (mousePressed && (e->buttons() && Qt::LeftButton)) {
        this->move(e->globalPos() - mousePoint);
        e->accept();
    }
}

void LookYsYfItemForm::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton){
        mousePressed = true;
        mousePoint = e->globalPos() - this->pos();
        e->accept();
    }
    else if(e->button() == Qt::RightButton && !isNormal){
        QMenu menu;
        menu.addAction(actClose);
        menu.exec(e->globalPos());
    }
}

void LookYsYfItemForm::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
        mousePressed = false;
}

void LookYsYfItemForm::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(!isNormal){
        isNormal = true;
        ui->icon->setHidden(true);
        ui->detailWidget->setHidden(false);
    }
    show();
}

void LookYsYfItemForm::closeWindow()
{
    close();
    if(parent)
        parent->LookYsYfFormTellBye();
}

void LookYsYfItemForm::yearChanged(int index)
{
    if(index == -1)
        return;
    int y = ui->cmbYear->itemData(index).toInt();
    int i = 0;
    while(i < _range.count()){
        if(_range.at(i) == y){
            _turnOn(false);
            AccountSuiteRecord* asr = account->getSuiteRecord(y);
            ui->spnUp->setMinimum(asr->startMonth);
            ui->spnUp->setMaximum(_range.at(i+2));
            ui->spnUp->setValue(_range.at(i+1));
            ui->spnDown->setMinimum(_range.at(i+1));
            ui->spnDown->setMaximum(asr->endMonth);
            ui->spnDown->setValue(_range.at(i+2));
            _turnOn();
            break;
        }
        i += 3;
    }
}

/**
 * @brief 记录开始或结束月份的改变
 * @param i
 */
void LookYsYfItemForm::monthChanged(int m)
{
    if(_range.isEmpty())
        return;
    int y = ui->cmbYear->currentData(Qt::UserRole).toInt();
    int i = 0;
    while(i < _range.count()){
        if(_range.at(i) == y)
            break;
        i+=3;
    }
    QSpinBox* spn = qobject_cast<QSpinBox*>(sender());
    if(spn == ui->spnUp){
        _range[i+1] = m;
        ui->spnDown->setMinimum(m);
    }
    else if(spn == ui->spnDown){
        _range[i+2] = m;
        ui->spnUp->setMaximum(m);
    }
}

void LookYsYfItemForm::show()
{
    if(isNormal)
        resize(500,300);
    else
        resize(36,36);
    QWidget::show();
}

/**
 * @brief 在指定年月范围内查找对应子目和发票号的应收或应付分录
 * @param fsub
 * @param ssub
 * @param timeRange     //搜索的时间范围（键为年份，值为对应年份内的搜索月份列表）
 * @param invoiceNums   //对应的发票号
 */
void LookYsYfItemForm::findItem(FirstSubject* fsub, SecondSubject* ssub, QHash<int,QList<int> >timeRange, QList<QStringList> invoiceNums)
{
    if(!fsub || !ssub || timeRange.isEmpty() || invoiceNums.isEmpty())
        return;
    _fsub = fsub;
    ui->fsub->setText(fsub->getName());
    _ssub = ssub;
    ui->ssub->setText(ssub->getName());
    _range.clear();
    _invoiceNums.clear();
    foreach(QStringList nums,invoiceNums)
        _invoiceNums<<nums;

    QList<int> years = timeRange.keys();
    qSort(years.begin(),years.end());
    disconnect(ui->cmbYear,SIGNAL(currentIndexChanged(int)),this,SLOT(yearChanged(int)));
    ui->cmbYear->clear();
    for(int i = 0; i < years.count(); ++i){
        int y = years.at(i);
        _range<<y;
        ui->cmbYear->addItem(QString::number(y),y);
        QList<int> ms = timeRange.value(y);
        qSort(ms.begin(),ms.end());
        _range<<ms.first(); _range<<ms.last();
    }
    connect(ui->cmbYear,SIGNAL(currentIndexChanged(int)),this,SLOT(yearChanged(int)));
    if(ui->cmbYear->count() > 1)
        ui->cmbYear->setCurrentIndex(ui->cmbYear->count()-1);
    else
        yearChanged(0);
    _search();
}

void LookYsYfItemForm::on_btnMin_clicked()
{
    isNormal = false;
    ui->detailWidget->setHidden(true);
    ui->icon->setHidden(false);
    show();
}

void LookYsYfItemForm::on_btnSearch_clicked()
{
    _search();
}

void LookYsYfItemForm::_search()
{
    ui->tw->setRowCount(0);
    QSqlQuery q(db);
    QStringList timeFilters;//时间过滤条件
    for(int i = 0; i < _range.count()/3; i++){
        int y = _range.at(i*3);
        int sm = _range.at(i*3+1);
        int em = _range.at(i*3+2);
        QDate sd(y,sm,1);
        QDate ed(y,em,1);
        ed.setDate(y,em,ed.daysInMonth());
        timeFilters<<QString("(%1.%2 >= '%3' and %1.%2 <= '%4')").arg(tbl_pz).arg(fld_pz_date)
                     .arg(sd.toString(Qt::ISODate)).arg(ed.toString(Qt::ISODate));
    }
    //科目过滤，指定的一级科目和二级科目，并且金额发生方向是借方（应收）或贷方（应付）
    QString baFilters = QString("%1.%2=%5 and %1.%3=%6 and %1.%4=%7").arg(tbl_ba)
            .arg(fld_ba_fid).arg(fld_ba_sid).arg(fld_ba_dir).arg(_fsub->getId())
            .arg(_ssub->getId()).arg(_fsub->getJdDir()?MDIR_J:MDIR_D);
    QString orderby = QString("%1.%3,%1.%4,%2.%5").arg(tbl_pz).arg(tbl_ba)
            .arg(fld_pz_date).arg(fld_pz_number).arg(fld_ba_number);
    QString s = QString("select %1.id,%1.%3,%1.%4,%1.%5,%2.%6,%2.%7 from "
                        "%1 join %2 on %1.%8=%2.id where (%9) and %10 order by %11")
            .arg(tbl_ba).arg(tbl_pz).arg(fld_ba_summary).arg(fld_ba_value)
            .arg(fld_ba_mt).arg(fld_pz_date).arg(fld_pz_number).arg(fld_ba_pid)
            .arg(timeFilters.join(" or ")).arg(baFilters).arg(orderby);

    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return;
    }
    int row = 0;
    Double sumv,sumwv;
    QStringList invoiceNums = _invoiceNums;
    while(q.next()){
        QString summary = q.value(1).toString();
        QString invoiceNum;Double wv;bool isYs;
        PaUtils::extractUSD(summary,isYs,invoiceNum,wv);
        if(!invoiceNums.contains(invoiceNum))
            continue;
        invoiceNums.removeOne(invoiceNum);
        Double v = Double(q.value(2).toDouble());
        //此处未考虑采用外币的情形
        Money* mt = mtTypes.value(q.value(3).toInt());
        if(!mt){
            LOG_SQLERROR(QString("fond a invalid money code in table %1(rowid=%2)")
                         .arg(tbl_ba).arg(q.value(0).toInt()));
            return;
        }
        QString date = q.value(4).toString();
        int pzNum = q.value(5).toInt();
        ui->tw->insertRow(row);
        QTableWidgetItem* item = new QTableWidgetItem(QString("%1(%2#)").arg(date).arg(pzNum));
        ui->tw->setItem(row,TI_DATE,item);
        item = new QTableWidgetItem(invoiceNum);
        item->setToolTip(summary);
        ui->tw->setItem(row,TI_IVNUM,item);
        item = new QTableWidgetItem(v.toString2());
        ui->tw->setItem(row,TI_VALUE,item);
        item = new QTableWidgetItem(mt->name());
        ui->tw->setItem(row,TI_MONEY,item);
        item = new QTableWidgetItem(wv.toString());
        ui->tw->setItem(row,TI_WVALUE,item);
        sumv += v; sumwv += wv;
        row++;
    }
    if(!invoiceNums.isEmpty()){
        for(int i = 0; i < invoiceNums.count(); ++i){
            ui->tw->insertRow(row);
            QTableWidgetItem* item = new QTableWidgetItem(invoiceNums.at(i));
            item->setForeground(QBrush(Qt::red));
            ui->tw->setItem(row,TI_IVNUM,item);
            row++;
        }
    }
    if(ui->tw->rowCount() > 0){
        ui->tw->insertRow(row);
        QTableWidgetItem* item = new QTableWidgetItem(tr("合计"));
        ui->tw->setItem(row,TI_DATE,item);
        item = new QTableWidgetItem(sumv.toString2());
        ui->tw->setItem(row,TI_VALUE,item);
        item = new QTableWidgetItem(sumwv.toString());
        ui->tw->setItem(row,TI_WVALUE,item);
    }
}

void LookYsYfItemForm::_turnOn(bool on)
{
    if(on){
        connect(ui->spnUp,SIGNAL(valueChanged(int)),this,SLOT(monthChanged(int)));
        connect(ui->spnDown,SIGNAL(valueChanged(int)),this,SLOT(monthChanged(int)));
    }
    else{
        disconnect(ui->spnUp,SIGNAL(valueChanged(int)),this,SLOT(monthChanged(int)));
        disconnect(ui->spnDown,SIGNAL(valueChanged(int)),this,SLOT(monthChanged(int)));
    }
}
