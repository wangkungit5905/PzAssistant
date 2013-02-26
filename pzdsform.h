#ifndef PZDSFORM_H
#define PZDSFORM_H

#include <QWidget>

namespace Ui {
class PzDSForm;
}

//客户名称结构
//struct ClientName{
//    QString sname,lname,acode;
//};

class PzDSForm : public QWidget
{
    Q_OBJECT
    
public:
    enum EnumPzDsType{
        PDT_INCOME     = 1,   //收入发票
        PDT_COST       = 2    //成本发票
    };

    explicit PzDSForm(int id, EnumPzDsType type = PDT_INCOME, QWidget *parent = 0);
    ~PzDSForm();
    
private slots:
    void on_btnClose_clicked();

    void on_btnUpToDS_clicked();

    void on_btnUpToPZ_clicked();

    void on_btnAddBill_clicked();

    void on_btnDelBill_clicked();

private:
    void init();
    void loadBills();
    void loadNewClients();

    Ui::PzDSForm *ui;
    EnumPzDsType type;
    int pzId;

};

#endif // PZDSFORM_H
