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
#include <QMessageBox>
#include <QStringList>
#include <QImageReader>

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

        m_clientAvatarImgUpdateTimer = new QTimer(this);
        QObject::connect(m_clientAvatarImgUpdateTimer, SIGNAL(timeout()), this, SLOT(updateClientAvatarPlaceholder()));

        QObject::connect(this, SIGNAL(updateClientVCard(const QString &, const QString &, const QString &, const QString &, const QByteArray &)),
                         m_xmppClient, SLOT(updateClientVCardForm(const QString &, const QString &, const QString &, const QString &, const QByteArray &)));
        QObject::connect(m_xmppClient, SIGNAL(savedClientVCard(const QByteArray &)), this, SLOT(recvClientAvatarImg(const QByteArray &)));
        QObject::connect(this, SIGNAL(updateClientAvatarImg(const QImage &)), this, SLOT(updateClientAvatarPlaceholder(const QImage &)));

        QObject::connect(this, SIGNAL(updateAvailableStatusType(const QXmppPresence::AvailableStatusType &)),
                         m_xmppClient, SLOT(modifyAvailableStatusType(const QXmppPresence::AvailableStatusType &)));

        QObject::connect(ui->label_self_nickname, SIGNAL(clicked(const QString &)), this, SLOT(editNicknameLabel(const QString &)));
        ui->label_self_nickname->setText(tr("Anonymous"));
        if (!gkConnDetails.nickname.isEmpty()) {
            ui->label_self_nickname->setText(gkConnDetails.nickname);
        }

        const QStringList headers({tr("Status"), tr("Nickname")});
        QVector<QVariant> root_data;
        for (const auto &header: headers) {
            root_data << header;
        }

        m_rootItem = QSharedPointer<GkXmppRosterTreeViewItem>(new GkXmppRosterTreeViewItem(root_data));
        m_xmppRosterTreeViewModel = new GkXmppRosterTreeViewModel(m_rootItem.get(), this);

        //
        // Fill out the presence status ComboBox!
        prefillAvailComboBox();

        ui->treeView_callsigns_groups->setModel(m_xmppRosterTreeViewModel);
        for (qint32 column = 0; column < m_xmppRosterTreeViewModel->columnCount(); ++column) {
            ui->treeView_callsigns_groups->header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
        }

        ui->treeView_callsigns_groups->setVisible(true);
        ui->treeView_callsigns_groups->show();
        QObject::connect(ui->treeView_callsigns_groups->selectionModel(), &QItemSelectionModel::selectionChanged, this, &GkXmppRosterDialog::updateActions);

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
 * @brief GkXmppRosterDialog::updateActions
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Editable Tree Model Example <https://doc.qt.io/qt-5/qtwidgets-itemviews-editabletreemodel-example.html>.
 */
void GkXmppRosterDialog::updateActions()
{
    //
    // TODO: Finish this section for XMPP!
    const bool hasSelection = !ui->treeView_callsigns_groups->selectionModel()->selection().isEmpty();

    return;
}

/*
 * @brief GkXmppRosterDialog::reconnectToXmpp reconnects back to the given XMPP server, for if the user has been disconnected, either
 * intentionally or unintentionally.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::reconnectToXmpp()
{
    if (!m_xmppClient->isConnected()) {
        m_xmppClient->createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, gkConnDetails.username,
                                               gkConnDetails.password, gkConnDetails.jid, false);
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
    switch (index) {
        case GK_XMPP_AVAIL_COMBO_AVAILABLE_IDX: // Online / Available
            reconnectToXmpp();
            emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::Online);
            break;
        case GK_XMPP_AVAIL_COMBO_AWAY_FROM_KB_IDX: // Away / Be Back Soon
            reconnectToXmpp();
            emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::Away);
            break;
        case GK_XMPP_AVAIL_COMBO_EXTENDED_AWAY_IDX: // XA / Extended Away
            reconnectToXmpp();
            emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::XA);
            break;
        case GK_XMPP_AVAIL_COMBO_INVISIBLE_IDX: // Online / Invisible
            reconnectToXmpp();
            emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::Invisible);
            break;
        case GK_XMPP_AVAIL_COMBO_BUSY_DND_IDX:
            reconnectToXmpp();
            emit updateAvailableStatusType(QXmppPresence::AvailableStatusType::DND);
            break;
        case GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX: // Offline / Unavailable
            if (m_xmppClient->isConnected()) {
                m_xmppClient->disconnectFromServer();
            }

            break;
        default:
            break;
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
 * @brief GkXmppRosterDialog::on_treeView_callsigns_groups_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void GkXmppRosterDialog::on_treeView_callsigns_groups_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->treeView_callsigns_groups);
    contextMenu->addAction(ui->actionAdd_Contact);
    contextMenu->addAction(ui->actionEdit_Contact);
    contextMenu->addAction(ui->actionDelete_Contact);

    //
    // Save the position data to the QAction
    ui->actionAdd_Contact->setData(QVariant(pos));
    ui->actionEdit_Contact->setData(QVariant(pos));
    ui->actionDelete_Contact->setData(QVariant(pos));

    contextMenu->exec(ui->treeView_callsigns_groups->mapToGlobal(pos));

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
    QModelIndex idx = ui->treeView_callsigns_groups->indexAt(ui->actionEdit_Contact->data().toPoint());
    QVariant data = ui->treeView_callsigns_groups->model()->data(idx);
    QString text = data.toString();

    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionDelete_Contact_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionDelete_Contact_triggered()
{
    QModelIndex idx = ui->treeView_callsigns_groups->indexAt(ui->actionDelete_Contact->data().toPoint());
    QVariant data = ui->treeView_callsigns_groups->model()->data(idx);
    QString text = data.toString();

    return;
}

/**
 * @brief GkXmppRosterDialog::on_treeView_callsigns_pending_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void GkXmppRosterDialog::on_treeView_callsigns_pending_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->treeView_callsigns_pending);
    contextMenu->addAction(ui->actionAcceptInvite);
    contextMenu->addAction(ui->actionRefuseInvite);
    contextMenu->addAction(ui->actionBlockUser);

    //
    // Save the position data to the QAction
    ui->actionAcceptInvite->setData(QVariant(pos));
    ui->actionRefuseInvite->setData(QVariant(pos));
    ui->actionBlockUser->setData(QVariant(pos));

    contextMenu->exec(ui->treeView_callsigns_pending->mapToGlobal(pos));

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
 * @brief GkXmppRosterDialog::on_pushButton_add_contact_submit_clicked
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
    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionAcceptInvite_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionAcceptInvite_triggered()
{
    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionRefuseInvite_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionRefuseInvite_triggered()
{
    return;
}

/**
 * @brief GkXmppRosterDialog::on_actionBlockUser_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::on_actionBlockUser_triggered()
{
    return;
}
