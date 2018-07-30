#ifndef JTPZDLG_H
#define JTPZDLG_H

#include <QDialog>

namespace Ui {
class JtpzDlg;
}

class AccountSuiteManager;
class FirstSubject;
class SecondSubject;
struct JtpzDatas;

class JtpzDlg : public QDialog
{
    Q_OBJECT

public:
    explicit JtpzDlg(AccountSuiteManager* smg, QWidget *parent = 0);
    ~JtpzDlg();
    QList<JtpzDatas*> getDatas();

private:
    SecondSubject* getSSubByName(QString name,FirstSubject* fsub);

    Ui::JtpzDlg *ui;
    AccountSuiteManager* smg;
};

#endif // JTPZDLG_H
