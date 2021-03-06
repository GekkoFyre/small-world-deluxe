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

#include "src/models/tableview/gk_solar_weather_forecast_model.hpp"
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
 * @brief GkSolarWeatherForeModel::GkSolarWeatherForeModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param parent
 */
GkSolarWeatherForeModel::GkSolarWeatherForeModel(QPointer<QTableView> tableView,
                                                 QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                                 QWidget *parent) : QAbstractTableModel(parent)
{
    setParent(parent);
    m_xmppClient = std::move(xmppClient);

    return;
}

/**
 * @brief GkSolarWeatherForeModel::GkSolarWeatherForeModelauthor Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkSolarWeatherForeModel::~GkSolarWeatherForeModel()
{
    return;
}

/**
 * @brief GkSolarWeatherForeModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_logs
 */
void GkSolarWeatherForeModel::populateData(const QList<GkRecvMsgsTableViewModel> &data_list)
{
    std::lock_guard<std::mutex> lock_guard(m_dataBatchMutex);
    m_data.clear();
    m_data = data_list;

    return;
}

/**
 * @brief GkSolarWeatherForeModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The identity of the user, as it should appear on the XMPP server.
 * @param msg The message data to be inserted.
 * @param timestamp The date/time to be inserted.
 * @param doNotAddToTable As the variable name suggests, do not make any modifications to the QTableView itself. Only
 * modify `m_data` instead.
 */
void GkSolarWeatherForeModel::insertData(const QString &bareJid, const QString &msg, const QDateTime &timestamp)
{
    try {
        std::lock_guard<std::mutex> lock_guard(m_dataBatchMutex);
        qint32 idx = -1;
        if (!m_data.isEmpty()) {
            //
            // Find the nearest timestamp to the one provided!
            const auto comp_timestamp = m_xmppClient->compareTimestamps(std::vector<GkRecvMsgsTableViewModel>(m_data.begin(), m_data.end()),
                                                                        timestamp.toMSecsSinceEpoch());
            for (qint32 i = 0; i < m_data.size(); ++i) {
                if (m_data.at(i).timestamp.toMSecsSinceEpoch() == comp_timestamp) {
                    idx = i;
                }
            }
        }

        GkRecvMsgsTableViewModel recvMsg;
        recvMsg.timestamp = timestamp;
        recvMsg.bareJid = bareJid;
        recvMsg.message = msg;
        recvMsg.nickName = m_xmppClient->getJidNickname(recvMsg.bareJid);

        //
        // Insert the data into the QTableView itself!
        beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
        if (!m_data.isEmpty() && idx >= 0) {
            m_data.insert(m_data.begin() + idx, recvMsg);
        } else {
            m_data.push_back(recvMsg);
        }

        endInsertRows();
        auto top = this->createIndex((m_data.count() + 1), 0, nullptr);
        auto bottom = this->createIndex((m_data.count() + 1), GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
        emit dataChanged(top, bottom);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkSolarWeatherForeModel::removeData removes ALL the data from the given QTableView model.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
qint32 GkSolarWeatherForeModel::removeData()
{
    try {
        std::lock_guard<std::mutex> lock_guard(m_dataBatchMutex);
        if (!m_data.isEmpty()) {
            qint32 counter = 0;
            qint32 data_size = m_data.count();
            for (auto iter = m_data.begin(); iter != m_data.end(); ++iter) {
                ++counter;
                beginRemoveRows(QModelIndex(), (m_data.count() - 1), (m_data.count() - 1));
                iter = m_data.erase(iter);
                endRemoveRows();

                if (data_size == counter) {
                    break;
                }
            }

            auto top = this->createIndex((m_data.count() - counter), 0, nullptr);
            auto bottom = this->createIndex((m_data.count() - counter), GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
            emit dataChanged(top, bottom);

            return counter;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return 0;
}

/**
 * @brief GkSolarWeatherForeModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param timestamp The Date/Time at which the message is to be removed from.
 * @param bareJid The username for which the message is to be removed from.
 * @return The row at which the data was removed from.
 */
qint32 GkSolarWeatherForeModel::removeData(const QDateTime &timestamp, const QString &bareJid)
{
    try {
        std::lock_guard<std::mutex> lock_guard(m_dataBatchMutex);
        if (!m_data.isEmpty()) {
            qint32 counter = 0;
            for (auto iter = m_data.begin(); iter != m_data.end(); ++iter) {
                ++counter;
                if (iter->timestamp == timestamp && iter->bareJid == bareJid) {
                    beginRemoveRows(QModelIndex(), (m_data.count() - 1), (m_data.count() - 1));
                    iter = m_data.erase(iter);
                    endRemoveRows();
                    break;
                }
            }

            auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
            auto bottom = this->createIndex((m_data.count() - 1), GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
            emit dataChanged(top, bottom);

            return counter;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return 0;
}

/**
 * @brief GkSolarWeatherForeModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkSolarWeatherForeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

/**
 * @brief GkSolarWeatherForeModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkSolarWeatherForeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_TOTAL_IDX; // Make sure to add the total of columns from within `GkSolarWeatherForeModel::headerData()`!
}

/**
 * @brief GkSolarWeatherForeModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkSolarWeatherForeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const QDateTime row_timestamp = m_data[index.row()].timestamp;
    const QString row_nickname = m_data[index.row()].nickName;
    const QString row_message = m_data[index.row()].message;

    switch (index.column()) {
        case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_DATETIME_IDX:
            return row_timestamp.toLocalTime().toString("[ dd MMM yy ] h:mm:ss ap");
        case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_NICKNAME_IDX:
            return row_nickname;
        case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_MSG_IDX:
            return row_message;
        default:
            break;
    }

    return QVariant();
}

/**
 * @brief GkSolarWeatherForeModel::setData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param value
 * @param role
 * @return
 */
bool GkSolarWeatherForeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::DisplayRole) {
        qint32 row = index.row();
        GkXmpp::GkRecvMsgsTableViewModel result;
        switch (index.column()) {
            case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_DATETIME_IDX:
                result.timestamp = value.toDateTime();
                break;
            case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_NICKNAME_IDX:
                result.nickName = value.toString();
                break;
            case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_MSG_IDX:
                result.message = value.toString();
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
 * @brief GkSolarWeatherForeModel::flags
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @return
 */
Qt::ItemFlags GkSolarWeatherForeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractTableModel::flags(index) | Qt::NoItemFlags;
}

/**
 * @brief GkSolarWeatherForeModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant GkSolarWeatherForeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_DATETIME_IDX:
                return tr("Timestamp");
            case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_NICKNAME_IDX:
                return tr("Nickname");
            case GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_MSG_IDX:
                return tr("Message");
            default:
                break;
        }
    }

    return QVariant();
}
