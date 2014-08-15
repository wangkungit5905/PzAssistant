#ifndef NOTEMGRFORM_H
#define NOTEMGRFORM_H

#include <QWidget>

class QListWidgetItem;

namespace Ui {
class NoteMgrForm;
}

class Account;
class DbUtil;
struct NoteStruct;

class NoteMgrForm : public QWidget
{
    Q_OBJECT

    enum NOTE_DATA_ROLE{
        NDR_MODIFY_TAG = Qt::UserRole,
        NDR_OBJECT = Qt::UserRole + 1
    };

public:
    explicit NoteMgrForm(Account* account, QWidget *parent = 0);
    ~NoteMgrForm();

protected:
    void closeEvent(QCloseEvent * event);

private slots:
    void titleListContextMenuRequested(const QPoint & pos);
    void on_btnReturn_clicked();

    void on_lwTitles_itemDoubleClicked(QListWidgetItem *item);

    void on_actAddNote_triggered();

    void on_actDelNote_triggered();

    void on_btnSave_clicked();

private:
    void readNotes();
    bool saveNotes();

    Ui::NoteMgrForm *ui;
    QList<NoteStruct*> notes;
    QList<NoteStruct*> delNotes;  //被删除的笔记id列表
    Account* account;
    DbUtil* dbUtil;
};

#endif // NOTEMGRFORM_H
