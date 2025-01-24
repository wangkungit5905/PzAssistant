#ifndef IMPJOURNARYDLG_H
#define IMPJOURNARYDLG_H

#include "xlsxdocument.h"

#include <QDialog>
#include <QStringList>
#include <QAction>
#include <QActionGroup>
#include <QHash>
#include <QListWidget>
#include <QTableWidget>
#include <QSettings>

namespace Ui {
class ImpJournaryDlg;
}
using namespace QXlsx;

class SecondSubject;
class SubjectManager;
class Journal;

class ImpJournaryDlg : public QDialog
{
    Q_OBJECT

public:

    enum TableIndex{
        TI_DATE = 0,    //日期
        TI_SUMMARY = 1, //摘要
        TI_INCOME = 2,  //收入
        TI_PAY = 3,     //支出
        TI_BALANCE = 4, //余额
        TI_INVOICE = 5, //发票号
        TI_REMARK = 6,  //备注
        TI_VTAG = 7     //审核
    };

    enum ItemDataRole{
        DR_SUBJECT   = Qt::UserRole,         //表单对应的现金或银行子目对象
        DR_READED    = Qt::UserRole + 1,     //是否已读取表单（布尔）
        DR_READ_FORM_DB = Qt::UserRole + 2,  //流水账是否读取自数据库（True:读取自数据库，False：读取自Excel电子表格）
        DR_OBJ       = Qt::UserRole + 3      //保存流水账记录结构对象
    };

    explicit ImpJournaryDlg(SubjectManager* sm,QWidget *parent = 0);
    ~ImpJournaryDlg();

private slots:
    void sheetListContextMeny(const QPoint &pos);
    void processSubChanged(bool isChecked);
    void doubleItemReadSheet(QListWidgetItem *item);
    void curSheetChanged(int index);

    void on_btnSave_clicked();

    void on_btnBrower_clicked();    

private:
    void init();
    void initJs();
    bool readSheet(int index, QTableWidget* tw);
    void rendRow(int row, Journal* j, QTableWidget* tw);
    bool isContainKeyword(QString t,QStringList kws);
    bool hasInvoice(QString t);


    Ui::ImpJournaryDlg *ui;
    SubjectManager* sm;
    QStringList titles;
    QXlsx::Document *excel;
    QActionGroup *ag1;
    QHash<QAction*,SecondSubject*> actionMap;
    QIcon icon_bank,icon_cash;

    QSettings *kwSetting;
    QStringList kwBeginBlas;   //表征期初余额的关键字
    //QStringList kwEndBlas;   //表征期末余额的关键字
};

#endif // IMPJOURNARYDLG_H
