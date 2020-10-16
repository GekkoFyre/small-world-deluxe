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

/**
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>.
 */
GkPaAudioPlayer::GkPaAudioPlayer(QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                                 QPointer<GekkoFyre::GkEventLogger> eventLogger, GekkoFyre::Database::Settings::GkAudioChannels audio_channels,
                                 QPointer<GekkoFyre::StringFuncs> stringFuncs, QObject *parent)
{
    gkStringFuncs = std::move(stringFuncs);

    fileHandler = std::make_unique<GkPaAudioFileHandler>(eventLogger);
    streamHandler = std::make_unique<GkPaStreamHandler>(database, output_device, eventLogger, stringFuncs, audio_channels, parent);;

    return;
}

GkPaAudioPlayer::~GkPaAudioPlayer()
{}

/**
 * @brief GkPaAudioPlayer::play
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>.
 * @param audio_file
 */
void GkPaAudioPlayer::play(QString audio_file)
{
    try {
        streamHandler->processEvent(AudioEventType::start, &fileHandler->getSound(audio_file.toStdString()), false);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A stream processing error has occurred with regards to the PortAudio library handling functions. Error:\n\n%1")
        .arg(QString::fromStdString(e.what())), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief  GkPaAudioPlayer::loop
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>.
 * @param audio_file
 */
void GkPaAudioPlayer::loop(QString audio_file)
{
    try {
        streamHandler->processEvent(AudioEventType::start, &fileHandler->getSound(audio_file.toStdString()), true);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A stream processing error has occurred with regards to the PortAudio library handling functions. Error:\n\n%1")
        .arg(QString::fromStdString(e.what())), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkPaAudioPlayer::stop
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>.
 */
void GkPaAudioPlayer::stop()
{
    streamHandler->processEvent(AudioEventType::stop);

    return;
}
