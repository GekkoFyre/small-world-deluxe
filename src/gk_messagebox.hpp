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
 **   Small world is distributed in the hope that it will be useful,
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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <QPointer>
#include <QMessageBox>
#include <exception>
#include <stdexcept>
#include <string>

namespace GekkoFyre {

class GkMessageBox: public QObject {
    Q_OBJECT

public:
    explicit GkMessageBox(const QString &title, const QString &msg, const QMessageBox::StandardButton &buttons,
                          const QMessageBox::StandardButton &def_button, const QMessageBox::Icon &icon,
                          const std::string &error_msg);

public slots:
    void displayErrorMessageBox(const QString &title, const QString &msg, const QMessageBox::StandardButton &buttons,
                                const QMessageBox::StandardButton &def_button, const QMessageBox::Icon &icon,
                                const std::string &error_msg);

};

class GkMessageBoxThread: public QThread {
    Q_OBJECT

public:
    explicit GkMessageBoxThread(const QString &title, const QString &msg, const QMessageBox::StandardButton &buttons,
                                const QMessageBox::StandardButton &def_button, const QMessageBox::Icon &icon,
                                const std::string &error_msg);

signals:
    void showErrorMsgBox(const QString &title, const QString &msg, const QMessageBox::StandardButton &buttons,
                         const QMessageBox::StandardButton &def_button, const QMessageBox::Icon &icon,
                         const std::string &error_msg);

protected:
    void run();

private:
    QString gkTitle;
    QString gkMsg;
    QMessageBox::StandardButton gkButtons;
    QMessageBox::StandardButton gkDefButton;
    QMessageBox::Icon gkIcon;
    std::string gkErrorMsg;

};

/**
 * @brief GkMessageBox::run executes a QMessageBox() when the GUI thread for Qt5 is otherwise not directly accessible!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkMessageBoxThread::GkMessageBoxThread(const QString &title, const QString &msg, const QMessageBox::StandardButton &buttons,
                                       const QMessageBox::StandardButton &def_button, const QMessageBox::Icon &icon,
                                       const std::string &error_msg)
{
    gkTitle = title;
    gkMsg = msg;
    gkButtons = buttons;
    gkDefButton = def_button;
    gkIcon = icon;
    gkErrorMsg = error_msg;

    return;
}

void GkMessageBoxThread::run()
{
    emit showErrorMsgBox(gkTitle, gkMsg, gkButtons, gkDefButton, gkIcon, gkErrorMsg);

    return;
}

/**
 * @brief GkMessageBox::displayErrorMessageBox
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkMessageBox::GkMessageBox(const QString &title, const QString &msg, const QMessageBox::StandardButton &buttons,
                           const QMessageBox::StandardButton &def_button, const QMessageBox::Icon &icon,
                           const std::string &error_msg)
{
    QPointer<GekkoFyre::GkMessageBoxThread> gkMessageBoxThread = new GkMessageBoxThread(title, msg, buttons, def_button, icon, error_msg);
    QObject::connect(gkMessageBoxThread, SIGNAL(showErrorMsgBox(const QString &, const QString &, const QMessageBox::StandardButton &,
                                                                const QMessageBox::StandardButton &, const QMessageBox::Icon &, const std::string &)),
                     this, SLOT(displayErrorMessageBox(const QString &, const QString &, const QMessageBox::StandardButton &,
                                                       const QMessageBox::StandardButton &, const QMessageBox::Icon &, const std::string &)
                                Qt::BlockingQueuedConnection));
    gkMessageBoxThread->start();
}

void GkMessageBox::displayErrorMessageBox(const QString &title, const QString &msg, const QMessageBox::StandardButton &buttons,
                                          const QMessageBox::StandardButton &def_button, const QMessageBox::Icon &icon,
                                          const std::string &error_msg)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(msg);
    msgBox.setStandardButtons(buttons);
    msgBox.setDefaultButton(def_button);
    msgBox.setIcon(icon);
    msgBox.exec();

    return;
}
};
