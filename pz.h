#ifndef PZ_H
#define PZ_H

#include <QString>
#include <QSqlDatabase>

#include "common.h"
#include "securitys.h"
#include "commdatastruct.h"
#include "appmodel.h"

//凭证类
class PingZheng
{
public:
    enum EditTag{
        DELETED      = -1,     //凭证被删除了
        INIT         = 0,      //凭证刚从数据库中读取，或已经执行了保存
        INFOEDITED   = 1,      //凭证信息内容被编辑了（这些内容的改变不会影响统计结果）
        VALUECHANGED = 2,      //凭证的金额发生了改变
        NEW          = 3       //新建凭证，需要插入到数据库中并回读id
    };

    PingZheng(User* user = NULL, QSqlDatabase db = QSqlDatabase::database());
    PingZheng(int id,QString date,int pnum,int znum,double js,double ds,
              PzClass pcls,int encnum,PzState state,User* vu = NULL,
              User* ru = NULL, User* bu = NULL,User* user = NULL,
              QSqlDatabase db = QSqlDatabase::database());
    PingZheng(PzData* data,User *puser,QSqlDatabase db= QSqlDatabase::database());
    static PingZheng* load(int id,QSqlDatabase db = QSqlDatabase::database());
    static PingZheng* create(User* user = NULL,QSqlDatabase db = QSqlDatabase::database());
    static PingZheng* create(QString date,int pnum,int znum,double js,double ds,
                             PzClass pcls,int encnum,PzState state,User* vu = NULL,
                             User* ru = NULL, User* bu = NULL,User* user = NULL,
                             QSqlDatabase db = QSqlDatabase::database());
    bool save();
    bool update();

    //属性访问
    int id(){return ID;}
    QString getDate(){return date;}
    void setDate(QString ds){date=ds;}
    int number(){return pnum;}
    void setNumber(int num){pnum = num;}
    int ZbNumber(){return znum;}
    void setZbNumber(int num){znum=num;}
    int encNumber(){return encNum;}
    void setEncNumber(int num){encNum=num;}
    double jsum(){return js;}
    void setJSum(double v){js=v;}
    double dsum(){return ds;}
    void setDSum(double v){ds=v;}
    PzClass getPzClass(){return pzCls;}
    void setPzClass(PzClass cls){pzCls=cls;}
    PzState getPzState(){return state;}
    void setPzState(PzState s){state=s;}
    User* verifyUser(){return vu;}
    void setVerifyUser(User* user){vu=user;}
    User* recordUser(){return ru;}
    void setRecordUser(User* user){ru=user;}
    User* bookKeeperUser(){return bu;}
    void setBookKeeperUser(User* user){bu=user;}
    User* opUser(){return user;}
    void setOpUser(User* u){user=u;}
    QList<BusiActionData*> baList(){return baLst;}
    void setBaList(QList<BusiActionData*> lst){baLst = lst;}
    EditTag editState(){return eState;}
    void setEditState(EditTag s){eState=s;}


private:
    int ID;               //id
    QString date;         //凭证日期（Qt::ISO格式）
    int pnum,znum,encNum; //凭证号，自编号和附件数
    double js,ds;         //借贷方合计值
    PzClass pzCls;        //凭证类别
    PzState state;        //凭证状态
    User *vu,*ru,*bu;     //审核、录入和记账用户    
    EditTag eState;       //凭证的编辑状态

    QList<BusiActionData*> baLst;    //会计分录列表
    QList<BusiActionData*> delLst;   //已被删除的会计分录列表
    User* user;                      //操作该凭证的用户
    QSqlDatabase db;
};

//凭证集内凭证排序时的比较函数
bool byDateLessThan(PingZheng *p1, PingZheng *p2);
bool byPzNumLessThan(PingZheng* p1, PingZheng* p2);
bool byZbNumLessThan(PingZheng* p1, PingZheng* p2);

//凭证集管理类
class PzSetMgr
{
public:
    PzSetMgr(int y, int m, User* user, QSqlDatabase db = QSqlDatabase::database());

    bool open();
    void close();
    bool isOpen();

    int year(){return y;}
    int month(){return m;}
    int getPzCount();
    int getMaxZbNum();
    bool resetPzNum(int by = 1);
    bool determineState();
    PzsState getState();
    void setstate(PzsState state);

    bool saveExtra();
    bool readExtra();
    bool readPreExtra();

    bool appendBlankPz(PzData* pd);
    bool insert(PzData* pd, int &ecode);
    bool remove(int pzNum);
    bool savePz();
    bool save();

    bool crtGdzcPz();
    bool crtDtfyImpPz(int y, int m, QList<PzData *> pzds);
    bool crtDtfyTxPz();
    bool delDtfyPz();
    bool crtJzhdsyPz();
    bool crtJzsyPz();
    bool crtJzlyPz();

    void finishAccount();

    CustomRelationTableModel* getModel();
    bool stat();
    static bool stat(int y, int m);
    bool getDetList();
    static bool getDetList(int y, int sm, int em);
    bool getTotalList();
    static bool getTotalList(int y,int sm, int em);
    bool find();
    static bool find(int y, int sm, int em);

private:
    bool isZbNumConflict(int num);

    bool isOpened;
    int y, m;
    QSqlDatabase db;
    User* user;
    //CustomRelationTableModel* model;
    PzsState state;                         //凭证集状态
    int maxPzNum;                           //最大可用凭证号
    int maxZbNum;                           //最大可用自编号

    QList<PingZheng*> pds;                  //凭证对象列表
    QList<PingZheng*> dpds;                 //已被删除的凭证对象列表

    QHash<int,Double> preExtra,preDetExtra; //期初主目和子目余额
    QHash<int,int>    preDir,preDetDir;     //期初主目和子目余额方向
    QHash<int,Double> curHpJ,curHpD;        //当期借方和贷方发生额
    QHash<int,Double> endExtra,endDetExtra; //期末主目和子目余额
    QHash<int,int>    endDir,endDetDir;     //期末主目和子目余额方向

    bool isReStat;                          //是否需要重新进行统计标志
    bool isReSave;                          //是否需要保存余额
};


#endif // PZ_H
