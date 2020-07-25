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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/gk_audio_decoding.hpp"
#include <boost/exception/all.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ostream>
#include <istream>
#include <iostream>
#include <ios>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef OPUS_LIBS_ENBLD
#include <opus_multistream.h>
#endif

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef __cplusplus
}
#endif

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

namespace fs = boost::filesystem;
namespace sys = boost::system;

GkAudioDecoding::GkAudioDecoding(QPointer<FileIo> fileIo,
                                 std::shared_ptr<GkLevelDb> database,
                                 std::shared_ptr<StringFuncs> stringFuncs,
                                 Database::Settings::Audio::GkDevice output_device,
                                 QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                 QObject *parent)
{
    setParent(parent);

    gkFileIo = std::move(fileIo);
    gkStringFuncs = std::move(stringFuncs);
    gkDb = std::move(database);
    gkEventLogger = std::move(eventLogger);

    gkOutputDev = output_device;

    // opus_state = std::make_unique<OpusState>(AUDIO_FRAMES_PER_BUFFER, AUDIO_CODECS_OPUS_MAX_PACKETS, gkOutputDev.dev_output_channel_count);
}

GkAudioDecoding::~GkAudioDecoding()
{}

/**
 * @brief GkAudioDecoding::gatherOggInfo
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filePath
 * @return
 */
AudioFileInfo GkAudioDecoding::gatherOggInfo(const boost::filesystem::path &filePath,
                                             sys::error_code &ec)
{
    std::mutex gather_ogg_info_mtx;
    std::lock_guard<std::mutex> lck_guard(gather_ogg_info_mtx);

    AudioFileInfo audio_file_info = initAudioFileInfoStruct();

    try {
        // Gather info from the given audio file!
        if (fs::exists(filePath, ec)) {
            if (fs::is_regular_file(filePath, ec)) {
                std::ifstream stream;
                stream.open(filePath.c_str(), std::ios::binary);
                OggVorbis_File ogg_file;

                const ov_callbacks callbacks { readOgg, seekOgg, nullptr, tellOgg };
                int result = ov_open_callbacks(&stream, &ogg_file, nullptr, 0, callbacks);
                if (result < 0) {
                    throw std::invalid_argument(tr("Error opening file: %1").arg(result).toStdString());
                }

                // Read file info
                vorbis_info *vorbis_info = ov_info(&ogg_file, -1);
                audio_file_info.type_codec = CodecSupport::OggVorbis;
                audio_file_info.sample_rate = vorbis_info->rate;
                audio_file_info.bitrate_lower = vorbis_info->bitrate_lower;
                audio_file_info.bitrate_upper = vorbis_info->bitrate_upper;
                audio_file_info.bitrate_nominal = vorbis_info->bitrate_nominal;
                audio_file_info.num_audio_channels = gkDb->convertAudioChannelsEnum(vorbis_info->channels);

                return audio_file_info;
            }
        }
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_gather_ogg_info = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_gather_ogg_info, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_gather_ogg_info);
        #else
        gkEventLogger->publishEvent(tr("An error occurred during the handling of waterfall / spectrograph data!"), GkSeverity::Error, e.what(), true);
        #endif
    }

    return audio_file_info;
}

/**
 * @brief GkAudioDecoding::initAudioFileInfoStruct
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
AudioFileInfo GkAudioDecoding::initAudioFileInfoStruct()
{
    AudioFileInfo audio_file_info;
    audio_file_info.audio_file_path = "";
    audio_file_info.is_output = false;
    audio_file_info.sample_rate = 0;
    audio_file_info.type_codec = CodecSupport::Unknown;
    audio_file_info.num_audio_channels = Database::Settings::audio_channels::Unknown;

    return audio_file_info;
}

/**
 * @brief GkAudioDecoding::char_to_int
 * @author sehe <https://stackoverflow.com/questions/16496288/decoding-opus-audio-data>
 * @param ch
 * @return
 */
uint32_t GkAudioDecoding::char_to_int(char ch[])
{
    return static_cast<uint32_t>(static_cast<unsigned char>(ch[0])<<24) |
               static_cast<uint32_t>(static_cast<unsigned char>(ch[1])<<16) |
               static_cast<uint32_t>(static_cast<unsigned char>(ch[2])<< 8) |
            static_cast<uint32_t>(static_cast<unsigned char>(ch[3])<< 0);
}

#ifdef OPUS_LIBS_ENBLD
/**
 * @brief GkAudioDecoding::decodeOpusFrame
 * @author sehe <https://stackoverflow.com/questions/16496288/decoding-opus-audio-data>
 * @param file_in
 * @param file_out
 * @return
 */
bool GkAudioDecoding::decodeOpusFrame(std::istream &file_in, std::ostream &file_out)
{
    char ch[4] = { 0 };

    if (!file_in.read(ch, 4) && file_in.eof()) {
        return false;
    }

    uint32_t len = char_to_int(ch);

    if(len > opus_state->data.size()) {
        throw std::runtime_error(tr("Invalid payload length").toStdString());
    }

    file_in.read(ch, 4);
    const uint32_t enc_final_range = char_to_int(ch);
    const auto data = reinterpret_cast<char *>(&opus_state->data.front());

    size_t read = 0ul;
    for (auto append_position = data; file_in && read < len; append_position += read) {
        read += file_in.readsome(append_position, len - read);
    }

    if (read < len) {
        QString read_error_msg = tr("Ran out of input, expecting %1 bytes but instead, got %2 at %3!")
                .arg(QString::number(len)).arg(QString::number(read)).arg(file_in.tellg());
        throw std::runtime_error(read_error_msg.toStdString());
    }

    int output_samples;
    const bool lost = (len == 0);
    if (lost) {
        opus_decoder_ctl(_decoder.get(), OPUS_GET_LAST_PACKET_DURATION(&output_samples));
    } else {
        output_samples = opus_max_frame_size;
    }

    output_samples = opus_decode(_decoder.get(), lost ? NULL : opus_state->data.data(), len, opus_state->out.data(), output_samples, 0);
    if (output_samples > 0) {
        for (int i = 0; i < (output_samples) * gkOutputDev.dev_output_channel_count; i++) {
            short s;
            s = opus_state->out[i];
            opus_state->fbytes[2 * i]   = s&0xFF;
            opus_state->fbytes[2 * i + 1] = (s >> 8)&0xFF;
        }

        if (!file_out.write(reinterpret_cast<char *>(opus_state->fbytes.data()), sizeof(short) * gkOutputDev.dev_output_channel_count * output_samples)) {
            throw std::runtime_error(tr("Error writing").toStdString());
        }
    } else {
        throw OpusErrorException(output_samples); // Negative return is error code...
    }

    uint32_t dec_final_range;
    opus_decoder_ctl(_decoder.get(), OPUS_GET_FINAL_RANGE(&dec_final_range));

    // Compare final range encoder range values of encoder and decoder
    if(enc_final_range != 0 && !lost && !opus_state->lost_prev && dec_final_range != enc_final_range) {
        std::ostringstream oss;
        oss << tr("Error: Range coder state mismatch between encoder and decoder in frame ").toStdString() << opus_state->frameno << ": " <<
               "0x" << std::setw(8) << std::setfill('0') << std::hex << (unsigned long)enc_final_range <<
               "0x" << std::setw(8) << std::setfill('0') << std::hex << (unsigned long)dec_final_range;

        throw std::runtime_error(oss.str());
    }

    opus_state->lost_prev = lost;
    opus_state->frameno++;

    return true;
}
#endif

/**
 * @brief GkAudioDecoding::readOgg
 * @author Daniel Wolf <https://stackoverflow.com/questions/52121854/how-to-use-ov-open-callbacks-to-open-an-ogg-vorbis-file-from-a-stream>
 * @param buffer
 * @param element_size
 * @param element_count
 * @param data_source
 * @return
 */
size_t GkAudioDecoding::readOgg(void *buffer, size_t element_size, size_t element_count, void *data_source)
{
    std::mutex read_ogg_file_mtx;
    std::lock_guard<std::mutex> lck_guard(read_ogg_file_mtx);

    std::ifstream &stream = *static_cast<std::ifstream *>(data_source);
    stream.read(static_cast<char *>(buffer), element_count);
    const std::streamsize bytes_read = stream.gcount();
    stream.clear(); // In case we read past the end-of-file

    return static_cast<size_t>(bytes_read);
}

/**
 * @brief GkAudioDecoding::seekOgg
 * @author Daniel Wolf <https://stackoverflow.com/questions/52121854/how-to-use-ov-open-callbacks-to-open-an-ogg-vorbis-file-from-a-stream>
 * @param data_source
 * @param offset
 * @param origin
 * @return
 */
int GkAudioDecoding::seekOgg(void *data_source, ogg_int64_t offset, int origin)
{
    std::mutex seek_ogg_file_mtx;
    std::lock_guard<std::mutex> lck_guard(seek_ogg_file_mtx);

    static const std::vector<std::ios_base::seekdir> seekDirections {
        std::ios_base::beg, std::ios_base::cur, std::ios_base::end
    };

    std::ifstream &stream = *static_cast<std::ifstream *>(data_source);
    stream.seekg(offset, seekDirections.at(origin));
    stream.clear(); // In case we read past the end-of-file

    return 0;
}

/**
 * @brief GkAudioDecoding::tellOgg
 * @author Daniel Wolf <https://stackoverflow.com/questions/52121854/how-to-use-ov-open-callbacks-to-open-an-ogg-vorbis-file-from-a-stream>
 * @param data_source
 * @return
 */
long GkAudioDecoding::tellOgg(void *data_source)
{
    std::mutex tell_ogg_file_mtx;
    std::lock_guard<std::mutex> lck_guard(tell_ogg_file_mtx);

    std::ifstream &stream = *static_cast<std::ifstream *>(data_source);
    const auto position = stream.tellg();
    assert(position >= 0);

    return static_cast<long>(position);
}

#ifdef OPUS_LIBS_ENBLD
const char *GkAudioDecoding::OpusErrorException::what() const noexcept
{
    return opus_strerror(code);
}
#endif
