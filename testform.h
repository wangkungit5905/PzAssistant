#ifndef TESTFORM_H
#define TESTFORM_H

#include <QDialog>

namespace Ui {
class TestForm;
}

class FirstSubject;
class SecondSubject;

class TestForm : public QDialog
{
    Q_OBJECT

public:
    explicit TestForm(QWidget *parent = 0);
    ~TestForm();

private slots:
    void fstSubChanged(int index);
private:
    Ui::TestForm *ui;
    FirstSubject* fsub;
    SecondSubject* ssub;
};

#endif // TESTFORM_H
