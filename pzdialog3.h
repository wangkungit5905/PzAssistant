#ifndef PZDIALOG3_H
#define PZDIALOG3_H

#include <QDialog>

#include "account.h"

namespace Ui {
class pzDialog3;
}

class PzDialog3 : public QDialog
{
    Q_OBJECT
    
public:
    explicit PzDialog3(PzSetMgr* pzSet,QWidget *parent = 0);
    ~PzDialog3();
    
private:
    Ui::pzDialog3 *ui;
};

#endif // PZDIALOG3_H
