#ifndef NABASEINFODIALOG_H
#define NABASEINFODIALOG_H

#include <QDialog>

struct SubSysNameItem;

namespace Ui {
class NABaseInfoDialog;
}

/**
 * @brief 新账户信息输入对话框类
 */
class NABaseInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NABaseInfoDialog(QWidget *parent = 0);
    ~NABaseInfoDialog();
private slots:
    void on_btnOk_clicked();
private:
    void init();

    Ui::NABaseInfoDialog *ui;
    QList<QString> usedCodes;               //已被使用的账户代码
    QList<SubSysNameItem*> supportSubSys;   //应用支持的科目系统
};

#endif // NABASEINFODIALOG_H
