#ifndef GLOBALVARNAMES_H
#define GLOBALVARNAMES_H

#include <QString>

/**
 * @brief 全局变量名称
 */

static QString GVN_RECENT_LOGIN_USER = "recentLoginUser";   //最近登录用户id
static QString GVN_IS_COLLAPSE_JZ = "isCollapseJz";         //是否收缩结转凭证的分录
static QString GVN_IS_BY_MT_FOR_OPPO_BA = "isByMtForOppoBa";//
static QString GVN_AUTOSAVE_INTERVAL = "autoSaveInterval";  //自动保存凭证的时间间隔
static QString GVN_JZLR_BY_YEAR = "jzlrByYear";             //是否按年结转本年利润
static QString GVN_VIEWORHIDE_COL_IN_DAILY_1 = "viewHideColInDailyAcc1";//在明细账视图中是否隐藏结算号和对方科目列
static QString GVN_VIEWORHIDE_COL_IN_DAILY_2 = "viewHideColInDailyAcc2";//在明细账视图中是否隐藏凭证id和科目id列
static QString GVN_IS_RUNTIMNE_UPDATE_EXTRA = "rt_update_extra";//是否实时更新余额
static QString GVN_GDZC_CZ_RATE = "canZhiRate";             //固定资产折旧残值率

#endif // GLOBALVARNAMES_H
