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

#include "pa_mic.hpp"
#include <boost/exception/all.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <chrono>
#include <iostream>
#include <deque>

using namespace GekkoFyre;
using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace boost;

boost::condition wait_audio;
boost::mutex wait_audio_mutex;
boost::mutex audio_buffer_mutex;
bool trigger = false;

std::deque<char> audio_buffer;

/**
 * @brief PaMic::PaMic handles most microphone functions via PortAudio.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
PaMic::PaMic(std::shared_ptr<AudioDevices> gkAudio, QObject *parent) : QObject(parent)
{
    gkAudioDevices = gkAudio;
}

PaMic::~PaMic()
{}

/**
 * @brief PaMic::recordMic Record from a selected/given microphone device on the user's computing device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device A specifically chosen PortAudio input device.
 * @param stream A PortAudio input device stream.
 * @param buffer_sec_record The read buffer, as measured in seconds.
 * @note <https://github.com/EddieRingle/portaudio/blob/master/examples/paex_record.c>
 * <https://github.com/EddieRingle/portaudio/blob/master/test/patest_read_record.c>
 */
bool PaMic::recordMic(const Device &device, PaStream *stream, std::vector<SAMPLE> *rec_data, const int &buffer_sec_record)
{
    try {
        PaError err = paNoError;
        paRecData data;
        int totalFrames;
        int numSamples;
        int numBytes;

        data.maxFrameIndex = totalFrames = buffer_sec_record * device.def_sample_rate;
        data.frameIndex = 0;
        numSamples = totalFrames * device.dev_input_channel_count;
        numBytes = numSamples * sizeof(int);
        data.recordedSamples = (SAMPLE *)malloc(numBytes);

        // Zero the buffer
        for (int i = 0; i < numSamples; ++i) {
            data.recordedSamples[i] = 0;
        }

        //
        // By using `paFramesPerBufferUnspecified` tells PortAudio to pick the best, possibly changing, buffer size
        // as according to many apps.
        // http://portaudio.com/docs/v19-doxydocs/open_default_stream.html
        // https://github.com/EddieRingle/portaudio/blob/master/test/patest_read_record.c
        //
        err = Pa_OpenStream(&stream, &device.stream_parameters, nullptr, device.def_sample_rate, AUDIO_FRAMES_PER_BUFFER,
                            paClipOff, recordCallback, &data);
        if (err != paNoError) {
            gkAudioDevices->portAudioErr(err);
        }

        err = Pa_StartStream(stream);
        if (err != paNoError) {
            gkAudioDevices->portAudioErr(err);
        }

        // Maybe good practice to do this?
        Pa_Sleep(2500);

        while (Pa_IsStreamActive(stream) == 1) {
            boost::xtime duration;
            boost::xtime_get(&duration, boost::TIME_UTC_);
            boost::interprocess::scoped_lock<boost::mutex> lock(wait_audio_mutex);
            if(!wait_audio.timed_wait(lock, duration)) {
                return false;
            }

            rec_data->clear();
            for (int j = 0; j < numSamples; ++j) {
                rec_data->push_back(data.recordedSamples[j]);
            }
        }

        return true;
    } catch (const std::exception &e) {
        throw std::runtime_error(e.what());
    }

    return false;
}

/**
 * @brief PaMic::recordCallback is needed to record an input audio stream in real-time.
 * @author trukvl <https://stackoverflow.com/questions/15690668/continuous-recording-in-portaudio-from-mic-or-output>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param input_buffer
 * @param output_buffer
 * @param frame_count
 * @param time_info
 * @param status_flags
 * @param user_data
 * @return
 */
int PaMic::recordCallback(const void *input_buffer, void *output_buffer, unsigned long frame_count,
                          const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags,
                          void *user_data)
{
    const char *buffer_ptr = (const char *)input_buffer;

    // Lock mutex to block user thread from modifying data buffer
    audio_buffer_mutex.lock();

    // Copy data to user buffer
    for (int i = 0; i < frame_count; ++i) {
        audio_buffer.push_back(buffer_ptr[i]);
    }

    // Unlock mutex, allow user to manipulate buffer
    audio_buffer_mutex.unlock();

    // Signal user thread to process audio
    wait_audio_mutex.lock();

    trigger = true;

    wait_audio.notify_one();
    wait_audio_mutex.unlock();

    return 0;
}
