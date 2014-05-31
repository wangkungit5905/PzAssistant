#include "databaseaccessform.h"
#include "ui_databaseaccessform.h"

#include "account.h"
#include "dbutil.h"
#include "config.h"

#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>




DatabaseAccessForm::DatabaseAccessForm(Account *account, AppConfig *config, QWidget *parent) :
    QWidget(parent),ui(new Ui::DatabaseAccessForm),account(account),appCfg(config)
{
    ui->setupUi(this);
    if(account){
        adb = account->getDbUtil()->getDb();
        ui->rdo_acc->setChecked(true);
    }
    else{
        ui->rdo_acc->setEnabled(false);
        ui->rdo_base->setChecked(true);
    }
    bdb = config->getBaseDbConnect();
    init();
}

DatabaseAccessForm::~DatabaseAccessForm()
{
    delete ui;
}

void DatabaseAccessForm::init()
{
    colWidths[FDT_INT] = 50;
    colWidths[FDT_DOUBLE] = 100;
    colWidths[FDT_TEXT] = 150;
    colWidths[FDT_NONE] = 100;
    editMode = false;
    tModel = NULL;
    loadTable(ui->rdo_acc->isChecked());
    connect(ui->rdo_acc,SIGNAL(toggled(bool)),this,SLOT(loadTable(bool)));
    connect(ui->sqlInput,SIGNAL(textChanged()),this,SLOT(sqlTextChanged()));
    connect(ui->lwTables,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(tableDoubleClicked(QListWidgetItem*)));
    enWidget(false);
    ui->btnExec->setEnabled(false);
    ui->btnClear->setEnabled(false);
    ui->tw->addAction(ui->insertRowAction);
    ui->tw->addAction(ui->deleteRowAction);
}

/**
 * @brief 装载指定数据库内的各个表格及其定义
 * @param isAccount
 */
void DatabaseAccessForm::loadTable(bool isAccount)
{
    disconnect(ui->lwTables,SIGNAL(currentRowChanged(int)),this,SLOT(curTableChanged(int)));
    clear(isAccount);
    QSqlDatabase db;
    if(isAccount)
        db = adb;
    else
        db = bdb;
    if(tModel){
        disconnect(tModel,SIGNAL(dataChanged(QModelIndex,QModelIndex)),this,SLOT(dataChanged(QModelIndex,QModelIndex)));
        delete tModel;
    }
    tModel = new QSqlTableModel(this,db);
    tModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    connect(tModel,SIGNAL(dataChanged(QModelIndex,QModelIndex)),this,SLOT(dataChanged(QModelIndex,QModelIndex)));

    QSqlQuery q(db);
    QString s = "select name from sqlite_master";
    if(!q.exec(s)){
        LOG_SQLERROR(s);
        return;
    }
    QString name;
    while(q.next()){
        name = q.value(0).toString();
        isAccount?tableNames_acc<<name:tableNames_base<<name;
        parseTableDefine(name,isAccount);
    }
    QListWidgetItem* item;
    foreach(QString name,isAccount?tableNames_acc:tableNames_base){
        item = new QListWidgetItem(name);
        ui->lwTables->addItem(item);
    }
    connect(ui->lwTables,SIGNAL(currentRowChanged(int)),this,SLOT(curTableChanged(int)));
}

/**
 * @brief 解析表格创建语句，并将各字段的定义放入对应表中
 * @param tName     表名
 * @param isAccount true：账户库，false：基本库
 */
void DatabaseAccessForm::parseTableDefine(QString tName, bool isAccount)
{
    QSqlRecord rec = isAccount?adb.record(tName):bdb.record(tName);
    for(int i = 0; i < rec.count(); ++i){
        QSqlField fld = rec.field(i);
        FieldDefine* fd = new FieldDefine;
        fd->name = fld.name();
        fd->type = FDT_NONE;
        switch(fld.type()){
        case QVariant::Int:
            fd->type = FDT_INT;
            break;
        case QVariant::String:
            fd->type = FDT_TEXT;
            break;
        case QVariant::Double:
            fd->type = FDT_DOUBLE;
            break;
        }
        isAccount?tableDefines_acc[tName].append(fd):tableDefines_base[tName].append(fd);
    }
}

/**
 * @brief 清除指定数据库装载的表格信息
 * @param isAccount
 */
void DatabaseAccessForm::clear(bool isAccount)
{
    ui->lwTables->clear();
    ui->tw->setModel(NULL);
    if(isAccount){
        tableNames_acc.clear();
        QHashIterator<QString,QList<FieldDefine*> > it(tableDefines_acc);
        while(it.hasNext()){
            it.next();
            foreach(FieldDefine* d, it.value())
                delete d;
            tableDefines_acc[it.key()].clear();
        }
        tableDefines_acc.clear();
    }
    else{
        tableNames_base.clear();
        QHashIterator<QString,QList<FieldDefine*> > it(tableDefines_base);
        while(it.hasNext()){
            it.next();
            foreach(FieldDefine* d, it.value())
                delete d;
            tableDefines_base[it.key()].clear();
        }
        tableDefines_base.clear();
    }
}

/**
 * @brief 当用户选择一个表格时，显示表格内容
 * @param index
 */
void DatabaseAccessForm::curTableChanged(int index)
{
    QString tableName = ui->rdo_acc->isChecked()?tableNames_acc.at(index):tableNames_base.at(index);
    tModel->setTable(tableName);
    tModel->select();
    ui->tw->setModel(tModel);
    if (tModel->lastError().type() != QSqlError::NoError)
        ui->stateInfo->setText(tModel->lastError().text());
    else
        ui->stateInfo->clear();
    adjustColWidth(tableName,ui->rdo_acc->isChecked());
    updateActions();
}

/**
 * @brief 表格内容被编辑后，启用提交和取消按钮
 * @param topLeft
 * @param bottomRight
 */
void DatabaseAccessForm::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if(!editMode)
        return;
    enWidget(true);
}

void DatabaseAccessForm::sqlTextChanged()
{
    bool r = !ui->sqlInput->toPlainText().isEmpty();
    ui->btnExec->setEnabled(r);
    ui->btnClear->setEnabled(r);
}

/**
 * @brief 双击表名，启动编辑功能
 * @param item
 */
void DatabaseAccessForm::tableDoubleClicked(QListWidgetItem *item)
{
    editMode = true;
    enWidget(true);
    connect(ui->tw->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                this, SLOT(currentChanged()));
    updateActions();
}

void DatabaseAccessForm::currentChanged()
{
    updateActions();
}

/**
 * @brief 根据表的各个字段的类型，调整表格的默认列宽
 * @param isAccount
 * @param t         表名称
 */
void DatabaseAccessForm::adjustColWidth(QString t,bool isAccount)
{
    QList<FieldDefine*> fldDefines;
    isAccount?fldDefines = tableDefines_acc.value(t):tableDefines_base.value(t);
    for(int i = 0; i < fldDefines.count(); ++i){
        ui->tw->setColumnWidth(i,colWidths.value(fldDefines.at(i)->type));
    }
}

/**
 * @brief 根据当前的编辑模式和表格数据是否被修改的情况，启用或禁用相关按钮
 * @param en
 */
void DatabaseAccessForm::enWidget(bool en)
{
    ui->lwTables->setEnabled(!en);
    ui->btnCommit->setEnabled(en);
    ui->btnRevert->setEnabled(en);
}

/**
 * @brief 取消对当前表格所做的编辑
 */
void DatabaseAccessForm::on_btnRevert_clicked()
{
    if(!editMode)
        return;
    tModel->revertAll();
    editMode = false;
    enWidget(false);
}

/**
 * @brief 提交对当前表格所做的编辑
 */
void DatabaseAccessForm::on_btnCommit_clicked()
{
    if(!editMode)
        return;
    if(!tModel->submitAll())
        ui->stateInfo->setText(tModel->lastError().text());
    else
        ui->stateInfo->clear();
    editMode = false;
    enWidget(false);
}

/**
 * @brief 执行Sql语句
 */
void DatabaseAccessForm::on_btnExec_clicked()
{
    QString sql = ui->sqlInput->toPlainText();
    if(sql.isEmpty())
        return;
    QSqlQueryModel* model = new QSqlQueryModel(ui->tw);;
    if(ui->rdo_acc->isChecked())
        model->setQuery(sql,adb);
    else
        model->setQuery(sql,bdb);
    ui->tw->setModel(model);
    if (model->lastError().type() != QSqlError::NoError)
            ui->stateInfo->setText(model->lastError().text());
    else if (model->query().isSelect())
        ui->stateInfo->setText(tr("查询成功！"));
    else
        ui->stateInfo->setText(tr("执行成功，受影响的行数：%1").arg(
                           model->query().numRowsAffected()));
    updateActions();
}

void DatabaseAccessForm::on_btnClear_clicked()
{
    ui->sqlInput->clear();
    ui->btnExec->setEnabled(false);
    ui->btnClear->setEnabled(false);
}

void DatabaseAccessForm::on_insertRowAction_triggered()
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tw->model());
    if (!model)
        return;

    QModelIndex insertIndex = ui->tw->currentIndex();
    int row = insertIndex.row() == -1 ? 0 : insertIndex.row();
    model->insertRow(row);
    insertIndex = model->index(row, 0);
    ui->tw->setCurrentIndex(insertIndex);
    ui->tw->edit(insertIndex);
}

void DatabaseAccessForm::on_deleteRowAction_triggered()
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tw->model());
    if (!model)
        return;

    QModelIndexList currentSelection = ui->tw->selectionModel()->selectedIndexes();
    for(int i = 0; i < currentSelection.count(); ++i){
        if (currentSelection.at(i).column() != 0)
            continue;
        model->removeRow(currentSelection.at(i).row());
    }

    model->submitAll();
    enWidget(false);
    updateActions();
}

void DatabaseAccessForm::updateActions()
{
    bool enableIns = editMode && qobject_cast<QSqlTableModel*>(ui->tw->model());
    bool enableDel = editMode && enableIns && ui->tw->currentIndex().isValid();

    ui->insertRowAction->setEnabled(enableIns);
    ui->deleteRowAction->setEnabled(enableDel);
}
