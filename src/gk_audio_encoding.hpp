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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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
#include <mutex>
#include <thread>
#include <cstdio>
#include <memory>
#include <string>
#include <QObject>
#include <QBuffer>
#include <QPointer>
#include <QSaveFile>
#include <QByteArray>

#ifdef __cplusplus
extern "C"
{
#endif

#include <opus/opusenc.h>

#ifdef __cplusplus
}
#endif

namespace GekkoFyre {

class GkAudioEncoding : public QObject {
    Q_OBJECT

public:
    explicit GkAudioEncoding(QPointer<GekkoFyre::GkLevelDb> database, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                             QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkAudioEncoding() override;

    QString codecEnumToStr(const GkAudioFramework::CodecSupport &codec);
    [[nodiscard]] GekkoFyre::GkAudioFramework::GkAudioRecordStatus getRecStatus() const;
    [[nodiscard]] GkAudioFramework::CodecSupport getCodec();

public slots:
    void stopEncode();
    void setRecStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &status);

private slots:
    void stopCaller();
    void handleError(const QString &msg, const GekkoFyre::System::Events::Logging::GkSeverity &severity);

    void encodeOpus(const qint32 &bitrate, qint32 sample_rate, const quint32 &max_amplitude,
                    const GekkoFyre::Database::Settings::GkAudioSource &audio_src, const QFileInfo &media_path,
                    const qint32 &frame_size = AUDIO_OPUS_FRAMES_PER_BUFFER);
    void encodeVorbis(const qint32 &bitrate, qint32 sample_rate, const quint32 &max_amplitude,
                      const GekkoFyre::Database::Settings::GkAudioSource &audio_src, const QFileInfo &media_path,
                      const qint32 &frame_size = AUDIO_FRAMES_PER_BUFFER);
    void encodeFLAC(const qint32 &bitrate, qint32 sample_rate, const quint32 &max_amplitude,
                    const GekkoFyre::Database::Settings::GkAudioSource &audio_src, const QFileInfo &media_path,
                    const qint32 &frame_size = AUDIO_FRAMES_PER_BUFFER);
    void encodeCodec2(const quint32 &max_amplitude, const GekkoFyre::Database::Settings::GkAudioSource &audio_src,
                      const QFileInfo &media_path);
    void encodePcmWav(const quint32 &max_amplitude, const GekkoFyre::Database::Settings::GkAudioSource &audio_src,
                      const QFileInfo &media_path);

signals:
    void pauseEncode();

    void encoded(QByteArray data);
    void error(const QString &msg, const GekkoFyre::System::Events::Logging::GkSeverity &severity);
    void initialize();

    //
    // Audio related
    void stopRecInput();
    void stopRecOutput();
    void startRecInput();
    void startRecOutput();

    //
    // Encoding related
    void recStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &status);
    void bytesRead(const qint64 &bytes, const bool &uncompressed = false);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // Status variables
    GekkoFyre::GkAudioFramework::GkAudioRecordStatus m_recActive;
    GkAudioFramework::CodecSupport m_codecUsed;

    //
    // Encoder variables
    bool m_initialized = false;                                 // Whether an encoding operation has begun or not; therefore block other attempts until this singular one has stopped.
    QSaveFile m_out_file;

    //
    // Opus related
    OggOpusEnc *m_opusEncoder = nullptr;
    OggOpusComments *m_opusComments = nullptr;

    //
    // Multithreading and mutexes
    // std::mutex m_refreshAudioBufs;

    void opusCleanup();

};
};
