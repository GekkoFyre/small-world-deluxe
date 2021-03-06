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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include <memory>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QPointer>
#include <QTreeView>
#include <QStringList>
#include <QModelIndex>
#include <QScopedPointer>
#include <QAbstractItemModel>

namespace GekkoFyre {

class GkXmppMucRosterTreeViewItem {

public:
    explicit GkXmppMucRosterTreeViewItem(const QVector<QVariant> &data, GkXmppMucRosterTreeViewItem *parent = nullptr);
    ~GkXmppMucRosterTreeViewItem();

    GkXmppMucRosterTreeViewItem *child(qint32 number);
    qint32 childCount() const;
    qint32 columnCount() const;
    QVariant data(qint32 column) const;
    bool insertChildren(qint32 position, qint32 count, qint32 columns);
    bool insertColumns(qint32 position, qint32 columns);
    GkXmppMucRosterTreeViewItem *parent();
    bool removeChildren(qint32 position, qint32 count);
    bool removeColumns(qint32 position, qint32 columns);
    qint32 childNumber() const;
    bool setData(qint32 column, const QVariant &value);

private:
    QVector<GkXmppMucRosterTreeViewItem *> childItems;
    QVector<QVariant> itemData;
    GkXmppMucRosterTreeViewItem *parentItem;
};

class GkXmppMucRosterTreeViewModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit GkXmppMucRosterTreeViewModel(const QStringList &headers, QObject *parent = nullptr);
    ~GkXmppMucRosterTreeViewModel() override;

    QVariant data(const QModelIndex &index, qint32 role) const override;
    QVariant headerData(qint32 section, Qt::Orientation orientation, qint32 role = Qt::DisplayRole) const override;

    QModelIndex index(qint32 row, qint32 column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QModelIndex indexForTreeItem(GkXmppMucRosterTreeViewItem *item);

    qint32 rowCount(const QModelIndex &parent = QModelIndex()) const override;
    qint32 columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, qint32 role = Qt::EditRole) override;
    bool setHeaderData(qint32 section, Qt::Orientation orientation, const QVariant &value, qint32 role = Qt::EditRole) override;

    bool insertColumns(qint32 position, qint32 columns, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(qint32 position, qint32 columns, const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(qint32 position, qint32 rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(qint32 position, qint32 rows, const QModelIndex &parent = QModelIndex()) override;

private:
    GkXmppMucRosterTreeViewItem *getItem(const QModelIndex &index) const;

    GkXmppMucRosterTreeViewItem *m_rootItem;
};
};
