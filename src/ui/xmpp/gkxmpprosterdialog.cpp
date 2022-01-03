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
#include "src/ui/xmpp/gkxmppmessagedialog.hpp"
#include <qxmpp/QXmppPresence.h>
#include <utility>
#include <iterator>
#include <QMenu>
#include <QBuffer>
#include <QRegExp>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringList>
#include <QImageReader>
#include <QApplication>
#include <QDesktopWidget>
#include <QRegExpValidator>
#include <QStandardItemModel>
#include <QItemSelectionModel>

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

GkXmppRosterDialog::GkXmppRosterDialog(QPointer<GekkoFyre::StringFuncs> stringFuncs, const GkUserConn &connection_details,
                                       QPointer<GekkoFyre::GkXmppClient> xmppClient, QPointer<GekkoFyre::GkLevelDb> database,
                                       QPointer<GekkoFyre::GkSystem> system, QPointer<GkEventLogger> eventLogger,
                                       std::shared_ptr<QList<GkXmppCallsign>> rosterList, const bool &skipConnectionCheck,
                                       QWidget *parent) : shownXmppPreviewNotice(false), QDialog(parent),
                                       ui(new Ui::GkXmppRosterDialog)
{
    ui->setupUi(this);

    try {
        gkStringFuncs = std::move(stringFuncs);
        gkConnDetails = connection_details;
        m_xmppClient = std::move(xmppClient);
        gkDb = std::move(database);
        gkSystem = std::move(system);
        gkEventLogger = std::move(eventLogger);
        m_rosterList = std::move(rosterList);

        m_progressBar = new QProgressBar(nullptr);
        m_initAppLaunch = true;
        m_presenceManuallySet = false;
        m_rosterSearchEnabled = true;
        m_connectingInit = false;

        ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX);

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
        ui->actionUnblockUser->setEnabled(false);

        ui->pushButton_self_avatar->setEnabled(false);
        ui->lineEdit_search_roster->setEnabled(false);
        ui->actionEdit_Nickname->setEnabled(false);

        //
        // Message dialog signals/slots and actions!
        QPointer<GkXmppMessageDialog> gkXmppMsgDlg = new GkXmppMessageDialog(gkStringFuncs, gkEventLogger, gkDb, gkConnDetails,
                                                                             m_xmppClient, m_rosterList, this);
        QObject::connect(this, SIGNAL(launchMsgDlg(const QString &, const qint32 &)), gkXmppMsgDlg, SLOT(openMsgDlg(const QString &, const qint32 &)));
        QObject::connect(this, SIGNAL(launchMsgDlg(const QStringList &, const qint32 &)), gkXmppMsgDlg, SLOT(openMsgDlg(const QStringList &, const qint32 &)));

        QObject::connect(this, SIGNAL(updateClientVCard(const QString &, const QString &, const QString &, const QString &, const QByteArray &, const QString &)),
                         m_xmppClient, SLOT(updateClientVCardForm(const QString &, const QString &, const QString &, const QString &, const QByteArray &, const QString &)));
        QObject::connect(m_xmppClient, SIGNAL(savedClientVCard(const QByteArray &, const QString &)), this, SLOT(recvClientAvatarImg(const QByteArray &, const QString &)));
        QObject::connect(this, SIGNAL(updateClientAvatarImg(const QByteArray &, const QString &)), this, SLOT(updateClientAvatar(const QByteArray &, const QString &)));
        QObject::connect(m_xmppClient, SIGNAL(sendUserVCard(const QXmppVCardIq &)), this, SLOT(updateUserVCard(const QXmppVCardIq &)));

        QObject::connect(this, SIGNAL(updateAvailableStatusType(const QXmppPresence::AvailableStatusType &)),
                         this, SLOT(procAvailableStatusType(const QXmppPresence::AvailableStatusType &)));
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

        //
        // Try to remember the last presence setting, if the end-user has already made use of this dialog!
        const QString last_online_pres_str = gkDb->read_xmpp_settings(Settings::GkXmppCfg::XmppLastOnlinePresence);
        if (!last_online_pres_str.isEmpty()) {
            const qint32 last_online_pres_idx = last_online_pres_str.toInt();
            ui->comboBox_current_status->setCurrentIndex(last_online_pres_idx);
            m_presenceManuallySet = false;
        }

        QObject::connect(m_xmppClient, &GkXmppClient::connecting, this, [=]() {
            if (!m_connectingInit) {
                //
                // QProgressBar initialization (m_progressBar) and display within the center of the screen!
                QObject::connect(m_xmppClient, SIGNAL(updateProgressBar(const qint32 &)), m_progressBar, SLOT(setValue(const qint32 &)));
                QObject::connect(m_xmppClient, SIGNAL(updateProgressBar(const qint32 &)), this, SLOT(checkProgressBar(const qint32 &)));
                m_progressBar->setMinimum(GK_XMPP_CREATE_CONN_PROG_BAR_MIN_PERCT);
                m_progressBar->setMaximum(GK_XMPP_CREATE_CONN_PROG_BAR_MAX_PERCT);
                m_progressBar->setHidden(false);
                m_progressBar->setOrientation(Qt::Orientation::Horizontal);
                m_progressBar->setVisible(true);
                m_progressBar->setParent(this);
                ui->verticalLayout->addWidget(m_progressBar);
                m_progressBar->show();
                m_progressBar->move((QApplication::desktop()->width() - m_progressBar->width()) / 2,
                                    (QApplication::desktop()->height() - m_progressBar->height()) / 2);
                m_connectingInit = true;
            }
        });

        QObject::connect(m_xmppClient, &QXmppClient::connected, this, [=]() {
            ui->pushButton_self_avatar->setEnabled(true);
            ui->lineEdit_search_roster->setEnabled(true);
            ui->actionEdit_Nickname->setEnabled(true);
            if (m_presenceManuallySet) {
                if (ui->comboBox_current_status->currentIndex() == GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX) {
                    ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_AVAILABLE_IDX);
                }
            }
        });

        QObject::connect(m_xmppClient, &QXmppClient::disconnected, this, [=]() {
            m_connectingInit = false;
            ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX); // Change presence status to 'offline' upon disconnection!
            ui->pushButton_self_avatar->setEnabled(false);
            ui->lineEdit_search_roster->setEnabled(false);
            ui->actionEdit_Nickname->setEnabled(false);

            ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX);
            cleanupTables();
        });

        //
        // Setup and initialize QTableView's...
        gkXmppPresenceTreeViewModel = new QStandardItemModel(this);
        gkXmppPendingTableViewModel = new GkXmppRosterPendingTableViewModel(ui->tableView_callsigns_pending, m_xmppClient, this);
        gkXmppBlockedTableViewModel = new GkXmppRosterBlockedTableViewModel(ui->tableView_callsigns_blocked, m_xmppClient, this);

        //
        // Users and presence
        m_presenceTodayContacts = new QStandardItem(tr("Today")); // For users that have made activity applicable to today's history!
        ui->tableView_callsigns_groups->setModel(gkXmppPresenceTreeViewModel);
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

        QObject::connect(this, SIGNAL(updatePresenceTableViewModel()), this, SLOT(recvUpdatePresenceTableViewModel()));
        QObject::connect(this, SIGNAL(updatePendingTableViewModel()), this, SLOT(recvUpdatePendingTableViewModel()));
        QObject::connect(this, SIGNAL(updateBlockedTableViewModel()), this, SLOT(recvUpdateBlockedTableViewModel()));

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

        //
        // See GkXmppRosterDialog::eventFilter() for further details!
        installEventFilter(this);
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

GkXmppRosterDialog::~GkXmppRosterDialog()
{
    cleanupTables();

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
    if (!bareJid.isEmpty() && !m_rosterList->isEmpty()) {
        for (const auto &entry: *m_rosterList) {
            if (entry.bareJid == bareJid) {
                if (!reason.isEmpty()) {
                    insertRosterPendingTable(m_xmppClient->presenceToIcon(entry.presence->availableStatusType()), bareJid, entry.vCard.nickName());
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
 * @brief GkXmppRosterDialog::addJidToRoster is executed when the roster entry of a particular bareJid is added as a
 * result of roster push.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 */
void GkXmppRosterDialog::addJidToRoster(const QString &bareJid)
{
    if (!bareJid.isEmpty() && !m_rosterList->isEmpty()) {
        for (const auto &entry: *m_rosterList) {
            if (entry.bareJid == bareJid) {
                if (m_xmppClient->isJidOnline(bareJid)) {
                    insertRosterPresenceTable(m_presenceTodayContacts, m_xmppClient->presenceToIcon(entry.presence->availableStatusType()),
                                              bareJid, entry.vCard.nickName());
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
    if (!bareJid.isEmpty() && !m_rosterList->isEmpty()) {
        for (const auto &entry: *m_rosterList) {
            if (entry.bareJid == bareJid) {
                removeRosterPresenceTable(bareJid);
                break;
            }
        }
    }

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
    delJidFromRoster(bareJid);
    return;
}

/**
 * @brief GkXmppRosterDialog::updateActions
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Editable Tree Model Example <https://doc.qt.io/qt-5/qtwidgets-itemviews-editabletreemodel-example.html>.
 */
void GkXmppRosterDialog::updateActions()
{
    if (ui->tableView_callsigns_groups->isActiveWindow() && !m_presenceRosterData.empty()) {
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
 * @param row At which row you wish to insert the new data. Optional variable.
 */
void GkXmppRosterDialog::insertRosterPresenceTable(QStandardItem *parentRow, const QIcon &presence, const QString &bareJid,
                                                   const QString &nickname, const qint32 row)
{
    if (!bareJid.isEmpty() || !nickname.isEmpty()) {
        GkPresenceTableViewModel presence_model;
        presence_model.presence = presence;
        presence_model.bareJid = bareJid;
        presence_model.nickName = nickname;

        std::vector<qint32> idx_vals;
        if (!m_presenceRosterData.empty()) {
            for (const auto &presence: m_presenceRosterData) {
                //
                // We can use pre-existing rows/indexes!
                idx_vals.push_back(presence.second.row);
            }

            idx_vals.push_back(row); // Also add the newly seen value!
        } else {
            //
            // We need to create a row/index from scratch!
            idx_vals.push_back(-1);
        }

        const auto incr_idx_val = gkStringFuncs->incrementIndexVal(idx_vals);
        presence_model.row = incr_idx_val;
        presence_model.added = false;

        QStandardItem *firstCell = new QStandardItem(presence, bareJid);
        QStandardItem *secondCell = new QStandardItem(presence, nickname);
        gkXmppPresenceTreeViewModel->insertRow(gkXmppPresenceTreeViewModel->rowCount(parentRow->index())); // Insert an empty row!
        gkXmppPresenceTreeViewModel->setItem(gkXmppPresenceTreeViewModel->rowCount() - 1, 0, nullptr);
        gkXmppPresenceTreeViewModel->setItem(gkXmppPresenceTreeViewModel->rowCount() - 1, 1, firstCell);
        gkXmppPresenceTreeViewModel->setItem(gkXmppPresenceTreeViewModel->rowCount() - 1, 2, secondCell);

        emit updatePresenceTableViewModel();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::removeRosterPresenceTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @return The child row at which the data was removed from.
 */
qint32 GkXmppRosterDialog::removeRosterPresenceTable(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        qint32 ret = 0;
        for (const auto &rosterPresence: m_presenceRosterData) {
            if (rosterPresence.second.bareJid == bareJid) {
                gkXmppPresenceTreeViewModel->removeRow(rosterPresence.second.row);
            }
        }

        for (auto iter = m_presenceRosterData.begin(); iter != m_presenceRosterData.end(); ++iter) {
            if (iter->second.bareJid == bareJid) {
                iter = m_presenceRosterData.erase(iter);
                break;
            }
        }

        return ret - 1;
    }

    return -1;
}

/**
 * @brief GkXmppRosterDialog::updateRosterPresenceTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param presence
 * @param bareJid
 * @param nickname
 */
void GkXmppRosterDialog::updateRosterPresenceTable(const QIcon presence, const QString bareJid, const QString nickname)
{
    if (!bareJid.isEmpty()) {
        qint32 ret = removeRosterPresenceTable(bareJid);
        insertRosterPresenceTable(m_presenceTodayContacts, presence, bareJid, nickname, ret);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::insertRosterPendingTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param online_status The requesting user's online availability and status (i.e. presence).
 * @param bareJid The requesting user's identification.
 * @param nickname The requesting user's nickname, if any.
 * @param row At which row you wish to insert the new data. Optional variable.
 */
void GkXmppRosterDialog::insertRosterPendingTable(const QIcon &online_status, const QString &bareJid, const QString &nickname,
                                                  const qint32 row)
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
 * @param row At which row you wish to insert the new data. Optional variable.
 */
void GkXmppRosterDialog::insertRosterPendingTable(const QIcon &online_status, const QString &bareJid, const QString &nickname,
                                                  const QString &reason, const qint32 row)
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
 * @return The row at which the data was removed from.
 */
qint32 GkXmppRosterDialog::removeRosterPendingTable(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        qint32 ret = 0;
        for (const auto &entry: m_pendingRosterData) {
            if (entry.bareJid == bareJid) {
                ret = gkXmppPendingTableViewModel->removeData(bareJid);
                break;
            }
        }

        for (auto iter = m_pendingRosterData.begin(); iter != m_pendingRosterData.end(); ++iter) {
            if (iter->bareJid == bareJid) {
                iter = m_pendingRosterData.erase(iter);
                break;
            }
        }

        return ret;
    }

    return -1;
}

/**
 * @brief GkXmppRosterDialog::updateRosterPendingTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param online_status
 * @param bareJid
 * @param nickname
 * @param reason
 */
void GkXmppRosterDialog::updateRosterPendingTable(const QIcon &online_status, const QString &bareJid, const QString &nickname, const QString &reason)
{
    if (!bareJid.isEmpty()) {
        qint32 ret = removeRosterPendingTable(bareJid);
        if (!reason.isEmpty()) {
            insertRosterPendingTable(online_status, bareJid, nickname, reason, ret);
            return;
        }

        insertRosterPendingTable(online_status, bareJid, nickname, ret);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::insertRosterBlockedTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @param reason
 * @param row At which row you wish to insert the new data. Optional variable.
 */
void GkXmppRosterDialog::insertRosterBlockedTable(const QString &bareJid, const QString &reason, const qint32 row)
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
 * @return The row at which the data was removed from.
 */
qint32 GkXmppRosterDialog::removeRosterBlockedTable(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        qint32 ret = 0;
        for (const auto &entry: m_blockedRosterData) {
            if (entry.bareJid == bareJid) {
                ret = gkXmppBlockedTableViewModel->removeData(bareJid);
                break;
            }
        }

        for (auto iter = m_blockedRosterData.begin(); iter != m_blockedRosterData.end(); ++iter) {
            if (iter->bareJid == bareJid) {
                iter = m_blockedRosterData.erase(iter);
                break;
            }
        }

        return ret;
    }

    return -1;
}

/**
 * @brief GkXmppRosterDialog::updateRosterBlockedTable
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @param reason
 */
void GkXmppRosterDialog::updateRosterBlockedTable(const QString &bareJid, const QString &reason)
{
    if (!bareJid.isEmpty()) {
        qint32 ret = removeRosterBlockedTable(bareJid);
        insertRosterBlockedTable(bareJid, reason, ret);
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

    ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX);
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
    if (!m_initAppLaunch) {
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

        m_presenceManuallySet = true;
    }

    m_initAppLaunch = false;
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

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionDelete_Contact_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionDelete_Contact_triggered()
{
    QModelIndex index = ui->tableView_callsigns_groups->indexAt(ui->actionDelete_Contact->data().toPoint());
    QModelIndex retrieve = gkXmppPresenceTreeViewModel->index(index.row(), GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
    QVariant data = ui->tableView_callsigns_groups->model()->data(retrieve);
    QString username = data.toString();

    if (!username.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Unsubscribe from presence for user, \"%1\"?").arg(username));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                m_xmppClient->unsubscribeToUser(m_xmppClient->addHostname(username));
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_pushButton_self_avatar_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_pushButton_self_avatar_clicked()
{
    try {
        const QString saved_path = gkDb->read_xmpp_recall(Settings::GkXmppRecall::XmppAvatarFolderDir);
        const QString ret_path = m_xmppClient->obtainAvatarFilePath(saved_path);
        if (!ret_path.isEmpty()) {
            const QFileInfo filePath = ret_path;
            gkDb->write_xmpp_recall(filePath.canonicalPath(), Settings::GkXmppRecall::XmppAvatarFolderDir);
            QByteArray avatarByteArray = m_xmppClient->processImgToByteArray(filePath);
            if (!avatarByteArray.isNull()) {
                if (!avatarByteArray.isEmpty()) {
                    const auto mime_type = gkSystem->getImgFormat(avatarByteArray, true);
                    const QString mime_type_uppercase = mime_type.toUpper();

                    //
                    // Determine if the image data needs converting to JPEG or not!
                    if (mime_type_uppercase == "JPEG" || mime_type_uppercase == "JPG") {
                        //
                        // Already a JPEG image! So we can proceed as normal...
                        //
                        m_clientAvatarImgBa = avatarByteArray;
                    } else {
                        //
                        // Convert the image to JPEG!
                        // https://doc.qt.io/qt-5/qimage.html#save
                        // https://doc.qt.io/qt-5/qtimageformats-index.html
                        //
                        QImage convert;
                        QBuffer buffer(&avatarByteArray);
                        buffer.open(QIODevice::WriteOnly);
                        if (!convert.save(&buffer, General::Xmpp::Avatar::defaultAvatarFormatSuffix, 85)) {
                            throw std::runtime_error(tr("There has been an error with converting your avatar from, \"%1\", towards, \"%2\"!")
                            .arg(mime_type_uppercase, General::Xmpp::Avatar::defaultAvatarFormatSuffix).toStdString());
                        }

                        buffer.seek(0);
                        QByteArray conv_array = buffer.readAll();
                        m_clientAvatarImgBa = conv_array;
                    }

                    emit updateClientVCard(gkConnDetails.firstName, gkConnDetails.lastName, gkConnDetails.email, gkConnDetails.nickname, avatarByteArray,
                                           General::Xmpp::Avatar::defaultAvatarFormat);
                    emit updateClientAvatarImg(m_clientAvatarImgBa, General::Xmpp::Avatar::defaultAvatarFormatSuffix);

                    return;
                }
            }
        }
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
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
 * @param avatar_pic An avatar picture that the connecting client might wish to upload for others to see and identify them.
 * @param img_type The image format that the avatar is originally within (i.e. PNG, JPEG, GIF, etc).
 */
void GkXmppRosterDialog::recvClientAvatarImg(const QByteArray &avatar_pic, const QString &img_type)
{
    if (!avatar_pic.isNull()) {
        if (!avatar_pic.isEmpty()) {
            emit updateClientAvatarImg(avatar_pic, img_type);
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::defaultClientAvatarPlaceholder
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::defaultClientAvatarPlaceholder()
{
    m_clientAvatarImgBa = gkDb->read_xmpp_settings(Settings::GkXmppCfg::XmppAvatarByteArray).toUtf8();
    if (!m_clientAvatarImgBa.isNull()) {
        if (!m_clientAvatarImgBa.isEmpty()) {
            const auto mime_type = gkSystem->getImgFormat(m_clientAvatarImgBa, true);
            emit updateClientAvatarImg(m_clientAvatarImgBa, mime_type);
        } else {
            QImage img_to_save = QImage(":/resources/contrib/images/raster/gekkofyre-networks/CurioDraco/gekkofyre_drgn_server_thumb_transp.png");
            QByteArray arr;
            QBuffer buf(&arr);
            buf.open(QIODevice::WriteOnly);
            img_to_save.save(&buf, "PNG");
            if (!arr.isNull()) {
                if (!arr.isEmpty()) {
                    emit updateClientAvatarImg(arr, "PNG");
                }
            }
        }
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::updateClientAvatar
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param avatar_img An avatar picture that the connecting client might wish to upload for others to see and identify them.
 * @param img_type The image format that the avatar is originally within (i.e. PNG, JPEG, GIF, etc).
 */
void GkXmppRosterDialog::updateClientAvatar(const QByteArray &avatar_img, const QString &img_type)
{
    try {
        if (!avatar_img.isNull()) {
            if (!avatar_img.isEmpty()) {
                const auto pixmap = m_xmppClient->rescaleAvatarImg(avatar_img, img_type);
                ui->pushButton_self_avatar->setIcon(QIcon(pixmap));
                ui->pushButton_self_avatar->setIconSize(QSize(150, 150));

                //
                // Only update the Google LevelDB database with an avatar image if connected!
                if (m_xmppClient->isConnected() && m_xmppClient->getNetworkState() != GkNetworkState::Connecting) {
                    gkDb->write_xmpp_settings(avatar_img, Settings::GkXmppCfg::XmppAvatarByteArray);
                    gkEventLogger->publishEvent(tr("vCard avatar has been registered for self-client."), GkSeverity::Debug, "", false, true, false, false);
                }
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::updateUserVCard processes roster entries for `ui->tableView_callsigns_groups()` and its
 * model, `gkXmppPresenceTreeViewModel()`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param vCard
 * @see ui->tableView_callsigns_groups(), GkXmppRosterDialog::gkXmppPresenceTreeViewModel()
 */
void GkXmppRosterDialog::updateUserVCard(const QXmppVCardIq &vCard)
{
    GkPresenceTableViewModel presence_model;
    presence_model.bareJid = vCard.from();
    presence_model.added = false;

    if (!m_rosterList->isEmpty()) {
        for (const auto &entry: *m_rosterList) {
            if (!entry.bareJid.isEmpty() && !presence_model.bareJid.isEmpty()) {
                if (entry.bareJid == presence_model.bareJid) {
                    presence_model.presence = m_xmppClient->presenceToIcon(entry.presence->availableStatusType());
                    if (!vCard.nickName().isEmpty()) {
                        presence_model.nickName = vCard.nickName();
                    }

                    if (!vCard.fullName().isEmpty() && presence_model.nickName.isEmpty()) {
                        presence_model.nickName = vCard.fullName();
                    }

                    if (!vCard.email().isEmpty() && presence_model.nickName.isEmpty()) {
                        presence_model.nickName = vCard.email();
                    }
                }

                if (!m_presenceRosterData.empty()) {
                    for (auto iter = m_presenceRosterData.begin(); iter != m_presenceRosterData.end(); ++iter) {
                        if (!iter->second.bareJid.isEmpty()) {
                            if (entry.bareJid == iter->second.bareJid) {
                                iter->second.nickName = presence_model.nickName;
                                updateRosterPresenceTable(m_xmppClient->presenceToIcon(m_xmppClient->getBareJidPresence(iter->second.bareJid).availableStatusType()), iter->second.bareJid, iter->second.nickName);
                                break;
                            }
                        }

                        break;
                    }

                    for (auto iter = m_pendingRosterData.begin(); iter != m_pendingRosterData.end(); ++iter) {
                        if (!iter->bareJid.isEmpty()) {
                            if (entry.bareJid == iter->bareJid) {
                                iter->nickName = presence_model.nickName;
                                updateRosterPendingTable(m_xmppClient->presenceToIcon(m_xmppClient->getBareJidPresence(iter->bareJid).availableStatusType()), iter->bareJid, iter->nickName);
                                break;
                            }
                        }

                        break;
                    }
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
        m_rosterSearchEnabled = false;
        ui->lineEdit_search_roster->setVisible(true);
        ui->lineEdit_search_roster->setPlaceholderText(tr("Change nickname..."));
        ui->lineEdit_search_roster->setText(ui->label_self_nickname->text());
        ui->lineEdit_search_roster->setFocus();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_lineEdit_search_roster_returnPressed
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_lineEdit_search_roster_returnPressed()
{
    if (!m_rosterSearchEnabled) {
        //
        // Nickname edit mode...
        QString newNickname = ui->lineEdit_search_roster->text();
        if (!newNickname.isEmpty()) {
            // Apply the new nickname!
            const auto mime_type = gkSystem->getImgFormat(m_clientAvatarImgBa, true);
            emit updateClientVCard(gkConnDetails.firstName, gkConnDetails.lastName, gkConnDetails.email, newNickname, m_clientAvatarImgBa, mime_type);

            //
            // Return back to default settings...
            ui->lineEdit_search_roster->setPlaceholderText(tr("Search..."));
            ui->lineEdit_search_roster->clear();
            m_rosterSearchEnabled = true;
            return;
        }
    } else {
        //
        // Roster-search mode...
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
    QModelIndex retrieve = gkXmppPresenceTreeViewModel->index(index.row(), GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
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
    QModelIndex retrieve = gkXmppPresenceTreeViewModel->index(index.row(), GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX, QModelIndex());
    QString username = retrieve.data().toString();
    if (!username.isEmpty()) {
        enablePresenceTableActions(true);
        QString bareJid = m_xmppClient->addHostname(username);
        if (m_xmppClient->isJidExist(bareJid)) {
            emit launchMsgDlg(bareJid, GK_XMPP_MSG_WINDOW_NEW_TAB_IDX); // TODO: Improve upon this so it works as it should!
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
            emit launchMsgDlg(bareJid, GK_XMPP_MSG_WINDOW_NEW_TAB_IDX); // TODO: Improve upon this so it works as it should!
            return;
        }

        return;
    }

    enableBlockedTableActions(false);
    return;
}

/**
 * @brief GkXmppRosterDialog::procAvailableStatusType
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param stat_type
 */
void GkXmppRosterDialog::procAvailableStatusType(const QXmppPresence::AvailableStatusType &stat_type)
{
    if (!m_xmppClient->isConnected() && m_xmppClient->getNetworkState() != GkNetworkState::Connecting) {
        reconnectToXmpp();
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::cleanupTables is intended to clean-up all the various QTableViews upon exit of this QDialog, for
 * example.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @see ui->tableView_callsigns_groups(), ui->tableView_callsigns_pending(), ui->tableView_callsigns_blocked()
 */
void GkXmppRosterDialog::cleanupTables()
{
    if (!m_presenceRosterData.empty()) {
        for (const auto &entry: m_presenceRosterData) {
            removeRosterPresenceTable(entry.second.bareJid);
        }
    }

    if (!m_pendingRosterData.isEmpty()) {
        for (const auto &entry: m_pendingRosterData) {
            removeRosterPendingTable(entry.bareJid);
        }
    }

    if (!m_blockedRosterData.isEmpty()) {
        for (const auto &entry: m_blockedRosterData) {
            removeRosterBlockedTable(entry.bareJid);
        }
    }

    m_presenceRosterData.clear();
    m_pendingRosterData.clear();
    m_blockedRosterData.clear();

    return;
}

/**
 * @brief GkXmppRosterDialog::checkProgressBar checks the given QProgressBar, usually `m_progressBar` in this case, to
 * see if it has reached the designated maximum value and if so, make the UI widget now hidden so the user can no longer
 * see it anymore. The QProgressBar has thusly served its purpose for now.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param percentage The current value that the QProgressBar is set at.
 */
void GkXmppRosterDialog::checkProgressBar(const qint32 &percentage)
{
    if (percentage >= m_progressBar->maximum()) {
        m_progressBar->setVisible(false);
        m_progressBar->setHidden(true);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::eventFilter
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param obj
 * @param event
 * @return
 */
bool GkXmppRosterDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Close) {  // QEvent::Hide is another potential candidate!
        if (qobject_cast<QObject *>(this) == obj) {
            gkDb->write_xmpp_settings(QString::number(ui->comboBox_current_status->currentIndex()), Settings::GkXmppCfg::XmppLastOnlinePresence);
        }
    }

    return QObject::eventFilter(obj, event);
}

/**
 * @brief GkXmppRosterDialog::recvUpdatePresenceTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::recvUpdatePresenceTableViewModel()
{
    for (auto iter = m_presenceRosterData.begin(); iter != m_presenceRosterData.end(); ++iter) {
        if (!iter->second.bareJid.isEmpty()) {
            if (!iter->second.added) {
                iter->second.added = true;
            }
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
    if (ui->tableView_callsigns_groups->isActiveWindow() && !m_presenceRosterData.empty()) {
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
