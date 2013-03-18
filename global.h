#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>

#include "config.h"
#include "common.h"
#include "securitys.h"
#include "commdatastruct.h"
#include "account.h"
#include "otherModule.h"
#include "logs/Logger.h"

extern QString orgName;        //创建应用程序的组织名
extern QString appName;        //应用程序名
extern QString appTitle;       //应用程序主窗口标题
extern QString versionStr;     //版本号字符串
extern QString aboutStr;       //版权声明字符串
extern Logger::LogLevel logLevel; //应用程序的日志级别

extern int screenWidth;        //屏幕宽度
extern int screenHeight;       //屏幕高度

extern int curAccountId;         //当前打开账户的id
//extern AccountBriefInfo* curAccInfo;       //当前账户信息
//extern int usedRptType;  //默认使用老式报表形式（新式为2）
//extern int curUsedSubSys;      //所使用的科目系统
extern Account* curAccount;

extern User* curUser;           //当前用户

extern QSqlDatabase adb;        //账户数据库连接
extern QSqlDatabase bdb;        //基本数据库连接
extern QString hVersion;        //应用程序当前支持的最高账户文件版本号

//应用程序的路径变量
extern QString DatabasePath;    //工作数据库路径
extern QString BaseDataPath;    //基础数据库路径


//extern QHash<int,QString> allFstSubs;  //所有一级科目id到科目名称的映射表
//extern QHash<int,QString> allFSCodes;  //所有一级科目id到科目代码的映射表

extern QHash<int,QString> allSndSubs;  //所有二级科目id到科目名称的映射表
extern QHash<int,QString> allSndSubLNames; //所有二级科目id到科目全称的映射表
extern QHash<int,QString> MTS;         //所有币种代码到名称的映射表（包括账户内未使用的外币）
extern QHash<int,QString> allMts;      //当前打开账户所用的所有币种代码到名称的映射表
extern QHash<int,int>  defaultSndSubs; //所有一级科目下的默认二级科目

//固定资产相关变量
extern QHash<int,GdzcType*> allGdzcProductCls; //所有固定资产产品类别表，键为固定资产类别的代码
extern QHash<int,QString> allGdzcSubjectCls; //所有固定资产产品科目类别名表，键为明细科目代码

//凭证类别和凭证状态
extern QSet<int> pzClsImps;         //自动引入的凭证类别代码集合
extern QSet<int> pzClsJzhds;        //结转汇兑损益的凭证类别代码集合
extern QSet<int> pzClsJzsys;        //结转损益的凭证类别代码集合
extern QHash<PzState,QString> pzStates; //凭证状态名表
extern QHash<PzClass,QString> pzClasses;//凭证类别名表

//凭证集状态
extern QHash<PzsState,QString> pzsStates; //凭证集状态名表
extern QHash<PzsState,QString> pzsStateDescs; //凭证集状态解释表

//需要作特别处理的科目ID
extern int subCashId;   //现金科目
extern int subBankId;   //银行存款
extern int subYsId;     //应收账款
extern int subYfId;     //应付账款
extern int subCashRmbId;//现金科目下的人民币子目


//应用的行为控制配置变量
extern bool isByMt;  //在按下等号键建立新的合计对冲业务活动时，是否按币种分开建立
extern bool isCollapseJz; //是否展开结转凭证中的业务活动表项的明细，默认展开。
extern int  autoSaveInterval; //自动保存到时间间隔
extern bool jzlrByYear;       //是否每年年底执行一次本年利润的结转
extern int  timeoutOfTemInfo; //在状态条上显示临时信息的超时时间（以毫秒计）
extern bool viewHideColInDailyAcc1; //是否在日记账表格中显示隐藏列（包括对方科目、结算号等）
extern bool viewHideColInDailyAcc2; //是否在日记账表格中显示隐藏列（包括凭证id、会计分录id等）
extern double czRate;               //固定资产折旧残值率

//应用程序剪贴板功能有关的全局标量
extern bool copyOrCut;                       //剪贴板存放到业务活动是复制还是剪切到（true：复制）
extern QList<BusiActionData2*> clbBaList;     //存放要粘贴的业务活动数据

//账户状态变量
extern QString lastModifyTime;   //账户最后修改时间

//extern VersionManager* accVM;    //管理账户数据库版本
//extern VersionManager* confVM;   //管理配置模块版本
//extern VersionManager* baseVM;   //管理基本库版本

//全局函数
extern int appInit();          //应用初始化
extern void initVMs();          //初始化版本维护对象
extern void appExit();          //应用退出前的执行例程
extern QString dirStr(int dir); //输出借贷方向的文字表达
extern QString dirVStr(double v); //根据值的符号输出借贷方向的文字表达
extern QString removeRightZero(QString str); //移除两位精度的实数的最后一位或二位0
//static QMap<int, QString> mapFstSubClass = appSetting.readFstSubClass();  //保存一级科目的类别（从整数到类别名称的映射）


#endif // GLOBAL_H
