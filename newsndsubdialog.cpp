#include "newsndsubdialog.h"
#include "ui_newsndsubdialog.h"
#include "subject.h"

//#include <QCompleter>

NewSndSubDialog::NewSndSubDialog(FirstSubject *fsub, QWidget *parent) :
    QDialog(parent),ui(new Ui::NewSndSubDialog),fsub(fsub)
{
    ui->setupUi(this);
    curNI = NULL;
    createdSSub = NULL;
    if(fsub){
        subMgr = fsub->parent();
        ui->lblFstSub->setText(fsub->getName());
        init();
    }
    else{
        subMgr = NULL;
    }
}

NewSndSubDialog::~NewSndSubDialog()
{
    delete ui;
}

void NewSndSubDialog::init()
{
    //初始化名称类别组合框的可选项
    niClses = SubjectManager::getAllNICls();
    QList<int> nicLst = niClses.keys();
    qSort(nicLst.begin(),nicLst.end());
    foreach(int id, nicLst)
        ui->cmbCls->addItem(niClses.value(id).first(),id);
    ui->cmbCls->setCurrentIndex(-1);

    //初始化名称输入完成器的数据模型
    nis = SubjectManager::getAllNI();
    QHashIterator<int,SubjectNameItem*> it(nis);
    QStandardItem* item;
    SubjectNameItem* ni;
    QVariant v;
    while(it.hasNext()){
        it.next();
        ni = it.value();
        item = new QStandardItem(ni->getShortName());
        v.setValue<SubjectNameItem*>(ni);
        item->setData(v,Qt::UserRole);
        model.appendRow(item);
    }
    completer.setModel(&model);
    completer.setFilterMode(Qt::MatchContains);
    ui->edtSName->setCompleter(&completer);
    connect(&completer,SIGNAL(activated(QModelIndex)),this,SLOT(niSelected(QModelIndex)));
    connect(&completer,SIGNAL(highlighted(QModelIndex)),this,SLOT(niHighlighted(QModelIndex)));
    connect(ui->edtSName,SIGNAL(returnPressed()),this,SLOT(nameInputCompleted()));
}

void NewSndSubDialog::keyPressEvent(QKeyEvent *event)
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
            ui->cmbCls->setFocus();
        else if(ui->cmbCls->hasFocus())
            ui->edtSubCode->setFocus();
        else if(ui->edtSubCode->hasFocus())
            ui->edtWeight->setFocus();
        else if(ui->edtWeight->hasFocus())
            ui->btnOk->setFocus();;
    }
}

/**
 * @brief 用户从完成列表中选择了一个名称
 * @param index
 */
void NewSndSubDialog::niSelected(const QModelIndex &index)
{
    curNI = completer.completionModel()->data(index,Qt::UserRole).value<SubjectNameItem*>();
    viewNI(curNI);
    if(curNI){
        SecondSubject* ssub = fsub->getChildSub(curNI);
        if(ssub){
            QMessageBox::warning(this,"",tr("该科目已存在！"));
            curNI = NULL;
            return;
        }        
        ui->edtSubCode->setFocus();
    }
}

/**
 * @brief 用户加亮了完成列表中的一个名称
 * @param index
 */
void NewSndSubDialog::niHighlighted(const QModelIndex &index)
{
    SubjectNameItem* ni = model.data(model.index(index.row(),0),Qt::EditRole).value<SubjectNameItem*>();
    viewNI(ni);
}

/**
 * @brief 用户完成了名称条目简称的输入
 */
void NewSndSubDialog::nameInputCompleted()
{

    QString sname = ui->edtSName->text();
    if(sname.isEmpty())
        return;
    curNI = SubjectManager::getNameItem(sname);
    viewNI(curNI);
    bool en = !curNI?true:false;
    ui->edtLName->setReadOnly(!en);
    ui->cmbCls->setEnabled(en);
    ui->edtRemCode->setReadOnly(!en);
    if(ui->edtCreator->text().isEmpty())
        ui->edtCreator->setText(curUser->getName());
    if(ui->edtCrtTime->text().isEmpty())
        ui->edtCrtTime->setText(QDateTime::currentDateTime().toString(Qt::ISODate));
}

/**
 * @brief 显示指定名称条目
 * @param ni
 */
void NewSndSubDialog::viewNI(SubjectNameItem *ni)
{
    if(ni){
        ui->edtLName->setText(ni->getLongName());
        ui->cmbCls->setCurrentIndex(ui->cmbCls->findData(ni->getClassId()));
        ui->edtRemCode->setText(ni->getRemCode());
        ui->edtCreator->setText(ni->getCreator()->getName());
        ui->edtCrtTime->setText(ni->getCreateTime().toString(Qt::ISODate));
    }
    else{
        ui->edtLName->clear();
        ui->cmbCls->setCurrentIndex(-1);
        ui->edtRemCode->clear();
        ui->edtCrtTime->clear();
        ui->edtCreator->clear();
    }
    ui->edtSubCode->clear();
    ui->edtWeight->clear();
}


void NewSndSubDialog::on_btnOk_clicked()
{
    QString sname = ui->edtSName->text();
    QString lname = ui->edtLName->text();
    QString remCode = ui->edtRemCode->text();
    if(sname.isEmpty() || lname.isEmpty() || remCode.isEmpty()){
        QMessageBox::warning(this,tr("信息不全"),tr("科目的简称、全称和助记符不能为空！"));
        return;
    }
    if(!curNI){
        int cls;
        if(ui->cmbCls->currentIndex() == -1){
            int index = ui->cmbCls->findText(tr("业务客户"));
            if(index == -1)
                cls = 2;
            else
                cls = ui->cmbCls->itemData(index).toInt();
        }
        else
            cls = ui->cmbCls->itemData(ui->cmbCls->currentIndex()).toInt();
        curNI = new SubjectNameItem(UNID,cls,sname,lname,remCode,QDateTime::currentDateTime(),curUser);
    }
    QString subCode = ui->edtSubCode->text();
    int weight = ui->edtWeight->text().toInt();
    createdSSub = new SecondSubject(fsub,UNID,curNI,subCode,weight,true,QDateTime::currentDateTime(),
                             QDateTime(),curUser);
    accept();
}
