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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/gk_audio_encoding.hpp"
#include <opus/opusenc.h>
#include <boost/exception/all.hpp>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <QtGui>
#include <QTimer>
#include <QByteArray>
#include <QMetaObject>
#include <QMessageBox>

#ifdef __cplusplus
extern "C"
{
#endif

#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
}
#endif

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

#define OGG_VORBIS_READ (1024)

GkAudioEncoding::GkAudioEncoding(QPointer<QAudioOutput> audioOutput, QPointer<QAudioInput> audioInput,
                                 const GkDevice &output_device, const GkDevice &input_device,
                                 QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QObject(parent)
{
    setParent(parent);
    gkEventLogger = std::move(eventLogger);

    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    gkInputDev = input_device;
    gkOutputDev = output_device;
    m_chosen_codec = CodecSupport::Unknown;

    QObject::connect(this, SIGNAL(pauseEncode()), this, SLOT(stopCaller()));
    QObject::connect(this, SIGNAL(error(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &)),
                     this, SLOT(handleError(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &)));
    QObject::connect(this, SIGNAL(encoded(QByteArray)), this, SLOT(processEncData(const QByteArray &)));
}

GkAudioEncoding::~GkAudioEncoding()
{
    if (m_initialized) {
        m_initialized = false;

        if (m_opus_encoder) {
            opus_encoder_destroy(m_opus_encoder);
        }
    }
}

/**
 * @brief GkAudioEncoding::codecEnumToStr converts an enum from, `GkAudioFramework::CodecSupport()`, to the given string
 * value.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec The given enum value.
 * @return The QString value for the given enum.
 */
QString GkAudioEncoding::codecEnumToStr(const CodecSupport &codec)
{
    switch (codec) {
        case PCM:
            return tr("PCM");
        case Loopback:
            return tr("Loopback");
        case OggVorbis:
            return tr("Ogg Vorbis");
        case Opus:
            return tr("Opus");
        case FLAC:
            return tr("FLAC");
        case Unsupported:
            return tr("Unsupported");
        default:
            break;
    }

    return tr("Unknown!");
}

/**
 * @brief GkAudioEncoding::stopEncode halts the encoding process from the upper-most level.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioEncoding::stopEncode()
{
    QTimer::singleShot(0, this, &GkAudioEncoding::stopEncode);
    return;
}

/**
 * @brief GkAudioEncoding::startCaller starts the process of encoding itself, whether that be done with Opus or another
 * codec such as Ogg Vorbis or even MP3.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path The location of where to save the audio file output (i.e. as a file).
 * @param audio_dev_info Information pertaining to the audio device in question to record from.
 * @param bitrate The bitrate to make the recording within.
 * @param codec_choice The choice of codec you wish to encode with, whether that be Opus, Ogg Vorbis, MP3, or possibly something else!
 * @param frame_size The frame size to record with.
 * @param application
 */
void GkAudioEncoding::startCaller(const fs::path &media_path, const Database::Settings::Audio::GkDevice &audio_dev_info,
                                  const qint32 &bitrate, const GkAudioFramework::CodecSupport &codec_choice,
                                  const qint32 &frame_size, const qint32 &application)
{
    try {
        if (m_initialized) {
            return;
        }

        if (codec_choice == CodecSupport::Opus) {
            //
            // NOTE: Opus only supports frame sizes between 2.5 - 60 milliseconds!
            //
            qint32 err;
            m_opus_encoder = opus_encoder_create(audio_dev_info.audio_device_info.preferredFormat().sampleRate(),
                                                 audio_dev_info.audio_device_info.preferredFormat().channelCount(),
                                                 application, &err);

            if (err < 0) {
                emit error(tr("Memory error encountered with Opus libraries. Do you have enough free memory?"), GkSeverity::Fatal);
                return;
            }

            m_initialized = true;
            m_chosen_codec = codec_choice;
            m_file_path = media_path;

            //
            // Set the desired bitrate, while other parameters can be set if needed as well. The Opus library is designed such to
            // have good defaults, so only set parameters you know that are truly needed. Doing otherwise is likely to result not in
            // worse audio quality, but actually better.
            //
            err = opus_encoder_ctl(m_opus_encoder, OPUS_SET_BITRATE(bitrate));
            if (err < 0) {
                emit error(tr("Failed to set the bitrate for the Opus codec."), GkSeverity::Warning);
                return;
            }
        } else {
            throw std::invalid_argument(tr("Invalid audio encoding codec specified! It is either not supported yet or an error was made.").toStdString());
        }

        m_channels = audio_dev_info.audio_device_info.preferredFormat().channelCount();
        m_frame_size = frame_size;
    } catch (const std::exception &e) {
        emit error(e.what(), GkSeverity::Fatal);
    }

    return;
}

/**
 * @brief GkAudioEncoding::stopCaller tells the encoding process to clear itself from memory after having been paused
 * from the upper-most level.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @see GkAudioEncoding::stopEncode().
 */
void GkAudioEncoding::stopCaller()
{
    deleteLater();
    return;
}

/**
 * @brief GkAudioEncoding::writeCaller performs the action of writing out the actual data from the encoding process itself.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data The raw data from the chosen codec, whether that be Opus or something like Ogg Vorbis or even MP3.
 */
void GkAudioEncoding::writeCaller(const QByteArray &data)
{
    m_buffer.append(data);
    if (m_chosen_codec == CodecSupport::Opus) {
        forever {
            QByteArray result = opusEncode();
            if (result.isEmpty()) {
                break;
            }

            emit encoded(result);
        };
    }

    return;
}

/**
 * @brief GkAudioEncoding::opusEncode perform an encoding of the given audio samples with Opus.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return Raw data from the Opus codec itself.
 */
QByteArray GkAudioEncoding::opusEncode()
{
    if (!m_initialized) {
        return QByteArray();
    }

    qint32 size = int(sizeof(float)) * m_channels * m_frame_size;
    if (m_buffer.size() < size) {
        return QByteArray();
    }

    qint32 nbBytes;
    QByteArray input = m_buffer.mid(0, size);
    m_buffer.remove(0, size);

    QByteArray output = QByteArray(AUDIO_OPUS_MAX_FRAME_SIZE, char(0));

    // Encode the frame...
    nbBytes = opus_encode_float(m_opus_encoder, reinterpret_cast<const float *>(input.constData()), m_frame_size, reinterpret_cast<uchar *>(output.data()), AUDIO_OPUS_MAX_FRAME_SIZE);

    if (nbBytes < 0) {
        emit error(tr("Opus encode failed: %0").arg(opus_strerror(nbBytes)), GkSeverity::Warning);
        return QByteArray();
    }

    output.resize(nbBytes);
    return output;
}

/**
 * @brief GkAudioEncoding::processEncData performs further processing, if any is needed, on the encoded audio information
 * as it comes out of the codecs for Opus, FLAC, PCM, etc.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data The raw, encoded data for the given codec.
 */
void GkAudioEncoding::processEncData(const QByteArray &data)
{
    gkEventLogger->publishEvent(tr("Recording of, \"%1\", with the %2 codec has begun!")
    .arg(QString::fromStdString(m_file_path.string())).arg(codecEnumToStr(m_chosen_codec)), GkSeverity::Info,
                                "", true, true, false, false);

    return;
}

/**
 * @brief GkAudioEncoding::handleError undertakes the handling of any errors in a clean and consistent manner. This should
 * be used where possible throughout this class if there's any possible errors to be had.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msg The error message itself.
 * @param severity The severity of the given error message as per above.
 */
void GkAudioEncoding::handleError(const QString &msg, const GkSeverity &severity)
{
    gkEventLogger->publishEvent(msg, severity, "", true, true, false, false);
    return;
}
