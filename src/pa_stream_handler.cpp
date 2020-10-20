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
{
    for (auto entry: gkSounds) {
        sf_close(entry.second.audioFile.file);
    }
}

/**
 * @brief GkPaStreamHandler::containsSound
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filename
 * @return
 */
bool GkPaStreamHandler::containsSound(const std::string &filename)
{
    return gkSounds.find(filename) != gkSounds.end();
}

/**
 * @brief GkPaStreamHandler::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param audioFile
 * @param loop
 */
void GkPaStreamHandler::processEvent(AudioEventType audioEventType, const std::string &mediaFilePath, bool loop)
{
    switch (audioEventType) {
        case start:
        {
            for (const auto &mediaInfo: gkSounds) {
                if (mediaInfo.first == mediaFilePath) {
                    PaError error = paNoError;
                    PaTime prefOutputLatency = gkPortAudioSys->deviceByIndex(pref_output_device.stream_parameters.device).defaultLowOutputLatency();
                    portaudio::DirectionSpecificStreamParameters outputParams(gkPortAudioSys->deviceByIndex(pref_output_device.stream_parameters.device),
                                                                              mediaInfo.second.audioFile.info.channels, portaudio::INT32, false, prefOutputLatency, nullptr);
                    portaudio::StreamParameters playback(portaudio::DirectionSpecificStreamParameters::null(), outputParams, mediaInfo.second.audioFile.info.samplerate,
                                                         AUDIO_FRAMES_PER_BUFFER, paPrimeOutputBuffersUsingStreamCallback);
                    streamPlayback = std::make_shared<portaudio::MemFunCallbackStream<GkPaStreamHandler>>(playback, *this, &GkPaStreamHandler::portAudioCallback);

                    gkEventLogger->handlePortAudioErrorCode(error, tr("Problem initializing an audio stream!"));

                    if (streamPlayback->isStopped()) {
                        streamPlayback->start();
                    }

                    gkData.push_back(mediaInfo.second);
                    break;
                }
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

    if (!gkData.empty()) {
        auto it = gkData.begin();
        while (it != gkData.end()) {
            GkAudioFramework::GkPlayback data_ptr = (*it);
            if (data_ptr.audioFile.info.channels > 0) {
                size_t stereoFrameCount = frameCount * data_ptr.audioFile.info.channels;

                qint32 *out = (qint32 *)output;
                auto data_buf = std::make_unique<qint32[]>(stereoFrameCount);
                sf_seek(data_ptr.audioFile.file, data_ptr.position, SEEK_SET);

                // https://stackoverflow.com/questions/22889766/difference-between-frames-and-items-in-libsndfile
                data_ptr.count = sf_readf_int(data_ptr.audioFile.file, data_buf.get(), frameCount);

                Q_ASSERT(data_ptr.count == frameCount);
                for (qint32 i = 0; i < stereoFrameCount; ++i) {
                    *out++ = data_buf[i];
                }

                data_ptr.position += AUDIO_FRAMES_PER_BUFFER;
                if (data_ptr.count > 0) {
                    return paContinue;
                } else {
                    return paComplete;
                }
            } else {
                std::throw_with_nested(std::invalid_argument(tr("Invalid amount of audio channels given to PortAudio! Should be greater than zero, and not '%1'.")
                .arg(QString::number(data_ptr.audioFile.info.channels)).toStdString()));
            }
        }
    } else {
        std::throw_with_nested(std::invalid_argument(tr("Empty data pointer given to PortAudio!").toStdString()));
    }

    return paContinue;
}
