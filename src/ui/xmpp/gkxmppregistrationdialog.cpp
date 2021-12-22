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

#include "gkxmppregistrationdialog.hpp"
#include "ui_gkxmppregistrationdialog.h"
#include "src/gk_string_funcs.hpp"
#include <qxmpp/QXmppDataForm.h>
#include <qxmpp/QXmppLogger.h>
#include <utility>
#include <QPixmap>
#include <QFormLayout>
#include <QMessageBox>
#include <QScopedPointer>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QRegularExpressionValidator>

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

/**
 * @brief GkXmppRegistrationDialog::GkXmppRegistrationDialog
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param gkRegUiRole
 * @param connection_details
 * @param xmppClient
 * @param eventLogger
 * @param parent
 * @note QXmppRegistrationManager Class Reference <https://doc.qxmpp.org/qxmpp-dev/classQXmppRegistrationManager.html>.
 */
GkXmppRegistrationDialog::GkXmppRegistrationDialog(const GkRegUiRole &gkRegUiRole, const GkUserConn &connection_details,
                                                   QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                                   QPointer<GekkoFyre::GkLevelDb> gkDb,
                                                   QPointer<GekkoFyre::GkEventLogger> eventLogger, QWidget *parent) :
                                                   m_netState(GkNetworkState::None), QDialog(parent),
                                                   ui(new Ui::GkXmppRegistrationDialog)
{
    ui->setupUi(this);
    try {
        //
        // Set these as invisible by default, unless the given XMPP server has a need for them!
        //

        //
        // User registration captcha
        ui->label_xmpp_captcha->setVisible(false);
        ui->frame_xmpp_captcha_top->setVisible(false);
        ui->frame_xmpp_captcha_bottom->setVisible(false);

        //
        // Change password captcha
        ui->label_xmpp_change_password_captcha->setVisible(false);
        ui->frame_xmpp_change_password_captcha_top->setVisible(false);
        ui->frame_xmpp_change_password_captcha_bottom->setVisible(false);

        //
        // Change email captcha
        ui->label_xmpp_change_email_captcha->setVisible(false);
        ui->frame_xmpp_change_email_captcha_top->setVisible(false);
        ui->frame_xmpp_change_email_captcha_bottom->setVisible(false);

        //
        // User login captcha
        ui->label_xmpp_login_captcha->setVisible(false);
        ui->frame_xmpp_login_captcha_top->setVisible(false);
        ui->frame_xmpp_login_captcha_bottom->setVisible(false);

        gkDekodeDb = std::move(gkDb);
        gkEventLogger = std::move(eventLogger);
        m_xmppClient = std::move(xmppClient);

        //
        // QXmpp and XMPP related
        //
        m_reg_remember_credentials = false;
        m_connDetails = connection_details;
        m_registerManager = m_xmppClient->getRegistrationMgr();

        //
        // Prefill server-type QComboBox...
        prefill_xmpp_server_type(GkXmpp::GkServerType::GekkoFyre);
        prefill_xmpp_server_type(GkXmpp::GkServerType::Custom);

        QObject::connect(m_xmppClient, SIGNAL(sendRegistrationForm(const QXmppRegisterIq &)),
                         this, SLOT(handleRegistrationForm(const QXmppRegisterIq &)));

        switch (gkRegUiRole) {
            case GkRegUiRole::AccountCreate:
                // Create a new user account on the given XMPP server
                ui->stackedWidget_xmpp_registration_dialog->setCurrentWidget(ui->page_account_signup_ui);
                break;
            case GkRegUiRole::AccountLogin:
                // Login to pre-existing user account on the given XMPP server
                ui->stackedWidget_xmpp_registration_dialog->setCurrentWidget(ui->page_account_login_ui);
                break;
            case GkRegUiRole::AccountChangePassword:
                // Change password for pre-existing user account on the given XMPP server
                ui->stackedWidget_xmpp_registration_dialog->setCurrentWidget(ui->page_account_change_password_ui);
                break;
            case GkRegUiRole::AccountChangeEmail:
                // Change e-mail address for pre-existing user account on the given XMPP server
                ui->stackedWidget_xmpp_registration_dialog->setCurrentWidget(ui->page_account_change_email_ui);
                break;
            default:
                // What to do by default, if no information is otherwise given!
                ui->stackedWidget_xmpp_registration_dialog->setCurrentWidget(ui->page_account_login_ui);
                break;
        }

        QObject::connect(this, SIGNAL(sendError(const QString &)), this, SLOT(handleError(const QString &)));

        //
        // Validate inputs for the Email Address
        //
        QRegularExpression rxEmail(R"(\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,4}\b)",
                                   QRegularExpression::CaseInsensitiveOption);
        ui->lineEdit_email->setValidator(new QRegularExpressionValidator(rxEmail, this));
        ui->lineEdit_change_email_new_address->setValidator(new QRegularExpressionValidator(rxEmail, this));

        QObject::connect(ui->lineEdit_email, SIGNAL(textChanged(const QString &)),
                         this, SLOT(setEmailInputColor(const QString &)));

        QObject::connect(ui->lineEdit_change_email_new_address, SIGNAL(textChanged(const QString &)),
                         this, SLOT(setEmailInputColor(const QString &)));

        //
        // Validate inputs for the Username
        //
        QRegularExpression rxUsername(R"(\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,4}\b)", QRegularExpression::CaseInsensitiveOption);
        ui->lineEdit_username->setValidator(new QRegularExpressionValidator(rxUsername, this));
        ui->lineEdit_login_username->setValidator(new QRegularExpressionValidator(rxUsername, this));
        ui->lineEdit_change_password_username->setValidator(new QRegularExpressionValidator(rxUsername, this));
        ui->lineEdit_change_email_username->setValidator(new QRegularExpressionValidator(rxUsername, this));

        QObject::connect(ui->lineEdit_username, SIGNAL(textChanged(const QString &)),
                         this, SLOT(setUsernameInputColor(const QString &)));

        QObject::connect(ui->lineEdit_login_username, SIGNAL(textChanged(const QString &)),
                         this, SLOT(setUsernameInputColor(const QString &)));

        QObject::connect(ui->lineEdit_change_password_username, SIGNAL(textChanged(const QString &)),
                         this, SLOT(setUsernameInputColor(const QString &)));

        QObject::connect(ui->lineEdit_change_email_username, SIGNAL(textChanged(const QString &)),
                         this, SLOT(setUsernameInputColor(const QString &)));

        if (m_xmppClient->isConnected() || m_netState == GkNetworkState::Connecting) {
            //
            // Disconnect from the server since filling out the form may take some time, and we might
            // timeout on the connection otherwise!
            //
            m_xmppClient->killConnectionFromServer(false);
        }

        QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::passwordChangeFailed, [=](const QXmppStanza::Error &error) {
            gkEventLogger->publishEvent(tr("An attempt to change the password has failed: %1").arg(error.text()), GkSeverity::Fatal, "",
                                        false, true, false, true);
        });

        QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::passwordChanged, this, [=](const QString &newPassword) {
            Q_UNUSED(newPassword);
            gkEventLogger->publishEvent(tr("Password successfully changed for user, \"%1\", that's registered with XMPP server: %2").arg(m_reg_user)
                                                .arg(QHostAddress(m_reg_domain).toString()), GkSeverity::Info, "", true, true, false, false);
            m_xmppClient->killConnectionFromServer(false);
        });

        m_updateNetworkStateTimer = new QTimer(this);
        QObject::connect(m_updateNetworkStateTimer, SIGNAL(timeout()), this, SLOT(updateNetworkState()));
        m_updateNetworkStateTimer->start(GK_XMPP_NETWORK_STATE_UPDATE_SECS * 1000);
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

GkXmppRegistrationDialog::~GkXmppRegistrationDialog()
{
    delete ui;
}

/**
 * @brief GkXmppRegistrationDialog::externalUserSignup
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param network_port
 * @param jid
 * @param email
 * @param password
 */
void GkXmppRegistrationDialog::externalUserSignup(const quint16 &network_port, const QString &jid, const QString &email,
                                                  const QString &password)
{
    if (jid.isEmpty()) {
        // Username field is empty!
        QMessageBox::warning(this, tr("Empty field!"), tr("The jid field cannot be empty!"), QMessageBox::Ok);
        return;
    }

    if (password.isEmpty()) {
        // Password field is empty!
        QMessageBox::warning(this, tr("Empty field!"), tr("The password field cannot be empty!"), QMessageBox::Ok);
        return;
    }

    if (network_port < 80) {
        // Password field is empty!
        QMessageBox::warning(this, tr("Invalid value!"), tr("The network port cannot be less than 80 (i.e. HTTP)!"), QMessageBox::Ok);
        return;
    }

    m_reg_jid = jid;
    m_reg_domain = m_xmppClient->getHostname(jid); // Extract the URI from the given JID!
    m_reg_email = email;
    m_reg_password = password;

    m_connDetails.jid = m_reg_jid;
    m_connDetails.email = m_reg_email;
    m_connDetails.password = m_reg_password;
    m_connDetails.server.domain = QHostAddress(m_reg_domain);

    // TODO: Implement proper captcha support!

    userSignup(network_port, jid, password);
    return;
}

/**
 * @brief GkXmppRegistrationDialog::prefillFormFields
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param jid
 * @param password
 * @param email
 * @param network_port
 */
void GkXmppRegistrationDialog::prefillFormFields(const QString &jid, const QString &password, const QString &email,
                                                 const quint16 &network_port)
{
    //
    // Register as global variables, if not empty!
    if (!jid.isEmpty()) {
        m_reg_user = m_xmppClient->getUsername(jid); // Extract the username from the given JID and its attached URI!
        m_reg_domain = m_xmppClient->getHostname(jid); // Extract the URI from the given JID!
    }

    if (!password.isEmpty()) {
        m_reg_password = password;
    }

    if (!email.isEmpty()) {
        m_reg_email = email;
    }

    if (network_port >= 80) {
        m_network_port = network_port;
    }

    if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_signup_ui) {
        if (!jid.isEmpty()) {
            if (!m_reg_user.isEmpty() && !m_reg_domain.isEmpty()) {
                ui->lineEdit_username->setText(QString("%1@%2").arg(m_reg_user).arg(m_reg_domain));
            }
        }

        if (!m_reg_password.isEmpty()) {
            ui->lineEdit_password->setText(m_reg_password);
        }

        if (!m_reg_email.isEmpty()) {
            ui->lineEdit_email->setText(m_reg_email);
        }

        if (m_network_port >= 80) {
            ui->spinBox_xmpp_port->setValue(m_network_port);
        }
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_signup_submit_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_signup_submit_clicked()
{
    QString jid;
    QString email;
    QString new_password;
    quint16 network_port;
    QString captcha;

    if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_signup_ui) {
        //
        // Account signup!
        email = ui->lineEdit_email->text();
        jid = ui->lineEdit_username->text();
        new_password = ui->lineEdit_password->text();
        network_port = ui->spinBox_xmpp_port->value();

        // Captcha
        captcha = ui->lineEdit_xmpp_captcha_input->text();

        if (jid.isEmpty()) {
            // Username field is empty!
            QMessageBox::warning(this, tr("Empty field!"), tr("The jid field cannot be empty!"), QMessageBox::Ok);
            return;
        }

        if (email.isEmpty()) {
            // Password field is empty!
            QMessageBox::warning(this, tr("Empty field!"), tr("The e-mail field cannot be empty!"), QMessageBox::Ok);
            return;
        }

        if (new_password.isEmpty()) {
            // Password field is empty!
            QMessageBox::warning(this, tr("Empty field!"), tr("The password field cannot be empty!"), QMessageBox::Ok);
            return;
        }

        if (network_port < 80) {
            // Password field is empty!
            QMessageBox::warning(this, tr("Invalid value!"), tr("The network port cannot be less than 80 (i.e. HTTP)!"), QMessageBox::Ok);
            return;
        }

        /*
        if (captcha.isEmpty()) {
            // Captcha field is empty!
            QMessageBox::warning(this, tr("Empty field!"), tr("The captcha field cannot be empty!"), QMessageBox::Ok);
            return;
        }
         */

        m_reg_jid = jid;
        m_reg_user = m_xmppClient->getUsername(jid); // Extract the username from the given JID and its attached URI!
        m_reg_domain = m_xmppClient->getHostname(jid); // Extract the URI from the given JID!
        m_reg_email = email;
        m_reg_password = new_password;
        m_reg_captcha = captcha;

        m_connDetails.jid = m_reg_jid;
        m_connDetails.email = m_reg_email;
        m_connDetails.password = m_reg_password;
        m_connDetails.server.domain = QHostAddress(m_reg_domain);
        m_connDetails.username = m_reg_user;

        if (m_reg_remember_credentials) {
            rememberCredentials();
        }

        userSignup(network_port, jid, new_password);
        return;
    }

    this->close();
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_signup_reset_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_signup_reset_clicked()
{
    ui->lineEdit_email->clear();
    ui->lineEdit_username->clear();
    ui->lineEdit_password->clear();
    ui->lineEdit_xmpp_captcha_input->clear();

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_signup_cancel_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_signup_cancel_clicked()
{
    this->close();
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_login_submit_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_login_submit_clicked()
{
    try {
        QString jid;
        QString old_password;
        QString captcha;
        quint16 network_port;

        if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_login_ui) {
            //
            // Account login!
            jid = ui->lineEdit_login_username->text();
            old_password = ui->lineEdit_login_password->text();
            network_port = ui->spinBox_login_xmpp_port->value();

            // Captcha
            captcha = ui->lineEdit_xmpp_login_captcha_input->text();

            if (jid.isEmpty()) {
                // Username field is empty!
                QMessageBox::warning(this, tr("Empty field!"), tr("The jid field cannot be empty!"), QMessageBox::Ok);
                return;
            }

            if (network_port < 80) {
                // Password field is empty!
                QMessageBox::warning(this, tr("Invalid value!"), tr("The network port cannot be less than 80 (i.e. HTTP)!"), QMessageBox::Ok);
                return;
            }

            if (old_password.isEmpty()) {
                // Password field is empty!
                QMessageBox::warning(this, tr("Empty field!"), tr("The password field cannot be empty!"), QMessageBox::Ok);
                return;
            }

            /*
            if (captcha.isEmpty()) {
                // Captcha field is empty!
                QMessageBox::warning(this, tr("Empty field!"), tr("The captcha field cannot be empty!"), QMessageBox::Ok);
                return;
            }
             */

            if (m_reg_remember_credentials) {
                rememberCredentials();
            }

            //
            // Attempt a login to the server!
            loginToServer(m_xmppClient->getHostname(jid), network_port, jid, old_password);

            return;
        } else {
            //
            // Default...

            return;
        }
    } catch (const std::exception &e) {
        print_exception(e);
    }

    this->close();
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_login_reset_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_login_reset_clicked()
{
    if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_signup_ui) {
        //
        // Account signup!
        ui->lineEdit_email->clear();
        ui->lineEdit_username->clear();
        ui->lineEdit_password->clear();

        // Captcha
        ui->lineEdit_xmpp_captcha_input->clear();
        return;
    } else if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_login_ui) {
        //
        // Account login!
        ui->lineEdit_login_username->clear();
        ui->lineEdit_login_password->clear();

        // Captcha
        ui->lineEdit_xmpp_login_captcha_input->clear();
        return;
    } else if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_change_password_ui) {
        //
        // Change of password!
        return;
    } else if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_change_email_ui) {
        //
        // Change of email!
        return;
    } else {
        //
        // Default...
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_login_cancel_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_login_cancel_clicked()
{
    this->close();
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_toolButton_xmpp_captcha_refresh_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_toolButton_xmpp_captcha_refresh_clicked()
{
    //
    // Refresh the captcha presented to the user!

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_toolButton_xmpp_login_captcha_refresh_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_toolButton_xmpp_login_captcha_refresh_clicked()
{
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_continue_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_continue_clicked()
{
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_retry_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_retry_clicked()
{
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_exit_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_exit_clicked()
{
    this->close();
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_change_password_reset_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_change_password_reset_clicked()
{
    ui->lineEdit_change_password_username->clear();
    ui->lineEdit_change_password_old_input->clear();
    ui->lineEdit_change_password_new_input->clear();
    ui->lineEdit_xmpp_change_password_captcha_input->clear();

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_change_password_cancel_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_change_password_cancel_clicked()
{
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_change_password_submit_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_change_password_submit_clicked()
{
    if (m_reg_remember_credentials) {
        rememberCredentials();
    }

    this->close();
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_change_email_submit_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_change_email_submit_clicked()
{
    if (m_reg_remember_credentials) {
        rememberCredentials();
    }

    this->close();
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_change_email_reset_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_change_email_reset_clicked()
{
    ui->lineEdit_change_email_new_address->clear();
    ui->lineEdit_change_email_username->clear();
    ui->lineEdit_change_email_password->clear();
    ui->lineEdit_xmpp_change_email_captcha_input->clear();

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_change_email_cancel_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_change_email_cancel_clicked()
{
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_comboBox_server_type_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRegistrationDialog::on_comboBox_server_type_currentIndexChanged(int index)
{
    switch (index) {
        case GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX:
            m_registerServerType = GkXmpp::GkServerType::GekkoFyre;
            ui->lineEdit_username->setPlaceholderText(tr("<username>"));
            readCredentials();
            return;
        case GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX:
            m_registerServerType = GkXmpp::GkServerType::Custom;
            ui->lineEdit_username->setPlaceholderText(tr("<user>@<host>.<tld>"));
            readCredentials();
            return;
        default:
            m_registerServerType = GkXmpp::GkServerType::Unknown;
            ui->lineEdit_username->setPlaceholderText(tr("<user>@<host>.<tld>"));
            readCredentials();
            return;
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_comboBox_xmpp_login_server_type_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRegistrationDialog::on_comboBox_xmpp_login_server_type_currentIndexChanged(int index)
{
    switch (index) {
        case GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX:
            m_loginServerType = GkXmpp::GkServerType::GekkoFyre;
            ui->lineEdit_login_username->setPlaceholderText(tr("<username>"));
            readCredentials();
            return;
        case GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX:
            m_loginServerType = GkXmpp::GkServerType::Custom;
            ui->lineEdit_login_username->setPlaceholderText(tr("<user>@<host>.<tld>"));
            readCredentials();
            return;
        default:
            m_loginServerType = GkXmpp::GkServerType::Unknown;
            ui->lineEdit_login_username->setPlaceholderText(tr("<user>@<host>.<tld>"));
            readCredentials();
            return;
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_comboBox_xmpp_change_password_server_type_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRegistrationDialog::on_comboBox_xmpp_change_password_server_type_currentIndexChanged(int index)
{
    switch (index) {
        case GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX:
            m_passwordServerType = GkXmpp::GkServerType::GekkoFyre;
            ui->lineEdit_change_password_username->setPlaceholderText(tr("<username>"));
            readCredentials();
            return;
        case GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX:
            m_passwordServerType = GkXmpp::GkServerType::Custom;
            ui->lineEdit_change_password_username->setPlaceholderText(tr("<user>@<host>.<tld>"));
            readCredentials();
            return;
        default:
            m_passwordServerType = GkXmpp::GkServerType::Unknown;
            ui->lineEdit_change_password_username->setPlaceholderText(tr("<user>@<host>.<tld>"));
            readCredentials();
            return;
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_comboBox_change_email_server_type_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkXmppRegistrationDialog::on_comboBox_change_email_server_type_currentIndexChanged(int index)
{
    switch (index) {
        case GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX:
            m_emailServerType = GkXmpp::GkServerType::GekkoFyre;
            ui->lineEdit_change_email_username->setPlaceholderText(tr("<username>"));
            readCredentials();
            return;
        case GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX:
            m_emailServerType = GkXmpp::GkServerType::Custom;
            ui->lineEdit_change_email_username->setPlaceholderText(tr("<user>@<host>.<tld>"));
            readCredentials();
            return;
        default:
            m_emailServerType = GkXmpp::GkServerType::Unknown;
            ui->lineEdit_change_email_username->setPlaceholderText(tr("<user>@<host>.<tld>"));
            readCredentials();
            return;
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_checkBox_remember_credentials_stateChanged indicates whether the user credentials, such
 * as the login username (and if applicable) password, should be saved to the Google LevelDB database or not.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1 Whether the QCheckBox, `ui->checkBox_remember_credentials()`, has been checked or not.
 */
void GkXmppRegistrationDialog::on_checkBox_remember_credentials_stateChanged(int arg1)
{
    m_reg_remember_credentials = gkDekodeDb->intBool(arg1);
    return;
}

/**
 * @brief GkXmppRegistrationDialog::handleRegistrationForm attempts to sign-up a user with the given XMPP server by
 * submitting the registration form received by function, `GkXmppRegistrationDialog::recvRegistrationForm()`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param registerIq
 * @note QXmppRegistrationManager Class Reference <https://doc.qxmpp.org/qxmpp-dev/classQXmppRegistrationManager.html>.
 * @see GkXmppRegistrationDialog::recvRegistrationForm().
 */
void GkXmppRegistrationDialog::handleRegistrationForm(const QXmppRegisterIq &registerIq)
{
    try {
        if (!m_reg_user.isEmpty() && !m_reg_password.isEmpty() && !m_reg_email.isEmpty()) {
            //
            // The form now needs to be completed!
            QXmppRegisterIq registration_form = registerIq;
            registration_form.setEmail(StringFuncs::htmlSpecialCharEncoding(m_reg_email));
            registration_form.setPassword(StringFuncs::htmlSpecialCharEncoding(m_reg_password));
            registration_form.setUsername(StringFuncs::htmlSpecialCharEncoding(m_reg_user));
            registration_form.setId(QStringLiteral("register1"));
            registration_form.setType(QXmppIq::Type::Set);

            //
            // Allow the registration of a new user to proceed!
            m_registerManager->setRegisterOnConnectEnabled(true);

            //
            // Send the filled out form itself! But only once we reconnect, where it will automatically be sent
            // immediately upon a successful connection being made to the server in question.
            // https://doc.qxmpp.org/qxmpp-1/classQXmppRegistrationManager.html
            //
            m_registerManager->setRegistrationFormToSend(registration_form);
            m_registerManager->sendCachedRegistrationForm();

            //
            // Disconnect from the given XMPP after so many seconds have passed by...
            QTimer::singleShot(GK_XMPP_HANDLE_DISCONNECTION_SINGLE_SHOT_TIMER_SECS * 1000, m_xmppClient, &GkXmppClient::disconnectFromServer);
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("Issues were encountered with trying to register user with XMPP server! Error: %1")
                                            .arg(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::loginToServer initializes the in-band logging-in of authorized users to a given XMPP
 * server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param hostname The XMPP server in question to connect towards (i.e. `xmpp.example.com`, without the quotes).
 * @param network_port The network port to connect towards, if manually specified.
 * @param username The username of the registrant to login as.
 * @param password The password of the registrant to login as.
 * @param credentials_fail Whether to fail or not if there are no credentials, or invalid credentials, provided.
 */
void GkXmppRegistrationDialog::loginToServer(const QString &hostname, const quint16 &network_port, const QString &password,
                                             const QString &jid, const bool &credentials_fail)
{
    try {
        //
        // Disconnect from server if already connected...
        if (!m_xmppClient->isConnected() || m_netState == GkNetworkState::Connecting) {
            m_xmppClient->killConnectionFromServer(false);
        }

        if (!jid.isEmpty() && !password.isEmpty()) {
            //
            // A username and password has been provided!
            QString user_hostname = m_xmppClient->getHostname(jid); // Just in-case the user has entered a differing hostname!
            if (!user_hostname.isEmpty()) {
                //
                // A hostname has been provided as well and thusly derived from the username!
                m_xmppClient->createConnectionToServer(user_hostname, network_port, password, jid, false);
                return;
            }

            //
            // No hostname has been provided, therefore we will use the pre-configured settings (if any)!
            m_xmppClient->createConnectionToServer(hostname, network_port, password, jid, false);
            return;
        }

        //
        // No username or password has been provided!
        m_xmppClient->createConnectionToServer(hostname, network_port);
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::userSignup initializes the signing-up and in-band registration of a new user to a
 * given XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param network_port The network port to connect towards, if manually specified.
 * @param username The username of the registrant in question.
 * @param password The password of the registrant in question.
 * @see GkXmppClient::createConnectionToServer().
 */
void GkXmppRegistrationDialog::userSignup(const quint16 &network_port, const QString &jid, const QString &password)
{
    try {
        if (m_xmppClient->isConnected() || m_netState == GkNetworkState::Connecting) {
            m_xmppClient->killConnectionFromServer(false);
        }

        if (jid.isEmpty() || password.isEmpty()) {
            throw std::invalid_argument(tr("Provided username and/or password are empty! Unable to continue with "
                                           "user registration.").toStdString());
        }

        //
        // A username and password has been provided!
        QString hostname = m_xmppClient->getHostname(jid);
        QString username = m_xmppClient->getUsername(jid);

        if (!hostname.isEmpty() && !username.isEmpty()) {
            //
            // A hostname has been provided as well and thusly derived from the username!
            m_xmppClient->createConnectionToServer(hostname, network_port, QString(), QString(), true);
            return;
        }
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::rememberCredentials if indicated, remembers the entered credentials for later use
 * by the Small World Deluxe application itself.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::rememberCredentials()
{
    if (!m_connDetails.jid.isEmpty() && !m_connDetails.server.url.isEmpty()) {
        if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_signup_ui) {
            //
            // Account registration
            gkDekodeDb->write_xmpp_settings(QString::number(ui->comboBox_server_type->currentIndex()), Settings::GkXmppCfg::XmppServerType);
            if (m_registerServerType != GkXmpp::GkServerType::GekkoFyre) {
                if (m_connDetails.server.port >= 80) {
                    gkDekodeDb->write_xmpp_settings(QString::number(m_connDetails.server.port), Settings::GkXmppCfg::XmppDomainPort);
                }
            }
        } else if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_login_ui) {
            //
            // Account login
            gkDekodeDb->write_xmpp_settings(QString::number(ui->comboBox_xmpp_login_server_type->currentIndex()), Settings::GkXmppCfg::XmppServerType);
            if (m_loginServerType != GkXmpp::GkServerType::GekkoFyre) {
                if (m_connDetails.server.port >= 80) {
                    gkDekodeDb->write_xmpp_settings(QString::number(m_connDetails.server.port), Settings::GkXmppCfg::XmppDomainPort);
                }
            }
        } else if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_change_password_ui) {
            //
            // Account change password
            gkDekodeDb->write_xmpp_settings(QString::number(ui->comboBox_xmpp_change_password_server_type->currentIndex()), Settings::GkXmppCfg::XmppServerType);
            if (m_passwordServerType != GkXmpp::GkServerType::GekkoFyre) {
                if (m_connDetails.server.port >= 80) {
                    gkDekodeDb->write_xmpp_settings(QString::number(m_connDetails.server.port), Settings::GkXmppCfg::XmppDomainPort);
                }
            }
        } else if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_change_email_ui) {
            //
            // Account change email
            gkDekodeDb->write_xmpp_settings(QString::number(ui->comboBox_change_email_server_type->currentIndex()), Settings::GkXmppCfg::XmppServerType);
            if (m_emailServerType != GkXmpp::GkServerType::GekkoFyre) {
                if (m_connDetails.server.port >= 80) {
                    gkDekodeDb->write_xmpp_settings(QString::number(m_connDetails.server.port), Settings::GkXmppCfg::XmppDomainPort);
                }
            }
        } else {
            //
            // Unknown!
            std::throw_with_nested(std::invalid_argument(tr("Error encountered whilst saving credentials to database!").toStdString()));
        }

        gkDekodeDb->write_xmpp_settings(m_connDetails.jid, Settings::GkXmppCfg::XmppJid);
        gkDekodeDb->write_xmpp_settings(QString::number(gkDekodeDb->boolInt(ui->checkBox_remember_credentials->isChecked())), Settings::GkXmppCfg::XmppCheckboxRememberCreds);

        if (!m_connDetails.password.isEmpty()) {
            gkDekodeDb->write_xmpp_settings(m_connDetails.password, Settings::GkXmppCfg::XmppPassword);
        }

        if (!m_connDetails.email.isEmpty()) {
            gkDekodeDb->write_xmpp_settings(m_connDetails.email, Settings::GkXmppCfg::XmppEmailAddr);
        }

        if (!m_connDetails.username.isEmpty()) {
            gkDekodeDb->write_xmpp_settings(m_connDetails.username, Settings::GkXmppCfg::XmppUsername);
        }
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::readCredentials reads any remembered credentials and applies them to the form(s), where
 * they are needed/required.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::readCredentials()
{
    const QString bareJid = gkDekodeDb->read_xmpp_settings(Settings::GkXmppCfg::XmppJid);
    const QString email = gkDekodeDb->read_xmpp_settings(Settings::GkXmppCfg::XmppEmailAddr);
    const QString password = gkDekodeDb->read_xmpp_settings(Settings::GkXmppCfg::XmppPassword);
    const QString portStr = gkDekodeDb->read_xmpp_settings(Settings::GkXmppCfg::XmppDomainPort);

    if (!email.isEmpty()) {
        ui->lineEdit_email->setText(email);

    }

    if (!password.isEmpty()) {
        ui->lineEdit_password->setText(password);
        ui->lineEdit_login_password->setText(password);
        ui->lineEdit_change_email_password->setText(password);
    }

    if (m_registerServerType == GkXmpp::GkServerType::GekkoFyre) {
        //
        // User Registration -- GekkoFyre Networks!
        if (!bareJid.isEmpty()) {
            ui->lineEdit_username->setText(m_xmppClient->getUsername(bareJid));
        }
    } else if (m_registerServerType == GkXmpp::GkServerType::Custom) {
        //
        // User Registration -- Custom!
        if (!bareJid.isEmpty()) {
            ui->lineEdit_username->setText(bareJid);
        }

        if (!portStr.isEmpty()) {
            ui->spinBox_xmpp_port->setValue(portStr.toUInt());
        }
    } else {
        //
        // User Registration -- Unknown!
        return;
    }

    if (m_loginServerType == GkXmpp::GkServerType::GekkoFyre) {
        //
        // User Login -- GekkoFyre Networks!
        if (!bareJid.isEmpty()) {
            ui->lineEdit_login_username->setText(m_xmppClient->getUsername(bareJid));
        }
    } else if (m_loginServerType == GkXmpp::GkServerType::Custom) {
        //
        // User Login -- Custom!
        if (!bareJid.isEmpty()) {
            ui->lineEdit_login_username->setText(bareJid);
        }

        if (!portStr.isEmpty()) {
            ui->spinBox_login_xmpp_port->setValue(portStr.toUInt());
        }
    } else {
        //
        // User Login -- Unknown!
        return;
    }

    if (m_passwordServerType == GkXmpp::GkServerType::GekkoFyre) {
        //
        // User Change Password -- GekkoFyre Networks!
        if (!bareJid.isEmpty()) {
            ui->lineEdit_change_password_username->setText(m_xmppClient->getUsername(bareJid));
        }
    } else if (m_passwordServerType == GkXmpp::GkServerType::Custom) {
        //
        // User Change Password -- Custom!
        if (!bareJid.isEmpty()) {
            ui->lineEdit_change_password_username->setText(bareJid);
        }

        if (!portStr.isEmpty()) {
            ui->spinBox_change_password_port->setValue(portStr.toUInt());
        }
    } else {
        //
        // User Change Password -- Unknown!
        return;
    }

    if (m_emailServerType == GkXmpp::GkServerType::GekkoFyre) {
        //
        // User Change Email -- GekkoFyre Networks!
        if (!bareJid.isEmpty()) {
            ui->lineEdit_change_email_username->setText(m_xmppClient->getUsername(bareJid));
        }
    } else if (m_emailServerType == GkXmpp::GkServerType::Custom) {
        //
        // User Change Email -- Custom!
        if (!bareJid.isEmpty()) {
            ui->lineEdit_change_email_username->setText(bareJid);
        }

        if (!portStr.isEmpty()) {
            ui->spinBox_change_email_port->setValue(portStr.toUInt());
        }
    } else {
        //
        // User Change Email -- Unknown!
        return;
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::setEmailInputColor adjusts the color of a given QLineEdit widget, dependent on
 * whether there's acceptable user input or not. If there's acceptable user input, the text appears as white, otherwise
 * it will be orange in coloration.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param adj_text Not a used value.
 */
void GkXmppRegistrationDialog::setEmailInputColor(const QString &adj_text)
{
    Q_UNUSED(adj_text);

    if (!ui->lineEdit_email->hasAcceptableInput()) {
        ui->lineEdit_email->setStyleSheet("QLineEdit { color: orange; }");
    } else {
        ui->lineEdit_email->setStyleSheet("QLineEdit { color: white; }");
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::setUsernameInputColor adjusts the color of a given QLineEdit widget, dependent on
 * whether there's acceptable user input or not. If there's acceptable user input, the text appears as white, otherwise
 * it will be orange in coloration.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param adj_text Not a used value.
 */
void GkXmppRegistrationDialog::setUsernameInputColor(const QString &adj_text)
{
    Q_UNUSED(adj_text);

    if (!ui->lineEdit_username->hasAcceptableInput()) {
        ui->lineEdit_username->setStyleSheet("QLineEdit { color: orange; }");
    } else {
        ui->lineEdit_username->setStyleSheet("QLineEdit { color: white; }");
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::handleError Handles the parsing of error messages and thusly, the disconnection of Small
 * World Deluxe from the given XMPP server in question.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param errorMsg The error message that we are dealing with.
 */
void GkXmppRegistrationDialog::handleError(const QString &errorMsg)
{
    QObject::disconnect(m_xmppClient, nullptr, this, nullptr);
    if (!errorMsg.isEmpty()) {
        gkEventLogger->publishEvent(errorMsg, GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::handleSuccess
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::handleSuccess()
{
    QObject::disconnect(m_xmppClient, nullptr, this, nullptr);

    return;
}

/**
 * @brief GkXmppRegistrationDialog::updateNetworkState
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::updateNetworkState()
{
    m_netState = m_xmppClient->getNetworkState();
    return;
}

/**
 * @brief GkXmppRegistrationDialog::prefill_xmpp_server_type
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param server_type
 */
void GkXmppRegistrationDialog::prefill_xmpp_server_type(const GkXmpp::GkServerType &server_type)
{
    switch (server_type) {
        case GkXmpp::GkServerType::GekkoFyre:
            ui->comboBox_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX, tr("GekkoFyre Networks (best choice!)"));
            ui->comboBox_xmpp_login_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX, tr("GekkoFyre Networks (best choice!)"));
            ui->comboBox_xmpp_change_password_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX, tr("GekkoFyre Networks (best choice!)"));
            ui->comboBox_change_email_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX, tr("GekkoFyre Networks (best choice!)"));
            break;
        case GkXmpp::GkServerType::Custom:
            ui->comboBox_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX, tr("Custom (use with caution)"));
            ui->comboBox_xmpp_login_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX, tr("Custom (use with caution)"));
            ui->comboBox_xmpp_change_password_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX, tr("Custom (use with caution)"));
            ui->comboBox_change_email_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX, tr("Custom (use with caution)"));
            break;
        default:
            std::throw_with_nested(std::invalid_argument(tr("Error encountered whilst pre-filling QComboBoxes for XMPP server-type to use!").toStdString()));
    }

    const QString serverTypeStr = gkDekodeDb->read_xmpp_settings(Settings::GkXmppCfg::XmppServerType);
    if (!serverTypeStr.isEmpty()) {
        on_comboBox_server_type_currentIndexChanged(serverTypeStr.toInt());
        on_comboBox_xmpp_login_server_type_currentIndexChanged(serverTypeStr.toInt());
        on_comboBox_xmpp_change_password_server_type_currentIndexChanged(serverTypeStr.toInt());
        on_comboBox_change_email_server_type_currentIndexChanged(serverTypeStr.toInt());
        return;
    }

    on_comboBox_server_type_currentIndexChanged(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX);
    on_comboBox_xmpp_login_server_type_currentIndexChanged(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX);
    on_comboBox_xmpp_change_password_server_type_currentIndexChanged(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX);
    on_comboBox_change_email_server_type_currentIndexChanged(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX);

    return;
}

/**
 * @brief GkXmppRegistrationDialog::print_exception
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param e
 * @param level
 */
void GkXmppRegistrationDialog::print_exception(const std::exception &e, int level)
{
    emit sendError(QString::fromStdString(e.what()));

    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        print_exception(e, level + 1);
    } catch (...) {}

    return;
}
