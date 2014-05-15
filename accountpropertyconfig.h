#ifndef ACCOUNTPROPERTYCONFIG_H
#define ACCOUNTPROPERTYCONFIG_H

#include <QDialog>
#include <QStack>
#include <QItemDelegate>

#include "commdatastruct.h"

#include "ui_apcbase.h"
#include "ui_apcsuite.h"
#include "ui_apcbank.h"
#include "ui_apcsubject.h"
#include "ui_apcdata.h"
#include "ui_apcreport.h"
#include "ui_apclog.h"
#include "ui_subsysjoincfgform.h"

namespace Ui {
class ApcBase;
}


class Account;
class QListWidget;
class QStackedWidget;
class SubjectManager;
class FirstSubject;
class SecondSubject;


class ApcBase : public QWidget
{
    Q_OBJECT

public:
    explicit ApcBase(Account* account, QWidget *parent = 0);
    ~ApcBase();
    void init();

private slots:
    void windowShallClosed();
    void textEdited();
    void on_addWb_clicked();

    void on_delWb_clicked();

private:
    Ui::ApcBase *ui;
    Account* account;
    bool iniTag;
    QList<Money*> wbs; //外币列表
    bool changed;
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
    void windowShallClosed();
    void curSuiteChanged(int index);
    void suiteDbClicked(QListWidgetItem* item);
    void on_btnNew_clicked();

    void on_btnEdit_clicked();

    void on_btnUsed_clicked();

    void on_btnCommit_clicked();

    void on_btnUpgrade_clicked();

private:
    void enWidget(bool en);


    Ui::ApcSuite *ui;
    Account* account;
    QHash<int,SubSysNameItem*> subSystems;
    QList<AccountSuiteRecord*> suites;
    EditActionEnum editAction;
    QStack<QString> stack_s;
    QStack<int> stack_i;
    bool iniTag;
};

class BankCfgNiCellWidget : public QTableWidgetItem
{
public:
    BankCfgNiCellWidget(SubjectNameItem* ni);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);
private:
    SubjectNameItem* ni;
};

class BankCfgItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    BankCfgItemDelegate(Account* account, QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
    void setReadOnly(bool isReadonly){readOnly = isReadonly;}
private:
    Account* account;
    bool readOnly;
};

class ApcBank : public QWidget
{
    Q_OBJECT

    enum EditActionEnum{
        EA_NONE = 0,
        EA_NEW  = 1,
        EA_EDIT = 2
    };    

public:
    enum ColumnIndex{
        CI_ID         = 0,
        CI_MONEY      = 1,
        CI_ACCOUNTNUM = 2,
        CI_NAME       = 3
    };

    explicit ApcBank(Account* account, QWidget *parent = 0);
    ~ApcBank();
    void init();
private slots:
    void windowShallClosed();
    void curBankChanged(int index);
    void curBankAccountChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void crtNameBtnClicked();
    void bankDbClicked();
    void on_editBank_clicked();

    void on_submit_clicked();

    void on_newBank_clicked();

    void on_delBank_clicked();

    void on_newAcc_clicked();

    void on_delAcc_clicked();

private:
    void viewBankAccounts();
    void enWidget(bool en);
    BankAccount* fondBankAccount(int id);

    Ui::ApcBank *ui;
    Account* account;
    //QList<BankAccount*> ba_news,ba_dels; //分别缓存新建和删除的帐号
    QList<Bank*> banks;
    QList<bool> editTags; //银行对象是否被修改过的标记

    Bank* curBank;

    //QStack<Bank*> stack;
    bool iniTag;
    EditActionEnum editAction;
    BankCfgItemDelegate* delegate;
};

class ApcSubject : public QWidget
{
    Q_OBJECT
    enum APC_SUB_PAGEINDEX{
        APCS_SYS = 0,   //科目系统页
        APCS_SUB = 1,   //科目页
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
    void windowShallClosed();
    //科目系统配置相关
    void importBtnClicked();
    void subSysCfgBtnClicked();
    //科目配置相关
    void selectedSubSys(bool checked);
    void curFSubClsChanged(int index);
    void fsubDBClicked(QListWidgetItem* item);
    void curFSubChanged(int row);
    void curSSubChanged(int row);
    void SelectedSSubChanged();
    void ssubDBClicked(QListWidgetItem* item);
    void defSubCfgChanged(bool checked);
    //名称条目配置相关
    void on_tw_currentChanged(int index);
    void currentNiClsRowChanged(int curRow);
    void niClsDoubleClicked(QListWidgetItem * item);
    void currentNiRowChanged(int curRow);
    void selectedNIChanged();
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

    void on_btnSSubAdd_clicked();

    void on_btnInspectNameConflit_clicked();

    void on_btnNIMerge_clicked();

    void on_rdoSortbyName_toggled(bool checked);

    void on_btnSSubMerge_clicked();

    void on_btnInspectDup_clicked();

private:

    bool mergeNameItem(SubjectNameItem* preNI, QList<SubjectNameItem*> nameItems);
    bool mergeSndSubject(QList<SecondSubject*> subjects, int &preSubIndex);
    bool notCommitWarning();
    void init_subsys();
    void init_NameItems();
    void init_subs();

    void loadFSub(int subSys);
    void loadSSub(SortByMode sortBy = SORTMODE_NAME);

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

    SubjectManager* curSubMgr;         //当前选择的科目系统
    FirstSubject* curFSub;
    SecondSubject* curSSub;
    SubjectNameItem* curNI;            //当前选中的名称条目
    int curNiCls;                      //当前选中的名称条目类别

    QBrush color_enabledSub,color_disabledSub;//启用和禁用科目的颜色
};

/**
 * @brief 配置科目系统衔接的窗口类
 */
class SubSysJoinCfgForm : public QDialog
{
    Q_OBJECT

    static const int COL_INDEX_SUBCODE = 0;     //源科目代码列
    static const int COL_INDEX_SUBNAME = 1;     //源科目名称列
    static const int COL_INDEX_SUBJOIN = 2;     //映射按钮列
    static const int COL_INDEX_NEWSUBCODE = 3;	//新科目代码列
    static const int COL_INDEX_NEWSUBNAME = 4;	//新科目名称列

public:
    explicit SubSysJoinCfgForm(int src, int des, Account* account, QWidget *parent = 0);
    ~SubSysJoinCfgForm();
    bool save();

private slots:
    //void mapBtnClicked();
    void destinationSubChanged(QTableWidgetItem* item);
private:
    void init();
    bool determineAllComplete();
    //void cloneSndSubject();
    //void preConfig();

    Ui::SubSysJoinCfgForm *ui;
    Account* account;
    bool isCompleted;     //科目衔接配置是否已经完成
    SubjectManager *sSmg/*,*dSmg*/;
    int subSys;                      //对接的科目系统的代码
    QList<SubSysJoinItem2*> ssjs;    //科目映射配置列表
    QList<bool> editTags;   //每个科目的映射条目被修改的标记列表
    QHash<QString,QString> subNames; //新科目系统的科目代码到科目名的映射表
    QString defJoinStr,mixedJoinStr; //默认对接和混合对接的箭头样式文本
};

//显示期初余额的借贷方向
class DirView : public QTableWidgetItem
{
public:
    DirView(MoneyDirection dir=MDIR_P,int type = QTableWidgetItem::UserType);
    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

private:
    MoneyDirection dir;
};

//编辑期初余额的借贷方向
class DirEdit_new : public QComboBox
{
    Q_OBJECT
public:
    DirEdit_new(MoneyDirection dir = MDIR_J, QWidget* parent = 0);
    void setDir(MoneyDirection dir);
    MoneyDirection getDir();
protected:
    void keyPressEvent(QKeyEvent* e );
signals:
    void dataEditCompleted(int col, bool isMove);
    void editNextItem(int row, int col);   //
private:
    MoneyDirection dir;
    int row,col;
};

/**
 * @brief 期初余额配置项目代理类
 */
class BeginCfgItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    BeginCfgItemDelegate(Account* account, bool readOnly=true, bool isFSub=false, QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                              const QModelIndex& index) const;
    void setReadOnly(bool isReadonly){readOnly = isReadonly;}
private slots:
    void commitAndCloseEditor(int colIndex, bool isMove);
signals:
    void moneyChanged(int oldMt, int newMt);
    void dirChanged(MoneyDirection oldDir, MoneyDirection newDir);
    void pvChanged(Double ov, Double nv);
    void mvChanged(Double ov, Double nv);

private:
    Account* account;
    bool readOnly;
    bool isFSub;           //是否用于一级科目的余额表格（默认为false）
    QHash<int,Money*> mts;

//    Money* oldMt;
//    MoneyDirection oldDir;
//    Double oldPv,oldMv;
};


/**
 * @brief 账户期初余额配置类（兼具余额显示）
 */
class ApcData : public QWidget
{
    Q_OBJECT

public:
    //期初余额表的列索引
    enum ColIndex{
        CI_MONEY    = 0,
        CI_DIR      = 1,
        CI_PV       = 2,
        CI_MV       = 3
    };

    explicit ApcData(Account* account, bool isCfg, QWidget *parent = 0);
    ~ApcData();
    void init();
    void setYM(int year, int month);

private slots:
    void curFSubChanged(int index);
    void curSSubChanged(int index);
    void curMtChanged(int index);
    void adjustColWidth(int col, int oldSize, int newSize);
    void dataChanged(QTableWidgetItem *item);
    void monthChanged(int m);
    void windowShallClosed();

    void on_add_clicked();

    void on_save_clicked();

    void on_actSetRate_triggered();

private:
    bool viewRates();
    void collect();
    bool exist(int sid);
    void viewCollectData();
    void watchDataChanged(bool en=true);
    void enAddBtn();

    Ui::ApcData *ui;
    Account *account;
    bool iniTag;
    SubjectManager* smg;
    FirstSubject* curFSub;
    SecondSubject* curSSub;
    QHash<int,Double> rates;
    QHash<int,Double> pvs,mvs;       //当前一级科目下所有二级科目的期初余额（原币、本币形式）（复合键：二级科目id+币种代码）
    QHash<int,MoneyDirection> dirs;  //当前一级科目下所有二级科目的期初余额方向
    QHash<int,Double> pvs_f,mvs_f;      //当前一级科目汇总后的期初余额（键为币种代码）
    QHash<int,MoneyDirection> dir_f;    //当前一级科目汇总后的期初余额方向
    int y,m;                    //期初年月（或余额年月）
    QHash<int,Money*> mts;      //账户所使用的所有币种对象
    QList<int> mtSorts;         //显示余额值的顺序（本币始终是第一个）
    BeginCfgItemDelegate *delegate,*delegate_fsub;
    bool readOnly;              //期初余额是否可编辑
    QBrush bg_red;              //当二级科目有余额项时所采用的前景色
    bool extraCfg;              //是期初余额配置（true：默认），还是余额显示
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
        APC_BASE     = 0,   //账户基本信息
        APC_SUITE    = 1,   //帐套信息
        APC_BANK     = 2,   //开户银行信息
        APC_SUBJECT  = 3,   //科目配置
        APC_DATA     = 4,   //期初数据配置
        APC_REPORT   = 5,   //报表配置
        APC_LOG      = 6    //账户日志
    };
    
public:
    explicit AccountPropertyConfig(Account* account, QWidget *parent = 0);
    ~AccountPropertyConfig();
    QByteArray* getState(){return NULL;}
    void setState(QByteArray* state){}

public slots:
    void closeAllPage();
private slots:
    void pageChanged(int index);
signals:
    void windowShallClosed();
private:
    void createIcons();
private:
    Account* account;
    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;
};




#endif // ACCOUNTPROPERTYCONFIG_H
