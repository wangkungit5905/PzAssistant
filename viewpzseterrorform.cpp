#include <QDebug>
#include <QSettings>
#include <QTextCodec>

#include "viewpzseterrorform.h"
#include "ui_viewpzseterrorform.h"
#include "tables.h"
#include "common.h"
#include "utils.h"
#include "subject.h"
#include "dbutil.h"
#include "pz.h"
#include "PzSet.h"

//InspectPzErrorThread::InspectPzErrorThread(int y, int m, PzSetMgr *pzMgr, QObject *parent):
//    QThread(parent),year(y),month(m),pzMgr(pzMgr)
//{
//    account = pzMgr->getAccount();
//    smg = pzMgr->getSubjectManager();
//}


/**
 * @brief InspectPzErrorThread::run
 * 凭证集错误检测，要检测的错误类别包括（标题分别代表错误编码）：
 * （1）警告级：
 *      1）自编号为空
 *      2）凭证号不连续
 *      3）自编号重复等
 *      4）摘要为空
 * （2）错误级：
 *      1）借贷金额不平衡
 *      2）科目未设置
 *      3）币种设置错误（在不允许采用外币的科目上使用了外币）
 *      4）借贷方向约定性错误（比如财务费用只能出现在手工凭证的借方，结转损益的贷方...）
 *      5）金额未设置
 */
//void InspectPzErrorThread::run()
//{
//    PingZhengError* e = NULL;
//    QSet<int> zbNums;   //已使用的自编号集合
//    QString ds,s1,s2,errStr;
//    QSqlQuery q1(db),q2(db);
//    int pid,bid,fsid,ssid,mt;
//    PzClass pc;
//    int pzNum = 0, pzAmount = 0; int baNum;
//    int tnum; int step = 0; int dir;
//    Double vj,vd;

//    es.clear();

//    //报告要扫描的凭证数
//    ds = QDate(year,month,1).toString(Qt::ISODate);
//    ds.chop(3);
//    s1 = QString("select count() from %1 where %2 like '%3%'")
//            .arg(tbl_pz).arg(fld_pz_date).arg(ds);
//    if(!q1.exec(s1)){
//        qDebug()<<QString("Exec sql error:  %1").arg(s1);
//        return;
//    }
//    q1.first();
//    pzAmount = q1.value(0).toInt();
//    if(pzAmount == 0)
//        return;
//    //emit adviceAmount(tnum);

//    s1 = QString("select * from %1 where %2 like '%3%' order by %4")
//            .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_number);
//    if(!q1.exec(s1)){
//        qDebug()<<QString("Exec sql error:  %1").arg(s1);
//        return;
//    }

//    while(q1.next()){
//        step++;
//        emit adviceProgressStep(step,pzAmount);
//        pid = q1.value(0).toInt();
//        pc = (PzClass)q1.value(PZ_CLS).toInt();

//        //凭证号连续性检测
//        tnum = q1.value(PZ_NUMBER ).toInt();
//        if(tnum - pzNum != 1){
//            e = new PingZhengError;
//            e->errorLevel = PZE_WARNING;
//            e->errorType = 2;
//            e->pid = pid;
//            e->bid = 0;
//            e->pzNum = tnum;
//            e->baNum = 0;
//            e->explain = tr("在%1号凭证前存在不连续的凭证号！").arg(tnum);
//            es<<e;
//        }
//        pzNum = tnum;

//        tnum = q1.value(PZ_ZBNUM).toInt();
//        //自编号是否为空检测
//        if(tnum == 0){
//            e = new PingZhengError;
//            e->errorLevel = PZE_WARNING;
//            e->errorType = 1;
//            e->pid = pid;
//            e->bid = 0;
//            e->pzNum = pzNum;
//            e->baNum = 0;
//            e->explain = tr("%1号凭证的自编号未设置！").arg(pzNum);
//            es<<e;
//        }
//        else if(zbNums.contains(tnum)){ //自编号重复性检测
//            e = new PingZhengError;
//            e->errorLevel = PZE_WARNING;
//            e->errorType = 3;
//            e->pid = pid;
//            e->bid = 0;
//            e->pzNum = tnum;
//            e->baNum = 0;
//            e->explain = tr("%1号凭证的自编号与其他凭证重复！").arg(tnum);
//            es<<e;
//        }

//        vj = Double(q1.value(PZ_JSUM).toDouble());
//        vd = Double(q1.value(PZ_DSUM).toDouble());
//        if(vj != vd){
//            e = new PingZhengError;
//            e->errorLevel = PZE_ERROR;
//            e->errorType = 1;
//            e->pid = pid;
//            e->bid = 0;
//            e->pzNum = pzNum;
//            e->baNum = 0;
//            e->explain = tr("%1号凭证借贷不平衡！").arg(pzNum);
//            es<<e;
//        }

//        s2 = QString("select * from %1 where pid = %2").arg(tbl_ba).arg(pid);
//        if(!q2.exec(s2)){
//            qDebug()<<QString("Exec sql error:  %1").arg(s2);
//            return;
//        }
//        while(q2.next()){
//            bid = q2.value(0).toInt();
//            baNum = q2.value(BACTION_NUMINPZ).toInt();
//            mt = q2.value(BACTION_MTYPE).toInt();
//            dir = q2.value(BACTION_DIR).toInt();

//            //摘要栏是否为空检测
//            if(q2.value(BACTION_SUMMARY).toString().isEmpty()){
//                e = new PingZhengError;
//                e->errorLevel = PZE_WARNING;
//                e->errorType = 4;
//                e->pid = pid;
//                e->bid = bid;
//                e->pzNum = pzNum;
//                e->baNum = baNum;
//                e->explain = tr("%1号凭证第%2条会计分录没有设置摘要！").arg(pzNum).arg(baNum);
//                es<<e;
//            }

//            //科目是否为空检测
//            fsid = q2.value(BACTION_FID).toInt();
//            ssid = q2.value(BACTION_SID).toInt();
//            if(fsid == 0 || ssid == 0){
//                e = new PingZhengError;
//                e->errorLevel = PZE_ERROR;
//                e->errorType = 2;
//                e->pid = pid;
//                e->bid = bid;
//                e->pzNum = pzNum;
//                e->baNum = baNum;
//                e->explain = tr("%1号凭证第%2条会计分录科目未设置！").arg(pzNum).arg(baNum);
//                es<<e;
//            }

//            //金额是否设置检测
//            if((q2.value(BACTION_VALUE).toDouble() == 0) /*&&
//               (q2.value(BACTION_VALUE).toDouble() == 0)*/){
//                e = new PingZhengError;
//                e->errorLevel = PZE_ERROR;
//                e->errorType = 5;
//                e->pid = pid;
//                e->bid = bid;
//                e->pzNum = pzNum;
//                e->baNum = baNum;
//                e->explain = tr("%1号凭证第%2条会计分录金额未设置！").arg(pzNum).arg(baNum);
//                es<<e;
//            }

//            //借贷方向约定性错误检测
//            if(!inspectDirEngage(fsid,dir,pc,errStr)){
//                e = new PingZhengError;
//                e->errorLevel = PZE_ERROR;
//                e->errorType = 4;
//                e->pid = pid;
//                e->bid = bid;
//                e->pzNum = pzNum;
//                e->baNum = baNum;
//                e->explain = errStr;
//                es<<e;
//            }
//            //币种设置错误检测
//            if(!inspectMtError(fsid,ssid,mt)){
//                e = new PingZhengError;
//                e->errorLevel = PZE_ERROR;
//                e->errorType = 3;
//                e->pid = pid;
//                e->bid = bid;
//                e->pzNum = pzNum;
//                e->baNum = baNum;
//                e->explain = tr("%1号凭证第%2条会计分录币种设置错误！").arg(pzNum).arg(baNum);
//                es<<e;
//            }
//        }
//    }
//}

/**
 * @brief MainWindow::inspectMtError
 *  币种设置错误检测（外币只使用在需要按币种进行分别核算的科目上）
 * @param fsid   一级科目id
 * @param ssid   二级科目id
 * @param mt     已设置的币种代码
 * @return       true：正确，false：错误
 */
//bool InspectPzErrorThread::inspectMtError(int fsid, int ssid, int mt)
//{
//    FirstSubject* fsub = smg->getFstSubject(fsid);
//    if(fsub->isUseForeignMoney() && account->getMasterMt()->code())
//        return false;
//    return true;
//}

/**
 * @brief MainWindow::inspectDirEngage
 *  借贷方向约定正确性检测：
 *  1、手工凭证：对于损益类的收入型科目只能出现在贷方，成本型科目只能出现在借方；
 *  2、特种凭证：
 *  （1）结转汇兑损益类凭证：
 *      结转的科目（银行、应收和应付）在贷方，财务费用科目在借方；
 *  （2）结转损益类凭证：
 *      结转收入时，收入在借方，本年利润在贷方
 *      结转成本时，成本在贷方，本年利润在借方
 *  （3）结转利润类凭证：
 *      本年利润在借方，利润分配在贷方
 * @param fsid  一级科目id
 * @param dir   借贷方向
 * @param pzc   凭证类别
 * @return      true：正确，false：错误
 */
//bool InspectPzErrorThread::inspectDirEngage(int fsid, int dir, PzClass pzc, QString& eStr)
//{
//    SubjectManager* sm = curAccount->getSubjectManager();
//    if(pzc == Pzc_Hand){
//        if(sm->isSySubject(fsid)){
//            //if(BusiUtil::isInSub(fsid) && dir == DIR_J){
//            if(!sm->getFstSubject(fsid)->getJdDir() && dir == DIR_J){
//                eStr = tr("手工凭证中，收入类科目必须在贷方");
//                return false;
//            }
//            //else if(BusiUtil::isFeiSub(fsid) && dir == DIR_D){
//            else if(sm->getFstSubject(fsid)->getJdDir() && dir == DIR_D){
//                eStr = tr("手工凭证中，费用类科目必须在借方");
//                return false;
//            }
//            else
//                return true;
//        }
//        else
//            return true;
//    }
//    //else if(pzc == Pzc_Jzhd_Bank || pzc == Pzc_Jzhd_Yf || pzc == Pzc_Jzhd_Ys){
//    if(pzClsJzhds.contains(pzc)){
//        if(sm->getFstSubject(fsid)->isUseForeignMoney() && (dir == DIR_J)){
//            eStr = tr("结转汇兑损益类凭证中，拟结转科目必须在贷方");
//            return false;
//        }
//        else if(fsid == sm->getCwfySub()->getId() && (dir == DIR_D)){
//            eStr = tr("结转汇兑损益类凭证中，财务费用科目必须在借方");
//            return false;
//        }
//        else
//            return true;
//    }
//    else if(pzc == Pzc_JzsyIn){
//        //if(BusiUtil::isInSub(fsid) && dir == DIR_D){
//        if(!sm->getFstSubject(fsid)->getJdDir() && dir == DIR_D){
//            eStr = tr("结转收入的凭证中，收入类科目必须在借方");
//            return false;
//        }
//        //else if(fsid == sm->getBnlrId() && dir == DIR_J){
//        else if(fsid = sm->getBnlrSub()->getId() && dir == DIR_J){
//            eStr = tr("结转收入的凭证中，本年利润必须在贷方");
//            return false;
//        }
//        else
//            return true;
//    }
//    else if(pzc == Pzc_JzsyFei){
//        //if(BusiUtil::isFeiSub(fsid) && dir == DIR_J){
//        if(!sm->getFstSubject(fsid)->getJdDir() && dir == DIR_J){
//            eStr = tr("结转费用的凭证中，费用类科目必须在贷方");
//            return false;
//        }
//        else if(fsid == sm->getBnlrSub()->getId() && dir == DIR_D){
//            eStr = tr("结转费用的凭证中，本年利润必须在借方");
//            return false;
//        }
//        else
//            return true;
//    }
//    else if(pzc == Pzc_Jzlr){
//        if(fsid == sm->getBnlrSub()->getId() && dir == DIR_D){
//            eStr = tr("结转利润的凭证中，本年利润必须在借方");
//            return false;
//        }
//        else if(fsid == sm->getLrfpSub()->getId() && dir == DIR_J){
//            return false;
//            eStr = tr("结转利润的凭证中，利润分配必须在贷方");
//        }
//        else
//            return true;
//    }
//}

//////////////////////////////ViewPzSetErrorForm///////////////////////////////////
ViewPzSetErrorForm::ViewPzSetErrorForm(AccountSuiteManager *pzMgr, QByteArray* state, QWidget *parent) :
    QDialog(parent),ui(new Ui::ViewPzSetErrorForm),pzMgr(pzMgr)
{
    ui->setupUi(this);
    account = pzMgr->getAccount();
    QSettings setting("./config/infos/errors.ini", QSettings::IniFormat);
    #ifdef Q_OS_WIN32
        setting.setIniCodec(QTextCodec::codecForName("UTF-8"));
    #else
        setting.setIniCodec(QTextCodec::codecForLocale());
    #endif
    setting.setIniCodec(QTextCodec::codecForLocale());

    QString key = "levels";
    setting.beginGroup(key);
    QStringList keys = setting.childKeys();
    ui->cmbLevel->addItem(tr("所有"),0);
    foreach(key, keys){
        QString c = setting.value(key).toString();
        ui->cmbLevel->addItem(setting.value(key).toString(),key.toInt());
    }
    setting.endGroup();

    key = "pz_warning";
    setting.beginGroup(key);
    keys = setting.childKeys();
    foreach(key,keys)
        wInfos[key.toInt()] = setting.value(key).toString();
    setting.endGroup();

    key = "pz_error";
    setting.beginGroup(key);
    keys = setting.childKeys();
    foreach(key,keys)
        eInfos[key.toInt()] = setting.value(key).toString();
    setting.endGroup();

    curLevel = 0;
    connect(ui->tw,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(doubleClicked(int,int)));
    ui->tw->setColumnHidden(5, true);
    ui->tw->setColumnWidth(3,200);
    inspect();
}

ViewPzSetErrorForm::~ViewPzSetErrorForm()
{
    delete ui;
    foreach(PingZhengError *p, es)
        delete p;
    es.clear();
}

void ViewPzSetErrorForm::inspect()
{
    ui->btnInspect->setEnabled(false);
    qDeleteAll(es);
    es.clear();
    if(!pzMgr->inspectPzError(es)){
        QMessageBox::critical(this, tr("错误信息"),tr("在检测凭证错误时发生错误！"));
        return;
    }
    viewErrors();
    ui->btnInspect->setEnabled(true);
}

//void ViewPzSetErrorForm::setErrors(QList<PingZhengError *> es)
//{
//    this->es = es;
//    viewErrors();

//}

void ViewPzSetErrorForm::doubleClicked(int row, int column)
{
    //应该用pz的id和会计分录的id，在表格中添加一个隐藏列，保存双击的错误行对应的错误条目在es列表中的序号
    int index = ui->tw->item(row,5)->text().toInt();
    PingZhengError* p = es.at(index);
    emit reqLoation(p->pz->id(),p->ba?p->ba->getId():0);
}

/**
 * @brief ViewPzSetErrorForm::viewStep
 *  显示检查进度
 * @param step
 * @param AmountStep
 */
//void ViewPzSetErrorForm::viewStep(int step, int AmountStep)
//{
//    ui->lblState->setText(tr("正在检查......%1%").arg(step/AmountStep));
//}

/**
 * @brief ViewPzSetErrorForm::inspcetPzErrEnd
 *  检查凭证错误的线程结束
 */
//void ViewPzSetErrorForm::inspcetPzErrEnd()
//{
//    ui->btnInspect->setEnabled(true);
//    ui->lblState->setText("");
//    es = t->getErrors();
//    if(es.empty()){
//        ui->lblState->setText(tr("未发现任何错误"));
//        return;
//    }
//    viewErrors();
//}

/**
 * @brief ViewPzSetErrorForm::viewErrors
 *  显示选定级别的错误信息
 */
void ViewPzSetErrorForm::viewErrors()
{
    ui->tw->clearContents();
    ui->tw->setRowCount(0);
    if(es.empty()){
        ui->lblState->setText(tr("未发现任何错误"));
        return;
    }
    ui->lblState->setText("");
    QTableWidgetItem* item;
    PingZhengError* pe;
    int row = -1;
    for(int i = 0; i < es.count(); ++i){
        pe = es[i];
        if(curLevel != 0 && pe->errorLevel != curLevel)
            continue;

        ui->tw->insertRow(i); row++;
        if(pe->errorLevel == PZE_WARNING)
            item = new QTableWidgetItem(tr("警告"));
        else
            item = new QTableWidgetItem(tr("错误"));
        ui->tw->setItem(row,0,item);
        item = new QTableWidgetItem(QString::number(pe->pz->number()));
        ui->tw->setItem(row,1,item);
        if(!pe->ba)
            item = NULL;
        else
            item = new QTableWidgetItem(QString::number(pe->ba->getNumber()));
        ui->tw->setItem(row,2,item);
        if(pe->errorLevel == PZE_WARNING)
            item = new QTableWidgetItem(wInfos.value(pe->errorType));
        else
            item = new QTableWidgetItem(eInfos.value(pe->errorType));
        ui->tw->setItem(row,3,item);
        if(pe->explain.isEmpty())
            ui->tw->setItem(row,4,NULL);
        else
            ui->tw->setItem(row,4, new QTableWidgetItem(pe->explain));
        ui->tw->setItem(row,5,new QTableWidgetItem(QString::number(i)));
    }
}

/**
 * @brief ViewPzSetErrorForm::on_cmbLevel_currentIndexChanged
 *  选择显示的错误信息级别
 * @param index
 */
void ViewPzSetErrorForm::on_cmbLevel_currentIndexChanged(int index)
{
    curLevel = ui->cmbLevel->itemData(index).toInt();
    viewErrors();
}

/**
 * @brief ViewPzSetErrorForm::on_btnInspect_clicked
 *  检查凭证错误按钮按下
 */
void ViewPzSetErrorForm::on_btnInspect_clicked()
{
    inspect();
}
