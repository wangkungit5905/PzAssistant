#ifndef STATUTIL_H
#define STATUTIL_H

#include <QList>

#include "commdatastruct.h"

class PingZheng;
class BusiAction;
class Account;
class DbUtil;
class SubjectManager;
class SubjectBase;
class FirstSubject;
class SecondSubject;
class AccountSuiteManager;


/**
 * @brief The StatUtil class
 *  负债对凭证集进行统计
 */
class StatUtil : public QObject
{
    Q_OBJECT

    const Double zv= Double(0.01); //某些情况下，外币科目在统计后，其原币值为0，这往往由于计算方式的不同而出现的误差
                                   //（将外币的余额直接乘以汇率得到的本币值，与每次将外币发生额转换为本币后在参予计算而得的结果往往有几分的差距）
                                   //但误差是否会超出1分呢？

public:
    StatUtil(QList<PingZheng *> *pzs, AccountSuiteManager* parent);
    void setPzSet(QList<PingZheng *> *pzs){this->pzs=pzs;}
    bool stat();
    bool save();
    void clear();
    Account* getAccount(){return account;}
    SubjectManager* getSubjectManager(){return smg;}
    int year(){return y;}
    int month(){return m;}
    int count(){return pzs->count();}

    //获取期初值和方向
    QHash<int,Double>& getPreValueFPm(){return preFExa;}
    QHash<int,Double>& getPreValueFMm(){return preFExaM;}
    QHash<int,Double>& getPreValueSPm(){return preSExa;}
    QHash<int,Double>& getPreValueSMm(){return preSExaM;}
    QHash<int,MoneyDirection>& getPreDirF(){return preFDir;}
    QHash<int,MoneyDirection>& getPreDirS(){return preSDir;}
    QHash<int,Double>& getSumPreValueF(){return sumPreFV;}
    QHash<int,Double>& getSumPreValueS(){return sumPreSV;}
    QHash<int,MoneyDirection>& getSumPreDirF(){return sumPreFD;}
    QHash<int,MoneyDirection>& getSumPreDirS(){return sumPreSD;}

    //获取本期发生额
    QHash<int,Double>& getCurValueJFPm(){return curJF;}
    QHash<int,Double>& getCurValueJFMm(){return curJFM;}
    QHash<int,Double>& getCurValueDFPm(){return curDF;}
    QHash<int,Double>& getCurValueDFMm(){return curDFM;}
    QHash<int,Double>& getCurValueJSPm(){return curJS;}
    QHash<int,Double>& getCurValueJSMm(){return curJSM;}
    QHash<int,Double>& getCurValueDSPm(){return curDS;}
    QHash<int,Double>& getCurValueDSMm(){return curDSM;}
    QHash<int,Double>& getSumCurValueJF(){return sumCurJF;}
    QHash<int,Double>& getSumCurValueJS(){return sumCurJS;}
    QHash<int,Double>& getSumCurValueDF(){return sumCurDF;}
    QHash<int,Double>& getSumCurValueDS(){return sumCurDS;}

   //获取期末余额及其方向
    QHash<int,Double>& getEndValueFPm(){return endFExa;}
    QHash<int,Double>& getEndValueFMm(){return endFExaM;}
    QHash<int,Double>& getEndValueSPm(){return endSExa;}
    QHash<int,Double>& getEndValueSMm(){return endSExaM;}
    QHash<int,MoneyDirection>& getEndDirF(){return endFDir;}
    QHash<int,MoneyDirection>& getEndDirS(){return endSDir;}
    QHash<int,Double>& getSumEndValueF(){return sumEndFV;}
    QHash<int,Double>& getSumEndValueS(){return sumEndSV;}
    QHash<int,MoneyDirection>& getSumEndDirF(){return sumEndFD;}
    QHash<int,MoneyDirection>& getSumEndDirS(){return sumEndSD;}

private slots:
    void addOrDelBa(BusiAction* ba, bool add); //增加或移除分录
    void subChangedOnBa(BusiAction* ba, FirstSubject* oldFSub, SecondSubject* oldSSub, Money* oldMt, Double oldValue); //分录的科目设置改变    
    void valueChangedOnBa(BusiAction* ba, Money* oldMt,Double &oldValue,MoneyDirection oldDir);  //分录的值改变    
signals:
    void extraException(BusiAction* ba,Double fv, MoneyDirection fd, Double sv, MoneyDirection sd);//f,s表示一二级科目，v,d表示余额和方向
private:
    void _adjustExtra(FirstSubject* fsub, SecondSubject* ssub, Money* mt,Double v, MoneyDirection dir, bool add=true);
    bool _baIsValid(BusiAction* ba);
    void _clearDatas();
    bool _statCurHappen();
    bool _readPreExtra();
    void _calEndExtra(bool isFst=true);
    void _calEndExtraForSingleSub(SubjectBase* sub, Money *mt, bool isFst=true);
    void _inspectExtraException(BusiAction* ba);
    void _calSumValue(bool isPre, bool isfst=true);    
    void _calCurSumValue(bool isJ, bool isFst=true);
    void _removeExtraItem(int key_f, int key_s);
    bool _verifyExtraUnity(QStringList &errors, bool isPre = true);
    void _collectSumForFSubMt(FirstSubject* fsub, int mt, Double& sum, MoneyDirection& dir,QHash<int,Double> vs, QHash<int,MoneyDirection>dirs);

    Account* account;
    DbUtil* dbUtil;
    AccountSuiteManager* sm;
    SubjectManager* smg;
    QList<PingZheng*>* pzs; //凭证对象集合
    int y,m;                //凭证集所属年月
    Money* masterMt;
    QHash<int,Double> rates;


    //命名约定：J：借方，D：贷方，F：一级科目，S：二级科目，M：表示以本币计
    //pre：期初，end：期末，cur：本期发生，Exa：余额，Dir：访问
    //注意：这些hash表的键是 “科目代码 X 10 + 币种代码”
    QHash<int,Double> preFExa, preSExa;                     //期初余额（以原币计）
    QHash<int,Double> preFExaM, preSExaM;                   //期初余额（以本币计）
    QHash<int,MoneyDirection> preFDir,preSDir;        //期初余额方向（以原币计）
    //QHash<int,MoneyDirection> preExaDirR,preDetExaDirR;     //期初余额方向（以本币计）
    QHash<int,Double> curJF, curJS, curDF, curDS;  //当期借贷发生额（以原币计）
    QHash<int,Double> curJFM,curJSM,curDFM,curDSM; //当期借贷发生额（以本币计）（仅用来保存涉及到外币的发生额）
    QHash<int,Double> endFExa, endSExa;                     //期末余额（以原币计）
    QHash<int,Double> endFExaM, endSExaM;                   //期末余额（以本币计）
    QHash<int,MoneyDirection> endFDir,endSDir;        //期末余额方向（以原币计）
    //QHash<int,MoneyDirection> endExaDirR,endDetExaDirR;      //期末余额方向（以本币计）

    //这些是各科目在各币种合计后的余额及其方向（这些hash表的键是科目代码）
    QHash<int,Double> sumPreFV,sumPreSV; //
    QHash<int,Double> sumCurJF,sumCurDF,sumCurJS,sumCurDS; //
    QHash<int,Double> sumEndFV,sumEndSV; //
    QHash<int,MoneyDirection> sumPreFD,sumPreSD,sumEndFD,sumEndSD;

};

#endif // STATUTIL_H
