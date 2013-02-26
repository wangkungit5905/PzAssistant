#ifndef APPMODEL_H
#define APPMODEL_H

#include <QSqlQueryModel>
#include <QSqlRelationalTableModel>
#include <QStringListModel>

#include "commdatastruct.h"

/**
  该类用于显示单表数据（在单表中存在一个整数列，其每个整数值对应于一个有意义的名称字符串）
  */
class SuitableSqlModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    SuitableSqlModel(int witch, QMap<int, QString> maps, QObject *parent = 0);

    QVariant data(const QModelIndex &item, int role) const;
private:
    QMap<int, QString> innermap;
    int witch;
};


/////////////////////////////////////////////////////////////////////////////////////
/**
  这个类用来自定义的方式来显示模型中的数据，通过设置模型中数据的不同角色来完成
  */
class CustomRelationModel : public QSqlRelationalTableModel
{
    Q_OBJECT

public:
    CustomRelationModel(int col, int role, QMap<int, QVariant> map, QObject * parent = 0);
    void setRoleData();

private:
    int col;
    int role;
    QMap<int, QVariant> map;
};


///////////////////////////////////////////////////////////////////////////
class CustomRelationTableModel : public QSqlRelationalTableModel
{
    Q_OBJECT

public:
    CustomRelationTableModel(QObject * parent = 0, QSqlDatabase db = QSqlDatabase());
    void setSort(QList<int> columns);
    void setSort(int column, Qt::SortOrder order);
    //void setFilter(const QString &filter);
    QString selectStatement () const;

private:
    QString masteTabName;  //主表名称
    QString orderByStr;    //ORDER BY子句

};

////////////////////////////////////////////////////////////////////////
class BaTableVHeadViewModel : public QStringListModel
{
    Q_OBJECT
public:
    BaTableVHeadViewModel(QList<BusiActionData*> baList, QObject * parent = 0);
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
private:
    QList<BusiActionData*> baList;
};

#endif // APPMODEL_H
