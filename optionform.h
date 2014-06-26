#ifndef OPTIONFORM_H
#define OPTIONFORM_H

#include "commdatastruct.h"
#include "widgets/configpanels.h"

namespace Ui {
class PzTemplateOptionForm;
}


class PzTemplateOptionForm : public ConfigPanelBase
{
    Q_OBJECT

public:
    explicit PzTemplateOptionForm(QWidget *parent = 0);
    ~PzTemplateOptionForm();

    bool isDirty();
    bool save();
    QString panelName(){return tr("凭证模板配置");}

private:
    Ui::PzTemplateOptionForm *ui;
    PzTemplateParameter parameter;
};

///////////////////////////////
class TestPanel : public ConfigPanelBase
{
    Q_OBJECT
public:
    explicit TestPanel(QWidget* parent=0);
    ~TestPanel(){}

    bool isDirty(){return false;}
    bool save(){return false;}
    QString panelName(){return tr("测试页");}
};



#endif // OPTIONFORM_H
