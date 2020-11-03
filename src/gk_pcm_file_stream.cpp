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

#include "src/gk_pcm_file_stream.hpp"
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
 * @brief GkPcmFileStream::GkPcmFileStream
 * @note Jarikus <https://stackoverflow.com/questions/41197576/how-to-play-mp3-file-using-qaudiooutput-and-qaudiodecoder>.
 */
GkPcmFileStream::GkPcmFileStream(QObject *parent) : m_input(&m_data), m_output(&m_data), m_state(State::Stopped), QIODevice(parent)
{
    setParent(parent);
    setOpenMode(QIODevice::ReadOnly);

    m_decoder = new QAudioDecoder();

    isInited = false;
    isDecodingFinished = false;
}

GkPcmFileStream::~GkPcmFileStream()
{}

/**
 * @brief
 * @return
 */
bool GkPcmFileStream::atEnd() const
{
    return m_output.size() && m_output.atEnd() && isDecodingFinished;
}

/**
 * @brief
 * @return
 */
QAudioFormat GkPcmFileStream::format()
{
    return QAudioFormat();
}

/**
 * @brief
 * @param data
 * @param maxSize
 * @return
 */
qint64 GkPcmFileStream::readData(char *data, qint64 maxlen)
{
    std::memset(data, 0, maxlen);

    if (m_state == State::Playing) {
        m_output.read(data, maxlen);

        // There is we send readed audio data via signal, for ability get audio signal for the who listen this signal.
        // Other word this emulate QAudioProbe behaviour for retrieve audio data which of sent to output device (speaker).
        if (maxlen > 0) {
            QByteArray buff(data, maxlen);
        }

        // Is finish of file
        if (atEnd()) {
            stop();
        }
    }

    return maxlen;
}

/**
 * @brief
 * @param data
 * @param maxSize
 * @return
 */
qint64 GkPcmFileStream::writeData(const char *data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);

    return 0;
}

/**
 * @brief
 * @param format
 * @return
 */
bool GkPcmFileStream::init(const QAudioFormat &format)
{
    m_format = format;
    m_decoder->setAudioFormat(m_format);

    QObject::connect(m_decoder, SIGNAL(bufferReady()), this, SLOT(bufferReady()));
    QObject::connect(m_decoder, SIGNAL(finished()), this, SLOT(finished()));

    // Initialize buffers
    if (!m_output.open(QIODevice::ReadOnly) || !m_input.open(QIODevice::WriteOnly)) {
        return false;
    }

    isInited = true;
    return true;
}

/**
 * @brief
 * @param filePath
 */
void GkPcmFileStream::play(const QString &filePath)
{
    clear();
    m_file.setFileName(filePath);

    if (!m_file.open(QIODevice::ReadOnly)) {
        return;
    }

    m_decoder->setSourceDevice(&m_file);
    m_decoder->start();

    m_state = State::Playing;

    return;
}

/**
 * @brief
 */
void GkPcmFileStream::stop()
{
    clear();
    m_state = State::Stopped;

    return;
}

/**
 * @brief
 */
void GkPcmFileStream::clear()
{
    m_decoder->stop();
    m_data.clear();
    isDecodingFinished = false;

    return;
}

/**
 * @brief
 */
void GkPcmFileStream::bufferReady()
{
    const QAudioBuffer &buffer = m_decoder->read();

    const int length = buffer.byteCount();
    const char *data = buffer.constData<char>();

    m_input.write(data, length);
    return;
}

/**
 * @brief
 */
void GkPcmFileStream::finished()
{
    isDecodingFinished = true;
    return;
}
