#ifndef COMPLETSUBINFODIALOG_H
#define COMPLETSUBINFODIALOG_H

#include <QDialog>

namespace Ui {
    class CompletSubInfoDialog;
}

class CompletSubInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CompletSubInfoDialog(int fid = 0, QWidget *parent = 0);
    ~CompletSubInfoDialog();
    void setName(QString name);
    QString getSName();
    QString getLName();
    QString getRemCode();
    int getSubCalss();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    Ui::CompletSubInfoDialog *ui;
};

#endif // COMPLETSUBINFODIALOG_H
