 
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

#include "src/gk_logger.hpp"
#include <utility>
#include <QDateTime>

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

std::mutex dataBatchMutex;
std::mutex setDateMutex;
std::mutex setEventNoMutex;

/**
 * @brief GkEventLogger::GkEventLogger
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param viewModel
 * @param parent
 */
GkEventLogger::GkEventLogger(QObject *parent)
{
    setParent(parent);

    return;
}

GkEventLogger::~GkEventLogger()
{
    return;
}

/**
 * @brief GkEventLogger::publishEvent allows the publishing of an event log and any of its component characteristics.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event The event message itself.
 * @param severity The severity of the issue, where it's just a warning all the way to a fatal error.
 * @param arguments Any arguments that are associated with the event message. This tends to be left blank.
 */
void GkEventLogger::publishEvent(const QString &event, const GkSeverity &severity, const QVariant &arguments, const bool &sys_notification)
{
    const std::lock_guard<std::mutex> lock(dataBatchMutex);

    GkEventLogging event_log;
    event_log.mesg.message = event;
    event_log.mesg.severity = severity;

    if (arguments.isValid()) {
        event_log.mesg.arguments = arguments;
    } else {
        event_log.mesg.arguments = "";
    }

    if (eventLogDb.empty()) {
        event_log.event_no = 1;
    } else {
        event_log.event_no = setEventNo(); // Set the event number accordingly!
    }

    event_log.mesg.date = QDateTime::currentMSecsSinceEpoch();

    eventLogDb.push_back(event_log);
    emit sendEvent(event_log);

    if (sys_notification) {
        systemNotification(tr("Small World Deluxe"), event);
    }

    return;
}

/**
 * @brief GkEventLogger::setDate will set the date as according to the milliseconds from UNIX epoch. There is no need to format
 * beforehand, as this will be done accordingly by the QTableView model.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The date since UNIX epoch, in milliseconds!
 */
qint64 GkEventLogger::setDate()
{
    const std::lock_guard<std::mutex> lock(setDateMutex);

    qint64 curr_date = QDateTime::currentMSecsSinceEpoch();
    return curr_date;
}

/**
 * @brief GkEventLogger::setEventNo creates a particular identification number for each event to uniquely identify and characterise
 * them, which not only makes data manipulation easier, but makes identifying by the user easier too!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The unique ID for each event entry.
 */
int GkEventLogger::setEventNo()
{
    const std::lock_guard<std::mutex> lock(setEventNoMutex);

    int event_number = eventLogDb.back().event_no;
    event_number += 1;

    return event_number;
}

void GkEventLogger::systemNotification(const QString &title, const QString &msg)
{
    try {
        if (!msg.isNull() && !msg.isEmpty()) {
            // Send out a system notification!
            system(QString("notify-send '%1' \"%2\"").arg(title).arg(msg).toStdString().c_str());
            return;
        }
    }  catch (const std::exception &e) {
        std::cerr << tr("Attempted to send out a system notification popup, but failed! Please check with the maintainer/distributor of the release you are using. Error:\n\n%1")
                     .arg(QString::fromStdString(e.what())).toStdString() << std::endl;
    }

    return;
}
