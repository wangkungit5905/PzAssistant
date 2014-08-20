#ifndef SECCONDIALOG_H
#define SECCONDIALOG_H

#include "ui_seccondialog.h"

class AppConfig;
class Right;
class User;
class UserGroup;
struct RightType;


////////////////////系统安全配置对话框类//////////////////////////////////
class SecConDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SecConDialog(QByteArray* state, QWidget *parent = 0);
    ~SecConDialog();
    void setState(QByteArray* state);
    QByteArray* getState();

    enum TabIndexEnum{
        TI_GROUP    = 0,
        TI_USER     = 1,
        TI_RIGHT    = 2
    };
    //在树控件项目中放置的数据角色枚举
    enum ItemDataRole{
        ROLE_RIGHTTYPE_CODE = Qt::UserRole,     //权限类型代码
        ROLE_RIGHT_CODE     = Qt::UserRole + 1, //权限代码
        ROLE_USERGROUP      = Qt::UserRole,     //组代码
        ROLE_USER           = Qt::UserRole,     //用户代码
        ROLE_ACCOUNT_CODE   = Qt::UserRole      //账户代码
    };

protected:
    void closeEvent(QCloseEvent * event);

private slots:
    void currentTabChanged(int index);
    void listContextMenuRequested(const QPoint &pos);
    void groupNameChanged(QString name);
    void userNameChanged(QString name);
    void on_btnSave_clicked();

    void on_btnClose_clicked();

    void on_btnCancel_clicked();

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
    bool isDirty();
    void save();
    QList<RightType*> getRightType(RightType* parent = NULL);
    QList<Right*> getRightsForType(RightType* type);

    //组相关函数
    void genRightTree(QTreeWidget* tree, RightType* type, bool isLeaf = false, QTreeWidgetItem* parent=NULL);
    void refreshRightsForGroup(UserGroup* group, QTreeWidgetItem *parent=NULL);
    void collectRightsForGroup(QSet<Right*> &rs, QTreeWidgetItem *parent=NULL);
    void isCurGroupChanged(UserGroup* g);

    //用户相关函数
    void viewUserInfos(User* u);
    void isCurUserChanged(User* u);
    void refreshRightsForUser(User* u, QTreeWidgetItem *parent=NULL);
    void modifyRightsForUser(QSet<Right*> rs, QTreeWidgetItem *parent=NULL);
    void collectRightsForUser(QSet<Right*> &rs, QTreeWidgetItem *parent=NULL);

    Ui::SecConDialog *ui;
    AppConfig* appCon;
    TabIndexEnum curTab;
    //这些集合保存被修改的对象，以便在退出时保存
    QSet<UserGroup*> set_groups;
    QSet<User*> set_Users;
    bool dirty;
    bool isCancel;
};


#endif // SECCONDIALOG_H
