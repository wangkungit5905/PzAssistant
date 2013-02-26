#ifndef RATESETDIALOG_H
#define RATESETDIALOG_H

#include <QDialog>

namespace Ui {
    class RateSetDialog;
}

class RateSetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RateSetDialog(QWidget *parent = 0);
    ~RateSetDialog();

private:
    Ui::RateSetDialog *ui;
};

#endif // RATESETDIALOG_H
