#include "seccondialog.h"
#include "config.h"
#include "securitys.h"
#include "widgets.h"
#include "myhelper.h"

#include <QBuffer>
#include <QCloseEvent>
#include <QMenu>

///////////////////////////SecConDialog/////////////////////////////////////
SecConDialog::SecConDialog(QByteArray* state, QWidget *parent) :QDialog(parent),ui(new Ui::SecConDialog)
{
    ui->setupUi(this);
    dirty = false;
    isCancel = false;
    ui->tabRight->setLayout(ui->trLayout);
    enColor.setColor(Qt::darkGreen);
    disColor.setColor(Qt::red);
    nonColor.setColor(Qt::gray);
    connect(ui->lwGroup,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(listContextMenuRequested(QPoint)));
    connect(ui->lwUsers,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(listContextMenuRequested(QPoint)));
    connect(ui->lwOwner,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(listContextMenuRequested(QPoint)));
    connect(ui->lwAccounts,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(listContextMenuRequested(QPoint)));
    connect(ui->lwGroupContain,SIGNAL(customContextMenuRequested(QPoint)),
            this,SLOT(listContextMenuRequested(QPoint)));
    connect(ui->edtGroupName,SIGNAL(textEdited(QString)),this,SLOT(groupNameChanged(QString)));
    connect(ui->edtUserName,SIGNAL(textEdited(QString)),this,SLOT(userNameChanged(QString)));
    connect(ui->twUserRights,SIGNAL(itemClicked(QTreeWidgetItem*,int)),this,SLOT(userRightItemClicked(QTreeWidgetItem*,int)));
    init();
    setState(state);
}

SecConDialog::~SecConDialog()
{
    if(!set_Users.isEmpty()){
        foreach(User* u, set_Users){
            if(u->getUserId() == 0)
                delete u;
        }
        set_Users.clear();
    }
    if(!set_groups.isEmpty()){
        foreach(UserGroup* g, set_groups){
            if(g->getGroupId() == 0)
                delete g;
        }
        set_groups.clear();
    }
    delete ui;
}

void SecConDialog::setState(QByteArray *state)
{
    qint8 tabIndex;
    if(!state)
        tabIndex = 0;
    else{
        QBuffer bf(state);
        QDataStream in(&bf);
        bf.open(QIODevice::ReadOnly);
        in>>tabIndex;
        bf.close();
        ui->tabMain->setCurrentIndex(tabIndex);
    }
    curTab = (TabIndexEnum)tabIndex;
}

QByteArray *SecConDialog::getState()
{
    QByteArray* ba = new QByteArray;
    QBuffer bf(ba);
    QDataStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    qint8 tabIndex = ui->tabMain->currentIndex();
    out<<tabIndex;
    bf.close();
    return ba;
}

//初始化
void SecConDialog::init()
{
    QTableWidgetItem* item;
    QList<int> codes;
    appCon = AppConfig::getInstance();
    //装载权限
    codes = allRights.keys();
    qSort(codes.begin(), codes.end());
    ui->tvRight->setRowCount(codes.count());
    ui->tvRight->setColumnWidth(0, 60);
    ui->tvRight->setColumnWidth(1, 150);
    ui->tvRight->setColumnWidth(2, 150);
    for(int r = 0; r < codes.count(); ++r){
        Right* right = allRights.value(codes[r]);
        item = new QTableWidgetItem(QString::number(right->getCode()));
        ui->tvRight->setItem(r, 0, item);
        item = new QTableWidgetItem(right->getType()->name);
        ui->tvRight->setItem(r, 1, item);
        item = new QTableWidgetItem(right->getName());
        ui->tvRight->setItem(r, 2, item);
        item = new QTableWidgetItem(right->getExplain());
        ui->tvRight->setItem(r, 3, item);
    }

    //装载用户组
    QListWidgetItem* litem;
    QList<UserGroup*> groups = allGroups.values();
    qSort(groups.begin(),groups.end(),groupByCode);
    foreach(UserGroup* g, groups){
        litem = new QListWidgetItem(g->getName(),ui->lwGroup);
        QVariant v;
        v.setValue<UserGroup*>(g);
        litem->setData(Qt::UserRole, v);
    }

    //初始化组面板的权限许可树
    genRightTree(ui->trwGroup,0);
    ui->trwGroup->expandAll();

    //装载用户
    genRightTree(ui->twUserRights,0);
    ui->twUserRights->expandAll();
    QList<User*> us = allUsers.values();
    qSort(us.begin(),us.end(),userByCode);
    foreach(User* u, us){
        QListWidgetItem* item = new QListWidgetItem(u->getName(),ui->lwUsers);
        QVariant v; v.setValue<User*>(u);
        item->setData(Qt::UserRole,v);
    }
    ui->lwUsers->setCurrentRow(0);
    connect(ui->tabMain,SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));
}

bool SecConDialog::isDirty()
{
    //判断当前显示的组或用户是否被编辑了
    if(curTab == TI_GROUP && ui->lwGroup->currentItem()){
        UserGroup* g = ui->lwGroup->currentItem()->data(ROLE_USERGROUP).value<UserGroup*>();
        isCurGroupChanged(g);
    }
    if(curTab == TI_USER && ui->lwUsers->currentItem()){
        User* u = ui->lwUsers->currentItem()->data(ROLE_USER).value<User*>();
        isCurUserChanged(u);
    }
    if(dirty)
        return true;
    if(!set_groups.isEmpty() || !set_Users.isEmpty()){
        dirty = true;
        return true;
    }
    if(allGroups.count() != ui->lwGroup->count()){
        dirty = true;
        return true;
    }
    if(allUsers.count() != ui->lwUsers->count()){
        dirty = true;
        return true;
    }
    return false;
}

void SecConDialog::save()
{
    if(!dirty)
        return;
    //保存组
    if(!set_groups.isEmpty()){
        foreach(UserGroup* g, set_groups.toList())
            appCon->saveUserGroup(g);
        set_groups.clear();
    }
    if(allGroups.count() != ui->lwGroup->count()){
        QList<UserGroup*> gs = allGroups.values();
        for(int i = 0; i < ui->lwGroup->count(); ++i){
            UserGroup* g = ui->lwGroup->item(i)->data(ROLE_USERGROUP).value<UserGroup*>();
            gs.removeOne(g);
        }
        if(!gs.isEmpty()){
            foreach(UserGroup* g, gs){
                allGroups.remove(g->getGroupCode());
                appCon->saveUserGroup(g,true);
                foreach(User* u, allUsers.values()){
                    if(u->getOwnerGroups().contains(g)){
                        u->delGroup(g);
                        set_Users.insert(u);
                    }
                }
            }
            qDeleteAll(gs);
        }
    }
    //保存用户
    if(!set_Users.isEmpty()){
        foreach(User* u ,set_Users)
            appCon->saveUser(u);
        set_Users.clear();
    }
    if(allUsers.count() != ui->lwUsers->count()){
        QList<User*> us = allUsers.values();
        for(int i = 0; i < ui->lwUsers->count(); ++i){
            User* u = ui->lwUsers->item(i)->data(ROLE_USER).value<User*>();
            us.removeOne(u);
        }
        if(!us.isEmpty()){
            foreach(User* u, us){
                allUsers.remove(u->getUserId());
                appCon->saveUser(u,true);
                delete u;
            }
        }
    }
    dirty = false;
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
 * @brief 生成权限树
 * @param tree      用户/组的权限树
 * @param type      权限类别
 * @param isLeaf    true：权限，false：权限类别
 * @param parent    父节点
 */
void SecConDialog::genRightTree(QTreeWidget* tree, RightType* type, bool isLeaf, QTreeWidgetItem *parent)
{
    if(isLeaf){
        foreach(Right* r, getRightsForType(type)){
            QStringList sl;
            sl<<QString("%1(%2)").arg(r->getName()).arg(r->getCode());
            QTreeWidgetItem* item = new QTreeWidgetItem(parent,sl);
            QVariant v; v.setValue<Right*>(r);
            item->setData(0,ROLE_RIGHT,v);
            item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
            item->setCheckState(0,Qt::Unchecked);
            if(tree == ui->trwGroup){
                groupRightItems<<item;
            }
            else{
                item->setForeground(0,nonColor);
                userRightItems<<item;
            }
        }
    }
    else{
        foreach(RightType* rt, getRightType(type)){
            QStringList sl;
            sl<<QString("%1(%2)").arg(rt->name).arg(rt->code);
            QTreeWidgetItem* item;
            if(!parent)
                item = new QTreeWidgetItem(tree,sl);
            else
                item = new QTreeWidgetItem(parent,sl);
            item->setData(0,ROLE_RIGHTTYPE_CODE,rt->code);
            QList<RightType*> rts = getRightType(rt);
            genRightTree(tree,rt,rts.isEmpty(),item);
        }

    }
}

/**
 * @brief 刷新指定用户组的权限
 * @param group
 * @param parent
 */
void SecConDialog::refreshRightsForGroup(UserGroup *group)
{
    if(!group)
        return;
    ui->edtGroupCode->setText(QString::number(group->getGroupCode()));
    ui->edtGroupName->setText(group->getName());
    ui->edtGroupExplain->setPlainText(group->getExplain());
    QSet<Right*> rs = group->getAllRights();
    foreach(QTreeWidgetItem* item, groupRightItems){
        Right* r = item->data(0,ROLE_RIGHT).value<Right*>();
        if(rs.contains(r)){
            item->setCheckState(0,Qt::Checked);
            if(group->isGroupRight(r))
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            else
                item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        }
        else{
            item->setCheckState(0,Qt::Unchecked);
            item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        }
    }
}

/**
 * @brief SecConDialog::refreshMemberInGroup
 * 刷新显示当前组的所属组成员
 * @param g
 */
void SecConDialog::refreshMemberInGroup(UserGroup *g)
{
    ui->lwGroupContain->clear();
    QList<UserGroup*> gs = g->getOwnerGroups().toList();
    qSort(gs.begin(),gs.end(),groupByCode);
    foreach(UserGroup* g, gs){
        QVariant v;
        v.setValue<UserGroup*>(g);
        QListWidgetItem* item = new QListWidgetItem(g->getName(),ui->lwGroupContain);
        item->setData(Qt::UserRole,v);
    }
}

/**
 * @brief 收集组的额外权限，以决定是否需要保存
 * @param group
 * @param rs 被赋于的权限（初始是空集合）
 * @param parent
 */
void SecConDialog::collectRightsForGroup(QSet<Right*> &rs)
{
    foreach(QTreeWidgetItem* item, groupRightItems){
        if((item->checkState(0) == Qt::Checked) && item->flags().testFlag(Qt::ItemIsEnabled)){
            Right* r = item->data(0,ROLE_RIGHT).value<Right*>();
            rs<<r;
        }
    }
}

/**
 * @brief 当前用户组的内容是否改变（包括组名、简介所属组和权限等）
 * @param g 当前显示的组对象
 */
void SecConDialog::isCurGroupChanged(UserGroup *g)
{
    QSet<Right*> rs;
    collectRightsForGroup(rs);
    bool changed = false;
    if(g->getExtraRights() != rs){
        g->setHaveRights(rs);
        changed = true;
    }
    QSet<UserGroup*> gs;
    for(int i = 0; i < ui->lwGroupContain->count(); ++i)
        gs.insert(ui->lwGroupContain->item(i)->data(Qt::UserRole).value<UserGroup*>());
    if(gs != g->getOwnerGroups()){
        g->setOwnerGroups(gs);
        changed = true;
    }
    if(ui->edtGroupName->text() != g->getName()){
        g->setName(ui->edtGroupName->text());
        changed = true;
    }
    if(ui->edtGroupExplain->toPlainText() != g->getExplain()){
        g->setExplain(ui->edtGroupExplain->toPlainText());
        changed = true;
    }
    if(changed){
        set_groups.insert(g);
        dirty = true;
    }
}

void SecConDialog::viewUserInfos(User *u)
{
    ui->edtUserId->setText(QString::number(u->getUserId()));
    ui->chkIsEnabled->setChecked(u->isEnabled());
    ui->edtUserName->setText(u->getName());
    ui->edtUserPw->setText(u->getPassword());
    ui->lwOwner->clear();
    QList<UserGroup*> gs = u->getOwnerGroups().toList();
    qSort(gs.begin(),gs.end(),groupByCode);
    foreach(UserGroup* g, gs){
        QListWidgetItem* item = new QListWidgetItem(g->getName(),ui->lwOwner);
        QVariant v; v.setValue<UserGroup*>(g);
        item->setData(ROLE_USERGROUP,v);
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
            item->setData(ROLE_ACCOUNT_CODE,code);
            ui->lwAccounts->addItem(item);
        }
    }
    refreshRightsForUser(u);
}

/**
 * @brief 判断当前用户的内容是否改变
 * @param u
 */
void SecConDialog::isCurUserChanged(User *u)
{
    bool isChanged = false;
    if(u->isEnabled() ^ ui->chkIsEnabled->isChecked()){
        u->setEnabled(ui->chkIsEnabled->isChecked());
        isChanged = true;
    }
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
        UserGroup* g = ui->lwOwner->item(i)->data(ROLE_USERGROUP).value<UserGroup*>();
        gs.insert(g);
    }
    if(u->getOwnerGroups() != gs){
        u->setOwnerGroups(gs);
        isChanged = true;
    }
    QStringList sl;
    for(int i = 0; i < ui->lwAccounts->count(); ++i)
        sl<<ui->lwAccounts->item(i)->data(ROLE_ACCOUNT_CODE).toString();
    qSort(sl.begin(),sl.end());
    if(sl != u->getExclusiveAccounts()){
        u->setExclusiveAccounts(sl);
        isChanged = true;
    }
    QSet<Right*> rs;
    collectDisRightsForUser(rs);
    if(rs != u->getAllDisRights()){
        isChanged = true;
        u->setAllDisRights(rs);
    }
    if(isChanged){
        set_Users.insert(u);
        dirty = true;
    }
}

/**
 * @brief 刷新用户具有的权限
 * @param u
 */
void SecConDialog::refreshRightsForUser(User *u)
{
    QSet<Right*> grs,rs;
    foreach(UserGroup* g, u->getOwnerGroups())
        grs += g->getAllRights();
    rs = grs - u->getAllDisRights();
    bool isRoot = u->isSuperUser();
    foreach(QTreeWidgetItem* ti, userRightItems){
        if(isRoot){
            ti->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
            ti->setCheckState(0,Qt::Unchecked);
            ti->setForeground(0,enColor);
            continue;
        }
        Right* r = ti->data(0,ROLE_RIGHT).value<Right*>();
        if(grs.contains(r))
            ti->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        else
            ti->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
        if(rs.contains(r)){
            ti->setCheckState(0,Qt::Unchecked);
            ti->setForeground(0,enColor);
        }
        else{
            if(u->isDisabledRight(r)){
                ti->setCheckState(0,Qt::Checked);
                ti->setForeground(0,disColor);
            }
            else{
                ti->setCheckState(0,Qt::Unchecked);
                ti->setForeground(0,nonColor);
            }
        }
    }
}

/**
 * @brief 收集用户的禁用权限
 * @param rs
 */
void SecConDialog::collectDisRightsForUser(QSet<Right *> &rs)
{
    foreach(QTreeWidgetItem* ti, userRightItems){
        if(!ti->flags().testFlag(Qt::ItemIsEnabled))
            continue;
        if(ti->checkState(0) == Qt::Checked){
            Right* r = ti->data(0,ROLE_RIGHT).value<Right*>();
            rs.insert(r);
        }
    }
}



void SecConDialog::closeEvent(QCloseEvent *event)
{
    if(isCancel){
        if(!set_Users.isEmpty()){
            foreach(User* u, set_Users){
                if(u->getUserId() == 0)
                    continue;
                appCon->restorUser(u);
            }
        }
        if(!set_groups.isEmpty()){
            foreach(UserGroup* g, set_groups){
                if(g->getGroupId() == 0)
                    continue;
                appCon->restoreUserGroup(g);
            }
        }
        event->accept();
    }
    else{
        if(isDirty()){
            QMessageBox::warning(this,"",tr("安全模块设置已更改，请先保存或取消！"));
            event->ignore();
            return;
        }
        event->accept();
    }
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
    }
    curTab = ti;
}

void SecConDialog::listContextMenuRequested(const QPoint &pos)
{
    QListWidget* lw = qobject_cast<QListWidget*>(sender());
    if(!lw)
        return;
    QMenu m;
    QListWidgetItem* item = lw->itemAt(pos);
    if(lw == ui->lwGroup){
        m.addAction(ui->actAddNewGroup);
        if(item)
            m.addAction(ui->actDelSelGroup);
    }
    else if(lw == ui->lwUsers){
        m.addAction(ui->actAddUser);
        if(item)
            m.addAction(ui->actDelUser);
    }
    else if(lw == ui->lwOwner){
        if(!ui->lwUsers->currentItem())
            return;
        m.addAction(ui->actAddGrpForUser);
        if(item)
            m.addAction(ui->actDelGrpForUser);
    }
    else if(lw == ui->lwAccounts){
        if(!ui->lwUsers->currentItem())
            return;
        User* u = ui->lwUsers->currentItem()->data(ROLE_USER).value<User*>();
        if(u->isSuperUser() || u->isAdmin())
            return;
        m.addAction(ui->actAddAcc);
        if(item)
            m.addAction(ui->actDelAcc);
    }
    else if(lw == ui->lwGroupContain){
        if(!ui->lwGroup->currentItem())
            return;
        m.addAction(ui->actAddGroup);
        if(item)
            m.addAction(ui->actDelGroup);
    }
    m.exec(lw->mapToGlobal(pos));
}

void SecConDialog::groupNameChanged(QString name)
{
    if(!ui->lwGroup->currentItem())
        return;
    ui->lwGroup->currentItem()->setText(name);
}

void SecConDialog::userNameChanged(QString name)
{
    if(!ui->lwUsers->currentItem())
        return;
    ui->lwUsers->currentItem()->setText(name);
}


//保存
void SecConDialog::on_btnSave_clicked()
{
    if(isDirty())
        save();
}

//关闭
void SecConDialog::on_btnClose_clicked()
{
    if(isDirty())
        save();
    MyMdiSubWindow* w = qobject_cast<MyMdiSubWindow*>(parent());
    if(w)
        w->close();
}

void SecConDialog::on_btnCancel_clicked()
{
    MyMdiSubWindow* w = qobject_cast<MyMdiSubWindow*>(parent());
    if(w){
        isCancel = true;
        w->close();
    }
}

/**
 * @brief 当前选择的组发生改变
 * @param current
 * @param previous
 */
void SecConDialog::on_lwGroup_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    UserGroup* g;
    //如果先前的用户组权限有改变，则先保存
    if(previous){
        g = previous->data(ROLE_USERGROUP).value<UserGroup*>();
        isCurGroupChanged(g);
    }
    //再刷新显示新的当前用户组的权限
    g = current->data(ROLE_USERGROUP).value<UserGroup*>();
    refreshRightsForGroup(g);
    refreshMemberInGroup(g);
}

/**
 * @brief 当前选择的用户发生改变
 * @param current
 * @param previous
 */
void SecConDialog::on_lwUsers_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    User* u;
    if(previous){
        u = previous->data(ROLE_USER).value<User*>();
        isCurUserChanged(u);
    }
    u = current->data(ROLE_USER).value<User*>();
    viewUserInfos(u);
}

/**
 * @brief 添加用户所属组
 */
void SecConDialog::on_actAddGrpForUser_triggered()
{
    if(!ui->lwUsers->currentItem())
        return;
    User* u = ui->lwUsers->currentItem()->data(ROLE_USER).value<User*>();
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
    lwGroup.setSelectionMode(QAbstractItemView::ExtendedSelection);
    qSort(gs.begin(),gs.end(),groupByCode);
    foreach(UserGroup* g, gs){
        QListWidgetItem* item = new QListWidgetItem(g->getName(),&lwGroup);
        QVariant v; v.setValue<UserGroup*>(g);
        item->setData(ROLE_USERGROUP,v);
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
        QList<QListWidgetItem*> items = lwGroup.selectedItems();
        if(items.isEmpty())
            return;
        QSet<Right*> rs;
        foreach(QListWidgetItem* item, items){
            UserGroup* g = item->data(ROLE_USERGROUP).value<UserGroup*>();
            QListWidgetItem* li = new QListWidgetItem(g->getName(),ui->lwOwner);
            QVariant v; v.setValue<UserGroup*>(g);
            li->setData(ROLE_USERGROUP,v);
            rs += g->getAllRights();
        }
        foreach(QTreeWidgetItem* item, userRightItems){
            Right* r = item->data(0,ROLE_RIGHT).value<Right*>();
            if(rs.contains(r)){
                item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
                if(u->isDisabledRight(r))
                    item->setForeground(0,disColor);
                else
                    item->setForeground(0,enColor);
            }
        }
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
    UserGroup* g = ui->lwOwner->takeItem(ui->lwOwner->currentRow())->data(Qt::UserRole)
            .value<UserGroup*>();
    QSet<Right*> rs = g->getAllRights();
    QSet<Right*> ars;for(int i = 0; i < ui->lwOwner->count(); ++i){
        UserGroup* g = ui->lwOwner->item(i)->data(ROLE_USERGROUP).value<UserGroup*>();
        ars += g->getAllRights();
    }
    foreach(Right* r, rs){
        if(ars.contains(r))
            rs.remove(r);
    }
    if(rs.isEmpty())
        return;
    foreach(QTreeWidgetItem* item, userRightItems){
        if(!item->flags().testFlag(Qt::ItemIsEnabled))
            continue;
        Right* r = item->data(0,ROLE_RIGHT).value<Right*>();
        if(rs.contains(r)){
            item->setCheckState(0,Qt::Unchecked);
            item->setForeground(0,nonColor);
            item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
        }
    }
}

/**
 * @brief 添加新用户组
 */
void SecConDialog::on_actAddNewGroup_triggered()
{
    QDialog dlg(this);
    int code = 0;
    for(int i=0; i < ui->lwGroup->count(); ++i){
        UserGroup* g = ui->lwGroup->item(i)->data(ROLE_USERGROUP).value<UserGroup*>();
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
    UserGroup* g = new UserGroup(0,code,name,QSet<Right*>());
    g->setExplain(edtExplain.text());
    QListWidgetItem* item = new QListWidgetItem(g->getName(),ui->lwGroup);
    QVariant v; v.setValue<UserGroup*>(g);
    item->setData(ROLE_USERGROUP,v);
    ui->lwGroup->setCurrentRow(ui->lwGroup->count()-1);
    set_groups.insert(g);
}

/**
 * @brief 删除用户组
 */
void SecConDialog::on_actDelSelGroup_triggered()
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
    dlg.setWindowTitle(tr("新用户"));
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
    dlg.resize(300,200);
    if(QDialog::Rejected == dlg.exec())
        return ;
    QString name = edtName.text();
    if(name.isEmpty()){
        QMessageBox::warning(this,"",tr("用户名不能为空！"));
        return;
    }
    bool isExist = false;
    for(int i = 0; i < ui->lwUsers->count(); ++i){
        User* u = ui->lwUsers->item(i)->data(ROLE_USER).value<User*>();
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
    QListWidgetItem* item = new QListWidgetItem(name,ui->lwUsers);
    QVariant v; v.setValue<User*>(u);
    item->setData(ROLE_USER,v);
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
    User* u = ui->lwUsers->currentItem()->data(ROLE_USER).value<User*>();
    QStringList codes;
    for(int i = 0; i < ui->lwAccounts->count(); ++i)
        codes<<ui->lwAccounts->item(i)->data(ROLE_ACCOUNT_CODE).toString();
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
        QListWidgetItem* item = new QListWidgetItem(accItem->accLName,&lwAccs);
        item->setData(Qt::UserRole,accItem->code);
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
    QListWidgetItem* item = new QListWidgetItem(tr("%1（%2）").arg(accItem->accName).arg(code),ui->lwAccounts);
    item->setData(ROLE_ACCOUNT_CODE,code);
    item->setToolTip(accItem->accLName);
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

/**
 * @brief SecConDialog::on_actAddGroup_triggered
 * 在当前组内添加所属组
 */
void SecConDialog::on_actAddGroup_triggered()
{
    UserGroup* cg = ui->lwGroup->currentItem()->data(Qt::UserRole).value<UserGroup*>();
    QList<UserGroup*> gs = allGroups.values();
    gs.removeOne(cg);
    foreach(UserGroup* g, cg->getOwnerGroups())
        gs.removeOne(g);
    if(gs.isEmpty()){
        myHelper::ShowMessageBoxWarning(tr("没有可用的组可以加入！"));
        return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(tr("添加所属组"));
    QLabel lb(tr("可用的组"),&dlg);
    QListWidget lw(&dlg);
    lw.setSelectionMode(QAbstractItemView::ExtendedSelection);
    qSort(gs.begin(),gs.end(),groupByCode);
    foreach(UserGroup* g, gs){
        QListWidgetItem* item = new QListWidgetItem(g->getName(),&lw);
        QVariant v; v.setValue<UserGroup*>(g);
        item->setData(Qt::UserRole,v);
    }
    QHBoxLayout lbtn;
    QPushButton btnOk(tr("确定"),&dlg);
    QPushButton btnCancel(tr("取消"),&dlg);
    connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));
    connect(&btnCancel,SIGNAL(clicked()),&dlg,SLOT(reject()));
    lbtn.addWidget(&btnOk); lbtn.addWidget(&btnCancel);
    QVBoxLayout* lm = new QVBoxLayout;
    lm->addWidget(&lb); lm->addWidget(&lw); lm->addLayout(&lbtn);
    dlg.setLayout(lm);
    if(dlg.exec() == QDialog::Rejected)
        return;
    QSet<Right*> rs;
    foreach(QListWidgetItem* item, lw.selectedItems()){
        ui->lwGroupContain->addItem(new QListWidgetItem(*item));
        UserGroup* g = item->data(Qt::UserRole).value<UserGroup*>();
        rs += g->getAllRights();
    }
    if(rs.isEmpty())
        return;
    foreach(QTreeWidgetItem* item, groupRightItems){
        Right* r = item->data(0,ROLE_RIGHT).value<Right*>();
        if(rs.contains(r)){
            item->setCheckState(0,Qt::Checked);
            item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
        }
    }
}

/**
 * @brief SecConDialog::on_actDelGroup_triggered
 * 在当前组中移除所选的组
 */
void SecConDialog::on_actDelGroup_triggered()
{
    QListWidgetItem* item = ui->lwGroup->currentItem();
    if(!item)
        return;
    UserGroup* cg = item->data(Qt::UserRole).value<UserGroup*>(); //当前组
    item = ui->lwGroupContain->currentItem();
    UserGroup* g = item->data(Qt::UserRole).value<UserGroup*>();  //欲移除组
    ui->lwGroupContain->takeItem(ui->lwGroupContain->currentRow());
    QSet<Right*> rs = g->getAllRights();    //欲移除组所拥有的全部权限
    QSet<Right*> grs;
    QSet<UserGroup*> gs = cg->getOwnerGroups();
    gs.remove(g);
    foreach(UserGroup* g, gs){              //当前组在移除某个所属组后，其所有所属组拥有的权限
        grs += g->getAllRights();
    }
    foreach(QTreeWidgetItem* item, groupRightItems){
        Right* r = item->data(0,ROLE_RIGHT).value<Right*>();
        if(rs.contains(r)){
            item->setCheckState(0,Qt::Unchecked);
            if(grs.contains(r))
                item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
            else
                item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        }
    }
}

void SecConDialog::on_chkViewPW_clicked(bool checked)
{
    if(checked)
        ui->edtUserPw->setEchoMode(QLineEdit::Normal);
    else
        ui->edtUserPw->setEchoMode(QLineEdit::PasswordEchoOnEdit);
}

/**
 * @brief 单击用户禁用权限
 * @param item
 * @param column
 */
void SecConDialog::userRightItemClicked(QTreeWidgetItem *item, int column)
{
    if(column != 0 || !item->flags().testFlag(Qt::ItemIsEnabled))
        return;
    if(item->checkState(0) == Qt::Checked)
        item->setForeground(0,disColor);
    else
        item->setForeground(0,enColor);
}
