 
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

#include "src/gk_logger.hpp"
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <utility>
#include <chrono>
#include <thread>
#include <QDateTime>
#include <QStandardPaths>

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

#if defined(GFYRE_ENBL_MSVC_WINTOAST)
using namespace WinToastLib;
#endif

std::mutex dataBatchMutex;
std::mutex setDateMutex;
std::mutex setEventNoMutex;

namespace fs = boost::filesystem;
namespace sys = boost::system;

/**
 * @brief GkEventLogger::GkEventLogger
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param viewModel
 * @param parent
 */
GkEventLogger::GkEventLogger(const QPointer<GekkoFyre::StringFuncs> &stringFuncs, QPointer<GekkoFyre::FileIo> fileIo, QObject *parent)
{
    try {
        setParent(parent);
        gkStringFuncs = std::move(stringFuncs);

        //
        // File I/O
        gkFileIo = std::move(fileIo);
        sys::error_code ec;
        fs::path slash = "/";
        fs::path native_slash = slash.make_preferred().native();

        const fs::path dir_to_append = fs::path(Filesystem::defaultDirAppend + native_slash.string() + Filesystem::fileLogData);
        const fs::path log_data_loc = gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
                                                                 true, QString::fromStdString(dir_to_append.string())).toStdString();
        if (fs::exists(log_data_loc)) { // Entity already exists!
            if (fs::is_directory(log_data_loc)) {
                // Entity is a directory, so therefore we must delete it before continuing!
                fs::remove_all(log_data_loc, ec);
                if (ec.failed()) {
                    throw ec.message();
                }
            }
        }

        gkWriteCsvIo.setFileName(QString::fromStdString(log_data_loc.string()));
        if (!gkWriteCsvIo.open(QFile::Append | QFile::Text)) {
            throw std::runtime_error(tr("Error with opening File I/O for logging data!").toStdString());
        }

        #if defined(GFYRE_ENBL_MSVC_WINTOAST)
        WinToast::instance()->setAppName(QString(General::productName).toStdWString());
        const auto aumi = WinToast::configureAUMI(QString(General::companyName).toStdWString(), QString(General::productName).toStdWString(),
                                                  QString(General::executableName).toStdWString(), QString(General::appVersion).toStdWString());
        WinToast::instance()->setAppUserModelId(aumi);
        if (!WinToast::instance()->initialize()) {
            publishEvent(tr("Could not initialize toast notification library!"), GkSeverity::Error, "", false, true, false, true);
        }

        toastTempl = std::make_unique<WinToastLib::WinToastTemplate>(WinToastTemplate::ImageAndText02);
        toastTempl->setImagePath(QString(":/resources/contrib/images/vector/gekkofyre-networks/rionquosue/logo_blank_border_text_square_rionquosue.svg").toStdWString());
        toastTempl->setTextField(QString(General::productName).toStdWString(), WinToastTemplate::FirstLine);
        #endif
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

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
 * @brief GkEventLogger::publishEvent allows the publishing of an event log and any of its component characteristics.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event The event message itself.
 * @param severity The severity of the issue, where it's just a warning all the way to a fatal error.
 * @param arguments Any arguments that are associated with the event message. This tends to be left blank.
 */
void GkEventLogger::publishEvent(const QString &event, const GkSeverity &severity, const QVariant &arguments, const bool &sys_notification,
                                 const bool &publishToConsole, const bool &publishToStatusBar, const bool &displayMsgBox)
{
    //
    // TODO: Introduce proper mutex'ing for multithreading!
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

    // TODO: Write out in a neater format! Such as XML, CSV, etc.
    gkWriteCsvIo.write(event_log.mesg.message.toStdString().c_str(), event_log.mesg.message.length());
    return;
}

/**
 * @brief GkEventLogger::recvXmppLog
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msg_type
 * @param msg
 */
void GkEventLogger::recvXmppLog(QXmppLogger::MessageType msg_type, QString msg)
{
    GkSeverity severity;
    switch (msg_type) {
        case QXmppLogger::MessageType::NoMessage:
            severity = GkSeverity::None;
            break;
        case QXmppLogger::MessageType::DebugMessage:
            severity = GkSeverity::Debug;
            break;
        case QXmppLogger::MessageType::InformationMessage:
            severity = GkSeverity::Info;
            break;
        case QXmppLogger::MessageType::WarningMessage:
            severity = GkSeverity::Warning;
            break;
        case QXmppLogger::MessageType::ReceivedMessage:
            severity = GkSeverity::Info;
            break;
        case QXmppLogger::MessageType::SentMessage:
            severity = GkSeverity::Info;
            break;
        case QXmppLogger::MessageType::AnyMessage:
            severity = GkSeverity::Info;
            break;
        default:
            severity = GkSeverity::Verbose;
            break;
    }

    publishEvent(msg, severity, "", true, false, true, false);
    return;
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
            if (!event_msg.mesg.arguments.isNull() && !event_msg.mesg.arguments.toString().isEmpty()) {
                QString msg = event_msg.mesg.message;
                msg.clear();
                msg = gkStringFuncs->addErrorMsg(event_msg.mesg.message, event_msg.mesg.arguments.toString());
            }

            // Send out a system notification!
            #if defined(GFYRE_ENBL_MSVC_WINTOAST)
            toastTempl->setTextField(event_msg.mesg.message.toStdWString(), WinToastTemplate::SecondLine);
            #elif __linux__
            system(QString("notify-send '%1' \"%2\"").arg(title).arg(event_msg.mesg.message).toStdString().c_str());
            #endif
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

/**
 * @brief GkEventLogger::writeToCsv
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event_msg
 * @param severity
 */
void GkEventLogger::writeToCsv(const Events::Logging::GkEventLogging &event_msg, const Events::Logging::GkSeverity &severity)
{
    if (!event_msg.mesg.message.isNull() && !event_msg.mesg.message.isEmpty()) {
        QString msg = event_msg.mesg.message;
        if (!event_msg.mesg.arguments.isNull() && !event_msg.mesg.arguments.toString().isEmpty()) {

        }
    }

    return;
}
