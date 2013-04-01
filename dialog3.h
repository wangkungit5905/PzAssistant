#ifndef DIALOG3_H
#define DIALOG3_H

#include <QCompleter>
#include <QSqlQueryModel>
#include <QStandardItemModel>
#include <QPrinter>


#include "common.h"
#include "widgets.h"
#include "HierarchicalHeaderView.h"
#include "printtemplate.h"
#include "previewdialog.h"
#include "account.h"

#include "ui_impothmoddialog.h"
#include "ui_antijzdialog.h"
#include "ui_gdzcadmindialog.h"
#include "ui_dtfyadmindialog.h"
#include "ui_happensubseldialog.h"
#include "ui_showtzdialog.h"
#include "ui_showdzdialog.h"
#include "ui_historypzdialog.h"
#include "ui_lookupsubjectextradialog.h"
#include "ui_accountpropertydialog.h"

//int pagingcal(int num){}

//引入其他模块凭证时的选择对话框类
class ImpOthModDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImpOthModDialog(int witch = 1, QWidget *parent = 0);
    ~ImpOthModDialog();
    bool isSelSkip();
    bool isSelGdzc();
    bool isSelDtfy();
    bool selModules(QSet<OtherModCode>& selMods);

private slots:
    void on_btnSkip_clicked();

private:
    Ui::ImpOthModDialog *ui;
    bool isSkip;
};

//反结转选择对话框类
class AntiJzDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AntiJzDialog(QHash<PzdClass,bool> haved, QWidget *parent = 0);
    ~AntiJzDialog();

    QHash<PzdClass, bool> selected();

private slots:


    void on_chkJzlr_clicked(bool checked);

    void on_chkJzhd_clicked(bool checked);

    void on_chkJzsy_clicked(bool checked);

private:
    Ui::AntiJzDialog *ui;
    QHash<PzdClass,bool> haved; //各类别的凭证是否已经实际结转了
};

//固定资产管理对话框
class GdzcAdminDialog : public QDialog
{
    Q_OBJECT

    static const int ColDate  = 0;
    static const int ColValue = 1;
    static const int ColPid   = 2;
    static const int ColBid   = 3;
    static const int ColId    = 4;

public:
    explicit GdzcAdminDialog(QByteArray *sinfo, QWidget *parent = 0);
    ~GdzcAdminDialog();
    void save(bool isConfirm = true);
    QByteArray* getState();
    void setState(QByteArray* info);

private slots:
    void curGzdcChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void nameChanged(const QString &text);
    void productClassChanged(int index);
    void subjectClassChanged(int index);
    void buyDateChanged(const QDate& date);
    void modelChanged(const QString &text);
    void primeChanged(const QString &text);
    //void remainChanged(const QString &text);
    void minPriceChanged(const QString &text);
    void zjMonthsChanged(int i);
    void otherInfoChanged();
    void zjItemInfoChanged(QTableWidgetItem* item);
    void vlistColWidthChanged(int logicalIndex, int oldSize, int newSize);
    void zjInfosColWidthChanged(int logicalIndex, int oldSize, int newSize);
    void zjPzColWidthChanged(int logicalIndex, int oldSize, int newSize);

    void on_btnAdd_clicked();

    void on_btnDel_clicked();

    //void on_btnAddZj_clicked();

    void on_btnDelZj_clicked();

    void on_btnOk_clicked();

    void on_btnSave_clicked();

    void on_rdoYear_toggled(bool checked);

    void on_actJtzj_triggered();

    void on_actPreview_triggered();

    void on_actViewList_triggered();

    void on_actSupply_triggered();

    void on_actNextZj_triggered();

    void on_chkHand_clicked(bool checked);

    void on_rdoAll_toggled(bool checked);

    void on_btnCancel_clicked();

    void on_btnRepeal_clicked();

private:
    void initCmb();
    void load(bool isAll);
    void viewInfo(int index);
    void viewZj(Gdzc* g);
    void bindSingal(bool bind = true);    
    void addZjInfo(QDate d,double v,int index);
    //void delZjInfos(int y, int m, int gid = 0);

    Ui::GdzcAdminDialog *ui;
    QDoubleValidator dv;     //用以数值输入部件的验证
    QList<Gdzc*> gdzcLst;    //固定资产对象列表
    QList<bool> isChanged;   //与上面这个对应的，固定资产内容是否被修改的标记
    QList<Gdzc*> delLst;     //被删除的固定资产列表
    //QTableWidgetItem* curItem; //当前选择的折旧信息条目中的一个字段
    Gdzc* curGdzc;            //当前选定的固定资产

    //这两个哈希表，从固定资产科目类别代码（SecSubjects表）到相关子目（FSAgent表）的映射
    //这些主要在建立折旧凭证和搜索固定资产时可用。
    QHash<int,int> gdzcSubIDs; //固定资产主目下的子目
    QHash<int,int> ljzjSubIDs; //累计折旧主目下的子目
    bool isCancel;             //是否取消在固定资产管理界面所做的修改
    QList<int> plistColWidths; //打印固定资产折旧清单的表格列宽
    QList<int> vlistColWidths; //显示固定资产折旧清单的表格列宽
    QSize vlistWinRect;        //显示固定资产折旧清单的窗口大小
    QList<int> zjInfoColWidths;//显示折旧信息的表格列宽
    QList<int> pzColWidths;    //预览折旧凭证的会计分录的表格列宽
    QSize pzWinRect;           //预览折旧凭证的窗口大小
};


//待摊费用管理对话框
class DtfyAdminDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DtfyAdminDialog(QByteArray *sinfo, QSqlDatabase db = QSqlDatabase::database(),
                             QWidget *parent = 0);
    ~DtfyAdminDialog();
    void save(bool isConfirm = true);
    QByteArray* getState();
    void setState(QByteArray* info);

private slots:
    void curDtfyChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void TxItemInfoChanged(QTableWidgetItem* item);
    void typeChanged(int index);
    void nameChanged(const QString &text);
    void importDateChanged(const QDate &date);
    void dFstSubChanged(int index);
    void dSndSubChanged(int index);
    void totalValueChanged(const QString &text);
    void remainValueChanged(const QString &text);
    void monthsChanged(int m);
    void replainTextChanged();
    void startDateChanged(const QDate &date);
    void vlistColWidthChanged(int logicalIndex, int oldSize, int newSize);
    void txItemColWidthChanged(int logicalIndex, int oldSize, int newSize);
    void TxPzColWidthChanged(int logicalIndex, int oldSize, int newSize);

    void on_actJttx_triggered();

    void on_actPreview_triggered();

    void on_actView_triggered();

    void on_btnComplete_clicked();

    void on_btnDelTx_clicked();

    void on_btnAdd_clicked();

    void on_btnDel_clicked();

    void on_actRepeal_triggered();

    void on_rdoAll_toggled(bool checked);

    void on_checkBox_clicked(bool checked);

    void on_btnSave_clicked();

    void on_btnCancel_clicked();

    void on_btnOk_clicked();

    void on_cmbFsub_currentIndexChanged(int index);

    void on_actCrtDtfyPz_triggered();

    void on_actRepealDtfyPz_triggered();

    void on_actPreviewDtfyPz_triggered();

private:
    void load(bool isAll);
    void viewInfo();
    void viewDSub();
    void viewFtItem();
    void bindSignal(bool bind = true);
    void insertTxInfo(QDate date,double v,int index);
    void removeTxInfo(int idx);
    void preview(QString title,QString d,QList<Dtfy::BaItemData*> datas);

    Ui::DtfyAdminDialog *ui;
    QList<Dtfy*> dtfys;    //待摊费用对象列表
    QList<bool> isChanged; //待摊费用对象是否被修改的标记列表
    QList<Dtfy*> dels;     //已被删除的待摊费用对象列表
    Dtfy* curDtfy;         //当前选择的待摊费用对象
    bool isCancel;         //
    QHash<int,DtfyType*> dTypes;  //待摊费用类别id到类别对象的映射表

    QList<int> plistColWidths; //打印摊销清单表格的列宽
    QList<int> vlistColWidths; //显示摊销清单的表格列宽
    QSize vlistWinRect;        //显示摊销清单的窗口大小

    QList<int> txItemColWidths;//显示摊销项的表格列宽

    QList<int> pzColWidths;    //预览摊销凭证的会计分录的表格列宽
    QSize pzWinRect;           //预览摊销凭证的窗口大小

    QHash<int, QHash<int,QString> > dsids;  //贷方子目，键为贷方主目id，值为该主目下的子目的id到名称的映射表

    QSqlDatabase db;
};

//科目余额与发生额科目选择对话框
class HappenSubSelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HappenSubSelDialog(int y, int m, QWidget *parent = 0);
    ~HappenSubSelDialog();

    void getSubRange(int& witch, QList<int>& fids, QHash<int,QList<int> >& sids,
                     double& gv, double& lv, bool& inc);
    void getTimeRange(int& sm, int& em);
    bool getExtraLimit(double& great, double& less);
    bool isIncDotInstat();
    bool isViewDotHapp();

private slots:
    void startFstSubChanged(int index);
    void endFstSubChanged(int index);

    void on_cmbSubCls_currentIndexChanged(int index);

    void on_btnOk_clicked();

private:
    Ui::HappenSubSelDialog *ui;

    QDoubleValidator dv;  //用于两个余额输入框的验证器
    SubjectComplete *sfCom,*ssCom,*efCom,*esCom;
    int cury,curm;  //要查看的帐的所在年月（月份数尽在开始和结束月份都是本月时有效）
};

//总账视图窗口类
class ShowTZDialog : public QDialog
{
    Q_OBJECT

public:
    //表格显示格式
    enum TableFormat{
        NONE         =0,   //
        COMMON       =1,   //通用金额式
        THREERAIL    =2    //三栏明细式（由银行、应收/应付等使用，带有外币栏）
    };

    explicit ShowTZDialog(int y, int m, QByteArray* sinfo = NULL, QWidget *parent = 0);
    ~ShowTZDialog();
    void setState(QByteArray* info);
    QByteArray* getState();

private slots:
    void onSelEndMonth(int index);
    void onSelSub(int index);
    void onSelStartMonth(int index);
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);

    void on_actToPdf_triggered();

    void on_actPrint_triggered();

    void on_actPreview_triggered();

private:
    void refreshTable();

    //生成指定格式表头的函数
    void genThForCommon();
    void genThForThreeRail();

    //生成指定格式表格数据的函数
    void genDataForCommon();
    void genDataForThreeRail();
    void printCommon(PrintTask task, QPrinter* printer);

    Ui::ShowTZDialog *ui;    
    SubjectComplete *fcom;
    int y;      //帐套年份
    int m;      //当前月份
    int sm,em;  //开始和结束月份

    int fid;  //当前选择的一级科目id
    QList<int> mts; //这是要在表格的借、贷和余额栏显示的外币列表
    QHash<TableFormat,QList<int> > colWidths;     //表格列宽度
    QHash<TableFormat,QList<int> > colPrtWidths;  //打印模板中的表格列宽度值
    QPrinter::Orientation pageOrientation;  //打印模板的页面方向
    PageMargin margins;  //页面边距

    HierarchicalHeaderView* hv;         //表头
    ProxyModelWithHeaderModels* imodel; //与表格视图相连的包含了表头数据模型的代理模型
    QStandardItemModel* headerModel;    //表头数据模型
    QStandardItemModel* dataModel;      //表格内容数据模型
    TableFormat curFormat;              //当前表格格式
};

//明细账视图窗口类
class ShowDZDialog : public QDialog
{
    Q_OBJECT

public:
    //表格显示格式
    enum TableFormat{
        NONE         =0,   //
        CASHDAILY    =1,   //现金日记账格式
        BANKRMB      =2,   //银行日记账格式（人民币）
        BANKWB       =3,   //银行日记账格式（外币）
        COMMON       =4,   //通用金额式
        THREERAIL    =5    //三栏明细式（由应收/应付等使用）
    }; 

    explicit ShowDZDialog(Account* account, QByteArray* sinfo = NULL, QWidget *parent = 0);
    ~ShowDZDialog();
    void setSubRange(int witch, QList<int> fids, QHash<int,QList<int> > sids,
                     double gv, double lv, bool inc);
    void setDateRange(int sm, int em, int y);
    void setState(QByteArray* info);
    QByteArray* getState();

private slots:
    void onSelFstSub(int index);
    void onSelSndSub(int index);
    void onSelMt(int index);
    void moveTo();
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);
    void paging(int rowsInTable, int& pageNum);
    void renPageData(int pageNum, QList<int>*& colWidths, QStandardItemModel& pdModel,
                     QStandardItemModel& phModel);    
    //void priorPaging(bool out, int pages);

    void on_actPrint_triggered();

    void on_actPreview_triggered();

    void on_actToPdf_triggered();

    //void on_btnClose_clicked();

signals:
    void openSpecPz(int pid, int bid); //打开指定id的凭证
    //void closeWidget();                //向对话框的父（mdi子窗口）报告，我要关闭了

private:
    void refreshTalbe();
    //void genTData(TableFormat tf);

//    void getDatas(int y, int sm, int em, int fid, int sid, int mt,
//                  QList<DailyAccountData*>& datas,
//                  QHash<int,double> preExtra,
//                  QHash<int,double> preExtraDir,
//                  QHash<int, double> rates);
    //生成指定格式表格数据的函数
    int genDataForCash( QList<DailyAccountData2*> datas,
                        QList<QList<QStandardItem*> >& pdatas,                        
                        Double prev, int preDir,
                        QHash<int,Double> preExtra,
                        QHash<int,int> preExtraDir,
                        QHash<int,Double> rates);
    int genDataForBankRMB(QList<DailyAccountData2 *> datas,
                           QList<QList<QStandardItem*> >& pdatas,                           
                           Double prev, int preDir,
                           QHash<int, Double> preExtra,
                           QHash<int,int> preExtraDir,
                           QHash<int, Double> rates);
    int genDataForBankWb(QList<DailyAccountData2*> datas,
                          QList<QList<QStandardItem*> >& pdatas,                          
                          Double prev, int preDir,
                          QHash<int,Double> preExtra,
                          QHash<int,int> preExtraDir,
                          QHash<int,Double> rates);
    int genDataForDetails(QList<DailyAccountData2*> datas,
                           QList<QList<QStandardItem*> >& pdatas,
                           Double prev, int preDir,
                           QHash<int,Double> preExtra,
                           QHash<int,int> preExtraDir,
                           QHash<int,Double> rates);
    int genDataForDetWai(QList<DailyAccountData2*> datas,
                          QList<QList<QStandardItem*> >& pdatas,
                          Double prev, int preDir,
                          QHash<int,Double> preExtra,
                          QHash<int,int> preExtraDir,
                          QHash<int,Double> rates);

    //生成指定格式表头的函数
    void genThForCash(QStandardItemModel* model = NULL);
    void genThForBankRmb(QStandardItemModel* model = NULL);
    void genThForBankWb(QStandardItemModel* model = NULL);
    void genThForDetails(QStandardItemModel* model = NULL);
    void genThForWai(QStandardItemModel* model = NULL);

    void printCommon(QPrinter* printer);
    TableFormat decideTableFormat(int fid,int sid,int mt);

    Ui::ShowDZDialog *ui;
    SubjectComplete *fcom, *scom;
    int witch;                  //科目选择模式（1：所有科目，2：指定类型科目，3：指定范围科目）
    QList<int> fids;            //一级科目的id列表（在选择模式3时有用）
    QHash<int,QList<int> > sids;//二级科目的id列表，键为一级科目id
    QHash<int,QList<QString> > sNames; //二级科目名列表
    double gv,lv;  //业务活动涉及的金额上下限
    bool inc;      //是否包含未入账凭证

    QHash<TableFormat,QList<int> > colWidths; //各种表格格式列宽度值
    QHash<TableFormat,QList<int> > colPrtWidths; //打印模板中的表格列宽度值
    QPrinter::Orientation pageOrientation;  //打印模板的页面方向
    PageMargin margins;  //页面边距

    //bool isInit;   //对象是否处于初始化状态的标志（即对象的构造函数阶段，还没有调用setDateRange方法）



    //用户当前选择的科目币种状态
    int fid;  //当前选择的一级科目id
    int sid;  //当前选择的二级科目id
    int mt;   //当前币种
    //与当前表格数据相对应的科目与币种的选择组合状态（通过与上面的状态相比较来决定更新表格数据）
    int tfid; int tsid; int tmt;

    //时间信息
    int cury;  //帐套年份
    int sm,em; //开始和结束月份

    //这两个可以合并
    QList<int> mtLst;  //币种代码列表（第一个始终是人民币）
    QList<int> mts; //这是要在表格的借、贷和余额栏显示的外币列表

    TableFormat tf,otf;                     //当前表格格式

    //视图显示有关的数据成员
    HierarchicalHeaderView* hv;         //表头
    QStandardItemModel* headerModel;    //表头数据模型
    QStandardItemModel* dataModel;      //表格内容数据模型
    ProxyModelWithHeaderModels* imodel; //与表格视图相连的包含了表头数据模型的代理模型

    //分页处理有关成员
    PrintTemplateDz* pt;                //打印模板类
    QStandardItemModel* pHeaderModel;   //分页处理后的表头数据模型
    QStandardItemModel* pDataModel;     //分页处理后的数据模型
    //ProxyModelWithHeaderModels* ipmodel;//分页处理后的表格代理模型
    QList<QList<QStandardItem*> > pdatas; //打印页面行数据（分页的边界由pageIndexes的元素值指定）
    QList<int> pageIndexes;             //每页的表格最后一行在pdatas列表中的索引值
    QList<TableFormat> pageTfs;         //每页的表格格式
    int maxRows;                        //每页的表格内最多可拥有的行数
    QList<int> pfids,psids,pmts;        //保存每页所属的一二级科目和币种
    QHash<int,QString> subPageNum;      //页号（第几页/总数）
    int pages;
    PrintTask curPrintTask;             //当前打印任务类别

    PreviewDialog* preview;

    QAction* actMoveTo;  //转到该凭证的QAction
    Account* account;
    SubjectManager* smg;
};


//显示历史凭证的窗口类
class HistoryPzDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryPzDialog(int pzId, int bid = 0, QByteArray* sinfo = NULL, QWidget *parent = 0);
    ~HistoryPzDialog();
    void setPz(int pzId, int bid = 0);
    void setState(QByteArray* info);
    QByteArray* getState();

private slots:
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);

private:
    void updateContent();

    Ui::HistoryPzDialog *ui;
    int pzId;  //凭证id
    int bid;   //要选择的会计分录id
    QHash<int,QString> PzStates;   //凭证状态名表
    QHash<int,QString> userNames;  //用户名表
    QStandardItemModel* bm; //访问凭证业务活动的数据模型
    QList<int> colWidths;  //表格列宽
};


//查看科目余额的对话框类
class LookupSubjectExtraDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LookupSubjectExtraDialog(Account* account, QWidget *parent = 0);
    ~LookupSubjectExtraDialog();

private slots:
    void monthChanged(int v);
    void fstSubChanged(int index);
    void sndSubChanged(int index);

    void on_spnYear_valueChanged(int year);

private:
    void refresh();

    Ui::LookupSubjectExtraDialog *ui;

    SubjectComplete *fCom,*sCom;
    int fid,sid;  //选择的一二级科目id
    int y,m;      //年，月
    QHash<int,Double> fsums,ssums; //一二级科目余额及其方向，键为科目id * 10 + 币种代码
    QHash<int,int> fdirs,sdirs;
    QList<int> mts;                //币种代码列表
    QStandardItemModel* model;
    Account* account;
    SubjectManager* sm;
};


//显示和编辑账户属性的对话框类
class AccountPropertyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountPropertyDialog(Account* account, QWidget *parent = 0);
    ~AccountPropertyDialog();
    bool isDirty();
    void save(bool confirm = false);

private slots:

    void on_btnAddMt_clicked();

    void on_btnOk_clicked();

    void on_btnCancel_clicked();

private:


    Ui::AccountPropertyDialog *ui;
    Account* account;

    //修改标记
    bool bCode;
    bool bSName;
    bool bLName;
    bool bSubType;
    bool bRptType;
    bool bSTime;
    bool bETime;
    bool bWaiMt;
    bool changed;

    QString info; //提示哪些信息发生了改变的信息串
    bool cancel;  //
};
#endif // DIALOG3_H

