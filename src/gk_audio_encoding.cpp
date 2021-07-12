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
                                 QPointer<QAudioInput> audioInput, const GkDevice &output_device, const GkDevice &input_device,
                                 QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QObject(parent)
{
    setParent(parent);
    gkDb = std::move(database);
    gkEventLogger = std::move(eventLogger);

    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    gkInputDev = input_device;
    gkOutputDev = output_device;

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

    if (m_audioInEncodeThread.joinable()) {
        m_audioInEncodeThread.join();
    }

    if (m_audioOutEncodeThread.joinable()) {
        m_audioOutEncodeThread.join();
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
 * @param audio_dev_info Information pertaining to the audio device in question to record from.
 * @param bitrate The bitrate to make the recording within.
 * @param codec_choice The choice of codec you wish to encode with, whether that be Opus, Ogg Vorbis, MP3, or possibly something else!
 * @param frame_size The frame size to record with.
 * @param application
 * @note rafix07 <https://stackoverflow.com/questions/53405439/how-to-use-stdasync-on-a-queue-of-vectors-or-a-vector-of-vectors-to-sort>.
 */
void GkAudioEncoding::startCaller(const QFileInfo &media_path, const Database::Settings::Audio::GkDevice &audio_dev_info,
                                  const qint32 &bitrate, const GkAudioFramework::CodecSupport &codec_choice,
                                  const qint32 &frame_size, const qint32 &application)
{
    try {
        if (m_initialized) {
            return;
        }

        //
        // Clear the audio buffer prior to use!
        m_buffer.clear();
        m_buffer.shrink_to_fit();

        //
        // Initiate variables
        if (media_path.exists()) {
            m_file_path = media_path; // The location of where to make the recording!
        }

        m_frameSize = frame_size;

        if (audio_dev_info.default_input_dev) {
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
            if (audio_dev_info.default_input_dev) {
                //
                // Input device
                if (m_audioInEncodeThread.joinable()) {
                    m_audioInEncodeThread.join();
                }

                m_audioInEncodeThread = std::thread(&GkAudioEncoding::processAudioInEncode, this, std::ref(codec_choice),
                                                    std::ref(bitrate), std::ref(audio_dev_info.chosen_sample_rate),
                                                    std::ref(frame_size));
                m_audioInEncodeThread.detach();
            } else if (audio_dev_info.default_output_dev) {
                //
                // Output device
                if (m_audioOutEncodeThread.joinable()) {
                    m_audioOutEncodeThread.join();
                }

                m_audioOutEncodeThread = std::thread(&GkAudioEncoding::processAudioOutEncode, this, std::ref(codec_choice),
                                                     std::ref(bitrate), std::ref(audio_dev_info.chosen_sample_rate),
                                                     std::ref(frame_size));
                m_audioOutEncodeThread.detach();
            } else {
                //
                // Unknown?
                throw std::invalid_argument(tr("Unable to determine if multimedia device is either input or output!").toStdString());
            }
        } else if (codec_choice == CodecSupport::OggVorbis) {
            //
            // Ogg Vorbis
            //
            if (audio_dev_info.default_input_dev) {
                //
                // Input device
                if (m_audioInEncodeThread.joinable()) {
                    m_audioInEncodeThread.join();
                }

                m_audioInEncodeThread = std::thread(&GkAudioEncoding::processAudioInEncode, this, std::ref(codec_choice),
                                                    std::ref(bitrate), std::ref(audio_dev_info.chosen_sample_rate),
                                                    std::ref(frame_size));
                m_audioInEncodeThread.detach();
            } else if (audio_dev_info.default_output_dev) {
                //
                // Output device
                if (m_audioOutEncodeThread.joinable()) {
                    m_audioOutEncodeThread.join();
                }

                m_audioOutEncodeThread = std::thread(&GkAudioEncoding::processAudioOutEncode, this, std::ref(codec_choice),
                                                     std::ref(bitrate), std::ref(audio_dev_info.chosen_sample_rate),
                                                     std::ref(frame_size));
                m_audioOutEncodeThread.detach();
            } else {
                //
                // Unknown?
                throw std::invalid_argument(tr("Unable to determine if multimedia device is either input or output!").toStdString());
            }
        } else if (codec_choice == CodecSupport::FLAC) {
            //
            // FLAC
            //
            if (audio_dev_info.default_input_dev) {
                //
                // Input device
                if (m_audioInEncodeThread.joinable()) {
                    m_audioInEncodeThread.join();
                }

                m_audioInEncodeThread = std::thread(&GkAudioEncoding::processAudioInEncode, this, std::ref(codec_choice),
                                                    std::ref(bitrate), std::ref(audio_dev_info.chosen_sample_rate),
                                                    std::ref(frame_size));
                m_audioInEncodeThread.detach();
            } else if (audio_dev_info.default_output_dev) {
                //
                // Output device
                if (m_audioOutEncodeThread.joinable()) {
                    m_audioOutEncodeThread.join();
                }

                m_audioOutEncodeThread = std::thread(&GkAudioEncoding::processAudioOutEncode, this, std::ref(codec_choice),
                                                     std::ref(bitrate), std::ref(audio_dev_info.chosen_sample_rate),
                                                     std::ref(frame_size));
                m_audioOutEncodeThread.detach();
            } else {
                //
                // Unknown?
                throw std::invalid_argument(tr("Unable to determine if multimedia device is either input or output!").toStdString());
            }
        } else {
            throw std::invalid_argument(tr("Invalid audio encoding codec specified! It is either not supported yet or an error was made.").toStdString());
        }

        m_initialized = true;
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
 * @brief GkAudioEncoding::processAudioIn is executed once enough audio samples have been written to the buffer in
 * question (100 milliseconds in this instance).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec The chosen multimedia codec to perform the encoding henceforth with.
 * @param bitrate The data rate (in kilobits per second, i.e. '192' without the quotes) at which you wish to encode the
 * multimedia file in question.
 * @param sample_rate The sample rate at which to encode with (i.e. 48,000 Hz).
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::processAudioInEncode(const GkAudioFramework::CodecSupport &codec, const qint32 &bitrate,
                                           const qint32 &sample_rate, const qint32 &frame_size)
{
    if (m_initialized) {
        gkAudioInputBuf->seek(0);
        m_buffer.append(gkAudioInputBuf->readAll());
        while (!m_buffer.isEmpty()) {
            if (codec == CodecSupport::Opus) {
                encodeOpus(bitrate, sample_rate, frame_size);
            } else if (codec == CodecSupport::OggVorbis) {
                encodeVorbis(bitrate, sample_rate, frame_size);
            } else if (codec == CodecSupport::FLAC) {
                encodeFLAC(bitrate, sample_rate, frame_size);
            } else {
                throw std::invalid_argument(tr("The codec you have chosen is not supported as of yet, please try another!").toStdString());
            }
        }
    }

    return;
}

/**
 * @brief GkAudioEncoding::processAudioOutEncode is executed once enough audio samples have been written to the buffer in
 * question (100 milliseconds in this instance).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec The chosen multimedia codec to perform the encoding henceforth with.
 * @param bitrate The data rate (in kilobits per second, i.e. '192' without the quotes) at which you wish to encode the
 * multimedia file in question.
 * @param sample_rate The sample rate at which to encode with (i.e. 48,000 Hz).
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::processAudioOutEncode(const GkAudioFramework::CodecSupport &codec, const qint32 &bitrate,
                                            const qint32 &sample_rate, const qint32 &frame_size)
{
    if (m_initialized) {
        gkAudioOutputBuf->seek(0);
        m_buffer.append(gkAudioOutputBuf->readAll());
        while (!m_buffer.isEmpty()) {
            if (codec == CodecSupport::Opus) {
                encodeOpus(bitrate, frame_size);
            } else if (codec == CodecSupport::OggVorbis) {
                encodeVorbis(bitrate, frame_size);
            } else if (codec == CodecSupport::FLAC) {
                encodeFLAC(bitrate, frame_size);
            } else {
                throw std::invalid_argument(tr("The codec you have chosen is not supported as of yet, please try another!").toStdString());
            }
        }
    }

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
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeOpus(const qint32 &bitrate, const qint32 &sample_rate, const qint32 &frame_size)
{
    //
    // Ogg Opus
    //
    if (!m_initialized) {
        return;
    }

    const opus_int32 m_sample_rate = sample_rate;
    if (m_sample_rate < 8000) {
        throw std::invalid_argument(tr("Invalid sample rate provided whilst trying to encode with the Opus codec!").toStdString());
    }

    const QDir dir_tmp = m_file_path.path(); // NOTE: Returns the file's path. This doesn't include the file name.
    const QString dir_path = dir_tmp.path();
    if (!dir_tmp.exists()) {
        bool ret = QDir().mkpath(dir_path);
        if (!ret) {
            throw std::runtime_error(tr("Unsuccessfully created directory, \"%1\"!").arg(dir_path).toStdString());
        }
    }

    QDir::setCurrent(dir_path);
    m_out_file.setFileName(m_file_path.fileName());
    if (m_file_path.exists()) {
        m_out_file.remove();
    }

    m_out_file.open(QIODevice::ReadWrite, QIODevice::NewOnly);
    if (!m_out_file.isOpen()) {
        throw std::runtime_error(tr("Error with opening file, \"%1\"!").arg(m_file_path.fileName()).toStdString());
    }

    //
    // Read any pre-existing data into the QByteArray, such as if we started encoding with Ogg Opus at a previous stage
    // but then paused for some reason or another!
    if (!m_out_file.readAll().isEmpty()) {
        m_fileData = m_out_file.readAll();
    }

    qint32 err = 0;
    std::lock_guard<std::mutex> lock_g(m_asyncOggOpusMtx);
    const qint32 m_size = int(sizeof(float)) * m_channels * frame_size;
    m_opusEncoder = opus_encoder_create(sample_rate, m_channels, OPUS_APPLICATION_AUDIO, &err);

    //
    // Set the desired bit-rate while other parameters can be set as needed, also. Just remember that the Opus library
    // is designed to have good defaults by standard, so only set parameters you know that are really needed. Doing so
    // otherwise is likely to result in worsened multimedia quality and/or overall performance.
    //
    err = opus_encoder_ctl(m_opusEncoder, OPUS_SET_BITRATE(bitrate));
    while (!m_buffer.isEmpty() && m_recActive == GkAudioRecordStatus::Active) {
        QByteArray input = m_buffer.mid(0, m_size);
        m_buffer.remove(0, m_size);

        //
        // Create and initiate the encoded Opus multimedia file comments!
        m_opusComments = ope_comments_create();
        ope_comments_add(m_opusComments, "ARTIST", tr("%1 by %2 et al.")
                .arg(General::productName, General::companyName).toStdString().c_str());
        ope_comments_add(m_opusComments, "TITLE", tr("Recorded on %1")
                .arg(QDateTime::currentDateTime().toString()).toStdString().c_str());

        QByteArray output = QByteArray(AUDIO_OPUS_MAX_FRAME_SIZE * 3, char(0));

        //
        // Encode the frame...
        const qint32 nbBytes = opus_encode_float(m_opusEncoder, reinterpret_cast<const float*>(input.constData()), frame_size, reinterpret_cast<uchar*>(output.data()), AUDIO_OPUS_MAX_FRAME_SIZE * 3);
        if (nbBytes < 0) {
            emit error(tr("Error encoding to file: %1\n\n%2").arg(m_file_path.path()),
                       GkSeverity::Fatal);

            opusCleanup();
            return;
        }

        //
        // Write out the encoded, Ogg Opus data, to the given output file in question!
        m_fileData.insert(m_out_file.size() + 1, output.data());
        m_out_file.seek(0);
        m_out_file.write(m_fileData);

        //
        // Perform any cleanup operations now...
        m_out_file.close();
        opusCleanup();
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodeVorbis
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bitrate The data rate (in kilobits per second, i.e. '192' without the quotes) at which you wish to encode the
 * multimedia file in question.
 * @param sample_rate The sample rate at which to encode with (i.e. 48,000 Hz).
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeVorbis(const qint32 &bitrate, const qint32 &sample_rate, const qint32 &frame_size)
{
    //
    // Ogg Vorbis
    // https://github.com/libsndfile/libsndfile/blob/master/examples/sndfilehandle.cc
    // https://github.com/libsndfile/libsndfile/blob/master/examples/sfprocess.c
    //
    if (!m_initialized) {
        return;
    }

    if (m_out_file.isOpen()) {
        std::lock_guard<std::mutex> lock_g(m_asyncOggVorbisMtx);
        const qint32 frame_size = AUDIO_FRAMES_PER_BUFFER * m_channels;
        while (!m_buffer.isEmpty()) {
            std::vector<qint32> input_frame;
            input_frame.reserve(frame_size);
            for (qint32 i = 0; i < frame_size; ++i) {
                // Convert from little endian...
                for (qint32 j = 0; j < frame_size; ++j) {
                    input_frame[j] += qFromLittleEndian<qint16>(m_buffer.data() + j);
                }
            }

            //
            // libsndfile:
            // For writing data chunks in terms of frames.
            // The number of items actually read/written = frames * number of channels.
            //
            m_handle_in.write(input_frame.data(), frame_size);
            m_buffer.remove(0, frame_size);
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
        }
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodeFLAC
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bitrate The data rate (in kilobits per second, i.e. '192' without the quotes) at which you wish to encode the
 * multimedia file in question.
 * @param sample_rate The sample rate at which to encode with (i.e. 48,000 Hz).
 * @param frame_size The size of the audio frame(s) in question.
 */
void GkAudioEncoding::encodeFLAC(const qint32 &bitrate, const qint32 &sample_rate, const qint32 &frame_size)
{
    //
    // FLAC
    // https://github.com/libsndfile/libsndfile/blob/master/examples/sndfilehandle.cc
    // https://github.com/libsndfile/libsndfile/blob/master/examples/sfprocess.c
    //
    if (!m_initialized) {
        return;
    }

    if (m_out_file.isOpen()) {
        std::lock_guard<std::mutex> lock_g(m_asyncFlacMtx);
        const qint32 frame_size = AUDIO_FRAMES_PER_BUFFER * m_channels;
        while (!m_buffer.isEmpty()) {
            std::vector<qint32> input_frame;
            input_frame.reserve(frame_size);
            for (qint32 i = 0; i < frame_size; ++i) {
                // Convert from little endian...
                for (qint32 j = 0; j < frame_size; ++j) {
                    input_frame[j] += qFromLittleEndian<qint16>(m_buffer.data() + j);
                }
            }

            //
            // libsndfile:
            // For writing data chunks in terms of frames.
            // The number of items actually read/written = frames * number of channels.
            //
            m_handle_in.write(input_frame.data(), frame_size);
            m_buffer.remove(0, frame_size);
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
        }
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
    opus_encoder_destroy(m_opusEncoder);
    ope_comments_destroy(m_opusComments);

    return;
}
