#ifndef SUBJECTSEARCHFORM_H
#define SUBJECTSEARCHFORM_H

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include "widgets.h"

namespace Ui {
class SubjectSearchForm;
}

class SubjectSearchForm : public StyledWidget
{
    Q_OBJECT
    
public:
    explicit SubjectSearchForm(QWidget *parent = 0);
    ~SubjectSearchForm();

    void attachDb(QSqlDatabase* db);
    void detachDb();
    
private slots:
    void on_btnSearch_clicked();
    void classChanged(int index);

    void keyWordEditingFinished();

private:
    void refresh();

    Ui::SubjectSearchForm *ui;
    QSqlDatabase* db;
    QSqlQueryModel* model;
};

#endif // SUBJECTSEARCHFORM_H
