#ifndef OPTIONFORM_H
#define OPTIONFORM_H

#include "commdatastruct.h"
#include "config.h"

#include <QCloseEvent>
#include <QWidget>
#include <QIcon>

namespace Ui {
    class PzTemplateOptionForm;
    class StationCfgForm;
    class AppCommCfgPanel;
    class SubSysJoinCfgForm;
    class SpecSubCfgForm;
}

class QTableWidgetItem;
class QListWidgetItem;
class QListWidget;
class QStackedWidget;
class QEventLoop;

class MainWindow;
class Account;
class SubjectManager;
class WorkStation;

/**
 * @brief 配置面板基类，所有要加入到配置面板集合类的面板类都应该继承自此类
 */
class ConfigPanelBase : public QWidget{
    Q_OBJECT
public:
    ConfigPanelBase(QWidget* parent=0):QWidget(parent){}
    virtual bool isDirty()=0;
    virtual bool save()=0;
    virtual QString panelName(){return "";}
};

class ConfigPanels : public QWidget
{
    Q_OBJECT

    enum closeReason{
        R_UNCERTAIN = 0,    //通过窗口关闭小安纽
        R_OK        = 1,
        R_CANCEL    = 2
    };

public:
    explicit ConfigPanels(QByteArray* state=0, QWidget *parent = 0);
    QByteArray* getState();

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void btnOkClicked();
    void btnCancelClicked();

private:
    void addPanel(ConfigPanelBase* panel, QIcon icon);
    bool isDirty();
    bool save();

    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;
    closeReason reason;
    QList<int> isChangedIndexes; //发生了变更的面板索引
    QList<ConfigPanelBase*> panels;
    QList<QIcon> icons;
    AppConfig* appCon;
};

/**
 * @brief 应用通用配置面板类
 */
class AppCommCfgPanel : public ConfigPanelBase
{
    Q_OBJECT

public:
    explicit AppCommCfgPanel(QWidget *parent = 0);
    ~AppCommCfgPanel();
    bool isDirty();
    bool save();
    QString panelName(){return tr("通用配置");}

private slots:
    void styleChanged(bool checked);
    void styleFromChanged(bool checked);

private:
    void init();
    Ui::AppCommCfgPanel *ui;    
};

/**
 * @brief 凭证模板配置面板类
 */
class PzTemplateOptionForm : public ConfigPanelBase
{
    Q_OBJECT

public:
    explicit PzTemplateOptionForm(QWidget *parent = 0);
    ~PzTemplateOptionForm();

    bool isDirty();
    bool save();
    QString panelName(){return tr("凭证模板");}
private:
    Ui::PzTemplateOptionForm *ui;
    PzTemplateParameter parameter;
};

/**
 * @brief 特定科目配置面板类
 *
 */
class StationCfgForm : public ConfigPanelBase
{
    Q_OBJECT

    enum DataRole{
        DR_OBJ  = Qt::UserRole+1,    //保存对象
        DR_ES   = Qt::UserRole+2     //保存编辑状态
    };

public:
    explicit StationCfgForm(QWidget *parent = 0);
    void setListener(MainWindow* listener);
    ~StationCfgForm();

    bool isDirty();
    bool save();
    QString panelName(){return tr("工作站");}

private slots:
    void customContextMenuRequested(const QPoint & pos);
    void currentStationChanged(QListWidgetItem * current, QListWidgetItem * previous);
    void editStation(QListWidgetItem * item);

    void on_actAdd_triggered();

    void on_actEdit_triggered();

    void on_actDel_triggered();

signals:
    void localStationChanged(WorkStation* ws);

private:
    void loadStations();
    void showStation(WorkStation *m);
    void setEditState();
    void collectData(QListWidgetItem *item);

    Ui::StationCfgForm *ui;
    QList<WorkStation*> ms,msDels;
    bool readonly;
    QHash<int,QString> osTypes;
    QMetaObject::Connection conn;

    //本站敏感信息，如果这些信息改变了，则必须报告给主窗口
    int localMID;
    QString lName,lDesc;
};



/////////////////////////////////////////////////////////////
/**
 * @brief 配置或显示科目系统衔接的窗口类
 */
class SubSysJoinCfgForm : public ConfigPanelBase
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
    bool isDirty();
    QString panelName(){return tr("科目系统\n衔接配置");}
    int exec();
    void setVisible(bool visible);
protected:
    void closeEvent(QCloseEvent* e);

private slots:
    void destinationSubChanged(QTableWidgetItem* item);
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

private:
    void init();
    bool determineAllComplete();

    Ui::SubSysJoinCfgForm *ui;
    AppConfig* appCfg;
    Account* account;
    bool isCompleted;     //科目衔接配置是否已经完成
    SubjectManager *sSmg/*,*dSmg*/;
    int subSys,pre_subSys;           //对接和前一个的科目系统的代码
    QList<SubSysJoinItem2*> ssjs;    //科目映射配置列表
    QList<bool> editTags;   //每个科目的映射条目被修改的标记列表
    QHash<QString,QString> subNames; //新科目系统的科目代码到科目名的映射表
    QString defJoinStr,mixedJoinStr; //默认对接和混合对接的箭头样式文本
    QList<FirstSubject*> temFstSubs; //存放临时创建的一级科目对象

    QEventLoop* m_loop;
    int m_result;
};

/////////////////////SpecSubCfgForm/////////////////////
class SpecSubCfgForm : public ConfigPanelBase
{
    Q_OBJECT

public:
    const QString CFG_FILE_TAG = "Special subject"; //在导入或导出的配置文件中的第一行表证的是哪类配置文件的标记串
    const QString CFG_FILE_KEY_SYSNUM = "SubjectSystemCount";   //科目系统数
    const QString CFG_FILE_KEY_SUBSYS = "SubjectSystemCode";        //科目系统代码

    explicit SpecSubCfgForm(QWidget *parent = 0);
    ~SpecSubCfgForm();
    bool isDirty();
    bool save();
    QString panelName(){return tr("特定科目");}

private slots:
    void subSysChanged(int index);
    void on_btnExport_clicked();

    void on_btnImport_clicked();

private:
    Ui::SpecSubCfgForm *ui;
    AppConfig* appCfg;
    QHash<int,QStringList> codes;   //键为科目系统代码,值为特定科目代码
    QHash<int,QStringList> names;   //特定科目名称
};

///////////////////////////////
class TestPanel : public ConfigPanelBase
{
    Q_OBJECT
public:
    explicit TestPanel(QWidget* parent=0);
    ~TestPanel(){}

    bool isDirty(){return false;}
    bool save(){return false;}
    QString panelName(){return tr("测试页");}
};



#endif // OPTIONFORM_H
