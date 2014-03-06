#ifndef PREVIEWDIALOG_H
#define PREVIEWDIALOG_H

#include <QDialog>
#include <QTableView>
#include <QPrinter>
#include <QTextLength>
#include <QFileDialog>
#include <QGraphicsView>
#include <QLabel>
#include <QStandardItemModel>
#include <QGraphicsAnchorLayout>
#include <QGraphicsRectItem>

#include "printtemplate.h"

class QGraphicsScene;
class QAbstractItemModel;

namespace Ui {
    class PreviewDialog;
}

//typedef int (*PagingFun)(int rowsInTable);

class PreviewDialog : public QDialog
{
    Q_OBJECT

public:
    //本类支持打印页面中包含一个表格的文档，页面的布局由打印模板类定义，表格数据通过模板视图中
    //的表格视图对象（QTableView）提供。根据页面类型的不同，该表格视图可能使用不同的数据模型
    //和表头结构。对于COMMON类型，其表头使用标准表头（QHeadView），数据使用标准的模型
    //（QStandardItemModel），除此之外的页面类型，使用了层次式表头（HierarchicalHeaderView）
    //数据源使用了复合代理模型（MyWithHeaderModels），此复合模型内，包含了表示表头
    //的数据模型和表格内容的数据模型

    explicit PreviewDialog(PrintTemplateBase* templateWidget, PrintPageType pageType,
                           QPrinter* printer, bool outPaging = false, QWidget *parent = 0);
    ~PreviewDialog();
    virtual void print();
    virtual int exec();
    virtual void exportPdf(const QString &filename);

private slots:
    void colWidthResized(int logicalIndex, int oldSize, int newSize);
    void on_spnPage_valueChanged(int arg1);

    void on_btnZoomIn_clicked();

    void on_btnZoomOut_clicked();

    void on_btnSet_clicked();

signals:
    /**
     * @请求分页信号
     * @param rowsInTable   每页的表格可容纳的行数
     * @param pageNum       打印完所有数据所需要的页数
     */
    void reqPaging(int rowsInTable,int& pageNum);

    /**
     * @brief 请求指定打印页的表格数据，和各列的宽度（回应此信号的槽，还必须设置好模型列的标题内容）
     * @param pageNum   页号
     * @param colWidths 列宽
     * @param pdModel   表格数据模型（附带表头数据模型）
     */
    void reqPageData(int pageNum, QList<int>*& colWidths, MyWithHeaderModels* pdModel);



private:
    void setupPage();
    void paintPage(int pagenum);
    void setupSpinBox();

    Ui::PreviewDialog *ui;
    qreal leftMargin;                  //左边留白
    qreal rightMargin;                 //右边留白
    qreal topMargin;                   //顶部留白
    qreal bottomMargin;                //底部留白
    int rowPerPage;                  //每页要打印到表格行数
    int pages;                       //需要打印的页数
    int sceneZoomFactor;             //场景缩放因子
    int rowHeight;
    QFont headerFont;                   //表头（标题行）字体
    QFontMetrics *headerFmt;
    QFont font;                         //表格文本字体
    QFontMetrics *fmt;                  //度量表格文本字体需占据的空间大小

    PrintPageType pageType;             //要打印的页面类型
    QTableView *tv;                     //位于view中的表格视图，显示每页的表格数据
    QGraphicsProxyWidget* proWidget;    //包装打印模板的代理部件
    PrintTemplateBase* tWidget;         //打印模板类

    QPrinter *printer;
    QGraphicsScene pageScene;

    QStandardItemModel* dataModel;      //用于保存所有分页前的原始表格数据
    QAbstractItemModel* headerModel;    //表头数据模型（自理分页用）
    MyWithHeaderModels* pageModel;      //用于保存每个打印页中表格的数据模型

    QStringList headDatas;              //当打印通用表格时，可以用来保存表头数据

    qreal oldw,oldh;

    QGraphicsAnchorLayout *l;
    QGraphicsWidget *w;
    Qt::GlobalColor sceneBgColor;
    QGraphicsRectItem* pageBack;  //页面的背景矩形
    bool isPreview;    //当前打印任务是否是预览
    bool outPaging;    //是否由外部进行分页处理
    QList<int>* colWidths;
};

#endif // PREVIEWDIALOG_H
