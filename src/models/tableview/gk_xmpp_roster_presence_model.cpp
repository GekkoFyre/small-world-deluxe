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

#include "src/models/tableview/gk_xmpp_roster_presence_model.hpp"
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
using namespace Security;

/**
 * @brief GkXmppRosterPresenceTableViewModel::GkXmppRosterPresenceTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param parent
 */
GkXmppRosterPresenceTableViewModel::GkXmppRosterPresenceTableViewModel(QPointer<QTableView> tableView,
                                                                       QWidget *parent) : QAbstractTableModel(parent)
{
    setParent(parent);

    proxyModel = new QSortFilterProxyModel(parent);
    tableView->setModel(proxyModel);
    proxyModel->setSourceModel(this);

    return;
}

/**
 * @brief GkXmppRosterPresenceTableViewModel::~GkXmppRosterPresenceTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkXmppRosterPresenceTableViewModel::~GkXmppRosterPresenceTableViewModel()
{
    return;
}

/**
 * @brief GkXmppRosterPresenceTableViewModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_logs
 */
void GkXmppRosterPresenceTableViewModel::populateData(const QList<GkPresenceTableViewModel> &data_list)
{
    dataBatchMutex.lock();

    m_data.clear();
    m_data = data_list;

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkXmppRosterPresenceTableViewModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkXmppRosterPresenceTableViewModel::insertData(const GkPresenceTableViewModel &data)
{
    dataBatchMutex.lock();

    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data.append(data);
    endInsertRows();

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkXmppRosterPresenceTableViewModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkXmppRosterPresenceTableViewModel::removeData(const QString &bareJid)
{
    dataBatchMutex.lock();
    for (auto iter = m_data.begin(); iter != m_data.end();) {
        if (iter->bareJid == bareJid) {
            beginRemoveRows(QModelIndex(), (m_data.count() - 1), (m_data.count() - 1));
            m_data.erase(iter);
            endRemoveRows();
        } else {
            ++iter;
        }
    }

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkXmppRosterPresenceTableViewModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkXmppRosterPresenceTableViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

/**
 * @brief GkXmppRosterPresenceTableViewModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkXmppRosterPresenceTableViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_TOTAL_IDX; // Make sure to add the total of columns from within `GkXmppRosterPresenceTableViewModel::headerData()`!
}

/**
 * @brief GkXmppRosterPresenceTableViewModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkXmppRosterPresenceTableViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const QIcon row_presence = m_data[index.row()].presence;
    const QString row_bareJid = m_data[index.row()].bareJid;
    const QString row_nickname = m_data[index.row()].nickName;

    switch (index.column()) {
        case GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_PRESENCE_IDX:
            return row_presence;
        case GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX:
            return row_bareJid;
        case GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_NICKNAME_IDX:
            return row_nickname;
        default:
            break;
    }

    return QVariant();
}

/**
 * @brief GkXmppRosterPresenceTableViewModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant GkXmppRosterPresenceTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_PRESENCE_IDX:
                return tr("Presence");
            case GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX:
                return tr("ID#");
            case GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_NICKNAME_IDX:
                return tr("Nickname");
            default:
                break;
        }
    }

    return QVariant();
}
