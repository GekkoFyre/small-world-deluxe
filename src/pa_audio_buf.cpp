/**
 **  ______  ______  ___   ___  ______  ______  ______  ______
 ** /_____/\/_____/\/___/\/__/\/_____/\/_____/\/_____/\/_____/\
 ** \:::_ \ \::::_\/\::.\ \\ \ \:::_ \ \:::_ \ \::::_\/\:::_ \ \
 **  \:\ \ \ \:\/___/\:: \/_) \ \:\ \ \ \:\ \ \ \:\/___/\:(_) ) )_
 **   \:\ \ \ \::___\/\:. __  ( (\:\ \ \ \:\ \ \ \::___\/\: __ `\ \
 **    \:\/.:| \:\____/\: \ )  \ \\:\_\ \ \:\/.:| \:\____/\ \ `\ \ \
 **     \____/_/\_____\/\__\/\__\/ \_____\/\____/_/\_____\/\_\/ \_\/
 **
 **
 **   If you have downloaded the source code for "Dekoder for Morse" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020. GekkoFyre.
 **
 **   Dekoder for Morse is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Dekoder is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Dekoder for Morse.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "pa_audio_buf.hpp"
#include <iostream>
#include <exception>
#include <algorithm>
#include <iterator>

#ifdef _WIN32
#include <Windows.h>
#endif

//
// If making use of `paFloat32` for all recordings, then be sure to use `0.0f` for the `GK_SAMPLE_SILENCE`!
//
#define GK_SAMPLE_SILENCE (0.0f)

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;

/**
 * @brief PaAudioBuf::PaAudioBuf processes the audio buffering and memory handling functions for PortAudio.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <http://portaudio.com/docs/v19-doxydocs-dev/pa__ringbuffer_8h.html>
 * <http://portaudio.com/docs/v19-doxydocs-dev/group__test__src.html>
 * @param buffer_size
 */

/**
 * @brief PaAudioBuf::PaAudioBuf
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param buffer_size
 * @param pref_output_device
 * @param pref_input_device
 * @param parent
 */
PaAudioBuf::PaAudioBuf(int buffer_size, const GkDevice &pref_output_device, const GkDevice &pref_input_device, QObject *parent)
{
    std::mutex pa_audio_buf_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_mtx);

    // The preferred input and output audio devices
    prefInputDevice = pref_input_device;
    prefOutputDevice = pref_output_device;

    maxFrameIndex = 0;
    frameIndex = 0;

    //
    // Original author: https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
    //
    circ_buffer_size = buffer_size;
    gkCircBuffer = std::make_unique<GkCircBuffer<float *>>(circ_buffer_size, this);
}

PaAudioBuf::~PaAudioBuf()
{}

/**
 * @brief PaAudioBuf::playbackCallback
 * @author Phil Burk <http://www.softsynth.com>, Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param inputBuffer
 * @param outputBuffer
 * @param framesPerBuffer
 * @param timeInfo
 * @param statusFlags
 * @param userData
 * @return
 */
int PaAudioBuf::playbackCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags)
{
    std::mutex playback_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(playback_loop_mtx);

    Q_UNUSED(inputBuffer);
    Q_UNUSED(timeInfo);
    Q_UNUSED(statusFlags);

    int numChannels = prefOutputDevice.sel_channels;
    std::unique_ptr<GkPaAudioData> data = std::make_unique<GkPaAudioData>();
    float *rptr = (&data->recordedSamples[data->frameIndex * numChannels]);
    float *wptr = (float *)outputBuffer;
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
 * @brief PaAudioBuf::recordCallback is responsible for copying PortAudio data (i.e. input audio device) to the
 * memory, asynchronously.
 * @author Phil Burk <http://www.softsynth.com>, Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param inputBuffer
 * @param outputBuffer
 * @param framesPerBuffer
 * @param timeInfo
 * @param statusFlags
 * @param userData
 * @return
 */
int PaAudioBuf::recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags)
{
    std::mutex record_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(record_loop_mtx);

    // Prevent unused variable warnings!
    Q_UNUSED(outputBuffer);
    Q_UNUSED(timeInfo);
    Q_UNUSED(statusFlags);

    int numChannels = prefInputDevice.sel_channels;
    std::unique_ptr<GkPaAudioData> data = std::make_unique<GkPaAudioData>();
    const float *rptr = (const float*)inputBuffer;
    float *wptr = &data->recordedSamples[data->frameIndex * numChannels];

    size_t framesToCalc = 0;
    size_t framesLeft = (data->maxFrameIndex - data->frameIndex);
    int finished;

    if (framesLeft < framesPerBuffer) {
        framesToCalc = framesLeft;
        finished = paComplete;
    } else {
        framesToCalc = framesPerBuffer;
        finished = paContinue;
    }

    if (inputBuffer == nullptr) {
        for (size_t i = 0; i < framesToCalc; ++i) {
            *wptr++ = GK_SAMPLE_SILENCE;  // Left
            if (numChannels == 2) {
                *wptr++ = GK_SAMPLE_SILENCE;  // Right
            }
        }
    } else {
        for (size_t i = 0; i < framesToCalc; ++i) {
            *wptr++ = *rptr++;  // Left
            if (numChannels == 2) {
                *wptr++ = *rptr++;  // Right
            }

            gkCircBuffer->put(wptr);
        }
    }

    data->frameIndex += framesToCalc;

    return finished;
}

/**
 * @brief PaAudioBuf::dumpMemory Will dump the contents of the buffer when it reaches the target
 * size, before starting over again.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The entire contents of the buffer once having reached the target size.
 */
std::vector<int> PaAudioBuf::dumpMemory()
{
    std::mutex pa_buf_dup_mem_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_buf_dup_mem_mtx);

    std::vector<int> ret_vec;
    if (gkCircBuffer != nullptr) {
        if (!gkCircBuffer->empty()) {
            float *data = gkCircBuffer->get();
            ret_vec.reserve(gkCircBuffer->size());
            size_t max_idx_counter;
            for (size_t i = 0; i < gkCircBuffer->size(); ++i) {
                ret_vec.push_back(data[i]);
                ++max_idx_counter;

                if (max_idx_counter == frameIndex) {
                    break;
                }
            }

            return ret_vec;
        }
    }

    ret_vec = fillVecZeros(circ_buffer_size);
    return ret_vec;
}

/**
 * @brief PaAudioBuf::prepOggVorbisBuf prepares an audio frame within the constraints as
 * those set forth by the Ogg Vorbis library for an audio buffer, for which this function
 * exports the data as.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return A data frame suitable to be used right away by the Ogg Vorbis library for recording, with
 * only small modifications needed to fit within the constraints of its data I/O buffer afterword.
 */
void PaAudioBuf::prepOggVorbisBuf(std::promise<std::vector<signed char>> vorbis_buf)
{
    std::mutex pa_prep_vorbis_buf_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_prep_vorbis_buf_mtx);

    auto pa_buf_data = dumpMemory();
    if (!pa_buf_data.empty()) {
        std::vector<signed char> vorbis_buf_tmp;

        vorbis_buf_tmp.reserve(pa_buf_data.size());
        vorbis_buf_tmp.assign(pa_buf_data.begin(), pa_buf_data.end());

        vorbis_buf.set_value(vorbis_buf_tmp);

        return;
    } else {
        throw std::runtime_error(tr("Audio data buffer frame is empty!").toStdString());
    }

    return;
}

size_t PaAudioBuf::size() const
{
    size_t rec_samples_size = 0;
    if (!gkCircBuffer->empty()) {
        rec_samples_size = gkCircBuffer->size();
        return rec_samples_size;
    }

    return rec_samples_size;
}

int PaAudioBuf::at(const int &idx)
{
    std::mutex pa_audio_buf_loc_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_loc_mtx);
    int ret_value = 0;
    if (gkCircBuffer != nullptr && is_rec_active) {
        if (!gkCircBuffer->empty()) {
            if (idx <= circ_buffer_size && idx >= 0) {
                for (size_t i = 0; i < gkCircBuffer->size(); ++i) {
                    if (i == idx) {
                        ret_value = gkCircBuffer->get()[i];
                        return ret_value;
                    }
                }
            }
        }
    }

    return ret_value;
}

void PaAudioBuf::push_back(const float &data)
{
    if (gkCircBuffer != nullptr) {
        if (gkCircBuffer->full()) {
            gkCircBuffer->reset();
        }

        std::vector<float> array_tmp;
        array_tmp.reserve(1);
        array_tmp.push_back(data);
        for (size_t i = 0; i < gkCircBuffer->capacity(); ++i) {
            gkCircBuffer->put(array_tmp.data());
        }
    }

    return;
}

bool PaAudioBuf::empty() const
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

bool PaAudioBuf::clear() const
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

void PaAudioBuf::abortRecording(const bool &recording_is_stopped, const int &wait_time)
{
    std::mutex pa_audio_buf_rec_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_rec_mtx);

    if (recording_is_stopped) {
        is_rec_active = false;
    } else {
        is_rec_active = true;
    }

    return;
}

/**
 * @brief PaAudioBuf::fillVecZeros The idea of this is to fill an std::vector() with all zeroes
 * when there is nothing else to return but this, so the program doesn't output an exception
 * otherwise.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param buf_size How large to make the std::vector().
 * @return The outputted std::vector() that's now filled with zeroes.
 */
std::vector<int> PaAudioBuf::fillVecZeros(const int &buf_size)
{
    std::mutex fill_vec_zeroes_mtx;
    std::lock_guard<std::mutex> lck_guard(fill_vec_zeroes_mtx);
    std::vector<int> ret_vec;
    ret_vec.reserve(buf_size);

    for (size_t i = 0; i < buf_size; ++i) {
        ret_vec.push_back(0);
    }

    return ret_vec;
}

/**
 * @brief PaAudioBuf::sampleFormatConvert
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sample_rate
 * @return
 */
portaudio::SampleDataFormat PaAudioBuf::sampleFormatConvert(const unsigned long sample_rate)
{
    switch (sample_rate) {
    case paFloat32:
        return portaudio::FLOAT32;
    case paInt32:
        return portaudio::INT32;
    case paInt24:
        return portaudio::INT24;
    case paInt16:
        return portaudio::INT16;
    case paInt8:
        return portaudio::INT8;
    case paUInt8:
        return portaudio::UINT8;
    default:
        return portaudio::INT16;
    }

    return portaudio::INT16;
}
