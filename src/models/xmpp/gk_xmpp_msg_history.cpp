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
 **   Small World is distributed in the hope that it will be useful,
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

#include "src/models/xmpp/gk_xmpp_msg_history.hpp"
#include <utility>
#include <iostream>
#include <exception>
#include <QDir>
#include <QFile>
#include <QDomElement>
#include <QTextStream>
#include <QStandardPaths>

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
 * @brief GkXmppMsgHistory::GkXmppMsgHistory
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param err_msg
 * @param err
 */
GkXmppMsgHistory::GkXmppMsgHistory(std::shared_ptr<QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign>> rosterList,
                                   QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                   QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QObject(parent)
{
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);
    if (rosterList) {
        m_rosterList = std::move(rosterList);
    }

    return;
}

GkXmppMsgHistory::~GkXmppMsgHistory() {}

/**
 * @brief GkXmppMsgHistory::recordMsgHistory will record the message history to a file on the end-user's computer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @param mam_msg
 */
void GkXmppMsgHistory::recordMsgHistory(const QString &bareJid, const QList<Network::GkXmpp::GkXmppMamMsg> &mam_msg)
{
    if (!bareJid.isEmpty()) {
        //
        // Create the message history file on the end-users computer regardless of whether the given QList contains
        // information or not! The end-user might see something as amiss otherwise.
        const QString dirToAppend = QDir::toNativeSeparators(QString::fromStdString(General::companyName) + "/" + QString::fromStdString(Filesystem::defaultDirAppend) + "/" + QString::fromStdString(Filesystem::xmppVCardDir));
        currPath.setPath(QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" + dirToAppend));
        if (!currPath.exists()) {
            if (currPath.mkpath(currPath.path())) {
                gkEventLogger->publishEvent(tr("Directory, \"%1\", has been created successfully!")
                .arg(currPath.canonicalPath()), GkSeverity::Info, "", false,
                true, false, false);
            }
        }

        QFile txtFile(QString(currPath.canonicalPath() + "/" + gkStringFuncs->getXmppUsername(bareJid) + Filesystem::xmlExtension));
        QTextStream textStream;
        if (!mam_msg.isEmpty()) {}
    }

    return;
}
