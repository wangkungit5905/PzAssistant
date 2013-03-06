#ifndef COMMDATASTRUCT_H
#define COMMDATASTRUCT_H

#include <QDate>
#include <QHash>

#include "cal.h"
#include "securitys.h"

//凭证状态代码
enum PzState{
    Pzs_Repeal     =    0,    //作废
    Pzs_Recording  =    1,    //初始录入态
    Pzs_Verify     =    2,    //已审核
    Pzs_Instat     =    3,    //已入账（可计入统计）
    Pzs_Max        =    100   //这个数是一个标志，它指示该凭证是刚插入到数据库的，还没有回读它的id

};

//凭证类别代码
enum PzClass{
    Pzc_Hand      =   0,       //手工录入的凭证（由用户添加，并允许用户修改）

    //由其他模块引入的凭证（由其他模块添加，允许人工修改）
    Pzc_GdzcImp   =   11,      //引入固定资产的凭证
    Pzc_GdzcZj    =   12,      //固定资产折旧凭证
    Pzc_DtfyImp   =   14,      //引入待摊费用的凭证
    Pzc_Dtfy      =   15,      //待摊费用分摊凭证
                               //工资
                               //计提税金

    //自动结转凭证（由系统添加，且不允许人工修改）
    Pzc_Jzhd_Bank =   31,      //结转汇兑损益-银行存款
    Pzc_Jzhd_Ys   =   32,      //结转汇兑损益-应收账款
    Pzc_Jzhd_Yf   =   33,      //结转汇兑损益-应付账款
    Pzc_JzsyIn    =   34,      //结转损益（收入类）
    Pzc_JzsyFei   =   35,      //结转损益（费用类）

    //其他需由系统添加，并允许人工修改的凭证
    Pzc_Jzlr      =   50       //结转本年利润到利润分配
};

//银行账户
struct BankAccount{
    bool isMain;     //是否基本户
    QString name;    //账户名（由银行名-币种名组成）
    int mt;          //币种
    QString accNum;  //帐号
};

//保存日记账表格行数据的结构
//金额式表格行
struct RowTypeForJe{
    int y,m,d;       //凭证日期
    QString pzNum;   //凭证号码
    QString summary; //凭证摘要
    int dh;         //本次发生方向
    double v;        //本次发生金额
    double em;       //余额
    int dir;         //余额借贷方向
    int pid;         //该业务活动所属的凭证id
    int sid;         //该业务活动的id（BusiActions表id字段）
};


//外币金额式表格行
struct RowTypeForWj{
    int y,m,d;       //凭证日期
    QString pzNum;   //凭证号码
    QString summary; //凭证摘要
    int dh;          //本次发生方向
    int mt;          //本次发生的币种代码
    double v;        //本次发生金额
    QHash<int,double> em; //分币种余额(key为币种代码)
    double etm;      //各币种合计余额值
    int dir;         //总余额借贷方向
    int pid;         //该业务活动所属的凭证id
    int bid;         //该业务活动的id（BusiActions表id字段）
};

//日记账/明细账单行数据结构
struct DailyAccountData{
    int y,m,d;       //凭证日期
    QString pzNum;   //凭证号码
    QString summary; //凭证摘要
    QString jsNum;   //结算号
    int     oppoSub; //对方科目
    int dh;          //本次发生方向
    int mt;          //本次发生的币种代码
    double v;        //本次发生金额
    int dir;         //总余额借贷方向
    int pid;         //该业务活动所属的凭证id
    int bid;         //该业务活动的id（BusiActions表id字段）
    //这些余额表示，到本次业务活动发生后，该科目的当前余额
    QHash<int,double> em; //按币种分开核算的余额(key为币种代码)
    QHash<int,int> dirs;  //各币种的余额方向
    double etm;           //各币种混合核算的余额值
};

struct DailyAccountData2{
    int y,m,d;       //凭证日期
    QString pzNum;   //凭证号码（有中文字符，因此用QString）
    QString summary; //凭证摘要
    QString jsNum;   //结算号
    int     oppoSub; //对方科目
    int dh;          //本次发生方向
    int mt;          //本次发生的币种代码
    Double v;        //本次发生金额
    int dir;         //总余额借贷方向
    int pid;         //该业务活动所属的凭证id
    int bid;         //该业务活动的id（BusiActions表id字段）
    //这些余额表示，到本次业务活动发生后，该科目的当前余额
    QHash<int,Double> em; //按币种分开核算的余额(key为币种代码)
    QHash<int,int> dirs;  //各币种的余额方向
    Double etm;           //各币种混合核算的余额值
};

//总账单行数据结构
struct TotalAccountData{
    int y,m;  //帐套年份、月份
    QHash<int,double> jvh; //本月借方合计（键均为币种代码）
    double jv;             //本月借方合计（用母币计）
    QHash<int,double> dvh; //本月贷方合计金额
    double dv;             //本月贷方合计（用母币计）    
    QHash<int,double> evh; //余额
    double ev;             //余额（用母币计）
    QHash<int,int> dirs;   //余额方向
    int dir;               //余额方向（用母币计）

};

struct TotalAccountData2{
    int y,m;  //帐套年份、月份
    QHash<int,Double> jvh; //本月借方合计（键均为币种代码）
    Double jv;             //本月借方合计（用母币计）
    QHash<int,Double> dvh; //本月贷方合计金额
    Double dv;             //本月贷方合计（用母币计）
    QHash<int,Double> evh; //余额
    Double ev;             //余额（用母币计）
    QHash<int,int> dirs;   //余额方向
    int dir;               //余额方向（用母币计）

};


//表示凭证中的单项业务活动的数据结构
struct BaData{
    QString summary; //摘要
    QString subject; //科目
    int  dir;        //借贷方向
    int mt;          //币种代码
    double v;        //金额
};

struct BaData2{
    QString summary; //摘要
    QString subject; //科目
    int  dir;        //借贷方向
    int mt;          //币种代码
    Double v;        //金额
};

//打印凭证时，每张凭证需包含的数据
struct PzPrintData{
    QDate date;           //凭证日期
    int attNums;          //附件数
    QString pzNum;        //凭证号
    QString pzZbNum;      //自编号
    QList<BaData*> baLst; //凭证业务活动列表
    double jsum,dsum;     //借贷合计值
    int producer;     //制单者
    int verify;       //审核者
    int bookKeeper;   //记账者
};

struct PzPrintData2{
    QDate date;           //凭证日期
    int attNums;          //附件数
    QString pzNum;        //凭证号
    QString pzZbNum;      //自编号
    QList<BaData2*> baLst; //凭证业务活动列表
    Double jsum,dsum;     //借贷合计值
    int producer;     //制单者
    int verify;       //审核者
    int bookKeeper;   //记账者
};

//包含业务活动数据的结构
struct BusiActionData{
    enum ActionState{
        DELETED = -1,  //被删除的
        BLANK   = 0,   //空白行
        INIT = 1,      //初始读入的
        EDITED = 2,    //被修改了
        NEW = 3,       //新增的
        NUMCHANGED = 4 //序号改变了
    };

    BusiActionData(){}
    BusiActionData(BusiActionData* other){
        id = other->id;
        pid = other->pid;
        summary = other->summary;
        fid = other->fid;
        sid = other->sid;
        mt = other->mt;
        dir = other->dir;
        v = other->v;
        num = other->num;
        state = NEW;
    }

    int id;           //该业务活动的
    int pid;          //业务活动所属凭证id
    QString summary;  //摘要
    int fid;          //一级科目id
    int sid;          //二级科目id
    int mt;           //币种
    int dir;          //借贷方向（1：借，0：贷，-1：未定）
    double v;         //金额
    int num;          //该业务活动在凭证中的序号
    ActionState state;//业务活动状态
};

//包含业务活动数据的结构(用Double类代替double数据类型)
struct BusiActionData2{
    enum ActionState{
        DELETED = -1,  //被删除的
        BLANK   = 0,   //空白行
        INIT = 1,      //初始读入的
        EDITED = 2,    //被修改了
        NEW = 3,       //新增的
        NUMCHANGED = 4 //序号改变了
    };

    BusiActionData2(){}
    BusiActionData2(BusiActionData2* other){
        id = other->id;
        pid = other->pid;
        summary = other->summary;
        fid = other->fid;
        sid = other->sid;
        mt = other->mt;
        dir = other->dir;
        v = other->v;
        num = other->num;
        state = NEW;
    }

    int id;           //该业务活动的
    int pid;          //业务活动所属凭证id
    QString summary;  //摘要
    int fid;          //一级科目id
    int sid;          //二级科目id
    int mt;           //币种
    int dir;          //借贷方向（1：借，0：贷，-1：未定）
    Double v;         //金额
    int num;          //该业务活动在凭证中的序号
    ActionState state;//业务活动状态
};

enum PzEditState{
    PZDELETEED    = -1,   //凭证被删除了
    PZINIT        = 0,    //凭证刚从数据库中读取，或已经执行了保存
    PZINFOEDITED  = 1,    //凭证信息内容被编辑了（这些内容的改变不会影响统计结果）
    PZVALUEEDITED = 2,    //凭证的金额发生了改变
    PZNEW         = 3     //凭证是新的，数据表中还没有对应记录，需要执行一次插入操作
};

struct PzData{
    int pzId;                     //凭证id
    QString date;                 //凭证日期
    int attNums;                  //附件数
    int pzNum;                    //凭证号
    int pzZbNum;                  //自编号
    QList<BusiActionData*> baLst; //凭证业务活动列表
    double jsum,dsum;             //借贷合计值
    PzEditState editState;        //凭证编辑状态
    PzState state;                //凭证状态
    PzClass pzClass;              //凭证类别
    User* producer;               //制单者
    User* verify;                 //审核者
    User* bookKeeper;             //记账者
};

struct PzData2{
    int pzId;                     //凭证id
    QString date;                 //凭证日期
    int attNums;                  //附件数
    int pzNum;                    //凭证号
    int pzZbNum;                  //自编号
    QList<BusiActionData2*> baLst; //凭证业务活动列表
    Double jsum,dsum;             //借贷合计值
    PzEditState editState;        //凭证编辑状态
    PzState state;                //凭证状态
    PzClass pzClass;              //凭证类别
    User* producer;               //制单者
    User* verify;                 //审核者
    User* bookKeeper;             //记账者
};

//MDI子窗口位置和尺寸信息
struct SubWindowDim{
    int x,y;  //子窗口位置
    int w,h;  //子窗口大小
};

//账户简要信息，此信息来自于基本数据库的AccountInfos表
struct AccountBriefInfo{
    int id;            //账户id
    QString code;      //账户代码
    //int usedSubSys;    //--账户所使用的科目系统（1：老式，2：新式）
    //int usedRptType;   //--账户所使用的报表类型（未来考虑）
    //QString baseTime;  //--账户开始记账年月
    QString fileName;  //账户数据库文件名
    QString accName;   //账户简称
    QString accLName;  //账户全称
    //QString lastTime;  //--账户最后修改时间
    //QString desc;      //--说明账户最后修改状态的说明性信息
};

//凭证错误级别
enum PingZhengErrorLevel{
    PZE_WARNING   = 1,       //警告级
    PZE_ERROR     = 2        //错误级
};

//凭证错误信息
struct PingZhengError{
    PingZhengErrorLevel errorLevel;      //错误级别
    int errorType;       //错误类型
    int pzNum;           //凭证号
    int baNum;           //会计分录序号
    int pid,bid;         //凭证id和会计分录id
    QString extraInfo;   //额外补充信息
    QString explain;     //描述错误的信息
};

#endif // COMMDATASTRUCT_H
