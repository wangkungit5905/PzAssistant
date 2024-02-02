#include <QKeyEvent>
#include <QBuffer>
#include <QInputDialog>
#include <QToolTip>
#include <QAction>

#include "pzdialog.h"
#include "cal.h"
#include "global.h"
#include "logs/Logger.h"
#include "commands.h"
#include "dialogs.h"
#include "PzSet.h"
#include "statements.h"
#include "completsubinfodialog.h"
#include "statutil.h"
#include "keysequence.h"
#include "widgets.h"
#include "myhelper.h"
#include "utils.h"
#include "lookysyfitemform.h"
#include "invoicestatform.h"
#include "batemplateform.h"

#include "ui_pzdialog.h"
#include "ui_historypzform.h"

////////////////////////////BaTableWidget////////////////////////////////////
BaTableWidget::BaTableWidget(QWidget *parent):QTableWidget(parent)
{
    //构造合计栏表格
    sumTable = new QTableWidget(this);
    sumTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sumTable->setMaximumHeight(60);
    sumTable->setColumnCount(4);
    sumTable->setRowCount(1);
    sumTable->horizontalHeader()->hide();
    sumTable->verticalHeader()->hide();
    sumTable->setFocusPolicy(Qt::NoFocus);//没有影响
    sumTable->setSelectionMode(QAbstractItemView::NoSelection);
    //初始化合计栏各列
    Qt::ItemFlags falgs = Qt::ItemIsEditable;
    falgs = ~falgs;
    lnItem = new QTableWidgetItem;
    lnItem->setFlags(falgs);
    sumTable->setItem(0,0,lnItem);
    QTableWidgetItem* item = new QTableWidgetItem(tr("合计"));
    item->setTextAlignment(Qt::AlignCenter);
    sumTable->setItem(0,1,item);

    jSumItem = new BAMoneyValueItem_new(DIR_J,0.00,QColor(Qt::red));
    QFont font = jSumItem->font();
    font.setBold(true);
    jSumItem->setFont(font);
    jSumItem->setForeColor(QColor("red"));
    jSumItem->setFlags(falgs);
    sumTable->setItem(0,2,jSumItem);
    dSumItem = new BAMoneyValueItem_new(DIR_D,0.00,QColor(Qt::red));
    dSumItem->setFont(font);
    dSumItem->setForeColor(QColor("red"));
    dSumItem->setFlags(falgs);
    sumTable->setItem(0,3,dSumItem);
    viewport()->stackUnder(sumTable);

//    //初始化快捷键
//    shortCut = new QShortcut(QKeySequence(tr("Ctrl+=")),this);
//    connect(shortCut,SIGNAL(activated()),this,SLOT(processShortcut()));


}

/**
 * @brief BaTableWidget::setValidRows
 * 设置垂直标题条（行号，有效行有数字标签，备用行用星号）
 * @param rows
 */
void BaTableWidget::setValidRows(int rows)
{
    validRows=rows;
    if(rows > rowCount())
        setRowCount(validRows + 10);
    if(rows <= 1)
        clearSum();

    QStringList hs;
    for(int i = 1; i < rows; ++i)
        hs<<QString::number(i);
    hs<<"*";
    for(int i = rows+1; i <= rowCount(); ++i)
        hs<<"";
    setVerticalHeaderLabels(hs);
}

/**
 * @brief BaTableWidget::setJSum 显示借方合计值
 * @param v
 */
void BaTableWidget::setJSum(Double v)
{
    jSumItem->setData(Qt::EditRole,v);
    QModelIndex index = sumTable->model()->index(0,2);
    sumTable->update(index);
}

/**
 * @brief BaTableWidget::setDSum 显示贷方合计值
 * @param v
 */
void BaTableWidget::setDSum(Double v)
{
    dSumItem->setData(Qt::EditRole,v);
    QModelIndex index = sumTable->model()->index(0,3);
    sumTable->update(index);
}

/**
 * @brief 清除合计栏的内容
 */
void BaTableWidget::clearSum()
{
    jSumItem->setData(Qt::EditRole,Double());
    dSumItem->setData(Qt::EditRole,Double());
    QModelIndex index = sumTable->model()->index(0,2);
    sumTable->update(index);
    index = sumTable->model()->index(0,3);
    sumTable->update(index);
}

/**
 * @brief BaTableWidget::setBalance 指示借贷是否平衡，true表示平衡，反之不平衡，用红色指示
 * @param isBalance
 */
void BaTableWidget::setBalance(bool isBalance)
{
    jSumItem->setOutControl(!isBalance);
    dSumItem->setOutControl(!isBalance);
}

/**
 * @brief BaTableWidget::setLongName 在合计栏的第一列显示二级科目全名
 * @param name
 */
void BaTableWidget::setLongName(QString name)
{
    lnItem->setText(name);
    QModelIndex index = sumTable->model()->index(0,0);
    sumTable->update(index);
}

/**
 * @brief BaTableWidget::switchRow 交换行
 * @param r1
 * @param r2
 */
void BaTableWidget::switchRow(int r1, int r2)
{
    if((r1 < 0) || (r2 < 0))
        return;
    if(r1 == r2)
        return;
    int ru,rd; //上面、下面一行的行序号
    if(r1 > r2){
        ru = r2; rd = r1;
    }
    else{
        ru = r1; rd = r2;
    }
    if(rd > rowCount()-1)
        return;

    QList<QTableWidgetItem*> items;
    //保存上面一行的表格项
    for(int i = 0; i < columnCount(); ++i)
        items.append(takeItem(ru,i));
    //将下面一行的表格项移到上面一行
    for(int i = 0; i < columnCount(); ++i)
        setItem(ru,i,takeItem(rd,i));
    //将保存的上面一行的表格项设置到下面一行
    for(int i = 0; i < items.count(); ++i)
        setItem(rd,i,items[i]);
}

/**
 * @brief BaTableWidget::isHasSelectedRows
 * 是否有选中行
 * @return
 */
bool BaTableWidget::isHasSelectedRows()
{
    bool found = false;
    QItemSelectionModel* smodel = selectionModel();
    if(smodel->hasSelection()){
       int i = 0;
       while((!found) && i < validRows){
           if(smodel->isRowSelected(i, QModelIndex()))
               found = true;
           i++;
       }
    }
    return found;
}

/**
 * @brief BaTableWidget::selectedRows
 * 获取选中的行
 * @param selRows：选中行的行号列表
 * @param isContinuous：选中的行是否是连续的
 */
void BaTableWidget::selectedRows(QList<int> &selRows, bool &isContinuous)
{
    if(!selRows.empty())
        selRows.clear();
    QItemSelectionModel* smodel = selectionModel();
    if(smodel->hasSelection()){
        for(int i = 0; i < validRows-1; ++i){
            if(smodel->isRowSelected(i,QModelIndex()))
                selRows<<i;
        }
    }
    isContinuous = true;
    for(int i = 1; i < selRows.count(); ++i){
        if(selRows.at(i) - selRows.at(i-1) != 1)
            isContinuous = false;
    }
}

/**
 * @brief BaTableWidget::updateSubTableGeometry 更新合计栏的几何尺寸
 */
void BaTableWidget::updateSubTableGeometry()
{
//    QWidget* wt = viewport();
//    QSize size = wt->size();
//    size.setHeight(size.height()-30);
//    wt->resize(size);
    int w = verticalHeader()->width() + columnWidth(0)
        + columnWidth(1) + columnWidth(2)-frameWidth();
    sumTable->setColumnWidth(0,w);
    sumTable->setColumnWidth(1,columnWidth(3));
    sumTable->setColumnWidth(2,columnWidth(4));
    sumTable->setColumnWidth(3,columnWidth(5));

    int h = rowHeight(0);
    int x = frameWidth();
    int y = height()-h;
    w = 0;
    for(int i = 0; i < sumTable->columnCount(); ++i)
        w += sumTable->columnWidth(i);
    sumTable->setGeometry(x,y,w,h);
    sumTable->setRowHeight(0,h-2);
}

/**
 * @brief BaTableWidget::setRowChangedTags 设置行修改标记
 * 两个列表的元素个数必须相等，且行索引必须在当前表格的有效行数内
 * @param rowIndex
 * @param states
 */
void BaTableWidget::setRowChangedTags(QList<int> rowIndex, QList<CommonItemEditState> states)
{

}

/**
 * @brief BaTableWidget::setRowChangedTag 设置指定行的修改标记
 * @param row
 * @param state
 */
void BaTableWidget::setRowChangedTag(int row, CommonItemEditState state)
{
    if(row >= validRows)
        return;
    QString tag = getModifyTag(state);
    QTableWidgetItem* item = verticalHeaderItem(row);
    if(!item)
        item = new QTableWidgetItem;
    if(tag.isEmpty())
        item->setText(QString::number(row+1));
    else
        item->setText(QString("%1%2").arg(row+1).arg(tag));
    setVerticalHeaderItem(row,item);

}

void BaTableWidget::resizeEvent(QResizeEvent *event)
{
    QTableWidget::resizeEvent(event);
    updateSubTableGeometry();
}

void BaTableWidget::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();
    int row = currentRow();    //在编辑器打开的时候，获取的行列号无效，不知何故？
    int col = currentColumn(); //但又不能避免在编辑器打开的时候处理此消息，这是一个潜在的bug

    //按下“=”自动请求创建合计对等会计分录
    //按下“Ctrl+=”键，则自动拷贝上一条会计分录并插入到当前行
    if(key == Qt::Key_Equal){
        if(e->modifiers() == Qt::ControlModifier){
            emit reqAutoCopyToCurBa();
        }
        else
            emit requestCrtNewOppoBa();

    }
    //这个只能在还没有打开编辑器的时候，在贷方列按回车键时满足条件
    else if(((key == Qt::Key_Return) || (key == Qt::Key_Enter))
            && col == BaTableWidget::DVALUE){
        emit requestAppendNewBa(row);
    }
    QTableWidget::keyPressEvent(e);
}

void BaTableWidget::mousePressEvent(QMouseEvent *event)
{
    if(Qt::RightButton == event->button()){
       int row = rowAt(event->y());
       int col = columnAt(event->x());
       //调整上下文菜单的可用性或添加、移除特定的菜单项
       //emit requestContextMenu(row, col);
   }
    QTableWidget::mousePressEvent(event);
}

QString BaTableWidget::getModifyTag(CommonItemEditState state)
{
    switch(state){
    case CIES_INIT:
        return "";
    case CIES_CHANGED:
        return "#";
    case CIES_NEW:
        return "+";
    }
    return "";
}



/////////////////////////////PzDialog////////////////////////////////
PzDialog::PzDialog(int month, AccountSuiteManager *psm, QByteArray* sinfo, QWidget *parent)
    : QDialog(parent),ui(new Ui::pzDialog),curRow(-1),isInteracting(false),pzMgr(psm),
    invoiceStatForm(0),baTemplate(0)
{
    ui->setupUi(this);
    curPz = NULL;
    appCfg = AppConfig::getInstance();
    initResources();
    //setCommonState(sinfo);
    msgTimer = new QTimer(this);
    msgTimer->setInterval(INFO_TIMEOUT);
    msgTimer->setSingleShot(true);
    QDoubleValidator* validator = new QDoubleValidator(this);
    validator->setDecimals(2);
    ui->edtRate->setValidator(validator);
    connect(msgTimer, SIGNAL(timeout()),this,SLOT(msgTimeout()));
    if(!psm){
        LOG_ERROR("Ping Zheng Set object is NULL!");
        return;
    }
    connect(pzMgr,SIGNAL(pzCountChanged(int)),this,SLOT(updatePzCount(int)));
    //初始化快捷键
    sc_copyprev = new QShortcut(QKeySequence(PZEDIT_COPYPREVROW),this);
    sc_save = new QShortcut(QKeySequence("Ctrl+s"),this);
    sc_copy = new QShortcut(QKeySequence("Ctrl+c"),this);
    sc_cut = new QShortcut(QKeySequence("Ctrl+x"),this);
    sc_paster = new QShortcut(QKeySequence("Ctrl+v"),this);
    sc_look = new QShortcut(QKeySequence("F12"),this);
    connect(sc_save,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_copyprev,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_copy,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_cut,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_paster,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_look,SIGNAL(activated()),this,SLOT(processShortcut()));

    account = pzMgr->getAccount();
    subMgr = pzMgr->getSubjectManager();

    smartSSubSet = appCfg->isOnSmartSSubSet();
    QString subCode = subMgr->getYsSub()->getCode();
    prefixes[subCode] = appCfg->getSmartSSubFix(subCode,AppConfig::SSF_PREFIXE);
    suffixes[subCode] = appCfg->getSmartSSubFix(subCode,AppConfig::SSF_SUFFIXE);
    if(prefixes.value(subCode).isEmpty() || suffixes.value(subCode).isEmpty())
        smartSSubSet=false;
    subCode = subMgr->getYfSub()->getCode();
    prefixes[subCode] = appCfg->getSmartSSubFix(subCode,AppConfig::SSF_PREFIXE);
    suffixes[subCode] = appCfg->getSmartSSubFix(subCode,AppConfig::SSF_SUFFIXE);
    if(prefixes.value(subCode).isEmpty() || suffixes.value(subCode).isEmpty())
        smartSSubSet=false;

    delegate = new ActionEditItemDelegate(subMgr,this);
    connect(delegate,SIGNAL(reqCopyPrevAction(int)),this,SLOT(copyPrewAction(int)));
    //connect(delegate,SIGNAL(updateSndSubject(int,int,SecondSubject*)),
    //        this,SLOT(updateSndSubject(int,int,SecondSubject*)));
    connect(delegate,SIGNAL(crtNewNameItemMapping(int,int,FirstSubject*,SubjectNameItem*,SecondSubject*&)),
            this,SLOT(creatNewNameItemMapping(int,int,FirstSubject*,SubjectNameItem*,SecondSubject*&)));
    connect(delegate,SIGNAL(crtNewSndSubject(int,int,FirstSubject*,SecondSubject*&,QString)),
            this,SLOT(creatNewSndSubject(int,int,FirstSubject*,SecondSubject*&,QString)));
    //connect(ui->tview,SIGNAL(currentCellChanged(int,int,int,int)),
    //        this,SLOT(currentCellChanged(int,int,int,int)));
    //connect(delegate,SIGNAL(moveNextRow(int)),this,SLOT(moveToNextBa(int)));

    //adjustTableSize();
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(tabColWidthResized(int,int,int)));
    connect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                   this,SLOT(tabRowHeightResized(int,int,int)));
    ui->tview->setItemDelegate(delegate);
    connect(ui->tview,SIGNAL(itemSelectionChanged()),this,SLOT(selectedRowChanged()));
    connect(pzMgr,SIGNAL(currentPzChanged(PingZheng*,PingZheng*)),this,SLOT(curPzChanged(PingZheng*,PingZheng*)));
    setMonth(month);
    connect(pzMgr->getStatUtil(),SIGNAL(extraException(BusiAction*,Double,MoneyDirection,Double,MoneyDirection)),
            this,SLOT(extraException(BusiAction*,Double,MoneyDirection,Double,MoneyDirection)));
    //connect(delegate,SIGNAL(extraException(BusiAction*,Double,MoneyDirection,Double,MoneyDirection)),
    //        this,SLOT(extraException(BusiAction*,Double,MoneyDirection,Double,MoneyDirection)));

    ui->edtRate->setContextMenuPolicy(Qt::ActionsContextMenu);
    if(!account->isReadOnly() && !pzMgr->isSuiteClosed() && pzMgr->getState() != Ps_Jzed){
        actModifyRate = new QAction(tr("修改汇率"),this);
        ui->edtRate->addAction(actModifyRate);
        connect(actModifyRate,SIGNAL(triggered()),this,SLOT(modifyRate()));
    }
    setCommonState(sinfo);
    lookAssistant=0;
}

PzDialog::~PzDialog()
{
    if(lookAssistant){
        disconnect(this,SIGNAL(findMatchBas(FirstSubject*,SecondSubject*,QHash<int,QList<int> >,QList<QStringList>)),
                   lookAssistant,SLOT(findItem(FirstSubject*,SecondSubject*,QHash<int,QList<int> >,QList<QStringList>)));
        delete lookAssistant;
    }
    if(invoiceStatForm)
        delete invoiceStatForm;
    if(baTemplate)
        delete baTemplate;
    delete ui;
}

void PzDialog::setCommonState(QByteArray *info)
{
    if(!info || info->isEmpty()){
        states.isValid = true;
        QList<int> tinfos;
        AppConfig::getInstance()->readPzEwTableState(tinfos);
        states.rowHeight = tinfos.first();
        states.colSummaryWidth = tinfos.at(1);
        states.colFstSubWidth = tinfos.at(2);
        states.colSndSubWidth = tinfos.at(3);
        states.colMtWidth = tinfos.at(4);
        states.colValueWidth = tinfos.at(5);
    }
    else{
        QBuffer bf(info);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        in>>states.isValid;
        in>>states.rowHeight;
        in>>states.colSummaryWidth;
        in>>states.colFstSubWidth;
        in>>states.colSndSubWidth;
        in>>states.colMtWidth;
        in>>states.colValueWidth;
        bf.close();
    }
    adjustTableSize();
}

QByteArray *PzDialog::getCommonState()
{
    //
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);

    out<<states.isValid;
    out<<states.rowHeight;
    out<<states.colSummaryWidth;
    out<<states.colFstSubWidth;
    out<<states.colSndSubWidth;
    out<<states.colMtWidth;
    out<<states.colValueWidth;
    bf.close();
    return info;
}

/**
 * @brief 设置要打开编辑的凭证集的月份数
 * @param month
 */
void PzDialog::setMonth(int month)
{
    if(!pzMgr->open(month)){
        QMessageBox::critical(this,tr("出错信息"),tr("打开%1年%2月凭证集时发生错误！"));
        return;
    }
    ui->edtPzCount->setText(QString::number(pzMgr->getPzCount()));
    //显示本期汇率
    pzMgr->getRates(rates,month);
    if(account->isUsedWb() && rates.isEmpty())
        QMessageBox::warning(this,msgTitle_warning,tr("本期汇率未设值"));
    QVariant v;
    foreach(Money* mt, account->getWaiMt()){
        v.setValue<Money*>(mt);
        ui->cmbMt->addItem(mt->name(),v);
    }
    ui->cmbMt->setCurrentIndex(0);
    connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),
            this,SLOT(moneyTypeChanged(int)));
    if(ui->cmbMt->count() > 0)
        moneyTypeChanged(0);
    curPz = pzMgr->first();    
    if(curPz){
        connect(curPz,SIGNAL(updateBalanceState(bool)),this,SLOT(pzBalanceStateChanged(bool)));
        pzBalanceStateChanged(curPz->jsum()==curPz->dsum());
    }
    refreshPzContent();

    //delegate->watchExtraException();
}

/**
 * @brief PzDialog::setReadonly
 * 根据当前凭证的审核状态和凭证集的状态，设置凭证内容显示部件的只读属性
 * @param r
 */
void PzDialog::adjustViewReadonly()
{
    //综合凭证集状态和凭证状态决定凭证是否可编辑
    bool b = account->isReadOnly() || pzMgr->isSuiteClosed() ||
            pzMgr->getState() == Ps_Jzed ||
            (curPz->getPzState() != Pzs_Recording && (curPz->getPzState() != Pzs_Repeal));
    ui->dateEdit->setReadOnly(b);
    ui->spnZbNum->setReadOnly(b);
    ui->spnEncNum->setReadOnly(b);
    delegate->setReadOnly(b);
    //告诉代理当前凭证的有效行数
    //if(!b)
    //    delegate->setVolidRows(curPz->baCount());
}

bool PzDialog::isDirty()
{
    return pzMgr->isDirty();
}

/**
 * @brief PzDialog::restoreState 恢复窗口状态
 */
void PzDialog::restoreStateInfo()
{
    ui->tview->setColumnWidth(BaTableWidget::SUMMARY,states.colSummaryWidth);
    ui->tview->setColumnWidth(BaTableWidget::FSTSUB,states.colFstSubWidth);
    ui->tview->setColumnWidth(BaTableWidget::SNDSUB,states.colSndSubWidth);
    ui->tview->setColumnWidth(BaTableWidget::MONEYTYPE,states.colMtWidth);
    ui->tview->setColumnWidth(BaTableWidget::JVALUE,states.colValueWidth);
    ui->tview->setColumnWidth(BaTableWidget::DVALUE,states.colValueWidth);
    for(int i = 0; i < ui->tview->rowCount(); ++i)
        ui->tview->setRowHeight(i,states.rowHeight);
}

/**
 * @brief PzDialog::updateContent
 * 更新凭证编辑窗口，以显示最新的修改内容，这可能是通过undo框架引起的
 * 为了缩小更新的界面范围，要增加两个参数（凭证的哪个部分进行更新，索引【在更新某个会计分录是使用】，会计分录的哪个部分）
 */
void PzDialog::updateContent()
{
    //curPz = pzMgr->getCurPz();
    refreshPzContent();
}

void PzDialog::moveToFirst()
{
    curPz = pzMgr->first();
    refreshPzContent();
}

void PzDialog::moveToPrev()
{
    curPz = pzMgr->previou();
    refreshPzContent();
}

void PzDialog::moveToNext()
{
    curPz = pzMgr->next();
    refreshPzContent();
    emit showMessage("move to next ping zheng!");
}

void PzDialog::moveToLast()
{
    curPz = pzMgr->last();
    refreshPzContent();
    emit showMessage("move to last ping zheng!",AE_WARNING);
}

/**
 * @brief PzDialog::seek 快速定位指定号的凭证
 * @param num
 */
void PzDialog::seek(int num)
{
    curPz = pzMgr->seek(num);
    refreshPzContent();
}

/**
 * @brief PzDialog::seek
 *  快速定位到指定凭证
 * @param pz
 */
void PzDialog::seek(PingZheng *pz, BusiAction* ba)
{
    if(!pzMgr->seek(pz)){
        QMessageBox::warning(this,tr("警告信息"),tr("无法定位到该凭证！"));
        return;
    }
    curPz = pzMgr->getCurPz();
    refreshPzContent();
    if(!ba)
        return;
    for(int i = 0; i < pz->baCount(); ++i){
        if(*ba == *pz->getBusiAction(i)){
            ui->tview->selectRow(i);
            return;
        }
    }
}

/**
 * @brief 选中指定序号的分录（基于1）
 * @param row
 */
void PzDialog::selectBa(int row)
{
    if(!curPz || row<1 || row > curPz->baCount())
        return;
    ui->tview->selectRow(row-1);
}

void PzDialog::openBusiactionTemplate(BATemplateEnum type)
{
    if(!baTemplate){
        baTemplate = new BaTemplateForm(pzMgr,this);

    }
    baTemplate->setTemplateType(type);
    baTemplate->show();
}

void PzDialog::addPz()
{
    //进行凭证集状态检测，以决定是否可以添加凭证
    PingZheng* oldCurPz = curPz;
    curPz = new PingZheng(pzMgr);
    QString ds;
    if(!oldCurPz)
        ds = QDate(pzMgr->year(),pzMgr->month(),1).toString(Qt::ISODate);
    else
        ds = pzMgr->getPz(pzMgr->getPzCount())->getDate();
    curPz->setDate(ds);
    curPz->setNumber(pzMgr->getPzCount()+1);
    curPz->setZbNumber(pzMgr->getMaxZbNum());
    curPz->setPzClass(Pzc_Hand);
    curPz->setRecordUser(curUser);
    curPz->setPzState(Pzs_Recording);
    AppendPzCmd* cmd = new AppendPzCmd(pzMgr,curPz);
    pzMgr->getUndoStack()->push(cmd);
    refreshPzContent();
}

void PzDialog::insertPz()
{
    //进行凭证集状态检测，以决定是否可以添加凭证
    PingZheng* oldCurPz = curPz;
    curPz = new PingZheng(pzMgr);
    QString ds;
    if(!oldCurPz)
        ds = QDate(pzMgr->year(),pzMgr->month(),1).toString(Qt::ISODate);
    else
        ds = oldCurPz->getDate();
    curPz->setDate(ds);
    curPz->setNumber(oldCurPz->number());
    curPz->setZbNumber(pzMgr->getMaxZbNum());
    curPz->setPzClass(Pzc_Hand);
    curPz->setRecordUser(curUser);
    curPz->setPzState(Pzs_Recording);
    InsertPzCmd* cmd = new InsertPzCmd(pzMgr,curPz);
    pzMgr->getUndoStack()->push(cmd);
    refreshPzContent();
}

void PzDialog::removePz()
{
    DelPzCmd* cmd = new DelPzCmd(pzMgr,curPz);
    pzMgr->getUndoStack()->push(cmd);
    curPz = pzMgr->getCurPz();
    //refreshPzContent();
}

/**
 * @brief 创建发票聚合凭证
 * @return
 */
bool PzDialog::crtGatherPz()
{
    QList<PingZheng*> pzLst;
    QUndoCommand* mainCmd = new QUndoCommand(tr("发票聚合凭证"));
    if(!pzMgr->crtGatherPz(pzMgr->year(),pzMgr->month(),pzLst) || !pzMgr->crtGatherPz(pzMgr->year(),pzMgr->month(),pzLst,false)){
        delete mainCmd;
        return false;
    }
    if(pzLst.isEmpty()){
        myHelper::ShowMessageBoxInfo(tr("本月导入的发票都已经销账！"));
        return true;
    }
    foreach(PingZheng* pz, pzLst)
        AppendPzCmd* cmd = new AppendPzCmd(pzMgr,pz,mainCmd);
    pzMgr->getUndoStack()->push(mainCmd);
    //刷新状态
    refreshPzContent();
    return true;
}

/**
 * @brief PzDialog::crtJzhdPz
 *  创建结转汇兑损益的凭证
 * @return
 */
bool PzDialog::crtJzhdPz()
{
    if(pzMgr->getState() != Ps_AllVerified){
        myHelper::ShowMessageBoxWarning(tr("凭证集内存在未审核凭证，不能结转！"));
        return true;
    }
    if(!pzMgr->getExtraState()){
        myHelper::ShowMessageBoxWarning(tr("余额无效，请重新进行统计并保存正确余额！"));
        return true;
    }
    //因为汇兑损益的结转要涉及到期末汇率，即下期的汇率，要求用户确认汇率是否正确
    QHash<int,Double> startRates,endRates;
    account->getRates(pzMgr->year(),pzMgr->month(),startRates);
    int yy,mm;
    if(pzMgr->month() == 12){
        yy = pzMgr->year()+1;
        mm = 1;
    }
    else{
        yy = pzMgr->year();
        mm = pzMgr->month()+1;
    }
    account->getRates(yy,mm,endRates);
    QString tip = tr("请确认汇率是否正确：%1年%2月美金汇率：").arg(yy).arg(mm);
    bool ok;
    double rate = QInputDialog::getDouble(0,tr("信息输入"),tip,endRates.value(USD).getv(),0,100,4,&ok);
    if(!ok)
        return true;
    endRates[USD] = Double(rate,4);
    account->setRates(yy,mm,endRates);   
    //创建移除原先存在的结转汇兑损益的凭证的命令
    QList<PingZheng*> pzLst = pzMgr->getAllJzhdPzs();
    if(!pzLst.isEmpty()){
        if(QMessageBox::warning(this,tr("警告信息"),tr("凭证集内已存在结转汇兑损益凭证，要覆盖吗？"),
                                QMessageBox::Yes|QMessageBox::No,QMessageBox::No) == QMessageBox::No)
            return true;
    }
    //创建一个总命令以包容所有涉及到创建结转汇兑损益的各类动作
    QUndoCommand* mainCmd = new QUndoCommand(tr("结转汇兑损益"));
    if(!pzLst.isEmpty()){
        foreach(PingZheng* pz, pzLst){
            DelPzCmd* cmd = new DelPzCmd(pzMgr,pz,mainCmd);
        }
    }
    //调用凭证集管理对象创建新的结转汇兑损益凭证对象
    pzLst.clear();
    if(!pzMgr->crtJzhdsyPz(pzMgr->year(),pzMgr->month(),pzLst,startRates,endRates,curUser)){
        delete mainCmd;
        return false;
    }
    //创建一个添加多个凭证对象到当前凭证集的命令对象
    foreach(PingZheng* pz, pzLst){
        AppendPzCmd* cmd = new AppendPzCmd(pzMgr,pz,mainCmd);
    }
    pzMgr->getUndoStack()->push(mainCmd);
    //刷新状态
    refreshPzContent();
    return true;
}

/**
 * @brief PzDialog::crtJzsyPz
 *  创建结转损益类科目余额到本年利润的凭证
 * @return
 */
bool PzDialog::crtJzsyPz()
{
    if(pzMgr->getState() != Ps_AllVerified){
        QMessageBox::warning(0,tr("警告信息"),tr("凭证集内存在未审核凭证，不能结转！"));
        return true;
    }
    if(!pzMgr->getExtraState()){
        QMessageBox::warning(0,tr("警告信息"),tr("余额无效，请重新进行统计并保存正确余额！"));
        return true;
    }
    QList<PingZheng*> oldPzs;
    pzMgr->getJzhdsyPz(oldPzs);
    if(oldPzs.count() != pzMgr->getJzhdsyMustPzNums()){
        QString info = tr("未结转汇兑损益或结转汇兑损益凭证数有误！");
        if(appCfg->remainForeignCurrencyUnity())
            info.append(tr("\n或许外币的原币余额与本币形式值不符！"));
        QMessageBox::warning(0,tr("警告信息"),info);
        return true;
    }

    oldPzs.clear();
    pzMgr->getJzsyPz(oldPzs);
    if(!oldPzs.isEmpty()){
        if(QMessageBox::warning(this,tr("警告信息"),tr("凭证集内已存在结转损益凭证，要重新创建吗？"),
                                QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
            return true;
    }
    QList<PingZheng*> createdPzs;
    if(!pzMgr->crtJzsyPz(pzMgr->year(),pzMgr->month(),createdPzs))
        return false;
    QUndoCommand* mainCmd = new QUndoCommand(tr("结转损益"));
    PingZheng* pz;
    for(int i = 0; i < oldPzs.count(); ++i){
        pz = oldPzs.at(i);
        DelPzCmd* cmd = new DelPzCmd(pzMgr,pz,mainCmd);
    }
    for(int i = 0; i < createdPzs.count(); ++i){
        pz = createdPzs.at(i);
        AppendPzCmd* cmd = new AppendPzCmd(pzMgr,pz,mainCmd);
    }
    pzMgr->getUndoStack()->push(mainCmd);
    refreshPzContent();
    return true;
}

/**
 * @brief 结转损益类科目余额到本年利润
 * @return
 */
bool PzDialog::crtJzbnlr()
{
    if(pzMgr->getState() != Ps_AllVerified){
        myHelper::ShowMessageBoxWarning(tr("凭证集内存在未审核凭证，不能结转！"));
        return true;
    }
    if(!pzMgr->getExtraState()){
        myHelper::ShowMessageBoxWarning(tr("余额无效，请重新进行统计并保存正确余额！"));
        return true;
    }

    //QList<PingZheng*> oldPzs;
    //考虑到实际这个功能好像不太需要
    //余下未实现的：
    //1、获取原有的结转利润凭证（可能有，可能没有），如果有，提示用户是否继续创建新的覆盖原有的
    //2、调用pzMgr创建结转利润凭证

    myHelper::ShowMessageBoxWarning(tr("功能未实现，如果确实需要，请联系开发人员！"));
    return true;
}

/**
 * @brief PzDialog::crtJtpz
 * @return
 * 创建记提凭证
 */
bool PzDialog::crtJtpz(QList<JtpzDatas*> datas)
{
    QUndoCommand* mainCmd = new QUndoCommand(tr("记提凭证"));
    QList<PingZheng*> pzLst;
    if(!pzMgr->crtJtpz(datas,pzLst)){
        delete mainCmd;
        return false;
    }
    //创建一个添加多个凭证对象到当前凭证集的命令对象
    foreach(PingZheng* pz, pzLst){
        AppendPzCmd* cmd = new AppendPzCmd(pzMgr,pz,mainCmd);
    }
    pzMgr->getUndoStack()->push(mainCmd);
    //刷新状态
    refreshPzContent();
    return true;
}

/**
 * @brief PzDialog::moveUpBa
 * 向上移动当前会计分录
 */
void PzDialog::moveUpBa()
{
    if(!curBa)
        return;
    int row = ui->tview->currentRow();
    int col = ui->tview->currentColumn();
    if(row <= 0)
        return;
    ModifyBaMoveCmd* cmd = new ModifyBaMoveCmd(pzMgr,curPz,curBa,1,pzMgr->getUndoStack());
    pzMgr->getUndoStack()->push(cmd);
    BaUpdateColumns updateCols;
    updateCols |= BUC_ALL;
    updateBas(row-1,2,updateCols);
    ui->tview->setCurrentCell(row-1,col);
}

/**
 * @brief PzDialog::moveDownBa
 * 向下移动当前会计分录
 */
void PzDialog::moveDownBa()
{
    if(!curBa)
        return;
    int row = ui->tview->currentRow();
    int col = ui->tview->currentColumn();
    if(row >= curPz->baCount()-1)
        return;
    ModifyBaMoveCmd* cmd = new ModifyBaMoveCmd(pzMgr,curPz,curBa,-1,pzMgr->getUndoStack());
    pzMgr->getUndoStack()->push(cmd);
    BaUpdateColumns updateCols;
    updateCols |= BUC_ALL;
    updateBas(row,2,updateCols);
    ui->tview->setCurrentCell(row+1,col);
}

/**
 * @brief PzDialog::addBa
 * 添加会计分录
 */
void PzDialog::addBa()
{
    if(!curPz)
        return;
    curBa = new BusiAction;
    curBa->setParent(curPz);
    curBa->setMt(account->getMasterMt(),0.0);
    AppendBaCmd* cmd = new AppendBaCmd(pzMgr,curPz,curBa);
    pzMgr->getUndoStack()->push(cmd);
    int rows = curPz->baCount();

    disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    //ui->tview->setRowCount(rows+1);
    ui->tview->setValidRows(rows+1);
    delegate->setVolidRows(rows);
    initBlankBa(rows);
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));

    BaUpdateColumns updateCols;
    updateCols |= BUC_ALL;
    updateBas(rows-1,1,updateCols);
    ui->tview->setCurrentCell(rows-1,BT_SUMMARY);
    ui->tview->scrollTo(ui->tview->model()->index(rows-1,BT_SUMMARY));
}

/**
 * @brief PzDialog::insertBa
 * 插入会计分录
 */
void PzDialog::insertBa(BusiAction* ba)
{
    if(curRow == -1)
        return;
    if(!ba){
        curBa = new BusiAction;
        curBa->setParent(curPz);
        curBa->setMt(account->getMasterMt(),0.0);
    }
    else
        curBa = ba;
    InsertBaCmd* cmd = new InsertBaCmd(pzMgr,curPz,curBa,curRow);
    pzMgr->getUndoStack()->push(cmd);
    int rows = curPz->baCount();
    ui->tview->insertRow(curRow);
    disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    //ui->tview->setRowCount(rows+1);
    ui->tview->setValidRows(rows+1);
    delegate->setVolidRows(rows);
//    if(!ba)
//        initBlankBa(rows);
//    else
        refreshSingleBa(curRow,curBa);
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));

    BaUpdateColumns updateCols;
    updateCols |= BUC_ALL;
    updateBas(curRow,rows-curRow,updateCols);
    ui->tview->setCurrentCell(curRow,BT_SUMMARY);
    ui->tview->edit(ui->tview->model()->index(curRow,BT_SUMMARY));
}

/**
 * @brief 在当前行或行尾（如果没有选择当前分录）插入多条分录
 * @param bas
 */
void PzDialog::insertBas(QList<BusiAction *> bas)
{
    if(bas.isEmpty())
        return;
    int row;
    if(curRow == -1 || curRow >= curPz->baCount())
        row = curPz->baCount();
    else{
        //按当前选择行的位置作为插入位，在选择多行的情况下，如果选择的行连续，则插入到首行，如果不连续，则插入的表尾
        QList<int> rows;bool isCon = true;
        ui->tview->selectedRows(rows,isCon);
        if(rows.isEmpty() || !isCon)
            row = curPz->baCount();
        else
            row = rows.first();
    }
    QUndoCommand* cmdMain = new QUndoCommand(tr("创建多条分录"));
    for(int i = 0; i < bas.count(); ++i){
        BusiAction* ba = bas.at(i);
        InsertBaCmd* cmd = new InsertBaCmd(pzMgr,curPz,ba,row+i,cmdMain);
    }
    pzMgr->getUndoStack()->push(cmdMain);
    int totalRows = curPz->baCount();

    disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    for(int i = 0; i < bas.count(); ++i){
        ui->tview->insertRow(row+i);
        initBlankBa(row+i);
    }
    ui->tview->setValidRows(totalRows+1);
    delegate->setVolidRows(totalRows);
    updateBas(row,bas.count(),BUC_ALL);
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    foreach (QTableWidgetSelectionRange r, ui->tview->selectedRanges())
        ui->tview->setRangeSelected(r,false);
    QTableWidgetSelectionRange range(row,0,row+bas.count()-1,BaTableWidget::DVALUE);
    ui->tview->setRangeSelected(range,true);
}

/**
 * @brief PzDialog::removeBa
 * 移除会计分录
 */
void PzDialog::removeBa()
{
    if(!ui->tview->isHasSelectedRows())
        return;
    QList<int> selRows;
    bool c;
    ui->tview->selectedRows(selRows,c);
    QString rowStr;
    foreach(int r, selRows)
        rowStr.append(QString::number(r+1)).append(",");
    rowStr.chop(1);
    QUndoCommand* mmd = new QUndoCommand(tr("移除会计分录（P%1B%2）")
                                         .arg(curPz->number()).arg(rowStr));
    foreach(int r, selRows){
        BusiAction* b = curPz->getBusiAction(r);
        ModifyBaDelCmd* cmd = new ModifyBaDelCmd(pzMgr,curPz,curPz->getBusiAction(r),mmd);
    }
    pzMgr->getUndoStack()->push(mmd);
    int rows = curPz->baCount();

    disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    for(int i = selRows.count()-1; i > -1; i--)
        ui->tview->removeRow(selRows.at(i));;
    ui->tview->setValidRows(rows+1);
    delegate->setVolidRows(rows);
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));

    ui->tview->setJSum(curPz->jsum());
    ui->tview->setDSum(curPz->dsum());
    if(rows == 0 || rows == 1)
        curRow = 0;
    else if(rows-1 < selRows.last())
        curRow = rows-1;
    else
        curRow = selRows.last();
    ui->tview->setCurrentCell(curRow,BT_SUMMARY);
}

/**
 * @brief PzDialog::getBaSelectedCase
 *  获取当前凭证中分录的选择情况（如果选择的分录是不连续的，则两个参数都返回true，包括全选的情况）
 *  这个用来控制分录移动按钮的可用性
 * @param first 如果选择的分录包含了第一个分录，则为true
 * @param last  如果选择的分录包含了最后一个分录，则为true
 */
void PzDialog::getBaSelectedCase(QList<int> rows, bool& conti)
{
    ui->tview->selectedRows(rows,conti);
}




/**
 * @brief PzDialog::msgTimeout 状态信息显示超时处理
 */
void PzDialog::msgTimeout()
{
    //ui->lblStateInfo->setText("");
}

/**
 * @brief PzDialog::moneyTypeChanged
 *  根据选择的外币币种，显示对应该外币的汇率
 * @param index
 */
void PzDialog::moneyTypeChanged(int index)
{
    ui->edtRate->setText(rates.value(ui->cmbMt->itemData(index).value<Money*>()->code()).toString());
}



/**
 * @brief PzDialog::processShortcut
 * 处理快捷键的主入口
 */
void PzDialog::processShortcut()
{
    if(sender() == sc_save)
        save();    
    else if(sender() == sc_look){
        if(!lookAssistant){
            lookAssistant = new LookYsYfItemForm(account,this);
            lookAssistant->show();
            lookAssistant->move(mapToGlobal(pos()));
            connect(this,SIGNAL(findMatchBas(FirstSubject*,SecondSubject*,QHash<int,QList<int> >,QList<QStringList>)),
                    lookAssistant,SLOT(findItem(FirstSubject*,SecondSubject*,QHash<int,QList<int> >,QList<QStringList>)));
        }
        else
            lookAssistant->setHidden(!lookAssistant->isHidden());
    }
    //编辑会计分录相关的快捷键
    else if(ui->tview->hasFocus()){
        if(sender() == sc_copyprev){
            if(curRow > 0)
                copyPrewAction(curRow);
        }
        else if(sender() == sc_copy){
            QList<int> rows; bool c;
            ui->tview->selectedRows(rows,c);
            if(rows.empty())
                return;
            copyOrCut = CO_COPY;
            clb_Bas.clear();
            foreach(int i, rows)
                clb_Bas<<curPz->getBusiAction(i);
        }
        else if(sender() == sc_cut){
            QList<int> rows; bool c;
            ui->tview->selectedRows(rows,c);
            if(rows.empty())
                return;
            copyOrCut = CO_CUT;
            clb_Bas.clear();
            CutBaCmd* cmd = new CutBaCmd(pzMgr,curPz,rows,&clb_Bas);
            pzMgr->getUndoStack()->push(cmd);
            refreshActions();
        }
        else if(sender() == sc_paster){
            if(clb_Bas.empty())
                return;
            if(curRow == -1)
                return;
            int row = curRow;
            int rows = clb_Bas.count();
            PasterBaCmd* cmd = new PasterBaCmd(curPz,curRow,&clb_Bas,copyOrCut==CO_COPY);
            pzMgr->getUndoStack()->push(cmd);
            refreshActions();
            //选中粘贴行
            curRow = row;
            for(int i = 0; i < rows; ++i)
                ui->tview->selectRow(curRow+i);
        }
    }
}

/**
 * @brief PzDialog::save
 * 保存凭证集内的所有凭证
 */
void PzDialog::save()
{
    if(!needSaveSSubs.isEmpty()){
        foreach(SecondSubject* ssub, needSaveSSubs)
            subMgr->saveSS(ssub);
        needSaveSSubs.clear();
    }
    pzMgr->save(AccountSuiteManager::SW_ALL);
}

void PzDialog::setPzState(PzState state)
{
    ModifyPzVStateCmd* cmd = new ModifyPzVStateCmd(pzMgr,curPz,state);
    pzMgr->getUndoStack()->push(cmd);
    adjustViewReadonly();
}


/**
 * @brief PzDialog::updatePzCount
 * @param count
 */
void PzDialog::updatePzCount(int count)
{
    ui->edtPzCount->setText(QString::number(count));
}

/**
 * @brief PzDialog::curPzChanged
 *  接收由凭证集管理对象发出的当前凭证改变信号，并显示该凭证内容
 * @param newPz
 * @param oldPz
 */
void PzDialog::curPzChanged(PingZheng *newPz, PingZheng *oldPz)
{
//    if(oldPz){
//        if(isPzMemModified){
//            ModifyPzComment* cmd = new ModifyPzComment(pzMgr,oldPz,ui->edtComment->toPlainText());
//            pzMgr->getUndoStack()->push(cmd);
//            isPzMemModified = false;
//        }
//    }
    curPz = newPz;
    updateContent();
    if(oldPz)
        disconnect(oldPz,SIGNAL(updateBalanceState(bool)),this,SLOT(pzBalanceStateChanged(bool)));
    if(curPz)
        connect(curPz,SIGNAL(updateBalanceState(bool)),this,SLOT(pzBalanceStateChanged(bool)));
}

/**
 * @brief PzDialog::moveToNextBa
 * 将当前会计分录设置为下一条（如果当前是最后一条，则自动创建一个新的会计分录）
 * @param row（基于0）
 */
void PzDialog::moveToNextBa(int row)
{
    if(row == curPz->baCount()-1){
        delegate->setVolidRows(row+2);
        initBlankBa(row+1);
    }
}

/**
 * @brief PzDialog::selectedBaChanged
 *  监视分录选择的改变，进而控制移动分录按钮的可用性
 */
void PzDialog::selectedRowChanged()
{
    QList<int> rows; bool conti;
    ui->tview->selectedRows(rows,conti);
    emit selectedBaChanged(rows,conti);
//    bool first,last;
//    if(rows.isEmpty()||!conti){
//        first=true;last=true;
//    }
//    else{
//        first = (rows.first() == 0);
//        last = (rows.last() == curPz->baCount()-1);
//    }
//    emit baIndexBoundaryChanged(first,last);
}

/**
 * @brief PzDialog::currentCellChanged
 * 跟踪当前会计分录的改变
 * @param currentRow
 * @param currentColumn
 * @param previousRow
 * @param previousColumn
 */
void PzDialog::currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    curRow = currentRow;
    if(curPz){
        curBa = curPz->getBusiAction(currentRow);
        curPz->setCurBa(curBa);
        if(curBa && curBa->getSecondSubject()){
            FirstSubject* fsub = curBa->getFirstSubject();
            SecondSubject* ssub = curBa->getSecondSubject();
            QString tip = ssub->getLName();
            if(fsub == subMgr->getBankSub())
                tip.append(tr("（账号：%1）").arg(subMgr->getBankAccount(ssub)->accNumber));
            ui->tview->setLongName(tip);
        }
        else
            ui->tview->setLongName("");
    }
    else{
        curBa = NULL;
        ui->tview->setLongName("");
    }
}

/**
 * @brief PzDialog::pzDateChanged
 * 凭证日期改变
 * @param date
 */
void PzDialog::pzDateChanged(const QDate &date)
{
    ModifyPzDateCmd*  cmd = new ModifyPzDateCmd(pzMgr,curPz,date.toString(Qt::ISODate));
    pzMgr->getUndoStack()->push(cmd);
}

/**
 * @brief PzDialog::pzZbNumChanged
 * 凭证自编号改变
 * @param i
 */
void PzDialog::pzZbNumChanged(int num)
{
    //curPz->setZbNumber(num);
    ModifyPzZNumCmd* cmd = new ModifyPzZNumCmd(pzMgr,curPz,num);
    pzMgr->getUndoStack()->push(cmd);
}

/**
 * @brief PzDialog::pzEncNumChanged
 * 凭证附件数改变
 * @param i
 */
void PzDialog::pzEncNumChanged(int num)
{
    //curPz->setEncNumber(num);
    ModifyPzEncNumCmd* cmd = new ModifyPzEncNumCmd(pzMgr,curPz,num);
    pzMgr->getUndoStack()->push(cmd);
}

/**
 * @brief PzDialog::BaDataChanged
 * 凭证的会计分录改变
 * @param item
 */
void PzDialog::BaDataChanged(QTableWidgetItem *item)
{
    int row = item->row();
    int col = item->column();

    //如果是备用行，则将备用行升级为有效行，再添加新的备用行
    int rows = ui->tview->getValidRows();
    if(row == rows - 1){
        curBa = new BusiAction;
        AppendBaCmd* cmd = new AppendBaCmd(pzMgr,curPz,curBa);
        pzMgr->getUndoStack()->push(cmd);
        int rows = curPz->baCount();
        disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
        ui->tview->setValidRows(rows+1);
        delegate->setVolidRows(rows);
        initBlankBa(rows);
        connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    }

    if(!curBa)
        return;
    ModifyMultiPropertyOnBa* multiCmd;
    ModifyBaSummaryCmd* scmd;
    FirstSubject* fsub=0;
    SecondSubject* ssub=0;
    Double v=0.0;
    Money* mt;
    MoneyDirection dir;
    BaUpdateColumns updateCols;

    switch(col){
    case BT_SUMMARY:
        scmd = new ModifyBaSummaryCmd(pzMgr,curPz,curBa,item->data(Qt::EditRole).toString());
        pzMgr->getUndoStack()->push(scmd);
        break;
    case BT_FSTSUB:
        fsub = item->data(Qt::EditRole).value<FirstSubject*>();
        //自动依据上一条分录设置应交税金的适配子目
        if(fsub == subMgr->getYjsjSub() && row > 0){
            BusiAction* ba = curPz->getBusiAction(row-1);
            FirstSubject* pfsub = ba?ba->getFirstSubject():0;
            if(pfsub){
                if(pfsub == subMgr->getZysrSub() || pfsub == subMgr->getYsSub())
                    ssub = subMgr->getXxseSSub();
                else if(pfsub == subMgr->getZycbSub() || pfsub == subMgr->getYfSub())
                    ssub = subMgr->getJxseSSub();
                else if(fsub->isHaveSmartAdapte())
                    ssub = fsub->getAdapteSSub(curBa->getSummary());
            }
        }
        //如果是应收应付科目，则提取摘要中的客户关键字信息自动适配子目
        else if(smartSSubSet && !curBa->getSummary().isEmpty() && /*!curBa->getSecondSubject() &&*/
                (fsub == subMgr->getYsSub() || fsub == subMgr->getYfSub())){
            SecondSubject* sub;
            sub = getAdapterSSub(fsub,curBa->getSummary(),prefixes.value(fsub->getCode()),
                                 suffixes.value(fsub->getCode()));
            if(sub){
                ssub = sub;
                //如果是收回应收或付出应付的分录，则自动查找对应发票号的应收应付分录
                QString prefixChar = curBa->getSummary().left(1);
                if(prefixChar == tr("收") || prefixChar == tr("付")){
                    QList<int> months;
                    QList<QStringList> invoiceNums;
                    PaUtils::extractInvoiceNum(curBa->getSummary(),months,invoiceNums);
                    if(!months.isEmpty()){
                        QHash<int,QList<int> > range;
                        int curY = pzMgr->year(); int curM = pzMgr->month();
                        for(int i=0; i < months.count(); ++i){
                            int m = months.at(i);
                            int y = curY;
                            if(m > curM)
                                y--;
                            if(!range.contains(y))
                                range[y] = QList<int>();
                            range[y]<<m;
                        }
                        emit findMatchBas(fsub,ssub,range,invoiceNums);
                    }
                }
            }
        }
        //如果一级科目设置了包含型的子目自动适配关键字，则适配子目
        else if(fsub && fsub->isHaveSmartAdapte())
            ssub = fsub->getAdapteSSub(curBa->getSummary());
        if(!ssub && fsub)
            ssub = fsub->getDefaultSubject();
        updateCols |= BUC_FSTSUB;
        updateCols |= BUC_SNDSUB;
        //如果是银行科目（其二级科目决定了匹配的币种），则根据银行账户的货币属性设置当前的币种
        if(fsub == subMgr->getBankSub()){
            mt = subMgr->getSubMatchMt(ssub);
            if(mt == curBa->getMt())
                v = curBa->getValue();
            else if(curBa->getMt()){
                if(mt == account->getMasterMt() && rates.contains(curBa->getMt()->code()))
                    v = curBa->getValue() * rates.value(curBa->getMt()->code());
                else if(rates.contains(mt->code()))
                    v = curBa->getValue() / rates.value(mt->code());
                updateCols |= BUC_MTYPE;
                updateCols |= BUC_VALUE;
            }
            else
                updateCols |= BUC_MTYPE;
        }
        else if(!curBa->getMt()){
            curBa->setMt(account->getMasterMt(),curBa->getValue());
            mt = curBa->getMt();
            v = curBa->getValue();
            updateCols |= BUC_MTYPE;
            updateCols |= BUC_VALUE;
        }
        //如果新的一级科目使用外币，或者不使用外币且当前货币是本币，则无须调整币种和金额
        else if(fsub && (fsub->isUseForeignMoney() || (!fsub->isUseForeignMoney() && curBa->getMt() == account->getMasterMt()))){
            mt = curBa->getMt();
            v = curBa->getValue();
        }
        else{
            mt = account->getMasterMt();
            if(curBa->getMt())
                v = rates.value(curBa->getMt()->code()) * curBa->getValue();
            updateCols |= BUC_MTYPE;
            updateCols |= BUC_VALUE;
        }
        multiCmd = new ModifyMultiPropertyOnBa(curBa,fsub,ssub,mt,v,curBa->getDir());
        pzMgr->getUndoStack()->push(multiCmd);
        break;
    case BT_SNDSUB:{
        if(isInteracting)
            return;
        fsub = curBa->getFirstSubject();
        ssub = item->data(Qt::EditRole).value<SecondSubject*>();
        if(!fsub)
            return;
        bool isEnChanged = false;
        if(ssub && !ssub->isEnabled()){
            if(QDialog::Rejected == myHelper::ShowMessageBoxQuesion(tr("二级科目“%1”已被禁用，是否重新启用？").arg(ssub->getName()))){
               QVariant v; v.setValue(curBa->getSecondSubject());
               item->setData(Qt::EditRole,v);
               return;
            }
            isEnChanged = true;
        }
        //如果是银行科目，则根据银行账户所属的币种设置币种对象
        if(ssub && subMgr->getBankSub() == fsub){
            mt = subMgr->getSubMatchMt(ssub);
            if(!mt)
                break;
            Money* curMt = curBa->getMt();
            if(curMt && mt != curMt){
                if(mt != account->getMasterMt() && rates.contains(mt->code()))
                    v = curBa->getValue() / rates.value(mt->code());
                else if(rates.contains(curMt->code()))
                    v = curBa->getValue() * rates.value(curMt->code());
                else
                    v = curBa->getValue();
                updateCols |= BUC_MTYPE;
                updateCols |= BUC_VALUE;
            }
            else
                v = curBa->getValue();
        }
        else{//如果是普通科目，且未设币种，则默认将币种设为本币
            if(!curBa->getMt()){
                mt = account->getMasterMt();
                updateCols |= BUC_MTYPE;
            }
            else
                mt = curBa->getMt();
            v = curBa->getValue();
        }
        if(isEnChanged){
            QUndoCommand* mmd  = new QUndoCommand;
            ModifySndSubEnableProperty* cmdSSub = new ModifySndSubEnableProperty(ssub,true,mmd);
            cmdSSub->setCreator(this);
            multiCmd = new ModifyMultiPropertyOnBa(curBa,fsub,ssub,mt,v,curBa->getDir(),mmd);
            pzMgr->getUndoStack()->push(mmd);
        }
        else{
            multiCmd = new ModifyMultiPropertyOnBa(curBa,fsub,ssub,mt,v,curBa->getDir());
            pzMgr->getUndoStack()->push(multiCmd);
        }
        if(ssub){
            QString tip = ssub->getLName();
            if(fsub == subMgr->getBankSub())
                tip.append(tr("（帐号：%1）").arg(subMgr->getBankAccount(ssub)->accNumber));
            ui->tview->setLongName(tip);
        }
        break;}
    case BT_MTYPE:
        mt = item->data(Qt::EditRole).value<Money*>();
        updateCols |= BUC_MTYPE;
        //如果金额为0，则不必调整金额，否则，必须调整金额
        if(curBa->getValue() != 0){
            if(curBa->getMt()){
                //如果从外币转到本币
                if(mt == account->getMasterMt())
                    v = curBa->getValue() * rates.value(curBa->getMt()->code(),1.0);
                //从甲外币转到乙外币（先将甲外币转到本币，再转到乙外币）
                else if(mt != account->getMasterMt() && curBa->getMt() != account->getMasterMt())
                    v = curBa->getValue() * rates.value(curBa->getMt()->code(),1.0) / rates.value(mt->code(),1.0);
                else   //从本币转到外币
                    v = curBa->getValue() / rates.value(mt->code(),1.0);
                updateCols |= BUC_VALUE;
            }
            else
                v = curBa->getValue();
        }
        else
            v = 0.0;
        multiCmd = new ModifyMultiPropertyOnBa(curBa,curBa->getFirstSubject(),curBa->getSecondSubject(),mt,v,curBa->getDir());
        pzMgr->getUndoStack()->push(multiCmd);
        break;
    case BT_JV:
        v = item->data(Qt::EditRole).value<Double>();
        dir = MDIR_J;
        mt = curBa->getMt();
        if(!mt)
            mt = account->getMasterMt();
        multiCmd = new ModifyMultiPropertyOnBa(curBa,curBa->getFirstSubject(),curBa->getSecondSubject(),mt,v,dir);
        pzMgr->getUndoStack()->push(multiCmd);
        updateCols |= BUC_VALUE;
        break;
    case BT_DV:
        v = item->data(Qt::EditRole).value<Double>();
        dir = MDIR_D;
        mt = curBa->getMt();
        if(!mt)
            mt = account->getMasterMt();
        multiCmd = new ModifyMultiPropertyOnBa(curBa,curBa->getFirstSubject(),curBa->getSecondSubject(),mt,v,dir);
        pzMgr->getUndoStack()->push(multiCmd);
        updateCols |= BUC_VALUE;
        break;
    }
    updateBas(row,1,updateCols);
}

/**
 * @brief PzDialog::creatNewNameItemMapping
 * 使用现存的名称项目，
 * @param row
 * @param col
 * @param ssbu
 */
void PzDialog::creatNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssub)
{
    isInteracting = true;
    if(!curUser->haveRight(allRights.value(Right::Account_Config_SetSndSubject))){
        myHelper::ShowMessageBoxWarning(tr("您没有创建新二级科目的权限！"));
        return;
    }
    if(QMessageBox::information(0,msgTitle_info,tr("确定要使用已有的名称条目“%1”在一级科目“%2”下创建二级科目吗？")
                                .arg(ni->getShortName()).arg(fsub->getName()),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::No){
        BASndSubItem_new* item = static_cast<BASndSubItem_new*>(ui->tview->item(row,col));
        item->setText(curBa->getSecondSubject()?curBa->getSecondSubject()->getName():"");
        delegate->userConfirmed();
        return;
    }
    isInteracting = false;
    ModifyBaSndSubNMMmd* cmd = new ModifyBaSndSubNMMmd(pzMgr,curPz,curBa,subMgr,fsub,ni,1,QDateTime::currentDateTime(),curUser);
    pzMgr->getUndoStack()->push(cmd);
    ssub = cmd->getSecondSubject();
    BaUpdateColumns updateCols;
    updateCols |= BUC_SNDSUB;
    updateBas(row,1,updateCols);
    delegate->userConfirmed();
    ui->tview->edit(ui->tview->model()->index(row,col+1));
}

/**
 * @brief PzDialog::creatNewSndSubject
 * 创建新的二级科目（使用新的名称信息）
 * @param fsub
 * @param ssub
 * @param name
 * @param row
 * @param col
 */
void PzDialog::creatNewSndSubject(int row, int col, FirstSubject* fsub, SecondSubject*& ssub, QString name)
{
    if(!curUser->haveRight(allRights.value(Right::Account_Config_SetSndSubject))){
        myHelper::ShowMessageBoxWarning(tr("您没有创建新二级科目的权限！"));
        return;
    }
    if(QMessageBox::information(0,msgTitle_info,tr("确定要用新的名称条目“%1”在一级科目“%2”下创建二级科目吗？")
                                .arg(name).arg(fsub->getName()),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes){
        CompletSubInfoDialog* dlg = new CompletSubInfoDialog(fsub->getId(),subMgr,0);
        dlg->setName(name);
        NameItemAlias* alias = SubjectManager::getMatchedAlias(name);
        if(alias){
            dlg->setLongName(alias->longName());
            dlg->setRemCode(alias->rememberCode());
        }
        if(QDialog::Accepted == dlg->exec()){
            ModifyBaSndSubNSMmd* cmd =
                new ModifyBaSndSubNSMmd(pzMgr,curPz,curBa,subMgr,fsub,
                                        dlg->getSName(),dlg->getLName(),
                                        dlg->getRemCode(),dlg->getSubCalss(),
                                        QDateTime::currentDateTime(),curUser);
            pzMgr->getUndoStack()->push(cmd);
            ssub = cmd->getSecondSubject();
            BaUpdateColumns updateCols;
            updateCols |= BUC_SNDSUB;
            updateBas(row,1,updateCols);
            delegate->userConfirmed();
            ui->tview->edit(ui->tview->model()->index(row,col));
            //如果新建的名称对象和别名对象两者简称相同，则如果全称也相同，则可以移除此别名，
            //如果全称不同，则将此别名加入到新建名称对象的别名列表
            if(alias && alias->shortName()==curBa->getSecondSubject()->getName()){
                if(alias->longName() != curBa->getSecondSubject()->getLName()){
                    curBa->getSecondSubject()->getNameItem()->addAlias(alias);
                }
                else{//这个实现有点过于草算，因为在用户没有确定保存时，它已经实际地删除了别名，而且这个实现也不支持Undo和Redo操作
                    subMgr->removeNameAlias(alias);
                    delete alias;
                }
            }            
            return;
        }
    }
    SecondSubject* oldSSub = curBa->getSecondSubject();
    BASndSubItem_new* item = static_cast<BASndSubItem_new*>(ui->tview->item(row,col));
    item->setText(oldSSub?oldSSub->getName():"");
    delegate->userConfirmed();
    ui->tview->edit(ui->tview->model()->index(row,col));
}

/**
 * @brief PzDialog::addCopyPrewAction
 *  拷贝上一条分录并插入到当前行
 * @param row
 * @param col
 */
void PzDialog::copyPrewAction(int row)
{
    if(row < 1)
        return;
    BusiAction* ba = new BusiAction(*curPz->getBusiAction(row-1));
    insertBa(ba);
}

/**
 * @brief PzDialog::tabColWidthResized
 * 跟踪记载表格列宽的改变
 * @param index
 * @param oldSize
 * @param newSize
 */
void PzDialog::tabColWidthResized(int index, int oldSize, int newSize)
{
    switch(index){
    case BaTableWidget::SUMMARY:
        states.colSummaryWidth = newSize;
        break;
    case BaTableWidget::FSTSUB:
        states.colFstSubWidth = newSize;
        break;
    case BaTableWidget::SNDSUB:
        states.colSndSubWidth = newSize;
        break;
    case BaTableWidget::MONEYTYPE:
        states.colMtWidth = newSize;
        break;
    case BaTableWidget::JVALUE:
        states.colValueWidth = newSize;
        disconnect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(tabColWidthResized(int,int,int)));
        ui->tview->setColumnWidth(BaTableWidget::DVALUE,newSize);
        connect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(tabColWidthResized(int,int,int)));
        break;
    case BaTableWidget::DVALUE:
        states.colValueWidth = newSize;
        disconnect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(tabColWidthResized(int,int,int)));
        ui->tview->setColumnWidth(BaTableWidget::JVALUE,newSize);
        connect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(tabColWidthResized(int,int,int)));
        break;
    }
    ui->tview->updateSubTableGeometry();
}

/**
 * @brief PzDialog::tabRowHeightResized
 * 跟踪记载表格行高的改变
 * @param index
 * @param oldSize
 * @param newSize
 */
void PzDialog::tabRowHeightResized(int index, int oldSize, int newSize)
{
    states.rowHeight = newSize;
    disconnect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
               this,SLOT(tabRowHeightResized(int,int,int)));
    for(int i = 0; i < ui->tview->rowCount(); ++i)
        ui->tview->setRowHeight(i,newSize);
    ui->tview->updateSubTableGeometry();
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(tabRowHeightResized(int,int,int)));
}

/**
 * @brief PzDialog::extraException
 *  当前凭证集余额产生异常（一般指余额违反了科目的通用约定，比如银行的余额通常在借方）
 * @param ba    引起余额变化的分录对象
 * @param fv    一级科目余额
 * @param fd    一级科目余额方向
 * @param sv    二级科目余额
 * @param sd    二级科目余额方向
 */
void PzDialog::extraException(BusiAction *ba,Double fv, MoneyDirection fd, Double sv, MoneyDirection sd)
{
    //点击消息框关闭后，要引起崩溃，拟通过其他的方式来提示
    QString info = tr("科目“%1-%2”的余额发生异常！--一级科目余额：%3（%4）--二级科目余额：%5（%6）")
                                     .arg(ba->getFirstSubject()->getName())
                                     .arg(ba->getSecondSubject()->getName())
                                     .arg(fv.toString2()).arg(fd==MDIR_J?tr("借"):tr("贷"))
                                     .arg(sv.toString2()).arg(sd==MDIR_J?tr("借"):tr("贷"));
//    QMessageBox::warning(this,tr("警告信息"),info);
    emit showMessage(info,AE_WARNING);
}

/**
 * @brief PzDialog::modifyRate
 *  接收用户设定的汇率，并保存
 */
void PzDialog::modifyRate()
{
    if(pzMgr->getState() == Ps_Jzed)
        return;
    Money* money = ui->cmbMt->itemData(ui->cmbMt->currentIndex()).value<Money*>();
    if(!money)
        return;
    double oldRate = ui->edtRate->text().toDouble();
    bool ok;
    double newRate = QInputDialog::getDouble(this,tr("信息录入"),tr("请输入新的%1年%2月的汇率").arg(pzMgr->year()).arg(pzMgr->month()),
                                             oldRate,0,100,4,&ok);
    if(!ok)
        return;
    Double rate = Double(newRate,4);
    if(rates.value(money->code()) == rate)
        return;
    rates[money->code()] = rate;
    ui->edtRate->setText(rate.toString());
    if(!account->setRates(pzMgr->year(),pzMgr->month(), rates))
        QMessageBox::critical(this,tr("出错信息"),tr("在保存%1年%2月的汇率时出错！"));
    pzMgr->rateChanged();
    if(curPz){
        ui->tview->setJSum(curPz->jsum());
        ui->tview->setDSum(curPz->dsum());
        pzBalanceStateChanged(curPz->isBalance());
        emit rateChanged(pzMgr->month());
    }
}

/**
 * @brief 监视凭证备注内容的改变
 */
void PzDialog::pzCommentChanged()
{
    ModifyPzComment* cmd = new ModifyPzComment(pzMgr,curPz,ui->edtComment->toPlainText());
    pzMgr->getUndoStack()->push(cmd);
}

void PzDialog::pzMemInfoModified(bool changed)
{
    isPzMemModified = changed;
}

/**
 * @brief PzDialog::pzBalanceStateChanged
 *  根据当前凭证的借贷方是否平衡来
 * @param isBalance
 */
void PzDialog::pzBalanceStateChanged(bool isBalance)
{
    ui->tview->setBalance(isBalance);
    if(isBalance)
        ui->lblDiff->setText("                    ");
    else
        ui->lblDiff->setText(tr("借方 - 贷方 = %1")
                             .arg((curPz->jsum()-curPz->dsum()).toString()));
}


/**
 * @brief PzDialog::adjustTableSize
 * 调整表格的行高和列宽
 */
void PzDialog::adjustTableSize()
{
    disconnect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(tabColWidthResized(int,int,int)));
    disconnect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                   this,SLOT(tabRowHeightResized(int,int,int)));
    restoreStateInfo();
    ui->tview->updateSubTableGeometry();
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(tabColWidthResized(int,int,int)));
    connect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                   this,SLOT(tabRowHeightResized(int,int,int)));
}

/**
 * @brief PzDialog::refreshPzContent 刷新凭证内容
 */
void PzDialog::refreshPzContent()
{
    //显示本期凭证总数
    isPzMemModified = false;
    installInfoWatch(false);
    if(curPz == NULL){
        disconnect(curPz,SIGNAL(updateBalanceState(bool)),ui->tview,SLOT(setBalance(bool)));
        ui->dateEdit->setReadOnly(true);
        //ui->dateEdit->clear();
        ui->dateEdit->setDate(QDate());
        ui->edtPzNum->clear();
        ui->spnZbNum->setReadOnly(true);
        ui->spnZbNum->clear();
        ui->spnEncNum->setReadOnly(true);
        ui->spnEncNum->clear();
        ui->edtRUser->clear();
        ui->edtVUser->clear();
        ui->edtBUser->clear();
        ui->edtComment->setReadOnly(true);
        ui->edtComment->clear();
        ui->lblClass->setPixmap(icons_pzcls.value(Pzc_NULL));
        ui->lblState->setPixmap(icons_pzstate.value(Pzs_NULL));
    }
    else{
        connect(curPz,SIGNAL(updateBalanceState(bool)),ui->tview,SLOT(setBalance(bool)));
        ui->dateEdit->setReadOnly(false);
        ui->dateEdit->setDate(curPz->getDate2());
        ui->edtPzNum->setText(QString::number(curPz->number()));
        ui->spnZbNum->setReadOnly(false);
        ui->spnZbNum->setValue(curPz->zbNumber());
        ui->spnEncNum->setReadOnly(false);
        ui->spnEncNum->setValue(curPz->encNumber());
        ui->edtRUser->setText(curPz->recordUser()?curPz->recordUser()->getName():"");
        ui->edtVUser->setText(curPz->verifyUser()?curPz->verifyUser()->getName():"");
        ui->edtBUser->setText(curPz->bookKeeperUser()?curPz->bookKeeperUser()->getName():"");
        ui->edtComment->setReadOnly(false);
        ui->edtComment->setPlainText(curPz->memInfo());
        //ui->edtComment->document()->setModified(true);
        ui->lblClass->setPixmap(icons_pzcls.value(curPz->getPzClass()));
        ui->lblState->setPixmap(icons_pzstate.value(curPz->getPzState()));        
        adjustViewReadonly();
    }
    refreshActions();
    installInfoWatch();
}

/**
 * @brief PzDialog::refreshActions 刷新会计分录
 */
void PzDialog::refreshActions()
{
    ui->tview->setLongName("");
    if(curPz == NULL){
        //validBas = -1;
        delegate->setReadOnly(true);
        ui->tview->clearContents();
        ui->tview->setValidRows(1);        
        return;
    }

    //installDataWatch(false);
    //curPz->removeTailBlank();
    //validBas = curPz->baCount();
    //delegate->setVolidRows(validBas);

    disconnect(ui->tview,SIGNAL(currentCellChanged(int,int,int,int)),
               this,SLOT(currentCellChanged(int,int,int,int)));
    disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    ui->tview->clearContents();
    int maxRows;
    if(curPz->baCount() >= BA_TABLE_MAXROWS)
        maxRows = curPz->baCount()+10;
    else
        maxRows = BA_TABLE_MAXROWS;
    ui->tview->setRowCount(maxRows);
    ui->tview->setValidRows(curPz->baCount()+1);
    //ui->tview->setRowCount(curPz->baCount()+1);
    int i = 0;
    rowSelStates.clear();
    foreach(BusiAction* ba, curPz->baList()){
        refreshSingleBa(i,ba);
        i++;
        rowSelStates<<false;
    }
    initBlankBa(i);
    ui->tview->setBalance(curPz->isBalance());
    pzBalanceStateChanged(curPz->isBalance());
    ui->tview->setJSum(curPz->jsum());
    ui->tview->setDSum(curPz->dsum());
    delegate->setVolidRows(curPz->baCount());

    //refreshVHeaderView();
    //installDataWatch();
    ui->tview->scrollToTop();
    //curRow = -1;
    curBa = NULL;
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    connect(ui->tview,SIGNAL(currentCellChanged(int,int,int,int)),
            this,SLOT(currentCellChanged(int,int,int,int)));

}

/**
 * @brief PzDialog::refreshSingleBa
 * @param index：在表格中的位置
 * @param ba     分录对象
 */
void PzDialog::refreshSingleBa(int row, BusiAction* ba)
{
    BASummaryItem_new* smItem = new BASummaryItem_new(ba->getSummary(), subMgr);
    ui->tview->setItem(row,BaTableWidget::SUMMARY,smItem);
    BAFstSubItem_new* fstItem = new BAFstSubItem_new(ba->getFirstSubject(), subMgr);
    QVariant v;
    v.setValue(ba->getFirstSubject());
    fstItem->setData(Qt::EditRole, v);
    ui->tview->setItem(row,BaTableWidget::FSTSUB,fstItem);
    BASndSubItem_new* sndItem = new BASndSubItem_new(ba->getSecondSubject(), subMgr);
    v.setValue(ba->getSecondSubject());
    sndItem->setData(Qt::EditRole,v);
    ui->tview->setItem(row,BaTableWidget::SNDSUB,sndItem);
    BAMoneyTypeItem_new* mtItem = new BAMoneyTypeItem_new(ba->getMt());
    v.setValue(ba->getMt());
    mtItem->setData(Qt::EditRole, v);
    ui->tview->setItem(row,BaTableWidget::MONEYTYPE,mtItem);
    BAMoneyValueItem_new* jItem,*dItem;
    if(ba->getDir() == DIR_J){
        jItem = new BAMoneyValueItem_new(DIR_J, ba->getValue());
        dItem = new BAMoneyValueItem_new(DIR_D, 0);
    }
    else{
        jItem = new BAMoneyValueItem_new(DIR_J, 0);
        dItem = new BAMoneyValueItem_new(DIR_D, ba->getValue());
    }
    ui->tview->setItem(row,BaTableWidget::JVALUE,jItem);
    ui->tview->setItem(row,BaTableWidget::DVALUE,dItem);
}

/**
 * @brief PzDialog::updateBas 会计分录的局部更新
 * 此方法应用于当凭证的会计分录在程序内部被改变而不是由用户的编辑动作而引起的改变时，
 * 更新显示凭证的最新内容
 * @param row：开始行
 * @param rows：要更新的行数
 * @param col： 要更新的列
 */
void PzDialog::updateBas(int row, int rows, BaUpdateColumns col)
{
    if(row < 0 || row > curPz->baCount()-1 || row+rows > curPz->baCount())
        return;
    if(!col)
        return;

    disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    QVariant v;
    //更新所有列
    if(col.testFlag(BUC_ALL)){
        for(int i = row; i < row+rows; ++i){
            BusiAction* ba = curPz->getBusiAction(i);
            ui->tview->item(i,BT_SUMMARY)->setData(Qt::EditRole,ba->getSummary());
            v.setValue(ba->getFirstSubject());
            ui->tview->item(i,BT_FSTSUB)->setData(Qt::EditRole,v);
            v.setValue(ba->getSecondSubject());
            ui->tview->item(i,BT_SNDSUB)->setData(Qt::EditRole,v);
            v.setValue(ba->getMt());
            ui->tview->item(i,BT_MTYPE)->setData(Qt::EditRole,v);
            v.setValue(ba->getValue());
            if(ba->getDir() == DIR_J){
                ui->tview->item(i,BT_JV)->setData(Qt::EditRole,v);
                ui->tview->item(i,BT_DV)->setData(Qt::EditRole,0);
            }
            else{
                ui->tview->item(i,BT_JV)->setData(Qt::EditRole,0);
                ui->tview->item(i,BT_DV)->setData(Qt::EditRole,v);
            }
        }
        ui->tview->setJSum(curPz->jsum());
        ui->tview->setDSum(curPz->dsum());
    }
    else{
        if(col.testFlag(BUC_SUMMARY)){
            for(int i = row; i < row+rows; ++i){
                ui->tview->item(i,BT_SUMMARY)->
                        setData(Qt::EditRole,curPz->getBusiAction(i)->getSummary());
            }
        }
        if(col.testFlag(BUC_FSTSUB)){
            for(int i = row; i < row+rows; ++i){
                v.setValue(curPz->getBusiAction(i)->getFirstSubject());
                ui->tview->item(i,BT_FSTSUB)->setData(Qt::EditRole,v);
            }
        }
        if(col.testFlag(BUC_SNDSUB)){
            for(int i = row; i < row+rows; ++i){
                v.setValue(curPz->getBusiAction(i)->getSecondSubject());
                ui->tview->item(i,BT_SNDSUB)->setData(Qt::EditRole,v);
            }
        }
        if(col.testFlag(BUC_MTYPE)){
            for(int i = row; i < row+rows; ++i){
                v.setValue(curPz->getBusiAction(i)->getMt());
                ui->tview->item(i,BT_MTYPE)->setData(Qt::EditRole,v);
            }
            ui->tview->setJSum(curPz->jsum());
            ui->tview->setDSum(curPz->dsum());
        }
        if(col.testFlag(BUC_VALUE)){
            for(int i = row; i < row+rows; ++i){
                v.setValue(curPz->getBusiAction(i)->getValue());
                if(curPz->getBusiAction(i)->getDir() == DIR_J){
                    ui->tview->item(i,BT_JV)->setData(Qt::EditRole,v);
                    ui->tview->item(i,BT_DV)->setData(Qt::EditRole,0);
                }
                else{
                    ui->tview->item(i,BT_JV)->setData(Qt::EditRole,0);
                    ui->tview->item(i,BT_DV)->setData(Qt::EditRole,v);
                }
            }            
            ui->tview->setJSum(curPz->jsum());
            ui->tview->setDSum(curPz->dsum());
        }
    }
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
}

/**
 * @brief PzDialog::showInfomation
 * @param info
 * @param level
 */\
void PzDialog::showInfomation(QString info, AppErrorLevel level)
{
    QString s;
    switch(level){
    case AE_OK:
        s = info;
        break;
    case AE_WARNING:
        s = tr("warning:%1").arg(info);
        break;
    case AE_CRITICAL:
        s = tr("general error:%1").arg(info);
        break;
    case AE_ERROR:
        s = tr("fault:%1").arg(info);
        break;
    }
    //ui->lblStateInfo->setStyleSheet(colors.value(level));
    //ui->lblStateInfo->setText(s);
    msgTimer->start();
}

/**
 * @brief PzDialog::isValidLastRow
 * 判断会计分录表格的最末行是否是空分录（即没有输入任何数据）
 * @return
 */
bool PzDialog::isBlankLastRow()
{
    int r = delegate->getVolidRows()-1;
    QTableWidgetItem* item = ui->tview->item(r,BT_SUMMARY);
    item = ui->tview->item(r,BT_FSTSUB);
    item = ui->tview->item(r,BT_SNDSUB);
    item = ui->tview->item(r,BT_MTYPE);
    item = ui->tview->item(r,BT_JV);
    item = ui->tview->item(r,BT_DV);
    if(!ui->tview->item(r,BT_SUMMARY) &&
       !ui->tview->item(r,BT_FSTSUB) &&
       !ui->tview->item(r,BT_SNDSUB) &&
       !ui->tview->item(r,BT_MTYPE) &&
       !ui->tview->item(r,BT_JV) &&
       !ui->tview->item(r,BT_DV))
        return true;
    else
        return false;
}

/**
 * @brief PzDialog::installInfoWatch 连接或断开凭证内容被编辑的信号
 * @param install
 */
void PzDialog::installInfoWatch(bool install)
{
    //其他监视项：
    //凭证状态的改变
    //凭证审核用户、记账用户的改变

    if(install){
        connect(ui->dateEdit,SIGNAL(dateChanged(QDate)),this,SLOT(pzDateChanged(QDate)));
        connect(ui->spnZbNum,SIGNAL(valueChanged(int)),this,SLOT(pzZbNumChanged(int)));
        connect(ui->spnEncNum,SIGNAL(valueChanged(int)),this,SLOT(pzEncNumChanged(int)));
        //connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
        connect(ui->edtComment,SIGNAL(textChanged()),this,SLOT(pzCommentChanged()));
        //connect(ui->edtComment,SIGNAL(modificationChanged(bool)),this,SLOT(pzMemInfoModified(bool)));

    }
    else{
        disconnect(ui->dateEdit,SIGNAL(dateChanged(QDate)),this,SLOT(pzDateChanged(QDate)));
        disconnect(ui->spnZbNum,SIGNAL(valueChanged(int)),this,SLOT(pzZbNumChanged(int)));
        disconnect(ui->spnEncNum,SIGNAL(valueChanged(int)),this,SLOT(pzEncNumChanged(int)));
        //disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
        disconnect(ui->edtComment,SIGNAL(textChanged()),this,SLOT(pzCommentChanged()));
        //disconnect(ui->edtComment,SIGNAL(modificationChanged(bool)),this,SLOT(pzMemInfoModified(bool)));

    }
}

/**
 * @brief 从摘要信息中提取客户名信息，并试图从指定主目中找出与此名称对应的子目
 * @param fsub      一级科目对象
 * @param summary   分录的摘要信息（XX[prefixe][客户名][suffixe]XX）,X代表其他任意字符
 * @param prefixe   客户名的前缀
 * @param suffix    客户名的后缀
 * @return
 */
SecondSubject *PzDialog::getAdapterSSub(FirstSubject *fsub, QString summary, QString prefixe, QString suffix)
{
    int pi = summary.indexOf(prefixe);
    if(pi==-1)
        return 0;
    int si = summary.indexOf(suffix);
    if(si==-1)
        return 0;
    int index = pi+prefixe.count();
    if((si - index) <= 1)
        return  0;
    QString name = summary.mid(index,si-index);
    return fsub->getChildSub(name);
}

/**
 * @brief 打开发票统计窗口
 */
void PzDialog::openInvoiceStatForm()
{
    if(!invoiceStatForm){
        invoiceStatForm = new InvoiceStatForm(pzMgr,this);
        invoiceStatForm->resize(800,500);
    }
    invoiceStatForm->show();
}

/**
 * @brief PzDialog::initResources 初始化资源
 */
void PzDialog::initResources()
{
    //初始化各种凭证类别对应的图标
    icons_pzcls[Pzc_Hand] = QPixmap(":/images/PzClass/recording.png");
    icons_pzcls[Pzc_GdzcImp] = QPixmap(":/images/PzClass/gdzcImp.png");
    icons_pzcls[Pzc_GdzcZj] = QPixmap(":/images/PzClass/gdzcZj.png");
    icons_pzcls[Pzc_DtfyImp] = QPixmap(":/images/PzClass/dtfyImp.png");
    icons_pzcls[Pzc_DtfyTx] = QPixmap(":/images/PzClass/dtfyTx.png");
    icons_pzcls[Pzc_Jzhd_Bank] = QPixmap(":/images/PzClass/hdsyBank.png");
    icons_pzcls[Pzc_Jzhd_Ys] = QPixmap(":/images/PzClass/hdsyYs.png");
    icons_pzcls[Pzc_Jzhd_Yf] = QPixmap(":/images/PzClass/hdsyYf.xcf");
    icons_pzcls[Pzc_JzsyIn] = QPixmap(":/images/PzClass/jzsyS.png");
    icons_pzcls[Pzc_JzsyFei] = QPixmap(":/images/PzClass/jzsyF.png");
    icons_pzcls[Pzc_Jzlr] = QPixmap(":/images/PzClass/jzBnlr.png");

    //初始化各种凭证状态对应的图标
    icons_pzstate[Pzs_Recording] = QPixmap(":/images/PzState/init.png");
    icons_pzstate[Pzs_Verify] = QPixmap(":/images/PzState/verify.png");
    icons_pzstate[Pzs_Instat] = QPixmap(":/images/PzState/instat.png");
    icons_pzstate[Pzs_Repeal] = QPixmap(":/images/PzState/repeal.png");
}

/**
 * @brief PzDialog::initBlankBa
 * 初始化空白会计分录对应的行
 */
void PzDialog::initBlankBa(int row)
{
    BASummaryItem_new* smItem = new BASummaryItem_new("", subMgr);
    ui->tview->setItem(row,BaTableWidget::SUMMARY,smItem);
    BAFstSubItem_new* fstItem = new BAFstSubItem_new(NULL, subMgr);
    ui->tview->setItem(row,BaTableWidget::FSTSUB,fstItem);
    BASndSubItem_new* sndItem = new BASndSubItem_new(NULL, subMgr);
    ui->tview->setItem(row,BaTableWidget::SNDSUB,sndItem);
    BAMoneyTypeItem_new* mtItem = new BAMoneyTypeItem_new(NULL);
    ui->tview->setItem(row,BaTableWidget::MONEYTYPE,mtItem);
    BAMoneyValueItem_new* jItem,*dItem;
    jItem = new BAMoneyValueItem_new(DIR_J, 0);
    dItem = new BAMoneyValueItem_new(DIR_D, 0);
    ui->tview->setItem(row,BaTableWidget::JVALUE,jItem);
    ui->tview->setItem(row,BaTableWidget::DVALUE,dItem);
}


/////////////////////////////////HistoryPzForm////////////////////////////////////


HistoryPzForm::HistoryPzForm(PingZheng *pz, QByteArray *sinfo, QWidget *parent) :QDialog(parent),
    ui(new Ui::HistoryPzForm),pz(pz)
{
    ui->setupUi(this);
    ReadOnlyItemDelegate* delegate = new ReadOnlyItemDelegate(this);
    ui->tview->setItemDelegate(delegate);
    //ui->tview->setColumnCount(6);
    setCommonState(sinfo);
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));
    curY = 0;
    curM = 0;
    ui->tview->setRowCount(BA_TABLE_MAXROWS);
    viewPzContent();
}

HistoryPzForm::~HistoryPzForm()
{
    delete ui;
}

void HistoryPzForm::setPz(PingZheng *pz)
{
    this->pz = pz;
    viewPzContent();
}

/**
 * @brief HistoryPzForm::setCurBa
 *  如果指定的会计分录存在于当前显示的凭证中，则选中它
 * @param bid
 */
void HistoryPzForm::setCurBa(int bid)
{
    if(!pz || !bid || (pz->baCount() == 0))
        return;
    for(int i = 0; i < pz->baCount(); ++i){
        if(pz->getBusiAction(i)->getId() == bid){
            ui->tview->selectRow(i);
            ui->tview->scrollTo(ui->tview->model()->index(i,0));
            return;
        }
    }
}

void HistoryPzForm::setCommonState(QByteArray *info)
{
    if(!info || info->isEmpty()){
        colWidths<<300<<100<<100<<50<<200<<200;
    }
    else{
        QBuffer bf(info);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        qint8 i8;
        qint16 i16;

        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            colWidths<<i16;
        }
        bf.close();
    }
    for(int i = 0; i < colWidths.count(); ++i)
        ui->tview->setColumnWidth(i,colWidths.at(i));

}

QByteArray *HistoryPzForm::getCommonState()
{
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 i8;
    qint16 i16;

    //显示业务活动的表格，固定为6列
    i8 = 6;
    out<<i8;
    for(int i = 0; i < 6; ++i){
        i16 = colWidths[i];
        out<<i16;
    }
    bf.close();
    return info;
}

void HistoryPzForm::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    colWidths[logicalIndex] = newSize;
    if(logicalIndex == BaTableWidget::JVALUE){
        disconnect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                    this,SLOT(colWidthChanged(int,int,int)));
        ui->tview->setColumnWidth(BaTableWidget::DVALUE,newSize);
        connect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(colWidthChanged(int,int,int)));
    }
    else if(logicalIndex == BaTableWidget::DVALUE){
        disconnect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                    this,SLOT(colWidthChanged(int,int,int)));
        ui->tview->setColumnWidth(BaTableWidget::JVALUE,newSize);
        connect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(colWidthChanged(int,int,int)));
    }
    ui->tview->updateSubTableGeometry();
}

void HistoryPzForm::viewPzContent()
{
    if(!pz){
        ui->edtDate->clear();
        ui->edtNumber->clear();
        ui->edtEncNum->clear();
        ui->tview->clearContents();
        ui->edtRUser->clear();
        ui->edtVUser->clear();
        ui->edtBUser->clear();
        ui->edtRate->clear();
        ui->edtMem->clear();
    }
    else{
        int y = pz->getDate2().year();
        int m = pz->getDate2().month();
        if((curY != y) ||(curM != m)){
            curY = y; curM = m;
            viewRates(y,m);
        }
        ui->edtDate->setText(pz->getDate());
        ui->edtNumber->setText(QString::number(pz->number()));
        ui->edtEncNum->setText(QString::number(pz->encNumber()));
        ui->edtRUser->setText(pz->recordUser()?pz->recordUser()->getName():"");
        ui->edtVUser->setText(pz->verifyUser()?pz->verifyUser()->getName():"");
        ui->edtBUser->setText(pz->bookKeeperUser()?pz->bookKeeperUser()->getName():"");
        ui->edtMem->setPlainText(pz->memInfo());
        viewBusiactions();
    }

}

void HistoryPzForm::viewBusiactions()
{
    if(!pz)
        return;
    ui->tview->clearContents();
    if(pz->baCount()+3>ui->tview->rowCount())
        ui->tview->setRowCount(pz->baCount()+3);
    else if(pz->baCount()+3<BA_TABLE_MAXROWS)
        ui->tview->setRowCount(BA_TABLE_MAXROWS);
    for(int i = 0; i < pz->baCount(); ++i)
        refreshSingleBa(i,pz->getBusiAction(i));
    ui->tview->setBalance(pz->isBalance());
    ui->tview->setJSum(pz->jsum());
    ui->tview->setDSum(pz->dsum());
    ui->tview->scrollToTop();
}

void HistoryPzForm::refreshSingleBa(int row, BusiAction *ba)
{
    QTableWidgetItem* item,*item2;
    item = new QTableWidgetItem(ba->getSummary());
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(row,BaTableWidget::SUMMARY,item);
    item = new QTableWidgetItem(ba->getFirstSubject()?ba->getFirstSubject()->getName():"");
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(row,BaTableWidget::FSTSUB,item);
    item = new QTableWidgetItem(ba->getSecondSubject()?ba->getSecondSubject()->getName():"");
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(row,BaTableWidget::SNDSUB,item);
    item = new QTableWidgetItem(ba->getMt()->name());
    item->setTextAlignment(Qt::AlignCenter);
    ui->tview->setItem(row,BaTableWidget::MONEYTYPE,item);
    if(ba->getDir() == MDIR_J){
        item = new QTableWidgetItem(ba->getValue().toString());
        item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        item2 = NULL;
    }
    else{
        item = NULL;
        item2 = new QTableWidgetItem(ba->getValue().toString());
        item2->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    }
    ui->tview->setItem(row,BaTableWidget::JVALUE,item);
    ui->tview->setItem(row,BaTableWidget::DVALUE,item2);
}

void HistoryPzForm::adjustTableSize()
{
}

void HistoryPzForm::viewRates(int y, int m)
{
    if(!pz)
        return;
    ui->cmbRates->clear();
    ui->edtRate->clear();
    int key = y*100+m;
    if(!rates.contains(key)){
        rates[key] = QHash<Money*,Double>();
        pz->parent()->getAccount()->getRates(y,m,rates[key]);
    }
    QHashIterator<Money*,Double> it(rates[key]);
    while(it.hasNext()){
        it.next();
        QVariant v;
        v.setValue<Money*>(it.key());
        ui->cmbRates->addItem(it.key()->name(),v);
    }
    if(ui->cmbRates->count() != 0){
        Money* mt = ui->cmbRates->itemData(ui->cmbRates->currentIndex()).value<Money*>();
        ui->edtRate->setText(rates.value(key).value(mt).toString(true));
    }
}

void PzDialog::on_btnOk_clicked()
{
    if(!pzMgr->getUndoStack()->isClean())
        pzMgr->save();
    MyMdiSubWindow* p = qobject_cast<MyMdiSubWindow*>(parent());
    if(p)
        p->close();
    else
        close();
}

void PzDialog::on_btnCancel_clicked()
{
    if(!pzMgr->getUndoStack()->isClean())
        pzMgr->rollback();
    MyMdiSubWindow* p = qobject_cast<MyMdiSubWindow*>(parent());
    if(p)
        p->close();
    else
        close();
}
