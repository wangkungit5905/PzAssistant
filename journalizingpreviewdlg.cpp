#include "journalizingpreviewdlg.h"
#include "ui_journalizingpreviewdlg.h"

#include "subject.h"
#include "myhelper.h"
#include "PzSet.h"
#include "pz.h"
#include "dbutil.h"
#include "delegates2.h"
#include "widgets/bawidgets.h"
#include "utils.h"
#include "config.h"
#include "completsubinfodialog.h"
#include "statements.h"

#include <QDialog>
#include <QMenu>
#include <QTableWidgetSelectionRange>
#include <QInputDialog>


//////////////////////////////////////VerifyTagWidget////////////////////////////////////////////////////////////////
VerifyTagWidget::VerifyTagWidget(QWidget *parent)
{
    pixOk.load(":/images/verify-ok.png");
    pixNot.load(":/images/verify-not.png");

    lblDate = new QLabel(this);
    lblDiffValue = new QLabel(this);
    lblDiffValue->setStyleSheet("color: rgb(203, 11, 42);");
    lblTag = new QLabel(this);
    lblTag->setPixmap(pixOk);
    QHBoxLayout* lh = new QHBoxLayout;
    lh->addWidget(lblTag);
    lh->addWidget(lblDiffValue);
    QVBoxLayout* lm = new QVBoxLayout(this);
    lm->setContentsMargins(-1,1,-1,1);
    lm->addWidget(lblDate);
    lm->addLayout(lh);
    setLayout(lm);
}

void VerifyTagWidget::setValue(Double v)
{

    if(v!=0){
        lblDiffValue->setText(v.toString());
        lblTag->setPixmap(pixNot);
    }
    else{
        lblDiffValue->setText("");
        lblTag->setPixmap(pixOk);
    }
}





///////////////////////////////////////JournalizingPreviewDlg//////////////////////////////////////////////////////
JournalizingPreviewDlg::JournalizingPreviewDlg(SubjectManager* sm, QWidget *parent) :
    QDialog(parent),ui(new Ui::JournalizingPreviewDlg),sm(sm),isInteracting(false)
{
    ui->setupUi(this);
    mmt = sm->getAccount()->getMasterMt();
    smg = sm->getAccount()->getSuiteMgr();
    curBa = 0;
    init();     
    foreach(CurInvoiceRecord* i,*smg->getCurInvoiceRecords()){
        if(i->inum == "00000000")
            continue;
        incomes<<i;
    }
    foreach(CurInvoiceRecord* i,*smg->getCurInvoiceRecords(false)){
        if(i->inum == "00000000")
            continue;
        costs<<i;
    }
    yss = smg->getYsInvoiceStats(); // 2. 准备前期未销账的应收发票和应付发票
    yfs = smg->getYfInvoiceStats();
    loadJournals();
    ui->twJos->setRowCount(jls.count());
    for(int r = 0; r < jls.count(); ++r){
    	Journalizing* j = jls.at(r);
        rendRow(r,j);
    }

    mergeToGroup();

}

JournalizingPreviewDlg::~JournalizingPreviewDlg()
{
    qDeleteAll(js);
    qDeleteAll(jls);
    delete ui;
}

/**
 * @brief 是否有未被保存的修改
 * @return
 */
bool JournalizingPreviewDlg::isDirty()
{
    for(int i = 0; i < jls.count(); ++i){
        Journalizing* j = jls.at(i);
        if(j->id == 0 || j->changed)
            return true;
    }
    return !doRemoves.isEmpty();
}


void JournalizingPreviewDlg::save()
{
    if(!doSaves.isEmpty())
        doSaves.clear();
    for(int i = 0; i < jls.count(); ++i){
        Journalizing* j = jls.at(i);
        if(j->id == 0 || j->changed)
            doSaves<<j;
    }
    bool isOk=true;
    if(!doSaves.isEmpty()){
        if(!dbUtil->saveJournalizings(doSaves))
            isOk = false;
    }
    if(!doRemoves.isEmpty()){
        if(!dbUtil->removeJournalizings(doRemoves))
            isOk = false;
    }
    if(!isOk){
        myHelper::ShowMessageBoxError(tr("保存流水账分录时发生错误！"));
        return;
    }
    if(!doSaves.isEmpty())
        doSaves.clear();
    if(!doRemoves.isEmpty()){
        qDeleteAll(doRemoves);
        doRemoves.clear();
    }
}

/**
 * @brief 在当前行的下一行摘要栏打开编辑器
 * @param row
 */
void JournalizingPreviewDlg::moveNextRow(int row)
{
    if(row == ui->twJos->rowCount()-1)
        return;
    ui->twJos->editItem(ui->twJos->item(row+1,JCI_SUMMARY));
}



void JournalizingPreviewDlg::init()
{
    initSubs();
    initKeywords();
    initColors();
    Account* account = sm->getAccount();
    year = account->getSuiteMgr()->year();
    month = account->getSuiteMgr()->month();
    dbUtil = account->getDbUtil();
    QDate d(year,month,1);
    QString ds = QDate(year,month,d.daysInMonth()).toString(Qt::ISODate);
    jsr_p = new Journal;
    jsr_p->id = -1;
    jsr_p->summary = tr("收入普票");
    jsr_p->date = ds;
    jsr_z = new Journal;
    jsr_z->id = -2;
    jsr_z->summary = tr("收入专票");
    jsr_z->date = ds;
    jcb_p = new Journal;
    jcb_p->id = -3;
    jcb_p->summary = tr("成本普票");
    jcb_p->date = ds;
    jcb_z = new Journal;
    jcb_z->id = -4;
    jcb_z->summary = tr("成本专票");
    jcb_z->date = ds;
    titles<<"凭证号"<<"组"<<"摘要"<<"一级科目"<<"二级科目"<<"币种"<<"借方"<<"贷方"<<"借贷平衡";
    ui->twJos->setHorizontalHeaderLabels(titles);
    ui->twJos->setColumnWidth(JCI_PNUM,60);
    ui->twJos->setColumnWidth(JCI_GROUP,30);
    ui->twJos->setColumnWidth(JCI_SUMMARY,400);
    ui->twJos->setColumnWidth(JCI_FSUB,100);
    ui->twJos->setColumnWidth(JCI_SSUB,150);
    ui->twJos->setColumnWidth(JCI_MTYPE,70);
    ui->twJos->setColumnWidth(JCI_JV,100);
    ui->twJos->setColumnWidth(JCI_DV,100);
    ui->twJos->setColumnWidth(JCI_VTAG,150);
    sc_save = new QShortcut(QKeySequence("Ctrl+s"),this);
    sc_copy = new QShortcut(QKeySequence("Ctrl+c"),this);
    sc_cut = new QShortcut(QKeySequence("Ctrl+x"),this);
    sc_paster = new QShortcut(QKeySequence("Ctrl+v"),this);
    connect(sc_save,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_copy,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_cut,SIGNAL(activated()),this,SLOT(processShortcut()));
    connect(sc_paster,SIGNAL(activated()),this,SLOT(processShortcut()));
    delegate = new JolItemDelegate(sm,this);
    connect(delegate,SIGNAL(reqCopyPrevAction(int)),this,SLOT(copyPrewAction(int)));
    connect(delegate,SIGNAL(crtNewNameItemMapping(int,int,FirstSubject*,SubjectNameItem*,SecondSubject*&)),
            this,SLOT(creatNewNameItemMapping(int,int,FirstSubject*,SubjectNameItem*,SecondSubject*&)));
    connect(delegate,SIGNAL(crtNewSndSubject(int,int,FirstSubject*,SecondSubject*&,QString)),
            this,SLOT(creatNewSndSubject(int,int,FirstSubject*,SecondSubject*&,QString)));
    connect(delegate,SIGNAL(moveNextRow(int)),this,SLOT(moveNextRow(int)));
    connect(ui->twJos,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(twContextMenuRequested(QPoint)));
    ui->twJos->setItemDelegate(delegate);
    connect(ui->twJos,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(currentCellChanged(int,int,int,int)));

    connect(ui->actCollectToPz,SIGNAL(triggered()),this,SLOT(mnuCollectToPz()));
    connect(ui->actDissolvePz,SIGNAL(triggered()),this,SLOT(mnuDissolvePz()));
    connect(ui->actInsert,SIGNAL(triggered()),this,SLOT(mnuInsertBa()));
    connect(ui->actRemove,SIGNAL(triggered()),this,SLOT(mnuRemoveBa()));
    connect(ui->actCopy,SIGNAL(triggered()),this,SLOT(mnuCopyBa()));
    connect(ui->actCut,SIGNAL(triggered()),this,SLOT(mnuCutBa()));
    connect(ui->actPaste,SIGNAL(triggered()),this,SLOT(mnuPasteBa()));
    connect(ui->actMerge,SIGNAL(triggered()),this,SLOT(mnuMerge()));
    connect(ui->tbInsert,SIGNAL(clicked()),this,SLOT(mnuInsertBa()));
    //connect(ui->tbInsert,SIGNAL(clicked()),ui->actInsert,SIGNAL(triggered()));
    connect(ui->tbRemove,SIGNAL(clicked()),this,SLOT(mnuRemoveBa()));
    ui->tbUp->setShortcut(QKeySequence(tr("Shift+Up")));
    ui->tbDown->setShortcut(QKeySequence(tr("Shift+Down")));
    ui->tbGUp->setShortcut(QKeySequence(tr("Ctrl+Up")));
    ui->tbGDown->setShortcut(QKeySequence(tr("Ctrl+Down")));

    appCfg = AppConfig::getInstance();
    smartSSubSet = appCfg->isOnSmartSSubSet();
    QString subCode = ysSub->getCode();
    prefixes[subCode] = appCfg->getSmartSSubFix(subCode,AppConfig::SSF_PREFIXE);
    suffixes[subCode] = appCfg->getSmartSSubFix(subCode,AppConfig::SSF_SUFFIXE);
    if(prefixes.value(subCode).isEmpty() || suffixes.value(subCode).isEmpty())
        smartSSubSet=false;
    subCode = yfSub->getCode();
    prefixes[subCode] = appCfg->getSmartSSubFix(subCode,AppConfig::SSF_PREFIXE);
    suffixes[subCode] = appCfg->getSmartSSubFix(subCode,AppConfig::SSF_SUFFIXE);
    if(prefixes.value(subCode).isEmpty() || suffixes.value(subCode).isEmpty())
        smartSSubSet=false;
    smg->getRates(rates,month);
}

void JournalizingPreviewDlg::initColors()
{
    color_pzNum1 = QColor(Qt::blue);
    color_pzNum2 = QColor(Qt::darkBlue);
    color_gNum1 = QColor(Qt::cyan);
    color_gNum2 = QColor(Qt::darkCyan);
}

void JournalizingPreviewDlg::initSubs()
{
    cashSub = sm->getCashSub();
	bankSub = sm->getBankSub();
	foreach(SecondSubject* sub, bankSub->getChildSubs())
		bankSubs[sub->getId()] = sub;
    SecondSubject* rmbSub = cashSub->getChildSubs().first();
	bankSubs[rmbSub->getId()] = rmbSub;
	ysSub = sm->getYsSub();
	yfSub = sm->getYfSub();
	zysrSub = sm->getZysrSub();
    zycbSub = sm->getZycbSub();
    xsfySub = sm->getXsfySub();
	glfySub = sm->getglfySub();
	cwfySub = sm->getCwfySub();
	yjsjSub = sm->getYjsjSub();
    gzSub = sm->getGzSub();

	jxseSSub = sm->getJxseSSub();
	xxseSSub = sm->getXxseSSub();
	hdsySSub = sm->getHdsySSub();
    sxfSSub = sm->getSxfSSub();
    sszbSub = sm->getSszbSub();
}

/**
 * @brief 初始化关键字列表
 */
void JournalizingPreviewDlg::initKeywords()
{
    kwSetting = new QSettings("./config/app/keywords.ini", QSettings::IniFormat);
    kwSetting->setIniCodec("UTF-8");
    kwSetting->beginGroup("keywords");
    kwCash = kwSetting->value("Cash").toStringList();
    kwBankTranMoney = kwSetting->value("BankTranMoney").toStringList();
    kwBuyForex = kwSetting->value("BuyForex").toStringList();
    kwOversea = kwSetting->value("Oversea").toStringList();
    kwSalary = kwSetting->value("Salary").toStringList();
    kwServiceFee = kwSetting->value("ServiceFee").toStringList();
    kwInvest = kwSetting->value("Invest").toStringList();
    kwPreInPay = kwSetting->value("PreInPay").toStringList();
    kwSetting->endGroup();
}

void JournalizingPreviewDlg::turnWatchData(bool on)
{
    if(on)
        connect(ui->twJos,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
    else
        disconnect(ui->twJos,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
}


void JournalizingPreviewDlg::loadJournals()
{
    if(!dbUtil->readJournals(year,month,js,sm)){
		myHelper::ShowMessageBoxWarning(tr("读取流水账记录时发生错误！"));
		return;
	}
	foreach(Journal* j, js)
        jMaps[j->id] = j;
    qSort(js.begin(),js.end(),journalThan);
    jMaps[jsr_p->id] = jsr_p;
    jMaps[jsr_z->id] = jsr_z;
    jMaps[jcb_p->id] = jcb_p;
    jMaps[jcb_z->id] = jcb_z;
    if(!dbUtil->readJournalizings(jls, jMaps, sm)){
		myHelper::ShowMessageBoxWarning(tr("读取流水分录表时发生错误！"));
		return;
	}
    if(jls.isEmpty())
        genarating();
}

void JournalizingPreviewDlg::initBlankBa(int row)
{
    BASummaryItem_new* smItem = new BASummaryItem_new("", sm);
    ui->twJos->setItem(row,JCI_SUMMARY,smItem);
    BAFstSubItem_new* fstItem = new BAFstSubItem_new(NULL, sm);
    ui->twJos->setItem(row,JCI_FSUB,fstItem);
    BASndSubItem_new* sndItem = new BASndSubItem_new(NULL, sm);
    ui->twJos->setItem(row,JCI_SSUB,sndItem);
    BAMoneyTypeItem_new* mtItem = new BAMoneyTypeItem_new(NULL);
    ui->twJos->setItem(row,JCI_MTYPE,mtItem);
    BAMoneyValueItem_new* jItem,*dItem;
    jItem = new BAMoneyValueItem_new(DIR_J, 0);
    dItem = new BAMoneyValueItem_new(DIR_D, 0);
    ui->twJos->setItem(row,JCI_JV,jItem);
    ui->twJos->setItem(row,JCI_DV,dItem);
}

/**
 * 用于初次生成分录时渲染单一的分录
 * @param row 
 * @param jo  
 */
void JournalizingPreviewDlg::rendRow(int row,Journalizing* jo)
{
    turnWatchData(false);
    BASummaryItem_new* smItem = new BASummaryItem_new(jo->summary, sm);
	QVariant v;
	v.setValue(jo);
    smItem->setData(DR_JOURNALIZING,v);
	v.setValue(jo->journal);
    smItem->setData(DR_JOURNAL,v);
    //smItem->setData(Qt::ToolTipRole,jo->journal->date);
    ui->twJos->setItem(row,JCI_SUMMARY,smItem);
    BAFstSubItem_new* fstItem = new BAFstSubItem_new(jo->fsub, sm);
    v.setValue(jo->fsub);
    fstItem->setData(Qt::EditRole, v);
    ui->twJos->setItem(row,JCI_FSUB,fstItem);
    BASndSubItem_new* sndItem = new BASndSubItem_new(jo->ssub, sm);
    v.setValue(jo->ssub);
    sndItem->setData(Qt::EditRole,v);
    ui->twJos->setItem(row,JCI_SSUB,sndItem);
    BAMoneyTypeItem_new* mtItem = new BAMoneyTypeItem_new(jo->mt);
    v.setValue(jo->mt);
    mtItem->setData(Qt::EditRole, v);
    ui->twJos->setItem(row,JCI_MTYPE,mtItem);
    BAMoneyValueItem_new* jItem,*dItem;
    if(jo->dir == DIR_J){
        jItem = new BAMoneyValueItem_new(DIR_J, jo->value);
        dItem = new BAMoneyValueItem_new(DIR_D, 0);
    }
    else{
        jItem = new BAMoneyValueItem_new(DIR_J, 0);
        dItem = new BAMoneyValueItem_new(DIR_D, jo->value);
    }
    ui->twJos->setItem(row,JCI_JV,jItem);
    ui->twJos->setItem(row,JCI_DV,dItem);
    //ui->twJos->setItem(row,JCI_VTAG,new QTableWidgetItem(jo->journal?jo->journal->date:""));
    turnWatchData();
}

/**
 * 合并凭证号、组、审核列
 */
void JournalizingPreviewDlg::mergeToGroup()
{
    int curGNum = 1;
    int curPNum = 1;
    int startPRow = 0;
    int startGRow = 0;
    turnWatchData(false);
    Journalizing* j = jls.first();
    Double v = j->mt==mmt?j->value:j->value*rates.value(j->mt->code());
    Double sum = j->dir==MDIR_J?v:sum-v;
    for(int i = 1; i < jls.count(); ++i){
        j = jls.at(i);
        if(j->gnum != curGNum){
            ui->twJos->setSpan(startGRow,JCI_GROUP,i-startGRow,1); //组号列合并
            QTableWidgetItem* item = new QTableWidgetItem(QString::number(curPNum));
            item->setBackgroundColor(curGNum%2?color_pzNum1:color_pzNum2);
            ui->twJos->setItem(startGRow,JCI_PNUM,item);
            item = new QTableWidgetItem(QString::number(curGNum));
            item->setBackgroundColor(curGNum%2?color_gNum1:color_gNum2);
            ui->twJos->setItem(startGRow,JCI_GROUP,item);
            ui->twJos->setSpan(startGRow,JCI_VTAG,i-startGRow,1); //平衡标志列合并
            VerifyTagWidget* w = new VerifyTagWidget(ui->twJos);
            w->setDate(j->journal?j->journal->date:"");
            w->setValue(sum);
            ui->twJos->setCellWidget(startGRow,JCI_VTAG,w);
            //ui->twJos->setItem(startGRow,JCI_VTAG,new QTableWidgetItem(j->journal?j->journal->date:""));
            startGRow = i;
            curGNum = j->gnum;
            sum = 0;
        }
        v = j->mt==mmt?j->value:j->value*rates.value(j->mt->code());
        if(j->dir == MDIR_J)
            sum += v;
        else
            sum -= v;
        if(j->pnum != curPNum){
            ui->twJos->setSpan(startPRow,JCI_PNUM,i-startPRow,1);
            startPRow = i;
            curPNum = j->pnum;
        }
    }
    if(startGRow < jls.count()-1){
        ui->twJos->setSpan(startGRow,JCI_GROUP,jls.count()-startGRow,1); //组号列合并
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(curPNum));
        item->setBackgroundColor(curGNum%2?color_pzNum1:color_pzNum2);
        ui->twJos->setItem(startGRow,JCI_PNUM,item);
        item = new QTableWidgetItem(QString::number(curGNum));
        item->setBackgroundColor(curGNum%2?color_gNum1:color_gNum2);
        ui->twJos->setItem(startGRow,JCI_GROUP,item);
        ui->twJos->setSpan(startGRow,JCI_VTAG,jls.count()-startGRow,1); //平衡标志列合并
        ui->twJos->setSpan(startPRow,JCI_PNUM,jls.count()-startPRow,1);
    }
    turnWatchData();
}

/**
 * @brief JournalizingPreviewDlg::getCurrentGroupRowIndex
 * 得到当前行所在组的开始和结束行索引
 * @param sr  开始行索引
 * @param er  结束行索引
 */
void JournalizingPreviewDlg::getCurrentGroupRowIndex(int &sr, int &er)
{
    if(!curBa){
        sr=-1;er=-1;
        return;
    }
    int curRow = ui->twJos->currentRow();
    int curGNum = curBa->gnum;
    int spanRows = ui->twJos->rowSpan(curRow,JCI_GROUP);
    if(spanRows == 1){
        sr=curRow;er=curRow;
        return;
    }
    if(curBa->numInGroup == 1)
        sr = curRow;
    else{
        int row=curRow-1;
        Journalizing* j = jls.at(row);
        while(j->gnum == curGNum){
            row--;
            if(row==-1)
                break;
            j = jls.at(row);
        }
        sr = ++row;
    }
    if(curBa->numInGroup == spanRows){
        er=curRow;
    }
    else{
        int row = curRow+1;
        Journalizing* j = jls.at(row);
        while(j->gnum == curGNum){
            row++;
            if(row==jls.count())
                break;
            j = jls.at(row);
        }
        er = --row;
    }
}

/**
 * @brief JournalizingPreviewDlg::calBalanceInGroup
 * 计算组内的借贷平衡值
 * @param sr 组的开始行数
 * @param er 组的结束行数
 */
void JournalizingPreviewDlg::calBalanceInGroup(int sr, int er)
{
    Double sum;
    for(int i = sr; i <= er; ++i){
        Journalizing *j = jls.at(i);
        Double v = j->mt==mmt?j->value:j->value * rates.value(j->mt->code());
        if(j->dir == MDIR_J)
            sum += v;
        else
            sum -= v;
    }
    VerifyTagWidget* w = static_cast<VerifyTagWidget*>(ui->twJos->cellWidget(sr,JCI_VTAG));
    if(w)
        w->setValue(sum);
}

/**
 * 根据当前的流水账，生成分录
 */
void JournalizingPreviewDlg::genarating()
{
    Journal* j,*j2;
    int gnum=0;
    for(int i=0; i<js.count(); ++i){
        gnum++;
        j = js.at(i);
        if(ignoreJs.contains(j)){
            gnum--;
            continue;
        }
        if(!j->invoices.isEmpty()){
            jls<<processInvoices(gnum,j);
            continue;
        }
        j2 = isBugForex(j);
        if(j2){
            jls<<processBuyForex(gnum,j,j2);
            continue;
        }
        j2 = isBankTranMoney(j);
        if(j2){
            jls<<processBankTranMoney(gnum,j,j2);
            continue;
        }
        j2 = isWithdrawal(j);
        if(j2){
            jls<<processCash(gnum,j,j2);
            continue;
        }
        SecondSubject* ssub = isFee(j);
        if(ssub){
            jls<<processFee(gnum,j,ssub);
            continue;
        }
        ssub = isTax(j);
        if(ssub){
            jls<<processTax(gnum,j,ssub);
            continue;
        }
        ssub = isSalary(j);
        if(ssub){
            jls<<processSalary(gnum,j,ssub);
            continue;
        }
        if(isOversea(j)){
            jls<<processOversea(gnum,j);
            continue;
        }
        bool processed = false;
        jls<<processOther(gnum,j,processed);
        if(processed)
            continue;
        //无法辨析的流水账，则原样输出
        jls<<processUnkownJournal(gnum,j);
    }
    //最后生成收入和成本聚合分录
    jls<<genGatherBas(++gnum);
    gnum = jls.last()->gnum+1;
    jls<<genGatherBas(gnum,false);
    //如果有认证发票数据信息的支持，则产生自动检查先前生成的分录，并添加当月认证，税金挂在应付的发票

    //自动归并到凭证（日期为分界，每个凭证最多8条分录）
    if(jls.isEmpty())
        return;
    Journalizing* jo=0;
    int maxBas = 8;
    int curGNum = 0;  //
    QStringList gDates; //组对应日期
    QDate d(year,month,1);
    QString endDate = QDate(year,month,d.daysInMonth()).toString(Qt::ISODate);
    QList<int> gBas;  //组的结束索引位置
    for(int i = 0; i < jls.count(); ++i){
        jo = jls.at(i);
        if(jo->gnum != curGNum){
            if(curGNum >= 1){
                jls.at(i-1)->journal->numOfBas = gBas.last() - jls.at(i-1)->journal->startPos + 1;
            }
            curGNum = jo->gnum;
            gDates<<(jo->journal?jo->journal->date:endDate);
            gBas<<i;
            jo->journal->startPos = i;
            continue;
        }
        gBas.last()=i;
    }
    int curPzNum = 1;
    int si=0;
    int ei=gBas.at(0);
    bool ok = false;
    QString curPzDate = gDates.first();
    for(int i = 1; i<gDates.count(); ++i){
        QString date = gDates.at(i);
        if(curPzDate != date){
            curPzDate = date;
            ok = true;
        }
        else{
            int nums = gBas.at(i) - si + 1;
            if(nums > maxBas)
                ok = true;
        }
        if(ok){
            for(int j = si; j<=ei; ++j)
                jls.at(j)->pnum = curPzNum;
            curPzNum++;
            si = gBas.at(i-1)+1;
            ei = gBas.at(i);
            ok = false;
        }
        else{
            ei = gBas.at(i);
        }
    }
    for(int j = si; j<=ei; ++j)  //处理最后一组
        jls.at(j)->pnum = curPzNum;
}

/**
 * @brief 为一组应收或应付发票生成紧凑格式的发票号串
 * @param invoices
 * @return
 */
QString JournalizingPreviewDlg::genTerseInvoiceNums(QList<InvoiceRecord *> invoices)
{
    QHash<int,QStringList> ins;
    foreach (InvoiceRecord *i, invoices) {
        if(!ins.contains(i->month))
            ins[i->month] = QStringList();
        ins[i->month]<<i->invoiceNumber;
    }
    QList<int> months;
    QList<QStringList> inums;
    months = ins.keys();
    qSort(months.begin(),months.end());
    for(int i = 0; i < months.count(); ++i)
        inums<<ins.value(months.at(i));
    return PaUtils::terseInvoiceNumsWithMonth(months,inums);
}

/**
 * 从文本串中提取发票号
 * @param t
 */
QStringList JournalizingPreviewDlg::extractInvoice(QString t)
{
    QStringList invoices;
    PaUtils::extractInvoiceNum4(t,invoices);//这里的发票号可能包含了20位的数传发票号，必须先将其转换为传统的8位
    for(int i = 0; i<invoices.count(); i++){
        if(invoices[i].size() == 20)
            invoices[i] = invoices[i].right(8);
    }
    return invoices;
}


/**
 * 将发票进行分类（应收、收入、应付、成本）
 * @param invoices 待处理发票号列表
 * @param incomes  收入发票
 * @param costs    成本发票
 * @param yss      应收发票
 * @param yfs      应付发票
 * @return         无法识别的发票号列表
 */
QStringList JournalizingPreviewDlg::separateInvoices(QStringList invoices,QList<CurInvoiceRecord*> &incomes,
        QList<CurInvoiceRecord*> &costs, QList<InvoiceRecord*> &yss, QList<InvoiceRecord*> &yfs)
{
    QStringList dontKonws;
    foreach(QString i,invoices){
        CurInvoiceRecord* pi = findIncomesOrCosts(i);
        if(pi){
            incomes<<pi;
            continue;
        }
        pi = findIncomesOrCosts(i,false);
        if(pi){
            costs<<pi;
            continue;
        }
        InvoiceRecord* py = findInYsOrYf(i);
        if(py){
            yss<<py;
            continue;
        }
        py = findInYsOrYf(i,false);
        if(py){
            yfs<<py;
            continue;
        }
        dontKonws<<i;
    }
    return dontKonws;
}

/**
 * 查找并返回对应发票号的当月收入或成本发票记录
 * @param  inum     [description]
 * @param  isIncome [description]
 * @return          [description]
 */
CurInvoiceRecord* JournalizingPreviewDlg::findIncomesOrCosts(QString inum, bool isIncome)
{
    QList<CurInvoiceRecord *>* ls;
    if(isIncome)
        ls = &incomes;
    else
        ls = &costs;
    foreach(CurInvoiceRecord* i, *ls){
        if(i->inum == inum)
            return i;
    }
    return 0;
}

/**
 * 查找并返回对应发票号的以前月度的应收或应付发票记录
 * @param  inum [description]
 * @param  isYs [description]
 * @return      [description]
 */
InvoiceRecord* JournalizingPreviewDlg::findInYsOrYf(QString inum, bool isYs)
{
    QList<InvoiceRecord *>* ls;
    if(isYs)
        ls = &yss;
    else
        ls = &yfs;    
    foreach(InvoiceRecord* i, *ls){
        if(i->invoiceNumber == inum)
            return i;
    }
    return 0;
}

/**
 * 处理带有发票相关的收付款
 * @param 组序号
 * @param j [description]
 */
QList<Journalizing*> JournalizingPreviewDlg::processInvoices(int gnum,Journal* j)
{
    QList<Journalizing*> js;
    Journalizing* jb,*jo;
    SecondSubject* ssub = sm->getSndSubject(j->bankId);
    FirstSubject* fsub = ssub->getParent();
    Money* mt = sm->getSubMatchMt(ssub);
    if(!mt)
        mt = mmt;
    QStringList invoices = extractInvoice(j->invoices);
    if(!invoices.isEmpty()){
        QList<CurInvoiceRecord*> incomes;
        QList<CurInvoiceRecord*> costs;
        QList<InvoiceRecord*> yss;
        QList<InvoiceRecord*> yfs;
        QStringList dontKonws = separateInvoices(invoices,incomes,costs,yss,yfs); //无法识别的发票号
        if(!dontKonws.isEmpty()){
            myHelper::ShowMessageBoxWarning(tr("在第 %1 组存在不能识别的发票号 %2").arg(gnum).arg(dontKonws.join(",")));
        }
        QString cname;
        if(j->dir == MDIR_J){
            if(!incomes.isEmpty()){
                SubjectNameItem* ni = incomes.first()->ni;
                cname = ni?ni->getShortName():incomes.first()->client;
            }
            else if(!yss.isEmpty()){
                cname = yss.first()->customer->getShortName();
            }
        }
        else{
            if(!costs.isEmpty()){
                SubjectNameItem* ni = costs.first()->ni;
                cname = ni?ni->getShortName():costs.first()->client;
            }
            else if(!yfs.isEmpty()){
                cname = yfs.first()->customer->getShortName();
            }
        }

        //生成银行方分录
        jb = new Journalizing;
        jb->journal = j;
        jb->gnum = gnum;
        jb->numInGroup = 1;
        if(j->dir == MDIR_J){
            jb->summary = tr("收%1运费").arg(cname);
        }
        else{
            jb->summary = tr("付%1运费").arg(cname);
        }
        jb->fsub = fsub;
        jb->ssub = ssub;
        jb->mt = mt;
        jb->value = j->value;
        jb->dir = j->dir;
        if(j->dir == MDIR_J){  //如果是进账
            js<<jb;
            if(!costs.isEmpty() || !yfs.isEmpty()){  //抵扣方在借方
                if(!incomes.isEmpty())
                    js<<genDeductionForIncome(j,gnum,costs,yfs); //成本和应付抵扣收入
                else
                    js<<genDeductionForIncome(j,gnum,costs,yfs,false); //成本和应付抵扣应收
            }
            if(!incomes.isEmpty())
                js<<genBaForIncomeOrCost(j,gnum,incomes);  //收入
            if(!yss.isEmpty())
                js<<genBaForYsOrYf(j,gnum,yss);            //应收
        }
        else{   //如果是付款
            if(!costs.isEmpty())
                js<<genBaForIncomeOrCost(j,gnum,costs);
            if(!yfs.isEmpty())
                js<<genBaForYsOrYf(j,gnum,yfs,false);
            if(!incomes.isEmpty() || !yss.isEmpty()){
                if(!costs.isEmpty())
                    js<<genDeductionForCost(j,gnum,incomes,yss);  //收入和应收抵扣成本
                else
                    js<<genDeductionForCost(j,gnum,incomes,yss,false); //收入和应收抵扣应付
            }
            js<<jb;
        }
        //检查借贷是否平衡，如果不平衡，且任一分录或任一发票有外币金额，则插入汇兑损益分录
        Double jsum,dsum;
        foreach(Journalizing* j, js){
            Double v = j->mt==mmt?j->value:j->value*rates.value(j->mt->code());
            if(j->dir == MDIR_J)
                jsum += v;
            else
                dsum += v;
        }
        if(jsum != dsum){
            bool hasWb = false;
            foreach (Journalizing *j, js) {
                if(j->mt != mmt){
                    hasWb = true;
                    break;
                }
            }
            if(!hasWb && !incomes.isEmpty()){
                foreach(CurInvoiceRecord* r, incomes){
                    if(r->wbMoney != 0){
                        hasWb = true;
                        break;
                    }
                }
            }
            if(!hasWb && !costs.isEmpty()){
                foreach(CurInvoiceRecord* r, costs){
                    if(r->wbMoney != 0){
                        hasWb = true;
                        break;
                    }
                }
            }
            if(!hasWb && !yss.isEmpty()){
                foreach(InvoiceRecord* r, yss){
                    if(r->wmoney != 0){
                        hasWb = true;
                        break;
                    }
                }
            }
            if(!hasWb && !yfs.isEmpty()){
                foreach(InvoiceRecord* r, yfs){
                    if(r->wmoney != 0){
                        hasWb = true;
                        break;
                    }
                }
            }
            if(hasWb){
                jb = new Journalizing;
                jb->journal = j;
                jb->gnum = gnum;
                jb->summary = tr("汇兑损益");
                jb->fsub = cwfySub;
                jb->ssub = hdsySSub;
                jb->mt = mmt;
                jb->dir = MDIR_J;
                jb->value = dsum - jsum;
                js.push_front(jb);
            }
        }
    }
    else{  //没有关联发票的收付款，目前仅原样展现
        //可能的情形，比如银行间划款，取现或存款
        jb = new Journalizing;
        jb->journal = j;
        jb->gnum = gnum;
        jb->numInGroup = 1;
        jb->summary = j->summary.isEmpty()?j->summary:j->remark;
        jb->fsub = fsub;
        jb->ssub = ssub;
        jb->dir = j->dir;
        jb->value = j->value;
        jb->mt = mt;
        jo = new Journalizing;
        jo->journal = j;
        jo->gnum = gnum;
        jo->numInGroup = 2;
        jo->summary = jb->summary;
        jo->dir = (jb->dir==MDIR_J)?MDIR_D:MDIR_J;
        jo->mt = mt;
        jo->value = j->value;
        if(j->dir == MDIR_J)
            js<<jb<<jo;
        else
            js<<jo<<jb;
    }
    for(int i = 0; i<js.count(); ++i)
        js.at(i)->numInGroup = i+1;
    return js;
}


/**
 * 流水账的备注和摘要是否包含指定关键字
 * @param  j   流水账
 * @param  kws 关键字列表
 * @return
 */
bool JournalizingPreviewDlg::isContainKeyword(Journal* j,QStringList kws)
{
    foreach(QString k, kws){
        if(j->summary.contains(k) || j->remark.contains(k))
            return true;        
    }
    return false;
}

/**
 * 判定是否是银行间划款，如是，则返回对端银行相应流水账
 * @param  j 
 * @return   
 */
Journal *JournalizingPreviewDlg::isBankTranMoney(Journal *j)
{
    if(!isContainKeyword(j,kwBankTranMoney))
        return 0;
    int pos; //定位到起始位置
    for(int i = 0; i < js.count(); ++i){
        if(js.at(i) == j){
            pos = i;
            break;
        }
    }
    //1、在流水账的备注中查找是否有银行账号或银行科目名
    SecondSubject* dBankSub = 0;
    QList<int> subIds;  //待匹配的银行科目id列表
    QString bankAcc = extracBankAccount(j->remark);
    if(!bankAcc.isEmpty()){
        foreach(BankAccount* bac, sm->getAccount()->getAllBankAccount()){
            if(bac->accNumber == bankAcc){
                subIds<<bankSub->getChildSub(bac->niObj)->getId();
                break;
            }
        }
    }
    //生成待匹配银行科目id列表（同币种不同银行）
    if(subIds.isEmpty()){
        foreach(SecondSubject* sub,bankSub->getChildSubs()){
            if(!sub->isEnabled())
                continue;
            if(sub->getId() == j->bankId)
                continue;
            Money* mt = sm->getSubMatchMt(sm->getSndSubject(j->bankId));
            if(mt != sm->getSubMatchMt(sub))
                continue;
            subIds<<sub->getId();
        }
    }
    //2、查找同一天，方向相反且金额相同的流水账（对方流水最好也是银行间划款的）
    for(int i = pos+1; i < js.count(); ++i){
        Journal* jd = js.at(i);
        if(!subIds.contains(jd->bankId))
            continue;
        if(j->dir == jd->dir)
            continue;
        if(j->date != jd->date)
            continue;
        if(j->value != jd->value)
            continue;
        //if(!isContainKeyword(jd,kwBankTranMoney))   //是否采用待定
        //    continue;
        return jd;
    }
    return 0;
}

/**
 * 处理银行间划款
 * @param  gnum
 * @param  js 收款方流水账
 * @param  jd 付款方流水账
 * @return
 */
QList<Journalizing*> JournalizingPreviewDlg::processBankTranMoney(int gnum, Journal* js, Journal* jd)
{
    QList<Journalizing*> ls;
    ignoreJs<<jd;
    Money* mt = sm->getSubMatchMt(sm->getSndSubject(js->bankId));
    Journalizing* j = new Journalizing;
    j->journal = js;
    j->gnum = gnum;
    j->numInGroup = 1;
    j->summary = tr("划款");
    j->fsub = bankSub;
    j->ssub = sm->getSndSubject(js->bankId);
    j->dir = js->dir;
    j->mt = mt;
    j->value = js->value;
    ls<<j;
    j = new Journalizing;
    j->journal = js;
    j->gnum = gnum;
    j->numInGroup = 2;
    j->summary = tr("划款");
    j->fsub = bankSub;
    j->ssub = sm->getSndSubject(jd->bankId);
    j->dir = (js->dir==MDIR_J)?MDIR_D:MDIR_J;
    j->mt = mt;
    j->value = js->value;
    ls<<j;
    return ls;
}

/**
 * 判定是否是现金存取款，如是，则返回对端相应流水账
 * @param  j
 * @return   
 */
Journal* JournalizingPreviewDlg::isWithdrawal(Journal* j)
{
    if(!isContainKeyword(j,kwCash))
        return 0;
    int pos; //定位到起始位置
    for(int i = 0; i < js.count(); ++i){
        if(js.at(i) == j){
            pos = i;
            break;
        }
    }
    SecondSubject* rmbSub = cashSub->getChildSubs().first();
    for(int i = pos+1; i<js.count(); ++i){
        Journal* j2 = js.at(i);
        if(j->date != j2->date)
            continue;
        if(j2->bankId == rmbSub->getId())
            continue;
        if(j2->dir == MDIR_J)
            continue;
        if(j->value != j2->value)
            continue;
        if(!isContainKeyword(j2,kwCash))
            continue;
        return j2;
    }
    return 0;
}

/**
 * 处理现金存取款
 * @param  gnum
 * @param  js 收款方流水账（现金进账）
 * @param  jd 付款方流水账（银行出账）
 * @return
 */
QList<Journalizing*> JournalizingPreviewDlg::processCash(int gnum, Journal* js, Journal* jd)
{
    QList<Journalizing*> ls;
    ignoreJs<<jd;
    Journalizing* j = new Journalizing;
    j->journal = js;
    j->gnum = gnum;
    j->numInGroup = 1;
    j->summary = tr("提现");
    j->fsub = cashSub;
    j->ssub = cashSub->getChildSubs().first();
    j->dir = MDIR_J;
    j->mt = mmt;
    j->value = js->value;
    ls<<j;
    j = new Journalizing;
    j->journal = jd;
    j->gnum = gnum;
    j->numInGroup = 2;
    j->summary = tr("提现");
    j->dir = MDIR_D;
    j->fsub = bankSub;
    j->ssub = sm->getSndSubject(jd->bankId);
    j->mt = mmt;
    j->value = js->value;
    ls<<j;
    return ls;
}

/**
 * @brief 判定是否是购汇或结汇
 * @param j
 * @return
 */
Journal *JournalizingPreviewDlg::isBugForex(Journal *j)
{
    if(!isContainKeyword(j,kwBuyForex))
        return 0;
    int pos = indexOf(j)+1;
    SecondSubject* bSSub_s = sm->getSndSubject(j->bankId);
    QString bankName = bSSub_s->getName().split("-")[0];
    Money* mt = sm->getSubMatchMt(bSSub_s);
    SecondSubject* bSSub_d = 0;
    foreach (SecondSubject* ssub, bankSub->getChildSubs()) {
        QString name = ssub->getName().split("-")[0];
        if(name == bankName && mt != sm->getSubMatchMt(ssub)){
            bSSub_d = ssub;
            break;
        }
    }
    Journal* jo;
    QList<Journal*> jos;
    for(int i = pos; i < js.count(); ++i){
        jo = js.at(i);
        if(jo->bankId != bSSub_d->getId())
            continue;
        if(jo->dir == j->dir)
            continue;
        if(jo->date != j->date)
            continue;
        jos<<jo;
    }
    if(jos.isEmpty())
        return 0;
    if(jos.count()==1)
        return jos.first();
    QDialog dlg(this);
    QLabel l1(tr("请选择对应出账金额"));
    QLabel l2(tr("购汇金额"),&dlg);
    QLineEdit edtUsd(j->value.toString(),&dlg);
    QHBoxLayout* lt = new QHBoxLayout;
    lt->addWidget(&l2);
    lt->addWidget(&edtUsd);
    QTableWidget tw;
    tw.setSelectionBehavior(QAbstractItemView::SelectRows);
    tw.setEditTriggers(QAbstractItemView::NoEditTriggers);
    tw.setColumnCount(2);
    tw.setRowCount(jos.count());
    QStringList titles;
    titles<<"ID"<<tr("金额");
    tw.setHorizontalHeaderLabels(titles);
    for(int i=0; i<jos.count(); ++i){
        Journal* j = jos.at(i);
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(j->id));
        QVariant v;
        v.setValue<Journal*>(j);
        item->setData(Qt::UserRole,v);
        tw.setItem(i,0,item);
        tw.setItem(i,1,new QTableWidgetItem(j->value.toString()));
    }
    QPushButton btnOk(tr("确定"),&dlg);
    QPushButton btnCancel(tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout *lb = new QHBoxLayout;
    lb->addStretch();
    lb->addWidget(&btnOk);
    lb->addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&l1);
    lm->addLayout(lt);
    lm->addWidget(&tw);
    lm->addLayout(lb);
    dlg.setLayout(lm);
    dlg.resize(300,500);
    if(dlg.exec() == QDialog::Rejected)
        return 0;
    int row = tw.currentRow();
    if(row == -1)
        return 0;
    jo = tw.item(row,0)->data(Qt::UserRole).value<Journal*>();
    return jo;
}

/**
 * @brief 处理购汇或结汇
 * @param gid
 * @param gnum
 * @param js
 * @param jd
 * @return
 */
QList<Journalizing *> JournalizingPreviewDlg::processBuyForex(int gnum, Journal *js, Journal *jd)
{
    QList<Journalizing *> ls;
    ignoreJs<<jd;
    SecondSubject *ssub_s = sm->getSndSubject(js->bankId);
    SecondSubject *ssub_d = sm->getSndSubject(jd->bankId);
    Journalizing* jo_hdsy = new Journalizing;
    jo_hdsy->journal = js;
    jo_hdsy->gnum = gnum;
    jo_hdsy->numInGroup = 1;
    jo_hdsy->summary = tr("汇兑损益");
    jo_hdsy->fsub = cwfySub;
    jo_hdsy->ssub = hdsySSub;
    jo_hdsy->dir = MDIR_J;
    jo_hdsy->mt = mmt;
    QHash<Money*, Double> rates;
    sm->getAccount()->getRates(year,month,rates);
    Double vs = js->value;
    if(js->mt != mmt)
        vs *= rates.value(js->mt);
    Double vd = jd->value;
    if(jd->mt != mmt)
        vd *= rates.value(jd->mt);
    if(js->dir == MDIR_J)
        jo_hdsy->value = vd - vs;
    else
        jo_hdsy->value = vs - vd;
    ls<<jo_hdsy;
    Journalizing* jo = new Journalizing;
    jo->journal = js;
    jo->gnum = gnum;
    jo->numInGroup = 2;
    if(js->dir == MDIR_J && js->mt != mmt){
        jo->summary = tr("购汇");
        jo->fsub = bankSub;
        jo->ssub = ssub_s;
        jo->dir = js->dir;
        jo->value = js->value;
        jo->mt = js->mt;
    }
    else{
        jo->summary = tr("结汇");
        jo->fsub = bankSub;
        jo->ssub = ssub_d;
        jo->dir = jd->dir;
        jo->value = jd->value;
        jo->mt = jd->mt;
    }
    ls<<jo;
    jo = new Journalizing;
    jo->gnum = gnum;
    jo->journal = js;
    jo->numInGroup = 3;
    jo->gnum = gnum;
    jo->summary = js->dir==MDIR_D?tr("结汇"):tr("购汇");
    jo->fsub = bankSub;
    jo->ssub = ssub_d;
    jo->dir = jd->dir;
    jo->mt = jd->mt;
    jo->value = jd->value;
    ls<<jo;
    return ls;
}

/**
 * 是否费用类支出
 * @param  j 
 * @return   
 */
SecondSubject *JournalizingPreviewDlg::isFee(Journal* j)
{
    if(j->dir == MDIR_J)
        return 0;
    SecondSubject* ssub;
    if(glfySub->isHaveSmartAdapte()){
        ssub = glfySub->getAdapteSSub(j->summary);
        if(ssub)
            return ssub;
        ssub = glfySub->getAdapteSSub(j->remark);
        if(ssub)
            return ssub;
    }
    if(cwfySub->isHaveSmartAdapte()){
        if(isContainKeyword(j,kwServiceFee)){
            ssub = cwfySub->getAdapteSSub(tr("手续费"));
            return ssub;
        }
        ssub = cwfySub->getAdapteSSub(j->summary);
        if(ssub)
            return ssub;
        ssub = cwfySub->getAdapteSSub(j->remark);
        if(ssub)
            return ssub;
    }
    if(xsfySub->isHaveSmartAdapte()){
        ssub = xsfySub->getAdapteSSub(j->summary);
        if(ssub)
            return ssub;
        ssub = xsfySub->getAdapteSSub(j->remark);
        if(ssub)
            return ssub;
    }
    return 0;
}

/**
 * 处理费用类支出
 */
QList<Journalizing*> JournalizingPreviewDlg::processFee(int gnum, Journal* jd, SecondSubject* ssub)
{
    QList<Journalizing*> ls;
    Journalizing* j = new Journalizing;
    j->gnum = gnum;
    j->journal = jd;
    j->numInGroup = 1;
    j->summary = jd->summary.isEmpty()?jd->remark:jd->summary;
    j->fsub = ssub->getParent();
    j->ssub = ssub;
    j->dir = MDIR_J;
    if(jd->mt != mmt)
        j->value = jd->value * rates.value(jd->mt->code());
    else
        j->value = jd->value;
    j->mt = mmt;
    ls<<j;
    j = new Journalizing;
    j->gnum = gnum;
    j->journal = jd;
    j->numInGroup = 2;
    j->summary = ls.last()->summary;
    j->ssub = sm->getSndSubject(jd->bankId);
    j->fsub = j->ssub->getParent();    
    j->dir = MDIR_D;
    j->value = jd->value;
    if(j->fsub == cashSub)
        j->mt = mmt;
    else
        j->mt = sm->getSubMatchMt(j->ssub);
    ls<<j;
    return ls;
}

/**
 * 是否是税金类支出项
 * @param  j [description]
 * @return   [description]
 */
SecondSubject *JournalizingPreviewDlg::isTax(Journal* j)
{
    SecondSubject* ssub=0;
    if(j->dir != MDIR_D || sm->getSndSubject(j->bankId)->getParent() != bankSub)
        return 0;
    if(glfySub->isHaveSmartAdapte()){
        ssub = glfySub->getAdapteSSub(j->summary);
        if(ssub)
            return ssub;
        else if(!j->remark.isEmpty()){
            ssub = glfySub->getAdapteSSub(j->remark);
            if(ssub)
                return ssub;
        }
    }
    if(yjsjSub->isHaveSmartAdapte()){
        ssub = yjsjSub->getAdapteSSub(j->summary);
        if(ssub)
            return ssub;
        else if(!j->remark.isEmpty()){
            ssub = yjsjSub->getAdapteSSub(j->remark);
            if(ssub)
                return ssub;
        }
    }
    return 0;
}

/**
 * 处理税金类支出
 */
QList<Journalizing*> JournalizingPreviewDlg::processTax(int gnum, Journal* jd, SecondSubject* ssub)
{
    QList<Journalizing*> ls;
    Journalizing *j = new Journalizing;
    j->gnum = gnum;
    j->journal = jd;
    j->numInGroup = 1;
    j->summary = jd->summary.isEmpty()?jd->remark:jd->summary;
    j->ssub = ssub;
    j->fsub = ssub->getParent();
    j->dir  = MDIR_J;
    j->value = jd->value;
    j->mt = mmt;
    ls<<j;
    j = new Journalizing;
    j->gnum = gnum;
    j->journal = jd;
    j->numInGroup = 2;
    j->summary = ls.last()->summary;
    j->ssub = sm->getSndSubject(jd->bankId);
    j->fsub = j->ssub->getParent();
    j->dir  = MDIR_D;
    j->value = jd->value;
    j->mt = mmt;
    ls<<j;
    return ls;
}

/**
 * @brief 判定是否是支付工资
 * @param j
 * @return
 */
SecondSubject *JournalizingPreviewDlg::isSalary(Journal *j)
{
    SecondSubject* ssub=0;
    if(j->dir == MDIR_D && isContainKeyword(j,kwSalary))
        ssub = gzSub->getDefaultSubject();
    return ssub;
}

/**
 * @brief 处理支付工资
 * @param gnum
 * @param jd
 * @param ssub
 * @return
 */
QList<Journalizing *> JournalizingPreviewDlg::processSalary(int gnum, Journal *jd, SecondSubject *ssub)
{
    QList<Journalizing *> ls;
    Journalizing* j = new Journalizing;
    j->journal = jd;
    j->gnum = gnum;
    j->numInGroup = 1;
    j->summary = jd->summary.isEmpty()?jd->remark:jd->summary;
    j->fsub = gzSub;
    j->ssub = ssub;
    j->dir = MDIR_J;
    j->value = jd->value;
    j->mt = mmt;
    ls<<j;
    j = new Journalizing;
    j->journal = jd;
    j->gnum = gnum;
    j->numInGroup = 2;
    j->summary = ls.last()->summary;
    j->ssub = sm->getSndSubject(jd->bankId);
    j->fsub = j->ssub->getParent();
    j->dir = MDIR_D;
    j->mt = mmt;
    j->value = jd->value;
    ls<<j;
    return ls;
}

/**
 * @brief 是否是国外账
 * @param j
 * @return
 */
bool JournalizingPreviewDlg::isOversea(Journal *j)
{
    if(j->dir != MDIR_J || !isContainKeyword(j,kwOversea))
        return false;
    return true;
}

/**
 * @brief 处理国外账
 * @param gnum
 * @param jd
 * @return
 */
QList<Journalizing *> JournalizingPreviewDlg::processOversea(int gnum, Journal *jd)
{
    QList<Journalizing *> ls;
    Journalizing *j = new Journalizing;
    j->journal = jd;
    j->gnum = gnum;
    j->numInGroup = 1;
    j->summary = jd->summary.isEmpty()?jd->remark:jd->summary;
    j->dir = jd->dir;
    j->fsub = bankSub;
    j->ssub = sm->getSndSubject(jd->bankId);
    j->mt = jd->mt;
    j->value = jd->value;
    ls<<j;
    j = new Journalizing;
    j->journal = jd;
    j->gnum = gnum;
    j->numInGroup = 2;
    j->summary = ls.last()->summary;
    j->fsub = zysrSub;
    j->ssub = zysrSub->getDefaultSubject();
    j->dir = MDIR_D;
    j->mt = mmt;
    if(jd->mt != mmt)
        j->value = jd->value * rates.value(jd->mt->code());
    else
        j->value = jd->value;
    ls<<j;
    return ls;
}


/**
 * 
 */
QList<Journalizing *> JournalizingPreviewDlg::processOther(int gnum, Journal *jd, bool &processed)
{
    QList<Journalizing *>ls;
    //投资款
    if(isContainKeyword(jd,kwInvest)){
        Journalizing *j1 = new Journalizing;
        Journalizing *j2 = new Journalizing;
        j1->journal = jd;
        j2->journal = jd;
        j1->gnum = gnum;
        j2->gnum = gnum;
        if(jd->dir == MDIR_J){
            j1->numInGroup = 1;
            j1->fsub = bankSub;
            j1->ssub = sm->getSndSubject(jd->bankId);
            j1->summary = jd->summary.isEmpty()?jd->remark:jd->summary;
            j1->mt = jd->mt;
            j1->value = jd->value;
            j1->dir = MDIR_J;
            j2->numInGroup = 2;
            j2->summary = j1->summary;
            j2->fsub = sszbSub;
            j2->ssub = 0;
            j2->mt = jd->mt;
            j2->dir = MDIR_D;
            j2->value = jd->value;
            ls<<j1<<j2;
        }
        else{
            j1->numInGroup = 2;
            j1->fsub = bankSub;
            j1->ssub = sm->getSndSubject(jd->bankId);
            j1->summary = jd->summary.isEmpty()?jd->remark:jd->summary;
            j1->mt = jd->mt;
            j1->value = jd->value;
            j1->dir = MDIR_D;
            j2->numInGroup = 1;
            j2->summary = j1->summary;
            j2->fsub = sszbSub;
            j2->ssub = 0;
            j2->mt = jd->mt;
            j2->dir = MDIR_J;
            j2->value = jd->value;
            ls<<j2<<j1;
        }
        processed = true;
        return ls;
    }
    //预收预付
    if(isContainKeyword(jd,kwPreInPay)){
        //按约定，在摘要栏写客户名，在备注栏写预收、预付关键字信息
        if(nameItems.isEmpty())
            nameItems = sm->getAllNameItems(SORTMODE_NAME);
        if(isolatedAlias.isEmpty())
            isolatedAlias = sm->getAllIsolatedAlias();
        SubjectNameItem* ni = 0;
        QString cname = jd->summary.trimmed();
        foreach(SubjectNameItem* n, nameItems){
            if(n->matchName(cname) == 0)
                continue;
            ni = n;
            break;
        }
        if(!ni){
            foreach(NameItemAlias* alias, isolatedAlias){
                if(alias->longName() == cname){
                    int clientClsId = AppConfig::getInstance()->getSpecNameItemCls(AppConfig::SNIC_COMMON_CLIENT);
                    ni = new SubjectNameItem(0,clientClsId,alias->shortName(),alias->longName(),alias->rememberCode(),alias->createdTime(),curUser);
                    break;
                }
            }
        }
        
        Journalizing *j1 = new Journalizing;
        Journalizing *j2 = new Journalizing;
        j1->journal = jd;
        j2->journal = jd;
        j1->gnum = gnum;
        j2->gnum = gnum;
        if(jd->dir == MDIR_J){
            j1->numInGroup = 1;
            j1->fsub = bankSub;
            j1->ssub = sm->getSndSubject(jd->bankId);
            if(ni)
                j1->summary = tr("预收%1运费").arg(ni->getShortName());
            else
                j1->summary = jd->summary.isEmpty()?jd->remark:jd->summary;
            j1->mt = jd->mt;
            j1->value = jd->value;
            j1->dir = MDIR_J;
            j2->numInGroup = 2;
            j2->summary = j1->summary;
            j2->fsub = ysSub;
            if(ni){
                j2->ssub = ysSub->getChildSub(ni);
                if(!j1->ssub) //客户名有对应名称条目，但没有对应子目，或客户名匹配账户内的孤立别名
                    j1->ssub = ysSub->addChildSub(ni);
            }
            j2->mt = jd->mt;
            j2->dir = MDIR_D;
            j2->value = jd->value;
            ls<<j1<<j2;
        }
        else{
            j1->numInGroup = 2;
            j1->fsub = bankSub;
            j1->ssub = sm->getSndSubject(jd->bankId);
            if(ni)
                j1->summary = j1->summary = tr("预付%1运费").arg(ni->getShortName());
            else
                j1->summary = jd->summary.isEmpty()?jd->remark:jd->summary;
            j1->mt = jd->mt;
            j1->value = jd->value;
            j1->dir = MDIR_D;
            j2->numInGroup = 1;
            j2->summary = j1->summary;
            j2->fsub = yfSub;
            if(ni){
                j2->ssub = yfSub->getChildSub(ni);
                if(!j2->ssub)
                    j2->ssub = yfSub->addChildSub(ni);
            }
            j2->mt = jd->mt;
            j2->dir = MDIR_J;
            j2->value = jd->value;
            ls<<j2<<j1;
        }
        processed = true;
        return ls;
    }
    
}

/**
 * @brief 处理无法辨识的流水账
 * @param gnum
 * @param jd
 * @return
 */
Journalizing *JournalizingPreviewDlg::processUnkownJournal(int gnum, Journal *jd)
{
    SecondSubject* ssub = sm->getSndSubject(jd->bankId);
    FirstSubject* fsub = ssub->getParent();
    Journalizing* j = new Journalizing;
    if(fsub == cashSub)
        j->mt = mmt;
    else
        j->mt = sm->getSubMatchMt(ssub);
    j->numInGroup = 1;
    j->journal = jd;
    j->gnum = gnum;
    j->summary = QString("%1 备注：%2").arg(jd->summary).arg(jd->remark);
    j->fsub = fsub;
    j->ssub = ssub;
    j->dir = jd->dir;
    j->value = jd->value;
    return j;
}


/**
 * 为收入或成本发票创建分录
 */
QList<Journalizing*> JournalizingPreviewDlg::genBaForIncomeOrCost(Journal *jo, int gnum, QList<CurInvoiceRecord*> ls)
{
    QList<Journalizing*> js;
    foreach(CurInvoiceRecord* r,ls){
        QString cliName;
        cliName = r->ni?r->ni->getShortName():r->client;
        Journalizing* j = new Journalizing;
        j->journal = jo;
        j->gnum = gnum;        
        if(r->isIncome){
            incomes.removeOne(r);
            j->summary = tr("收%1运费 %2").arg(cliName).arg(r->inum);
            j->fsub = zysrSub;
            j->ssub = zysrSub->getDefaultSubject();
            j->dir = MDIR_D;
        }
        else{
            costs.removeOne(r);
            j->summary = tr("付%1运费 %2").arg(cliName).arg(r->inum);
            j->fsub = zycbSub;
            j->ssub = zycbSub->getDefaultSubject();
            j->dir = MDIR_J;
        }
        if(r->wbMoney != 0)
            j->summary += tr("（$%1）").arg(r->wbMoney.toString());
        j->mt = mmt;
        j->value = r->money;
        js<<j;
        if(r->type){  //专票
            j->value = r->money - r->taxMoney;
            j = new Journalizing;
            j->journal = jo;
            j->gnum = gnum;        
            j->mt = mmt;
            j->value = r->taxMoney;
            if(r->isIncome){
                j->summary = tr("收%1运费 %2").arg(cliName).arg(r->inum);
                j->fsub = yjsjSub;
                j->ssub = xxseSSub;
                j->dir = MDIR_D;
            }
            else{
                j->summary = tr("付%1运费 %2").arg(cliName).arg(r->inum);
                j->fsub = yjsjSub;
                j->ssub = jxseSSub;
                j->dir = MDIR_J;
            }
            js<<j;
        }
    }
    return js;
}

/**
 * @brief 为一组应收或应付发票创建一条分录
 * @param ls
 * @return
 */
Journalizing* JournalizingPreviewDlg::genBaForYsOrYf(Journal* jo, int gnum, QList<InvoiceRecord *> ls, bool isYs)
{
    if(ls.isEmpty())
        return 0;
    SubjectNameItem* ni = ls.first()->customer;
    Journalizing* j = new Journalizing;
    j->journal = jo;
    j->gnum = gnum;        
    j->mt = mmt;
    QString sOrf;
    if(isYs){
        j->dir = MDIR_D;
        j->fsub = ysSub;
        j->ssub = ysSub->getChildSub(ni);
        sOrf = tr("收");
    }
    else{
        j->dir = MDIR_J;
        j->fsub = yfSub;
        j->ssub = yfSub->getChildSub(ni);
        sOrf = tr("付");
    }
    Double v,wv;
    foreach(InvoiceRecord* r, ls){
        v += r->money;
        wv += r->wmoney;
    }
    j->value = v;
    QString strInvoices = genTerseInvoiceNums(ls); //生成应收或应付发票号串
    j->summary = tr("%1%2运费 %3").arg(sOrf).arg(ni->getShortName()).arg(strInvoices);
    if(wv != 0)
        j->summary += tr("（$%1）").arg(wv.toString());
    return j;
}

/**
 * 生成成本或应付抵扣收入或应收的分录
 * @param  costs 成本发票
 * @param  yfs   应付发票
 * @param  inOrYs true：收入，false：应收
 * @return       生成的分录列表
 */
QList<Journalizing*> JournalizingPreviewDlg::genDeductionForIncome(Journal* jo, int gnum, QList<CurInvoiceRecord*> costs,QList<InvoiceRecord*> yfs, bool inOrYs)
{
    QList<Journalizing*> js;
    QString strInOrYf;
    if(inOrYs)
        strInOrYf = tr("收入");
    else
        strInOrYf = tr("应收");
    foreach(CurInvoiceRecord* i, costs){
        this->costs.removeOne(i);
        Journalizing* j = new Journalizing;
        j->journal = jo;
        j->gnum = gnum;        
        j->fsub = zycbSub;
        j->ssub = zycbSub->getDefaultSubject();
        j->dir = MDIR_J;
        j->value = i->money;
        j->mt = mmt;
        j->summary = tr("%1成本抵扣%2 %3").arg(i->ni?i->ni->getShortName():i->client).arg(strInOrYf).arg(i->inum);
        if(i->wbMoney != 0)
            j->summary += tr("（$%1）").arg(i->wbMoney.toString());
        js<<j;
    }
    if(!yfs.isEmpty()){
        Double v,wv;
        QString strInvoices = genTerseInvoiceNums(yfs);
        foreach(InvoiceRecord* i, yfs){
            v += i->money;
            wv += i->wmoney;
        } 
        Journalizing* j = new Journalizing;
        j->journal = jo;
        j->gnum = gnum;        
        j->mt = mmt;
        j->value = v;
        j->dir = MDIR_J;
        j->fsub = yfSub;
        QString cname = yfs.first()->customer?yfs.first()->customer->getShortName():"";
        j->ssub = yfSub->getChildSub(yfs.first()->customer);
        j->summary = tr("%1应付抵扣%2 %3").arg(cname).arg(strInOrYf).arg(strInvoices);
        if(wv != 0)
            j->summary += tr("（$%1）").arg(wv.toString());
        js<<j;
    }
    return js;
}

/**
 * 生成收入或应收抵扣成本或应付的分录
 * @param  incomes 收入发票
 * @param  yss   应收发票
 * @param  costOrYf true：成本，false：应付
 * @return       生成的分录列表
 */
QList<Journalizing*> JournalizingPreviewDlg::genDeductionForCost(Journal* jo, int gnum, QList<CurInvoiceRecord*> incomes,QList<InvoiceRecord*> yss, bool costOrYf)
{
    QList<Journalizing*> js;
    QString strCostOrYs;
    if(costOrYf)
        strCostOrYs = tr("成本");
    else
        strCostOrYs = tr("应付");
    foreach(CurInvoiceRecord* i, incomes){
        this->incomes.removeOne(i);
        Journalizing* j = new Journalizing;
        j->journal = jo;
        j->gnum = gnum;        
        j->fsub = zysrSub;
        j->ssub = zysrSub->getDefaultSubject();
        j->dir = MDIR_D;
        j->value = i->money;
        j->mt = mmt;
        j->summary = tr("%1收入抵扣%2 %3").arg(i->ni?i->ni->getShortName():i->client).arg(strCostOrYs).arg(i->inum);
        if(i->wbMoney != 0)
            j->summary += tr("（$%1）").arg(i->wbMoney.toString());
        js<<j;
    }
    if(!yss.isEmpty()){
        Double v,wv;
        QString strInvoices = genTerseInvoiceNums(yss);
        foreach(InvoiceRecord* i, yss){
            v += i->money;
            wv += i->wmoney;
        } 
        Journalizing* j = new Journalizing;
        j->journal = jo;
        j->gnum = gnum;        
        j->mt = mmt;
        j->value = v;
        j->dir = MDIR_D;
        j->fsub = ysSub;
        QString cname = yss.first()->customer?yss.first()->customer->getShortName():"";
        j->ssub = ysSub->getChildSub(yss.first()->customer);
        j->summary = tr("%1应收抵扣%2 %3").arg(cname).arg(strCostOrYs).arg(strInvoices);
        if(wv != 0)
            j->summary += tr("（$%1）").arg(wv.toString());
        js<<j;
    }
    return js;
}

/**
 * 生成收入或成本的聚合分录组
 * 将普票和专票分开，先普后专
 * @param  true：收入，false：成本
 * @return
 */
QList<Journalizing*> JournalizingPreviewDlg::genGatherBas(int gnum, bool isIncome)
{
    //首先要对遗留的发票列表进行排序，按发票类别和发票号的顺序，要支持这个，
    //CurInvoiceRecord结构类型必须支持排序接口
    QList<Journalizing*> js,rjs;
    QList<CurInvoiceRecord*> *ls;
    if(isIncome){
        ls = &incomes;
        qSort(ls->begin(),ls->end(),incomeInvoiceThan);  //收入发票按发票号的顺序
    }
    else{
        ls = &costs;
        qSort(ls->begin(),ls->end(),costInvoiceThan);  //成本发票按客户名、发票号的顺序
    }
    if(ls->isEmpty())
        return js;
     
    int startPos=-1;  //专票开始索引位
    for(int i=0; i<ls->count(); ++i){
        if(ls->at(i)->type){
            startPos = i;
            break;
        }        
    }
    Double sum;
    CurInvoiceRecord* r;
    Journalizing *j1,*j2;
    QString summary;
    SecondSubject* ssub;
    //普票
    if(!ls->isEmpty()){
        for(int i=0; i<startPos; ++i){
            r = ls->at(i);
            if(r->state == 2)
                continue;
            //sum += r->money;
            j1 = new Journalizing;
            j1->journal = isIncome?jsr_p:jcb_p;
            j1->numInGroup = i+1;
            j1->value = r->money;
            j1->mt = mmt;
            QString cname = r->ni?r->ni->getShortName():r->client;
            if(isIncome){
                j1->dir = MDIR_J;
                j1->fsub = ysSub;
                ssub = ysSub->getChildSub(r->ni);
                if(!ssub)
                    ssub = ysSub->addChildSub(r->ni);
                j1->ssub = ssub;
                if(r->wbMoney != 0)
                    j1->summary = tr("应收%1运费 %2（$%3）").arg(cname).arg(r->inum).arg(r->wbMoney.toString());
                else
                    j1->summary = tr("应收%1运费 %2").arg(cname).arg(r->inum);
            }
            else{
                j1->dir = MDIR_D;
                j1->fsub = yfSub;
                ssub = yfSub->getChildSub(r->ni);
                if(!ssub)
                    ssub = ysSub->addChildSub(r->ni);
                j1->ssub = ssub;
                if(r->wbMoney != 0)
                    j1->summary = tr("应付%1运费 %2（$%3）").arg(cname).arg(r->inum).arg(r->wbMoney.toString());
                else
                    j1->summary = tr("应付%1运费 %2").arg(cname).arg(r->inum);
            }
            js<<j1;
        }        
    }
    //对分录进行分组，避免过多的分录数目
    QList<Journalizing*> ts;
    while(!js.isEmpty()){
        int nums = js.count();
        int ei=0;
        if(nums < 63)
            ei = js.count();
        else if(nums < 96)
            ei = 47;
        else
            ei = 63;
        for(int i = 0; i < ei; ++i){
            Journalizing* jo = js.takeAt(0);
            jo->gnum = gnum;
            sum += jo->value;
            ts<<jo;
        }
        j2 = new Journalizing;
        j2->journal = isIncome?jsr_p:jcb_p;
        j2->gnum = gnum;
        j2->mt = mmt;
        j2->value = sum;
        if(isIncome){
            j2->summary = tr("应收运费");
            j2->fsub = zysrSub;
            j2->ssub = zysrSub->getDefaultSubject();
            j2->dir = MDIR_D;
            ts<<j2;
        }
        else{
            j2->summary = tr("应付运费");
            j2->fsub = zycbSub;
            j2->ssub = zycbSub->getDefaultSubject();
            j2->dir = MDIR_J;
            ts.push_front(j2);
        }
        for(int i = 0; i < ts.count(); ++i)
            ts.at(i)->numInGroup = i+1;
        rjs<<ts;
        ts.clear();
        sum = 0;
        gnum++;
    }
    //专票
    int numInGroup = 0;
    if(startPos != -1){
        for(int i=startPos; i<ls->count(); ++i){
            r = ls->at(i);
            if(r->state == 2)
                continue;
            numInGroup++;
            QString cname = r->ni?r->ni->getShortName():r->client;
            sum += r->money - r->taxMoney;
            j1 = new Journalizing;
            j1->journal = isIncome?jsr_z:jcb_z;
            j2 = new Journalizing;
            j2->journal = isIncome?jsr_z:jcb_z;
            if(isIncome){                
                if(r->wbMoney > 0)
                    j1->summary = tr("应收%1运费 %2（$%3）").arg(cname).arg(r->inum).arg(r->wbMoney.toString());
                else
                    j1->summary = summary = tr("应收%1运费 %2").arg(cname).arg(r->inum);
                j1->fsub = ysSub;
                ssub = ysSub->getChildSub(r->ni);
                if(!ssub)
                    ssub = ysSub->addChildSub(r->ni);
                j1->ssub = ssub;
                j1->dir = MDIR_J;  
                j1->mt = mmt;
                j1->value = r->money;
                js<<j1;
                j2->summary = tr("应收%1运费 %2").arg(cname).arg(r->inum);
                j2->fsub = yjsjSub;
                j2->ssub = xxseSSub;
                j2->mt = mmt;
                j2->value = r->taxMoney;
                j2->dir = MDIR_D;
                js<<j2;
            }
            else{
                if(r->wbMoney > 0)
                    j1->summary = tr("应付%1运费 %2（$%3）").arg(cname).arg(r->inum).arg(r->wbMoney.toString());
                else
                    j1->summary = summary = tr("应付%1运费 %2").arg(cname).arg(r->inum);
                j1->fsub = yfSub;
                ssub = yfSub->getChildSub(r->ni);
                if(!ssub)
                    ssub = yfSub->addChildSub(r->ni);
                j1->ssub = ssub;
                j1->dir = MDIR_D;
                j1->mt = mmt;
                j1->value = r->money;
                j2->summary = tr("应付%1运费 %2").arg(cname).arg(r->inum);
                j2->fsub = yjsjSub;
                j2->ssub = jxseSSub;
                j2->mt = mmt;
                j2->value = r->taxMoney;
                j2->dir = MDIR_J;
                js<<j2;
                js<<j1;
            }            
        }
        Double jv,dv;
        while(!js.isEmpty()){
            int nums = js.count();
            int ei = 0;
            if(nums < 61)
                ei = js.count();
            else if(nums < 93)
                ei = 45;
            else
                ei = 61;
            int i = 0;
            for(i; i < ei; ++i){
                Journalizing* jo = js.takeAt(0);
                jo->gnum = gnum;
                if(jo->dir == MDIR_J)
                    jv += jo->value;
                else
                    dv += jo->value;
                ts<<jo;
            }
            j1 = new Journalizing;
            j1->gnum = gnum;
            j1->journal = isIncome?jsr_z:jcb_z;
            j1->mt = mmt;
            j1->value = isIncome?jv-dv:dv-jv;
            if(isIncome){
                j1->summary = tr("应收运费");
                j1->fsub = zysrSub;
                j1->ssub = zysrSub->getDefaultSubject();
                j1->dir = MDIR_D;
                ts<<j1;
            }
            else{
                j1->summary = tr("应付运费");
                j1->fsub = zycbSub;
                j1->ssub = zycbSub->getDefaultSubject();
                j1->dir = MDIR_J;
                ts.push_front(j1);
            }
            for(int i = 0; i < ts.count(); ++i)
                ts.at(i)->numInGroup = i+1;
            rjs<<ts;
            gnum++;
            sum = 0;jv=0;dv=0;
            ts.clear();
        }
    }
    return rjs;
}

/**
 * 
 * @return 
 */
bool JournalizingPreviewDlg::createPzs()
{
    int curPzNum = 0;
    Journalizing* j;
    PingZheng* pz=0;
    BusiAction* ba;
    QList<BusiAction*> bas;
    QDate d = QDate(year,month,1);
    QString endDay = QDate(year,month,d.daysInMonth()).toString(Qt::ISODate);
    for(int i=0; i<jls.count(); ++i){
        j = jls.at(i);
        if(j->pnum != curPzNum){
            if(pz){
                pz->setBaList(bas);
                bas.clear();
            }
            curPzNum = j->pnum;
            pz = new PingZheng(smg);
            pz->setRecordUser(curUser);
            pz->setNumber(curPzNum);
            pz->setDate(j->journal?j->journal->date:endDay);
            smg->append(pz);
        }
        ba = new BusiAction();
        ba->setParent(pz);
        ba->setSummary(j->summary);
        ba->setFirstSubject(j->fsub);
        ba->setSecondSubject(j->ssub);
        ba->setMt(j->mt,j->value);
        ba->setDir(j->dir);
        bas<<ba;
    }
    if(!bas.isEmpty()){
        pz->setBaList(bas);
    }
}

/**
 * @brief 定位流水帐对象在js列表中的索引位置
 * @param j
 * @return
 */
int JournalizingPreviewDlg::indexOf(Journal *j)
{
    for(int i = 0; i < js.count(); ++i){
        if(js.at(i) == j)
            return i;
    }
    return -1;
}

SecondSubject *JournalizingPreviewDlg::getAdapterSSub(FirstSubject *fsub, QString summary, QString prefixe, QString suffix)
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
 * @brief JournalizingPreviewDlg::updateBas 会计分录的局部更新
 * 此方法应用于当凭证的会计分录在程序内部被改变而不是由用户的编辑动作而引起的改变时，
 * 更新显示凭证的最新内容
 * @param row：开始行
 * @param rows：要更新的行数
 * @param col： 要更新的列
 */
void JournalizingPreviewDlg::updateBas(int row, int rows, BaUpdateColumns col)
{
    if(row < 0 || row > jls.count()-1 || row+rows > jls.count())
        return;
    if(!col)
        return;

    turnWatchData(false);
    QVariant v;
    //更新所有列
    if(col.testFlag(BUC_ALL)){
        for(int i = row; i < row+rows; ++i){
            Journalizing* ba = jls.at(i);
            ui->twJos->item(i,JCI_SUMMARY)->setData(Qt::EditRole,ba->summary);
            v.setValue(ba->fsub);
            ui->twJos->item(i,JCI_FSUB)->setData(Qt::EditRole,v);
            v.setValue(ba->ssub);
            ui->twJos->item(i,JCI_SSUB)->setData(Qt::EditRole,v);
            v.setValue(ba->mt);
            ui->twJos->item(i,JCI_MTYPE)->setData(Qt::EditRole,v);
            v.setValue(ba->value);
            if(ba->dir == MDIR_J){
                ui->twJos->item(i,JCI_JV)->setData(Qt::EditRole,v);
                ui->twJos->item(i,JCI_DV)->setData(Qt::EditRole,0);
            }
            else{
                ui->twJos->item(i,JCI_JV)->setData(Qt::EditRole,0);
                ui->twJos->item(i,JCI_DV)->setData(Qt::EditRole,v);
            }
        }
    }
    else{
        if(col.testFlag(BUC_SUMMARY)){
            for(int i = row; i < row+rows; ++i){
                ui->twJos->item(i,JCI_SUMMARY)->
                        setData(Qt::EditRole,jls.at(i)->summary);
            }
        }
        if(col.testFlag(BUC_FSTSUB)){
            for(int i = row; i < row+rows; ++i){
                v.setValue(jls.at(i)->fsub);
                ui->twJos->item(i,JCI_FSUB)->setData(Qt::EditRole,v);
            }
        }
        if(col.testFlag(BUC_SNDSUB)){
            for(int i = row; i < row+rows; ++i){
                v.setValue(jls.at(i)->ssub);
                ui->twJos->item(i,JCI_SSUB)->setData(Qt::EditRole,v);
            }
        }
        if(col.testFlag(BUC_MTYPE)){
            for(int i = row; i < row+rows; ++i){
                v.setValue(jls.at(i)->mt);
                ui->twJos->item(i,JCI_MTYPE)->setData(Qt::EditRole,v);
            }
            //ui->twJos->setJSum(curPz->jsum());
            //ui->twJos->setDSum(curPz->dsum());
        }
        if(col.testFlag(BUC_VALUE)){
            for(int i = row; i < row+rows; ++i){
                v.setValue(jls.at(i)->value);
                if(jls.at(i)->dir == MDIR_J){
                    ui->twJos->item(i,JCI_JV)->setData(Qt::EditRole,v);
                    ui->twJos->item(i,JCI_DV)->setData(Qt::EditRole,0);
                }
                else{
                    ui->twJos->item(i,JCI_JV)->setData(Qt::EditRole,0);
                    ui->twJos->item(i,JCI_DV)->setData(Qt::EditRole,v);
                }
            }
            //ui->tview->setJSum(curPz->jsum());
            //ui->tview->setDSum(curPz->dsum());
        }
    }
    turnWatchData();
}

void JournalizingPreviewDlg::insertBa(int row, Journalizing* j)
{
    if(row < 0 || row >= jls.count())
        return;
    jls.insert(row,j);
    ui->twJos->insertRow(row);
    rendRow(row,j);
}

/**
 * 移动组sg到dg
 * @param sg 源组号
 * @param dg 目的组号
 */
void JournalizingPreviewDlg::moveGroupTo(int sg,int dg)
{
    int startSg=-1,endSg=-1,startDg=-1;
    for(int i = 0; i < jls.count(); ++i){
        Journalizing* j = jls.at(i);
        if(startSg == -1 && j->gnum == sg){
            startSg = i;
            continue;
        }
        if(endSg == -1 && j->gnum>sg){
            endSg = i;
            continue;
        }
        if(startDg == -1 && j->gnum == dg){
            startDg = i;
            continue;
        }
        if(startSg != -1 && endSg != -1 && startDg != -1)
            break;
    }
    if(sg > dg){  //up
        Journalizing* j;
        for(int i=0; i<endSg-startSg+1; ++i){
            j = jls.takeAt(startSg);
            jls.insert(startDg+i,j);
        }
    }  
    else{  //down

    }
}

/**
 * 移动凭证
 * @param sp 源凭证号
 * @param dp 目的凭证号
 */
void JournalizingPreviewDlg::movePzTo(int sp, int dp)
{

}

/**
 * @brief 检查是否正确选择了组（组的每行都选择才表示选择了组，目前，为了简化，即使选择一行，也表示选择了该组）
 * @param groups  选择的组
 * @return
 */
bool JournalizingPreviewDlg::selectedGroups()
{
    selectedGs.clear();
    selectedGroupRows.clear();
    //基于这样的事实，组行选模式下，每个选择范围都表示一行
    foreach (QTableWidgetSelectionRange range, ui->twJos->selectedRanges()) {
        int tr = range.topRow();
        Journalizing* jo = ui->twJos->item(tr,JCI_SUMMARY)->data(DR_JOURNALIZING).value<Journalizing*>();
        if(selectedGs.contains(jo->gnum)){
            selectedGroupRows.last() = tr;
            continue;
        }
        selectedGs<<jo->gnum;
        selectedGroupRows<<jo->journal->startPos;
        selectedGroupRows<<jo->journal->startPos;
    }
    return true;
}



//////////////////////slots functionj////////////////////////////////

void JournalizingPreviewDlg::BaDataChanged(QTableWidgetItem *item)
{
    int row = item->row();
    int col = item->column();
    if(col<=JCI_GROUP)
        return;

//    //如果是备用行，则将备用行升级为有效行，再添加新的备用行
//    int rows = ui->twJos->getValidRows();
//    if(row == rows - 1){
//        curBa = new BusiAction;
//        AppendBaCmd* cmd = new AppendBaCmd(pzMgr,curPz,curBa);
//        pzMgr->getUndoStack()->push(cmd);
//        int rows = curPz->baCount();
//        disconnect(ui->twJos,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
//        ui->twJos->setValidRows(rows+1);
//        delegate->setVolidRows(rows);
//        initBlankBa(rows);
//        connect(ui->twJos,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(BaDataChanged(QTableWidgetItem*)));
//    }

    if(!curBa)
        return;
//    ModifyMultiPropertyOnBa* multiCmd;
//    ModifyBaSummaryCmd* scmd;
    FirstSubject* fsub=0;
    SecondSubject* ssub=0;
    Double v=0.0;
    Money* mt;
//    MoneyDirection dir;
    BaUpdateColumns updateCols;

    switch(col){
    case JCI_SUMMARY:{
        QString content = item->data(Qt::EditRole).toString();
        if(curBa->summary != content){
            curBa->summary = content;
            curBa->changed = true;
        }
        break;
    }
    case JCI_FSUB:  //这里存在一个问题，原先是应收，改应付后，对应子目没有设置???????
        fsub = item->data(Qt::EditRole).value<FirstSubject*>();
        curBa->fsub = fsub;
        curBa->changed = true;
        //自动依据上一条分录设置应交税金的适配子目
        if(fsub == yjsjSub && row > 0){
            Journalizing* ba = jls.at(row-1);
            FirstSubject* pfsub = ba?ba->fsub:0;
            if(pfsub){
                if(pfsub == zysrSub || pfsub == ysSub)
                    ssub = xxseSSub;
                else if(pfsub == zycbSub || pfsub == yfSub)
                    ssub = jxseSSub;
                else if(fsub->isHaveSmartAdapte())
                    ssub = fsub->getAdapteSSub(curBa->summary);
                curBa->ssub = ssub;
            }
        }
        //如果是应收应付科目，则提取摘要中的客户关键字信息自动适配子目
        else if(smartSSubSet && !curBa->summary.isEmpty() && (fsub == ysSub || fsub == yfSub)){
            SecondSubject* sub;
            sub = getAdapterSSub(fsub,curBa->summary,prefixes.value(fsub->getCode()),
                                 suffixes.value(fsub->getCode()));
            if(sub){
                ssub = sub;
                //如果是收回应收或付出应付的分录，则自动查找对应发票号的应收应付分录
                QString prefixChar = curBa->summary.left(1);
                if(prefixChar == tr("收") || prefixChar == tr("付")){
                    QList<int> months;
                    QList<QStringList> invoiceNums;
                    PaUtils::extractInvoiceNum(curBa->summary,months,invoiceNums);
                    if(!months.isEmpty()){
                        QHash<int,QList<int> > range;
                        for(int i=0; i < months.count(); ++i){
                            int m = months.at(i);
                            int y = year;
                            if(m > month)
                                y--;
                            if(!range.contains(y))
                                range[y] = QList<int>();
                            range[y]<<m;
                        }
                        //emit findMatchBas(fsub,ssub,range,invoiceNums);
                    }
                }
            }
            curBa->ssub = ssub;
        }
        //如果一级科目设置了包含型的子目自动适配关键字，则适配子目
        else if(fsub && fsub->isHaveSmartAdapte()){
            ssub = fsub->getAdapteSSub(curBa->summary);
            curBa->ssub = ssub;
        }
        if(!ssub && fsub){
            ssub = fsub->getDefaultSubject();
            curBa->ssub = ssub;
        }
        updateCols |= BUC_FSTSUB;
        updateCols |= BUC_SNDSUB;
        //如果是银行科目（其二级科目决定了匹配的币种），则根据银行账户的货币属性设置当前的币种
        if(fsub == bankSub){
            mt = sm->getSubMatchMt(ssub);
            if(mt == curBa->mt)
                v = curBa->value;
            else if(curBa->mt){
                if(mt == mmt && rates.contains(curBa->mt->code()))
                    v = curBa->value * rates.value(curBa->mt->code());
                else if(rates.contains(mt->code()))
                    v = curBa->value / rates.value(mt->code());
                updateCols |= BUC_MTYPE;
                updateCols |= BUC_VALUE;
            }
            else
                updateCols |= BUC_MTYPE;
        }
        else if(!curBa->mt){
            curBa->mt = mmt;
            mt = curBa->mt;
            v = curBa->value;
            updateCols |= BUC_MTYPE;
            updateCols |= BUC_VALUE;
        }
        //如果新的一级科目使用外币，或者不使用外币且当前货币是本币，则无须调整币种和金额
        else if(fsub && (fsub->isUseForeignMoney() || (!fsub->isUseForeignMoney() && curBa->mt == mmt))){
            mt = curBa->mt;
            v = curBa->value;
        }
        else{
            mt = mmt;
            if(curBa->mt)
                v = rates.value(curBa->mt->code()) * curBa->value;
            //updateCols |= BUC_MTYPE;
            //updateCols |= BUC_VALUE;
        }
        break;
    case JCI_SSUB:{
        if(isInteracting)
            return;
        fsub = curBa->fsub;
        ssub = item->data(Qt::EditRole).value<SecondSubject*>();
        if(!fsub)
            return;
        bool isEnChanged = false;
        if(ssub && !ssub->isEnabled()){
            if(QDialog::Rejected == myHelper::ShowMessageBoxQuesion(tr("二级科目“%1”已被禁用，是否重新启用？").arg(ssub->getName()))){
               QVariant v; v.setValue(curBa->ssub);
               item->setData(Qt::EditRole,v);
               return;
            }
            isEnChanged = true;
        }
        //如果是银行科目，则根据银行账户所属的币种设置币种对象
        if(ssub && bankSub == fsub){
            mt = sm->getSubMatchMt(ssub);
            if(!mt)
                break;
            Money* curMt = curBa->mt;
            if(curMt && mt != curMt){
                if(mt != sm->getAccount()->getMasterMt() && rates.contains(mt->code()))
                    v = curBa->value / rates.value(mt->code());
                else if(rates.contains(curMt->code()))
                    v = curBa->value * rates.value(curMt->code());
                else
                    v = curBa->value;
                updateCols |= BUC_MTYPE;
                updateCols |= BUC_VALUE;
            }
            else
                v = curBa->value;
        }
        else{//如果是普通科目，且未设币种，则默认将币种设为本币
            if(!curBa->mt){
                mt = sm->getAccount()->getMasterMt();
                updateCols |= BUC_MTYPE;
            }
            else
                mt = curBa->mt;
            v = curBa->value;
        }
        if(isEnChanged){
            ssub->setEnabled(true);
            //QUndoCommand* mmd  = new QUndoCommand;
            //ModifySndSubEnableProperty* cmdSSub = new ModifySndSubEnableProperty(ssub,true,mmd);
            //cmdSSub->setCreator(this);
            //multiCmd = new ModifyMultiPropertyOnBa(curBa,fsub,ssub,mt,v,curBa->getDir(),mmd);
            //pzMgr->getUndoStack()->push(mmd);
        }
        curBa->ssub = ssub;
        curBa->mt = mt;
        curBa->value = v;
        curBa->changed = true;
        //if(ssub){
        //    QString tip = ssub->getLName();
        //    if(fsub == bankSub())
        //        tip.append(tr("（帐号：%1）").arg(sm->getBankAccount(ssub)->accNumber));
         //   ui->tview->setLongName(tip);
        //}
        break;}
    case JCI_MTYPE:{
        mt = item->data(Qt::EditRole).value<Money*>();
        updateCols |= BUC_MTYPE;
        //如果金额为0，则不必调整金额，否则，必须调整金额
        if(curBa->value != 0){
            if(curBa->mt){
                //如果从外币转到本币
                if(mt == sm->getAccount()->getMasterMt())
                    v = curBa->value * rates.value(curBa->mt->code(),1.0);
                //从甲外币转到乙外币（先将甲外币转到本币，再转到乙外币）
                else if(mt != sm->getAccount()->getMasterMt() && curBa->mt != sm->getAccount()->getMasterMt())
                    v = curBa->value * rates.value(curBa->mt->code(),1.0) / rates.value(mt->code(),1.0);
                else   //从本币转到外币
                    v = curBa->value / rates.value(mt->code(),1.0);
                updateCols |= BUC_VALUE;
            }
            else
                v = curBa->value;
        }
        else
            v = 0.0;
        curBa->value = v;
        curBa->mt = mt;
        int sr=-1,er=-1;
        getCurrentGroupRowIndex(sr,er);
        calBalanceInGroup(sr,er);
        break;}
    case JCI_JV:{
        curBa->value = item->data(Qt::EditRole).value<Double>();
        curBa->dir = MDIR_J;
        curBa->changed = true;
        updateCols |= BUC_VALUE;
        int sr=-1,er=-1;
        getCurrentGroupRowIndex(sr,er);
        calBalanceInGroup(sr,er);
        break;}
    case JCI_DV:{
        curBa->value = item->data(Qt::EditRole).value<Double>();
        curBa->dir = MDIR_D;
        curBa->changed = true;
        updateCols |= BUC_VALUE;
        int sr=-1,er=-1;
        getCurrentGroupRowIndex(sr,er);
        calBalanceInGroup(sr,er);
        break;}
    }
    updateBas(row,1,updateCols);
}

void JournalizingPreviewDlg::twContextMenuRequested(const QPoint &pos)
{
    QTableWidgetItem* item = ui->twJos->itemAt(pos);
    if(item){
        int col = item->column();
        int row = item->row();
        if(col == JCI_PNUM){
            QMenu* m = new QMenu(this);
            m->addAction(ui->actDissolvePz);
            m->popup(ui->twJos->mapToGlobal(pos));
        }
        else if(col == JCI_GROUP){
            QMenu* m = new QMenu(this);
            m->addAction(ui->actCollectToPz);
            if(selectedGroups() && selectedGs.count()==2){  //目前暂不支持多于2组的合并
                m->addAction(ui->actMerge);
            }
            m->popup(ui->twJos->mapToGlobal(pos));
        }
        else{
            QMenu* m = new QMenu(this);
            m->addAction(ui->actInsert);
            m->addAction(ui->actRemove);
            m->popup(ui->twJos->mapToGlobal(pos));
        }
    }
}

/**
 * @brief JournalizingPreviewDlg::currentCellChanged
 * @param currentRow
 * @param currentColumn
 * @param previousRow
 * @param previousColumn
 */
void JournalizingPreviewDlg::currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if(currentRow == -1)
        curBa = 0;
    else
        curBa = jls.at(currentRow);
}

/**
 * [creatNewNameItemMapping description]
 * @param row  [description]
 * @param col  [description]
 * @param fsub [description]
 * @param ni   [description]
 * @param ssub [description]
 */
void JournalizingPreviewDlg::creatNewNameItemMapping(int row, int col, FirstSubject *fsub, SubjectNameItem *ni, SecondSubject*& ssub)
{
    isInteracting = true;
    if(!curUser->haveRight(allRights.value(Right::Account_Config_SetSndSubject))){
        myHelper::ShowMessageBoxWarning(tr("您没有创建新二级科目的权限！"));
        return;
    }
    if(QMessageBox::information(0,msgTitle_info,tr("确定要使用已有的名称条目“%1”在一级科目“%2”下创建二级科目吗？")
                                .arg(ni->getShortName()).arg(fsub->getName()),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::No){
        BASndSubItem_new* item = static_cast<BASndSubItem_new*>(ui->twJos->item(row,col));
        item->setText(curBa->ssub?curBa->ssub->getName():"");
        delegate->userConfirmed();
        return;
    }
    isInteracting = false;
    // ModifyBaSndSubNMMmd* cmd = new ModifyBaSndSubNMMmd(pzMgr,curPz,curBa,subMgr,fsub,ni,1,QDateTime::currentDateTime(),curUser);
    // pzMgr->getUndoStack()->push(cmd);
    //ssub = cmd->getSecondSubject();
    ssub = fsub->addChildSub(ni);
    BaUpdateColumns updateCols;
    updateCols |= BUC_SNDSUB;
    updateBas(row,1,updateCols);
    delegate->userConfirmed();
    ui->twJos->edit(ui->twJos->model()->index(row,col+1));
}


void JournalizingPreviewDlg::creatNewSndSubject(int row, int col, FirstSubject* fsub, SecondSubject*& ssub, QString name)
{
    if(!curUser->haveRight(allRights.value(Right::Account_Config_SetSndSubject))){
        myHelper::ShowMessageBoxWarning(tr("您没有创建新二级科目的权限！"));
        return;
    }
    if(QMessageBox::information(0,msgTitle_info,tr("确定要用新的名称条目“%1”在一级科目“%2”下创建二级科目吗？")
                                .arg(name).arg(fsub->getName()),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes){
        CompletSubInfoDialog* dlg = new CompletSubInfoDialog(fsub->getId(),sm,0);
        dlg->setName(name);
        NameItemAlias* alias = SubjectManager::getMatchedAlias(name);
        if(alias){
            dlg->setLongName(alias->longName());
            dlg->setRemCode(alias->rememberCode());
        }
        if(QDialog::Accepted == dlg->exec()){
            // ModifyBaSndSubNSMmd* cmd =
            //     new ModifyBaSndSubNSMmd(pzMgr,curPz,curBa,subMgr,fsub,
            //                             dlg->getSName(),dlg->getLName(),
            //                             dlg->getRemCode(),dlg->getSubCalss(),
            //                             QDateTime::currentDateTime(),curUser);
            // pzMgr->getUndoStack()->push(cmd);
            // ssub = cmd->getSecondSubject();
            SubjectNameItem* ni = ni = sm->addNameItem(dlg->getSName(),dlg->getLName(),
                      dlg->getRemCode(),dlg->getSubCalss(),QDateTime::currentDateTime(),curUser);
            ssub = fsub->addChildSub(ni);// sm->addNameItem(ni);
            BaUpdateColumns updateCols;
            updateCols |= BUC_SNDSUB;
            updateBas(row,1,updateCols);
            delegate->userConfirmed();
            ui->twJos->edit(ui->twJos->model()->index(row,col));
            //如果新建的名称对象和别名对象两者简称相同，则如果全称也相同，则可以移除此别名，
            //如果全称不同，则将此别名加入到新建名称对象的别名列表
            if(alias && alias->shortName()==curBa->ssub->getName()){
                if(alias->longName() != curBa->ssub->getLName()){
                    curBa->ssub->getNameItem()->addAlias(alias);
                }
                else{//这个实现有点过于草算，因为在用户没有确定保存时，它已经实际地删除了别名，而且这个实现也不支持Undo和Redo操作
                    sm->removeNameAlias(alias);
                    delete alias;
                }
            }            
            return;
        }
    }
    SecondSubject* oldSSub = curBa->ssub;
    BASndSubItem_new* item = static_cast<BASndSubItem_new*>(ui->twJos->item(row,col));
    item->setText(oldSSub?oldSSub->getName():"");
    delegate->userConfirmed();
    ui->twJos->edit(ui->twJos->model()->index(row,col));
}


/**
 * @brief JournalizingPreviewDlg::copyPrewAction
 * 拷贝前一条分录，如果当前行是组的第一条分录，则新增分录添加到前一个组，否则添加到当期分录所在组
 * @param row
 */
void JournalizingPreviewDlg::copyPrewAction(int row)
{
    if(row < 1)
        return;
    Journalizing* j = new Journalizing;
    Journalizing* jp = jls.at(row-1);
    j->pnum = jp->pnum;
    j->gnum = jp->gnum;
    j->numInGroup = jp->numInGroup+1;
    if(row != jls.count()-1){
        for(int i=row+1; i < jls.count(); ++i){
            Journalizing* j1 = jls.at(i);
            if(j1->gnum != jp->gnum)
                break;
            j1->numInGroup++;
        }
    }    
    j->summary = jp->summary;
    j->fsub = jp->fsub;
    j->ssub = jp->ssub;
    j->mt = jp->mt;
    j->dir = jp->dir;
    j->value = jp->value;
    insertBa(row,j);
    if(jls.at(row-1)->gnum != jls.at(row+1)->gnum){
        int spanRows = ui->twJos->rowSpan(row-1,JCI_GROUP);
        int startRow = row-spanRows;
        ui->twJos->setSpan(startRow,JCI_GROUP,spanRows+1,1);
        ui->twJos->setSpan(startRow,JCI_VTAG,spanRows+1,1);
    }
    ui->twJos->selectRow(row);
    ui->twJos->edit(ui->twJos->model()->index(row,JCI_SUMMARY));
    int sr=-1,er=-1;
    getCurrentGroupRowIndex(sr,er);
    calBalanceInGroup(sr,er);
}

/**
 * @brief 将一个或多个组归并到一个凭证
 */
void JournalizingPreviewDlg::mnuCollectToPz()
{
    int curRow = ui->twJos->currentRow();
    int spnum = jls.at(curRow)->pnum;
    int sgnum = jls.at(curRow)->gnum;
    int sgStart=-1,sgEnd = -1;
    foreach(QTableWidgetSelectionRange r, ui->twJos->selectedRanges()){
        if(sgStart==-1 && curRow >= r.topRow() && curRow <= r.bottomRow()){
            sgStart = r.topRow();
            sgEnd = r.bottomRow();
            continue;
        }
        //if(jls.at(r->topRow()))
    }

}

/**
 * @brief 将凭证内的组解散为独立的组
 */
void JournalizingPreviewDlg::mnuDissolvePz()
{
    int i = 0;
}

/**
 * @brief 在当前行插入新分录，新分录属于当期分录所在组
 */
void JournalizingPreviewDlg::mnuInsertBa()
{
    if(!curBa)
        return;
    int row = ui->twJos->currentRow();
    Journalizing* j = new Journalizing;
    j->journal = curBa->journal;
    j->pnum = curBa->pnum;
    j->gnum = curBa->gnum;
    j->numInGroup = curBa->numInGroup;
    j->mt = mmt;
    int spanRows = ui->twJos->rowSpan(row,JCI_GROUP);
    if(row < jls.count()-1){  //调整组内后续分录的序号
        int r = row;
        Journalizing* nj =  jls.at(r);
        while(nj->gnum == curBa->gnum){
            nj->numInGroup += 1;
            nj->changed = true;
            if(r == jls.count()-1)
                break;
            nj = jls.at(++r);
        }
    }
    jls.insert(row,j);
    ui->twJos->insertRow(row);
    rendRow(row,j);
    if(curBa->numInGroup == 2){
        ui->twJos->setSpan(row,JCI_GROUP,spanRows+1,1);
        ui->twJos->setSpan(row,JCI_VTAG,spanRows+1,1);
    }
    ui->twJos->selectRow(row);
}

/**
 * @brief 移除当前行所在分录
 */
void JournalizingPreviewDlg::mnuRemoveBa()
{
    if(!curBa)
        return;
    if(curBa->fsub && curBa->fsub == bankSub){
        myHelper::ShowMessageBoxWarning(tr("银行账分录不能删除！"));
        return;
    }
    int row = ui->twJos->currentRow();
    int spanRows = ui->twJos->rowSpan(row,JCI_GROUP);
    int numInGroup = curBa->numInGroup;
    if(spanRows > 1){  //更新组内后续分录的序号
        int rows = spanRows - curBa->numInGroup;
        for(int i=0; i<rows; ++i){
            Journalizing * j = jls.at(row+i);
            j->numInGroup -= 1;
            j->changed = true;
        }
    }
    else{ //只包含一条分录的组，则要更新后续分录的组号
        if(row < jls.count()-1){
            turnWatchData(false);
            int curGNum = curBa->gnum;
            for(int i = row+1; i < ui->twJos->rowCount(); ++i){
                Journalizing * j = jls.at(i);
                if(j->gnum != curGNum){
                    curGNum = j->gnum;
                    j->gnum -= 1;
                    j->changed = true;
                    ui->twJos->item(i,JCI_GROUP)->setText(QString::number(j->gnum));
                    continue;
                }
                j->gnum = curGNum-1;
            }
            turnWatchData();
        }
    }
    doRemoves<<jls.takeAt(row);
    if(spanRows > 1){ //重新计算组的借贷平衡
        int sr = row-numInGroup+1;
        int er = sr+spanRows-2;
        calBalanceInGroup(sr,er);
    }
    //turnWatchData(false);
    ui->twJos->removeRow(row);
    //turnWatchData();
    if(row > ui->twJos->rowCount()-1)
        ui->twJos->selectRow(row-1);
    else
        ui->twJos->selectRow(row);
}


void JournalizingPreviewDlg::mnuCopyBa()
{
    int i = 0;
}


void JournalizingPreviewDlg::mnuCutBa()
{
    int i = 0;
}


void JournalizingPreviewDlg::mnuPasteBa()
{
    int i = 0;
}

/**
 * @brief 合并组
 */
void JournalizingPreviewDlg::mnuMerge()
{
    if(selectedGs.empty())
        return;

    bool isSerial = true;
    int sg = selectedGs.first();
    for(int i=1; i<selectedGs.count(); ++i){
        int g = selectedGs.at(i);
        if(g-sg > 1){
            isSerial = false;
            break;
        }
    }
    if(isSerial){  //如果合并的组号是连续的
        //检测选择的组是否跨凭证
        Journalizing* j = jls.at(selectedGroupRows.first());
        int curPz = j->pnum;
        QList<int> pzNums;      //所选组所跨越的凭证
        QList<int> pzRowIndex;  //所属凭证的开始和结束索引位置，每两个原始对应一张凭证
        pzNums<<j->pnum;
        pzRowIndex<<selectedGroupRows.first()<<selectedGroupRows.first();
        //bool isCrossPz = false;
        for(int i = selectedGroupRows.first()+1; i <= selectedGroupRows.last(); ++i){
            j = jls.at(i);
            if(curPz != j->pnum){
                curPz = j->pnum;
                pzNums<<j->pnum;
                pzRowIndex<<i<<i;
                continue;
            }
            pzRowIndex.last()=i;
        }
        if(pzNums.count()>2){
            myHelper::ShowMessageBoxWarning(tr("所选的组跨越了2张以上的凭证，合并操作只支持跨越2张凭证！"));
            return;
        }
        if(pzNums.count() == 2){  //跨越凭证，则调整凭证列的跨越行
            curPz = pzNums.first();
            int curPos=0;
            if(selectedGroupRows.first()>0){
                curPos = selectedGroupRows.first();
                j = jls.at(curPos);
                while(j->pnum == curPz){
                    curPos--;
                    if(curPos<0)
                        break;
                    j = jls.at(curPos);
                }
            }
            pzRowIndex.first()=curPos;
            if(selectedGroupRows.last() < jls.count()-1){
                curPos = pzNums.last();
                curPos = selectedGroupRows.last();
                j = jls.at(curPos);
                while(j->pnum == curPz){
                    curPz++;
                    if(curPz==jls.count())
                        break;
                    j = jls.at(curPos);
                }
            }
            pzRowIndex.last()=curPos;
            QStringList ps;
            foreach(int p, pzNums)
                ps<<QString::number(p);
            bool ok=false;
            int dp = QInputDialog::getItem(this,"",tr("请选择目的凭证号："),ps,0,false,&ok).toInt();
            if(!ok)
                return;
            int spanRows=0;
            if(dp == pzNums.first()){ //合并到前一张凭证
                pzRowIndex[1] = selectedGroupRows.last();
                spanRows = pzRowIndex[1] - pzRowIndex[0] + 1;
                ui->twJos->setSpan(pzRowIndex.first(),JCI_PNUM,spanRows,1);
            }
            else{
                pzRowIndex[2] = selectedGroupRows.first();
                spanRows = pzRowIndex[3] - pzRowIndex[2] + 1;
                ui->twJos->setSpan(selectedGroupRows.first(),JCI_PNUM,spanRows,1);
            }
        }

        int numInGroup = 1;
        for(int i = selectedGroupRows.first(); i <= selectedGroupRows.last(); ++i){  //更新组内分录序号
            j = jls.at(i);
            j->numInGroup = numInGroup++;
            j->gnum = selectedGs.first();
            j->changed = true;
        }
        int spanRows = selectedGroupRows.last()-selectedGroupRows.first()+1;
        ui->twJos->setSpan(selectedGroupRows.first(),JCI_GROUP,spanRows,1);
        ui->twJos->setSpan(selectedGroupRows.first(),JCI_VTAG,spanRows,1);
        //更新后续组号
        int curGNum = selectedGs.last();
        int newGNum = selectedGs.first();
        if(selectedGroupRows.last() < jls.count()-1){
            for(int i = selectedGroupRows.last()+1; i < jls.count(); ++i){
                Journalizing* j = jls.at(i);
                if(j->gnum != curGNum){
                    curGNum = j->gnum;
                    newGNum++;
                    j->gnum=newGNum;
                    j->changed = true;
                    ui->twJos->item(i,JCI_GROUP)->setText(QString::number(newGNum));
                    continue;
                }
                j->gnum = newGNum;
                j->changed = true;
            }
        }
        return;
    }

    QStringList gs;
    foreach(int g, selectedGs)
        gs<<QString::number(g);
    bool ok=false;
    int dg = QInputDialog::getItem(this,"",tr("请选择目的组号："),gs,0,false,&ok).toInt();
    if(!ok)
        return;
    int pznum = 0;
    for(int i = 0; i<jls.count(); ++i){
        Journalizing* jo = jls.at(i);
        if(jo->gnum == dg){
            pznum = jo->pnum;
            break;
        }
    }

    int eg = selectedGs.last();
    int si=-1,ei=-1;
    int numInGroup = 1;


    //不连续组的合并
    int idx = selectedGs.indexOf(dg);
    selectedGroupRows.removeAt(idx*2);
    selectedGroupRows.removeAt(idx*2);
    selectedGs.removeAt(idx);

    int disi =0;   //目的组开始行
    for(int i = 0; i<jls.count(); ++i){
        if(jls.at(i)->gnum == dg){
            disi = i;
            break;
        }
    }
    if(disi == 0)
        disi = jls.count();
    int diei = disi;  //目的组插入后结束位
    for(int i = disi; i<jls.count(); ++i){
        if(jls.at(i)->gnum == dg+1){
            diei = i;
            break;
        }
    }

    for(int i=0; i < selectedGs.count(); ++i){
        int g = selectedGs.at(i);
        int si = selectedGroupRows.at(i*2);
        int ei = selectedGroupRows.at(i*2+1);
        int rows = ei-si+1;
        if(g < dg){  //待合并组组目的组之前
            //调整jls列表
            Journal* curJ = 0;
            for(int i=0; i<rows; ++i){
                Journalizing* jo = jls.at(si+i);
                int newd = diei+i;  //插入位置
                if(curJ != jo->journal){
                    jo->journal->startPos=newd;  //调整待合并自然组的开始索引位置
                    curJ = jo->journal;
                }
                jo->gnum = dg;
                jo->pnum = pznum;
                jls.insert(newd,jo);
                ui->twJos->insertRow(newd);
                for(int j = 2; j<ui->twJos->columnCount()-1; ++j){  //调整表格行
                     ui->twJos->setItem(newd,j,ui->twJos->takeItem(si+i,j));
                }
            }
            diei=diei+rows;
            for(int i = si; i <= ei; ++i){
                jls.removeAt(si);
                ui->twJos->removeRow(si);
                disi--;diei--;
            }
            int spanRows = diei-disi;
            ui->twJos->setSpan(disi,JCI_GROUP,spanRows,1);
            ui->twJos->setSpan(si,JCI_VTAG,spanRows,1);
        }
        else{ //待合并组组目的组之后
            //调整jls列表

            //调整表格行
        }
    }
    //最后选择新合并的组，以便看清所做
}

void JournalizingPreviewDlg::processShortcut()
{
    if(sender() == sc_save)
        save();
    else if(ui->twJos->hasFocus()){
        if(sender() == sc_copy){
//            QList<int> rows; bool c;
//            ui->tview->selectedRows(rows,c);
//            if(rows.empty())
//                return;
//            copyOrCut = CO_COPY;
//            clb_Bas.clear();
//            foreach(int i, rows)
//                clb_Bas<<curPz->getBusiAction(i);
        }
        else if(sender() == sc_cut){
//            QList<int> rows; bool c;
//            ui->tview->selectedRows(rows,c);
//            if(rows.empty())
//                return;
//            copyOrCut = CO_CUT;
//            clb_Bas.clear();
//            CutBaCmd* cmd = new CutBaCmd(pzMgr,curPz,rows,&clb_Bas);
//            pzMgr->getUndoStack()->push(cmd);
//            refreshActions();
        }
        else if(sender() == sc_paster){
//            if(clb_Bas.empty())
//                return;
//            if(curRow == -1)
//                return;
//            int row = curRow;
//            int rows = clb_Bas.count();
//            PasterBaCmd* cmd = new PasterBaCmd(curPz,curRow,&clb_Bas,copyOrCut==CO_COPY);
//            pzMgr->getUndoStack()->push(cmd);
//            refreshActions();
//            //选中粘贴行
//            curRow = row;
//            for(int i = 0; i < rows; ++i)
//                ui->tview->selectRow(curRow+i);
        }
    }
}


void JournalizingPreviewDlg::on_btnGenrate_clicked()
{
    createPzs();
}


void JournalizingPreviewDlg::on_btnClose_clicked()
{
    if(isDirty() && myHelper::ShowMessageBoxQuesion(tr("分录已改变，需要保存吗？")) == QDialog::Accepted)
        save();
    accept();
}

void JournalizingPreviewDlg::on_tbUp_clicked()
{
    int i = 0;
}

void JournalizingPreviewDlg::on_tbDown_clicked()
{
    int i = 0;
}

void JournalizingPreviewDlg::on_tbGUp_clicked()
{
    int i = 0;
}

void JournalizingPreviewDlg::on_tbGDown_clicked()
{
    int i = 0;
}



///////////////////////////////////////////////////////////////
bool journalThan(Journal *j1, Journal *j2)
{
    if(j1->date != j2->date)
        return j1->date<j2->date;
    if(j1->dir != j2->dir)
        return j1->dir > j2->dir;
    return j1->bankId > j2->bankId;

}


/**
 * @brief 收入发票排序函数（按先普后专，发票号顺序）
 * @param i1
 * @param i2
 * @return
 */
bool incomeInvoiceThan(CurInvoiceRecord *i1, CurInvoiceRecord *i2)
{
    if(i1->type != i2->type)
        return i1->type == false;
    return i1->inum < i2->inum;
}

/**
 * @brief 成本发票排序函数（按先普后专，客户、发票号顺序）
 * @param i1
 * @param i2
 * @return
 */
bool costInvoiceThan(CurInvoiceRecord *i1, CurInvoiceRecord *i2)
{
    if(i1->type != i2->type)
        return !i1->type && i2->type;
    if(i1->client == i2->client)
        return i1->inum < i2->inum;
    return i1->client < i2->client;
}





