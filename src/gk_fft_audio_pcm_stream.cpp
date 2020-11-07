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

#include "src/gk_fft_audio_pcm_stream.hpp"
#include <exception>
#include <utility>

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
 * @brief GkFFTAudioPcmStream::GkFFTAudioPcmStream
 * @note Jarikus <https://stackoverflow.com/questions/41197576/how-to-play-mp3-file-using-qaudiooutput-and-qaudiodecoder>.
 */
GkFFTAudioPcmStream::GkFFTAudioPcmStream(QPointer<QAudioInput> audioInput, const GkDevice &audio_device_details,
                                         QObject *parent) : m_input(&m_data), m_output(&m_data), m_state(State::Stopped),
                                         QIODevice(parent)
{
    setParent(parent);
    setOpenMode(QIODevice::ReadOnly);

    pref_audio_device = audio_device_details;
    gkAudioInput = std::move(audioInput);

    QObject::connect(gkAudioInput, SIGNAL(bufferReady()), this, SLOT(bufferReady()));
    QObject::connect(gkAudioInput, SIGNAL(finished()), this, SLOT(finished()));

    // Initialize buffers
    if (!m_output.open(QIODevice::ReadOnly) || !m_input.open(QIODevice::WriteOnly)) {
        throw std::runtime_error(tr("Error with initializing audio PCM stream for FFT calculations!").toStdString());
    }

    isInited = true;
    isDecodingFinished = false;
}

GkFFTAudioPcmStream::~GkFFTAudioPcmStream()
{
    if (!gkAudioInputEventLoop.isNull()) {
        delete gkAudioInputEventLoop;
    }
}

/**
 * @brief GkFFTAudioPcmStream::atEnd
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
bool GkFFTAudioPcmStream::atEnd() const
{
    return m_output.size() && m_output.atEnd() && isDecodingFinished;
}

/**
 * @brief GkFFTAudioPcmStream::readData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param maxSize
 * @return
 */
qint64 GkFFTAudioPcmStream::readData(char *data, qint64 maxlen)
{
    std::memset(data, 0, maxlen);

    if (m_state == State::Recording) {
        m_output.read(data, maxlen);

        // There is we send readed audio data via signal, for ability get audio signal for the who listen this signal.
        // Other word this emulate QAudioProbe behaviour for retrieve audio data which of sent to output device (speaker).
        if (maxlen > 0) {
            QByteArray buff(data, maxlen);
        }

        // Is finish of file
        if (atEnd()) {
            // stop();
            clear();
        }
    }

    return maxlen;
}

/**
 * @brief GkFFTAudioPcmStream::writeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param maxSize
 * @return
 */
qint64 GkFFTAudioPcmStream::writeData(const char *data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);

    return 0;
}

/**
 * @brief GkFFTAudioPcmStream::play
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filePath
 */
void GkFFTAudioPcmStream::play(const QString &filePath)
{
    Q_UNUSED(filePath);
    clear();

    gkAudioInput->start(&m_input);
    m_state = State::Recording;
    gkAudioInputEventLoop = new QEventLoop(this);

    do {
        gkAudioInputEventLoop->exec(QEventLoop::WaitForMoreEvents);
    } while (gkAudioInput->state() == QAudio::ActiveState);

    delete gkAudioInputEventLoop;
    return;
}

/**
 * @brief GkFFTAudioPcmStream::stop
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudioPcmStream::stop()
{
    clear();
    m_state = State::Stopped;

    return;
}

/**
 * @brief GkFFTAudioPcmStream::clear
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudioPcmStream::clear()
{
    gkAudioInput->stop();
    m_data.clear();
    isDecodingFinished = false;

    return;
}

/**
 * @brief GkFFTAudioPcmStream::bufferReady
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudioPcmStream::bufferReady()
{
    return;
}

/**
 * @brief GkFFTAudioPcmStream::finished
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudioPcmStream::finished()
{
    isDecodingFinished = true;
    return;
}
