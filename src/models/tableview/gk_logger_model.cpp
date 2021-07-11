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

#include "src/models/tableview/gk_logger_model.hpp" 
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
 * @brief GkEventLoggerTableViewModel::GkEventLoggerTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param parent
 */
GkEventLoggerTableViewModel::GkEventLoggerTableViewModel(QPointer<GkLevelDb> database, QWidget *parent)
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
 * @brief GkEventLoggerTableViewModel::~GkEventLoggerTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkEventLoggerTableViewModel::~GkEventLoggerTableViewModel()
{
    //
    // Send a stacktrace via Brakepad / Crashpad by way of Sentry upon exit, whether it be a
    // normal exit or one made by a system exception!
    //
    // https://docs.sentry.io/error-reporting/capturing/?platform=native
    //
    const QString severityStr = determineSeverity();
    const GkSeverity gkSeverityEnum = gkDb->convSeverityToEnum(severityStr);

    switch (gkSeverityEnum) {
        case GkSeverity::Debug:
            // Debug
            sentry_set_level(SENTRY_LEVEL_DEBUG);
            break;
        case GkSeverity::Verbose:
            // Verbose
            sentry_set_level(SENTRY_LEVEL_DEBUG);
            break;
        case GkSeverity::Info:
            // Info
            sentry_set_level(SENTRY_LEVEL_INFO);
            break;
        case GkSeverity::Warning:
            // Warning
            sentry_set_level(SENTRY_LEVEL_WARNING);
            break;
        case GkSeverity::Error:
            // Error
            sentry_set_level(SENTRY_LEVEL_ERROR);
            break;
        case GkSeverity::Fatal:
            // Fatal
            sentry_set_level(SENTRY_LEVEL_FATAL);
            break;
        case GkSeverity::None:
            // None
            sentry_set_level(SENTRY_LEVEL_INFO); // This is the best that we can do given the choices available!
            break;
        default:
            // Warning
            sentry_set_level(SENTRY_LEVEL_WARNING);
            break;
    }

    sentry_value_t capture_logger_exit = sentry_value_new_event();
    sentry_event_value_add_stacktrace(capture_logger_exit, nullptr, 0);
    sentry_capture_event(capture_logger_exit);

    // Clear any memory used by Sentry & Crashpad before making sure the process itself terminates...
    sentry_shutdown(); // TODO: Replace with, `sentry_close()`, upon updating Sentry in the future!
    return;
}

/**
 * @brief GkEventLoggerTableViewModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_logs
 */
void GkEventLoggerTableViewModel::populateData(const QList<GkEventLogging> &event_logs)
{
    dataBatchMutex.lock();

    m_data.clear();
    m_data = event_logs;

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkEventLoggerTableViewModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkEventLoggerTableViewModel::insertData(const GkEventLogging &event)
{
    dataBatchMutex.lock();

    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data.append(event);
    endInsertRows();

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_EVENTLOG_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkEventLoggerTableViewModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkEventLoggerTableViewModel::removeData(const GkEventLogging &event)
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
    auto bottom = this->createIndex((m_data.count() - 1), GK_EVENTLOG_TABLEVIEW_MODEL_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkEventLoggerTableViewModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkEventLoggerTableViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

/**
 * @brief GkEventLoggerTableViewModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkEventLoggerTableViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_EVENTLOG_TABLEVIEW_MODEL_TOTAL_IDX; // Make sure to add the total of columns from within `GkEventLoggerTableViewModel::headerData()`!
}

/**
 * @brief GkEventLoggerTableViewModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkEventLoggerTableViewModel::data(const QModelIndex &index, int role) const
{


    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const int row_event_no = m_data[index.row()].event_no;

    const qint64 row_date_time = m_data[index.row()].mesg.date;
    QDateTime timestamp;
    timestamp.setMSecsSinceEpoch(row_date_time);

    const GkSeverity row_severity = m_data[index.row()].mesg.severity;
    const QString row_severity_str = gkDb->convSeverityToStr(row_severity);
    const QString row_message = m_data[index.row()].mesg.message;

    sentry_value_t crumb = sentry_value_new_breadcrumb("default", "Events Logger");
    // sentry_value_set_by_key(crumb, "timestamp", sentry_value_new_int32(row_message.toStdString().c_str()));
    sentry_value_set_by_key(crumb, "message", sentry_value_new_string(row_message.toStdString().c_str()));
    sentry_value_set_by_key(crumb, "level", sentry_value_new_string(row_severity_str.toStdString().c_str()));
    sentry_add_breadcrumb(crumb);

    switch (index.column()) {
    case GK_EVENTLOG_TABLEVIEW_MODEL_EVENT_NO_IDX:
        return row_event_no;
    case GK_EVENTLOG_TABLEVIEW_MODEL_DATETIME_IDX:
        return timestamp.toString(General::Logging::dateTimeFormatting);
    case GK_EVENTLOG_TABLEVIEW_MODEL_SEVERITY_IDX:
        return row_severity_str;
    case GK_EVENTLOG_TABLEVIEW_MODEL_MESSAGE_IDX:
        return row_message;
    }

    return QVariant();
}

/**
 * @brief GkEventLoggerTableViewModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
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

/**
 * @brief GkEventLoggerTableViewModel::determineSeverity
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString GkEventLoggerTableViewModel::determineSeverity() const
{
    QPointer<GkEventLoggerTableViewModel> model = new GkEventLoggerTableViewModel(gkDb);
    QModelIndex idx = model->index(rowCount(), GK_EVENTLOG_TABLEVIEW_MODEL_SEVERITY_IDX);
    const QString gkSeverity = data(idx).toString();

    return gkSeverity;
}
