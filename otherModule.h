#ifndef OTHERMODULE_H
#define OTHERMODULE_H


#include <QString>
#include <QDate>
#include <QStringList>
#include <QHash>
#include <QSqlQuery>

//#include "commdatastruct.h"
#include "securitys.h"
#include "global.h"

//固定资产类别类
class GdzcClass{
public:
    GdzcClass(int code,QString name,int zjMonths):
        code(code),name(name),zjMonths(zjMonths){}
    int getCode(){return code;}
    QString getName(){return name;}
    int getZjMonth(){return zjMonths;}

private:
    int code;      //类别代码
    QString name;  //类别名称
    int zjMonths;   //折旧年限（以月份计）
};

//固定资产类
class Gdzc{
public:
    //折旧信息条目的编辑状态
    enum ZjItemState{
        INIT =    1,  //从gdzczjs表中读取后，未修改过，或修改后保存了
        CHANGED = 2,  //从gdzczjs表中读取后，被修改过
        NEW  =    3,  //新增的条目
        DELETED = 4   //被删除的条目
    };

    struct GdzcZjInfoItem{
        GdzcZjInfoItem(){}
        GdzcZjInfoItem(QDate d,double v,ZjItemState state=INIT,int pid=0,
                       int bid=0,int id=0):
            id(id),state(state),date(d),v(v),bid(bid),pid(pid){}
        int id;             //gdzczjs表中的id
        ZjItemState state;  //条目状态
        int pid;            //属于此固定资产折旧的凭证id
        int bid;            //属于此固定资产折旧的会计分录id
        QDate date;         //凭证日期
        double v;           //折旧价
    };

    Gdzc();
    Gdzc(int id,int code,GdzcClass* productClass,int subCls, QString name,QString model,QDate buyDate,
         double prime,double remain, double min,int zjMonths=0,QString otherInfo="");
    GdzcClass* getProductClass(){return productClass;}
    void setProductClass(GdzcClass* productClass);
    int getSubClass(){return subClass;}
    void setSubClass(int subCls){subClass=subCls;}
    QString getName(){return name;}
    void setName(QString name){this->name = name;}
    QString getModel(){return model;}
    void setModel(QString model){this->model=model;}
    QDate getBuyDate(){return buyDate;}
    void setBuyDate(QDate date){buyDate=date;}
    double getPrimeValue(){return primev;}
    void setPrimeValue(double v){primev=v;}
    double getRemainValue(){return remainv;}
    void setRemainValue(double v){remainv=v;}
    double getMinValue(){return minv;}
    void setMinValue(double v){minv=v;}
    int getZjMonths(){return zjMonths;}
    void setZjMonths(int months){zjMonths=months;}
    double getZjValue(){return primev / zjMonths;}
    QString getOtherInfo(){return otherInfo;}
    void setOtherInfo(QString info){otherInfo=info;}
    bool isStartZj(){if(primev==remainv) return false; else return true;}

    QList<GdzcZjInfoItem*> getZjInfos();
    //QList<GdzcZjInfoItem*> getAllZjInfos();
    bool addZjInfo(QDate d, double v, int pid=0, int bid=0);
    bool addZjInfo(GdzcZjInfoItem* item);
    bool removeZjInfo(int index);
    bool removeZjInfo(QString ds);
    void setZjDate(QString ds, int index);
    void setZjValue(double v, int index);
    bool isZjComplete();


    void save();
    void remove();

    static bool getSubClasses(QHash<int,QString>& names);
    static bool getSubClsToGs(QHash<int,int>& subIds);
    static bool getSubClsToLs(QHash<int,int>& subIds);
    static bool load(QList<Gdzc *> &accLst, bool isAll = true);
    static bool removeZjInfos(int y, int m, int gid = 0);
    static bool createZjPz(int y,int m,User *user);
    static bool repealZjPz(int y,int m);
    //static bool adjustZjValue(int pid,int bid, double v, bool isAdd = true);

private:
    void loadZjInfos();

    int id;                //固定资产标识（数据库记录的唯一性标识）
    int code;              //固定资产代码（应用内的唯一性标识）
    GdzcClass* productClass;//产品类别
    int subClass;          //科目类别
    QString name;          //名称
    QString model;         //厂牌型号
    QDate buyDate;         //购入日期
    double primev;         //买入价
    double remainv;        //剩余价
    double minv;           //最低限值（残值）
    int zjMonths;          //折旧月数（对于自定义类型参照此，对于其他明确类型，参照固定资产类型的折旧规定）
    QString otherInfo;     //其他说明信息    

    QList<GdzcZjInfoItem*> zjInfos;     //固定资产的折旧信息
    QList<GdzcZjInfoItem*> zjInfoDels;  //删除的固定资产折旧信息
    //QList<int>

};

//待摊费用类别类
class DtfyType{
public:
    DtfyType(int code,QString name):
        code(code),name(name){}
    int getCode(){return code;}
    QString getName(){return name;}

    static bool load(QHash<int,DtfyType*>& l, QSqlDatabase db = QSqlDatabase::database());

private:
    int code;      //类别代码
    QString name;  //类别名称
};

//待摊费用类
class Dtfy
{
public:
    enum FtItemState{
        INIT =    1,  //从gdzczjs表中读取后，未修改过，或修改后保存了
        CHANGED = 2,  //从gdzczjs表中读取后，被修改过
        NEW  =    3,  //新增的条目
        DELETED = 4   //被删除的条目
    };
    //表示某个待摊费用在某个月份的分摊项
    struct FtItem{
        int id;
        int pid;
        int bid;
        FtItemState state;   //编辑状态
        QString date;        //分摊月份
        double v;            //分摊值
    };
    //摊销清单项（表示摊销清单中的一行）
    struct TxmItem{
        QString name;      //待摊费用名称
        double total;      //总值
        double txSum;      //累计摊销
        double remain;     //余值
        double curMonth;   //本月摊销
    };

    //表示待摊费用、计提摊销凭证中的会计分录的一行
    struct BaItemData{
        QString summary;    //摘要
        int fid;            //一级科目id
        int sid;            //二级科目id
        int mt;             //币种
        int dir;            //方向
        double v;           //计提的摊销值
    };

    Dtfy(QSqlDatabase db = QSqlDatabase::database()):t(0),dfid(0),dsid(0),
        db(db),id(0),total(0),remain(0),months(0),pid(0),jbid(0),dbid(0),
        iDate(QDate::currentDate().toString(Qt::ISODate)),
        sDate(QDate::currentDate().toString(Qt::ISODate)){}
    Dtfy(int id,int code,DtfyType* t,QString name,QString impDate,
         double total,double remain,int months,QString sDate,
         int dfid,int dsid,int pid,int jbid,int dbid,QString explain,
         QSqlDatabase db = QSqlDatabase::database())
        :id(id),code(code),t(t),name(name),iDate(impDate),total(total),
          remain(remain),months(months),sDate(sDate),dfid(dfid),dsid(dsid),
          pid(pid),jbid(jbid),dbid(dbid),explain(explain),db(db){}


    int getId(){return id;}
    int getCode(){return code;}
    QString getName(){return name;}
    void setName(QString name){this->name=name;}
    QDate importDate(){return QDate::fromString(iDate,Qt::ISODate);}
    void setImportDate(QDate d){iDate=d.toString(Qt::ISODate);}
    DtfyType* getType(){return t;}
    void setType(DtfyType* t){this->t = t;}
    double totalValue(){return total;}
    void setTotalValue(double v){total=v;}
    double remainValue(){return remain;}
    void setRemainValue(double v){remain=v;}
    int getMonths(){return months;}
    void setMonths(int m){months=m;}
    double getTxValue(){return total/months;}
    QDate startDate(){return QDate::fromString(sDate,Qt::ISODate);}
    void setStartDate(QDate d){sDate = d.toString(Qt::ISODate);}
    int getDFid(){return dfid;}
    void setDFid(int id){dfid=id;}
    int getDSid(){return dsid;}
    void setDSid(int id){dsid=id;}
    int getPid(){return pid;}
    void setPid(int id){pid=id;}
    int getJBid(){return jbid;}
    void setJBid(int id){jbid=id;}
    int getDBid(){return dbid;}
    void setDBid(int id){dbid=id;}
    QString getExPlain(){return explain;}
    void setExplain(QString s){explain = s;}
    QList<FtItem*> getFtItems(){return txs;}
    void setFtItems(QList<FtItem*> items){txs=items;}
    bool isStartTx(){return remain<total;}
    bool isFinishTx(){return remain == 0;}

    bool insertTxInfo(QDate date, double v, int idx, int pid=0, int bid=0);
    bool removeTxInfo(int idx);
    void setTxDate(QString date, int index);
    void setTxValue(double v, int index);

    bool save();
    bool update();
    bool remove();
    void crtNewCode();

    static bool getDtfySubId(int &fid,QHash<int, int> &sids,
                             QSqlDatabase db = QSqlDatabase::database());
    static bool load(QList<Dtfy *> &dl, bool all = false,
                     QSqlDatabase db = QSqlDatabase::database());
    static bool genImpPzData(int y, int m, QList<PzData *> pzds, User *user,
                             QSqlDatabase db = QSqlDatabase::database());
    static bool createTxPz(int y,int m,User *user,
                           QSqlDatabase db = QSqlDatabase::database());
    static bool repealTxPz(int y,int m);
    static bool getFtManifest(int y,int m,QList<TxmItem*>& l,
                              QSqlDatabase db = QSqlDatabase::database());
    static bool genDtfyPzDatas(PzData *data,QSqlDatabase db);
    static bool genPzBaDatas(int y,int m,QList<BaItemData*>& l,
                             QSqlDatabase db = QSqlDatabase::database());

private:
    void saveFtItems();

    int id,code;
    DtfyType* t;
    QString name,sDate,iDate,explain;  //待摊费用名称，开始分摊日期，引入日期，说明信息
    double total,remain;         //总价，剩余值
    int months;                  //分摊月份数
    int dfid,dsid;               //贷方主目和子目的id
    int pid,jbid,dbid;           //引入此待摊费用的凭证id，借贷方会计分录id
    QList<FtItem*> txs;          //已摊销项列表
    QList<FtItem*> dels;         //已删除的摊销项列表

    QSqlDatabase db;
};

#endif // OTHERMODULE_H
