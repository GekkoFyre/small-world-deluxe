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

#pragma once

#include "src/defines.hpp"
#include "src/file_io.hpp"
#include "src/models/tableview/gk_logger_model.hpp"
#include <qxmpp/QXmppLogger.h>
#include <mutex>
#include <memory>
#include <QFile>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QPointer>
#include <QSystemTrayIcon>

#if defined(GFYRE_ENBL_MSVC_WINTOAST)
#include "src/contrib/WinToast/src/wintoastlib.h"
#endif

namespace GekkoFyre {

class GkEventLogger : public QObject {
    Q_OBJECT

public:
    explicit GkEventLogger(const QPointer<QSystemTrayIcon> &sysTrayIcon, const QPointer<GekkoFyre::StringFuncs> &stringFuncs,
                           QPointer<GekkoFyre::FileIo> fileIo, const quintptr &win_id, QObject *parent = nullptr);
    ~GkEventLogger() override;

public slots:
    void publishEvent(const QString &event, const GekkoFyre::System::Events::Logging::GkSeverity &severity = GekkoFyre::System::Events::Logging::GkSeverity::Warning,
                      const QVariant &arguments = "", const bool &sys_notification = false, const bool &publishToConsole = true,
                      const bool &publishToStatusBar = false, const bool &displayMsgBox = false, const bool &flashTaskbar = false);
    void recvXmppLog(QXmppLogger::MessageType msg_type, QString msg);

signals:
    void sendEvent(const GekkoFyre::System::Events::Logging::GkEventLogging &event);
    void removeEvent(const GekkoFyre::System::Events::Logging::GkEventLogging &event);
    void sendToStatusBar(const QString &msg);

private:
    QPointer<QSystemTrayIcon> m_trayIcon;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QList<GekkoFyre::System::Events::Logging::GkEventLogging> eventLogDb;                       // Where the event log itself is stored in memory...

    //
    // File I/O
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QFile gkWriteCsvIo;

    //
    // Microsoft Windows
    quintptr m_windowId;

    #if defined(GFYRE_ENBL_MSVC_WINTOAST)
    std::unique_ptr<WinToastLib::WinToastTemplate> toastTempl;
    #endif

    int setEventNo();
    void systemNotification(const QString &title, const GekkoFyre::System::Events::Logging::GkEventLogging &event_msg);
    void sendToConsole(const GekkoFyre::System::Events::Logging::GkEventLogging &event_msg,
                       const GekkoFyre::System::Events::Logging::GkSeverity &severity);
    void writeToCsv(const GekkoFyre::System::Events::Logging::GkEventLogging &event_msg,
                    const GekkoFyre::System::Events::Logging::GkSeverity &severity);

};
};
