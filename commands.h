#ifndef COMMANDS_H
#define COMMANDS_H


#include <QUndoCommand>
#include <QHash>

#include "commdatastruct.h"
#include "cal.h"

#define CMD_PZDATE   1
#define CMD_PZZNUM   2
#define CMD_PZENCNUM 3
#define CMD_PZVSTATE 4
#define CMD_PZVUSER  5
#define CMD_PZBUSER  6

#define CMD_CRTBBA   20
#define CMD_BA_SUMMARY 21
#define CMD_BA_FSTSUB  22
#define CMD_BA_SNDSUB  23
#define CMD_BA_MT      24
#define CMD_BA_VALUE   25
#define CMD_BA_REMOVE  26
#define CMD_BA_MOVE    27
#define CMD_BA_CUT     28

class SubjectManager;
class SubjectNameItem;
class AccountSuiteManager;
class PingZheng;
class User;
class BusiAction;
class FirstSubject;
class SecondSubject;
class Money;

//1、添加凭证
//2、插入凭证
//3、删除凭证

/**
 * @brief The CrtPzCommand class
 * 添加凭证（为了使命令正确显示添加的凭证号，添加前，必须设置凭证号）
 */
class AppendPzCmd : public QUndoCommand
{
public:
    AppendPzCmd(AccountSuiteManager* pm, PingZheng* pz, QUndoCommand *parent = 0);
    void undo();
    void redo();

private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;
};

/**
 * @brief The InsertPzCmd class
 * 插入凭证（为了使命令正确显示添加的凭证号，插入前，必须设置凭证号）
 */
class InsertPzCmd : public QUndoCommand
{
public:
    InsertPzCmd(AccountSuiteManager* pm, PingZheng* pz, QUndoCommand *parent = 0);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;
};

/**
 * @brief The DelPzCommand class
 * 删除凭证
 */
class DelPzCmd : public QUndoCommand
{
public:
    DelPzCmd(AccountSuiteManager* pm, PingZheng *pz, QUndoCommand *parent = 0);
    void undo();
    void redo();

private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;   //凭证对象
};

//class DelMultiPzCmd : public QUndoCommand
//{
//public:
//    DelMultiPzCmd(PzSetMgr* pm, QList<PingZheng*> pzLst, QUndoCommand *parent = 0);
//    void undo();
//    void redo();

//private:
//    PzSetMgr* pm;     //凭证集
//    QList<PingZheng*> pzLst;   //要删除的凭证对象
//};

//1、修改凭证的日期
//2、批量修改凭证的号码（当按凭证日期或自编号顺序重置凭证号时调用）
//3、修改凭证的自编号
//4、修改凭证的附件数
//5、修改凭证的审核状态（录入态-审核态-记账态）
//6、修改凭证的审核用户（对于凭证的录入者，在创建凭证的初始阶段赋于，因此也不用为此创建一个命令）
//7、修改凭证的记账用户
//8、修改凭证的借贷合计值（对于这个命令，要进一步斟酌，因为借贷合计值的更改是由于用户对凭证内的会计分录的修改造成的）

/**
 * @brief The ModifyPzDateCommand class
 * 修改凭证的日期
 */
class ModifyPzDateCmd : public QUndoCommand
{
public:
    ModifyPzDateCmd(AccountSuiteManager* pm, PingZheng* pz, QString ds, QUndoCommand *parent = 0);
    int	id() const{return CMD_PZDATE;}
    bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();

private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    QString oldDate;
    QString newDate;
};

/**
 * @brief The BatchModifyPzNum class
 * 批量修改凭证的号码
 */
class BatchModifyPzNumCmd : public QUndoCommand
{

};

/**
 * @brief The ModifyPzZNumCmd class
 * 修改凭证的自编号
 */
class ModifyPzZNumCmd : public QUndoCommand
{
public:
    ModifyPzZNumCmd(AccountSuiteManager* pm, PingZheng* pz, int pnum, QUndoCommand* parent = 0);
    int	id() const{return CMD_PZZNUM;}
    bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();

private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    int oldPNum;
    int newPNum;
};

/**
 * @brief The ModifyPzEncNumCmd class
 * 修改凭证的附件数
 */
class ModifyPzEncNumCmd : public QUndoCommand
{
public:
    ModifyPzEncNumCmd(AccountSuiteManager* pm, PingZheng* pz, int encnum, QUndoCommand* parent = 0);
    int	id() const{return CMD_PZENCNUM;}
    bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    int oldENum;
    int newENum;
};

/**
 * @brief The ModifyPzVUserCmd class
 * 修改凭证的审核状态
 */
class ModifyPzVStateCmd : public QUndoCommand
{
public:
    ModifyPzVStateCmd(AccountSuiteManager* pm, PingZheng* pz, PzState state, QUndoCommand* parent = 0);
    int	id() const{return CMD_PZVSTATE;}
    bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    PzState oldState;
    PzState newState;
    //QHash<PzState,QString> stateNames;  //凭证状态名表
};

/**
 * @brief The ModifyPzVUserCmd class
 * 修改凭证的审核用户
 */
class ModifyPzVUserCmd : public QUndoCommand
{
public:
    ModifyPzVUserCmd(AccountSuiteManager* pm, PingZheng* pz, User* user, QUndoCommand* parent = 0);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    User* oldUser;
    User* newUser;
};

/**
 * @brief The ModifyPzBUserCmd class
 * 修改凭证的记账用户
 */
class ModifyPzBUserCmd : public QUndoCommand
{
public:
    ModifyPzBUserCmd(AccountSuiteManager* pm, PingZheng* pz, User* user, QUndoCommand* parent = 0);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    User* oldUser;
    User* newUser;
};

//1、添加会计分录
//2、插入会计分录
//3、修改会计分录的摘要
//4、修改会计分录的一级科目
//5、修改会计分录的二级科目
//6、修改会计分录的币种
//7、修改会计分录的借贷值（方向和值）
//8、删除会计分录
//9、移动会计分录

/**
 * @brief The CrtBlankBaCmd class
 * 添加会计分录
 */
class AppendBaCmd : public QUndoCommand
{
public:
    AppendBaCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, QUndoCommand* parent = 0);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
};

/**
 * @brief The InsertBaCmd class
 * 插入会计分录
 */
class InsertBaCmd : public QUndoCommand
{
public:
    InsertBaCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, int row, QUndoCommand* parent = 0);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    int row;          //插入位置
};

/**
 * @brief The ModifyBaSummaryCmd class
 * 修改会计分录的摘要
 */
class ModifyBaSummaryCmd : public QUndoCommand
{
public:
    ModifyBaSummaryCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, QString summary, QUndoCommand* parent = 0);
    int	id() const{return CMD_BA_SUMMARY;}
    bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    QString newSummary;
    QString oldSummary;
};

class ModifyBaFSubMmd : public QUndoCommand
{
public:
    ModifyBaFSubMmd(QString text, AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, QUndoStack* stack, QUndoCommand* parent = 0);
    int	id() const{return CMD_BA_FSTSUB;}
    bool mergeWith(const QUndoCommand* command);
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    QUndoStack* pstack;
};

/**
 * @brief The ModifyBaFSubCmd class
 * 修改会计分录的一级科目
 */
class ModifyBaFSubCmd : public QUndoCommand
{
public:
    ModifyBaFSubCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, FirstSubject* fsub, QUndoCommand* parent = 0);
    //int	id() const{return CMD_BA_FSTSUB;}
    //bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    FirstSubject* newFSub;
    FirstSubject* oldFSub;

    friend class ModifyBaFSubMmd;
};

/**
 * @brief The ModifyBaSSubCmd class
 * 修改会计分录的二级科目
 */
class ModifyBaSSubCmd : public QUndoCommand
{
public:
    ModifyBaSSubCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, SecondSubject* ssub, QUndoCommand* parent = 0);
    //int	id() const{return CMD_BA_SNDSUB;}
    //bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();
    void setsub(SecondSubject* sub){newSSub=sub;} //此函数应用于宏命令中
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    SecondSubject* newSSub;
    SecondSubject* oldSSub;
};

///**
// * @brief The ModifyBaMtCmd class
// * 修改会计分录的币种
// */
//class ModifyBaMtCmd : public QUndoCommand
//{
//public:
//    ModifyBaMtCmd(PzSetMgr* pm, PingZheng* pz, BusiAction* ba, bool dir, Money* mt, Double rate, QUndoStack* stack, QUndoCommand* parent = 0);
//    int	id() const{return CMD_BA_MT;}
//    bool mergeWith(const QUndoCommand* command);
//    void undo();
//    void redo();
//private:
//    PzSetMgr* pm;     //凭证集
//    PingZheng* pz;    //凭证对象
//    BusiAction* ba;
//    Money* newMt;
//    Money* oldMt;
//    Double rate;      //汇率
//    Double oldValue;
//    bool dir;         //汇率计算方向（true：外币转为本币，false：本币转为外币）
//    QUndoStack* pstack;
//};

/**
 * @brief The ModifyBaMtMmd class
 * 更改会计分录的币种的宏命令（包括设置币种命令，并按需调整金额命令）
 */
class ModifyBaMtMmd : public QUndoCommand
{
public:
    ModifyBaMtMmd(QString text, AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, QUndoStack* stack, QUndoCommand* parent = 0);
    int	id() const{return CMD_BA_VALUE;}
    bool mergeWith(const QUndoCommand* command);
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    QUndoStack* pstack;
};

/**
 * @brief The ModifyBaMtCmd class
 * 修改会计分录的币种命令
 */
class ModifyBaMtCmd : public QUndoCommand
{
public:
    ModifyBaMtCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, Money* mt, Double v, QUndoCommand* parent = 0);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    Money *newMt,*oldMt;
    Double newValue,oldValue;
    friend class ModifyBaMtMmd;
};



/**
 * @brief The ModifyBaValueCmd class
 * 修改会计分录的借贷值（方向和值）
 */
class ModifyBaValueCmd : public QUndoCommand
{
public:
    ModifyBaValueCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, Double v,MoneyDirection dir, QUndoCommand* parent = 0);
    int	id() const{return CMD_BA_VALUE;}
    bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    Double newValue, oldValue;
    MoneyDirection newDir,oldDir;
};

/**
 * @brief The ModifBaDelCmd class
 * 移除会计分录命令
 */
class ModifyBaDelCmd : public QUndoCommand
{
public:
    ModifyBaDelCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, QUndoCommand* parent = 0);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
};

/**
 * @brief The CutBaCmd class
 * 剪切会计分录命令
 */
class CutBaCmd : public QUndoCommand
{
public:
    CutBaCmd(AccountSuiteManager* pm, PingZheng* pz, QList<int> rows, QList<BusiAction*>* baLst, QUndoCommand* parent = 0);
    int	id() const{return CMD_BA_CUT;}
    bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    QList<BusiAction*>* baLst; //被剪切的会计分录对象的暂存位置
    QList<int> rows;  //被剪切的会计分录对象的索引号列表
};

/**
 * @brief The PasterBaCmd class
 * 粘贴会计分录命令
 */
class PasterBaCmd : public QUndoCommand
{
public:
    PasterBaCmd(PingZheng* pz, int row, QList<BusiAction*>* baLst, bool copy=true, QUndoCommand* parent = 0);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    int row,rows;     //插入位置，粘贴对象的数目
    QList<BusiAction*>* baLst; //指向剪贴板缓存列表
    bool copy;        //复制（true）或剪切（false）操作
    //QList<BusiAction*> cacheLst; //内部缓存列表
};

/**
 * @brief The ModifyBaMoveCmd class
 * 移动会计分录命令
 */
class ModifyBaMoveCmd : public QUndoCommand
{
public:
    ModifyBaMoveCmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, int rows, QUndoStack* stack, QUndoCommand* parent = 0);
    //int	id() const{return CMD_BA_MOVE;}
    //bool mergeWith(const QUndoCommand* command);
    void undo();
    void redo();
private:
    AccountSuiteManager* pm;     //凭证集
    PingZheng* pz;    //凭证对象
    BusiAction* ba;
    int rows;         //移动的行数（正数：向上，负数：向下）
    QUndoStack* pstack;
};

/**
 * @brief The CrtNameItemCmd class
 * 创建新的名称条目
 */
class CrtNameItemCmd : public QUndoCommand
{
public:
    CrtNameItemCmd(QString sname, QString lname, QString remCode, int nameCls, SubjectManager* subMgr, QUndoCommand* parent = 0);
    void undo();
    void redo();
    SubjectNameItem* getNI(){return ni;}
private:
    SubjectManager* subMgr;
    SubjectNameItem* ni;
    QString sname,lname,remCode;
    int nameCls;
};

/**
 * @brief The CreateNewNameMappingCmd class
 * 利用现存的名称条目对象在指定一级科目下创建二级科目
 */
class CrtSndSubUseNICmd : public QUndoCommand
{
public:
    CrtSndSubUseNICmd(SubjectManager* subMgr, FirstSubject* fsub, SubjectNameItem* ni, QUndoCommand* parent = 0);
    //int	id() const{return ;}
    //bool mergeWith(const QUndoCommand* command);
    void setNI(SubjectNameItem* ni){this->ni=ni;}
    void undo();
    void redo();
    SecondSubject* getSecondSubject(){return ssub;}
private:
    SubjectManager* subMgr;
    FirstSubject* fsub;
    SubjectNameItem* ni;
    SecondSubject* ssub;
};

/**
 * @brief The CrtSndSubUseNameCmd class
 * 使用名称信息创建新的名称条目，继而用此名称条目在指定一级科目下创建子目
 */
//class CrtSndSubUseNameCmd : public QUndoCommand
//{
//public:
//    CrtSndSubUseNameCmd(SubjectManager* subMgr, FirstSubject* fsub,QString sname,
//                        QString lname, QString remCode, int cls, QUndoCommand* parent = 0);
//    void undo();
//    void redo();
//    SecondSubject* getSecondSubject(){return ssub;}
//private:
//    SubjectManager* subMgr;
//    FirstSubject* fsub;
//    SubjectNameItem* ni;
//    SecondSubject* ssub;
//    QString sname,lname,remCode;
//    int nameCls;
//};

/**
 * @brief The CrtNewNameMapCmd class
 * 修改会计分录的二级科目对象宏命令（此二级科目是利用现存名称条目新建的）
 * 此命令用于凭证编辑窗口，当用户选择二级科目（此二级科目需要使用现存的名称条目创建时）
 */
class ModifyBaSndSubNMMmd : public QUndoCommand
{
public:
    ModifyBaSndSubNMMmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, SubjectManager* subMgr, FirstSubject* fsub, SubjectNameItem* ni, QUndoCommand* parent = 0);
    void undo();
    void redo();
    SecondSubject* getSecondSubject(){return ssub;}
private:
    SubjectManager* subMgr;
    FirstSubject* fsub;
    SubjectNameItem* ni;
    SecondSubject* ssub;
    AccountSuiteManager* pm;
    PingZheng* pz;
    BusiAction* ba;
};

/**
 * @brief The ModifyBaSndSubNSMmd class
 * 修改会计分录的二级科目对象宏命令
 * 此命令用于凭证编辑窗口，当用户选择一个需新建的二级科目时（此二级科目所使用的名称条目也需要新建）
 */
class ModifyBaSndSubNSMmd : public QUndoCommand
{
public:
    ModifyBaSndSubNSMmd(AccountSuiteManager* pm, PingZheng* pz, BusiAction* ba, SubjectManager* subMgr, FirstSubject* fsub, QString sname, QString lname, QString remCode, int nameCls, QUndoCommand* parent = 0);
    void undo();
    void redo();
    SecondSubject* getSecondSubject(){return ssub;}
private:
    SubjectManager* subMgr;
    FirstSubject* fsub;
    SubjectNameItem* ni;
    SecondSubject* ssub;
    AccountSuiteManager* pm;
    PingZheng* pz;
    BusiAction* ba;
    QString sname,lname,remCode;
    int nameCls;
};

/**
 * @brief The ModifyMultiPropertyOnBa class
 *  一次修改多个会计分录对象的属性
 */
class ModifyMultiPropertyOnBa : public QUndoCommand
{
public:
    ModifyMultiPropertyOnBa(BusiAction* ba, FirstSubject* fsub, SecondSubject* ssub, Money* mt, Double v, MoneyDirection dir,QUndoCommand* parent=0);
    void undo();
    void redo();
private:
    BusiAction* ba;
    FirstSubject* oldFSub, *newFSub;
    SecondSubject* oldSSub,*newSSub;
    Money* oldMt,*newMt;
    Double oldValue,newValue;
    MoneyDirection oldDir,newDir;
};



#endif // COMMANDS_H
