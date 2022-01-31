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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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
#include <cstdio>
#include <future>
#include <cstring>
#include <utility>
#include <exception>

#ifdef __cplusplus
extern "C"
{
#endif

#include <sndfile.h>
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
{
    if (inputFile.is_open()) {
        inputFile.close();
    }
}

/**
 * @brief GkMultimedia::loadAudioFile loads an audio file along with its properties into memory and creates an OpenAL
 * audio buffer along the way, ready for audio playback.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical path to the given audio file to be analyzed and then loaded into memory.
 * @return The thusly created OpenAL audio buffer.
 * @note OpenAL Source Play Example <https://github.com/kcat/openal-soft/blob/master/examples/alplay.c>.
 */
ALuint GkMultimedia::loadAudioFile(const QFileInfo &file_path)
{
    try {
        ALenum format;
        ALuint buffer;
        SNDFILE *sndfile;
        SF_INFO sfinfo;
        short *membuf;
        sf_count_t num_frames;
        ALsizei num_bytes;

        //
        // Initialize libsndfile!
        sndfile = sf_open(file_path.canonicalFilePath().toStdString().c_str(), SFM_READ, &sfinfo);
        if (!sndfile) {
            throw std::runtime_error(tr("Unable to open audio file and thusly initialize libsndfile, \"%1\"!")
                                             .arg(file_path.canonicalFilePath()).toStdString());
        }

        if (sfinfo.frames < 1 || sfinfo.frames > (sf_count_t)(INT_MAX / sizeof(short)) / sfinfo.channels) {
            throw std::runtime_error(tr("Bad sample count in, \"%1\" (%2 frames)!")
                                             .arg(file_path.canonicalFilePath(), sfinfo.frames).toStdString());
        }

        //
        // Obtain the sound format, and then determine the OpenAL format!
        format = AL_NONE;
        if (sfinfo.channels == 1) {
            format = AL_FORMAT_MONO16;
        } else if (sfinfo.channels == 2) {
            format = AL_FORMAT_STEREO16;
        } else if (sfinfo.channels == 3) {
            if (sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, nullptr, 0) == SF_AMBISONIC_B_FORMAT) {
                format = AL_FORMAT_BFORMAT2D_16;
            }
        } else if (sfinfo.channels == 4) {
            if (sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, nullptr, 0) == SF_AMBISONIC_B_FORMAT) {
                format = AL_FORMAT_BFORMAT3D_16;
            }
        } else {
            sf_close(sndfile);
            throw std::invalid_argument(tr("Unsupported number of audio channels given: \"%1 channels\"!")
                                                .arg(QString::number(sfinfo.channels)).toStdString());
        }

        //
        // Decode the entire audio file to a given buffer!
        membuf = static_cast<short *>(malloc((size_t) (sfinfo.frames * sfinfo.channels) * sizeof(short)));
        num_frames = sf_readf_short(sndfile, membuf, sfinfo.frames);
        if (num_frames < 1) {
            free(membuf);
            sf_close(sndfile);
            throw std::runtime_error(tr("Failed to read samples in %1 (%2 frames)!")
                                             .arg(file_path.canonicalFilePath(), QString::number(num_frames)).toStdString());
        }

        num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

        //
        // Buffer the audio data into a new buffer object, then free the data and close the given file!
        alCall(alGenBuffers, GK_AUDIO_STREAM_NUM_BUFS, &buffer);
        alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

        free(membuf);
        sf_close(sndfile);

        return buffer;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return ALuint();
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
 * @brief GkMultimedia::ffmpegDecodeAudioPacket attempts to universally decode any given audio file into PCM data, provided it
 * is a supported codec as provided by FFmpeg.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codecCtx Contexual information for the audio file's codec and other errata.
 * @return Whether the decoding process was a success or not.
 * @note TheSHEEEP <https://stackoverflow.com/a/21404721>.
 */
bool GkMultimedia::ffmpegDecodeAudioPacket(AVCodecContext *codecCtx)
{
    SwrContext *swrCtx = swr_alloc_set_opts(nullptr, codecCtx->channel_layout, AV_SAMPLE_FMT_FLT,
                                            codecCtx->sample_rate, codecCtx->channel_layout, codecCtx->sample_fmt,
                                            codecCtx->sample_rate, 0, nullptr);
    qint32 result = swr_init(swrCtx);

    //
    // Create the destination buffer!
    uint8_t **destBuf = nullptr;
    qint32 destBufLineSize;
    av_samples_alloc_array_and_samples(&destBuf, &destBufLineSize, codecCtx->channels,
                                       2048, AV_SAMPLE_FMT_FLT, 0);

    return false;
}

/**
 * @brief GkMultimedia::loadRawFileData loads raw file data into a file-stream pointer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path
 * @param size
 * @return
 */
std::vector<char> GkMultimedia::loadRawFileData(const QString &file_path, ALsizei size, const size_t &cursor)
{
    if (!inputFile.is_open()) {
        inputFile.open(file_path.toStdString(), std::ios::binary);
        if (!inputFile.is_open()) {
            gkEventLogger->publishEvent(tr("Error! Could not open file, \"%1\"!").arg(file_path), GkSeverity::Warning, "", false, true, false, true, false);
            return std::vector<char>();
        }
    }

    //
    // TODO: Optimize this to use less memory!
    char *data = new char[size];
    inputFile.read(data, size);

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

    //
    // Create outside the loop for performance reasons...
    ALsizei dataSize = GK_AUDIO_STREAM_BUF_SIZE;
    char *data = new char[dataSize];

    while (--buffersProcessed) {
        ALuint buffer;
        alCall(alSourceUnqueueBuffers, source, 1, &buffer);

        //
        // Initialize the buffer with a padding of zeroes!
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
 * @brief GkMultimedia::playAudioFile will attempt to play an audio file of any, given, supported audio format provided
 * it's either supported by FFmpeg or it's simply a WAV file.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical path to the given audio file to be played.
 * @note OpenAL Source Play Example <https://github.com/kcat/openal-soft/blob/master/examples/alplay.c>.
 */
void GkMultimedia::playAudioFile(const QFileInfo &file_path)
{
    try {
        if (!alIsExtensionPresent("AL_EXT_FLOAT32")) {
            throw std::runtime_error(tr("The OpenAL extension, \"AL_EXT_FLOAT32\", is not present! Please install it before continuing.").toStdString());
        }

        if (file_path.exists() && file_path.isFile()) {
            if (file_path.isReadable()) {
                if (gkSysOutputAudioDev.isEnabled && gkSysOutputAudioDev.alDevice && gkSysOutputAudioDev.alDeviceCtx &&
                    !gkSysOutputAudioDev.isStreaming) {
                    ALuint source;
                    ALfloat offset;
                    ALenum state;

                    auto buffer = loadAudioFile(file_path);
                    if (!buffer) {
                        throw std::invalid_argument(tr("Error encountered with creating OpenAL buffer!").toStdString());
                    }

                    //
                    // Create the source to play the sound with!
                    source = 0;
                    alCall(alGenSources, 1, &source);
                    alCall(alSourcef, source, AL_PITCH, 1);
                    alCall(alSourcef, source, AL_GAIN, 1.0f);
                    alCall(alSource3f, source, AL_POSITION, 0, 0, 0);
                    alCall(alSource3f, source, AL_VELOCITY, 0, 0, 0);
                    alCall(alSourcei, source, AL_BUFFER, (ALint)buffer);

                    //
                    // NOTE: If the audio is played faster, then the sampling rate might be incorrect. Check if
                    // the input sample rate for the re-sampler is the same as in the decoder. AND the output
                    // sample rate is the same you use in the encoder.
                    //
                    alCall(alSourcePlay, source);
                    do {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        alGetSourcei(source, AL_SOURCE_STATE, &state);

                        alGetSourcef(source, AL_SEC_OFFSET, &offset);
                        printf("\rOffset: %f  ", offset);
                        fflush(stdout);
                    } while (alGetError() == AL_NO_ERROR && state == AL_PLAYING);

                    //
                    // Cleanup now as we're done!
                    alCall(alDeleteSources, GK_AUDIO_STREAM_NUM_BUFS, &source);
                    alCall(alDeleteBuffers, GK_AUDIO_STREAM_NUM_BUFS, &buffer);

                    return;
                } else {
                    throw std::invalid_argument(tr("An issue was detected with your choice of output audio device. Please check your configuration and try again.").toStdString());
                }
            }
        }
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
