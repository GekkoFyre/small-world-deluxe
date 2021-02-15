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
#include <utility>
#include <QMessageBox>

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

GkXmppRosterDialog::GkXmppRosterDialog(const GkUserConn &connection_details, QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                       QPointer<GekkoFyre::GkLevelDb> database, QPointer<GkEventLogger> eventLogger,
                                       QWidget *parent) : shownXmppPreviewNotice(false), QDialog(parent),
                                       ui(new Ui::GkXmppRosterDialog)
{
    ui->setupUi(this);

    try {
        gkConnDetails = connection_details;
        m_xmppClient = std::move(xmppClient);
        gkDb = std::move(database);
        gkEventLogger = std::move(eventLogger);

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

        if (!m_xmppClient->isConnected()) {
            ui->stackedWidget_roster_ui->setCurrentWidget(ui->page_login_or_create_account);
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
 * @brief GkXmppRosterDialog::prefillAvailComboBox
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRosterDialog::prefillAvailComboBox()
{
    // TODO: Insert an icon to the left within this QComboBox at a future date!
    ui->comboBox_current_status->insertItem(GK_XMPP_AVAIL_COMBO_AVAILABLE_IDX, tr("Online / Available"));
    ui->comboBox_current_status->insertItem(GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX, tr("Offline / Unavailable"));

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
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountCreate, gkConnDetails, m_xmppClient, gkEventLogger, nullptr);
    gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
    gkXmppRegistrationDlg->show();

    return;
}
