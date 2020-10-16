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
GkPaStreamHandler::GkPaStreamHandler(QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                                     QPointer<GekkoFyre::GkEventLogger> eventLogger, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                     GkAudioChannels audio_channels, QObject *parent) : gkData()
{
    try {
        setParent(parent);

        gkDb = std::move(database);
        gkEventLogger = std::move(eventLogger);
        gkStringFuncs = std::move(stringFuncs);

        //
        // Initialize variables
        //
        pref_output_device = output_device;
        gkAudioChannels = audio_channels;
        channelCount = gkDb->convertAudioChannelsToCount(gkAudioChannels);
        PaError error = paNoError;

        if (gkAudioChannels != GkAudioChannels::Unknown) {
            error = Pa_Initialize();
            gkEventLogger->handlePortAudioErrorCode(error, tr("Problem initializing PortAudio itself!"));

            PaStreamParameters out_param;
            out_param.device = pref_output_device.stream_parameters.device;
            out_param.channelCount = gkDb->convertAudioChannelsToCount(gkAudioChannels);
            out_param.hostApiSpecificStreamInfo = nullptr;
            out_param.sampleFormat = paFloat32 | paNonInterleaved;
            // out_param.suggestedLatency = gkPortAudioSys->deviceByIndex(pref_output_device.stream_parameters.device).defaultLowOutputLatency();

            error = Pa_OpenStream(&gkPaStream, nullptr, &out_param, pref_output_device.def_sample_rate, AUDIO_FRAMES_PER_BUFFER,
                                  paNoFlag, &portAudioCallback, gkData.data());
            gkEventLogger->handlePortAudioErrorCode(error, tr("Problem initializing an audio stream!"));
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
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>.
 * @param audioEventType
 * @param audioFile
 * @param loop
 */
void GkPaStreamHandler::processEvent(AudioEventType audioEventType, GkAudioFramework::SndFileCallback *audioFile, bool loop)
{
    switch (audioEventType) {
        case start:
            if (Pa_IsStreamStopped(gkPaStream)) {
                Pa_StartStream(gkPaStream);
            }

            gkData.push_back(new GkPlayback {audioFile, 0, loop });
            break;
        case stop:
            Pa_StopStream(gkPaStream);
            for (auto instance: gkData) {
                delete instance;
            }

            gkData.clear();
            break;
    }
}

/**
 * @brief GkPaGkPaStreamHandler::portAudioCallback
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param input
 * @param output
 * @param frameCount
 * @param paTimeInfo
 * @param statusFlags
 * @param userData
 * @return
 */
qint32 GkPaStreamHandler::portAudioCallback(const void *input, void *output, size_t frameCount, const PaStreamCallbackTimeInfo *paTimeInfo,
                                            PaStreamCallbackFlags statusFlags, void *userData)
{
    Q_UNUSED(input);
    Q_UNUSED(paTimeInfo);
    Q_UNUSED(statusFlags);

    GkPaStreamHandler *handler = (GkPaStreamHandler *)userData;
    if (channelCount > 0) {
        size_t stereoFrameCount = frameCount * channelCount;
        std::memset((qint32 *)output, 0, stereoFrameCount * sizeof(qint32));

        if (handler->gkData.size() > 0) {
            auto it = handler->gkData.begin();
            while (it != handler->gkData.end()) {
                GkPlayback *data_ptr = (*it);

                qint32 *outputBuffer = new qint32[stereoFrameCount];
                qint32 *bufferCursor = outputBuffer;

                quint32 framesLeft = (quint32)frameCount;
                quint32 framesRead;

                bool playbackEnded = false;
                while (framesLeft > 0) {
                    sf_seek(data_ptr->audioFile->file, data_ptr->position, SEEK_SET);
                    if (framesLeft > (data_ptr->audioFile->info.frames - data_ptr->position)) {
                        framesRead = (quint32) (data_ptr->audioFile->info.frames - data_ptr->position);
                        if (data_ptr->loop) {
                            data_ptr->position = 0;
                        } else {
                            playbackEnded = true;
                            framesLeft = framesRead;
                        }
                    } else {
                        framesRead = framesLeft;
                        data_ptr->position += framesRead;
                    }

                    sf_readf_int(data_ptr->audioFile->file, bufferCursor, framesRead);
                    bufferCursor += framesRead;
                    framesLeft -= framesRead;
                }

                qint32 *outputCursor = (qint32 *)output;
                if (data_ptr->audioFile->info.channels == 1) {
                    for (size_t i = 0; i < stereoFrameCount; ++i) {
                        *outputCursor += (0.5 * outputBuffer[i]);
                        ++outputCursor;
                        *outputCursor += (0.5 * outputBuffer[i]);
                        ++outputCursor;
                    }
                } else {
                    for (size_t i = 0; i < stereoFrameCount; ++i) {
                        *outputCursor += (0.5 * outputBuffer[i]);
                        ++outputCursor;
                    }
                }

                if (playbackEnded) {
                    it = handler->gkData.erase(it);
                    delete data_ptr;
                } else {
                    ++it;
                }
            }
        }
    } else {
        std::throw_with_nested(std::invalid_argument(tr("Invalid amount of audio channels given to PortAudio! Should be greater than zero, and not '%1'.")
        .arg(QString::number(channelCount)).toStdString()));
    }

    return paContinue;
}
