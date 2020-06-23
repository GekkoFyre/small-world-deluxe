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

#include "gk_audio_encoding.hpp"
#include <boost/exception/all.hpp>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <QStandardPaths>
#include <QMessageBox>

#ifdef __cplusplus
extern "C"
{
#endif

#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <opus_multistream.h>
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

namespace fs = boost::filesystem;
namespace sys = boost::system;

#define OGG_VORBIS_READ (1024)

// Static variables
size_t GkAudioEncoding::ogg_buf_counter = 0;

GkAudioEncoding::GkAudioEncoding(QPointer<FileIo> fileIo,
                                 std::shared_ptr<PaAudioBuf<int16_t>> audio_buf,
                                 std::shared_ptr<GkLevelDb> database,
                                 QPointer<GekkoFyre::SpectroGui> spectroGui,
                                 std::shared_ptr<StringFuncs> stringFuncs,
                                 Database::Settings::Audio::GkDevice input_device,
                                 QObject *parent)
{
    qRegisterMetaType<GekkoFyre::GkAudioFramework::Bitrate>("GekkoFyre::GkAudioFramework::Bitrate");
    qRegisterMetaType<GekkoFyre::GkAudioFramework::CodecSupport>("GekkoFyre::GkAudioFramework::CodecSupport");

    gkFileIo = std::move(fileIo);
    gkAudioBuf = std::move(audio_buf);
    gkStringFuncs = std::move(stringFuncs);
    gkDb = std::move(database);
    gkInputDev = std::move(input_device);
    gkSpectroGui = std::move(spectroGui);

    opus_state = std::make_unique<OpusState>(AUDIO_FRAMES_PER_BUFFER, AUDIO_CODECS_OPUS_MAX_PACKETS, gkInputDev.dev_input_channel_count);
    recording_in_progress = false;
    ogg_buf_counter = 0;

    QObject::connect(this, SIGNAL(recAudioFrameOgg(std::vector<signed char> &, const int &, const GkAudioFramework::Bitrate &, const boost::filesystem::path &)),
                     this, SLOT(oggVorbisBuf(std::vector<signed char> &, const int &, const GkAudioFramework::Bitrate &, const boost::filesystem::path &)));
    QObject::connect(this, SIGNAL(submitOggVorbisBuf(const std::vector<signed char> &, const GkAudioFramework::Bitrate &, const boost::filesystem::path &)),
                     this, SLOT(recordOggVorbis(const std::vector<signed char> &, const GkAudioFramework::Bitrate &, const boost::filesystem::path &)));
}

GkAudioEncoding::~GkAudioEncoding()
{}

/**
 * @brief GkAudioEncoding::recordAudioFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filePath
 * @param codec
 * @param bitrate
 */
void GkAudioEncoding::recordAudioFile(const CodecSupport &codec, const Bitrate &bitrate)
{
    std::mutex record_audio_file_mtx;
    std::lock_guard<std::mutex> lck_guard(record_audio_file_mtx);
    sys::error_code ec;

    try {
        fs::path default_path = fs::path(gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::MusicLocation), true).toStdString());

        QString audioRecLoc = gkDb->read_misc_audio_settings(audio_cfg::AudioRecLoc);
        fs::path audio_rec_path;

        if (!audioRecLoc.isEmpty()) {
            // There exists a given path that has been saved previously in the settings, perhaps by the user...
            audio_rec_path = fs::path(audioRecLoc.toStdString());
        } else {
            // We must create a new, default path by scratch...
            audio_rec_path = fs::path(gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::MusicLocation)).toStdString());
        }

        if (fs::exists(audio_rec_path, ec) && fs::is_directory(audio_rec_path, ec)) {
            // The given path already exists! Nothing more needs to be done with it...
        } else {
            // We need to create this given path firstly...
            if (!fs::create_directory(audio_rec_path, ec)) {
                throw std::runtime_error(tr("An issue was encountered while creating a path for audio file storage!").toStdString());
            }
        }

        if (codec == OggVorbis) {
            // Using Ogg Vorbis
            std::promise<std::vector<signed char>> ogg_frame_promise;
            std::future<std::vector<signed char>> ogg_frame_future = ogg_frame_promise.get_future();

            while (recording_in_progress) {
                // ogg_audio_frame_thread = std::thread(&PaAudioBuf<int16_t>::prepOggVorbisBuf, gkAudioBuf, std::move(ogg_frame_promise));
                std::vector<signed char> audio_frame_vec = ogg_frame_future.get();

                if (audio_frame_vec.empty()) {
                    throw std::runtime_error(tr("Audio data buffer frame is empty!").toStdString());
                }

                emit recAudioFrameOgg(audio_frame_vec, AUDIO_CODECS_OGG_VORBIS_BUFFER_SIZE, bitrate, audio_rec_path);
            }
        } else if (codec == Opus) {
            // Using Opus
        } else if (codec == FLAC) {
            // Using FLAC
        } else if (codec == PCM) {
            // Using PCM
        } else {
            // Unknown!
            throw std::invalid_argument(tr("Recording with unknown media (i.e. codec) format!").toStdString());
        }
    } catch (const sys::system_error &e) {
        ec = e.code();
        QMessageBox::warning(nullptr, tr("Error!"), ec.message().c_str(), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

void GkAudioEncoding::startRecording(const bool &recording_is_started)
{
    recording_in_progress = recording_is_started;

    return;
}

/**
 * @brief GkAudioEncoding::recordOggVorbis
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_frame_buf
 * @param filePath
 * @return
 */
void GkAudioEncoding::recordOggVorbis(const std::vector<signed char> &audio_frame_buf,
                                      const Bitrate &bitrate,
                                      const fs::path &filePath)
{
    std::mutex record_ogg_vorbis_mtx;
    std::lock_guard<std::mutex> lck_guard(record_ogg_vorbis_mtx);

    try {
        if (!audio_frame_buf.empty()) {
            signed char read_buffer[(OGG_VORBIS_READ * 4) + 44];
            size_t buf_size = ((OGG_VORBIS_READ * 4) + 44);
            ogg_stream_state os;            // Take physical pages, weld into a logical stream of packets.
            ogg_page og;                    // One Ogg bitstream page. Vorbis packets are inside.
            ogg_packet op;                  // One raw packet of data for decode.
            vorbis_info vi;                 // Struct that stores all the static vorbis bitstream settings.
            vorbis_comment vc;              // Struct that stores all the user comments.
            vorbis_dsp_state vd;            // Central working state for the packet->PCM decoder.
            vorbis_block vb;                // Local working space for packet->PCM decode.
            int eos = 0;

            vorbis_info_init(&vi);
            int ret = 0;
            #if defined(_MSC_VER) && (_MSC_VER > 1900)
            ret = vorbis_encode_init_vbr(&vi, gkInputDev.dev_input_channel_count, gkInputDev.def_sample_rate,
                                             AUDIO_CODECS_OGG_VORBIS_ENCODE_QUALITY);

            if (ret) {
                throw std::runtime_error(tr("There has been an error in initializing an Ogg Vorbis audio encode!").toStdString());
            }
            #elif __MINGW32__
            // TODO: Find a replacement for the above that's suitable within MinGW!
            #endif

            // Add a comment to the encoded file!
            vorbis_comment_init(&vc);
            vorbis_comment_add_tag(&vc, General::productName,
                                   tr("Encoded with %1 (v%2)").arg(General::productName).arg(General::appVersion)
                                   .toStdString().c_str());

            // Setup the analysis state and auxiliary encoding storage
            vorbis_analysis_init(&vd, &vi);
            vorbis_block_init(&vd, &vb);

            //
            // Setup our `packet->stream` encoder.
            // Also pick a random serial number; that way we can more likely build
            // chained streams just by concatenation.
            //
            srand(time(nullptr));
            ogg_stream_init(&os, rand());

            //
            // Vorbis streams begin with three headers; the initial header (with
            // most of the codec setup parameters) which is mandated by the Ogg
            // bitstream spec.  The second header holds any comment fields.  The
            // third header holds the bitstream codebook.  We merely need to
            // make the headers, then pass them to libvorbis one at a time;
            // libvorbis handles the additional Ogg bitstream constraints
            //

            ogg_packet header;
            ogg_packet header_comm;
            ogg_packet header_code;

            vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
            ogg_stream_packetin(&os, &header); // Automatically placed in its own page

            ogg_stream_packetin(&os, &header_comm);
            ogg_stream_packetin(&os, &header_code);

            // This ensures the actual audio data will start on a new page, as per spec
            while (!eos) {
                int result = ogg_stream_flush(&os, &og);
                if (result == 0) {
                    break;
                }

                fwrite(og.header, 1, og.header_len, stdout);
                fwrite(og.body, 1, og.body_len, stdout);
            }

            while (!eos) {
                long i;

                for (size_t j = 0; j < audio_frame_buf.size(); ++j) {
                    for (size_t k = 0; k < buf_size; ++k) {
                        // Break up the data!
                        read_buffer[k] = audio_frame_buf[j];
                        // j += k;
                    }

                    long bytes = fread(read_buffer, 1, (OGG_VORBIS_READ * 4), stdin); // Stereo hardwired here
                    if (bytes == 0) {
                        //
                        // End of file.  This can be done implicitly in the mainline,
                        // but it's easier to see here in non-clever fashion.
                        // Tell the library we're at end of stream so that it can handle
                        // the last frame and mark end of stream in the output properly.
                        //
                        vorbis_analysis_wrote(&vd, 0);
                    } else {
                        // Data to encode
                        // Expose the buffer to submit data
                        float **buffer = vorbis_analysis_buffer(&vd, OGG_VORBIS_READ);

                        // Uninterleave samples
                        for (i = 0; i < bytes / 4; ++i) {
                            buffer[0][i] = ((read_buffer[i * 4 + 1] << 8) | (0x00ff&(int)read_buffer[i * 4])) / 32768.f;
                            buffer[1][i] = ((read_buffer[i * 4 + 3] << 8) | (0x00ff&(int)read_buffer[i * 4 + 2])) / 32768.f;
                        }

                        // Tell the library how much we actually submitted
                        vorbis_analysis_wrote(&vd, i);
                    }
                }

                // Vorbis does some data preanalysis, then divvies up blocks for
                // more involved (potentially parallel) processing.  Get a single
                // block for encoding now.
                while (vorbis_analysis_blockout(&vd, &vb) == 1) {
                    // Analysis, assume we want to use bitrate management
                    vorbis_analysis(&vb, nullptr);
                    vorbis_bitrate_addblock(&vb);

                    while(vorbis_bitrate_flushpacket(&vd, &op)) {
                        // Weld the packet into the bitstream
                        ogg_stream_packetin(&os, &op);
                        // Write out pages (if any)
                        while (!eos) {
                            int result = ogg_stream_pageout(&os, &og);
                            if (result == 0) {
                                break;
                            }

                            fwrite(og.header, 1, og.header_len, stdout);
                            fwrite(og.body, 1, og.body_len, stdout);

                            // This could be set above, but for illustrative purposes, I do
                            // it here (to show that vorbis does know where the stream ends).
                            if (ogg_page_eos(&og)) {
                                eos = 1;
                            }
                        }
                    }
                }
            }

            //
            // Clean up and exit.  vorbis_info_clear() must be called last.
            //
            ogg_stream_clear(&os);
            vorbis_block_clear(&vb);
            vorbis_dsp_clear(&vd);
            vorbis_comment_clear(&vc);
            vorbis_info_clear(&vi);

            return;
        }
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_record_ogg_vorbis_vec = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_record_ogg_vorbis_vec, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_record_ogg_vorbis_vec);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()));
        #endif
    }

    return;
}

void GkAudioEncoding::recordPcm(const std::vector<short> &audio_rec, const boost::filesystem::path &filePath)
{
    Q_UNUSED(filePath);
    Q_UNUSED(audio_rec);

    return;
}

void GkAudioEncoding::recordFlac(const std::vector<short> &audio_rec, const boost::filesystem::path &filePath)
{
    Q_UNUSED(filePath);
    Q_UNUSED(audio_rec);

    return;
}

/**
 * @brief GkAudioEncoding::char_to_int
 * @author sehe <https://stackoverflow.com/questions/16496288/decoding-opus-audio-data>
 * @param ch
 * @return
 * @note The author suggests reading with `boost::spirit::big_dword` or similar instead.
 */
uint32_t GkAudioEncoding::char_to_int(char ch[])
{
    return static_cast<uint32_t>(static_cast<unsigned char>(ch[0])<<24) |
               static_cast<uint32_t>(static_cast<unsigned char>(ch[1])<<16) |
               static_cast<uint32_t>(static_cast<unsigned char>(ch[2])<< 8) |
            static_cast<uint32_t>(static_cast<unsigned char>(ch[3])<< 0);
}

void GkAudioEncoding::oggVorbisBuf(std::vector<signed char> &audio_rec, const int &buf_size,
                                   const Bitrate &bitrate,
                                   const boost::filesystem::path &filePath)
{
    std::vector<signed char> total_buf;
    while (ogg_buf_counter < buf_size) {
        total_buf.insert(std::end(total_buf), std::begin(audio_rec), std::end(audio_rec));

        //
        // Remove the elements from the std::vector (i.e. `audio_rec`) that are already present
        // within `total_buf`.
        //
        auto pred = [&total_buf](const signed char &key)->bool {
            return std::find(total_buf.begin(), total_buf.end(), key) != total_buf.end();
        };

        audio_rec.erase(std::remove_if(audio_rec.begin(), audio_rec.end(), pred), audio_rec.end());
        ogg_buf_counter += audio_rec.size();
    }

    emit submitOggVorbisBuf(total_buf, bitrate, filePath);
    ogg_buf_counter = 0;

    return;
}

const char *GkAudioEncoding::OpusErrorException::what() const noexcept
{
    return opus_strerror(code);
}
