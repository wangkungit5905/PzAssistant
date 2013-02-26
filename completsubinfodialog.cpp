#include <QSqlQuery>
#include <QKeyEvent>

#include "completsubinfodialog.h"
#include "ui_completsubinfodialog.h"

#include "utils.h"

CompletSubInfoDialog::CompletSubInfoDialog(int fid,QWidget *parent) : QDialog(parent),
    ui(new Ui::CompletSubInfoDialog)
{
    ui->setupUi(this);

    QSqlQuery q;
    QString s;
    QString defClsName;  //默认类别名

    //首先获取需要作考虑的一些一级科目的id
    int gid;       //固定资产id
    BusiUtil::getIdByCode(gid,"1501");

    //将业务客户类别设置为默认
    if(fid == gid)
        defClsName = tr("固定资产类");
    else
        defClsName = tr("业务客户");

    s = "select clsCode, name from SndSubClass";
    bool r = q.exec(s);
    int idx,i = 0 ;
    if(r){
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            if(name == defClsName)
                idx = i;
            else
                i++;
            ui->cmbClass->addItem(name, code);
        }
    }

    ui->cmbClass->setCurrentIndex(idx);
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

