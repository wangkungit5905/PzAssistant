#ifndef DIALOG2_H
#define DIALOG2_H

#include <QtGlobal>

#include <QWidget>
#include <QSqlQueryModel>
#include <QStandardItemModel>
#include <QMenu>

#include "delegates2.h"
#include "securitys.h"
#include "HierarchicalHeaderView.h"
#include "widgets.h"
#include "account.h"

#include "ui_basummaryform.h"
#include "ui_ratesetdialog.h"
#include "ui_printselectdialog.h"
#include "ui_logindialog.h"
#include "ui_seccondialog.h"
#include "ui_searchdialog.h"


#ifdef Q_OS_LINUX
#include "ExcelFormat.h"
//#include "BasicExcel.h"
using namespace ExcelFormat;
using namespace YExcel;
#endif

#ifdef Q_OS_WIN
#include "excelUtils.h"
#endif

#define FSTSUBTYPE QTreeWidgetItem::UserType+1  //放置一级科目的树节点的类型

class PingZheng;
class AccountSuiteManager;
class BASummaryForm : public QWidget
{
    Q_OBJECT

public:
    explicit BASummaryForm(QWidget *parent = 0);    
    ~BASummaryForm();
    void setData(QString data);
    QString getData();

signals:
    void dataEditCompleted(ActionEditItemDelegate2::ColumnIndex col);

private:
    Ui::BASummaryForm *ui;
};

//Q_DECLARE_METATYPE(BASummaryForm)






//////////////////选择打印的凭证的对话框///////////////////////////////
class PrintSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrintSelectDialog(AccountSuiteManager* pzMgr,QWidget *parent = 0);
    ~PrintSelectDialog();
    void setPzSet(QSet<int> pznSet);
    void setCurPzn(int pzNum);
    int getSelectedPzs(QList<PingZheng*>& pzs);
    int getPrintMode();

private slots:
    void selectedSelf(bool checked);
private:
    void enableWidget(bool en);
    QString IntSetToStr(QSet<int> set);
    bool strToIntSet(QString s, QSet<int>& set);

    Ui::PrintSelectDialog *ui;
    QSet<int> pznSet;  //欲对应的凭证号码的集合
    AccountSuiteManager* pzMgr;
};





////////////////////登录对话框类///////////////////////////////////////////
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();
    User* getLoginUser();

private slots:
    void on_btnLogin_clicked();

    void on_btnCancel_clicked();

private:
    void init();

    Ui::LoginDialog *ui;
};


////////////////////系统安全配置对话框类//////////////////////////////////
class SecConDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SecConDialog(QWidget *parent = 0);
    ~SecConDialog();

private slots:
    void onRightellChanged(int row, int column);

    void on_actAddRight_triggered();

    void on_actDelRight_triggered();

    void on_btnSave_clicked();

    void on_btnClose_clicked();

    void on_actChgGrpRgt_triggered();

    void on_actChgOpeRgt_triggered();

    void on_actChgUserOwner_triggered();



    void on_lwGroup_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void init();
    void initRightTypes(int pcode, QTreeWidgetItem* pitem = NULL);
    QTreeWidgetItem* findItem(QTreeWidget* tree, int code, QTreeWidgetItem* startItem = NULL);
    void saveRights();
    void saveGroups();
    void saveUsers();
    void saveOperates();

    Ui::SecConDialog *ui;
    //QSqlDatabase db;
    //QSqlTableModel* rmodel;

    QIntValidator* vat; //用于验证代码
    //数据修改标记
    bool rightDirty,groupDirty,userDirty,operDirty;
};

//凭证搜索对话框类
class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = 0);
    ~SearchDialog();

private:
    Ui::SearchDialog *ui;
};



#endif // DIALOG2_H
