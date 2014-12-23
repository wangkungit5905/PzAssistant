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

//捎带主机表列索引
enum MacColIndex{
    MCI_MID     = 0,
    MCI_TYPE    = 1,
    MCI_OSTYPE  = 2,
    MCI_NAME    = 3,
    MCI_DESC    = 4
};

//主机类
class Machine
{
public:
    Machine(int id, MachineType type,int mid,bool isLocal,QString name,QString desc,int osType=1);
    Machine(Machine& other);
    int getId(){return id;}
    int getMID(){return mid;}
    void setMID(int id){mid=id;}
    MachineType getType(){return type;}
    void setType(MachineType type){this->type=type;}
    bool isLocalStation(){return isLocal;}
    void setLocalMachine(bool local){isLocal=local;}
    QString name(){return sname;}
    void setName(QString name){sname=name;}
    QString description(){return desc;}
    void setDescription(QString desc){this->desc=desc;}
    int osType(){return _osType;}
    void setOsType(int type){_osType = type;}

    QString serialToText();
    static Machine* serialFromText(QString serialText);
    static void serialAllToBinary(int mv, int sv, QByteArray* ds);
    static bool serialAllFromBinary(QList<Machine*> &macs, int &mv, int &sv, QByteArray* ds);

    bool operator ==(const Machine &other) const;
    bool operator !=(const Machine &other) const;
private:
    int id;
    MachineType type;   //主机类型（电脑(1)、云(2)）
    int mid;            //主机标识
    bool isLocal;       //是否是本机
    QString sname;      //主机简称
    QString desc;       //主机全称（或描述信息）
    int _osType;         //宿主操作系统类型

    friend class AppConfig;
};
Q_DECLARE_METATYPE(Machine*)
bool byMacMID(Machine *mac1, Machine *mac2);

/**
 * @brief The TransferRecordManager class
 *  账户转移记录管理实用类
 */
class TransferRecordManager : public QObject
{
    Q_OBJECT
public:
    TransferRecordManager(QString filename,QWidget* parent=0);
    ~TransferRecordManager();
    bool setFilename(QString filename);
    bool saveTransferRecord(AccontTranferInfo* rec);
    bool attechAppCfgInfo(QList<BaseDbVersionEnum> &verTypes, QStringList &verNames, QList<int> &mvs, QList<int> &svs, QList<QByteArray*> &infos);
    bool upgradeAppCfg();
    bool clearTemAppCfgTable();
    bool getAccountInfo(QString& accCode, QString& accName, QString& accLName);
    int getSourceStationId(){return smid;}
    int getDestinateStationId(){return dmid;}
    AccontTranferInfo* getLastTransRec();
    bool isMacUpdated(){return macUpdated;}
private:
    QSqlDatabase db;
    bool isTranOut;                 //true：转出，false：转入
    bool connected;
    bool isExistTemAppCfgTable;     //是否存在捎带应用配置信息的临时表的标记
    bool macUpdated;                //本站拥有的工作站信息是否已更新
    int smid,dmid;                  //源站和目的站的id
    AccontTranferInfo* trRec;       //账户最近的转移记录
    QHash<int,Machine*> localMacs;  //本地拥有的工作站列表
    QWidget* p;
    AppConfig* conf;
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
    bool isTakeAppConInfo();
    QDir getDirection();
    
private slots:
    void selectAccountChanged(int index);

    void on_cmbMachines_currentIndexChanged(int index);

    void on_btnBrowser_clicked();

    void on_btnOk_clicked();

private:
    bool canTransferOut(DontTranReason& reason);


    Ui::TransferOutDialog *ui;
    AppConfig* conf;
    QHash<AccountTransferState, QString> tranStates; //转移状态名称表
    QList<AccountCacheItem*> accCacheItems; //本地缓存账户列表
    QList<Machine*> localMacs;  //本地主机、捎带主机列表
    Machine* lm;    //本机对象
    QDir dir;
    QList<User*> upUsers;   //更新的用户对象列表
};

class TransferInDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransferInDialog(QWidget *parent = 0);
    ~TransferInDialog();

private slots:

    void on_btnOk_clicked();

    void on_btnSelectFile_clicked();

private:
    Ui::TransferInDialog *ui;
    TransferRecordManager* trMgr;    
    QString fileName;   //选择的要引入的账户的绝对文件名
};

#endif // TRANSFEROUTDIALOG_H
