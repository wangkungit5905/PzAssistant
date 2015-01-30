#include <QSettings>
#include <QTreeWidget>
#include <QTextCodec>

#include "aboutform.h"
#include "ui_aboutform.h"
#include "config.h"
#include "securitys.h"

AboutForm::AboutForm(QString copyRightText, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutForm)
{
    ui->setupUi(this);

    WorkStation* mac = AppConfig::getInstance()->getLocalStation();
    if(mac)
        ui->edtWsName->setText(mac->name());
    else
        ui->edtWsName->setText(tr("本站未知"));
    QSettings setting(":files/ini/revisionHistorys.ini",QSettings::IniFormat);
    setting.setIniCodec(QTextCodec::codecForName("utf-8"));
    QString key = "revisions";
    QStringList keys;
    setting.beginGroup(key);
    keys = setting.childKeys();
    keys.sort();
    QString maxBuild = keys.last();  //最后的构建号
    ui->edtCR->setPlainText(copyRightText.append(".").append(maxBuild));

    QTreeWidgetItem *item;
    QHash<QString,QTreeWidgetItem*> nodes;
    for(int i = 0; i < keys.count(); ++i){
        item = new QTreeWidgetItem(ui->tw);
        item->setText(0,QString("%1(%2)").arg(keys.at(i)).arg(setting.value(keys.at(i)).toString()));
        nodes[keys.at(i)] = item;
    }
    setting.endGroup();
    QHashIterator<QString,QTreeWidgetItem*> it(nodes);
    QString explain;
    while(it.hasNext()){
        it.next();
        key = it.key();
        setting.beginGroup(key);
        keys = setting.childKeys();
        foreach(QString ckey, keys){
            explain = setting.value(ckey).toString();
            if(ckey == "reason"){
                nodes[key]->setText(0,nodes.value(key)->text(0).append("--").append(explain));
                continue;
            }
            item = new QTreeWidgetItem(nodes.value(key));
            item->setText(0,explain);
        }
        setting.endGroup();
    }
    ui->tw->expandItem(nodes.value(maxBuild));
}

AboutForm::~AboutForm()
{
    delete ui;
}
