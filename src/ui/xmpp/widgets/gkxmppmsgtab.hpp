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
#include "src/gk_xmpp_client.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/models/xmpp/gk_xmpp_msg_handler.hpp"
#include "src/models/tableview/gk_xmpp_recv_msgs_model.hpp"
#include "src/models/spelling/gk_text_edit_spelling_highlight.hpp"
#include <QWidget>
#include <QString>
#include <QObject>
#include <QPointer>
#include <queue>
#include <string>

namespace Ui {
class GkXmppMsgTab;
}

class GkXmppMsgTab : public QWidget
{
    Q_OBJECT

public:
    explicit GkXmppMsgTab(QPointer<GekkoFyre::GkTextEditSpellHighlight> spellCheckWidget,
                          GekkoFyre::Network::GkXmpp::GkUserConn connDetails,
                          QPointer<GekkoFyre::GkEventLogger> eventLogger,
                          QPointer<GekkoFyre::StringFuncs> stringFuncs, QWidget *parent = nullptr);
    ~GkXmppMsgTab();

public slots:
    //
    // QXmpp Roster handling and related
    void updateRosterStatus(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &gkMsgTabRoster);
    void updateToolbarStatus(const QString &value);

    //
    // QXmpp message handling and related
    void recvXmppMsg(const QXmppMessage &msg);
    void recvMsgArchive(const QString &bareJid);
    void getArchivedMessagesFromDb(const QXmppMessage &message, const bool &wipeExistingHistory = false);

private slots:
    //
    // Individual one-on-one chat
    void on_toolButton_view_roster_triggered(QAction *arg1);
    void on_toolButton_font_triggered(QAction *arg1);
    void on_toolButton_font_reset_triggered(QAction *arg1);
    void on_toolButton_insert_triggered(QAction *arg1);
    void on_toolButton_attach_file_triggered(QAction *arg1);
    void on_comboBox_tx_msg_shortcut_cmds_currentIndexChanged(int index);

    void updateInterface(const QStringList &bareJids);

signals:
    void updateTabHeader(const QString &header_title);

private:
    Ui::GkXmppMsgTab *ui;

    //
    // Miscellaneous
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    std::queue<QString> m_toolBarTextQueue;

    //
    // QTableView and related
    QPointer<GekkoFyre::GkXmppRecvMsgsTableViewModel> gkXmppRecvMsgsTableViewModel;
    QPointer<GekkoFyre::GkXmppRecvMsgsTableViewModel> gkXmppRecvMucChatTableViewModel;

    //
    // QXmpp and XMPP related
    GekkoFyre::Network::GkXmpp::GkUserConn gkConnDetails;
    QPointer<GekkoFyre::GkXmppClient> m_xmppClient;
    QStringList m_bareJids;
    GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster gkTabRoster;

    //
    // Multithreading, mutexes, etc.
    std::mutex m_archivedMsgsFromDbMtx;

    //
    // Widgets
    QPointer<GekkoFyre::GkTextEditSpellHighlight> gkSpellCheckerHighlighter;
};
