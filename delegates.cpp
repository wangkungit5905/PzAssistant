#include <QComboBox>
#include <QLineEdit>
#include <QPainter>
#include <QPoint>
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QDebug>
#include <QDoubleSpinBox>
//#include <QRect>

#include "tables.h"
#include "delegates.h"
#include "common.h"
#include "completsubinfodialog.h"
#include "dialog2.h"
#include "utils.h"



iTosItemDelegate::iTosItemDelegate(QMap<int, QString> map, QObject *parent) : QItemDelegate(parent)
{
    isEnabled = true;
    QMapIterator<int, QString> i(map);
    while (i.hasNext()) {
        i.next();
        innerMap.insert(i.key(),i.value());
    }

}

iTosItemDelegate::iTosItemDelegate(QHash<int, QString> map, QObject *parent) : QItemDelegate(parent)
{
    isEnabled = true;
    QHashIterator<int, QString> i(map);
    while (i.hasNext()) {
        i.next();
        innerMap.insert(i.key(),i.value());
    }
}


QWidget* iTosItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const{
    QComboBox *editor = new QComboBox(parent);
    editor->setEnabled(isEnabled);
    QMapIterator<int, QString> i(innerMap);
    while (i.hasNext()) {
        i.next();
        editor->addItem(i.value(), i.key());
    }
    return editor;
}

void iTosItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int value = index.model()->data(index, Qt::EditRole).toInt();
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    int idx = comboBox->findData(value);
    comboBox->setCurrentIndex(idx);
}

void iTosItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
               const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    //comboBox->interpretText();
    int idx = comboBox->currentIndex();
    int value = comboBox->itemData(idx).toInt();

    model->setData(index, value, Qt::EditRole);

}

void iTosItemDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);

}

void iTosItemDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option,
             const QModelIndex & index ) const
{
    int value = index.model()->data(index, Qt::EditRole).toInt();
    painter->drawText(option.rect, Qt::AlignCenter, innerMap[value]);
}

//只是想使货币类型列不可编辑的权益之计
void iTosItemDelegate::setBoxEnanbled(bool isEnabled)
{
    this->isEnabled = isEnabled;
}

///////////////////////////////////////////////////////////////////////////////
SummaryDelegate::SummaryDelegate(QObject *parent) : QItemDelegate(parent)
{

}

QWidget*  SummaryDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
   BASummaryForm *editor = new BASummaryForm(parent);

    return editor;
}

void SummaryDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();
    BASummaryForm *edt = static_cast<BASummaryForm*>(editor);
    edt->setData(value);
}

void SummaryDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
               const QModelIndex &index) const
{
    BASummaryForm *edt = static_cast<BASummaryForm*>(editor);
    QString value = edt->getData();

    model->setData(index, value, Qt::EditRole);
}

void SummaryDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    rect.setHeight(100);
    editor->setGeometry(rect);
}

void SummaryDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const
{
    QString value = index.model()->data(index).toString();
    //分离摘要内容部分和引用信息部分（是一个xml片段）
    int start = value.indexOf("<");
    int end   = value.lastIndexOf(">");
    if((start > 0) && (end > start))
        value.remove(start, end - start + 1); //(start - 1)是为了移除紧随摘要后的换行符
    painter->drawText(option.rect, Qt::AlignCenter, value);


}

//bool SummaryDelegate::eventFilter(QObject *editor, QEvent *event)
//{
//    //QItemDelegate::eventFilter();
//    int t = event->type();
//    int i = 0;
//}

///////////////////////////////////////////////////////////////////////////////////////
SmartComboBox::SmartComboBox(int witch, QSqlQueryModel* model, int curFid,
                             QWidget* parent) : QComboBox(parent)
{
    this->witch = witch;

    if(witch == 2){
        setEditable(true);  //使其可以输入新的二级科目名
        this->curFid = curFid;
    }
    this->model = model;    //这个model是来自创建代理类时传递给它的
    keys = new QString;
    listview = new QListView(parent);

    if(witch == 1){ //一级科目
        listview->setModel(model);
        listview->setModelColumn(FSTSUB_SUBNAME);
    }
    else if(witch == 2){ //二级科目
        //应该使用取自SecSubjects表的model        
        smodel = new QSqlTableModel;
        smodel->setTable("SecSubjects");
        smodel->setSort(SNDSUB_REMCODE, Qt::AscendingOrder);
        listview->setModel(smodel);
        listview->setModelColumn(SNDSUB_SUBNAME);
        smodel->select();
    }
    listview->setFixedHeight(150); //最好是设置一个与父窗口的高度合适的尺寸
    //QPoint globalP = mapToGlobal(parent->pos());
    //listview->move(parent->x(),parent->y() + this->height());


}

void SmartComboBox::keyPressEvent ( QKeyEvent * e )
{
    static int i = 0;
    static bool isDigit = true;  //true：输入的是科目的数字代码，false：科目的助记符
    int subId; //在FsAgent表中的ID
    int sid;   //在SndSubClass表中的ID
    int index;

    /////////////////
    int idx,c;
    bool br = false;   //是否需要触发newMappingItem信号
    //////////////////

    int keyCode = e->key();

    if(witch == 1){ //提示的是一级科目条目

        //如果是字母键，则输入的是科目助记符，则按助记符快速定位
        if(((keyCode >= Qt::Key_A) && (keyCode <= Qt::Key_Z))){
            keys->append(keyCode);
            if(keys->size() == 1){ //接收到第一个字符，需要重新按科目助记符排序，并装载到列表框
                //model->sort(FSTSUB_REMCODE);
                isDigit = false;
                model->setQuery(QString("SELECT * FROM %1 where %2=1 ORDER BY %3")
                                .arg(tbl_fsub).arg(fld_fsub_isview).arg(fld_fsub_remcode));
                i = 0;
                listview->show();
            }
            //定位到最匹配的条目
            QString remCode = model->data(model->index(i, FSTSUB_REMCODE)).toString();
            int rows = model->rowCount();
            while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
                i++;
                remCode = model->data(model->index(i, FSTSUB_REMCODE)).toString();
            }
            if(i < rows)
                listview->setCurrentIndex(model->index(i, FSTSUB_SUBNAME));



        }
        //如果是数字键则键入的是科目代码，则按科目代码快速定位
        else if((keyCode >= Qt::Key_0) && (keyCode <= Qt::Key_9)){
            keys->append(keyCode);
            isDigit = true;
            if(keys->size() == 1){
                model->setQuery(QString("SELECT * FROM %1 where %2=1 ORDER BY %3")
                                .arg(tbl_fsub).arg(fld_fsub_isview).arg(fld_fsub_subcode));
                i = 0;
                listview->show();
            }
            QString subCode = model->data(model->index(i, FSTSUB_SUBCODE)).toString();
            int rows = model->rowCount();
            while((keys->compare(subCode) > 0) && (i < rows)){
                i++;
                subCode = model->data(model->index(i, FSTSUB_SUBCODE)).toString();
            }
            if(i < rows)
                listview->setCurrentIndex(model->index(i, FSTSUB_SUBNAME));
        }
        //如果是其他编辑键
        else if(listview->isVisible()){
            switch(keyCode){
            case Qt::Key_Backspace:  //退格键，则维护键盘字符缓存，并进行重新定位
                keys->chop(1);
                if(keys->size() == 0)
                    listview->hide();
                else{ //用遗留的字符根据是数字还是字母再重新进行依据科目代码或助记符进行定位
                    if(isDigit){
                        i = 0;
                        QString subCode = model->data(model->index(i, FSTSUB_SUBCODE)).toString();
                        int rows = model->rowCount();
                        while((keys->compare(subCode) > 0) && (i < rows)){
                            i++;
                            subCode = model->data(model->index(i, FSTSUB_SUBCODE)).toString();
                        }
                        if(i < rows)
                            listview->setCurrentIndex(model->index(i, FSTSUB_SUBNAME));
                    }
                    else{
                        i = 0;
                        QString remCode = model->data(model->index(i, FSTSUB_REMCODE)).toString();
                        int rows = model->rowCount();
                        while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
                            i++;
                            remCode = model->data(model->index(i, FSTSUB_REMCODE)).toString();
                        }
                        if(i < rows)
                            listview->setCurrentIndex(model->index(i, FSTSUB_SUBNAME));
                    }
                }
                break;

            case Qt::Key_Up:
                if(listview->currentIndex().row() > 0){
                    listview->setCurrentIndex(model->index(listview->currentIndex().row() - 1, FSTSUB_SUBNAME));
                }
                break;

            case Qt::Key_Down:
                if(listview->currentIndex().row() < model->rowCount() - 1){
                    listview->setCurrentIndex(model->index(listview->currentIndex().row() + 1, FSTSUB_SUBNAME));
                }
                break;

            case Qt::Key_Return:  //回车键
            case Qt::Key_Enter:
                index = listview->currentIndex().row();
                subId = model->data(model->index(index, 0)).toInt();
                setCurrentIndex(findData(subId));
                listview->hide();
                keys->clear();
                break;

            }
        }
        else
            QComboBox::keyPressEvent(e);
    }

    else if(witch == 2){  //提示的是二级科目
        //如果是字母键，则输入的是科目助记符，则按助记符快速定位
        //字母键只有在输入法未打开的情况下才会接收到
        if(((keyCode >= Qt::Key_A) && (keyCode <= Qt::Key_Z))){
            keys->append(keyCode);
            if(keys->size() == 1){ //接收到第一个字符，需要重新按科目助记符排序，并装载到列表框
                isDigit = false;
                smodel->select();
                i = 0;
                listview->show();
            }
            //定位到最匹配的条目
            QString remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
            int rows = smodel->rowCount();
            while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
                i++;
                remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
            }
            if(i < rows)
                listview->setCurrentIndex(smodel->index(i, SNDSUB_SUBNAME));


            //QComboBox::keyPressEvent(e);
        }
        //如果是数字键则键入的是科目代码，则按科目代码快速定位
        else if((keyCode >= Qt::Key_0) && (keyCode <= Qt::Key_9)){
            keys->append(keyCode);
            isDigit = true;
            if(keys->size() == 1)
                i = 0;
            showPopup();
            //定位到最匹配的科目
            QString subCode = model->data(model->index(i, 3)).toString();
            int rows = model->rowCount();
            while((keys->compare(subCode) > 0) && (i < rows)){
                i++;
                subCode = model->data(model->index(i, 3)).toString();
            }
            if(i < rows)
                setCurrentIndex(i);

        }
        //如果是其他编辑键
        else if(listview->isVisible() || (keys->size() > 0)){
            int cindex, fid;
            QString fname, sname, qtext, station; //一、二级科目名
            QSqlQuery* query;
            switch(keyCode){
            case Qt::Key_Backspace:  //退格键，则维护键盘字符缓存，并进行重新定位
                keys->chop(1);
                if(keys->size() == 0)
                    listview->hide();
                if((keys->size() == 0) && isDigit)
                    hidePopup();
                else{ //用遗留的字符根据是数字还是字母再重新进行依据科目代码或助记符进行定位
                    if(isDigit){
                        i = 0;
                        QString subCode = model->data(model->index(i, 3)).toString();
                        int rows = model->rowCount();
                        while((keys->compare(subCode) > 0) && (i < rows)){
                            i++;
                            subCode = model->data(model->index(i, 3)).toString();
                        }
                        if(i < rows)
                            listview->setCurrentIndex(model->index(i, 3));
                    }
                    else{
                        i = 0;
                        QString remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
                        int rows = smodel->rowCount();
                        while((keys->compare(remCode, Qt::CaseInsensitive) > 0) && (i < rows)){
                            i++;
                            remCode = smodel->data(smodel->index(i, SNDSUB_REMCODE)).toString();
                        }
                        if(i < rows)
                            listview->setCurrentIndex(smodel->index(i, SNDSUB_SUBNAME));
                    }
                }
                break;

            case Qt::Key_Up:
                if(listview->currentIndex().row() > 0){
                    listview->setCurrentIndex(smodel->index(listview->currentIndex().row() - 1, SNDSUB_SUBNAME));
                }
                break;

            case Qt::Key_Down:
                idx = listview->currentIndex().row();
                c = smodel->rowCount();
                if( idx < c - 1){
                    listview->setCurrentIndex(smodel->index(idx + 1, SNDSUB_SUBNAME));
                }
                break;

            case Qt::Key_Return:  //回车键
            case Qt::Key_Enter:
                index = listview->currentIndex().row();
                sid = smodel->data(smodel->index(index, 0)).toInt();

                //在当前一级科目下没有任何二级科目的映射，或者如果在当前的smodel中
                //找不到此sid值，说明当前一级科目和选择的二级科目没有对应的映射条目
                if(!findSubMapper(curFid, sid, &cindex))
                    br = true;  //设置一个标志
                else
                    setCurrentIndex(cindex);

                listview->hide();
                keys->clear();
                if(br) //要放在最后触发信号，才不会崩溃（因为弹出其他的窗口会是编辑器失去焦点，进而会销毁编辑器对象）
                    emit newMappingItem(curFid, sid);
                break;
            }
        }
        //在智能提示框没有出现时输入的文本（中文文本），需要在SecSubjects表中进行查找，
        //如果不能找到一个匹配的，这说明是一个新的二级科目
        else{
            if(keyCode == Qt::Key_Return){
                QString name = currentText();
                int sid;
                if(!findSubName(name,sid))
                    emit newSndSubject(curFid, name);
                else
                    emit newMappingItem(curFid,sid);
            }
        }

    }
}

void SmartComboBox::focusOutEvent(QFocusEvent* e)
{
    listview->hide();
}

//在this.model中查找是否有此一级和二级科目的对应条目
bool SmartComboBox::findSubMapper(int fid, int sid, int* index)
{
    int c = model->rowCount();
    bool founded = false;

    if(c > 0){
        int i = 0;
        while((i < c) && !founded){
            int fv = model->data(model->index(i,1)).toInt();
            int sv = model->data(model->index(i, 2)).toInt();
            if((fv == fid) && (sv == sid)){
                founded = true;
                (*index) = i;
            }
            i++;
        }
    }
    return founded;
}

//在二级科目表SecSubjects中查找是否存在名称为name的二级科目
bool SmartComboBox::findSubName(QString name, int& sid)
{
    QSqlQuery q;
    QString s = QString("select id from SecSubjects "
                        "where subName = '%1'").arg(name);
    if(q.exec(s) && q.first()){
        sid = q.value(0).toInt();
        return true;
    }
    else
        return false;
}

/////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
ViewDoubleDelegate::ViewDoubleDelegate(QObject *parent) : QItemDelegate(parent)
{
}

QWidget *ViewDoubleDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    //QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    QLineEdit *editor = new QLineEdit(parent);
    //显示用户只可以输入数字，其小数位最大2位

    return editor;
}

void ViewDoubleDelegate::setEditorData(QWidget *editor,
                                       const QModelIndex &index) const
{
    double value = index.model()->data(index, Qt::EditRole).toDouble();
//    QDoubleSpinBox *edt = static_cast<QDoubleSpinBox*>(editor);
//    edt->setMaximum(1.79769e+308);
//    edt->setMinimum(-1.79769e+308);
//    edt->setValue(value);
    QString t;
    if(value == 0)
        t = "";
    else
        t = QString::number(value, 'f', 2);
    QLineEdit *edt = static_cast<QLineEdit*>(editor);
    edt->setText(t);
}

void ViewDoubleDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
               const QModelIndex &index) const
{
    //QDoubleSpinBox *edt = static_cast<QDoubleSpinBox*>(editor);
    QLineEdit *edt = static_cast<QLineEdit*>(editor);
    double value = edt->text().toDouble();
    model->setData(index, value, Qt::EditRole);
}

void ViewDoubleDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}

void ViewDoubleDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option,
             const QModelIndex & index ) const
{
    double value = index.model()->data(index).toDouble();
    QString t;
    if(value == 0)
        t = "";
    else
        t = QString::number(value, 'f', 2);
    painter->drawText(option.rect, Qt::AlignCenter, t);
}



/////////////////////////////////////////////////////////////////////////
//PZFormItemDelegate::PZFormItemDelegate()
//{

//}

//QWidget* PZFormItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
//                           const QModelIndex &index) const
//{
//    if(index.column() == PZ_DATE){  //凭证日期
//        QCalendarWidget *calendar = new QCalendarWidget;
//        calendar->setWindowFlags(calendar->windowFlags()|Qt::Popup);
//        calendar->show();
//    }
//}

//void PZFormItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
//{

//}

//void PZFormItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
//               const QModelIndex &index) const
//{

//}

//void PZFormItemDelegate::updateEditorGeometry(QWidget *editor,
//    const QStyleOptionViewItem &option, const QModelIndex &index) const
//{

//}

//void PZFormItemDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option,
//             const QModelIndex & index ) const
//{

//}
