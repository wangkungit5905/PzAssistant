#include <QtGlobal>
#ifdef Q_OS_WIN

#include "excelUtils.h"


//////////////////////////BasicExcel///////////////////////////
BasicExcel::BasicExcel(QObject* parent)
{
    excel = new QAxObject("Excel.Application", parent); //获取Excel对象
    if(!excel->isNull())
        workbooks = excel->querySubObject("Workbooks");//得到Workbooks集合的指针

}

//创建一个新的具有给定数目电子表格的工作簿workbook(至少是1)
void BasicExcel::New(int sheetNum)
{
    if(!workbooks->isNull()){
        workbook = workbooks->querySubObject("Add()");
        sheets = workbook->querySubObject("Sheets");
        //sheets->querySubObject("Delete()"); //删除默认创建的3个空工作簿，无法删除
        int num = sheets->dynamicCall("Count()").toInt();
        if(sheetNum > num){
            for(int i = 0; i < sheetNum - num; ++i)
                sheets->querySubObject("Add()");
        }

    }


}

bool BasicExcel::SaveAs(QString filename/*, bool isViewDlg*/)
{
    QString fname;
    if(!workbook->isNull()){
//        if(isViewDlg){//需要由Excel来打开另存为对话框，但并不会打开对话框，郁闷！
//            fname = excel->dynamicCall("GetSaveAsFilename(QString,QString)",
//                                       filename,"*.xls").toString();
//        }
//        else
            fname = filename;
        workbook->dynamicCall("SaveAs (const QString&)", fname);
    }
}


//打开指定Excel文件
bool BasicExcel::Load(QString fname)
{
    workbook = workbooks->querySubObject("Open(const QString&)", fname);
    sheets = workbook->querySubObject("Sheets");
}

///得到指定索引位置的Excel worksheet指针，索引基于0.如果索引无效返回0.
BasicExcelWorksheet* BasicExcel::GetWorksheet(int sheetIndex)
{
    if(!sheets->isNull()){
        QAxObject *sheet = sheets->querySubObject("Item(const QVariant&)", sheetIndex);
        return new BasicExcelWorksheet(sheet);
    }
}

//获取指定名称的Excel worksheet，如果名称无效返回0
BasicExcelWorksheet* BasicExcel::GetWorksheet(QString name)
{
    if(!sheets->isNull()){
        QAxObject *sheet = sheets->querySubObject("Item(const QVariant&)", name);
        return new BasicExcelWorksheet(sheet);
    }
}


//设置是否可见
void BasicExcel::setVisible(bool isView)
{
    //excel->dynamicCall("SetVisible(bool)", isView);
    excel->setProperty("Visible", isView);
}


////////////////////////BasicExcelWorksheet//////////////////////////////////





//////////////////////BasicExcelWorksheet////////////////////////////////////////
BasicExcelWorksheet::BasicExcelWorksheet(QObject* parent)
{

}

BasicExcelWorksheet::BasicExcelWorksheet(QAxObject* axObj, QObject* parent)
{
    obj = axObj;
}

void BasicExcelWorksheet::active()
{
    obj->dynamicCall("Activate()");
}


//获取单元格，行列基于0
BasicExcelCell* BasicExcelWorksheet::cell(int row, int col)
{
    row++;
    col++;
    QAxObject* cell = obj->querySubObject("Cells(int, int)", row, col);
    if(!cell->isNull())
        return new BasicExcelCell(cell);
    else
        return NULL;
}

//将整数的列索引转换为字母形式（目前假定两位字母序列足以）
QString BasicExcelWorksheet::genColWord(int col)
{
    char c = 'A' - 1;
    if(col <= 26)
        return QString(c + col);
    else{
        char c1 = c + col / 26; //十位上的字母
        char c0 = c + col % 26; //个位上的字母
        return QString(c1).append(c0);
    }
}

//合并指定区域的单元格，行列基于0
void BasicExcelWorksheet::mergeCells(int row, int col, ushort rowRange, ushort colRange)
{
    //QAxObject* topLeft = obj->querySubObject("Cells(int, int)", row, col);
    //QAxObject* btnRight = obj->querySubObject("Cells(int, int)", row + rowRange, col + colRange);
    //本想利用“Range(cell1, cell2)”方法来创建区域，但不知如何写函数原型

    row++;
    col++;
    QString strRange = genColWord(col);
    strRange.append(QString::number(row)).append(":");
    strRange.append(genColWord(col+colRange - 1))
            .append(QString::number(row + rowRange - 1));

    QAxObject* range = obj->querySubObject("Range(QString)", strRange);
    range->dynamicCall("Merge()");
}

////////////////////BasicExcelCell////////////////////////////
BasicExcelCell::BasicExcelCell(QAxObject* axObj, QObject* parent)
{
    obj = axObj;
}

int BasicExcelCell::getInt()
{
    return obj->property("Value").toInt();
}

double BasicExcelCell::getDouble()
{
    return obj->dynamicCall("Value()").toDouble();
}

QString BasicExcelCell::getString()
{
    return obj->property("Value").toString();
}

void BasicExcelCell::Set(int val)
{
    obj->setProperty("Value", val);
}

void BasicExcelCell::Set(double val)
{
    obj->setProperty("Value", val);
}

void BasicExcelCell::Set(QString val)
{
    obj->setProperty("Value", val);
}

#endif
