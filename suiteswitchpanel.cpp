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
    curSuite = NULL;
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
        btn->setToolTip(jzed?btn_tip_view:btn_tip_edit);
    }
}

void SuiteSwitchPanel::curSuiteChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    AccountSuiteManager* preSuite=NULL;
    if(previous){
        int preSuiteId = previous->data(ROLE_CUR_SUITE).toInt();
        preSuite = account->getSuiteMgr(preSuiteId);
        previous->setIcon(icon_unSelected);
    }
    if(current){
        int asrId = current->data(ROLE_CUR_SUITE).toInt();
        curSuite = account->getSuiteMgr(asrId);
        current->setIcon(icon_selected);
    }
    ui->stackedWidget->setCurrentIndex(ui->lstSuite->currentRow());
    account->setCurSuite(curSuite->getSuiteRecord()->year);
    emit selectedSuiteChanged(preSuite,curSuite);
}



/**
 * @brief 用户单击了查看/编辑按钮
 */
void SuiteSwitchPanel::viewBtnClicked()
{
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    int month;
    witchSuiteMonth(month,sender(),COL_VIEW);
    int curRow = month - curSuite->getSuiteRecord()->startMonth;
    QToolButton* curBtn = qobject_cast<QToolButton*>(tw->cellWidget(curRow,COL_OPEN));
    QToolButton* btn;

    if(!curSuite->isSuiteClosed() && curSuite->getState(month) != Ps_Jzed){
        //在同一帐套内，同时只能打开一个凭证集进行编辑操作，因此打开另一个月份的凭证集前要关闭先前打开的凭证集
        if(curSuite->isPzSetOpened() && curSuite->month() != month){
            if(curSuite->isDirty())
                curSuite->save();
            emit prepareClosePzSet(curSuite,curSuite->month());
            int m = curSuite->month();
            int row = m - curSuite->getSuiteRecord()->startMonth;
            btn = qobject_cast<QToolButton*>(tw->cellWidget(row,COL_OPEN));
            curSuite->closePzSet();
            if(btn)
                setBtnIcon(btn,false);
            emit pzsetClosed(curSuite,curSuite->month());
            if(!curSuite->open(month)){
                QMessageBox::critical(this,tr("出错信息"),tr("在打开%1年%2月的凭证集时出错！").arg(curSuite->year()).arg(month));
                return;
            }
            emit pzSetOpened(curSuite,month);
        }
        else if(!curSuite->isPzSetOpened()){
            if(!curSuite->open(month)){
                QMessageBox::critical(this,tr("出错信息"),tr("在打开%1年%2月的凭证集时出错！").arg(curSuite->year()).arg(month));
                return;
            }
            emit pzSetOpened(curSuite,month);
        }
        if(curBtn){
            setBtnIcon(curBtn,true);
            curBtn->setChecked(!curBtn->isChecked());
        }
    }
    else{
        int rows = curSuite->getSuiteRecord()->endMonth - curSuite->getSuiteRecord()->startMonth + 1;
        for(int i = 0; i < rows; ++i){
            btn = qobject_cast<QToolButton*>(tw->cellWidget(i,COL_OPEN));
            if(btn){
                if(!btn->text().isEmpty())
                    btn->setText("");
            }
        }
        curBtn->setText("*");
    }
    emit viewPzSet(curSuite,month);
}

/**
 * @brief 打开/关闭凭证集
 */
void SuiteSwitchPanel::openBtnClicked(bool checked)
{
    int month;
    witchSuiteMonth(month,sender(),COL_OPEN);
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    if(curSuite->isPzSetOpened()){
        int preOpenedMonth = curSuite->month();
        //if(curSuite->isDirty())
        //    curSuite->save();
        emit prepareClosePzSet(curSuite,preOpenedMonth);
        curSuite->closePzSet();
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
        if(!btn->text().isEmpty())
            btn->setText("");
        if(checked){
            if(!curSuite->open(month)){
                QMessageBox::critical(this,tr("出错信息"),tr("打开%1年%2月凭证集时发生错误！").arg(curSuite->year()).arg(month));
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

        int month = curSuite->newPzSet();
        QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
        AccountSuiteRecord* asr = curSuite->getSuiteRecord();
        int row = asr->endMonth - asr->startMonth;
        tw->insertRow(row);
        crtTableRow(row,month,tw,false);
    }
}

void SuiteSwitchPanel::init()
{
    setMaximumWidth(240);
    btn_tip_edit = tr("编辑");
    btn_tip_view = tr("查看");
    icon_selected = QIcon(":/images/accSuiteSelected.png");
    icon_unSelected = QIcon(":/images/accSuiteUnselected.png");
    icon_open = QIcon(":/images/pzs_open.png");
    icon_close = QIcon(":/images/pzs_close.png");
    icon_edit = QIcon(":/images/pzs_edit.png");
    icon_lookup = QIcon(":/images/pzs_lookup.png");
    QListWidgetItem* li;
    foreach(AccountSuiteRecord* as, account->getAllSuiteRecords()){
        suiteRecords[as->id] = as;
        li = new QListWidgetItem(ui->lstSuite);
        li->setData(ROLE_CUR_SUITE,as->id);
        li->setIcon(icon_unSelected);
        li->setText(as->name);
        if(as->isCur)
            curSuite = account->getSuiteMgr(as->id);
        initSuiteContent(as);
    }
    connect(ui->lstSuite,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this,SLOT(curSuiteChanged(QListWidgetItem*,QListWidgetItem*)));
    if(curSuite){
        int asrId = curSuite->getSuiteRecord()->id;
        for(int i = 0; i < ui->lstSuite->count(); ++i){
            if(ui->lstSuite->item(i)->data(ROLE_CUR_SUITE).toInt() == asrId){
                ui->lstSuite->item(i)->setIcon(icon_selected);
                ui->lstSuite->setCurrentRow(i);                
                break;
            }
        }
    }
}

void SuiteSwitchPanel::initSuiteContent(AccountSuiteRecord *as)
{
    QTableWidget* tw = new QTableWidget(this);
    tw->setMaximumWidth(130);
    tw->setShowGrid(false);
    tw->horizontalHeader()->setVisible(false);
    tw->verticalHeader()->setVisible(false);
    tw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tw->setRowCount(as->endMonth - as->startMonth + 1);
    tw->setColumnCount(3);
    tw->setColumnWidth(COL_MONTH,40);
    tw->setColumnWidth(COL_OPEN,40);
    tw->setColumnWidth(COL_VIEW,40);
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
    ti->setFlags(Qt::NoItemFlags|Qt::ItemIsEnabled);
    tw->setItem(row,COL_MONTH,ti);
    QToolButton* btn = new QToolButton(this);
    btn->setIcon(viewAndEdit?icon_lookup:icon_edit);
    btn->setToolTip(viewAndEdit?btn_tip_view:btn_tip_edit);
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tw->setCellWidget(row,COL_VIEW,btn);
    connect(btn,SIGNAL(clicked()),this,SLOT(viewBtnClicked()));
    btn = new QToolButton(this);
    btn->setCheckable(true);
    setBtnIcon(btn,false);
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tw->setCellWidget(row,COL_OPEN,btn);
    connect(btn,SIGNAL(clicked(bool)),this,SLOT(openBtnClicked(bool)));
}

/**
 * @brief 当按钮单击事件发生时用于确定该按钮属于哪个帐套的哪个月份
 * @param suiteId   帐套id
 * @param month     月份
 */
void SuiteSwitchPanel::witchSuiteMonth(int &month, QObject *sender, ColType col)
{
    QTableWidget* tw = qobject_cast<QTableWidget*>(ui->stackedWidget->currentWidget());
    month = 0;
    for(int i = 0; i < tw->rowCount(); ++i){
        if(sender == tw->cellWidget(i,col)){
            month = curSuite->getSuiteRecord()->startMonth + i;
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
        btn->setToolTip(tr("关闭凭证集"));
    }
    else{
        btn->setIcon(icon_open);
        btn->setToolTip(tr("打开凭证集"));
    }
}
