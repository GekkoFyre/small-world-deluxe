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

#include "src/gk_fyr_data.hpp"
#include <utility>
#include <exception>
#include <QMessageBox>
#include <QDomElement>

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
 * @brief GkFyrFormat::GkFyrFormat
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @note Recorded Playback <https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/wikis/Features/Recorded-Playback>.
 */
GkFyrFormat::GkFyrFormat(QObject *parent) : QObject(parent)
{
    setParent(parent);

    return;
}

GkFyrFormat::~GkFyrFormat()
{}

/**
 * @brief GkFyrFormat::calcTotalTime
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFyrFormat::calcTotalTime()
{
    return;
}

GkFyrData::GkFyrData(QPointer<GekkoFyre::GkLevelDb> database, QPointer<GekkoFyre::GkFFTAudio> fftAudio,
                     QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QThread(parent)
{
    setParent(parent);

    gkEventLogger = std::move(eventLogger);
    gkDb = std::move(database);
    gkFftAudio = std::move(fftAudio);

    m_data = new GkFyrFormat(this);
    start();

    // Move event processing of GkFyrData to this thread
    QObject::moveToThread(this);
}

GkFyrData::~GkFyrData()
{
    quit();
    wait();
}

/**
 * @brief GkXmppClient::run
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFyrData::run()
{
    exec();
    return;
}

/**
 * @brief GkFyrData::createFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filePath
 */
void GkFyrData::createFile(const fs::path &filePath)
{
    return;
}
