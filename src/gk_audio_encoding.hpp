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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include <sndfile.h>
#include <opus/opusenc.h>
#include <boost/filesystem.hpp>
#include <mutex>
#include <thread>
#include <cstdio>
#include <memory>
#include <string>
#include <QObject>
#include <QBuffer>
#include <QPointer>
#include <QIODevice>
#include <QByteArray>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>

namespace fs = boost::filesystem;
namespace sys = boost::system;

namespace GekkoFyre {

/**
 * @brief GkSfVirtualInterface
 * @author Soheil Armin <https://stackoverflow.com/questions/59233615/how-to-use-qbytearray-instead-of-sndfile>.
 */
class GkSfVirtualInterface : public QBuffer {
    Q_OBJECT

public:
    static sf_count_t qbuffer_get_filelen(void *user_data) {
        QBuffer *buff = (QBuffer *)user_data;
        return buff->size();
    }

    static sf_count_t qbuffer_seek(sf_count_t offset, int whence, void *user_data) {
        QBuffer *buff = (QBuffer *)user_data;
        switch (whence) {
            case SEEK_SET:
                buff->seek(offset);
                break;
            case SEEK_CUR:
                buff->seek(buff->pos() + offset);
                break;
            case SEEK_END:
                buff->seek(buff->size() + offset);
                break;
            default:
                break;
        };

        return buff->pos();
    }

    static sf_count_t qbuffer_read(void *ptr, sf_count_t count, void *user_data) {
        QBuffer *buff = (QBuffer *)user_data;
        return buff->read((char *)ptr, count);
    }

    static sf_count_t qbuffer_write(const void *ptr, sf_count_t count, void *user_data) {
        QBuffer *buff = (QBuffer *)user_data;
        return buff->write((const char *)ptr, count);
    }

    static sf_count_t qbuffer_tell(void *user_data) {
        QBuffer *buff = (QBuffer *)user_data;
        return buff->pos();
    }
};

class GkAudioEncoding : public QObject {
    Q_OBJECT

public:
    explicit GkAudioEncoding(QPointer<GekkoFyre::GkLevelDb> database, QPointer<QAudioOutput> audioOutput,
                             QPointer<QAudioInput> audioInput, const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                             const GekkoFyre::Database::Settings::Audio::GkDevice &input_device,
                             QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkAudioEncoding() override;

    QString codecEnumToStr(const GkAudioFramework::CodecSupport &codec);

public slots:
    void startCaller(const fs::path &media_path, const GekkoFyre::Database::Settings::Audio::GkDevice &audio_dev_info,
                     const qint32 &bitrate, const GekkoFyre::GkAudioFramework::CodecSupport &codec_choice,
                     const qint32 &frame_size = AUDIO_FRAMES_PER_BUFFER, const qint32 &application = OPUS_APPLICATION_AUDIO);
    void stopEncode();

private slots:
    void stopCaller();
    void handleError(const QString &msg, const GekkoFyre::System::Events::Logging::GkSeverity &severity);
    void processAudioIn();

    void startRecBuffer();
    void encodeOpus(const QByteArray &data_buf);
    void encodeVorbis(const QByteArray &data_buf);
    void encodeFLAC(const QByteArray &data_buf);

signals:
    void pauseEncode();

    void encoded(QByteArray data);
    void error(const QString &msg, const GekkoFyre::System::Events::Logging::GkSeverity &severity);
    void initialize();

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // QAudioSystem initialization and buffers
    //
    GekkoFyre::Database::Settings::Audio::GkDevice gkInputDev;
    GekkoFyre::Database::Settings::Audio::GkDevice gkOutputDev;
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;

    //
    // Encoder variables
    //
    bool m_initialized = false;
    fs::path m_file_path;
    QByteArray m_buffer;
    GkAudioFramework::CodecSupport m_chosen_codec;
    QPointer<QBuffer> record_input_buf;
    SNDFILE	*m_handle_in;

    //
    // Opus related
    //
    opus_int32 m_sample_rate = 48000;
    qint32 m_channels = 0;
    qint32 m_frame_size = 0;
    OggOpusEnc *m_opus_encoder = nullptr;
    OggOpusComments *m_opus_comments = nullptr;

};
};
