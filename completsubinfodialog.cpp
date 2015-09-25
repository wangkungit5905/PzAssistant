#include <QSqlQuery>
#include <QKeyEvent>

#include "completsubinfodialog.h"
#include "ui_completsubinfodialog.h"

#include "tables.h"
#include "subject.h"
#include "config.h"

CompletSubInfoDialog::CompletSubInfoDialog(int fid,SubjectManager* smg,QWidget *parent) : QDialog(parent),
    ui(new Ui::CompletSubInfoDialog)
{
    ui->setupUi(this);

    //首先获取需要作考虑的一些一级科目的id
    int gid = smg->getGdzcSub()->getId();

    //将业务客户类别设置为默认
    int nameCls;   //默认名称类别的代码
    AppConfig* conf = AppConfig::getInstance();
    if(fid == gid)
        nameCls = conf->getSpecNameItemCls(AppConfig::SNIC_GDZC);
    else
        nameCls = conf->getSpecNameItemCls(AppConfig::SNIC_COMMON_CLIENT);
    QHash<int,QStringList> nameClses = smg->getAllNICls();
    QList<int> codes;
    codes = nameClses.keys();
    qSort(codes.begin(),codes.end());
    int index,code;
    for(int i = 0;i < codes.count();++i){
        code = codes.at(i);
        ui->cmbClass->addItem(nameClses.value(code).first(),code);
        if(code == nameCls)
            index = i;
    }
    ui->cmbClass->setCurrentIndex(index);
    ui->edtSName->setFocus();
}

CompletSubInfoDialog::~CompletSubInfoDialog()
{
    delete ui;
}

void CompletSubInfoDialog::setName(QString name)
{
    ui->edtSName->setText(name);
}

void CompletSubInfoDialog::setLongName(QString name)
{
    ui->edtLName->setText(name);
}

void CompletSubInfoDialog::setRemCode(QString code)
{
    ui->edtRemCode->setText(code);
}

void CompletSubInfoDialog::setNameClass(int clsCode)
{
    ui->cmbClass->setCurrentIndex(ui->cmbClass->findData(clsCode));
}

QString CompletSubInfoDialog::getSName()
{
    return ui->edtSName->text();
}

QString CompletSubInfoDialog::getLName()
{
    return ui->edtLName->text();
}

QString CompletSubInfoDialog::getRemCode()
{
    return ui->edtRemCode->text();
}

int CompletSubInfoDialog::getSubCalss()
{
    int idx = ui->cmbClass->currentIndex();
    return ui->cmbClass->itemData(idx).toInt();
}

void CompletSubInfoDialog::keyPressEvent(QKeyEvent *event)
{
    int k = event->key();
    if((k == Qt::Key_Return) || (k == Qt::Key_Enter)){
        if(ui->edtSName->hasFocus() && (ui->edtSName->text() != "")){
            if(ui->edtLName->text().isEmpty())
                ui->edtLName->setText(ui->edtSName->text());
            ui->edtLName->selectAll();
            ui->edtLName->setFocus();
        }
        else if(ui->edtLName->hasFocus() && (ui->edtLName->text() != ""))
            ui->edtRemCode->setFocus();
        else if(ui->edtRemCode->hasFocus() && (ui->edtRemCode->text() != ""))
            ui->cmbClass->setFocus();
        else if(ui->cmbClass->hasFocus())
            ui->btnOk->setFocus();
    }
}

