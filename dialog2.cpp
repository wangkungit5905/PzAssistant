#include <qxml.h>
#include <QDomDocument>
#include <QSqlQuery>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QBuffer>
#include <QItemSelectionModel>

#include <qglobal.h>


#include "dialog2.h"
#include "utils.h"
#include "widgets.h"
#include "printUtils.h"
#include "global.h"
#include "widgets.h"
#include "utils.h"
#include "completsubinfodialog.h"
#include "mainwindow.h"
#include "tdpreviewdialog.h"
#include "previewdialog.h"
#include "tables.h"
#include "subject.h"
#include "cal.h"
#include "dbutil.h"
#include "PzSet.h"
#include "pz.h"

//tem
//#include "dialog3.h"

#ifdef Q_OS_LINUX
#define	FW_NORMAL	400
#define	FW_BOLD		700
#endif


BASummaryForm::BASummaryForm(QWidget *parent) : QWidget(parent),
    ui(new Ui::BASummaryForm)
{
    ui->setupUi(this);
    setLayout(ui->mLayout);    

    //初始化对方科目列表
    QSqlQuery q;
    bool r = q.exec(QString("select id,%1 from %2 where %3=1")
                    .arg(fld_fsub_name).arg(tbl_fsub).arg(fld_fsub_isview));
    ui->cmbOppSub->addItem("", 0);  //不指向任何科目
    while(q.next()){
        int id = q.value(0).toInt();
        QString name = q.value(1).toString();
        ui->cmbOppSub->addItem(name, id);
    }

}

BASummaryForm::~BASummaryForm()
{
    emit dataEditCompleted(ActionEditItemDelegate2::SUMMARY);
    delete ui;
}

//将摘要字段的数据进行分离，分别填入不同的显示部件中
void BASummaryForm::setData(QString data)
{
    QString summ,xmlStr,refNum,bankNum,opSub;
    int start = data.indexOf("<");
    int end   = data.lastIndexOf(">");
    if((start > 0) && (end > start)){
        summ = data.left(start);
        ui->edtSummary->setText(summ);
        xmlStr = data.right(data.count() - start);
        QDomDocument dom;
        bool r = dom.setContent(xmlStr);
        if(dom.elementsByTagName("fp").count() > 0){
            QDomNode node = dom.elementsByTagName("fp").at(0);
            QString fp = node.childNodes().at(0).nodeValue();
            ui->edtFpNum->setText(fp);
        }
        if(dom.elementsByTagName("bp").count() > 0){
            QDomNode node = dom.elementsByTagName("bp").at(0);
            QString bp = node.childNodes().at(0).nodeValue();
            ui->edtBankNum->setText(bp);
        }
        if(dom.elementsByTagName("op").count() > 0){
            QDomNode node = dom.elementsByTagName("op").at(0);
            int op = node.childNodes().at(0).nodeValue().toInt();
            int idx = ui->cmbOppSub->findData(op);
            ui->cmbOppSub->setCurrentIndex(idx);
        }
    }
    else{
        ui->edtSummary->setText(data);
    }

}



QString BASummaryForm::getData()
{
    bool isHave = false;
    QDomDocument dom;
    QDomElement root = dom.createElement("rp");
    QString fpStr = ui->edtFpNum->text();
    QString bpStr = ui->edtBankNum->text();

    if(fpStr != ""){
        isHave = true;
        QDomText fpText = dom.createTextNode(fpStr);
        QDomElement fpEle = dom.createElement("fp");
        fpEle.appendChild(fpText);
        root.appendChild(fpEle);
    }

    if(bpStr != ""){
        isHave = true;
        QDomText bpText = dom.createTextNode(bpStr);
        QDomElement bpEle = dom.createElement("bp");
        bpEle.appendChild(bpText);
        root.appendChild(bpEle);
    }

    if(ui->cmbOppSub->currentIndex() != 0){
        isHave = true;
        int id = ui->cmbOppSub->itemData(ui->cmbOppSub->currentIndex()).toInt();
        QDomText opText = dom.createTextNode(QString::number(id));
        QDomElement opEle = dom.createElement("op");
        opEle.appendChild(opText);
        root.appendChild(opEle);
    }

    QString s;
    if(isHave){
        dom.appendChild(root);
        s = ui->edtSummary->text()/*.append("\n")*/.append(dom.toString());
        s.chop(1);
    }
    else
        s = ui->edtSummary->text();

    return s;
}





////////////////////////PrintSelectDialog////////////////////////////////////
PrintSelectDialog::PrintSelectDialog(QList<PingZheng*> choosablePzSets, PingZheng* curPz, QWidget *parent) :
    QDialog(parent),ui(new Ui::PrintSelectDialog),pzSets(choosablePzSets),curPz(curPz)
{
    ui->setupUi(this);
    if(choosablePzSets.isEmpty())
        enableWidget(false);
    else{
        ui->edtCur->setText(QString::number(curPz?curPz->number():0));
        QString allText;
        int count = pzSets.count();
        if(count == 1)
            allText = "1";
        else
            allText = QString("1-%1").arg(count);
        ui->edtAll->setText(allText);
    }
    connect(ui->rdoSel,SIGNAL(toggled(bool)),this,SLOT(selectedSelf(bool)));
}

PrintSelectDialog::~PrintSelectDialog()
{
    delete ui;
}

//设置要打印的凭证号集合
void PrintSelectDialog::setPzSet(QSet<int> pznSet)
{
    QString ps = IntSetToStr(pznSet);
    ui->edtSel->setText(ps); //将集合解析为简写文本表示形式
    ui->rdoSel->setChecked(true);
}

//设置当前的凭证号
void PrintSelectDialog::setCurPzn(int pzNum)
{
    if(pzNum != 0)
        ui->edtCur->setText(QString::number(pzNum));
    else{
        ui->rdoCur->setEnabled(false);
        ui->edtCur->setEnabled(false);
    }

}

/**
 * @brief PrintSelectDialog::getSelectedPzs
 *  获取选择要打印的凭证对象
 * @param pzs
 * @return 0：未选，1：所有凭证，2：当前凭证，3：自选凭证
 */
int PrintSelectDialog::getSelectedPzs(QList<PingZheng *> &pzs)
{
    pzs.clear();
    if(pzSets.isEmpty())
        return 0;
    if(ui->rdoCur->isChecked() && curPz){
        pzs<<curPz;
        return 2;
    }
    else if(ui->rdoAll->isChecked()){
        pzs = pzSets;
        return 1;
    }
    else{
        QSet<int> pzNums;
        if(!strToIntSet(ui->edtSel->text(),pzNums)){
            QMessageBox::warning(this,tr("警告信息"),tr("凭证范围选择格式有误！"));
            return 0;
        }
        foreach (PingZheng* pz, pzSets){
            if(pzNums.contains(pz->number()))
                pzs<<pz;
        }
        return 3;
    }
}

//获取打印模式（返回值 1：输出地打印机，2:打印预览，3：输出到PDF）
//int PrintSelectDialog::getPrintMode()
//{
//    if(ui->rdoPrint->isChecked())
//        return 1;
//    else if(ui->rdoPreview->isChecked())
//        return 2;
//    else
//        return 3;
//}

/**
 * @brief PrintSelectDialog::selectedSelf
 *  选择自选模式后，启用右边的编辑框
 * @param checked
 */
void PrintSelectDialog::selectedSelf(bool checked)
{
    ui->edtSel->setReadOnly(!checked);
}

/**
 * @brief PrintSelectDialog::enableWidget
 * @param en
 */
void PrintSelectDialog::enableWidget(bool en)
{
    ui->rdoAll->setEnabled(en);
    ui->rdoCur->setEnabled(en);
    ui->rdoSel->setEnabled(en);
    ui->edtAll->setEnabled(en);
    ui->edtCur->setEnabled(en);
    ui->edtSel->setEnabled(en);
}

/**
 * @brief PrintSelectDialog::IntSetToStr
 *  将凭证号集合转换为文本表示的简略范围表示形式
 * @param set
 * @return
 */
QString PrintSelectDialog::IntSetToStr(QSet<int> set)
{
    QString s;
    if(set.count() > 0){
        QList<int> pzs = set.toList();
        qSort(pzs.begin(),pzs.end());
        int prev = pzs[0],next = pzs[0];
        for(int i = 1; i < pzs.count(); ++i){
            if((pzs[i] - next) == 1){
                next = pzs[i];
            }
            else{
                if(prev == next)
                    s.append(QString::number(prev)).append(",");
                else
                    s.append(QString("%1-%2").arg(prev).arg(next)).append(",");
                prev = next = pzs[i];
            }
        }
        if(prev == next)
            s.append(QString::number(prev));
        else
            s.append(QString("%1-%2").arg(prev).arg(pzs[pzs.count() - 1]));
    }
    return s;
}

/**
 * @brief PrintSelectDialog::strToIntSet
 *  将文本表示的凭证号选择范围转换为等价的凭证号集合
 * @param s
 * @param set
 * @return
 */
bool PrintSelectDialog::strToIntSet(QString s, QSet<int> &set)
{
    //首先用规则表达式验证字符串中是否存在不可解析的字符，如有则返回false
    set.clear();
    if(false)
        return false;
    //对打印范围的编辑框文本进行解析，生成凭证号集合
    QStringList sels = s.split(",");
    for(int i = 0; i < sels.count(); ++i){
        if(sels[i].indexOf('-') == -1)
            set.insert(sels[i].toInt());
        else{
            QStringList ps = sels[i].split("-");
            int start = ps[0].toInt();
            int end = ps[1].toInt();
            for(int j = start; j <= end; ++j)
                set.insert(j);
        }
    }
    return true;
}


//////////////////////////LoginDialog///////////////////////////
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    //setLayout(ui->mLayout);
    //this->witch = witch;
    init();
}

void LoginDialog::init()
{    
    QHashIterator<int,User*> it(allUsers);
    int ruIndex = 0, index = 0;
    while(it.hasNext()){
        it.next();
        if(it.value()->getUserId() == recentUserId)
            ruIndex = index;
        ui->cmbUsers->addItem(it.value()->getName(),it.value()->getUserId());
        index++;
    }
    ui->cmbUsers->setCurrentIndex(ruIndex);
    ui->edtPw->setFocus();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

User* LoginDialog::getLoginUser()
{
    int userId = ui->cmbUsers->itemData(ui->cmbUsers->currentIndex()).toInt();
    return allUsers.value(userId);
}

//登录
void LoginDialog::on_btnLogin_clicked()
{

    int userId =ui->cmbUsers->itemData(ui->cmbUsers->currentIndex()).toInt();
    if(allUsers.value(userId)->verifyPw(ui->edtPw->text()))
        accept();
    else
        QMessageBox::warning(this, tr("警告信息"), tr("密码不正确"));
}

//取消登录
void LoginDialog::on_btnCancel_clicked()
{
    QDialog::reject();
}

///////////////////////////SecConDialog/////////////////////////////////////
SecConDialog::SecConDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SecConDialog)
{
    ui->setupUi(this);
    setLayout(ui->mLayout);
    ui->tabRight->setLayout(ui->trLayout);
    ui->tabGroup->setLayout(ui->tgLayout);
    ui->tabUser->setLayout(ui->tuLayout);
    ui->tabOperate->setLayout(ui->toLayout);

    //初始化数据修改标记
    rightDirty = false;
    groupDirty = false;
    userDirty = false;
    operDirty = false;

    //添加权限表的上下文菜单
    ui->tvRight->addAction(ui->actAddRight);
    ui->tvRight->addAction(ui->actDelRight);

    //添加修改组权限的上下文菜单
    ui->trwGroup->addAction(ui->actChgGrpRgt);

    //添加修改用户所属组的上下文菜单
    ui->lwOwner->addAction(ui->actChgUserOwner);

    //添加修改操作所需权限的上下文菜单
    ui->trwOper->addAction(ui->actChgOpeRgt);

    init();
}

//初始化
void SecConDialog::init()
{
    QTableWidgetItem* item;
    ValidableTableWidgetItem* vitem;

    QList<int> codes;
    vat = new QIntValidator(1, 1000, this);


    //装载权限
    codes = allRights.keys();
    qSort(codes.begin(), codes.end());
    ui->tvRight->setRowCount(codes.count());
    ui->tvRight->setColumnCount(4);
    QStringList headTitles;
    headTitles << tr("权限代码") << tr("权限类别") << tr("权限名称") << tr("权限简介");
    ui->tvRight->setHorizontalHeaderLabels(headTitles);
    ui->tvRight->setColumnWidth(0, 80);
    ui->tvRight->setColumnWidth(1, 80);
    ui->tvRight->setColumnWidth(2, 150);
    //int w = ui->tvRight->width();
    ui->tvRight->setColumnWidth(3, 500);

    for(int r = 0; r < codes.count(); ++r){
        Right* right = allRights.value(codes[r]);        
        item = new ValidableTableWidgetItem(QString::number(right->getCode()), vat);
        ui->tvRight->setItem(r, 0, item);        
        item = new QTableWidgetItem;
        item->setData(Qt::EditRole, right->getType());
        ui->tvRight->setItem(r, 1, item);
        item = new QTableWidgetItem(right->getName());
        ui->tvRight->setItem(r, 2, item);
        item = new QTableWidgetItem(right->getExplain());
        ui->tvRight->setItem(r, 3, item);
    }

    iTosItemDelegate* rightTypeDele = new iTosItemDelegate(allRightTypes, this);
    ui->tvRight->setItemDelegateForColumn(1, rightTypeDele);

    connect(ui->tvRight, SIGNAL(cellChanged(int,int)),
            this, SLOT(onRightellChanged(int,int)));

    //装载用户组
    QListWidgetItem* litem;
    QHashIterator<int,UserGroup*> it(allGroups);
    while(it.hasNext()){
        it.next();
        litem = new QListWidgetItem(it.value()->getName());
        litem->setData(Qt::UserRole, it.key());
        ui->lwGroup->addItem(litem);
    }


    //装载用户

    //装载操作


    //装载权限类型
    initRightTypes(0);
}

//初始化用户组和操作的（第一层次）权限类型
void SecConDialog::initRightTypes(int pcode, QTreeWidgetItem* pitem)
{
    QString s;
    QSqlQuery q(bdb);
    s = QString("select code,name from rightType where pcode = %1").arg(pcode);
    bool r = q.exec(s);
    QTreeWidgetItem* item;
    if(pcode == 0){  //第一层次权限类型
        QList<QTreeWidgetItem*> items;
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            item = new QTreeWidgetItem(ui->trwGroup, QStringList(name));
            item->setData(0,Qt::UserRole,code);
            //items.append(item);
        }
        //ui->trwGroup->addTopLevelItems(items);
        for(int i = 0; i < ui->trwGroup->topLevelItemCount(); ++i){
            item = ui->trwGroup->topLevelItem(i);
            int code = item->data(0, Qt::UserRole).toInt();
            initRightTypes(code, item);
        }
    }
    else{
        while(q.next()){
            int code = q.value(0).toInt();
            QString name = q.value(1).toString();
            item = new QTreeWidgetItem(pitem, QStringList(name));
            item->setData(0,Qt::UserRole,code);
            //pitem->addChild(item);
            initRightTypes(code,item);
        }
    }


    //将组权限加入到对应的权限类别分支中
    item = ui->trwGroup->topLevelItem(0);
    //int i = 0;
    QTreeWidgetItem* ritem;
//    QHashIterator<int,Right*> it(allRights);
//    while(it.hasNext()){
//        it.next();
//        ritem = new QTreeWidgetItem(QStringList(it.value()->getName()));
//        //ritem = new QTreeWidgetItem;                            //QTreeWidgetItem::UserType); //此树项目表示权限而不是权限类别
//        //ritem->setText(0, it.value()->getName());
//        ritem->setCheckState(0,Qt::Checked);
//        //item = findItem(ui->trwGroup, it.value()->getType());
//        QString rname = it.value()->getName();
//        QString rtname = allRightTypes.value(it.value()->getType());
//        //QList<QTreeWidgetItem*> items = ui->trwGroup->findItems(rtname,Qt::MatchFixedString);
////        if(items.count() == 1){
////            items[0]->addChild(ritem);

////        }
//        item->addChild(ritem);
//        i++;
//    }

//    QList<QTreeWidgetItem *> items;
//    for(int i = 0; i<10; ++i){
//        ritem = new QTreeWidgetItem(item, QStringList(QString("Item-%1").arg(i)));
        //items.append(ritem);
//    }
//    item->addChildren(items);
//    int j = 0;
}

SecConDialog::~SecConDialog()
{
    delete ui;
}

//在树中查找具有指定用户数据的项目（只考虑第一次匹配的项目）
QTreeWidgetItem* SecConDialog::findItem(QTreeWidget* tree, int code, QTreeWidgetItem* startItem)
{
    QTreeWidgetItem* item;
    int count;
    int i = 0;
    if(startItem == NULL){  //从顶级项目开始查找
        count = tree->topLevelItemCount();
        while(i < count){
            item = tree->topLevelItem(i);
            if(item->data(0, Qt::UserRole).toInt() == code)
                return item;
            if(item->childCount() > 0)
                item = findItem(tree, code, item);
            if(item == NULL)
                i++;
            else
                return item;
        }
    }
    else{  //从指定项目开始查找
        count = startItem->childCount();
        while(i < count){
            item = startItem->child(i);
            if(item->data(0, Qt::UserRole).toInt() == code)
                return item;
            if(item->childCount() > 0)
                item = findItem(tree, code, item);
            if(item == NULL)
                i++;
            else
                return item;
        }
    }
    return NULL;
}

//添加权限
void SecConDialog::on_actAddRight_triggered()
{
    //自动赋一个未使用的权限代码
    int rows = ui->tvRight->rowCount();
    ui->tvRight->setRowCount(++rows);
    //因为在初始化显示权限表前已经根据权限代码进行了排序，因此，表的最后一行即是最大的代码数字
    int code = ui->tvRight->item(rows -2, 0)->data(Qt::DisplayRole).toInt();
    ValidableTableWidgetItem* vitem = new ValidableTableWidgetItem(QString::number(++code), vat);
    ui->tvRight->setItem(rows - 1, 0, vitem);
    ui->tvRight->setItem(rows - 1, 1, new QTableWidgetItem());
    ui->tvRight->setItem(rows - 1, 2, new QTableWidgetItem());
    ui->tvRight->setCurrentCell(rows - 1, 1);
    rightDirty = true;
}

//删除权限
void SecConDialog::on_actDelRight_triggered()
{
    //首先获取选择的行
    QList<int> row;
    QList<QTableWidgetSelectionRange> ranges = ui->tvRight->selectedRanges();
    foreach(QTableWidgetSelectionRange range, ranges){
        for(int i = 0; i < range.rowCount(); ++i){
            row.append(range.topRow() + i);
        }
    }
    if(row.count() > 0){
        qSort(row.begin(), row.end());
        for(int i = row.count() - 1; i > -1; i--){
            int r = row[i];
            ui->tvRight->removeRow(r);
        }
        rightDirty = true;
    }


}

//保存
void SecConDialog::on_btnSave_clicked()
{
    if(rightDirty){
        saveRights();
        rightDirty = false;
    }
    else if(groupDirty){
        saveGroups();
        groupDirty = false;
    }
    else if(userDirty){
        saveUsers();
        userDirty = false;
    }
    else if(operDirty){
        saveOperates();
        operDirty = false;
    }

    //还要考虑保持同4个全局变量的同步（allRights,allGroups,allUsers,allOperates）
}

//保存权限
void SecConDialog::saveRights()
{

    QSqlQuery q(bdb);
    QString s;
    bool rt;
    bool typeChanged = false; //权限类别改变了
    bool nameChanged = false; //名称改变标记
    bool explChanged = false; //权限简介改变标记
    QList<int> prevCodes = allRights.keys();

    //遍历权限表格
    for(int r = 0; r < ui->tvRight->rowCount(); ++r){
        int code = ui->tvRight->item(r, 0)->data(Qt::DisplayRole).toInt();
        int type = ui->tvRight->item(r, 1)->data(Qt::EditRole).toInt();
        QString name = ui->tvRight->item(r, 2)->text();
        QString explain = ui->tvRight->item(r, 3)->text();
        if(!allRights.contains(code)){ //如果在全局权限中没有对应项，则应新建
            Right* right = new Right(code, type, name, explain);
            allRights[code] = right;
            //在基础数据库中添加新记录
            s = QString("insert into rights(code,type,name,explain) values(%1,%2,'%3','%4')")
                    .arg(code).arg(type).arg(name).arg(explain);
            rt = q.exec(s);
        }
        else if(type != allRights.value(code)->getType()){
            allRights.value(code)->setType(type);
            typeChanged = true;
        }
        else if((name != allRights.value(code)->getName())){
            allRights.value(code)->setName(name);
            nameChanged = true;
        }
        else if(explain != allRights.value(code)->getExplain()){
            allRights.value(code)->setExplain(explain);
            explChanged = true;
        }
        //更新名称和简介内容
        if(typeChanged || nameChanged || explChanged){
            s = QString("update rights set ");
            if(typeChanged)
                s.append(QString("type = %1, ").arg(type));
            if(nameChanged)
                s.append(QString("name = '%1', ").arg(name));
            if(explChanged)
                s.append(QString("explain = '%1', ").arg(explain));
            s.chop(2);
            s.append(QString(" where code = %1").arg(code));
            rt = q.exec(s);
            nameChanged = false; explChanged = false;
            int j = 0;
        }
        prevCodes.removeOne(code); //移除已经处理过的项目
    }
    //删除遗留权限
    if(prevCodes.count() > 0){
        for(int i = 0; i < prevCodes.count(); ++i){
            allRights.remove(prevCodes[i]);
            //删除数据库中的对应记录
            s = QString("delete from rights where code = %1").arg(prevCodes[i]);
            rt = q.exec(s);
            int j = 0;
        }
    }

}

void SecConDialog::saveGroups()
{

}

void SecConDialog::saveUsers()
{

}

void SecConDialog::saveOperates(){

}


//关闭
void SecConDialog::on_btnClose_clicked()
{
    on_btnSave_clicked();
    close();
}

//修改组权限
void SecConDialog::on_actChgGrpRgt_triggered()
{

}

//修改用户所属组
void SecConDialog::on_actChgUserOwner_triggered()
{

}

//修改操作所需权限
void SecConDialog::on_actChgOpeRgt_triggered()
{

}


//当权限表的内容改变
void SecConDialog::onRightellChanged(int row, int column)
{
    if(column == 0){ //如果改变的是权限代码，要检测是否有同值冲突
        ValidableTableWidgetItem* item = (ValidableTableWidgetItem*)ui->tvRight->item(row, column);
        int code = item->text().toInt();
        //检测权限表中是否存在重复的代码
        for(int r = 0; r < ui->tvRight->rowCount(); ++r){
            if(r != row){ //排除本行
                if(code == ui->tvRight->item(r, 0)->data(Qt::DisplayRole).toInt()){
                     QMessageBox::warning(this, tr("提示信息"), tr("代码冲突或无效"));
                     item->setBackground(QBrush(Qt::red));
                     return;
                }
            }
        }
        item->setBackground(Qt::white);
    }
    rightDirty = true;

}

void SecConDialog::on_lwGroup_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    //如果先前的用户组权限有改变，则先保存

    //再设置新的当前用户组的权限
}


/////////////////////SearchDialog///////////////////////////////////////
SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog)
{
    ui->setupUi(this);
}

SearchDialog::~SearchDialog()
{
    delete ui;
}











