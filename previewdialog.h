#ifndef PREVIEWDIALOG_H
#define PREVIEWDIALOG_H

#include <QDialog>
#include <QtGui/QTableView>
#include <QtGui/QPrinter>
#include <QtGui/QTextLength>
#include <QtGui/QFileDialog>
#include <QtGui/QGraphicsView>
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
    //数据源使用了复合代理模型（ProxyModelWithHeaderModels），此复合模型内，包含了表示表头
    //的数据模型和表格内容的数据模型



    explicit PreviewDialog(PrintTemplateBase* templateWidget, PrintPageType pageType,
                           QPrinter* printer, bool outPaging = false, QWidget *parent = 0);
    ~PreviewDialog();
    //void setPaging(PagingFun pagingFun);
    //void setPagedData(QStandardItemModel* model);
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
    void reqPaging(int rowsInTable,int& pageNum);
    //请求指定打印页的表格数据，和各列的宽度（回应此信号的槽，还必须设置好模型列的标题内容）
    //参数 pageNUm：页号，colWidths：列宽，pdModel：表格数据，hModel：表头数据
    void reqPageData(int pageNum, QList<int>*& colWidths, QStandardItemModel& pdModel,
                     QStandardItemModel& hModel);
    //预先分页信号（用以使视图可以设置打印的页面范围
    //参数out：false:表示是由预览框本身负责分页，true：由外部负责视图分页
    //pages：当out为真时，表示每页最多有几行，为假时，表示预览框分页后的结果
    //void priorPaging(bool out, int pages);

private:
    void setupPage();
    void paintPage(int pagenum);
    void setupSpinBox();

    Ui::PreviewDialog *ui;

    //PagingFun paging;  //计算分页数的函数指针

    qreal leftMargin;                  //左边留白
    qreal rightMargin;                 //右边留白
    qreal topMargin;                   //顶部留白
    qreal bottomMargin;                //底部留白
    int rowPerPage;                  //每页要打印到表格行数
    //int curRow;                      //当前打印的表格开始行号
    int pages;                       //需要打印的页数
    //QList<int> hideCols;             //表格中需要隐藏的列

    int sceneZoomFactor;             //场景缩放因子

    //int defRowHeight;                //行高，这个高度是用来确定可打印的行数的一个预定值
    int rowHeight;
    QFont headerFont;                //表头（标题行）字体
    QFontMetrics *headerFmt;
    QFont font;                      //表格文本字体
    QFontMetrics *fmt;               //度量表格文本字体需占据的空间大小


    //QGraphicsView *view;
    PrintPageType pageType;               //要打印的页面类型
    QTableView *tv;                  //位于view中的表格视图，显示每页的表格数据
    //PrintTemplateDz* ptd;
    //PrintTemplateTz* ptt;
    //执行本期统计打印模板类的指针
    //指向通用打印模板类的指针
    QGraphicsProxyWidget* proWidget; //包装打印模板的代理部件
    //QWidget* tWidget;         //打印模板类
    PrintTemplateBase* tWidget;         //打印模板类

    QPrinter *printer;
    QGraphicsScene pageScene;

    QStandardItemModel *dataModel;       //用于保存所有分页前的表格数据
    QAbstractItemModel* headerModel;     //表头数据模型（自理分页用）
    //QStandardItemModel* headerModel;     //表头数据模型
    QStandardItemModel* pageModel;       //用于保存每页中的表格数据的模型（自理分页用）

    QStandardItemModel oHeaderModel;      //表头数据模型（外部分页用）
    QStandardItemModel oPageModel;       //用于保存每页中的表格数据的模型（外部分页用）

    ProxyModelWithHeaderModels* pageProxyModel;  //每页表格的代理模型
    QStringList headDatas;   //当打印通用表格时，可以用来保存表头数据

    qreal oldw,oldh;

    QGraphicsAnchorLayout *l;
    QGraphicsWidget *w;
    Qt::GlobalColor sceneBgColor;
    QGraphicsRectItem* pageBack;  //页面的背景矩形
    bool isPreview;    //当前打印任务是否是预览
    bool outPaging;    //是否由外部进行分页处理
    //QList<int> bounds; //分页的边界索引（以页号-1作为列表的索引值，值表示打印在当前页最后一行数据在dataModel中的索引号）
};

#endif // PREVIEWDIALOG_H
