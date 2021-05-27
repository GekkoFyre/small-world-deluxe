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
#include "src/gk_string_funcs.hpp"
#include "src/models/tableview/gk_xmpp_recv_msgs_model.hpp"
#include <QtSpell.hpp>
#include <qxmpp/QXmppMessage.h>
#include <queue>
#include <memory>
#include <QEvent>
#include <QString>
#include <QObject>
#include <QDialog>
#include <QPointer>
#include <QDateTime>
#include <QStringList>

namespace Ui {
class GkXmppMessageDialog;
}

class GkPlainTextKeyEnter : public QObject {
    Q_OBJECT

protected:
    bool eventFilter(QObject *obj, QEvent *event);

signals:
    void submitMsgEnterKey();

};

class GkXmppMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkXmppMessageDialog(QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                 QPointer<GekkoFyre::GkLevelDb> database, QPointer<QtSpell::TextEditChecker> spellChecking,
                                 const GekkoFyre::Network::GkXmpp::GkUserConn &connection_details,
                                 QPointer<GekkoFyre::GkXmppClient> xmppClient, const QStringList &bareJids,
                                 QWidget *parent = nullptr);
    ~GkXmppMessageDialog();

private slots:
    void on_toolButton_font_clicked();
    void on_toolButton_font_reset_clicked();
    void on_toolButton_insert_clicked();
    void on_toolButton_attach_file_clicked();
    void on_toolButton_view_roster_clicked();
    void on_tableView_recv_msg_dlg_customContextMenuRequested(const QPoint &pos);
    void on_textEdit_tx_msg_dialog_textChanged();
    void on_lineEdit_message_search_returnPressed();

    void updateInterface(const QStringList &bareJids);
    void determineNickname();
    void submitMsgEnterKey();
    void updateToolbarStatus(const QString &value);

    //
    // Message handling and QXmppArchiveManager-related
    void recvXmppMsg(const QXmppMessage &msg);
    void procMsgArchive(const QString &bareJid);
    void updateMsgHistory();
    QXmppMessage createXmppMessageIq(const QString &to, const QString &from, const QString &message) const;

    //
    // QXmppMamManager handling
    void msgArchiveSuccReceived();
    void procMamArchive(const QString &bareJid);
    void getArchivedMessages();
    void getArchivedMessagesFromDb(const QString &bareJid, const bool &insertData, const bool &presented = false);

signals:
    void updateToolbar(const QString &value);

    //
    // Message handling and QXmppArchiveManager-related
    void sendXmppMsg(const QString &bareJid, const QXmppMessage &msg, const QDateTime &beginTimestamp,
                     const QDateTime &endTimestamp);

    //
    // QXmppMamManager handling
    void updateMamArchive(const QString &bareJid);

private:
    Ui::GkXmppMessageDialog *ui;

    //
    // QTableView and related
    //
    QPointer<GekkoFyre::GkXmppRecvMsgsTableViewModel> gkXmppRecvMsgsTableViewModel;

    //
    // Spell-checking, dictionaries, etc.
    //
    QPointer<QtSpell::TextEditChecker> m_spellChecker;

    //
    // Miscellaneous
    //
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    std::queue<QString> m_toolBarTextQueue;

    //
    // QXmpp and XMPP related
    //
    GekkoFyre::Network::GkXmpp::GkUserConn gkConnDetails;
    QPointer<GekkoFyre::GkXmppClient> m_xmppClient;
    GekkoFyre::Network::GkXmpp::GkNetworkState m_netState;
    QStringList m_bareJids;
    QString m_clientNickname;
};

