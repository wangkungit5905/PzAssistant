#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSqlTableModel>
#include <QListWidgetItem>
#include <QStringListModel>
#include <QSqlRelationalDelegate>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QSqlRecord>

#include "tables.h"
#include "common.h"
#include "subjectConfigDialog.h"
#include "config.h"
#include "global.h"

FstSubConWin::FstSubConWin(QWidget* parent) : QWidget(parent)
{

    model.setTable(tbl_fsub);
    mapper.setModel(&model);

    groupBox.setTitle(tr("一级科目类别"));
    QVBoxLayout groupLayout;
    subClassAll.setText(tr("全部"));
    connect(&subClassAll, SIGNAL(clicked()), &sigMapper, SLOT(map()));
    sigMapper.setMapping(&subClassAll, 0);
    subClassAll.setChecked(true);
    groupLayout.addWidget(&subClassAll);



    //创建科目类别无线按钮
    QSqlQuery q;
    QString s = "select code,name from FstSubClasses";
    if(q.exec(s)){
        while(q.next()){
            QString cname = q.value(1).toString();
            int code = q.value(0).toInt();
            QRadioButton* chk = new QRadioButton(cname);
            connect(chk, SIGNAL(clicked()), &sigMapper, SLOT(map()));
            sigMapper.setMapping(chk, code);
            groupLayout.addWidget(chk);
        }
    }
    connect(&sigMapper, SIGNAL(mapped(int)), this, SLOT(selSubClass(int)));


    groupBox.setLayout(&groupLayout);

    QLabel* lblName = new QLabel(tr("科目名称"));
    edtName = new QLineEdit;
    mapper.addMapping(edtName, FSUB_SUBNAME);
    QLabel* lblCode = new QLabel(tr("科目代码"));
    edtCode = new QLineEdit;
    mapper.addMapping(edtCode, FSUB_SUBCODE);
    QLabel* lblRem = new QLabel(tr("科目助记符"));
    edtRem = new QLineEdit;
    mapper.addMapping(edtRem, FSUB_REMCODE);
    QLabel* lblWeight = new QLabel(tr("使用权重值"));
    edtWeight = new QLineEdit;
    mapper.addMapping(edtWeight, FSUB_WEIGHT);
    btnFirst = new QPushButton(tr("第一个"));
    connect(btnFirst, SIGNAL(clicked()), &mapper, SLOT(toFirst()));
    btnNext =  new QPushButton(tr("下一个"));
    connect(btnNext, SIGNAL(clicked()), &mapper, SLOT(toNext()));
    btnPrev = new QPushButton(tr("上一个"));
    connect(btnPrev, SIGNAL(clicked()), &mapper, SLOT(toPrevious()));
    btnLast = new QPushButton(tr("最后一个"));
    connect(btnLast, SIGNAL(clicked()), &mapper, SLOT(toLast()));
    btnSave = new QPushButton(tr("保存"));
    connect(btnSave, SIGNAL(clicked()), this, SLOT(save()));
    //chkIsReqDet = new CustomCheckBox(tr("是否需要明细支持"));
    chkIsView = new QCheckBox(tr("是否显示"));
    QGridLayout* gridLayout = new QGridLayout;
    gridLayout->addWidget(lblName,0,0);
    gridLayout->addWidget(edtName, 0, 1);
    gridLayout->addWidget(btnFirst, 0, 2);
    gridLayout->addWidget(lblCode, 1, 0);
    gridLayout->addWidget(edtCode, 1, 1);
    gridLayout->addWidget(btnPrev, 1, 2);
    gridLayout->addWidget(lblRem, 2, 0);
    gridLayout->addWidget(edtRem, 2, 1);
    gridLayout->addWidget(btnNext, 2, 2);
    gridLayout->addWidget(lblWeight, 3, 0);
    gridLayout->addWidget(edtWeight, 3,1);
    gridLayout->addWidget(btnLast, 3,2);

    QVBoxLayout* detailLayout = new QVBoxLayout;    
    QLabel* lblDetail = new QLabel(tr("科目简要说明"));
    //chkIsReqDet = new QCheckBox(tr("是否需要明细支持"));
    QHBoxLayout* l = new QHBoxLayout;
    l->addWidget(lblDetail);
    l->addWidget(chkIsView);
    l->addWidget(chkIsReqDet);
    l->addWidget(btnSave);
    details = new QTextEdit;
    //mapper.addMapping(details, FSTSUB_DESC);
    mapper.addMapping(chkIsReqDet, FSUB_ISUSEWB, "checkstate");
    mapper.addMapping(chkIsView, FSUB_ISVIEW, "checkstate");
    QLabel* lblUtils = new QLabel(tr("使用举例"));
    utils = new QTextEdit;
    //mapper.addMapping(utils, FSTSUB_UTILS);
    detailLayout->addLayout(l);
    detailLayout->addWidget(details);
    detailLayout->addWidget(lblUtils);
    detailLayout->addWidget(utils);

    QVBoxLayout* rLayout = new QVBoxLayout;
    rLayout->addLayout(gridLayout);
    rLayout->addLayout(detailLayout);

    mainLayout.addWidget(&groupBox,0,Qt::AlignLeft);
    mainLayout.addLayout(rLayout);
    setLayout(&mainLayout);
    model.select();
    mapper.toFirst();
}

void FstSubConWin::save()
{
    int index = mapper.currentIndex();
    mapper.submit();
    mapper.setCurrentIndex(index);
}

//当用选择一个一级科目的类别时
void FstSubConWin::selSubClass(int cls)
{
    if(cls == 0)
        model.setFilter("");
    else
        model.setFilter(QString("belongTo = %1").arg(cls));
    model.select();
    mapper.toFirst();
}

///////////////////////////////////////////////////////////////////////////////////

SndSubConWin::SndSubConWin(QWidget* parent) : QWidget(parent)
{
    isChanged = false;
    isRequired = false;
    curRowIndex = -1;

    fstModel = new QSqlTableModel;
    fstModel->setTable(tbl_fsub);
    fstModel->select();

    sndModel = new QSqlRelationalTableModel;
    sndModel->setTable(tbl_ssub);
    sndModel->setRelation(SSUB_NID, QSqlRelation(tbl_nameItem, "id", fld_ni_name));

    //sndModel->select();

    mapper = new QDataWidgetMapper;
    mapper->setModel(sndModel);

    QRadioButton* subClassAll = new QRadioButton(tr("全部"));
    subClassAll->setChecked(true);
    QRadioButton* subClass1 = new QRadioButton(tr("资产类"));
    QRadioButton* subClass2 = new QRadioButton(tr("负债类"));
    QRadioButton* subClass3 = new QRadioButton(tr("共同类"));
    QRadioButton* subClass4 = new QRadioButton(tr("所有者权益类"));
    QRadioButton* subClass5 = new QRadioButton(tr("成本类"));
    QRadioButton* subClass6 = new QRadioButton(tr("损益类"));


    QVBoxLayout* subClsLayout = new QVBoxLayout;
    subClsLayout->addWidget(subClassAll);
    subClsLayout->addWidget(subClass1);
    subClsLayout->addWidget(subClass2);
    subClsLayout->addWidget(subClass3);
    subClsLayout->addWidget(subClass4);
    subClsLayout->addWidget(subClass5);
    subClsLayout->addWidget(subClass6);
    subClsLayout->addWidget(subClassAll);
    QGroupBox* group = new QGroupBox(tr("一级科目类别"));
    group->setLayout(subClsLayout);

    sigMapper = new QSignalMapper(this);
    connect(subClassAll, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClassAll, 0);
    connect(subClass1, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass1, 1);
    connect(subClass2, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass2, 2);
    connect(subClass3, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass3, 3);
    connect(subClass4, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass4, 4);
    connect(subClass5, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass5, 5);
    connect(subClass6, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass6, 6);

    connect(sigMapper, SIGNAL(mapped(int)), this, SLOT(subClsToggled(int)));

    fstList = new QListView;
    fstList->setModel(fstModel);
    fstList->setModelColumn(FSUB_SUBNAME);
    connect(fstList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(fstListClicked(QModelIndex)));

    sndList = new QListView;
    sndList->setModel(sndModel);
    sndList->setModelColumn(SSUB_NID);
//    sndList->setModel(sndModel->relationModel(FSAGENT_SID));
//    sndList->setModelColumn(SNDSUB_SUBNAME);
    connect(sndList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(sndListClicked(QModelIndex)));
    sndList->setItemDelegate(new QSqlRelationalDelegate(sndList));

//    QLabel* lblName = new QLabel(tr("科目名称"));
//    edtName = new QLineEdit;
//    mapper->addMapping(edtName, SNDSUB_SUBNAME);
    QLabel* lblCode = new QLabel(tr("科目代码"));
    edtCode = new QLineEdit;
    connect(edtCode, SIGNAL(textEdited(QString)), this, SLOT(dataChanged()));
    mapper->addMapping(edtCode, SSUB_SUBCODE);
//    QLabel* lblRem = new QLabel(tr("科目助记符"));
//    edtRem = new QLineEdit;
//    mapper->addMapping(edtRem, SNDSUB_REMCODE);
    QLabel* lblStat = new QLabel(tr("使用频度统计值"));
    edtStat = new QSpinBox;
    edtStat->setValue(0);
    connect(edtStat, SIGNAL(valueChanged(int)), this, SLOT(dataChanged()));
    mapper->addMapping(edtStat, SSUB_WEIGHT);

    btnAdd = new QPushButton(tr("新增"));
    btnDel = new QPushButton(tr("删除"));
    btnMoveUp = new QPushButton(tr("上移"));
    btnMoveDown = new QPushButton(tr("下移"));
    btnSave = new QPushButton(tr("保存"));
    connect(btnSave, SIGNAL(clicked()), this, SLOT(save()));

    QGridLayout* grid = new QGridLayout;
    grid->addWidget(lblCode, 0, 0);
    grid->addWidget(edtCode, 0, 1);
    grid->addWidget(btnAdd, 0, 2);
    grid->addWidget(lblStat, 1, 0);
    grid->addWidget(edtStat, 1,1);
    grid->addWidget(btnDel, 1, 2);
    grid->addWidget(btnMoveUp, 2, 2);
    grid->addWidget(btnSave, 3, 0);
    grid->addWidget(btnMoveDown, 3, 2);

    QHBoxLayout* main = new QHBoxLayout;
    main->addWidget(group);
    main->addWidget(fstList);
    main->addWidget(sndList);
    main->addLayout(grid);
    setLayout(main);



}



void SndSubConWin::subClsToggled(int witch)
{
    if(witch != 0){
        fstModel->setFilter(QString("belongTo=%1").arg(witch));
    }
    else{
        fstModel->setFilter("");
    }
    fstModel->select();
}


void SndSubConWin::fstListClicked(const QModelIndex &index)
{
    int fstSubId = fstModel->data(fstModel->index(index.row(), 0)).toInt();
    sndModel->setFilter(QString("fid=%1").arg(fstSubId));
    sndModel->select();
//    sndList->setModel(sndModel->relationModel(FSAGENT_SID));
//    sndList->setModelColumn(SNDSUB_SUBNAME);
    //sndList->setItemDelegate(new QSqlRelationalDelegate(this));
    int row = 10;
    row = sndModel->rowCount();
    int i = 0;

}

void SndSubConWin::sndListClicked(const QModelIndex& index)
{
    int id = sndModel->data(sndModel->index(index.row(), 0)).toInt();
    int fstId = sndModel->data(sndModel->index(index.row(), SSUB_FID)).toInt();
    QString sndId = sndModel->data(sndModel->index(index.row(), SSUB_NID)).toString();

    //第一次选择
    if(curRowIndex == -1){
        mapper->setCurrentIndex(index.row());
        curRowIndex = index.row();
        isChanged = false;  //记录的切换可能会引起edtStat（QSpinBox）值的改变，但这个改变不是用户的输入行为引起的
        return;
    }

    if(isChanged){
        if(isRequired || (QMessageBox::critical(this,
                                                    tr("确认消息"),
                                                    tr("数据已经改变，需要保存吗？")) == QMessageBox::Ok )){
                //save();
                sndModel->submit();
                isRequired = true;
        }
    }
    mapper->setCurrentIndex(index.row());
    curRowIndex = index.row();
    isChanged = false;
}

void SndSubConWin::dataChanged()
{
    isChanged = true;
}

void SndSubConWin::save()
{
    int index = mapper->currentIndex();
    sndModel->submit();
    mapper->setCurrentIndex(index);
    isChanged = false;
}


///////////////////////////////////////////////////////////////////////////////////

SubInfoConWin::SubInfoConWin(QWidget* parent) : QWidget(parent)
{
    isChanged = false;
    isRequirSave = false;
    curSndInfoId = 0; //这里的0代表不指向任何有效的二级（信息）科目
    curSndInfoListIndex = -1; //未选择任意项目

    //创建数据模型
    sndInfoModel = new QSqlRelationalTableModel;
    sndInfoModel->setTable("SecSubjects");
    sndInfoModel->setRelation(NI_CALSS, QSqlRelation("SndSubClass", "id", "name"));
    sndInfoModel->select();

    sndModel = new QSqlTableModel;
    sndModel->setTable(tbl_ssub);
    //sndModel->setRelation(SUNSUB_CALSS, QSqlRelation("SndSubClass", "id", "name"));
    //secModel->select();

    fstModel = new QSqlTableModel;
    fstModel->setTable(tbl_fsub);
    fstModel->select();

    //创建二级科目类别选择框（其中的类别来自配置信息）
    QGroupBox* box1 = new QGroupBox(tr("二级科目类别"));
    QRadioButton* btnSecAll = new QRadioButton(tr("全部"));
    QRadioButton* btnBusiClient = new QRadioButton(tr("业务客户"));
    QRadioButton* btnBankClient = new QRadioButton(tr("银行客户"));
    QRadioButton* btnOther = new QRadioButton(tr("其他"));
    QVBoxLayout* groupLayout = new QVBoxLayout;
    groupLayout->addWidget(btnSecAll);
    groupLayout->addWidget(btnBusiClient);
    groupLayout->addWidget(btnBankClient);
    groupLayout->addWidget(btnOther);
    box1->setLayout(groupLayout);

    //创建二级科目信息列表框（显示的是二级科目的信息）
    sndList = new QListView;
    sndList->setModel(sndInfoModel);
    sndList->setModelColumn(NI_NAME);
    connect(sndList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(sndListItemClicked(QModelIndex)));

    dataMapper = new QDataWidgetMapper(this);
    dataMapper->setModel(sndInfoModel);
    dataMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
    QLabel *lblName = new QLabel(tr("科目名称"));

    edtName = new QLineEdit;
    connect(edtName, SIGNAL(textEdited(QString)), this, SLOT(dataChanged()));
    dataMapper->addMapping(edtName, NI_NAME);

    QLabel* lblLName = new QLabel(tr("科目全称"));
    edtLName = new QLineEdit;
    connect(edtLName, SIGNAL(textEdited(QString)), this, SLOT(dataChanged()));
    dataMapper->addMapping(edtLName, NI_LNAME);

    QLabel* lblRem = new QLabel(tr("科目助记符"));
    edtRem = new QLineEdit;
    connect(edtRem, SIGNAL(textEdited(QString)), this, SLOT(dataChanged()));
    dataMapper->addMapping(edtRem, NI_REMCODE);

    QLabel* lblSubCls = new QLabel(tr("科目所属类别"));
    cmbSubCls = new QComboBox;
    //捕捉这个信号，当用程序代码改变当前索引时，也会触发，因此不合适，应当从底层模型
    //connect(cmbSubCls, SIGNAL(currentIndexChanged(int)), this, SLOT(dataChanged()));
    cmbSubCls->setModel(sndInfoModel->relationModel(NI_CALSS));
    cmbSubCls->setModelColumn(NICLASS_NAME);
    dataMapper->addMapping(cmbSubCls, NI_CALSS);
    dataMapper->setItemDelegate(new QSqlRelationalDelegate(this));
    dataMapper->toFirst();

    //创建新增、删除二级科目的按钮
    btnAdd = new QPushButton(tr("新增"));
    connect(btnAdd, SIGNAL(clicked()), this, SLOT(add()));
    btnDel = new QPushButton(tr("删除"));
    btnSave = new QPushButton(tr("保存"));
    connect(btnSave, SIGNAL(clicked()), this, SLOT(save()));
//    QVBoxLayout* btnLayout = new QVBoxLayout;
//    btnLayout->addWidget(btnAdd);
//    btnLayout->addWidget(btnDel);
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(lblName, 0, 0);
    grid->addWidget(edtName, 0, 1);
    grid->addWidget(lblRem, 0, 2);
    grid->addWidget(edtRem, 0, 3);
    grid->addWidget(lblLName, 1, 0);
    grid->addWidget(edtLName, 1, 1, 1, -1);
    grid->addWidget(lblSubCls, 2, 0);
    grid->addWidget(cmbSubCls, 2, 1);
    grid->addWidget(btnAdd, 3, 0);
    grid->addWidget(btnDel, 3, 1);
    grid->addWidget(btnSave, 3, 2);
    //创建显示一级科目的列表框
    fstList = new QListWidget;
    //
    for(int i = 0; i < fstModel->rowCount(); ++i){
        QListWidgetItem* item = new QListWidgetItem(fstList);
        int fstSubId = fstModel->data(fstModel->index(i, 0)).toInt();
        item->setText(fstModel->data(fstModel->index(i, FSUB_SUBNAME)).toString());
        item->setData(Qt::UserRole, fstSubId);
        item->setCheckState(Qt::Unchecked);
        checkStateMap[fstSubId] = Qt::Unchecked; //
    }
    connect(fstList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(fstListItemClicked(QListWidgetItem*)));
    //fstList->setModel(fstModel);
    //fstList->setModelColumn(FSTSUB_SUBNAME);

    //创建一级科目的类别选择框
    QGroupBox* box2 = new QGroupBox(tr("一级科目类别"));
    QRadioButton* subClassAll = new QRadioButton(tr("全部"));
    subClassAll->setChecked(true);
    QRadioButton* subClass1 = new QRadioButton(tr("资产类"));
    QRadioButton* subClass2 = new QRadioButton(tr("负债类"));
    QRadioButton* subClass3 = new QRadioButton(tr("共同类"));
    QRadioButton* subClass4 = new QRadioButton(tr("所有者权益类"));
    QRadioButton* subClass5 = new QRadioButton(tr("成本类"));
    QRadioButton* subClass6 = new QRadioButton(tr("损益类"));
    QVBoxLayout* groupLayout2 = new QVBoxLayout;
    groupLayout2->addWidget(subClassAll);
    groupLayout2->addWidget(subClass1);
    groupLayout2->addWidget(subClass2);
    groupLayout2->addWidget(subClass3);
    groupLayout2->addWidget(subClass4);
    groupLayout2->addWidget(subClass5);
    groupLayout2->addWidget(subClass6);
    box2->setLayout(groupLayout2);

    sigMapper = new QSignalMapper(this);

    connect(subClassAll, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClassAll, 0);
    connect(subClass1, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass1, 1);
    connect(subClass2, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass2, 2);
    connect(subClass3, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass3, 3);
    connect(subClass4, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass4, 4);
    connect(subClass5, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass5, 5);
    connect(subClass6, SIGNAL(clicked()), sigMapper, SLOT(map()));
    sigMapper->setMapping(subClass6, 6);

    connect(sigMapper, SIGNAL(mapped(int)), this, SLOT(fstSubClsClicked(int)));

    QHBoxLayout* main = new QHBoxLayout;
    main->addWidget(box1);
    main->addWidget(sndList);
    main->addLayout(grid);
    main->addWidget(fstList);
    main->addWidget(box2);

    setLayout(main);
}

void SubInfoConWin::fstSubClsClicked(int witch)
{
    if(witch != 0){
        fstModel->setFilter(QString("belongTo=%1").arg(witch));
    }
    else{
        fstModel->setFilter("");
    }
    fstModel->select();
}

//处理一级科目列表框的项目单击事件
void SubInfoConWin::fstListItemClicked(QListWidgetItem * item)
{
    int checkState = item->checkState();
    int i = 0;
}

//设置指定Id的一级科目的选中状态
void SubInfoConWin::setFstListCheckState(int fstId, Qt::CheckState checkState)
{
    QListWidgetItem* item;
    int i,rows;
    rows = fstList->count();
    i = 0;
    while((i < rows)){
        item = fstList->item(i);
        if(item->data(Qt::UserRole).toInt() == fstId){
            item->setCheckState(checkState);
            break;
        }
        ++i;
    }
}

//当选择一个二级科目时，根据当前选择的二级科目所属的一级科目来更新一级科目列表框中的一级科目的选中状态
void SubInfoConWin::sndListItemClicked(const QModelIndex &index)
{
    bool isRequirSave = false;  //是否需要保存一级科目列表框的选择状态，此值从对话框中读取
    //选择的二级科目没有改变或是第一次选择
    if((curSndInfoListIndex == index.row()) || (curSndInfoListIndex == -1)){
        curSndInfoListIndex =  index.row();
        return;
    }

    //如果映射部件的数据发生了改变，则提示用户是否提交改变的数据到模型
    if(isChanged){
        //如果用户先前已经同意过要保存数据，或者再次请求用户确认保存
        if(isRequirSave || (QMessageBox::question(this, tr("确认消息"),
                                                  tr("数据已经改变，是否需要保存？")) == QMessageBox::Ok)){
            dataMapper->submit();
            isChanged = false;
            isRequirSave = true;
        }
    }

    //1、首先要根据一级科目列表框的选取情况视情决定回写到模型数据
    int p = 0; //开始比对的位置
    int c = fstList->count();
    //定位到第一个不一致的项
    while(p < c){
        QListWidgetItem* item = fstList->item(p);
        int fstId = item->data(Qt::UserRole).toInt();
        if((checkStateMap[fstId] != item->checkState()) && !isRequirSave){
            if(QMessageBox::critical(this, tr("请求确认"),
                                  tr("数据发生改变，需要保存吗？")) == QMessageBox::Ok){
                isRequirSave = true;
                break;
            }
        }
        ++p;
    }
    //如果发现有不一致项目并需要保存
    if((p < c) && isRequirSave){
        for(int i = p; i < c; ++i){
            QListWidgetItem* item = fstList->item(i);
            int fstId = item->data(Qt::UserRole).toInt();
            if(checkStateMap[fstId] != item->checkState()){
                //没有选中，则要删除fsagnet表中的对应条目
                //获取前一次选择的二级科目的ID
                int sndId = sndInfoModel->data(sndInfoModel->index(curSndInfoListIndex,
                                                                   0)).toInt();
                if(item->checkState() == Qt::Unchecked){
                    int r = 0;
                    int rows = sndModel->rowCount();
                    while(r < rows){
                        if((sndId = sndModel->data(sndModel->index(r, SSUB_NID)).toInt()) &&
                           (fstId == sndModel->data(sndModel->index(i, SSUB_FID)).toInt())){
                            sndModel->removeRow(r);
                            sndModel->submit();
                            break;
                        }
                    }
                }
                //选中，则要在fsagent表中新增对应条目
                else{
                    int row = sndModel->rowCount();
                    sndModel->insertRow(row);
                    sndModel->setData(sndModel->index(row, SSUB_NID), sndId);
                    sndModel->setData(sndModel->index(row, SSUB_FID), fstId);
                    sndModel->submit();
                }
            }
        }
        //isRequirSave = false;
    }


    //2、设置dataMapper的当前索引与二级科目列表框中的索引对应
    curSndInfoListIndex =  index.row();
    dataMapper->setCurrentIndex(index.row());
    curSndInfoId = sndInfoModel->data(sndInfoModel->index(index.row(), 0)).toInt();
//    int sndSubInfoId = sndInfoModel->data(sndInfoModel->index(index.row(),
//                                                              SNDSUB_ID)).toInt();
    //3、设置新选择的二级科目所属的一级科目的情况更新一级科目列表框中的选中情况
    //在fsagent表中查找具有此sid的所有条目，并根据这些条目所属的一级科目，
    //在一级科目列表中更新选择状态
    sndModel->setFilter(QString("sid=%1").arg(curSndInfoId));
    sndModel->select();    

    //清除一级科目列表框和checkStateMap的选中状态
    for(int i = 0; i < fstModel->rowCount(); ++i)
        fstList->item(i)->setCheckState(Qt::Unchecked);
    QMapIterator<int, Qt::CheckState> i(checkStateMap);
    while(i.hasNext()){
        i.next();
        checkStateMap[i.key()] = Qt::Unchecked;
    }

    //将新选择的二级科目的状态更新到一级科目列表框和checkStateMap中
    int rows;
    rows = sndModel->rowCount();
    if(rows > 0){
        for(int i = 0; i < rows; ++i){
            int fstId = sndModel->data(sndModel->index(i, SSUB_FID)).toInt();
            setFstListCheckState(fstId, Qt::Checked);
            //4、更新checkStateMap的选中状态与一级科目列表框的选中情况相对应
            checkStateMap[fstId] = Qt::Checked;
        }
    }

}

void SubInfoConWin::add()
{
    int row = sndInfoModel->rowCount();
    sndInfoModel->insertRow(row);
    sndInfoModel->setData(sndInfoModel->index(row, NI_CALSS), 1);
    sndInfoModel->setData(sndInfoModel->index(row, NI_NAME), tr("科目名"));
    sndInfoModel->submit();
    dataMapper->setCurrentIndex(row);
}

void SubInfoConWin::del()
{

}

void SubInfoConWin::save()
{
    int index = dataMapper->currentIndex();
    dataMapper->submit();
    dataMapper->setCurrentIndex(index);

}

//映射部件的数据发生改变时，要将数据提交到模型
void SubInfoConWin::dataChanged()
{
    //dataMapper->submit();
    isChanged = true;
}

//////////////////////////////////////////////////////////////////////////////////

SubjectConfigDialog::SubjectConfigDialog(QWidget* parent) : QDialog(parent)
{
    tab = new QTabWidget;

    fstTabPage = new FstSubConWin;
    tab->addTab(fstTabPage, tr("一级科目"));
    sndTabPage = new SndSubConForm;
    tab->addTab(sndTabPage, tr("二级科目"));
    main.addWidget(tab);
    resize(900,500);
    setLayout(&main);
}

////////////////////////////////////////////////////////////////////////////

SndSubConForm::SndSubConForm(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);
    QSqlQuery q;

    //装载一级科目类别数据
    q.exec("select code, name from FstSubClasses");
    while(q.next()){
        QString name = q.value(1).toString();
        int code = q.value(0).toInt();
        ui.cmbFst->addItem(name, code);
    }
//    PSetting setting;
//    QMap<int, QString> map = setting.readFstSubClass();
//    ui.cmbFst->addItem(tr("全部"), 0);
//    QMapIterator<int, QString> i(map);
//    while (i.hasNext()) {
//        i.next();
//        ui.cmbFst->addItem(i.value(), i.key());
//    }

    fstModel = new QSqlQueryModel;
    QString s = QString("select id, %1 from %2 order by %3")
            .arg(fld_fsub_name).arg(tbl_fsub).arg(fld_fsub_subcode);
    fstModel->setQuery(s);



    ui.lstFst->setModel(fstModel);
    ui.lstFst->setModelColumn(1);    

    //装载二级科目类别
    ui.cmbSnd->addItem(tr("全部"), 0);
    if(q.exec(QString("select * from %1").arg(tbl_nameItemCls))){
        while(q.next()){
            ui.cmbSnd->addItem(q.value(NICLASS_NAME).toString(),
                               q.value(NICLASS_CODE));
        }
    }

    //装载二级科目
    sndModel = new QSqlRelationalTableModel;
    sndModel->setTable(tbl_nameItem);
    sndModel->setRelation(NI_CALSS, QSqlRelation(tbl_nameItemCls, fld_nic_clscode, fld_nic_name));
    sndModel->select();

    mapper2 = new QDataWidgetMapper;
    mapper2->setModel(sndModel);
    mapper2->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper2->addMapping(ui.edtSubSName, NI_NAME);
    mapper2->addMapping(ui.edtSubLName, NI_LNAME);
    mapper2->addMapping(ui.edtRemark, NI_REMCODE);

    mapper2->addMapping(ui.cmbClass, NI_CALSS);
    ui.cmbClass->setModel(sndModel->relationModel(NI_CALSS));
    ui.cmbClass->setModelColumn(NICLASS_NAME);
    mapper2->setItemDelegate(new QSqlRelationalDelegate());

    //mapper2->toFirst();
//    QSqlRecord recs = sndModel->query().record();
//    QString fls;
//    for(int i = 0; i < recs.count(); ++i){
//        fls.append(recs.fieldName(i));
//        fls.append(", ");
//    }

//    for(int i = 0; i < sndModel->rowCount(); ++i){
//        QString sname = sndModel->data(sndModel->index(i, SNDSUB_SUBNAME)).toString();
//        int sid = sndModel->data(sndModel->index(i, SNDSUB_ID)).toInt();
//        int clsId = sndModel->data(sndModel->index(i, SNDSUB_CALSS)).toInt();
//        //  int clsId = sndModel->relationModel(SNDSUB_CALSS)->data(
//        //          sndModel->relationModel(SNDSUB_CALSS)->index(i,0)).toInt();
//        //QString cname = sndModel->data(sndModel->index(i, SNDSUB_CALSS)).toString();
//        QListWidgetItem* item = new QListWidgetItem(sname);
//        item->setData(Qt::UserRole, sid);
//        ui.lstSnd->addItem(item);
//        clsHash[sid] = clsId;
//    }

    //装载二级科目(注意：如果二级科目的类别Id值在SndSubClass表中不存在，则此方式
    //装载的二级科目与sndModel的数据会不一致)
    //QSqlQuery q;
    s = QString("select id, %1, %2 from %3")
            .arg(fld_ni_name).arg(fld_ni_class).arg(tbl_nameItem);
    bool r = q.exec(s);
    if(r){
        while(q.next()){
            int sid = q.value(0).toInt();
            QString sname = q.value(1).toString();
            int clsId = q.value(2).toInt();
            QListWidgetItem* item = new QListWidgetItem(sname);
            item->setData(Qt::UserRole, sid);
            ui.lstSnd->addItem(item);
            clsHash[sid] = clsId;
        }
    }

    mapModel = new QSqlQueryModel;
//    ui.lstOwnership->setModel(mapModel);
//    ui.lstOwnership->setModelColumn(5);
    mapper1 = new QDataWidgetMapper;
    mapper1->setModel(mapModel);
    mapper1->addMapping(ui.edtCode, 2);
    mapper1->addMapping(ui.edtWeight, 3);
    //mapper1->addMapping(ui.chkByMt, 5, "checkstate");
    //mapper1->addMapping(ui.chkByMt, FSAGENT_ISDETBYMT);


    ui.btnAddTo->setEnabled(false);
    ui.btnRomveTo->setEnabled(false);

    connect(ui.cmbFst, SIGNAL(currentIndexChanged(int)), this, SLOT(fstSubClassChanged(int)));
    connect(ui.lstSnd, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(sndSubclicked(QListWidgetItem*)));
    connect(ui.cmbSnd, SIGNAL(currentIndexChanged(int)), this, SLOT(sndSubClassChangee(int)));
    connect(ui.lstFst, SIGNAL(clicked(QModelIndex)), this, SLOT(fstSubclicked(QModelIndex)));
    connect(ui.lstOwnership, SIGNAL(clicked(QModelIndex)), this, SLOT(mapLstItemClicked(QModelIndex)));
    connect(ui.btnAddTo, SIGNAL(clicked()), this, SLOT(btnAddToClicked()));
    connect(ui.btnRomveTo, SIGNAL(clicked()), this, SLOT(btnRemoveToClicked()));
    connect(ui.btnAdd, SIGNAL(clicked()), this, SLOT(btnAddClicked()));
    connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(btnSaveClicked()));
    connect(ui.btnDel, SIGNAL(clicked()), this, SLOT(btnDelClicked()));
    connect(ui.edtSubSName, SIGNAL(editingFinished()), this, SLOT(edtSNameEditingFinished()));

}

//选择一个一级科目类别，在类别中只显示属于该类别的一级科目
void SndSubConForm::fstSubClassChanged(int index)
{
    QString s;
    int cls = ui.cmbFst->itemData(index).toInt();
    if(cls == 0)
        s = QString("select id, %2 from %1").arg(fld_fsub_name).arg(tbl_fsub);
    else
        s = QString("select id, %1 from %2 where %3 = %4")
                .arg(fld_fsub_name).arg(tbl_fsub).arg(fld_fsub_class).arg(cls);
    fstModel->setQuery(s);
}

//选择一个二级科目的类别，在列表中只显示属于该类别的科目
void SndSubConForm::sndSubClassChangee(int index)
{
//    if(index == 0){ //选全部二级科目
//        for(int i = 0; i < ui.lstSnd->count(); ++i)
//            ui.lstSnd->item(i)->setHidden(false);
//    }
//    else{
//        int cls = ui.cmbSnd->itemData(index).toInt();
//        for(int i = 0; i < ui.lstSnd->count(); ++i){
//            int sid = ui.lstSnd->item(i)->data(Qt::UserRole).toInt();
//            if(cls == clsHash.value(sid))
//                ui.lstSnd->item(i)->setHidden(false);
//            else
//                ui.lstSnd->item(i)->setHidden(true);
//        }
//    }
    refreshSndList();
}

//选择一个一级科目，在所属关系列表中显示该一级科目下的二级科目
void SndSubConForm::fstSubclicked(const QModelIndex &index)
{
    fid = fstModel->data(fstModel->index(index.row(), 0)).toInt();
    refreshMapList();
}

//在二级科目列表中选择一个二级科目
void SndSubConForm::sndSubclicked(QListWidgetItem* item)
{
    sid = item->data(Qt::UserRole).toInt();
    ui.btnAddTo->setEnabled(true);
    int r = ui.lstSnd->currentRow();
    mapper2->setCurrentIndex(r);
    ui.edtCode->setText("");      //只有在所属关系列表中选中一项时，这两个才有意义
    ui.edtWeight->setText("");
}

//选择一个一二级科目的映射
void SndSubConForm::mapLstItemClicked(const QModelIndex &index)
{
    ui.btnRomveTo->setEnabled(true);
    fid = mapModel->data(mapModel->index(index.row(), 0)).toInt();
    sid = mapModel->data(mapModel->index(index.row(), 1)).toInt();
    mapper1->setCurrentIndex(ui.lstOwnership->currentIndex().row());

    //定位mapper2以使其显示二级科目的信息
    int i = 0;
    while(i < sndModel->rowCount()){
        int id = sndModel->data(sndModel->index(i, 0)).toInt();
        if(id == sid)
            break;
        i++;
    }
    mapper2->setCurrentIndex(i);

}

//将当前选中的二级科目加入到当前选中的一级科目下
void SndSubConForm::btnAddToClicked()
{
    QSqlQuery q;
    QString s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,1,1)")
            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_weight)
            .arg(fld_ssub_enable).arg(fid).arg(sid);
    q.exec(s);
    //回读此新的二级科目的id，并添加到全局变量中allSndSubs和allSndSubLNames
    s = QString("select last_insert_rowid()");
    bool r = q.exec(s);
    r = q.first();
    int id = q.value(0).toInt();
    allSndSubs[id] = q.value(1).toString();
    allSndSubLNames[id] = q.value(2).toString();
    ui.btnAddTo->setEnabled(false);

    //刷新
    refreshMapList();
//    s = QString("select FSAgent.fid, FSAgent.sid, FSAgent.subCode, "
//                "FSAgent.FrequencyStat, SecSubjects.subName "
//                "from FSAgent join SecSubjects "
//                "where (FSAgent.sid = SecSubjects.id) and "
//                "(FSAgent.fid = %1)").arg(fid);
//    mapModel->setQuery(s);
//    ui.lstOwnership->setModel(mapModel);
//    ui.lstOwnership->setModelColumn(4);
}

//从当前选中的一级科目中移除当前选中的二级科目
void SndSubConForm::btnRemoveToClicked()
{
    QSqlQuery q;
    QString s = QString("select id from %1 where %2=%3 and %4=%5").arg(tbl_ssub)
            .arg(fld_ssub_fid).arg(fid).arg(fld_ssub_nid).arg(sid);
    q.exec(s); q.first();
    int id = q.value(0).toInt();
    allSndSubs.remove(id);
    allSndSubLNames.remove(id);
    s = QString("delete from %1 where id=%2").arg(tbl_ssub).arg(id);
    q.exec(s);
    ui.btnRomveTo->setEnabled(false);
    refreshMapList();

//    s = QString("select FSAgent.fid, FSAgent.sid, FSAgent.subCode, "
//                "FSAgent.FrequencyStat, SecSubjects.subName "
//                "from FSAgent join SecSubjects "
//                "where (FSAgent.sid = SecSubjects.id) and "
//                "(FSAgent.fid = %1)").arg(fid);
//    mapModel->setQuery(s);
//    ui.lstOwnership->setModel(mapModel);
//    ui.lstOwnership->setModelColumn(4);

}

//新增二级科目
void SndSubConForm::btnAddClicked()
{
    int row = sndModel->rowCount();
    sndModel->insertRow(row);
    sndModel->setData(sndModel->index(row, NI_NAME), tr("科目名"));
    sndModel->setData(sndModel->index(row, NI_CALSS), 1);
    sndModel->submit();
    sndModel->select();
    mapper2->toLast();

    //将新增的二级科目添加到lstSnd
    int id = sndModel->data(sndModel->index(row, 0)).toInt();
    QString sname = sndModel->data(sndModel->index(row, NI_NAME)).toString();
    QListWidgetItem* item = new QListWidgetItem(sname);
    item->setData(Qt::UserRole, id);
    ui.lstSnd->addItem(item);
    refreshSndList();
    ui.lstSnd->setCurrentRow(row);
}

//删除二级科目
void SndSubConForm::btnDelClicked()
{
    //首先要在二级科目列表框中移除选中的科目
    int idx = ui.lstSnd->currentRow();
    //QListWidgetItem* item = ui.lstSnd->item(idx);
    //sid = item->data(Qt::UserRole).toInt();
    //ui.lstSnd->removeItemWidget(item);
    ui.lstSnd->takeItem(idx);

    //移除clsHash中的对应条目
    clsHash.take(sid);
    sndModel->removeRow(idx);
    sndModel->submit();

    if(idx == sndModel->rowCount()) //如果删除的是列表中的最后一个
        mapper2->setCurrentIndex(idx - 1);
    else
        mapper2->setCurrentIndex(idx);
}

//保存二级科目
void SndSubConForm::btnSaveClicked()
{
    QSqlQuery q;
    QString s;
    bool r;

    mapper2->submit();
    //mapper1->submit(); //mapModel是只读的

    //保存二级科目代码、权重值等信息到FSAgetn表
    QString subCode = ui.edtCode->text();
    int weight = ui.edtWeight->text().toInt();
    int isDetByMt;

    //首先检查在FSAgent表中是否已存在指定的一二级科目的对应关系条目
    s = QString("select id from %1 where %2=%3 and %4=%5").arg(tbl_ssub)
            .arg(fld_ssub_fid).arg(fid).arg(fld_ssub_nid).arg(sid);
    if(q.exec(s) && q.first()){
        int id = q.value(0).toInt();
        //这里还没有考虑保存启用状态等信息
        s = QString("update %1 set %2='%3',%4=%5 where id=%6")
                .arg(tbl_ssub).arg(fld_ssub_code).arg(subCode)
                .arg(fld_ssub_weight).arg(weight).arg(id);
        r = q.exec(s);
    }
    else{
        s = QString("insert into %1(%2,%3,%4,%5,%6,%7) values(%8,%9,'%10',%11,1,%12)")
                .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_code)
                .arg(fld_ssub_weight).arg(fld_ssub_enable).arg(fld_ssub_creator)
                .arg(fid).arg(sid).arg(subCode).arg(weight).arg(curUser->getUserId());
        r = q.exec(s);
    }

    refreshMapList();

}

//刷新一二级科目映射关系列表
void SndSubConForm::refreshMapList()
{
//    QString s = QString("select FSAgent.fid, FSAgent.sid, FSAgent.subCode, "
//                        "FSAgent.FrequencyStat, SecSubjects.subName, "
//                        "FSAgent.isDetByMt from FSAgent join SecSubjects "
//                        "where (FSAgent.sid = SecSubjects.id) and "
//                        "(FSAgent.fid = %1)").arg(fid);
    QString s = QString("select %1.%2,%1.%3,%1.%4,%1.%5, %6.%7 from %1 join %6 "
                        "where %1.%3 = %6.id and %1.%2 = %8")
            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_code)
            .arg(fld_ssub_weight).arg(tbl_nameItem).arg(fld_ni_name).arg(fid);
    //刷新所属关系列表
    mapModel->setQuery(s);
    int c = mapModel->rowCount();
    sndIdSet.clear();
    if(c > 0){
        for(int i = 0; i< c; ++i)
            sndIdSet.insert(mapModel->data(mapModel->index(i, 1)).toInt());
    }
    ui.lstOwnership->setModel(mapModel);
    ui.lstOwnership->setModelColumn(4);

    refreshSndList();

}

//刷新二级科目列表
void SndSubConForm::refreshSndList()
{    
    int index = ui.cmbSnd->currentIndex();
    if(index == 0){ //选全部二级科目
        for(int i = 0; i < ui.lstSnd->count(); ++i){
            int sid = ui.lstSnd->item(i)->data(Qt::UserRole).toInt();
            if(sndIdSet.contains(sid))
                ui.lstSnd->item(i)->setHidden(true);
            else
                ui.lstSnd->item(i)->setHidden(false);
        }
    }
    else{
        int cls = ui.cmbSnd->itemData(index).toInt(); //当前选择的二级科目类别ID
        for(int i = 0; i < ui.lstSnd->count(); ++i){
            int sid = ui.lstSnd->item(i)->data(Qt::UserRole).toInt();
            if((cls == clsHash.value(sid)) && (!sndIdSet.contains(sid)))
                ui.lstSnd->item(i)->setHidden(false);
            else
                ui.lstSnd->item(i)->setHidden(true);
        }
    }

}

void SndSubConForm::edtSNameEditingFinished()
{
    QString sname = ui.edtSubSName->text();
    if(ui.lstSnd->currentItem() != NULL)
        ui.lstSnd->currentItem()->setText(sname);

}
