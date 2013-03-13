#ifndef COMMON_H
#define COMMON_H

#include <QPrinter>

//表名代码
//#define FSTSUBTABLE 1         //一级科目
//#define SNDSUBTABLE 2         //二级科目信息表
//#define ACTIONSTABLE 3        //业务活动表
//#define PINGZHENGTABLE 4      //凭证表
//#define FSTSUBCLASSTABLE 5    //一级科目类别表
//#define FSAGENTTABLE 6        //一级科目到二级科目的中间表
//#define SNDSUBCLSTABLE 7      //二级科目类别表
//#define ACCOUNTINFO    8      //账户信息表
//#define EXCHANGERATE   9      //汇率表
//#define ACCOUNTBOOKGROUP 10   //凭证分册类别表
//#define SUJECTEXTRA 11        //科目余额表
//#define REPORTSTRUCTS 12      //报表结构信息表
//#define PZSETSTATE 13         //凭证集状态表
//#define BANKS  14             //开户行帐号信息表
//#define BALANCESHEET 15       //资产负债表
//#define INCOMESTATMENTS_OLD 16    //利润表（老）
//#define INCOMESTATMENTS_NEW 17    //利润表（新）
//#define REPORTADDITIONINFO  18    //报表其他信息表

//一级科目余额方向表（SubjectExtraDirs）
//（各科目的字段名由代表科目类别的字母代码加上科目国标代码组成）
//A（资产类）B（负债类）C（共同类）D（所有者权益类）E（成本类）F（损益类）
#define SED_YEAR 1      //(year INTEGER)
#define SED_MONTH 2     //(month INTEGER) （如果month=12，则表示是年度余额）
#define SED_MT 3        //（mt  INTEGER） 币种代码
#define SED_SUBSTART 4  //第一个科目所对应的字段索引号
// ......               （1：借方，0：平，-1：贷方）


//明细科目余额表（ detailExtras）
#define DE_SEID 1       //对应于表SubjectExtras中的id值(seid INTEGER)
#define DE_FSID 2       //对应于在表FSAgent中的id值(fsid INTEGER)
                        //币种代码（由seid对应的记录所决定）
#define DE_DIR   3      //余额方向（dir INTEGER）
#define DE_VALUE 4      //余额值（value REAL）



//报表结构信息表（ReportStructs）
#define RS_CLSID   1    //报表类别代码（clsid INTEGER）
                        //（1：资产负债表，2：利润表，3：现金流量表，4：所有者权益变动表）
#define RS_TYPE    2    //报表分类代码（tid INTEGER）
                        //（对于不同的报表类别，可能代表不同的意思，
                        //比如，对于利润表，1：代表采用老科目系统的报表格式，2：
                        //代表采用新科目系统的报表格式）
#define RS_VIEWORDER 3   //报表字段显示顺序（viewOrder INTERGER）
#define RS_CALORDER  4   //报表字段计算顺序（calOrder INTEGER）
#define RS_ROWNUMBER 5   //该字段在报表中的对应行数（rowNum INTEGER）
#define RS_FIELDNAME 6  //报表字段名（fname TEXT）
                        //（此字段名在系统内部使用，比如在计算公式中引用）
#define RS_FIELDTITLE 7 //报表字段标题（ftitle TEXT）
                        //（显示在报表左侧的说明报表字段意思的）
#define RS_FIELDFORMULA 8  //报表字段的计算公式（fformula TEXT）
                        //（计算公式有计算项和运算符号组成，所有的计算项由一对方括弧包围）
                        //计算项可以是报表字段名，科目余额表中代表科目余额的字段名
                        //以及其他必要的参考项

//报表附加信息表（ReportAdditionInfo）
//用于保存报表的其他相关信息（比如存放报表数据的表名，报表标题等）
#define RAI_CLSID   1    //报表类别代码（clsid INTEGER）
                        //（1：资产负债表，2：利润表，3：现金流量表，4：所有者权益变动表）
#define RAI_TYPE    2    //报表分类代码（tid INTEGER）
#define RAI_TABLENAME 3  //存放报表数据的表名(tname TEXT)
#define RAI_TITLE 4      //报表标题(title TEXT)
//目前仅此两项

//凭证集状态表（PZSetStates）
#define PZSS_Y 1      //凭证集合的年（year INTEGER）
#define PZSS_M 2      //凭证集合的月（month INTEGER）
#define PZSS_S 3      //凭证集合的状态(state INTEGER)


//固定资产产品类别表
#define GDZCC_CODE 1    //类别代码（code INTEGER）
#define GDZCC_NAME 2    //类别名称（name TEXT）
#define GDZCC_DESC 3    //类别说明信息（desc TEXT）
#define GDZCC_GDZJNY 4  //此类别规定的折旧年限（zjnx INTEGER）

//固定资产表（gdzcs）
#define GDZC_CODE    1  //固定资产代码（code INTEGER）
#define GDZC_PCLS    2  //固定资产所属产品类别代码（pcls INTEGER）
#define GDZC_SCLS    3  //固定资产折旧时使用的明细科目类别代码（scls INTEGER）
#define GDZC_NAME    4  //固定资产名称（name TEXT）
#define GDZC_MODEL   5  //厂牌型号（model ）
#define GDZC_BUYDATE 6  //购买日期（buyDate TEXT）
#define GDZC_PRIMEV  7  //固定资产原值（prime DOUBLE）
#define GDZC_REMAINV 8  //固定资产剩余值（remain DOUBLE）
#define GDZC_MINV    9  //固定资产最小值（残值）（min DOUBLE）
#define GDZC_ZJM     10  //折旧月数（zjMonths INTEGER）
#define GDZC_PID     11 //购入固定资产时的凭证id（pid INTEGER）
#define GDZC_BID     12 //业务活动表对应的id（bid INTEGER）
#define GDZC_DESC    13 //该固定资产的其他详细信息（desc TEXT）

//固定资产折旧表（gdzczjs）
#define GZJ_GID      1  //外键-gdzcs表中的id（gid INTEGER）
#define GZJ_PID      2  //计提的凭证id（pid INTEGER）
#define GZJ_BID      3  //计提的业务会计分录id（bid INTEGER）
#define GZJ_DATE     4  //计提折旧日期（date TEXT）
#define GZJ_PRICE    5  //折旧值（price DOUBLE）

//待摊费用表
#define DTFY_CODE    1  //待摊费用代码（code INTEGER）
#define DTFY_TYPE    2  //待摊费用类别id（tid INTEGER）
#define DTFY_NAME    3  //名称（name TEXT）
#define DTFY_IMPDATE 4  //引入日期-作为凭证日期（impDate TEXT）
#define DTFY_TOTAL   5  //待摊费用总值（total DOUBLE）
#define DTFY_REMAIN  6  //待摊费用余值（remain DOUBLE）
#define DTFY_MONTHS  7  //分摊月份（months INTEGER）
#define DTFY_START   8  //开始分摊时间（startDate TEXT）
#define DTFY_DFID    9  //贷方科目（主目，银行存款或现金）（dfid INTEGER）
#define DTFY_DSID    10  //贷方科目（子目）（dsid INTEGER）
#define DTFY_PID     11 //引入此待摊费用的凭证id（pid INTEGER）
#define DTFY_JBID    12 //引入此待摊费用的凭证的借方会计分录id（jbid INTEGER）
#define DTFY_DBID    13 //引入此待摊费用的凭证的贷方会计分录id（dbid INTEGER）
#define DTFY_EXPLAIN 14 //说明信息（explain TEXT）

//分摊情况表-ftqks
#define FT_DID       1  //外键，对应待摊费用表的id列(did INTEGER)
#define FT_PID       2  //记录此分摊项的凭证id(pid INTEGER)
#define FT_BID       3  //记录此分摊项的会计分录id(bid INTEGER)
#define FT_DATE      4  //分摊日期(date TEXT)
#define FT_VALUE     5  //分摊值(value DOUBL)


//资产负债表


//利润表(老--OldProfits)
#define PROFITS_YEAR  1   //利润表年份（year INTEGER）
#define PROFITS_MONTH 2   //利润表月份（month INTEGER）（月份数为0表示此记录是年报数据）
#define PROFITS_START 3   //利润表字段开始值（fn REAL）（n表示数字）


//利润表(新--NewProfits)

//营业收入（OpratingIncome  REAL）
//（减）营业成本（OpratingCost REAL）
//     营业税金及附加
//     销售费用
//     管理费用
//     财务费用
//     资产减值损失
//（加）公允价值变动损益
//     投资损益
//     （其中）对联营企业和合营企业的投资收益？？？
//营业利润
//（加）营业外收入
//（减）营业外支出
//    （其中）非流动资产处置损失？？？
//利润总额
//（减）所得税费用
//净利润
//  基本每股收益？？
//  稀释每股收益？？



//利润表（旧--OldProfits）
//主营业务收入
//（减）主营业务成本
//     主营业务税金及附加（营业税金及附加）
//主营业务利润
//（加）其他业务利润（其他业务收入-其他业务成本）
//（减）营业费用（销售费用）
//     管理费用
//     财务费用
//营业利润
//（加）投资收益
//     补贴收入
//     营业外收入
//（减）营业外支出
//（加）以前年度损益调整
//利润总额
//（减）所得税
//净利润





//////////////////////////////基础数据库某些表字段信息/////////////////////////
//子窗口信息表（subWinInfos）
#define SWI_ENUM  1   //字窗口类别枚举值（winEnum INTEGER）
#define SWI_X     2   //x,y,w,h是字窗口最后一次关闭时的位置和大小尺寸（x INTEGER）
#define SWI_Y     3   //              （y INTEGER）
#define SWI_W     4   //              （w INTEGER）
#define SWI_H     5   //              （h INTEGER）
#define SWI_TBL   6   //子窗体内表格各列的宽度信息（tblInfo BLOB）

//账户信息表（AccountInfos）此表的字段可以精简，只保留账户代码和文件名
#define ACCIN_CODE       1   //账户代码（code TEXT）
#define ACCIN_BASETIME   2   //开始记账年月
#define ACCIN_USS        3   //账户所采用的科目系统（usedSubSys INTEGER）
#define ACCIN_USRPT      4   //账户所采用的报表类型（usedRptType INTEGER）
#define ACCIN_FNAME      5   //账户对应的数据库文件名（filename TEXT）
#define ACCIN_NAME       6   //账户简称（name TEXT）
#define ACCIN_LNAME      7   //账户详细名称（lname TEXT）
#define ACCIN_LASTTIME 8   //账户最后修改时间（lastTime TEXT）
#define ACCIN_DESC     9   //说明账户最后修改状态的说明性信息（desc TEXT）

//配置信息变量表（configs）
#define CFG_TYPE  1       //变量类型（type INTEGER）
#define CFG_NAME  2       //变量名（name TEXT）（1：整型，2：实型，3：字符串）
#define CFG_VALUE 3       //变量值（value TEXT）

//凭证集状态描述表（pzsStateNames）
//CREATE TABLE pzsStateNames(id INTEGER PRIMARY KEY, code INTEGER, state TEXT)

//凭证状态名表（pzStateNames）
//CREATE TABLE pzStateNames(id INTEGER PRIMARY KEY, code INTEGER, state TEXT)


///////////////////////////////////////////////////////

//货币类型代码
#define ALLMT 0
#define RMB 1
#define USD 2

//科目类别代码
#define SC_ASSETS  1 //资产类
#define SC_DEBT    2 //负债类
#define SC_OWNER   3 //所有者权益类
#define SC_COST    4 //成本
#define SC_PROFITANDLOSS 5 //损益类

////////////可产生凭证的其他模块类别枚举代码////////////////////////////////
enum OtherModCode{
    OM_GDZC      =   1,     //固定资产模块
    OM_DTFY      =   2      //待摊费用模块
};


/////////////凭证集状态代码////////////////////////////////////////////////
enum PzsState{
    Ps_NoOpen      =  -1,   //未打开    //新的简化版凭证集状态
    Ps_Rec         =  1,    //初始状态（凭证集内只要有一张凭证处于录入态）
    Ps_AllVerified = 60,    //入账态（除作废凭证外所有凭证都已审核）
    Ps_Jzed        = 100    //已结账，不能作改动
};

/////////////////////////////////////////////////////////////////////////


#define MAXMT 10   //最大的货币种类数目（必须是10、100、1000...）



//报表类别代码
#define RPT_BALANCE 1     /**资产负债表*/
#define RPT_PROFIT  2     //利润表
#define RPT_CASH    3     //现金流量表
#define RPT_OWNER   4     //所有者权益变动表

//报表类型代码
//#define RPT_OLD     1     //老式
//#define RPT_NEW     2     //新式

//科目系统代码
#define SUBSYS_OLE  1     //老式
#define SUBSYS_NEW  2     //新式

//凭证打印模板
//#define LEFTMARGIN 10;
//#define RIGHTMARGIN 10;
//#define TOPMARGIN 10;
//#define BUTTOMMARGIN 10;

#define MAXROWS 8   //在凭证打印模板中最多可以有的业务活动行数(不包括标题和合计行)
#define MIDGAP  120  //两张凭证之间的间隔距离(一张A4纸打印2张凭证)
#define TITLEHEIGHT 15 //业务活动列表标题条高度

//////////////////////凭证大类代码/////////////////////////////////////////
enum PzdClass{
    Pzd_Imp       =   1,       //引入类凭证
    Pzd_Jzhd      =   2,       //结转汇兑损益类凭证
    Pzd_Jzsy      =   3,       //结转损益类凭证
    Pzd_Jzlr      =   4        //结转本年利润类凭证
};

//////////////////////////////////////////////////////////////////
//科目级别
enum SujectLevel{
    FstSubject   = 1,       //一级科目
    SndSubject   = 2        //二级科目
};

enum PrintTask{
    TOPRINT   = 1,   //输出到打印机
    PREVIEW = 2,   //打印预览
    TOPDF   = 3    //输出到pdf文件
};

//描述页面边距的结构
struct PageMargin{
    QPrinter::Unit unit;
    qreal left,right,top,bottom;
};

//////////////////////////////////////////////////////////////////

//借贷方向常量
#define DIR_J  1    //借
#define DIR_D  -1   //贷
#define DIR_P  0    //平

//
#define MAXPZNUMS   1000    //在一个凭证集内的最大凭证数（目前为转到某号凭证界定一个上限）

///////////////////////////////////////////////////////////////////

#endif // COMMON_H
