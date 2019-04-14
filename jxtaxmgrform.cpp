#include "jxtaxmgrform.h"
#include "ui_jxtaxmgrform.h"

#include "account.h"
#include "PzSet.h"
#include "dbutil.h"
#include "myhelper.h"
#include "subject.h"
#include "pz.h"
#include "utils.h"

#include <QDoubleValidator>
#include <QMenu>
#include <QDateEdit>

JxTaxMgrDlg::JxTaxMgrDlg(Account *account, QWidget *parent) : QDialog(parent), ui(new Ui::JxTaxMgrForm),
    account(account)
{
    ui->setupUi(this);
    asMgr = account->getSuiteMgr();
    sm = account->getSubjectManager(asMgr->getSuiteRecord()->subSys);
    if(!asMgr->isPzSetOpened())
        return;
    QDoubleValidator* validator = new QDoubleValidator(this);
    validator->setDecimals(2);
    ui->edtTaxAmount->setValidator(validator);
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
    while(it.hasNext()){
        it.next();
        HisAuthCostInvoiceInfo* hi = new HisAuthCostInvoiceInfo;
        hi->id=0;
        hi->pzNum = tr("%1年%2月%3#").arg(it.value().left(4)).arg(it.value().mid(4,2)).arg(it.value().right(3));
        hi->baId = baIDs.value(it.key());
        hi->iNum = it.key();
        hi->taxMoney = taxed.value(it.key());
        hi->client = clients.value(it.key());
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
                    account->getDbUtil()->readCostInvoiceForTax(inum,tm,ci->money,ci->cName);
                }
                ci->taxMoney = ba->getValue();
                caInvoices<<ci;
            }
        }
    }
}

/**
 * @brief 请求历史进项税表上下文菜单
 * @param pos
 */
void JxTaxMgrDlg::cusContextMenuRequested(const QPoint &pos)
{
    QMenu m;
    m.addAction(ui->actAddHis);
    if(ui->twHistorys->itemAt(pos))
        m.addAction(ui->actDelHis);
        m.addAction(ui->actJrby);
    m.exec(ui->twHistorys->mapToGlobal(pos));
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
        if(account->getDbUtil()->readCostInvoiceForTax(ri->inum,ri->taxMoney,ri->money,ri->cName)){
            turnDataInspect(false);
            ui->twCurAuth->item(r,CICA_TAX)->setText(ri->taxMoney.toString());
            ui->twCurAuth->item(r,CICA_MONEY)->setText(ri->money.toString());
            ui->twCurAuth->item(r,CICA_CLIENT)->setText(ri->cName);
            calCurAuthTaxSum();
            turnDataInspect();
        }
        break;
    }
    case CICA_TAX:{
            ri->taxMoney = item->text().toDouble();
            calCurAuthTaxSum();
        }
        break;
    case CICA_MONEY:
        ri->money = item->text().toDouble();
        break;
    case CICA_CLIENT:
        ri->cName = item->text();
        break;
    }
}

void JxTaxMgrDlg::init()
{
    DbUtil* du = account->getDbUtil();
    if(!du->readCurAuthCostInvAmount(asMgr->year(),asMgr->month(),taxAmount)||!du->readCurAuthCostInvoices(asMgr->year(),asMgr->month(),caInvoices)){
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
    showHaInvoices();
    //VerifyCurInvoices();
    turnDataInspect();
}

/**
 * @brief 显示本月认证成本发票信息
 */
void JxTaxMgrDlg::showCaInvoices()
{
    if(taxAmount != 0)
        ui->edtTaxAmount->setText(taxAmount.toString());
    ui->twCurAuth->setRowCount(caInvoices.count()+1);   
    Double sum; int row=0;
    foreach (CurAuthCostInvoiceInfo* ri, caInvoices) {
        ui->twCurAuth->setItem(row,CICA_ISCUR,new QTableWidgetItem(ri->isCur?" + ":"=>"));
        ui->twCurAuth->setItem(row,CICA_NUM,new QTableWidgetItem(ri->inum));
        ui->twCurAuth->item(row,CICA_NUM)->setData(DR_VERIFY_STATE,VS_NOEXIST);
        ui->twCurAuth->setItem(row,CICA_TAX,new QTableWidgetItem(ri->taxMoney.toString()));
        ui->twCurAuth->setItem(row,CICA_MONEY,new QTableWidgetItem(ri->money.toString()));
        ui->twCurAuth->setItem(row,CICA_CLIENT,new QTableWidgetItem(ri->cName));
        row++;
        sum += ri->taxMoney;
    }
//    ui->btnApply->setEnabled(sum == taxAmount);
//    btnAddCa = new QPushButton(tr("新增"),this);
//    connect(btnAddCa,SIGNAL(clicked()),this,SLOT(addCurAuthInvoice()));
//    ui->twCurAuth->setCellWidget(row,CICA_NUM,btnAddCa);
    ui->twCurAuth->setItem(row,CICA_NUM,new QTableWidgetItem(tr("合计")));
    ui->twCurAuth->setItem(row,CICA_TAX,new QTableWidgetItem(sum.toString()));
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
        ui->twHistorys->setItem(row,CIH_CLIENT,new QTableWidgetItem(ri->client->getLName()));
        row++;
    }
}

/**
 * @brief 验证本月进项税归置是否正确
 */
//void JxTaxMgrDlg::VerifyCurInvoices()
//{
//    FirstSubject* yjsjFSub = sm->getYjsjSub();
//    FirstSubject* yfFSub = sm->getYfSub();
//    SecondSubject* jxSSub = sm->getJxseSSub();
//    QList<PingZheng *> pzs;
//    asMgr->getPzSet(asMgr->month(),pzs);
//    foreach (PingZheng* pz, pzs) {
//        //首先判断凭证是否为进项税调节凭证（即将暂存在应付借方的税金移入进项税）
//        //按约定，此凭证不许容纳其他无关分录
//        bool isAdjustPz = true;
//        foreach (BusiAction* ba, pz->baList()) {
//            if(ba->getFirstSubject() != yjsjFSub && ba->getFirstSubject() != yfFSub){
//                isAdjustPz = false;
//                break;
//            }
//        }
//        int row = 0;
//        foreach (BusiAction* ba, pz->baList()) {
//             //如果不是调整凭证，则遇到进项税，则检查本月认证表。
//            if(!isAdjustPz && ba->getDir() == MDIR_J && ba->getFirstSubject() == yjsjFSub && ba->getSecondSubject() == jxSSub){
//                QString inum,cName; Double wbMoney;
//                PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wbMoney);
//                PaUtils::extractCustomerName(ba->getSummary(),cName);
//                SubjectNameItem* ni = SubjectManager::getNameItem(cName);
//                if(ni)
//                    cName = ni->getLongName();
//                ui->twCost->insertRow(row);
//                ui->twCost->setItem(row,CIC_PZ,new QTableWidgetItem(QString::number(pz->number())));
//                ui->twCost->setItem(row,CIC_NUMBER,new QTableWidgetItem(inum));
//                VerifyState vs = isInvoiceAuthed(inum,ba->getValue());
//                setInvoiceColor(ui->twCost->item(row,CIC_NUMBER),vs);
//                ui->twCost->item(row,CIC_NUMBER)->setData(DR_VERIFY_STATE,vs);
//                ui->twCost->setItem(row,CIC_TAX,new QTableWidgetItem(ba->getValue().toString()));
//                ui->twCost->setItem(row,CIC_CLIENT,new QTableWidgetItem(cName));
//                ui->twCost->item(row,CIC_PZ)->setData(Qt::UserRole,ba->getId());
//                row++;
//            }
//            //如果是调整凭证，则遇到进项税，则检查本月认证表，遇到应付，则检查历史未认证表
//            else if(isAdjustPz ){
//                QString inum;Double wbMoney;
//                PaUtils::extractOnlyInvoiceNum(ba->getSummary(),inum,wbMoney);
//                if(ba->getDir() == MDIR_D && ba->getFirstSubject()==yfFSub){
//                    SecondSubject* cSub = ba->getSecondSubject();
//                    VerifyState vs = inHistory(inum,cSub,ba->getValue());
//                }
//                else if(ba->getDir() == MDIR_J && ba->getFirstSubject() == yjsjFSub && ba->getSecondSubject() == jxSSub){
//                    VerifyState vs = isInvoiceAuthed(inum,ba->getValue());
//                    ui->twCost->insertRow(row);
//                    ui->twCost->setItem(row,CIC_PZ,new QTableWidgetItem(QString::number(pz->number())));
//                    ui->twCost->setItem(row,CIC_NUMBER,new QTableWidgetItem(inum));
//                    setInvoiceColor(ui->twCost->item(row,CIC_NUMBER),vs);
//                    ui->twCost->item(row,CIC_NUMBER)->setData(DR_VERIFY_STATE,vs);
//                    ui->twCost->setItem(row,CIC_TAX,new QTableWidgetItem(ba->getValue().toString()));
//                    QString cName;
//                    PaUtils::extractCustomerName(ba->getSummary(),cName);
//                    SubjectNameItem* ni = SubjectManager::getNameItem(cName);
//                    if(ni)
//                        cName = ni->getLongName();
//                    ui->twCost->setItem(row,CIC_CLIENT,new QTableWidgetItem(cName));
//                    ui->twCost->item(row,CIC_PZ)->setData(Qt::UserRole,ba->getId());
//                    row++;
//                }
//            }
//        }
//    }
//    if(ui->twCost->rowCount() == 0){
//        myHelper::ShowMessageBoxInfo(tr("本月未发生借方的进项税额！"));
//    }
//}


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

void JxTaxMgrDlg::on_btnApply_clicked()
{
    //从本月认证发票列表提取一个发票号，如果它存在于新增发票表中，则跳过，否则检查是否存在于历史缓存中，
    //如果存在，则将其加入待转换列表中（在最后步骤将基于此表创建转换凭证，即将暂存于应付账款下的金额计入到
    //进项税额中），否则加入到缺失发票列表中（即认证的发票没有在当前认证和历史未认证列表中出现）
    //持续迭代，直至最后一张发票。
    //如果转换列表不空，则创建新凭证
    //如果缺失列表不空，则提示用户缺失的凭证号
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
 * @brief 验证本月进项税归置是否正确
 */
void JxTaxMgrDlg::on_btnVerify_clicked()
{
    //VerifyCurInvoices();
}

/**
 * @brief 新增历史记录
 */
void JxTaxMgrDlg::on_actAddHis_triggered()
{
    initHistoryDatas();
    /*客户
     凭证（xxxx年xx月xxx#）发票号
    开票日期    税额    金额*/
    //输入前三项，自动查找指定凭证内的是否存在对应分录，如果找到则设置分录id，否则提示用户输入无效
//    QDialog dlg(this);
//    QLabel lClient(tr("客户："),&dlg);
//    QLineEdit eCSName(&dlg);
//    QLineEdit eLSName(&dlg);eLSName.setReadOnly(true);
//    QHBoxLayout lc;
//    lc.addWidget(&lClient,1); lc.addWidget(&eCSName,1); lc.addWidget(&eLSName,2);
//    QLabel lPz(tr("凭证号："),&dlg);
//    QDateEdit dePz(&dlg); dePz.displayFormat("yyyy-MM");
//    dePz.setMinimumDate(account->getStartDate());
//    dePz.setMaximumDate(account->getEndDate());
//    QSpinBox sbPzNum(&dlg); sbPzNum.setMinimum(1); sbPzNum.setMaximum(1000);
//    QLabel lInvoice(tr("发票号："),&dlg);
//    QLineEdit edInvoice(&dlg);edInvoice.setInputMask("99999999");
//    QHBoxLayout lp; lp.addWidget(&lPz); lp.addWidget(&dePz); lp.addWidget(&sbPzNum);
//    lp.addWidget(&lInvoice); lp.addWidget(&edInvoice);
//    QLabel lDate(tr("开票日期"),&dlg);
//    QDateEdit deDate(&dlg);deDate.setMinimumDate(account->getStartDate()); deDate.setMaximumDate(account->getEndDate());
//    QLabel lTax(tr("税额："),&dlg); QLineEdit edTax(&dlg);
//    QLabel lMoney(tr("金额"),&dlg); QLineEdit edMoney(&dlg);
//    QDoubleValidator dv; dv.setDecimals(2); edTax.setValidator(&dv); edMoney.setValidator(&dv);
//    QHBoxLayout lyTax; lyTax.addWidget(&lDate); lyTax.addWidget(&deDate); lyTax.addWidget(&lTax);
//    lyTax.addWidget(&edTax); lyTax.addWidget(&lMoney); lyTax.addWidget(&edMoney);
//    QPushButton btnOk(tr("确定"),&dlg); QPushButton btnCancel(tr("取消"),&dlg);
//    connect(&btnOk,SIGNAL(clicked(),&dlg,SLOT(accept()));
//    connect(&btnCancel,SIGNAL(clicked(),&dlg,SLOT(reject()));
//    QHBoxLayout lb; lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
//    QVBoxLayout *lm = new QVBoxLayout;
//    lm->addLayout(&lc); lm->addLayout(&lp); lm->addLayout(lyTax); lm->addLayout(&lb);
//    dlg.setLayout(lm);
//    dlg.show();
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
}

void JxTaxMgrDlg::on_btnInitHis_clicked()
{
    initHistoryDatas();
}

void JxTaxMgrDlg::on_btnSaveHis_clicked()
{
    if(!account->getDbUtil()->updateHisNotAuthCosInvoices(haInvoices)){
        myHelper::ShowMessageBoxError(tr("在保存历史未认证发票信息时发生错误！"));
        return;
    }
    if(!haInvoices_del.isEmpty() && !account->getDbUtil()->removeHisNotAuthCosInvoices(haInvoices_del))
        myHelper::ShowMessageBoxError(tr("在移除历史未认证发票信息时发生错误！"));
}

/**
 * @brief 保存本月认证发票信息
 */
void JxTaxMgrDlg::on_btnSaveCur_clicked()
{
    if(!account->getDbUtil()->updateCurAuthCostInvAmount(asMgr->year(),asMgr->month(),ui->edtTaxAmount->text().toDouble()))
        myHelper::ShowMessageBoxError(tr("在保存本期认证发票税额总数时发生错误！"));
    if(!account->getDbUtil()->saveCurAuthCostInvoices(asMgr->year(),asMgr->month(),caInvoices))
        myHelper::ShowMessageBoxError(tr("在保存本期认证发票税额信息时发生错误！"));
}

void JxTaxMgrDlg::on_chkInited_clicked(bool checked)
{
    ui->btnInitHis->setEnabled(!checked);
}

void JxTaxMgrDlg::on_actDelCur_triggered()
{
    int row = ui->twCurAuth->currentRow();
    delete caInvoices.at(row);
    caInvoices.removeAt(row);
    ui->twCurAuth->removeRow(row);
    calCurAuthTaxSum();
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
    asMgr->append(pz);
}

/**
 * @brief 将历史进项税计入本月认证的进项税
 */
void JxTaxMgrDlg::on_actJrby_triggered()
{
    int row = ui->twHistorys->currentRow();
    ui->twHistorys->removeRow(row);
    HisAuthCostInvoiceInfo* hi = haInvoices.takeAt(row);
    haInvoices_del<<hi;
    CurAuthCostInvoiceInfo* ci = new CurAuthCostInvoiceInfo;
    ci->isCur = false;
    ci->cName = hi->client->getName();
    ci->ni = hi->client->getNameItem();
    ci->inum = hi->iNum;
    ci->taxMoney = hi->taxMoney;
    ci->money = hi->money;
    caInvoices<<ci;
    row = caInvoices.count();
    turnDataInspect(false);
    ui->twCurAuth->insertRow(row-1);
    ui->twCurAuth->setItem(row-1,CICA_ISCUR,new QTableWidgetItem("=>"));
    ui->twCurAuth->setItem(row-1,CICA_NUM,new QTableWidgetItem(ci->inum));
    ui->twCurAuth->setItem(row-1,CICA_MONEY,new QTableWidgetItem(ci->money.toString()));
    ui->twCurAuth->setItem(row-1,CICA_TAX,new QTableWidgetItem(ci->taxMoney.toString()));
    ui->twCurAuth->setItem(row-1,CICA_CLIENT,new QTableWidgetItem(ci->cName));
    Double sum = Double(ui->twCurAuth->item(row,CICA_TAX)->text().toDouble());
    sum += ci->taxMoney;
    ui->twCurAuth->item(row,CICA_TAX)->setText(sum.toString());
    turnDataInspect();
}
