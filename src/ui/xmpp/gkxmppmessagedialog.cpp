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

#include "gkxmppmessagedialog.hpp"
#include "ui_gkxmppmessagedialog.h"
#include <chrono>
#include <thread>
#include <utility>
#include <iterator>
#include <iostream>
#include <QMap>
#include <QIcon>
#include <QPixmap>
#include <QKeyEvent>

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

bool GkPlainTextKeyEnter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return)) {
            // The return key has been pressed!
            emit submitMsgEnterKey();
        } else {
            return QObject::eventFilter(obj, event);
        }

        return true;
    } else {
        return QObject::eventFilter(obj, event);
    }

    return false;
}

/**
 * @brief GkXmppMessageDialog::GkXmppMessageDialog
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param xmppClient The XMPP client object.
 * @param bareJid The user we are in communiqué with!
 * @param parent The parent to this dialog.
 */
GkXmppMessageDialog::GkXmppMessageDialog(QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                         QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Network::GkXmpp::GkUserConn &connection_details,
                                         QPointer<GekkoFyre::GkXmppClient> xmppClient, const QStringList &bareJids,
                                         QWidget *parent) : QDialog(parent), ui(new Ui::GkXmppMessageDialog)
{
    ui->setupUi(this);

    try {
        gkStringFuncs = std::move(stringFuncs);
        gkEventLogger = std::move(eventLogger);
        gkDb = std::move(database);
        gkConnDetails = connection_details;
        m_xmppClient = std::move(xmppClient);
        m_bareJids = bareJids;

        //
        // Initialize any boolean values...
        startupSucc = false; // Has startup of this class already succeeded?

        //
        // Setup and initialize signals and slots...
        QObject::connect(this, SIGNAL(updateToolbar(const QString &)), this, SLOT(updateToolbarStatus(const QString &)));
        QObject::connect(this, SIGNAL(sendXmppMsg(const QXmppMessage &)), m_xmppClient, SLOT(sendXmppMsg(const QXmppMessage &)));
        QObject::connect(m_xmppClient, SIGNAL(xmppMsgUpdate(const QXmppMessage &)), this, SLOT(recvXmppMsg(const QXmppMessage &)));
        QObject::connect(m_xmppClient, SIGNAL(updateMsgHistory()), this, SLOT(updateMsgHistory()));
        QObject::connect(m_xmppClient, SIGNAL(msgArchiveSuccReceived()), this, SLOT(msgArchiveSuccReceived()));
        QObject::connect(this, SIGNAL(updateMamArchive(const QString &, const bool &, const bool &)),
                         this, SLOT(procMamArchive(const QString &, const bool &, const bool &)));

        //
        // Setup and initialize QTableView's...
        gkXmppRecvMsgsTableViewModel = new GkXmppRecvMsgsTableViewModel(ui->tableView_recv_msg_dlg, m_xmppClient, this);
        ui->tableView_recv_msg_dlg->setModel(gkXmppRecvMsgsTableViewModel);

        ui->tableView_recv_msg_dlg->horizontalHeader()->setSectionResizeMode(GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_DATETIME_IDX, QHeaderView::ResizeToContents);
        ui->tableView_recv_msg_dlg->horizontalHeader()->setSectionResizeMode(GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_NICKNAME_IDX, QHeaderView::ResizeToContents);
        ui->tableView_recv_msg_dlg->horizontalHeader()->setStretchLastSection(true);
        ui->tableView_recv_msg_dlg->horizontalHeader()->setHidden(true);
        ui->tableView_recv_msg_dlg->setSortingEnabled(false);
        ui->tableView_recv_msg_dlg->setVisible(true);
        ui->tableView_recv_msg_dlg->show();
        ui->tableView_recv_msg_dlg->scrollToBottom();

        ui->label_callsign_1_stats->setText(QString("1 %1").arg(tr("user in chat")));
        ui->label_msging_callsign_status->setText("");

        ui->tab_user_msg->setWindowIcon(QPixmap(":/resources/contrib/images/vector/no-attrib/walkie-talkies.svg"));
        ui->toolButton_font->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/font.svg"));
        ui->toolButton_font_reset->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/eraser.svg"));
        ui->toolButton_insert->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/moustache-cream.svg"));
        ui->toolButton_attach_file->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/attached-file.svg"));

        QPointer<GkPlainTextKeyEnter> gkPlaintextKeyEnter = new GkPlainTextKeyEnter();
        QObject::connect(gkPlaintextKeyEnter, SIGNAL(submitMsgEnterKey()), this, SLOT(submitMsgEnterKey()));
        ui->textEdit_tx_msg_dialog->installEventFilter(gkPlaintextKeyEnter);
        // m_spellChecker->setTextEdit(ui->textEdit_tx_msg_dialog); // Add the QtSpell spelling-checker to the QTextEdit object!

        determineNickname();
        updateInterface(m_bareJids);

        if (!m_xmppClient->isConnected()) {
            ui->textEdit_tx_msg_dialog->setEnabled(false);
        }

        if (m_xmppClient->isConnected()) {
            ui->textEdit_tx_msg_dialog->setEnabled(true);
            getArchivedMessages(); // Gather anything possible from the Google LevelDB database!
        }

        QObject::connect(m_xmppClient, &QXmppClient::connected, this, [=]() {
            ui->textEdit_tx_msg_dialog->setEnabled(true);

            //
            // Now gather any data possible from the given XMPP server via the Internet!
            getArchivedMessages();
        });

        QObject::connect(m_xmppClient, &QXmppClient::disconnected, this, [=]() {
            ui->textEdit_tx_msg_dialog->setEnabled(false);
        });
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An error has occurred related to XMPP functions. The error in question:\n\n%1").arg(QString::fromStdString(e.what())),
                                    GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

GkXmppMessageDialog::~GkXmppMessageDialog()
{
    delete ui;
}

/**
 * @brief GkXmppMessageDialog::on_toolButton_font_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::on_toolButton_font_clicked()
{
    return;
}

/**
 * @brief GkXmppMessageDialog::on_toolButton_font_reset_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::on_toolButton_font_reset_clicked()
{
    return;
}

/**
 * @brief GkXmppMessageDialog::on_toolButton_insert_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::on_toolButton_insert_clicked()
{
    return;
}

/**
 * @brief GkXmppMessageDialog::on_toolButton_attach_file_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::on_toolButton_attach_file_clicked()
{
    return;
}

/**
 * @brief GkXmppMessageDialog::on_toolButton_view_roster_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::on_toolButton_view_roster_clicked()
{
    return;
}

/**
 * @brief GkXmppMessageDialog::on_tableView_recv_msg_dlg_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void GkXmppMessageDialog::on_tableView_recv_msg_dlg_customContextMenuRequested(const QPoint &pos)
{
    return;
}

/**
 * @brief GkXmppMessageDialog::on_textEdit_tx_msg_dialog_textChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::on_textEdit_tx_msg_dialog_textChanged()
{
    return;
}

/**
 * @brief GkXmppMessageDialog::on_lineEdit_message_search_returnPressed
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::on_lineEdit_message_search_returnPressed()
{
    return;
}

/**
 * @brief GkXmppMessageDialog::updateInterface
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppMessageDialog::updateInterface(const QStringList &bareJids)
{
    ui->label_callsign_1_stats->setText(tr("%1 users in chat").arg(QString::number(bareJids.count() + 1))); // Includes both the user in communique and the client themselves!
    auto tmpRosterList = m_xmppClient->getRosterMap();
    for (const auto &bareJid: bareJids) {
        for (const auto &rosterJid: tmpRosterList) {
            if (bareJid == rosterJid.bareJid) {
                if (bareJids.count() == 1) {
                    ui->tabWidget_chat_window->setTabText(0, gkStringFuncs->trimStrToCharLength(rosterJid.vCard.nickName(), 16, true));
                    ui->label_callsign_2_name->setText(tr("Welcome, %1 and %2!").arg(m_xmppClient->getUsername(gkConnDetails.jid)).arg(m_xmppClient->getUsername(bareJid)));
                    setWindowTitle(tr("%1 -- Small World Deluxe").arg(gkStringFuncs->trimStrToCharLength(rosterJid.vCard.nickName(), 32, true)));
                    break;
                } else {
                    ui->tabWidget_chat_window->setTabText(0, QString("%1, ...").arg(gkStringFuncs->trimStrToCharLength(rosterJid.vCard.nickName(), 16, false)));
                    ui->label_callsign_2_name->setText(tr("Welcome, %1, %2, etc.!").arg(m_xmppClient->getUsername(gkConnDetails.jid)).arg(m_xmppClient->getUsername(bareJid)));
                    setWindowTitle(tr("%1, etc. -- Small World Deluxe").arg(gkStringFuncs->trimStrToCharLength(rosterJid.vCard.nickName(), 32, true)));
                    break;
                }
            }
        }
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::determineNickname
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::determineNickname()
{
    if (!gkConnDetails.nickname.isEmpty()) {
        m_clientNickname = gkConnDetails.nickname;
    }

    if (!gkConnDetails.firstName.isEmpty() && m_clientNickname.isEmpty()) {
        m_clientNickname = gkConnDetails.firstName;
    }

    if (!gkConnDetails.email.isEmpty() && m_clientNickname.isEmpty()) {
        m_clientNickname = gkConnDetails.email;
    }

    if (!gkConnDetails.username.isEmpty() && m_clientNickname.isEmpty()) {
        m_clientNickname = gkConnDetails.username;
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::submitMsgEnterKey processes the carriage return for UI element, `ui->textEdit_tx_msg_dialog()`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::submitMsgEnterKey()
{
    m_netState = m_xmppClient->getNetworkState();
    if (!m_xmppClient->isConnected() || m_netState != GkNetworkState::Connected) {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Disconnected"));
        msgBox.setText(tr("You are currently not connected to any XMPP server! Make a connection now?"));
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Warning);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                m_xmppClient->createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, gkConnDetails.password,
                                                       gkConnDetails.jid, false);
                break;
            case QMessageBox::Cancel:
                return;
            default:
                return;
        }
    }

    if (m_xmppClient->isConnected()) {
        const auto plaintext = ui->textEdit_tx_msg_dialog->toPlainText();
        if (!plaintext.isEmpty()) {
            for (const auto bareJid: m_bareJids) {
                if (!bareJid.isEmpty()) {
                    const auto toMsg = createXmppMessageIq(bareJid, gkConnDetails.jid, plaintext);
                    if (toMsg.isXmppStanza()) {
                        emit sendXmppMsg(toMsg);
                    }
                }
            }
        }

        ui->textEdit_tx_msg_dialog->clear();
    } else {
        m_netState = m_xmppClient->getNetworkState();
        if (m_netState == GkNetworkState::Connecting) {
            emit updateToolbar(tr("Attempting to make a connection... please wait..."));
        }
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::updateToolbarStatus
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 */
void GkXmppMessageDialog::updateToolbarStatus(const QString &value)
{
    if (!value.isEmpty()) {
        m_toolBarTextQueue.emplace(value);
        qint32 counter = 0;
        while (!m_toolBarTextQueue.empty()) {
            if (counter > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            }

            ui->label_msging_callsign_status->setText(m_toolBarTextQueue.front());
            m_toolBarTextQueue.pop();
            ++counter;
        }

        QPointer<QTimer> toolbarTimer = new QTimer(this);
        QObject::connect(toolbarTimer, &QTimer::timeout, ui->label_msging_callsign_status, &QLabel::clear);
        toolbarTimer->start(3000);
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::recvXmppMsg notifies that an XMPP message stanza is received. The QXmppMessage parameter
 * contains the details of the message sent to this client. In other words whenever someone sends you a message this signal
 * is emitted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msg The received message stanza in question.
 */
void GkXmppMessageDialog::recvXmppMsg(const QXmppMessage &msg)
{
    if (msg.isXmppStanza() && !msg.body().isEmpty()) {
        if (!this->isActiveWindow()) {
            gkEventLogger->publishEvent(gkStringFuncs->trimStrToCharLength(msg.body(), 80, true), GkSeverity::Info, "", true, true, false, false);
        }

        gkXmppRecvMsgsTableViewModel->insertData(msg.from(), msg.body());
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::procMsgArchive processes and manages the archiving of chat messages and their histories from
 * a given XMPP server, provided said server supports this functionality. This will take the information gathered from
 * the, `m_xmppClient`, object and insert it into the appropriate QTableView.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The username in relation to the chat history being requested.
 */
void GkXmppMessageDialog::procMsgArchive(const QString &bareJid)
{
    auto rosterMap = m_xmppClient->getRosterMap();
    if (!rosterMap.isEmpty()) {
        gkXmppRecvMsgsTableViewModel.clear();
        for (const auto &roster: rosterMap) {
            if (roster.bareJid == bareJid) {
                if (!roster.archive_messages.isEmpty()) {
                    for (const auto &message: roster.archive_messages) {
                        gkXmppRecvMsgsTableViewModel->insertData(bareJid, message.message.body(), message.message.date());
                    }
                }
            }
        }
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::updateMsgHistory
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::updateMsgHistory()
{
    for (const auto &bareJid: m_bareJids) {
        procMsgArchive(bareJid);
    }

    procMsgArchive(gkConnDetails.jid);

    return;
}

/**
 * @brief GkXmppMessageDialog::createXmppMessageIq is simply a helper function to create a QXmppMessage stanza.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param to The recepient of this message.
 * @param from The sender of this message.
 * @param message The body of the given message.
 * @return The thusly created QXmppMessage stanza.
 */
QXmppMessage GkXmppMessageDialog::createXmppMessageIq(const QString &to, const QString &from, const QString &message) const
{
    QXmppMessage xmppMsg;
    xmppMsg.setFrom(from);
    xmppMsg.setTo(to);
    xmppMsg.setBody(message);
    xmppMsg.setStamp(QDateTime::currentDateTimeUtc());
    xmppMsg.setPrivate(false);
    xmppMsg.setReceiptRequested(true);
    xmppMsg.setType(QXmppMessage::Chat);

    return xmppMsg;
}

/**
 * @brief GkXmppMessageDialog::msgArchiveSuccReceived
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::msgArchiveSuccReceived()
{
    if (!startupSucc) {
        emit updateMamArchive(gkConnDetails.jid, true, true);
        for (const auto &bareJid: m_bareJids) {
            emit updateMamArchive(bareJid, false, true);
        }

        startupSucc = true;
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::procMamArchive is a helper class for dealing with the processing of recorded messages that
 * have been archived to a given XMPP server, according to specification, XEP-0313 (Message Archive Management).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The user we are in communiqué with!
 * @param wipeExistingHistory Whether to wipe the pre-existing chat history or not, specifically from the given QTableView
 * object, GekkoFyre::GkXmppRecvMsgsTableViewModel().
 * @param presented Should we update the fact that these messages are now already 'presented' (and thereby inserted into
 * the QTableView) or not?
 */
void GkXmppMessageDialog::procMamArchive(const QString &bareJid, const bool &wipeExistingHistory, const bool &presented)
{
    try {
        if (!startupSucc) {
            getArchivedMessagesFromDb(bareJid, true, wipeExistingHistory, presented);
        }
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::getArchivedMessages
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::getArchivedMessages()
{
    for (const auto &bareJid: m_bareJids) {
        m_xmppClient->getArchivedMessages(gkConnDetails.jid, QString(), bareJid); // Get archived messages sent by the Jid in question!
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::getArchivedMessagesFromDb attempts to get previously archived messages from the Google
 * LevelDB database that is associated with this user's instance of Small World Deluxe, and insert them into the chat
 * window, thereby saving bandwidth from not having to unnecessarily re-download messages from the given XMPP server
 * again.
 * NOTE: The Google LevelDB functionality is currently disabled!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The user we are in communiqué with!
 * @param insertData Whether to insert the archived messages into the given QTableView or not.
 * @param wipeExistingHistory Whether to wipe the pre-existing chat history or not, specifically from the given QTableView
 * object, GekkoFyre::GkXmppRecvMsgsTableViewModel().
 * @param updateSortFilterProxy Should the QSortFilterProxyModel for the QTableView be updated and reset back to its beginning
 * values (please use this sparingly because of required system resources for calculations!)?
 * @param presented Should we update the fact that these messages are now already 'presented' (and thereby inserted into
 * the QTableView) or not?
 */
void GkXmppMessageDialog::getArchivedMessagesFromDb(const QString &bareJid, const bool &insertData, const bool &wipeExistingHistory,
                                                    const bool &updateSortFilterProxy, const bool &presented)
{
    try {
        if (!bareJid.isEmpty()) {
            auto rosterMap = m_xmppClient->getRosterMap(); // Grab the `GkXmppCallsign()` object and temporarily store it in memory for just use in this function!
            if (!rosterMap.isEmpty()) {
                std::lock_guard<std::mutex> lock_guard(m_archivedMsgsFromDbMtx);
                if (wipeExistingHistory) {
                    gkXmppRecvMsgsTableViewModel->removeData();
                }

                for (auto roster = rosterMap.begin(); roster != rosterMap.end(); ++roster) {
                    if (roster->bareJid == bareJid) {
                        gkEventLogger->publishEvent(tr("Gathering archived messages for XMPP user, \"%1\", from the database!").arg(roster->bareJid), GkSeverity::Debug,
                                                    "", false, true, false, false, false);
                        if (!roster->messages.isEmpty()) {
                            for (auto iter = roster->messages.begin(); iter != roster->messages.end(); ++iter) {
                                if (!iter->presented && insertData) {
                                    gkXmppRecvMsgsTableViewModel->insertData(bareJid, iter->message.body(), iter->message.stamp());
                                    iter->presented = true;
                                } else {
                                    iter->presented = presented;
                                }
                            }

                            m_xmppClient->updateRosterMap(rosterMap);
                        }

                        ui->tableView_recv_msg_dlg->scrollToBottom();
                        break;
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}
