#include "tools/externaltoolconfigform.h"
#include "ui_externaltoolconfigform.h"
#include "config.h"
#include "widgets.h"
#include <QFileDialog>
#include <QMenu>

ExternalToolConfigForm::ExternalToolConfigForm(QList<ExternalToolCfgItem*>* tools, QWidget *parent) :
    QWidget(parent),ui(new Ui::ExternalToolConfigForm),tools(tools)
{
    ui->setupUi(this);
    appCon = AppConfig::getInstance();
    showTools();
    inspectConfigChange();
    isCloseAfterSave = true;
    isUpdateMenu = false;
    tools_copy = *tools;
    connect(ui->lwTools,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(ToolListContextMenuRequested(QPoint)));
}

ExternalToolConfigForm::~ExternalToolConfigForm()
{
    delete ui;
}

bool ExternalToolConfigForm::maybeSave()
{
    if(!isCloseAfterSave)
        return false;
    //这样处理主要是在如果改变了配置项，但因为没有按回车键或失去编辑单元的输入焦点而没有引发内容改变信号
    //这样就不会保存最后的修改单元
    int row = ui->lwTools->currentRow();
    bool isDirty = false;
    if(row != -1){
        ExternalToolCfgItem* tool = tools->at(row);
        if(ui->edtName->hasFocus() && ui->edtName->text() != tool->name){
            isDirty = true;
            tool->name = ui->edtName->text();
        }
        else if(ui->edtCommand->hasFocus() && ui->edtCommand->text() != tool->commandLine){
            isDirty = true;
            tool->commandLine = ui->edtCommand->text();
        }
        else if(ui->edtParas->hasFocus() && ui->edtParas->text() != tool->parameter){
            isDirty = true;
            tool->parameter = ui->edtParas->text();
        }
        if(isDirty){
            ui->lwTools->currentItem()->setData(Qt::UserRole,true);
            return true;
        }
    }    
    for(int i = 0; i < ui->lwTools->count(); ++i){
        isDirty = ui->lwTools->item(i)->data(Qt::UserRole).toBool();
        if(isDirty)
            return true;
    }    
    if(!delTools.isEmpty())
        return true;
    return false;
}

void ExternalToolConfigForm::save()
{
    bool isDirty = false;
    tools_copy.clear();
    for(int i = 0; i < ui->lwTools->count(); ++i){
        tools_copy<<tools->at(i);
        isDirty = ui->lwTools->item(i)->data(Qt::UserRole).toBool();
        if(isDirty){
            appCon->saveExternalTool(tools->at(i));
            ui->lwTools->item(i)->setData(Qt::UserRole,false);
            isUpdateMenu = true;
        }
    }
    if(!delTools.isEmpty()){
        foreach(ExternalToolCfgItem* tool, delTools){
            if(tool->id == 0)
                continue;
            appCon->saveExternalTool(tool,true);
        }
        qDeleteAll(delTools);
        delTools.clear();
        isUpdateMenu = true;
    }
}

void ExternalToolConfigForm::currentToolChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if(current){
        inspectConfigChange(false);
        ExternalToolCfgItem* tool = tools->at(ui->lwTools->currentRow());
        ui->edtName->setText(tool->name);
        ui->edtCommand->setText(tool->commandLine);
        ui->edtParas->setText(tool->parameter);
        inspectConfigChange();
    }
}


void ExternalToolConfigForm::toolConfigChanged()
{
    QLineEdit* editor = qobject_cast<QLineEdit*>(sender());
    QListWidgetItem* item = ui->lwTools->currentItem();
    if(item && editor){
        bool isChanged = false;
        ExternalToolCfgItem* tool = tools->at(ui->lwTools->currentRow());
        if(editor == ui->edtName && ui->edtName->text() != tool->name){
            tool->name = ui->edtName->text();
            item->setText(tool->name);
            isChanged = true;
        }
        else if(editor == ui->edtCommand && tool->commandLine != ui->edtCommand->text()){
            tool->commandLine = ui->edtCommand->text();
            isChanged = true;
        }
        else if(editor == ui->edtParas && tool->parameter != ui->edtParas->text()){
            tool->parameter = ui->edtParas->text();
            isChanged = true;
        }
        if(isChanged){
            ui->btnSave->setEnabled(true);
            item->setData(Qt::UserRole,true);
        }
    }
}

void ExternalToolConfigForm::ToolListContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(ui->actAdd);
    QListWidgetItem* item = ui->lwTools->itemAt(pos);
    if(item)
        menu.addAction(ui->actRemove);
    menu.exec(mapToGlobal(pos));
}

void ExternalToolConfigForm::on_btnBrowser_clicked()
{
    int row = ui->lwTools->currentRow();
    if(row == -1)
        return;
    ExternalToolCfgItem* tool = tools->at(row);
    QString fileName = QFileDialog::getOpenFileName(this,tr("请选择工具所在软件的可执行文件"));
    if(fileName.isEmpty())
        return;
    tool->commandLine = fileName;
    ui->edtCommand->setText(fileName);
    ui->lwTools->currentItem()->setData(Qt::UserRole,true);
}

void ExternalToolConfigForm::showTools()
{
//    if(!appCon->readAllExternalTools(*tools)){
//        QMessageBox::warning(this,"",tr("在读取外部工具配置项时发生错误！"));
//        return;
//    }
    foreach(ExternalToolCfgItem* item, *tools){
        QListWidgetItem* li = new QListWidgetItem(item->name,ui->lwTools);
        li->setData(Qt::UserRole,false);
    }
    connect(ui->lwTools,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this,SLOT(currentToolChanged(QListWidgetItem*,QListWidgetItem*)));
    if(!tools->isEmpty())
        ui->lwTools->setCurrentRow(0);
}

void ExternalToolConfigForm::inspectConfigChange(bool on)
{
    if(on){
        connect(ui->edtName,SIGNAL(editingFinished()),this,SLOT(toolConfigChanged()));
        connect(ui->edtCommand,SIGNAL(editingFinished()),this,SLOT(toolConfigChanged()));
        connect(ui->edtParas,SIGNAL(editingFinished()),this,SLOT(toolConfigChanged()));
    }
    else{
        disconnect(ui->edtName,SIGNAL(editingFinished()),this,SLOT(toolConfigChanged()));
        disconnect(ui->edtCommand,SIGNAL(editingFinished()),this,SLOT(toolConfigChanged()));
        disconnect(ui->edtParas,SIGNAL(editingFinished()),this,SLOT(toolConfigChanged()));
    }
}

void ExternalToolConfigForm::on_btnSave_clicked()
{
    if(maybeSave())
        save();
    ui->btnSave->setEnabled(false);
}

void ExternalToolConfigForm::on_btnCancel_clicked()
{
    isCloseAfterSave = false;
    tools->clear();
    *tools = tools_copy;
//    //移除新加项目但未保存的
//    foreach(ExternalToolCfgItem* tool,*tools){
//        if(tool->id == 0){
//            eTools.removeOne(tool);
//            delete tool;
//        }
//    }
//    //只可惜这里不能恢复原先的顺序
//    if(!delTools.isEmpty())
//        tools->append(delTools);
    delTools.clear();
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}

void ExternalToolConfigForm::on_actAdd_triggered()
{
    ExternalToolCfgItem* tool = new ExternalToolCfgItem;
    tool->id = 0;
    tool->name = tr("工具名称");
    *tools<<tool;
    //QListWidgetItem*
    QListWidgetItem* li = new QListWidgetItem(tool->name,ui->lwTools);
    ui->lwTools->setCurrentItem(li);
    li->setData(Qt::UserRole,true);
    ui->btnSave->setEnabled(true);
}

void ExternalToolConfigForm::on_actRemove_triggered()
{
    int row = ui->lwTools->currentRow();
    if(row == -1)
        return;
    delTools<<tools->takeAt(row);
    delete ui->lwTools->takeItem(row);
    if(row == ui->lwTools->count())
        ui->lwTools->setCurrentRow(row-1);
    else
        ui->lwTools->setCurrentRow(row);
    ui->btnSave->setEnabled(true);
}
