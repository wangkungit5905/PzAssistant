#ifndef EXTERNALTOOLCONFIGFORM_H
#define EXTERNALTOOLCONFIGFORM_H

#include <QWidget>

namespace Ui {
class ExternalToolConfigForm;
}

class QListWidgetItem;
struct ExternalToolCfgItem;
class AppConfig;

class ExternalToolConfigForm : public QWidget
{
    Q_OBJECT

public:
    explicit ExternalToolConfigForm(QList<ExternalToolCfgItem*>* tools, QWidget *parent = 0);
    ~ExternalToolConfigForm();
    bool maybeSave();
    bool isUpdateMenuItem(){return isUpdateMenu;}

public slots:
    void save();

private slots:
    void currentToolChanged(QListWidgetItem * current, QListWidgetItem * previous);
    void toolConfigChanged();
    void ToolListContextMenuRequested(const QPoint &pos);
    void on_btnBrowser_clicked();

    void on_btnSave_clicked();

    void on_btnCancel_clicked();

    void on_actAdd_triggered();

    void on_actRemove_triggered();

private:
    void showTools();
    void inspectConfigChange(bool on=true);

    QList<ExternalToolCfgItem*> *tools,tools_copy,delTools;
    Ui::ExternalToolConfigForm *ui;
    AppConfig* appCon;
    bool isCloseAfterSave;//是否保存后关闭
    bool isUpdateMenu;      //是否需要更新启动菜单（当配置项的名称、命令行、参数列表改变，或新增、移除等）
};

#endif // EXTERNALTOOLCONFIGFORM_H
