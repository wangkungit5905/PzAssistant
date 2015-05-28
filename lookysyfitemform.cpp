#include "lookysyfitemform.h"
#include "pzdialog.h"
#include "dbutil.h"
#include "tables.h"
#include "utils.h"
#include "account.h"
#include "ui_lookysyfitemform.h"

#include <QMouseEvent>
#include <QMenu>

IntItemDelegate::IntItemDelegate(Account *account, QWidget *parent)
    :QItemDelegate(parent),_account(account)
{

}

QWidget *IntItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int col = index.column();
    if(col == CI_JOIN || col == CI_YEAR)
        return 0;
    int row = index.row();
    int year = index.model()->data(index.model()->index(row,CI_YEAR)).toInt();
    if(year == 0)
        return 0;
    AccountSuiteRecord* asr = _account->getSuiteRecord(year);
    if(!asr)
        return 0;
    QSpinBox* editor = new QSpinBox(parent);
    if(col == CI_SMONTH){
        editor->setMinimum(asr->startMonth);
        editor->setMaximum(index.model()->data(index.model()->index(row,CI_EMONTH)).toInt());
    }
    else{
        editor->setMinimum(index.model()->data(index.model()->index(row,CI_SMONTH)).toInt());
        editor->setMaximum(asr->endMonth);
    }
    return editor;
}

void IntItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QSpinBox* edit = qobject_cast<QSpinBox*>(editor);
    if(edit)
        edit->setValue(index.model()->data(index, Qt::EditRole).toInt());
}

void IntItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSpinBox* edit = qobject_cast<QSpinBox*>(editor);
    if(edit){
        if(edit->value() != model->data(index,Qt::EditRole).toInt())
            model->setData(index, edit->value());
    }
}

void IntItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    editor->setGeometry(rect);
}


////////////////////////////////////////////////////////////////////////
LookYsYfItemForm::LookYsYfItemForm(Account *account, QWidget *parent) : QWidget(parent),
    ui(new Ui::LookYsYfItemForm),account(account)
{
    ui->setupUi(this);
    _iconPix = QPixmap(":/images/accountRefresh.png");
    ui->icon->setPixmap(_iconPix);
    sc_look = new QShortcut(QKeySequence("F12"),this);
    connect(sc_look,SIGNAL(activated()),this,SLOT(hide()));
    ui->tw->setColumnWidth(TI_MONEY,50);
    setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::Tool);
    //setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    mousePressed = false;
    actQuit = new QAction(tr("关闭"),this);
    connect(actQuit,SIGNAL(triggered()),this,SLOT(closeWindow()));
    isNormal = false;
    ui->detailWidget->setHidden(true);
    ui->icon->setHidden(false);

    db = account->getDbUtil()->getDb();
    mtTypes = account->getAllMoneys();
    _fsub = 0; _ssub = 0;
    this->parent = qobject_cast<PzDialog*>(parent);
    _flickeTimer.setSingleShot(false);
    _flickeTimer.setInterval(500);
    _catchTimer.setSingleShot(true);
    _catchTimer.setInterval(500);
    connect(&_flickeTimer,SIGNAL(timeout()),this,SLOT(flickerIcon()));
    connect(&_catchTimer,SIGNAL(timeout()),this,SLOT(catchMouse()));

    QList<AccountSuiteRecord*> rs = account->getAllSuiteRecords();
    ui->twRange->setRowCount(rs.count());
    int row=0;
    for(int i = rs.count()-1; i >= 0; i--,row++){
        QCheckBox* chk = new QCheckBox(this);
        ui->twRange->setCellWidget(row,IntItemDelegate::CI_JOIN,chk);
        AccountSuiteRecord* asr = rs.at(i);
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(asr->year));
        ui->twRange->setItem(row,IntItemDelegate::CI_YEAR,item);
        item = new QTableWidgetItem(QString::number(asr->startMonth));
        ui->twRange->setItem(row,IntItemDelegate::CI_SMONTH,item);
        item = new QTableWidgetItem(QString::number(asr->endMonth));
        ui->twRange->setItem(row,IntItemDelegate::CI_EMONTH,item);
    }
    IntItemDelegate* delegate = new IntItemDelegate(account,this);
    ui->twRange->setItemDelegate(delegate);
}

LookYsYfItemForm::~LookYsYfItemForm()
{
    delete ui;
}

void LookYsYfItemForm::leaveEvent(QEvent *event)
{
    QMouseEvent* e = static_cast<QMouseEvent*>(event);
    if(!e)
        return;
    if(!isNormal){
        _catchTimer.stop();
        return;
    }
    _catchTimer.start();
}

void LookYsYfItemForm::enterEvent(QEvent *event)
{
    QMouseEvent* e = static_cast<QMouseEvent*>(event);
    if(!e)
        return;
    if(isNormal){
        _catchTimer.stop();
        return;
    }
    _catchTimer.start();
}

void LookYsYfItemForm::mouseMoveEvent(QMouseEvent *e)
{
    if (mousePressed && (e->buttons() && Qt::LeftButton)) {
        this->move(e->globalPos() - mousePoint);
        if(_catchTimer.isActive())
            _catchTimer.stop();
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
        menu.addAction(actQuit);
        menu.exec(e->globalPos());
    }
}

void LookYsYfItemForm::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
        mousePressed = false;
}

void LookYsYfItemForm::closeWindow()
{
    close();
    if(parent)
        parent->LookYsYfFormTellBye();
}

void LookYsYfItemForm::flickerIcon()
{
    static int count = 0;
    if(count == 6){
        _flickeTimer.stop();
        return;
    }
    if(count%2 == 1)
        ui->icon->setPixmap(_iconPix);
    else
        ui->icon->setPixmap(QPixmap());
    count++;
}

/**
 * @brief 捕获鼠标的进入和退出事件（进入或退出窗口区域达指定的时间间隔）
 */
void LookYsYfItemForm::catchMouse()
{
    if(!isNormal){
        isNormal = true;
        ui->icon->setHidden(true);
        ui->detailWidget->setHidden(false);
        if(_flickeTimer.isActive()){
            _flickeTimer.stop();
            ui->icon->setPixmap(_iconPix);
        }
    }
    else{
        isNormal = false;
        ui->detailWidget->setHidden(true);
        ui->icon->setHidden(false);
    }
    show();
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
    if(!fsub || !ssub || timeRange.isEmpty() || invoiceNums.isEmpty()){
        ui->btnSearch->setEnabled(false);
        return;
    }
    ui->btnSearch->setEnabled(true);
    _fsub = fsub;
    ui->fsub->setText(fsub->getName());
    _ssub = ssub;
    ui->ssub->setText(ssub->getName());
    _invoiceNums.clear();
    foreach(QStringList nums,invoiceNums)
        _invoiceNums<<nums;
    for(int row=0; row < ui->twRange->rowCount(); ++row){
        QCheckBox* chk = qobject_cast<QCheckBox*>(ui->twRange->cellWidget(row,IntItemDelegate::CI_JOIN));
        if(!chk)
            continue;
        int year = ui->twRange->item(row,IntItemDelegate::CI_YEAR)->text().toInt();
        chk->setChecked(timeRange.contains(year));
        if(chk->isChecked()){
            QList<int> ms = timeRange.value(year);
            qSort(ms.begin(),ms.end());
            ui->twRange->item(row,IntItemDelegate::CI_SMONTH)->setText(QString::number(ms.first()));
            ui->twRange->item(row,IntItemDelegate::CI_EMONTH)->setText(QString::number(ms.last()));
        }
    }
    _search();
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
    for(int row = 0; row < ui->twRange->rowCount(); row++){
        QCheckBox* chk = qobject_cast<QCheckBox*>(ui->twRange->cellWidget(row,IntItemDelegate::CI_JOIN));
        if(!chk || !chk->isChecked())
            continue;
        int y = ui->twRange->item(row,IntItemDelegate::CI_YEAR)->text().toInt();
        int sm = ui->twRange->item(row,IntItemDelegate::CI_SMONTH)->text().toInt();
        int em = ui->twRange->item(row,IntItemDelegate::CI_EMONTH)->text().toInt();
        QDate sd(y,sm,1);
        QDate ed(y,em,1);
        ed.setDate(y,em,ed.daysInMonth());
        timeFilters<<QString("(%1.%2 >= '%3' and %1.%2 <= '%4')").arg(tbl_pz).arg(fld_pz_date)
                     .arg(sd.toString(Qt::ISODate)).arg(ed.toString(Qt::ISODate));
    }
    if(timeFilters.isEmpty())
        return;
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
    //闪烁图标以提醒用户，我已经帮你找到了对应的应收应付项
    if(!isNormal && ui->tw->rowCount() > 0)
        _flickeTimer.start();
}

void LookYsYfItemForm::on_btnQuit_clicked()
{
    closeWindow();
}


