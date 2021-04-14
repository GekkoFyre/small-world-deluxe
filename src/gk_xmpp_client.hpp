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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include "src/models/system/gk_network_ping_model.hpp"
#include <qxmpp/QXmppIq.h>
#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppLogger.h>
#include <qxmpp/QXmppDataForm.h>
#include <qxmpp/QXmppPresence.h>
#include <qxmpp/QXmppVersionIq.h>
#include <qxmpp/QXmppRegisterIq.h>
#include <qxmpp/QXmppMucManager.h>
#include <qxmpp/QXmppVCardManager.h>
#include <qxmpp/QXmppRosterManager.h>
#include <qxmpp/QXmppVersionManager.h>
#include <qxmpp/QXmppTransferManager.h>
#include <qxmpp/QXmppClientExtension.h>
#include <qxmpp/QXmppDiscoveryManager.h>
#include <qxmpp/QXmppRegistrationManager.h>
#include <memory>
#include <utility>
#include <QMap>
#include <QUrl>
#include <QList>
#include <QTimer>
#include <QString>
#include <QObject>
#include <QPointer>
#include <QSslError>
#include <QByteArray>
#include <QSslSocket>
#include <QDnsLookup>
#include <QDomElement>
#include <QStringList>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QCoreApplication>
#include <QDnsServiceRecord>

namespace GekkoFyre {

class GkXmppClient : public QXmppClient {
    Q_OBJECT

public:
    explicit GkXmppClient(const Network::GkXmpp::GkUserConn &connection_details, QPointer<GekkoFyre::GkLevelDb> database,
                          QPointer<GekkoFyre::GkEventLogger> eventLogger, const bool &connectNow = false,
                          QObject *parent = nullptr);
    ~GkXmppClient() override;

    void createConnectionToServer(const QString &domain_url, const quint16 &network_port, const QString &username = "",
                                  const QString &password = "", const QString &jid = "", const bool &user_signup = false);
    bool createMuc(const QString &room_name, const QString &room_subject, const QString &room_desc);

    static bool isHostnameSame(const QString &hostname, const QString &comparison = "");
    static QString getUsername(const QString &username);
    static QString getHostname(const QString &username);
    GekkoFyre::Network::GkXmpp::GkNetworkState getNetworkState() const;

    //
    // User, roster and presence details
    std::shared_ptr<QXmppRegistrationManager> getRegistrationMgr();
    QXmppPresence statusToPresence(const Network::GkXmpp::GkOnlineStatus &status);
    bool deleteUserAccount();

    QString getErrorCondition(const QXmppStanza::Error::Condition &condition);

public slots:
    void clientConnected();
    void stateChanged(QXmppClient::State state);

    //
    // User, roster and presence details
    void presenceChanged(const QString &bareJid, const QString &resource);
    void modifyPresence(const QXmppPresence::Type &pres);

    //
    // Registration management
    void handleRegistrationForm(const QXmppRegisterIq &registerIq);

private slots:
    //
    // Event & Logging management
    void handleServers();
    void handleSuccess();

    void handleError(QXmppClient::Error errorMsg);
    void handleError(const QString &errorMsg);
    void handleSslErrors(const QList<QSslError> &errorMsg);
    void handleSocketError(QAbstractSocket::SocketError errorMsg);

    void recvXmppLog(QXmppLogger::MessageType msgType, const QString &msg);
    void versionReceivedSlot(const QXmppVersionIq &version);

    //
    // User, roster and presence details
    void notifyNewSubscription(const QString &bareJid);
    void handleRosterReceived();

    void itemAdded(const QString &bareJid);
    void itemRemoved(const QString &bareJid);
    void itemChanged(const QString &bareJid);

    void handleSslGreeting();

signals:
    //
    // User, roster and presence details
    void setPresence(const QXmppPresence::Type &pres);
    void sendRegistrationForm(const QXmppRegisterIq &registerIq);

    //
    // Event & Logging management
    void sendError(const QString &error);
    void sendError(const QXmppClient::Error &error);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GkEventLogger> gkEventLogger;
    QPointer<GkNetworkPingModel> gkNetworkPing;
    QList<QDnsServiceRecord> m_dnsRecords;

    //
    // Connection details and related variables
    //
    Network::GkXmpp::GkUserConn m_connDetails;
    QPointer<QDnsLookup> m_dns;
    qint32 m_keepalive;
    std::unique_ptr<QXmppDiscoveryManager> m_discoMgr;
    std::unique_ptr<QXmppVersionManager> m_versionMgr;

    //
    // Timers and Event Loops
    //
    std::unique_ptr<QElapsedTimer> m_dnsKeepAlive;

    //
    // User, roster and presence details
    //
    Network::GkXmpp::GkOnlineStatus m_status;
    std::unique_ptr<QXmppPresence> m_presence;
    std::shared_ptr<QXmppRosterManager> m_rosterManager;
    QStringList rosterGroups;

    //
    // SSL / TLS / STARTTLS
    //
    QPointer<QSslSocket> m_sslSocket;

    //
    // QXmpp and XMPP related
    //
    QXmppConfiguration config;
    std::shared_ptr<QXmppRegistrationManager> m_registerManager;
    std::unique_ptr<QXmppMucManager> m_mucManager;
    std::unique_ptr<QXmppMucRoom> m_pRoom;
    std::unique_ptr<QXmppTransferManager> m_transferManager;
    QScopedPointer<QXmppLogger> m_xmppLogger;

    GekkoFyre::Network::GkXmpp::GkNetworkState m_netState;
    QString m_id;

    void initRosterMgr();

};
};
