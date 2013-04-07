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
class PzSetMgr;
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

class BusiAction{

public:
    BusiAction()
    {
        md = BAMD++;
        id = 0;
        parent = NULL;
        summary = "";
        fsub = NULL;
        ssub = NULL;
        mt = 0;
        dir = MDIR_D;
        v = 0.00;
        num = 0;
    }
    BusiAction(BusiAction& other)
    {
        md = BAMD++;
        id = 0;
        parent = other.parent;
        summary = other.summary;
        fsub = other.fsub;
        ssub = other.ssub;
        mt = other.mt;
        dir = other.dir;
        v = other.v;
        num = other.num;
    }
    ~BusiAction(){}
    BusiAction(int id,PingZheng* p,QString summary,FirstSubject* fsub,SecondSubject* ssub,
               Money* mt,MoneyDirection dir,Double v,int num):id(id),parent(p),summary(summary),
        fsub(fsub),ssub(ssub),mt(mt),dir(dir),v(v),num(num),isDeleted(false)
    {md = BAMD++;}

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
    void setMt(Money* mt);

    Double getValue() const{return v;}
    void setValue(Double value);

    MoneyDirection getDir() const{return dir;}
    void setDir(MoneyDirection direct);

    int getNumber(){return num;}
    void setNumber(int number);


    void setDelete(bool isDelete){isDeleted = isDelete;}
    BusiActionEditStates getEditState(){return witchEdited;}
    void resetEditState(){witchEdited = ES_BA_INIT;isDeleted=false;}
    bool isDelete(){return isDeleted;}

    bool operator ==(const BusiAction& other){return md == other.md;}
    //bool operator !=(const BusiAction& other){return (md != other.md);}

private:
    long md;          //表证该对象的魔术字
    int id;           //该业务活动的
    PingZheng* parent;          //业务活动所属凭证id
    QString summary;            //摘要
    FirstSubject* fsub;         //一级科目id
    SecondSubject* ssub;        //二级科目id
    Money* mt;                  //币种
    MoneyDirection dir;                    //借贷方向（1：借，0：贷，-1：未定）
    Double v;                   //金额
    int num;                    //该业务活动在凭证中的序号（基于1）
    BusiActionEditStates witchEdited;
    bool isDeleted;

    friend class PingZheng;
    friend class PzSetMgr;
};

//指示凭证对象的哪个属性值被修改的标记
enum PingZhengEditState{
    ES_PZ_INIT       = 0x0,      //初始
    ES_PZ_DATE       = 0x01,     //日期
    ES_PZ_PZNUM      = 0x04,     //凭证号
    ES_PZ_ZBNUM      = 0x08,     //自编号
    ES_PZ_ENCNUM     = 0x10,     //附件数
    ES_PZ_PZSTATE    = 0x20,     //凭证审核状态
    ES_PZ_JSUM       = 0x40,     //借方合计
    ES_PZ_DSUM       = 0x80,     //贷方合计
    ES_PZ_RUSER      = 0x100,     //录入用户
    ES_PZ_VUSER      = 0x200,     //审核用户
    ES_PZ_BUSER      = 0x400,     //入账用户
    ES_PZ_CLASS      = 0x800,     //凭证类别
    ES_PZ_BACTION    = 0x1000     //会计分录
};
Q_DECLARE_FLAGS(PingZhengEditStates, PingZhengEditState)
Q_DECLARE_OPERATORS_FOR_FLAGS(PingZhengEditStates)

//凭证类
class PingZheng
{
public:
    PingZheng(PzSetMgr* p = 0);
    PingZheng(int id,QString date,int pnum,int znum,Double js,Double ds,
              PzClass pcls,int encnum,PzState state,User* vu = NULL,
              User* ru = NULL, User* bu = NULL, PzSetMgr* p= NULL);
    ~PingZheng(){}

//    static PingZheng* load(int id,QSqlDatabase db = QSqlDatabase::database());
//    static PingZheng* create(User* user = NULL,QSqlDatabase db = QSqlDatabase::database());
//    static PingZheng* create(QString date,int pnum,int znum,double js,double ds,
//                             PzClass pcls,int encnum,PzState state,User* vu = NULL,
//                             User* ru = NULL, User* bu = NULL,User* user = NULL,
//                             QSqlDatabase db = QSqlDatabase::database());
//    bool save();
//    bool update();

    //属性访问
    long getMd(){return md;}
    int id(){return ID;}
    PzSetMgr* parent(){return p;}
    void setParent(PzSetMgr* parent){p = parent;}
    QString getDate(){return date;}
    QDate getDate2() const{return QDate::fromString(date,Qt::ISODate);}
    void setDate(QString ds)
    {
        QString d = ds.trimmed();
        if(d != date){
            date=ds;
            witchEdited |= ES_PZ_DATE;
        }
    }
    void setDate(QDate d)
    {
        QString ds = d.toString(Qt::ISODate);
        if(ds != date){
            date = ds;
            witchEdited |= ES_PZ_DATE;
        }
    }
    int number() const{return pnum;}
    void setNumber(int num)
    {
        if(pnum != num){
            pnum = num;
            witchEdited |= ES_PZ_PZNUM;
        }
    }
    int ZbNumber(){return znum;}
    void setZbNumber(int num)
    {
        if(znum != num){
           znum = num;
           witchEdited |= ES_PZ_ZBNUM;
        }
    }
    int encNumber(){return encNum;}
    void setEncNumber(int num)
    {
        if(encNum != num){
           encNum=num;
           witchEdited |= ES_PZ_ENCNUM;
        }
    }
    Double jsum(){return js;}
    void setJSum(Double v)
    {
        if(js != v){
            js=v;
            witchEdited |= ES_PZ_JSUM;
        }
    }
    Double dsum(){return ds;}
    void setDSum(Double v)
    {
        if(ds != v){
            ds=v;
            witchEdited |= ES_PZ_DSUM;
        }
    }
    PzClass getPzClass(){return pzCls;}
    void setPzClass(PzClass cls)
    {
        if(pzCls != cls){
           pzCls=cls;
           witchEdited |= ES_PZ_CLASS;
        }
    }
    PzState getPzState(){return state;}
    void setPzState(PzState s)
    {
        if(state != s){
            state=s;
            witchEdited |= ES_PZ_PZSTATE;
        }
    }
    User* verifyUser(){return vu;}
    void setVerifyUser(User* user)
    {
        if(vu == NULL || vu != user){
            vu=user;
            witchEdited |= ES_PZ_VUSER;
        }
    }
    User* recordUser(){return ru;}
    void setRecordUser(User* user)
    {
        if(ru == NULL || ru != user){
            ru=user;
            witchEdited |= ES_PZ_RUSER;
        }
    }
    User* bookKeeperUser(){return bu;}
    void setBookKeeperUser(User* user)
    {
        if(bu == NULL || bu != user){
            bu=user;
            witchEdited |= ES_PZ_BUSER;
        }
    }

    //会计分录方法
    void setCurBa(BusiAction* ba){curBa = ba;}   //设置凭证的当前会计分录
    BusiAction* getBusiAction(int n)
    {
        if(n >= baLst.count() || n < 0)
            return NULL;
        else
            return baLst.at(n);
    }
    BusiAction* appendBlank();
    bool append(BusiAction* ba, bool isUpdate = true);
    bool insert(int index, BusiAction *ba);
    bool remove(int index);
    bool remove(BusiAction* ba);
    bool restore(BusiAction* ba);
    BusiAction* take(int index);
    bool moveUp(int row,int nums = 1);
    bool moveDown(int row,int nums = 1);

    int baCount(){return baLst.count();}
    QList<BusiAction*>& baList(){return baLst;}
    void setBaList(QList<BusiAction*> lst){baLst = lst;}


    //编辑状态方法
    PingZhengEditStates getEditState(){return witchEdited;}
    void resetEditState(){witchEdited=ES_PZ_INIT;isDeleted=false;}
    bool isDelete(){return isDeleted;}
    void setDeleted(bool isDel){isDeleted = isDel;}
    //void removeTailBlank();

    bool operator ==(PingZheng& other){return md == other.md;}
    //bool operator !=(PingZheng& other){return md != other.md;}

private:
    bool hasBusiAction(BusiAction* ba);
    void calSum();

private:
//    bool saveBaOrder();
//    bool saveNewPz();
//    bool saveInfoPart();
//    bool saveContent();

    long md;                            //表证该对象的魔术字
    int ID;                             //id
    PzSetMgr* p;                        //该凭证所属的凭证集管理器对象
    QString date;                       //凭证日期（Qt::ISO格式）
    int pnum,znum,encNum;               //凭证号，自编号和附件数
    Double js,ds;                       //借贷方合计值
    PzClass pzCls;                      //凭证类别
    PzState state;                      //凭证状态
    User *vu,*ru,*bu;                   //审核、录入和记账用户
    QList<BusiAction*> baLst;           //会计分录列表
    QList<BusiAction*> baDels;          //被删除的会计分录列表
    BusiAction* curBa;                  //当前会计分录对象
    PingZhengEditStates witchEdited;    //凭证哪个部分被编辑的标志
    bool isDeleted;                     //是否被删除的标记


    friend class PzSetMgr;
    friend class BusiAction;
    friend class DbUtil;
};

//凭证集内凭证排序时的比较函数
bool byDateLessThan(PingZheng *p1, PingZheng *p2);
bool byPzNumLessThan(PingZheng* p1, PingZheng* p2);
bool byZbNumLessThan(PingZheng* p1, PingZheng* p2);



#endif // PZ_H
