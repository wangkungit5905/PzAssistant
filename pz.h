#ifndef PZ_H
#define PZ_H

#include <QString>
#include <QSqlDatabase>
#include <QUndoStack>
#include <QObject>

#include "common.h"
#include "securitys.h"
#include "commdatastruct.h"
//#include "appmodel.h"
//#include "otherAsist.h"
#include "subject.h"

class PingZheng;
class AccountSuiteManager;
class Money;

//魔术数字（在比较对象时使用）
static long PZMD = 1;     //标识凭证对象
static long BAMD = 1;     //标识会计分录对象



//指示会计分录对象的哪些属性被编辑了的标记
enum BusiActionEditState{
    ES_BA_INIT    = 0x0,     //初始
    ES_BA_PARENT  = 0x01,    //宿主凭证
    ES_BA_SUMMARY = 0x02,    //摘要
    ES_BA_FSUB    = 0x04,    //一级科目
    ES_BA_SSUB    = 0x08,    //二级科目
    ES_BA_MT      = 0x10,    //币种
    ES_BA_VALUE   = 0x20,    //金额变了
    ES_BA_DIR     = 0x40,    //方向
    ES_BA_NUMBER  = 0x80     //序号
};
Q_DECLARE_FLAGS(BusiActionEditStates, BusiActionEditState)
Q_DECLARE_OPERATORS_FOR_FLAGS(BusiActionEditStates)
Q_DECLARE_METATYPE(BusiActionEditStates)
//Q_FLAGS(BusiActionEditState, BusiActionEditStates)

class BusiAction : public QObject{
    Q_OBJECT
    Q_PROPERTY(int Md READ getMd CONSTANT)
    Q_PROPERTY(int Id READ getId)
    Q_PROPERTY(PingZheng* Parent READ getParent WRITE setParent)
    Q_PROPERTY(QString Summary READ getSummary WRITE setSummary)
    Q_PROPERTY(FirstSubject* FirstSubject READ getFirstSubject WRITE setFirstSubject)
    Q_PROPERTY(SecondSubject* SecondSubject READ getSecondSubject WRITE setSecondSubject)
    Q_PROPERTY(Money* MoneyType READ getMt/* WRITE setMt*/)
    Q_PROPERTY(Double Value READ getValue WRITE setValue /*NOTIFY valueOrDirChanged*/)
    Q_PROPERTY(MoneyDirection Dir READ getDir WRITE setDir)

public:
    BusiAction();
    BusiAction(BusiAction& other);
    ~BusiAction(){}
    BusiAction(int id,PingZheng* p,QString summary,FirstSubject* fsub,SecondSubject* ssub,
               Money* mt,MoneyDirection dir,Double v,int num);

    void integratedSetValue(FirstSubject* fsub,SecondSubject* ssub,Money* mt,Double v,MoneyDirection dir);

    long getMd(){return md;}
    int getId(){return id;}
    PingZheng* getParent(){return parent;}
    void setParent(PingZheng* p);
    QString getSummary() const{return summary;}
    void setSummary(QString s);
    FirstSubject* getFirstSubject(){return fsub;}
    void setFirstSubject(FirstSubject* fsub);
    SecondSubject* getSecondSubject(){return ssub;}
    void setSecondSubject(SecondSubject* ssub);
    Money* getMt() const{return mt;}
    void setMt(Money* mt, Double v);
    Double getValue() const{return v;}
    void setValue(Double value);
    MoneyDirection getDir() const{return dir;}
    void setDir(MoneyDirection direct);
    int getNumber(){return num;}
    void setNumber(int number);


    void setDelete(bool isDelete){isDeleted = isDelete;}
    BusiActionEditStates getEditState();
    void setEditState(BusiActionEditState state);
    void resetEditState(){setProperty(ObjEditState,ES_BA_INIT);isDeleted=false;}
    bool isDelete(){return isDeleted;}

    bool operator ==(const BusiAction& other){return md == other.md;}
    bool operator !=(const BusiAction& other){return (md != other.md);}

signals:
    //void dirChanged(MoneyDirection oldDir,BusiAction* ba);
    void valueChanged(Money* oldMt,Double &oldValue,MoneyDirection oldDir,BusiAction* ba);
    void subChanged(FirstSubject* oldFSub, SecondSubject* oldSSub, Money* oldMt, Double oldValue, BusiAction* ba);

private:
    long md;          //表证该对象的魔术字
    int id;           //该业务活动的
    PingZheng* parent;          //业务活动所属凭证id
    QString summary;            //摘要
    FirstSubject* fsub;         //一级科目id
    SecondSubject* ssub;        //二级科目id
    Money* mt;                  //币种
    MoneyDirection dir;         //借贷方向（1：借，0：未定，-1：贷）
    Double v;                   //金额
    int num;                    //该业务活动在凭证中的序号（基于1）
    //BusiActionEditStates witchEdited;
    bool isDeleted;

    friend class PingZheng;
    friend class AccountSuiteManager;
    friend class DbUtil;
    //friend class ModifyMultiPropertyOnBa;
};

//指示凭证对象的哪个属性值被修改的标记
enum PingZhengEditState{
    ES_PZ_INIT       = 0x0,      //初始
    ES_PZ_DATE       = 0x01,     //日期
    ES_PZ_PZNUM      = 0x02,     //凭证号
    ES_PZ_ZBNUM      = 0x04,     //自编号
    ES_PZ_ENCNUM     = 0x08,     //附件数
    ES_PZ_PZSTATE    = 0x10,     //凭证审核状态
    ES_PZ_JSUM       = 0x20,     //借方合计
    ES_PZ_DSUM       = 0x40,     //贷方合计
    ES_PZ_RUSER      = 0x080,     //录入用户
    ES_PZ_VUSER      = 0x100,     //审核用户
    ES_PZ_BUSER      = 0x200,     //入账用户
    ES_PZ_CLASS      = 0x400,     //凭证类别
    ES_PZ_BACTION    = 0x8000     //会计分录
};
Q_DECLARE_FLAGS(PingZhengEditStates, PingZhengEditState)
Q_DECLARE_OPERATORS_FOR_FLAGS(PingZhengEditStates)
Q_DECLARE_METATYPE(PingZhengEditStates)

//凭证类
class PingZheng : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int Md READ getMd CONSTANT)
    Q_PROPERTY(int Id READ id)
    Q_PROPERTY(AccountSuiteManager* Parent READ parent WRITE setParent)
    Q_PROPERTY(QDate Date READ getDate2 WRITE setDate)
    Q_PROPERTY(QString DateStr READ getDate WRITE setDate)
    Q_PROPERTY(int Number READ number WRITE setNumber)
    Q_PROPERTY(int ZbNumer READ zbNumber WRITE setZbNumber)
    Q_PROPERTY(int EncNumber READ encNumber WRITE setEncNumber)
    Q_PROPERTY(Double JSum READ jsum)
    Q_PROPERTY(Double DSum READ dsum)
    Q_PROPERTY(FirstSubject* OppoSubject READ getOppoSubject WRITE setOppoSubject)
    Q_PROPERTY(PzState state READ getPzState WRITE setPzState)
    Q_PROPERTY(User* Recorder READ recordUser WRITE setRecordUser)
    Q_PROPERTY(User* Verifier READ verifyUser WRITE setVerifyUser)
    Q_PROPERTY(User* BookKeeper READ bookKeeperUser WRITE setBookKeeperUser)

public:
    PingZheng(AccountSuiteManager* p = 0);
    PingZheng(AccountSuiteManager* parent,int id,QString date,int pnum,int m_znum,Double js,Double ds,
              PzClass pzCls,int encnum,PzState state,User* vu = NULL,
              User* ru = NULL, User* bu = NULL);
    ~PingZheng();

    //属性访问
    long getMd(){return md;}
    int id(){return ID;}
    AccountSuiteManager* parent(){return p;}
    void setParent(AccountSuiteManager* parent){p = parent;}
    QString getDate(){return date;}
    QDate getDate2() const{return QDate::fromString(date,Qt::ISODate);}
    void setDate(QString ds);
    void setDate(QDate d);
    int number() const{return pnum;}
    void setNumber(int num);
    int zbNumber(){return m_znum;}
    void setZbNumber(int num);
    int encNumber(){return encNum;}
    void setEncNumber(int num);
    Double jsum(){return js;}
    Double dsum(){return ds;}
    bool isBalance(){return js == ds;}
    PzClass getPzClass(){return pzCls;}
    void setPzClass(PzClass cls);
    FirstSubject* getOppoSubject(){return oppoSub;}
    void setOppoSubject(FirstSubject* sub){oppoSub=sub;}
    PzState getPzState(){return state;}
    void setPzState(PzState s);
    User* verifyUser(){return vu;}
    void setVerifyUser(User* user);
    User* recordUser(){return ru;}
    void setRecordUser(User* user);
    User* bookKeeperUser(){return bu;}
    void setBookKeeperUser(User* user);
    QString memInfo(){return "";}
    void setMemInfo(QString info){}


    //会计分录方法
    BusiAction* getCurBa(){return curBa;}
    void setCurBa(BusiAction* ba){curBa = ba;}   //设置凭证的当前会计分录
    BusiAction* getBusiAction(int n);
    BusiAction* appendBlank();
    //BusiAction* append(QString summary,FirstSubject* fsub, SecondSubject* ssub, Money* mt, MoneyDirection dir, Double v);
    bool append(BusiAction* ba, bool isUpdate = true);
    bool insert(int index, BusiAction *ba);
    //bool remove(int index);
    bool remove(BusiAction* ba);
    bool restore(BusiAction* ba);
    BusiAction* take(int index);
    bool moveUp(int row,int nums = 1);
    bool moveDown(int row,int nums = 1);
    bool isEmpty(){return baLst.isEmpty();}
    bool isFirst(){return (!baLst.isEmpty() && (baLst.first() == curBa));}
    bool isLast(){return (!baLst.isEmpty() && (baLst.last() == curBa));}

    int baCount(){return baLst.count();}
    QList<BusiAction*>& baList(){return baLst;}
    void setBaList(QList<BusiAction*> lst);


    //编辑状态方法
    PingZhengEditStates getEditState();
    void resetEditState(){setProperty(ObjEditState,ES_PZ_INIT);isDeleted=false;}
    void setEditState(PingZhengEditState state);
    bool isDelete(){return isDeleted;}
    void setDeleted(bool isDel){isDeleted = isDel;}
    //void removeTailBlank();

    bool operator ==(PingZheng& other){return md == other.md;}
    bool operator !=(PingZheng& other){return md != other.md;}

private slots:
    //void adjustSumForDirChanged(MoneyDirection oldDir,BusiAction* ba);
    void adjustSumForValueChanged(Money* oldMt,Double &oldValue,MoneyDirection oldDir, BusiAction* ba);
    void subChanged(FirstSubject* oldFSub, SecondSubject* oldSSub, Money* oldMt, Double oldValue, BusiAction *ba);
signals:
    void updateBalanceState(bool balance); //更新凭证的借贷平衡状态
    void mustRestat();          //告诉父对象，由于其包含的分录发生了影响统计结果的改变
    void pzContentChanged(PingZheng* pz); //凭证内容的任何改变都将触发
    void indexBoundaryChanged(bool first, bool last);
    void pzStateChanged(PzState oldState, PzState newState);

    //由statUtil类监视的信号
    void addOrDelBa(BusiAction* ba, bool add); //增加或移除分录（当需要针对单一科目、币种组合需要进行重新统计时触发，比如分录的金额不为0，就要对分录涉及的科目和币种组合进行统计）

    //把这4个信号整合为2个，值改变和科目改变
    void subChangedOnBa(BusiAction* ba, FirstSubject* oldFSub, SecondSubject* oldSSub, Money* oldMt, Double oldValue); //分录的科目设置改变
    void valueChangedOnBa(BusiAction* ba, Money* oldMt,Double &oldValue,MoneyDirection oldDir);  //分录的值改变
    //void mtChangedOnBa(BusiAction* ba, Money* oldMt); //分录的币种改变
    //void dirChangedOnBa(BusiAction* ba, MoneyDirection od); //分录的方向改变
private:
    bool hasBusiAction(BusiAction* ba);
    void calSum();
    void _recalSumForValueChanged(Money* oldMt,Double &oldValue,MoneyDirection oldDir, BusiAction* ba);
    void adjustSumForBaChanged(BusiAction* ba, bool add = true);
    void watchBusiaction(BusiAction* ba, bool isWatch=true);
private:
//    bool saveBaOrder();
//    bool saveNewPz();
//    bool saveInfoPart();
//    bool saveContent();

    long md;                            //表证该对象的魔术字
    int ID;                             //id
    AccountSuiteManager* p;                        //该凭证所属的凭证集管理器对象
    QString date;                       //凭证日期（Qt::ISO格式）
    int pnum,m_znum,encNum;               //凭证号，自编号和附件数
    Double js,ds;                       //借贷方合计值
    PzClass pzCls;                      //凭证类别
    PzState state;                      //凭证状态
    User *vu,*ru,*bu;                   //审核、录入和记账用户
    QList<BusiAction*> baLst;           //会计分录列表
    QList<BusiAction*> baDels;          //被删除的会计分录列表
    QList<BusiAction*> ba_saveAfterDels;//保存被删除后执行了保存操作的分录对象（用以支持恢复任何情况下被删除的分录对象）
    BusiAction* curBa;                  //当前会计分录对象
    bool isDeleted;                     //是否被删除的标记
    FirstSubject* oppoSub;              //结转汇兑损益类凭证的对方科目

    friend class AccountSuiteManager;
    friend class BusiAction;
    friend class DbUtil;
};

//凭证集内凭证排序时的比较函数
bool byDateLessThan(PingZheng *p1, PingZheng *p2);
bool byPzNumLessThan(PingZheng* p1, PingZheng* p2);
bool byZbNumLessThan(PingZheng* p1, PingZheng* p2);



#endif // PZ_H
