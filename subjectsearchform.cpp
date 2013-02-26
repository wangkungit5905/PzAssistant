#include <QSqlQuery>
#include <QDebug>

#include "subjectsearchform.h"
#include "ui_subjectsearchform.h"

SubjectSearchForm::SubjectSearchForm(QWidget *parent) :
    QWidget(parent),ui(new Ui::SubjectSearchForm),db(NULL),model(NULL)
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
    QString s = "select clsCode,name from SndSubClass";
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

    model->setQuery("select id,subName,subLName,remCode from SecSubjects",*db);
    ui->tview->setModel(model);
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
    s = "select id,subName,subLName,remCode from SecSubjects ";
    keyWord = ui->edtKeyWord->text();
    int code = ui->cmbClass->itemData(ui->cmbClass->currentIndex()).toInt();
    if(!keyWord.isEmpty())
        fKey = QString("subName like '%%1%'").arg(keyWord);
    if(code != 0)
        fCode = QString("classId = %1").arg(code);
    if(!fKey.isEmpty() && code == 0)
        s.append("where ").append(fKey);
    else if(fKey.isEmpty() && code != 0)
        s.append("where ").append(fCode);
    else if(!fKey.isEmpty() && code != 0)
        s.append("where ").append(fKey).append(" and ").append(fCode);

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
