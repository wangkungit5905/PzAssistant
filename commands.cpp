#include <QList>

#include "commands.h"
#include "subject.h"
#include "pz.h"
#include "PzSet.h"
#include "global.h"
#include "logs/Logger.h"
#include "statements.h"


//////////////////////////CrtPzCommand/////////////////////////////////
AppendPzCmd::AppendPzCmd(AccountSuiteManager *pm, PingZheng *pz, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("添加凭证（P%1）").arg(pz->number()));

}

void AppendPzCmd::undo()
{
    pm->remove(pz);
//    if(pm->getPzCount() == 0)
//        pm->setCurPz(NULL);
//    else
//        pm->seek(pz->number()-1);
}

void AppendPzCmd::redo()
{
    pm->append(pz);
}

////////////////////////////InsertPzCmd//////////////////////////////
InsertPzCmd::InsertPzCmd(AccountSuiteManager *pm, PingZheng *pz, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("插入凭证（P%1）").arg(pz->number()));
}

void InsertPzCmd::undo()
{
    pm->remove(pz);
}

void InsertPzCmd::redo()
{
    pm->insert(pz);
}


///////////////////////////////DelPzCommand////////////////////////
DelPzCmd::DelPzCmd(AccountSuiteManager *pm, PingZheng* pz, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("删除凭证（P%1）").arg(pz->number()));
}

void DelPzCmd::undo()
{
    pm->restorePz(pz);
}

void DelPzCmd::redo()
{
    pm->remove(pz);
}

/////////////////////////////////ModifyPzDateCmd//////////////////////////
ModifyPzDateCmd::ModifyPzDateCmd(AccountSuiteManager *pm, PingZheng *pz, QString ds, QUndoCommand *parent)
    : QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("设置日期为“%1”（P%2）").arg(ds.arg(pz->number())));
    oldDate = pz->getDate();
    newDate = ds;
}

bool ModifyPzDateCmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_PZDATE)
        return false;

    const ModifyPzDateCmd* other = static_cast<const ModifyPzDateCmd*>(command);
    if (pz != other->pz)
        return false;

    newDate = other->newDate;
    setText(QObject::tr("设置日期为“%1”（P%2）").arg(newDate).arg(pz->number()));
    return true;
}

void ModifyPzDateCmd::undo()
{    
    //LOG_INFO(QString("call ModifyPzDateCmd::undo(),set pingzheng(%1) date to %2").arg(pz->number()).arg(oldDate));
    pz->setDate(oldDate);
    pm->setCurPz(pz);
}

void ModifyPzDateCmd::redo()
{        
    //LOG_INFO(QString("call ModifyPzDateCmd::redo(),set pingzheng(%1) date to %2").arg(pz->number()).arg(newDate));
    pz->setDate(newDate);
    pm->setCurPz(pz);
}

/////////////////////////ModifyPzZNumCmd///////////////////////////

ModifyPzZNumCmd::ModifyPzZNumCmd(AccountSuiteManager *pm, PingZheng *pz, int pnum, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("设置自编号为“%1”（P%2）").arg(pnum).arg(pz->number()));
    oldPNum = pz->zbNumber();
    newPNum = pnum;
}

bool ModifyPzZNumCmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_PZZNUM)
        return false;

    const ModifyPzZNumCmd* other = static_cast<const ModifyPzZNumCmd*>(command);
    if (pz != other->pz)
        return false;

    newPNum = other->newPNum;
    setText(QObject::tr("设置自编号为“%1”（P%2）").arg(newPNum).arg(pz->number()));
    return true;
}

void ModifyPzZNumCmd::undo()
{
    pz->setZbNumber(oldPNum);
    pm->setCurPz(pz);
}

void ModifyPzZNumCmd::redo()
{
    pz->setZbNumber(newPNum);
    pm->setCurPz(pz);
}


/////////////////////////////ModifyPzEncNumCmd///////////////////////
ModifyPzEncNumCmd::ModifyPzEncNumCmd(AccountSuiteManager *pm, PingZheng *pz, int encnum, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("设置附件数为“%1”（P%2）").arg(encnum).arg(pz->number()));
    oldENum = pz->encNumber();
    newENum = encnum;
}

bool ModifyPzEncNumCmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_PZENCNUM)
        return false;

    const ModifyPzEncNumCmd* other = static_cast<const ModifyPzEncNumCmd*>(command);
    if (pz != other->pz)
        return false;

    newENum = other->newENum;
    setText(QObject::tr("设置附件数为“%1”（P%2）").arg(newENum).arg(pz->number()));
    return true;
}

void ModifyPzEncNumCmd::undo()
{
    pz->setEncNumber(oldENum);
    pm->setCurPz(pz);
}

void ModifyPzEncNumCmd::redo()
{
    pz->setEncNumber(newENum);
    pm->setCurPz(pz);
}

///////////////////////////////////////ModifyPzVStateCmd//////////////////////////////
ModifyPzVStateCmd::ModifyPzVStateCmd(AccountSuiteManager *pm, PingZheng *pz, PzState state, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz)
{
//    stateNames[Pzs_Repeal] = QObject::tr("repeal");
//    stateNames[Pzs_Recording] = QObject::tr("recording");
//    stateNames[Pzs_Verify] = QObject::tr("verified");
//    stateNames[Pzs_Instat] = QObject::tr("instated");
    setText(QObject::tr("设置凭证状态为“%1”（P%2）").arg(pzStates.value(state)).arg(pz->number()));
    oldState = pz->getPzState();
    newState = state;
}

bool ModifyPzVStateCmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_PZVSTATE)
        return false;

    const ModifyPzVStateCmd* other = static_cast<const ModifyPzVStateCmd*>(command);
    if (pz != other->pz)
        return false;

    newState = other->newState;
    setText(QObject::tr("设置凭证状态为“%1”（P%2）").arg(pzStates.value(newState)).arg(pz->number()));
    return true;
}

void ModifyPzVStateCmd::undo()
{
    pz->setPzState(oldState);
    pm->setCurPz(pz);
}

void ModifyPzVStateCmd::redo()
{
    pz->setPzState(newState);
    pm->setCurPz(pz);
}

///////////////////////////////ModifyPzVUserCmd////////////////////////////////////////
ModifyPzVUserCmd::ModifyPzVUserCmd(AccountSuiteManager *pm, PingZheng *pz, User *user, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("设置审核用户为“%1”（P%2）").arg(user?user->getName():"").arg(pz->number()));
    oldUser = pz->verifyUser();
    newUser = user;
}

void ModifyPzVUserCmd::undo()
{
    pz->setVerifyUser(oldUser);
    pm->setCurPz(pz);
}

void ModifyPzVUserCmd::redo()
{
    pz->setVerifyUser(newUser);
    pm->setCurPz(pz);
}

////////////////////////////////ModifyPzBUserCmd///////////////////////////////////
ModifyPzBUserCmd::ModifyPzBUserCmd(AccountSuiteManager *pm, PingZheng *pz, User *user, QUndoCommand *parent)
:QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("设置记账用户为“%1”（P%2）").arg(user?user->getName():"").arg(pz->number()));
    oldUser = pz->bookKeeperUser();
    newUser = user;
}

void ModifyPzBUserCmd::undo()
{
    pz->setBookKeeperUser(oldUser);
    pm->setCurPz(pz);
}

void ModifyPzBUserCmd::redo()
{
    pz->setBookKeeperUser(newUser);
    pm->setCurPz(pz);
}

//////////////////////////////ModifyPzComment////////////////////////////////////
ModifyPzComment::ModifyPzComment(AccountSuiteManager *pm, PingZheng *pz, QString info, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("修改备注信息为“%1”").arg(info));
    oldInfo = pz->memInfo();
    newInfo = info;
}

bool ModifyPzComment::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_PZCOMMENT)
        return false;

    const ModifyPzComment* other = static_cast<const ModifyPzComment*>(command);
    if (pz != other->pz)
        return false;

    newInfo = other->newInfo;
    setText(QObject::tr("修改备注信息为“%1”").arg(newInfo));
    return true;
}

void ModifyPzComment::undo()
{
    pz->setMemInfo(oldInfo);
    pm->setCurPz(pz);
}

void ModifyPzComment::redo()
{
    pz->setMemInfo(newInfo);
    pm->setCurPz(pz);
}

/////////////////////////////////CrtBlankBaCmd///////////////////////////////
AppendBaCmd::AppendBaCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction* ba, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
//    setText(QObject::tr("append bauiaction(%1) in PingZheng(%2#)")
//            .arg((ba->getNumber()==0)?QObject::tr("Blank busiaction"):QString::number(ba->getNumber()))
//            .arg(pz->number()));
    setText(QObject::tr("添加分录（P%1）").arg(pz->number()));
    pz->append(ba);
}

void AppendBaCmd::undo()
{
    pz->remove(ba);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void AppendBaCmd::redo()
{
    if(!pz->restore(ba))
        pz->append(ba);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

////////////////////////////////InsertBaCmd/////////////////////////////////
InsertBaCmd::InsertBaCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, int row, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz),ba(ba),row(row)
{
    setText(QObject::tr("插入分录（P%1B%2）").arg(pz->number()).arg(row+1));
}

void InsertBaCmd::undo()
{
    pz->remove(ba);
}

void InsertBaCmd::redo()
{
    pz->insert(row, ba);
}

///////////////////////////////ModifyBaSummaryCmd//////////////////////////////////
ModifyBaSummaryCmd::ModifyBaSummaryCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, QString summary, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    setText(QObject::tr("设置摘要为“%1”（P%2B%3）").arg(summary).arg(pz->number()).arg(ba->getNumber()));

    newSummary = summary;
    oldSummary = ba->getSummary();
}

bool ModifyBaSummaryCmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_BA_SUMMARY)
        return false;

    const ModifyBaSummaryCmd* other = static_cast<const ModifyBaSummaryCmd*>(command);
    if (pz != other->pz || ba != other->ba)
        return false;

    newSummary = other->newSummary;
    setText(QObject::tr("设置摘要为“%1”（P%2B%3）").arg(newSummary).arg(pz->number()).arg(ba->getNumber()));
    return true;
}

void ModifyBaSummaryCmd::undo()
{
    ba->setSummary(oldSummary);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void ModifyBaSummaryCmd::redo()
{
    ba->setSummary(newSummary);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

///////////////////////////////////////ModifyBaFSubMmd/////////////////////////////
ModifyBaFSubMmd::ModifyBaFSubMmd(QString text, AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, QUndoStack *stack, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),pstack(stack)
{
    setText(text);
}

bool ModifyBaFSubMmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_BA_FSTSUB)
        return false;

    const ModifyBaFSubMmd* other = static_cast<const ModifyBaFSubMmd*>(command);
    if (pz != other->pz || ba != other->ba)
        return false;

    if(childCount() == 0 || childCount() != other->childCount())
        return false;
    const ModifyBaFSubCmd* cmd1 = static_cast<const ModifyBaFSubCmd*>(child(0));
    const ModifyBaFSubCmd* cmd2 = static_cast<const ModifyBaFSubCmd*>(other->child(0));
    if(!cmd1 || !cmd2)
        return false;
    if(cmd1->oldFSub == cmd2->newFSub && cmd1->newFSub == cmd2->oldFSub){
        pstack->setIndex(pstack->index()-2);
        return true;
    }
    return false;
}

//////////////////////////////////////////ModifyBaFSubCmd/////////////////////////
ModifyBaFSubCmd::ModifyBaFSubCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, FirstSubject *fsub, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    setText(QObject::tr("设置一级科目为“%1”（P%2B%3）").arg(fsub->getName())
            .arg(pz->number()).arg(ba->getNumber()));
    newFSub = fsub;
    oldFSub = ba->getFirstSubject();
}

//bool ModifyBaFSubCmd::mergeWith(const QUndoCommand *command)
//{
//    if(command->id() != CMD_BA_FSTSUB)
//        return false;

//    const ModifyBaFSubCmd* other = static_cast<const ModifyBaFSubCmd*>(command);
//    if (pz != other->pz || ba != other->ba)
//        return false;

//    newFSub = other->newFSub;
//    setText(QObject::tr("set bauiaction(%1) of first subject to '%2' in PingZheng(%3#)")
//            .arg(ba->getNumber()).arg(newFSub->getName()).arg(pz->number()));
//    return true;
//}

void ModifyBaFSubCmd::undo()
{
    ba->setFirstSubject(oldFSub);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void ModifyBaFSubCmd::redo()
{
    ba->setFirstSubject(newFSub);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

/////////////////////////////////////ModifyBaSSubCmd////////////////////////////
ModifyBaSSubCmd::ModifyBaSSubCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, SecondSubject *ssub, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    if(ssub)
        setText(QObject::tr("设置二级科目为“%1”（P%2B%3）").arg(ssub->getName())
            .arg(pz->number()).arg(ba->getNumber()));
    else
        setText(QObject::tr("清除二级科目（P%1B%2）").arg(pz->number()).arg(ba->getNumber()));

    newSSub = ssub;
    oldSSub = ba->getSecondSubject();
}

//bool ModifyBaSSubCmd::mergeWith(const QUndoCommand *command)
//{
//    if(command->id() != CMD_BA_SNDSUB)
//        return false;

//    const ModifyBaSSubCmd* other = static_cast<const ModifyBaSSubCmd*>(command);
//    if (pz != other->pz || ba != other->ba)
//        return false;

//    newSSub = other->newSSub;
//    setText(QObject::tr("set bauiaction(%1) of second subject to '%2' in PingZheng(%3#)")
//            .arg(ba->getNumber()).arg(newSSub->getName()).arg(pz->number()));
//    return true;
//}

void ModifyBaSSubCmd::undo()
{
    ba->setSecondSubject(oldSSub);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void ModifyBaSSubCmd::redo()
{
    ba->setSecondSubject(newSSub);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

/////////////////////////////////////ModifyBaMtCmd//////////////////////////////
//ModifyBaMtCmd::ModifyBaMtCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, bool dir,
//    Money *mt, Double rate, QUndoStack* stack, QUndoCommand *parent)
//    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),dir(dir),rate(rate),pstack(stack)
//{
//    setText(QObject::tr("set bauiaction(%1) of money type to '%2' in PingZheng(%3#)")
//            .arg(ba->getNumber()).arg(mt->name()).arg(pz->number()));
//    newMt = mt;
//    oldMt = ba->getMt();
//    oldValue = ba->getValue();
//}

//bool ModifyBaMtCmd::mergeWith(const QUndoCommand *command)
//{
//    if(command->id() != CMD_BA_MT)
//        return false;

//    const ModifyBaMtCmd* other = static_cast<const ModifyBaMtCmd*>(command);
//    if (pz != other->pz || ba != other->ba)
//        return false;

//    if(oldMt == other->newMt && newMt == other->oldMt){
//        pstack->setIndex(pstack->index()-2);
//        return true;
//    }
//    newMt = other->newMt;
//    setText(QObject::tr("set bauiaction(%1) of money type to '%2' in PingZheng(%3#)")
//            .arg(ba->getNumber()).arg(newMt->name()).arg(pz->number()));
//    return true;
//}

//void ModifyBaMtCmd::undo()
//{
//    ba->setMt(oldMt);
//    ba->setValue(oldValue);
//    pm->setCurPz(pz);
//    pz->setCurBa(ba);
//}

//void ModifyBaMtCmd::redo()
//{
//    ba->setMt(newMt);
//    if(dir)
//        ba->setValue(oldValue * rate);
//    else
//        ba->setValue(oldValue / rate);
//    pm->setCurPz(pz);
//    pz->setCurBa(ba);
//}

////////////////////////////////ModifyBaMtCmd////////////////////////////////////
ModifyBaMtCmd::ModifyBaMtCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, Money *mt, Double v, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    setText(QObject::tr("设置币种为“%1”（P%2B%3）() of money type to '' in PingZheng(#)")
            .arg(mt->name()).arg(pz->number()).arg(ba->getNumber()));
    oldMt = ba->getMt();
    newMt = mt;
    oldValue = ba->getValue();
    newValue = v;
}

void ModifyBaMtCmd::undo()
{
    ba->setMt(oldMt,oldValue);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void ModifyBaMtCmd::redo()
{
    ba->setMt(newMt,newValue);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

/////////////////////////////////////ModifyBaMtMmd///////////////////////////////
ModifyBaMtMmd::ModifyBaMtMmd(QString text, AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, QUndoStack* stack, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),pstack(stack)
{
    setText(text);
}

bool ModifyBaMtMmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_BA_MT)
        return false;

    const ModifyBaMtMmd* other = static_cast<const ModifyBaMtMmd*>(command);
    if (pz != other->pz || ba != other->ba)
        return false;

    if(childCount() == 0 || childCount() != other->childCount()){
        return false;
    }

    const ModifyBaMtCmd* cmd1 = static_cast<const ModifyBaMtCmd*>(child(0)) ;
    const ModifyBaMtCmd* cmd2 = static_cast<const ModifyBaMtCmd*>(other->child(0));
    if(!cmd1 || !cmd2)
        return false;
    //连续2个可对冲的命令，将导致两个命令都被取消
    if(cmd1->newMt == cmd2->oldMt && cmd1->oldMt == cmd2->newMt){
        pstack->setIndex(pstack->index()-2);
        return true;
    }
    return false;
}

/////////////////////////////////ModifyBaValueCmd//////////////////////////////////
ModifyBaValueCmd::ModifyBaValueCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, Double v, MoneyDirection dir, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    QString ds = (dir==MDIR_J)?QObject::tr("借"):QObject::tr("贷");
    setText(QObject::tr("设置金额为“%1”（%2P%3B%4）").arg(v.getv())
            .arg(ds).arg(pz->number()).arg(ba->getNumber()));
    newValue = v;
    newDir = dir;
    oldValue = ba->getValue();
    oldDir = ba->getDir();
}

bool ModifyBaValueCmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_BA_VALUE)
        return false;

    const ModifyBaValueCmd* other = static_cast<const ModifyBaValueCmd*>(command);
    if (pz != other->pz || ba != other->ba)
        return false;

    newValue = other->newValue;
    newDir = other->newDir;
    QString ds = (newDir==MDIR_J)?QObject::tr("借"):QObject::tr("贷");
    setText(QObject::tr("设置金额为“%1”（%2P%3B%4）").arg(newValue.getv())
            .arg(ds).arg(pz->number()).arg(ba->getNumber()));
    return true;
}

void ModifyBaValueCmd::undo()
{
    ba->setValue(oldValue);
    ba->setDir(oldDir);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void ModifyBaValueCmd::redo()
{
    ba->setValue(newValue);
    ba->setDir(newDir);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}


////////////////////////////CutBaCmd/////////////////////////////////////////

CutBaCmd::CutBaCmd(AccountSuiteManager *pm, PingZheng *pz, QList<int> rows, QList<BusiAction *>* baLst, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),rows(rows),baLst(baLst)
{
    QString numStr;
    foreach(int row, rows)
        numStr.append(QString("%1,").arg(row+1));
    numStr.chop(1);
    setText(QObject::tr("剪切分录（P%1B%2）").arg(pz->number()).arg(numStr));
}

bool CutBaCmd::mergeWith(const QUndoCommand *command)
{
    if(command->id() != CMD_BA_CUT)
        return false;

    const CutBaCmd* other = static_cast<const CutBaCmd*>(command);
    if(pm != other->pm)
        return false;
    return true;
}

void CutBaCmd::undo()
{
    BusiAction* ba;
    foreach(int i, rows){
        ba = baLst->takeFirst();
        pz->insert(i,ba);
    }
    pm->setCurPz(pz);
}

void CutBaCmd::redo()
{
    BusiAction* ba;
    for(int i = rows.count()-1; i > -1; i--){
        ba = pz->take(rows.at(i));
        baLst->push_front(ba);
    }
    pm->setCurPz(pz);
}


/////////////////////////PasterBaCmd////////////////////////////////////////////
PasterBaCmd::PasterBaCmd(PingZheng *pz, int row, QList<BusiAction *>* baLst, bool copy, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pz->parent()),pz(pz),row(row),baLst(baLst),copy(copy)
{
    setText(QObject::tr("粘贴分录（P%1R%2）").arg(pz->number()).arg(row));
    rows = baLst->count();
}

void PasterBaCmd::undo()
{
    BusiAction* ba;
    for(int i = row+rows-1; i >= row; i--){
        ba = pz->take(i);
        if(copyOrCut == CO_CUT)
            baLst->push_front(ba);
    }
    pm->setCurPz(pz);
}

void PasterBaCmd::redo()
{
    int r = row;
    foreach(BusiAction* ba, *baLst){
        BusiAction* b1;
        if(copy)
            b1 = new BusiAction(*ba);
        else
            b1 = ba;
        pz->insert(r++,b1);
    }
    if(!copy)
        baLst->clear();
    pm->setCurPz(pz);
}


///////////////////////////////ModifyBaMoveCmd//////////////////////////////////
ModifyBaMoveCmd::ModifyBaMoveCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, int rows, QUndoStack* stack, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),rows(rows),pstack(stack)
{
    setText(QObject::tr("向%1移动分录%2行（P%3C%4）")
            .arg((rows>0)?QObject::tr("up"):QObject::tr("down")).arg(rows).arg(pz->number())
            .arg(ba->getNumber()));

}

//bool ModifyBaMoveCmd::mergeWith(const QUndoCommand *command)
//{
//    if(command->id() != CMD_BA_MOVE)
//        return false;

//    const ModifyBaMoveCmd* other = static_cast<const ModifyBaMoveCmd*>(command);
//    if (pz != other->pz || ba != other->ba)
//        return false;

//    if(rows = -other->rows){
//        pstack->setIndex(pstack->index()-2);
//        return true;
//    }

//    rows += other->rows;
//    if(rows != 0)
//        setText(QObject::tr("move  %1 bauiaction(%2) %3 rows in PingZheng(%4#)")
//                .arg((rows>0)?QObject::tr("up"):QObject::tr("down"))
//                .arg(ba->getNumber()).arg(rows).arg(pz->number()));
//    else
//        setText(QObject::tr("do nothings"));
//    return true;
//}

void ModifyBaMoveCmd::undo()
{
    if(rows > 0)
        pz->moveDown(ba->getNumber()-1,rows);
    else if(rows < 0)
        pz->moveUp(ba->getNumber()-1,-rows);


}

void ModifyBaMoveCmd::redo()
{
    if(rows > 0)
        pz->moveUp(ba->getNumber()-1,rows);
    else if(rows < 0)
        pz->moveDown(ba->getNumber()-1,-rows);
}



/////////////////////////////ModifBaDelCmd////////////////////////////////////
ModifyBaDelCmd::ModifyBaDelCmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    setText(QObject::tr("删除分录（P%1B%2）").arg(pz->number()).arg(ba->getNumber()));

}

void ModifyBaDelCmd::undo()
{
    pz->restore(ba);
    pm->setCurPz(pz);
}

void ModifyBaDelCmd::redo()
{
    pz->remove(ba);
    pm->setCurPz(pz);    
}

/////////////////////////////////////CrtSndSubUseNICmd//////////////////////////////////
CrtSndSubUseNICmd::CrtSndSubUseNICmd(SubjectManager *subMgr, FirstSubject *fsub, SubjectNameItem *ni, QUndoCommand *parent)
    :QUndoCommand(parent),subMgr(subMgr),fsub(fsub),ni(ni)
{
    setText(QObject::tr("创建二级科目“%1”（NF-%2）")
            .arg(ni?ni->getShortName():"").arg(fsub?fsub->getName():""));
}

void CrtSndSubUseNICmd::undo()
{
    fsub->removeChildSub(ssub);
}

void CrtSndSubUseNICmd::redo()
{
    if(fsub->containChildSub(ni))
        return;
    SecondSubject* ss = fsub->restoreChildSub(ni);
    if(!ss)
        ssub = fsub->addChildSub(ni);
    else
        ssub = ss;
}

/////////////////////////////ModifyBaSndSubNMMmd//////////////////////////
ModifyBaSndSubNMMmd::ModifyBaSndSubNMMmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, SubjectManager *subMgr, FirstSubject *fsub, SubjectNameItem *ni, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),subMgr(subMgr),fsub(fsub),ni(ni),ssub(0)
{
    //注意：这里的“S”表示用已有的名称条目创建的二级科目
    setText(QObject::tr("设置二级科目为“%1”（P%2B%3-S）").arg(ni->getShortName()
                       .arg(pz->number()).arg(ba->getNumber())));
    CrtSndSubUseNICmd* cmd1 = new CrtSndSubUseNICmd(subMgr,fsub,ni,this);
    ModifyBaSSubCmd* cmd2 = new ModifyBaSSubCmd(pm,pz,ba,ssub,this);
}

void ModifyBaSndSubNMMmd::undo()
{
    CrtSndSubUseNICmd* cmd1 = const_cast<CrtSndSubUseNICmd*>(static_cast<const CrtSndSubUseNICmd*>(child(0)));
    ModifyBaSSubCmd* cmd2 = const_cast<ModifyBaSSubCmd*>(static_cast<const ModifyBaSSubCmd*>(child(1)));
    if(!cmd1 || !cmd2)
        return;
    cmd2->undo();
    cmd1->undo();
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void ModifyBaSndSubNMMmd::redo()
{
    //先执行利用新名称条目创建子目的命令，再执行修改会计分录二级科目的命令
    CrtSndSubUseNICmd* cmd1 = const_cast<CrtSndSubUseNICmd*>(static_cast<const CrtSndSubUseNICmd*>(child(0)));
    ModifyBaSSubCmd* cmd2 = const_cast<ModifyBaSSubCmd*>(static_cast<const ModifyBaSSubCmd*>(child(1)));

    if(!cmd1 || !cmd2)
        return;
    cmd1->redo();
    ssub = cmd1->getSecondSubject();
    cmd2->setsub(ssub);
    cmd2->redo();
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}


//////////////////////////////CrtNameItemCmd/////////////////////////
CrtNameItemCmd::CrtNameItemCmd(QString sname, QString lname, QString remCode, int nameCls, SubjectManager *subMgr, QUndoCommand *parent)
    :QUndoCommand(parent),sname(sname),lname(lname),remCode(remCode),nameCls(nameCls),subMgr(subMgr)
{
    setText(QObject::tr("创建名称条目“%1”").arg(sname));
}

void CrtNameItemCmd::undo()
{
    subMgr->removeNameItem(ni);
}

void CrtNameItemCmd::redo()
{
    ni = subMgr->restoreNI(sname,lname,remCode,nameCls);
    if(!ni)
        ni = subMgr->addNameItem(sname,lname,remCode,nameCls);

}

///////////////////////////////ModifyBaSndSubNSMmd////////////////////////
ModifyBaSndSubNSMmd::ModifyBaSndSubNSMmd(AccountSuiteManager *pm, PingZheng *pz, BusiAction *ba, SubjectManager *subMgr, FirstSubject *fsub, QString sname, QString lname, QString remCode, int nameCls, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),subMgr(subMgr),fsub(fsub),sname(sname),lname(lname),remCode(remCode),nameCls(nameCls),ni(0),ssub(0)
{
    //注意：这里的“SN”表示用新建的名称条目在一级科目下创建的二级科目
    setText(QObject::tr("设置二级科目为“%1”（P%2B%3-SN）").arg(sname).arg(pz->number())
            .arg(ba->getNumber()));
    CrtNameItemCmd* cmd1 = new CrtNameItemCmd(sname,lname,remCode,nameCls,subMgr,this);
    CrtSndSubUseNICmd* cmd2 = new CrtSndSubUseNICmd(subMgr,fsub,ni,this);
    ModifyBaSSubCmd* cmd3 = new ModifyBaSSubCmd(pm,pz,ba,ssub,this);
}

void ModifyBaSndSubNSMmd::undo()
{
    CrtNameItemCmd* cmd1 = const_cast<CrtNameItemCmd*>(static_cast<const CrtNameItemCmd*>(child(0)));
    CrtSndSubUseNICmd* cmd2 = const_cast<CrtSndSubUseNICmd*>(static_cast<const CrtSndSubUseNICmd*>(child(1)));
    ModifyBaSSubCmd* cmd3 = const_cast<ModifyBaSSubCmd*>(static_cast<const ModifyBaSSubCmd*>(child(2)));
    if(!cmd1 || !cmd2 || !cmd3)
        return;
    cmd3->undo();
    cmd2->undo();
    cmd1->undo();
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void ModifyBaSndSubNSMmd::redo()
{
    //先创建名称条目，在利用创建的名称条目来创建二级科目，最后设置会计分录的二级科目
    CrtNameItemCmd* cmd1 = const_cast<CrtNameItemCmd*>(static_cast<const CrtNameItemCmd*>(child(0)));
    CrtSndSubUseNICmd* cmd2 = const_cast<CrtSndSubUseNICmd*>(static_cast<const CrtSndSubUseNICmd*>(child(1)));
    ModifyBaSSubCmd* cmd3 = const_cast<ModifyBaSSubCmd*>(static_cast<const ModifyBaSSubCmd*>(child(2)));
    if(!cmd1 || !cmd2 || !cmd3){
        //LOG_INFO("three cmd(cmd1,cmd2,cmd3) transform failed !");
        return;
    }
    //LOG_INFO("three cmd(cmd1,cmd2,cmd3) transform secessed !");
    cmd1->redo();
    ni = cmd1->getNI();
    cmd2->setNI(ni);
    cmd2->redo();
    ssub = cmd2->getSecondSubject();
    cmd3->setsub(ssub);
    cmd3->redo();
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}



/////////////////////////////////////////////////////////////////


ModifyMultiPropertyOnBa::ModifyMultiPropertyOnBa(BusiAction *ba, FirstSubject *fsub, SecondSubject *ssub, Money *mt, Double v, MoneyDirection dir,QUndoCommand* parent)
    :QUndoCommand(parent),ba(ba),newFSub(fsub),newSSub(ssub),newMt(mt),newValue(v),newDir(dir)
{
    oldFSub = ba->getFirstSubject();
    oldSSub = ba->getSecondSubject();
    oldMt = ba->getMt();
    oldValue = ba->getValue();
    oldDir = ba->getDir();
    QString info;
    if(oldFSub != newFSub)
        info.append(QObject::tr("一级科目：%1，").arg(newFSub?newFSub->getName():""));
    if(oldSSub != newSSub)
        info.append(QObject::tr("二级科目：%1，").arg(newSSub?newSSub->getName():""));
    if(oldMt != newMt)
        info.append(QObject::tr("币种：%1，").arg(newMt?newMt->name():""));
    if(oldValue != newValue)
        info.append(QObject::tr("金额：%1，").arg(newValue.toString2()));
    if(oldDir != newDir)
        info.append(QObject::tr("方向：%1，").arg(newDir==MDIR_J?QObject::tr("借"):QObject::tr("贷")));
    setText(QObject::tr("修改分录对象（%1）").arg(info));
}

void ModifyMultiPropertyOnBa::undo()
{
    ba->integratedSetValue(oldFSub,oldSSub,oldMt,oldValue,oldDir);
}

void ModifyMultiPropertyOnBa::redo()
{
    ba->integratedSetValue(newFSub,newSSub,newMt,newValue,newDir);
}








