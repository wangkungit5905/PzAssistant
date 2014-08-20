#ifndef DIALOG2_H
#define DIALOG2_H

#include <QtGlobal>

#include "ui_printselectdialog.h"
#include "ui_logindialog.h"
#include "ui_searchdialog.h"

class PingZheng;
class User;

//////////////////选择打印的凭证的对话框///////////////////////////////
class PrintSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrintSelectDialog(QList<PingZheng*> choosablePzSets, PingZheng* curPz, QWidget *parent = 0);
    ~PrintSelectDialog();
    void setPzSet(QSet<int> pznSet);
    void setCurPzn(int pzNum);
    int getSelectedPzs(QList<PingZheng*>& pzs);

private slots:
    void selectedSelf(bool checked);
private:
    void enableWidget(bool en);
    QString IntSetToStr(QSet<int> set);
    bool strToIntSet(QString s, QSet<int>& set);

    Ui::PrintSelectDialog *ui;
    QSet<int> pznSet;  //欲对应的凭证号码的集合
    PingZheng* curPz;         //当前显示的凭证（可能在凭证编辑对话框或历史凭证对话框中）
    QList<PingZheng*> pzSets; //可选的凭证集合
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
