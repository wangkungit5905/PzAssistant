#pragma once
#include <QtGlobal>
#ifdef Q_OS_WIN
#include "qobject.h"

class QAxObject;

class ExcelUtil:public QObject
{
	Q_OBJECT
public:
    ExcelUtil(/*QString xlsFilePath, */QObject *parent = 0);
	virtual ~ExcelUtil();

    bool open(QString fileName);
    bool createNew(QString fileName);

	QAxObject *getWorkBooks();
	QAxObject *getWorkBook();
	QAxObject *getWorkSheets();
	QAxObject *getWorkSheet();

	/**************************************************************************/
	/* 工作表                                                                 */
	/**************************************************************************/
	void selectSheet(const QString& sheetName);	
	void selectSheet(int sheetIndex);//sheetIndex 起始于 1
	void deleteSheet(const QString& sheetName);
	void deleteSheet(int sheetIndex);
	void insertSheet(QString sheetName, int index=1);
	int getSheetsCount();	
	QString getSheetName();//在 selectSheet() 之后才可调用
	QString getSheetName(int sheetIndex);

	/**************************************************************************/
	/* 单元格                                                                 */
	/**************************************************************************/
	void setCellString(int row, int column, const QString& value);	
	void setCellString(const QString& cell, const QString& value);//cell 例如 "A7"	
	void mergeCells(const QString& cells);//range 例如 "A5:C7"
	void mergeCells(int topLeftRow, int topLeftColumn, int bottomRightRow, int bottomRightColumn);
	QVariant getCellValue(int row, int column);
	QVariant getCellValue(const QString& cell);
	void clearCell(int row, int column);
	void clearCell(const QString& cell);

	/**************************************************************************/
	/* 布局格式                                                               */
	/**************************************************************************/
	void getUsedRange(int *topLeftRow, int *topLeftColumn, int *bottomRightRow, int *bottomRightColumn);
	void setColumnWidth(int column, int width);
	void setRowHeight(int row, int height);
	void setCellTextCenter(int row, int column);
	void setCellTextCenter(const QString& cell);
	void setCellTextWrap(int row, int column, bool isWrap);
	void setCellTextWrap(const QString& cell, bool isWrap);
	void setAutoFitRow(int row);
	void mergeSerialSameCellsInAColumn(int column, int topRow);
	int getUsedRowsCount();
	void setCellFontBold(int row, int column, bool isBold);
	void setCellFontBold(const QString& cell, bool isBold);
	void setCellFontSize(int row, int column, int size);
	void setCellFontSize(const QString& cell, int size);

	/**************************************************************************/
	/* 文件                                                                   */
	/**************************************************************************/
	void save();
	void close();

private:
	QAxObject *m_pExcel;
	QAxObject *m_pWorkBooks;
	QAxObject *m_pWorkBook;
	QAxObject *m_pSheets;
	QAxObject *m_pSheet;
};

#endif
