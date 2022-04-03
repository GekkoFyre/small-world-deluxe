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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_xmpp_client.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/ui/xmpp/widgets/gkxmppmsgtab.hpp"
#include "src/ui/xmpp/widgets/gkxmppmuctab.hpp"
#include "src/models/tableview/gk_xmpp_recv_msgs_model.hpp"
#include "src/models/spelling/gk_text_edit_spelling_highlight.hpp"
#include <qxmpp/QXmppMessage.h>
#include <queue>
#include <mutex>
#include <thread>
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

class GkXmppMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkXmppMessageDialog(QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                 QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Network::GkXmpp::GkUserConn &connection_details,
                                 QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                 std::shared_ptr<QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign>> rosterList,
                                 QWidget *parent = nullptr);
    ~GkXmppMessageDialog();

public slots:
    void openMsgDlg(const QString &bareJid, const qint32 &tabIdx);
    void closeMsgDlg(const QString &bareJid, const qint32 &tabIdx);
    void openMucDlg(const QString &mucJid, const qint32 &tabIdx);
    void closeMucDlg(const QString &mucJid, const qint32 &tabIdx);

private slots:
    void on_tableView_recv_msg_dlg_customContextMenuRequested(const QPoint &pos);
    void on_textEdit_tx_msg_dialog_textChanged();
    void on_lineEdit_message_search_returnPressed();

    void determineNickname();
    void submitMsgEnterKey();

    //
    // Message handling and QXmppArchiveManager-related
    void updateMsgHistory();
    QXmppMessage createXmppMessageIq(const QString &to, const QString &from, const QString &message) const;

    //
    // QXmppMamManager handling
    void msgArchiveSuccReceived();
    void dlArchivedMessages();

    // Tab window management
    void on_tabWidget_chat_window_tabCloseRequested(int index);

signals:
    void updateToolbar(const QString &value);

    //
    // Message handling and QXmppArchiveManager-related
    void sendXmppMsg(const QXmppMessage &msg);
    void procMsgArchive(const QString &bareJid);
    void procMsgArchive(const QStringList &bareJids);

    //
    // QXmppMamManager handling
    void updateTableModel();

    //
    // QXmpp Roster handling and related
    void updateRoster(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &gkMsgTabRoster);

private:
    Ui::GkXmppMessageDialog *ui;

    //
    // Widgets
    QPointer<GekkoFyre::GkTextEditSpellHighlight> gkSpellCheckerHighlighter;

    //
    // Multithreading, mutexes, etc.
    std::mutex m_archivedMsgsFromDbMtx;
    std::vector<std::thread> m_archivedMsgsBulkThreadVec;

    //
    // Miscellaneous
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::GkLevelDb> gkDb;

    //
    // QXmpp and XMPP related
    GekkoFyre::Network::GkXmpp::GkUserConn gkConnDetails;
    QPointer<GekkoFyre::GkXmppClient> m_xmppClient;
    GekkoFyre::Network::GkXmpp::GkNetworkState m_netState;
    std::shared_ptr<QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign>> m_rosterList;
    QStringList m_bareJids;
    QString m_clientNickname;

    //
    // QTabWidget related
    QPointer<GkXmppMsgTab> gkXmppMsgTab;
    QPointer<GkXmppMucTab> gkXmppMucTab;
    QMap<quint16, GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster> gkTabMap;                    // A QMap of all the opened tabs, with the key being the QTabWidget index.

    void updateUsersHelper();

};
