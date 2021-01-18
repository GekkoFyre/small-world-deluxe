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

#include "src/models/tableview/gk_active_msgs_model.hpp"
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
 * @brief GkActiveMsgsTableViewModel::GkActiveMsgsTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param parent
 */
GkActiveMsgsTableViewModel::GkActiveMsgsTableViewModel(QPointer<GkLevelDb> database, QWidget *parent)
    : QAbstractTableModel(parent)
{
    setParent(parent);
    gkDb = std::move(database);

    table = new QTableView(parent);
    QPointer<QVBoxLayout> layout = new QVBoxLayout(parent);
    proxyModel = new QSortFilterProxyModel(parent);

    table->setModel(proxyModel);
    layout->addWidget(table);
    proxyModel->setSourceModel(this);

    return;
}

/**
 * @brief GkActiveMsgsTableViewModel::~GkActiveMsgsTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkActiveMsgsTableViewModel::~GkActiveMsgsTableViewModel()
{
    return;
}

/**
 * @brief GkActiveMsgsTableViewModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_logs
 */
void GkActiveMsgsTableViewModel::populateData(const QList<GkEventLogging> &event_logs)
{
    return;
}

/**
 * @brief GkActiveMsgsTableViewModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkActiveMsgsTableViewModel::insertData(const GkEventLogging &event)
{
    dataBatchMutex.lock();

    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data.append(event);
    endInsertRows();

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_ACTIVE_MSGS_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkActiveMsgsTableViewModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkActiveMsgsTableViewModel::removeData(const GkEventLogging &event)
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
    auto bottom = this->createIndex((m_data.count() - 1), GK_ACTIVE_MSGS_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkActiveMsgsTableViewModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkActiveMsgsTableViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

/**
 * @brief GkActiveMsgsTableViewModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkActiveMsgsTableViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_ACTIVE_MSGS_TABLEVIEW_MODEL_TOTAL_IDX; // Make sure to add the total of columns from within `GkActiveMsgsTableViewModel::headerData()`!
}

/**
 * @brief GkActiveMsgsTableViewModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkActiveMsgsTableViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    return QVariant();
}

/**
 * @brief GkActiveMsgsTableViewModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant GkActiveMsgsTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case GK_ACTIVE_MSGS_TABLEVIEW_MODEL_OFFSET_IDX:
                return tr("Offset");
            case GK_ACTIVE_MSGS_TABLEVIEW_MODEL_DATETIME_IDX:
                return tr("Date & Time");
            case GK_ACTIVE_MSGS_TABLEVIEW_MODEL_AGE_IDX:
                return tr("Age");
            case GK_ACTIVE_MSGS_TABLEVIEW_MODEL_SNR_IDX:
                return tr("SNR");
            case GK_ACTIVE_MSGS_TABLEVIEW_MODEL_MSG_IDX:
                return tr("Message(s)");
        }
    }

    return QVariant();
}
