#ifndef COMMDATASTRUCT_H
#define COMMDATASTRUCT_H

#include <QDate>
#include <QHash>

#include "cal.h"
#include "securitys.h"

const int UNID      = 0;    //无意义的id值，比如对于新创建但还未保存的二级科目对象的id值
const int UNCLASS   = 0;    //未知的分类
const int ALLCLASS  = 0;    //所有类别


//剪贴板操作
enum ClipboardOperate{
    CO_COPY     = 1,
    CO_CUT      = 2,
    CO_PASTER   =3
};

//智能提示列表框项目的排序模式
enum SortByMode{
    SM_NAME   = 1,     //按名称字符
    SM_CODE   = 2,     //按科目代码
    SM_REMCOD = 3      //按助记符
};

//应用错误级别名称
enum AppErrorLevel{
    AE_OK            = 0,       //正常信息
    AE_WARNING       = 1,       //警告信息（影响操作的顺利运行）
    AE_CRITICAL      = 2,       //临界错误（影响操作的正确结果）
    AE_ERROR         = 3        //致命错误（影响应用程序的运行，可能导致崩溃）
};

//通用项目编辑状态
enum CommonItemEditState{
    CIES_PENDING  = -1,    //未决的（比如数据库表中还没有与此对象对应的记录）
    CIES_INIT     = 0,     //初始的（从数据库表中读取并创建该对象后未做更改）
    CIES_NEW      = 1,     //新增的（对象是新增的，但在数据库表中已存在对应记录）
    CIES_CHANGED  = 2,     //改变了
    CIES_DELETED  = 3,     //删除了
    CIES_DELETEDAFTERCHANGED = 4 //修改后被删除
};

//名称条目编辑状态
enum NameItemEditState{
    ES_NI_INIT = 0x0,       //未做任何修改
    ES_NI_CLASS = 0x01,     //名称条目类别改变
    ES_NI_SNAME = 0x02,     //名称条目的基本信息被修改
    ES_NI_LNAME = 0x04,     //名称条目的子目被修改
    ES_NI_SYMBOL = 0x08     //名称条目助记符
};
Q_DECLARE_FLAGS(NameItemEditStates, NameItemEditState)
Q_DECLARE_OPERATORS_FOR_FLAGS(NameItemEditStates)

//一级科目编辑状态
enum FirstSubjectEditState{
    ES_FS_INIT      = 0x0,      //未做任何修改
    ES_FS_CODE      = 0x01,     //科目代码
    ES_FS_SYMBOL    = 0x02,     //助记符
    ES_FS_CLASS     = 0x04,     //科目所属类别
    ES_FS_JDDIR     = 0x08,     //记账方向
    ES_FS_ISENABLED = 0x10,     //是否启用
    ES_FS_ISUSEWB   = 0x20,     //是否使用外币
    ES_FS_WEIGHT    = 0x40,     //权重
    ES_FS_NAME      = 0x80,     //科目名称
    ES_FS_DESC      = 0x100,    //简要描述
    ES_FS_USAGE     = 0x200,    //用例说明
    ES_FS_CHILD     = 0x400     //从属的二级科目
};
Q_DECLARE_FLAGS(FirstSubjectEditStates, FirstSubjectEditState)
Q_DECLARE_OPERATORS_FOR_FLAGS(FirstSubjectEditStates)

//二级科目编辑状态
enum SecondSubjectEditState{
    ES_SS_INIT = 0x0,       //未做任何修改
    ES_SS_CODE = 0x01,      //二级科目的基本信息被修改
    ES_SS_WEIGHT = 0x02,    //二级科目的子目被修改
    ES_SS_ISENABLED = 0x04  //是否启用
};
Q_DECLARE_FLAGS(SecondSubjectEditStates, SecondSubjectEditState)
Q_DECLARE_OPERATORS_FOR_FLAGS(SecondSubjectEditStates)

//金额方向枚举
enum MoneyDirection{
    MDIR_J  =   1,
    MDIR_P  =   0,
    MDIR_D  =   -1
};
Q_DECLARE_METATYPE(MoneyDirection)

//凭证状态代码
enum PzState{
    Pzs_Repeal     =    0,    //作废
    Pzs_Recording  =    1,    //初始录入态
    Pzs_Verify     =    2,    //已审核
    Pzs_Instat     =    3,    //已入账（可计入统计）
    Pzs_Max        =    100   //这个数是一个标志，它指示该凭证是刚插入到数据库的，还没有回读它的id

};
Q_DECLARE_METATYPE(PzState)

//凭证类别代码
enum PzClass{
    Pzc_Hand      =   0,       //手工录入的凭证（由用户添加，并允许用户修改）

    //由其他模块引入的凭证（由其他模块添加，允许人工修改）
    Pzc_GdzcImp   =   11,      //引入固定资产的凭证
    Pzc_GdzcZj    =   12,      //固定资产折旧凭证
    Pzc_DtfyImp   =   14,      //引入待摊费用的凭证
    Pzc_DtfyTx    =   15,      //待摊费用分摊凭证
                               //工资
                               //计提税金

    //自动结转凭证（由系统添加，且不允许人工修改）
    Pzc_Jzhd      =   30,      //这是新的表示结转汇兑损益凭证的类别代码
    Pzc_Jzhd_Bank =   31,      //结转汇兑损益-银行存款
    Pzc_Jzhd_Ys   =   32,      //结转汇兑损益-应收账款
    Pzc_Jzhd_Yf   =   33,      //结转汇兑损益-应付账款
    Pzc_JzsyIn    =   34,      //结转损益（收入类）
    Pzc_JzsyFei   =   35,      //结转损益（费用类）
    Pzc_Jzhd_Yus  =   36,      //结转汇兑损益-预收账款
    Pzc_Jzhd_Yuf  =   37,      //结转汇兑损益-预付账款

    //其他需由系统添加，并允许人工修改的凭证
    Pzc_Jzlr      =   50       //结转本年利润到利润分配
};
Q_DECLARE_METATYPE(PzClass)

////凭证子类别
//enum PzSubClass{
//    Pzc_sub_jzhd_bank   = 1,    //结转银行存款
//    Pzc_sub_jzhd_ys     = 2,    //结转应收账款
//    Pzc_sub_jzhd_yf     = 3,    //结转应付账款
//    Pzc_sub_jzhd_yus    = 4,    //结转预收账款
//    Pzc_sub_jzhd_yuf    = 5     //结转预付账款
//};

//银行账户
//struct BankAccount{
//    bool isMain;     //是否基本户
//    QString name;    //账户名（由银行名-币种名组成）
//    int mt;          //币种
//    QString accNum;  //帐号
//};

struct Bank{
    int id;
    bool isMain;    //是否基本户
    QString name,lname; //简称和全称
};

class SubjectNameItem;
class SecondSubject;

struct BankAccount{
    CommonItemEditState editState;
    int id;
    Bank* bank;             //账户所属银行对象
    Money* mt;              //该账户所对应的币种
    QString accNumber;      //帐号
    SubjectNameItem* niObj; //对应的名称条目对象（这个域在Account对象初始化阶段就要设置）
    SecondSubject* subObj;  //对应的二级科目对象（这个域只在科目管理器返回此结构时设置）
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

//明细账视图搜寻条件结构：
struct DVFilterRecord{
    CommonItemEditState editState;  //
    int id;
    bool isDef;                     //是否是系统默认的过滤条件
    bool isCur;                     //是否是最后关闭窗口时应用的过滤条件
    bool isFst;                     //科目范围是一级科目还是二级科目
    int curFSub;                    //当前一级科目
    int curSSub;                    //当前二级科目
    int curMt;                      //当前币种代码
    QString name;                   //名称
    QDate startDate,endDate;        //开始时间、结束时间    
    QList<int> subIds;              //选定的科目代码列表（如果选定了一个一级科目，则首元素保存此一级科目的id，
                                    //后续跟随二级科目，否者此列表保存所有选择的一级科目id）
};
Q_DECLARE_METATYPE(DVFilterRecord*)





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
//struct BaData{
//    QString summary; //摘要
//    QString subject; //科目
//    int  dir;        //借贷方向
//    int mt;          //币种代码
//    double v;        //金额
//};

//struct BaData2{
//    QString summary; //摘要
//    QString subject; //科目
//    int  dir;        //借贷方向
//    int mt;          //币种代码
//    Double v;        //金额
//};

//打印凭证时，每张凭证需包含的数据
//struct PzPrintData{
//    QDate date;           //凭证日期
//    int attNums;          //附件数
//    QString pzNum;        //凭证号
//    QString pzZbNum;      //自编号
//    QList<BaData*> baLst; //凭证业务活动列表
//    double jsum,dsum;     //借贷合计值
//    int producer;     //制单者
//    int verify;       //审核者
//    int bookKeeper;   //记账者
//};

//struct PzPrintData2{
//    QDate date;           //凭证日期
//    int attNums;          //附件数
//    QString pzNum;        //凭证号
//    QString pzZbNum;      //自编号
//    QList<BaData2*> baLst; //凭证业务活动列表
//    Double jsum,dsum;     //借贷合计值
//    int producer;     //制单者
//    int verify;       //审核者
//    int bookKeeper;   //记账者
//};

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

//本地账户缓存表（LocalAccountCaches)
//字段名
//#define tbl_localAccountCache "LocalAccountCaches"
//#define fld_lac_code "code"                 //账户编码
//#define fld_lac_name "name"                 //账户简称
//#define fld_lac_lname "lname"               //账户全称
//#define fld_lac_filename "fname"            //账户文件名
//#define fld_lac_isLastOpen "isLastOpen"     //
//#define fld_lac_tranState "tstate"          //
//#define fld_lac_tranInTime "tranInTime"     //
//#define fld_lac_tranOutMid "tranOutMid"     //
//#define fld_lac_tranOutTime "tranOutTime"   //
////#define fld_lac_hash "hashValue"            //
//账户简要信息，此信息来自于基本数据库的AccountInfos表
struct AccountBriefInfo{
    int id;                     //账户id
    QString code;               //账户代码
    QString fname;              //账户数据库文件名
    QString sname;              //账户简称
    QString lname;              //账户全称
    bool isRecent;              //是否是最近打开的账户
    //AccountTransferState tstate        //转移状态（待转移功能加入后使用）
    //QDateTime tinTime;          //转入时间
    //int tOutMid;                //转出主机的MID
    //QDateTime tOutTime;         //转出时间
    //QString hashValue;          //账户文件的Hash值
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
