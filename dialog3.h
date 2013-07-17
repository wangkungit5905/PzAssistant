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
#include "otherModule.h"

#include "ui_impothmoddialog.h"
#include "ui_antijzdialog.h"
#include "ui_gdzcadmindialog.h"
#include "ui_dtfyadmindialog.h"
#include "ui_showtzdialog.h"
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
    explicit GdzcAdminDialog(Account* account, QByteArray *sinfo, QWidget *parent = 0);
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

    Account* account;
    SubjectManager* smg;
};


//待摊费用管理对话框
class DtfyAdminDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DtfyAdminDialog(Account* account, QByteArray *sinfo, QWidget *parent = 0);
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
    Account* account;
    SubjectManager* smg;
    QSqlDatabase db;
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




#endif // DIALOG3_H

