#include <QStringList>
#include <QBuffer>

#include "tables.h"
#include "utils.h"
#include "securitys.h"


QSet<int> BusiUtil::pset; //具有正借负贷特性的一级科目id集合
QSet<int> BusiUtil::nset; //具有负借正贷特性的一级科目id集合
QSet<int> BusiUtil::spset; //具有正借负贷特性的二级科目id集合
QSet<int> BusiUtil::snset; //具有负借正贷特性的二级科目id集合

QSet<int> fidByMt; //需要按币种进行明细统计功能的一级科目id集合（这是临时代码，这个信息需要在一级科目表中反映）

QSet<int> BusiUtil::impPzCls;   //由其他模块引入的凭证类别代码集合
QSet<int> BusiUtil::jzhdPzCls;  //由系统自动产生的结转汇兑损益凭证类别代码集合
QSet<int> BusiUtil::jzsyPzCls;  //由系统自动产生的结转损益凭证类别代码集合
QSet<int> BusiUtil::otherPzCls; //其他由系统产生并允许用户修改的凭证类别代码集合
QSet<int> BusiUtil::accToMt;    //需要按币种核算的科目id集合

QSet<int> BusiUtil::inIds;  //损益类科目中的收入类科目id集合
QSet<int> BusiUtil::feiIds; //损益类科目中的费用类科目id集合
QSet<int> BusiUtil::inSIds;  //损益类科目中的收入类子目id集合
QSet<int> BusiUtil::feiSIds; //损益类科目中的费用类子目id集合

ExpressParse::ExpressParse()
{
    OpratorPri p1 = {'(',1};
    lpri[0] = p1;
    OpratorPri p2 = {'*',5};
    lpri[1] = p2;
    OpratorPri p3 = {'/',5};
    lpri[2] = p3;
    OpratorPri p4 = {'+',3};
    lpri[3] = p4;
    OpratorPri p5 = {'-',3};
    lpri[4] = p5;
    OpratorPri p6 = {')',6};
    lpri[5] = p6;
    OpratorPri p7 = {'=',1};
    lpri[6] = p7;

    OpratorPri p8 = {'(',6};
    rpri[0] = p8;
    OpratorPri p9 = {'*',4};
    rpri[1] = p9;
    OpratorPri p10 = {'/',4};
    rpri[2] = p10;
    OpratorPri p11 = {'+',2};
    rpri[3] = p11;
    OpratorPri p12 = {'-',2};
    rpri[4] = p12;
    OpratorPri p13 = {')',1};
    rpri[5] = p13;
    OpratorPri p14 = {'=',1};
    rpri[6] = p14;

}

//求左运算符op的优先级
int ExpressParse::leftpri(char op){
    int i;
    for (i=0; i<MaxOp; i++)
        if(lpri[i].ch==op)
            return lpri[i].pri;
}

//求右运算符op的优先级
int ExpressParse::rightpri(char op){
    int i;
    for (i=0;i<MaxOp;i++)
        if (rpri[i].ch==op)
            return rpri[i].pri;
}


int ExpressParse::InOp(char ch){
    if (ch=='(' || ch==')' || ch=='+' || ch=='-' || ch=='*' || ch=='/'||ch=='=')
        return 1;
    else
        return 0;
}

//判断str是否为运算符
bool ExpressParse::InOp(QString str)
{
    if(str.length() == 1){
        char ch = str[0].toAscii();
        if(ch=='(' || ch==')' || ch=='+' || ch=='-' || ch=='*' || ch=='/'||ch=='=')
            return true;
    else
        false;

    }

}

//op1和op2运算符优先级的比较结果
int ExpressParse::Precede(char op1, char op2)
{
    if(leftpri(op1) == rightpri(op2))
        return 0;
    else if(leftpri(op1) < rightpri(op2))
        return -1;
    else
        return 1;
}

//计算后缀表达式的值
void ExpressParse::compValue(QStack<char> &op, QStack<double> &st)
{
    double a,b,c;
    switch (op.top()){
    case '+':    //判定为'+'号
        a = st.pop();
        b = st.pop();
        c = a+b;    //计算c
        st.push(c); //将计算结果进栈
        break;
    case '-':    //判定为'-'号
        a = st.pop();
        b = st.pop();
        c=b-a;    //计算c
        st.push(c); //将计算结果进栈
        break;
    case '*':    //判定为'*'号
        a = st.pop();
        b = st.pop();
        c=a*b;    //计算c
        st.push(c); //将计算结果进栈
        break;
    case '/':    //判定为'/'号
        a = st.pop();
        b = st.pop();
        if (a!=0){
            c=b/a;   //计算c
            st.push(c);//将计算结果进栈
        }
        else{
            qDebug() << QObject::tr("\n\t除零错误!\n");
            exit(0);  //异常退出
        }
        break;
    }
}

//将操作数按需进行标量值的替换
double ExpressParse::replaceValue(QString op)
{
    //首先判断是否是数字
    double v;
    bool ok;
    v = op.toDouble(&ok);
    if(ok)
        return v;
    else{
        if(!vhash->contains(op)){  //该标量还没有对应的值
            qDebug() << QString(QObject::tr("标量%1没有对应值")).arg(op);
            return 0;
        }
        else
            return vhash->value(op);
    }
}

void ExpressParse::trans(char *exp)
{
    QStack<char> op;
    QStack<double> st;
    int i=0;  //i作为postexp的下标
    double d;

    op.push('=');  //将'='进栈，这是计算结束的标志
    while(*exp!='='||op.top()!='='){   //exp表达式未扫描完时循环
        if(!InOp(*exp)){  //为数字字符的情况
            d=0;
            while (*exp>='0' && *exp<='9'){ //判定为数字
                d=10*d+*exp-'0';
                exp++;
            }
            st.push(d);
        }
        else     //为运算符的情况
            //栈顶操作符和表达式中扫描到的操作符进行优先级比较
            switch(Precede(op.top(),*exp)){
            case -1:   //栈顶运算符的优先级低
                op.push(*exp);
                exp++;   //继续扫描其他字符
                break;
            case 0:    //只有括号满足这种情况(在刚刚执行完括弧里的表达式后，去掉左括弧)
                op.pop(); //将(退栈
                exp++;   //继续扫描其他字符
                break;
            case 1:             //退栈并输出到postexp中
                compValue(op,st);
                op.pop();
                break;
            }
    }
    while (op.top()!='='){ //此时exp扫描完毕,退栈到'='为止
        compValue(op,st);
        op.pop();
    }
    //qDebug() << st.top();
}

double ExpressParse::trans(QStack<QString> exp)
{
    QStack<char> op; //操作符栈
    QStack<double> st;  //操作数栈

    op.push('=');//将'='进栈
    while(exp.top()!="="||op.top()!='='){   //exp表达式未扫描完时循环
        if(!InOp(exp.top())){  //如果是操作数，则将操作数压入操作数栈st
            double v = replaceValue(exp.pop());
            st.push(v);
        }
        else     //为运算符的情况
            //栈顶操作符和表达式中扫描到的操作符进行优先级比较
            switch(Precede(op.top(), exp.top()[0].toAscii())){
            case -1:   //栈顶运算符的优先级低
                op.push(exp.top()[0].toAscii());
                exp.pop();   //继续扫描其他字符
                break;
            case 0:    //只有括号满足这种情况(在刚刚执行完括弧里的表达式后，去掉左括弧)
                op.pop();
                exp.pop();   //继续扫描其他字符
                break;
            case 1:             //退栈并输出到postexp中
                compValue(op,st);
                op.pop();
                break;
            }
    }

    while(op.top()!='='){ //此时exp扫描完毕,退栈到'='为止
        compValue(op,st);
        op.pop();
    }
    //qDebug() << st.top();
    return st.top();
}

//对原始表达式（表达式中可以带有标量，这里要求标量用方括弧包裹）进行求值
double ExpressParse::calValue(QString exp, QHash<QString, double>* vhash)
{
    if(exp == "")
        return 0;

    QStack<QString> expStk;
    //this->vhash.unite(vhash);
    this->vhash = vhash; //标量值集合

    //首先将原始表达式转换为中的操作符和操作数按从右到左的顺序压入堆栈
    int pos = 0;
    QStringList lst;
    QString str;

    //搜索3种类型的符号，标量（由方括弧包裹，中间是任意的字母数字序列）、
    //实数、运算符（“+”“-”“*”“/”“(”“)”）
    QRegExp reg("(\\[\\w+\\])|(\\d+\\.?\\d{0,2})|([\\+|\\-|\\*|\\\\])");
    while((pos = reg.indexIn(exp, pos)) != -1){
        str = exp.mid(pos, reg.matchedLength());
        if(str.left(1) == "["){
            int l = str.length();
            str = str.mid(1, l - 2);
        }
        lst.append(str);
        expStk.push_front(str);
        pos += reg.matchedLength();
    }
    expStk.push_front("=");  //这个作为求值结束的依据，必须的

    //求值
    double v;
    v = this->trans(expStk);

    //qDebug() << v;
    return v;
}

/////////////////////////BusiUtil///////////////////////////////////
/**
    类的初始化函数
*/
void BusiUtil::init()
{
    QSqlQuery q,q2;
    QString s;
    bool r;

    //初始化一二级科目的借贷方向特效id集合
    //资产类（正：借，负：贷）
    //负债类（负：借，正：贷）
    //所有者权益类：（正：贷，负：借）
    //损益类-收入（正：贷，负：借）
    //损益类-费用（正：借，负：贷）
    r = q.exec("select id from FirSubjects where (jdDir = 1) and "
                    "(isView = 1)");
    while(q.next())
        pset.insert(q.value(0).toInt());

    r = q.exec("select id from FirSubjects where (jdDir = 0) and "
                                "(isView = 1)");
    while(q.next())
        nset.insert(q.value(0).toInt());

    int fid;
    QList<int> ids;
    QList<QString> names;
    QSetIterator<int>* it = new QSetIterator<int>(pset);
    while(it->hasNext()){
        fid = it->next();
        getSndSubInSpecFst(fid,ids,names);
        foreach(int id, ids)
            spset << id;
    }

    ids.clear();
    names.clear();
    it = new QSetIterator<int>(nset);
    while(it->hasNext()){
        fid = it->next();
        getSndSubInSpecFst(fid,ids,names);
        foreach(int id, ids)
            snset << id;
    }

    //初始化凭证类别代码集合
    impPzCls.insert(Pzc_GdzcZj);
    impPzCls.insert(Pzc_Dtfy);

    jzhdPzCls.insert(Pzc_Jzhd_Bank);
    jzhdPzCls.insert(Pzc_Jzhd_Ys);
    jzhdPzCls.insert(Pzc_Jzhd_Yf);

    jzsyPzCls.insert(Pzc_JzsyIn);
    jzsyPzCls.insert(Pzc_JzsyFei);

    otherPzCls.insert(Pzc_Jzlr);

    //初始化需要按币种分别核算的科目的集合
    //这些是临时代码，未来需要从一级科目表的某个字段中读取或采用其他的配置机制
    getIdByCode(fid,"1002"); //银行
    accToMt.insert(fid);
    getIdByCode(fid,"1131"); //应收
    accToMt.insert(fid);
    getIdByCode(fid,"2121"); //应付
    accToMt.insert(fid);
    getIdByCode(fid,"1151"); //预付
    accToMt.insert(fid);
    getIdByCode(fid,"2131"); //预收
    accToMt.insert(fid);

    //初始化损益类科目中收入类和费用类科目id集合
    r = q.exec("select id,jdDir from FirSubjects where (belongTo=5) "
               "and (isView = 1)");
    int dir,sid;
    while(q.next()){
        fid = q.value(0).toInt();
        dir = q.value(1).toInt();
        if(dir == 1)
            feiIds.insert(fid);
        else
            inIds.insert(fid);
        s = QString("select id from FSAgent where fid = %1").arg(fid);
        r = q2.exec(s);
        while(q2.next()){
            sid = q2.value(0).toInt();
            if(dir == 1)
                feiSIds.insert(sid);
            else
                inSIds.insert(sid);
        }
    }

}

/**
    读取指定年月的汇率
*/
bool BusiUtil::getRates(int y,int m, QHash<int,double>& rates){
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("select code,sign,name from MoneyTypes where "
                            "code != 1");
    r = q.exec(s);
    QHash<int,QString> msHash; //货币代码到货币符号的映射
    while(q.next()){
        msHash[q.value(0).toInt()] = q.value(1).toString();
    }

    QList<int> mtcs = msHash.keys();
    s = QString("select ");
    for(int i = 0; i<mtcs.count(); ++i){
        s.append(msHash[mtcs[i]]).append("2rmb,");
    }
    s.chop(1);
    s.append(" from ExchangeRates ");
    s.append(QString("where (year = %1) and (month = %2)").arg(y).arg(m));
    if(!q.exec(s) || !q.first()){
        qDebug() << QObject::tr("没有指定汇率！");
        //QMessageBox::information(0,QObject::tr("提示信息"),
        //                         QString(QObject::tr("没有设置%1年%2月的汇率")).arg(y).arg(m));
        return false;
    }
    for(int i = 0;i<mtcs.count();++i){
        rates[mtcs[i]] = q.value(i).toDouble();
    }
    //(*rates)[RMB] = 1.00;
    return true;
}

bool BusiUtil::getRates2(int y,int m, QHash<int,Double>& rates){
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("select code,sign,name from MoneyTypes where "
                            "code != 1");
    r = q.exec(s);
    QHash<int,QString> msHash; //货币代码到货币符号的映射
    while(q.next()){
        msHash[q.value(0).toInt()] = q.value(1).toString();
    }

    QList<int> mtcs = msHash.keys();
    s = QString("select ");
    for(int i = 0; i<mtcs.count(); ++i){
        s.append(msHash[mtcs[i]]).append("2rmb,");
    }
    s.chop(1);
    s.append(" from ExchangeRates ");
    s.append(QString("where (year = %1) and (month = %2)").arg(y).arg(m));
    if(!q.exec(s) || !q.first()){
        qDebug() << QObject::tr("没有指定汇率！");
        //QMessageBox::information(0,QObject::tr("提示信息"),
        //                         QString(QObject::tr("没有设置%1年%2月的汇率")).arg(y).arg(m));
        return false;
    }
    for(int i = 0;i<mtcs.count();++i){
        rates[mtcs[i]] = Double(q.value(i).toDouble());
    }
    //(*rates)[RMB] = 1.00;
    return true;
}

/**
    保存指定年月的汇率
*/
bool BusiUtil::saveRates(int y,int m, QHash<int,double>& rates)
{
    QSqlQuery q;
    QString s,vs;
    bool r;

    QList<int> mtCodeLst;  //币种代码
    QList<QString> mtFields; //存放与币种对应汇率的字段名（按序号一一对应）
    s = QString("select code,sign from MoneyTypes where code!=%1").arg(RMB);
    r = q.exec(s);
    while(q.next()){
        mtCodeLst << q.value(0).toInt();
        mtFields << q.value(1).toString() + "2rmb";
    }

    s = QString("select id from ExchangeRates where (year = %1) and (month = %2)")
            .arg(y).arg(m);
    if(q.exec(s) && q.first()){
        int id = q.value(0).toInt();
        s = QString("update ExchangeRates set ");
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(mtCodeLst[i])){
                s.append(mtFields[i]).append("=")
                        .append(QString::number(rates.value(mtCodeLst[i])))
                        .append(", ");
            }
        }

        s.chop(2);
        s.append(QString(" where id = %1").arg(id));
        r = q.exec(s);
        return r;
    }
    else{
        s = QString("insert into ExchangeRates(year,month,");
        vs = QString("values(%1,%2,").arg(y).arg(m);
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(mtCodeLst[i])){
                s.append(mtFields[i]).append(", ");
                vs.append(QString::number(rates.value(mtCodeLst[i]),'f',2))
                        .append(", ");
            }
        }
        s.chop(2); vs.chop(2);
        s.append(") "); vs.append(")");
        s.append(vs);
        r = q.exec(s);
        return r;
    }
}

bool BusiUtil::saveRates2(int y, int m, QHash<int, Double> &rates)
{
    QSqlQuery q;
    QString s,vs;
    bool r;

    QList<int> mtCodeLst;  //币种代码
    QList<QString> mtFields; //存放与币种对应汇率的字段名（按序号一一对应）
    s = QString("select code,sign from MoneyTypes where code!=%1").arg(RMB);
    r = q.exec(s);
    while(q.next()){
        mtCodeLst << q.value(0).toInt();
        mtFields << q.value(1).toString() + "2rmb";
    }

    s = QString("select id from ExchangeRates where (year = %1) and (month = %2)")
            .arg(y).arg(m);
    if(q.exec(s) && q.first()){
        int id = q.value(0).toInt();
        s = QString("update ExchangeRates set ");
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(mtCodeLst[i])){
                s.append(mtFields[i]).append("=")
                        .append(rates.value(mtCodeLst[i]).toString())
                        .append(", ");
            }
        }

        s.chop(2);
        s.append(QString(" where id = %1").arg(id));
        r = q.exec(s);
        return r;
    }
    else{
        s = QString("insert into ExchangeRates(year,month,");
        vs = QString("values(%1,%2,").arg(y).arg(m);
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(mtCodeLst[i])){
                s.append(mtFields[i]).append(", ");
                vs.append(rates.value(mtCodeLst[i]).toString())
                        .append(", ");
            }
        }
        s.chop(2); vs.chop(2);
        s.append(") "); vs.append(")");
        s.append(vs);
        r = q.exec(s);
        return r;
    }
}


/**
    获取所有指定类别的损类总账和明细科目的id列表
    参数，isIncome：true（收入类），false（费用类）
*/
bool BusiUtil::getAllIdForSy(bool isIncome, QHash<int, QList<int> >& ids)
{
    QString s;
    QSqlQuery q;
    bool r;

    int cid = 5; //损益类科目类别代码
    if(isIncome){
        s = QString("select id from FirSubjects where (belongTo=%1) and "
                    "(jdDir=0) and (isView=1)").arg(cid);
    }
    else{
        s = QString("select id from FirSubjects where (belongTo=%1) and "
                    "(jdDir=1) and (isView=1)").arg(cid);
    }
    r = q.exec(s);
    QList<QString> snames;
    while(q.next()){
        int fid = q.value(0).toInt();
        getSndSubInSpecFst(fid,ids[fid],snames);
    }
}


/**
    获取所有总目id到总目名的哈希表
*/
bool BusiUtil::getAllSubFName(QHash<int,QString>& names, bool isByView)
{
    QString s;
    QSqlQuery q;

    s = "select id,subName from FirSubjects";
    if(isByView)
        s.append(" where isView = 1");
    if(q.exec(s)){
        if(names.count() > 0)
            names.clear();
        while(q.next())
            names[q.value(0).toInt()] = q.value(1).toString();
        return true;
    }
    return false;
}

/**
    获取所有总目id到总目代码的哈希表
*/
bool BusiUtil::getAllSubFCode(QHash<int,QString>& codes, bool isByView)
{
    QString s;
    QSqlQuery q;

    s = "select id,subCode from FirSubjects";
    if(isByView)
        s.append(" where isView = 1");
    if(q.exec(s)){
        if(!codes.empty())
            codes.clear();
        while(q.next())
            codes[q.value(0).toInt()] = q.value(1).toString();
        return true;
    }
    return false;
}

/**
    获取所有子目id到子目名的哈希表
*/
bool BusiUtil::getAllSubSName(QHash<int,QString>& names)
{
    QSqlQuery q;
    QString s;

    s = QString("select FSAgent.id,SecSubjects.subName from FSAgent "
                "join SecSubjects where FSAgent.sid = SecSubjects.id");
    if(!q.exec(s)){
        QMessageBox::information(0, QObject::tr("提示信息"),
                                 QString(QObject::tr("不能获取所有子科目哈希表")));
        return false;
    }
    if(names.count() > 0)
        names.clear();
    while(q.next()){
        int id = q.value(0).toInt();
        names[id] = q.value(1).toString();
    }
    return true;
}

//获取所有SecSubjects表中的二级科目名列表
bool BusiUtil::getAllSndSubNameList(QStringList& names)
{
    QSqlQuery q;
    bool r = q.exec("select subName from SecSubjects order by subLName");
    if(r){
        while(q.next())
            names.append(q.value(0).toString());
    }
    return r;
}

//获取所有子目id到子目全名的哈希表
bool BusiUtil::getAllSubSLName(QHash<int,QString>& names)
    {
        QSqlQuery q;
        QString s;

        s = QString("select FSAgent.id,SecSubjects.subLName from FSAgent "
                    "join SecSubjects where FSAgent.sid = SecSubjects.id");
        if(!q.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取所有子科目全名的哈希表")));
            return false;
        }
        while(q.next())
            names[q.value(0).toInt()] = q.value(1).toString();
        return true;
    }


//获取所有一级科目列表，按科目代码的顺序（参数ids：科目代码，names：科目名称，isByView：是否只提取当前账户需要的科目）
bool BusiUtil::getAllFstSub(QList<int>& ids, QList<QString>& names, bool isByView)
{
    QString s;
    QSqlQuery q;
    if(isByView)
        s = QString("select id,subName from FirSubjects where isView = 1 ");
    else
        s = QString("select id,subName from FirSubjects ");
    s.append("order by subCode");
    bool r = q.exec(s);
    while(q.next()){
        ids.append(q.value(0).toInt());
        names.append(q.value(1).toString());
    }
    return r;
}

//获取所有指定总目下的子目（参数ids：子目id，names：子目名）
bool BusiUtil::getSndSubInSpecFst(int pid, QList<int>& ids, QList<QString>& names)
{
    QString s;
    QSqlQuery q;

    if(!ids.empty())
        ids.clear();
    if(!names.empty())
        names.clear();
    s = QString("select FSAgent.id, SecSubjects.subName from FSAgent "
                "join SecSubjects where (FSAgent.sid = SecSubjects.id) "
                "and (FSAgent.fid = %1)").arg(pid);
    bool r = q.exec(s);
    while(q.next()){
        ids.append(q.value(0).toInt());
        names.append(q.value(1).toString());
    }
    return r;
}

//获取指定总目下、指定子目子集的名称（参数sids：指定子目子集id，names：子目名）
bool BusiUtil::getSubSetNameInSpecFst(int pid, QList<int> sids, QList<QString>& names)
{
    QString s;
    QSqlQuery q;

    if(!names.empty())
        names.clear();
    s = QString("select FSAgent.id, SecSubjects.subName from FSAgent "
                "join SecSubjects where (FSAgent.sid = SecSubjects.id) "
                "and (FSAgent.fid = %1)").arg(pid);
    bool r = q.exec(s);
    int sid;
    while(q.next()){
        sid = q.value(0).toInt();
        if(sids.contains(sid))
            names.append(q.value(1).toString());
    }
    return r;
}

//获取指定凭证id下的所有业务活动列表
bool BusiUtil::getActionsInPz(int pid, QList<BusiActionData*>& busiActions)
{
    QString s;
    QSqlQuery q;

    if(busiActions.count() > 0)
        busiActions.clear();

    s = QString("select * from BusiActions where pid = %1 order by NumInPz").arg(pid);
    bool r = q.exec(s);
    while(q.next()){
        BusiActionData* ba = new BusiActionData;
        ba->id = q.value(BACTION_ID).toInt();
        ba->pid = pid;
        ba->summary = q.value(BACTION_SUMMARY).toString();
        ba->fid = q.value(BACTION_FID).toInt();
        ba->sid = q.value(BACTION_SID).toInt();
        ba->mt  = q.value(BACTION_MTYPE).toInt();
        ba->dir = q.value(BACTION_DIR).toInt();
        if(ba->dir == DIR_J)
            ba->v = q.value(BACTION_JMONEY).toDouble();
        else
            ba->v = q.value(BACTION_DMONEY).toDouble();
        ba->num = q.value(BACTION_NUMINPZ).toInt();
        ba->state = BusiActionData::INIT;
        busiActions.append(ba);
    }
    return r;
}

bool BusiUtil::getActionsInPz2(int pid, QList<BusiActionData2 *> &busiActions)
{
    QString s;
    QSqlQuery q;

    if(busiActions.count() > 0)
        busiActions.clear();

    s = QString("select * from BusiActions where pid = %1 order by NumInPz").arg(pid);
    bool r = q.exec(s);
    while(q.next()){
        BusiActionData2* ba = new BusiActionData2;
        ba->id = q.value(BACTION_ID).toInt();
        ba->pid = pid;
        ba->summary = q.value(BACTION_SUMMARY).toString();
        ba->fid = q.value(BACTION_FID).toInt();
        ba->sid = q.value(BACTION_SID).toInt();
        ba->mt  = q.value(BACTION_MTYPE).toInt();
        ba->dir = q.value(BACTION_DIR).toInt();
        if(ba->dir == DIR_J)
            ba->v = Double(q.value(BACTION_JMONEY).toDouble());
        else
            ba->v = Double(q.value(BACTION_DMONEY).toDouble());
        ba->num = q.value(BACTION_NUMINPZ).toInt();
        ba->state = BusiActionData2::INIT;
        busiActions.append(ba);
    }
    return r;
}

//获取所有一级科目下的默认子科目（使用频度最高的子科目）
bool BusiUtil::getDefaultSndSubs(QHash<int,int>& defSubs)
{
    QString s;
    QSqlQuery q1,q2;
    bool r;
    int fs; //科目被使用的频度值或权重值
    int fid,id;

    r = q1.exec("select id from FirSubjects where isView = 1");
    while(q1.next()){
        fid = q1.value(0).toInt();
        s = QString("select id,FrequencyStat from FSAgent where fid = %1")
                .arg(fid);
        r = q2.exec(s);
        fs=id=0;
        //找出在某个一级科目下的最大使用频度值的明细科目id
        while(q2.next()){
            int tfs = q2.value(1).toInt();
            if(fs < tfs){
                fs = tfs;
                id = q2.value(0).toInt();
            }
        }
        defSubs[fid] = id;
    }

}

//保存指定凭证下的业务活动
bool BusiUtil::saveActionsInPz(int pid, QList<BusiActionData*>& busiActions,
                               QList<BusiActionData*> dels)
{
    QString s;
    QSqlQuery q1,q2,q3,q4;
    bool r, hasNew = false;

    s = QString("insert into BusiActions(pid,summary,firSubID,secSubID,"
                "moneyType,jMoney,dMoney,dir,NumInPz) values(:pid,:summary,"
                ":fid,:sid,:mt,:jv,:dv,:dir,:num)");
    r = q1.prepare(s);
    s = QString("update BusiActions set summary=:summary,firSubID=:fid,"
                "secSubID=:sid,moneyType=:mt,jMoney=:jv,dMoney=:dv,"
                "dir=:dir,NumInPz=:num where id=:id");
    r = q2.prepare(s);
    s = "update BusiActions set NumInPz=:num where id=:id";
    r = q3.prepare(s);
    s = "delete from BusiActions where id=:id";
    r = q4.prepare(s);

    BusiActionData* blankItem;
    if(!busiActions.isEmpty()){        
        for(int i = 0; i < busiActions.count(); ++i){
            busiActions[i]->num = i + 1;  //在保存的同时，重新赋于顺序号
            switch(busiActions[i]->state){
            case BusiActionData::INIT:
                break;
            case BusiActionData::NEW:
                hasNew = true;
                q1.bindValue(":pid",busiActions[i]->pid);
                q1.bindValue(":summary", busiActions[i]->summary);
                q1.bindValue(":fid", busiActions[i]->fid);
                q1.bindValue(":sid", busiActions[i]->sid);
                q1.bindValue(":mt", busiActions[i]->mt);
                if(busiActions[i]->dir == DIR_J){
                    q1.bindValue(":jv", busiActions[i]->v);
                    q1.bindValue(":dv",0);
                    q1.bindValue(":dir", DIR_J);
                }
                else{
                    q1.bindValue(":jv",0);
                    q1.bindValue(":dv", busiActions[i]->v);
                    q1.bindValue(":dir", DIR_D);
                }
                q1.bindValue(":num", busiActions[i]->num);
                r = q1.exec();
                if(r)
                    busiActions[i]->state = BusiActionData::INIT;
                break;
            case BusiActionData::EDITED:
                q2.bindValue(":summary", busiActions[i]->summary);
                q2.bindValue(":fid", busiActions[i]->fid);
                q2.bindValue(":sid", busiActions[i]->sid);
                q2.bindValue(":mt", busiActions[i]->mt);
                if(busiActions[i]->dir == DIR_J){
                    q2.bindValue(":jv", busiActions[i]->v);
                    q2.bindValue(":dv",0);
                    q2.bindValue(":dir", DIR_J);
                }
                else{
                    q2.bindValue(":jv",0);
                    q2.bindValue(":dv", busiActions[i]->v);
                    q2.bindValue(":dir", DIR_D);
                }
                q2.bindValue(":num", busiActions[i]->num);
                q2.bindValue("id", busiActions[i]->id);
                r = q2.exec();
                if(r)
                    busiActions[i]->state = BusiActionData::INIT;
                break;
            case BusiActionData::NUMCHANGED:
                q3.bindValue(":num", busiActions[i]->num);
                q3.bindValue(":id", busiActions[i]->id);
                r = q3.exec();
                if(r)
                    busiActions[i]->state = BusiActionData::INIT;
                break;
            }
        }
        //回读新增的业务活动的id(待需要时再使用此代码)
        if(hasNew){
            r = getActionsInPz(pid,busiActions);            
        }
    }
    if(!dels.isEmpty()){
        for(int i = 0; i < dels.count(); ++i){
            q4.bindValue(":id", dels[i]->id);
            r = q4.exec();
        }
    }
    return r;
}

bool BusiUtil::saveActionsInPz2(int pid, QList<BusiActionData2 *> &busiActions, QList<BusiActionData2 *> dels)
{
    QString s;
    QSqlQuery q1,q2,q3,q4;
    bool r, hasNew = false;

    s = QString("insert into BusiActions(pid,summary,firSubID,secSubID,"
                "moneyType,jMoney,dMoney,dir,NumInPz) values(:pid,:summary,"
                ":fid,:sid,:mt,:jv,:dv,:dir,:num)");
    r = q1.prepare(s);
    s = QString("update BusiActions set summary=:summary,firSubID=:fid,"
                "secSubID=:sid,moneyType=:mt,jMoney=:jv,dMoney=:dv,"
                "dir=:dir,NumInPz=:num where id=:id");
    r = q2.prepare(s);
    s = "update BusiActions set NumInPz=:num where id=:id";
    r = q3.prepare(s);
    s = "delete from BusiActions where id=:id";
    r = q4.prepare(s);

    BusiActionData2* blankItem;
    if(!busiActions.isEmpty()){
        for(int i = 0; i < busiActions.count(); ++i){
            busiActions[i]->num = i + 1;  //在保存的同时，重新赋于顺序号
            switch(busiActions[i]->state){
            case BusiActionData2::INIT:
                break;
            case BusiActionData2::NEW:
                hasNew = true;
                q1.bindValue(":pid",busiActions[i]->pid);
                q1.bindValue(":summary", busiActions[i]->summary);
                q1.bindValue(":fid", busiActions[i]->fid);
                q1.bindValue(":sid", busiActions[i]->sid);
                q1.bindValue(":mt", busiActions[i]->mt);
                if(busiActions[i]->dir == DIR_J){
                    q1.bindValue(":jv", busiActions[i]->v.getv());
                    q1.bindValue(":dv",0);
                    q1.bindValue(":dir", DIR_J);
                }
                else{
                    q1.bindValue(":jv",0);
                    q1.bindValue(":dv", busiActions[i]->v.getv());
                    q1.bindValue(":dir", DIR_D);
                }
                q1.bindValue(":num", busiActions[i]->num);
                r = q1.exec();
                if(r)
                    busiActions[i]->state = BusiActionData2::INIT;
                break;
            case BusiActionData2::EDITED:
                q2.bindValue(":summary", busiActions[i]->summary);
                q2.bindValue(":fid", busiActions[i]->fid);
                q2.bindValue(":sid", busiActions[i]->sid);
                q2.bindValue(":mt", busiActions[i]->mt);
                if(busiActions[i]->dir == DIR_J){
                    q2.bindValue(":jv", busiActions[i]->v.getv());
                    q2.bindValue(":dv",0);
                    q2.bindValue(":dir", DIR_J);
                }
                else{
                    q2.bindValue(":jv",0);
                    q2.bindValue(":dv", busiActions[i]->v.getv());
                    q2.bindValue(":dir", DIR_D);
                }
                q2.bindValue(":num", busiActions[i]->num);
                q2.bindValue("id", busiActions[i]->id);
                r = q2.exec();
                if(r)
                    busiActions[i]->state = BusiActionData2::INIT;
                break;
            case BusiActionData2::NUMCHANGED:
                q3.bindValue(":num", busiActions[i]->num);
                q3.bindValue(":id", busiActions[i]->id);
                r = q3.exec();
                if(r)
                    busiActions[i]->state = BusiActionData2::INIT;
                break;
            }
        }
        //回读新增的业务活动的id(待需要时再使用此代码)
        if(hasNew){
            r = getActionsInPz2(pid,busiActions);
        }
    }
    if(!dels.isEmpty()){
        for(int i = 0; i < dels.count(); ++i){
            q4.bindValue(":id", dels[i]->id);
            r = q4.exec();
        }
    }
    return r;
}

//删除与指定id的凭证相关的业务活动
bool BusiUtil::delActionsInPz(int pzId)
{
    QString s;
    QSqlQuery q;
    s = QString("delete from BusiActions where pid = %1").arg(pzId);
    bool r = q.exec(s);
    int rows = q.numRowsAffected();
    return r;
}

/**
    生成明细科目日记账数据列表（金额式）
    参数sid：明细科目id，dlist：日记账数据，preExtra：前期余额，curSum：当期合计
*/
bool BusiUtil::getDailyForJe(int y,int m, int fid, int sid,
                   QList<RowTypeForJe*>& dlist, double& preExtra, int& preExtraDir)
{
    QSqlQuery q;
    QString s;

    //获取汇率
    QHash<int,double> rates;
    getRates(y,m,rates);
    rates[RMB] = 1;

    //读取前期余额
    int yy,mm;
    if(m == 12){
        yy = y - 1;
        mm = 1;
    }
    else{
        yy = y;
        mm = m - 1;
    }
    QHash<int,double> preExtras;
    QHash<int,int> preExtraDirs;
    if(sid == 0)  //读取总账期初科目余额
        readExtraForSub(yy,mm,fid,preExtras,preExtraDirs);
    else          //读取明细期初科目余额
        readExtraForDetSub(yy,mm,sid,preExtras,preExtraDirs);
    //合计前期余额
    QHashIterator<int,double> i(preExtras);
    while(i.hasNext()){
        i.next();
        if(preExtraDirs.value(i.key()) == DIR_P)
            continue;
        else if(preExtraDirs.value(i.key()) == DIR_J)
            preExtra += (i.value() * rates.value(i.key()));
        else
            preExtra -= (i.value() * rates.value(i.key()));
    }
    if(preExtra == 0)
        preExtraDir = DIR_P;
    else if(preExtra > 0)
        preExtraDir = DIR_J;
    else
        preExtraDir = DIR_D;

    QString d = QDate(y,m,1).toString(Qt::ISODate);
    d.chop(3);

    if(sid == 0){
        s = QString("select PingZhengs.date, PingZhengs.number, PingZhengs.class, "
                    "BusiActions.summary, BusiActions.jMoney, BusiActions.dMoney, "
                    "BusiActions.moneyType,BusiActions.dir, BusiActions.toTotal, "
                    "BusiActions.toDetails, BusiActions.pid, PingZhengs.accBookGroup, BusiActions.id "
                    "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
                    "where (BusiActions.firSubID = %1) and (PingZhengs.date like '%2%')"
                    "order by PingZhengs.date").arg(fid).arg(d);
    }
    else{
        s = QString("select PingZhengs.date, PingZhengs.number, PingZhengs.class, "
                    "BusiActions.summary, BusiActions.jMoney, BusiActions.dMoney, "
                    "BusiActions.moneyType,BusiActions.dir, BusiActions.toTotal, "
                    "BusiActions.toDetails, BusiActions.pid, PingZhengs.accBookGroup, BusiActions.id "
                    "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
                    "where (BusiActions.secSubID = %1) and (PingZhengs.date like '%2%')"
                    "order by PingZhengs.date").arg(sid).arg(d);
    }

    bool r = q.exec(s);
    QHash<int,QString> bgCls;  //凭证分册类型哈希表
    getPzSClass(bgCls);
    double extra = preExtra; //保存每次发生后的余额，初值等于期初余额
    while(q.next()){
        RowTypeForJe* item = new RowTypeForJe;
        //凭证日期
        QString date= q.value(0).toString();
        QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        item->y = d.year();
        item->m = d.month();
        item->d = d.day();

        //凭证号
        int num = q.value(1).toInt(); //凭证号
        int pc = q.value(2).toInt();  //凭证类别
        item->pzNum = QString("%1%2").arg(bgCls.value(pc)).arg(num);
        //凭证摘要
        QString summary = q.value(3).toString();
        int idx = summary.indexOf("<");
        if(idx != -1){
            summary = summary.left(idx);
        }
        item->summary = summary;
        //借、贷方金额
        int mt = q.value(6).toInt();    //业务发生的币种
        item->dh = q.value(7).toInt();  //业务发生的借贷方向
        if(item->dh == DIR_J)
            item->v = q.value(4).toDouble() * rates.value(mt); //发生在借方
        else
            item->v = q.value(5).toDouble() * rates.value(mt); //发生在贷方

        //余额
        if(item->dh == DIR_J){
            extra += item->v;
            item->em = extra;
        }
        else{
            extra -= item->v;
            item->em = extra;
        }
        //余额方向
        item->pid = q.value(10).toInt();
        item->sid = q.value(12).toInt();
        item->dir = getDirSubExa(extra,item->pid);

        dlist.append(item);
    }

}

/**
    生成明细科目日记账数据列表（外币金额式）
    参数sid：明细科目id，dlist：日记账数据，preExtra：前期余额，curSum：当期合计
*/
bool BusiUtil::getDailyForWj(int y,int m, int fid, int sid, QList<RowTypeForWj*>& dlist,
                             QHash<int,double>& preExtra,QHash<int,int>& preExtraDir)
{
    QSqlQuery q;
    QString s;

    //获取汇率
    QHash<int,double> rates;
    getRates(y,m,rates);
    rates[RMB] = 1;

    //读取前期余额
    int yy,mm;
    if(m == 12){
        yy = y - 1;
        mm = 1;
    }
    else{
        yy = y;
        mm = m - 1;
    }

    if(sid == 0) //读取总账科目余额
        readExtraForSub(yy,mm,fid,preExtra,preExtraDir);
    else
        readExtraForDetSub(yy,mm,sid,preExtra,preExtraDir);
    QHash<int,double> esums = preExtra; //保存每次发生业务活动后的各币种余额，初值就是前期余额
    //合计前期余额
    QHashIterator<int,double> i(preExtra);
    double tsums = 0;  //保存期初总余额及其每次发生后的总余额（各币种合计值）
    while(i.hasNext()){ //计算期初总余额
        i.next();
        if(preExtraDir.value(i.key()) == DIR_P)
            continue;
        else if(preExtraDir.value(i.key()) == DIR_J)
            tsums += (i.value() * rates.value(i.key()));
        else
            tsums -= (i.value() * rates.value(i.key()));
    }


    QString d = QDate(y,m,1).toString(Qt::ISODate);
    d.chop(3);
    if(sid == 0){  //读取总分类日记账数据
        s = QString("select PingZhengs.date,PingZhengs.number,BusiActions.summary,"
                    "BusiActions.id,BusiActions.pid,BusiActions.jMoney,BusiActions.dMoney,"
                    "BusiActions.moneyType,BusiActions.dir "
                    "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
                    "where (BusiActions.firSubID = %1) and (PingZhengs.date like '%2%') "
                    "order by PingZhengs.date").arg(fid).arg(d);
    }
    else{
        s = QString("select PingZhengs.date,PingZhengs.number,BusiActions.summary,"
                    "BusiActions.id,BusiActions.pid,BusiActions.jMoney,BusiActions.dMoney,"
                    "BusiActions.moneyType,BusiActions.dir "
                    "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
                    "where (BusiActions.secSubID = %1) and (PingZhengs.date like '%2%') "
                    "order by PingZhengs.date")
                    .arg(sid).arg(d);
    }

    bool r = q.exec(s);
    while(q.next()){
        RowTypeForWj* item = new RowTypeForWj;
        //凭证日期
        QString date= q.value(0).toString();
        QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        item->y = d.year();
        item->m = d.month();
        item->d = d.day();
        //凭证号
        int num = q.value(1).toInt(); //凭证号        
        item->pzNum = QObject::tr("计%1").arg(num);        
        //凭证摘要
        QString summary = q.value(2).toString();
        int idx = summary.indexOf("<");
        if(idx != -1){
            summary = summary.left(idx);
        }
        item->summary = summary;
        //借、贷方金额
        item->mt = q.value(7).toInt();  //业务发生的币种
        item->dh = q.value(8).toInt();  //业务发生的借贷方向
        if(item->dh == DIR_J)
            item->v = q.value(5).toDouble(); //发生在借方
        else
            item->v = q.value(6).toDouble(); //发生在贷方

        //余额
        if(item->dh == DIR_J){
            tsums += (item->v * rates.value(item->mt));
            esums[item->mt] += item->v;
        }
        else{
            tsums -= (item->v * rates.value(item->mt));
            esums[item->mt] -= item->v;
        }
        //确定总余额的方向和值（余额值始终用正数表示，而在计算时，借方用正数，贷方用负数）
        item->em = esums;  //分币种余额
        if(tsums > 0){
            item->etm = tsums; //各币种合计余额
            item->dir = DIR_J;
        }
        else if(tsums < 0){
            item->etm = -tsums;
            item->dir = DIR_D;
        }
        else{
            item->etm = 0;
            item->dir = DIR_P;
        }
        item->pid = q.value(4).toInt();
        item->bid = q.value(3).toInt(); //这个？？测试目的
        dlist.append(item);
    }

}

//将各币种的余额汇总为用母币计的余额并确定余额方向
//参数 extra，extraDir：余额及其方向，键为币种代码，rate：汇率
//    mExtra，mDir：用母币计的余额值和方向
bool BusiUtil::calExtraAndDir(QHash<int,double> extra,QHash<int,int> extraDir,
                           QHash<int,double> rate,double& mExtra,int& mDir)
{
    mExtra = 0;
    QHashIterator<int,double> i(extra);
    while(i.hasNext()){ //计算期初总余额
        i.next();
        if(extraDir.value(i.key()) == DIR_P)
            continue;
        else if(extraDir.value(i.key()) == DIR_J)
            mExtra += i.value() * rate.value(i.key());
        else
            mExtra -= i.value() * rate.value(i.key());
    }
    if(mExtra == 0)
        mDir = DIR_P;
    else if(mExtra > 0)
        mDir = DIR_J;
    else{
        mDir = DIR_D;
        mExtra = -mExtra;
    }
}

//获取指定月份范围，指定科目（或科目范围）的日记账/明细账数据
//参数--y：年份，sm、em：开始和结束月份，fid、sid：一二级科目id，mt：币种
//     prev：期初总余额（各币种合计），preDir：期初余额方向
//     datas：保存数据,preExtra：前期余额，preExtraDir：前期余额方向
//     rates为每月的汇率，键为月份 * 10 + 币种代码（其中，期初余额是个位数，用币种代码表示）
//     fids：指定的总账科目代码，sids：所有在fids中指定的总账科目所属的明细科目代码总集合
//     gv和lv表示要提取的业务活动的值上下界限，inc：是否包含未记账凭证
bool BusiUtil::getDailyAccount(int y, int sm, int em, int fid, int sid, int mt,
                               double& prev, int& preDir,
                               QList<DailyAccountData*>& datas,
                               QHash<int,double>& preExtra,
                               QHash<int,int>& preExtraDir,
                               QHash<int, double>& rates,
                               QList<int> fids,
                               QHash<int,QList<int> > sids,
                               double gv,double lv,bool inc)
{
    QSqlQuery q;
    QString s;

    //读取前期余额
    int yy,mm;
    if(sm == 1){
        yy = y - 1;
        mm = 12;
    }
    else{
        yy = y;
        mm = sm - 1;
    }

    QHash<int,double> ra;
    QHashIterator<int,double>* it;
    //获取期初汇率并加入汇率表
    getRates(yy,mm,ra);
    ra[RMB] = 1;
    it = new QHashIterator<int,double>(ra);
    while(it->hasNext()){
        it->next();
        rates[it->key()] = it->value();  //期初汇率的键直接是货币代码
    }

    //只有提取指定总账科目的明细发生项的情况下，读取余额才有意义
    if(fid != 0){
        //读取总账科目或明细科目的余额
        if(sid == 0)
            readExtraForSub(yy,mm,fid,preExtra,preExtraDir);
        else
            readExtraForDetSub(yy,mm,sid,preExtra,preExtraDir);
        //如果还指定了币种，则只保留该币种的余额
        if(mt != ALLMT){
            QHashIterator<int,double> it(preExtra);
            while(it.hasNext()){
                it.next();
                if(mt != it.key()){
                    preExtra.remove(it.key());
                    preExtraDir.remove(it.key());
                }
            }
        }
    }


    //保存每次发生业务活动后的各币种余额，初值就是前期余额
    QHash<int,double> esums = preExtra;
    double tsums = 0;  //保存按母币计的期初总余额及其每次发生后的总余额（将各币种用母币合计）
    QHashIterator<int,double> i(preExtra);
    while(i.hasNext()){ //计算期初总余额
        i.next();
        if(preExtraDir.value(i.key()) == DIR_P)
            continue;
        else if(preExtraDir.value(i.key()) == DIR_J)
            tsums += (i.value() * ra.value(i.key()));
        else
            tsums -= (i.value() * ra.value(i.key()));
    }
    if(tsums == 0){
        prev = 0; preDir = DIR_P;
    }
    else if(tsums > 0){
        prev = tsums; preDir = DIR_J;
    }
    else{
        prev = -tsums; preDir = DIR_D;
    }

    //获取指定月份范围的汇率
    ra.clear();
    for(int i = sm; i <= em; ++i){
        getRates(y,i,ra);
        ra[RMB] = 1;
        it = new QHashIterator<int,double>(ra);
        while(it->hasNext()){
            it->next();
            rates[i*10+it->key()] = it->value();
        }
        ra.clear();
    }


    //构造查询语句
    QString sd = QDate(y,sm,1).toString(Qt::ISODate);
    QString ed = QDate(y,em,QDate(y,em,1).daysInMonth()).toString(Qt::ISODate);

    s = QString("select PingZhengs.date,PingZhengs.number,BusiActions.summary,"
                "BusiActions.id,BusiActions.pid,BusiActions.jMoney,BusiActions.dMoney,"
                "BusiActions.moneyType,BusiActions.dir,BusiActions.firSubID,BusiActions.secSubID "
                "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
                "where (PingZhengs.date >= '%1') and (PingZhengs.date <= '%2')")
                .arg(sd).arg(ed);
    if(fid != 0)
        s.append(QString(" and (BusiActions.firSubID = %1)").arg(fid));
    if(sid != 0)
        s.append(QString(" and (BusiActions.secSubID = %1)").arg(sid));
    if(mt != ALLMT)
        s.append(QString(" and (BusiActions.moneyType = %1)").arg(mt));
    if(gv != 0)
        s.append(QString(" and (((BusiActions.dir = %1) and (BusiActions.jMoney > %3)) "
                         "or ((BusiActions.dir = %2) and (BusiActions.dMoney > %3)))")
                 .arg(DIR_J).arg(DIR_D).arg(gv));
    if(lv != 0)
        s.append(QString(" and (((BusiActions.dir = %1) and (BusiActions.jMoney < %3)) "
                         "or ((BusiActions.dir = %2) and (BusiActions.dMoney < %3)))")
                 .arg(DIR_J).arg(DIR_D).arg(lv));
    if(!inc)
        s.append(QString(" and (PingZhengs.pzState = %1)").arg(Pzs_Instat));
    s.append(" order by PingZhengs.date");

    if(!q.exec(s))
        return false;

    //if(!datas.empty())
    //    datas.clear();

    while(q.next()){
        int id = q.value(9).toInt();
        //如果要提取所有选定主目，则过滤掉所有未选定主目
        if(fid == 0){            
            if(!fids.contains(id))
                continue;
        }
        //如果选择所有子目，则过滤掉所有未选定子目
        if(sid == 0){
            int iid = q.value(10).toInt();
            if(!sids.value(id).contains(iid))
                continue;
        }
        DailyAccountData* item = new DailyAccountData;
        //凭证日期
        QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        item->y = d.year();
        item->m = d.month();
        item->d = d.day();
        //凭证号
        int num = q.value(1).toInt(); //凭证号
        item->pzNum = QObject::tr("计%1").arg(num);
        //凭证摘要
        QString summary = q.value(2).toString();
        //如果是现金、银行科目
        //结算号
        //item->jsNum =
        //对方科目
        //item->oppoSub =
        int idx = summary.indexOf("<");
        if(idx != -1){
            summary = summary.left(idx);
        }
        item->summary = summary;
        //借、贷方金额
        item->mt = q.value(7).toInt();  //业务发生的币种
        item->dh = q.value(8).toInt();  //业务发生的借贷方向
        if(item->dh == DIR_J)
            item->v = q.value(5).toDouble(); //发生在借方
        else
            item->v = q.value(6).toDouble(); //发生在贷方

        //余额
        if(item->dh == DIR_J){
            tsums += (item->v * rates.value(item->m*10+item->mt));
            esums[item->mt] += item->v;
        }
        else{
            tsums -= (item->v * rates.value(item->m*10+item->mt));
            esums[item->mt] -= item->v;
        }

        //保存分币种的余额及其方向
        //item->em = esums;  //分币种余额
        it = new QHashIterator<int,double>(esums);
        while(it->hasNext()){
            it->next();
            if(it->value()>0){
                item->em[it->key()] = it->value();
                item->dirs[it->key()] = DIR_J;
            }
            else if(it->value() < 0){
                item->em[it->key()] = -it->value();
                item->dirs[it->key()] = DIR_D;
            }
            else{
                item->em[it->key()] = 0;
                item->dirs[it->key()] = DIR_P;
            }
        }

        //确定总余额的方向和值（余额值始终用正数表示，而在计算时，借方用正数，贷方用负数）
        if(tsums > 0){
            item->etm = tsums; //各币种合计余额
            item->dir = DIR_J;
        }
        else if(tsums < 0){
            item->etm = -tsums;
            item->dir = DIR_D;
        }
        else{
            item->etm = 0;
            item->dir = DIR_P;
        }
        item->pid = q.value(4).toInt();
        item->bid = q.value(3).toInt(); //这个？？测试目的
        datas.append(item);
    }
    return true;
}

//获取指定月份范围，指定科目（或科目范围）的日记账/明细账数据
//参数--y：年份，sm、em：开始和结束月份，fid、sid：一二级科目id，mt：币种
//     prev：期初总余额（各币种按本币合计），preDir：期初余额方向
//     datas：日记账数据,
//     preExtra：原币形式的前期余额，preExtraR：本币形式的前期外币余额
//     preExtraDir：前期余额方向
//     rates为每月的汇率，键为月份 * 10 + 币种代码（其中，期初余额是个位数，用币种代码表示）
//     fids：指定的总账科目代码，
//     sids：所有在fids中指定的总账科目所属的明细科目代码总集合
//     gv和lv表示要提取的业务活动的值上下界限，inc：是否包含未记账凭证
bool BusiUtil::getDailyAccount2(int y, int sm, int em, int fid, int sid, int mt,
                                Double &prev, int &preDir,
                                QList<DailyAccountData2 *> &datas,
                                QHash<int, Double> &preExtra,
                                QHash<int,Double>& preExtraR,
                                QHash<int, int> &preExtraDir,
                                QHash<int, Double> &rates,
                                QList<int> fids, QHash<int, QList<int> > sids,
                                Double gv, Double lv, bool inc)
{
    QSqlQuery q;
    QString s;

    //读取前期余额
    int yy,mm;
    if(sm == 1){
        yy = y - 1;
        mm = 12;
    }
    else{
        yy = y;
        mm = sm - 1;
    }

    QHash<int,Double> ra;
    QHashIterator<int,Double>* it;

    //获取期初汇率并加入汇率表
    if(!getRates2(yy,mm,ra))
        return false;
    ra[RMB] = 1.00;
    it = new QHashIterator<int,Double>(ra);
    while(it->hasNext()){
        it->next();
        rates[it->key()] = it->value();  //期初汇率的键直接是货币代码
    }

    //只有提取指定总账科目的明细发生项的情况下，读取余额才有意义
    if(fid != 0){
        //读取总账科目或明细科目的余额
        if(sid == 0)
            readExtraForSub2(yy,mm,fid,preExtra,preExtraR,preExtraDir);
        else
            readExtraForDetSub2(yy,mm,sid,preExtra,preExtraR,preExtraDir);
        //如果还指定了币种，则只保留该币种的余额
        if(mt != ALLMT){
            QHashIterator<int,Double> it(preExtra);
            while(it.hasNext()){
                it.next();
                if(mt != it.key()){
                    preExtra.remove(it.key());
                    preExtraDir.remove(it.key());
                }
            }
        }
    }


    //以原币形式保存每次发生业务活动后的各币种余额，初值就是前期余额（还要根据前期余额的方向调整符号）
    QHash<int,Double> esums;
    it = new QHashIterator<int,Double>(preExtra);
    while(it->hasNext()){
        it->next();
        Double v = it->value();
        if(preExtraDir.value(it->key()) == DIR_D)
            v.changeSign();
        esums[it->key()] = v;
    }

    //计算期初总余额
    Double tsums = 0.00;  //保存按母币计的期初总余额及其每次发生后的总余额（将各币种用母币合计）
    QHashIterator<int,Double> i(preExtra);
    while(i.hasNext()){ //计算期初总余额
        i.next();
        int mt = i.key() % 10;
        if(preExtraDir.value(i.key()) == DIR_P)
            continue;
        else if(preExtraDir.value(i.key()) == DIR_J){
            //tsums += (i.value() * ra.value(i.key()));
            if(mt == RMB)
                tsums += i.value();
            else
                tsums += preExtraR.value(it->key());
        }
        else{
            //tsums -= (i.value() * ra.value(i.key()));
            if(mt == RMB)
                tsums -= i.value();
            else
                tsums -= preExtraR.value(it->key());
        }
    }
    if(tsums == 0){
        prev = 0.00; preDir = DIR_P;
    }
    else if(tsums > 0){
        prev = tsums; preDir = DIR_J;
    }
    else{
        prev = tsums;
        prev.changeSign();
        preDir = DIR_D;
    }

    //获取指定月份范围的汇率
    ra.clear();
    for(int i = sm; i <= em; ++i){
        getRates2(y,i,ra);
        ra[RMB] = 1.00;
        it = new QHashIterator<int,Double>(ra);
        while(it->hasNext()){
            it->next();
            rates[i*10+it->key()] = it->value();
        }
        ra.clear();
    }


    //构造查询语句
    QString sd = QDate(y,sm,1).toString(Qt::ISODate);
    QString ed = QDate(y,em,QDate(y,em,1).daysInMonth()).toString(Qt::ISODate);

    s = QString("select PingZhengs.date,PingZhengs.number,BusiActions.summary,"
                "BusiActions.id,BusiActions.pid,BusiActions.jMoney,BusiActions.dMoney,"
                "BusiActions.moneyType,BusiActions.dir,BusiActions.firSubID,"
                "BusiActions.secSubID,PingZhengs.isForward "
                "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
                "where (PingZhengs.date >= '%1') and (PingZhengs.date <= '%2')")
                .arg(sd).arg(ed);
    if(fid != 0)
        s.append(QString(" and (BusiActions.firSubID = %1)").arg(fid));
    if(sid != 0)
        s.append(QString(" and (BusiActions.secSubID = %1)").arg(sid));
    //if(mt != ALLMT)
    //    s.append(QString(" and (BusiActions.moneyType = %1)").arg(mt));
    if(gv != 0)
        s.append(QString(" and (((BusiActions.dir = %1) and (BusiActions.jMoney > %3)) "
                         "or ((BusiActions.dir = %2) and (BusiActions.dMoney > %3)))")
                 .arg(DIR_J).arg(DIR_D).arg(gv.toString()));
    if(lv != 0)
        s.append(QString(" and (((BusiActions.dir = %1) and (BusiActions.jMoney < %3)) "
                         "or ((BusiActions.dir = %2) and (BusiActions.dMoney < %3)))")
                 .arg(DIR_J).arg(DIR_D).arg(lv.toString()));
    if(!inc) //将已入账的凭证纳入统计范围
        s.append(QString(" and (PingZhengs.pzState = %1)").arg(Pzs_Instat));
    else     //将未审核、已审核、已入账的凭证纳入统计范围
        s.append(QString(" and (PingZhengs.pzState != %1)").arg(Pzs_Repeal));
    s.append(" order by PingZhengs.date");

    if(!q.exec(s))
        return false;

    int mType,pzCls,id,fsubId;
    int cwfyId; //财务费用的科目id
    getIdByCode(cwfyId,"5503");

    while(q.next()){
        id = q.value(9).toInt();
        mType = q.value(7).toInt();  //业务发生的币种
        pzCls = q.value(11).toInt();    //凭证类别
        fsubId = q.value(9).toInt();    //会计分录所涉及的一级科目id
        //如果要提取所有选定主目，则跳过所有未选定主目
        if(fid == 0){
            if(!fids.contains(id))
                continue;
        }
        //如果选择所有子目，则过滤掉所有未选定子目
        if(sid == 0){
            int iid = q.value(10).toInt();
            if(!sids.value(id).contains(iid))
                continue;
        }

        //当前凭证是否是结转汇兑损益的凭证
        bool isJzhdPz = pzCls == Pzc_Jzhd_Bank || pzCls == Pzc_Jzhd_Ys
                        || pzCls == Pzc_Jzhd_Yf;
        //如果是结转汇兑损益的凭证作特别处理
        if(isJzhdPz){
            if(mt == RMB && fid != cwfyId) //如果指定的是人民币，则跳过非财务费用方的会计分录
                continue;
        }

        //对于非结转汇兑损益的凭证，如果指定了币种，则跳过非此币种的会计分录
        if((mt != 0 && mt != mType && !isJzhdPz))
            continue;

        DailyAccountData2* item = new DailyAccountData2;
        //凭证日期
        QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        item->y = d.year();
        item->m = d.month();
        item->d = d.day();
        //凭证号
        int num = q.value(1).toInt(); //凭证号
        item->pzNum = QObject::tr("计%1").arg(num);
        //凭证摘要
        QString summary = q.value(2).toString();
        //如果是现金、银行科目
        //结算号
        //item->jsNum =
        //对方科目
        //item->oppoSub =
        int idx = summary.indexOf("<");
        if(idx != -1){
            summary = summary.left(idx);
        }
        item->summary = summary;
        //借、贷方金额
        item->mt = q.value(7).toInt();  //业务发生的币种
        item->dh = q.value(8).toInt();  //业务发生的借贷方向
        if(item->dh == DIR_J)
            item->v = Double(q.value(5).toDouble()); //发生在借方
        else
            item->v = Double(q.value(6).toDouble()); //发生在贷方

        //余额
        if(item->dh == DIR_J){
            tsums += (item->v * rates.value(item->m*10+item->mt));
            esums[item->mt] += item->v;
        }
        else{
            tsums -= (item->v * rates.value(item->m*10+item->mt));
            esums[item->mt] -= item->v;
        }

        //保存分币种的余额及其方向
        //item->em = esums;  //分币种余额
        it = new QHashIterator<int,Double>(esums);
        while(it->hasNext()){
            it->next();
            if(it->value()>0){
                item->em[it->key()] = it->value();
                item->dirs[it->key()] = DIR_J;
            }
            else if(it->value() < 0.00){
                item->em[it->key()] = it->value();
                item->em[it->key()].changeSign();
                item->dirs[it->key()] = DIR_D;
            }
            else{
                item->em[it->key()] = 0;
                item->dirs[it->key()] = DIR_P;
            }
        }

        //确定总余额的方向和值（余额值始终用正数表示，而在计算时，借方用正数，贷方用负数）
        if(tsums > 0){
            item->etm = tsums; //各币种合计余额
            item->dir = DIR_J;
        }
        else if(tsums < 0){
            item->etm = tsums;
            item->etm.changeSign();
            item->dir = DIR_D;
        }
        else{
            item->etm = 0;
            item->dir = DIR_P;
        }
        item->pid = q.value(4).toInt();
        item->bid = q.value(3).toInt(); //这个？？测试目的
        datas.append(item);
    }
    return true;
}


//获取指定月份范围，指定总账科目的总账数据
//参数 y：帐套年份，sm，em：开始和结束月份，fid：总账科目id，datas：读取的数据
//    preExtra：期初余额，preExtraDir：期初余额方向，
//rate：每月的汇率，键为月份 * 10 + 币种代码（期初汇率的月份代码为0，因此它的键为币种代码-个位数）
//注意：返回的数据列表的第一条记录是上年或上月结转的余额
bool BusiUtil::getTotalAccount(int y, int sm, int em, int fid,
                            QList<TotalAccountData*>& datas,
                            QHash<int,double>& preExtra,
                            QHash<int,int>& preExtraDir,
                            QHash<int, double>& rates)
{
    QSqlQuery q;
    QString s;

    //读取前期余额
    int yy,mm;
    if(sm == 1){
        yy = y - 1;
        mm = 12;
    }
    else{
        yy = y;
        mm = sm - 1;
    }

    QHash<int,double> ra,preRa; //期初汇率
    QHashIterator<int,double>* it;

    //获取期初汇率
    getRates(yy,mm,preRa);
    preRa[RMB] = 1;
    it = new QHashIterator<int,double>(preRa);
    while(it->hasNext()){
        it->next();
        rates[it->key()] = it->value();
    }
    //获取指定月份范围的汇率
    for(int i = sm; i <= em; ++i){
        getRates(y,i,ra);
        ra[RMB] = 1;
        it = new QHashIterator<int,double>(ra);
        while(it->hasNext()){
            it->next();
            rates[i*10+it->key()] = it->value();
        }
        ra.clear();
    }

    //读取总账科目余额
    readExtraForSub(yy,mm,fid,preExtra,preExtraDir);    
    double extra;  //期初余额（用母币计），兼做每月期末余额
    int edir;      //前期余额方向（用母币计），兼做每月期末余额方向
    QHash<int,double> extras; //期初余额（依币种分开计），兼做每月期末余额（这个哈希表用正数表示借方，负数表示贷方）
    //计算用母币计的期初余额与方向
    calExtraAndDir(preExtra,preExtraDir,preRa,extra,edir);

    //调整数值，正数表示借方，负数表示贷方
    it = new QHashIterator<int,double>(preExtra);
    while(it->hasNext()){
        it->next();
        if(preExtraDir.value(it->key()) == DIR_D) //如果余额是贷方，则用负数表示，以便后续累加
            extras[it->key()] = -it->value();
        else
            extras[it->key()] = it->value();
    }

    QString sd = QDate(y,sm,1).toString(Qt::ISODate);
    QString ed = QDate(y,em,QDate(y,em,1).daysInMonth()).toString(Qt::ISODate);
    s = QString("select PingZhengs.date,BusiActions.jMoney,BusiActions.dMoney,"
                "BusiActions.moneyType,BusiActions.dir "
                "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
                "where (BusiActions.firSubID = %1) and (PingZhengs.date >= '%2') "
                "and (PingZhengs.date <= '%3') order by PingZhengs.date")
            .arg(fid).arg(sd).arg(ed);
    if(!q.exec(s))
        return false;

    TotalAccountData* item;

    //这个项目对应于上年结转或上月结转
    item = new TotalAccountData;
    item->y = yy;
    item->m = mm;
    item->evh = preExtra;
    item->ev = extra;
    item->dir = edir;
    datas<<item;

    int om = 0,cm; //前期和当期月份
    QString date;
    int dir,mt;    //业务活动发生的方向和币种
    double sumjm = 0,sumdm = 0;    //本月借贷合计（用母币计）
    //double sumjy = 0,sumdy = 0;    //本年借贷合计（用母币计）
    QHash<int,double> sumjmh,sumdmh; //本月借贷合计
    //QHash<int,double> sumjyh,sumdyh; //本年借贷合计
    double v;
    while(q.next()){
        date = q.value(0).toString();
        cm = date.mid(5,2).toInt();
        if((om!=0)&&(om!=cm)){ //如果跨月，则需将前一个月的数据项保存到列表
            item->y = y;
            item->m = om;
            item->jvh = sumjmh;
            item->jv = sumjm;
            item->dvh = sumdmh;
            item->dv = sumdm;
            //item->evh = extras;
            //各币种的余额及其方向
            it = new QHashIterator<int,double>(extras);
            while(it->hasNext()){
                it->next();
                if(it->value() == 0){
                    item->evh[it->key()] = 0;
                    item->dirs[it->key()] = DIR_P;
                }
                else if(it->value() > 0){
                    item->evh[it->key()] = it->value();
                    item->dirs[it->key()] = DIR_J;
                }
                else{
                    item->evh[it->key()] = -it->value();
                    item->dirs[it->key()] = DIR_D;
                }
            }
            //用母币计的余额及其方向
            if(extra == 0){
                item->ev = 0;
                item->dir = DIR_P;
            }
            if(extra < 0){
                item->ev = -extra;
                item->dir = DIR_D;
            }
            else{
                item->ev = extra;
                item->dir = DIR_J;
            }
            datas<<item;
            sumjmh.clear();
            sumdmh.clear();
            sumjm = 0;
            sumdm = 0;
        }
        if(om!=cm){
            item = new TotalAccountData;
            om = cm;
        }
        dir = q.value(4).toInt();
        mt = q.value(3).toInt();        
        if(dir == DIR_J){
            v = q.value(1).toDouble();
            sumjmh[mt] += v;
            sumjm += v*rates.value(cm*10+mt);
            extras[mt] += v;
            extra += v*rates.value(cm*10+mt);
        }
        else{
            v = q.value(2).toDouble();
            sumdmh[mt] += v;
            sumdm += v*rates.value(cm*10+mt);
            extras[mt] -= v;
            extra -= v*rates.value(cm*10+mt);
        }
    }

    //将最后一月的数据保存到列表
    item->y = y;
    item->m = om;
    item->jvh = sumjmh;
    item->jv = sumjm;
    item->dvh = sumdmh;
    item->dv = sumdm;
    //各币种的余额及其方向
    it = new QHashIterator<int,double>(extras);
    while(it->hasNext()){
        it->next();
        if(it->value() == 0){
            item->evh[it->key()] = 0;
            item->dirs[it->key()] = DIR_P;
        }
        else if(it->value() > 0){
            item->evh[it->key()] = it->value();
            item->dirs[it->key()] = DIR_J;
        }
        else{
            item->evh[it->key()] = -it->value();
            item->dirs[it->key()] = DIR_D;
        }
    }
    //用母币计的余额及其方向
    if(extra == 0){
        item->ev = 0;
        item->dir = DIR_P;
    }
    if(extra < 0){
        item->ev = -extra;
        item->dir = DIR_D;
    }
    else{
        item->ev = extra;
        item->dir = DIR_J;
    }
    datas<<item;
    return true;
}

/**
    生成欲打印凭证的数据集合
    参数：y、m 凭证所属年月
         datas 生成的凭证数据集
         pznSet 要打印的凭证号集合
*/
bool BusiUtil::genPzPrintDatas(int y, int m, QList<PzPrintData*> &datas, QSet<int> pznSet)
{
    QSqlQuery q,q2;
    QString s;

    //获取所有一级、二级科目的名称表和用户名称表
    QHash<int,QString> fsub,ssub,slsub,users;
    getAllSubFName(fsub);    //一级科目名
    getAllSubSLName(slsub); //二级科目全名
    getAllSubSName(ssub);   //二级科目简名
    getAllUser(users);      //用户名
    QHash<int,double> rates;
    getRates(y,m,rates);    //汇率
    rates[RMB] = 1;

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select * from PingZhengs where date like '%1%' "
                "order by number").arg(ds);
    bool r = q.exec(s);
    while(q.next()){
        int pzNum = q.value(PZ_NUMBER).toInt();   //凭证号
        if((pznSet.count() == 0) || pznSet.contains(pzNum)){
            //获取该凭证的业务活动数
            int pid = q.value(PZ_ID).toInt();
            s = QString("select count() from BusiActions where pid = %1").arg(pid);
            r = q2.exec(s);
            r = q2.first();
            int bac = q2.value(0).toInt(); //该凭证包含的业务活动数
            //计算将此凭证完全打印出来需要几张凭证
            int pzc;
            if((bac % MAXROWS) == 0)
                pzc = bac / MAXROWS;
            else
                pzc = bac / MAXROWS + 1;

            //获取业务活动
            s = QString("select * from BusiActions where pid = %1").arg(pid);
            r = q2.exec(s);

            double jsum = 0; double dsum = 0; //借贷合计值
            for(int i = 0; i < pzc; ++i){
                PzPrintData* pd = new PzPrintData;
                pd->date = q.value(PZ_DATE).toDate();     //凭证日期
                pd->attNums = q.value(PZ_ENCNUM).toInt(); //附件数
                if(pzc == 1)
                    pd->pzNum = QString::number(pzNum);
                else{
                    pd->pzNum = QString::number(pzNum) + '-' + QString("%1/%2").arg(i+1).arg(pzc);
                }
                QList<BaData*> baList;
                int num = 0; //已提取的业务活动数

                while((num < MAXROWS) && (q2.next())){
                    num++;
                    BaData* bd = new BaData;
                    QString summary = q2.value(BACTION_SUMMARY).toString();
                    int idx = summary.indexOf('<');
                    if(idx != -1)
                        summary.chop(summary.count()- idx);
                    bd->summary = summary;
                    bd->dir = q2.value(BACTION_DIR).toInt();
                    bd->mt = q2.value(BACTION_MTYPE).toInt();
                    if(bd->dir == DIR_J){
                        bd->v = q2.value(BACTION_JMONEY).toDouble();
                        jsum += bd->v * rates.value(bd->mt);
                    }
                    else{
                        bd->v = q2.value(BACTION_DMONEY).toDouble();
                        dsum += bd->v * rates.value(bd->mt);
                    }
                    int fid = q2.value(BACTION_FID).toInt();
                    int sid = q2.value(BACTION_SID).toInt();
                    if(slsub.value(sid) == "") //如果二级科目没有全名，则用简名代替
                        bd->subject = fsub.value(fid) + QObject::tr("——") + ssub.value(sid);
                    else
                        bd->subject = fsub.value(fid) + QObject::tr("——") + slsub.value(sid);
                    baList.append(bd);
                }
                pd->baLst = baList;
                pd->jsum = jsum;
                pd->dsum = dsum;
                pd->producer = q.value(PZ_RUSER).toInt();    //制单者
                pd->verify = q.value(PZ_VUSER).toInt();      //审核者
                pd->bookKeeper = q.value(PZ_BUSER).toInt();  //记账者
                //pd->producer = allUsers.value(q2.value(PZ_RUSER).toInt())->getName();  //制单者
                //pd->verify = allUsers.value(q2.value(PZ_VUSER).toInt())->getName();    //审核者
                //pd->bookKeeper = allUsers.value(q2.value(PZ_BUSER).toInt())->getName();//记账者
                datas.append(pd);
            }
        }
    }
}

bool BusiUtil::genPzPrintDatas2(int y, int m, QList<PzPrintData2 *> &datas, QSet<int> pznSet)
{
    QSqlQuery q,q2;
    QString s;

    //获取所有一级、二级科目的名称表和用户名称表
    QHash<int,QString> fsub,ssub,slsub,users;
    getAllSubFName(fsub);    //一级科目名
    getAllSubSLName(slsub); //二级科目全名
    getAllSubSName(ssub);   //二级科目简名
    getAllUser(users);      //用户名
    QHash<int,Double> rates;
    getRates2(y,m,rates);    //汇率
    rates[RMB] = 1.00;

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select * from PingZhengs where date like '%1%' "
                "order by number").arg(ds);
    bool r = q.exec(s);
    while(q.next()){
        int pzNum = q.value(PZ_NUMBER).toInt();   //凭证号
        if((pznSet.count() == 0) || pznSet.contains(pzNum)){
            //获取该凭证的业务活动数
            int pid = q.value(PZ_ID).toInt();
            s = QString("select count() from BusiActions where pid = %1").arg(pid);
            if(!q2.exec(s))
                return false;
            q2.first();
            int bac = q2.value(0).toInt(); //该凭证包含的业务活动数
            //计算将此凭证完全打印出来需要几张凭证
            int pzc;
            if((bac % MAXROWS) == 0)
                pzc = bac / MAXROWS;
            else
                pzc = bac / MAXROWS + 1;

            //获取业务活动
            s = QString("select * from BusiActions where pid = %1").arg(pid);
            if(!q2.exec(s))
                return false;

            Double jsum = 0.00,dsum = 0.00; //借贷合计值
            for(int i = 0; i < pzc; ++i){
                PzPrintData2* pd = new PzPrintData2;
                pd->date = q.value(PZ_DATE).toDate();     //凭证日期
                pd->attNums = q.value(PZ_ENCNUM).toInt(); //附件数
                if(pzc == 1)
                    pd->pzNum = QString::number(pzNum);
                else{
                    pd->pzNum = QString::number(pzNum) + '-' + QString("%1/%2").arg(i+1).arg(pzc);
                }
                QList<BaData2*> baList;
                int num = 0; //已提取的业务活动数

                while((num < MAXROWS) && (q2.next())){
                    num++;
                    BaData2* bd = new BaData2;
                    QString summary = q2.value(BACTION_SUMMARY).toString();
                    int idx = summary.indexOf('<');
                    if(idx != -1)
                        summary.chop(summary.count()- idx);
                    bd->summary = summary;
                    bd->dir = q2.value(BACTION_DIR).toInt();
                    bd->mt = q2.value(BACTION_MTYPE).toInt();
                    if(bd->dir == DIR_J){
                        bd->v = Double(q2.value(BACTION_JMONEY).toDouble());
                        jsum += bd->v * rates.value(bd->mt);
                    }
                    else{
                        bd->v = Double(q2.value(BACTION_DMONEY).toDouble());
                        dsum += bd->v * rates.value(bd->mt);
                    }
                    int fid = q2.value(BACTION_FID).toInt();
                    int sid = q2.value(BACTION_SID).toInt();
                    if(slsub.value(sid) == "") //如果二级科目没有全名，则用简名代替
                        bd->subject = fsub.value(fid) + QObject::tr("——") + ssub.value(sid);
                    else
                        bd->subject = fsub.value(fid) + QObject::tr("——") + slsub.value(sid);
                    baList.append(bd);
                }
                pd->baLst = baList;
                pd->jsum = jsum;
                pd->dsum = dsum;
                pd->producer = q.value(PZ_RUSER).toInt();    //制单者
                pd->verify = q.value(PZ_VUSER).toInt();      //审核者
                pd->bookKeeper = q.value(PZ_BUSER).toInt();  //记账者
                //pd->producer = allUsers.value(q2.value(PZ_RUSER).toInt())->getName();  //制单者
                //pd->verify = allUsers.value(q2.value(PZ_VUSER).toInt())->getName();    //审核者
                //pd->bookKeeper = allUsers.value(q2.value(PZ_BUSER).toInt())->getName();//记账者
                datas.append(pd);
            }
        }
    }
    return true;
}

/**
    结转损益（将损益类科目结转至本年利润）
*/
bool BusiUtil::genForwordPl(int y, int m, User *user)
{
    //直接从科目余额表中读取损益类科目的余额数进行结转，因此在执行此操作之前必须在打开凭证集后，
    //并且处于结转损益前的准备状态
    QSqlQuery q;
    QString s;
    bool r = 0;    

    //因为在主窗口调用此函数时已经检测了凭证集的状态，因此这里忽略凭证集状态检测
    QHash<int,double>rates;
    if(!getRates(y,m,rates)){
        qDebug() << QObject::tr("不能获取%1年%2月的汇率").arg(y).arg(m);
        return false;
    }
    rates[RMB] = 1;

    //基本步骤：
    //1、读取科目余额
    QHash<int,double>extra,extraDet; //总账科目和明细账科目余额
    QHash<int,int>extraDir,extraDetDir; //总账科目和明细账科目余额方向
    if(!readExtraByMonth(y,m,extra,extraDir,extraDet,extraDetDir)){
        qDebug() << "Don't save subject extra !";
        return false;
    }

    //2、创建结转凭证。
    //在创建结转凭证前要检测是否以前已经进行过结转，如有则要先删除与此两凭证相关的所有业务活动
    //并且重用凭证表的对应记录
    int idNewIn;    //结转收入的凭证id
    int idNewFei;   //结转费用的凭证id
    int pzNum;  //结转凭证所使用的凭证号（结转收入和结转费用两张凭证号是紧挨的，且收入在前费用在后）

    QDate pzDate(y,m,1); //凭证日期为当前打开凭证集的最后日期
    pzDate.setDate(y,m,pzDate.daysInMonth());
    QString strPzDate = pzDate.toString(Qt::ISODate);    

    //创建新的或读取现存的结转收入类凭证id值
    s = QString("select id,number from PingZhengs where (isForward = %1) and "
                "(date='%2')").arg(Pzc_JzsyIn).arg(strPzDate);
    if(q.exec(s) && q.first()){        
        idNewIn = q.value(0).toInt();
        pzNum = q.value(1).toInt();
        s = QString("delete from BusiActions where pid = %1").arg(idNewIn);
        r = q.exec(s); //删除原先属于此凭证的所有业务活动
        //恢复凭证状态到初始录入态
        s = QString("update PingZhengs set pzState=%1,ruid=%2 where id = %3")
                .arg(Pzs_Recording).arg(user->getUserId()).arg(idNewIn);
        r = q.exec(s);
    }
    else{
        //创建新凭证之前，先获取凭证集内可用的最大凭证号
        pzNum = getMaxPzNum(y,m);        
        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
                    "values('%1',%2,%3,%4,%5)").arg(strPzDate).arg(pzNum)
                .arg(Pzc_JzsyIn).arg(Pzs_Recording).arg(user->getUserId());
        r = q.exec(s);
        //回读此凭证的id
        s = QString("select id from PingZhengs where (isForward = %1) and "
                    "(date='%2')").arg(Pzc_JzsyIn).arg(strPzDate);
        r = q.exec(s); r = q.first();
        idNewIn = q.value(0).toInt();
    }

    //创建新的或读取现存的结转费用类凭证id值
    s = QString("select id,number from PingZhengs where (isForward = %1) and "
                "(date='%2')").arg(Pzc_JzsyFei).arg(strPzDate);
    pzNum++;
    if(q.exec(s) && q.first()){
        idNewFei = q.value(0).toInt();
        s = QString("delete from BusiActions where pid = %1").arg(idNewFei);
        r = q.exec(s); //删除原先属于此凭证的所有业务活动
        //恢复凭证状态到初始录入态
        s = QString("update PingZhengs set pzState=%1,ruid=%2 where id = %3")
                .arg(Pzs_Recording).arg(user->getUserId()).arg(idNewFei);
        r = q.exec(s);
    }
    else{
        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
                    "values('%1',%2,%3,%4,%5)").arg(strPzDate).arg(pzNum)
                .arg(Pzc_JzsyFei).arg(Pzs_Recording).arg(user->getUserId());
        r = q.exec(s);
        //回读此凭证的id
        s = QString("select id from PingZhengs where (isForward = %1) and "
                    "(date='%2')").arg(Pzc_JzsyFei).arg(strPzDate);
        r = q.exec(s); r = q.first();
        idNewFei = q.value(0).toInt();
    }

    //3、填充结转凭证的业务活动数据
    //获取“本年利润-结转”子账户-的id
    int bnlrId;
    if(!getIdByName(bnlrId, QObject::tr("本年利润"))){
        qDebug() << QObject::tr("不能获取本年利润科目的id值");
        return false;
    }
    s = QString("select FSAgent.id from FSAgent join SecSubjects "
                "where (FSAgent.sid = SecSubjects.id) and "
                "(FSAgent.fid = %1) and (SecSubjects.subName = '%2')")
            .arg(bnlrId).arg(QObject::tr("结转"));
    if(!q.exec(s) || !q.first()){
        qDebug() << QObject::tr("不能获取”本年利润-结转“子账户");
        return false;
    }    
    int bnlrSid = q.value(0).toInt();  //本年利润--结转子账户id

    QList<BusiActionData*> fad,iad;      //费用和收入结转凭证的业务活动数据列表
    QHash<int, QList<int> > fids,iids;   //费用类和收入类的明细科目id集合，键为一级科目id，值为该一级科目下的子目id集合
    BusiUtil::getAllIdForSy(true,iids);  //获取所有收入类总账-明细id列表
    BusiUtil::getAllIdForSy(false,fids); //获取所有费用类总账-明细id列表

    QList<int> fidLst = fids.keys();     //费用类一级科目id的列表（按id的大小顺序排列，应该是以科目代码顺序排列，
    qSort(fidLst.begin(),fidLst.end());  //这里简略实现，使用此列表只是为了生成的业务活动按预定的顺序）
    QList<int> iidLst = iids.keys();     //收入类一级科目id的列表（按id的大小顺序排列，应该是以科目代码顺序排列）
    qSort(iidLst.begin(),iidLst.end());
    QHash<int,QString> mtHash;
    getMTName(mtHash);
    QList<int> mts = mtHash.keys();      //所有币种代码列表

    int sid,key;  //二级科目id、查询余额的键
    BusiActionData *bd1;
    int num = 1; //业务活动在凭证内的序号

    double iv = 0, fv = 0; //结转凭证的借贷合计值
    SubjectManager* sm = curAccount->getSubjectManager();

    //结转收入类凭证的业务活动列表
    for(int i = 0; i < iidLst.count(); ++i)
        for(int j = 0; j < iids.value(iidLst[i]).count(); ++j)
            for(int k = 0; k < mts.count(); ++k){
                sid = iids.value(iidLst[i])[j];
                key = sid * 10 + mts[k];
                if(extraDet.contains(key) && extraDet.value(key) != 0){
                    bd1 = new BusiActionData;
                    bd1->state = BusiActionData::NEW; //新业务活动
                    bd1->num = num++;
                    bd1->pid = idNewIn;
                    bd1->fid = iidLst[i];
                    bd1->sid = sid;
                    bd1->summary = QObject::tr("结转（%1-%2）至本年利润")
                            .arg(sm->getFstSubName(iidLst[i]))
                            .arg(sm->getSndSubName(sid));
                    bd1->mt = mts[k];
                    //结转收入类到本年利润，一般是损益类科目放在借方，本年利润放在贷方
                    //因此，如果损益类科目余额是借方时，把它放在借方，并用负数表示。
                    bd1->dir = DIR_J;
                    if(extraDetDir.value(key) == DIR_J)
                        bd1->v = -extraDet.value(key);
                    else
                        bd1->v = extraDet.value(key);
                    iad.append(bd1);
                    iv += rates.value(bd1->mt) * bd1->v;
                }
            }
    //创建本年利润-结转的借方会计分录
    bd1 = new BusiActionData;
    bd1->state = BusiActionData::NEW; //新业务活动
    bd1->num = num++;
    bd1->pid = idNewIn;
    bd1->fid = bnlrId;
    bd1->sid = bnlrSid;
    bd1->summary = QObject::tr("结转收入至本年利润");
    bd1->mt = RMB;                     //币种
    bd1->dir = DIR_D;
    bd1->v = iv;
    iad.append(bd1);

    //结转费用类凭证的业务活动列表
    //SubjectManager* sm = curAccount->getSubjectManager();
    num = 1;
    for(int i = 0; i < fidLst.count(); ++i)
        for(int j = 0; j < fids.value(fidLst[i]).count(); ++j)
            for(int k = 0; k < mts.count(); ++k){
                sid = fids.value(fidLst[i])[j];
                key = sid * 10 + mts[k];
                if(extraDet.contains(key) && extraDet.value(key) != 0){
                    bd1 = new BusiActionData;
                    bd1->state = BusiActionData::NEW; //新业务活动
                    bd1->num = num++;
                    bd1->pid = idNewFei;
                    bd1->fid = fidLst[i];
                    bd1->sid = sid;
                    bd1->summary = QObject::tr("结转（%1-%2）至本年利润")
                            .arg(sm->getFstSubName(fidLst[i]))
                            .arg(sm->getSndSubName(sid));
                    bd1->mt = mts[k];
                    //结转费用类到本年利润，一般是损益类科目放在贷方，本年利润方在借方
                    //因此，如果损益类科目余额是贷方时，把它放在贷方，并用负数表示。
                    bd1->dir = DIR_D;
                    if(extraDetDir.value(key) == DIR_D)
                        bd1->v = -extraDet.value(key);
                    else
                        bd1->v = extraDet.value(key);
                    fad.append(bd1);
                    fv += rates.value(bd1->mt) * bd1->v;
                }
            }

    //创建本年利润-结转的借方会计分录
    bd1 = new BusiActionData;
    bd1->state = BusiActionData::NEW; //新业务活动
    bd1->num = num++;
    bd1->pid = idNewFei;
    bd1->fid = bnlrId;
    bd1->sid = bnlrSid;
    bd1->summary = QObject::tr("结转费用至本年利润");
    bd1->mt = RMB;                        //币种
    bd1->dir = DIR_J;
    bd1->v = fv;
    fad.append(bd1);

    //保存业务活动数据
    saveActionsInPz(idNewIn,iad);
    saveActionsInPz(idNewFei,fad);
    //更新合计值
    s = QString("update PingZhengs set jsum = %1,dsum = %2 where id = %3")
            .arg(iv).arg(iv).arg(idNewIn);
    r = q.exec(s);
    s = QString("update PingZhengs set jsum = %1,dsum = %2 where id = %3")
            .arg(fv).arg(fv).arg(idNewFei);
    r = q.exec(s);

    setPzsState(y,m,Ps_Jzsy);
}

/**
    结转损益（将损益类科目结转至本年利润）
*/
bool BusiUtil::genForwordPl2(int y, int m, User *user)
{
    //直接从科目余额表中读取损益类科目的余额数进行结转，因此在执行此操作之前必须在打开凭证集后，
    //并且处于结转损益前的准备状态
    QSqlQuery q;
    QString s;
    bool r = 0;

    //因为在主窗口调用此函数时已经检测了凭证集的状态，因此这里忽略凭证集状态检测
    //结转损益类科目，不会涉及到外币，因此用不着使用汇率
    QHash<int,Double>rates;
    if(!getRates2(y,m,rates)){
        qDebug() << QObject::tr("不能获取%1年%2月的汇率").arg(y).arg(m);
        return false;
    }
    rates[RMB] = Double(1.00);

    //基本步骤：
    //1、读取科目余额
    QHash<int,Double>extra,extraDet; //总账科目和明细账科目余额
    QHash<int,int>extraDir,extraDetDir; //总账科目和明细账科目余额方向
    if(!readExtraByMonth2(y,m,extra,extraDir,extraDet,extraDetDir)){
        qDebug() << "Don't read subject extra !";
        return false;
    }

    //查看财务费用-汇兑损益的余额
    qDebug()<<QObject::tr("财务费用-汇兑损益的余额： %1").arg(extraDet.value(601).toString());

    //2、创建结转凭证。
    //在创建结转凭证前要检测是否以前已经进行过结转，如有则要先删除与此两凭证相关的所有业务活动
    //并且重用凭证表的对应记录
    int idNewIn;    //结转收入的凭证id
    int idNewFei;   //结转费用的凭证id
    int pzNum;  //结转凭证所使用的凭证号（结转收入和结转费用两张凭证号是紧挨的，且收入在前费用在后）

    QDate pzDate(y,m,1); //凭证日期为当前打开凭证集的最后日期
    pzDate.setDate(y,m,pzDate.daysInMonth());
    QString strPzDate = pzDate.toString(Qt::ISODate);

    //创建新的或读取现存的结转收入类凭证id值
    s = QString("select id,number from PingZhengs where (isForward = %1) and "
                "(date='%2')").arg(Pzc_JzsyIn).arg(strPzDate);
    if(q.exec(s) && q.first()){
        idNewIn = q.value(0).toInt();
        pzNum = q.value(1).toInt();
        s = QString("delete from BusiActions where pid = %1").arg(idNewIn);
        r = q.exec(s); //删除原先属于此凭证的所有业务活动
        //恢复凭证状态到初始录入态
        s = QString("update PingZhengs set pzState=%1,ruid=%2 where id = %3")
                .arg(Pzs_Recording).arg(user->getUserId()).arg(idNewIn);
        r = q.exec(s);
    }
    else{
        //创建新凭证之前，先获取凭证集内可用的最大凭证号
        pzNum = getMaxPzNum(y,m);
        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
                    "values('%1',%2,%3,%4,%5)").arg(strPzDate).arg(pzNum)
                .arg(Pzc_JzsyIn).arg(Pzs_Recording).arg(user->getUserId());
        r = q.exec(s);
        //回读此凭证的id
        s = QString("select id from PingZhengs where (isForward = %1) and "
                    "(date='%2')").arg(Pzc_JzsyIn).arg(strPzDate);
        r = q.exec(s); r = q.first();
        idNewIn = q.value(0).toInt();
    }

    //创建新的或读取现存的结转费用类凭证id值
    s = QString("select id,number from PingZhengs where (isForward = %1) and "
                "(date='%2')").arg(Pzc_JzsyFei).arg(strPzDate);
    pzNum++;
    if(q.exec(s) && q.first()){
        idNewFei = q.value(0).toInt();
        s = QString("delete from BusiActions where pid = %1").arg(idNewFei);
        r = q.exec(s); //删除原先属于此凭证的所有业务活动
        //恢复凭证状态到初始录入态
        s = QString("update PingZhengs set pzState=%1,ruid=%2 where id = %3")
                .arg(Pzs_Recording).arg(user->getUserId()).arg(idNewFei);
        r = q.exec(s);
    }
    else{
        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
                    "values('%1',%2,%3,%4,%5)").arg(strPzDate).arg(pzNum)
                .arg(Pzc_JzsyFei).arg(Pzs_Recording).arg(user->getUserId());
        r = q.exec(s);
        //回读此凭证的id
        s = QString("select id from PingZhengs where (isForward = %1) and "
                    "(date='%2')").arg(Pzc_JzsyFei).arg(strPzDate);
        r = q.exec(s); r = q.first();
        idNewFei = q.value(0).toInt();
    }

    //3、填充结转凭证的业务活动数据
    //获取“本年利润-结转”子账户-的id
    int bnlrId;
    if(!getIdByName(bnlrId, QObject::tr("本年利润"))){
        qDebug() << QObject::tr("不能获取本年利润科目的id值");
        return false;
    }
    s = QString("select FSAgent.id from FSAgent join SecSubjects "
                "where (FSAgent.sid = SecSubjects.id) and "
                "(FSAgent.fid = %1) and (SecSubjects.subName = '%2')")
            .arg(bnlrId).arg(QObject::tr("结转"));
    if(!q.exec(s) || !q.first()){
        qDebug() << QObject::tr("不能获取”本年利润-结转“子账户");
        return false;
    }
    int bnlrSid = q.value(0).toInt();  //本年利润--结转子账户id

    QList<BusiActionData2*> fad,iad;      //费用和收入结转凭证的业务活动数据列表
    QHash<int, QList<int> > fids,iids;   //费用类和收入类的明细科目id集合，键为一级科目id，值为该一级科目下的子目id集合
    BusiUtil::getAllIdForSy(true,iids);  //获取所有收入类总账-明细id列表
    BusiUtil::getAllIdForSy(false,fids); //获取所有费用类总账-明细id列表

    QList<int> fidLst = fids.keys();     //费用类一级科目id的列表（按id的大小顺序排列，应该是以科目代码顺序排列，
    qSort(fidLst.begin(),fidLst.end());  //这里简略实现，使用此列表只是为了生成的业务活动按预定的顺序）
    QList<int> iidLst = iids.keys();     //收入类一级科目id的列表（按id的大小顺序排列，应该是以科目代码顺序排列）
    qSort(iidLst.begin(),iidLst.end());
    QHash<int,QString> mtHash;
    getMTName(mtHash);
    QList<int> mts = mtHash.keys();      //所有币种代码列表

    int sid,key;  //二级科目id、查询余额的键
    BusiActionData2 *bd1;
    int num = 1; //业务活动在凭证内的序号

    Double iv, fv; //结转凭证的借贷合计值
    SubjectManager* sm = curAccount->getSubjectManager();

    //结转收入类凭证的业务活动列表（这里的最内层循环也可以省略）
    for(int i = 0; i < iidLst.count(); ++i)
        for(int j = 0; j < iids.value(iidLst[i]).count(); ++j)
            for(int k = 0; k < mts.count(); ++k){
                sid = iids.value(iidLst[i])[j];
                key = sid * 10 + mts[k];
                if(extraDet.contains(key) && !extraDet[key].isZero()){
                    bd1 = new BusiActionData2;
                    bd1->state = BusiActionData2::NEW; //新业务活动
                    bd1->num = num++;
                    bd1->pid = idNewIn;
                    bd1->fid = iidLst[i];
                    bd1->sid = sid;
                    bd1->summary = QObject::tr("结转（%1-%2）至本年利润")
                            .arg(sm->getFstSubName(iidLst[i]))
                            .arg(sm->getSndSubName(sid));
                    bd1->mt = mts[k];
                    //结转收入类到本年利润，一般是损益类科目放在借方，本年利润放在贷方
                    //而且，损益类中收入类科目余额约定是贷方，故不做不做方向检测
                    bd1->dir = DIR_J;
                    bd1->v = extraDet.value(key);
                    iad.append(bd1);
                    iv += (bd1->v * rates.value(bd1->mt));
                }
            }
    //创建本年利润-结转的借方会计分录
    bd1 = new BusiActionData2;
    bd1->state = BusiActionData2::NEW; //新业务活动
    bd1->num = num++;
    bd1->pid = idNewIn;
    bd1->fid = bnlrId;
    bd1->sid = bnlrSid;
    bd1->summary = QObject::tr("结转收入至本年利润");
    bd1->mt = RMB;                     //币种
    bd1->dir = DIR_D;
    bd1->v = iv;
    iad.append(bd1);

    //结转费用类凭证的业务活动列表
    num = 1;
    for(int i = 0; i < fidLst.count(); ++i)
        for(int j = 0; j < fids.value(fidLst[i]).count(); ++j)
            for(int k = 0; k < mts.count(); ++k){
                sid = fids.value(fidLst[i])[j];
                key = sid * 10 + mts[k];
                if(extraDet.contains(key) && !extraDet[key].isZero()){
                    bd1 = new BusiActionData2;
                    bd1->state = BusiActionData2::NEW; //新业务活动
                    bd1->num = num++;
                    bd1->pid = idNewFei;
                    bd1->fid = fidLst[i];
                    bd1->sid = sid;
                    bd1->summary = QObject::tr("结转（%1-%2）至本年利润")
                            .arg(sm->getFstSubName(fidLst[i]))
                            .arg(sm->getSndSubName(sid));
                    bd1->mt = mts[k];
                    //结转费用类到本年利润，一般是损益类科目放在贷方，本年利润方在借方
                    //而且，损益类中费用类科目余额是约定在借方，故不做方向检测
                    bd1->dir = DIR_D;
                    bd1->v = extraDet.value(key);
                    fad.append(bd1);
                    fv += (bd1->v * rates.value(bd1->mt));
                }
            }

    //创建本年利润-结转的借方会计分录
    bd1 = new BusiActionData2;
    bd1->state = BusiActionData2::NEW; //新业务活动
    bd1->num = num++;
    bd1->pid = idNewFei;
    bd1->fid = bnlrId;
    bd1->sid = bnlrSid;
    bd1->summary = QObject::tr("结转费用至本年利润");
    bd1->mt = RMB;                        //币种
    bd1->dir = DIR_J;
    bd1->v = fv;
    fad.append(bd1);

    //保存业务活动数据
    saveActionsInPz2(idNewIn,iad);
    saveActionsInPz2(idNewFei,fad);
    //更新合计值
    s = QString("update PingZhengs set jsum = %1,dsum = %2 where id = %3")
            .arg(iv.getv()).arg(iv.getv()).arg(idNewIn);
    r = q.exec(s);
    s = QString("update PingZhengs set jsum = %1,dsum = %2 where id = %3")
            .arg(fv.getv()).arg(fv.getv()).arg(idNewFei);
    r = q.exec(s);

    setPzsState2(y,m,Ps_Jzsy);
    return true;
}


//取消结转损益类科目到本年利润的凭证
bool BusiUtil::antiForwordPl(int y, int m)
{
    return delSpecPz(y,m,Pzc_Jzhd_Bank) &&
           delSpecPz(y,m,Pzc_Jzhd_Ys) &&
           delSpecPz(y,m,Pzc_Jzhd_Yf) &&
           setPzsState(y,m,Ps_JzsyPre);
}

//是否需要创建结转损益类科目到本年利润的凭证
bool BusiUtil::reqForwordPl(int y, int m, bool& req)
{
    //为简单起见，只通过检测凭证集状态来确定
    PzsState state;
    bool r = getPzsState(y,m,state);
    if(state >= Ps_Stat4)
        req = false;
    else
        req = true;
    return r;
}


/**
    结转汇兑损益
*/
//bool BusiUtil::genForwordEx(int y, int m, User* user)
//{
//    //只有在通过了审核阶段1（即凭证集已经进行了第一阶段的统计并保存了余额后）
//    QSqlQuery q;
//    QString s;
//    bool r;

//    int state = 0;
//    if(!getPzsState(y,m,state) && state == PS_STAT2){
//        QMessageBox::warning(0,QObject::tr("操作拒绝"),
//            QObject::tr("只有在凭证集通过了第一阶段的统计并保存余额后，才能进行此操作"));
//        return false;
//    }

//    //获取当期汇率表
//    QHash<int,double> rates,endRates,diffRates;
//    if(!getRates(y, m, rates)){
//        QMessageBox::information(0 ,QObject::tr("提示信息"),QObject::tr("没有指定当期汇率！"));
//        return false;
//    }

//    //读取期末汇率（即下一月汇率），并初始化汇率设定对话框
//    int yy = y;
//    int mm = m;
//    if(mm == 12){
//        mm = 1;
//        yy++;
//    }
//    else
//        mm++;
//    //
//    RateSetDialog* dlg = new RateSetDialog(2);
//    dlg->setCurRates(&rates);    //设置本期汇率
//    getRates(yy, mm, endRates);  //获取期末汇率
//    dlg->setEndRates(&endRates); //设置期末汇率
//    if(QDialog::Rejected == dlg->exec())
//        return false;
//    saveRates(yy,mm,endRates);

//    //计算汇率差
//    QHashIterator<int,double> it(rates);
//    while(it.hasNext()){
//        it.next();
//        double diff = it.value() - endRates.value(it.key()); //期初汇率 - 期末汇率
//        if(diff != 0)
//            diffRates[it.key()] = diff;
//    }
//    if(diffRates.count() == 0)   //如果所有的外币汇率都没有变化，则无须进行汇兑损益的结转
//        return false;

//    //获取财务费用-汇兑损益科目id
//    int cwId,hdId;
//    if(!getIdByName(cwId, QObject::tr("财务费用")))
//        return false;
//    if(!getSidByName(QObject::tr("财务费用"),QObject::tr("汇兑损益"),hdId))
//        return false;

//    QString bCode;//银行存款科目代码及科目id
//    if(!getSubCodeByName(bCode, QString(QObject::tr("银行存款"))))
//        return false;
//    int bankId = 0;
//    if(!getIdByCode(bankId, bCode))
//        return false;

//    QString ysCode;//应收账款科目代码及科目id
//    if(!getSubCodeByName(ysCode, QString(QObject::tr("应收账款"))))
//        return false;
//    int ysId = 0;
//    if(!BusiUtil::getIdByCode(ysId, ysCode))
//        return false;

//    QString yfCode;//应付账款科目代码及科目id
//    if(!getSubCodeByName(yfCode, QString(QObject::tr("应付账款"))))
//        return false;
//    int yfId = 0;
//    if(!getIdByCode(yfId, yfCode))
//        return false;

//    //获取结转汇兑凭证的id值
//    int pid;  //结转汇兑损益的凭证的id
//    QDate d(y,m,1);
//    d.setDate(y,m,d.daysInMonth());
//    QString ds = d.toString(Qt::ISODate);

//    s = QString("select id from PingZhengs where (date='%1') and (isForward=%2)")
//            .arg(ds).arg(PZC_JZHD);
//    if(q.exec(s) && q.first()){
//        pid = q.value(0).toInt();
//        //删除原先属于该凭证的所有业务活动
//        s = QString("delete from BusiActions where pid=%1").arg(pid);
//        r = q.exec(s);
//    }
//    else{
//        int pzNum = getMaxPzNum(y,m);
//        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
//                    "values('%1',%2,%3,%4,%5)").arg(ds).arg(pzNum).arg(PZC_JZHD)
//                .arg(PZS_RECORDING).arg(user->getUserId());
//        r = q.exec(s);
//        if(r){
//            s = QString("select id from PingZhengs where (date='%1') and (isForward=%2)")
//                    .arg(ds).arg(PZC_JZHD);
//            r = q.exec(s); r = q.first();
//            pid = q.value(0).toInt();
//        }
//        else{
//            qDebug() << QObject::tr("创建结转汇兑凭证失败");
//            return false;
//        }
//    }

//    QList<int> wbIds,wbMts;  //银行存款科目下的外币科目id及其对应的币种代码列表
//    getOutMtInBank(wbIds, wbMts);

//    //读取银行存款-外币子科目余额，并依余额值、余额方向和汇率差的方向生成业务活动数据
//    QList<BusiActionData*> balst;  //保存业务活动数据的列表
//    BusiActionData *bd1,*bd2;
//    double v,sum=0;   //子目余额、借贷值合计
//    int edir;   //余额方向（整形表示，有1，0，-1）
//    bool ebdir; //余额方向
//    bool dir; //业务活动的银行存款方的借贷方向，该方向的判定规则：
//              //银行存款余额的方向 && 汇率差的方向（汇率变小为借或正） = 业务活动的借贷方向
//    int num = 1;  //业务活动序号

//    QList<int> ids;        //银行存款、应收、应付总目下的所有子目的id列表
//    QList<QString> names;  //上面子目的名称
//    QHash<int,QString> bankSubNames;  //银行存款下所有外币科目（科目id到科目名称）的映射表

//    //生成与结转银行存款汇兑损益的业务活动条目
//    if(getSndSubInSpecFst(bankId,ids,names)){
//        for(int i = 0; i < ids.count(); ++i){
//            if(names[i].indexOf(QObject::tr("人民币")) == -1)
//                bankSubNames[ids[i]] = names[i];
//        }
//        for(int i = 0; i < wbIds.count(); ++i){
//            if(readDetExtraForMt(y,m,wbIds[i],wbMts[i],v,edir)){
//                if(edir == DIR_J)
//                    ebdir = true;
//                else if(edir == DIR_D)
//                    ebdir = false;
//                else            //跳过余额为零的子目
//                    continue;
//                bd1 = new BusiActionData;  //存放银行存款一方
//                bd2 = new BusiActionData;  //存放财务费用一方
//                bd1->num = num++;
//                bd2->num = num++;
//                bd1->state = BusiActionData::NEW;
//                bd2->state = BusiActionData::NEW;
//                bd1->pid = pid;
//                bd2->pid = pid;
//                bd1->fid = bankId;
//                bd2->fid = cwId;
//                bd1->sid = wbIds[i];
//                bd2->sid = hdId;
//                bd1->mt = RMB;
//                bd2->mt = RMB;
//                bd1->summary = QObject::tr("结转汇兑损益");
//                bd2->summary = QObject::tr("结转自银行存款-%1的汇兑损益")
//                        .arg(bankSubNames.value(wbIds[i]));
//                dir = (diffRates.value(wbMts[i]) > 0);
//                if(dir){
//                    bd1->v = v * diffRates.value(wbMts[i]);
//                    bd2->v = v * diffRates.value(wbMts[i]);
//                }
//                else{
//                    bd1->v = -v * diffRates.value(wbMts[i]);
//                    bd2->v = -v * diffRates.value(wbMts[i]);
//                }
//                sum += bd1->v;
//                bd1->dir = ebdir && dir;
//                bd2->dir = !(ebdir && dir);
//                balst << bd1 << bd2;
//            }
//        }
//    }


//    //生成与结转应收账款汇兑损益的业务活动条目
//    ids.clear(); names.clear();
//    wbMts = rates.keys();  //获取所有外币的代码列表
//    if(getSndSubInSpecFst(ysId,ids,names)){
//        for(int i = 0; i < wbMts.count(); ++i) //外层币种
//            for(int j = 0; j < ids.count(); ++j){      //内存应收账款子目
//                if(readDetExtraForMt(y,m,ids[j],wbMts[i],v,edir)){
//                    if(edir == DIR_J)
//                        ebdir = true;
//                    else if(edir == DIR_D)
//                        ebdir = false;
//                    else            //跳过余额为零的子目
//                        continue;
//                    bd1 = new BusiActionData;  //存放应收账款一方
//                    bd2 = new BusiActionData;  //存放财务费用一方
//                    bd1->num = num++;
//                    bd2->num = num++;
//                    bd1->state = BusiActionData::NEW;
//                    bd2->state = BusiActionData::NEW;
//                    bd1->pid = pid;
//                    bd2->pid = pid;
//                    bd1->fid = ysId;
//                    bd2->fid = cwId;
//                    bd1->sid = ids[j];
//                    bd2->sid = hdId;
//                    bd1->mt = RMB;
//                    bd2->mt = RMB;
//                    bd1->summary = QObject::tr("结转汇兑损益");
//                    bd2->summary = QObject::tr("结转自应收账款-%1的汇兑损益").arg(names[j]);
//                    dir = (diffRates.value(wbMts[i]) > 0); //汇率降低为真
//                    if(dir){
//                        bd1->v = v * diffRates.value(wbMts[i]);
//                        bd2->v = v * diffRates.value(wbMts[i]);
//                    }
//                    else{
//                        bd1->v = -v * diffRates.value(wbMts[i]);
//                        bd2->v = -v * diffRates.value(wbMts[i]);
//                    }
//                    sum += bd1->v;
//                    bd1->dir = ebdir && dir;
//                    bd2->dir = !(ebdir && dir);
//                    balst << bd1 << bd2;
//                }
//            }
//    }

//    //生成与结转应付存款汇兑损益的业务活动条目
//    ids.clear();
//    names.clear();
//    if(getSndSubInSpecFst(yfId,ids,names)){
//        for(int i = 0; i < wbMts.count(); ++i) //外层币种
//            for(int j = 0; j < ids.count(); ++j){      //内层应付账款子目
//                if(readDetExtraForMt(y,m,ids[j],wbMts[i],v,edir)){
//                    if(edir == DIR_J)
//                        ebdir = true;
//                    else if(edir == DIR_D)
//                        ebdir = false;
//                    else            //跳过余额为零的子目
//                        continue;
//                    bd1 = new BusiActionData;  //存放应付账款一方
//                    bd2 = new BusiActionData;  //存放财务费用一方
//                    bd1->num = num++;
//                    bd2->num = num++;
//                    bd1->state = BusiActionData::NEW;
//                    bd2->state = BusiActionData::NEW;
//                    bd1->pid = pid;
//                    bd2->pid = pid;
//                    bd1->fid = yfId;
//                    bd2->fid = cwId;
//                    bd1->sid = ids[j];
//                    bd2->sid = hdId;
//                    bd1->mt = RMB;
//                    bd2->mt = RMB;
//                    bd1->summary = QObject::tr("结转汇兑损益");
//                    bd2->summary = QObject::tr("结转自应付账款-%1的汇兑损益").arg(names[j]);
//                    dir = (diffRates.value(wbMts[i]) > 0); //汇率降低为真
//                    if(dir){
//                        bd1->v = v * diffRates.value(wbMts[i]);
//                        bd2->v = v * diffRates.value(wbMts[i]);
//                    }
//                    else{
//                        bd1->v = -v * diffRates.value(wbMts[i]);
//                        bd2->v = -v * diffRates.value(wbMts[i]);
//                    }
//                    sum += bd1->v;
//                    bd1->dir = ebdir && dir;
//                    bd2->dir = !(ebdir && dir);
//                    balst << bd1 << bd2;
//                }
//            }
//    }


//    //保存业务活动到数据库
//    saveActionsInPz(pid,balst);

//    //清除内存
//    for(int i = 0; i < balst.count(); ++i)
//        delete balst[i];
//    balst.clear();

//    //更新借贷合计值
//    s = QString("update PingZhengs set jsum=%1,dsum=%1 where id=%2").arg(sum).arg(pid);
//    r = q.exec(s);
//    return r;
//}

//获取指定年月指定特种类别（非手工录入凭证）凭证的id（如不存在则创建新的）
bool BusiUtil::getPzIdForSpecCls(int y, int m, int cls, User* user, int& id)
{
    QSqlQuery q;
    bool r;
    QString s;

    QDate d(y,m,1);
    d.setDate(y,m,d.daysInMonth());
    QString ds = d.toString(Qt::ISODate);

    s = QString("select id from PingZhengs where (date='%1') and (isForward=%2)")
            .arg(ds).arg(cls);
    if(q.exec(s) && q.first()){
        id = q.value(0).toInt();
        //删除原先属于该凭证的所有业务活动
        s = QString("delete from BusiActions where pid=%1").arg(id);
        r = q.exec(s);
    }
    else{
        int pzNum = getMaxPzNum(y,m);
        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
                    "values('%1',%2,%3,%4,%5)").arg(ds).arg(pzNum).arg(cls)
                .arg(Pzs_Recording).arg(user->getUserId());
        r = q.exec(s);
        if(r){
            s = QString("select id from PingZhengs where (date='%1') and (isForward=%2)")
                    .arg(ds).arg(cls);
            r = q.exec(s); r = q.first();
            id = q.value(0).toInt();
        }
        else{
            qDebug() << QObject::tr("创建结转汇兑凭证失败");
            return false;
        }
    }
}

/**
    结转汇兑损益（此版本将产生3个凭证，分别结转银行，应收和应付）
    执行此操作到前提是凭证集状态必须是Ps_Stat2--即所有相关凭证（包括手工录入和自动引入的凭证）
    都已审核/入账，并进行了统计和余额保存。
*/
bool BusiUtil::genForwordEx(int y, int m, User* user, int state)
{
    //如果凭证集处于Ps_Stat1状态，则必须测试当期是否需要引入由其他模块产生的
    //需要在结转汇兑损益之前存在的凭证，如果有，则提示用户先进行相应的操作。如果
    //没有，则直接将凭证集状态转到Ps_Stat1态，以便进行后续操作。


    //只有在通过了审核阶段1（即凭证集已经进行了第一阶段的统计并保存了余额后）
    QSqlQuery q;
    QString s;
    bool r;

    PzsState pzsState;
    if(getPzsState(y,m,pzsState)){
        QMessageBox::warning(0,QObject::tr("操作拒绝"),
            QObject::tr("无法获取当前年月的凭证集状态！"));
        return false;
    }
    if((pzsState != Ps_Stat1) && (pzsState != Ps_Stat2)){
        QMessageBox::warning(0,QObject::tr("操作拒绝"),
            QObject::tr("执行此操作前，必须先进行第一阶段的统计并保存了余额后！"));
        return false;
    }

    //获取当期汇率表
    QHash<int,double> rates,endRates,diffRates;
    if(!getRates(y, m, rates)){
        QMessageBox::information(0 ,QObject::tr("提示信息"),QObject::tr("没有指定当期汇率！"));
        return false;
    }

    //读取期末汇率（即下一月汇率），并初始化汇率设定对话框
    int yy = y;
    int mm = m;
    if(mm == 12){
        mm = 1;
        yy++;
    }
    else
        mm++;
    //
    RateSetDialog* dlg = new RateSetDialog(2);
    dlg->setCurRates(&rates);    //设置本期汇率
    getRates(yy, mm, endRates);  //获取期末汇率
    dlg->setEndRates(&endRates); //设置期末汇率
    if(QDialog::Rejected == dlg->exec())
        return false;
    saveRates(yy,mm,endRates);

    //计算汇率差
    QHashIterator<int,double> it(rates);
    while(it.hasNext()){
        it.next();
        double diff = it.value() - endRates.value(it.key()); //期初汇率 - 期末汇率
        if(diff != 0)
            diffRates[it.key()] = diff;
    }
    if(diffRates.count() == 0)   //如果所有的外币汇率都没有变化，则无须进行汇兑损益的结转
        return false;

    //获取财务费用-汇兑损益科目id
    int cwId,hdId;
    if(!getIdByName(cwId, QObject::tr("财务费用")))
        return false;
    if(!getSidByName(QObject::tr("财务费用"),QObject::tr("汇兑损益"),hdId))
        return false;

    QString bCode;//银行存款科目代码及科目id
    if(!getSubCodeByName(bCode, QString(QObject::tr("银行存款"))))
        return false;
    int bankId = 0;
    if(!getIdByCode(bankId, bCode))
        return false;

    QString ysCode;//应收账款科目代码及科目id
    if(!getSubCodeByName(ysCode, QString(QObject::tr("应收账款"))))
        return false;
    int ysId = 0;
    if(!BusiUtil::getIdByCode(ysId, ysCode))
        return false;

    QString yfCode;//应付账款科目代码及科目id
    if(!getSubCodeByName(yfCode, QString(QObject::tr("应付账款"))))
        return false;
    int yfId = 0;
    if(!getIdByCode(yfId, yfCode))
        return false;


    //获取结转汇兑凭证的id值
    int bankPid;  //结转银行汇兑损益的凭证的id
    int ysPid;    //结转应收汇兑损益的凭证的id
    int yfPid;    //结转应付汇兑损益的凭证的id
    getPzIdForSpecCls(y,m,Pzc_Jzhd_Bank,user,bankPid);
    getPzIdForSpecCls(y,m,Pzc_Jzhd_Ys,user,ysPid);
    getPzIdForSpecCls(y,m,Pzc_Jzhd_Yf,user,yfPid);

    QList<int> wbIds,wbMts;  //银行存款科目下的外币科目id及其对应的币种代码列表
    getOutMtInBank(wbIds, wbMts);

    //读取银行存款-外币子科目余额，并依余额值、余额方向和汇率差的方向生成业务活动数据
    QList<BusiActionData*> balst;  //保存业务活动数据的列表
    BusiActionData *bd1,*bd2;
    double v,sum=0;   //子目余额、借贷值合计
    int edir;   //余额方向（整形表示，有1，0，-1）
    bool ebdir; //余额方向
    bool dir; //业务活动的银行存款方的借贷方向，该方向的判定规则：
              //银行存款余额的方向 && 汇率差的方向（汇率变小为借或正） = 业务活动的借贷方向
    int num = 1;  //业务活动序号

    QList<int> ids;        //银行存款、应收、应付总目下的所有子目的id列表
    QList<QString> names;  //上面子目的名称
    QHash<int,QString> bankSubNames;  //银行存款下所有外币科目（科目id到科目名称）的映射表

    //创建结转银行存款各子目的结转汇兑损益凭证
    //生成与结转银行存款汇兑损益的业务活动条目
    if(getSndSubInSpecFst(bankId,ids,names)){
        for(int i = 0; i < ids.count(); ++i){
            if(names[i].indexOf(QObject::tr("人民币")) == -1)
                bankSubNames[ids[i]] = names[i];
        }
        for(int i = 0; i < wbIds.count(); ++i){
            if(readDetExtraForMt(y,m,wbIds[i],wbMts[i],v,edir)){
                if(edir == DIR_J)
                    ebdir = true;
                else if(edir == DIR_D)
                    ebdir = false;
                else            //跳过余额为零的子目
                    continue;
                bd1 = new BusiActionData;  //存放银行存款一方
                bd2 = new BusiActionData;  //存放财务费用一方
                bd1->num = num++;
                bd2->num = num++;
                bd1->state = BusiActionData::NEW;
                bd2->state = BusiActionData::NEW;
                bd1->pid = bankPid;
                bd2->pid = bankPid;
                bd1->fid = bankId;
                bd2->fid = cwId;
                bd1->sid = wbIds[i];
                bd2->sid = hdId;
                bd1->mt = RMB;
                bd2->mt = RMB;
                bd1->summary = QObject::tr("结转汇兑损益");
                bd2->summary = QObject::tr("结转自银行存款-%1的汇兑损益")
                        .arg(bankSubNames.value(wbIds[i]));
                dir = (diffRates.value(wbMts[i]) > 0);
                if(dir){
                    bd1->v = v * diffRates.value(wbMts[i]);
                    bd2->v = v * diffRates.value(wbMts[i]);
                }
                else{
                    bd1->v = -v * diffRates.value(wbMts[i]);
                    bd2->v = -v * diffRates.value(wbMts[i]);
                }
                sum += bd1->v;
                bd1->dir = ebdir && dir;
                bd2->dir = !(ebdir && dir);
                balst << bd1 << bd2;
            }
        }
        //保存业务活动到数据库
        saveActionsInPz(bankPid,balst);

        //清除内存
        for(int i = 0; i < balst.count(); ++i)
            delete balst[i];
        balst.clear();

        //更新借贷合计值
        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
                .arg(sum).arg(bankPid).arg(state);
        r = q.exec(s);
    }

    //生成与结转应收账款汇兑损益的业务活动条目
    ids.clear(); names.clear();
    wbMts = rates.keys();  //获取所有外币的代码列表
    if(getSndSubInSpecFst(ysId,ids,names)){
        for(int i = 0; i < wbMts.count(); ++i) //外层币种
            for(int j = 0; j < ids.count(); ++j){      //内存应收账款子目
                if(readDetExtraForMt(y,m,ids[j],wbMts[i],v,edir)){
                    if(edir == DIR_J)
                        ebdir = true;
                    else if(edir == DIR_D)
                        ebdir = false;
                    else            //跳过余额为零的子目
                        continue;
                    bd1 = new BusiActionData;  //存放应收账款一方
                    bd2 = new BusiActionData;  //存放财务费用一方
                    bd1->num = num++;
                    bd2->num = num++;
                    bd1->state = BusiActionData::NEW;
                    bd2->state = BusiActionData::NEW;
                    bd1->pid = ysPid;
                    bd2->pid = ysPid;
                    bd1->fid = ysId;
                    bd2->fid = cwId;
                    bd1->sid = ids[j];
                    bd2->sid = hdId;
                    bd1->mt = RMB;
                    bd2->mt = RMB;
                    bd1->summary = QObject::tr("结转汇兑损益");
                    bd2->summary = QObject::tr("结转自应收账款-%1的汇兑损益").arg(names[j]);
                    dir = (diffRates.value(wbMts[i]) > 0); //汇率降低为真
                    if(dir){
                        bd1->v = v * diffRates.value(wbMts[i]);
                        bd2->v = v * diffRates.value(wbMts[i]);
                    }
                    else{
                        bd1->v = -v * diffRates.value(wbMts[i]);
                        bd2->v = -v * diffRates.value(wbMts[i]);
                    }
                    sum += bd1->v;
                    bd1->dir = ebdir && dir;
                    bd2->dir = !(ebdir && dir);
                    balst << bd1 << bd2;
                }
            }
        //保存业务活动到数据库
        saveActionsInPz(ysPid,balst);

        //清除内存
        for(int i = 0; i < balst.count(); ++i)
            delete balst[i];
        balst.clear();

        //更新借贷合计值和凭证状态
        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
                .arg(sum).arg(ysPid).arg(state);
        r = q.exec(s);
    }

    //生成与结转应付存款汇兑损益的业务活动条目
    ids.clear();
    names.clear();
    if(getSndSubInSpecFst(yfId,ids,names)){
        for(int i = 0; i < wbMts.count(); ++i) //外层币种
            for(int j = 0; j < ids.count(); ++j){      //内层应付账款子目
                if(readDetExtraForMt(y,m,ids[j],wbMts[i],v,edir)){
                    if(edir == DIR_J)
                        ebdir = true;
                    else if(edir == DIR_D)
                        ebdir = false;
                    else            //跳过余额为零的子目
                        continue;
                    bd1 = new BusiActionData;  //存放应付账款一方
                    bd2 = new BusiActionData;  //存放财务费用一方
                    bd1->num = num++;
                    bd2->num = num++;
                    bd1->state = BusiActionData::NEW;
                    bd2->state = BusiActionData::NEW;
                    bd1->pid = yfPid;
                    bd2->pid = yfPid;
                    bd1->fid = yfId;
                    bd2->fid = cwId;
                    bd1->sid = ids[j];
                    bd2->sid = hdId;
                    bd1->mt = RMB;
                    bd2->mt = RMB;
                    bd1->summary = QObject::tr("结转汇兑损益");
                    bd2->summary = QObject::tr("结转自应付账款-%1的汇兑损益").arg(names[j]);
                    dir = (diffRates.value(wbMts[i]) > 0); //汇率降低为真
                    if(dir){
                        bd1->v = v * diffRates.value(wbMts[i]);
                        bd2->v = v * diffRates.value(wbMts[i]);
                    }
                    else{
                        bd1->v = -v * diffRates.value(wbMts[i]);
                        bd2->v = -v * diffRates.value(wbMts[i]);
                    }
                    sum += bd1->v;
                    bd1->dir = ebdir && dir;
                    bd2->dir = !(ebdir && dir);
                    balst << bd1 << bd2;
                }
            }
        //保存业务活动到数据库
        saveActionsInPz(yfPid,balst);

        //清除内存
        for(int i = 0; i < balst.count(); ++i)
            delete balst[i];
        balst.clear();

        //更新借贷合计值和凭证状态
        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
                .arg(sum).arg(yfPid).arg(state);
        r = q.exec(s);
    }
    return r;
}

/**
    结转汇兑损益（此版本将产生3个凭证，分别结转银行，应收和应付）
    执行此操作到前提是凭证集状态必须是Ps_Stat2--即所有相关凭证（包括手工录入和自动引入的凭证）
    都已审核/入账，并进行了统计和余额保存。
*/
bool BusiUtil::genForwordEx2(int y, int m, User *user, int state)
{
    //如果凭证集处于Ps_Stat1状态，则必须测试当期是否需要引入由其他模块产生的
    //需要在结转汇兑损益之前存在的凭证，如果有，则提示用户先进行相应的操作。如果
    //没有，则直接将凭证集状态转到Ps_Stat1态，以便进行后续操作。


    //只有在通过了审核阶段1（即凭证集已经进行了第一阶段的统计并保存了余额后）
    QSqlQuery q;
    QString s;

    PzsState pzsState;
    if(!getPzsState(y,m,pzsState)){
        QMessageBox::warning(0,QObject::tr("操作拒绝"),
            QObject::tr("无法获取当前年月的凭证集状态！"));
        return false;
    }
    if((pzsState != Ps_Stat1) && (pzsState != Ps_Stat2)){
        QMessageBox::warning(0,QObject::tr("操作拒绝"),
            QObject::tr("执行此操作前，必须先进行第一阶段的统计并保存了余额后！"));
        return false;
    }

    //获取当期汇率表
    QHash<int,Double> rates,endRates,diffRates;//本期、期末汇率和汇率差
    if(!getRates2(y, m, rates)){
        QMessageBox::information(0 ,QObject::tr("提示信息"),QObject::tr("没有指定当期汇率！"));
        return false;
    }

    //读取期末汇率（即下一月汇率），并初始化汇率设定对话框
    int yy = y;
    int mm = m;
    if(mm == 12){
        mm = 1;
        yy++;
    }
    else
        mm++;

    if(!getRates2(yy, mm, endRates)){
        QMessageBox::information(0 ,QObject::tr("提示信息"),QObject::tr("没有指定期末汇率！"));
        return false;
    }
    //
//    RateSetDialog* dlg = new RateSetDialog(2);
//    dlg->setCurRates(&rates);    //设置本期汇率
//    getRates(yy, mm, endRates);  //获取期末汇率
//    dlg->setEndRates(&endRates); //设置期末汇率
//    if(QDialog::Rejected == dlg->exec())
//        return false;
//    saveRates(yy,mm,endRates);

    //计算汇率差
    QHashIterator<int,Double> it(rates);
    while(it.hasNext()){
        it.next();
        Double diff = it.value() - endRates.value(it.key()); //期初汇率 - 期末汇率
        if(diff != 0)
            diffRates[it.key()] = diff;
    }
    if(diffRates.count() == 0){   //如果所有的外币汇率都没有变化，则无须进行汇兑损益的结转
        QMessageBox::information(0 ,QObject::tr("提示信息"),QObject::tr("汇率没有发生变动，无需结转汇兑损益！"));
        return true;
    }

    //获取财务费用、及其下的汇兑损益科目id
    int cwId,hdId;
    if(!getIdByName(cwId, QObject::tr("财务费用")))
        return false;
    if(!getSidByName(QObject::tr("财务费用"),QObject::tr("汇兑损益"),hdId))
        return false;

    QString bCode;//银行存款科目代码及科目id
    if(!getSubCodeByName(bCode, QString(QObject::tr("银行存款"))))
        return false;
    int bankId = 0;
    if(!getIdByCode(bankId, bCode))
        return false;

    QString ysCode;//应收账款科目代码及科目id
    if(!getSubCodeByName(ysCode, QString(QObject::tr("应收账款"))))
        return false;
    int ysId = 0;
    if(!BusiUtil::getIdByCode(ysId, ysCode))
        return false;

    QString yfCode;//应付账款科目代码及科目id
    if(!getSubCodeByName(yfCode, QString(QObject::tr("应付账款"))))
        return false;
    int yfId = 0;
    if(!getIdByCode(yfId, yfCode))
        return false;


    //获取结转汇兑凭证的id值
    int bankPid;  //结转银行汇兑损益的凭证的id
    int ysPid;    //结转应收汇兑损益的凭证的id
    int yfPid;    //结转应付汇兑损益的凭证的id
    getPzIdForSpecCls(y,m,Pzc_Jzhd_Bank,user,bankPid);
    getPzIdForSpecCls(y,m,Pzc_Jzhd_Ys,user,ysPid);
    getPzIdForSpecCls(y,m,Pzc_Jzhd_Yf,user,yfPid);

    QList<int> wbIds,wbMts;  //银行存款科目下的外币科目id及其对应的币种代码列表
    getOutMtInBank(wbIds, wbMts);

    //读取银行存款-外币子科目余额，并依余额值、余额方向和汇率差的方向生成业务活动数据
    QList<BusiActionData2*> balst;  //保存业务活动数据的列表
    BusiActionData2 *bd1,*bd2;
    Double v,sum=0.00;   //子目余额、借贷值合计
    int edir;   //余额方向（整形表示，有1，0，-1）
    bool ebdir; //余额方向
    bool dir; //业务活动的银行存款方的借贷方向，该方向的判定规则：
              //银行存款余额的方向 && 汇率差的方向（汇率变小为借或正） = 业务活动的借贷方向
    int num = 1;  //业务活动序号

    QList<int> ids;        //银行存款、应收、应付总目下的所有子目的id列表
    QList<QString> names;  //上面子目的名称
    QHash<int,QString> bankSubNames;  //银行存款下所有外币科目（科目id到科目名称）的映射表

    //创建结转银行存款各子目的结转汇兑损益凭证
    //生成与结转银行存款汇兑损益的业务活动条目
    if(getSndSubInSpecFst(bankId,ids,names)){
        for(int i = 0; i < ids.count(); ++i){
            if(names[i].indexOf(QObject::tr("人民币")) == -1)
                bankSubNames[ids[i]] = names[i];
        }
        sum = 0.00;num=1;
        //创建银行一方的会计分录（每个银行外币账户对应一项）
        for(int i = 0; i < wbIds.count(); ++i){
            if(!readDetExtraForMt2(y,m,wbIds[i],wbMts[i],v,edir))
                return false;
            if(edir == DIR_P)
                continue;
            bd1 = new BusiActionData2;  //存放银行存款一方
            bd1->num = num++;
            bd1->state = BusiActionData2::NEW;
            bd1->pid = bankPid;
            bd1->fid = bankId;
            bd1->sid = wbIds[i];
            bd1->mt = RMB;
            bd1->summary = QObject::tr("结转汇兑损益");
            bd1->dir = DIR_D;
            bd1->v = v * diffRates.value(wbMts[i]);			
            sum += bd1->v;
            balst << bd1;
        }
        //创建财务费用一方，是上面这些的合计
        bd2 = new BusiActionData2;
        bd2->num = num++;
        bd2->state = BusiActionData2::NEW;
        bd2->pid = bankPid;
        bd2->fid = cwId;
        bd2->sid = hdId;
        bd2->mt = RMB;
        bd2->summary = QObject::tr("结转自银行存款的汇兑损益");
        bd2->dir = DIR_J;
        bd2->v = sum;
        balst<<bd2;
        //保存业务活动到数据库
        saveActionsInPz2(bankPid,balst);

        //清除内存
        for(int i = 0; i < balst.count(); ++i)
            delete balst[i];
        balst.clear();

        //更新借贷合计值
        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
                .arg(sum.getv()).arg(bankPid).arg(state);
        if(!q.exec(s))
            return false;
    }

    //生成与结转应收账款汇兑损益的业务活动条目
    sum=0.00; num=0;
    ids.clear(); names.clear();
    wbMts = rates.keys();  //获取所有外币的代码列表
    if(getSndSubInSpecFst(ysId,ids,names)){
        for(int i = 0; i < wbMts.count(); ++i) //外层币种
            for(int j = 0; j < ids.count(); ++j){      //内存应收账款子目
                if(!readDetExtraForMt2(y,m,ids[j],wbMts[i],v,edir))
                    return false;

                if(edir == DIR_P)
                    continue;
                bd1 = new BusiActionData2;  //存放应收账款一方
                bd1->num = num++;
                bd1->state = BusiActionData2::NEW;
                bd1->pid = ysPid;
                bd1->fid = ysId;
                bd1->sid = ids[j];
                bd1->mt = RMB;
                bd1->summary = QObject::tr("结转汇兑损益");
                bd1->dir = DIR_D;
                bd1->v = v * diffRates.value(wbMts[i]);
				//应收余额在贷方实际上就变成了应付
		        if(edir == DIR_D)
		            bd1->v.changeSign();
                sum += bd1->v;
                balst << bd1;
            }
        //创建财务费用一方
        bd2 = new BusiActionData2;  //存放财务费用一方
        bd2->num = num++;
        bd2->state = BusiActionData2::NEW;
        bd2->pid = ysPid;
        bd2->fid = cwId;
        bd2->sid = hdId;
        bd2->mt = RMB;
        bd2->summary = QObject::tr("结转自应收账款的汇兑损益");
        bd2->dir = DIR_J;
        bd2->v = sum;
        balst<<bd2;

        //保存业务活动到数据库
        saveActionsInPz2(ysPid,balst);

        //清除内存
        for(int i = 0; i < balst.count(); ++i)
            delete balst[i];
        balst.clear();

        //更新借贷合计值和凭证状态
        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
                .arg(sum.getv()).arg(ysPid).arg(state);
        if(!q.exec(s))
            return false;
    }

    //生成与结转应付存款汇兑损益的业务活动条目
    ids.clear();
    names.clear();
    sum = 0.00; num=0;
    if(getSndSubInSpecFst(yfId,ids,names)){
        for(int i = 0; i < wbMts.count(); ++i) //外层币种
            for(int j = 0; j < ids.count(); ++j){      //内层应付账款子目
                if(!readDetExtraForMt2(y,m,ids[j],wbMts[i],v,edir))
                    return false;
                if(edir == DIR_P)
                    continue;
                bd1 = new BusiActionData2;  //存放应付账款一方
                bd1->num = num++;
                bd1->state = BusiActionData2::NEW;
                bd1->pid = yfPid;
                bd1->fid = yfId;
                bd1->sid = ids[j];
                bd1->mt = RMB;
                bd1->summary = QObject::tr("结转汇兑损益");
                bd1->dir = DIR_D;
                bd1->v = v * diffRates.value(wbMts[i]);
				//应付余额在借方实际上就变成了应收
                if(edir == DIR_D)
                    bd1->v.changeSign();
                sum += bd1->v;
                balst << bd1;
            }
        //创建财务费用一方
        bd2 = new BusiActionData2;  //存放财务费用一方
        bd2->num = num++;
        bd2->state = BusiActionData2::NEW;
        bd2->pid = yfPid;
        bd2->fid = cwId;
        bd2->sid = hdId;
        bd2->mt = RMB;
        bd2->summary = QObject::tr("结转自应付账款的汇兑损益");
        bd2->dir = DIR_J;
        bd2->v = sum;
        balst<<bd2;
        //保存业务活动到数据库
        saveActionsInPz2(yfPid,balst);

        //清除内存
        for(int i = 0; i < balst.count(); ++i)
            delete balst[i];
        balst.clear();

        //更新借贷合计值和凭证状态
        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
                .arg(sum.getv()).arg(yfPid).arg(state);
        if(!q.exec(s))
            return false;
    }
    return true;
}

//在FSAgent表中创建新的一二级科目的映射条目
bool BusiUtil::newFstToSnd(int fid, int sid, int& id)
{
    //获取一级和二级科目的名称
    QString /*fname, sname, */s; //一、二级科目名
    QSqlQuery q;
    bool r;

    s = QString("insert into FSAgent(fid, sid) values(%1, %2)").arg(fid).arg(sid);
    r = q.exec(s);
    if(r){
        s = QString("select id from FSAgent where (fid=%1) and (sid=%2)")
                .arg(fid).arg(sid);
        r = q.exec(s); r = q.first();
        if(r)
            id = q.value(0).toInt();
    }
    return r;
}

/**
    创建新的二级科目名称，并建立与指定一级科目的对应关系
    参数 fid：一级科目id，id：明细科目id（FSAgent表中的id），name：二级科目名，
        lname：二级科目全称，remCode：二级科目助记符，clsCode：二级科目类别代码
*/

bool BusiUtil::newSndSubAndMapping(int fid, int& id, QString name,QString lname,
                                   QString remCode, int clsCode)
{
    QSqlQuery q;
    QString s;
    //在SecSubject表中创建新二级科目名称前，不进行检测是因为调用此函数前已经进行了相应的检测
    s = QString("insert into SecSubjects(subName,subLName,remCode,classId) "
                "values('%1','%2','%3',%4)").arg(name).arg(lname).arg(remCode).arg(clsCode);
    bool r = q.exec(s);
    if(!r)
        return false;
    //回读此二级科目的id
    s = QString("select id from SecSubjects where subName = '%1'").arg(name);
    r = q.exec(s);
    if(!r)
        return false;
    r = q.first();
    int sid = q.value(0).toInt();
    r = newFstToSnd(fid,sid,id);
    return r;
}

//读取指定一级科目到指定二级科目的映射条目的id
bool BusiUtil::getFstToSnd(int fid, int sid, int& id)
{
    QSqlQuery q;
    QString s;
    s = QString("select id from FSAgent where (fid = %1) and (sid = %2)")
            .arg(fid).arg(sid);
    bool r = q.exec(s);
    if(r && q.first()){
        id = q.value(0).toInt();
        return true;
    }
    else
        return false;

}

bool BusiUtil::getSndSubNameForId(int id, QString& name, QString& lname)
{
    QSqlQuery q;
    QString s;
    s = QString("select SecSubjects.subName,SecSubjects.subName from FSAgent join SecSubjects "
                "where (SecSubjects.id = FSAgent.sid) and (FSAgent.id = %1)")
            .arg(id);
    bool r = q.exec(s);
    if(r && q.first()){
        name = q.value(0).toString();
        lname = q.value(1).toString();
        return true;
    }
    else
        return false;
}

/**
    读取指定月份指定总账科目的余额（参数fid：总账科目id，v是余额值，dir是方向，key为币种代码）
*/
bool BusiUtil::readExtraForSub(int y,int m, int fid, QHash<int,double>& v, QHash<int,int>& dir)
{
    QSqlQuery q;
    QString s;

    //获取总账科目余额对应的字段名称
    s = QString("select subCode from FirSubjects where id = %1").arg(fid);
    bool r = q.exec(s);
    r = q.first();
    if(!r){
        QMessageBox::information(0, QObject::tr("提示信息"),
                                 QString(QObject::tr("不能获取id为%1的总账科目代码")).arg(fid));
        return false;
    }
    QString code = q.value(0).toString();
    char c = 'A' + code.left(1).toInt() - 1;
    QString fname;
    fname.append(c).append(code);

    s = QString("select mt,%1 from SubjectExtras where (year = %2) and "
                "(month = %3)").arg(fname).arg(y).arg(m);
    if(!q.exec(s) || !q.first()){
        //QMessageBox::information(0, QObject::tr("提示信息"),
        //                         QString(QObject::tr("不能获取%1年%2月id为%3的总账科目余额值")).arg(y).arg(m).arg(fid));
        qDebug() << QObject::tr("不能获取%1年%2月id为%3的总账科目余额值")
                    .arg(y).arg(m).arg(fid);
        return false;
    }

    v.clear();
    dir.clear();

    q.seek(-1);
    while(q.next())
        v[q.value(0).toInt()] = q.value(1).toDouble();

    //读取方向
    s = QString("select mt,%1 from SubjectExtraDirs where (year = %2) and "
                "(month = %3)").arg(fname).arg(y).arg(m);
    if(!q.exec(s) || !q.first()){
        //QMessageBox::information(0, QObject::tr("提示信息"),
        //                         QString(QObject::tr("不能获取%1年%2月id为%3的总账科目余额方向")).arg(y).arg(m).arg(fid));
        qDebug() << QObject::tr("不能获取%1年%2月id为%3的总账科目余额方向")
                    .arg(y).arg(m).arg(fid);
        return false;
    }
    q.seek(-1);
    while(q.next())
        dir[q.value(0).toInt()] = q.value(1).toInt();
    return true;
}

/**
    读取指定月份指定总账科目的余额（参数fid：总账科目id，v是余额值，dir是方向，key为币种代码，
    wv是以本币形式表示的外币余额（仅对于需要保存外币余额的科目而言））
    v和wv的键为币种代码
*/
bool BusiUtil::readExtraForSub2(int y, int m, int fid, QHash<int, Double> &v,
                                QHash<int,Double>& wv, QHash<int, int> &dir)
{
    QSqlQuery q;
    QString s,s1;

    //获取总账科目余额对应的字段名称
    s = QString("select subCode from FirSubjects where id = %1").arg(fid);
    bool r = q.exec(s);
    r = q.first();
    if(!r){
        QMessageBox::information(0, QObject::tr("提示信息"),
                                 QString(QObject::tr("不能获取id为%1的总账科目代码")).arg(fid));
        return false;
    }
    QString code = q.value(0).toString();
    char c = 'A' + code.left(1).toInt() - 1;
    QString fname;
    fname.append(c).append(code);

    s = QString("select mt,%1 from SubjectExtras where (year = %2) and "
                "(month = %3)").arg(fname).arg(y).arg(m);    
    if(!q.exec(s)){
        qDebug() << QObject::tr("在获取%1年%2月id为%3的总账科目余额值时发生错误")
                    .arg(y).arg(m).arg(fid);
        return false;
    }

    v.clear();
    wv.clear();
    dir.clear();

    //如果在SubjectExtras表中没有记录，则SubjectMmtExtras表中也不会有
    //这也表示此科目到余额为0
    if(!q.first()){
        //如果总账科目需要按币种分别核算，则设置本币和所有外币的余额
        //if(isAccMt(fid)){
        //
        //}
        return true;
    }

    q.seek(-1);
    while(q.next())
        v[q.value(0).toInt()] = Double(q.value(1).toDouble());

    //读取方向
    s = QString("select mt,%1 from SubjectExtraDirs where (year = %2) and "
                "(month = %3)").arg(fname).arg(y).arg(m);
    if(!q.exec(s)){
        qDebug() << QObject::tr("在获取%1年%2月id为%3的总账科目余额方向时发生错误")
                    .arg(y).arg(m).arg(fid);
        return false;
    }
    if(!q.first()){
        qDebug() << QObject::tr("在获取%1年%2月id为%3的总账科目余额方向时，发现记录不存在")
                    .arg(y).arg(m).arg(fid);
        return false;
    }
    q.seek(-1);
    while(q.next())
        dir[q.value(0).toInt()] = q.value(1).toInt();

    //如果总账科目需要按币种分别核算，则读取以本币形式保存的外币余额
    if(isAccMt(fid)){
        s = QString("select mt,%1 from SubjectMmtExtras where (year = %2) and "
                    "(month = %3)").arg(fname).arg(y).arg(m);
        if(!q.exec(s)){
            qDebug() << QObject::tr("在获取%1年%2月id为%3的总账科目外币余额值（本币形式）时发生错误")
                        .arg(y).arg(m).arg(fid);
            return false;
        }
        while(q.next())
            wv[q.value(0).toInt()] = Double(q.value(1).toDouble());
    }
    return true;
}


/**
    读取指定月份指定明细科目的余额（参数sid：明细科目id，v是余额值，dir是方向，key为币种代码）
*/
bool BusiUtil::readExtraForDetSub(int y,int m, int sid, QHash<int,double>& v, QHash<int,int>& dir)
{
    QSqlQuery q,q2;
    QString s;
    bool r;

    //获取与该明细科目余额币种对应的总账科目的余额的记录id
    s = QString("select id,mt from SubjectExtras where (year = %1) and "
                "(month = %2)").arg(y).arg(m);
    if(!q.exec(s) || !q.first()){
        //QMessageBox::information(0, QObject::tr("提示信息"),
        //                         QString(QObject::tr("不能获取%1年%2月id为%3的明细科目余额值")).arg(y).arg(m).arg(sid));
        qDebug() << QObject::tr("不能获取%1年%2月的总账科目余额值记录").arg(y).arg(m);
        return false;
    }

    v.clear();
    dir.clear();

    q.seek(-1);
    while(q.next()){
        int seid = q.value(0).toInt();
        int mt = q.value(1).toInt();
        s = QString("select value,dir from detailExtras where (seid = %1) "
                    "and (fsid = %2)").arg(seid).arg(sid);
        if(!q2.exec(s)){
            //QMessageBox::information(0, QObject::tr("提示信息"),
            //                         QString(QObject::tr("不能获取%1年%2月id为%3的明细科目余额值")).arg(y).arg(m).arg(sid));
            qDebug() << QObject::tr("不能获取%1年%2月id为%3的明细科目余额值")
                        .arg(y).arg(m).arg(sid);
            return false;
        }
        if(q2.first()){  //要是有，就只有一个记录
            v[mt] = q2.value(0).toDouble();
            dir[mt] = q2.value(1).toInt();
        }
    }
    return r;
}

bool BusiUtil::readExtraForDetSub2(int y, int m, int sid,
                                   QHash<int, Double> &v,
                                   QHash<int,Double>& wv,
                                   QHash<int, int> &dir)
{
    QSqlQuery q,q2;
    QString s;

    //获取与该明细科目余额币种对应的总账科目的余额的记录id
    s = QString("select id,mt from SubjectExtras where (year = %1) and "
                "(month = %2)").arg(y).arg(m);
    if(!q.exec(s)){
        qDebug() << QObject::tr("在获取%1年%2月的总账科目余额记录时发生错误")
                    .arg(y).arg(m);
        return false;
    }

    v.clear();
    wv.clear();
    dir.clear();

    while(q.next()){
        int seid = q.value(0).toInt();
        int mt = q.value(1).toInt();
        s = QString("select value,dir from detailExtras where (seid = %1) "
                    "and (fsid = %2)").arg(seid).arg(sid);
        if(!q2.exec(s)){
            qDebug() << QObject::tr("在获取%1年%2月id为%3的明细科目余额值时发生错误")
                        .arg(y).arg(m).arg(sid);
            return false;
        }
        if(q2.first()){  //要是有，就只有一个记录
            v[mt] = Double(q2.value(0).toDouble());
            dir[mt] = q2.value(1).toInt();
        }
    }

    //如果明细科目需要以币种分别核算，则必须读取与外币余额对应的本币余额
    if(isAccMtS(sid)){
        //获取与该明细科目外币余额币种对应的总账科目的余额的记录id
        s = QString("select id,mt from SubjectMmtExtras where (year = %1) and "
                    "(month = %2)").arg(y).arg(m);
        if(!q.exec(s)){
            qDebug() << QObject::tr("在获取%1年%2月的总账科目外币余额记录时发生错误")
                        .arg(y).arg(m);
            return false;
        }
        while(q.next()){
            int seid = q.value(0).toInt();
            int mt = q.value(1).toInt();
            s = QString("select value from detailMmtExtras where (seid = %1) "
                        "and (fsid = %2)").arg(seid).arg(sid);
            if(!q2.exec(s)){
                qDebug() << QObject::tr("在获取%1年%2月id为%3的明细科目余额值时发生错误")
                            .arg(y).arg(m).arg(sid);
                return false;
            }
            if(q2.first()){  //要是有，就只有一个记录
                wv[mt] = Double(q2.value(0).toDouble());
            }
        }
    }
    return true;
}


/**
    从表SubjectExtras和SubjectExtraDirs读取指定月份的所有主、子科目余额及其方向
    参数 sums：一级科目余额，ssums：明细科目余额，key：id x 10 + 币种代码
    fdirs和sdirs为一二级科目余额的方向
    ????要不要考虑凭证集的状态？？？
*/
bool BusiUtil::readExtraByMonth(int y,int m, QHash<int,double>& sums,
                             QHash<int,int>& fdirs, QHash<int,double>& ssums, QHash<int,int>& sdirs)
{
    QSqlQuery q,q2;
    QSqlRecord rec;
    QString s;

    s = QString("select * from SubjectExtras where (year = %1) "
            "and (month = %2)").arg(y).arg(m);
    if(!(q.exec(s) && q.first())){
        qDebug() << QString("Dot't exist extra value on %1年%2月").arg(y).arg(m);
        return false;
    }
    rec = q.record();
    q.seek(-1);//定位到第一条记录的前面

    //获取一级科目id到保存科目余额值的字段名的映射
    QHash<int,QString>names;
    getFidToFldName(names);
    QHashIterator<int,QString> i(names);

    //获取一级科目余额值和明细科目余额值及其方向
    while(q.next()){
        int seid;//保存一级科目余额值记录的id值
        int mt = q.value(SE_MT).toInt(); //余额的货币类型
        seid = q.value(0).toInt();
        i.toFront();
        while(i.hasNext()){ //读取一级科目余额
            i.next();
            int idx = rec.indexOf(i.value());
            double v = q.value(idx).toDouble();
            if(v != 0)
                sums[i.key()*10+mt] = v;
        }
        //获取明细科目的余额及其方向
        s = QString("select * from detailExtras where seid = %1")
                .arg(seid);
        if(!q2.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取%1年%2月的明细科目科目余额值")).arg(y).arg(m));
            return false;
        }
        while(q2.next()){
            int sid = q2.value(DE_FSID).toInt(); //明细科目id
            ssums[sid*10+mt] = q2.value(DE_VALUE).toDouble(); //余额值
            sdirs[sid*10+mt] = q2.value(DE_DIR).toInt();      //方向
        }
    }

    //获取一级科目余额方向
    s = QString("select * from SubjectExtraDirs where (year = %1) "
            "and (month = %2)").arg(y).arg(m);
    if(!(q.exec(s) && q.first())){
        qDebug() << QString("Dot't exist extra value on %1年%2月").arg(y).arg(m);
        return false;
    }
    rec = q.record();
    q.seek(-1);//定位到第一条记录的前面
    while(q.next()){
        int mt = q.value(SED_MT).toInt(); //余额方向对应的币种
        i.toFront();
        while(i.hasNext()){ //读取一级科目余额
            i.next();
            int idx = rec.indexOf(i.value());
            int dir = q.value(idx).toInt();
            if(dir != 0)
                fdirs[i.key()*10+mt] = dir;
        }
    }
    return true;
}

bool BusiUtil::readExtraByMonth2(int y, int m, QHash<int, Double> &sums, QHash<int, int> &fdirs, QHash<int, Double> &ssums, QHash<int, int> &sdirs)
{
    QSqlQuery q,q2;
    QSqlRecord rec;
    QString s;

    s = QString("select * from SubjectExtras where (year = %1) "
            "and (month = %2)").arg(y).arg(m);
    if(!(q.exec(s) && q.first())){
        qDebug() << QString("Dot't exist extra value on %1年%2月").arg(y).arg(m);
        return false;
    }
    rec = q.record();
    q.seek(-1);//定位到第一条记录的前面

    //获取一级科目id到保存科目余额值的字段名的映射
    QHash<int,QString>names;
    getFidToFldName(names);
    QHashIterator<int,QString> i(names);

    //获取一级科目余额值和明细科目余额值及其方向
    while(q.next()){
        int seid;//保存一级科目余额值记录的id值
        int mt = q.value(SE_MT).toInt(); //余额的货币类型
        seid = q.value(0).toInt();
        i.toFront();
        while(i.hasNext()){ //读取一级科目余额
            i.next();
            int idx = rec.indexOf(i.value());
            double v = q.value(idx).toDouble();
            if(v != 0)
                sums[i.key()*10+mt] = Double(v);
        }
        //获取明细科目的余额及其方向
        s = QString("select * from detailExtras where seid = %1")
                .arg(seid);
        if(!q2.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取%1年%2月的明细科目科目余额值")).arg(y).arg(m));
            return false;
        }
        while(q2.next()){
            int sid = q2.value(DE_FSID).toInt(); //明细科目id
            ssums[sid*10+mt] = Double(q2.value(DE_VALUE).toDouble()); //余额值
            sdirs[sid*10+mt] = q2.value(DE_DIR).toInt();      //方向
        }
    }

    //获取一级科目余额方向
    s = QString("select * from SubjectExtraDirs where (year = %1) "
            "and (month = %2)").arg(y).arg(m);
    if(!(q.exec(s) && q.first())){
        qDebug() << QString("Dot't exist extra value on %1年%2月").arg(y).arg(m);
        return false;
    }
    rec = q.record();
    q.seek(-1);//定位到第一条记录的前面
    while(q.next()){
        int mt = q.value(SED_MT).toInt(); //余额方向对应的币种
        i.toFront();
        while(i.hasNext()){ //读取一级科目余额
            i.next();
            int idx = rec.indexOf(i.value());
            int dir = q.value(idx).toInt();
            if(dir != 0)
                fdirs[i.key()*10+mt] = dir;
        }
    }
    return true;
}

//读取科目余额，键为科目id*10+币种代码，但金额以本币计
bool BusiUtil::readExtraByMonth3(int y, int m, QHash<int, Double> &sumsR,
                                 QHash<int, int> &fdirsR,
                                 QHash<int, Double> &ssumsR,
                                 QHash<int, int> &sdirsR)
{
    QSqlQuery q,q2;
    QSqlRecord rec;
    QString s;

    QHash<int,Double> rates;
    if(!getRates2(y,m,rates))
        return false;

    s = QString("select * from SubjectExtras where (year = %1) "
            "and (month = %2)").arg(y).arg(m);
    if(!(q.exec(s) && q.first())){
        qDebug() << QString("Dot't exist extra value on %1年%2月").arg(y).arg(m);
        return false;
    }
    rec = q.record();
    q.seek(-1);//定位到第一条记录的前面

    //获取一级科目id到保存科目余额值的字段名的映射
    QHash<int,QString>names;
    getFidToFldName(names);
    QHashIterator<int,QString> i(names);

    //获取一级科目余额值和明细科目余额值及其方向
    int seid;             //保存一级科目余额值记录的id值
    bool isWb = false;    //是否是外币
    int mt;
    while(q.next()){
        mt = q.value(SE_MT).toInt(); //余额的货币类型
        if(mt != RMB)
            isWb = true;
        seid = q.value(0).toInt();
        i.toFront();
        int idx;
        Double v;
        while(i.hasNext()){ //读取一级科目余额
            i.next();
            idx = rec.indexOf(i.value());
            v = Double(q.value(idx).toDouble());
            if(v != 0){
                if(isWb)
                    v = v * rates.value(mt);
                sumsR[i.key()*10+mt] = v;
            }
        }
        //获取明细科目的余额及其方向
        s = QString("select * from detailExtras where seid = %1")
                .arg(seid);
        if(!q2.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("不能获取%1年%2月的明细科目科目余额值")).arg(y).arg(m));
            return false;
        }
        while(q2.next()){
            int sid = q2.value(DE_FSID).toInt(); //明细科目id
            v = Double(q2.value(DE_VALUE).toDouble()); //余额值
            if(isWb)
                v = v * rates.value(mt);
            ssumsR[sid*10+mt] = v;
            sdirsR[sid*10+mt] = q2.value(DE_DIR).toInt();      //方向
        }
    }

    //获取一级科目余额方向
    s = QString("select * from SubjectExtraDirs where (year = %1) "
            "and (month = %2)").arg(y).arg(m);
    if(!(q.exec(s) && q.first())){
        qDebug() << QString("Dot't exist extra value on %1年%2月").arg(y).arg(m);
        return false;
    }
    rec = q.record();
    q.seek(-1);//定位到第一条记录的前面
    while(q.next()){
        int mt = q.value(SED_MT).toInt(); //余额方向对应的币种
        i.toFront();
        while(i.hasNext()){ //读取一级科目余额
            i.next();
            int idx = rec.indexOf(i.value());
            int dir = q.value(idx).toInt();
            if(dir != 0)
                fdirsR[i.key()*10+mt] = dir;
        }
    }
    return true;
}

//从subjectMmtExtra表和detailMmtExtra表中读取科目的精确的以本币记的外币（转换为本币的）余额
//参数 sumsR和ssumsR表示主目和子目的余额，
//exist表示指定年月的外币余额是读取自总余额表还是外币余额表（这发生在读取账户记账开始月份时，
//用读取自总余额表的外币本币值直接乘以对应汇率而得到）
bool BusiUtil::readExtraByMonth4(int y, int m, QHash<int, Double> &sumsR,
                                 QHash<int, Double> &ssumsR,
                                 bool& exist)
{
    QSqlQuery q;
    QString s;

    s = QString("select * from %1 where %2=%3 and %4=%5").arg(tbl_sem)
            .arg(fld_sem_year).arg(y).arg(fld_sem_month).arg(m);
    if(!q.exec(s))
        return false;

    QList<int> mts = allMts.keys();
    mts.removeOne(RMB);

    QHash<int,QString> fldNames;  //键为科目代码，值为保存科目代码的字段名
    QList<int> ids;            //科目id列表
    getFidToFldName2(fldNames);
    ids = fldNames.keys();

    //读取主目余额
    //如果表中没有指定年月的余额记录（比如期初），则采用直接将外币余额值转换为本币的值
    if(!q.first()){
        QHash<int,int> dirs;
        if(!readExtraByMonth3(y,m,sumsR,dirs,ssumsR,dirs))
            return false;
        exist = false;
        //去除多余的主科目余额
        int id,mt;
        QList<int> sids;  //子目id列表
        QHashIterator<int,Double>* it = new QHashIterator<int,Double>(sumsR);
        while(it->hasNext()){
            it->next();
            id = it->key()/10;
            mt = it->key()%10;
            if(!mts.contains(mt) ||                          //不是外币
                    (mts.contains(mt) && !fldNames.contains(id)))//是外币，但科目不要求按币种核算
                sumsR.remove(it->key());
        }

        //获取所有需要按币种核算的子目id列表
        QList<int> ss; QList<QString> ns;
        for(int i = 0; i < ids.count(); ++i){
            getSndSubInSpecFst(ids[i],ss,ns);
            sids<<ss;
        }
        //去除多余子目余额
        it = new QHashIterator<int,Double>(ssumsR);
        while(it->hasNext()){
            it->next();
            id = it->key()/10;
            mt = it->key()%10;
            if(!mts.contains(mt) ||
                (mts.contains(mt) && !sids.contains(id)))
                ssumsR.remove(it->key());
        }
        return true;
    }
    exist = true;
    q.seek(-1);
    int seid,fsid,mt,idx,key;
    QList<int> seids;   //seid列表（subjectMmtExtras表保存余额的记录id列表，每个币种对应一个记录）
    QHash<int,int> seidToMt;  //subjectMmtExtras表中的记录对应的币种代码
    QSqlRecord rec;
    rec = q.record();
    while(q.next()){
        seid = q.value(0).toInt();
        seids<<seid;
        mt = q.value(SEM_MT).toInt();
        seidToMt[seid] = mt;
        for(int i = 0; i < ids.count(); ++i){
            idx = rec.indexOf(fldNames.value(ids[i]));
            key = ids[i] * 10 + mt;
            sumsR[key] = Double(q.value(idx).toDouble());
        }
    }

    //读取子目余额
    for(int i = 0; i < seids.count(); ++i){
        s = QString("select * from %1 where %2=%3").arg(tbl_sdem)
                .arg(fld_sdem_seid).arg(seids[i]);
        if(!q.exec(s))
            return false;
        while(q.next()){
            fsid = q.value(SDEM_FSID).toInt();
            key = fsid * 10 + seidToMt.value(seids[i]);
            ssumsR[key] = Double(q.value(SDEM_VALUE).toDouble());
        }
    }
    return true;
}


/**
    读取指定月份-指定明细科目-指定币种的余额（参数sid：明细科目id，mt：币种代码，v是余额值，dir是方向）
*/
bool BusiUtil::readDetExtraForMt(int y,int m, int sid, int mt, double& v, int& dir)
{
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("select id from SubjectExtras where (year=%1) and (month=%2) "
                "and (mt=%3)").arg(y).arg(m).arg(mt);
    if(!q.exec(s) || !q.first()){
        qDebug() << QObject::tr("不能获取%1年%2月，币种代码为%3的余额记录")
                    .arg(y).arg(m).arg(mt);
        return false;
    }
    int seid = q.value(0).toInt();
    s = QString("select value,dir from detailExtras where (seid=%1) and (fsid=%2)")
            .arg(seid).arg(sid);
    if(!q.exec(s) || !q.first()){
        qDebug() << QObject::tr("不能获取%1年%2月，明细科目id=%3，币种代码为%4的余额记录")
                    .arg(y).arg(m).arg(sid).arg(mt);
        return false;
    }
    v= q.value(0).toDouble();
    dir = q.value(1).toInt();
    return true;
}

bool BusiUtil::readDetExtraForMt2(int y, int m, int sid, int mt, Double &v, int &dir)
{
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("select id from SubjectExtras where (year=%1) and (month=%2) "
                "and (mt=%3)").arg(y).arg(m).arg(mt);
    if(!q.exec(s)){
        qDebug() << QObject::tr("不能获取%1年%2月，币种代码为%3的余额记录")
                    .arg(y).arg(m).arg(mt);
        return false;
    }
    if(!q.first()){
        v = 0.00;
        dir = DIR_P;
        return true;
    }
    int seid = q.value(0).toInt();
    s = QString("select value,dir from detailExtras where (seid=%1) and (fsid=%2)")
            .arg(seid).arg(sid);
    if(!q.exec(s)){
        qDebug() << QObject::tr("不能获取%1年%2月，明细科目id=%3，币种代码为%4的余额记录")
                    .arg(y).arg(m).arg(sid).arg(mt);
        return false;
    }
    if(!q.first()){
        v = 0.00;
        dir = DIR_P;
    }
    else{
        v= Double(q.value(0).toDouble());
        dir = q.value(1).toInt();
    }
    return true;
}

///**
//    保存期初余额值。key为一二级科目的id * 10 + 币种代码
//    带F的是总账科目，带S的是明细科目
//    基本实现测率：分两步走，先处理总账，后处理明细账
//        对于每个大步，首先比较新、老值集，将需要更新、新增的科目加入到更新列表，将需要清零的科目
//        加入到清零列表，然后分别处理
//*/
//bool BusiUtil::savePeriodBeginValues(int y, int m, QHash<int, double> oldF, QHash<int, int> oldFDir,
//                                     QHash<int, double> oldS, QHash<int, int> oldSDir,
//                                     QHash<int, double> newF, QHash<int, int> newFDir,
//                                     QHash<int, double> newS, QHash<int, int> newSDir)
//{

//    QHash<int,int> exists; //SubjectExtras表中已有的保存各币种余额值记录的id(key是币种代码，value是记录id)
//    QSqlQuery q;
//    QString s;

//    s = QString("select id,mt from SubjectExtras where (year = %1) "
//                "and (month = %2)").arg(y).arg(m);
//    bool r = q.exec(s);
//    while(q.next()){
//        exists[q.value(1).toInt()] = q.value(0).toInt();
//    }

//    //第一步 处理总账科目余额
//    QHash<int,QString> fldNames; //一级科目id到保存其余额的字段名
//    BusiUtil::getFidToFldName(fldNames);

//    QHash<int,double> dels(oldF);  //要删除的总账科目余额表（对于总账科目的余额，删除只是将其字段值清零，因为他们共用同一条记录）key为科目的id * 10 + 币种代码
//    QHash<int,QList<int> > ups;    //需要更新值的科目id列表，key是币种代码，value是要更新的科目id列表
//    QHash<int,QList<int> > resets; //需要清零的科目id列表，key是币种代码，value需要清零的科目id

//    QHashIterator<int,double>* it = new QHashIterator<int,double>(newF);
//    while(it->hasNext()){
//        it->next();
//        if(oldF.contains(it->key())){ //如果新老值集都包含该科目，则无需删除
//            dels.remove(it->key());
//            if(it->value() != oldF.value(it->key())){ //如果值发生了改变，则将该科目的id加入到更新表
//                ups[it->key() % 10].append(it->key() / 10);
//            }
//        }
//        else
//            ups[it->key() % 10].append(it->key() / 10); //新增的条目当然也加入到更新列表，因为对于总账科目余额值的添加和更新都在一个记录上进行，执行的都是SQL的更新操作
//    }
//    //构造需清零的科目id列表
//    if(dels.count()>0){
//        it = new QHashIterator<int,double>(dels);
//        while(it->hasNext()){
//            it->next();
//            resets[it->key() % 10].append(it->key() / 10);
//        }
//    }

//    //保存改变或新增的一级科目余额值
//    QHashIterator<int,QList<int> >* ii = new QHashIterator<int,QList<int> >(ups);
//    while(ii->hasNext()){
//        ii->next();
//        if(!exists.contains(ii->key())){ //构造插入语句（也即是说，在SubjectExtra表中还没有对应于该币种的余额的记录）
//            s = QString("insert into SubjectExtras(year,month,state,mt,");
//            QString vs = QString("values(%1,%2,%3,%4,")
//                    .arg(y).arg(m).arg(1).arg(ii->key());
//            for(int i = 0; i < ii->value().count(); ++i){
//                s.append(fldNames.value(ii->value()[i]));
//                s.append(",");
//                vs.append(QString::number(newF.value(ii->value()[i] * 10 + ii->key()),'f',2));
//                vs.append(",");
//            }
//            s.chop(1);
//            s.append(") ");
//            vs.chop(1);
//            vs.append(")");
//            s.append(vs);
//            r = q.exec(s);
//            //回读该条记录的id值
//            s = QString("select id from SubjectExtras where (year = %1) "
//                    "and (month = %2) and (mt = %3)").arg(y).arg(m).arg(ii->key());
//            r = q.exec(s); r = q.first();
//            exists[ii->key()] = q.value(0).toInt();
//        }
//        else{ //构造更新语句（也即是说，在SubjectExtra表中已经存在对应于该币种的余额的记录）
//            s = QString("update SubjectExtras set ");
//            for(int i = 0; i < ii->value().count(); ++i){
//                s.append(QString("%1=%2,").arg(fldNames.value(ii->value()[i]))
//                         .arg(newF.value(ii->value()[i] * 10 + ii->key())));
//            }
//            s.chop(1);
//            s.append(QString(" where (year=%1) and (month=%2) and (mt=%3)")
//                     .arg(y).arg(m).arg(ii->key()));
//            r = q.exec(s);
//        }
//    }

//    //对已不存在的余额值清零
//    ii = new QHashIterator<int,QList<int> >(resets);
//    while(ii->hasNext()){
//        ii->next();
//        s = QString("update SubjectExtras set ");
//        for(int i = 0; i < ii->value().count(); ++i){
//            s.append(QString("%1=0,").arg(fldNames.value(ii->value()[i])));
//        }
//        s.chop(1);s.append(" ");
//        s.append(QString("where (year=%1) and (month=%2) and (mt=%3)")
//                 .arg(y).arg(m).arg(ii->key()));
//        r = q.exec(s);
//    }

//    //第二步 处理明细科目余额
//    dels.clear();
//    dels = oldS;
//    QList<int> upLst,newLst,deLst; //要更新、新增和删除的键列表
//    it = new QHashIterator<int,double>(newS);
//    while(it->hasNext()){
//        it->next();
//        if(oldS.contains(it->key())){
//            dels.remove(it->key());
//            if(it->value() != oldS.value(it->key())){
//                upLst.append(it->key());
//            }
//        }
//        else
//            newLst.append(it->key());
//    }
//    deLst = dels.keys();

//    //删除操作
//    for(int i = 0; i < deLst.count(); ++i){
//        s = QString("delete from detailExtras where (seid = %1) "
//                    "and (fsid = %2)").arg(exists.value(deLst[i]%10))
//                .arg(deLst[i]/10);
//        r = q.exec(s);
//    }
//    //更新操作
//    for(int i = 0; i < upLst.count(); ++i){
//        s = QString("update detailExtras set value = %1 "
//                    "where (seid = %2) and (fsid = %3)")
//                .arg(newS.value(upLst[i]))
//                .arg(exists.value(upLst[i]%10))
//                .arg(upLst[i]/10);
//        r = q.exec(s);
//    }

//    //插入操作
//    for(int i = 0; i < newLst.count(); ++i){
//        s = QString("insert into detailExtras(seid,fsid,value) "
//                    "values(%1,%2,%3)").arg(exists.value(newLst[i]%10))
//                .arg(newLst[i]/10).arg(newS.value(newLst[i]));
//        r = q.exec(s);
//    }
//    if(r)
//        return true;
//    else
//        return false;
//}

/**
    此方法用来保存设定的期初值，和当期余额值。key为一二级科目的id * 10 + 币种代码
    isSetup true：表示保存当期余额，false：保存设定的期初值。
    基本实现策略：

*/
bool BusiUtil::savePeriodBeginValues(int y, int m, QHash<int, double> newF, QHash<int, int> newFDir,
                                     QHash<int, double> newS, QHash<int, int> newSDir,
                                     PzsState& pzsState, bool isSetup)
{
    QSqlQuery q;
    QString s;

    QHash<int,QString> fldNames; //一级科目id到保存其余额的字段名
    BusiUtil::getFidToFldName(fldNames);

    //1、初始化exist表，此表是SubjectExtras表中已有的保存各币种余额值记录的id
    //(key是币种代码，value是记录id)
    QHash<int,int> exists;
    s = QString("select id,mt from SubjectExtras where (year = %1) "
                "and (month = %2)").arg(y).arg(m);
    bool r = q.exec(s);
    while(q.next())
        exists[q.value(1).toInt()] = q.value(0).toInt();

    //获取所有币种代码列表
    QHash<int,QString> mtHash; //币种代码和名称
    getMTName(mtHash);
    QList<int> mtLst = mtHash.keys(); //币种代码的列表

    //获取所有本账户内启用的总账和明细帐科目id
    QList<int> fids,sids;
    r = q.exec("select id from FirSubjects where isView = 1");
    while(q.next())
        fids << q.value(0).toInt();
    r = q.exec("select id from FSAgent");
    while(q.next())
        sids << q.value(0).toInt();

    //读取老值集
    QHash<int, double> oldF,oldS;   //余额
    QHash<int,int>     odF, odS;    //方向
    readExtraByMonth(y,m,oldF,odF,oldS,odS);

    //初始化总账科目余额和方向的sql更新语句
    QString s1,s2,vs1,vs2;
    int key;    
    bool oc,nc; //分别表示某个指定余额值是否存在于老值或新值集中
    bool flag  = false; //是否需要执行sql语句的标记
    //1、处理总账科目余额及方向
    //遍历所有币种和总账科目id的组合键来匹配键值
    for(int i = 0; i < mtLst.count(); ++i){
        if(!exists.contains(mtLst[i])){ //不存在则执行插入操作
            s1 = "insert into SubjectExtras(year,month,mt,";
            s2 = "insert into SubjectExtraDirs(year,month,mt,";
            vs1 = QString(" values(%1,%2,%3,").arg(y).arg(m).arg(mtLst[i]);
            vs2 = QString(" values(%1,%2,%3,").arg(y).arg(m).arg(mtLst[i]);
            for(int j = 0; j < fids.count(); ++j){
                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
                if(newF.contains(key)){
                    s1.append(fldNames.value(fids[j])).append(",");
                    s2.append(fldNames.value(fids[j])).append(",");
                    vs1.append(QString("%1,").arg(QString::number(newF.value(key),'f',2)));
                    vs2.append(QString("%1,").arg(QString::number(newFDir.value(key))));
                }
            }
            s1.chop(1); s1.append(")");
            vs1.chop(1); vs1.append(")");
            s1.append(vs1);
            s2.chop(1); s2.append(")");
            vs2.chop(1); vs2.append(")");
            s2.append(vs2);
            r = q.exec(s1);
            r = q.exec(s2);
            //回读seid
            s1 = QString("select id from SubjectExtras where (year=%1) and "
                         "(month=%2) and (mt=%3)").arg(y).arg(m).arg(mtLst[i]);
            r = q.exec(s1); r = q.first();
            exists[mtLst[i]] = q.value(0).toInt();
        }
        else{  //存在则执行更新操作
            s1 = "update SubjectExtras set ";
            s2 = "update SubjectExtraDirs set ";
            for(int j = 0; j < fids.count(); ++j){
                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
                nc = newF.contains(key);
                oc = oldF.contains(key);                
                if(!nc && !oc)
                    continue;
                else if(oldF.value(key) == newF.value(key)){ //值没变但方向变了
                    if(odF.value(key) !=  newFDir.value(key)){
                        flag = true;
                        s2.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
                                  .arg(QString::number(newFDir.value(key))));
                    }                    
                }
                else if(!nc && oc){ //如果新值没有老值有，则清零
                    flag = true;
                    s1.append(QString("%1=0,").arg(fldNames.value(fids[j])));
                    s2.append(QString("%1=0,").arg(fldNames.value(fids[j])));
                }
                else{
                    flag = true;
                    s1.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
                              .arg(QString::number(newF.value(key),'f',2)));
                    s2.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
                              .arg(QString::number(newFDir.value(key))));
                }
            }
            if(flag){
                s1.chop(1);
                s2.chop(1);
                QString ws = QString(" where (year=%1) and (month=%2) and (mt=%3)")
                        .arg(y).arg(m).arg(mtLst[i]);
                s1.append(ws); s2.append(ws);
                r = q.exec(s1);
                r = q.exec(s2);
            }
        }
    }

    //2、处理明细账科目余额及方向
    for(int i = 0; i < mtLst.count(); ++i){
        for(int j = 0; j < sids.count(); ++j){
            key = sids[j] * 10 + mtLst[i];
            oc = oldS.contains(key); //老值集是否包含该键
            nc = newS.contains(key); //新值集是否包含该键
            if(!oc && !nc)                //如果新老值都不包含，则跳过
                continue;
            else if(oc && !nc){ //如果老值包含新值不包含则要删除
                s = QString("delete from detailExtras where (seid=%1) and (fsid=%2) ")
                            .arg(exists.value(mtLst[i])).arg(sids[j]);
                r = q.exec(s);                
            }
            else if(!oldS.contains(key) && newS.contains(key)){ //如果新值包含老值不包含则要新增
                s = QString("insert into detailExtras(seid,fsid,dir,value) "
                            "values(%1,%2,%3,%4)").arg(exists.value(mtLst[i]))
                        .arg(sids[j]).arg(QString::number(newSDir.value(key)))
                        .arg(QString::number(newS.value(key),'f',2));
                r = q.exec(s);                
            }
            else if((oldS.value(key) != newS.value(key))
                    || (odS.value(key) != newSDir.value(key))){ //值或方向改变了
                s = QString("update detailExtras set dir=%1,value=%2 where "
                            "(seid=%3) and (fsid=%4)").arg(QString::number(newSDir.value(key)))
                        .arg(QString::number(newS.value(key),'f',2))
                        .arg(exists.value(mtLst[i])).arg(sids[j]);
                r = q.exec(s);
            }
        }
    }

    //应根据当前凭证集内的凭证状态和凭证集先前到状态来决定此时的凭证集状态

    //判定保存余额后的凭证集的新状态
    //PzsState pzsState;
    if(!isSetup)  //如果是保存设定的期初值，则凭证集状态为结账        
        setPzsState(y,m,Ps_Jzed);
    else{        
        if(!getPzsState(y,m,pzsState)){  //如果不能读取凭证集状态，则假定它是初始态
            pzsState = Ps_Rec;
            setPzsState(y,m,pzsState);
        }
        else{
            bool stateChanged = false;
            bool req;
            switch(pzsState){
            case Ps_HandV:
                pzsState = Ps_Stat1;
                stateChanged = true;
                break;
            case Ps_ImpV:
                reqGenJzHdsyPz(y,m,req);
                if(req)
                    pzsState = Ps_Stat2;
                else
                    pzsState = Ps_JzsyPre;
                stateChanged = true;
                break;
            case Ps_JzhdV:
                reqGenImpOthPz(y,m,req);
                if(req)
                    pzsState = Ps_Stat3;
                else
                    pzsState = Ps_JzsyPre;
                stateChanged = true;
                break;
            case Ps_JzsyV:
                //如果配置为每年年底执行一次本年利润的结转，且月份不是12月份，则直接准备结账
                if(jzlrByYear && (m != 12))
                    pzsState = Ps_Stat5;
                else{
                    pzsState = Ps_Stat4;
                }
                stateChanged = true;
                break;
            case Ps_JzbnlrV:
                pzsState = Ps_Stat5;
                stateChanged = true;
                break;
            }
            //保存凭证集状态
            if(stateChanged)
                setPzsState(y,m,pzsState);
        }
    }
    return r;
}

/**
 * @brief BusiUtil::savePeriodBeginValues2
 *  保存原币余额到表SubjectExtras，detailExtras
 * @param y
 * @param m
 * @param newF
 * @param newFDir
 * @param newS
 * @param newSDir
 * @param pzsState
 * @param isSetup
 * @return
 */
bool BusiUtil::savePeriodBeginValues2(int y, int m, QHash<int, Double> newF, QHash<int, int> newFDir, QHash<int, Double> newS, QHash<int, int> newSDir, PzsState &pzsState, bool isSetup)
{
    QSqlQuery q;
    QString s;

    QHash<int,QString> fldNames; //一级科目id到保存其余额的字段名
    BusiUtil::getFidToFldName(fldNames);

    //1、初始化exist表，此表是SubjectExtras表中已有的保存各币种余额值记录的id
    //(key是币种代码，value是记录id)
    QHash<int,int> exists;
    s = QString("select id,mt from SubjectExtras where (year = %1) "
                "and (month = %2)").arg(y).arg(m);
    bool r = q.exec(s);
    while(q.next())
        exists[q.value(1).toInt()] = q.value(0).toInt();

    //获取所有币种代码列表
    QHash<int,QString> mtHash; //币种代码和名称
    getMTName(mtHash);
    QList<int> mtLst = mtHash.keys(); //币种代码的列表

    //获取所有本账户内启用的总账和明细帐科目id
    QList<int> fids,sids;
    r = q.exec("select id from FirSubjects where isView = 1");
    while(q.next())
        fids << q.value(0).toInt();
    r = q.exec("select id from FSAgent");
    while(q.next())
        sids << q.value(0).toInt();

    //读取老值集
    QHash<int, Double> oldF,oldS;   //余额
    QHash<int,int>     odF, odS;    //方向
    readExtraByMonth2(y,m,oldF,odF,oldS,odS);

    //初始化总账科目余额和方向的sql更新语句
    QString s1,s2,vs1,vs2;
    int key;
    bool oc,nc; //分别表示某个指定余额值是否存在于老值或新值集中
    bool flag  = false; //是否需要执行sql语句的标记
    //1、处理总账科目余额及方向
    //遍历所有币种和总账科目id的组合键来匹配键值
    for(int i = 0; i < mtLst.count(); ++i){
        if(!exists.contains(mtLst[i])){ //不存在则执行插入操作
            s1 = "insert into SubjectExtras(year,month,mt,";
            s2 = "insert into SubjectExtraDirs(year,month,mt,";
            vs1 = QString(" values(%1,%2,%3,").arg(y).arg(m).arg(mtLst[i]);
            vs2 = QString(" values(%1,%2,%3,").arg(y).arg(m).arg(mtLst[i]);
            for(int j = 0; j < fids.count(); ++j){
                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
                if(newF.contains(key)){
                    s1.append(fldNames.value(fids[j])).append(",");
                    s2.append(fldNames.value(fids[j])).append(",");
                    QString sv;
                    if(newF.value(key) == 0)
                        sv = "0";
                    else
                        sv = newF.value(key).toString();
                    vs1.append(QString("%1,").arg(sv));
                    vs2.append(QString("%1,").arg(newFDir.value(key)));
                }
            }
            s1.chop(1); s1.append(")");
            vs1.chop(1); vs1.append(")");
            s1.append(vs1);
            s2.chop(1); s2.append(")");
            vs2.chop(1); vs2.append(")");
            s2.append(vs2);
            r = q.exec(s1);
            r = q.exec(s2);
            //回读seid
            s1 = QString("select id from SubjectExtras where (year=%1) and "
                         "(month=%2) and (mt=%3)").arg(y).arg(m).arg(mtLst[i]);
            r = q.exec(s1); r = q.first();
            exists[mtLst[i]] = q.value(0).toInt();
        }
        else{  //存在则执行更新操作
            s1 = "update SubjectExtras set ";
            s2 = "update SubjectExtraDirs set ";
            for(int j = 0; j < fids.count(); ++j){
                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
                nc = newF.contains(key);
                oc = oldF.contains(key);
                if(!nc && !oc)  //都不存在则忽略
                    continue;
                else if(oldF.value(key) == newF.value(key)){ //值没变但方向变了
                    if(odF.value(key) !=  newFDir.value(key)){
                        flag = true;
                        s2.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
                                  .arg(QString::number(newFDir.value(key))));
                    }
                }
                else if(!nc && oc){ //如果新值没有老值有，则清零
                    flag = true;
                    s1.append(QString("%1=0,").arg(fldNames.value(fids[j])));
                    s2.append(QString("%1=0,").arg(fldNames.value(fids[j])));
                }
                else{
                    flag = true;
                    QString sv;
                    if(newF.value(key) == 0)
                        sv = "0";
                    else
                        sv = newF.value(key).toString();
                    s1.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
                              .arg(sv));
                    s2.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
                              .arg(newFDir.value(key)));
               }
            }
            if(flag){
                s1.chop(1);
                s2.chop(1);
                QString ws = QString(" where (year=%1) and (month=%2) and (mt=%3)")
                        .arg(y).arg(m).arg(mtLst[i]);
                s1.append(ws); s2.append(ws);
                r = q.exec(s1);
                r = q.exec(s2);
            }
        }
    }

    //2、处理明细账科目余额及方向
    for(int i = 0; i < mtLst.count(); ++i){
        for(int j = 0; j < sids.count(); ++j){
            key = sids[j] * 10 + mtLst[i];
            oc = oldS.contains(key); //老值集是否包含该键
            nc = newS.contains(key); //新值集是否包含该键
            if(!oc && !nc)                //如果新老值都不包含，则跳过
                continue;
            else if(oc && !nc){ //如果老值包含新值不包含则要删除
                s = QString("delete from detailExtras where (seid=%1) and (fsid=%2) ")
                            .arg(exists.value(mtLst[i])).arg(sids[j]);
                r = q.exec(s);
            }
            else if(!oldS.contains(key) && newS.contains(key)){ //如果新值包含老值不包含则要新增
                //注意：如果这里直接使用Double::getv()方法，由于返回的双精度实数使用了科学计数法，
                //当数位超过7位时，看起来好像失去了精度（最后几位好像被省略了）
                //因此，这里我使用了字符串来代替，以输出实际的实数值。
                QString sv;
                if(newS.value(key) == 0)
                    sv = "0";
                else
                    sv = newS.value(key).toString();
                s = QString("insert into detailExtras(seid,fsid,dir,value) "
                            "values(%1,%2,%3,%4)").arg(exists.value(mtLst[i]))
                        .arg(sids[j]).arg(newSDir.value(key))
                        .arg(sv);
                r = q.exec(s);
            }
            else if((oldS.value(key) != newS.value(key))
                    || (odS.value(key) != newSDir.value(key))){ //值或方向改变了
                QString sv;
                if(newS.value(key) == 0)
                    sv = "0";
                else
                    sv = newS.value(key).toString();
                s = QString("update detailExtras set dir=%1,value=%2 where "
                            "(seid=%3) and (fsid=%4)").arg(newSDir.value(key))
                        .arg(sv).arg(exists.value(mtLst[i])).arg(sids[j]);
                r = q.exec(s);
            }
        }
    }

    //应根据当前凭证集内的凭证状态和凭证集先前到状态来决定此时的凭证集状态

    //判定保存余额后的凭证集的新状态
    //PzsState pzsState;
    if(!isSetup)  //如果是保存设定的期初值，则凭证集状态为结账
        setPzsState(y,m,Ps_Jzed);
    else{
        if(!getPzsState(y,m,pzsState)){  //如果不能读取凭证集状态，则假定它是初始态
            pzsState = Ps_Rec;
            setPzsState(y,m,pzsState);
        }
        else{
            bool stateChanged = false;
            bool req;
            switch(pzsState){
            case Ps_HandV:
                pzsState = Ps_Stat1;
                stateChanged = true;
                break;
            case Ps_ImpV:
                reqGenJzHdsyPz(y,m,req);
                if(req)
                    pzsState = Ps_Stat2;
                else
                    pzsState = Ps_JzsyPre;
                stateChanged = true;
                break;
            case Ps_JzhdV:
                reqGenImpOthPz(y,m,req);
                if(req)
                    pzsState = Ps_Stat3;
                else
                    pzsState = Ps_JzsyPre;
                stateChanged = true;
                break;
            case Ps_JzsyV:
                //如果配置为每年年底执行一次本年利润的结转，且月份不是12月份，则直接准备结账
                if(jzlrByYear && (m != 12))
                    pzsState = Ps_Stat5;
                else{
                    pzsState = Ps_Stat4;
                }
                stateChanged = true;
                break;
            case Ps_JzbnlrV:
                pzsState = Ps_Stat5;
                stateChanged = true;
                break;
            }
            //保存凭证集状态
            if(stateChanged)
                setPzsState(y,m,pzsState);
        }
    }
    return r;
}

//保存科目的外币（转换为人民币）余额(SubjectMmtExtras,detailMmtExtras)
bool BusiUtil::savePeriodEndValues(int y, int m, QHash<int, Double> newF,
                                    QHash<int, Double> newS)
{
    QSqlQuery q;
    QString s;

    //保存主科目
    QHash<int,QString> fldNames; //需要按币种核算的一级科目id到保存其余额的字段名
    BusiUtil::getFidToFldName2(fldNames);

    //1、初始化exist表，此表是SubjectMmtExtras表中已有的保存外币余额值记录的id
    //(key是币种代码，value是记录id)
    QHash<int,int> exists;
    s = QString("select id,%1 from %2 where (%3 = %4) "
                "and (%5 = %6)").arg(fld_sem_mt).arg(tbl_sem).arg(fld_sem_year)
            .arg(y).arg(fld_sem_month).arg(m);
    if(!q.exec(s))
        return false;
    while(q.next())
        exists[q.value(1).toInt()] = q.value(0).toInt();

    //获取所有币种代码列表
    QHash<int,QString> mtHash; //币种代码和名称
    getMTName(mtHash);
    QList<int> mtLst = mtHash.keys(); //币种代码的列表
    mtLst.removeOne(RMB);

    //获取所有本账户内启用的需要按币种进行核算的总账和明细帐科目id
    QList<int> fids,sids;
    fids = fldNames.keys();
    for(int i = 0; i < fids.count(); ++i){
        s = QString("select id from %1 where fid=%2")
                .arg(tbl_fsa).arg(fids[i]);
        if(!q.exec(s))
            return false;
        while(q.next())
            sids<<q.value(0).toInt();
    }

    //读取老值集
    QHash<int, Double> oldF,oldS;   //余额
    bool exist;  //指定年月的外币余额是读取自总余额表还是外币余额表，
    readExtraByMonth4(y,m,oldF,oldS,exist); //从subjectMmtExtra表中读取

    //初始化总账科目余额和方向的sql更新语句
    QString s1,vs1;
    int key;
    bool oc,nc; //分别表示某个指定余额值是否存在于老值或新值集中
    bool flag  = false; //是否需要执行sql语句的标记
    //1、处理总账科目余额及方向
    //遍历所有币种和总账科目id的组合键来匹配键值
    for(int i = 0; i < mtLst.count(); ++i){
        if(!exists.contains(mtLst[i])){ //不存在则执行插入操作
            s1 = QString("insert into %1(%2,%3,%4,").arg(tbl_sem)
                    .arg(fld_sem_year).arg(fld_sem_month).arg(fld_sem_mt);
            vs1 = QString(" values(%1,%2,%3,").arg(y).arg(m).arg(mtLst[i]);
            for(int j = 0; j < fids.count(); ++j){
                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
                if(newF.contains(key)){
                    s1.append(fldNames.value(fids[j])).append(",");
                    QString sv;
                    if(newF.value(key) == 0)
                        sv = "0";
                    else
                        sv = newF.value(key).toString();
                    vs1.append(QString("%1,").arg(sv));
                }
            }
            s1.chop(1); s1.append(")");
            vs1.chop(1); vs1.append(")");
            s1.append(vs1);
            if(!q.exec(s1))
                return false;
            //回读seid
            s1 = QString("select id from %1 where (%2=%3) and "
                         "(%4=%5) and (%6=%7)").arg(tbl_sem).arg(fld_sem_year)
                    .arg(y).arg(fld_sem_month).arg(m).arg(fld_sem_mt).arg(mtLst[i]);
            if(!q.exec(s1))
                return false;
            if(!q.first())
                return false;
            exists[mtLst[i]] = q.value(0).toInt();
        }
        else{  //存在则执行更新操作
            s1 = QString("update %1 set ").arg(tbl_sem);
            for(int j = 0; j < fids.count(); ++j){
                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
                nc = newF.contains(key);
                oc = oldF.contains(key);
                if(!nc && !oc)  //都不存在则忽略
                    continue;
                else if(oldF.value(key) == newF.value(key)) //值没变
                    continue;
                else if(!nc && oc){ //如果新值没有老值有，则清零
                    flag = true;
                    s1.append(QString("%1=0,").arg(fldNames.value(fids[j])));
                }
                else{
                    flag = true;
                    QString sv;
                    if(newF.value(key) == 0)
                        sv = "0";
                    else
                        sv = newF.value(key).toString();
                    s1.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
                              .arg(sv));
               }
            }
            if(flag){
                s1.chop(1);
                QString ws = QString(" where (%1=%2) and (%3=%4) and (%5=%6)")
                        .arg(fld_sem_year).arg(y).arg(fld_sem_month).arg(m)
                        .arg(fld_sem_mt).arg(mtLst[i]);
                s1.append(ws);
                if(!q.exec(s1))
                    return false;
            }
        }
    }

    //保存子科目
    for(int i = 0; i < mtLst.count(); ++i){
        for(int j = 0; j < sids.count(); ++j){
            key = sids[j] * 10 + mtLst[i];
            oc = oldS.contains(key); //老值集是否包含该键
            nc = newS.contains(key); //新值集是否包含该键
            if(!oc && !nc)                //如果新老值都不包含，则跳过
                continue;
            //如果老值包含新值不包含，或新值等于0则要删除
            else if((oc && !nc) || (newS.value(key) == 0.00)){
                s = QString("delete from %1 where (%2=%3) and (%4=%5)").arg(tbl_sdem)
                            .arg(fld_sdem_seid).arg(exists.value(mtLst[i]))
                            .arg(fld_sdem_fsid).arg(sids[j]);
                if(!q.exec(s))
                    return false;
            }
            //如果新值包含老值不包含则要新增，或者根本没有创建对应月份记录
            else if(!exist || (!oldS.contains(key) && newS.contains(key))){
                //注意：如果这里直接使用Double::getv()方法，由于返回的双精度实数使用了科学计数法，
                //当数位超过7位时，看起来好像失去了精度（最后几位好像被省略了）
                //因此，这里我使用了字符串来代替，以输出实际的实数值。
                QString sv;
                if(newS.value(key) == 0)
                    sv = "0";
                else
                    sv = newS.value(key).toString();
                s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)")
                            .arg(tbl_sdem).arg(fld_sdem_seid).arg(fld_sdem_fsid)
                        .arg(fld_sdem_value).arg(exists.value(mtLst[i]))
                        .arg(sids[j]).arg(newS.value(key).toString());
                if(!q.exec(s))
                    return false;
            }
            else if(oldS.value(key) != newS.value(key)){ //值改变了
                QString sv;
                if(newS.value(key) == 0)
                    sv = "0";
                else
                    sv = newS.value(key).toString();
                s = QString("update %1 set %2=%3 where (%4=%5) and (%6=%7)")
                            .arg(tbl_sdem).arg(fld_sdem_value).arg(sv)
                        .arg(fld_sdem_seid).arg(exists.value(mtLst[i]))
                        .arg(fld_sdem_fsid).arg(sids[j]);
                if(!q.exec(s))
                    return false;
            }
        }
    }
}


//为查询处于指定状态的某些类别的凭证生成过滤子句
void BusiUtil::genFiltState(QList<int> pzCls, PzState state, QString& s)
{
    QString ts;
    if(!pzCls.empty()){
        for(int i = 0; i < pzCls.count(); ++i){
            ts.append(QString("(isForward=%1) or ").arg(pzCls[i]));
        }
        ts.chop(4);
        if(pzCls.count()>1)
            s = QString("(%1) and (pzState=%2)").arg(ts).arg(state);
        else
            s = QString("%1 and (pzState=%2)").arg(ts).arg(state);
    }
}

/**
 * @brief BusiUtil::genFiltStateForSpecPzCls
 *  生成过滤出指定类别的凭证的条件语句
 * @param pzClses
 */
QString BusiUtil::genFiltStateForSpecPzCls(QList<int> pzClses)
{
    if(pzClses.empty())
        return "";
    QString sql;
    for(int i = 0; i < pzClses.count(); ++i){
        sql.append(QString("(%1=%2) or ").arg(fld_pz_class).arg(pzClses.at(i)));
    }
    sql.chop(4);
    return sql;
}

//删除指定年月的指定类别凭证
bool BusiUtil::delSpecPz(int y, int m, PzClass pzCls)
{
    QString s;
    QSqlQuery q,q2;

    QDate d(y,m,1);
    QString ds = d.toString(Qt::ISODate);
    ds.chop(3);

    s = QString("select id from PingZhengs where (date like '%1%') and (isForward=%2)")
            .arg(ds).arg(pzCls);
    if(q.exec(s)){
        int id;
        while(q.next()){
            id = q.value(0).toInt();
            s = QString("delete from BusiActions where pid=%1").arg(id);
            if(!q2.exec(s))  //删除凭证所属业务活动
                return false;
            s = QString("delete from PingZhengs where id=%1").arg(id);
            if(q2.exec(s))   //删除凭证
                return true;
            else
                return false;
        }
        return true; //没有凭证记录
    }
    else
        return false;
}

/**
    构造统计查询语句（根据当前凭证集的状态来构造将不同类别的凭证纳入统计范围的SQL语句）
*/
bool BusiUtil::genStatSql(int y, int m, QString& s)
{
    QList<int> rpz;    //要查询的处于录入状态的凭证类别列表
    QList<int> inpz;   //要查询的处于入账状态的凭证类别列表
    QString rs;   //录入状态凭证过滤条件
    QString ins;  //入账状态凭证过滤条件

//    QSet<int> states;  //需要作处理的凭证集状态（因为目前还不确定具体哪些状态需要处理）
//    states<<Ps_Ori<<Ps_HandV<<Ps_Stat1<<Ps_ImpOther<<Ps_ImpV<<Ps_Stat2
//          <<Ps_Jzhd<<Ps_Jzhd1<<Ps_Stat3<<Ps_Jzsy<<Ps_Jzsy1<<Ps_Stat4
//          <<Ps_Jzbnlr<<Ps_Jzbnlr1<<Ps_Stat5;

    //获取凭证集状态
    PzsState pzsState;
    if(!getPzsState(y,m,pzsState)){
        pzsState = Ps_Rec;
        setPzsState(y,m,pzsState);
    }
//    if(!states.contains(pzsState))
//        return false;

    switch(pzsState){
    case Ps_Rec: //如果是原始态，则查询所有手工录入的非作废凭证
        rpz<<Pzc_Hand;
        break;
    case Ps_HandV:  //查询所有手工录入的已入账凭证
    case Ps_Stat1:
        inpz<<Pzc_Hand;
        break;

    case Ps_ImpOther:  //查询所有手工录入的已入账凭证，以及由其他模块引入未审核凭证
        inpz<<Pzc_Hand;
        rpz = impPzCls.toList();
        break;
    case Ps_ImpV :  //查询所有手工录入，以及由其他模块引入的已入账凭证
    case Ps_Stat2:
        inpz<<Pzc_Hand<<impPzCls.toList();
        break;

    case Ps_Jzhd:  //查询所有手工录入、其他模块引入的入账凭证，以及结转汇兑损益的未审核凭证
        inpz<<Pzc_Hand<<impPzCls.toList();
        rpz<<jzhdPzCls.toList();
        break;
    case Ps_JzhdV: //查询所有手工录入、其他模块引入以及结转汇兑损益的入账凭证
    case Ps_Stat3:
        inpz<<Pzc_Hand<<impPzCls.toList()<<jzhdPzCls.toList();
        break;

    case Ps_Jzsy: //查询所有手工录入、其他模块引入以及结转汇兑损益的入账凭证，以及结转损益的未审核凭证
        inpz<<Pzc_Hand<<impPzCls.toList()<<jzhdPzCls.toList();
        rpz<<jzsyPzCls.toList();
        break;

    case Ps_JzsyV: //查询所有手工录入、其他模块引入、结转汇兑损益、结转损益的入账凭证
    case Ps_Stat4:
        inpz<<Pzc_Hand<<impPzCls.toList()<<jzhdPzCls.toList()<<jzsyPzCls.toList();
        break;

    case Ps_Jzbnlr: //查询以上所有凭证以及结转本年利润的未审核凭证
        inpz<<Pzc_Hand<<impPzCls.toList()<<jzhdPzCls.toList()<<jzsyPzCls.toList();
        rpz<<Pzc_Jzlr;
        break;

    case Ps_JzbnlrV: //查询以上所有入账凭证
    case Ps_Stat5:
    case Ps_Jzed:
        inpz<<Pzc_Hand<<impPzCls.toList()<<jzhdPzCls.toList()
            <<jzsyPzCls.toList()<<Pzc_Jzlr;
        break;

    }

    if(inpz.empty() && rpz.empty())
        return false;
    else{
        //凭证集的年月时间
        QDate d = QDate(y,m,1);
        QString sd = d.toString(Qt::ISODate);
        sd.chop(3);

        //装配查询语句（初始语句是查询指定年月的所有凭证）
        s = QString("select * from PingZhengs where (date like '%1%')").arg(sd);
        genFiltState(inpz,Pzs_Instat,ins);
        genFiltState(rpz,Pzs_Recording,rs);

        if(!inpz.empty() && !rpz.empty()){
            s.append(QString(" and ((%1) or (%2))").arg(ins).arg(rs));
        }
        else if(!inpz.empty()){
            s.append(QString(" and (%1)").arg(ins));
        }
        else
            s.append(QString(" and (%1)").arg(rs));
    }

    return true;
}

/**
    计算本期发生额，参数jSums：一级科目借方发生额，key = 一级科目id x 10 + 币种代码
                 参数dSums：一级科目贷方发生额，key = 一级科目id x 10 + 币种代码
                    sjSums：明细科目借方发生额，key = 明细科目id x 10 + 币种代码
                    sdSums：明细科目贷方发生额，key = 明细科目id x 10 + 币种代码
                    isSave：是否可以保存余额，amount：参予统计的凭证数
    应根据凭证集的状态来决定参予统计的凭证范围
*/
bool BusiUtil::calAmountByMonth(int y, int m, QHash<int,double>& jSums, QHash<int,double>& dSums,
             QHash<int,double>& sjSums, QHash<int,double>& sdSums,
                                bool& isSave, int& amount)
{
    QSqlQuery q,q2;
    QString s;
    bool r;

    //初始化汇率表
    QHash<int,double>rates;
    if(!getRates(y,m,rates))
        return false;

    //初始化币种表
    QHash<int,QString> mts;
    if(!getMTName(mts))
        return false;
    rates[RMB] = 1;

    //初始化需要进行明细核算的一级科目的id列表
    QList<int> detSubs;
    if(!getReqDetSubs(detSubs))
        return false;

    //生成查询语句
    if(!genStatSql(y,m,s))
        return false;

    if(!q.exec(s)){
        QMessageBox::information(0, QObject::tr("提示信息"),
                                 QString(QObject::tr("在计算本期发生额时不能获取凭证集")));
        return false;
    }

    //判定是否可以保存余额
    QSet<PzsState> canSave;  //可以执行保存余额动作的凭证集状态代码集合
    canSave<<Ps_HandV<<Ps_Stat1
           <<Ps_ImpV<<Ps_Stat2
           <<Ps_JzhdV<<Ps_Stat3
           <<Ps_JzsyV<<Ps_Stat4
           <<Ps_JzbnlrV<<Ps_Stat5;
    PzsState state;
    getPzsState(y,m,state);
    if(canSave.contains(state))
        isSave = true;
    else
        isSave = false;

    //遍历凭证集
    amount = 0;
    while(q.next()){
        int pid,fid,sid,ac,mtype;
        double jv,dv;  //业务活动的借贷金额
        int dir;      //业务活动借贷方向

        pid = q.value(0).toInt(); //凭证id
        s = QString("select * from BusiActions where pid = %1").arg(pid);
        if(!q2.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("在计算本期发生额时不能获取id为%1的凭证的业务活动表")).arg(pid));
            return false;
        }
        amount++;
        //遍历凭证的业务活动
        while(q2.next()){
            int key;
            fid = q2.value(BACTION_FID).toInt();
            sid = q2.value(BACTION_SID).toInt();
            mtype = q2.value(BACTION_MTYPE).toInt();
            dir = q2.value(BACTION_DIR).toInt();
            if(dir == DIR_J){//发生在借方
                jv = q2.value(BACTION_JMONEY).toDouble();
                jSums[fid*10+mtype] += jv;
                if(!dSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
                    dSums[fid*10+mtype] = 0;
                if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
                    key = sid*10+mtype;
                    sjSums[key] += jv;
                    if(!sdSums.contains(key))
                        sdSums[key] = 0;
                }
            }
            else{
                dv = q2.value(BACTION_DMONEY).toDouble();
                dSums[fid*10+mtype] += dv;
                if(!jSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
                    jSums[fid*10+mtype] = 0;
                if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
                    key = sid*10+mtype;
                    sdSums[key] += dv;
                    if(!sjSums.contains(key))
                        sjSums[key] = 0;
                }
            }
        }        
    }

    //执行四舍五入
//    QString ds;
//    double v;
//    QHashIterator<int,double>* it = new QHashIterator<int,double>(jSums);
//    while(it->hasNext()){
//        it->next();
//        ds = QString::number(it->value(),'f',2);
//        v = ds.toDouble();
//        jSums[it->key()] = v;
//        jSums[it->key()] = QString::number(it->value(),'f',2).toDouble();
//    }
//    it = new QHashIterator<int,double>(dSums);
//    while(it->hasNext()){
//        it->next();
//        dSums[it->key()] = QString::number(it->value(),'f',2).toDouble();
//    }
//    it = new QHashIterator<int,double>(sjSums);
//    while(it->hasNext()){
//        it->next();
//        dSums[it->key()] = QString::number(it->value(),'f',2).toDouble();
//    }
//    it = new QHashIterator<int,double>(sdSums);
//    while(it->hasNext()){
//        it->next();
//        dSums[it->key()] = QString::number(it->value(),'f',2).toDouble();
//    }

    return true;
}

//使用Double来计算本期发生额
bool BusiUtil::calAmountByMonth2(int y, int m, QHash<int,Double>& jSums, QHash<int,Double>& dSums,
             QHash<int,Double>& sjSums, QHash<int,Double>& sdSums,
                                bool& isSave, int& amount)
{
    QSqlQuery q,q2;
    QString s;
    bool r;

    //初始化汇率表
    QHash<int,Double>rates;
    if(!getRates2(y,m,rates))
        return false;
    rates[RMB] = Double(1.00);

    //初始化币种表
    QHash<int,QString> mts;
    if(!getMTName(mts))
        return false;

    //初始化需要进行明细核算的一级科目的id列表
    QList<int> detSubs;
    if(!getReqDetSubs(detSubs))
        return false;

    //生成查询语句
    if(!genStatSql(y,m,s))
        return false;

    if(!q.exec(s)){
        QMessageBox::information(0, QObject::tr("提示信息"),
                                 QString(QObject::tr("在计算本期发生额时不能获取凭证集")));
        return false;
    }

    //判定是否可以保存余额
    QSet<PzsState> canSave;  //可以执行保存余额动作的凭证集状态代码集合
    canSave<<Ps_HandV<<Ps_Stat1
           <<Ps_ImpV<<Ps_Stat2
           <<Ps_JzhdV<<Ps_Stat3
           <<Ps_JzsyV<<Ps_Stat4
           <<Ps_JzbnlrV<<Ps_Stat5;
    PzsState state;
    getPzsState(y,m,state);
    if(canSave.contains(state))
        isSave = true;
    else
        isSave = false;

    //遍历凭证集
    amount = 0;
    int pid,fid,sid,mtype;
    PzClass pzCls; //凭证类别
    Double jv,dv;  //业务活动的借贷金额
    int dir;       //业务活动借贷方向
    bool isJzhdPz = false;
    while(q.next()){
        pid = q.value(0).toInt(); //凭证id
        pzCls = (PzClass)q.value(PZ_CLS).toInt();
        isJzhdPz = pzCls == Pzc_Jzhd_Bank || pzCls == Pzc_Jzhd_Ys ||
                pzCls == Pzc_Jzhd_Yf;
        s = QString("select * from BusiActions where pid = %1").arg(pid);
        if(!q2.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("在计算本期发生额时不能获取id为%1的凭证的业务活动表")).arg(pid));
            return false;
        }
        amount++;
        //遍历凭证的业务活动
        while(q2.next()){
            fid = q2.value(BACTION_FID).toInt();
            //如果是结转汇兑损益的凭证，则跳过非财务费用方的会计分录，因为这些要计入到外币部分
            if(isJzhdPz && isAccMt(fid))
                continue;
            int key;
            sid = q2.value(BACTION_SID).toInt();
            mtype = q2.value(BACTION_MTYPE).toInt();
            dir = q2.value(BACTION_DIR).toInt();

            if(dir == DIR_J){//发生在借方
                jv = Double(q2.value(BACTION_JMONEY).toDouble());
                jSums[fid*10+mtype] += jv;
                if(!dSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
                    dSums[fid*10+mtype] = Double(0.00);
                if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
                    key = sid*10+mtype;
                    sjSums[key] += jv;
                    if(!sdSums.contains(key))
                        sdSums[key] = Double(0.00);
                }
            }
            else{
                dv = Double(q2.value(BACTION_DMONEY).toDouble());
                dSums[fid*10+mtype] += dv;
                if(!jSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
                    jSums[fid*10+mtype] = Double(0.00);
                if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
                    key = sid*10+mtype;
                    sdSums[key] += dv;
                    if(!sjSums.contains(key))
                        sjSums[key] = Double(0.00);
                }
            }

        }
    }
    return true;
}

//计算本期末余额（参数所涉及的金额都以本币计）
bool BusiUtil::calCurExtraByMonth3(int y,int m,
       QHash<int,Double> preExaR, QHash<int,Double> preDetExaR,     //期初余额
       QHash<int,int> preExaDirR, QHash<int,int> preDetExaDirR,     //期初余额方向
       QHash<int,Double> curJHpnR, QHash<int,Double> curJDHpnR,     //当期借方发生额
       QHash<int,Double> curDHpnR, QHash<int,Double>curDDHpnR,      //当期贷方发生额
       QHash<int,Double> &endExaR, QHash<int,Double>&endDetExaR,    //期末余额
       QHash<int,int> &endExaDirR, QHash<int,int> &endDetExaDirR)
{
    //第一步：计算总账科目余额
    Double v;
    int dir,fid,sid;
    QHashIterator<int,Double> cj(curJHpnR);
    while(cj.hasNext()){
        cj.next();
        int key = cj.key();
        fid = key/10;
        //确定本期借贷相抵后的借贷方向
        v = curJHpnR[key] - curDHpnR[key];  //借方 - 贷方
        if(v > 0)
            dir = DIR_J;
        else if(v < 0){
            dir = DIR_D;
            v.changeSign();
        }
        else
            dir = DIR_P;

        if(dir == DIR_P){ //本期借贷相抵（平）余额值和方向同期初
            endExaR[key] = preExaR.value(key);
            endExaDirR[key] = preExaDirR.value(key);
        }
        else if(preExaDirR.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
            endExaR[key] = preExaR.value(key) + v;
            endExaDirR[key] = preExaDirR.value(key);
        }
        else{
            Double tv;
            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
            if(dir == DIR_J)
                tv = v - preExaR.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
            else
                tv = preExaR.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
            if(tv > 0){ //余额在借方
                //如果是收入类科目，要将它固定为贷方
                if(isInSub(fid)){
                    tv.changeSign();
                    endExaR[key] = tv;
                    endExaDirR[key] = DIR_D;
                }
                else{
                    endExaR[key] = tv;
                    endExaDirR[key] = DIR_J;
                }
            }
            else if(tv < 0){ //余额在贷方
                //如果是费用类科目，要将它固定为借方
                if(isFeiSub(fid)){
                    endExaR[key] = tv;
                    endExaDirR[key] = DIR_J;
                }
                else{
                    tv.changeSign();
                    endExaR[key] = tv;
                    endExaDirR[key] = DIR_D;
                }
            }
            else{
                endExaR[key] = 0;
                endExaDirR[key] = DIR_P;
                //或者可以考虑，将余额值为0的科目从hash表中移除
            }
        }
    }


    //第二步：计算明细科目余额
    QHashIterator<int,Double> dj(curJDHpnR);
    while(dj.hasNext()){
        dj.next();
        int key = dj.key();
        sid = key/10;
        v = curJDHpnR[key] - curDDHpnR[key];
        if(v > 0)
            dir = DIR_J;
        else if(v < 0){
            dir = DIR_D;
            v.changeSign();
        }
        else
            dir = DIR_P;

        if(dir == DIR_P){ //本期借贷相抵（平）
            endDetExaR[key] = preDetExaR.value(key);
            endDetExaDirR[key] = preDetExaDirR.value(key);
        }
        else if(preDetExaDirR.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
            endDetExaR[key] = preDetExaR.value(key) + v;
            endDetExaDirR[key] = preDetExaDirR.value(key);
        }
        else{
            Double tv;
            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
            if(dir == DIR_J)
                tv = v - preDetExaR.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
            else
                tv = preDetExaR.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
            if(tv > 0){ //余额在借方
                //如果是收入类科目，要将它固定为贷方
                if(isInSSub(sid)){
                    tv.changeSign();
                    endDetExaR[key] = tv;
                    endDetExaDirR[key] = DIR_D;
                }
                else{
                    endDetExaR[key] = tv;
                    endDetExaDirR[key] = DIR_J;
                }
            }
            else if(tv < 0){ //余额在贷方
                //如果是费用类科目，要将它固定为借方
                if(isFeiSSub(sid)){
                    endDetExaR[key] = tv;
                    endDetExaDirR[key] = DIR_J;
                }
                else{
                    tv.changeSign();
                    endDetExaR[key] = tv;
                    endDetExaDirR[key] = DIR_D;
                }

            }
            else{
                endDetExaR[key] = 0;
                endDetExaDirR[key] = DIR_P;
                //或者可以考虑，将余额值为0的科目从hash表中移除
            }
        }
    }
    //将本期未发生的总账科目余额加入到总账期末余额表中
    QHashIterator<int,Double> p(preExaR);
    while(p.hasNext()){
        p.next();
        int key = p.key();
        if(!endExaR.contains(key)){
            endExaR[key] = preExaR[key];
            endExaDirR[key] = preExaDirR[key];
        }
    }
    //将本期未发生的明细科目余额加入到明细期末余额表中
    QHashIterator<int,Double> pd(preDetExaR);
    while(pd.hasNext()){
        pd.next();
        int key = pd.key();
        if(!endDetExaR.contains(key)){
            endDetExaR[key] = preDetExaR[key];
            endDetExaDirR[key] = preDetExaDirR[key];
        }
    }
}



//计算本期末余额
bool BusiUtil::calCurExtraByMonth(int y,int m,
  QHash<int,double> preExa, QHash<int,double> preDetExa,     //期初余额
  QHash<int,int> preExaDir, QHash<int,int> preDetExaDir,     //期初余额方向
  QHash<int,double> curJHpn, QHash<int,double> curJDHpn,     //当期借方发生额
  QHash<int,double> curDHpn, QHash<int,double>curDDHpn,      //当期贷方发生额
  QHash<int,double> &endExa, QHash<int,double>&endDetExa,    //期末余额
  QHash<int,int> &endExaDir, QHash<int,int> &endDetExaDir)   //期末余额方向
{
    //第一步：计算总账科目余额
    double v;
    int dir;
    QHashIterator<int,double> cj(curJHpn);
    while(cj.hasNext()){
        cj.next();
        int key = cj.key();
        //确定本期借贷相抵后的借贷方向
        v = curJHpn[key] - curDHpn[key];  //借方 - 贷方
        if(v > 0)
            dir = DIR_J;
        else if(v < 0){
            dir = DIR_D;
            v = 0 - v;
        }
        else
            dir = DIR_P;

        if(dir == DIR_P){ //本期借贷相抵（平）
            endExa[key] = preExa.value(key);
            endExaDir[key] = preExaDir.value(key);
        }
        else if(preExaDir.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
            endExa[key] = preExa.value(key) + v;
            endExaDir[key] = preExaDir.value(key);
        }
        else{
            double tv;
            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
            if(dir == DIR_J)
                tv = v - preExa.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
            else
                tv = preExa.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
            if(tv > 0){ //余额在借方
                endExa[key] = tv;
                endExaDir[key] = DIR_J;
            }
            else if(tv < 0){ //余额在贷方
                endExa[key] = 0 - tv;
                endExaDir[key] = DIR_D;
            }
            else{
                endExa[key] = 0;
                endExaDir[key] = DIR_P;
            }
        }
    }


    //第二步：计算明细科目余额
    QHashIterator<int,double> dj(curJDHpn);
    while(dj.hasNext()){
        dj.next();
        int key = dj.key();
        v = curJDHpn[key] - curDDHpn[key];
        if(v > 0)
            dir = DIR_J;
        else if(v < 0){
            dir = DIR_D;
            v = 0 - v;
        }
        else
            dir = DIR_P;

        if(dir == DIR_P){ //本期借贷相抵（平）
            endDetExa[key] = preDetExa.value(key);
            endDetExaDir[key] = preDetExaDir.value(key);
        }
        else if(preDetExaDir.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
            endDetExa[key] = preDetExa.value(key) + v;
            endDetExaDir[key] = preDetExaDir.value(key);
        }
        else{
            double tv;
            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
            if(dir == DIR_J)
                tv = v - preDetExa.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
            else
                tv = preDetExa.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
            if(tv > 0){ //余额在借方
                endDetExa[key] = tv;
                endDetExaDir[key] = DIR_J;
            }
            else if(tv < 0){ //余额在贷方
                endDetExa[key] = 0 - tv;
                endDetExaDir[key] = DIR_D;
            }
            else{
                endDetExa[key] = 0;
                endDetExaDir[key] = DIR_P;
            }
        }
    }
    //将本期未发生的总账科目余额加入到总账期末余额表中
    QHashIterator<int,double> p(preExa);
    while(p.hasNext()){
        p.next();
        int key = p.key();
        if(!endExa.contains(key)){
            endExa[key] = preExa[key];
            endExaDir[key] = preExaDir[key];
        }
    }
    //将本期未发生的明细科目余额加入到明细期末余额表中
    QHashIterator<int,double> pd(preDetExa);
    while(pd.hasNext()){
        pd.next();
        int key = pd.key();
        if(!endDetExa.contains(key)){
            endDetExa[key] = preDetExa[key];
            endDetExaDir[key] = preDetExaDir[key];
        }
    }
}

bool BusiUtil::calCurExtraByMonth2(int y,int m,
       QHash<int,Double> preExa, QHash<int,Double> preDetExa,     //期初余额
       QHash<int,int> preExaDir, QHash<int,int> preDetExaDir,     //期初余额方向
       QHash<int,Double> curJHpn, QHash<int,Double> curJDHpn,     //当期借方发生额
       QHash<int,Double> curDHpn, QHash<int,Double>curDDHpn,      //当期贷方发生额
       QHash<int,Double> &endExa, QHash<int,Double>&endDetExa,    //期末余额
       QHash<int,int> &endExaDir, QHash<int,int> &endDetExaDir)  //期末余额方向
{
    //第一步：计算总账科目余额
    Double v;
    int dir,fid;
    QHashIterator<int,Double> cj(curJHpn);
    while(cj.hasNext()){
        cj.next();
        int key = cj.key();
        fid = key/10;
        //确定本期借贷相抵后的借贷方向
        v = curJHpn[key] - curDHpn[key];  //借方 - 贷方
        if(v > 0)
            dir = DIR_J;
        else if(v < 0){
            dir = DIR_D;
            v.changeSign();
        }
        else
            dir = DIR_P;

        if(dir == DIR_P){ //本期借贷相抵（平）余额值和方向同期初
            endExa[key] = preExa.value(key);
            endExaDir[key] = preExaDir.value(key);
        }
        else if(preExaDir.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
            endExa[key] = preExa.value(key) + v;
            endExaDir[key] = preExaDir.value(key);
        }
        else{
            Double tv;
            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
            if(dir == DIR_J)
                tv = v - preExa.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
            else
                tv = preExa.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
            if(tv > 0){ //余额在借方
                //如果是收入类科目，要将它固定为贷方
                if(BusiUtil::isInSub(fid)){
                    tv.changeSign();
                    endExa[key] = tv;
                    endExaDir[key] = DIR_D;
                }
                else{
                    endExa[key] = tv;
                    endExaDir[key] = DIR_J;
                }

            }
            else if(tv < 0){ //余额在贷方
                //如果是费用类科目，要将它固定为借方
                if(BusiUtil::isFeiSub(fid)){
                    //tv.changeSign();
                    endExa[key] = tv;
                    endExaDir[key] = DIR_J;
                }
                else{
                    tv.changeSign();
                    endExa[key] = tv;
                    endExaDir[key] = DIR_D;
                }

            }
            else{
                endExa[key] = 0;
                endExaDir[key] = DIR_P;
            }
        }
    }


    //第二步：计算明细科目余额
    QHashIterator<int,Double> dj(curJDHpn);
    int sid;
    while(dj.hasNext()){
        dj.next();
        int key = dj.key();
        sid = key/10;
        v = curJDHpn[key] - curDDHpn[key];
        if(v > 0)
            dir = DIR_J;
        else if(v < 0){
            dir = DIR_D;
            v.changeSign();
        }
        else
            dir = DIR_P;

        if(dir == DIR_P){ //本期借贷相抵（平）
            endDetExa[key] = preDetExa.value(key);
            endDetExaDir[key] = preDetExaDir.value(key);
        }
        else if(preDetExaDir.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
            endDetExa[key] = preDetExa.value(key) + v;
            endDetExaDir[key] = preDetExaDir.value(key);
        }
        else{
            Double tv;
            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
            if(dir == DIR_J)
                tv = v - preDetExa.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
            else
                tv = preDetExa.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
            if(tv > 0){ //余额在借方
                //如果是收入类科目，要将它固定为贷方
                if(isInSSub(sid)){
                    tv.changeSign();
                    endDetExa[key] = tv;
                    endDetExaDir[key] = DIR_D;
                }
                else{
                    endDetExa[key] = tv;
                    endDetExaDir[key] = DIR_J;
                }

            }
            else if(tv < 0){ //余额在贷方
                //如果是费用类科目，要将它固定为借方
                if(isFeiSSub(sid)){
                    endDetExa[key] = tv;
                    endDetExaDir[key] = DIR_J;
                }
                else{
                    tv.changeSign();
                    endDetExa[key] = tv;
                    endDetExaDir[key] = DIR_D;
                }
            }
            else{
                endDetExa[key] = 0;
                endDetExaDir[key] = DIR_P;
            }
        }
    }
    //将本期未发生的总账科目余额加入到总账期末余额表中
    QHashIterator<int,Double> p(preExa);
    while(p.hasNext()){
        p.next();
        int key = p.key();
        if(!endExa.contains(key)){
            endExa[key] = preExa[key];
            endExaDir[key] = preExaDir[key];
        }
    }
    //将本期未发生的明细科目余额加入到明细期末余额表中
    QHashIterator<int,Double> pd(preDetExa);
    while(pd.hasNext()){
        pd.next();
        int key = pd.key();
        if(!endDetExa.contains(key)){
            endDetExa[key] = preDetExa[key];
            endDetExaDir[key] = preDetExaDir[key];
        }
    }
}

/**
 * @brief BusiUtil::calAmountByMonth3
 *  用本位币来计算指定月份的各科目发生额
 *  第3版和第2版的区别是金额都以人民币来表示(键为币种代码，值为转换为人民币的金额值，即值以本币计)
 * @param y
 * @param m
 * @param jSums     借方主目发生额
 * @param dSums     借方子目发生额
 * @param sjSums    贷方主目发生额
 * @param sdSums    贷方子目发生额
 * @param isSave    是否可以保存余额
 * @param amount    统计涉及到的凭证数
 * @return
 */
bool BusiUtil::calAmountByMonth3(int y, int m, QHash<int, Double> &jSums, QHash<int, Double> &dSums, QHash<int, Double> &sjSums, QHash<int, Double> &sdSums, bool &isSave, int &amount)
{
    QSqlQuery q,q2;
    QString s;

    //初始化汇率表
    QHash<int,Double>rates;
    if(!getRates2(y,m,rates))
        return false;
    rates[RMB] = Double(1.00);

    //初始化需要进行明细核算的一级科目的id列表
    QList<int> detSubs;
    if(!getReqDetSubs(detSubs))
        return false;

    //生成查询语句
    if(!genStatSql(y,m,s))
        return false;

    if(!q.exec(s)){
        QMessageBox::information(0, QObject::tr("提示信息"),
                                 QString(QObject::tr("在计算本期发生额时不能获取凭证集")));
        return false;
    }

    //判定是否可以保存余额
    QSet<PzsState> canSave;  //可以执行保存余额动作的凭证集状态代码集合
    canSave<<Ps_HandV<<Ps_Stat1
           <<Ps_ImpV<<Ps_Stat2
           <<Ps_JzhdV<<Ps_Stat3
           <<Ps_JzsyV<<Ps_Stat4
           <<Ps_JzbnlrV<<Ps_Stat5;
    PzsState state;
    getPzsState(y,m,state);
    if(canSave.contains(state))
        isSave = true;
    else
        isSave = false;

    //遍历凭证集
    amount = 0;
    while(q.next()){
        int pid,fid,sid,ac,mtype;
        PzClass pzCls; //凭证类别
        Double jv,dv;  //业务活动的借贷金额
        int dir;      //业务活动借贷方向

        pid = q.value(0).toInt(); //凭证id
        pzCls =(PzClass)q.value(PZ_CLS).toInt();
        s = QString("select * from BusiActions where pid = %1").arg(pid);
        if(!q2.exec(s)){
            QMessageBox::information(0, QObject::tr("提示信息"),
                                     QString(QObject::tr("在计算本期发生额时不能获取id为%1的凭证的业务活动表")).arg(pid));
            return false;
        }
        amount++;
        //遍历凭证的业务活动
        while(q2.next()){
            int key;
            fid = q2.value(BACTION_FID).toInt();
            sid = q2.value(BACTION_SID).toInt();
            mtype = q2.value(BACTION_MTYPE).toInt();
            dir = q2.value(BACTION_DIR).toInt();

            //如果当前凭证是结转汇兑损益类的凭证，则需特别处理
            //注意：该类凭证的会计分录银行、应收/应付方始终在贷方，财务费用始终在借方，而且
            //币种为本币，这是一个约定，由生成结转凭证的函数来保证，不能改变
            //如果要支持多种外币，则必须从会计分录中读取此条结转汇兑损益的会计分录所对应的
            //外币代码，目前，为了简化，外币只有美元项
            if(pzCls == Pzc_Jzhd_Bank || pzCls == Pzc_Jzhd_Ys || pzCls == Pzc_Jzhd_Yf){
                //continue;
                if(isAccMt(fid)){  //如果是银行、应收/应付方
                    dv = Double(q2.value(BACTION_DMONEY).toDouble());
                    key = fid*10+USD;
                    dSums[key] += dv;
                    if(!jSums.contains(key))
                        jSums[key] = Double(0.00);
                    key = sid*10+USD;
                    sdSums[key] += dv;
                    if(!sjSums.contains(key))
                        sjSums[key] = Double(0.00);
                }
                else{ //财务费用方
                    jv = Double(q2.value(BACTION_JMONEY).toDouble());
                    key = fid*10+mtype;
                    jSums[key] += jv;
                    if(!dSums.contains(key))
                        dSums[key] = Double(0.00);
                    key = sid*10+mtype;
                    sjSums[key] += jv;
                    if(!sdSums.contains(key))
                        sdSums[key] = Double(0.00);
                }
            }
            //else if(pzCls == Pzc_Jzhd_Ys){

            //}
            //else if(pzCls == Pzc_Jzhd_Yf){

            //}
            else{
                if(dir == DIR_J){//发生在借方
                    jv = Double(q2.value(BACTION_JMONEY).toDouble());
                    if(mtype != RMB)//如果是外币，则将其转换为人民币
                        jv = jv * rates.value(mtype);
                    jSums[fid*10+mtype] += jv;
                    if(!dSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
                        dSums[fid*10+mtype] = Double(0.00);
                    if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
                        key = sid*10+mtype;
                        sjSums[key] += jv;
                        if(!sdSums.contains(key))
                            sdSums[key] = Double(0.00);
                    }
                }
                else{
                    dv = Double(q2.value(BACTION_DMONEY).toDouble());
                    if(mtype != RMB)//如果是外币，则将其转换为人民币
                        dv = dv * rates.value(mtype);
                    dSums[fid*10+mtype] += dv;
                    if(!jSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
                        jSums[fid*10+mtype] = Double(0.00);
                    if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
                        key = sid*10+mtype;
                        sdSums[key] += dv;
                        if(!sjSums.contains(key))
                            sjSums[key] = Double(0.00);
                    }
                }
            }
        }
    }
    return true;
}


/**
    计算科目各币种合计余额及其方向
    参数  exas：科目余额, exaDirs：科目方向  （这两个参数的键是科目id * 10 +　币种代码）
         sums：科目各币种累计余额，dirs：科目各币种累计余额的方向（这两个参数的键是科目id）
*/
bool BusiUtil::calSumByMt(QHash<int,double> exas, QHash<int,int>exaDirs,
                       QHash<int,double>& sums, QHash<int,int>& dirs,
                       QHash<int,double> rates)
{
    QHashIterator<int,double> it(exas);
    int id,mt;
    double v;
    //基本思路是借方　－　贷方，并根据差值的符号来判断余额方向
    while(it.hasNext()){
        it.next();
        id = it.key() / 10;
        mt = it.key() % 10;        
        v = it.value() * rates.value(mt);
        //v = QString::number(v,'f',2).toDouble(); //执行四舍五入，规避多一分问题
        if(exaDirs.value(it.key()) == DIR_P)
            //continue;
            sums[id] = 0;
        else if(exaDirs.value(it.key()) == DIR_J)
            sums[id] += v;
        else
            sums[id] -= v;
    }
    QHashIterator<int,double> i(sums);
    while(i.hasNext()){
        i.next();
        //执行四舍五入，规避多一分问题
        //sums[i.key()] = QString::number(i.value(),'f',2).toDouble();
        if(i.value() == 0)
            dirs[i.key()] = DIR_P;
        else if(i.value() > 0)
            dirs[i.key()] = DIR_J;
        else{
            sums[i.key()] = -sums.value(i.key());
            dirs[i.key()] = DIR_D;
        }
    }
}

bool BusiUtil::calSumByMt2(QHash<int,Double> exas, QHash<int,int>exaDirs,
                           QHash<int,Double>& sums, QHash<int,int>& dirs,
                           QHash<int,Double> rates)
{
    QHashIterator<int,Double> it(exas);
    int id,mt;
    Double v;
    //基本思路是借方　－　贷方，并根据差值的符号来判断余额方向
    while(it.hasNext()){
        it.next();
        id = it.key() / 10;
        mt = it.key() % 10;
        if(mt == RMB)
            v = it.value();
        else
            v = it.value() * rates.value(mt);
        if(exaDirs.value(it.key()) == DIR_P)
            //continue;
            sums[id] = 0;
        else if(exaDirs.value(it.key()) == DIR_J)
            sums[id] += v;
        else
            sums[id] -= v;
    }
    QHashIterator<int,Double> i(sums);
    while(i.hasNext()){
        i.next();
        if(i.value() == 0)
            dirs[i.key()] = DIR_P;
        else if(i.value() > 0)
            dirs[i.key()] = DIR_J;
        else{
            sums[i.key()].changeSign();
            dirs[i.key()] = DIR_D;
        }
    }
}

//获取凭证集内最大的可用凭证号
int BusiUtil::getMaxPzNum(int y, int m)
{
    QSqlQuery q;
    QString s;
    bool r;

    int num = 0;
    QDate d(y,m,1);
    QString ds = d.toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select max(number) from PingZhengs where (pzState != %1) "
                "and (date like '%2%')").arg(Pzs_Repeal).arg(ds);

    r = q.exec(s); r = q.first();
    num = q.value(0).toInt();
    return ++num;
}

//读取凭证集状态
bool BusiUtil::getPzsState(int y,int m,PzsState& state)
{
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("select state from PZSetStates where (year=%1) and (month=%2)")
            .arg(y).arg(m);
    if(q.exec(s) && q.first()){
        state = (PzsState)q.value(0).toInt();
        return true;
    }
    else
        return false;
}

//设置凭证集状态
bool BusiUtil::setPzsState(int y,int m,PzsState state)
{
    QSqlQuery q;
    QString s;
    bool r;

    //首先检测是否存在对应记录
    s = QString("select state from PZSetStates where (year=%1) and (month=%2)")
            .arg(y).arg(m);
    if(q.exec(s) && q.first()){
        s = QString("update PZSetStates set state=%1 where (year=%2) and (month=%3)")
                .arg(state).arg(y).arg(m);
        r = q.exec(s);
        return r;
    }
    else{
        s = QString("insert into PZSetStates(year,month,state) values(%1,%2,%3)")
                .arg(y).arg(m).arg(state);
        r = q.exec(s);
        return r;
    }
}

/**
 * @brief BusiUtil::setPzsState2
 *  //设置凭证集状态，并根据设定的状态调整凭证集内凭证的审核入账状态
 * @param y
 * @param m
 * @param state
 * @return
 */
bool BusiUtil::setPzsState2(int y, int m, PzsState state)
{
    QSqlQuery q;
    QList<int> rpzs,inpzs;  //要影响的凭证类别（分别对应入账凭证、录入状态凭证）
    QString s1 = QString("update PingZhengs set pzState=");  //更新语句头
    QString fs1,fs2;  //过滤条件表达式（分别对应入账凭证、录入状态凭证）

    switch(state){
    case Ps_Rec:  //初始态，所有手工凭证都将处于录入态
        rpzs<<Pzc_Hand;
        break;
    case Ps_HandV: //所有手工输入凭证都审核通过并入账
    case Ps_Stat1:
        inpzs<<Pzc_Hand;
        break;
    case Ps_Jzhd:   //结转汇兑损益的凭证处于未审核，而手工凭证入账
        inpzs<<Pzc_Hand;
        rpzs<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf;
        break;
    case Ps_JzhdV:  //结转汇兑损益凭证和手工凭证都入账
    case Ps_Stat3:
    case Ps_JzsyPre:
        inpzs<<Pzc_Hand<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf;
        break;
    case Ps_Jzsy: //结转损益凭证处于未审核，而手工、结转汇兑损益凭证都入账
        rpzs<<Pzc_JzsyIn<<Pzc_JzsyFei;
        inpzs<<Pzc_Hand<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf;
        break;
    case Ps_JzsyV: //手工、结转汇兑损益、结转损益凭证都入账
        inpzs<<Pzc_Hand<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf<<Pzc_JzsyIn<<Pzc_JzsyFei;
        break;
    case Ps_Jzbnlr: //结转本年利润处于未审核、而手工、结转汇兑损益、结转损益凭证都入账
        rpzs<<Pzc_Jzlr;
        inpzs<<Pzc_Hand<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf<<Pzc_JzsyIn<<Pzc_JzsyFei;
        break;
    case Ps_JzbnlrV:  //手工、结转汇兑损益、结转损益、结转本年利润凭证都入账
        inpzs<<Pzc_Hand<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf<<Pzc_JzsyIn<<Pzc_JzsyFei<<Pzc_Jzlr;
        break;
    case Ps_Lrfp :   //待以后考虑
        break;
    case Ps_LrfpV:   //待以后考虑
        break;
    }

    QString ds = QDate(y,m,1).toString(Qt::ISODate);
    ds.chop(3);

    if(rpzs.empty() && inpzs.empty()){
        QString s = QString("%1 where (date like '%2%') and (%3!=%4))")
                  .arg(Pzs_Instat).arg(ds)
                  .arg(fld_pz_state).arg(Pzs_Repeal);
        QString sql = s1 + s;
        if(!q.exec(sql))
            return false;
    }
    else{
        if(!rpzs.empty()){
            fs1 = BusiUtil::genFiltStateForSpecPzCls(rpzs);
            QString s = QString("%1 where (date like '%2%') and (%3!=%4) and (%5)")
                      .arg(Pzs_Recording).arg(ds)
                      .arg(fld_pz_state).arg(Pzs_Repeal)
                      .arg(fs1);
            QString sql = s1 + s;
            if(!q.exec(sql))
                return false;
        }
        if(!inpzs.empty()){
            fs2 = BusiUtil::genFiltStateForSpecPzCls(inpzs);
            QString s = QString("%1 where (date like '%2%') and (%3!=%4) and (%5)")
                      .arg(Pzs_Instat).arg(ds)
                      .arg(fld_pz_state).arg(Pzs_Repeal)
                      .arg(fs2);
            QString sql = s1 + s;
            if(!q.exec(sql))
                return false;
        }
    }
    return setPzsState(y,m,state);
}

/**
    获取银行存款下所有外币账户对应的明细科目id列表
    参数 ids：明细科目列表，mt：对应的外币币种代码列表
*/
bool BusiUtil::getOutMtInBank(QList<int>& ids, QList<int>& mt)
{
    QSqlQuery q;
    QString s;
    bool r;

    QHash<QString,int> mts;  //外币名称到币种代码的映射
    s = QString("select name,code from MoneyTypes where (code!=%1)").arg(RMB);
    r = q.exec(s);
    while(q.next())
        mts[q.value(0).toString()] = q.value(1).toInt();

    int bankId;
    if(!getIdByCode(bankId,"1002"))
        return false;
    s = QString("select FSAgent.id, SecSubjects.subName from FSAgent join SecSubjects "
                "where (FSAgent.sid = SecSubjects.id) and (FSAgent.fid = %1)").arg(bankId);
    r = q.exec(s);
    int id,idx;
    QString name,mname;
    while(q.next()){
        id = q.value(0).toInt();
        name = q.value(1).toString();
        idx = name.indexOf("-");
        if(idx != -1){
            mname = name.right(name.count() - idx - 1);
            if(mts.contains(mname)){
                ids << id;
                mt << mts.value(mname);
            }
        }
    }
}

//新建凭证
bool BusiUtil::crtNewPz(PzData* pz)
{
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("insert into PingZhengs(date,number,zbNum,jsum,dsum,isForward,encNum"
                ",pzState,vuid,ruid,buid) values('%1',%2,%3,%4,%5,%6,%7,%8,%9,%10,%11)")
            .arg(pz->date).arg(pz->pzNum).arg(pz->pzZbNum).arg(pz->jsum)
            .arg(pz->dsum).arg(pz->pzClass).arg(pz->attNums).arg(Pzs_Max)
            .arg(pz->verify!=NULL?pz->verify->getUserId():NULL)
            .arg(pz->producer!=NULL?pz->producer->getUserId():NULL)
            .arg(pz->bookKeeper!=NULL?pz->bookKeeper->getUserId():NULL);
    r = q.exec(s);
    if(r){
        s = QString("select id from PingZhengs where pzState = %1").arg(Pzs_Max);
        if(q.exec(s) && q.first()){
            pz->pzId = q.value(0).toInt();
            s = QString("update PingZhengs set pzState = %1 where pzState = %2")
                    .arg(pz->state).arg(Pzs_Max);
            if(q.exec(s))
                return true;
            else
                return false;
        }
        else
            return false;
    }
    return r;
}

//按凭证日期，重新设置凭证集内的凭证号
bool BusiUtil::assignPzNum(int y, int m)
{
    QSqlQuery q,q1;
    QString s;
    bool r;

    QDate d(y,m,1);
    QString ds = d.toString(Qt::ISODate);
    ds.chop(3);
    s = QString("select id from PingZhengs where date like '%1%' order by date").arg(ds);
    r = q.exec(s);
    int id, num = 1;
    while(q.next()){
        id = q.value(0).toInt();
        s = QString("update PingZhengs set number=%1 where id=%2").arg(num++).arg(id);
        r = q1.exec(s);
    }
    return r;
}

//按二级科目id获取二级科目名（这里的id是SecSubjects表的id）
bool BusiUtil::getSNameForId(int sid, QString& name, QString& lname)
{
    QSqlQuery q;
    QString s;

    s = QString("select subName,subLName from SecSubjects where id = %1").arg(sid);
    if(q.exec(s) && q.first()){
        name = q.value(0).toString();
        lname = q.value(1).toString();
        return true;
    }
    else
        return false;
}

//保存账户信息到账户文件（中的AccountInfos表中）
bool BusiUtil::saveAccInfo(AccountBriefInfo* accInfo)
{
    QSqlQuery q;
    QString s;

    s = QString("select id from AccountInfos where code = '%1'")
            .arg(accInfo->code);
//    if(q.exec(s) && q.first()){
//        int id = q.value(0).toInt();
//        s = QString("update AccountInfos set code='%1',baseTime='%2',usedSubId=%3,"
//                    "sname='%4',lname='%5',lastTime='%6',desc='%7' where id=%8")
//                .arg(accInfo->code).arg(accInfo->baseTime).arg(accInfo->usedSubSys)
//                /*.arg(accInfo->usedRptType)*/.arg(accInfo->accName)
//                .arg(accInfo->accLName).arg(accInfo->lastTime)
//                .arg(accInfo->desc).arg(id);
//    }
//    else{
//        s = QString("insert into AccountInfos(code,baseTime,usedSubId,sname,"
//                    "lname,lastTime,desc) values('%1','%2',%3,'%4','%5','%6','%7')")
//                .arg(accInfo->code).arg(accInfo->baseTime).arg(accInfo->usedSubSys)
//                /*.arg(accInfo->usedRptType)*/.arg(accInfo->accName)
//                .arg(accInfo->accLName).arg(accInfo->lastTime).arg(accInfo->desc);
//    }

    if(q.exec(s) && q.first()){
        int id = q.value(0).toInt();
        s = QString("update AccountInfos set code='%1',"
                    "sname='%2',lname='%3' where id=%4")
                .arg(accInfo->code).arg(accInfo->accName)
                .arg(accInfo->accLName).arg(id);
    }
    else{
        s = QString("insert into AccountInfos(code,sname,lname) "
                    "values('%1','%2','%3')")
                .arg(accInfo->code).arg(accInfo->accName)
                .arg(accInfo->accLName);
    }
    bool r = q.exec(s);
    return r;
}

//读取银行帐号
bool BusiUtil::readAllBankAccont(QHash<int,BankAccount*>& banks)
{
    QSqlQuery q;
    QString s, accNum;
    int id;

    QList<int> bankIds,mtIds;
    QHash<int,bool> isMains;
    QStringList bankNames,mtNames;
    s = "select id,IsMain,name from Banks";
    q.exec(s);
    while(q.next()){
        id = q.value(0).toInt();
        bankIds << id;
        bankNames << q.value(2).toString();
        isMains[id] = q.value(1).toBool();
    }

    s = "select code,name from MoneyTypes";
    q.exec(s);
    while(q.next()){
        mtIds << q.value(0).toInt();
        mtNames << q.value(1).toString();
    }
    for(int i = 0; i < bankIds.count(); ++i)
        for(int j = 0; j < mtIds.count(); ++j){
            s = QString("select accNum from BankAccounts where (bankId=%1) and "
                        "(mtID=%2)").arg(bankIds[i]).arg(mtIds[j]);
            if(q.exec(s) && q.first()){
                accNum = q.value(0).toString();
                s = QString("select FSAgent.id from FSAgent join SecSubjects on "
                            "FSAgent.sid = SecSubjects.id where SecSubjects.subName='%1'")
                        .arg(QString("%1-%2").arg(bankNames[i]).arg(mtNames[j]));
                bool r = q.exec(s);
                if(r && q.first()){
                    BankAccount* ba = new BankAccount;
                    ba->accNum = accNum;
                    ba->isMain = isMains.value(bankIds[i]);
                    ba->mt = mtIds[j];
                    ba->name = QString("%1-%2").arg(bankNames[i]).arg(mtNames[j]);
                    banks[q.value(0).toInt()] = ba;
                }
            }
        }
    if(banks.count() > 0)
        return true;
    else
        return false;
}

//刷新凭证集状态
bool BusiUtil::refreshPzsState(int y,int m,CustomRelationTableModel* model/*,
                      QSet<int> pcImps, QSet<int> pcJzhds,QSet<int> pcJzsys*/)


{
    //（此刷新动作主要使凭证状态从录入态转到审核态）
    //执行此函数的时机是，用户修改了凭证状态（而非凭证的其他数据）
    bool ori = false;  //初始态（任一手工录入凭证处于录入状态时，它为真）
    bool handV = true; //所有手工录入凭证处于审核或入账状态时，它为真
    bool handE = false;//手工录入凭证是否存在

    bool imp = false;  //引入态（任一自动引入的凭证还未审核或入账时，它为真）
    bool impV = true;  //引入审核态（所有自动引入的凭证都已审核或入账时，它为真）
    bool impE = false; //引入凭证是否存在

    bool jzhd = false;  //结转汇兑态（任一结转汇兑损益的凭证处于录入态时，它为真）
    bool jzhdV = true;  //结转汇兑审核态（所有结转汇兑损益的凭证已审核或入账时，它为真）
    bool jzhdE = false; //结转汇兑损益的凭证是否存在


    bool jzsy = false; //结转损益态（任一结转损益的凭证处于录入态时，它为真）
    bool jzsyV = true; //结转损益审核态（所有结转损益的凭证处于已审核或入账时，它为真）
    bool jzsyE = false;//结转损益的凭证是否存在

    bool jzlr = false; //结转本年利润态（结转本年利润的凭证处于录入态时，它为真）
    bool jzlrV = true; //结转本年利润态（结转本年利润的凭证已审核或入账时，它为真）
    bool jzlrE = false;//结转本年利润的凭证是否存在

    PzsState oldPs, newPs = Ps_Rec;  //原先保存的和新的凭证集状态
    if(!getPzsState(y,m,oldPs)){
        qDebug() << "Don't get current pingzheng set state!!";
        return false;
    }
    if(oldPs == Ps_Jzed)  //如果已结账，则不用继续
        return true;

    int state;
    int pzCls;
    for(int i = 0; i < model->rowCount(); ++i){
        state = model->data(model->index(i,PZ_PZSTATE)).toInt();
        pzCls = model->data(model->index(i,PZ_CLS )).toInt();

        if(state == Pzs_Repeal)
            continue;
        //如果是手工录入凭证
        if(pzCls == Pzc_Hand){
            handE = true;
            if(state == Pzs_Recording){
                ori = true;
                handV = false;
                break;
            }
        }
        //如果是其他模块引入的凭证
        else if(impPzCls.contains(pzCls)){
            impE = true;
            if(state == Pzs_Recording){
                imp = true;
                impV = false;
                break;
            }
        }
        //如果是结转汇兑损益的凭证
        else if(jzhdPzCls.contains(pzCls)){
            jzhdE = true;
            if(state == Pzs_Recording){
                jzhd = true;
                jzhdV = false;
                break;
            }
        }
        //如果是结转损益的凭证
        else if(jzsyPzCls.contains(pzCls)){
            jzsyE = true;
            if(state == Pzs_Recording){
                jzsy = true;
                jzsyV = false;
                break;
            }
        }
        //如果是结转本年利润的凭证
        else if((pzCls == Pzc_Jzlr)){
            jzlrE = true;
            if(state == Pzs_Recording){
                jzlr = true;
                jzlrV = false;
                break;
            }
        }
    }

    //根据凭证的扫描情况以及凭证集的先前状态，决定新状态
    if(ori)
        newPs = Ps_Rec;
    else if(imp)
        newPs = Ps_ImpOther;
    else if(jzhd)
        newPs = Ps_Jzhd;
    else if(jzsy)
        newPs = Ps_Jzsy;
    else if(jzlr)
        newPs = Ps_Jzbnlr;

    else if((oldPs == Ps_Jzbnlr)  && jzlrV && jzlrE)   //如果已经审核了结转本年利润凭证
        newPs = Ps_JzbnlrV;
    else if((oldPs == Ps_Jzsy) && jzsyV && jzsyE) //如果结转损益凭证都进行了审核
        newPs = Ps_JzsyV;
    else if((oldPs == Ps_Jzhd) && jzhdV && jzhdE) //如果结转汇兑损益凭证都进行了审核
        newPs = Ps_JzhdV;
    else if((oldPs == Ps_ImpOther) && impV && impE) //如果引入的其他凭证都审核通过
        newPs = Ps_ImpV;
    else if((oldPs == Ps_Rec) && handV && handE) //如果所有手工录入的凭证都审核通过
        newPs = Ps_HandV;

    setPzsState(y,m,newPs);
}

//取消结转汇兑损益凭证
bool BusiUtil::antiJzhdsyPz(int y, int m)
{
    PzsState state;
    getPzsState(y,m,state);
    if((state < Ps_ImpOther) && (state >= Ps_JzsyPre)){
        QMessageBox::warning(0,QObject::tr("操作拒绝"),
            QObject::tr("执行此操作前，必须已经结转了汇兑损益，并且还未执行损益类科目的结转！"));
        return false;
    }
    return delSpecPz(y,m,Pzc_Jzhd_Bank) &&
           delSpecPz(y,m,Pzc_Jzhd_Ys) &&
           delSpecPz(y,m,Pzc_Jzhd_Yf);
}

//生成固定资产折旧凭证
bool BusiUtil::genGdzcZjPz(int y,int m)
{

    return true;
}

//生成待摊费用凭证
bool BusiUtil::genDtfyPz(int y,int m)
{

    return true;
}

//引入其他模块产生的凭证
bool BusiUtil::impPzFromOther(int y,int m, QSet<OtherModCode> mods)
{
    PzsState state;
    getPzsState(y,m,state);
    if((state != Ps_Stat1) && (state != Ps_Stat3)){
        QMessageBox::warning(0,QObject::tr("提示信息"),
            QObject::tr("执行此操作前，必须先统计保存了所有的手工凭证或引入凭证之后"));
        return false;
    }

    if(!mods.empty()){
        //一个模块的出错，应不影响其他模块的执行
        bool err = false;
        if(mods.contains(OM_GDZC) && !genGdzcZjPz(y,m)){
            err = true;
            QMessageBox::warning(0,QObject::tr("提示信息"),
                QObject::tr("在执行引入固定资产折旧凭证时出错！！"));
        }
        if(mods.contains(OM_DTFY) && !genDtfyPz(y,m)){
            err = true;
            QMessageBox::warning(0,QObject::tr("提示信息"),
                QObject::tr("在执行引入待摊费用凭证时出错！！"));
        }
        //如果所有要引入的模块都未发生错误，则更新凭证集状态
        if(!err){
            BusiUtil::setPzsState(y,m,Ps_ImpOther);
            return true;
        }
        else
            return false;
    }    
    return true;
}

//取消引入的由其他模块产生的凭证
bool BusiUtil::antiImpPzFromOther(int y, int m, QSet<OtherModCode> mods)
{
    PzsState state;
    getPzsState(y,m,state);
    if((state < Ps_ImpOther) && (state >= Ps_JzsyPre)){
        QMessageBox::warning(0,QObject::tr("操作拒绝"),
            QObject::tr("执行此操作前，必须已经引入了由其他模块产生的凭证，并且未执行损益类科目的结转！"));
        return false;
    }
    if(!mods.empty()){
        bool err = false;
        if(mods.contains(OM_GDZC) && !antiGdzcPz(y,m)){
            err = true;
            QMessageBox::warning(0,QObject::tr("提示信息"),
                QObject::tr("在执行取消引入的固定资产折旧凭证时出错！！"));
        }
        if(mods.contains(OM_DTFY) && !antiDtfyPz(y,m)){
            err = true;
            QMessageBox::warning(0,QObject::tr("提示信息"),
                QObject::tr("在执行引入取消引入的待摊费用凭证时出错！！"));
        }
        //如果所有要引入的模块都未发生错误，则更新凭证集状态
        if(!err){
            bool req;
            BusiUtil::reqGenJzHdsyPz(y,m,req);
            if(req) //如果还没有进行汇兑损益的结转，则置新状态为进行结转汇兑损益前的状态
                BusiUtil::setPzsState(y,m,Ps_Stat1);
            else
                BusiUtil::setPzsState(y,m,Ps_Stat3);
            return true;
        }
        else
            return false;
    }
    return true;
}

//取消固定资产管理模块引入的凭证
bool BusiUtil::antiGdzcPz(int y, int m)
{
    return delSpecPz(y,m,Pzc_GdzcZj);
}

//取消待摊费用管理模块引入的凭证
bool BusiUtil::antiDtfyPz(int y, int m)
{
    return delSpecPz(y,m,Pzc_Dtfy);
}

//是否是由其他模块引入的凭证类别
bool BusiUtil::isImpPzCls(PzClass pzc)
{
    return impPzCls.contains(pzc);
}
//是否是由系统自动产生的结转汇兑损益凭证类别
bool BusiUtil::isJzhdPzCls(PzClass pzc)
{
    return impPzCls.contains(pzc);
}
//是否是由系统自动产生的结转损益凭证类别
bool BusiUtil::isJzsyPzCls(PzClass pzc)
{
    return impPzCls.contains(pzc);
}
//是否是其他由系统产生并允许用户修改的凭证类别
bool BusiUtil::isOtherPzCls(PzClass pzc)
{
    return impPzCls.contains(pzc);
}

//判断其他模块是否需要在指定年月产生引入凭证，如果是，则置req为true
bool BusiUtil::reqGenImpOthPz(int y,int m, bool& req)
{
    //任一模块需要产生引入凭证则置req为true，目前仅考虑固定资产和待摊费用
    if(reqGenGdzcPz(y,m,req) && reqGenDtfyPz(y,m,req))
        return true;
    else
        return false;
}

//判断固定资产管理模块是否需要产生凭证
bool BusiUtil::reqGenGdzcPz(int y,int m, bool& req)
{
    //只有在根据当前的固定资产折旧情况，需要进行折旧，且凭证集内不存在相关凭证时，则置req为true
    req = false;
    return true;
}

//判断待摊费用管理模块是否需要产生凭证
bool BusiUtil::reqGenDtfyPz(int y,int m, bool& req)
{
    //只有在根据当前待摊费用的扣除情况，需要进行摊销，且凭证集内不存在相关凭证时，则置req为true
    req = false;
    return true;
}

//判断是否需要结转汇兑损益，进行汇兑损益的结转必须有两个前提
//1、当前月与下一月份之间的汇率差不为0，2、相关科目存在外币余额，且凭证集内无对应凭证）
bool BusiUtil::reqGenJzHdsyPz(int y, int m, bool& req)
{
    QSqlQuery q;
    QHash<int,double> r1,r2;
    QString s;
    //bool req2 = false;
    bool exsit = true;   //3个科目中存在外币余额的科目是否在凭证集内存在相应结转汇兑损益凭证
                         //只要有一个科目有外币余额，且不存在结转凭证，即为false。
    bool rateDiff = false;  //汇率差是否为0标记，默认为0。

    //结转汇兑损益有3种类型的凭证（银行、应收/应付）缺一不可（当然，还要考虑是否存在外币余额，）
    //只有余额不为0，才有必要执行结转汇兑损益操作
    int i = 0;
    QList<int> ids;
    ids<<0<<0<<0;
    getIdByCode(ids[0],"1002");//银行存款
    getIdByCode(ids[1],"1131");//应收账款
    getIdByCode(ids[2],"2121");//应付账款
    QHash<int,double> extras;
    QHash<int,int> dirs;
    int pzCls = 0;
    while((i<3) && exsit){
        readExtraForSub(y,m,ids[i],extras,dirs);
        if(extras.empty())
            i++;
        else if((extras.count() == 1) && extras.keys()[0] == RMB) //如果只有人民币余额
            i++;
        else{
            i++;
            if(i==0)
                pzCls = Pzc_Jzhd_Bank;
            else if(i == 1)
                pzCls = Pzc_Jzhd_Ys;
            else
                pzCls = Pzc_Jzhd_Yf;
            s = QString("select id from PingZhengs where isForward = %1").arg(pzCls);
            bool r = q.exec(s);
            if(r && !q.first()){
                exsit = false;
                break;
            }
        }
    }

    int yy,mm;
    if(m == 12){
        yy = y + 1;
        mm = 1;
    }
    else{
        yy = y;
        mm = m + 1;
    }
    //如果汇率读取错误，则返回假
    if(!getRates(y,m,r1) || (!getRates(yy,mm,r2)))
        return false;
    //如果汇率设置不完整，则有可能需要进行结转
    if(r1.count() != r2.count()){
        rateDiff = true;
    }
    QHashIterator<int,double> it(r1);
    while(it.hasNext()){
        it.next();
        //如果汇率设置不完整，则有可能需要进行结转
        if(!r2.contains(it.key())){
            rateDiff = true;
        }
        //如果有汇率差，就必须进行结转
        else if(it.value() != r2.value(it.key())){
            rateDiff = true;
        }
    }

    req = rateDiff && !exsit;
    return true;
}

//生成结转本年利润的凭证
bool BusiUtil::genJzbnlr(int y, int m, PzData& d)
{
    PzsState state;
    getPzsState(y,m,state);
    if(state != Ps_Stat4){
        QMessageBox::warning(0,QObject::tr("提示信息"),
            QObject::tr("在结转本年利润前，必须统计保存结转损益后的余额"));
        return false;
    }
    return crtNewPz(&d);
}

//取消结转本年利润的凭证
bool BusiUtil::antiJzbnlr(int y, int m)
{
    return delSpecPz(y,m,Pzc_Jzlr) && setPzsState(y,m,Ps_Stat4);
}

//是否需要创建结转本年利润的凭证
bool BusiUtil::reqGenJzbnlr(int y, int m, bool& req)
{
    PzsState state;
    bool r = getPzsState(y,m,state);
    if(state < Ps_Stat5){
        if(jzlrByYear){
            if(m == 12)
                req = true;
            else
                req = false;
        }
        else
            req = false;
    }
    req = false;
    return r;
}


//获取指定范围的科目id列表
//参数sfid，ssid代表开始的一级、二级科目id，efid、esid代表结束的一级、二级科目id
//fids和sids是返回的指定范围的一二级科目id列表
bool BusiUtil::getSubRange(int sfid,int ssid,int efid,int esid,
                           QList<int>& fids,QHash<int,QList<int> >& sids)
{
    QSqlQuery q;
    QString s;
    bool r;

    //获取开始和结束一级科目的科目代码
    QString sfcode,efcode;
    s = QString("select subCode from FirSubjects where id=%1").arg(sfid);
    if(!q.exec(s) || !q.first()){
        qDebug() << QString("error! while get first subject(id:%1)").arg(sfid);
        return false;
    }
    sfcode = q.value(0).toString();
    s = QString("select subCode from FirSubjects where id=%1").arg(efid);
    if(!q.exec(s) || !q.first()){
        qDebug() << QString("error! while get first subject(id:%1)").arg(efid);
        return false;
    }
    efcode = q.value(0).toString();

    //获取一级科目的范围
    s = QString("select id from FirSubjects where (subCode>=%1) and (subCode<=%2) ")
            .arg(sfcode).arg(efcode);
    q.exec(s);
    while(q.next()){
        fids<<q.value(0).toInt();
    }


    //获取二级科目的范围
    QList<int> ids;
    QList<QString> name;
    for(int i = 0; i < fids.count(); ++i){
        getSndSubInSpecFst(fids[i],ids,name);
        //开始或结束的一级科目，在范围选择模式时，可能只选择了部分二级科目，因此必须特别处理
        //但对于其他选择模式，则隐含选择所有
        if((sfid == fids[i]) && (ssid != 0)){
            for(int j = 0; j < ids.count(); ++j){
                if((ids[j] >= ssid) && (ids[j] <= esid))
                    sids[fids[i]]<<ids[j];
            }
        }
        else if((efid == fids[i]) && (esid != 0)){
            for(int j = 0; j < ids.count(); ++j){
                if((ids[j] >= ssid) && (ids[j] <= esid))
                    sids[fids[i]]<<ids[j];
            }
        }
        else
            sids[fids[i]] = ids;
    }
}

//指定id的凭证是否处于指定的年月内
bool BusiUtil::isPzInMonth(int y, int m, int pzid, bool& isIn)
{
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("select date from PingZhengs where id = %1").arg(pzid);
    if(!q.exec(s) || !q.first())
        return false;
    s = q.value(0).toString();
    int year = s.left(4).toInt();
    int month = s.mid(5,2).toInt();
    if((year == y) && (month == m))
        isIn = true;
    else
        isIn = false;
    return true;
}

//获取指定一级科目是否需要按币种进行分别核算
bool BusiUtil::isAccMt(int fid)
{
    return accToMt.contains(fid);
}

//获取指定二级科目是否需要按币种进行分别核算
bool BusiUtil::isAccMtS(int sid)
{
    QSqlQuery q;
    QString s;
    s = QString("select fid from FSAgent where id=%1").arg(sid);
    if(!q.exec(s) || !q.first())
        return false;
    else{
        int fid = q.value(0).toInt();
        return isAccMt(fid);
    }
}

//获取所有在二级科目类别表中名为“固定资产类”的科目
//bool BusiUtil::getGdzcSubClass(QHash<int,QString>& names)
//{
//    QSqlQuery q;
//    QString s;

//    s = QObject::tr("select clsCode from SndSubClass where name = '固定资产类'");
//    if(!q.exec(s) || !q.first())
//        return false;
//    int code = q.value(0).toInt();

//    s = QString("select id,subName from SecSubjects where classId=%1").arg(code);
//    if(!q.exec(s))
//        return false;
//    while(q.next()){
//        int id = q.value(0).toInt();
//        QString name = q.value(1).toString();
//        names[id] = name;
//    }
//    return true;
//}

////////////////////////////////////////////////////////////////////////////
//获取子窗口信息
bool VariousUtils::getSubWinInfo(int winEnum, SubWindowDim*& info, QByteArray*& otherInfo)
{
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("select * from subWinInfos where winEnum = %1").arg(winEnum);
    r = q.exec(s);
    r = q.first();
    if(r){
        info = new SubWindowDim;
        info->x = q.value(SWI_X).toInt();
        info->y = q.value(SWI_Y).toInt();
        info->w = q.value(SWI_W).toInt();
        info->h = q.value(SWI_H).toInt();
        otherInfo = new QByteArray(q.value(SWI_TBL).toByteArray());
    }
    else{
        info = NULL;
        otherInfo = NULL;
    }
    return r;
}

//保存字窗口信息
bool VariousUtils::saveSubWinInfo(int winEnum, SubWindowDim* info, QByteArray* otherInfo)
{
    QSqlQuery q;
    QString s;
    bool r;

    if(otherInfo == NULL)
        otherInfo = new QByteArray;
    s = QString("select * from subWinInfos where winEnum = %1").arg(winEnum);
    if(q.exec(s) && q.first()){
        int id = q.value(0).toInt();
        s = QString("update subWinInfos set winEnum= :enum,x=:x,y=:y,w=:w,h=:h"
                    ",tblInfo=:info where id=:id");
        r = q.prepare(s);
        q.bindValue(":enum",winEnum);
        q.bindValue(":x",info->x);
        q.bindValue(":y",info->y);
        q.bindValue(":w",info->w);
        q.bindValue(":h",info->h);
        q.bindValue(":info",*otherInfo);
        q.bindValue(":id",id);
    }
    else{
        s = QString("insert into subWinInfos(winEnum,x,y,w,h,tblInfo) "
                    "values(:enum,:x,:y,:w,:h,:info)");
        r = q.prepare(s);
        q.bindValue(":enum",winEnum);
        q.bindValue(":x",info->x);
        q.bindValue(":y",info->y);
        q.bindValue(":w",info->w);
        q.bindValue(":h",info->h);
        q.bindValue(":info",*otherInfo);
    }
    r = q.exec();
    return r;
}

//读取子窗口信息
//QVariant VariousUtils::getSubWinInfo2(int winEnum)
//{
//    QSqlQuery q;
//    QString s;

//    s = QString("select * from subWinInfos where winEnum = %1").arg(winEnum);
//    if(q.exec(s) && q.first()){
//        QVariant v;
//        QByteArray ba = q.value(SWI_TBL).toByteArray();
//        QBuffer bf(&ba);
//        bf.open(QIODevice::ReadOnly);
//        QDataStream in(&bf);
//        in>>v;
//        return v;
//    }
//    return QVariant();
//}

//保存子窗口信息
//bool VariousUtils::saveSubWinInfo2(int winEnum, QVariant otherInfo)
//{
//    QSqlQuery q;
//    QString s;
//    bool r;

//    //一定要将QVariant类型序列化到字节数组中，才能在数据库中保存
//    QByteArray ba;
//    QBuffer buffer(&ba);
//    buffer.open(QIODevice::WriteOnly);
//    QDataStream out(&buffer);
//    out<<otherInfo;
//    buffer.close();

//    s = QString("select * from subWinInfos where winEnum = %1").arg(winEnum);
//    if(q.exec(s) && q.first()){
//        int id = q.value(0).toInt();
//        s = QString("update subWinInfos set winEnum=:enum,tblInfo=:info where id=:id");
//        r = q.prepare(s);
//        q.bindValue(":enum",winEnum);
//        q.bindValue(":info",ba);
//        q.bindValue(":id",id);
//    }
//    else{
//        s = QString("insert into subWinInfos(winEnum,tblInfo) "
//                    "values(:enum,:info)");
//        r = q.prepare(s);
//        q.bindValue(":enum",winEnum);
//        q.bindValue(":info",ba);
//    }
//    r = q.exec();
//    return r;
//}

//获取子窗口信息
bool VariousUtils::getSubWinInfo3(int winEnum,QByteArray*& ba)
{
    QSqlQuery q;
    QString s;

    s = QString("select * from subWinInfos where winEnum = %1").arg(winEnum);
    if(q.exec(s) && q.first()){
        ba = new QByteArray(q.value(SWI_TBL).toByteArray());
        return true;
    }
    return false;
}

//保存字窗口信息
bool VariousUtils::saveSubWinInfo3(int winEnum, QByteArray* otherInfo)
{
    QSqlQuery q;
    QString s;
    bool r;

    s = QString("select * from subWinInfos where winEnum = %1").arg(winEnum);
    if(q.exec(s) && q.first()){
        int id = q.value(0).toInt();
        s = QString("update subWinInfos set winEnum=:enum,tblInfo=:info where id=:id");
        r = q.prepare(s);
        q.bindValue(":enum",winEnum);
        q.bindValue(":info",*otherInfo);
        q.bindValue(":id",id);
    }
    else{
        s = QString("insert into subWinInfos(winEnum,tblInfo) "
                    "values(:enum,:info)");
        r = q.prepare(s);
        q.bindValue(":enum",winEnum);
        q.bindValue(":info",*otherInfo);
    }
    r = q.exec();
    return r;
}


