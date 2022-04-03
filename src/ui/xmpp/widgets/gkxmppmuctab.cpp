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

#include "gkxmppmuctab.hpp"
#include "ui_gkxmppmuctab.h"
#include <chrono>
#include <utility>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

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

GkXmppMucTab::GkXmppMucTab(QPointer<GekkoFyre::GkTextEditSpellHighlight> spellCheckWidget,
                           GekkoFyre::Network::GkXmpp::GkUserConn connDetails,
                           QPointer<GekkoFyre::GkEventLogger> eventLogger,
                           QPointer<GekkoFyre::StringFuncs> stringFuncs, QWidget *parent) :
                           QWidget(parent), ui(new Ui::GkXmppMucTab)
{
    ui->setupUi(this);

    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    gkConnDetails = connDetails;

    //
    // Initialize spelling and grammar checker, dictionaries, etc.
    gkSpellCheckerHighlighter = std::move(spellCheckWidget);
    ui->verticalLayout_6->removeWidget(ui->textEdit_muc_tx_msg_dialog);
    ui->verticalLayout_6->addWidget(gkSpellCheckerHighlighter);

    //
    // Setup and initialize QTableView's...
    gkXmppRecvMucChatTableViewModel = new GkXmppRecvMsgsTableViewModel(ui->tableView_muc_recv_conversation, m_xmppClient, this);
    // gkXmppMsgEngine = new GkXmppMsgEngine(this);
    ui->tableView_muc_recv_conversation->setModel(gkXmppRecvMucChatTableViewModel);

    ui->label_muc_callsign_1_stats->setText(QString("1 %1").arg(tr("user in chat")));
    ui->label_muc_msging_callsign_status->setText("");

    ui->toolButton_muc_font->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/font.svg"));
    ui->toolButton_muc_font_reset->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/eraser.svg"));
    ui->toolButton_muc_insert->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/moustache-cream.svg"));
    ui->toolButton_muc_attach_file->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/attached-file.svg"));

    gkSpellCheckerHighlighter->setEnabled(true);
    QObject::connect(m_xmppClient, &QXmppClient::disconnected, this, [=]() {
        gkSpellCheckerHighlighter->setEnabled(false);
    });

    updateInterface(m_bareJids);
}

GkXmppMucTab::~GkXmppMucTab()
{
    delete ui;
}

/**
 * @brief GkXmppMucTab::updateToolbarStatus
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 */
void GkXmppMucTab::updateToolbarStatus(const QString &value)
{
    if (!value.isEmpty()) {
        m_toolBarTextQueue.emplace(value);
        qint32 counter = 0;
        while (!m_toolBarTextQueue.empty()) {
            if (counter > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            }

            ui->label_muc_msging_callsign_status->setText(m_toolBarTextQueue.front());
            m_toolBarTextQueue.pop();
            ++counter;
        }

        QPointer<QTimer> toolbarTimer = new QTimer(this);
        QObject::connect(toolbarTimer, &QTimer::timeout, ui->label_muc_msging_callsign_status, &QLabel::clear);
        toolbarTimer->start(3000);
    }

    return;
}

/**
 * @brief GkXmppMucTab::recvXmppMsg notifies that an XMPP message stanza is received. The QXmppMessage parameter contains
 * the details of the message sent to this client. In other words, whenever someone sends you a message this SLOT is
 * activated via the related SIGNAL.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msg
 */
void GkXmppMucTab::recvXmppMsg(const QXmppMessage &msg)
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
 * @brief GkXmppMucTab::recvMsgArchive processes and manages the archiving of chat messages and their histories from
 * a given XMPP server, provided said server supports this functionality. This will take the information gathered from
 * the, `m_xmppClient`, object and insert it into the appropriate QTableView.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The username(s) in relation to the chat history being requested.
 */
void GkXmppMucTab::recvMsgArchive(const QStringList &bareJids)
{
    // TODO: Refactor this code!
    if (!gkTabRoster.roster.isEmpty()) {
        gkXmppRecvMsgsTableViewModel.clear();
        for (const auto &roster: gkTabRoster.roster) {
            for (const auto &bareJid: bareJids) {
                if (roster.bareJid == bareJid) {
                    if (!roster.archive_messages.isEmpty()) {
                        for (const auto &message: roster.archive_messages) {
                            gkXmppRecvMsgsTableViewModel->insertData(bareJid, message.message.body(), message.message.date());
                        }
                    }
                }
            }
        }
    }

    return;
}

/**
 * @brief GkXmppMucTab::getArchivedMessagesFromDb attempts to get previously archived messages from the Google
 * LevelDB database that is associated with this user's instance of Small World Deluxe, and insert them into the chat
 * window, thereby saving bandwidth from not having to unnecessarily re-download messages from the given XMPP server
 * again.
 * NOTE: The Google LevelDB functionality is currently disabled!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param message The QXmppMessage stanza itself to be inserted into the QTableView model, GekkoFyre::GkXmppRecvMsgsTableViewModel().
 * @param wipeExistingHistory Whether to wipe the pre-existing chat history or not, specifically from the given QTableView
 * object, GekkoFyre::GkXmppRecvMsgsTableViewModel().
 */
void GkXmppMucTab::getArchivedMessagesFromDb(const QXmppMessage &message, const bool &wipeExistingHistory)
{
    try {
        std::lock_guard<std::mutex> lock_guard(m_archivedMsgsFromDbMtx);
        if (wipeExistingHistory) {
            gkXmppRecvMsgsTableViewModel->removeData();
        }

        if (message.isXmppStanza() && !message.body().isEmpty()) {
            gkXmppRecvMsgsTableViewModel->insertData(message.from(), message.body(), message.stamp());
        }

        ui->tableView_muc_recv_conversation->scrollToBottom();
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkXmppMucTab::openMucDlg opens a new dialog from within this classes own UI, via the (Q)Xmpp roster
 * manager, based upon a MUC so that the end-user may send/receive messages to others within the framework of a
 * chat-room.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param mucRoster
 */
void GkXmppMucTab::openMucDlg(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &mucRoster)
{
    gkTabRoster = mucRoster;

    return;
}

/**
 * @brief GkXmppMucTab::closeMucDlg closes a previously used MUC dialog, created at one point to engage in
 * multi-user communications with specified end-users on the given Xmpp server or servers.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param mucJid The MUC to close message dialog we were in communiqué with!
 * @param tabIdx The tab index in relation to the aforementioned!
 */
void GkXmppMucTab::closeMucDlg(const QString &mucJid, const qint32 &tabIdx)
{
    return;
}

/**
 * @brief GkXmppMucTab::on_toolButton_muc_view_roster_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void GkXmppMucTab::on_toolButton_muc_view_roster_triggered(QAction *arg1)
{
    QMessageBox::information(nullptr, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkXmppMucTab::on_toolButton_muc_font_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void GkXmppMucTab::on_toolButton_muc_font_triggered(QAction *arg1)
{
    QMessageBox::information(nullptr, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkXmppMucTab::on_toolButton_muc_font_reset_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void GkXmppMucTab::on_toolButton_muc_font_reset_triggered(QAction *arg1)
{
    QMessageBox::information(nullptr, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkXmppMucTab::on_toolButton_muc_insert_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void GkXmppMucTab::on_toolButton_muc_insert_triggered(QAction *arg1)
{
    QMessageBox::information(nullptr, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkXmppMucTab::on_toolButton_muc_attach_file_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void GkXmppMucTab::on_toolButton_muc_attach_file_triggered(QAction *arg1)
{
    QString filePath = QFileDialog::getOpenFileName(nullptr, tr("Open Image"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                    tr("All Image Files (*.png *.jpg *.jpeg *.jpe *.jfif *.exif *.bmp *.gif);;PNG (*.png);;JPEG (*.jpg *.jpeg *.jpe *.jfif *.exif);;Bitmap (*.bmp);;GIF (*.gif);;All Files (*.*)"));
    if (!filePath.isEmpty()) {
        // TODO: Implement this function!
        return;
    }

    return;
}

/**
 * @brief GkXmppMucTab::on_comboBox_muc_tx_msg_shortcut_cmds_currentIndexChanged processes shortcuts and macros within
 * the (Q)Xmpp environment, as either configured by default or the end-user themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppMucTab::on_comboBox_muc_tx_msg_shortcut_cmds_currentIndexChanged(int index)
{
    QMessageBox::information(nullptr, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkXmppMucTab::updateInterface updates the tab's widget interface but this time around, for an MUC oriented one!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJids The bareJid(s) involved within this specific chat session.
 */
void GkXmppMucTab::updateInterface(const QStringList &bareJids)
{
    ui->label_muc_callsign_1_stats->setText(tr("%1 users in chat").arg(QString::number(bareJids.count() + 1))); // Includes both the user in communique and the client themselves!
    for (const auto &bareJid: bareJids) {
        for (const auto &rosterJid: gkTabRoster.roster) {
            if (bareJid == rosterJid.bareJid) {
                if (bareJids.count() == 1) {
                    emit updateTabHeader(gkStringFuncs->trimStrToCharLength(rosterJid.vCard.nickName(), 16, true));
                    ui->label_muc_callsign_2_name->setText(tr("Welcome, %1 and %2!").arg(gkStringFuncs->getXmppUsername(gkConnDetails.jid)).arg(gkStringFuncs->getXmppUsername(bareJid)));
                    setWindowTitle(tr("%1 -- Small World Deluxe").arg(gkStringFuncs->trimStrToCharLength(rosterJid.vCard.nickName(), 32, true)));
                    break;
                } else {
                    emit updateTabHeader(gkStringFuncs->trimStrToCharLength(rosterJid.vCard.nickName(), 16, false));
                    ui->label_muc_callsign_2_name->setText(tr("Welcome, %1, %2, etc.!").arg(gkStringFuncs->getXmppUsername(gkConnDetails.jid)).arg(gkStringFuncs->getXmppUsername(bareJid)));
                    setWindowTitle(tr("%1, etc. -- Small World Deluxe").arg(gkStringFuncs->trimStrToCharLength(rosterJid.vCard.nickName(), 32, true)));
                    break;
                }
            }
        }
    }

    return;
}
