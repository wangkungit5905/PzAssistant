#ifndef SUBJECTCONFIGWINDOW_H
#define SUBJECTCONFIGWINDOW_H

//#include <QWidget>
#include <QDialog>
#include <QGroupBox>
#include <QRadioButton>
#include <QListView>
#include <QLineEdit>
#include <QDataWidgetMapper>
#include <QSqlTableModel>
#include <QLabel>
#include <QTextEdit>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QSignalMapper>
#include <QListWidget>
#include <QComboBox>
#include <QSqlRelationalTableModel>
#include <QSignalMapper>
#include <QCheckBox>

#include "ui_sndsubconfig.h"

//配置一级科目
class FstSubConWin : public QWidget
{
    Q_OBJECT

public:
    FstSubConWin(QWidget* parent = 0);

public slots:
    void save();
    void selSubClass(int cls);

private:
    QGroupBox groupBox;
    QRadioButton /**subClass1,*subClass2,*subClass3,*subClass4,*subClass5,*subClass6,**/subClassAll;
    QSignalMapper sigMapper;

    QHBoxLayout mainLayout;

    QLineEdit *edtName, *edtCode, *edtRem, *edtWeight;
    QTextEdit *details, *utils;
    QCheckBox *chkIsReqDet; //是否需要明细支持
    QCheckBox* chkIsView;   //是否显示（是否是在记账时要使用的科目）
    QPushButton *btnFirst, *btnNext, *btnPrev, *btnLast, *btnSave;
    QDataWidgetMapper mapper;
    QSqlTableModel model;

};

//正向显示（即指定一级科目下的二级科目）
class SndSubConWin : public QWidget
{
    Q_OBJECT
public:
    SndSubConWin(QWidget* parent = 0);

public slots:
    void subClsToggled(int witch);
    void fstListClicked(const QModelIndex &index);
    void sndListClicked(const QModelIndex& index);
    void dataChanged();
    void save();

private:
    QSqlTableModel *fstModel;      //一级科目的model
    QSqlRelationalTableModel* sndModel;  //二级科目的model
    QListView *fstList, *sndList;
    QLineEdit /**edtName, */*edtCode, /**edtRem*/;  //科目名，代码，助记符
    QSpinBox *edtStat; //科目使用频度统计值
    QPushButton *btnAdd, *btnDel, *btnMoveUp, *btnMoveDown, *btnSave;
    QSignalMapper* sigMapper;
    QDataWidgetMapper* mapper;

    bool isChanged;
    bool isRequired;
    int curRowIndex;  //当前选择的二级科目的索引

};

//反向显示或配置二级科目的窗体（配置指定二级科目名可以在多个一级科目中出现）
class SubInfoConWin : public QWidget
{
    Q_OBJECT

public:
    SubInfoConWin(QWidget* parent = 0);

public slots:
    void fstSubClsClicked(int witch);
    void fstListItemClicked(QListWidgetItem* item);
    void sndListItemClicked(const QModelIndex &index);
    void save();
    void add();
    void del();
    void dataChanged();



private:

    void setFstListCheckState(int fstId, Qt::CheckState checkState);

    QListView* sndList; //显示二级科目的列表
    QListWidget* fstList; //显示一级科目的列表
    QSqlRelationalTableModel* sndInfoModel; //指向二级科目信息表的model
    //QSqlTableModel* sndModel;     //执向二级科目映射表fsagent的model
    QSqlTableModel* sndModel;     //执向二级科目映射表fsagent的model
    QSqlTableModel* fstModel;     //指向一级科目表的Model

    QLineEdit *edtName, *edtLName, *edtRem; //科目名称、全名和助记符
    QComboBox* cmbSubCls;                   //科目所属类别
    QPushButton *btnAdd, *btnDel, *btnSave;
    QDataWidgetMapper* dataMapper;
    QSignalMapper* sigMapper; //用来映射一级科目类别的无线按钮组
    QMap<int, Qt::CheckState> checkStateMap; //这个是用来记录一级科目列表框的初始选中情况
                                 //每当二级科目列表框的当前选择项改变时，通过当前一级科目列表框
                                 //的实际选择情况和此map中保存的初始情况，以决定删除或增加记录
    //QModelIndex* curSndSubIndex; //当前选择的二级科目的模型索引
    bool isChanged;   //记录dataMapper映射部件的数据是否改变了
    bool isRequirSave; //数据是否需要保存
    //bool isAlways;     //当数据需要保存时，是否不再询问用户，总是默认要保存
    int curSndInfoId;  //当前选择的二级（信息）科目的ID值
    int curSndInfoListIndex; //二级科目列表的当前索引

};

//目前正在使用的二级科目配置窗口
class SndSubConForm : public QWidget
{
    Q_OBJECT

public:
    SndSubConForm(QWidget* parent = 0);

public slots:
    void fstSubClassChanged(int index);
    void sndSubClassChangee(int index);
    void fstSubclicked(const QModelIndex &index);
    void sndSubclicked(QListWidgetItem* item);
    void mapLstItemClicked(const QModelIndex &index);
    void btnAddToClicked();
    void btnRemoveToClicked();
    void btnAddClicked();
    void btnDelClicked();
    void btnSaveClicked();
    void edtSNameEditingFinished();

private:
    void refreshMapList();
    void refreshSndList();

    Ui::ConForm1 ui;
    QSqlQueryModel* fstModel, *mapModel;
    QSqlRelationalTableModel* sndModel;
    QDataWidgetMapper* mapper1; //映射FSAgent表的其他字段到对应部件中
    QDataWidgetMapper* mapper2; //映射二级科目表中的其他字段到对应显示部件中

    QHash<int, int> clsHash; //保存二级科目ID和科目类别的对应关系
    int fid, sid;  //当在一二级科目列表中选择一个项目时，用这两个变量保存该项目所属的
                   //一级和二级科目的ID值
    QSet<int> sndIdSet; //保存当前选择的一级科目下的二级科目id（FSAgent.sid）值集合
};


class SubjectConfigDialog : public QDialog
{
    Q_OBJECT

public:
    SubjectConfigDialog(QWidget* parent = 0);

private:
    FstSubConWin* fstTabPage;
    SubInfoConWin* secInfoTabPage;
    SndSubConWin* secTabPage;
    SndSubConForm* sndTabPage;
    QTabWidget* tab;

    QVBoxLayout main;
};



#endif // SUBJECTCONFIGWINDOW_H
