#ifndef TAXESCOMPARISONFORM_H
#define TAXESCOMPARISONFORM_H


#include "ui_taxesexcelfilecfgform.h"
#include <QWidget>

#ifdef Q_OS_WIN

namespace Ui {
class TaxesComparisonForm;
}

class QSettings;
class Account;
class AccountSuiteManager;

/**
 * @brief 保存导入的发票金额税额数据的表字段
 */
const QString tbl_taxes = "tem_taxes";
const QString fld_tax_class = "incomeOrCost";           //收入还是成本发票
const QString fld_tax_index = "number";                 //序号
const QString fld_tax_invoice_code = "invoiceCode";     //发票代码
const QString fld_tax_invoice_number= "invoiceNumber";  //发票号码
const QString fld_tax_date = "date";                    //开票日期
const QString fld_tax_gfCode = "gfCode";                //购方纳税人识别码
const QString fld_tax_money = "money";                  //发票金额
const QString fld_tax_taxMoney = "taxMoney";            //税额
const QString fld_tax_repeat_tag = "repeatTag";         //作废标记
const QString fld_tax_invoice_type = "invoiceType";     //发票属性（好像是专票或普票之类）
const int FI_TAX_CLASS = 1;
const int FI_TAX_INDEX = 2;
const int FI_TAX_INVOICECODE = 3;
const int FI_TAX_INCOICENUMBER = 4;
const int FI_TAX_DATE = 5;
const int FI_TAX_GFCODE = 6;
const int FI_TAX_MONEY = 7;
const int FI_TAX_TAXMONEY = 8;
const int FI_TAX_REPEATTEG = 9;
const int FI_TAX_INVOICETYPE = 10;

enum InvoiceType{
    IT_COMMON    = 1,   //普通发票
    IT_DEDICATED = 2    //专用发票
};

//显示导入结果的表格列索引
enum ImportTableIndex{
    ITI_TYPE    = 0,            //专票/普票
    ITI_NUMBER  = 1,            //序号
    ITI_INVOICE_CODE    = 2,    //发票代码
    ITI_INVOICE_NUMBER  = 3,    //发票号码
    ITI_DATE            = 4,    //开票日期
    ITI_GFCODE          = 5,    //购方纳税人代码
    ITI_MONEY           = 6,    //发票金额
    ITI_TAX_MONEY       = 7,    //发票税额
    ITI_REPEAL_TAG      = 8     //作废标记
};

//比较结果列表索引
enum ResultTableIndex{
    RTI_NUMBER         = 0, //序号
    RTI_INVOICE_NUMBER = 1, //发票号列（如果无法提取发票号码，则显示原始摘要信息）
    RTI_RESULT         = 2, //比较结果列（比较结果说明，附带结果代码及其正确和错误的税金值）
    RTI_PZ_NUMBER      = 3, //凭证号列（附带凭证id）
    RTI_BA_NUMBER      = 4  //分录序号列（附带分录id）
};

/**
 * @brief 比对结果错误代码
 */
enum CompareResult{
    CR_OK   = 1,
    CR_MONEY_NOTEQUAL = 2,
    CR_DIR_ERROR =3 ,
    CR_NOT_FONDED = 4,
    CR_NOT_DISTINGUISH = 5,
    CR_MONTYTYPE_ERROR = 6
};


class ExcelUtil;

/**
 * @brief 存放税金的excel文件中单元格索引信息的结构
 */
struct TaxExcelCfgInfo{
    int id;             //默认组的id为0
    QString groupName;  //配置组名
    QString showName;   //组的显示名

    //主识别单元格索引
    QString nsrName;    //纳税人名称，即账户所属公司名
    QString nsrTaxCode; //纳税人识别码
    QString startDate, endDate; //开始日期和结束日期

    //必填信息
    int startRow;           //开始行号
    QString colNumber;         //序号列
    QString colInvoiceNumber;  //发票号码列
    QString colDate;           //开票日期列
    QString colMoney;          //发票金额列
    QString colTaxMoney;       //发票税额列

    //可选信息
    QString colInvoiceCode;    //发票代码列
    QString colGfCode;         //购方纳税人识别码列
    QString colRepealTag;      //作废标记列
    QString colInvoiceType;    //发票类型列（专票/普票）
};

/**
 * @brief 保存发票税金的Excel文件的格式配置对话框类
 */
class TaxesExcelFileCfgDlg : public QDialog
{
    Q_OBJECT

public:
    explicit TaxesExcelFileCfgDlg(QWidget *parent = 0);
    ~TaxesExcelFileCfgDlg();

    int getStartRow(){return ui->spnStartRow->value();}
    int getColNumber(){return ui->spnNumber->value();}
    int getColInvoiceCode(){return ui->spnInvoiceCode->value();}
    int getColInvoiceNumber(){return ui->spnInvoiceNumber->value();}
    int getColDate(){return ui->spnDate->value();}
    int getColGfCode(){return ui->spnGfCode->value();}
    int getColMoney(){return ui->spnMoney->value();}
    int getColTaxMoney(){return ui->spnTaxMoney->value();}
    int getColRepealTag(){return ui->spnRepealTag->value();}
    int getColInvoiceType(){return ui->spnType->value();}

private slots:
    void cfgGroupChanged(int index);

    void on_btnSaveAs_clicked();

private:
    void readCfgInfos();
    void saveCfgInfos(TaxExcelCfgInfo* item);
    void viewCfgInfos(TaxExcelCfgInfo* item);

    Ui::TaxesExcelFileCfgForm *ui;
    QSettings* cfgFile;
    QList<TaxExcelCfgInfo*> cfgGroups;

    //配置项的键名及段落名
    const QString defGroupName = "DefaultGroup";
    const QString key_Id = "GroupId";
    const QString key_showname = "ShowName";
    const QString key_nsrName = "nsrName";
    const QString key_TaxCode = "nsrCode";
    const QString key_startdate = "startDate";
    const QString key_enddate = "endDate";
    const QString key_startRow = "startRow";
    const QString key_col_number = "number";
    const QString key_col_invoiceCode = "invoiceCode";
    const QString key_col_invoiceNumber = "invoiceNumber";
    const QString key_col_invoiceType = "invoiceType";
    const QString key_col_date = "invoiceDate";
    const QString key_col_gfCode = "gfTaxCode";
    const QString key_col_money = "invoiceMoney";
    const QString key_col_taxMoney = "invoiceTaxMoney";
    const QString key_col_repeatTag = "repeatTag";
};

/**
 * @brief 执行发票税金导入、显示及比对的窗体类
 */
class TaxesComparisonForm : public QWidget
{
    Q_OBJECT

public:
    explicit TaxesComparisonForm(AccountSuiteManager* asMgr, QWidget *parent = 0);
    ~TaxesComparisonForm();

private slots:
    void sheetChanged(int index);
    void cellDoubleClicked(int row, int column);
    void on_btnBrowseExcel_clicked();

    void on_btnConfig_clicked();

    void on_btnImpIncome_clicked();

    void on_btnSaveIncome_clicked();

    void on_btnSaveCost_clicked();

    void on_btnImpCost_clicked();

    void on_btnClearIncome_clicked();

    void on_btnClearCost_clicked();

    void on_btnExec_clicked();

    void on_actNaviToPz_triggered();

signals:
    void openSpecPz(int pid, int bid);

private:
    void initColumnWidth();
    bool initTable();
    bool clearTable(bool isIncome = true);
    bool readTable();
    bool saveImport(bool isIncome = true);
    bool import(bool isIncome = true);
    QString subInvoiceNumber(QString summary);
    CompareResult compare(QString invoiceNumber, Double taxMoney, Double& correctValue, bool isIncome);
    void transferNumber(QString invoiceNumber, bool isIncome=true);

    //int y,m;
    Account* account;
    AccountSuiteManager* asMgr;
    ExcelUtil* excel;
    Ui::TaxesComparisonForm *ui;
    QStringList sl_i,sl_c,rl_i,rl_c; //sl代表源发票号列表，rl代表结果列表，i代表收入发票，c代表成本发票

    const QString invoice_type_ded =  tr("专用发票");
    const QString invoice_type_com = tr("普通普票");
    const QString repeal_explain = tr("已作废");
    const QString result_explain_ok =  tr("正确");
    const QString result_explain_money_notequal = tr("金额不符");
    const QString result_explain_dir_error = tr("方向错误");
    const QString result_explain_moneytype_error = tr("币种错误");
    const QString result_explain_not_fonded = tr("疑似未认证");
    //const QString result_explain_not_auth = tr("疑似未认证");
    const QString result_explain_not_distinguish = tr("不可识别的发票号");
    const QString result_explain_invoice_repeat = tr("发票号重复使用");
    const QString result_explain_invoice_not_instant = tr("发票未入账");
};
#endif //Q_OS_WIN
#endif // TAXESCOMPARISONFORM_H
