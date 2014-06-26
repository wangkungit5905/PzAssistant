#ifndef CONFIGPANELS_H
#define CONFIGPANELS_H

#include <QWidget>

class QListWidget;
class QStackedWidget;

/**
 * @brief 配置面板基类，所有要加入到配置面板集合类的面板类都应该继承自此类
 */
class ConfigPanelBase : public QWidget{
    Q_OBJECT
public:
    ConfigPanelBase(QWidget* parent=0):QWidget(parent){}
    virtual bool isDirty()=0;
    virtual bool save()=0;
    virtual QString panelName(){return "";}
};

class ConfigPanels : public ConfigPanelBase
{
    Q_OBJECT

    enum closeReason{
        R_UNCERTAIN = 0,    //通过窗口关闭小安纽
        R_OK        = 1,
        R_CANCEL    = 2
    };

public:
    explicit ConfigPanels(QWidget *parent = 0);
    void addPanel(ConfigPanelBase* panel, QIcon icon);

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    //void pageChanged(int index);
    void btnOkClicked();
    void btnCancelClicked();

private:
    bool isDirty();
    bool save();

    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;
    closeReason reason;
    QList<int> isChangedIndexes; //发生了变更的面板索引
};

#endif // CONFIGPANELS_H
