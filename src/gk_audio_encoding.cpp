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

GkAudioEncoding::GkAudioEncoding(QPointer<GekkoFyre::GkLevelDb> database, QPointer<QAudioOutput> audioOutput,
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

    QObject::connect(this, SIGNAL(pauseEncode()), this, SLOT(stopCaller()));
    QObject::connect(this, SIGNAL(error(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &)),
                     this, SLOT(handleError(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &)));
    QObject::connect(this, SIGNAL(recStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &)),
                     this, SLOT(setRecStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &)));

    //
    // Setup I/O!
    m_audioInputBuf = gkAudioInput->start();
    QObject::connect(m_audioInputBuf, &QIODevice::readyRead, this, &GkAudioEncoding::onReadyRead);

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

            m_encodeOpusThread = std::thread(&GkAudioEncoding::encodeOpus, this, bitrate, gkAudioInput->format().sampleRate(), std::ref(audio_source), media_path, m_frameSize);
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
void GkAudioEncoding::encodeOpus(const qint32 &bitrate, qint32 sample_rate, const GkAudioSource &audio_src,
                                 const QFileInfo &media_path, const qint32 &frame_size)
{
    try {
        //
        // Ogg Opus
        //
        std::lock_guard<std::mutex> lock_g(m_encodeOggOpusMtx);
        QDir::setCurrent(media_path.absolutePath());

        qint32 err = 0;
        if (audio_src == GkAudioSource::Input) {
            if (gkAudioInput->state() != QAudio::ActiveState) {
                gkEventLogger->publishEvent(tr("Opus encoding has terminated early. Reason: Audio 'input' was not active."), GkSeverity::Error, "",
                                            false, true, false, true, false);
                emit recStatus(GkAudioRecordStatus::Defunct);

                return;
            }
        } else if (audio_src == GkAudioSource::Output) {
            if (gkAudioOutput->state() != QAudio::ActiveState) {
                gkEventLogger->publishEvent(tr("Opus encoding has terminated early. Reason: Audio 'output' was not active."), GkSeverity::Error, "",
                                            false, true, false, true, false);
                emit recStatus(GkAudioRecordStatus::Defunct);

                return;
            }
        } else {
            throw std::invalid_argument(tr("An invalid or currently unsupported audio source has been chosen!").toStdString());
        }

        gkEventLogger->publishEvent(tr("Initiate encoding with Opus as the codec! Frame count: %1. Sample rate: %2. Bit rate: %3. Channels: %4.")
                                            .arg(QString::number(m_buffer.size()), QString::number(AUDIO_OPUS_DEFAULT_SAMPLE_RATE), QString::number(bitrate), QString::number(m_channels)), GkSeverity::Info, "",
                                    false, true, false, false, false);

        m_initialized = true;
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

        m_opusEncoder = ope_encoder_create_file(media_path.absoluteFilePath().toStdString().c_str(), m_opusComments,
                                                AUDIO_OPUS_DEFAULT_SAMPLE_RATE, m_channels, 0, &err);
        if (err != OPUS_OK || !m_opusEncoder) {
            opusCleanup();
            throw std::runtime_error(tr("Error writing to file, \"%1\", via Opus encoder. Error: %2")
                                             .arg(media_path.fileName(), gkStringFuncs->handleOpusError(err)).toStdString());
        }

        qint32 ret = m_buffer.size();
        while (1) {
            if (ret > 0) {
                opus_int16 input_frame[AUDIO_OPUS_FRAMES_PER_BUFFER] = {};
                const qint32 total_bytes_ready = m_buffer.size();
                for (qint32 i = 0; i < total_bytes_ready; ++i) {
                    //
                    // Convert from littleEndian...
                    for (qint32 j = 0; j < AUDIO_OPUS_FRAMES_PER_BUFFER; ++j) {
                        input_frame[j] = qFromLittleEndian<opus_int16>(m_buffer.data() + j * sizeof(opus_int16));
                    }

                    //
                    // https://stackoverflow.com/questions/46786922/how-to-confirm-opus-encode-buffer-size
                    qint32 frame_size = AUDIO_OPUS_FRAMES_PER_BUFFER;
                    if (sample_rate == 48000) {
                        //
                        // The frame-size must therefore be 10 milliseconds for stereo!
                        frame_size = ((48000 / 1000) * 2) * 10;
                    }

                    //
                    // Encode the frame...
                    const opus_int32 nbBytes = ope_encoder_write(m_opusEncoder, input_frame, frame_size);
                    if (nbBytes < 0) {
                        emit error(tr("Error encoding to file: %1").arg(media_path.absoluteFilePath()), GkSeverity::Fatal);

                        opusCleanup();
                    }

                    m_totalCompBytesWritten += nbBytes;
                    emit bytesRead(m_totalCompBytesWritten, false);

                    //
                    // Commit out the memory buffer to the file itself!
                    const qint32 buf_size = AUDIO_OPUS_FRAMES_PER_BUFFER * sizeof(opus_int16);
                    m_buffer.remove(0, buf_size);
                    ret -= buf_size;
                }
            } else {
                break;
            }
        }

        opusCleanup();
        gkEventLogger->publishEvent(tr("Finished encoding with the Opus codec! File: %1").arg(media_path.absoluteFilePath()),
                                    GkSeverity::Info, "", true, true, false, false, true);
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
        m_out_file.setFileName(media_path.absoluteFilePath());
        if (media_path.exists() && media_path.isFile()) {
            throw std::invalid_argument(tr("Attempting to encode towards file, \"%1\", has failed! Error: file already exists.")
                                                .arg(media_path.absoluteFilePath()).toStdString());
        }

        m_out_file.open(QIODevice::WriteOnly);
        if (!m_out_file.isOpen()) {
            throw std::runtime_error(tr("Error with opening file, \"%1\"!").arg(media_path.absoluteFilePath()).toStdString());
        }

        const qint32 m_size = frame_size * m_channels;
        m_initialized = true;
        do {
            if (!m_initialized) {
                return;
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
        m_out_file.setFileName(media_path.absoluteFilePath()); // This supports either no path (i.e. just the filename), a relative path, or an absolute path!
        if (media_path.exists() && media_path.isFile()) {
            throw std::invalid_argument(tr("Attempting to encode towards file, \"%1\", has failed! Error: file already exists.")
                                                .arg(media_path.absoluteFilePath()).toStdString());
        }

        m_out_file.open(QIODevice::WriteOnly);
        if (!m_out_file.isOpen()) {
            throw std::runtime_error(tr("Error with opening file, \"%1\"!").arg(media_path.absoluteFilePath()).toStdString());
        }

        const qint32 m_size = frame_size * m_channels;
        m_initialized = true;
        do {
            if (!m_initialized) {
                return;
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
 * @brief GkAudioEncoding::onReadyRead
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioEncoding::onReadyRead()
{
    m_buffer.append(m_audioInputBuf->readAll()); // Record QAudioInput to the buffer object, `m_buffer`!
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
