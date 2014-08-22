#ifndef OPTIONFORM_H
#define OPTIONFORM_H

#include "ui_specsubcodecfgform.h"
#include "commdatastruct.h"
#include "config.h"
#include "widgets/configpanels.h"

namespace Ui {
class PzTemplateOptionForm;
}


/**
 * @brief 凭证模板配置面板类
 */
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

/////////////////////////////////////////////////////////////////
//对此类的是否要提供的必要性，还存在争议，暂且不作深入实现
//class SpecSubCodeCfgform : public ConfigPanelBase
//{
//    Q_OBJECT

//public:
//    explicit SpecSubCodeCfgform(QWidget *parent = 0);
//    ~SpecSubCodeCfgform();
//    bool isDirty();
//    bool save();
//    QString panelName(){return tr("特定科目配置");}

//private:
//    void init();
//    QWidget* crtTab(int subSys);

//    Ui::SpecSubCodeCfgform *ui;
//    AppConfig* appCon;
//    QHash<AppConfig::SpecSubCode, QString> names;

//};

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
