#include <QTreeView>
#include <QSqlQuery>
#include <QBuffer>

#include <QGraphicsWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QInputDialog>
#include <QDialogButtonBox>

#include "tables.h"
#include "dialog3.h"
#include "widgets.h"
#include "utils.h"
#include "global.h"
#include "pz.h"
#include "subject.h"
#include "dbutil.h"
#include "PzSet.h"


//tem
//#include "dialog2.h"



ImpOthModDialog::ImpOthModDialog(int witch, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImpOthModDialog)
{
    ui->setupUi(this);
    setLayout(ui->mLayout);

    isSkip = false;
    if(witch != 1)
        ui->btnSkip->setVisible(false);
}

ImpOthModDialog::~ImpOthModDialog()
{
    delete ui;
}

//单击跳过
void ImpOthModDialog::on_btnSkip_clicked()
{
    isSkip = true;
    accept();
}

bool ImpOthModDialog::isSelSkip()
{
    return isSkip;
}

bool ImpOthModDialog::isSelGdzc()
{
    return !isSkip && ui->chkGdzc->checkState();
}

bool ImpOthModDialog::isSelDtfy()
{
    return !isSkip && ui->chkDtfy->checkState();
}

//返回选择的模块
bool ImpOthModDialog::selModules(QSet<OtherModCode>& selMods)
{
    if(isSkip)
        return false; //false：代表未选择任何模块
    if(ui->chkGdzc->checkState())
        selMods.insert(OM_GDZC);
    if(ui->chkDtfy->checkState())
        selMods.insert(OM_DTFY);
    return !selMods.empty();
}

////////////////////////////AntiJzDialog/////////////////////////////////////////
//参数 reqs：键为要取消的凭证大类代码，值为是否已经进行了结转操作，如果还未执行结转，
//则不能执行取消操作，相应的选择框也将禁用。
//state：当前凭证集的状态
AntiJzDialog::AntiJzDialog(QHash<PzdClass,bool> haved, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AntiJzDialog)
{
    ui->setupUi(this);
    this->haved = haved;
    ui->chkJzhd->setEnabled(haved.value(Pzd_Jzhd));
    ui->chkJzsy->setEnabled(haved.value(Pzd_Jzsy));
    ui->chkJzlr->setEnabled(haved.value(Pzd_Jzlr));
}

AntiJzDialog::~AntiJzDialog()
{
    delete ui;
}

//获取用户的选择，如果未选，则返回false，否则返回true
//参数 reqs：键为要取消的凭证大类代码，值表示对应的选择框是否被选择了
QHash<PzdClass,bool> AntiJzDialog::selected()
{
    QHash<PzdClass,bool> sels;
    sels[Pzd_Jzhd] = ui->chkJzhd->isChecked();
    sels[Pzd_Jzsy] = ui->chkJzsy->isChecked();
    sels[Pzd_Jzlr] = ui->chkJzlr->isChecked();
    return sels;
}

//用户选择了取消结转本年利润，则要自动选择其他比它优先级低的选取框
void AntiJzDialog::on_chkJzlr_clicked(bool checked)
{
    int i = 0;
}

//用户选择了取消结转汇兑损益
void AntiJzDialog::on_chkJzhd_clicked(bool checked)
{
    if(checked){
        if(ui->chkJzsy->isEnabled()){
            ui->chkJzsy->setChecked(true);
            ui->chkJzsy->setEnabled(false);
        }
        if(ui->chkJzlr->isEnabled()){
            ui->chkJzlr->setChecked(true);
            ui->chkJzlr->setEnabled(false);
        }
    }
    else{
        ui->chkJzsy->setChecked(false);
        ui->chkJzsy->setEnabled(haved.value(Pzd_Jzsy));
        ui->chkJzlr->setChecked(false);
        ui->chkJzlr->setEnabled(haved.value(Pzd_Jzlr));
    }
}

//用户选择了取消结转损益
void AntiJzDialog::on_chkJzsy_clicked(bool checked)
{
    if(checked && ui->chkJzlr->isEnabled()){
        ui->chkJzlr->setChecked(true);
        ui->chkJzlr->setEnabled(false);
    }
    else{
        ui->chkJzlr->setChecked(false);
        ui->chkJzlr->setEnabled(haved.value(Pzd_Jzlr));
    }
}

/////////////////////////GdzcAdminDialog////////////////////////////////////////
GdzcAdminDialog::GdzcAdminDialog(Account* account, QByteArray *sinfo, QWidget *parent) :
    QDialog(parent),ui(new Ui::GdzcAdminDialog),account(account)
{
    ui->setupUi(this);
    smg = account->getSubjectManager();
    isCancel = false;    

    ui->btnJtzj->addAction(ui->actJtzj);
    ui->btnJtzj->addAction(ui->actPreview);
    ui->btnJtzj->addAction(ui->actViewList);
    ui->btnAddZj->addAction(ui->actSupply);
    ui->btnAddZj->addAction(ui->actNextZj);

    //ui->rdoPart->setChecked(true);
    dv.setDecimals(2);
    ui->edtPrime->setValidator(&dv);
    ui->edtMin->setValidator(&dv);

    initCmb();
    load(ui->rdoAll->isChecked());
    setState(sinfo);

    ui->twZjTable->setColumnWidth(ColDate,zjInfoColWidths[ColDate]);
    ui->twZjTable->setColumnWidth(ColValue,zjInfoColWidths[ColValue]);
    ui->twZjTable->setColumnWidth(ColPid,20);
    ui->twZjTable->setColumnWidth(ColBid,20);
    ui->twZjTable->setColumnWidth(ColId,20);
    ui->twZjTable->setColumnHidden(ColPid,true);
    ui->twZjTable->setColumnHidden(ColBid,true);
    ui->twZjTable->setColumnHidden(ColId,true);
    connect(ui->twZjTable->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(zjInfosColWidthChanged(int,int,int)));
}

GdzcAdminDialog::~GdzcAdminDialog()
{
    delete ui;
}

//初始化固定资产产品类别和科目类别选择列表，及其对应的子目映射表
void GdzcAdminDialog::initCmb()
{
    QString s;
    QSqlQuery q;
    bool r;

    //初始化固定资产产品类别列表
    QList<int> keys = allGdzcProductCls.keys();
    qSort(keys.begin(),keys.end());
    for(int i = 0; i < keys.count(); ++i){
        QString name = allGdzcProductCls.value(keys[i])->getName();
        int code = allGdzcProductCls.value(keys[i])->getCode();
        ui->cmbProCls->addItem(name,code);
    }

    //初始化固定资产科目类别列表
    QList<int> subIds = allGdzcSubjectCls.keys();
    qSort(subIds.begin(),subIds.end());
    for(int i = 0; i < subIds.count(); ++i)
        ui->cmbSubCls->addItem(allGdzcSubjectCls.value(subIds[i]),subIds[i]);

    //获取固定资产、累计折旧的科目id
    int gid,lid;
    //BusiUtil::getIdByCode(gid,"1501");
    //BusiUtil::getIdByCode(lid,"1502");
    gid = smg->getGdzcSub()->getId();
    lid = smg->getLjzjSub()->getId();
    //建立固定资产科目类别代码到相应子目id的映射
    for(int i = 0; i < subIds.count(); ++i){
        s = QString("select id from FSAgent where (fid=%1) and (sid=%2)")
                .arg(gid).arg(subIds[i]);
        r = q.exec(s);
        r = q.first();
        gdzcSubIDs[subIds[i]] = q.value(0).toInt();

        s = QString("select id from FSAgent where (fid=%1) and (sid=%2)")
                .arg(lid).arg(subIds[i]);
        r = q.exec(s);
        r = q.first();
        ljzjSubIDs[subIds[i]] = q.value(0).toInt();
    }
}

//装载所有固定资产及其类别
void GdzcAdminDialog::load(bool isAll)
{
    disconnect(ui->lvGdzc,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(curGzdcChanged(QListWidgetItem*,QListWidgetItem*)));

    //清除所有相关数据成员
    foreach(Gdzc* g,gdzcLst)
        delete g;
    gdzcLst.clear();
    isChanged.clear();
    ui->lvGdzc->clear();

    Gdzc::load(gdzcLst,isAll);  //装载所选的固定资产
    //刷新界面
    if(!gdzcLst.empty()){
        for(int i = 0; i < gdzcLst.count(); ++i){
            isChanged<<false;
            QListWidgetItem* item = new QListWidgetItem(gdzcLst[i]->getName());
            ui->lvGdzc->addItem(item);
        }
        ui->lvGdzc->setCurrentRow(0);
        viewInfo(0);
        curGdzc = gdzcLst[0];
        ui->btnDel->setEnabled(true);
    }
    else{
        curGdzc = NULL;
        viewInfo(-1);
        ui->btnDel->setEnabled(false);
    }

    connect(ui->lvGdzc,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(curGzdcChanged(QListWidgetItem*,QListWidgetItem*)));
}

//显示固定资产信息
void GdzcAdminDialog::viewInfo(int index)
{
    bindSingal(false);

    if(index == -1){ //没有任何固定资产选择，则清除界面
        ui->lvGdzc->clear();
        ui->edtName->setText("");
        ui->edtModel->setText("");
        ui->edtPrime->setText("");
        ui->edtRemain->setText("");
        ui->edtMin->setText("");
        ui->twZjTable->clearContents();
        ui->datBuy->setDate(curAccount->getStartDate());
        ui->spnZjMonth->setValue(0);
        ui->cmbProCls->setCurrentIndex(0);
        ui->cmbSubCls->setCurrentIndex(0);
        ui->txtOtherInfo->setPlainText("");
    }
    else{
        Gdzc* g = gdzcLst[index];
        int idx = ui->cmbProCls->findData(g->getProductClass()->getCode());
        ui->cmbProCls->setCurrentIndex(idx);
        idx = ui->cmbSubCls->findData(g->getSubClass());
        ui->cmbSubCls->setCurrentIndex(idx);
        ui->edtName->setText(g->getName());
        ui->edtModel->setText(g->getModel());
        ui->datBuy->setDate(g->getBuyDate());
        ui->edtPrime->setText(QString::number(g->getPrimeValue(),'f',2));
        ui->edtRemain->setText(QString::number(g->getRemainValue(),'f',2));
        ui->edtMin->setText(QString::number(g->getMinValue(),'f',2));
        if(g->getProductClass()->getZjMonth() == 0)
            ui->spnZjMonth->setEnabled(true);
        else
            ui->spnZjMonth->setEnabled(false);
        ui->rdoMonth->setChecked(true);
        ui->spnZjMonth->setValue(g->getZjMonths());
        //根据当前固定资产是否已开始折旧，启用和禁用相应的部件
        bool en = g->isStartZj();
        ui->btnDel->setEnabled(!en);
        ui->datBuy->setReadOnly(en);
        ui->cmbProCls->setEnabled(!en);
        ui->cmbSubCls->setEnabled(!en);
        ui->edtPrime->setReadOnly(en);
        ui->edtMin->setReadOnly(en);
        ui->rdoAll->setEnabled(!en);
        ui->rdoMonth->setEnabled(!en);
        ui->spnZjMonth->setReadOnly(en);
        ui->txtOtherInfo->setPlainText(g->getOtherInfo());
        viewZj(g);
    }
    bindSingal();
}

//显示固定资产折旧情况表
void GdzcAdminDialog::viewZj(Gdzc* g)
{
    disconnect(ui->twZjTable,SIGNAL(itemChanged(QTableWidgetItem*)),
            this, SLOT(zjItemInfoChanged(QTableWidgetItem*)));
    ui->twZjTable->clearContents();
    ui->twZjTable->setRowCount(0);
    QTableWidgetItem* item;
    int row = 0;
    foreach(Gdzc::GdzcZjInfoItem* info, g->getZjInfos()){
        //if(info->state == Gdzc::DELETED)
        //    continue;
        ui->twZjTable->insertRow(row);
        item = new QTableWidgetItem(info->date.toString(Qt::ISODate));
        item->setTextAlignment(Qt::AlignCenter);
        ui->twZjTable->setItem(row,ColDate,item);
        item = new QTableWidgetItem(QString::number(info->v,'f',2));
        item->setTextAlignment(Qt::AlignCenter);
        ui->twZjTable->setItem(row,ColValue,item);
        item = new QTableWidgetItem(QString::number(info->pid));
        ui->twZjTable->setItem(row,ColPid,item);
        item = new QTableWidgetItem(QString::number(info->bid));
        ui->twZjTable->setItem(row,ColBid,item);
        item = new QTableWidgetItem(QString::number(info->id));
        ui->twZjTable->setItem(row,ColId,item);
        row++;
    }
    connect(ui->twZjTable,SIGNAL(itemChanged(QTableWidgetItem*)),
            this, SLOT(zjItemInfoChanged(QTableWidgetItem*)));
}

//绑定编辑固定资产信息内容的改变事件
void GdzcAdminDialog::bindSingal(bool bind)
{
    if(bind){
        connect(ui->edtName,SIGNAL(textEdited(QString)),this,SLOT(nameChanged(QString)));
        connect(ui->cmbProCls,SIGNAL(currentIndexChanged(int)),
                this,SLOT(productClassChanged(int)));
        connect(ui->cmbSubCls,SIGNAL(currentIndexChanged(int)),
                this,SLOT(subjectClassChanged(int)));
        connect(ui->datBuy,SIGNAL(dateChanged(QDate)),this,SLOT(buyDateChanged(QDate)));
        connect(ui->edtModel,SIGNAL(textEdited(QString)),this,SLOT(modelChanged(QString)));
        connect(ui->edtPrime,SIGNAL(textEdited(QString)),this,SLOT(primeChanged(QString)));
        //connect(ui->edtRemain,SIGNAL(textChanged(QString)),this,SLOT(remainChanged(QString)));
        connect(ui->edtMin,SIGNAL(textEdited(QString)),this,SLOT(minPriceChanged(QString)));
        connect(ui->txtOtherInfo,SIGNAL(textChanged()),this,SLOT(otherInfoChanged()));
    }
    else{
        disconnect(ui->edtName,SIGNAL(textEdited(QString)),this,SLOT(nameChanged(QString)));
        disconnect(ui->cmbProCls,SIGNAL(currentIndexChanged(int)),
                this,SLOT(productClassChanged(int)));
        disconnect(ui->cmbSubCls,SIGNAL(currentIndexChanged(int)),
                this,SLOT(subjectClassChanged(int)));
        disconnect(ui->datBuy,SIGNAL(dateChanged(QDate)),this,SLOT(buyDateChanged(QDate)));
        disconnect(ui->edtModel,SIGNAL(textEdited(QString)),this,SLOT(modelChanged(QString)));
        disconnect(ui->edtPrime,SIGNAL(textEdited(QString)),this,SLOT(primeChanged(QString)));
        //disconnect(ui->edtRemain,SIGNAL(textChanged(QString)),this,SLOT(remainChanged(QString)));
        disconnect(ui->edtMin,SIGNAL(textEdited(QString)),this,SLOT(minPriceChanged(QString)));
        disconnect(ui->txtOtherInfo,SIGNAL(textChanged()),this,SLOT(otherInfoChanged()));
    }
}

//保存
void GdzcAdminDialog::save(bool isConfirm)
{
    if(isCancel)
        return;

    if((isChanged.contains(true) || !delLst.empty())){
        if(!isConfirm ||
           (isConfirm && (QMessageBox::Yes == QMessageBox::warning(this,tr("提示信息"),
             tr("当前固定资产内容已被修改且未保存，需要保存吗？"),
             QMessageBox::Yes|QMessageBox::No)))){
                for(int i = 0; i < isChanged.count(); ++i){
                    if(isChanged[i])
                        gdzcLst[i]->save();
                }
                if(!delLst.empty()){
                    for(int i = 0; i < delLst.count(); ++i){
                        delLst[i]->remove();
                        delete delLst[i];
                    }
                    delLst.clear();
                }
        }
    }    
}

QByteArray* GdzcAdminDialog::getState()
{
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 i8;
    qint16 i16;

    //显示固定资产折旧清单的窗口大小
    i16 = vlistWinRect.width();
    out<<i16;
    i16 = vlistWinRect.height();
    out<<i16;
    //plistColWidths打印固定资产折旧清单的表格列宽
    //vlistColWidths显示固定资产折旧清单的表格列宽
    i8 = 5;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = plistColWidths[i];
        out<<i16;
        i16 = vlistColWidths[i];
        out<<i16;
    }
    //zjInfoColWidths显示折旧信息的表格列宽
    i8 = 2;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = zjInfoColWidths[i];
        out<<i16;
    }
    //预览折旧凭证的窗口大小
    i16 = pzWinRect.width();
    out<<i16;
    i16 = pzWinRect.height();
    out<<i16;
    //pzColWidths预览折旧凭证的会计分录的表格列宽
    i8 = 6;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = pzColWidths[i];
        out<<i16;
    }

    bf.close();
    return info;
}

void GdzcAdminDialog::setState(QByteArray* info)
{
    if(info == NULL){
        plistColWidths<<300<<120<<120<<120<<120;
        vlistColWidths<<300<<120<<120<<120<<120;
        zjInfoColWidths<<200<<200;
        pzColWidths<<300<<150<<150<<150<<150<<150;
        vlistWinRect = QSize(850,400);
        pzWinRect = QSize(700,400);
    }
    else{
        QBuffer bf(info);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        qint8 i8;
        qint16 i16;

        //显示固定资产折旧清单的窗口大小
        in>>i16;
        vlistWinRect.setWidth(i16);
        in>>i16;
        vlistWinRect.setHeight(i16);
        //打印、查看固定资产折旧清单的表格列宽
        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            plistColWidths<<i16;
            in>>i16;
            vlistColWidths<<i16;
        }
        //显示折旧信息的表格列宽
        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            zjInfoColWidths<<i16;
        }
        //预览折旧凭证的窗口大小
        in>>i16;
        pzWinRect.setWidth(i16);
        in>>i16;
        pzWinRect.setHeight(i16);
        //预览折旧凭证的会计分录的表格列宽
        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            pzColWidths<<i16;
        }
        bf.close();
    }
}

//创建固定资产折旧信息，并添加到当前固定资产折旧信息列表和右边显示的折旧信息列表中
void GdzcAdminDialog::addZjInfo(QDate d,double v,int index)
{
    Gdzc::GdzcZjInfoItem* info = new Gdzc::GdzcZjInfoItem(d,v,Gdzc::NEW);
    if(!curGdzc->addZjInfo(info)){
        qDebug() << tr("在增加折旧信息条目时，出现日期冲突（%1）")
                    .arg(d.toString(Qt::ISODate));
        return;
    }

    QTableWidgetItem* item;
    ui->twZjTable->insertRow(index);
    item = new QTableWidgetItem(info->date.toString(Qt::ISODate));
    item->setTextAlignment(Qt::AlignCenter);
    ui->twZjTable->setItem(index,ColDate,item);
    item = new QTableWidgetItem(QString::number(info->v,'f',2));
    item->setTextAlignment(Qt::AlignCenter);
    ui->twZjTable->setItem(index,ColValue,item);
    item = new QTableWidgetItem(QString::number(info->pid));
    ui->twZjTable->setItem(index,ColPid,item);
    item = new QTableWidgetItem(QString::number(info->bid));
    ui->twZjTable->setItem(index,ColBid,item);
    item = new QTableWidgetItem(QString::number(info->id));
    ui->twZjTable->setItem(index,ColId,item);
}

//删除指定年月的折旧记录，重新计算各个固定资产的净值
//void GdzcAdminDialog::delZjInfos(int y, int m, int gid)
//{

//}

//选中一个固定资产
void GdzcAdminDialog::curGzdcChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    int index = current->listWidget()->currentRow();
    curGdzc = gdzcLst[index];
    viewInfo(index);
}

//固定资产名称改变
void GdzcAdminDialog::nameChanged(const QString &text)
{
    curGdzc->setName(text);
    QListWidgetItem* item = ui->lvGdzc->currentItem();
    item->setText(text);
    int idx = ui->lvGdzc->currentRow();
    isChanged[idx] = true;
}

//固定资产类型改变
void GdzcAdminDialog::productClassChanged(int index)
{
    GdzcType* t = allGdzcProductCls.value(ui->cmbProCls->itemData(index).toInt());
    curGdzc->setProductClass(t);
    isChanged[ui->lvGdzc->currentRow()] = true;
    //如果该类别没有指定折旧年限，则必须启用折旧月份输入部件
    if(t->getZjMonth() == 0)
        ui->spnZjMonth->setEnabled(true);
    else
        ui->spnZjMonth->setEnabled(false);
}

//固定资产的科目类别改变
void GdzcAdminDialog::subjectClassChanged(int index)
{
    int cls = ui->cmbSubCls->itemData(index).toInt();
    curGdzc->setSubClass(cls);
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//固定资产的购入日期发生改变
void GdzcAdminDialog::buyDateChanged(const QDate& date)
{
    curGdzc->setBuyDate(date);
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//固定资产厂牌型号改变
void GdzcAdminDialog::modelChanged(const QString &text)
{
    curGdzc->setModel(text);
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//固定资产原值改变
void GdzcAdminDialog::primeChanged(const QString &text)
{
    double v = text.toDouble();
    curGdzc->setPrimeValue(v);
    curGdzc->setRemainValue(v);
    ui->edtRemain->setText(text);
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//固定资产剩余值改变
//void GdzcAdminDialog::remainChanged(const QString &text)
//{
//    curGdzc->setRemainValue(text.toDouble());
//    isChanged[ui->lvGdzc->currentRow()] = true;
//}

//固定资产残值改变
void GdzcAdminDialog::minPriceChanged(const QString &text)
{
    curGdzc->setMinValue(text.toDouble());
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//固定资产折旧年限改变
void GdzcAdminDialog::zjMonthsChanged(int i)
{
    int months;
    if(ui->rdoMonth->isChecked())
        months = ui->spnZjMonth->value();
    else
        months = ui->spnZjMonth->value() * 12;
    curGdzc->setZjMonths(months);
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//固定资产其他信息改变
void GdzcAdminDialog::otherInfoChanged()
{
    curGdzc->setOtherInfo(ui->txtOtherInfo->toPlainText());
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//折旧信息条目的某个字段的内容改变了
void GdzcAdminDialog::zjItemInfoChanged(QTableWidgetItem* item)
{
    //我们只对日期和折旧值字段感兴趣
    if(item->column() == ColDate){
        curGdzc->setZjDate(item->text(),item->row());
        isChanged[ui->lvGdzc->currentRow()] = true;
    }
    else if(item->column() == ColValue){
        curGdzc->setZjValue(item->text().toDouble(),item->row());
        isChanged[ui->lvGdzc->currentRow()] = true;
    }
}

//显示固定资产折旧清单的表格列宽发生了改变
void GdzcAdminDialog::vlistColWidthChanged(int logicalIndex,
                                           int oldSize, int newSize)
{
    vlistColWidths[logicalIndex] = newSize;
}

//折旧信息表列宽改变了
void GdzcAdminDialog::zjInfosColWidthChanged(int logicalIndex,
                                             int oldSize, int newSize)
{
    zjInfoColWidths[logicalIndex] = newSize;
}

//折旧凭证的会计分录的表格列宽改变了
void GdzcAdminDialog::zjPzColWidthChanged(int logicalIndex,
                                          int oldSize, int newSize)
{
    pzColWidths[logicalIndex] = newSize;
}

//选择年或月
void GdzcAdminDialog::on_rdoYear_toggled(bool checked)
{
    //选择以年计
    if(checked){
        int y = ui->spnZjMonth->value() / 12;
        ui->spnZjMonth->setValue(y);
        ui->lblYear->setText(tr("年"));
    }
    else{
        int m = ui->spnZjMonth->value() * 12;
        ui->spnZjMonth->setValue(m);
        ui->lblYear->setText(tr("月"));
    }
}


//添加固定资产
void GdzcAdminDialog::on_btnAdd_clicked()
{
    curGdzc = new Gdzc();
    curGdzc->setProductClass(allGdzcProductCls.value(ui->cmbProCls->itemData(
                                      ui->cmbProCls->currentIndex()).toInt()));
    curGdzc->setBuyDate(QDate::currentDate());
    gdzcLst<<curGdzc;
    isChanged<<true;
    QListWidgetItem* item = new QListWidgetItem;
    ui->lvGdzc->addItem(item);
    ui->lvGdzc->setCurrentItem(item);
    ui->edtName->setFocus();
    ui->btnDel->setEnabled(true);
}

//删除固定资产
void GdzcAdminDialog::on_btnDel_clicked()
{
    int index = ui->lvGdzc->currentRow();
    delLst<<gdzcLst.takeAt(index);
    ui->lvGdzc->takeItem(index);

    if(gdzcLst.empty()){
        curGdzc = NULL;
        ui->btnDel->setEnabled(false);
    }
    else{
        //如果删除的是最后一个，则显示上一个，否者，显示下一个
        if(index == gdzcLst.count())
            index--;
        viewInfo(index);
    }
}

//添加固定资产折旧
//void GdzcAdminDialog::on_btnAddZj_clicked()
//{

//}

//删除固定资产折旧
void GdzcAdminDialog::on_btnDelZj_clicked()
{
    int row = ui->twZjTable->currentRow();
    if(row == -1)
        return;
    curGdzc->removeZjInfo(row);
    ui->twZjTable->removeRow(row);
    ui->edtRemain->setText(
            QString::number(curGdzc->getRemainValue(),'f',2));
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//确定
void GdzcAdminDialog::on_btnOk_clicked()
{
    save(false);
    accept();
}

//取消
void GdzcAdminDialog::on_btnCancel_clicked()
{
    isCancel = true;
    reject();
}


void GdzcAdminDialog::on_btnSave_clicked()
{
    save(false);
}

//创建计提折旧凭证
void GdzcAdminDialog::on_actJtzj_triggered()
{
    //确定凭证的所属年月
    int y = curAccount->getEndDate().year();
    int m = curAccount->getEndDate().month();
    if(QMessageBox::Yes == QMessageBox::warning(this,tr("提示信息"),
        tr("确定要创建%1年%2月的固定资产折旧凭证吗？").arg(y).arg(m),
        QMessageBox::Yes|QMessageBox::No)){
        Gdzc::createZjPz(y,m,curUser); //创建折旧凭证
        on_rdoAll_toggled(false);
    }

}

//撤销本月计提折旧
void GdzcAdminDialog::on_btnRepeal_clicked()
{
    //确定凭证的所属年月
    int y = curAccount->getEndDate().year();
    int m = curAccount->getEndDate().month();
    if(QMessageBox::Yes == QMessageBox::warning(this,tr("提示信息"),
        tr("确定要撤销%1年%2月固定资产计提折旧吗？").arg(y).arg(m),
        QMessageBox::Yes|QMessageBox::No)){
        Gdzc::repealZjPz(y,m);              //撤销折旧凭证
        on_rdoAll_toggled(false);
    }

}

//预览计提折旧凭证
void GdzcAdminDialog::on_actPreview_triggered()
{    
    QHash<int,double> zjs; //按科目类别汇总后的折旧值

    //搜索固定资产表，查找所有净值大于残值的固定资产
    QList<Gdzc*> glst;
    Gdzc::load(glst,false);

    if(glst.empty()){
        QMessageBox::information(this,tr("提示信息"),tr("没有可供折旧的固定资产！"));
        return;
    }

    //计算这些固定资产的折旧值，按固定资产的科目列表进行汇总
    foreach(Gdzc* g, glst){
        zjs[g->getSubClass()] = g->getZjValue();
    }

    QStandardItemModel m;
    QStandardItem* item;
    QList<QStandardItem*> l;
    QList<int> subKeys;    //固定资产的科目类别代码列表，主要是为了以一致的顺序显示
    subKeys = zjs.keys();
    qSort(subKeys.begin(),subKeys.end());
    double sum = 0,v;
    for(int i = 0; i < subKeys.count(); ++i){
        //列：摘要、一级科目、二级科目、币种、借方、贷方
        item = new QStandardItem(tr("计提本月折旧"));
        l<<item;
        item = new QStandardItem(tr("累计折旧"));
        l<<item;
        item = new QStandardItem(tr("%1").arg(allGdzcSubjectCls.value(subKeys[i])));
        l<<item;
        item = new QStandardItem(tr("人民币"));
        l<<item<<NULL;
        v = zjs.value(subKeys[i]);
        sum += v;
        item = new QStandardItem(QString::number(v,'f',2));
        l<<item;
        m.appendRow(l);
        l.clear();
    }
    item = new QStandardItem(tr("计提折旧"));
    l<<item;
    item = new QStandardItem(tr("管理费用"));
    l<<item;
    item = new QStandardItem(tr("累计折旧"));
    l<<item;
    item = new QStandardItem(tr("人民币"));
    l<<item;
    item = new QStandardItem(QString::number(sum,'f',2));
    l<<item<<NULL;
    m.appendRow(l);
    l.clear();

    m.setHeaderData(0,Qt::Horizontal,tr("摘要"));
    m.setHeaderData(1,Qt::Horizontal,tr("一级科目"));
    m.setHeaderData(2,Qt::Horizontal,tr("二级科目"));
    m.setHeaderData(3,Qt::Horizontal,tr("币种"));
    m.setHeaderData(4,Qt::Horizontal,tr("借方"));
    m.setHeaderData(5,Qt::Horizontal,tr("贷方"));

    //创建表格和对话框窗口以显示
    QDialog dlg;
    QString year,month;
    year = QString::number(curAccount->getEndDate().year());
    month = QString::number(curAccount->getEndDate().month());
    QLabel title(tr("%1年%2月计提固定资产折旧凭证").arg(year).arg(month));
    QTableView tv;
    tv.setModel(&m);
    tv.setColumnWidth(0,pzColWidths[0]);
    tv.setColumnWidth(1,pzColWidths[1]);
    tv.setColumnWidth(2,pzColWidths[2]);
    tv.setColumnWidth(3,pzColWidths[3]);
    tv.setColumnWidth(4,pzColWidths[4]);
    tv.setColumnWidth(5,pzColWidths[5]);
    tv.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(tv.horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(zjPzColWidthChanged(int,int,int)));
    QVBoxLayout* ly = new QVBoxLayout;
    ly->addWidget(&title);
    ly->addWidget(&tv);
    dlg.setLayout(ly);
    dlg.resize(pzWinRect);
    dlg.exec();
    pzWinRect = dlg.size();
}

//查看、打印固定资产折旧清单
void GdzcAdminDialog::on_actViewList_triggered()
{
    //打开选择对话框，由用户在当前帐套内选择要显示的月份
    bool ok;
    int y = curAccount->getCurSuiteRecord()->year;
    QString t = curAccount->getSuiteName(y);
    int im,m;
    if(curAccount->getCurSuiteRecord()->year == curAccount->getEndSuiteRecord()->year)
        im = curAccount->getEndDate().month();
    else
        im = 12;
    m = QInputDialog::getInt(this,tr("请求信息"),
                         tr("当前帐套：%1").arg(t),im,1,12,1,&ok);
    if(ok){
        //名称	原值	累计已提折旧	净值	本月计提折旧	本月原值变动
        //构造数据源
        QDate d(y,m,1);
        QString ds = d.toString(Qt::ISODate);
        ds.chop(3);
        QString s = QString("select gdzcs.name,gdzcs.prime,gdzcs.prime-gdzcs.remain,"
                            "gdzcs.remain,gdzczjs.price from gdzczjs join gdzcs on "
                            "gdzczjs.gid=gdzcs.id where gdzczjs.date like '%1%'")
                .arg(ds);
        QSqlQuery q;
        bool r = q.exec(s);
        QStandardItemModel* mo = new QStandardItemModel;
        QList<QStandardItem*> l;
        ApStandardItem* item;
        double sumPv=0,sumJsv=0,sumRv=0,sumJv=0; //合计：原值，累计计提，净值，本月计提
        double v;
        while(q.next()){
            item = new ApStandardItem(q.value(0).toString());
            l<<item;
            v = q.value(1).toDouble();
            sumPv += v;
            item = new ApStandardItem(v);
            l<<item;
            v = q.value(2).toDouble();
            sumJsv += v;
            item = new ApStandardItem(v);
            l<<item;
            v = q.value(3).toDouble();
            sumRv += v;
            item = new ApStandardItem(v);
            l<<item;
            v = q.value(4).toDouble();
            sumJv += v;
            item = new ApStandardItem(v);
            l<<item;
            mo->appendRow(l);
            l.clear();
        }
        item = new ApStandardItem(tr("合计"));
        l<<item;
        item = new ApStandardItem(sumPv);
        l<<item;
        item = new ApStandardItem(sumJsv);
        l<<item;
        item = new ApStandardItem(sumRv);
        l<<item;
        item = new ApStandardItem(sumJv);
        l<<item;
        mo->appendRow(l);
        l.clear();

        mo->setHeaderData(0,Qt::Horizontal,tr("名  称"));
        mo->setHeaderData(1,Qt::Horizontal,tr("原值"));
        mo->setHeaderData(2,Qt::Horizontal,tr("累计已提折旧"));
        mo->setHeaderData(3,Qt::Horizontal,tr("净值"));
        mo->setHeaderData(4,Qt::Horizontal,tr("本月计提折旧"));

        QLabel* title = new QLabel(tr("固定资产计提折旧汇总表"));
        QFont f = title->font();
        f.setPointSize(24);
        title->setFont(f);
        QLabel* dl = new QLabel(tr("%1年%2月").arg(y).arg(m));
        QTableView* tv = new QTableView;        
        tv->setModel(mo);
        tv->setColumnWidth(0,vlistColWidths[0]);
        tv->setColumnWidth(1,vlistColWidths[1]);
        tv->setColumnWidth(2,vlistColWidths[2]);
        tv->setColumnWidth(3,vlistColWidths[3]);
        tv->setColumnWidth(4,vlistColWidths[4]);
        connect(tv->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(vlistColWidthChanged(int,int,int)));
        QDialog dlg;
        IntentButton* btnPrint = new IntentButton(2,tr("打印"));
        IntentButton* btnPreview = new IntentButton(1,tr("打印预览"));
        IntentButton* btnClose = new IntentButton(0,tr("关闭"));
        QHBoxLayout* hl = new QHBoxLayout;
        hl->addWidget(btnPreview);
        hl->addWidget(btnPrint);
        hl->addWidget(btnClose);
        connect(btnClose,SIGNAL(intentClicked(int)),&dlg,SLOT(done(int)));
        connect(btnPrint,SIGNAL(intentClicked(int)),&dlg,SLOT(done(int)));
        connect(btnPreview,SIGNAL(intentClicked(int)),&dlg,SLOT(done(int)));
        QVBoxLayout* ly = new QVBoxLayout;
        ly->addWidget(title,1,Qt::AlignCenter);
        ly->addWidget(dl,1);
        ly->addWidget(tv,5);
        ly->addLayout(hl);
        dlg.setLayout(ly);
        dlg.resize(vlistWinRect);
        bool ret = false;
        while(!ret){
            int intent = dlg.exec();
            if(intent != 0){  //打印清单
                GdzcJtzjHzTable* tp = new GdzcJtzjHzTable(mo,&plistColWidths);
                tp->setDate(y,m);
                QPrinter* printer;
                if(intent == 1)
                    printer = new QPrinter;
                else
                    printer = new QPrinter(QPrinter::PrinterResolution);
                PreviewDialog pv(tp,COMMONPAGE,printer);
                if(intent == 1)
                    pv.exec();
                else{
                    pv.print();
                    ret = true;
                }
                delete tp;
            }
        }
        vlistWinRect = dlg.size();
        delete title;
        delete dl;
        delete tv;
        delete ly;
        delete mo;
    }
}

//补充指定期间不完善的折旧信息（这些折旧信息发生在建账前，或建账后没有使用固定资产折旧管理功能
//而手工创建了折旧凭证，因此，在gdzczjs表中就没有了相应的记录）
void GdzcAdminDialog::on_actSupply_triggered()
{
    QDialog dlg(this);
    QDateEdit sed,eed;
    sed.setDisplayFormat("yyyy-MM-dd");
    sed.setDate(curGdzc->getBuyDate());
    eed.setDisplayFormat("yyyy-MM-dd");
    eed.setDate(curAccount->getEndDate());
    QLabel l1(tr("开始日期："));
    QLabel l2(tr("结束日期："));
    QLabel l3(tr("折旧值："));
    double v = curGdzc->getPrimeValue() / curGdzc->getZjMonths();
    QLineEdit edtValue(QString::number(v,'f',2));
    QHBoxLayout lh1,lh2;
    lh1.addWidget(&l1);
    lh1.addWidget(&sed);
    lh1.addWidget(&l2);
    lh1.addWidget(&eed);
    lh2.addWidget(&l3);
    lh2.addWidget(&edtValue);
    QDialogButtonBox box(QDialogButtonBox::Ok|
                         QDialogButtonBox::Cancel,
                         Qt::Horizontal,&dlg);
    connect(&box,SIGNAL(accepted()),&dlg,SLOT(accept()));
    connect(&box,SIGNAL(rejected()),&dlg,SLOT(reject()));
    QVBoxLayout *lm = new QVBoxLayout;
    lm->addLayout(&lh1);
    lm->addLayout(&lh2);
    lm->addWidget(&box);
    dlg.setLayout(lm);

    if(dlg.exec() == QDialog::Accepted){
        QDate sd,ed;
        sd = sed.date();
        ed = eed.date();
        int index = ui->twZjTable->currentRow();
        if(index == -1)
            index = ui->twZjTable->rowCount()-1;
        disconnect(ui->twZjTable,SIGNAL(itemChanged(QTableWidgetItem*)),
                this, SLOT(zjItemInfoChanged(QTableWidgetItem*)));
        for(QDate d = sd; d <= ed; d=d.addMonths(1)){
            addZjInfo(d,v,++index);
            //sum += v;
        }
        connect(ui->twZjTable,SIGNAL(itemChanged(QTableWidgetItem*)),
                this, SLOT(zjItemInfoChanged(QTableWidgetItem*)));
        ui->edtRemain->setText(QString::number(curGdzc->getRemainValue(),'f',2));
        isChanged[ui->lvGdzc->currentRow()] = true;
    }
}

//创建一个新的折旧信息条目，时间自动设置为当前选择的条目的下一月
void GdzcAdminDialog::on_actNextZj_triggered()
{
    QDate d;
    double v;
    int index;

    v = curGdzc->getPrimeValue() / curGdzc->getZjMonths();
    index = ui->twZjTable->currentRow();
    if(index >= 0){
        d = QDate::fromString(ui->twZjTable->item(index,ColDate)->text(),Qt::ISODate);
        d = d.addMonths(1);
    }
    else{
        d = QDate::currentDate();
        index = ui->twZjTable->rowCount()-1;
    }
    disconnect(ui->twZjTable,SIGNAL(itemChanged(QTableWidgetItem*)),
            this, SLOT(zjItemInfoChanged(QTableWidgetItem*)));
    addZjInfo(d,v,index+1);
    connect(ui->twZjTable,SIGNAL(itemChanged(QTableWidgetItem*)),
            this, SLOT(zjItemInfoChanged(QTableWidgetItem*)));
    ui->edtRemain->setText(QString::number(curGdzc->getRemainValue(),'f',2));
    isChanged[ui->lvGdzc->currentRow()] = true;
}

//单击手动编辑折旧信息列表
void GdzcAdminDialog::on_chkHand_clicked(bool checked)
{
    ui->btnAddZj->setEnabled(checked);
    ui->btnDelZj->setEnabled(checked);
}

//选择的固定资产的范围改变
void GdzcAdminDialog::on_rdoAll_toggled(bool checked)
{
    //装载前，执行必要的保存操作
    save();
    load(checked);
}



////////////////////////////DtfyAdminDialog//////////////////////////////////////////
DtfyAdminDialog::DtfyAdminDialog(Account* account, QByteArray *sinfo, QWidget *parent) :
    QDialog(parent),ui(new Ui::DtfyAdminDialog),account(account)
{
    ui->setupUi(this);
    db = account->getDbUtil()->getDb();
    smg = account->getSubjectManager();
    curDtfy = NULL;
    isCancel = false;

    ui->btnJt->addAction(ui->actCrtDtfyPz);
    ui->btnJt->addAction(ui->actPreviewDtfyPz);
    ui->btnJt->addAction(ui->actRepealDtfyPz);
    ui->btnJt->addAction(ui->actJttx);
    ui->btnJt->addAction(ui->actPreview);
    ui->btnJt->addAction(ui->actRepeal);   
    ui->btnJt->addAction(ui->actView);

    QHash<int,DtfyType*> ts;
    if(!DtfyType::load(ts,db))
        return;
    //装载待摊费用类别
    foreach(DtfyType* t, ts){
        ui->cmbType->addItem(t->getName(),t->getCode());
        dTypes[t->getCode()] = t;
    }
    //装载贷方主目
    SubjectManager* sm = curAccount->getSubjectManager();
    int cashId,bankId;
    cashId = sm->getCashSub()->getId();
    bankId = sm->getBankSub()->getId();
    ui->cmbFsub->addItem("",0);
    ui->cmbFsub->addItem(sm->getFstSubject(cashId)->getName(),cashId);
    ui->cmbFsub->addItem(sm->getFstSubject(bankId)->getName(),bankId);
    QList<int> ids;
    QList<QString> names;
    //BusiUtil::getSndSubInSpecFst(subCashId,ids,names);
    //for(int i = 0; i < ids.count(); ++i)
    //    dsids[subCashId][ids[i]] = names[i];
    FirstSubject* fsub = smg->getCashSub(); //现金下只有一个子目
    dsids[fsub->getId()][fsub->getChildSub(0)->getId()] = fsub->getChildSub(0)->getName();

    //BusiUtil::getSndSubInSpecFst(subBankId,ids,names);
    //for(int i = 0; i < ids.count(); ++i)
    //    dsids[subBankId][ids[i]] = names[i];
    fsub = smg->getBankSub();
    foreach(SecondSubject* ssub, fsub->getChildSubs())
        dsids[fsub->getId()][ssub->getId()] = ssub->getName();

    load(ui->rdoAll->isChecked());
    setState(sinfo);
    ui->twFt->setColumnWidth(0,txItemColWidths[0]);
    ui->twFt->setColumnWidth(1,txItemColWidths[1]);
    connect(ui->twFt->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(txItemColWidthChanged(int,int,int)));
}

DtfyAdminDialog::~DtfyAdminDialog()
{
    delete ui;
}

void DtfyAdminDialog::save(bool isConfirm)
{
    if(isCancel)
        return;

    if((isChanged.contains(true) || !dels.empty())){
        if(!isConfirm ||
           (isConfirm && (QMessageBox::Yes == QMessageBox::warning(this,tr("提示信息"),
             tr("当前待摊费用内容已被修改且未保存，需要保存吗？"),
             QMessageBox::Yes|QMessageBox::No)))){
            for(int i = 0; i < isChanged.count(); ++i){
                if(isChanged[i])
                    dtfys[i]->save();
            }
            if(!dels.empty()){
                for(int i = 0; i < dels.count(); ++i){
                    dels[i]->remove();
                    delete dels[i];
                }
                dels.clear();
            }
        }
    }
}

//
QByteArray* DtfyAdminDialog::getState()
{
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 i8;
    qint16 i16;

    //显示摊销清单的窗口大小
    i16 = vlistWinRect.width();
    out<<i16;
    i16 = vlistWinRect.height();
    out<<i16;
    //plistColWidths打印摊销清单的表格列宽
    //vlistColWidths显示摊销清单的表格列宽
    i8 = 5;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = plistColWidths[i];
        out<<i16;
        i16 = vlistColWidths[i];
        out<<i16;
    }
    //txItemColWidths显示摊销项的表格列宽
    i8 = 2;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = txItemColWidths[i];
        out<<i16;
    }
    //预览摊销凭证的窗口大小
    i16 = pzWinRect.width();
    out<<i16;
    i16 = pzWinRect.height();
    out<<i16;
    //pzColWidths预览摊销凭证的会计分录的表格列宽
    i8 = 6;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = pzColWidths[i];
        out<<i16;
    }

    bf.close();
    return info;
}

//
void DtfyAdminDialog::setState(QByteArray* info)
{
    if(info == NULL){
        plistColWidths<<300<<120<<120<<120<<120;
        vlistColWidths<<300<<120<<120<<120<<120;
        txItemColWidths<<200<<200;
        pzColWidths<<300<<150<<150<<150<<150<<150;
        vlistWinRect = QSize(850,400);
        pzWinRect = QSize(700,400);
    }
    else{
        QBuffer bf(info);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        qint8 i8;
        qint16 i16;

        //显示摊销清单的窗口大小
        in>>i16;
        vlistWinRect.setWidth(i16);
        in>>i16;
        vlistWinRect.setHeight(i16);
        //打印、查看摊销清单的表格列宽
        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            plistColWidths<<i16;
            in>>i16;
            vlistColWidths<<i16;
        }
        //显示摊销项的表格列宽
        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            txItemColWidths<<i16;
        }
        //预览摊销凭证的窗口大小
        in>>i16;
        pzWinRect.setWidth(i16);
        in>>i16;
        pzWinRect.setHeight(i16);
        //预览摊销凭证的会计分录的表格列宽
        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            pzColWidths<<i16;
        }
        bf.close();
    }
}

//
void DtfyAdminDialog::load(bool isAll)
{
    disconnect(ui->dlst,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(curDtfyChanged(QListWidgetItem*,QListWidgetItem*)));
    foreach(Dtfy* d,dtfys)
        delete d;
    dtfys.clear();
    isChanged.clear();
    ui->dlst->clear();
    Dtfy::load(dtfys,isAll,db);

    if(!dtfys.empty()){
        for(int i = 0; i < dtfys.count(); ++i){
            isChanged<<false;
            QListWidgetItem* item = new QListWidgetItem(dtfys[i]->getName());
            ui->dlst->addItem(item);
        }
        ui->dlst->setCurrentRow(0);
        curDtfy = dtfys[0];
        ui->btnDel->setEnabled(true);
    }
    else{
        curDtfy = NULL;
        ui->btnDel->setEnabled(false);
    }
    viewInfo();
    connect(ui->dlst,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(curDtfyChanged(QListWidgetItem*,QListWidgetItem*)));
}

void DtfyAdminDialog::viewInfo()
{
    bindSignal(false);
    if(curDtfy == NULL){
        ui->edtName->setText("");
        ui->edtName->setReadOnly(true);
        ui->cmbType->setCurrentIndex(0);
        ui->edtTotal->setText("");
        ui->edtTotal->setReadOnly(true);
        ui->edtRemain->setText("");
        ui->edtRemain->setReadOnly(true);
        ui->spnMonth->setValue(0);
        ui->spnMonth->setReadOnly(true);
        ui->txtExplain->setPlainText("");
        ui->txtExplain->setReadOnly(true);
        ui->datStart->setDate(QDate::currentDate());
        ui->datStart->setReadOnly(true);
        ui->datImp->setDate(QDate::currentDate());
        ui->datImp->setReadOnly(true);
        viewDSub();

        ui->twFt->clearContents();
    }
    else{
        ui->edtName->setText(curDtfy->getName());
        ui->edtName->setReadOnly(false);
        ui->datImp->setDate(curDtfy->importDate());
        if(curDtfy->getType() == NULL)
            ui->cmbType->setCurrentIndex(0);
        else
            ui->cmbType->setCurrentIndex(ui->cmbType->
                   findData(curDtfy->getType()->getCode()));
        ui->edtTotal->setText(QString::number(curDtfy->totalValue(),'f',2));
        ui->edtRemain->setText(QString::number(curDtfy->remainValue(),'f',2));
        ui->spnMonth->setValue(curDtfy->getMonths());
        ui->txtExplain->setPlainText(curDtfy->getExPlain());
        ui->txtExplain->setReadOnly(false);
        ui->datStart->setDate(curDtfy->startDate());
        viewDSub();
        viewFtItem();

        bool r = curDtfy->isStartTx();
        ui->edtTotal->setReadOnly(r);
        ui->edtRemain->setReadOnly(r);
        ui->spnMonth->setReadOnly(r);
        ui->datStart->setReadOnly(r);
        ui->datImp->setReadOnly(r);
        ui->cmbFsub->setEnabled(!r);
        ui->cmbSsub->setEnabled(!r);
    }
    bindSignal();
}

//显示贷方科目
void DtfyAdminDialog::viewDSub()
{
    if((curDtfy == NULL) || (curDtfy->getDFid() == 0)){
        ui->cmbFsub->setCurrentIndex(0);
        ui->cmbSsub->clear();
    }
    else{
        int idx = ui->cmbFsub->findData(curDtfy->getDFid());
        ui->cmbFsub->setCurrentIndex(idx);
        ui->cmbSsub->clear();
        QHashIterator<int,QString> it(dsids.value(curDtfy->getDFid()));
        while(it.hasNext()){
            it.next();
            ui->cmbSsub->addItem(it.value(),it.key());
        }
        idx = ui->cmbSsub->findData(curDtfy->getDSid());
        ui->cmbSsub->setCurrentIndex(idx);
    }
}


//显示分摊项目
void DtfyAdminDialog::viewFtItem()
{
    disconnect(ui->twFt,SIGNAL(itemChanged(QTableWidgetItem*)),
               this,SLOT(TxItemInfoChanged(QTableWidgetItem*)));
    QTableWidgetItem* item;
    int row = 0;
    ui->twFt->clearContents();
    ui->twFt->setRowCount(0);
    foreach(Dtfy::FtItem* ft, curDtfy->getFtItems()){
        ui->twFt->insertRow(row);
        item = new QTableWidgetItem(ft->date);
        item->setTextAlignment(Qt::AlignCenter);
        ui->twFt->setItem(row,0,item);
        item = new QTableWidgetItem(QString::number(ft->v,'f',2));
        item->setTextAlignment(Qt::AlignCenter);
        ui->twFt->setItem(row,1,item);
        row++;
    }
    connect(ui->twFt,SIGNAL(itemChanged(QTableWidgetItem*)),
               this,SLOT(TxItemInfoChanged(QTableWidgetItem*)));
}

//添加待摊费用
void DtfyAdminDialog::on_btnAdd_clicked()
{
    curDtfy = new Dtfy(db);
    int tid = ui->cmbType->itemData(ui->cmbType->currentIndex()).toInt();
    curDtfy->setType(dTypes.value(tid));
    dtfys<<curDtfy;
    isChanged<<true;
    QListWidgetItem* item = new QListWidgetItem;
    ui->dlst->addItem(item);
    ui->dlst->setCurrentItem(item);
    ui->edtName->setFocus();
    ui->btnDel->setEnabled(true);
    viewInfo();
}

//删除待摊费用
void DtfyAdminDialog::on_btnDel_clicked()
{
    int index = ui->dlst->currentRow();
    dels<<dtfys.takeAt(index);
    if(dtfys.empty())
        curDtfy = NULL;
    else if(index == dtfys.count())
        curDtfy = dtfys[--index];
    viewInfo();
}

//
void DtfyAdminDialog::on_rdoAll_toggled(bool checked)
{
    save();
    if(curDtfy)
        curDtfy = NULL;

    load(checked);
}

//
void DtfyAdminDialog::on_checkBox_clicked(bool checked)
{
    ui->btnComplete->setEnabled(checked);
    ui->btnDelTx->setEnabled(checked);
}

void DtfyAdminDialog::bindSignal(bool bind)
{
    if(bind){
        connect(ui->edtName,SIGNAL(textChanged(QString)),this,SLOT(nameChanged(QString)));
        connect(ui->datImp,SIGNAL(dateChanged(QDate)),this,SLOT(importDateChanged(QDate)));
        connect(ui->cmbType,SIGNAL(currentIndexChanged(int)),this,SLOT(typeChanged(int)));
        connect(ui->cmbFsub,SIGNAL(currentIndexChanged(int)),this,SLOT(dFstSubChanged(int)));
        connect(ui->cmbSsub,SIGNAL(currentIndexChanged(int)),this,SLOT(dSndSubChanged(int)));
        connect(ui->edtTotal,SIGNAL(textChanged(QString)),this,SLOT(totalValueChanged(QString)));
        connect(ui->edtRemain,SIGNAL(textChanged(QString)),this,SLOT(remainValueChanged(QString)));
        connect(ui->spnMonth,SIGNAL(valueChanged(int)),this,SLOT(monthsChanged(int)));
        connect(ui->txtExplain,SIGNAL(textChanged()),this,SLOT(replainTextChanged()));
        connect(ui->datStart,SIGNAL(dateChanged(QDate)),this,SLOT(startDateChanged(QDate)));
    }
    else{
        disconnect(ui->edtName,SIGNAL(textChanged(QString)),this,SLOT(nameChanged(QString)));
        disconnect(ui->datImp,SIGNAL(dateChanged(QDate)),this,SLOT(importDateChanged(QDate)));
        disconnect(ui->cmbType,SIGNAL(currentIndexChanged(int)),this,SLOT(typeChanged(int)));
        disconnect(ui->cmbFsub,SIGNAL(currentIndexChanged(int)),this,SLOT(dFstSubChanged(int)));
        disconnect(ui->cmbSsub,SIGNAL(currentIndexChanged(int)),this,SLOT(dSndSubChanged(int)));
        disconnect(ui->edtTotal,SIGNAL(textChanged(QString)),this,SLOT(totalValueChanged(QString)));
        disconnect(ui->edtRemain,SIGNAL(textChanged(QString)),this,SLOT(remainValueChanged(QString)));
        disconnect(ui->spnMonth,SIGNAL(valueChanged(int)),this,SLOT(monthsChanged(int)));
        disconnect(ui->txtExplain,SIGNAL(textChanged()),this,SLOT(replainTextChanged()));
        disconnect(ui->datStart,SIGNAL(dateChanged(QDate)),this,SLOT(startDateChanged(QDate)));
    }
}

//插入摊销项
void DtfyAdminDialog::insertTxInfo(QDate date,double v,int index)
{
    if(!curDtfy->insertTxInfo(date,v,index)){
        qDebug() << tr("在增加摊销项时，出现日期冲突（%1）")
                    .arg(date.toString(Qt::ISODate));
        return;
    }
    QTableWidgetItem* item;
    ui->twFt->insertRow(index);
    item = new QTableWidgetItem(date.toString(Qt::ISODate));
    item->setTextAlignment(Qt::AlignCenter);
    ui->twFt->setItem(index,0,item);
    item = new QTableWidgetItem(QString::number(v,'f',2));
    item->setTextAlignment(Qt::AlignCenter);
    ui->twFt->setItem(index,1,item);
}

//显示预览窗口（待摊费用凭证或计提摊销凭证）
void DtfyAdminDialog::preview(QString title,QString d,QList<Dtfy::BaItemData*> datas)
{
    QStandardItemModel m;
    QList<QStandardItem*> items;
    SubjectManager* sm = curAccount->getSubjectManager();
    double sum = 0;
    foreach(Dtfy::BaItemData* ba,datas){
        //列：摘要、一级科目、二级科目、币种、借方、贷方
        items<<new QStandardItem(ba->summary);
        items<<new QStandardItem(sm->getFstSubject(ba->fid)->getName());
        items<<new QStandardItem(sm->getSndSubject(ba->sid)->getName());
        items<<new QStandardItem(allMts.value(ba->mt));
        if(ba->dir == DIR_J)
            items<<new ApStandardItem(ba->v)<<NULL;
        else
            items<<NULL<<new ApStandardItem(ba->v);
        sum += ba->v;
        m.appendRow(items);
        items.clear();
    }
    m.setHeaderData(0,Qt::Horizontal,tr("摘要"));
    m.setHeaderData(1,Qt::Horizontal,tr("一级科目"));
    m.setHeaderData(2,Qt::Horizontal,tr("二级科目"));
    m.setHeaderData(3,Qt::Horizontal,tr("币种"));
    m.setHeaderData(4,Qt::Horizontal,tr("借方"));
    m.setHeaderData(5,Qt::Horizontal,tr("贷方"));

    QDialog dlg;
    QLabel* lblTitle = new QLabel(title);
    QLabel* lblDate = new QLabel(d);
    QTableView tv;
    tv.setModel(&m);
    tv.setColumnWidth(0,pzColWidths[0]);
    tv.setColumnWidth(1,pzColWidths[1]);
    tv.setColumnWidth(2,pzColWidths[2]);
    tv.setColumnWidth(3,pzColWidths[3]);
    tv.setColumnWidth(4,pzColWidths[4]);
    tv.setColumnWidth(5,pzColWidths[5]);
    tv.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(tv.horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(TxPzColWidthChanged(int,int,int)));
    QVBoxLayout* ly = new QVBoxLayout;
    ly->addWidget(lblTitle,0,Qt::AlignCenter);
    ly->addWidget(lblDate);
    ly->addWidget(&tv);
    dlg.setLayout(ly);
    dlg.resize(pzWinRect);
    dlg.exec();
    pzWinRect = dlg.size();
    delete lblTitle;
    delete lblDate;
}

//移除摊销项
void DtfyAdminDialog::removeTxInfo(int idx)
{
    if(!curDtfy->removeTxInfo(idx)){
        qDebug() << tr("在移除摊销项时出错！");
        return;
    }
    ui->twFt->removeRow(idx);
}

//
void DtfyAdminDialog::curDtfyChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    int index = ui->dlst->currentRow();
    curDtfy = dtfys[index];
    viewInfo();
}

void DtfyAdminDialog::typeChanged(int index)
{
    if(!curDtfy)
        return;
    isChanged[ui->dlst->currentRow()] = true;
    curDtfy->setType(dTypes.value(ui->cmbType->itemData(index).toInt()));
}

void DtfyAdminDialog::nameChanged(const QString &text)
{
    if(!curDtfy)
        return;
    int index = ui->dlst->currentRow();
    isChanged[index] = true;
    curDtfy->setName(text);
    QListWidgetItem* item = ui->dlst->currentItem();
    item->setText(text);
}

//
void DtfyAdminDialog::importDateChanged(const QDate &date)
{
    if(!curDtfy)
        return;
    int index = ui->dlst->currentRow();
    isChanged[index] = true;
    curDtfy->setImportDate(date);
    ui->datStart->setDate(date);
}

//贷方主目改变
void DtfyAdminDialog::dFstSubChanged(int index)
{
    if(!curDtfy)
        return;
    isChanged[ui->dlst->currentRow()] = true;
    curDtfy->setDFid(ui->cmbFsub->itemData(index).toInt());
}

//贷方子目改变
void DtfyAdminDialog::dSndSubChanged(int index)
{
    if(!curDtfy)
        return;
    isChanged[ui->dlst->currentRow()] = true;
    curDtfy->setDSid(ui->cmbSsub->itemData(index).toInt());
}

//摊销信息改变
void DtfyAdminDialog::TxItemInfoChanged(QTableWidgetItem* item)
{
    if(item->column() == 0){
        curDtfy->setTxDate(item->text(),item->row());
        isChanged[ui->dlst->currentRow()] = true;
    }
    else if(item->column() == 1){
        curDtfy->setTxValue(item->text().toDouble(),item->row());
        isChanged[ui->dlst->currentRow()] = true;
    }
}

void DtfyAdminDialog::totalValueChanged(const QString &text)
{
    if(!curDtfy)
        return;
    int index = ui->dlst->currentRow();
    isChanged[index] = true;
    double v = text.toDouble();
    curDtfy->setTotalValue(v);
    curDtfy->setRemainValue(v);
    ui->edtRemain->setText(text);
}

void DtfyAdminDialog::remainValueChanged(const QString &text)
{
    if(!curDtfy)
        return;
    int index = ui->dlst->currentRow();
    isChanged[index] = true;
    curDtfy->setRemainValue(text.toDouble());
}

void DtfyAdminDialog::monthsChanged(int m)
{
    if(!curDtfy)
        return;
    int index = ui->dlst->currentRow();
    isChanged[index] = true;
    curDtfy->setMonths(m);
}

void DtfyAdminDialog::replainTextChanged()
{
    if(!curDtfy)
        return;
    int index = ui->dlst->currentRow();
    isChanged[index] = true;
    curDtfy->setExplain(ui->txtExplain->toPlainText());
}

void DtfyAdminDialog::startDateChanged(const QDate &date)
{
    if(!curDtfy)
        return;
    int index = ui->dlst->currentRow();
    isChanged[index] = true;
    curDtfy->setStartDate(date);
}

//显示某月摊销清单的表格列宽改变了
void DtfyAdminDialog::vlistColWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    vlistColWidths[logicalIndex] = newSize;
}

//某个待摊费用的已摊销项的表格列宽改变了
void DtfyAdminDialog::txItemColWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    txItemColWidths[logicalIndex] = newSize;
}

//显示摊销凭证的列宽改变了
void DtfyAdminDialog::TxPzColWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    pzColWidths[logicalIndex] = newSize;
}

//用户选择了一个贷方主目
void DtfyAdminDialog::on_cmbFsub_currentIndexChanged(int index)
{
    if(!curDtfy)
        return;
    int subId = ui->cmbFsub->itemData(index).toInt();
    ui->cmbSsub->clear();
    if(subId != 0){
        QHashIterator<int,QString> it(dsids.value(subId));
        while(it.hasNext()){
            it.next();
            ui->cmbSsub->addItem(it.value(),it.key());
        }
    }
    curDtfy->setDFid(subId);
    //curDtfy->setDSid(ui->cmbSsub->itemData(ui->cmbSsub->currentIndex()).toInt());
    isChanged[ui->dlst->currentRow()] = true;
}

//创建待摊费用凭证
void DtfyAdminDialog::on_actCrtDtfyPz_triggered()
{

}

//撤销创建的待摊费用凭证
void DtfyAdminDialog::on_actRepealDtfyPz_triggered()
{

}

//预览将要创建的待摊费用凭证
void DtfyAdminDialog::on_actPreviewDtfyPz_triggered()
{
//    QList<Dtfy::BaItemData*> datas;
//    if(!Dtfy::genDtfyPzDatas(2011,7,datas))
//        return;
//    preview(tr("引入待摊费用凭证"),tr("2011年7月"),datas);
}

//创建计提摊销凭证
void DtfyAdminDialog::on_actJttx_triggered()
{
    //AccountSuiteManager* pzset = curAccount->getPzSet();
    //pzset->crtDtfyTxPz();
}

//取消计提摊销凭证
void DtfyAdminDialog::on_actRepeal_triggered()
{
    //AccountSuiteManager* pzset = curAccount->getPzSet();
    //pzset->delDtfyPz();
}


//预览计提摊销凭证
void DtfyAdminDialog::on_actPreview_triggered()
{

}

//查看摊销清单
void DtfyAdminDialog::on_actView_triggered()
{
    //打开选择对话框，由用户在当前帐套内选择要显示的月份
    bool ok;
    int y = curAccount->getCurSuiteRecord()->year;
    QString t = curAccount->getSuiteName(y);
    int im,m;
    if(curAccount->getCurSuiteRecord()->year == curAccount->getEndSuiteRecord()->year)
        im = curAccount->getEndDate().month();
    else
        im = 12;
    m = QInputDialog::getInt(this,tr("请求信息"),
                         tr("当前帐套：%1").arg(t),im,1,12,1,&ok);
    if(ok){
        //名称	原值	累计摊销	余值	本月摊销
        //构造数据源
        QList<Dtfy::TxmItem*> txs;
        if(!Dtfy::getFtManifest(y,m,txs,db))
            return;

        QStandardItemModel* mo = new QStandardItemModel;
        QList<QStandardItem*> l;
        ApStandardItem* item;
        double sumTv=0,sumTsv=0,sumRv=0,sumTcv=0; //合计：总值，累计摊销，余值，本月摊销
        foreach(Dtfy::TxmItem* tx,txs){
            item = new ApStandardItem(tx->name);
            l<<item;
            sumTv += tx->total;
            item = new ApStandardItem(tx->total);
            l<<item;
            sumTsv += tx->txSum;
            item = new ApStandardItem(tx->txSum);
            l<<item;
            sumRv += tx->remain;
            item = new ApStandardItem(tx->remain);
            l<<item;
            sumTcv += tx->curMonth;
            item = new ApStandardItem(tx->curMonth);
            l<<item;
            mo->appendRow(l);
            l.clear();
        }
        item = new ApStandardItem(tr("合计"));
        l<<item;
        item = new ApStandardItem(sumTv);
        l<<item;
        item = new ApStandardItem(sumTsv);
        l<<item;
        item = new ApStandardItem(sumRv);
        l<<item;
        item = new ApStandardItem(sumTcv);
        l<<item;
        mo->appendRow(l);
        l.clear();

        mo->setHeaderData(0,Qt::Horizontal,tr("名  称"));
        mo->setHeaderData(1,Qt::Horizontal,tr("总值"));
        mo->setHeaderData(2,Qt::Horizontal,tr("累计摊销"));
        mo->setHeaderData(3,Qt::Horizontal,tr("余值"));
        mo->setHeaderData(4,Qt::Horizontal,tr("本月摊销"));

        QLabel* title = new QLabel(tr("待摊费用计提摊销汇总表"));
        QFont f = title->font();
        f.setPointSize(24);
        title->setFont(f);
        QLabel* dl = new QLabel(tr("%1年%2月").arg(y).arg(m));
        QTableView* tv = new QTableView;
        tv->setModel(mo);
        tv->setColumnWidth(0,vlistColWidths[0]);
        tv->setColumnWidth(1,vlistColWidths[1]);
        tv->setColumnWidth(2,vlistColWidths[2]);
        tv->setColumnWidth(3,vlistColWidths[3]);
        tv->setColumnWidth(4,vlistColWidths[4]);
        connect(tv->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
                this,SLOT(vlistColWidthChanged(int,int,int)));
        QDialog dlg;
        IntentButton* btnPrint = new IntentButton(2,tr("打印"));
        IntentButton* btnPreview = new IntentButton(1,tr("打印预览"));
        IntentButton* btnClose = new IntentButton(0,tr("关闭"));
        QHBoxLayout* hl = new QHBoxLayout;
        hl->addWidget(btnPreview);
        hl->addWidget(btnPrint);
        hl->addWidget(btnClose);
        connect(btnClose,SIGNAL(intentClicked(int)),&dlg,SLOT(done(int)));
        connect(btnPrint,SIGNAL(intentClicked(int)),&dlg,SLOT(done(int)));
        connect(btnPreview,SIGNAL(intentClicked(int)),&dlg,SLOT(done(int)));
        QVBoxLayout* ly = new QVBoxLayout;
        ly->addWidget(title,1,Qt::AlignCenter);
        ly->addWidget(dl,1);
        ly->addWidget(tv,5);
        ly->addLayout(hl);
        dlg.setLayout(ly);
        dlg.resize(vlistWinRect);
        bool ret = false;
        while(!ret){
            int intent = dlg.exec();
            if(intent != 0){  //打印清单
                DtfyJttxHzTable* tp = new DtfyJttxHzTable(mo,&plistColWidths);
                tp->setDate(y,m);
                QPrinter* printer;
                if(intent == 1)
                    printer = new QPrinter;
                else
                    printer = new QPrinter(QPrinter::PrinterResolution);
                PreviewDialog pv(tp,COMMONPAGE,printer);
                if(intent == 1)
                    pv.exec();
                else{
                    pv.print();
                    ret = true;
                }
                delete tp;
            }
        }
        delete title;
        delete dl;
        delete tv;
        delete ly;
        delete mo;
    }
}

//补齐摊销项
void DtfyAdminDialog::on_btnComplete_clicked()
{
    QDialog dlg(this);
    QDateEdit sed,eed;
    sed.setDisplayFormat("yyyy-MM-dd");
    sed.setDate(curDtfy->startDate());
    eed.setDisplayFormat("yyyy-MM-dd");
    eed.setDate(curAccount->getEndDate());
    QLabel l1(tr("开始日期："));
    QLabel l2(tr("结束日期："));
    QLabel l3(tr("摊销值："));
    double v = curDtfy->totalValue() / curDtfy->getMonths();
    QLineEdit edtValue(QString::number(v,'f',2));
    QHBoxLayout lh1,lh2;
    lh1.addWidget(&l1);
    lh1.addWidget(&sed);
    lh1.addWidget(&l2);
    lh1.addWidget(&eed);
    lh2.addWidget(&l3);
    lh2.addWidget(&edtValue);
    QDialogButtonBox box(QDialogButtonBox::Ok|
                         QDialogButtonBox::Cancel,
                         Qt::Horizontal,&dlg);
    connect(&box,SIGNAL(accepted()),&dlg,SLOT(accept()));
    connect(&box,SIGNAL(rejected()),&dlg,SLOT(reject()));
    QVBoxLayout *lm = new QVBoxLayout;
    lm->addLayout(&lh1);
    lm->addLayout(&lh2);
    lm->addWidget(&box);
    dlg.setLayout(lm);

    if(dlg.exec() == QDialog::Accepted){
        QDate sd,ed;
        sd = sed.date();
        ed = eed.date();
        int index = ui->twFt->currentRow();
        if(index == -1)
            index = ui->twFt->rowCount()-1;
        disconnect(ui->twFt,SIGNAL(itemChanged(QTableWidgetItem*)),
                this, SLOT(TxItemInfoChanged(QTableWidgetItem*)));
        for(QDate d = sd; d <= ed; d=d.addMonths(1))
            insertTxInfo(d,v,++index);

        connect(ui->twFt,SIGNAL(itemChanged(QTableWidgetItem*)),
                this, SLOT(TxItemInfoChanged(QTableWidgetItem*)));
        ui->edtRemain->setText(QString::number(curDtfy->remainValue(),'f',2));
        isChanged[ui->dlst->currentRow()] = true;
    }
}

//删除摊销项
void DtfyAdminDialog::on_btnDelTx_clicked()
{
    int idx = ui->twFt->currentRow();
    removeTxInfo(idx);
    isChanged[ui->dlst->currentRow()] = true;
    disconnect(ui->edtRemain,SIGNAL(textChanged(QString)),this,SLOT(remainValueChanged(QString)));
    ui->edtRemain->setText(QString::number(curDtfy->remainValue(),'f',2));
    connect(ui->edtRemain,SIGNAL(textChanged(QString)),this,SLOT(remainValueChanged(QString)));
}

void DtfyAdminDialog::on_btnSave_clicked()
{
    save(false);
}

void DtfyAdminDialog::on_btnOk_clicked()
{
    save(false);
    accept();
}

void DtfyAdminDialog::on_btnCancel_clicked()
{
    isCancel = true;
    reject();
}




//////////////////////////////ShowTZDialog///////////////////////////////////////
ShowTZDialog::ShowTZDialog(int y, int m, QByteArray* sinfo, QWidget *parent) : QDialog(parent),
    ui(new Ui::ShowTZDialog),y(y),m(m)
{
    QSqlQuery q;

    ui->setupUi(this);

    headerModel = NULL;
    dataModel = NULL;
    hv = NULL;

    //初始化一级科目组合框
    q.exec(QString("select id,%1 from %2 where %3=1")
           .arg(fld_fsub_name).arg(tbl_fsub).arg(fld_fsub_isview));
    while(q.next())
        ui->cmbSub->addItem(q.value(1).toString(),q.value(0).toInt());
    fcom = new SubjectComplete;
    ui->cmbSub->setCompleter(fcom);

    //初始化开始和结束月份
    for(int i = 0; i < 12; ++i)    {
        QString item = tr("%1月").arg(i+1);
        ui->cmbSm->addItem(item);
        ui->cmbEm->addItem(item);
    }
    ui->cmbSm->setCurrentIndex(0);
    ui->cmbEm->setCurrentIndex(m-1);

    //初始化外币代码列表，并使它们以一致的顺序显示
    mts = allMts.keys();
    mts.removeOne(RMB);
    qSort(mts.begin(),mts.end());

    hv = new HierarchicalHeaderView(Qt::Horizontal, ui->tview);
    hv->setHighlightSections(true);
    //hv->setClickable(true);
    hv->setSectionsClickable(true);
    ui->tview->setHorizontalHeader(hv);

    connect(ui->cmbSub, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSub(int)));
    connect(ui->cmbSm, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelStartMonth(int)));
    connect(ui->cmbEm, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelEndMonth(int)));
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));

    fid = ui->cmbSub->itemData(ui->cmbSub->currentIndex()).toInt();
    sm = ui->cmbSm->currentIndex()+1;
    em = ui->cmbEm->currentIndex()+1;


    ui->btnPrint->addAction(ui->actPrint);
    ui->btnPrint->addAction(ui->actPreview);
    ui->btnPrint->addAction(ui->actToPdf);

    setState(sinfo);

    onSelSub(0);
}

ShowTZDialog::~ShowTZDialog()
{
    delete ui;
}

void ShowTZDialog::setState(QByteArray* info)
{
    if(info == NULL){
        colWidths[COMMON]<<100<<300<<200<<200<<100<<200;
        colWidths[THREERAIL]<<100<<200<<100<<100<<100<<100<<100<<50<<100<<100;
        colPrtWidths[COMMON]<<100<<300<<200<<200<<100<<200;
        colPrtWidths[THREERAIL]<<100<<200<<100<<100<<100<<100<<100<<50<<100<<100;
        pageOrientation = QPrinter::Landscape;
        margins.unit = QPrinter::Didot;
        margins.left = 20; margins.right = 20;
        margins.top = 30; margins.bottom = 30;
    }
    else{
        QBuffer bf(info);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        qint8 i8;
        qint16 i16;

        //通用式表格列宽
        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            colWidths[COMMON] << i16;
            in>>i16;
            colPrtWidths[COMMON]<<i16;
        }
        //三栏式表格列宽
        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            colWidths[THREERAIL] << i16;
            in>>i16;
            colPrtWidths[THREERAIL]<<i16;
        }

        //页边距和页面方向
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

QByteArray* ShowTZDialog::getState()
{
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 i8;
    qint16 i16;

    //两种表格
    //（通用式总共6列）
    i8 = 6;
    out<<i8;
    for(int i = 0; i < i8; ++i){
        i16 = colWidths.value(COMMON)[i];
        out<<i16;
        i16 = colPrtWidths.value(COMMON)[i];
        out<<i16;
    }
    //（三栏式总共10列）
    i8 = 10;
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
    return info;
}

//选择一个总账科目
void ShowTZDialog::onSelSub(int index)
{
    fid = ui->cmbSub->itemData(index).toInt();
    refreshTable();
}

//选择开始月份
void ShowTZDialog::onSelStartMonth(int index)
{
    sm = index+1;
    if(index > ui->cmbEm->currentIndex()){
        disconnect(ui->cmbEm, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelEndMonth(int)));
        ui->cmbEm->setCurrentIndex(index);
        connect(ui->cmbEm, SIGNAL(currentIndexChanged(int)),this,SLOT(onSelEndMonth(int)));
    }
    else
        refreshTable();
}

//当表格的宽度改变时，记录在内部的状态列表中
void ShowTZDialog::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    colWidths[curFormat][logicalIndex] = newSize;
}

//选择结束月份
void ShowTZDialog::onSelEndMonth(int index)
{
    em = index + 1;
    refreshTable();
}

//刷新表格数据
void ShowTZDialog::refreshTable()
{
    disconnect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));

    if(headerModel){
        delete headerModel;
    }
    headerModel = new QStandardItemModel;

    if(dataModel){
        delete dataModel;
    }
    dataModel = new MyWithHeaderModels;

    if((fid == subBankId) || (fid == subYsId) || (fid == subYfId)){
        curFormat = THREERAIL;
        genThForThreeRail();
        genDataForThreeRail();
    }
    else{
        curFormat = COMMON;
        genThForCommon();
        genDataForCommon();
    }

    dataModel->setHorizontalHeaderModel(headerModel);
    ui->tview->setModel(dataModel);

    //设置列宽
    for(int i = 0; i < colWidths.value(curFormat).count(); ++i)
        ui->tview->setColumnWidth(i,colWidths.value(curFormat)[i]);

    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));
}

//生成通用金额式格式表头
void ShowTZDialog::genThForCommon()
{
    //总共6个字段
    QStandardItem* fi;  //第一级表头
    QStandardItem* si;  //第二级表头
    QList<QStandardItem*> l1,l2;//l1用于存放表头第一级，l2存放表头第二级

    fi = new QStandardItem(QString(tr("%1年")).arg(y));
    l1<<fi;
    si = new QStandardItem(tr("月份"));  //index: 0
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();
    fi = new QStandardItem(tr("摘要"));  //index: 1
    l1<<fi;

    //借方
    fi = new QStandardItem(tr("借方"));//index: 2
    l1<<fi;
    //贷方
    fi = new QStandardItem(tr("贷方"));//index: 3
    l1<<fi;
    //余额方向
    fi = new QStandardItem(tr("方向"));//index: 4
    l1<<fi;

    //余额
    fi = new QStandardItem(tr("余额"));//index: 5
    l1<<fi;

    headerModel->appendRow(l1);
    l1.clear();
}

//生成三栏明细式表头
void ShowTZDialog::genThForThreeRail()
{
    //总共10个字段（在只有一种外币的情况下）
    QStandardItem* fi;  //第一级表头
    QStandardItem* si;  //第二级表头
    QList<QStandardItem*> l1,l2;//l1用于存放表头第一级，l2存放表头第二级

    fi = new QStandardItem(QString(tr("%1年")).arg(y));
    l1<<fi;
    si = new QStandardItem(tr("月 份")); //index:0
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();
    fi = new QStandardItem(tr("摘要"));   //index:1
    l1<<fi;
    fi = new QStandardItem(tr("汇率"));   //index:2
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i]));
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }

    //借方
    fi = new QStandardItem(tr("借方"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i]));  //index:3
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));                 //index:4
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    //贷方
    fi = new QStandardItem(tr("贷方"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i])); //index:5
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));                //index:6
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    //余额方向
    fi = new QStandardItem(tr("方向"));                //index:7
    l1<<fi;

    //余额
    fi = new QStandardItem(tr("余额"));
    l1<<fi;
    for(int i = 0; i < mts.count(); ++i){
        si = new QStandardItem(allMts.value(mts[i])); //index:8
        l2<<si;
        fi->appendColumn(l2);
        l2.clear();
    }
    si = new QStandardItem(tr("金额"));                //index:9
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    headerModel->appendRow(l1);
    l1.clear();
}

//生成三栏明细式格式表格数据
void ShowTZDialog::genDataForCommon()
{
    QList<TotalAccountData2*> datas;
    QHash<int,Double> preExtra;
    QHash<int,int> preExtraDir;
    QHash<int, Double> rates;

    if(!BusiUtil::getTotalAccount(y,sm,em,fid,datas,preExtra,preExtraDir,rates))
        return;

    ApStandardItem* item;
    QList<QStandardItem*> l;

    if(sm == 1)
        item = new ApStandardItem(tr("上年结转"));
    else
        item = new ApStandardItem(tr("上月结转"));
    l<<NULL<<item<<NULL<<NULL;
    item = new ApStandardItem(dirStr(datas[0]->dir));
    l<<item;
    item = new ApStandardItem(datas[0]->ev);
    l<<item;
    dataModel->appendRow(l);
    l.clear();

    Double sumjy = 0.0, sumdy = 0.0;
    for(int i = 1; i < datas.count(); ++i){
        //添加本月合计行
        item = new ApStandardItem(datas[i]->m);           //index：0  月份
        l<<item;
        item = new ApStandardItem(tr("本月合计"));         //index：1 摘要
        l<<item;
        item = new ApStandardItem(datas[i]->jv);         //index：2 借方
        sumjy += datas[i]->jv;
        l<<item;
        item = new ApStandardItem(datas[i]->dv);         //index：3 贷方
        sumdy += datas[i]->dv;
        l<<item;
        item = new ApStandardItem(dirStr(datas[i]->dir)); //index：4 余额方向
        l<<item;
        item = new ApStandardItem(datas[i]->ev);          //index：5 余额
        l<<item;
        dataModel->appendRow(l);
        l.clear();

        //添加本年累计行
        item = new ApStandardItem(datas[i]->m);
        l<<item;
        item = new ApStandardItem(tr("本年合计"));
        l<<item;
        item = new ApStandardItem(sumjy);
        l<<item;
        item = new ApStandardItem(sumdy);
        l<<item<<NULL<<NULL;
        dataModel->appendRow(l);
        l.clear();
    }
}

//生成三栏明细式表格数据
void ShowTZDialog::genDataForThreeRail()
{
//    QList<TotalAccountData*> datas;
//    QHash<int,double> preExtra;
//    QHash<int,int> preExtraDir;
//    QHash<int, double> rates;

//    ApStandardItem* item;
//    QList<QStandardItem*> l;

//    for(int i = 0; i < 10; ++i){
//        item = new ApStandardItem(QString("Item %1").arg(i));
//        l<<item;
//    }
//    dataModel->appendRow(l);
//    l.clear();

    QList<TotalAccountData2*> datas;
    QHash<int,Double> preExtra;
    QHash<int,int> preExtraDir;
    QHash<int, Double> rates;

    if(!BusiUtil::getTotalAccount(y,sm,em,fid,datas,preExtra,preExtraDir,rates))
        return;

    ApStandardItem* item;
    QList<QStandardItem*> l;

    if(sm == 1)
        item = new ApStandardItem(tr("上年结转"));
    else
        item = new ApStandardItem(tr("上月结转"));
    l<<NULL<<item;                                 //index:0,1（月份，摘要）
    for(int i = 0; i<mts.count(); ++i){            //index：2 汇率
        item = new ApStandardItem(rates.value(mts[i]));
        l<<item;
    }
    for(int i = 0; i < mts.count()*2+2; ++i)       //index：3，4，5，6（借方、贷方）
        l<<NULL;
    item = new ApStandardItem(dirStr(datas[0]->dir)); //index：7 方向
    l<<item;
    for(int i = 0; i < mts.count(); ++i){           //index：8 余额（外币）
        item = new ApStandardItem(datas[0]->evh.value(mts[i]));
        l<<item;
    }
    item = new ApStandardItem(datas[0]->ev);        //index：9 余额（金额）
    l<<item;
    dataModel->appendRow(l);
    l.clear();

    QHash<int,Double> sumjyh,sumdyh; //外币部分年累计
    Double sumjy = 0.0, sumdy = 0.0;     //金额部分年累计
    Double v;
    for(int i = 1; i < datas.count(); ++i){
        //添加本月合计行
        item = new ApStandardItem(datas[i]->m);           //index：0  月份
        l<<item;
        item = new ApStandardItem(tr("本月合计"));         //index：1 摘要
        l<<item;
        for(int j = 0; j < mts.count(); ++j){
            item = new ApStandardItem(rates.value(datas[i]->m*10+mts[j])); //index：2 汇率
            l<<item;
        }
        for(int j = 0; j < mts.count(); ++j){            //index：3 借方（外币）
            v = datas[i]->jvh.value(mts[j]);
            item = new ApStandardItem(v);
            l<<item;
            sumjyh[mts[j]] += v;
        }
        item = new ApStandardItem(datas[i]->jv);         //index：4 借方（金额）
        sumjy += datas[i]->jv;
        l<<item;
        for(int j = 0; j < mts.count(); ++j){            //index：5 贷方（外币）
            v = datas[i]->dvh.value(mts[j]);
            item = new ApStandardItem(v);
            l<<item;
            sumdyh[mts[j]] += v;
        }
        item = new ApStandardItem(datas[i]->dv);         //index：6 贷方（金额）
        sumdy += datas[i]->dv;
        l<<item;
        item = new ApStandardItem(dirStr(datas[i]->dir)); //index：7 余额方向
        l<<item;
        for(int j = 0; j < mts.count(); ++j){             //index：8 余额（外币）
            item = new ApStandardItem(datas[i]->evh.value(mts[j]));
            l<<item;
        }
        item = new ApStandardItem(datas[i]->ev);          //index：9 余额（金额）
        l<<item;

        dataModel->appendRow(l);
        l.clear();

        //添加本年累计行
        item = new ApStandardItem(datas[i]->m);      //0
        l<<item;
        item = new ApStandardItem(tr("本年合计"));    //1,2
        l<<item<<NULL;
        for(int j = 0; j < mts.count(); ++j){        //3
            item = new ApStandardItem(sumjyh.value(mts[j]));
            l<<item;
        }
        item = new ApStandardItem(sumjy);            //4
        l<<item;
        for(int j = 0; j < mts.count(); ++j){        //5
            item = new ApStandardItem(sumdyh.value(mts[j]));
            l<<item;
        }
        item = new ApStandardItem(sumdy);            //6
        l<<item;
        for(int j = 0; j < mts.count()+2; ++j)       //7,8,9
            l<<NULL;

        dataModel->appendRow(l);
        l.clear();
    }

}

void ShowTZDialog::printCommon(PrintTask task, QPrinter* printer)
{
    HierarchicalHeaderView* thv = new HierarchicalHeaderView(Qt::Horizontal);

    //创建打印模板实例
    QList<int> colw(colPrtWidths.value(curFormat));
    PrintTemplateTz* pt = new PrintTemplateTz(dataModel,thv,&colw);
    SubjectManager* sm = curAccount->getSubjectManager();
    pt->setMasteMt(allMts.value(RMB));
    pt->setSubName(tr("%1（%2）").arg(sm->getFstSubject(fid)->getName())
                   .arg(sm->getFstSubject(fid)->getCode()));
    pt->setAccountName(curAccount->getLName());
    pt->setCreator(curUser->getName());
    pt->setPrintDate(QDate::currentDate());

    //设置打印机的页边距和方向
    printer->setPageMargins(margins.left,margins.top,margins.right,margins.bottom,margins.unit);
    printer->setOrientation(pageOrientation);

    PreviewDialog* dlg = new PreviewDialog(pt,DETAILPAGE,printer);

    if(task == PREVIEW){
        //接收打印页面设置的修改
        if(dlg->exec() == QDialog::Accepted){
            for(int i = 0; i < colw.count(); ++i)
                colPrtWidths[curFormat][i] = colw[i];
            pageOrientation = printer->orientation();
            printer->getPageMargins(&margins.left,&margins.top,&margins.right,
                                    &margins.bottom,margins.unit);
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

void ShowTZDialog::on_actToPdf_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::ScreenResolution);
    printCommon(TOPDF, printer);
    delete printer;
}

void ShowTZDialog::on_actPrint_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::PrinterResolution);
    printCommon(TOPRINT, printer);
    delete printer;
}

void ShowTZDialog::on_actPreview_triggered()
{
    QPrinter* printer= new QPrinter(QPrinter::HighResolution);
    printCommon(PREVIEW, printer);
    delete printer;
}



