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
 * @brief GkSinewaveTest::GkSinewaveTest
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param eventLogger
 * @param freq
 * @param parent
 */
GkSinewaveTest::GkSinewaveTest(const GkDevice &audio_dev, QPointer<GekkoFyre::GkEventLogger> eventLogger, qint32 freq,
                               QObject *parent) : QIODevice(parent), buffer(nullptr)
{
    gkEventLogger = std::move(eventLogger);
    gkAudioDevice = audio_dev;

    setFreq(freq);
    open(QIODevice::ReadOnly);
}

GkSinewaveTest::~GkSinewaveTest()
{
    delete[] buffer;
}

/**
 * @brief GkSinewaveTest::setFreq
 * @author Graeme (radi8) <https://github.com/radi8/Morse>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq
 */
void GkSinewaveTest::setFreq(qint32 freq)
{
    if (buffer) {
        delete[] buffer;
    }

    const qint32 upper_freq = 10000;
    const qint32 full_waves = 3;

    // Arbitary upper frequency
    if (freq > upper_freq) {
        freq = upper_freq;
    }

    // We create a buffer with some full waves of freq,
    // therefore we need room for this many samples:
    qint32 buflen = gkAudioDevice.pref_sample_rate * full_waves / freq;

    buffer = new qint32[buflen];

    // Now fill this buffer with the sine wave
    qint32 *t = buffer;
    for (qint32 i = 0; i < buflen; ++i) {
        qint32 value = 32767.0 * std::sin(M_PI * 2 * i * freq / gkAudioDevice.pref_sample_rate);
        *t++ = value;
    }

    send_pos = buffer;
    end = buffer + buflen;
    samples = 0;

    return;
}

/**
 * @brief GkSinewaveTest::setDuration
 * @author Graeme (radi8) <https://github.com/radi8/Morse>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param ms
 */
void GkSinewaveTest::setDuration(qint32 ms)
{
    samples = (gkAudioDevice.pref_sample_rate * ms) / 1000;
    samples &= 0x7ffffffe;
    send_pos = buffer;

    return;
}

/**
 * @brief GkSinewaveTest::readData
 * @author Graeme (radi8) <https://github.com/radi8/Morse>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param max_length
 * @return
 */
qint64 GkSinewaveTest::readData(char *data, qint64 max_length)
{
    quint64 len = max_length;

    char *t = data;
    while (len) {
        // As long as we should provide samples, do this:
        if (samples) {
            qint32 value = *send_pos++;
            //TODO: this is the place where we could modify the
            // value, e.g. to ramp it up or down, or to attenuate it
            if (send_pos == end) {
                send_pos = buffer;
            }

            *t++ = value        & 0xff;
            *t++ = (value >> 8) & 0xff;
            --samples;
        } else {
            // But afterwards, return zero
            *t++ = 0;
            *t++ = 0;
        }

        len -= 2;
    }

    return max_length;
}

/**
 * @brief GkSinewaveTest::writeData is a dummy implementation and does nothing; it's just needed to inherit successfully from
 * the QIODevice member.
 * @author Graeme (radi8) <https://github.com/radi8/Morse>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param length
 * @return
 */
qint64 GkSinewaveTest::writeData(const char *data, qint64 length)
{
    Q_UNUSED(data);
    Q_UNUSED(length);

    return 0;
}

/**
 * @brief GkSinewaveOutput::GkSinewaveOutput
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkSinewaveOutput::GkSinewaveOutput(const GkDevice &audio_dev, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                   QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput,
                                   QObject *parent) : QObject(parent)
{
    setParent(this);

    gkEventLogger = std::move(eventLogger);
    gkAudioDevice = audio_dev;
    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);

    buffer = new char[(audio_dev.pref_sample_rate * AUDIO_SINE_WAVE_PLAYBACK_SECS) + 1];
    gkSinewaveTest = new GkSinewaveTest(gkAudioDevice, gkEventLogger, 300, this);

    output = gkAudioOutput->start();
    timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(writeMore()));
    timer->setSingleShot(true);
}

GkSinewaveOutput::~GkSinewaveOutput()
{
    delete[] buffer;
}

/**
 * @brief GkSinewaveOutput::playSound
 * @param milliseconds
 */
void GkSinewaveOutput::playSound(quint32 milliseconds)
{
    gkSinewaveTest->setDuration(milliseconds);
    gkAudioOutput->resume();
    writeMore();

    return;
}

/**
 * @brief GkSinewaveOutput::writeMore
 */
void GkSinewaveOutput::writeMore()
{
    if (!gkAudioOutput) {
        return;
    }

    if (gkAudioOutput->state() == QAudio::StoppedState) {
        return;
    }

    int chunks = gkAudioOutput->bytesFree() / gkAudioOutput->periodSize();
    while (chunks) {
        int len = gkSinewaveTest->read(buffer, gkAudioOutput->periodSize());
        if (len > 0) {
            output->write(buffer, len);
        }

        --chunks;
    }

    timer->start(100);
    return;
}
