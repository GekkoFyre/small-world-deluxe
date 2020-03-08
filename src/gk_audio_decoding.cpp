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

#include "gk_audio_decoding.hpp"
#include <boost/exception/all.hpp>
#include <fstream>
#include <iomanip>

#ifdef __cplusplus
extern "C"
{
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
namespace fs = boost::filesystem;
namespace sys = boost::system;

GkAudioDecoding::GkAudioDecoding(portaudio::System *paInit,
                                 std::shared_ptr<FileIo> fileIo,
                                 std::shared_ptr<AudioDevices> audioDevs,
                                 QPointer<PaAudioBuf> audio_buf,
                                 std::shared_ptr<GkLevelDb> database,
                                 std::shared_ptr<StringFuncs> stringFuncs,
                                 Database::Settings::Audio::GkDevice output_device,
                                 QObject *parent)
{
    gkFileIo = fileIo;
    gkAudioDevices = audioDevs;
    gkAudioBuf = audio_buf;
    gkStringFuncs = stringFuncs;
    gkDb = database;

    gkPaInit = paInit;
    gkOutputDev = output_device;
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
        HWND hwnd_gather_ogg_info;
        gkStringFuncs->modalDlgBoxOk(hwnd_gather_ogg_info, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_gather_ogg_info);
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
