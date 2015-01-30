#ifndef TRANSFEROUTDIALOG_H
#define TRANSFEROUTDIALOG_H

#include <QDialog>
#include <QDir>
#include <QSqlDatabase>
#include <QStack>

#include "commdatastruct.h"
#include "ui_transferindialog.h"
#include "ui_batchoutputdialog.h"
#include "ui_batchimportdialog.h"

namespace Ui {
class TransferOutDialog;
}

class AppConfig;
struct AccountCacheItem;
struct AccountTranferInfo;
class WorkStation;

const QString TRANSFER_MANAGER_CONNSTR = "AccountTranserConnect";
const QString TEM_BACKUP_DIRNAME = "temBackup";

//捎带主机表列索引
enum MacColIndex{
    MCI_MID     = 0,
    MCI_TYPE    = 1,
    MCI_OSTYPE  = 2,
    MCI_NAME    = 3,
    MCI_DESC    = 4
};





/**
 * @brief 由转移账户捎带的应用配置信息结构
 */
struct TakeAppCfgInfos{
    QList<BaseDbVersionEnum> verTypes;  //版本类别（表示那一类配置信息）
    QStringList verNames;               //版本类别的可视名称
    QList<int> mvs;                     //主版本号列表
    QList<int> svs;                     //次版本号列表
    QList<QByteArray *> infos;          //配置信息的二级值载体
};

/**
 * @brief 账户转移操作实用类
 * 执行账户转入、转出操作的实用类
 */
class AccountTransferUtil : public QObject
{
    Q_OBJECT
public:
    AccountTransferUtil(bool out = true, QObject* parent=0);
    ~AccountTransferUtil();
    bool transferOut(QString fileName, QString desDirName, QString intent, QString &info,WorkStation* desWS=0,bool isTake=false,TakeAppCfgInfos* cfgs=0);
    bool transferIn(QString fileName,QString &info);

    bool setAccontFile(QString fname);
    bool getAccountBaseInfos(QString &acode, QString &sname,QString &lname);
    bool canTransferIn(AccountCacheItem* acItem, QString &infos);
    AccountTranferInfo *getLastTransRec();

private:
    bool isExistAppCfgInfo();
    bool attechAppCfgInfo(TakeAppCfgInfos* cfgs);
    bool backup(QString filename, bool isMasteBD=false);
    bool restore(QString filename, bool isMasteBD=false);
    bool saveTransferRecord(AccountTranferInfo* rec);
    bool upgradeAppCfg();
    bool clearTemAppCfgTable();
    bool getAccountCode(QString &aCode);

    QSqlDatabase db;
    bool trMode;                        //true：转出，false：转入
    AccountTranferInfo* trRec;          //账户最近的转移记录
    QHash<int,WorkStation*> localMacs;  //本地拥有的工作站列表
    AppConfig* appCfg;
    QString bkDirName;                  //临时备份目录名
};

class TransferOutDialog : public QDialog
{
    Q_OBJECT

    //账户不能转出的原因
    enum DontTranReason{
        DTR_OK          = 0,
        DTR_NULLACC     = 1, //账户未选择
        DTR_NOTSTATE    = 2, //选定账户转移状态不符（该账户没有处于已转入目的主机状态）
        DTR_NULLDESC    = 3, //未填写转出说明信息
        DTR_NULLMACHINE = 4, //未选择转出目的机
        DTR_NOTLOCAL    = 5  //本机未定义
    };
    
public:
    explicit TransferOutDialog(QWidget *parent = 0);
    ~TransferOutDialog();
    QString getAccountFileName();
    WorkStation* getDestiMachine();
    QString getDescription();
    bool isTakeAppConInfo();
    QDir getDirection();
    
private slots:
    void selectAccountChanged(int index);
    void transInPhraseSelected(QString text);

    void on_cmbMachines_currentIndexChanged(int index);

    void on_btnBrowser_clicked();

    void on_btnOk_clicked();

private:
    bool canTransferOut(DontTranReason& reason);

    Ui::TransferOutDialog *ui;
    AppConfig* conf;
    QHash<AccountTransferState, QString> tranStates; //转移状态名称表
    QList<AccountCacheItem*> accCacheItems; //本地缓存账户列表
    QList<WorkStation*> localMacs;  //本地主机、捎带主机列表
    WorkStation* lm;    //本机对象
};

class TransferInDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransferInDialog(QWidget *parent = 0);
    ~TransferInDialog();

private slots:
    void transInPhraseSelected(QString text);

    void on_btnOk_clicked();

    void on_btnSelectFile_clicked();

private:
    Ui::TransferInDialog *ui;
    QString fileName;   //选择的要引入的账户的绝对文件名
    AppConfig* conf;
    AccountTransferUtil* trUtil;
};

/**
 * @brief 批量转出账户类
 */
class BatchOutputDialog : public QDialog
{
    Q_OBJECT

public:
    enum ColumnIndex{
        CI_SEL      = 0,
        CI_CODE     = 1,
        CI_NAME     = 2,
        CI_INTENT   = 3     //转出意图
    };

    explicit BatchOutputDialog(QWidget *parent = 0);
    ~BatchOutputDialog();


private slots:
    void selectMacChanged(int index);
    void on_btnSelDir_clicked();

    void on_btnOk_clicked();

private:
    Ui::BatchOutputDialog *ui;
    AppConfig* appCfg;
    QList<AccountCacheItem *> accounts;
    WorkStation* desWs;
    QHash<MachineType,QString> macTypes;
    QHash<int,QString> osTypes;
};

/**
 * @brief 批量账户账户类
 */
class BatchImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchImportDialog(QWidget *parent = 0);
    ~BatchImportDialog();

private slots:
    void on_btnSel_clicked();

    void on_btnImp_clicked();

private:
    Ui::BatchImportDialog *ui;
    QStringList files;
};
#endif // TRANSFEROUTDIALOG_H
