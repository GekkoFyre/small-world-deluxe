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
#include <QMessageBox>

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
GkMultimedia::GkMultimedia(QPointer<GekkoFyre::GkAudioDevices> audio_devs, std::vector<GkDevice> sysOutputAudioDevs,
                           std::vector<GkDevice> sysInputAudioDevs, QPointer<GekkoFyre::GkLevelDb> database,
                           QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                           QObject *parent) : gkAudioState(GkAudioState::Stopped), m_frameSize(0), m_recordBuffer(0),
                           QObject(parent)
{
    gkAudioDevices = std::move(audio_devs);
    gkDb = std::move(database);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    gkSysOutputAudioDevs = std::move(sysOutputAudioDevs);
    gkSysInputAudioDevs = std::move(sysInputAudioDevs);

    //
    // Initialize variables
    audioPlaybackSource = 0;

    return;
}

GkMultimedia::~GkMultimedia()
{
    if (playAudioFileThread.joinable() || recordAudioFileThread.joinable()) {
        emit updateAudioState(GkAudioState::Stopped); // Stop the playing and/or recording of all audio files!
    }

    return;
}

/**
 * @brief GkMultimedia::convFFmpegCodecIdToEnum converts a given, QComboBox index to its equivalent enumerator as found
 * within the FFmpeg source code. Only codecs officially supported and tested by Small World Deluxe are convertable.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec_id_str The QComboBox index to be converted towards its equivalent enumerator, as found within the FFmpeg
 * source code.
 * @return The equivalent enumerator, as found within the FFmpeg source code.
 */
AVCodecID GkMultimedia::convFFmpegCodecIdToEnum(const qint32 &codec_id_str)
{
    switch (codec_id_str) {
        case AUDIO_PLAYBACK_CODEC_PCM_IDX:
            return AV_CODEC_ID_PCM_S16LE;
        case AUDIO_PLAYBACK_CODEC_LOOPBACK_IDX:
            return AV_CODEC_ID_NONE;
        case AUDIO_PLAYBACK_CODEC_VORBIS_IDX:
            return AV_CODEC_ID_VORBIS;
        case AUDIO_PLAYBACK_CODEC_CODEC2_IDX:
            return AV_CODEC_ID_CODEC2;
        case AUDIO_PLAYBACK_CODEC_OPUS_IDX:
            return AV_CODEC_ID_OPUS;
        case AUDIO_PLAYBACK_CODEC_FLAC_IDX:
            return AV_CODEC_ID_FLAC;
        default:
            return AV_CODEC_ID_NONE;
    }

    return AV_CODEC_ID_NONE;
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
 * @brief GkMultimedia::ffmpegCheckSampleFormat checks that a given sample format is supported by the encoder.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec The desired codec to be used and its associated information.
 * @param sample_fmt The desired sample format the check is to be made against.
 * @return Whether a positive result has been garnered or not.
 * @note Fabrice Bellard <https://ffmpeg.org/doxygen/trunk/encode__audio_8c_source.html>.
 */
qint32 GkMultimedia::ffmpegCheckSampleFormat(const AVCodec *codec, const AVSampleFormat &sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt) {
            return 1;
        }

        ++p;
    }

    return 0;
}

/**
 * @brief GkMultimedia::ffmpegSelectSampleRate simply picks the highest available and supported sample rate.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec The desired codec to be used and its associated information.
 * @return The best, possible and highest available sample rate that we can use with regards to encoding an audio file.
 */
qint32 GkMultimedia::ffmpegSelectSampleRate(const AVCodec *codec)
{
    const qint32 *p;
    qint32 bestSampleRate = 0;

    if (!codec->supported_samplerates) {
        return GK_AUDIO_FFMPEG_DEFAULT_SAMPLE_RATE;
    }

    p = codec->supported_samplerates;
    while (*p) {
        if (!bestSampleRate || std::abs(GK_AUDIO_FFMPEG_DEFAULT_SAMPLE_RATE - *p) < std::abs(GK_AUDIO_FFMPEG_DEFAULT_SAMPLE_RATE - bestSampleRate)) {
            bestSampleRate = *p;
        }

        ++p;
    }

    return bestSampleRate;
}

/**
 * @brief GkMultimedia::ffmpegSelectChannelLayout will select/determine a layout with the possible highest channel count.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec The desired codec to be used and its associated information.
 * @return
 * @note Fabrice Bellard <https://ffmpeg.org/doxygen/trunk/encode__audio_8c_source.html>.
 */
qint32 GkMultimedia::ffmpegSelectChannelLayout(const AVCodec *codec)
{
    const uint64_t *p;
    uint64_t bestChLayout = 0;
    qint32 bestNbChannels = 0;

    if (!codec->channel_layouts) {
        return AV_CH_LAYOUT_STEREO;
    }

    p = codec->channel_layouts;
    while (*p) {
        qint32 nbChannels = av_get_channel_layout_nb_channels(*p);
        if (nbChannels > bestNbChannels) {
            bestChLayout = *p;
            bestNbChannels = nbChannels;
        }

        ++p;
    }

    return bestChLayout;
}

/**
 * @brief GkMultimedia::ffmpegEncodeAudioPacket attempts to universally encode any given audio file via given PCM data,
 * provided the desired codec is a supported format as provided by FFmpeg.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codecCtx Contexual information for the audio file's codec and other errata.
 * @param frame
 * @param pkt
 * @param output
 * @note Fabrice Bellard <https://ffmpeg.org/doxygen/trunk/encode__audio_8c_source.html>.
 */
void GkMultimedia::ffmpegEncodeAudioPacket(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt, QFile *output)
{
    try {
        qint32 ret = 0;

        //
        // Send the frame for encoding via FFmpeg!
        ret = avcodec_send_frame(codecCtx, frame);
        if (ret < 0) {
            throw std::invalid_argument(tr("Error encountered with sending audio frame to the encoder!").toStdString());
        }

        //
        // Read all the available output packets (in general there maybe any number of them)!
        while (ret >= 0) {
            ret = avcodec_receive_packet(codecCtx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                return;
            } else if (ret < 0) {
                throw std::runtime_error(tr("Error encountered with encoding audio frame!").toStdString());
            }

            char *buf;
            std::memcpy(buf, pkt->data, pkt->size);
            output->write(buf);
            av_packet_unref(pkt);
        }

        return;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
}

/**
 * @brief GkMultimedia::openAlSelectBitDepth is for querying against the OpenAL libraries alongside the end-user's
 * desired configuration, so that the highest bit-depth that we are able to make use of can be chosen. This is
 * particularly pertinent towards encoding audio with FFmpeg.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bit_depth The highest bit-depth (16-bits, 24-bits, etc.) that we are able to make use of.
 * @return The highest, available bit-depth that we are able to make use of.
 */
qint32 GkMultimedia::openAlSelectBitDepth(const ALenum &bit_depth)
{
    switch (bit_depth) {
        case AL_FORMAT_MONO8:
            return 8;
        case AL_FORMAT_MONO16:
            return 16;
        case AL_FORMAT_STEREO8:
            return 8;
        case AL_FORMAT_STEREO16:
            return 16;
        default:
            return AL_FORMAT_STEREO16; // Default value to return!
    }

    return 0;
}

/**
 * @brief GkMultimedia::getOutputAudioDevice returns the output audio device. Useful for when working across different
 * classes within C++.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The output audio device.
 */
GkDevice GkMultimedia::getOutputAudioDevice()
{
    for (const auto &output_audio_dev: gkSysOutputAudioDevs) {
        if (output_audio_dev.isEnabled) {
            return output_audio_dev;
        }
    }

    return GkDevice();
}

/**
 * @brief GkMultimedia::getInputAudioDevice returns the input audio device. Useful for when working across different
 * classes within C++.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The input audio device.
 */
GkDevice GkMultimedia::getInputAudioDevice()
{
    for (const auto &input_audio_dev: gkSysInputAudioDevs) {
        if (input_audio_dev.isEnabled) {
            return input_audio_dev;
        }
    }

    return GkDevice();
}

/**
 * @brief GkMultimedia::checkOpenAlExtensions checks if certain extensions/plugins are present or not within an
 * end-user's OpenAL implementation, on their system. An exception is thrown otherwise if they are not present.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkMultimedia::checkOpenAlExtensions()
{
    try {
        if (!alIsExtensionPresent("AL_EXT_FLOAT32")) {
            throw std::runtime_error(tr("The OpenAL extension, \"AL_EXT_FLOAT32\", is not present! Please ensure you have the correct configuration before continuing.").toStdString());
        }

        if (!alIsExtensionPresent("AL_EXT_BFORMAT")) {
            throw std::runtime_error(tr("The OpenAL extension, \"AL_EXT_BFORMAT\", is not present! Please ensure you have the correct configuration before continuing.").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
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
        checkOpenAlExtensions();
        if (file_path.exists()) {
            if (file_path.isFile()) {
                //
                // We are working with a file
                if (file_path.isReadable()) {
                    const auto outputAudioDev = getOutputAudioDevice();
                    if (outputAudioDev.isEnabled && outputAudioDev.alDevice && outputAudioDev.alDeviceCtx &&
                        !outputAudioDev.isStreaming) {
                        ALfloat offset;
                        ALenum state;

                        auto buffer = loadAudioFile(file_path);
                        if (!buffer) {
                            throw std::invalid_argument(tr("Error encountered with creating OpenAL buffer!").toStdString());
                        }

                        //
                        // Create the source to play the sound with!
                        alCall(alGenSources, 1, &audioPlaybackSource);
                        alCall(alSourcef, audioPlaybackSource, AL_PITCH, 1);
                        alCall(alSourcef, audioPlaybackSource, AL_GAIN, 1.0f);
                        alCall(alSource3f, audioPlaybackSource, AL_POSITION, 0, 0, 0);
                        alCall(alSource3f, audioPlaybackSource, AL_VELOCITY, 0, 0, 0);
                        alCall(alSourcei, audioPlaybackSource, AL_BUFFER, (ALint)buffer);

                        //
                        // NOTE: If the audio is played faster, then the sampling rate might be incorrect. Check if
                        // the input sample rate for the re-sampler is the same as in the decoder. AND the output
                        // sample rate is the same you use in the encoder.
                        //
                        alCall(alSourcePlay, audioPlaybackSource);
                        do {
                            std::this_thread::sleep_for(std::chrono::milliseconds(GK_AUDIO_VOL_PLAYBACK_REFRESH_INTERVAL));
                            alGetSourcei(audioPlaybackSource, AL_SOURCE_STATE, &state);

                            alGetSourcef(audioPlaybackSource, AL_SEC_OFFSET, &offset);
                            printf("\rOffset: %f  ", offset);
                            fflush(stdout);
                        } while (alGetError() == AL_NO_ERROR && state == AL_PLAYING && gkAudioState == GkAudioState::Playing);

                        //
                        // Cleanup now as we're done!
                        alCall(alDeleteSources, GK_AUDIO_STREAM_NUM_BUFS, &audioPlaybackSource);
                        alCall(alDeleteBuffers, GK_AUDIO_STREAM_NUM_BUFS, &buffer);
                        emit playingFinished();

                        return;
                    } else {
                        throw std::invalid_argument(tr("An issue was detected with your choice of output audio device. Please check your configuration and try again.").toStdString());
                    }
                } else {
                    throw std::invalid_argument(tr("The given file is of an unreadable nature, or another process has exclusive rights over it. Please select another file and try again.").toStdString());
                }
            } else if (file_path.isDir()) {
                //
                // We are working with a directory
                throw std::invalid_argument(tr("Unable to playback given object, since it is a directory and not a file.").toStdString());
            } else {
                //
                // The given object is of an unknown nature
                throw std::invalid_argument(tr("The given file is an object of unknown nature. Please select another file for playback and try again.").toStdString());
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    return;
}

/**
 * @brief GkMultimedia::recordAudioFile attempts to record from a given input device to a specified file on the end-user's
 * storage device of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The file to make the recording towards.
 * @param recording_device The audio device we are to record an audio stream from.
 * @param codec_id The codec used in encoding a given audio stream.
 * @param avg_bitrate The average bitrate for encoding with. This is unused for constant quantizer encoding.
 * @note OpenAL Recording Example <https://github.com/kcat/openal-soft/blob/master/examples/alrecord.c>,
 * Nikolaus Gradwohl <https://stackoverflow.com/a/3164561>,
 * Fabrice Bellard <https://ffmpeg.org/doxygen/trunk/encode__audio_8c_source.html>.
 */
void GkMultimedia::recordAudioFile(const QFileInfo &file_path, const ALCchar *recording_device, const AVCodecID &codec_id,
                                   const int64_t &avg_bitrate)
{
    try {
        if (!recording_device) {
            throw std::invalid_argument(tr("An invalid audio device has been specified; unable to proceed with recording! Please check your settings and try again.").toStdString());
        }

        checkOpenAlExtensions();
        if (file_path.exists()) {
            if (file_path.isFile()) {
                //
                // We are working with a file
                QMessageBox msgBox;
                msgBox.setParent(nullptr);
                msgBox.setWindowTitle(tr("Invalid destination!"));
                msgBox.setText(tr("You are attempting to record over a pre-existing file: \"%1\"\n\nDo you wish to continue?").arg(file_path.canonicalFilePath()));
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel | QMessageBox::Abort);
                msgBox.setDefaultButton(QMessageBox::Ok);
                msgBox.setIcon(QMessageBox::Icon::Warning);
                int ret = msgBox.exec();

                switch (ret) {
                    case QMessageBox::Ok:
                        QFile::remove(file_path.canonicalFilePath());
                        break;
                    case QMessageBox::Cancel:
                        return;
                    case QMessageBox::Abort:
                        return;
                    default:
                        return;
                }
            } else if (file_path.isDir()) {
                //
                // We are working with a directory
                throw std::invalid_argument(tr("Unable to record towards given object, since it is an already existing directory.").toStdString());
            } else {
                //
                // The given object is of an unknown nature
                throw std::invalid_argument(tr("The given file-path points to an existing object of unknown nature. Please select another destination and try again.").toStdString());
            }
        }

        GkDevice chosenInputDevice;
        for (const auto &input_audio_dev: gkSysInputAudioDevs) {
            const QString audio_dev_name = input_audio_dev.audio_dev_str;
            if (!audio_dev_name.isEmpty() && !audio_dev_name.isNull()) {
                const std::string conv_txt = gkStringFuncs->convTo8BitStr(audio_dev_name);
                if (conv_txt == recording_device) {
                    chosenInputDevice = input_audio_dev;
                    break;
                }
            } else {
                throw std::invalid_argument(tr("An invalid audio device name has been provided! Please check the configuration of your audio devices and try again.").toStdString());
            }
        }

        //
        // Gather any saved settings from the Google LevelDB database...
        const qint32 input_audio_dev_chosen_sample_rate = gkDb->read_misc_audio_settings(GkAudioCfg::AudioInputSampleRate).toInt();
        const qint32 input_audio_dev_chosen_number_channels = gkDb->read_misc_audio_settings(GkAudioCfg::AudioInputChannels).toInt();

        //
        // Calculate required variables and constants!
        ALuint bit_depth = openAlSelectBitDepth(chosenInputDevice.pref_audio_format);
        m_frameSize = input_audio_dev_chosen_number_channels * bit_depth / 8;
        const qint32 bytesPerSample = 2;
        const qint32 safetyFactor = 2; // In order to prevent buffer overruns! Must be larger than inputBuffer to avoid the circular buffer from overwriting itself between captures...
        const qint32 audioFrameSampleCountPerChannel = GK_AUDIO_FRAME_DURATION * input_audio_dev_chosen_sample_rate / 1000;
        const qint32 audioFrameSampleCountTotal = audioFrameSampleCountPerChannel * input_audio_dev_chosen_number_channels;
        ALCsizei bufSize = audioFrameSampleCountTotal * bytesPerSample * safetyFactor;

        //
        // Proceed with the capturing/recording of an audio stream!
        ALCdevice *rec_dev = alcCaptureOpenDevice(recording_device, chosenInputDevice.pref_sample_rate,
                                                  chosenInputDevice.pref_audio_format, bufSize);
        if (!rec_dev) {
            throw std::runtime_error(tr("ERROR: Unable to initialize input audio device, \"%1\"! Out of memory?")
            .arg(chosenInputDevice.audio_dev_str).toStdString());
        }

        qint32 ret = 0;
        const AVCodec *codec;
        AVCodecContext *c = nullptr;
        AVFrame *frame;
        AVPacket *pkt;
        QFile *file;

        //
        // Find the desired encoder!
        codec = avcodec_find_encoder(codec_id);
        if (!codec) {
            throw std::invalid_argument(tr("Audio codec could not be found!").toStdString());
        }

        c = avcodec_alloc_context3(codec);
        if (!c) {
            throw std::runtime_error(tr("Could not allocate audio codec context!").toStdString());
        }

        //
        // Configure sample format!
        switch (chosenInputDevice.pref_sample_rate) {
            case AL_FORMAT_MONO8:
                c->sample_fmt = AV_SAMPLE_FMT_U8;
                break;
            case AL_FORMAT_MONO16:
                c->sample_fmt = AV_SAMPLE_FMT_S16;
                break;
            case AL_FORMAT_STEREO8:
                c->sample_fmt = AV_SAMPLE_FMT_U8;
                break;
            case AL_FORMAT_STEREO16:
                c->sample_fmt = AV_SAMPLE_FMT_S16;
                break;
            default:
                c->sample_fmt = AV_SAMPLE_FMT_S16; // Default value to return!
                break;
        }

        //
        // Configure sample parameters!
        c->bit_rate = avg_bitrate * 1000;

        if (!ffmpegCheckSampleFormat(codec, c->sample_fmt)) {
            throw std::invalid_argument(tr("Chosen audio encoder does not support given sample format, \"%1\"!")
                                                .arg(QString::fromStdString(av_get_sample_fmt_name(c->sample_fmt))).toStdString());
        }

        //
        // Configure other audio paramete   rs supported by the chosen encoder!
        c->sample_rate = ffmpegSelectSampleRate(codec);
        c->channel_layout = ffmpegSelectChannelLayout(codec);
        c->channels = av_get_channel_layout_nb_channels(c->channel_layout);

        //
        // Initiate the encoding process
        if (avcodec_open2(c, codec, nullptr) < 0) {
            throw std::runtime_error(tr("Unable to open audio codec to begin encoding with.").toStdString());
        }

        file = new QFile(file_path.absoluteFilePath()); // NOTE: Cannot use canonical file path here, as if the file does not exist, it returns an empty string!
        if (!file->open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
            throw std::runtime_error(tr("Unable to open file, \"%1\"!").arg(file_path.fileName()).toStdString());
        }

        //
        // Packet for holding encoded output!
        pkt = av_packet_alloc();
        if (!pkt) {
            throw std::runtime_error(tr("Could not allocate the packet for encoding audio with.").toStdString());
        }

        //
        // Frame containing raw audio input
        frame = av_frame_alloc();
        if (!frame) {
            throw std::runtime_error(tr("Could not allocate audio frame.").toStdString());
        }

        frame->nb_samples = c->frame_size;
        frame->format = c->sample_fmt;
        frame->channel_layout = c->channel_layout;

        //
        // Allocate the data buffers!
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            throw std::runtime_error(tr("Unable to allocate audio data buffers for encoding!").toStdString());
        }

        alcCaptureStart(rec_dev);
        do {
            ALCint count = 0;
            alcGetIntegerv(rec_dev, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &count);
            if (count < 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(GK_AUDIO_VOL_PLAYBACK_REFRESH_INTERVAL));
                continue;
            }

            if (count > bufSize) {
                ALbyte *data = static_cast<ALbyte *>(calloc(m_frameSize, (ALuint) count));
                free(m_recordBuffer);
                m_recordBuffer = data;
                bufSize = count;
            }

            alcCaptureSamples(rec_dev, (ALCvoid *)m_recordBuffer, count);
            std::memcpy(frame->data[0], m_recordBuffer, count);

            //
            // Record and encode the buffer!
            ffmpegEncodeAudioPacket(c, frame, pkt, file);
        } while (alGetError() == AL_NO_ERROR && gkAudioState == GkAudioState::Recording);

        //
        // Flush the encoder
        ffmpegEncodeAudioPacket(c, nullptr, pkt, file);

        //
        // Close the file pointer
        file->close();

        //
        // Free up FFmpeg resources
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&c);

        //
        // Free up OpenAL sources
        alcCaptureStop(rec_dev);
        alcCaptureCloseDevice(rec_dev);

        //
        // Send the signal that recording has finished!
        emit recordingFinished();
        emit updateAudioState(GkAudioState::Stopped);

        return;
    } catch (const std::exception &e) {
        emit recordingFinished();
        emit updateAudioState(GkAudioState::Stopped); // Immediately halt any recording!
        QMessageBox::critical(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkMultimedia::mediaAction initiates a given multimedia action such as the playing of a multimedia file, or the
 * recording to a certain multimedia file, or even the stopping of all such processes, and does this via the actioning
 * of multi-threaded processes.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_state The media state we are supposed to action.
 * @param file_path The audio file we are supposed to be playing or recording towards, given the right multimedia
 * @param recording_device The audio device we are to record an audio stream from.
 * @param codec_id The codec used in encoding a given audio stream.
 * @param avg_bitrate The average bitrate for encoding with. This is unused for constant quantizer encoding.
 * action.
 */
void GkMultimedia::mediaAction(const GkAudioState &media_state, const QFileInfo &file_path, const ALCchar *recording_device,
                               const AVCodecID &codec_id, const int64_t &avg_bitrate)
{
    try {
        if (media_state == GkAudioState::Playing) {
            //
            // Playing
            playAudioFileThread = std::thread(&GkMultimedia::playAudioFile, this, file_path);
            playAudioFileThread.detach();

            return;
        } else if (media_state == GkAudioState::Recording) {
            //
            // Recording
            if (!recording_device) {
                throw std::invalid_argument(tr("An invalid audio device has been specified; unable to proceed with recording! Please check your settings and try again.").toStdString());
            }

            Q_ASSERT(codec_id != AV_CODEC_ID_NONE);
            recordAudioFileThread = std::thread(&GkMultimedia::recordAudioFile, this, file_path, recording_device, codec_id, avg_bitrate);
            recordAudioFileThread.detach();

            //
            // Send a message to the event log that recording has been initiated!
            gkEventLogger->publishEvent(tr("Recording of audio towards file, \"%1\", has been initiated!").arg(file_path.fileName()),
                                        GkSeverity::Info, "", false, true, true, false, false);

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
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
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
 * @brief GkMultimedia::changeVolume updates and sets the volume level to a new, given value, for a given OpenAL audio
 * source.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The new value to set the volume towards for a given OpenAL audio source.
 */
void GkMultimedia::changeVolume(const qint32 &value)
{
    if (gkAudioState == GkAudioState::Playing) {
        const qreal calc_val = static_cast<qreal>(value / 100);
        alCall(alSourcef, audioPlaybackSource, AL_GAIN, calc_val);
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
