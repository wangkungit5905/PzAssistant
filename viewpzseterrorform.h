#ifndef VIEWPZSETERRORFORM_H
#define VIEWPZSETERRORFORM_H

#include <QDialog>
#include <QThread>
#include <QByteArray>

#include "commdatastruct.h"

class Account;
class SubjectManager;
class AccountSuiteManager;


namespace Ui {
class ViewPzSetErrorForm;
}

//class InspectPzErrorThread : public QThread
//{
//    Q_OBJECT
//public:
//    InspectPzErrorThread(int y, int m, Account *account, QObject * parent = 0);
//    void run();
//    bool isErrExist(){return !es.empty();}
//    QList<PingZhengError*> getErrors(){return es;}

//signals:
//    //void adviceAmount(int pzAmount);     //报告此次检测的凭证总数
//    void adviceProgressStep(int stepNum, int pzAmount); //报告进度

//private:
//    bool inspectMtError(int fsid, int ssid, int mt);
//    bool inspectDirEngage(int fsid, int dir, PzClass pzc, QString& eStr);
//private:
//    Account* account;
//    QSqlDatabase db;
//    SubjectManager* smg;
//    int year,month;
//    QList<PingZhengError*> es;
//};

class ViewPzSetErrorForm : public QDialog
{
    Q_OBJECT
    
public:
    explicit ViewPzSetErrorForm(AccountSuiteManager* pzMgr, QByteArray* state = NULL, QWidget *parent = 0);
    ~ViewPzSetErrorForm();
    //void setErrors(QList<PingZhengError*> es);
    void setState(QByteArray* state){}
    QByteArray* getState(){return NULL;}
    void inspect();

private slots:
    void doubleClicked(int row, int column);
    //void viewStep(int step, int AmountStep);
    //void inspcetPzErrEnd();

    void on_cmbLevel_currentIndexChanged(int index);

    void on_btnInspect_clicked();

private:
    void viewErrors();
signals:
    void reqLoation(int pid, int bid);   //请求定位到指定凭证，并加亮选择出错的会计分录
private:
    Ui::ViewPzSetErrorForm *ui;
    QList<PingZhengError*> es;
    QHash<int,QString> wInfos,eInfos; //警告、错误等级别的各种错误描述文本
    int curLevel;
    //InspectPzErrorThread* t;
    Account *account;
    AccountSuiteManager* pzMgr;
};

#endif // VIEWPZSETERRORFORM_H
