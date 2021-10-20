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
#include "src/gk_logger.hpp"
#include "src/gk_string_funcs.hpp"
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
#include <QObject>
#include <QString>
#include <QPointer>
#include <QFileInfo>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavutil/frame.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
} // extern "C"
#endif

namespace GekkoFyre {

class GkMultimedia : public QObject {
    Q_OBJECT

public:
    explicit GkMultimedia(QPointer<GekkoFyre::GkAudioDevices> audio_devs,
                          GekkoFyre::Database::Settings::Audio::GkDevice sysOutputAudioDev,
                          GekkoFyre::Database::Settings::Audio::GkDevice sysInputAudioDev,
                          QPointer<GekkoFyre::StringFuncs> stringFuncs,
                          QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkMultimedia() override;

    GkAudioFramework::GkAudioFileInfo analyzeAudioFileMetadata(const QFileInfo &file_path, const AVCodec *codec,
                                                               const AVCodecContext *codecCtx, qint32 audioStreamIndex,
                                                               const bool &printToConsole = false) const;

    [[nodiscard]] GekkoFyre::Database::Settings::Audio::GkDevice getOutputAudioDevice();
    [[nodiscard]] GekkoFyre::Database::Settings::Audio::GkDevice getInputAudioDevice();

    [[nodiscard]] bool decodeAudioFile(const QFileInfo &file_path);

public slots:
    void playAudioFile(const QFileInfo &file_path);
    void recordAudioFile(const QFileInfo &file_path);

private:
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // Mulithreading and mutexes
    //

    //
    // Audio System miscellaneous variables
    //
    GekkoFyre::Database::Settings::Audio::GkDevice gkSysOutputAudioDev;
    GekkoFyre::Database::Settings::Audio::GkDevice gkSysInputAudioDev;
    QString tmpFile;
    FILE *outFile;

    qint32 findAudioStream(const AVFormatContext *formatCtx);
    qint32 receiveAndHandle(AVCodecContext *codecCtx, AVFrame *frame);
    void handleFrame(const AVCodecContext *codecCtx, const AVFrame *frame);
    void drainDecoder(AVCodecContext *codecCtx, AVFrame *frame);
    float getSample(const AVCodecContext *codecCtx, uint8_t *buffer, int sampleIndex);
    bool ffmpegDecodeAudioFile(const QFileInfo &file_path);

    std::vector<char> loadRawFileData(const QString &file_path, ALsizei size);
    void updateStream(const ALuint source, const ALenum &format, const std::int32_t &sample_rate, const std::vector<char> &buf, std::size_t &cursor);

    bool is_big_endian();
    std::int32_t convert_to_int(char *buffer, std::size_t len);
    bool loadWavFileHeader(std::ifstream &file, std::uint8_t &channels, std::int32_t &sampleRate,
                           std::uint8_t &bitsPerSample, ALsizei &size);
    char *loadWavFileData(const QFileInfo &file_path, std::uint8_t &channels, std::int32_t &sampleRate,
                          std::uint8_t &bitsPerSample, ALsizei &size);

};
};
