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
#include <opus/opus.h>
#include <cstdio>
#include <future>
#include <cstring>
#include <cstdlib>
#include <utility>
#include <exception>
#include <QDir>
#include <QMessageBox>

#ifdef __cplusplus
extern "C"
{
#endif

#include <sndfile.h>

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

    //
    // Initialize variables within audio devices
    for (const auto &input_audio_dev: gkSysInputAudioDevs) {
        if (getInputAudioDevice().audio_dev_str == input_audio_dev.audio_dev_str) {
            m_recordBuffer = input_audio_dev.alDeviceRecBuf;
        }
    }

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
 * @brief GkMultimedia::convAudioCodecIdxToEnum converts a given, QComboBox index to its equivalent enumerator as found
 * within the, `defines.hpp`, source file. Only codecs officially supported and tested by Small World Deluxe are
 * convertable.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec_id_str The QComboBox index to be converted towards its equivalent, audio codec-defined enumerator.
 * @return The equivalent enumerator, as found within the `defines.hpp` source file.
 */
CodecSupport GkMultimedia::convAudioCodecIdxToEnum(const qint32 &codec_id_str)
{
    switch (codec_id_str) {
        case AUDIO_PLAYBACK_CODEC_CODEC2_IDX:
            return CodecSupport::Codec2;
        case AUDIO_PLAYBACK_CODEC_VORBIS_IDX:
            return CodecSupport::OggVorbis;
        case AUDIO_PLAYBACK_CODEC_AAC_IDX:
            return CodecSupport::AAC;
        case AUDIO_PLAYBACK_CODEC_OPUS_IDX:
            return CodecSupport::Opus;
        case AUDIO_PLAYBACK_CODEC_FLAC_IDX:
            return CodecSupport::FLAC;
        case AUDIO_PLAYBACK_CODEC_PCM_IDX:
            return CodecSupport::PCM;
        case AUDIO_PLAYBACK_CODEC_LOOPBACK_IDX:
            return CodecSupport::Loopback;
        default:
            return CodecSupport::Unsupported;
    }

    return CodecSupport::Unknown;
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
            std::free(membuf);
            sf_close(sndfile);
            throw std::runtime_error(tr("Failed to read samples in %1 (%2 frames)!")
                                             .arg(file_path.canonicalFilePath(), QString::number(num_frames)).toStdString());
        }

        num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

        //
        // Buffer the audio data into a new buffer object, then free the data and close the given file!
        alCall(alGenBuffers, GK_AUDIO_STREAM_NUM_BUFS, &buffer);
        alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

        std::free(membuf);
        sf_close(sndfile);

        return buffer;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return ALuint();
}

/**
 * @brief GkMultimedia::checkForFileToBeginRecording initiates the first process/function in beginning a sequence to
 * record towards a given file, in that it asks the end-user what to do in the event of an already existing file,
 * provided a path towards an existing file has been provided. Otherwise, the function simply remains dormant. A
 * QMessageBox is presented to the end-user in the event of an existing file being provided as the given file-path,
 * asking what action should precede the initiation of a recording session.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical, or in this use case, the absolute path to the given audio file to be played.
 */
void GkMultimedia::checkForFileToBeginRecording(const QFileInfo &file_path)
{
    try {
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
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
}

/**
 * @brief GkMultimedia::convAudioCodecToFileExtStr converts a given codec enumerator/identifier to the given string, but
 * more importantly, it does this in a way that's suitable for use as a file extension.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec_id The type of audio codec we are dealing with.
 * @return The codec enumerator as a string, but suited for use as a file extension.
 */
QString GkMultimedia::convAudioCodecToFileExtStr(GkAudioFramework::CodecSupport codec_id)
{
    switch (codec_id) {
        case GkAudioFramework::CodecSupport::Codec2:
            return Filesystem::audio_format_codec2;
        case GkAudioFramework::CodecSupport::PCM:
            return Filesystem::audio_format_pcm_wav;
        case GkAudioFramework::CodecSupport::Loopback:
            return Filesystem::audio_format_loopback;
        case GkAudioFramework::CodecSupport::OggVorbis:
            return Filesystem::audio_format_ogg_vorbis;
        case GkAudioFramework::CodecSupport::Opus:
            return Filesystem::audio_format_ogg_opus;
        case GkAudioFramework::CodecSupport::FLAC:
            return Filesystem::audio_format_flac;
        case GkAudioFramework::CodecSupport::AAC:
            return Filesystem::audio_format_aac;
        case GkAudioFramework::CodecSupport::RawData:
            return Filesystem::audio_format_raw_data;
        case GkAudioFramework::CodecSupport::Unsupported:
            return Filesystem::audio_format_unsupported;
        case GkAudioFramework::CodecSupport::Unknown:
            return Filesystem::audio_format_unknown;
        default:
            return Filesystem::audio_format_default;
    }

    return QString();
}

/**
 * @brief GkMultimedia::convertToPcm converts raw audio samples to the PCM WAV audio format, via the libsndfile
 * multimedia set of libraries. This function is overloaded and in this particular case, samples that are of the
 * 16-bit integer type are to be converted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical, or in this use case, the absolute path to the given audio file to be played.
 * @param samples The raw audio samples that are to be converted to the PCM WAV format.
 * @param sample_size The size of the `samples` 16-bit integer array.
 * @note Thai Pangsakulyanont <https://gist.github.com/dtinth/1177001>.
 */
void GkMultimedia::convertToPcm(const QFileInfo &file_path, qint16 *samples, const qint32 &sample_size)
{
    try {
        SF_INFO readInfo, writeInfo;
        readInfo.format = 0;

        const std::string conv_in_file_path = gkStringFuncs->convTo8BitStr(file_path.absoluteFilePath()); // Ensure that the file-path is converted fairly and truly to the correct std::string!
        SNDFILE *in = sf_open(conv_in_file_path.c_str(), SFM_READ, &readInfo);
        if (!in) {
            throw std::runtime_error(tr("Cannot open file for reading: \"%1\"").arg(file_path.fileName()).toStdString());
        }

        writeInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        writeInfo.samplerate = readInfo.samplerate;
        writeInfo.channels   = readInfo.channels;

        QString out_file_path = QDir(file_path.absolutePath()).filePath(file_path.baseName() + convAudioCodecToFileExtStr(CodecSupport::PCM));
        const std::string conv_out_file_path = gkStringFuncs->convTo8BitStr(out_file_path); // Ensure that the file-path is converted fairly and truly to the correct std::string!
        SNDFILE *out = sf_open(conv_out_file_path.c_str(), SFM_WRITE, &writeInfo);
        if (!out) {
            throw std::runtime_error(tr("Cannot open file for writing: \"%1\"").arg(QFileInfo(out_file_path).fileName()).toStdString());
        }

        while (true) {
            qint32 items = sf_read_short(in, samples, sample_size);
            if (items > 0) {
                sf_write_short(out, samples, items);
            } else {
                break;
            }
        }

        //
        // Clean up any items that are now disused!
        sf_close(in);
        sf_close(out);

        //
        // Now we wish to delete the original file which contained the **raw data**, and is only a temporary file anyway!
        if (file_path.exists() && file_path.isFile()) {
            if (!QFile(file_path.absoluteFilePath()).remove()) {
                throw std::runtime_error(tr("Issues were encountered with attempting to remove temporary file, \"%1\"").arg(file_path.fileName()).toStdString());
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
}

/**
 * @brief GkMultimedia::convertToPcm converts raw audio samples to the PCM WAV audio format, via the libsndfile
 * multimedia set of libraries. This function is overloaded and in this particular case, samples that are of the
 * 32-bit integer type are to be converted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical, or in this use case, the absolute path to the given audio file to be played.
 * @param samples The raw audio samples that are to be converted to the PCM WAV format.
 * @param sample_size The size of the `samples` 32-bit integer array.
 * @note Thai Pangsakulyanont <https://gist.github.com/dtinth/1177001>.
 */
void GkMultimedia::convertToPcm(const QFileInfo &file_path, qint32 *samples, const qint32 &sample_size)
{
    try {
        SF_INFO readInfo, writeInfo;
        readInfo.format = 0;

        const std::string conv_in_file_path = gkStringFuncs->convTo8BitStr(file_path.absoluteFilePath()); // Ensure that the file-path is converted fairly and truly to the correct std::string!
        SNDFILE *in = sf_open(conv_in_file_path.c_str(), SFM_READ, &readInfo);
        if (!in) {
            throw std::runtime_error(tr("Cannot open file for reading: \"%1\"").arg(file_path.fileName()).toStdString());
        }

        writeInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        writeInfo.samplerate = readInfo.samplerate;
        writeInfo.channels   = readInfo.channels;

        QString out_file_path = QDir(file_path.absolutePath()).filePath(file_path.baseName() + convAudioCodecToFileExtStr(CodecSupport::PCM));
        const std::string conv_out_file_path = gkStringFuncs->convTo8BitStr(out_file_path); // Ensure that the file-path is converted fairly and truly to the correct std::string!
        SNDFILE *out = sf_open(conv_out_file_path.c_str(), SFM_WRITE, &writeInfo);
        if (!out) {
            throw std::runtime_error(tr("Cannot open file for writing: \"%1\"").arg(QFileInfo(out_file_path).fileName()).toStdString());
        }

        while (true) {
            qint64 items = sf_read_int(in, samples, sample_size);
            if (items > 0) {
                sf_write_int(out, samples, items);
            } else {
                break;
            }
        }

        //
        // Clean up any items that are now disused!
        sf_close(in);
        sf_close(out);

        //
        // Now we wish to delete the original file which contained the **raw data**, and is only a temporary file anyway!
        if (file_path.exists() && file_path.isFile()) {
            if (!QFile(file_path.absoluteFilePath()).remove()) {
                throw std::runtime_error(tr("Issues were encountered with attempting to remove temporary file, \"%1\"").arg(file_path.fileName()).toStdString());
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
}

/**
 * @brief GkMultimedia::convertToPcm converts raw audio samples to the PCM WAV audio format, via the libsndfile
 * multimedia set of libraries. This function is overloaded and in this particular case, samples that are of the
 * float-type are to be converted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical, or in this use case, the absolute path to the given audio file to be played.
 * @param samples The raw audio samples that are to be converted to the PCM WAV format.
 * @param sample_size The size of the `samples` floating-point array.
 * @note Thai Pangsakulyanont <https://gist.github.com/dtinth/1177001>.
 */
void GkMultimedia::convertToPcm(const QFileInfo &file_path, float *samples, const qint32 &sample_size)
{
    try {
        SF_INFO readInfo, writeInfo;
        readInfo.format = 0;

        const std::string conv_in_file_path = gkStringFuncs->convTo8BitStr(file_path.absoluteFilePath()); // Ensure that the file-path is converted fairly and truly to the correct std::string!
        SNDFILE *in = sf_open(conv_in_file_path.c_str(), SFM_READ, &readInfo);
        if (!in) {
            throw std::runtime_error(tr("Cannot open file for reading: \"%1\"").arg(file_path.fileName()).toStdString());
        }

        writeInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        writeInfo.samplerate = readInfo.samplerate;
        writeInfo.channels   = readInfo.channels;

        QString out_file_path = QDir(file_path.absolutePath()).filePath(file_path.baseName() + convAudioCodecToFileExtStr(CodecSupport::PCM));
        const std::string conv_out_file_path = gkStringFuncs->convTo8BitStr(out_file_path); // Ensure that the file-path is converted fairly and truly to the correct std::string!
        SNDFILE *out = sf_open(conv_out_file_path.c_str(), SFM_WRITE, &writeInfo);
        if (!out) {
            throw std::runtime_error(tr("Cannot open file for writing: \"%1\"").arg(QFileInfo(out_file_path).fileName()).toStdString());
        }

        while (true) {
            qint64 items = sf_read_float(in, samples, sample_size);
            if (items > 0) {
                sf_write_float(out, samples, items);
            } else {
                break;
            }
        }

        //
        // Clean up any items that are now disused!
        sf_close(in);
        sf_close(out);

        //
        // Now we wish to delete the original file which contained the **raw data**, and is only a temporary file anyway!
        if (file_path.exists() && file_path.isFile()) {
            if (!QFile(file_path.absoluteFilePath()).remove()) {
                throw std::runtime_error(tr("Issues were encountered with attempting to remove temporary file, \"%1\"").arg(file_path.fileName()).toStdString());
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
}

/**
 * @brief GkMultimedia::encodeOpus encodes to the Opus format. A container must be provided separately, such as Ogg
 * which is commonly used alongside Opus. Only parses data which is in the PCM Format already, so must use a function
 * such as **GkMultimedia::convertToPcm()** to do the pre-encoding firstly.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param f The system file pointer.
 * @param pcm_data The raw, PCM data to be encoded by the Opus codec.
 * @parm sample_rate The given sample rate to encode for, such as 44,100 kHz.
 * @parm samples_size The number of the audio samples within the audio buffer used for encoding.
 * @parm num_channels The number of available audio channels to encode for, whether it be '1' for mono or '2' for
 * stereo, as an example.
 * @note Matin Kh <https://stackoverflow.com/q/56368106>.
 * @see GkMultimedia::convertToPcm().
 */
void GkMultimedia::encodeOpus(FILE *f, float *pcm_data, const qint32 &sample_rate, const qint32 &samples_size,
                              const qint32 &num_channels)
{
    try {
        std::vector<unsigned char> output_buf;
        qint32 error;

        OpusEncoder *encoder = opus_encoder_create(sample_rate, 1, OPUS_APPLICATION_VOIP, &error);
        if (error != OPUS_OK) {
            throw std::runtime_error(tr("Failed to create Opus codec pointer! Out of memory?").toStdString());
        }

        qint32 bytes_written = opus_encode_float(encoder, pcm_data, samples_size, output_buf.data(), 4000);
        if (bytes_written < 0) {
            if (bytes_written == OPUS_BAD_ARG) {
                throw std::invalid_argument(tr("Invalid arguments given for encoding with the Opus audio format.").toStdString());
            } else {
                throw std::invalid_argument(tr("Failed to initiate encoding with the Opus audio format.").toStdString());
            }
        }

        //
        // Output the encoded data to the given file pointer!
        fwrite(output_buf.data(), bytes_written, 1, f);

        //
        // Clean up the now, unneeded, PCM data!
        std::free(pcm_data);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
}

/**
 * @brief GkMultimedia::addOggContainer
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkMultimedia::addOggContainer()
{
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
 * @param file_path The canonical, or in this use case, the absolute path to the given audio file to be played.
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
 * @see GkMultimedia::convertToPcm(), GkMultimedia::encodeOpus().
 */
void GkMultimedia::recordAudioFile(const QFileInfo &file_path, const ALCchar *recording_device,
                                   const GkAudioFramework::CodecSupport &codec_id, const int64_t &avg_bitrate)
{
    try {
        if (!recording_device || !m_recordBuffer) {
            throw std::invalid_argument(tr("An invalid audio device has been specified; unable to proceed with recording! Please check your settings and try again.").toStdString());
        }

        //
        // Check that the requisite OpenAL extensions are available and ready-to-use!
        checkOpenAlExtensions();

        //
        // Ask the user what action should precede the initiation of a recording session (i.e., the code below) in the
        // event that the given file-path is one to an already existing file!
        //
        checkForFileToBeginRecording(file_path);

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

        FILE *f;
        size_t samples_size = chosenInputDevice.pref_sample_rate / 25;

        //
        // Convert the absolute file path to an std::string!
        const std::string absFilePath = gkStringFuncs->convTo8BitStr(file_path.absoluteFilePath());

        f = fopen(absFilePath.c_str(), "wb"); // NOTE: Cannot use canonical file path here, as if the file does not exist, it returns an empty string!
        if (!f) {
            throw std::runtime_error(tr("Unable to open file, \"%1\"!").arg(file_path.fileName()).toStdString());
        }

        alcCaptureStart(rec_dev);
        while (alGetError() == AL_NO_ERROR && gkAudioState == GkAudioState::Recording) {
            ALCsizei count = 0;
            alcGetIntegerv(rec_dev, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &count);
            if (count < 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(GK_AUDIO_VOL_PLAYBACK_REFRESH_INTERVAL));
                continue;
            }

            alcCaptureSamples(rec_dev, reinterpret_cast<ALCvoid *>(m_recordBuffer->data()), count);

            //
            // Setup the audio buffers
            const size_t pcm_data_size = samples_size * chosenInputDevice.sel_channels * sizeof(float);
            float *pcm_data = static_cast<float *>(std::malloc(pcm_data_size));
            for (size_t i = 0; i < pcm_data_size; ++i) {
                pcm_data[i] = static_cast<float>(m_recordBuffer->at(i));
            }

            encodeOpus(f, pcm_data, chosenInputDevice.pref_sample_rate, samples_size, chosenInputDevice.sel_channels);
        };

        //
        // Close the file pointer in order to begin cleaning up!
        fclose(f);

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
        gkStringFuncs->print_exception(e);
    }

    return;
}

/**
 * @brief GkMultimedia::mediaAction initiates a given multimedia action such as the playing of a multimedia file, or the
 * recording to a certain multimedia file, or even the stopping of all such processes, and does this via the actioning
 * of multi-threaded processes.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_state The media state we are supposed to action.
 * @param codec_id The codec used in encoding a given audio stream.
 * @param file_path The audio file we are supposed to be playing or recording towards, given the right multimedia
 * @param recording_device The audio device we are to record an audio stream from.
 * @param avg_bitrate The average bitrate for encoding with. This is unused for constant quantizer encoding.
 * action.
 */
void GkMultimedia::mediaAction(const GkAudioState &media_state, const QFileInfo &file_path, const ALCchar *recording_device,
                               const CodecSupport &codec_id, const int64_t &avg_bitrate)
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
