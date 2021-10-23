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
#include "src/gk_system.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_xmpp_msg_handler.hpp"
#include "src/models/system/gk_network_ping_model.hpp"
#include <qxmpp/QXmppIq.h>
#include <qxmpp/QXmppStanza.h>
#include <qxmpp/QXmppGlobal.h>
#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppLogger.h>
#include <qxmpp/QXmppMessage.h>
#include <qxmpp/QXmppVCardIq.h>
#include <qxmpp/QXmppDataForm.h>
#include <qxmpp/QXmppPresence.h>
#include <qxmpp/QXmppResultSet.h>
#include <qxmpp/QXmppArchiveIq.h>
#include <qxmpp/QXmppVersionIq.h>
#include <qxmpp/QXmppMamManager.h>
#include <qxmpp/QXmppRegisterIq.h>
#include <qxmpp/QXmppMucManager.h>
#include <qxmpp/QXmppVCardManager.h>
#include <qxmpp/QXmppCarbonManager.h>
#include <qxmpp/QXmppRosterManager.h>
#include <qxmpp/QXmppArchiveManager.h>
#include <qxmpp/QXmppVersionManager.h>
#include <qxmpp/QXmppTransferManager.h>
#include <qxmpp/QXmppClientExtension.h>
#include <qxmpp/QXmppDiscoveryManager.h>
#include <qxmpp/QXmppRegistrationManager.h>
#include <mutex>
#include <queue>
#include <future>
#include <thread>
#include <memory>
#include <utility>
#include <QDir>
#include <QMap>
#include <QUrl>
#include <QList>
#include <QTimer>
#include <QString>
#include <QThread>
#include <QObject>
#include <QPointer>
#include <QSslError>
#include <QFileInfo>
#include <QDateTime>
#include <QByteArray>
#include <QSslSocket>
#include <QDnsLookup>
#include <QDomElement>
#include <QStringList>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDnsServiceRecord>

namespace GekkoFyre {

class GkXmppClient : public QXmppClient {
    Q_OBJECT

public:
    explicit GkXmppClient(const Network::GkXmpp::GkUserConn &connection_details, QPointer<GekkoFyre::GkLevelDb> database,
                          QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::FileIo> fileIo,
                          QPointer<GekkoFyre::GkSystem> system, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                          const bool &connectNow = false, QObject *parent = nullptr);
    ~GkXmppClient() override;

    void createConnectionToServer(const QString &domain_url, const quint16 &network_port, const QString &password = "",
                                  const QString &jid = "", const bool &user_signup = false);
    void killConnectionFromServer(const bool &askReconnectPolicy = false);
    bool createMuc(const QString &room_name, const QString &room_subject, const QString &room_desc);

    [[nodiscard]] static bool isHostnameSame(const QString &hostname, const QString &comparison = "");
    [[nodiscard]] static QString getUsername(const QString &username);
    [[nodiscard]] static QString getHostname(const QString &username);
    [[nodiscard]] QXmppPresence getBareJidPresence(const QString &bareJid, const QString &resource = "");
    [[nodiscard]] QString getJidNickname(const QString &bareJid);
    QString addHostname(const QString &username);
    [[nodiscard]] GekkoFyre::Network::GkXmpp::GkNetworkState getNetworkState() const;
    [[nodiscard]] bool isJidExist(const QString &bareJid);
    [[nodiscard]] bool isJidOnline(const QString &bareJid);
    [[nodiscard]] bool isAvatarImgScaleOk(const QByteArray &avatar_img, const QString &img_type);

    //
    // Date & Time Management
    [[nodiscard]] QDateTime calcMinTimestampForXmppMsgHistory(const QString &bareJid, const QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign> &msg_history);
    [[nodiscard]] QDateTime calcMaxTimestampForXmppMsgHistory(const QString &bareJid, const QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign> &msg_history);
    [[nodiscard]] qint64 compareTimestamps(const std::vector<GekkoFyre::Network::GkXmpp::GkRecvMsgsTableViewModel> &data, const qint64 &value);

    //
    // User, roster and presence details
    [[nodiscard]] std::shared_ptr<QXmppRegistrationManager> getRegistrationMgr();
    [[nodiscard]] QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign> getRosterMap();
    void updateRosterMap(const QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign> &rosterList);
    [[nodiscard]] QXmppPresence statusToPresence(const Network::GkXmpp::GkOnlineStatus &status);
    [[nodiscard]] Network::GkXmpp::GkOnlineStatus presenceToStatus(const QXmppPresence::AvailableStatusType &xmppPresence);
    [[nodiscard]] QString presenceToString(const QXmppPresence::AvailableStatusType &xmppPresence);
    [[nodiscard]] QIcon presenceToIcon(const QXmppPresence::AvailableStatusType &xmppPresence);
    bool deleteUserAccount();
    [[nodiscard]] QString obtainAvatarFilePath(const QString &set_dir_path = "");

    //
    // Message and QXmppMamManager handling
    [[nodiscard]] bool getMsgRecved() const;

    //
    // vCard management
    [[nodiscard]] QByteArray processImgToByteArray(const QFileInfo &filePath);
    [[nodiscard]] QPixmap rescaleAvatarImg(const QByteArray &avatar_img, const QString &img_type);

    [[nodiscard]] QString getErrorCondition(const QXmppStanza::Error::Condition &condition);

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
    void unsubscribeToUser(const QString &bareJid, const QString &reason = "");

    //
    // Registration management
    void handleRegistrationForm(const QXmppRegisterIq &registerIq);

    //
    // vCard management
    void updateClientVCardForm(const QString &first_name, const QString &last_name, const QString &email,
                               const QString &callsign, const QByteArray &avatar_pic, const QString &img_type);

    //
    // QXmppMamManager handling
    void getArchivedMessagesBulk(const QString &from = QString());

    //
    // Message handling
    void sendXmppMsg(const QXmppMessage &msg);

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
    void notifyNewSubscription(const QString &bareJid, const QXmppPresence &presence);
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

    //
    // Message handling and QXmppArchiveManager-related
    void recvXmppMsgUpdate(const QXmppMessage &message);
    void archiveListReceived(const QList<QXmppArchiveChat> &chats, const QXmppResultSetReply &rsmReply);
    void archiveChatReceived(const QXmppArchiveChat &chat, const QXmppResultSetReply &rsmReply);
    void handleFirstPartyMsg(const QXmppMessage &message, const bool &enqueue = true);
    void handleThirdPartyMsg(const QXmppMessage &message, const bool &enqueue = true);
    void filterIncomingResults(QXmppMessage message);
    void insertArchiveMessage(const QXmppMessage &message, const bool &enqueue = true);

    //
    // QXmppMamManager handling
    void archivedMessageReceived(const QString &queryId, const QXmppMessage &message);
    void resultsReceived(const QString &queryId, const QXmppResultSetReply &resultSetReply, bool complete);
    void setMsgRecved(const bool &setValid);

signals:
    //
    // User, roster and presence details
    void setPresence(const QXmppPresence::Type &pres);
    void sendRegistrationForm(const QXmppRegisterIq &registerIq);

    void sendSubscriptionRequest(const QString &bareJid);                        // A subscription request was made without a reason, therefore notify the client!
    void sendSubscriptionRequest(const QString &bareJid, const QString &reason); // A subscription request was made with a reason, therefore notify the client!
    void retractSubscriptionRequest(const QString &bareJid);                     // A subscription request was retracted, therefore delete JID!

    void addJidToRoster(const QString &bareJid); // Subscription request was successful, add new JID!
    void delJidFromRoster(const QString &bareJid); // User requested a deletion from the roster, therefore remove JID!
    void changeRosterJid(const QString &bareJid); // A change needs to be made within the roster, therefore modify JID!

    void updateRoster();
    void updateProgressBar(const qint32 &percentage); // Progress bar for monitoring connection status towards a given XMPP server!

    //
    // vCard management
    void sendClientVCard(const QXmppVCardIq &vCard);
    void savedClientVCard(const QByteArray &avatar_pic, const QString &img_type);
    void sendUserVCard(const QXmppVCardIq &vCard);

    //
    // Event & Logging management
    void sendError(const QString &error);
    void sendError(const QXmppClient::Error &error);
    void connecting();

    //
    // Message handling and QXmppArchiveManager-related
    void xmppMsgUpdate(const QXmppMessage &message);
    void updateMsgHistory();
    void sendIncomingResults(QXmppMessage message);

    //
    // Message handling and QXmppMamManager handling
    void msgArchiveSuccReceived();
    void procXmppMsg(const QXmppMessage &msg, const bool &wipeExistingHistory = false);
    void msgRecved(const bool &setValid);
    void procFirstPartyMsg(const QXmppMessage &message, const bool &enqueue = true);
    void procThirdPartyMsg(const QXmppMessage &message, const bool &enqueue = true);
    void sendXmppMsgToArchive(const QXmppMessage &message, const bool &enqueue = true);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::GkSystem> gkSystem;
    QPointer<GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
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
    QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign> m_rosterList;   // A list of all the bareJids, including the client themselves!

    //
    // Filesystem & Directories
    //
    QDir vcard_save_path;

    //
    // SSL / TLS / STARTTLS
    //
    QPointer<QSslSocket> m_sslSocket;

    //
    // Queue's relating to XMPP
    //
    std::queue<QXmppPresence::AvailableStatusType> m_availStatusTypeQueue;

    //
    // Message handling
    void getArchivedMessagesFine(qint32 recursion, const QString &from = QString(), const QDateTime &preset_time = {});
    bool m_msgRecved;

    //
    // Multithreading, mutexes, etc.
    //
    std::mutex m_updateRosterMapMtx;
    std::mutex m_archivedMsgsFineMtx;
    std::thread m_archivedMsgsFineThread;

    //
    // QXmpp and XMPP related
    //
    QXmppConfiguration config;
    bool m_askToReconnectAuto;
    bool m_sslIsEnabled;
    bool m_isMuc;
    std::shared_ptr<QXmppRegistrationManager> m_registerManager;
    std::unique_ptr<QXmppMucManager> m_mucManager;
    std::unique_ptr<QXmppMucRoom> m_pRoom;
    std::unique_ptr<QXmppArchiveManager> m_xmppArchiveMgr;
    std::unique_ptr<QXmppMamManager> m_xmppMamMgr;
    std::unique_ptr<QXmppTransferManager> m_transferManager;
    std::unique_ptr<QXmppVCardManager> m_vCardManager;
    std::unique_ptr<QXmppCarbonManager> m_xmppCarbonMgr;
    QScopedPointer<QXmppLogger> m_xmppLogger;

    GekkoFyre::Network::GkXmpp::GkNetworkState m_netState;
    QString m_id;

};
};
