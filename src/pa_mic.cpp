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
#include <boost/circular_buffer.hpp>
#include <vector>

using namespace GekkoFyre;
using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

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
 * @author Phil Burk <http://www.softsynth.com> <https://github.com/EddieRingle/portaudio/blob/master/examples/paex_record.c>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device
 * @param total_sec_record
 */
void PaMic::recordMic(const Device &device, PaStream *stream, const int &total_sec_record)
{
    PaError err = paNoError;
    paRecData data;
    int totalFrames;
    int numSamples;
    int numBytes;

    data.maxFrameIndex = totalFrames = total_sec_record * device.def_sample_rate;
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

    err = Pa_OpenStream(&stream, &device.stream_parameters, nullptr, device.def_sample_rate, AUDIO_FRAMES_PER_BUFFER, paClipOff, nullptr, nullptr);
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
    boost::circular_buffer<float> cb(4096);

    Pa_Sleep(5000);
    while((err = Pa_IsStreamActive(stream)) == 1) {
        if (err != paNoError) {
            gkAudioDevices->portAudioErr(err);
        }
    }

    return;
}
