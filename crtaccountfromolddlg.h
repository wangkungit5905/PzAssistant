#ifndef CRTACCOUNTFROMOLDDLG_H
#define CRTACCOUNTFROMOLDDLG_H

#include <QDialog>
#include <QSqlDatabase>

#include "cal.h"
#include "commdatastruct.h"

namespace Ui {
class CrtAccountFromOldDlg;
}

class AppConfig;

class CrtAccountFromOldDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CrtAccountFromOldDlg(QWidget *parent = 0);
    ~CrtAccountFromOldDlg();

private slots:
    void on_btnSelect_clicked();

    void on_btnCreate_clicked();

private:
    bool removeDefCreated();
    bool transferNames();
    bool transferBanks();
    bool transferRates();
    bool transferSubjects();
    bool transferFSubExtras();
    bool transferSSubExtras();
    bool editTransferRecord();
    bool extraUnityInspect();
    bool subUsedInExtra(int fid, bool &isUsed, QString &info);

    Ui::CrtAccountFromOldDlg *ui;
    int year;                               //帐套年份
    QString oldFile,newFile;
    AppConfig* appCfg;
    QSqlDatabase db_o,db_n;
    QString conn_o,conn_n;
    QHash<QString,int> fidMaps_o,fidMaps_n; //新、老主目的科目代码->科目id
    QHash<int,int> fsubMaps;                //一级科目id映射表（键：老主目id，值：新主目id）
    //QHash<int,int> mixedSubSids;            //混合对接子目id映射表（键为老子目id，值为新子目id）
    QHash<int,int> mixedSubFids;            //混合对接一级科目映射表（键为老科目id，值为新科目id）

    int pRMB_n,pUSD_n,pRMB_o,pUSD_o;        //余额指针表（键为币种代码，值为指针值）
    QHash<int,Double> fsubExtrasRMB,fsubExtrasUSD,fsubMExtras;   //一级科目的原币余额和本币余额（键为科目id）
    QHash<int,MoneyDirection> fsubDirsRMB,fsubDirsUSD;         //一级科目的余额方向
};

#endif // CRTACCOUNTFROMOLDDLG_H
