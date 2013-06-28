#ifndef SUITESWITCHPANEL_H
#define SUITESWITCHPANEL_H

#include "commdatastruct.h"

#include <QWidget>
#include <QListWidgetItem>
#include <QToolButton>
#include <QTableWidget>



class Account;
class AccountSuiteManager;

namespace Ui {
class SuiteSwitchPanel;
}

class SuiteSwitchPanel : public QWidget
{
    Q_OBJECT
    
public:
    enum ColType{
        COL_MONTH   = 0,
        COL_OPEN    = 1,
        COL_VIEW    = 2
    };

    explicit SuiteSwitchPanel(Account* account, QWidget *parent = 0);
    ~SuiteSwitchPanel();
    
private slots:
    void curSuiteChanged(QListWidgetItem * current, QListWidgetItem * previous);
    void swichBtnClicked();
    void viewBtnClicked();
    void openBtnClicked();
    void newPzSet();
signals:
    void selectedSuiteChanged(AccountSuiteManager* previous, AccountSuiteManager* current);
    void viewPzSet(AccountSuiteManager* accSmg, int month);
    void pzSetOpened(AccountSuiteManager* accSmg, int month);
    void prepareClosePzSet(AccountSuiteManager* accSmg, int month);
    void pzsetClosed(AccountSuiteManager* accSmg, int month);
private:
    void init();
    void initSuiteContent(AccountSuiteRecord* as);
    void crtTableRow(int row, int m, QTableWidget* tw);
    void witchSuiteMonth(AccountSuiteRecord *&suiteRecord, int &month, QObject* sender, ColType col);
    void setBtnIcon(QToolButton* btn, bool opened);

    Ui::SuiteSwitchPanel *ui;
    Account* account;
    QHash<int,AccountSuiteRecord*> suiteRecords;    //帐套记录表（键为帐套id）
    QHash<int,int> openedMonths;                    //每个帐套当前以编辑模式打开的月份数（键为帐套id）
    int curAsrId;  //与当前选择的帐套记录的id
    QIcon icon_unSelected, icon_selected;
};

#endif // SUITESWITCHPANEL_H
