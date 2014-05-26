#include <QBuffer>
#include <QMenu>
#include <QDebug>

#include "curstatdialog.h"
#include "ui_curstatdialog.h"
#include "statutil.h"
#include "HierarchicalHeaderView.h"
#include "subject.h"
#include "widgets.h"
#include "PzSet.h"
#include "previewdialog.h"

CurStatDialog::CurStatDialog(StatUtil *statUtil, QByteArray* sinfo, QWidget *parent)
    :DialogWithPrint(parent),ui(new Ui::CurStatDialog),statUtil(statUtil)
{
    ui->setupUi(this);
    init(account);

    headerModel = NULL;
    dataModel = NULL;

    //初始化表格行的背景色
//    row_tc_fsub = QColor(Qt::blue);
//    row_tc_ssub =QColor(Qt::black);
//    row_tc_sum = QColor(Qt::red);
    row_bk_ssub = QBrush(QColor(200,200,255));
    row_bk_fsub = QBrush(QColor(150,150,255));
    row_bk_sum = QBrush(QColor(100,100,255));

    //初始化自定义的层次式表头
    hv = new HierarchicalHeaderView(Qt::Horizontal, ui->tview);
    hv->setHighlightSections(true);
    //hv->setClickable(true);
    hv->setSectionsClickable(true);
    //hv->setStyleSheet("QHeaderView::section {background-color:darkcyan;}");
    ui->tview->setHorizontalHeader(hv);
    setState(sinfo);
    stat();
}

CurStatDialog::~CurStatDialog()
{
    delete ui;
}

/**
 * @brief CurStatDialog::setState
 *  恢复窗口状态（获取表格的各字段的宽度信息（返回列表的第一个元素是当前显示的表格类型））
 *  第一次调用此函数时，已经显示表格数据（此函数应在setDate函数后得到调用）
 * @param info
 */
void CurStatDialog::setState(QByteArray *info)
{
    qint8 i8;
    qint16 i16;

    //如果数据库中无状态信息，则使用默认的设置
    if((info == NULL) || info->isNull()){
        stateInfo.tFormat = COMMON;
        ui->rdoJe->setChecked(true);
        stateInfo.viewDetails = false;
        ui->cmbSndSub->setEnabled(false);
        ui->chkIsDet->setChecked(false);
        //屏幕视图下的表格列宽
        stateInfo.colWidths[COMMON] << 80; //科目编码
        stateInfo.colWidths[COMMON] << 200;   //科目名称
        stateInfo.colWidths[COMMON] << 50;   //方向
        stateInfo.colWidths[COMMON] << 150;   //期初金额
        stateInfo.colWidths[COMMON] << 150;   //本期借方发生
        stateInfo.colWidths[COMMON] << 150;   //本期贷方发生
        stateInfo.colWidths[COMMON] << 50;   //方向
        stateInfo.colWidths[COMMON] << 150;   //期末余额
        stateInfo.colWidths[THREERAIL] << 80; //科目编码
        stateInfo.colWidths[THREERAIL] << 200; //科目名称
        stateInfo.colWidths[THREERAIL] << 50; //方向
        stateInfo.colWidths[THREERAIL] << 150; //期初金额（外币）
        stateInfo.colWidths[THREERAIL] << 150; //期初金额（金额）
        stateInfo.colWidths[THREERAIL] << 150; //本期借方发生（外币）
        stateInfo.colWidths[THREERAIL] << 150; //本期借方发生（金额）
        stateInfo.colWidths[THREERAIL] << 150; //本期贷方发生（外币）
        stateInfo.colWidths[THREERAIL] << 150; //本期贷方发生（金额）
        stateInfo.colWidths[THREERAIL] << 50; //方向
        stateInfo.colWidths[THREERAIL] << 150;//期末余额（外币）
        stateInfo.colWidths[THREERAIL] << 150;//期末余额（金额）
        //打印模式下的表格列宽
        stateInfo.colPriWidths[COMMON] << 80; //科目编码
        stateInfo.colPriWidths[COMMON] << 200;   //科目名称
        stateInfo.colPriWidths[COMMON] << 50;   //方向
        stateInfo.colPriWidths[COMMON] << 150;   //期初金额
        stateInfo.colPriWidths[COMMON] << 150;   //本期借方发生
        stateInfo.colPriWidths[COMMON] << 150;   //本期贷方发生
        stateInfo.colPriWidths[COMMON] << 50;   //方向
        stateInfo.colPriWidths[COMMON] << 150;   //期末余额
        stateInfo.colPriWidths[THREERAIL] << 80; //科目编码
        stateInfo.colPriWidths[THREERAIL] << 200; //科目名称
        stateInfo.colPriWidths[THREERAIL] << 50; //方向
        stateInfo.colPriWidths[THREERAIL] << 150; //期初金额（外币）
        stateInfo.colPriWidths[THREERAIL] << 150; //期初金额（金额）
        stateInfo.colPriWidths[THREERAIL] << 150; //本期借方发生（外币）
        stateInfo.colPriWidths[THREERAIL] << 150; //本期借方发生（金额）
        stateInfo.colPriWidths[THREERAIL] << 150; //本期贷方发生（外币）
        stateInfo.colPriWidths[THREERAIL] << 150; //本期贷方发生（金额）
        stateInfo.colPriWidths[THREERAIL] << 50; //方向
        stateInfo.colPriWidths[THREERAIL] << 150;//期末余额（外币）
        stateInfo.colPriWidths[THREERAIL] << 150;//期末余额（金额）
        stateInfo.pageOrientation = QPrinter::Portrait;
        stateInfo.margins.unit = QPrinter::Didot;
        stateInfo.margins.left = 20;
        stateInfo.margins.right = 20;
        stateInfo.margins.top = 30;
        stateInfo.margins.bottom = 30;
    }
    else{
        QBuffer bf(info);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        //表格显示格式
        in>>i8;
        if(i8 == COMMON){
            stateInfo.tFormat = COMMON;
            ui->rdoJe->setChecked(true);
        }
        else{
            stateInfo.tFormat = THREERAIL;
            ui->rdoJe->setChecked(false);
        }
        //是否显示明细科目
        bool b;
        in>>b;
        stateInfo.viewDetails = b;
        ui->chkIsDet->setChecked(b);
        //获取通用表格格式的列数及列宽
        in>>i8;
        int cc = i8;
        stateInfo.colWidths[COMMON].clear();
        stateInfo.colPriWidths[COMMON].clear();
        for(int i = 0; i < cc; ++i){
            in>>i16;
            stateInfo.colWidths[COMMON]<<i16;
            in>>i16;
            stateInfo.colPriWidths[COMMON]<<i16;
        }
        //获取三栏式表格格式的列数及列宽
        in>>i8;
        cc = i8;
        stateInfo.colWidths[THREERAIL].clear();
        stateInfo.colPriWidths[THREERAIL].clear();
        for(int i = 0; i < cc; ++i){
            in>>i16;
            stateInfo.colWidths[THREERAIL]<<i16;
            in>>i16;
            stateInfo.colPriWidths[THREERAIL]<<i16;
        }

        //页边距和页面方向
        in>>i8;
        stateInfo.pageOrientation = (QPrinter::Orientation)i8;
        in>>i8;
        stateInfo.margins.unit = (QPrinter::Unit)i8;
        double d;
        in>>d;
        stateInfo.margins.left = d;
        in>>d;
        stateInfo.margins.right = d;
        in>>d;
        stateInfo.margins.top = d;
        in>>d;
        stateInfo.margins.bottom = d;
        bf.close();
    }

    //恢复表格格式选择和是否显示明细科目的选择
    if(stateInfo.tFormat == COMMON)
        ui->rdoJe->setChecked(true);
    else
        ui->rdoJe->setChecked(false);

    ui->chkIsDet->setChecked(stateInfo.viewDetails);
    //ui->cmbSndSub->setEnabled(stateInfo.viewDetails);

    connect(ui->rdoJe,SIGNAL(toggled(bool)),this,SLOT(onTableFormatChanged(bool)));
    connect(ui->chkIsDet,SIGNAL(toggled(bool)),this,SLOT(onDetViewChanged(bool)));
}

/**
 * @brief CurStatDialog::getState
 *  获取状态信息
 * @return
 */
QByteArray *CurStatDialog::getState()
{
    //状态信息序列化顺序：
    //1、表格格式
    //2：是否显示明细
    //3：通用格式表格的列数和各列列宽
    //4：三栏式表格的列数和各列宽
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 i8;
    qint16 i16;

    i8 = stateInfo.tFormat;
    out<<i8;
    out<<stateInfo.viewDetails;
    i8 = 8;
    out<<i8;   //通用格式为8列
    for(int i = 0; i < 8; ++i){
        i16 = stateInfo.colWidths.value(COMMON)[i];
        out<<i16;
        i16 = stateInfo.colPriWidths.value(COMMON)[i];
        out<<i16;
    }
    i8 = 12;
    out<<i8;  //三栏式为12列
    for(int i = 0; i < 12; ++i){
        i16 = stateInfo.colWidths.value(THREERAIL)[i];
        out<<i16;
        i16 = stateInfo.colPriWidths.value(THREERAIL)[i];
        out<<i16;
    }

    i8 = stateInfo.pageOrientation;
    out<<i8;
    i8 = (qint8)stateInfo.margins.unit;
    out<<i8;
    double d = stateInfo.margins.left; out<<d;
    d = stateInfo.margins.right; out<<d;
    d = stateInfo.margins.top; out<<d;
    d = stateInfo.margins.bottom; out<<d;

    bf.close();
    return info;
}

void CurStatDialog::stat()
{
    if(!statUtil->stat()){
        QMessageBox::critical(this,tr("错误提示"),tr("在进行本期统计时发生错误！"));
        return;
    }
    initHashs();
    viewRates();
    viewTable();
}

/**
 * @brief 打印统计表格
 * @param action
 */
void CurStatDialog::print(PrintActionClass action)
{
    switch (action) {
    case PAC_TOPRINTER:
        on_actPrint_triggered();
        break;
    case PAC_PREVIEW:
        on_actPreview_triggered();
        break;
    case PAC_TOPDF:
        on_actToPDF_triggered();
        break;
    }
}

/**
 * @brief CurStatDialog::save
 *  保存当前余额数据到数据库中
 */
void CurStatDialog::save()
{
    if(curAccount->isReadOnly())
        return;
    if(!statUtil->save()){
        QMessageBox::critical(this,tr("错误提示"),tr("保存余额时，发生错误！"));
        return;
    }
    //emit pzsExtraSaved();
    ui->btnSave->setEnabled(false);
    ui->btnClose->setEnabled(true);
}

/**
 * @brief CurStatDialog::colWidthChanged
 *  跟踪记录表格列宽的变化，以便在下次打开本窗口时可以用户选择的列宽显示
 * @param logicalIndex
 * @param oldSize
 * @param newSize
 */
void CurStatDialog::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    stateInfo.colWidths[stateInfo.tFormat][logicalIndex] = newSize;
}

/**
 * @brief CurStatDialog::onSelFstSub
 *  当选择一个总账科目时，初始化可用的明细科目
 * @param index
 */
void CurStatDialog::onSelFstSub(int index)
{
    fsub = ui->cmbFstSub->itemData(index).value<FirstSubject*>();
    //fsub = ui->cmbFstSub->getFirstSubject();
    disconnect(ui->cmbSndSub, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    if(index == 0){
        ui->cmbSndSub->clear();
        ui->cmbSndSub->addItem(tr("所有"),0);
        ui->cmbSndSub->setEnabled(false);
    }
    else{
        ui->cmbSndSub->setEnabled(true);
        ui->cmbSndSub->setFirstSubject(fsub);
        //ui->cmbSndSub->clear();
        ui->cmbSndSub->insertItem(0,tr("所有"),0);
        //QVariant v;
        //foreach(SecondSubject* sub, fsub->getChildSubs()){
        //    v.setValue<SecondSubject*>(sub);
        //    ui->cmbSndSub->addItem(sub->getName(),v);
        //}
        //scom->setPid(fsub->getId());
    }
    ssub = NULL;
    connect(ui->cmbSndSub, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    viewTable();
}

/**
 * @brief CurStatDialog::onSelSndSub
 *  当用户选择一个二级科目时，显示指定二级科目的统计数据
 * @param index
 */
void CurStatDialog::onSelSndSub(int index)
{
    ssub = ui->cmbSndSub->itemData(index).value<SecondSubject*>();
    viewTable();
}

/**
 * @brief CurStatDialog::onTableFormatChanged
 *  按用户选择的表格格式显示统计数据
 * @param checked
 */
void CurStatDialog::onTableFormatChanged(bool checked)
{
    if(checked)
        stateInfo.tFormat = COMMON;
    else
        stateInfo.tFormat = THREERAIL;
    viewTable();
}

/**
 * @brief CurStatDialog::onDetViewChanged
 *  在统计数据中是否显示明细科目的统计数据
 * @param checked
 */
void CurStatDialog::onDetViewChanged(bool checked)
{
    stateInfo.viewDetails = checked;
    ui->cmbSndSub->setEnabled(checked);
    viewTable();
}

void CurStatDialog::init(Account *acc)
{
    account = statUtil->getAccount();
    //smg = account->getSubjectManager();
    smg = statUtil->getSubjectManager();
    ui->cmbFstSub->setSubjectManager(smg);
    ui->cmbFstSub->setSubjectClass();
    ui->cmbFstSub->insertItem(0,tr("所有"));
    ui->cmbSndSub->setSubjectManager(smg);
    ui->cmbSndSub->setSubjectClass(SubjectSelectorComboBox::SC_SND);

    //初始化货币代码列表，并使它们以一致的顺序显示
    mts = account->getAllMoneys().keys();
    mts.removeOne(account->getMasterMt()->code());
    qSort(mts.begin(),mts.end()); //为了使人民币总是第一个

    //初始化一级科目组合框
    disconnect(ui->cmbFstSub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelFstSub(int)));
    disconnect(ui->cmbSndSub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    disconnect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int))
            ,this, SLOT(colWidthChanged(int,int,int)));

    //ui->cmbFstSub->clear();
    //ui->cmbFstSub->addItem(tr("所有"),0);
    //FSubItrator* fsubIt = smg->getFstSubItrator();
    //QVariant v;
    //while(fsubIt->hasNext()){
    //    fsubIt->next();
    //    v.setValue<FirstSubject*>(fsubIt->value());
    //    ui->cmbFstSub->addItem(fsubIt->value()->getName(),v);
    //}
    //ui->cmbSndSub->clear();
    ui->cmbSndSub->addItem(tr("所有"),0);
    fsub = NULL; ssub = NULL;


    ui->lbly->setText(QString::number(statUtil->year()));
    ui->lblm->setText(QString::number(statUtil->month()));
    ui->cmbFstSub->setCurrentIndex(0);
    connect(ui->cmbFstSub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelFstSub(int)));
    connect(ui->cmbSndSub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int))
            ,this, SLOT(colWidthChanged(int,int,int)));

}


void CurStatDialog::initHashs()
{
    //allFSubs = smg->getAllFstSubHash();
    //allSSubs = smg->getAllSndSubHash();
    allMts = account->getAllMoneys();
    int y = statUtil->year();
    int m = statUtil->month();
    account->getRates(y,m,sRates);
    int yy,mm;
    if(m == 12){
        yy = y+1;
        mm = 1;
    }
    else{
        yy = y;
        mm = m+1;
    }
    account->getRates(yy,mm,eRates);

    //期初余额及其方向
    preExa = statUtil->getPreValueFPm();
    preExaDir = statUtil->getPreDirF();
    preDetExa = statUtil->getPreValueSPm();
    preDetExaDir = statUtil->getPreDirS();
    preExaR = statUtil->getPreValueFMm();
    preDetExaR = statUtil->getPreValueSMm();

    //读取以原币计的期初余额（包括本币和外币的余额）
    //BusiUtil::readExtraByMonth2(yy,mm,preExa,preExaDir,preDetExa,preDetExaDir); //读取期初数
    //读取以本币计的期初余额（包括本币和外币的余额）
    //BusiUtil::readExtraByMonth3(yy,mm,preExaR,preExaDirR,preDetExaR,preDetExaDirR); //读取期初数
    //读取外币期初余额（这些余额都以本币计，是每月发生统计后的精确值，且仅包含外币部分）
//    bool exist;
//    QHash<int,Double> es,eds;
//    BusiUtil::readExtraByMonth4(yy,mm,es,eds,exist);
//    //用精确值代替直接从原币币转换来的外币值
//    if(exist){
//        QHashIterator<int,Double>* it = new QHashIterator<int,Double>(es);
//        while(it->hasNext()){
//            it->next();
//            preExaR[it->key()] = it->value();
//        }
//        it = new QHashIterator<int,Double>(eds);
//        while(it->hasNext()){
//            it->next();
//            preDetExaR[it->key()] = it->value();
//        }
//    }

    PzsState pzsState = account->getSuiteMgr(account->getSuiteRecord(statUtil->year())->id)->getState(m);
    ui->lblPzsState->setText(pzsStates.value(pzsState));
    ui->lblPzsState->setToolTip(pzsStateDescs.value(pzsState));

    //获取本期发生额
    curJHpn = statUtil->getCurValueJFPm();
    curJHpnR = statUtil->getCurValueJFMm();
    curJDHpn = statUtil->getCurValueJSPm();
    curJDHpnR = statUtil->getCurValueJSMm();
    curDHpn = statUtil->getCurValueDFPm();
    curDHpnR = statUtil->getCurValueDFMm();
    curDDHpn = statUtil->getCurValueDSPm();
    curDDHpnR = statUtil->getCurValueDSMm();

//    //计算以原币计的本期发生额
//    int amount;
//    if(!BusiUtil::calAmountByMonth2(y,m,curJHpn,curDHpn,curJDHpn,curDDHpn,isCanSave,amount)){
//        qDebug() << "Don't get current happen cash amount!!";
//        return;
//    }
//    //计算以本币计的本期发生额
//    if(!BusiUtil::calAmountByMonth3(y,m,curJHpnR,curDHpnR,curJDHpnR,curDDHpnR,isCanSave,amount)){
//        qDebug() << "Don't get current happen cash amount!!";
//        return;
//    }


    QString info = tr("本期共有%1张凭证参予了统计。").arg(statUtil->count());
    emit infomation(info);

    //获取余额
    endExa = statUtil->getEndValueFPm();
    endExaR = statUtil->getEndValueFMm();
    endExaDir = statUtil->getEndDirF();
    endDetExa = statUtil->getEndValueSPm();
    endDetExaR = statUtil->getEndValueSMm();
    endDetExaDir = statUtil->getEndDirS();

//    //计算以原币计本期余额
//    BusiUtil::calCurExtraByMonth2(y,m,preExa,preDetExa,preExaDir,preDetExaDir,
//                                 curJHpn,curJDHpn,curDHpn,curDDHpn,
//                                 endExa,endDetExa,endExaDir,endDetExaDir);
//    //计算以本币计的本期余额
//    BusiUtil::calCurExtraByMonth3(y,m,preExaR,preDetExaR,preExaDir,preDetExaDir,
//                                 curJHpnR,curJDHpnR,curDHpnR,curDDHpnR,
//                                 endExaR,endDetExaR,endExaDirR,endDetExaDirR);

    //debug 输出应收-宁波佳利的前期余额、本期发生、期末余额及其方向
    QString title = "(new)YS-nbjl";
    qDebug()<<tr("%1--Pre:  rmb=%2, usd=%3, rmb-dir=%4, usd-dir=%5")
              .arg(title).arg(preDetExa.value(961).toString()).arg(preDetExa.value(962).toString())
              .arg(preDetExaDir.value(961)).arg(preDetExaDir.value(962));
    qDebug()<<tr("%1--PreR:  usdR=%2")
              .arg(title).arg(preDetExaR.value(962).toString());
    qDebug()<<tr("%1--CurJ: rmb=%2, usd=%3").arg(title).arg(curJDHpn.value(961).toString())
              .arg(curJDHpn.value(962).toString());
    qDebug()<<tr("%1--CurJR: usdR=%2").arg(title).arg(curJDHpnR.value(962).toString());
    qDebug()<<tr("%1--CurD: rmb=%2, usd=%3").arg(title).arg(curDDHpn.value(961).toString())
              .arg(curDDHpn.value(962).toString());
    qDebug()<<tr("%1--CurDR: usdR=%2").arg(title).arg(curDDHpnR.value(962).toString());
    qDebug()<<tr("%1--End: rmb=%2, usd=%3, rmb-dir=%4, usd-dir=%5").arg(title)
              .arg(endDetExa.value(961).toString()).arg(endDetExa.value(962).toString())
              .arg(endDetExaDir.value(961)).arg(endDetExaDir.value(962));
    qDebug()<<tr("%1--EndR: usdR=%2").arg(title)
              .arg(endDetExaR.value(962).toString());
}

/**
 * @brief CurStatDialog::viewRates
 *  显示期初和期末汇率
 */
void CurStatDialog::viewRates()
{
    QHashIterator<int,Double> it(sRates);
    QVariant v;
    Money* mt;
    while(it.hasNext()){
        it.next();
        mt = allMts.value(it.key());
        if(mt != account->getMasterMt()){
            v.setValue<Money*>(mt);
            ui->cmbStartRate->addItem(mt->name(),v);
            ui->cmbEndRate->addItem(mt->name(),v);
        }
    }
    ui->cmbStartRate->setCurrentIndex(0);
    ui->cmbEndRate->setCurrentIndex(0);
    mt = ui->cmbStartRate->itemData(0).value<Money*>();
    ui->edtStartRate->setText(sRates.value(mt->code()).toString());
    ui->edtEndRate->setText(eRates.value(mt->code()).toString());
}

/**
 * @brief CurStatDialog::viewTable
 *  在表格中显示统计的数据（期初余额、本期发生额及其余额）
 */
void CurStatDialog::viewTable()
{
    genHeaderDatas();
    genDatas();
    dataModel->setHorizontalHeaderModel(headerModel);
    ui->tview->setModel(dataModel);

    disconnect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int))
            ,this, SLOT(colWidthChanged(int,int,int)));
    //设置列宽
    TableFormat curFmt = stateInfo.tFormat;
    for(int i = 0; i < stateInfo.colWidths.value(curFmt).count(); ++i)
        ui->tview->setColumnWidth(i,stateInfo.colWidths.value(curFmt)[i]);
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int))
            ,this, SLOT(colWidthChanged(int,int,int)));

    //决定保存按钮的启用状态
    if(account->isReadOnly()){
        ui->btnSave->setEnabled(false);
        return;
    }
    //应根据凭证集的状态和当前选择的科目的范围来决定保存按钮是否启用
    PzsState state = account->getSuiteMgr(account->getSuiteRecord(statUtil->year())->id)->getState(statUtil->month());
    isCanSave = (state == Ps_AllVerified);
    ui->btnSave->setEnabled(!fsub && !ssub && isCanSave);
}

/**
 * @brief CurStatDialog::genHeaderDatas
 *  生成表头数据
 */
void CurStatDialog::genHeaderDatas()
{
    QStandardItem* mitem;
    QStandardItem* item;
    QList<QStandardItem*> l1,l2;//l1用于存放表头第一级，l2存放表头第二级

    if(headerModel)
        delete headerModel;
    headerModel = new QStandardItemModel;

    //设置表头
    if(ui->rdoJe->isChecked()){ //金额式
        item = new QStandardItem(tr("科目编码"));    //0
        l1<<item;
        item = new QStandardItem(tr("科目名称"));    //1
        l1<<item;
        item = new QStandardItem(tr("方向"));       //2
        l1<<item;
        item = new QStandardItem(tr("期初金额"));    //3
        l1<<item;
        item = new QStandardItem(tr("本期借方发生")); //4
        l1<<item;
        item = new QStandardItem(tr("本期贷方发生")); //5
        l1<<item;
        item = new QStandardItem(tr("方向"));        //6
        l1<<item;
        item = new QStandardItem(tr("期末金额"));     //7
        l1<<item;
    }
    else{ //外币金额式
        mitem = new QStandardItem(tr("科目编码"));            //0
        l1<<mitem;
        mitem = new QStandardItem(tr("科目名称"));            //1
        l1<<mitem;
        mitem = new QStandardItem(tr("方向"));               //2
        l1<<mitem;
        mitem = new QStandardItem(tr("期初金额"));
        l1<<mitem;
        for(int i = 0; i < mts.count(); ++i){
            item = new QStandardItem(allMts.value(mts.at(i))->name()); //3 期初金额（外币）
            l2<<item;
            mitem->appendColumn(l2);
            l2.clear();
        }
        item = new QStandardItem(tr("金额"));                //4 期初金额（金额）
        l2<<item;
        mitem->appendColumn(l2);
        l2.clear();

        mitem = new QStandardItem(tr("本期借方发生"));
        l1<<mitem;
        for(int i = 0; i < mts.count(); ++i){
            item = new QStandardItem(allMts.value(mts.at(i))->name()); //5 借方（外币）
            l2<<item;
            mitem->appendColumn(l2);
            l2.clear();
        }
        item = new QStandardItem(tr("金额"));                //6借方（金额）
        l2<<item;
        mitem->appendColumn(l2);
        l2.clear();

        mitem = new QStandardItem(tr("本期贷方发生"));
        l1<<mitem;
        for(int i = 0; i < mts.count(); ++i){
            item = new QStandardItem(allMts.value(mts.at(i))->name()); //7 贷方（外币）
            l2<<item;
            mitem->appendColumn(l2);
            l2.clear();
        }
        item = new QStandardItem(tr("金额"));                //8 贷方（金额）
        l2<<item;
        mitem->appendColumn(l2);
        l2.clear();

        mitem = new QStandardItem(tr("方向"));               //9 余额方向
        l1<<mitem;

        mitem = new QStandardItem(tr("期末金额"));
        l1<<mitem;
        for(int i = 0; i < mts.count(); ++i){
            item = new QStandardItem(allMts.value(mts.at(i))->name()); //10 余额（外币）
            l2<<item;
            mitem->appendColumn(l2);
            l2.clear();
        }
        item = new QStandardItem(tr("金额"));                //11 余额（金额）
        l2<<item;
        mitem->appendColumn(l2);
        l2.clear();
    }
    headerModel->appendRow(l1);
    l1.clear();
}

/**
 * @brief CurStatDialog::genDatas
 *  生成统计数据
 */
void CurStatDialog::genDatas()
{
    if(dataModel)
        delete dataModel;
    dataModel = new MyWithHeaderModels;

    QList<QStandardItem*> items;
    ApStandardItem *item;
    QString tips;
    int masterMt = account->getMasterMt()->code();
    Double v = 0.00;

    //因为需要以科目代码的顺序来显示科目余额，因此，必须先获取此科目id序列
    QList<int> ids;
    if(!fsub)
        for(int i = 1; i < ui->cmbFstSub->count(); ++i)
            ids<<ui->cmbFstSub->itemData(i).value<FirstSubject*>()->getId();
    else
        ids<<fsub->getId();

    //这些hash表表示的是各个科目下的各币种按本币计的合计金额
    //本来各币种分开核算，那么键就要考虑币种因数，而这些hash的key为总账或明细账科目id
    QHash<int,Double> spreExa = statUtil->getSumPreValueF();            //期初余额
    QHash<int,Double> spreDetExa = statUtil->getSumPreValueS();
    QHash<int,MoneyDirection> spreExaDir = statUtil->getSumPreDirF();   //期初余额方向
    QHash<int,MoneyDirection> spreDetExaDir = statUtil->getSumPreDirS();

    QHash<int,Double> scurJHpn = statUtil->getSumCurValueJF();          //当期借贷发生额
    QHash<int,Double> scurJDHpn = statUtil->getSumCurValueJS();
    QHash<int,Double> scurDHpn = statUtil->getSumCurValueDF();
    QHash<int,Double> scurDDHpn = statUtil->getSumCurValueDS();

    QHash<int,Double> sendExa =  statUtil->getSumEndValueF();           //期末余额
    QHash<int,Double> sendDetExa = statUtil->getSumEndValueS();
    QHash<int,MoneyDirection> sendExaDir = statUtil->getSumEndDirF();   //期末余额方向
    QHash<int,MoneyDirection> sendDetExaDir = statUtil->getSumEndDirS();

    //显示数据
    FirstSubject* fs; SecondSubject* ss;
    Double jsums;Double dsums;  //借、贷合计值
    if(ui->rdoJe->isChecked()){ //金额式
        //输出数据
        for(int i = 0; i < ids.count(); ++i){
            if(sendExa.contains(ids.at(i))){
                //共8列
                fs = smg->getFstSubject(ids.at(i));
                items<<new ApStandardItem(fs->getCode()); //0 科目代码
                items<<new ApStandardItem(fs->getName()); //1 科目名称
                items<<new ApStandardItem(dirStr(spreExaDir.value(ids.at(i)))); //2 期初方向
                items<<new ApStandardItem(spreExa.value(ids.at(i))); //3 期初金额
                v = scurJHpn.value(ids.at(i)); //4 本期借方发生
                jsums += v;
                items<<new ApStandardItem(v);
                v = scurDHpn.value(ids.at(i));//5 本期贷方发生
                dsums += v;
                items<<new ApStandardItem(v);
                items<<new ApStandardItem(dirStr(sendExaDir.value(ids.at(i))));//6 期末方向
                items<<new ApStandardItem(sendExa.value(ids.at(i)));//7 期末余额
                setTableRowBackground(TRT_FSUB,items);
                dataModel->appendRow(items);
                items.clear();//至此，主科目余额加载完毕

                //如果需要显示明细科目的余额及其本期发生额，则
                if(ui->chkIsDet->isChecked()){
                    QList<int> sids;
                    if(!ssub){
                        QHash<int,QString> ssNames; //子目id到名称的映射
                        FirstSubject* fsub = smg->getFstSubject(ids.at(i));
                        foreach(SecondSubject* ssub,fsub->getChildSubs(SORTMODE_NAME)){
                            ssNames[ssub->getId()] = ssub->getName();
                            sids<<ssub->getId();
                        }
                        //sids = ssNames.keys();
                        //qSort(sids.begin(),sids.end());
                    }
                    else
                        sids<<ssub->getId();
                    for(int j = 0; j < sids.count(); ++j){
                        if(sendDetExa.contains(sids.at(j))){
                            ss = smg->getSndSubject(sids.at(j));
                            items<<new ApStandardItem(fs->getCode() + ss->getCode());//0 科目代码
                            items<<new ApStandardItem(ss->getName());//1 科目名称
                            items<<new ApStandardItem(dirStr(spreDetExaDir.value(sids.at(j))));//2 期初方向
                            v = spreDetExa.value(sids.at(j)); //3 期初金额
                            items<<new ApStandardItem(v);
                            v = scurJDHpn.value(sids.at(j));//4 本期借方发生（所有币种类型的发生额合计）
                            items<<new ApStandardItem(v);
                            v = scurDDHpn.value(sids.at(j)); //5 本期贷方发生
                            items<<new ApStandardItem(v);
                            items<<new ApStandardItem(dirStr(sendDetExaDir.value(sids.at(j))));//6 期末方向
                            v = sendDetExa.value(sids.at(j));//7 期末余额
                            items<<new ApStandardItem(v);
                            setTableRowBackground(TRT_SSUB,items);
                            dataModel->appendRow(items);
                            items.clear();
                        }
                    }
                }
            }
        }

        //加入合计行
        items<<new ApStandardItem(tr("合  计"));
        items<<new ApStandardItem<<new ApStandardItem<<new ApStandardItem;
        items<<new ApStandardItem(jsums);
        items<<new ApStandardItem(dsums);
        items<<new ApStandardItem<<new ApStandardItem;
        setTableRowBackground(TRT_SUM,items);
        dataModel->appendRow(items);
        items.clear();
     }
     else{//外币金额式（共12列）
        QHash<int,Double> curJSums,curDSums;   //本期借贷方合计值（按币种合计，key为币种代码）
        for(int i = 0; i < ids.count(); ++i){
            if(sendExa.contains(ids.at(i))){
                fs = smg->getFstSubject(ids.at(i));
                items<<new ApStandardItem(fs->getCode()); //0 科目代码
                items<<new ApStandardItem(fs->getName());//1 科目名称
                items<<new ApStandardItem(dirStr(spreExaDir.value(ids.at(i))));//2 期初方向
                for(int k = 0; k<mts.count(); ++k)
                    items<<new ApStandardItem(preExa.value(ids.at(i)*10+mts.at(k))); //3 期初金额（外币部分）
                item = new ApStandardItem(spreExa.value(ids.at(i)));
                tips = tr("人民币：%1  %2\n美金：%3  %4  %5")
                        .arg(dirStr(preExaDir.value(ids.at(i)*10+masterMt)))
                        .arg(preExa.value(ids.at(i)*10+masterMt).toString2())
                        .arg(dirStr(preExaDir.value(ids.at(i)*10+USD)))
                        .arg(preExa.value(ids.at(i)*10+USD).toString2())
                        .arg(preExaR.value(ids.at(i)*10+USD).toString2());
                item->setData(tips,Qt::ToolTipRole);
                items<<item;//4 期初金额（总额部分）
                for(int k = 0; k<mts.count(); ++k){
                    v = curJHpn.value(ids.at(i)*10+mts.at(k)); //5 本期借方发生（外币部分）
                    curJSums[mts.value(k)] += v;
                    items<<new ApStandardItem(v);
                }
                v = scurJHpn.value(ids[i]); //6 本期借方发生（各币种合计总额部分）
                jsums += v;
                item = new ApStandardItem(v);
                tips = tr("人民币：%1\n美金：%2  %3")
                        .arg(curJHpn.value(ids.at(i)*10+masterMt).toString2())
                        .arg(curJHpn.value(ids.at(i)*10+USD).toString2())
                        .arg(curJHpnR.value(ids.at(i)*10+USD).toString2());
                item->setData(tips,Qt::ToolTipRole);
                items<<item;
                for(int k = 0; k<mts.count(); ++k){
                    v = curDHpn.value(ids.at(i)*10+mts.at(k));
                    curDSums[mts.at(k)] += v;
                    items<<new ApStandardItem(v);//7 本期贷方发生（外币部分）
                }
                v = scurDHpn.value(ids.at(i));//8 本期贷方发生（各币种合计总额部分）
                dsums += v;
                item = new ApStandardItem(v);
                tips = tr("人民币：%1\n美金：%2  %3")
                        .arg(curDHpn.value(ids.at(i)*10+RMB).toString2())
                        .arg(curDHpn.value(ids.at(i)*10+USD).toString2())
                        .arg(curDHpnR.value(ids.at(i)*10+USD).toString2());
                item->setData(tips,Qt::ToolTipRole);
                items<<item;
                items<<new ApStandardItem(dirStr(sendExaDir.value(ids.at(i))));//9 期末方向

                for(int k = 0; k<mts.count(); ++k){
                    v = endExa.value(ids.at(i)*10+mts.at(k));
                    items<<new ApStandardItem(v);//10 期末余额（外币部分）
                }

                v = sendExa.value(ids.at(i)); //11 期末余额（总额部分）
                item = new ApStandardItem(v);
                tips = tr("人民币：%1  %2\n美金：%3  %4  %5")
                        .arg(dirStr(endExaDir.value(ids.at(i)*10+masterMt)))
                        .arg(endExa.value(ids.at(i)*10+masterMt).toString2())
                        .arg(dirStr(endExaDir.value(ids.at(i)*10+USD)))
                        .arg(endExa.value(ids.at(i)*10+USD).toString2())
                        .arg(endExaR.value(ids.at(i)*10+USD).toString2());
                item->setData(tips,Qt::ToolTipRole);
                items<<item;
                setTableRowBackground(TRT_FSUB,items);
                dataModel->appendRow(items);
                items.clear();//至此，主科目余额加载完毕

                //如果需要显示明细科目的余额及其本期发生额，则
                if(ui->chkIsDet->isChecked()){
                    QList<int> sids;
                    if(!ssub){
                        QHash<int,QString> ssNames; //子目id到名称的映射
                        FirstSubject* fsub = smg->getFstSubject(ids.at(i));
                        foreach(SecondSubject* ssub,fsub->getChildSubs(SORTMODE_NAME)){
                            ssNames[ssub->getId()] = ssub->getName();
                            sids<<ssub->getId();
                        }
//                        foreach(SecondSubject* ssub,smg->getFstSubject(ids.at(i))->getChildSubs())
//                            ssNames[ssub->getId()] = ssub->getName();
//                        sids = ssNames.keys();
//                        qSort(sids.begin(),sids.end());
                    }
                    else
                        sids<<ssub->getId();
                    for(int j = 0; j < sids.count(); ++j){
                        if(sendDetExa.contains(sids.at(j))){
                            ss = smg->getSndSubject(sids.at(j));
                            items<<new ApStandardItem(fs->getCode()+ss->getCode());//0 科目代码
                            items<<new ApStandardItem(ss->getName());//1 科目名称
                            items<<new ApStandardItem(dirStr(spreDetExaDir.value(sids.at(j))));//2 期初方向
                            for(int k = 0; k<mts.count(); ++k){
                                v = preDetExa.value(sids.at(j)*10+mts.at(k)); //3 期初金额（外币部分）
                                items<<new ApStandardItem(v);
                            }
                            v = spreDetExa.value(sids.at(j));//4 期初金额（总额部分）
                            item = new ApStandardItem(v);
                            tips = tr("人民币：%1  %2\n美金：%3  %4  %5")
                                   .arg(dirStr(preDetExaDir.value(sids.at(j)*10+masterMt)))
                                   .arg(preDetExa.value(sids.at(j)*10+masterMt).toString2())
                                   .arg(dirStr(preDetExaDir.value(sids.at(j)*10+USD)))
                                   .arg(preDetExa.value(sids.at(j)*10+USD).toString2())
                                   .arg(preDetExaR.value(sids.at(j)*10+USD).toString2());
                            item->setData(tips,Qt::ToolTipRole);
                            items<<item;
                            for(int k = 0; k<mts.count(); ++k){
                                v = curJDHpn.value(sids.at(j)*10+mts.at(k));
                                items<<new ApStandardItem(v);//5 本期借方发生（外币部分）
                            }
                            v = scurJDHpn.value(sids[j]);//6 本期借方发生（总额部分）
                            item = new ApStandardItem(v);
                            tips = tr("人民币：%1\n美金：%2  %3")
                                    .arg(curJDHpn.value(sids.at(j)*10+masterMt).toString2())
                                    .arg(curJDHpn.value(sids.at(j)*10+USD).toString2())
                                    .arg(curJDHpnR.value(sids.at(j)*10+USD).toString2());
                            item->setData(tips,Qt::ToolTipRole);
                            items<<item;
                            for(int k = 0; k<mts.count(); ++k){
                                v = curDDHpn.value(sids.at(j)*10+mts.at(k));
                                items<<new ApStandardItem(v);//7 本期贷方发生（外币部分）
                            }
                            v = scurDDHpn.value(sids[j]);//8 本期贷方发生（总额部分）
                            item = new ApStandardItem(v);
                            tips = tr("人民币：%1\n美金：%2  %3")
                                    .arg(curDDHpn.value(sids.at(j)*10+masterMt).toString2())
                                    .arg(curDDHpn.value(sids.at(j)*10+USD).toString2())
                                    .arg(curDDHpnR.value(sids.at(j)*10+USD).toString2());
                            item->setData(tips,Qt::ToolTipRole);
                            items<<item;
                            items<<new ApStandardItem(dirStr(sendDetExaDir.value(sids[j])));//9 期末方向
                            for(int k = 0; k<mts.count(); ++k){
                                v = endDetExa.value(sids.at(j)*10+mts.at(k));//10 期末余额（外币部分）
                                items<<new ApStandardItem(v);
                            }
                            //11 期末余额（总额部分）
                            v = sendDetExa.value(sids.at(j));
                            item = new ApStandardItem(v);
                            tips = tr("人民币：%1  %2\n美金：%3  %4  %5")
                                    .arg(dirStr(endDetExaDir.value(sids.at(j)*10+masterMt)))
                                    .arg(endDetExa.value(sids.at(j)*10+masterMt).toString2())
                                    .arg(dirStr(endDetExaDir.value(sids.at(j)*10+USD)))
                                    .arg(endDetExa.value(sids.at(j)*10+USD).toString2())
                                    .arg(endDetExaR.value(sids.at(j)*10+USD).toString2());
                            item->setData(tips,Qt::ToolTipRole);
                            items<<item;
                            setTableRowBackground(TRT_SSUB,items);
                            dataModel->appendRow(items);
                            items.clear();
                        }
                    }
                }
            }
        }
        //加入合计行
        items<<new ApStandardItem(tr("合  计"));
        items<<new ApStandardItem<<new ApStandardItem;
        for(int k = 0; k <= mts.count(); ++k) //期初金额不需合计
            items<<new ApStandardItem;
        //借方合计值（外币部分）
        for(int k = 0; k < mts.count(); ++k){
            v = curJSums.value(mts.value(k));
            items<<new ApStandardItem(v);
        }
        //借方合计值（各币种合计）
        if(jsums != 0)
            items<<new ApStandardItem(jsums);
        else
            items<<new ApStandardItem;
        //贷方合计值（外币部分）
        for(int k = 0; k < mts.count(); ++k){
            v = curDSums.value(mts.value(k));
            items<<new ApStandardItem(v);
        }
        //贷方合计值（各币种合计）
        if(dsums != 0)
            items<<new ApStandardItem(dsums);
        else
            items<<new ApStandardItem;
        items<<new ApStandardItem;
        for(int k = 0; k < mts.count(); ++k)
            items<<new ApStandardItem;
        items<<new ApStandardItem;
        setTableRowBackground(TRT_SUM,items);
        dataModel->appendRow(items);
        items.clear();
     }//外币金额式
}

/**
 * @brief CurStatDialog::printCommon
 *  打印任务的公共操作
 * @param task
 * @param printer
 */
void CurStatDialog::printCommon(PrintTask task, QPrinter *printer)
{
    HierarchicalHeaderView* thv = new HierarchicalHeaderView(Qt::Horizontal);

    //创建打印模板实例
    QList<int> colw(stateInfo.colPriWidths.value(stateInfo.tFormat));
    PrintTemplateStat* pt = new PrintTemplateStat(dataModel,thv,&colw);
    pt->setAccountName(account->getLName());
    pt->setCreator(curUser->getName());
    pt->setPrintDate(QDate::currentDate());

    //设置打印机的页边距和方向
    printer->setPageMargins(stateInfo.margins.left,stateInfo.margins.top,
                            stateInfo.margins.right,stateInfo.margins.bottom,
                            stateInfo.margins.unit);
    printer->setOrientation(stateInfo.pageOrientation);

    PreviewDialog* dlg = new PreviewDialog(pt,STATPAGE,printer);

    if(task == PREVIEW){
        //接收打印页面设置的修改
        if(dlg->exec() == QDialog::Accepted){
            for(int i = 0; i < colw.count(); ++i)
                stateInfo.colPriWidths[stateInfo.tFormat][i] = colw[i];
            stateInfo.pageOrientation = printer->orientation();
            printer->getPageMargins(&stateInfo.margins.left,&stateInfo.margins.top,
                                    &stateInfo.margins.right,&stateInfo.margins.bottom,
                                    stateInfo.margins.unit);
        }
    }
    else if(task == TOPRINT){
        dlg->print();
    }
    else{
        QString filename;
        dlg->exportPdf(filename);
    }

    delete thv;
}

/**
 * @brief CurStatDialog::setTableRowBackground
 *  根据表格行的总类设置表格行的背景色
 * @param rt
 * @param l
 */
void CurStatDialog::setTableRowBackground(CurStatDialog::TableRowType rt, const QList<QStandardItem *> l)
{
    QBrush br;
    switch(rt){
    case TRT_FSUB:
        br = row_bk_fsub;
        break;
    case TRT_SSUB:
        br = row_bk_ssub;
        break;
    case TRT_SUM:
        br = row_bk_sum;
        break;
    }
    for(int i = 0; i < l.count(); ++i){
        l.at(i)->setBackground(br);
    }
}

/**
 * @brief 根据表格行的种类设置表格行的文本颜色
 * @param rt
 * @param l
 */
void CurStatDialog::setTableRowTextColor(CurStatDialog::TableRowType rt, const QList<QStandardItem *> l)
{
//    QColor color;
//    switch(rt){
//    case TRT_FSUB:
//        color = row_tc_fsub;
//        break;
//    case TRT_SSUB:
//        color = row_tc_ssub;
//        break;
//    case TRT_SUM:
//        color = row_tc_sum;
//        break;
//    }
//    for(int i = 0; i < l.count(); ++i){
//        l.at(i)->setData(color,Qt::ForegroundRole);

//    }
}

/**
 * @brief CurStatDialog::on_actPrint_triggered
 *  打印统计数据
 */
void CurStatDialog::on_actPrint_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::PrinterResolution);
    printCommon(TOPRINT, printer);
    delete printer;
}

/**
 * @brief CurStatDialog::on_actPreview_triggered
 *  打印预览
 */
void CurStatDialog::on_actPreview_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::HighResolution);
    printCommon(PREVIEW, printer);
    delete printer;
}

/**
 * @brief CurStatDialog::on_actToPDF_triggered
 *  打印到pdf文件
 */
void CurStatDialog::on_actToPDF_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::ScreenResolution);
    printCommon(TOPDF, printer);
    delete printer;
}

/**
 * @brief CurStatDialog::on_actToExcel_triggered
 *  导出统计数据表格到Excel文件
 */
void CurStatDialog::on_actToExcel_triggered()
{
    QMessageBox::information(this,tr("提示信息"),tr("本功能未实现！"));
}

void CurStatDialog::on_btnSave_clicked()
{
    save();
}

/**
 * @brief CurStatDialog::on_btnRefresh_clicked
 *  重新统计，并刷新显示
 */
void CurStatDialog::on_btnRefresh_clicked()
{
    stat();
}

void CurStatDialog::on_btnClose_clicked()
{
    //应根据余额是否改变，而提示用户是否保存余额（这个可通过询问凭证集管理对象或统计实用对象）
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}
