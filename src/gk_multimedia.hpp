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
#include "src/gk_logger.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/dek_db.hpp"
#include "src/audio_devices.hpp"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <fstream>
#include <QFile>
#include <QObject>
#include <QString>
#include <QPointer>
#include <QFileInfo>

namespace GekkoFyre {

class GkMultimedia : public QObject {
    Q_OBJECT

public:
    explicit GkMultimedia(QPointer<GekkoFyre::GkAudioDevices> audio_devs,
                          std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> sysOutputAudioDevs,
                          std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> sysInputAudioDevs,
                          QPointer<GekkoFyre::GkLevelDb> database, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                          QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkMultimedia() override;

    [[nodiscard]] GekkoFyre::Database::Settings::Audio::GkDevice getOutputAudioDevice();
    [[nodiscard]] GekkoFyre::Database::Settings::Audio::GkDevice getInputAudioDevice();

    [[nodiscard]] GkAudioFramework::CodecSupport convAudioCodecIdxToEnum(const qint32 &codec_id_str);

public slots:
    void mediaAction(const GekkoFyre::GkAudioFramework::GkAudioState &media_state, const QFileInfo &file_path,
                     const ALCchar *recording_device = nullptr,
                     const GkAudioFramework::CodecSupport &codec_id = GkAudioFramework::CodecSupport::Opus,
                     const int64_t &avg_bitrate = 64000);
    void setAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &audioState);
    void changeVolume(const qint32 &value);

signals:
    void playingFinished();
    void recordingFinished();
    void updateAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &audioState);

private:
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // Mulithreading and mutexes
    std::thread playAudioFileThread;
    std::thread recordAudioFileThread;

    //
    // Audio System miscellaneous variables
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> gkSysOutputAudioDevs;
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> gkSysInputAudioDevs;
    GekkoFyre::GkAudioFramework::GkAudioState gkAudioState;

    ALuint audioPlaybackSource;
    ALuint m_frameSize;
    std::shared_ptr<std::vector<ALshort>> m_recordBuffer;

    [[nodiscard]] ALuint loadAudioFile(const QFileInfo &file_path);
    void checkForFileToBeginRecording(const QFileInfo &file_path);
    [[nodiscard]] QString convAudioCodecToFileExtStr(GkAudioFramework::CodecSupport codec_id);

    //
    // Raw audio encoders
    void convertToPcm(const QFileInfo &file_path, qint16 *samples, const qint32 &sample_size);
    void convertToPcm(const QFileInfo &file_path, qint32 *samples, const qint32 &sample_size);
    void convertToPcm(const QFileInfo &file_path, float *samples, const qint32 &sample_size);
    void encodeOpus(FILE *f, float *pcm_data, const qint32 &sample_rate, const qint32 &samples_size, const qint32 &num_channels);

    //
    // Container encoders
    void addOggContainer();

    [[nodiscard]] qint32 openAlSelectBitDepth(const ALenum &bit_depth);

    void checkOpenAlExtensions();
    void playAudioFile(const QFileInfo &file_path);
    void recordAudioFile(const QFileInfo &file_path, const ALCchar *recording_device, const GkAudioFramework::CodecSupport &codec_id,
                         const int64_t &avg_bitrate);

    [[nodiscard]] bool is_big_endian();
    [[nodiscard]] std::int32_t convert_to_int(char *buffer, std::size_t len);

};
};
