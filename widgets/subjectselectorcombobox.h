#ifndef SUBJECTSELECTORCOMBOBOX_H
#define SUBJECTSELECTORCOMBOBOX_H

#include "commdatastruct.h"

#include <QComboBox>
#include <QKeyEvent>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QTreeView>


class SubjectManager;
class FirstSubject;
class SubjectBase;
class SecondSubject;

class SubjectSelectorComboBox : public QComboBox
{
    Q_OBJECT
public:
    enum SUBJECTCATALOG{
        SC_FST = 1,
        SC_SND = 2
    };

    explicit SubjectSelectorComboBox(QWidget *parent = 0);
    explicit SubjectSelectorComboBox(SubjectManager* subMgr, FirstSubject* fsub, SUBJECTCATALOG which = SC_FST, QWidget *parent = 0);

    void setSubjectManager(SubjectManager* smg);
    void setSubjectClass(SUBJECTCATALOG subClass = SC_FST);
    void setSubject(SubjectBase *fsub);
    void setParentSubject(FirstSubject* fsub);
    //void addTemSndSub(SecondSubject* ssub);
    //FirstSubject* getFirstSubject(){return fsub;}
    //SecondSubject* getSecondSubject(){return ssub;}
    int findSubject(SubjectBase* sub);

protected:
    void keyPressEvent(QKeyEvent* event);
    void focusOutEvent(QFocusEvent* event);
private slots:
    void completed(const QModelIndex& index);
    void nameChanged(const QString& text);

private:
    void init();
    void switchModel(bool on=true);
    void loadFstSubs();
    void loadSndSubs();
    void refreshModel();
    void showCompleteList();


    //QCompleter completer;
    SortByMode sortBy;
    SUBJECTCATALOG which;
    SubjectManager* subMgr;
    FirstSubject* fsub;
    SecondSubject* ssub;
    QStandardItemModel sourceModel;
    QSortFilterProxyModel model;
    QTreeView tv;

    QString keys;
};

#endif // SUBJECTSELECTORCOMBOBOX_H
