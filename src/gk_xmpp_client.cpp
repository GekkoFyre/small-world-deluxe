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

#include "src/gk_xmpp_client.hpp"
#include <boost/exception/all.hpp>
#include <qxmpp/QXmppLogger.h>
#include <exception>
#include <QEventLoop>
#include <QMessageBox>
#include <QStringList>

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

namespace fs = boost::filesystem;
namespace sys = boost::system;

#define OGG_VORBIS_READ (1024)

GkXmppClient::GkXmppClient(const GkConnection &connection_details, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                           QObject *parent) : QXmppClient(parent), m_rosterManager(findExtension<QXmppRosterManager>())
{
    setParent(parent);
    gkEventLogger = std::move(eventLogger);

    QObject::connect(this, SIGNAL(connected()), this, SLOT(clientConnected()));
    QObject::connect(m_rosterManager.get(), SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));

    // Then QXmppRoster::presenceChanged() is emitted whenever presence of
    // someone in roster changes...
    QObject::connect(m_rosterManager.get(), SIGNAL(presenceChanged(const QString &, const QString &)),
                     this, SLOT(presenceChanged(const QString &, const QString &)));

    client = new QXmppClient(parent);
    m_presence = std::make_unique<QXmppPresence>();

    //
    // Setup logging...
    QXmppLogger *logger = QXmppLogger::getLogger();
    logger->setLoggingType(QXmppLogger::SignalLogging);
    QObject::connect(logger, SIGNAL(message(QXmppLogger::MessageType, QString)),
                     gkEventLogger, SLOT(recvXmppLog(QXmppLogger::MessageType, QString)));

    QEventLoop loop;
    QObject::connect(client, SIGNAL(connected()), &loop, SLOT(quit()));
    QObject::connect(client, SIGNAL(disconnected()), &loop, SLOT(quit()));

    QXmppConfiguration config;
    config.setDomain(connection_details.server.domain);
    config.setHost(connection_details.server.host.toString());
    config.setPort(connection_details.server.port);
    config.setUser(connection_details.jid);
    config.setPassword(connection_details.password);
    config.setSaslAuthMechanism(""); // TODO: Configure this value properly!

    if (!connection_details.server.joined) {
        client->connectToServer(config, *m_presence);
    }
}

GkXmppClient::~GkXmppClient()
{}

/**
 * @brief GkXmppClient::clientConnected
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::clientConnected()
{
    return;
}

/**
 * @brief GkXmppClient::rosterReceived
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::rosterReceived()
{
    const QStringList jids = m_rosterManager->getRosterBareJids();
    for (const QString &bareJid: jids) {
        QString name = m_rosterManager->getRosterEntry(bareJid).name();
        if (name.isEmpty()) {
            name = "-";
        }

        qDebug("Roster received: %s [%s]", qPrintable(bareJid), qPrintable(name));
    }

    return;
}

/**
 * @brief GkXmppClient::presenceChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @param resource
 */
void GkXmppClient::presenceChanged(const QString &bareJid, const QString &resource)
{
    gkEventLogger->publishEvent(tr("Presence changed for %1 towards %2.")
    .arg(qPrintable(bareJid)).arg(qPrintable(resource)));

    return;
}

/**
 * @brief GkXmppClient::modifyPresence
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pres
 */
void GkXmppClient::modifyPresence(const QXmppPresence::Type &pres)
{
    m_presence->setType(pres);
    gkEventLogger->publishEvent(tr("User has changed their XMPP status towards %1."), GkSeverity::Info, "",
                                true, true, false, false); // TODO: Make this complete!

    return;
}
