#include <QInputDialog>
#include <QKeyEvent>

#include "tables.h"
#include "common.h"
#include "global.h"
#include "pzdialog2.h"
#include "ui_pzdialog2.h"
#include "widgets.h"
#include "subject.h"
#include "dbutil.h"


////////////////////////MapLabel/////////////////////////////////////////
MapLabel::MapLabel(QWidget* parent) : QLabel(parent)
{
    curKey = 0;
}

void MapLabel::setMap(QHash<int,QString> labelHash)
{
    inerHash = labelHash;

}

void MapLabel::setValue(int key)
{
    if(inerHash.contains(key)){
        curKey = key;
        setText(inerHash.value(key));
    }
    else{
        setText("");
        curKey = 0;
    }
}

int MapLabel::getValue()
{
    return curKey;
}

///////////////////////////PzDialog2////////////////////////////////////
PzDialog2::PzDialog2(Account *account, int year, int month, CustomRelationTableModel* model,
                     bool readOnly, QWidget *parent) : QDialog(parent),ui(new Ui::PzDialog2),account(account)
{
    ui->setupUi(this);
    this->readOnly = readOnly;
    cury = year; curm = month;
    dbUtil = account->getDbUtil();
    if(!dbUtil->getPzsState(cury,curm, curPzSetState))
        curPzSetState = Ps_Rec;
    pzStateDirty = false;
    pzDirty = false;
    acDirty = false;
    initStage = true;
    isContinue =  false;
    isSameDir = false;
    cury = year; curm = month;
    this->model = model;
    model->select();
    dataMapping = new QDataWidgetMapper;
    dataMapping->setModel(model);

    QHashIterator<int,User*> it(allUsers);
    while(it.hasNext()){
        it.next();
        userNames[it.key()] = it.value()->getName();
    }
    rowHeight = 30;

    QHashIterator<PzState,QString> ip(pzStates);
    while(ip.hasNext()){
        ip.next();
        PzStates.insert(ip.key(),ip.value());
    }
    int subSys = account->getCurSuite()->subSys;
    smg = account->getSubjectManager(subSys);

    init();
    timer = new QTimer;
    timer->setInterval(autoSaveInterval);
    connect(timer,SIGNAL(timeout()),this,SLOT(autoSave()));

    //ui->twActions->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color:darkcyan;}");
}

PzDialog2::~PzDialog2()
{
    delete ui;
}

//初始化
void PzDialog2::init()
{
    //初始化银行存款、应收和应付账款的科目id
    //BusiUtil::getIdByCode(bankId,"1002");
    //BusiUtil::getIdByCode(ysId,"1131");
    //BusiUtil::getIdByCode(yfId,"2121");

    //初始化最大可用凭证号（模型的最后一行即是最大凭证号）
    maxPzNum = 1;
    maxPzZbNum = 1;    
    int num;
    for(int i = 0; i < model->rowCount(); ++i){
        num = model->data(model->index(model->rowCount()-1,PZ_NUMBER)).toInt();
        if(maxPzNum < num)
            maxPzNum = num;
        num = model->data(model->index(model->rowCount()-1,PZ_ZBNUM)).toInt();
        if(maxPzZbNum < num)
            maxPzZbNum = num;
    }
    if(maxPzNum != 1)
        maxPzNum++;
    if(maxPzZbNum != 1)
        maxPzZbNum++;

    //初始化币种列表及汇率表
    foreach(Money* mt,account->getWaiMt())
        ui->cmbMt->addItem(mt->name(),mt->code());
    if(account->getRates(cury, curm, rates))
        currentMtChanged(0); //显示第一个外币的汇率    
    rates[account->getMasterMt()->code()] = 1.00;      //不管是否成功读取汇率，人民币对人民币的汇率必须设置，否则在折算时会出错
    connect(ui->cmbMt, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentMtChanged(int)));

    //建立数据映射
    dataMapping->addMapping(ui->pzDate, PZ_DATE, "date");
    dataMapping->addMapping(ui->lblPzNum, PZ_NUMBER, "text");
    dataMapping->addMapping(ui->edtZbNum, PZ_ZBNUM);
    dataMapping->addMapping(ui->spbEnc, PZ_ENCNUM);
    dataMapping->addMapping(ui->edtJSum, PZ_JSUM);
    dataMapping->addMapping(ui->edtDSum, PZ_DSUM);
    ui->lblPzState->setMap(PzStates);
    dataMapping->addMapping(ui->lblPzState, PZ_PZSTATE, "key");
    ui->lblRUser->setMap(userNames);
    dataMapping->addMapping(ui->lblRUser, PZ_RUSER, "key");
    ui->lblBUser->setMap(userNames);
    dataMapping->addMapping(ui->lblBUser, PZ_BUSER, "key");
    ui->lblVUser->setMap(userNames);
    dataMapping->addMapping(ui->lblVUser, PZ_VUSER, "key");

    if(model->rowCount() > 0){
        dataMapping->toFirst();
        curPzId = model->data(model->index(0,0)).toInt();
        curPzClass = model->data(model->index(0,PZ_CLS )).toInt();
    }
    else{
        curPzId = 0;
        curPzClass = Pzc_Hand;
    }

    dbUtil->getActionsInPz(curPzId, busiActions);
    numActions = busiActions.count();

    //初始化业务活动表    
    ui->twActions->setColumnHidden(6,true);
    ui->twActions->setColumnHidden(7,true);
    ui->twActions->setColumnHidden(8,true);
    ui->twActions->setColumnHidden(9,true);


    //添加业务活动表的上下文菜单
    ui->twActions->addAction(ui->actAddNewAction);
    ui->twActions->addAction(ui->actCopyAction);
    ui->twActions->addAction(ui->actCutAction);
    ui->twActions->addAction(ui->actPasteAction);
    ui->twActions->addAction(ui->actDelAction);
    ui->twActions->addAction(ui->actInsertNewAction);
    ui->twActions->addAction(ui->actInsertOppoAction);
    ui->twActions->addAction(ui->actInsSelOppoAction);    
    //ui->twActions->addAction(ui->actCollaps);

    connect(ui->twActions, SIGNAL(requestContextMenu(int,int)),
            this, SLOT(refreshContextMenu(int,int)));
    connect(ui->twActions,SIGNAL(requestCrtNewOppoAction()),
            this, SLOT(requestCrtNewOppoAction()));
    connect(ui->twActions,SIGNAL(requestAppendNewAction(int)),
            this,SLOT(demandAppendNewAction(int)));
    connect(ui->twActions,SIGNAL(reqAutoCopyToCurAction()),
            this, SLOT(autoCopyToCurAction()));
    //connect(ui->twActions, SIGNAL(currentCellChanged(int,int,int,int)),
    //        this,SLOT(currentCellChanged(int,int,int,int)));
    connect(ui->twActions,SIGNAL(itemSelectionChanged()),
            this,SLOT(curItemSelectionChanged()));
    //connect(ui->twActions->verticalHeader(), SIGNAL(sectionClicked(int)),
    //        this, SLOT(rowClicked(int)));

    for(int i = 0; i < ui->twActions->rowCount(); ++i)
        ui->twActions->setRowHeight(i, rowHeight); //设置表格行高

    //设置表格的项目代理
    delegate = new ActionEditItemDelegate(smg,this);
    ui->twActions->setItemDelegate(delegate);
    connect(delegate, SIGNAL(newSndSubMapping(int,int,int,int,bool)),
            ui->twActions, SLOT(newSndSubMapping(int,int,int,int,bool)));
    connect(delegate, SIGNAL(newSndSubAndMapping(int,QString,int,int)),
            ui->twActions, SLOT(newSndSubAndMapping(int,QString,int,int)));
    connect(delegate,SIGNAL(sndSubjectDisabled(int)),
            ui->twActions,SLOT(sndSubjectDisabeld(int)));
    connect(delegate, SIGNAL(moveNextRow(int)),
            this, SLOT(demandAppendNewAction(int)));
    connect(delegate, SIGNAL(reqCopyPrevAction(int,int)),
            this, SLOT(autoCopyToCurAction2(int,int)));

    //initAction();

    installDataWatch();
    installDataWatch2();
    adjustViewMode();

    initStage = false;
    connect(this, SIGNAL(recalSum()), this, SLOT(calSums()));
}

//初始化业务活动表数据为当前凭证的业务活动（即把busiActions列表中的数据显示到表格中）
void PzDialog2::initAction()
{
    installDataWatch(false);
    curPzId = getCurPzId();
    if(busiActions.count() > 0){
        busiActions.clear();
        ui->twActions->clearContents();
        numActions = 0;
    }    

    if(curPzId != 0){
        //installDataWatch(false);
        dbUtil->getActionsInPz(curPzId, busiActions);
        //addBusiAct();
        numActions = busiActions.count();
        //refreshVHeaderView();

        QTableWidgetItem* item;
        if(curPzClass == Pzc_Hand || curPzClass == Pzc_JzsyIn || curPzClass == Pzc_JzsyFei
          || curPzClass == Pzc_Jzlr  || !isCollapseJz){
            delegate->setVolidRows(numActions);
            //int subSys = curAccount->getCurSuite()->subSys;
            //SubjectManager* smg = curAccount->getSubjectManager(subSys);
            for(int i = 0; i < busiActions.count(); ++i){
                ui->twActions->appendRow();  //添加一个有效行
                BASummaryItem* smItem = new BASummaryItem(busiActions[i]->summary, smg);
                ui->twActions->setItem(i,0,smItem);
                BAFstSubItem* fstItem = new BAFstSubItem(busiActions[i]->fid, smg);
                ui->twActions->setItem(i,1,fstItem);
                BASndSubItem* sndItem = new BASndSubItem(busiActions[i]->sid,smg);
                ui->twActions->setItem(i,2,sndItem);
                BAMoneyTypeItem* mtItem = new BAMoneyTypeItem(busiActions[i]->mt, &allMts);
                ui->twActions->setItem(i,3,mtItem);
                BAMoneyValueItem* jItem,*dItem;
                if(busiActions[i]->dir == DIR_J){
                    jItem = new BAMoneyValueItem(1, busiActions[i]->v.getv());
                    dItem = new BAMoneyValueItem(0, 0);
                }
                else{
                    jItem = new BAMoneyValueItem(1, 0);
                    dItem = new BAMoneyValueItem(0, busiActions[i]->v.getv());
                }
                ui->twActions->setItem(i,4,jItem);
                ui->twActions->setItem(i,5,dItem);

                //以下4列仅用于调式
                item = new QTableWidgetItem(QString::number(busiActions[i]->dir));
                ui->twActions->setItem(i,6,item);
                item = new QTableWidgetItem(QString::number(busiActions[i]->id));
                ui->twActions->setItem(i,7,item);
                item = new QTableWidgetItem(QString::number(busiActions[i]->pid));
                ui->twActions->setItem(i,8,item);
                item = new QTableWidgetItem(QString::number(busiActions[i]->num));
                ui->twActions->setItem(i,9,item);
            }
            if(isEnabled()){
                appendBlankAction(); //添加一个空记录，以便即时编辑
            }            
        }
//        else if(curPzClass == Pzc_Jzhd_Bank ||  //如果是结转汇兑损益的凭证
//                curPzClass == Pzc_Jzhd_Ys ||
//                curPzClass == Pzc_Jzhd_Yf){
//            //对银行存款下的汇兑损益进行合计，只需要处理一方的值，因为这些凭证都是平衡的
//            Double sums;
//            bool dir = false;

//            delegate->setVolidRows(2);
//            for(int i = 0; i < busiActions.count(); ++i){
//                if(busiActions[i]->fid == bankId ||
//                   busiActions[i]->fid == ysId ||
//                   busiActions[i]->fid == yfId ){
//                    if(busiActions[i]->dir == DIR_J)
//                        sums += busiActions[i]->v;
//                    else
//                        sums -= busiActions[i]->v;
//                }

//            }
//            //确定方向
//            if(sums > 0)
//                dir = true;
//            else
//                sums.changeSign();


//            //创建1对合计的结转汇兑损益的业务活动
//            QString summ1 = tr("结转汇兑损益"); //银行、应收或应付一方的摘要
//            QString summ2;                    //和财务费用一方的摘要
//            QString fstName;                  //银行、应收或应付一方的一级科目名

//            if(curPzClass == Pzc_Jzhd_Bank){
//                summ2 = tr("结转自银行存款的汇兑损益");
//                fstName = tr("银行存款");
//            }
//            else if(curPzClass == Pzc_Jzhd_Ys){
//                summ2 = tr("结转自应收账款的汇兑损益");
//                fstName = tr("应收账款");
//            }
//            else if(curPzClass == Pzc_Jzhd_Yf){
//                summ2 = tr("结转自应付账款的汇兑损益");
//                fstName = tr("应付账款");
//            }
//            ui->twActions->insertRow(0);
//            item = new QTableWidgetItem(summ1);
//            item->setTextAlignment(Qt::AlignCenter);
//            ui->twActions->setItem(0,0,item);
//            item = new QTableWidgetItem(fstName);
//            item->setTextAlignment(Qt::AlignCenter);
//            ui->twActions->setItem(0,1,item);
//            item = new QTableWidgetItem(tr("人民币"));
//            item->setTextAlignment(Qt::AlignCenter);
//            ui->twActions->setItem(0,3,item);
//            if(dir == DIR_J){
//                item = new BAMoneyValueItem(1,sums.getv());
//                ui->twActions->setItem(0,4,item);
//            }
//            else{
//                item = new BAMoneyValueItem(0,sums.getv());
//                ui->twActions->setItem(0,5,item);
//            }

//            ui->twActions->insertRow(1);
//            item = new QTableWidgetItem(summ2);
//            item->setTextAlignment(Qt::AlignCenter);
//            ui->twActions->setItem(1,0,item);
//            item = new QTableWidgetItem(tr("财务费用"));
//            item->setTextAlignment(Qt::AlignCenter);
//            ui->twActions->setItem(1,1,item);
//            item = new QTableWidgetItem(tr("汇兑损益"));
//            item->setTextAlignment(Qt::AlignCenter);
//            ui->twActions->setItem(1,2,item);
//            item = new QTableWidgetItem(tr("人民币"));
//            item->setTextAlignment(Qt::AlignCenter);
//            ui->twActions->setItem(1,3,item);
//            if(dir == DIR_J){
//                item = new BAMoneyValueItem(0,sums.getv());
//                ui->twActions->setItem(1,5,item);
//            }
//            else{
//                item = new BAMoneyValueItem(1,sums.getv());
//                ui->twActions->setItem(1,4,item);
//            }
//        }
        ui->twActions->scrollToTop();
        refreshVHeaderView();        
    }
    installDataWatch();
}

//调整表格各列的宽度和高度（以适应凭证编辑对话框大小的改变）
void PzDialog2::adjustTable()
{
    //处理列宽
    int totalW = ui->twActions->width();
    QList<double> wrates;
    wrates << 0.4 << 0.1 << 0.1 << 0.1 << 0.15 << 0.15;
    int sumw = 0;
    int i;
    for(i = 0; i < 6; ++i){
        ui->twActions->setColumnWidth(i, totalW * wrates[i]);
        sumw += totalW * wrates[i];
    }
    int w = totalW - sumw + 7 * 6 - 1; //假定6条列线，每条占用7个像素
    ui->twActions->setColumnWidth(0, totalW * wrates[0] - w);

    //处理垂直滚动条的出现时机
    int maxRows = ui->twActions->height() / rowHeight;
    if(maxRows > numActions)
        ui->twActions->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    else
        ui->twActions->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

//调整显示模式（是否只读或可编辑）
void PzDialog2::adjustViewMode()
{
    //综合考虑当前用户的权限、凭证集状态（这个由主窗口通过readOnly成员来控制）、凭证状态和凭证类别
    //如果当前用户没有编辑凭证的权限，则以只读方式显示
    //或当前凭证未处于录入态，或当前凭证人工创建的凭证
    bool r = false;
//    if(curPzSetState == Ps_Jzed ||
//       !curUser->haveRight(allRights.value(Right::EditPz)))
//        r = true;
//    //如果是还未审核的结转汇兑损益类凭证
//    else if(curPzSetState == Ps_Jzhd && (curPzClass == Pzc_Jzhd_Bank ||
//                                         curPzClass == Pzc_Jzhd_Yf ||
//                                         curPzClass == Pzc_Jzhd_Ys))
//        r = false;
//    //如果是还未审核的结转损益类的凭证
//    else if(curPzSetState == Ps_Jzsy && (curPzClass == Pzc_JzsyIn ||
//                                         curPzClass == Pzc_JzsyFei))
//        r = false;
//    else if(curPzSetState == Ps_Jzbnlr && curPzClass == Pzc_Jzlr)
//        r = false;
//    else{
//        r = readOnly;
//        r = r || (ui->lblPzState->getValue() != Pzs_Recording);
//        r = r || (curPzClass != Pzc_Hand);
//    }


    //为了简化控制，仅规定只有在凭证集已结账或凭证已审核或入账的情况下，不运行编辑凭证
    if(curPzSetState == Ps_Jzed)
        r = true;
    else{
        int curRow = dataMapping->currentIndex();
        int pzState = model->data(model->index(curRow, PZ_PZSTATE)).toInt();
        if(pzState == Pzs_Verify || pzState == Pzs_Instat)
            r = true;
    }

    ui->pzDate->setReadOnly(r);
    delegate->setReadOnly(r);
    ui->spbEnc->setReadOnly(r);
    ui->edtZbNum->setReadOnly(r);
    ui->edtRate->setReadOnly(r);

    if((curPzSetState == Ps_Rec) && (model->rowCount() == 0))
        ui->edtRate->setReadOnly(false);
}

//自动拷贝前一条业务活动到当前位置（主要是在编辑器未打开的情况下按下“Ctrl+=”时触发）
void PzDialog2::autoCopyToCurAction()
{
    if(isEditable()){
        int row = ui->twActions->currentRow();
        if(row > 0){
            BusiActionData2* newItem = busiActions[row-1];
            insertNewAction(row,curPzId,row+2,newItem->dir,newItem->v,newItem->mt,
                            newItem->summary,newItem->fid,newItem->sid,BusiActionData2::NEW);
            refreshVHeaderView();
            //定位光标
            ui->twActions->setCurrentCell(row,0);
            ui->twActions->edit(ui->twActions->model()->index(row,0));
            recalSum();
            acDirty = true;
            pzContentModify();
        }        
    }
}

//自动拷贝前一条业务活动到当前位置（主要是在编辑器打开的情况下按下“Ctrl+=”时触发）
void PzDialog2::autoCopyToCurAction2(int row,int col)
{
    if(isEditable() && (row > 0)){
        BusiActionData2* newItem = busiActions[row-1];
        insertNewAction(row,curPzId,row+2,newItem->dir,newItem->v,newItem->mt,
                        newItem->summary,newItem->fid,newItem->sid,BusiActionData2::NEW);
        refreshVHeaderView();
        //定位光标
        ui->twActions->setCurrentCell(row,0);
        ui->twActions->edit(ui->twActions->model()->index(row,0));
        recalSum();
        acDirty = true;
        pzContentModify();
    }
}

//视情自动添加新的业务活动
//（即在最后备用空白行的贷方列按回车键，则自动在表格最下面创建新的业务活动）
void PzDialog2::demandAppendNewAction(int row)
{
    //始终确保最后一行是备用行
    //如果是在最后一个有效行到贷方列按下回车键，则新增一个空白行
    if(row == numActions-1)
        appendBlankAction();

    //如果是在备用行到贷方列按下回车键，则新增两个空白行
    else if(row == numActions){
        appendBlankAction();
        appendBlankAction();
        ui->twActions->setCurrentCell(numActions-2,0);
        ui->twActions->edit(ui->twActions->model()->index(numActions-2,0));
    }
}

//刷新业务活动表格的上下文菜单（根据情况适当地启用或禁用菜单项）
void PzDialog2::refreshContextMenu(int row, int col)
{
    //如果当前凭证有效，则启用新增业务活动菜单项
    bool r = curPzId != 0;
    ui->actAddNewAction->setEnabled(r);

    //如果光标所在行号在当前业务活动数范围内，则启用插入业务新活动菜单项
    //如果光标所处行是一个有效的业务活动，则启用删除业务活动菜单项
    //r = row <= numActions - 1;
    r = (selRows.count() == 1);
    ui->actInsertNewAction->setEnabled(r);
    ui->actDelAction->setEnabled(r);

    r = false;
    //如果鼠标光标位于借贷金额列，且光标所处行的上面一些行的同一借贷方向上的合计值不为0，
    //则可以启用插入合计对冲业务活动菜单项
    if((row < numActions) && ((col == ActionEditItemDelegate::JV)
                              || (col == ActionEditItemDelegate::DV))){

        r = canCrtOppoSumAction(row,col,oSums,oSum,oDir,oNum);
    }
    ui->actInsertOppoAction->setEnabled(r);
    ui->actInsSelOppoAction->setEnabled(isSameDir);


    //处理拷贝、粘贴菜单项
    r = !selRows.empty();
    ui->actCopyAction->setEnabled(r);
    ui->actCutAction->setEnabled(r);
    r = r && ui->twActions->isCurRowVolid();
    ui->actPasteAction->setEnabled(r);

    //ui->actCopyAction->setEnabled(ui->twActions->hasSelectedRow());
    //ui->actCutAction->setEnabled(ui->twActions->hasSelectedRow());
    //ui->actPasteAction->setEnabled();

    if(curPzClass == Pzc_Jzhd_Bank||curPzClass==Pzc_Jzhd_Ys||curPzClass==Pzc_Jzhd_Yf){
        ui->actCollaps->setVisible(true);
        if(isCollapseJz) //如果当前是折叠，则显示展开
            ui->actCollaps->setText(tr("展开"));
        else
            ui->actCollaps->setText(tr("折叠"));
    }
    else
        ui->actCollaps->setVisible(false);

}

//测试指定行列位置是否适合插入新的合计对冲业务活动（参数row和col为当前表格位置，
//sums为各币种分开计算的合计值，sum为所有币种合计值，dir为借贷方向）
bool PzDialog2::canCrtOppoSumAction(int row, int col, QHash<int, Double> &sums,
                                    Double& sum, int& dir, int& num)
{
    if((row == 0) || (row > numActions)) //第一行和最末空白行以下行
        return false;


    sums.clear();
    sum = 0;
    num = 0;

    //从行号row开始，在busiActions列表中向上追溯同一借贷方向的业务活动，将金额按币种累计，
    //直到遇到不同的借贷方向的业务活动为止，新建的对冲业务活动数等于遇到的币种数
    int scanDir;  //扫描的借贷方向
    int scanCol;  //要扫描的列
    if(col == ActionEditItemDelegate::JV){ //新建的对冲科目处于借方则要扫描贷方的合计值
        dir = DIR_J;
        scanDir = DIR_D;
        scanCol = ActionEditItemDelegate::DV;
    }
    else{
        dir = DIR_D;
        scanDir = DIR_J;
        scanCol = ActionEditItemDelegate::JV;
    }
    int curRow = row-1;
    while(curRow > -1){
        if(scanDir == busiActions[curRow]->dir){
            sums[busiActions[curRow]->mt] += busiActions[curRow]->v;
            sum += busiActions[curRow]->v * rates.value(busiActions[curRow]->mt);
            curRow--;
            num++;
        }
        else
            break;
    }
    return sum != 0;
}

//重新根据业务活动表格内显示的顺序分配凭证内的业务活动序号
void PzDialog2::reAllocActionNumber()
{
    if(numActions == 0)
        return;
    for(int i = 0; i < numActions; ++i){
        busiActions[i]->num = i+1;
        ui->twActions->item(i,ActionEditItemDelegate::NUM)->setData(Qt::EditRole,i+1);
    }
}

//显示当前凭证集的导航状态信息（在每次移动、新增、删除凭证后必须调用）
void PzDialog2::viewCurPzInfo()
{    
    int curRow = dataMapping->currentIndex();
    curPzClass = model->data(model->index(curRow,PZ_CLS)).toInt();
    ui->lblPzCount->setText(QString::number(model->rowCount()));
    ui->lblIdx->setText(QString::number(curRow + 1));
    initAction();
    disconnect(ui->edtJSum,SIGNAL(textChanged(QString)),this,SLOT(jdSumChanged()));
    disconnect(ui->edtDSum,SIGNAL(textChanged(QString)),this,SLOT(jdSumChanged()));
    calSums(); //显示借贷合计值
    connect(ui->edtJSum,SIGNAL(textChanged(QString)),this,SLOT(jdSumChanged()));
    connect(ui->edtDSum,SIGNAL(textChanged(QString)),this,SLOT(jdSumChanged()));
    adjustViewMode();

    int pzState = model->data(model->index(curRow, PZ_PZSTATE)).toInt();
    emit pzStateChanged(pzState);
    int pzNum = ui->lblPzNum->text().toInt();
    emit curPzNumChanged(pzNum);
    emit curIndexChanged(curRow+1,model->rowCount());


    //还要将当前凭证的其他信息传递给主窗口（通过信号触发）
}

//刷新业务活动表格的垂直标题条
void PzDialog2::refreshVHeaderView()
{
    //应该区分凭证类别
    QString title;
    vheadLst.clear();
    if(((curPzClass == Pzc_Jzhd_Bank) ||
        (curPzClass == Pzc_Jzhd_Ys) ||
        (curPzClass == Pzc_Jzhd_Yf))  && !isCollapseJz){
        for(int i = 1; i < 3; ++i)
            vheadLst << QString::number(i);
    }
    else{
        for(int i = 0; i < numActions-1; ++i){
            switch(busiActions[i]->state){
            case BusiActionData::BLANK:
                title = QString::number(i+1).append('?');
                break;
            case BusiActionData::EDITED:
                title = QString::number(i+1).append('*');
                break;
            case BusiActionData::NEW:
                title = QString::number(i+1).append('+');
                break;
            case BusiActionData::NUMCHANGED:
                 title = QString::number(i+1).append('.');
                 break;
            default:
                title = QString::number(i+1);
            }
            vheadLst << title;
        }
        for(int i = numActions; i < ui->twActions->rowCount(); ++i)
            vheadLst << "";
    }
    ui->twActions->setVerticalHeaderLabels(vheadLst);
}


//安装/卸载业务活动表格数据改变监视器
void PzDialog2::installDataWatch(bool install)
{
    if(install)
        connect(ui->twActions, SIGNAL(itemChanged(QTableWidgetItem*)),
                this, SLOT(actionDataItemChanged(QTableWidgetItem*)));
    else
        disconnect(ui->twActions, SIGNAL(itemChanged(QTableWidgetItem*)),
                   this, SLOT(actionDataItemChanged(QTableWidgetItem*)));
}

//安装/卸载凭证其他内容的改变监视器（监视凭证日期、自编号、附件数等）
void PzDialog2::installDataWatch2(bool install)
{
    if(install){
        connect(ui->spbEnc, SIGNAL(valueChanged(int)), this, SLOT(encChanged()));
        connect(ui->pzDate, SIGNAL(dateChanged(QDate)), this, SLOT(pzDateChanged()));
        connect(ui->edtZbNum,SIGNAL(textEdited(QString)), this, SLOT(zbNumChanged()));
        connect(ui->edtJSum,SIGNAL(textChanged(QString)),this,SLOT(jdSumChanged()));
        connect(ui->edtDSum,SIGNAL(textChanged(QString)),this,SLOT(jdSumChanged()));
    }
    else{
        disconnect(ui->spbEnc, SIGNAL(valueChanged(int)), this, SLOT(encChanged()));
        disconnect(ui->pzDate, SIGNAL(dateChanged(QDate)), this, SLOT(pzDateChanged()));
        disconnect(ui->edtZbNum,SIGNAL(textEdited(QString)), this, SLOT(zbNumChanged()));
        disconnect(ui->edtJSum,SIGNAL(textChanged(QString)),this,SLOT(jdSumChanged()));
        disconnect(ui->edtDSum,SIGNAL(textChanged(QString)),this,SLOT(jdSumChanged()));
    }
}

//设置凭证集的日期范围
void PzDialog2::setDateRange(QDate start, QDate end)
{
    ui->pzDate->setDateRange(start, end);
}

//主要是为了在刚打开对话框时能够使主窗口得到当前凭证的信息
void PzDialog2::show()
{
    if(curPzId != 0){
        //显示凭证总数和序号
        viewCurPzInfo();
        //int idx = dataMapping->currentIndex();
        //int state = model->data(model->index(idx,PZ_PZSTATE)).toInt();
        //emit pzStateChanged(state);
    }
    QDialog::show();
}


//显示指定币种的汇率
void PzDialog2::currentMtChanged(int index)
{
    int mtCode = ui->cmbMt->itemData(index).toInt();
    //移除汇率编辑框的文本改变事件的信号连接
    ui->edtRate->setText(rates.value(mtCode).toString());
    //重新建立汇率编辑框的文本改变事件的信号连接
}




//返回当前凭证的id
int PzDialog2::getCurPzId(){
    int idx = dataMapping->currentIndex();
    return model->data(model->index(idx,0)).toInt();
}

//是否只读方式打开（这个函数由主窗口根据打开的凭证集的状态来调用）
//窗口的只读模式还由凭证的状态来决定，这些控制主要通过adjustViewMode()函数来完成
void PzDialog2::setReadOnly(bool readOnly)
{
    this->readOnly = readOnly;
    delegate->setReadOnly(readOnly);
}

//获取当前凭证的状态
int PzDialog2::getPzState()
{
    int idx = dataMapping->currentIndex();
    if(idx > -1)
        return model->data(model->index(idx, PZ_PZSTATE)).toInt();
    else
        return -1;
}

//设置当前凭证的状态
void PzDialog2::setPzState(int scode, User* user)
{
    int idx = dataMapping->currentIndex();
    if(idx > -1){
        int userId = user->getUserId();
        int oldS = model->data(model->index(idx,PZ_PZSTATE)).toInt();
        model->setData(model->index(idx, PZ_PZSTATE), scode);
        switch(scode){
        case Pzs_Verify://如果变为审核态，则设置审核者，并清除入账者
            model->setData(model->index(idx,PZ_VUSER),userId);
            model->setData(model->index(idx,PZ_BUSER),0);
            break;
        case Pzs_Instat:  //凭证入账，则仅仅记录入账者
            model->setData(model->index(idx,PZ_BUSER),userId);
            break;
        case Pzs_Recording: //凭证回到初始的录入态，则清除其他用户标志
        case Pzs_Repeal:  //如果作废，则仅仅保留录入者
            model->setData(model->index(idx,PZ_VUSER),0);
            model->setData(model->index(idx,PZ_BUSER),0);
            break;
        }
        pzDirty = true;
        //如果凭证状态在录入态和审核态之间转变或者直接从入账态到录入态（这发生在取消审核时）
        if(((oldS == Pzs_Recording) && (scode == Pzs_Verify)) ||
           ((oldS == Pzs_Verify) && (scode == Pzs_Recording)) ||
           ((oldS == Pzs_Instat) && (scode == Pzs_Recording)))
            pzStateDirty = true;
        ui->lblPzState->setValue(scode);
        adjustViewMode();
    }
}

//返回当前凭证是否被修改了
bool PzDialog2::isDirty()
{
    return pzStateDirty || pzDirty || acDirty;
}

//重新分配凭证号
void PzDialog2::reAssignPzNum()
{
    dbUtil->assignPzNum(cury,curm);
    installDataWatch2(false);
    model->select();
    dataMapping->setCurrentIndex(0);
    installDataWatch2();
    viewCurPzInfo();
}


//刷新显示当前凭证
//void PzDialog2::refreshInfo()
//{
//    int curIdx = dataMapping->currentIndex();
//    model->select();
//    if(curIdx < model->rowCount())
//        dataMapping->setCurrentIndex(curIdx);
//}

/////////////////////////槽函数部分//////////////////////////////////
//处理凭证记录的导航按钮的槽
void PzDialog2::naviFst()
{
    save(false);
    installDataWatch2(false);
    dataMapping->toFirst();
    installDataWatch2();
    viewCurPzInfo();
    //ui->lblIdx->setText(QString::number(dataMapping->currentIndex() + 1));
    //int pzNum = ui->lblPzNum->text().toInt();
    //curPzId = model->data(model->index(0,0)).toInt();
    //initAction();
    //emit curPzNumChanged(pzNum);
}

void PzDialog2::naviNext()
{
    save(false);
    installDataWatch2(false);
    dataMapping->toNext();
    installDataWatch2();
    viewCurPzInfo();
//    ui->lblIdx->setText(QString::number(dataMapping->currentIndex() + 1));
//    int pzNum = ui->lblPzNum->text().toInt();
//    initAction();
//    emit curPzNumChanged(pzNum);
}
void PzDialog2::naviPrev()
{
    save(false);
    installDataWatch2(false);
    dataMapping->toPrevious();
    installDataWatch2();
    viewCurPzInfo();
//    ui->lblIdx->setText(QString::number(dataMapping->currentIndex() + 1));
//    int pzNum = ui->lblPzNum->text().toInt();
//    initAction();
//    emit curPzNumChanged(pzNum);
}
void PzDialog2::naviLast()
{
    save(false);
    installDataWatch2(false);
    dataMapping->toLast();
    installDataWatch2();
    viewCurPzInfo();
//    ui->lblIdx->setText(QString::number(dataMapping->currentIndex() + 1));
//    int pzNum = ui->lblPzNum->text().toInt();
//    initAction();
//    emit curPzNumChanged(pzNum);
}

//转到指定id的凭证
void PzDialog2::naviTo(int pid, int bid)
{
    save(false);
    int row = 0;
    int fid = 0;
    while((row < model->rowCount()) && (fid != pid)){
        fid = model->data(model->index(row, 0)).toInt();
        row++;
    }
    installDataWatch2(false);
    dataMapping->setCurrentIndex(row-1);
    installDataWatch2();
    viewCurPzInfo();

    //选择指定的会计分录....
    if(bid == 0)
            return;
    row = -1;
    for(int i = 0; i < busiActions.count(); ++i){
        if(busiActions[i]->id == bid){
            row = i;
            break;
        }
    }
    if(row != -1)
        ui->twActions->selectRow(row);
}

//转到指定号码的凭证
bool PzDialog2::naviTo2(int num)
{
    save(false);
    //定位dataMapping
    bool r = false;
    int pzNum = 0, i = 0;
    while(!r && (i < model->rowCount())){
        pzNum = model->data(model->index(i, PZ_NUMBER)).toInt();
        if(num == pzNum){
            installDataWatch2(false);
            dataMapping->setCurrentIndex(i);
            installDataWatch2();
            r = true;
        }
        i++;
    }
    if(r)
        viewCurPzInfo();
    return r;
}

//添加凭证
void PzDialog2::addPz()
{
    bool ok;
    save(false);
    int row = model->rowCount();
    int idx = dataMapping->currentIndex();
    ok = model->insertRow(row);

    //默认的凭证日期应该是当前显示的凭证日期同等的日期
    //如果当前凭证集为空，则选择会计期间中的最前的一个日期
    QString date = ui->pzDate->date().toString(Qt::ISODate);
    if(row == 0){
        date.chop(2);
        date.append("01"); //设置时间为某月的第一天
    }

    model->setData(model->index(row, PZ_DATE), date);          //凭证日期
    model->setData(model->index(row, PZ_NUMBER), maxPzNum++);  //设置凭证号
    model->setData(model->index(row, PZ_ZBNUM), maxPzZbNum++);            //设置凭证自编号
    model->setData(model->index(row, PZ_JSUM), 0);             //借方合计值归零
    model->setData(model->index(row, PZ_DSUM), 0);             //贷方合计值归零
    model->setData(model->index(row, PZ_CLS), 0);              //这是一条人工新建的凭证
    model->setData(model->index(row, PZ_ENCNUM), 0);           //默认附件数为0
    model->setData(model->index(row, PZ_PZSTATE), Pzs_Recording); //凭证状态为初始录入态
    model->setData(model->index(row, PZ_RUSER), curUser->getUserId()); //录入凭证的用户
    model->setData(model->index(row, PZ_VUSER), 0);            //审核凭证的用户
    model->setData(model->index(row, PZ_BUSER), 0);            //凭证记账的用户
    model->submit();
    model->select();

    //定位到新增加的凭证
    dataMapping->setCurrentIndex(row);
    viewCurPzInfo();
}

//插入凭证
void PzDialog2::insertPz()
{
    //插入空凭证
    int idx = dataMapping->currentIndex();
    int pzNum = model->data(model->index(idx,PZ_NUMBER)).toInt();
    int pzZbNum = model->data(model->index(idx,PZ_ZBNUM)).toInt();
    QString pzDate = model->data(model->index(idx,PZ_DATE)).toString();

    //修改其后凭证的号码,因为当前凭证集的排列顺序可能是按照凭证号，因此，必须扫描整个凭证集
    for(int i = 0; i < model->rowCount(); ++i){
        int num = model->data(model->index(i,PZ_ZBNUM)).toInt();    //自编号
        int num2 = model->data(model->index(i,PZ_NUMBER)).toInt();  //凭证号
        if(num <= pzZbNum)
            continue;
        else{
            model->setData(model->index(i,PZ_ZBNUM),num+1);
            model->setData(model->index(i,PZ_NUMBER),num2+1);
        }
    }
    dataMapping->submit();

    //直接在模型中插入行，总插入的末尾，不知为何？
    PzData* pz = new PzData;
    pz->date = pzDate;
    pz->pzNum = pzNum+1;
    pz->pzZbNum = pzZbNum+1;
    pz->jsum = 0;
    pz->dsum = 0;
    pz->pzClass = Pzc_Hand;
    pz->attNums = 0;
    pz->state = Pzs_Recording;
    pz->producer = curUser;
    pz->bookKeeper = NULL;
    pz->verify = NULL;
    dbUtil->crtNewPz(pz);
    model->select();
    dataMapping->setModel(model);
    dataMapping->setCurrentIndex(idx+1);
    viewCurPzInfo();
}

//删除凭证
void PzDialog2::delPz()
{
    //如果删除的凭证的总号或分号是当前使用的最大值，则必须更新maxPzNum和maxPzSubNum
    if(QMessageBox::Yes == QMessageBox::question(0, tr("确认消息"), tr("确定要删除该凭证及其相关的业务活动吗？"),
                                                 QMessageBox::Yes | QMessageBox::No)){
        //save(false);
        //删除相关的业务活动
        dbUtil->delActionsInPz(curPzId);
        int index = dataMapping->currentIndex();
        //为保持凭证号的连贯性，需调整后续凭证的号码
        if(index < model->rowCount()-1)
            for(int i = index+1; i < model->rowCount(); ++i)
                model->setData(model->index(i,PZ_NUMBER),i+1);

        model->removeRow(index);
        dataMapping->submit();
        model->select();
        //定位到下一个有效的凭证
        if(index == model->rowCount())  //删除的是最后一张凭证
            dataMapping->setCurrentIndex(index - 1);
        else
            dataMapping->setCurrentIndex(index);
        viewCurPzInfo();
        maxPzNum--;
    }    
}

//添加空业务活动
void PzDialog2::addBusiAct()
{
    appendNewAction(curPzId,numActions+1);

    //打开摘要列编辑器
    ui->twActions->setCurrentCell(numActions-2,0);
    ui->twActions->edit(ui->twActions->model()->index(numActions-2,0));
    //ui->twActions->editItem((BASummaryItem*)ui->twActions->item(numActions-1,0));

}

//删除业务活动
void PzDialog2::delBusiAct()
{
    if(selRows.count() == 0)
        return;
    if((selRows.count() > 0) && selRows[0] == numActions-1)
        return;

    if(QMessageBox::Yes == QMessageBox::question(this, tr("确认消息"),
                    tr("确实要删除此业务活动吗？"),QMessageBox::Yes|QMessageBox::No)){
        int rowNum = selRows.count();
        QList<int> rows(selRows);   //因为在删除最后的选择行后，会自动清空选择行列表（不知为何？）从而造成后续删除动作的崩溃
        bool blankRow = false; //选中的行中是否有末尾的空白行
        for(int i = rowNum - 1; i >= 0 ; i--){   //从下往上删，才能正确执行
            if(rows[i] == (numActions - 1)){  //最后一个空白行不能删除
                blankRow = true;
                continue;
            }
            //如果业务活动是新增的或空白的则直接删除之
            if((busiActions[rows[i]]->state == BusiActionData2::NEW) ||
                busiActions[rows[i]]->state == BusiActionData2::BLANK){
                busiActions.removeAt(rows[i]);
            }
            else{
                busiActions[rows[i]]->state = BusiActionData2::DELETED;
                delActions.append(busiActions.takeAt(rows[i]));
                acDirty = true;
                //emit pzContentChanged();
                pzContentModify();
            }
            ui->twActions->removeRow(rows[i]);//此语句必须位于后面，否则会崩溃
        }
        if(blankRow)
            numActions -= (rowNum-1);
        else
            numActions -= rowNum;
        delegate->setVolidRows(numActions);
        recalSum();
        refreshVHeaderView();
    }    
}

//向上移动业务活动
void PzDialog2::moveUp()
{
    //int row = ui->twActions->currentRow();
    //int col = ui->twActions->currentColumn();
    if(selRows.count() != 1)
        return;
    int row = selRows[0];
    if(row > 0){
        installDataWatch(false);
        ui->twActions->switchRow(row,row - 1);
        installDataWatch();
        //ui->twActions->setCurrentCell(row - 1, col);
        ui->twActions->selectRow(row - 1);
        selRows.clear();
        selRows.append(row - 1);
        emit selectedBaAction(true);
        busiActions.swap(row,row - 1);
        if(busiActions[row]->state == BusiActionData2::INIT)
            busiActions[row]->state = BusiActionData2::NUMCHANGED;
        if(busiActions[row - 1]->state == BusiActionData2::INIT)
            busiActions[row - 1]->state = BusiActionData2::NUMCHANGED;
        refreshVHeaderView();
        acDirty = true;
        pzContentModify();
    }
}

//向下移动业务活动
void PzDialog2::moveDown()
{
    //int row = ui->twActions->currentRow();
    //int col = ui->twActions->currentColumn();
    if(selRows.count() != 1)
        return;
    int row = selRows[0];
    if(row < numActions-1){
        installDataWatch(false);
        ui->twActions->switchRow(row,row + 1);
        installDataWatch();
        //ui->twActions->setCurrentCell(row + 1, col);
        ui->twActions->selectRow(row + 1);
        selRows.clear();
        selRows.append(row + 1);
        emit selectedBaAction(true);
        busiActions.swap(row,row + 1);
        if(busiActions[row]->state == BusiActionData2::INIT)
            busiActions[row]->state = BusiActionData2::NUMCHANGED;
        if(busiActions[row + 1]->state == BusiActionData2::INIT)
            busiActions[row - 1]->state = BusiActionData2::NUMCHANGED;
        refreshVHeaderView();
        acDirty = true;
        pzContentModify();
    }
}

//是否可以向上移动业务活动
bool PzDialog2::canMoveBaUp()
{
    if(selRows.count() == 0)
        return false;
    if((selRows.count() != 1) || ((selRows.count() == 1) && (selRows[0] == 0)))
        return false;
    else
        return true;

}

//是否可以向下移动业务活动
bool PzDialog2::canMoveBaDown()
{
    if(selRows.count() == 0)
        return false;
    if((selRows.count() != 1) ||
            ((selRows.count() == 1) && (selRows[0] == (numActions-2))))
        return false;
    else
        return true;
}

//保存对凭证的修改（参数isForm：保存前是否需要确认，默认为true，无须确认）
void PzDialog2::save(bool isForm)
{
    if(curPzId == 0)
        return;
    if(pzStateDirty || pzDirty || acDirty){
        if((isForm) || (QMessageBox::Yes == QMessageBox::question(this, tr("询问信息"),
           tr("凭证已改变，保存吗？"), QMessageBox::Yes|QMessageBox::No))){

            if(pzStateDirty || pzDirty){ //保存凭证一般数据
                int idx = dataMapping->currentIndex();
                dataMapping->submit();
                dataMapping->setCurrentIndex(idx);
                pzDirty = false;
            }
            //凭证集状态的改变在主窗口内检测并显示，这里可以不用考虑
            //在主窗口的保存命令中，会检测凭证集的状态，并显示。
            if(pzStateDirty){  //如果凭证状态发生了改变，则还要刷新凭证集的状态
//                PzsState oldState,newState;
//                BusiUtil::getPzsState(cury,curm,oldState);
//                BusiUtil::getPzsState(cury,curm,newState);
//                if(oldState != newState)
//                    emit pzsStateChanged();
                pzStateDirty = false;
            }
            if(acDirty){  //保存凭证业务活动数据
                //剔除并保存空白业务活动
                QHash<int,BusiActionData2*> blanks;
                for(int i = busiActions.count()-1; i > -1 ; i--){
                    if(busiActions[i]->state == BusiActionData::BLANK){
                        blanks[i] = busiActions.takeAt(i);
                        //delete busiActions[i];
                        //busiActions.removeAt(i);
                    }
                }
                dbUtil->saveActionsInPz(curPzId,busiActions,delActions);
                //恢复先前存在到空白业务活动
                QList<int> bs = blanks.keys();
                qSort(bs.begin(),bs.end());
                for(int i = 0; i < bs.count(); ++i)
                    busiActions.insert(bs[i],blanks.value(bs[i]));
                //BusiActionData* ba = new BusiActionData;
                //ba->state = BusiActionData::BLANK;
                //busiActions.append(ba);
                //numActions = busiActions.count();

                if(delActions.count() > 0){
                    for(int i = 0; i < delActions.count(); ++i)
                        delete delActions[i];
                    delActions.clear();
                }
                acDirty = false;
                refreshVHeaderView();
            }
        }
        else{                     //取消保存
            pzDirty = false;
            acDirty = false;
            pzStateDirty = false;
            if(!delActions.empty())
                delActions.clear();
            //虽然没有提交到数据库，但模型内的保存的是已经修改的内容，必须重新从数据库中获取原始数据
            int idx = dataMapping->currentIndex();
            installDataWatch2(false);            
            model->select();
            dataMapping->setCurrentIndex(idx);
            installDataWatch2();
            emit pzContentChanged(false); //还要向主窗口报告使保存按钮无效
        }
    }    
}

//关闭凭证窗口时要调用的槽
void PzDialog2::closeDlg()
{
    save(false);
    close();
}

//请求创建新的合计对冲业务活动
void PzDialog2::requestCrtNewOppoAction()
{
    int curRow = ui->twActions->currentRow();
    int curCol = ui->twActions->currentColumn();
    if(canCrtOppoSumAction(curRow,curCol,oSums,oSum,oDir,oNum)){
        QInputDialog* dlg = new QInputDialog(this);
        QStringList items;
        items << tr("混合币种") << tr("按币种分开合计");
        dlg->setComboBoxItems(items);
        if(dlg->exec() == QDialog::Accepted){
            QString sel = dlg->textValue();
            if(sel == tr("混合币种"))
                isByMt = false;
            else
                isByMt = true;
            crtNewSumOppoAction(curRow);
        }
    }
}

//
void PzDialog2::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    adjustTable();
}

//当用户改变了汇率后
void PzDialog2::on_edtRate_returnPressed()
{
    Double v = Double(ui->edtRate->text().toDouble());
    int idx = ui->cmbMt->currentIndex();
    int mtCode = ui->cmbMt->itemData(idx).toInt(); //当前选择的外币代码
    if(v != rates.value(mtCode)){
        if(QMessageBox::question(this, tr("确认信息"), tr("同意修改汇率吗？"),
                                 QMessageBox::Yes|QMessageBox::No)
                == QMessageBox::Yes){
            rates[mtCode] = v;
            //将新的汇率值保存到数据库中
            curAccount->setRates(cury, curm, rates);
        }

    }
}

void PzDialog2::keyPressEvent(QKeyEvent * event)
{
//    QKeySequence::Copy	9	Copy.
//    QKeySequence::Cut	8	Cut.
//    QKeySequence::Delete	7	Delete.
//    QKeySequence::Paste	10	Paste.
//    QKeySequence::New	6
//    QKeySequence::Print	18	Print document.
//    QKeySequence::Redo	12	Redo.
//    QKeySequence::Undo	11	Undo.

//    QKeySequence::Save	5
    if(event->matches(QKeySequence::Save)){
        save();
        emit saveCompleted();
    }
    if(event->matches(QKeySequence::Copy)){
        int i = 0;
    }
    if(event->matches(QKeySequence::Cut)){
        int i = 0;
    }
    if(event->matches(QKeySequence::Paste)){
        int i = 0;
    }


}

//当业务活动表格中的某个项目的数据发生改变时
void PzDialog2::actionDataItemChanged(QTableWidgetItem *item)
{
    //用这个信号来捕获表格项数据改变事件，有一个缺陷，即在仅仅打开编辑器，
    //而在没有实际编辑数据后关闭编辑器后也会触发此信号，因此还必须检测是否数据实际发生了改变
    int row = item->row();
    //if(row >= numActions)
    //    return;
    if(row >= busiActions.count())
        return;
    int col = item->column();
    bool b = false;

    if((col == ActionEditItemDelegate::SUMMARY)
            && (item->data(Qt::EditRole).toString() != busiActions[row]->summary)){
        busiActions[row]->summary = item->data(Qt::EditRole).toString();
        b = true;
    }
    else if(((col == ActionEditItemDelegate::FSTSUB)
             && (item->data(Qt::EditRole).toInt() != busiActions[row]->fid))){
        //QHash<int,QString> subs;  //
        int fid = item->data(Qt::EditRole).toInt();
        FirstSubject* fsub = smg->getFstSubject(fid);
        int sid = ui->twActions->item(row,col+1)->data(Qt::EditRole).toInt();
        SecondSubject* ssub = smg->getSndSubject(sid);
        //BusiUtil::getOwnerSub(fid,subs);

        //如果一级科目是现金，则默认设置二级科目为人民币，币种也为人民币，金额将自动转换
        //int cid; //现金科目的id
        //BusiUtil::getIdByCode(cid,"1001");
        if(fid == smg->getCashSub()->getId()){
            ui->twActions->item(row,ActionEditItemDelegate::SNDSUB)->setData(Qt::EditRole,fsub->getDefaultSubject()->getId());
            ui->twActions->item(row,ActionEditItemDelegate::MTYPE)->setData(Qt::EditRole,account->getMasterMt()->code());
            busiActions[row]->sid = fsub->getDefaultSubject()->getId();
            busiActions[row]->mt = account->getMasterMt()->code();
        }

        //如果当前的一级科目没有包含当前的二级科目则设置二级科目为默认值
        else if(!fsub->containChildSub(ssub)){
            installDataWatch(false);
            sid = smg->getFstSubject(fid)->getDefaultSubject()->getId();
            ui->twActions->item(row,col+1)->setData(Qt::EditRole, sid);
            installDataWatch();
            busiActions[row]->sid = sid;
        }
        busiActions[row]->fid = fid;
        b = true;
        emit mustRestat();
    }
    else if((col == ActionEditItemDelegate::SNDSUB)
            && (item->data(Qt::EditRole).toInt() != busiActions[row]->sid)){

        int sid = item->data(Qt::EditRole).toInt();
        //int cid; //现金科目id
        //int bid;//银行存款科目的id
        //BusiUtil::getIdByCode(cid,"1001");
        //BusiUtil::getIdByCode(bid,"1002");
        int fid = ui->twActions->item(row,ActionEditItemDelegate::FSTSUB)->data(Qt::EditRole).toInt();

        //如果一级科目是现金，则设置与二级科目对应的币种
        if(fid == smg->getCashSub()->getId()){
//            QString mName = allSndSubs.value(sid);
//            QHashIterator<int,QString> it(allMts);
//            int mt;
//            while(it.hasNext()){
//                it.next();
//                if(mName == it.value()){
//                    mt = it.key();
//                    break;
//                }
//            }
            ui->twActions->item(row, ActionEditItemDelegate::MTYPE)->setData(Qt::EditRole, account->getMasterMt()->code());
            busiActions[row]->mt = account->getMasterMt()->code();
        }

        //如果一级科目为银行存款，则寻找二级科目中的币种后缀，并根据二级科目中的币种自动设置相对应的币种
        else if(fid == smg->getBankSub()->getId()){
            QString sname = smg->getSndSubject(sid)->getName();
            int idx = sname.indexOf("-");
            QString mtName = sname.right(sname.length()-idx-1);
            QHashIterator<int,Money*> it(account->getAllMoneys());
            int mt;
            while(it.hasNext()){
                it.next();
                if(mtName == it.value()->name()){
                    mt = it.key();
                    break;
                }
            }
            ui->twActions->item(row, ActionEditItemDelegate::MTYPE)->setData(Qt::EditRole, mt);
            busiActions[row]->mt = mt;
        }
        busiActions[row]->sid = sid;
        b = true;
        emit mustRestat();
    }
    else if((col == ActionEditItemDelegate::MTYPE)
              && (item->data(Qt::EditRole).toInt() != busiActions[row]->mt)){
        int mt = item->data(Qt::EditRole).toInt();
        //在币种改变时，要自动调整金额
        installDataWatch(false);
        if(busiActions[row]->mt == RMB){
            busiActions[row]->v /= rates.value(mt);
            if(busiActions[row]->dir == DIR_J){
                ui->twActions->item(row,ActionEditItemDelegate::JV)->setData(Qt::EditRole,busiActions[row]->v.getv());
            }
            else{
                ui->twActions->item(row,ActionEditItemDelegate::DV)->setData(Qt::EditRole,busiActions[row]->v.getv());
            }
        }
        else{
            busiActions[row]->v *= rates.value(busiActions[row]->mt);
            if(busiActions[row]->dir == DIR_J){
                ui->twActions->item(row,ActionEditItemDelegate::JV)->setData(Qt::EditRole,busiActions[row]->v.getv());
            }
            else{
                ui->twActions->item(row,ActionEditItemDelegate::DV)->setData(Qt::EditRole,busiActions[row]->v.getv());
            }
        }
        installDataWatch();
        busiActions[row]->mt = mt;
        b = true;
        emit mustRestat();
    }
    else if(col == ActionEditItemDelegate::JV){
        if((busiActions[row]->dir == DIR_J)
            && (Double(item->data(Qt::EditRole).toDouble()) != busiActions[row]->v)){
            busiActions[row]->v = Double(item->data(Qt::EditRole).toDouble());

        }
        //在金额的方向改变时，自动复位相对方向的金额
        else if((busiActions[row]->dir == DIR_D)
                && Double(item->data(Qt::EditRole).toDouble()) != 0){
            busiActions[row]->v = Double(item->data(Qt::EditRole).toDouble());
            busiActions[row]->dir = DIR_J;
            installDataWatch(false);
            ui->twActions->item(row,ActionEditItemDelegate::DV)->setData(Qt::EditRole,0);
            installDataWatch();

        }
        b = true;
        calSums();
        emit mustRestat();
    }
    else if(col == ActionEditItemDelegate::DV){
        if((busiActions[row]->dir == DIR_D)
            && Double(item->data(Qt::EditRole).toDouble()) != busiActions[row]->v){
            busiActions[row]->v = Double(item->data(Qt::EditRole).toDouble());
        }
        else if((busiActions[row]->dir == DIR_J)
                && Double(item->data(Qt::EditRole).toDouble()) != 0){
            busiActions[row]->v = Double(item->data(Qt::EditRole).toDouble());
            busiActions[row]->dir = DIR_D;
            installDataWatch(false);
            ui->twActions->item(row,ActionEditItemDelegate::JV)->setData(Qt::EditRole,0);
            installDataWatch();
            //b = true;
        }
        b = true;
        calSums();
        emit mustRestat();
    }

    if(b){
        acDirty = true;        
        if(busiActions[row]->state == BusiActionData2::BLANK)
            busiActions[row]->state = BusiActionData2::NEW;
        else if(busiActions[row]->state != BusiActionData2::NEW){
            busiActions[row]->state = BusiActionData2::EDITED;
        }
        refreshVHeaderView();
        pzContentModify();
        //emit pzContentChanged(true);
    }


 }

//创建新的合计对冲业务活动（参数dir：新业务活动的借贷方向（true：借），row：开始搜索的行号）
void PzDialog2::crtNewSumOppoAction(int row)
{
    //如果与冲业务活动相对的业务活动数只有一条，则自动拷贝此业务活动的摘要内容
    QString summ;
    if(oNum == 1)
        summ = busiActions[row]->summary;

    //按币种分别建立合计对冲业务活动
    if(isByMt){
        QHashIterator<int,Double> it(oSums);
        while(it.hasNext()){
            it.next();
            if(oDir == DIR_J)
                insertNewAction(row++, curPzId, row+1, DIR_J, it.value(),it.key());
            else
                insertNewAction(row++, curPzId, row+1, DIR_D, it.value(),it.key());
            busiActions[row-1]->state = BusiActionData2::NEW;
        }
    }
    else{ //将所有币种金额合计创建一个单一的对冲业务活动
        if(oDir == DIR_J)
            insertNewAction(row, curPzId, row+1, DIR_J, oSum, RMB, summ);
        else
            insertNewAction(row, curPzId, row+1, DIR_D, oSum, RMB, summ);
        busiActions[row]->state = BusiActionData2::NEW;
    }
    refreshVHeaderView();
    calSums();
    acDirty = true;
    pzContentModify();
}

//初始化新的空业务活动的数据
void PzDialog2::initNewAction(int row, int pid, int num, int dir, Double v,
        int mt, QString summary,int fid,int sid, BusiActionData2::ActionState state)
{
    installDataWatch(false);
    QTableWidgetItem* item;
    BASummaryItem* smItem = new BASummaryItem(summary, smg);//摘要
    ui->twActions->setItem(row,0,smItem);
    BAFstSubItem* fstItem = new BAFstSubItem(fid, smg);  //一级科目
    ui->twActions->setItem(row,1,fstItem);
    int subSys = curAccount->getCurSuite()->subSys;
    SubjectManager* smg = curAccount->getSubjectManager(subSys);
    BASndSubItem* sndItem = new BASndSubItem(sid, smg);
    ui->twActions->setItem(row,2,sndItem);              //二级科目
    BAMoneyTypeItem* mtItem = new BAMoneyTypeItem(mt, &allMts);//币种
    ui->twActions->setItem(row,3,mtItem);
    BAMoneyValueItem* jItem;
    BAMoneyValueItem* dItem;
    if(dir == DIR_J){
        jItem = new BAMoneyValueItem(DIR_J, v.getv());       //借方金额
        dItem = new BAMoneyValueItem(DIR_D, 0);       //贷方金额
    }
    else if(dir == DIR_D){
        jItem = new BAMoneyValueItem(DIR_J, 0);
        dItem = new BAMoneyValueItem(DIR_D, v.getv());
    }
    else{
        jItem = new BAMoneyValueItem(DIR_J, 0);
        dItem = new BAMoneyValueItem(DIR_D, 0);
    }
    ui->twActions->setItem(row,4,jItem);
    ui->twActions->setItem(row,5,dItem);

    item = new QTableWidgetItem(QString::number(dir)); //借贷方向（默认借方）
    ui->twActions->setItem(row,6,item);
    item = new QTableWidgetItem(QString::number(0));   //新业务活动在还未保存前id为0
    ui->twActions->setItem(row,7,item);
    item = new QTableWidgetItem(QString::number(pid)); //所属凭证id（pid）
    ui->twActions->setItem(row,8,item);
    item = new QTableWidgetItem(QString::number(num)); //序号
    ui->twActions->setItem(row,9,item);

    installDataWatch();

    busiActions[row]->id = 0;
    busiActions[row]->state = state;
    busiActions[row]->num = num;
    busiActions[row]->pid = pid;
    busiActions[row]->fid = fid;
    busiActions[row]->sid = sid;
    busiActions[row]->summary = summary;
    busiActions[row]->mt = mt;
    busiActions[row]->dir = dir;
    busiActions[row]->v = v;

    //如果是插入，则还需调整后续业务活动的序号
    //if(row < numActions - 1){
    //    for(int i = row+1; i < numActions; ++i){
    //        busiActions[i]->num++;
    //        ui->twActions->item(i,ActionEditItemDelegate::NUM)->setData(Qt::EditRole, busiActions[i]->num);
    //    }
    //}

    //acDirty = true; //这是空的业务活动，还没有实质的用户输入数据，因此无须保存
}

//当前凭证是否可编辑
bool PzDialog2::isEditable()
{
    //需要综合考虑凭证集状态、凭证状态和凭证类别
    if(curPzId == 0)
        return false;
    int state = getPzState();
    if((state == Pzs_Verify) || (state == Pzs_Instat))
        return false;
    if(curPzSetState != Ps_Rec)
        return false;
    if(curPzClass != Pzc_Hand)
        return false;
    return !readOnly;
}

//在表格尾部添加备用空白业务活动
void PzDialog2::appendBlankAction()
{
    ui->twActions->appendRow();
    BusiActionData2* aData = new BusiActionData2;
    aData->state = BusiActionData2::BLANK;
    busiActions.append(aData);
    initNewAction(numActions,curPzId,numActions+1);
    numActions++;
    delegate->setVolidRows(numActions);
    refreshVHeaderView();
}

//在表格尾部添加新的业务活动
void PzDialog2::appentNewAction(BusiActionData2 *ba)
{
    appendNewAction(ba->pid,ba->num,ba->dir,ba->v,ba->mt,ba->summary,
                    ba->fid,ba->sid,ba->state);
}

//在表格尾部添加新的业务活动（用提供到参数初始化当前最后一条业务活动，然后在表格及内部列表中添加新的空业务活动）
void PzDialog2::appendNewAction(int pid, int num, int dir, Double v,int mt,
    QString summary, int fid, int sid, BusiActionData2::ActionState state)
{
    //if(numActions == ui->twActions->rowCount())
    //    ui->twActions->insertRow(numActions);
    ui->twActions->appendRow();
    BusiActionData2* aData = new BusiActionData2;
    aData->state = BusiActionData2::BLANK;
    busiActions.append(aData);
    initNewAction(numActions-1,pid,num,dir,v,mt,summary,fid,sid,state);
    numActions++;
    delegate->setVolidRows(numActions);
    refreshVHeaderView();
}

//在指定位置row插入新的业务活动
void PzDialog2::insertNewAction(int row, BusiActionData2 *ba)
{
    insertNewAction(row,ba->pid,ba->num,ba->dir,ba->v,ba->mt,ba->summary,ba->fid,ba->sid,ba->state);
}

//在指定位置row插入新的业务活动
void PzDialog2::insertNewAction(int row, int pid, int num, int dir, Double v,
    int mt, QString summary, int fid, int sid, BusiActionData2::ActionState  state)
{
    ui->twActions->insertRow(row);
    BusiActionData2* aData = new BusiActionData2;
    busiActions.insert(row,aData);
    numActions++;
    initNewAction(row,pid,num,dir,v,mt,summary,fid,sid,state);
    delegate->setVolidRows(numActions);
    //ui->twActions->setCurrentCell(row,0);
    //ui->twActions->edit(ui->twActions->model()->index(row,0));

}

//计算借贷方向的合计值，并返回是否借贷平衡
bool PzDialog2::calSums()
{
    Double jv,dv;
    for(int i = 0; i < numActions; ++i){
        if(busiActions[i]->dir == DIR_J)
            jv += busiActions[i]->v * rates.value(busiActions[i]->mt);
        else
            dv += busiActions[i]->v * rates.value(busiActions[i]->mt);
    }
    //QString jvs = QString::number(jv,'f',2);
    //QString dvs = QString::number(dv,'f',2);

    //ui->edtJSum->setText(removeRightZero(jvs));
    //ui->edtDSum->setText(removeRightZero(dvs));
    ui->edtJSum->setText(jv.toString());
    ui->edtDSum->setText(dv.toString());


    if(jv != dv){
    //if(jvs != dvs){
        //设置背景为红色
        //edtJsum->setStyleSheet("background-color:rgba(255,0,0,255)");
        //edtDsum->setStyleSheet("background-color:rgba(255,0,0,255)");
        //将显示合计值的控件的字体颜色设为红色
        QPalette pal = ui->edtJSum->palette();
        pal.setColor(QPalette::Text,QColor(255,0,0));
        ui->edtJSum->setPalette(pal);
        ui->edtDSum->setPalette(pal);
        return false;
    }
    else{
        //设置背景为白色
        //edtJsum->setStyleSheet("background-color:rgba(255,255,255,255)");
        //edtDsum->setStyleSheet("background-color:rgba(255,255,255,255)");
        //将显示合计值的控件的字体颜色设为黑色
        QPalette pal = ui->edtJSum->palette();
        pal.setColor(QPalette::Text,QColor(0,0,0));
        ui->edtJSum->setPalette(pal);
        ui->edtDSum->setPalette(pal);
        return true;
    }
}

//向主窗口报告凭证内容改变了
void PzDialog2::pzContentModify()
{
    emit pzContentChanged(true);
    if(!timer->isActive())
        timer->start();
}

//凭证自编号改变了
void PzDialog2::zbNumChanged()
{
    pzDirty = true;
    pzContentModify();
}

//凭证附件数改变了
void PzDialog2::encChanged()
{
    pzDirty = true;
    pzContentModify();
}

//凭证日期改变了
void PzDialog2::pzDateChanged()
{
    pzDirty = true;
    pzContentModify();
}

//凭证到借贷合计金额改变了
void PzDialog2::jdSumChanged()
{
    pzDirty = true;
    //pzContentModify();  //合计金额值到改变，只能由业务活动表格内到数据改变引起，因此此处无须再次调用
}

//在定时器超时时，自动保存被修改到凭证
void PzDialog2::autoSave()
{
//    save();
//    timer->stop();
//    emit saveCompleted();
}

//用户单击了业务活动表行标题，这里要判断选择的行，以及行是否是连续的
//void PzDialog2::rowClicked(int logicalIndex)
//{
//    if(logicalIndex < (numActions-1)){
//        ui->twActions->selectedRows(selRows, isContinue);
//        //确定所选行的借贷方向是否同向
//        if(selRows.count() > 0){
//            if(isContinue){
//                isSameDir = true;
//                int startRow = selRows[0];
//                for(int i = 1; i < selRows.count(); ++i){
//                    if(startRow != selRows[i] - 1){
//                        isSameDir = false;
//                        break;
//                    }
//                    else
//                        startRow = selRows[i];
//                }
//            }
//            emit selectedBaAction(true);
//        }
//        else
//            emit selectedBaAction(false);

//    }
//}

//主要是为了清除已经选择的行集合
//void PzDialog2::currentCellChanged(int currentRow, int currentColumn,
//                        int previousRow, int previousColumn)
//{
    //selRows.clear();
    //isContinue = false;
    //isSameDir = false;
    //emit selectedBaAction(false);
//}

//监视用户选择行为
void PzDialog2::curItemSelectionChanged()
{
    ui->twActions->selectedRows(selRows,isContinue);
    if(!selRows.empty()){
        if(isContinue){
            isSameDir = true;
            int dir = busiActions[selRows[0]]->dir ;
            for(int i = 1; i < selRows.count(); ++i){
                if(dir != busiActions[selRows[i]]->dir){
                    isSameDir = false;
                    break;
                }
            }
        }
        else
            isSameDir = false;
        emit selectedBaAction(true);
    }
    else{
        isContinue = false;
        isSameDir = false;
        emit selectedBaAction(false);
    }
}

//无法捕获在水平标题条上的单击事件
//void PzDialog2::cellClicked(int row, int column)
//{
//    int i = 0;
//}

//凭证日期改变了
//void PzDialog2::pzDateChanged()
//{
//    pzContentModify();
//}

//凭证附件数改变了
//void PzDialog2::pzEncNumChanged()
//{
//    pzContentModify();
//}

//凭证自编号改变了
//void PzDialog2::pzZbNumChanged()
//{
//    pzContentModify();
//}

//上下文菜单-添加业务活动
void PzDialog2::on_actAddNewAction_triggered()
{
    if(isEditable()){
        addBusiAct();
        acDirty = true;
        pzContentModify();
    }
}

//上下文菜单-插入新的空白业务活动
void PzDialog2::on_actInsertNewAction_triggered()
{
    if(!isEditable())
        return;
    if(selRows.empty() || (selRows.count() > 1))
        return;
    int row = selRows[0];
    insertNewAction(row,curPzId,row+1);
    refreshVHeaderView();
    acDirty = true;
    pzContentModify();
}

//上下文菜单-插入合计对冲业务活动
void PzDialog2::on_actInsertOppoAction_triggered()
{
    if(!isEditable())
        return;
    requestCrtNewOppoAction();
}

//上下文菜单-删除业务活动
void PzDialog2::on_actDelAction_triggered()
{    
    if(isEditable() && !selRows.empty()){
        delBusiAct();
        acDirty = true;
        pzContentModify();
    }
}

//复制业务活动
void PzDialog2::on_actCopyAction_triggered()
{

    if(selRows.empty())
        return;    
    clbBaList.clear();
    BusiActionData2* ba;
    foreach(int i, selRows){
        ba = busiActions[i];
        clbBaList << ba;
    }
    copyOrCut = true;
}

//剪切业务活动
void PzDialog2::on_actCutAction_triggered()
{

    if(selRows.empty() || !isEditable())
        return;
    clbBaList.clear();
    for(int i = selRows.count()-1; i > -1; i--){
        BusiActionData2* ba = new BusiActionData2(busiActions[selRows[i]]); //复制剪切到业务活动
        delActions.append(busiActions.takeAt(selRows[i]));
        delActions[delActions.count()-1]->state = BusiActionData2::DELETED;
        ui->twActions->removeRows(selRows[i]);
        numActions--;
        clbBaList.push_front(ba); //保持剪切时的顺序
    }
    delegate->setVolidRows(numActions);
    acDirty = true;
    copyOrCut = false;
    refreshVHeaderView();
    pzContentModify();
}

//粘贴业务活动
void PzDialog2::on_actPasteAction_triggered()
{

    if(!isEditable() || (clbBaList.count() == 0))
        return;
    if(selRows.empty() || (selRows.count() > 1))
        return;
    int stPos = selRows[0];
    int pos = stPos;
    int num = clbBaList.count();
    int oldPid; //复制的业务活动所属的凭证id
    BusiActionData2::ActionState oldState;  //复制的业务活动的编辑状态
    if(pos < numActions-1){  //插入粘贴
        for(int i = 0; i < num; ++i){
            //保存复制时的原始状态数据
            oldPid = clbBaList[i]->pid;
            oldState = clbBaList[i]->state;
            //修改为新的状态数据
            clbBaList[i]->pid = curPzId;
            //if(copyOrCut || oldPid != curPzId)
                clbBaList[i]->state = BusiActionData2::NEW;
            //else
            //    clbBaList[i]->state = BusiActionData::EDITED;
            insertNewAction(pos++, clbBaList[i]);
            //恢复复制时的原始数据
            clbBaList[i]->pid = oldPid;
            clbBaList[i]->state = oldState;
        }
        for(int i = stPos+num; i < numActions; ++i){
            if(busiActions[i]->state == BusiActionData2::INIT)
                busiActions[i]->state = BusiActionData2::NUMCHANGED;
        }
    }
    else if(pos == numActions-1){ //末尾添加粘贴
        for(int i = 0; i < num; ++i){
            //保存复制时的原始数据
            oldPid = clbBaList[i]->pid;
            oldState = clbBaList[i]->state;
            //修改为新的状态数据
            clbBaList[i]->pid = curPzId;            
            clbBaList[i]->state = BusiActionData2::NEW;
            appentNewAction(clbBaList[i]);
            //恢复复制时的原始数据
            clbBaList[i]->pid = oldPid;
            clbBaList[i]->state = oldState;
        }
    }    
    recalSum();
    acDirty = true;
    pzContentModify();
    refreshVHeaderView();
}

//折叠或展开结转汇兑损益的业务活动
void PzDialog2::on_actCollaps_triggered()
{
    isCollapseJz = !isCollapseJz;
    initAction();
}

//凭证日期编辑结束
//void PzDialog2::on_pzDate_editingFinished()
//{
//    int idx = dataMapping->currentIndex();
//    QString old = model->data(model->index(idx,PZ_DATE)).toString();
//    if(old != ui->pzDate->date().toString(Qt::ISODate))
//        emit pzContentModify();
//}

//凭证附件数编辑结束
//void PzDialog2::on_spbEnc_editingFinished()
//{
//    int idx = dataMapping->currentIndex();
//    int old = model->data(model->index(idx,PZ_ENCNUM)).toInt();
//    if(ui->spbEnc->value() != old)
//        emit pzContentModify();
//}


//创建所选业务活动到合计对冲业务活动
void PzDialog2::on_actInsSelOppoAction_triggered()
{
    if(!isSameDir)
        return;

    int dir,row;
    row = selRows.last()+1; //插入位置在所选行的最后一行的下一行
    if(busiActions[row-1]->dir == DIR_J)  //新业务活动的方向
        dir = DIR_D;
    else
        dir = DIR_J;

    if(selRows.count() == 1){
        insertNewAction(row,curPzId,row+1,dir,busiActions[row-1]->v,
                        busiActions[row-1]->mt,busiActions[row-1]->summary,
                        0,0,BusiActionData2::NEW);
    }
    else{   //选择行数多余一条
        QInputDialog* dlg = new QInputDialog(this);
        QStringList items;
        items << tr("混合币种") << tr("按币种分开合计");
        dlg->setComboBoxItems(items);
        if(dlg->exec() == QDialog::Accepted){
            QString sel = dlg->textValue();
            if(sel == tr("混合币种"))
                isByMt = false;
            else
                isByMt = true;
        }
        else{ //取消建立
            return;
        }
        int mt;
        Double v;
        if(isByMt){  //按币种分开建立
            QHash<int,Double> sums;
            for(int i = 0; i < selRows.count(); ++i){
                mt = busiActions[selRows[i]]->mt;
                v = busiActions[selRows[i]]->v;
                sums[mt] += v;
            }
            QHashIterator<int,Double> it(sums);
            while(it.hasNext()){
                it.next();
                insertNewAction(row++,curPzId,row,dir,it.value(),it.key(),"",0,0,
                                BusiActionData2::NEW);
            }
        }
        else{        //将所有业务活动金额归并为母币
            Double sum;
            for(int i = 0; i < selRows.count(); ++i){
                mt = busiActions[selRows[i]]->mt;
                sum += (busiActions[selRows[i]]->v * rates.value(mt));
            }
            insertNewAction(row,curPzId,row+1,dir,sum,RMB,"",0,0,
                            BusiActionData2::NEW);
        }
    }
    refreshVHeaderView();
    calSums();
    acDirty = true;
    pzContentModify();
}
