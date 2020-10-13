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
#include "src/gk_circ_buffer.hpp"
#include <codec2/codec2.h>
#include <exception>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <mutex>
#include <list>

namespace GekkoFyre {

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;

template <class T>
class PaAudioBuf {

public:
    explicit PaAudioBuf(int buffer_size, const GekkoFyre::Database::Settings::Audio::GkDevice &pref_output_device,
                        const GekkoFyre::Database::Settings::Audio::GkDevice &pref_input_device);
    virtual ~PaAudioBuf();

    int playbackCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags);
    int recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags);
    void setVolume(const float &value);

    [[nodiscard]] virtual size_t size() const;
    [[nodiscard]] virtual bool empty() const;
    [[nodiscard]] virtual bool clear() const;
    virtual T grab() const;
    virtual void append(const std::vector<T> &vec);
    [[nodiscard]] virtual bool full() const;

private:
    std::shared_ptr<GekkoFyre::GkCircBuffer<T>> gkCircBuffer;
    GekkoFyre::Database::Settings::Audio::GkDevice prefInputDevice;
    GekkoFyre::Database::Settings::Audio::GkDevice prefOutputDevice;

    int circ_buffer_size;
    int maxFrameIndex;
    int frameIndex;
    float calcVolIdx; // A floating-point value between 0.0 - 1.0 that determines the amplitude of the audio signal (i.e. raw data buffer).

};

/**
 * @brief PaAudioBuf<T>::PaAudioBuf
 * @param buffer_size
 * @param pref_output_device
 * @param pref_input_device
 * @param parent
 */
template<class T>
PaAudioBuf<T>::PaAudioBuf(int buffer_size, const GkDevice &pref_output_device, const GkDevice &pref_input_device)
{
    std::mutex pa_audio_buf_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_mtx);

    // The preferred input and output audio devices
    prefInputDevice = pref_input_device;
    prefOutputDevice = pref_output_device;

    calcVolIdx = 1.f;
    maxFrameIndex = 0;
    frameIndex = 0;

    //
    // Original author: https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
    //
    circ_buffer_size = buffer_size;
    gkCircBuffer = std::make_shared<GkCircBuffer<T>>(circ_buffer_size);
}

template<class T>
PaAudioBuf<T>::~PaAudioBuf()
{
    return;
}

template<class T>
int PaAudioBuf<T>::playbackCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                                    const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags)
{
    std::mutex playback_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(playback_loop_mtx);

    //
    // Please do not modify this without advanced knowledge, as complicated as it looks, it should allow the playback of audio...
    // http://portaudio.com/docs/v19-doxydocs/api_overview.html
    //

    Q_UNUSED(inputBuffer);
    Q_UNUSED(timeInfo);
    Q_UNUSED(statusFlags);

    int numChannels = prefOutputDevice.sel_channels;
    std::unique_ptr<GkPaAudioData> data = std::make_unique<GkPaAudioData>();
    auto *rptr = (&data->recordedSamples[data->frameIndex * numChannels]);
    auto *wptr = (T *)outputBuffer;
    size_t framesLeft = (data->maxFrameIndex - data->frameIndex);
    int finished;

    if (framesLeft < framesPerBuffer) {
        // Final buffer!
        for (size_t i = 0; i < framesLeft; ++i) {
            *wptr++ = *rptr++;  // Left
            if (numChannels == 2) {
                *wptr++ = *rptr++; // Right
            }
        }
        for (size_t i = 0; i < framesPerBuffer; ++i) {
            *wptr++ = 0; // Left
            if (numChannels == 2) {
                *wptr++ = 0;  // Right
            }
        }

        data->frameIndex += framesLeft;
        finished = paComplete;
    } else {
        for (size_t i = 0; i < framesPerBuffer; ++i) {
            *wptr++ = *rptr++;  // Left
            if (numChannels == 2) {
                *wptr++ = *rptr++;  // Right
            }
        }

        data->frameIndex += framesPerBuffer;
        finished = paContinue;
    }

    return finished;
}

/**
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @tparam T
 * @param inputBuffer
 * @param outputBuffer
 * @param framesPerBuffer
 * @param timeInfo
 * @param statusFlags
 * @return
 */
template<class T>
int PaAudioBuf<T>::recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                                  const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags)
{
    std::mutex record_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(record_loop_mtx);

    // Prevent unused variable warnings!
    Q_UNUSED(outputBuffer);
    Q_UNUSED(timeInfo);
    Q_UNUSED(statusFlags);

    int finished = paContinue;
    auto *buffer_ptr = (T *)inputBuffer;

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        buffer_ptr[i] *= (T)(buffer_ptr[i] * calcVolIdx);
        gkCircBuffer->put(buffer_ptr[i] + i);
    }

    return finished;
}

/**
 * @brief PaAudioBuf<T>::setVolume
 * @param value
 */
template<class T>
void PaAudioBuf<T>::setVolume(const float &value)
{
    calcVolIdx = value;

    return;
}

/**
 * @brief PaAudioBuf<T>::size
 * @return
 */
template<class T>
size_t PaAudioBuf<T>::size() const
{
    size_t rec_samples_size = 0;
    if (!gkCircBuffer->empty()) {
        rec_samples_size = gkCircBuffer->size();
        return rec_samples_size;
    }

    return rec_samples_size;
}

/**
 * @brief PaAudioBuf<T>::empty
 * @return
 */
template<class T>
bool PaAudioBuf<T>::empty() const
{
    if (gkCircBuffer != nullptr) {
        if (gkCircBuffer->empty()) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

/**
 * @brief PaAudioBuf<T>::clear
 * @return
 */
template<class T>
bool PaAudioBuf<T>::clear() const
{
    if (gkCircBuffer != nullptr) {
        if (!gkCircBuffer->empty()) {
            gkCircBuffer->reset();

            if (gkCircBuffer->empty()) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief PaAudioBuf<T>::get
 * @return
 */
template<class T>
T PaAudioBuf<T>::grab() const
{
    return gkCircBuffer->grab();
}

/**
 * @brief PaAudioBuf<T>::append
 */
template<class T>
void PaAudioBuf<T>::append(const std::vector<T> &vec)
{
    for (const auto &data: vec) {
        gkCircBuffer->put(data);
    }

    return;
}

/**
 * @brief PaAudioBuf<T>::full
 * @return
 */
template<class T>
bool PaAudioBuf<T>::full() const
{
    if (gkCircBuffer->full()) {
        return true;
    }

    return false;
}
};
