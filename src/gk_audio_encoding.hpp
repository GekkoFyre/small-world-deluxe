/**
 **  ______  ______  ___   ___  ______  ______  ______  ______
 ** /_____/\/_____/\/___/\/__/\/_____/\/_____/\/_____/\/_____/\
 ** \:::_ \ \::::_\/\::.\ \\ \ \:::_ \ \:::_ \ \::::_\/\:::_ \ \
 **  \:\ \ \ \:\/___/\:: \/_) \ \:\ \ \ \:\ \ \ \:\/___/\:(_) ) )_
 **   \:\ \ \ \::___\/\:. __  ( (\:\ \ \ \:\ \ \ \::___\/\: __ `\ \
 **    \:\/.:| \:\____/\: \ )  \ \\:\_\ \ \:\/.:| \:\____/\ \ `\ \ \
 **     \____/_/\_____\/\__\/\__\/ \_____\/\____/_/\_____\/\_\/ \_\/
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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include "src/file_io.hpp"
#include "src/spectro_gui.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/dek_db.hpp"
#include <portaudiocpp/System.hxx>
#include <boost/filesystem.hpp>
#include <portaudio.h>
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

#ifdef _WIN32
#include "src/string_funcs_windows.hpp"
#elif __linux__
#include "src/string_funcs_linux.hpp"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <opus.h>
#include <stdint.h>

#ifdef __cplusplus
}
#endif

namespace GekkoFyre {

class GkAudioEncoding : public QObject {
    Q_OBJECT

private:
    struct OpusErrorException: public virtual std::exception {
        OpusErrorException(int code) : code(code) {}
        const char *what() const noexcept;

    private:
        const int code;
    };

    struct OpusState {
        OpusState(int max_frame_size, int max_payload_bytes, int channels): out(max_frame_size * channels),
            fbytes(max_frame_size * channels * sizeof(decltype(out)::value_type)), data(max_payload_bytes) {}
        std::vector<short> out;
        std::vector<unsigned char> fbytes, data;
        int32_t frameno = 0;
        bool lost_prev = true;
    };

public:
    explicit GkAudioEncoding(std::shared_ptr<GekkoFyre::FileIo> fileIo,
                             QPointer<PaAudioBuf> input_audio_buf,
                             std::shared_ptr<GekkoFyre::GkLevelDb> database,
                             QPointer<SpectroGui> spectroGui,
                             std::shared_ptr<GekkoFyre::StringFuncs> stringFuncs,
                             GekkoFyre::Database::Settings::Audio::GkDevice input_device,
                             QObject *parent = nullptr);
    virtual ~GkAudioEncoding();

    void recordAudioFile(const GkAudioFramework::CodecSupport &codec, const GkAudioFramework::Bitrate &bitrate);

signals:
    void recAudioFrameOgg(std::vector<signed char> &audio_rec, const int &buf_size,
                          const GkAudioFramework::Bitrate &bitrate,
                          const boost::filesystem::path &filePath);
    void recAudioFramePcm(const std::vector<short> &audio_rec, const int &buf_size,
                          const boost::filesystem::path &filePath);
    void recAudioFrameFlac(const std::vector<short> &audio_rec, const int &buf_size,
                           const boost::filesystem::path &filePath);

    void submitOggVorbisBuf(const std::vector<signed char> &audio_frame_buf,
                            const GkAudioFramework::Bitrate &bitrate,
                            const boost::filesystem::path &filePath);
    void submitPcmBuf(const std::vector<short> &audio_rec, const boost::filesystem::path &filePath);
    void submitFlacBuf(const std::vector<short> &audio_rec, const boost::filesystem::path &filePath);

public slots:
    void startRecording(const bool &recording_is_started);

private slots:
    void oggVorbisBuf(std::vector<signed char> &audio_rec, const int &buf_size,
                      const GkAudioFramework::Bitrate &bitrate,
                      const boost::filesystem::path &filePath);

    void recordOggVorbis(const std::vector<signed char> &audio_frame_buf,
                         const GkAudioFramework::Bitrate &bitrate,
                         const boost::filesystem::path &filePath);
    void recordPcm(const std::vector<short> &audio_rec, const boost::filesystem::path &filePath);
    void recordFlac(const std::vector<short> &audio_rec, const boost::filesystem::path &filePath);

private:
    std::shared_ptr<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::SpectroGui> gkSpectroGui;
    QPointer<GekkoFyre::PaAudioBuf> gkAudioBuf;
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GkLevelDb> gkDb;
    GekkoFyre::Database::Settings::Audio::GkDevice gkInputDev;

    bool recording_in_progress;
    static size_t ogg_buf_counter;
    std::unique_ptr<OpusState> opus_state;

    //
    // Threads
    //
    std::thread ogg_audio_frame_thread;

    static uint32_t char_to_int(char ch[4]);

};
};

Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::Bitrate);
Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::CodecSupport);
