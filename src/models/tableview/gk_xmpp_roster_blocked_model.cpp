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

#include "src/models/tableview/gk_xmpp_roster_blocked_model.hpp"
#include <utility>
#include <QVBoxLayout>
#include <QHeaderView>

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
 * @brief GkXmppRosterBlockedTableViewModel::GkXmppRosterBlockedTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param parent
 */
GkXmppRosterBlockedTableViewModel::GkXmppRosterBlockedTableViewModel(QPointer<QTableView> tableView,
                                                                     QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                                                     QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                                                     QWidget *parent) : QAbstractTableModel(parent)
{
    setParent(parent);

    gkStringFuncs = std::move(stringFuncs);
    proxyModel = new QSortFilterProxyModel(parent);
    tableView->setModel(proxyModel);
    m_xmppClient = std::move(xmppClient);
    proxyModel->setSourceModel(this);

    return;
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::~GkXmppRosterBlockedTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkXmppRosterBlockedTableViewModel::~GkXmppRosterBlockedTableViewModel()
{
    return;
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_logs
 */
void GkXmppRosterBlockedTableViewModel::populateData(const QList<GkBlockedTableViewModel> &data_list)
{
    dataBatchMutex.lock();

    m_data.clear();
    m_data = data_list;

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkXmppRosterBlockedTableViewModel::insertData(const GkBlockedTableViewModel &data)
{
    dataBatchMutex.lock();

    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data.append(data);
    endInsertRows();

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The username for which the roster item is to be removed from.
 * @return The row at which the data was removed from.
 */
qint32 GkXmppRosterBlockedTableViewModel::removeData(const QString &bareJid)
{
    dataBatchMutex.lock();
    qint32 counter = 0;
    for (auto iter = m_data.begin(); iter != m_data.end(); ++iter) {
        ++counter;
        if (iter->bareJid == bareJid) {
            beginRemoveRows(QModelIndex(), (m_data.count() - 1), (m_data.count() - 1));
            iter = m_data.erase(iter);
            endRemoveRows();
            break;
        }
    }

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return counter;
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkXmppRosterBlockedTableViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkXmppRosterBlockedTableViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_TOTAL_IDX; // Make sure to add the total of columns from within `GkXmppRosterBlockedTableViewModel::headerData()`!
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkXmppRosterBlockedTableViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const QString row_bareJid = m_data[index.row()].bareJid;
    const QString row_reason = m_data[index.row()].reason;

    switch (index.column()) {
        case GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_BAREJID_IDX:
            return gkStringFuncs->getXmppUsername(row_bareJid);
        case GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_REASON_IDX:
            return row_reason;
        default:
            break;
    }

    return QVariant();
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::setData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param value
 * @param role
 * @return
 */
bool GkXmppRosterBlockedTableViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::DisplayRole) {
        qint32 row = index.row();
        GkXmpp::GkBlockedTableViewModel result;
        switch (index.column()) {
            case GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_BAREJID_IDX:
                result.bareJid = value.toString();
                break;
            case GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_REASON_IDX:
                result.reason = value.toString();
                break;
            default:
                return false;
        }

        emit dataChanged(index, index);
        return true;
    }

    return false;
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::flags
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @return
 */
Qt::ItemFlags GkXmppRosterBlockedTableViewModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractTableModel::flags(index) | Qt::NoItemFlags;
}

/**
 * @brief GkXmppRosterBlockedTableViewModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant GkXmppRosterBlockedTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_BAREJID_IDX:
                return tr("ID#");
            case GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_REASON_IDX:
                return tr("Reason for block");
            default:
                break;
        }
    }

    return QVariant();
}
