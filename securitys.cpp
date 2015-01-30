#include "securitys.h"
#include "global.h"

#include <QBuffer>
#include <QTextStream>

QHash<int,User*> allUsers;
QHash<int,RightType*> allRightTypes;
QHash<int,Right*> allRights;
QHash<int,UserGroup*> allGroups;
QHash<int,Operate*> allOperates;


RightType *RightType::serialFromText(QString serialText, const QHash<int, RightType *>& rightTypes)
{
    QStringList sl = serialText.split("||");
    if(sl.count() != 4)
        return 0;
    bool ok = false;
    int c = sl.at(SOFI_RT_CODE).toInt(&ok);
    if(!ok)
        return 0;
    int pcode = sl.at(SOFI_RT_PC).toInt(&ok);
    RightType* pt = rightTypes.value(pcode);
    if(!ok || ok && !pt && pcode!=0)
        return 0;
    RightType* rt = new RightType;
    rt->code = c; rt->pType = pt;
    rt->name = sl.at(SOFI_RT_NAME); rt->explain = sl.at(SOFI_RT_DESC);
    return rt;
}

QString RightType::serialToText()
{
    QStringList sl;
    for(int i = 0; i < 4; ++i)
        sl<<"";
    sl[SOFI_RT_CODE] = QString::number(code);
    sl[SOFI_RT_PC] = QString::number((pType?pType->code:0));
    sl[SOFI_RT_NAME] = name;
    sl[SOFI_RT_DESC] = explain;
    return sl.join("||");
}

/**
 * @brief 序列化所有权限类型对象到字节数组中
 * @param ds
 */
void RightType::serialAllToBinary(int mv, int sv, QByteArray *ds)
{
    QList<RightType*> rts = allRightTypes.values();
    qSort(rts.begin(),rts.end(),rightTypeByCode);
    QBuffer bf(ds);
    QTextStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    out<<QString("version=%1.%2\n").arg(mv).arg(sv);
    foreach(RightType* rt, rts)
        out<<rt->serialToText()<<"\n";
    bf.close();
}

bool RightType::serialAllFromBinary(QList<RightType *> &rts, int &mv, int &sv, QByteArray *ds)
{
    QBuffer bf(ds);
    QTextStream in(&bf);
    bf.open(QIODevice::ReadOnly);
    QStringList sl = in.readLine().split("=");
    if(sl.count() != 2)
        return false;
    sl = sl.at(1).split(".");
    if(sl.count() != 2)
        return false;
    bool ok;
    mv = sl.at(0).toInt(&ok);
    if(!ok)
        return false;
    sv = sl.at(1).toInt(&ok);
    if(!ok)
        return false;

    QHash<int, RightType *> rightTypes;
    while(!in.atEnd()){
        RightType* rt = serialFromText(in.readLine(),rightTypes);
        if(!rt)
            return false;
        rts<<rt;
        rightTypes[rt->code] = rt;
    }
    return true;
}

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
    bool r = appCon->initRightTypes(allRightTypes);
    if(r)
        r = appCon->initRights(allRights);
    if(r)
        r = appCon->initUserGroups(allGroups);
    if(r)
        r = appCon->initUsers(allUsers);
    return r;
}

///////////////////////User类/////////////////////////////////////
User::User(int id, QString name, QString password, QSet<UserGroup*> ownerGroups)
{
    this->id = id;
    enabled = true;
    this->name = name;
    this->password = password;
    groups = ownerGroups;
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

/**
 * @brief 返回用户禁用权限代码列表串
 * @return
 */
QString User::getDisRightCodes()
{
    QStringList ls;
    foreach(Right* r,disRights)
        ls.append(QString::number(r->getCode()));
    qSort(ls.begin(),ls.end());
    return ls.join(",");
}

//返回用户具有的所有权限
QSet<Right*> User::getAllRights()
{
    QSet<Right*> rs;
    foreach(UserGroup* g,groups)
        rs += g->getAllRights();
    rs -= disRights;
    return rs;
}

//是否用户具有指定的权限
bool User::haveRight(Right* right)
{
    if(isSuperUser())
        return true;
    if(disRights.contains(right))
        return false;
    foreach(UserGroup* g, groups){
        if(g->hasRight(right))
            return true;
    }
    return false;
}

//是否用户具有指定的权限集
bool User::haveRights(QSet<Right*> rights)
{
    if(isSuperUser())
        return true;
    QSet<Right*> rs = getAllRights();
    return rs.contains(rights);
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

QString User::serialToText()
{
    QStringList ls;
    for(int i = 0; i < 7; ++i)
        ls<<"";
    ls[SOFI_USER_CODE] = QString::number(id);
    ls[SOFI_USER_ISENABLED] = (enabled?"1":"0");
    ls[SOFI_USER_NAME] = name;
    ls[SOFI_USER_PASSWORD] = password;
    ls[SOFI_USER_GROUPS] = getOwnerGroupCodeList();
    ls[SOFI_USER_ACCOUNTS] = getExclusiveAccounts().join(",");
    ls[SOFI_USER_DISRIGHTS] = getDisRightCodes();
    return ls.join("||");
}

User *User::serialFromText(QString serialText,const QHash<int,Right*> &rights, const QHash<int,UserGroup*> &groups)
{
    QStringList sl = serialText.split("||");
    if(sl.count() != 7)
        return 0;
    bool ok;
    int c = sl.at(SOFI_USER_CODE).toInt(&ok);
    if(!ok)
        return 0;
    bool isEnable = (sl.at(SOFI_USER_ISENABLED) == "0")?false:true;
    QSet<UserGroup*> gs;
    if(!sl.at(SOFI_USER_GROUPS).isEmpty()){
        QStringList ls = sl.at(SOFI_USER_GROUPS).split(",");
        foreach(QString s, ls){
            bool ok;
            int gc = s.toInt(&ok);
            UserGroup* g = groups.value(gc);
            if(!ok || ok && !g){
                LOG_SQLERROR(QString("Create User object from text failed! Reason is group code invalid, text = '%1'").arg(serialText));
                continue;
            }
            gs<<g;
        }
    }
    User* u = new User(c,sl.at(SOFI_USER_NAME),sl.at(SOFI_USER_PASSWORD),gs);
    u->setEnabled(isEnable);
    if(!sl.at(SOFI_USER_ACCOUNTS).isEmpty())
        u->setExclusiveAccounts(sl.at(SOFI_USER_ACCOUNTS).split(","));
    if(!sl.at(SOFI_USER_DISRIGHTS).isEmpty()){
        QStringList ls = sl.at(SOFI_USER_DISRIGHTS).split(",");
        QSet<Right*> rs;
        foreach(QString s, ls){
            bool ok;
            int rc = s.toInt(&ok);
            Right* r = rights.value(rc);
            if(!ok || ok && !r){
                LOG_ERROR(QString("Create User object from text failed! Reason is right code invalid, text = '%1'").arg(serialText));
                continue;
            }
            rs<<r;
        }
        u->setAllDisRights(rs);
    }
    //allUsers[c] = u;
    return u;
}

void User::serialAllToBinary(int mv, int sv, QByteArray *ds)
{
    QList<User*> us = allUsers.values();
    qSort(us.begin(),us.end(),userByCode);
    QBuffer bf(ds);
    QTextStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    out<<QString("version=%1.%2\n").arg(mv).arg(sv);
    foreach(User* u, us)
        out<<u->serialToText()<<"\n";
    bf.close();
}

bool User::serialAllFromBinary(QList<User *> &users, int &mv, int &sv, QByteArray *ds,const QHash<int,Right*> &rights, const QHash<int,UserGroup*> &groups)
{
    QBuffer bf(ds);
    QTextStream in(&bf);
    bf.open(QIODevice::ReadOnly);
    QStringList sl = in.readLine().split("=");
    if(sl.count() != 2)
        return false;
    sl = sl.at(1).split(".");
    if(sl.count() != 2)
        return false;
    bool ok;
    mv = sl.at(0).toInt(&ok);
    if(!ok)
        return false;
    sv = sl.at(1).toInt(&ok);
    if(!ok)
        return false;

    while(!in.atEnd()){
        User* u = serialFromText(in.readLine(),rights,groups);
        if(!u)
            return false;
        users<<u;
    }
    return true;
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

QString Right::serialToText()
{
    QStringList sl;
    for(int i = 0; i < 4; ++i)
        sl<<"";
    sl[SOFI_RIGHT_CODE] = QString::number(code);
    sl[SOFI_RIGHT_RT] = QString::number((type?type->code:0));
    sl[SOFI_RIGHT_NAEM] = name;
    sl[SOFI_RIGHT_DESC] = explain;
    return sl.join("||");
}

Right *Right::serialFromText(QString serialText, const QHash<int, RightType *> &rightTypes)
{
    QStringList ls = serialText.split("||");
    if(ls.count() != 4)
        return 0;
    bool ok;
    int rc = ls.at(SOFI_RIGHT_CODE).toInt(&ok);
    if(!ok)
        return 0;
    int rtc = ls.at(SOFI_RIGHT_RT).toInt(&ok);
    RightType* rt = rightTypes.value(rtc);
    if(!ok || ok && !rt)
        LOG_ERROR(QString("Create Right Object failed! Reason is right type code invalid! text is '%1'").arg(serialText));
    Right* r = new Right(rc,rt,ls.at(SOFI_RIGHT_NAEM),ls.at(SOFI_RIGHT_DESC));
    return r;
}

void Right::serialAllToBinary(int mv, int sv, QByteArray *ds)
{
    QList<Right*> rs = allRights.values();
    qSort(rs.begin(),rs.end(),rightByCode);
    QBuffer bf(ds);
    QTextStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    out<<QString("version=%1.%2\n").arg(mv).arg(sv);
    foreach(Right* r, rs)
        out<<r->serialToText()<<"\n";
    bf.close();
}

bool Right::serialAllFromBinary(const QHash<int, RightType *> &rightTypes, QList<Right *> &rs, int &mv, int &sv, QByteArray *ds)
{
    QBuffer bf(ds);
    QTextStream in(&bf);
    bf.open(QIODevice::ReadOnly);
    QStringList sl = in.readLine().split("=");
    if(sl.count() != 2)
        return false;
    sl = sl.at(1).split(".");
    if(sl.count() != 2)
        return false;
    bool ok;
    mv = sl.at(0).toInt(&ok);
    if(!ok)
        return false;
    sv = sl.at(1).toInt(&ok);
    if(!ok)
        return false;

    while(!in.atEnd()){
        Right* r = serialFromText(in.readLine(),rightTypes);
        if(!r)
            return false;
        rs<<r;
    }
    return true;
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

//返回组具有的不属于任何所属组的权限列表（即额外权限）
QSet<Right*> UserGroup::getExtraRights()
{
    return rights;
}

/**
 * @brief UserGroup::getAllRights
 * 返回组所拥有的所有权限
 * @return
 */
QSet<Right *> UserGroup::getAllRights()
{
    QSet<Right*> rs;
    foreach(UserGroup* g, ownerGroups)
        rs += g->getAllRights();
    return rs + rights;
}

/**
 * @brief 返回组所拥有的额外权限的代码的字符串，代码之间用逗号分隔
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
    this->rights = rights;
}

//增加组额外权限
void UserGroup::addRight(Right* right)
{
    foreach(UserGroup* g, ownerGroups){
        if(g->hasRight(right))
            return;
    }
    rights.insert(right);
}

//删除组额外权限
void UserGroup::removeRight(Right* right)
{
    rights.remove(right);
}

QString UserGroup::getOwnerCodeList()
{
    QList<UserGroup*> gs = ownerGroups.toList();
    qSort(gs.begin(),gs.end(),groupByCode);
    QStringList sl;
    foreach(UserGroup* g, gs)
        sl<<QString::number(g->getGroupCode());
    return sl.join(",");
}

/**
 * @brief UserGroup::isGroupRight
 * 判断指定权限是否是所属组中某个组拥有的权限
 * @param r
 * @return
 */
bool UserGroup::isGroupRight(Right *r)
{
    foreach(UserGroup* g, ownerGroups){
        if(g->hasRight(r))
            return true;
    }
    return false;
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
    foreach(UserGroup* g, ownerGroups){
        if(g->hasRight(r))
            return true;
    }
    return false;
}

QString UserGroup::serialToText()
{
    QStringList ls;
    for(int i = 0; i < 5; ++i)
        ls<<"";
    ls[SOFI_GROUP_CODE] = QString::number(code);
    ls[SOFI_GROUP_NAME] = name;
    ls[SOFI_GROUP_RIGHTS] = getRightCodeList();
    ls[SOFI_GROUP_OWNER] = getOwnerCodeList();
    ls[SOFI_GROUP_DESC] = explain;
    return ls.join("||");
}

UserGroup *UserGroup::serialFromText(QString serialText,const QHash<int, Right*> &rights, const QHash<int,UserGroup*> &groups)
{
    QStringList ls = serialText.split("||");
    if(ls.count() != 5)
        return 0;
    bool ok;
    int gc = ls.at(SOFI_GROUP_CODE).toInt(&ok);
    if(!ok)
        return 0;
    QSet<Right*> rs;
    if(!ls.at(SOFI_GROUP_RIGHTS).isEmpty()){
        QStringList sl = ls.at(SOFI_GROUP_RIGHTS).split(",");
        foreach(QString s, sl){
            bool ok;
            int rc = s.toInt(&ok);
            Right* r = rights.value(rc);
            if(!ok || ok && !r){
                LOG_ERROR(QString("Create Group Object failed from serial text! Reason is right code invalid. text is '%1'").arg(serialText));
                continue;
            }
            rs<<r;
        }
    }
    UserGroup* ng = new UserGroup(UNID,gc,ls.at(SOFI_GROUP_NAME),rs);
    if(!ls.at(SOFI_GROUP_OWNER).isEmpty()){
        QSet<UserGroup*> gs;
        QStringList sl = ls.at(SOFI_GROUP_OWNER).split(",");
        foreach(QString s, sl){
            bool ok;
            int gc = s.toInt(&ok);
            UserGroup* g = groups.value(gc);
            if(!ok || ok && !g){
                LOG_ERROR(QString("Create Group Object failed from serial text! Reason is owner group code invalid. text is '%1'").arg(serialText));
                continue;
            }
            gs<<g;
        }
        ng->setOwnerGroups(gs);
    }
    return ng;
}

void UserGroup::serialAllToBinary(int mv, int sv, QByteArray *ds)
{
    QList<UserGroup*> gs = allGroups.values();
    qSort(gs.begin(),gs.end(),groupByCode);
    QBuffer bf(ds);
    QTextStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    out<<QString("version=%1.%2\n").arg(mv).arg(sv);
    foreach(UserGroup* g, gs)
        out<<g->serialToText()<<"\n";
    bf.close();
}

bool UserGroup::serialAllFromBinary(QList<UserGroup *> &groups, int &mv, int &sv, QByteArray *ds,const QHash<int, Right*> &rights)
{
    QBuffer bf(ds);
    QTextStream in(&bf);
    bf.open(QIODevice::ReadOnly);
    QStringList sl = in.readLine().split("=");
    if(sl.count() != 2)
        return false;
    sl = sl.at(1).split(".");
    if(sl.count() != 2)
        return false;
    bool ok;
    mv = sl.at(0).toInt(&ok);
    if(!ok)
        return false;
    sv = sl.at(1).toInt(&ok);
    if(!ok)
        return false;

    QHash<int,UserGroup*> gHashs;
    while(!in.atEnd()){
        UserGroup* g = serialFromText(in.readLine(),rights,gHashs);
        if(!g)
            return false;
        groups<<g;
        gHashs[g->getGroupCode()] = g;
    }
    return true;
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


///////////////////////////////////////////////////////////////////////
WorkStation::WorkStation(int id, MachineType type, int mid, bool isLocal, QString name, QString desc,int osType)
    :id(id),type(type),mid(mid),isLocal(isLocal),sname(name),desc(desc),_osType(osType)
{
}

WorkStation::WorkStation(WorkStation &other)
{
    id = other.id;
    type = other.type;
    mid = other.mid;
    isLocal = other.isLocal;
    sname = other.sname;
    desc = other.desc;
    _osType = other._osType;
}

/**
 * @brief 序列化对象到文本
 * @return
 */
QString WorkStation::serialToText()
{
    QStringList ls;
    for(int i = 0; i < 6; ++i)
        ls<<"";
    ls[SOFI_WS_MID] = QString::number(mid);
    ls[SOFI_WS_TYPE] = QString::number((int)type);
    ls[SOFI_WS_ISLOCAL] = (isLocal?"1":"0");
    ls[SOFI_WS_NAME] = sname;
    ls[SOFI_WS_DESC] = desc;
    ls[SOFI_WS_OSTYPE] = QString::number(_osType);
    return ls.join("||");
}

WorkStation *WorkStation::serialFromText(QString serialText)
{
    QStringList ls = serialText.split("||");
    if(ls.count() != 6)
        return 0;
    bool ok;
    int mid = ls.at(SOFI_WS_MID).toInt(&ok);
    if(!ok)
        return 0;
    MachineType type = (MachineType)ls.at(SOFI_WS_TYPE).toInt(&ok);
    if(!ok)
        return 0;
    if(type != MT_COMPUTER && type != MT_COMPUTER){
        LOG_ERROR(QString("Create Workstation Object failed! Reason is type code invalid! text is '%1'").arg(serialText));
        return 0;
    }
    bool isLocal = (ls.at(SOFI_WS_ISLOCAL) == "0")?false:true;
    int osType = ls.at(SOFI_WS_OSTYPE).toInt(&ok);
    if(!ok)
        LOG_ERROR(QString("Create Workstation Object have Warning! Reason is Operate System code invalid! text is '%1'").arg(serialText));
    WorkStation* mac = new WorkStation(UNID,type,mid,isLocal,ls.at(SOFI_WS_NAME),ls.at(SOFI_WS_DESC),osType);
    return mac;
}

void WorkStation::serialAllToBinary(int mv, int sv, QByteArray *ds)
{
    QList<WorkStation*> macs = AppConfig::getInstance()->getAllMachines().values();
    qSort(macs.begin(),macs.end(),byMacMID);
    QBuffer bf(ds);
    QTextStream out(&bf);
    bf.open(QIODevice::WriteOnly);
    out<<QString("version=%1.%2\n").arg(mv).arg(sv);
    foreach(WorkStation* m, macs)
        out<<m->serialToText()<<"\n";
    bf.close();
}

bool WorkStation::serialAllFromBinary(QList<WorkStation *> &macs, int &mv, int &sv, QByteArray *ds)
{
    QBuffer bf(ds);
    QTextStream in(&bf);
    bf.open(QIODevice::ReadOnly);
    QStringList sl = in.readLine().split("=");
    if(sl.count() != 2)
        return false;
    sl = sl.at(1).split(".");
    if(sl.count() != 2)
        return false;
    bool ok;
    mv = sl.at(0).toInt(&ok);
    if(!ok)
        return false;
    sv = sl.at(1).toInt(&ok);
    if(!ok)
        return false;

    while(!in.atEnd()){
        WorkStation* m = serialFromText(in.readLine());
        if(!m)
            return false;
        macs<<m;
    }
    return true;
}

bool WorkStation::operator ==(const WorkStation &other) const
{
    if(mid != other.mid)
        return false;
    if( type != other.type || _osType != other._osType ||
           sname != other.sname || desc != other.desc)
        return false;
    return true;
}

bool WorkStation::operator !=(const WorkStation &other) const
{
    return !(*this == other);
}

bool byMacMID(WorkStation *mac1, WorkStation *mac2)
{
    return mac1->getMID() < mac2->getMID();
}




