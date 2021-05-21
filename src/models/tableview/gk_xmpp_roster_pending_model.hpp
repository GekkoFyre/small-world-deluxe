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

#pragma once

#include "src/defines.hpp"
#include "src/gk_xmpp_client.hpp"
#include <memory>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QTableView>
#include <QModelIndex>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

namespace GekkoFyre {

class GkXmppRosterPendingTableViewModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit GkXmppRosterPendingTableViewModel(QPointer<QTableView> tableView, QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                               QWidget *parent = nullptr);
    ~GkXmppRosterPendingTableViewModel() override;

    void populateData(const QList<GekkoFyre::Network::GkXmpp::GkPendingTableViewModel> &data_list);
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

public slots:
    void insertData(const GekkoFyre::Network::GkXmpp::GkPendingTableViewModel &data);
    qint32 removeData(const QString &bareJid);

private:
    QList<GekkoFyre::Network::GkXmpp::GkPendingTableViewModel> m_data;

    QPointer<QSortFilterProxyModel> proxyModel;
    QPointer<QTableView> table;
    QPointer<GekkoFyre::GkXmppClient> m_xmppClient;

    QMutex dataBatchMutex;
};
};
