#ifndef PADIALOG_H
#define PADIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>

class PaDialog : public QDialog
{
    Q_OBJECT
public:

    explicit PaDialog(QWidget *parent, Qt::WindowFlags flag = 0);
    ~PaDialog();

    QVBoxLayout *mainLayout;
    QVBoxLayout *pageLayout;
    QHBoxLayout *buttonsLayout;

    QDialogButtonBox *buttonBox;
};

#endif // PADIALOG_H
