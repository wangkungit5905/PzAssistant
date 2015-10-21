#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <QDoubleValidator>

/**
 * @brief 可接受空串作为0
 */
class MyDoubleValidator : public QDoubleValidator
{
    Q_OBJECT
public:
    MyDoubleValidator(QObject * parent = 0):QDoubleValidator(parent){}
    MyDoubleValidator(double bottom, double top, int decimals, QObject * parent = 0):
        QDoubleValidator(bottom,top,decimals,parent){}

    QValidator::State	validate(QString & input, int & pos) const
    {
        if(input.isEmpty())
            return QValidator::Acceptable;
        else
            return QDoubleValidator::validate(input,pos);
    }
};


#endif // VALIDATOR_H

