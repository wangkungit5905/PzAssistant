#ifndef SUBJECT_H
#define SUBJECT_H

#include <QHash>
#include <QStringList>

#include "commdatastruct.h"
#include "global.h"



const int SUPERUSERID   = 1;  //超级用户ID
const int CLIENTCLASSID = 2;  //业务客户类别id

const int DEFALUT_SUB_WEIGHT  = 65535; //默认科目的权重
const int INIT_WEIGHT         = 1;     //科目的初始权重

static int FSTSUBMD = 1;     //一级科目对象使用的魔术字
static int NAMEITEMMD = 1;   //名称条目对象使用的魔术字
static int SNDSUBMD = 1;     //二级科目对象使用的魔术字


//class Money;
class Account;
class SecondSubject;
class FirstSubject;
class DbUtil;
struct SubjectNameItem;

//虚拟科目对象
//extern FirstSubject* FS_ALL;    //表示所有一级科目（md=1）
//extern FirstSubject* FS_NULL;   //表示空一级科目（即没有设置）（md=2）
//extern SubjectNameItem* NI_ALL; //表示“所有或全部”意思的名称条目（md=1）
//extern SubjectNameItem* NI_NULL;//表示空（md=2）
//extern SecondSubject* SS_ALL;   //表示任意一级科目下的所有二级科目（md=1）
//extern SecondSubject* SS_NULL;  //表示空二级科目（md=2）

/**
 *@brief Subject Class
 *会计科目基类
 */
class SubjectBase{
public:
    SubjectBase(){}

    virtual int getId()=0;
    virtual QString getCode()=0;
    virtual void setCode(QString subCode)=0;
    virtual int getWeight()=0;
    virtual void setWeight(int subWeight)=0;
    virtual bool isEnabled()=0;
    virtual void setEnabled(bool en)=0;
    virtual QString getName()=0;
    virtual void setName(QString subName)=0;
    virtual QString getRemCode()=0;
    virtual void setRemCode(QString code)=0;
    virtual bool getJdDir()=0;
    virtual bool isUseForeignMoney()=0;
    virtual void setDelete(bool isDeleted)=0;
    virtual bool isDelete()=0;
};
Q_DECLARE_METATYPE(SubjectBase*)



class FirstSubject : public SubjectBase{
public:
    FirstSubject():md(FSTSUBMD++),id(0),_parent(NULL){defSub=NULL;}
    FirstSubject(const FirstSubject &other);
    FirstSubject(SubjectManager* parent, int id,SubjectClass subcls,QString subName,QString subCode,QString remCode,int subWeight,bool isEnable,
                 bool jdDir = true,bool isUseWb = true,QString explain = "",QString usage = "",int subSys=1);
    ~FirstSubject();

    SubjectManager* parent(){return _parent;}
    void setParent(SubjectManager* p){_parent=p;}
    int getMd(){return md;}
    int getId(){return id;}
    QString getCode(){return code;}
    void setCode(QString subCode);
    int getWeight(){return weight;}
    void setWeight(int subWeight);
    bool isEnabled(){return isEnable;}
    void setEnabled(bool en);
    QString getName(){return name;}
    void setName(QString subName);
    QString getRemCode(){return remCode;}
    void setRemCode(QString code);
    bool getJdDir(){return jdDir;}
    void setJdDir(bool dir);
    bool isUseForeignMoney(){return isUseWb;}
    void setIsUseForeignMoney(bool isUse);
    //一级科目永远不能删除
    void setDelete(bool isDeleted){}
    bool isDelete(){return false;}
    bool isSameSub(FirstSubject* other){return md==other->md && id==other->id;}

    FirstSubjectEditStates getEditState(){return witchEdited;}
    void resetEditState(){witchEdited=ES_FS_INIT;}
    void childEdited(){witchEdited |= ES_FS_CHILD;} //当子目被编辑后，要调用此方法
    SubjectClass getSubClass(){return subClass;}
    void setSubClass(SubjectClass c);
    QString getBriefExplain(){return briefExplain;}
    void setBriefExplain(QString s);
    QString getUsage(){return usage;}
    void setUsage(QString s);
    QList<int> getAllSSubIds();

    //子科目操作方法
    void addChildSub(SecondSubject* sub);
    SecondSubject* addChildSub(SubjectNameItem* ni,QString Code="",
                                   int subWeight=0,bool isEnable=true,
                               QDateTime createTime=QDateTime::currentDateTime(),
                               User* creator=curUser);
    int getChildCount(){return childSubs.count();}

    //SecondSubject* addChildSub(SecondSubject* ssub);
    //void addChildSub(SecondSubject* sub, bool isInit = false);

    SecondSubject* restoreChildSub(SecondSubject* sub);
    SecondSubject* restoreChildSub(SubjectNameItem* ni);
    bool removeChildSub(SecondSubject* sub);
    bool removeChildSub(SubjectNameItem* name);
    bool removeChildSubForId(int id);

    SecondSubject* getChildSub(int index);
    QList<SecondSubject*> getChildSubs(SortByMode sortBy = SORTMODE_NONE);
    QList<int> getChildSubIds();
    bool getRangeChildSubs(SecondSubject* ssub,SecondSubject* esub, QList<SecondSubject*>& subs);
    bool containChildSub(SecondSubject* sndSub);
    bool containChildSub(SubjectNameItem* ni);
    SecondSubject* getChildSub(SubjectNameItem* ni);
    void setDefaultSubject(SecondSubject* ssub);
    SecondSubject* getChildSub(QString name);
    SecondSubject* getDefaultSubject();

    //子目智能适配功能
    void addSmartAdapteSSub(QSet<QString> keySets, SecondSubject* ssub);
    void removeSmartAdapteSSub(QSet<QString> keySets, SecondSubject* ssub);
    SecondSubject* getAdapteSSub(QString summary);
    bool isHaveSmartAdapte(){return !keys.isEmpty();}
    void clearSmartAdaptes();

private:
    SubjectManager* _parent;
    int md;         //科目魔术字
    int id;         //科目id
    int subSys;     //所属科目系统代码
    QString code;   //科目代码
    int weight;     //科目权重
    bool isEnable;  //是否启用
    bool jdDir;     //记账方向，1（正向，增加记在借方，减少记在贷方），0（反之）
    bool isUseWb;   //是否需要使用外币

    CommonItemEditState editState;  //科目的编辑状态，提供科目是否要保存的依据
    FirstSubjectEditStates witchEdited; //哪个部分被编辑了

    QString name;           //科目名称（简称）
    QString remCode;        //科目助记符
    SubjectClass subClass;           //科目类别
    QString briefExplain;   //科目简介
    QString usage;          //科目用例
    QList<SecondSubject*> childSubs;  //该一级科目包含的二级科目
    QList<SecondSubject*> delSubs;    //暂存被移除的子目
    SecondSubject* defSub;  //默认子目
    QList<QSet<QString> > keys;         //智能子目适配关键字列表
    QList<SecondSubject*> adapteSSubs;  //适配子目列表（顺序和关键字列表必须一致）

    friend class SubjectManager;
};

Q_DECLARE_METATYPE(FirstSubject)
Q_DECLARE_METATYPE(FirstSubject*)

bool byNameThan_fs(FirstSubject *fs1, FirstSubject *fs2);
bool bySubCodeThan_fs(FirstSubject *fs1, FirstSubject *fs2);
bool byRemCodeThan_fs(FirstSubject *fs1, FirstSubject *fs2);

/**
 * @brief The FSubItrator class
 *  以科目代码的顺序迭代出一级科目对象的简单迭代器类
 */
class FSubItrator{
public:
    FSubItrator(QHash<int,FirstSubject*> &fsubs);
    bool hasNext();
    FirstSubject* next();
    int key();
    FirstSubject* value();
    void toFront();
private:
    QHash<int,FirstSubject*> fsubHash;
    QList<int> ids; //一级科目的id列表（以科目代码的顺序）
    int index;      //当前的迭代器指针索引位置
};


/**
 * @brief The SubjectNameItem struct
 * 二级科目名称条目结构类型（对应于表“SecSubjects”）
 */
class SubjectNameItem{
public:
    SubjectNameItem(int id,int cls,QString sname,QString lname,QString remCode,QDateTime crtTime,User* user);
    SubjectNameItem();

    SubjectNameItem(const SubjectNameItem& other);
    ~SubjectNameItem(){}

    int getMd(){return md;}
    int getId(){return id;}
    int getClassId(){return clsId;}
    void setClassId(int cid);
    QString getShortName(){return sname;}
    void setShortName(QString name);
    QString getLongName(){return lname;}
    void setLongName(QString name);
    QString getRemCode(){return remCode;}
    void setRemCode(QString code);
    QDateTime getCreateTime(){return crtTime;}
    User* getCreator(){return crtUser;}
    NameItemEditStates getEditState(){return witchEdit;}
    inline void resetEditState(){witchEdit = ES_NI_INIT;isDeleted=false;}
    void setDelete(bool state){isDeleted = state;}
    inline bool isDelete(){return isDeleted;}

    bool operator ==(SubjectNameItem& other){return md == other.md;}

private:
    int md;                //表证该对象的魔术字
    NameItemEditStates witchEdit;
    int id;                 //记录id
    int clsId;              //名称类别代码
    QString sname,lname;    //简称，全称
    QString remCode;        //助记符
    QDateTime crtTime;      //创建时间
    User* crtUser;          //创建者
    bool isDeleted;         //是否被删除了

    friend class SubjectManager;
    friend class DbUtil;
};

Q_DECLARE_METATYPE(SubjectNameItem)
Q_DECLARE_METATYPE(SubjectNameItem*)

bool byRemCodeThan_ni(SubjectNameItem *ni1, SubjectNameItem *ni2);
bool byNameThan_ni(SubjectNameItem *ni1, SubjectNameItem *ni2);
bool byCreateTimeThan_ni(SubjectNameItem *ni1, SubjectNameItem *ni2);

/**
 * @brief The SecondSubject class
 * 二级科目类
 */
class SecondSubject : public SubjectBase{
public:
    SecondSubject(FirstSubject* parent, int id, SubjectNameItem* name, QString Code,
                  int subWeight, bool isEnable, QDateTime crtTime, QDateTime disTime, User* crtUser);
    SecondSubject(FirstSubject* parent, int id, int nameId, int nameClsId, QString sName, QString lName,
                  QString subCode, QString remCode, int subWeight, bool isEnable,
                  User* crtUser, QDateTime crtTime = QDateTime::currentDateTime());
    SecondSubject();
    SecondSubject(const SecondSubject& other);
    ~SecondSubject(){}



    //CommonItemEditState getState(){return editState;}
    //void setEditState(CommonItemEditState state){editState=state;}

    int getMd(){return md;}
    int getId(){return id;}
    QString getCode(){return code;}
    void setCode(QString subCode);
    int getWeight(){return weight;}
    void setWeight(int subWeight);
    bool isEnabled(){return isEnable;}
    void setEnabled(bool en);
    QString getName(){return nItem->getShortName();}
    void setName(QString subName){nItem->setShortName(subName);}
    QString getRemCode(){return nItem->getRemCode();}
    void setRemCode(QString code){nItem->setRemCode(code);}
    bool getJdDir(){return parent->getJdDir();}
    bool isUseForeignMoney(){return parent->isUseForeignMoney();}
    void setDelete(bool isDeleted){this->isDeleted=isDeleted;}
    bool isDelete(){return isDeleted;}
    bool isDef();

    SecondSubjectEditStates getEditState(){return witchEdit;}
    void resetEditState(){witchEdit = ES_SS_INIT;isDeleted=false;}

    QString getLName(){return nItem->getLongName();}
    void setLName(QString name){nItem->setLongName(name);}
    //int getNameCls(){return nItem->getClassId();}//返回科目使用的名称条目的所属类别代码
    //int getNameId(){return nItem->getId();}    //返回科目使用的名称条目的id
    void setParent(FirstSubject* p){if(p){parent=p;witchEdit |= ES_SS_FID;}}
    FirstSubject* getParent(){return parent;}
    SubjectNameItem* getNameItem(){return nItem;}
    void setNameItem(SubjectNameItem* ni){nItem=ni; witchEdit |= ES_SS_NID;}
    QDateTime getCreateTime(){return crtTime;}
    void setCreateTime(QDateTime time){crtTime=time;witchEdit |= ES_SS_CTIME;}
    User* getCreator(){return creator;}
    void setCreator(User* user){creator=user; witchEdit |= ES_SS_CUSER;}
    QDateTime getDisableTime(){return disTime;}
    void setDisableTime(QDateTime time){disTime=time;witchEdit |= ES_SS_DISABLE;}


    bool operator ==(SecondSubject& other){/*if(&other == SS_ALL) return true;else*/ return md == other.md;}
    bool operator !=(SecondSubject& other){/*if(&other == SS_ALL) return false;else*/ return md != other.md;}

private:
    //void setId(int id){this->id=id;}

private:
    int md;         //表证该对象的魔术字
    int id;         //科目id
    QString code;   //科目代码
    int weight;     //科目权重
    bool isEnable;  //是否启用
    QDateTime crtTime,disTime;  //创建时间、禁用时间
    User* creator;              //创建者
    //CommonItemEditState editState;  //科目的编辑状态，提供科目是否要保存的依据
    bool isDeleted;  //是否删除的标记

    FirstSubject* parent;        //所属的一级科目
    SubjectNameItem* nItem; //描述该二级科目名称信息
    SecondSubjectEditStates witchEdit; //记录那些部分被修改了的标志

    friend class SubjectManager;
    friend class DbUtil;
};

Q_DECLARE_METATYPE(SecondSubject)
Q_DECLARE_METATYPE(SecondSubject*)

bool bySubCodeThan_ss(SecondSubject *ss1, SecondSubject *ss2);
bool byRemCodeThan_ss(SecondSubject *ss1, SecondSubject *ss2);
bool bySubNameThan_ss(SecondSubject *ss1, SecondSubject *ss2);
bool byCreateTimeThan_ss(SecondSubject *ss1, SecondSubject *ss2);


///////////////////////////SubjectManager/////////////////////////////////////
class DbUtil;

//科目管理类-
class SubjectManager
{
public:


    SubjectManager(Account* account, int subSys = DEFAULT_SUBSYS_CODE);
    Account* getAccount(){return account;}
    bool loadAfterImport();
    int getSubSysCode(){return subSys;}
    QDate getStartDate(){return startDate;}
    QDate getEndDate(){return endDate;}
    void setStartDate(QDate date){startDate=date;}
    void setEndDate(QDate date){endDate=date;}

    //保存方法
    bool save();
    bool saveFS(FirstSubject* fsub);
    bool saveSS(SecondSubject* ssub);
    bool saveNI(SubjectNameItem* ni);
    void rollback();

    //名称条目相关方法
    static int getBankClsCode();
    static int getNotUsedNiClsCode();
    static bool addNiClass(int code, QString name, QString explain);
    static bool modifyNiClass(int code, QString name, QString explain);
    static bool isUsedNiCls(int code);
    static bool removeNiCls(int code);
    static QList<SubjectNameItem*> getAllNameItems(SortByMode sortBy = SORTMODE_NONE);
    static QString getNIClsName(int clsId){return nameItemCls.value(clsId).first();}
    static QString getNIClsLName(int clsId){return nameItemCls.value(clsId).last();}
    void removeNameItem(SubjectNameItem* nItem, bool delInLib = false);
    static bool restoreNI(SubjectNameItem* nItem);
    static SubjectNameItem* restoreNI(QString sname, QString lname, QString remCode, int nameCls);
    static QHash<int,QStringList> getAllNICls(){return nameItemCls;}
    static SubjectNameItem* getNameItem(int nid){return nameItems.value(nid);}
    static SubjectNameItem* getNameItem(QString name);
    static QHash<int,SubjectNameItem*>& getAllNI(){return nameItems;}
    static bool containNI(QString name);
    bool containNI(SubjectNameItem* ni);
    bool nameItemIsUsed(SubjectNameItem* ni);

    QHash<SubjectClass,QString>& getFstSubClass(){return fsClsNames;}
    //按科目id获取科目对象的方法
    FirstSubject* getFstSubject(int id){return fstSubHash.value(id);}

    void getUseWbSubs(QList<FirstSubject*>& fsubs);
    QList<FirstSubject*> getSyClsSubs(bool in=true);
    SecondSubject* getSndSubject(int id){return sndSubs.value(id);}
    FirstSubject* getFstSubject(QString code);
    QHash<int,FirstSubject*>& getAllFstSubHash(){return fstSubHash;}
    QHash<int,SecondSubject*>& getAllSndSubHash(){return sndSubs;}
    QList<SecondSubject*> getSubSubjectUseNI(SubjectNameItem* ni);

    FSubItrator* getFstSubItrator(){return new FSubItrator(fstSubHash);}


    //获取特种主目对象的方法
    FirstSubject* getNullFSub(){return FSub_NULL;}
    FirstSubject* getCashSub(){return cashSub;}
    FirstSubject* getBankSub(){return bankSub;}
    FirstSubject* getGdzcSub(){return gdzcSub;}
    FirstSubject* getLjzjSub(){return ljzjSub;}
    FirstSubject* getCwfySub(){return cwfySub;}
    FirstSubject* getDtfySub(){return dtfySub;}
    FirstSubject* getBnlrSub(){return bnlrSub;}
    FirstSubject* getLrfpSub(){return lrfpSub;}
    FirstSubject* getYsSub(){return ysSub;}
    FirstSubject* getYfSub(){return yfSub;}
    FirstSubject* getYjsjSub(){return yjsjSub;}
    FirstSubject* getZysrSub(){return zysrSub;}
    FirstSubject* getZycbSub(){return zycbSub;}
    bool isSySubject(int sid);
    bool isSyClsSubject(int sid, bool &yes, bool isFst=true);

    //获取特种子目对象方法
    SecondSubject* getJxseSSub();
    SecondSubject* getXxseSSub();

    bool isUsedSSub(SecondSubject* ssub);

    SubjectNameItem* addNameItem(QString sname,QString lname,QString rcode,int clsId,
                                 QDateTime crtTime=QDateTime::currentDateTime(),User* creator=curUser);
    void addNameItem(SubjectNameItem* ni);

    //
    SecondSubject* addSndSubject(FirstSubject* fsub,SubjectNameItem* ni,QString code="",
                                 int weight=1,bool isEnabled=true,
                                 QDateTime crtTime=QDateTime::currentDateTime(),User* creator=curUser);

    //银行账户有关的方法
    BankAccount* getBankAccount(SecondSubject *ssub);
    Money* getSubMatchMt(SecondSubject* ssub);
    bool isBankSndSub(SecondSubject* ssub);



private slots:


private:
    bool init();

    //保存方法


    //科目对象恢复方法
    bool restoreFS(FirstSubject* sub);
    bool restoreSS(SecondSubject* sub);
    bool restoreNIFromDb(SubjectNameItem* ni);

    Account* account;
    DbUtil* dbUtil;
    int subSys;   //科目系统的类型
    QDate startDate,endDate;                    //科目系统启用的开始、截止日期

    QHash<SubjectClass,QString> fsClsNames;     //一级科目类别名称表（这是程序内置的类别名称）
    QHash<int,FirstSubject*> fstSubHash;        //一级科目哈希表
    QHash<int, SecondSubject*> sndSubs;         //所有二级科目（键为二级科目id）

    static QHash<int,QStringList> nameItemCls;      //名称条目类别表
    static QHash<int,SubjectNameItem*> nameItems;   //所有名称条目（因为多个科目管理器对象要共享名称条目信息）
    static QList<SubjectNameItem*> delNameItems;    //缓存被删除的名称条目
    static FirstSubject* FS_ALL;
    static FirstSubject* FS_NULL;


    //特种科目
    FirstSubject* FSub_NULL;               //空的一级科目（其id为-1，没有任何实际的科目相对应）
    FirstSubject *cashSub,*bankSub,*ysSub,*yfSub;  //现金、银行科目对象
    FirstSubject *gdzcSub,*dtfySub,*ljzjSub,*bnlrSub,*lrfpSub;//固定资产、待摊费用、累计折旧、本年利润和利润分配科目id
    FirstSubject *cwfySub,*yjsjSub; //财务费用、应交税金
    FirstSubject *zysrSub, *zycbSub; //主营业务收入、主营业务成本

    friend class DbUtil;
};



#endif // SUBJECT_H
