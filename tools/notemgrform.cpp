#include "notemgrform.h"
#include "ui_notemgrform.h"
#include "tables.h"
#include "account.h"
#include "dbutil.h"

#include <QSqlQuery>
#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QVariant>
#include <QCloseEvent>

NoteMgrForm::NoteMgrForm(Account *account, QWidget *parent) :
    QWidget(parent),ui(new Ui::NoteMgrForm),account(account)
{
    ui->setupUi(this);
    dbUtil = account->getDbUtil();
    ui->stackedWidget->setCurrentIndex(0);
    readNotes();
    connect(ui->lwTitles,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(titleListContextMenuRequested(QPoint)));
}

NoteMgrForm::~NoteMgrForm()
{
    if(!notes.isEmpty())
        qDeleteAll(notes);
    delete ui;
}

void NoteMgrForm::closeEvent(QCloseEvent *event)
{
    if(ui->stackedWidget->currentIndex() == 1){
        QListWidgetItem* item = ui->lwTitles->currentItem();
        if(item->text() != ui->edtTitle->text()){
            item->setText(ui->edtTitle->text());
            NoteStruct* note = item->data(NDR_OBJECT).value<NoteStruct*>();
            note->title = ui->edtTitle->text();
            item->setData(NDR_MODIFY_TAG,true);
        }
        if(ui->NoteTexts->document()->isModified()){
            NoteStruct* note = item->data(NDR_OBJECT).value<NoteStruct*>();
            note->content = ui->NoteTexts->document()->toHtml("utf-8");
            note->lastEditTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
            item->setData(NDR_MODIFY_TAG,true);
        }
    }
    bool isChanged = false;
    for(int i = 0; i < ui->lwTitles->count(); ++i){
        isChanged = ui->lwTitles->item(i)->data(NDR_MODIFY_TAG).toBool();
        if(isChanged)
            break;
    }
    if(isChanged || !delNotes.isEmpty()){
        if(QMessageBox::Yes == QMessageBox::warning(this,"",tr("存在有未保存的笔记，如果需要保存按确定，再返回到笔记列表保存笔记！"),
                                                    QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes))
            event->ignore();
        else
            event->accept();
    }
    else
        event->accept();
}


void NoteMgrForm::titleListContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* item = ui->lwTitles->itemAt(pos);
    QMenu menu(this);
    menu.addAction(ui->actAddNote);
    if(item)
        menu.addAction(ui->actDelNote);
    menu.exec(mapToGlobal(pos));
}

/**
 * @brief 笔记内容被改变
 */
void NoteMgrForm::noteContentChanged()
{
    ui->btnSave->setEnabled(true);
}

void NoteMgrForm::on_btnReturn_clicked()
{
    bool isNote = ui->stackedWidget->currentIndex() == 1;
    if(isNote){
        QListWidgetItem* item = ui->lwTitles->currentItem();
        if(ui->edtTitle->text() != item->text()){
            NoteStruct* note = item->data(NDR_OBJECT).value<NoteStruct*>();
            note->title = ui->edtTitle->text();
            item->setText(ui->edtTitle->text());
            item->setData(NDR_MODIFY_TAG,true);
        }
        QTextDocument* doc = ui->NoteTexts->document();
        if(doc->isModified()){
            NoteStruct* note = item->data(NDR_OBJECT).value<NoteStruct*>();
            note->content = doc->toHtml("utf-8");
            note->lastEditTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
            item->setData(NDR_MODIFY_TAG,true);
            doc->setModified(false);
        }
        if(item->data(NDR_MODIFY_TAG).toBool())
            ui->btnSave->setEnabled(true);
        ui->stackedWidget->setCurrentIndex(0);
        ui->btnReturn->setEnabled(false);
        ui->edtTitle->clear();
        ui->edtCrtTime->clear();
        ui->edtLastTime->clear();
    }
}

void NoteMgrForm::on_lwTitles_itemDoubleClicked(QListWidgetItem *item)
{
    disconnect(ui->edtTitle,SIGNAL(textEdited(QString)),this,SLOT(noteContentChanged()));
    disconnect(ui->NoteTexts,SIGNAL(textChanged()),this,SLOT(noteContentChanged()));
    ui->stackedWidget->setCurrentIndex(1);
    NoteStruct* note = item->data(NDR_OBJECT).value<NoteStruct*>();
    ui->NoteTexts->document()->setHtml(note->content);
    ui->NoteTexts->document()->setModified(false);
    ui->btnReturn->setEnabled(true);
    ui->edtTitle->setText(note->title);
    QString ds = QDateTime::fromMSecsSinceEpoch(note->crtTime).toString("yyyy-M-d h:m:s");
    ui->edtCrtTime->setText(tr("创建：%1").arg(ds));
    ds = QDateTime::fromMSecsSinceEpoch(note->lastEditTime).toString("yyyy-M-d h:m:s");
    ui->edtLastTime->setText(tr("修改：%1").arg(ds));
    connect(ui->edtTitle,SIGNAL(textEdited(QString)),this,SLOT(noteContentChanged()));
    connect(ui->NoteTexts,SIGNAL(textChanged()),this,SLOT(noteContentChanged()));
}

void NoteMgrForm::on_actAddNote_triggered()
{
    NoteStruct* note = new NoteStruct;
    note->id = 0;
    note->title = tr("新笔记");
    note->crtTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    note->lastEditTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    notes<<note;
    QListWidgetItem* item = new QListWidgetItem(note->title,ui->lwTitles);
    QVariant v; v.setValue<NoteStruct*>(note);
    item->setData(NDR_OBJECT,v);
    item->setData(NDR_MODIFY_TAG,true);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->lwTitles->editItem(item);
}

void NoteMgrForm::on_actDelNote_triggered()
{
    int row = ui->lwTitles->currentRow();
    QListWidgetItem* item = ui->lwTitles->currentItem();
    if(item){
        NoteStruct* note = item->data(NDR_OBJECT).value<NoteStruct*>();
        delNotes<<note;
        notes.removeOne(note);
        delete ui->lwTitles->takeItem(row);
        ui->btnSave->setEnabled(true);
    }
}

void NoteMgrForm::readNotes()
{
    if(!dbUtil->readNotes(notes)){
        QMessageBox::critical(this,"",tr("读取笔记失败！"));
        return;
    }
    foreach(NoteStruct* note, notes){
        QListWidgetItem* item = new QListWidgetItem(note->title,ui->lwTitles);
        QVariant v;
        v.setValue<NoteStruct*>(note);
        item->setData(NDR_OBJECT,v);
        item->setData(NDR_MODIFY_TAG,false);
    }
}

bool NoteMgrForm::saveNotes()
{
    for(int i = 0; i < ui->lwTitles->count(); ++i){
        QListWidgetItem* item = ui->lwTitles->item(i);
        if(!item->data(NDR_MODIFY_TAG).toBool())
            continue;
        NoteStruct* note = item->data(NDR_OBJECT).value<NoteStruct*>();
        if(!dbUtil->saveNote(note))
            return false;
        item->setData(NDR_MODIFY_TAG,false);
    }
    if(!delNotes.isEmpty()){
        foreach(NoteStruct* note, delNotes){
            if(!dbUtil->saveNote(note,true))
                return false;
        }
        qDeleteAll(delNotes);
        delNotes.clear();
    }
    return true;
}

void NoteMgrForm::on_btnSave_clicked()
{
    if(!saveNotes())
        QMessageBox::warning(this,"",tr("保存笔记失败"));
    else
        ui->btnSave->setEnabled(false);
}

