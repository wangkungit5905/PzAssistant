#include "configpanels.h"
#include "widgets.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCloseEvent>

ConfigPanels::ConfigPanels(QWidget *parent) : ConfigPanelBase(parent)
{
    reason = R_UNCERTAIN;
    setWindowTitle(tr("应用选项配置"));
    contentsWidget = new QListWidget(this);
    contentsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMaximumWidth(96);
    contentsWidget->setSpacing(12);

    pagesWidget = new QStackedWidget(this);
    connect(contentsWidget,SIGNAL(currentRowChanged(int)),pagesWidget,SLOT(setCurrentIndex(int)));

    QHBoxLayout* contentLayout = new QHBoxLayout;
    contentLayout->addWidget(contentsWidget);
    contentLayout->addWidget(pagesWidget);
    QPushButton* btnOk = new QPushButton(tr("确定"),this);
    connect(btnOk,SIGNAL(clicked()),this,SLOT(btnOkClicked()));
    QPushButton* btnCancel = new QPushButton(tr("取消"),this);
    connect(btnCancel,SIGNAL(clicked()),this,SLOT(btnCancelClicked()));
    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(btnOk);
    buttonsLayout->addWidget(btnCancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(contentLayout,1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
}

void ConfigPanels::addPanel(ConfigPanelBase *panel, QIcon icon)
{
    QListWidgetItem* item = new QListWidgetItem(contentsWidget);
    item->setIcon(icon);
    item->setText(panel->panelName());
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    pagesWidget->addWidget(panel);
}


void ConfigPanels::closeEvent(QCloseEvent *event)
{
    if((reason == R_UNCERTAIN) && isDirty()){
        if(QMessageBox::warning(this,"",tr("保存已做出的更改吗？"),
                                QMessageBox::Yes|QMessageBox::No,
                                QMessageBox::Yes)){
            save();
        }
    }
    else if(reason == R_OK && isDirty())
        save();
    event->accept();
}

//void ConfigPanels::pageChanged(int index)
//{
//    pagesWidget->setCurrentIndex(index);
//}

void ConfigPanels::btnOkClicked()
{
    reason = R_OK;
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}

void ConfigPanels::btnCancelClicked()
{
    reason = R_CANCEL;
    MyMdiSubWindow* w = static_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}

/**
 * @brief 轮询所有的配置面板是否配置数据被修改而未保存
 * @return
 */
bool ConfigPanels::isDirty()
{
    isChangedIndexes.clear();
    for(int i = 0; i < pagesWidget->count(); ++i){
        ConfigPanelBase* panel = qobject_cast<ConfigPanelBase*>(pagesWidget->widget(i));
        if(panel && panel->isDirty())
            isChangedIndexes<<i;
    }
    return !isChangedIndexes.isEmpty();
}

bool ConfigPanels::save()
{
    if(isChangedIndexes.isEmpty())
        return true;
    bool ok = true;
    foreach(int i, isChangedIndexes){
        ConfigPanelBase* panel = qobject_cast<ConfigPanelBase*>(pagesWidget->widget(i));
        if(panel && !panel->save()){
            ok = false;
            QMessageBox::critical(this,tr("保存出错"),tr("在保存“%1”时发生错误！").arg(panel->panelName()));
        }
    }
    return ok;
}
