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

#include "src/pa_audio_player.hpp"

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

/**
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkPaAudioPlayer::GkPaAudioPlayer(portaudio::System *portAudioSys, QPointer<GekkoFyre::GkLevelDb> database,
                                 const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                                 const QPointer<GekkoFyre::GkEventLogger> &eventLogger,
                                 QPointer<GekkoFyre::StringFuncs> stringFuncs, QObject *parent)
{
    gkPortAudioSys = portAudioSys;
    gkStringFuncs = std::move(stringFuncs);

    streamHandler = new GkPaStreamHandler(gkPortAudioSys, std::move(database), output_device, eventLogger, stringFuncs, parent);;

    return;
}

GkPaAudioPlayer::~GkPaAudioPlayer()
{}

/**
 * @brief GkPaAudioPlayer::play
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_file
 */
void GkPaAudioPlayer::play(const fs::path &audio_file)
{
    try {
        streamHandler->processEvent(AudioEventType::start, audio_file, false);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A stream processing error has occurred with regards to the PortAudio library handling functions. Error:\n\n%1")
        .arg(QString::fromStdString(e.what())), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief  GkPaAudioPlayer::loop
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_file
 */
void GkPaAudioPlayer::loop(const fs::path &audio_file)
{
    try {
        streamHandler->processEvent(AudioEventType::start, audio_file, true);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A stream processing error has occurred with regards to the PortAudio library handling functions. Error:\n\n%1")
        .arg(QString::fromStdString(e.what())), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkPaAudioPlayer::stop
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_file
 */
void GkPaAudioPlayer::stop(const fs::path &audio_file)
{
    try {
        streamHandler->processEvent(AudioEventType::stop, audio_file);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A stream processing error has occurred with regards to the PortAudio library handling functions. Error:\n\n%1")
                .arg(QString::fromStdString(e.what())), QMessageBox::Ok);
    }

    return;
}
