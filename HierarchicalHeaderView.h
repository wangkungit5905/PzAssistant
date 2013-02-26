/*
Copyright (c) 2009, Krasnoshchekov Petr
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Krasnoshchekov Petr ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Krasnoshchekov Petr BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef HIERARCHICAL_HEADER_VIEW_H
#define HIERARCHICAL_HEADER_VIEW_H

#include <QHeaderView>
#include <QProxyModel>
#include <QPointer>

/**
层次式的QTableView.
表头数据来自表的data()函数返回的QAbstractItemModel，用来表示表头结构。
对于水平表头使用HorizontalHeaderDataRole，而垂直标题使用VerticalHeaderDataRole。
如果表头数据模型的data()函数的Qt::UserRole角色返回一个有效的QVariant，则表头的文本被旋转。否则以正常的水平方式显示。
*/


//这个代理模型，将表格的行列表头所用到的数据模型与表格本身的数据模型结合在一起统一管理
//由于QTableView.setModel()内部会调用horizontalHeader->setModel(model)和
//verticalHeader->setModel(model)，因此有必要使用此代理模型
class ProxyModelWithHeaderModels: public QProxyModel
{
    Q_OBJECT
public:
    ProxyModelWithHeaderModels(QObject* parent=0);

    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;

    void setHorizontalHeaderModel(QAbstractItemModel* model);

    QAbstractItemModel* getHorizontalHeaderModel();

    void setVerticalHeaderModel(QAbstractItemModel* model);

    QAbstractItemModel* getVerticalHeaderModel();

private:
    QPointer<QAbstractItemModel> _horizontalHeaderModel;//行列表头所用的数据模型
    QPointer<QAbstractItemModel> _verticalHeaderModel;
};


class HierarchicalHeaderView : public QHeaderView
{
    Q_OBJECT

    class private_data;
    private_data* _pd;

    QStyleOptionHeader styleOptionForCell(int logicalIndex) const;

public:
    enum HeaderDataModelRoles
    {
        HorizontalHeaderDataRole=Qt::UserRole,
        VerticalHeaderDataRole=Qt::UserRole+1
    };

    HierarchicalHeaderView(Qt::Orientation orientation, QWidget* parent = 0);
    ~HierarchicalHeaderView();

    void setModel(QAbstractItemModel* model);

protected:
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const;
    QSize sectionSizeFromContents(int logicalIndex) const;

private slots:
    void on_sectionResized(int logicalIndex);
};

#endif
