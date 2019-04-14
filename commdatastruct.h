#ifndef COMMDATASTRUCT_H
#define COMMDATASTRUCT_H

#include <QDate>
#include <QHash>
#include <QBitArray>

#include "cal.h"

class PingZheng;
class BusiAction;
class User;
class FirstSubject;
class WorkStation;
class NameItemAlias;


const int UNID      = 0;    //无意义的id值，比如对于新创建但还未保存的二级科目对象的id值
const int UNCLASS   = 0;    //未知的分类
const int ALLCLASS  = 0;    //所有类别


//变量值类型枚举
enum VariableType{
    VT_BOOLEAN    = 1, //布尔型
    VT_INTEGER = 2, //整形
    VT_FLOAT   = 3, //浮点型
    VT_STRING  = 4  //字符串
};

//剪贴板操作
enum ClipboardOperate{
    CO_COPY     = 1,
    CO_CUT      = 2,
    CO_PASTER   =3
};

//智能提示列表框项目的排序模式
enum SortByMode{
    SORTMODE_NONE       = 0,     //自然顺序（通常是记录的id或随机顺序）
    SORTMODE_NAME       = 1,     //按名称字符
    SORTMODE_CODE       = 2,     //按科目代码
    SORTMODE_REMCODE    = 3,     //按助记符
    SORTMODE_CRT_TIME   = 4      //创建时间
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
    ES_NI_SNAME = 0x02,     //名称条目简称被修改
    ES_NI_LNAME = 0x04,     //名称条目全称被修改
    ES_NI_SYMBOL = 0x08,    //名称条目助记符被修改
    ES_NI_ALIAS = 0x10      //别名改变（仅涉及到别名的增加，不记录对别名本身的改变）
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
    ES_FS_CHILD     = 0x400,    //从属的二级科目
    ES_FS_DEFSUB    = 0x800     //默认二级科目
};
Q_DECLARE_FLAGS(FirstSubjectEditStates, FirstSubjectEditState)
Q_DECLARE_OPERATORS_FOR_FLAGS(FirstSubjectEditStates)

//二级科目编辑状态
enum SecondSubjectEditState{
    ES_SS_INIT      = 0x0,  //未做任何修改
    ES_SS_FID       = 0x01, //父科目
    ES_SS_NID       = 0x02, //科目所用的名称条目id
    ES_SS_CODE      = 0x04, //科目代码
    ES_SS_WEIGHT    = 0x08, //权重
    ES_SS_ISENABLED = 0x10, //是否启用
    ES_SS_DISABLE   = 0x20, //禁用时间
    ES_SS_CTIME     = 0x40, //创建时间
    ES_SS_CUSER     = 0x80  //创建者
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
    Pzs_NULL       =    -1,   //空状态
    Pzs_Repeal     =    0,    //作废
    Pzs_Recording  =    1,    //初始录入态
    Pzs_Verify     =    2,    //已审核
    Pzs_Instat     =    3,    //已入账（可计入统计）
    Pzs_Max        =    100   //这个数是一个标志，它指示该凭证是刚插入到数据库的，还没有回读它的id

};
Q_DECLARE_METATYPE(PzState)

//凭证类别代码
enum PzClass{
    Pzc_NULL      = -1,        //空凭证
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

/**
 * @brief 一级科目大类类别枚举
 */
enum SubjectClass{
    SC_NULL = -1,       //不属于任何类别
    SC_ALL  = 0,        //所有
    SC_ZC   = 1,        //资财类
    SC_FZ   = 2,        //负债类
    SC_QY   = 3,        //所有者权益类
    SC_CB   = 4,        //成本类
    SC_SY   = 5,        //损益类
    SC_GT   = 6         //共同类
};

////凭证子类别
//enum PzSubClass{
//    Pzc_sub_jzhd_bank   = 1,    //结转银行存款
//    Pzc_sub_jzhd_ys     = 2,    //结转应收账款
//    Pzc_sub_jzhd_yf     = 3,    //结转应付账款
//    Pzc_sub_jzhd_yus    = 4,    //结转预收账款
//    Pzc_sub_jzhd_yuf    = 5     //结转预付账款
//};

class SubjectNameItem;
class SecondSubject;
struct Bank;

struct BankAccount{
    int id;
    Bank* parent;
    Money* mt;              //该账户所对应的币种
    QString accNumber;      //帐号
    SubjectNameItem* niObj; //对应的名称条目对象（这个域在Account对象初始化阶段就要设置）
    //SecondSubject* subObj;  //对应的二级科目对象（这个域只在科目管理器返回此结构时设置）
};

struct Bank{
    int id;
    bool isMain;                //是否基本户
    QString name,lname;         //简称和全称
    QList<BankAccount*> bas;    //该开户行下开设的账户
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
    QHash<int,Double> em; //按币种分开核算的原币余额(key为币种代码)
    QHash<int,Double> mm; //存放外币原币余额对应的本币值(key为币种代码)
    QHash<int,int> dirs;  //各币种的余额方向
    Double etm;           //各币种混合核算的余额值
};

//明细账视图搜寻条件结构：
struct DVFilterRecord{
    CommonItemEditState editState;  //
    int id;
    int suiteId;                    //帐套id
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

/**
 * @brief The PzPrintData struct
 *  打印一页凭证需要的数据结构（如果一个凭证对象包含的分录对象超出了一页凭证可拥有的最大行数，则用多个此对象表示属于同一张凭证的凭证数据）
 */
struct PzPrintData{
    QDate date;           //凭证日期
    int attNums;          //附件数
    QString pzNum;        //凭证号
    QString pzZbNum;      //自编号
    QList<BusiAction*> baLst; //凭证业务活动列表
    Double jsum,dsum;     //借贷合计值
    User* producer;     //制单者
    User* verify;       //审核者
    User* bookKeeper;   //记账者
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

//账户转移状态枚举类型（待引入账户的转入转出功能后，移到transform.h文件中）
enum AccountTransferState{
    ATS_INVALID     =   0,      //无效状态
    ATS_TRANSOUTED  =   1,      //已转出
    ATS_TRANSINDES  =   2,      //已转入目标主机
    ATS_TRANSINOTHER =  3       //已转入其他主机
};

//主机类型
enum MachineType{
   MT_COMPUTER  = 1,    //物理电脑
   MT_CLOUDY    = 2     //云账户
};

/**
 * @brief 账户缓存条目结构（与本地账户缓存表对应）
 */
struct AccountCacheItem{
    AccountCacheItem(){}
    AccountCacheItem(AccountCacheItem& other){
        code = other.code;
        fileName = other.fileName;
        accName = other.accName;
        accLName = other.accLName;
        inTime = other.inTime;
        outTime = other.outTime;
        s_ws = other.s_ws;
        tState = other.tState;
        lastOpened = other.lastOpened;
    }

    int id;
    QString code;       //账户代码
    QString fileName;   //账户数据库文件名
    QString accName;    //账户简称
    QString accLName;   //账户全称
    QDateTime inTime;   //转入时间（最近一次转入账户到本主站的时间，三种转移状态下都有意义）
    QDateTime outTime;  //转出时间（已转出时有意义，其他状态无意义）
    WorkStation* s_ws;      //转出源站  要转入的目的主机（已转出），转出此账户的源主机（转入到目的机或其他机）
    WorkStation* d_ws;      //转入目的站
    AccountTransferState tState; //转移状态
    bool lastOpened;    //是否是最后打开的账户
};

/**
 * @brief 科目系统名称条目
 */
struct SubSysNameItem{
    int code;               //科目系统代码
    QString name,explain;   //科目系统名称及其解释
    QDate startTime;        //科目系统的启用时间
    bool isImport;          //是否已导入给科目系统的科目
    bool isConfiged;        //是否已终结科目系统的衔接配置
};

/**
 * @brief 科目系统对接配置条目结构
 */
struct SubSysJoinItem{
    int id;                      //记录id
    FirstSubject *sFSub, *dFSub; //源一级科目，目的一级科目
    bool isDef;                  //true：默认对接，false：混合（并入）对接
};

/**
 * @brief 科目系统对接配置条目结构
 */
struct SubSysJoinItem2{
    int id;                     //记录id
    QString scode,dcode;        //源一级科目代码，目的一级科目代码
    bool isDef;                 //true：默认对接，false：混合（并入）对接
};

/**
 * @brief 帐套记录结构
 */
struct AccountSuiteRecord{
    int id;
    int year,recentMonth;       //帐套所属年份、最后打开月份
    int startMonth,endMonth;    //开始月份和结束月份
    int subSys;                 //帐套采用的科目系统代码
    QString name;               //帐套名
    bool isClosed;              //是否已关账
    bool isUsed;                //帐套是否已启用
    bool isCur;                 //是否当前帐套

    bool operator !=(const AccountSuiteRecord& other);
};

//凭证错误级别
enum PingZhengErrorLevel{
    PZE_WARNING   = 1,       //警告级
    PZE_ERROR     = 2        //错误级
};

//凭证错误信息（由凭证集检错方法使用）
struct PingZhengError{
    PingZhengErrorLevel errorLevel;      //错误级别
    int errorType;       //错误类型
    PingZheng* pz;       //出现错误的凭证对象
    BusiAction* ba;      //出现错误的分录对象
    QString extraInfo;   //额外补充信息
    QString explain;     //描述错误的信息
};

//可以在MDI区域打开的子窗口类型代码
enum subWindowType{
    SUBWIN_NONE         = 0,    //不指代任何子窗口类型
    SUBWIN_PZEDIT       = 1,    //凭证编辑窗口（新）
    SUBWIN_PZSTAT       = 2,    //本期统计窗口（新）
    SUBWIN_DETAILSVIEW  = 3,    //明细账视图（新）
    TOTALDAILY = 6,             //总分类账窗口
    SUBWIN_SETUPBASE  = 7,      //设置账户期初余额窗口
    SUBWIN_BASEDATAEDIT = 9,    //基本数据库编辑窗口
    SUBWIN_GDZCADMIN =  10,     //固定资产管理窗口
    //DTFYADMIN = 11,           //待摊费用管理窗口
    SUBWIN_TOTALVIEW = 12,      //总账视图
    SUBWIN_HISTORYVIEW = 14,    //历史凭证
    SUBWIN_LOOKUPSUBEXTRA =15,  //查看科目余额
    SUBWIN_ACCOUNTPROPERTY=16,  //查看账户属性
    SUBWIN_VIEWPZSETERROR=17,   //查看凭证错误窗口
    SUBWIN_SQL = 18,            //SQL工具窗口
    SUBWIN_OPTION = 19,         //选项窗口
    SUBWIN_TAXCOMPARE = 20,     //税金比对
    SUBWIN_SECURITY = 21,       //安全模块配置窗口
    SUBWIN_NOTEMGR = 22,        //笔记管理
    SUBWIN_EXTERNALTOOLS = 23,  //外部工具
    SUBWIN_LOGVIEW = 24,        //日志视图
    SUBWIN_YSYFSTAT = 25,       //应收应付发票统计
    SUBWIN_INCOST = 26,         //收入/成本发票管理
    SUBWIN_PZSEARCH = 27,       //凭证搜索对话框
    SUBWIN_QUARTERSTAT = 28,    //季度统计
    SUBWIN_YEAR = 29,           //年度累计
    SUBWIN_IMPJOURNAL = 30,     //导入流水账
    SUBWIN_JOURNALIZING = 31,   //流水分录预览调整
    SUBWIN_JXTAXMGR = 32        //进项税管理
    //设置期初余额的窗口
    //科目配置窗口
};

enum PrintActionClass{
    PAC_NONE        = 0,    //未知打印动作
    PAC_TOPRINTER   = 1,    //输出到打印机
    PAC_PREVIEW     = 2,    //打印预览
    PAC_TOPDF       = 3,    //输出到pdf文件
    PAC_TOEXCEL     = 4     //输出到Excel文件
};

struct MixedJoinCfg{
    int s_fsubId;   //源一级科目id
    int s_ssubId;   //源二级科目id
    int d_fsubId;   //对接一级科目id
    int d_ssubId;   //对接二级科目id
};

/**
 * @brief 凭证打印模板参数
 */
struct PzTemplateParameter{
    double titleHeight;        //分录表标题条高度（高度单位是毫米）
    double baRowHeight;        //分录行高度（所有行高总和）
    int baRows;             //分录最大行数
    double factor[5];       //分录表格列宽分配因子（从左到右分别是摘要栏、科目栏、借贷方、外币、汇率列）
    int cutAreaHeight;      //两张凭证之间的裁剪区域高度
    int topBottonMargin;    //凭证的上下边界高度
    int leftRightMargin;    //凭证的左右边界宽度
    int fontSize;           //分录表字体尺寸
    bool isPrintCutLine;    //是否打印裁剪线
    bool isPrintMidLine;    //是否打印中心线
};

/**
 * @brief 笔记结构
 */
struct NoteStruct{
    int id;
    qint64 crtTime,lastEditTime;
    QString title;
    QString content;
};
Q_DECLARE_METATYPE(NoteStruct*)

/**
 * @brief 外部工具配置项
 */
struct ExternalToolCfgItem{
    int id;
    QString name,commandLine,parameter;
};

/**
 * @brief 以文本方式序列化对象的字段索引
 */

enum SerialObjectFieldIndex{
    //Right Type Object
    SOFI_RT_PC = 0,
    SOFI_RT_CODE = 1,
    SOFI_RT_NAME = 2,
    SOFI_RT_DESC = 3,

    //Right Object
    SOFI_RIGHT_CODE = 0,
    SOFI_RIGHT_RT = 1,
    SOFI_RIGHT_NAEM = 2,
    SOFI_RIGHT_DESC = 3,

    //User Group Object
    SOFI_GROUP_CODE = 0,
    SOFI_GROUP_NAME = 1,
    SOFI_GROUP_RIGHTS = 2,
    SOFI_GROUP_OWNER = 3,
    SOFI_GROUP_DESC = 4,

    //User Object
    SOFI_USER_CODE = 0,
    SOFI_USER_ISENABLED = 1,
    SOFI_USER_NAME = 2,
    SOFI_USER_PASSWORD = 3,
    SOFI_USER_GROUPS = 4,
    SOFI_USER_ACCOUNTS = 5,
    SOFI_USER_DISRIGHTS = 6,

    //WorkStation Object 101||1||0||本机Pc（W）||家里的桌面电脑||1
    SOFI_WS_MID = 0,
    SOFI_WS_TYPE = 1,
    SOFI_WS_ISLOCAL = 2,
    SOFI_WS_NAME = 3,
    SOFI_WS_DESC = 4,
    SOFI_WS_OSTYPE = 5
};

/**
 * @brief 基本库版本类型枚举
 * 对于数据库结构版本，每次当基本库中表结构发生改变，或增删表时，要递增版本号，
 * 其他版本类型记录该表的记录内容改变，这些表中的记录是应用正常运行所需的
 */
enum BaseDbVersionEnum{
    BDVE_DB = 1,            //数据库结构版本
    BDVE_RIGHTTYPE = 2,     //权限类型
    BDVE_RIGHT = 3,         //权限
    BDVE_GROUP = 4,         //组
    BDVE_USER = 5,          //用户
    BDVE_WORKSTATION = 6,   //工作站
    BDVE_SPECSUBJECT = 7,   //特定科目
    BDVE_COMMONPHRASE = 11  //常用提示短语
};

/**
 * @brief 常用提示短语类别
 */
enum CommonPromptPhraseClass{
    CPPC_TRAN_IN    = 1,
    CPPC_TRAN_OUT   = 2
};

/**
 * @brief 发票的销账状态（即应收是否收入，应付是否付了）
 */
enum CancelAccountState {
    CAS_NONE = 0,   //未销账
    CAS_PARTLY = 1, //部分销账
    CAS_OK = 2      //已销账
};

/**
 * @brief 发票使用记录
 */
struct InvoiceRecord{
    int id;
    int year,month;
    bool isIncome;              //true：收入，false：成本
    bool isCommon;              //true：普票，false：专票
    QString invoiceNumber;      //发票号
    SubjectNameItem* customer;  //关联客户
    int pzNumber;               //凭证号
    int baRID;                  //分录记录的id
    Double money,wmoney;        //账面金额（本币金额），外币金额
    Money* wmt;                 //外币币种（通常是美金）
    Double taxMoney;            //税额
    CancelAccountState state;   //发票销账状态

    InvoiceRecord(){
        id = 0;
        year=0; month=0;
        isIncome=true;isCommon=true;
        customer=0;pzNumber=0;
        baRID=0;/*money=0;wmoney=0;*/
        wmt=0;state=CAS_NONE;
    }
};

/**
 * @brief 本月收入/成本发票表格预定义列类型枚举
 */
enum CurInvoiceColumnType{
    CT_NONE = 0,        //未指定（不会利用的列）
    CT_NUMBER   = 1,    //序号
    CT_DATE     = 2,    //开票日期
    CT_INVOICE  = 3,    //发票号*
    CT_CLIENT   = 4,    //客户名*
    CT_MONEY    = 5,    //发票金额*
    CT_TAXMONEY = 6,    //税额
    CT_WBMONEY  = 7,    //外币金额
    CT_ICLASS   = 8,    //发票类别
    CT_SFINFO = 9       //收/付款情况
};

/**
 * @brief 本月收入/成本发票记录
 */
struct CurInvoiceRecord{
    int id;
    int y,m;                        //所属年月
    bool isIncome,type;             //1收入/0成本,1专票/0普票
    int processState;               //处理状态（0：凭证中未发现对应发票，1：发现且正确，2：发现但有问题）
    PingZheng* pz;                  //出现此发票对应的凭证
    BusiAction* ba;                 //出现此发票对应的分录（对应主营收入/成本，或在聚合凭证中是应收/应付）
    int num;                        //序号
    QString inum,client,sfInfo,errors;     //发票号,单位名称，收/付情况，错误信息
    QDate date;                     //开票日期
    QString dateStr;                //保存表达日期的原始字符串（日期列无法识别时保存原始字符串）
    SubjectNameItem* ni;            //匹配的名称对象
    NameItemAlias* alias;           //匹配的别名对象
    Double money,taxMoney,wbMoney;  //发票金额,税额,外币金额
    int state;                      //发票性质（1正常、2作废、3冲红）
    QBitArray* tags;                //修改标记
    CurInvoiceRecord(){
        id=0;y=0;m=0;
        isIncome=true;type=true;
        processState=0;
        ni=0;alias=0;state=1;
        tags = new QBitArray(16);
        pz=0;ba=0;
    }
};
Q_DECLARE_METATYPE(CurInvoiceRecord*)

/**
 * @brief The JtpzDatas struct
 * 创建记提凭证所需要的数据记录
 */
struct JtpzDatas{
    FirstSubject* jFsub=0,*dFsub=0;  //借方一级科目，贷方一级科目
    SecondSubject* jSsub=0,*dSsub=0; //借方二级科目，贷方二级科目
    SecondSubject* dSsub2=0;         //贷方二级科目（记提增值税时有2个贷方）
    QString summary;                 //分录摘要
    Double value=0.0;                //金额
    int group=0;                     //所属组（一个组代表一张凭证，一般记提凭证有3张即有3组）
};


enum CurInvoiceRecordModifyTag{
    CI_TAG_ISINCOME = 0,
    CI_TAG_TYPE     = 1,
    CI_TAG_ISPROCESS= 2,
    CI_TAG_SFSTATE  = 3,
    CI_TAG_NUMBER   = 4,
    CI_TAG_INUMBER  = 5,
    CI_TAG_CLIENT   = 6,
    CI_TAG_NAMEITEM = 7,
    CI_TAG_DATE     = 8,
    CI_TAG_MONEY    = 9,
    CI_TAG_TAXMONEY = 10,
    CI_TAG_WBMONEY  = 11,
    CI_TAG_STATE    = 12,
};

/**
 * @brief 分录模板类型枚举
 */
enum BATemplateEnum{
    BATE_BANK_INCOME = 1,
    BATE_YS_INCOME = 2,
    BATE_BANK_COST = 3,
    BATE_YF_COST = 4,
    BATE_YS_GATHER = 5,
    BATE_YF_GATHER = 6
};

/**
 * @brief 输出到Excel表格的列的值类型
 */
enum TableColValueType{
    TCVT_TEXT   = 0,
    TCVT_INT    = 1,
    TCVT_DOUBLE = 2,
    TCVT_BOOL   = 3
};


/**
 * 银行/现金流水账结构
 */
struct Journal{
    int id;
    int priNum;       //原始序号，基于1
    QString date;     //日期
    int bankId;       //科目id
    Money* mt;        //币种
    MoneyDirection dir;    //收入1/支出-1
    QString summary;  //摘要
    Double value;     //金额
    Double balance;   //余额
    QString invoices; //发票号
    QString remark;   //备注
    bool vTag;        //审核标志（初值为0，1：表通过）
    int oppoId;       //相对方流水账id（比如对于银行间划款或提现，都必须将对方流水并入此流水算一次）
    int startPos;     //组的第一条分录所在索引位置
    int numOfBas;     //组包含的分录数目
    Journal(){
        id = 0;
        priNum = 0;
        bankId = 0;
        mt = 0;
        dir = MDIR_P;
        vTag = false;
        oppoId = 0;
        startPos = -1;
        numOfBas = 0;
    }
};
Q_DECLARE_METATYPE(Journal*)

/**
 * 流水账分录结构
 */
struct Journalizing{
    int id;
    Journal* journal;     //分录对应流水帐
    int gnum;             //组序号
    int numInGroup;       //组内序号
    int pnum;             //凭证号
    QString summary;
    FirstSubject* fsub;
    SecondSubject* ssub;
    Money* mt;
    MoneyDirection dir;
    Double value;
    bool changed;
    Journalizing(){
        id = 0;
        journal = 0;
        gnum = 0;
        numInGroup = 0;
        pnum = 0;
        fsub = 0;
        ssub = 0;
        mt = 0;
        dir = MDIR_P;
        changed = false;
    }
};
Q_DECLARE_METATYPE(Journalizing*)


/**
 * @brief
 */
enum BaUpdateColumn{
    BUC_SUMMARY     = 0x01,
    BUC_FSTSUB      = 0x02,
    BUC_SNDSUB      = 0x04,
    BUC_MTYPE       = 0x08,
    BUC_VALUE       = 0x10,
    BUC_ALL         = 0x12
};
Q_DECLARE_FLAGS(BaUpdateColumns, BaUpdateColumn)
Q_DECLARE_OPERATORS_FOR_FLAGS(BaUpdateColumns)


#endif // COMMDATASTRUCT_H
