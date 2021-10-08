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
#include "src/gk_pcm_file_stream.hpp"
#include "src/gk_audio_encoding.hpp"
#include <AudioFile.h>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <QDir>
#include <QMap>
#include <QObject>
#include <QString>
#include <QThread>
#include <QPointer>
#include <QFileInfo>
#include <QByteArray>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>

namespace GekkoFyre {

class GkPaStreamHandler : public QObject {
    Q_OBJECT

public:
    explicit GkPaStreamHandler(QPointer<GekkoFyre::GkLevelDb> database, QPointer<QAudioOutput> audioOutput,
                               QPointer<QAudioInput> audioInput, QPointer<GekkoFyre::GkAudioEncoding> audioEncoding,
                               QPointer<GekkoFyre::GkEventLogger> eventLogger, std::shared_ptr<AudioFile<double>> audioFileLib,
                               QObject *parent = nullptr);
    ~GkPaStreamHandler() override;

    void processEvent(GkAudioFramework::AudioEventType audioEventType, const GekkoFyre::Database::Settings::GkAudioSource &audio_source,
                      const QFileInfo &mediaFilePath = QFileInfo(), const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec = GekkoFyre::GkAudioFramework::CodecSupport::Unknown,
                      bool loop_media = false, qint32 encode_bitrate = 8);
    void processEvent(GkAudioFramework::AudioEventType audioEventType, const GekkoFyre::Database::Settings::GkAudioSource &audio_source,
                      const QDir &mediaFilePath = QDir(), const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec = GekkoFyre::GkAudioFramework::CodecSupport::Unknown,
                      bool loop_media = false, qint32 encode_bitrate = 8);

    QFileInfo createRecordMediaFile(const QDir &media_path, const GkAudioFramework::CodecSupport &supported_codec);

private slots:
    void playMediaFile(const QFileInfo &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                       const GekkoFyre::Database::Settings::GkAudioSource &audio_source);
    void playMediaFile(const QDir &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                       const GekkoFyre::Database::Settings::GkAudioSource &audio_source);
    void recordMediaFile(const QDir &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                         const GekkoFyre::Database::Settings::GkAudioSource &audio_source, qint32 encoding_bitrate);
    void startMediaLoopback();
    void playbackHandleStateChanged(QAudio::State changed_state);
    void recordingHandleStateChanged(QAudio::State changed_state);

    //
    // Stopping of either playing or recording of multimedia files
    void stopMediaFile(const QFileInfo &media_path);
    void stopMediaFile(const QDir &media_path);
    void stopRecordingFile(const QFileInfo &media_path);
    void stopRecordingFile(const QDir &media_path);

signals:
    //
    // Playing of multimedia files
    void playMedia(const QFileInfo &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                   const GekkoFyre::Database::Settings::GkAudioSource &audio_source);
    void playMedia(const QDir &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                   const GekkoFyre::Database::Settings::GkAudioSource &audio_source);

    //
    // Recording of multimedia files
    void recordMedia(const QDir &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                     const GekkoFyre::Database::Settings::GkAudioSource &audio_source, qint32 encoding_bitrate);

    //
    // Stopping of either playing or recording of multimedia files
    void stopMedia(const QFileInfo &media_path);
    void stopMedia(const QDir &media_path);
    void stopRecording(const QFileInfo &media_path);
    void stopRecording(const QDir &media_path);
    void stopLoopback(const QFileInfo &media_path);
    void stopLoopback(const QDir &media_path);

    //
    // Miscellaneous
    void startLoopback();

    //
    // Changing the playback state of multimedia files (i.e. for recording, playing, etc.)
    void changePlaybackState(QAudio::State changed_state);
    void changeRecorderState(QAudio::State changed_state);

    //
    // Audio related
    void stopRecInput();
    void stopRecOutput();
    void startRecInput();
    void startRecOutput();

    //
    // Encoding of multimedia files
    void initEncode(const QFileInfo &media_path, const qint32 &bitrate, const GekkoFyre::GkAudioFramework::CodecSupport &codec_choice,
                    const GekkoFyre::Database::Settings::GkAudioSource &audio_source, const qint32 &frame_size = AUDIO_FRAMES_PER_BUFFER,
                    const qint32 &application = OPUS_APPLICATION_AUDIO);
    void writeEncode(const QByteArray &data);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    std::unique_ptr<GkPcmFileStream> gkPcmFileStream;

    //
    // AudioFile objects and related
    std::shared_ptr<AudioFile<double>> gkAudioFile;

    //
    // Audio encoding related objects
    QPointer<GekkoFyre::GkAudioEncoding> gkAudioEncoding;
    QFileInfo m_mediaFile;

    //
    // Multithreading and mutexes
    std::mutex m_recordMediaFileHelperMutex;
    std::mutex m_createRecordMediaFileInfoMutex;
    std::mutex m_createRecordMediaFileDirMutex;

    //
    // Audio System initialization and buffers
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;
    QMap<QString, AudioFile<double>> gkSounds;
    QMultiMap<GekkoFyre::GkAudioFramework::AudioEventType, QString> gkAudioEvents;

    void playMediaFileHelper(QFileInfo media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                             const GekkoFyre::Database::Settings::GkAudioSource &audio_source);

    void recordMediaFileHelper(QDir media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                               const GekkoFyre::Database::Settings::GkAudioSource &audio_source, qint32 encoding_bitrate);

};
};
