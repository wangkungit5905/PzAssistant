#ifndef ACCOUNTPROPERTYCONFIG_H
#define ACCOUNTPROPERTYCONFIG_H

#include <QDialog>
#include <QStack>
#include "account.h"

#include "ui_apcbase.h"
#include "ui_apcsuite.h"
#include "ui_apcbank.h"
#include "ui_apcsubject.h"
#include "ui_apcreport.h"
#include "ui_apclog.h"
#include "ui_subsysjoincfgform.h"

namespace Ui {
class ApcBase;
}


class Account;
class QListWidget;
class QStackedWidget;
class FirstSubject;
class SecondSubject;


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
    enum EditActionEnum{
        EA_NONE = 0,
        EA_NEW  = 1,
        EA_EDIT = 2
    };

public:

    explicit ApcSuite(Account* account,QWidget *parent = 0);
    ~ApcSuite();
    void init();
private slots:
    void curSuiteChanged(int index);
    void suiteDbClicked(QListWidgetItem* item);
    void on_btnNew_clicked();

    void on_btnEdit_clicked();

    void on_btnUsed_clicked();

    void on_btnCommit_clicked();

private:
    void enWidget(bool en);
    bool joinExtra(int year, int sc, int dc);


    Ui::ApcSuite *ui;
    Account* account;
    QList<Account::AccountSuiteRecord*> suites;
    EditActionEnum editAction;
    QStack<QString> stack_s;
    QStack<int> stack_i;
    bool iniTag;
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
    enum APC_SUB_PAGEINDEX{
        APCS_SYS = 0,   //科目系统页
        APCS_SUB = 1,   //科目页
        //APCS_SND = 2,   //二级科目页
        APCS_NAME= 2    //名称条目页
    };

    enum APC_SUB_EDIT_ACTION{
        APCEA_NONE       = 0,   //
        APCEA_NEW_NICLS  = 1,   //新建名称条目类别名
        APCEA_EDIT_NICLS = 2,   //编辑名称条目类别名
        APCEA_NEW_NI     = 3,   //新建名称条目
        APCEA_EDIT_NI    = 4,   //编辑名称条目
        APCEA_NEW_SSUB   = 5,   //新建二级科目
        APCEA_EDIT_SSUB  = 6,   //编辑二级科目
        APCEA_EDIT_FSUB  = 7    //编辑一级科目
    };

public:
    explicit ApcSubject(Account* account, QWidget *parent = 0);
    ~ApcSubject();
    void init();
    void save();

private slots:
    //科目系统配置相关
    void importBtnClicked();
    void subSysCfgBtnClicked();
    //科目配置相关
    void selectedSubSys(bool checked);
    void curFSubClsChanged(int index);
    void fsubDBClicked(QListWidgetItem* item);
    void curFSubChanged(int row);
    void curSSubChanged(int row);
    void ssubDBClicked(QListWidgetItem* item);
    //名称条目配置相关
    void on_tw_currentChanged(int index);
    void currentNiClsRowChanged(int curRow);
    void niClsDoubleClicked(QListWidgetItem * item);
    void currentNiRowChanged(int curRow);
    void niDoubleClicked(QListWidgetItem * item);
    void loadNameItems();
    void on_btnNiEdit_clicked();

    void on_btnNiCommit_clicked();

    void on_btnNewNI_clicked();

    void on_btnDelNI_clicked();

    void on_btnNewNiCls_clicked();

    void on_btnNiClsEdit_clicked();

    void on_btnNiClsCommit_clicked();

    void on_btnDelNiCls_clicked();

    void on_niClsBox_toggled(bool en);

    void on_btnFSubEdit_clicked();

    void on_btnFSubCommit_clicked();

    void on_btnSSubEdit_clicked();

    void on_btnSSubCommit_clicked();

    void on_btnSSubDel_clicked();

private:
    void subJoinConfig(int sCode, int dCode);
    bool notCommitWarning();
    //bool testCommited();
    void init_subsys();
    void init_NameItems();
    void init_subs();

    void loadFSub(int subSys);
    void loadSSub();

    void viewFSub();
    void viewSSub();
    void viewNI(SubjectNameItem* ni);
    void viewNiCls(int cls);

    //控制界面显示部件的可编辑性
    void enFSubWidget(bool en);
    void enSSubWidget(bool en);
    void enNiWidget(bool en);
    void enNiClsWidget(bool en);


    Ui::ApcSubject *ui;

    //4个tab页的初始化完成标志
    bool iniTag_subsys;
    bool iniTag_ni;
    bool iniTag_sub;

    Account* account;
    QHash<int,QStringList> NiClasses; //名称条目类别表

    QList<SubSysNameItem*> subSysNames; //当前系统支持的科目系统列表

    //编辑缓存区
    QStack<QString> stack_strs;
    QStack<int> stack_ints;
    APC_SUB_EDIT_ACTION editAction;
    //APC_SUB_EDIT_ACTION fsubEditAction; //当前一级科目的编辑动作
    //APC_SUB_EDIT_ACTION niEditAction;  //当前名称条目编辑动作
    //APC_SUB_EDIT_ACTION niClsEditAction; //当前名称条目类别的编辑动作

    SubjectManager* curSubMgr;         //当前选择的科目系统
    FirstSubject* curFSub;
    SecondSubject* curSSub;
    SubjectNameItem* curNI;            //当前选中的名称条目
    int curNiCls;                      //当前选中的名称条目类别
};

/**
 * @brief 配置科目系统衔接的窗口类
 */
class SubSysJoinCfgForm : public QDialog
{
    Q_OBJECT

public:
    explicit SubSysJoinCfgForm(int src, int des, Account* account, QWidget *parent = 0);
    ~SubSysJoinCfgForm();
    bool save();

private slots:
    void mapBtnClicked();
    void destinationSubChanged(QTableWidgetItem* item);
private:
    void init();
    bool determineAllComplete();
    void cloneSndSubject();
    void preConfig();

    Ui::SubSysJoinCfgForm *ui;
    Account* account;
    bool isCompleted;     //科目衔接配置是否已经完成
    bool isCloned;  //二级科目是否已经克隆
    SubjectManager *sSmg,*dSmg;
    QList<SubSysJoinItem*> ssjs;    //科目映射配置列表
    QList<bool> editTags;   //每个科目的映射条目被修改的标记列表
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

    /**
     * @brief 账户属性配置页索引
     */
    enum APC_PAGEINDEX{
        APC_BASE     = 0,
        APC_SUITE    = 1,
        APC_BANK     = 2,
        APC_SUBJECT  = 3,
        APC_REPORT   = 4,
        APC_LOG      = 5
    };
    
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
