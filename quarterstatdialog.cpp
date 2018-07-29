#include "quarterstatdialog.h"
#include "ui_quarterstatdialog.h"

#include "statutil.h"
#include "HierarchicalHeaderView.h"
#include "config.h"
#include "subject.h"
//#include ""

QuarterStatDialog::QuarterStatDialog(StatUtil *statUtil, QByteArray* cinfo, QByteArray* pinfo, bool isQuarterStat, QWidget *parent)
    :DialogWithPrint(parent),ui(new Ui::QuarterStatDialog),statUtil(statUtil),isQuarterStat(isQuarterStat)
{
    ui->setupUi(this);
    account = statUtil->getAccount();
    smg = statUtil->getSubjectManager();
    suiteMgr = account->getSuiteMgr();
    setProperState(pinfo);
    init();

    headerModel = NULL;
    dataModel = NULL;

    //初始化表格行的背景色
    QString cssName = AppConfig::getInstance()->getAppStyleName();
    if(cssName == "navy"){
        row_bk_ssub = QColor(0xC8C8FF);
        row_bk_fsub = QColor(0x9696FF);
        row_bk_sum = QColor(0x6464FF);
    }else if(cssName == "pink"){
        row_bk_ssub = QColor(0xFDD8D8);
        row_bk_fsub = QColor(0xFEBEBE);
        row_bk_sum = QColor(0xF9A2A2);
    }
    else{
        row_bk_ssub = QColor(0xE8E8EA);
        row_bk_fsub = QColor(0xD9D9DC);
        row_bk_sum = QColor(0xC7C7CD);
    }

    //初始化自定义的层次式表头
    hv = new HierarchicalHeaderView(Qt::Horizontal, ui->tview);
    hv->setHighlightSections(true);
    QFont font = hv->font();
    font.setPixelSize(12);
    hv->setSectionsClickable(true);
    ui->tview->setHorizontalHeader(hv);
    setCommonState(cinfo);
    //stat();

}

QuarterStatDialog::~QuarterStatDialog()
{
    delete ui;
}

void QuarterStatDialog::print(PrintActionClass action)
{

}

/**
 * @brief QuarterStatDialog::onSelFstSub
 * 选择一级科目
 * @param index
 */
void QuarterStatDialog::onSelFstSub(int index)
{

}

/**
 * @brief QuarterStatDialog::onSelSndSub
 * 选择二级科目
 * @param index
 */
void QuarterStatDialog::onSelSndSub(int index)
{

}

void QuarterStatDialog::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{

}

void QuarterStatDialog::setCommonState(QByteArray *info)
{

}

void QuarterStatDialog::setProperState(QByteArray *info)
{

}

void QuarterStatDialog::init()
{
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
    if(fsub){
        ui->cmbSndSub->setEnabled(true);
        ui->cmbFstSub->setSubject(fsub);
        ui->cmbSndSub->setParentSubject(fsub);
        ui->cmbSndSub->insertItem(0,tr("所有"),0);
        if(ssub)
            ui->cmbSndSub->setSubject(ssub);
        else
            ui->cmbSndSub->setCurrentIndex(0);
    }
    else{
        ui->cmbSndSub->setEnabled(false);
        ui->cmbFstSub->setCurrentIndex(0);
        ui->cmbSndSub->addItem(tr("所有"),0);
    }
    ui->lbly->setText(QString::number(statUtil->year()));
    if(isQuarterStat){
        ui->lblMonth->setText(tr("季度"));
        //int endMonth = suiteMgr

    }
    else{
        ui->lblMonth->setText(tr("月份"));
    }
    ui->lblMonth->setText(QString::number(statUtil->month()));
    connect(ui->cmbFstSub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelFstSub(int)));
    connect(ui->cmbSndSub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int))
            ,this, SLOT(colWidthChanged(int,int,int)));
}

void QuarterStatDialog::initHashs()
{

}

void QuarterStatDialog::viewRates()
{

}

void QuarterStatDialog::viewTable()
{

}

void QuarterStatDialog::genHeaderDatas()
{

}

void QuarterStatDialog::genDatas()
{

}

void QuarterStatDialog::printCommon(PrintTask task, QPrinter *printer)
{

}

void QuarterStatDialog::setTableRowBackground(TableRowType rt, const QList<QStandardItem *> l)
{

}

void QuarterStatDialog::setTableRowTextColor(TableRowType rt, const QList<QStandardItem *> l)
{

}
