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

#include "src/gk_xmpp_client.hpp"
#include <qxmpp/QXmppMucIq.h>
#include <qxmpp/QXmppUtils.h>
#include <qxmpp/QXmppStreamFeatures.h>
#include <iostream>
#include <exception>
#include <QFile>
#include <QImage>
#include <QBuffer>
#include <QMessageBox>
#include <QFileDialog>
#include <QImageReader>
#include <QStandardPaths>
#include <QXmlStreamWriter>

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

namespace fs = boost::filesystem;
namespace sys = boost::system;

/**
 * @brief GkXmppClient::GkXmppClient The client-class for all such XMPP calls within Small World Deluxe.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param connection_details The details pertaining to making a successful connection towards the given XMPP server.
 * @param eventLogger The object for processing logging information.
 * @param connectNow Whether to initiate a connection now or at a later time instead.
 * @param parent The parent object.
 */
GkXmppClient::GkXmppClient(const GkUserConn &connection_details, QPointer<GekkoFyre::GkLevelDb> database,
                           QPointer<GekkoFyre::FileIo> fileIo, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                           const bool &connectNow, QObject *parent) : m_rosterManager(findExtension<QXmppRosterManager>()),
                                                                      m_registerManager(findExtension<QXmppRegistrationManager>()),
                                                                      m_versionMgr(findExtension<QXmppVersionManager>()),
                                                                      m_vCardManager(findExtension<QXmppVCardManager>()),
                                                                      m_xmppLogger(QXmppLogger::getLogger()),
                                                                      m_discoMgr(findExtension<QXmppDiscoveryManager>()),
                                                                      QXmppClient(parent)
{
    try {
        setParent(parent);
        m_connDetails = connection_details;
        gkDb = std::move(database);
        gkFileIo = std::move(fileIo);
        gkEventLogger = std::move(eventLogger);
        m_sslSocket = new QSslSocket(this);

        m_registerManager = std::make_shared<QXmppRegistrationManager>();
        m_mucManager = std::make_unique<QXmppMucManager>();
        m_transferManager = std::make_unique<QXmppTransferManager>();

        addExtension(m_rosterManager.get());
        addExtension(m_versionMgr.get());
        addExtension(m_discoMgr.get());

        if (m_registerManager) {
            addExtension(m_registerManager.get());
        }

        if (m_vCardManager) {
            addExtension(m_vCardManager.get());
        }

        if (m_mucManager) {
            addExtension(m_mucManager.get());
        }

        if (m_transferManager) {
            addExtension(m_transferManager.get());
        }

        sys::error_code ec;
        fs::path slash = "/";
        native_slash = slash.make_preferred().native();

        const fs::path dir_to_append = fs::path(Filesystem::defaultDirAppend + native_slash.string() + Filesystem::xmppVCardDir);
        vcard_save_path = gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
                                                                    true, QString::fromStdString(dir_to_append.string())).toStdString(); // Path to save final database towards
        if (!fs::exists(vcard_save_path)) {
            if (fs::create_directory(vcard_save_path, ec)) {
                gkEventLogger->publishEvent(tr("Directory, \"%1\", has been created successfully!")
                                                    .arg(QString::fromStdString(vcard_save_path.string())), GkSeverity::Info, "",
                                            false, true, false, false);
            } else {
                throw std::runtime_error(tr("Unsuccessfully attempted to capture vCards for XMPP user roster. Error: %1").arg(QString::fromStdString(ec.message())).toStdString());
            }
        }

        if (!fs::is_directory(vcard_save_path)) {
            fs::remove(vcard_save_path, ec);
            if (ec.failed()) {
                throw std::runtime_error(tr("Unsuccessfully attempted to capture vCards for XMPP user roster. Error: %1").arg(QString::fromStdString(ec.message())).toStdString());
            }

            if (fs::create_directory(vcard_save_path)) {
                gkEventLogger->publishEvent(tr("Directory, \"%1\", has been created successfully!")
                                                    .arg(QString::fromStdString(vcard_save_path.string())), GkSeverity::Info, "",
                                            false, true, false, false);
            }
        }

        //
        // Setup error handling for QXmppClient...
        QObject::connect(this, SIGNAL(sendError(const QString &)), this, SLOT(handleError(const QString &)));

        //
        // This signal is emitted to indicate that one or more SSL errors were
        // encountered while establishing the identity of the server...
        QObject::connect(this, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(handleSslErrors(const QList<QSslError> &)));

        //
        // This signal is emitted when the XMPP connection encounters any error...
        QObject::connect(this, SIGNAL(error(QXmppClient::Error)), this, SLOT(handleError(QXmppClient::Error)));

        QObject::connect(m_sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), m_sslSocket, SLOT(ignoreSslErrors()));

        QObject::connect(m_sslSocket, SIGNAL(connected()), this, SLOT(handleSslGreeting()));

        QObject::connect(m_sslSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError(QAbstractSocket::SocketError)));

        QObject::connect(m_vCardManager.get(), SIGNAL(vCardReceived(const QXmppVCardIq &)), this, SLOT(vCardReceived(const QXmppVCardIq &)));
        QObject::connect(m_vCardManager.get(), SIGNAL(clientVCardReceived()), this, SLOT(clientVCardReceived()));
        QObject::connect(this, SIGNAL(sendClientVCard(const QXmppVCardIq &)), this, SLOT(updateClientVCard(const QXmppVCardIq &)));

        m_status = GkOnlineStatus::Online;
        m_keepalive = 60;

        m_presence = std::make_shared<QXmppPresence>();

        if (m_versionMgr) {
            m_versionMgr->setClientName(General::companyNameMin);
            m_versionMgr->setClientVersion(General::appVersion);
            QObject::connect(m_versionMgr.get(), SIGNAL(versionReceived(const QXmppVersionIq &)),
                             this, SLOT(versionReceivedSlot(const QXmppVersionIq &)));
        }

        //
        // Notifies upon when a successful connection is made, thus enabling the activation of other, highly essential code!
        QObject::connect(this, SIGNAL(connected()), this, SLOT(clientConnected()));

        //
        // Setup logging...
        m_xmppLogger->setLoggingType(QXmppLogger::SignalLogging);
        QObject::connect(m_xmppLogger.get(), SIGNAL(message(QXmppLogger::MessageType, const QString &)),
                         gkEventLogger, SLOT(recvXmppLog(QXmppLogger::MessageType, const QString &)));

        //
        // Initialize all the SIGNALS and SLOTS for QXmppRosterManager...
        QObject::connect(m_rosterManager.get(), SIGNAL(presenceChanged(const QString &, const QString &)), this, SLOT(presenceChanged(const QString &, const QString &)));
        QObject::connect(m_rosterManager.get(), SIGNAL(rosterReceived()), this, SLOT(handleRosterReceived()));
        QObject::connect(m_rosterManager.get(), SIGNAL(subscriptionReceived(const QString &)), this, SLOT(notifyNewSubscription(const QString &)));
        QObject::connect(m_rosterManager.get(), SIGNAL(itemAdded(const QString &)), this, SLOT(itemAdded(const QString &)));
        QObject::connect(m_rosterManager.get(), SIGNAL(itemRemoved(const QString &)), this, SLOT(itemRemoved(const QString &)));
        QObject::connect(m_rosterManager.get(), SIGNAL(itemChanged(const QString &)), this, SLOT(itemChanged(const QString &)));

        //
        // Find the XMPP servers as defined by either the user themselves or GekkoFyre Networks...
        QString dns_lookup_str;
        switch (m_connDetails.server.type) {
            case GkServerType::GekkoFyre:
                //
                // Settings for GekkoFyre Networks' server have been specified!
                if (m_connDetails.server.settings_client.auto_connect || connectNow) {
                    gkEventLogger->publishEvent(tr("Attempting connection towards GekkoFyre Networks!"), GkSeverity::Info,
                                                "", false, true, false, false);
                    createConnectionToServer(GkXmppGekkoFyreCfg::defaultUrl, GK_DEFAULT_XMPP_SERVER_PORT, getUsername(m_connDetails.jid),
                                             m_connDetails.password, m_connDetails.jid, false);
                }

                break;
            case GkServerType::Custom:
                //
                // Settings for a custom server have been specified!
                if (m_connDetails.server.settings_client.uri_lookup_method == GkUriLookupMethod::QtDnsSrv) {
                    m_dns = new QDnsLookup(this);

                    //
                    // Setup the signals for the DNS object
                    QObject::connect(m_dns, SIGNAL(finished()), this, SLOT(handleServers()));

                    m_dns->setType(QDnsLookup::SRV);
                    dns_lookup_str = QString("_xmpp-client._tcp.%1").arg(m_connDetails.server.url);
                    m_dns->setName(dns_lookup_str);
                    m_dns->lookup();

                    m_dnsKeepAlive = std::make_unique<QElapsedTimer>();
                    m_dnsKeepAlive->start();
                    while (!m_dns->isFinished()) {
                        continue;
                    }
                }

                if (m_connDetails.server.settings_client.auto_connect || connectNow) {
                    createConnectionToServer(m_connDetails.server.url, m_connDetails.server.port, getUsername(m_connDetails.jid),
                                             m_connDetails.password, m_connDetails.jid, false);
                }

                break;
            case GkServerType::Unknown:
                throw std::invalid_argument(tr("Unable to perform DNS lookup for XMPP; has a server been specified?").toStdString());
            default:
                throw std::invalid_argument(tr("Unable to perform DNS lookup for XMPP; has a server been specified?").toStdString());
        }

        //
        // As soon as you connect towards a XMPP server with no JID, this connection should come alive provided that
        // the most minimal of in-band user registration is supported by said server!
        //
        QObject::connect(m_registerManager.get(), SIGNAL(registrationFormReceived(const QXmppRegisterIq &)),
                         this, SLOT(handleRegistrationForm(const QXmppRegisterIq &)));

        QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::registrationSucceeded, this, [=]() {
            gkEventLogger->publishEvent(tr("User, \"%1\", has been successfully registered with XMPP server: %2").arg(configuration().user())
                                                .arg(configuration().domain()), GkSeverity::Info, "", true, true, false, false);
            disconnectFromServer();
        });

        QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::registrationFailed, [=](const QXmppStanza::Error &error) {
            gkEventLogger->publishEvent(tr("Requesting the registration form failed: %1").arg(getErrorCondition(error.condition())), GkSeverity::Fatal, "",
                                        false, true, false, true);
        });

        QObject::connect(this, &QXmppClient::stateChanged, this, [=](QXmppClient::State state) {
            switch (state) {
                case ConnectingState:
                    gkEventLogger->publishEvent(tr("...attempting to make connection towards XMPP server: %1").arg(m_connDetails.server.url), GkSeverity::Info, "",
                                                true, true, true, false);
                    m_netState = GkNetworkState::Connecting;
                    break;
                case ConnectedState:
                    gkEventLogger->publishEvent(tr("Connected to XMPP server: %1").arg(m_connDetails.server.url), GkSeverity::Info, "",
                                                true, true, true, false);
                    m_netState = GkNetworkState::Connected;
                    break;
                case DisconnectedState:
                    gkEventLogger->publishEvent(tr("Disconnected from XMPP server: %1").arg(m_connDetails.server.url), GkSeverity::Info, "",
                                                true, true, true, false);
                    m_netState = GkNetworkState::Disconnected;
                    break;
                default:
                    break;
            }
        });

        //
        // Setting up service discovery correctly for this manager
        // ------------------------------------------------------------
        // This manager automatically recognizes whether the local server
        // supports XEP-0077 (see supportedByServer()). You just need to
        // request the service discovery information from the server on
        // connect as below...
        //
        QObject::connect(this, &QXmppClient::connected, this, [=]() {
            //
            // The service discovery manager is added to the client by default...
            if (m_discoMgr) {
                m_discoMgr->requestInfo(configuration().domain());
            }

            if (m_registerManager->supportedByServer()) {
                gkEventLogger->publishEvent(tr("XEP-0077 is supported by: %1").arg(m_connDetails.server.url),
                                            GkSeverity::Info, "", false, true, false, false);
            } else {
                gkEventLogger->publishEvent(tr("Failed to find support for XEP-0077 on server: %1").arg(m_connDetails.server.url),
                                            GkSeverity::Info, "", false, true, false, false);
            }

            //
            // Manage queues that were created prior to a full connection being made...
            for (; !m_availStatusTypeQueue.empty(); m_availStatusTypeQueue.pop()) {
                m_presence->setAvailableStatusType(m_availStatusTypeQueue.front());
            }

            //
            // Request the client's own vCard from the server...
            m_vCardManager->requestClientVCard();

            //
            // Request vCard of all the bareJids in roster...
            if (!m_rosterList.isEmpty()) {
                for (const auto &entry: m_rosterList) {
                    m_vCardManager->requestVCard(entry.bareJid);
                }
            }
        });

        QObject::connect(this, &QXmppClient::disconnected, this, [=]() {
            if (!m_connDetails.server.settings_client.auto_reconnect && !m_connDetails.server.url.isEmpty() &&
                !m_connDetails.jid.isEmpty()) {
                if (!this->isConnected()) { // We have been disconnected from the given XMPP server!
                    QMessageBox msgBoxPolicy;
                    msgBoxPolicy.setParent(nullptr);
                    msgBoxPolicy.setWindowTitle(tr("Disconnected!"));
                    msgBoxPolicy.setText(tr("You have been disconnected from the XMPP server, \"%1\"! Do you wish to enable a automatic reconnect policy?").arg(m_connDetails.server.url));
                    msgBoxPolicy.setStandardButtons(QMessageBox::Apply | QMessageBox::Ignore | QMessageBox::Cancel);
                    msgBoxPolicy.setDefaultButton(QMessageBox::Apply);
                    msgBoxPolicy.setIcon(QMessageBox::Icon::Information);
                    qint32 ret = msgBoxPolicy.exec();
                    switch (ret) {
                        case QMessageBox::Apply:
                            gkDb->write_xmpp_settings(QString::fromStdString(gkDb->boolEnum(true)), GkXmppCfg::XmppAutoReconnect);
                            gkDb->write_xmpp_settings(QString::fromStdString(gkDb->boolEnum(false)), GkXmppCfg::XmppAutoReconnectIgnore);
                            return;
                        case QMessageBox::Ignore:
                            gkDb->write_xmpp_settings(QString::fromStdString(gkDb->boolEnum(true)), GkXmppCfg::XmppAutoReconnectIgnore);
                            gkDb->write_xmpp_settings(QString::fromStdString(gkDb->boolEnum(false)), GkXmppCfg::XmppAutoReconnect);
                            return;
                        case QMessageBox::Cancel:
                            gkDb->write_xmpp_settings(QString::fromStdString(gkDb->boolEnum(false)), GkXmppCfg::XmppAutoReconnect);
                            gkDb->write_xmpp_settings(QString::fromStdString(gkDb->boolEnum(false)), GkXmppCfg::XmppAutoReconnectIgnore);
                            return;
                        default:
                            gkDb->write_xmpp_settings(QString::fromStdString(gkDb->boolEnum(false)), GkXmppCfg::XmppAutoReconnect);
                            gkDb->write_xmpp_settings(QString::fromStdString(gkDb->boolEnum(false)), GkXmppCfg::XmppAutoReconnectIgnore);
                            return;
                    }
                }
            }
        });
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An issue has occurred within the XMPP subsystem. Error: %1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return;
}

GkXmppClient::~GkXmppClient()
{
    if (isConnected() || m_netState == GkNetworkState::Connecting) {
        disconnectFromServer();
    }

    QObject::disconnect(this, nullptr, this, nullptr);
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
        if ((isConnected() && m_netState != GkNetworkState::Connecting) && !room_name.isEmpty()) {
            QString room_jid = QString("%1@conference.%2").arg(room_name).arg(m_connDetails.server.url);
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
                m_pRoom->setNickName(m_connDetails.nickname);
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
        } else {
            throw std::runtime_error(tr("Are you connected to an XMPP server?").toStdString());
        }
    } catch (const std::exception &e) {
        emit sendError(tr("An issue was encountered while creating a MUC! Error: %1").arg(QString::fromStdString(e.what())));
    }

    return false;
}

/**
 * @brief GkXmppClient::isHostnameSame
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param hostname
 * @param comparison
 * @return
 */
bool GkXmppClient::isHostnameSame(const QString &hostname, const QString &comparison)
{
    return false;
}

/**
 * @brief GkXmppClient::getUsername
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param username
 * @return
 */
QString GkXmppClient::getUsername(const QString &username)
{
    #if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    auto domain = username.split('@', QString::SkipEmptyParts);
    #else
    auto domain = username.split('@', Qt::SkipEmptyParts);
    #endif
    if (!domain.empty()) {
        return domain.first();
    }

    return QString();
}

/**
 * @brief GkXmppClient::getHostname
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param username
 * @return
 */
QString GkXmppClient::getHostname(const QString &username)
{
    #if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    auto domain = username.split('@', QString::SkipEmptyParts);
    #else
    auto domain = username.split('@', Qt::SkipEmptyParts);
    #endif
    if (!domain.empty()) {
        return domain.last();
    }

    return QString();
}

/**
 * @brief GkXmppClient::getNetworkState
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
GekkoFyre::Network::GkXmpp::GkNetworkState GkXmppClient::getNetworkState() const
{
    return m_netState;
}

/**
 * @brief GkXmppClient::isJidExist determines whether the JID exists or not.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The user in question.
 * @return Whether the given bareJid exists or not.
 */
bool GkXmppClient::isJidExist(const QString &bareJid)
{
    for (const auto &entry: m_rosterList) {
        if (entry.bareJid == bareJid) {
            return true;
        }
    }

    return false;
}

/**
 * @brief GkXmppClient::isJidOnline checks to see whether the bareJid is online, in any capacity, at all.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The user in question.
 * @return Whether the given bareJid is online, in any capacity, at all.
 */
bool GkXmppClient::isJidOnline(const QString &bareJid)
{
    for (const auto &entry: m_rosterList) {
        if (entry.bareJid == bareJid) {
            switch (entry.presence->availableStatusType()) {
                case QXmppPresence::Online:
                    return true;
                case QXmppPresence::Away:
                    return true;
                case QXmppPresence::XA:
                    return true;
                case QXmppPresence::DND:
                    return true;
                case QXmppPresence::Chat:
                    return true;
                case QXmppPresence::Invisible:
                    return false;
                default:
                    return false;
            }

            break;
        }
    }

    return false;
}

/**
 * @brief GkXmppClient::getRegistrationMgr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
std::shared_ptr<QXmppRegistrationManager> GkXmppClient::getRegistrationMgr()
{
    return m_registerManager;
}

/**
 * @brief GkXmppClient::getRosterMap
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QVector<GekkoFyre::Network::GkXmpp::GkXmppCallsign> GkXmppClient::getRosterMap()
{
    if (!m_rosterList.isEmpty()) {
        return m_rosterList;
    }

    return QVector<GekkoFyre::Network::GkXmpp::GkXmppCallsign>();
}

/**
 * @brief GkXmppClient::statusToPresence
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param status
 * @return
 */
QXmppPresence GkXmppClient::statusToPresence(const GkXmpp::GkOnlineStatus &status)
{
    auto result = QXmppPresence {};
    result.setType(QXmppPresence::Available);

    switch (status) {
        case GkXmpp::Online:
            result.setAvailableStatusType(QXmppPresence::Chat);
            result.setStatusText(tr("Online"));
            break;
        case GkXmpp::Away:
            result.setAvailableStatusType(QXmppPresence::Away);
            result.setStatusText(tr("Away"));
            break;
        case GkXmpp::DoNotDisturb:
            result.setAvailableStatusType(QXmppPresence::DND);
            result.setStatusText(tr("Do Not Disturb"));
            break;
        case GkXmpp::NotAvailable:
            result.setAvailableStatusType(QXmppPresence::XA);
            result.setStatusText(tr("Not Available"));
            break;
        case GkXmpp::Invisible:
        case GkXmpp::NetworkError:
        default:
            result.setAvailableStatusType(QXmppPresence::Invisible);
            result.setStatusText(tr("Invisible"));
            break;
    }

    return result;
}

/**
 * @brief GkXmppClient::presenceToStatus
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param xmppPresence
 * @return
 */
Network::GkXmpp::GkOnlineStatus GkXmppClient::presenceToStatus(const QXmppPresence::AvailableStatusType &xmppPresence)
{
    auto result = GkXmpp::GkOnlineStatus {};
    switch (xmppPresence) {
        case QXmppPresence::Online:
            return GkXmpp::Online;
        case QXmppPresence::Away:
            return GkXmpp::Away;
        case QXmppPresence::XA:
            return GkXmpp::NotAvailable;
        case QXmppPresence::DND:
            return GkXmpp::DoNotDisturb;
        case QXmppPresence::Chat:
            return GkXmpp::Online;
        case QXmppPresence::Invisible:
            return GkXmpp::Invisible;
        default:
            return GkXmpp::NetworkError;
    }

    return result;
}

/**
 * @brief GkXmppClient::presenceToString
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param xmppPresence
 * @return
 */
QString GkXmppClient::presenceToString(const QXmppPresence::AvailableStatusType &xmppPresence)
{
    switch (xmppPresence) {
        case QXmppPresence::Online:
            return tr("Online");
        case QXmppPresence::Away:
            return tr("Away");
        case QXmppPresence::XA:
            return tr("Not Available");
        case QXmppPresence::DND:
            return tr("Do Not Disturb");
        case QXmppPresence::Chat:
            return tr("Online");
        case QXmppPresence::Invisible:
            return tr("Invisible");
        default:
            return tr("Network Error");
    }

    return QString();
}

/**
 * @brief GkXmppClient::presenceToIcon
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param xmppPresence
 * @return
 */
QIcon GkXmppClient::presenceToIcon(const QXmppPresence::AvailableStatusType &xmppPresence)
{
    switch (xmppPresence) {
        case QXmppPresence::Online:
            return QIcon(":/resources/contrib/images/raster/gekkofyre-networks/green-circle.png");
        case QXmppPresence::Away:
            return QIcon(":/resources/contrib/images/raster/gekkofyre-networks/yellow-circle.png");
        case QXmppPresence::XA:
            return QIcon(":/resources/contrib/images/raster/gekkofyre-networks/yellow-halftone-circle.png");
        case QXmppPresence::DND:
            return QIcon(":/resources/contrib/images/raster/gekkofyre-networks/red-circle.png");
        case QXmppPresence::Chat:
            return QIcon(":/resources/contrib/images/raster/gekkofyre-networks/green-circle.png");
        case QXmppPresence::Invisible:
            return QIcon(":/resources/contrib/images/raster/gekkofyre-networks/green-halftone-circle.png");
        default:
            return QIcon(":/resources/contrib/images/raster/gekkofyre-networks/red-halftone-circle.png");
    }

    return QIcon();
}

/**
 * @brief GkXmppClient::deleteUserAccount will delete a given, already registered user account from the provided XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
bool GkXmppClient::deleteUserAccount()
{
    try {
        if (isConnected()) {
            m_registerManager->deleteAccount();
            QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::accountDeleted, [=]() {
                disconnectFromServer();
                gkEventLogger->publishEvent(tr("User account deleted successfully!"), GkSeverity::Info, "",
                                            false, true, false, true);
            });

            return true;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(e.what(), GkSeverity::Fatal, "", false, true, false, true);
    }

    return false;
}

/**
 * @brief GkXmppClient::obtainAvatarFilePath
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString GkXmppClient::obtainAvatarFilePath()
{
    QString filePath = QFileDialog::getOpenFileName(nullptr, tr("Open Image"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                    tr("All Image Files (*.png *.jpg *.jpeg *.jpe *.jfif *.exif *.bmp *.gif);;PNG (*.png);;JPEG (*.jpg *.jpeg *.jpe *.jfif *.exif);;Bitmap (*.bmp);;GIF (*.gif);;All Files (*.*)"));
    return filePath;
}

/**
 * @brief GkXmppClient::processImgToByteArray processes a given image, or avatar in this case, into a QByteArray so that
 * it is readily usable by QXmppVCardManager().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filePath The path to which image file should be loaded into memory, ready to be processed into a QByteArray.
 * @return The QByteArray as produced from the given image file.
 * @see GkXmppClient::updateClientVCardForm().
 */
QByteArray GkXmppClient::processImgToByteArray(const QString &filePath)
{
    QPointer<QFile> imageFile = new QFile(filePath);
    imageFile->open(QIODevice::ReadOnly);
    QByteArray byteArray = imageFile->readAll();
    imageFile->close();
    delete imageFile;

    return byteArray;
}

/**
 * @brief GkXmppClient::clientConnected
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::clientConnected()
{
    gkEventLogger->publishEvent(tr("A connection has been successfully made towards XMPP server: %1").arg(m_connDetails.server.url),
                                GkSeverity::Info, "", true, true, true, false);

    m_vCardManager->requestVCard(m_connDetails.jid);
    GkXmpp::GkXmppCallsign client_callsign;
    client_callsign.presence = std::make_shared<QXmppPresence>(statusToPresence(m_connDetails.status));
    client_callsign.vCard.nickname = m_connDetails.nickname;
    client_callsign.server = m_connDetails.server;
    client_callsign.bareJid = m_connDetails.jid;
    m_rosterList.push_back(client_callsign);
    emit updateRoster();

    return;
}

/**
 * @brief GkXmppClient::handleRosterReceived is emitted when the Roster IQ is received after a successful connection. That
 * is the roster entries are empty before this signal is emitted. One should use `getRosterBareJids()` and `getRosterEntry()`
 * only after this signal has been emitted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::handleRosterReceived()
{
    auto rosterBareJids = m_rosterManager->getRosterBareJids();
    if (!rosterBareJids.isEmpty()) {
        for (const auto &rawBareJid: rosterBareJids) {
            const auto jidItem = m_rosterManager->getRosterEntry(rawBareJid);
            GkXmppCallsign callsign;
            callsign.bareJid = jidItem.bareJid();
            callsign.presence = std::make_shared<QXmppPresence>(QXmppPresence::Type::Unsubscribed);
            callsign.subStatus = jidItem.subscriptionType();
            m_rosterList.push_back(callsign);
        }
    }

    emit updateRoster();
    return;
}

/**
 * @brief GkXmppClient::vCardReceived processes and saves vCards for users saved/added within the user's roster for the
 * given XMPP server in question. Avatars are also saved in conjunction with the vCard, being part of the vCard itself.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param vCard The vCard in question to process and derive information from.
 */
void GkXmppClient::vCardReceived(const QXmppVCardIq &vCard)
{
    try {
        QString bareJid = vCard.from();
        gkEventLogger->publishEvent(tr("vCard received for user, \"%1\"").arg(bareJid), GkSeverity::Debug,
                                    "", false, true, false, false);

        fs::path imgFileName = fs::path(vcard_save_path.string() + native_slash.string() + bareJid.toStdString() + ".png");
        fs::path xmlFileName = fs::path(vcard_save_path.string() + native_slash.string() + bareJid.toStdString() + ".xml");
        GkXmppVCard vCardTmp;

        QFile xmlFile(QString::fromStdString(xmlFileName.string()));
        if (xmlFile.open(QIODevice::ReadWrite)) {
            QXmlStreamWriter stream(&xmlFile);
            vCard.toXml(&stream);
            xmlFile.close();
            emit sendUserVCard(vCard);

            vCardTmp.firstName = vCard.firstName();
            vCardTmp.lastName = vCard.lastName();
            vCardTmp.nickname = vCard.nickName();
            vCardTmp.email = vCard.email();

            gkEventLogger->publishEvent(tr("vCard XML data saved to filesystem for user, \"%1\"").arg(bareJid),
                                        GkSeverity::Debug, "", false, true, false, false);
        }

        QByteArray photo = vCard.photo();
        vCardTmp.avatarImg = photo;
        for (auto iter = m_rosterList.begin(); iter != m_rosterList.end(); ++iter) {
            if (iter->bareJid == bareJid) {
                iter->vCard = vCardTmp;
                break;
            }
        }

        if (!photo.isEmpty()) {
            QPointer<QBuffer> buffer = new QBuffer(this);
            buffer->setData(photo);
            buffer->open(QIODevice::ReadOnly);
            QImageReader imageReader(buffer);
            QImage image = imageReader.read();
            if (image.save(QString::fromStdString(imgFileName.string()))) {
                gkEventLogger->publishEvent(tr("vCard avatar saved to filesystem for user, \"%1\"").arg(bareJid),
                                            GkSeverity::Debug, "", false, true, false, false);
                return;
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppClient::clientVCardReceived and the likewise signal is emitted when the client's vCard is received after
 * calling the requestClientVCard() function.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::clientVCardReceived()
{
    try {
        m_clientVCard = m_vCardManager->clientVCard();
        fs::path imgFileName = fs::path(vcard_save_path.string() + native_slash.string() + config.jid().toStdString() + ".png");
        fs::path xmlFileName = fs::path(vcard_save_path.string() + native_slash.string() + config.jid().toStdString() + ".xml");
        GkXmppVCard vCardTmp;

        QFile xmlFile(QString::fromStdString(xmlFileName.string()));
        if (xmlFile.open(QIODevice::ReadWrite)) {
            QXmlStreamWriter stream(&xmlFile);
            m_clientVCard.toXml(&stream);
            xmlFile.close();

            vCardTmp.firstName = m_clientVCard.firstName();
            vCardTmp.lastName = m_clientVCard.lastName();
            vCardTmp.nickname = m_clientVCard.nickName();
            vCardTmp.email = m_clientVCard.email();

            gkEventLogger->publishEvent(tr("vCard XML data saved to filesystem for self-client."),
                                        GkSeverity::Debug, "", false, true, false, false);
        }

        QByteArray photo = m_clientVCard.photo();
        vCardTmp.avatarImg = photo;
        for (auto iter = m_rosterList.begin(); iter != m_rosterList.end(); ++iter) {
            if (iter->bareJid == m_connDetails.jid) {
                iter->vCard = vCardTmp;
                break;
            }
        }

        if (!photo.isEmpty()) {
            QPointer<QBuffer> buffer = new QBuffer(this);
            buffer->setData(photo);
            buffer->open(QIODevice::ReadOnly);
            QImageReader imageReader(buffer);
            QImage image = imageReader.read();
            if (image.save(QString::fromStdString(imgFileName.string()))) {
                gkEventLogger->publishEvent(tr("vCard avatar saved to filesystem for self-client."),
                                            GkSeverity::Debug, "", false, true, false, false);
                emit savedClientVCard(photo);
                return;
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppClient::updateClientVCard will update and upload the connecting client's own vCard to the given, connected
 * towards XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param vCard The connecting client's own vCard in question to upload to the given XMPP server.
 */
void GkXmppClient::updateClientVCard(const QXmppVCardIq &vCard)
{
    if (isConnected() && m_netState != GkNetworkState::Connecting) {
        m_vCardManager->setClientVCard(vCard);
        gkEventLogger->publishEvent(tr("Your user details have been updated successfully with the XMPP server!"), GkSeverity::Info, "", true, true, false, false);
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
    try {
        for (auto iter = m_rosterList.begin(); iter != m_rosterList.end(); ++iter) {
            if (iter->bareJid == bareJid) {
                iter->presence = std::make_shared<QXmppPresence>(m_rosterManager->getPresence(bareJid, resource));
                emit updateRoster();
                gkEventLogger->publishEvent(tr("Presence changed for user, \"%1\", towards: %2")
                                                    .arg(bareJid).arg(resource));
                break;
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkXmppClient::modifyPresence sets the presence for the client themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pres The presence to be set towards.
 */
void GkXmppClient::modifyPresence(const QXmppPresence::Type &pres)
{
    try {
        for (auto iter = m_rosterList.begin(); iter != m_rosterList.end(); ++iter) {
            if (iter->bareJid == m_connDetails.jid) {
                m_presence->setType(pres);
                iter->presence.reset();
                iter->presence = std::make_shared<QXmppPresence>(pres);
                emit updateRoster();

                gkEventLogger->publishEvent(tr("You have successfully changed your presence status towards: %1"), GkSeverity::Info, "",
                                            true, true, false, false);
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkXmppClient::getPresence
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @return
 */
std::shared_ptr<QXmppPresence> GkXmppClient::getPresence(const QString &bareJid)
{
    if (!bareJid.isEmpty() && !m_rosterList.isEmpty()) {
        for (const auto &entry: m_rosterList) {
            if (bareJid == entry.bareJid) {
                return entry.presence;
            }
        }
    }

    return std::shared_ptr<QXmppPresence>();
}

/**
 * @brief GkXmppClient::modifyAvailableStatusType changes whether the connecting client themselves is available, invisible,
 * offline, etc. which is then shown to the other users within their roster.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param stat_type The availability status that the connecting client wishes to show to others.
 */
void GkXmppClient::modifyAvailableStatusType(const QXmppPresence::AvailableStatusType &stat_type)
{
    if (isConnected()) {
        m_presence->setAvailableStatusType(stat_type);
    }

    if (!isConnected()) {
        m_availStatusTypeQueue.emplace(stat_type);
    }

    return;
}

/**
 * @brief GkXmppClient::acceptSubscriptionRequest accepts a subscription request (i.e. a user request to be added to the
 * address book).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The username making the request in question.
 */
void GkXmppClient::acceptSubscriptionRequest(const QString &bareJid)
{
    m_rosterManager->acceptSubscription(bareJid);
    m_vCardManager->requestVCard(bareJid);
    gkEventLogger->publishEvent(tr("Invite request has successfully been processed for user, \"%1\"").arg(bareJid),
                                GkSeverity::Info, "", true, true, false, false);

    return;
}

/**
 * @brief GkXmppClient::refuseSubscriptionRequest refuses a subscription request (i.e. a user request to be added to the
 * address book).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The username in question.
 */
void GkXmppClient::refuseSubscriptionRequest(const QString &bareJid)
{
    m_rosterManager->refuseSubscription(bareJid);
    gkEventLogger->publishEvent(tr("Invite request has successfully been refused for user, \"%1\"").arg(bareJid),
                                GkSeverity::Info, "", true, true, false, false);

    return;
}

/**
 * @brief GkXmppClient::blockUser attempts to block a user from making any requests to the client themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::blockUser(const QString &bareJid)
{
    m_blockList.push_back(bareJid);
    gkEventLogger->publishEvent(tr("User, \"%1\", has been successfully added to the blocklist.").arg(bareJid),
                                GkSeverity::Info, "", true, true, false, false);

    return;
}

/**
 * @brief GkXmppClient::unblockUser attempts to unblock a user so that they can make requests once again to the client themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::unblockUser(const QString &bareJid)
{
    for (auto iter = m_blockList.begin(); iter != m_blockList.end(); ++iter) {
        if (*iter == bareJid) {
            m_blockList.erase(iter);
            gkEventLogger->publishEvent(tr("User, \"%1\", has been successfully removed from the blocklist.").arg(bareJid),
                                        GkSeverity::Info, "", true, true, false, false);

            break;
        }
    }

    return;
}

/**
 * @brief GkXmppClient::subscribeToUser requests a subscription to the given contact. As a result, the server will initiate
 * a roster push, causing the `m_rosterManager->itemAdded()` or `m_rosterManager->itemChanged()` signal to be emitted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::subscribeToUser(const QString &bareJid, const QString &reason)
{
    if (!bareJid.isEmpty()) {
        if (!reason.isEmpty()) {
            m_rosterManager->subscribe(bareJid, reason);
            return;
        }

        m_rosterManager->subscribe(bareJid);
    }

    return;
}

/**
 * @brief GkXmppClient::handleRegistrationForm
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param registerIq The user registration form to be filled out, as received from the connected towards XMPP server.
 */
void GkXmppClient::handleRegistrationForm(const QXmppRegisterIq &registerIq)
{
    try {
        qDebug() << "XMPP registration form received: " << registerIq.instructions();
        if (registerIq.type() == QXmppIq::Type::Error) {
            throw std::runtime_error(tr("Requesting the registration form failed: %1").arg(getErrorCondition(registerIq.error().condition())).toStdString());
        } else if (registerIq.type() == QXmppIq::Type::Result) {
            emit sendRegistrationForm(registerIq);
        } else {
            throw std::runtime_error(tr("An error has occurred during user registration with connected XMPP server: %1 received unwanted stanza type %2")
                                             .arg(m_connDetails.server.url).arg(QString::number(registerIq.type())).toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("Issues were encountered with trying to register user with XMPP server! Error: %1")
        .arg(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppClient::updateClientVCardForm processes the details of creating a vCard for the connecting client before
 * sending it to the function, updateClientVCard(), for upload to the given, connected towards XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param first_name The first, birth name of the connecting client.
 * @param last_name The last, birth name of the connecting client.
 * @param email The email address of the connecting client.
 * @param callsign The call-sign, as used in amateur radio, or otherwise a given nickname will do just fine.
 * @param avatar_pic An avatar picture that the connecting client might wish to upload for others to see and identify them.
 */
void GkXmppClient::updateClientVCardForm(const QString &first_name, const QString &last_name, const QString &email,
                                         const QString &callsign, const QByteArray &avatar_pic)
{
    QXmppVCardIq client_vcard;
    if (first_name.isEmpty()) {
        client_vcard.setFirstName(first_name);
    }

    if (last_name.isEmpty()) {
        client_vcard.setLastName(last_name);
    }

    if (callsign.isEmpty()) {
        client_vcard.setNickName(callsign);
    }

    if (email.isEmpty()) {
        client_vcard.setEmail(email);
    }

    if (!avatar_pic.isEmpty()) {
        client_vcard.setPhoto(avatar_pic);
    }

    emit sendClientVCard(client_vcard);
    return;
}

/**
 * @brief GkXmppClient::handleServers
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::handleServers()
{
    //
    // Check that the lookup has succeeded
    switch (m_dns->error()) {
        case QDnsLookup::NoError:
            // No errors! Yay!
            m_dnsRecords = m_dns->serviceRecords();
            if (!m_dnsRecords.isEmpty()) {
                size_t counter = 0;
                for (const auto &record: m_dnsRecords) {
                    ++counter;
                    gkEventLogger->publishEvent(tr("DNS lookup succeeded for, \"%1\"! Record %1 of %2 found: %3 (target port: %4)")
                    .arg(QString::number(counter)).arg(QString::number(m_dnsRecords.size())).arg(record.name()).arg(QString::number(record.port())),
                    GkSeverity::Error, "", false, true, false, true);
                }
            }

            break;
        case QDnsLookup::ResolverError:
            gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\". There has been a Resolver Error.").arg(m_connDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::OperationCancelledError:
            gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\". The operation was cancelled abruptly.").arg(m_connDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::InvalidRequestError:
            gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\". There has been an Invalid Request.").arg(m_connDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::InvalidReplyError:
            gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\". There has been an Invalid Reply.").arg(m_connDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::ServerFailureError:
            gkEventLogger->publishEvent(tr(R"(DNS lookup failed for, "%1". There has been a "Server Failure" error on the other end.)").arg(m_connDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::ServerRefusedError:
            gkEventLogger->publishEvent(tr(R"(DNS lookup failed for, "%1". There has been a "Server Refused" error on the other end.)").arg(m_connDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::NotFoundError:
            gkEventLogger->publishEvent(tr(R"(DNS lookup failed for, "%1". There has been a "Not Found" error.)").arg(m_connDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        default:
            // No errors?
            break;
    }

    return;
}

/**
 * @brief GkXmppClient::handleSuccess
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::handleSuccess()
{
    //
    // TODO: Fill out this section!

    return;
}

/**
 * @brief GkXmppClient::handleError
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param errorMsg
 */
void GkXmppClient::handleError(QXmppClient::Error errorMsg)
{
    switch (errorMsg) {
        case QXmppClient::Error::NoError:
            break;
        case QXmppClient::Error::SocketError:
            emit sendError(tr("XMPP error encountered due to TCP socket. Error: %1").arg(this->socketErrorString()));
            break;
        case QXmppClient::Error::KeepAliveError:
            emit sendError(tr("XMPP error encountered due to no response from a keep alive."));
            break;
        case QXmppClient::Error::XmppStreamError:
            emit sendError(tr("XMPP error encountered due to XML stream. Error: %1").arg(getErrorCondition(this->xmppStreamError())));
            break;
        default:
            emit sendError(tr("An unknown XMPP error has been encountered!"));
            break;
    }

    return;
}

/**
 * @brief GkXmppClient::handleError
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param errorMsg
 */
void GkXmppClient::handleError(const QString &errorMsg)
{
    if (!errorMsg.isEmpty()) {
        if (isConnected() || m_netState == GkNetworkState::Connecting) {
            disconnectFromServer();
        }

        std::cerr << errorMsg.toStdString() << std::endl;
        QMessageBox::warning(nullptr, tr("Error!"), errorMsg, QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkXmppClient::handleSslErrors handles all the SSL errors given by the XMPP connection client, if
 * there are any.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param errorMsg The QList of error messages, if any are present.
 */
void GkXmppClient::handleSslErrors(const QList<QSslError> &errorMsg)
{
    if (!errorMsg.isEmpty()) {
        for (const auto &error: errorMsg) {
            emit sendError(error.errorString());
        }
    }

    return;
}

/**
 * @brief GkXmppClient::handleSocketError
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param errorMsg
 */
void GkXmppClient::handleSocketError(QAbstractSocket::SocketError errorMsg)
{
    switch (errorMsg) {
        case QAbstractSocket::SocketError::ConnectionRefusedError:
            emit sendError(tr("SSL Socket Error: Connection refused."));
            break;
        case QAbstractSocket::RemoteHostClosedError:
            emit sendError(tr("SSL Socket Error: Remote host closed."));
            break;
        case QAbstractSocket::HostNotFoundError:
            emit sendError(tr("SSL Socket Error: Host not found."));
            break;
        case QAbstractSocket::SocketAccessError:
            emit sendError(tr("SSL Socket Error: Socket access error."));
            break;
        case QAbstractSocket::SocketResourceError:
            emit sendError(tr("SSL Socket Error: Socket resource error."));
            break;
        case QAbstractSocket::SocketTimeoutError:
            emit sendError(tr("SSL Socket Error: Socket timeout error."));
            break;
        case QAbstractSocket::DatagramTooLargeError:
            emit sendError(tr("SSL Socket Error: Datagram too large."));
            break;
        case QAbstractSocket::NetworkError:
            emit sendError(tr("SSL Socket Error: Network error."));
            break;
        case QAbstractSocket::AddressInUseError:
            emit sendError(tr("SSL Socket Error: Address in use."));
            break;
        case QAbstractSocket::SocketAddressNotAvailableError:
            emit sendError(tr("SSL Socket Error: Address not available."));
            break;
        case QAbstractSocket::UnsupportedSocketOperationError:
            emit sendError(tr("SSL Socket Error: Unsupported socket operation."));
            break;
        case QAbstractSocket::UnfinishedSocketOperationError:
            emit sendError(tr("SSL Socket Error: Unfinished socket operation."));
            break;
        case QAbstractSocket::ProxyAuthenticationRequiredError:
            emit sendError(tr("SSL Socket Error: Proxy authentication required."));
            break;
        case QAbstractSocket::SslHandshakeFailedError:
            emit sendError(tr("SSL Socket Error: SSL handshake failed."));
            break;
        case QAbstractSocket::ProxyConnectionRefusedError:
            emit sendError(tr("SSL Socket Error: Proxy connection refused."));
            break;
        case QAbstractSocket::ProxyConnectionClosedError:
            emit sendError(tr("SSL Socket Error: Proxy connection closed."));
            break;
        case QAbstractSocket::ProxyConnectionTimeoutError:
            emit sendError(tr("SSL Socket Error: Proxy connection timeout."));
            break;
        case QAbstractSocket::ProxyNotFoundError:
            emit sendError(tr("SSL Socket Error: Proxy not found."));
            break;
        case QAbstractSocket::ProxyProtocolError:
            emit sendError(tr("SSL Socket Error: Proxy protocol error."));
            break;
        case QAbstractSocket::OperationError:
            emit sendError(tr("SSL Socket Error: Operational error."));
            break;
        case QAbstractSocket::SslInternalError:
            emit sendError(tr("SSL Socket Error: Internal SSL error."));
            break;
        case QAbstractSocket::SslInvalidUserDataError:
            emit sendError(tr("SSL Socket Error: Invalid user data error."));
            break;
        case QAbstractSocket::TemporaryError:
            emit sendError(tr("SSL Socket Error: Temporary error."));
            break;
        case QAbstractSocket::UnknownSocketError:
            emit sendError(tr("SSL Socket Error: Unknown socket error."));
            break;
        default:
            break;
    }

    return;
}

/**
 * @brief GkXmppClient::recvXmppLog
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msgType
 * @param msg
 */
void GkXmppClient::recvXmppLog(QXmppLogger::MessageType msgType, const QString &msg)
{
    switch (msgType) {
        case QXmppLogger::MessageType::NoMessage:
            return;
        case QXmppLogger::DebugMessage:
            gkEventLogger->publishEvent(msg, GkSeverity::Debug, "", false, true, false, false);
            return;
        case QXmppLogger::InformationMessage:
            gkEventLogger->publishEvent(msg, GkSeverity::Info, "", false, true, false, false);
            return;
        case QXmppLogger::WarningMessage:
            gkEventLogger->publishEvent(msg, GkSeverity::Warning, "", false, true, false, true);
            return;
        case QXmppLogger::ReceivedMessage:
            gkEventLogger->publishEvent(msg, GkSeverity::Info, "", false, true, false, false);
            return;
        case QXmppLogger::SentMessage:
            gkEventLogger->publishEvent(msg, GkSeverity::Info, "", false, true, false, false);
            return;
        case QXmppLogger::AnyMessage:
            gkEventLogger->publishEvent(msg, GkSeverity::Debug, "", false, true, false, false);
            return;
        default:
            return;
    }

    return;
}

/**
 * @brief GkXmppClient::createConnectionToServer
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */

/**
 * @brief GkXmppClient::createConnectionToServer
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param domain_url
 * @param network_port
 * @param username
 * @param password
 * @param jid
 * @param user_signup
 * @param send_registration_form
 */
void GkXmppClient::createConnectionToServer(const QString &domain_url, const quint16 &network_port, const QString &username,
                                            const QString &password, const QString &jid, const bool &user_signup)
{
    try {
        if (domain_url.isEmpty()) {
            throw std::invalid_argument(tr("An invalid XMPP Server URL has been provided! Please check your settings and try again.").toStdString());
        }

        if (network_port < 80) {
            throw std::invalid_argument(tr("An invalid XMPP Server network port has been provided! It cannot be less than 80/TCP (HTTP)!").toStdString());
        }

        m_registerManager->setRegisterOnConnectEnabled(false);
        if (user_signup) { // Override...
            //
            // Allow the registration of a new user to proceed!
            // Sets whether to only request the registration form and not to connect with username/password.
            //
            m_registerManager->setRegisterOnConnectEnabled(true); // https://doc.qxmpp.org/qxmpp-1/classQXmppRegistrationManager.html
        }

        // Attempt a connection to the given XMPP server! If no username and password are give, then we will attempt to
        // connect anonymously!
        if (!username.isEmpty() && !password.isEmpty() && !m_registerManager->registerOnConnectEnabled()) {
            if (!jid.isEmpty()) {
                config.setJid(jid);
            } else {
                config.setJid(QString("%1@%2").arg(username).arg(GkXmppGekkoFyreCfg::defaultUrl));
            }

            config.setPassword(password);
            config.setHost(domain_url);
            config.setPort(network_port);
        } else {
            if (!domain_url.isEmpty()) {
                m_presence = nullptr;
                config.setDomain(domain_url);
                config.setPort(network_port);
            } else {
                throw std::invalid_argument(tr("The given URL (used for the JID) is empty! As such, connection towards the XMPP server cannot proceed.").toStdString());
            }
        }

        if (!m_registerManager->registerOnConnectEnabled()) {
            //
            // Enable only if we are not attempting an in-band user registration...
            QObject::connect(this, &QXmppClient::presenceReceived, this, [=](const QXmppPresence &presence) {
                gkEventLogger->publishEvent(presence.statusText(), GkSeverity::Info, "", false, true, false, false);
            });
        }

        //
        // You only need to provide a domain to connectToServer()...
        config.setAutoAcceptSubscriptions(false);
        config.setAutoReconnectionEnabled(m_connDetails.server.settings_client.auto_reconnect);
        config.setStreamSecurityMode(QXmppConfiguration::StreamSecurityMode::TLSDisabled);
        if (m_connDetails.server.settings_client.enable_ssl && QSslSocket::supportsSsl()) { // https://xmpp.org/extensions/xep-0035.html
            config.setStreamSecurityMode(QXmppConfiguration::StreamSecurityMode::TLSEnabled);
            qDebug() << tr("SSL version for build: ") << QSslSocket::sslLibraryBuildVersionString();
            qDebug() << tr("SSL version for run-time: ") << QSslSocket::sslLibraryVersionNumber();
        }

        config.setIgnoreSslErrors(false); // Do NOT ignore SSL warnings!
        if (m_connDetails.server.settings_client.ignore_ssl_errors) {
            // We have been instructed to IGNORE any and all SSL warnings!
            config.setIgnoreSslErrors(true);
        }

        config.setResource(General::companyNameMin);
        config.setUseSASLAuthentication(true);
        config.setSaslAuthMechanism("PLAIN");

        if (m_presence) {
            connectToServer(config, *m_presence);
        } else {
            connectToServer(config);
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkXmppClient::handleSslGreeting sends a ping back to the server upon having successfully authenticated on this
 * side (the client's side) regarding SSL/TLS!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::handleSslGreeting()
{
    return;
}

/**
 * @brief GkXmppClient::versionReceivedSlot
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param version
 */
void GkXmppClient::versionReceivedSlot(const QXmppVersionIq &version)
{
    if (version.type() == QXmppIq::Result) {
        QString version_str = version.name() + " " + version.version() + (!version.os().isEmpty() ? "@" + version.os() : QString());
        gkEventLogger->publishEvent(tr("%1 server version: %2").arg(m_connDetails.server.url).arg(version_str), GkSeverity::Info, "", false, true, false, false);
    }

    return;
}

/**
 * @brief GkXmppClient::notifyNewSubscription
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::notifyNewSubscription(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        if (!m_rosterList.isEmpty()) {
            for (const auto &entry: m_rosterList) {
                if (entry.bareJid == bareJid) {
                    GkXmppCallsign callsign;
                    callsign.bareJid = bareJid;
                    callsign.presence = std::make_shared<QXmppPresence>(QXmppPresence::Type::Unsubscribed);
                    callsign.subStatus = entry.subStatus;
                    m_rosterList.push_back(callsign);
                    emit updateRoster();
                    emit sendSubscriptionRequest(bareJid);
                    gkEventLogger->publishEvent(tr("User, \"%1\", wishes to add you to their roster!").arg(getUsername(bareJid)),
                                                GkSeverity::Info, "", true, true, false, false);
                    break;
                }
            }
        }
    }

    return;
}

/**
 * @brief GkXmppClient::itemAdded
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::itemAdded(const QString &bareJid)
{
    emit addJidToRoster(bareJid);
    gkEventLogger->publishEvent(tr("User, \"%1\", successfully added to roster!").arg(getUsername(bareJid)),
                                GkSeverity::Info, "", true, true, false, false);

    return;
}

/**
 * @brief GkXmppClient::itemRemoved
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::itemRemoved(const QString &bareJid)
{
    emit delJidFromRoster(bareJid);
    gkEventLogger->publishEvent(tr("User, \"%1\", successfully removed from roster!").arg(getUsername(bareJid)),
                                GkSeverity::Info, "", true, true, false, false);

    return;
}

/**
 * @brief GkXmppClient::itemChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::itemChanged(const QString &bareJid)
{
    emit changeRosterJid(bareJid);
    gkEventLogger->publishEvent(tr("Details for user, \"%1\", have changed within the roster!").arg(getUsername(bareJid)),
                                GkSeverity::Info, "", true, true, false, false);

    return;
}

/**
 * @brief GkXmppClient::getErrorCondition
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param condition
 * @return
 */
QString GkXmppClient::getErrorCondition(const QXmppStanza::Error::Condition &condition)
{
    switch (condition) {
        case QXmppStanza::Error::BadRequest:
            return tr("Bad request.");
        case QXmppStanza::Error::Conflict:
            return tr("Conflict.");
        case QXmppStanza::Error::FeatureNotImplemented:
            return tr("Feature not implemented.");
        case QXmppStanza::Error::Forbidden:
            return tr("Forbidden.");
        case QXmppStanza::Error::Gone:
            return tr("Gone.");
        case QXmppStanza::Error::InternalServerError:
            return tr("Internal server error.");
        case QXmppStanza::Error::ItemNotFound:
            return tr("Item not found.");
        case QXmppStanza::Error::JidMalformed:
            return tr("JID malformed (see: \"%1\").").arg(configuration().jid());
        case QXmppStanza::Error::NotAcceptable:
            return tr("Not acceptable.");
        case QXmppStanza::Error::NotAllowed:
            return tr("Not allowed.");
        case QXmppStanza::Error::NotAuthorized:
            return tr("Not authorized.");
        case QXmppStanza::Error::RecipientUnavailable:
            return tr("Recipient unavailable.");
        case QXmppStanza::Error::Redirect:
            return tr("Redirect.");
        case QXmppStanza::Error::RegistrationRequired:
            return tr("Registration required.");
        case QXmppStanza::Error::RemoteServerNotFound:
            return tr("Remote server not found.");
        case QXmppStanza::Error::RemoteServerTimeout:
            return tr("Remote server timeout.");
        case QXmppStanza::Error::ResourceConstraint:
            return tr("Resource constraint.");
        case QXmppStanza::Error::ServiceUnavailable:
            return tr("Service unavailable.");
        case QXmppStanza::Error::SubscriptionRequired:
            return tr("Subscription required.");
        case QXmppStanza::Error::UndefinedCondition:
            return tr("Undefined condition.");
        case QXmppStanza::Error::UnexpectedRequest:
            return tr("Unexpected request.");
        case QXmppStanza::Error::PolicyViolation:
            return tr("Policy violation.");
        default:
            break;
    }

    return QString();
}
