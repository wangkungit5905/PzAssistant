#ifndef DBUTIL_H
#define DBUTIL_H

#include <QSqlDatabase>


#include "account.h"



/**
 * @brief The DbUtil class
 *  提供对账户数据库的直接访问支持，所有其他的类要访问账户数据库，必须通过此类
 */

const QString AccConnName = "Account";
const int nmv = 1;  //这是新老余额存储机制转换的衔接版本号
const int nsv = 6;  //此版本号前使用老存储机制，此版本后（包括此版本）使用新存储机制

class SubjectManager;
class FirstSubject;
class PingZheng;
class NameItemAlias;
struct SmartSSubAdapteItem;
struct InvoiceRowStruct;
struct PzFindFilteCondition;
struct PzFindBaContent;
struct CurAuthCostInvoiceInfo;
struct HisAuthCostInvoiceInfo;
struct Journal;

class DbUtil
{
public:
    enum InfoField{
        ACODE     = 1,           //账户代码
        SNAME     = 2,           //账户简称
        LNAME     = 3,           //账户全称
        //RPTTYPE   = 6,           //报表类型
        MASTERMT  = 7,           //本币代码
        WAIMT     = 8,           //外币代码列表
        STIME     = 9,           //账户记账起始时间
        ENDTIME     = 10,          //账户记账终止时间（当前账户最后记账时间）
        CSUITE    = 11,          //账户当前帐套年份
        SUITENAME = 12,          //帐套名列表
        LASTACCESS= 13,          //账户最后访问时间
        ISJXTAXMGR = 14,        //是否需要进项进项税管理
        LOGFILE   = 50,          //与该账户相关的日志文件名
        DBVERSION = 51           //账户文件的版本号（用来表示数据库内表格的变动）
    };

    //数据库操作错误代码
    enum ErrorCode{
        Transaction_open    = 1,    //打开事务
        Transaction_commit  = 2,    //提交事务
        Transaction_rollback  = 3     //回滚事务
    };

    DbUtil();
    ~DbUtil();
    bool setFilename(QString fname);
    bool init();
    void close();

    //账户配置变量访问函数
    bool getCfgVariable(QString name, QVariant &value);
    bool setCfgVariable(QString name, QVariant value);

    //账户升级事务相关
    bool importFstSubjects(int subSys);
    //bool getSubSysJoinCfgInfo(SubjectManager *src, SubjectManager *des, QList<SubSysJoinItem*>& cfgs);
    //bool setSubSysJoinCfgInfo(SubjectManager *src, SubjectManager *des, QList<SubSysJoinItem *> &cfgs);
    bool tableExist(QString tableName);

    //账户信息相关
    //bool readAccBriefInfo(AccountBriefInfo& info);
    bool initAccount(Account::AccountInfo &infos);
    bool initSuites(QList<AccountSuiteRecord*> &suites);
    bool saveAccountInfo(Account::AccountInfo &infos);

    //银行相关
    bool saveBankInfo(Bank* ba,bool isDel = false);

    //帐套相关
    bool saveSuites(QList<AccountSuiteRecord *> &suites);
    bool saveSuite(AccountSuiteRecord* suite);

    //科目相关
    void clearStaticDataMember();
    bool initNameItems();
    bool initSubjects(SubjectManager* smg, int subSys);
    bool saveNameItemClass(int code, QString name, QString explain);
    bool saveNameItem(SubjectNameItem* ni);
    bool removeNameItem(SubjectNameItem* ni);
    bool removeNameItemCls(int code);
    bool removeSndSubjects(QList<SecondSubject*> subs);
    bool saveSndSubject(SecondSubject* sub);
    bool saveSndSubjects(QList<SecondSubject*> subs);
    bool savefstSubject(FirstSubject* fsub);
    int getBankSubMatchMoney(SecondSubject* sub);
    bool nameItemIsUsed(SubjectNameItem* ni);
    bool ssubIsUsed(SecondSubject* ssub);
    bool ssubIsUsedInExtraTable(SecondSubject* ssub);
    //bool isSubSysImported(int subSys);
    //bool isSubSysJoinConfiged(int source, int destinate);
    bool mergeSecondSubject(int startYear,int startMonth,int endYear, int endMonth, SecondSubject* preSub, QList<SecondSubject*> mergedSubs, bool isInclude);
    bool replaceMapSidWithReserved(SecondSubject *preSub, QList<SecondSubject *> mergedSubs);
    bool removeNameAlias(NameItemAlias* alias);
    bool upgradeNameAlias(SubjectNameItem* ni, NameItemAlias* alias);

    //货币相关
    bool initMoneys(Account* account);
    bool initBanks(Account* account);
    bool moneyIsUsed(Money* mt, bool &used);
    bool saveMoneys(QList<Money*> moneys);
    bool removeMoney(Money* mt);
    bool addMoney(Money* mt);


    //凭证数统计
    bool scanPzSetCount(int y, int m, int &repeal, int &recording, int &verify, int &instat, int &amount);

    //余额相关
    bool readAllExtraForSSubMMt(int y, int m, int mt, QList<int> sids,QHash<int,Double>& vs, QHash<int,MoneyDirection>& dirs);
    bool readAllWbExtraForFSub(int y, int m, QList<int> sids, QList<int> mts, QHash<int,Double>& vs, QHash<int,Double>& wvs,QHash<int,MoneyDirection>& dirs);
    bool readExtraForMF(int y, int m, int mt, int fid, Double &v, Double &wv, MoneyDirection& dir);
    bool readExtraForMS(int y, int m, int mt, int sid, Double &v, Double &wv, MoneyDirection& dir);
    bool readExtraForFSub(int y,int m, int fid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,MoneyDirection>& dir);
    bool readExtraForSSub(int y,int m, int sid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,MoneyDirection>& dir);

    //bool saveExtraForSSub(int y,int m, int fid, const QHash<int,Double>& v, const QHash<int,Double>& wv,const QHash<int,MoneyDirection>& dir);
    bool readExtraForPm(int y,int m, QHash<int,Double>& fsums,
                                     QHash<int,MoneyDirection>& fdirs,
                                     QHash<int,Double>& ssums,
                                     QHash<int,MoneyDirection>& sdirs);
    bool readExtraForMm(int y,int m, QHash<int,Double>& fsums,
                                     QHash<int,Double>& ssums);
    bool saveExtraForPm(int y, int m, const QHash<int, Double>& fsums,
                                      const QHash<int, MoneyDirection>& fdirs,
                                      const QHash<int, Double>& ssums,
                                      const QHash<int, MoneyDirection>& sdirs);
    bool saveExtraForMm(int y, int m, const QHash<int, Double>& fsums,
                                      const QHash<int, Double>& ssums);
    //bool verifyPMUnity(int y, int m, Money* mt, FirstSubject* fsub, Double rate);
    bool verifyExtraForFsub(int y, int m, FirstSubject* fsub);
    bool readExtraForAllSSubInFSub(int y, int m, FirstSubject* fsub, QHash<int, Double>& pvs, QHash<int, MoneyDirection> &dirs,
                                   QHash<int, Double>& mvs);
    bool saveExtraForAllSSubInFSub(int y, int m, FirstSubject* fsub,
                          const QHash<int, Double> fpvs, const QHash<int, Double> fmvs,
                          QHash<int, MoneyDirection> fdirs, const QHash<int, Double> &v,
                          const QHash<int, Double> &wv, const QHash<int, MoneyDirection> &dir);

    bool convertExtraInYear(int year, const QHash<int,int> maps, bool isFst = true);
    bool convertPzInYear(int year, const QHash<int,int> fMaps, const QHash<int,int> sMaps);
    bool isUsedWbForFSub(FirstSubject* fsub, bool &isExist);
    bool getMixJoinInfo(int sc, int dc, QList<MixedJoinCfg *> &cfgInfos);
    bool appendMixJoinInfo(int sc, int dc, QList<MixedJoinCfg *> cfgInfos);

    //日记账
    bool getDetViewFilters(int suiteId, QList<DVFilterRecord*>& rs);
    bool saveDetViewFilter(const QList<DVFilterRecord *> &dvf);
    bool getDailyAccount2(QHash<int,SubjectManager*> smgs, QDate sd, QDate ed, int fid, int sid, int mt,
                            Double& prev, int& preDir,
                            QList<DailyAccountData2*>& datas,
                            QHash<int,Double>& preExtra,
                            QHash<int,Double>& preExtraR,
                            QHash<int,int>& preExtraDir,
                            QHash<int, Double>& rates,
                            QList<int> subIds = QList<int>(),
                            //QHash<int,QList<int> > sids = QHash<int,QList<int> >(),
                            Double gv = 0.00,
                            Double lv = 0.00,
                            bool inc = false);

    //提供给SubjectComplete类的数据模型所用的查询对象
    QSqlQuery* getQuery(){/*QSqlQuery q(db); return q;*/return new QSqlQuery(db);}
    QSqlDatabase& getDb(){return db;}

    //服务函数
    bool getFS_Id_name(QList<int> &ids, QList<QString> &names, int subSys = 1);

    //凭证集相关
    bool getPzsState(int y,int m,PzsState& state);
    bool setPzsState(int y,int m,PzsState state);
    bool setExtraState(int y, int m, bool isVolid);
    bool getExtraState(int y, int m);
    bool getRates(int y, int m, QHash<int,Double>& rates);
    bool saveRates(int y,int m, QHash<int,Double>& rates);
    bool loadPzSet(int y, int m, QList<PingZheng*> &pzs, AccountSuiteManager *parent);
    bool isContainPz(int y, int m, int pid);
    bool inspectJzPzExist(int y, int m, PzdClass pzCls, int& count);



    bool clearPzSet(int y, int m);
    bool clearRates(int y, int m);
    bool clearExtras(int y, int m);

    //凭证相关
    bool getPz(int pid, PingZheng*& pz, AccountSuiteManager *parent);
    bool savePingZhengs(QList<PingZheng*> pzs);
    bool savePingZheng(PingZheng* pz);
    bool delPingZhengs(QList<PingZheng*> pzs);
    bool delPingZheng(PingZheng* pz);

    ////////////////////////////////////////////////////////////////////
    bool assignPzNum(int y, int m);
    bool crtNewPz(PzData* pz);
    bool delActionsInPz(int pzId);
    bool getActionsInPz(int pid, QList<BusiActionData2*>& busiActions);
    bool saveActionsInPz(int pid, QList<BusiActionData2*>& busiActions,
                                    QList<BusiActionData2*> dels = QList<BusiActionData2*>());
    bool delSpecPz(int y, int m, PzdClass pzCls, int &affected);
    QList<PzClass> getSpecClsPzCode(PzdClass cls);
    bool haveSpecClsPz(int y, int m, QHash<PzdClass,bool>& isExist);
    bool specPzClsInstat(int y, int m, PzdClass cls, int &affected);
    bool setAllPzState(int y, int m, PzState state, PzState includeState,
                                  int &affected, User* user);
    //////////////////////////////////////////////////////////////////

    //笔记功能函数
    bool readNotes(QList<NoteStruct *> &notes);
    bool saveNote(NoteStruct* note, bool isDel=false);
    //子窗口状态访问函数
    bool getSubWinInfo(int winEnum, QByteArray *state);
    bool saveSubWinInfo(int winEnum, QByteArray *state);

    //智能子目适配项访问函数
    bool loadSmartSSubAdaptes(SubjectManager* sm, QList<SmartSSubAdapteItem *> &items);
    bool saveSmartSSubAdapters(QList<SmartSSubAdapteItem *> &items,bool del=false);

    //发票使用情况记录访问函数
    bool removeInvoiceRecords(QList<InvoiceRecord*> records);
    bool clearInvoiceRecords(int year, int month);
    bool saveInvoiceRecords(QList<InvoiceRecord*> records);
    bool saveInvoiceRecords(int year, int month, QList<InvoiceRecord*> records);
    bool getInvoiceRecordsForYear(AccountSuiteManager *asMgr, QList<InvoiceRecord*> &incomes, QList<InvoiceRecord*> &costs);
    bool getInvoiceRecords(int year, int month, QList<InvoiceRecord*> &incomes, QList<InvoiceRecord*> &costs);

    //本月收入/成本发票访问函数
    bool loadCurInvoice(int y, int m, QList<CurInvoiceRecord*> &records, bool isYs=true);
    bool saveCurInvoice(int y, int m, const QList<CurInvoiceRecord*> &records);
    bool clearCurInvoice(int y, int m,int scope=0);
    bool saveIsolatedNameAlias(NameItemAlias* nameAlias);
    bool readCostInvoiceForTax(QString inum,Double &tax,Double &money,QString &client,QString &date);

    //分录模板数据访问函数
    bool existBaTemlateDatas(int type);
    bool readBaTemplateDatas(int type, QList<InvoiceRowStruct*> &datas);
    bool saveBaTemplateDatas(int type,const QList<InvoiceRowStruct*> &datas);

    bool findPz(const PzFindFilteCondition &filter, QList<PzFindBaContent*> &bas, bool &hasnMore, int count=100,bool nextPage=false);

    //进项税发票管理类函数
    bool crtJxTaxTable();
    bool clearCurAutoInv(int y,int m);
    bool readCurAuthCostInvAmount(int y, int m,Double& value);
    bool updateCurAuthCostInvAmount(int y, int m,Double value);
    bool readCurAuthCostInvoices(int y,int m,QList<CurAuthCostInvoiceInfo*>& rs,SubjectManager* sm);
    bool saveCurAuthCostInvoices(int y,int m,QList<CurAuthCostInvoiceInfo*> rs);
    bool readHisNotAuthCostInvoices(SubjectManager *sm, QList<HisAuthCostInvoiceInfo*> &rs);
    bool updateHisNotAuthCosInvoices(QList<HisAuthCostInvoiceInfo*> rs);
    bool removeHisNotAuthCosInvoices(QList<HisAuthCostInvoiceInfo *> &rs);
    QString getOriginalSummary(int bid);

    //自动凭证生成相关
    bool saveJournals(QList<Journal* > js);
    bool readJournals(int year,int month,QList<Journal* > &js,SubjectManager* sm);
    bool readJournays(int y, int m, QList<SecondSubject*> &bankSubs, QList<QList<Journal *> *> &jjs, SubjectManager* sm);
    bool saveJournalizings(QList<Journalizing* > js);
    bool readJournalizings(QList<Journalizing* > &js, const QHash<int, Journal *> &jMaps, SubjectManager* sm);
    bool removeJournalizings(const QList<Journalizing *> js);
    bool clearJournalizings();

private:
    bool saveAccInfoPiece(InfoField code, QString value);
    bool _readAccountSuites(QList<AccountSuiteRecord*>& suites);
    bool _saveAccountSuite(AccountSuiteRecord* suite);



    //科目相关
    bool _saveFirstSubject(FirstSubject* sub);
    bool _saveSecondSubject(SecondSubject* sub);
    bool _removeSecondSubject(SecondSubject* sub);
    bool _saveNameItem(SubjectNameItem* ni);
    bool _removeNameItem(SubjectNameItem* ni);
    bool _mergeExtraWithinRange(int startYear,int startMonth,int endYear, int endMonth, SecondSubject* preSub, QList<SecondSubject*> mergedSubs,bool isInclude);
    bool _mergeExtraWithinMonth(int year, int month, SecondSubject* preSub, QList<SecondSubject*> mergedSubs);
    bool _mergeExtra(int point, SecondSubject* preSub, QList<SecondSubject*> mergedSubs,bool isPrimary = true);
    bool _replaceSidWithResorved(int startYear,int startMonth,int endYear, int endMonth, SecondSubject* preSub, QList<SecondSubject*> mergedSubs);
    bool _replaceSidWithResorved2(int startYear,int startMonth,int endYear, int endMonth, SecondSubject* preSub, QList<SecondSubject*> mergedSubs);

    //凭证相关
    bool _savePingZheng(PingZheng* pz);
    bool _saveBusiactionsInPz(PingZheng* pz);
    bool _delPingZheng(PingZheng* pz);

    //余额相关辅助函数
    bool _readExtraPointInYear(int y, QList<int> &points);
    bool _readExtraPoint(int y, int m, QHash<int, int> &mtHashs);
    bool _readExtraPoint(int y, int m, int mt, int& pid);
    bool _readExtraForPm(int y, int m, QHash<int,Double> &sums, QHash<int,MoneyDirection> &dirs, bool isFst = true);
    bool _readExtraForMm(int y, int m, QHash<int,Double> &sums, bool isFst = true);
    void _replaeAccurateExtra(QHash<int,Double> &sums, QHash<int,Double> &asums);
    bool _crtExtraPoint(int y, int m, int mt, int& pid);
    bool _saveExtrasForPm(int y, int m, const QHash<int,Double> &sums, const QHash<int,MoneyDirection> &dirs, bool isFst = true);
    bool _saveExtrasForMm(int y, int m, const QHash<int,Double> &sums, bool isFst = true);
    bool _readExtraForSubMoney(int y, int m, int mt, int sid, Double &v, Double &wv, MoneyDirection& dir,bool fst=true);
    bool _readExtraForFSub(int y,int m, int fid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,MoneyDirection>& dir);
    bool _readExtraForSSub(int y,int m, int sid, QHash<int,Double>& v, QHash<int,Double>& wv,QHash<int,MoneyDirection>& dir);
    bool _readExtrasForSubLst(int y,int m, const QList<int> sids, QHash<int,Double>& pvs, QHash<int,Double>& mvs,QHash<int,MoneyDirection>& dirs,bool isFst = true);
    bool _saveExtrasForSubLst(int y,int m, const QList<int> sids, const QHash<int,Double>& pvs, const QHash<int,Double>& mvs, const QHash<int,MoneyDirection>& dirs,bool isFst = true);
    bool _convertExtraInYear(int year, const QHash<int,int> maps, bool isFst = true);
    //
    int _genKeyForExtraPoint(int y, int m, int mt);
    bool _isTransformExtra(int y, int fid, int sid, QList<int> &tfids, int &tsid);
    bool _transformExtra(QHash<int,int> maps, QHash<int,Double>& ExtrasP, QHash<int,Double>& extrasM, QHash<int,MoneyDirection>& dirs);
    bool _extraUnityInspectForFSub(FirstSubject* fsub, int mt, Double sum, MoneyDirection dir, QHash<int,Double> values, QHash<int,MoneyDirection> dirs, bool& ok);

    //表格创建函数
    void crtGdzcTable();

    //

    void warn_transaction(ErrorCode witch, QString context);
    void errorNotify(QString info);
    bool isNewExtraAccess(){return (mv==nmv && sv>=nsv) || (mv>nmv);}//是否采用新余额存取机制

    void _getPreYM(int y, int m, int& yy, int& mm);
    void _getNextYM(int y, int m, int& yy, int& mm);

    //笔记功能函数
    bool initNoteTable();



private:
    QSqlDatabase db;
    int masterMt;   //母币代码（在访问余额状态时要用，因为该状态只保存在母币余额中）
    QString fileName;   //账户文件名
    QHash<InfoField,QString> pNames; //账户信息片段名称表
    QHash<int,int> extraPoints;  //余额指针缓存表（键为余额的年、月和币种所构成的复合键（高4位是年份，中2位是月份，低1位是币种），值为SE_Point表的id）
                                 //使用此变量是为了减少读取指针表的次数，现在暂缓实施
    int mv,sv;  //账户文件版本号（临时保存，用于那些代理函数执行前的版本比较）
};

#endif // DBUTIL_H
