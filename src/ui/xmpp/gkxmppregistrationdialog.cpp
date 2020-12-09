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
 **   Copyright (C) 2020. GekkoFyre.
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
#include <utility>
#include <QRegularExpression>
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

        //
        // QXmpp and XMPP related
        //
        gkConnDetails = connection_details;
        gkXmppClient = std::move(xmppClient);
        gkXmppRegistrationMgr = std::make_unique<QXmppRegistrationManager>();
        gkXmppClient->addExtension(gkXmppRegistrationMgr.get());

        if (gkXmppRegistrationMgr->supportedByServer()) {
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
            QRegularExpression rxUsername(R"(\b[A-Za-z0-9_]\b)", QRegularExpression::CaseInsensitiveOption);
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

            if (gkXmppClient->isConnected()) {
                //
                // Disconnect from the server since filling out the form may take some time, and we might
                // timeout on the connection otherwise!
                //
                gkXmppClient->disconnectFromServer();
            }

            if (gkXmppRegistrationMgr) { // Verify that the object exists, and the extension is activated at the given server!
                QObject::connect(gkXmppRegistrationMgr.get(), &QXmppRegistrationManager::registrationFormReceived, [=](const QXmppRegisterIq &iq) {
                    qDebug() << "Form received:" << iq.instructions();
                    registerIqReceived(iq); // You now need to complete the form!
                });

                QObject::connect(gkXmppRegistrationMgr.get(), &QXmppRegistrationManager::registrationFailed, [=](const QXmppStanza::Error &error) {
                    gkEventLogger->publishEvent(tr("Requesting the registration form failed:\n\n%1").arg(error.text()), GkSeverity::Fatal, "",
                                                false, true, false, true);
                });

                QObject::connect(gkXmppRegistrationMgr.get(), &QXmppRegistrationManager::passwordChangeFailed, [=](const QXmppStanza::Error &error) {
                    gkEventLogger->publishEvent(tr("An attempt to change the password has failed:\n\n%1").arg(error.text()), GkSeverity::Fatal, "",
                                                false, true, false, true);
                });
            }
        } else {
            QMessageBox::critical(this, tr("Error!"), tr(""), QMessageBox::Ok);
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Error!"));
            msgBox.setText(tr(R"(User registration is not supported by this server. Aborting...")"));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            int ret = msgBox.exec();

            switch (ret) {
                case QMessageBox::Ok:
                    this->close();
                    break;
                default:
                    this->close();
                    break;
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
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
    QString username = ui->lineEdit_username->text();
    QString email = ui->lineEdit_email->text();
    QString password = ui->lineEdit_password->text();
    QString captcha = ui->lineEdit_xmpp_captcha_input->text();

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

    if (password.isEmpty()) {
        // Password field is empty!
        QMessageBox::warning(this, tr("Empty field!"), tr("The password field cannot be empty!"), QMessageBox::Ok);
        return;
    }

    if (captcha.isEmpty()) {
        // Captcha field is empty!
        QMessageBox::warning(this, tr("Empty field!"), tr("The captcha field cannot be empty!"), QMessageBox::Ok);
        return;
    }

    //
    // All fields should be valid, therefore sign-up this user!
    sendFilledRegistrationForm(username, email, password, captcha);

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
    return;
}

/**
 * @brief GkXmppRegistrationDialog::on_pushButton_login_reset_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkXmppRegistrationDialog::on_pushButton_login_reset_clicked()
{
    ui->lineEdit_login_username->clear();
    ui->lineEdit_login_password->clear();
    ui->lineEdit_xmpp_login_captcha_input->clear();

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
 * @brief GkXmppRegistrationDialog::handleRegistrationForm
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param registerIq
 */
void GkXmppRegistrationDialog::handleRegistrationForm(const QXmppRegisterIq &registerIq)
{
    if (!registerIq.form().isNull()) {
        QString errorMsg = tr("Registration at this server requires support for XMPP Data Forms. Initializing...");
        if (!registerIq.instructions().isEmpty()) {
            errorMsg = tr("%1\n\nServer message: %2").arg(errorMsg).arg(registerIq.instructions());
        }

        std::cout << errorMsg.toStdString() << std::endl;
    }

    m_netState = GkNetworkState::WaitForRegistrationConfirmation;
    return;
}

/**
 * @brief GkXmppRegistrationDialog::registerIqReceived
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param registerIq
 */
void GkXmppRegistrationDialog::registerIqReceived(const QXmppRegisterIq &registerIq)
{
    return;
}

/**
 * @brief GkXmppRegistrationDialog::sendFilledRegistrationForm attempts to sign-up a user with the given XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param user The given username to sign-up with.
 * @param email The given email address for this particular user account, upon which a verification email will be sent out
 * in order to verify that this is a real user and not a spam account.
 * @param password The given password to use with this user account.
 * @param captcha The captcha secret, otherwise the signing-up process will not proceed!
 * @note QXmppRegistrationManager Class Reference <https://doc.qxmpp.org/qxmpp-dev/classQXmppRegistrationManager.html>,
 * Kadu by Rafał Malinowski <https://github.com/vogel/kadu/blob/master/plugins/jabber_protocol/services/jabber-register-account.cpp>.
 */
void GkXmppRegistrationDialog::sendFilledRegistrationForm(const QString &user, const QString &email, const QString &password,
                                                          const QString &captcha)
{
    try {
        auto gkRegisterIq = QXmppRegisterIq {};
        gkRegisterIq.setEmail(email);
        gkRegisterIq.setPassword(password);
        gkRegisterIq.setType(QXmppIq::Type::Set);
        gkRegisterIq.setUsername(user);

        gkXmppRegistrationMgr->setRegistrationFormToSend(gkRegisterIq);
        gkEventLogger->publishEvent(tr("User, \"%1\", has been registered with XMPP server: %2")
                                            .arg(gkConnDetails.jid).arg(gkConnDetails.server.url), GkSeverity::Info,
                                    "", true, true, false, false);
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::setEmailInputColor adjusts the color of a given QLineEdit widget, dependent on
 * whether there's acceptable user input or not. If there's acceptable user input, the text appears as Black, otherwise
 * it will be Red in coloration.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param adj_text Not a used value.
 */
void GkXmppRegistrationDialog::setEmailInputColor(const QString &adj_text)
{
    Q_UNUSED(adj_text);

    if (!ui->lineEdit_email->hasAcceptableInput()) {
        ui->lineEdit_email->setStyleSheet("QLineEdit { color: red; }");
    } else {
        ui->lineEdit_email->setStyleSheet("QLineEdit { color: black; }");
    }

    return;
}

/**
 * @brief GkXmppRegistrationDialog::setUsernameInputColor adjusts the color of a given QLineEdit widget, dependent on
 * whether there's acceptable user input or not. If there's acceptable user input, the text appears as Black, otherwise
 * it will be Red in coloration.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param adj_text Not a used value.
 */
void GkXmppRegistrationDialog::setUsernameInputColor(const QString &adj_text)
{
    Q_UNUSED(adj_text);

    if (!ui->lineEdit_username->hasAcceptableInput()) {
        ui->lineEdit_username->setStyleSheet("QLineEdit { color: red; }");
    } else {
        ui->lineEdit_username->setStyleSheet("QLineEdit { color: black; }");
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
    QObject::disconnect(gkXmppClient, nullptr, this, nullptr);
    if (!errorMsg.isEmpty()) {
        gkEventLogger->publishEvent(errorMsg, GkSeverity::Fatal, "", false, true, false, true);
    }

    deleteLater();
    return;
}
