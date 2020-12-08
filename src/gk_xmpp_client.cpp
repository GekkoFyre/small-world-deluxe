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
#include <qxmpp/QXmppDataForm.h>
#include <qxmpp/QXmppLogger.h>
#include <qxmpp/QXmppMucIq.h>
#include <iostream>
#include <exception>
#include <QList>
#include <QEventLoop>
#include <QMessageBox>
#include <QStringList>
#include <QDnsServiceRecord>

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

GkXmppClient::GkXmppClient(const GkUserConn &connection_details, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                           QObject *parent) : m_rosterManager(findExtension<QXmppRosterManager>()), QXmppClient(parent)
{
    try {
        setParent(parent);
        gkConnDetails = connection_details;
        gkEventLogger = std::move(eventLogger);

        QObject::connect(this, SIGNAL(connected()), this, SLOT(clientConnected()));
        QObject::connect(m_rosterManager.get(), SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));

        // Then QXmppRoster::presenceChanged() is emitted whenever presence of
        // someone in roster changes...
        QObject::connect(m_rosterManager.get(), SIGNAL(presenceChanged(const QString &, const QString &)),
                         this, SLOT(presenceChanged(const QString &, const QString &)));

        m_dns = new QDnsLookup(this);
        m_presence = std::make_unique<QXmppPresence>();
        m_mucManager = std::make_unique<QXmppMucManager>();

        //
        // Setup the signals for the DNS object
        QObject::connect(m_dns, SIGNAL(finished()), this, SLOT(handleServers()));

        //
        // This signal is emitted when the client state changes...
        QObject::connect(this, SIGNAL(stateChanged (QXmppClient::State)), this, SLOT());

        //
        // This signal is emitted when the XMPP connection encounters any error...
        QObject::connect(this, SIGNAL(error(QXmppClient::Error)),
                         this, SLOT(handleError(QXmppClient::Error)));

        //
        // Find the XMPP servers as defined by either the user themselves or GekkoFyre Networks...
        m_dns->setType(QDnsLookup::SRV);
        QString dns_lookup_str;
        switch (gkConnDetails.server.type) {
            case GkServerType::Google:
                dns_lookup_str = "_xmpp-client._tcp.gmail.com";
                break;
            case GkServerType::Unknown:
                throw std::invalid_argument(tr("Unable to perform DNS lookup for XMPP; has a server been specified?").toStdString());
            default:
                dns_lookup_str = gkConnDetails.server.domain.toString();
                break;
        }

        m_dns->setName(dns_lookup_str);
        m_dns->lookup();

        //
        // Setup logging...
        QXmppLogger *logger = QXmppLogger::getLogger();
        logger->setLoggingType(QXmppLogger::SignalLogging);
        QObject::connect(logger, SIGNAL(message(QXmppLogger::MessageType, QString)),
                         gkEventLogger, SLOT(recvXmppLog(QXmppLogger::MessageType, QString)));

        QEventLoop loop;
        QObject::connect(this, SIGNAL(connected()), &loop, SLOT(quit()));
        QObject::connect(this, SIGNAL(disconnected()), &loop, SLOT(quit()));

        QXmppConfiguration config;
        if (!gkConnDetails.jid.isEmpty() || !gkConnDetails.password.isEmpty()) {
            config.setUser(gkConnDetails.jid);
            config.setPassword(gkConnDetails.password);
        } else {
            m_presence = nullptr;
        }

        // You only need to provide a domain to connectToServer()...
        config.setAutoAcceptSubscriptions(false);
        config.setAutoReconnectionEnabled(false);
        config.setIgnoreSslErrors(false);
        config.setStreamSecurityMode(QXmppConfiguration::StreamSecurityMode::TLSDisabled);
        if (gkConnDetails.server.settings_client.enable_ssl) {
            config.setStreamSecurityMode(QXmppConfiguration::StreamSecurityMode::TLSRequired);
        }

        config.setDomain(gkConnDetails.server.domain.toString());
        config.setPort(gkConnDetails.server.port);
        config.setUseNonSASLAuthentication(false);
        config.setUseSASLAuthentication(false);

        if (gkConnDetails.server.settings_client.auto_connect) {
            if (m_presence) {
                connectToServer(config, *m_presence);
            } else {
                connectToServer(config);
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An issue has occurred within the XMPP subsystem. Error:\n\n%1")
        .arg(QString::fromStdString(e.what())), GkSeverity::Fatal, "", false,
        true, false, true);
    }

    return;
}

GkXmppClient::~GkXmppClient()
{
    deleteClientConnection();
    QObject::disconnect(this, nullptr, this, nullptr);
    deleteLater();
}

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
        if (!room_name.isEmpty() && !m_dns.isNull()) {
            QString room_jid = QString("%1@conference.%2").arg(room_name).arg(gkConnDetails.server.domain.toString());
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
        gkEventLogger->publishEvent(tr("An issue was encountered while creating a MUC! Error:\n\n%1").arg(QString::fromStdString(e.what())),
                                    GkSeverity::Fatal, "", false, true, false, true);
    }

    return false;
}

/**
 * @brief GkXmppClient::clientConnected
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::clientConnected()
{
    gkEventLogger->publishEvent(tr("A connection has been successfully made towards XMPP server: %1").arg(gkConnDetails.server.domain.toString()),
                                GkSeverity::Info, "", true, true, true, false);
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
    .arg(bareJid).arg(resource));

    return;
}

/**
 * @brief GkXmppClient::stateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param state
 * @note QXmppClient Class Reference <https://doc.qxmpp.org/qxmpp-dev/classQXmppClient.html#a8bd2617265568c9769a8ba608a4ff05d>.
 */
void GkXmppClient::stateChanged(QXmppClient::State state)
{
    switch (state) {
        case QXmppClient::State::DisconnectedState:
            gkEventLogger->publishEvent(tr("Disconnected from XMPP server: %1").arg(gkConnDetails.server.domain.toString()), GkSeverity::Info, "",
                                        true, true, true, false);
            return;
        case QXmppClient::State::ConnectingState:
            gkEventLogger->publishEvent(tr("...attempting to make connection towards XMPP server: %1").arg(gkConnDetails.server.domain.toString()), GkSeverity::Info, "",
                                        true, true, true, false);
            return;
        case QXmppClient::State::ConnectedState:
            gkEventLogger->publishEvent(tr("Connected to XMPP server: %1").arg(gkConnDetails.server.domain.toString()), GkSeverity::Info, "",
                                        true, true, true, false);
            return;
        default:
            break;
    }

    return;
}

/**
 * @brief GkXmppClient::createClientConnection creates a connection to the given XMPP server via the client
 * object, QXmppClient().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param config The connection parameters to use when creating a connection to the given XMPP server.
 */
void GkXmppClient::createClientConnection(const QXmppConfiguration &config)
{
    if (config.domain().isEmpty()) {
        gkEventLogger->publishEvent(tr("Unable to make XMPP connection, the given host parameter is invalid!"), GkSeverity::Fatal, "",
                                    true, true, false, false);
        return;
    }

    if (!isConnected()) {
        connectToServer(config);
    }

    return;
}

/**
 * @brief GkXmppClient::deleteClientConnection deletes a connection from the given XMPP server via the client
 * object, QXmppClient().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::deleteClientConnection()
{
    if (isConnected()) {
        disconnectFromServer();
    }

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

/**
 * @brief GkXmppClient::handleServers
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::handleServers()
{
    // Check that the lookup has succeeded
    if (m_dns->error() != QDnsLookup::NoError) {
        gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\".").arg(gkConnDetails.server.domain.toString()), GkSeverity::Error, "",
                                    true, true, false, false);
        m_dns->deleteLater();

        return;
    }

    // Handle the results of the DNS lookup
    const auto records = m_dns->serviceRecords();
    for (const QDnsServiceRecord &record: records) {}

    m_dns->deleteLater();
    return;
}

/**
 * @brief GkXmppClient::handleError
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param errorMsg
 */
void GkXmppClient::handleError(QXmppClient::Error errorMsg)
{
    QObject::disconnect(this, nullptr, this, nullptr);
    switch (errorMsg) {
        case QXmppClient::Error::NoError:
            break;
        case QXmppClient::Error::SocketError:
            gkEventLogger->publishEvent(tr("XMPP error encountered due to TCP socket."), GkSeverity::Fatal, "",
                                        false, true, false, true);
            break;
        case QXmppClient::Error::KeepAliveError:
            gkEventLogger->publishEvent(tr("XMPP error encountered due to no response from a keep alive."), GkSeverity::Fatal, "",
                                        false, true, false, true);
            break;
        case QXmppClient::Error::XmppStreamError:
            gkEventLogger->publishEvent(tr("XMPP error encountered due to XML stream."), GkSeverity::Fatal, "",
                                        false, true, false, true);
            break;
        default:
            std::cerr << tr("An unknown XMPP error has been encountered!").toStdString() << std::endl;
            break;
    }

    deleteLater();
    return;
}