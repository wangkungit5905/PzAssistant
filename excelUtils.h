#ifndef EXCELUTILS_H
#define EXCELUTILS_H


#include <QtGlobal>

#ifdef Q_OS_WIN

#include <QString>
#include <QAxObject>

class BasicExcelCell{
public:
    BasicExcelCell(QObject* parent = 0){};
    BasicExcelCell(QAxObject* axObj, QObject* parent = 0);

    int getInt();
    double getDouble();
    QString getString();

    void Set(int val);
    void Set(double val);
    void Set(QString val);

private:
    QAxObject* obj;
};


class BasicExcelWorksheet{
public:
    BasicExcelWorksheet(QObject* parent = 0);
    BasicExcelWorksheet(QAxObject* axObj, QObject* parent = 0);

    void active();
    BasicExcelCell* cell(int row, int col);

    void mergeCells(int row, int col, ushort rowRange, ushort colRange);

private:
    QString genColWord(int col);

    QAxObject* obj;

};

//利用QActive模块读写Excel文件的实用类
class BasicExcel{
public:
    BasicExcel(QObject* parent = 0);

    void New(int sheetNum);
    bool SaveAs(QString filename/*, bool isViewDlg = false*/);
    bool Load(QString fname);

    BasicExcelWorksheet* GetWorksheet(int sheetIndex);	///< Get a pointer to an Excel worksheet at the given index. Index starts from 0. Returns 0 if index is invalid.
    BasicExcelWorksheet* GetWorksheet(QString name);	///< Get a pointer to an Excel worksheet that has given ANSI name. Returns 0 if there is no Excel worksheet with the given name.

    void setVisible(bool isView);

private:
    QAxObject* excel;      //Excel应用程序对象
    QAxObject *workbooks;  //工作簿集合Workbooks对象
    QAxObject *workbook;   //工作簿Workbook对象
    QAxObject *sheets;     //工作表集合sheets对象
};





#endif //Q_OS_WIN32
#endif // EXCELUTILS_H
