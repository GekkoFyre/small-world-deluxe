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

#include "src/gk_multimedia.hpp"
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <cstdio>
#include <future>
#include <fstream>
#include <cstring>
#include <utility>
#include <exception>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>

#ifdef __cplusplus
} // extern "C"
#endif

#define RAW_OUT_ON_PLANAR true

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

/**
 * @brief GkMultimedia::GkMultimedia
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkMultimedia::GkMultimedia(QPointer<GekkoFyre::GkAudioDevices> audio_devs, GkDevice sysOutputAudioDev, GkDevice sysInputAudioDev,
                           QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                           QObject *parent) : QObject(parent)
{
    gkAudioDevices = std::move(audio_devs);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    gkSysOutputAudioDev = std::move(sysOutputAudioDev);
    gkSysInputAudioDev = std::move(sysInputAudioDev);

    return;
}

GkMultimedia::~GkMultimedia()
{}

/**
 * @brief GkMultimedia::findAudioStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param formatCtx
 * @return
 * @note targodan <https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg>
 */
qint32 GkMultimedia::findAudioStream(const AVFormatContext *formatCtx)
{
    qint32 audioStreamIndex = -1;
    for (size_t i = 0; i < formatCtx->nb_streams; ++i) {
        //
        // Use the first audio stream we can find.
        // NOTE: There may be more than one, depending on the file.
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }

    return audioStreamIndex;
}

/**
 * @brief GkMultimedia::receiveAndHandle is designed to receive as many frames as possible and handle them correctly.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codecCtx
 * @param frame
 * @return
 * @note targodan <https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg>
 */
qint32 GkMultimedia::receiveAndHandle(AVCodecContext *codecCtx, AVFrame *frame)
{
    qint32 err = 0;

    //
    // Read the packets from the decoder.
    // NOTE: Each packet may generate more than one frame, depending on the codec!
    while ((err = avcodec_receive_frame(codecCtx, frame)) == 0) {
        handleFrame(codecCtx, frame);
        av_frame_unref(frame);
    }

    return err;
}

/**
 * @brief GkMultimedia::handleFrame
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codecCtx
 * @param frame
 * @note targodan <https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg>
 */
void GkMultimedia::handleFrame(const AVCodecContext *codecCtx, const AVFrame *frame)
{
    if (av_sample_fmt_is_planar(codecCtx->sample_fmt) == 1) {
        //
        // This means that the data of each channel is in its own buffer.
        // => frame->extended_data[i] contains data for the i-th channel.
        for (qint32 s = 0; s < frame->nb_samples; ++s) {
            for (qint32 c = 0; c < codecCtx->channels; ++c) {
                float sample = getSample(codecCtx, frame->extended_data[c], s);
                fwrite(&sample, sizeof(float), 1, outFile);
            }
        }
    } else {
        // This means that the data of each channel is in the same buffer.
        // => frame->extended_data[0] contains data of all channels.
        if (RAW_OUT_ON_PLANAR) {
            fwrite(frame->extended_data[0], 1, frame->linesize[0], outFile);
        } else {
            for (int s = 0; s < frame->nb_samples; ++s) {
                for (int c = 0; c < codecCtx->channels; ++c) {
                    float sample = getSample(codecCtx, frame->extended_data[0], s * codecCtx->channels + c);
                    fwrite(&sample, sizeof(float), 1, outFile);
                }
            }
        }
    }

    return;
}

/**
 * @brief GkMultimedia::drainDecoder
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codecCtx
 * @param frame
 */
void GkMultimedia::drainDecoder(AVCodecContext *codecCtx, AVFrame *frame)
{
    qint32 err = 0;

    //
    // Some codecs may buffer frames. Sending NULL activates drain-mode!
    if ((err = avcodec_send_packet(codecCtx, NULL)) == 0) {
        //
        // Read the remaining packets from the decoder!
        err = receiveAndHandle(codecCtx, frame);
        if (err != AVERROR(EAGAIN) && err != AVERROR_EOF) {
            //
            // Neither EAGAIN nor EOF => Something went wrong!
            gkEventLogger->publishEvent(tr("Receive error #%1 encountered whilst decoding an audio file.")
            .arg(QString::number(err)), GkSeverity::Fatal, "", true, true, false, false, false);
        }
    } else {
        gkEventLogger->publishEvent(tr("Send error #%1 encountered whilst decoding an audio file.")
        .arg(QString::number(err)), GkSeverity::Fatal, "", true, true, false, false, false);
    }

    return;
}

/**
 * @brief GkMultimedia::getSample
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codecCtx
 * @param buffer
 * @param sampleIndex
 * @return
 * @note targodan <https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg>
 */
float GkMultimedia::getSample(const AVCodecContext *codecCtx, uint8_t *buffer, int sampleIndex)
{
    int64_t val = 0;
    float ret = 0;
    int sampleSize = av_get_bytes_per_sample(codecCtx->sample_fmt);
    switch (sampleSize) {
        case 1:
            //
            // 8-bit samples are always unsigned!
            val = reinterpret_cast<uint8_t *>(buffer)[sampleIndex];
            //
            // Make signed!
            val -= 127;
            break;
        case 2:
            val = reinterpret_cast<int16_t *>(buffer)[sampleIndex];
            break;
        case 4:
            val = reinterpret_cast<int32_t *>(buffer)[sampleIndex];
            break;
        case 8:
            val = reinterpret_cast<int64_t *>(buffer)[sampleIndex];
            break;
        default:
            gkEventLogger->publishEvent(tr("Invalid sample size, %1.")
            .arg(QString::number(sampleSize)), GkSeverity::Fatal, "", true, true, false, false, false);
            return 0;
    }

    return 0;
}

/**
 * @brief GkMultimedia::ffmpegDecodeAudioFile attempts to universally decode any given audio file into PCM data, provided it
 * is a supported codec as provided by FFmpeg.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical path to the given audio file to be decoded.
 * @return Whether the decoding process was a success or not.
 * @note Mathieu Rodic <https://rodic.fr/blog/libavcodec-tutorial-decode-audio-file/>,
 * targodan <https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg>,
 * FFmpeg Documentation <https://libav.org/documentation/doxygen/master/group__lavu__sampfmts.html>
 */
bool GkMultimedia::ffmpegDecodeAudioFile(const QFileInfo &file_path)
{
    //
    // Initalize all the muxers, demuxers, and protocols for the libavformat library!
    // NOTE: Does nothing if called twice during the course of a program's execution...
    av_register_all();

    qint32 ret = 0;

    //
    // Obtain the formatCtx from the given audio file!
    AVFormatContext *formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, file_path.canonicalFilePath().toStdString().c_str(), nullptr, 0) != 0) {
        gkEventLogger->publishEvent(tr("Unable to open file, \"%1\"!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    //
    // In case the file had no header, read some frames and find out which formatCtx and codecs are used.
    // This does not consume any data. Any read packets are buffered for later use.
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        gkEventLogger->publishEvent(tr("Could not retrieve stream info from file, \"%1\"!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    //
    // Try to find an audio stream!
    qint32 audioStreamIndex = findAudioStream(formatCtx);
    if (audioStreamIndex == -1) {
        //
        // No audio stream was found!
        gkEventLogger->publishEvent(tr("No available audio streams within file, \"%1\"!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        avformat_close_input(&formatCtx);
        return false;
    }

    //
    // Find the index of the first audio stream!
    qint32 stream_index =- 1;
    for (qint32 i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
    }

    if (stream_index == -1) {
        gkEventLogger->publishEvent(tr("Could not retrieve audio stream from file, \"%1\"!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    AVStream *stream = formatCtx->streams[stream_index];

    //
    // Find the correct decoder for the codecCtx!
    AVCodec *codec = avcodec_find_decoder(formatCtx->streams[audioStreamIndex]->codecpar->codec_id);
    if (codec == nullptr) {
        //
        // A decoder could not be found!
        gkEventLogger->publishEvent(tr("A suitable decoder could not be found for file, \"%1\"!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        avformat_close_input(&formatCtx);
        return false;
    }

    //
    // Initialize codec context for the decoder!
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    if (codecCtx == nullptr) {
        avformat_close_input(&formatCtx);
        gkEventLogger->publishEvent(tr("Failed to allocate a decoding context for stream #%1 with file, \"%2\"!")
        .arg(QString::number(stream_index), file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    //
    // Fill the codecCtx with the parameters of the codec used in the read file.
    qint32 err = 0;
    if ((err = avcodec_parameters_to_context(codecCtx, formatCtx->streams[audioStreamIndex]->codecpar)) != 0) {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        gkEventLogger->publishEvent(tr("Error #%1 with setting context parameters for file, \"%2\"!")
        .arg(QString::number(err), file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    //
    // Explicitly request non-planar data!
    codecCtx->request_sample_fmt = av_get_alt_sample_fmt(codecCtx->sample_fmt, 0);

    //
    // Initialize the decoder.
    if ((err = avcodec_open2(codecCtx, codec, nullptr)) != 0) {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    //
    // Print some intersting file information!
    analyzeAudioFileMetadata(file_path, codec, codecCtx, audioStreamIndex, true);

    AVFrame *frame = nullptr;
    if ((frame = av_frame_alloc()) == nullptr) {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    // Prepare the packet.
    AVPacket packet;

    // Set default values.
    av_init_packet(&packet);

    while ((err = av_read_frame(formatCtx, &packet)) != AVERROR_EOF) {
        if(err != 0) {
            gkEventLogger->publishEvent(tr("Read error #%1 whilst processing file, \"%2\"!")
            .arg(QString::number(err), file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
            break;
        }

        //
        // Does the packet belong to the correct stream?
        if (packet.stream_index != audioStreamIndex) {
            //
            // Free the buffers used by the frame and reset all fields!
            av_packet_unref(&packet);
            continue;
        }

        //
        // We have a valid packet => send it to the decoder!
        if ((err = avcodec_send_packet(codecCtx, &packet)) == 0) {
            //
            // The packet was sent successfully. We don't need it anymore.
            // => Free the buffers used by the frame and reset all fields.
            av_packet_unref(&packet);
        } else {
            //
            // Something went wrong...
            // EAGAIN is technically no error here but if it occurs we would need to buffer
            // the packet and send it again after receiving more frames. Thusly we handle it as an error here!
            gkEventLogger->publishEvent(tr("Read error #%1 whilst processing file, \"%2\"!")
            .arg(QString::number(err), file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
            break;
        }

        //
        // Receive and handle frames.
        // EAGAIN means we need to send before receiving again. So that's not an error!
        if ((err = receiveAndHandle(codecCtx, frame)) != AVERROR(EAGAIN)) {
            //
            // Not EAGAIN => Something went wrong.
            gkEventLogger->publishEvent(tr("Read error #%1 whilst processing file, \"%2\"!")
            .arg(QString::number(err), file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
            break; // Don't return, so we can clean up nicely.
        }
    }

    //
    // Drain the decoder!
    drainDecoder(codecCtx, frame);

    // Free all data used by the frame!
    av_frame_free(&frame);

    // Close the context and free all data associated to it, but not the context itself!
    avcodec_close(codecCtx);

    // Free the context itself!
    avcodec_free_context(&codecCtx);

    // We are done here. Close the input!
    avformat_close_input(&formatCtx);

    // Close the outfile!
    fclose(outFile);

    //
    // Success at last!
    return true;
}

/**
 * @brief GkMultimedia::loadRawFileData loads raw file data into a file-stream pointer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path
 * @param size
 * @return
 */
std::vector<char> GkMultimedia::loadRawFileData(const QString &file_path, ALsizei size)
{
    std::ifstream in(file_path.toStdString(), std::ios::binary);
    if (!in.is_open()) {
        gkEventLogger->publishEvent(tr("Error! Could not open file, \"%1\"!").arg(file_path), GkSeverity::Warning, "", false, true, false, true, false);
        return std::vector<char>();
    }

    //
    // TODO: Optimize this to use less memory!
    char *data = new char[size];
    in.read(data, size);

    std::vector<char> buf(data, data + size);
    delete[] data;
    return buf;
}

/**
 * @brief GkMultimedia::updateStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param source
 * @param format
 * @param sample_rate
 * @param buf
 * @param cursor
 * @note IndieGameDev.net <https://indiegamedev.net/2020/02/25/the-complete-guide-to-openal-with-c-part-2-streaming-audio/>
 */
void GkMultimedia::updateStream(const ALuint source, const ALenum &format, const int32_t &sample_rate,
                                const std::vector<char> &buf, size_t &cursor)
{
    ALint buffersProcessed = 0;
    alCall(alGetSourcei, source, AL_BUFFERS_PROCESSED, &buffersProcessed);

    if (buffersProcessed <= 0) {
        return;
    }

    while (--buffersProcessed) {
        ALuint buffer;
        alCall(alSourceUnqueueBuffers, source, 1, &buffer);

        ALsizei dataSize = GK_AUDIO_STREAM_BUF_SIZE;

        char *data = new char[dataSize];
        std::memset(data, 0, dataSize);

        std::size_t dataSizeToCopy = GK_AUDIO_STREAM_BUF_SIZE;
        if (cursor + GK_AUDIO_STREAM_BUF_SIZE > buf.size()) {
            dataSizeToCopy = buf.size() - cursor;
        }

        std::memcpy(&data[0], &buf[cursor], dataSizeToCopy);
        cursor += dataSizeToCopy;

        if (dataSizeToCopy < GK_AUDIO_STREAM_BUF_SIZE) {
            cursor = 0;
            std::memcpy(&data[dataSizeToCopy], &buf[cursor], GK_AUDIO_STREAM_BUF_SIZE - dataSizeToCopy);
            cursor = GK_AUDIO_STREAM_BUF_SIZE - dataSizeToCopy;
        }

        alCall(alBufferData, buffer, format, data, GK_AUDIO_STREAM_BUF_SIZE, sample_rate);
        alCall(alSourceQueueBuffers, source, 1, &buffer);

        delete[] data;
    }

    return;
}

/**
 * @brief GkMultimedia::analyzeAudioFileMetadata will analyze a given multimedia audio file and output the metadata
 * contained within, provided there is any.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical location of the multimedia audio file to be analyzed in question.
 * @return The meta data of the analyzed multimedia audio file.
 * @note Both the TagLib and FFmpeg libraries are used within this function together for synergistic effects.
 * targodan <https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg>
 */
GkAudioFramework::GkAudioFileInfo GkMultimedia::analyzeAudioFileMetadata(const QFileInfo &file_path, const AVCodec *codec,
                                                                         const AVCodecContext *codecCtx, qint32 audioStreamIndex,
                                                                         const bool &printToConsole) const
{
    try {
        if (file_path.exists() && file_path.isReadable()) { // Check that the QFileInfo parameter given is valid and the file in question exists!
            if (file_path.isFile()) { // Are we dealing with a file or directory?
                qint32 ret = 0;
                GkAudioFramework::GkAudioFileInfo audioFileInfo;
                TagLib::FileRef fileRef(file_path.canonicalFilePath().toStdString().c_str());
                if (!fileRef.isNull() && fileRef.file() && codec->sample_fmts != nullptr) {
                    audioFileInfo.file_size = file_path.size();
                    audioFileInfo.file_size_hr = gkStringFuncs->fileSizeHumanReadable(audioFileInfo.file_size);

                    std::shared_ptr<GkAudioFramework::GkAudioFileProperties> info = std::make_shared<GkAudioFramework::GkAudioFileProperties>();
                    info->bitrate = fileRef.audioProperties()->bitrate();
                    info->sampleRate = codecCtx->sample_rate;
                    info->channels = codecCtx->channels;
                    info->stream_idx =audioStreamIndex;
                    info->sample_format = codecCtx->sample_fmt;
                    info->sample_format_str = av_get_sample_fmt_name(info->sample_format);
                    info->sample_size = av_get_bytes_per_sample(info->sample_format);
                    info->float_output = av_sample_fmt_is_planar(codecCtx->sample_fmt) != 0 ? "yes" : "no";
                    info->lengthInMilliseconds = fileRef.audioProperties()->lengthInMilliseconds();
                    info->lengthInSeconds = fileRef.audioProperties()->lengthInSeconds();

                    GkAudioFramework::GkAudioFileMetadata meta;
                    TagLib::Tag *tag = fileRef.tag();
                    meta.title = QString::fromWCharArray(tag->title().toCWString());
                    meta.artist = QString::fromWCharArray(tag->artist().toCWString());
                    meta.album = QString::fromWCharArray(tag->album().toCWString());
                    meta.year_raw = tag->year();
                    meta.comment = QString::fromWCharArray(tag->comment().toCWString());
                    meta.track_no = tag->track();
                    meta.genre = QString::fromWCharArray(tag->genre().toCWString());

                    audioFileInfo.type_codec_str = codec->long_name;

                    audioFileInfo.metadata = meta;
                    audioFileInfo.info = info;

                    if (printToConsole) {
                        //
                        // Print a separator...
                        std::cout << "--------------------------------------------------" << std::endl;

                        //
                        // Print out some useful information!
                        std::cout << tr("Stream Index: #").toStdString() << info->stream_idx << std::endl;
                        std::cout << tr("Bitrate: ").toStdString() << info->bitrate << std::endl;
                        std::cout << tr("Sample rate: ").toStdString() << info->sampleRate << std::endl;
                        std::cout << tr("Channels: ").toStdString() << info->channels << std::endl;
                        std::cout << tr("Sample format: ").toStdString() << info->sample_format_str.toStdString() << std::endl;
                        std::cout << tr("Sample size: ").toStdString() << info->sample_size << std::endl;
                        std::cout << tr("Float output: ").toStdString() << info->float_output << std::endl;

                        //
                        // Print a separator...
                        std::cout << "--------------------------------------------------" << std::endl;
                    }

                    return audioFileInfo;
                } else {
                    throw std::runtime_error(tr("Unable to initialize FFmpeg object. Out of memory?").toStdString());
                }
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return GkAudioFramework::GkAudioFileInfo();
}

/**
 * @brief GkMultimedia::getOutputAudioDevice returns the output audio device. Useful for when working across different
 * classes within C++.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The output audio device.
 */
GkDevice GkMultimedia::getOutputAudioDevice()
{
    return gkSysOutputAudioDev;
}

/**
 * @brief GkMultimedia::getInputAudioDevice returns the input audio device. Useful for when working across different
 * classes within C++.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The input audio device.
 */
GkDevice GkMultimedia::getInputAudioDevice()
{
    return gkSysInputAudioDev;
}

/**
 * @brief GkMultimedia::decodeAudioFile attempts to decode a given audio file according to a number of detected parameters,
 * some of which are provided by FFmpeg and others which too are provided by TagLib. It is intended as a universal approach
 * to decoding audio files of all types, provided they are both supported by FFmpeg and TagLib, but primarily the former
 * which does the heavy lifting in terms of the decoding work itself. TagLib only detects the metadata and doesn't do
 * any decoding work on its own.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical path to the given audio file to be decoded.
 * @return Whether the operation was a success or not.
 * @note Mathieu Rodic <https://rodic.fr/blog/libavcodec-tutorial-decode-audio-file/>
 */
bool GkMultimedia::decodeAudioFile(const QFileInfo &file_path)
{
    if (file_path.isReadable() && file_path.exists()) {
        if (file_path.isFile()) {
            GkAudioFramework::GkAudioFileDecoded decoded;
            char *str1 = const_cast<char *>(file_path.completeBaseName().toStdString().c_str());
            char *str2 = const_cast<char *>(General::GkAudio::audioFileExtensionRaw);
            std::strcat(str1, str2);
            tmpFile = str1;
            outFile = fopen(str1, "w+");

            if (outFile == nullptr) {
                gkEventLogger->publishEvent(tr("Unable to open output file, \"%1\"!")
                .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", false, true, false, true, false);
            }

            //
            // https://baptiste-wicht.com/posts/2017/09/cpp11-concurrency-tutorial-futures.html
            std::chrono::milliseconds span (GK_AUDIO_OUTPUT_DECODE_TIMEOUT * 1000);
            std::future<bool> f = std::async(std::launch::deferred, &GkMultimedia::ffmpegDecodeAudioFile, this, file_path);
            while (f.wait_for(span) == std::future_status::timeout) {
                //
                // TODO: Program a QProgressBar here!
                continue;
            }

            const bool ret = f.get();
            if (!ret) {
                gkEventLogger->publishEvent(tr("Error encountered while attempting to decode file, \"%1\"!")
                .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
                return false;
            }

            return true;
        }
    }

    return false;
}

/**
 * @brief GkMultimedia::playAudioFile will attempt to play an audio file of any, given, supported audio format provided
 * it's either supported by FFmpeg or it's simply a WAV file.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical path to the given audio file to be played.
 * @note IndieGameDev.net <https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/>,
 * IndieGameDev.net <https://indiegamedev.net/2020/02/25/the-complete-guide-to-openal-with-c-part-2-streaming-audio/>
 */
void GkMultimedia::playAudioFile(const QFileInfo &file_path)
{
    try {
        if (!alIsExtensionPresent("AL_EXT_FLOAT32")) {
            throw std::runtime_error(tr("The OpenAL extension, \"AL_EXT_FLOAT32\", is not present! Please install it before continuing.").toStdString());
        }

        if (file_path.exists() && file_path.isReadable()) {
            if (file_path.isFile()) {
                if (gkSysOutputAudioDev.isEnabled && gkSysOutputAudioDev.alDevice && gkSysOutputAudioDev.alDeviceCtx && !gkSysOutputAudioDev.isStreaming) {
                    std::future<bool> f = std::async(std::launch::deferred, &GkMultimedia::decodeAudioFile, this, file_path);
                    if (f.get()) {
                        std::vector<char> buf = loadRawFileData(tmpFile, GK_AUDIO_STREAM_BUF_SIZE * GK_AUDIO_STREAM_NUM_BUFS);
                        if (!buf.empty()) {
                            //
                            // We have data we can possibly work with!
                            ALuint buffers[GK_AUDIO_STREAM_NUM_BUFS];
                            alCall(alGenBuffers, GK_AUDIO_STREAM_NUM_BUFS, &buffers[0]);

                            const size_t buf_size = buf.size() - buf.size() % 4;
                            for (size_t i = 0; i < GK_AUDIO_STREAM_NUM_BUFS; ++i) {
                                alCall(alBufferData, buffers[i], AL_FORMAT_STEREO_FLOAT32, &buf[i * GK_AUDIO_STREAM_BUF_SIZE], GK_AUDIO_STREAM_BUF_SIZE, 44100);
                            }

                            ALuint source;
                            alCall(alGenSources, 1, &source);
                            alCall(alSourcef, source, AL_PITCH, 1);
                            alCall(alSourcef, source, AL_GAIN, 1.0f);
                            alCall(alSource3f, source, AL_POSITION, 0, 0, 0);
                            alCall(alSource3f, source, AL_VELOCITY, 0, 0, 0);
                            alCall(alSourcei, source, AL_LOOPING, AL_FALSE);
                            alCall(alSourceQueueBuffers, source, GK_AUDIO_STREAM_NUM_BUFS, &buffers[0]);

                            //
                            // NOTE: If the audio is played faster, then the sampling rate might be incorrect. Check if
                            // the input sample rate for the re-sampler is the same as in the decoder. AND the output
                            // sample rate is the same you use in the encoder.
                            //
                            alCall(alSourcePlay, source);

                            ALint state = AL_PLAYING;
                            std::size_t cursor = GK_AUDIO_STREAM_BUF_SIZE * GK_AUDIO_STREAM_NUM_BUFS;
                            while (state == AL_PLAYING) {
                                updateStream(source, AL_FORMAT_STEREO_FLOAT32, 44100, buf, cursor);
                                alCall(alGetSourcei, source, AL_SOURCE_STATE, &state);
                            }

                            alCall(alDeleteSources, 1, &source);
                            alCall(alDeleteBuffers, GK_AUDIO_STREAM_NUM_BUFS, &buffers[0]);

                            return;
                        }
                    }
                } else {
                    throw std::invalid_argument(tr("An issue was detected with your choice of output audio device. Please check your configuration and try again.").toStdString());
                }
            }
        }

        // throw std::runtime_error(tr("An error was encountered while attempting to play file, \"%1\"!").arg(file_path.canonicalFilePath()).toStdString());
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
}

/**
 * @brief GkMultimedia::recordAudioFile attempts to record from a given input device to a specified file on the end-user's
 * storage device of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The file to make the recording towards.
 */
void GkMultimedia::recordAudioFile(const QFileInfo &file_path)
{
    return;
}

/**
 * @brief GkMultimedia::is_big_endian will detect the endianness of the end-user's system at runtime.
 * @author Pharap <https://stackoverflow.com/a/1001373>
 * @return True if Big Endian, otherwise False for Small Endianness.
 */
bool GkMultimedia::is_big_endian()
{
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1;
}

/**
 * @brief GkMultimedia::convert_to_int
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param buffer
 * @param len
 * @return
 */
std::int32_t GkMultimedia::convert_to_int(char *buffer, std::size_t len)
{
    std::int32_t a = 0;
    if (!is_big_endian()) { // Proceed if Little Endianness...
        std::memcpy(&a, buffer, len);
    } else {
        //
        // Big Endianness otherwise...
        for (std::size_t i = 0; i < len; ++i) {
            reinterpret_cast<char *>(&a)[3 - i] = buffer[i];
        }
    }

    return a;
}

/**
 * @brief GkMultimedia::loadWavFileHeader
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file
 * @param channels
 * @param sampleRate
 * @param bitsPerSample
 * @param size
 * @return
 * @note IndieGameDev.net <https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/>,
 * Florian Fainelli <https://ffainelli.github.io/openal-example/>
 */
bool GkMultimedia::loadWavFileHeader(std::ifstream &file, uint8_t &channels, int32_t &sampleRate, uint8_t &bitsPerSample,
                                     ALsizei &size)
{
    char buffer[4];
    if (!file.is_open()) {
        return false;
    }

    //
    // RIFF
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Unable to read RIFF while attempting to parse WAV file."), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    if (std::strncmp(buffer, "RIFF", 4) != 0) {
        gkEventLogger->publishEvent(tr("Unable to parse given audio! Not a valid WAV file (header doesn't begin with RIFF)."), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    //
    // The size of the file
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Unable to parse size of the WAV file."), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    //
    // The WAV itself
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Unable to parse WAV!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    if (std::strncmp(buffer, "WAVE", 4) != 0) {
        gkEventLogger->publishEvent(tr("Not a valid WAV file! The header does not contain a WAV."), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    //
    // "fmt/0"
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Unable to read, \"fmt/0\"!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    //
    // This is always '16', which is the size of the 'fmt' data chunk...
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Could not read the '16' within the, \"fmt/0\"!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    //
    // PCM should be '1'?
    if (!file.read(buffer, 2)) {
        gkEventLogger->publishEvent(tr("Could not read any PCM!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    //
    // The number of channels
    if (!file.read(buffer, 2)) {
        gkEventLogger->publishEvent(tr("Could not read the number of channels!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    channels = convert_to_int(buffer, 2);

    //
    // Sample rate
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Could not read the sample rate!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    sampleRate = convert_to_int(buffer, 4);

    //
    // (sampleRate * bitsPerSample * channels) / 8
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Unable to read: \"(sampleRate * bitsPerSample * channels) / 8\""), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    if (!file.read(buffer, 2)) {
        gkEventLogger->publishEvent(tr("Unknown error!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    //
    // bitsPerSample
    if (!file.read(buffer, 2)) {
        gkEventLogger->publishEvent(tr("Unable to read bits per sample!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    bitsPerSample = convert_to_int(buffer, 2);

    //
    // Data chunk header, "data"!
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Unable to read the data chunk header!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    if (std::strncmp(buffer, "data", 4) != 0) {
        gkEventLogger->publishEvent(tr("The given file is not a valid WAV sample (it does not have a valid 'data' tag)!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    //
    // Size of data
    if (!file.read(buffer, 4)) {
        gkEventLogger->publishEvent(tr("Unable to read data size!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    size = convert_to_int(buffer, 4);

    //
    // Cannot be at the end-of-file!
    if (file.eof()) {
        gkEventLogger->publishEvent(tr("Unexpectedly reached EOF on given file!"), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    if (file.fail()) {
        gkEventLogger->publishEvent(tr("Error! Fail state set on the file."), GkSeverity::Warning, "", true, true, false, false, false);
        return false;
    }

    return true;
}

/**
 * @brief GkMultimedia::loadWavFileData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path
 * @param channels
 * @param sampleRate
 * @param bitsPerSample
 * @param size
 * @return
 * @note IndieGameDev.net <https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/>,
 * Florian Fainelli <https://ffainelli.github.io/openal-example/>
 */
char *GkMultimedia::loadWavFileData(const QFileInfo &file_path, uint8_t &channels, int32_t &sampleRate,
                                    uint8_t &bitsPerSample, ALsizei &size)
{
    std::ifstream in(file_path.canonicalFilePath().toStdString(), std::ios::binary);
    if (!in.is_open()) {
        gkEventLogger->publishEvent(tr("Error! Could not open file, \"%1\"!").arg(file_path.canonicalFilePath()), GkSeverity::Warning, "", false, true, false, true, false);
        return nullptr;
    }

    if (!loadWavFileHeader(in, channels, sampleRate, bitsPerSample, size)) {
        gkEventLogger->publishEvent(tr("Unable to load WAV header of, \"%1\"!").arg(file_path.canonicalFilePath()), GkSeverity::Warning, "", false, true, false, true, false);
        return nullptr;
    }

    char *data = new char[size];
    in.read(data, size);

    return data;
}
