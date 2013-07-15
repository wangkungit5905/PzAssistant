#include "suiteswitchpanel.h"
#include "ui_suiteswitchpanel.h"

#include "account.h"
#include "PzSet.h"


#include <QTableWidget>
#include <QPushButton>

SuiteSwitchPanel::SuiteSwitchPanel(Account *account, QWidget *parent) :
    QWidget(parent), ui(new Ui::SuiteSwitchPanel), account(account)
{
    ui->setupUi(this);
    init();
}

SuiteSwitchPanel::~SuiteSwitchPanel()
{
    delete ui;
}

void SuiteSwitchPanel::curSuiteChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if(previous)
        previous->setIcon(icon_unSelected);
    if(current)
        current->setIcon(icon_selected);
    ui->stackedWidget->setCurrentIndex(ui->lstSuite->currentRow());

}

/**
 * @brief 用户单击了切换按钮
 */
void SuiteSwitchPanel::swichBtnClicked()
{    
    int suiteId = ui->lstSuite->currentItem()->data(Qt::UserRole).toInt();
    AccountSuiteRecord* asr = suiteRecords.value(suiteId);
    AccountSuiteManager* previous = account->getPzSet(curAsrId);
    if(curAsrId != asr->id){
        curAsrId = asr->id;
        AccountSuiteManager* current  = account->getPzSet(curAsrId);
        account->setCurSuite(asr->year);
        emit selectedSuiteChanged(previous,current);
    }
}

/**
 * @brief 用户单击了查看按钮
 */
void SuiteSwitchPanel::viewBtnClicked()
{
    AccountSuiteRecord* asr;
    int month;
    witchSuiteMonth(asr,month,sender(),COL_VIEW);
    AccountSuiteManager* previous = account->getPzSet(curAsrId);
    AccountSuiteManager* current = previous;
    if(curAsrId != asr->id){
        curAsrId = asr->id;
        current = account->getPzSet(curAsrId);
        emit selectedSuiteChanged(previous,current);
    }
    //如果要以编辑模式打开，则调整打开凭证集按钮的图标和文本
    if(!asr->isClosed && current->getState(month) != Ps_Jzed){
        //在打开另一个月份的凭证集前要关闭先前打开的凭证集
        if(current->isOpened() && current->month() != month){
            if(current->isDirty())
                current->save();
            emit prepareClosePzSet(current,current->month());
            current->close();
            emit pzsetClosed(current,current->month());
        }
        if(!current->open(month)){
            QMessageBox::critical(this,tr("出错信息"),tr("在打开%1年%2月的凭证集时出错！").arg(asr->year).arg(month));
            return;
        }
        emit pzSetOpened(current,month);
        int row = month - asr->startMonth;
        QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
        QToolButton* btn = qobject_cast<QToolButton*>(tw->cellWidget(row,COL_OPEN));
        if(btn)
            setBtnIcon(btn,true);
    }
    emit viewPzSet(current,month);
//    if(monthBySuites.value(curAsrId) != month){
//        monthBySuites[curAsrId] = month;
//        emit viewPzSet(current,month);
    //    }
}

/**
 * @brief 打开/关闭凭证集
 */
void SuiteSwitchPanel::openBtnClicked(bool checked)
{
    AccountSuiteRecord* asr;
    int month;
    witchSuiteMonth(asr,month,sender(),COL_OPEN);
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    AccountSuiteManager* curSuite = account->getPzSet(asr->id);
    if(curSuite->isOpened()){
        int preOpedMonth = curSuite->month();
        if(curSuite->isDirty())
            curSuite->save();
        emit prepareClosePzSet(curSuite,preOpedMonth);
        curSuite->close();
        emit pzsetClosed(curSuite,preOpedMonth);
        int row = preOpedMonth - curSuite->getSuiteRecord()->startMonth;
        QToolButton* btn = static_cast<QToolButton*>(tw->cellWidget(row,COL_OPEN));
        if(btn)
            setBtnIcon(btn,false);
        if(preOpedMonth == month)
            return;
    }
    QToolButton* btn = qobject_cast<QToolButton*>(sender());
    if(btn){
        //bool opened = btn->arrowType()==Qt::LeftArrow;
        setBtnIcon(btn,checked);
        if(!curSuite->open(month)){
            QMessageBox::critical(this,tr("出错信息"),tr("打开%1年%2月凭证集时发生错误！").arg(asr->year).arg(month));
            return;
        }
        emit pzSetOpened(curSuite,month);
    }
}

/**
 * @brief 用户请求创建新凭证集
 */
void SuiteSwitchPanel::newPzSet()
{
    if(QMessageBox::Yes == QMessageBox::warning(this,tr("确认信息"),tr("确定要新建凭证集吗？"),QMessageBox::Yes|QMessageBox::No)){

        int month = account->getPzSet(curAsrId)->newPzSet();
        QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
        AccountSuiteRecord* asr = suiteRecords.value(curAsrId);
        int row = asr->endMonth - asr->startMonth;
        tw->insertRow(row);
        crtTableRow(row,month,tw);
    }
}

void SuiteSwitchPanel::init()
{
    curAsrId = 0;
    icon_selected = QIcon(":/images/accSuiteSelected.png");
    icon_unSelected = QIcon(":/images/accSuiteUnselected.png");
    icon_open = QIcon(":/images/pzs_open.png");
    icon_close = QIcon(":/images/pzs_close.png");
    icon_edit = QIcon(":/images/pzs_dit.png");
    QListWidgetItem* li;
    foreach(AccountSuiteRecord* as, account->getAllSuites()){
        suiteRecords[as->id] = as;
        li = new QListWidgetItem(ui->lstSuite);
        li->setData(Qt::UserRole,as->id);
        li->setIcon(icon_unSelected);
        li->setText(as->name);
        initSuiteContent(as);
    }
    connect(ui->lstSuite,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this,SLOT(curSuiteChanged(QListWidgetItem*,QListWidgetItem*)));
    connect(ui->lstSuite,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(swichBtnClicked()));
    AccountSuiteRecord* curSuite = account->getCurSuite();
    if(curSuite){
        curAsrId = curSuite->id;
        for(int i = 0; i < ui->lstSuite->count(); ++i){
            if(ui->lstSuite->item(i)->data(Qt::UserRole).toInt() == curAsrId){
                ui->lstSuite->setCurrentRow(i);
                break;
            }
        }
    }
    else
        curAsrId = 0;

}

void SuiteSwitchPanel::initSuiteContent(AccountSuiteRecord *as)
{
    QTableWidget* tw = new QTableWidget(this);
    //tw->setGridStyle(Qt::NoPen);
    tw->setShowGrid(false);
    tw->horizontalHeader()->setVisible(false);
    tw->verticalHeader()->setVisible(false);
    tw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tw->setRowCount(as->endMonth - as->startMonth + 1);
    tw->setColumnCount(3);
    tw->setColumnWidth(COL_MONTH,25);
    tw->setColumnWidth(COL_OPEN,40);
    tw->setColumnWidth(COL_VIEW,40);
    QTableWidgetItem* ti;
    for(int row = 0,m = as->startMonth; m <= as->endMonth; ++row,++m)
        crtTableRow(row,m,tw);
    if(!as->isClosed && as->endMonth < 12){
        tw->insertRow(tw->rowCount());
        QPushButton* btn = new QPushButton(tr("新键"));
        tw->setCellWidget(tw->rowCount()-1,1,btn);
        connect(btn,SIGNAL(clicked()),this,SLOT(newPzSet()));
    }
    tw->insertRow(tw->rowCount());
    tw->setSpan(tw->rowCount()-1,0,1,3);
    QPushButton* btn = new QPushButton(tr("切换到该账套"));
    tw->setCellWidget(tw->rowCount()-1,0,btn);
    connect(btn,SIGNAL(clicked()),this,SLOT(swichBtnClicked()));
    ui->stackedWidget->addWidget(tw);
}

/**
 * @brief 创建凭证集选择表的行内容
 * @param row
 */
void SuiteSwitchPanel::crtTableRow(int row, int m, QTableWidget* tw)
{
    QTableWidgetItem* ti;
    ti = new QTableWidgetItem(tr("%1月").arg(m));
    tw->setItem(row,COL_MONTH,ti);
    QToolButton* btn = new QToolButton(this);
    btn->setIcon(QIcon(":/images/open.png"));
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tw->setCellWidget(row,COL_VIEW,btn);
    connect(btn,SIGNAL(clicked()),this,SLOT(viewBtnClicked()));
    btn = new QToolButton(this);
    btn->setCheckable(true);
    setBtnIcon(btn,false);
    tw->setCellWidget(row,COL_OPEN,btn);
    connect(btn,SIGNAL(toggled(bool)),this,SLOT(openBtnClicked(bool)));
}

/**
 * @brief 当按钮单击事件发生时用于确定该按钮属于哪个帐套的哪个月份
 * @param suiteId   帐套id
 * @param month     月份
 */
void SuiteSwitchPanel::witchSuiteMonth(AccountSuiteRecord* &suiteRecord, int &month, QObject *sender, ColType col)
{

    int suiteId = ui->lstSuite->currentItem()->data(Qt::UserRole).toInt();
    suiteRecord = suiteRecords.value(suiteId);
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    month = 0;
    for(int i = 0; i < tw->rowCount(); ++i){
        if(sender == tw->cellWidget(i,col)){
            month = suiteRecord->startMonth + i;
            break;
        }
    }
}

/**
 * @brief 根据凭证集的打开帐套设置按钮的箭头类型和提示语
 * @param btn
 * @param opened
 */
void SuiteSwitchPanel::setBtnIcon(QToolButton *btn, bool opened)
{
    if(opened){
        btn->setIcon(icon_close);
        //btn->setArrowType(Qt::LeftArrow);
        btn->setToolTip(tr("关闭凭证集"));
    }
    else{
        btn->setIcon(icon_open);
        //btn->setArrowType(Qt::RightArrow);
        btn->setToolTip(tr("打开凭证集"));
    }
}
