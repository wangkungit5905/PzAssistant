#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>

#include "appmodel.h"
//#include "config.h"
//#include "global.h"
/**
  参数width：指定显示哪个表
  */
SuitableSqlModel::SuitableSqlModel(int witch, QMap<int, QString> maps, QObject *parent) : QSqlQueryModel(parent)
{
    innermap = maps;
    this->witch = witch;
}

QVariant SuitableSqlModel::data(const QModelIndex &index, int role) const
{
    QVariant value = QSqlQueryModel::data(index, role);
    if (value.isValid() && role == Qt::DisplayRole) {
        switch(witch){
        case 1:
            if (index.column() == 2)
                return innermap[value.toInt()];
            else
                return value.toString().toUpper();
            break;
        //case 2:
        }
    }
    return value;
}

//////////////////////////////////////////////////////////////////////////////////////////
CustomRelationModel::CustomRelationModel(int col, int role, QMap<int, QVariant> map, QObject * parent )
    : QSqlRelationalTableModel(parent)
{
    this->col = col;
    this->role = role;
    QMapIterator<int, QVariant> i(map);
    while(i.hasNext()){
        i.next();
        this->map.insert(i.key(), i.value());
    }


}

void CustomRelationModel::setRoleData()
{
    for(int i = 0; i < rowCount(); ++i){
        QModelIndex index = this->index(i, col);
        this->setData(index, map[i], role);
    }
}


////////////////////////////////////////////////////////////////////////////
CustomRelationTableModel::CustomRelationTableModel(QObject * parent, QSqlDatabase db) :
        QSqlRelationalTableModel(parent, db)
{

}


//设置数据模型的排序方式（参数columns是一个整形数组，具有偶数个元素，每两个为一对，
//其中，第一个表示要排序的列序号，第一个表示排序的方式-升序或降序）

void CustomRelationTableModel::setSort(QList<int> columns)
{
    //为简单期间，不做参数的有效性检测（列序号是否超出表的字段数，排序方式是否有效等）
    int count = columns.count();

    //构造ORDER BY子句
    orderByStr = " ORDER BY ";
    QString fname, tname;
    tname = tableName();

    for(int i = 0; i < count; i += 2){
        fname = record().fieldName(columns[i]); //字段名
        if(columns[i+1] == Qt::AscendingOrder)
            orderByStr = orderByStr + tname + "." + "\"" + fname + "\"" + " ASC, ";
        else
            orderByStr = orderByStr + tname + "." + "\"" + fname + "\"" + " DESC, ";

    }
    orderByStr.chop(2);
//    sqlStr = sqlStr + orderByStr;
//    setQuery(QSqlQuery(sqlStr));
    //select();
}

//对基类函数的重载，其功能由上面的函数实现
void CustomRelationTableModel::setSort(int column, Qt::SortOrder order)
{
    QList<int> cols;
    cols << column << order ;
    setSort(cols);
}


QString CustomRelationTableModel::selectStatement () const
{
    QString sql = QSqlRelationalTableModel::selectStatement();
    //可能需要去除order by子句，重新构造


    sql = sql + orderByStr;
    int i = 0;
    return sql;
}

//void CustomRelationTableModel::setFilter(const QString &filter)
//{

//}


//////////////////////////BaTableVHeadViewModel/////////////////////////
BaTableVHeadViewModel::BaTableVHeadViewModel(QList<BusiActionData*> baList,
                  QObject* parent) : QStringListModel(parent)
{
    this->baList = baList;
}

QVariant BaTableVHeadViewModel::data(const QModelIndex& index, int role) const
{
    if(role == Qt::DisplayRole){
        QString tag;
        switch(baList[index.row()]->state){
        case BusiActionData::BLANK:
            tag = "?";
            break;
        case BusiActionData::EDITED:
            tag = "*";
            break;
        case BusiActionData::NEW:
            tag = "+";
            break;
        case BusiActionData::INIT:
            tag = "";
            break;
        case BusiActionData::DELETED:
            tag = "-";
            break;
        }
        if(index.row() < baList.count())
            return QString("%1%2").arg(index.row()).arg(tag);
        else
            return QString("");
    }
}
