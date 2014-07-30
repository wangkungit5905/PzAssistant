
#include "ExcelUtil.h"
#ifdef Q_OS_WIN
#include <QAxObject>
#include <QFile.h>
#include <QMessageBox>

ExcelUtil::ExcelUtil(/*QString xlsFilePath, */QObject *parent) :QObject(parent)
{
	m_pSheet = 0;
    m_pSheets = 0;
    m_pWorkBook = 0;

	m_pExcel = new QAxObject("Excel.Application", parent);
	m_pWorkBooks = m_pExcel->querySubObject("Workbooks");
    if(!m_pWorkBooks)
        QMessageBox::warning(0,"",tr("无法启动Microsoft Excel应用，请检查是否安装！"));

//	QFile file(xlsFilePath);
//	if (file.exists())
//	{
//		m_pWorkBooks->dynamicCall("Open(const QString&)", xlsFilePath);
//		m_pWorkBook = m_pExcel->querySubObject("ActiveWorkBook");
//		m_pSheets = m_pWorkBook->querySubObject("WorkSheets");
//    }
//    else{
//        m_pWorkBooks->dynamicCall("Add(const QString&)", xlsFilePath);
//        m_pWorkBook = m_pExcel->querySubObject("ActiveWorkBook");
//        m_pSheets = m_pWorkBook->querySubObject("WorkSheets");
//    }
}


ExcelUtil::~ExcelUtil()
{
    close();
}

bool ExcelUtil::open(QString fileName)
{
    if(!m_pWorkBooks)
        return false;
    QFile file(fileName);
    if(file.exists())
    {
        m_pWorkBooks->dynamicCall("Open(const QString&)", fileName);
        m_pWorkBook = m_pExcel->querySubObject("ActiveWorkBook");
        m_pSheets = m_pWorkBook->querySubObject("WorkSheets");
        if(m_pWorkBook && m_pSheets)
            return true;
        else
            return false;
    }
    return false;
}

bool ExcelUtil::createNew(QString fileName)
{
    int c = m_pWorkBooks->property("Count").toInt();
    m_pWorkBooks->dynamicCall("Add()");
    c = m_pWorkBooks->property("Count").toInt();
    //m_pWorkBook = m_pWorkBooks->querySubObject("Item(int)", 1);//好像新建的工作簿都位于队首

    m_pWorkBook = m_pExcel->querySubObject("ActiveWorkBook");//这也能获取到刚创建的工作簿
    if(!m_pWorkBook)
        return false;
    m_pSheets = m_pWorkBook->querySubObject("WorkSheets");
    c = m_pSheets->property("Count").toInt();
    for(int i = c; i > 0; i--)
        deleteSheet(i);
    m_pWorkBook->dynamicCall("SaveAs(const QString&)", fileName);
    return true;
}

QAxObject * ExcelUtil::getWorkBooks(){ return m_pWorkBooks; }
QAxObject * ExcelUtil::getWorkBook(){ return m_pWorkBook; }
QAxObject * ExcelUtil::getWorkSheets(){ return m_pSheets; }
QAxObject * ExcelUtil::getWorkSheet(){ return m_pSheet; }

void ExcelUtil::selectSheet(const QString& sheetName)
{
	m_pSheet = m_pSheets->querySubObject("Item(const QString&)", sheetName);
}
void ExcelUtil::selectSheet(int sheetIndex) //sheetIndex 起始于 1
{
	m_pSheet = m_pSheets->querySubObject("Item(int)", sheetIndex);
}

void ExcelUtil::deleteSheet(const QString& sheetName)
{
	QAxObject *a = m_pSheets->querySubObject("Item(const QString&)", sheetName);
	a->dynamicCall("delete");
}	
void ExcelUtil::deleteSheet(int sheetIndex)
{
	QAxObject *a = m_pSheets->querySubObject("Item(int)", sheetIndex);
	a->dynamicCall("delete");
}
void ExcelUtil::insertSheet(QString sheetName, int index)
{
    //不知道vb形式的“Worksheets.Add Count:=2, Before:=Sheets(1)”原型如何表示？
    m_pSheets->querySubObject("Add()");//默认实现是添加到sheet列表的第一个位置
//    int c = m_pSheets->property("Count").toInt();
//    QString sname;
//    for(int i = 1; i <= c; ++i){
//        selectSheet(i);
//        sname = getSheetName();
//    }
    QAxObject *a = m_pSheets->querySubObject("Item(int)", 1);
    //selectSheet(c);
    //sname = getSheetName();
	a->setProperty("Name", sheetName);
}
int ExcelUtil::getSheetsCount()
{
	return m_pSheets->property("Count").toInt();
}
QString ExcelUtil::getSheetName()//在 selectSheet() 之后才可调用
{
	return m_pSheet->property("Name").toString();
}
QString ExcelUtil::getSheetName(int sheetIndex)
{
	QAxObject *a = m_pSheets->querySubObject("Item(int)", sheetIndex);
	return a->property("Name").toString();
}

void ExcelUtil::setCellString(int row, int column, const QString& value)
{
    if(!m_pSheet)
        return;
	QAxObject *range = m_pSheet->querySubObject("Cells(int,int)", row, column);
	range->dynamicCall("SetValue(const QString&)", value);
}
void ExcelUtil::setCellString(const QString& cell, const QString& value)//cell 例如 "A7"	
{
	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range->dynamicCall("SetValue(const QString&)", value);
}
void ExcelUtil::mergeCells(const QString& cells)//range 例如 "A5:C7"
{
	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cells);
	range->setProperty("VerticalAlignment", -4108);//xlCenter
	range->setProperty("WrapText", true);
	range->setProperty("MergeCells", true);
}
void ExcelUtil::mergeCells(int topLeftRow, int topLeftColumn, int bottomRightRow, int bottomRightColumn)
{
	QString cell;
	cell.append(QChar(topLeftColumn - 1 + 'A'));
	cell.append(QString::number(topLeftRow));
	cell.append(":");
	cell.append(QChar(bottomRightColumn - 1 + 'A'));
	cell.append(QString::number(bottomRightRow));

	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range->setProperty("VerticalAlignment", -4108);//xlCenter
	range->setProperty("WrapText", true);
	range->setProperty("MergeCells", true);
}
QVariant ExcelUtil::getCellValue(const QString& cell)
{
	//QAxObject *range = m_pSheet->querySubObject("Cells(int,int)", row, column);
	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	return range->property("Value");
}
QVariant ExcelUtil::getCellValue(int row, int column)
{
	QAxObject *range = m_pSheet->querySubObject("Cells(int,int)", row, column);
	return range->property("Value");
}
void ExcelUtil::clearCell(int row, int column)
{
	QString cell;
	cell.append(QChar(column - 1 + 'A'));
	cell.append(QString::number(row));

	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range->dynamicCall("ClearContents()");
}
void ExcelUtil::clearCell(const QString& cell)
{
	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range->dynamicCall("ClearContents()");
}


void ExcelUtil::getUsedRange(int *topLeftRow, int *topLeftColumn, int *bottomRightRow, int *bottomRightColumn)
{
	QAxObject *usedRange = m_pSheet->querySubObject("UsedRange");
	*topLeftRow = usedRange->property("Row").toInt();
	*topLeftColumn = usedRange->property("Column").toInt();

	QAxObject *rows = usedRange->querySubObject("Rows");
	*bottomRightRow = *topLeftRow + rows->property("Count").toInt() - 1;

	QAxObject *columns = usedRange->querySubObject("Columns");
	*bottomRightColumn = *topLeftColumn + columns->property("Count").toInt() - 1;
}
void ExcelUtil::setColumnWidth(int column, int width)
{
	QString columnName;
	columnName.append(QChar(column - 1 + 'A'));
	columnName.append(":");
	columnName.append(QChar(column - 1 + 'A'));

	QAxObject * col = m_pSheet->querySubObject("Columns(const QString&)", columnName);
	col->setProperty("ColumnWidth", width);
}
void ExcelUtil::setRowHeight(int row, int height)
{
	QString rowsName;
	rowsName.append(QString::number(row));
	rowsName.append(":");
	rowsName.append(QString::number(row));

	QAxObject * r = m_pSheet->querySubObject("Rows(const QString &)", rowsName);
	r->setProperty("RowHeight", height);
}
void ExcelUtil::setCellTextCenter(int row, int column)
{
	QString cell;
	cell.append(QChar(column - 1 + 'A'));
	cell.append(QString::number(row));

	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range->setProperty("HorizontalAlignment", -4108);//xlCenter
}
void ExcelUtil::setCellTextCenter(const QString& cell)
{
	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range->setProperty("HorizontalAlignment", -4108);//xlCenter
}
void ExcelUtil::setCellTextWrap(int row, int column, bool isWrap)
{
	QString cell;
	cell.append(QChar(column - 1 + 'A'));
	cell.append(QString::number(row));

	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range->setProperty("WrapText", isWrap);
}
void ExcelUtil::setCellTextWrap(const QString& cell, bool isWrap)
{
	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range->setProperty("WrapText", isWrap);
}
void ExcelUtil::setAutoFitRow(int row)
{
	QString rowsName;
	rowsName.append(QString::number(row));
	rowsName.append(":");
	rowsName.append(QString::number(row));

	QAxObject * rows = m_pSheet->querySubObject("Rows(const QString &)", rowsName);
	rows->dynamicCall("AutoFit()");
}
void ExcelUtil::mergeSerialSameCellsInAColumn(int column, int topRow)
{
	int a, b, c, rowsCount;
	getUsedRange(&a, &b, &rowsCount, &c);

	int aMergeStart = topRow, aMergeEnd = topRow + 1;

	QString value;
	while (aMergeEnd <= rowsCount)
	{
		value = getCellValue(aMergeStart, column).toString();
		while (value == getCellValue(aMergeEnd, column).toString())
		{
			clearCell(aMergeEnd, column);
			aMergeEnd++;
		}
		aMergeEnd--;
		mergeCells(aMergeStart, column, aMergeEnd, column);

		aMergeStart = aMergeEnd + 1;
		aMergeEnd = aMergeStart + 1;
	}
}
int ExcelUtil::getUsedRowsCount()
{
	QAxObject *usedRange = m_pSheet->querySubObject("UsedRange");
	int topRow = usedRange->property("Row").toInt();
	QAxObject *rows = usedRange->querySubObject("Rows");
	int bottomRow = topRow + rows->property("Count").toInt() - 1;
	return bottomRow;
}
void ExcelUtil::setCellFontBold(int row, int column, bool isBold)
{
	QString cell;
	cell.append(QChar(column - 1 + 'A'));
	cell.append(QString::number(row));

	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range = range->querySubObject("Font");
	range->setProperty("Bold", isBold);
}
void ExcelUtil::setCellFontBold(const QString& cell, bool isBold)
{
	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range = range->querySubObject("Font");
	range->setProperty("Bold", isBold);
}
void ExcelUtil::setCellFontSize(int row, int column, int size)
{
	QString cell;
	cell.append(QChar(column - 1 + 'A'));
	cell.append(QString::number(row));

	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range = range->querySubObject("Font");
	range->setProperty("Size", size);
}
void ExcelUtil::setCellFontSize(const QString& cell, int size)
{
	QAxObject *range = m_pSheet->querySubObject("Range(const QString&)", cell);
	range = range->querySubObject("Font");
	range->setProperty("Size", size);
}

void ExcelUtil::save()
{
	m_pWorkBook->dynamicCall("Save()");
}
void ExcelUtil::close()
{
	m_pExcel->dynamicCall("Quit()");
	delete m_pSheet;
	delete m_pSheets;
	delete m_pWorkBook;
	delete m_pWorkBooks;
	delete m_pExcel;

	m_pExcel = 0;
	m_pWorkBooks = 0;
	m_pWorkBook = 0;
	m_pSheets = 0;
	m_pSheet = 0;
}

#endif
