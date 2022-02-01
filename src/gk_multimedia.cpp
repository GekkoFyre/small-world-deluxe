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
using namespace Network;
using namespace GkXmpp;
using namespace Security;

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
    if (playAudioFileThread.joinable()) {
        emit updateAudioState(GkAudioState::Stopped); // Stop the playing and/or recording of all audio files!
    }

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
                    } while (alGetError() == AL_NO_ERROR && state == AL_PLAYING && gkAudioState == GkAudioState::Playing);

                    //
                    // Cleanup now as we're done!
                    alCall(alDeleteSources, GK_AUDIO_STREAM_NUM_BUFS, &source);
                    alCall(alDeleteBuffers, GK_AUDIO_STREAM_NUM_BUFS, &buffer);
                    emit playingFinished();

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
 * @brief GkMultimedia::mediaAction initiates a given multimedia action such as the playing of a multimedia file, or the
 * recording to a certain multimedia file, or even the stopping of all such processes, and does this via the actioning
 * of multi-threaded processes.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_state The media state we are supposed to action.
 * @param file_path The audio file we are supposed to be playing or recording towards, given the right multimedia
 * action.
 */
void GkMultimedia::mediaAction(const GkAudioState &media_state, const QFileInfo &file_path)
{
    if (media_state == GkAudioState::Playing) {
        //
        // Playing
        playAudioFileThread = std::thread(&GkMultimedia::playAudioFile, this, file_path);
        playAudioFileThread.detach();

        return;
    } else if (media_state == GkAudioState::Recording) {
        //
        // Recording

        return;
    } else if (media_state == GkAudioState::Stopped) {
        //
        // Stopped

        return;
    } else {
        //
        // Unknown value given!
        throw std::invalid_argument(tr("Unable to action multimedia with given, invalid value!").toStdString());
    }

    return;
}

/**
 * @brief GkMultimedia::setAudioState sets the new audio/multimedia state, whether that be that Small World Deluxe is
 * currently playing a audio file, recording to an audio file, or has ceased all activities of the sort.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioState The new audio state.
 */
void GkMultimedia::setAudioState(const GkAudioState &audioState)
{
    gkAudioState = audioState;

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
