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
                                                   QPointer<GekkoFyre::GkEventLogger> eventLogger, QWidget *parent) :
    m_netState(GkNetworkState::None), QDialog(parent), ui(new Ui::GkXmppRegistrationDialog)
{
    ui->setupUi(this);
    try {
        //
        // Set these as invisible by default, unless the given XMPP server has a need for them!
        ui->label_xmpp_login_captcha->setVisible(false);
        ui->frame_xmpp_login_captcha_top->setVisible(false);
        ui->frame_xmpp_login_captcha_bottom->setVisible(false);

        gkEventLogger = std::move(eventLogger);
        m_xmppClient = std::move(xmppClient);

        //
        // QXmpp and XMPP related
        //
        gkConnDetails = connection_details;
        m_registerManager = m_xmppClient->getRegistrationMgr();

        QObject::connect(m_xmppClient, SIGNAL(sendRegistrationForm(const QXmppRegisterIq &)),
                         this, SLOT(recvRegistrationForm(const QXmppRegisterIq &)));

        QObject::connect(this, SIGNAL(registerUser(const std::unique_ptr<QXmppRegisterIq> &)),
                         this, SLOT(sendRegistrationForm(const std::unique_ptr<QXmppRegisterIq> &)));

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

        if (m_xmppClient->isConnected()) {
            //
            // Disconnect from the server since filling out the form may take some time, and we might
            // timeout on the connection otherwise!
            //
            m_xmppClient->disconnectFromServer();
        }

        QObject::connect(m_xmppClient, &QXmppClient::connected, m_xmppClient, [=]() {
            QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::registrationFailed, [=](const QXmppStanza::Error &error) {
                gkEventLogger->publishEvent(tr("Requesting the registration form failed:\n\n%1").arg(error.text()), GkSeverity::Fatal, "",
                                            false, true, false, true);
            });

            QObject::connect(m_registerManager.get(), &QXmppRegistrationManager::passwordChangeFailed, [=](const QXmppStanza::Error &error) {
                gkEventLogger->publishEvent(tr("An attempt to change the password has failed:\n\n%1").arg(error.text()), GkSeverity::Fatal, "",
                                            false, true, false, true);
            });
        });
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

GkXmppRegistrationDialog::~GkXmppRegistrationDialog()
{
    delete ui;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_signup_submit_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_signup_submit_clicked()
{
    QString username;
    QString email;
    QString new_password;
    quint16 network_port;
    QString captcha;

    if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_signup_ui) {
        //
        // Account signup!
        email = ui->lineEdit_email->text();
        username = ui->lineEdit_username->text();
        new_password = ui->lineEdit_password->text();
        network_port = ui->spinBox_xmpp_port->value();

        // Captcha
        captcha = ui->lineEdit_xmpp_captcha_input->text();

        if (username.isEmpty()) {
            // Username field is empty!
            QMessageBox::warning(this, tr("Empty field!"), tr("The username field cannot be empty!"), QMessageBox::Ok);
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

        m_reg_username = username;
        m_reg_email = email;
        m_reg_password = new_password;
        m_reg_captcha = captcha;

        userSignup(gkConnDetails.server.url, network_port, username, new_password);
        // emit registerUser(m_registerForm);
        return;
    }

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
        QString username;
        QString old_password;
        QString captcha;
        quint16 network_port;

        if (ui->stackedWidget_xmpp_registration_dialog->currentWidget() == ui->page_account_login_ui) {
            //
            // Account login!
            username = ui->lineEdit_login_username->text();
            old_password = ui->lineEdit_login_password->text();
            network_port = ui->spinBox_login_xmpp_port->value();

            // Captcha
            captcha = ui->lineEdit_xmpp_login_captcha_input->text();

            if (username.isEmpty()) {
                // Username field is empty!
                QMessageBox::warning(this, tr("Empty field!"), tr("The username field cannot be empty!"), QMessageBox::Ok);
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

            //
            // Attempt a login to the server!
            loginToServer(gkConnDetails.server.url, network_port, username, old_password);

            return;
        } else {
            //
            // Default...

            return;
        }
    } catch (const std::exception &e) {
        print_exception(e);
    }

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
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_change_email_submit_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_change_email_submit_clicked()
{
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
 * @brief GkXmppRegistrationDialog::recvRegistrationForm receives the registration form necessary for the sign-up of a
 * new user to the desired XMPP server in question. Without this, you are neither able to sign-up nor login to a given
 * XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param registerIq The user registration form to be filled out, as received from the connected towards XMPP server.
 * @note QXmppRegistrationManager Class Reference <https://doc.qxmpp.org/qxmpp-dev/classQXmppRegistrationManager.html>.
 * @see GkXmppRegistrationDialog::sendRegistrationForm().
 */
void GkXmppRegistrationDialog::recvRegistrationForm(const QXmppRegisterIq &registerIq)
{
    m_netState = GkNetworkState::WaitForRegistrationForm;
    m_registerForm = std::make_unique<QXmppRegisterIq>(registerIq);
    m_id = registerIq.id();

    gkEventLogger->publishEvent(tr("Received the registration form for XMPP server:\n\n%1")
                                        .arg(gkConnDetails.server.url), GkSeverity::Debug, "", false,
                                true, false, false);

    return;
}

/**
 * @brief GkXmppRegistrationDialog::sendRegistrationForm attempts to sign-up a user with the given XMPP server by
 * submitting the registration form received by function, `GkXmppRegistrationDialog::recvRegistrationForm()`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param registerIq The user registration form to be filled out, as received from the connected towards XMPP server.
 * @note QXmppRegistrationManager Class Reference <https://doc.qxmpp.org/qxmpp-dev/classQXmppRegistrationManager.html>.
 * @see GkXmppRegistrationDialog::recvRegistrationForm().
 */
void GkXmppRegistrationDialog::sendRegistrationForm(const std::unique_ptr<QXmppRegisterIq> &registerIq)
{
    QXmppRegisterIq registration_form;
    try {
        registration_form.setForm(registerIq->form());
        registration_form.setInstructions(registerIq->instructions());
        registration_form.setEmail(m_reg_email);
        registration_form.setPassword(m_reg_password);
        registration_form.setUsername(m_reg_username);
        registration_form.setType(QXmppIq::Type::Set);

        QObject::connect(m_xmppClient, &QXmppClient::connected, [=]() {
            m_registerManager->setRegistrationFormToSend(registration_form);
            m_registerManager->sendCachedRegistrationForm();

            gkEventLogger->publishEvent(tr("User, \"%1\", has been registered with XMPP server:\n\n%2")
            .arg(gkConnDetails.username).arg(gkConnDetails.server.url), GkSeverity::Info, "", false,
            true, false, true);
        });
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("Issues were encountered with trying to register user with XMPP server! Error:\n\n%1")
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
void GkXmppRegistrationDialog::loginToServer(const QString &hostname, const quint16 &network_port, const QString &username,
                                             const QString &password, const bool &credentials_fail)
{
    try {
        //
        // Disconnect from server if already connected...
        if (!m_xmppClient->isConnected()) {
            m_xmppClient->disconnectFromServer();
        }

        if (!username.isEmpty() && !password.isEmpty()) {
            //
            // A username and password has been provided!
            QString user_hostname = m_xmppClient->getHostname(username); // Just in-case the user has entered a differing hostname!
            if (!user_hostname.isEmpty()) {
                //
                // A hostname has been provided as well and thusly derived from the username!
                m_xmppClient->createConnectionToServer(user_hostname, network_port, username, password);
                return;
            }

            //
            // No hostname has been provided, therefore we will use the pre-configured settings (if any)!
            m_xmppClient->createConnectionToServer(hostname, network_port, username, password);
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
 * @param hostname The XMPP server in question to connect towards (i.e. `xmpp.example.com`, without the quotes).
 * @param network_port The network port to connect towards, if manually specified.
 * @param username The username of the registrant in question.
 * @param password The password of the registrant in question.
 * @see GkXmppClient::createConnectionToServer().
 */
void GkXmppRegistrationDialog::userSignup(const QString &hostname, const quint16 &network_port, const QString &username,
                                          const QString &password)
{
    try {
        if (m_xmppClient->isConnected()) {
            m_xmppClient->disconnectFromServer();
        }

        if (username.isEmpty() || password.isEmpty()) {
            throw std::invalid_argument(tr("Provided username and/or password are empty! Unable to continue with "
                                           "user registration.").toStdString());
        }

        //
        // A username and password has been provided!
        QString user_hostname = m_xmppClient->getHostname(username); // Just in-case the user has entered a differing hostname!
        if (!user_hostname.isEmpty()) {
            //
            // A hostname has been provided as well and thusly derived from the username!
            m_xmppClient->createConnectionToServer(user_hostname, network_port, username, password);
            return;
        }

        m_xmppClient->createConnectionToServer(hostname, network_port, username, password, true);
    } catch (const std::exception &e) {
        print_exception(e);
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
