#include "suiteswitchpanel.h"
#include "ui_suiteswitchpanel.h"

#include "account.h"
#include "PzSet.h"
#include "dbutil.h"


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

/**
 * @brief SuiteSwitchPanel::setJzState
 *  设置指定账套的指定月份是否结账（这通常是由于用户选择结账或反结账菜单项后引起，适时调整账套面板的按钮图标）
 *  当结账后只可历史凭证方式浏览，反结账后，可以编辑方式浏览
 * @param sm
 * @param month
 * @param jzed
 */
void SuiteSwitchPanel::setJzState(AccountSuiteManager *sm, int month, bool jzed)
{
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    int row = month - sm->getSuiteRecord()->startMonth;
    QToolButton* btn = qobject_cast<QToolButton*>(tw->cellWidget(row,COL_VIEW));
    if(btn){
        btn->setIcon(jzed?icon_lookup:icon_edit);
        btn->setToolTip(jzed?tr("查看"):tr("编辑"));
    }
}

void SuiteSwitchPanel::curSuiteChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
//    if(previous)
//        previous->setIcon(icon_unSelected);
//    if(current)
//        current->setIcon(icon_selected);
    ui->stackedWidget->setCurrentIndex(ui->lstSuite->currentRow());
    if(!current)
        return;
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    if(!tw)
        return;
    QPushButton* btn = qobject_cast<QPushButton*>(tw->cellWidget(tw->rowCount()-1,0));
    if(!btn)
        return;
    bool b = current->data(ROLE_CUR_SUITE).toBool();
    btn->setEnabled(!b);

    if(!previous)
        return;
    int row = previous->listWidget()->row(previous);
    tw = qobject_cast<QTableWidget*>(ui->stackedWidget->widget(row));
    if(!tw)
        return;
    btn = qobject_cast<QPushButton*>(tw->cellWidget(tw->rowCount()-1,0));
    if(!btn)
        return;
    b = previous->data(ROLE_CUR_SUITE).toBool();
    btn->setEnabled(!b);
}

/**
 * @brief 用户单击了切换按钮
 */
void SuiteSwitchPanel::swichBtnClicked()
{
    ui->lstSuite->currentItem()->setIcon(icon_selected);
    ui->lstSuite->currentItem()->setData(ROLE_CUR_SUITE,true);
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if(btn)
        btn->setEnabled(false);
    int suiteId = ui->lstSuite->currentItem()->data(Qt::UserRole).toInt();
    AccountSuiteRecord* asr = suiteRecords.value(suiteId);
    AccountSuiteManager* previous = account->getPzSet(curAsrId);
    if(curAsrId != asr->id){
        curAsrId = asr->id;
        AccountSuiteManager* current  = account->getPzSet(curAsrId);
        account->setCurSuite(asr->year);
        int prevAsrID = previous->getSuiteRecord()->id;
        for(int i = 0; i < ui->lstSuite->count(); ++i){
            int id = ui->lstSuite->item(i)->data(Qt::UserRole).toInt();
            if(id == prevAsrID){
                ui->lstSuite->item(i)->setIcon(icon_unSelected);
                ui->lstSuite->item(i)->setData(ROLE_CUR_SUITE,false);
                break;
            }
        }
        emit selectedSuiteChanged(previous,current);
    }
}

/**
 * @brief 用户单击了查看/编辑按钮
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
        //在同一帐套内，同时只能打开一个凭证集进行编辑操作，因此打开另一个月份的凭证集前要关闭先前打开的凭证集
        if(current == previous && current->isOpened() && current->month() != month){
            if(current->isDirty())
                current->save();
            emit prepareClosePzSet(current,current->month());
            int m = current->month();
            int row = m - asr->startMonth;
            QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
            QToolButton* btn = qobject_cast<QToolButton*>(tw->cellWidget(row,COL_OPEN));
            current->close();
            if(btn)
                setBtnIcon(btn,false);
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
        int preOpenedMonth = curSuite->month();
        if(curSuite->isDirty())
            curSuite->save();
        emit prepareClosePzSet(curSuite,preOpenedMonth);
        curSuite->close();
        emit pzsetClosed(curSuite,preOpenedMonth);
        if(preOpenedMonth != month){
            int row = preOpenedMonth - curSuite->getSuiteRecord()->startMonth;
            QToolButton* btn = static_cast<QToolButton*>(tw->cellWidget(row,COL_OPEN));
            if(btn){
                btn->setChecked(!btn->isChecked());
                setBtnIcon(btn,btn->isChecked());
            }
        }
    }
    QToolButton* btn = qobject_cast<QToolButton*>(sender());
    if(btn){
        setBtnIcon(btn,checked);
        if(checked){
            if(!curSuite->open(month)){
                QMessageBox::critical(this,tr("出错信息"),tr("打开%1年%2月凭证集时发生错误！").arg(asr->year).arg(month));
                return;
            }
            else
                emit pzSetOpened(curSuite,month);
        }        
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
        crtTableRow(row,month,tw,false);
    }
}

void SuiteSwitchPanel::init()
{
    curAsrId = 0;
    icon_selected = QIcon(":/images/accSuiteSelected.png");
    icon_unSelected = QIcon(":/images/accSuiteUnselected.png");
    icon_open = QIcon(":/images/pzs_open.png");
    icon_close = QIcon(":/images/pzs_close.png");
    icon_edit = QIcon(":/images/pzs_edit.png");
    icon_lookup = QIcon(":/images/pzs_lookup.png");
    QListWidgetItem* li;
    foreach(AccountSuiteRecord* as, account->getAllSuites()){
        suiteRecords[as->id] = as;
        li = new QListWidgetItem(ui->lstSuite);
        li->setData(Qt::UserRole,as->id);
        li->setData(ROLE_CUR_SUITE, false);
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
                ui->lstSuite->item(i)->setIcon(icon_selected);
                ui->lstSuite->item(i)->setData(ROLE_CUR_SUITE,true);
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
    tw->setColumnWidth(COL_MONTH,40);
    tw->setColumnWidth(COL_OPEN,40);
    tw->setColumnWidth(COL_VIEW,40);
    QTableWidgetItem* ti;
    for(int row = 0,m = as->startMonth; m <= as->endMonth; ++row,++m){
        bool viewAndEdit=true;
        if(!as->isClosed){
            PzsState state;
            account->getDbUtil()->getPzsState(as->year,m,state);
            if(state != Ps_Jzed)
                viewAndEdit = false;
        }
        crtTableRow(row,m,tw,viewAndEdit);
    }
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
 * @brief SuiteSwitchPanel::crtTableRow
 *  创建凭证集选择表的行内容
 * @param row           行号
 * @param m             行指代的月份
 * @param tw            表格部件
 * @param viewAndEdit   true：查看凭证集（默认）、false：编辑凭证集
 */
void SuiteSwitchPanel::crtTableRow(int row, int m, QTableWidget* tw,bool viewAndEdit)
{
    QTableWidgetItem* ti;
    ti = new QTableWidgetItem(tr("%1月").arg(m));
    tw->setItem(row,COL_MONTH,ti);
    QToolButton* btn = new QToolButton(this);
    //btn->setIcon(QIcon(":/images/open.png"));
    btn->setIcon(viewAndEdit?icon_lookup:icon_edit);
    btn->setToolTip(viewAndEdit?tr("查看"):tr("编辑"));
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tw->setCellWidget(row,COL_VIEW,btn);
    connect(btn,SIGNAL(clicked()),this,SLOT(viewBtnClicked()));
    btn = new QToolButton(this);
    btn->setCheckable(true);
    setBtnIcon(btn,false);
    tw->setCellWidget(row,COL_OPEN,btn);
    connect(btn,SIGNAL(clicked(bool)),this,SLOT(openBtnClicked(bool)));
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
