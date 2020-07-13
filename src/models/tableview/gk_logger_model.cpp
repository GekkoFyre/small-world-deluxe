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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/models/tableview/gk_logger_model.hpp" 
#include <utility>
#include <QDateTime>
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

GkEventLoggerTableViewModel::GkEventLoggerTableViewModel(std::shared_ptr<GkLevelDb> database, QWidget *parent)
    : QAbstractTableModel(parent)
{
    GkDb = std::move(database);

    table = new QTableView(parent);
    table->setModel(this);

    return;
}

GkEventLoggerTableViewModel::~GkEventLoggerTableViewModel()
{
    return;
}

/**
 * @brief GkEventLoggerTableViewModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_logs
 */
void GkEventLoggerTableViewModel::populateData(const QList<GkEventLogging> &event_logs)
{
    m_data.clear();
    m_data = event_logs;

    return;
}

void GkEventLoggerTableViewModel::insertData(const GkEventLogging &event)
{
    m_data.push_back(event);

    return;
}

void GkEventLoggerTableViewModel::removeData(const GkEventLogging &event)
{
    for (int i = 0; i < m_data.size(); ++i) {
        if (m_data[i].event_no == event.event_no) {
            m_data.removeAt(i); // Remove any occurrence of this value, one at a time!
        }
    }

    return;
}

int GkEventLoggerTableViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

int GkEventLoggerTableViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 4; // Make sure to add the total of columns from within `GkEventLoggerTableViewModel::headerData()`!
}

QVariant GkEventLoggerTableViewModel::data(const QModelIndex &index, int role) const
{
    int row_event_no = m_data[index.row()].event_no;

    qint64 row_date_time = m_data[index.row()].mesg.date;
    QDateTime timestamp;
    timestamp.setMSecsSinceEpoch(row_date_time);

    QString row_severity = GkDb->convSeverityToStr(m_data[index.row()].mesg.severity);
    QString row_message = m_data[index.row()].mesg.message;

    switch (index.column()) {
    case GK_EVENTLOG_TABLEVIEW_MODEL_EVENT_NO_IDX:
        return row_event_no;
    case GK_EVENTLOG_TABLEVIEW_MODEL_DATETIME_IDX:
        return timestamp.toString(tr("yyyy-MM-dd hh:mm:ss"));
    case GK_EVENTLOG_TABLEVIEW_MODEL_SEVERITY_IDX:
        return row_severity;
    case GK_EVENTLOG_TABLEVIEW_MODEL_MESSAGE_IDX:
        return row_message;
    }

    return QVariant();
}

QVariant GkEventLoggerTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case GK_EVENTLOG_TABLEVIEW_MODEL_EVENT_NO_IDX:
            return tr("Event No.");
        case GK_EVENTLOG_TABLEVIEW_MODEL_DATETIME_IDX:
            return tr("Date & Time");
        case GK_EVENTLOG_TABLEVIEW_MODEL_SEVERITY_IDX:
            return tr("Severity");
        case GK_EVENTLOG_TABLEVIEW_MODEL_MESSAGE_IDX:
            return tr("Message");
        }
    }

    return QVariant();
}
