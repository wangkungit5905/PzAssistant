#include <QFileDialog>

#include "impbeginextradlg.h"
#include "ui_impbeginextradlg.h"
#include "myhelper.h"
#include "xlsxworksheet.h"
#include "subject.h"
#include "dbutil.h"



ImpBeginExtraDlg::ImpBeginExtraDlg(Account *account, QWidget *parent) :
    QDialog(parent),ui(new Ui::ImpBeginExtraDlg),account(account),excel(0)
{
    ui->setupUi(this);
    ui->twYs->setColumnWidth(CI_CLIENT, 300);
    ui->twYf->setColumnWidth(CI_CLIENT, 300);
    sm = account->getSubjectManager();
    ysFSub = sm->getYsSub();
    yfFSub = sm->getYfSub();
    globalNameItems = sm->getAllNameItems(SORTMODE_NAME);
    QHashIterator<int, QStringList> it(sm->getAllNICls());
    while (it.hasNext()) {
        it.next();
        if(it.value().first() == tr("客户")){
            clientClsCode = it.key();
            break;
        }
    }
}

ImpBeginExtraDlg::~ImpBeginExtraDlg()
{
    delete ui;
}

void ImpBeginExtraDlg::on_btnBrowse_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,tr("选择余额表文件"),".","*.xlsx");
    if(filename.isEmpty())
        return;
    ui->edtFille->setText(filename);
    if(excel){
        delete excel;
        excel = 0;
    }
    excel = new QXlsx::Document(filename,this);
    ui->twYs->clear();
    ui->twYf->clear();
    QStringList sheets = excel->sheetNames();
    if(sheets.isEmpty())
        return;
    sheetYs = (Worksheet*)excel->sheet(tr("应收"));
    sheetYf = (Worksheet*)excel->sheet(tr("应付"));
    if(!sheetYs || !sheetYf){
        myHelper::ShowMessageBoxWarning(tr("文件中没有找到应收或应付表单"));
        return;
    }

    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
    readExtra();
    readExtra(false);
    qApp->restoreOverrideCursor();
}


void ImpBeginExtraDlg::readExtra(bool isYs)
{
    QTableWidget* tw=0;
    QString info;
    Worksheet* sheet=0;
    if(isYs){
        tw = ui->twYs;
        sheet = sheetYs;
        info = tr("应收");
    }
    else{
        tw = ui->twYf;
        sheet = sheetYf;
        info = tr("应付");
    }

    int colSubCode=-1,colClientName=-1,colJValue=-1,colDValue=-1;
    for(int c = 1; c < 12; c++){
        QString title =  sheetYs->read(1,c).toString();
        if(title == tr("科目代码"))
            colSubCode = c;
        else if(title == tr("科目名称"))
            colClientName = c;
        else if(title == tr("期末借方余额"))
            colJValue = c;
        else if(title == tr("期末贷方余额"))
            colDValue = c;
        else
            continue;
    }
    if(colSubCode == -1 || colClientName == -1 || colJValue == -1 || colDValue == 0){
        info = tr("%1%2").arg(info).arg(tr("表单必须有这些列：科目代码，科目名称，期末借方余额，期末贷方余额"));
        myHelper::ShowMessageBoxWarning(info);
        return;
    }
    QString subCode,clientName;
    Double JV,DV;
    int row = 2;
    while(true){
        subCode = sheet->read(row,colSubCode).toString();
        if(subCode.isEmpty())
            break;
        clientName = sheet->read(row,colClientName).toString();
        JV = Double(sheet->read(row,colJValue).toDouble());
        DV = Double(sheet->read(row,colDValue).toDouble());
        appendExtraRow(subCode,clientName,JV,DV,tw);
        row++;
    }
}

void ImpBeginExtraDlg::appendExtraRow(QString code, QString clientName, Double jv, Double dv, QTableWidget *tw)
{
    FirstSubject* fsub;
    QList<SecondSubject*>* subjects;
    SecondSubject* ssub = 0;
    SubjectNameItem* ni;
    if(tw == ui->twYs){
        subjects = &ysSndSubjects;
        fsub = ysFSub;
    }
    else {
        subjects = &yfSndSubjects;
        fsub = yfFSub;
    }
    int row = tw->rowCount();
    tw->insertRow(row);
    QTableWidgetItem* item = new QTableWidgetItem(code);
    tw->setItem(row,CI_CODE,item);
    item = new QTableWidgetItem(clientName);
    tw->setItem(row,CI_CLIENT,item);
    item = new QTableWidgetItem(jv.toString());
    tw->setItem(row,CI_JV,item);
    item = new QTableWidgetItem(dv.toString());
    tw->setItem(row,CI_DV,item);
    ni = getMacthNameItem(clientName);
    if(!ni){
        ni = new SubjectNameItem();
        ni->setShortName(clientName);
        ni->setLongName(clientName);
        ni->setClassId(clientClsCode);
        nameItems.append(ni);
    }
    if(jv == 0.0 && dv == 0.0)
        return;
    ssub = getMatchSubject(ni,fsub);
    if(!ssub){
        ssub = new SecondSubject();
        ssub->setNameItem(ni);
        fsub->addChildSub(ssub);
        subjects->append(ssub);
    }
    QVariant v;
    v.setValue<SecondSubject*>(ssub);
    tw->item(row,CI_CLIENT)->setData(Qt::UserRole,v);
}

SubjectNameItem* ImpBeginExtraDlg::getMacthNameItem(QString longName)
{
    for(int i = 0; i < nameItems.count(); ++i){
        if(longName == nameItems.at(i)->getLongName())
            return nameItems.at(i);
    }
    for(int i = 0; i < globalNameItems.count(); ++i){
        if(longName == globalNameItems.at(i)->getLongName())
            return globalNameItems.at(i);
    }
    return 0;
}

SecondSubject* ImpBeginExtraDlg::getMatchSubject(SubjectNameItem *ni,FirstSubject* fsub)
{
    QList<SecondSubject*> subjects;
    if(fsub == ysFSub)
        subjects = ysSndSubjects;
    else
        subjects = yfSndSubjects;
    for(int i = 0; i < subjects.count(); ++i){
        if(ni->getLongName() == subjects.at(i)->getLName())
            return subjects.at(i);
    }

    QList<SecondSubject*> subs = fsub->getChildSubs(SORTMODE_NAME);
    for(int i = 0; i < subs.count(); ++i){
        if(ni->getLongName() == subs.at(i)->getLName())
            return subs.at(i);
    }
    return 0;
}

void ImpBeginExtraDlg::execSum(bool isYs)
{
    QTableWidget* tw;
    Double sum;
    int md = account->getMasterMt()->code();
    if(isYs)
        tw = ui->twYs;
    else
        tw = ui->twYf;
    for(int r = 0; r < tw->rowCount(); ++r){
        SecondSubject* ssub = 0;
        ssub = tw->item(r,CI_CLIENT)->data(Qt::UserRole).value<SecondSubject*>();
        if(!ssub)
            continue;
        MoneyDirection dir = MDIR_P;
        Double v;
        if(!tw->item(r,CI_JV)->text().isEmpty()){
            v = Double(tw->item(r,CI_JV)->text().toDouble());
            sum += v;
            if(isYs)
                pvs_ys[ssub->getId()*10 + md] += v;
            else
                pvs_yf[ssub->getId()*10 + md] += v;
        }
        else{
            v = Double(tw->item(r,CI_DV)->text().toDouble());
            sum -= v;
            if(isYs)
                pvs_ys[ssub->getId()*10 + md] -= v;
            else
                pvs_yf[ssub->getId()*10 + md] -= v;
        }


    }
    //考虑到有可能存在重复科目，所以方向要最后确定
    QHashIterator<int,Double>* it;
    if(isYs)
        it = new QHashIterator<int,Double>(pvs_ys);
    else
        it = new QHashIterator<int,Double>(pvs_yf);
    while(it->hasNext()){
        it->next();
        if(it->value() == 0.0)
            if(isYs)
                dirs_ys[it->key()] = MDIR_P;
            else
                dirs_yf[it->key()] = MDIR_P;
        else if(it->value() > 0.0)
            if(isYs)
                dirs_ys[it->key()] = MDIR_J;
            else
                dirs_yf[it->key()] = MDIR_J;
        else{
            if(isYs){
                dirs_ys[it->key()] = MDIR_D;
                pvs_ys[it->key()].changeSign();
            }
            else{
                dirs_yf[it->key()] = MDIR_D;
                pvs_yf[it->key()].changeSign();
            }
        }
    }


    if(isYs)
        ys_f[md] = sum;
    else
        yf_f[md] = sum;
    if(sum == 0){
        if(isYs)
            dir_f_ys[md] = MDIR_P;
        else
            dir_f_yf[md] = MDIR_P;
    }
    else if(sum > 0){
        if(isYs)
            dir_f_ys[md] = MDIR_J;
        else
            dir_f_yf[md] = MDIR_J;
    }
    else{
        if(isYs)
            dir_f_ys[md] = MDIR_D;
        else
            dir_f_yf[md] = MDIR_D;
    }

}

void ImpBeginExtraDlg::on_btnImport_clicked()
{
    DbUtil* dbutil = account->getDbUtil();
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
    foreach (SubjectNameItem* ni, nameItems) {
        if(!dbutil->saveNameItem(ni)){
            myHelper::ShowMessageBoxError(tr("保存名称条目%1时出错！").arg(ni->getLongName()));
            return;
        }
    }
    if(!dbutil->saveSndSubjects(ysSndSubjects) || !dbutil->saveSndSubjects(yfSndSubjects)){
        myHelper::ShowMessageBoxError(tr("保存应收或应付子目时出错！"));
        return;
    }
    execSum();
    execSum(false);
    AccountSuiteRecord* sr = account->getStartSuiteRecord();
    int y = sr->year;
    int m = sr->startMonth-1;
    if(!sr)
        return;
    if(!dbutil->saveExtraForAllSSubInFSub(y,m,ysFSub,ys_f,ys_f,dir_f_ys,pvs_ys,pvs_ys,dirs_ys)){
        myHelper::ShowMessageBoxError(tr("在保存应收期初余额时出错！"));
    }
    if(!dbutil->saveExtraForAllSSubInFSub(y,m,yfFSub,yf_f,yf_f,dir_f_yf,pvs_yf,pvs_yf,dirs_yf)){
        myHelper::ShowMessageBoxError(tr("在保存应付期初余额时出错！"));
    }
    qApp->restoreOverrideCursor();
}
