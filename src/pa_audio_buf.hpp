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
#include <RtAudio.h>
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

#ifdef __cplusplus
extern "C"
{
#endif

#include <sndfile.h>

#ifdef __cplusplus
} // extern "C"
#endif

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
    explicit PaAudioBuf(qint32 buffer_size, qint64 num_samples_per_channel, const GekkoFyre::Database::Settings::Audio::GkDevice &pref_output_device,
                        const GekkoFyre::Database::Settings::Audio::GkDevice &pref_input_device, QPointer<GekkoFyre::GkLevelDb> gkDb);
    virtual ~PaAudioBuf();

    qint32 recordCallback(void *outputBuffer, void *inputBuffer, quint32 nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
    qint32 recordCallbackCircBuf(void *outputBuffer, void *inputBuffer, quint32 nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
    void setVolume(const float &value);

    [[nodiscard]] virtual size_t size() const;
    [[nodiscard]] virtual bool empty() const;
    [[nodiscard]] virtual bool clear() const;
    virtual T grab() const;
    virtual void append(const std::vector<T> &vec);
    [[nodiscard]] virtual bool full() const;

private:
    QPointer<GekkoFyre::GkLevelDb> gkLevelDb;

    static inline std::shared_ptr<GekkoFyre::GkCircBuffer<T>> gkCircBuffer;
    GekkoFyre::Database::Settings::Audio::GkDevice prefInputDevice;
    GekkoFyre::Database::Settings::Audio::GkDevice prefOutputDevice;

    qint32 circ_buffer_size;
    qint32 maxFrameIndex;
    qint64 numSamplesPerChannel;
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
PaAudioBuf<T>::PaAudioBuf(qint32 buffer_size, qint64 num_samples_per_channel, const GkDevice &pref_output_device, const GkDevice &pref_input_device, QPointer<GekkoFyre::GkLevelDb> gkDb)
{
    std::mutex pa_audio_buf_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_mtx);

    gkLevelDb = std::move(gkDb);

    // The preferred input and output audio devices
    prefInputDevice = pref_input_device;
    prefOutputDevice = pref_output_device;

    calcVolIdx = 1.f;
    maxFrameIndex = 0;
    numSamplesPerChannel = num_samples_per_channel;

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

/**
 * @brief PaAudioBuf<T>::recordCallback Recording callback for RtAudio library, with interleaved buffers.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @tparam T is meant to be some kind of integer, such as qint64, float, double, etc.
 * @param outputBuffer The output audio buffer.
 * @param inputBuffer The input audio buffer.
 * @param nBufferFrames
 * @param streamTime
 * @param status
 * @param userData Any miscellaneous, user-related data passes though here.
 * @return Whether the operation was a success, or if we should continue, etc.
 */
template<class T>
qint32 PaAudioBuf<T>::recordCallback(void *outputBuffer, void *inputBuffer, quint32 nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
    std::mutex record_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(record_loop_mtx);

    // Prevent unused variable warnings!
    Q_UNUSED(outputBuffer);
    Q_UNUSED(streamTime);
    Q_UNUSED(status);

    T *data_ptr = (GkAudioFramework::GkRecord *)userData;
    quint32 frames = nBufferFrames;
    if (data_ptr->frameCounter + nBufferFrames > data_ptr->totalFrames) {
        frames = data_ptr->totalFrames - data_ptr->frameCounter;
        data_ptr->bufferBytes = frames * data_ptr->channels * sizeof(T);
    }

    size_t offset = data_ptr->frameCounter * data_ptr->channels;
    std::memcpy(data_ptr->buffer + offset, inputBuffer, data_ptr->bufferBytes);
    data_ptr->frameCounter += frames;

    if (data_ptr->frameCounter >= data_ptr->totalFrames) {
        return 2;
    }

    return 0;
}

/**
 * @brief PaAudioBuf<T>::recordCallbackCircBuf is meant to make use of the circular buffer functionality over the more standard
 * recorder callback (i.e. PaAudioBuf<T>::recordCallback()). Otherwise, this is a recording callback for the RtAudio library, with
 * interleaved buffers.
 * @tparam T is meant to be some kind of integer, such as qint64, float, double, etc.
 * @param outputBuffer The output audio buffer.
 * @param inputBuffer The input audio buffer.
 * @param nBufferFrames
 * @param streamTime
 * @param status
 * @param userData Any miscellaneous, user-related data passes though here.
 * @return Whether the operation was a success, or if we should continue, etc.
 */
template<class T>
qint32 PaAudioBuf<T>::recordCallbackCircBuf(void *outputBuffer, void *inputBuffer, quint32 nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
    std::mutex record_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(record_loop_mtx);

    // Prevent unused variable warnings!
    Q_UNUSED(outputBuffer);
    Q_UNUSED(streamTime);
    Q_UNUSED(status);

    T *data_ptr = (GkAudioFramework::GkRecord *)userData;
    quint32 frames = nBufferFrames;
    if (data_ptr->frameCounter + nBufferFrames > data_ptr->totalFrames) {
        frames = data_ptr->totalFrames - data_ptr->frameCounter;
        data_ptr->bufferBytes = frames * data_ptr->channels * sizeof(T);
    }

    T *buffer_ptr[data_ptr->totalFrames + 1];
    size_t offset = data_ptr->frameCounter * data_ptr->channels;
    std::memcpy(data_ptr->buffer + offset, inputBuffer, data_ptr->bufferBytes);
    data_ptr->frameCounter += frames;

    for (size_t i = 0; i < data_ptr->totalFrames; ++i) {
        buffer_ptr[i] *= (T)(buffer_ptr[i] * calcVolIdx); // The volume adjustment!
        gkCircBuffer->put(buffer_ptr[i] + i);
    }

    if (data_ptr->frameCounter >= data_ptr->totalFrames) {
        return 2;
    }

    return 0;
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
 * @brief PaAudioBuf<T>::grab
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
