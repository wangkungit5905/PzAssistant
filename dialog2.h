#ifndef DIALOG2_H
#define DIALOG2_H

#include <QtGlobal>

#include <QWidget>
#include <QSqlQueryModel>
#include <QStandardItemModel>
#include <QMenu>

#include "delegates2.h"
#include "securitys.h"
#include "HierarchicalHeaderView.h"
#include "widgets.h"
#include "account.h"

#include "ui_basummaryform.h"
#include "ui_ratesetdialog.h"
#include "ui_printselectdialog.h"
#include "ui_logindialog.h"
#include "ui_seccondialog.h"
#include "ui_searchdialog.h"


#ifdef Q_OS_LINUX
#endif

#ifdef Q_OS_WIN
//#include "excelUtils.h"
#endif

#define FSTSUBTYPE QTreeWidgetItem::UserType+1  //放置一级科目的树节点的类型

class PingZheng;
class AccountSuiteManager;


//Q_DECLARE_METATYPE(BASummaryForm)






//////////////////选择打印的凭证的对话框///////////////////////////////
class PrintSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrintSelectDialog(QList<PingZheng*> choosablePzSets, PingZheng* curPz, QWidget *parent = 0);
    ~PrintSelectDialog();
    void setPzSet(QSet<int> pznSet);
    void setCurPzn(int pzNum);
    int getSelectedPzs(QList<PingZheng*>& pzs);
    //int getPrintMode();

private slots:
    void selectedSelf(bool checked);
private:
    void enableWidget(bool en);
    QString IntSetToStr(QSet<int> set);
    bool strToIntSet(QString s, QSet<int>& set);

    Ui::PrintSelectDialog *ui;
    QSet<int> pznSet;  //欲对应的凭证号码的集合
    //AccountSuiteManager* pzMgr;
    PingZheng* curPz;         //当前显示的凭证（可能在凭证编辑对话框或历史凭证对话框中）
    QList<PingZheng*> pzSets; //可选的凭证集合
};





////////////////////登录对话框类///////////////////////////////////////////
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();
    User* getLoginUser();

private slots:
    void on_btnLogin_clicked();

    void on_btnCancel_clicked();

private:
    void init();

    Ui::LoginDialog *ui;
};


////////////////////系统安全配置对话框类//////////////////////////////////
class SecConDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SecConDialog(QWidget *parent = 0);
    ~SecConDialog();

    enum TabIndexEnum{
        TI_GROUP    = 0,
        TI_USER     = 1,
        TI_RIGHT    = 2,
        TI_RIGHTTYPE= 3,
        TI_OPERATER = 4
    };

private slots:
    void currentTabChanged(int index);

    void onRightellChanged(int row, int column);

    void on_actAddRight_triggered();

    void on_actDelRight_triggered();

    void on_btnSave_clicked();

    void on_btnClose_clicked();

    void on_actChgGrpRgt_triggered();

    void on_actChgOpeRgt_triggered();

    void on_actChgUserOwner_triggered();

    void on_lwGroup_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_lwUsers_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_actAddGrpForUser_triggered();

    void on_actDelGrpForUser_triggered();

    void on_actAddGroup_triggered();

    void on_actDelGroup_triggered();

    void on_actAddUser_triggered();

    void on_actDelUser_triggered();

    void on_actAddAcc_triggered();

    void on_actDelAcc_triggered();

private:
    void init();
    QList<RightType*> getRightType(RightType* parent = NULL);
    QList<Right*> getRightsForType(RightType* type);

    //组相关函数
    //void initRightTreesInGroupPanel();
    void genRightTree(RightType* type, bool isLeaf = false, QTreeWidgetItem* parent=NULL);
    void initRightTypes(int pcode, QTreeWidgetItem* pitem = NULL);
    void refreshRightForGroup(UserGroup* group, QTreeWidgetItem *parent=NULL);
    void collectRightForGroup(QSet<Right*> &rs, QTreeWidgetItem *parent=NULL);
    void isCurGroupChanged(UserGroup* g);

    //用户相关函数
    void viewUserInfos(User* u);
    void isCurUserChanged(User* u);

    QTreeWidgetItem* findItem(QTreeWidget* tree, int code, QTreeWidgetItem* startItem = NULL);
    void saveRights();
    void saveGroups();
    void saveUsers();
    void saveOperates();

    Ui::SecConDialog *ui;
    AppConfig* appCon;
    TabIndexEnum curTab;
    //这些集合保存被修改的对象，以便在退出时保存
    QSet<UserGroup*> set_groups;
    QSet<User*> set_Users;

    QIntValidator* vat; //用于验证代码
    //数据修改标记
    //bool rightDirty,groupDirty,userDirty,operDirty;
};

//凭证搜索对话框类
class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = 0);
    ~SearchDialog();

private:
    Ui::SearchDialog *ui;
};



#endif // DIALOG2_H
