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

namespace Ui {
class ApcBase;
}

class AppConfig;
class Account;
class QListWidget;
class QStackedWidget;
class SubjectManager;
class FirstSubject;
class SecondSubject;
class SubSysJoinCfgForm;


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
    void currentUserChanged(QListWidgetItem * current, QListWidgetItem * previous);
    void on_addWb_clicked();

    void on_delWb_clicked();

    void on_addUser_clicked();

    void on_removeUser_clicked();

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

signals:
    void suiteUpdated();
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

    bool isEdit;
    bool iniTag;
    EditActionEnum editAction;
    BankCfgItemDelegate* delegate;
};

/**
 * @brief 智能子目适配配置项结构
 */
struct SmartSSubAdapteItem{
    int id;
    int subSys;         //科目系统代码
    FirstSubject* fsub; //关联主目
    SecondSubject* ssub;//适配子目
    QString keys;       //关键词
};

/**
 * @brief 编辑智能子目适配项的对话框类
 */
class SmartSSubAdapteEditDlg : public QDialog
{
    Q_OBJECT
public:
    SmartSSubAdapteEditDlg(SubjectManager* sm, bool isEdit=true, QWidget* parent=0);
    FirstSubject* getFirstSubject(){return _fsub;}
    void setFirstSubject(FirstSubject* fsub);
    SecondSubject* getSecondSubject(){return _ssub;}
    void setSecondSubject(SecondSubject* ssub);
    QString getKeys(){return edtKeys.text();}
    void setKeys(QString keys);
private slots:
    void currentAdapteItemChanged(int currentRow);
    void firstSubjectChanged(int index);
private:
    void loadSSubs();

    QHBoxLayout h1,h2,h3;
    QVBoxLayout* lm;
    QLabel lab1,lab2;
    QPushButton btnOk,btnCancel;
    QLineEdit edtKeys;
    QComboBox cmbFSubs;
    QListWidget lw;
    SubjectManager* _sm;
    FirstSubject* _fsub;
    SecondSubject* _ssub;
    bool _isEdit;     //false：导入默认配置项时的确认对话框，true：编辑对话框
};

class ApcSubject : public QWidget
{
    Q_OBJECT
    enum APC_SUB_PAGEINDEX{
        APCS_SYS = 0,           //科目系统页
        APCS_SUB = 1,           //科目页
        APCS_NAME= 2,           //名称条目页
        APCS_ALIAS       = 3,   //别名
        APCS_SMARTADAPTE = 4    //智能适配子目
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
    //智能适配配置相关
    void subjectSystemChanged(int index);
    void SmartTableMenuRequested(const QPoint & pos);    
    //别名管理
    void curNameObjChanged(int index);
    void curAliasChanged(int index);
    void showIsolatedAlias(bool checked);

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

    void on_btnLoadDefs_clicked();

    void on_btnSaveSmart_clicked();

    void on_actAddSmartItem_triggered();

    void on_actEditSmartItem_triggered();

    void on_actRemoveSmartItem_triggered();

    void on_edtNI_NampInput_textEdited(const QString &arg1);

    void on_edtSSubNameInput_textEdited();

    void aliasListContextMenuRequest(const QPoint &pos);

    void on_actDelAlias_triggered();

private:

    bool mergeNameItem(SubjectNameItem* preNI, QList<SubjectNameItem*> nameItems);
    bool mergeSndSubject(QList<SecondSubject*> subjects, int &preSubIndex,bool &isCancel);
    bool notCommitWarning();
    void init_subsys();
    void init_NameItems();
    void init_subs();
    void init_alias();
    void init_smarts();
    void loadSmartItems(SubjectManager* sm);

    void loadFSub(int subSys);
    void loadSSub(SortByMode sortBy = SORTMODE_NAME);
    void loadIsolatedAlias();

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

    //特权标志
    bool isPrivilegeUser;   //是否特权用户
    bool isFSubSetRight;    //一级科目设置权限
    bool isSSubSetRight;    //二级科目设置权限

    //4个tab页的初始化完成标志
    bool iniTag_subsys;
    bool iniTag_ni;
    bool iniTag_sub;
    bool iniTag_alias;
    bool iniTag_smart;

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

    QList<SmartSSubAdapteItem *> SmartAdaptes;      //智能子目适配配置项列表
    QList<SmartSSubAdapteItem *> SmartAdaptes_del;  //被移除的配置项

    QBrush color_enabledSub,color_disabledSub;//启用和禁用科目的颜色
    QList<int> ni_fuzzyNameIndexes; //名称条目配置页面中模糊定位索引
    QList<int> ssub_fuzzyNameIndexes;
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

    explicit ApcData(Account* account, bool isCfg, QByteArray* state=0, QWidget *parent = 0);
    ~ApcData();
    void init(QByteArray* state=0);
    void setYM(int year, int month);
    QByteArray *getProperState();

private slots:
    void curFSubChanged(int index);
    void curSSubChanged(int index);
    void curMtChanged(int index);
    void adjustColWidth(int col, int oldSize, int newSize);
    void dataChanged(QTableWidgetItem *item);
    void monthChanged(int m);
    void nameChanged(const QString &name);
    void searchModeChanged(bool isPre);
    void windowShallClosed();

    void on_add_clicked();

    void on_save_clicked();

    void on_actSetRate_triggered();



private:
    bool isRateNull();
    bool viewRates();
    void collect();
    bool exist(int sid);
    void viewCollectData();
    void watchDataChanged(bool en=true);
    void enAddBtn();
    void getNextMonth(int y, int m, int &yy, int &mm);
    void fuzzySearch(bool isPre=true);

    Ui::ApcData *ui;
    Account *account;
    bool iniTag;
    SubjectManager* smg;
    FirstSubject* curFSub;
    SecondSubject* curSSub;
    QHash<int,Double> srates,erates; //期初期末汇率
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
    QFont boldFont;
    bool extraCfg;              //是期初余额配置（true：默认），还是余额显示

    //需要保存的专有状态信息（b：期初编辑，e：余额显示）
    qint16 b_fsubId,b_ssubId,e_fsubId,e_ssubId,e_y;
    qint8 e_m;
    QList<int> fuzzyNameIndexes;    //名称模糊搜索定位索引
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
    explicit AccountPropertyConfig(Account* account, QByteArray* cinfo, QWidget *parent = 0);
    ~AccountPropertyConfig();
    QByteArray* getCommonState();
    void setCommonState(QByteArray* state);

public slots:
    void closeAllPage();
private slots:
    void pageChanged(int index);
    void suiteUpdated(){emit suiteChanged();}
signals:
    void suiteChanged();
    void windowShallClosed();
private:
    void createIcons();
private:
    Account* account;
    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;
};




#endif // ACCOUNTPROPERTYCONFIG_H
