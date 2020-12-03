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
#include <qxmpp/QXmppDataForm.h>
#include <qxmpp/QXmppLogger.h>
#include <exception>
#include <QList>
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
    gkConnDetails = connection_details;
    gkEventLogger = std::move(eventLogger);

    QObject::connect(this, SIGNAL(connected()), this, SLOT(clientConnected()));
    QObject::connect(m_rosterManager.get(), SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));

    // Then QXmppRoster::presenceChanged() is emitted whenever presence of
    // someone in roster changes...
    QObject::connect(m_rosterManager.get(), SIGNAL(presenceChanged(const QString &, const QString &)),
                     this, SLOT(presenceChanged(const QString &, const QString &)));

    client = new QXmppClient(parent);
    m_presence = std::make_unique<QXmppPresence>();
    m_mucManager = std::make_unique<QXmppMucManager>();

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
    config.setDomain(gkConnDetails.server.domain);
    config.setHost(gkConnDetails.server.host.toString());
    config.setPort(gkConnDetails.server.port);
    config.setUser(gkConnDetails.jid);
    config.setPassword(gkConnDetails.password);
    config.setSaslAuthMechanism(""); // TODO: Configure this value properly!

    if (!gkConnDetails.server.joined) {
        client->connectToServer(config, *m_presence);
    }
}

GkXmppClient::~GkXmppClient()
{}

/**
 * @brief GkXmppClient::createMuc
 * @author musimbate <https://stackoverflow.com/questions/29056790/qxmpp-creating-a-muc-room-xep-0045-on-the-server>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param room_name
 * @param room_subject
 * @param room_desc
 * @return
 */
bool GkXmppClient::createMuc(const QString &room_name, const QString &room_subject, const QString &room_desc)
{
    try {
        if (!room_name.isEmpty() && !gkConnDetails.server.domain.isEmpty()) {
            QString room_jid = QString("%1@conference.%2").arg(room_name).arg(gkConnDetails.server.domain);
            QList<QXmppMucRoom *> rooms = m_mucManager->rooms();
            QXmppMucRoom *r;
            foreach(r, rooms) {
                if (r->jid() == room_jid) {
                    gkEventLogger->publishEvent(tr("%1 has joined the MUC, \"%2\".")
                    .arg(tr("<unknown>")).arg(room_jid), GkSeverity::Info, "",
                    true, true, false, false);
                }
            }

            m_pRoom.reset(m_mucManager->addRoom(room_jid));
            if (m_pRoom) {
                // Nickname...
                m_pRoom->setNickName(gkConnDetails.nickname);
                // Join the room itself...
                m_pRoom->join();
            }

            QXmppDataForm form(QXmppDataForm::Submit);
            QList<QXmppDataForm::Field> fields;

            {
                QXmppDataForm::Field field(QXmppDataForm::Field::HiddenField);
                field.setKey("FORM_TYPE");
                field.setValue("http://jabber.org/protocol/muc#roomconfig");
                fields.append(field);
            }

            QXmppDataForm::Field field;
            field.setKey("muc#roomconfig_roomname");
            field.setValue(room_name);
            fields.append(field);

            field.setKey("muc#roomconfig_subject");
            field.setValue(room_subject);
            fields.append(field);

            field.setKey("muc#roomconfig_roomdesc");
            field.setValue(room_desc);
            fields.append(field);

            {
                QXmppDataForm::Field field(QXmppDataForm::Field::BooleanField);
                field.setKey("muc#roomconfig_persistentroom");
                field.setValue(true);
                fields.append(field);
            }

            form.setFields(fields);
            m_pRoom->setConfiguration(form);

            return true;
        }
    } catch (const std::exception &e) {
        //
    }

    return false;
}

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
