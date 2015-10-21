#include "dialog2.h"
#include "global.h"
#include "pz.h"
#include "securitys.h"
#include "myhelper.h"


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
