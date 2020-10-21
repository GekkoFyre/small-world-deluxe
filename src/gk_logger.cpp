 
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

#include "src/gk_logger.hpp"
#include <utility>
#include <chrono>
#include <thread>
#include <QDateTime>

#ifdef __cplusplus
extern "C"
{
#endif

#include <portaudio.h>

#ifdef __cplusplus
} // extern "C"
#endif

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
GkEventLogger::GkEventLogger(QPointer<GekkoFyre::StringFuncs> stringFuncs, QObject *parent)
{
    setParent(parent);
    gkStringFuncs = std::move(stringFuncs);

    return;
}

GkEventLogger::~GkEventLogger()
{
    // Clear any memory used by Sentry & Crashpad before making sure the process itself terminates!
    sentry_shutdown();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    return;
}

/**
 * @brief GkEventLogger::handlePortAudioErrorCode An event handler for PortAudio error codes.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pa_error The error code/return from a PortAudio function.
 * @return Whether there was actually an error or not.
 */
bool GkEventLogger::handlePortAudioErrorCode(const PaError &pa_error, const QString &arguments)
{
    if (pa_error != paNoError) {
        QString log_msg = tr("There has been an error with regard to PortAudio!");
        if (!arguments.isEmpty()) {
            log_msg += QString(" ");
            log_msg += tr("Error:\n\n%1 [ %2 ]").arg(arguments).arg(QString::fromStdString(Pa_GetErrorText(pa_error)));
        }

        publishEvent(log_msg, GkSeverity::Error, "", false, true, false, true);
        return false;
    }

    return true;
}

/**
 * @brief GkEventLogger::publishEvent allows the publishing of an event log and any of its component characteristics.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event The event message itself.
 * @param severity The severity of the issue, where it's just a warning all the way to a fatal error.
 * @param arguments Any arguments that are associated with the event message. This tends to be left blank.
 */
void GkEventLogger::publishEvent(const QString &event, const GkSeverity &severity, const QVariant &arguments, const bool &sys_notification,
                                 const bool &publishToConsole, const bool &publishToStatusBar, const bool &displayMsgBox)
{
    std::lock_guard<std::mutex> lock(dataBatchMutex);

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
        systemNotification(tr("Small World Deluxe"), event_log);
    }

    if (publishToConsole) {
        sendToConsole(event_log, event_log.mesg.severity);
    }

    if (publishToStatusBar) {
        QDateTime timestamp;
        timestamp.setMSecsSinceEpoch(event_log.mesg.date);

        emit sendToStatusBar(QString("(%1) %2").arg(timestamp.toString(tr("hh:mm:ss"))).arg(event_log.mesg.message));
    }

    if (displayMsgBox) {
        if (event_log.mesg.severity == GkSeverity::Warning || event_log.mesg.severity == GkSeverity::Fatal || event_log.mesg.severity == GkSeverity::Error) {
            QMessageBox::warning(nullptr, tr("Error!"), event_log.mesg.message, QMessageBox::Ok);
        } else {
            QMessageBox::information(nullptr, tr("Status"), event_log.mesg.message, QMessageBox::Ok);
        }
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
    std::lock_guard<std::mutex> lock(setDateMutex);

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
    std::lock_guard<std::mutex> lock(setEventNoMutex);

    int event_number = eventLogDb.back().event_no;
    event_number += 1;

    return event_number;
}

/**
 * @brief GkEventLogger::systemNotification
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param title
 * @param msg
 */
void GkEventLogger::systemNotification(const QString &title, const GkEventLogging &event_msg)
{
    try {
        if (!event_msg.mesg.message.isNull() && !event_msg.mesg.message.isEmpty()) {
            QString msg = event_msg.mesg.message;
            if (!event_msg.mesg.arguments.isNull() && !event_msg.mesg.arguments.toString().isEmpty()) {
                msg.clear();
                msg = gkStringFuncs->addErrorMsg(event_msg.mesg.message, event_msg.mesg.arguments.toString());
            }

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

/**
 * @brief GkEventLogger::sendToConsole if the correct flag for this function is enabled, then the event log message will be published
 * via CLI interface as well in order to provide extra information to the user via as many means as possible, so that it is ensured
 * the event's message is as widely seen/noticed as possible.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msg The event log message in question that is to be published.
 * @param severity The severity of the event log message in question, that will thus determine the std::out() interface to
 * be used (i.e. `std::cout` vs. `std::cerr`).
 */
void GkEventLogger::sendToConsole(const GkEventLogging &event_msg, const Events::Logging::GkSeverity &severity)
{
    if (!event_msg.mesg.message.isNull() && !event_msg.mesg.message.isEmpty()) {
        QString msg = event_msg.mesg.message;
        if (!event_msg.mesg.arguments.isNull() && !event_msg.mesg.arguments.toString().isEmpty()) {
            msg.clear();
            msg = gkStringFuncs->addErrorMsg(event_msg.mesg.message, event_msg.mesg.arguments.toString());
        }

        if (severity == GkSeverity::None || severity == GkSeverity::Debug || severity == GkSeverity::Verbose ||
            severity == GkSeverity::Info) {
            std::cout << msg.toStdString() << std::endl;
        } else {
            std::cerr << msg.toStdString() << std::endl;
        }
    }

    return;
}
