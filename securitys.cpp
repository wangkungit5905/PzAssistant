#include "securitys.h"
#include "global.h"

QHash<int,User*> allUsers;
QHash<int,RightType*> allRightTypes;
QHash<int,Right*> allRights;
QHash<int,UserGroup*> allGroups;
QHash<int,Operate*> allOperates;


bool rightTypeByCode(RightType *rt1, RightType *rt2)
{
    return rt1->code < rt2->code;
}

bool rightByCode(Right *r1, Right *r2)
{
    return r1->getCode() < r2->getCode();
}

bool groupByCode(UserGroup* g1, UserGroup* g2)
{
    return g1->getGroupCode() < g2->getGroupCode();
}

bool userByCode(User* u1, User* u2)
{
    return u1->getUserId() < u2->getUserId();
}

//初始化用户、组、权限和操作
bool initSecurity()
{
    AppConfig* appCon = AppConfig::getInstance();
    bool r = appCon->getRightTypes(allRightTypes);
    if(r)
        r = appCon->getRights(allRights);
    if(r)
        r = appCon->getUserGroups(allGroups);
    if(r)
        r = appCon->getUsers(allUsers);
    return r;
}

///////////////////////User类/////////////////////////////////////
User::User(int id, QString name, QString password, QSet<UserGroup*> ownerGroups)
{
    this->id = id;
    this->name = name;
    this->password = password;
    groups = ownerGroups;
    refreshRights();
}

QString User::getName()
{
    return name;
}

void User::setName(QString name)
{
    this->name = name;
}

void User::setPassword(QString password)
{
    this->password = password;
}

//校验密码
bool User::verifyPw(QString password)
{
    if(this->password == password)
        return true;
    else
        return false;
}

//返回用户所属组列表
QSet<UserGroup*> User::getOwnerGroups()
{
    return groups;
}

//设置用户所属组
void User::setOwnerGroups(QSet<UserGroup*> groups)
{
    this->groups = groups;
    refreshRights();
}

/**
 * @brief 返回用户所属组的代码串
 * @return
 */\
QString User::getOwnerGroupCodeList()
{
    QList<UserGroup*> gs = groups.toList();
    qSort(gs.begin(),gs.end(),groupByCode);
    QStringList sl;
    foreach(UserGroup* g, gs)
        sl<<QString::number(g->getGroupCode());
    return sl.join(",");
}

//添加组
void User::addGroup(UserGroup* group)
{
    groups.insert(group);
    refreshRights();
}

//删除组
void User::delGroup(UserGroup* group)
{
    groups.remove(group);
    refreshRights();
}

/**
 * @brief 添加用户权限（该权限可能不属于任何用户所属的组）
 * @param r
 */
void User::addRight(Right *r)
{
    if(rights.contains(r))
        return;
    rights.insert(r);
    extraRights.insert(r);
    //如果添加的权限不属于用户当前所属的任何一个组，则将该权限视为额外权限
//    bool isExtra = true;
//    foreach(UserGroup* g, groups){
//        if(g->hasRight(r)){
//            isExtra = false;
//            break;
//        }
//    }
//    if(isExtra)
//        extraRights.insert(r);
//    rights.insert(r);
}

/**
 * @brief 移除用户的额外权限
 * @param r
 */
//void User::removeExtraRight(Right *r)
//{
//    if(extraRights.contains(r)){
//        extraRights.remove(r);
//        rights.remove(r);
//        return;
//    }
//    //如果要移除的权限是属于用户当前所属的某个组，
//    //则将该组从用户所属组中移除，并将该组的其他权限加入到额外权限集中
//    foreach(UserGroup* g,groups){
//        QSet<Right*> rs = g->getHaveRights();
//        if(rs.contains(r)){
//            delGroup(g);
//            rs.remove(r);
//            extraRights += rs;
//            return;
//        }
//    }
//}

/**
 * @brief 返回额外权限代码列表
 * @return
 */
QString User::getExtraRightCodes()
{
    QSetIterator<Right*> it(extraRights);
    QStringList sl;
    while(it.hasNext())
        sl<<QString::number(it.next()->getCode());
    qSort(sl.begin(),sl.end());
    return sl.join(",");
}

//刷新用户具有的所有权限
void User::refreshRights()
{
    rights.clear();
    foreach(UserGroup* g, groups)
        rights += g->getHaveRights();
    if(!extraRights.isEmpty()){
        foreach(Right* r, extraRights){
            if(rights.contains(r))
                extraRights.remove(r);
            rights.insert(r);
        }
    }
}

//返回用户具有的所有权限
QSet<Right*> User::getAllRights()
{
    return rights;
}

/**
 * @brief 设置用户的所有权限
 * @param rs
 */
void User::setAllRights(QSet<Right *> rs)
{
    rights = rs;
    QSet<Right*> rs_t;
    QSet<UserGroup*> gs;
    //如果用户所属的某个组所拥有的全部权限包含在rs中，
    //则可以继续保留用户与组的所属关系，否则用户就不能属于该组
    //这样处理是为了尽可能保留用户所属组的设置信息
    foreach(UserGroup* g, groups){
        if(rs.contains(g->getHaveRights())){
            gs.insert(g);
            rs_t += g->getHaveRights();
        }
    }
    groups = gs;
    rs -= rs_t;
    if(!rs.isEmpty())
        extraRights = rs;
}

//是否用户具有指定的权限
bool User::haveRight(Right* right)
{
    if(isSuperUser())
        return true;
    else
        return rights.contains(right);
}

//是否用户具有指定的权限集
bool User::haveRights(QSet<Right*> rights)
{
    if(isSuperUser())
        return true;
    else
        return this->rights.contains(rights);
}

/**
 * @brief 是否是超级用户
 * @return
 */
bool User::isSuperUser()
{
    QSetIterator<UserGroup*> it(groups);
    while(it.hasNext()){
        if(it.next()->getGroupCode() == USER_GROUP_ROOT_ID)
            return true;
    }
    return false;
}

/**
 * @brief 是否是管理员
 * @return
 */
bool User::isAdmin()
{
    QSetIterator<UserGroup*> it(groups);
    while(it.hasNext()){
        if(it.next()->getGroupCode() == USER_GROUP_ADMIN_ID)
            return true;
    }
    return false;
}

/**
 * @brief 是否可以访问指定账户
 * @param account
 * @return
 */
bool User::canAccessAccount(Account *account)
{
    if(isSuperUser() || isAdmin())
        return true;
    return accountCodes.contains(account->getCode());
}

/**
 * @brief 返回用户可以访问的账户代码列表
 * @return
 */
QStringList User::getExclusiveAccounts()
{
    QSetIterator<QString> it(accountCodes);
    QStringList sl;
    while(it.hasNext()){
        sl<<it.next();
    }
    qSort(sl.begin(),sl.end());
    return sl;
}

void User::setExclusiveAccounts(QStringList codes)
{
    accountCodes.clear();
    foreach(QString code,codes)
        accountCodes.insert(code);
}

///////////////////////////right类////////////////////////////////////

Right::Right(int code, RightType *type, QString name, QString explain)
{
    this->code = code;
    this->type = type;
    this->name = name;
    this->explain = explain;
}

void Right::setType(RightType* t)
{
    type = t;
}

RightType* Right::getType()
{
    return type;
}

void Right::setCode(int c)
{
    code = c;
}

int Right::getCode()
{
    return code;
}

void Right::setName(QString name)
{
    this->name = name;
}

QString Right::getName()
{
    return name;
}

void Right::setExplain(QString explain)
{
    this->explain = explain;
}

QString Right::getExplain()
{
    return explain;
}


//////////////////UserGroup类////////////////////////////////////////////
UserGroup::UserGroup(int id, int code, QString name, QSet<Right*> haveRights)
{
    this->id = id;
    this->code = code;
    this->name = name;
    if(haveRights.count() > 0)
        this->rights = haveRights;
}

//返回组名
QString UserGroup::getName()
{
    return name;
}

//设置组名
void UserGroup::setName(QString name)
{
    this->name = name;
}

//发回组具有的权限列表
QSet<Right*> UserGroup::getHaveRights()
{
    return rights;
}

/**
 * @brief 返回组所拥有的所有权限的代码的字符串，代码之间用逗号分隔
 * @return
 */
QString UserGroup::getRightCodeList()
{
    QList<Right*> rs = rights.toList();
    qSort(rs.begin(),rs.end(),rightByCode);
    QStringList sl;
    foreach(Right* r, rs)
        sl<<QString::number(r->getCode());
    return sl.join(",");
}

//设置组具有的权限列表
void UserGroup::setHaveRights(QSet<Right*> rights)
{
    if(rights.count() > 0)
        this->rights = rights;
}

//增加组权限
void UserGroup::addRight(Right* right)
{
    rights.insert(right);
}

//删除组权限
void UserGroup::delRight(Right* right)
{
    rights.remove(right);
}

/**
 * @brief 返回组代码
 */
int UserGroup::getGroupCode()
{
    return code;
}

/**
 * @brief 组是否用于指定权限r（组本身具有的权限和其所属组拥有的权限）
 * @param r
 * @return
 */
bool UserGroup::hasRight(Right *r)
{
    if(rights.contains(r))
        return true;
//    QSetIterator<UserGroup*> it(ownerGroups);
//    while(it.hasNext()){
//        if(it.next()->hasRight(r))
//            return true;
//    }
    return false;
}


////////////////////////////////Operate类//////////////////////////////////
Operate::Operate(int code, QString name, QSet<Right*> rights)
{
    this->code = code;
    this->name = name;
    this->rights = rights;
}

void Operate::setCode(int c)
{
    code = c;
}

int Operate::getCode()
{
    return code;
}

void Operate::setName(QString name)
{
    this->name = name;
}

QString Operate::getName()
{
    return name;
}

QSet<Right*> Operate::getRights()
{
    return rights;
}

void Operate::setRight(QSet<Right*> rights)
{
    this->rights = rights;
}

//判断某用户是否具备执行该操作所要求的所有权限
bool Operate::isPermition(User* user)
{
    QSet<Right*> userRights = user->getAllRights();
    QSetIterator<Right*> it(rights);
    while(it.hasNext()){
        if(!userRights.contains(it.next()))
            return false;        
    }
    return true;
}




