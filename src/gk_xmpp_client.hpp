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
#include "src/file_io.hpp"
#include "src/gk_logger.hpp"
#include "src/models/system/gk_network_ping_model.hpp"
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <qxmpp/QXmppIq.h>
#include <qxmpp/QXmppGlobal.h>
#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppLogger.h>
#include <qxmpp/QXmppVCardIq.h>
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
#include <queue>
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
                          QPointer<GekkoFyre::FileIo> fileIo, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                          const bool &connectNow = false, QObject *parent = nullptr);
    ~GkXmppClient() override;

    void createConnectionToServer(const QString &domain_url, const quint16 &network_port, const QString &username = "",
                                  const QString &password = "", const QString &jid = "", const bool &user_signup = false);
    bool createMuc(const QString &room_name, const QString &room_subject, const QString &room_desc);

    static bool isHostnameSame(const QString &hostname, const QString &comparison = "");
    static QString getUsername(const QString &username);
    static QString getHostname(const QString &username);
    GekkoFyre::Network::GkXmpp::GkNetworkState getNetworkState() const;
    bool isJidExist(const QString &bareJid);
    bool isJidOnline(const QString &bareJid);

    //
    // User, roster and presence details
    std::shared_ptr<QXmppRegistrationManager> getRegistrationMgr();
    QVector<GekkoFyre::Network::GkXmpp::GkXmppCallsign> getRosterMap();
    QXmppPresence statusToPresence(const Network::GkXmpp::GkOnlineStatus &status);
    Network::GkXmpp::GkOnlineStatus presenceToStatus(const QXmppPresence::AvailableStatusType &xmppPresence);
    QString presenceToString(const QXmppPresence::AvailableStatusType &xmppPresence);
    QIcon presenceToIcon(const QXmppPresence::AvailableStatusType &xmppPresence);
    bool deleteUserAccount();
    QString obtainAvatarFilePath();

    //
    // vCard management
    QByteArray processImgToByteArray(const QString &filePath);

    QString getErrorCondition(const QXmppStanza::Error::Condition &condition);

public slots:
    void clientConnected();

    //
    // User, roster and presence details
    void presenceChanged(const QString &bareJid, const QString &resource);
    void modifyPresence(const QXmppPresence::Type &pres);
    std::shared_ptr<QXmppPresence> getPresence(const QString &bareJid);
    void modifyAvailableStatusType(const QXmppPresence::AvailableStatusType &stat_type);
    void acceptSubscriptionRequest(const QString &bareJid);
    void refuseSubscriptionRequest(const QString &bareJid);
    void blockUser(const QString &bareJid);
    void unblockUser(const QString &bareJid);
    void subscribeToUser(const QString &bareJid, const QString &reason = "");

    //
    // Registration management
    void handleRegistrationForm(const QXmppRegisterIq &registerIq);

    //
    // vCard management
    void updateClientVCardForm(const QString &first_name, const QString &last_name, const QString &email,
                               const QString &callsign, const QByteArray &avatar_pic);

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

    //
    // vCard management
    void vCardReceived(const QXmppVCardIq &vCard);
    void clientVCardReceived();
    void updateClientVCard(const QXmppVCardIq &vCard);

    void itemAdded(const QString &bareJid);
    void itemRemoved(const QString &bareJid);
    void itemChanged(const QString &bareJid);

    void handleSslGreeting();

signals:
    //
    // User, roster and presence details
    void setPresence(const QXmppPresence::Type &pres);
    void sendRegistrationForm(const QXmppRegisterIq &registerIq);

    void sendSubscriptionRequest(const QString &bareJid); // A subscription request was made, therefore notify client!
    void retractSubscriptionRequest(const QString &bareJid); // A subscription request was retracted, therefore delete JID!

    void addJidToRoster(const QString &bareJid); // Subscription request was successful, add new JID!
    void delJidFromRoster(const QString &bareJid); // User requested a deletion from the roster, therefore remove JID!
    void changeRosterJid(const QString &bareJid); // A change needs to be made within the roster, therefore modify JID!

    void updateRoster();

    //
    // vCard management
    void sendClientVCard(const QXmppVCardIq &vCard);
    void savedClientVCard(const QByteArray &avatar_pic);
    void sendUserVCard(const QXmppVCardIq &vCard);

    //
    // Event & Logging management
    void sendError(const QString &error);
    void sendError(const QXmppClient::Error &error);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::FileIo> gkFileIo;
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
    GekkoFyre::Network::GkXmpp::GkHost m_host;
    Network::GkXmpp::GkOnlineStatus m_status;
    std::shared_ptr<QXmppPresence> m_presence;
    std::shared_ptr<QXmppRosterManager> m_rosterManager;
    QStringList rosterGroups;
    QVector<QString> m_blockList;
    QVector<GekkoFyre::Network::GkXmpp::GkXmppCallsign> m_rosterList;

    //
    // Filesystem & Directories
    //
    boost::filesystem::path native_slash;
    boost::filesystem::path vcard_save_path;

    //
    // vCard management
    //
    QXmppVCardIq m_clientVCard;

    //
    // SSL / TLS / STARTTLS
    //
    QPointer<QSslSocket> m_sslSocket;

    //
    // Queue's relating to XMPP
    //
    std::queue<QXmppPresence::AvailableStatusType> m_availStatusTypeQueue;

    //
    // QXmpp and XMPP related
    //
    QXmppConfiguration config;
    std::shared_ptr<QXmppRegistrationManager> m_registerManager;
    std::unique_ptr<QXmppMucManager> m_mucManager;
    std::unique_ptr<QXmppMucRoom> m_pRoom;
    std::unique_ptr<QXmppTransferManager> m_transferManager;
    std::unique_ptr<QXmppVCardManager> m_vCardManager;
    QScopedPointer<QXmppLogger> m_xmppLogger;

    GekkoFyre::Network::GkXmpp::GkNetworkState m_netState;
    QString m_id;

    void initRosterMgr();

};
};
