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
#include "src/file_io.hpp"
#include "src/spectro_gui.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include <boost/filesystem.hpp>
#include <memory>
#include <string>
#include <vector>
#include <future>
#include <thread>
#include <exception>
#include <iostream>
#include <ostream>
#include <QObject>
#include <QPointer>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef OPUS_LIBS_ENBLD
#include <opus.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
} // extern "C"
#endif

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
    explicit GkAudioEncoding(QPointer<GekkoFyre::FileIo> fileIo,
                             std::shared_ptr<PaAudioBuf<float>> input_audio_buf,
                             QPointer<GekkoFyre::GkLevelDb> database,
                             QPointer<GkSpectroWaterfall> spectroGui,
                             QPointer<GekkoFyre::StringFuncs> stringFuncs,
                             GekkoFyre::Database::Settings::Audio::GkDevice input_device,
                             QPointer<GekkoFyre::GkEventLogger> eventLogger,
                             QObject *parent = nullptr);
    ~GkAudioEncoding() override;

    void recordAudioFile(const GkAudioFramework::CodecSupport &codec, const GkAudioFramework::Bitrate &bitrate);

signals:
    void recAudioFrameOgg(std::vector<signed char> &audio_rec, const int &buf_size,
                          const GkAudioFramework::Bitrate &bitrate,
                          const boost::filesystem::path &filePath);
    void recAudioFramePcm(const std::vector<qint16> &audio_rec, const int &buf_size,
                          const boost::filesystem::path &filePath);
    void recAudioFrameFlac(const std::vector<qint16> &audio_rec, const int &buf_size,
                           const boost::filesystem::path &filePath);

    void submitOggVorbisBuf(const std::vector<signed char> &audio_frame_buf,
                            const GkAudioFramework::Bitrate &bitrate,
                            const boost::filesystem::path &filePath);
    void submitPcmBuf(const std::vector<qint16> &audio_rec, const boost::filesystem::path &filePath);
    void submitFlacBuf(const std::vector<qint16> &audio_rec, const boost::filesystem::path &filePath);

public slots:
    void startRecording(const bool &recording_is_started);

private slots:
    void oggVorbisBuf(std::vector<signed char> &audio_rec, const int &buf_size,
                      const GkAudioFramework::Bitrate &bitrate,
                      const boost::filesystem::path &filePath);

    void recordOggVorbis(const std::vector<signed char> &audio_frame_buf,
                         const GkAudioFramework::Bitrate &bitrate,
                         const boost::filesystem::path &filePath);
    void recordPcm(const std::vector<qint16> &audio_rec, const boost::filesystem::path &filePath);
    void recordFlac(const std::vector<qint16> &audio_rec, const boost::filesystem::path &filePath);

private:
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::GkSpectroWaterfall> gkSpectroGui;
    std::shared_ptr<GekkoFyre::PaAudioBuf<float>> gkAudioBuf;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    GekkoFyre::Database::Settings::Audio::GkDevice gkInputDev;

    bool recording_in_progress;
    static size_t ogg_buf_counter;

    #ifdef OPUS_LIBS_ENBLD
    std::unique_ptr<OpusState> opus_state;
    #endif

    //
    // Threads
    //
    std::thread ogg_audio_frame_thread;

    static uint32_t char_to_int(char ch[4]);

};
};

Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::Bitrate);
Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::CodecSupport);
