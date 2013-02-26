#ifndef TABLES_H
#define TABLES_H

#include <QString>

//class TableManager{
//public:

//};

//*************************一级科目类别表*************************//
#define tbl_fsclass "FstSubClasses"
//字段名
#define fld_fsc_code "code"
#define fld_fsc_name "name"
//字段索引
#define FSCLS_CODE 1 //类别代码(code INTEGER)
#define FSCLS_NAME 2 //类别名称(name TEXT)

//*************************一级科目表*********************************//
#define tbl_fsub "FirSubjects"
//字段名
#define fld_fsub_succode "subCode"
#define fld_fsub_remcode "remCode"
#define fld_fsub_class   "belongTo"
#define fld_fsub_jddir   "jdDir"
#define fld_fsub_isview  "isView"
#define fld_fsub_isreqdet "isReqDet"
#define fld_fsub_weight  "weight"
#define fld_fsub_name    "subName"
#define fld_fsub_desc    "description"
#define fld_fsub_util    "utils"
//字段索引
#define FSTSUB_ID 0            //(id INTEGER PRIMARY KEY)
#define FSTSUB_SUBCODE 1       //一级科目代码（国标）(subCode varchar(4))
#define FSTSUB_REMCODE 2       //科目助记符(remCode varchar(10))
#define FSTSUB_BELONGTO 3      //所属类别（目前是6大类别）(belongTo integer)
#define FSTSUB_DIR      4      //科目的借贷方向判定方法（jdDir integer）（1：增加在借方，减少在贷方；0：增加在贷方，减少在借方）
#define FSTSUB_ISVIEW   5      //是否启用该科目(isView integer)(1：启用，0：不启用)
#define FSTSUB_ISREQDET 6      //是否需要在余额表中显示该科目的余额或是否需要明细统计支持(isReqDet INTEGER)
#define FSTSUB_WEIGHT    7     //科目使用的权重值(weight integer)
#define FSTSUB_SUBNAME 8       //科目名(subName varchar(10))
#define FSTSUB_DESC 9          //对科目的描述(description TEXT)
#define FSTSUB_UTILS 10        //科目的使用范围和例子用法等(utils TEXT)

//*************************二级科目类别表*************************//
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

//*************************二级科目名称表*************************//
//二级科目信息表
//同一个二级科目名称可能会属于多个一级科目，比如在应收账款和应付账款科目下可能按客户名分列二级科目
//为了是客户信息不至于在二级科目表中造成很多重复，我只在此表中保存科目名称等不变的信息，而科目代码
//等特定于二级科目的信息保存在FSAGENT表中
#define tbl_ssub "SecSubjects"
//字段名
#define fld_ssub_name "subName"
#define fld_ssub_lname "subLName"
#define fld_ssub_remcode "remCode"
#define fld_ssub_class "classId"
//字段索引
#define SNDSUB_ID 0            //(id INTEGER PRIMARY KEY)
#define SNDSUB_SUBNAME 1       //二级科目名(subName VERCHAR(10))
#define SNDSUB_SUBLONGNAME 2   //二级科目详名(subLName TEXT)
#define SNDSUB_REMCODE 3       //科目助记符(remCode varchar(10))
#define SNDSUB_CALSS 4         //科目所属类别(classId INTEGER)

//*************************二级科目映射表*************************//
//一级科目到二级科目的代理映射
#define tbl_fsa  "FSAgent"
//字段名
#define fld_fsa_fid "fid"
#define fld_fsa_sid "sid"
#define fld_fsa_code "subCode"
#define fld_fsa_fs "FrequencyStat"
#define fld_fsa_isdetbymt "isDetByMt"
#define fld_fsa_enable "isEnabled"
//字段索引
#define FSAGENT_ID 0        //（id INTEGER PRIMARY KEY）
#define FSAGENT_FID 1       //所属的一级科目ID（fid INTEGER）
#define FSAGENT_SID 2       //对应的二级科目信息表中的ID（sid INTEGER）
#define FSAGENT_SUBCODE 3   //科目代码（用户根据行业特点自定义的）(subCode VARCHAR(10))
#define FSAGENT_FS 4        //科目的使用频度统计值(FrequencyStat INTEGER)
#define FSAGENT_ISDETBYMT 5 //是否需要按币种建立明细账（isDetByMt INTEGER）
#define FSAGENT_ENABLED   6 //是否在账户中启用(isEnabled INTEGER)

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
//#define BACTION_PID 1         //所属的凭证ID（pid INTEGER）
//#define BACTION_SUMMARY 2     //业务活动摘要（summary TEXT）
//#define BACTION_FID 3         //一级科目（firSubID INTEGER）
//#define BACTION_SID 4         //二级科目（secSubID INTEGER）
//#define BACTION_MTYPE 5       //货币类型（moneyType INTEGER）
//#define BACTION_JMONEY 6      //借方金额（jMoney REAL）
//#define BACTION_DMONEY 7      //贷方金额（dMoney REAL）
//#define BACTION_DIR   8       //借贷方向（1：借，0：贷）（dir INTEGER）
//#define BACTION_NUMINPZ   9   //该业务活动在凭证业务活动表中的序号（NumInPz INTEGER）
//                              //（序号决定了在表中的具体位置，基于1）

//***********************主科目外币（转换为本币）余额表************************
/* CREATE TABLE SubjectMmtExtras(id INTEGER PRIMARY KEY, year INTEGER,
   month INTEGER,mt INTEGER,A1002 REAL,A1131 REAL,B2121 REAL)            */
//字段名
#define tbl_sem  "SubjectMmtExtras"
#define fld_sem_year  "year"
#define fld_sem_month "month"
#define fld_sem_mt  "mt"
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
