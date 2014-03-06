#ifndef SUBJECTSELECTORCOMBOBOX_H
#define SUBJECTSELECTORCOMBOBOX_H

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

    //过滤
    enum FILTERSECTION{
        FS_NAME = 0,
        FS_CODE = 1,
        FS_REMCODE = 2
    };

    explicit SubjectSelectorComboBox(QWidget *parent = 0);
    explicit SubjectSelectorComboBox(SubjectManager* subMgr, FirstSubject* fsub, SUBJECTCATALOG which = SC_FST, QWidget *parent = 0);

    void setSubjectManager(SubjectManager* smg);
    void setSubjectClass(SUBJECTCATALOG subClass = SC_FST);
    void setFirstSubject(FirstSubject* fsub);
    //FirstSubject* getFirstSubject(){return fsub;}
    //SecondSubject* getSecondSubject(){return ssub;}

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
    int findSubject(SubjectBase* sub);

    //QCompleter completer;
    FILTERSECTION sortBy;
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
