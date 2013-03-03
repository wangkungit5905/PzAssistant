#ifndef DIALOG2_H
#define DIALOG2_H

#include <QtGlobal>

#include <QWidget>
#include <QSqlQueryModel>
#include <QStandardItemModel>
#include <QMenu>

//#include "delegates.h"
#include "delegates.h"
#include "delegates2.h"
#include "securitys.h"
#include "HierarchicalHeaderView.h"
#include "widgets.h"
#include "account.h"

#include "ui_basummaryform.h"
#include "ui_ratesetdialog.h"
#include "ui_viewextradialog.h"
#include "ui_detailsviewdialog2.h"
#include "ui_printselectdialog.h"
#include "ui_setupbasedialog2.h"
#include "ui_logindialog.h"
#include "ui_seccondialog.h"
#include "ui_searchdialog.h"


#ifdef Q_OS_LINUX
#include "ExcelFormat.h"
//#include "BasicExcel.h"
using namespace ExcelFormat;
using namespace YExcel;
#endif

#ifdef Q_OS_WIN
#include "excelUtils.h"
#endif

#define FSTSUBTYPE QTreeWidgetItem::UserType+1  //放置一级科目的树节点的类型


class BASummaryForm : public QWidget
{
    Q_OBJECT

public:
    explicit BASummaryForm(QWidget *parent = 0);    
    ~BASummaryForm();
    void setData(QString data);
    QString getData();

signals:
    void dataEditCompleted(ActionEditItemDelegate::ColumnIndex col);

private:
    Ui::BASummaryForm *ui;
};

//Q_DECLARE_METATYPE(BASummaryForm)

//汇率设定对话框
class RateSetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RateSetDialog(QWidget *parent = 0);
    RateSetDialog(int witch, QWidget* parent = 0);
    ~RateSetDialog();

    void setCurRates(QHash<int,double>* rates);
    void setEndRates(QHash<int,double>* rates);

public slots:
    void rateChanged();
    void curMtChanged(int index);

private:
    Ui::RateSetDialog *ui;

    int witch; //1：用于设置当期汇率，2：设置期末（下月）汇率
    int curMt; //当前选择的币种代码
    QHash<int,double> *crates, *erates;
    QHash<int,QString> mnames;
};

//显示科目余额的对话框（并统计本期发生额）
class ViewExtraDialog : public QDialog
{
    Q_OBJECT

public:
    enum TableFormat{
        COMMON = 1,    //通用金额式
        THREERAIL = 2  //三栏式
    };

    struct StateInfo{
        TableFormat tFormat;  //最后关闭时，显示的表格格式
        bool viewDetails;     //最后关闭时，是否选择了显示明细选择框
        QPrinter::Orientation pageOrientation;  //打印模板的页面方向
        PageMargin margins;                        //页边距
        QHash<TableFormat, QList<int> > colWidths; //各种表格格式下的列宽
        QHash<TableFormat, QList<int> > colPriWidths; //打印各种表格格式时的列宽
    } stateInfo;

//    struct StateInfo2{
//        qint8 tFormat;      //最后关闭时，显示的表格格式
//        bool viewDetails;   //最后关闭时，是否选择了显示明细选择框
//        QHash<qint8, QList<qint16> > colWidths; //各种表格格式下的列宽
//    } stateInfo2;


    explicit ViewExtraDialog(int y = 2011, int m = 5, QByteArray* sinfo = NULL, QWidget *parent = 0);
    ~ViewExtraDialog();
    void setDate(int y, int m);
    void setState(QByteArray* info);
    QByteArray* getState();

public slots:
    //bool close();
    //void toExcel();
    void save();
    //void print();
    //void viewAgain(bool state);
    void colWidthChanged(int logicalIndex, int oldSize, int newSize);

signals:
    void infomation(QString info);       //向主窗口发送要在状态条上显示的信息
    void pzsExtraSaved();

private slots:
    void onSelFstSub(int index);
    void onSelSndSub(int index);

    void on_actPrint_triggered();

    void on_actPreview_triggered();

    void on_actToExcel_triggered();

    //void on_cmbFstSub_currentIndexChanged(int index);

    void onTableFormatChanged(bool checked);

    void onDetViewChanged(bool checked);

    void on_actToPDF_triggered();

private:
    void viewRates();
    void calSumByMt(QHash<int,Double> exasR, QHash<int,int>exaDirsR,
                    QHash<int,Double>& sumsR, QHash<int,int>& dirsR,
                    int witch = 1);
    void viewTable();
    void genHeaderDatas();
    void genDatas();
    void initHashs();
    void genSheetHeader(BasicExcel* xls, int sn = 0);
    void genSheetDatas(BasicExcel* xls, int sn = 0);
    void addToEndExtra(int key,double v,int curDir,
                       QHash<int,double> preExa,QHash<int,int>preExaDir,
                       QHash<int,double> endExa,QHash<int,int>endExaDir);
    void printCommon(PrintTask task, QPrinter* printer);



    Ui::ViewExtraDialog *ui;
    QPrinter* printer;
    int y,m; //统计数据集所属年月

    //与表格有关的数据成员
    HierarchicalHeaderView* hv;
    ProxyModelWithHeaderModels* imodel; //与表格视图相连的包含了表头数据模型的代理模型
    QStandardItemModel* headerModel;    //表头数据模型
    QStandardItemModel* dataModel; //表格内容数据模型

    //------------------------------------
    //QList<qint16> colPrtWidths;     //打印模板中的表格列宽度值
    //QPrinter::Orientation pageOrientation;  //打印模板的页面方向
    //PageMargin margins;  //页面边距

    //---------------------------------------

    QHash<int,QString> idToCode, sidToCode; //一二级科目id到科目代码的映射
    QHash<int,QString> idToName, sidToName; //一二级科目id到科目名称的映射
    QHash<int,Double> sRates,eRates; //期初、期末汇率表

    //数据表（键为科目id * 10 + 币种代码）
    QHash<int,Double> preExa, preDetExa;                    //期初余额（以原币计）
    QHash<int,Double> preExaR, preDetExaR;                  //期初余额（以本币计）
    QHash<int,int>    preExaDir,preDetExaDir;               //期初余额方向（以原币计）
    QHash<int,int>    preExaDirR,preDetExaDirR;             //期初余额方向（以本币计）
    QHash<int,Double> curJHpn, curJDHpn, curDHpn, curDDHpn; //当期借贷发生额（以原币计）
    QHash<int,Double> curJHpnR, curJDHpnR, curDHpnR, curDDHpnR; //当期借贷发生额（以本币计）
    QHash<int,Double> endExa, endDetExa;                    //期末余额（以原币计）
    QHash<int,Double> endExaR, endDetExaR;                  //期末余额（以本币计）
    QHash<int,int>    endExaDir,endDetExaDir;               //期末余额方向（以原币计）
    QHash<int,int>    endExaDirR,endDetExaDirR;             //期末余额方向（以本币计）

    //QList<int> /*mts,mts,mts*//*,mts*/; //分别是期初栏、当期借、贷方发生额栏、期末余额栏的外币代码列表
    QList<int> mts; //外币代码列表（用于保持外币金额显示的一致顺序）取代上面4个列表
    QMenu* mnuPrint; //附加在打印按钮上的菜单

    SubjectComplete *fcom, *scom;  //一二级科目选择框使用的完成器
    int fid,sid; //当前选择的一二级科目id
    bool isCanSave; //是否可以保存余额（基于当前的凭证集状态）
};

Q_DECLARE_METATYPE(ViewExtraDialog::StateInfo)
//Q_DECLARE_METATYPE(ViewExtraDialog::StateInfo2)

///////////////////////显示日记账、明细账的对话框///////////////////////////////////////

class DetailsViewDialog2 : public QDialog
{
    Q_OBJECT

public:
    explicit DetailsViewDialog2(int witch = 1, QWidget *parent = 0);
    ~DetailsViewDialog2();
    void refresh();

public slots:
    //void viewDetials();
    void saveExtra();
    void toExcelFile();
    void moveTo();
    void curSndSubChanged(const QString &text);
    void curSndSubChanged(int index);
    void selFstSub(int index);
    void setDateLimit(int sy,int sm,int ey,int em);
    void tableModeChanged();
    void toPdf();
    void printPreview();
    void print();

signals:
    void openSpecPz(int pid, int bid);  //打开包含此会计分录的凭证



private slots:
    void on_btnRefresh_clicked();

private:
    void initModel();
    void genDetails();
    void genTableHead();
    void getHappenMt();

    Ui::DetailsViewDialog2 *ui;
    int witch;  //1：现金日记账，2：银行日记账，3：明细账，4：总分类账    
    SubjectComplete *fcom,*scom;   //由一二级科目选择框使用的完成器

    QAction* actMoveTo;  //转到该凭证的QAction

    QHash<int,QString> jmt,dmt,emt; //借、贷和余额所发生的外币代码列表
    QList<int> jmtl,dmtl,emtl; //为确保外币显示顺序的一致性，用这些排序后的列表保存币种代码

    HierarchicalHeaderView* hv;
    ProxyModelWithHeaderModels* imodel; //与表格视图相连的包含了表头数据模型的代理模型
    QStandardItemModel* headerModel;    //表头数据模型
    QStandardItemModel* dataModel;      //表格内容数据模型

    int fid;      //当前选择的总账科目id;
    int sid;      //当前选择的明细科目id;
    int curPzId;  //当前选择业务活动所属凭证ID
    int curPzBgId; //当前选择业务活动所属凭证所属的凭证分册类型ID
    int viewCols;  //表格的总可见列数
};


//////////////////选择打印的凭证的对话框///////////////////////////////
class PrintSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrintSelectDialog(QWidget *parent = 0);
    ~PrintSelectDialog();
    void append(int num);
    void setPzSet(QSet<int> pznSet);
    void setCurPzn(int pzNum);
    void remove(int num);
    int getPrintPzSet(QSet<int>& pznSet);
    int getPrintMode();


private:
    Ui::PrintSelectDialog *ui;
    QSet<int> pznSet;  //欲对应的凭证号码的集合
};



///////////////////////////////////////////////////////
//设置基准数据的对话框类（一二级科目基准数据、各种会计报表的基准数据）
class SetupBaseDialog2 : public QDialog
{
    Q_OBJECT

public:
    //保存某个总账科目余额值条目的数据结构
    struct FstExtData{
        int mt;      //币种
        Double v;    //金额
        Double rv;   //外币对应的人民币金额
        int dir;     //方向
    };

    //保存某个明细科目余额值条目的数据结构
    struct DetExtData{
        int subId;   //科目id
        int mt;      //币种
        Double v;    //金额
        Double rv;   //外币对应的人民币金额
        int dir;     //方向
        bool tag;    //是否被标记，可以帮助用户识别此行数据是否被处理过
        QString desc;//与标记配合的由用户输入的描述信息
    };

    explicit SetupBaseDialog2(Account* account, QWidget *parent = 0);
    ~SetupBaseDialog2();
    void closeDlg();

private slots:

    void cellChanged(int row, int column);
    void newMapping(int fid, int sid, int row, int col);
    void newSndSub(int fid, QString name, int row, int col);
    void editNext(int row, int col);
    void sortIndicatorChanged(int logicalIndex, Qt::SortOrder order);

    void on_twDir_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

    void on_btnAdd_clicked();

    void on_btnDel_clicked();

    void on_btnOk_clicked();

    void on_btnSave_clicked();

    void on_btnCancel_clicked();

    void on_cmbMts_currentIndexChanged(int index);

signals:
    void dialogClosed(int witch);

private:
    void initTable();
    void initTree();
    void initDatas(int witch = 1);
    void refresh();
    void refreshFstExt();
    void reCalFstExt();
    void installDataInspect(bool inst = true);

    Ui::SetupBaseDialog2 *ui;
    Account* account;         //账户对象
    bool isInit;              //是否处于对话框构建的初始化阶段
    QTreeWidgetItem* sjtItem; //科目余额节点
    QHash<int,QTreeWidgetItem*> sjtNodes; //一级科目节点，key为科目id

    QHash<int,Double> rates; //期初汇率
    QHash<int,QString> mts;  //币种代码表
    QHash<int,QString> dirs; //借贷方向的文字显示表
    QHash<int,QString> fstClass;    //一级科目类别
    QHash<int,QString> fstSubNames; //一级科目名
    QHash<int,QString> fstSubCodes; //一级科目代码
    QHash<int,int> sidTofids;       //二级科目id到一级科目id的反向映射

    QHeaderView* hv;                //明细科目余额表格的水平表头
    int curSortCol;                 //当前排序列
    Qt::SortOrder curOrder;         //当前排序顺序

    DetExtItemDelegate* dlgt;

    int year,month; //余额所属年、月
    int curSubId; //当前选定的一级科目ID
    QHash<int,Double> fExts, sExts; //存储从数据库中读取的一二级科目的余额值，key为id x 10 + 币种代码
    QHash<int,Double> fRExts, sRExts; //一二级科目外币余额对应的人民币金额，键同上
    QHash<int,int> fExtDirs, sExtDirs; //一二级科目的余额方向
    bool isDirty;

    QHash<int,QList<FstExtData*> > fDatas;  //一级科目余额表，键为一级科目ID，值为余额数据列表
    QHash<int,QList<DetExtData*> > sDatas;  //二级科目余额表，键为一级科目ID，值为余额数据列表
};

////////////////////登录对话框类///////////////////////////////////////////
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();
    User* getLoginUser();

private slots:
    void on_btnLogin_clicked();

    void on_btnCancel_clicked();

private:
    void init();

    //int witch;  //1：登录，2：登出
    Ui::LoginDialog *ui;
};


////////////////////系统安全配置对话框类//////////////////////////////////
class SecConDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SecConDialog(QWidget *parent = 0);
    ~SecConDialog();

private slots:
    void onRightellChanged(int row, int column);

    void on_actAddRight_triggered();

    void on_actDelRight_triggered();

    void on_btnSave_clicked();

    void on_btnClose_clicked();

    void on_actChgGrpRgt_triggered();

    void on_actChgOpeRgt_triggered();

    void on_actChgUserOwner_triggered();



    void on_lwGroup_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void init();
    void initRightTypes(int pcode, QTreeWidgetItem* pitem = NULL);
    QTreeWidgetItem* findItem(QTreeWidget* tree, int code, QTreeWidgetItem* startItem = NULL);
    void saveRights();
    void saveGroups();
    void saveUsers();
    void saveOperates();

    Ui::SecConDialog *ui;
    //QSqlDatabase db;
    //QSqlTableModel* rmodel;

    QIntValidator* vat; //用于验证代码
    //数据修改标记
    bool rightDirty,groupDirty,userDirty,operDirty;
};

//凭证搜索对话框类
class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = 0);
    ~SearchDialog();

private:
    Ui::SearchDialog *ui;
};



#endif // DIALOG2_H
