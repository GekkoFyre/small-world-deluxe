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
#include "src/gk_logger.hpp"
#include <boost/filesystem.hpp>
#include <memory>
#include <string>
#include <vector>
#include <exception>
#include <iostream>
#include <QObject>
#include <QBuffer>
#include <QThread>
#include <QPointer>
#include <QIODevice>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>

#ifdef __cplusplus
extern "C"
{
#endif

#include <opus.h>
#include <stdint.h>

#ifdef __cplusplus
} // extern "C"
#endif

namespace fs = boost::filesystem;
namespace sys = boost::system;

namespace GekkoFyre {

class GkAudioEncoding : public QObject {
    Q_OBJECT

private:
    #ifdef OPUS_LIBS_ENBLD
    struct OpusErrorException: public virtual std::exception {
        OpusErrorException(int code) : code(code) {}
        const char *what() const noexcept;

    private:
        const int code;
    };

    struct OpusState {
        OpusState(int max_frame_size, int max_payload_bytes, int channels): out(max_frame_size * channels),
                  fbytes(max_frame_size * channels * sizeof(decltype(out)::value_type)), data(max_payload_bytes) {}
        std::vector<qint16> out;
        std::vector<unsigned char> fbytes, data;
        int32_t frameno = 0;
        bool lost_prev = true;
    };
    #endif

public:
    explicit GkAudioEncoding(QPointer<QAudioOutput> audioOutput, QPointer<QAudioInput> audioInput,
                             const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                             const GekkoFyre::Database::Settings::Audio::GkDevice &input_device,
                             QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkAudioEncoding() override;

    QString codecEnumToStr(const GkAudioFramework::CodecSupport &codec);

public slots:
    void startCaller(const fs::path &media_path, const GekkoFyre::Database::Settings::Audio::GkDevice &audio_dev_info,
                     const qint32 &bitrate, const GekkoFyre::GkAudioFramework::CodecSupport &codec_choice,
                     const qint32 &frame_size = AUDIO_FRAMES_PER_BUFFER, const qint32 &application = OPUS_APPLICATION_AUDIO);
    void writeCaller(const QByteArray &data);
    void stopEncode();

private slots:
    void stopCaller();

    QByteArray opusEncode();
    void processEncData(const QByteArray &data);

    void handleError(const QString &msg, const GekkoFyre::System::Events::Logging::GkSeverity &severity);

signals:
    void pauseEncode();

    void encoded(QByteArray data);
    void error(const QString &msg, const GekkoFyre::System::Events::Logging::GkSeverity &severity);

private:
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
    qint32 m_channels = -1;
    qint32 m_frame_size = -1;
    fs::path m_file_path;
    QByteArray m_buffer;
    GkAudioFramework::CodecSupport m_chosen_codec;
    OpusEncoder *m_opus_encoder = nullptr;

};
};
