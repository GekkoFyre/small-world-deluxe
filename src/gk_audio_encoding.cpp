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

#include "gk_audio_encoding.hpp"
#include <boost/exception/all.hpp>
#include <vector>
#include <fstream>

#ifdef __cplusplus
extern "C"
{
#endif

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

namespace fs = boost::filesystem;
namespace sys = boost::system;

GkAudioEncoding::GkAudioEncoding(portaudio::System *paInit,
                                 std::shared_ptr<FileIo> fileIo,
                                 std::shared_ptr<AudioDevices> audioDevs,
                                 QPointer<PaAudioBuf> audio_buf,
                                 std::shared_ptr<GkLevelDb> database,
                                 std::shared_ptr<StringFuncs> stringFuncs,
                                 Database::Settings::Audio::GkDevice input_device,
                                 QObject *parent)
{
    gkFileIo = fileIo;
    gkAudioDevices = audioDevs;
    gkAudioBuf = audio_buf;
    gkStringFuncs = stringFuncs;
    gkDb = database;

    gkPaInit = paInit;
    gkInputDev = input_device;
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
void GkAudioEncoding::recordAudioFile(const boost::filesystem::path &filePath, const CodecSupport &codec,
                                      const Bitrate &bitrate)
{
    std::mutex record_audio_file_mtx;
    std::lock_guard<std::mutex> lck_guard(record_audio_file_mtx);

    bool ret = false;
    try {
        if (codec == OggVorbis) {
            // Using Ogg Vorbis
            // bool ret = recordOggVorbis();
            if (!ret) {
                throw std::runtime_error(tr("Unknown error while recording with Ogg Vorbis!").toStdString());
            }
        } else if (codec == FLAC) {
            // Using FLAC
        } else if (codec == PCM) {
            // Using PCM
        } else {
            // Unknown!
            throw std::invalid_argument(tr("Recording with unknown media (i.e. codec) format!").toStdString());
        }
    } catch (const std::exception &e) {
        HWND hwnd_record_audio_file_vec;
        gkStringFuncs->modalDlgBoxOk(hwnd_record_audio_file_vec, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_record_audio_file_vec);
    }

    return;
}

/**
 * @brief GkAudioEncoding::recordOggVorbis
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filePath
 * @return
 */
bool GkAudioEncoding::recordOggVorbis(const fs::path &filePath)
{
    std::mutex record_ogg_vorbis_mtx;
    std::lock_guard<std::mutex> lck_guard(record_ogg_vorbis_mtx);

    try {
        if (gkPaInit != nullptr) {
            #define READ (1024)
            signed char read_buffer[(READ * 4) + 44];
            ogg_stream_state os;            // Take physical pages, weld into a logical stream of packets.
            ogg_page og;                    // One Ogg bitstream page. Vorbis packets are inside.
            ogg_packet op;                  // One raw packet of data for decode.
            vorbis_info vi;                 // Struct that stores all the static vorbis bitstream settings.
            vorbis_comment vc;              // Struct that stores all the user comments.
            vorbis_dsp_state vd;            // Central working state for the packet->PCM decoder.
            vorbis_block vb;                // Local working space for packet->PCM decode.
            int eos = 0;

            vorbis_info_init(&vi);
            int ret = vorbis_encode_init_vbr(&vi, gkInputDev.dev_input_channel_count, gkInputDev.def_sample_rate,
                                             AUDIO_CODECS_OGG_VORBIS_ENCODE_QUALITY);
            if (ret) {
                throw std::runtime_error(tr("There has been an error in initializing an Ogg Vorbis audio encode!").toStdString());
            }

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
                long bytes = fread(read_buffer, 1, (READ * 4), stdin); // Stereo hardwired here
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
                    float **buffer = vorbis_analysis_buffer(&vd, READ);

                    // Uninterleave samples
                    for (i = 0; i < bytes / 4; ++i) {
                        buffer[0][i] = ((read_buffer[i * 4 + 1] << 8) | (0x00ff&(int)read_buffer[i * 4])) / 32768.f;
                        buffer[1][i] = ((read_buffer[i * 4 + 3] << 8) | (0x00ff&(int)read_buffer[i * 4 + 2])) / 32768.f;
                    }

                    // Tell the library how much we actually submitted
                    vorbis_analysis_wrote(&vd, i);
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

            return true;
        }
    } catch (const std::exception &e) {
        HWND hwnd_record_ogg_vorbis_vec;
        gkStringFuncs->modalDlgBoxOk(hwnd_record_ogg_vorbis_vec, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_record_ogg_vorbis_vec);
    }

    return false;
}
