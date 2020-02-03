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
#include <chrono>

using namespace GekkoFyre;
using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace boost;

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
 * Phil Burk <http://www.softsynth.com>
 * @param device A specifically chosen PortAudio input device.
 * @param stream A PortAudio input device stream.
 * @param continuous Whether to record continuously from the given input device until otherwise, or not.
 * @param buffer_sec_record The read buffer, as measured in seconds.
 * @note <https://github.com/EddieRingle/portaudio/blob/master/examples/paex_record.c>
 * <https://github.com/EddieRingle/portaudio/blob/master/test/patest_read_record.c>
 */
circular_buffer<double> PaMic::recordMic(const Device &device, PaStream *stream, const bool &continuous,
                                         const int &buffer_sec_record)
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
        data.rec_samples = (int *)malloc(numBytes); // From now on, `rec_samples` is initialised

        if (data.rec_samples == nullptr) {
            throw std::runtime_error(tr("Could not allocate record array.").toStdString());
        }

        for (int i = 0; i < numSamples; ++i) {
            data.rec_samples[i] = 0;
        }

        //
        // By using `paFramesPerBufferUnspecified` tells PortAudio to pick the best, possibly changing, buffer size
        // as according to many apps.
        // http://portaudio.com/docs/v19-doxydocs/open_default_stream.html
        //
        err = Pa_OpenStream(&stream, &device.stream_parameters, nullptr, device.def_sample_rate, AUDIO_FRAMES_PER_BUFFER,
                            paClipOff, nullptr, nullptr);
        if (err != paNoError) {
            gkAudioDevices->portAudioErr(err);
        }

        err = Pa_StartStream(stream);
        if (err != paNoError) {
            gkAudioDevices->portAudioErr(err);
        }

        //
        // Keep a 'circular buffer' within memory
        // https://www.boost.org/doc/libs/1_72_0/doc/html/circular_buffer.html
        //
        circular_buffer<double> circ_buffer(numSamples);

        Pa_Sleep(5000);

        if (!continuous && buffer_sec_record > 0) {
            std::time_t start = time(0);
            int timeLeft = buffer_sec_record;
            while ((timeLeft > 0) && (err = Pa_IsStreamActive(stream)) == 1) {
                std::time_t end = time(0);
                std::time_t timeTaken = end - start;
                timeLeft = buffer_sec_record - timeTaken;

                if (err != paNoError) {
                    gkAudioDevices->portAudioErr(err);
                }
            }
        } else {
            while ((err = Pa_IsStreamActive(stream)) == 1) {
                if (err != paNoError) {
                    gkAudioDevices->portAudioErr(err);
                }

                err = Pa_ReadStream(stream, &circ_buffer[0], data.maxFrameIndex);
                if (err != paNoError) {
                    gkAudioDevices->portAudioErr(err);
                }

                if (circ_buffer.full()) { // The circular buffer is full! Although technically it could keep going...
                    return circ_buffer;
                }
            }
        }
    } catch (const std::exception &e) {
        throw e.what();
    }

    return circular_buffer<double>();
}
