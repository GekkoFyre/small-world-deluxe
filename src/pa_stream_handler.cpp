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

#include "src/pa_stream_handler.hpp"
#include <utility>
#include <algorithm>
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
GkPaStreamHandler::GkPaStreamHandler(portaudio::System *portAudioSys, QPointer<GekkoFyre::GkLevelDb> database,
                                     const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                                     QPointer<GekkoFyre::GkEventLogger> eventLogger, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                     QObject *parent)
{
    try {
        setParent(parent);

        gkPortAudioSys = portAudioSys;
        gkDb = std::move(database);
        gkEventLogger = std::move(eventLogger);
        gkStringFuncs = std::move(stringFuncs);

        //
        // Initialize variables
        //
        pref_output_device = output_device;
        PaError error = paNoError;
        if (pref_output_device.dev_output_channel_count > 0) {
            error = Pa_Initialize();
            gkEventLogger->handlePortAudioErrorCode(error, tr("Problem initializing PortAudio itself!"));
        } else {
            throw std::invalid_argument(tr("Unable to determine the amount of channels required for audio playback!").toStdString());
        }
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
    }

    return;
}

GkPaStreamHandler::~GkPaStreamHandler()
{}

/**
 * @brief GkPaStreamHandler::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param audioFile
 * @param loop
 * @see Ben Key <https://stackoverflow.com/questions/29249657/playing-wav-file-with-portaudio-and-sndfile>.
 */
void GkPaStreamHandler::processEvent(AudioEventType audioEventType, const fs::path &mediaFilePath, bool loop)
{
    switch (audioEventType) {
        case start:
        {
            PaError error = paNoError;
            if (!(gkPlayback.audioFile.file = sf_open(mediaFilePath.c_str(), SFM_READ, &gkPlayback.audioFile.info))) {
                throw std::invalid_argument(tr("Unable to open audio file, \"%1\", for reading! Error:\n\n%2")
                .arg(QString::fromStdString(mediaFilePath.filename().string())).arg(QString::fromStdString(sf_strerror(nullptr))).toStdString());
            }

            PaTime prefOutputLatency = gkPortAudioSys->deviceByIndex(pref_output_device.stream_parameters.device).defaultLowOutputLatency();
            portaudio::DirectionSpecificStreamParameters outputParams(gkPortAudioSys->deviceByIndex(pref_output_device.stream_parameters.device),
                                                                      gkPlayback.audioFile.info.channels, portaudio::FLOAT32, false, prefOutputLatency, nullptr);
            portaudio::StreamParameters playbackParams(portaudio::DirectionSpecificStreamParameters::null(), outputParams, gkPlayback.audioFile.info.samplerate,
                                                       AUDIO_FRAMES_PER_BUFFER, paPrimeOutputBuffersUsingStreamCallback);
            streamPlayback = std::make_shared<portaudio::MemFunCallbackStream<GkPaStreamHandler>>(playbackParams, *this, &GkPaStreamHandler::portAudioCallback);

            gkEventLogger->handlePortAudioErrorCode(error, tr("Problem initializing an audio stream!"));

            if (streamPlayback->isStopped()) {
                streamPlayback->start();
            }
        }

            break;
        case stop:
        {
            streamPlayback->stop();

            /*
            for (const auto &mediaInfo: gkSounds) {
                if (mediaInfo.first == mediaFilePath) {
                    gkData.erase(std::remove(gkData.begin(), gkData.end(), mediaInfo.second), gkData.end());
                }
            }
            */
        }

            break;
    }
}

/**
 * @brief GkPaGkPaStreamHandler::portAudioCallback
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param input
 * @param output
 * @param frameCount
 * @param paTimeInfo
 * @param statusFlags
 * @param userData
 * @return
 */
qint32 GkPaStreamHandler::portAudioCallback(const void *input, void *output, size_t frameCount, const PaStreamCallbackTimeInfo *paTimeInfo,
                                            PaStreamCallbackFlags statusFlags)
{
    Q_UNUSED(input);
    Q_UNUSED(paTimeInfo);
    Q_UNUSED(statusFlags);

    if (gkPlayback.audioFile.info.channels > 0) {
        if (gkPlayback.audioFile.info.channels > GK_AUDIO_MAX_CHANNELS) {
            std::throw_with_nested(std::invalid_argument(tr("Small World Deluxe is unable to process more than %1-channels at this stage of development. Therefore, the number requested, \"%2\", is invalid!")
            .arg(QString::number(GK_AUDIO_MAX_CHANNELS)).arg(QString::number(gkPlayback.audioFile.info.channels)).toStdString()));
        }

        // https://stackoverflow.com/questions/22889766/difference-between-frames-and-items-in-libsndfile
        sf_count_t total = 0;
        size_t stereoFrameCount = AUDIO_FRAMES_PER_BUFFER * gkPlayback.audioFile.info.channels;
        float data_buf[stereoFrameCount];
        do {
            gkPlayback.count = sf_readf_float(gkPlayback.audioFile.file, data_buf, AUDIO_FRAMES_PER_BUFFER);

            // Make sure we don't read more frames than we allocated...
            if (total + gkPlayback.count > gkPlayback.audioFile.info.frames) {
                gkPlayback.count = gkPlayback.audioFile.info.frames - total;
            }

            for (qint32 ch = 0; ch < gkPlayback.audioFile.info.channels ; ++ch) {
                for (qint32 k = 0; k < gkPlayback.count ; ++k) {
                    out(total + k, ch) = buffer[k * gkPlayback.audioFile.info.channels + ch];
                }
            }

            total += gkPlayback.count;
        } while (gkPlayback.count > 0 && total < gkPlayback.audioFile.info.frames);

        if (gkPlayback.count > 0) {
            return paContinue; // Continue looping this function!
        } else {
            return paComplete; // End this function!
        }
    } else {
        std::throw_with_nested(std::invalid_argument(tr("Invalid amount of audio channels given to PortAudio! Should be greater than zero, and not '%1'.")
        .arg(QString::number(gkPlayback.audioFile.info.channels)).toStdString()));
    }

    return paContinue;
}
