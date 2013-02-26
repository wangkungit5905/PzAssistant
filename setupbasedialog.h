#ifndef SETUPBASEDIALOG_H
#define SETUPBASEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QString>
#include <QHash>
#include <QStandardItemModel>
#include <QSqlTableModel>
#include <QDataWidgetMapper>
#include <QGridLayout>
#include <QLabel>



namespace Ui {
    class SetupBaseDialog;
}


class SetupBaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetupBaseDialog(bool isWizard = true, QWidget *parent = 0);
    ~SetupBaseDialog();

public slots:
    void switchPage();
    void saveExtra();
    void saveProfit();
    void saveAsset();
    void checkBoxToggled (bool checked);
    void endStep();
    void readExtraData(const QDate &date);
    void viewSubExtra(QString code, const QPoint& pos);
    void refreshExtra();

signals:
    void toNextStep(int curStep, int nextStep);


private:
    void crtExtraPage();
    void crtReportPage(int clsId, int  typeId);



    Ui::SetupBaseDialog *ui;

    bool isWizard; //是否在向导步骤中打开此对话框
    QHash<QString, QLineEdit*> edtHash;  //显示余额的部件对象名到对象指针的映射
    QHash<int, QGridLayout*> lytHash;    //科目类别代码到布局容器对象指针的映射
//    QList<QStandardItemModel*> mplst;  //保存利润表数据的模型的列表
//    QList<QStandardItemModel*> malst;  //保存资产负债表数据的模型的列表
    QStandardItemModel *mAsset, *mProfit;

    QSqlTableModel* extraModel;     //显示余额数值的模型
    QDataWidgetMapper* extraMapper; //映射余额值到显示部件    

};


#endif // SETUPBASEDIALOG_H
