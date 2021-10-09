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
 **   Copyright (C) 2020 - 2021. GekkoFyre.
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

#include "src/gk_sinewave.hpp"
#include <cmath>
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
 * @brief GkSinewaveOutput::GkSinewaveOutput
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkSinewaveOutput::GkSinewaveOutput(const QString &output_audio_dev_name, QPointer<GekkoFyre::AudioDevices> audio_devs,
                                   QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QObject(parent)
{
    gkAudioDevices = std::move(audio_devs);
    gkEventLogger = std::move(eventLogger);
    gkOutputDevName = output_audio_dev_name;

    //
    // Set a default sample rate; useful for if it cannot otherwise be calculated!
    sampleRate = GK_AUDIO_SINEWAVE_TEST_DEFAULT_SAMPLE_RATE;

    //
    // Set a default amount of time for which to play the Sinewave Test!
    playLength = 3000;

    try {
        //
        // Need to create new OpenAL device and context in order to perform a sinewave test!
        mTestDevice = alcOpenDevice(gkOutputDevName.toStdString().c_str());

        //
        // Create OpenAL context for audio device under test!
        if (!alcCall(alcCreateContext, mTestCtx, mTestDevice, mTestDevice, nullptr) || !mTestCtx) {
            throw std::runtime_error(tr("ERROR: Could not create audio context for device under sinewave test!").toStdString());
        }

        if (!alcCall(alcMakeContextCurrent, mTestCtxCurr, mTestDevice, mTestCtx) || mTestCtxCurr != ALC_TRUE) {
            throw std::runtime_error(tr("ERROR: Attempt at making the audio context current has failed for output device!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }
}

GkSinewaveOutput::~GkSinewaveOutput()
{
    if (mTestCtx) {
        alcCall(alcDestroyContext, mTestDevice, mTestCtx);
    }

    if (mTestDevice) {
        ALCboolean audio_test_dev_closed;
        alcCall(alcCloseDevice, audio_test_dev_closed, mTestDevice, mTestDevice);
    }
}

/**
 * @brief GkSinewaveOutput::setPlayLength sets the amount of time, in milliseconds, to play the sinewave sound test for
 * until it is ceased.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param milliseconds The amount of time, in milliseconds, to play the sinewave test for.
 */
void GkSinewaveOutput::setPlayLength(quint32 milliseconds)
{
    playLength = milliseconds;
    return;
}

/**
 * @brief GkSinewaveOutput::setBufferLength sets the length and size of the needed audio buffer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkSinewaveOutput::setBufferLength()
{
    bufferLength = calcBufferLength();
    return;
}

/**
 * @brief GkSinewaveOutput::setSampleRate works out the sample rate that's applicable to the given audio device under
 * test for the sinewave signal playback.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkSinewaveOutput::setSampleRate()
{
    sampleRate = gkAudioDevices->getAudioDevSampleRate(mTestDevice);
    return;
}

/**
 * @brief GkSinewaveOutput::play will play the sinewave audio sample over the given end-user's chosen audio device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkSinewaveOutput::play()
{
    try {
        setBufferLength();
        ALuint buffer;
        ALuint source;

        alCall(alGenBuffers, 1, &buffer);
        ALshort *sinewave_data = generateSineWaveData();
        alCall(alBufferData, buffer, AL_FORMAT_STEREO16, sinewave_data, sizeof(sinewave_data), bufferLength * 2);
        alCall(alGenSources, 1, &source);
        alCall(alSourcei, source, AL_BUFFER, buffer);
        alCall(alSourcei, source, AL_LOOPING, AL_FALSE);
        alCall(alSourcePlay, source);

        alCall(alSourceStop, source);
        alCall(alDeleteSources, 1, &source);
        alCall(alDeleteBuffers, 1, &buffer);

        //
        // Finally, delete the generated sinewave data!
        if (sinewave_data) {
            delete[] sinewave_data;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkSinewaveOutput::calcBufferLength calculates the length and size of the needed audio buffer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return Required audio buffer length/size.
 */
qint32 GkSinewaveOutput::calcBufferLength()
{
    if (playLength > 0) {
        qint32 buf_length = playLength * sampleRate;
        return buf_length;
    }

    return 0;
}

/**
 * @brief GkSinewaveOutput::generateSineWaveData creates the actual sinewave data, via mathematical functions.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The generated sinewave data.
 */
ALshort *GkSinewaveOutput::generateSineWaveData()
{
    ALshort *data = new ALshort[bufferLength * 2];
    for (qint32 i = 0; i < bufferLength; ++i) {
        data[i * 2] += std::sin(2 * M_PI * GK_AUDIO_SINEWAVE_TEST_FREQ_HZ * i / bufferLength) * SHRT_MAX;
        data[i * 2 + 1] += -1 * std::sin(2 * M_PI * GK_AUDIO_SINEWAVE_TEST_FREQ_HZ * i / bufferLength) * SHRT_MAX; // Anti-phase component...
    }

    return data;
}
