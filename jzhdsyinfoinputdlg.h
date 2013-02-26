#ifndef JZHDSYINFOINPUTDLG_H
#define JZHDSYINFOINPUTDLG_H

#include <QDialog>
#include "utils.h"

namespace Ui {
class JzHdsyInfoInputDlg;
}

class JzHdsyInfoInputDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit JzHdsyInfoInputDlg(QWidget *parent = 0);
    ~JzHdsyInfoInputDlg();
    int getYear();
    int getMonth();
    Double getRate();
    bool isRateSave(){return !rateExist;}
    
private slots:
    void on_dateEdit_dateChanged(const QDate &date);

private:
    Ui::JzHdsyInfoInputDlg *ui;
    bool rateExist;            //汇率是否以存在
};

#endif // JZHDSYINFOINPUTDLG_H
