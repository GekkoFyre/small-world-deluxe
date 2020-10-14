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
#include "src/dek_db.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_logger.hpp"
#include "src/pa_audio_file.hpp"
#include "src/pa_audio_struct.hpp"
#include <memory>
#include <vector>
#include <string>
#include <QObject>
#include <QString>
#include <QPointer>

#ifdef __cplusplus
extern "C"
{
#endif

#include <portaudio.h>

#ifdef __cplusplus
} // extern "C"
#endif

namespace GekkoFyre {

struct Playback {
    GkAudioFile *audioFile;
    qint32 position;
    bool loop;
};

enum AudioEventType {
    start,
    stop
};

class GkPaStreamHandler : public QObject {
    Q_OBJECT

public:
    explicit GkPaStreamHandler(QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                               QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                               GekkoFyre::Database::Settings::GkAudioChannels audio_channels, QObject *parent = nullptr);
    ~GkPaStreamHandler() override;

    void processEvent(AudioEventType audioEventType, GkAudioFile *audioFile = nullptr, bool loop = false);
    static int portAudioCallback(const void *input, void *output, size_t frameCount, const PaStreamCallbackTimeInfo *paTimeInfo,
                                 PaStreamCallbackFlags statusFlags, void *userData);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    const int CHANNEL_COUNT = 2;
    const int SAMPLE_RATE = 44000;
    const PaStreamParameters *NO_INPUT = nullptr;

    //
    // PortAudio initialization and buffers
    //
    PaStream *gkPaStream;
    GekkoFyre::Database::Settings::GkAudioChannels gkAudioChannels;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_output_device;

    PaStream *stream;
    std::vector<Playback *> data;

};
};
