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

#include "src/models/tableview/xmpp/gk_callsigns_roster_pending_model.hpp"
#include <utility>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHeaderView>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

/**
 * @brief GkCallsignsRosterPendingModel::GkCallsignsRosterPendingModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param parent
 */
GkCallsignsRosterPendingModel::GkCallsignsRosterPendingModel(QWidget *parent) : QAbstractTableModel(parent)
{
    setParent(parent);

    table = new QTableView(parent);
    QPointer<QVBoxLayout> layout = new QVBoxLayout(parent);
    proxyModel = new QSortFilterProxyModel(parent);

    table->setModel(proxyModel);
    layout->addWidget(table);
    proxyModel->setSourceModel(this);

    return;
}

/**
 * @brief GkCallsignsRosterPendingModel::~GkCallsignsRosterPendingModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkCallsignsRosterPendingModel::~GkCallsignsRosterPendingModel()
{
    return;
}

/**
 * @brief GkCallsignsRosterPendingModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_logs
 */
void GkCallsignsRosterPendingModel::populateData(const QList<GkEventLogging> &event_logs)
{
    return;
}

/**
 * @brief GkCallsignsRosterPendingModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkCallsignsRosterPendingModel::insertData(const QString &bareJid, const QXmppPresence &presence)
{
    dataBatchMutex.lock();

    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    Network::GkXmpp::GkXmppPendingCallsign xmppPendingCallsign;
    xmppPendingCallsign.bareJid = bareJid;
    xmppPendingCallsign.presence = presence;
    m_data.append(xmppPendingCallsign);
    endInsertRows();

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_XMPP_ROSTER_PENDING_CALLSIGNS_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkCallsignsRosterPendingModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkCallsignsRosterPendingModel::removeData(const QString &bareJid)
{
    dataBatchMutex.lock();
    for (auto it = m_data.begin(); it != m_data.end();) {
        if (it->bareJid == bareJid) {
            beginRemoveRows(QModelIndex(), (m_data.count() - 1), (m_data.count() - 1));
            it = m_data.erase(it);
            endRemoveRows();
        } else {
            ++it;
        }
    }

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_XMPP_ROSTER_PENDING_CALLSIGNS_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkCallsignsRosterPendingModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkCallsignsRosterPendingModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.size();
}

/**
 * @brief GkCallsignsRosterPendingModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkCallsignsRosterPendingModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_XMPP_ROSTER_PENDING_CALLSIGNS_MODEL_TOTAL_IDX; // Make sure to add the total of columns from within `GkCallsignsRosterPendingModel::headerData()`!
}

/**
 * @brief GkCallsignsRosterPendingModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkCallsignsRosterPendingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    auto xmppPendingCallsign = m_data[index.row()];
    switch (index.column()) {
        case GK_XMPP_ROSTER_PENDING_CALLSIGNS_MODEL_PRESENCE_IDX:
            return xmppPendingCallsign.presence.statusText();
        case GK_XMPP_ROSTER_PENDING_CALLSIGNS_MODEL_NICKNAME_IDX:
            return xmppPendingCallsign.bareJid;
    }

    return QVariant();
}

/**
 * @brief GkCallsignsRosterPendingModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant GkCallsignsRosterPendingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case GK_XMPP_ROSTER_PENDING_CALLSIGNS_MODEL_PRESENCE_IDX:
                return tr("Presence / Status");
            case GK_XMPP_ROSTER_PENDING_CALLSIGNS_MODEL_NICKNAME_IDX:
                return tr("Nickname");
        }
    }

    return QVariant();
}
