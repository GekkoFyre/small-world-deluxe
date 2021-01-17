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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/models/system/gk_network_ping_model.hpp"
#include <utility>
#include <QStringList>

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

GkNetworkPingModel::GkNetworkPingModel(QPointer<GkEventLogger> &eventLogger, QObject *parent) : QThread(parent)
{
    setParent(parent);
    gkEventLogger = std::move(eventLogger);

    pingProc = new QProcess(this);
    QObject::connect(pingProc, SIGNAL(started()), this, SLOT(verifyStatus()));
    QObject::connect(pingProc, SIGNAL(finished(qint32)), this, SLOT(readResult()));

    start();

    // Move event processing of GkNetworkPingModel to this thread
    QObject::moveToThread(this);
    return;
}

GkNetworkPingModel::~GkNetworkPingModel()
{
    quit();
    wait();

    return;
}

/**
 * @brief GkNetworkPingModel::run
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkNetworkPingModel::run()
{
    exec();
    return;
}

/**
 * @brief
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param host
 */
void GkNetworkPingModel::ping(const QHostAddress &host)
{
    try {
        if (pingProc) {
            QHostInfo info = QHostInfo::fromName(host.toString()); // WARNING: Blocking lookup function! Not asymmetric...
            if (info.error() == QHostInfo::NoError) {
                QString cmd = "ping";
                QStringList args;

                for (const auto &address: info.addresses()) {
                    #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
                    args << QString("-w%1 -n%2").arg(QString::number(GK_NETWORK_PING_TIMEOUT_MILLISECS))
                    .arg(QString::number(GK_NETWORK_PING_COUNT)) << address.toString();
                    #elif __linux__
                    args << QString("-w%1 -c%2").arg(QString::number(GK_NETWORK_PING_TIMEOUT_MILLISECS / 1000))
                    .arg(QString::number(GK_NETWORK_PING_COUNT)) << address.toString();
                    #endif

                    pingProc->start(cmd, args);
                    pingProc->waitForStarted(7000);
                    running = true;
                    pingProc->waitForFinished(5000);
                }
            } else {
                throw std::runtime_error(tr("An issue has occurred while resolving a hostname! Error:\n\n%1")
                .arg(info.errorString()).toStdString());
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
bool GkNetworkPingModel::isRunning()
{
    return running;
}

/**
 * @brief
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
bool GkNetworkPingModel::isFinished()
{
    return pingProc->atEnd();
}

/**
 * @brief
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkNetworkPingModel::verifyStatus()
{
    if (pingProc->isReadable()) {
        QObject::connect(pingProc, SIGNAL(readyRead()), this, SLOT(readResult()));
    }

    return;
}

/**
 * @brief
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkNetworkPingModel::readResult()
{
    running = false;
    gkEventLogger->publishEvent(pingProc->readLine(), GkSeverity::Info, "", false, true, true, false);

    return;
}
