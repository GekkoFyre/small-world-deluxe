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
#include <QBuffer>
#include <QMessageBox>
#include <QTemporaryDir>

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
        case AUDIO_PLAYBACK_CODEC_RAW_IDX:
            return CodecSupport::RawData;
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
 * @param sample_buf A QByteArray containing the audio samples, ready for further processing by libsndfile.
 * @param channels The number of audio channels within the given audio samples.
 * @param sample_rate The sample rate of the given audio samples.
 * @param samples The raw audio samples that are to be converted to the PCM WAV format.
 * @param sample_size The size of the `samples` 16-bit integer array.
 * @return The file-path to where the converted, PCM WAV data is stored.
 * @note Thai Pangsakulyanont <https://gist.github.com/dtinth/1177001>.
 */
GkSndFile GkMultimedia::convertToPcm(std::shared_ptr<QByteArray> &sample_buf, const qint32 &channels,
                                     const qint32 &sample_rate, std::vector<qint16> samples, const size_t &sample_size)
{
    try {
        SF_INFO readInfo, writeInfo;
        readInfo.format = 0;

        std::shared_ptr<QByteArray> empty_buf = std::make_shared<QByteArray>();
        GkSndFile sndVirtIn = openVirtualSndFile(SFM_READ, channels, sample_rate, SF_FORMAT_RAW | SF_FORMAT_PCM_S8, QIODevice::ReadWrite, sample_buf);
        GkSndFile sndVirtOut = openVirtualSndFile(SFM_WRITE, channels, sample_rate, SF_FORMAT_WAV | SF_FORMAT_FLOAT, QIODevice::ReadOnly, empty_buf);

        while (true) {
            qint64 items = sf_readf_short(sndVirtIn.sndFile, samples.data(), sample_size);
            if (items > 0) {
                sf_writef_short(sndVirtOut.sndFile, samples.data(), items);
                continue;
            } else {
                break;
            }
        }

        //
        // Clean up any items that are now disused!
        sndVirtOut.buf->close();
        sndVirtIn.buf->close();
        sf_close(sndVirtOut.sndFile);
        sf_close(sndVirtIn.sndFile);

        return sndVirtOut;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return GkSndFile{};
}

/**
 * @brief GkMultimedia::convertToPcm converts raw audio samples to the PCM WAV audio format, via the libsndfile
 * multimedia set of libraries. This function is overloaded and in this particular case, samples that are of the
 * 32-bit integer type are to be converted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sample_buf A QByteArray containing the audio samples, ready for further processing by libsndfile.
 * @param channels The number of audio channels within the given audio samples.
 * @param sample_rate The sample rate of the given audio samples.
 * @param samples The raw audio samples that are to be converted to the PCM WAV format.
 * @param sample_size The size of the `samples` 32-bit integer array.
 * @return The file-path to where the converted, PCM WAV data is stored.
 * @note Thai Pangsakulyanont <https://gist.github.com/dtinth/1177001>.
 */
GkSndFile GkMultimedia::convertToPcm(std::shared_ptr<QByteArray> &sample_buf, const qint32 &channels,
                                     const qint32 &sample_rate, std::vector<qint32> samples, const size_t &sample_size)
{
    try {
        SF_INFO readInfo, writeInfo;
        readInfo.format = 0;

        std::shared_ptr<QByteArray> empty_buf = std::make_shared<QByteArray>();
        GkSndFile sndVirtIn = openVirtualSndFile(SFM_READ, channels, sample_rate, SF_FORMAT_RAW | SF_FORMAT_PCM_16, QIODevice::ReadWrite, sample_buf);
        GkSndFile sndVirtOut = openVirtualSndFile(SFM_WRITE, channels, sample_rate, SF_FORMAT_WAV | SF_FORMAT_FLOAT, QIODevice::ReadOnly, empty_buf);

        while (true) {
            qint64 items = sf_readf_int(sndVirtIn.sndFile, samples.data(), sample_size);
            if (items > 0) {
                sf_writef_int(sndVirtOut.sndFile, samples.data(), items);
                continue;
            } else {
                break;
            }
        }

        //
        // Clean up any items that are now disused!
        sndVirtOut.buf->close();
        sndVirtIn.buf->close();
        sf_close(sndVirtOut.sndFile);
        sf_close(sndVirtIn.sndFile);

        return sndVirtOut;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return GkSndFile{};
}

/**
 * @brief GkMultimedia::convertToPcm converts raw audio samples to the PCM WAV audio format, via the libsndfile
 * multimedia set of libraries. This function is overloaded and in this particular case, samples that are of the
 * float-type are to be converted.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sample_buf A QByteArray containing the audio samples, ready for further processing by libsndfile.
 * @param channels The number of audio channels within the given audio samples.
 * @param sample_rate The sample rate of the given audio samples.
 * @param samples The raw audio samples that are to be converted to the PCM WAV format.
 * @param sample_size The size of the `samples` floating-point array.
 * @return The file-path to where the converted, PCM WAV data is stored.
 * @note Thai Pangsakulyanont <https://gist.github.com/dtinth/1177001>.
 */
GkSndFile GkMultimedia::convertToPcm(std::shared_ptr<QByteArray> &sample_buf, const qint32 &channels,
                                     const qint32 &sample_rate, std::vector<float> samples, const size_t &sample_size)
{
    try {
        SF_INFO readInfo, writeInfo;
        readInfo.format = 0;

        std::shared_ptr<QByteArray> empty_buf = std::make_shared<QByteArray>();
        GkSndFile sndVirtIn = openVirtualSndFile(SFM_READ, channels, sample_rate, SF_FORMAT_RAW | SF_FORMAT_FLOAT, QIODevice::ReadWrite, sample_buf);
        GkSndFile sndVirtOut = openVirtualSndFile(SFM_WRITE, channels, sample_rate, SF_FORMAT_WAV | SF_FORMAT_FLOAT, QIODevice::ReadOnly, empty_buf);

        while (true) {
            qint64 items = sf_readf_float(sndVirtIn.sndFile, samples.data(), sample_size);
            if (items > 0) {
                sf_writef_float(sndVirtOut.sndFile, samples.data(), items);
                continue;
            } else {
                break;
            }
        }

        //
        // Clean up any items that are now disused!
        sndVirtOut.buf->close();
        sndVirtIn.buf->close();
        sf_close(sndVirtOut.sndFile);
        sf_close(sndVirtIn.sndFile);

        return sndVirtOut;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return GkSndFile{};
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
void GkMultimedia::encodeOpus(FILE *f, float *pcm_data, const ALuint &sample_rate, const size_t &samples_size,
                              const qint32 &num_channels)
{
    try {
        std::vector<unsigned char> output_buf;
        qint32 error;

        OpusEncoder *encoder = opus_encoder_create(static_cast<opus_int32>(sample_rate), 1, OPUS_APPLICATION_VOIP, &error);
        if (error != OPUS_OK) {
            throw std::runtime_error(tr("Failed to create Opus codec pointer! Out of memory?").toStdString());
        }

        qint32 bytes_written = opus_encode_float(encoder, pcm_data, static_cast<qint32>(samples_size), output_buf.data(), 4000);
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
 * @brief GkMultimedia::qbufGetFileLen
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param user_data
 * @return
 */
sf_count_t GkMultimedia::qbufGetFileLen(void *user_data)
{
    QBuffer *buf = (QBuffer *)user_data;
    return buf->size();
}

/**
 * @brief GkMultimedia::qbufSeek
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param offset
 * @param whence
 * @param user_data
 * @return
 */
sf_count_t GkMultimedia::qbufSeek(sf_count_t offset, qint32 whence, void *user_data)
{
    QBuffer *buf = (QBuffer *)user_data;
    switch (whence) {
        case SEEK_SET:
            buf->seek(offset);
            break;
        case SEEK_CUR:
            buf->seek(buf->pos() + offset);
            break;
        case SEEK_END:
            buf->seek(buf->size() + offset);
            break;
        default:
            break;
    }

    return buf->pos();
}

/**
 * @brief GkMultimedia::qbufRead
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param ptr
 * @param count
 * @param user_data
 * @return
 */
sf_count_t GkMultimedia::qbufRead(void *ptr, sf_count_t count, void *user_data)
{
    QBuffer *buf = (QBuffer *)user_data;
    return buf->read((char *)ptr, count);
}

/**
 * @brief GkMultimedia::qbufWrite
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param ptr
 * @param count
 * @param user_data
 * @return
 */
sf_count_t GkMultimedia::qbufWrite(const void *ptr, sf_count_t count, void *user_data)
{
    QBuffer *buf = (QBuffer *)user_data;
    return buf->write((const char *)ptr, count);
}

/**
 * @brief GkMultimedia::qbufTell
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param user_data
 * @return
 */
sf_count_t GkMultimedia::qbufTell(void *user_data)
{
    QBuffer *buf = (QBuffer *) user_data;
    return buf->pos();
}

/**
 * @brief GkMultimedia::openVirtualSndFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param mode
 * @param channels
 * @param sample_rate
 * @param format
 * @param buf_mode The mode to which we open QBuffer in, such as `QIODevice::ReadWrite`, for example.
 * @param sample_buf A QByteArray containing the audio samples, ready for further processing by libsndfile.
 * @return
 * @note Soheil Armin <https://stackoverflow.com/a/59233795/4293625>.
 */
GkSndFile GkMultimedia::openVirtualSndFile(const qint32 &mode, const qint32 &channels, const qint32 &sample_rate,
                                           const qint32 &format, const QIODevice::OpenModeFlag &buf_mode,
                                           std::shared_ptr<QByteArray> &sample_buf)
{
    try {
        GkSndFile sndFileStruct;
        sndFileStruct.vioPtr.get_filelen = qbufGetFileLen;
        sndFileStruct.vioPtr.seek = qbufSeek;
        sndFileStruct.vioPtr.read = qbufRead;
        sndFileStruct.vioPtr.write = qbufWrite;
        sndFileStruct.vioPtr.tell = qbufTell;

        sndFileStruct.sfInfo.channels = channels;
        sndFileStruct.sfInfo.samplerate = sample_rate;
        sndFileStruct.sfInfo.format = format;
        if (!sf_format_check(&sndFileStruct.sfInfo)) {
            throw std::invalid_argument(tr("Incorrect parameters have been passed to libsndfile encoder!").toStdString());
        }

        if (!sample_buf->isEmpty()) {
            sndFileStruct.buf = std::make_shared<QBuffer>(sample_buf.get());
        } else {
            sndFileStruct.buf = std::make_shared<QBuffer>();
        }

        sndFileStruct.buf->open(buf_mode);
        sndFileStruct.sndFile = sf_open_virtual(&sndFileStruct.vioPtr, mode, &sndFileStruct.sfInfo, (void *)(sndFileStruct.buf.get()));
        if (!sndFileStruct.sndFile) {
            throw std::runtime_error(tr("Unable to open Virtual I/O interface with regards to libsndfile!").toStdString());
        }

        return sndFileStruct;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return GkSndFile{};
}

/**
 * @brief GkMultimedia::outputFileContents reads out the contents of a given file as raw, binary data, into a buffer and
 * therefore into memory, ready for further processing down the line.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The file that is to have the raw data read from and outputted into memory.
 * @return The raw, binary data of a given file read into a buffer and therefore, into memory.
 */
char *GkMultimedia::outputFileContents(const QFileInfo &file_path)
{
    std::string conv_file_path = gkStringFuncs->convTo8BitStr(file_path.absoluteFilePath());
    std::ifstream binFile(conv_file_path, std::ifstream::binary);
    if (binFile) {
        //
        // Obtain the length of the given file!
        binFile.seekg(0, std::ifstream::end);
        size_t length = static_cast<size_t>(binFile.tellg());
        binFile.seekg(0, std::ifstream::beg);

        //
        // Read the entire contents of the file to a buffer, at once!
        char *buffer = new char[length];
        binFile.read(buffer, length);
        binFile.close();

        return buffer;
    }

    return nullptr;
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

        FILE *f;
        size_t samples_size = chosenInputDevice.pref_sample_rate / 25;

        //
        // Convert the absolute file-path to an std::string!
        const std::string absFilePath = gkStringFuncs->convTo8BitStr(file_path.absoluteFilePath());

        f = fopen(absFilePath.c_str(), "wb"); // NOTE: Cannot use canonical file-path here, as if the file does not exist, it returns an empty string!
        if (!f) {
            throw std::runtime_error(tr("Unable to open file, \"%1\"!").arg(file_path.fileName()).toStdString());
        }

        while (alGetError() == AL_NO_ERROR && gkAudioState == GkAudioState::Recording) {
            QTemporaryDir dir;
            std::vector<float> output_raw_pcm;
            if (dir.isValid()) {
                //
                // We have to create a PCM WAV file for these particular codecs!
                if (codec_id == CodecSupport::Opus) {
                    //
                    // Convert the raw audio data to a PCM WAV file through the creation of temporary files on the
                    // end-users local storage (i.e. HDD/SSD). These temporary files will be created and deleted as
                    // needed.
                    //
                    std::shared_ptr<QByteArray> input_raw_audio = std::make_shared<QByteArray>(reinterpret_cast<const char*>(m_recordBuffer->data()), m_recordBuffer->size());
                    convertToPcm(input_raw_audio, input_audio_dev_chosen_number_channels,
                                 input_audio_dev_chosen_sample_rate, output_raw_pcm, m_recordBuffer->size());
                }

                //
                // Proceed with encoding of audio files to the specified codec, whether that be AAC, MP3, etc.!
                if (codec_id == CodecSupport::Opus) {
                    //
                    // Setup the audio buffers
                    if (output_raw_pcm.empty()) {
                        throw std::invalid_argument(tr("Unable to process PCM data in preparation for conversion towards, \"%1\"!.")
                                                            .arg(codecEnumToStr(codec_id)).toStdString());
                    }

                    encodeOpus(f, output_raw_pcm.data(), chosenInputDevice.pref_sample_rate, samples_size, chosenInputDevice.sel_channels);
                    output_raw_pcm.clear();
                } else {
                    break;
                }
            }

            output_raw_pcm.clear();
            continue;
        };

        //
        // Close the file pointer in order to begin cleaning up!
        fclose(f);

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
 * @brief GkMultimedia::codecEnumToStr converts an enum from, `GkAudioFramework::CodecSupport()`, to the given string
 * value.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec The given enum value.
 * @return The QString value for the given enum.
 */
QString GkMultimedia::codecEnumToStr(const CodecSupport &codec)
{
    switch (codec) {
        case CodecSupport::PCM:
            return QStringLiteral("PCM");
        case CodecSupport::AAC:
            return QStringLiteral("AAC");
        case CodecSupport::Loopback:
            return tr("Loopback");
        case CodecSupport::OggVorbis:
            return QStringLiteral("Ogg Vorbis");
        case CodecSupport::Opus:
            return QStringLiteral("Opus");
        case CodecSupport::FLAC:
            return QStringLiteral("FLAC");
        case CodecSupport::Codec2:
            return QStringLiteral("Codec2");
        case CodecSupport::RawData:
            return tr("Raw Data");
        case CodecSupport::Unsupported:
            return tr("Unsupported");
        default:
            break;
    }

    return tr("Unknown!");
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
