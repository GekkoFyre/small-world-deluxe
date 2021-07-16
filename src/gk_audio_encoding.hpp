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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_string_funcs.hpp"
#include <sndfile.h>
#include <sndfile.hh>
#include <opus/opusenc.h>
#include <mutex>
#include <thread>
#include <cstdio>
#include <memory>
#include <string>
#include <QFile>
#include <QObject>
#include <QBuffer>
#include <QPointer>
#include <QIODevice>
#include <QByteArray>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>

#ifdef __cplusplus
extern "C"
{
#endif

#include "src/contrib/opus/include/opus.h"

#ifdef __cplusplus
}
#endif

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
    explicit GkAudioEncoding(const QPointer<QBuffer> &audioInputBuf, const QPointer<QBuffer> &audioOutputBuf,
                             QPointer<GekkoFyre::GkLevelDb> database, QPointer<QAudioOutput> audioOutput,
                             QPointer<QAudioInput> audioInput, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                             QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkAudioEncoding() override;

    QString codecEnumToStr(const GkAudioFramework::CodecSupport &codec);
    [[nodiscard]] GekkoFyre::GkAudioFramework::GkAudioRecordStatus getRecStatus() const;

public slots:
    void startCaller(const QFileInfo &media_path, const qint32 &bitrate, const GekkoFyre::GkAudioFramework::CodecSupport &codec_choice,
                     const GekkoFyre::Database::Settings::GkAudioSource &audio_source, const qint32 &frame_size = AUDIO_FRAMES_PER_BUFFER,
                     const qint32 &application = OPUS_APPLICATION_AUDIO);
    void stopEncode();

    void setRecStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &status);

private slots:
    void stopCaller();
    void handleError(const QString &msg, const GekkoFyre::System::Events::Logging::GkSeverity &severity);

    void encodeOpus(const qint32 &bitrate, const GekkoFyre::Database::Settings::GkAudioSource &audio_src,
                    const qint32 &frame_size = AUDIO_OPUS_MAX_FRAME_SIZE);
    void encodeVorbis(const qint32 &bitrate, qint32 sample_rate, const GekkoFyre::Database::Settings::GkAudioSource &audio_src,
                      const qint32 &frame_size = AUDIO_FRAMES_PER_BUFFER);
    void encodeFLAC(const qint32 &bitrate, qint32 sample_rate, const GekkoFyre::Database::Settings::GkAudioSource &audio_src,
                    const qint32 &frame_size = AUDIO_FRAMES_PER_BUFFER);

    void refreshAudioBuffers(const GekkoFyre::Database::Settings::GkAudioSource &audio_src);

signals:
    void pauseEncode();

    void encoded(QByteArray data);
    void error(const QString &msg, const GekkoFyre::System::Events::Logging::GkSeverity &severity);
    void initialize();

    void recStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &status);
    void updateAudioBuffers(const GekkoFyre::Database::Settings::GkAudioSource &audio_src);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // QAudioSystem initialization and buffers
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;

    //
    // Status variables
    GekkoFyre::GkAudioFramework::GkAudioRecordStatus m_recActive;

    //
    // Encoder variables
    bool m_initialized = false;                                 // Whether an encoding operation has begun or not; therefore block other attempts until this singular one has stopped.
    QFileInfo m_file_path;                                      // The file-path to the audio file where the encoded information will be written.
    QByteArray m_buffer;                                        // A QByteArray, providing more readily accessible information as needed by the FLAC, Ogg Vorbis, Ogg Opus, etc. encoders.
    QByteArray m_fileData;                                      // For any pre-existing data, such as an Ogg Opus file that had already started encoding previously.
    QPointer<QBuffer> gkAudioInputBuf;                          // For reading RAW PCM audio data from a given QAudioInput into.
    QPointer<QBuffer> gkAudioOutputBuf;                         // For reading RAW PCM audio data from a given QAudioOutput into.
    QPointer<QBuffer> m_encoded_buf;                            // For holding the encoded data whether it be FLAC, Ogg Vorbis, Ogg Opus, etc. as calculated from `record_input_buf`.
    SndfileHandle m_handle_in;                                  // The libsndfile handler, for all related operations such as reading, writing (and hence conversion), etc.
    QFile m_out_file;

    //
    // Opus related
    qint32 m_channels = 0;
    qint32 m_frameSize = 0;
    OpusEncoder *m_opusEncoder = nullptr;
    OggOpusComments *m_opusComments = nullptr;

    //
    // Multithreading and mutexes
    std::mutex m_encodeOggOpusMtx;
    std::mutex m_encodeOggVorbisMtx;
    std::mutex m_encodeFlacMtx;
    std::thread m_encodeOpusThread;
    std::thread m_encodeVorbisThread;
    std::thread m_encodeFLACThread;

    [[nodiscard]] QByteArray opusEncodeHelper(OpusEncoder *opusEncoder, const qint32 &frame_size,
                                              const qint32 &m_size, const qint32 &max_packet_size);
    void opusCleanup();
    void processByteArray();

};
};
