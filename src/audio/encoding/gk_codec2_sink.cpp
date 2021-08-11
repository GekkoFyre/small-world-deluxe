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

#include "src/audio/encoding/gk_codec2_sink.hpp"
#include <cstring>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <exception>
#include <QDir>
#include <QtGui>

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;
using namespace Network;
using namespace GkXmpp;

GkCodec2Sink::GkCodec2Sink(const QString &fileLoc, const qint32 &codec2_mode, const qint32 &natural, const bool &save_pcm_copy,
                           QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QIODevice(parent), codec2(nullptr),
                           buf(nullptr), bits(nullptr), m_file(new QSaveFile(this)), m_filePcm(new QSaveFile(this))
{
    setParent(parent);
    gkEventLogger = std::move(eventLogger);

    //
    // Initialize variables
    m_done = false;
    m_failed = false;
    m_savePcmCopy = save_pcm_copy;
    buf_empt = 0;
    m_fileLoc = fileLoc;
    m_mode = codec2_mode;
    m_initialized = false;
    m_recActive = GkAudioRecordStatus::Defunct;

    //
    // Initialize Codec2!
    codec2 = codec2_create(m_mode);
    if (codec2 == nullptr) {
        m_done = true;
        m_failed = true;
        gkEventLogger->publishEvent(tr("Unable to initialize Codec2. Perhaps you are using an unsupported mode?"),
                                    GkSeverity::Fatal, "", false, true, false, true, false);
        return;
    }

    nsam = codec2_samples_per_frame(codec2);
    nbit = codec2_bits_per_frame(codec2);
    buf = (short *)malloc(nsam * sizeof(short));
    nbyte = ((nbit + 7) / 8);
    bits = (unsigned char *)malloc(nbyte * sizeof(char));
    codec2_set_natural_or_gray(codec2, !natural);

    m_fileInfo.setFile(m_fileLoc);
    m_file->setFileName(m_fileInfo.absoluteFilePath());
    if (!m_file->open(QIODevice::WriteOnly)) {
        m_done = true;
        m_failed = true;
        gkEventLogger->publishEvent(tr("Unable to open file for Codec2 encoding: %1").arg(m_file->fileName()),
                                    GkSeverity::Fatal, "", false, true, false, true, false);
        return;
    }

    m_filePcm->setFileName(QDir::toNativeSeparators(m_fileInfo.absolutePath() + "/" + m_fileInfo.baseName() + "." + Filesystem::audio_format_pcm_wav));
    if (m_savePcmCopy) {
        if (!m_filePcm->open(QIODevice::WriteOnly)) {
            m_done = true;
            m_failed = true;
            gkEventLogger->publishEvent(tr("Unable to open file for Codec2 encoding: %1").arg(m_filePcm->fileName()),
                                        GkSeverity::Fatal, "", false, true, false, true, false);
            return;
        }
    }

    return;
}

GkCodec2Sink::~GkCodec2Sink()
{
    if (codec2) {
        codec2_destroy(codec2);
    }

    if (buf) {
        free(buf);
    }

    if (bits) {
        free(bits);
    }
}

/**
 * @brief GkCodec2Sink::readData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param maxlen
 * @return
 */
qint64 GkCodec2Sink::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);

    return 0;
}

/**
 * @brief GkPcmWavSink::writeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param len
 * @return
 */
qint64 GkCodec2Sink::writeData(const char *data, qint64 len)
{
    if (m_done) {
        QTimer::singleShot(0, this, SLOT(stop()));
    }

    auto *ptr = reinterpret_cast<const short *>(data);
    qint32 maxval = 0;
    for (qint32 i = 0; i < len / sizeof(short); ++i) {
        qint32 val = std::abs((qint32)(*ptr));
        if (val > maxval) {
            maxval = val;
        }

        ++ptr;
    }

    if (m_savePcmCopy) {
        m_filePcm->write(data, len);
    }

    for (qint32 buf_ptr = 0; buf_ptr < len;) {
        qint32 read_bytes = qMin(((qint64)(sizeof(short) * nsam - buf_empt)), len - buf_ptr);
        if (read_bytes != ((qint64)(sizeof(short) * nsam - buf_empt))) {
            std::memcpy(buf + buf_empt / sizeof(short), data + buf_ptr, read_bytes);
            buf_empt += read_bytes;
            buf_ptr += read_bytes;
            break;
        } else {
            std::memcpy(buf + buf_empt / sizeof(short), data + buf_ptr, read_bytes);
            codec2_encode(codec2, bits, buf);
            m_file->write((char *)bits, nbyte);
            buf_empt = 0;
        }

        buf_ptr += read_bytes;
    }

    return len;
}

/**
 * @brief GkCodec2Sink::isFailed refers to the fact as to whether the encoding operation has failed or not!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return Has the encoding operation failed?
 */
bool GkCodec2Sink::isFailed()
{
    return m_failed;
}

/**
 * @brief GkCodec2Sink::isDone has the encoding operation finished? Failed? Any of these states will return a true value.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return refers to whether the encoding operation has finished, failed, etc. all of which will return as 'true'.
 */
bool GkCodec2Sink::isDone()
{
    return m_done;
}

/**
 * @brief GkCodec2Sink::start
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkCodec2Sink::start()
{
    open(QIODevice::WriteOnly);
    return;
}

/**
 * @brief GkPcmWavSink::stop
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkCodec2Sink::stop()
{
    close();
    m_file->commit();
    if (m_savePcmCopy) {
        m_filePcm->commit();
    }

    return;
}
