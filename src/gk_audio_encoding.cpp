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

#include "src/gk_audio_encoding.hpp"
#include <vector>
#include <chrono>
#include <cstring>
#include <utility>
#include <exception>
#include <QDir>
#include <QtGui>
#include <QDateTime>
#include <QMessageBox>

#ifdef __cplusplus
extern "C"
{
#endif

#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

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

/**
 * @brief GkAudioEncoding::GkAudioEncoding
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param audioOutput
 * @param audioInput
 * @param audioInputBuf
 * @param stringFuncs
 * @param eventLogger
 * @param parent
 * @note Audio Input Example <https://doc.qt.io/qt-5/qtmultimedia-multimedia-audioinput-example.html>.
 */
GkAudioEncoding::GkAudioEncoding(QPointer<GekkoFyre::GkLevelDb> database, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                 QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QObject(parent)
{
    setParent(parent);
    gkDb = std::move(database);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    //
    // Initialize variables
    m_initialized = false;
    m_recActive = GkAudioRecordStatus::Defunct;

    QObject::connect(this, SIGNAL(pauseEncode()), this, SLOT(stopCaller()));
    QObject::connect(this, SIGNAL(error(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &)),
                     this, SLOT(handleError(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &)));
    QObject::connect(this, SIGNAL(recStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &)),
                     this, SLOT(setRecStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &)));

    return;
}

GkAudioEncoding::~GkAudioEncoding()
{
    emit recStatus(GkAudioRecordStatus::Defunct);
    if (m_initialized) {
        m_initialized = false;

        if (m_opusEncoder) {
            ope_encoder_destroy(m_opusEncoder);
        }

        if (m_opusComments) {
            ope_comments_destroy(m_opusComments);
        }

        if (m_out_file.isOpen()) {
            m_out_file.commit();
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
        case CodecSupport::PCM:
            return tr("PCM");
        case CodecSupport::Loopback:
            return tr("Loopback");
        case CodecSupport::OggVorbis:
            return tr("Ogg Vorbis");
        case CodecSupport::Opus:
            return tr("Opus");
        case CodecSupport::FLAC:
            return tr("FLAC");
        case CodecSupport::Unsupported:
            return tr("Unsupported");
        default:
            break;
    }

    return tr("Unknown!");
}

/**
 * @brief GkAudioEncoding::getRecStatus gets the status of whether recording is active or not throughout Small World
 * Deluxe. This also applies to whether any encoding is being performed, since both have an effect on one another.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
GkAudioRecordStatus GkAudioEncoding::getRecStatus() const
{
    return m_recActive;
}

GkAudioFramework::CodecSupport GkAudioEncoding::getCodec()
{
    return m_codecUsed;
}

/**
 * @brief GkAudioEncoding::stopEncode halts the encoding process from the upper-most level.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioEncoding::stopEncode()
{
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
 * @brief GkAudioEncoding::handleError undertakes the handling of any errors in a clean and consistent manner. This should
 * be used where possible throughout this class if there's any possible errors to be had.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param msg The error message itself.
 * @param severity The severity of the given error message as per above.
 */
void GkAudioEncoding::handleError(const QString &msg, const GkSeverity &severity)
{
    gkEventLogger->publishEvent(msg, severity, "", true, true, false, false, true);
    return;
}

/**
 * @brief GkAudioEncoding::setRecStatus sets the status of whether recording is active or not throughout Small World
 * Deluxe. This also applies to whether any encoding is being performed, since both have an effect on one another.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param status
 */
void GkAudioEncoding::setRecStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &status)
{
    m_recActive = status;

    return;
}

/**
 * @brief GkAudioEncoding::encodeOpus will perform an encoding with the Ogg Opus library and its parameters.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>,
 * александр дмитрыч <https://stackoverflow.com/questions/51638654/how-to-encode-and-decode-audio-data-with-opus>
 * @param bitrate The data rate (in kilobits per second, i.e. '192' without the quotes) at which you wish to encode the
 * multimedia file in question.
 * @param sample_rate The sample rate at which to encode with (i.e. 48,000 Hz).
 * @param max_amplitude The maximum amplitude of an audio signal that can be possibly reached.
 * @param media_path The absolute path to where the encoded information will be written.
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeOpus(const qint32 &bitrate, qint32 sample_rate, const quint32 &max_amplitude,
                                 const GkAudioSource &audio_src, const QFileInfo &media_path, const qint32 &frame_size)
{
    try {
        //
        // Ogg Opus
        //
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodeVorbis
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bitrate The data rate (in kilobits per second, i.e. '192' without the quotes) at which you wish to encode the
 * multimedia file in question.
 * @param sample_rate The sample rate at which to encode with (i.e. 48,000 Hz).
 * @param max_amplitude The maximum amplitude of an audio signal that can be possibly reached.
 * @param media_path The absolute path to where the encoded information will be written.
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeVorbis(const qint32 &bitrate, qint32 sample_rate, const quint32 &max_amplitude,
                                   const GkAudioSource &audio_src, const QFileInfo &media_path, const qint32 &frame_size)
{
    try {
        //
        // Ogg Opus
        //
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodeFLAC
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bitrate The data rate (in kilobits per second, i.e. '192' without the quotes) at which you wish to encode the
 * multimedia file in question.
 * @param sample_rate The sample rate at which to encode with (i.e. 48,000 Hz).
 * @param max_amplitude The maximum amplitude of an audio signal that can be possibly reached.
 * @param media_path The absolute path to where the encoded information will be written.
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeFLAC(const qint32 &bitrate, qint32 sample_rate, const quint32 &max_amplitude,
                                 const GkAudioSource &audio_src, const QFileInfo &media_path, const qint32 &frame_size)
{
    try {
        //
        // Ogg Opus
        //
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodeCodec2 begins the audio encoding process with the Codec2 speech/telephony, low bit-rate,
 * audio codec.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param max_amplitude The maximum amplitude of an audio signal that can be possibly reached.
 * @param audio_src The audio source in question, whether it is input, output, or a mix of the two.
 * @param media_path The location of where to save the audio file output (i.e. as a file).
 */
void GkAudioEncoding::encodeCodec2(const quint32 &max_amplitude, const Settings::GkAudioSource &audio_src,
                                   const QFileInfo &media_path)
{
    try {
        if (audio_src == GkAudioSource::Input) {
            #ifdef CODEC2_LIBS_ENBLD
            m_codec2Sink = new GkCodec2Sink(media_path.canonicalFilePath(), CODEC2_MODE_1300, true, false, max_amplitude,
                                            gkAudioInput->format(), gkEventLogger, this);
            m_codec2Sink->start();
            emit stopRecInput();
            gkAudioInput->moveToThread(this->thread());
            gkAudioInput->start(m_codec2Sink);
            #endif
        } else if (audio_src == GkAudioSource::Output) {
            #ifdef CODEC2_LIBS_ENBLD
            m_codec2Sink = new GkCodec2Sink(media_path.canonicalFilePath(), CODEC2_MODE_1300, true, false, max_amplitude,
                                            gkAudioOutput->format(), gkEventLogger, this);
            m_codec2Sink->start();
            emit stopRecOutput();
            gkAudioOutput->moveToThread(this->thread());
            gkAudioOutput->start(m_codec2Sink);
            #endif
        } else {
            throw std::invalid_argument(tr("An invalid or currently unsupported audio source has been chosen!").toStdString());
        }
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodePcmWav encodes to a audio file using PCM/WAV settings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param max_amplitude The maximum amplitude of an audio signal that can be possibly reached.
 * @param audio_src The audio source in question, whether it is input, output, or a mix of the two.
 * @param media_path The location of where to save the audio file output (i.e. as a file).
 */
void GkAudioEncoding::encodePcmWav(const quint32 &max_amplitude, const Settings::GkAudioSource &audio_src,
                                   const QFileInfo &media_path)
{
    try {
        //
        // PCM WAV
        //
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
    }

    return;
}

/**
 * @brief GkAudioEncoding::opusCleanup cleans up after the Opus multimedia encoder in a neat and tidy fashion, all
 * contained within the one function.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioEncoding::opusCleanup()
{
    emit recStatus(GkAudioRecordStatus::Finished);
    if (m_opusComments) {
        ope_comments_destroy(m_opusComments);
    }

    if (m_opusEncoder) {
        ope_encoder_destroy(m_opusEncoder);
    }

    return;
}
