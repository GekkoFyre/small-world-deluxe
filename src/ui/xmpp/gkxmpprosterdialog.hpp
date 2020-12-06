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
#include "src/gk_xmpp_client.hpp"
#include "src/ui/xmpp/gkxmppmessagedialog.hpp"
#include "src/ui/xmpp/gkxmppregistrationdialog.hpp"
#include "src/gk_logger.hpp"
#include <memory>
#include <QString>
#include <QObject>
#include <QDialog>
#include <QPointer>

namespace Ui {
class GkXmppRosterDialog;
}

class GkXmppRosterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkXmppRosterDialog(const GekkoFyre::Network::GkXmpp::GkUserConn &connection_details,
                                QPointer<GekkoFyre::GkXmppClient> xmppClient,
                                QPointer<GekkoFyre::GkEventLogger> eventLogger, QWidget *parent = nullptr);
    ~GkXmppRosterDialog();

private slots:
    void on_comboBox_current_status_currentIndexChanged(int index);
    void on_pushButton_user_login_clicked();
    void on_pushButton_user_create_account_clicked();

private:
    Ui::GkXmppRosterDialog *ui;

    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // QXmpp and XMPP related
    //
    GekkoFyre::Network::GkXmpp::GkUserConn gkConnDetails;
    QPointer<GekkoFyre::GkXmppClient> gkXmppClient;
    QPointer<QXmppClient> xmppClientPtr;
    QPointer<GkXmppMessageDialog> gkXmppMsgDlg;
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg;

    void prefillAvailComboBox();
};

