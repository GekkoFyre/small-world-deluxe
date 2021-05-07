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
                                       QPointer<GekkoFyre::GkLevelDb> database, QPointer<GkEventLogger> eventLogger,
                                       const bool &skipConnectionCheck, QWidget *parent) : shownXmppPreviewNotice(false),
                                       QDialog(parent), ui(new Ui::GkXmppRosterDialog)
{
    ui->setupUi(this);

    try {
        gkConnDetails = connection_details;
        m_xmppClient = std::move(xmppClient);
        gkDb = std::move(database);
        gkEventLogger = std::move(eventLogger);

        ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX);
        ui->comboBox_current_status->setEnabled(false);
        m_clientAvatarImgUpdateTimer = new QTimer(this);

        //
        // Disable tab-ordering for the following widgets...
        ui->lineEdit_self_nickname->setTabOrder(nullptr, nullptr);
        ui->pushButton_user_create_account->setTabOrder(nullptr, nullptr);
        ui->treeWidget_callsigns_blocked->setTabOrder(nullptr, nullptr);
        ui->pushButton_user_login->setTabOrder(nullptr, nullptr);
        ui->scrollArea_callsigns_blocked->setTabOrder(nullptr, nullptr);
        ui->scrollArea_add_new_contact->setTabOrder(nullptr, nullptr);
        ui->lineEdit_add_contact_username->setTabOrder(nullptr, nullptr);
        ui->pushButton_add_contact_submit->setTabOrder(nullptr, nullptr);
        ui->pushButton_add_contact_cancel->setTabOrder(nullptr, nullptr);

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

        QObject::connect(ui->label_self_nickname, SIGNAL(clicked(const QString &)), this, SLOT(editNicknameLabel(const QString &)));
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
            ui->comboBox_current_status->setEnabled(true);
        });

        QObject::connect(m_xmppClient, &QXmppClient::disconnected, this, [=]() {
            //
            // Change the presence status to 'Offline / Unavailable' when disconnected!
            ui->comboBox_current_status->setCurrentIndex(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX);
            ui->comboBox_current_status->setEnabled(false);
        });

        //
        // Users, presence, and contact requests
        ui->treeWidget_callsigns_groups->setHeaderLabels(QStringList() << tr("# / Presence") << tr("ID") << tr("Nickname"));
        ui->treeWidget_callsigns_groups->header()->setStretchLastSection(true);
        ui->treeWidget_callsigns_groups->header()->setSectionResizeMode(GK_XMPP_ROSTER_TREEWIDGET_MODEL_PRESENCE_IDX, QHeaderView::ResizeToContents);
        ui->treeWidget_callsigns_groups->header()->setSectionResizeMode(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX, QHeaderView::ResizeToContents);
        ui->treeWidget_callsigns_groups->header()->setSortIndicator(GK_XMPP_ROSTER_TREEWIDGET_MODEL_NICKNAME_IDX, Qt::SortOrder::DescendingOrder);
        ui->treeWidget_callsigns_groups->header()->setSortIndicatorShown(true);
        // ui->treeWidget_callsigns_groups->header()->hide();
        ui->treeWidget_callsigns_groups->setItemsExpandable(true);
        ui->treeWidget_callsigns_groups->setRootIsDecorated(true);
        ui->treeWidget_callsigns_groups->setVisible(true);
        ui->treeWidget_callsigns_groups->show();

        m_subRequests = std::make_shared<QTreeWidgetItem>(xmppRosterPresenceInsertTreeRoot("0", "", tr("Subscription Requests")));
        m_onlineUsers = std::make_shared<QTreeWidgetItem>(xmppRosterPresenceInsertTreeRoot("0", "", tr("Online")));
        m_offlineUsers = std::make_shared<QTreeWidgetItem>(xmppRosterPresenceInsertTreeRoot("0", "", tr("Offline")));

        //
        // Blocked users
        ui->treeWidget_callsigns_blocked->setHeaderLabels(QStringList() << tr("Nickname") << tr("Reason"));
        ui->treeWidget_callsigns_blocked->header()->setStretchLastSection(true);
        ui->treeWidget_callsigns_blocked->header()->setSectionResizeMode(GK_XMPP_ROSTER_BLOCKED_TREEWIDGET_MODEL_NICKNAME_IDX, QHeaderView::ResizeToContents);
        ui->treeWidget_callsigns_blocked->header()->setSortIndicator(GK_XMPP_ROSTER_BLOCKED_TREEWIDGET_MODEL_NICKNAME_IDX, Qt::SortOrder::DescendingOrder);
        ui->treeWidget_callsigns_blocked->header()->setSortIndicatorShown(true);
        // ui->treeWidget_callsigns_blocked->header()->hide();
        ui->treeWidget_callsigns_blocked->setItemsExpandable(true);
        ui->treeWidget_callsigns_blocked->setRootIsDecorated(true);
        ui->treeWidget_callsigns_blocked->setVisible(true);
        ui->treeWidget_callsigns_blocked->show();

        QObject::connect(ui->treeWidget_callsigns_groups->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                         &GkXmppRosterDialog::updateActions);
        QObject::connect(m_xmppClient, SIGNAL(updateRoster()), this, SLOT(updateRoster()));

        updateRoster();
        if (m_rosterList.isEmpty()) {
            xmppRosterPresenceInsertTreeChild(m_subRequests.get(), QIcon(), "", tr("Empty."));
            xmppRosterPresenceInsertTreeChild(m_onlineUsers.get(), QIcon(), "", tr("Empty."));
            xmppRosterPresenceInsertTreeChild(m_offlineUsers.get(), QIcon(), "", tr("Empty."));
        }

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
 * @brief GkXmppRosterDialog::subscriptionRequestRecv handles the processing and management of external subscription requests
 * to the given, connected towards XMPP server in question. This function will only work if Small World Deluxe is actively
 * connected towards an XMPP server at the time.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The external user's details.
 */
void GkXmppRosterDialog::subscriptionRequestRecv(const QString &bareJid)
{
    if (!bareJid.isEmpty()) {
        updateRoster();
        for (const auto &entry: m_rosterList) {
            if (entry.bareJid == bareJid) {
                xmppRosterPresenceRemoveTreeChild(m_subRequests.get(), tr("Empty."), GK_XMPP_ROSTER_TREEWIDGET_MODEL_NICKNAME_IDX);
                xmppRosterPresenceInsertTreeChild(m_subRequests.get(), m_xmppClient->presenceToIcon(entry.presence->availableStatusType()), bareJid, entry.vCard.nickname);
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
        xmppRosterPresenceRemoveTreeChild(m_subRequests.get(), bareJid, GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX);
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
 * @brief GkXmppRosterDialog::updateActions
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Editable Tree Model Example <https://doc.qt.io/qt-5/qtwidgets-itemviews-editabletreemodel-example.html>.
 */
void GkXmppRosterDialog::updateActions()
{
    if (ui->treeWidget_callsigns_groups->isActiveWindow()) {
        const bool presenceHasSelection = !ui->treeWidget_callsigns_groups->selectionModel()->selection().isEmpty();
        ui->actionEdit_Contact->setEnabled(presenceHasSelection);
        ui->actionDelete_Contact->setEnabled(presenceHasSelection);
        ui->actionBlockUser->setEnabled(presenceHasSelection);

    ui->actionAcceptInvite->setEnabled(presenceHasSelection);
    ui->actionRefuseInvite->setEnabled(presenceHasSelection);
    ui->actionAcceptInvite->setVisible(presenceHasSelection);
    ui->actionRefuseInvite->setVisible(presenceHasSelection);

    if (ui->treeWidget_callsigns_blocked->isActiveWindow()) {
        const bool blockedHasSelection = !ui->treeWidget_callsigns_blocked->selectionModel()->selection().isEmpty();
        ui->actionUnblockUser->setEnabled(blockedHasSelection);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::xmppRosterPresenceInsertTreeRoot
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param presence
 * @param bareJid
 * @param nickname
 * @return The QTreeWidgetItem object that was generated by this function.
 */
QTreeWidgetItem *GkXmppRosterDialog::xmppRosterPresenceInsertTreeRoot(const QString &presence, const QString &bareJid,
                                                                      const QString &nickname)
{
    QTreeWidgetItem *treeItem = new QTreeWidgetItem(ui->treeWidget_callsigns_groups);

    treeItem->setText(GK_XMPP_ROSTER_TREEWIDGET_MODEL_PRESENCE_IDX, presence);
    treeItem->setText(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX, bareJid);
    treeItem->setText(GK_XMPP_ROSTER_TREEWIDGET_MODEL_NICKNAME_IDX, nickname);

    return treeItem;
}

/**
 * @brief GkXmppRosterDialog::xmppRosterPresenceInsertTreeChild
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @param presence
 * @param bareJid
 * @param nickname
 */
void GkXmppRosterDialog::xmppRosterPresenceInsertTreeChild(QTreeWidgetItem *parent, const QIcon &presence, const QString &bareJid,
                                                           const QString &nickname)
{
    QTreeWidgetItem *treeItem = new QTreeWidgetItem();

    treeItem->setIcon(GK_XMPP_ROSTER_TREEWIDGET_MODEL_PRESENCE_IDX, presence);
    treeItem->setText(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX, bareJid);
    treeItem->setText(GK_XMPP_ROSTER_TREEWIDGET_MODEL_NICKNAME_IDX, nickname);
    parent->addChild(treeItem);

    return;
}

/**
 * @brief GkXmppRosterDialog::xmppRosterPresenceRemoveTreeChild
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @param desc
 * @param column
 */
void GkXmppRosterDialog::xmppRosterPresenceRemoveTreeChild(QTreeWidgetItem *parent, const QString &desc, const qint32 &column)
{
    QList<QTreeWidgetItem *> search = ui->treeWidget_callsigns_groups->findItems(desc, Qt::MatchFlag::MatchExactly, column);
    if (search.isEmpty()) {
        return;
    }

    for (const auto &item: search) {
        delete item;
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
        m_xmppClient->createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, gkConnDetails.username,
                                               gkConnDetails.password, gkConnDetails.jid, false);
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
    gkXmppMsgDlg = new GkXmppMessageDialog(m_xmppClient, bareJid, this);
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
                m_xmppClient->disconnectFromServer();
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
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountLogin, gkConnDetails, m_xmppClient, gkEventLogger, this);
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
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountCreate, gkConnDetails, m_xmppClient, gkEventLogger, this);
    gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
    gkXmppRegistrationDlg->show();

    return;
}

/**
 * @brief GkXmppRosterDialog::on_treeWidget_callsigns_groups_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void GkXmppRosterDialog::on_treeWidget_callsigns_groups_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->treeWidget_callsigns_groups);
    contextMenu->addAction(ui->actionAdd_Contact);
    contextMenu->addAction(ui->actionEdit_Contact);
    contextMenu->addAction(ui->actionDelete_Contact);

    contextMenu->addAction(ui->actionAcceptInvite);
    contextMenu->addAction(ui->actionRefuseInvite);
    contextMenu->addAction(ui->actionBlockUser);

    updateActions();

    //
    // Save the position data to the QAction
    ui->actionAdd_Contact->setData(QVariant(pos));
    ui->actionEdit_Contact->setData(QVariant(pos));
    ui->actionDelete_Contact->setData(QVariant(pos));

    ui->actionAcceptInvite->setData(QVariant(pos));
    ui->actionRefuseInvite->setData(QVariant(pos));
    ui->actionBlockUser->setData(QVariant(pos));

    contextMenu->exec(ui->treeWidget_callsigns_groups->mapToGlobal(pos));
    QModelIndex index = ui->treeWidget_callsigns_groups->indexAt(pos); // The exact item that the right-click has been made over!

    return;
}

/**
 * @brief GkXmppRosterDialog::on_treeWidget_callsigns_groups_itemClicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param item
 * @param column
 */
void GkXmppRosterDialog::on_treeWidget_callsigns_groups_itemClicked(QTreeWidgetItem *item, int column)
{
    updateActions();
    if (item == m_subRequests.get()) {
        ui->actionAcceptInvite->setEnabled(true);
        ui->actionRefuseInvite->setEnabled(true);
        ui->actionAcceptInvite->setVisible(true);
        ui->actionRefuseInvite->setVisible(true);
    }

    return;
}

/**
 * @brief GkXmppRosterDialog::on_treeWidget_callsigns_groups_itemDoubleClicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param item
 * @param column
 */
void GkXmppRosterDialog::on_treeWidget_callsigns_groups_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QList<QTreeWidgetItem *> items;
    items = ui->treeWidget_callsigns_groups->selectedItems();
    for (const auto &item: items) {
        launchMsgDlg(item->text(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX));
        break;
    }

    updateActions();
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
    QModelIndex idx = ui->treeWidget_callsigns_groups->indexAt(ui->actionEdit_Contact->data().toPoint());
    QVariant data = ui->treeWidget_callsigns_groups->model()->data(idx);
    QString text = data.toString();

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionDelete_Contact_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionDelete_Contact_triggered()
{
    QModelIndex idx = ui->treeWidget_callsigns_groups->indexAt(ui->actionDelete_Contact->data().toPoint());
    QVariant data = ui->treeWidget_callsigns_groups->model()->data(idx);
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
        emit updateClientVCard("", "", "", "", avatarByteArray);
        m_clientAvatarImgUpdateTimer->start(GK_NETWORK_CONN_TIMEOUT_MILLSECS);
        ui->pushButton_self_avatar->setText(tr("Updating..."));
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
 * @brief GkXmppRosterDialog::on_treeWidget_callsigns_blocked_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void GkXmppRosterDialog::on_treeWidget_callsigns_blocked_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->treeWidget_callsigns_blocked);
    contextMenu->addAction(ui->actionUnblockUser);

    //
    // Save the position data to the QAction
    ui->actionUnblockUser->setData(QVariant(pos));

    contextMenu->exec(ui->treeWidget_callsigns_blocked->mapToGlobal(pos));
    return;
}

/**
 * @brief GkXmppRosterDialog::on_treeWidget_callsigns_blocked_itemClicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param item
 * @param column
 */
void GkXmppRosterDialog::on_treeWidget_callsigns_blocked_itemClicked(QTreeWidgetItem *item, int column)
{
    return;
}

/**
 * @brief GkXmppRosterDialog::on_treeWidget_callsigns_blocked_itemDoubleClicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param item
 * @param column
 */
void GkXmppRosterDialog::on_treeWidget_callsigns_blocked_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QList<QTreeWidgetItem *> items;
    items = ui->treeWidget_callsigns_groups->selectedItems();
    for (const auto &item: items) {
        launchMsgDlg(item->text(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX));
        break;
    }

    updateActions();
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
 * @brief GkXmppRosterDialog::updateClientAvatarPlaceholder
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::updateClientAvatarPlaceholder()
{
    ui->pushButton_self_avatar->setText(tr("<avatar/>"));
    return;
}

/**
 * @brief GkXmppRosterDialog::updateClientAvatarPlaceholder
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param avatar_img
 */
void GkXmppRosterDialog::updateClientAvatarPlaceholder(const QImage &avatar_img)
{
    ui->pushButton_self_avatar->setIcon(QIcon(QPixmap::fromImage(avatar_img)));
    gkEventLogger->publishEvent(tr("vCard avatar has been registered for self-client."), GkSeverity::Debug, "", false, true, false, false);
    m_clientAvatarImgUpdateTimer->stop();

    return;
}

/**
 * @brief GkXmppRosterDialog::updateUserVCard
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param vCard
 */
void GkXmppRosterDialog::updateUserVCard(const QXmppVCardIq &vCard)
{
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
    return;
}

/**
 * @brief GkXmppRosterDialog::on_pushButton_add_contact_cancel_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_pushButton_add_contact_cancel_clicked()
{
    ui->lineEdit_add_contact_username->clear();
    ui->stackedWidget_roster_ui->setCurrentWidget(ui->page_user_roster);

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionAcceptInvite_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionAcceptInvite_triggered()
{
    QList<QTreeWidgetItem *> items;
    items = ui->treeWidget_callsigns_groups->selectedItems();
    for (const auto &item: items) {
        emit acceptSubscription(item->text(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX));
        break;
    }

    updateActions();
    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionRefuseInvite_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionRefuseInvite_triggered()
{
    QList<QTreeWidgetItem *> items;
    items = ui->treeWidget_callsigns_groups->selectedItems();
    for (const auto &item: items) {
        emit refuseSubscription(item->text(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX));
        break;
    }

    updateActions();
    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionBlockUser_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionBlockUser_triggered()
{
    QList<QTreeWidgetItem *> items;
    items = ui->treeWidget_callsigns_groups->selectedItems();
    for (const auto &item: items) {
        QString bareJid = item->text(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX);
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Do you wish to block user, \"%1\"?").arg(bareJid));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                emit blockUser(bareJid);
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }

        break;
    }

    updateActions();
    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionUnblockUser_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionUnblockUser_triggered()
{
    QList<QTreeWidgetItem *> items;
    items = ui->treeWidget_callsigns_groups->selectedItems();
    for (const auto &item: items) {
        QString bareJid = item->text(GK_XMPP_ROSTER_TREEWIDGET_MODEL_BAREJID_IDX);
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Do you wish to unblock user, \"%1\", and allow possible communications once again?").arg(bareJid));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                emit unblockUser(bareJid);
                break;
            case QMessageBox::Cancel:
                break;
            default:
                break;
        }

        break;
    }

    updateActions();
    return;
}
