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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
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

typedef struct
{
  void          *codec2;
  unsigned char *bits;
}
callbackData;

typedef struct WireConfig_s
{
    int isInputInterleaved;
    int isOutputInterleaved;
    int numInputChannels;
    int numOutputChannels;
    int framesPerCallback;
    /* count status flags */
    int numInputUnderflows;
    int numInputOverflows;
    int numOutputUnderflows;
    int numOutputOverflows;
    int numPrimingOutputs;
    int numCallbacks;
} WireConfig_t;

template <class T>
class PaAudioBuf {

public:
    explicit PaAudioBuf(int buffer_size, const GekkoFyre::Database::Settings::Audio::GkDevice &pref_output_device,
                        const GekkoFyre::Database::Settings::Audio::GkDevice &pref_input_device);
    virtual ~PaAudioBuf();

    PaAudioBuf operator*(const PaAudioBuf &) const;
    PaAudioBuf operator+(const PaAudioBuf &) const;

    bool is_rec_active;

    int wireCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                     const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);
    int recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags);
    int codec2LocalCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);
    void setVolume(const float &value);

    virtual size_t size() const;
    virtual bool empty() const;
    virtual bool clear() const;
    virtual T grab() const;
    virtual void append(const T &data);
    virtual bool full() const;

private:
    std::shared_ptr<GekkoFyre::GkCircBuffer<T>> gkCircBuffer;
    GekkoFyre::Database::Settings::Audio::GkDevice prefInputDevice;
    GekkoFyre::Database::Settings::Audio::GkDevice prefOutputDevice;

    int circ_buffer_size;
    int maxFrameIndex;
    int frameIndex;
    float calcVolIdx; // A floating-point value between 0.0 - 1.0 that determines the amplitude of the audio signal (i.e. raw data buffer).

    std::vector<T> fillVecZeros(const int &buf_size);

    portaudio::SampleDataFormat sampleFormatConvert(const unsigned long sample_rate);

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

/**
 * @brief PaAudioBuf<T>::wireCallback allows complete audio passthrough without any modifications to the data itself!
 * @note <https://github.com/EddieRingle/portaudio/blob/master/test/patest_wire.c>
 * @return Whether to continue with the audio stream or not.
 */
template<class T>
int PaAudioBuf<T>::wireCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    std::mutex playback_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(playback_loop_mtx);

    Q_UNUSED(timeInfo);

    T *in;
    T *out;
    int inStride;
    int outStride;
    int inDone = 0;
    int outDone = 0;
    WireConfig_t *config = (WireConfig_t *)userData;
    unsigned int i;
    int inChannel, outChannel;

    // This may get called with NULL inputBuffer during initial setup.
    if (inputBuffer == nullptr) {
        return 0;
    }

    // Count flags
    if((statusFlags & paInputUnderflow) != 0) config->numInputUnderflows += 1;
    if((statusFlags & paInputOverflow) != 0) config->numInputOverflows += 1;
    if((statusFlags & paOutputUnderflow) != 0) config->numOutputUnderflows += 1;
    if((statusFlags & paOutputOverflow) != 0) config->numOutputOverflows += 1;
    if((statusFlags & paPrimingOutput) != 0) config->numPrimingOutputs += 1;
    config->numCallbacks += 1;

    inChannel = 0, outChannel = 0;
    while (!(inDone && outDone)) {
        if (config->isInputInterleaved) {
            in = ((T*)inputBuffer) + inChannel;
            inStride = config->numInputChannels;
        } else {
            in = ((T**)inputBuffer)[inChannel];
            inStride = 1;
        }

        if (config->isOutputInterleaved) {
            out = ((T*)outputBuffer) + outChannel;
            outStride = config->numOutputChannels;
        } else {
            out = ((T**)outputBuffer)[outChannel];
            outStride = 1;
        }

        for (i = 0; i < framesPerBuffer; ++i) {
            *out = *in;
            out += outStride;
            in += inStride;
        }

        if(inChannel < (config->numInputChannels - 1)) {
            inChannel++;
        } else {
            inDone = 1;
        }

        if(outChannel < (config->numOutputChannels - 1)) {
            outChannel++;
        } else {
            outDone = 1;
        }
    }

    return paContinue;
}

/**
 * @brief PaAudioBuf<T>::recordCallback
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
 * @note <https://github.com/keithmgould/tiptoe/blob/master/codec2/main.c>
 */
template<class T>
int PaAudioBuf<T>::codec2LocalCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                                       const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                                       void *userData)
{
    std::mutex record_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(record_loop_mtx);

    Q_UNUSED(timeInfo);
    Q_UNUSED(statusFlags);

    callbackData *data = (callbackData*)userData;
    T out = outputBuffer;
    T in = inputBuffer;

    if (inputBuffer == nullptr) {
        for (size_t i = 0; i < framesPerBuffer; ++i) {
            *out++ = 0;
        }
    } else {
        codec2_encode(data->codec2, data->bits, in);
        codec2_decode(data->codec2, out, data->bits);
    }

    return paContinue;
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
void PaAudioBuf<T>::append(const T &data)
{
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
