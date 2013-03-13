#ifndef TABLES_H
#define TABLES_H

#include <QString>

//账户信息表(AccountInfo) 该表的每行代表一个账户的信息
//CREATE TABLE AccountInfo(id INTEGER PRIMARY KEY, code INTEGER, name TEXT, value TEXT)
#define tbl_accInfo     "AccountInfo"
#define fld_acci_code   "code"          //账户信息字段的枚举编号（code INTEGER）
#define fld_acci_name   "name"          //账户信息字段名（name TEXT）
#define fld_acci_value  "value"         //账户信息字段值（value TEXT）
#define ACCINFO_CODE     1
#define ACCINFO_NAME     2
#define ACCINFO_VALUE    3


//*************************币种表*************************//
//CREATE TABLE MoneyTypes(id INTEGER PRIMARY KEY, code INTEGER, sign TEXT, name TEXT)
//字段名
#define tbl_moneyType " MoneyTypes"
#define fld_mt_code "code"  //币种代码
#define fld_mt_sign "sign"  //币种符号
#define fld_mt_name "name"  //币种名称
//字段索引
#define MT_CODE 1
#define MT_SIGN 2
#define MT_NAME 3


//*************************汇率表***********************************//
//CREATE TABLE ExchangeRates(id INTEGER PRIMARY KEY, year INTEGER, month INTEGER, usd2rmb REAL)
//字段名
//注意：汇率字段的命名规则：币种符号 + “2” + 本币符号（rmb）
#define tbl_rateTable   "ExchangeRates"
#define fld_rt_year     "year"
#define fld_rt_month    "month"
//字段索引
#define RT_YEAR     1
#define RT_MONTH    2

//银行表(Banks)
//字段名
#define tbl_bank            "Banks"
#define fld_bank_isMain     "IsMain"    //是否基本户(bool)
#define fld_bank_name       "name"      //开户行简称(TEXT)
#define fld_bank_lname      "lname"     //全称(TEXT)
//字段索引
#define BANK_ISMAIN 1
#define BANK_NAME   2
#define BANK_LNAME  3

//银行账户表--在银行下开设的与币种相关的银行账户
//字段名
#define tbl_bankAcc     "BankAccounts"
#define fld_bankAcc_bankId  "bankID"    //账户所属的银行ID（INTEGER）
#define fld_bankAcc_mt      "mtID"      //账户币种ID INTEGER）
#define fld_bankAcc_accNum  "accNum"    //帐号（TEXT）
//字段索引
#define BA_BANKID 1
#define BA_MT  2
#define BA_ACCNUM  3


//*************************一级科目类别表*************************//
//create table FstSubClasses(id integer primary key, subSys integer, code integer, name text)
//字段名
#define tbl_fsclass "FstSubClasses"
#define fld_fsc_subSys  "subSys"
#define fld_fsc_code    "code"      //类别代码(INTEGER)
#define fld_fsc_name    "name"      //类别名称(TEXT)
//字段索引
#define FSCLS_SUBSYS    1
#define FSCLS_CODE      2
#define FSCLS_NAME      3

//*************************一级科目表*********************************//
//CREATE TABLE FirSubjects(id INTEGER PRIMARY KEY, subSys INTEGER, subCode varchar(4),
//remCode varchar(10), belongTo integer, jdDir integer, isView integer,
//isUseWb INTEGER, weight integer, subName varchar(10))
//字段名
#define tbl_fsub "FirSubjects"
#define fld_fsub_subSys  "subSys"       //科目系统代码
#define fld_fsub_subcode "subCode"      //一级科目代码（国标）(subCode varchar(4))
#define fld_fsub_remcode "remCode"      //科目助记符(remCode varchar(10))
#define fld_fsub_class   "belongTo"     //所属类别（目前是6大类别）(belongTo integer)
#define fld_fsub_jddir   "jdDir"        //科目的借贷方向判定方法（jdDir integer）
                                        //（1：增加在借方，减少在贷方；0：增加在贷方，减少在借方）
#define fld_fsub_isview  "isView"       //是否启用该科目(isView integer)(1：启用，0：不启用)
#define fld_fsub_isUseWb "isUseWb"      //是否需要使用外币
#define fld_fsub_weight  "weight"       //科目使用的权重值(weight integer)
#define fld_fsub_name    "subName"      //科目名(subName varchar(10))
//字段索引
#define FSTSUB_SUBSYS       1
#define FSTSUB_SUBCODE      2
#define FSTSUB_REMCODE      3
#define FSTSUB_BELONGTO     4
#define FSTSUB_DIR          5
#define FSTSUB_ISVIEW       6
#define FSTSUB_ISUSEWB      7
#define FSTSUB_WEIGHT       8
#define FSTSUB_SUBNAME      9

//*************************名称条目类别表*************************//
//二级科目类别表
#define tbl_ssclass "SndSubClass"
//字段名
#define fld_ssc_clscode  "clsCode"
#define fld_ssc_name "name"
#define fld_ssc_explain "explain"
//字段索引
#define SNDSUBCLASS_ID 0
#define SNDSUBCLASS_CODE 1      //类别代码（clsCode INTEGER）
#define SNDSUBCLASS_NAME 2      //名称 (name TEXT )
#define SNDSUBCLASS_EXPLAIN 3   //简要说明 (explain TEXT)

//*************************名称条目表*************************//
//名称条目表
//CREATE TABLE nameItems(id INTEGER PRIMARY KEY, sName VERCHAR(10), lName TEXT,
//  remCode varchar(10), classId INTEGER, createdTime TimeStamp NOT NULL
//  DEFAULT (datetime('now','localtime')), creator integer)
//字段名
#define tbl_ssub "nameItems"
#define fld_ssub_name       "sName"         //简称
#define fld_ssub_lname      "lName"         //全称
#define fld_ssub_remcode    "remCode"       //助记符
#define fld_ssub_class      "classId"       //名称类别代码
#define fld_ssub_crtTime    "createdTime"   //创建时间
#define fld_ssub_creator    "creator"       //创建者
//字段索引
#define SNDSUB_SUBNAME      1
#define SNDSUB_SUBLONGNAME  2
#define SNDSUB_REMCODE      3
#define SNDSUB_CALSS        4
#define SNDSUB_CREATERTIME  5
#define SNDSUB_CREATOR      6

//*************************二级科目映射表*************************//
//一级科目到二级科目的代理映射
//CREATE TABLE FSAgent(id INTEGER PRIMARY KEY, fid INTEGER, sid INTEGER,
//subCode varchar(5), weight INTEGER, isEnabled INTEGER,disabledTime TimeStamp,
//createdTime NOT NULL DEFAULT (datetime('now','localtime')),creator integer)
//字段名
#define tbl_fsa  "FSAgent"
#define fld_fsa_fid     "fid"           //所属的一级科目ID
#define fld_fsa_sid     "sid"           //对应的名称条目中的ID
#define fld_fsa_code    "subCode"       //科目代码（用户根据行业特点自定义的）
#define fld_fsa_weight  "weight"        //科目的使用权重
#define fld_fsa_enable  "isEnabled"     //是否在账户中启用
#define fld_fsa_disTime "disabledTime"  //禁用时间
#define fld_fsa_crtTime "createdTime"   //创建时间
#define fld_fsa_creator "creator"       //创建者
//字段索引
#define FSA_FID         1
#define FSA_SID         2
#define FSA_SUBCODE     3
#define FSA_WEIGHT      4
#define FSA_ENABLED     5
#define FSA_DISABLETIME 6
#define FSA_CREATETIME  7
#define FSA_CREATOR     8

//******************凭证表*********************************//
//字段名
#define tbl_pz  "PingZhengs"
#define fld_pz_date  "date"
#define fld_pz_number  "number"
#define fld_pz_zbnum  "zbNum"
#define fld_pz_jsum  "jsum"
#define fld_pz_dsum  "dsum"
#define fld_pz_class  "isForward"
#define fld_pz_encnum  "encNum"
#define fld_pz_state  "pzState"
#define fld_pz_vu  "vuid"
#define fld_pz_ru  "ruid"
#define fld_pz_bu  "buid"
//字段索引
#define PZ_ID 0               //（id INTEGER PRIMARY KEY）
#define PZ_DATE 1             //凭证日期（date TEXT）
#define PZ_NUMBER 2           //凭证总号（number INTEGER）
#define PZ_ZBNUM 3            //凭证自编号（zbNum INTEGER）
#define PZ_JSUM 4             //借方合计（jsum REAL）
#define PZ_DSUM 5             //贷方合计（dsum REAL）
#define PZ_CLS  6             //凭证类别(isForward INTEGER)
#define PZ_ENCNUM  7          //凭证附件张数（encNum INTEGER）
#define PZ_PZSTATE 8          //凭证状态（pzState INTEGER）    -1：作废态，1：初始录入态，2：已审核态，3：记账态，
#define PZ_VUSER 9            //审核凭证的用户的ID（vuid INTEGER）
#define PZ_RUSER 10           //录入凭证的用户的ID（ruid INTEGER）
#define PZ_BUSER 11           //凭证入账的用户ID（buid INTEGER）

//会计分录表
#define tbl_ba "BusiActions"
//字段名
#define fld_ba_pid "pid"
#define fld_ba_summary "summary"
#define fld_ba_fid "firSubID"
#define fld_ba_sid "secSubID"
#define fld_ba_mt "moneyType"
#define fld_ba_jv "jMoney"
#define fld_ba_dv "dMoney"
#define fld_ba_dir "dir"
#define fld_ba_number "NumInPz"
//字段索引
#define BACTION_PID 1         //所属的凭证ID（pid INTEGER）
#define BACTION_SUMMARY 2     //业务活动摘要（summary TEXT）
#define BACTION_FID 3         //一级科目（firSubID INTEGER）
#define BACTION_SID 4         //二级科目（secSubID INTEGER）
#define BACTION_MTYPE 5       //货币类型（moneyType INTEGER）
#define BACTION_JMONEY 6      //借方金额（jMoney REAL）
#define BACTION_DMONEY 7      //贷方金额（dMoney REAL）
#define BACTION_DIR   8       //借贷方向（1：借，0：贷）（dir INTEGER）
#define BACTION_NUMINPZ   9   //该业务活动在凭证业务活动表中的序号（NumInPz INTEGER）
                              //（序号决定了在表中的具体位置，基于1）

//**************************老主目余额表（以原币计）*****************************/
//（各科目的字段名由代表科目类别的字母代码加上科目国标代码组成）
//A（资产类）B（负债类）C（共同类）D（所有者权益类）E（成本类）F（损益类）
//CREATE TABLE SubjectExtras(id INTEGER PRIMARY KEY, year INTEGER, month INTEGER,
//state INTEGER, mt INTEGER, A1001 REAL...)
#define tbl_se "SubjectExtras"
#define fld_se_year     "year"
#define fld_se_month    "month"
#define fld_se_state    "state"     //余额是否有效（即最近一次保存此余额后凭证集是否做了影响统计余额的操作）
#define fld_se_mt       "mt"        //币种代码

#define SE_YEAR 1      //(year INTEGER)
#define SE_MONTH 2     //(month INTEGER) （如果month=12，则表示是年度余额）
#define SE_STATE 3     //（state INTEGER）余额结转状态
#define SE_MT 4        //（mt  INTEGER） 币种代码
#define SE_SUBSTART 5  //第一个科目所对应的字段索引号
// ......

//***********************主科目外币（转换为本币）余额表************************
/* CREATE TABLE SubjectMmtExtras(id INTEGER PRIMARY KEY, year INTEGER,
   month INTEGER,mt INTEGER,A1002 REAL,A1131 REAL,B2121 REAL)            */
//字段名
#define tbl_sem  "SubjectMmtExtras"
#define fld_sem_year  "year"        //
#define fld_sem_month "month"       //
#define fld_sem_mt  "mt"            //币种代码
#define fld_sem_bank "A1002"
#define fld_sem_ys "A1131"
#define fld_sem_yf "B2121"
//字段索引
#define SEM_YEAR  1
#define SEM_MONTH 2
#define SEM_MT    3
#define SEM_BANK  4
#define SEM_YS    5
#define SEM_YF    6

//***********************子科目外币（转换为本币）余额表************************
/*  CREATE TABLE detailMmtExtras(id integer primary key,seid integer,
    fsid integer,value REAL)                              */
//字段名
#define tbl_sdem  "detailMmtExtras"
#define fld_sdem_seid  "seid"
#define fld_sdem_fsid  "fsid"
#define fld_sdem_value "value"
//字段索引
#define SDEM_SEID  1
#define SDEM_FSID  2
#define SDEM_VALUE 3

//账户信息表(AccountInfo) 该表的每行代表一个账户的信息
//CREATE TABLE AccountInfo(id INTEGER PRIMARY KEY, code INTEGER, name TEXT, value TEXT)
//字段名
#define tbl_account "AccountInfo"
#define fld_acc_code "code"
#define fld_acc_name "name"
#define fld_acc_value "value"
//字段索引
#define ACCOUNT_CODE     1      //账户信息字段的枚举编号（code INTEGER）
#define ACCOUNT_NAME     2      //账户信息字段名（name TEXT）
#define ACCOUNT_VALUE    3      //账户信息字段值（value TEXT）


//凭证集状态表
//CREATE TABLE PZSetStates(id INTEGER PRIMARY KEY, year INTEGER, month INTEGER, state INTEGER)
//字段名
#define tbl_pzsStates "PZSetStates"
#define fld_pzss_year "year"
#define fld_pzss_month "month"
#define fld_pzss_state "state"
//字段索引
#define PZSS_YEAR 1
#define PZSS_MONTH 2
#define PZSS_STATE 3

////////////////////////////新余额表系列//////////////////////////////////////////////
//余额指针表
//字段名
const QString tbl_nse_point   = "SE_Point";
const QString fld_sep_year    = "year";
const QString fld_sep_month   = "month";
const QString fld_sep_mt      = "mt";
//字段索引
const int NSE_POINT_YEAR  =1;
const int NSE_POINT_MONTH =2;
const int NSE_POINT_MT    =3;

//新余额表
const QString tbl_nse_p_f = "SE_PM_F";   //一级科目原币余额
const QString tbl_nse_m_f = "SE_MM_F";   //一级科目本币余额
const QString tbl_nse_p_s = "SE_PM_S";   //二级科目原币余额
const QString tbl_nse_m_s = "SE_MM_S";   //二级科目本币余额
//字段名
const QString fld_nse_pid =    "pid";   //余额指针
const QString fld_nse_sid =    "sid";   //科目id
const QString fld_nse_dir =    "dir" ;  //余额方向
const QString fld_nse_value =  "value"; //余额值
//字段索引
const int NSE_E_PID   =1;
const int NSE_E_SID   =2;
const int NSE_E_DIR   =3;
const int NSE_E_VALUE =4;







//转移表（transfers）
//create table transfers(id integer primary key, smid integer, dmid integer,
//state integer, outTime text, inTime text)
//字段名
#define tbl_transfer "transfers"
#define fld_trans_smid "smid"
#define fld_trans_dmid "dmid"
#define fld_trans_state "state"
#define fld_trans_outTime "outTime"
#define fld_trans_inTime "inTime"
//字段索引
#define TRANS_SMID      1
#define TRANS_DMID      2
#define TRANS_STATE     3
#define TRANS_OUTTIME   4
#define TRANS_INTIME    5

//转移动作情况描述表（transferDescs）
//create table transferDescs(id integer primary key, tid integer, outDesc text, inDesc text)
//字段名
#define tbl_transferDesc "transferDescs"
#define fld_transDesc_tid "tid"
#define fld_transDesc_out "outDesc"
#define fld_transDesc_in  "inDesc"
//字段索引
#define TRANSDESC_TID   1
#define TRANSDESC_OUT   2
#define TRANSDESC_IN    3

//////////*****************基本库数据表********************************//////////
//凭证状态名表
//CREATE TABLE pzStateNames(id INTEGER PRIMARY KEY, code INTEGER, state TEXT)
//字段名
#define tbl_pzStateName "pzStateNames"
#define fld_pzsn_code "code"
#define fld_pzsn_name "state"
//字段索引
#define PZSN_CODE 1
#define PZSN_NAME 2


//凭证集状态名表
//CREATE TABLE "pzsStateNames"(id INTEGER PRIMARY KEY, code INTEGER, state TEXT, desc TEXT)
//字段名
#define tbl_pzsStateNames "pzsStateNames"
#define fld_pzssn_code "code"
#define fld_pzssn_sname "state"
#define fld_pzssn_lname "desc"
//字段索引
#define PZSSN_CODE  1
#define PZSSN_SNAME 2
#define PZSSN_LNAME 3

//主机表（machines）
//CREATE TABLE machines(id integer primary key, type integer, mid integer,
//isLocal integer, sname text, lname text)
//字段名
#define tbl_machines "machines"
#define fld_mac_mid  "mid"
#define fld_mac_type "type"
#define fld_mac_islocal "isLocal"
#define fld_mac_sname "sname"
#define fld_mac_desc  "lname"
//字段索引
#define MACS_TYPE        1
#define MACS_MID         2
#define MACS_ISLOCAL     3
#define MACS_NAME        4
#define MACS_DESC        5


#endif // TABLES_H
