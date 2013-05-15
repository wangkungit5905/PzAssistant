#include <QKeyEvent>


#include "variousWidgets.h"

void PAComboBox::keyPressEvent(QKeyEvent *event)
{
    if(!isPartnerView || !partner){
        QComboBox::keyPressEvent(event);
        return;
    }
    int keyCode = event->key();
    int row = partner->currentRow();
    if(keyCode == Qt::Key_Up){
        if(row > 0)
            partner->setCurrentRow(row-1);
        event->accept();
    }
    else if(keyCode == Qt::Key_Down){
        if(row < partner->count()-1)
            partner->setCurrentRow(row+1);
        event->accept();
    }
    else
        QComboBox::keyPressEvent(event);
}


////////////////////////////////////////////////////////////////////////////
PASpinBox::PASpinBox(QWidget* parent) : QSpinBox(parent){}

void PASpinBox::setDefaultValue(int v, QString t)
{
    if((v > minimum()) && v < maximum()){
        defValue = v;
        defText = t;
        setValue(v);
    }
}

QString PASpinBox::textFromValue(int value) const
{
    if(value == defValue)
        return defText;
    else
        return QSpinBox::textFromValue(value);
}
