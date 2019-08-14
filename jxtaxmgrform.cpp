#include "jxtaxmgrform.h"
#include "ui_jxtaxmgrform.h"

#include "account.h"
#include "PzSet.h"
#include "dbutil.h"
#include "myhelper.h"
#include "subject.h"
#include "pz.h"
#include "utils.h"
#include "commands.h"

#include <QDoubleValidator>
#include <QMenu>
#include <QDateEdit>

JxTaxMgrDlg::JxTaxMgrDlg(Account *account, QWidget *parent) : QDialog(parent), ui(new Ui::JxTaxMgrForm),
    account(account),curTaxEdited(false),hisTaxEdited(false)
{
    ui->setupUi(this);
    asMgr = account->getSuiteMgr();
    sm = account->getSubjectManager(asMgr->getSuiteRecord()->subSys);
    if(!asMgr->isPzSetOpened())
        return;
    c_ok=QColor("darkGreen");
    c_adjust = QColor("blue");
    c_error = QColor("darkMagenta");
    c_noexist = QColor("red");
    connect(ui->twHistorys,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(cusContextMenuRequested(QPoint)));
    connect(ui->twCurAuth,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(curAuthMenyRequested(QPoint)));
    init();
}

JxTaxMgrDlg::~JxTaxMgrDlg()
{
    delete ui;
}

/**
 * @brief 初始化历史数据
 */
void JxTaxMgrDlg::initHistoryDatas()
{
    //扫描本账套内的所有专票
    QList<PingZheng *> pzs;
    FirstSubject* yfFSub = sm->getYfSub();
    FirstSubject* yjsjFSub = sm->getYjsjSub();
    SecondSubject* jxSSub = sm->getJxseSSub();
    QHash<QString,QString> pzNums; //键：发票号，值高4位年份，中两位月份，低三位凭证号
    QHash<QString,int> baIDs;     //键：发票号，值：分录id
    QHash<QString,Double> taxed;  //键：发票号，值：税额
    QHash<QString,SecondSubject*> clients;
    int endMonth = asMgr->month();
    AccountSuiteManager* preAsMgr = 0;
    int startMonth=endMonth-5;
    if(startMonth < 1){
        AccountSuiteRecord* r = account->getSuiteRecord(asMgr->year()-1);
        if(r){
            preAsMgr = account->getSuiteMgr(r->id);
            if(startMonth + 12 < preAsMgr->getSuiteRecord()->startMonth)
                startMonth = preAsMgr->getSuiteRecord()->startMonth;
        }
        else
            startMonth = asMgr->getSuiteRecord()->startMonth;
    }
    for(int m = startMonth; m <= endMonth; ++m){
        pzs.clear();
        if(m < 1)
            preAsMgr->getPzSet(m+12,pzs);
        else
            asMgr->getPzSet(m,pzs);
        foreach (PingZheng* pz,pzs) {
            foreach (BusiAction* ba, pz->baList()) {
                if(ba->getDir() == MDIR_J && ba->getFirstSubject() == yfFSub && ba->getSummary().contains(tr("（税金）"))){
                    QString inum; Double wb;
                    PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wb);
                    if(inum.isEmpty())
                        continue;                    
                    QString pzNum = pz->getDate2().toString("yyyyMM");
                    int pn = pz->number();
                    if(pn<10)
                        pzNum = pzNum+"00"+QString::number(pn);
                    else if(pn<100)
                        pzNum = pzNum+"0"+QString::number(pn);
                    else
                        pzNum = pzNum+QString::number(pn);
                    pzNums[inum] = pzNum;
                    baIDs[inum] = ba->getId();
                    taxed[inum] = ba->getValue();
                    clients[inum] = ba->getSecondSubject();
                }
                else if(ba->getDir() == MDIR_J && ba->getFirstSubject() == yjsjFSub && ba->getSecondSubject() == jxSSub){
                    QString inum; Double wb;
                    PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wb);
                    if(pzNums.contains(inum)){
                        pzNums.remove(inum);
                        baIDs.remove(inum);
                        taxed.remove(inum);
                        clients.remove(inum);
                    }
                }
            }
        }
    }
    if(pzNums.isEmpty()){
        myHelper::ShowMessageBoxInfo(tr("未能扫描到任何历史遗留的未认证成本专票！"));
        return;
    }
    QHashIterator<QString,QString> it(pzNums);
    ui->twHistorys->setRowCount(pzNums.count());
    QString clientName,date;
    Double tax,money;
    while(it.hasNext()){
        it.next();
        HisAuthCostInvoiceInfo* hi = new HisAuthCostInvoiceInfo;
        hi->id=0;
        hi->pzNum = tr("%1年%2月%3#").arg(it.value().left(4)).arg(it.value().mid(4,2)).arg(it.value().right(3));
        hi->baId = baIDs.value(it.key());
        hi->iNum = it.key();
        hi->taxMoney = taxed.value(it.key());
        hi->client = clients.value(it.key());
        tax=0;money=0;clientName="";date="";
        if(account->getDbUtil()->readCostInvoiceForTax(it.key(),tax,money,clientName,date)){
            hi->date = date;
            hi->money = money;
        }
        if(hi->client->getName() == tr("其他"))
            hi->summary = account->getDbUtil()->getOriginalSummary(hi->baId);
        haInvoices<<hi;
    }
    showHaInvoices();
}

/**
 * @brief 扫描本月凭证集，提取本月计入的进项税
 */
void JxTaxMgrDlg::scanCurrentTax()
{
    int m = asMgr->month();
    QList<PingZheng*> pzs;
    FirstSubject* yjsjFSub = sm->getYjsjSub();
    SecondSubject* jxseSSub = sm->getJxseSSub();
    asMgr->getPzSet(m,pzs);
    foreach(PingZheng* pz, pzs){
        foreach(BusiAction* ba, pz->baList()){
            if(ba->getDir() == MDIR_J && ba->getFirstSubject() == yjsjFSub && ba->getSecondSubject() == jxseSSub){
                CurAuthCostInvoiceInfo* ci = new CurAuthCostInvoiceInfo;
                ci->isCur = true;
                QString inum; Double wb;
                PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wb);
                ci->inum = inum;
                if(!inum.isEmpty()){
                    Double tm;
                    QString date;
                    account->getDbUtil()->readCostInvoiceForTax(inum,tm,ci->money,ci->clientName,date);
                    ci->ni = findMatchedNiForClientName(ci->clientName);
                    if(!ci->ni)
                        myHelper::ShowMessageBoxWarning(tr("没有找到与客户名“%1”对应的名称对象").arg(ci->clientName));
                }
                ci->taxMoney = ba->getValue();
                caInvoices<<ci;
            }
        }
    }
    if(!caInvoices.isEmpty()){
        curTaxEdited = true;
        ui->btnSaveCur->setEnabled(true);
    }
}

/**
 * @brief 请求历史进项税表上下文菜单
 * @param pos
 */
void JxTaxMgrDlg::cusContextMenuRequested(const QPoint &pos)
{    
    if(ui->twHistorys->itemAt(pos)){
        QMenu m;
        m.addAction(ui->actDelHis);
        m.addAction(ui->actJrby);
        m.exec(ui->twHistorys->mapToGlobal(pos));
    }
}

void JxTaxMgrDlg::curAuthMenyRequested(const QPoint &pos)
{
    QMenu m;
    m.addAction(ui->actDelCur);
    m.exec(ui->twCurAuth->mapToGlobal(pos));
}

void JxTaxMgrDlg::addCurAuthInvoice()
{
    CurAuthCostInvoiceInfo* ri = new CurAuthCostInvoiceInfo;
    ri->id=0;
    ri->edited = true;
    caInvoices<<ri;
    int row = ui->twCurAuth->rowCount()-1;
    turnDataInspect(false);
    ui->twCurAuth->insertRow(row);
    ui->twCurAuth->setItem(row,CICA_NUM,new QTableWidgetItem);
    ui->twCurAuth->item(row,CICA_NUM)->setData(DR_VERIFY_STATE,VS_NOEXIST);
    setInvoiceColor(ui->twCurAuth->item(row,CICA_NUM),VS_NOEXIST);
    ui->twCurAuth->setItem(row,CICA_TAX,new QTableWidgetItem);
    ui->twCurAuth->setItem(row,CICA_MONEY,new QTableWidgetItem);
    ui->twCurAuth->setItem(row,CICA_CLIENT,new QTableWidgetItem);
    ui->twCurAuth->selectRow(row);
    ui->twCurAuth->edit(ui->twCurAuth->model()->index(row,CICA_NUM));
    turnDataInspect();
}

/**
 * @brief 本月认证发票表格数据改变
 */
void JxTaxMgrDlg::DataChanged(QTableWidgetItem *item)
{
    int r = item->row();
    if(r == ui->twCurAuth->rowCount()-1)
        return;
    int c = item->column();
    CurAuthCostInvoiceInfo* ri = caInvoices.at(r);
    ri->edited=true;
    switch (c) {
    //查找curInvoices表，是否有与此发票对应的记录，如果有，读取税额、金额、客户数据
    case CICA_NUM:{
        QRegExp re("\\d{8}");
        if(re.indexIn(item->text()) == -1){
            myHelper::ShowMessageBoxWarning(tr("发票号不符合规范！"));
            return;
        }
        ri->inum = item->text();
        QString clientName,date;
        if(account->getDbUtil()->readCostInvoiceForTax(ri->inum,ri->taxMoney,ri->money,clientName,date)){
            ri->ni = findMatchedNiForClientName(clientName);
            if(!ri->ni)
                myHelper::ShowMessageBoxWarning(tr("没有找到与客户名“%1”对应的名称对象").arg(clientName));
            turnDataInspect(false);
            ui->twCurAuth->item(r,CICA_TAX)->setText(ri->taxMoney.toString());
            ui->twCurAuth->item(r,CICA_MONEY)->setText(ri->money.toString());
            ui->twCurAuth->item(r,CICA_CLIENT)->setText(ri->ni?ri->ni->getLongName():clientName);
            calCurAuthTaxSum();
            turnDataInspect();
        }
        curTaxEdited = true;
        break;
    }
    case CICA_TAX:{
            ri->taxMoney = item->text().toDouble();
            calCurAuthTaxSum();
            curTaxEdited = true;
        }
        break;
    case CICA_MONEY:
        ri->money = item->text().toDouble();
        curTaxEdited = true;
        break;
    case CICA_CLIENT:
    {
        QString name = item->text();
        ri->ni = findMatchedNiForClientName(name);
        if(!ri->ni){
            myHelper::ShowMessageBoxWarning(tr("不存在一个现有名称对象与输入的客户名匹配，请重新输入！"));
        }
        curTaxEdited = true;
        break;
    }
    }
    if(curTaxEdited)
        ui->btnSaveCur->setEnabled(true);
}

void JxTaxMgrDlg::init()
{
    DbUtil* du = account->getDbUtil();
    if(!du->readCurAuthCostInvoices(asMgr->year(),asMgr->month(),caInvoices,sm)){
        myHelper::ShowMessageBoxError(tr("在读取本月认证发票信息时发生错误！"));
        return;
    }
    if(caInvoices.isEmpty())
        scanCurrentTax();    
    showCaInvoices();
    if(!du->readHisNotAuthCostInvoices(sm,haInvoices)){
        myHelper::ShowMessageBoxError(tr("在读取历史未认证发票信息时发生错误！"));
        return;
    }
    foreach(HisAuthCostInvoiceInfo* hi, haInvoices){
        if(hi->client->getName() == tr("其他")){
            hi->summary = du->getOriginalSummary(hi->baId);
        }
    }
    showHaInvoices();
    turnDataInspect();
}

/**
 * @brief 显示本月认证成本发票信息
 */
void JxTaxMgrDlg::showCaInvoices()
{
    ui->twCurAuth->setRowCount(caInvoices.count()+1);
    Double sum; int row=0;
    bool exist = false;
    foreach (CurAuthCostInvoiceInfo* ri, caInvoices) {
        ui->twCurAuth->setItem(row,CICA_ISCUR,new QTableWidgetItem(ri->isCur?" + ":"=>"));
        ui->twCurAuth->setItem(row,CICA_NUM,new QTableWidgetItem(ri->inum));
        ui->twCurAuth->item(row,CICA_NUM)->setData(DR_VERIFY_STATE,VS_NOEXIST);
        ui->twCurAuth->setItem(row,CICA_TAX,new QTableWidgetItem(ri->taxMoney.toString()));
        ui->twCurAuth->setItem(row,CICA_MONEY,new QTableWidgetItem(ri->money.toString()));
        ui->twCurAuth->setItem(row,CICA_CLIENT,new QTableWidgetItem(ri->ni?ri->ni->getLongName():ri->clientName));
        row++;
        sum += ri->taxMoney;
        if(!ri->isCur)
            exist = true;
    }
    ui->twCurAuth->setItem(row,CICA_NUM,new QTableWidgetItem(tr("合计")));
    ui->twCurAuth->setItem(row,CICA_TAX,new QTableWidgetItem(sum.toString()));
    ui->btnCrtPz->setEnabled(exist);
}

/**
 * @brief 计算本月认证发票税额总计
 */
void JxTaxMgrDlg::calCurAuthTaxSum()
{
    Double sum;
    for(int i=0;i<caInvoices.count();++i)
        sum += caInvoices.at(i)->taxMoney;
     ui->twCurAuth->item(ui->twCurAuth->rowCount()-1,CICA_TAX)->setText(sum.toString());
}

/**
 * @brief 显示历史未认证发票信息
 */
void JxTaxMgrDlg::showHaInvoices()
{
    ui->twHistorys->setRowCount(haInvoices.count());
    int row = 0;
    foreach (HisAuthCostInvoiceInfo* ri, haInvoices) {
        ui->twHistorys->setItem(row,CIH_PZ,new QTableWidgetItem(ri->pzNum));
        ui->twHistorys->setItem(row,CIH_INVOICE,new QTableWidgetItem(ri->iNum));
        ui->twHistorys->setItem(row,CIH_DATE,new QTableWidgetItem(ri->date));
        ui->twHistorys->setItem(row,CIH_TAX,new QTableWidgetItem(ri->taxMoney.toString()));
        ui->twHistorys->setItem(row,CIH_MONEY,new QTableWidgetItem(ri->money.toString()));
        QTableWidgetItem* item = new QTableWidgetItem(ri->client->getLName());
        item->setToolTip(ri->summary);
        ui->twHistorys->setItem(row,CIH_CLIENT,item);
        row++;
    }
}

void JxTaxMgrDlg::turnDataInspect(bool on)
{
    if(on)
        connect(ui->twCurAuth,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(DataChanged(QTableWidgetItem*)));
    else
        disconnect(ui->twCurAuth,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(DataChanged(QTableWidgetItem*)));
}

/**
 * @brief 查询本月认证发票表格，是否指定的发票已被认证
 * @param inum
 * @return
 */
JxTaxMgrDlg::VerifyState JxTaxMgrDlg::isInvoiceAuthed(QString inum,Double tax)
{
    if(inum.isEmpty())
        return VS_ADJUST;
    for(int r = 0; r < ui->twCurAuth->rowCount()-1;++r){
        if(inum == ui->twCurAuth->item(r,CICA_NUM)->text()){
            if(tax == ui->twCurAuth->item(r,CICA_TAX)->text().toDouble()){
                ui->twCurAuth->item(r,CICA_NUM)->setData(DR_VERIFY_STATE,VS_OK);
                setInvoiceColor(ui->twCurAuth->item(r,CICA_NUM),VS_OK);
                return VS_OK;
            }
            else{
                ui->twCurAuth->item(r,CICA_NUM)->setData(DR_VERIFY_STATE,VS_ERROR);
                setInvoiceColor(ui->twCurAuth->item(r,CICA_NUM),VS_ERROR);
                return VS_ERROR;
           }
        }
    }
    return VS_NOEXIST;
}

/**
 * @brief  指定进项税发票是否在历史记录表中
 * @param num
 * @param sub
 * @param tax
 * @return
 */
JxTaxMgrDlg::VerifyState JxTaxMgrDlg::inHistory(QString num, SecondSubject *sub, Double tax)
{
    for(int r = 0; r<ui->twHistorys->rowCount();++r){
        if(num == ui->twHistorys->item(r,CIH_INVOICE)->text()){
            if(sub == haInvoices.at(r)->client && tax == haInvoices.at(r)->taxMoney)
                return VS_OK;
            else
                return VS_ERROR;
        }
    }
    return VS_NOEXIST;
}

void JxTaxMgrDlg::setInvoiceColor(QTableWidgetItem *item, JxTaxMgrDlg::VerifyState state)
{
    QColor c;
    switch(state){
    case VS_OK:
        c=c_ok;
        break;
    case VS_ADJUST:
        c=c_adjust;
        break;
    case VS_ERROR:
        c=c_error;
        break;
    case VS_NOEXIST:
        c=c_noexist;
        break;
    default:
        c=QColor("black");
    }
    item->setForeground(QBrush(c));
}

/**
 * @brief JxTaxMgrDlg::findMatchedNiForClientName
 * 查找匹配客户名的名称对象
 * @param cname
 * @return
 */
SubjectNameItem *JxTaxMgrDlg::findMatchedNiForClientName(QString cname)
{
    foreach(SubjectNameItem* ni,sm->getAllNameItems()){
        if(ni->getLongName() == cname){
            return ni;
        }
    }
    return 0;
}

bool JxTaxMgrDlg::isDirty()
{
    return curTaxEdited || hisTaxEdited;
}

void JxTaxMgrDlg::save()
{
    if(curTaxEdited)
        on_btnSaveCur_clicked();
    if(hisTaxEdited)
        on_btnSaveHis_clicked();
}


/**
 * @brief 删除历史记录
 */
void JxTaxMgrDlg::on_actDelHis_triggered()
{
    int row = ui->twHistorys->currentRow();
    delete haInvoices.at(row);
    haInvoices.removeAt(row);
    ui->twHistorys->removeRow(row);
    hisTaxEdited = true;
    ui->btnSaveHis->setEnabled(true);
}

void JxTaxMgrDlg::on_btnInitHis_clicked()
{
    initHistoryDatas();
    if(!haInvoices.isEmpty()){
        hisTaxEdited = true;
        ui->btnSaveHis->setEnabled(true);
    }
}

void JxTaxMgrDlg::on_btnSaveHis_clicked()
{
    if(!account->getDbUtil()->updateHisNotAuthCosInvoices(haInvoices)){
        myHelper::ShowMessageBoxError(tr("在保存历史未认证发票信息时发生错误！"));
        return;
    }
    if(!haInvoices_del.isEmpty() && !account->getDbUtil()->removeHisNotAuthCosInvoices(haInvoices_del))
        myHelper::ShowMessageBoxError(tr("在移除历史未认证发票信息时发生错误！"));
    hisTaxEdited = false;
    ui->btnSaveHis->setEnabled(false);
}

/**
 * @brief 保存本月认证发票信息
 */
void JxTaxMgrDlg::on_btnSaveCur_clicked()
{
    if(curTaxEdited && !account->getDbUtil()->saveCurAuthCostInvoices(asMgr->year(),asMgr->month(),caInvoices))
        myHelper::ShowMessageBoxError(tr("在保存本期认证发票税额信息时发生错误！"));
    curTaxEdited = false;
    ui->btnSaveCur->setEnabled(false);
}

void JxTaxMgrDlg::on_chkInited_clicked(bool checked)
{
    ui->btnInitHis->setEnabled(!checked);
}


/**
 * @brief JxTaxMgrDlg::on_actDelCur_triggered
 * 移除本月认证发票
 */
void JxTaxMgrDlg::on_actDelCur_triggered()
{
    int row = ui->twCurAuth->currentRow();
    delete caInvoices.at(row);
    caInvoices.removeAt(row);
    ui->twCurAuth->removeRow(row);
    calCurAuthTaxSum();
    curTaxEdited = true;
}

/**
 * @brief 创建将历史进项税计入本月进项税的调整凭证
 */
void JxTaxMgrDlg::on_btnCrtPz_clicked()
{
    QList<CurAuthCostInvoiceInfo*> caIns;
    foreach (CurAuthCostInvoiceInfo* ca, caInvoices) {
        if(!ca->isCur)
            caIns<<ca;
    }
    if(caIns.isEmpty()){
        myHelper::ShowMessageBoxInfo(tr("本月认证的进项税发票都是当月计入的，无需调整！"));
        return;
    }
    PingZheng* pz = asMgr->crtJxTaxPz(caIns);
    if(!pz){
        myHelper::ShowMessageBoxInfo(tr("在创建历史进项税调整凭证时发生错误！"));
        return;
    }
    AppendPzCmd* cmd = new AppendPzCmd(asMgr,pz);
    asMgr->getUndoStack()->push(cmd);
    ui->btnCrtPz->setEnabled(false);
}

/**
 * @brief 将历史进项税计入本月认证的进项税
 */
void JxTaxMgrDlg::on_actJrby_triggered()
{
    int row = ui->twHistorys->currentRow();
    QString hInum = ui->twHistorys->item(row,CIH_INVOICE)->text();
    ui->twHistorys->removeRow(row);
    HisAuthCostInvoiceInfo* hi=0;
    foreach (HisAuthCostInvoiceInfo* thi,haInvoices){
        if(thi->iNum == hInum){
            hi = thi;
            haInvoices_del<<thi;
            haInvoices.removeOne(thi);
            break;
        }
    }
    CurAuthCostInvoiceInfo* ci = new CurAuthCostInvoiceInfo;
    ci->isCur = false;
    ci->ni = hi->client->getNameItem();
    ci->inum = hi->iNum;
    ci->taxMoney = hi->taxMoney;
    ci->money = hi->money;
    ci->originalSummary = hi->summary;
    caInvoices<<ci;
    row = caInvoices.count();
    turnDataInspect(false);
    ui->twCurAuth->insertRow(row-1);
    ui->twCurAuth->setItem(row-1,CICA_ISCUR,new QTableWidgetItem("=>"));
    ui->twCurAuth->setItem(row-1,CICA_NUM,new QTableWidgetItem(ci->inum));
    ui->twCurAuth->setItem(row-1,CICA_MONEY,new QTableWidgetItem(ci->money.toString()));
    ui->twCurAuth->setItem(row-1,CICA_TAX,new QTableWidgetItem(ci->taxMoney.toString()));
    QTableWidgetItem* item = new QTableWidgetItem(ci->ni->getLongName());
    item->setToolTip(ci->originalSummary);
    ui->twCurAuth->setItem(row-1,CICA_CLIENT,item);
    Double sum = Double(ui->twCurAuth->item(row,CICA_TAX)->text().toDouble());
    sum += ci->taxMoney;
    ui->twCurAuth->item(row,CICA_TAX)->setText(sum.toString());
    turnDataInspect();
    hisTaxEdited = true;
    curTaxEdited = true;
    ui->btnSaveHis->setEnabled(true);
    ui->btnSaveCur->setEnabled(true);
    ui->btnCrtPz->setEnabled(true);
}

/**
 * @brief JxTaxMgrDlg::on_edtFilter_textChanged
 * 当用户输入待查询发票号时
 * @param arg1
 */
void JxTaxMgrDlg::on_edtFilter_textChanged(const QString &t)
{
    for(int r = 0; r < ui->twHistorys->rowCount(); ++r){
        if(ui->twHistorys->item(r,CIH_INVOICE)->text().startsWith(t))
            ui->twHistorys->showRow(r);
        else
            ui->twHistorys->hideRow(r);
    }
}

void JxTaxMgrDlg::on_rdbFilter_toggled(bool checked)
{
    ui->edtFilter->setEnabled(checked);
}

/**
 * @brief JxTaxMgrDlg::on_btnAddCurs_clicked
 * 扫描本月凭证集，将未认证发票添加到历史表中
 */
void JxTaxMgrDlg::on_btnAddCurs_clicked()
{
    QList<PingZheng *> pzs;
    asMgr->getPzSet(asMgr->month(),pzs);    
    int row = haInvoices.count();
    foreach(PingZheng* pz, pzs){
        foreach(BusiAction* ba, pz->baList()){
            if(ba->getDir() == MDIR_J && ba->getFirstSubject() == sm->getYfSub() &&
                    ba->getSummary().contains(tr("（税金）"))){                
                QString inum; Double wb;
                PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wb);
                if(inum.isEmpty()){
                    myHelper::ShowMessageBoxWarning(tr("无法提取出有效的认证发票的发票号，摘要信息：“%1”").arg(ba->getSummary()));
                    continue;
                }
                HisAuthCostInvoiceInfo* hi = new HisAuthCostInvoiceInfo;
                hi->id=0;
                QString d = pz->getDate();
                hi->pzNum = tr("%1年%2月%3#").arg(d.left(4)).arg(d.mid(5,2)).arg(pz->number());
                hi->baId = ba->getId();
                hi->iNum = inum;
                hi->taxMoney = ba->getValue();
                hi->client = ba->getSecondSubject();
                if(hi->client->getName() == tr("其他"))
                    hi->summary = ba->getSummary();
                Double money,tax;
                QString clientName,date;
                if(account->getDbUtil()->readCostInvoiceForTax(inum,tax,money,clientName,date)){
                    hi->date = date;
                    hi->money = money;
                }
                haInvoices<<hi;
                hisTaxEdited = true;
                ui->twHistorys->insertRow(row);
                ui->twHistorys->setItem(row,CIH_PZ,new QTableWidgetItem(hi->pzNum));
                ui->twHistorys->setItem(row,CIH_INVOICE,new QTableWidgetItem(hi->iNum));
                ui->twHistorys->setItem(row,CIH_DATE,new QTableWidgetItem(hi->date));
                ui->twHistorys->setItem(row,CIH_TAX,new QTableWidgetItem(hi->taxMoney.toString()));
                ui->twHistorys->setItem(row,CIH_MONEY,new QTableWidgetItem(hi->money.toString()));
                ui->twHistorys->setItem(row,CIH_CLIENT,new QTableWidgetItem(hi->client->getLName()));
                row++;
            }
        }
    }
    ui->btnSaveHis->setEnabled(hisTaxEdited);
}

void JxTaxMgrDlg::on_btnOk_clicked()
{
    save();
    accept();
}
