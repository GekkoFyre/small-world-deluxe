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

#include "gkxmpprosterdialog.hpp"
#include "ui_gkxmpprosterdialog.h"
#include "src/ui/xmpp/gkxmppregistrationdialog.hpp"
#include <qxmpp/QXmppPresence.h>
#include <utility>
#include <QMenu>
#include <QBuffer>
#include <QRegExp>
#include <QMessageBox>
#include <QStringList>
#include <QImageReader>
#include <QRegExpValidator>

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

GkXmppRosterDialog::GkXmppRosterDialog(const GkUserConn &connection_details, QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                       QPointer<GekkoFyre::GkLevelDb> database, std::shared_ptr<nuspell::Dictionary> nuspellDict,
                                       QPointer<GkEventLogger> eventLogger, const bool &skipConnectionCheck, QWidget *parent) :
                                       shownXmppPreviewNotice(false), QDialog(parent), ui(new Ui::GkXmppRosterDialog)
{
    ui->setupUi(this);

    try {
        gkConnDetails = connection_details;
        m_xmppClient = std::move(xmppClient);
        gkDb = std::move(database);
        m_nuspellDict = std::move(nuspellDict);
        gkEventLogger = std::move(eventLogger);

        ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX);
        ui->comboBox_current_status->setEnabled(false);

        //
        // Disable tab-ordering for the following widgets...
        ui->lineEdit_self_nickname->setTabOrder(nullptr, nullptr);
        ui->pushButton_user_create_account->setTabOrder(nullptr, nullptr);
        ui->tableView_callsigns_blocked->setTabOrder(nullptr, nullptr);
        ui->pushButton_user_login->setTabOrder(nullptr, nullptr);
        ui->scrollArea_callsigns_blocked->setTabOrder(nullptr, nullptr);
        ui->scrollArea_add_new_contact->setTabOrder(nullptr, nullptr);
        ui->lineEdit_add_contact_username->setTabOrder(nullptr, nullptr);
        ui->pushButton_add_contact_submit->setTabOrder(nullptr, nullptr);
        ui->pushButton_add_contact_cancel->setTabOrder(nullptr, nullptr);

        ui->actionEdit_Contact->setEnabled(false);
        ui->actionDelete_Contact->setEnabled(false);
        ui->actionBlockPresenceUser->setEnabled(false);
        ui->actionBlockPendingUser->setEnabled(false);
        ui->actionAcceptInvite->setEnabled(false);
        ui->actionRefuseInvite->setEnabled(false);
        ui->actionAcceptInvite->setVisible(false);
        ui->actionRefuseInvite->setVisible(false);
        ui->actionUnblockUser->setEnabled(false);

        QObject::connect(this, SIGNAL(updateClientVCard(const QString &, const QString &, const QString &, const QString &, const QByteArray &)),
                         m_xmppClient, SLOT(updateClientVCardForm(const QString &, const QString &, const QString &, const QString &, const QByteArray &)));
        QObject::connect(m_xmppClient, SIGNAL(savedClientVCard(const QByteArray &)), this, SLOT(recvClientAvatarImg(const QByteArray &)));
        QObject::connect(this, SIGNAL(updateClientAvatarImg(const QImage &)), this, SLOT(updateClientAvatar(const QImage &)));
        QObject::connect(m_xmppClient, SIGNAL(sendUserVCard(const QXmppVCardIq &)), this, SLOT(updateUserVCard(const QXmppVCardIq &)));

        QObject::connect(this, SIGNAL(updateAvailableStatusType(const QXmppPresence::AvailableStatusType &)),
                         m_xmppClient, SLOT(modifyAvailableStatusType(const QXmppPresence::AvailableStatusType &)));

        QObject::connect(m_xmppClient, SIGNAL(sendSubscriptionRequest(const QString &)), this, SLOT(subscriptionRequestRecv(const QString &)));
        QObject::connect(m_xmppClient, SIGNAL(retractSubscriptionRequest(const QString &)), this, SLOT(subscriptionRequestRetracted(const QString &)));
        QObject::connect(this, SIGNAL(acceptSubscription(const QString &)), m_xmppClient, SLOT(acceptSubscriptionRequest(const QString &)));
        QObject::connect(this, SIGNAL(refuseSubscription(const QString &)), m_xmppClient, SLOT(refuseSubscriptionRequest(const QString &)));
        QObject::connect(this, SIGNAL(blockUser(const QString &)), m_xmppClient, SLOT(blockUser(const QString &)));
        QObject::connect(this, SIGNAL(unblockUser(const QString &)), m_xmppClient, SLOT(unblockUser(const QString &)));
        QObject::connect(m_xmppClient, SIGNAL(addJidToRoster(const QString &)), this, SLOT(addJidToRoster(const QString &)));
        QObject::connect(m_xmppClient, SIGNAL(delJidFromRoster(const QString &)), this, SLOT(delJidFromRoster(const QString &)));
        QObject::connect(m_xmppClient, SIGNAL(changeRosterJid(const QString &)), this, SLOT(changeRosterJid(const QString &)));

        defaultClientAvatarPlaceholder();
        ui->label_self_nickname->setText(tr("Anonymous"));
        if (!gkConnDetails.nickname.isEmpty()) {
            ui->label_self_nickname->setText(gkConnDetails.nickname);
        }

        QPointer<QRegExpValidator> rxUsernameAdd = new QRegExpValidator(this);
        QRegExp rxUsernameExp(R"(\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,4}\b)");
        rxUsernameExp.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
        rxUsernameAdd->setRegExp(rxUsernameExp);
        ui->lineEdit_add_contact_username->setValidator(rxUsernameAdd);

        //
        // Fill out the presence status ComboBox!
        prefillAvailComboBox();

        QObject::connect(m_xmppClient, &QXmppClient::connected, this, [=]() {
            //
            // Change the presence status to the last used once connected!
            ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_AVAILABLE_IDX); // TODO: Implement the Google LevelDB functions for this!
            ui->pushButton_self_avatar->setEnabled(true);
            ui->lineEdit_search_roster->setEnabled(true);
        });

        QObject::connect(m_xmppClient, &QXmppClient::disconnected, this, [=]() {
            //
            // Change the presence status to 'Offline / Unavailable' when disconnected!
            ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX);
            ui->pushButton_self_avatar->setEnabled(false);
            ui->lineEdit_search_roster->setEnabled(false);
        });

        //
        // Setup and initialize QTableView's...
        gkXmppPresenceTableViewModel = new GkXmppRosterPresenceTableViewModel(ui->tableView_callsigns_groups, m_xmppClient, this);
        gkXmppPendingTableViewModel = new GkXmppRosterPendingTableViewModel(ui->tableView_callsigns_pending, m_xmppClient, this);
        gkXmppBlockedTableViewModel = new GkXmppRosterBlockedTableViewModel(ui->tableView_callsigns_blocked, m_xmppClient, this);

        //
        // Users and presence
        ui->tableView_callsigns_groups->setModel(gkXmppPresenceTableViewModel);
        ui->tableView_callsigns_groups->horizontalHeader()->setSectionResizeMode(GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_PRESENCE_IDX, QHeaderView::ResizeToContents);
        ui->tableView_callsigns_groups->horizontalHeader()->setSectionResizeMode(GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX, QHeaderView::ResizeToContents);
        ui->tableView_callsigns_groups->horizontalHeader()->setStretchLastSection(true);
        ui->tableView_callsigns_groups->setVisible(true);
        ui->tableView_callsigns_groups->show();

        //
        // Contact requests
        ui->tableView_callsigns_pending->setModel(gkXmppPendingTableViewModel);
        ui->tableView_callsigns_pending->horizontalHeader()->setSectionResizeMode(GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_PRESENCE_IDX, QHeaderView::ResizeToContents);
        ui->tableView_callsigns_pending->horizontalHeader()->setSectionResizeMode(GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_BAREJID_IDX, QHeaderView::ResizeToContents);
        ui->tableView_callsigns_pending->horizontalHeader()->setStretchLastSection(true);
        ui->tableView_callsigns_pending->setVisible(true);
        ui->tableView_callsigns_pending->show();

        //
        // Blocked users
        ui->tableView_callsigns_blocked->setModel(gkXmppBlockedTableViewModel);
        ui->tableView_callsigns_blocked->horizontalHeader()->setSectionResizeMode(GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_BAREJID_IDX, QHeaderView::ResizeToContents);
        ui->tableView_callsigns_blocked->horizontalHeader()->setStretchLastSection(true);
        ui->tableView_callsigns_blocked->setVisible(true);
        ui->tableView_callsigns_blocked->show();

        QObject::connect(ui->tableView_callsigns_groups->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                         &GkXmppRosterDialog::updateActions);
        QObject::connect(m_xmppClient, SIGNAL(updateRoster()), this, SLOT(updateRoster()));

        QObject::connect(this, SIGNAL(updatePresenceTableViewModel()), this, SLOT(recvUpdatePresenceTableViewModel()));
        QObject::connect(this, SIGNAL(updatePendingTableViewModel()), this, SLOT(recvUpdatePendingTableViewModel()));
        QObject::connect(this, SIGNAL(updateBlockedTableViewModel()), this, SLOT(recvUpdateBlockedTableViewModel()));

        updateRoster();
        shownXmppPreviewNotice = gkDb->read_xmpp_alpha_notice();
        if (!shownXmppPreviewNotice) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Please read..."));
            msgBox.setText(tr("Do take note that the XMPP feature of %1 is very much in a %2 state as of this stage. Please see the "
                              "official website at [ %3 ] for further updates!")
                              .arg(General::productName).arg(General::xmppVersion).arg(General::officialWebsite));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Information);
            msgBox.exec();

            shownXmppPreviewNotice = true;
            gkDb->write_xmpp_alpha_notice(shownXmppPreviewNotice);
        }

        if (!skipConnectionCheck) {
            if (!m_xmppClient->isConnected()) {
                ui->frame_self_info->setVisible(false);
                ui->stackedWidget_roster_ui->setCurrentWidget(ui->page_login_or_create_account);
            } else {
                ui->stackedWidget_roster_ui->setCurrentWidget(ui->page_user_roster);
            }
        } else {
            ui->stackedWidget_roster_ui->setCurrentWidget(ui->page_user_roster);
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

GkXmppRosterDialog::~GkXmppRosterDialog()
{
    delete ui;
}

/**
 * @brief GkXmppRosterDialog::on_label_self_nickname_customContextMenuRequested brings up a context-menu for the
 * object, `ui->label_self_nickname()`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_label_self_nickname_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->label_self_nickname);
    contextMenu->addAction(ui->actionEdit_Nickname);

    updateActions();

    //
    // Save the position data to the QAction
    ui->actionEdit_Nickname->setData(QVariant(pos));
    contextMenu->exec(ui->label_self_nickname->mapToGlobal(pos));

    return;
}

/**
 * @brief GkXmppRosterDialog::subscriptionRequestRecv handles the processing and management of external subscription requests
 * to the given, connected towards XMPP server in question. This function will only work if Small World Deluxe is actively
 * connected towards an XMPP server at the time.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The external user's details.
 */
void GkXmppRosterDialog::subscriptionRequestRecv(const QString &bareJid)
{
    subscriptionRequestRecv(bareJid, QString());
    return;
}

/**
 * @brief GkXmppRosterDialog::subscriptionRequestRecv handles the processing and management of external subscription requests
 * to the given, connected towards XMPP server in question. This function will only work if Small World Deluxe is actively
 * connected towards an XMPP server at the time.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The external user's details.
 * @param reason The reason for making the subscription request, as defined by the external user.
 */
void GkXmppRosterDialog::subscriptionRequestRecv(const QString &bareJid, const QString &reason)
{
    if (!bareJid.isEmpty()) {
        updateRoster();
        for (const auto &entry: m_rosterList) {
            if (entry.bareJid == bareJid) {
                if (!reason.isEmpty()) {
                    insertRosterPendingTable(m_xmppClient->presenceToIcon(entry.presence->availableStatusType()), bareJid, entry.vCard.nickname);
                } else {
                    insertRosterPendingTable(m_xmppClient->presenceToIcon(entry.presence->availableStatusType()), bareJid, entry.vCard.nickname);
                }

                gkEventLogger->publishEvent(tr("A user of %1 with the nickname/callsign, \"%2\", is requesting to share presence details with you!")
                                            .arg(General::productName).arg(bareJid), GkSeverity::Info, "",
                                            true, true, false, false);
                break;
            }
        }

        updateActions();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::subscriptionRequestRetracted removes the given subscription request (if still active and
 * present) as requested by the external user.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The external user's details.
 */
void GkXmppRosterDialog::subscriptionRequestRetracted(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        removeRosterPendingTable(bareJid);
        updateActions();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::updateRoster
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::updateRoster()
{
    m_rosterList = m_xmppClient->getRosterMap();
    return;
}

/**
 * @brief GkXmppRosterDialog::addJidToRoster is executed when the roster entry of a particular bareJid is added as a
 * result of roster push.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppRosterDialog::addJidToRoster(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        updateRoster();
        for (const auto &entry: m_rosterList) {
            if (entry.bareJid == bareJid) {
                if (m_xmppClient->isJidOnline(bareJid)) {
                    insertRosterPresenceTable(m_xmppClient->presenceToIcon(entry.presence->availableStatusType()),
                                              bareJid, entry.vCard.nickname);
                } else {
                    insertRosterPresenceTable(m_xmppClient->presenceToIcon(entry.presence->availableStatusType()),
                                              bareJid, entry.vCard.nickname);
                }

                break;
            }
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::delJidFromRoster is executed when the roster entry of a particular bareJid is removed as a
 * result of roster push.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppRosterDialog::delJidFromRoster(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        for (const auto &entry: m_rosterList) {
            if (entry.bareJid == bareJid) {
                if (m_xmppClient->isJidOnline(bareJid)) {
                    removeRosterPresenceTable(bareJid);
                } else {
                    removeRosterPresenceTable(bareJid);
                }

                break;
            }
        }
    }

    updateRoster();
    return;
}

/**
 * @brief GkXmppRosterDialog::changeRosterJid is executed when the roster entry of a particular bareJid changes as a
 * result of roster push.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppRosterDialog::changeRosterJid(const QString &bareJid)
{
    return;
}

/**
 * @brief GkXmppRosterDialog::updateActions
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Editable Tree Model Example <https://doc.qt.io/qt-5/qtwidgets-itemviews-editabletreemodel-example.html>.
 */
void GkXmppRosterDialog::updateActions()
{
    if (ui->tableView_callsigns_groups->isActiveWindow() && !m_presenceRosterData.isEmpty()) {
        const bool presenceHasSelection = !ui->tableView_callsigns_groups->selectionModel()->selection().isEmpty();
        enablePresenceTableActions(presenceHasSelection);
    }

    if (ui->tableView_callsigns_pending->isActiveWindow() && !m_pendingRosterData.isEmpty()) {
        const bool pendingHasSelection = !ui->tableView_callsigns_pending->selectionModel()->selection().isEmpty();
        enablePendingTableActions(pendingHasSelection);
    }

    if (ui->tableView_callsigns_blocked->isActiveWindow() && !m_blockedRosterData.isEmpty()) {
        const bool blockedHasSelection = !ui->tableView_callsigns_blocked->selectionModel()->selection().isEmpty();
        enableBlockedTableActions(blockedHasSelection);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::insertRosterPresenceTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param presence
 * @param bareJid
 * @param nickname
 */
void GkXmppRosterDialog::insertRosterPresenceTable(const QIcon &presence, const QString &bareJid, const QString &nickname)
{
    if (!bareJid.isEmpty() || !nickname.isEmpty()) {
        GkPresenceTableViewModel presence_model;
        presence_model.presence = QIcon();
        if (!presence.isNull()) {
            presence_model.presence = presence;
        }

        presence_model.bareJid = bareJid;
        presence_model.nickName = nickname;
        presence_model.added = false;
        m_presenceRosterData.push_back(presence_model);
        emit updatePresenceTableViewModel();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::removeRosterPresenceTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppRosterDialog::removeRosterPresenceTable(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        updateRoster();
        for (const auto &entry: m_presenceRosterData) {
            if (entry.bareJid == bareJid) {
                gkXmppPresenceTableViewModel->removeData(bareJid);
                break;
            }
        }

        for (auto iter = m_presenceRosterData.begin(); iter != m_presenceRosterData.end();) {
            if (iter->bareJid == bareJid) {
                m_presenceRosterData.erase(iter);
                break;
            } else {
                ++iter;
            }
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::insertRosterPendingTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param online_status The requesting user's online availability and status (i.e. presence).
 * @param bareJid The requesting user's identification.
 * @param nickname The requesting user's nickname, if any.
 */
void GkXmppRosterDialog::insertRosterPendingTable(const QIcon &online_status, const QString &bareJid, const QString &nickname)
{
    insertRosterPendingTable(online_status, bareJid, nickname, QString());
    return;
}

/**
 * @brief GkXmppRosterDialog::insertRosterPendingTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param online_status The requesting user's online availability and status (i.e. presence).
 * @param bareJid The requesting user's identification.
 * @param nickname The requesting user's nickname, if any.
 * @param reason The requesting user's reason for making the subscription request.
 */
void GkXmppRosterDialog::insertRosterPendingTable(const QIcon &online_status, const QString &bareJid, const QString &nickname,
                                                  const QString &reason)
{
    if (!bareJid.isEmpty() || !nickname.isEmpty()) {
        GkPendingTableViewModel pending_model;
        pending_model.presence = QIcon(":/resources/contrib/images/raster/gekkofyre-networks/red-halftone-circle.png");
        if (!online_status.isNull()) {
            pending_model.presence = online_status;
        }

        pending_model.bareJid = bareJid;
        pending_model.nickName = nickname;
        pending_model.reason = "None specified.";
        if (reason.isEmpty()) {
            pending_model.reason = reason;
        }

        pending_model.added = false;
        m_pendingRosterData.push_back(pending_model);
        emit updatePendingTableViewModel();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::removeRosterPendingTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppRosterDialog::removeRosterPendingTable(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        updateRoster();
        for (const auto &entry: m_pendingRosterData) {
            if (entry.bareJid == bareJid) {
                gkXmppPendingTableViewModel->removeData(bareJid);
                break;
            }
        }

        for (auto iter = m_pendingRosterData.begin(); iter != m_pendingRosterData.end();) {
            if (iter->bareJid == bareJid) {
                m_pendingRosterData.erase(iter);
                break;
            } else {
                ++iter;
            }
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::insertRosterBlockedTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @param reason
 */
void GkXmppRosterDialog::insertRosterBlockedTable(const QString &bareJid, const QString &reason)
{
    if (bareJid.isEmpty()) {
        GkBlockedTableViewModel blocked_model;
        blocked_model.bareJid = bareJid;
        blocked_model.reason = reason;
        blocked_model.added = false;
        m_blockedRosterData.push_back(blocked_model);
        emit updateBlockedTableViewModel();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::removeRosterBlockedTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppRosterDialog::removeRosterBlockedTable(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        updateRoster();
        for (const auto &entry: m_blockedRosterData) {
            if (entry.bareJid == bareJid) {
                gkXmppBlockedTableViewModel->removeData(bareJid);
                break;
            }
        }

        for (auto iter = m_blockedRosterData.begin(); iter != m_blockedRosterData.end();) {
            if (iter->bareJid == bareJid) {
                m_blockedRosterData.erase(iter);
                break;
            } else {
                ++iter;
            }
        }
    }

    return;
}

/*
 * @brief GkXmppRosterDialog::reconnectToXmpp reconnects back to the given XMPP server, for if the user has been disconnected, either
 * intentionally or unintentionally.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::reconnectToXmpp()
{
    if (!m_xmppClient->isConnected() || m_xmppClient->getNetworkState() != GkNetworkState::Connecting) {
        m_xmppClient->createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, gkConnDetails.password,
                                               gkConnDetails.jid, false);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::launchMsgDlg
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The user we are in communique with!
 */
void GkXmppRosterDialog::launchMsgDlg(const QString &bareJid)
{
    gkXmppMsgDlg = new GkXmppMessageDialog(m_nuspellDict, m_xmppClient, bareJid, this);
    if (gkXmppMsgDlg) {
        if (!gkXmppMsgDlg->isVisible()) {
            gkXmppMsgDlg->setWindowFlags(Qt::Window);
            gkXmppMsgDlg->show();
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::prefillAvailComboBox
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::prefillAvailComboBox()
{
    // TODO: Insert an icon to the left within this QComboBox at a future date!
    ui->comboBox_current_status->insertItem(GK_XMPP_AVAIL_COMBO_AVAILABLE_IDX, QString("  %1").arg(tr("Online / Available")));
    ui->comboBox_current_status->setItemIcon(GK_XMPP_AVAIL_COMBO_AVAILABLE_IDX,
                                             QIcon(":/resources/contrib/images/raster/gekkofyre-networks/green-circle.png"));

    ui->comboBox_current_status->insertItem(GK_XMPP_AVAIL_COMBO_AWAY_FROM_KB_IDX, QString("  %1").arg(tr("Away / Be Back Soon")));
    ui->comboBox_current_status->setItemIcon(GK_XMPP_AVAIL_COMBO_AWAY_FROM_KB_IDX,
                                             QIcon(":/resources/contrib/images/raster/gekkofyre-networks/yellow-circle.png"));

    ui->comboBox_current_status->insertItem(GK_XMPP_AVAIL_COMBO_EXTENDED_AWAY_IDX, QString("  %1").arg(tr("XA / Extended Away")));
    ui->comboBox_current_status->setItemIcon(GK_XMPP_AVAIL_COMBO_EXTENDED_AWAY_IDX,
                                             QIcon(":/resources/contrib/images/raster/gekkofyre-networks/yellow-halftone-circle.png"));

    ui->comboBox_current_status->insertItem(GK_XMPP_AVAIL_COMBO_INVISIBLE_IDX, QString("  %1").arg(tr("Online / Invisible")));
    ui->comboBox_current_status->setItemIcon(GK_XMPP_AVAIL_COMBO_INVISIBLE_IDX,
                                             QIcon(":/resources/contrib/images/raster/gekkofyre-networks/green-halftone-circle.png"));

    ui->comboBox_current_status->insertItem(GK_XMPP_AVAIL_COMBO_BUSY_DND_IDX, QString("  %1").arg(tr("Busy / DND")));
    ui->comboBox_current_status->setItemIcon(GK_XMPP_AVAIL_COMBO_BUSY_DND_IDX,
                                             QIcon(":/resources/contrib/images/raster/gekkofyre-networks/red-circle.png"));

    ui->comboBox_current_status->insertItem(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX, QString("  %1").arg(tr("Offline / Unavailable")));
    ui->comboBox_current_status->setItemIcon(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX,
                                             QIcon(":/resources/contrib/images/raster/gekkofyre-networks/red-halftone-circle.png"));

    return;
}

/**
 * @brief GkXmppRosterDialog::on_comboBox_current_status_currentIndexChanged modify the availability and 'online status' of
 * the logged-in XMPP user, whether that be, "Available / Online", or, "Unavailable / Offline".
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRosterDialog::on_comboBox_current_status_currentIndexChanged(int index)
{
    if (!m_xmppClient->isConnected()) {
        switch (index) {
            case GK_XMPP_AVAIL_COMBO_AVAILABLE_IDX: // Online / Available
                emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::Online);
                break;
            case GK_XMPP_AVAIL_COMBO_AWAY_FROM_KB_IDX: // Away / Be Back Soon
                emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::Away);
                break;
            case GK_XMPP_AVAIL_COMBO_EXTENDED_AWAY_IDX: // XA / Extended Away
                emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::XA);
                break;
            case GK_XMPP_AVAIL_COMBO_INVISIBLE_IDX: // Online / Invisible
                emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::Invisible);
                break;
            case GK_XMPP_AVAIL_COMBO_BUSY_DND_IDX:
                emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::DND);
                break;
            case GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX: // Offline / Unavailable
                m_xmppClient->killConnectionFromServer(false);
                break;
            default:
                break;
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_pushButton_user_login_clicked process the login of an existing user to the given XMPP
 * server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_pushButton_user_login_clicked()
{
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountLogin, gkConnDetails, m_xmppClient, gkDb, gkEventLogger, this);
    gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
    gkXmppRegistrationDlg->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(gkXmppRegistrationDlg, SIGNAL(destroyed(QObject*)), this, SLOT(show()));
    gkXmppRegistrationDlg->show();

    return;
}

/**
 * @brief GkXmppRosterDialog::on_pushButton_user_create_account_clicked process the account creation of a new user to the
 * given XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_pushButton_user_create_account_clicked()
{
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountCreate, gkConnDetails, m_xmppClient, gkDb, gkEventLogger, this);
    gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
    gkXmppRegistrationDlg->show();

    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_groups_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void GkXmppRosterDialog::on_tableView_callsigns_groups_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->tableView_callsigns_groups);
    contextMenu->addAction(ui->actionAdd_Contact);
    contextMenu->addAction(ui->actionEdit_Contact);
    contextMenu->addAction(ui->actionDelete_Contact);
    contextMenu->addAction(ui->actionBlockPresenceUser);

    updateActions();

    //
    // Save the position data to the QAction
    ui->actionAdd_Contact->setData(QVariant(pos));
    ui->actionEdit_Contact->setData(QVariant(pos));
    ui->actionDelete_Contact->setData(QVariant(pos));
    ui->actionBlockPresenceUser->setData(QVariant(pos));

    contextMenu->exec(ui->tableView_callsigns_groups->mapToGlobal(pos));
    QModelIndex index = ui->tableView_callsigns_groups->indexAt(pos); // The exact item that the right-click has been made over!

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionAdd_Contact_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionAdd_Contact_triggered()
{
    ui->stackedWidget_roster_ui->setCurrentWidget(ui->page_add_new_contact);

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionEdit_Contact_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionEdit_Contact_triggered()
{
    QModelIndex idx = ui->tableView_callsigns_groups->indexAt(ui->actionEdit_Contact->data().toPoint());
    QVariant data = ui->tableView_callsigns_groups->model()->data(idx);
    QString text = data.toString();

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionDelete_Contact_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionDelete_Contact_triggered()
{
    QModelIndex idx = ui->tableView_callsigns_groups->indexAt(ui->actionDelete_Contact->data().toPoint());
    QVariant data = ui->tableView_callsigns_groups->model()->data(idx);
    QString text = data.toString();

    return;
}

/**
 * @brief GkXmppRosterDialog::on_pushButton_self_avatar_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_pushButton_self_avatar_clicked()
{
    QString filePath = m_xmppClient->obtainAvatarFilePath();
    if (!filePath.isEmpty()) {
        QByteArray avatarByteArray = m_xmppClient->processImgToByteArray(filePath);
        emit updateClientVCard(gkConnDetails.firstName, gkConnDetails.lastName, gkConnDetails.email, gkConnDetails.nickname, avatarByteArray);
        m_clientAvatarImg = avatarByteArray;
        QImage avatar_img = QImage::fromData(m_clientAvatarImg);
        emit updateClientAvatarImg(avatar_img);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_lineEdit_self_nickname_returnPressed
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @see GkXmppRosterDialog::editNicknameLabel().
 */
void GkXmppRosterDialog::on_lineEdit_self_nickname_returnPressed()
{
    ui->stackedWidget_self_nickname->setCurrentWidget(ui->page_self_nickname_label);
    QString entered_text = ui->lineEdit_self_nickname->text();
    ui->label_self_nickname->setText(tr("Anonymous"));
    if (!entered_text.isEmpty()) {
        ui->label_self_nickname->setText(entered_text);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_blocked_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void GkXmppRosterDialog::on_tableView_callsigns_blocked_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->tableView_callsigns_blocked);
    contextMenu->addAction(ui->actionUnblockUser);

    //
    // Save the position data to the QAction
    ui->actionUnblockUser->setData(QVariant(pos));

    contextMenu->exec(ui->tableView_callsigns_blocked->mapToGlobal(pos));
    return;
}

/**
 * @brief GkXmppRosterDialog::recvClientAvatarImg receives the current, set avatar image from the given XMPP server for
 * the logged-in client themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param avatar_pic
 */
void GkXmppRosterDialog::recvClientAvatarImg(const QByteArray &avatar_pic)
{
    if (!avatar_pic.isEmpty()) {
        m_clientAvatarImg = avatar_pic;
        QPointer<QBuffer> buffer = new QBuffer(this);
        buffer->setData(m_clientAvatarImg);
        buffer->open(QIODevice::ReadOnly);
        QImageReader imageReader(buffer);
        QImage image = imageReader.read();
        emit updateClientAvatarImg(image);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::defaultClientAvatarPlaceholder
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::defaultClientAvatarPlaceholder()
{
    m_clientAvatarImg = gkDb->read_xmpp_settings(Settings::GkXmppCfg::XmppAvatarByteArray).toUtf8();
    if (!m_clientAvatarImg.isEmpty()) {
        QImage avatar_img = QImage::fromData(QByteArray::fromBase64(m_clientAvatarImg));
        emit updateClientAvatarImg(avatar_img);
    } else {
        emit updateClientAvatarImg(QImage(":/resources/contrib/images/raster/gekkofyre-networks/CurioDraco/gekkofyre_drgn_server_thumb_transp.png"));
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::updateClientAvatar
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param avatar_img
 */
void GkXmppRosterDialog::updateClientAvatar(const QImage &avatar_img)
{
    ui->pushButton_self_avatar->setIcon(QIcon(QPixmap::fromImage(avatar_img)));
    ui->pushButton_self_avatar->setIconSize(QSize(150, 150));

    QByteArray ba;
    QPointer<QBuffer> buffer = new QBuffer(this);
    buffer->open(QIODevice::WriteOnly);
    avatar_img.save(buffer, "PNG");
    gkDb->write_xmpp_settings(buffer->readAll().toBase64(), Settings::GkXmppCfg::XmppAvatarByteArray);
    gkEventLogger->publishEvent(tr("vCard avatar has been registered for self-client."), GkSeverity::Debug, "", false, true, false, false);

    return;
}

/**
 * @brief GkXmppRosterDialog::updateUserVCard processes roster entries for `ui->tableView_callsigns_groups()` and its
 * model, `gkXmppPresenceTableViewModel()`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param vCard
 * @see ui->tableView_callsigns_groups(), GkXmppRosterDialog::gkXmppPresenceTableViewModel()
 */
void GkXmppRosterDialog::updateUserVCard(const QXmppVCardIq &vCard)
{
    updateRoster();
    GkPresenceTableViewModel presence_model;
    presence_model.bareJid = vCard.from();
    presence_model.added = false;

    if (!m_rosterList.isEmpty()) {
        for (const auto &entry: m_rosterList) {
            if (!entry.bareJid.isEmpty()) {
                if (entry.bareJid == presence_model.bareJid) {
                    presence_model.presence = m_xmppClient->presenceToIcon(entry.presence->availableStatusType());
                    if (vCard.nickName().isEmpty() && vCard.email().isEmpty() && vCard.fullName().isEmpty()) {
                        presence_model.nickName = "";
                        break;
                    }

                    if (!vCard.nickName().isEmpty()) {
                        presence_model.nickName = vCard.nickName();
                        break;
                    }

                    if (!vCard.email().isEmpty()) {
                        presence_model.nickName = vCard.email();
                        break;
                    }

                    if (!vCard.fullName().isEmpty()) {
                        presence_model.nickName = vCard.fullName();
                        break;
                    }

                    break;
                }
            }
        }
    }

    emit updatePresenceTableViewModel();
    return;
}

/**
 * @brief GkXmppRosterDialog::editNicknameLabel upon double-clicking the QLabel, ui->label_self_nickname(), the
 * QStackWidget, ui->stackedWidget_self_nickname(), will change towards, ui->lineEdit_self_nickname(), and allow the
 * editing of the connecting client's own nickname on the given XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The text as transferred from ui->label_self_nickname() prior to the double-clicking.
 * @see GkXmppRosterDialog::on_lineEdit_self_nickname_returnPressed().
 */
void GkXmppRosterDialog::editNicknameLabel(const QString &value)
{
    ui->stackedWidget_self_nickname->setCurrentWidget(ui->page_self_nickname_editable);
    if (!value.isEmpty()) {
        ui->lineEdit_self_nickname->setText(value);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_pushButton_add_contact_submit_clicked requests for an external JID to be added to the
 * connecting client's own roster via XMPP subscription.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_pushButton_add_contact_submit_clicked()
{
    QString username = ui->lineEdit_add_contact_username->text();
    QString reason = ui->lineEdit_add_contact_reason->text();
    if (!username.isEmpty()) {
        m_xmppClient->subscribeToUser(m_xmppClient->addHostname(username), reason); // Convert username into a bareJid!
    }

    ui->lineEdit_add_contact_username->clear();
    ui->lineEdit_add_contact_reason->clear();
    ui->stackedWidget_roster_ui->setCurrentWidget(ui->page_user_roster);

    return;
}

/**
 * @brief GkXmppRosterDialog::on_pushButton_add_contact_cancel_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_pushButton_add_contact_cancel_clicked()
{
    ui->lineEdit_add_contact_username->clear();
    ui->lineEdit_add_contact_reason->clear();
    ui->stackedWidget_roster_ui->setCurrentWidget(ui->page_user_roster);

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionAcceptInvite_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionAcceptInvite_triggered()
{
    if (!m_bareJidPendingSel.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Accept invite for user, \"%1\"?").arg(m_bareJidPendingSel));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                emit acceptSubscription(m_bareJidPendingSel);
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }

        updateActions();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionRefuseInvite_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionRefuseInvite_triggered()
{
    if (!m_bareJidPendingSel.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Refuse invite and decline communications for user, \"%1\"?").arg(m_bareJidPendingSel));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                emit refuseSubscription(m_bareJidPendingSel);
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }

        updateActions();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionBlockPendingUser_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionBlockPendingUser_triggered()
{
    if (!m_bareJidPendingSel.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Do you wish to block user, \"%1\"?").arg(m_bareJidPendingSel));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                emit blockUser(m_bareJidPendingSel);
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }

        updateActions();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionBlockPresenceUser_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionBlockPresenceUser_triggered()
{
    if (!m_bareJidPresenceSel.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Do you wish to block user, \"%1\"?").arg(m_bareJidPresenceSel));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                emit blockUser(m_bareJidPresenceSel);
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }

        updateActions();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionUnblockUser_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionUnblockUser_triggered()
{
    if (!m_bareJidBlockedSel.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Do you wish to unblock user, \"%1\", and allow possible communications once again?").arg(m_bareJidBlockedSel));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                emit unblockUser(m_bareJidBlockedSel);
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }

        updateActions();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionEdit_Nickname_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionEdit_Nickname_triggered()
{
    if (ui->lineEdit_search_roster->isEnabled()) {
        ui->lineEdit_search_roster->setVisible(true);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_lineEdit_search_roster_returnPressed
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_lineEdit_search_roster_returnPressed()
{
    QString newNickname = ui->lineEdit_search_roster->text();
    if (!newNickname.isEmpty()) {
        // Apply the new nickname!
        emit updateClientVCard(gkConnDetails.firstName, gkConnDetails.lastName, gkConnDetails.email, newNickname, m_clientAvatarImg);
        return;
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_lineEdit_search_roster_inputRejected
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_lineEdit_search_roster_inputRejected()
{
    if (ui->lineEdit_search_roster->isVisible()) {
        ui->lineEdit_search_roster->setVisible(false);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_pending_pressed
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRosterDialog::on_tableView_callsigns_pending_pressed(const QModelIndex &index)
{
    updateActions();
    QModelIndex retrieve = gkXmppPendingTableViewModel->index(index.row(), GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
    QString username = retrieve.data().toString();
    if (!username.isEmpty()) {
        enablePendingTableActions(true);
        m_bareJidPendingSel = m_xmppClient->addHostname(username);
        enablePendingTableActions(false);
        return;
    }

    enablePendingTableActions(false);
    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_pending_doubleClicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRosterDialog::on_tableView_callsigns_pending_doubleClicked(const QModelIndex &index)
{
    updateActions();
    QModelIndex retrieve = gkXmppPendingTableViewModel->index(index.row(), GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
    QString username = retrieve.data().toString();
    if (!username.isEmpty()) {
        enablePendingTableActions(true);
        m_bareJidPendingSel = m_xmppClient->addHostname(username);
        on_actionAcceptInvite_triggered();
        enablePendingTableActions(false);
        return;
    }

    enablePendingTableActions(false);
    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_pending_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void GkXmppRosterDialog::on_tableView_callsigns_pending_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->label_self_nickname);
    contextMenu->addAction(ui->actionAcceptInvite);
    contextMenu->addAction(ui->actionRefuseInvite);
    contextMenu->addAction(ui->actionBlockPendingUser);

    updateActions();

    //
    // Save the position data to the QAction
    ui->actionAcceptInvite->setData(QVariant(pos));
    ui->actionRefuseInvite->setData(QVariant(pos));
    ui->actionBlockPendingUser->setData(QVariant(pos));
    contextMenu->exec(ui->tableView_callsigns_pending->mapToGlobal(pos));

    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_groups_pressed
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRosterDialog::on_tableView_callsigns_groups_pressed(const QModelIndex &index)
{
    updateActions();
    QModelIndex retrieve = gkXmppPresenceTableViewModel->index(index.row(), GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
    QString username = retrieve.data().toString();
    if (!username.isEmpty()) {
        enablePresenceTableActions(true);
        QString bareJid = m_xmppClient->addHostname(username);
        if (m_xmppClient->isJidExist(bareJid)) {
            m_bareJidPresenceSel = bareJid;
            return;
        }

        return;
    }

    enablePresenceTableActions(false);
    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_groups_doubleClicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRosterDialog::on_tableView_callsigns_groups_doubleClicked(const QModelIndex &index)
{
    updateActions();
    QModelIndex retrieve = gkXmppPresenceTableViewModel->index(index.row(), GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
    QString username = retrieve.data().toString();
    if (!username.isEmpty()) {
        enablePresenceTableActions(true);
        QString bareJid = m_xmppClient->addHostname(username);
        if (m_xmppClient->isJidExist(bareJid)) {
            launchMsgDlg(username);
            return;
        }

        return;
    }

    enablePresenceTableActions(false);
    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_blocked_pressed
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRosterDialog::on_tableView_callsigns_blocked_pressed(const QModelIndex &index)
{
    updateActions();
    QModelIndex retrieve = gkXmppBlockedTableViewModel->index(index.row(), GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
    QString username = retrieve.data().toString();
    if (!username.isEmpty()) {
        enableBlockedTableActions(true);
        QString bareJid = m_xmppClient->addHostname(username);
        if (m_xmppClient->isJidExist(bareJid)) {
            m_bareJidBlockedSel = bareJid;
            return;
        }

        return;
    }

    enableBlockedTableActions(false);
    return;
}

/**
 * @brief GkXmppRosterDialog::on_tableView_callsigns_blocked_doubleClicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRosterDialog::on_tableView_callsigns_blocked_doubleClicked(const QModelIndex &index)
{
    updateActions();
    QModelIndex retrieve = gkXmppBlockedTableViewModel->index(index.row(), GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
    QString username = retrieve.data().toString();
    if (!username.isEmpty()) {
        enableBlockedTableActions(true);
        QString bareJid = m_xmppClient->addHostname(username);
        if (m_xmppClient->isJidExist(bareJid)) {
            launchMsgDlg(username);
            return;
        }

        return;
    }

    enableBlockedTableActions(false);
    return;
}

/**
 * @brief GkXmppRosterDialog::recvUpdatePresenceTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::recvUpdatePresenceTableViewModel()
{
    for (auto iter = m_presenceRosterData.begin(); iter != m_presenceRosterData.end(); ++iter) {
        if (!iter->added) {
            gkXmppPresenceTableViewModel->insertData(*iter);
            iter->added = true;
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::recvUpdatePendingTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::recvUpdatePendingTableViewModel()
{
    for (auto iter = m_pendingRosterData.begin(); iter != m_pendingRosterData.end(); ++iter) {
        if (!iter->added) {
            gkXmppPendingTableViewModel->insertData(*iter);
            iter->added = true;
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::recvUpdateBlockedTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::recvUpdateBlockedTableViewModel()
{
    for (auto iter = m_blockedRosterData.begin(); iter != m_blockedRosterData.end(); ++iter) {
        if (!iter->added) {
            gkXmppBlockedTableViewModel->insertData(*iter);
            iter->added = true;
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::enablePresenceTableActions
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param enable
 */
void GkXmppRosterDialog::enablePresenceTableActions(const bool &enable)
{
    if (ui->tableView_callsigns_groups->isActiveWindow() && !m_presenceRosterData.isEmpty()) {
        ui->actionEdit_Contact->setEnabled(enable);
        ui->actionDelete_Contact->setEnabled(enable);
        ui->actionBlockPendingUser->setEnabled(enable);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::enablePendingTableActions
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param enable
 */
void GkXmppRosterDialog::enablePendingTableActions(const bool &enable)
{
    if (ui->tableView_callsigns_pending->isActiveWindow() && !m_pendingRosterData.isEmpty()) {
        ui->actionAcceptInvite->setEnabled(enable);
        ui->actionRefuseInvite->setEnabled(enable);
        ui->actionAcceptInvite->setVisible(enable);
        ui->actionRefuseInvite->setVisible(enable);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::enableBlockedTableActions
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param enable
 */
void GkXmppRosterDialog::enableBlockedTableActions(const bool &enable)
{
    if (ui->tableView_callsigns_blocked->isActiveWindow() && !m_blockedRosterData.isEmpty()) {
        ui->actionUnblockUser->setEnabled(enable);
    }

    return;
}
