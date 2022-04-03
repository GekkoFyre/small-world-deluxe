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

#include "gkxmppmessagedialog.hpp"
#include "ui_gkxmppmessagedialog.h"
#include "src/models/xmpp/gk_xmpp_msg_handler.hpp"
#include <chrono>
#include <thread>
#include <utility>
#include <iterator>
#include <algorithm>
#include <QIcon>
#include <QUuid>
#include <QPixmap>
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

/**
 * @brief GkXmppMessageDialog::GkXmppMessageDialog
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param xmppClient The XMPP client object.
 * @param bareJid The user we are in communiqué with!
 * @param parent The parent to this dialog.
 */
GkXmppMessageDialog::GkXmppMessageDialog(QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                         QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Network::GkXmpp::GkUserConn &connection_details,
                                         QPointer<GekkoFyre::GkXmppClient> xmppClient, std::shared_ptr<QList<GkXmppCallsign>> rosterList,
                                         QWidget *parent) : QDialog(parent), ui(new Ui::GkXmppMessageDialog)
{
    ui->setupUi(this);

    try {
        gkStringFuncs = std::move(stringFuncs);
        gkEventLogger = std::move(eventLogger);
        gkDb = std::move(database);
        gkConnDetails = connection_details;
        m_xmppClient = std::move(xmppClient);
        m_rosterList = std::move(rosterList);

        //
        // Initialize spelling and grammar checker, dictionaries, etc.
        gkSpellCheckerHighlighter = new GkTextEditSpellHighlight(gkEventLogger);

        //
        // QTabWidget initialization!
        gkXmppMsgTab = new GkXmppMsgTab(gkSpellCheckerHighlighter, gkConnDetails, gkEventLogger, gkStringFuncs, this);
        gkXmppMucTab = new GkXmppMucTab(gkSpellCheckerHighlighter, gkConnDetails, gkEventLogger, gkStringFuncs, this);

        QObject::connect(this, SIGNAL(closeMsgTab(const QString &, const qint32 &)),
                         gkXmppMsgTab, SLOT(closeMsgDlg(const QString &, const qint32 &)));
        QObject::connect(this, SIGNAL(closeMucTab(const QString &, const qint32 &)),
                         gkXmppMucTab, SLOT(closeMucDlg(const QString &, const qint32 &)));
        QObject::connect(this, SIGNAL(updateToolbar(const QString &)),
                         gkXmppMsgTab, SLOT(updateToolbarStatus(const QString &)));
        QObject::connect(this, SIGNAL(updateToolbar(const QString &)),
                         gkXmppMucTab, SLOT(updateToolbarStatus(const QString &)));
        QObject::connect(m_xmppClient, SIGNAL(xmppMsgUpdate(const QXmppMessage &)),
                         gkXmppMsgTab, SLOT(recvXmppMsg(const QXmppMessage &)));
        QObject::connect(m_xmppClient, SIGNAL(xmppMsgUpdate(const QXmppMessage &)),
                         gkXmppMucTab, SLOT(recvXmppMsg(const QXmppMessage &)));
        QObject::connect(this, SIGNAL(procMsgArchive(const QString &)),
                         gkXmppMsgTab, SLOT(recvMsgArchive(const QString &)));
        QObject::connect(this, SIGNAL(procMsgArchive(const QStringList &)),
                         gkXmppMucTab, SLOT(recvMsgArchive(const QStringList &)));
        QObject::connect(m_xmppClient, SIGNAL(procXmppMsg(const QXmppMessage &, const bool &)),
                         gkXmppMsgTab, SLOT(getArchivedMessagesFromDb(const QXmppMessage &, const bool &)));
        QObject::connect(m_xmppClient, SIGNAL(procXmppMsg(const QXmppMessage &, const bool &)),
                         gkXmppMucTab, SLOT(getArchivedMessagesFromDb(const QXmppMessage &, const bool &)));
        QObject::connect(this, SIGNAL(addMsgTab(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &)),
                         gkXmppMsgTab, SLOT(openMsgDlg(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &)));
        QObject::connect(this, SIGNAL(addMucTab(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &)),
                         gkXmppMucTab, SLOT(openMucDlg(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &)));
        QObject::connect(this, SIGNAL(addMsgTab(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &)),
                         this, SLOT(openMsgTab(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &)));
        QObject::connect(this, SIGNAL(addMucTab(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &)),
                         this, SLOT(openMucTab(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &)));

        //
        // Setup and initialize signals and slots...
        QObject::connect(this, SIGNAL(sendXmppMsg(const QXmppMessage &)), m_xmppClient, SLOT(sendXmppMsg(const QXmppMessage &)));
        QObject::connect(m_xmppClient, SIGNAL(updateMsgHistory()), this, SLOT(updateMsgHistory()));
        QObject::connect(m_xmppClient, SIGNAL(msgArchiveSuccReceived()), this, SLOT(msgArchiveSuccReceived()));

        determineNickname();
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An error has occurred related to XMPP functions. The error in question:\n\n%1").arg(QString::fromStdString(e.what())),
                                    GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

GkXmppMessageDialog::~GkXmppMessageDialog()
{
    for (auto &archivedMsgsThread: m_archivedMsgsBulkThreadVec) {
        if (archivedMsgsThread.joinable()) {
            archivedMsgsThread.join();
        }
    }

    delete ui;
}

/**
 * @brief GkXmppMessageDialog::openMsgTab opens a new dialog from within this classes own UI, via the (Q)Xmpp roster
 * manager, so that the end-user may send/receive messages to other end-users.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msgRoster
 */
void GkXmppMessageDialog::openMsgTab(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &msgRoster)
{
    //
    // NOTE: If you call addTab() after show(), the layout system will try to adjust to the changes in its
    // widgets hierarchy and may cause flicker. To prevent this, you can set the QWidget::updatesEnabled property
    // to false prior to changes; remember to set the property to true when the changes are done, making the
    // widget receive paint events again.
    //
    quint16 max_idx = findLargestNum(gkTabMap);
    ++max_idx;

    gkTabMap.insert(max_idx, msgRoster);
    ui->tabWidget_chat_window->addTab(gkXmppMsgTab, QString::number(max_idx));

    return;
}

/**
 * @brief GkXmppMessageDialog::openMucTab opens a new dialog from within this classes own UI, via the (Q)Xmpp roster
 * manager, based upon a MUC so that the end-user may send/receive messages to others within the framework of a
 * chat-room.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param mucRoster
 */
void GkXmppMessageDialog::openMucTab(const GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster &mucRoster)
{
    //
    // NOTE: If you call addTab() after show(), the layout system will try to adjust to the changes in its
    // widgets hierarchy and may cause flicker. To prevent this, you can set the QWidget::updatesEnabled property
    // to false prior to changes; remember to set the property to true when the changes are done, making the
    // widget receive paint events again.
    //
    quint16 max_idx = findLargestNum(gkTabMap);
    ++max_idx;

    gkTabMap.insert(max_idx, mucRoster);
    ui->tabWidget_chat_window->addTab(gkXmppMucTab, QString::number(max_idx));

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
 * @brief GkXmppMessageDialog::submitMsgEnterKey processes the carriage return for UI element, `gkSpellCheckerHighlighter()`.
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
        qint32 ret = msgBox.exec();

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
        const auto plaintext = gkSpellCheckerHighlighter->toPlainText();
        if (!plaintext.isEmpty()) {
            for (const auto &bareJid: m_bareJids) {
                if (!bareJid.isEmpty()) {
                    const auto toMsg = createXmppMessageIq(bareJid, gkConnDetails.jid, plaintext);
                    if (toMsg.isXmppStanza()) {
                        emit sendXmppMsg(toMsg);
                    }
                }
            }
        }

        gkSpellCheckerHighlighter->clear();
    } else {
        m_netState = m_xmppClient->getNetworkState();
        if (m_netState == GkNetworkState::Connecting) {
            emit updateToolbar(tr("Attempting to make a connection... please wait..."));
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
        emit procMsgArchive(bareJid);
    }

    emit procMsgArchive(gkConnDetails.jid);

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
    QUuid uuid = QUuid::createUuid();
    QXmppMessage xmppMsg;
    xmppMsg.setId(uuid.toString());
    xmppMsg.setStamp(QDateTime::currentDateTimeUtc());
    xmppMsg.setFrom(from);
    xmppMsg.setTo(to);
    xmppMsg.setBody(message);
    xmppMsg.setState(QXmppMessage::Active); // User is actively participating in the chat session...
    xmppMsg.setPrivate(false);
    xmppMsg.setReceiptRequested(false);
    xmppMsg.setType(QXmppMessage::Chat);

    return xmppMsg;
}

/**
 * @brief GkXmppMessageDialog::msgArchiveSuccReceived
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::msgArchiveSuccReceived()
{
    gkEventLogger->publishEvent(tr("New message received!"), GkSeverity::Info, "", true, true, false, false, true);
    return;
}

/**
 * @brief GkXmppMessageDialog::getArchivedMessagesBulk
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppMessageDialog::dlArchivedMessages()
{
    for (const auto &bareJid: m_bareJids) {
        m_archivedMsgsBulkThreadVec.emplace_back(&GkXmppClient::getArchivedMessagesBulk, m_xmppClient, bareJid);
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::on_tabWidget_chat_window_tabCloseRequested closes the given, specified tab window.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index The specified tab window to close.
 * @see GkXmppMsgTab::closeMsgDlg(), GkXmppMucTab::closeMucDlg().
 */
void GkXmppMessageDialog::on_tabWidget_chat_window_tabCloseRequested(qint32 index)
{
    for (const auto &tab_idx: gkTabMap.toStdMap()) {
        if (tab_idx.first == index) {
            //
            // We have found an associated tab window's index!
            if (tab_idx.second.isMuc) {
                //
                // We are dealing with a MUC-style of chat session.
                ui->tabWidget_chat_window->removeTab(index);
                emit closeMucTab(m_xmppClient->getJidNickname(tab_idx.second.mucCtx.jid), index);
            } else {
                if (!tab_idx.second.roster.isEmpty()) {
                    //
                    // We are dealing with a one-on-one style of chat session.
                    ui->tabWidget_chat_window->removeTab(index);
                    emit closeMsgTab(tab_idx.second.roster.last().bareJid, index);
                } else {
                    gkEventLogger->publishEvent(tr("XMPP roster is unexpectedly empty! Must contain at least one element."),
                                                GkSeverity::Error, "", false, true, false, true, false);
                }
            }

            break;
        }
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::updateUsersHelper is simply a helper function for processing updates with regard to the
 * variable, `m_bareJids`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @see GkXmppMessageDialog::openMsgTab().
 */
void GkXmppMessageDialog::updateUsersHelper()
{
    try {
        if (!isVisible()) {
            setWindowFlags(Qt::Window);
            setAttribute(Qt::WA_DeleteOnClose, true);
            show();
        }

        if (!gkSpellCheckerHighlighter.isNull()) {
            if (m_xmppClient->isConnected()) {
                gkSpellCheckerHighlighter->setEnabled(true);
                dlArchivedMessages(); // Gather anything possible from the Google LevelDB database!
            }

            QObject::connect(m_xmppClient, &QXmppClient::connected, this, [=]() {
                gkSpellCheckerHighlighter->setEnabled(true);

                //
                // Now gather any data possible from the given XMPP server via the Internet!
                dlArchivedMessages();
            });
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkXmppMessageDialog::findLargestNum finds the largest possible number/index used within the, gkTabMap(), variable
 * so that it maybe safely incremented.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param tab_map
 * @return The largest possible index/number that was found within the, gkTabMap(), variable.
 */
quint16 GkXmppMessageDialog::findLargestNum(const QMap<quint16, GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster> &tab_map)
{
    if (!tab_map.isEmpty()) {
        std::vector<quint16> tmp_vec;
        for (const auto &tab_idx: tab_map.toStdMap()) {
            tmp_vec.emplace_back(tab_idx.first);
        }

        const quint16 max_idx = *std::max_element(tmp_vec.begin(), tmp_vec.end());
        return max_idx;
    }

    return 0;
}
