#ifndef IMPBEGINEXTRADLG_H
#define IMPBEGINEXTRADLG_H

#include "xlsxdocument.h"
#include "cal.h"
#include "commdatastruct.h"

#include <QDialog>

namespace Ui {
class ImpBeginExtraDlg;
}

using namespace QXlsx;

class QTableWidget;
class DbUtils;
class Account;
class SubjectManager;
class FirstSubject;
class SubjectNameItem;
class SecondSubject;

class ImpBeginExtraDlg : public QDialog
{
    Q_OBJECT

public:
    enum ColumnIndex{
        CI_CODE = 0,
        CI_CLIENT = 1,
        CI_JV = 2,
        CI_DV = 3
    };

    explicit ImpBeginExtraDlg(Account* account,QWidget *parent = 0);
    ~ImpBeginExtraDlg();

private slots:
    void on_btnBrowse_clicked();

    void on_btnImport_clicked();

private:
    void readExtra(bool isYs=true);
    void appendExtraRow(QString code,QString clientName,Double jv,Double dv, QTableWidget* tw);
    SubjectNameItem *getMacthNameItem(QString longName);
    SecondSubject *getMatchSubject(SubjectNameItem* ni, FirstSubject *fsub);
    void execSum(bool isYs=true);

    Ui::ImpBeginExtraDlg *ui;
    QXlsx::Document *excel;
    Worksheet *sheetYs,*sheetYf;
    Account* account;
    SubjectManager* sm;
    FirstSubject *ysFSub,*yfFSub;
    QList<SubjectNameItem*> nameItems,globalNameItems;
    QList<SecondSubject*> ysSndSubjects,yfSndSubjects;
    int clientClsCode;
    QHash<int,Double> ys_f,yf_f;        //应收应付一级科目汇总后的期初余额（键为币种代码）
    QHash<int,MoneyDirection> dir_f_ys,dir_f_yf;    //应收应付一级科目汇总后的期初余额方向
    QHash<int,Double> pvs_ys,pvs_yf;       //应收应付一级科目下所有二级科目的期初余额（原币、本币形式）（复合键：二级科目id+币种代码）
    QHash<int,MoneyDirection> dirs_ys,dirs_yf;  //应收应付一级科目下所有二级科目的期初余额方向
};

#endif // IMPBEGINEXTRADLG_H
