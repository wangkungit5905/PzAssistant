#ifndef NEWSNDSUBDIALOG_H
#define NEWSNDSUBDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QCompleter>

namespace Ui {
class NewSndSubDialog;
}

//class QCompleter;
class SubjectManager;
class FirstSubject;
class SecondSubject;
class SubjectNameItem;

class NewSndSubDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewSndSubDialog(FirstSubject* fsub, QWidget *parent = 0);
    ~NewSndSubDialog();
    void init();
    SecondSubject* getCreatedSubject(){return createdSSub;}

protected:
    void keyPressEvent(QKeyEvent *event);
private slots:
    void niSelected(const QModelIndex& index);
    void niHighlighted(const QModelIndex& index);
    void nameInputCompleted();
    void on_btnOk_clicked();

private:
    void viewNI(SubjectNameItem* ni);

    Ui::NewSndSubDialog *ui;
    SubjectManager* subMgr;
    FirstSubject* fsub;
    SecondSubject* createdSSub;
    QCompleter completer;
    QStandardItemModel model;
    QHash<int,QStringList> niClses;
    QHash<int,SubjectNameItem*> nis;
    SubjectNameItem* curNI;
};

#endif // NEWSNDSUBDIALOG_H
