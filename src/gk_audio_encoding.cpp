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

#include "src/gk_audio_encoding.hpp"
#include <vector>
#include <chrono>
#include <cstring>
#include <utility>
#include <iterator>
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

#define OGG_VORBIS_READ (1024)

GkAudioEncoding::GkAudioEncoding(const QPointer<QBuffer> &audioInputBuf, const QPointer<QBuffer> &audioOutputBuf,
                                 QPointer<GekkoFyre::GkLevelDb> database, QPointer<QAudioOutput> audioOutput,
                                 QPointer<QAudioInput> audioInput, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                 QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QObject(parent)
{
    setParent(parent);
    gkDb = std::move(database);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);

    //
    // Initialize variables
    m_initialized = false;
    m_recActive = GkAudioRecordStatus::Defunct;

    //
    // Open and initialize the buffers for reading and writing purposes!
    m_encoded_buf = new QBuffer(this);
    gkAudioInputBuf = std::move(audioInputBuf);
    gkAudioOutputBuf = std::move(audioOutputBuf);

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
            opus_encoder_destroy(m_opusEncoder);
        }

        if (m_opusComments) {
            ope_comments_destroy(m_opusComments);
        }

        if (m_out_file.isOpen()) {
            m_out_file.close();
        }
    }

    if (m_encodeOpusThread.joinable()) {
        m_encodeOpusThread.join();
    }

    if (m_encodeVorbisThread.joinable()) {
        m_encodeVorbisThread.join();
    }

    if (m_encodeFLACThread.joinable()) {
        m_encodeFLACThread.join();
    }

    m_buffer.clear();
    m_buffer.shrink_to_fit();
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
 * @brief GkAudioEncoding::getRecStatus gets the status of whether recording is active or not throughout Small World
 * Deluxe. This also applies to whether any encoding is being performed, since both have an effect on one another.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
GkAudioRecordStatus GkAudioEncoding::getRecStatus() const
{
    return m_recActive;
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
 * @param bitrate The bitrate to make the recording within.
 * @param codec_choice The choice of codec you wish to encode with, whether that be Opus, Ogg Vorbis, MP3, or possibly something else!
 * @param audio_source The audio source in question, whether it is input, output, or a mix of the two.
 * @param frame_size The frame size to record with.
 * @param application
 * @note rafix07 <https://stackoverflow.com/questions/53405439/how-to-use-stdasync-on-a-queue-of-vectors-or-a-vector-of-vectors-to-sort>.
 */
void GkAudioEncoding::startCaller(const QFileInfo &media_path, const qint32 &bitrate, const GkAudioFramework::CodecSupport &codec_choice,
                                  const GkAudioSource &audio_source, const qint32 &frame_size, const qint32 &application)
{
    try {
        if (m_initialized) {
            return;
        }

        if (audio_source == GkAudioSource::Input) {
            if (gkAudioInput->state() != QAudio::ActiveState) {
                gkEventLogger->publishEvent(tr("Audio encoding has terminated early. Reason: Audio 'input' was not active."), GkSeverity::Error, "",
                                            false, true, false, true, false);
                emit recStatus(GkAudioRecordStatus::Defunct);
                return;
            }
        } else if (audio_source == GkAudioSource::Output) {
            if (gkAudioOutput->state() != QAudio::ActiveState) {
                gkEventLogger->publishEvent(tr("Audio encoding has terminated early. Reason: Audio 'output' was not active."), GkSeverity::Error, "",
                                            false, true, false, true, false);
                emit recStatus(GkAudioRecordStatus::Defunct);
                return;
            }
        } else {
            throw std::invalid_argument(tr("An invalid or currently unsupported audio source has been chosen!").toStdString());
        }

        //
        // Clear the audio buffer prior to use!
        m_buffer.clear();
        m_buffer.shrink_to_fit();

        //
        // Set frame-size!
        m_frameSize = frame_size;

        if (audio_source == GkAudioSource::Input) {
            //
            // Input device
            m_channels = gkAudioInput->format().channelCount();

            //
            // QAudio defines Stereo as a count of '3' for some odd reason??? No idea why...
            if (gkAudioInput->format().channelCount() == 3) {
                m_channels = 2;
            }
        } else {
            //
            // Output device
            m_channels = gkAudioOutput->format().channelCount();

            //
            // QAudio defines Stereo as a count of '3' for some odd reason??? No idea why...
            if (gkAudioOutput->format().channelCount() == 3) {
                m_channels = 2;
            }
        }

        if (m_channels < 1) {
            throw std::invalid_argument(tr("Invalid number of audio channels provided whilst trying to encode with the Opus codec!").toStdString());
        }

        if (codec_choice == CodecSupport::Opus) {
            //
            // Ogg Opus
            //
            if (m_encodeOpusThread.joinable()) {
                m_encodeOpusThread.join();
            }

            m_encodeOpusThread = std::thread(&GkAudioEncoding::encodeOpus, this, bitrate, std::ref(audio_source), media_path, m_frameSize);
            m_encodeOpusThread.detach();
        } else if (codec_choice == CodecSupport::OggVorbis) {
            //
            // Ogg Vorbis
            //
            if (m_encodeVorbisThread.joinable()) {
                m_encodeVorbisThread.join();
            }

            m_encodeVorbisThread = std::thread(&GkAudioEncoding::encodeVorbis, this, bitrate, gkAudioInput->format().sampleRate(),
                                               std::ref(audio_source), media_path, m_frameSize);
            m_encodeVorbisThread.detach();
        } else if (codec_choice == CodecSupport::FLAC) {
            //
            // FLAC
            //
            if (m_encodeFLACThread.joinable()) {
                m_encodeFLACThread.join();
            }

            m_encodeFLACThread = std::thread(&GkAudioEncoding::encodeFLAC, this, bitrate, gkAudioInput->format().sampleRate(),
                                             std::ref(audio_source), media_path, m_frameSize);
            m_encodeFLACThread.detach();
        } else {
            throw std::invalid_argument(tr("Invalid audio encoding codec specified! It is either not supported yet or an error was made.").toStdString());
        }
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
 * @param media_path The absolute path to where the encoded information will be written.
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeOpus(const qint32 &bitrate, const GkAudioSource &audio_src, const QFileInfo &media_path,
                                 const qint32 &frame_size)
{
    try {
        //
        // Ogg Opus
        //
        std::lock_guard<std::mutex> lock_g(m_encodeOggOpusMtx);
        QDir::setCurrent(media_path.absolutePath());
        m_out_file.setFileName(media_path.fileName());
        if (media_path.exists() && media_path.isFile()) {
            throw std::invalid_argument(tr("Attempting to encode towards file, \"%1\", has failed! Error: file already exists.")
            .arg(media_path.absoluteFilePath()).toStdString());
        }

        m_out_file.open(QIODevice::ReadWrite, QIODevice::NewOnly);
        if (!m_out_file.isOpen()) {
            throw std::runtime_error(tr("Error with opening file, \"%1\"!").arg(media_path.absoluteFilePath()).toStdString());
        }

        //
        // Read any pre-existing data into the QByteArray, such as if we started encoding with Ogg Opus at a previous stage
        // but then paused for some reason or another!
        if (!m_out_file.readAll().isEmpty()) {
            m_fileData = m_out_file.readAll();
        }

        qint32 err = 0;
        const qint32 m_size = qint32(sizeof(float)) * m_channels * frame_size;
        m_opusEncoder = opus_encoder_create(AUDIO_OPUS_DEFAULT_SAMPLE_RATE, m_channels, OPUS_APPLICATION_VOIP, &err);
        if (err != OPUS_OK || !m_opusEncoder) {
            throw std::runtime_error(gkStringFuncs->handleOpusError(err).toStdString());
        }

        //
        // Set the desired bit-rate while other parameters can be set as needed, also. Just remember that the Opus library
        // is designed to have good defaults by standard, so only set parameters you know that are really needed. Doing so
        // otherwise is likely to result in worsened multimedia quality and/or overall performance.
        //
        m_initialized = true;
        err = opus_encoder_ctl(m_opusEncoder, OPUS_SET_BITRATE(bitrate));
        gkEventLogger->publishEvent(tr("Initiate encoding with Opus as the codec! Frame size: %1. Sample rate: %2. Bit rate: %3. Channels: %4.")
                                            .arg(QString::number(m_size), QString::number(AUDIO_OPUS_DEFAULT_SAMPLE_RATE), QString::number(bitrate), QString::number(m_channels)), GkSeverity::Info, "",
                                    false, true, false, false, false);
        if (err != OPUS_OK) {
            throw std::runtime_error(gkStringFuncs->handleOpusError(err).toStdString());
        }

        refreshAudioBuffers(audio_src, m_size); // Execute this just once before the do-while loop, in order to prime the variables beforehand!
        // std::this_thread::sleep_for (std::chrono::milliseconds(AUDIO_ENCODING_VAR_PRIME_SLEEP_MILLISECS));

        do {
            if (!m_initialized) {
                if (m_opusEncoder) {
                    opus_encoder_destroy(m_opusEncoder);
                }

                return;
            }

            if (audio_src == GkAudioSource::Input) {
                if (gkAudioInput->state() != QAudio::ActiveState) {
                    gkEventLogger->publishEvent(tr("Opus encoding has terminated early. Reason: Audio 'input' was not active."), GkSeverity::Error, "",
                                                false, true, false, true, false);
                    emit recStatus(GkAudioRecordStatus::Defunct);
                    if (m_opusEncoder) {
                        opus_encoder_destroy(m_opusEncoder);
                    }

                    return;
                }
            } else if (audio_src == GkAudioSource::Output) {
                if (gkAudioOutput->state() != QAudio::ActiveState) {
                    gkEventLogger->publishEvent(tr("Opus encoding has terminated early. Reason: Audio 'output' was not active."), GkSeverity::Error, "",
                                                false, true, false, true, false);
                    emit recStatus(GkAudioRecordStatus::Defunct);
                    if (m_opusEncoder) {
                        opus_encoder_destroy(m_opusEncoder);
                    }

                    return;
                }
            } else {
                throw std::invalid_argument(tr("An invalid or currently unsupported audio source has been chosen!").toStdString());
            }

            refreshAudioBuffers(audio_src, m_size);
            const auto result = opusEncodeHelper(m_opusEncoder, frame_size, m_size, AUDIO_OPUS_MAX_FRAMES_PER_BUFFER * 3);
            if (!result.isEmpty()) {
                //
                // Write out the encoded, Ogg Opus data, to the given output file in question!
                m_fileData.insert(m_out_file.size() + 1, result.data());
                m_out_file.seek(0);
                m_out_file.write(m_fileData);
                m_out_file.flush();
            }
        } while (!m_buffer.isEmpty() || m_recActive == GkAudioRecordStatus::Active);

        //
        // Create and initiate the encoded Opus multimedia file comments!
        m_opusComments = ope_comments_create();
        err = ope_comments_add(m_opusComments, "ARTIST", tr("%1 by %2 et al.")
                .arg(General::productName, General::companyName).toStdString().c_str());
        if (err != OPUS_OK) {
            opusCleanup();
            throw std::runtime_error(tr("Error with inserting comments while encoding with Opus! Error: %1")
                                             .arg(gkStringFuncs->handleOpusError(err)).toStdString());
        }

        err = ope_comments_add(m_opusComments, "TITLE", tr("Recorded on %1")
                .arg(QDateTime::currentDateTime().toString()).toStdString().c_str());
        if (err != OPUS_OK) {
            opusCleanup();
            throw std::runtime_error(tr("Error with inserting comments while encoding with Opus! Error: %1")
                                             .arg(gkStringFuncs->handleOpusError(err)).toStdString());
        }

        //
        // Perform any cleanup operations now...
        opusCleanup();
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
 * @param media_path The absolute path to where the encoded information will be written.
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeVorbis(const qint32 &bitrate, qint32 sample_rate, const GkAudioSource &audio_src,
                                   const QFileInfo &media_path, const qint32 &frame_size)
{
    try {
        //
        // Ogg Vorbis
        // https://github.com/libsndfile/libsndfile/blob/master/examples/sndfilehandle.cc
        // https://github.com/libsndfile/libsndfile/blob/master/examples/sfprocess.c
        //
        std::lock_guard<std::mutex> lock_g(m_encodeOggVorbisMtx);
        QDir::setCurrent(media_path.absolutePath());
        m_out_file.setFileName(media_path.fileName());
        if (media_path.exists() && media_path.isFile()) {
            throw std::invalid_argument(tr("Attempting to encode towards file, \"%1\", has failed! Error: file already exists.")
                                                .arg(media_path.absoluteFilePath()).toStdString());
        }

        m_out_file.open(QIODevice::ReadWrite, QIODevice::NewOnly);
        if (!m_out_file.isOpen()) {
            throw std::runtime_error(tr("Error with opening file, \"%1\"!").arg(media_path.absoluteFilePath()).toStdString());
        }

        //
        // Read any pre-existing data into the QByteArray, such as if we started encoding with Ogg Opus at a previous stage
        // but then paused for some reason or another!
        if (!m_out_file.readAll().isEmpty()) {
            m_fileData = m_out_file.readAll();
        }

        const qint32 m_size = frame_size * m_channels;
        m_initialized = true;
        do {
            if (!m_initialized) {
                return;
            }

            if (audio_src == GkAudioSource::Input) {
                gkAudioInputBuf->seek(0);
                m_buffer.append(gkAudioInputBuf->readAll());
            } else if (audio_src == GkAudioSource::Output) {
                gkAudioOutputBuf->seek(0);
                m_buffer.append(gkAudioOutputBuf->readAll());
            } else {
                throw std::invalid_argument(tr("An invalid or currently unsupported audio source has been chosen!").toStdString());
            }

            std::vector<qint32> input_frame;
            input_frame.reserve(m_size);
            for (qint32 i = 0; i < m_size; ++i) {
                // Convert from little endian...
                for (qint32 j = 0; j < m_size; ++j) {
                    input_frame[j] += qFromLittleEndian<qint16>(m_buffer.data() + j);
                }
            }

            //
            // libsndfile:
            // For writing data chunks in terms of frames.
            // The number of items actually read/written = frames * number of channels.
            //
            m_handle_in.write(input_frame.data(), m_size);
            m_buffer.remove(0, m_size);
            input_frame.clear();

            //
            // Write to an output file!
            m_encoded_buf->seek(0);
            m_out_file.write(m_encoded_buf->readAll());
            m_out_file.flush();

            //
            // DO NOT DELETE THIS!
            m_encoded_buf->buffer().clear();
            m_encoded_buf->seek(0);
        } while (!m_buffer.isEmpty() || m_recActive == GkAudioRecordStatus::Active);
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodeFLAC
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bitrate The data rate (in kilobits per second, i.e. '192' without the quotes) at which you wish to encode the
 * multimedia file in question.
 * @param sample_rate The sample rate at which to encode with (i.e. 48,000 Hz).
 * @param media_path The absolute path to where the encoded information will be written.
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeFLAC(const qint32 &bitrate, qint32 sample_rate, const GkAudioSource &audio_src,
                                 const QFileInfo &media_path, const qint32 &frame_size)
{
    try {
        //
        // FLAC
        // https://github.com/libsndfile/libsndfile/blob/master/examples/sndfilehandle.cc
        // https://github.com/libsndfile/libsndfile/blob/master/examples/sfprocess.c
        //
        std::lock_guard<std::mutex> lock_g(m_encodeFlacMtx);
        QDir::setCurrent(media_path.absolutePath());
        m_out_file.setFileName(media_path.fileName());
        if (media_path.exists() && media_path.isFile()) {
            throw std::invalid_argument(tr("Attempting to encode towards file, \"%1\", has failed! Error: file already exists.")
                                                .arg(media_path.absoluteFilePath()).toStdString());
        }

        m_out_file.open(QIODevice::ReadWrite, QIODevice::NewOnly);
        if (!m_out_file.isOpen()) {
            throw std::runtime_error(tr("Error with opening file, \"%1\"!").arg(media_path.absoluteFilePath()).toStdString());
        }

        //
        // Read any pre-existing data into the QByteArray, such as if we started encoding with Ogg Opus at a previous stage
        // but then paused for some reason or another!
        if (!m_out_file.readAll().isEmpty()) {
            m_fileData = m_out_file.readAll();
        }

        const qint32 m_size = frame_size * m_channels;
        m_initialized = true;
        do {
            if (!m_initialized) {
                return;
            }

            if (audio_src == GkAudioSource::Input) {
                gkAudioInputBuf->seek(0);
                m_buffer.append(gkAudioInputBuf->readAll());
            } else if (audio_src == GkAudioSource::Output) {
                gkAudioOutputBuf->seek(0);
                m_buffer.append(gkAudioOutputBuf->readAll());
            } else {
                throw std::invalid_argument(tr("An invalid or currently unsupported audio source has been chosen!").toStdString());
            }

            std::vector<qint32> input_frame;
            input_frame.reserve(m_size);
            for (qint32 i = 0; i < m_size; ++i) {
                // Convert from little endian...
                for (qint32 j = 0; j < m_size; ++j) {
                    input_frame[j] += qFromLittleEndian<qint16>(m_buffer.data() + j);
                }
            }

            //
            // libsndfile:
            // For writing data chunks in terms of frames.
            // The number of items actually read/written = frames * number of channels.
            //
            m_handle_in.write(input_frame.data(), m_size);
            m_buffer.remove(0, m_size);
            input_frame.clear();

            //
            // Write to an output file!
            m_encoded_buf->seek(0);
            m_out_file.write(m_encoded_buf->readAll());
            m_out_file.flush();

            //
            // DO NOT DELETE THIS!
            m_encoded_buf->buffer().clear();
            m_encoded_buf->seek(0);
        } while (!m_buffer.isEmpty() || m_recActive == GkAudioRecordStatus::Active);
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    return;
}

/**
 * @brief GkAudioEncoding::refreshAudioBuffers refreshes/updates the audio buffers for both the audio input and/or output
 * devices.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_src The audio source for which buffers to update henceforth.
 * @param buf_size The minimum size of the buffer that must be reached to exit the internal loop within this function.
 */
void GkAudioEncoding::refreshAudioBuffers(const GkAudioSource &audio_src, const qint32 &buf_size)
{
    std::lock_guard<std::mutex> lock_guard(m_refreshAudioBufs);
    if (audio_src == GkAudioSource::Input) {
        if (gkAudioInput->state() != QAudio::ActiveState) {
            m_recActive = GkAudioRecordStatus::Defunct; // Has to modify the variable directly, since we are working from differing threads than the main, GUI-thread!
            return;
        }

        do {
            gkAudioInputBuf->seek(0);
            m_buffer.append(gkAudioInputBuf->readAll());
        } while (m_buffer.isEmpty() && m_buffer.size() < buf_size);
    } else if (audio_src == GkAudioSource::Output) {
        if (gkAudioOutput->state() != QAudio::ActiveState) {
            m_recActive = GkAudioRecordStatus::Defunct; // Has to modify the variable directly, since we are working from differing threads than the main, GUI-thread!
            return;
        }

        do {
            gkAudioOutputBuf->seek(0);
            m_buffer.append(gkAudioOutputBuf->readAll());
        } while (m_buffer.isEmpty() && m_buffer.size() < buf_size);
    } else {
        return;
    }

    return;
}

/**
 * @brief GkAudioEncoding::opusEncodeHelper performs the actual encoding of given QByteArray data (obtained via either
 * QAudioInput and/or QAudioOutput) into Ogg Opus data before returning said data also as a QByteArray, for easy handling
 * via other Qt functions/code.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param opusEncoder The Ogg Opus object required for doing the act of encoding itself.
 * @param frame_size The frame size to use when encoding.
 * @param max_packet_size The maximum packet size to use when encoding.
 * @return The encoded Ogg Opus data, returned as a QByteArray for easy handling via other Qt functions/code.
 */
QByteArray GkAudioEncoding::opusEncodeHelper(OpusEncoder *opusEncoder, const qint32 &frame_size,
                                             const qint32 &m_size, const qint32 &max_packet_size)
{
    try {
        if (!m_initialized) {
            return QByteArray();
        }

        QByteArray input = m_buffer.mid(0, m_size);
        QByteArray output = QByteArray(AUDIO_OPUS_MAX_FRAMES_PER_BUFFER * 3, char(0));
        m_buffer.remove(0, m_size);
        if (input.isEmpty()) {
            emit error(tr("No input data to work with!"), GkSeverity::Error);
            return QByteArray();
        }

        //
        // Encode the frame...
        const qint32 nbBytes = opus_encode_float(opusEncoder, reinterpret_cast<const float *>(input.constData()), frame_size,
                                                 reinterpret_cast<uchar *>(output.data()), max_packet_size);
        if (nbBytes < 0) {
            throw std::runtime_error(tr("Error encoding with Opus. Given output was expected to be larger than zero, but instead were: %1 bytes.")
            .arg(QString::number(nbBytes)).toStdString());
        }

        //
        // Write out the encoded, Ogg Opus data, to the given output file in question!
        output.resize(nbBytes);
        return output;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("Ogg Opus encoding has failed! Error: %1")
        .arg(QString::fromStdString(e.what())).toStdString()));
    }

    return QByteArray();
}

/**
 * @brief GkAudioEncoding::opusCleanup cleans up after the Opus multimedia encoder in a neat and tidy fashion, all
 * contained within the one function.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioEncoding::opusCleanup()
{
    emit recStatus(GkAudioRecordStatus::Finished);
    if (m_out_file.isOpen()) {
        m_out_file.close();
    }

    if (m_opusComments) {
        ope_comments_destroy(m_opusComments);
    }

    if (m_opusEncoder) {
        opus_encoder_destroy(m_opusEncoder);
    }

    return;
}
