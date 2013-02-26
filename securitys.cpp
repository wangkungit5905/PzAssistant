#include "securitys.h"
#include "global.h"

//QSqlDatabase bdb;
QHash<int,User*> allUsers;
QHash<int,QString> allRightTypes;
QHash<int,Right*> allRights;
QHash<int,UserGroup*> allGroups;
QHash<int,Operate*> allOperates;

//初始化用户、组、权限和操作
bool initSecurity()
{
    bool r = false;
    QSqlQuery q(bdb);

    r = q.exec("select * from rightType");
    while(q.next()){
        int code = q.value(2).toInt();
        QString name = q.value(3).toString();
        allRightTypes[code] = name;
    }

    r = q.exec("select * from rights");
    while(q.next()){
        int code = q.value(1).toInt();
        int type = q.value(2).toInt();
        QString name = q.value(3).toString();
        QString explain = q.value(4).toString();
        Right* right = new Right(code,type,name,explain);
        allRights[code] = right;
    }

    UserGroup* group;
    r = q.exec("select * from groups");
    while(q.next()){
        int code = q.value(1).toInt();        
        QString name = q.value(2).toString();
        if(code == 1){ //超级用户组
            group = new UserGroup(1, name);
            allGroups[1] = group;
            continue;
        }
        QString rs = q.value(3).toString();        
        if(rs != ""){
            QStringList rl = rs.split(",");
            QSet<Right*> haveRights;
            for(int i = 0; i < rl.count(); ++i){
                haveRights.insert(allRights.value(rl[i].toInt()));
            }
            group = new UserGroup(code, name, haveRights);
            allGroups[code] = group;
        }

    }

    r = q.exec("select * from users");
    while(q.next()){
        int id = q.value(0).toInt();
        QString name = q.value(1).toString();
        QString pw = q.value(2).toString();
        QString gs = q.value(3).toString();
        if(gs != ""){
            QStringList gr = gs.split(",");
            QSet<UserGroup*> groups;
            for(int i = 0; i < gr.count(); ++i){
                groups.insert(allGroups.value(gr[i].toInt()));
            }
            User* user = new User(id, name, pw, groups);
            allUsers[id] = user;
        }
    }

    r = q.exec("select * from permitions");
    while(q.next()){
        int code = q.value(1).toInt();
        QString name = q.value(2).toString();
        QString rst = q.value(3).toString();
        if(rst != ""){
            QSet<Right*> rights;
            QStringList rs = rst.split(",");
            for(int i = 0; i < rs.count(); ++i){
                rights.insert(allRights.value(rs[i].toInt()));
            }
            Operate* op = new Operate(code, name, rights);
            allOperates[code] = op;
        }
    }

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
    this->password = encryptPw(password);
}

//校验密码
bool User::verifyPw(QString password)
{
    QString pw = encryptPw(password);
    if(this->password == pw)
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

//刷新用户具有的所有权限
void User::refreshRights()
{
    rights.clear();
    QSetIterator<UserGroup*> it(groups);
    while(it.hasNext()){
        rights = rights + it.next()->getHaveRights();
    }
}

//返回用户具有的所有权限
QSet<Right*> User::getAllRight()
{
    return rights;
}

//是否用户具有指定的权限
bool User::haveRight(Right* right)
{
    if(id == 1)
        return true;
    else
        return rights.contains(right);
}

//是否用户具有指定的权限集
bool User::haveRights(QSet<Right*> rights)
{
    return this->rights.contains(rights);
}

///////////////////////////right类////////////////////////////////////

Right::Right(int code, int type, QString name, QString explain)
{
    this->code = code;
    this->type = type;
    this->name = name;
    this->explain = explain;
}

void Right::setType(int t)
{
    type = t;
}

int Right::getType()
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
UserGroup::UserGroup(int code, QString name, QSet<Right*> haveRights)
{
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
    QSet<Right*> userRights = user->getAllRight();
    QSetIterator<Right*> it(rights);
    while(it.hasNext()){
        if(!userRights.contains(it.next()))
            return false;        
    }
    return true;
}
