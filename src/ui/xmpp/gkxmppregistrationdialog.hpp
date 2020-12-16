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

#pragma once

#include "src/defines.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_xmpp_client.hpp"
#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppRegisterIq.h>
#include <qxmpp/QXmppDiscoveryManager.h>
#include <qxmpp/QXmppRegistrationManager.h>
#include <memory>
#include <QString>
#include <QObject>
#include <QDialog>
#include <QPointer>

namespace Ui {
class GkXmppRegistrationDialog;
}

class GkXmppRegistrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkXmppRegistrationDialog(const GekkoFyre::Network::GkXmpp::GkRegUiRole &gkRegUiRole,
                                      const GekkoFyre::Network::GkXmpp::GkUserConn &connection_details,
                                      QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                      QPointer<GekkoFyre::GkEventLogger> eventLogger, QWidget *parent = nullptr);
    ~GkXmppRegistrationDialog() override;

private slots:
    void on_pushButton_signup_submit_clicked();
    void on_pushButton_signup_reset_clicked();
    void on_pushButton_signup_cancel_clicked();
    void on_pushButton_login_submit_clicked();
    void on_pushButton_login_reset_clicked();
    void on_pushButton_login_cancel_clicked();
    void on_toolButton_xmpp_captcha_refresh_clicked();
    void on_toolButton_xmpp_login_captcha_refresh_clicked();
    void on_pushButton_continue_clicked();
    void on_pushButton_retry_clicked();
    void on_pushButton_exit_clicked();

    void on_pushButton_change_password_reset_clicked();
    void on_pushButton_change_password_cancel_clicked();
    void on_pushButton_change_password_submit_clicked();

    void on_pushButton_change_email_submit_clicked();
    void on_pushButton_change_email_reset_clicked();
    void on_pushButton_change_email_cancel_clicked();

    void askForRegistration();
    void handleRegistrationForm(const QXmppRegisterIq &registerIq);
    void handleRegistrationConfirmation(const QXmppRegisterIq &registerIq);
    void registerIqReceived(QXmppRegisterIq registerIq);
    void sendFilledRegistrationForm();

    void setEmailInputColor(const QString &adj_text);
    void setUsernameInputColor(const QString &adj_text);

    void clientError(QXmppClient::Error error);
    void handleError(const QString &errorMsg);
    void handleSuccess();

signals:
    void sendError(const QString &errorMsg);

private:
    Ui::GkXmppRegistrationDialog *ui;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // QXmpp and XMPP related
    //
    GekkoFyre::Network::GkXmpp::GkUserConn gkConnDetails;
    QPointer<GekkoFyre::GkXmppClient> m_xmppClient;
    std::shared_ptr<QXmppRegistrationManager> m_registerManager;

    //
    // Registration details for the user in question...
    QString m_username;
    QString m_email;
    QString m_password;
    QString m_captcha;

    GekkoFyre::Network::GkXmpp::GkNetworkState m_netState;
    QString m_id;
};

