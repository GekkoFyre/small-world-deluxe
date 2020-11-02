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

#include "src/gk_sndfile_wrapper.hpp"
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
 * @brief
 */
GkSndfileCpp::GkSndfileCpp()
{}

GkSndfileCpp::~GkSndfileCpp()
{}

/**
 * @brief
 * @param fileName
 * @param mode
 * @param format
 * @return
 */
bool GkSndfileCpp::open(QString fileName, QIODevice::OpenMode mode, QAudioFormat format)
{
    qint32 snd_mode;
    switch (mode) {
        case QIODevice::ReadOnly:
            snd_mode = SFM_READ;
            m_info.channels = 0;
            m_info.format = 0;
            m_info.frames = 0;
            m_info.samplerate = 0;

            break;
        case QIODevice::WriteOnly:
            snd_mode = SFM_WRITE;
            m_info.channels = format.channelCount();
            m_info.format = SF_FORMAT_WAV;
            m_info.frames = 0;
            m_info.samplerate = format.sampleRate();

            break;
        case QIODevice::ReadWrite:
            snd_mode = SFM_RDWR;
            break;
        default:
            return false;
    }

    m_sndfile = sf_open(fileName.toLocal8Bit(), snd_mode, &m_info);
    QIODevice::open(mode);
    return m_sndfile;
}

/**
 * @brief
 */
void GkSndfileCpp::close()
{
    sf_close(m_sndfile);
    return;
}

/**
 * @brief
 * @return
 */
qint64 GkSndfileCpp::pos() const
{
    sf_count_t p = sf_seek(m_sndfile, 0, SEEK_CUR);
    return p;
}

/**
 * @brief
 * @return
 */
qint64 GkSndfileCpp::size() const
{
    sf_count_t p = sf_seek(m_sndfile, 0, SEEK_END);
    return p;
}

/**
 * @brief
 * @param pos
 * @return
 */
bool GkSndfileCpp::seek(qint64 pos)
{
    sf_seek(m_sndfile, pos, SEEK_SET);
    return true;
}

/**
 * @brief
 * @return
 */
bool GkSndfileCpp::atEnd() const
{
    if (pos() == size()) {
        return true;
    }

    return false;
}

/**
 * @brief
 * @return
 */
bool GkSndfileCpp::reset()
{
    sf_count_t p = sf_seek(m_sndfile, 0, SEEK_SET);
    if (p == -1) {
        return false;
    }

    return true;
}

/**
 * @brief
 * @return
 */
QAudioFormat GkSndfileCpp::format()
{
    QAudioFormat audioFormat;
    audioFormat.setCodec("audio/pcm");
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleRate(m_info.samplerate);
    audioFormat.setChannelCount(m_info.channels);
    switch(m_info.format & 0x0f)
    {
        case SF_FORMAT_PCM_U8:
            audioFormat.setSampleSize(8);
            audioFormat.setSampleType(QAudioFormat::UnSignedInt);
            break;
        case SF_FORMAT_PCM_S8:
            audioFormat.setSampleSize(8);
            audioFormat.setSampleType(QAudioFormat::SignedInt);
            break;
        case SF_FORMAT_PCM_16:
            audioFormat.setSampleSize(16);
            audioFormat.setSampleType(QAudioFormat::SignedInt);
            break;
        case SF_FORMAT_PCM_24:
            audioFormat.setSampleSize(24);
            audioFormat.setSampleType(QAudioFormat::SignedInt);
            break;
        case SF_FORMAT_PCM_32:
            audioFormat.setSampleSize(32);
            audioFormat.setSampleType(QAudioFormat::SignedInt);
            break;
        default:
            audioFormat.setSampleSize(8);
            audioFormat.setSampleType(QAudioFormat::UnSignedInt);
            break;
    }

    return audioFormat;
}

/**
 * @brief
 * @param data
 * @param maxSize
 * @return
 */
qint64 GkSndfileCpp::readData(char *data, qint64 maxSize)
{
    return sf_read_raw(m_sndfile, data, maxSize);
}

/**
 * @brief
 * @param data
 * @param maxSize
 * @return
 */
qint64 GkSndfileCpp::writeData(const char *data, qint64 maxSize)
{
    return sf_write_raw(m_sndfile, data, maxSize);
}
