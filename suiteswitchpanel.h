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
    static const int ROLE_CUR_SUITE = Qt::UserRole + 1;       //用此角色来保存当前打开帐套到id
    enum ColType{
        COL_MONTH   = 0,
        COL_OPEN    = 1,
        COL_VIEW    = 2
    };

    explicit SuiteSwitchPanel(Account* account, QWidget *parent = 0);
    ~SuiteSwitchPanel();
    void setJzState(AccountSuiteManager* sm, int month, bool jzed = true);

    
public slots:
    void switchToSuite(int y);
private slots:
    void curSuiteChanged(QListWidgetItem * current, QListWidgetItem * previous);
    //void swichBtnClicked();
    void viewBtnClicked();
    void openBtnClicked(bool checked);
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
    void crtTableRow(int row, int m, QTableWidget* tw, bool viewAndEdit=true);
    void witchSuiteMonth(int &month, QObject* sender, ColType col);
    void setBtnIcon(QToolButton* btn, bool opened);

    Ui::SuiteSwitchPanel *ui;
    Account* account;
    QHash<int,AccountSuiteRecord*> suiteRecords;    //帐套记录表（键为帐套id）
    QHash<int,int> openedMonths;                    //每个帐套当前以编辑模式打开的月份数（键为帐套id）
    //int curAsrId;  //当前选择的帐套记录的id
    AccountSuiteManager* curSuite;                   //当前选择的帐套对象
    QIcon icon_unSelected, icon_selected;
    QIcon icon_open, icon_close, icon_edit, icon_lookup; //凭证集的打开、关闭、编辑和查看图标

    QString btn_tip_edit,btn_tip_view;
};

#endif // SUITESWITCHPANEL_H
