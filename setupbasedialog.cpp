#include <QSqlQuery>
#include <QMessageBox>
#include <QCheckBox>
#include <QSqlRecord>
#include <QToolButton>
//#include <QMouseEvent>
#include <QTableView>
#include <QDebug>

#include "setupbasedialog.h"
#include "ui_setupbasedialog.h"
#include "global.h"
#include "common.h"
#include "dialogs.h"
#include "tables.h"


/////////////////////////////////////////////////////////////////////

SetupBaseDialog::SetupBaseDialog(bool isWizard, QWidget *parent) : QDialog(parent),
    ui(new Ui::SetupBaseDialog)
{
    ui->setupUi(this);
    this->isWizard = isWizard;

    if(isWizard){
        ui->btnEnd->setText(tr("关闭"));
        ui->lblSetp->setVisible(false);
    }

    extraModel = new QSqlTableModel;
    extraMapper = new QDataWidgetMapper;

    ui->datExtra->setDate(QDate::currentDate());
    ui->datAsset->setDate(QDate::currentDate());
    ui->datProfit->setDate(QDate::currentDate());

    crtExtraPage();

    //创建输入资产负债表的页面
    crtReportPage(RPT_BALANCE, curAccount->getReportType());
    //创建输入利润表数据的页面
    crtReportPage(RPT_PROFIT, curAccount->getReportType());




    //创建添加利润表数据的页面


    //创建添加资产负债表数据的页面

}

SetupBaseDialog::~SetupBaseDialog()
{
    delete ui;
}

void SetupBaseDialog::switchPage()
{
    if(ui->rdoExtra->isChecked())
        ui->stackedWidget->setCurrentIndex(0);
    else if(ui->rdoProfit->isChecked())
        ui->stackedWidget->setCurrentIndex(1);
    else
        ui->stackedWidget->setCurrentIndex(2);
}

//是否输入报表的年度余额数
void SetupBaseDialog::checkBoxToggled (bool checked)
{
     QCheckBox* send = qobject_cast<QCheckBox*>(sender());
     if(send == ui->chkProfit){
         if(checked)
             ui->datProfit->setDisplayFormat("yyyy");
         else
             ui->datProfit->setDisplayFormat("yyyy-M");
     }
     else{
         if(checked)
             ui->datAsset->setDisplayFormat("yyyy");
         else
             ui->datAsset->setDisplayFormat("yyyy-M");
     }

}

//创建显示和编辑科目余额的tab页
void SetupBaseDialog::crtExtraPage()
{
    QSqlQuery q;
    QString name,code, oname;
    QString s;
    bool r;

    int pt = 1;  //因为查询必须严格按科目类别的顺序，当此二者不同时，
    int t;       //程序知道遇到了新的科目类别，必须重置row和col变量
    int row = 0;
    int col = 0;

    //根据当前使用的科目系统的类别初始化布局容器映射表
    s = "select code, name from FstSubClasses";
    r = q.exec(s);
    if(r){
        int c = 0;
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            ui->tabWidget->setTabText(c, name);
            QGridLayout* lyt = new QGridLayout;
            lytHash[code] = lyt;
            ui->tabWidget->widget(c++)->setLayout(lyt);
        }
        if(c == 5)  //界面上预设了6个页面，但老科目系统只需要5个
            ui->tabWidget->removeTab(c);
    }

    //初始化显示余额值的模型及其映射器
    extraModel->setTable("SubjectExtras");
    int year = ui->datExtra->date().year();
    int month = ui->datExtra->date().month();
    QString fltStr = QString("(year = %1) and (month = %2)")
                     .arg(year).arg(month);
    extraModel->setFilter(fltStr);
    extraMapper->setModel(extraModel);

    extraModel->select();

    //获取SubjectExtras的字段信息
    s = "select * from SubjectExtras";
    r = q.exec(s);
    QSqlRecord rec = q.record();

    //创建显示科目余额的部件
    s = QString("select subCode, subName,belongTo,isReqDet from "
                "FirSubjects where isView = 1 order by subCode");
    bool result = q.exec(s);
    if(result){
        while(q.next()){
            QString oname;
            name = q.value(1).toString();  //科目名称
            code = q.value(0).toString();  //科目代码
            t = q.value(2).toInt();        //科目类别
            bool isReqDet = q.value(3).toBool(); //是否需要子目的余额支持
            QChar strCode;   //用大写字符（A-F）表示的科目类别代码
            switch(t){
            case 1:
                strCode = 'A';
                break;
            case 2:
                strCode = 'B';
                break;
            case 3:
                strCode = 'C';
                break;
            case 4:
                strCode = 'D';
                break;
            case 5:
                strCode = 'E';
                break;
            case 6:
                strCode = 'F';
                break;
            }

            if(pt != t){
                row = 0;
                col = 0;
                pt = t;
            }
            //QLabel* label = new QLabel(name);
            QLabel* label;
            QLineEdit* edt = new QLineEdit;
            oname = QString("%1%2").arg(strCode).arg(code);  //对象名（类别代码字母+科目代码）
            edt->setObjectName(oname);
            //建立映射
            extraMapper->addMapping(edt,rec.indexOf(oname));
            if(isReqDet){                
                label = new ClickAbleLabel(name,code);
                ClickAbleLabel* lp = (ClickAbleLabel*)label;
                connect(lp, SIGNAL(viewSubExtra(QString,QPoint))
                        , this, SLOT(viewSubExtra(QString,QPoint)));
            }
            else{
                label = new QLabel(name);
            }
            lytHash[t]->addWidget(label, row/3, col++ % 6);
            lytHash[t]->addWidget(edt, row/3, col++ % 6);
            edtHash[oname] = edt;

//            switch(t){
//            case 1:
//                oname = QString("A%1").arg(code);  //对象名（类别代码字母+科目代码）
//                edt->setObjectName(oname);
//                //ui->gl1->addWidget(label, row/3, col++ % 6);
//                //ui->gl1->addWidget(edt, row/3, col++ % 6);
//                lytHash[t]->addWidget(label, row/3, col++ % 6);
//                lytHash[t]->addWidget(edt, row/3, col++ % 6);
//                edtHash[oname] = edt;
//                break;
//            case 2:
//                oname = QString("B%1").arg(code);
//                edt->setObjectName(oname);
//                ui->gl2->addWidget(label, row/3, col++ % 6);
//                ui->gl2->addWidget(edt, row/3, col++ % 6);
//                edtHash[oname] = edt;
//                break;
//            case 3:
//                oname = QString("C%1").arg(code);
//                edt->setObjectName(oname);
//                ui->gl3->addWidget(label, row/3, col++ % 6);
//                ui->gl3->addWidget(edt, row/3, col++ % 6);
//                edtHash[oname] = edt;
//                break;
//            case 4:
//                oname = QString("D%1").arg(code);
//                edt->setObjectName(oname);
//                ui->gl4->addWidget(label, row/3, col++ % 6);
//                ui->gl4->addWidget(edt, row/3, col++ % 6);
//                edtHash[oname] = edt;
//                break;
//            case 5:
//                oname = QString("E%1").arg(code);
//                edt->setObjectName(oname);
//                ui->gl5->addWidget(label, row/3, col++ % 6);
//                ui->gl5->addWidget(edt, row/3, col++ % 6);
//                edtHash[oname] = edt;
//                break;
//            case 6:
//                oname = QString("F%1").arg(code);
//                edt->setObjectName(oname);
//                ui->gl6->addWidget(label, row/3, col++ % 6);
//                ui->gl6->addWidget(edt, row/3, col++ % 6);
//                edtHash[oname] = edt;
//                break;
//            }
            row++;
        }
    }
    extraMapper->toFirst();
}


//创建指定类别指定类型的报表数据输入界面
//尚未考虑的细节是对非数值型报表字段的处理（应使它们不可编辑，还有报表标题部分）
void SetupBaseDialog::crtReportPage(int clsId, int  typeId)
{
    //从ReportStructs表中读取报表结构信息
    QSqlQuery q;
    QString s = QString("select rowNum, fname, ftitle from ReportStructs where  "
                        "(clsid = %1) and (tid = %2) order by viewOrder")
            .arg(clsId).arg(typeId);
    bool r = q.exec(s);
    if(r){
        QStandardItemModel* model;
        switch (clsId){
        case 1:  //资产负债表
            mAsset = new QStandardItemModel;
            ui->tabAsset->setModel(mAsset);
            model = mAsset;
            break;
        case 2:  //利润表
            mProfit = new QStandardItemModel;
            ui->tabProfit->setModel(mProfit);
            model = mProfit;
            break;
        }


        QList<QStandardItem*> itemLst;
        //加入报表字段标题和行数
        while(q.next()){
            QString rowNumber = QString::number(q.value(0).toInt());
            QString fname = q.value(1).toString();
            QString title = q.value(2).toString();
            QStandardItem* itemName = new QStandardItem(fname);
            QStandardItem* itemRow = new QStandardItem(rowNumber);
            QStandardItem* itemTitle = new QStandardItem(title);
            QStandardItem* itemValue = new QStandardItem("");
            itemLst.clear();
            itemLst << itemName << itemTitle << itemRow <<itemValue;
            model->appendRow(itemLst);
        }
        //加入报表标题
        model->setHeaderData(0, Qt::Horizontal, "fname");
        model->setHeaderData(1, Qt::Horizontal, tr("项目"));
        model->setHeaderData(2, Qt::Horizontal, tr("行数"));
        model->setHeaderData(3, Qt::Horizontal, tr("本月数"));
    }    
}

void SetupBaseDialog::saveExtra()
{
    //保存科目余额数据
    QSqlQuery q;
    QString s;
    bool r;

    int year = ui->datExtra->date().year();
    int month = ui->datExtra->date().month();

    //为健壮起见，应检测是否存在对应记录，如有则先删除之
    s = QString("delete from SubjectExtras where (year = %1) and "
                "(month = %2)").arg(year).arg(month);
    r = q.exec(s);

    //构造插入语句
    s = "insert into SubjectExtras(year,month,state,";
    QString v = "values(:year,:month,:state,";
    QList<QString> fidLst = edtHash.keys();
    qSort(fidLst.begin(), fidLst.end());
    for(int i = 0; i < fidLst.count(); ++i){
        s.append(fidLst[i]);
        s.append(",");
        v.append(":");
        v.append(fidLst[i]);
        v.append(",");
    }
    s.chop(1);
    s.append(") ");
    v.chop(1);
    v.append(")");
    s.append(v);
    r = q.prepare(s);

    //绑定值
    q.bindValue(":year", year);
    q.bindValue(":month", month);
    q.bindValue(":state", 1);  //已结转状态
    QHashIterator<QString, QLineEdit*> i(edtHash);
    while(i.hasNext()){
        i.next();
        q.bindValue(QString(":%1").arg(i.key()), i.value()->text().toDouble());
    }
    r = q.exec();

//    extraMapper->submit();  //单独调用此不会保存数据，搞不懂？
//    extraModel->submit();
}


//保存利润表数据
void SetupBaseDialog::saveProfit()
{
    bool r;
    int month;
    int year = ui->datProfit->date().year();
    if(ui->chkProfit->isChecked())
        month = 0;
    else
        month = ui->datProfit->date().month();

    //这里为了简化代码，直接硬编码表名，应该利用PSetting类读取
    QString tabName = "OldProfits";
    QSqlQuery q;

    //检查利润表中是否已存在当前选择年月的数据记录，如有则删除或更新
    QString s = QString("select id from %1 where (year = %2) and (month "
                        "= %3)").arg(tabName).arg(year).arg(month);
    if(q.exec(s) && q.first())
       if(QMessageBox::Yes == QMessageBox::question(this, tr("确认信息"),
            tr("利润表中已有对应数据记录，要覆盖吗？"),
            QMessageBox::Yes | QMessageBox::No)){
        s = QString("delete from %1 where (year = %2) and (month = %3)")
            .arg(tabName).arg(year).arg(month);
        r = q.exec(s);
    }
    else
        return;

    //将新数据保存到利润表中
    s = QString("insert into %1(year, month, ").arg(tabName);
    QString vs = QString("values(%1, %2, ").arg(year).arg(month);
    for(int i = 0; i < mProfit->rowCount(); ++i){
        QString fname = mProfit->data(mProfit->index(i, 0)).toString();
        double v = mProfit->data(mProfit->index(i, 3)).toDouble();
        s.append(fname);
        s.append(", ");
        vs.append(QString::number(v, 'f', 2));
        vs.append(", ");
    }
    s.chop(2);
    s.append(") ");
    vs.chop(2);
    vs.append(")");
    s.append(vs);
    r = q.exec(s);


    //mProfit
}

//保存资产负债表数据
void SetupBaseDialog::saveAsset()
{
    int i = 0;
}

void SetupBaseDialog::endStep()
{
    emit toNextStep(4, 4);  //步骤完成
}

/**
    从余额表中读取选定年月的余额数据，并显示在余额页中
*/
void SetupBaseDialog::readExtraData(const QDate &date)
{
    QString s;
    QSqlQuery q;
    bool r;

    int year = date.year();
    int month = date.month();

//    //extraModel->setTable("SubjectExtras");
//    s = QString("(year = %1) and (month = %2)").arg(year).arg(month);
//    extraModel->setFilter(s);
//    //extraMapper->setModel(extraModel);
//    extraModel->select();
//    int count = extraModel->rowCount();
//    extraMapper->toFirst();



    s = QString("select * from SubjectExtras where (year = %1) and "
                "(month = %2)").arg(year).arg(month);
    r = q.exec(s);
    if(r && q.first()){
        QSqlRecord rec = q.record();
        for(int i = SE_SUBSTART; i < rec.count(); ++i){
            QString fname = rec.fieldName(i);
            if(edtHash.contains(fname)){
                double v = q.value(i).toDouble();
                if(v != 0)
                    edtHash[fname]->setText(QString::number(v, 'f', 2));
            }
        }
    }
    else{
        QHashIterator<QString, QLineEdit*> i(edtHash);
        while(i.hasNext()){
            i.next();
            i.value()->setText("");
        }
    }
}

//显示子目的余额
void SetupBaseDialog::viewSubExtra(QString code, const QPoint& pos)
{
    int year = ui->datExtra->date().year();
    int month = ui->datExtra->date().month();

    //在显示子目余额前，首先要确保在SubjectExtras表中存在对应的总目余额记录条目
    QSqlQuery q;
    QString s = QString("select id from SubjectExtras where (year = %1) "
                        "and (month = %2)").arg(year).arg(month);
    if(q.exec(s) && q.first()){
        DetailExtraDialog* view = new DetailExtraDialog(code, year, month);
        connect(view, SIGNAL(extraValueChanged()), this, SLOT(refreshExtra()));
        view->move(pos);
        view->exec();
    }
    else
        QMessageBox::information(this, tr("提醒信息"),
                                 tr("总账科目的余额值还未保存，请先保存之"));

}

//重新刷新显示总账科目的余额值
void SetupBaseDialog::refreshExtra()
{
    extraModel->select();
    extraMapper->toFirst();
}


