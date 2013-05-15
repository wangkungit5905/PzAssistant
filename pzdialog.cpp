#include <QKeyEvent>

#include "pzdialog.h"
#include "cal.h"
#include "global.h"
#include "logs/Logger.h"
#include "commands.h"
#include "dialogs.h"
#include "PzSet.h"
#include "statements.h"
#include "completsubinfodialog.h"

#include "ui_pzdialog.h"
////////////////////////////BaTableWidget////////////////////////////////////
BaTableWidget::BaTableWidget(QWidget *parent):QTableWidget(parent)
{
    //构造合计栏表格
    sumTable = new QTableWidget(this);
    sumTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sumTable->setMaximumHeight(30);
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
    QTableWidgetItem* item = new QTableWidgetItem(tr("sum"));
    item->setTextAlignment(Qt::AlignCenter);
    sumTable->setItem(0,1,item);

    jSumItem = new BAMoneyValueItem_new(DIR_J,0.00,QColor(Qt::red));
    //jSumItem->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);
    jSumItem->setFlags(falgs);
    sumTable->setItem(0,2,jSumItem);
    dSumItem = new BAMoneyValueItem_new(DIR_D,0.00,QColor(Qt::red));
    //dSumItem->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);
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

///**
// * @brief BaTableWidget::processShortcut
// * 处理快捷键的主入口
// */
//void BaTableWidget::processShortcut()
//{
//    LOG_INFO("enter processShortcut()!");
//    if(sender() == shortCut){
//        LOG_INFO("shortcut Ctrl+= is actived!");
//    }
//}




/////////////////////////////PzDialog////////////////////////////////
PzDialog::PzDialog(int y, int m, PzSetMgr *psm, const StateInfo states, QWidget *parent)
    : QWidget(parent),ui(new Ui::pzDialog),curRow(-1),isInteracting(false)
{
    ui->setupUi(this);
    curPz = NULL;
    initResources();
    if(states.isValid)
        this->states = states;
    else{
        this->states.rowHeight = PZEW_DEFROWHEIGHT;
        this->states.colSummaryWidth = PZEW_DEFCW_SUMMARY;
        this->states.colFstSubWidth = PZEW_DEFCW_FS;
        this->states.colSndSubWidth = PZEW_DEFCW_SS;
        this->states.colMtWidth = PZEW_DEFCW_MT;
        this->states.colValueWidth = PZEW_DEFCW_V;
    }


    msgTimer = new QTimer(this);
    msgTimer->setInterval(INFO_TIMEOUT);
    msgTimer->setSingleShot(true);
    connect(msgTimer, SIGNAL(timeout()),this,SLOT(msgTimeout()));

    if(!psm){
        showInfomation(tr("Ping Zheng Set object is NULL!"),AE_WARNING);
        return;
    }

    //初始化快捷键
    sc_copyprev = new QShortcut(QKeySequence("Ctrl+="),this);
    sc_save = new QShortcut(QKeySequence("Ctrl+s"),this);
    sc_copy = new QShortcut(QKeySequence("Ctrl+c"),this);
    sc_cut = new QShortcut(QKeySequence("Ctrl+x"),this);
    sc_paster = new QShortcut(QKeySequence("Ctrl+v"),this);
    connect(sc_save,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_copyprev,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_copy,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_cut,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_paster,SIGNAL(activated()),this,SLOT(processShortcut()));

    pmg = psm;
    account = pmg->getAccount();
    subMgr = account->getSubjectManager();
    delegate = new ActionEditItemDelegate(subMgr,this);
    ui->tview->setItemDelegate(delegate);
    if(!pmg->isOpened() && !pmg->open(y,m)){
        showInfomation(tr("Ping Zheng Set don't open!"),AE_WARNING);
        //curPz = NULL;
        //curIdx = -1;
    }
    else{
        //显示本期汇率
        rates = pmg->getRates();
        if(rates.isEmpty()){
            showInfomation(tr("Don't get current period exchange rates"),AE_WARNING);
            return;
        }
        //rates.remove(account->getMasterMtObject());//移除本币
        QHash<int, Money*> mts = account->getAllMoneys();
        mts.remove(account->getMasterMt()->code());  //移除本币
        QHashIterator<int,Double> it(rates);
        QVariant v;
        while(it.hasNext()){
            it.next();
            Money* mt = mts.value(it.key());
            v.setValue<Money*>(mt);
            ui->cmbMt->addItem(mt->name(),v);
        }
        ui->cmbMt->setCurrentIndex(0);
        connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),
                this,SLOT(moneyTypeChanged(int)));
        moneyTypeChanged(0);
        //rates[account->getMasterMtObject()] = 1.00;

        //显示本期凭证总数
        //ui->edtPzCount->setText(QString::number(pzSet->getPzCount()));

        QString s = tr("current period pingzheng amount:%1 among").arg(pmg->getPzCount());
        int repeal = pmg->getStatePzCount(Pzs_Repeal);
        int record = pmg->getStatePzCount(Pzs_Recording);
        int verify = pmg->getStatePzCount(Pzs_Verify);
        int instat = pmg->getStatePzCount(Pzs_Instat);
        if(record != 0)
            s.append(tr("(Don't verify:%1)").arg(record));
        if(verify != 0)
            s.append(tr("(Haved verify:%1)").arg(verify));
        if(instat != 0)
            s.append(tr("(Have bookered:%1)").arg(instat));
        if(repeal != 0)
            s.append(tr("(Have repeal:%1)").arg(repeal));
        showInfomation(s);

        curPz = pmg->first();
        refreshPzContent();

        //connect(delegate,SIGNAL(updateSndSubject(int,int,SecondSubject*)),
        //        this,SLOT(updateSndSubject(int,int,SecondSubject*)));
        connect(delegate,SIGNAL(crtNewNameItemMapping(int,int,FirstSubject*,SubjectNameItem*,SecondSubject*&)),
                this,SLOT(creatNewNameItemMapping(int,int,FirstSubject*,SubjectNameItem*,SecondSubject*&)));
        connect(delegate,SIGNAL(crtNewSndSubject(int,int,FirstSubject*,SecondSubject*&,QString)),
                this,SLOT(creatNewSndSubject(int,int,FirstSubject*,SecondSubject*&,QString)));
        connect(ui->tview,SIGNAL(currentCellChanged(int,int,int,int)),
                this,SLOT(currentCellChanged(int,int,int,int)));
        //connect(delegate,SIGNAL(moveNextRow(int)),this,SLOT(moveToNextBa(int)));

        adjustTableSize();
        connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(tabColWidthResized(int,int,int)));
        connect(ui->tview->verticalHeader(),SIGNAL(sectionResized(int,int,int)),
                       this,SLOT(tabRowHeightResized(int,int,int)));
    }
}

PzDialog::~PzDialog()
{
    //delete msgTimer;
    delete ui;
    //
}

/**
 * @brief PzDialog::setReadonly
 * 根据当前凭证的审核状态和凭证集的状态，设置凭证内容显示部件的只读属性
 * @param r
 */
void PzDialog::setReadonly()
{
    //综合凭证集状态和凭证状态决定凭证是否可编辑
    bool b = curPz->getPzState() != Pzs_Recording;
    ui->dateEdit->setReadOnly(b);
    ui->spnZbNum->setReadOnly(b);
    ui->spnEncNum->setReadOnly(b);
    delegate->setReadOnly(b);
    //告诉代理当前凭证的有效行数
    //if(!b)
    //    delegate->setVolidRows(curPz->baCount());
}

/**
 * @brief PzDialog::restoreState 恢复窗口状态
 * @param datas 第1个元素表示表格行高，后6个元素表示表格列宽
 */
void PzDialog::restoreState()
{
    ui->tview->setColumnWidth(BaTableWidget::SUMMARY,states.colSummaryWidth);
    ui->tview->setColumnWidth(BaTableWidget::FSTSUB,states.colFstSubWidth);
    ui->tview->setColumnWidth(BaTableWidget::SNDSUB,states.colSndSubWidth);
    ui->tview->setColumnWidth(BaTableWidget::MONEYTYPE,states.colMtWidth);
    ui->tview->setColumnWidth(BaTableWidget::JVALUE,states.colValueWidth);
    ui->tview->setColumnWidth(BaTableWidget::DVALUE,states.colValueWidth);
    for(int i = 0; i < ui->tview->rowCount(); ++i)
        ui->tview->setRowHeight(i,states.rowHeight);
//    baColWidths.clear();
//    if(datas.count() != 7){
//        baRowHeight = 20;
//        baColWidths<<DEFCW_SUMMARY<<DEFCW_FS<<DEFCW_SS<<DEFCW_MT<<DEFCW_JV<<DEFCW_DV;
//    }
//    else{
//        baRowHeight = datas.at(0);
//        for(int i = 1; i < 7; ++i)
//            baColWidths<<datas.at(i);
//    }
//    for(int i = 0; i < 6; ++i)
//        ui->tview->setColumnWidth(i,baColWidths.at(i));
//    for(int i = 0; i < ui->tview->rowCount(); ++i)
//        ui->tview->setRowHeight(i,baRowHeight);
}

///**
// * @brief PzDialog::getState 获取窗口的状态信息
// * @return
// */
//PzDialog::stateInfo *PzDialog::getState()
//{


////    datas<<baRowHeight;
////    for(int i = 0; i < 6; ++i)
////        datas<<ui->tview->columnWidth(i);
//}

/**
 * @brief PzDialog::updateContent
 * 更新凭证编辑窗口，以显示最新的修改内容，这可能是通过undo框架引起的
 * 为了缩小更新的界面范围，要增加两个参数（凭证的哪个部分进行更新，索引【在更新某个会计分录是使用】，会计分录的哪个部分）
 */
void PzDialog::updateContent()
{
    curPz = pmg->getCurPz();
    refreshPzContent();
}

void PzDialog::moveToFirst()
{
    curPz = pmg->first();
    refreshPzContent();
}

void PzDialog::moveToPrev()
{
    curPz = pmg->previou();
    refreshPzContent();
}

void PzDialog::moveToNext()
{
    curPz = pmg->next();
    refreshPzContent();
}

void PzDialog::moveToLast()
{
    curPz = pmg->last();
    refreshPzContent();
}

/**
 * @brief PzDialog::seek 快速定位指定号的凭证
 * @param num
 */
void PzDialog::seek(int num)
{
    curPz = pmg->seek(num);
    refreshPzContent();
}

void PzDialog::addPz()
{
    //进行凭证集状态检测，以决定是否可以添加凭证
    PingZheng* oldCurPz = curPz;
    curPz = new PingZheng(pmg);
    QString ds;
    if(!oldCurPz)
        ds = QDate(pmg->year(),pmg->month(),1).toString(Qt::ISODate);
    else
        ds = pmg->getPz(pmg->getPzCount())->getDate();
    curPz->setDate(ds);
    curPz->setNumber(pmg->getPzCount()+1);
    curPz->setZbNumber(pmg->getMaxZbNum()+1);
    curPz->setPzClass(Pzc_Hand);
    curPz->setRecordUser(curUser);
    curPz->setPzState(Pzs_Recording);
    AppendPzCmd* cmd = new AppendPzCmd(pmg,curPz);
    pmg->getUndoStack()->push(cmd);
    refreshPzContent();
}

void PzDialog::insertPz()
{
    //进行凭证集状态检测，以决定是否可以添加凭证
    PingZheng* oldCurPz = curPz;
    curPz = new PingZheng(pmg);
    QString ds;
    if(!oldCurPz)
        ds = QDate(pmg->year(),pmg->month(),1).toString(Qt::ISODate);
    else
        ds = oldCurPz->getDate();
    curPz->setDate(ds);
    curPz->setNumber(oldCurPz->number());
    curPz->setZbNumber(pmg->getMaxZbNum()+1);
    curPz->setPzClass(Pzc_Hand);
    curPz->setRecordUser(curUser);
    curPz->setPzState(Pzs_Recording);
    InsertPzCmd* cmd = new InsertPzCmd(pmg,curPz);
    pmg->getUndoStack()->push(cmd);
    refreshPzContent();
}

void PzDialog::removePz()
{
    DelPzCmd* cmd = new DelPzCmd(pmg,curPz);
    pmg->getUndoStack()->push(cmd);
    curPz = pmg->getCurPz();
    refreshPzContent();
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
    ModifyBaMoveCmd* cmd = new ModifyBaMoveCmd(pmg,curPz,curBa,1,pmg->getUndoStack());
    pmg->getUndoStack()->push(cmd);
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
    ModifyBaMoveCmd* cmd = new ModifyBaMoveCmd(pmg,curPz,curBa,-1,pmg->getUndoStack());
    pmg->getUndoStack()->push(cmd);
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
    AppendBaCmd* cmd = new AppendBaCmd(pmg,curPz,curBa);
    pmg->getUndoStack()->push(cmd);
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
void PzDialog::insertBa()
{
    if(curRow == -1)
        return;
    curBa = new BusiAction;
    InsertBaCmd* cmd = new InsertBaCmd(pmg,curPz,curBa,curRow);
    pmg->getUndoStack()->push(cmd);
    int rows = curPz->baCount();

    disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    //ui->tview->setRowCount(rows+1);
    ui->tview->setValidRows(rows+1);
    delegate->setVolidRows(rows);
    initBlankBa(rows);
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));

    BaUpdateColumns updateCols;
    updateCols |= BUC_ALL;
    updateBas(curRow,rows-curRow,updateCols);
    ui->tview->setCurrentCell(curRow,BT_SUMMARY);
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
    QUndoCommand* mmd = new QUndoCommand(tr("remove busiaction(%1) in pingzheng(%2#)")
                                         .arg(rowStr).arg(curPz->number()));
    foreach(int r, selRows){
        BusiAction* b = curPz->getBusiAction(r);
        ModifyBaDelCmd* cmd = new ModifyBaDelCmd(pmg,curPz,curPz->getBusiAction(r),mmd);
    }
    pmg->getUndoStack()->push(mmd);
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

//void PzDialog::setPz(PingZheng *pz)
//{
//    curPz = pz;
//    refreshPzContent();
//}

/**
 * @brief PzDialog::msgTimeout 状态信息显示超时处理
 */
void PzDialog::msgTimeout()
{
    ui->lblStateInfo->setText("");
}

/**
 * @brief PzDialog::moneyTypeChanged 监视外币选择的改变
 * @param index
 */
void PzDialog::moneyTypeChanged(int index)
{
    ui->edtRate->setText(rates.value(ui->cmbMt->itemData(index).value<Money*>()->code()).toString());
}

/**
 * @brief PzDialog::updateSndSubject
 * 更新会计分录的二级科目（因为该二级科目是新建的）
 * @param row
 * @param col
 * @param ssub
 */
//void PzDialog::updateSndSubject(int row, int col, SecondSubject *ssub)
//{
//    //curBa->setSecondSubject(ssub);
//    BusiAction* ba = curPz->getBusiAction(row);
//    ba->setSecondSubject(ssub);

    //refreshSingleBa(row,ba);
    //BASndSubItem* item = qobject_cast<BASndSubItem*>(ui->tview->item(row,col));

//    BASndSubItem* item = new BASndSubItem(ssub,subMgr);
//    QVariant v;
//    v.setValue(ssub);
//    item->setData(Qt::UserRole,v);
//    ui->tview->setItem(row,col,item);

//    QVariant v;
//    v.setValue(ssub);
//    ui->tview->model()->setData(ui->tview->model()->index(row,col),v);
//    LOG_INFO(QString("enter updateSndSubject(%1,%2,%3)").arg(row).arg(col).arg(ssub->getName()));
    //    ui->tview->update(ui->tview->model()->index(row,col));
//}

/**
 * @brief PzDialog::processShortcut
 * 处理快捷键的主入口
 */
void PzDialog::processShortcut()
{
    if(sender() == sc_save)
        save();
    //编辑会计分录相关的快捷键
    else if(ui->tview->hasFocus()){
        if(sender() == sc_copyprev)
            copyPreviouBa();
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
            CutBaCmd* cmd = new CutBaCmd(pmg,curPz,rows,&clb_Bas);
            pmg->getUndoStack()->push(cmd);
            refreshActions();
        }
        else if(sender() == sc_paster){
            if(clb_Bas.empty())
                return;
            if(curRow == -1)
                return;
            int row = curRow;
            int rows = clb_Bas.count();
            PasterBaCmd* cmd = new PasterBaCmd(pmg,curPz,curRow,&clb_Bas);
            pmg->getUndoStack()->push(cmd);
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
 * 保存凭证集和在编辑凭证集的过程中对科目的更改
 */
void PzDialog::save()
{
    LOG_INFO("shortcut save is actived!");
    //subMgr->save();
    //pzSet->save();
}

/**
 * @brief PzDialog::moveToNextBa
 * 将当前会计分录设置为下一条（如果当前是最后一条，则自动创建一个新的会计分录）
 * @param row（基于0）
 */
void PzDialog::moveToNextBa(int row)
{
    LOG_INFO(QString("move to next Bauiaction()%1").arg(row));
    if(row == curPz->baCount()-1){
        delegate->setVolidRows(row+2);
        initBlankBa(row+1);
    }
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
    //如果
//    if(currentRow == curPz->baCount())
//        delegate->setVolidRows(currentRow+1);
    //moveToNextBa(currentRow);
    curRow = currentRow;
    curBa = curPz->getBusiAction(currentRow);

}

/**
 * @brief PzDialog::pzDateChanged
 * 凭证日期改变
 * @param date
 */
void PzDialog::pzDateChanged(const QDate &date)
{
    //curPz->setDate(date);
    ModifyPzDateCmd*  cmd = new ModifyPzDateCmd(pmg,curPz,date.toString(Qt::ISODate));
    pmg->getUndoStack()->push(cmd);
}

/**
 * @brief PzDialog::pzZbNumChanged
 * 凭证自编号改变
 * @param i
 */
void PzDialog::pzZbNumChanged(int num)
{
    //curPz->setZbNumber(num);
    ModifyPzZNumCmd* cmd = new ModifyPzZNumCmd(pmg,curPz,num);
    pmg->getUndoStack()->push(cmd);
}

/**
 * @brief PzDialog::pzEncNumChanged
 * 凭证附件数改变
 * @param i
 */
void PzDialog::pzEncNumChanged(int num)
{
    //curPz->setEncNumber(num);
    ModifyPzEncNumCmd* cmd = new ModifyPzEncNumCmd(pmg,curPz,num);
    pmg->getUndoStack()->push(cmd);
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
    LOG_INFO(QString("cell(%1,%2) data changed !").arg(row).arg(col));

    //如果是备用行，则将备用行升级为有效行，再添加新的备用行
    int rows = ui->tview->getValidRows();
    if(row == rows - 1){
        curBa = new BusiAction;
        AppendBaCmd* cmd = new AppendBaCmd(pmg,curPz,curBa);
        pmg->getUndoStack()->push(cmd);
        int rows = curPz->baCount();
        disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
        ui->tview->setValidRows(rows+1);
        delegate->setVolidRows(rows);
        initBlankBa(rows);
        connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    }

    if(!curBa)
        return;

    QUndoCommand* mmd;
    ModifyBaFSubMmd* mfCmd;
    ModifyBaMtMmd* mmCmd;
    ModifyBaSummaryCmd* scmd;
    ModifyBaFSubCmd* fscmd;
    ModifyBaSSubCmd* sscmd;
    ModifyBaMtCmd*   mtcmd;
    ModifyBaValueCmd* vcmd;
    FirstSubject* fsub;
    SecondSubject* ssub;
    Double v;
    Money* mt;
    MoneyDirection dir;
    BaUpdateColumns updateCols;
    QVariant va;

    switch(col){
    case BT_SUMMARY:
        scmd = new ModifyBaSummaryCmd(pmg,curPz,curBa,item->data(Qt::EditRole).toString());
        pmg->getUndoStack()->push(scmd);
        break;
    case BT_FSTSUB:
        fsub = item->data(Qt::EditRole).value<FirstSubject*>();
        //ssub = ui->tview->item(row,BT_SNDSUB)->data(Qt::EditRole).value<SecondSubject*>();
        mfCmd = new ModifyBaFSubMmd(tr("set busiaction(%1) of first subject to %2 in pingzheng(%3#")
                .arg(curBa->getNumber()).arg(fsub->getName()).arg(curPz->number()),
                                    pmg,curPz,curBa,pmg->getUndoStack());
        fscmd = new ModifyBaFSubCmd(pmg,curPz,curBa,fsub,mfCmd);
        //如果一级科目是现金，则默认设置二级科目为账户的本币，如果是外币,要将币种也设为本币
        //如果金额不为零还要调整金额
        if(fsub == subMgr->getCashSub()){
            ssub = fsub->getDefaultSubject();
            sscmd = new ModifyBaSSubCmd(pmg,curPz,curBa,ssub,mfCmd);
            updateCols |= BUC_SNDSUB;
            mt = account->getMasterMt(); //本币
            if(mt != curBa->getMt()){
                mtcmd = new ModifyBaMtCmd(pmg,curPz,curBa,mt,mfCmd);
                updateCols |= BUC_MTYPE;
                dir = curBa->getDir();
                if(dir == MDIR_J)
                    v = ui->tview->item(row,BT_JV)->data(Qt::EditRole).toDouble();
                else
                    v = ui->tview->item(row,BT_JV)->data(Qt::EditRole).toDouble();
                if(v != 0){
                    v = v * rates.value(curBa->getMt()->code());
                    vcmd = new ModifyBaValueCmd(pmg,curPz,curBa,v,dir,mfCmd);
                    updateCols |= BUC_VALUE;
                }
            }
        }
        //如果当前设置的二级科目不属于当前一级科目，则清空二级科目的设置
        else if(!curBa->getSecondSubject() && !fsub->containChildSub(ssub)){
            sscmd = new ModifyBaSSubCmd(pmg,curPz,curBa,NULL,mfCmd);
            updateCols |= BUC_SNDSUB;
        }

        pmg->getUndoStack()->push(mfCmd);
        break;
    case BT_SNDSUB:
        LOG_INFO("enter BaDataChanged() function case BT_SNDSUB !");
        if(isInteracting)
            return;
        fsub = curBa->getFirstSubject();
        ssub = item->data(Qt::EditRole).value<SecondSubject*>();
        mmd = new QUndoCommand(tr("set busiaction(%1) of second subject to %2 in pingzheng(%3)")
                                             .arg(curBa->getNumber()).arg(ssub->getName()).arg(curPz->number()));
        sscmd = new ModifyBaSSubCmd(pmg,curPz,curBa,ssub,mmd);

        //如果是银行科目，则根据银行账户所属的币种设置币种对象
        if(subMgr->getBankSub() == fsub){
            mt = subMgr->getSubMatchMt(ssub);
            if(mt != curBa->getMt()){
                mtcmd = new ModifyBaMtCmd(pmg,curPz,curBa,mt,mmd);
                updateCols |= BUC_MTYPE;
                //如果需要，当值不为0时，调整金额
                //if(v != 0){
                //    vcmd = new ModifyBaValueCmd(pzSet,curPz,curBa,v,mmd);
                //}
            }
        }        
        else{//如果是普通科目，且未设币种，则默认将币种设为本币
            if(!curBa->getMt()){
                mtcmd = new ModifyBaMtCmd(pmg,curPz,curBa,account->getMasterMt(),mmd);
                updateCols |= BUC_MTYPE;
            }
        }
        pmg->getUndoStack()->push(mmd);
        break;
    case BT_MTYPE:
        mt = item->data(Qt::EditRole).value<Money*>();
        dir = curBa->getDir();
        mmCmd = new ModifyBaMtMmd(tr("set busiaction(%1) of money type to %2 in pingzheng(%3#)")
                                    .arg(curBa->getNumber()).arg(mt->name()).arg(curPz->number()),
                                  pmg,curPz,curBa,pmg->getUndoStack());
        mtcmd = new ModifyBaMtCmd(pmg,curPz,curBa,mt,mmCmd);
        //如果金额为0，则不必调整金额，否则，必须调整金额
        if(curBa->getValue() != 0){
            //如果从外币转到本币
            if(mt == account->getMasterMt())//
                v = curBa->getValue() * rates.value(curBa->getMt()->code());
            //从甲外币转到乙外币（先将甲外币转到本币，再转到乙外币）
            else if(mt != account->getMasterMt() && curBa->getMt() != account->getMasterMt())
                v = curBa->getValue() * rates.value(curBa->getMt()->code()) / rates.value(mt->code());
            else   //从本币转到外币
                v = curBa->getValue() / rates.value(mt->code());
            vcmd = new ModifyBaValueCmd(pmg,curPz,curBa,v,dir,mmCmd);
            updateCols |= BUC_VALUE;
        }
        pmg->getUndoStack()->push(mmCmd);
        break;
    case BT_JV:
        v = item->data(Qt::EditRole).value<Double>();
        vcmd = new ModifyBaValueCmd(pmg,curPz,curBa,v,MDIR_J);
        pmg->getUndoStack()->push(vcmd);
        updateCols |= BUC_VALUE;
        break;
    case BT_DV:
        v = item->data(Qt::EditRole).value<Double>();
        vcmd = new ModifyBaValueCmd(pmg,curPz,curBa,v,MDIR_D);
        pmg->getUndoStack()->push(vcmd);
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
    //LOG_INFO("enter creatNewNameItemMapping()");
    if(QMessageBox::information(0,msgTitle_info,tr("Confirm request create new second subject mapping item?\n"
                                                "first subject:%1\nsecond subject:%2").arg(fsub->getName()).arg(ni->getShortName()),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
        return;
    isInteracting = false;
    ModifyBaSndSubNMMmd* cmd = new ModifyBaSndSubNMMmd(pmg,curPz,curBa,subMgr,fsub,ni);
    pmg->getUndoStack()->push(cmd);
    ssub = cmd->getSecondSubject();
    BaUpdateColumns updateCols;
    updateCols |= BUC_SNDSUB;
    updateBas(row,1,updateCols);
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
    //LOG_INFO("enter PzDialog::creatNewSndSubject()");
    CompletSubInfoDialog* dlg = new CompletSubInfoDialog(fsub->getId(),subMgr,0);
    dlg->setName(name);
    if(QDialog::Accepted == dlg->exec()){
        ModifyBaSndSubNSMmd* cmd =
            new ModifyBaSndSubNSMmd(pmg,curPz,curBa,subMgr,fsub,
                                    dlg->getSName(),dlg->getLName(),
                                    dlg->getRemCode(),dlg->getSubCalss());
        pmg->getUndoStack()->push(cmd);
        ssub = cmd->getSecondSubject();
        BaUpdateColumns updateCols;
        updateCols |= BUC_SNDSUB;
        updateBas(row,1,updateCols);
        //有由编辑器触发数据编辑完成信号，进而触发转到下一列信号
        //ui->tview->edit(ui->tview->model()->index(row,col+1));
    }
    else
        ssub = curBa->getSecondSubject();
    //LOG_INFO("exit PzDialog::creatNewSndSubject()");
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
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
                   this,SLOT(tabRowHeightResized(int,int,int)));
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
    ui->tview->setColumnWidth(BaTableWidget::SUMMARY,states.colSummaryWidth);
    ui->tview->setColumnWidth(BaTableWidget::FSTSUB,states.colFstSubWidth);
    ui->tview->setColumnWidth(BaTableWidget::SNDSUB,states.colSndSubWidth);
    ui->tview->setColumnWidth(BaTableWidget::MONEYTYPE,states.colMtWidth);
    ui->tview->setColumnWidth(BaTableWidget::JVALUE,states.colValueWidth);
    ui->tview->setColumnWidth(BaTableWidget::DVALUE,states.colValueWidth);

    for(int i = 0; i < ui->tview->rowCount(); ++i)
        ui->tview->setRowHeight(i,states.rowHeight);
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
    ui->edtPzCount->setText(QString::number(pmg->getPzCount()));
    if(curPz == NULL)
       return;
    installInfoWatch(false);
    ui->dateEdit->setDate(curPz->getDate2());
    ui->edtPzNum->setText(QString::number(curPz->number()));
    ui->spnZbNum->setValue(curPz->zbNumber());
    ui->spnEncNum->setValue(curPz->encNumber());
    ui->edtRUser->setText(curPz->recordUser()->getName());
    ui->edtVUser->setText(curPz->verifyUser()?curPz->verifyUser()->getName():"");
    ui->edtBUser->setText(curPz->bookKeeperUser()?curPz->bookKeeperUser()->getName():"");
    ui->lblClass->setPixmap(icons_pzcls.value(curPz->getPzClass()));
    ui->lblState->setPixmap(icons_pzstate.value(curPz->getPzState()));
    refreshActions();
    installInfoWatch();
    setReadonly();
}

/**
 * @brief PzDialog::refreshActions 刷新会计分录
 */
void PzDialog::refreshActions()
{
    if(curPz == NULL){
        //validBas = -1;
        return;
    }

    //installDataWatch(false);
    //curPz->removeTailBlank();
    //validBas = curPz->baCount();
    //delegate->setVolidRows(validBas);

    disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    ui->tview->clearContents();
    ui->tview->setRowCount(MAXROWS);
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
    ui->tview->setJSum(curPz->jsum());
    ui->tview->setDSum(curPz->dsum());
    delegate->setVolidRows(curPz->baCount());

    //refreshVHeaderView();
    //installDataWatch();
    ui->tview->scrollToTop();
    //curRow = -1;
    curBa = NULL;
    connect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
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
            ui->tview->item(i,BT_SUMMARY)->setData(Qt::EditRole,curPz->getBusiAction(i)->getSummary());
            v.setValue(curPz->getBusiAction(i)->getFirstSubject());
            ui->tview->item(i,BT_FSTSUB)->setData(Qt::EditRole,v);
            v.setValue(curPz->getBusiAction(i)->getSecondSubject());
            ui->tview->item(i,BT_SNDSUB)->setData(Qt::EditRole,v);
            v.setValue(curPz->getBusiAction(i)->getMt());
            ui->tview->item(i,BT_MTYPE)->setData(Qt::EditRole,v);
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
    ui->lblStateInfo->setStyleSheet(colors.value(level));
    ui->lblStateInfo->setText(s);
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
 * @brief PzDialog::copyPreviouBa
 * 复制上一条会计分录并插入到当前行
 */
void PzDialog::copyPreviouBa()
{
    LOG_INFO("shortcut copy previou is actived!");
}

/**
 * @brief PzDialog::copyCutPaste
 * 处理拷贝、剪切和粘贴操作
 * @param witch（）
 */
void PzDialog::copyCutPaste(ClipboardOperate witch)
{
    if(witch == CO_COPY){
        QList<int> rows; bool isc;
        ui->tview->selectedRows(rows,isc);
        if(rows.empty())
            return;
        clb_Bas.clear();
        foreach(int i, rows)
            clb_Bas<<curPz->getBusiAction(i);
    }
    else if(witch == CO_CUT){

    }
    else{

    }
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
    }
    else{
        disconnect(ui->dateEdit,SIGNAL(dateChanged(QDate)),this,SLOT(pzDateChanged(QDate)));
        disconnect(ui->spnZbNum,SIGNAL(valueChanged(int)),this,SLOT(pzZbNumChanged(int)));
        disconnect(ui->spnEncNum,SIGNAL(valueChanged(int)),this,SLOT(pzEncNumChanged(int)));
        //disconnect(ui->tview,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    }
}

/**
 * @brief PzDialog::initResources 初始化资源
 */
void PzDialog::initResources()
{
    //初始化不同级别的状态信息所采用的样式表颜色
    colors[AE_OK] = "color: rgb(0, 85, 0)";
    colors[AE_WARNING] = "color: rgb(255, 0, 127)";
    colors[AE_CRITICAL] = "color: rgb(85, 0, 0)";
    colors[AE_ERROR] = "color: rgb(255, 0, 0)";

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
