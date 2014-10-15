#ifndef DATABASEACCESSFORM_H
#define DATABASEACCESSFORM_H

#include <QWidget>
#include <QHash>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QListWidgetItem>

#include "delegates.h"

namespace Ui {
class DatabaseAccessForm;
}

class Account;
class AppConfig;

class DatabaseAccessForm : public QWidget
{
    Q_OBJECT
    //字段数据类型
    enum FieldDataType{
        FDT_NONE,
        FDT_INT,
        FDT_TEXT,
        FDT_DOUBLE
    };
    //字段定义
    struct FieldDefine{
        QString name;
        FieldDataType type;
    };
    
public:
    explicit DatabaseAccessForm(Account* account, AppConfig* config, QWidget *parent = 0);
    ~DatabaseAccessForm();
    void init();
    void parseTableDefine(QString tName,bool isAccount = true);
    void clear(bool isAccount = true);

private slots:
    void loadTable(bool isAccount = true);
    void curTableChanged(int index);
    void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight);
    void sqlTextChanged();
    void tableDoubleClicked(QListWidgetItem* item);
    void currentChanged();
    void on_btnRevert_clicked();

    void on_btnCommit_clicked();

    void on_btnExec_clicked();

    void on_btnClear_clicked();

    void on_insertRowAction_triggered();

    void on_deleteRowAction_triggered();

private:
    void updateActions();
    void adjustColWidth(QString t, bool isAccount);
    void enWidget(bool en);
    Ui::DatabaseAccessForm *ui;
    Account* account;
    AppConfig* appCfg;
    QList<QString> tableNames_acc;
    QHash<QString,QList<FieldDefine*> > tableDefines_acc;
    QList<QString> tableNames_base;
    QHash<QString,QList<FieldDefine*> > tableDefines_base;
    QHash<FieldDataType,qint16> colWidths; //每种数据类型的默认列宽

    QSqlTableModel* tModel;
    bool editMode;  //true：编辑模式，false：浏览模式
    QSqlDatabase adb,bdb;

    QAbstractItemDelegate* defDelegate;
    FourDecimalDoubleDelegate* delegate;
};

#endif // DATABASEACCESSFORM_H
