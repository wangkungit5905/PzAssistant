#include <QSqlQuery>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>


#include "sqltooldialog.h"
#include "ui_sqltooldialog.h"

SqlToolDialog::SqlToolDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::SqlToolDialog)
{
    ui->setupUi(this);

    setLayout(ui->mlayout);
    ui->tview->setContextMenuPolicy(Qt::CustomContextMenu);

    mc=bc=0;
    model = new  QSqlQueryModel;
    tmodel = new QSqlTableModel;
    initTableList();

}

SqlToolDialog::~SqlToolDialog()
{
    delete ui;
}

void SqlToolDialog::excuteSql()
{
    QSqlQuery q;
    bool r;
    int c;
    bool isSelect;

    QString s = ui->edtSql->toPlainText().simplified();
    if(QString::compare(s.left(6), "select", Qt::CaseInsensitive) == 0)
        isSelect = true;
    else
        isSelect = false;

    if(!s.isEmpty()){
        r = q.exec(s);
        if(r){  //成功执行
            if(isSelect){//如果执行的是select语句，则显示查询的数据
                model->setQuery(s);
                ui->tview->setModel(model);
                //ui->stackedWidget->setCurrentIndex(0);
            }
            else{
                c = q.numRowsAffected();
                QString t = QString(tr("受影响的记录有： %1 条。")).arg(c);
                ui->edtResult->setPlainText(t);
                //ui->stackedWidget->setCurrentIndex(1);
            }


        }
        else{  //输出错误信息
            QString t = QString(tr("执行失败！"));
            ui->edtResult->setPlainText(t);
            //ui->stackedWidget->setCurrentIndex(1);
        }
    }

}

//读取所有数据库内的表，并加入到组合框中
void SqlToolDialog::initTableList()
{
    /** sqlite_master表结构：
            type:类型（table,...）
            name：表名
            tbl_name：表名
            rootpage：一个整数，不知是什么意思？
            sql：创建表所用的Sql语句
    */

    QSqlQuery q;
    bool r;

    ui->cmbTables->clear();
    QString s = "select * from sqlite_master";
    if(q.exec(s)){
        while(q.next()){
            QString tname = q.value(2).toString();
            ui->cmbTables->addItem(tname);
            mc++;
        }
    }
}

void SqlToolDialog::selectedTable(const QString &text)
{
    if(ui->chkIsEdit->isChecked()){
        tmodel->setTable(text);
        tmodel->select();
        ui->tview->setModel(tmodel);
    }
    else{
        QString s = QString("select * from %1").arg(text);
        ui->edtSql->setPlainText(s);
        excuteSql();
    }

}


void SqlToolDialog::toggled(bool checked)
{
    ui->btnSave->setEnabled(checked);
}

void SqlToolDialog::save()
{

}

void SqlToolDialog::contextMenuRequested(const QPoint &pos)
{
    //检测是否有选中的记录，如有则添加删除记录菜单
    actDel = new QAction(tr("删除记录"), this);
    actClear = new QAction(tr("清空表"), this);
    actAdd = new QAction(tr("插入记录"), this);
    connect(actDel, SIGNAL(triggered()), this, SLOT(delRec()));
    connect(actClear, SIGNAL(triggered()), this, SLOT(clearAll()));
    connect(actAdd, SIGNAL(triggered()), this, SLOT(addNewRec()));

    QMenu* m = new QMenu;
    m->addAction(actDel);
    m->addAction(actClear);
    m->addAction(actAdd);
    //m->popup(mapToParent(pos));
    //m->popup(pos);
    m->popup(mapToGlobal(pos));
}

//删除选中的记录
void SqlToolDialog::delRec()
{
    QItemSelectionModel* selModel = ui->tview->selectionModel();
    if(selModel->hasSelection() && ui->chkIsEdit->isChecked()){
        QModelIndex item;
        QModelIndexList selist = selModel->selectedIndexes();
        QSet<int> rowset;
        foreach(item, selist){        //统计要删除的行的序号
            rowset.insert(item.row());
        }
        QList<int> rowlst = rowset.toList();
        //从后向前删除
        qSort(rowlst.begin(), rowlst.end());
        for(int i = rowlst.count()-1; i >= 0; --i){
            tmodel->removeRow(rowlst[i]);
        }
        //model->submit();
    }
}

//清空表
void SqlToolDialog::clearAll()
{
    if(ui->chkIsEdit->isChecked()){
        int rows = tmodel->rowCount();
        tmodel->removeRows(0, rows);
    }

}

//添加新记录
void SqlToolDialog::addNewRec()
{
    if(ui->chkIsEdit->isChecked()){
        int row = tmodel->rowCount();
        tmodel->insertRow(row);
    }
}

//附加基础数据库到当前连接中
void SqlToolDialog::attachBasis(bool checked)
{
    bool r;
    QString s;
    QSqlQuery q;
    QString filename = "./datas/basicdatas/basicdata.dat";

    if(checked){ //将基础数据库附加到当前数据库连接上
        s = QString("attach database '%1' as basic").arg(filename);
        r = q.exec(s);
        if(r){
            s = "select name from basic.sqlite_master";
            r = q.exec(s);
            QString tname;
            while(q.next()){
                tname = q.value(0).toString();
                ui->cmbTables->addItem(QString("basic.%1").arg(tname));
                bc++;
            }
        }
    }
    else{ //将基础数据库与当前数据库连接分离
        s = QString("detach database basic");
        r = q.exec(s);
        //移除基础数据库中的表名
        for(int i = ui->cmbTables->count()-1; i>mc-1;i--)
            ui->cmbTables->removeItem(i);
    }
}

//附加导入数据库
void SqlToolDialog::on_chkImport_toggled(bool checked)
{
    bool r;
    QString s;
    QSqlQuery q;
    QString filename;

    if(checked){ //将外部数据库附加到当前数据库连接上
        //取得数据库的路径名
        filename = QFileDialog::
            getOpenFileName(this,tr("打开数据库文件"),QDir::currentPath(),tr("Sqlite(*.dat)"));
        if(filename == "")
            ui->btnImport->setEnabled(false);
        s = QString("attach database '%1' as import").arg(filename);
        r = q.exec(s);
        if(r){
            s = "select name from import.sqlite_master";
            r = q.exec(s);
            QString tname;
            while(q.next()){
                tname = q.value(0).toString();
                ui->cmbTables->addItem(QString("import.%1").arg(tname));
                bc++;
            }
        }
    }
    else{ //将外部数据库与当前数据库连接分离
        if(ui->btnImport->isEnabled()){
            s = QString("detach database import");
            r = q.exec(s);
            //移除基础数据库中的表名
            for(int i = ui->cmbTables->count()-1; i>mc-1;i--)
                ui->cmbTables->removeItem(i);
        }
    }
}

//执行导入操作
void SqlToolDialog::on_btnImport_clicked()
{

}

////////////////////////////BaseDataEditDialog///////////////////////////////////////////
BaseDataEditDialog::BaseDataEditDialog(QSqlDatabase* con, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BaseDataEditDialog)
{
    ui->setupUi(this);
    setLayout(ui->mLayout);
    this->con = con;

    model = new  QSqlQueryModel;
    tmodel = new QSqlTableModel(parent,*con);
    vh = ui->tview->verticalHeader();
    connect(vh, SIGNAL(sectionClicked(int)), this, SLOT(seledRow(int)));
    init();
}

BaseDataEditDialog::~BaseDataEditDialog()
{
    delete ui;
}

void BaseDataEditDialog::init()
{
    QSqlQuery q(*con);
    bool r;

    ui->cmbTables->clear();
    QString s = "select * from sqlite_master";
    if(q.exec(s)){
        while(q.next()){
            QString tname = q.value(2).toString();
            ui->cmbTables->addItem(tname);

        }
    }
}

void BaseDataEditDialog::excuteSql()
{
    QSqlQuery q(*con);
    bool r;
    int c;
    bool isSelect;

    QString s = ui->edtSql->toPlainText().simplified();
    if(QString::compare(s.left(6), "select", Qt::CaseInsensitive) == 0)
        isSelect = true;
    else
        isSelect = false;

    if(!s.isEmpty()){
        r = q.exec(s);
        if(r){  //成功执行
            if(isSelect){//如果执行的是select语句，则显示查询的数据
                model->setQuery(s,*con);
                ui->tview->setModel(model);
                ui->lblMsg->setText(tr("查询成功完成！"));
            }
            else{
                c = q.numRowsAffected();
                QString t = QString(tr("操作成功完成，受影响的记录有： %1 条。")).arg(c);
                ui->lblMsg->setText(t);
            }
        }
        else{  //输出错误信息
            QString t = QString(tr("执行失败！"));
            ui->lblMsg->setText(t);
        }
    }
}

//选择一个表后
void BaseDataEditDialog::on_cmbTables_currentIndexChanged(const QString &arg1)
{
    tmodel->setTable(arg1);
    tmodel->select();
    ui->tview->setModel(tmodel);
    ui->edtSql->setPlainText(QString("select * from %1").arg(arg1));
    selRows.clear();
}

void BaseDataEditDialog::on_btnSubmit_clicked()
{
    tmodel->submit();
}

void BaseDataEditDialog::on_btnRevert_clicked()
{
    tmodel->revert();
}

void BaseDataEditDialog::on_btnSwitch_clicked(bool checked)
{
    if(checked){
        ui->btnSwitch->setText(tr("编辑模式"));
        ui->tview->setEditTriggers(QAbstractItemView::DoubleClicked |
                                   QAbstractItemView::EditKeyPressed |
                                   QAbstractItemView::AnyKeyPressed);
    }
    else{
        ui->btnSwitch->setText(tr("只读模式"));
        ui->tview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
}

void BaseDataEditDialog::on_btnAdd_clicked()
{
    if(ui->btnSwitch->isChecked()){
        if(ui->tview->model() == tmodel){
            int row = tmodel->rowCount();
            tmodel->insertRow(row);
            ui->tview->scrollToBottom();
            int i = 0;
        }
    }
}

void BaseDataEditDialog::on_btnDel_clicked()
{
    if(ui->btnSwitch->isChecked()){
        if(selRows.isEmpty())
            return;
        QList<int> rowLst = selRows.toList();
        qSort(rowLst.begin(),rowLst.end());
        for(int i = rowLst.count()-1; i > -1; i--)
            tmodel->removeRow(rowLst[i]);
    }
}

void BaseDataEditDialog::seledRow(int rowIndex)
{
    if(selRows.contains(rowIndex))
        selRows.remove(rowIndex);
    else
        selRows.insert(rowIndex);
    if(!selRows.isEmpty())
        ui->btnDel->setEnabled(true);
    else
        ui->btnDel->setEnabled(false);
}

void BaseDataEditDialog::on_tview_clicked(const QModelIndex &index)
{
    selRows.clear();
    ui->btnDel->setEnabled(false);
}
