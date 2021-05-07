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
#include "src/gk_xmpp_client.hpp"
#include "src/models/tableview/gk_xmpp_roster_presence_model.hpp"
#include "src/models/tableview/gk_xmpp_roster_pending_model.hpp"
#include "src/models/tableview/gk_xmpp_roster_blocked_model.hpp"
#include "src/ui/xmpp/gkxmppmessagedialog.hpp"
#include "src/gk_logger.hpp"
#include <memory>
#include <QImage>
#include <QTimer>
#include <QVector>
#include <QAction>
#include <QString>
#include <QObject>
#include <QDialog>
#include <QPointer>
#include <QByteArray>
#include <QTreeWidgetItem>

namespace Ui {
class GkXmppRosterDialog;
}

class GkXmppRosterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkXmppRosterDialog(const GekkoFyre::Network::GkXmpp::GkUserConn &connection_details, QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                QPointer<GekkoFyre::GkLevelDb> database, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                const bool &skipConnectionCheck = false, QWidget *parent = nullptr);
    ~GkXmppRosterDialog();

private slots:
    void on_comboBox_current_status_currentIndexChanged(int index);
    void on_pushButton_user_login_clicked();
    void on_pushButton_user_create_account_clicked();
    void on_tableView_callsigns_groups_customContextMenuRequested(const QPoint &pos);
    void on_actionAdd_Contact_triggered();
    void on_actionEdit_Contact_triggered();
    void on_actionDelete_Contact_triggered();
    void on_pushButton_self_avatar_clicked();
    void on_lineEdit_self_nickname_returnPressed();
    void on_tableView_callsigns_blocked_customContextMenuRequested(const QPoint &pos);

    //
    // VCard management
    //
    void recvClientAvatarImg(const QByteArray &avatar_pic);
    void defaultClientAvatarPlaceholder();
    void updateClientAvatar(const QImage &avatar_img);
    void updateUserVCard(const QXmppVCardIq &vCard);
    void editNicknameLabel(const QString &value);

    //
    // XMPP Roster management and related
    //
    void subscriptionRequestRecv(const QString &bareJid);
    void subscriptionRequestRetracted(const QString &bareJid);
    void addJidToRoster(const QString &bareJid); // Subscription request was successful, add new JID!
    void delJidFromRoster(const QString &bareJid); // User requested a deletion from the roster, therefore remove JID!
    void changeRosterJid(const QString &bareJid); // A change needs to be made within the roster, therefore modify JID!
    void on_label_self_nickname_customContextMenuRequested(const QPoint &pos);
    void on_pushButton_add_contact_cancel_clicked();
    void on_pushButton_add_contact_submit_clicked();
    void on_actionAcceptInvite_triggered();
    void on_actionRefuseInvite_triggered();
    void on_actionBlockUser_triggered();
    void on_actionUnblockUser_triggered();
    void on_actionEdit_Nickname_triggered();
    void on_lineEdit_edit_nickname_returnPressed();
    void on_lineEdit_edit_nickname_inputRejected();
    void on_tableView_callsigns_pending_clicked(const QModelIndex &index);
    void on_tableView_callsigns_pending_doubleClicked(const QModelIndex &index);
    void on_tableView_callsigns_groups_clicked(const QModelIndex &index);
    void on_tableView_callsigns_groups_doubleClicked(const QModelIndex &index);
    void on_tableView_callsigns_blocked_clicked(const QModelIndex &index);
    void on_tableView_callsigns_blocked_doubleClicked(const QModelIndex &index);

signals:
    void updateAvailableStatusType(const QXmppPresence::AvailableStatusType &stat_type);
    void updateClientVCard(const QString &first_name, const QString &last_name, const QString &email,
                           const QString &callsign, const QByteArray &avatar_pic);
    void updateClientAvatarImg(const QImage &avatar_img);

    void acceptSubscription(const QString &bareJid);
    void refuseSubscription(const QString &bareJid);
    void blockUser(const QString &bareJid);
    void unblockUser(const QString &bareJid);

private:
    Ui::GkXmppRosterDialog *ui;

    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkXmppClient> m_xmppClient;
    QPointer<GkXmppMessageDialog> gkXmppMsgDlg;
    bool shownXmppPreviewNotice;

    //
    // QTableView and related
    //
    QPointer<GekkoFyre::GkXmppRosterPresenceTableViewModel> gkXmppPresenceTableViewModel;
    QPointer<GekkoFyre::GkXmppRosterPendingTableViewModel> gkXmppPendingTableViewModel;
    QPointer<GekkoFyre::GkXmppRosterBlockedTableViewModel> gkXmppBlockedTableViewModel;
    QVector<GekkoFyre::Network::GkXmpp::GkPresenceTableViewModel> m_presenceRosterData;
    QVector<GekkoFyre::Network::GkXmpp::GkPendingTableViewModel> m_pendingRosterData;
    QVector<GekkoFyre::Network::GkXmpp::GkBlockedTableViewModel> m_blockedRosterData;

    void insertRosterPresenceTable(const QIcon &presence, const QString &bareJid, const QString &nickname);
    void removeRosterPresenceTable(const QString &bareJid);
    void updateRoster();
    void updateActions();

    //
    // QXmpp and XMPP related
    //
    GekkoFyre::Network::GkXmpp::GkUserConn gkConnDetails;
    QVector<GekkoFyre::Network::GkXmpp::GkXmppCallsign> m_rosterList;

    //
    // VCard management
    //
    QByteArray m_clientAvatarImg;

    void reconnectToXmpp();
    void launchMsgDlg(const QString &bareJid);
    void prefillAvailComboBox();
};

