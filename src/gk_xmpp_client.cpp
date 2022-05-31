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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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
#include <chrono>
#include <cstring>
#include <iostream>
#include <iterator>
#include <exception>
#include <QFile>
#include <QImage>
#include <QBuffer>
#include <QSysInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QImageWriter>
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

/**
 * @brief GkXmppClient::GkXmppClient The client-class for all such XMPP calls within Small World Deluxe.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param connection_details The details pertaining to making a successful connection towards the given XMPP server.
 * @param eventLogger The object for processing logging information.
 * @param connectNow Whether to initiate a connection now or at a later time instead.
 * @param parent The parent object.
 */
GkXmppClient::GkXmppClient(const GkUserConn &connection_details, QPointer<GekkoFyre::GkLevelDb> database,
                           QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::FileIo> fileIo,
                           QPointer<GekkoFyre::GkSystem> system, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                           const bool &connectNow, QObject *parent) : m_rosterManager(findExtension<QXmppRosterManager>()),
                                              m_registerManager(findExtension<QXmppRegistrationManager>()),
                                              m_versionMgr(findExtension<QXmppVersionManager>()),
                                              m_vCardManager(findExtension<QXmppVCardManager>()),
                                              m_xmppArchiveMgr(findExtension<QXmppArchiveManager>()),
                                              m_xmppMamMgr(findExtension<QXmppMamManager>()),
                                              m_xmppCarbonMgr(findExtension<QXmppCarbonManager>()),
                                              m_xmppLogger(QXmppLogger::getLogger()),
                                              m_discoMgr(findExtension<QXmppDiscoveryManager>()),
                                              m_rosterList(std::make_shared<QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign>>()),
                                              m_mucList(std::make_shared<QList<GekkoFyre::Network::GkXmpp::GkXmppMuc>>()),
                                              QXmppClient(parent)
{
    try {
        setParent(parent);
        m_connDetails = connection_details;
        gkDb = std::move(database);
        gkStringFuncs = std::move(stringFuncs);
        gkFileIo = std::move(fileIo);
        gkSystem = std::move(system);
        gkEventLogger = std::move(eventLogger);
        m_sslSocket = new QSslSocket(this);

        m_registerManager = std::make_shared<QXmppRegistrationManager>();
        m_mucManager = std::make_unique<QXmppMucManager>();
        m_transferManager = std::make_unique<QXmppTransferManager>();
        m_xmppArchiveMgr = std::make_unique<QXmppArchiveManager>();
        m_xmppMamMgr = std::make_unique<QXmppMamManager>();
        m_xmppCarbonMgr = std::make_unique<QXmppCarbonManager>();

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

        if (m_xmppArchiveMgr) {
            addExtension(m_xmppArchiveMgr.get());
        }

        if (m_xmppMamMgr) {
            addExtension(m_xmppMamMgr.get());
        }

        if (m_xmppCarbonMgr) {
            addExtension(m_xmppCarbonMgr.get());
        }

        //
        // Booleans and other variables
        m_askToReconnectAuto = false;
        m_sslIsEnabled = false;
        m_isMuc = false;

        const QString dir_to_append = QDir::toNativeSeparators(QString::fromStdString(General::companyName) + "/" + QString::fromStdString(Filesystem::defaultDirAppend) + "/" + QString::fromStdString(Filesystem::xmppVCardDir));
        vcard_save_path.setPath(QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" + dir_to_append)); // Path to save final database towards
        if (!vcard_save_path.exists()) {
            if (vcard_save_path.mkpath(vcard_save_path.path())) {
                gkEventLogger->publishEvent(tr("Directory, \"%1\", has been created successfully!")
                .arg(vcard_save_path.absolutePath()), GkSeverity::Info, "", false,
                true, false, false);
            } else {
                throw std::runtime_error(tr("Unsuccessfully attempted to capture vCards for XMPP user roster. Directory: %1")
                .arg(vcard_save_path.absolutePath()).toStdString());
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
        QObject::connect(this, SIGNAL(refreshDisplayedClientAvatar(const QByteArray &)),
                         parent, SLOT(updateDisplayedClientAvatar(const QByteArray &)));

        m_status = GkOnlineStatus::Online;
        m_keepalive = 60;

        m_presence = std::make_shared<QXmppPresence>();

        if (m_versionMgr) {
            QSysInfo sysInfo;
            m_versionMgr->setClientName(General::companyNameMin);
            m_versionMgr->setClientVersion(General::appVersion);
            m_versionMgr->setClientOs(sysInfo.prettyProductName());
            gkEventLogger->publishEvent(tr("Client O/S configured as, \"%1\", with regard to XMPP settings.").arg(sysInfo.prettyProductName()), GkSeverity::Debug,
                                        "", false, true, false, false, false);
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
        QObject::connect(m_rosterManager.get(), SIGNAL(subscriptionRequestReceived (const QString &, const QXmppPresence &)), // TODO: No such signal?
                         this, SLOT(notifyNewSubscription(const QString &, const QXmppPresence &)));
        QObject::connect(m_rosterManager.get(), SIGNAL(itemAdded(const QString &)), this, SLOT(itemAdded(const QString &)));
        QObject::connect(m_rosterManager.get(), SIGNAL(itemRemoved(const QString &)), this, SLOT(itemRemoved(const QString &)));
        QObject::connect(m_rosterManager.get(), SIGNAL(itemChanged(const QString &)), this, SLOT(itemChanged(const QString &)));

        //
        // QXmppArchiveManager and QXmppMamManager handling...
        QObject::connect(this, SIGNAL(procXmppMsg(const QXmppMessage &, const bool &)),
                         this, SLOT(updateQueues(const QXmppMessage &, const bool &)));
        QObject::connect(this, SIGNAL(sendXmppMsgToArchive(const QXmppMessage &, const bool &)),
                         this, SLOT(insertArchiveMessage(const QXmppMessage &, const bool &)));
        QObject::connect(this, SIGNAL(sendIncomingResults(QXmppMessage )),
                         this, SLOT(filterIncomingResults(QXmppMessage )));
        QObject::connect(this, SIGNAL(procFirstPartyMsg(const QXmppMessage &, const bool &)),
                         this, SLOT(handleFirstPartyMsg(const QXmppMessage &, const bool &)));
        QObject::connect(this, SIGNAL(procThirdPartyMsg(const QXmppMessage &, const bool &)),
                         this, SLOT(handleThirdPartyMsg(const QXmppMessage &, const bool &)));
        QObject::connect(m_xmppArchiveMgr.get(), SIGNAL(archiveChatReceived(const QXmppArchiveChat &, const QXmppResultSetReply &)),
                         this, SLOT(archiveChatReceived(const QXmppArchiveChat &, const QXmppResultSetReply &)));
        QObject::connect(m_xmppArchiveMgr.get(), SIGNAL(archiveListReceived(const QList<QXmppArchiveChat> &, const QXmppResultSetReply &)),
                         this, SLOT(archiveListReceived(const QList<QXmppArchiveChat> &, const QXmppResultSetReply &)));
        QObject::connect(m_xmppMamMgr.get(), SIGNAL(archivedMessageReceived(const QString &, const QXmppMessage &)),
                         this, SLOT(archivedMessageReceived(const QString &, const QXmppMessage &)));
        QObject::connect(this, SIGNAL(createXmppMuc(const QString &, const QString &, const QString &)),
                         this, SLOT(createMuc(const QString &, const QString &, const QString &)));
        QObject::connect(this, SIGNAL(joinXmppMuc(const QString &)), this, SLOT(joinMuc(const QString &)));
        QObject::connect(m_mucManager.get(), SIGNAL(invitationReceived(const QString &, const QString &, const QString &)),
                         this, SLOT(invitationToMuc(const QString &, const QString &, const QString &)));

        //
        // The spelling mistake for the SIGNAL, QXmppMamManager::resultsRecieved(), seems to be unknowingly intentional by
        // the authors of QXmpp, according to the documentation provided here: https://doc.qxmpp.org/qxmpp-1/classQXmppMamManager.html
        // Tests done and provided by GekkoFyre Networks also show that the SIGNAL works as intended only with the spelling
        // mistake present, so whoever pushed through this change must likely be dyslexic or at the very least, a bad speller!
        //
        QObject::connect(m_xmppMamMgr.get(), SIGNAL(resultsRecieved(const QString &, const QXmppResultSetReply &, bool)),
                         this, SLOT(resultsReceived(const QString &, const QXmppResultSetReply &, bool)));

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
                    createConnectionToServer(GkXmppGekkoFyreCfg::defaultUrl, GK_DEFAULT_XMPP_SERVER_PORT, m_connDetails.password,
                                             m_connDetails.jid, false);
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
                    createConnectionToServer(m_connDetails.server.url, m_connDetails.server.port, m_connDetails.password,
                                             m_connDetails.jid, false);
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
        QObject::connect(this, SIGNAL(messageReceived(const QXmppMessage &)), this, SLOT(recvXmppMsgUpdate(const QXmppMessage &)));
        QObject::connect(m_registerManager.get(), SIGNAL(registrationFormReceived(const QXmppRegisterIq &)),
                         this, SLOT(handleRegistrationForm(const QXmppRegisterIq &)));

        QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::registrationSucceeded, this, [=]() {
            gkEventLogger->publishEvent(tr("User, \"%1\", has been successfully registered with XMPP server: %2").arg(configuration().user())
                                                .arg(configuration().domain()), GkSeverity::Info, "", true, true, false, false);
            killConnectionFromServer(false);
        });

        QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::registrationFailed, [=](const QXmppStanza::Error &error) {
            gkEventLogger->publishEvent(tr("Requesting the registration form failed: %1").arg(getErrorCondition(error.condition())), GkSeverity::Fatal, "",
                                        false, true, false, true);
        });

        QObject::connect(this, &QXmppClient::stateChanged, this, [=](QXmppClient::State state) {
            switch (state) {
                case ConnectingState:
                    emit updateProgressBar((1 / GK_XMPP_CREATE_CONN_PROG_BAR_TOT_PERCT) * 100);
                    gkEventLogger->publishEvent(tr("...attempting to make connection towards XMPP server: %1").arg(m_connDetails.server.url), GkSeverity::Info, "",
                                                true, true, true, false);
                    m_netState = GkNetworkState::Connecting;
                    emit connecting();
                    break;
                case ConnectedState:
                    emit updateProgressBar((2 / GK_XMPP_CREATE_CONN_PROG_BAR_TOT_PERCT) * 100);
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

        QObject::connect(this, &QXmppClient::presenceReceived, this, [=](const QXmppPresence &presence) {
            gkEventLogger->publishEvent(presence.statusText(), GkSeverity::Info, "", false, true, false, false);
            switch (presence.type()) {
                case QXmppPresence::Subscribe:
                    notifyNewSubscription(QXmppUtils::jidToBareJid(presence.from()));
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
            // Enable carbon copies for this client!
            m_xmppCarbonMgr->setCarbonsEnabled(true);
            QObject::connect(m_xmppCarbonMgr.get(), &QXmppCarbonManager::messageSent, this, &QXmppClient::messageReceived);
            QObject::connect(m_xmppCarbonMgr.get(), &QXmppCarbonManager::messageReceived, this, &QXmppClient::messageReceived);
        });

        QObject::connect(this, &QXmppClient::disconnected, this, [=]() {
            m_rosterList->clear(); // Clear the roster-list upon disconnection from given XMPP server!
        });
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An issue has occurred within the XMPP subsystem. Error: %1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return;
}

GkXmppClient::~GkXmppClient()
{
    if (m_archivedMsgsFineThread.joinable()) {
        m_archivedMsgsFineThread.join();
    }

    if (isConnected() || m_netState == GkNetworkState::Connecting) {
        killConnectionFromServer(false);
    }

    QObject::disconnect(this, nullptr, this, nullptr);
}

/**
 * @brief GkXmppClient::createMuc creates a brand new MUC from scratch instead of joining a pre-existing one.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param room_name The title of the soon-to-be created MUC.
 * @param room_subject The subject of the MUC.
 * @param room_desc The description of the MUC.
 * @note musimbate <https://stackoverflow.com/questions/29056790/qxmpp-creating-a-muc-room-xep-0045-on-the-server>,
 */
void GkXmppClient::createMuc(const QString &room_name, const QString &room_subject, const QString &room_desc)
{
    try {
        if ((isConnected() && m_netState != GkNetworkState::Connecting) && !room_name.isEmpty()) {
            QString room_jid = QStringLiteral("%1@conference.%2").arg(room_name, m_connDetails.server.url);
            for (const auto &room: m_mucManager->rooms()) {
                GkXmppMuc gkXmppMuc;
                gkXmppMuc.room_ptr.reset(room);
                gkXmppMuc.jid = room_jid;
                if (gkXmppMuc.room_ptr->jid() == room_jid) {
                    gkEventLogger->publishEvent(tr("%1 has joined the MUC, \"%2\".").arg(tr("<unknown>"), room_jid), GkSeverity::Info, "", true, true, false, false);
                }

                gkXmppMuc.room_ptr.reset(m_mucManager->addRoom(room_jid));
                if (gkXmppMuc.room_ptr) {
                    //
                    // Set the nickname...
                    gkXmppMuc.room_ptr->setNickName(m_connDetails.nickname);

                    //
                    // Then proceed to join the room itself!
                    gkXmppMuc.room_ptr->join();
                }

                QXmppDataForm form(QXmppDataForm::Submit);
                QList<QXmppDataForm::Field> fields;

                {
                    QXmppDataForm::Field field(QXmppDataForm::Field::HiddenField);
                    field.setKey(QStringLiteral("FORM_TYPE"));
                    field.setValue(QStringLiteral("http://jabber.org/protocol/muc#roomconfig"));
                    fields.append(field);
                }

                QXmppDataForm::Field field;
                field.setKey(QStringLiteral("muc#roomconfig_roomname"));
                field.setValue(room_name);
                fields.append(field);

                field.setKey(QStringLiteral("muc#roomconfig_subject"));
                field.setValue(room_subject);
                fields.append(field);

                field.setKey(QStringLiteral("muc#roomconfig_roomdesc"));
                field.setValue(room_desc);
                fields.append(field);

                {
                    QXmppDataForm::Field field(QXmppDataForm::Field::BooleanField);
                    field.setKey(QStringLiteral("muc#roomconfig_persistentroom"));
                    field.setValue(true);
                    fields.append(field);
                }

                form.setFields(fields);
                gkXmppMuc.room_ptr->setConfiguration(form);

                m_mucList->push_back(gkXmppMuc);
            }

            return;
        } else {
            throw std::runtime_error(tr("Are you connected to an XMPP server?").toStdString());
        }
    } catch (const std::exception &e) {
        emit sendError(tr("An issue was encountered while creating a MUC! Error: %1").arg(QString::fromStdString(e.what())));
    }

    return;
}

/**
 * @brief GkXmppClient::joinMuc joins a pre-existing MUC instead of creating a brand new one from scratch.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param room_addr The address to the room/server that is to be joined and connected towards.
 */
void GkXmppClient::joinMuc(const QString &room_addr)
{
    try {
        if ((isConnected() && m_netState != GkNetworkState::Connecting) && !room_addr.isEmpty()) {
            for (const auto &room: m_mucManager->rooms()) {
                GkXmppMuc gkXmppMuc;
                gkXmppMuc.room_ptr.reset(room);
                gkXmppMuc.jid = room_addr;
                if (gkXmppMuc.room_ptr->jid() == room_addr) {
                    gkEventLogger->publishEvent(tr("%1 has joined the MUC, \"%2\".").arg(tr("<unknown>"), room_addr), GkSeverity::Info, "", true, true, false, false);
                }

                gkXmppMuc.room_ptr.reset(m_mucManager->addRoom(room_addr));
                if (gkXmppMuc.room_ptr) {
                    //
                    // Set the nickname...
                    gkXmppMuc.room_ptr->setNickName(m_connDetails.nickname);

                    //
                    // Then proceed to join the room itself!
                    gkXmppMuc.room_ptr->join();
                }

                m_mucList->push_back(gkXmppMuc);
            }

            return;
        } else {
            throw std::runtime_error(tr("Are you connected to an XMPP server?").toStdString());
        }
    } catch (const std::exception &e) {
        emit sendError(tr("An issue was encountered during the joining to an MUC! Error: %1").arg(QString::fromStdString(e.what())));
    }

    return;
}

/**
 * @brief GkXmppClient::invitationToMuc is called upon an invitation being given for an MUC.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param roomJid The address to the MUC itself.
 * @param inviter The end-user who invited you to the MUC.
 * @param reason The reason given for inviting you to the MUC in the first place, if provided.
 */
void GkXmppClient::invitationToMuc(const QString &roomJid, const QString &inviter, const QString &reason)
{
    try {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Invitation!"));
        msgBox.setText(tr("You have been invited to the MUC, \"%1\", by %2? Do you wish to join?\n\nReason given: %3").arg(roomJid, inviter, reason));
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Question);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                emit joinXmppMuc(roomJid);
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }
    } catch (const std::exception &e) {
        emit sendError(tr("An issue was encountered during invitation to an MUC! Error: %1").arg(QString::fromStdString(e.what())));
    }

    return;
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
 * @brief GkXmppClient::getBareJidPresence returns the presence of the given resource of the given bareJid.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The user in question.
 * @param resource The given server resource to utilize in the lookup.
 * @return A QXmppPresence stanza.
 */
QXmppPresence GkXmppClient::getBareJidPresence(const QString &bareJid, const QString &resource)
{
    switch (m_connDetails.server.type) {
        case GkXmpp::GekkoFyre:
            return m_rosterManager->getPresence(bareJid, General::xmppResourceGFyre);
        case GkXmpp::Custom:
            return m_rosterManager->getPresence(bareJid, resource);
        case GkXmpp::Unknown:
            return m_rosterManager->getPresence(bareJid, resource);
        default:
            throw std::invalid_argument(tr("Invalid argument provided with attempted lookup for presence status of user, \"%1\"!")
                                        .arg(gkStringFuncs->getXmppUsername(bareJid)).toStdString());
    }
}

/**
 * @brief GkXmppClient::getJidNickname removes everything after and including the last slash.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @return
 */
QString GkXmppClient::getJidNickname(const QString &bareJid)
{
    QString ret = bareJid;
    qint32 pos = bareJid.lastIndexOf(QChar('/'));


    return ret.left(pos);
}

/**
 * @brief GkXmppClient::addHostname
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param username
 * @return
 */
QString GkXmppClient::addHostname(const QString &username)
{
    QString bareJid = QString(username + "@" + m_connDetails.server.url);
    if (!bareJid.isEmpty()) {
        return bareJid;
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
    for (const auto &entry: *m_rosterList) {
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
    for (const auto &entry: *m_rosterList) {
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
 * @brief GkXmppClient::isAvatarImgScaleOk checks whether the end-user's avatar meets certain criteria for image scaling sizes.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param avatar_img The given end-users avatar that is to be tested against.
 * @param img_type The image format (i.e. extension) of the given end-users avatar.
 * @return Whether the end-users avatar checks out okay or not.
 */
bool GkXmppClient::isAvatarImgScaleOk(const QByteArray &avatar_img, const QString &img_type)
{
    try {
        QPixmap pixmap;
        if (!pixmap.loadFromData(avatar_img, img_type.toStdString().c_str())) {
            throw std::runtime_error(tr("Unable to load avatar image from raw data!").toStdString());
        }

        if (pixmap.size().width() > GK_XMPP_AVATAR_SIZE_MAX_WIDTH || pixmap.size().height() > GK_XMPP_AVATAR_SIZE_MAX_HEIGHT) {
            return true;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An issue has occurred whilst processing your avatar image. Error:\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return false;
}

/**
 * @brief GkXmppClient::calcMinTimestampForXmppMsgHistory calculates the most minimum timestamp applicable to a QList of
 * XMPP messages for use in message history archive retrieval functions and so on.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The user we are in communiqué with!
 * @param msg_history The given list of XMPP messages to calculate the most minimum QDateTime timestamp from.
 * @return The most mimimum QDateTime timestamp applicable to the given list of XMPP messages.
 */
QDateTime GkXmppClient::calcMinTimestampForXmppMsgHistory(const QString &bareJid, const std::shared_ptr<QList<GkXmppCallsign>> &msg_history)
{
    try {
        if (!msg_history->isEmpty()) {
            QDateTime min_timestamp = QDateTime::currentDateTimeUtc();
            for (const auto &stanza: *msg_history) {
                if (stanza.bareJid == bareJid) {
                    for (const auto &mam: stanza.messages) {
                        if (mam.message.isXmppStanza() && !mam.message.body().isEmpty()) {
                            min_timestamp = stanza.messages.at(0).message.stamp();
                            if (msg_history->size() > 1) {
                                if (min_timestamp < mam.message.stamp()) {
                                    min_timestamp = mam.message.stamp();
                                }
                            } else {
                                break;
                            }
                        }
                    }

                    break;
                }
            }

            return min_timestamp;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An error has occurred whilst calculating timestamp information from XMPP message history data.\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return QDateTime::currentDateTimeUtc();
}

/**
 * @brief GkXmppClient::calcMaxTimestampForXmppMsgHistory calculates the most maximum timestamp applicable to a QList of
 * XMPP messages for use in message history archive retrieval functions and so on.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The user we are in communiqué with!
 * @param msg_history The given list of XMPP messages to calculate the most maxmimum QDateTime timestamp from.
 * @return The most maximum QDateTime timestamp applicable to the given list of XMPP messages.
 */
QDateTime GkXmppClient::calcMaxTimestampForXmppMsgHistory(const QString &bareJid, const std::shared_ptr<QList<GkXmppCallsign>> &msg_history)
{
    try {
        if (!msg_history->isEmpty()) {
            if (msg_history->size() > 1) {
                for (const auto &stanza: *msg_history) {
                    if (stanza.bareJid == bareJid) {
                        QDateTime max_timestamp = stanza.messages.at(0).message.stamp();
                        for (const auto &mam: stanza.messages) {
                            if (max_timestamp > mam.message.stamp()) {
                                max_timestamp = mam.message.stamp();
                            }
                        }

                        return max_timestamp;
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An error has occurred whilst calculating timestamp information from XMPP message history data.\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return QDateTime::currentDateTimeUtc();
}

/**
 * @brief GkXmppClient::compareTimestamps compares a given QList() of timestamps and returns the closet
 * matching value to the given input reference.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data The QList of data for making comparisons.
 * @param value The reference value in which to make the comparison against.
 * @return The closest, compared, value which has been found.
 */
qint64 GkXmppClient::compareTimestamps(const std::vector<GekkoFyre::Network::GkXmpp::GkRecvMsgsTableViewModel> &data,
                                       const qint64 &value)
{
    std::vector<qint64> timestamps;
    for (const auto &stanza: data) {
        timestamps.push_back(stanza.timestamp.toMSecsSinceEpoch());
    }

    auto const it = std::lower_bound(timestamps.begin(), timestamps.end(), value);
    if (it == timestamps.end()) { return -1; }
    return *it;
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
std::shared_ptr<QList<GkXmppCallsign>> GkXmppClient::getRosterMap()
{
    return m_rosterList;
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
                killConnectionFromServer(false);
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
 * @brief GkXmppClient::obtainAvatarFilePath is a function for grabbing the absolute file-path for the end-user's choice
 * of avatar image to upload to the given XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param set_dir_path preset a directory to open under than the default option.
 * @return The absolute file-path to the end-user's chosen avatar image.
 */
QString GkXmppClient::obtainAvatarFilePath(const QString &set_dir_path)
{
    QString open_path = set_dir_path;
    if (open_path.isEmpty()) {
        open_path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }

    QString filePath = QFileDialog::getOpenFileName(nullptr, tr("Open Image"), open_path,
                                                    tr("All Image Files (*.png *.jpg *.jpeg *.jpe *.jfif *.exif *.bmp *.gif);;PNG (*.png);;JPEG (*.jpg *.jpeg *.jpe *.jfif *.exif);;Bitmap (*.bmp);;GIF (*.gif);;All Files (*.*)"));
    return filePath;
}

/**
 * @brief GkXmppClient::getArchivedMessagesBulk retrieves archived messages. For each received message, the
 * `m_xmppMamMgr->archivedMessageReceived()` signal is emitted. Once all messages are received, the `m_xmppMamMgr->resultsRecieved()`
 * signal is emitted. It returns a result set that can be used to page through the results. The number of results may
 * be limited by the server. This function in particular tries to receive all of the archived messages in question via
 * a bulk manner, grabbing what it can and as much of it as possible, quite unlike its cousin function which is much
 * more fine-tuned, GkXmppClient::getArchivedMessagesFine().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param from
 * @see GkXmppClient::getArchivedMessagesFine().
 */
void GkXmppClient::getArchivedMessagesBulk(const QString &from)
{
    try {
        QXmppResultSetQuery queryLimit;
        queryLimit.setBefore("");
        queryLimit.setMax(GK_XMPP_MAM_BACKLOG_BULK_FETCH_COUNT);

        //
        // Retrieve any archived messages from the given XMPP server as according to the specification, XEP-0313!
        m_xmppMamMgr->retrieveArchivedMessages({}, {}, from, {}, QDateTime::currentDateTimeUtc(), queryLimit);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkXmppClient::getArchivedMessagesFine retrieves archived messages. For each received message, the
 * `m_xmppMamMgr->archivedMessageReceived()` signal is emitted. Once all messages are received, the `m_xmppMamMgr->resultsRecieved()`
 * signal is emitted. It returns a result set that can be used to page through the results. The number of results may
 * be limited by the server. This function in particular tries to receive a particular set of the archived messages in
 * question via a filtered manner, grabbing what it via calculated QDateTime's, quite unlike its cousin function which is
 * more for grabbing a bulk amount of messages, GkXmppClient::getArchivedMessagesBulk().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param recursion The amount of times this function has been recurisvely executed.
 * @param from The bareJid we are receiving from.
 * @see GkXmppClient::getArchivedMessagesBulk(), GkXmppMessageHandler::addToQueue()
 */
void GkXmppClient::getArchivedMessagesFine(qint32 recursion, const QString &from)
{
    try {
        QXmppResultSetQuery queryLimit;
        queryLimit.setBefore("");
        queryLimit.setMax(GK_XMPP_MAM_BACKLOG_FINE_FETCH_COUNT);
        while (!m_msgRetrievalTimestamps.empty()) { // While more than '0' in GkXmppMessageHandler::addToQueue()!
            //
            // Retrieve any archived messages from the given XMPP server as according to the specification, XEP-0313!
            m_xmppMamMgr->retrieveArchivedMessages({}, {}, from, m_msgRetrievalTimestamps.front(), QDateTime::currentDateTimeUtc(), queryLimit);
            std::this_thread::sleep_for(std::chrono::milliseconds(GK_XMPP_MAM_THREAD_SLEEP_MILLISECS));
        }

        return;
    } catch (const std:: exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkXmppClient::filterIncomingResults filters incoming messages that are being processed as part of handling by
 * function, `GkXmppClient::resultsReceived()`, and assigning said messages to the proper roster with regard to the
 * class-wide variable, `m_rosterList`, from the QQueue-based message queue within the associated struct.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param mam_msg_list
 */
void GkXmppClient::filterIncomingResults(QXmppMessage message)
{
    try {
        //
        // This object does not currently exist in memory!
        emit procXmppMsg(message, false);
        emit sendXmppMsgToArchive(message, false);
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    return;
}

/**
 * @brief GkXmppClient::insertArchiveMessage inserts a QXmppMessage stanza according to XMPP standard, XEP-0313, into the
 * class-wide variable, `m_rosterList`, but only after it has passed all the tests, including filters.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param message The message stanza itself that is to be inserted into memory.
 * @param enqueue Whether the incoming message(s) should be added to a queue for further processing or not, or otherwise
 * be made ready as quickly as possible for displaying to the end-user. Please be aware of the context you are using this
 * parameter within.
 * @see GkXmppClient::archivedMessageReceived().
 */
void GkXmppClient::insertArchiveMessage(const QXmppMessage &message, const bool &enqueue)
{
    try {
        if (message.isXmppStanza() && !message.body().isEmpty()) {
            emit procFirstPartyMsg(message, enqueue);
            emit procThirdPartyMsg(message, enqueue);

            return;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(e.what(), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppClient::processImgToByteArray processes a given image, or avatar in this case, into a QByteArray so that
 * it is readily usable by QXmppVCardManager().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filePath The path to which image file should be loaded into memory, ready to be processed into a QByteArray.
 * @return The QByteArray as produced from the given image file.
 * @see GkXmppClient::updateClientVCardForm().
 */
QByteArray GkXmppClient::processImgToByteArray(const QFileInfo &filePath)
{
    try {
        if (filePath.exists() && filePath.isFile() && filePath.isReadable()) {
            QScopedPointer<QFile> imageFile = QScopedPointer<QFile>(new QFile(filePath.canonicalFilePath(), this));
            if (!imageFile->open(QIODevice::ReadOnly)) {
                throw std::invalid_argument(tr("Error with reading avatar! Unable to open file: %1").arg(filePath.canonicalFilePath()).toStdString());
            }

            const QByteArray byteArray = imageFile->readAll();
            imageFile->close();
            return byteArray;
        } else {
            throw std::invalid_argument(tr("Error with reading avatar!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QByteArray();
}

/**
 * @brief GkXmppClient::rescaleAvatarImg scales a given end-user's avatar image to a pre-specified, constrained size.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param avatar_img The given end-users avatar that is to be modified.
 * @param img_type The image format (i.e. extension) of the given end-users avatar.
 * @return The now scaled end-users avatar.
 */
QPixmap GkXmppClient::rescaleAvatarImg(const QByteArray &avatar_img, const QString &img_type)
{
    try {
        QPixmap pixmap;
        if (!pixmap.loadFromData(avatar_img, img_type.toStdString().c_str())) {
            throw std::runtime_error(tr("Unable to load avatar image from raw data!").toStdString());
        }

        if (!isAvatarImgScaleOk(avatar_img, img_type)) {
            // Make sure to use bilinear filtering despite its potential performance constraints on slower computer
            // systems...
            pixmap.scaled(QSize(150, 150), Qt::AspectRatioMode::KeepAspectRatioByExpanding,
                          Qt::TransformationMode::SmoothTransformation);
            return pixmap;
        }

        return pixmap;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An issue has occurred whilst processing your avatar image. Error:\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return QPixmap();
}

/**
 * @brief GkXmppClient::clientConnected
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::clientConnected()
{
    emit updateProgressBar((3 / GK_XMPP_CREATE_CONN_PROG_BAR_TOT_PERCT) * 100);
    gkEventLogger->publishEvent(tr("A connection has been successfully made towards XMPP server: %1").arg(m_connDetails.server.url),
                                GkSeverity::Info, "", true, true, true, false);

    m_vCardManager->requestVCard(m_connDetails.jid);
    GkXmpp::GkXmppCallsign client_callsign;
    client_callsign.presence = std::make_shared<QXmppPresence>(statusToPresence(m_connDetails.status));
    client_callsign.vCard.nickName() = m_connDetails.nickname;
    client_callsign.server = m_connDetails.server;
    client_callsign.bareJid = m_connDetails.jid;
    client_callsign.msg_window_idx = GK_XMPP_MSG_WINDOW_CLIENT_SELF_TAB_IDX;
    client_callsign.party = GkXmppParty::FirstParty;
    m_rosterList->push_back(client_callsign);

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
            callsign.party = GkXmppParty::ThirdParty;
            callsign.msg_window_idx = GK_XMPP_MSG_WINDOW_UNSET_TAB_IDX;
            callsign.presence = std::make_shared<QXmppPresence>(QXmppPresence::Type::Unavailable);
            callsign.subStatus = jidItem.subscriptionType();
            switch (jidItem.subscriptionType()) {
                case QXmppRosterIq::Item::None:
                    emit retractSubscriptionRequest(callsign.bareJid);
                    emit delJidFromRoster(callsign.bareJid);
                    break;
                case QXmppRosterIq::Item::Both:
                case QXmppRosterIq::Item::To:
                case QXmppRosterIq::Item::From:
                    m_rosterList->push_back(callsign);
                    m_vCardManager->requestVCard(callsign.bareJid);
                    emit retractSubscriptionRequest(callsign.bareJid);
                    emit addJidToRoster(callsign.bareJid);
                    break;
                case QXmppRosterIq::Item::Remove:
                    emit retractSubscriptionRequest(callsign.bareJid);
                    break;
                case QXmppRosterIq::Item::NotSet:
                    m_rosterList->push_back(callsign);
                    notifyNewSubscription(callsign.bareJid);
                    break;
                default:
                    break;
            }
        }
    }

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

        QFileInfo imgFileName = QDir::toNativeSeparators(vcard_save_path.absolutePath() + "/" +
                                                                 gkStringFuncs->getXmppHostname(bareJid) + "/" +
                                                         gkStringFuncs->getXmppUsername(bareJid) + ".png");
        QFileInfo xmlFileName = QDir::toNativeSeparators(vcard_save_path.absolutePath() + "/" +
                                                                 gkStringFuncs->getXmppHostname(bareJid) + "/" +
                                                         gkStringFuncs->getXmppUsername(bareJid) + ".xml");

        if (!QDir(xmlFileName.absolutePath()).exists()) {
            //
            // The host sub-folder does not exist!
            if (!QDir().mkdir(xmlFileName.absolutePath())) {
                throw std::runtime_error(tr("An error was encountered with creating directory, \"%1\"!")
                .arg(xmlFileName.absolutePath()).toStdString());
            }
        }

        QFile xmlFile(xmlFileName.filePath());
        if (!xmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return;
        }

        QXmlStreamWriter stream(&xmlFile);
        vCard.toXml(&stream);
        xmlFile.commitTransaction();
        emit sendUserVCard(vCard);
        xmlFile.close();

        gkEventLogger->publishEvent(tr("vCard XML data saved to filesystem for user, \"%1\"").arg(bareJid),
                                    GkSeverity::Debug, "", false, true, false, false);

        const QString img_file_path = imgFileName.absoluteFilePath();
        QByteArray photo = vCard.photo();
        for (auto iter = m_rosterList->begin(); iter != m_rosterList->end(); ++iter) {
            if (iter->bareJid == bareJid) {
                iter->vCard = vCard;
                break;
            }
        }

        if (!photo.isNull() && !photo.isEmpty()) {
            QImage image;
            image.loadFromData(photo, vCard.photoType().toStdString().c_str());
            QImageWriter image_save(img_file_path);
            if (image_save.write(image)) {
                gkEventLogger->publishEvent(tr("vCard avatar saved to filesystem for user, \"%1\"").arg(bareJid),
                                            GkSeverity::Debug, "", false, true, false, false);
                if (imgFileName.baseName() == gkStringFuncs->getXmppUsername(m_connDetails.jid)) {
                    emit refreshDisplayedClientAvatar(photo);
                }

                return;
            } else {
                throw std::runtime_error(tr("Error encountered with saving image data for file, \"%1\"!")
                .arg(img_file_path).toStdString());
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
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
    const QXmppVCardIq m_clientVCard = m_vCardManager->clientVCard();
    const QFileInfo imgFileLoc = QDir::toNativeSeparators(vcard_save_path.absolutePath() + "/" + config.jidBare() + ".png");
    const QFileInfo xmlFileLoc = QDir::toNativeSeparators(vcard_save_path.absolutePath() + "/" + config.jidBare() + ".xml");

    try {
        //
        // Read/write out user contact information!
        if (xmlFileLoc.absoluteDir().exists() && xmlFileLoc.absoluteDir().isReadable()) {
            QDir::setCurrent(xmlFileLoc.absolutePath()); // Set the working directory to this!
            QPointer<QFile> xmlFile = new QFile(xmlFileLoc.absoluteFilePath(), this);
            if (xmlFile->open(QIODevice::ReadWrite | QIODevice::Truncate)) { // Overwrite the XML file wholly each time, no matter if it is pre-existing or not!
                QXmlStreamWriter stream(xmlFile);
                m_clientVCard.toXml(&stream);
                xmlFile->close();

                gkEventLogger->publishEvent(tr("vCard XML data saved to filesystem for self-client."),
                                            GkSeverity::Debug, "", false, true, false, false);
            }
        } else {
            throw std::invalid_argument(tr("Unable to read/write client vCard information!").toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    try {
        //
        // Read/write out avatar information!
        if (imgFileLoc.absoluteDir().exists() && imgFileLoc.absoluteDir().isReadable()) {
            QByteArray photo = m_clientVCard.photo();
            for (auto iter = m_rosterList->begin(); iter != m_rosterList->end(); ++iter) {
                if (iter->bareJid == m_connDetails.jid) {
                    iter->vCard = m_clientVCard;
                    break;
                }
            }

            if (!photo.isNull()) {
                if (!photo.isEmpty()) {
                    QPointer<QBuffer> buffer = new QBuffer(&photo, this);
                    buffer->open(QIODevice::ReadOnly);
                    QImageReader imageReader(buffer, m_clientVCard.photoType().toStdString().c_str());
                    QDir::setCurrent(imgFileLoc.absolutePath()); // Set the working directory to this!
                    QImage image = imageReader.read();
                    if (image.save(imgFileLoc.absoluteFilePath(), m_clientVCard.photoType().toStdString().c_str())) {
                        gkEventLogger->publishEvent(tr("vCard avatar saved to filesystem for self-client."),
                                                    GkSeverity::Debug, "", false, true, false, false);
                        emit savedClientVCard(photo, m_clientVCard.photoType());
                        return;
                    }
                } else {
                    gkEventLogger->publishEvent(tr("No client avatar that has been previously saved to the connected towards XMPP server was detected; using defaults."), GkSeverity::Info, "",
                                                false, true, false, false, false);
                }
            }
        } else {
            throw std::invalid_argument(tr("Unable to read/write client vCard information!").toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
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
        for (auto iter = m_rosterList->begin(); iter != m_rosterList->end(); ++iter) {
            if (iter->bareJid == bareJid) {
                iter->presence = std::make_shared<QXmppPresence>(m_rosterManager->getPresence(bareJid, resource));
                emit updateProgressBar((4 / GK_XMPP_CREATE_CONN_PROG_BAR_TOT_PERCT) * 100);
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
        for (auto iter = m_rosterList->begin(); iter != m_rosterList->end(); ++iter) {
            if (iter->bareJid == m_connDetails.jid && iter->party == GkXmppParty::FirstParty) {
                m_presence->setType(pres);
                iter->presence.reset();
                iter->presence = std::make_shared<QXmppPresence>(pres);

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
    if (!bareJid.isEmpty() && !m_rosterList->isEmpty()) {
        for (const auto &entry: *m_rosterList) {
            if (bareJid == entry.bareJid && entry.party == GkXmppParty::ThirdParty) {
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
            iter = m_blockList.erase(iter);
            gkEventLogger->publishEvent(tr("User, \"%1\", has been successfully removed from the blocklist.").arg(bareJid),
                                        GkSeverity::Info, "", true, true, false, false);

            break;
        } else {
            ++iter;
        }
    }

    return;
}

/**
 * @brief GkXmppClient::subscribeToUser requests a subscription to the given contact. As a result, the server will initiate
 * a roster push, causing the `m_rosterManager->itemAdded()` or `m_rosterManager->itemChanged()` signal to be emitted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The identification of the user that wants to subscribe to the client's presence.
 * @param reason The reason for making the subscription, as defined by the client.
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
 * @brief GkXmppClient::unsubscribeToUser removes a subscription to the given contact.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The identification of the user that wants to unsubscribe from the client's presence.
 * @param reason The reason for making the unsubscription, as defined by the client.
 */
void GkXmppClient::unsubscribeToUser(const QString &bareJid, const QString &reason)
{
    if (!bareJid.isEmpty()) {
        if (!reason.isEmpty()) {
            m_rosterManager->unsubscribe(bareJid, reason);
            return;
        }

        m_rosterManager->unsubscribe(bareJid);
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
 * @param img_type The image format that the avatar is originally within (i.e. PNG, JPEG, GIF, etc).
 */
void GkXmppClient::updateClientVCardForm(const QString &first_name, const QString &last_name, const QString &email,
                                         const QString &callsign, const QByteArray &avatar_pic, const QString &img_type)
{
    QXmppVCardIq client_vcard;
    if (!first_name.isEmpty()) {
        client_vcard.setFirstName(first_name);
    }

    if (!last_name.isEmpty()) {
        client_vcard.setLastName(last_name);
    }

    if (!callsign.isEmpty()) {
        client_vcard.setNickName(callsign);
    }

    if (!email.isEmpty()) {
        client_vcard.setEmail(email);
    }

    if (!avatar_pic.isEmpty()) {
        client_vcard.setPhoto(avatar_pic);
        client_vcard.setPhotoType(img_type);
    }

    emit sendClientVCard(client_vcard);
    return;
}

/**
 * @brief GkXmppClient::sendXmppMsg is a utility function to send messages to all the resources associated with the specified
 * bareJid(s) within the contained QXmppMessage stanza.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msg The QXmppMessage to process and ultimately, transmit.
 */
void GkXmppClient::sendXmppMsg(const QXmppMessage &msg)
{
    if (msg.isXmppStanza()) {
        std::lock_guard<std::mutex> lck_guard(m_archivedMsgsFineMtx);
        const bool msg_sent_succ = sendPacket(msg);
        if (msg_sent_succ) {
            const auto curr_timestamp = QDateTime::currentDateTimeUtc();
            m_msgRetrievalTimestamps.emplace(curr_timestamp);
            m_archivedMsgsFineThread = std::thread(&GkXmppClient::getArchivedMessagesFine, this, 0, msg.to());
            m_archivedMsgsFineThread.detach();
        }
    }

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
    if (!errorMsg.isNull() && !errorMsg.isEmpty()) {
        if (isConnected() || m_netState != GkNetworkState::Disconnected) {
            m_netState = GkNetworkState::Disconnected;
            killConnectionFromServer(true);
        }

        std::cerr << errorMsg.toStdString() << std::endl;
        gkEventLogger->publishEvent(errorMsg, GkSeverity::Fatal, "", false, true, false, true, false);
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
            if (!error.errorString().isEmpty()) {
                emit sendError(error.errorString());
            }
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
void GkXmppClient::createConnectionToServer(const QString &domain_url, const quint16 &network_port, const QString &password,
                                            const QString &jid, const bool &user_signup)
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
        if (!jid.isEmpty() && !password.isEmpty() && !m_registerManager->registerOnConnectEnabled()) {
            config.setJid(jid);
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
 * @brief GkXmppClient::killConnectionFromServer is a housekeeping function for managing disconnections from the configured
 * XMPP server, while taking into account other facets of Small World Deluxe at the same time and how they might
 * interact with such a disconnection.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param askReconnectPolicy Whether to ask about reconnecting automatically or not when disconnected suddenly, without
 * reason, from the configured XMPP server.
 */
void GkXmppClient::killConnectionFromServer(const bool &askReconnectPolicy)
{
    m_askToReconnectAuto = askReconnectPolicy;
    disconnectFromServer();
    m_netState = GkNetworkState::Disconnected;

    return;
}

/**
 * @brief GkXmppClient::handleSslGreeting sends a ping back to the server upon having successfully authenticated on this
 * side (the client's side) regarding SSL/TLS!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::handleSslGreeting()
{
    if (m_sslSocket->isEncrypted()) {
        m_sslIsEnabled = true;
        return;
    }

    m_sslIsEnabled = false;
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
 * @brief GkXmppClient::notifyNewSubscription is the result of a signal that's emitted when a JID asks to subscribe to
 * the client's presence.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The identification of the user that wants to subscribe to the client's presence.
 */
void GkXmppClient::notifyNewSubscription(const QString &bareJid)
{
    notifyNewSubscription(bareJid, QXmppPresence());
    return;
}

/**
 * @brief GkXmppClient::notifyNewSubscription is the result of a signal that's emitted when a JID asks to subscribe to
 * the client's presence.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The identification of the user that wants to subscribe to the client's presence.
 * @param presence Stanza containing the reason / message (presence.statusText()).
 */
void GkXmppClient::notifyNewSubscription(const QString &bareJid, const QXmppPresence &presence)
{
    try {
        if (!bareJid.isEmpty()) {
            GkXmppCallsign callsign;
            callsign.bareJid = bareJid;
            callsign.presence = std::make_shared<QXmppPresence>(presence);
            callsign.subStatus = QXmppRosterIq::Item::SubscriptionType::NotSet; // TODO: Change this so it is set dynamically, and therefore can handle more possible situations!
            callsign.msg_window_idx = GK_XMPP_MSG_WINDOW_UNSET_TAB_IDX;
            callsign.party = GkXmppParty::ThirdParty;
            m_rosterList->push_back(callsign);
            if (!presence.statusText().isEmpty()) {
                emit sendSubscriptionRequest(bareJid, presence.statusText());
            } else {
                emit sendSubscriptionRequest(bareJid);
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An issue was encountered whilst processing a subscription request! Error:\n\n%1").arg(QString::fromStdString(e.what())));
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
    emit retractSubscriptionRequest(bareJid);
    emit addJidToRoster(bareJid);
    gkEventLogger->publishEvent(tr("User, \"%1\", successfully added to roster!").arg(
                                        gkStringFuncs->getXmppUsername(bareJid)),
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
    gkEventLogger->publishEvent(tr("User, \"%1\", successfully removed from roster!").arg(
                                        gkStringFuncs->getXmppUsername(bareJid)),
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
    gkEventLogger->publishEvent(tr("Details for user, \"%1\", have changed within the roster!").arg(
                                        gkStringFuncs->getXmppUsername(bareJid)),
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

/**
 * @brief GkXmppClient::recvXmppMsgUpdate notifies that an XMPP message stanza is received. The QXmppMessage parameter
 * contains the details of the message sent to this client. In other words whenever someone sends you a message this signal
 * is emitted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param message The received message stanza in question.
 */
void GkXmppClient::recvXmppMsgUpdate(const QXmppMessage &message)
{
    try {
        switch (message.type()) {
            case QXmppMessage::Normal:
                gkEventLogger->publishEvent(tr("Received a message with type, \"Normal\", not sure what to do with it so skipping for now..."), GkSeverity::Debug,
                                            "", false, true, false, false);
                return;
            case QXmppMessage::Error:
                throw std::invalid_argument(tr("Chat message received with an error!").toStdString());
            case QXmppMessage::Chat:
                emit xmppMsgUpdate(message);
                gkEventLogger->publishEvent(tr("QXmppMessage stanza has been received!"), GkSeverity::Debug, "", false, true, false, false);
                return;
            case QXmppMessage::GroupChat:
                emit xmppMsgUpdate(message);
                gkEventLogger->publishEvent(tr("QXmppMessage stanza has been received!"), GkSeverity::Debug, "", false, true, false, false);
                return;
            case QXmppMessage::Headline:
                gkEventLogger->publishEvent(tr("Received a message with type, \"Headline\", not sure what to do with it so skipping for now..."), GkSeverity::Debug,
                                            "", false, true, false, false);
                return;
            default:
                return;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    return;
}

/**
 * @brief GkXmppClient::archiveListReceived processes and manages the archiving of chat messages and their histories from
 * a given XMPP server, provided said server supports this functionality.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param chats
 * @param rsmReply
 */
void GkXmppClient::archiveListReceived(const QList<QXmppArchiveChat> &chats, const QXmppResultSetReply &rsmReply)
{
    for (const auto &chat: chats) {
        for (auto iter = m_rosterList->begin(); iter != m_rosterList->end(); ++iter) {
            if (iter->bareJid == chat.with()) {
                QList<GkXmppArchiveMsg> msg_struct_list;
                for (const auto &message: chat.messages()) {
                    GkXmppArchiveMsg arch_msg;
                    arch_msg.message = message;
                    arch_msg.presented = false;
                    msg_struct_list.push_back(arch_msg);
                }

                if (!msg_struct_list.isEmpty()) {
                    std::copy(msg_struct_list.begin(), msg_struct_list.end(), std::back_inserter(iter->archive_messages));
                }

                break;
            }
        }
    }

    emit updateMsgHistory();
    return;
}

/**
 * @brief GkXmppClient::archiveChatReceived processes and manages the archiving of chat messages and their histories from
 * a given XMPP server, provided said server supports this functionality.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param chat
 * @param rsmReply
 */
void GkXmppClient::archiveChatReceived(const QXmppArchiveChat &chat, const QXmppResultSetReply &rsmReply)
{
    for (auto iter = m_rosterList->begin(); iter != m_rosterList->end(); ++iter) {
        if (iter->bareJid == chat.with()) {
            QList<GkXmppArchiveMsg> arch_msg_list;
            for (const auto &message: chat.messages()) {
                GkXmppArchiveMsg arch_msg;
                arch_msg.message = message;
                arch_msg.presented = false;
                arch_msg_list.push_back(arch_msg);
            }

            if (!arch_msg_list.isEmpty()) {
                std::copy(arch_msg_list.begin(), arch_msg_list.end(), std::back_inserter(iter->archive_messages));
            }

            break;
        }
    }

    emit updateMsgHistory();
    return;
}

/**
 * @brief GkXmppClient::handleFirstPartyMsg processes the handling of first-party messages.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param message The message stanza to handle/process.
 * @param enqueue Whether the incoming message(s) should be added to a queue for further processing or not, or otherwise
 * be made ready as quickly as possible for displaying to the end-user. Please be aware of the context you are using this
 * parameter within.
 */
void GkXmppClient::handleFirstPartyMsg(const QXmppMessage &message, const bool &enqueue)
{
    try {
        if (message.isXmppStanza() && !message.body().isEmpty()) {
            //
            // Messages that have been received towards/from the first-party!
            for (auto iter = m_rosterList->begin(); iter != m_rosterList->end(); ++iter) {
                if (iter->bareJid == m_connDetails.jid && iter->party == GkXmppParty::FirstParty) {
                    if (m_connDetails.jid == message.to() || m_connDetails.jid == getJidNickname(message.from())) {
                        GkXmppMamMsg mam_msg;
                        mam_msg.message = message;
                        mam_msg.presented = false;
                        mam_msg.party = GkXmppParty::FirstParty;
                        if (enqueue) {
                            iter->msg_queue.enqueue(mam_msg);
                            break;
                        }

                        iter->messages.push_back(mam_msg);
                        break;
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(e.what(), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppClient::handleThirdPartyMsg processes the handling of third-party messages.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param message The message stanza to handle/process.
 * @param enqueue Whether the incoming message(s) should be added to a queue for further processing or not, or otherwise
 * be made ready as quickly as possible for displaying to the end-user. Please be aware of the context you are using this
 * parameter within.
 */
void GkXmppClient::handleThirdPartyMsg(const QXmppMessage &message, const bool &enqueue)
{
    try {
        if (message.isXmppStanza() && !message.body().isEmpty()) {
            //
            // Messages that have been sent towards/from the third-party!
            for (auto iter = m_rosterList->begin(); iter != m_rosterList->end(); ++iter) {
                if ((iter->bareJid == message.to() || iter->bareJid == getJidNickname(message.from())) && iter->party == GkXmppParty::ThirdParty) {
                    GkXmppMamMsg mam_msg;
                    mam_msg.message = message;
                    mam_msg.presented = false;
                    mam_msg.party = GkXmppParty::ThirdParty;
                    if (enqueue) {
                        iter->msg_queue.enqueue(mam_msg);
                        break;
                    }

                    iter->messages.push_back(mam_msg);
                    break;
                }
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(e.what(), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppClient::archivedMessageReceived is a signal that's emitted when an archived message is received. Inserts
 * QXmppMessage stanzas into the class-wide variable, `m_rosterList`, if applicable.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param queryId Unknown.
 * @param message The message stanza itself that the comparison is either to be made against and/or to be inserted into
 * memory via the class-wide variable, `m_rosterList`.
 * @see GkXmppClient::getArchivedMessagesBulk(), GkXmppClient::getArchivedMessagesFine(), GkXmppClient::filterArchivedMessage()
 */
void GkXmppClient::archivedMessageReceived(const QString &queryId, const QXmppMessage &message)
{
    Q_UNUSED(queryId);
    emit sendXmppMsgToArchive(message, true);

    return;
}

/**
 * @brief GkXmppClient::resultsReceived is a signal that's emitted when all results for a request have been received.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param queryId
 * @param resultSetReply
 * @param complete
 */
void GkXmppClient::resultsReceived(const QString &queryId, const QXmppResultSetReply &resultSetReply, bool complete)
{
    Q_UNUSED(queryId);
    Q_UNUSED(resultSetReply);
    Q_UNUSED(complete);

    try {
        if (!m_rosterList->isEmpty()) {
            for (auto roster = m_rosterList->begin(); roster != m_rosterList->end();) {
                const auto queue = roster->msg_queue;
                if (!queue.isEmpty()) {
                    const qint32 target = queue.size();
                    for (qint32 i = 0; i < queue.size(); ++i) {
                        if (queue[i].message.isXmppStanza() && !queue[i].message.id().isEmpty() && !queue[i].message.body().isEmpty()) {
                            emit sendIncomingResults(queue[i].message); // This object does not exist in memory yet!
                            if (i + 1 == queue.size()) {
                                roster->msg_queue.clear();
                                ++roster;
                                break;
                            }
                        }
                    }

                    //
                    // Tell the program that receiving of all (applicable) messages from the archives of the given XMPP server has been
                    // accomplished and is (hopefully) successful!
                    emit msgArchiveSuccReceived();
                    return;
                } else {
                    break;
                }
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(e.what(), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppClient::updateQueues
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msg The QXmppMessage stanza itself to be inserted into the QTableView model, GekkoFyre::GkXmppRecvMsgsTableViewModel().
 * @param wipeExistingHistory Whether to wipe the pre-existing chat history or not, specifically from the given QTableView
 * object, GekkoFyre::GkXmppRecvMsgsTableViewModel().
 */
void GkXmppClient::updateQueues(const QXmppMessage &msg, const bool &wipeExistingHistory)
{
    if (!m_msgRetrievalTimestamps.empty()) {
        m_msgRetrievalTimestamps.pop();
    }

    return;
}
