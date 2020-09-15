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
 **   Copyright (C) 2020. GekkoFyre.
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

#include "src/models/tableview/gk_callsign_msgs_model.hpp"
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
 * @brief GkCallsignMsgsTableViewModel::GkCallsignMsgsTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param parent
 */
GkCallsignMsgsTableViewModel::GkCallsignMsgsTableViewModel(QPointer<GkLevelDb> database, QWidget *parent)
    : QAbstractTableModel(parent)
{
    setParent(parent);
    GkDb = std::move(database);

    table = new QTableView(parent);
    QPointer<QVBoxLayout> layout = new QVBoxLayout(parent);
    proxyModel = new QSortFilterProxyModel(parent);

    table->setModel(proxyModel);
    layout->addWidget(table);
    proxyModel->setSourceModel(this);

    return;
}

/**
 * @brief GkCallsignMsgsTableViewModel::~GkCallsignMsgsTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkCallsignMsgsTableViewModel::~GkCallsignMsgsTableViewModel()
{
    return;
}

/**
 * @brief GkCallsignMsgsTableViewModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_logs
 */
void GkCallsignMsgsTableViewModel::populateData(const QList<GkEventLogging> &event_logs)
{
    return;
}

/**
 * @brief GkCallsignMsgsTableViewModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkCallsignMsgsTableViewModel::insertData(const GkEventLogging &event)
{
    dataBatchMutex.lock();

    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data.append(event);
    endInsertRows();

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_CSIGN_MSGS_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkCallsignMsgsTableViewModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkCallsignMsgsTableViewModel::removeData(const GkEventLogging &event)
{
    dataBatchMutex.lock();

    for (int i = 0; i < m_data.size(); ++i) {
        if (m_data[i].event_no == event.event_no) {
            beginRemoveRows(QModelIndex(), (m_data.count() - 1), (m_data.count() - 1));
            m_data.removeAt(i); // Remove any occurrence of this value, one at a time!
            endRemoveRows();
        }
    }

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_CSIGN_MSGS_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkCallsignMsgsTableViewModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkCallsignMsgsTableViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

/**
 * @brief GkCallsignMsgsTableViewModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkCallsignMsgsTableViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_CSIGN_MSGS_TABLEVIEW_MODEL_TOTAL_IDX; // Make sure to add the total of columns from within `GkCallsignMsgsTableViewModel::headerData()`!
}

/**
 * @brief GkCallsignMsgsTableViewModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkCallsignMsgsTableViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    return QVariant();
}

/**
 * @brief GkCallsignMsgsTableViewModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant GkCallsignMsgsTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case GK_CSIGN_MSGS_TABLEVIEW_MODEL_CALLSIGN_IDX:
                return tr("Callsign(s)");
            case GK_CSIGN_MSGS_TABLEVIEW_MODEL_DATETIME_IDX:
                return tr("Date & Time");
            case GK_CSIGN_MSGS_TABLEVIEW_MODEL_AGE_IDX:
                return tr("Age");
            case GK_CSIGN_MSGS_TABLEVIEW_MODEL_SNR_IDX:
                return tr("SNR");
            case GK_CSIGN_MSGS_TABLEVIEW_MODEL_OFFSET_IDX:
                return tr("Offset");
            case GK_CSIGN_MSGS_TABLEVIEW_MODEL_CHECKMARK_IDX:
                return tr("✔");
            case GK_CSIGN_MSGS_TABLEVIEW_MODEL_NAME_IDX:
                return tr("Name");
            case GK_CSIGN_MSGS_TABLEVIEW_MODEL_COMMENT_IDX:
                return tr("Comment");
        }
    }

    return QVariant();
}
