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
GdzcAdminDialog::GdzcAdminDialog(QByteArray* sinfo,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GdzcAdminDialog)
{
    ui->setupUi(this);

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
    BusiUtil::getIdByCode(gid,"1501");
    BusiUtil::getIdByCode(lid,"1502");
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
        ui->datBuy->setDate(curAccount->getStartTime());
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
    int y = curAccount->getEndTime().year();
    int m = curAccount->getEndTime().month();
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
    int y = curAccount->getEndTime().year();
    int m = curAccount->getEndTime().month();
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
    year = QString::number(curAccount->getEndTime().year());
    month = QString::number(curAccount->getEndTime().month());
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
    int y = curAccount->getCurSuite();
    QString t = curAccount->getSuiteName(y);
    int im,m;
    if(curAccount->getCurSuite() == curAccount->getEndSuite())
        im = curAccount->getEndTime().month();
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
    eed.setDate(curAccount->getEndTime());
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
DtfyAdminDialog::DtfyAdminDialog(QByteArray *sinfo, QSqlDatabase db, QWidget *parent) :
    db(db),QDialog(parent),ui(new Ui::DtfyAdminDialog)
{
    ui->setupUi(this);
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
    cashId = sm->getCashId();
    bankId = sm->getBankId();
    ui->cmbFsub->addItem("",0);
    ui->cmbFsub->addItem(sm->getFstSubName(cashId),cashId);
    ui->cmbFsub->addItem(sm->getFstSubName(bankId),bankId);
    QList<int> ids;
    QList<QString> names;
    BusiUtil::getSndSubInSpecFst(subCashId,ids,names);
    for(int i = 0; i < ids.count(); ++i)
        dsids[subCashId][ids[i]] = names[i];
    ids.clear();
    names.clear();
    BusiUtil::getSndSubInSpecFst(subBankId,ids,names);
    for(int i = 0; i < ids.count(); ++i)
        dsids[subBankId][ids[i]] = names[i];

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
        items<<new QStandardItem(sm->getFstSubName(ba->fid));
        items<<new QStandardItem(sm->getSndSubName(ba->sid));
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
    PzSetMgr* pzset = curAccount->getPzSet();
    pzset->crtDtfyTxPz();
}

//取消计提摊销凭证
void DtfyAdminDialog::on_actRepeal_triggered()
{
    PzSetMgr* pzset = curAccount->getPzSet();
    pzset->delDtfyPz();
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
    int y = curAccount->getCurSuite();
    QString t = curAccount->getSuiteName(y);
    int im,m;
    if(curAccount->getCurSuite() == curAccount->getEndSuite())
        im = curAccount->getEndTime().month();
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
    eed.setDate(curAccount->getEndTime());
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


////////////////////////////HappenSubSelDialog//////////////////////////////////////
HappenSubSelDialog::HappenSubSelDialog(int y, int m, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HappenSubSelDialog)
{
    ui->setupUi(this);
    cury = y;curm = m;

    QSqlQuery q;
    int id; QString name;

    //初始化一级科目组合框
    q.exec("select id,subName from FirSubjects");    
    ui->cmbSF->addItem(tr("所有"),0);
    ui->cmbEF->addItem(tr("所有"),0);
    while(q.next()){
        id = q.value(0).toInt();
        name = q.value(1).toString();
        ui->cmbSF->addItem(name,id);
        ui->cmbEF->addItem(name,id);
    }

    //初始化科目类型组合框
    q.exec("select code,name from FstSubClasses");
    ui->cmbSubCls->addItem(tr("所有"),0);
    while(q.next()){
        id = q.value(0).toInt();
        name = q.value(1).toString();
        ui->cmbSubCls->addItem(name,id);
    }

    //初始化月份范围调节器
    ui->spnSMon->setDefaultValue(m,tr("本"));
    ui->spnEMon->setDefaultValue(m,tr("本"));
    ui->spnSMon->setMinimum(curAccount->getSuiteFirstMonth(cury));
    ui->spnSMon->setMaximum(12);
    ui->spnSMon->setValue(curAccount->getCurMonth());
    ui->spnEMon->setMinimum(ui->spnSMon->minimum());
    ui->spnEMon->setMaximum(curAccount->getSuiteLastMonth(cury));
    ui->spnEMon->setValue(curAccount->getCurMonth());

    //设置余额框的验证器
    dv.setDecimals(2);
    ui->edtGreat->setValidator(&dv);
    ui->edtLess->setValidator(&dv);

    sfCom = new SubjectComplete;
    efCom = new SubjectComplete;
    ssCom = new SubjectComplete(SndSubject);
    esCom = new SubjectComplete(SndSubject);

    ui->cmbSF->setCompleter(sfCom);
    ui->cmbEF->setCompleter(efCom);
    ui->cmbSS->setCompleter(ssCom);
    ui->cmbES->setCompleter(esCom);

    connect(ui->cmbSF,SIGNAL(currentIndexChanged(int)),
            this,SLOT(startFstSubChanged(int)));
    connect(ui->cmbEF,SIGNAL(currentIndexChanged(int)),
            this,SLOT(endFstSubChanged(int)));

    //如果不连接此信号，则用户在使用鼠标从完成列表中选择一个项目时，不会触发QComboBox的当前索引改变信号
//    connect(sfCom,SIGNAL(activated(QModelIndex)),
//            this,SLOT(selItemInComplete(QModelIndex)));
//    connect(ssCom,SIGNAL(activated(QModelIndex)),
//            this,SLOT(selItemInComplete(QModelIndex)));
//    connect(efCom,SIGNAL(activated(QModelIndex)),
//            this,SLOT(selItemInComplete(QModelIndex)));
//    connect(esCom,SIGNAL(activated(QModelIndex)),
//            this,SLOT(selItemInComplete(QModelIndex)));


}

HappenSubSelDialog::~HappenSubSelDialog()
{
    delete ui;
}

//返回用户选择的要查看的科目范围，参数witch表示用户的选择模式
//1：选择所有科目，2：选择指定类型的科目，3：指定范围的科目（只有在此模式下，参数fids和sids才有效）
//参数fids表示选择的一级科目的id值列表或一级科目类别代码，
//sids表示选择的二级科目的id值列表（键为一级科目id，值为属于此一级科目的二级科目id列表）
void HappenSubSelDialog::getSubRange(int& witch, QList<int>& fids,
                                     QHash<int,QList<int> >& sids,
                                     double& gv, double& lv, bool& inc)
{
    if(!fids.empty())
        fids.clear();
    if(sids.empty())
        sids.clear();

    //科目类别和起始一级科目为所有
    if((ui->cmbSubCls->currentIndex() == 0) &&
            (ui->cmbSF->currentIndex() == 0)){
        witch = 1;

    }
    //选中了一种科目类别
    else if(ui->cmbSubCls->currentIndex() > 0){
        witch =2;
        int cls = ui->cmbSubCls->itemData(ui->cmbSubCls->currentIndex()).toInt();
        fids<<cls;
    }
    else{
        witch = 3;
        int sfid = ui->cmbSF->itemData(ui->cmbSF->currentIndex()).toInt();
        int ssid = ui->cmbSS->itemData(ui->cmbSS->currentIndex()).toInt();
        int efid = ui->cmbEF->itemData(ui->cmbEF->currentIndex()).toInt();
        int esid = ui->cmbES->itemData(ui->cmbES->currentIndex()).toInt();
        BusiUtil::getSubRange(sfid,ssid,efid,esid,fids,sids);
    }

    if(!ui->edtGreat->text().isEmpty())
        gv = ui->edtGreat->text().toDouble();
    if(!ui->edtLess->text().isEmpty())
        lv = ui->edtLess->text().toDouble();
    inc = ui->chkNonInstat->isChecked();
}

//返回用户选择的凭证时间范围（当年月份数）
void HappenSubSelDialog::getTimeRange(int& sm, int& em)
{
    if((ui->spnSMon->value() == 0) &&
       (ui->spnEMon->value() == 0)){
        sm = curm;em = curm;
    }
    else{
        sm = ui->spnSMon->value();
        em = ui->spnEMon->value();
    }
}

//返回用户指定的余额限定值（返回true，表示用户至少指定了一个余额限定值）
bool HappenSubSelDialog::getExtraLimit(double& great, double& less)
{
    bool r = false;
    if(!ui->edtGreat->text().isEmpty()){
        great = ui->edtGreat->text().toDouble();
        r = true;
    }
    if(!ui->edtLess->text().isEmpty()){
        less = ui->edtLess->text().toDouble();
        r = true;
    }
    return r;
}

//是否包含未入账凭证
bool HappenSubSelDialog::isIncDotInstat()
{
    return ui->chkNonInstat->isChecked();
}

//是否不显示未发生项
bool HappenSubSelDialog::isViewDotHapp()
{
    return ui->chkNonHapp->isChecked();
}

//当用户选择了开始一级科目时，启用开始二级科目和结束一级科目
void HappenSubSelDialog::startFstSubChanged(int index)
{
    if(index > 0){
        ui->cmbSS->setEnabled(true);
        ui->cmbEF->setEnabled(true);
    }
    else{
        ui->cmbSS->setEnabled(false);
        ui->cmbEF->setEnabled(false);
    }
    //装载当前选择的一级科目下所有的明细科目
    int id = ui->cmbSF->itemData(index).toInt();
    QString s = QString("select FSAgent.id,SecSubjects.subName"
                        " from FSAgent join SecSubjects on "
                        "SecSubjects.id = FSAgent.sid where fid = %1").arg(id);
    ui->cmbSS->clear();
    ui->cmbSS->addItem(tr("所有"),0);
    QSqlQuery q;
    q.exec(s);
    while(q.next())
        ui->cmbSS->addItem(q.value(1).toString(),q.value(0).toInt());
    ssCom->setPid(id);
}

//当用户选择了结束一级科目改变时，启用结束二级科目的组合框
void HappenSubSelDialog::endFstSubChanged(int index)
{
    if(index > 0)
        ui->cmbES->setEnabled(true);
    else
        ui->cmbES->setEnabled(false);
    //装载当前选择的一级科目下所有的明细科目
    int id = ui->cmbEF->itemData(index).toInt();
    QString s = QString("select FSAgent.id,SecSubjects.subName"
                        " from FSAgent join SecSubjects on "
                        "SecSubjects.id = FSAgent.sid where fid = %1").arg(id);
    ui->cmbES->clear();
    ui->cmbES->addItem(tr("所有"),0);
    QSqlQuery q;
    q.exec(s);
    while(q.next())
        ui->cmbES->addItem(q.value(1).toString(),q.value(0).toInt());
    esCom->setPid(id);
}

//此功能已经在完成器内部实现
//在科目选取框的完成器中选定一个项目时
//void HappenSubSelDialog::selItemInComplete(const QModelIndex &index)
//{
//    const QAbstractItemModel* model = index.model();
//    int id = model->data(model->index(index.row(),2)).toInt();
//    SubjectComplete* com = static_cast<SubjectComplete*>(sender());
//    int idx;
//    if(com == sfCom){
//        idx = ui->cmbSF->findData(id);
//        ui->cmbSF->setCurrentIndex(idx);
//    }
//    else if(com == ssCom){
//        idx = ui->cmbSS->findData(id);
//        ui->cmbSS->setCurrentIndex(idx);
//    }
//    else if(com == efCom){
//        idx = ui->cmbEF->findData(id);
//        ui->cmbEF->setCurrentIndex(idx);
//    }
//    else{
//        idx = ui->cmbES->findData(id);
//        ui->cmbES->setCurrentIndex(idx);
//    }
//}


//当用户选择了科目类型时，启用禁用开始一级科目的组合框
void HappenSubSelDialog::on_cmbSubCls_currentIndexChanged(int index)
{
    if(index == 0){
        ui->cmbSF->setEnabled(true);
        if(ui->cmbSF->currentIndex() == 0){
            ui->cmbSS->setEnabled(false);
            ui->cmbEF->setEnabled(false);
            ui->cmbES->setEnabled(false);
        }
        else{
            ui->cmbSS->setEnabled(true);
            ui->cmbEF->setEnabled(true);
            if(ui->cmbEF->currentIndex() == 0)
                ui->cmbES->setEnabled(false);
            else
                ui->cmbES->setEnabled(true);
        }
    }
    else{
        ui->cmbSF->setEnabled(false);
        ui->cmbSS->setEnabled(false);
        ui->cmbEF->setEnabled(false);
        ui->cmbES->setEnabled(false);
    }

}

void HappenSubSelDialog::on_btnOk_clicked()
{
    if(ui->spnSMon->value() > ui->spnEMon->value()){
        QMessageBox::warning(this,tr("错误提示"), tr("起始月份设置不正确！！"));
        return;
    }
    else
        accept();
}


//////////////////////////////ShowTZDialog///////////////////////////////////////
ShowTZDialog::ShowTZDialog(int y, int m, QByteArray* sinfo, QWidget *parent) : QDialog(parent),
    ui(new Ui::ShowTZDialog),y(y),m(m)
{
    QSqlQuery q;

    ui->setupUi(this);

    headerModel = NULL;
    dataModel = NULL;
    imodel = NULL;
    hv = NULL;

    //初始化一级科目组合框
    q.exec("select id,subName from FirSubjects where isView=1");
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
    hv->setClickable(true);
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
    dataModel = new QStandardItemModel;

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

    if(imodel){
        delete imodel;
    }
    imodel = new ProxyModelWithHeaderModels;

    imodel->setModel(dataModel);
    imodel->setHorizontalHeaderModel(headerModel);

    ui->tview->setModel(imodel);

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
    QList<TotalAccountData*> datas;
    QHash<int,double> preExtra;
    QHash<int,int> preExtraDir;
    QHash<int, double> rates;

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

    double sumjy = 0, sumdy = 0;
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

    QList<TotalAccountData*> datas;
    QHash<int,double> preExtra;
    QHash<int,int> preExtraDir;
    QHash<int, double> rates;

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

    QHash<int,double> sumjyh,sumdyh; //外币部分年累计
    double sumjy = 0, sumdy = 0;     //金额部分年累计
    double v;
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
    ProxyModelWithHeaderModels* m = new ProxyModelWithHeaderModels;
    m->setModel(dataModel);
    m->setHorizontalHeaderModel(headerModel);

    //创建打印模板实例
    QList<int> colw(colPrtWidths.value(curFormat));
    PrintTemplateTz* pt = new PrintTemplateTz(m,thv,&colw);
    SubjectManager* sm = curAccount->getSubjectManager();
    pt->setMasteMt(allMts.value(RMB));
    pt->setSubName(tr("%1（%2）").arg(sm->getFstSubName(fid))
                   .arg(sm->getFstSubCode(fid)));
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
    delete m;
    //delete pt;
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



////////////////////////////////ShowDZDialog/////////////////////////////////////////////
ShowDZDialog::ShowDZDialog(QByteArray* sinfo, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShowDZDialog)
{
    ui->setupUi(this);
    ui->pbr->setVisible(false);
    tfid = -1;tsid = -1;tmt = -1;
    mt = RMB;
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

    mtLst = allMts.keys();
    qSort(mtLst.begin(),mtLst.end()); //为了使人民币总是第一个

    //初始化货币代码列表，并使它们以一致的顺序显示
    mts = allMts.keys();
    mts.removeOne(RMB);
    qSort(mts.begin(),mts.end());
    ui->cmbMt->addItem(tr("所有"),ALLMT);
    ui->cmbMt->addItem(allMts.value(RMB),RMB);
    for(int i = 0; i < mts.count(); ++i)
        ui->cmbMt->addItem(allMts.value(mts[i]),mts[i]);

    ui->btnPrint->addAction(ui->actPrint);
    ui->btnPrint->addAction(ui->actPreview);
    ui->btnPrint->addAction(ui->actToPdf);

    setState(sinfo);
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));
}

ShowDZDialog::~ShowDZDialog()
{
    //delete fcom;  //会崩溃
    //delete scom;
    delete ui;    
}

//设置要显示的科目范围，参数witch为选择模式，
//1：选择所有科目，2：选择指定类型的科目，3：指定范围的科目（只有在此模式下，参数fids和sids才有效）
//参数fids表示选择的一级科目的id值列表或一级科目类别代码，
//sids表示选择的二级科目的id值列表（键为一级科目id，值为属于此一级科目的二级科目id列表）
void ShowDZDialog::setSubRange(int witch, QList<int> fids,
                               QHash<int,QList<int> > sids,
                               double gv, double lv, bool inc)
{
    int fid; QString name;

    this->witch = witch;
    this->fids = fids;
    this->sids = sids;
    this->gv = gv;
    this->lv = lv;
    this->inc = inc;

    //如果选择所有主目或选择某一类别的主目或选择的主目范围多于一个，则添加“所有”选项
    if((witch == 1) || (witch == 2) ||
       ((witch == 3) && fids.count() > 1))
        ui->cmbFsub->addItem(tr("所有"),0);

    QSqlQuery q;
    QString s;
    //如果选择所有主目或某类别主目，则要加载主目的id和名称到fids，及其主目所属的子目的id和名称
    if((witch == 1) || (witch == 2)){
        if(witch == 1)
            s = "select id,subName from FirSubjects where isView = 1";
        else
            s = QString("select id,subName from FirSubjects where (isView = 1) and "
                        "(belongTo = %1)").arg(fids[0]);
        q.exec(s);
        this->fids.clear(); this->sids.clear();
        while(q.next()){
            //int id; QString name;
            fid = q.value(0).toInt();
            name = q.value(1).toString();
            ui->cmbFsub->addItem(name, fid);
            this->fids<<fid;
            BusiUtil::getSndSubInSpecFst(fid,this->sids[fid],sNames[fid]);
        }
    }
    else{
        s = "select id,subName from FirSubjects where isView = 1";
        q.exec(s);        
        while(q.next()){
            fid = q.value(0).toInt();
            name = q.value(1).toString();
            if(fids.contains(fid)){
                ui->cmbFsub->addItem(name, fid);
                BusiUtil::getSndSubInSpecFst(fid,this->sids[fid],sNames[fid]);
            }
        }
        //ui->cmbFsub->setCurrentIndex(0);
    }
}

//设置日期范围
void ShowDZDialog::setDateRange(int sm, int em, int y)
{
    cury = y;
    this->sm = sm;
    this->em = em;

    QDate d(y,sm,1);
    QString sd = d.toString(Qt::ISODate);
    sd.chop(3);
    d = QDate(y,em,1);
    d = QDate(y,em,d.daysInMonth());
    QString ed = d.toString(Qt::ISODate);
    ed.chop(3);
    ui->lblDate->setText(tr("%1——%2").arg(sd).arg(ed));

    //isInit = false;

    //建立三个组合框的信号连接
    connect(ui->cmbFsub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelFstSub(int)));
    connect(ui->cmbSsub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));

    if(ui->cmbFsub->count() == 1)
        onSelFstSub(0);
    else if(ui->cmbFsub->count() > 1)
        ui->cmbFsub->setCurrentIndex(1);

    //ui->pbr->setVisible(false);
}

//设置视图的内部状态
void ShowDZDialog::setState(QByteArray* info)
{
    if(info == NULL){
        //屏幕视图上的表格列宽
        colWidths[CASHDAILY]<<50<<50<<50<<300<<0<<100<<100<<50<<100<<0<<0;        
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
        colPrtWidths[CASHDAILY]<<50<<50<<50<<300<<0<<100<<100<<50<<100<<0<<0;
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
QByteArray* ShowDZDialog::getState()
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
    for(int i = 0; i < 11; ++i){
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

//用户选择一个总账科目（更新二级科目选择列表为该科目下的所有子目）
void ShowDZDialog::onSelFstSub(int index)
{
    fid = ui->cmbFsub->itemData(index).toInt();
    scom->setPid(fid);

    disconnect(ui->cmbSsub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    disconnect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));

    QList<QString> names;
    if((witch == 1) || (witch == 2)){
        QList<int> ids;
        //BusiUtil::getSndSubInSpecFst(fid,ids,names);

        //初始化可选的明细科目列表
        ui->cmbSsub->clear();
        if(fid == subCashId){ //现金科目下，只有人民币子目，且币种为人民币
            ui->cmbSsub->addItem(tr("人民币"),subCashRmbId);
            ui->cmbMt->clear();
            ui->cmbMt->addItem(tr("人民币"),RMB);
        }
        else{//如果该主目只有一个子目
            if(ids.count() == 1){
                ui->cmbSsub->addItem(sNames.value(fid)[0], sids.value(fid)[0]);
            }
            else{
                ui->cmbSsub->addItem(tr("所有"), 0);
                for(int i = 0; i < sids.value(fid).count(); ++i)
                    ui->cmbSsub->addItem(sNames.value(fid)[i], sids.value(fid)[i]);
            }
            //装载属于该总账科目的所有子目id
            //if((fid != 0) && sids.value(fid).empty())
            //    sids[fid]<<ids;

            //初始化可选的币种列表
            ui->cmbMt->clear();
            //如果主目是要按币种分别核算的科目或选择所有科目，则要添加所有币种
            if(BusiUtil::isAccMt(fid) || (fid == 0)){
                ui->cmbMt->addItem(tr("所有"), ALLMT);
                QList<int> mts = allMts.keys();
                qSort(mts.begin(),mts.end());
                for(int i = 0; i < mts.count(); ++i)
                    ui->cmbMt->addItem(allMts.value(mts[i]), mts[i]);
            }
            else{
                ui->cmbMt->addItem(tr("人民币"),RMB);
            }
        }

    }
    else{  //范围选择模式
        //if(fid != 0)
        //    BusiUtil::getSubSetNameInSpecFst(fid,sids.value(fid),names);
        ui->cmbMt->clear();
        ui->cmbSsub->clear();
        if(fid == subCashId){  //如果是现金科目
            ui->cmbSsub->addItem(tr("人民币"),subCashRmbId);            
            ui->cmbMt->addItem(tr("人民币"),RMB);
        }
        else{
            if(sids.value(fid).count() == 1) //如果该主目只有一个子目
                ui->cmbSsub->addItem(sNames.value(fid)[0], sids.value(fid)[0]);
            else{  //否者加载所有选择的子目
                ui->cmbSsub->addItem(tr("所有"), 0);
                for(int i = 0; i < sids.value(fid).count(); ++i)
                    ui->cmbSsub->addItem(allSndSubs.value(sids.value(fid)[i]),
                                         sids.value(fid)[i]);
            }

            //初始化可选的币种列表
            ui->cmbMt->clear();

            if(BusiUtil::isAccMt(fid) || (fid == 0)){  //如果主目是要按币种分别核算的科目，则要添加所有币种
                ui->cmbMt->addItem(tr("所有"), ALLMT);
                QList<int> mts = allMts.keys();
                qSort(mts.begin(),mts.end());
                for(int i = 0; i < mts.count(); ++i)
                    ui->cmbMt->addItem(allMts.value(mts[i]), mts[i]);
            }
            else{
                ui->cmbMt->addItem(tr("人民币"),RMB);
            }
        }
    }

    ui->cmbSsub->setCurrentIndex(0);
    sid = ui->cmbSsub->itemData(0).toInt();
    ui->cmbMt->setCurrentIndex(0); //所有
    mt = ui->cmbMt->itemData(0).toInt();

    connect(ui->cmbSsub,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelSndSub(int)));
    connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));

    refreshTalbe();
}

//用户选择一个明细科目
void ShowDZDialog::onSelSndSub(int index)
{
    sid = ui->cmbSsub->itemData(index).toInt();    
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
    //如果是银行子科目，则根据子科目名的币名后缀,自动设置对应币种
    if(fid == subBankId){
        disconnect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));
        ui->cmbMt->clear();
        if(sid != 0){
            QString name = ui->cmbSsub->currentText();
            int idx = name.indexOf('-');
            name = name.right(name.count()-idx-1);
            QHashIterator<int,QString> it(allMts);
            while(it.hasNext()){
                it.next();
                if(name == it.value()){
                    ui->cmbMt->addItem(it.value(),it.key());
                    mt = it.key();
                    break;
                }
            }
        }
        else{
            ui->cmbMt->addItem(tr("所有"), 0);
            QList<int> mts = allMts.keys();
            qSort(mts.begin(),mts.end());
            for(int i = 0; i < mts.count(); ++i)
                ui->cmbMt->addItem(allMts.value(mts[i]),mts[i]);
        }

        connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));
    }
//    //如果是其他科目，则只选人民币
//    else{
//        ui->cmbMt->addItem(allMts.value(RMB),RMB);
//        mt = RMB;
//    }

//    connect(ui->cmbMt,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelMt(int)));
    refreshTalbe();
}

//用户选择币种
void ShowDZDialog::onSelMt(int index)
{
    mt = ui->cmbMt->itemData(index).toInt();
    refreshTalbe();
}

//转到该凭证
void ShowDZDialog::moveTo()
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
void ShowDZDialog::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    colWidths[tf][logicalIndex] = newSize;
}


//更新明细账表格数据（生成页面数据）
//参数 pageNum = 0时表示刷新视图的表格内容，其他大于0的值，表示请求刷新指定打印页的表格内容
void ShowDZDialog::refreshTalbe()
{
    disconnect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));

    //当对象处于初始化状态时，不允许调用refreshTable方法
    if((tfid == fid) && (tsid == sid) && tmt == mt)
        return;

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
    if(!BusiUtil::getDailyAccount2(cury,sm,em,fid,sid,mt,prev,preDir,datas,
                                  preExtra,preExtraR,preExtraDir,rates,fids,sids,gv,lv,inc))
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


//生成现金日记账表头
void ShowDZDialog::genThForCash(QStandardItemModel* model)
{
    //总共11个字段，其中3个隐藏了（序号分别为3、9、10）
    QStandardItem* fi;  //第一级表头
    QStandardItem* si;  //第二级表头
    QList<QStandardItem*> l1,l2;//l1用于存放表头第一级，l2存放表头第二级

    fi = new QStandardItem(QString(tr("%1年")).arg(cury));
    l1<<fi;
    si = new QStandardItem(tr("月"));
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();
    si = new QStandardItem(tr("日"));
    l2<<si;
    fi->appendColumn(l2);
    l2.clear();

    fi = new QStandardItem(tr("凭证号"));
    l1<<fi;
    fi = new QStandardItem(tr("摘要"));
    l1<<fi;
    fi = new QStandardItem(tr("对方科目"));
    l1<<fi;

    //viewCols = 4;//前面4列必定有

    //借方
    fi = new QStandardItem(tr("借方金额"));
    l1<<fi;
    //贷方
    fi = new QStandardItem(tr("贷方金额"));
    l1<<fi;
    //余额方向
    fi = new QStandardItem(tr("方向"));
    l1<<fi;
    //viewCols++;

    //余额
    fi = new QStandardItem(tr("余额"));
    l1<<fi;
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

//生成银行人民币日记账表头
void ShowDZDialog::genThForBankRmb(QStandardItemModel* model)
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
void ShowDZDialog::genThForBankWb(QStandardItemModel* model)
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
        si = new QStandardItem(allMts.value(mts[i]));
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
        si = new QStandardItem(allMts.value(mts[i]));
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
        si = new QStandardItem(allMts.value(mts[i]));
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
void ShowDZDialog::genThForDetails(QStandardItemModel* model)
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
void ShowDZDialog::genThForWai(QStandardItemModel* model)
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
        si = new QStandardItem(allMts.value(mts[i])); //5
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
        si = new QStandardItem(allMts.value(mts[i])); //7
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
        si = new QStandardItem(allMts.value(mts[i])); //10
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
//void ShowDZDialog::getDatas(int y, int sm, int em, int fid, int sid, int mt,
//              QList<DailyAccountData*>& datas,
//              QHash<int,double> preExtra,
//              QHash<int,double> preExtraDir,
//              QHash<int, double> rates)
//{
//    if(!BusiUtil::getDailyAccount(y,sm,em,fid,sid,mt,datas,preExtra,preExtraDir,rates))
//        return;
//}

//生成现金日记账数据（返回值为总共生成的行数）
int ShowDZDialog::genDataForCash(QList<DailyAccountData2 *> datas,
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
    l<<NULL<<NULL<<NULL;
    if(sm == 1)
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
    int om = datas[0]->m;  //用以判定是否跨月（如果跨月，则需要插入本月合计行）
    for(int i = 0; i < datas.count(); ++i){
        //插入本月合计行和累计行
        if(om != datas[i]->m){
            item = new ApStandardItem(om); //0：月
            l<<item<<NULL<<NULL;
            item = new ApStandardItem(tr("本月合计"));
            l<<item<<NULL;
            item = new ApStandardItem(jmsums.getv());  //5：借方
            l<<item;
            item = new ApStandardItem(dmsums.getv());  //6：贷方
            l<<item;
            item = new ApStandardItem(dirStr(datas[i-1]->dir));        //7：余额方向
            l<<item;
            item = new ApStandardItem(datas[i-1]->etm.getv());    //8：余额
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

        om = datas[i]->m;
        item = new ApStandardItem(datas[i]->m); //0：月
        l<<item;
        item = new ApStandardItem(datas[i]->d); //1：日
        l<<item;
        item = new ApStandardItem(datas[i]->pzNum);              //2：凭证号
        l<<item;        
        item = new ApStandardItem(datas[i]->summary,Qt::AlignLeft|Qt::AlignVCenter);            //3：摘要
        l<<item;
        item = new ApStandardItem;                               //4：对方科目
        l<<item;
        if(datas[i]->dh == DIR_J){
            jmsums += datas[i]->v;
            jysums += datas[i]->v;
            item = new ApStandardItem(datas[i]->v.getv());  //5：借方
            l<<item;
            item = new ApStandardItem;               //6：贷方
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
        item = new ApStandardItem(dirStr(datas[i]->dir)); //7：余额方向
        l<<item;
        item = new ApStandardItem(datas[i]->etm.getv());    //8：余额
        l<<item;
        //添加两个隐藏列（业务活动所属凭证id和业务活动本身的id）
        item = new ApStandardItem(datas[i]->pid);//9
        l<<item;
        item = new ApStandardItem(datas[i]->bid);//10
        l<<item;
        //mo->appendRow(l);
        pdatas<<l;
        rows++;
        l.clear();
    }    

    //插入末尾的本月累计和本年累计行
    item = new ApStandardItem(QString::number(em)); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本月合计"));
    l<<item<<NULL;
    item = new ApStandardItem(jmsums.getv());  //5：借方
    l<<item;
    item = new ApStandardItem(dmsums.getv());  //6：贷方
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));
        l<<item;
        item = new ApStandardItem(preExtra.value(RMB).getv());
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir)); //7：余额方向
        l<<item;
        item = new ApStandardItem(datas[datas.count()-1]->etm.getv());    //8：余额
        l<<item;
    }
    l<<NULL<<NULL;
    //mo->appendRow(l);
    pdatas<<l;
    rows++;
    l.clear();

    item = new ApStandardItem(em); //0：月
    l<<item<<NULL<<NULL;
    item = new ApStandardItem(tr("本年累计"));
    l<<item<<NULL;
    item = new ApStandardItem(jysums);  //5：借方
    l<<item;
    item = new ApStandardItem(dysums);  //6：贷方
    l<<item;
    if(datas.empty()){
        item = new ApStandardItem(dirStr(preExtraDir.value(RMB)));
        l<<item;
        item = new ApStandardItem(preExtra.value(RMB));
        l<<item;
    }
    else{
        item = new ApStandardItem(dirStr(datas[datas.count()-1]->dir)); //7：余额方向
        l<<item;
        item = new ApStandardItem(datas[datas.count()-1]->etm);    //8：余额
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
int ShowDZDialog::genDataForBankRMB(QList<DailyAccountData2*> datas,
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
    if(sm == 1)
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
    item = new ApStandardItem(em); //0：月
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

    item = new ApStandardItem(em); //0：月
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
int ShowDZDialog::genDataForBankWb(QList<DailyAccountData2 *> datas,
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
    if(sm == 1)
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
                if(mt == mts[j]){
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
                if(mt == mts[j]){
                    item = new ApStandardItem(dwmsums.value(mts[j]));  //9：贷方（外币）
                    l<<item;
                    dwmsums[mts[j]] = 0;
                }
                else
                    l<<NULL;
            }
            item = new ApStandardItem(dmsums);  //10：贷方（金额）
            l<<item;
            if(mt == ALLMT)
                item = new ApStandardItem(dirStr(datas[i-1]->dir)); //11：余额方向
            else
                item = new ApStandardItem(dirStr(datas[i-1]->dirs.value(mt)));
            l<<item;            
            for(int j = 0; j < mts.count(); ++j){
                if(mt == datas[i]->mt){
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
    item = new ApStandardItem(em); //0：月
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

    item = new ApStandardItem(em); //0：月
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
int ShowDZDialog::genDataForDetails(QList<DailyAccountData2*> datas,
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
    if(sm == 1)
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
    item = new ApStandardItem(QString::number(em)); //0：月
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

    item = new ApStandardItem(em); //0：月
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
int ShowDZDialog::genDataForDetWai(QList<DailyAccountData2 *> datas,
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
    if(sm == 1)
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
    item = new ApStandardItem(em); //0：月
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

    item = new ApStandardItem(em); //0：月
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
void ShowDZDialog::paging(int rowsInTable, int& pageNum)
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
    if(fid == 0)
        fids = this->fids;//即在打开明细视图前你指定的一级科目范围
    else
        fids<<fid;
    if(mt == ALLMT)
        mts = allMts.keys();
    else
        mts<<mt;

    //2、在一个3重循环内，逐个提取每项组合的发生项，并扩展为表格行数据至pdatas中
    //（即添加期初项、在每月的末尾添加本月合计和本年累计项）
    QList<DailyAccountData2*> datas;
    QHash<int,Double> preExtra;
	QHash<int,Double> preExtraR;
    QHash<int,int> preExtraDir;
    QHash<int,Double>rates;
    int rows;  //每个组合产生的行数
    for(int i = 0; i < fids.count(); ++i){
        sids.clear();
        if(sid == 0)
            sids = this->sids.value(fids[i]);//即在打开明细视图前你指定的某个一级科目的二级科目范围
        else
            sids<<sid;
        for(int j = 0; j < sids.count(); ++j)
            for(int k = 0; k < mts.count(); ++k){
                //获取原始的发生项数据
                Double prev;
                int preDir;
                datas.clear();
                if(!BusiUtil::getDailyAccount2(cury,sm,em,fids[i],sids[j],mts[k],
                                              prev,preDir,datas,preExtra,preExtraR,
                                              preExtraDir,rates))
                    return;

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
void ShowDZDialog::renPageData(int pageNum, QList<int>*& colWidths, QStandardItemModel& pdModel,
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
        s = tr("%1日记账").arg(sm->getFstSubName(pfids[pageNum-1]));
    else
        s = tr("%1明细账").arg(sm->getFstSubName(pfids[pageNum-1]));;
    pt->setTitle(s);
    pt->setPageNum(subPageNum.value(pageNum));
    //pt->setMasteMt(allMts.value(RMB));
    //pt->setDateRange(cury,sm,em);
    //SubjectManager* sm = curAccount->getSubjectManager();
    s = tr("%1（%2）——（%3）")
            .arg(sm->getFstSubName(pfids[pageNum-1]))
            .arg(sm->getFstSubCode(pfids[pageNum-1]))
            .arg(sm->getSndSubLName(psids[pageNum-1]));
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
//void ShowDZDialog::priorPaging(bool out, int pages)
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
void ShowDZDialog::printCommon(QPrinter* printer)
{
    HierarchicalHeaderView* thv = new HierarchicalHeaderView(Qt::Horizontal);
    ProxyModelWithHeaderModels* m = new ProxyModelWithHeaderModels;    
    SubjectManager* smg = curAccount->getSubjectManager();

    if((fid == 0) || (sid == 0) || (mt == 0)){
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
    if((fid != 0) && (sid != 0) && (mt != 0)){
        QString s;
        if((fid == subCashId) || (fid == subBankId))
            s = tr("%1日记账").arg(smg->getFstSubName(fid));
        else
            s = tr("%1明细账").arg(smg->getFstSubName(fid));
        pt->setTitle(s);
        pt->setMasteMt(allMts.value(RMB));
        pt->setDateRange(cury,sm,em);
        s = tr("%1（%2）——（%3）").arg(smg->getFstSubName(fid))
                .arg(smg->getFstSubCode(fid))
                .arg(smg->getSndSubLName(sid));
        pt->setSubName(s);
        pt->setAccountName(curAccount->getLName());
        pt->setCreator(curUser->getName());
        pt->setPrintDate(QDate::currentDate());

    }
    //如果由明细视图来负责分页处理，则要进行占位处理，否则，在生成页面数据后，
    //打印预览正常，但打印到文件或打印机则截短了字符串的长度到原始长度
    else{
        pt->setTitle("          ");
        pt->setMasteMt(allMts.value(RMB));
        pt->setDateRange(cury,sm,em);
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
    if((fid == 0) || (sid == 0) || (mt == 0)){
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
void ShowDZDialog::on_actPrint_triggered()
{
    curPrintTask = TOPRINT;
    QPrinter* printer= new QPrinter(QPrinter::PrinterResolution);
    printCommon(printer);
    delete printer;
    delete preview;
    preview = NULL;
}

//打印预览
void ShowDZDialog::on_actPreview_triggered()
{
    curPrintTask = PREVIEW;
    QPrinter* printer= new QPrinter(QPrinter::HighResolution);
    printCommon(printer);
    delete printer;
    delete preview;
    preview = NULL;
}

//打印到PDF文件
void ShowDZDialog::on_actToPdf_triggered()
{
    curPrintTask = TOPDF;
    QPrinter* printer= new QPrinter(QPrinter::ScreenResolution);
    printCommon(printer);
    delete printer;
    delete preview;
    preview = NULL;
}

//void ShowDZDialog::on_btnClose_clicked()
//{
//    emit closeWidget();
//}


ShowDZDialog::TableFormat ShowDZDialog::decideTableFormat(int fid,int sid, int mt)
{
    //判定要采用的表格格式
    TableFormat tf;
    if(fid == 0)
        tf = THREERAIL;
    else if(fid == subCashId)
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


///////////////////////////HistoryPzDialog/////////////////////////////////////////////////
HistoryPzDialog::HistoryPzDialog(int pzId, int bid, QByteArray* sinfo, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HistoryPzDialog)
{
    ui->setupUi(this);
    this->pzId = pzId;
    this->bid = bid;

    QHashIterator<PzState,QString> ip(pzStates);
    while(ip.hasNext()){
        ip.next();
        PzStates.insert(ip.key(),ip.value());
    }
    ui->lblState->setMap(PzStates);

    QHashIterator<int,User*> it(allUsers);
    while(it.hasNext()){
        it.next();
        userNames[it.key()] = it.value()->getName();
    }
    ui->lblRUser->setMap(userNames);
    ui->lblVUser->setMap(userNames);
    ui->lblBUser->setMap(userNames);

    bm = new QStandardItemModel;
    setState(sinfo);
    updateContent();
    connect(ui->tview->horizontalHeader(),SIGNAL(sectionResized(int,int,int)),
            this,SLOT(colWidthChanged(int,int,int)));
}

HistoryPzDialog::~HistoryPzDialog()
{
    delete ui;
}

//设置当前显示的凭证
void HistoryPzDialog::setPz(int pzId, int bid)
{
    this->pzId = pzId;
    this->bid = bid;
    updateContent();
}

void HistoryPzDialog::setState(QByteArray* info)
{
    if(info == NULL){
        colWidths<<300<<100<<100<<50<<200<<200;
    }
    else{
        QBuffer bf(info);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        qint8 i8;
        qint16 i16;

        in>>i8;
        for(int i = 0; i < i8; ++i){
            in>>i16;
            colWidths<<i16;
        }
        bf.close();
    }
}

QByteArray* HistoryPzDialog::getState()
{
    QByteArray* info = new QByteArray;
    QBuffer bf(info);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 i8;
    qint16 i16;

    //显示业务活动的表格，固定为6列
    i8 = 6;
    out<<i8;
    for(int i = 0; i < 6; ++i){
        i16 = colWidths[i];
        out<<i16;
    }
    bf.close();
    return info;
}

//当用户改变了表格的列宽时，保存到内部的状态列表中
void HistoryPzDialog::colWidthChanged(int logicalIndex, int oldSize, int newSize)
{
    colWidths[logicalIndex] = newSize;
}

//更新凭证内容
void HistoryPzDialog::updateContent()
{
    QSqlQuery q;
    QString s;
    s = QString("select * from PingZhengs where id = %1").arg(pzId);
    if(!q.exec(s) || !q.first()){
        //触发信号，向主窗口报告错误
        return;
    }
    ui->lblPzNum->setText(QString::number(q.value(PZ_NUMBER).toInt()));
    ui->lblDate->setText(q.value(PZ_DATE).toString());
    ui->lblEncNum->setText(QString::number(q.value(PZ_ENCNUM).toInt()));
    ui->lblState->setValue(q.value(PZ_PZSTATE).toInt());
    ui->lblJSum->setText(QString::number(q.value(PZ_JSUM).toDouble(),'f',2));
    ui->lblDSum->setText(QString::number(q.value(PZ_DSUM).toDouble(),'f',2));
    ui->lblRUser->setValue(q.value(PZ_RUSER).toInt());
    ui->lblVUser->setValue(q.value(PZ_VUSER).toInt());
    ui->lblBUser->setValue(q.value(PZ_BUSER).toInt());

    bm->clear();
    SubjectManager* sm = curAccount->getSubjectManager();
    QList<BusiActionData*> datas;
    BusiUtil::getActionsInPz(pzId,datas);
    QList<QStandardItem*> l;
    ApStandardItem* item;
    int row;
    for(int i = 0; i < datas.count(); ++i){
        if(bid == datas[i]->id)
            row = i;
        item = new ApStandardItem(datas[i]->summary,Qt::AlignLeft|Qt::AlignVCenter);
        l<<item;
        item = new ApStandardItem(sm->getFstSubName(datas[i]->fid));
        l<<item;
        item = new ApStandardItem(sm->getSndSubName(datas[i]->sid));
        l<<item;
        item = new ApStandardItem(allMts.value(datas[i]->mt));
        l<<item;
        if(datas[i]->dir == DIR_J){
            item = new ApStandardItem(datas[i]->v);
            l<<item<<NULL;
        }
        else{
            item = new ApStandardItem(datas[i]->v);
            l<<NULL<<item;
        }
        bm->appendRow(l);
        l.clear();
    }
    ui->tview->setModel(bm);
    bm->setHeaderData(0,Qt::Horizontal,tr("摘要"));
    bm->setHeaderData(1,Qt::Horizontal,tr("总账科目"));
    bm->setHeaderData(2,Qt::Horizontal,tr("明细科目"));
    bm->setHeaderData(3,Qt::Horizontal,tr("币种"));
    bm->setHeaderData(4,Qt::Horizontal,tr("借方"));
    bm->setHeaderData(5,Qt::Horizontal,tr("贷方"));

    //设置列宽
    for(int i = 0; i < colWidths.count(); ++i)
        ui->tview->setColumnWidth(i,colWidths[i]);

    if(!datas.empty())
        ui->tview->selectRow(row);
}

//////////////////////////////LookupSubjectExtraDialog///////////////////////////////////
LookupSubjectExtraDialog::LookupSubjectExtraDialog(Account* account, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LookupSubjectExtraDialog),account(account)
{
    ui->setupUi(this);

    model = new QStandardItemModel;
    fCom = new SubjectComplete;
    sCom = new SubjectComplete(SndSubject);
    ui->cmbFstSub->setCompleter(fCom);
    ui->cmbSndSub->setCompleter(sCom);
    ui->spnYear->setMinimum(account->getBaseYear());
    ui->spnYear->setMaximum(account->getEndSuite());
    y = account->getCurSuite();
    ui->spnYear->setValue(y);
    on_spnYear_valueChanged(y);

    QSqlQuery q;
    int id; QString name;

    //初始化一级科目组合框
    q.exec("select id,subName from FirSubjects");
    ui->cmbFstSub->addItem(tr("所有"),0);
    while(q.next()){
        id = q.value(0).toInt();
        name = q.value(1).toString();
        ui->cmbFstSub->addItem(name,id);
    }
    fid = 0;

    mts = allMts.keys();
    qSort(mts.begin(),mts.end());

    connect(ui->cmbFstSub,SIGNAL(currentIndexChanged(int)),
            this,SLOT(fstSubChanged(int)));
    connect(ui->spnMonth,SIGNAL(valueChanged(int)),
            this,SLOT(monthChanged(int)));
    monthChanged(ui->spnMonth->value());
}


LookupSubjectExtraDialog::~LookupSubjectExtraDialog()
{
    delete ui;
}

//选择帐套
void LookupSubjectExtraDialog::on_spnYear_valueChanged(int year)
{
    y = year;

    int by = account->getBaseYear();
    int bm = account->getBaseMonth();
    int sy = account->getStartSuite();
    int sm = account->getSuiteFirstMonth(y);
    int em = account->getSuiteLastMonth(y);

    //期初年份比起始帐套年份早，则月份选择范围限制在期初月份上
    int month = ui->spnMonth->value();
    if(y == by && by < sy){
        ui->spnMonth->setMinimum(bm);
        ui->spnMonth->setMaximum(bm);
        ui->spnMonth->setValue(bm);
    }
    else if(y == by && by == sy){
        ui->spnMonth->setMinimum(bm);
        ui->spnMonth->setMaximum(em);
        ui->spnMonth->setValue(em);
    }
    else{
        ui->spnMonth->setMinimum(sm);
        ui->spnMonth->setMaximum(em);
        ui->spnMonth->setValue(em);
    }

    //月份数实际没有变，要实际调用一次更新
    if(month == bm || month == em)
        monthChanged(month);
}


//选择月份
void LookupSubjectExtraDialog::monthChanged(int v)
{
    m = v;
    fsums.clear();
    ssums.clear();
    fdirs.clear();
    sdirs.clear();
    if(!BusiUtil::readExtraByMonth2(y,m,fsums,fdirs,ssums,sdirs)){
        model->clear();
        return;
    }
    refresh();
}

//选择一级科目
void LookupSubjectExtraDialog::fstSubChanged(int index)
{
    fid = ui->cmbFstSub->itemData(index).toInt();
    disconnect(ui->cmbSndSub,SIGNAL(currentIndexChanged(int)),
               this,SLOT(sndSubChanged(int)));

    //装载当前选择的一级科目下所有的明细科目
    QString s = QString("select FSAgent.id,SecSubjects.subName"
                        " from FSAgent join SecSubjects on "
                        "SecSubjects.id = FSAgent.sid where fid = %1")
            .arg(fid);
    ui->cmbSndSub->clear();
    ui->cmbSndSub->addItem(tr("所有"),0);
    QSqlQuery q;
    q.exec(s);
    while(q.next())
        ui->cmbSndSub->addItem(q.value(1).toString(),q.value(0).toInt());
    //如果选择的一级科目下只有一个二级科目，则省略“所有”项目
    if(ui->cmbSndSub->count() == 2)
        ui->cmbSndSub->removeItem(0);
    sCom->setPid(fid);
    ui->cmbSndSub->setCurrentIndex(0);
    sid = ui->cmbSndSub->itemData(0).toInt();

    connect(ui->cmbSndSub,SIGNAL(currentIndexChanged(int)),
                   this,SLOT(sndSubChanged(int)));
    refresh();

}

//选择二级科目
void LookupSubjectExtraDialog::sndSubChanged(int index)
{
    sid = ui->cmbSndSub->itemData(index).toInt();
    refresh();
}

//刷新余额表格
void LookupSubjectExtraDialog::refresh()
{
    //QHash<int,double> fsums,ssums;
    //QHash<int,int> fdirs,sdirs;
    SubjectManager* sm = curAccount->getSubjectManager();
    ApStandardItem* item;
    QList<QStandardItem*> l;
    int key;
    QList<int>ids;

    model->clear();

    //显示所有一级科目的余额
    if(fid == 0){
        for(int i = 1; i < ui->cmbFstSub->count(); ++i)
            ids<<ui->cmbFstSub->itemData(i).toInt();
        QList<int> mtLst;
        for(int i = 0; i < ids.count(); ++i){
            mtLst.clear();
            if(BusiUtil::isAccMt(ids[i]))
                mtLst = mts;
            else
                mtLst<<RMB;
            for(int j = 0; j < mtLst.count(); ++j){
                key = ids[i] * 10 + mtLst[j];
                if(fsums.contains(key)){
                    item = new ApStandardItem(sm->getFstSubName(ids[i]));
                    l<<item;
                    item = new ApStandardItem(allMts.value(mts[j]));
                    l<<item;
                    item = new ApStandardItem(fsums.value(key));
                    l<<item;
                    item = new ApStandardItem(dirStr(fdirs.value(key)));
                    l<<item;
                    model->appendRow(l);
                    l.clear();
                }
            }
        }
    }
    //显示指定一级科目下的所有二级科目的余额
    else if((fid != 0) && (sid == 0)){
        //获取该一级科目下的所有二级科目的id列表
        QList<QString> names;
        BusiUtil::getSndSubInSpecFst(fid,ids,names);
        qSort(ids.begin(),ids.end());
        //如果该科目开始要求按币种分别核算
        if(BusiUtil::isAccMt(fid)){
            for(int i = 0; i < ids.count(); ++i)
                for(int j = 0; j < mts.count(); ++j){
                    key = ids[i] * 10 + mts[j];
                    if(ssums.contains(key)){
                        item = new ApStandardItem(allSndSubs.value(ids[i]));
                        l<<item;
                        item = new ApStandardItem(allMts.value(mts[j]));
                        l<<item;
                        item = new ApStandardItem(ssums.value(key).getv());
                        l<<item;
                        item = new ApStandardItem(dirStr(sdirs.value(key)));
                        l<<item;
                        model->appendRow(l);
                        l.clear();
                    }
                }
        }
        else{
            for(int i = 0; i < ids.count(); ++i){
                key = ids[i] * 10 + RMB;
                if(ssums.contains(key)){
                    item = new ApStandardItem(allSndSubs.value(ids[i]));
                    l<<item;
                    item = new ApStandardItem(allMts.value(RMB));
                    l<<item;
                    item = new ApStandardItem(ssums.value(key).getv());
                    l<<item;
                    item = new ApStandardItem(dirStr(sdirs.value(key)));
                    l<<item;
                    model->appendRow(l);
                    l.clear();
                }
            }
        }
    }
    //显示指定一级科目下的指定二级科目的余额
    else{
        QList<int> mtLst;
        if(BusiUtil::isAccMt(fid))
            mtLst = mts;
        else
            mtLst<<RMB;
        for(int i = 0; i < mtLst.count(); ++i){
            key = sid * 10 + mtLst[i];
            item = new ApStandardItem(allSndSubs.value(sid));
            l<<item;
            item = new ApStandardItem(allMts.value(mtLst[i]));
            l<<item;
            item = new ApStandardItem(ssums.value(key).getv());
            l<<item;
            item = new ApStandardItem(dirStr(sdirs.value(key)));
            l<<item;
            model->appendRow(l);
            l.clear();
        }
    }

    ui->tview->setModel(model);
    model->setHeaderData(0,Qt::Horizontal,tr("科目名"));
    model->setHeaderData(1,Qt::Horizontal,tr("币种"));
    model->setHeaderData(2,Qt::Horizontal,tr("余额"));
    model->setHeaderData(3,Qt::Horizontal,tr("方向"));
}


/////////////////////////////AccountPropertyDialog///////////////////////////////////////////
AccountPropertyDialog::AccountPropertyDialog(Account *account, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccountPropertyDialog)
{
    ui->setupUi(this);
    this->account = account;
    ui->cmbSubType->addItem(tr("旧科目"),SUBSYS_OLE);
    ui->cmbSubType->addItem(tr("新科目"),SUBSYS_NEW);

    ui->edtAccCode->setText(account->getCode());
    ui->edtSName->setText(account->getSName());
    ui->edtLName->setText(account->getLName());
    ui->edtAccFName->setText(account->getFileName());
    ui->datStart->setDate(account->getStartTime());
    ui->datEnd->setDate(account->getEndTime());
    ui->cmbSubType->setCurrentIndex(ui->cmbSubType->
                                    findData(account->getSubType()));
    //ui->cmbRptType->setCurrentIndex();
    foreach(int y,account->getSuites()){
        QListWidgetItem* item = new QListWidgetItem(account->getSuiteName(y));
        item->setData(Qt::UserRole,y);
        ui->lstSuites->addItem(item);
    }
    ui->edtCurSuite->setText(account->getSuiteName(account->getCurSuite()));
    ui->edtMMt->setText(allMts.value(account->getMasterMt()));
    ui->edtWaiMt->setText(account->getWaiMtStr());
    ui->btnAddMt->setEnabled(MTS.count() > account->getWaiMt().count()+1);
    ui->btnDelMt->setEnabled(!account->getWaiMt().empty());

    bCode = false;
    bSName = false;
    bLName = false;
    bSubType = false;
    bRptType = false;
    bSTime = false;
    bETime = false;
    bWaiMt = false;
    changed = false;
    cancel = false;
}

AccountPropertyDialog::~AccountPropertyDialog()
{
    delete ui;
}

bool AccountPropertyDialog::isDirty()
{
    info.clear();
    info = tr("如下信息发生了改变：\n");
    if(account->getCode() != ui->edtAccCode->text()){
        changed = true;
        bCode = true;
        info.append(tr("账户代码\n"));
    }
    if(account->getSName() != ui->edtSName->text()){
        changed = true;
        bSName = true;
        info.append(tr("账户简称\n"));
    }
    if(account->getLName() != ui->edtLName->text()){
        changed = true;
        bLName = true;
        info.append(tr("账户全称\n"));
    }
    if(account->getSubType() != ui->cmbSubType->
            itemData(ui->cmbSubType->currentIndex()).toInt()){
        changed = true;
        bSubType = true;
        info.append(tr("账户科目系统\n"));
    }
    //else if(account->getReportType() != ui->cmbRptType->
    //        itemData(ui->cmbRptType->currentIndex()).toInt()){
    //    changed = true;
    //    bRptType = true;
    //    info.append(tr("账户报表系统\n"));
    //}
    if(account->getStartTime() != ui->datStart->date()){
        changed = true;
        bSTime = true;
        info.append(tr("账户开始记账时间\n"));
    }
    if(account->getEndTime() != ui->datEnd->date()){
        changed = true;
        bETime = true;
        info.append(tr("账户结束记账时间\n"));
    }
    if(account->getWaiMtStr() != ui->edtWaiMt->text()){
        changed = true;
        bWaiMt = true;
        info.append(tr("账户使用的外币\n"));
    }
    info.append(tr("需要保存吗？\n"));
    return changed;
}

void AccountPropertyDialog::save(bool confirm)
{
    if(cancel)
        return;

    if(isDirty()){
        if(!confirm ||
           (confirm && (QMessageBox::Yes == QMessageBox::warning(this,
           tr("提示信息"),info,QMessageBox::Yes|QMessageBox::No)))){
            if(bCode)
                account->setCode(ui->edtAccCode->text());
            if(bSName)
                account->setSName(ui->edtSName->text());
            if(bLName)
                account->setLName(ui->edtLName->text());
            if(bSubType){
                SubjectManager::SubjectSysType subType;
                subType = (SubjectManager::SubjectSysType)ui->cmbSubType->
                        itemData(ui->cmbSubType->currentIndex()).toInt();
                account->setSubType(subType);
            }
            if(bRptType)
                account->setReportType((Account::ReportType)ui->cmbRptType->
                                       itemData(ui->cmbRptType->currentIndex()).toInt());
            if(bSTime){
                QDate date = ui->datStart->date();
                account->setStartTime(date);
            }
            if(bETime){
                QDate date = ui->datEnd->date();
                date.setYMD(date.year(),date.month(),date.daysInMonth());
                account->setEndTime(date);
            }
            if(bWaiMt){
                QList<int>waiMt;
                QStringList wl = ui->edtWaiMt->text().split(",");
                QHashIterator<int,QString> it(MTS);
                while(it.hasNext()){
                    it.next();
                    if(wl.contains(it.value()))
                        waiMt<<it.key();
                }
                account->setWaiMt(waiMt);
            }
        }
    }
}

//增加外币
void AccountPropertyDialog::on_btnAddMt_clicked()
{
//    QHash<int,QString> mts = MTS;
//    //移除已经使用的币种
//    mts.remove(account->getMasterMt());
//    foreach(int mt,account->getWaiMt())
//        mts.remove(mt);

//    if(!mts.empty()){
//        QStringList items;
//        QList<int> mtLst;
//        mtLst = mts.keys();
//        qSort(mtLst.begin(),mtLst.end());
//        for(int i = 0; i < mtLst.count(); ++i)
//            items<<MTS.value(mtLst[i]);
//        bool ok;
//        int curItem;
//        QInputDialog::getItem(this,tr("增加外币"), tr("请选择一个外币"),
//                              items, curItem,false,&ok);
//        if(ok)
//            account->addWaiMt(mtLst[curItem]);
//    }
}

void AccountPropertyDialog::on_btnOk_clicked()
{
    save();
    cancel = true;
    emit accepted();
}

void AccountPropertyDialog::on_btnCancel_clicked()
{
    cancel = true;
    emit rejected();
}


















