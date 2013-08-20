#include <QStringList>
#include <QBuffer>

#include "tables.h"
#include "utils.h"
#include "securitys.h"
#include "subject.h"


QSet<int> BusiUtil::pset; //具有正借负贷特性的一级科目id集合
QSet<int> BusiUtil::nset; //具有负借正贷特性的一级科目id集合
QSet<int> BusiUtil::spset; //具有正借负贷特性的二级科目id集合
QSet<int> BusiUtil::snset; //具有负借正贷特性的二级科目id集合

QSet<int> fidByMt; //需要按币种进行明细统计功能的一级科目id集合（这是临时代码，这个信息需要在一级科目表中反映）

QSet<int> BusiUtil::impPzCls;   //由其他模块引入的凭证类别代码集合
//QSet<int> BusiUtil::jzhdPzCls;  //由系统自动产生的结转汇兑损益凭证类别代码集合
QSet<int> BusiUtil::jzsyPzCls;  //由系统自动产生的结转损益凭证类别代码集合
QSet<int> BusiUtil::otherPzCls; //其他由系统产生并允许用户修改的凭证类别代码集合
QSet<int> BusiUtil::accToMt;    //需要按币种核算的科目id集合

QSet<int> BusiUtil::inIds;  //损益类科目中的收入类科目id集合
QSet<int> BusiUtil::feiIds; //损益类科目中的费用类科目id集合
QSet<int> BusiUtil::inSIds;  //损益类科目中的收入类子目id集合
QSet<int> BusiUtil::feiSIds; //损益类科目中的费用类子目id集合

QSqlDatabase BusiUtil::db;

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
bool BusiUtil::init(QSqlDatabase& db)
{
    BusiUtil::db = db;
    QSqlQuery q(db),q2(db);

    //初始化一二级科目的借贷方向特效id集合
    //资产类（正：借，负：贷）
    //负债类（负：借，正：贷）
    //所有者权益类：（正：贷，负：借）
    //损益类-收入（正：贷，负：借）
    //损益类-费用（正：借，负：贷）
    QString s = QString("select id from %1 where %2=1 and %3=1")
            .arg(tbl_fsub).arg(fld_fsub_jddir).arg(fld_fsub_isview);
    if(!q.exec(s))
        return false;
    pset.clear();
    while(q.next())
        pset.insert(q.value(0).toInt());

    s = QString("select id from %1 where %2=0 and %3=1")
            .arg(tbl_fsub).arg(fld_fsub_jddir).arg(fld_fsub_isview);
    if(!q.exec(s))
        return false;
    nset.clear();
    while(q.next())
        nset.insert(q.value(0).toInt());

    int fid;
    QList<int> ids;
    QList<QString> names;
    QSetIterator<int>* it = new QSetIterator<int>(pset);
    spset.clear();
    while(it->hasNext()){
        fid = it->next();
        getSndSubInSpecFst(fid,ids,names);
        foreach(int id, ids)
            spset << id;
    }

    ids.clear();
    names.clear();
    it = new QSetIterator<int>(nset);
    snset.clear();
    while(it->hasNext()){
        fid = it->next();
        getSndSubInSpecFst(fid,ids,names);
        foreach(int id, ids)
            snset << id;
    }

    //初始化凭证类别代码集合
    impPzCls.clear();
    impPzCls.insert(Pzc_GdzcZj);
    impPzCls.insert(Pzc_DtfyTx);

    //这些可以通过访问全局变量pzClsJzhds来获得
//    jzhdPzCls.clear();
//    jzhdPzCls.insert(Pzc_Jzhd_Bank);
//    jzhdPzCls.insert(Pzc_Jzhd_Ys);
//    jzhdPzCls.insert(Pzc_Jzhd_Yf);

    jzsyPzCls.clear();
    jzsyPzCls.insert(Pzc_JzsyIn);
    jzsyPzCls.insert(Pzc_JzsyFei);

    otherPzCls.clear();
    otherPzCls.insert(Pzc_Jzlr);

    //初始化需要按币种分别核算的科目的集合
    s = QString("select id from %1 where %2=1").arg(tbl_fsub).arg(fld_fsub_isUseWb);
    if(!q.exec(s))
        return false;
    accToMt.clear();
    while(q.next())
        accToMt.insert(q.value(0).toInt());

    //初始化损益类科目中收入类和费用类科目id集合
    s = QString("select id,%1 from %2 where %3=5 and %4=1")
            .arg(fld_fsub_jddir).arg(tbl_fsub).arg(fld_fsub_class).arg(fld_fsub_isview);
    if(!q.exec(s))
        return false;
    int dir,sid;
    feiIds.clear();inIds.clear();
    feiSIds.clear();inSIds.clear();
    while(q.next()){
        fid = q.value(0).toInt();
        dir = q.value(1).toInt();
        if(dir == 1)
            feiIds.insert(fid);
        else
            inIds.insert(fid);
        s = QString("select id from %1 where %2 = %3")
                .arg(tbl_ssub).arg(fld_ssub_fid).arg(fid);
        if(!q2.exec(s))
            return false;
        while(q2.next()){
            sid = q2.value(0).toInt();
            if(dir == 1)
                feiSIds.insert(sid);
            else
                inSIds.insert(sid);
        }
    }
    return true;
}

/**
 * @brief BusiUtil::getRates2
 *  读取指定年月的汇率
 * @param y
 * @param m
 * @param rates     汇率表，键为币种代码
 * @param mainMt    记账货币（母币）
 * @return
 */
bool BusiUtil::getRates2(int y, int m, QHash<int,Double>& rates, int mainMt){
    QSqlQuery q(db);
    QString s = QString("select * from %1").arg(tbl_moneyType).arg(fld_mt_code);
    if(!q.exec(s))
        return false;
    QString mainSign; //母币符号
    QHash<int,QString> msHash; //货币代码到货币符号的映射
    int mt; QString mtSign;
    while(q.next()){
        mt = q.value(MT_CODE).toInt();
        mtSign = q.value(MT_SIGN).toString();
        if(mt != mainMt)
            msHash[mt] = mtSign;
        else
            mainSign = mtSign;
    }

    QList<int> mtcs = msHash.keys();
    s = QString("select ");
    for(int i = 0; i<mtcs.count(); ++i){
        s.append(msHash.value(mtcs.at(i)));
        s.append(QString("2%1,").arg(mainSign));
    }
    s.chop(1);
    s.append(QString(" from %1 ").arg(tbl_rateTable));
    s.append(QString("where (%1 = %2) and (%3 = %4)")
             .arg(fld_rt_year).arg(y).arg(fld_rt_month).arg(m));
    if(!q.exec(s))
        return false;
    if(!q.first()){
        qDebug() << QObject::tr("没有指定汇率！");
        return false;
    }
    for(int i = 0;i<mtcs.count();++i)
        rates[mtcs.at(i)] = Double(q.value(i).toDouble());
    return true;
}

/**
 * @brief BusiUtil::saveRates2
    保存指定年月的汇率
 * @param y
 * @param m
 * @param rates     汇率表，键为币种代码
 * @param mainMt    记账货币（母币）
 * @return
 */
bool BusiUtil::saveRates2(int y, int m, QHash<int, Double> &rates, int mainMt)
{
    QSqlQuery q(db);
    QString s,vs;

    QList<int> wbCodes;     //外币币种代码
    QList<QString> wbSigns; //外币符号列表
    QList<QString> mtFields; //存放与外币币种对应汇率的字段名（按序号一一对应）
    s = QString("select %1,%2 from %3").arg(fld_mt_code).arg(fld_mt_sign).arg(tbl_moneyType);
    if(!q.exec(s))
        return false;
    QString mainSign,mtSign;
    int mt;
    while(q.next()){
        mt = q.value(0).toInt();
        mtSign = q.value(1).toString();
        if(mt == mainMt)
            mainSign = mtSign;
        else{
            wbCodes << mt;
            wbSigns << mtSign;
        }
    }
    for(int i = 0; i < wbCodes.count(); ++i)
        mtFields << wbSigns.at(i) + "2" + mainSign;
    s = QString("select id from %1 where (%2 = %3) and (%4 = %5)")
            .arg(tbl_rateTable).arg(fld_rt_year).arg(y).arg(fld_rt_month).arg(m);
    if(!q.exec(s))
        return false;
    if(q.first()){
        int id = q.value(0).toInt();
        s = QString("update %1 set ").arg(tbl_rateTable);
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(wbCodes.at(i)))
                s.append(QString("%1=%2,").arg(mtFields.at(i))
                         .arg(rates.value(wbCodes.at(i)).toString()));

        }

        s.chop(1);
        s.append(QString(" where id = %1").arg(id));
        if(!q.exec(s))
            return false;
    }
    else{
        s = QString("insert into %1(%2,%3,").arg(tbl_rateTable).arg(fld_rt_year).arg(fld_rt_month);
        vs = QString("values(%1,%2,").arg(y).arg(m);
        for(int i = 0; i < mtFields.count(); ++i){
            if(rates.contains(wbCodes.at(i))){
                s.append(mtFields.at(i)).append(",");
                vs.append(rates.value(wbCodes[i]).toString()).append(",");
            }
        }
        s.chop(1); vs.chop(1);
        s.append(") "); vs.append(")");
        s.append(vs);
        if(!q.exec(s))
            return false;
    }
    return true;
}

/**
 * @brief BusiUtil::getFstSubCls
 *  获取一级科目类别代码
 *  此功能将并入科目管理器对象中
 * @param clsNames
 * @param subSys
 * @return
 */
//bool BusiUtil::getFstSubCls(QHash<int, QString> &clsNames, int subSys)
//{
//    QSqlQuery q;
//    QString s = QString("select %1,%2 from %3 where %4=%5")
//            .arg(fld_fsc_code).arg(fld_fsc_name).arg(tbl_fsclass).arg(fld_fsc_subSys)
//            .arg(subSys);
//    if(!(q.exec(s) && q.first())){
//        QMessageBox::information(0,QObject::tr("提示信息"),
//                                 QString(QObject::tr("未能一级科目类别")));
//        return false;
//    }
//    q.seek(-1);
//    while(q.next())
//        clsNames[q.value(0).toInt()] = q.value(1).toString();
//}
/**
 * @brief BusiUtil::getMTName
 *  获取币种代码表
 * @param names
 * @return
 */
bool BusiUtil::getMTName(QHash<int, QString> &names)
{
    QSqlQuery q(db);
    QString s;

    s = QString("select %1,%2 from %3").arg(fld_mt_code).arg(fld_mt_name).arg(tbl_moneyType);
    if(!q.exec(s))
        return false;
    while(q.next())
        names[q.value(0).toInt()] = q.value(1).toString();
    return true;
}

/**
 * @brief BusiUtil::getSubCodeByName
 *  按科目名称获取科目代码（一级科目）
 * @param code      科目代码
 * @param name      科目名称
 * @param subSys    科目系统代码
 * @return
 */
//bool BusiUtil::getSubCodeByName(QString &code, QString name, int subSys)
//{
//        QSqlQuery q;
//        QString s = QString("select %1 from %2 where %3=%4 and %5='%6'")
//                .arg(fld_fsub_subcode).arg(tbl_fsub).arg(fld_fsub_subSys)
//                .arg(subSys).arg(fld_fsub_name).arg(name);
//        if(!q.exec(s))
//            return false;
//        if(!q.first()){
//            QMessageBox::information(0,QObject::tr("提示信息"),
//                                     QString(QObject::tr("未能找到科目%1")).arg(name));
//            return false;
//        }
//        code = q.value(0).toString();
//        return true;
//}

/**
 * @brief BusiUtil::getIdByCode
 *  按科目代码获取该科目在一级科目表中的id值
 * @param id
 * @param code
 * @return
 */
bool BusiUtil::getIdByCode(int &id, QString code, int subSys)
{
    QSqlQuery q(db);
    QString s = QString("select id from %1 where %2 = %3 and %4 = '%5'")
            .arg(tbl_fsub).arg(fld_fsub_subSys).arg(subSys)
            .arg(fld_fsub_subcode).arg(code);
    if(!q.exec(s))
        return false;
    if(!q.first()){
        QMessageBox::information(0,QObject::tr("提示信息"),
                                 QString(QObject::tr("未能找到科目代码%1")).arg(code));
        return false;
    }
    id = q.value(0).toInt();
    return true;
}

/**
 * @brief BusiUtil::getSidByName
 *  按科目名称获取该二级科目id
 * @param fname     一级科目名称
 * @param sname     二级科目名称
 * @param id        明细科目id
 * @return
 */
//bool BusiUtil::getSidByName(QString fname, QString sname, int &id, int subSys)
//{
//        QSqlQuery q;
//        QString s;
//        int fid;
//        if(!getIdByName(fid,fname,subSys))
//            return false;
//        s = QString("select %1.id from %1 join %2 where "
//                    "(%1.%3=%2.id) and (%1.%4=%5) "
//                    "and (%2.%6='%7')").arg(tbl_ssub).arg(tbl_nameItem).arg(fld_ssub_nid)
//                .arg(fld_ssub_fid).arg(fid).arg(fld_ni_name).arg(sname);
//        if(!q.exec(s))
//            return false;
//        if(!q.first())
//            return false;
//        id = q.value(0).toInt();
//        return true;
//}

/**
 * @brief BusiUtil::getIdByName
 *  按科目名称获取该科目在一级科目表中的id值
 * @param id
 * @param name
 * @return
 */
//bool BusiUtil::getIdByName(int &id, QString name, int subSys)
//{
//        QSqlQuery q;
//        QString s;

//        s = QString("select id from %1 where %2 = %3 and %4 = '%5'").arg(tbl_fsub)
//                .arg(fld_fsub_subSys).arg(subSys).arg(fld_fsub_name).arg(name);
//        if(!q.exec(s))
//            return false;
//        if(!q.first()){
//            QMessageBox::information(0,QObject::tr("提示信息"),
//                                     QString(QObject::tr("未能找到科目：%1")).arg(name));
//            return false;
//        }
//        id = q.value(0).toInt();
//        return true;
//}

/**
 * @brief BusiUtil::getIdsByCls
 *  获取指定科目类别下的所有一级科目的id
        参数cls：，isByView：
 * @param ids       一级科目类别代码
 * @param cls       科目类别
 * @param isByView  是否只输出配置为显示的一级科目
 * @return
 */
//bool BusiUtil::getIdsByCls(QList<int> &ids, int cls, bool isByView, int subSys)
//{
//    QSqlQuery q;
//    QString s;

//    s = QString("select id from %1 where %2=%3 and %4 = %5").arg(tbl_fsub)
//            .arg(fld_fsub_subSys).arg(subSys).arg(fld_fsub_class).arg(cls);
//    if(isByView)
//        s.append(QString(" and %1 = 1").arg(fld_fsub_isview));
//    if(!q.exec(s))
//        return false;
//    while(q.next())
//        ids.append(q.value(0).toInt());
//    return true;
//}

/**

    参数，isIncome：
*/
/**
 * @brief BusiUtil::getAllIdForSy
 *  获取所有指定类别的损类总账和明细科目的id列表
 * @param isIncome  true（收入类），false（费用类）
 * @param ids
 * @return
 */
//bool BusiUtil::getAllIdForSy(bool isIncome, QHash<int, QList<int> >& ids, int subSys)
//{
//    QString s;
//    QSqlQuery q;

//    //获取损益类科目类别代码
//    s = QString("select %1 from %2 where %3=%4 and %5='%6'")
//            .arg(fld_fsc_code).arg(tbl_fsclass).arg(fld_fsc_subSys)
//            .arg(subSys).arg(fld_fsc_name).arg(QObject::tr("损益类"));
//    if(!q.exec(s))
//        return false;
//    if(!q.first())
//        return false;
//    int cid = q.value(0).toInt();
//    s = QString("select id from %1 where (%2=%3) and "
//                "(%4=%5) and (%6=1)").arg(tbl_fsub).arg(fld_fsub_class).arg(cid)
//            .arg(fld_fsub_jddir).arg(isIncome?0:1).arg(fld_fsub_isview);
//    if(!q.exec(s))
//        return false;
//    QList<QString> snames;
//    while(q.next()){
//        int fid = q.value(0).toInt();
//        getSndSubInSpecFst(fid,ids[fid],snames);
//    }
//    return true;
//}


/**
 * @brief BusiUtil::getAllSubFName
 *  获取所有总目id到总目名的哈希表
 * @param names
 * @param isByView
 * @return
 */
//bool BusiUtil::getAllSubFName(QHash<int,QString>& names, bool isByView)
//{
//    QSqlQuery q;

//    QString s = QString("select id,%1 from %2").arg(fld_fsub_name).arg(tbl_fsub);
//    if(isByView)
//        s.append(QString(" where %1=1").arg(fld_fsub_isview));
//    if(q.exec(s)){
//        if(names.count() > 0)
//            names.clear();
//        while(q.next())
//            names[q.value(0).toInt()] = q.value(1).toString();
//        return true;
//    }
//    return false;
//}

/**
 * @brief BusiUtil::getAllSubFCode
 *  获取所有总目id到总目代码的哈希表
 * @param codes
 * @param isByView
 * @return
 */
//bool BusiUtil::getAllSubFCode(QHash<int,QString>& codes, bool isByView)
//{
//    QSqlQuery q;
//    QString s = QString("select id,%1 from %2")
//            .arg(fld_fsub_subcode).arg(tbl_fsub);
//    if(isByView)
//        s.append(QString(" where %3=1").arg(fld_fsub_isview));
//    if(q.exec(s)){
//        if(!codes.empty())
//            codes.clear();
//        while(q.next())
//            codes[q.value(0).toInt()] = q.value(1).toString();
//        return true;
//    }
//    return false;
//}

/**
    获取所有子目id到子目名的哈希表
*/
//bool BusiUtil::getAllSubSName(QHash<int,QString>& names)
//{
//    QSqlQuery q;
//    QString s;

//    s = QString("select %1.id,%2.%3 from %1 "
//                "join %2 where %1.%4 = %2.id")
//            .arg(tbl_ssub).arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ssub_nid);
//    if(!q.exec(s)){
//        QMessageBox::information(0, QObject::tr("提示信息"),
//                                 QString(QObject::tr("不能获取所有子科目哈希表")));
//        return false;
//    }
//    if(names.count() > 0)
//        names.clear();
//    while(q.next()){
//        int id = q.value(0).toInt();
//        names[id] = q.value(1).toString();
//    }
//    return true;
//}

/**
 * @brief BusiUtil::getAllSndSubNameList
 *  获取所有二级科目所用的名称条目
 * @param names
 * @return
 */\
//bool BusiUtil::getAllSndSubNameList(QStringList& names)
//{
//    QSqlQuery q;
//    QString s = QString("select %1 from %2 order by %3")
//            .arg(fld_ni_name).arg(tbl_nameItem).arg(fld_ni_lname);
//    if(!q.exec(s))
//        return false;
//    while(q.next())
//        names.append(q.value(0).toString());
//    return true;
//}

/**
 * @brief BusiUtil::getAllSubSCode
 *  获取所有子目id到子目代码的哈希表
 * @param codes
 * @return
 */
//bool BusiUtil::getAllSubSCode(QHash<int, QString> &codes)
//{
//    QSqlQuery q;
//    QString s = QString("select %1.id,%1.%2 from %1 join %3 where %1.%4 = %3.id")
//            .arg(tbl_ssub).arg(fld_ssub_code).arg(tbl_nameItem).arg(fld_ssub_nid);
//    if(!q.exec(s)){
//        QMessageBox::information(0, QObject::tr("提示信息"),
//                                 QString(QObject::tr("不能获取所有子科目哈希表")));
//        return false;
//    }
//    while(q.next())
//        codes[q.value(0).toInt()] = q.value(1).toString();
//    return true;
//}

///**
// * @brief BusiUtil::getReqDetSubs
// *  获取需要进行明细核算的id列表
// *  此函数应该为获取需要按币种进行核算的主目id列表，因为当前系统规定所有的科目都有二级科目
// * @param ids
// * @return
// */
//bool BusiUtil::getReqDetSubs(QList<int> &ids)
//{
//    QSqlQuery q;
//    QString s = QString("select id from %1 where %2 = 1")
//            .arg(tbl_fsub).arg(fld_fsub_isUseWb);
//    if(!q.exec(s)){
//        QMessageBox::information(0, QObject::tr("提示信息"),
//                                QString(QObject::tr("不能获取需要明细支持的一级科目id列表")));
//        return false;
//    }
//    while(q.next())
//        ids.append(q.value(0).toInt());
//    return true;
//}


//获取所有子目id到子目全名的哈希表
//bool BusiUtil::getAllSubSLName(QHash<int,QString>& names)
//    {
//        QSqlQuery q;
//        QString s;

//        s = QString("select %1.id,%2.%3 from %1 "
//                    "join %2 where %1.%4 = %2.id")
//                .arg(tbl_ssub).arg(tbl_nameItem).arg(fld_ni_lname).arg(fld_ssub_nid);
//        if(!q.exec(s)){
//            QMessageBox::information(0, QObject::tr("提示信息"),
//                                     QString(QObject::tr("不能获取所有子科目全名的哈希表")));
//            return false;
//        }
//        while(q.next())
//            names[q.value(0).toInt()] = q.value(1).toString();
//        return true;
//    }


/**
 * @brief BusiUtil::getAllFstSub
 *  获取所有一级科目列表，按科目代码的顺序
 * @param ids       科目代码
 * @param names     科目名称
 * @param isByView  是否只提取当前账户需要的科目
 * @return
 */
//bool BusiUtil::getAllFstSub(QList<int>& ids, QList<QString>& names, bool isByView)
//{
//    QString s;
//    QSqlQuery q;
//    if(isByView)
//        s = QString("select id,%1 from %2 where %3=1")
//                .arg(fld_fsub_name).arg(tbl_fsub).arg(fld_fsub_isview);
//    else
//        s = QString("select id,%1 from %2").arg(fld_fsub_name).arg(tbl_fsub);
//    s.append(QString(" order by %1").arg(fld_fsub_subcode));
//    if(!q.exec(s))
//        return false;
//    while(q.next()){
//        ids.append(q.value(0).toInt());
//        names.append(q.value(1).toString());
//    }
//    return true;
//}

/**
 * @brief BusiUtil::getAllSubCode
 *  获取所有总目id到总目代码的哈希表
 * @param codes
 * @param isByView
 * @return
 */
//bool BusiUtil::getAllSubCode(QHash<int, QString> &codes, bool isByView)
//{
//    QString s = QString("select id,%1 from %2").arg(fld_fsub_subcode).arg(tbl_fsub);
//    QSqlQuery q;
//    if(isByView)
//        s.append(QString(" where %1=1").arg(fld_fsub_isview));
//    if(!q.exec(s))
//        return false;
//    while(q.next())
//        codes[q.value(0).toInt()] = q.value(1).toString();
//    return true;
//}

/**
 * @brief BusiUtil::getSndSubInSpecFst
 *  获取所有指定总目下的子目（参数ids：，names：）
 * @param pid       主目id
 * @param ids       子目id
 * @param names     子目名
 * @param isAll     是否提取所有科目（true：所有科目（默认），false：只提取启用的科目）
 * @param subSys    科目系统代码
 * @return
 */
bool BusiUtil::getSndSubInSpecFst(int pid, QList<int>& ids, QList<QString>& names, bool isAll ,int subSys)
{
    QString s;
    QSqlQuery q(db);

    if(!ids.empty())
        ids.clear();
    if(!names.empty())
        names.clear();
    s = QString("select %1.id, %2.%3 from %1 "
                "join %2 where %1.%4 = %2.id "
                "and %1.%5 = %6")
            .arg(tbl_ssub).arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ssub_nid)
            .arg(fld_ssub_fid).arg(pid);
    if(!isAll)
        s.append(QString(" and %1.%2=1").arg(tbl_ssub).arg(fld_ssub_enable));
    if(!q.exec(s))
        return false;
    while(q.next()){
        ids.append(q.value(0).toInt());
        names.append(q.value(1).toString());
    }
    return true;
}

/**
 * @brief BusiUtil::getOwnerSub
 *  获取指定一级科目下的所有子科目（id到科目名的哈希表）
 * @param oid
 * @param names
 * @return
 */
//bool BusiUtil::getOwnerSub(int oid, QHash<int, QString> &names)
//{
//    QSqlQuery q;
//    QString s = QString("select %1.id,%2.%3 from %1 join %2 where %1.%4 = %2.id "
//                "and %1.%5 = %6").arg(tbl_ssub).arg(tbl_nameItem).arg(fld_ni_name)
//                .arg(fld_ssub_nid).arg(fld_ssub_fid).arg(oid);
//    if(!q.exec(s))
//        return false;
//    while(q.next())
//        names[q.value(0).toInt()] = q.value(1).toString();
//    return true;
//}



/**
 * @brief BusiUtil::getActionsInPz2
 *  获取指定凭证内的会计分录
 * @param pid
 * @param busiActions
 * @return
 */
//bool BusiUtil::getActionsInPz(int pid, QList<BusiActionData2 *> &busiActions)
//{
//    QString s;
//    QSqlQuery q;

//    if(busiActions.count() > 0){
//        qDeleteAll(busiActions);
//        busiActions.clear();
//    }

//    s = QString("select * from %1 where %2 = %3 order by %4")
//            .arg(tbl_ba).arg(fld_ba_pid).arg(pid).arg(fld_ba_number);
//    if(!q.exec(s))
//        return false;
//    while(q.next()){
//        BusiActionData2* ba = new BusiActionData2;
//        ba->id = q.value(0).toInt();
//        ba->pid = pid;
//        ba->summary = q.value(BACTION_SUMMARY).toString();
//        ba->fid = q.value(BACTION_FID).toInt();
//        ba->sid = q.value(BACTION_SID).toInt();
//        ba->mt  = q.value(BACTION_MTYPE).toInt();
//        ba->dir = q.value(BACTION_DIR).toInt();
//        //if(ba->dir == DIR_J)
//        ba->v = Double(q.value(BACTION_VALUE).toDouble());
//        //else
//        //    ba->v = Double(q.value(BACTION_DMONEY).toDouble());
//        ba->num = q.value(BACTION_NUMINPZ).toInt();
//        ba->state = BusiActionData2::INIT;
//        busiActions.append(ba);
//    }
//    return true;
//}

/**
 * @brief BusiUtil::getDefaultSndSubs
 *  获取所有一级科目下的默认子科目（使用频度最高的子科目）
 * @param defSubs
 * @return
 */
//bool BusiUtil::getDefaultSndSubs(QHash<int,int>& defSubs, int subSys)
//{
//    QString s;
//    QSqlQuery q1,q2;
//    int fs; //科目被使用的频度值或权重值
//    int fid,id;

//    s = QString("select id from %1 where %2=%3 and %4=1")
//            .arg(tbl_fsub).arg(fld_fsub_subSys).arg(subSys).arg(fld_fsub_isview);
//    if(!q1.exec())
//        return false;
//    while(q1.next()){
//        fid = q1.value(0).toInt();
//        s = QString("select id,%1 from %2 where %3=%4").arg(fld_ssub_weight)
//                .arg(tbl_ssub).arg(fld_ssub_fid).arg(fid);
//        if(!q2.exec(s))
//            return false;
//        fs=id=0;
//        //找出在某个一级科目下的最大使用频度值的明细科目id
//        while(q2.next()){
//            int tfs = q2.value(1).toInt();
//            if(fs < tfs){
//                fs = tfs;
//                id = q2.value(0).toInt();
//            }
//        }
//        defSubs[fid] = id;
//    }

//}

/**
 * @brief BusiUtil::getSidToFid
 *  获取子目id所属的总目id反向映射表
 * @param sidToFids
 * @return
 */
//bool BusiUtil::getSidToFid(QHash<int, int> &sidToFids, int subSys)
//{
//    QSqlQuery q;
//    QString s;

//    s = QString("select id from %1 where %2=%3").arg(tbl_fsub).arg(fld_fsub_subSys).arg(subSys);
//    if(!q.exec(s))
//        return false;
//    QList<int> fids;
//    while(q.next())
//        fids<<q.value(0).toInt();
//    s = QString("select id,%1 from %2").arg(fld_ssub_fid).arg(tbl_ssub);
//    if(!q.exec(s))
//        return false;
//    int fid,id;
//    while(q.next()){
//        fid = q.value(1).toInt();
//        if(fids.contains(fid)){
//            id = q.value(0).toInt();
//            sidToFids[id] = fid;
//        }
//    }
//    return true;
//}

//bool BusiUtil::saveActionsInPz2(int pid, QList<BusiActionData2 *> &busiActions, QList<BusiActionData2 *> dels)
//{
//    QString s;
//    QSqlQuery q1,q2,q3,q4;
//    bool r, hasNew = false;
//    QSqlDatabase db = QSqlDatabase::database();

//    if(!busiActions.isEmpty()){
//        if(!db.transaction())
//            return false;

//        s = QString("insert into BusiActions(pid,summary,firSubID,secSubID,"
//                    "moneyType,jMoney,dMoney,dir,NumInPz) values(:pid,:summary,"
//                    ":fid,:sid,:mt,:jv,:dv,:dir,:num)");
//        q1.prepare(s);
//        s = QString("update BusiActions set summary=:summary,firSubID=:fid,"
//                    "secSubID=:sid,moneyType=:mt,jMoney=:jv,dMoney=:dv,"
//                    "dir=:dir,NumInPz=:num where id=:id");
//        q2.prepare(s);
//        s = "update BusiActions set NumInPz=:num where id=:id";
//        q3.prepare(s);
//        for(int i = 0; i < busiActions.count(); ++i){
//            busiActions[i]->num = i + 1;  //在保存的同时，重新赋于顺序号
//            switch(busiActions[i]->state){
//            case BusiActionData2::INIT:
//                break;
//            case BusiActionData2::NEW:
//                hasNew = true;
//                q1.bindValue(":pid",busiActions[i]->pid);
//                q1.bindValue(":summary", busiActions[i]->summary);
//                q1.bindValue(":fid", busiActions[i]->fid);
//                q1.bindValue(":sid", busiActions[i]->sid);
//                q1.bindValue(":mt", busiActions[i]->mt);
//                if(busiActions[i]->dir == DIR_J){
//                    q1.bindValue(":jv", busiActions[i]->v.getv());
//                    q1.bindValue(":dv",0);
//                    q1.bindValue(":dir", DIR_J);
//                }
//                else{
//                    q1.bindValue(":jv",0);
//                    q1.bindValue(":dv", busiActions[i]->v.getv());
//                    q1.bindValue(":dir", DIR_D);
//                }
//                q1.bindValue(":num", busiActions[i]->num);
//                q1.exec();
//                break;
//            case BusiActionData2::EDITED:
//                q2.bindValue(":summary", busiActions[i]->summary);
//                q2.bindValue(":fid", busiActions[i]->fid);
//                q2.bindValue(":sid", busiActions[i]->sid);
//                q2.bindValue(":mt", busiActions[i]->mt);
//                if(busiActions[i]->dir == DIR_J){
//                    q2.bindValue(":jv", busiActions[i]->v.getv());
//                    q2.bindValue(":dv",0);
//                    q2.bindValue(":dir", DIR_J);
//                }
//                else{
//                    q2.bindValue(":jv",0);
//                    q2.bindValue(":dv", busiActions[i]->v.getv());
//                    q2.bindValue(":dir", DIR_D);
//                }
//                q2.bindValue(":num", busiActions[i]->num);
//                q2.bindValue("id", busiActions[i]->id);
//                q2.exec();
//                break;
//            case BusiActionData2::NUMCHANGED:
//                q3.bindValue(":num", busiActions[i]->num);
//                q3.bindValue(":id", busiActions[i]->id);
//                q3.exec();
//                break;
//            }
//            busiActions[i]->state = BusiActionData2::INIT;
//        }
//        if(!db.commit())
//            return false;
//        //回读新增的业务活动的id(待需要时再使用此代码)
//        if(hasNew){
//            r = getActionsInPz(pid,busiActions);
//        }
//    }
//    if(!dels.isEmpty()){
//        if(!db.transaction())
//            return false;
//        s = "delete from BusiActions where id=:id";
//        q4.prepare(s);
//        for(int i = 0; i < dels.count(); ++i){
//            q4.bindValue(":id", dels[i]->id);
//            q4.exec();
//        }
//        if(!db.commit())
//            return false;
//    }
//    return r;
//}

//删除与指定id的凭证相关的业务活动
//bool BusiUtil::delActionsInPz(int pzId)
//{
//    QString s;
//    QSqlQuery q;
//    s = QString("delete from BusiActions where pid = %1").arg(pzId);
//    bool r = q.exec(s);
//    int rows = q.numRowsAffected();
//    return r;
//}


//将各币种的余额汇总为用母币计的余额并确定余额方向
//参数 extra，extraDir：余额及其方向，键为币种代码，rate：汇率
//    mExtra，mDir：用母币计的余额值和方向
//bool BusiUtil::calExtraAndDir(QHash<int,double> extra,QHash<int,int> extraDir,
//                           QHash<int,double> rate,double& mExtra,int& mDir)
//{
//    mExtra = 0;
//    QHashIterator<int,double> i(extra);
//    while(i.hasNext()){ //计算期初总余额
//        i.next();
//        if(extraDir.value(i.key()) == DIR_P)
//            continue;
//        else if(extraDir.value(i.key()) == DIR_J)
//            mExtra += i.value() * rate.value(i.key());
//        else
//            mExtra -= i.value() * rate.value(i.key());
//    }
//    if(mExtra == 0)
//        mDir = DIR_P;
//    else if(mExtra > 0)
//        mDir = DIR_J;
//    else{
//        mDir = DIR_D;
//        mExtra = -mExtra;
//    }
//    return true;
//}

/**
 * @brief BusiUtil::calExtraAndDir2
 *  将各币种的余额汇总为用母币计的余额并确定余额方向
 * @param extra     余额，键为币种代码
 * @param extraDir  余额方向，键为币种代码
 * @param rate      汇率
 * @param mExtra    用母币计的余额值
 * @param mDir      余额方向
 * @return
 */\
bool BusiUtil::calExtraAndDir2(QHash<int, Double> extra, QHash<int, int> extraDir, QHash<int, Double> rate, Double &mExtra, int &mDir)
{
    mExtra = 0;
    QHashIterator<int,Double> i(extra);
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
        mExtra.changeSign();
    }
    return true;
}

//获取指定月份范围，指定科目（或科目范围）的日记账/明细账数据
//参数--y：年份，sm、em：开始和结束月份，fid、sid：一二级科目id，mt：币种
//     prev：期初总余额（各币种合计），preDir：期初余额方向
//     datas：保存数据,preExtra：前期余额，preExtraDir：前期余额方向
//     rates为每月的汇率，键为月份 * 10 + 币种代码（其中，期初余额是个位数，用币种代码表示）
//     fids：指定的总账科目代码，sids：所有在fids中指定的总账科目所属的明细科目代码总集合
//     gv和lv表示要提取的业务活动的值上下界限，inc：是否包含未记账凭证
//bool BusiUtil::getDailyAccount(int y, int sm, int em, int fid, int sid, int mt,
//                               double& prev, int& preDir,
//                               QList<DailyAccountData*>& datas,
//                               QHash<int,double>& preExtra,
//                               QHash<int,int>& preExtraDir,
//                               QHash<int, double>& rates,
//                               QList<int> fids,
//                               QHash<int,QList<int> > sids,
//                               double gv,double lv,bool inc)
//{
//    QSqlQuery q;
//    QString s;

//    //读取前期余额
//    int yy,mm;
//    if(sm == 1){
//        yy = y - 1;
//        mm = 12;
//    }
//    else{
//        yy = y;
//        mm = sm - 1;
//    }

//    QHash<int,double> ra;
//    QHashIterator<int,double>* it;
//    //获取期初汇率并加入汇率表
//    getRates(yy,mm,ra);
//    ra[RMB] = 1;
//    it = new QHashIterator<int,double>(ra);
//    while(it->hasNext()){
//        it->next();
//        rates[it->key()] = it->value();  //期初汇率的键直接是货币代码
//    }

//    //只有提取指定总账科目的明细发生项的情况下，读取余额才有意义
//    if(fid != 0){
//        //读取总账科目或明细科目的余额
//        if(sid == 0)
//            readExtraForSub(yy,mm,fid,preExtra,preExtraDir);
//        else
//            readExtraForDetSub(yy,mm,sid,preExtra,preExtraDir);
//        //如果还指定了币种，则只保留该币种的余额
//        if(mt != ALLMT){
//            QHashIterator<int,double> it(preExtra);
//            while(it.hasNext()){
//                it.next();
//                if(mt != it.key()){
//                    preExtra.remove(it.key());
//                    preExtraDir.remove(it.key());
//                }
//            }
//        }
//    }


//    //保存每次发生业务活动后的各币种余额，初值就是前期余额
//    QHash<int,double> esums = preExtra;
//    double tsums = 0;  //保存按母币计的期初总余额及其每次发生后的总余额（将各币种用母币合计）
//    QHashIterator<int,double> i(preExtra);
//    while(i.hasNext()){ //计算期初总余额
//        i.next();
//        if(preExtraDir.value(i.key()) == DIR_P)
//            continue;
//        else if(preExtraDir.value(i.key()) == DIR_J)
//            tsums += (i.value() * ra.value(i.key()));
//        else
//            tsums -= (i.value() * ra.value(i.key()));
//    }
//    if(tsums == 0){
//        prev = 0; preDir = DIR_P;
//    }
//    else if(tsums > 0){
//        prev = tsums; preDir = DIR_J;
//    }
//    else{
//        prev = -tsums; preDir = DIR_D;
//    }

//    //获取指定月份范围的汇率
//    ra.clear();
//    for(int i = sm; i <= em; ++i){
//        getRates(y,i,ra);
//        ra[RMB] = 1;
//        it = new QHashIterator<int,double>(ra);
//        while(it->hasNext()){
//            it->next();
//            rates[i*10+it->key()] = it->value();
//        }
//        ra.clear();
//    }


//    //构造查询语句
//    QString sd = QDate(y,sm,1).toString(Qt::ISODate);
//    QString ed = QDate(y,em,QDate(y,em,1).daysInMonth()).toString(Qt::ISODate);

//    s = QString("select PingZhengs.date,PingZhengs.number,BusiActions.summary,"
//                "BusiActions.id,BusiActions.pid,BusiActions.jMoney,BusiActions.dMoney,"
//                "BusiActions.moneyType,BusiActions.dir,BusiActions.firSubID,BusiActions.secSubID "
//                "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
//                "where (PingZhengs.date >= '%1') and (PingZhengs.date <= '%2')")
//                .arg(sd).arg(ed);
//    if(fid != 0)
//        s.append(QString(" and (BusiActions.firSubID = %1)").arg(fid));
//    if(sid != 0)
//        s.append(QString(" and (BusiActions.secSubID = %1)").arg(sid));
//    if(mt != ALLMT)
//        s.append(QString(" and (BusiActions.moneyType = %1)").arg(mt));
//    if(gv != 0)
//        s.append(QString(" and (((BusiActions.dir = %1) and (BusiActions.jMoney > %3)) "
//                         "or ((BusiActions.dir = %2) and (BusiActions.dMoney > %3)))")
//                 .arg(DIR_J).arg(DIR_D).arg(gv));
//    if(lv != 0)
//        s.append(QString(" and (((BusiActions.dir = %1) and (BusiActions.jMoney < %3)) "
//                         "or ((BusiActions.dir = %2) and (BusiActions.dMoney < %3)))")
//                 .arg(DIR_J).arg(DIR_D).arg(lv));
//    if(!inc)
//        s.append(QString(" and (PingZhengs.pzState = %1)").arg(Pzs_Instat));
//    s.append(" order by PingZhengs.date");

//    if(!q.exec(s))
//        return false;

//    //if(!datas.empty())
//    //    datas.clear();

//    while(q.next()){
//        int id = q.value(9).toInt();
//        //如果要提取所有选定主目，则过滤掉所有未选定主目
//        if(fid == 0){
//            if(!fids.contains(id))
//                continue;
//        }
//        //如果选择所有子目，则过滤掉所有未选定子目
//        if(sid == 0){
//            int iid = q.value(10).toInt();
//            if(!sids.value(id).contains(iid))
//                continue;
//        }
//        DailyAccountData* item = new DailyAccountData;
//        //凭证日期
//        QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
//        item->y = d.year();
//        item->m = d.month();
//        item->d = d.day();
//        //凭证号
//        int num = q.value(1).toInt(); //凭证号
//        item->pzNum = QObject::tr("计%1").arg(num);
//        //凭证摘要
//        QString summary = q.value(2).toString();
//        //如果是现金、银行科目
//        //结算号
//        //item->jsNum =
//        //对方科目
//        //item->oppoSub =
//        int idx = summary.indexOf("<");
//        if(idx != -1){
//            summary = summary.left(idx);
//        }
//        item->summary = summary;
//        //借、贷方金额
//        item->mt = q.value(7).toInt();  //业务发生的币种
//        item->dh = q.value(8).toInt();  //业务发生的借贷方向
//        if(item->dh == DIR_J)
//            item->v = q.value(5).toDouble(); //发生在借方
//        else
//            item->v = q.value(6).toDouble(); //发生在贷方

//        //余额
//        if(item->dh == DIR_J){
//            tsums += (item->v * rates.value(item->m*10+item->mt));
//            esums[item->mt] += item->v;
//        }
//        else{
//            tsums -= (item->v * rates.value(item->m*10+item->mt));
//            esums[item->mt] -= item->v;
//        }

//        //保存分币种的余额及其方向
//        //item->em = esums;  //分币种余额
//        it = new QHashIterator<int,double>(esums);
//        while(it->hasNext()){
//            it->next();
//            if(it->value()>0){
//                item->em[it->key()] = it->value();
//                item->dirs[it->key()] = DIR_J;
//            }
//            else if(it->value() < 0){
//                item->em[it->key()] = -it->value();
//                item->dirs[it->key()] = DIR_D;
//            }
//            else{
//                item->em[it->key()] = 0;
//                item->dirs[it->key()] = DIR_P;
//            }
//        }

//        //确定总余额的方向和值（余额值始终用正数表示，而在计算时，借方用正数，贷方用负数）
//        if(tsums > 0){
//            item->etm = tsums; //各币种合计余额
//            item->dir = DIR_J;
//        }
//        else if(tsums < 0){
//            item->etm = -tsums;
//            item->dir = DIR_D;
//        }
//        else{
//            item->etm = 0;
//            item->dir = DIR_P;
//        }
//        item->pid = q.value(4).toInt();
//        item->bid = q.value(3).toInt(); //这个？？测试目的
//        datas.append(item);
//    }
//    return true;
//}

/**
获取指定月份范围，指定科目（或科目范围）的日记账/明细账数据
参数--y：年份，sm、em：开始和结束月份，fid、sid：一二级科目id，mt：币种
     prev：期初总余额（各币种按本币合计），preDir：期初余额方向
     datas：日记账数据,
     preExtra：原币形式的前期余额，preExtraR：本币形式的前期外币余额
     preExtraDir：前期余额方向
     rates为每月的汇率，键为月份 * 10 + 币种代码（其中，期初余额是个位数，用币种代码表示）
     fids：指定的总账科目代码，
     sids：所有在fids中指定的总账科目所属的明细科目代码总集合
     gv和lv表示要提取的业务活动的值上下界限，inc：是否包含未记账凭证
*/
//bool BusiUtil::getDailyAccount2(int y, int sm, int em, int fid, int sid, int mt,
//                                Double &prev, int &preDir,
//                                QList<DailyAccountData2 *> &datas,
//                                QHash<int, Double> &preExtra,
//                                QHash<int,Double>& preExtraR,
//                                QHash<int, int> &preExtraDir,
//                                QHash<int, Double> &rates,
//                                QList<int> fids, QHash<int, QList<int> > sids,
//                                Double gv, Double lv, bool inc)
//{
//    QSqlQuery q(db);
//    QString s;

//    //读取前期余额
//    int yy,mm;
//    if(sm == 1){
//        yy = y - 1;
//        mm = 12;
//    }
//    else{
//        yy = y;
//        mm = sm - 1;
//    }

//    QHash<int,Double> ra;
//    QHashIterator<int,Double>* it;

//    //获取期初汇率并加入汇率表
//    if(!getRates2(yy,mm,ra))
//        return false;
//    ra[RMB] = 1.00;
//    it = new QHashIterator<int,Double>(ra);
//    while(it->hasNext()){
//        it->next();
//        rates[it->key()] = it->value();  //期初汇率的键直接是货币代码
//    }

//    //只有提取指定总账科目的明细发生项的情况下，读取余额才有意义
//    if(fid != 0){
//        //读取总账科目或明细科目的余额
//        if(sid == 0)
//            readExtraForSub2(yy,mm,fid,preExtra,preExtraR,preExtraDir);
//        else
//            readExtraForDetSub2(yy,mm,sid,preExtra,preExtraR,preExtraDir);
//        //如果还指定了币种，则只保留该币种的余额
//        if(mt != ALLMT){
//            QHashIterator<int,Double> it(preExtra);
//            while(it.hasNext()){
//                it.next();
//                if(mt != it.key()){
//                    preExtra.remove(it.key());
//                    preExtraDir.remove(it.key());
//                }
//            }
//        }
//    }


//    //以原币形式保存每次发生业务活动后的各币种余额，初值就是前期余额（还要根据前期余额的方向调整符号）
//    QHash<int,Double> esums;
//    it = new QHashIterator<int,Double>(preExtra);
//    while(it->hasNext()){
//        it->next();
//        Double v = it->value();
//        if(preExtraDir.value(it->key()) == DIR_D)
//            v.changeSign();
//        esums[it->key()] = v;
//    }

//    //计算期初总余额
//    Double tsums = 0.00;  //保存按母币计的期初总余额及其每次发生后的总余额（将各币种用母币合计）
//    QHashIterator<int,Double> i(preExtra);
//    while(i.hasNext()){ //计算期初总余额
//        i.next();
//        int mt = i.key() % 10;
//        if(preExtraDir.value(i.key()) == DIR_P)
//            continue;
//        else if(preExtraDir.value(i.key()) == DIR_J){
//            //tsums += (i.value() * ra.value(i.key()));
//            if(mt == RMB)
//                tsums += i.value();
//            else
//                tsums += preExtraR.value(it->key());
//        }
//        else{
//            //tsums -= (i.value() * ra.value(i.key()));
//            if(mt == RMB)
//                tsums -= i.value();
//            else
//                tsums -= preExtraR.value(it->key());
//        }
//    }
//    if(tsums == 0){
//        prev = 0.00; preDir = DIR_P;
//    }
//    else if(tsums > 0){
//        prev = tsums; preDir = DIR_J;
//    }
//    else{
//        prev = tsums;
//        prev.changeSign();
//        preDir = DIR_D;
//    }

//    //获取指定月份范围的汇率
//    ra.clear();
//    for(int i = sm; i <= em; ++i){
//        getRates2(y,i,ra);
//        ra[RMB] = 1.00;
//        it = new QHashIterator<int,Double>(ra);
//        while(it->hasNext()){
//            it->next();
//            rates[i*10+it->key()] = it->value();
//        }
//        ra.clear();
//    }


//    //构造查询语句
//    QString sd = QDate(y,sm,1).toString(Qt::ISODate);
//    QString ed = QDate(y,em,QDate(y,em,1).daysInMonth()).toString(Qt::ISODate);

////    s = QString("select PingZhengs.date,PingZhengs.number,BusiActions.summary,"
////                "BusiActions.id,BusiActions.pid,BusiActions.jMoney,"
////                "BusiActions.moneyType,BusiActions.dir,BusiActions.firSubID,"
////                "BusiActions.secSubID,PingZhengs.isForward "
////                "from PingZhengs join BusiActions on BusiActions.pid = PingZhengs.id "
////                "where (PingZhengs.date >= '%1') and (PingZhengs.date <= '%2')")
////                .arg(sd).arg(ed);
//    //date,number,summary,bid,pid,value,mt,dir,fid,sid,pzClass
//    s = QString("select %1.%2,%1.%3,%4.%5,"
//                "%4.id,%4.%6,%4.%7,%4.%8,"
//                "%4.%9,%4.%10,"
//                "%4.%11,%1.%12 "
//                "from %1 join %4 on %4.%6 = %1.id "
//                "where (%1.%2 >= '%13') and (%1.%2 <= '%14')")
//            .arg(tbl_pz).arg(fld_pz_date).arg(fld_pz_number).arg(tbl_ba).arg(fld_ba_summary)
//            .arg(fld_ba_pid).arg(fld_ba_value).arg(fld_ba_mt).arg(fld_ba_dir).arg(fld_ba_fid)
//            .arg(fld_ba_sid).arg(fld_pz_class).arg(sd).arg(ed);
//    if(fid != 0)
//        //s.append(QString(" and (BusiActions.firSubID = %1)").arg(fid));
//        s.append(QString(" and (%1.%2 = %3)").arg(tbl_ba).arg(fld_ba_fid).arg(fid));
//    if(sid != 0)
//        //s.append(QString(" and (BusiActions.secSubID = %1)").arg(sid));
//        s.append(QString(" and (%1.%2 = %3)").arg(tbl_ba).arg(fld_ba_sid).arg(sid));
//    //if(mt != ALLMT)
//    //    s.append(QString(" and (BusiActions.moneyType = %1)").arg(mt));
//    if(gv != 0)
////        s.append(QString(" and (((BusiActions.dir = %1) and (BusiActions.jMoney > %3)) "
////                         "or ((BusiActions.dir = %2) and (BusiActions.dMoney > %3)))")
////                 .arg(DIR_J).arg(DIR_D).arg(gv.toString()));
//        s.append(QString(" and (%1.%2 > %3)")
//                 .arg(tbl_ba).arg(fld_ba_value).arg(gv.toString()));
//    if(lv != 0)
////        s.append(QString(" and (((BusiActions.dir = %1) and (BusiActions.jMoney < %3)) "
////                         "or ((BusiActions.dir = %2) and (BusiActions.dMoney < %3)))")
////                 .arg(DIR_J).arg(DIR_D).arg(lv.toString()));
//        s.append(QString(" and (%1.%2 < %3)")
//                 .arg(tbl_ba).arg(fld_ba_value).arg(lv.toString()));
//    if(!inc) //将已入账的凭证纳入统计范围
//        //s.append(QString(" and (PingZhengs.pzState = %1)").arg(Pzs_Instat));
//        s.append(QString(" and (%1.%2 = %3)").arg(tbl_pz).arg(fld_pz_state).arg(Pzs_Instat));
//    else     //将未审核、已审核、已入账的凭证纳入统计范围
//        //s.append(QString(" and (PingZhengs.pzState != %1)").arg(Pzs_Repeal));
//        s.append(QString(" and (%1.%2 != %3)").arg(tbl_pz).arg(fld_pz_state).arg(Pzs_Repeal));
//    //s.append(" order by PingZhengs.date");
//    s.append(QString(" order by %1.%2").arg(tbl_pz).arg(fld_pz_number));

//    if(!q.exec(s))
//        return false;

//    int mType,id,fsubId;
//    PzClass pzCls;
//    int cwfyId; //财务费用的科目id
//    getIdByCode(cwfyId,"5503");

//    while(q.next()){
//        id = q.value(8).toInt();  //fid
//        mType = q.value(6).toInt();  //业务发生的币种
//        pzCls = (PzClass)q.value(10).toInt();    //凭证类别
//        fsubId = q.value(8).toInt();    //会计分录所涉及的一级科目id
//        //如果要提取所有选定主目，则跳过所有未选定主目
//        if(fid == 0){
//            if(!fids.contains(id))
//                continue;
//        }
//        //如果选择所有子目，则过滤掉所有未选定子目
//        if(sid == 0){
//            int iid = q.value(9).toInt();
//            if(!sids.value(id).contains(iid))
//                continue;
//        }

//        //当前凭证是否是结转汇兑损益的凭证
//        bool isJzhdPz = pzClsJzhds.contains(pzCls)/*Pzc_Jzhd_Bank || pzCls == Pzc_Jzhd_Ys
//                        || pzCls == Pzc_Jzhd_Yf*/;
//        //如果是结转汇兑损益的凭证作特别处理
//        if(isJzhdPz){
//            if(mt == RMB && fid != cwfyId) //如果指定的是人民币，则跳过非财务费用方的会计分录
//                continue;
//        }

//        //对于非结转汇兑损益的凭证，如果指定了币种，则跳过非此币种的会计分录
//        if((mt != 0 && mt != mType && !isJzhdPz))
//            continue;

//        DailyAccountData2* item = new DailyAccountData2;
//        //凭证日期
//        QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
//        item->y = d.year();
//        item->m = d.month();
//        item->d = d.day();
//        //凭证号
//        int num = q.value(1).toInt(); //凭证号
//        item->pzNum = QObject::tr("计%1").arg(num);
//        //凭证摘要
//        QString summary = q.value(2).toString();
//        //如果是现金、银行科目
//        //结算号
//        //item->jsNum =
//        //对方科目
//        //item->oppoSub =
//        int idx = summary.indexOf("<");
//        if(idx != -1){
//            summary = summary.left(idx);
//        }
//        item->summary = summary;
//        //借、贷方金额
//        item->mt = q.value(6).toInt();  //业务发生的币种
//        item->dh = q.value(7).toInt();  //业务发生的借贷方向
//        item->v = Double(q.value(5).toDouble());
////        if(item->dh == DIR_J)
////            item->v = Double(q.value(5).toDouble()); //发生在借方
////        else
////            item->v = Double(q.value(6).toDouble()); //发生在贷方

//        //余额
//        if(item->dh == DIR_J){
//            tsums += (item->v * rates.value(item->m*10+item->mt));
//            esums[item->mt] += item->v;
//        }
//        else{
//            tsums -= (item->v * rates.value(item->m*10+item->mt));
//            esums[item->mt] -= item->v;
//        }

//        //保存分币种的余额及其方向
//        //item->em = esums;  //分币种余额
//        it = new QHashIterator<int,Double>(esums);
//        while(it->hasNext()){
//            it->next();
//            if(it->value()>0){
//                item->em[it->key()] = it->value();
//                item->dirs[it->key()] = DIR_J;
//            }
//            else if(it->value() < 0.00){
//                item->em[it->key()] = it->value();
//                item->em[it->key()].changeSign();
//                item->dirs[it->key()] = DIR_D;
//            }
//            else{
//                item->em[it->key()] = 0;
//                item->dirs[it->key()] = DIR_P;
//            }
//        }

//        //确定总余额的方向和值（余额值始终用正数表示，而在计算时，借方用正数，贷方用负数）
//        if(tsums > 0){
//            item->etm = tsums; //各币种合计余额
//            item->dir = DIR_J;
//        }
//        else if(tsums < 0){
//            item->etm = tsums;
//            item->etm.changeSign();
//            item->dir = DIR_D;
//        }
//        else{
//            item->etm = 0;
//            item->dir = DIR_P;
//        }
//        item->pid = q.value(4).toInt();
//        item->bid = q.value(3).toInt(); //这个？？测试目的
//        datas.append(item);
//    }
//    return true;
//}


//获取指定月份范围，指定总账科目的总账数据
//参数 y：帐套年份，sm，em：开始和结束月份，fid：总账科目id，datas：读取的数据
//    preExtra：期初余额，preExtraDir：期初余额方向，
//rate：每月的汇率，键为月份 * 10 + 币种代码（期初汇率的月份代码为0，因此它的键为币种代码-个位数）
//注意：返回的数据列表的第一条记录是上年或上月结转的余额
bool BusiUtil::getTotalAccount(int y, int sm, int em, int fid,
                            QList<TotalAccountData2*>& datas,
                            QHash<int,Double>& preExtra,
                            QHash<int,int>& preExtraDir,
                            QHash<int, Double>& rates)
{
    QSqlQuery q(db);
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

    QHash<int,Double> ra,preRa; //期初汇率
    QHashIterator<int,Double>* it;

    //获取期初汇率
    getRates2(yy,mm,preRa);
    preRa[RMB] = 1;
    it = new QHashIterator<int,Double>(preRa);
    while(it->hasNext()){
        it->next();
        rates[it->key()] = it->value();
    }
    //获取指定月份范围的汇率
    for(int i = sm; i <= em; ++i){
        getRates2(y,i,ra);
        ra[RMB] = 1;
        it = new QHashIterator<int,Double>(ra);
        while(it->hasNext()){
            it->next();
            rates[i*10+it->key()] = it->value();
        }
        ra.clear();
    }

    //读取总账科目余额
    QHash<int,Double> wv;
    readExtraForSub2(yy,mm,fid,preExtra,wv,preExtraDir);
    Double extra;  //期初余额（用母币计），兼做每月期末余额
    int edir;      //前期余额方向（用母币计），兼做每月期末余额方向
    QHash<int,Double> extras; //期初余额（依币种分开计），兼做每月期末余额（这个哈希表用正数表示借方，负数表示贷方）
    //计算用母币计的期初余额与方向
    calExtraAndDir2(preExtra,preExtraDir,preRa,extra,edir);

    //调整数值，正数表示借方，负数表示贷方
    it = new QHashIterator<int,Double>(preExtra);
    while(it->hasNext()){
        it->next();
        if(preExtraDir.value(it->key()) == DIR_D) //如果余额是贷方，则用负数表示，以便后续累加
            extras[it->key()].changeSign();
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

    TotalAccountData2* item;

    //这个项目对应于上年结转或上月结转
    item = new TotalAccountData2;
    item->y = yy;
    item->m = mm;
    item->evh = preExtra;
    item->ev = extra;
    item->dir = edir;
    datas<<item;

    int om = 0,cm; //前期和当期月份
    QString date;
    int dir,mt;    //业务活动发生的方向和币种
    Double sumjm = 0.0,sumdm = 0.0;    //本月借贷合计（用母币计）
    QHash<int,Double> sumjmh,sumdmh; //本月借贷合计
    Double v;
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
            it = new QHashIterator<int,Double>(extras);
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
                    item->evh[it->key()] = it->value();
                    item->evh[it->key()].changeSign();
                    item->dirs[it->key()] = DIR_D;
                }
            }
            //用母币计的余额及其方向
            if(extra == 0){
                item->ev = 0;
                item->dir = DIR_P;
            }
            if(extra < 0){
                item->ev = extra;
                item->ev.changeSign();
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
            item = new TotalAccountData2;
            om = cm;
        }
        dir = q.value(4).toInt();
        mt = q.value(3).toInt();        
        if(dir == DIR_J){
            v = Double(q.value(1).toDouble());
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
    it = new QHashIterator<int,Double>(extras);
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
            item->evh[it->key()] = it->value();
            item->evh[it->key()].changeSign();
            item->dirs[it->key()] = DIR_D;
        }
    }
    //用母币计的余额及其方向
    if(extra == 0){
        item->ev = 0;
        item->dir = DIR_P;
    }
    if(extra < 0){
        item->ev = extra;
        item->ev.changeSign();
        item->dir = DIR_D;
    }
    else{
        item->ev = extra;
        item->dir = DIR_J;
    }
    datas<<item;
    return true;
}

//bool BusiUtil::genPzPrintDatas2(int y, int m, QList<PzPrintData2 *> &datas, QSet<int> pznSet)
//{
//    QSqlQuery q,q2;
//    QString s;

//    //获取所有一级、二级科目的名称表和用户名称表
//    QHash<int,QString> fsub,ssub,slsub,users;
//    getAllSubFName(fsub);    //一级科目名
//    getAllSubSLName(slsub); //二级科目全名
//    getAllSubSName(ssub);   //二级科目简名
//    //getAllUser(users);      //用户名
//    QHash<int,Double> rates;
//    getRates2(y,m,rates);    //汇率
//    rates[RMB] = 1.00;

//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    s = QString("select * from PingZhengs where date like '%1%' "
//                "order by number").arg(ds);
//    bool r = q.exec(s);
//    while(q.next()){
//        int pzNum = q.value(PZ_NUMBER).toInt();   //凭证号
//        if((pznSet.count() == 0) || pznSet.contains(pzNum)){
//            //获取该凭证的业务活动数
//            int pid = q.value(0).toInt();
//            s = QString("select count() from BusiActions where pid = %1").arg(pid);
//            if(!q2.exec(s))
//                return false;
//            q2.first();
//            int bac = q2.value(0).toInt(); //该凭证包含的业务活动数
//            //计算将此凭证完全打印出来需要几张凭证
//            int pzc;
//            if((bac % MAXROWS) == 0)
//                pzc = bac / MAXROWS;
//            else
//                pzc = bac / MAXROWS + 1;

//            //获取业务活动
//            s = QString("select * from BusiActions where pid = %1").arg(pid);
//            if(!q2.exec(s))
//                return false;

//            Double jsum = 0.00,dsum = 0.00; //借贷合计值
//            for(int i = 0; i < pzc; ++i){
//                PzPrintData2* pd = new PzPrintData2;
//                pd->date = q.value(PZ_DATE).toDate();     //凭证日期
//                pd->attNums = q.value(PZ_ENCNUM).toInt(); //附件数
//                if(pzc == 1)
//                    pd->pzNum = QString::number(pzNum);
//                else{
//                    pd->pzNum = QString::number(pzNum) + '-' + QString("%1/%2").arg(i+1).arg(pzc);
//                }
//                QList<BaData2*> baList;
//                int num = 0; //已提取的业务活动数

//                while((num < MAXROWS) && (q2.next())){
//                    num++;
//                    BaData2* bd = new BaData2;
//                    QString summary = q2.value(BACTION_SUMMARY).toString();
//                    int idx = summary.indexOf('<');
//                    if(idx != -1)
//                        summary.chop(summary.count()- idx);
//                    bd->summary = summary;
//                    bd->dir = q2.value(BACTION_DIR).toInt();
//                    bd->mt = q2.value(BACTION_MTYPE).toInt();
//                    if(bd->dir == DIR_J){
//                        bd->v = Double(q2.value(BACTION_JMONEY).toDouble());
//                        jsum += bd->v * rates.value(bd->mt);
//                    }
//                    else{
//                        bd->v = Double(q2.value(BACTION_DMONEY).toDouble());
//                        dsum += bd->v * rates.value(bd->mt);
//                    }
//                    int fid = q2.value(BACTION_FID).toInt();
//                    int sid = q2.value(BACTION_SID).toInt();
//                    if(slsub.value(sid) == "") //如果二级科目没有全名，则用简名代替
//                        bd->subject = fsub.value(fid) + QObject::tr("——") + ssub.value(sid);
//                    else
//                        bd->subject = fsub.value(fid) + QObject::tr("——") + slsub.value(sid);
//                    baList.append(bd);
//                }
//                pd->baLst = baList;
//                pd->jsum = jsum;
//                pd->dsum = dsum;
//                pd->producer = q.value(PZ_RUSER).toInt();    //制单者
//                pd->verify = q.value(PZ_VUSER).toInt();      //审核者
//                pd->bookKeeper = q.value(PZ_BUSER).toInt();  //记账者
//                //pd->producer = allUsers.value(q2.value(PZ_RUSER).toInt())->getName();  //制单者
//                //pd->verify = allUsers.value(q2.value(PZ_VUSER).toInt())->getName();    //审核者
//                //pd->bookKeeper = allUsers.value(q2.value(PZ_BUSER).toInt())->getName();//记账者
//                datas.append(pd);
//            }
//        }
//    }
//    return true;
//}

/**
    结转损益（将损益类科目结转至本年利润）
*/
//bool BusiUtil::genForwordPl2(int y, int m, User *user)
//{
//    //直接从科目余额表中读取损益类科目的余额数进行结转，因此在执行此操作之前必须在打开凭证集后，
//    //并且处于结转损益前的准备状态
//    QSqlQuery q;
//    QString s;
//    bool r = 0;

//    //因为在主窗口调用此函数时已经检测了凭证集的状态，因此这里忽略凭证集状态检测
//    //结转损益类科目，不会涉及到外币，因此用不着使用汇率
////    QHash<int,Double>rates;
////    if(!getRates2(y,m,rates)){
////        qDebug() << QObject::tr("不能获取%1年%2月的汇率").arg(y).arg(m);
////        return false;
////    }
////    rates[RMB] = Double(1.00);

//    //基本步骤：
//    //1、读取科目余额
//    QHash<int,Double>extra,extraDet; //总账科目和明细账科目余额
//    QHash<int,MoneyDirection>extraDir,extraDetDir; //总账科目和明细账科目余额方向
//    if(!readExtraByMonth2(y,m,extra,extraDir,extraDet,extraDetDir)){
//        qDebug() << "Don't read subject extra !";
//        return false;
//    }

//    //查看财务费用-汇兑损益的余额
//    qDebug()<<QObject::tr("财务费用-汇兑损益的余额： %1").arg(extraDet.value(601).toString());

//    //2、创建结转凭证。
//    //在创建结转凭证前要检测是否以前已经进行过结转，如有则要先删除与此两凭证相关的所有业务活动
//    //并且重用凭证表的对应记录
//    int idNewIn;    //结转收入的凭证id
//    int idNewFei;   //结转费用的凭证id
//    int pzNum;  //结转凭证所使用的凭证号（结转收入和结转费用两张凭证号是紧挨的，且收入在前费用在后）

//    QDate pzDate(y,m,1); //凭证日期为当前打开凭证集的最后日期
//    pzDate.setDate(y,m,pzDate.daysInMonth());
//    QString strPzDate = pzDate.toString(Qt::ISODate);

//    //创建新的或读取现存的结转收入类凭证id值
//    s = QString("select id,number from PingZhengs where (isForward = %1) and "
//                "(date='%2')").arg(Pzc_JzsyIn).arg(strPzDate);
//    if(q.exec(s) && q.first()){
//        idNewIn = q.value(0).toInt();
//        pzNum = q.value(1).toInt();
//        s = QString("delete from BusiActions where pid = %1").arg(idNewIn);
//        r = q.exec(s); //删除原先属于此凭证的所有业务活动
//        //恢复凭证状态到初始录入态
//        s = QString("update PingZhengs set pzState=%1,ruid=%2 where id = %3")
//                .arg(Pzs_Recording).arg(user->getUserId()).arg(idNewIn);
//        r = q.exec(s);
//    }
//    else{
//        //创建新凭证之前，先获取凭证集内可用的最大凭证号
//        pzNum = getMaxPzNum(y,m);
//        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
//                    "values('%1',%2,%3,%4,%5)").arg(strPzDate).arg(pzNum)
//                .arg(Pzc_JzsyIn).arg(Pzs_Recording).arg(user->getUserId());
//        r = q.exec(s);
//        //回读此凭证的id
//        s = QString("select id from PingZhengs where (isForward = %1) and "
//                    "(date='%2')").arg(Pzc_JzsyIn).arg(strPzDate);
//        r = q.exec(s); r = q.first();
//        idNewIn = q.value(0).toInt();
//    }

//    //创建新的或读取现存的结转费用类凭证id值
//    s = QString("select id,number from PingZhengs where (isForward = %1) and "
//                "(date='%2')").arg(Pzc_JzsyFei).arg(strPzDate);
//    pzNum++;
//    if(q.exec(s) && q.first()){
//        idNewFei = q.value(0).toInt();
//        s = QString("delete from BusiActions where pid = %1").arg(idNewFei);
//        r = q.exec(s); //删除原先属于此凭证的所有业务活动
//        //恢复凭证状态到初始录入态
//        s = QString("update PingZhengs set pzState=%1,ruid=%2 where id = %3")
//                .arg(Pzs_Recording).arg(user->getUserId()).arg(idNewFei);
//        r = q.exec(s);
//    }
//    else{
//        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
//                    "values('%1',%2,%3,%4,%5)").arg(strPzDate).arg(pzNum)
//                .arg(Pzc_JzsyFei).arg(Pzs_Recording).arg(user->getUserId());
//        r = q.exec(s);
//        //回读此凭证的id
//        s = QString("select id from PingZhengs where (isForward = %1) and "
//                    "(date='%2')").arg(Pzc_JzsyFei).arg(strPzDate);
//        r = q.exec(s); r = q.first();
//        idNewFei = q.value(0).toInt();
//    }

//    //3、填充结转凭证的业务活动数据
//    //获取“本年利润-结转”子账户-的id
//    int bnlrId;
//    if(!getIdByName(bnlrId, QObject::tr("本年利润"))){
//        qDebug() << QObject::tr("不能获取本年利润科目的id值");
//        return false;
//    }
//    s = QString("select %1.id from %1 join %2 "
//                "where (%1.%3 = %2.id) and "
//                "(%1.%4 = %5) and (%2.%6 = '%7')")
//            .arg(tbl_ssub).arg(tbl_nameItem).arg(fld_ssub_nid).arg(fld_ssub_fid)
//            .arg(bnlrId).arg(fld_ni_name).arg(QObject::tr("结转"));
//    if(!q.exec(s) || !q.first()){
//        qDebug() << QObject::tr("不能获取”本年利润-结转“子账户");
//        return false;
//    }
//    int bnlrSid = q.value(0).toInt();  //本年利润--结转子账户id

//    QList<BusiActionData2*> fad,iad;      //费用和收入结转凭证的业务活动数据列表
//    QHash<int, QList<int> > fids,iids;   //费用类和收入类的明细科目id集合，键为一级科目id，值为该一级科目下的子目id集合
//    BusiUtil::getAllIdForSy(true,iids);  //获取所有收入类总账-明细id列表
//    BusiUtil::getAllIdForSy(false,fids); //获取所有费用类总账-明细id列表

//    QList<int> fidLst = fids.keys();     //费用类一级科目id的列表（按id的大小顺序排列，应该是以科目代码顺序排列，
//    qSort(fidLst.begin(),fidLst.end());  //这里简略实现，使用此列表只是为了生成的业务活动按预定的顺序）
//    QList<int> iidLst = iids.keys();     //收入类一级科目id的列表（按id的大小顺序排列，应该是以科目代码顺序排列）
//    qSort(iidLst.begin(),iidLst.end());
//    QHash<int,QString> mtHash;
//    getMTName(mtHash);
//    QList<int> mts = mtHash.keys();      //所有币种代码列表

//    int sid,key;  //二级科目id、查询余额的键
//    BusiActionData2 *bd1;
//    int num = 1; //业务活动在凭证内的序号

//    Double iv, fv; //结转凭证的借贷合计值
//    SubjectManager* sm = curAccount->getSubjectManager(/*subSys*/); //这个要修改

//    //结转收入类凭证的业务活动列表（这里的最内层循环也可以省略）
//    for(int i = 0; i < iidLst.count(); ++i)
//        for(int j = 0; j < iids.value(iidLst[i]).count(); ++j)
//            for(int k = 0; k < mts.count(); ++k){
//                sid = iids.value(iidLst[i])[j];
//                key = sid * 10 + mts[k];
//                if(extraDet.contains(key) && !extraDet[key].isZero()){
//                    bd1 = new BusiActionData2;
//                    bd1->state = BusiActionData2::NEW; //新业务活动
//                    bd1->num = num++;
//                    bd1->pid = idNewIn;
//                    bd1->fid = iidLst[i];
//                    bd1->sid = sid;
//                    bd1->summary = QObject::tr("结转（%1-%2）至本年利润")
//                            .arg(sm->getFstSubject(iidLst.at(i))->getName())  //  ( sm->getFstSubName(iidLst[i]))
//                            .arg(sm->getSndSubject(sid)->getName());   //arg(sm->getSndSubName(sid));
//                    bd1->mt = mts[k];
//                    //结转收入类到本年利润，一般是损益类科目放在借方，本年利润放在贷方
//                    //而且，损益类中收入类科目余额约定是贷方，故不做不做方向检测
//                    bd1->dir = DIR_J;
//                    bd1->v = extraDet.value(key);
//                    iad.append(bd1);
//                    //iv += (bd1->v * rates.value(bd1->mt));
//                    iv += bd1->v;
//                }
//            }
//    //创建本年利润-结转的借方会计分录
//    bd1 = new BusiActionData2;
//    bd1->state = BusiActionData2::NEW; //新业务活动
//    bd1->num = num++;
//    bd1->pid = idNewIn;
//    bd1->fid = bnlrId;
//    bd1->sid = bnlrSid;
//    bd1->summary = QObject::tr("结转收入至本年利润");
//    bd1->mt = RMB;                     //币种
//    bd1->dir = DIR_D;
//    bd1->v = iv;
//    iad.append(bd1);

//    //结转费用类凭证的业务活动列表
//    num = 1;
//    for(int i = 0; i < fidLst.count(); ++i)
//        for(int j = 0; j < fids.value(fidLst[i]).count(); ++j)
//            for(int k = 0; k < mts.count(); ++k){
//                sid = fids.value(fidLst[i])[j];
//                key = sid * 10 + mts[k];
//                if(extraDet.contains(key) && !extraDet[key].isZero()){
//                    bd1 = new BusiActionData2;
//                    bd1->state = BusiActionData2::NEW; //新业务活动
//                    bd1->num = num++;
//                    bd1->pid = idNewFei;
//                    bd1->fid = fidLst[i];
//                    bd1->sid = sid;
//                    bd1->summary = QObject::tr("结转（%1-%2）至本年利润")
//                            .arg(sm->getFstSubject(fidLst[i])->getName())
//                            .arg(sm->getSndSubject(sid)->getName());
//                    bd1->mt = mts[k];
//                    //结转费用类到本年利润，一般是损益类科目放在贷方，本年利润方在借方
//                    //而且，损益类中费用类科目余额是约定在借方，故不做方向检测
//                    bd1->dir = DIR_D;
//                    bd1->v = extraDet.value(key);
//                    fad.append(bd1);
//                    //fv += (bd1->v * rates.value(bd1->mt));
//                    fv == bd1->v;
//                }
//            }

//    //创建本年利润-结转的借方会计分录
//    bd1 = new BusiActionData2;
//    bd1->state = BusiActionData2::NEW; //新业务活动
//    bd1->num = num++;
//    bd1->pid = idNewFei;
//    bd1->fid = bnlrId;
//    bd1->sid = bnlrSid;
//    bd1->summary = QObject::tr("结转费用至本年利润");
//    bd1->mt = RMB;                        //币种
//    bd1->dir = DIR_J;
//    bd1->v = fv;
//    fad.append(bd1);

//    //保存业务活动数据
//    saveActionsInPz2(idNewIn,iad);
//    saveActionsInPz2(idNewFei,fad);
//    //更新合计值
//    s = QString("update PingZhengs set jsum = %1,dsum = %2 where id = %3")
//            .arg(iv.getv()).arg(iv.getv()).arg(idNewIn);
//    r = q.exec(s);
//    s = QString("update PingZhengs set jsum = %1,dsum = %2 where id = %3")
//            .arg(fv.getv()).arg(fv.getv()).arg(idNewFei);
//    r = q.exec(s);

//    return true;
//}

//获取指定年月指定特种类别（非手工录入凭证）凭证的id（如不存在则创建新的）
//bool BusiUtil::getPzIdForSpecCls(int y, int m, int cls, User* user, int& id)
//{
//    QSqlQuery q;
//    bool r;
//    QString s;

//    QDate d(y,m,1);
//    d.setDate(y,m,d.daysInMonth());
//    QString ds = d.toString(Qt::ISODate);

//    s = QString("select id from PingZhengs where (date='%1') and (isForward=%2)")
//            .arg(ds).arg(cls);
//    if(q.exec(s) && q.first()){
//        id = q.value(0).toInt();
//        //删除原先属于该凭证的所有业务活动
//        s = QString("delete from BusiActions where pid=%1").arg(id);
//        r = q.exec(s);
//    }
//    else{
//        int pzNum = getMaxPzNum(y,m);
//        s = QString("insert into PingZhengs(date,number,isForward,pzState,ruid) "
//                    "values('%1',%2,%3,%4,%5)").arg(ds).arg(pzNum).arg(cls)
//                .arg(Pzs_Recording).arg(user->getUserId());
//        r = q.exec(s);
//        if(r){
//            s = QString("select id from PingZhengs where (date='%1') and (isForward=%2)")
//                    .arg(ds).arg(cls);
//            r = q.exec(s); r = q.first();
//            id = q.value(0).toInt();
//        }
//        else{
//            qDebug() << QObject::tr("创建结转汇兑凭证失败");
//            return false;
//        }
//    }
//}


/**
    结转汇兑损益（此版本将产生3个凭证，分别结转银行，应收和应付）
*/
//bool BusiUtil::genForwordEx2(int y, int m, User *user, int state)
//{
//    QSqlQuery q;
//    QString s;

//    //获取当期汇率表
//    QHash<int,Double> rates,endRates,diffRates;//本期、期末汇率和汇率差
//    if(!getRates2(y, m, rates)){
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

//    if(!getRates2(yy, mm, endRates)){
//        QMessageBox::information(0 ,QObject::tr("提示信息"),QObject::tr("没有指定期末汇率！"));
//        return false;
//    }

//    //计算汇率差
//    QHashIterator<int,Double> it(rates);
//    while(it.hasNext()){
//        it.next();
//        Double diff = it.value() - endRates.value(it.key()); //期初汇率 - 期末汇率
//        if(diff != 0)
//            diffRates[it.key()] = diff;
//    }
//    if(diffRates.count() == 0){   //如果所有的外币汇率都没有变化，则无须进行汇兑损益的结转
//        QMessageBox::information(0 ,QObject::tr("提示信息"),QObject::tr("汇率没有发生变动，无需结转汇兑损益！"));
//        return true;
//    }

//    //获取财务费用、及其下的汇兑损益科目id
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
//    int bankPid;  //结转银行汇兑损益的凭证的id
//    int ysPid;    //结转应收汇兑损益的凭证的id
//    int yfPid;    //结转应付汇兑损益的凭证的id
//    getPzIdForSpecCls(y,m,Pzc_Jzhd_Bank,user,bankPid);
//    getPzIdForSpecCls(y,m,Pzc_Jzhd_Ys,user,ysPid);
//    getPzIdForSpecCls(y,m,Pzc_Jzhd_Yf,user,yfPid);

//    QList<int> wbIds,wbMts;  //银行存款科目下的外币科目id及其对应的币种代码列表
//    getOutMtInBank(wbIds, wbMts);

//    //读取银行存款-外币子科目余额，并依余额值、余额方向和汇率差的方向生成业务活动数据
//    QList<BusiActionData2*> balst;  //保存业务活动数据的列表
//    BusiActionData2 *bd1,*bd2;
//    Double v,sum=0.00;   //子目余额、借贷值合计
//    int edir;   //余额方向（整形表示，有1，0，-1）
//    bool ebdir; //余额方向
//    bool dir; //业务活动的银行存款方的借贷方向，该方向的判定规则：
//              //银行存款余额的方向 && 汇率差的方向（汇率变小为借或正） = 业务活动的借贷方向
//    int num = 1;  //业务活动序号

//    QList<int> ids;        //银行存款、应收、应付总目下的所有子目的id列表
//    QList<QString> names;  //上面子目的名称
//    QHash<int,QString> bankSubNames;  //银行存款下所有外币科目（科目id到科目名称）的映射表

//    //1、创建结转银行存款各子目的结转汇兑损益凭证
//    //生成与结转银行存款汇兑损益的业务活动条目
//    if(getSndSubInSpecFst(bankId,ids,names)){
//        for(int i = 0; i < ids.count(); ++i){
//            if(names[i].indexOf(QObject::tr("人民币")) == -1)
//                bankSubNames[ids[i]] = names[i];
//        }
//        sum = 0.00;num=1;
//        //创建银行一方的会计分录（每个银行外币账户对应一项）
//        for(int i = 0; i < wbIds.count(); ++i){
//            if(!readDetExtraForMt2(y,m,wbIds[i],wbMts[i],v,edir))
//                return false;
//            if(edir == DIR_P)
//                continue;
//            bd1 = new BusiActionData2;  //存放银行存款一方
//            bd1->num = num++;
//            bd1->state = BusiActionData2::NEW;
//            bd1->pid = bankPid;
//            bd1->fid = bankId;
//            bd1->sid = wbIds[i];
//            bd1->mt = RMB;
//            bd1->summary = QObject::tr("结转汇兑损益");
//            bd1->dir = DIR_D;
//            bd1->v = v * diffRates.value(wbMts[i]);
//            sum += bd1->v;
//            balst << bd1;
//        }
//        //创建财务费用一方，是上面这些的合计
//        bd2 = new BusiActionData2;
//        bd2->num = num++;
//        bd2->state = BusiActionData2::NEW;
//        bd2->pid = bankPid;
//        bd2->fid = cwId;
//        bd2->sid = hdId;
//        bd2->mt = RMB;
//        bd2->summary = QObject::tr("结转自银行存款的汇兑损益");
//        bd2->dir = DIR_J;
//        bd2->v = sum;
//        balst<<bd2;
//        //保存业务活动到数据库
//        saveActionsInPz2(bankPid,balst);

//        //清除内存
//        for(int i = 0; i < balst.count(); ++i)
//            delete balst[i];
//        balst.clear();

//        //更新借贷合计值
//        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
//                .arg(sum.getv()).arg(bankPid).arg(state);
//        if(!q.exec(s))
//            return false;
//    }

//    //2、生成与结转应收账款汇兑损益的业务活动条目
//    sum=0.00; num=0;
//    ids.clear(); names.clear();
//    wbMts = rates.keys();  //获取所有外币的代码列表
//    if(getSndSubInSpecFst(ysId,ids,names)){
//        for(int i = 0; i < wbMts.count(); ++i) //外层币种
//            for(int j = 0; j < ids.count(); ++j){      //内存应收账款子目
//                if(!readDetExtraForMt2(y,m,ids[j],wbMts[i],v,edir))
//                    return false;

//                if(edir == DIR_P)
//                    continue;
//                bd1 = new BusiActionData2;  //存放应收账款一方
//                bd1->num = num++;
//                bd1->state = BusiActionData2::NEW;
//                bd1->pid = ysPid;
//                bd1->fid = ysId;
//                bd1->sid = ids[j];
//                bd1->mt = RMB;
//                bd1->summary = QObject::tr("结转汇兑损益");
//                bd1->dir = DIR_D;
//                bd1->v = v * diffRates.value(wbMts[i]);
//				//应收余额在贷方实际上就变成了应付
//		        if(edir == DIR_D)
//		            bd1->v.changeSign();
//                sum += bd1->v;
//                balst << bd1;
//            }
//        //创建财务费用一方
//        bd2 = new BusiActionData2;  //存放财务费用一方
//        bd2->num = num++;
//        bd2->state = BusiActionData2::NEW;
//        bd2->pid = ysPid;
//        bd2->fid = cwId;
//        bd2->sid = hdId;
//        bd2->mt = RMB;
//        bd2->summary = QObject::tr("结转自应收账款的汇兑损益");
//        bd2->dir = DIR_J;
//        bd2->v = sum;
//        balst<<bd2;

//        //保存业务活动到数据库
//        saveActionsInPz2(ysPid,balst);

//        //清除内存
//        for(int i = 0; i < balst.count(); ++i)
//            delete balst[i];
//        balst.clear();

//        //更新借贷合计值和凭证状态
//        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
//                .arg(sum.getv()).arg(ysPid).arg(state);
//        if(!q.exec(s))
//            return false;
//    }

//    //3、生成与结转应付存款汇兑损益的业务活动条目
//    ids.clear();
//    names.clear();
//    sum = 0.00; num=0;
//    if(getSndSubInSpecFst(yfId,ids,names)){
//        for(int i = 0; i < wbMts.count(); ++i) //外层币种
//            for(int j = 0; j < ids.count(); ++j){      //内层应付账款子目
//                if(!readDetExtraForMt2(y,m,ids[j],wbMts[i],v,edir))
//                    return false;
//                if(edir == DIR_P)
//                    continue;
//                bd1 = new BusiActionData2;  //存放应付账款一方
//                bd1->num = num++;
//                bd1->state = BusiActionData2::NEW;
//                bd1->pid = yfPid;
//                bd1->fid = yfId;
//                bd1->sid = ids[j];
//                bd1->mt = RMB;
//                bd1->summary = QObject::tr("结转汇兑损益");
//                bd1->dir = DIR_D;
//                bd1->v = v * diffRates.value(wbMts[i]);
//				//应付余额在借方实际上就变成了应收
//                if(edir == DIR_D)
//                    bd1->v.changeSign();
//                sum += bd1->v;
//                balst << bd1;
//            }
//        //创建财务费用一方
//        bd2 = new BusiActionData2;  //存放财务费用一方
//        bd2->num = num++;
//        bd2->state = BusiActionData2::NEW;
//        bd2->pid = yfPid;
//        bd2->fid = cwId;
//        bd2->sid = hdId;
//        bd2->mt = RMB;
//        bd2->summary = QObject::tr("结转自应付账款的汇兑损益");
//        bd2->dir = DIR_J;
//        bd2->v = sum;
//        balst<<bd2;
//        //保存业务活动到数据库
//        saveActionsInPz2(yfPid,balst);

//        //清除内存
//        for(int i = 0; i < balst.count(); ++i)
//            delete balst[i];
//        balst.clear();

//        //更新借贷合计值和凭证状态
//        s = QString("update PingZhengs set jsum=%1,dsum=%1,pzState=%3 where id=%2")
//                .arg(sum.getv()).arg(yfPid).arg(state);
//        if(!q.exec(s))
//            return false;
//    }
//    return true;
//}

/**
 * @brief BusiUtil::newFstToSnd
 *  在FSAgent表中创建新的一二级科目的映射条目
 * @param fid   主目id
 * @param sid   名称条目id
 * @param id    新建的子目id
 * @return
 */
//bool BusiUtil::newFstToSnd(int fid, int sid, int& id)
//{
//    QSqlQuery q;
//    QString s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,1,1)")
//            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fld_ssub_nid).arg(fld_ssub_weight)
//            .arg(fld_ssub_enable).arg(fid).arg(sid);
//    if(!q.exec(s))
//        return false;
//    s = QString("select last_insert_rowid()");
//    if(!q.exec(s))
//        return false;
//    if(!q.first())
//        return false;
//    id = q.value(0).toInt();
//    return true;
//}

/**
 * @brief BusiUtil::newSndSubAndMapping
 *  创建新的名称条目，并在指定一级科目下创建使用此名称的二级科目
 * @param fid       一级科目id
 * @param id        新建二级科目的id
 * @param name      名称条目简称
 * @param lname     名称条目全称
 * @param remCode   名称条目助记符
 * @param clsCode   名称条目类别
 * @return
 */
//bool BusiUtil::newSndSubAndMapping(int fid, int& id, QString name,QString lname,
//                                   QString remCode, int clsCode)
//{
//    QSqlQuery q;
//    //在SecSubject表中创建新二级科目名称前，不进行检测是因为调用此函数前已经进行了相应的检测
//    QString s = QString("insert into %1(%2,%3,%4,%5) values('%6','%7','%8',%9)")
//            .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_lname).arg(fld_ni_remcode)
//            .arg(fld_ni_class).arg(name).arg(lname).arg(remCode).arg(clsCode);
//    id = 0;
//    if(!q.exec(s))
//        return false;

//    //回读此二级科目的id
//    s = QString("select last_insert_rowid()");
//    if(!q.exec(s))
//        return false;
//    if(!q.first())
//        return false;
//    int sid = q.value(0).toInt();
//    return newFstToSnd(fid,sid,id);
//}

/**
 * @brief BusiUtil::getFstToSnd
 *  读取指定一级科目下，使用指定名称条目的二级科目的id
 * @param fid   一级科目id
 * @param sid   名称条目id
 * @param id    二级科目id
 * @return
 */
//bool BusiUtil::getFstToSnd(int fid, int nid, int& id)
//{
//    QSqlQuery q;
//    QString s = QString("select id from %1 where %2 = %3 and %4 = %5")
//            .arg(tbl_ssub).arg(fld_ssub_fid).arg(fid).arg(fld_ssub_nid).arg(nid);
//    if(!q.exec(s))
//        return false;
//    if(!q.first()){
//        id = 0;
//        return false;
//    }
//    id = q.value(0).toInt();
//    return true;
//}

/**
 * @brief BusiUtil::isSndSubDisabled
 *  读取指定的二级科目是否已被禁用
 * @param id        二级科目id
 * @param enabled   是否被禁用（false：禁用，true：启用）
 * @return
 */
//bool BusiUtil::isSndSubDisabled(int id, bool &enabled)
//{
//    QSqlQuery q;
//    QString s = QString("select %1 from %2 where id=%3")
//            .arg(fld_ssub_enable).arg(tbl_ssub).arg(id);
//    if(!q.exec(s))
//        return false;
//    if(!q.first())
//        return false;
//    enabled = q.value(0).toBool();
//    return true;
//}

/**
 * @brief BusiUtil::getSndSubNameForId
 *  获取指定二级科目的简称和全称
 * @param id
 * @param name
 * @param lname
 * @return
 */
//bool BusiUtil::getSndSubNameForId(int id, QString& name, QString& lname)
//{
//    QSqlQuery q;
//    QString s = QString("select %1.%2,%1.%3 from %4 join %1 "
//                "where (%1.id = %4.%5) and (%4.id = %6)")
//            .arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ni_lname).arg(tbl_ssub)
//            .arg(fld_ssub_nid).arg(id);
//    if(!q.exec(s))
//        return false;
//    if(q.first()){
//        name = q.value(0).toString();
//        lname = q.value(1).toString();
//    }
//    return true;
//}



/**
 * @brief BusiUtil::readExtraForSub2
 *  读取指定月份指定总账科目的余额
 * @param y
 * @param m
 * @param fid   总账科目id
 * @param v     以原币形式表示的外币余额
 * @param wv    以本币形式表示的外币余额（仅对于需要保存外币余额的科目而言）
 * @param dir   方向（币种代码）
 * @return
 */
bool BusiUtil::readExtraForSub2(int y, int m, int fid, QHash<int, Double> &v,
                                QHash<int,Double>& wv, QHash<int, int> &dir)
{
    QSqlQuery q(db);

    //获取总账科目余额对应的字段名称
    QString s = QString("select %1 from %2 where id = %3")
            .arg(fld_fsub_subcode).arg(tbl_fsub).arg(fid);
    if(!q.exec(s))
        return false;
    if(!q.first()){
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
    if(!q.first())
        return true;    

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
 * @brief BusiUtil::readExtraForDetSub2
 *  读取指定月份指定明细科目的余额
 * @param y
 * @param m
 * @param sid   明细科目id
 * @param v     余额值（原币）
 * @param wv    余额值（本币）
 * @param dir   余额方向
 * @return
 */
bool BusiUtil::readExtraForDetSub2(int y, int m, int sid,
                                   QHash<int, Double> &v,
                                   QHash<int,Double>& wv,
                                   QHash<int, int> &dir)
{
    QSqlQuery q(db),q2(db);
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
 * @brief BusiUtil::readExtraByMonth2
 *  读取老机制的余额（本币形式）
 * @param y
 * @param m
 * @param sums      一级科目余额(key：id x 10 + 币种代码)
 * @param fdirs     一级科目余额的方向
 * @param ssums     明细科目余额
 * @param sdirs     二级科目余额的方向
 * @return
 */
bool BusiUtil::readExtraByMonth2(int y, int m, QHash<int, Double> &sums, QHash<int, MoneyDirection> &fdirs, QHash<int, Double> &ssums, QHash<int, MoneyDirection> &sdirs)
{
    QSqlQuery q(db),q2(db);
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
            sdirs[sid*10+mt] = (MoneyDirection)q2.value(DE_DIR).toInt();      //方向
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
                fdirs[i.key()*10+mt] = (MoneyDirection)dir;
        }
    }
    return true;
}

//读取科目余额，键为科目id*10+币种代码，但金额以本币计（即从表subjectExtra和detailExtra读取原币形式
//的余额值，如果是外币就直接乘以汇率得出以本币计的余额值）
bool BusiUtil::readExtraByMonth3(int y, int m, QHash<int, Double> &sumsR,
                                 QHash<int, MoneyDirection> &fdirsR,
                                 QHash<int, Double> &ssumsR,
                                 QHash<int, MoneyDirection> &sdirsR)
{
    QSqlQuery q(db),q2(db);
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
            sdirsR[sid*10+mt] = (MoneyDirection)q2.value(DE_DIR).toInt();      //方向
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
                fdirsR[i.key()*10+mt] = (MoneyDirection)dir;
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
    QSqlQuery q(db);
    QString s;

    s = QString("select * from %1 where %2=%3 and %4=%5").arg(tbl_sem)
            .arg(fld_sem_year).arg(y).arg(fld_sem_month).arg(m);
    if(!q.exec(s))
        return false;

    //QList<int> mts = allMts.keys();
    //mts.removeOne(RMB);

    QHash<int,QString> fldNames;  //键为科目代码，值为保存科目代码的字段名
    QList<int> ids;            //科目id列表
    getFidToFldName2(fldNames);
    ids = fldNames.keys();

    //读取主目余额
    //如果表中没有指定年月的余额记录（比如期初），则采用直接将外币余额值转换为本币的值
    if(!q.first()){
        QHash<int,MoneyDirection> dirs;
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
            if((mt == RMB) ||                          //不是外币
                    ((mt != RMB) && !fldNames.contains(id)))//是外币，但科目不要求按币种核算
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
            if((mt == RMB) ||
                    ((mt != RMB) && !sids.contains(id)))
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
 * @brief BusiUtil::readDetExtraForMt2
 *  读取指定月份-指定明细科目-指定币种的余额
 * @param y
 * @param m
 * @param sid       明细科目id
 * @param mt        币种代码
 * @param v         余额值
 * @param dir       余额方向
 * @return
 */
//bool BusiUtil::readDetExtraForMt2(int y, int m, int sid, int mt, Double &v, int &dir)
//{
//    QSqlQuery q;
//    QString s = QString("select id from SubjectExtras where (year=%1) and (month=%2) "
//                "and (mt=%3)").arg(y).arg(m).arg(mt);
//    if(!q.exec(s)){
//        qDebug() << QObject::tr("不能获取%1年%2月，币种代码为%3的余额记录")
//                    .arg(y).arg(m).arg(mt);
//        return false;
//    }
//    if(!q.first()){
//        v = 0.00;
//        dir = DIR_P;
//        return true;
//    }
//    int seid = q.value(0).toInt();
//    s = QString("select value,dir from detailExtras where (seid=%1) and (fsid=%2)")
//            .arg(seid).arg(sid);
//    if(!q.exec(s)){
//        qDebug() << QObject::tr("不能获取%1年%2月，明细科目id=%3，币种代码为%4的余额记录")
//                    .arg(y).arg(m).arg(sid).arg(mt);
//        return false;
//    }
//    if(!q.first()){
//        v = 0.00;
//        dir = DIR_P;
//    }
//    else{
//        v= Double(q.value(0).toDouble());
//        dir = q.value(1).toInt();
//    }
//    return true;
//}


/**
 * @brief BusiUtil::savePeriodBeginValues2
 *  保存各科目原币形式的余额到表SubjectExtras，detailExtras
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
//bool BusiUtil::savePeriodBeginValues2(int y, int m, QHash<int, Double> newF, QHash<int, MoneyDirection> newFDir, QHash<int, Double> newS, QHash<int, MoneyDirection> newSDir, bool isSetup)
//{
//    QSqlQuery q(db);
//    QString s;

//    QHash<int,QString> fldNames; //一级科目id到保存其余额的字段名
//    BusiUtil::getFidToFldName(fldNames);

//    //1、初始化exist表，此表是SubjectExtras表中已有的保存各币种余额值记录的id
//    //(key是币种代码，value是记录id)
//    QHash<int,int> exists;
//    s = QString("select id,mt from SubjectExtras where (year = %1) "
//                "and (month = %2)").arg(y).arg(m);
//    q.exec(s);
//    while(q.next())
//        exists[q.value(1).toInt()] = q.value(0).toInt();

//    //获取所有币种代码列表
//    QHash<int,QString> mtHash; //币种代码和名称
//    getMTName(mtHash);
//    QList<int> mtLst = mtHash.keys(); //币种代码的列表

//    //获取所有本账户内启用的总账和明细帐科目id
//    QList<int> fids,sids;
//    q.exec(QString("select id from %1 where %2=1")
//           .arg(tbl_fsub).arg(fld_fsub_isview));
//    while(q.next())
//        fids << q.value(0).toInt();
//    q.exec(QString("select id from %1").arg(tbl_ssub));
//    while(q.next())
//        sids << q.value(0).toInt();

//    //读取老值集
//    QHash<int, Double> oldF,oldS;   //余额
//    QHash<int,MoneyDirection> odF, odS;    //方向
//    readExtraByMonth2(y,m,oldF,odF,oldS,odS);

//    //初始化总账科目余额和方向的sql更新语句
//    QString s1,s2,vs1,vs2;
//    int key;
//    bool oc,nc; //分别表示某个指定余额值是否存在于老值或新值集中
//    bool flag  = false; //是否需要执行sql语句的标记
//    //1、处理总账科目余额及方向
//    //遍历所有币种和总账科目id的组合键来匹配键值
//    for(int i = 0; i < mtLst.count(); ++i){
//        if(!exists.contains(mtLst[i])){ //不存在则执行插入操作
//            s1 = "insert into SubjectExtras(year,month,mt,";
//            s2 = "insert into SubjectExtraDirs(year,month,mt,";
//            vs1 = QString(" values(%1,%2,%3,").arg(y).arg(m).arg(mtLst[i]);
//            vs2 = QString(" values(%1,%2,%3,").arg(y).arg(m).arg(mtLst[i]);
//            for(int j = 0; j < fids.count(); ++j){
//                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
//                if(newF.contains(key)){
//                    s1.append(fldNames.value(fids[j])).append(",");
//                    s2.append(fldNames.value(fids[j])).append(",");
//                    QString sv;
//                    if(newF.value(key) == 0)
//                        sv = "0";
//                    else
//                        sv = newF.value(key).toString();
//                    vs1.append(QString("%1,").arg(sv));
//                    vs2.append(QString("%1,").arg(newFDir.value(key)));
//                }
//            }
//            s1.chop(1); s1.append(")");
//            vs1.chop(1); vs1.append(")");
//            s1.append(vs1);
//            s2.chop(1); s2.append(")");
//            vs2.chop(1); vs2.append(")");
//            s2.append(vs2);
//            q.exec(s1);
//            q.exec(s2);
//            //回读seid
//            s1 = QString("select id from SubjectExtras where (year=%1) and "
//                         "(month=%2) and (mt=%3)").arg(y).arg(m).arg(mtLst[i]);
//            q.exec(s1); q.first();
//            exists[mtLst[i]] = q.value(0).toInt();
//        }
//        else{  //存在则执行更新操作
//            s1 = "update SubjectExtras set ";
//            s2 = "update SubjectExtraDirs set ";
//            for(int j = 0; j < fids.count(); ++j){
//                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
//                nc = newF.contains(key);
//                oc = oldF.contains(key);
//                if(!nc && !oc)  //都不存在则忽略
//                    continue;
//                else if(oldF.value(key) == newF.value(key)){ //值没变但方向变了
//                    if(odF.value(key) !=  newFDir.value(key)){
//                        flag = true;
//                        s2.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
//                                  .arg(QString::number(newFDir.value(key))));
//                    }
//                }
//                else if(!nc && oc){ //如果新值没有老值有，则清零
//                    flag = true;
//                    s1.append(QString("%1=0,").arg(fldNames.value(fids[j])));
//                    s2.append(QString("%1=0,").arg(fldNames.value(fids[j])));
//                }
//                else{
//                    flag = true;
//                    QString sv;
//                    if(newF.value(key) == 0)
//                        sv = "0";
//                    else
//                        sv = newF.value(key).toString();
//                    s1.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
//                              .arg(sv));
//                    s2.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
//                              .arg(newFDir.value(key)));
//               }
//            }
//            if(flag){
//                s1.chop(1);
//                s2.chop(1);
//                QString ws = QString(" where (year=%1) and (month=%2) and (mt=%3)")
//                        .arg(y).arg(m).arg(mtLst[i]);
//                s1.append(ws); s2.append(ws);
//                q.exec(s1);
//                q.exec(s2);
//            }
//        }
//    }

//    //2、处理明细账科目余额及方向
//    //QSqlDatabase db = QSqlDatabase::database();
//    if(!db.transaction()){
//        QMessageBox::critical(0,QObject::tr("错误信息"),QObject::tr("在保存余额操作期间，启动事务失败！"));
//        return false;
//    }
//    for(int i = 0; i < mtLst.count(); ++i){
//        for(int j = 0; j < sids.count(); ++j){
//            key = sids[j] * 10 + mtLst[i];
//            oc = oldS.contains(key); //老值集是否包含该键
//            nc = newS.contains(key); //新值集是否包含该键
//            if(!oc && !nc)                //如果新老值都不包含，则跳过
//                continue;
//            else if(oc && !nc){ //如果老值包含新值不包含则要删除
//                s = QString("delete from detailExtras where (seid=%1) and (fsid=%2) ")
//                            .arg(exists.value(mtLst[i])).arg(sids[j]);
//                q.exec(s);
//            }
//            //如果新值包含老值不包含则要新增
//            else if(!oldS.contains(key) && newS.contains(key)){
//                //注意：如果这里直接使用Double::getv()方法，由于返回的双精度实数使用了科学计数法，
//                //当数位超过7位时，看起来好像失去了精度（最后几位好像被省略了）
//                //因此，这里我使用了字符串来代替，以输出实际的实数值。
//                QString sv;
//                if(newS.value(key) == 0){ //如果新值是0，则不保存，同时为了节省空间，也要删除对应记录
//                    s = QString("delete from detailExtras where seid=%1 and fsid=%2")
//                            .arg(exists.value(mtLst[i])).arg(sids[j]);
//                }
//                else{
//                    sv = newS.value(key).toString();
//                    s = QString("insert into detailExtras(seid,fsid,dir,value) "
//                                "values(%1,%2,%3,%4)").arg(exists.value(mtLst[i]))
//                            .arg(sids[j]).arg(newSDir.value(key))
//                            .arg(sv);
//                }
//                q.exec(s);
//            }
//            else if((oldS.value(key) != newS.value(key))
//                    || (odS.value(key) != newSDir.value(key))){ //值或方向改变了
//                QString sv;
//                if(newS.value(key) == 0){
//                    s = QString("delete from detailExtras where seid=%1 and fsid=%2")
//                            .arg(exists.value(mtLst[i])).arg(sids[j]);
//                }
//                else{
//                    sv = newS.value(key).toString();
//                    s = QString("update detailExtras set dir=%1,value=%2 where "
//                                "(seid=%3) and (fsid=%4)").arg(newSDir.value(key))
//                            .arg(sv).arg(exists.value(mtLst[i])).arg(sids[j]);
//                }
//                q.exec(s);
//            }
//        }
//    }
//    if(!db.commit()){
//        QMessageBox::critical(0,QObject::tr("错误信息"),QObject::tr("在保存余额操作期间，提交事务失败！"));
//        return false;
//    }

//    //设置余额状态为有效
//    setExtraState(y,m,true);

//    if(isSetup)  //如果是保存设定的期初值，则凭证集状态为结账
//        setPzsState(y,m,Ps_Jzed);
//    return true;
//}

//保存科目的外币（转换为人民币）余额(SubjectMmtExtras,detailMmtExtras)
//bool BusiUtil::savePeriodEndValues(int y, int m, QHash<int, Double> newF,
//                                    QHash<int, Double> newS)
//{
//    QSqlQuery q(db);
//    QString s;

//    //保存主科目
//    QHash<int,QString> fldNames; //需要按币种核算的一级科目id到保存其余额的字段名
//    BusiUtil::getFidToFldName2(fldNames);

//    //1、初始化exist表，此表是SubjectMmtExtras表中已有的保存外币余额值记录的id
//    //(key是币种代码，value是记录id)
//    QHash<int,int> exists;
//    s = QString("select id,%1 from %2 where (%3 = %4) "
//                "and (%5 = %6)").arg(fld_sem_mt).arg(tbl_sem).arg(fld_sem_year)
//            .arg(y).arg(fld_sem_month).arg(m);
//    if(!q.exec(s))
//        return false;
//    while(q.next())
//        exists[q.value(1).toInt()] = q.value(0).toInt();

//    //获取所有币种代码列表
//    QHash<int,QString> mtHash; //币种代码和名称
//    getMTName(mtHash);
//    QList<int> mtLst = mtHash.keys(); //币种代码的列表
//    mtLst.removeOne(RMB);

//    //获取所有本账户内启用的需要按币种进行核算的总账和明细帐科目id
//    QList<int> fids,sids;
//    fids = fldNames.keys();
//    for(int i = 0; i < fids.count(); ++i){
//        s = QString("select id from %1 where fid=%2")
//                .arg(tbl_ssub).arg(fids[i]);
//        if(!q.exec(s))
//            return false;
//        while(q.next())
//            sids<<q.value(0).toInt();
//    }

//    //读取老值集
//    QHash<int, Double> oldF,oldS;   //余额
//    bool exist;  //指定年月的外币余额是读取自总余额表还是外币余额表，
//    readExtraByMonth4(y,m,oldF,oldS,exist); //从subjectMmtExtra表中读取

//    //初始化总账科目余额和方向的sql更新语句
//    QString s1,vs1;
//    int key;
//    bool oc,nc; //分别表示某个指定余额值是否存在于老值或新值集中
//    bool flag  = false; //是否需要执行sql语句的标记
//    //1、处理总账科目余额及方向
//    //遍历所有币种和总账科目id的组合键来匹配键值
//    for(int i = 0; i < mtLst.count(); ++i){
//        if(!exists.contains(mtLst[i])){ //不存在则执行插入操作
//            s1 = QString("insert into %1(%2,%3,%4,").arg(tbl_sem)
//                    .arg(fld_sem_year).arg(fld_sem_month).arg(fld_sem_mt);
//            vs1 = QString(" values(%1,%2,%3,").arg(y).arg(m).arg(mtLst[i]);
//            for(int j = 0; j < fids.count(); ++j){
//                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
//                if(newF.contains(key)){
//                    s1.append(fldNames.value(fids[j])).append(",");
//                    QString sv;
//                    if(newF.value(key) == 0)
//                        sv = "0";
//                    else
//                        sv = newF.value(key).toString();
//                    vs1.append(QString("%1,").arg(sv));
//                }
//            }
//            s1.chop(1); s1.append(")");
//            vs1.chop(1); vs1.append(")");
//            s1.append(vs1);
//            if(!q.exec(s1))
//                return false;
//            //回读seid
//            s1 = QString("select id from %1 where (%2=%3) and "
//                         "(%4=%5) and (%6=%7)").arg(tbl_sem).arg(fld_sem_year)
//                    .arg(y).arg(fld_sem_month).arg(m).arg(fld_sem_mt).arg(mtLst[i]);
//            if(!q.exec(s1))
//                return false;
//            if(!q.first())
//                return false;
//            exists[mtLst[i]] = q.value(0).toInt();
//        }
//        else{  //存在则执行更新操作
//            s1 = QString("update %1 set ").arg(tbl_sem);
//            for(int j = 0; j < fids.count(); ++j){
//                key = fids[j] * 10 + mtLst[i];    //科目id加币种代码构成键
//                nc = newF.contains(key);
//                oc = oldF.contains(key);
//                if(!nc && !oc)  //都不存在则忽略
//                    continue;
//                else if(oldF.value(key) == newF.value(key)) //值没变
//                    continue;
//                else if(!nc && oc){ //如果新值没有老值有，则清零
//                    flag = true;
//                    s1.append(QString("%1=0,").arg(fldNames.value(fids[j])));
//                }
//                else{
//                    flag = true;
//                    QString sv;
//                    if(newF.value(key) == 0)
//                        sv = "0";
//                    else
//                        sv = newF.value(key).toString();
//                    s1.append(QString("%1=%2,").arg(fldNames.value(fids[j]))
//                              .arg(sv));
//               }
//            }
//            if(flag){
//                s1.chop(1);
//                QString ws = QString(" where (%1=%2) and (%3=%4) and (%5=%6)")
//                        .arg(fld_sem_year).arg(y).arg(fld_sem_month).arg(m)
//                        .arg(fld_sem_mt).arg(mtLst[i]);
//                s1.append(ws);
//                if(!q.exec(s1))
//                    return false;
//            }
//        }
//    }

//    //保存子科目
//    //QSqlDatabase db = QSqlDatabase::database();
//    if(!db.transaction()){
//        QMessageBox::critical(0,QObject::tr("错误信息"),QObject::tr("在保存余额操作期间，启动事务失败！"));
//        return false;
//    }
//    for(int i = 0; i < mtLst.count(); ++i){
//        for(int j = 0; j < sids.count(); ++j){
//            key = sids[j] * 10 + mtLst[i];
//            oc = oldS.contains(key); //老值集是否包含该键
//            nc = newS.contains(key); //新值集是否包含该键
//            if(!oc && !nc)                //如果新老值都不包含，则跳过
//                continue;
//            //如果老值包含新值不包含，或新值等于0则要删除
//            else if((oc && !nc) || (newS.value(key) == 0.00)){
//                s = QString("delete from %1 where (%2=%3) and (%4=%5)").arg(tbl_sdem)
//                            .arg(fld_sdem_seid).arg(exists.value(mtLst[i]))
//                            .arg(fld_sdem_fsid).arg(sids[j]);
//                if(!q.exec(s))
//                    return false;
//            }
//            //如果新值包含老值不包含则要新增，或者根本没有创建对应月份记录
//            else if(!exist || (!oldS.contains(key) && newS.contains(key))){
//                //注意：如果这里直接使用Double::getv()方法，由于返回的双精度实数使用了科学计数法，
//                //当数位超过7位时，看起来好像失去了精度（最后几位好像被省略了）
//                //因此，这里我使用了字符串来代替，以输出实际的实数值。
//                QString sv;
//                if(newS.value(key) == 0)
//                    continue;
//                sv = newS.value(key).toString();
//                s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)")
//                            .arg(tbl_sdem).arg(fld_sdem_seid).arg(fld_sdem_fsid)
//                        .arg(fld_sdem_value).arg(exists.value(mtLst[i]))
//                        .arg(sids[j]).arg(newS.value(key).toString());
//                if(!q.exec(s))
//                    return false;
//            }
//            else if(oldS.value(key) != newS.value(key)){ //值改变了
//                QString sv;
//                if(newS.value(key) == 0)
//                    continue;
//                sv = newS.value(key).toString();
//                s = QString("update %1 set %2=%3 where (%4=%5) and (%6=%7)")
//                            .arg(tbl_sdem).arg(fld_sdem_value).arg(sv)
//                        .arg(fld_sdem_seid).arg(exists.value(mtLst[i]))
//                        .arg(fld_sdem_fsid).arg(sids[j]);
//                if(!q.exec(s))
//                    return false;
//            }
//        }
//    }
//    if(!db.commit()){
//        QMessageBox::critical(0,QObject::tr("错误信息"),QObject::tr("在保存余额操作期间，提交事务失败！"));
//        return false;
//    }
//}


//为查询处于指定状态的某些类别的凭证生成过滤子句
//void BusiUtil::genFiltState(QList<int> pzCls, PzState state, QString& s)
//{
//    QString ts;
//    if(!pzCls.empty()){
//        for(int i = 0; i < pzCls.count(); ++i){
//            ts.append(QString("(isForward=%1) or ").arg(pzCls[i]));
//        }
//        ts.chop(4);
//        if(pzCls.count()>1)
//            s = QString("(%1) and (pzState=%2)").arg(ts).arg(state);
//        else
//            s = QString("%1 and (pzState=%2)").arg(ts).arg(state);
//    }
//}

/**
 * @brief BusiUtil::genFiltStateForSpecPzCls
 *  生成过滤出指定类别的凭证的条件语句
 * @param pzClses
 */
//QString BusiUtil::genFiltStateForSpecPzCls(QList<int> pzClses)
//{
//    if(pzClses.empty())
//        return "";
//    QString sql;
//    for(int i = 0; i < pzClses.count(); ++i){
//        sql.append(QString("(%1=%2) or ").arg(fld_pz_class).arg(pzClses.at(i)));
//    }
//    sql.chop(4);
//    return sql;
//}

/**
 * @brief BusiUtil::delSpecPz
 *  删除指定年月的指定大类别凭证
 * @param y
 * @param m
 * @param pzCls         凭证大类
 * @param pzCntAffected 删除的凭证数
 * @return
 */
//bool BusiUtil::delSpecPz(int y, int m, PzdClass pzCls, int 	&affected)
//{
//    QString s;
//    QSqlQuery q,q2;
//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    QList<PzClass> codes = getSpecClsPzCode(pzCls);
//    QSqlDatabase db = QSqlDatabase::database();
//    if(!db.transaction())
//        return false;
//    QList<int> ids;
//    for(int i = 0; i < codes.count(); ++i){
//        s = QString("select id from %1 where (%2 like '%3%') and (%4=%5)")
//                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class).arg(codes.at(i));
//        q.exec(s);q.first();
//        ids<<q.value(0).toInt();
//    }
//    if(!db.commit())
//        return false;
//    if(!db.transaction())
//        return false;
//    affected = 0;
//    for(int i = 0; i < ids.count(); ++i){
//        s = QString("delete from %1 where %2=%3").arg(tbl_ba).arg(fld_ba_pid).arg(ids.at(i));
//        q.exec(s);
//        s = QString("delete from %1 where id=%2").arg(tbl_pz).arg(ids.at(i));
//        q.exec(s);
//        affected += q.numRowsAffected();
//    }
//    return db.commit();
//}

/**
 * @brief BusiUtil::haveSpecClsPz
 *  检测凭证集内各类特殊凭证是否存在
 * @param y
 * @param m
 * @param isExist
 * @return
 */
//bool BusiUtil::haveSpecClsPz(int y, int m, QHash<PzdClass, bool> &isExist)
//{
//    QSqlQuery q;
//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    QString s = QString("select id from %1 where (%2 like '%3%') and (%4=%5 or %4=%6 or %4=%7)")
//            .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class).arg(Pzc_Jzhd_Bank)
//            .arg(Pzc_Jzhd_Ys).arg(Pzc_Jzhd_Yf);
//    if(!q.exec(s))
//        return false;
//    isExist[Pzd_Jzhd] = q.first();

//    s = QString("select id from %1 where (%2 like '%3%') and (%4=%5 or %4=%6)")
//                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class)
//                .arg(Pzc_JzsyIn).arg(Pzc_JzsyFei);
//    if(!q.exec(s))
//        return false;
//    isExist[Pzd_Jzsy] = q.first();
//    s = QString("select id from %1 where (%2 like '%3%') and (%4=%5)")
//                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_class)
//                .arg(Pzc_Jzlr);
//    if(!q.exec(s))
//        return false;
//    isExist[Pzd_Jzlr] = q.first();
//    return true;
//}

/**
 * @brief BusiUtil::setExtraState
 *  设置余额是否有效
 * @param y
 * @param m
 * @param isVolid
 * @return
 */
bool BusiUtil::setExtraState(int y, int m, bool isVolid)
{
    //记录外币余额的那条记录，不考虑用state字段来记录
    QSqlQuery q(db);
    QString s = QString("select id from %1 where %2=%3 and %4=%5 and %6=%7")
            .arg(tbl_se).arg(fld_se_year).arg(y).arg(fld_se_month).arg(m)
            .arg(fld_se_mt).arg(RMB);
    if(!q.exec(s))
        return false;
    bool isCrt = false;
    if(q.first()){
        int id = q.value(0).toInt();
        s = QString("update %1 set %2=%3 where id=%4")
                .arg(tbl_se).arg(fld_se_state).arg(isVolid?1:0).arg(id);
    }
    else{
        s = QString("insert into %1(%2,%3,%4,%5) values(%6,%7,%8,%9)")
                .arg(tbl_se).arg(fld_se_year).arg(fld_se_month).arg(fld_se_state)
                .arg(fld_se_mt).arg(y).arg(m).arg(isVolid?1:0).arg(RMB);
    }
    if(!q.exec(s))
        return false;
    //因为在余额表SubjectExtras中插入一条记录（保存人民币的那条记录），
    //一定要同步在余额方向表（SubjectExtraDirs）中创建一条对应的记录，
    //要不然在保存余额时会遗失余额方向信息，导致余额保存信息不完整
    if(isCrt){
        s = QString("select id from SubjectExtraDirs where year=%1 and month=%2 and mt=%3")
                .arg(y).arg(m).arg(RMB);
        if(!q.exec(s))
            return false;
        if(!q.first()){
            s = QString("insert into SubjectExtraDirs(year,month,mt) values(%1,%2,%3)")
                    .arg(y).arg(m).arg(RMB);
            if(!q.exec(s))
                return false;
        }
    }
    return true;
}

/**
 * @brief BusiUtil::getExtraState
 *  获取余额是否正确的反映了指定凭证集的最新状态
 * @param y
 * @param m
 * @return
 */
bool BusiUtil::getExtraState(int y, int m)
{
    QSqlQuery q(db);
    QString s = QString("select %1 from %2 where %3=%4 and %5=%6 and %7=%8")
            .arg(fld_se_state).arg(tbl_se).arg(fld_se_year).arg(y).arg(fld_se_month)
            .arg(m).arg(fld_se_mt).arg(RMB);
    if(!q.exec(s))
        return false;
    if(!q.first())
        return false;
    return q.value(0).toBool();
}

/**
 * @brief BusiUtil::specPzClsInstat
 *  将指定类别凭证全部入账
 * @param y
 * @param m
 * @param cls
 * @param affected  收影响的凭证条目
 * @return
 */
//bool BusiUtil::specPzClsInstat(int y, int m, PzdClass cls, int &affected)
//{
//    QSqlDatabase db = QSqlDatabase::database();
//    QSqlQuery q(db);
//    QString s;
//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    QList<PzClass> codes = getSpecClsPzCode(cls);
//    if(!db.transaction())
//        return false;
//    affected = 0;
//    for(int i = 0; i < codes.count(); ++i){
//        s = QString("update %1 set %2=%3 where (%4 like '%5%') and %6=%7")
//                .arg(tbl_pz).arg(fld_pz_state).arg(Pzs_Instat)
//                .arg(fld_pz_date).arg(ds).arg(fld_pz_class).arg(codes.at(i));
//        q.exec(s);
//        affected += q.numRowsAffected();
//    }
//    return db.commit();
//}

/**
 * @brief BusiUtil::setAllPzState
 *  设置凭证集内的所有具有指定状态的凭证到目的状态
 * @param y
 * @param m
 * @param state         目的状态
 * @param includeState  指定的凭证状态
 * @param affected      受影响的行数
 * @param user
 * @return
 */
//bool BusiUtil::setAllPzState(int y, int m, PzState state, PzState includeState , int &affected, User *user)
//{
//    QSqlQuery q;
//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    QString userField;
//    if(state == Pzs_Recording)
//        userField = fld_pz_ru;
//    else if(state == Pzs_Verify)
//        userField = fld_pz_vu;
//    else if(state == Pzs_Instat)
//        userField = fld_pz_bu;
//    QString s = QString("update %1 set %2=%3,%4=%5 where (%6 like '%7%') and %2=%8")
//            .arg(tbl_pz).arg(fld_pz_state).arg(state).arg(userField)
//            .arg(user->getUserId()).arg(fld_pz_date).arg(ds).arg(includeState);
//    affected = 0;
//    if(!q.exec(s))
//        return false;
//    affected = q.numRowsAffected();
//    return true;
//}


/**
 * @brief BusiUtil::genStatSql2
 *  依据新的简化版凭证集状态
 *  构造统计查询语句（根据当前凭证集的状态来构造将不同类别的凭证纳入统计范围的SQL语句）
 * @param y
 * @param m
 * @param s
 * @return
 */
//bool BusiUtil::genStatSql2(int y, int m, QString &s)
//{
//    //获取凭证集状态
//    PzsState pzsState;
//    //refreshPzsState2(y,m);
//    getPzsState(y,m,pzsState);
//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    switch(pzsState){
//    case Ps_Rec:
//        //应该只返回必要的字段（id、凭证类别等）
//        s = QString("select * from %1 where (%2 like '%3%') and %4!=%5")
//                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_state).arg(Pzs_Repeal);
//        break;
//    case Ps_AllVerified:
//        case Ps_Jzed:
//        s = QString("select * from %1 where (%2 like '%3%') and %4=%5")
//                .arg(tbl_pz).arg(fld_pz_date).arg(ds).arg(fld_pz_state).arg(Pzs_Instat);
//        break;
//    }
//    return true;
//}


//使用Double来计算本期发生额
//bool BusiUtil::calAmountByMonth2(int y, int m, QHash<int,Double>& jSums, QHash<int,Double>& dSums,
//             QHash<int,Double>& sjSums, QHash<int,Double>& sdSums,
//                                bool& isSave, int& amount)
//{
//    QSqlQuery q(db),q2(db);
//    QString s;

//    //初始化汇率表
//    QHash<int,Double>rates;
//    if(!getRates2(y,m,rates))
//        return false;
//    rates[RMB] = Double(1.00);

//    //初始化币种表
//    QHash<int,QString> mts;
//    if(!getMTName(mts))
//        return false;

//    //初始化需要进行明细核算的一级科目的id列表
//    //QList<int> detSubs;
//    //if(!getReqDetSubs(detSubs))
//    //    return false;

//    //生成查询语句
//    if(!genStatSql2(y,m,s))
//        return false;

//    if(!q.exec(s)){
//        QMessageBox::information(0, QObject::tr("提示信息"),
//                                 QString(QObject::tr("在计算本期发生额时不能获取凭证集")));
//        return false;
//    }

//    //判定是否可以保存余额
//    PzsState state;
//    getPzsState(y,m,state);
//    isSave = (state == Ps_AllVerified);

//    //遍历凭证集
//    amount = 0;
//    int pid,fid,sid,mtype;
//    PzClass pzCls; //凭证类别
//    Double jv,dv;  //业务活动的借贷金额
//    MoneyDirection dir;       //业务活动借贷方向
//    bool isJzhdPz = false;
//    while(q.next()){
//        pid = q.value(0).toInt(); //凭证id
//        pzCls = (PzClass)q.value(PZ_CLS).toInt();
//        isJzhdPz = pzClsJzhds.contains(pzCls)/*Pzc_Jzhd_Bank || pzCls == Pzc_Jzhd_Ys ||
//                pzCls == Pzc_Jzhd_Yf*/;
//        s = QString("select * from BusiActions where pid = %1").arg(pid);
//        if(!q2.exec(s)){
//            QMessageBox::information(0, QObject::tr("提示信息"),
//                                     QString(QObject::tr("在计算本期发生额时不能获取id为%1的凭证的业务活动表")).arg(pid));
//            return false;
//        }
//        amount++;
//        //遍历凭证的业务活动
//        while(q2.next()){
//            fid = q2.value(BACTION_FID).toInt();
//            //如果是结转汇兑损益的凭证，则跳过非财务费用方的会计分录，因为这些要计入到外币部分
//            if(isJzhdPz && isAccMt(fid))
//                continue;
//            int key;
//            sid = q2.value(BACTION_SID).toInt();
//            mtype = q2.value(BACTION_MTYPE).toInt();
//            dir = (MoneyDirection)q2.value(BACTION_DIR).toInt();

//            if(dir == MDIR_J){//发生在借方
//                jv = Double(q2.value(BACTION_VALUE).toDouble());
//                jSums[fid*10+mtype] += jv;
//                if(!dSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
//                    dSums[fid*10+mtype] = Double(0.00);
//                //if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
//                key = sid*10+mtype;
//                sjSums[key] += jv;
//                if(!sdSums.contains(key))
//                    sdSums[key] = Double(0.00);
//                //}
//            }
//            else{
//                dv = Double(q2.value(BACTION_VALUE).toDouble());
//                dSums[fid*10+mtype] += dv;
//                if(!jSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
//                    jSums[fid*10+mtype] = Double(0.00);
//                //if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
//                key = sid*10+mtype;
//                sdSums[key] += dv;
//                if(!sjSums.contains(key))
//                    sjSums[key] = Double(0.00);
//                //}
//            }

//        }
//    }
//    return true;
//}

//计算本期末余额（参数所涉及的金额都以本币计）
//bool BusiUtil::calCurExtraByMonth3(int y,int m,
//       QHash<int,Double> preExaR, QHash<int,Double> preDetExaR,     //期初余额
//       QHash<int,MoneyDirection> preExaDirR, QHash<int,MoneyDirection> preDetExaDirR,     //期初余额方向
//       QHash<int,Double> curJHpnR, QHash<int,Double> curJDHpnR,     //当期借方发生额
//       QHash<int,Double> curDHpnR, QHash<int,Double>curDDHpnR,      //当期贷方发生额
//       QHash<int,Double> &endExaR, QHash<int,Double>&endDetExaR,    //期末余额
//       QHash<int,MoneyDirection> &endExaDirR, QHash<int,MoneyDirection> &endDetExaDirR)
//{
//    //第一步：计算总账科目余额
//    Double v;
//    MoneyDirection dir;
//    int fid,sid;
//    QHashIterator<int,Double> cj(curJHpnR);
//    while(cj.hasNext()){
//        cj.next();
//        int key = cj.key();
//        fid = key/10;
//        //确定本期借贷相抵后的借贷方向
//        v = curJHpnR[key] - curDHpnR[key];  //借方 - 贷方
//        if(v > 0)
//            dir = MDIR_J;
//        else if(v < 0){
//            dir = MDIR_D;
//            v.changeSign();
//        }
//        else
//            dir = MDIR_P;

//        if(dir == MDIR_P){ //本期借贷相抵（平）余额值和方向同期初
//            endExaR[key] = preExaR.value(key);
//            endExaDirR[key] = preExaDirR.value(key);
//        }
//        else if(preExaDirR.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
//            endExaR[key] = preExaR.value(key) + v;
//            endExaDirR[key] = preExaDirR.value(key);
//        }
//        else{
//            Double tv;
//            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
//            if(dir == MDIR_J)
//                tv = v - preExaR.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
//            else
//                tv = preExaR.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
//            if(tv > 0){ //余额在借方
//                //如果是收入类科目，要将它固定为贷方
//                if(isInSub(fid)){
//                    tv.changeSign();
//                    endExaR[key] = tv;
//                    endExaDirR[key] = MDIR_D;
//                }
//                else{
//                    endExaR[key] = tv;
//                    endExaDirR[key] = MDIR_J;
//                }
//            }
//            else if(tv < 0){ //余额在贷方
//                //如果是费用类科目，要将它固定为借方
//                if(isFeiSub(fid)){
//                    endExaR[key] = tv;
//                    endExaDirR[key] = MDIR_J;
//                }
//                else{
//                    tv.changeSign();
//                    endExaR[key] = tv;
//                    endExaDirR[key] = MDIR_D;
//                }
//            }
//            else{
//                endExaR[key] = 0;
//                endExaDirR[key] = MDIR_P;
//                //或者可以考虑，将余额值为0的科目从hash表中移除
//            }
//        }
//    }


//    //第二步：计算明细科目余额
//    QHashIterator<int,Double> dj(curJDHpnR);
//    while(dj.hasNext()){
//        dj.next();
//        int key = dj.key();
//        sid = key/10;
//        v = curJDHpnR[key] - curDDHpnR[key];
//        if(v > 0)
//            dir = MDIR_J;
//        else if(v < 0){
//            dir = MDIR_D;
//            v.changeSign();
//        }
//        else
//            dir = MDIR_P;

//        if(dir == DIR_P){ //本期借贷相抵（平）
//            endDetExaR[key] = preDetExaR.value(key);
//            endDetExaDirR[key] = preDetExaDirR.value(key);
//        }
//        else if(preDetExaDirR.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
//            endDetExaR[key] = preDetExaR.value(key) + v;
//            endDetExaDirR[key] = preDetExaDirR.value(key);
//        }
//        else{
//            Double tv;
//            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
//            if(dir == MDIR_J)
//                tv = v - preDetExaR.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
//            else
//                tv = preDetExaR.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
//            if(tv > 0){ //余额在借方
//                //如果是收入类科目，要将它固定为贷方
//                if(isInSSub(sid)){
//                    tv.changeSign();
//                    endDetExaR[key] = tv;
//                    endDetExaDirR[key] = MDIR_D;
//                }
//                else{
//                    endDetExaR[key] = tv;
//                    endDetExaDirR[key] = MDIR_J;
//                }
//            }
//            else if(tv < 0){ //余额在贷方
//                //如果是费用类科目，要将它固定为借方
//                if(isFeiSSub(sid)){
//                    endDetExaR[key] = tv;
//                    endDetExaDirR[key] = MDIR_J;
//                }
//                else{
//                    tv.changeSign();
//                    endDetExaR[key] = tv;
//                    endDetExaDirR[key] = MDIR_D;
//                }

//            }
//            else{
//                endDetExaR[key] = 0;
//                endDetExaDirR[key] = MDIR_P;
//                //或者可以考虑，将余额值为0的科目从hash表中移除
//            }
//        }
//    }
//    //将本期未发生的总账科目余额加入到总账期末余额表中
//    QHashIterator<int,Double> p(preExaR);
//    while(p.hasNext()){
//        p.next();
//        int key = p.key();
//        if(!endExaR.contains(key)){
//            endExaR[key] = preExaR[key];
//            endExaDirR[key] = preExaDirR[key];
//        }
//    }
//    //将本期未发生的明细科目余额加入到明细期末余额表中
//    QHashIterator<int,Double> pd(preDetExaR);
//    while(pd.hasNext()){
//        pd.next();
//        int key = pd.key();
//        if(!endDetExaR.contains(key)){
//            endDetExaR[key] = preDetExaR[key];
//            endDetExaDirR[key] = preDetExaDirR[key];
//        }
//    }
//}



//计算本期末余额
//bool BusiUtil::calCurExtraByMonth(int y,int m,
//  QHash<int,double> preExa, QHash<int,double> preDetExa,     //期初余额
//  QHash<int,int> preExaDir, QHash<int,int> preDetExaDir,     //期初余额方向
//  QHash<int,double> curJHpn, QHash<int,double> curJDHpn,     //当期借方发生额
//  QHash<int,double> curDHpn, QHash<int,double>curDDHpn,      //当期贷方发生额
//  QHash<int,double> &endExa, QHash<int,double>&endDetExa,    //期末余额
//  QHash<int,int> &endExaDir, QHash<int,int> &endDetExaDir)   //期末余额方向
//{
//    //第一步：计算总账科目余额
//    double v;
//    int dir;
//    QHashIterator<int,double> cj(curJHpn);
//    while(cj.hasNext()){
//        cj.next();
//        int key = cj.key();
//        //确定本期借贷相抵后的借贷方向
//        v = curJHpn[key] - curDHpn[key];  //借方 - 贷方
//        if(v > 0)
//            dir = DIR_J;
//        else if(v < 0){
//            dir = DIR_D;
//            v = 0 - v;
//        }
//        else
//            dir = DIR_P;

//        if(dir == DIR_P){ //本期借贷相抵（平）
//            endExa[key] = preExa.value(key);
//            endExaDir[key] = preExaDir.value(key);
//        }
//        else if(preExaDir.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
//            endExa[key] = preExa.value(key) + v;
//            endExaDir[key] = preExaDir.value(key);
//        }
//        else{
//            double tv;
//            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
//            if(dir == DIR_J)
//                tv = v - preExa.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
//            else
//                tv = preExa.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
//            if(tv > 0){ //余额在借方
//                endExa[key] = tv;
//                endExaDir[key] = DIR_J;
//            }
//            else if(tv < 0){ //余额在贷方
//                endExa[key] = 0 - tv;
//                endExaDir[key] = DIR_D;
//            }
//            else{
//                endExa[key] = 0;
//                endExaDir[key] = DIR_P;
//            }
//        }
//    }


//    //第二步：计算明细科目余额
//    QHashIterator<int,double> dj(curJDHpn);
//    while(dj.hasNext()){
//        dj.next();
//        int key = dj.key();
//        v = curJDHpn[key] - curDDHpn[key];
//        if(v > 0)
//            dir = DIR_J;
//        else if(v < 0){
//            dir = DIR_D;
//            v = 0 - v;
//        }
//        else
//            dir = DIR_P;

//        if(dir == DIR_P){ //本期借贷相抵（平）
//            endDetExa[key] = preDetExa.value(key);
//            endDetExaDir[key] = preDetExaDir.value(key);
//        }
//        else if(preDetExaDir.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
//            endDetExa[key] = preDetExa.value(key) + v;
//            endDetExaDir[key] = preDetExaDir.value(key);
//        }
//        else{
//            double tv;
//            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
//            if(dir == DIR_J)
//                tv = v - preDetExa.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
//            else
//                tv = preDetExa.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
//            if(tv > 0){ //余额在借方
//                endDetExa[key] = tv;
//                endDetExaDir[key] = DIR_J;
//            }
//            else if(tv < 0){ //余额在贷方
//                endDetExa[key] = 0 - tv;
//                endDetExaDir[key] = DIR_D;
//            }
//            else{
//                endDetExa[key] = 0;
//                endDetExaDir[key] = DIR_P;
//            }
//        }
//    }
//    //将本期未发生的总账科目余额加入到总账期末余额表中
//    QHashIterator<int,double> p(preExa);
//    while(p.hasNext()){
//        p.next();
//        int key = p.key();
//        if(!endExa.contains(key)){
//            endExa[key] = preExa[key];
//            endExaDir[key] = preExaDir[key];
//        }
//    }
//    //将本期未发生的明细科目余额加入到明细期末余额表中
//    QHashIterator<int,double> pd(preDetExa);
//    while(pd.hasNext()){
//        pd.next();
//        int key = pd.key();
//        if(!endDetExa.contains(key)){
//            endDetExa[key] = preDetExa[key];
//            endDetExaDir[key] = preDetExaDir[key];
//        }
//    }
//}

//bool BusiUtil::calCurExtraByMonth2(int y,int m,
//       QHash<int,Double> preExa, QHash<int,Double> preDetExa,     //期初余额
//       QHash<int,MoneyDirection> preExaDir, QHash<int,MoneyDirection> preDetExaDir,     //期初余额方向
//       QHash<int,Double> curJHpn, QHash<int,Double> curJDHpn,     //当期借方发生额
//       QHash<int,Double> curDHpn, QHash<int,Double>curDDHpn,      //当期贷方发生额
//       QHash<int,Double> &endExa, QHash<int,Double>&endDetExa,    //期末余额
//       QHash<int,MoneyDirection> &endExaDir, QHash<int,MoneyDirection> &endDetExaDir)  //期末余额方向
//{
//    //第一步：计算总账科目余额
//    Double v;
//    MoneyDirection dir;
//    int fid;
//    QHashIterator<int,Double> cj(curJHpn);
//    while(cj.hasNext()){
//        cj.next();
//        int key = cj.key();
//        fid = key/10;
//        //确定本期借贷相抵后的借贷方向
//        v = curJHpn[key] - curDHpn[key];  //借方 - 贷方
//        if(v > 0)
//            dir = MDIR_J;
//        else if(v < 0){
//            dir = MDIR_D;
//            v.changeSign();
//        }
//        else
//            dir = MDIR_P;

//        if(dir == MDIR_P){ //本期借贷相抵（平）余额值和方向同期初
//            endExa[key] = preExa.value(key);
//            endExaDir[key] = preExaDir.value(key);
//        }
//        else if(preExaDir.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
//            endExa[key] = preExa.value(key) + v;
//            endExaDir[key] = preExaDir.value(key);
//        }
//        else{
//            Double tv;
//            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
//            if(dir == MDIR_J)
//                tv = v - preExa.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
//            else
//                tv = preExa.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
//            if(tv > 0){ //余额在借方
//                //如果是收入类科目，要将它固定为贷方
//                if(BusiUtil::isInSub(fid)){
//                    tv.changeSign();
//                    endExa[key] = tv;
//                    endExaDir[key] = MDIR_D;
//                }
//                else{
//                    endExa[key] = tv;
//                    endExaDir[key] = MDIR_J;
//                }

//            }
//            else if(tv < 0){ //余额在贷方
//                //如果是费用类科目，要将它固定为借方
//                if(BusiUtil::isFeiSub(fid)){
//                    //tv.changeSign();
//                    endExa[key] = tv;
//                    endExaDir[key] = MDIR_J;
//                }
//                else{
//                    tv.changeSign();
//                    endExa[key] = tv;
//                    endExaDir[key] = MDIR_D;
//                }

//            }
//            else{
//                endExa[key] = 0;
//                endExaDir[key] = MDIR_P;
//            }
//        }
//    }


//    //第二步：计算明细科目余额
//    QHashIterator<int,Double> dj(curJDHpn);
//    int sid;
//    while(dj.hasNext()){
//        dj.next();
//        int key = dj.key();
//        sid = key/10;
//        v = curJDHpn[key] - curDDHpn[key];
//        if(v > 0)
//            dir = MDIR_J;
//        else if(v < 0){
//            dir = MDIR_D;
//            v.changeSign();
//        }
//        else
//            dir = MDIR_P;

//        if(dir == MDIR_P){ //本期借贷相抵（平）
//            endDetExa[key] = preDetExa.value(key);
//            endDetExaDir[key] = preDetExaDir.value(key);
//        }
//        else if(preDetExaDir.value(key) == dir){ //本期发生额借贷方向与期初相同，则直接加到同一方向
//            endDetExa[key] = preDetExa.value(key) + v;
//            endDetExaDir[key] = preDetExaDir.value(key);
//        }
//        else{
//            Double tv;
//            //始终用借方去减贷方，如果值为正，则余额在借方，值为负，则余额在贷方
//            if(dir == MDIR_J)
//                tv = v - preDetExa.value(key); //借方（当前发生借贷相抵后） - 贷方（期初余额）
//            else
//                tv = preDetExa.value(key) - v; //借方（期初余额） - 贷方（当前发生借贷相抵后）
//            if(tv > 0){ //余额在借方
//                //如果是收入类科目，要将它固定为贷方
//                if(isInSSub(sid)){
//                    tv.changeSign();
//                    endDetExa[key] = tv;
//                    endDetExaDir[key] = MDIR_D;
//                }
//                else{
//                    endDetExa[key] = tv;
//                    endDetExaDir[key] = MDIR_J;
//                }

//            }
//            else if(tv < 0){ //余额在贷方
//                //如果是费用类科目，要将它固定为借方
//                if(isFeiSSub(sid)){
//                    endDetExa[key] = tv;
//                    endDetExaDir[key] = MDIR_J;
//                }
//                else{
//                    tv.changeSign();
//                    endDetExa[key] = tv;
//                    endDetExaDir[key] = MDIR_D;
//                }
//            }
//            else{
//                endDetExa[key] = 0;
//                endDetExaDir[key] = MDIR_P;
//            }
//        }
//    }
//    //将本期未发生的总账科目余额加入到总账期末余额表中
//    QHashIterator<int,Double> p(preExa);
//    while(p.hasNext()){
//        p.next();
//        int key = p.key();
//        if(!endExa.contains(key)){
//            endExa[key] = preExa[key];
//            endExaDir[key] = preExaDir[key];
//        }
//    }
//    //将本期未发生的明细科目余额加入到明细期末余额表中
//    QHashIterator<int,Double> pd(preDetExa);
//    while(pd.hasNext()){
//        pd.next();
//        int key = pd.key();
//        if(!endDetExa.contains(key)){
//            endDetExa[key] = preDetExa[key];
//            endDetExaDir[key] = preDetExaDir[key];
//        }
//    }
//}

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
//bool BusiUtil::calAmountByMonth3(int y, int m, QHash<int, Double> &jSums, QHash<int, Double> &dSums, QHash<int, Double> &sjSums, QHash<int, Double> &sdSums, bool &isSave, int &amount)
//{
//    QSqlQuery q(db),q2(db);
//    QString s;

//    //初始化汇率表
//    QHash<int,Double>rates;
//    if(!getRates2(y,m,rates))
//        return false;
//    rates[RMB] = Double(1.00);

//    //初始化需要进行明细核算的一级科目的id列表
//    //QList<int> detSubs;
//    //if(!getReqDetSubs(detSubs))
//    //    return false;

//    //生成查询语句
//    if(!genStatSql2(y,m,s))
//        return false;

//    if(!q.exec(s)){
//        QMessageBox::information(0, QObject::tr("提示信息"),
//                                 QString(QObject::tr("在计算本期发生额时不能获取凭证集")));
//        return false;
//    }

//    //判定是否可以保存余额
//    PzsState state;
//    getPzsState(y,m,state);
//    isSave = (state == Ps_AllVerified);

//    //遍历凭证集
//    amount = 0;
//    while(q.next()){
//        int pid,fid,sid,ac,mtype;
//        PzClass pzCls; //凭证类别
//        Double jv,dv;  //业务活动的借贷金额
//        int dir;      //业务活动借贷方向

//        pid = q.value(0).toInt(); //凭证id
//        pzCls =(PzClass)q.value(PZ_CLS).toInt();
//        s = QString("select * from BusiActions where pid = %1").arg(pid);
//        if(!q2.exec(s)){
//            QMessageBox::information(0, QObject::tr("提示信息"),
//                                     QString(QObject::tr("在计算本期发生额时不能获取id为%1的凭证的业务活动表")).arg(pid));
//            return false;
//        }
//        amount++;
//        //遍历凭证的业务活动
//        while(q2.next()){
//            int key;
//            fid = q2.value(BACTION_FID).toInt();
//            sid = q2.value(BACTION_SID).toInt();
//            mtype = q2.value(BACTION_MTYPE).toInt();
//            dir = q2.value(BACTION_DIR).toInt();

//            //如果当前凭证是结转汇兑损益类的凭证，则需特别处理
//            //注意：该类凭证的会计分录银行、应收/应付方始终在贷方，财务费用始终在借方，而且
//            //币种为本币，这是一个约定，由生成结转凭证的函数来保证，不能改变
//            //如果要支持多种外币，则必须从会计分录中读取此条结转汇兑损益的会计分录所对应的
//            //外币代码，目前，为了简化，外币只有美元项
//            if(pzClsJzhds.contains(pzCls)/*Pzc_Jzhd_Bank || pzCls == Pzc_Jzhd_Ys || pzCls == Pzc_Jzhd_Yf*/){
//                //continue;
//                if(isAccMt(fid)){  //如果是银行、应收/应付方
//                    dv = Double(q2.value(BACTION_VALUE).toDouble());
//                    key = fid*10+USD;
//                    dSums[key] += dv;
//                    if(!jSums.contains(key))
//                        jSums[key] = Double(0.00);
//                    key = sid*10+USD;
//                    sdSums[key] += dv;
//                    if(!sjSums.contains(key))
//                        sjSums[key] = Double(0.00);
//                }
//                else{ //财务费用方
//                    jv = Double(q2.value(BACTION_VALUE).toDouble());
//                    key = fid*10+mtype;
//                    jSums[key] += jv;
//                    if(!dSums.contains(key))
//                        dSums[key] = Double(0.00);
//                    key = sid*10+mtype;
//                    sjSums[key] += jv;
//                    if(!sdSums.contains(key))
//                        sdSums[key] = Double(0.00);
//                }
//            }
//            //else if(pzCls == Pzc_Jzhd_Ys){

//            //}
//            //else if(pzCls == Pzc_Jzhd_Yf){

//            //}
//            else{
//                if(dir == DIR_J){//发生在借方
//                    jv = Double(q2.value(BACTION_VALUE).toDouble());
//                    if(mtype != RMB)//如果是外币，则将其转换为人民币
//                        jv = jv * rates.value(mtype);
//                    jSums[fid*10+mtype] += jv;
//                    if(!dSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
//                        dSums[fid*10+mtype] = Double(0.00);
//                    //if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
//                    key = sid*10+mtype;
//                    sjSums[key] += jv;
//                    if(!sdSums.contains(key))
//                        sdSums[key] = Double(0.00);
//                    //}
//                }
//                else{
//                    dv = Double(q2.value(BACTION_VALUE).toDouble());
//                    if(mtype != RMB)//如果是外币，则将其转换为人民币
//                        dv = dv * rates.value(mtype);
//                    dSums[fid*10+mtype] += dv;
//                    if(!jSums.contains(fid*10+mtype)) //这是为了确保jSums和dSums的key集合相同
//                        jSums[fid*10+mtype] = Double(0.00);
//                    //if(detSubs.contains(fid)){ //仅对需要进行明细核算的科目进行明细科目的合计计算
//                	key = sid*10+mtype;
//                    sdSums[key] += dv;
//                    if(!sjSums.contains(key))
//                        sjSums[key] = Double(0.00);
//                    //}
//                }
//            }
//        }
//    }
//    return true;
//}


/**
    计算科目各币种合计余额及其方向
    参数  exas：科目余额, exaDirs：科目方向  （这两个参数的键是科目id * 10 +　币种代码）
         sums：科目各币种累计余额，dirs：科目各币种累计余额的方向（这两个参数的键是科目id）
*/
//bool BusiUtil::calSumByMt(QHash<int,double> exas, QHash<int,int>exaDirs,
//                       QHash<int,double>& sums, QHash<int,int>& dirs,
//                       QHash<int,double> rates)
//{
//    QHashIterator<int,double> it(exas);
//    int id,mt;
//    double v;
//    //基本思路是借方　－　贷方，并根据差值的符号来判断余额方向
//    while(it.hasNext()){
//        it.next();
//        id = it.key() / 10;
//        mt = it.key() % 10;
//        v = it.value() * rates.value(mt);
//        //v = QString::number(v,'f',2).toDouble(); //执行四舍五入，规避多一分问题
//        if(exaDirs.value(it.key()) == DIR_P)
//            //continue;
//            sums[id] = 0;
//        else if(exaDirs.value(it.key()) == DIR_J)
//            sums[id] += v;
//        else
//            sums[id] -= v;
//    }
//    QHashIterator<int,double> i(sums);
//    while(i.hasNext()){
//        i.next();
//        //执行四舍五入，规避多一分问题
//        //sums[i.key()] = QString::number(i.value(),'f',2).toDouble();
//        if(i.value() == 0)
//            dirs[i.key()] = DIR_P;
//        else if(i.value() > 0)
//            dirs[i.key()] = DIR_J;
//        else{
//            sums[i.key()] = -sums.value(i.key());
//            dirs[i.key()] = DIR_D;
//        }
//    }
//}

//bool BusiUtil::calSumByMt2(QHash<int,Double> exas, QHash<int,int>exaDirs,
//                           QHash<int,Double>& sums, QHash<int,int>& dirs,
//                           QHash<int,Double> rates)
//{
//    QHashIterator<int,Double> it(exas);
//    int id,mt;
//    Double v;
//    //基本思路是借方　－　贷方，并根据差值的符号来判断余额方向
//    while(it.hasNext()){
//        it.next();
//        id = it.key() / 10;
//        mt = it.key() % 10;
//        if(mt == RMB)
//            v = it.value();
//        else
//            v = it.value() * rates.value(mt);
//        if(exaDirs.value(it.key()) == DIR_P)
//            //continue;
//            sums[id] = 0;
//        else if(exaDirs.value(it.key()) == DIR_J)
//            sums[id] += v;
//        else
//            sums[id] -= v;
//    }
//    QHashIterator<int,Double> i(sums);
//    while(i.hasNext()){
//        i.next();
//        if(i.value() == 0)
//            dirs[i.key()] = DIR_P;
//        else if(i.value() > 0)
//            dirs[i.key()] = DIR_J;
//        else{
//            sums[i.key()].changeSign();
//            dirs[i.key()] = DIR_D;
//        }
//    }
//}

/**
 * @brief BusiUtil::getFidToFldName
 *  获取所有一级科目id到保存科目余额的字段名称的映射
 * @param names
 * @return
 */
bool BusiUtil::getFidToFldName(QHash<int, QString> &names)
{
    QSqlQuery q(db);
    QString s = QString("select id,%1 from %2").arg(fld_fsub_subcode).arg(tbl_fsub);
    char c;
    if(!q.exec(s)){
        QMessageBox::information(0, QObject::tr("提示信息"),
                                 QString(QObject::tr("不能获取一级科目id到保存余额字段名称的映射")));
        return false;
    }
    while(q.next()){
        int id = q.value(0).toInt();
        QString code = q.value(1).toString();
        c = 'A' + code.left(1).toInt() -1;
        names[id] = QString(c).append(code);
    }
}

//获取凭证集内最大的可用凭证号
//int BusiUtil::getMaxPzNum(int y, int m)
//{
//    QSqlQuery q(db);
//    QString s;
//    bool r;

//    int num = 0;
//    QDate d(y,m,1);
//    QString ds = d.toString(Qt::ISODate);
//    ds.chop(3);
//    s = QString("select max(number) from PingZhengs where (pzState != %1) "
//                "and (date like '%2%')").arg(Pzs_Repeal).arg(ds);

//    r = q.exec(s); r = q.first();
//    num = q.value(0).toInt();
//    return ++num;
//}

//读取凭证集状态
bool BusiUtil::getPzsState(int y,int m,PzsState& state)
{
    QSqlQuery q(db);
    if(y==0 && m==0){
        state = Ps_NoOpen;
        return true;
    }
    QString s = QString("select %1 from %2 where (%3=%4) and (%5=%6)")
            .arg(fld_pzss_state).arg(tbl_pzsStates).arg(fld_pzss_year)
            .arg(y).arg(fld_pzss_month).arg(m);
    if(!q.exec(s))
        return false;
    if(!q.first())  //还没有记录，则表示刚开始录入凭证
        state = Ps_Rec;
    else
        state = (PzsState)q.value(0).toInt();
    return true;
}

//设置凭证集状态
bool BusiUtil::setPzsState(int y,int m,PzsState state)
{
    QSqlQuery q(db);
    //首先检测是否存在对应记录
    QString s = QString("select %1 from %2 where (%3=%4) and (%5=%6)")
            .arg(fld_pzss_state).arg(tbl_pzsStates).arg(fld_pzss_year)
            .arg(y).arg(fld_pzss_month).arg(m);
    if(!q.exec(s))
        return false;
    if(q.first()){
        s = QString("update %1 set %2=%3 where (%4=%5) and (%6=%7)")
                .arg(tbl_pzsStates).arg(fld_pzss_state).arg(state)
                .arg(fld_pzss_year).arg(y).arg(fld_pzss_month).arg(m);
        if(!q.exec(s))
            return false;
    }
    else{
        s = QString("insert into %1(%2,%3,%4) values(%5,%6,%7)").arg(tbl_pzsStates)
                .arg(fld_pzss_year).arg(fld_pzss_month).arg(fld_pzss_state)
                .arg(y).arg(m).arg(state);
        if(!q.exec(s))
            return false;
    }
    return true;
}


/**
 * @brief BusiUtil::getOutMtInBank
 *  获取银行存款下所有外币账户对应的明细科目id列表
 * @param ids   明细科目列表
 * @param mt    对应的外币币种代码列表
 * @return
 */
//bool BusiUtil::getOutMtInBank(QList<int>& ids, QList<int>& mt)
//{
//    QSqlQuery q(db);
//    QString s;

//    QHash<QString,int> mts;  //外币名称到币种代码的映射
//    s = QString("select %1,%2 from %3 where %4!=%5").arg(fld_mt_name)
//            .arg(fld_mt_code).arg(tbl_moneyType).arg(fld_mt_code).arg(RMB);
//    if(!q.exec(s))
//        return false;
//    while(q.next())
//        mts[q.value(0).toString()] = q.value(1).toInt();

//    int bankId;
//    if(!getIdByCode(bankId,"1002"))
//        return false;
//    s = QString("select %1.id, %2.%3 from %1 join %2 "
//                "where (%1.%4 = %2.id) and (%1.%5 = %6)")
//            .arg(tbl_ssub).arg(tbl_nameItem).arg(fld_ni_name).arg(fld_ssub_nid)
//            .arg(fld_ssub_fid).arg(bankId);
//    if(!q.exec(s))
//        return false;
//    int id,idx;
//    QString name,mname;
//    while(q.next()){
//        id = q.value(0).toInt();
//        name = q.value(1).toString();
//        idx = name.indexOf("-");
//        if(idx != -1){
//            mname = name.right(name.count() - idx - 1);
//            if(mts.contains(mname)){
//                ids << id;
//                mt << mts.value(mname);
//            }
//        }
//    }
//    return true;
//}

//新建凭证
//bool BusiUtil::crtNewPz(PzData* pz)
//{
//    QSqlQuery q;
//    QString s = QString("insert into %1(%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12) "
//                        "values('%13',%14,%15,%16,%17,%18,%19,%20,%21,%22,%23)").arg(tbl_pz)
//            .arg(fld_pz_date).arg(fld_pz_number).arg(fld_pz_zbnum).arg(fld_pz_jsum)
//            .arg(fld_pz_dsum).arg(fld_pz_class).arg(fld_pz_encnum).arg(fld_pz_state)
//            .arg(fld_pz_vu).arg(fld_pz_ru).arg(fld_pz_bu)
//            .arg(pz->date).arg(pz->pzNum).arg(pz->pzZbNum).arg(pz->jsum)
//            .arg(pz->dsum).arg(pz->pzClass).arg(pz->attNums).arg(pz->state)
//            .arg(pz->verify!=NULL?pz->verify->getUserId():NULL)
//            .arg(pz->producer!=NULL?pz->producer->getUserId():NULL)
//            .arg(pz->bookKeeper!=NULL?pz->bookKeeper->getUserId():NULL);
//    if(!q.exec(s))
//        return false;

//    s = QString("select last_insert_rowid()");
//    if(q.exec(s) && q.first())
//        pz->pzId = q.value(0).toInt();
//    else
//        return false;
//    return true;
//}

//按凭证日期，重新设置凭证集内的凭证号
//bool BusiUtil::assignPzNum(int y, int m)
//{

//}

/**
 * @brief BusiUtil::getSNameForId
 *  按名称条目id获取名称条目的简称和全称
 * @param sid       名称条目id
 * @param name      简称
 * @param lname     全称
 * @return
 */
//bool BusiUtil::getSNameForId(int sid, QString& name, QString& lname)
//{
//    QSqlQuery q;
//    QString s = QString("select %1,%2 from %3 where id = %4")
//            .arg(fld_ni_name).arg(fld_ni_lname).arg(tbl_nameItem).arg(sid);
//    if(!q.exec(s))
//        return false;
//    if(q.first()){
//        name = q.value(0).toString();
//        lname = q.value(1).toString();
//    }
//    return true;
//}

//保存账户信息到账户文件（中的AccountInfos表中）
//bool BusiUtil::saveAccInfo(AccountBriefInfo* accInfo)
//{
//    QSqlQuery q;
//    QString s;

//    s = QString("select id from AccountInfos where code = '%1'")
//            .arg(accInfo->code);
////    if(q.exec(s) && q.first()){
////        int id = q.value(0).toInt();
////        s = QString("update AccountInfos set code='%1',baseTime='%2',usedSubId=%3,"
////                    "sname='%4',lname='%5',lastTime='%6',desc='%7' where id=%8")
////                .arg(accInfo->code).arg(accInfo->baseTime).arg(accInfo->usedSubSys)
////                /*.arg(accInfo->usedRptType)*/.arg(accInfo->accName)
////                .arg(accInfo->accLName).arg(accInfo->lastTime)
////                .arg(accInfo->desc).arg(id);
////    }
////    else{
////        s = QString("insert into AccountInfos(code,baseTime,usedSubId,sname,"
////                    "lname,lastTime,desc) values('%1','%2',%3,'%4','%5','%6','%7')")
////                .arg(accInfo->code).arg(accInfo->baseTime).arg(accInfo->usedSubSys)
////                /*.arg(accInfo->usedRptType)*/.arg(accInfo->accName)
////                .arg(accInfo->accLName).arg(accInfo->lastTime).arg(accInfo->desc);
////    }

//    if(q.exec(s) && q.first()){
//        int id = q.value(0).toInt();
//        s = QString("update AccountInfos set code='%1',"
//                    "sname='%2',lname='%3' where id=%4")
//                .arg(accInfo->code).arg(accInfo->sname)
//                .arg(accInfo->lname).arg(id);
//    }
//    else{
//        s = QString("insert into AccountInfos(code,sname,lname) "
//                    "values('%1','%2','%3')")
//                .arg(accInfo->code).arg(accInfo->sname)
//                .arg(accInfo->lname);
//    }
//    bool r = q.exec(s);
//    return r;
//}

//读取银行帐号
//bool BusiUtil::readAllBankAccont(QHash<int,BankAccount*>& banks)
//{
//    QSqlQuery q;
//    QString s, accNum;
//    int id;

//    QList<int> bankIds,mtIds;
//    QHash<int,bool> isMains;
//    QStringList bankNames,mtNames;
//    s = "select id,IsMain,name from Banks";
//    q.exec(s);
//    while(q.next()){
//        id = q.value(0).toInt();
//        bankIds << id;
//        bankNames << q.value(2).toString();
//        isMains[id] = q.value(1).toBool();
//    }

//    s = "select code,name from MoneyTypes";
//    q.exec(s);
//    while(q.next()){
//        mtIds << q.value(0).toInt();
//        mtNames << q.value(1).toString();
//    }
//    for(int i = 0; i < bankIds.count(); ++i)
//        for(int j = 0; j < mtIds.count(); ++j){
//            s = QString("select accNum from BankAccounts where (bankId=%1) and "
//                        "(mtID=%2)").arg(bankIds[i]).arg(mtIds[j]);
//            if(!q.exec(s))
//                return false;
//            if(!q.first())
//                return true;
//            accNum = q.value(0).toString();
//            s = QString("select %1.id from %1 join %2 on %1.%3 = %2.id where %2.%4='%5'")
//                    .arg(tbl_fsa).arg(tbl_nameItem).arg(fld_fsa_nid).arg(fld_ni_name)
//                    .arg(QString("%1-%2").arg(bankNames[i]).arg(mtNames[j]));
//            if(!q.exec(s))
//                return false;
//            if(q.first()){
//                BankAccount* ba = new BankAccount;
//                ba->accNumber = accNum;
//                ba->bank->isMain = isMains.value(bankIds[i]);
//                ba->mt = mtIds[j];
//                ba->name = QString("%1-%2").arg(bankNames[i]).arg(mtNames[j]);
//                banks[q.value(0).toInt()] = ba;
//            }
//        }
//    return true;
//}


///**
// * @brief BusiUtil::scanPzSetCount
// *  扫描凭证集内的凭证，分别计数各类凭证的数目和总数
// * @param y
// * @param m
// * @param repeal        作废
// * @param recording     录入
// * @param verify        审核
// * @param instat        入账
// * @param amount        总数
// * @return
// */
//bool BusiUtil::scanPzSetCount(int y, int m, int &repeal, int &recording, int &verify, int &instat, int &amount)
//{

//}

/**
 * @brief BusiUtil::inspectJzPzExist
 *  检查指定类别的凭证是否存在、是否齐全
 * @param y
 * @param m
 * @param pzCls 凭证大类别
 * @param count 指定的凭证大类必须具有的凭证数（返回值大于0，则表示凭证不齐全）
 * @return
 */
//bool BusiUtil::inspectJzPzExist(int y, int m, PzdClass pzCls, int &count)
//{
//    QSqlQuery q;
//    QList<PzClass> pzClses;
//    if(pzCls == Pzd_Jzhd)
//        pzClses<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf;
//    else if(pzCls == Pzd_Jzsy)
//        pzClses<<Pzc_JzsyIn<<Pzc_JzsyFei;
//    else if(pzCls == Pzd_Jzlr)
//        pzClses<<Pzc_Jzlr;
//    count = pzClses.count();
//    QString ds = QDate(y,m,1).toString(Qt::ISODate);
//    ds.chop(3);
//    QString s = QString("select %1 from %2 where %3 like '%4%'")
//            .arg(fld_pz_class).arg(tbl_pz).arg(fld_pz_date).arg(ds)/*.arg(fld_pz_number)*/;
//    if(!q.exec(s))
//        return false;
//    PzClass cls;
//    while(q.next()){
//        cls = (PzClass)q.value(0).toInt();
//        if(pzClses.contains(cls)){
//            pzClses.removeOne(cls);
//            count--;
//        }
//        if(count==0)
//            break;
//    }
//    return true;
//}

//生成固定资产折旧凭证
//bool BusiUtil::genGdzcZjPz(int y,int m)
//{

//    return true;
//}

//生成待摊费用凭证
//bool BusiUtil::genDtfyPz(int y,int m)
//{

//    return true;
//}

//引入其他模块产生的凭证
//bool BusiUtil::impPzFromOther(int y,int m, QSet<OtherModCode> mods)
//{
//    PzsState state;
//    getPzsState(y,m,state);
//    if((state != Ps_Stat1) && (state != Ps_Stat3)){
//        QMessageBox::warning(0,QObject::tr("提示信息"),
//            QObject::tr("执行此操作前，必须先统计保存了所有的手工凭证或引入凭证之后"));
//        return false;
//    }

//    if(!mods.empty()){
//        //一个模块的出错，应不影响其他模块的执行
//        bool err = false;
//        if(mods.contains(OM_GDZC) && !genGdzcZjPz(y,m)){
//            err = true;
//            QMessageBox::warning(0,QObject::tr("提示信息"),
//                QObject::tr("在执行引入固定资产折旧凭证时出错！！"));
//        }
//        if(mods.contains(OM_DTFY) && !genDtfyPz(y,m)){
//            err = true;
//            QMessageBox::warning(0,QObject::tr("提示信息"),
//                QObject::tr("在执行引入待摊费用凭证时出错！！"));
//        }
//        //如果所有要引入的模块都未发生错误，则更新凭证集状态
//        if(!err){
//            BusiUtil::setPzsState(y,m,Ps_ImpOther);
//            return true;
//        }
//        else
//            return false;
//    }
//    return true;
//}

//取消引入的由其他模块产生的凭证
//bool BusiUtil::antiImpPzFromOther(int y, int m, QSet<OtherModCode> mods)
//{
//    PzsState state;
//    getPzsState(y,m,state);
//    if((state < Ps_ImpOther) && (state >= Ps_JzsyPre)){
//        QMessageBox::warning(0,QObject::tr("操作拒绝"),
//            QObject::tr("执行此操作前，必须已经引入了由其他模块产生的凭证，并且未执行损益类科目的结转！"));
//        return false;
//    }
//    if(!mods.empty()){
//        bool err = false;
//        if(mods.contains(OM_GDZC) && !antiGdzcPz(y,m)){
//            err = true;
//            QMessageBox::warning(0,QObject::tr("提示信息"),
//                QObject::tr("在执行取消引入的固定资产折旧凭证时出错！！"));
//        }
//        if(mods.contains(OM_DTFY) && !antiDtfyPz(y,m)){
//            err = true;
//            QMessageBox::warning(0,QObject::tr("提示信息"),
//                QObject::tr("在执行引入取消引入的待摊费用凭证时出错！！"));
//        }
//        //如果所有要引入的模块都未发生错误，则更新凭证集状态
//        if(!err){
//            bool req;
//            BusiUtil::reqGenJzHdsyPz(y,m,req);
//            if(req) //如果还没有进行汇兑损益的结转，则置新状态为进行结转汇兑损益前的状态
//                BusiUtil::setPzsState(y,m,Ps_Stat1);
//            else
//                BusiUtil::setPzsState(y,m,Ps_Stat3);
//            return true;
//        }
//        else
//            return false;
//    }
//    return true;
//}

//取消固定资产管理模块引入的凭证
//bool BusiUtil::antiGdzcPz(int y, int m)
//{
//    return true;
//}

//取消待摊费用管理模块引入的凭证
//bool BusiUtil::antiDtfyPz(int y, int m)
//{
//    return true;
//}

//是否是由其他模块引入的凭证类别
//bool BusiUtil::isImpPzCls(PzClass pzc)
//{
//    return impPzCls.contains(pzc);
//}
//是否是由系统自动产生的结转汇兑损益凭证类别
//bool BusiUtil::isJzhdPzCls(PzClass pzc)
//{
//    return impPzCls.contains(pzc);
//}
//是否是由系统自动产生的结转损益凭证类别
//bool BusiUtil::isJzsyPzCls(PzClass pzc)
//{
//    return impPzCls.contains(pzc);
//}
//是否是其他由系统产生并允许用户修改的凭证类别
//bool BusiUtil::isOtherPzCls(PzClass pzc)
//{
//    return impPzCls.contains(pzc);
//}

//判断其他模块是否需要在指定年月产生引入凭证，如果是，则置req为true
//bool BusiUtil::reqGenImpOthPz(int y,int m, bool& req)
//{
//    //任一模块需要产生引入凭证则置req为true，目前仅考虑固定资产和待摊费用
//    if(reqGenGdzcPz(y,m,req) && reqGenDtfyPz(y,m,req))
//        return true;
//    else
//        return false;
//}

//判断固定资产管理模块是否需要产生凭证
//bool BusiUtil::reqGenGdzcPz(int y,int m, bool& req)
//{
//    //只有在根据当前的固定资产折旧情况，需要进行折旧，且凭证集内不存在相关凭证时，则置req为true
//    req = false;
//    return true;
//}

//判断待摊费用管理模块是否需要产生凭证
//bool BusiUtil::reqGenDtfyPz(int y,int m, bool& req)
//{
//    //只有在根据当前待摊费用的扣除情况，需要进行摊销，且凭证集内不存在相关凭证时，则置req为true
//    req = false;
//    return true;
//}

////判断是否需要结转汇兑损益，进行汇兑损益的结转必须有两个前提
////1、当前月与下一月份之间的汇率差不为0，2、相关科目存在外币余额，且凭证集内无对应凭证）
//bool BusiUtil::reqGenJzHdsyPz(int y, int m, bool& req)
//{
//    QSqlQuery q;
//    QHash<int,double> r1,r2;
//    QString s;
//    //bool req2 = false;
//    bool exsit = true;   //3个科目中存在外币余额的科目是否在凭证集内存在相应结转汇兑损益凭证
//                         //只要有一个科目有外币余额，且不存在结转凭证，即为false。
//    bool rateDiff = false;  //汇率差是否为0标记，默认为0。

//    //结转汇兑损益有3种类型的凭证（银行、应收/应付）缺一不可（当然，还要考虑是否存在外币余额，）
//    //只有余额不为0，才有必要执行结转汇兑损益操作
//    int i = 0;
//    QList<int> ids;
//    ids<<0<<0<<0;
//    getIdByCode(ids[0],"1002");//银行存款
//    getIdByCode(ids[1],"1131");//应收账款
//    getIdByCode(ids[2],"2121");//应付账款
//    QHash<int,double> extras;
//    QHash<int,int> dirs;
//    int pzCls = 0;
//    while((i<3) && exsit){
//        readExtraForSub(y,m,ids[i],extras,dirs);
//        if(extras.empty())
//            i++;
//        else if((extras.count() == 1) && extras.keys()[0] == RMB) //如果只有人民币余额
//            i++;
//        else{
//            i++;
//            if(i==0)
//                pzCls = Pzc_Jzhd_Bank;
//            else if(i == 1)
//                pzCls = Pzc_Jzhd_Ys;
//            else
//                pzCls = Pzc_Jzhd_Yf;
//            s = QString("select id from PingZhengs where isForward = %1").arg(pzCls);
//            bool r = q.exec(s);
//            if(r && !q.first()){
//                exsit = false;
//                break;
//            }
//        }
//    }

//    int yy,mm;
//    if(m == 12){
//        yy = y + 1;
//        mm = 1;
//    }
//    else{
//        yy = y;
//        mm = m + 1;
//    }
//    //如果汇率读取错误，则返回假
//    if(!getRates(y,m,r1) || (!getRates(yy,mm,r2)))
//        return false;
//    //如果汇率设置不完整，则有可能需要进行结转
//    if(r1.count() != r2.count()){
//        rateDiff = true;
//    }
//    QHashIterator<int,double> it(r1);
//    while(it.hasNext()){
//        it.next();
//        //如果汇率设置不完整，则有可能需要进行结转
//        if(!r2.contains(it.key())){
//            rateDiff = true;
//        }
//        //如果有汇率差，就必须进行结转
//        else if(it.value() != r2.value(it.key())){
//            rateDiff = true;
//        }
//    }

//    req = rateDiff && !exsit;
//    return true;
//}


//获取指定范围的科目id列表
//参数sfid，ssid代表开始的一级、二级科目id，efid、esid代表结束的一级、二级科目id
//fids和sids是返回的指定范围的一二级科目id列表
//bool BusiUtil::getSubRange(int sfid,int ssid,int efid,int esid,
//                           QList<int>& fids,QHash<int,QList<int> >& sids)
//{
//    QSqlQuery q;
//    QString s;
//    bool r;

//    //获取开始和结束一级科目的科目代码
//    QString sfcode,efcode;
//    s = QString("select %1 from %2 where id=%3").arg(fld_fsub_subcode).arg(tbl_fsub).arg(sfid);
//    if(!q.exec(s) || !q.first()){
//        qDebug() << QString("error! while get first subject(id:%1)").arg(sfid);
//        return false;
//    }
//    sfcode = q.value(0).toString();
//    s = QString("select %1 from %2 where id=%1").arg(fld_fsub_subcode).arg(tbl_fsub).arg(efid);
//    if(!q.exec(s) || !q.first()){
//        qDebug() << QString("error! while get first subject(id:%1)").arg(efid);
//        return false;
//    }
//    efcode = q.value(0).toString();

//    //获取一级科目的范围
//    s = QString("select id from %1 where %2>=%3 and %2<=%4) ")
//            .arg(tbl_fsub).arg(fld_fsub_subcode).arg(sfcode).arg(efcode);
//    q.exec(s);
//    while(q.next()){
//        fids<<q.value(0).toInt();
//    }


//    //获取二级科目的范围
//    QList<int> ids;
//    QList<QString> name;
//    for(int i = 0; i < fids.count(); ++i){
//        getSndSubInSpecFst(fids[i],ids,name);
//        //开始或结束的一级科目，在范围选择模式时，可能只选择了部分二级科目，因此必须特别处理
//        //但对于其他选择模式，则隐含选择所有
//        if((sfid == fids[i]) && (ssid != 0)){
//            for(int j = 0; j < ids.count(); ++j){
//                if((ids[j] >= ssid) && (ids[j] <= esid))
//                    sids[fids[i]]<<ids[j];
//            }
//        }
//        else if((efid == fids[i]) && (esid != 0)){
//            for(int j = 0; j < ids.count(); ++j){
//                if((ids[j] >= ssid) && (ids[j] <= esid))
//                    sids[fids[i]]<<ids[j];
//            }
//        }
//        else
//            sids[fids[i]] = ids;
//    }
//}

//指定id的凭证是否处于指定的年月内
//bool BusiUtil::isPzInMonth(int y, int m, int pzid, bool& isIn)
//{
//    QSqlQuery q;
//    QString s;
//    bool r;

//    s = QString("select date from PingZhengs where id = %1").arg(pzid);
//    if(!q.exec(s) || !q.first())
//        return false;
//    s = q.value(0).toString();
//    int year = s.left(4).toInt();
//    int month = s.mid(5,2).toInt();
//    if((year == y) && (month == m))
//        isIn = true;
//    else
//        isIn = false;
//    return true;
//}

//获取指定一级科目是否需要按币种进行分别核算
bool BusiUtil::isAccMt(int fid)
{
    return accToMt.contains(fid);
}

//获取指定二级科目是否需要按币种进行分别核算
bool BusiUtil::isAccMtS(int sid)
{
    QSqlQuery q;
    QString s = QString("select %1 from %2 where id=%3")
            .arg(fld_ssub_fid).arg(tbl_ssub).arg(sid);
    if(!q.exec(s) || !q.first())
        return false;
    else{
        int fid = q.value(0).toInt();
        return isAccMt(fid);
    }
}

/**
 * @brief BusiUtil::getSpecClsPzCode
 *  获取指定大类凭证的类别代码列表
 * @param cls
 * @return
 */
//QList<PzClass> BusiUtil::getSpecClsPzCode(PzdClass cls)
//{
//    QList<PzClass> codes;
//    if(cls == Pzd_Jzhd)
//        codes<<Pzc_Jzhd_Bank<<Pzc_Jzhd_Ys<<Pzc_Jzhd_Yf;
//    else if(cls == Pzd_Jzsy)
//        codes<<Pzc_JzsyIn<<Pzc_JzsyFei;
//    else if(cls == Pzd_Jzlr)
//        codes<<Pzc_Jzlr;
//    return codes;
//}

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
//bool VariousUtils::getSubWinInfo(int winEnum, SubWindowDim*& info, QByteArray*& otherInfo)
//{

//}

//保存字窗口信息
//bool VariousUtils::saveSubWinInfo(int winEnum, SubWindowDim* info, QByteArray* otherInfo)
//{

//}

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

//因为原先表示方向的是整形宏定义常量，现在用枚举常量来代替，因此提供这些转换函数
void transferDirection(const QHash<int,int> &sd, QHash<int,MoneyDirection>& dd)
{
    QHashIterator<int,int> it(sd);
    while(it.hasNext()){
        it.next();
        dd[it.key()] = (MoneyDirection)it.value();
    }
}

void transferAntiDirection(const QHash<int, MoneyDirection> &sd, QHash<int, int> &dd)
{
    QHashIterator<int,MoneyDirection> it(sd);
    while(it.hasNext()){
        it.next();
        dd[it.key()] = it.value();
    }
}


//////////////////////////////BackupUtil/////////////////////////////////
BackupUtil::BackupUtil(QString srcDir, QString bacDir)
{
    if(srcDir.isEmpty())
        sorDir.setPath(DatabasePath);
    else
        sorDir.setPath(srcDir);
    if(bacDir.isEmpty())
        backDir.setPath(BackupPath);
    else
        backDir.setPath(bacDir);
    _loadBackupFiles();
}

/**
 * @brief BackupUtil::backup
 * @param fileName      账户文件名（不包含路径）
 * @param reason        备份缘由
 * @param backFileName  对应的备份文件名
 * @return
 */
bool BackupUtil::backup(QString fileName, BackupUtil::BackupReason reason)
{
    QString sn = sorDir.absolutePath()+QDir::separator() + fileName;
    QString timeTag = QDateTime::currentDateTime().toString(Qt::ISODate);
    timeTag.replace(":","-");
    fileName = _cutSuffix(fileName);
    QString dn = QString("%1%2_%3_%4.bak").arg(backDir.absolutePath()+QDir::separator())
            .arg(fileName).arg(_getReasonTag(reason)).arg(timeTag);
    if(!QFile::copy(sn,dn))
        return false;
    files<<dn;
    stk_sor.push(sn);
    stk_back.push(dn);
    return true;
}

/**
 * @brief BackupUtil::restore
 *  恢复最近备份的文件
 * @param error
 * @return
 */
bool BackupUtil::restore(QString &error)
{
    if(stk_back.isEmpty() || stk_sor.isEmpty()){
        error = "Don't exist restorable file!";
        return false;
    }
    QString srcFile = stk_sor.pop();
    QString backFile = stk_back.pop();
    if(!_copyFile(backFile,srcFile)){
        error = "File restore failed on copy!";
        return false;
    }
    return true;
}

/**
 * @brief BackupUtil::restore
 *  恢复指定文件的最近备份版本
 * @param fileName  原始文件名（不带路径）
 * @param reason    备份缘由
 * @param error     出错信息
 * @return
 */
bool BackupUtil::restore(QString fileName, BackupReason reason, QString &error)
{
    int index = _fondLastFile(fileName,reason);
    if(index == -1){
        error = "Backup file not fonded!";
        return false;
    }
    QString sn = stk_sor.at(index);
    QString bn = stk_back.at(index);
    stk_sor.remove(index);
    stk_back.remove(index);
    if(!_copyFile(bn,sn)){
        error = "File retore failed on copy!";
        return false;
    }
    return true;
}

/**
 * @brief BackupUtil::fondLastFile
 *  查找与指定文件和备份缘由匹配的最新备份文件在堆栈中的序号
 * @param fileName  原始文件名（不带路径）
 * @param reason
 * @return
 */
int BackupUtil::_fondLastFile(QString fileName, BackupUtil::BackupReason reason)
{
    if(stk_back.isEmpty() || stk_sor.isEmpty())
        return -1;
    QString fn = _cutSuffix(fileName);
    fn = QString("%1_%2").arg(fn).arg(_getReasonTag(reason));
    for(int i = stk_back.count()-1; i >=0; i--){
        QString bname = stk_back.at(i);
        if(bname.contains(fn))
            return i;
    }
    return -1;
}

/**
 * @brief BackupUtil::clear
 *  删除备份目录下的所有文件
 */
void BackupUtil::clear()
{
    QFileInfoList filelist = backDir.entryInfoList(QDir::Files,QDir::Name);
    foreach(QFileInfo finfo, filelist){
        QString fileName = finfo.fileName();
        backDir.remove(fileName);
    }
    files.clear();
}

void BackupUtil::setBackupDirectory(QString path)
{
    backDir.setPath(path);
    _loadBackupFiles();
}

void BackupUtil::setSourceDirectory(QString path)
{
    sorDir.setPath(path);
}

/**
 * @brief BackupUtil::_loadBackupFiles
 *  装载备份目录下的所有备份文件名到列表files
 */
void BackupUtil::_loadBackupFiles()
{
    if(!files.isEmpty())
        files.clear();
    QStringList filters;
    filters << "*.bak";
    backDir.setNameFilters(filters);
    QFileInfoList filelist = backDir.entryInfoList(filters, QDir::Files);
    foreach(QFileInfo finfo, filelist){
        QString fileName = finfo.absoluteFilePath();
        files<<fileName;
    }
    if(!files.isEmpty())
        files.sort();
}

/**
 * @brief BackupUtil::_getReasonTag
 *  返回表示备份缘由的字符串
 * @param reason
 * @return
 */
QString BackupUtil::_getReasonTag(BackupUtil::BackupReason reason)
{
    if(reason == BR_UPGRADE)
        return "UP";
    else
        return "TR";
}

/**
 * @brief BackupUtil::_copyFile
 *  为恢复文件而执行拷贝操作
 * @param sn
 * @param dn
 * @return
 */
bool BackupUtil::_copyFile(QString sn, QString dn)
{
    QString filename = QFileInfo(dn).fileName();
    if(sorDir.exists(filename))
        sorDir.remove(dn);
    return QFile::copy(sn,dn);
}

/**
 * @brief BackupUtil::_cutSuffix
 *  截去文件名中的后缀名
 * @param fileName
 * @return
 */
QString BackupUtil::_cutSuffix(QString fileName)
{

    int idx = fileName.lastIndexOf('.');
    if(idx == -1)
        return fileName;
    else{
        QString fname = fileName.left(idx);
        return fname;
    }
}
