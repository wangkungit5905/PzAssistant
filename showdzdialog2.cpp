#include "ui_showdzdialog2.h"
#include "ui_subjectrangeselectdialog.h"
#include "showdzdialog2.h"
#include "account.h"
#include "previewdialog.h"
#include "HierarchicalHeaderView.h"
#include "subject.h"
#include "dbutil.h"

#include <QBuffer>
#include <QInputDialog>


ShowDZDialog2::ShowDZDialog2(Account* account,QByteArray* sinfo, QWidget *parent) :
    QDialog(parent),ui(new Ui::ShowDZDialog2),account(account)
{
    ui->setupUi(this);
    smg = account->getSubjectManager();
    allMts = account->getAllMoneys();
    mmtObj = account->getMasterMt();
    ui->pbr->setVisible(false);
    tfid = -1;tsid = -1;tmt = -1;
    //mt = RMB;
    otf = tf = NONE;
    preview = NULL;
    pDataModel = NULL;
    pt = NULL;

    actMoveTo = new QAction(tr("转到该凭证"), ui->tview);
    ui->tview->addAction(actMoveTo);
    connect(actMoveTo, SIGNAL(triggered()), this, SLOT(moveTo()));

    headerModel = NULL;
    pHeaderModel = NULL;
    dataModel = NULL;
    pDataModel = NULL;
    imodel = NULL;
    //ipmodel = NULL;
    hv = NULL;

    hv = new HierarchicalHeaderView(Qt::Horizontal, ui->tview);
    hv->setHighlightSections(true);
    hv->setClickable(true);
    ui->tview->setHorizontalHeader(hv);

    fcom = new SubjectComplete;
    ui->cmbFsub->setCompleter(fcom);
    scom = new SubjectComplete(SndSubject);
    ui->cmbSsub->setCompleter(scom);

    //初始化货币代码列表，并使它们以一致的顺序显示
    mts = allMts.keys();
    mts.removeOne(account->getMasterMt()->code());
    qSort(mts.begin(),mts.end());

    //    ui->cmbMt->addItem(tr("所有"),ALLMT);
    //    ui->cmbMt->addItem(allMts.value(RMB)->name(),RMB);
    //    for(int i = 0; i < mts.count(); ++i)
    //        ui->cmbMt->addItem(allMts.value(mts[i])->name(),mts[i]);

    ui->btnPrint->addAction(ui->actPrint);
    ui->btnPrint->addAction(ui->actPreview);
    ui->btnPrint->addAction(ui->actToPdf);

//    //设置时间范围的可选界限
//    ui->startDate->setMinimumDate(account->getStartDate());
//    ui->startDate->setMaximumDate(account->getEndDate());
//    ui->endDate->setMinimumDate(account->getStartDate());
//    ui->endDate->setMaximumDate(account->getEndDate());

    setState(sinfo);
    readFilters();
    //initSubjectItems();


    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));
    connect(ui->startDate,SIGNAL(dateChanged(QDate)),this,SLOT(startDateChanged(QDate)));
    connect(ui->endDate,SIGNAL(dateChanged(QDate)),this,SLOT(endDateChanged(QDate)));
}

ShowDZDialog2::~ShowDZDialog2()
{
    //delete fcom;  //会崩溃
    //delete scom;
    delete ui;
    if(!account->getDbUtil()->saveDetViewFilter(filters))
        QMessageBox::critical(this,tr("出错信息"),tr("保存历史过滤条件时出错！"));
    qDeleteAll(filters);
}

//设置视图的内部状态
void ShowDZDialog2::setState(QByteArray* info)
{
    if(info == NULL){
        //屏幕视图上的表格列宽
        colWidths[CASHDAILY]<<50<<50<<50<<50<<300<<50<<100<<100<<50<<100<<10<<10;
        colWidths[BANKRMB]<<50<<50<<50<<300<<0<<0<<100<<100<<50<<100<<0<<0;
        colWidths[BANKWB]<<50<<50<<50<<300<<0<<0;
        for(int i = 0; i < mts.count(); ++i) //汇率
            colWidths[BANKWB]<<50;
        for(int i = 0; i < mts.count()*2+2; ++i) //借贷方
            colWidths[BANKWB]<<100;
        colWidths[BANKWB]<<50; //方向
        for(int i = 0; i < mts.count()+1; ++i) //余额
            colWidths[BANKWB]<<100;
        colWidths[BANKWB]<<0<<0;
        colWidths[COMMON]<<50<<50<<50<<300<<100<<100<<50<<100<<0<<0;
        colWidths[THREERAIL]<<50<<50<<50<<300;
        for(int i = 0; i < mts.count(); ++i) //汇率
            colWidths[THREERAIL]<<50;
        for(int i = 0; i < mts.count()*2+2; ++i) //借贷方
            colWidths[THREERAIL]<<100;
        colWidths[THREERAIL]<<50; //方向
        for(int i = 0; i < mts.count()+1; ++i) //余额
            colWidths[THREERAIL]<<100;
        colWidths[THREERAIL]<<0<<0;

        //打印模板上的表格列宽
        colPrtWidths[CASHDAILY]<<50<<50<<50<<50<<300<<0<<100<<100<<50<<100<<0<<0;
        colPrtWidths[BANKRMB]<<50<<50<<50<<300<<0<<0<<100<<100<<50<<100<<0<<0;
        colPrtWidths[BANKWB]<<50<<50<<50<<300<<0<<0;
        for(int i = 0; i < mts.count(); ++i) //汇率
            colPrtWidths[BANKWB]<<50;
        for(int i = 0; i < mts.count()*2+2; ++i) //借贷方
            colPrtWidths[BANKWB]<<100;
        colPrtWidths[BANKWB]<<50; //方向
        for(int i = 0; i < mts.count()+1; ++i) //余额
            colPrtWidths[BANKWB]<<100;
        colPrtWidths[BANKWB]<<0<<0;
        colPrtWidths[COMMON]<<50<<50<<50<<300<<100<<100<<50<<100<<0<<0;
        colPrtWidths[THREERAIL]<<50<<50<<50<<300;
        for(int i = 0; i < mts.count(); ++i) //汇率
            colPrtWidths[THREERAIL]<<50;
        for(int i = 0; i < mts.count()*2+2; ++i) //借贷方
            colPrtWidths[THREERAIL]<<100;
        colPrtWidths[THREERAIL]<<50; //方向
        for(int i = 0; i < mts.count()+1; ++i) //余额
            colPrtWidths[THREERAIL]<<100;
        colPrtWidths[THREERAIL]<<0<<0;

        pageOrientation = QPrinter::Landscape;
        margins.unit = QPrinter::Didot;
        margins.left = 20; margins.right = 20;
        margins.top = 30; margins.bottom = 30;
    }
    else{
        QBuffer bf(info);
        QDataStream in(&bf);
        qint8 i8;
        qint16 i16;
        bf.open(QIODevice::ReadOnly);

        TableFormat tf;
        for(int i = 0; i < 5; ++i){
            in>>i8;  //表格格式枚举值代码
            tf = (TableFormat)i8;
            in>>i8;  //表格列数
            colWidths[tf].clear();
            colPrtWidths[tf].clear();
            for(int j = 0; j < i8; ++j){
                in>>i16;
                colWidths[tf]<<i16; //表格列宽
                in>>i16;
                colPrtWidths[tf]<<i16;
            }
        }

        //页面方向和页边距
        in>>i8;
        pageOrientation = (QPrinter::Orientation)i8;
        in>>i8;
        margins.unit = (QPrinter::Unit)i8;
        double d;
        in>>d;
        margins.left = d;
        in>>d;
        margins.right = d;
        in>>d;
        margins.top = d;
        in>>d;
        margins.bottom = d;

        bf.close();
    }
}

//获取视图的内部状态
QByteArray* ShowDZDialog2::getState()
{
    QByteArray* ba = new QByteArray;
    QBuffer bf(ba);
    QDataStream out(&bf);
    qint8 i8;
    qint16 i16;

    bf.open(QIODevice::WriteOnly);
    //按先表格格式的枚举值，后跟该表格格式的总列数，再后跟各列的宽度值（先屏幕，后打印模板），
    //隐藏列的宽度为0,最后是打印的页面方向和页边距。

    //现金日记账表格列数和宽度，总共11个字段，其中3个隐藏了（序号分别为3、9、10）
    i8 = CASHDAILY;  //格式代码
    out<<i8;
    i8 = 11;
    out<<i8;         //总列数
    for(int i = 0; i < 12; ++i){
        i16 = colWidths.value(CASHDAILY)[i];   //屏幕视图列宽
        out<<i16;
        i16 = colPrtWidths.value(CASHDAILY)[i];//打印视图列宽
        out<<i16;
    }

    //银行存款（人民币），总共12个字段，其中4个隐藏了（序号分别为4、5、10、11）
    i8 = BANKRMB;
    out<<i8;
    i8 = 12;
    out<<i8;
    for(int i = 0; i < 12; ++i){
        i16 = colWidths.value(BANKRMB)[i];
        out<<i16;
        i16 = colPrtWidths.value(BANKRMB)[i];
        out<<i16;
    }

    //银行存款（外币），总共16个字段，其中4个隐藏了（序号分别为4、5、14、15）在只有一种外币的情况下
    i8 = BANKWB;
    out<<i8;
    i8 = 12 + mts.count() * 4;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = colWidths.value(BANKWB)[i];
        out<<i16;
        i16 = colPrtWidths.value(BANKWB)[i];
        out<<i16;
    }//总共10个字段，其中2个隐藏了（序号分别为8、9）

    //通用金额式，总共10个字段，其中2个隐藏了（序号分别为8、9）
    i8 = COMMON;
    out<<i8;
    i8 = 10;
    out<<i8;
    for(int i = 0; i < 10; ++i){
        i16 = colWidths.value(COMMON)[i];
        out<<i16;
        i16 = colPrtWidths.value(COMMON)[i];
        out<<i16;
    }

    //三栏明细式，总共14个字段，其中2个隐藏了（序号分别为12、13）在只有一种外币的情况下
    i8 = THREERAIL;
    out<<i8;
    i8 = 10 + mts.count()*4;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = colWidths.value(THREERAIL)[i];
        out<<i16;
        i16 = colPrtWidths.value(THREERAIL)[i];
        out<<i16;
    }

    i8 = pageOrientation;
    out<<i8;
    i8 = (qint8)margins.unit;
    out<<i8;
    double d = margins.left; out<<d;
    d = margins.right; out<<d;
    d = margins.top; out<<d;
    d = margins.bottom; out<<d;

    bf.close();
    return ba;
}

/**
 * @brief ShowDZDialog2::curFilterChanged
 *  用户选择了一个过滤条件，则要刷新界面的过滤条件设置
 * @param current
 * @param previous
 */
void ShowDZDialog2::curFilterChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    curFilter = current->data(Qt::UserRole).value<DVFilterRecord*>();
    ui->btnSaveAs->setEnabled(curFilter->isDef);
    ui->btnDelFilter->setEnabled(!curFilter->isDef);
    if(curFilter->isFst)
        ui->lblSelMode->setText(tr("一级科目"));
    else
        ui->lblSelMode->setText(tr("二级科目"));
    initFilter();
}

/**
 * @brief ShowDZDialog2::startDateChanged
 *  开始日期改变
 * @param date
 */
void ShowDZDialog2::startDateChanged(const QDate &date)
{
    ui->endDate->setMinimumDate(date);
    curFilter->startDate = date;
    adjustSaveBtn();
}

void ShowDZDialog2::endDateChanged(const QDate &date)
{
    curFilter->endDate = date;
    adjustSaveBtn();
}



/**
 * @brief ShowDZDialog2::onSelFstSub
 *  当用户选择一个一级科目时，则装载该科目下的所有二级科目到二级科目选择组合框中
 * @param index
 */
void ShowDZDialog2::onSelFstSub(int index)
{
    curFSub = ui->cmbFsub->itemData(index).value<FirstSubject*>();
    curFilter->curFSub = curFSub?curFSub->getId():0;
    adjustSaveBtn();
    scom->setPid(curFilter->curFSub);
    subIds.clear();
    if(!curFSub){
        curSSub = NULL;
        subIds = curFilter->subIds;
    }

    disconnect(ui->cmbSsub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    disconnect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));

    //如果是选择所有一级科目，则不装载任何二级科目，否则装载该一级科目下的所有二级科目
    ui->cmbSsub->clear();
    QVariant v;
    if(curFSub){
        if(curFSub->getChildCount() > 1)
            ui->cmbSsub->addItem(tr("所有"));
        SecondSubject* ssub;
        for(int i = 0; i < curFSub->getChildCount(); ++i){
            ssub = curFSub->getChildSub(i);
            v.setValue<SecondSubject*>(ssub);
            ui->cmbSsub->addItem(ssub->getName(),v);
        }
    }
    else
        ui->cmbSsub->addItem(tr("所有"));
    onSelSndSub(0);
    connect(ui->cmbSsub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));

//    refreshTalbe();

//    QList<QString> names;
//    if((witch == 1) || (witch == 2)){
//        QList<int> ids;
//        //BusiUtil::getSndSubInSpecFst(fid,ids,names);

//        //初始化可选的明细科目列表
//        ui->cmbSsub->clear();
//        if(fid == subCashId){ //现金科目下，只有人民币子目，且币种为人民币
//            ui->cmbSsub->addItem(tr("人民币"),subCashRmbId);
//            ui->cmbMt->clear();
//            ui->cmbMt->addItem(tr("人民币"),RMB);
//        }
//        else{//如果该主目只有一个子目
//            if(ids.count() == 1){
//                ui->cmbSsub->addItem(sNames.value(fid)[0], sids.value(fid)[0]);
//            }
//            else{
//                ui->cmbSsub->addItem(tr("所有"), 0);
//                for(int i = 0; i < sids.value(fid).count(); ++i)
//                    ui->cmbSsub->addItem(sNames.value(fid)[i], sids.value(fid)[i]);
//            }
//            //装载属于该总账科目的所有子目id
//            //if((fid != 0) && sids.value(fid).empty())
//            //    sids[fid]<<ids;

//            //初始化可选的币种列表
//            ui->cmbMt->clear();
//            //如果主目是要按币种分别核算的科目或选择所有科目，则要添加所有币种
//            //if(BusiUtil::isAccMt(fid) || (fid == 0)){
//            if(fsub->isUseForeignMoney() || (fid == 0)){
//                ui->cmbMt->addItem(tr("所有"), ALLMT);
//                QList<int> mts = allMts.keys();
//                qSort(mts.begin(),mts.end());
//                for(int i = 0; i < mts.count(); ++i)
//                    ui->cmbMt->addItem(allMts.value(mts.at(i))->name(), mts.at(i));
//            }
//            else{
//                ui->cmbMt->addItem(tr("人民币"),RMB);
//            }
//        }

//    }
//    else{  //范围选择模式
//        //if(fid != 0)
//        //    BusiUtil::getSubSetNameInSpecFst(fid,sids.value(fid),names);
//        ui->cmbMt->clear();
//        ui->cmbSsub->clear();
//        if(fid == subCashId){  //如果是现金科目
//            ui->cmbSsub->addItem(tr("人民币"),subCashRmbId);
//            ui->cmbMt->addItem(tr("人民币"),RMB);
//        }
//        else{
//            if(sids.value(fid).count() == 1) //如果该主目只有一个子目
//                ui->cmbSsub->addItem(sNames.value(fid)[0], sids.value(fid)[0]);
//            else{  //否者加载所有选择的子目
//                ui->cmbSsub->addItem(tr("所有"), 0);
//                QList<SecondSubject*> ssubs = smg->getFstSubject(fid)->getChildSubs();
//                SecondSubject* ssub;
//                for(int i = 0; i < ssubs.count(); ++i){
//                    ssub = ssubs.at(i);
//                    ui->cmbSsub->addItem(ssub->getName(),ssub->getId());
//                }
//                //for(int i = 0; i < sids.value(fid).count(); ++i)
//                //    ui->cmbSsub->addItem(allSndSubs.value(sids.value(fid)[i]),
//                //                         sids.value(fid)[i]);
//            }

//            //初始化可选的币种列表
//            ui->cmbMt->clear();

//            //if(BusiUtil::isAccMt(fid) || (fid == 0)){  //如果主目是要按币种分别核算的科目，则要添加所有币种
//            if(fsub->isUseForeignMoney() || (fid == 0)){  //如果主目是要按币种分别核算的科目，则要添加所有币种
//                ui->cmbMt->addItem(tr("所有"), ALLMT);
//                QList<int> mts = allMts.keys();
//                qSort(mts.begin(),mts.end());
//                for(int i = 0; i < mts.count(); ++i)
//                    ui->cmbMt->addItem(allMts.value(mts[i])->name(), mts[i]);
//            }
//            else{
//                ui->cmbMt->addItem(tr("人民币"),RMB);
//            }
//        }
//    }

//    ui->cmbSsub->setCurrentIndex(0);
//    sid = ui->cmbSsub->itemData(0).toInt();
//    ui->cmbMt->setCurrentIndex(0); //所有
//    mt = ui->cmbMt->itemData(0).toInt();




}

/**
 * @brief ShowDZDialog2::onSelSndSub
 *  当用户选择一个二级科目时，根据一级科目的货币使用属性，调整可用的货币选项
 * @param index
 */
void ShowDZDialog2::onSelSndSub(int index)
{
    curSSub = ui->cmbSsub->itemData(index).value<SecondSubject*>();
    curFilter->curSSub = curSSub?curSSub->getId():0;
    adjustSaveBtn();
    subIds.clear();
    //如果在一级科目选择模式下选择所有一级科目，或在二级科目选择模式下选择所有科目，
    //则可以利用当前过滤中的已选择科目的id列表
    if((curFilter->isFst && !curFSub) || (!curFilter->isFst && !curSSub))
        subIds = curFilter->subIds;
    //如果在一级科目选择模式下选择某个一级科目下的所有科目
    else if(curFilter->isFst && curFSub && !curSSub){
        for(int i = 0; i < curFSub->getChildCount();++i)
            subIds<<curFSub->getChildSub(i)->getId();
    }
    //其他的选择情况都是选择了一个确定的一级科目和二级科目的组合，这就不需要科目id列表了

    ui->cmbMt->clear();
    QVariant v;
    if(curFSub && !curFSub->isUseForeignMoney()){
        v.setValue<Money*>(mmtObj);
        ui->cmbMt->addItem(mmtObj->name(),v);
    }
    //如果是银行子科目，则根据子科目名的币名后缀,自动设置对应币种
    else if(curSSub && (curFSub->getId() == smg->getBankSub()->getId())){
        QString name = curSSub->getName();
        int idx = name.indexOf('-');
        name = name.right(name.count()-idx-1);
        QHashIterator<int,Money*> it(allMts);
        while(it.hasNext()){
            it.next();
            if(name == it.value()->name()){
                curMt = it.value();
                v.setValue<Money*>(curMt);
                ui->cmbMt->addItem(curMt->name(),v);
                break;
            }
        }
    }
    else{
        ui->cmbMt->addItem(tr("所有"));
        v.setValue<Money*>(mmtObj);
        ui->cmbMt->addItem(mmtObj->name(),v);
        Money* mt;
        for(int i = 0; i < mts.count(); ++i){
            mt = allMts.value(mts.at(i));
            v.setValue<Money*>(mt);
            ui->cmbMt->addItem(mt->name(),v);
        }
    }
    onSelMt(0);

//    sid = ssub->getId();
//    disconnect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));
//    ui->cmbMt->clear();

//    //如果是现金、应收/应付，或选择银行存款下的所有项目则添加所有币种
//    if((fid == subCashId) || (fid == subYsId) ||
//            (fid == subYfId) || (index == 0)){
//        ui->cmbMt->addItem(tr("所有"),ALLMT);
//        ui->cmbMt->addItem(allMts.value(RMB), RMB);
//        for(int i = 0; i < mts.count(); ++i)
//            ui->cmbMt->addItem(allMts.value(mts[i]), mts[i]);
//        ui->cmbMt->setCurrentIndex(0);
//        mt = ALLMT;
//    }
//    //如果是银行子科目，则根据子科目名的币名后缀,自动设置对应币种
//    if(fid == subBankId){
//        disconnect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));
//        ui->cmbMt->clear();
//        if(sid != 0){
//            QString name = ui->cmbSsub->currentText();
//            int idx = name.indexOf('-');
//            name = name.right(name.count()-idx-1);
//            QHashIterator<int,Money*> it(allMts);
//            while(it.hasNext()){
//                it.next();
//                if(name == it.value()->name()){
//                    ui->cmbMt->addItem(it.value()->name(),it.key());
//                    mt = it.key();
//                    break;
//                }
//            }
//        }
//        else{
//            ui->cmbMt->addItem(tr("所有"), 0);
//            QList<int> mts = allMts.keys();
//            qSort(mts.begin(),mts.end());
//            for(int i = 0; i < mts.count(); ++i)
//                ui->cmbMt->addItem(allMts.value(mts[i])->name(),mts[i]);
//        }

//        connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));
//    }
//    //如果是其他科目，则只选人民币
//    else{
//        ui->cmbMt->addItem(allMts.value(RMB),RMB);
//        mt = RMB;
//    }

//    connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));
//    refreshTalbe();
}

//用户选择币种
void ShowDZDialog2::onSelMt(int index)
{
    curMt = ui->cmbMt->itemData(index).value<Money*>();
    curFilter->curMt = curMt?curMt->code():0;
    adjustSaveBtn();
    refreshTalbe();
}

//转到该凭证
void ShowDZDialog2::moveTo()
{
    QItemSelectionModel* selModel = ui->tview->selectionModel();
    if(selModel->hasSelection()){
        int row = selModel->currentIndex().row();
        int pidCol;
        switch(tf){
        case CASHDAILY:
            pidCol = 9;
            break;
        case BANKRMB:
            pidCol = 10;
            break;
        case BANKWB:
            pidCol = 14;
            break;
        case COMMON:
            pidCol = 9;
            break;
        case THREERAIL:
            pidCol = 12;
            break;
        }
        int pid = imodel->data(imodel->index(row, pidCol)).toInt();
        int bid =  imodel->data(imodel->index(row, pidCol+1)).toInt();
        emit  openSpecPz(pid, bid);

    }
}

//当用户改变表格的列宽时，记录在内部状态表中
void ShowDZDialog2::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    colWidths[tf][logicalIndex] = newSize;
}


//更新明细账表格数据（生成页面数据）
//参数 pageNum = 0时表示刷新视图的表格内容，其他大于0的值，表示请求刷新指定打印页的表格内容
void ShowDZDialog2::refreshTalbe()
{
    disconnect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));

    int fid = curFSub?curFSub->getId():0;
    int sid = curSSub?curSSub->getId():0;
    int mt = curMt?curMt->code():0;

    //当对象处于初始化状态时，不允许调用refreshTable方法
    //if((tfid == fid) && (tsid == sid) && tmt == mt)
    //    return;

    //刷新视图的表格内容
    otf = tf; //保存原先的表格格式
    //先将原先隐藏的列显示（主要是为了能够正确地绘制表头，否则将会丢失某些列）
    switch(otf){
    case CASHDAILY:
        if(!viewHideColInDailyAcc1)
            ui->tview->showColumn(4);
        if(!viewHideColInDailyAcc2){
            ui->tview->showColumn(9);
            ui->tview->showColumn(10);
        }
        break;
    case BANKRMB:
        if(!viewHideColInDailyAcc1){
            ui->tview->showColumn(4);
            ui->tview->showColumn(5);
        }
        if(!viewHideColInDailyAcc2){
            ui->tview->showColumn(10);
            ui->tview->showColumn(11);
        }
        break;
    case BANKWB:
        if(!viewHideColInDailyAcc1){
            ui->tview->showColumn(4);
            ui->tview->showColumn(5);
        }
        if(!viewHideColInDailyAcc2){
            ui->tview->showColumn(11 + mts.count()*3);
            ui->tview->showColumn(12 + mts.count()*3);
        }
        break;
    case COMMON:
        if(!viewHideColInDailyAcc2){
            ui->tview->showColumn(8);
            ui->tview->showColumn(9);
        }
        break;
    case THREERAIL:
        if(!viewHideColInDailyAcc2){
            ui->tview->showColumn(9+mts.count()*3);
            ui->tview->showColumn(10+mts.count()*3);
        }
        break;
    }


    //判定要采用的表格格式
    tf = decideTableFormat(fid,sid,mt);

    if(headerModel){
        delete headerModel;
        headerModel = NULL;
    }
    headerModel = new QStandardItemModel;

    if(dataModel){
        delete dataModel;
        dataModel = NULL;
    }
    dataModel = new QStandardItemModel;

    //生成表头和表格数据内容
    QList<DailyAccountData2*> datas;    //数据
    QHash<int,Double> preExtra;         //期初余额（按币种）
    QHash<int,Double> preExtraR;        //外币科目的期初本币形式余额（按币种）
    QHash<int,int> preExtraDir;         //期初余额方向（按币种）
    QHash<int, Double> rates;           //每月汇率（其中期初汇率直接用币种代码作为键
                                        //而其他月份汇率，则是：月份*10+币种代码）
    //混合了各币种余额值和方向
    Double prev;  //期初余额
    int preDir;   //期初余额方向
    //if(!BusiUtil::getDailyAccount2(cury,sm,em,fid,sid,mt,prev,preDir,datas,
    //                              preExtra,preExtraR,preExtraDir,rates,fids,sids,gv,lv,inc))
    //    return;
    QHash<int,SubjectManager*> smgs;
    for(int y = ui->startDate->date().year(); y <= ui->endDate->date().year(); ++y){
        smgs[y] = account->getSubjectManager(account->getSuite(y)->subSys);
    }
    if(!account->getDbUtil()->getDailyAccount2(smgs,ui->startDate->date(),ui->endDate->date(),fid,sid,mt,prev,preDir,datas,
                                  preExtra,preExtraR,preExtraDir,rates,subIds,gv,lv,inc))
        return;

    pdatas.clear();  //还要注意删除列表内的每个元素对象
    switch(tf){
    case CASHDAILY:
        genThForCash();
        genDataForCash(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
        break;
    case BANKRMB:
        genThForBankRmb();
        genDataForBankRMB(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
        break;
    case BANKWB:
        genThForBankWb();
        genDataForBankWb(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
        break;
    case COMMON:
        genThForDetails();
        genDataForDetails(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
        break;
    case THREERAIL:
        genThForWai();
        genDataForDetWai(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
        break;
    }
    foreach(QList<QStandardItem*> l, pdatas)
        dataModel->appendRow(l);

    if(imodel){
        delete imodel;
        //imodel = NULL;
    }
    imodel = new ProxyModelWithHeaderModels;

    imodel->setModel(dataModel);
    imodel->setHorizontalHeaderModel(headerModel);

    ui->tview->setModel(imodel); //它会调用verticalHeader->setModel(model)

    //隐藏列
    switch(tf){
    case CASHDAILY:
        if(!viewHideColInDailyAcc1)
            ui->tview->hideColumn(4);
        if(!viewHideColInDailyAcc2){
            ui->tview->hideColumn(9);
            ui->tview->hideColumn(10);
        }
        break;
    case BANKRMB:
        if(!viewHideColInDailyAcc1){
            ui->tview->hideColumn(4);
            ui->tview->hideColumn(5);
        }
        if(!viewHideColInDailyAcc2){
            ui->tview->hideColumn(10);
            ui->tview->hideColumn(11);
        }
        break;
    case BANKWB:
        if(!viewHideColInDailyAcc1){
            ui->tview->hideColumn(4);
            ui->tview->hideColumn(5);
        }
        if(!viewHideColInDailyAcc2){
            ui->tview->hideColumn(11 + mts.count()*3);
            ui->tview->hideColumn(12 + mts.count()*3);
        }
        break;
    case COMMON:
        if(!viewHideColInDailyAcc2){
            ui->tview->hideColumn(8);
            ui->tview->hideColumn(9);
        }
        break;
    case THREERAIL:
        if(!viewHideColInDailyAcc2){
            ui->tview->hideColumn(9+mts.count()*3);
            ui->tview->hideColumn(10+mts.count()*3);
        }
        break;
    }

    //设置列宽

    for(int i = 0; i < colWidths.value(tf).count(); ++i)
        ui->tview->setColumnWidth(i, colWidths.value(tf)[i]);
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));

    tfid = fid; tsid = sid; tmt = mt;
}

/**
 * @brief ShowDZDialog2::readFilters
 */
void ShowDZDialog2::readFilters()
{
    if(!account->getDbUtil()->getDetViewFilters(filters))
        QMessageBox::critical(this,tr("出错信息"),tr("读取明细账的历史过滤条件时出错"));
    if(filters.isEmpty()){
        curFilter = new DVFilterRecord;
        curFilter->editState = CIES_NEW;
        curFilter->id = 0;
        curFilter->isDef = true;
        curFilter->isFst = true;
        curFilter->isCur = true;
        curFilter->curFSub = 0;
        curFilter->curSSub = 0;
        curFilter->curMt = account->getMasterMt()->code();
        curFilter->name = tr("默认");
        curFilter->startDate = account->getStartDate();
        curFilter->endDate = account->getEndDate();
        filters<<curFilter;
        if(!account->getDbUtil()->saveDetViewFilter(filters))
            QMessageBox::critical(this,tr("出错信息"),tr("在保存明细账的默认历史过滤条件时出错"));
    }
    QListWidgetItem *item,*curItem;
    QVariant v;
    foreach(DVFilterRecord* r, filters){
        v.setValue<DVFilterRecord*>(r);
        item = new QListWidgetItem(r->name);
        item->setData(Qt::UserRole,v);
        ui->lstHistory->addItem(item);
        if(r->isCur)
            curItem = item;
    }
    connect(ui->lstHistory,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this,SLOT(curFilterChanged(QListWidgetItem*,QListWidgetItem*)));
    ui->lstHistory->setCurrentItem(curItem);
}

/**
 * @brief ShowDZDialog2::initFilter
 *  根据当前选中的过滤条件项目，设置窗明细账口的过滤条件（时间范围、科目范围、当前选中的科目和币种等）
 */
void ShowDZDialog2::initFilter()
{
    //时间范围
    ui->startDate->setDate(curFilter->startDate);
    ui->endDate->setDate(curFilter->endDate);
    initSubjectList();
    initSubjectItems();    
}

/**
 * @brief ShowDZDialog2::initSubjectItems
 *  根据当前过滤条件的科目范围设定，装载选定的科目到科目选择组合框
 */
void ShowDZDialog2::initSubjectItems()
{
    QVariant v;
    FirstSubject* fsub;
    //SecondSubject* ssub;
    ui->cmbFsub->clear();
    ui->cmbSsub->clear();
    bool fonded;
    int index, curIndex;

    //如果是第一次打开窗口时，由于可选的科目id列表是空的，所有要填充所有启用的一级科目的id
//    if(curFilter->isDef && curFilter->subIds.isEmpty()){
//        ui->cmbFsub->addItem(tr("所有"));
//        FSubItrator* it = smg->getFstSubItrator();
//        while(it->hasNext()){
//            it->next();
//            fsub = it->value();
//            if(!fsub->isEnabled())
//                continue;
//            v.setValue<FirstSubject*>(fsub);
//            ui->cmbFsub->addItem(fsub->getName(),v);
//        }
//        fonded = true;
//        curIndex = 0;
//    }
//    else{//否者只加载选定的科目

    //如果是一级科目选择模式，则加载所有在科目范围中指定的一级科目到一级科目选择组合框中
    if(curFilter->isFst){
        index = -1; fonded = false;
        if(curFilter->subIds.count() > 1){
            ui->cmbFsub->addItem(tr("所有"));
            if(curFilter->curFSub == 0){
                fonded = true;
                curIndex = 0;
            }
            index++;
        }
        foreach(int sid, curFilter->subIds){
            fsub = smg->getFstSubject(sid);
            v.setValue<FirstSubject*>(fsub);
            ui->cmbFsub->addItem(fsub->getName(),v);
            index++;
            if(!fonded && (fsub->getId() == curFilter->curFSub)){
                curIndex = index;
                fonded = true;
            }
        }
        if(fonded)
            ui->cmbFsub->setCurrentIndex(curIndex);
        //if(fonded)
        //    ui->cmbFsub->setCurrentIndex(curIndex);

        //如果选择了一个具体的一级科目，则装载该一级科目下的所有二级科目，否则则不装载任何二级科目
//        if(curFilter->curFSub != 0){
//            index = -1;fonded = false;
//            fsub = smg->getFstSubject(curFilter->curFSub);
//            if(fsub->getChildCount() > 1){
//                ui->cmbSsub->addItem(tr("所有"));
//                index++;
//                if(curFilter->curSSub == 0){
//                    curIndex = 0;
//                    fonded = true;
//                }
//            }
//            for(int i = 0; i < fsub->getChildCount(); ++i){
//                index++;
//                ssub = fsub->getChildSub(i);
//                v.setValue<SecondSubject*>(ssub);
//                ui->cmbSsub->addItem(ssub->getName(),v);
//                if(!fonded && (curFilter->curSSub == ssub->getId())){
//                    fonded = true;
//                    curIndex = index;
//                }
//            }
//            if(fonded)
//                ui->cmbSsub->setCurrentIndex(curIndex);
//        }
//        else
//            ui->cmbSsub->addItem(tr("所有"));
    }
    //如果是二级科目选择模式，则在一级科目组合框内加载选择的一级科目，二级科目组合框内加载选择的二级科目
    else{
        fsub = smg->getFstSubject(curFilter->curFSub);
        v.setValue<FirstSubject*>(fsub);
        ui->cmbFsub->addItem(fsub->getName(),v);
        onSelFstSub(0);
//        index = -1; fonded = false;
//        if(curFilter->subIds.count() > 1){
//            ui->cmbSsub->addItem(tr("所有"));
//            index++;
//            if(curFilter->curSSub == 0){
//                curIndex = inex;
//                fonded = true;
//            }
//            for(int i = 0; i < subIds.count(); ++i){
//                ssub = smg->getSndSubject(subIds.at(i));

//            }
//        }
    }
//    }

}

/**
 * @brief ShowDZDialog2::initSubjectList
 *  根据当前过滤条件的科目范围设定，在科目列表框中显示选定的科目
 */
void ShowDZDialog2::initSubjectList()
{
    ui->lstSubs->clear();
    QVariant v;
    QListWidgetItem* item;
    FirstSubject* fsub;
    SecondSubject* ssub;

    foreach(int sid, curFilter->subIds){
        if(curFilter->isFst){
            fsub = smg->getFstSubject(sid);
            v.setValue<FirstSubject*>(fsub);
            item = new QListWidgetItem(fsub->getName());
            item->setData(Qt::UserRole,v);
        }
        else{
            ssub = smg->getSndSubject(sid);
            v.setValue<SecondSubject*>(ssub);
            item = new QListWidgetItem(ssub->getName());
            item->setData(Qt::UserRole,v);
        }
        ui->lstSubs->addItem(item);
    }
}

/**
 * @brief ShowDZDialog2::adjustSaveBtn
 *  调整保存按钮的启用状态
 */
void ShowDZDialog2::adjustSaveBtn()
{
    if(curFilter->editState != CIES_NEW){
        curFilter->editState = CIES_CHANGED;
        ui->btnSaveFilter->setEnabled(true);
    }
}




//生成现金日记账表头
void ShowDZDialog2::genThForCash(QStandardItemModel* model)
{
    //总共12个字段，其中3个隐藏了（序号分别为5、10、11）
    QStandardItem* fi;  //第一级表头
    //QStandardItem* si;  //第二级表头
    QList<QStandardItem*> l1/*,l2*/;//l1用于存放表头第一级，l2存放表头第二级

//    fi = new QStandardItem(QString(tr("%1年")).arg(cury));
//    l1<<fi;
//    si = new QStandardItem(tr("月"));
//    l2<<si;
//    fi->appendColumn(l2);
//    l2.clear();
//    si = new QStandardItem(tr("日"));
//    l2<<si;
//    fi->appendColumn(l2);
//    l2.clear();

    fi = new QStandardItem(tr("年"));                 //0
    l1<<fi;
    fi = new QStandardItem(tr("月"));                 //1
    l1<<fi;
    fi = new QStandardItem(tr("日"));                 //2
    l1<<fi;
    fi = new QStandardItem(tr("凭证号"));              //3
    l1<<fi;
    fi = new QStandardItem(tr("摘要"));                //4
    l1<<fi;
    fi = new QStandardItem(tr("对方科目"));             //5
    l1<<fi;
    fi = new QStandardItem(tr("借方金额"));             //6借方
    l1<<fi;
    fi = new QStandardItem(tr("贷方金额"));             //7贷方
    l1<<fi;
    fi = new QStandardItem(tr("方向"));                //8余额方向
    l1<<fi;
    fi = new QStandardItem(tr("余额"));                //9余额
    l1<<fi;
    fi = new QStandardItem(tr("PID"));                //10
    l1<<fi;
    fi = new QStandardItem(tr("SID"));                //11
    l1<<fi;

    if(model == NULL)
        headerModel->appendRow(l1);
    else
        model->appendRow(l1);
    l1.clear();
}

//生成银行人民币日记账表头
void ShowDZDialog2::genThForBankRmb(QStandardItemModel* model)
{
    //总共12个字段，其中4个隐藏了（序号分别为4、5、10、11）
    QStandardItem* fi;  //第一级表头
    QStandardItem* si;  //第二级表头
    QList<QStandardItem*> l1,l2;//l1用于存放表头第一级，l2存放表头第二级

    fi = new QStandardItem(QString(tr("%1年")).arg(cury));
    l1<<fi;
    si = new QStandardItem(tr("月")); //0
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();
    si = new QStandardItem(tr("日")); //1
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    fi = new QStandardItem(tr("凭证号"));//2
    l1<<fi;
    fi = new QStandardItem(tr("摘要"));//3
    l1<<fi;
    fi = new QStandardItem(tr("结算号"));//4
    l1<<fi;
    fi = new QStandardItem(tr("对方科目"));//5
    l1<<fi;
    //借方
    fi = new QStandardItem(tr("借方金额"));//6
    l1<<fi;
    //贷方
    fi = new QStandardItem(tr("贷方金额"));//7
    l1<<fi;
    //余额方向
    fi = new QStandardItem(tr("方向"));//8
    l1<<fi;
    //余额
    fi = new QStandardItem(tr("余额"));//9
    l1<<fi;

    fi = new QStandardItem(tr("PID"));//10
    l1<<fi;
    fi = new QStandardItem(tr("SID"));//11
    l1<<fi;

    if(model ==NULL)
        headerModel->appendRow(l1);
    else
        model->appendRow(l1);
    l1.clear();
}

//生成银行外币日记账表头
void ShowDZDialog2::genThForBankWb(QStandardItemModel* model)
{
    //总共16个字段，其中4个隐藏了（序号分别为4、5、14、15）在只有一种外币的情况下
    QStandardItem* fi;  //第一级表头
    QStandardItem* si;  //第二级表头
    QList<QStandardItem*> l1,l2;//l1用于存放表头第一级，l2存放表头第二级

    fi = new QStandardItem(QString(tr("%1年")).arg(cury));
    l1<<fi;
    si = new QStandardItem(tr("月")); //index:0
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();
    si = new QStandardItem(tr("日")); //index:1
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    fi = new QStandardItem(tr("凭证号")); //index:2
    l1<<fi;
    fi = new QStandardItem(tr("摘要"));   //index:3
    l1<<fi;
    fi = new QStandardItem(tr("结算号")); //index:4
    l1<<fi;
    fi = new QStandardItem(tr("对方科目"));//index:5
    l1<<fi;
    fi = new QStandardItem(tr("汇率"));   //index:6
    for(int i = 0; i < mts.count(); ++i){
        si = new ApStandardItem(allMts.value(mts[i]));
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    l1<<fi;

    //借方
    fi = new QStandardItem(tr("借方"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i])->name());
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    //贷方
    fi = new QStandardItem(tr("贷方"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i])->name());
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    //余额方向
    fi = new QStandardItem(tr("方向"));
    l1<<fi;

    //余额
    fi = new QStandardItem(tr("余额"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i])->name());
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    fi = new QStandardItem(tr("PID"));
    l1<<fi;
    fi = new QStandardItem(tr("SID"));
    l1<<fi;

    if(model == NULL)
        headerModel->appendRow(l1);
    else
        model->appendRow(l1);
    l1.clear();
}

//生成通用明细账表头（金额式-即不需要外币金额栏）
void ShowDZDialog2::genThForDetails(QStandardItemModel* model)
{
    //总共10个字段，其中2个隐藏了（序号分别为8、9）
    QStandardItem* fi;  //第一级表头
    QStandardItem* si;  //第二级表头
    QList<QStandardItem*> l1,l2;//l1用于存放表头第一级，l2存放表头第二级

    fi = new QStandardItem(QString(tr("%1年")).arg(cury));
    l1<<fi;
    si = new QStandardItem(tr("月"));  //index: 0
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();
    si = new QStandardItem(tr("日"));  //index: 1
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    fi = new QStandardItem(tr("凭证号"));//index: 2
    l1<<fi;
    fi = new QStandardItem(tr("摘要"));  //index: 3
    l1<<fi;

    //借方
    fi = new QStandardItem(tr("借方"));//index: 4
    l1<<fi;
    //贷方
    fi = new QStandardItem(tr("贷方"));//index: 5
    l1<<fi;
    //余额方向
    fi = new QStandardItem(tr("方向"));//index: 6
    l1<<fi;
    //viewCols++;

    //余额
    fi = new QStandardItem(tr("余额"));//index: 7
    l1<<fi;
    fi = new QStandardItem(tr("PID"));//index: 8
    l1<<fi;
    fi = new QStandardItem(tr("SID"));//index: 9
    l1<<fi;

    if(model == NULL)
        headerModel->appendRow(l1);
    else
        model->appendRow(l1);
    l1.clear();
}

//生成明细账表头（三栏明细式）
void ShowDZDialog2::genThForWai(QStandardItemModel* model)
{
    //总共14个字段，其中2个隐藏了（序号分别为12、13）在只有一种外币的情况下
    QStandardItem* fi;  //第一级表头
    QStandardItem* si;  //第二级表头
    QList<QStandardItem*> l1,l2;//l1用于存放表头第一级，l2存放表头第二级

    fi = new QStandardItem(QString(tr("%1年")).arg(cury));
    l1<<fi;
    si = new QStandardItem(tr("月")); //index:0
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();
    si = new QStandardItem(tr("日")); //index:1
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    fi = new QStandardItem(tr("凭证号")); //index:2
    l1<<fi;
    fi = new QStandardItem(tr("摘要"));   //index:3
    l1<<fi;
    fi = new QStandardItem(tr("汇率"));   //index:4
    for(int i = 0; i < mts.count(); ++i){
        si = new ApStandardItem(allMts.value(mts[i]));
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    l1<<fi;

    //借方
    fi = new QStandardItem(tr("借方"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i])->name()); //5
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));                //6
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    //贷方
    fi = new QStandardItem(tr("贷方"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i])->name()); //7
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));                //8
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    //余额方向
    fi = new QStandardItem(tr("方向"));                //9
    l1<<fi;

    //余额
    fi = new QStandardItem(tr("余额"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i])->name()); //10
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));                //11
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    fi = new QStandardItem(tr("PID"));//index:12
    l1<<fi;
    fi = new QStandardItem(tr("SID"));//index:13
    l1<<fi;

    if(model == NULL)
        headerModel->appendRow(l1);
    else
        model->appendRow(l1);
    l1.clear();
}

//获取前期余额及其指定月份区间的每笔发生项数据
//void ShowDZDialog2::getDatas(int y, int sm, int em, int fid, int sid, int mt,
//              QList<DailyAccountData*>& datas,
//              QHash<int,double> preExtra,
//              QHash<int,double> preExtraDir,
//              QHash<int, double> rates)
//{
//    if(!BusiUtil::getDailyAccount(y,sm,em,fid,sid,mt,datas,preExtra,preExtraDir,rates))
//        return;
//}

//生成现金日记账数据（返回值为总共生成的行数）
int ShowDZDialog2::genDataForCash(QList<DailyAccountData2 *> datas,
                                 QList<QList<QStandardItem*> >& pdatas,
                                 Double prev, int preDir,
                                 QHash<int, Double> preExtra,
                                 QHash<int,int> preExtraDir,
                                 QHash<int, Double> rates)
{
//    QStandardItemModel* mo;
//    if(model == NULL)
//        mo = dataModel;
//    else
//        mo = model;
//    QList<DailyAccountData*> datas;       //数据
//    QHash<int,double> preExtra;           //期初余额
//    QHash<int,int> preExtraDir;           //期初余额方向
//    QHash<int, double> rates; //每月汇率

//    if(!BusiUtil::getDailyAccount(cury,sm,em,fid,sid,mt,datas,preExtra,preExtraDir,rates))
//        return;

    ApStandardItem* item;
    QList<QStandardItem*> l;
    int rows = 0;
    if(datas.empty())
        return 0;
    //期初余额部分
    l<<NULL<<NULL<<NULL<<NULL;
    if(ui->startDate->date().month() == 1)
        item = new ApStandardItem(tr("上年结转"));
    else
        item = new ApStandardItem(tr("上月结转"));
    l<<item;
    l<<NULL<<NULL<<NULL;
    item = new ApStandardItem(dirStr(preDir));
    l<<item;
    item = new ApStandardItem(prev.getv());
    l<<item;
    l<<NULL<<NULL;
    pdatas<<l;
    rows++;
    l.clear();

    if(datas.empty())  //如果在指定月份期间未发生任何业务活动，则返回
        return 0;

    //发生额部分
    Double jmsums = 0.00,jysums = 0.00, dmsums = 0.00, dysums = 0.00;  //当期借、贷方合计值
    int oy = datas.first()->y;  //用以判定是否跨年（如果跨年，则需要插入本年合计行）
    int om = datas.first()->m;  //用以判定是否跨月（如果跨月，则需要插入本月合计行）
    for(int i = 0; i < datas.count(); ++i){
        //插入本月合计行和累计行
        if(om != datas[i]->m){
            item = new ApStandardItem(oy); //0：年
            l<<item;
            item = new ApStandardItem(om); //1：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("本月合计"));
            l<<item<<NULL;
            item = new ApStandardItem(jmsums.getv());  //6：借方
            l<<item;
            item = new ApStandardItem(dmsums.getv());  //7：贷方
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));        //8：余额方向
            l<<item;
            item = new ApStandardItem(datas[i-1]->etm.getv());    //9：余额
            l<<item<<NULL<<NULL;
            //mo->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();

            jmsums = 0.00; dmsums = 0.00;
            item = new ApStandardItem(QString::number(om)); //0：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("累计"));
            l<<item<<NULL;
            item = new ApStandardItem(jysums.getv());  //5：借方
            l<<item;
            item = new ApStandardItem(dysums.getv());  //6：贷方
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));        //7：余额方向
            l<<item;
            item = new ApStandardItem(datas[i-1]->etm);    //8：余额
            l<<item<<NULL<<NULL;
            //mo->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();
        }

        oy = datas.at(i)->y;
        om = datas.at(i)->m;
        item = new ApStandardItem(datas[i]->y); //0：年
        l<<item;
        item = new ApStandardItem(datas[i]->m); //1：月
        l<<item;
        item = new ApStandardItem(datas[i]->d); //2：日
        l<<item;
        item = new ApStandardItem(datas[i]->pzNum);              //3：凭证号
        l<<item;
        item = new ApStandardItem(datas[i]->summary,Qt::AlignLeft|Qt::AlignVCenter);            //4：摘要
        l<<item;
        item = new ApStandardItem;                               //5：对方科目
        l<<item;
        if(datas[i]->dh == DIR_J){
            jmsums += datas[i]->v;
            jysums += datas[i]->v;
            item = new ApStandardItem(datas[i]->v.getv());  //6：借方
            l<<item;
            item = new ApStandardItem;               //7：贷方
            l<<item;
        }
        else{
            dmsums += datas[i]->v;
            dysums += datas[i]->v;
            item = new ApStandardItem;
            l<<item;
            item = new ApStandardItem(datas[i]->v.getv());
            l<<item;
        }
        item = new ApStandardItem(dirStr(datas[i]->dir)); //8：余额方向
        l<<item;
        item = new ApStandardItem(datas[i]->etm.getv());    //9：余额
        l<<item;
        //添加两个隐藏列（业务活动所属凭证id和业务活动本身的id）
        item = new ApStandardItem(datas[i]->pid);//10
        l<<item;
        item = new ApStandardItem(datas[i]->bid);//11
        l<<item;
        //mo->appendRow(l);
        pdatas<<l;
        rows++;
        l.clear();
    }

    //插入末尾的本月累计和本年累计行
    item = new ApStandardItem(ui->endDate->date().year());  //0：年
    l<<item;
    item = new ApStandardItem(ui->endDate->date().month()); //1：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本月合计"));  //4：摘要
    l<<item<<NULL;
    item = new ApStandardItem(jmsums.getv());  //6：借方
    l<<item;
    item = new ApStandardItem(dmsums.getv());  //7：贷方
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));
        l<<item;
        item = new ApStandardItem(preExtra.value(RMB).getv());
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir)); //8：余额方向
        l<<item;
        item = new ApStandardItem(datas[datas.count()-1]->etm.getv());    //9：余额
        l<<item;
    }
    l<<NULL<<NULL;
    //mo->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    item = new ApStandardItem(ui->endDate->date().year()); //0：年
    l<<item;
    item = new ApStandardItem(ui->endDate->date().month()); //1：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本年累计"));      //4
    l<<item<<NULL;
    item = new ApStandardItem(jysums);  //6：借方
    l<<item;
    item = new ApStandardItem(dysums);  //7：贷方
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));
        l<<item;
        item = new ApStandardItem(preExtra.value(RMB));
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir)); //8：余额方向
        l<<item;
        item = new ApStandardItem(datas[datas.count()-1]->etm);    //9：余额
        l<<item;
    }
    l<<NULL<<NULL;
    //mo->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();
    return rows;
}

//生成银行日记账数据
int ShowDZDialog2::genDataForBankRMB(QList<DailyAccountData2*> datas,
                                     QList<QList<QStandardItem*> >& pdatas,
                                     Double prev, int preDir,
                                     QHash<int,Double> preExtra,
                                     QHash<int,int> preExtraDir,
                                     QHash<int,Double> rates)
{
//    QList<DailyAccountData*> datas;       //数据
//    QHash<int,double> preExtra;           //期初余额
//    QHash<int,int> preExtraDir;           //期初余额方向
//    QHash<int, double> rates; //每月汇率

//    if(!BusiUtil::getDailyAccount(cury,sm,em,fid,sid,mt,datas,preExtra,preExtraDir,rates))
//        return;
    ApStandardItem* item;
    QList<QStandardItem*> l;
    int rows = 0;

    //期初余额部分
    l<<NULL<<NULL<<NULL;
    if(ui->startDate->date().month()== 1)
        item = new ApStandardItem(tr("上年结转"));
    else
        item = new ApStandardItem(tr("上月结转"));
    l<<item;
    l<<NULL<<NULL<<NULL<<NULL;
    //item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));  //期初余额方向
    item = new ApStandardItem(dirStr(preDir));  //期初余额方向
    l<<item;
    //item = new ApStandardItem(preExtra.value(RMB));//期初余额
    item = new ApStandardItem(prev);//期初余额
    l<<item;
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    if(datas.empty())
        return rows;

    //发生额部分
    Double jmsums = 0.00,jysums = 0.00, dmsums = 0.00, dysums = 0.00;  //当期借、贷方合计值
    int om = datas[0]->m;  //用以判定是否需要插入本月合计行
    for(int i = 0; i < datas.count(); ++i){
        //插入本月合计行和累计行
        if(om != datas[i]->m){
            item = new ApStandardItem(om); //0：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("本月合计"));
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(jmsums);  //5：借方
            l<<item;
            item = new ApStandardItem(dmsums);  //6：贷方
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));//7：余额方向
            l<<item;
            item = new ApStandardItem(datas[i-1]->etm); //8：余额
            l<<item<<NULL<<NULL;
            //dataModel->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();
            jmsums = 0; dmsums = 0;

            item = new ApStandardItem(om); //0：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("累计"));
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(jysums);  //5：借方
            l<<item;
            item = new ApStandardItem(dysums);  //6：贷方
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir)); //7：余额方向
            l<<item;
            item = new ApStandardItem(datas[i-1]->etm); //8：余额
            l<<item<<NULL<<NULL;
            //dataModel->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();
        }

        om = datas[i]->m;
        item = new ApStandardItem(datas[i]->m); //0：月
        l<<item;
        item = new ApStandardItem(datas[i]->d); //1：日
        l<<item;
        item = new ApStandardItem(datas[i]->pzNum);              //2：凭证号
        l<<item;
        item = new ApStandardItem(datas[i]->summary,Qt::AlignLeft | Qt::AlignVCenter);            //3：摘要
        l<<item;
        item = new ApStandardItem;                               //4：结算号
        l<<item;
        item = new ApStandardItem;                               //5：对方科目
        l<<item;

        if(datas[i]->dh == DIR_J){
            jmsums += datas[i]->v;
            jysums += datas[i]->v;
            item = new ApStandardItem(datas[i]->v);  //6：借方
            l<<item;
            item = new ApStandardItem;               //7：贷方
            l<<item;
        }
        else{
            dmsums += datas[i]->v;
            dysums += datas[i]->v;
            item = new ApStandardItem;
            l<<item;
            item = new ApStandardItem(datas[i]->v);
            l<<item;
        }
        item = new ApStandardItem(dirStr(datas[i]->dir));  //8：余额方向
        l<<item;
        item = new ApStandardItem(datas[i]->etm);    //9：余额
        l<<item;
        //添加两个隐藏列（业务活动所属凭证id和业务活动本身的id）
        item = new ApStandardItem(datas[i]->pid);//10
        l<<item;
        item = new ApStandardItem(datas[i]->bid);//11
        l<<item;
        //dataModel->appendRow(l);
        pdatas<<l;
        rows++;
        l.clear();
    }
    //插入末尾的本月累计和本年累计行
    item = new ApStandardItem(ui->endDate->date().month()); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本月合计"));
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(jmsums);  //6：借方
    l<<item;
    item = new ApStandardItem(dmsums);  //7：贷方
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));
        l<<item;
        item = new ApStandardItem(preExtra.value(RMB));
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir));        //8：余额方向
        l<<item;
        item = new ApStandardItem(datas[datas.count()-1]->etm); //9：余额
        l<<item;
    }
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    item = new ApStandardItem(ui->endDate->date().month()); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本年累计"));
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(jysums);  //6：借方
    l<<item;
    item = new ApStandardItem(dysums);  //7：贷方
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));
        l<<item;
        item = new ApStandardItem(preExtra.value(RMB));
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir));        //8：余额方向
        l<<item;
        item = new ApStandardItem(datas[datas.count()-1]->etm); //9：余额
        l<<item;
    }
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    return rows;
}

//生成三栏金额式数据，参数mt是要显示的货币代码
int ShowDZDialog2::genDataForBankWb(QList<DailyAccountData2 *> datas,
                                    QList<QList<QStandardItem*> >& pdatas,
                                    Double prev, int preDir,
                                    QHash<int, Double> preExtra,
                                    QHash<int,int> preExtraDir,
                                    QHash<int, Double> rates)
{
    ApStandardItem* item;
    QList<QStandardItem*> l;
    int rows = 0;

    //期初余额部分
    l<<NULL<<NULL<<NULL;  //0、1、2 跳过月、日、凭证号栏
    if(ui->startDate->date().month() == 1)
        item = new ApStandardItem(tr("上年结转"));  //3 摘要栏
    else
        item = new ApStandardItem(tr("上月结转"));
    l<<item;
    l<<NULL<<NULL;   //4、5跳过结算号、对方科目
    for(int i = 0; i < mts.count(); ++i){      //6、汇率栏
        item = new ApStandardItem(rates.value(mts[i]));
        l<<item;
    }
    for(int i = 0; i < mts.count()*2 + 2; ++i) //7、8、9、10借贷栏
        l<<NULL;
    int dir;      //各币种合计的余额方向
    Double esum;  //各币种合计的余额，初值为期初金额
    esum = prev;
    item = new ApStandardItem(dirStr(preDir));  //11 期初余额方向
    l<<item;

    for(int i = 0; i < mts.count(); ++i){
        item = new ApStandardItem(preExtra.value(mts[i]));//12 期初余额（外币）
        l<<item;
    }
    item = new ApStandardItem(esum);//13 期初余额（金额）
    l<<item;
    l<<NULL<<NULL;
    pdatas<<l;
    rows++;
    l.clear();

    if(datas.empty())
        return rows;

    //发生额部分
    Double jmsums = 0.00,jysums = 0.00;   //本月、本年借方合计值（总额）
    Double dmsums = 0.00, dysums = 0.00;  //本月、本年贷方合计值（总额）
    QHash<int,Double> jwmsums,jwysums;    //本月、本年借方合计值（外币部分）
    QHash<int,Double> dwmsums,dwysums;    //本月、本年贷方合计值（外币部分）
    int om = datas[0]->m;  //清单的起始月份，用以判定是否需要插入本月合计行
    for(int i = 0; i < datas.count(); ++i){
        //插入本月合计行和累计行
        if(om != datas[i]->m){
            //本月合计行
            item = new ApStandardItem(om); //0：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("本月合计"));       //3：摘要
            l<<item<<NULL<<NULL<<NULL;
            for(int j = 0; j < mts.count(); ++j){
                if(curMt->code() == mts[j]){
                    item = new ApStandardItem(jwmsums.value(mts[j]));  //7：借方（外币）
                    l<<item;
                    jwmsums[mts[j]] = 0;
                }
                else
                    l<<NULL;
            }
            item = new ApStandardItem(jmsums);  //8：借方（金额）
            l<<item;
            for(int j = 0; j < mts.count(); ++j){
                if(curMt->code() == mts[j]){
                    item = new ApStandardItem(dwmsums.value(mts[j]));  //9：贷方（外币）
                    l<<item;
                    dwmsums[mts[j]] = 0;
                }
                else
                    l<<NULL;
            }
            item = new ApStandardItem(dmsums);  //10：贷方（金额）
            l<<item;
            if(!curMt)
                item = new ApStandardItem(dirStr(datas[i-1]->dir)); //11：余额方向
            else
                item = new ApStandardItem(dirStr(datas[i-1]->dirs.value(curMt->code())));
            l<<item;
            for(int j = 0; j < mts.count(); ++j){
                if(curMt->code() == datas[i]->mt){
                    item = new ApStandardItem(datas[i-1]->em.value(mts[j])); //12：余额（外币）
                    l<<item;
                }
                else
                    l<<NULL;
            }
            item = new ApStandardItem(datas[i-1]->etm); //13：余额（金额）
            l<<item<<NULL<<NULL;
            //dataModel->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();
            jmsums = 0.00; dmsums = 0.00;

            //累计行
            item = new ApStandardItem(om); //0：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("累计"));
            l<<item<<NULL<<NULL<<NULL;
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(jwysums.value(mts[j]));  //7：借方（外币）
                l<<item;
            }
            item = new ApStandardItem(jysums);  //8：借方（金额）
            l<<item;
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(dwysums.value(mts[j]));  //9：贷方（外币）
                l<<item;
            }
            item = new ApStandardItem(dysums);  //10：贷方（金额）
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));        //11：余额方向
            l<<item;
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(datas[i-1]->em.value(mts[j])); //12：余额（外币）
                l<<item;
            }
            item = new ApStandardItem(datas[i-1]->etm); //13：余额（金额）
            l<<item<<NULL<<NULL;
            pdatas<<l;
            rows++;
            l.clear();
        }

        //发生行
        om = datas[i]->m;
        item = new ApStandardItem(datas[i]->m); //0：月
        l<<item;
        item = new ApStandardItem(datas[i]->d); //1：日
        l<<item;
        item = new ApStandardItem(datas[i]->pzNum);              //2：凭证号
        l<<item;
        item = new ApStandardItem(datas[i]->summary,Qt::AlignLeft | Qt::AlignVCenter);            //3：摘要
        l<<item;
        item = new ApStandardItem;                               //4：结算号
        l<<item;
        item = new ApStandardItem;                               //5：对方科目
        l<<item;

        //因为对于每一次的发生项，只能对应一个币种，因此可以直接提取汇率在下面使用
        Double rate = rates.value(datas[i]->m*10+datas[i]->mt);
        //汇率部分
        for(int j = 0; j < mts.count(); ++j){
            if(datas[i]->mt == mts[j])
                item = new ApStandardItem(rate);  //6：汇率
            else
                item = NULL;
            l<<item;
        }
        if(datas[i]->dh == DIR_J){
            for(int j = 0; j < mts.count(); ++j){
                if(mts[j] == datas[i]->mt){
                    item = new ApStandardItem(datas[i]->v);//7：借方（外币）
                    l<<item;
                }
                else
                    l<<NULL;
            }
            item = new ApStandardItem(datas[i]->v * rate);  //8：借方（金额）
            l<<item;
            jmsums += datas[i]->v * rate;
            jysums += datas[i]->v * rate;
            jwmsums[datas[i]->mt] += datas[i]->v;
            jwysums[datas[i]->mt] += datas[i]->v;
            for(int j = 0; j < mts.count()+1; ++j) //9、10：贷方（为空）
                l<<NULL;
        }
        else{
            for(int j = 0; j < mts.count()+1; ++j) //7、8：借方（为空）
                l<<NULL;
            for(int j = 0; j < mts.count(); ++j){
                if(mts[j] == datas[i]->mt){
                    item = new ApStandardItem(datas[i]->v);//9：贷方（外币）
                    l<<item;
                }
                else
                    l<<NULL;
            }
            item = new ApStandardItem(datas[i]->v * rate);//10：贷方（金额）
            l<<item;
            dmsums += datas[i]->v * rate;
            dysums += datas[i]->v * rate;
            dwmsums[datas[i]->mt] += datas[i]->v;
            dwysums[datas[i]->mt] += datas[i]->v;
        }
        item = new ApStandardItem(dirStr(datas[i]->dir));        //11：余额方向
        l<<item;
        for(int j = 0; j < mts.count(); ++j){
            item = new ApStandardItem(datas[i]->em.value(mts[j])); //12：余额（外币）
            l<<item;
        }
        item = new ApStandardItem(datas[i]->etm);    //13：余额（金额）
        l<<item;
        //添加两个隐藏列
        item = new ApStandardItem(datas[i]->pid);//14：业务活动所属凭证id
        l<<item;
        item = new ApStandardItem(datas[i]->bid);//15：业务活动本身的id
        l<<item;
        //dataModel->appendRow(l);
        pdatas<<l;
        rows++;
        l.clear();
    }
    //插入末尾的本月累计和本年累计行
    item = new ApStandardItem(ui->endDate->date().month()); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本月合计"));
    l<<item<<NULL<<NULL<<NULL;
    for(int j = 0; j < mts.count(); ++j){
        item = new ApStandardItem(jwmsums.value(mts[j]));  //7：借方（外币）
        l<<item;
    }
    item = new ApStandardItem(jmsums);  //8：借方（金额）
    l<<item;
    for(int j = 0; j < mts.count(); ++j){
        item = new ApStandardItem(dwmsums.value(mts[j]));  //9：贷方（外币）
        l<<item;
    }
    item = new ApStandardItem(dmsums);  //10：贷方（金额）
    l<<item;
    if(datas.empty()){ //如果选定的月份范围未发生任何业务活动，则利用期初数值替代
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));  //期初余额方向
        l<<item;
        for(int i = 0; i < mts.count(); ++i){
            item = new ApStandardItem(preExtra.value(mts[i]));//期初余额
            l<<item;
        }
        item = new ApStandardItem(preExtra.value(RMB));//期初余额
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir));        //11：余额方向
        l<<item;
        for(int j = 0; j < mts.count(); ++j){
            item = new ApStandardItem(datas[datas.count()-1]->em.value(mts[j])); //12：余额（外币）
            l<<item;
        }
        item = new ApStandardItem(datas[datas.count()-1]->etm); //13：余额（金额）
        l<<item;
    }
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    item = new ApStandardItem(ui->endDate->date().month()); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本年累计")); //3：摘要
    l<<item<<NULL<<NULL<<NULL;
    for(int j = 0; j < mts.count(); ++j){
        item = new ApStandardItem(jwysums.value(mts[j]));  //7：借方（外币）
        l<<item;
    }
    item = new ApStandardItem(jysums);  //8：借方（金额）
    l<<item;
    for(int j = 0; j < mts.count(); ++j){
        item = new ApStandardItem(dwysums.value(mts[j]));  //9：贷方（外币）
        l<<item;
    }
    item = new ApStandardItem(dysums);  //10：贷方（金额）
    l<<item;
    if(datas.empty()){ //如果选定的月份范围未发生任何业务活动，则利用期初数值替代
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));  //期初余额方向
        l<<item;
        for(int i = 0; i < mts.count(); ++i){
            item = new ApStandardItem(preExtra.value(mts[i]));//期初余额
            l<<item;
        }
        item = new ApStandardItem(preExtra.value(RMB));//期初余额
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir));        //11：余额方向
        l<<item;
        for(int j = 0; j < mts.count(); ++j){
            item = new ApStandardItem(datas[datas.count()-1]->em.value(mts[j])); //12：余额（外币）
            l<<item;
        }
        item = new ApStandardItem(datas[datas.count()-1]->etm); //13：余额（金额）
        l<<item;
    }
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    return rows;
}

//生成其他科目明细账数据（不区分外币）
int ShowDZDialog2::genDataForDetails(QList<DailyAccountData2*> datas,
                                     QList<QList<QStandardItem*> >& pdatas,
                                     Double prev, int preDir,
                                     QHash<int,Double> preExtra,
                                     QHash<int,int> preExtraDir,
                                     QHash<int,Double> rates)
{
//    QList<DailyAccountData*> datas;       //数据
//    QHash<int,double> preExtra;           //期初余额
//    QHash<int,int> preExtraDir;           //期初余额方向
//    QHash<int, double> rates; //每月汇率

//    if(!BusiUtil::getDailyAccount(cury,sm,em,fid,sid,mt,datas,preExtra,preExtraDir,rates))
//        return;
    ApStandardItem* item;
    QList<QStandardItem*> l;
    int rows = 0;

    //期初余额部分
    l<<NULL<<NULL<<NULL;
    if(ui->startDate->date().month() == 1)
        item = new ApStandardItem(tr("上年结转"));
    else
        item = new ApStandardItem(tr("上月结转"));
    l<<item;
    l<<NULL<<NULL;
    item = new ApStandardItem(dirStr(preDir));
    l<<item;
    item = new ApStandardItem(prev);
    l<<item;
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    if(datas.empty())
        return rows;

    //发生额部分
    Double jmsums = 0.00,jysums = 0.00, dmsums = 0.00, dysums = 0.00;  //当期借、贷方合计值
    int om = datas[0]->m;  //用以判定是否需要插入本月合计行
    for(int i = 0; i < datas.count(); ++i){
        //插入本月合计行和累计行
        if(om != datas[i]->m){
            item = new ApStandardItem(om); //0：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("本月合计")); //3
            l<<item;
            item = new ApStandardItem(jmsums);  //4：借方
            l<<item;
            item = new ApStandardItem(dmsums);  //5：贷方
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));        //6：余额方向
            l<<item;
            item = new ApStandardItem(datas[i-1]->etm);    //7：余额
            l<<item<<NULL<<NULL;
            //dataModel->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();

            jmsums = 0; dmsums = 0;
            item = new ApStandardItem(QString::number(om)); //0：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("累计"));//3
            l<<item;
            item = new ApStandardItem(jysums);  //4：借方
            l<<item;
            item = new ApStandardItem(dysums);  //5：贷方
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));        //6：余额方向
            l<<item;
            item = new ApStandardItem(datas[i-1]->etm);    //7：余额
            l<<item<<NULL<<NULL;
            //dataModel->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();
        }

        om = datas[i]->m;
        item = new ApStandardItem(datas[i]->m); //0：月
        l<<item;
        item = new ApStandardItem(datas[i]->d); //1：日
        l<<item;
        item = new ApStandardItem(datas[i]->pzNum);              //2：凭证号
        l<<item;
        item = new ApStandardItem(datas[i]->summary,Qt::AlignLeft|Qt::AlignVCenter); //3：摘要
        l<<item;
        if(datas[i]->dh == DIR_J){
            jmsums += datas[i]->v;
            jysums += datas[i]->v;
            item = new ApStandardItem(datas[i]->v);  //4：借方
            l<<item;
            item = new ApStandardItem;               //5：贷方
            l<<item;
        }
        else{
            dmsums += datas[i]->v;
            dysums += datas[i]->v;
            item = new ApStandardItem;
            l<<item;
            item = new ApStandardItem(datas[i]->v);
            l<<item;
        }
        item = new ApStandardItem(dirStr(datas[i]->dir)); //6：余额方向
        l<<item;
        item = new ApStandardItem(datas[i]->etm);    //7：余额
        l<<item;
        //添加两个隐藏列（业务活动所属凭证id和业务活动本身的id）
        item = new ApStandardItem(datas[i]->pid);//8
        l<<item;
        item = new ApStandardItem(datas[i]->bid);//9
        l<<item;
        //dataModel->appendRow(l);
        pdatas<<l;
        rows++;
        l.clear();
    }

    //插入末尾的本月累计和本年累计行
    item = new ApStandardItem(QString::number(ui->endDate->date().month())); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本月合计"));//3
    l<<item;
    item = new ApStandardItem(jmsums);  //4：借方
    l<<item;
    item = new ApStandardItem(dmsums);  //5：贷方
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));
        l<<item;
        item = new ApStandardItem(preExtra.value(RMB));
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir)); //6：余额方向
        l<<item;
        item = new ApStandardItem(datas[datas.count()-1]->etm);    //7：余额
        l<<item;
    }
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    item = new ApStandardItem(ui->endDate->date().month()); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本年累计"));//3
    l<<item;
    item = new ApStandardItem(jysums);  //4：借方
    l<<item;
    item = new ApStandardItem(dysums);  //5：贷方
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));
        l<<item;
        item = new ApStandardItem(preExtra.value(RMB));
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir)); //6：余额方向
        l<<item;
        item = new ApStandardItem(datas[datas.count()-1]->etm);    //7：余额
        l<<item;
    }
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    return rows;
}

//生成三栏明细式表格数据（仅对应收/应付，或在显示所有币种的明细账时）
int ShowDZDialog2::genDataForDetWai(QList<DailyAccountData2 *> datas,
                                    QList<QList<QStandardItem*> >& pdatas,
                                    Double prev, int preDir,
                                    QHash<int, Double> preExtra,
                                    QHash<int,int> preExtraDir,
                                    QHash<int, Double> rates)
{
    ApStandardItem* item;
    QList<QStandardItem*> l;
    int rows = 0;

    //期初余额部分
    l<<NULL<<NULL<<NULL;  //0、1、2 跳过月、日、凭证号栏
    if(ui->startDate->date().month() == 1)
        item = new ApStandardItem(tr("上年结转"));  //3 摘要栏
    else
        item = new ApStandardItem(tr("上月结转"));
    l<<item;  //3
    for(int i = 0; i < mts.count(); ++i){      //4、汇率栏
        item = new ApStandardItem(rates.value(mts[i]));
        l<<item;
    }
    for(int i = 0; i < mts.count()*2 + 2; ++i) //5、6、7、8借贷栏
        l<<NULL;
    item = new ApStandardItem(dirStr(preDir));  //9：期初余额方向
    l<<item;
    for(int i = 0; i < mts.count(); ++i){
        item = new ApStandardItem(preExtra.value(mts[i]));//10：期初余额（外币）
        l<<item;
    }
    item = new ApStandardItem(prev);//11：期初余额
    l<<item;
    l<<NULL<<NULL; //12、13
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    if(datas.empty())
        return rows;

    //发生额部分
    Double jmsums = 0.00,jysums = 0.00, dmsums = 0.00, dysums = 0.00;  //当期借、贷方合计值
    QHash<int,Double> jwmsums,jwysums,dwmsums,dwysums;     //当期借、贷方合计值（外币部分）
    int om = datas[0]->m;  //用以判定是否需要插入本月合计行
    for(int i = 0; i < datas.count(); ++i){
        //插入本月合计行和累计行
        if(om != datas[i]->m){
            //本月合计行
            item = new ApStandardItem(om); //0：月
            l<<item<<NULL<<NULL;           //1、2：日、凭证号
            item = new ApStandardItem(tr("本月合计"));       //3：摘要
            l<<item<<NULL;                                 //4：汇率
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(jwmsums.value(mts[j]));  //5：借方（外币）
                l<<item;
                jwmsums[mts[j]] = 0;
            }
            item = new ApStandardItem(jmsums);  //6：借方（金额）
            l<<item;
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(dwmsums.value(mts[j]));  //7：贷方（外币）
                l<<item;
                dwmsums[mts[j]] = 0;
            }
            item = new ApStandardItem(dmsums);  //8：贷方（金额）
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));        //9：余额方向
            l<<item;
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(datas[i-1]->em.value(mts[j])); //10：余额（外币）
                l<<item;
            }
            item = new ApStandardItem(datas[i-1]->etm); //11：余额（金额）
            l<<item<<NULL<<NULL;
            //dataModel->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();
            jmsums = 0; dmsums = 0;

            //累计行
            item = new ApStandardItem(om); //0：月
            l<<item<<NULL<<NULL;                   //1、2：日、凭证号列
            item = new ApStandardItem(tr("累计"));  //3：摘要列
            l<<item<<NULL;                         //4：汇率列
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(jwysums.value(mts[j]));  //5：借方（外币）
                l<<item;
            }
            item = new ApStandardItem(jysums);  //6：借方（金额）
            l<<item;
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(dwysums.value(mts[j]));  //7：贷方（外币）
                l<<item;
            }
            item = new ApStandardItem(dysums);  //8：贷方（金额）
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));        //9：余额方向
            l<<item;
            for(int j = 0; j < mts.count(); ++j){
                item = new ApStandardItem(datas[i-1]->em.value(mts[j])); //10：余额（外币）
                l<<item;
            }
            item = new ApStandardItem(datas[i-1]->etm); //11：余额（金额）
            l<<item<<NULL<<NULL;
            //dataModel->appendRow(l);
            pdatas<<l;
            rows++;
            l.clear();
        }

        //发生行

        //如果当前发生项是外币，但用户只需要显示人民币项目，则忽略此（这主要是在应收/应付的情况下，
        //因为这两个科目的人民币明细列表格式也采用三栏式）
        //if((datas[i]->mt != RMB) && isOnlyRmb)
        //    continue;
        //if((datas[i]->mt == RMB) && !isOnlyRmb)
        //    continue;

        om = datas[i]->m;
        item = new ApStandardItem(datas[i]->m); //0：月
        l<<item;
        item = new ApStandardItem(datas[i]->d); //1：日
        l<<item;
        item = new ApStandardItem(datas[i]->pzNum);              //2：凭证号
        l<<item;
        item = new ApStandardItem(datas[i]->summary,Qt::AlignLeft | Qt::AlignVCenter); //3：摘要
        l<<item;
        Double rate = rates.value(datas[i]->m*10+datas[i]->mt);
        if(datas[i]->mt != RMB){
            item = new ApStandardItem(rate);  //4：汇率
            l<<item;
        }
        else
            l<<NULL;

        if(datas[i]->dh == DIR_J){
            for(int j = 0; j < mts.count(); ++j){
                if(mts[j] == datas[i]->mt){
                    item = new ApStandardItem(datas[i]->v);//5：借方（外币）
                    l<<item;
                }
                else
                    l<<NULL;
            }
            item = new ApStandardItem(datas[i]->v * rate);  //6：借方（金额）
            l<<item;
            jmsums += datas[i]->v * rate;
            jysums += datas[i]->v * rate;
            jwmsums[datas[i]->mt] += datas[i]->v;
            jwysums[datas[i]->mt] += datas[i]->v;
            for(int j = 0; j < mts.count()+1; ++j) //7、8：贷方（为空）
                l<<NULL;
        }
        else{
            for(int j = 0; j < mts.count()+1; ++j) //5、6：借方（为空）
                l<<NULL;
            for(int j = 0; j < mts.count(); ++j){
                if(mts[j] == datas[i]->mt){
                    item = new ApStandardItem(datas[i]->v);//7：贷方（外币）
                    l<<item;
                }
                else
                    l<<NULL;
            }
            item = new ApStandardItem(datas[i]->v * rate);//8：贷方（金额）
            l<<item;
            dmsums += datas[i]->v * rate;
            dysums += datas[i]->v * rate;
            dwmsums[datas[i]->mt] += datas[i]->v;
            dwysums[datas[i]->mt] += datas[i]->v;
        }
        item = new ApStandardItem(dirStr(datas[i]->dir));        //9：余额方向
        l<<item;
        for(int j = 0; j < mts.count(); ++j){
            item = new ApStandardItem(datas[i]->em.value(mts[j])); //10：余额（外币）
            l<<item;
        }
        item = new ApStandardItem(datas[i]->etm);    //11：余额（金额）
        l<<item;
        //添加两个隐藏列
        item = new ApStandardItem(datas[i]->pid);//12：业务活动所属凭证id
        l<<item;
        item = new ApStandardItem(datas[i]->bid);//13：业务活动本身的id
        l<<item;
        //dataModel->appendRow(l);
        pdatas<<l;
        rows++;
        l.clear();
    }
    //插入末尾的本月累计和本年累计行
    item = new ApStandardItem(ui->endDate->date().month()); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本月合计"));
    l<<item<<NULL;
    for(int j = 0; j < mts.count(); ++j){
        item = new ApStandardItem(jwmsums.value(mts[j]));  //5：借方（外币）
        l<<item;
    }
    item = new ApStandardItem(jmsums);  //6：借方（金额）
    l<<item;
    for(int j = 0; j < mts.count(); ++j){
        item = new ApStandardItem(dwmsums.value(mts[j]));  //7：贷方（外币）
        l<<item;
    }
    item = new ApStandardItem(dmsums);  //8：贷方（金额）
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preDir));  //9：期初余额方向
        l<<item;
        for(int i = 0; i < mts.count(); ++i){
            item = new ApStandardItem(preExtra.value(mts[i]));//10：期初余额
            l<<item;
        }
        item = new ApStandardItem(prev);//11：期初余额
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir));        //9：余额方向
        l<<item;
        for(int j = 0; j < mts.count(); ++j){
            item = new ApStandardItem(datas[datas.count()-1]->em.value(mts[j])); //10：余额（外币）
            l<<item;
        }
        item = new ApStandardItem(datas[datas.count()-1]->etm); //11：余额（金额）
        l<<item;
    }
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    item = new ApStandardItem(ui->endDate->date().month()); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本年累计")); //3：摘要
    l<<item<<NULL;
    for(int j = 0; j < mts.count(); ++j){
        item = new ApStandardItem(jwysums.value(mts[j]));  //5：借方（外币）
        l<<item;
    }
    item = new ApStandardItem(jysums);  //6：借方（金额）
    l<<item;
    for(int j = 0; j < mts.count(); ++j){
        item = new ApStandardItem(dwysums.value(mts[j]));  //7：贷方（外币）
        l<<item;
    }
    item = new ApStandardItem(dysums);  //8：贷方（金额）
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));  //9：期初余额方向
        l<<item;
        for(int i = 0; i < mts.count(); ++i){
            item = new ApStandardItem(dirStr(preDir));//10：期初余额
            l<<item;
        }
        item = new ApStandardItem(preExtra.value(RMB));//11：期初余额
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir));        //9：余额方向
        l<<item;
        for(int j = 0; j < mts.count(); ++j){
            item = new ApStandardItem(datas[datas.count()-1]->em.value(mts[j])); //10：余额（外币）
            l<<item;
        }
        item = new ApStandardItem(datas[datas.count()-1]->etm); //11：余额（金额）
        l<<item;
    }
    l<<NULL<<NULL;
    //dataModel->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    return rows;
}

//分页处理，参数 rowsInTable：每页可打印的最大行数，pageNum：需要多少页
void ShowDZDialog2::paging(int rowsInTable, int& pageNum)
{
    //如果当前显示的是所有明细科目或所有币种，则需要按明细科目、币种和可打印行数进行分页处理
    maxRows = rowsInTable;
    pageIndexes.clear();
    pdatas.clear(); //还要删除列表中的每个对象
    pageTfs.clear();//还要删除列表中的每个对象
    pfids.clear();
    psids.clear();
    pmts.clear();

    //1、确定要显示的总账科目、明细科目和币种的范围
    QList<int> fids,sids,mts;
    if(!curFSub)
        fids = curFilter->subIds;//即在打开明细视图前你指定的一级科目范围
    else
        fids<<curFSub->getId();
    if(!curMt)
        mts = allMts.keys();
    else
        mts<<curMt->code();

    //************************
    //下面这些代码要调整，

    //2、在一个3重循环内，逐个提取每项组合的发生项，并扩展为表格行数据至pdatas中
    //（即添加期初项、在每月的末尾添加本月合计和本年累计项）
    QList<DailyAccountData2*> datas;
    QHash<int,Double> preExtra;
    //QHash<int,Double> preExtraR;
    QHash<int,int> preExtraDir;
    QHash<int,Double>rates;
    int rows;  //每个组合产生的行数
    for(int i = 0; i < fids.count(); ++i){
        //sids.clear();
        //if(!curSSub)
        //    sids = this->sids.value(fids[i]);//即在打开明细视图前你指定的某个一级科目的二级科目范围
        //else
        //    sids<<curSSub->getId();
        for(int j = 0; j < sids.count(); ++j)
            for(int k = 0; k < mts.count(); ++k){
                //获取原始的发生项数据
                Double prev;
                int preDir;
                datas.clear();
                //if(!BusiUtil::getDailyAccount2(cury,sm,em,fids[i],sids[j],mts[k],
                //                              prev,preDir,datas,preExtra,preExtraR,
                //                              preExtraDir,rates))
                //    return;

                //跳过无余额且期间内未发生的
                if(datas.empty() && (preDir == DIR_P))
                    continue;


                int index = pdatas.count() - 1; //在扩展行数据之前，记下最后行的索引
                TableFormat tf = decideTableFormat(fids[i],sids[j],mts[k]); //打印此项组合要使用的表格类型
                //由原始数据扩展为行数据
                switch(tf){
                case CASHDAILY:
                    rows = genDataForCash(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
                    break;
                case BANKRMB:
                    rows = genDataForBankRMB(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
                    break;
                case BANKWB:
                    rows = genDataForBankWb(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
                    break;
                case COMMON:
                    rows = genDataForDetails(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
                    break;
                case THREERAIL:
                    rows = genDataForDetWai(datas,pdatas,prev,preDir,preExtra,preExtraDir,rates);
                    break;
                }

                //3、根据每项组合的边界和表格可容纳的最大行数的限制来分页，
                //即确定每页的最后一行在行数据列表中的索引至pageIndexes列表中

                //确定该项组合需要几页才可以打印完
                int pageSum;
                pageSum = rows / maxRows;
                if((rows % maxRows) > 0)
                    pageSum++;

                if(rows < maxRows){ //一页内可以打印
                    pageTfs<<tf;
                    pageIndexes<<pdatas.count()-1;
                    pfids<<fids[i];
                    psids<<sids[j];
                    pmts<<mts[k];
                    subPageNum[pageTfs.count()] = "1/1";
                }
                else{
                    int pages;
                    pages = rows/maxRows;
                    int pi = 0;
                    for(int p = 1; p <= pages; ++p){
                        pageTfs<<tf;
                        pageIndexes<<index+maxRows*p;
                        pfids<<fids[i];
                        psids<<sids[j];
                        pmts<<mts[k];
                        pi++;
                        subPageNum[pageTfs.count()] = QString("%1/%2").arg(pi).arg(pageSum);
                    }
                    if((rows % maxRows) > 0){ //添加没有满页的最末页
                        pageTfs<<tf;
                        pageIndexes<<pdatas.count()-1;
                        pfids<<fids[i];
                        psids<<sids[j];
                        pmts<<mts[k];
                        pi++;
                        subPageNum[pageTfs.count()] = QString("%1/%2").arg(pi).arg(pageSum);
                    }
                }
        }
    }
    pages = pageTfs.count();
    pageNum = pages;

    if(pages > 0){
        ui->pbr->setRange(1,pages+1);
    }
}

//生成指定打印页的表格数据，和各列的宽度
//参数 pageNUm：页号，colWidths：列宽，pdModel：表格数据，hModel：表头数据
//注意：触发信号的源，只是传送两个空指针，必须由处理槽进行new
void ShowDZDialog2::renPageData(int pageNum, QList<int>*& colWidths, QStandardItemModel& pdModel,
                               QStandardItemModel& phModel)
{
    if(pageNum > pageTfs.count())
        return;

    //生成列标题和列宽
    switch(pageTfs[pageNum-1]){
    case CASHDAILY:
        genThForCash(&phModel);
        break;
    case BANKRMB:
        genThForBankRmb(&phModel);
        break;
    case BANKWB:
        genThForBankWb(&phModel);
        break;
    case COMMON:
        genThForDetails(&phModel);
        break;
    case THREERAIL:
        genThForWai(&phModel);
        break;
    }
    colWidths = &this->colWidths[pageTfs[pageNum-1]];

    //从pdatas列表中提取属于指定页的行数据
    int start,end;
    if(pageNum == 1){
        start = 0;
        end = pageIndexes[0];
    }
    else{
        start = pageIndexes[pageNum-2]+1;
        end = pageIndexes[pageNum -1];
    }
    //当调用pdModel.clear()删除内容时，可能也删除了pdatas中的指向的对象，因此
    //每次装载数据时，必须重新创建每个项目对象，否则在往前翻页时会崩溃。
    QList<QStandardItem*> l;
    QStandardItem* item;
    Qt::Alignment align;
    for(int i = start; i <= end; ++i){
        for(int j = 0; j < pdatas[i].count(); ++j){
            if(pdatas[i][j]){
                align = pdatas[i][j]->textAlignment();
                item = new QStandardItem(pdatas[i][j]->text());
                item->setTextAlignment(align);
                item->setEditable(false);
            }
            else
                item = NULL;
            l<<item;
        }
        pdModel.appendRow(l);
        l.clear();
    }

    //处理其他页面数据
    QString s;
    SubjectManager* sm = curAccount->getSubjectManager();
    if((pfids[pageNum-1] == subCashId) ||
        pfids[pageNum-1] == subBankId)
        s = tr("%1日记账").arg(sm->getFstSubject(pfids[pageNum-1])->getName());
    else
        s = tr("%1明细账").arg(sm->getFstSubject(pfids[pageNum-1])->getName());;
    pt->setTitle(s);
    pt->setPageNum(subPageNum.value(pageNum));
    //pt->setMasteMt(allMts.value(RMB));
    //pt->setDateRange(cury,sm,em);
    //SubjectManager* sm = curAccount->getSubjectManager();
    s = tr("%1（%2）——（%3）")
            .arg(sm->getFstSubject(pfids[pageNum-1])->getName())
            .arg(sm->getFstSubject(pfids[pageNum-1])->getCode())
            .arg(sm->getSndSubject(psids[pageNum-1])->getLName());
    pt->setSubName(s);
    //pt->setAccountName(curAccInfo->accLName);
    //pt->setCreator(curUser->getName());
    //pt->setPrintDate(QDate::currentDate());
    //pt->update();
    //pt->reLayout();

    if(curPrintTask == PREVIEW)
        return;
    else
        ui->pbr->setVisible(true);

    ui->pbr->setValue(pageNum);
    if(pageNum == pages)
        ui->pbr->setVisible(false);


}

//预先分页信号（用以使视图可以设置打印的页面范围
//参数out：false:表示是由预览框本身负责分页，true：由外部负责视图分页
//void ShowDZDialog2::priorPaging(bool out, int pages)
//{
//    if(out){
//        int rows = dataModel->rowCount();
//        this->pages = rows/pages;
//        if((rows % pages) != 0)
//            this->pages++;
//    }
//    else
//        this->pages = pages;
//}

//打印任务公共操作
void ShowDZDialog2::printCommon(QPrinter* printer)
{
    HierarchicalHeaderView* thv = new HierarchicalHeaderView(Qt::Horizontal);
    ProxyModelWithHeaderModels* m = new ProxyModelWithHeaderModels;
    SubjectManager* smg = curAccount->getSubjectManager();

    if(!curFSub || !curSSub || !curMt){
        m->setHorizontalHeaderModel(headerModel);
    }
    else{
        m->setModel(dataModel);
        m->setHorizontalHeaderModel(headerModel);
    }
    //创建打印模板实例
    QList<int> colw(colPrtWidths[tf]);
    if(pt == NULL)
        pt = new PrintTemplateDz(m,thv,&colw);

    //如果是明确的一二级科目和币种的组合，即由预览框来负责分页处理
    if(curFSub && curSSub && curMt){
        QString s;
        if((curFSub->getId() == smg->getCashSub()->getId()) || (curFSub->getId() == smg->getBankSub()->getId()))
            s = tr("%1日记账").arg(curFSub->getName());
        else
            s = tr("%1明细账").arg(curFSub->getName());
        pt->setTitle(s);
        pt->setMasteMt(allMts.value(RMB)->name());
        pt->setDateRange2(ui->startDate->date(),ui->endDate->date());
        s = tr("%1（%2）——（%3）").arg(curFSub->getName())
                .arg(curFSub->getCode())
                .arg(curSSub->getLName());
        pt->setSubName(s);
        pt->setAccountName(curAccount->getLName());
        pt->setCreator(curUser->getName());
        pt->setPrintDate(QDate::currentDate());

    }
    //如果由明细视图来负责分页处理，则要进行占位处理，否则，在生成页面数据后，
    //打印预览正常，但打印到文件或打印机则截短了字符串的长度到原始长度
    else{
        pt->setTitle("          ");
        pt->setMasteMt(allMts.value(RMB)->name());
        pt->setDateRange2(ui->startDate->date(),ui->endDate->date());
        pt->setSubName("sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss");
        pt->setAccountName(curAccount->getLName());
        pt->setCreator(curUser->getName());
        pt->setPrintDate(QDate::currentDate());
    }
    //设置打印机的页边距和方向
    printer->setPageMargins(margins.left,margins.top,margins.right,margins.bottom,margins.unit);
    printer->setOrientation(pageOrientation);

    if(preview != NULL){
        delete preview;
        preview = NULL;
    }
    if(!curFSub || !curSSub || !curMt){
        preview = new PreviewDialog(pt,DETAILPAGE,printer,true);
        connect(preview,SIGNAL(reqPaging(int,int&)),
                this,SLOT(paging(int,int&)));
        connect(preview,SIGNAL(reqPageData(int,QList<int>*&,QStandardItemModel&,QStandardItemModel&)),
                this,SLOT(renPageData(int,QList<int>*&,QStandardItemModel&,QStandardItemModel&)));
    }
    else
        preview = new PreviewDialog(pt,DETAILPAGE,printer);

    //connect(preview,SIGNAL(priorPaging(bool,int)),this,SLOT(priorPaging(bool,int)));
    //preview->setupPage(true);

    //设置打印页的范围
    printer->setFromTo(1,pages);

    if(curPrintTask == PREVIEW){
        //接收打印页面设置的修改
        if(preview->exec() == QDialog::Accepted){
            for(int i = 0; i < colw.count(); ++i)
                colPrtWidths[tf][i] = colw[i];
            pageOrientation = printer->orientation();
            printer->getPageMargins(&margins.left,&margins.top,&margins.right,
                                    &margins.bottom,margins.unit);
        }
    }
    else if(curPrintTask == TOPRINT){
        preview->print();
    }
    else{
        QString filename;
        preview->exportPdf(filename);
    }

    delete thv;
    delete m;
    delete pt;
    pt = NULL;
}

//打印
void ShowDZDialog2::on_actPrint_triggered()
{
    curPrintTask = TOPRINT;
    QPrinter* printer= new QPrinter(QPrinter::PrinterResolution);
    printCommon(printer);
    delete printer;
    delete preview;
    preview = NULL;
}

//打印预览
void ShowDZDialog2::on_actPreview_triggered()
{
    curPrintTask = PREVIEW;
    QPrinter* printer= new QPrinter(QPrinter::HighResolution);
    printCommon(printer);
    delete printer;
    delete preview;
    preview = NULL;
}

//打印到PDF文件
void ShowDZDialog2::on_actToPdf_triggered()
{
    curPrintTask = TOPDF;
    QPrinter* printer= new QPrinter(QPrinter::ScreenResolution);
    printCommon(printer);
    delete printer;
    delete preview;
    preview = NULL;
}

//void ShowDZDialog2::on_btnClose_clicked()
//{
//    emit closeWidget();
//}

/**
 * @brief ShowDZDialog2::on_btnSaveFiler_clicked
 *  保存过滤条件
 */
void ShowDZDialog2::on_btnSaveFilter_clicked()
{

    if(!account->getDbUtil()->saveDetViewFilter(filters))
        QMessageBox::critical(this,tr("出错信息"),tr("保存明细账视图的过滤条件时出错！"));
    ui->btnSaveFilter->setEnabled(false);
}

/**
 * @brief ShowDZDialog2::on_btnSaveAs_clicked
 *  将默认过滤条件保存为命名的过滤条件
 */
void ShowDZDialog2::on_btnSaveAs_clicked()
{
    //如果当前选择的过滤条件是默认的，则允许用户为创建一个新的过滤条件而输入命名的名称
    if(!curFilter->isDef)
        return;

    QString name; bool ok;
    name = QInputDialog::getText(this,tr("信息输入"),tr("如果你要创建新的过滤条件，以便以后使用，则请输入一个过滤条件的名称"),QLineEdit::Normal,"",&ok);
    if(!ok)
        return;
    DVFilterRecord* fr = new DVFilterRecord;
    fr->id = 0;
    fr->editState = CIES_NEW;
    fr->isCur = true;
    fr->isDef = false;
    fr->isFst = curFilter->isFst;
    fr->curFSub = curFilter->curFSub;
    fr->curSSub = curFilter->curSSub;
    fr->curMt = curFilter->curMt;
    fr->name = name;
    fr->startDate = curFilter->startDate;
    fr->endDate = curFilter->endDate;
    fr->subIds = curFilter->subIds;
    filters<<fr;
    QVariant v;
    v.setValue<DVFilterRecord*>(fr);
    QListWidgetItem* li = new QListWidgetItem(name);
    li->setData(Qt::UserRole,v);;
    ui->lstHistory->addItem(li);
    ui->lstHistory->setCurrentItem(li);
    on_btnSaveFilter_clicked();
}

/**
 * @brief ShowDZDialog2::on_btnDelFilter_clicked
 *  删除选定的过滤条件项目
 */
void ShowDZDialog2::on_btnDelFilter_clicked()
{
    QListWidgetItem* item = ui->lstHistory->currentItem();
    DVFilterRecord* r = item->data(Qt::UserRole).value<DVFilterRecord*>();
    r->editState = CIES_DELETED;
    item->setHidden(true);
}

/**
 * @brief ShowDZDialog2::on_btnSubRange_clicked
 *  打开设定科目范围对话框
 */
void ShowDZDialog2::on_btnSubRange_clicked()
{
    SubjectRangeSelectDialog* dlg = new SubjectRangeSelectDialog(smg,curFilter->subIds,curFilter->curFSub,this);
    if(QDialog::Rejected == dlg->exec())
        return;

    curFilter->subIds = dlg->getSelectedSubIds();
    curFilter->isFst = dlg->isSelectedFst();
    adjustSaveBtn();
    initFilter();
}

void ShowDZDialog2::on_btnRefresh_clicked()
{
    refreshTalbe();
}



ShowDZDialog2::TableFormat ShowDZDialog2::decideTableFormat(int fid,int sid, int mt)
{
    //判定要采用的表格格式
    TableFormat tf;
    if(fid == 0)
        tf = THREERAIL;
    else if(fid == smg->getCashSub()->getId())
        tf = CASHDAILY;
    else if((fid == subBankId) && (sid == 0)) //银行-所有
        tf = BANKWB;
    else if((fid == subBankId) && (mt == RMB))
        tf = BANKRMB;
    else if((fid == subBankId) && (mt != RMB))
        tf = BANKWB;
    else if((fid == subYsId) || (fid == subYfId))
        tf = THREERAIL;
    else
        tf = COMMON;
    return tf;
}



/////////////////////////////////SubjectRangeSelectDialog//////////////////////////////////////////////
SubjectRangeSelectDialog::SubjectRangeSelectDialog(SubjectManager* smg, const QList<int>& subIds, int fstSubId, QWidget *parent)
    :QDialog(parent),ui(new Ui::SubjectRangeSelectDialog),smg(smg)
{
    ui->setupUi(this);
    this->subIds = subIds;
    curFsub = smg->getFstSubject(fstSubId);
    loadSubjects();
    connect(ui->rdoFst,SIGNAL(toggled(bool)),this,SLOT(onSubjectSelectModeChanged(bool)));
    connect(ui->lstFstSubs,SIGNAL(currentRowChanged(int)),this,SLOT(curFstSubChanged(int)));
}

SubjectRangeSelectDialog::~SubjectRangeSelectDialog()
{
}

/**
 * @brief SubjectRangeSelectDialog::getSelectedSubIds
 *  获取选择的科目id
 * @return
 */
QList<int> SubjectRangeSelectDialog::getSelectedSubIds()
{
    QList<int> sids;
    if(ui->rdoFst->isChecked()){
        QTableWidgetItem* ti;
        for(int r = 0; r < ui->tblFstSubs->rowCount(); ++r){
            ti = ui->tblFstSubs->item(r,1);
            if(ti->checkState() == Qt::Checked)
                sids<<ti->data(Qt::UserRole).value<FirstSubject*>()->getId();

        }
    }
    else{
        QListWidgetItem* li;
        for(int i = 0; i < ui->lstSndSubs->count(); ++i){
            li = ui->lstSndSubs->item(i);
            if(li->checkState() == Qt::Checked)
                sids<<li->data(Qt::UserRole).value<SecondSubject*>()->getId();
        }
    }
    return sids;
}

/**
 * @brief SubjectRangeSelectDialog::isSelectedFst
 *  选择的是一级科目还是二级科目
 * @return
 */
bool SubjectRangeSelectDialog::isSelectedFst()
{
    return ui->rdoFst->isChecked();
}

/**
 * @brief SubjectRangeSelectDialog::getSelectedFstSub
 *  获取选择的一级科目（在选择一级科目模式时）
 * @return
 */
FirstSubject *SubjectRangeSelectDialog::getSelectedFstSub()
{
    if(ui->rdoFst->isChecked())
        return NULL;
    QListWidgetItem* item = ui->lstFstSubs->currentItem();
    return item->data(Qt::UserRole).value<FirstSubject*>();
}

//SecondSubject *SubjectRangeSelectDialog::getSelectedSndSub()
//{

//}



void SubjectRangeSelectDialog::loadSubjects()
{
    QVariant v;
    QListWidgetItem* li;
    QTableWidgetItem* ti;
    FirstSubject* fsub;
    ui->tblFstSubs->clearContents();
    ui->lstSndSubs->clear();

    //装载一级科目    
    FSubItrator* it = smg->getFstSubItrator();
    int r = -1;
    while(it->hasNext()){
        it->next();
        fsub = it->value();
        if(!fsub->isEnabled())
            continue;
        r++;
        v.setValue<FirstSubject*>(fsub);
        ui->tblFstSubs->insertRow(r);
        ti = new QTableWidgetItem(fsub->getCode());
        ui->tblFstSubs->setItem(r,0,ti);
        ti = new QTableWidgetItem(fsub->getName());
        ti->setCheckState((!curFsub && subIds.contains(fsub->getId()))?Qt::Checked:Qt::Unchecked);
        ti->setData(Qt::UserRole,v);
        ui->tblFstSubs->setItem(r,1,ti);
    }

    //装载二级科目
    it->toFront();
    int index = -1,curIndex = -1;
    while(it->hasNext()){
        it->next();
        fsub = it->value();
        if(!fsub->isEnabled())
            continue;
        index++;
        v.setValue<FirstSubject*>(fsub);
        li = new QListWidgetItem(fsub->getName());
        li->setData(Qt::UserRole,v);
        ui->lstFstSubs->addItem(li);
        if(curFsub && (curFsub->getId() == fsub->getId()))
            curIndex = index;
    }
    ui->lstFstSubs->setCurrentRow(curIndex);
    if(curIndex != -1){
        SecondSubject* ssub;
        for(int i = 0; i < curFsub->getChildCount(); ++i){
            ssub = curFsub->getChildSub(i);
            v.setValue<SecondSubject*>(ssub);
            li = new QListWidgetItem(ssub->getName());
            li->setCheckState(subIds.contains(ssub->getId())?Qt::Checked:Qt::Unchecked);
            ui->lstSndSubs->addItem(li);            
        }
    }
    connect(ui->lstSndSubs,SIGNAL(itemChanged(QListWidgetItem*)),
            this,SLOT(itemCheckStateChanged(QListWidgetItem*)));
    if(!curFsub){
        ui->stackedWidget->setCurrentIndex(0);
        ui->rdoFst->setChecked(true);
    }
    else{
        ui->stackedWidget->setCurrentIndex(1);
        ui->rdoSnd->setChecked(true);
    }
    isAllSelected();
    isAllSelected(false);
}

/**
 * @brief SubjectRangeSelectDialog::isAllSelected
 *  判断所有一级科目（一级科目选择模式下）或二级科目（二级科目选择模式下）是否都选择了或都不选择
 *  进而决定全选和全不选按钮的启用状态
 * @param isFst true：一级科目选择模式，flase：二级科目选择模式下
 */
void SubjectRangeSelectDialog::isAllSelected(bool isFst)
{
    bool isAll = true;
    bool isAllNot = true;
    Qt::CheckState state;
    if(isFst){
        for(int r = 0; r < ui->tblFstSubs->rowCount(); ++r){
            state = ui->tblFstSubs->item(r,1)->checkState();
            if(state == Qt::Unchecked){
                isAll = false;
                if(!isAllNot)
                    break;
            }
            else{
                isAllNot = false;
                if(!isAll)
                    break;
            }
        }
        ui->btnSelAll->setEnabled(!isAll);
        ui->btnSelAllNot->setEnabled(!isAllNot);
    }
    else{
        for(int r = 0; r < ui->lstSndSubs->count(); ++r){
            state = ui->lstSndSubs->item(r)->checkState();
            if(state == Qt::Unchecked){
                isAll = false;
                if(!isAllNot)
                    break;
            }
            else{
                isAllNot = false;
                if(!isAll)
                    break;
            }
        }
        ui->btnSelAllSnd->setEnabled(!isAll);
        ui->btnSelAllSndNot->setEnabled(!isAllNot);
    }
}

/**
 * @brief SubjectRangeSelectDialog::onSubjectSelectModeChanged
 *  当科目选择模式改变时
 * @param checked   true：一级科目，false：二级科目
 */
void SubjectRangeSelectDialog::onSubjectSelectModeChanged(bool checked)
{
    if(checked)
        ui->stackedWidget->setCurrentIndex(0);
    else
        ui->stackedWidget->setCurrentIndex(1);
}

/**
 * @brief SubjectRangeSelectDialog::curFstSubChanged
 *  在二级科目选择模式下，当选择的一级科目改变时，刷新右边的列表，显示该一级科目下的所有二级科目及其选择状态
 * @param currentRow
 */
void SubjectRangeSelectDialog::curFstSubChanged(int currentRow)
{
    ui->lstSndSubs->clear();
    FirstSubject* fsub = ui->lstFstSubs->item(currentRow)->data(Qt::UserRole).value<FirstSubject*>();
    SecondSubject* ssub;
    QVariant v;
    QListWidgetItem* item;
    for(int i = 0; i < fsub->getChildCount(); ++i){
        ssub = fsub->getChildSub(i);
        v.setValue<SecondSubject*>(ssub);
        item = new QListWidgetItem(ssub->getName());
        item->setData(Qt::UserRole,v);
        item->setCheckState(subIds.contains(ssub->getId())?Qt::Checked:Qt::Unchecked);
        ui->lstSndSubs->addItem(item);
    }
    isAllSelected(false);
}

void SubjectRangeSelectDialog::itemCheckStateChanged(QListWidgetItem *item)
{
    isAllSelected(false);
}

void SubjectRangeSelectDialog::on_btnSelAllSnd_clicked()
{    
    for(int r = 0; r < ui->lstSndSubs->count(); ++r)
        ui->lstSndSubs->item(r)->setCheckState(Qt::Checked);
}

void SubjectRangeSelectDialog::on_btnSelAllSndNot_clicked()
{
    for(int r = 0; r < ui->lstSndSubs->count(); ++r)
        ui->lstSndSubs->item(r)->setCheckState(Qt::Unchecked);
}

/**
 * @brief SubjectRangeSelectDialog::on_btnSelAll_clicked
 *  全选或反选一级科目
 */
void SubjectRangeSelectDialog::on_btnSelAll_clicked()
{
    for(int r = 0; r < ui->tblFstSubs->rowCount(); ++r)
        ui->tblFstSubs->item(r,1)->setCheckState(Qt::Checked);
}

void SubjectRangeSelectDialog::on_btnSelAllNot_clicked()
{
    for(int r = 0; r < ui->tblFstSubs->rowCount(); ++r)
        ui->tblFstSubs->item(r,1)->setCheckState(Qt::Unchecked);
}



