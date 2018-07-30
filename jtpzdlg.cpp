#include "jtpzdlg.h"
#include "ui_jtpzdlg.h"

#include "commdatastruct.h"
#include "PzSet.h"
#include "subject.h"

JtpzDlg::JtpzDlg(AccountSuiteManager *smg, QWidget *parent) :
    QDialog(parent),ui(new Ui::JtpzDlg),smg(smg)
{
    ui->setupUi(this);
}

JtpzDlg::~JtpzDlg()
{
    delete ui;
}

QList<JtpzDatas *> JtpzDlg::getDatas()
{
    QList<JtpzDatas*> datas;
    SubjectManager* sm = smg->getSubjectManager();

    QStringList errors;
    QString name;
    int g = 1;
    if(ui->gpSalary->isChecked()){
        JtpzDatas* d = new JtpzDatas;
        d->group = g++;
        d->jFsub = sm->getglfySub();  //管理费用
        if(!d->jFsub)
            errors<<tr("无法获取科目：管理费用，请配置基本库的特定科目表\n");
        name = tr("工资");
        d->jSsub = getSSubByName(name,d->jFsub);
        if(!d->jSsub)
            errors<<tr("无法获取科目：管理费用-工资\n");
        d->dFsub = sm->getFstSubject("2211"); //应付职工薪酬
        if(!d->dFsub)
            errors<<tr("无法获取科目：应付职工薪酬（2211）\n");
        d->dSsub = getSSubByName(name,d->dFsub);
        if(!d->dSsub)
            errors<<tr("无法获取科目：应付职工薪酬-%1\n").arg(name);
        d->summary = tr("计提本月工资");
        datas<<d;
    }
    if(ui->gpAdded->isChecked() && ui->chkAddedvaluetax->isChecked()){
        JtpzDatas* d = new JtpzDatas;
        d->group = g++;
        d->jFsub = sm->getYjsjSub();
        if(!d->jFsub)
            errors<<tr("无法获取科目：应交税费\n");
        d->jSsub = sm->getXxseSSub();
        if(!d->jSsub)
            errors<<tr("无法获取科目：应交税费-应交增值税（销项）\n");
        d->dFsub = d->jFsub;
        name = tr("未交增值税");
        d->dSsub = getSSubByName(name,d->dFsub);
        if(!d->dSsub)
            errors<<tr("无法获取科目：应交税费-%1\n").arg(name);
        d->dSsub2 = sm->getJxseSSub();
        if(!d->dSsub2)
            errors<<tr("无法获取科目：应交税费-应交增值税（进项）\n");
        d->summary = tr("计提本月增值税");
        datas<<d;
    }
    if(ui->gpOther->isChecked()){
        if(ui->chkCbj->isChecked()){
            JtpzDatas* d = new JtpzDatas;
            d->group = g;
            d->jFsub = sm->getglfySub();
            if(!d->jFsub)
                errors<<tr("无法获取科目：管理费用，请配置基本库的特定科目表\n");
            name = tr("残保金");
            d->jSsub = getSSubByName(name,d->jFsub);
            if(!d->jSsub)
                errors<<tr("无法获取科目：管理费用-%1\n").arg(name);
            d->dFsub = sm->getYjsjSub();
            if(!d->dFsub)
                errors<<tr("无法获取科目：应交税费\n");
            d->dSsub = getSSubByName(name,d->dFsub);
            if(!d->dSsub)
                errors<<tr("无法获取科目：应交税费-%1\n").arg(name);
            d->summary = tr("计提本月残保金");
            datas<<d;
        }
        if(ui->chkStampDuty->isChecked()){
            JtpzDatas* d = new JtpzDatas;
            d->group = g;
            d->jFsub = sm->getglfySub();
            if(!d->jFsub)
                errors<<tr("无法获取科目：管理费用，请配置基本库的特定科目表\n");
            name = tr("印花税");
            d->jSsub = getSSubByName(name,d->jFsub);
            if(!d->jSsub)
                errors<<tr("无法获取科目：管理费用-%1\n").arg(name);
            d->dFsub = sm->getYjsjSub();
            if(!d->dFsub)
                errors<<tr("无法获取科目：应交税费\n");
            name = tr("应交印花税");
            d->dSsub = getSSubByName(name,d->dFsub);
            if(!d->dSsub)
                errors<<tr("无法获取科目：应交税费-%1\n").arg(name);
            d->summary = tr("计提本月印花税");
            datas<<d;
        }
        if(ui->chkCity->isChecked()){
            JtpzDatas* d = new JtpzDatas;
            d->group = g;
            d->jFsub = sm->getFstSubject("6403"); //
            if(!d->jFsub)
                errors<<tr("无法获取科目：营业税金及附加（6403）\n");
            name = tr("城建税");
            d->jSsub = getSSubByName(name,d->jFsub);
            if(!d->jSsub)
                errors<<tr("无法获取科目：营业税金及附加-%1\n").arg(name);
            d->dFsub = sm->getYjsjSub();
            if(!d->dFsub)
                errors<<tr("无法获取科目：应交税费\n");
            name = tr("应交城市维护建设税");
            d->dSsub = getSSubByName(name,d->dFsub);
            if(d->dSsub)
                errors<<tr("无法获取科目：应交税费-%1\n").arg(name);
            d->summary = tr("计提本月城建税");
            datas<<d;
        }
        if(ui->chkEducation->isChecked()){
            JtpzDatas* d = new JtpzDatas;
            d->group = g;
            d->jFsub = sm->getFstSubject("6403"); //营业税金及附加
            if(!d->jFsub)
                errors<<tr("无法获取科目：营业税金及附加（6403）\n");
            name = tr("教育费附加");
            d->jSsub = getSSubByName(name,d->jFsub);
            if(!d->jSsub)
                errors<<tr("无法获取科目：营业税金及附加-%1\n").arg(name);
            d->dFsub = sm->getYjsjSub();
            if(!d->dFsub)
                errors<<tr("无法获取科目：应交税费\n");
            d->dSsub = getSSubByName(name,d->dFsub);
            if(!d->dSsub)
                errors<<tr("无法获取科目：应交税费-%1\n").arg(name);
            d->summary = tr("计提本月教育附加费");
            datas<<d;
        }
    }
    return datas;
}


SecondSubject *JtpzDlg::getSSubByName(QString name, FirstSubject *fsub)
{
    foreach(SecondSubject* ssub, fsub->getChildSubs()){
        if(ssub->getName() == name)
            return ssub;
    }
    return 0;
}
