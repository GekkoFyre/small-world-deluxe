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
#include "src/audio_devices.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/dek_db.hpp"
#include <portaudiocpp/System.hxx>
#include <ogg/os_types.h>
#include <boost/filesystem.hpp>
#include <memory>
#include <string>
#include <future>
#include <thread>
#include <vector>
#include <exception>
#include <QObject>
#include <QString>
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
} // extern "C"
#endif

namespace GekkoFyre {

class GkAudioDecoding : public QObject {
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
    explicit GkAudioDecoding(std::shared_ptr<GekkoFyre::FileIo> fileIo,
                             QPointer<GekkoFyre::PaAudioBuf> audio_buf,
                             std::shared_ptr<GekkoFyre::GkLevelDb> database,
                             std::shared_ptr<GekkoFyre::StringFuncs> stringFuncs,
                             GekkoFyre::Database::Settings::Audio::GkDevice output_device,
                             QObject *parent = nullptr);
    virtual ~GkAudioDecoding();

    GekkoFyre::GkAudioFramework::AudioFileInfo gatherOggInfo(const boost::filesystem::path &filePath,
                                                             boost::system::error_code &ec);
    GekkoFyre::GkAudioFramework::AudioFileInfo initAudioFileInfoStruct();

private:
    std::shared_ptr<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::PaAudioBuf> gkAudioBuf;
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GkLevelDb> gkDb;
    GekkoFyre::Database::Settings::Audio::GkDevice gkOutputDev;

    struct OpusDecoderDeleter {
        void operator()(OpusDecoder *opusDecoder) const {
            opus_decoder_destroy(opusDecoder);
        }
    };

    std::unique_ptr<OpusDecoder, OpusDecoderDeleter> _decoder;
    std::unique_ptr<OpusState> opus_state;
    const int opus_max_frame_size = AUDIO_FRAMES_PER_BUFFER;

    static uint32_t char_to_int(char ch[4]);
    bool decodeOpusFrame(std::istream &file_in, std::ostream &file_out);

    static size_t readOgg(void *buffer, size_t element_size, size_t element_count, void *data_source);
    static int seekOgg(void *data_source, ogg_int64_t offset, int origin);
    static long tellOgg(void *data_source);

};
};
