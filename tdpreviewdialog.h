/*
 * Table View Print & Preview dialog
 * Copyright (C) 2004-2008 by Gordos Kund / QnD Co Bt.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact gordos.kund@gmail.com with any questions on this license.
 */

#ifndef TDPREVIEWDIALOG_H
#define TDPREVIEWDIALOG_H
    #include <QDialog>
    #include <ui_tdpreviewdialog.h>
    #include <QTableView> //checked
    #include <QPrinter> //checked
    #include <QTextLength> //checked
    #include <QFileDialog> //checked
    #include <QGraphicsView> //checked

/*! \class TDPreviewDialog
 *  \brief TDPreviewDialog dialog
 *  \author Kund Gordos
 *  \version 0.12
 *  \date     2008
 */

class QGraphicsScene;
class QAbstractItemModel;

// Text preview widget
class TDPreviewDialog : public QDialog
{
        Q_OBJECT
	Q_ENUMS (Grids)
public:
	enum Grids {
		NoGrid=0x0,
		NormalGrid=0x1,
		AlternateColor=0x2,
		AlternateWithGrid=0x3
	};

	TDPreviewDialog(QTableView *p_tableView, QPrinter * p_printer,  QWidget *parent=0);
        virtual ~TDPreviewDialog();
	virtual void setHeaderText(const QString &text);
        virtual void setHeaderStdText(const QString &text);
	virtual void setGridMode(Grids);
	virtual void print();
	virtual int exec();
	virtual void exportPdf(const QString &filename);

private slots:
	virtual void on_setupToolButton_clicked();
	virtual void on_zoomInToolButton_clicked();
	virtual void on_zoomOutToolButton_clicked();
	virtual void on_pageSpinBox_valueChanged(int value);

private:
        Ui_TDPreviewDialog ui;
	virtual void setupPage();
	virtual void paintPage(int pagenum);
	virtual void setupSpinBox();
	QGraphicsView *view;
        QTableView *tableView;  //外部需要打印的表格视图
	QPrinter *printer;
        TDPreviewDialog::Grids gridMode; //表格的网格线模式
        int lines;                       //表格行数（包括）
        int pages;                       //需要打印的页数
        int leftMargin;                  //左边留白
        int rightMargin;                 //右边留白
        int topMargin;                   //顶部留白
        int bottomMargin;                //底部留白
        int spacing;                     //表格行间距？
        int headerSize;                  //页标题高度（表示表格名的文本区域-在表格标题行之上）
        int footerSize;                  //页脚注高度
        int sceneZoomFactor;             //
        double columnZoomFactor;         //表格列缩放因子
        double rowHeight;                //表格行高
        double columnMultiplier;         //
        QString headerText;              //页标题文本
        QString footerText;              //页脚注文本
	QVector<QTextLength> colSizes;
        QAbstractItemModel *model;       //提取表格数据的模型
	QGraphicsScene pageScene;
        QFont titleFont;                 //页标题文本的字体
        QFont headerFont;                //表头（标题行）字体
        QFont font;                      //表格文本字体
        QFontMetrics *titleFmt;          //用于计算标题文本需占据的空间大小
	QFontMetrics *headerFmt;
        QFontMetrics *fmt;               //度量表格文本字体需占据的空间大小
        QString headerStdText;           //处于表格左上角上的文本（比如用于显示表格数据的时间区域信息等）
};

#endif
