#include <qxml.h>
#include <QDomDocument>
#include <QSqlQuery>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QBuffer>
#include <QItemSelectionModel>

#include <qglobal.h>


#include "dialog2.h"
#include "utils.h"
#include "widgets.h"
#include "printUtils.h"
#include "global.h"
#include "widgets.h"
#include "utils.h"
#include "completsubinfodialog.h"
#include "mainwindow.h"
#include "tdpreviewdialog.h"
#include "previewdialog.h"
#include "tables.h"
#include "subject.h"
#include "cal.h"
#include "dbutil.h"
#include "PzSet.h"
#include "pz.h"

//tem
//#include "dialog3.h"

#ifdef Q_OS_LINUX
#define	FW_NORMAL	400
#define	FW_BOLD		700
#endif







////////////////////////PrintSelectDialog////////////////////////////////////
PrintSelectDialog::PrintSelectDialog(QList<PingZheng*> choosablePzSets, PingZheng* curPz, QWidget *parent) :
    QDialog(parent),ui(new Ui::PrintSelectDialog),pzSets(choosablePzSets),curPz(curPz)
{
    ui->setupUi(this);
    if(choosablePzSets.isEmpty())
        enableWidget(false);
    else{
        ui->edtCur->setText(QString::number(curPz?curPz->number():0));
        QString allText;
        int count = pzSets.count();
        if(count == 1)
            allText = "1";
        else
            allText = QString("1-%1").arg(count);
        ui->edtAll->setText(allText);
    }
    connect(ui->rdoSel,SIGNAL(toggled(bool)),this,SLOT(selectedSelf(bool)));
}

PrintSelectDialog::~PrintSelectDialog()
{
    delete ui;
}

//设置要打印的凭证号集合
void PrintSelectDialog::setPzSet(QSet<int> pznSet)
{
    QString ps = IntSetToStr(pznSet);
    ui->edtSel->setText(ps); //将集合解析为简写文本表示形式
    ui->rdoSel->setChecked(true);
}

//设置当前的凭证号
void PrintSelectDialog::setCurPzn(int pzNum)
{
    if(pzNum != 0)
        ui->edtCur->setText(QString::number(pzNum));
    else{
        ui->rdoCur->setEnabled(false);
        ui->edtCur->setEnabled(false);
    }

}

/**
 * @brief PrintSelectDialog::getSelectedPzs
 *  获取选择要打印的凭证对象
 * @param pzs
 * @return 0：未选，1：所有凭证，2：当前凭证，3：自选凭证
 */
int PrintSelectDialog::getSelectedPzs(QList<PingZheng *> &pzs)
{
    pzs.clear();
    if(pzSets.isEmpty())
        return 0;
    if(ui->rdoCur->isChecked() && curPz){
        pzs<<curPz;
        return 2;
    }
    else if(ui->rdoAll->isChecked()){
        pzs = pzSets;
        return 1;
    }
    else{
        QSet<int> pzNums;
        if(!strToIntSet(ui->edtSel->text(),pzNums)){
            QMessageBox::warning(this,tr("警告信息"),tr("凭证范围选择格式有误！"));
            return 0;
        }
        foreach (PingZheng* pz, pzSets){
            if(pzNums.contains(pz->number()))
                pzs<<pz;
        }
        return 3;
    }
}

//获取打印模式（返回值 1：输出地打印机，2:打印预览，3：输出到PDF）
//int PrintSelectDialog::getPrintMode()
//{
//    if(ui->rdoPrint->isChecked())
//        return 1;
//    else if(ui->rdoPreview->isChecked())
//        return 2;
//    else
//        return 3;
//}

/**
 * @brief PrintSelectDialog::selectedSelf
 *  选择自选模式后，启用右边的编辑框
 * @param checked
 */
void PrintSelectDialog::selectedSelf(bool checked)
{
    ui->edtSel->setReadOnly(!checked);
}

/**
 * @brief PrintSelectDialog::enableWidget
 * @param en
 */
void PrintSelectDialog::enableWidget(bool en)
{
    ui->rdoAll->setEnabled(en);
    ui->rdoCur->setEnabled(en);
    ui->rdoSel->setEnabled(en);
    ui->edtAll->setEnabled(en);
    ui->edtCur->setEnabled(en);
    ui->edtSel->setEnabled(en);
}

/**
 * @brief PrintSelectDialog::IntSetToStr
 *  将凭证号集合转换为文本表示的简略范围表示形式
 * @param set
 * @return
 */
QString PrintSelectDialog::IntSetToStr(QSet<int> set)
{
    QString s;
    if(set.count() > 0){
        QList<int> pzs = set.toList();
        qSort(pzs.begin(),pzs.end());
        int prev = pzs[0],next = pzs[0];
        for(int i = 1; i < pzs.count(); ++i){
            if((pzs[i] - next) == 1){
                next = pzs[i];
            }
            else{
                if(prev == next)
                    s.append(QString::number(prev)).append(",");
                else
                    s.append(QString("%1-%2").arg(prev).arg(next)).append(",");
                prev = next = pzs[i];
            }
        }
        if(prev == next)
            s.append(QString::number(prev));
        else
            s.append(QString("%1-%2").arg(prev).arg(pzs[pzs.count() - 1]));
    }
    return s;
}

/**
 * @brief PrintSelectDialog::strToIntSet
 *  将文本表示的凭证号选择范围转换为等价的凭证号集合
 * @param s
 * @param set
 * @return
 */
bool PrintSelectDialog::strToIntSet(QString s, QSet<int> &set)
{
    //首先用规则表达式验证字符串中是否存在不可解析的字符，如有则返回false
    set.clear();
    if(false)
        return false;
    //对打印范围的编辑框文本进行解析，生成凭证号集合
    QStringList sels = s.split(",");
    for(int i = 0; i < sels.count(); ++i){
        if(sels[i].indexOf('-') == -1)
            set.insert(sels[i].toInt());
        else{
            QStringList ps = sels[i].split("-");
            int start = ps[0].toInt();
            int end = ps[1].toInt();
            for(int j = start; j <= end; ++j)
                set.insert(j);
        }
    }
    return true;
}


//////////////////////////LoginDialog///////////////////////////
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    //setLayout(ui->mLayout);
    //this->witch = witch;
    init();
}

void LoginDialog::init()
{    
    QHashIterator<int,User*> it(allUsers);
    int ruIndex = 0, index = 0;
    int recentUserId;
    AppConfig::getInstance()->getCfgVar(AppConfig::CVC_ResentLoginUser,recentUserId);
    while(it.hasNext()){
        it.next();
        if(it.value()->getUserId() == recentUserId)
            ruIndex = index;
        ui->cmbUsers->addItem(it.value()->getName(),it.value()->getUserId());
        index++;
    }
    ui->cmbUsers->setCurrentIndex(ruIndex);
    ui->edtPw->setFocus();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

User* LoginDialog::getLoginUser()
{
    int userId = ui->cmbUsers->itemData(ui->cmbUsers->currentIndex()).toInt();
    return allUsers.value(userId);
}

//登录
void LoginDialog::on_btnLogin_clicked()
{

    int userId =ui->cmbUsers->itemData(ui->cmbUsers->currentIndex()).toInt();
    if(allUsers.value(userId)->verifyPw(ui->edtPw->text()))
        accept();
    else
        QMessageBox::warning(this, tr("警告信息"), tr("密码不正确"));
}

//取消登录
void LoginDialog::on_btnCancel_clicked()
{
    QDialog::reject();
}

///////////////////////////SecConDialog/////////////////////////////////////
SecConDialog::SecConDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SecConDialog)
{
    ui->setupUi(this);
    ui->tabRight->setLayout(ui->trLayout);
    ui->tabOperate->setLayout(ui->toLayout);

    //初始化数据修改标记
//    rightDirty = false;
//    groupDirty = false;
//    userDirty = false;
//    operDirty = false;

    //添加用户面板上三个列表框的上下文菜单
    ui->lwOwner->addAction(ui->actAddGrpForUser);
    ui->lwOwner->addAction(ui->actDelGrpForUser);
    ui->lwUsers->addAction(ui->actAddUser);
    ui->lwUsers->addAction(ui->actDelUser);
    ui->lwAccounts->addAction(ui->actAddAcc);
    ui->lwAccounts->addAction(ui->actDelAcc);

    //添加增删用户组的上下文菜单
    ui->lwGroup->addAction(ui->actAddGroup);
    ui->lwGroup->addAction(ui->actDelGroup);

    //添加权限表的上下文菜单
//    ui->tvRight->addAction(ui->actAddRight);
//    ui->tvRight->addAction(ui->actDelRight);



    //
    //ui->lwOwner->addAction(ui->actChgUserOwner);

    //添加修改操作所需权限的上下文菜单
    //ui->trwOper->addAction(ui->actChgOpeRgt);
    ui->tabOperate->setVisible(false);
    ui->tabRight->setVisible(false);
    ui->tabRightType->setVisible(false);
    init();
}

//初始化
void SecConDialog::init()
{
    QTableWidgetItem* item;
    ValidableTableWidgetItem* vitem;

    QList<int> codes;
    vat = new QIntValidator(1, 1000, this);
    appCon = AppConfig::getInstance();



    //装载权限
    codes = allRights.keys();
    qSort(codes.begin(), codes.end());
    ui->tvRight->setRowCount(codes.count());
    ui->tvRight->setColumnWidth(0, 80);
    ui->tvRight->setColumnWidth(1, 80);
    ui->tvRight->setColumnWidth(2, 150);
    ui->tvRight->setColumnWidth(3, 500);

    for(int r = 0; r < codes.count(); ++r){
        Right* right = allRights.value(codes[r]);        
        item = new ValidableTableWidgetItem(QString::number(right->getCode()), vat);
        ui->tvRight->setItem(r, 0, item);        
        item = new QTableWidgetItem;
        item->setData(Qt::EditRole, right->getType()->code);
        ui->tvRight->setItem(r, 1, item);
        item = new QTableWidgetItem(right->getName());
        ui->tvRight->setItem(r, 2, item);
        item = new QTableWidgetItem(right->getExplain());
        ui->tvRight->setItem(r, 3, item);
    }

    //iTosItemDelegate* rightTypeDele = new iTosItemDelegate(allRightTypes, this);
    //ui->tvRight->setItemDelegateForColumn(1, rightTypeDele);

    connect(ui->tvRight, SIGNAL(cellChanged(int,int)),
            this, SLOT(onRightellChanged(int,int)));

    //装载用户组
    QListWidgetItem* litem;
    QList<UserGroup*> groups = allGroups.values();
    qSort(groups.begin(),groups.end(),groupByCode);
    foreach(UserGroup* g, groups){
        litem = new QListWidgetItem(g->getName());
        QVariant v;
        v.setValue<UserGroup*>(g);
        litem->setData(Qt::UserRole, v);
        ui->lwGroup->addItem(litem);
    }

    //初始化组面板的权限许可树
    genRightTree(0);
    ui->trwGroup->expandAll();

    //装载用户
    QList<User*> us = allUsers.values();
    qSort(us.begin(),us.end(),userByCode);
    foreach(User* u, us){
        QListWidgetItem* item = new QListWidgetItem(u->getName());
        QVariant v; v.setValue<User*>(u);
        item->setData(Qt::UserRole,v);
        ui->lwUsers->addItem(item);
    }
    ui->lwUsers->setCurrentRow(0);



    //装载权限类型
    //initRightTypes(0);

    ui->tabMain->setCurrentIndex(TI_GROUP);
    curTab = TI_GROUP;
    connect(ui->tabMain,SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));
}

/**
 * @brief 返回指定类别下的子类别
 * @param parent
 * @return
 */
QList<RightType *> SecConDialog::getRightType(RightType *parent)
{
    QList<RightType*> rts;
    foreach(RightType* rt, allRightTypes.values()){
        if(rt->pType == parent)
            rts<<rt;
    }
    qSort(rts.begin(),rts.end(),rightTypeByCode);
    return rts;
}

/**
 * @brief 返回指定类别下的权限
 * @param type
 * @return
 */
QList<Right *> SecConDialog::getRightsForType(RightType *type)
{
    QList<Right* > rs;
    foreach(Right* r, allRights.values()){
        if(r->getType() == type)
            rs<<r;
    }
    qSort(rs.begin(),rs.end(),rightByCode);
    return rs;
}

/**
 * @brief 初始化在组面板上的权限树
 * 权限按类别在树上分类展开，树的叶子是该类别下的权限，如果许可权限，则叶子有勾对标记
 */
//void SecConDialog::initRightTreesInGroupPanel()
//{

//}

/**
 * @brief 生成权限树
 * @param type      权限类别
 * @param isLeaf    true：权限，false：权限类别
 * @param parent    父节点
 */
void SecConDialog::genRightTree(RightType* type, bool isLeaf, QTreeWidgetItem *parent)
{
    if(isLeaf){
        foreach(Right* r, getRightsForType(type)){
            QStringList sl;
            sl<<QString("%1(%2)").arg(r->getName()).arg(r->getCode());
            QTreeWidgetItem* item = new QTreeWidgetItem(parent,sl);
            item->setData(0,Qt::UserRole+1,r->getCode());
            //item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            item->setCheckState(0,Qt::Unchecked);
        }
    }
    else{        
        foreach(RightType* rt, getRightType(type)){
            QStringList sl;
            sl<<QString("%1(%2)").arg(rt->name).arg(rt->code);
            QTreeWidgetItem* item;
            if(!parent)
                item = new QTreeWidgetItem(ui->trwGroup,sl);
            else
                item = new QTreeWidgetItem(parent,sl);
            item->setData(0,Qt::UserRole,rt->code);
            QList<RightType*> rts = getRightType(rt);
            genRightTree(rt,rts.isEmpty(),item);
        }

    }
}

//初始化用户组和操作的（第一层次）权限类型
void SecConDialog::initRightTypes(int pcode, QTreeWidgetItem* pitem)
{
    QString s;
    QSqlQuery q(bdb);
    s = QString("select code,name from rightType where pcode = %1").arg(pcode);
    bool r = q.exec(s);
    QTreeWidgetItem* item;
    if(pcode == 0){  //第一层次权限类型
        QList<QTreeWidgetItem*> items;
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            item = new QTreeWidgetItem(ui->trwGroup, QStringList(name));
            item->setData(0,Qt::UserRole,code);
            //items.append(item);
        }
        //ui->trwGroup->addTopLevelItems(items);
        for(int i = 0; i < ui->trwGroup->topLevelItemCount(); ++i){
            item = ui->trwGroup->topLevelItem(i);
            int code = item->data(0, Qt::UserRole).toInt();
            initRightTypes(code, item);
        }
    }
    else{
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            item = new QTreeWidgetItem(pitem, QStringList(name));
            item->setData(0,Qt::UserRole,code);
            //pitem->addChild(item);
            initRightTypes(code,item);
        }
    }


    //将组权限加入到对应的权限类别分支中
    item = ui->trwGroup->topLevelItem(0);
    //int i = 0;
    QTreeWidgetItem* ritem;
//    QHashIterator<int,Right*> it(allRights);
//    while(it.hasNext()){
//        it.next();
//        ritem = new QTreeWidgetItem(QStringList(it.value()->getName()));
//        //ritem = new QTreeWidgetItem;                            //QTreeWidgetItem::UserType); //此树项目表示权限而不是权限类别
//        //ritem->setText(0, it.value()->getName());
//        ritem->setCheckState(0,Qt::Checked);
//        //item = findItem(ui->trwGroup, it.value()->getType());
//        QString rname = it.value()->getName();
//        QString rtname = allRightTypes.value(it.value()->getType());
//        //QList<QTreeWidgetItem*> items = ui->trwGroup->findItems(rtname,Qt::MatchFixedString);
////        if(items.count() == 1){
////            items[0]->addChild(ritem);

////        }
//        item->addChild(ritem);
//        i++;
//    }

//    QList<QTreeWidgetItem *> items;
//    for(int i = 0; i<10; ++i){
//        ritem = new QTreeWidgetItem(item, QStringList(QString("Item-%1").arg(i)));
        //items.append(ritem);
//    }
//    item->addChildren(items);
    //    int j = 0;
}

/**
 * @brief 刷新指定用户组的权限
 * @param group
 * @param parent
 */
void SecConDialog::refreshRightForGroup(UserGroup *group, QTreeWidgetItem* parent)
{
    if(!parent){
        ui->edtGroupCode->setText(QString::number(group->getGroupCode()));
        ui->edtGroupName->setText(group->getName());
        ui->edtGroupExplain->setText(group->getExplain());
        for(int i = 0; i < ui->trwGroup->topLevelItemCount(); ++i){
            QTreeWidgetItem* item = ui->trwGroup->topLevelItem(i);
            refreshRightForGroup(group,item);
        }
    }
    else{
        int c = parent->childCount();
        if(c == 0){
            QSet<Right*> rs = group->getHaveRights();
            int rc = parent->data(0,Qt::UserRole+1).toInt();
            Right* r = allRights.value(rc);
            if(r)
                parent->setCheckState(0,rs.contains(r)?Qt::Checked:Qt::Unchecked);
        }
        else{
            for(int i = 0; i < c; ++i){
                QTreeWidgetItem* item = parent->child(i);
                refreshRightForGroup(group,item);
            }
        }
    }
}

/**
 * @brief 收集组的权限，以决定是否需要保存
 * @param group
 * @param rs 被赋于的权限（初始是空集合）
 * @param parent
 */
void SecConDialog::collectRightForGroup(QSet<Right*> &rs, QTreeWidgetItem *parent)
{
    if(!parent){
        for(int i = 0; i < ui->trwGroup->topLevelItemCount(); ++i){
            QTreeWidgetItem* item = ui->trwGroup->topLevelItem(i);
            collectRightForGroup(rs,item);
        }
    }
    else{
        int c = parent->childCount();
        if(c == 0){
            int rc = parent->data(0,Qt::UserRole+1).toInt();
            Right* r = allRights.value(rc);
            if(r && (parent->checkState(0) == Qt::Checked))
                    rs.insert(r);
        }
        else{
            for(int i = 0; i < c; ++i){
                QTreeWidgetItem* item = parent->child(i);
                collectRightForGroup(rs,item);
            }
        }
    }
}

/**
 * @brief 当前用户组的内容是否改变（包括组名、简介和权限等）
 * @param g 当前显示的组对象
 */
void SecConDialog::isCurGroupChanged(UserGroup *g)
{
    QSet<Right*> rs;
    collectRightForGroup(rs,0);
    bool changed = false;
    if(g->getHaveRights() != rs){
        g->setHaveRights(rs);
        changed = true;
    }
    if(ui->edtGroupName->text() != g->getName()){
        g->setName(ui->edtGroupName->text());
        changed = true;
    }
    if(ui->edtGroupExplain->text() != g->getExplain()){
        g->setExplain(ui->edtGroupExplain->text());
        changed = true;
    }
    if(changed)
        set_groups.insert(g);
}

void SecConDialog::viewUserInfos(User *u)
{
    ui->edtUserId->setText(QString::number(u->getUserId()));
    ui->edtUserName->setText(u->getName());
    ui->edtUserPw->setText(u->getPassword());
    ui->lwOwner->clear();
    QList<UserGroup*> gs = u->getOwnerGroups().toList();
    qSort(gs.begin(),gs.end(),groupByCode);
    foreach(UserGroup* g, gs){
        QListWidgetItem* item = new QListWidgetItem(g->getName());
        QVariant v; v.setValue<UserGroup*>(g);
        item->setData(Qt::UserRole,v);
        ui->lwOwner->addItem(item);
    }
    ui->lwAccounts->clear();
    if(!u->isSuperUser() && !u->isAdmin()){
        foreach(QString code,u->getExclusiveAccounts()){
            AccountCacheItem* accItem = appCon->getAccountCacheItem(code);
            QListWidgetItem* item;
            if(accItem){
                item = new QListWidgetItem(tr("%1（%2）").arg(accItem->accName).arg(code));
                item->setToolTip(accItem->accLName);
            }
            else{
                item = new QListWidgetItem(code);
                item->setFlags(item->flags() & !Qt::ItemIsEnabled );
                item->setToolTip("该账户在本机不存在");
            }
            item->setData(Qt::UserRole,code);
            ui->lwAccounts->addItem(item);
        }
    }
}

/**
 * @brief 判断当前用户的内容是否改变
 * @param u
 */
void SecConDialog::isCurUserChanged(User *u)
{
    bool isChanged = false;
    if(u->getName() != ui->edtUserName->text()){
        u->setName(ui->edtUserName->text());
        isChanged = true;
    }
    if(u->getPassword() != ui->edtUserPw->text()){
        u->setPassword(ui->edtUserPw->text());
        isChanged = true;
    }
    QSet<UserGroup*> gs;
    for(int i = 0; i < ui->lwOwner->count(); ++i){
        UserGroup* g = ui->lwOwner->item(i)->data(Qt::UserRole).value<UserGroup*>();
        gs.insert(g);
    }
    if(u->getOwnerGroups() != gs){
        u->setOwnerGroups(gs);
        isChanged = true;
    }
    QStringList sl;
    for(int i = 0; i < ui->lwAccounts->count(); ++i)
        sl<<ui->lwAccounts->item(i)->data(Qt::UserRole).toString();
    qSort(sl.begin(),sl.end());
    if(sl != u->getExclusiveAccounts()){
        u->setExclusiveAccounts(sl);
        isChanged = true;
    }
    if(isChanged)
        set_Users.insert(u);
}

SecConDialog::~SecConDialog()
{
    delete ui;
}

void SecConDialog::currentTabChanged(int index)
{
    TabIndexEnum ti = (TabIndexEnum)index;
    switch(curTab){
    case TI_GROUP:

        break;
    case TI_USER:
        break;
    case TI_RIGHT:
        break;
    case TI_RIGHTTYPE:
        break;
    }
    curTab = ti;
}

//在树中查找具有指定用户数据的项目（只考虑第一次匹配的项目）
QTreeWidgetItem* SecConDialog::findItem(QTreeWidget* tree, int code, QTreeWidgetItem* startItem)
{
    QTreeWidgetItem* item;
    int count;
    int i = 0;
    if(startItem == NULL){  //从顶级项目开始查找
        count = tree->topLevelItemCount();
        while(i < count){
            item = tree->topLevelItem(i);
            if(item->data(0, Qt::UserRole).toInt() == code)
                return item;
            if(item->childCount() > 0)
                item = findItem(tree, code, item);
            if(item == NULL)
                i++;
            else
                return item;
        }
    }
    else{  //从指定项目开始查找
        count = startItem->childCount();
        while(i < count){
            item = startItem->child(i);
            if(item->data(0, Qt::UserRole).toInt() == code)
                return item;
            if(item->childCount() > 0)
                item = findItem(tree, code, item);
            if(item == NULL)
                i++;
            else
                return item;
        }
    }
    return NULL;
}

//添加权限
void SecConDialog::on_actAddRight_triggered()
{
    //自动赋一个未使用的权限代码
    int rows = ui->tvRight->rowCount();
    ui->tvRight->setRowCount(++rows);
    //因为在初始化显示权限表前已经根据权限代码进行了排序，因此，表的最后一行即是最大的代码数字
    int code = ui->tvRight->item(rows -2, 0)->data(Qt::DisplayRole).toInt();
    ValidableTableWidgetItem* vitem = new ValidableTableWidgetItem(QString::number(++code), vat);
    ui->tvRight->setItem(rows - 1, 0, vitem);
    ui->tvRight->setItem(rows - 1, 1, new QTableWidgetItem());
    ui->tvRight->setItem(rows - 1, 2, new QTableWidgetItem());
    ui->tvRight->setCurrentCell(rows - 1, 1);
    //rightDirty = true;
}

//删除权限
void SecConDialog::on_actDelRight_triggered()
{
    //首先获取选择的行
    QList<int> row;
    QList<QTableWidgetSelectionRange> ranges = ui->tvRight->selectedRanges();
    foreach(QTableWidgetSelectionRange range, ranges){
        for(int i = 0; i < range.rowCount(); ++i){
            row.append(range.topRow() + i);
        }
    }
    if(row.count() > 0){
        qSort(row.begin(), row.end());
        for(int i = row.count() - 1; i > -1; i--){
            int r = row[i];
            ui->tvRight->removeRow(r);
        }
        //rightDirty = true;
    }


}

//保存
void SecConDialog::on_btnSave_clicked()
{
    //判断当前显示的组是否被编辑了
    if(curTab == TI_GROUP){
        //int gc = ui->lwGroup->currentItem()->data(Qt::UserRole).toInt();
        //UserGroup* g = allGroups.value(gc);
        UserGroup* g = ui->lwGroup->currentItem()->data(Qt::UserRole).value<UserGroup*>();
        isCurGroupChanged(g);
    }
    if(curTab == TI_USER){
        //int uc = ui->lwUsers->currentItem()->data(Qt::UserRole).toInt();
        //User* u = allUsers.value(uc);
        User* u = ui->lwUsers->currentItem()->data(Qt::UserRole).value<User*>();
        isCurUserChanged(u);
    }
    //...

    if(!set_groups.isEmpty()){
        foreach(UserGroup* g, set_groups.toList())
            appCon->saveUserGroup(g);
        set_groups.clear();        
    }
    QList<UserGroup*> gs = allGroups.values();
    for(int i = 0; i < ui->lwGroup->count(); ++i){
        UserGroup* g = ui->lwGroup->item(i)->data(Qt::UserRole).value<UserGroup*>();
        gs.removeOne(g);
    }
    if(!gs.isEmpty()){
        foreach(UserGroup* g, gs)
            appCon->saveUserGroup(g,true);
    }

    if(!set_Users.isEmpty()){
        foreach(User* u ,set_Users)
            appCon->saveUser(u);
        set_Users.clear();        
    }
    QList<User*> us = allUsers.values();
    for(int i = 0; i < ui->lwUsers->count(); ++i){
        User* u = ui->lwUsers->item(i)->data(Qt::UserRole).value<User*>();
        us.removeOne(u);
    }
    if(!us.isEmpty()){
        foreach(User* u, us)
            appCon->saveUser(u,true);
    }
}

//保存权限
void SecConDialog::saveRights()
{
    QSqlQuery q(bdb);
    QString s;
    bool rt;
    bool typeChanged = false; //权限类别改变了
    bool nameChanged = false; //名称改变标记
    bool explChanged = false; //权限简介改变标记
    QList<int> prevCodes = allRights.keys();

    //遍历权限表格
    for(int r = 0; r < ui->tvRight->rowCount(); ++r){
        int code = ui->tvRight->item(r, 0)->data(Qt::DisplayRole).toInt();
        int t = ui->tvRight->item(r, 1)->data(Qt::EditRole).toInt();
        RightType* type = allRightTypes.value(t);
        QString name = ui->tvRight->item(r, 2)->text();
        QString explain = ui->tvRight->item(r, 3)->text();
        if(!allRights.contains(code)){ //如果在全局权限中没有对应项，则应新建
            Right* right = new Right(code, type, name, explain);
            allRights[code] = right;
            //在基础数据库中添加新记录
            s = QString("insert into rights(code,type,name,explain) values(%1,%2,'%3','%4')")
                    .arg(code).arg(type->code).arg(name).arg(explain);
            rt = q.exec(s);
        }
        else if(type != allRights.value(code)->getType()){
            allRights.value(code)->setType(type);
            typeChanged = true;
        }
        else if((name != allRights.value(code)->getName())){
            allRights.value(code)->setName(name);
            nameChanged = true;
        }
        else if(explain != allRights.value(code)->getExplain()){
            allRights.value(code)->setExplain(explain);
            explChanged = true;
        }
        //更新名称和简介内容
        if(typeChanged || nameChanged || explChanged){
            s = QString("update rights set ");
            if(typeChanged)
                s.append(QString("type = %1, ").arg(type->code));
            if(nameChanged)
                s.append(QString("name = '%1', ").arg(name));
            if(explChanged)
                s.append(QString("explain = '%1', ").arg(explain));
            s.chop(2);
            s.append(QString(" where code = %1").arg(code));
            rt = q.exec(s);
            nameChanged = false; explChanged = false;
            int j = 0;
        }
        prevCodes.removeOne(code); //移除已经处理过的项目
    }
    //删除遗留权限
    if(prevCodes.count() > 0){
        for(int i = 0; i < prevCodes.count(); ++i){
            allRights.remove(prevCodes[i]);
            //删除数据库中的对应记录
            s = QString("delete from rights where code = %1").arg(prevCodes[i]);
            rt = q.exec(s);
            int j = 0;
        }
    }

}

void SecConDialog::saveGroups()
{

}

void SecConDialog::saveUsers()
{

}

void SecConDialog::saveOperates(){

}


//关闭
void SecConDialog::on_btnClose_clicked()
{
    if(!set_groups.isEmpty() || set_Users.isEmpty()){
        if(QMessageBox::Yes == QMessageBox::warning(this,"",tr("安全模块配置已被修改，要保存吗？"),
                                                   QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)){
            on_btnSave_clicked();
        }
    }
    close();
}

//修改组权限
void SecConDialog::on_actChgGrpRgt_triggered()
{

}

//修改用户所属组
void SecConDialog::on_actChgUserOwner_triggered()
{

}

//修改操作所需权限
void SecConDialog::on_actChgOpeRgt_triggered()
{

}


//当权限表的内容改变
void SecConDialog::onRightellChanged(int row, int column)
{
    if(column == 0){ //如果改变的是权限代码，要检测是否有同值冲突
        ValidableTableWidgetItem* item = (ValidableTableWidgetItem*)ui->tvRight->item(row, column);
        int code = item->text().toInt();
        //检测权限表中是否存在重复的代码
        for(int r = 0; r < ui->tvRight->rowCount(); ++r){
            if(r != row){ //排除本行
                if(code == ui->tvRight->item(r, 0)->data(Qt::DisplayRole).toInt()){
                     QMessageBox::warning(this, tr("提示信息"), tr("代码冲突或无效"));
                     item->setBackground(QBrush(Qt::red));
                     return;
                }
            }
        }
        item->setBackground(Qt::white);
    }
    //rightDirty = true;

}

/**
 * @brief 当前选择的组发生改变
 * @param current
 * @param previous
 */
void SecConDialog::on_lwGroup_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    int gc;
    UserGroup* g;
    //如果先前的用户组权限有改变，则先保存
    if(previous){
        g = previous->data(Qt::UserRole).value<UserGroup*>();
        //gc = previous->data(Qt::UserRole).toInt();
        //g = allGroups.value(gc);
        isCurGroupChanged(g);
    }
    //再刷新显示新的当前用户组的权限
    //gc = current->data(Qt::UserRole).toInt();
    //g = allGroups.value(gc);
    g = current->data(Qt::UserRole).value<UserGroup*>();
    refreshRightForGroup(g,0);
}

/**
 * @brief 当前选择的用户发生改变
 * @param current
 * @param previous
 */
void SecConDialog::on_lwUsers_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    int uc;
    User* u;
    if(previous){
        u = previous->data(Qt::UserRole).value<User*>();
        isCurUserChanged(u);
    }
    u = current->data(Qt::UserRole).value<User*>();
    viewUserInfos(u);
}

/**
 * @brief 添加用户所属组
 */
void SecConDialog::on_actAddGrpForUser_triggered()
{
    if(!ui->lwUsers->currentItem())
        return;
    User* u = ui->lwUsers->currentItem()->data(Qt::UserRole).value<User*>();
    QList<UserGroup*> gs = allGroups.values();
    QSetIterator<UserGroup*> it(u->getOwnerGroups());
    while(it.hasNext()){
        UserGroup* g = it.next();
        gs.removeOne(g);
    }
    if(gs.isEmpty()){
        QMessageBox::warning(this,"",tr("没有可用组可以添加！"));
        return;
    }
    QDialog dlg(this);
    QLabel title(tr("可用组："),&dlg);
    QListWidget lwGroup(&dlg);
    qSort(gs.begin(),gs.end(),groupByCode);
    foreach(UserGroup* g, gs){
        QListWidgetItem* item = new QListWidgetItem(g->getName());
        QVariant v; v.setValue<UserGroup*>(g);
        item->setData(Qt::UserRole,v);
        lwGroup.addItem(item);
    }
    lwGroup.setCurrentRow(0);
    QPushButton btnOk(tr("确定"),&dlg),btnCancel(tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb;
    lb.addWidget(&btnOk);
    lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&title);
    lm->addWidget(&lwGroup);
    lm->addLayout(&lb);
    dlg.setLayout(lm);
    dlg.resize(200,300);
    if(dlg.exec() == QDialog::Accepted){
        UserGroup* g = lwGroup.currentItem()->data(Qt::UserRole).value<UserGroup*>();
        QListWidgetItem* item = new QListWidgetItem(g->getName());
        QVariant v; v.setValue<UserGroup*>(g);
        item->setData(Qt::UserRole,v);
        ui->lwOwner->addItem(item);
    }
}

/**
 * @brief 移除用户所属组
 */
void SecConDialog::on_actDelGrpForUser_triggered()
{
    if(!ui->lwUsers->currentItem())
        return;
    if(!ui->lwOwner->currentItem())
        return;
    delete ui->lwOwner->takeItem(ui->lwOwner->currentRow());
}

/**
 * @brief 添加新用户组
 */
void SecConDialog::on_actAddGroup_triggered()
{
    QDialog dlg(this);
    int code = 0;
    for(int i=0; i < ui->lwGroup->count(); ++i){
        UserGroup* g = ui->lwGroup->item(i)->data(Qt::UserRole).value<UserGroup*>();
        if(g->getGroupCode() > code)
            code = g->getGroupCode();
    }
    code++;
    QLabel lbl0(tr("代码"),&dlg);
    QLineEdit edtCode(QString::number(code), &dlg);
    edtCode.setReadOnly(true);
    QLabel lbl1(tr("组名"),&dlg);
    QLineEdit edtName(&dlg);
    QLabel lbl2(tr("简要说明"),&dlg);
    QLineEdit edtExplain(&dlg);
    QGridLayout gl;
    gl.addWidget(&lbl0,0,0); gl.addWidget(&edtCode,0,1);
    gl.addWidget(&lbl1,1,0); gl.addWidget(&edtName,1,1);
    gl.addWidget(&lbl2,2,0); gl.addWidget(&edtExplain,2,1);
    QPushButton btnOk(tr("确定"),&dlg), btnCancel(tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb;
    lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addLayout(&gl); lm->addLayout(&lb);
    dlg.setLayout(lm);
    dlg.resize(200,200);
    if(QDialog::Rejected == dlg.exec())
        return;
    QString name = edtName.text();
    if(name.isEmpty()){
        QMessageBox::warning(this,"",tr("组名不能为空！"));
        return;
    }
    bool isExist = false;
    foreach(UserGroup* g, allGroups.values()){
        if(name == g->getName()){
            isExist = true;
            break;
        }
    }
    if(isExist){
        QMessageBox::warning(this,"",tr("组名（%1）重复！").arg(name));
        return;
    }
    UserGroup* g = new UserGroup(code,name,QSet<Right*>());
    g->setExplain(edtExplain.text());
    QListWidgetItem* item = new QListWidgetItem(g->getName());
    QVariant v; v.setValue<UserGroup*>(g);
    item->setData(Qt::UserRole,v);
    ui->lwGroup->addItem(item);
    ui->lwGroup->setCurrentRow(ui->lwGroup->count()-1);
    set_groups.insert(g);
}

/**
 * @brief 删除用户组
 */
void SecConDialog::on_actDelGroup_triggered()
{
    if(!ui->lwGroup->currentItem())
        return;
    delete ui->lwGroup->takeItem(ui->lwGroup->currentRow());
}

/**
 * @brief 创建新用户
 */
void SecConDialog::on_actAddUser_triggered()
{
    QDialog dlg(this);
    QLabel lbl1(tr("用户名"),&dlg),lbl2(tr("密码"),&dlg),lbl3(tr("确认密码"),&dlg);
    QLineEdit edtName(&dlg),edtPw(&dlg),edtPwConfirm(&dlg);
    edtPw.setEchoMode(QLineEdit::Password);
    edtPwConfirm.setEchoMode(QLineEdit::Password);
    QPushButton btnOk(tr("确定"),&dlg),btnCancel(tr("取消"),&dlg);
    QGridLayout gl;
    gl.addWidget(&lbl1,0,0); gl.addWidget(&edtName,0,1);
    gl.addWidget(&lbl2,1,0); gl.addWidget(&edtPw,1,1);
    gl.addWidget(&lbl3,2,0); gl.addWidget(&edtPwConfirm,2,1);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb; lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addLayout(&gl); lm->addLayout(&lb);
    dlg.setLayout(lm);
    dlg.resize(100,100);
    if(QDialog::Rejected == dlg.exec())
        return ;
    QString name = edtName.text();
    if(name.isEmpty()){
        QMessageBox::warning(this,"",tr("用户名不能为空！"));
        return;
    }
    bool isExist = false;
    for(int i = 0; i < ui->lwUsers->count(); ++i){
        User* u = ui->lwUsers->item(i)->data(Qt::UserRole).value<User*>();
        if(u->getName() == name){
            isExist = true;
            break;
        }
    }
    if(isExist){
        QMessageBox::warning(this,"",tr("用户名有冲突！"));
        return;
    }
    QString pw = edtPw.text();
    if(pw != edtPwConfirm.text()){
        QMessageBox::warning(this,"",tr("两次密码输入不一致，请重新输入密码！"));
        return;
    }
    User* u = new User(UNID,name,pw);
    QListWidgetItem* item = new QListWidgetItem(name);
    QVariant v; v.setValue<User*>(u);
    item->setData(Qt::UserRole,v);
    ui->lwUsers->addItem(item);
    ui->lwUsers->setCurrentRow(ui->lwUsers->count()-1);
    set_Users.insert(u);
}

/**
 * @brief 删除用户
 */
void SecConDialog::on_actDelUser_triggered()
{
    if(!ui->lwUsers->currentItem())
        return;
    delete ui->lwUsers->takeItem(ui->lwUsers->currentRow());
}

/**
 * @brief 添加专属账户
 */
void SecConDialog::on_actAddAcc_triggered()
{
    if(!ui->lwUsers->currentItem())
        return;
    QList<AccountCacheItem*> accItems = appCon->getAllCachedAccounts();
    User* u = ui->lwUsers->currentItem()->data(Qt::UserRole).value<User*>();
    QStringList codes = u->getExclusiveAccounts();
    foreach(AccountCacheItem* item, accItems){
        if(codes.contains(item->code))
            accItems.removeOne(item);
    }
    if(accItems.isEmpty()){
        QMessageBox::warning(this,"",tr("已没有可用账户！"));
        return;
    }
    QDialog dlg(this);
    QListWidget lwAccs(&dlg);
    foreach(AccountCacheItem* accItem, accItems){
        QListWidgetItem* item = new QListWidgetItem(accItem->accLName);
        item->setData(Qt::UserRole,accItem->code);
        lwAccs.addItem(item);
    }
    lwAccs.setCurrentRow(0);
    QPushButton btnOk(tr("确定"),&dlg),btnCancel(tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    QHBoxLayout lb; lb.addWidget(&btnOk); lb.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&lwAccs); lm->addLayout(&lb);
    dlg.setLayout(lm);
    dlg.resize(400,200);
    if(QDialog::Rejected == dlg.exec())
        return;
    QString code = lwAccs.currentItem()->data(Qt::UserRole).toString();
    AccountCacheItem* accItem = appCon->getAccountCacheItem(code);
    QListWidgetItem* item = new QListWidgetItem(tr("%1（%2）").arg(accItem->accName).arg(code));
    item->setData(Qt::UserRole,code);
    item->setToolTip(accItem->accLName);
    ui->lwAccounts->addItem(item);
    ui->lwAccounts->setCurrentRow(ui->lwAccounts->count()-1);
}

/**
 * @brief 移除专属账户
 */
void SecConDialog::on_actDelAcc_triggered()
{
    if(!ui->lwAccounts->currentItem())
        return;
    delete ui->lwAccounts->takeItem(ui->lwAccounts->currentRow());
}


/////////////////////SearchDialog///////////////////////////////////////
SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog)
{
    ui->setupUi(this);
}

SearchDialog::~SearchDialog()
{
    delete ui;
}
















