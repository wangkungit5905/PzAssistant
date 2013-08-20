#ifndef TRANSFEROUTDIALOG_H
#define TRANSFEROUTDIALOG_H

#include <QDialog>
#include <QDir>
#include <QSqlDatabase>
#include <QStack>

#include "commdatastruct.h"
#include "ui_transferindialog.h"

namespace Ui {
class TransferOutDialog;
}

class AppConfig;
struct AccountCacheItem;
struct AccontTranferInfo;

const QString TRANSFER_MANAGER_CONNSTR = "TranserManagerConnect";

//主机类
class Machine
{
public:
    Machine(int id, MachineType type,int mid,bool isLocal,QString name,QString desc);
    Machine(Machine& other);
    int getId(){return id;}
    int getMID(){return mid;}
    void setMID(int id){mid=id;}
    MachineType getType(){return type;}
    void setType(MachineType type){this->type=type;}
    bool isLocalMachine(){return isLocal;}
    void setLocalMachine(bool local){isLocal=local;}
    QString name(){return sname;}
    void setName(QString name){sname=name;}
    QString description(){return desc;}
    void setDescription(QString desc){this->desc=desc;}

    bool operator ==(const Machine &other) const;
    bool operator !=(const Machine &other) const;
private:
    int id;
    MachineType type;   //主机类型（电脑(1)、云(2)）
    int mid;            //主机标识
    bool isLocal;       //是否是本机
    QString sname;      //主机简称
    QString desc;       //主机全称（或描述信息）

    friend class AppConfig;
};

bool byMacMID(Machine *mac1, Machine *mac2);

/**
 * @brief The TransferRecordManager class
 *  账户转移记录管理实用类
 */
class TransferRecordManager : public QObject
{
    Q_OBJECT
public:
    TransferRecordManager(QString filename);
    bool setFilename(QString filename);
    bool saveTransferRecord(AccontTranferInfo* rec);
    bool attechMachineInfo(QList<Machine *> macs);
    bool updateMachines();
    bool clearTemMachineTable();
    bool getAccountInfo(QString& accCode, QString& accName, QString& accLName);
    AccontTranferInfo* getLastTransRec();
    bool isMacUpdated(){return macUpdated;}


private slots:
    void execUpdate();

private:
    QHash<int, Machine *> getMergeMacs();
    bool _getTakeMacs();
    bool _inspectSelUpMacs();

    QSqlDatabase db;
    bool connected;
    bool isExistTemMacTable;     //是否存在捎带的临时主机表标记
    bool macUpdated;             //主机信息是否已更新
    AccontTranferInfo* trRec; //账户最近的转移记录    
    QHash<int,Machine*> localMacs, takeMacs, mergeMacs;
    QList<Machine*> newMacs,upMacs,notMacs,notExistMacs;//新增、更新，未更新的、不存在的主机列表
    QList<Machine*> selUpMacs; //用户选择的需要更新的主机
    QTableWidget* tab;
};

class TransferOutDialog : public QDialog
{
    Q_OBJECT

    //账户不能转出的原因
    enum DontTranReason{
        DTR_OK          = 0,
        DTR_NULLACC     = 1, //账户未选择
        DTR_NOTSTATE    = 2, //选定账户转移状态不符（该账户没有处于以转入目的主机状态）
        DTR_NULLDESC    = 3, //未填写转出说明信息
        DTR_NULLMACHINE = 4, //未选择转出目的机
        DTR_NOTLOCAL    = 5  //本机未定义
    };
    
public:
    explicit TransferOutDialog(QWidget *parent = 0);
    ~TransferOutDialog();
    QString getAccountFileName();
    Machine* getDestiMachine();
    QString getDescription();
    bool isTakeMachineInfo();
    QDir getDirection();
    
private slots:
    void selectAccountChanged(int index);
    void enAct();

    void on_cmbMachines_currentIndexChanged(int index);

    void on_btnBrowser_clicked();

    void on_chkTake_toggled(bool checked);

    void on_btnOk_clicked();

    void on_actReset_triggered();

    void on_actDel_triggered();

private:
    bool canTransferOut(DontTranReason& reason);


    Ui::TransferOutDialog *ui;
    AppConfig* conf;
    QHash<AccountTransferState, QString> tranStates; //转移状态名称表
    QList<AccountCacheItem*> accCacheItems; //本地缓存账户列表
    QList<Machine*> localMacs,takeMacs;  //本地主机、捎带主机列表
    Machine* lm;    //本机对象
    QDir dir;
};

class TransferInDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransferInDialog(QWidget *parent = 0);
    ~TransferInDialog();

private slots:
    void refreshMachines();

    void on_btnOk_clicked();

    void on_btnSelectFile_clicked();

private:
    Ui::TransferInDialog *ui;
    TransferRecordManager* trMgr;    
    QString fileName;   //选择的要引入的账户的绝对文件名
};

#endif // TRANSFEROUTDIALOG_H
