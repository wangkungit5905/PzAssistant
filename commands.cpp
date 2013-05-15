#include <QList>

#include "commands.h"
#include "subject.h"
#include "pz.h"
#include "PzSet.h"
#include "global.h"
#include "logs/Logger.h"
#include "statements.h"


//////////////////////////CrtPzCommand/////////////////////////////////
AppendPzCmd::AppendPzCmd(PzSetMgr *pm, PingZheng *pz, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("append pingzheng(%1#)").arg(pz->number()));

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
InsertPzCmd::InsertPzCmd(PzSetMgr *pm, PingZheng *pz, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("insert pingzheng(%1#)").arg(pz->number()));
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
DelPzCmd::DelPzCmd(PzSetMgr *pm, PingZheng* pz, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("delete pingzheng(%1#)").arg(pz->number()));
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
ModifyPzDateCmd::ModifyPzDateCmd(PzSetMgr *pm, PingZheng *pz, QString ds, QUndoCommand *parent)
    : QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("set PingZheng(%1#) date to %2").arg(pz->number()).arg(ds));
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
    setText(QObject::tr("set PingZheng(%1#) date to %2").arg(pz->number()).arg(newDate));
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

ModifyPzZNumCmd::ModifyPzZNumCmd(PzSetMgr *pm, PingZheng *pz, int pnum, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("set PingZheng(%1#) znum to %2").arg(pz->number()).arg(pnum));
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
    setText(QObject::tr("set PingZheng(%1#) znum to %2").arg(pz->number()).arg(newPNum));
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
ModifyPzEncNumCmd::ModifyPzEncNumCmd(PzSetMgr *pm, PingZheng *pz, int encnum, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("set PingZheng(%1#) enum to %2").arg(pz->number()).arg(encnum));
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
    setText(QObject::tr("set PingZheng(%1#) enum to %2").arg(pz->number()).arg(newENum));
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
ModifyPzVStateCmd::ModifyPzVStateCmd(PzSetMgr *pm, PingZheng *pz, PzState state, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz)
{
    stateNames[Pzs_Repeal] = QObject::tr("repeal");
    stateNames[Pzs_Recording] = QObject::tr("recording");
    stateNames[Pzs_Verify] = QObject::tr("verified");
    stateNames[Pzs_Instat] = QObject::tr("instated");
    setText(QObject::tr("set PingZheng(%1#) state to %2").arg(pz->number()).arg(stateNames.value(state)));
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
    setText(QObject::tr("set PingZheng(%1#) state to %2").arg(pz->number()).arg(stateNames.value(newState)));
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
ModifyPzVUserCmd::ModifyPzVUserCmd(PzSetMgr *pm, PingZheng *pz, User *user, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("set PingZheng(%1#) verify user to %2").arg(pz->number()).arg(user?user->getName():""));
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
ModifyPzBUserCmd::ModifyPzBUserCmd(PzSetMgr *pm, PingZheng *pz, User *user, QUndoCommand *parent)
:QUndoCommand(parent),pm(pm),pz(pz)
{
    setText(QObject::tr("set PingZheng(%1#) instat user to %2").arg(pz->number()).arg(user?user->getName():""));
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

/////////////////////////////////CrtBlankBaCmd///////////////////////////////
AppendBaCmd::AppendBaCmd(PzSetMgr *pm, PingZheng *pz, BusiAction* ba, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
//    setText(QObject::tr("append bauiaction(%1) in PingZheng(%2#)")
//            .arg((ba->getNumber()==0)?QObject::tr("Blank busiaction"):QString::number(ba->getNumber()))
//            .arg(pz->number()));
    setText(QObject::tr("append Blank bauiaction(point->%1) in PingZheng(%2#)")
            .arg((int)ba).arg(pz->number()));
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
InsertBaCmd::InsertBaCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, int row, QUndoCommand *parent):
    QUndoCommand(parent),pm(pm),pz(pz),ba(ba),row(row)
{
    setText(QObject::tr("insert bauiaction(%1) in PingZheng(%2#)")
            .arg(row+1).arg(pz->number()));
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
ModifyBaSummaryCmd::ModifyBaSummaryCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, QString summary, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    setText(QObject::tr("set bauiaction(%1) of summary to '%2' in PingZheng(%3#)")
            .arg(ba->getNumber()).arg(summary).arg(pz->number()));
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
    setText(QObject::tr("set bauiaction(%1) of summary to '%2' in PingZheng(%3#)")
            .arg(ba->getNumber()).arg(newSummary).arg(pz->number()));
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
ModifyBaFSubMmd::ModifyBaFSubMmd(QString text, PzSetMgr *pm, PingZheng *pz, BusiAction *ba, QUndoStack *stack, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
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
ModifyBaFSubCmd::ModifyBaFSubCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, FirstSubject *fsub, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    setText(QObject::tr("set bauiaction(%1) of first subject to '%2' in PingZheng(%3#)")
            .arg(ba->getNumber()).arg(fsub->getName()).arg(pz->number()));
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
ModifyBaSSubCmd::ModifyBaSSubCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, SecondSubject *ssub, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    if(ssub)
        setText(QObject::tr("set bauiaction(%1) of second subject to '%2' in PingZheng(%3#)")
            .arg(ba->getNumber()).arg(ssub->getName()).arg(pz->number()));
    else
        setText(QObject::tr("clear bauiaction(%1) of second subject in PingZheng(%2#)")
            .arg(ba->getNumber()).arg(pz->number()));
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
ModifyBaMtCmd::ModifyBaMtCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, Money *mt, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    setText(QObject::tr("set bauiaction(%1) of money type to '%2' in PingZheng(%3#)")
            .arg(ba->getNumber()).arg(mt->name()).arg(pz->number()));
    oldMt = ba->getMt();
    newMt = mt;
    oldValue = ba->getValue();
}

void ModifyBaMtCmd::undo()
{
    ba->setMt(oldMt);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

void ModifyBaMtCmd::redo()
{
    ba->setMt(newMt);
    pm->setCurPz(pz);
    pz->setCurBa(ba);
}

/////////////////////////////////////ModifyBaMtMmd///////////////////////////////
ModifyBaMtMmd::ModifyBaMtMmd(QString text, PzSetMgr *pm, PingZheng *pz, BusiAction *ba, QUndoStack* stack, QUndoCommand *parent)
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
ModifyBaValueCmd::ModifyBaValueCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, Double v, MoneyDirection dir, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    QString dirStr;
    if(dir == DIR_J)
        dirStr = dirStrJ;
    else
        dirStr = dirStrD;
    setText(QObject::tr("set bauiaction(%1) of value to %2(%3) in PingZheng(%4#)")
            .arg(ba->getNumber()).arg(v.getv())
            .arg(dirStr).arg(pz->number()));
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
    setText(QObject::tr("set bauiaction(%1) of value to %2(%3) in PingZheng(%4#)")
            .arg(ba->getNumber()).arg(newValue.getv())
            .arg((newDir == DIR_J)?dirStrJ:dirStrD).arg(pz->number()));
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

CutBaCmd::CutBaCmd(PzSetMgr *pm, PingZheng *pz, QList<int> rows, QList<BusiAction *>* baLst, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),rows(rows),baLst(baLst)
{
    QString numStr;
    foreach(BusiAction* ba, *baLst)
        numStr.append(QString::number(ba->getNumber())).append(",");
    numStr.chop(1);
    setText(QObject::tr("cut busiaction(%1) int pingzheng(%2)")
            .arg(numStr).arg(pz->number()));
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
PasterBaCmd::PasterBaCmd(PzSetMgr *pm, PingZheng *pz, int row, QList<BusiAction *>* baLst, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),row(row),baLst(baLst)
{
    setText(QObject::tr("paster busiactions(amount: %1) at row(%2) in pingzheng(%3#)")
            .arg(baLst->count()).arg(row).arg(pz->number()));
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
}

void PasterBaCmd::redo()
{
    int r = row;
    foreach(BusiAction* ba, *baLst){
        pz->insert(r++,ba);
    }
    if(copyOrCut == CO_CUT)
        baLst->clear();
}


///////////////////////////////ModifyBaMoveCmd//////////////////////////////////
ModifyBaMoveCmd::ModifyBaMoveCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, int rows, QUndoStack* stack, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),rows(rows),pstack(stack)
{
    setText(QObject::tr("move  %1 bauiaction(%2) %3 rows in PingZheng(%4#)")
            .arg((rows>0)?QObject::tr("up"):QObject::tr("down"))
            .arg(ba->getNumber()).arg(rows).arg(pz->number()));

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
ModifyBaDelCmd::ModifyBaDelCmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba)
{
    setText(QObject::tr("delete busiaction(%1) in pingzheng(%2#)").arg(ba->getNumber()).arg(pz->number()));

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
    setText(QObject::tr("create new second subject(%1) use name item")
            .arg(ni?ni->getShortName():""));
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
ModifyBaSndSubNMMmd::ModifyBaSndSubNMMmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, SubjectManager *subMgr, FirstSubject *fsub, SubjectNameItem *ni, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),subMgr(subMgr),fsub(fsub),ni(ni),ssub(0)
{
    setText(QObject::tr("set busiaction(%1) second subject(%2) in pingzheng(%3) "
                        "use new name mapping").arg(ba->getNumber()).arg(ni->getShortName())
            .arg(pz->number()));
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
    setText(QObject::tr("create name item %1").arg(sname));
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
ModifyBaSndSubNSMmd::ModifyBaSndSubNSMmd(PzSetMgr *pm, PingZheng *pz, BusiAction *ba, SubjectManager *subMgr, FirstSubject *fsub, QString sname, QString lname, QString remCode, int nameCls, QUndoCommand *parent)
    :QUndoCommand(parent),pm(pm),pz(pz),ba(ba),subMgr(subMgr),fsub(fsub),sname(sname),lname(lname),remCode(remCode),nameCls(nameCls),ni(0),ssub(0)
{
    setText(QObject::tr("set busiaction(%1) of second subject to %2 in pingzheng(%3)")
            .arg(ba->getNumber()).arg(sname).arg(pz->number()));
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






