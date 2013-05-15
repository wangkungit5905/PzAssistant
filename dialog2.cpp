#include <qxml.h>
#include <QDomDocument>
#include <QSqlQuery>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QBuffer>
#include <QItemSelectionModel>

#include <qglobal.h>


#include "dialog2.h"
#include "utils.h"
#include "widgets.h"
#include "printUtils.h"
#include "global.h"
#include "widgets.h"
#include "utils.h"
#include "completsubinfodialog.h"
#include "mainwindow.h"
#include "tdpreviewdialog.h"
#include "previewdialog.h"
#include "tables.h"
#include "subject.h"
#include "cal.h"
#include "dbutil.h"

//tem
//#include "dialog3.h"

#ifdef Q_OS_LINUX
#define	FW_NORMAL	400
#define	FW_BOLD		700
#endif


BASummaryForm::BASummaryForm(QWidget *parent) : QWidget(parent),
    ui(new Ui::BASummaryForm)
{
    ui->setupUi(this);
    setLayout(ui->mLayout);    

    //初始化对方科目列表
    QSqlQuery q;
    bool r = q.exec(QString("select id,%1 from %2 where %3=1")
                    .arg(fld_fsub_name).arg(tbl_fsub).arg(fld_fsub_isview));
    ui->cmbOppSub->addItem("", 0);  //不指向任何科目
    while(q.next()){
        int id = q.value(0).toInt();
        QString name = q.value(1).toString();
        ui->cmbOppSub->addItem(name, id);
    }

}

BASummaryForm::~BASummaryForm()
{
    emit dataEditCompleted(ActionEditItemDelegate2::SUMMARY);
    delete ui;
}

//将摘要字段的数据进行分离，分别填入不同的显示部件中
void BASummaryForm::setData(QString data)
{
    QString summ,xmlStr,refNum,bankNum,opSub;
    int start = data.indexOf("<");
    int end   = data.lastIndexOf(">");
    if((start > 0) && (end > start)){
        summ = data.left(start);
        ui->edtSummary->setText(summ);
        xmlStr = data.right(data.count() - start);
        QDomDocument dom;
        bool r = dom.setContent(xmlStr);
        if(dom.elementsByTagName("fp").count() > 0){
            QDomNode node = dom.elementsByTagName("fp").at(0);
            QString fp = node.childNodes().at(0).nodeValue();
            ui->edtFpNum->setText(fp);
        }
        if(dom.elementsByTagName("bp").count() > 0){
            QDomNode node = dom.elementsByTagName("bp").at(0);
            QString bp = node.childNodes().at(0).nodeValue();
            ui->edtBankNum->setText(bp);
        }
        if(dom.elementsByTagName("op").count() > 0){
            QDomNode node = dom.elementsByTagName("op").at(0);
            int op = node.childNodes().at(0).nodeValue().toInt();
            int idx = ui->cmbOppSub->findData(op);
            ui->cmbOppSub->setCurrentIndex(idx);
        }
    }
    else{
        ui->edtSummary->setText(data);
    }

}



QString BASummaryForm::getData()
{
    bool isHave = false;
    QDomDocument dom;
    QDomElement root = dom.createElement("rp");
    QString fpStr = ui->edtFpNum->text();
    QString bpStr = ui->edtBankNum->text();

    if(fpStr != ""){
        isHave = true;
        QDomText fpText = dom.createTextNode(fpStr);
        QDomElement fpEle = dom.createElement("fp");
        fpEle.appendChild(fpText);
        root.appendChild(fpEle);
    }

    if(bpStr != ""){
        isHave = true;
        QDomText bpText = dom.createTextNode(bpStr);
        QDomElement bpEle = dom.createElement("bp");
        bpEle.appendChild(bpText);
        root.appendChild(bpEle);
    }

    if(ui->cmbOppSub->currentIndex() != 0){
        isHave = true;
        int id = ui->cmbOppSub->itemData(ui->cmbOppSub->currentIndex()).toInt();
        QDomText opText = dom.createTextNode(QString::number(id));
        QDomElement opEle = dom.createElement("op");
        opEle.appendChild(opText);
        root.appendChild(opEle);
    }

    QString s;
    if(isHave){
        dom.appendChild(root);
        s = ui->edtSummary->text()/*.append("\n")*/.append(dom.toString());
        s.chop(1);
    }
    else
        s = ui->edtSummary->text();

    return s;
}

//////////////////显示科目余额（本期统计）对话框（新）///////////////////////////////////////////
ViewExtraDialog::ViewExtraDialog(Account* account, int y, int m, QByteArray* sinfo, QWidget *parent) :
    QDialog(parent),ui(new Ui::ViewExtraDialog),y(y),m(m),account(account)
{
    ui->setupUi(this);

    headerModel = NULL;
    dataModel = NULL;
    imodel = NULL;
    dbUtil = account->getDbUtil();
    smg = account->getSubjectManager();

    //初始化自定义的层次式表头
    hv = new HierarchicalHeaderView(Qt::Horizontal, ui->tview);
    hv->setHighlightSections(true);
    hv->setClickable(true);
    //hv->setStyleSheet("QHeaderView::section {background-color:darkcyan;}");
    ui->tview->setHorizontalHeader(hv);

    //初始化货币代码列表，并使它们以一致的顺序显示
    mts = account->getAllMoneys().keys();
    mts.removeOne(account->getMasterMt()->code());
    qSort(mts.begin(),mts.end()); //为了使人民币总是第一个    

    //添加按钮菜单
    mnuPrint = new QMenu;
    mnuPrint->addAction(ui->actPrint);
    mnuPrint->addAction(ui->actPreview);
    mnuPrint->addAction((ui->actToPDF));
    mnuPrint->addAction(ui->actToExcel);
    ui->btnPrt->setMenu(mnuPrint);

    //初始化一级科目组合框
    ui->cmbFstSub->addItem(tr("所有"),0);
    FSubItrator* fsubIt = smg->getFstSubItrator();
    QVariant v;
    while(fsubIt->hasNext()){
        fsubIt->next();
        v.setValue<FirstSubject*>(fsubIt->value());
        ui->cmbFstSub->addItem(fsubIt->value()->getName(),v);
    }
    ui->cmbSndSub->addItem(tr("所有"),0);
    fsub = NULL; ssub = NULL;
    fcom = new SubjectComplete;
    scom = new SubjectComplete(SndSubject);
    ui->cmbFstSub->setCompleter(fcom);
    ui->cmbSndSub->setCompleter(scom);

    ui->lbly->setText(QString::number(y));
    ui->lblm->setText(QString::number(m));

    setState(sinfo);
    setDate(y,m);

    connect(ui->cmbFstSub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelFstSub(int)));
    connect(ui->cmbSndSub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int))
            ,this, SLOT(colWidthChanged(int,int,int)));
}

ViewExtraDialog::~ViewExtraDialog()
{
    delete ui;
}

//设置统计年月
void ViewExtraDialog::setDate(int y, int m)
{
    this->y = y;
    this->m = m;
    ui->lbly->setText(QString::number(y));
    ui->lblm->setText(QString::number(m));

    initHashs();
    viewRates();
    viewTable();
}


//恢复窗口状态（获取表格的各字段的宽度信息（返回列表的第一个元素是当前显示的表格类型））
//第一次调用此函数时，已经显示表格数据（此函数应在setDate函数后得到调用）
void ViewExtraDialog::setState(QByteArray* info)
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
    ui->cmbSndSub->setEnabled(stateInfo.viewDetails);

    connect(ui->rdoJe,SIGNAL(toggled(bool)),this,SLOT(onTableFormatChanged(bool)));
    connect(ui->chkIsDet,SIGNAL(toggled(bool)),this,SLOT(onDetViewChanged(bool)));
}

//保存表格状态
QByteArray* ViewExtraDialog::getState()
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

//当表格的列宽改变时
void ViewExtraDialog::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    stateInfo.colWidths[stateInfo.tFormat][logicalIndex] = newSize;
}

/**
 * @brief ViewExtraDialog::save
 *  保存当前余额数据到数据库中
 */
void ViewExtraDialog::save()
{
	if(curAccount->getReadOnly())
        return;
    QHash<int,Double> oldF,oldS;    //前期余额值
    QHash<int,MoneyDirection> oldFDir,oldSDir; //前期余额方向
    BusiUtil::readExtraByMonth2(y,m,oldF,oldFDir,oldS,oldSDir);
    BusiUtil::savePeriodBeginValues2(y,m,endExa,endExaDir,endDetExa,endDetExaDir,false);
    BusiUtil::savePeriodEndValues(y,m,endExaR,endDetExaR);//以本币计的外币余额
    emit pzsExtraSaved();
    ui->btnSave->setEnabled(false);
    ui->btnCancel->setEnabled(false);
    ui->btnClose->setEnabled(true);
}

/**
 * @brief ViewExtraDialog::onSelFstSub
 *  当选择一个总账科目时，初始化可用的明细科目
 * @param index
 */
void ViewExtraDialog::onSelFstSub(int index)
{
    fsub = ui->cmbFstSub->itemData(index).value<FirstSubject*>();
    disconnect(ui->cmbSndSub, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    if(index == 0){
        ui->cmbSndSub->clear();
        ui->cmbSndSub->addItem(tr("所有"),0);
        ui->cmbSndSub->setEnabled(false);
    }
    else{
        ui->cmbSndSub->setEnabled(true);
        ui->cmbSndSub->clear();
        ui->cmbSndSub->addItem(tr("所有"),0);
        QVariant v;
        foreach(SecondSubject* sub, fsub->getChildSubs()){
            v.setValue<SecondSubject*>(sub);
            ui->cmbSndSub->addItem(sub->getName(),v);
        }
        scom->setPid(fsub->getId());
    }
    ssub = NULL;
    connect(ui->cmbSndSub, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    viewTable();
}

/**
 * @brief ViewExtraDialog::onSelSndSub
 *  当用户选择一个二级科目时，显示指定二级科目的统计数据
 * @param index
 */
void ViewExtraDialog::onSelSndSub(int index)
{
    ssub = ui->cmbSndSub->itemData(index).value<SecondSubject*>();
    viewTable();
}

/**
 * @brief ViewExtraDialog::onTableFormatChanged
 *  用户选择了一种表格格式
 * @param checked
 */
void ViewExtraDialog::onTableFormatChanged(bool checked)
{
    if(checked)
        stateInfo.tFormat = COMMON;
    else
        stateInfo.tFormat = THREERAIL;
    viewTable();
}

/**
 * @brief ViewExtraDialog::onDetViewChanged
 *  用户选择需不需要显示明细科目
 * @param checked
 */
void ViewExtraDialog::onDetViewChanged(bool checked)
{
    stateInfo.viewDetails = checked;
    ui->cmbSndSub->setEnabled(checked);
    viewTable();
}

/**
 * @brief ViewExtraDialog::printCommon
 *  打印通用操作
 * @param task
 * @param printer
 */
void ViewExtraDialog::printCommon(PrintTask task, QPrinter* printer)
{
    HierarchicalHeaderView* thv = new HierarchicalHeaderView(Qt::Horizontal);
    ProxyModelWithHeaderModels* m = new ProxyModelWithHeaderModels;
    m->setModel(dataModel);
    m->setHorizontalHeaderModel(headerModel);

    //创建打印模板实例
    QList<int> colw(stateInfo.colPriWidths.value(stateInfo.tFormat));
    PrintTemplateStat* pt = new PrintTemplateStat(m,thv,&colw);
    pt->setAccountName(curAccount->getLName());
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
    delete m;
}

/**
 * @brief ViewExtraDialog::on_actPrint_triggered
 *  打印表格
 */
void ViewExtraDialog::on_actPrint_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::PrinterResolution);
    printCommon(TOPRINT, printer);
    delete printer;
}

/**
 * @brief ViewExtraDialog::on_actPreview_triggered
 *  打印预览
 */
void ViewExtraDialog::on_actPreview_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::HighResolution);
    printCommon(PREVIEW, printer);
    delete printer;
}

/**
 * @brief ViewExtraDialog::on_actToPDF_triggered
 *  打印到pdf文件
 */
void ViewExtraDialog::on_actToPDF_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::ScreenResolution);
    printCommon(TOPDF, printer);
    delete printer;
}

/**
 * @brief ViewExtraDialog::viewRates
 *  显示期初和期末汇率
 */
void ViewExtraDialog::viewRates()
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
 * @brief ViewExtraDialog::calSumByMt
 *  分别合计所有科目的各币种合计值
 * @param exasR     科目余额（键为科目id*10+币种代码，值为以本币计的余额值及其方向）
 * @param exaDirsR  余额方向
 * @param sumsR     合计后的余额（键为科目id，值为以本币计的合计后的余额值及其方向）
 * @param dirsR     合计后的余额方向
 * @param witch     1：表示一级科目，2：二级科目
 */
void ViewExtraDialog::calSumByMt(QHash<int, Double> exasR,
                                 QHash<int, MoneyDirection> exaDirsR,
                                 QHash<int, Double> &sumsR,
                                 QHash<int, MoneyDirection> &dirsR, int witch)
{
    QHashIterator<int,Double> it(exasR);
    int id;
    FirstSubject* fsub; SecondSubject* ssub;

    //基本思路是借方　－　贷方，并根据差值的符号来判断余额方向
    while(it.hasNext()){
        it.next();
        id = it.key() / 10;
        if(exaDirsR.value(it.key()) == MDIR_P){
            if(sumsR.contains(id))
                continue;
            else
                sumsR[id] = 0;
        }
        else if(exaDirsR.value(it.key()) == MDIR_J)
            sumsR[id] += it.value();
        else
            sumsR[id] -= it.value();
    }
    bool isIn ;  //是否是损益类凭证的收入类科目
    bool isFei; //是否是损益类凭证的费用类科目

    QHashIterator<int,Double> i(sumsR);
    while(i.hasNext()){
        i.next();
        id = i.key();
        if(witch == 1){
            fsub = smg->getFstSubject(id);
            ssub = NULL;
        }
        else{
            ssub = smg->getSndSubject(id);
            fsub = ssub->getParent();
        }
        if(i.value() == 0)
            dirsR[id] = MDIR_P;
        else if(i.value() > 0){
            isIn = (smg->isSySubject(fsub->getId()) && !fsub->getJdDir());
            //如果是收入类科目，要将它固定为贷方
            if(isIn){
                sumsR[id].changeSign();
                dirsR[id] = MDIR_D;
            }
            else{
                dirsR[id] = MDIR_J;
            }
        }
        else{
            isFei = (smg->isSySubject(fsub->getId()) && fsub->getJdDir());
            //如果是费用类科目，要将它固定为借方
            if(isFei){
                dirsR[id] = MDIR_J;
                //为什么不需要更改金额符号，是因为对于一般的科目是根据符号来判断科目的余额方向
                //但对费用类科目，我们认为即使是负数也将其定为借方（因为运算法则是借方-贷方，方向任为借方）
                //sumsR[id].changeSign();
            }
            else{
                sumsR[id].changeSign();
                dirsR[id] = MDIR_D;
            }
        }
    }
}


//将表格数据导出的excel文件
void ViewExtraDialog::on_actToExcel_triggered()
{
    QMessageBox::information(this,tr("提示信息"),tr("本功能未实现！"));
//    QFileDialog* dlg = new QFileDialog(this);
//    dlg->setConfirmOverwrite(false);
//    dlg->setAcceptMode(QFileDialog::AcceptSave);
//    dlg->setDefaultSuffix("xls");
//    QString curPath = QDir::currentPath();
//    curPath.append("/").append("outExcels");
//    curPath = QDir::toNativeSeparators(curPath);
//    dlg->setDirectory(curPath);
//    QString fname;
//    if(dlg->exec() == QDialog::Accepted){
//        QStringList flst = dlg->selectedFiles();
//        if(flst.count()==1){
//            fname = QDir::toNativeSeparators(flst[0]);
//            fname = fname.replace("\\", "\\\\");
//        }
//        else
//            return;
//    }

//    BasicExcel* xls = new BasicExcel;
//#ifdef Q_OS_WIN32
//    xls->setVisible(true);
//#endif
//    xls->New(1);
//    genSheetHeader(xls);
//    genSheetDatas(xls);

//    xls->SaveAs(fname);
}


//在表格中显示统计的数据（本期发生额及其余额）
void ViewExtraDialog::viewTable()
{
    genHeaderDatas();
    genDatas();

    if(imodel)
        delete imodel;
    imodel = new ProxyModelWithHeaderModels;
    imodel->setModel(dataModel);
    imodel->setHorizontalHeaderModel(headerModel);
    ui->tview->setModel(imodel);
    //ui->tview->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color:darkcyan;}");

    disconnect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int))
            ,this, SLOT(colWidthChanged(int,int,int)));
    //设置列宽
    TableFormat curFmt = stateInfo.tFormat;
    for(int i = 0; i < stateInfo.colWidths.value(curFmt).count(); ++i)
        ui->tview->setColumnWidth(i,stateInfo.colWidths.value(curFmt)[i]);
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int))
            ,this, SLOT(colWidthChanged(int,int,int)));

    //决定保存按钮的启用状态
    if(account->getReadOnly()){
        ui->btnSave->setEnabled(false);
        return;
    }
    //应根据凭证集的状态和当前选择的科目的范围来决定保存按钮是否启用
    PzsState state;
    dbUtil->getPzsState(y,m,state);
    isCanSave = (state == Ps_AllVerified);
    ui->btnSave->setEnabled(!fsub && !ssub && isCanSave);
}

//生成表头数据
void ViewExtraDialog::genHeaderDatas()
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
 * @brief ViewExtraDialog::genDatas
 *  生成统计数据
 */
void ViewExtraDialog::genDatas(){

    if(dataModel)
        delete dataModel;
    dataModel = new QStandardItemModel;

    QList<QStandardItem*> items;
    ApStandardItem *item;
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
    QHash<int,Double> spreExa, spreDetExa; //期初余额
    QHash<int,MoneyDirection> spreExaDir, spreDetExaDir; //期初余额方向
    QHash<int,Double> scurJHpn, scurJDHpn, scurDHpn, scurDDHpn; //当期借贷发生额
    QHash<int,Double> sendExa, sendDetExa; //期末余额
    QHash<int,MoneyDirection> sendExaDir, sendDetExaDir; //期末余额方向


    //将期初总账科目各币种余额累加--生成总账科目本位币余额
    calSumByMt(preExaR,preExaDirR,spreExa,spreExaDir);
    //将期初明细科目各币种余额累加--生成明细科目本位币余额
    calSumByMt(preDetExaR,preDetExaDirR,spreDetExa,spreDetExaDir,2);

    //合计当期总账和明细科目借贷发生额（即将个科目下的各币种进行累加）
    QHashIterator<int,Double>* it;
    //借方主目
    it = new QHashIterator<int,Double>(curJHpnR);
    int id;
    while(it->hasNext()){
        it->next();
        id = it->key() / 10;
        scurJHpn[id] += it->value();
    }
    //借方子目
    it = new QHashIterator<int,Double>(curJDHpnR);
    while(it->hasNext()){
        it->next();
        id = it->key() / 10;
        scurJDHpn[id] += it->value();
    }
    //贷方主目
    it = new QHashIterator<int,Double>(curDHpnR);
    while(it->hasNext()){
        it->next();
        id = it->key() / 10;
        scurDHpn[id] += it->value();
    }
    //贷方子目
    it = new QHashIterator<int,Double>(curDDHpnR);
    while(it->hasNext()){
        it->next();
        id = it->key() / 10;
        scurDDHpn[id] += it->value();
    }

    //将本期余额中的总账科目各币种余额进行累加
    calSumByMt(endExaR,endExaDirR,sendExa,sendExaDir);
    //将本期余额中的明细科目各币种余额进行累加
    calSumByMt(endDetExaR,endDetExaDirR,sendDetExa,sendDetExaDir,2);

    //显示数据
    FirstSubject* fs; SecondSubject* ss;
    Double jsums;Double dsums;  //借、贷合计值
    if(ui->rdoJe->isChecked()){ //金额式
        //输出数据               
        for(int i = 0; i < ids.count(); ++i){
            if(sendExa.contains(ids.at(i))){
                //共8列
                fs = allFSubs.value(ids[i]);
                item = new ApStandardItem(fs->getCode()); //0 科目代码
                items<<item;
                item = new ApStandardItem(fs->getName()); //1 科目名称
                items<<item;
                item = new ApStandardItem(dirStr(spreExaDir.value(ids.at(i)))); //2 期初方向
                items<<item;
                item = new ApStandardItem(spreExa.value(ids.at(i))); //3 期初金额
                items<<item;
                v = scurJHpn.value(ids.at(i)); //4 本期借方发生
                jsums += v;
                item = new ApStandardItem(v);
                items<<item;
                v = scurDHpn.value(ids.at(i));//5 本期贷方发生
                dsums += v;
                item = new ApStandardItem(v);
                items<<item;
                item = new ApStandardItem(dirStr(sendExaDir.value(ids.at(i))));//6 期末方向
                items<<item;
                item = new ApStandardItem(sendExa.value(ids.at(i)));//7 期末余额
                items<<item;

                dataModel->appendRow(items);
                items.clear();//至此，主科目余额加载完毕

                //如果需要显示明细科目的余额及其本期发生额，则
                if(ui->chkIsDet->isChecked()){
                    QList<int> sids;
                    if(!ssub){
                        QHash<int,QString> ssNames; //子目id到名称的映射
                        foreach(SecondSubject* ssub,smg->getFstSubject(ids.at(i))->getChildSubs())
                            ssNames[ssub->getId()] = ssub->getName();
                        sids = ssNames.keys();
                        qSort(sids.begin(),sids.end());
                    }
                    else
                        sids<<ssub->getId();
                    for(int j = 0; j < sids.count(); ++j){
                        if(sendDetExa.contains(sids.at(j))){
                            ss = allSSubs.value(sids.at(j));
                            item = new ApStandardItem(fs->getCode() + ss->getCode());//0 科目代码
                            items.append(item);
                            item = new ApStandardItem(ss->getName());//1 科目名称
                            items.append(item);
                            item = new ApStandardItem(dirStr(spreDetExaDir.value(sids.at(j))));//2 期初方向
                            items.append(item);
                            v = spreDetExa.value(sids.at(j)); //3 期初金额
                            item = new ApStandardItem(v);
                            items<<item;
                            v = scurJDHpn.value(sids.at(j));//4 本期借方发生（所有币种类型的发生额合计）
                            item = new ApStandardItem(v);
                            items<<item;
                            v = scurDDHpn.value(sids.at(j)); //5 本期贷方发生
                            item = new ApStandardItem(v);
                            items<<item;
                            item = new ApStandardItem(dirStr(sendDetExaDir.value(sids.at(j))));//6 期末方向
                            items.append(item);
                            v = sendDetExa.value(sids.at(j));//7 期末余额
                            item = new ApStandardItem(v);
                            items<<item;

                            dataModel->appendRow(items);
                            items.clear();
                        }
                    }
                }
            }
        }

        //加入合计行
        item = new ApStandardItem(tr("合  计"));
        items<<item<<NULL<<NULL<<NULL;
        item = new ApStandardItem(jsums);
        items<<item;
        item = new ApStandardItem(dsums);
        items<<item;
        dataModel->appendRow(items);
        items.clear();
    }
    else{//外币金额式（共12列）
        QHash<int,Double> curJSums,curDSums;   //本期借贷方合计值（按币种合计，key为币种代码）
        for(int i = 0; i < ids.count(); ++i){
            if(sendExa.contains(ids.at(i))){
                fs = allFSubs.value(ids.at(i));
                item = new ApStandardItem(fs->getCode()); //0 科目代码
                items<<item;
                item = new ApStandardItem(fs->getName());//1 科目名称
                items<<item;
                item = new ApStandardItem(dirStr(spreExaDir.value(ids.at(i))));//2 期初方向
                items<<(item);
                for(int k = 0; k<mts.count(); ++k){
                    item = new ApStandardItem(preExa.value(ids.at(i)*10+mts.at(k))); //3 期初金额（外币部分）
                    items<<item;
                }
                item = new ApStandardItem(spreExa.value(ids.at(i)));//4 期初金额（总额部分）
                items<<item;
                for(int k = 0; k<mts.count(); ++k){
                    v = curJHpn.value(ids.at(i)*10+mts.at(k)); //5 本期借方发生（外币部分）
                    curJSums[mts.value(k)] += v;
                    item = new ApStandardItem(v);
                    items<<item;
                }
                v = scurJHpn.value(ids[i]); //6 本期借方发生（各币种合计总额部分）
                jsums += v;
                item = new ApStandardItem(v);
                items.append(item);
                for(int k = 0; k<mts.count(); ++k){
                    v = curDHpn.value(ids.at(i)*10+mts.at(k));
                    curDSums[mts.at(k)] += v;
                    item = new ApStandardItem(v);//7 本期贷方发生（外币部分）
                    items<<item;
                }
                v = scurDHpn.value(ids.at(i));//8 本期贷方发生（各币种合计总额部分）
                dsums += v;
                item = new ApStandardItem(v);
                items.append(item);
                item = new ApStandardItem(dirStr(sendExaDir.value(ids.at(i))));//9 期末方向
                items.append(item);

                for(int k = 0; k<mts.count(); ++k){
                    v = endExa.value(ids.at(i)*10+mts.at(k));
                    item = new ApStandardItem(v);//10 期末余额（外币部分）
                    items<<item;
                }

                v = sendExa.value(ids.at(i)); //11 期末余额（总额部分）
                item = new ApStandardItem(v);
                items.append(item);

                dataModel->appendRow(items);
                items.clear();//至此，主科目余额加载完毕

                //如果需要显示明细科目的余额及其本期发生额，则
                if(ui->chkIsDet->isChecked()){
                    QList<int> sids;
                    if(!ssub){
                        QHash<int,QString> ssNames; //子目id到名称的映射
                        foreach(SecondSubject* ssub,smg->getFstSubject(ids.at(i))->getChildSubs())
                            ssNames[ssub->getId()] = ssub->getName();
                        sids = ssNames.keys();
                        qSort(sids.begin(),sids.end());
                    }
                    else
                        sids<<ssub->getId();
                    for(int j = 0; j < sids.count(); ++j){
                        if(sendDetExa.contains(sids.at(j))){
                            ss = allSSubs.value(sids.at(j));
                            item = new ApStandardItem(fs->getCode()+ss->getCode());//0 科目代码
                            items<<item;
                            item = new ApStandardItem(ss->getName());//1 科目名称
                            items<<item;
                            item = new ApStandardItem(dirStr(spreDetExaDir.value(sids.at(j))));//2 期初方向
                            items<<item;
                            for(int k = 0; k<mts.count(); ++k){
                                v = preDetExa.value(sids.at(j)*10+mts.at(k)); //3 期初金额（外币部分）
                                item = new ApStandardItem(v);
                                items<<item;
                            }
                            v = spreDetExa.value(sids.at(j));//4 期初金额（总额部分）
                            item = new ApStandardItem(v);
                            items.append(item);
                            for(int k = 0; k<mts.count(); ++k){
                                v = curJDHpn.value(sids.at(j)*10+mts.at(k));
                                item = new ApStandardItem(v);//5 本期借方发生（外币部分）
                                items<<item;
                            }
                            v = scurJDHpn.value(sids[j]);//6 本期借方发生（总额部分）
                                item = new ApStandardItem(v);
                            items.append(item);
                            for(int k = 0; k<mts.count(); ++k){
                                v = curDDHpn.value(sids.at(j)*10+mts.at(k));
                                item = new ApStandardItem(v);//7 本期贷方发生（外币部分）
                                items<<item;
                            }
                            v = scurDDHpn.value(sids[j]);//8 本期贷方发生（总额部分）
                            item = new ApStandardItem(v);
                            items.append(item);
                            item = new ApStandardItem(dirStr(sendDetExaDir.value(sids[j])));//9 期末方向
                            items.append(item);
                            for(int k = 0; k<mts.count(); ++k){
                                v = endDetExa.value(sids.at(j)*10+mts.at(k));//10 期末余额（外币部分）
                                item = new ApStandardItem(v);
                                items<<item;
                            }
                            //11 期末余额（总额部分）
                            v = sendDetExa.value(sids.at(j));
                            item = new ApStandardItem(v);
                            items.append(item);

                            dataModel->appendRow(items);
                            items.clear();
                        }
                    }
                }
            }
        }
        //加入合计行
        item = new ApStandardItem(tr("合  计"));
        items<<item<<NULL<<NULL;
        for(int k = 0; k <= mts.count(); ++k) //期初金额不需合计
            items<<NULL;
        //借方合计值（外币部分）
        for(int k = 0; k < mts.count(); ++k){
            v = curJSums.value(mts.value(k));
            item = new ApStandardItem(v);
            items<<item;
        }
        //借方合计值（各币种合计）
        if(jsums != 0)
            item = new ApStandardItem(jsums);
        else
            item = NULL;
        items<<item;
        //贷方合计值（外币部分）
        for(int k = 0; k < mts.count(); ++k){
            v = curDSums.value(mts.value(k));
            item = new ApStandardItem(v);
            items<<item;
        }
        //贷方合计值（各币种合计）
        if(dsums != 0)
            item = new ApStandardItem(dsums);
        else
            item = NULL;
        items<<item;

        dataModel->appendRow(items);
        items.clear();
    }//外币金额式
}

//初始化显示表格所需的数据哈希表
void ViewExtraDialog::initHashs()
{
    allFSubs = smg->getAllFstSubHash();
    allSSubs = smg->getAllSndSubHash();
    allMts = account->getAllMoneys();

    account->getRates(y,m,sRates);
    sRates[RMB] = 1.00;
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
    eRates[RMB] = 1.00;

    if(m == 1){
        yy = y -1;
        mm = 12;
    }
    else{
        yy = y;
        mm = m-1;
    }

    //读取以原币计的期初余额（包括本币和外币的余额）
    BusiUtil::readExtraByMonth2(yy,mm,preExa,preExaDir,preDetExa,preDetExaDir); //读取期初数
    //dbUtil->readExtraForPm(yy,mm,preExa,preExaDir,preDetExa,preDetExaDir);
    //读取以本币计的期初余额（包括本币和外币的余额）
    BusiUtil::readExtraByMonth3(yy,mm,preExaR,preExaDirR,preDetExaR,preDetExaDirR); //读取期初数
    //dbUtil->readExtraForMm()
    //读取外币期初余额（这些余额都以本币计，是每月发生统计后的精确值，且仅包含外币部分）
    bool exist;
    QHash<int,Double> es,eds;
    BusiUtil::readExtraByMonth4(yy,mm,es,eds,exist);
    //用精确值代替直接从原币币转换来的外币值
    if(exist){
        QHashIterator<int,Double>* it = new QHashIterator<int,Double>(es);
        while(it->hasNext()){
            it->next();
            preExaR[it->key()] = it->value();
        }
        it = new QHashIterator<int,Double>(eds);
        while(it->hasNext()){
            it->next();
            preDetExaR[it->key()] = it->value();
        }
    }

    PzsState pzsState;
    dbUtil->getPzsState(y,m,pzsState);
    ui->lblPzsState->setText(pzsStates.value(pzsState));
    ui->lblPzsState->setToolTip(pzsStateDescs.value(pzsState));

    //计算以原币计的本期发生额
    int amount;
    if(!BusiUtil::calAmountByMonth2(y,m,curJHpn,curDHpn,curJDHpn,curDDHpn,isCanSave,amount)){
        qDebug() << "Don't get current happen cash amount!!";
        return;
    }
    //计算以本币计的本期发生额
    if(!BusiUtil::calAmountByMonth3(y,m,curJHpnR,curDHpnR,curJDHpnR,curDDHpnR,isCanSave,amount)){
        qDebug() << "Don't get current happen cash amount!!";
        return;
    }


    QString info = tr("本期共有%1张凭证参予了统计。").arg(amount);
    emit infomation(info);

    //计算以原币计本期余额
    BusiUtil::calCurExtraByMonth2(y,m,preExa,preDetExa,preExaDir,preDetExaDir,
                                 curJHpn,curJDHpn,curDHpn,curDDHpn,
                                 endExa,endDetExa,endExaDir,endDetExaDir);
    //计算以本币计的本期余额
    BusiUtil::calCurExtraByMonth3(y,m,preExaR,preDetExaR,preExaDir,preDetExaDir,
                                 curJHpnR,curJDHpnR,curDHpnR,curDDHpnR,
                                 endExaR,endDetExaR,endExaDirR,endDetExaDirR);

    //debug 输出应收-宁波佳利的前期余额、本期发生、期末余额及其方向
    QString title = "(old)YS-nbjl";
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

    //输出佳利的贷方数值
//    qDebug()<<QString("(old)YS-nbjl-D: rmb=%1, usd=%2, usdR=%3")
//              .arg(curDDHpn.value(961).toString()).arg(curDDHpn.value(962).toString())
//              .arg(curDDHpnR.value(962).toString());

}




////////////////////////PrintSelectDialog////////////////////////////////////
PrintSelectDialog::PrintSelectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrintSelectDialog)
{
    ui->setupUi(this);
}

PrintSelectDialog::~PrintSelectDialog()
{
    delete ui;
}

//添加一张要打印的凭证
void PrintSelectDialog::append(int num)
{
    pznSet.insert(num);
}

//设置要打印的凭证号集合
void PrintSelectDialog::setPzSet(QSet<int> pznSet)
{
    this->pznSet = pznSet;
    QString ps = VariousUtils::IntSetToStr(pznSet);
    //VariousUtils::IntSetToStr(pznSet, ps);

    ui->edtSel->setText(ps); //将集合解析为简写文本表示形式
//    if(pznSet.count() > 0){
//        QList<int> pzs = pznSet.toList();
//        qSort(pzs.begin(),pzs.end());
//        int prev = pzs[0],next = pzs[0];
//        QString s;
//        for(int i = 1; i < pzs.count(); ++i){
//            if((pzs[i] - next) == 1){
//                next = pzs[i];
//            }
//            else{
//                if(prev == next)
//                    s.append(QString::number(prev)).append(",");
//                else
//                    s.append(QString("%1-%2").arg(prev).arg(next)).append(",");
//                prev = next = pzs[i];
//            }
//        }
//        if(prev == next)
//            s.append(QString::number(prev));
//        else
//            s.append(QString("%1-%2").arg(prev).arg(pzs[pzs.count() - 1]));
//        //s.chop(1);
//        ui->edtSel->setText(s);
//    }

}

//设置当前的凭证号
void PrintSelectDialog::setCurPzn(int pzNum)
{
    if(pzNum != 0)
        ui->edtCur->setText(QString::number(pzNum));
    else{
        ui->rdoCur->setEnabled(false);
        ui->edtCur->setEnabled(false);
    }

}

//移除指定凭证
void PrintSelectDialog::remove(int num)
{
    pznSet.remove(num);
}

/**
 * @brief PrintSelectDialog::getPrintPzSet
 *  获取欲打印的凭证号集合
 * @param pznSet
 * @return 0：所有凭证，1：当前凭证，2：自选凭证
 */
int PrintSelectDialog::getPrintPzSet(QSet<int>& pznSet)
{
    if((ui->rdoCur->isChecked()) && (ui->edtCur->text() != "")){
        pznSet.insert(ui->edtCur->text().toInt());
        return 1;
    }
    else if(ui->rdoSel->isChecked()){
        VariousUtils::strToIntSet(ui->edtSel->text(), pznSet);

//        pznSet.clear();
//        //对打印范围的编辑框文本进行解析，生成凭证号集合
//        QStringList sels = ui->edtSel->text().split(",");
//        for(int i = 0; i < sels.count(); ++i){
//            if(sels[i].indexOf('-') == -1)
//                pznSet.insert(sels[i].toInt());
//            else{
//                QStringList ps = sels[i].split("-");
//                int start = ps[0].toInt();
//                int end = ps[1].toInt();
//                for(int j = start; j <= end; ++j)
//                    pznSet.insert(j);
//            }
//        }
        return 2;
    }
    else
        return 0;
}

//获取打印模式（返回值 1：输出地打印机，2:打印预览，3：输出到PDF）
int PrintSelectDialog::getPrintMode()
{
    if(ui->rdoPrint->isChecked())
        return 1;
    else if(ui->rdoPreview->isChecked())
        return 2;
    else
        return 3;
}



////////////////////////SetupBaseDialog2/////////////////////////////////////////
SetupBaseDialog2::SetupBaseDialog2(Account *account, QWidget *parent) :
    QDialog(parent),ui(new Ui::SetupBaseDialog2),account(account)
{
    ui->setupUi(this);
    isDirty = false;
    ui->btnSave->setEnabled(isDirty);
    curSortCol = -1;
    curOrder = Qt::AscendingOrder;
    ui->tvDetails->hideColumn(DetExtItemDelegate::INDEX);

    year = account->getBaseYear();
    month = account->getBaseMonth();
    smg = account->getSubjectManager(account->getStartSuite()->subSys);
    dbUtil = account->getDbUtil();
    initTable();
    initTree();
    ui->dateEdit->setDate(QDate(year,month,1));

    dbUtil->readExtraForPm(year,month,fExts,fExtDirs,sExts,sExtDirs);
    dbUtil->readExtraForMm(year,month,fRExts,sRExts);
    initDatas();
    refresh();
    installDataInspect();
}

SetupBaseDialog2::~SetupBaseDialog2()
{
    delete ui;
}

//由外部程序（通常是MDI子窗口实例来调用，以保存对话框内编辑的数据）
void SetupBaseDialog2::closeDlg()
{
    if(isDirty && (QMessageBox::Yes == QMessageBox::question(this,tr("确认消息"),
       tr("数据已改变，需要保存吗？"),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)))
        on_btnSave_clicked();
    close();
}

//初始化内部使用的Hash表，及其代理
void SetupBaseDialog2::initTable()
{
    if(!curAccount->getRates(year,month,rates))
        return;
    //获取账户所采用的所有外币
    foreach(Money* mt, account->getWaiMt()){
        QVariant v; v.setValue<Money*>(mt);
        ui->cmbMts->addItem(mt->name(),v);
    }
    ui->cmbMts->setCurrentIndex(0);


    //BusiUtil::getFstSubCls(fstClass);      //一级科目类别
    fstClass = smg->getFstSubClass();
    //BusiUtil::getAllSubFName(fstSubNames); //一级科目名
    //BusiUtil::getAllSubCode(fstSubCodes);  //一级科目代码
    foreach(FirstSubject* fsub, smg->getAllFstSubHash()){
        fstSubNames[fsub->getId()] = fsub->getName();
        fstSubCodes[fsub->getId()] = fsub->getCode();
    }

    //BusiUtil::getMTName(mts);              //获取币种代码
    foreach(Money* mt, account->getAllMoneys())
        mts[mt->code()] = mt->name();
    //BusiUtil::getSidToFid(sidTofids);      //获取二级科目id到一级科目id的反向映射表
    dirs[DIR_J] = tr("借");
    dirs[DIR_D] = tr("贷");
    dirs[DIR_P] = tr("平");

    dlgt = new DetExtItemDelegate(smg,this);
    ui->tvDetails->setItemDelegate(dlgt);
    connect(dlgt, SIGNAL(newMapping(int,int,int,int)),
            this,SLOT(newMapping(int,int,int,int)));
    connect(dlgt,SIGNAL(newSndSub(int,QString,int,int)),
            this,SLOT(newSndSub(int,QString,int,int)));

    hv = ui->tvDetails->horizontalHeader();
    hv->setSortIndicator(0,Qt::AscendingOrder);
    hv->setSortIndicatorShown(true);
    connect(hv,SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
            this,SLOT(sortIndicatorChanged(int,Qt::SortOrder)));
}

//初始化树部件
void SetupBaseDialog2::initTree()
{
    isInit = true;    
    QHash<int,QTreeWidgetItem*> sjtClsItems; //科目类别代码到对应树节点的映射


    //获取科目余额节点对应的树项目，并加入一级科目类别节点
    sjtItem = ui->twDir->topLevelItem(0);
    QHashIterator<int,QString>* i = new QHashIterator<int,QString>(fstClass);
    while(i->hasNext()){
        i->next();
        QStringList strs;
        strs.append(i->value());
        //QTreeWidgetItem* item = new QTreeWidgetItem(strs);
        QTreeWidgetItem* item = new QTreeWidgetItem(i->value().split(" "));
        sjtItem->addChild(item);
        sjtClsItems[i->key()] = item;
    }

    //将各个科目按照它们所属类别分别加入到对应分支中
    i = new QHashIterator<int,QString>(fstSubNames);
    while(i->hasNext()){
        i->next();
        QStringList subName;
        subName.append(i->value());
        QTreeWidgetItem* item = new QTreeWidgetItem(subName, FSTSUBTYPE);
        int clsId = fstSubCodes.value(i->key()).left(1).toInt();//科目的类别代码
        sjtClsItems.value(clsId)->addChild(item);
        item->setData(1,Qt::EditRole,i->key()); //在节点的第二列保存科目的id
        sjtNodes[i->key()] = item;  //保存对此科目节点的引用
    }
    isInit = false;
}

//当前树节点变更
void SetupBaseDialog2::on_twDir_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    if(!isInit){
        if(current->type() == FSTSUBTYPE){
            curSubId = current->data(1,Qt::EditRole).toInt(); //一级科目的id
            dlgt->setFid(curSubId);
            refresh();
            //QHash<int,QString> sidForFsub;
            //BusiUtil::getOwnerSub(curSubId, sidForFsub); //获取当前一级科目下的所有二级科目表
            //if(sDgt != NULL)
            //    delete sDgt;
            //sDgt = new iTosItemDelegate(sidForFsub, this); //创建二级科目代理显示对象
            //QString s = QString("select FSAgent.id,FSAgent.fid,FSAgent.sid,"
            //                    "FSAgent.subCode,FSAgent.FrequencyStat,SecSubjects.subName from "
            //                    "FSAgent join SecSubjects where (FSAgent.sid = SecSubjects.id) and "
            //                    "(fid = %1) order by FSAgent.subCode").arg(curSubId);
            //sndSubModel->setQuery(s);
            //sDgt = new RelationMappingDelegate(sndSubModel, 2, 0, 5);
            //sDgt->setCurFstId(curSubId);
            //connect(sDgt, SIGNAL(updateSndSub()),this,SLOT(updateSndSub()));

        }        
    }
}

//初始化数据模型（必须在初始化fExts和sExts后才能调用）
void SetupBaseDialog2::initDatas(int witch)
{
    QList<QStandardItem*> items;
    FstExtData* fItem;
    DetExtData* sItem;

    if(witch == 1){ //科目余额
        fDatas.clear();
        sDatas.clear();

        //首先处理一级科目余额值
        QHashIterator<int,Double>* it = new QHashIterator<int,Double>(fExts);
        while(it->hasNext()){
            it->next();
            int fid = it->key() / 10;
            if(!fDatas.contains(fid)){
                fDatas[fid] = QList<FstExtData*>();
            }
            //将余额值添加到列表中
            fItem = new FstExtData;
            fItem->mt = it->key() % 10;
            fItem->v = it->value();
            if(fRExts.contains(it->key()))
                fItem->rv = fRExts.value(it->key());
            fItem->dir = fExtDirs.value(it->key());
            fDatas[fid] << fItem;
        }
        //第二步处理二级科目余额值
        it = new QHashIterator<int,Double>(sExts);
        while(it->hasNext()){
            it->next();
            int id = it->key() / 10;
            //需要获取该二级科目id所属的一级科目的id值
            //int fid = sidTofids.value(id);
            int fid = smg->getSndSubject(id)->getParent()->getId();
            //FirstSubject* fsub = smg->getFstSubject(fid);
            if(!sDatas.contains(fid))
                sDatas[fid] = QList<DetExtData*>();
            sItem = new DetExtData;
            sItem->subId = id;
            sItem->mt = it->key() % 10;
            sItem->v = it->value();
            if(sRExts.contains(it->key()))
                sItem->rv = sRExts.value(it->key());
            sItem->dir = sExtDirs.value(it->key());
            sItem->tag = false;
            sDatas[fid] << sItem;
        }
    }
}

//显示一级科目余额
void SetupBaseDialog2::refreshFstExt()
{
    ui->tvMaster->clearContents();

    //显示一级科目余额
    int rows = fDatas.value(curSubId).count();
    ui->tvMaster->setRowCount(rows);
    FstExtData* fData;
    for(int i = 0; i < rows; ++i){
        fData = fDatas.value(curSubId)[i];
        BAMoneyTypeItem* mtItem = new BAMoneyTypeItem(fData->mt, &allMts);
        BAMoneyValueItem* vItem,*rvItem;
        if(fData->dir == DIR_J){
            vItem = new BAMoneyValueItem(1,fData->v);
            rvItem = new BAMoneyValueItem(1,fData->rv);
        }
        else{
            vItem = new BAMoneyValueItem(0,fData->v);
            rvItem = new BAMoneyValueItem(0,fData->rv);
        }
        DirItem* dItem = new DirItem(fData->dir);
        ui->tvMaster->setItem(i,0,mtItem);
        ui->tvMaster->setItem(i,1,vItem);
        ui->tvMaster->setItem(i,2,rvItem);
        ui->tvMaster->setItem(i,3,dItem);
    }
}

//刷新显示
void SetupBaseDialog2::refresh()
{
    refreshFstExt();

    //根据当前选定的一级科目，显示相关的余额值
    ui->tvDetails->clearContents();

    //显示明细科目余额
    installDataInspect(false);
    int rows = sDatas.value(curSubId).count();
    ui->tvDetails->setRowCount(rows);
    DetExtData* sData;
    for(int i = 0; i < rows; ++i){
        sData = sDatas.value(curSubId)[i];
        int subSys = curAccount->getCurSuite()->subSys;
        SubjectManager* smg = curAccount->getSubjectManager(subSys);
        BASndSubItem* sItem = new BASndSubItem(sData->subId,smg);
        BAMoneyTypeItem* mtItem = new BAMoneyTypeItem(sData->mt, &allMts);
        BAMoneyValueItem* vItem,*rvItem;
        if(sData->dir == DIR_J){
            vItem = new BAMoneyValueItem(1,sData->v);
            rvItem = new BAMoneyValueItem(1,sData->rv);
        }
        else{
            vItem = new BAMoneyValueItem(0,sData->v);
            rvItem = new BAMoneyValueItem(0,sData->rv);
        }
        DirItem* dItem = new DirItem(sData->dir);
        QTableWidgetItem* iItem = new QTableWidgetItem(QString::number(i));
        tagItem* tItem = new tagItem(sData->tag);
        tItem->setData(Qt::DisplayRole,sData->desc);
        ui->tvDetails->setItem(i,0,sItem);
        ui->tvDetails->setItem(i,1,mtItem);
        ui->tvDetails->setItem(i,2,vItem);
        ui->tvDetails->setItem(i,3,rvItem);
        ui->tvDetails->setItem(i,4,dItem);
        ui->tvDetails->setItem(i,5,iItem);
        ui->tvDetails->setItem(i,6,tItem);
    }
    if(curSortCol != -1)
        ui->tvDetails->model()->sort(curSortCol,curOrder);
    installDataInspect();
}

//新增明细科目余额
void SetupBaseDialog2::on_btnAdd_clicked()
{
    installDataInspect(false);
        int row = sDatas.value(curSubId).count();
        DetExtData* sdata = new DetExtData;
        sDatas[curSubId].append(sdata);
        sdata->subId = 0;
        sdata->v = 0.00;
        sdata->rv = 0.00;
        sdata->dir = MDIR_P;
        sdata->mt = RMB;
        sdata->tag = false;
        sdata->desc = "";

        ui->tvDetails->insertRow(row);
        int subSys = curAccount->getCurSuite()->subSys;
        SubjectManager* smg = curAccount->getSubjectManager(subSys);
        BASndSubItem* sItem = new BASndSubItem(sdata->subId ,smg);
        BAMoneyTypeItem* mtItem = new BAMoneyTypeItem(sdata->mt, &allMts);
        BAMoneyValueItem* vItem = new BAMoneyValueItem(sdata->dir,sdata->v);
        BAMoneyValueItem* rvItem = new BAMoneyValueItem(sdata->dir,sdata->v);
        DirItem* dItem = new DirItem(sdata->dir);
        QTableWidgetItem* iItem = new QTableWidgetItem(QString::number(sDatas[curSubId].count()-1));
        tagItem* tItem = new tagItem;
        ui->tvDetails->setItem(row,0,sItem);
        ui->tvDetails->setItem(row,1,mtItem);
        ui->tvDetails->setItem(row,2,vItem);
        ui->tvDetails->setItem(row,3,rvItem);
        ui->tvDetails->setItem(row,4,dItem);
        ui->tvDetails->setItem(row,5,iItem);
        ui->tvDetails->setItem(row,6,tItem);
        ui->tvDetails->edit(ui->tvDetails->model()->index(row,0));
        ui->tvDetails->scrollToItem(sItem);
        installDataInspect();
}

//删除明细科目余额
void SetupBaseDialog2::on_btnDel_clicked()
{
//    if(ui->tvDetails->currentIndex().isValid()){
//        int row = ui->tvDetails->currentRow();
//        ui->tvDetails->removeRow(row);
//        sDatas[curSubId].removeAt(row);
//        reCalFstExt();
//        isDirty = true;
//        ui->btnSave->setEnabled(isDirty);
//    }
    QList<int> rows;
    QItemSelectionModel* model = ui->tvDetails->selectionModel();
    for(int i = 0; i < ui->tvDetails->rowCount(); ++i)
        if(model->isRowSelected(i,QModelIndex()))
            rows<<i;
    if(!rows.empty()){
        for(int i = rows.count()-1; i > -1; i--){
            ui->tvDetails->removeRow(rows[i]);
            sDatas[curSubId].removeAt(rows[i]);
        }
        reCalFstExt();
        isDirty = true;
        ui->btnSave->setEnabled(true);
    }
}

void SetupBaseDialog2::cellChanged(int row, int column)
{
    MoneyDirection dir;
    BAMoneyValueItem* item;
    bool req = false; //是否需要刷新显示一级科目的余额值
    int index = ui->tvDetails->item(row,DetExtItemDelegate::INDEX)
            ->data(Qt::EditRole).toInt();//获取该余额条目在列表中的索引位置
    DetExtData* data = sDatas.value(curSubId)[index];

    switch(column){
    case DetExtItemDelegate::SUB:
        data->subId = ui->tvDetails->item(row,column)->data(Qt::EditRole).toInt();
        break;
    case DetExtItemDelegate::MT:
        data->mt = ui->tvDetails->item(row,column)->data(Qt::EditRole).toInt();
        req = true;
        break;
    case DetExtItemDelegate::MV:
        data->v = ui->tvDetails->item(row,column)->data(Qt::EditRole).toDouble();
        if(data->v == 0.00){ //如果值为0，则设置方向为平
            data->dir = MDIR_P;
            QAbstractItemModel* m = ui->tvDetails->model();
            m->setData(m->index(row,DetExtItemDelegate::DIR),DIR_P);
        }
        //如果改变的是外币余额，且还没有输入与此外币余额对应的人民币余额，
        //则自动利用原币和期初汇率转换过来的本币值
        if(data->mt != RMB && data->rv == 0){
            data->rv = data->v * rates.value(data->mt);
            QAbstractItemModel* m = ui->tvDetails->model();
            m->setData(m->index(row,DetExtItemDelegate::RV),data->rv.getv());
        }
        req = true;
        break;
    case DetExtItemDelegate::RV:
        data->rv = ui->tvDetails->item(row,column)->data(Qt::EditRole).toDouble();
        req = true;
        break;
    case DetExtItemDelegate::DIR:
        dir = (MoneyDirection)ui->tvDetails->item(row,column)->data(Qt::EditRole).toInt();
        data->dir = dir;
        item = (BAMoneyValueItem*)ui->tvDetails->item(row,2);
        item->setDir(dir);
        ui->tvDetails->setItem(row,2,item);
        req = true;
        break;
    case DetExtItemDelegate::TAG:
        data->tag = ui->tvDetails->item(row,column)->data(Qt::EditRole).toBool();
        data->desc = ui->tvDetails->item(row,column)->data(Qt::DisplayRole).toString();
        break;
    }

    isDirty = true;
    if(req)
        reCalFstExt();
    ui->btnSave->setEnabled(isDirty);
}

/**
 * @brief SetupBaseDialog2::newMapping
 *  创建新的二级科目
 * @param fid   一级科目id
 * @param sid
 * @param row
 * @param col
 */
void SetupBaseDialog2::newMapping(int fid, int nid, int row, int col)
{
    QString name,lname;
    name = smg->getNameItem(nid)->getShortName();
    lname = smg->getNameItem(nid)->getLongName();
    //if(!BusiUtil::getSNameForId(nid,name,lname))
    //    return;
    //int subSys = curAccount->getCurSuite()->subSys;
    //SubjectManager* sm = curAccount->getSubjectManager(subSys);
    QString s = tr("在当前一级科目%1下创建新的二级科目%2")
            .arg(smg->getFstSubject(fid)->getName()).arg(name);
    if(QMessageBox::Yes == QMessageBox::question(this,tr("确认消息"),s,
                  QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)){
        FirstSubject* fsub = smg->getFstSubject(fid);
        SubjectNameItem* ni = smg->getNameItem(nid);
        SecondSubject* ssub = smg->addSndSubject(fsub,ni);
        ui->tvDetails->item(row,col)->setData(Qt::EditRole, ssub->getId());
        ui->tvDetails->edit(ui->tvDetails->model()->index(row,col+1));//将输入焦点移到右边栏
    }
    else
        ui->tvDetails->edit(ui->tvDetails->model()->index(row,col));//将输入焦点保留在此

}

/**
 * @brief SetupBaseDialog2::newSndSub
 *  创建新的名称条目，并用此名称在指定一级科目下创建二级科目
 * @param fid
 * @param name
 * @param row
 * @param col
 */
void SetupBaseDialog2::newSndSub(int fid, QString name, int row, int col)
{
    QString s = tr("在当前一级科目%1下创建新的二级科目%2")
            .arg(smg->getFstSubject(fid)->getName()).arg(name);
    if(QMessageBox::Yes == QMessageBox::question(this,tr("确认消息"),s,
                  QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)){
        CompletSubInfoDialog* dlg = new CompletSubInfoDialog(fid,smg,this);
        dlg->setName(name);
        if(dlg->exec() == QDialog::Accepted){
            QString sname = dlg->getSName();
            QString lname = dlg->getLName();
            QString remCode = dlg->getRemCode();
            int clsCode = dlg->getSubCalss();
            SubjectNameItem* ni = smg->addNameItem(sname,lname,remCode,clsCode);
            SecondSubject* ssub = smg->addSndSubject(smg->getFstSubject(fid),ni);
            ui->tvDetails->item(row,col)->setData(Qt::EditRole, ssub->getId());
            ui->tvDetails->edit(ui->tvDetails->model()->index(row,col+1));//将输入焦点移到右边栏
        }
    }
    else
        ui->tvDetails->edit(ui->tvDetails->model()->index(row,col));//将输入焦点保留在此
}

//打开明细科目余额表中的下一个项目的编辑器
void SetupBaseDialog2::editNext(int row, int col)
{
    //int rows = sDatas.value(curSubId).count();
    //if((row < rows))

    if(col < 4)
        ui->tvDetails->edit(ui->tvDetails->model()->index(row,col+1));
}

//排序的字段和方向改变了
void SetupBaseDialog2::sortIndicatorChanged(int logicalIndex, Qt::SortOrder order)
{
    curSortCol = logicalIndex;
    curOrder = order;
    ui->tvDetails->model()->sort(logicalIndex,order);
}

//安装/下载明细科目余额表数据改变监视器
void SetupBaseDialog2::installDataInspect(bool inst)
{
    if(inst)
        connect(ui->tvDetails,SIGNAL(cellChanged(int,int)),
                this,SLOT(cellChanged(int,int)));
    else
        disconnect(ui->tvDetails,SIGNAL(cellChanged(int,int)),
                   this,SLOT(cellChanged(int,int)));
}

//重新合计一级科目的余额值
void SetupBaseDialog2::reCalFstExt()
{
    //按币种和借贷方向计算合计值（将借方和贷方分别进行累计，然后按借贷的差值决定方向）
    QHash<int,Double> jsums,dsums; //分别是某个币种借方和贷方的合计值（键为币种代码）
    QHash<int,Double> jrsums,drsums; //以人民币计的借贷方合计值
    QHash<int,MoneyDirection> dirs;  //借贷汇总后新的一级科目余额的方向（键为币种代码）
    int mt,dir;
    Double v,rv;

    QList<DetExtData*>* data = &sDatas[curSubId];
    for(int i = 0; i < data->count(); ++i){
        if((*data)[i]->subId == 0)  //跳过没有指定有效明细科目id的余额条目
            continue;
        mt = (*data)[i]->mt;
        v = (*data)[i]->v;
        rv = (*data)[i]->rv;
        dir = (*data)[i]->dir;
        if(dir == DIR_J){
            jsums[mt] += v;
            jrsums[mt] += rv;
            dsums[mt] += 0.00;  //这是为了确保jsums和dsums具有相同的键集
            drsums[mt] += 0.00;
        }
        else if(dir == DIR_D){
            dsums[mt] += v;
            drsums[mt] += rv;
            jsums[mt] += 0.00;
            jrsums[mt] += 0.00;
        }
        else{  //平
            dsums[mt] += 0.00;
            drsums[mt] += 0.00;
            jsums[mt] += 0.00;
            jrsums[mt] += 0.00;
        }
    }
    QHashIterator<int,Double> it(jsums);
    fDatas[curSubId].clear();
    while(it.hasNext()){
        it.next();
        v = jsums.value(it.key()) - dsums.value(it.key());
        rv = jrsums.value(it.key()) - drsums.value(it.key());
        if(v == 0)
            dirs[it.key()] = MDIR_P;
        else if(v > 0)
            dirs[it.key()] = MDIR_J;
        else{
            dirs[it.key()] = MDIR_D;
            v.changeSign();           //取其绝对值
            rv.changeSign();
        }
        FstExtData* data = new FstExtData;
        data->dir = dirs.value(it.key());
        data->mt = it.key();
        data->v = v;
        data->rv = rv;
        fDatas[curSubId].append(data);
    }

    refreshFstExt();//在主表中更新显示
}

//确定按钮
void SetupBaseDialog2::on_btnOk_clicked()
{
    if(isDirty)
        on_btnSave_clicked();
    emit dialogClosed(MainWindow::SETUPBASE);
    //close();
}

//取消按钮
void SetupBaseDialog2::on_btnCancel_clicked()
{
    emit dialogClosed(MainWindow::SETUPBASE);
}

//选择一个外币，显示与该外币对应的汇率
void SetupBaseDialog2::on_cmbMts_currentIndexChanged(int index)
{
    Money* mt = ui->cmbMts->itemData(index).value<Money*>();
    ui->edtRate->setText(rates.value(mt->code()).toString());
}


//保存余额值到数据库中
void SetupBaseDialog2::on_btnSave_clicked()
{
    //构造新的余额值和方向集
    QHash<int,Double> fSums,sSums;   //总科目和明细科目值集（原币计）
    QHash<int,Double> fRSums,sRSums; //总科目和明细科目值集（本币计）
    QHash<int,MoneyDirection> fDirs,sDirs;      //总科目和明细科目方向

    int key;

    //总账科目值集
    FstExtData* fdata;
    QHashIterator<int,QList<FstExtData*> > it(fDatas);
    while(it.hasNext()){
        it.next();
        for(int i = 0; i < it.value().count(); ++i){
            fdata = it.value()[i];
            key = it.key() *10 + fdata->mt;
            fSums[key] = fdata->v;
            fRSums[key] = fdata->rv;
            fDirs[key] = fdata->dir;
        }
    }

    //明细科目值集
    DetExtData* sdata;
    QHashIterator<int,QList<DetExtData*> > ii(sDatas);
    while(ii.hasNext()){
        ii.next();
        for(int i = 0; i < ii.value().count(); ++i){
            sdata = ii.value()[i];
            key = sdata->subId * 10 + sdata->mt;
            sSums[key] = sdata->v;
            sRSums[key] = sdata->rv;
            sDirs[key] = sdata->dir;
        }
    }

    //保存
    //BusiUtil::savePeriodBeginValues2(year, month, fSums, fDirs,
    //                                 sSums, sDirs); //保存原币期初余额
    //BusiUtil::savePeriodEndValues(year,month,fRSums,sRSums);   //保存本币期初余额
    dbUtil->saveExtraForPm(year,month,fSums,fDirs,sSums,sDirs);
    dbUtil->saveExtraForMm(year,month,fRSums,sRSums);
    isDirty = false;
    ui->btnSave->setEnabled(isDirty);
}


//////////////////////////LoginDialog///////////////////////////
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    //setLayout(ui->mLayout);
    //this->witch = witch;
    init();
}

void LoginDialog::init()
{
    //if(witch == 1){
        QHashIterator<int,User*> it(allUsers);
        while(it.hasNext()){
            it.next();
            ui->cmbUsers->addItem(it.value()->getName(),it.value()->getUserId());
        }
    //}
//    else if(witch == 2){
//        ui->btnLogin->setText(tr("登出"));
//        ui->cmbUsers->setEnabled(false);
//    }

}

LoginDialog::~LoginDialog()
{
    delete ui;
}

User* LoginDialog::getLoginUser()
{
    int userId = ui->cmbUsers->itemData(ui->cmbUsers->currentIndex()).toInt();
    return allUsers.value(userId);
}

//登录
void LoginDialog::on_btnLogin_clicked()
{

    int userId =ui->cmbUsers->itemData(ui->cmbUsers->currentIndex()).toInt();
    if(allUsers.value(userId)->verifyPw(ui->edtPw->text()))
        accept();
    else
        QMessageBox::warning(this, tr("警告信息"), tr("密码不正确"));
}

//取消登录
void LoginDialog::on_btnCancel_clicked()
{
    QDialog::reject();
}

///////////////////////////SecConDialog/////////////////////////////////////
SecConDialog::SecConDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SecConDialog)
{
    ui->setupUi(this);
    setLayout(ui->mLayout);
    ui->tabRight->setLayout(ui->trLayout);
    ui->tabGroup->setLayout(ui->tgLayout);
    ui->tabUser->setLayout(ui->tuLayout);
    ui->tabOperate->setLayout(ui->toLayout);

    //初始化数据修改标记
    rightDirty = false;
    groupDirty = false;
    userDirty = false;
    operDirty = false;

    //添加权限表的上下文菜单
    ui->tvRight->addAction(ui->actAddRight);
    ui->tvRight->addAction(ui->actDelRight);

    //添加修改组权限的上下文菜单
    ui->trwGroup->addAction(ui->actChgGrpRgt);

    //添加修改用户所属组的上下文菜单
    ui->lwOwner->addAction(ui->actChgUserOwner);

    //添加修改操作所需权限的上下文菜单
    ui->trwOper->addAction(ui->actChgOpeRgt);

    init();
}

//初始化
void SecConDialog::init()
{
    QTableWidgetItem* item;
    ValidableTableWidgetItem* vitem;

    QList<int> codes;
    vat = new QIntValidator(1, 1000, this);


    //装载权限
    codes = allRights.keys();
    qSort(codes.begin(), codes.end());
    ui->tvRight->setRowCount(codes.count());
    ui->tvRight->setColumnCount(4);
    QStringList headTitles;
    headTitles << tr("权限代码") << tr("权限类别") << tr("权限名称") << tr("权限简介");
    ui->tvRight->setHorizontalHeaderLabels(headTitles);
    ui->tvRight->setColumnWidth(0, 80);
    ui->tvRight->setColumnWidth(1, 80);
    ui->tvRight->setColumnWidth(2, 150);
    //int w = ui->tvRight->width();
    ui->tvRight->setColumnWidth(3, 500);

    for(int r = 0; r < codes.count(); ++r){
        Right* right = allRights.value(codes[r]);        
        item = new ValidableTableWidgetItem(QString::number(right->getCode()), vat);
        ui->tvRight->setItem(r, 0, item);        
        item = new QTableWidgetItem;
        item->setData(Qt::EditRole, right->getType());
        ui->tvRight->setItem(r, 1, item);
        item = new QTableWidgetItem(right->getName());
        ui->tvRight->setItem(r, 2, item);
        item = new QTableWidgetItem(right->getExplain());
        ui->tvRight->setItem(r, 3, item);
    }

    iTosItemDelegate* rightTypeDele = new iTosItemDelegate(allRightTypes, this);
    ui->tvRight->setItemDelegateForColumn(1, rightTypeDele);

    connect(ui->tvRight, SIGNAL(cellChanged(int,int)),
            this, SLOT(onRightellChanged(int,int)));

    //装载用户组
    QListWidgetItem* litem;
    QHashIterator<int,UserGroup*> it(allGroups);
    while(it.hasNext()){
        it.next();
        litem = new QListWidgetItem(it.value()->getName());
        litem->setData(Qt::UserRole, it.key());
        ui->lwGroup->addItem(litem);
    }


    //装载用户

    //装载操作


    //装载权限类型
    initRightTypes(0);
}

//初始化用户组和操作的（第一层次）权限类型
void SecConDialog::initRightTypes(int pcode, QTreeWidgetItem* pitem)
{
    QString s;
    QSqlQuery q(bdb);
    s = QString("select code,name from rightType where pcode = %1").arg(pcode);
    bool r = q.exec(s);
    QTreeWidgetItem* item;
    if(pcode == 0){  //第一层次权限类型
        QList<QTreeWidgetItem*> items;
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            item = new QTreeWidgetItem(ui->trwGroup, QStringList(name));
            item->setData(0,Qt::UserRole,code);
            //items.append(item);
        }
        //ui->trwGroup->addTopLevelItems(items);
        for(int i = 0; i < ui->trwGroup->topLevelItemCount(); ++i){
            item = ui->trwGroup->topLevelItem(i);
            int code = item->data(0, Qt::UserRole).toInt();
            initRightTypes(code, item);
        }
    }
    else{
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            item = new QTreeWidgetItem(pitem, QStringList(name));
            item->setData(0,Qt::UserRole,code);
            //pitem->addChild(item);
            initRightTypes(code,item);
        }
    }


    //将组权限加入到对应的权限类别分支中
    item = ui->trwGroup->topLevelItem(0);
    //int i = 0;
    QTreeWidgetItem* ritem;
//    QHashIterator<int,Right*> it(allRights);
//    while(it.hasNext()){
//        it.next();
//        ritem = new QTreeWidgetItem(QStringList(it.value()->getName()));
//        //ritem = new QTreeWidgetItem;                            //QTreeWidgetItem::UserType); //此树项目表示权限而不是权限类别
//        //ritem->setText(0, it.value()->getName());
//        ritem->setCheckState(0,Qt::Checked);
//        //item = findItem(ui->trwGroup, it.value()->getType());
//        QString rname = it.value()->getName();
//        QString rtname = allRightTypes.value(it.value()->getType());
//        //QList<QTreeWidgetItem*> items = ui->trwGroup->findItems(rtname,Qt::MatchFixedString);
////        if(items.count() == 1){
////            items[0]->addChild(ritem);

////        }
//        item->addChild(ritem);
//        i++;
//    }

//    QList<QTreeWidgetItem *> items;
//    for(int i = 0; i<10; ++i){
//        ritem = new QTreeWidgetItem(item, QStringList(QString("Item-%1").arg(i)));
        //items.append(ritem);
//    }
//    item->addChildren(items);
//    int j = 0;
}

SecConDialog::~SecConDialog()
{
    delete ui;
}

//在树中查找具有指定用户数据的项目（只考虑第一次匹配的项目）
QTreeWidgetItem* SecConDialog::findItem(QTreeWidget* tree, int code, QTreeWidgetItem* startItem)
{
    QTreeWidgetItem* item;
    int count;
    int i = 0;
    if(startItem == NULL){  //从顶级项目开始查找
        count = tree->topLevelItemCount();
        while(i < count){
            item = tree->topLevelItem(i);
            if(item->data(0, Qt::UserRole).toInt() == code)
                return item;
            if(item->childCount() > 0)
                item = findItem(tree, code, item);
            if(item == NULL)
                i++;
            else
                return item;
        }
    }
    else{  //从指定项目开始查找
        count = startItem->childCount();
        while(i < count){
            item = startItem->child(i);
            if(item->data(0, Qt::UserRole).toInt() == code)
                return item;
            if(item->childCount() > 0)
                item = findItem(tree, code, item);
            if(item == NULL)
                i++;
            else
                return item;
        }
    }
    return NULL;
}

//添加权限
void SecConDialog::on_actAddRight_triggered()
{
    //自动赋一个未使用的权限代码
    int rows = ui->tvRight->rowCount();
    ui->tvRight->setRowCount(++rows);
    //因为在初始化显示权限表前已经根据权限代码进行了排序，因此，表的最后一行即是最大的代码数字
    int code = ui->tvRight->item(rows -2, 0)->data(Qt::DisplayRole).toInt();
    ValidableTableWidgetItem* vitem = new ValidableTableWidgetItem(QString::number(++code), vat);
    ui->tvRight->setItem(rows - 1, 0, vitem);
    ui->tvRight->setItem(rows - 1, 1, new QTableWidgetItem());
    ui->tvRight->setItem(rows - 1, 2, new QTableWidgetItem());
    ui->tvRight->setCurrentCell(rows - 1, 1);
    rightDirty = true;
}

//删除权限
void SecConDialog::on_actDelRight_triggered()
{
    //首先获取选择的行
    QList<int> row;
    QList<QTableWidgetSelectionRange> ranges = ui->tvRight->selectedRanges();
    foreach(QTableWidgetSelectionRange range, ranges){
        for(int i = 0; i < range.rowCount(); ++i){
            row.append(range.topRow() + i);
        }
    }
    if(row.count() > 0){
        qSort(row.begin(), row.end());
        for(int i = row.count() - 1; i > -1; i--){
            int r = row[i];
            ui->tvRight->removeRow(r);
        }
        rightDirty = true;
    }


}

//保存
void SecConDialog::on_btnSave_clicked()
{
    if(rightDirty){
        saveRights();
        rightDirty = false;
    }
    else if(groupDirty){
        saveGroups();
        groupDirty = false;
    }
    else if(userDirty){
        saveUsers();
        userDirty = false;
    }
    else if(operDirty){
        saveOperates();
        operDirty = false;
    }

    //还要考虑保持同4个全局变量的同步（allRights,allGroups,allUsers,allOperates）
}

//保存权限
void SecConDialog::saveRights()
{

    QSqlQuery q(bdb);
    QString s;
    bool rt;
    bool typeChanged = false; //权限类别改变了
    bool nameChanged = false; //名称改变标记
    bool explChanged = false; //权限简介改变标记
    QList<int> prevCodes = allRights.keys();

    //遍历权限表格
    for(int r = 0; r < ui->tvRight->rowCount(); ++r){
        int code = ui->tvRight->item(r, 0)->data(Qt::DisplayRole).toInt();
        int type = ui->tvRight->item(r, 1)->data(Qt::EditRole).toInt();
        QString name = ui->tvRight->item(r, 2)->text();
        QString explain = ui->tvRight->item(r, 3)->text();
        if(!allRights.contains(code)){ //如果在全局权限中没有对应项，则应新建
            Right* right = new Right(code, type, name, explain);
            allRights[code] = right;
            //在基础数据库中添加新记录
            s = QString("insert into rights(code,type,name,explain) values(%1,%2,'%3','%4')")
                    .arg(code).arg(type).arg(name).arg(explain);
            rt = q.exec(s);
        }
        else if(type != allRights.value(code)->getType()){
            allRights.value(code)->setType(type);
            typeChanged = true;
        }
        else if((name != allRights.value(code)->getName())){
            allRights.value(code)->setName(name);
            nameChanged = true;
        }
        else if(explain != allRights.value(code)->getExplain()){
            allRights.value(code)->setExplain(explain);
            explChanged = true;
        }
        //更新名称和简介内容
        if(typeChanged || nameChanged || explChanged){
            s = QString("update rights set ");
            if(typeChanged)
                s.append(QString("type = %1, ").arg(type));
            if(nameChanged)
                s.append(QString("name = '%1', ").arg(name));
            if(explChanged)
                s.append(QString("explain = '%1', ").arg(explain));
            s.chop(2);
            s.append(QString(" where code = %1").arg(code));
            rt = q.exec(s);
            nameChanged = false; explChanged = false;
            int j = 0;
        }
        prevCodes.removeOne(code); //移除已经处理过的项目
    }
    //删除遗留权限
    if(prevCodes.count() > 0){
        for(int i = 0; i < prevCodes.count(); ++i){
            allRights.remove(prevCodes[i]);
            //删除数据库中的对应记录
            s = QString("delete from rights where code = %1").arg(prevCodes[i]);
            rt = q.exec(s);
            int j = 0;
        }
    }

}

void SecConDialog::saveGroups()
{

}

void SecConDialog::saveUsers()
{

}

void SecConDialog::saveOperates(){

}


//关闭
void SecConDialog::on_btnClose_clicked()
{
    on_btnSave_clicked();
    close();
}

//修改组权限
void SecConDialog::on_actChgGrpRgt_triggered()
{

}

//修改用户所属组
void SecConDialog::on_actChgUserOwner_triggered()
{

}

//修改操作所需权限
void SecConDialog::on_actChgOpeRgt_triggered()
{

}


//当权限表的内容改变
void SecConDialog::onRightellChanged(int row, int column)
{
    if(column == 0){ //如果改变的是权限代码，要检测是否有同值冲突
        ValidableTableWidgetItem* item = (ValidableTableWidgetItem*)ui->tvRight->item(row, column);
        int code = item->text().toInt();
        //检测权限表中是否存在重复的代码
        for(int r = 0; r < ui->tvRight->rowCount(); ++r){
            if(r != row){ //排除本行
                if(code == ui->tvRight->item(r, 0)->data(Qt::DisplayRole).toInt()){
                     QMessageBox::warning(this, tr("提示信息"), tr("代码冲突或无效"));
                     item->setBackground(QBrush(Qt::red));
                     return;
                }
            }
        }
        item->setBackground(Qt::white);
    }
    rightDirty = true;

}

void SecConDialog::on_lwGroup_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    //如果先前的用户组权限有改变，则先保存

    //再设置新的当前用户组的权限
}


/////////////////////SearchDialog///////////////////////////////////////
SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog)
{
    ui->setupUi(this);
}

SearchDialog::~SearchDialog()
{
    delete ui;
}











