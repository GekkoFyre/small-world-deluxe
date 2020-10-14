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

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;

/**
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>.
 */
GkPaStreamHandler::GkPaStreamHandler(QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                                     QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                     GkAudioChannels audio_channels, QObject *parent)
                                     : data()
{
    setParent(parent);

    gkDb = std::move(database);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    //
    // Initialize variables
    //
    pref_output_device = output_device;
    gkAudioChannels = audio_channels;
    PaError error;
    
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
                              paNoFlag, &portAudioCallback, this);
        gkEventLogger->handlePortAudioErrorCode(error, tr("Problem initializing an audio stream!"));
    } else {
        throw std::invalid_argument(tr("Unable to determine the amount of channels required for audio playback!").toStdString());
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
void GkPaStreamHandler::processEvent(AudioEventType audioEventType, GkAudioFile *audioFile, bool loop)
{
    switch (audioEventType) {
        case start:
            if (Pa_IsStreamStopped(stream)) {
                Pa_StartStream(stream);
            }

            data.push_back(new Playback { audioFile,0, loop });
            break;
        case stop:
            Pa_StopStream(stream);
            for (auto instance: data) {
                delete instance;
            }

            data.clear();
            break;
    }
}

/**
 * @brief GkPaGkPaStreamHandler::portAudioCallback
 * @author Copyright (c) 2015 Andy Stanton <https://github.com/andystanton/sound-example/blob/master/LICENSE>.
 * @param input
 * @param output
 * @param frameCount
 * @param paTimeInfo
 * @param statusFlags
 * @param userData
 * @return
 */
int GkPaStreamHandler::portAudioCallback(const void *input, void *output, size_t frameCount, const PaStreamCallbackTimeInfo *paTimeInfo,
                                         PaStreamCallbackFlags statusFlags, void *userData)
{
    GkPaStreamHandler *handler = (GkPaStreamHandler *)userData;

    size_t stereoFrameCount = frameCount * handler->CHANNEL_COUNT;
    std::memset((int *) output, 0, stereoFrameCount * sizeof(int));

    if (handler->data.size() > 0) {
        auto it = handler->data.begin();
        while (it != handler->data.end()) {
            Playback *data = (*it);
            GkAudioFramework::SndFileCallback *audioFile = (GkAudioFramework::SndFileCallback *)data->audioFile;

            int *outputBuffer = new int[stereoFrameCount];
            int *bufferCursor = outputBuffer;

            quint32 framesLeft = (quint32)frameCount;
            quint32 framesRead;

            bool playbackEnded = false;
            while (framesLeft > 0) {
                sf_seek(audioFile->file, data->position, SEEK_SET);
                if (framesLeft > (audioFile->info.frames - data->position)) {
                    framesRead = (quint32) (audioFile->info.frames - data->position);
                    if (data->loop) {
                        data->position = 0;
                    } else {
                        playbackEnded = true;
                        framesLeft = framesRead;
                    }
                } else {
                    framesRead = framesLeft;
                    data->position += framesRead;
                }

                sf_readf_int(audioFile->file, bufferCursor, framesRead);
                bufferCursor += framesRead;
                framesLeft -= framesRead;
            }


            int *outputCursor = (int *)output;
            if (audioFile->info.channels == 1) {
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
                it = handler->data.erase(it);
                delete data;
            } else {
                ++it;
            }
        }
    }

    return paContinue;
}
