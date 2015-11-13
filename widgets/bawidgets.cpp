#include <QDomDocument>

#include "bawidgets.h"
#include "../logs/Logger.h"
#include "statements.h"

/////////////////////////BusinessActionItem/////////////////////////////////////////////////
BASummaryItem_new::BASummaryItem_new(const QString content,
                             SubjectManager* subMgr, int type) : QTableWidgetItem(type)
{
    oppoSubject = 0;
    subManager = subMgr;
    parse(content);
}


QTableWidgetItem* BASummaryItem_new::clone() const
{
    QString content = assembleContent();
    BASummaryItem_new *item = new BASummaryItem_new(content);
    *item = *this;
    return item;
}

//返回指定角色的数据内容
QVariant BASummaryItem_new::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole)
        return summary;
    if(role == Qt::EditRole)
        return assembleContent();
    if(role == Qt::ToolTipRole)
        return genQuoteInfo();
    return QTableWidgetItem::data(role);

}


void BASummaryItem_new::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        parse(value.toString());
    QTableWidgetItem::setData(role, value);
}

//将内容解析分离为摘要体和引用体
void BASummaryItem_new::parse(QString content)
{
    QString summ,xmlStr;
    int start = content.indexOf("<");
    int end   = content.lastIndexOf(">");
    if((start > 0) && (end > start)){
        summ = content.left(start);               //摘要体部分
        summary = summ;

        xmlStr = content.right(content.count() - start); //引用体部分
        QDomDocument dom;
        bool r = dom.setContent(xmlStr);
        //发票号部分
        if(dom.elementsByTagName("fp").count() > 0){
            QDomNode node = dom.elementsByTagName("fp").at(0);
            fpNums = node.childNodes().at(0).nodeValue().split(",");
        }
        //银行票据号部分
        if(dom.elementsByTagName("bp").count() > 0){
            QDomNode node = dom.elementsByTagName("bp").at(0);
            bankNums = node.childNodes().at(0).nodeValue();
        }
        //对方科目部分
        if(dom.elementsByTagName("op").count() > 0){
            QDomNode node = dom.elementsByTagName("op").at(0);
            oppoSubject = node.childNodes().at(0).nodeValue().toInt();
        }
    }
    else{
        summary = content;
    }
}

//装配内容
QString BASummaryItem_new::assembleContent() const
{
    bool isHave = false;
    QDomDocument dom;
    QDomElement root = dom.createElement("rp");
    QString fpStr = fpNums.join(",");
    //QString bpStr = ui->edtBankNum->text();

    //发票号部分
    if(fpStr != ""){
        isHave = true;
        QDomText fpText = dom.createTextNode(fpStr);
        QDomElement fpEle = dom.createElement("fp");
        fpEle.appendChild(fpText);
        root.appendChild(fpEle);
    }
    //银行票据号部分
    if(bankNums != ""){
        isHave = true;
        QDomText bpText = dom.createTextNode(bankNums);
        QDomElement bpEle = dom.createElement("bp");
        bpEle.appendChild(bpText);
        root.appendChild(bpEle);
    }
    //对方科目部分
    if(oppoSubject != 0){
        isHave = true;
        QDomText opText = dom.createTextNode(QString::number(oppoSubject));
        QDomElement opEle = dom.createElement("op");
        opEle.appendChild(opText);
        root.appendChild(opEle);
    }

    //QString s;
    if(isHave){
        dom.appendChild(root);
        QString s = QString("%1%2").arg(summary).arg(dom.toString());
        s.chop(1);
        return s;
    }
    else
        return summary;
}

//生成摘要引用部分的信息以便在工具提示框中显示
QString BASummaryItem_new::genQuoteInfo() const
{
    QString s;
    if(fpNums.count() > 0){
        s = QObject::tr("invoice number:");
        for(int i = 0; i < fpNums.count(); ++i)
            s.append(fpNums[i]).append(",");
        s.chop(1);
    }
    if(bankNums != "")
        s.append("\n").append(QObject::tr("bank bill:"))
                .append(bankNums);
    if(oppoSubject != 0)
        s.append("\n").append(QObject::tr("opposite side subject"))
                .append(subManager->getFstSubject(oppoSubject)->getName());

    return s;
}

///////////////////////////////BAFstSubItem///////////////////////////////
BAFstSubItem_new::BAFstSubItem_new(FirstSubject* fsub, SubjectManager* subMgr,int type) :
                           QTableWidgetItem(type),fsub(fsub),subManager(subMgr)
{}

QVariant BAFstSubItem_new::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole){
        if(!fsub)
            return "";
        else
            return fsub->getName();
    }
    if(role == Qt::EditRole){
        QVariant v;
        v.setValue(fsub);
        return v;
    }
    return QTableWidgetItem::data(role);
}

void BAFstSubItem_new::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        fsub = value.value<FirstSubject*>();
    QTableWidgetItem::setData(role, value);
}

///////////////////////BASndSubItem/////////////////////////////
BASndSubItem_new::BASndSubItem_new(SecondSubject* ssub, SubjectManager *subMgr, int type)
    : QTableWidgetItem(type),ssub(ssub),subMgr(subMgr)
{}

QVariant BASndSubItem_new::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::ToolTipRole){
        QString tip;
        if(ssub){
            tip = ssub->getLName();
            if(subMgr->isBankSndSub(ssub)){
                BankAccount* ba = subMgr->getBankAccount(ssub);
                tip.append(QObject::tr("\n账户：%1\n是否基本户：%2").arg(ba->accNumber).
                           arg(ba->parent->isMain?yesStr:noStr));
            }
        }
        return tip;
    }
    if(role == Qt::DisplayRole){
        if(ssub)
            return ssub->getName();
        else
            return "";
    }

    if(role == Qt::EditRole){
        QVariant v;
        v.setValue(ssub);
        return v;
    }
    return QTableWidgetItem::data(role);
}

void BASndSubItem_new::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole){
        ssub = value.value<SecondSubject*>();
    }
    QTableWidgetItem::setData(role, value);
}


///////////////////////////BAMoneyTypeItem2/////////////////////////////
BAMoneyTypeItem_new::BAMoneyTypeItem_new(Money* mt, int type)
    : QTableWidgetItem(type),mt(mt)
{}

QVariant BAMoneyTypeItem_new::data(int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int)Qt::AlignCenter;
    if(role == Qt::DisplayRole){
        if(!mt)
            return "";
        else
            return mt->name();
    }
    if(role == Qt::EditRole){
        QVariant v;
        v.setValue(mt);
        return v;
    }
    return QTableWidgetItem::data(role);
}

void BAMoneyTypeItem_new::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        mt = value.value<Money*>();
    QTableWidgetItem::setData(role, value);
}


//////////////////////////////////BAMoneyValueItem////////////////////////
//设置金额的借贷方向
BAMoneyValueItem_new::BAMoneyValueItem_new(int witch, double v, QColor forColor, int type):
    QTableWidgetItem(type),witch(witch),v(Double(v)),outCon(false),forColor(forColor)
{
//    QFont f = font();
//    f.setBold(true);
//    setFont(f);
}

BAMoneyValueItem_new::BAMoneyValueItem_new(int witch, Double v, QColor forColor, int type):
    QTableWidgetItem(type),witch(witch),v(v),outCon(false),forColor(forColor)
{
//    QFont f = font();
//    f.setBold(true);
//    setFont(f);
}

void BAMoneyValueItem_new::setDir(int dir)
{
    if((dir == DIR_J) || (dir == DIR_P) || (dir == DIR_D))
        witch = dir;
    else{
        LOG_ERROR(QObject::tr("direction code(%1) is invalid!").arg(dir));
        dir = DIR_P;
    }
}

QVariant BAMoneyValueItem_new::data(int role) const
{
    if(role == Qt::ForegroundRole){
        if(!outCon){
            if(v < 0.00)             //负数用红色
                return qVariantFromValue(QColor(Qt::red));
            else if(witch == DIR_J)  //借方金额用蓝色
                return qVariantFromValue(QColor(Qt::blue));
            else if(witch == DIR_D)  //贷方金额用绿色
                return qVariantFromValue(QColor(Qt::darkGreen));
        }
        else
            return qVariantFromValue(forColor);

    }
    if(role == Qt::DisplayRole)
        return v.toString();
    if(role == Qt::EditRole){
        QVariant va;
        va.setValue(v);
        return va;
    }
    return QTableWidgetItem::data(role);
}

void BAMoneyValueItem_new::setData(int role, const QVariant &value)
{
    if(role == Qt::EditRole)
        //v = Double(value.toDouble());
        v = value.value<Double>();
//    if(role == Qt::ForegroundRole){
//        if(outCon)
//            forColor = value.value<QColor>();
//    }
    QTableWidgetItem::setData(role, value);
}

void BAMoneyValueItem_new::setData(int role, const Double &value)
{
    if(role == Qt::EditRole)
        v = value;
    QTableWidgetItem::setData(role, v.getv());
}
