#ifndef FSTSUBEDITCOMBOBOX_H
#define FSTSUBEDITCOMBOBOX_H

#include "subject.h"

#include <QComboBox>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QTreeView>


class SubjectManager;

class FstSubEditComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit FstSubEditComboBox(SubjectManager* subMgr, QWidget *parent = 0);
    void setSubject(FirstSubject* fsub);
    FirstSubject* getSubject(){return fsub;}
    void setCurrentIndex(int index);

protected:
    void keyPressEvent(QKeyEvent* event);
    void focusOutEvent(QFocusEvent* event);

signals:
    void dataEditCompleted(int col, bool isMove);
private slots:
    void completed(QModelIndex index);
    void nameChanged(QString text);
private:
    void loadSubs();
    void switchModel(bool on=true);
    void refreshModel();
    void showCompleteList();

    QString keys;           //
    SortByMode sortBy;      //
    FirstSubject* fsub;     //
    SubjectManager *subMgr; //
    QStandardItemModel sourceModel;
    QSortFilterProxyModel model;
    QTreeView tv;
};

#endif // FSTSUBEDITCOMBOBOX_H
