#include <QSqlQuery>
#include <QDebug>

#include "subjectsearchform.h"
#include "ui_subjectsearchform.h"
#include "tables.h"

SubjectSearchForm::SubjectSearchForm(QWidget *parent) :
    StyledWidget(parent),ui(new Ui::SubjectSearchForm),db(NULL),model(NULL)
{
    ui->setupUi(this);
}

SubjectSearchForm::~SubjectSearchForm()
{
    //detachDb();
    delete ui;
}

void SubjectSearchForm::attachDb(QSqlDatabase *db)
{
    this->db = db;
    model = new QSqlQueryModel(this);
    ui->btnSearch->setEnabled(true);
    ui->edtKeyWord->setReadOnly(false);
    ui->cmbClass->clear();
    QSqlQuery q(*db);
    QString s = QString("select %1,%2 from %3")
            .arg(fld_fsc_code).arg(fld_fsc_name).arg(tbl_fsclass);
    if(!q.exec(s)){
        qDebug()<<tr("在提取二级科目类别时发生错误！");
        return;
    }
    int code;QString name;
    ui->cmbClass->addItem(tr("所有"),0);
    while(q.next()){
        code = q.value(0).toInt();
        name = q.value(1).toString();
        ui->cmbClass->addItem(name,code);
    }

    connect(ui->cmbClass,SIGNAL(currentIndexChanged(int)),
            this,SLOT(classChanged(int)));
    connect(ui->edtKeyWord,SIGNAL(editingFinished()),
            this,SLOT(keyWordEditingFinished()));
    s = QString("select id,%1,%2,%3 from %4").arg(fld_ni_name)
            .arg(fld_ni_remcode).arg(fld_ni_lname).arg(tbl_nameItem);
    model->setQuery(s,*db);
    ui->tview->setModel(model);    
    ui->tview->setColumnWidth(0,30);
    ui->tview->setColumnWidth(1,80);
    ui->tview->setColumnWidth(2,50);
}

void SubjectSearchForm::detachDb()
{
    db = NULL;
    disconnect(ui->cmbClass,SIGNAL(currentIndexChanged(int)),
               this,SLOT(classChanged(int)));
    disconnect(ui->edtKeyWord,SIGNAL(editingFinished()),
            this,SLOT(keyWordEditingFinished()));
    ui->cmbClass->clear();
    ui->btnSearch->setEnabled(false);
    ui->edtKeyWord->setReadOnly(true);
    ui->tview->setModel(NULL);    
}


void SubjectSearchForm::on_btnSearch_clicked()
{
    refresh();
}

//选择的类别改变了
void SubjectSearchForm::classChanged(int index)
{
    refresh();
}

// （刷新查询结果）在指定的类别中模糊搜索含有指定关键字的二级科目名
void SubjectSearchForm::refresh()
{
    QString fKey,fCode,keyWord,s;
    QSqlQuery q(*db);
    s = QString("select id,%1,%2,%3 from %4").arg(fld_ni_name)
            .arg(fld_ni_lname).arg(fld_ni_remcode).arg(tbl_nameItem);
    keyWord = ui->edtKeyWord->text();
    int code = ui->cmbClass->itemData(ui->cmbClass->currentIndex()).toInt();
    if(!keyWord.isEmpty())
        fKey = QString("%1 like '%%2%'").arg(fld_ni_name).arg(keyWord);
    if(code != 0)
        fCode = QString("%1 = %2").arg(fld_ni_class).arg(code);
    if(!fKey.isEmpty() && code == 0)
        s.append(" where ").append(fKey);
    else if(fKey.isEmpty() && code != 0)
        s.append(" where ").append(fCode);
    else if(!fKey.isEmpty() && code != 0)
        s.append(" where ").append(fKey).append(" and ").append(fCode);

    if(!s.isEmpty())
        model->setQuery(s,*db);
    ui->tview->setModel(model);

    model->setHeaderData(0,Qt::Horizontal,"ID");
    model->setHeaderData(1,Qt::Horizontal,tr("简称"));
    model->setHeaderData(2,Qt::Horizontal,tr("全称"));
    model->setHeaderData(3,Qt::Horizontal,tr("助记符"));
    //ui->tview->setColumnHidden(0,true);
    ui->tview->setColumnWidth(0,30);
    ui->tview->setColumnWidth(1,100);
    ui->tview->setColumnWidth(1,400);
    ui->tview->setColumnWidth(1,100);
}

void SubjectSearchForm::keyWordEditingFinished()
{
    refresh();
}
