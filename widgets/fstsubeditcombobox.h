#ifndef FSTSUBEDITCOMBOBOX_H
#define FSTSUBEDITCOMBOBOX_H

#include "subject.h"

#include <QComboBox>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QTreeView>


class SubjectManager;

/**
 * @brief 用于分录编辑表格的一级科目编辑器类，可以通过科目代码或科目助记符的形式输入，也
 * 可以直接收入科目名称，在输入期间可以弹出适配的科目列表
 */
class FstSubEditComboBox : public QWidget
{
    Q_OBJECT
public:
    explicit FstSubEditComboBox(SubjectManager* subMgr, QWidget *parent = 0);
    void setSubject(FirstSubject* fsub);
    FirstSubject* getSubject(){return fsub;}
    void setCurrentIndex(int index);

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

signals:
    void dataEditCompleted(int col, bool isMove);
private slots:
    void completed(QModelIndex index);
    void nameChanged(QString text);
    void subjectIndexChanged(int index);
private:
    void loadSubs();
    void switchModel(bool on=true);
    void refreshModel();
    void hideTView(bool isHide);


    QString keys;           //
    SortByMode sortBy;      //
    FirstSubject* fsub;     //
    SubjectManager *subMgr; //
    QStandardItemModel sourceModel;
    QSortFilterProxyModel model;
    QComboBox* com;
    QTreeView* tv;
};

#endif // FSTSUBEDITCOMBOBOX_H
