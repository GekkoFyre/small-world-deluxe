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
#include <utility>
#include <exception>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>

#ifdef __cplusplus
} // extern "C"
#endif

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
 * @brief GkMultimedia::ffmpegDecodeAudioFile attempts to universally decode any given audio file into PCM data, provided it
 * is a supported codec as provided by FFmpeg.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical path to the given audio file to be decoded.
 * @param sample_rate The sample rate at which the audio file is to be decoded into.
 * @param data The raw data as provided by the conversion process.
 * @param size The size of the raw data as provided by the conversion process.
 * @return Whether the decoding process was a success or not.
 * @note Mathieu Rodic <https://rodic.fr/blog/libavcodec-tutorial-decode-audio-file/>,
 * TheSHEEEP <https://stackoverflow.com/a/21404721>
 */
bool GkMultimedia::ffmpegDecodeAudioFile(const QFileInfo &file_path, const qint32 &sample_rate, float **data, qint32 *size)
{
    //
    // Initalize all the muxers, demuxers, and protocols for the libavformat library!
    // NOTE: Does nothing if called twice during the course of a program's execution...
    av_register_all();

    //
    // Obtain the format from the given audio file!
    AVFormatContext *format = avformat_alloc_context();
    if (avformat_open_input(&format, file_path.canonicalFilePath().toStdString().c_str(), nullptr, nullptr) != 0) {
        gkEventLogger->publishEvent(tr("Unable to open file, \"%1\"!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    if (avformat_find_stream_info(format, nullptr) < 0) {
        gkEventLogger->publishEvent(tr("Could not retrieve stream info from file, \"%1\"!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    //
    // Find the index of the first audio stream!
    qint32 stream_index =- 1;
    for (qint32 i = 0; i < format->nb_streams; ++i) {
        if (format->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
    }

    if (stream_index == -1) {
        gkEventLogger->publishEvent(tr("Could not retrieve audio stream from file, \"%1\"!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    AVStream *stream = format->streams[stream_index];

    //
    // Find and open the codec!
    AVCodecContext *codec = stream->codec;
    if (avcodec_open2(codec, avcodec_find_decoder(codec->codec_id), nullptr) < 0) {
        gkEventLogger->publishEvent(tr("Failed to open decoder for stream #%1 in file, \"%1\"!")
        .arg(QString::number(stream_index), file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    //
    // Prepare re-sampler!
    struct SwrContext* swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_count",  codec->channels, 0);
    av_opt_set_int(swr, "out_channel_count", 2, 0);
    av_opt_set_int(swr, "in_channel_layout",  codec->channel_layout, 0);
    av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(swr, "in_sample_rate", codec->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate", sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt",  codec->sample_fmt, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLT,  0);
    swr_init(swr);

    if (!swr_is_initialized(swr)) {
        gkEventLogger->publishEvent(tr("Unable to initialize re-sampler due to unknown reasons!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    //
    // Prepare to read the data
    AVPacket packet;
    av_init_packet(&packet);
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        gkEventLogger->publishEvent(tr("An error was encountered while allocating the frame!")
        .arg(file_path.canonicalFilePath()), GkSeverity::Fatal, "", true, true, false, false, false);
        return false;
    }

    //
    // Iterate through the frames
    *data = nullptr;
    *size = 0;
    while (av_read_frame(format, &packet) >= 0) {
        //
        // Decode only a single frame
        qint32 gotFrame;
        if (avcodec_decode_audio4(codec, frame, &gotFrame, &packet) < 0) {
            break;
        }

        if (!gotFrame) {
            continue;
        }

        //
        // Resample the frames
        float *buffer;
        av_samples_alloc((uint8_t**) &buffer, nullptr, 1, frame->nb_samples, AV_SAMPLE_FMT_DBL, 0);
        qint32 frame_count = swr_convert(swr, (uint8_t **) &buffer, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);

        //
        // Append the resampled frames to the data pointer!
        *data = (float *)std::realloc(*data, (*size + frame->nb_samples) * sizeof(float));
        std::memcpy(*data + *size, buffer, frame_count * sizeof(float));
        *size += frame_count;
    }

    //
    // Cleanup
    av_frame_free(&frame);
    swr_free(&swr);
    avcodec_close(codec);
    avformat_free_context(format);

    //
    // Success at last!
    return true;
}

/**
 * @brief GkMultimedia::analyzeAudioFileMetadata will analyze a given multimedia audio file and output the metadata
 * contained within, provided there is any.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical location of the multimedia audio file to be analyzed in question.
 * @return The meta data of the analyzed multimedia audio file.
 * @note Both the TagLib and FFmpeg libraries are used within this function together for synergistic effects.
 */
GkAudioFramework::GkAudioFileInfo GkMultimedia::analyzeAudioFileMetadata(const QFileInfo &file_path) const
{
    try {
        if (file_path.exists() && file_path.isReadable()) { // Check that the QFileInfo parameter given is valid and the file in question exists!
            if (file_path.isFile()) { // Are we dealing with a file or directory?
                qint32 ret = 0;
                GkAudioFramework::GkAudioFileInfo audioFileInfo;
                AVFormatContext *pFormatCtx = avformat_alloc_context();
                TagLib::FileRef fileRef(file_path.canonicalFilePath().toStdString().c_str());
                if (!fileRef.isNull() && fileRef.file() && pFormatCtx) {
                    AVDictionaryEntry *dict = nullptr;
                    ret = avformat_open_input(&pFormatCtx, file_path.canonicalFilePath().toStdString().c_str(), nullptr, nullptr);
                    if (ret < 0) {
                        throw std::runtime_error(tr("Error with opening file via FFmpeg for multimedia analysis. File: %1")
                                                         .arg(file_path.canonicalFilePath()).toStdString());
                    }

                    ret = avformat_find_stream_info(pFormatCtx, nullptr);
                    if (ret < 0) {
                        throw std::runtime_error(tr("Error with finding stream info within file via FFmpeg. File: %1")
                                                         .arg(file_path.canonicalFilePath()).toStdString());
                    }

                    audioFileInfo.file_size = file_path.size();
                    audioFileInfo.file_size_hr = gkStringFuncs->fileSizeHumanReadable(audioFileInfo.file_size);

                    std::shared_ptr<GkAudioFramework::GkAudioFileProperties> info = std::make_shared<GkAudioFramework::GkAudioFileProperties>();
                    info->bitrate = fileRef.audioProperties()->bitrate();
                    info->sampleRate = fileRef.audioProperties()->sampleRate();
                    info->channels = fileRef.audioProperties()->channels();
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

                    audioFileInfo.type_codec_str = avcodec_get_name(pFormatCtx->audio_codec_id);

                    audioFileInfo.metadata = meta;
                    audioFileInfo.info = info;

                    //
                    // Print a separator...
                    std::cout << "--------------------------------------------------" << std::endl;

                    //
                    // Now print out the key/value pairs!
                    bool existingValues = false;
                    while ((dict = av_dict_get(pFormatCtx->metadata, "", dict, AV_DICT_IGNORE_SUFFIX))) {
                        QString key = dict->key;
                        QString value = dict->value;
                        if (!key.isEmpty() && !value.isEmpty()) {
                            std::cout << QString("[ %1 ]: %2").arg(key, value).toStdString() << std::endl;
                            existingValues = true;
                        }
                    }

                    if (!existingValues) { // Were any key/value pairs detected? If not, let the end-user know so that they don't suspect an error otherwise!
                        std::cout << tr("Unable to detect any metadata within multimedia file, \"%1\"!").arg(file_path.canonicalFilePath()).toStdString() << std::endl;
                    }

                    //
                    // Print a separator...
                    std::cout << "--------------------------------------------------" << std::endl;

                    //
                    // Terminate any hanging pointers so that there are no memory leaks!
                    avformat_close_input(&pFormatCtx);
                    avformat_free_context(pFormatCtx);

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
GkAudioFramework::GkAudioFileDecoded GkMultimedia::decodeAudioFile(const QFileInfo &file_path)
{
    try {
        if (file_path.isReadable() && file_path.exists()) {
            if (file_path.isFile()) {
                float *data_ptr;
                qint32 size = 0;
                GkAudioFramework::GkAudioFileDecoded decoded;
                decoded.properties = analyzeAudioFileMetadata(file_path);
                const bool ret = ffmpegDecodeAudioFile(file_path, decoded.properties.info->sampleRate, &data_ptr, &size);

                if (!ret) {
                    if (data_ptr) {
                        delete[] data_ptr;
                    }

                    throw std::runtime_error(tr("Error encountered while atttempting to decode file, \"%1\"!").arg(file_path.canonicalFilePath()).toStdString());
                }

                decoded.samples.assign(data_ptr, data_ptr + size);
                if (data_ptr) {
                    delete[] data_ptr;
                }

                return decoded;
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return GkAudioFramework::GkAudioFileDecoded();
}

/**
 * @brief GkMultimedia::playAudioFile will attempt to play an audio file of any, given, supported audio format provided
 * it's either supported by FFmpeg or it's simply a WAV file.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical path to the given audio file to be played.
 * @note IndieGameDev.net <https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/>
 */
void GkMultimedia::playAudioFile(const QFileInfo &file_path)
{
    try {
        if (file_path.exists() && file_path.isReadable()) {
            if (file_path.isFile()) {
                if (gkSysOutputAudioDev.isEnabled && gkSysOutputAudioDev.alDevice && gkSysOutputAudioDev.alDeviceCtx) {
                    if (!gkSysOutputAudioDev.isStreaming) {
                        auto decoded = decodeAudioFile(file_path);
                        if (!decoded.samples.empty()) {
                            //
                            // We have data we can possibly work with!
                            ALuint buffer;
                            alCall(alGenBuffers, 1, &buffer);

                            const size_t buf_size = decoded.samples.size() - decoded.samples.size() % 4;
                            alCall(alBufferData, buffer, AL_FORMAT_STEREO_FLOAT32, decoded.samples.data(), buf_size, decoded.properties.info->sampleRate);
                            decoded.samples.clear(); // Erase the audio samples to free up RAM!

                            ALuint source;
                            alCall(alGenSources, 1, &source);
                            alCall(alSourcef, source, AL_PITCH, 1);
                            alCall(alSourcef, source, AL_GAIN, 0.75f);
                            alCall(alSource3f, source, AL_POSITION, 0, 0, 0);
                            alCall(alSource3f, source, AL_VELOCITY, 0, 0, 0);
                            alCall(alSourcei, source, AL_LOOPING, AL_FALSE);
                            alCall(alSourcei, source, AL_BUFFER, buffer);

                            alCall(alSourcePlay, source);
                            ALint state = AL_PLAYING;

                            while (state == AL_PLAYING) {
                                alCall(alGetSourcei, source, AL_SOURCE_STATE, &state);
                            }

                            alCall(alDeleteSources, 1, &source);
                            alCall(alDeleteBuffers, 1, &buffer);

                            return;
                        }
                    } else {
                        throw std::runtime_error(tr("Output audio device, \"%1\", is already in use!").arg(gkSysOutputAudioDev.audio_dev_str).toStdString());
                    }
                } else {
                    throw std::invalid_argument(tr("An issue was detected with your choice of output audio device. Please check your configuration and try again.").toStdString());
                }
            }
        }

        throw std::runtime_error(tr("An error was encountered while attempting to play file, \"%1\"!").arg(file_path.canonicalFilePath()).toStdString());
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

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
