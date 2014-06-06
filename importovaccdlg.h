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
struct MixedJoinCfg;

/**
 * @brief 导入旧版账户的凭证集
 */
class ImportOVAccDlg : public QDialog
{
    Q_OBJECT

    //混合对接科目配置项（用科目对象替换科目id）
    struct MixedJoinSubObj{
        int sFid;
        int sSid;
        FirstSubject* dFSub;
        SecondSubject* dSSub;
        bool isNew;
    };

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
    bool processSSubMaps(int fid,QSqlQuery q);
    bool isExistCfg(int sfid, int ssid);

    Ui::ImportOVAccDlg *ui;
    QSqlDatabase sdb;
    QString connName;
    Account* account;
    SuiteSwitchPanel* panel;
    AccountSuiteManager *curSuite;
    SubjectManager* sm;
    QHash<int,FirstSubject*> fsubIdMaps;  //新旧一级科目映射表(键为旧一级科目id，值为对应的新一级科目对象)
    QHash<int,SecondSubject*> ssubIdMaps; //新旧二级科目映射表（键为旧二级科目id，值为新二级科目对象）
    QList<MixedJoinSubObj*> mixedJoinItems;  //新建的混合对接项
};

#endif // IMPORTOVACCDLG_H
