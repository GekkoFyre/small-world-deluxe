/**
 **     __                 _ _   __    __           _     _ 
 **    / _\_ __ ___   __ _| | | / / /\ \ \___  _ __| | __| |
 **    \ \| '_ ` _ \ / _` | | | \ \/  \/ / _ \| '__| |/ _` |
 **    _\ \ | | | | | (_| | | |  \  /\  / (_) | |  | | (_| |
 **    \__/_| |_| |_|\__,_|_|_|   \/  \/ \___/|_|  |_|\__,_|
 **                                                         
 **                  ___     _                              
 **                 /   \___| |_   ___  _____               
 **                / /\ / _ \ | | | \ \/ / _ \              
 **               / /_//  __/ | |_| |>  <  __/              
 **              /___,' \___|_|\__,_/_/\_\___|              
 **
 **
 **   If you have downloaded the source code for "Small World Deluxe" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020 - 2021. GekkoFyre.
 **
 **   Small World Deluxe is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Small world is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Small World Deluxe.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/models/treeview/gk_generic_treeview_model.hpp"
#include <QApplication>

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;
using namespace Network;
using namespace GkXmpp;
using namespace Security;

/**
 * @brief GkGenericTreeViewItem::GkGenericTreeViewItem
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param parent
 */
GkGenericTreeViewItem::GkGenericTreeViewItem(const QVector<QVariant> &data, GkGenericTreeViewItem *parent) : m_itemData(data), m_parentItem(parent)
{
    return;
}

GkGenericTreeViewItem::~GkGenericTreeViewItem()
{
    qDeleteAll(m_childItems);
}

/**
 * @brief GkGenericTreeViewItem::appendChild inserts a new child node into the given QTreeView object.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param child
 */
void GkGenericTreeViewItem::appendChild(GkGenericTreeViewItem *child)
{
    m_childItems.append(child);

    return;
}

/**
 * @brief GkGenericTreeViewItem::removeChild removes a currently present child node from the given QTreeView object.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param child
 */
void GkGenericTreeViewItem::removeChild(GkGenericTreeViewItem *child)
{
    m_childItems.removeAll(child); // Removes all occurrences of the given value/pointer!

    return;
}

GkGenericTreeViewItem *GkGenericTreeViewItem::child(qint32 row)
{
    if (row < 0 || row >= m_childItems.size()) {
        return nullptr;
    }

    return m_childItems.value(row);
}

qint32 GkGenericTreeViewItem::childCount() const
{
    return m_childItems.count();
}

qint32 GkGenericTreeViewItem::columnCount() const
{
    return m_itemData.count();
}

QVariant GkGenericTreeViewItem::data(qint32 column) const
{
    if (column < 0 || column >= m_itemData.size()) {
        return QVariant();
    }

    return m_itemData.value(column);
}

/**
 * @brief GkGenericTreeViewItem::insertChildren
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param position
 * @param count
 * @param columns
 * @return
 */
bool GkGenericTreeViewItem::insertChildren(qint32 position, qint32 count, qint32 columns)
{
    if (position < 0 || position > m_childItems.size()) {
        return false;
    }

    for (qint32 row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        GkGenericTreeViewItem *item = new GkGenericTreeViewItem(data, this);
        m_childItems.insert(position, item);
    }

    return true;
}

/**
 * @brief GkGenericTreeViewItem::insertColumns
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param position
 * @param columns
 * @return
 */
bool GkGenericTreeViewItem::insertColumns(qint32 position, qint32 columns)
{
    if (position < 0 || position > m_itemData.size()) {
        return false;
    }

    for (qint32 column = 0; column < columns; ++column) {
        m_itemData.insert(position, QVariant());
    }

    for (GkGenericTreeViewItem *child : qAsConst(m_childItems)) {
        child->insertColumns(position, columns);
    }

    return true;
}

GkGenericTreeViewItem *GkGenericTreeViewItem::parent()
{
    return m_parentItem;
}

/**
 * @brief GkGenericTreeViewItem::removeChildren
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param position
 * @param count
 * @return
 */
bool GkGenericTreeViewItem::removeChildren(qint32 position, qint32 count)
{
    if (position < 0 || position + count > m_childItems.size()) {
        return false;
    }

    for (qint32 row = 0; row < count; ++row) {
        delete m_childItems.takeAt(position);
    }

    return true;
}

/**
 * @brief GkGenericTreeViewItem::removeColumns
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param position
 * @param columns
 * @return
 */
bool GkGenericTreeViewItem::removeColumns(qint32 position, qint32 columns)
{
    if (position < 0 || position + columns > m_itemData.size()) {
        return false;
    }

    for (qint32 column = 0; column < columns; ++column) {
        m_itemData.remove(position);
    }

    for (GkGenericTreeViewItem *child : qAsConst(m_childItems)) {
        child->removeColumns(position, columns);
    }

    return true;
}

qint32 GkGenericTreeViewItem::childNumber() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<GkGenericTreeViewItem *>(this));
    }

    return 0;
}

bool GkGenericTreeViewItem::setData(qint32 column, const QVariant &value)
{
    if (column < 0 || column >= m_itemData.size()) {
        return false;
    }

    m_itemData[column] = value;
    return true;
}

qint32 GkGenericTreeViewItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<GkGenericTreeViewItem *>(this));
    }

    return 0;
}

/**
 * @brief GkGenericTreeViewModel::GkGenericTreeViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param headers
 * @param data
 * @param parent
 * @note QTreeView and QTableView dynamic changes <https://codingadventures1.blogspot.com/2020/04/dynamically-adding-items-to-qtreeview.html>.
 */
GkGenericTreeViewModel::GkGenericTreeViewModel(const QStringList &headers, QObject *parent) : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    for (const QString &header: headers) {
        rootData << header;
    }

    m_rootItem = new GkGenericTreeViewItem(rootData);
    return;
}

GkGenericTreeViewModel::~GkGenericTreeViewModel()
{
    delete m_rootItem;
    return;
}

QVariant GkGenericTreeViewModel::data(const QModelIndex &index, qint32 role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    GkGenericTreeViewItem *item = getItem(index);
    return item->data(index.column());
}

QVariant GkGenericTreeViewModel::headerData(qint32 section, Qt::Orientation orientation, qint32 role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_rootItem->data(section);
    }

    return QVariant();
}

QModelIndex GkGenericTreeViewModel::index(qint32 row, qint32 column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return QModelIndex();
    }

    GkGenericTreeViewItem *parentItem = getItem(parent);
    if (!parentItem) {
        return QModelIndex();
    }

    GkGenericTreeViewItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    }

    return QModelIndex();
}

QModelIndex GkGenericTreeViewModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    GkGenericTreeViewItem *childItem = getItem(index);
    GkGenericTreeViewItem *parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == m_rootItem || !parentItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

/**
 * @brief GkGenericTreeViewModel::indexForTreeItem calculates a QModelIndex, ready for use in creating a child, by
 * external QTableView functions.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param item
 * @return The calculated QModelIndex in question.
 */
QModelIndex GkGenericTreeViewModel::indexForTreeItem(GkGenericTreeViewItem *item)
{
    return createIndex(item->childNumber(), 0, item);
}

qint32 GkGenericTreeViewModel::rowCount(const QModelIndex &parent) const
{
    const GkGenericTreeViewItem *parentItem = getItem(parent);
    return parentItem ? parentItem->childCount() : 0;
}

qint32 GkGenericTreeViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_rootItem->columnCount();
}

Qt::ItemFlags GkGenericTreeViewModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

bool GkGenericTreeViewModel::setData(const QModelIndex &index, const QVariant &value, qint32 role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    GkGenericTreeViewItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result) {
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    }

    return result;
}

bool
GkGenericTreeViewModel::setHeaderData(qint32 section, Qt::Orientation orientation, const QVariant &value, qint32 role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal) {
        return false;
    }

    const bool result = m_rootItem->setData(section, value);

    if (result) {
        emit headerDataChanged(orientation, section, section);
    }

    return result;
}

/**
 * @brief GkGenericTreeViewModel::insertData inserts data into a given QTreeView where the model is set towards this
 * class.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The value itself to be inserted into the QTreeView.
 * @param rootItem The possible parent of the value to be modified.
 * @note Simple Tree Model Example by Qt Project <https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html>,
 * Editable Tree Model Example <https://doc.qt.io/qt-5/qtwidgets-itemviews-editabletreemodel-example.html>.
 */
void GkGenericTreeViewModel::insertData(const QVariant &value, GkGenericTreeViewItem *rootItem)
{
    if (value.isValid()) {
        QVector<QVariant> columnData;
        QList<GkGenericTreeViewItem *> rootItems;
        rootItems << rootItem;
        columnData << value;

        //
        // Append a new item to the current parent's list of children.
        GkGenericTreeViewItem *parent = rootItems.last();
        parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
        for (qint32 column = 0; column < columnData.size(); ++column) {
            parent->child(parent->childCount() - 1)->setData(column, columnData[column]);
        }
    }

    return;
}

/**
 * @brief GkGenericTreeViewModel::insertData inserts a QVector() of data values into a given QTreeView where the model
 * is set towards this class.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param values The values themselves to be inserted into the QTreeView.
 * @param rootItem The possible parent of the value to be modified.
 * @note Editable Tree Model Example <https://doc.qt.io/qt-5/qtwidgets-itemviews-editabletreemodel-example.html>.
 */
void GkGenericTreeViewModel::insertData(const QVector<QVariant> &values, GkGenericTreeViewItem *rootItem)
{
    if (!values.isEmpty()) {
        for (const auto &value: values) {
            if (value.isValid()) { // TODO: Improve the performance of this function!
                QVector<QVariant> columnData;
                QList<GkGenericTreeViewItem *> rootItems;
                rootItems << rootItem;
                columnData << value;

                //
                // Append a new item to the current parent's list of children.
                GkGenericTreeViewItem *parent = rootItems.last();
                parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
                for (qint32 column = 0; column < columnData.size(); ++column) {
                    parent->child(parent->childCount() - 1)->setData(column, columnData[column]);
                }
            }
        }
    }

    return;
}

/**
 * @brief GkGenericTreeViewModel::removeData removes supplied data from the given QTreeView.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param row The exact row which is to be erased.
 */
void GkGenericTreeViewModel::removeData(const qint32 &row)
{
    m_rootItem->removeChildren(row, 1);

    return;
}

/**
 * @brief GkGenericTreeViewModel::buildMap builds a map to a QModelIndex from a given QTreeView.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>, tunglt <https://stackoverflow.com/a/54150063>
 * @param index
 * @param model
 * @param itemMap
 */
void GkGenericTreeViewModel::buildMap(const QModelIndex &index, const QAbstractItemModel *model, QMap<GkGenericTreeViewItem *, QModelIndex> &itemMap)
{
    if (!index.isValid()) {
        return;
    }

    GkGenericTreeViewItem *item = static_cast<GkGenericTreeViewItem *>(index.internalPointer());
    itemMap.insert(item, index);

    qint32 rows = model->rowCount(index);
    qint32 cols = model->columnCount(index);

    for (qint32 i = 0; i < rows; ++i) {
        for (qint32 j = 0; j < cols; ++j) {
            QModelIndex idChild = model->index(i, j, index);
            buildMap(idChild, model, itemMap);
        }
    }

    return;
}

GkGenericTreeViewItem *GkGenericTreeViewModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        GkGenericTreeViewItem *item = static_cast<GkGenericTreeViewItem *>(index.internalPointer());
        if (item) {
            return item;
        }
    }

    return m_rootItem;
}
