#ifndef VARIOUSWIDGETS_H
#define VARIOUSWIDGETS_H

#include <QComboBox>
#include <QListWidget>
#include <QSpinBox>

/**
 * @brief The PAComboBox class
 * 此类和伙伴部件一起使用，当伙伴部件显示时，将其接收到的上下箭头键盘事件传递给伙伴
 */
//class PAComboBox : public QComboBox
//{
//    Q_OBJECT
//public:
//    PAComboBox(QWidget* parent = 0):QComboBox(parent),isPartnerView(false),partner(NULL){}
//    void setPartner(QListWidget* pw){partner=pw;}
//    void setPartnerViewState(bool isView){isPartnerView=isView;}
//protected:
//    void keyPressEvent(QKeyEvent* event);
//private:
//    bool isPartnerView;   //伙伴部件是否显示
//    QListWidget* partner; //伙伴部件
//};

/**
 * @brief The ApSpinBox class
 * 支持默认值的特别显示，QSpinBox只支持最小值的特别显示
 */
class PASpinBox :public QSpinBox
{
    Q_OBJECT
public:
    PASpinBox(QWidget* parent = 0);
    void setDefaultValue(int v, QString t);

protected:
    QString textFromValue(int value) const;

private:
    int defValue;  //默认值
    QString defText; //默认值对应的文本
};

#endif // VARIOUSWIDGETS_H

