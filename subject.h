#ifndef SUBJECT_H
#define SUBJECT_H

#include <QHash>
#include <QStringList>

#include "commdatastruct.h"
#include "global.h"

const int UNID      = 0;    //无意义的id值，比如对于新创建但还未保存的二级科目对象的id值
const int UNCLASS   = 0;    //未知的分类
const int ALLCLASS  = 0;    //所有类别

const int SUPERUSERID   = 1;  //超级用户ID
const int CLIENTCLASSID = 2;  //业务客户类别id

const int DEFALUTSUBFS  = 65535; //默认科目的权重

static int FSTSUBMD = 1;     //一级科目对象使用的魔术字
static int NAMEITEMMD = 1;   //名称条目对象使用的魔术字
static int SNDSUBMD = 1;     //二级科目对象使用的魔术字


//class Money;
class Account;
class SecondSubject;
class FirstSubject;
struct SubjectNameItem;

//虚拟科目对象
extern FirstSubject* FS_ALL;    //表示所有一级科目（md=1）
extern FirstSubject* FS_NULL;   //表示空一级科目（即没有设置）（md=2）
extern SubjectNameItem* NI_ALL; //表示“所有或全部”意思的名称条目（md=1）
extern SubjectNameItem* NI_NULL;//表示空（md=2）
extern SecondSubject* SS_ALL;   //表示任意一级科目下的所有二级科目（md=1）
extern SecondSubject* SS_NULL;  //表示空二级科目（md=2）

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




class FirstSubject : public SubjectBase{
public:
    FirstSubject():md(FSTSUBMD++),id(0){}
    FirstSubject(const FirstSubject &other);
    FirstSubject(int id,int subcls,QString subName,QString subCode,QString remCode,int subWeight,
                 bool jdDir = true,bool isUseWb = true,QString explain = "",QString usage = "",int subSys=1);
    ~FirstSubject();

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

    FirstSubjectEditStates getEditState(){return witchEdited;}
    void resetEditState(){witchEdited=ES_FS_INIT;}
    void childEdited(){witchEdited |= ES_FS_CHILD;} //当子目被编辑后，要调用此方法
    int getSubClass(){return subClass;}
    void setSubClass(int c);
    QString getBriefExplain(){return briefExplain;}
    void setBriefExplain(QString s);
    QString getUsage(){return usage;}
    void setUsage(QString s);


    //子科目操作方法（按约定：添加子目用科目管理器使用的方法）
    //bool addChildSub(SecondSubject* sub);
    int getChildCount(){return childSubs.count();}

    //SecondSubject* addChildSub(SecondSubject* ssub);
    //void addChildSub(SecondSubject* sub, bool isInit = false);

    SecondSubject* restoreChildSub(SecondSubject* sub);
    SecondSubject* restoreChildSub(SubjectNameItem* ni);
    bool removeChildSub(SecondSubject* sub);
    bool removeChildSub(SubjectNameItem* name);

    SecondSubject* getChildSub(int index){if(index<0 || index>=childSubs.count()) return 0;return childSubs.at(index);}
    QList<SecondSubject*>& getChildSubs(){return childSubs;}
    bool getRangeChildSubs(SecondSubject* ssub,SecondSubject* esub, QList<SecondSubject*>& subs);
    bool containChildSub(SecondSubject* sndSub);
    bool containChildSub(SubjectNameItem* ni);
    SecondSubject* getChildSub(SubjectNameItem* ni);
    void setDefaultSubject(SecondSubject* ssub){defSub=ssub;}
    SecondSubject* getDefaultSubject(){return defSub;}

private:
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
    int subClass;           //科目类别
    QString briefExplain;   //科目简介
    QString usage;          //科目用例
    QList<SecondSubject*> childSubs;  //该一级科目包含的二级科目
    QList<SecondSubject*> delSubs;    //暂存被移除的子目
    SecondSubject* defSub;  //默认子目

    friend class SubjectManager;
};

Q_DECLARE_METATYPE(FirstSubject)
Q_DECLARE_METATYPE(FirstSubject*)

bool byNameThan_fs(FirstSubject *fs1, FirstSubject *fs2);
bool bySubCodeThan_fs(FirstSubject *fs1, FirstSubject *fs2);
bool byRemCodeThan_fs(FirstSubject *fs1, FirstSubject *fs2);


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
    inline void setClassId(int cid);
    QString getShortName(){return sname;}
    inline void setShortName(QString name);
    QString getLongName(){return lname;}
    inline void setLongName(QString name);
    QString getRemCode(){return remCode;}
    inline void setRemCode(QString code);
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
};

Q_DECLARE_METATYPE(SubjectNameItem)
Q_DECLARE_METATYPE(SubjectNameItem*)

bool byRemCodeThan_ni(SubjectNameItem *ni1, SubjectNameItem *ni2);
bool byNameThan_ni(SubjectNameItem *ni1, SubjectNameItem *ni2);

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

    SecondSubjectEditStates getEditState(){return witchEdit;}
    void resetEditState(){witchEdit = ES_SS_INIT;isDeleted=false;}

    QString getLName(){return nItem->getLongName();}
    void setLName(QString name){nItem->setLongName(name);}
    //int getNameCls(){return nItem->getClassId();}//返回科目使用的名称条目的所属类别代码
    //int getNameId(){return nItem->getId();}    //返回科目使用的名称条目的id
    void setParent(FirstSubject* p){if(p)parent=p;}
    FirstSubject* getParent(){return parent;}
    SubjectNameItem* getNameItem(){return nItem;}
    QDateTime getCreateTime(){return crtTime;}
    User* getCreator(){return creator;}
    QDateTime getDiableTime(){return disTime;}
    void setDisableTime(QDateTime time){disTime=time;}


    bool operator ==(SecondSubject& other){if(&other == SS_ALL) return true;else return md == other.md;}
    bool operator !=(SecondSubject& other){if(&other == SS_ALL) return false;else return md != other.md;}

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

};

Q_DECLARE_METATYPE(SecondSubject)
Q_DECLARE_METATYPE(SecondSubject*)

bool bySubCodeThan_ss(SecondSubject *ss1, SecondSubject *ss2);
bool byRemCodeThan_ss(SecondSubject *ss1, SecondSubject *ss2);
bool bySubNameThan_ss(SecondSubject *ss1, SecondSubject *ss2);


///////////////////////////SubjectManager/////////////////////////////////////
class DbUtil;

//科目管理类-
class SubjectManager
{
public:
    //保存FSAgent表中的一条一级科目到二级科目的映射记录
    //    struct FsaMap{
    //        int id;   //FSAgent表的id列
    //        int fid;  //FSAgent表的fid列
    //        int sid;  //FSAgent表的sid列
    //    };



    SubjectManager(Account* account, int subSys = 1);
    Account* getAccount(){return account;}

    //保存方法
    void save();
    void rollback();

    static QString getNIClsName(int clsId){return nameItemCls.value(clsId).first();}
    static QString getNIClsLName(int clsId){return nameItemCls.value(clsId).last();}
    static QHash<int,QStringList>& getAllNICls(){return nameItemCls;}
    static SubjectNameItem* getNameItem(int nid){return nameItems.value(nid);}

    //按科目id获取科目对象的方法
    FirstSubject* getFstSubject(int id){fstSubHash.value(id);}
    SecondSubject* getSndSubject(int id){return sndSubs.value(id);}

    //获取特种科目的方法
    FirstSubject* getCashSub(){return cashSub;}
    FirstSubject* getBankSub(){return bankSub;}
    FirstSubject* getGdzcSub(){return gdzcSub;}
    FirstSubject* getCwfySub(){return cwfySub;}
    FirstSubject* getDtfySub(){return dtfySub;}
    FirstSubject* getBnlrSub(){return bnlrSub;}
    FirstSubject* getLrfpSub(){return lrfpSub;}
    bool isSySubject(int sid);
    QList<BankAccount*>& getBankAccounts();

    //
    SubjectNameItem* addNameItem(QString sname,QString lname,QString rcode,int clsId,
                                 QDateTime crtTime=QDateTime::currentDateTime(),User* creator=curUser);
    //
    SecondSubject* addSndSubject(FirstSubject* fsub,SubjectNameItem* ni,QString code="",
                                 int weight=1,bool isEnabled=true,
                                 QDateTime crtTime=QDateTime::currentDateTime(),User* creator=curUser);

    ////////////////////////////////////////////////////////////
    //获取科目名的方法
    //QString getFstSubName(int id);
    //QString getFstSubCode(int id);
    //QString getSndSubName(int id);
    //QString getSndSubLName(int id);
    //bool getFstSubClasses(QHash<int,QString>& subcls);
    //bool getSSClasses(QHash<int,QString>& subcls);
    //bool getAllFstSub(QList<int>& ids, QList<QString>& names);
    //bool getAllFstSub2(QList<int>& ids, QList<QString>& names);
    //bool getSndSubInSpecFst(int pid, QList<int>& ids, QList<QString>& names);
    //bool getFstToSnd(int fid, int sid, int& id);
    //bool getOwnerSub(int oid, QHash<int,QString>& names);

    //bool getFstSubs(QList<FirstSubject *> &subLst, int subCls = 0, bool enabled = true);

    ////////////////////////////////////////////////////////////////////////////////

    //操作一级科目列表的方法
//    int fsCount(){return fstSubs.count();}
//    QList<FirstSubject*> getAllFstSubs();
//    QList<FirstSubject*> getSpecClassSubs(int clsId);
//    bool getRangeSubs(FirstSubject* ssub,FirstSubject* esub, QList<FirstSubject*>& subs);
//    FirstSubject* getFS(int index){if(index<0 || index>=fstSubs.count()) return 0;return fstSubs.at(index);}
//    FirstSubject* getFSById(int id);
//    FirstSubject* getFsByCode(QString code);

    //操作二级科目名称列表的方法
//    void getAllNameClasses(QHash<int, QString> &nameCls);
//    QList<SubjectNameItem*> getAllNameItems(){return nameItems;}
//    bool nameItemIsUsing(SubjectNameItem* nItem);
//    bool hasNameItem(SubjectNameItem* ni);
//    QList<SubjectNameItem*> searchNameItem(QString keyWord);
//    int niCount(){return nameItems.count();}
//    SubjectNameItem* getNI(int index){if(index<0 || index>=nameItems.count()) return 0;return nameItems.at(index);}
//    void addNameItem(SubjectNameItem* nItem){if(nItem) nameItems<<nItem;}
//    SubjectNameItem* addNameItem(QString sname,QString lname, QString remCode, int nameCls);
//    void removeNameItem(SubjectNameItem* nItem);
//    bool restoreNI(SubjectNameItem* nItem);
//    SubjectNameItem* restoreNI(QString sname, QString lname, QString remCode, int nameCls);

    //QList<SubjectNameItem*> getSSinFS(FirstSubject* p);

    //操作二级科目列表的方法
    //SecondSubject* getSSById(int id);
    //QList<SecondSubject*> getAllSndSubs(){return sndSubs.values();}
    //int ssCount(){return sndSubs.count();}
    //SecondSubject* getSS(int index){if(index<0 || index>=sndSubs.count()) return 0;return sndSubs.at(index);}

    //银行账户有关的方法
    //bool getAllBankAccount(QHash<int,BankAccount*>& banks);
    //BankAccount* getBankAccount(int sid);
    //Money* getSubMatchMt(SecondSubject* ssub);

    //获取要作特别处理的一级科目的方法

    //FirstSubject* getCashSub(){return cashSub;}
    //FirstSubject* getBankSub(){return bankSub;}
    //FirstSubject* getYsSub(){return ysSub;}
    //FirstSubject* getYfSub(){return yfSub;}
    //bool isReqWbSub(FirstSubject* fsub){return wbSubs.contains(fsub);}
    //FirstSubject* getGdzcSub(){return gdzcSub;}
    //FirstSubject* getDtfySub(){return dtfySub;}
    //FirstSubject* getLjzjSub(){return ljzjSub;}
    //FirstSubject* getBnlrSub(){return bnlrSub;}

    //特殊二级科目处理方法
    //int getDefaultSndSubId(int fid){return defSndSubIds.value(fid);}//返回指定主目下的默认子目id
    //int getMasterMtId(){return masterMtId;}
    //int getMasterBankId(){return masterBankId;}
    //bool isBankSndSub(SecondSubject* ssub){return bankSids.contains(ssub->getId());}//是否是银行存款下的子目

    //待摊费用科目类
    //void getDtfySubIds(QHash<int, int> &h);
    //void getDtfySubNames();

    //
    //bool isUsing(SecondSubject* sub);//指定的二级科目是否已被使用


private:
    bool init();

    //保存方法
    bool saveFS(FirstSubject* fsub);
    bool saveSS(SecondSubject* ssub);
    bool saveNI(SubjectNameItem* ni);

    //科目对象恢复方法
    bool restoreFS(FirstSubject* sub);
    bool restoreSS(SecondSubject* sub);
    bool restoreNIFromDb(SubjectNameItem* ni);

    Account* account;
    DbUtil* dbUtil;
    int subSys;   //科目系统的类型

    QHash<int,QString> fstSubCls;           //一级科目类别表
    QList<FirstSubject*> fstSubs;           //所有的一级科目
    QHash<int,FirstSubject*> fstSubHash;    //一级科目哈希表
    QHash<int, SecondSubject*> sndSubs;     //所有二级科目（键为二级科目id）

    static QHash<int,QStringList> nameItemCls;    //名称条目类别表
    static QHash<int,SubjectNameItem*> nameItems; //所有名称条目（因为多个科目管理器对象要共享名称条目信息）
    //QList<SubjectNameItem*> delNameItems;


    //特种科目
    FirstSubject *cashSub,*bankSub;  //现金、银行科目对象
    FirstSubject *gdzcSub,*dtfySub,*ljzjSub,*bnlrSub,*lrfpSub;//固定资产、待摊费用、累计折旧、本年利润和利润分配科目id
    FirstSubject *cwfySub;

    //特种科目类别代码
    int sySubCls;   //损益类科目类别
    friend class DbUtil;
    //////////////////////可能要废弃//////////////////////////////////////


    //（注意：这3个列表的同一个索引是对应同一个一级科目，并且是以科目代码的顺序）
    //QList<int> fstIds; //所有一级科目的id
    //QList<QString> fstSubNames, fstSubCodes; //所有一级科目的名称和代码


    //QStringList sndNames,sndLNames; //从SecSubjects表读取的所有二级科目名称和全称的列表
    //QHash<int,int> secIdx;          //键为SecSubjects表id列，值为对应的sndNames或sndLNames列表的索引
    //QHash<int,int> sndIdx;          //键为FSAgent表id列，值为对应的sndNames或sndLNames列表的索引
    //QList<FsaMap> fsaMaps;          //保存FSAgent表的所有映射条目


    //QHash<int,QList<int> > subBelongs; //所有一级科目属下的二级科目
    //QHash<int,int> defSndSubIds;       //所有一级科目属下的默认二级科目id

    //////////////////////可能要废弃//////////////////////////////////////

    //QList<int> plIds;                  //损益类主科目id

    //QList<int> bankSids;               //所有银行存款下的子目id
    //QHash<int,BankAccount*> banks;     //键为银行子目id



    //一些特别的一级科目id

    //QList<FirstSubject*> wbSubs;     //所有需要使用外币的科目（如银行、应收和应付等）


    //一些特别的二级级科目id
    //int masterMtId;    //现金科目下与母币对应的子目id
    //int masterBankId;  //银行存款下与基本户-母币账户对应的子目id


};



#endif // SUBJECT_H
