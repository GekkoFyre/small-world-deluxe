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

#include "src/pa_audio_file.hpp"
#include <utility>
#include <exception>

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
GkPaAudioFileHandler::GkPaAudioFileHandler(QPointer<GekkoFyre::GkEventLogger> eventLogger) : gkSounds()
{
    gkEventLogger = std::move(eventLogger);

    return;
}

GkPaAudioFileHandler::~GkPaAudioFileHandler()
{
    for (auto entry: gkSounds) {
        sf_close(entry.second.file);
    }
}

/**
 * @brief GkPaAudioFileHandler::containsSound
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>.
 * @param filename
 * @return
 */
bool GkPaAudioFileHandler::containsSound(std::string filename)
{
    return gkSounds.find(filename) != gkSounds.end();
}

/**
 * @brief GkPaAudioFileHandler::getSound
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>.
 * @param filename The **FULL** filesystem path to the audio file in question!
 * @return
 */
GkAudioFramework::SndFileCallback &GkPaAudioFileHandler::getSound(std::string filename)
{
    if (!containsSound(filename)) {
        GkAudioFramework::SndFileCallback sound;
        SF_INFO info;
        info.format = 0;
        SNDFILE *audioFile = sf_open(filename.c_str(), SFM_READ, &info);

        if (!audioFile) {
            std::stringstream error;
            gkEventLogger->publishEvent(QString("Unable to open audio file, \"%1\", due to filesystem error!").arg(QString::fromStdString(filename)),
                                        GkSeverity::Error, "", true, true, false, false);
            std::throw_with_nested(std::runtime_error(QString("Unable to open audio file, \"%1\", due to filesystem error!").arg(QString::fromStdString(filename)).toStdString()));
        }

        gkSounds[filename] = sound;
    }
    
    return gkSounds[filename];
}
