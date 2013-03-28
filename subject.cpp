
#include "subject.h"
#include "logs/Logger.h"
#include "dbutil.h"
#include "global.h"

QHash<int,QStringList> SubjectManager::nameItemCls;
QHash<int,SubjectNameItem*> SubjectManager::nameItems;

//虚拟科目对象
FirstSubject* FS_ALL;
FirstSubject* FS_NULL;
SubjectNameItem* NI_ALL;
SubjectNameItem* NI_NULL;
SecondSubject* SS_ALL;
SecondSubject* SS_NULL;


/////////////////////////////////FirstSubject///////////////////////////

FirstSubject::FirstSubject(const FirstSubject &other)
{
    md = FSTSUBMD++;
    id = other.id;  //应该另外创建id？
    name = other.name;
    code = other.code;
    weight = other.weight;
    jdDir = other.jdDir;
    isUseWb = other.isUseWb;
    editState = other.editState;
    remCode = other.remCode;
    subClass = other.subClass;
    briefExplain = other.briefExplain;
    usage = other.usage;
    subSys = other.subSys;
    childSubs = other.childSubs;
    defSub = other.defSub;
}

FirstSubject::FirstSubject(int id, int subcls, QString subName, QString subCode, QString remCode,
            int subWeight, bool jdDir, bool isUseWb, QString explain, QString usage, int subSys):
    SubjectBase(),md(FSTSUBMD++),id(id),subClass(subcls),name(subName),code(subCode),remCode(remCode),
    weight(subWeight),isEnable(isEnable),jdDir(jdDir),isUseWb(isUseWb),
    briefExplain(explain),usage(usage),subSys(subSys),defSub(NULL)
{
}

FirstSubject::~FirstSubject()
{
    //要删除其下的二级科目对象
}

void FirstSubject::setCode(QString subCode)
{
    QString c = subCode.trimmed();
    if(code != c){
        code = c;
        witchEdited|=ES_FS_CODE;
    }
}

void FirstSubject::setWeight(int subWeight)
{
    if(subWeight!= weight)
    {
        weight = subWeight;
        witchEdited|=ES_FS_WEIGHT;
    }
}

void FirstSubject::setEnabled(bool en)
{
    if(!en && isEnable || en && !isEnable)
    {
        isEnable = en;
        witchEdited |= ES_FS_ISENABLED;
    }
}

void FirstSubject::setName(QString subName)
{
    QString n = subName.trimmed();
    if(n!=name){
        name = subName;
        witchEdited |= ES_FS_NAME;
    }
}

void FirstSubject::setRemCode(QString code)
{
    QString c = code.trimmed();
    if(c != remCode){
       remCode=code;
       witchEdited |= ES_FS_SYMBOL;
    }
}

void FirstSubject::setJdDir(bool dir)
{
    if(!dir && jdDir || dir && !jdDir){
       jdDir=dir;
       witchEdited |= ES_FS_JDDIR;
    }
}

void FirstSubject::setIsUseForeignMoney(bool isUse)
{
    if(isUseWb ^ isUse){
        isUseWb = isUse;
        witchEdited |= ES_FS_ISUSEWB;
    }
}

void FirstSubject::setSubClass(int c)
{
    if(subClass != c){
        subClass=c;
        witchEdited = ES_FS_CLASS;
    }
}

void FirstSubject::setBriefExplain(QString s)
{
    QString desc = s.trimmed();
    if(desc != briefExplain){
        briefExplain = desc;
        witchEdited |= ES_FS_DESC;
    }
}

void FirstSubject::setUsage(QString s)
{
    QString u = s.trimmed();
    if(usage != u){
        usage = u;
        witchEdited |= ES_FS_USAGE;
    }
}

/**
 * @brief FirstSubject::addChildSub
 *  利用已有的名称条目添加新子目
 * @param name      子目所使用的名称条目对象
 * @param Code      子目的代码
 * @param subWeight 子目权重
 * @param isEnable  子目是否启用
 * @param isInit    是否在科目管理器的初始化阶段调用此方法
 * @return
 */
SecondSubject *FirstSubject::addChildSub(SubjectNameItem* name,QString Code,int subWeight,bool isEnable,
                                         QDateTime crtTime,User* crtUser)
{
    if(!name)
        return NULL;
    foreach(SecondSubject* sub, childSubs)
        if(sub->getNameItem() == name)
            return NULL;
    SecondSubject* sb = new SecondSubject(this,0,name,Code,subWeight,isEnable,crtTime,QDateTime(),curUser);
    childSubs<<sb;
    witchEdited |= ES_FS_CHILD;
    return sb;
}

//void FirstSubject::addChildSub(SecondSubject *sub, bool isInit)
//{
//    if(sub){
//        childSubs<<sub;
//        sub->setParent(this);
//        if(!isInit)
//            witchEdited |= ES_FS_CHILD;
//    }
//}

/**
 * @brief FirstSubject::restoreChildSub
 *  恢复被删除的子目对象（被删除的子目，如果为执行保存操作，则仍保留在删除队列中）
 * @param sub
 * @return
 */
SecondSubject *FirstSubject::restoreChildSub(SecondSubject *sub)
{
    for(int i = 0; i< delSubs.count(); ++i){
        if(delSubs.at(i) == sub){
            sub->setDelete(false);
            childSubs<<delSubs.takeAt(i);
            witchEdited |= ES_FS_CHILD;
            return childSubs.last();
        }
    }
    return NULL;
}

/**
 * @brief FirstSubject::removeChildSub
 *  移除子目
 * @param sub
 * @return
 */
bool FirstSubject::removeChildSub(SecondSubject *sub)
{
    if(!sub)
        return false;
    if(!childSubs.removeOne(sub))
        return false;
    delSubs<<sub;
    sub->setDelete(true);
    witchEdited |= ES_FS_CHILD;
    return true;
}


/**
 * @brief FirstSubject::getRangeChildSubs
 *  返回指定范围的子目对象
 * @param ssub：起始科目
 * @param esub：终止科目
 * @param subs：在范围内的科目
 * @return
 */
bool FirstSubject::getRangeChildSubs(SecondSubject *ssub, SecondSubject *esub, QList<SecondSubject *> &subs)
{
    if(childSubs.empty())
        return true;
    SecondSubject* sub = childSubs.first();
    int count = childSubs.count();
    int si;
    if(!ssub)
        si = 0;
    else{
        si = 1;
        while(sub != ssub && si < count)
            sub = childSubs.at(si++);
    }

    int ei;
    if(!esub)
        ei = count-1;
    else{
        ei = si;
        while(sub != esub && si < count)
            sub = childSubs.at(ei++);
    }
    if(si == count || ei == count){
        LOG_ERROR("Index overflow in function FirstSubject::getRangeChildSubs()");
        return false;
    }
    for(int i = si; i <= ei; i++)
        subs<<childSubs.at(i);
    return true;
}

/**
 * @brief FirstSubject::containChildSub
 *  是否包含指定的子目
 * @param ni
 * @return
 */
bool FirstSubject::containChildSub(SecondSubject *sndSub)
{
    if(!sndSub)
        return false;
    return childSubs.contains(sndSub);
}

/**
 * @brief FirstSubject::containChildSub
 *  是否包含使用了指定名称条目的子目对象
 * @param ni
 * @return
 */
bool FirstSubject::containChildSub(SubjectNameItem *ni)
{
    if(!ni)
        return false;
    for(int i = 0; i < childSubs.count(); ++i)
        if(ni == childSubs.at(i)->getNameItem())
            return true;
    return false;
}

/**
 * @brief FirstSubject::getChildSub
 *  返回该一级科目下使用了指定名称条目的二级科目对象
 * @param ni
 * @return
 */
SecondSubject *FirstSubject::getChildSub(SubjectNameItem *ni)
{
    for(int i = 0; i < childSubs.count(); ++i){
        if(ni == childSubs.at(i)->getNameItem())
            return childSubs.at(i);
    }
    return NULL;
}


//针对一级科目的排序比较函数
bool byNameThan_fs(FirstSubject *fs1, FirstSubject *fs2)
{return fs1->getName() < fs2->getName();}
bool bySubCodeThan_fs(FirstSubject *fs1, FirstSubject *fs2)
{return fs1->getCode() < fs2->getCode();}
bool byRemCodeThan_fs(FirstSubject *fs1, FirstSubject *fs2)
{return fs1->getRemCode() < fs2->getRemCode();}

//针对名称条目的排序比较函数
bool byRemCodeThan_ni(SubjectNameItem *ni1, SubjectNameItem *ni2)
{return ni1->getRemCode() < ni2->getRemCode();}
bool byNameThan_ni(SubjectNameItem *ni1, SubjectNameItem *ni2)
{return ni1->getShortName() < ni2->getShortName();}

//针对二级科目的排序比较函数
bool bySubCodeThan_ss(SecondSubject *ss1, SecondSubject *ss2)
{return ss1->getCode() < ss2->getCode();}
bool byRemCodeThan_ss(SecondSubject *ss1, SecondSubject *ss2)
{return ss1->getRemCode() < ss2->getRemCode();}
bool bySubNameThan_ss(SecondSubject *ss1, SecondSubject *ss2)
{return ss1->getName() < ss2->getName();}



//////////////////////////SubjectNameItem//////////////////////////////////////////
/**
 * @brief SubjectNameItem::SubjectNameItem
 *  此构造函数用于初始化名称条目期间
 * @param id
 * @param cls
 * @param sname
 * @param lname
 * @param remCode
 * @param crtTime
 * @param user
 */
SubjectNameItem::SubjectNameItem(int id,int cls,QString sname,QString lname,QString remCode,
                                 QDateTime crtTime,User* user):
    id(id),clsId(cls),sname(sname),lname(lname),remCode(remCode),crtTime(crtTime),
    crtUser(curUser),isDeleted(false)
{
    md = NAMEITEMMD++;

}

/**
 * @brief SubjectNameItem::SubjectNameItem
 *  在动态生成名称条目时调用
 */
SubjectNameItem::SubjectNameItem():
    id(UNID),clsId(UNCLASS),isDeleted(false),crtTime(QDateTime::currentDateTime()),
        crtUser(curUser)
{
    md=NAMEITEMMD++;
}


SubjectNameItem::SubjectNameItem(const SubjectNameItem &other)
{
    md = NAMEITEMMD++;
    witchEdit = other.witchEdit;
    id = other.id;
    clsId = other.clsId;
    sname = other.sname;
    lname = other.lname;
    remCode = other.remCode;
    crtTime = other.crtTime;
    crtUser = other.crtUser;
    isDeleted = other.isDeleted;
}

void SubjectNameItem::setClassId(int cid)
{
    if(cid != clsId){
        clsId = cid;
        witchEdit |= ES_NI_CLASS;
    }
}

void SubjectNameItem::setShortName(QString name)
{
    QString n = name.trimmed();
    if(sname != n){
        sname = n;
        witchEdit |= ES_NI_SNAME;
    }
}

void SubjectNameItem::setLongName(QString name)
{
    QString n = name.trimmed();
    if(lname != n){
        lname = n;
        witchEdit |= ES_NI_LNAME;
    }
}

void SubjectNameItem::setRemCode(QString code)
{
    QString c = code.trimmed();
    if(remCode != c){
        remCode = c;
        witchEdit |= ES_NI_SYMBOL;
    }
}



/////////////////////////////SecondSubject///////////////////////////////////////////////
/**
 * @brief SecondSubject::SecondSubject
 *  用于在初始化期间、利用已有名称条目在指定一级科目下创建二级科目的时候
 * @param p
 * @param id
 * @param name
 * @param Code
 * @param subWeight
 * @param isEnable
 * @param crtTime
 * @param disTime
 * @param crtUser
 */
SecondSubject::SecondSubject(FirstSubject *p, int id, SubjectNameItem *name, QString Code, int subWeight,
                             bool isEnable, QDateTime crtTime, QDateTime disTime, User *crtUser):
    parent(p),id(id),nItem(name),code(Code),weight(subWeight),isEnable(isEnable),isDeleted(false),
    crtTime(crtTime),disTime(disTime),creator(crtUser)
{
    md=SNDSUBMD++;
}


SecondSubject::SecondSubject(const SecondSubject& other)
{
    md=SNDSUBMD++;
    //id = other.id;
    id = UNID;
    code = other.code;
    weight = other.weight;
    isEnable = other.isEnable;
    parent = other.parent;
    nItem = other.nItem;
    crtTime = other.crtTime;
    disTime = other.disTime;
    creator = other.creator;
    isDeleted = other.isDeleted;
}

/**
 * @brief SecondSubject::SecondSubject
 *  创建新的二级科目对象（在还没有创建名称信息条目对象的情况下调用）
 * @param p
 * @param id
 * @param nameId
 * @param nameClsId
 * @param sName
 * @param lName
 * @param subCode
 * @param remCode
 * @param subWeight
 * @param isEnable
 * @param crtUser
 * @param crtTime
 */
SecondSubject::SecondSubject(FirstSubject* p, int id, int nameId, int nameClsId, QString sName, QString lName, QString subCode, QString remCode,
                             int subWeight, bool isEnable, User *crtUser, QDateTime crtTime):
    parent(p),id(id),isDeleted(false)
{
    //首先要检查给定的名称是否存在，如存在，则使用现存的名称条目的其他信息，如不存在，则创建新的名称条目并使用之
    //待以后实现
    md=SNDSUBMD++;
    nItem = new SubjectNameItem;
    nItem->setClassId(nameClsId);
    nItem->setShortName(sName);
    nItem->setLongName(lName);
    nItem->setRemCode(remCode);
    //nItem
}

SecondSubject::SecondSubject():
    id(UNID),crtTime(QDateTime::currentDateTime()),creator(curUser),isDeleted(false)
{
    md=SNDSUBMD++;
}

void SecondSubject::setCode(QString subCode)
{
    QString c =subCode.trimmed();
    if(c != code){
        code = c;
        witchEdit |= ES_SS_CODE;
        parent->childEdited();
    }
}

void SecondSubject::setWeight(int subWeight)
{
    if(weight != subWeight){
        weight = subWeight;
        witchEdit |= ES_SS_WEIGHT;
        parent->childEdited();
    }
}

void SecondSubject::setEnabled(bool en)
{
    if(en && !isEnable || !en && isEnable){
        isEnable=en;
        witchEdit |= ES_SS_ISENABLED;
        parent->childEdited();
    }
}

////////////////////////////SubjectManager////////////////////////////////////////////
SubjectManager::SubjectManager(Account *account, int subSys):
    account(account),subSys(subSys)
{
    dbUtil = account->getDbUtil();
    init();
}


bool SubjectManager::init()
{
    dbUtil->initSubjects(this,subSys);
}

/**
 * @brief SubjectManager::isSySubject
 *  是损益类科目吗
 * @param sid
 * @return
 */
bool SubjectManager::isSySubject(int sid)
{
    FirstSubject* sub = getFstSubject(sid);
    return sySubCls == sub->getSubClass();
}

