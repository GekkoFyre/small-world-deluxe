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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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

#include "src/models/treeview/xmpp/gk_xmpp_muc_roster_model.hpp"
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

/**
 * @brief GkXmppMucRosterTreeViewItem::GkXmppMucRosterTreeViewItem
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param parent
 */
GkXmppMucRosterTreeViewItem::GkXmppMucRosterTreeViewItem(const QVector<QVariant> &data, GkXmppMucRosterTreeViewItem *parent) : itemData(data), parentItem(parent)
{
    return;
}

GkXmppMucRosterTreeViewItem::~GkXmppMucRosterTreeViewItem()
{
    qDeleteAll(childItems);
    return;
}

GkXmppMucRosterTreeViewItem *GkXmppMucRosterTreeViewItem::child(qint32 number)
{
    if (number < 0 || number >= childItems.size()) {
        return nullptr;
    }

    return childItems.at(number);
}

qint32 GkXmppMucRosterTreeViewItem::childCount() const
{
    return childItems.count();
}

qint32 GkXmppMucRosterTreeViewItem::columnCount() const
{
    return itemData.count();
}

QVariant GkXmppMucRosterTreeViewItem::data(qint32 column) const
{
    if (column < 0 || column >= itemData.size()) {
        return QVariant();
    }

    return itemData.at(column);
}

bool GkXmppMucRosterTreeViewItem::insertChildren(qint32 position, qint32 count, qint32 columns)
{
    if (position < 0 || position > childItems.size()) {
        return false;
    }

    for (qint32 row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        GkXmppMucRosterTreeViewItem *item = new GkXmppMucRosterTreeViewItem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

bool GkXmppMucRosterTreeViewItem::insertColumns(qint32 position, qint32 columns)
{
    if (position < 0 || position > itemData.size()) {
        return false;
    }

    for (qint32 column = 0; column < columns; ++column) {
        itemData.insert(position, QVariant());
    }

    for (GkXmppMucRosterTreeViewItem *child : qAsConst(childItems)) {
        child->insertColumns(position, columns);
    }

    return true;
}

GkXmppMucRosterTreeViewItem *GkXmppMucRosterTreeViewItem::parent()
{
    return parentItem;
}

bool GkXmppMucRosterTreeViewItem::removeChildren(qint32 position, qint32 count)
{
    if (position < 0 || position + count > childItems.size()) {
        return false;
    }

    for (qint32 row = 0; row < count; ++row) {
        delete childItems.takeAt(position);
    }

    return true;
}

bool GkXmppMucRosterTreeViewItem::removeColumns(qint32 position, qint32 columns)
{
    if (position < 0 || position + columns > itemData.size()) {
        return false;
    }

    for (qint32 column = 0; column < columns; ++column) {
        itemData.remove(position);
    }

    for (GkXmppMucRosterTreeViewItem *child : qAsConst(childItems)) {
        child->removeColumns(position, columns);
    }

    return true;
}

qint32 GkXmppMucRosterTreeViewItem::childNumber() const
{
    if (parentItem) {
        return parentItem->childItems.indexOf(const_cast<GkXmppMucRosterTreeViewItem *>(this));
    }

    return 0;
}

bool GkXmppMucRosterTreeViewItem::setData(qint32 column, const QVariant &value)
{
    if (column < 0 || column >= itemData.size()) {
        return false;
    }

    itemData[column] = value;
    return true;
}

/**
 * @brief GkXmppMucRosterTreeViewModel::GkXmppMucRosterTreeViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param headers
 * @param data
 * @param parent
 */
GkXmppMucRosterTreeViewModel::GkXmppMucRosterTreeViewModel(const QStringList &headers, QObject *parent) : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    for (const QString &header: headers) {
        rootData << header;
    }

    m_rootItem = new GkXmppMucRosterTreeViewItem(rootData);
    return;
}

GkXmppMucRosterTreeViewModel::~GkXmppMucRosterTreeViewModel()
{
    delete m_rootItem;
    return;
}

QVariant GkXmppMucRosterTreeViewModel::data(const QModelIndex &index, qint32 role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    GkXmppMucRosterTreeViewItem *item = getItem(index);
    return item->data(index.column());
}

QVariant GkXmppMucRosterTreeViewModel::headerData(qint32 section, Qt::Orientation orientation, qint32 role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_rootItem->data(section);
    }

    return QVariant();
}

QModelIndex GkXmppMucRosterTreeViewModel::index(qint32 row, qint32 column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return QModelIndex();
    }

    GkXmppMucRosterTreeViewItem *parentItem = getItem(parent);
    if (!parentItem) {
        return QModelIndex();
    }

    GkXmppMucRosterTreeViewItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    }

    return QModelIndex();
}

QModelIndex GkXmppMucRosterTreeViewModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    GkXmppMucRosterTreeViewItem *childItem = getItem(index);
    GkXmppMucRosterTreeViewItem *parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == m_rootItem || !parentItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

/**
 * @brief GkXmppMucRosterTreeViewModel::indexForTreeItem calculates a QModelIndex, ready for use in creating a child, by
 * external QTableView functions.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param item
 * @return The calculated QModelIndex in question.
 */
QModelIndex GkXmppMucRosterTreeViewModel::indexForTreeItem(GkXmppMucRosterTreeViewItem *item)
{
    return createIndex(item->childNumber(), 0, item);
}

qint32 GkXmppMucRosterTreeViewModel::rowCount(const QModelIndex &parent) const
{
    const GkXmppMucRosterTreeViewItem *parentItem = getItem(parent);
    return parentItem ? parentItem->childCount() : 0;
}

qint32 GkXmppMucRosterTreeViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_rootItem->columnCount();
}

Qt::ItemFlags GkXmppMucRosterTreeViewModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

bool GkXmppMucRosterTreeViewModel::setData(const QModelIndex &index, const QVariant &value, qint32 role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    GkXmppMucRosterTreeViewItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result) {
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    }

    return result;
}

bool
GkXmppMucRosterTreeViewModel::setHeaderData(qint32 section, Qt::Orientation orientation, const QVariant &value, qint32 role)
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

bool GkXmppMucRosterTreeViewModel::insertColumns(qint32 position, qint32 columns, const QModelIndex &parent)
{
    beginInsertColumns(parent, position, position + columns - 1);
    const bool success = m_rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool GkXmppMucRosterTreeViewModel::removeColumns(qint32 position, qint32 columns, const QModelIndex &parent)
{
    beginRemoveColumns(parent, position, position + columns - 1);
    const bool success = m_rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (m_rootItem->columnCount() == 0) {
        removeRows(0, rowCount());
    }

    return success;
}

bool GkXmppMucRosterTreeViewModel::insertRows(qint32 position, qint32 rows, const QModelIndex &parent)
{
    GkXmppMucRosterTreeViewItem *parentItem = getItem(parent);
    if (!parentItem) {
        return false;
    }

    beginInsertRows(parent, position, position + rows - 1);
    const bool success = parentItem->insertChildren(position, rows, m_rootItem->columnCount());
    endInsertRows();

    return success;
}

bool GkXmppMucRosterTreeViewModel::removeRows(qint32 position, qint32 rows, const QModelIndex &parent)
{
    GkXmppMucRosterTreeViewItem *parentItem = getItem(parent);
    if (!parentItem) {
        return false;
    }

    beginRemoveRows(parent, position, position + rows - 1);
    const bool success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

GkXmppMucRosterTreeViewItem *GkXmppMucRosterTreeViewModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        GkXmppMucRosterTreeViewItem *item = static_cast<GkXmppMucRosterTreeViewItem *>(index.internalPointer());
        if (item) {
            return item;
        }
    }

    return m_rootItem;
}
