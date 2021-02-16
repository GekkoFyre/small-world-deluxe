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
#include <qxmpp/QXmppVCardIq.h>
#include <iostream>
#include <exception>
#include <QMessageBox>

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

/**
 * @brief GkXmppVcardCache::grabVCard
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @return
 */
GkXmppVcardData GkXmppVcardCache::grabVCard(const QString &bareJid)
{
    QScopedPointer<QDomDocument> vcXmlDoc(new QDomDocument());
    QByteArray vcByteArray;
    GkXmppVcardData data;

    vcXmlDoc->setContent(vcByteArray);

    data.nickname = getElementStore(vcXmlDoc, "nickname");
    data.fullName = getElementStore(vcXmlDoc, "fullName");
    data.firstName = getElementStore(vcXmlDoc, "firstName");
    data.middleName = getElementStore(vcXmlDoc, "middleName");
    data.lastName = getElementStore(vcXmlDoc, "lastName");
    data.webUrl = getElementStore(vcXmlDoc, "webUrl");
    data.email = getElementStore(vcXmlDoc, "email");

    return data;
}

/**
 * @brief GkXmppVcardCache::getElementStore
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param doc
 * @param nodeName
 * @return
 */
QString GkXmppVcardCache::getElementStore(const QScopedPointer<QDomDocument> &doc, const QString &nodeName)
{
    QString val = "";

    QDomNode nodeElement = doc->elementsByTagName(nodeName).item(0);
    QDomNode te = nodeElement.firstChild();

    if (!te.isNull()) {
        val = te.nodeValue();
    }

    return val;
}

/**
 * @brief GkXmppClient::GkXmppClient The client-class for all such XMPP calls within Small World Deluxe.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param connection_details The details pertaining to making a successful connection towards the given XMPP server.
 * @param eventLogger The object for processing logging information.
 * @param connectNow Whether to intiiate a connection now or at a later time instead.
 * @param parent The parent object.
 */
GkXmppClient::GkXmppClient(const GkUserConn &connection_details, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                           const bool &connectNow, QObject *parent) : m_registerManager(findExtension<QXmppRegistrationManager>()),
                                                                      m_rosterManager(findExtension<QXmppRosterManager>()),
                                                                      m_vcardMgr(findExtension<QXmppVCardManager>()),
                                                                      m_versionMgr(findExtension<QXmppVersionManager>()),
                                                                      m_discoMgr(findExtension<QXmppDiscoveryManager>()),
                                                                      QXmppClient(parent)
{
    try {
        setParent(parent);
        gkConnDetails = connection_details;
        gkEventLogger = std::move(eventLogger);
        m_sslSocket = new QSslSocket(this);

        addExtension(m_rosterManager.get());
        addExtension(m_vcardMgr.get());
        addExtension(m_versionMgr.get());
        addExtension(m_discoMgr.get());

        QObject::connect(this, SIGNAL(sendError(const QString &)), this, SLOT(handleError(const QString &)));

        //
        // This signal is emitted when the client state changes...
        QObject::connect(this, SIGNAL(stateChanged(QXmppClient::State)), this, SLOT(stateChanged(QXmppClient::State)));

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

        m_status = GkOnlineStatus::Online;
        m_keepalive = 60;

        m_vcardCache = std::make_unique<GkXmppVcardCache>(parent);
        m_presence = std::make_unique<QXmppPresence>();

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
        // Find the XMPP servers as defined by either the user themselves or GekkoFyre Networks...
        QString dns_lookup_str;
        switch (gkConnDetails.server.type) {
            case GkServerType::GekkoFyre:
                //
                // Settings for GekkoFyre Networks' server have been specified!
                break;
            case GkServerType::Custom:
                //
                // Settings for a custom server have been specified!
                if (gkConnDetails.server.settings_client.uri_lookup_method == GkUriLookupMethod::QtDnsSrv) {
                    m_dns = new QDnsLookup(this); // TODO: Finish implementing this!

                    //
                    // Setup the signals for the DNS object
                    QObject::connect(m_dns, SIGNAL(finished()), this, SLOT(handleServers()));

                    m_dns->setType(QDnsLookup::SRV);
                    dns_lookup_str = QString("_xmpp-client._tcp.%1").arg(gkConnDetails.server.url);
                    m_dns->setName(dns_lookup_str);
                    m_dns->lookup();
                }

                break;
            case GkServerType::Unknown:
                throw std::invalid_argument(tr("Unable to perform DNS lookup for XMPP; has a server been specified?").toStdString());
            default:
                throw std::invalid_argument(tr("Unable to perform DNS lookup for XMPP; has a server been specified?").toStdString());
        }

        //
        // Setup logging...
        QXmppLogger *logger = QXmppLogger::getLogger();
        logger->setLoggingType(QXmppLogger::SignalLogging);
        QObject::connect(logger, SIGNAL(message(QXmppLogger::MessageType, const QString &)),
                         gkEventLogger, SLOT(recvXmppLog(QXmppLogger::MessageType, const QString &)));

        if (gkConnDetails.server.settings_client.auto_connect || connectNow) {
            createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, gkConnDetails.username,
                                     gkConnDetails.password);
        }

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
                m_discoMgr->requestInfo(gkConnDetails.server.url);
            }

            m_registerManager = std::make_shared<QXmppRegistrationManager>();
            m_mucManager = std::make_unique<QXmppMucManager>();
            m_transferManager = std::make_unique<QXmppTransferManager>();

            m_registerManager.reset(findExtension<QXmppRegistrationManager>());
            m_mucManager.reset(findExtension<QXmppMucManager>());
            m_transferManager.reset(findExtension<QXmppTransferManager>());

            if (m_registerManager) {
                addExtension(m_registerManager.get());
            }

            if (m_mucManager) {
                addExtension(m_mucManager.get());
            }

            if (m_transferManager) {
                addExtension(m_transferManager.get());
            }

            QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::registrationFailed, this, [=](const QXmppStanza::Error &error) {
                emit sendError(tr("Requesting the XMPP registration form failed: %1").arg(error.text()));
            });

            QObject::connect(m_registerManager.get(), SIGNAL(registrationFormReceived(const QXmppRegisterIq &)),
                             this, SLOT(handleRegistrationForm(const QXmppRegisterIq &)));
        });
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An issue has occurred within the XMPP subsystem. Error:\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return;
}

GkXmppClient::~GkXmppClient()
{
    if (isConnected()) {
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
        if (isConnected() && !room_name.isEmpty()) {
            QString room_jid = QString("%1@conference.%2").arg(room_name).arg(gkConnDetails.server.url);
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
        } else {
            throw std::runtime_error(tr("Are you connected to an XMPP server?").toStdString());
        }
    } catch (const std::exception &e) {
        emit sendError(tr("An issue was encountered while creating a MUC! Error:\n\n%1").arg(QString::fromStdString(e.what())));
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
 * @brief GkXmppClient::getHostname
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param username
 * @return
 */
QString GkXmppClient::getHostname(const QString &username)
{
    auto domain = username.split('@', Qt::SkipEmptyParts);
    if (!domain.empty()) {
        return domain.last();
    }

    return QString();
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
        case GkXmpp::Offline:
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
 * @brief GkXmppClient::clientConnected
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::clientConnected()
{
    gkEventLogger->publishEvent(tr("A connection has been successfully made towards XMPP server: %1").arg(gkConnDetails.server.url),
                                GkSeverity::Info, "", true, true, true, false);
    return;
}

/**
 * @brief GkXmppClient::handleRosterReceived
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::handleRosterReceived()
{
    try {
        const QStringList bareJids = m_rosterManager->getRosterBareJids();
        for (const auto &bareJid: bareJids) {
            // Get the contact name, as we might need it...
            QString name = m_rosterManager->getRosterEntry(bareJid).name(); //-V808

            // Attempt to get the vCard...
            GkXmppVcardData vcData = m_vcardCache->grabVCard(bareJid);
            if (vcData.isEmpty()) {
                gkEventLogger->publishEvent(tr("User, %1, has no VCard. Requesting...").arg(bareJid), GkSeverity::Info, "", false, true, false, false);
                if (m_vcardMgr) {
                    m_vcardMgr->requestVCard(bareJid);
                }
            }

            // Prepare groups...
            QStringList groups_roster = m_rosterManager->getRosterEntry(bareJid).groups().values();

            // Iterate through the group list and add them to a cache!
            for (const auto &user: groups_roster) {
                if (!rosterGroups.contains(user)) {
                    rosterGroups.push_back(user);
                }
            }

            // TODO: Do something with this!
            // qint32 subType = (qint32)m_rosterManager->getRosterEntry(bareJid).subscriptionType();
        }
    } catch (const std::exception &e) {
        emit sendError(tr("An issue has been encountered while managing the XMPP roster! Error:\n\n%1")
        .arg(QString::fromStdString(e.what())));
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
            gkEventLogger->publishEvent(tr("Disconnected from XMPP server: %1").arg(gkConnDetails.server.url), GkSeverity::Info, "",
                                        true, true, true, false);
            return;
        case QXmppClient::State::ConnectingState:
            gkEventLogger->publishEvent(tr("...attempting to make connection towards XMPP server: %1").arg(gkConnDetails.server.url), GkSeverity::Info, "",
                                        true, true, true, false);
            return;
        case QXmppClient::State::ConnectedState:
            gkEventLogger->publishEvent(tr("Connected to XMPP server: %1").arg(gkConnDetails.server.url), GkSeverity::Info, "",
                                        true, true, true, false);
            return;
        default:
            break;
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
 * @brief GkXmppClient::handleRegistrationForm
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param registerIq The user registration form to be filled out, as received from the connected towards XMPP server.
 */
void GkXmppClient::handleRegistrationForm(const QXmppRegisterIq &registerIq)
{
    try {
        qDebug() << "XMPP registration form received: " << registerIq.instructions();
        if (registerIq.type() == QXmppIq::Type::Error) {
            switch (registerIq.error().condition()) {
                case QXmppStanza::Error::Conflict:
                    throw std::runtime_error(tr("A user has already been previously registered with this username for server: %1")
                    .arg(gkConnDetails.server.url).toStdString());
                case QXmppStanza::Error::RegistrationRequired:
                {
                    //
                    // The form now needs to be completed!
                    QXmppRegisterIq registration_form;
                    if (m_registerManager->registerOnConnectEnabled()) { // If not handled here, process the form otherwise in `GkXmppRegistrationDialog`!
                        if (!isConnected()) {
                            createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, gkConnDetails.username,
                                                     gkConnDetails.password);
                        }

                        registration_form.setForm(registerIq.form());
                        registration_form.setInstructions(registerIq.instructions());
                        registration_form.setEmail(gkConnDetails.email);
                        registration_form.setPassword(config.password());
                        registration_form.setUsername(config.user());
                        registration_form.setType(QXmppIq::Type::Set);

                        m_registerManager->setRegistrationFormToSend(registration_form);
                        m_registerManager->sendCachedRegistrationForm();

                        gkEventLogger->publishEvent(tr("User, \"%1\", has been registered with XMPP server:\n\n%2")
                                                            .arg(config.user()).arg(gkConnDetails.server.url), GkSeverity::Info, "", true,
                                                    true, false, false);
                    }
                }

                    break;
                default:
                    throw std::invalid_argument(tr("An unknown error has occurred during user registration with connected XMPP server: %1")
                                                        .arg(gkConnDetails.server.url).toStdString());
            }
        } else if (registerIq.type() == QXmppIq::Type::Result) {
            handleSuccess();
        } else {
            throw std::runtime_error(tr("An error has occurred during user registration with connected XMPP server: %1\n\nreceived unwanted stanza type %2")
                                             .arg(gkConnDetails.server.url).arg(QString::number(registerIq.type())).toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("Issues were encountered with trying to register user with XMPP server! Error:\n\n%1")
        .arg(e.what()), GkSeverity::Fatal, "", false, true, false, true);
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
            break;
        case QDnsLookup::ResolverError:
            gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\". There has been a Resolver Error.").arg(gkConnDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::OperationCancelledError:
            gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\". The operation was cancelled abruptly.").arg(gkConnDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::InvalidRequestError:
            gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\". There has been an Invalid Request.").arg(gkConnDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::InvalidReplyError:
            gkEventLogger->publishEvent(tr("DNS lookup failed for, \"%1\". There has been an Invalid Reply.").arg(gkConnDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::ServerFailureError:
            gkEventLogger->publishEvent(tr(R"(DNS lookup failed for, "%1". There has been a "Server Failure" error on the other end.)").arg(gkConnDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::ServerRefusedError:
            gkEventLogger->publishEvent(tr(R"(DNS lookup failed for, "%1". There has been a "Server Refused" error on the other end.)").arg(gkConnDetails.server.url), GkSeverity::Error, "",
                                        false, true, false, true);

            return;
        case QDnsLookup::NotFoundError:
            gkEventLogger->publishEvent(tr(R"(DNS lookup failed for, "%1". There has been a "Not Found" error.)").arg(gkConnDetails.server.url), GkSeverity::Error, "",
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
            emit sendError(tr("XMPP error encountered due to TCP socket. Error:\n\n%1").arg(this->socketErrorString()));
            break;
        case QXmppClient::Error::KeepAliveError:
            emit sendError(tr("XMPP error encountered due to no response from a keep alive."));
            break;
        case QXmppClient::Error::XmppStreamError:
            emit sendError(tr("XMPP error encountered due to XML stream. Error:\n\n%1").arg(getErrorCondition(this->xmppStreamError())));
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
        if (isConnected()) {
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
 */
void GkXmppClient::createConnectionToServer(const QString &domain_url, const quint16 &network_port, const QString &username,
                                            const QString &password, const bool &user_signup)
{
    try {
        if (domain_url.isEmpty()) {
            throw std::invalid_argument(tr("An invalid XMPP Server URL has been provided! Please check your settings and try again.").toStdString());
        }

        if (network_port < 80) {
            throw std::invalid_argument(tr("An invalid XMPP Server network port has been provided! It cannot be less than 80/TCP (HTTP)!").toStdString());
        }

        if (user_signup) { // Override...
            //
            // Allow the registration of a new user to proceed!
            // Sets whether to only request the registration form and not to connect with username/password.
            //
            m_registerManager->setRegisterOnConnectEnabled(true); // https://doc.qxmpp.org/qxmpp-1/classQXmppRegistrationManager.html
        }

        //
        // Neither username nor the password can be nullptr, both have to be available
        // to be of any use!
        if (username.isEmpty() || password.isEmpty()) {
            m_presence = nullptr;
        } else {
            config.setJid(username);
            config.setPassword(password);
        }

        QObject::connect(this, &QXmppClient::presenceReceived, this, [=](const QXmppPresence &presence) {
            gkEventLogger->publishEvent(presence.statusText(), GkSeverity::Info, "", true, true, false, false);
        });

        // You only need to provide a domain to connectToServer()...
        config.setAutoAcceptSubscriptions(false);
        config.setAutoReconnectionEnabled(gkConnDetails.server.settings_client.auto_reconnect);
        config.setStreamSecurityMode(QXmppConfiguration::StreamSecurityMode::TLSDisabled);
        if (gkConnDetails.server.settings_client.enable_ssl && QSslSocket::supportsSsl()) { // https://xmpp.org/extensions/xep-0035.html
            config.setStreamSecurityMode(QXmppConfiguration::StreamSecurityMode::TLSEnabled);
            qDebug() << tr("SSL version for build: ") << QSslSocket::sslLibraryBuildVersionString();
            qDebug() << tr("SSL version for run-time: ") << QSslSocket::sslLibraryVersionNumber();
        }

        config.setIgnoreSslErrors(false); // Do NOT ignore SSL warnings!
        if (gkConnDetails.server.settings_client.ignore_ssl_errors) {
            // We have been instructed to IGNORE any and all SSL warnings!
            config.setIgnoreSslErrors(true);
        }

        config.setResource(General::companyNameMin);
        config.setDomain(domain_url);
        config.setHost(domain_url);
        config.setPort(network_port);
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
 * @brief GkXmppClient::initRosterMgr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppClient::initRosterMgr()
{
    if (isConnected()) {
        if ((config.user().isEmpty() || config.password().isEmpty()) || gkConnDetails.email.isEmpty()) {
            // Unable to signup!
            return;
        } else {
            QObject::connect(m_rosterManager.get(), SIGNAL(presenceChanged(const QString &, const QString &)), this, SLOT(presenceChanged(const QString &, const QString &)), Qt::UniqueConnection);
            QObject::connect(m_rosterManager.get(), SIGNAL(rosterReceived()), this, SLOT(handleRosterReceived()), Qt::UniqueConnection);
            QObject::connect(m_rosterManager.get(), SIGNAL(subscriptionReceived(const QString &)), this, SLOT(notifyNewSubscription(const QString &)), Qt::UniqueConnection);
            QObject::connect(m_rosterManager.get(), SIGNAL(itemAdded(const QString &)), this, SLOT(itemAdded(const QString &)), Qt::UniqueConnection);
            QObject::connect(m_rosterManager.get(), SIGNAL(itemRemoved(const QString &)), this, SLOT(itemRemoved(const QString &)), Qt::UniqueConnection);
            QObject::connect(m_rosterManager.get(), SIGNAL(itemChanged(const QString &)), this, SLOT(itemChanged(const QString &)), Qt::UniqueConnection);
        }
    }

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
        gkEventLogger->publishEvent(tr("%1 server version: %2").arg(gkConnDetails.server.url).arg(version_str), GkSeverity::Info, "", false, true, false, false);
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
    return;
}

/**
 * @brief GkXmppClient::itemAdded
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::itemAdded(const QString &bareJid)
{
    return;
}

/**
 * @brief GkXmppClient::itemRemoved
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::itemRemoved(const QString &bareJid)
{
    return;
}

/**
 * @brief GkXmppClient::itemChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppClient::itemChanged(const QString &bareJid)
{
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
            return tr("JID malformed.");
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
