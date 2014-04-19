#ifndef IMPORTOVACCDLG_H
#define IMPORTOVACCDLG_H


#include <QDialog>
#include <QSqlDatabase>


namespace Ui {
class ImportOVAccDlg;
}

class Account;
class AccountSuiteManager;
class SubjectManager;
class FirstSubject;
class SecondSubject;
class SuiteSwitchPanel;

/**
 * @brief 导入旧版账户的凭证集
 */
class ImportOVAccDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ImportOVAccDlg(Account* account, SuiteSwitchPanel* panel, QWidget *parent = 0);
    ~ImportOVAccDlg();

signals:
    void switchSuiteTo(int y);

private slots:
    void importDateChanged(const QDate &date);
    void on_btnFile_clicked();

    void on_btnImport_clicked();

private:
    bool init();
    bool compareRate();
    bool createMaps();
    bool importPzSet();

    Ui::ImportOVAccDlg *ui;
    QSqlDatabase sdb;
    QString connName;
    Account* account;
    SuiteSwitchPanel* panel;
    AccountSuiteManager *curSuite;
    SubjectManager* sm;
    QHash<int,FirstSubject*> fsubIdMaps;  //新旧一级科目映射表(键为旧一级科目id，值为对应的新一级科目对象)
    QHash<int,SecondSubject*> ssubIdMaps; //新旧二级科目映射表（键为旧二级科目id，值为新二级科目对象）
};

#endif // IMPORTOVACCDLG_H
