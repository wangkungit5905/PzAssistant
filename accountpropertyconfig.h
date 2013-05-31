#ifndef ACCOUNTPROPERTYCONFIG_H
#define ACCOUNTPROPERTYCONFIG_H

#include <QDialog>
#include "account.h"

#include "ui_apcbase.h"
#include "ui_apcsuite.h"
#include "ui_apcbank.h"
#include "ui_apcsubject.h"
#include "ui_apcreport.h"
#include "ui_apclog.h"

namespace Ui {
class ApcBase;
}


class Account;
class QListWidget;
class QStackedWidget;


class ApcBase : public QWidget
{
    Q_OBJECT

public:
    explicit ApcBase(Account* account, QWidget *parent = 0);
    ~ApcBase();

private:
    Ui::ApcBase *ui;
    Account* account;
};

class ApcSuite : public QWidget
{
    Q_OBJECT

public:
    explicit ApcSuite(QWidget *parent = 0);
    ~ApcSuite();

private:
    Ui::ApcSuite *ui;
};

class ApcBank : public QWidget
{
    Q_OBJECT

public:
    explicit ApcBank(QWidget *parent = 0);
    ~ApcBank();

private:
    Ui::ApcBank *ui;
};

class ApcSubject : public QWidget
{
    Q_OBJECT

public:
    explicit ApcSubject(QWidget *parent = 0);
    ~ApcSubject();

private:
    Ui::ApcSubject *ui;
};

class ApcReport : public QWidget
{
    Q_OBJECT

public:
    explicit ApcReport(QWidget *parent = 0);
    ~ApcReport();

private:
    Ui::ApcReport *ui;
};

class ApcLog : public QWidget
{
    Q_OBJECT

public:
    explicit ApcLog(QWidget *parent = 0);
    ~ApcLog();

private:
    Ui::ApcLog *ui;
};


class AccountPropertyConfig : public QDialog
{
    Q_OBJECT
    
public:
    explicit AccountPropertyConfig(Account* account, QWidget *parent = 0);
    ~AccountPropertyConfig();
    
private slots:
    void pageChanged(int index);
private:
    void createIcons();
private:
    Account* account;
    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;
};

#endif // ACCOUNTPROPERTYCONFIG_H
