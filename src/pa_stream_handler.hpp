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
#include "src/gk_pcm_file_stream.hpp"
#include <AudioFile.h>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <QObject>
#include <QString>
#include <QThread>
#include <QPointer>
#include <QEventLoop>
#include <QByteArray>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>

namespace fs = boost::filesystem;
namespace sys = boost::system;

namespace GekkoFyre {

class GkPaStreamHandler : public QThread {
    Q_OBJECT

public:
    explicit GkPaStreamHandler(QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                               QPointer<QAudioOutput> audioOutput, QPointer<QAudioInput> audioInput, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                               std::shared_ptr<AudioFile<double>> audioFileLib, QObject *parent = nullptr);
    ~GkPaStreamHandler() override;

    void processEvent(GkAudioFramework::AudioEventType audioEventType, const fs::path &mediaFilePath = fs::path(),
                      const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec = GekkoFyre::GkAudioFramework::CodecSupport::Unknown,
                      bool loop_media = false);
    void run() Q_DECL_OVERRIDE;

private slots:
    void playMediaFile(const fs::path &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec);
    void recordMediaFile(const fs::path &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec);
    void stopMediaFile(const fs::path &media_path);
    void startMediaLoopback();
    void playbackHandleStateChanged(QAudio::State changed_state);
    void recordingHandleStateChanged(QAudio::State changed_state);

signals:
    void playMedia(const fs::path &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec);
    void recordMedia(const fs::path &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec);
    void stopMedia(const fs::path &media_path);
    void startLoopback();
    void changePlaybackState(QAudio::State changed_state);
    void changeRecorderState(QAudio::State changed_state);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    std::unique_ptr<GkPcmFileStream> gkPcmFileStream;

    //
    // AudioFile objects and related
    //
    std::shared_ptr<AudioFile<double>> gkAudioFile;

    //
    // QAudioSystem initialization and buffers
    //
    QPointer<QEventLoop> procMediaEventLoop;
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;
    QPointer<QBuffer> record_input_buf;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_output_device;
    std::map<fs::path, AudioFile<double>> gkSounds;

};
};
