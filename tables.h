#ifndef TABLES_H
#define TABLES_H

#include <QString>

//账户信息表(AccountInfo) 该表的每行代表一个账户的信息
//CREATE TABLE AccountInfo(id INTEGER PRIMARY KEY, code INTEGER, name TEXT, value TEXT)
const QString tbl_accInfo     = "AccountInfo";
const QString fld_acci_code   = "code";     //账户信息字段的枚举编号（code INTEGER）
const QString fld_acci_name   = "name";     //账户信息字段名（name TEXT）
const QString fld_acci_value  = "value";    //账户信息字段值（value TEXT）
const int ACCINFO_CODE  = 1;
const int ACCINFO_NAME  = 2;
const int ACCINFO_VALUE = 3;

//帐套表accountSuites，从accountInfo表内读取有关帐套的数据进行初始化
//字段名
const QString tbl_accSuites      = "accountSuites";
const QString fld_accs_year      = "year";         //帐套所属年份（integer）
const QString fld_accs_subSys    = "subSys";       //帐套所用的科目系统代码（integer）
const QString fld_accs_isCur     = "isCurrent";    //是否是当前帐套（integer）
const QString fld_accs_recentMonth = "recentMonth";//最近打开月份
const QString fld_accs_name      = "name";         //帐套名（text）
//字段索引
const int ACCS_YEAR         = 1;
const int ACCS_SUBSYS       = 2;
const int ACCS_ISCUR        = 3;
const int ACCS_RECENTMONTH  = 4;
const int ACCS_NAME         = 5;



//*************************币种表*************************//
//CREATE TABLE MoneyTypes(id INTEGER PRIMARY KEY, code INTEGER, sign TEXT, name TEXT)
//字段名
const QString tbl_moneyType = "MoneyTypes";
const QString fld_mt_isMaster   = "isMaster";   //是否是母币
const QString fld_mt_code       = "code";       //币种代码
const QString fld_mt_sign       = "sign";       //币种符号
const QString fld_mt_name       = "name";       //币种名称
//字段索引
const int MT_MASTER = 1;
const int MT_CODE   = 2;
const int MT_SIGN   = 3;
const int MT_NAME   = 4;


//*************************汇率表***********************************//
//CREATE TABLE ExchangeRates(id INTEGER PRIMARY KEY, year INTEGER, month INTEGER, usd2rmb REAL)
//字段名（注意：汇率字段的命名规则：币种符号 + “2” + 本币符号（rmb））
const QString tbl_rateTable  = "ExchangeRates";
const QString fld_rt_year    = "year";
const QString fld_rt_month   = "month";
//字段索引
const int RT_YEAR   = 1;
const int RT_MONTH  = 2;

//银行表(Banks)
//字段名
const QString tbl_bank           = "Banks";
const QString fld_bank_isMain    = "IsMain";    //是否基本户(bool)
const QString fld_bank_name      = "name";      //开户行简称(TEXT)
const QString fld_bank_lname     = "lname";     //全称(TEXT)
//字段索引
const int BANK_ISMAIN = 1;
const int BANK_NAME   = 2;
const int BANK_LNAME  = 3;

//银行账户表--在银行下开设的与币种相关的银行账户
//字段名
const QString tbl_bankAcc         = "BankAccounts";
const QString fld_bankAcc_bankId  = "bankID";    //账户所属的银行ID（INTEGER）
const QString fld_bankAcc_mt      = "mtID";      //账户币种ID INTEGER）
const QString fld_bankAcc_accNum  = "accNum";    //帐号（TEXT）
const QString fld_bankAcc_nameId   = "nameId";     //该账户对应的名称条目id
//字段索引
const int BA_BANKID = 1;
const int BA_MT     = 2;
const int BA_ACCNUM = 3;
const int BA_ACCNAME= 4;


//*************************一级科目类别表*************************//
//create table FstSubClasses(id integer primary key, subSys integer, code integer, name text)
//字段名
const QString tbl_fsclass    = "FstSubClasses";
const QString fld_fsc_subSys = "subSys";
const QString fld_fsc_code   = "code";      //类别代码(INTEGER)
const QString fld_fsc_name   = "name";      //类别名称(TEXT)
//字段索引
const int FSCLS_SUBSYS   = 1;
const int FSCLS_CODE     = 2;
const int FSCLS_NAME     = 3;

//*************************一级科目表*********************************//
//CREATE TABLE FirSubjects(id INTEGER PRIMARY KEY, subSys INTEGER, subCode varchar(4),
//remCode varchar(10), belongTo integer, jdDir integer, isView integer,
//isUseWb INTEGER, weight integer, subName varchar(10))
//字段名
const QString tbl_fsub         = "FirSubjects";
const QString fld_fsub_subSys  = "subSys";       //科目系统代码（INTEGER）
const QString fld_fsub_subcode = "subCode";      //一级科目代码（国标）(varchar(4))
const QString fld_fsub_remcode = "remCode";      //科目助记符(varchar(10))
const QString fld_fsub_class   = "clsId";        //所属类别（integer)
const QString fld_fsub_jddir   = "jdDir";        //科目的借贷方向判定方法（integer）
                                        //（1：增加在借方，减少在贷方；0：增加在贷方，减少在借方）
const QString fld_fsub_isview  = "isView";       //是否启用该科目(integer)(1：启用，0：不启用)
const QString fld_fsub_isUseWb = "isUseWb";      //是否需要使用外币
const QString fld_fsub_weight  = "weight";       //科目使用的权重值(integer)
const QString fld_fsub_name    = "subName";      //科目名(varchar(10))
//字段索引
const int FSUB_SUBSYS     =  1;
const int FSUB_SUBCODE    =  2;
const int FSUB_REMCODE    =  3;
const int FSUB_CLASS      =  4;
const int FSUB_DIR        =  5;
const int FSUB_ISVIEW     =  6;
const int FSUB_ISUSEWB    =  7;
const int FSUB_WEIGHT     =  8;
const int FSUB_SUBNAME    =  9;

//*************************名称条目类别表*************************//
//二级科目类别表
//字段名
const QString tbl_nameItemCls = "NameItemClass";
const QString fld_nic_clscode = "clsCode";      //类别代码（INTEGER）
const QString fld_nic_name    = "name";         //名称（TEXT）
const QString fld_nic_explain = "explain";      //简要说明（TEXT）
//字段索引
const int NICLASS_CODE    = 1;
const int NICLASS_NAME    = 2;
const int NICLASS_EXPLAIN = 3;

//*************************名称条目表*************************//
//名称条目表
//字段名
const QString tbl_nameItem    = "nameItems";
const QString fld_ni_name     = "sName";         //简称（TEXT）
const QString fld_ni_lname    = "lName";         //全称（TEXT）
const QString fld_ni_remcode  = "remCode";       //助记符（TEXT）
const QString fld_ni_class    = "classId";       //名称类别代码（INTEGER）
const QString fld_ni_crtTime  = "createdTime";   //创建时间（TEXT）（TimeStamp NOT NULL DEFAULT (datetime('now','localtime'))）
const QString fld_ni_creator  = "creator";       //创建者（INTEGER）
//字段索引
const int NI_NAME        = 1;
const int NI_LNAME       = 2;
const int NI_REMCODE     = 3;
const int NI_CALSS       = 4;
const int NI_CREATERTIME = 5;
const int NI_CREATOR     = 6;

//*************************二级科目表*************************//
//字段名
const QString tbl_ssub        = "SndSubject";
const QString fld_ssub_fid     = "fid";           //所属的一级科目ID（INTEGER）
const QString fld_ssub_nid     = "nid";           //对应的名称条目中的ID（INTEGER）
const QString fld_ssub_code    = "subCode";       //科目代码（用户根据行业特点自定义的）（TEXT）
const QString fld_ssub_weight  = "weight";        //科目的使用权重（INTEGER）
const QString fld_ssub_enable  = "isEnabled";     //是否在账户中启用（INTEGER）
const QString fld_ssub_disTime = "disabledTime";  //禁用时间（TEXT）
const QString fld_ssub_crtTime = "createdTime";   //创建时间（TEXT）（NOT NULL DEFAULT (datetime('now','localtime'))）
const QString fld_ssub_creator = "creator";       //创建者（INTEGER）
//字段索引
const int SSUB_FID         = 1;
const int SSUB_NID         = 2;
const int SSUB_SUBCODE     = 3;
const int SSUB_WEIGHT      = 4;
const int SSUB_ENABLED     = 5;
const int SSUB_DISABLETIME = 6;
const int SSUB_CREATETIME  = 7;
const int SSUB_CREATOR     = 8;

//******************凭证表*********************************//
//字段名
const QString tbl_pz        = "PingZhengs";
const QString fld_pz_date   = "date";       //凭证日期（TEXT）
const QString fld_pz_number = "number";     //凭证总号（INTEGER）
const QString fld_pz_zbnum  = "zbNum";      //凭证自编号（INTEGER）
const QString fld_pz_jsum   = "jsum";       //借方合计（REAL）
const QString fld_pz_dsum   = "dsum";       //贷方合计（REAL）
const QString fld_pz_class  = "isForward";  //凭证类别(INTEGER)
const QString fld_pz_encnum = "encNum";     //凭证附件张数（INTEGER）
const QString fld_pz_state  = "pzState";    //凭证状态（INTEGER） -1：作废态，1：初始录入态，2：已审核态，3：记账态，
const QString fld_pz_vu     = "vuid";       //审核凭证的用户的ID（INTEGER）
const QString fld_pz_ru     = "ruid";       //录入凭证的用户的ID（INTEGER）
const QString fld_pz_bu     = "buid";       //凭证入账的用户ID（INTEGER）
//字段索引
const int PZ_DATE    = 1;
const int PZ_NUMBER  = 2;
const int PZ_ZBNUM   = 3;
const int PZ_JSUM    = 4;
const int PZ_DSUM    = 5;
const int PZ_CLS     = 6;
const int PZ_ENCNUM  = 7;
const int PZ_PZSTATE = 8;
const int PZ_VUSER   = 9;
const int PZ_RUSER   = 10;
const int PZ_BUSER   = 11;

//会计分录表
//字段名
const QString tbl_ba         = "BusiActions";
const QString fld_ba_pid     = "pid";           //所属的凭证ID（INTEGER）
const QString fld_ba_summary = "summary";       //业务活动摘要（TEXT）
const QString fld_ba_fid     = "firSubID";      //一级科目（ INTEGER）
const QString fld_ba_sid     = "secSubID";      //二级科目（INTEGER）
const QString fld_ba_mt      = "moneyType";     //货币类型（INTEGER）
const QString fld_ba_jv      = "jMoney";        //借方金额（REAL）
const QString fld_ba_dv      = "dMoney";        //贷方金额（REAL）
const QString fld_ba_dir     = "dir";           //借贷方向（1：借，0：贷）（INTEGER）
const QString fld_ba_number  = "NumInPz";       //该业务活动在凭证业务活动表中的序号（INTEGER）
                                                //（序号决定了在表中的具体位置，基于1）
//字段索引
const int BACTION_PID     = 1;
const int BACTION_SUMMARY = 2;
const int BACTION_FID     = 3;
const int BACTION_SID     = 4;
const int BACTION_MTYPE   = 5;
const int BACTION_JMONEY  = 6;
const int BACTION_DMONEY  = 7;
const int BACTION_DIR     = 8;
const int BACTION_NUMINPZ = 9;


//**************************老主目余额表（以原币计）*****************************/
//（各科目的字段名由代表科目类别的字母代码加上科目国标代码组成）
//A（资产类）B（负债类）C（共同类）D（所有者权益类）E（成本类）F（损益类）
//CREATE TABLE SubjectExtras(id INTEGER PRIMARY KEY, year INTEGER, month INTEGER,
//state INTEGER, mt INTEGER, A1001 REAL...)
const QString tbl_se        = "SubjectExtras";
const QString fld_se_year   = "year";
const QString fld_se_month  = "month";
const QString fld_se_state  = "state";     //余额是否有效（即最近一次保存此余额后凭证集是否做了影响统计余额的操作）
const QString fld_se_mt     = "mt" ;       //币种代码

const int SE_YEAR     = 1;  //(year INTEGER)
const int SE_MONTH    = 2;  //(month INTEGER) （如果month=12，则表示是年度余额）
const int SE_STATE    = 3;  //（state INTEGER）余额结转状态
const int SE_MT       = 4;  //（mt  INTEGER） 币种代码
const int SE_SUBSTART = 5;  //第一个科目所对应的字段索引号
// ......

//***********************主科目外币（转换为本币）余额表************************
/* CREATE TABLE SubjectMmtExtras(id INTEGER PRIMARY KEY, year INTEGER,
   month INTEGER,mt INTEGER,A1002 REAL,A1131 REAL,B2121 REAL)            */
//字段名
const QString tbl_sem       = "SubjectMmtExtras";
const QString fld_sem_year  = "year" ;  //
const QString fld_sem_month = "month";  //
const QString fld_sem_mt    = "mt" ;    //币种代码
const QString fld_sem_bank  = "A1002";  //银行
const QString fld_sem_ys    = "A1131";  //应收
const QString fld_sem_yf    = "B2121";  //应付
const QString fld_sem_yuf   = "A1151";  //预付
const QString fld_sem_yus   = "B2131";  //预收

//字段索引
const int SEM_YEAR  = 1;
const int SEM_MONTH = 2;
const int SEM_MT    = 3;
const int SEM_BANK  = 4;
const int SEM_YS    = 5;
const int SEM_YF    = 6;
const int SEM_YUF   = 7;
const int SEM_YUS   = 8;

//***********************子科目外币（转换为本币）余额表************************
/*  CREATE TABLE detailMmtExtras(id integer primary key,seid integer,
    fsid integer,value REAL)                              */
//字段名
const QString tbl_sdem       = "detailMmtExtras";
const QString fld_sdem_seid  = "seid";  //连接到SubjectMmtExtras表的id
const QString fld_sdem_fsid  = "fsid";  //连接到FSAgent表的id
const QString fld_sdem_value = "value";
//字段索引
const int SDEM_SEID  = 1;
const int SDEM_FSID  = 2;
const int SDEM_VALUE = 3;

//账户信息表(AccountInfo) 该表的每行代表一个账户的信息
//CREATE TABLE AccountInfo(id INTEGER PRIMARY KEY, code INTEGER, name TEXT, value TEXT)
//字段名
//const QString tbl_account   = "AccountInfo";
//const QString fld_acc_code  = "code";   //账户信息字段的枚举编号（INTEGER）
//const QString fld_acc_name  = "name";   //账户信息字段名（TEXT）
//const QString fld_acc_value = "value";  //账户信息字段值（TEXT）
////字段索引
//const int ACCOUNT_CODE    = 1;
//const int ACCOUNT_NAME    = 2;
//const int ACCOUNT_VALUE   = 3;


//凭证集状态表
//CREATE TABLE PZSetStates(id INTEGER PRIMARY KEY, year INTEGER, month INTEGER, state INTEGER)
//字段名
const QString tbl_pzsStates  = "PZSetStates";
const QString fld_pzss_year  = "year";
const QString fld_pzss_month = "month";
const QString fld_pzss_state = "state";
//字段索引
const int PZSS_YEAR  = 1;
const int PZSS_MONTH = 2;
const int PZSS_STATE = 3;

////////////////////////////新余额表系列//////////////////////////////////////////////
//余额指针表
//字段名
const QString tbl_nse_point   = "SE_Point";
const QString fld_nse_year    = "year";
const QString fld_nse_month   = "month";
const QString fld_nse_mt      = "mt";
const QString fld_nse_state   = "state";
//字段索引
const int NSE_POINT_YEAR  = 1;
const int NSE_POINT_MONTH = 2;
const int NSE_POINT_MT    = 3;
const int NSE_POINT_STATE = 4;

//新余额表
const QString tbl_nse_p_f = "SE_PM_F";   //一级科目原币余额
const QString tbl_nse_m_f = "SE_MM_F";   //一级科目本币余额
const QString tbl_nse_p_s = "SE_PM_S";   //二级科目原币余额
const QString tbl_nse_m_s = "SE_MM_S";   //二级科目本币余额
//字段名
const QString fld_nse_pid =    "pid";   //余额指针
const QString fld_nse_sid =    "sid";   //科目id
const QString fld_nse_value =  "value"; //余额值
const QString fld_nse_dir =    "dir" ;  //余额方向（保存本币余额的表不包含本字段）
//字段索引
const int NSE_E_PID   = 1;
const int NSE_E_SID   = 2;
const int NSE_E_VALUE = 3;
const int NSE_E_DIR   = 4;






//转移表（transfers）
//create table transfers(id integer primary key, smid integer, dmid integer,
//state integer, outTime text, inTime text)
//字段名
const QString tbl_transfer      = "transfers";
const QString fld_trans_smid    = "smid";
const QString fld_trans_dmid    = "dmid";
const QString fld_trans_state   = "state";
const QString fld_trans_outTime = "outTime";
const QString fld_trans_inTime  = "inTime";
//字段索引
const int TRANS_SMID     = 1;
const int TRANS_DMID     = 2;
const int TRANS_STATE    = 3;
const int TRANS_OUTTIME  = 4;
const int TRANS_INTIME   = 5;

//转移动作情况描述表（transferDescs）
//create table transferDescs(id integer primary key, tid integer, outDesc text, inDesc text)
//字段名
const QString tbl_transferDesc  = "transferDescs";
const QString fld_transDesc_tid = "tid";
const QString fld_transDesc_out = "outDesc";
const QString fld_transDesc_in  = "inDesc";
//字段索引
const int TRANSDESC_TID  = 1;
const int TRANSDESC_OUT  = 2;
const int TRANSDESC_IN   = 3;

//////////*****************基本库数据表********************************//////////
//凭证状态名表
//CREATE TABLE pzStateNames(id INTEGER PRIMARY KEY, code INTEGER, state TEXT)
//字段名
const QString tbl_pzStateName = "pzStateNames";
const QString fld_pzsn_code   = "code";
const QString fld_pzsn_name   = "state";
//字段索引
const int PZSN_CODE = 1;
const int PZSN_NAME = 2;


//凭证集状态名表
//CREATE TABLE "pzsStateNames"(id INTEGER PRIMARY KEY, code INTEGER, state TEXT, desc TEXT)
//字段名
const QString tbl_pzsStateNames = "pzsStateNames";
const QString fld_pzssn_code    = "code";
const QString fld_pzssn_sname   = "state";
const QString fld_pzssn_lname   = "desc";
//字段索引
const int PZSSN_CODE  = 1;
const int PZSSN_SNAME = 2;
const int PZSSN_LNAME = 3;

//主机表（machines）
//CREATE TABLE machines(id integer primary key, type integer, mid integer,
//isLocal integer, sname text, lname text)
//字段名
const QString tbl_machines      = "machines";
const QString fld_mac_mid       = "mid";
const QString fld_mac_type      = "type";
const QString fld_mac_islocal   = "isLocal";
const QString fld_mac_sname     = "sname";
const QString fld_mac_desc      = "lname";
//字段索引
const int MACS_TYPE       = 1;
const int MACS_MID        = 2;
const int MACS_ISLOCAL    = 3;
const int MACS_NAME       = 4;
const int MACS_DESC       = 5;


#endif // TABLES_H
