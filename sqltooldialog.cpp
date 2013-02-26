#include <QSqlQuery>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>


#include "sqltooldialog.h"
#include "ui_sqltooldialog.h"

SqlToolDialog::SqlToolDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::SqlToolDialog)
{
    ui->setupUi(this);

    setLayout(ui->mlayout);
    ui->tview->setContextMenuPolicy(Qt::CustomContextMenu);

    mc=bc=0;
    model = new  QSqlQueryModel;
    tmodel = new QSqlTableModel;
    initTableList();

}

SqlToolDialog::~SqlToolDialog()
{
    delete ui;
}

void SqlToolDialog::excuteSql()
{
    QSqlQuery q;
    bool r;
    int c;
    bool isSelect;

    QString s = ui->edtSql->toPlainText().simplified();
    if(QString::compare(s.left(6), "select", Qt::CaseInsensitive) == 0)
        isSelect = true;
    else
        isSelect = false;

    if(!s.isEmpty()){
        r = q.exec(s);
        if(r){  //成功执行
            if(isSelect){//如果执行的是select语句，则显示查询的数据
                model->setQuery(s);
                ui->tview->setModel(model);
                //ui->stackedWidget->setCurrentIndex(0);
            }
            else{
                c = q.numRowsAffected();
                QString t = QString(tr("受影响的记录有： %1 条。")).arg(c);
                ui->edtResult->setPlainText(t);
                //ui->stackedWidget->setCurrentIndex(1);
            }


        }
        else{  //输出错误信息
            QString t = QString(tr("执行失败！"));
            ui->edtResult->setPlainText(t);
            //ui->stackedWidget->setCurrentIndex(1);
        }
    }

}

//读取所有数据库内的表，并加入到组合框中
void SqlToolDialog::initTableList()
{
    /** sqlite_master表结构：
            type:类型（table,...）
            name：表名
            tbl_name：表名
            rootpage：一个整数，不知是什么意思？
            sql：创建表所用的Sql语句
    */

    QSqlQuery q;
    bool r;

    ui->cmbTables->clear();
    QString s = "select * from sqlite_master";
    if(q.exec(s)){
        while(q.next()){
            QString tname = q.value(2).toString();
            ui->cmbTables->addItem(tname);
            mc++;
        }
    }
}

void SqlToolDialog::selectedTable(const QString &text)
{
    if(ui->chkIsEdit->isChecked()){
        tmodel->setTable(text);
        tmodel->select();
        ui->tview->setModel(tmodel);
    }
    else{
        QString s = QString("select * from %1").arg(text);
        ui->edtSql->setPlainText(s);
        excuteSql();
    }

}


void SqlToolDialog::toggled(bool checked)
{
    ui->btnSave->setEnabled(checked);
}

void SqlToolDialog::save()
{
    //if(ui->chkIsEdit)
    //    tmodel->submit();

//    //此是临时代码
//    if(ui->chkAttach->isChecked()){
//        QStringList states;
//        QSqlQuery q;
//        bool r;

//        states << "delete from basic.pzsStateNames"
//               << "insert into basic.pzsStateNames(code,state) values(1,'初始状态')";

//        for(int i=0;i<states.count();++i){
//            QString s = states.at(i);
//            r = q.exec(s);
//            int j = 0;
//        }
//    }

    //此是临时代码，创建固定资产管理相关表
    QSqlQuery q;
    bool r;
    r = q.exec("DROP TABLE IF EXISTS gdzc_classes");
    //r = q.exec("CREATE TABLE gdzc_classes(id INTEGER PRIMARY KEY, code INTEGER, name TEXT, zjMonths INTEGER)");
    //r = q.exec(tr("insert into gdzc_classes(code,name,zjMonths) values(1,'电子产品',36)"));
    //r = q.exec(tr("insert into gdzc_classes(code,name,zjMonths) values(20,'自定义',0)"));

    r = q.exec("DROP TABLE IF EXISTS gdzcs");
    r = q.exec("CREATE TABLE gdzcs(id INTEGER PRIMARY KEY,code INTEGER,pcls INTEGER,"
               "scls INTEGER,name TEXT,model TEXT,buyDate TEXT,prime DOUBLE,remain DOUBLE,"
               "min DOUBLE,zjMonths INTEGER,pid INTEGER,bid INTEGER,desc TEXT)");
    r = q.exec("DROP TABLE IF EXISTS gdzczjs");
    r = q.exec("CREATE TABLE gdzczjs(id INTEGER PRIMARY KEY,gid INTEGER,pid INTEGER,"
               "bid INTEGER,date TEXT,price DOUBLE)");
    int i = 0;
}

void SqlToolDialog::contextMenuRequested(const QPoint &pos)
{
    //检测是否有选中的记录，如有则添加删除记录菜单
    actDel = new QAction(tr("删除记录"), this);
    actClear = new QAction(tr("清空表"), this);
    actAdd = new QAction(tr("插入记录"), this);
    connect(actDel, SIGNAL(triggered()), this, SLOT(delRec()));
    connect(actClear, SIGNAL(triggered()), this, SLOT(clearAll()));
    connect(actAdd, SIGNAL(triggered()), this, SLOT(addNewRec()));

    QMenu* m = new QMenu;
    m->addAction(actDel);
    m->addAction(actClear);
    m->addAction(actAdd);
    //m->popup(mapToParent(pos));
    //m->popup(pos);
    m->popup(mapToGlobal(pos));
}

//删除选中的记录
void SqlToolDialog::delRec()
{
    QItemSelectionModel* selModel = ui->tview->selectionModel();
    if(selModel->hasSelection() && ui->chkIsEdit->isChecked()){
        QModelIndex item;
        QModelIndexList selist = selModel->selectedIndexes();
        QSet<int> rowset;
        foreach(item, selist){        //统计要删除的行的序号
            rowset.insert(item.row());
        }
        QList<int> rowlst = rowset.toList();
        //从后向前删除
        qSort(rowlst.begin(), rowlst.end());
        for(int i = rowlst.count()-1; i >= 0; --i){
            tmodel->removeRow(rowlst[i]);
        }
        //model->submit();
    }
}

//清空表
void SqlToolDialog::clearAll()
{
    if(ui->chkIsEdit->isChecked()){
        int rows = tmodel->rowCount();
        tmodel->removeRows(0, rows);
    }

}

//添加新记录
void SqlToolDialog::addNewRec()
{
    if(ui->chkIsEdit->isChecked()){
        int row = tmodel->rowCount();
        tmodel->insertRow(row);
    }
}

//附加基础数据库到当前连接中
void SqlToolDialog::attachBasis(bool checked)
{
    bool r;
    QString s;
    QSqlQuery q;
    QString filename = "./datas/basicdatas/basicdata.dat";

    if(checked){ //将基础数据库附加到当前数据库连接上
        s = QString("attach database '%1' as basic").arg(filename);
        r = q.exec(s);
        if(r){
            s = "select name from basic.sqlite_master";
            r = q.exec(s);
            QString tname;
            while(q.next()){
                tname = q.value(0).toString();
                ui->cmbTables->addItem(QString("basic.%1").arg(tname));
                bc++;
            }
        }
    }
    else{ //将基础数据库与当前数据库连接分离
        s = QString("detach database basic");
        r = q.exec(s);
        //移除基础数据库中的表名
        for(int i = ui->cmbTables->count()-1; i>mc-1;i--)
            ui->cmbTables->removeItem(i);
    }
}

//附加导入数据库
void SqlToolDialog::on_chkImport_toggled(bool checked)
{
    bool r;
    QString s;
    QSqlQuery q;
    QString filename;

    if(checked){ //将外部数据库附加到当前数据库连接上
        //取得数据库的路径名
        filename = QFileDialog::
            getOpenFileName(this,tr("打开数据库文件"),QDir::currentPath(),tr("Sqlite(*.dat)"));
        if(filename == "")
            ui->btnImport->setEnabled(false);
        s = QString("attach database '%1' as import").arg(filename);
        r = q.exec(s);
        if(r){
            s = "select name from import.sqlite_master";
            r = q.exec(s);
            QString tname;
            while(q.next()){
                tname = q.value(0).toString();
                ui->cmbTables->addItem(QString("import.%1").arg(tname));
                bc++;
            }
        }
    }
    else{ //将外部数据库与当前数据库连接分离
        if(ui->btnImport->isEnabled()){
            s = QString("detach database import");
            r = q.exec(s);
            //移除基础数据库中的表名
            for(int i = ui->cmbTables->count()-1; i>mc-1;i--)
                ui->cmbTables->removeItem(i);
        }
    }
}

//执行导入操作
void SqlToolDialog::on_btnImport_clicked()
{
    QSqlQuery q,q1;
    QStringList tables,sqls;
    bool r;
    QString s;

    //创建新的账户信息表，将老的账户信息表内的数据转入新表
    r = q.exec("CREATE TABLE AccountInfo(id INTEGER PRIMARY KEY, code int, name TEXT, value TEXT)");
    r = q.exec("select * from AccountInfos");
    r = q.first();
    //账户代码
    QString ts = q.value(ACCOUNT_CODE).toString();
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::ACODE).arg("accountCode").arg(ts);
    r = q1.exec(s);
    //账户开始记账时间
    ts = q.value(ACCOUNT_BASETIME).toString().append("-01");
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::STIME).arg("startTime").arg(ts);
    r = q1.exec(s);
    //账户终止记账时间
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::ETIME).arg("endTime").arg("2011-11-30");
    r = q1.exec(s);
    //账户简称
    ts = q.value(ACCOUNT_SNAME).toString();
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::SNAME).arg("shortName").arg(ts);
    r = q1.exec(s);
    //账户全称
    ts = q.value(ACCOUNT_LNAME).toString();
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::LNAME).arg("longName").arg(ts);
    r = q1.exec(s);
    //账户所用科目类型
    int v = q.value(ACCOUNT_USEDSUB).toInt();
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::SUBTYPE).arg("subType").arg(QString::number(v));
    r = q1.exec(s);
    //所用报表类型
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::RPTTYPE).arg("reportType").arg(QString::number(v));
    r = q1.exec(s);
    //本币
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::MASTERMT).arg("masterMt").arg("1");
    r = q1.exec(s);
    //外币
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::WAIMT).arg("WaiBiList").arg("2");
    r = q1.exec(s);

    //当前帐套
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::CSUITE).arg("currentSuite").arg("2011");
    r = q1.exec(s);
    //帐套名列表
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::SUITENAME).arg("suiteNames").arg(tr("2011,2011年"));
    r = q1.exec(s);
    //账户最后访问时间
    ts = q.value(ACCOUNT_LASTTIME).toString();
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::LASTACCESS).arg("lastAccessTime").arg(ts);
    r = q1.exec(s);
    //日志文件名
    ts = curAccount->getFileName();
    //ts.replace(".dat",".log");
    ts.append(".log");
    s = QString("insert into AccountInfo(code,name,value) values(%1,'%2','%3')")
            .arg(Account::LOGFILE).arg("logFileName").arg(ts);
    r = q1.exec(s);

    ////////////////////////////////////////////////////////////////////
    //创建固定资产类别表
    s = "CREATE TABLE gdzc_classes(id INTEGER PRIMARY KEY, code INTEGER, name TEXT, zjMonths INTEGER)";
    r = q1.exec(s);
    //创建固定资产表
    s = "CREATE TABLE gdzcs(id INTEGER PRIMARY KEY,code INTEGER,pcls INTEGER,"
        "scls INTEGER,name TEXT,model TEXT,buyDate TEXT,prime DOUBLE,"
        "remain DOUBLE,min DOUBLE,zjMonths INTEGER,desc TEXT)";
    r = q1.exec(s);
    //创建固定资产折旧信息表
    s = "CREATE TABLE gdzczjs(id INTEGER PRIMARY KEY,gid INTEGER,pid INTEGER,"
        "bid INTEGER,date TEXT,price DOUBLE)";
    r = q1.exec(s);
    /////////////////////////////////////////////////////////////////////


    /////////////////////////////////////////////////////
//    tables = QString("FstSubClasses,FirSubjects,SndSubClass,SecSubjects,FSAgent,"
//                     "PingZhengs,BusiActions,ExchangeRates,Banks,MoneyTypes,"
//                     "BankAccounts,AccountInfos").split(",");
//    sqls << "insert into FstSubClasses(id,code,name) select id,code,name from import.FstSubClasses"
//         << "insert into FirSubjects(id,subCode,remCode,belongTo,isView,isReqDet,weight,subName,description,utils) select id,subCode,remCode,belongTo,isView,isReqDet,weight,subName,description,utils from import.FirSubjects"
//         << "insert into SndSubClass(id,clsCode,name,explain) select id,clsCode,name,explain from import.SndSubClass"
//         << "insert into SecSubjects(id,subName,subLName,remCode,classId) select id,subName,subLName,remCode,classId from import.SecSubjects"
//         << "insert into FSAgent(id,fid,sid,subCode,isDetByMt,FrequencyStat) select id,fid,sid,subCode,isDetByMt,FrequencyStat from import.FSAgent"
//         << "insert into PingZhengs(id,date,number,zbNum,jsum, dsum,isForward, encNum) select id,date,number,numberSub,jsum,dsum,isForward,encNum from import.PingZhengs"
//         << "insert into BusiActions(id,pid,summary,firSubID,secSubID,moneyType,jMoney,dMoney,dir) select id,pid,summary,firSubID,secSubID,moneyType,jMoney,dMoney,dir from import.BusiActions"
//         << "insert into ExchangeRates(id,year,month,usd2rmb) select id,year,month,usd2rmb from import.ExchangeRates"
//         << "insert into Banks(id,isMain,name,lname) select id,isMain,name,lname from import.Banks"
//         << "insert into MoneyTypes(id,code,sign,name) select id,code,sign,name from import.MoneyTypes"
//         << "insert into BankAccounts(id,bankID,mtID,accNum) select id,bankID,mtID,accNum from import.BankAccounts"
//         << "insert into AccountInfos(id,code,sname,lname,usedSubId) select id,code,sname,lname,usedSubId from import.AccountInfos";

//    foreach(QString name, tables){
//        r = q.exec(QString("delete from %1").arg(name));
//        if(!r && QMessageBox::Yes ==
//                QMessageBox::critical(this,tr("出错提示"),
//                    tr("在删除表%1中的记录时出错，终止导入吗？").arg(name),
//                    QMessageBox::Yes | QMessageBox::No))
//            return;
//    }

//    foreach(QString s, sqls){
//        r = q.exec(s);
//        if(!r && QMessageBox::Yes ==
//                QMessageBox::critical(this,tr("出错提示"),
//                    tr("在执行导入语句 %1 时出错，终止导入吗？").arg(s),
//                    QMessageBox::Yes | QMessageBox::No))
//            return;
//    }

//    //将老版用0表示贷方改为新版用-1代表贷方
//    r = q.exec("update BusiActions set dir = -1 where dir = 0");
//    //添加凭证的用户信息（将录入者设置为凌坤，审核者和记账者为章莹，凭证状态为初始录入态）
//    r = q.exec("update PingZhengs set vuid = 4,buid=4,ruid=3,pzState=1");

//    //添加本年利润-结转子科目
//    QString s = QString("select id from FirSubjects where subCode = '3131'");
//    r = q.exec(s); r = q.first();
//    int fid = q.value(0).toInt(); //本年利润科目id
//    s = QString("select id from SecSubjects where subName='%1'").arg(tr("结转"));
//    int sid;
//    QString sname = tr("结转");
//    if(q.exec(s) && q.first())
//        sid = q.value(0).toInt();
//    else{
//        s = QString("insert into SecSubjects(subName,subLName,remCode,classId) "
//                    "values('%1','%2','%3',%4)").arg(sname).arg(sname).arg("jz").arg(22);
//        r = q.exec(s);
//        s = QString("select id from SecSubjects where subName='%1'").arg(tr("结转"));
//        q.exec(s); q.first();
//        sid = q.value(0).toInt();
//    }
//    s = QString("insert into FSAgent(fid,sid,subCode) values(%1,%2,'%3')")
//            .arg(fid).arg(sid).arg("01");
//    r = q.exec(s);

//    //清空余额表
//    s = QString("delete from SubjectExtras");
//    r = q.exec(s);
//    s = QString("delete from SubjectExtraDirs");
//    r = q.exec(s);
//    s = QString("delete from detailExtras");
//    r = q.exec(s);

//    int i = 0;
}

////////////////////////////BaseDataEditDialog///////////////////////////////////////////
BaseDataEditDialog::BaseDataEditDialog(QSqlDatabase* con, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BaseDataEditDialog)
{
    ui->setupUi(this);
    setLayout(ui->mLayout);
    this->con = con;

    model = new  QSqlQueryModel;
    tmodel = new QSqlTableModel(parent,*con);
    vh = ui->tview->verticalHeader();
    connect(vh, SIGNAL(sectionClicked(int)), this, SLOT(seledRow(int)));
    init();
}

BaseDataEditDialog::~BaseDataEditDialog()
{
    delete ui;
}

void BaseDataEditDialog::init()
{
    QSqlQuery q(*con);
    bool r;

    ui->cmbTables->clear();
    QString s = "select * from sqlite_master";
    if(q.exec(s)){
        while(q.next()){
            QString tname = q.value(2).toString();
            ui->cmbTables->addItem(tname);

        }
    }
}

void BaseDataEditDialog::excuteSql()
{
    QSqlQuery q(*con);
    bool r;
    int c;
    bool isSelect;

    QString s = ui->edtSql->toPlainText().simplified();
    if(QString::compare(s.left(6), "select", Qt::CaseInsensitive) == 0)
        isSelect = true;
    else
        isSelect = false;

    if(!s.isEmpty()){
        r = q.exec(s);
        if(r){  //成功执行
            if(isSelect){//如果执行的是select语句，则显示查询的数据
                model->setQuery(s,*con);
                ui->tview->setModel(model);
                ui->lblMsg->setText(tr("查询成功完成！"));
            }
            else{
                c = q.numRowsAffected();
                QString t = QString(tr("操作成功完成，受影响的记录有： %1 条。")).arg(c);
                ui->lblMsg->setText(t);
            }
        }
        else{  //输出错误信息
            QString t = QString(tr("执行失败！"));
            ui->lblMsg->setText(t);
        }
    }
}

//选择一个表后
void BaseDataEditDialog::on_cmbTables_currentIndexChanged(const QString &arg1)
{
    tmodel->setTable(arg1);
    tmodel->select();
    ui->tview->setModel(tmodel);
    ui->edtSql->setPlainText(QString("select * from %1").arg(arg1));
    selRows.clear();
}

void BaseDataEditDialog::on_btnSubmit_clicked()
{
    tmodel->submit();
}

void BaseDataEditDialog::on_btnRevert_clicked()
{
    tmodel->revert();
}

void BaseDataEditDialog::on_btnSwitch_clicked(bool checked)
{
    if(checked){
        ui->btnSwitch->setText(tr("编辑模式"));
        ui->tview->setEditTriggers(QAbstractItemView::DoubleClicked |
                                   QAbstractItemView::EditKeyPressed |
                                   QAbstractItemView::AnyKeyPressed);
    }
    else{
        ui->btnSwitch->setText(tr("只读模式"));
        ui->tview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
}

void BaseDataEditDialog::on_btnAdd_clicked()
{
    if(ui->btnSwitch->isChecked()){
        if(ui->tview->model() == tmodel){
            int row = tmodel->rowCount();
            tmodel->insertRow(row);
            ui->tview->scrollToBottom();
            int i = 0;
        }
    }
}

void BaseDataEditDialog::on_btnDel_clicked()
{
    if(ui->btnSwitch->isChecked()){
        if(selRows.isEmpty())
            return;
        QList<int> rowLst = selRows.toList();
        qSort(rowLst.begin(),rowLst.end());
        for(int i = rowLst.count()-1; i > -1; i--)
            tmodel->removeRow(rowLst[i]);
    }
}

void BaseDataEditDialog::seledRow(int rowIndex)
{
    if(selRows.contains(rowIndex))
        selRows.remove(rowIndex);
    else
        selRows.insert(rowIndex);
    if(!selRows.isEmpty())
        ui->btnDel->setEnabled(true);
    else
        ui->btnDel->setEnabled(false);
}

void BaseDataEditDialog::on_tview_clicked(const QModelIndex &index)
{
    selRows.clear();
    ui->btnDel->setEnabled(false);
}
