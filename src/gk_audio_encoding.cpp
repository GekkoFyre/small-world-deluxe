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
#include <boost/exception/all.hpp>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <exception>
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

GkAudioEncoding::GkAudioEncoding(QPointer<GekkoFyre::GkLevelDb> database, QPointer<QAudioOutput> audioOutput,
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
    m_chosen_codec = CodecSupport::Unknown;

    //
    // Open the buffer for reading and writing purposes!
    record_input_buf = new QBuffer(this);
    record_input_buf->open(QBuffer::ReadWrite);

    QObject::connect(this, SIGNAL(pauseEncode()), this, SLOT(stopCaller()));
    QObject::connect(this, SIGNAL(error(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &)),
                     this, SLOT(handleError(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &)));
    QObject::connect(gkAudioInput, &QAudioInput::notify, this, &GkAudioEncoding::processAudioIn);
    QObject::connect(this, SIGNAL(initialize()), this, SLOT(startRecBuffer()));
}

GkAudioEncoding::~GkAudioEncoding()
{
    if (m_initialized) {
        m_initialized = false;

        if (m_opus_encoder) {
            ope_encoder_destroy(m_opus_encoder);
        }

        if (m_opus_comments) {
            ope_comments_destroy(m_opus_comments);
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

        m_chosen_codec = codec_choice;
        m_file_path = media_path;
        m_channels = gkAudioInput->format().channelCount();

        //
        // QAudio defines Stereo as a count of '3' for some odd reason??? No idea why...
        if (gkAudioInput->format().channelCount() == 3) {
            m_channels = 2;
        }

        if (m_sample_rate < 8000) {
            throw std::invalid_argument(tr("Invalid sample rate provided whilst trying to encode with the Opus codec!").toStdString());
        }

        if (m_channels < 1) {
            throw std::invalid_argument(tr("Invalid number of audio channels provided whilst trying to encode with the Opus codec!").toStdString());
        }

        if (codec_choice == CodecSupport::Opus) {
            //
            // Ogg Opus
            //
            m_opus_comments = ope_comments_create();
            ope_comments_add(m_opus_comments, "ARTIST", tr("%1 by %2 et al.")
            .arg(General::productName).arg(General::companyName).toStdString().c_str());
            ope_comments_add(m_opus_comments, "TITLE", tr("Recorded on %1")
            .arg(QDateTime::currentDateTime().toString()).toStdString().c_str());

            //
            // NOTE: Opus only supports frame sizes between 2.5 - 60 milliseconds!
            // https://stackoverflow.com/questions/46786922/how-to-confirm-opus-encode-buffer-size
            //
            qint32 err;
            m_opus_encoder = ope_encoder_create_file(m_file_path.string().c_str(), m_opus_comments, m_sample_rate, m_channels, 0, &err);

            if (!m_opus_encoder) {
                emit error(tr("Error encoding to file: %1\n\n%2").arg(QString::fromStdString(m_file_path.string()))
                                   .arg(QString::fromStdString(ope_strerror(err))), GkSeverity::Fatal);

                ope_comments_destroy(m_opus_comments);
                return;
            }

            emit initialize();
        } else if (codec_choice == CodecSupport::OggVorbis) {
            //
            // Ogg Vorbis
            //
            SF_VIRTUAL_IO m_virtual_io;
            m_virtual_io.get_filelen = GkSfVirtualInterface::qbuffer_get_filelen;
            m_virtual_io.seek = GkSfVirtualInterface::qbuffer_seek;
            m_virtual_io.read = GkSfVirtualInterface::qbuffer_read;
            m_virtual_io.write = GkSfVirtualInterface::qbuffer_write;
            m_virtual_io.tell = GkSfVirtualInterface::qbuffer_tell;

            SF_INFO sf_handle_in_info;
            sf_handle_in_info.frames = AUDIO_FRAMES_PER_BUFFER;
            sf_handle_in_info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
            sf_handle_in_info.channels = m_channels;
            sf_handle_in_info.samplerate = m_sample_rate;
            sf_handle_in_info.sections = false;
            sf_handle_in_info.seekable = true;

            QPointer<QBuffer> output_buf = new QBuffer(this);
            m_handle_in = sf_open_virtual(&m_virtual_io, SFM_WRITE, &sf_handle_in_info, (void *)output_buf);
            if (!m_handle_in) {
                emit error(tr("Error encoding to file: %1\n\n%2").arg(QString::fromStdString(m_file_path.string()))
                                   .arg(QString::fromStdString(sf_strerror(m_handle_in))), GkSeverity::Fatal);
            }

            emit initialize();
        } else if (codec_choice == CodecSupport::FLAC) {
            //
            // FLAC
            //
            SF_VIRTUAL_IO m_virtual_io;
            m_virtual_io.get_filelen = GkSfVirtualInterface::qbuffer_get_filelen;
            m_virtual_io.seek = GkSfVirtualInterface::qbuffer_seek;
            m_virtual_io.read = GkSfVirtualInterface::qbuffer_read;
            m_virtual_io.write = GkSfVirtualInterface::qbuffer_write;
            m_virtual_io.tell = GkSfVirtualInterface::qbuffer_tell;

            SF_INFO sf_handle_in_info;
            sf_handle_in_info.frames = AUDIO_FRAMES_PER_BUFFER;
            sf_handle_in_info.format = SF_FORMAT_FLAC;
            sf_handle_in_info.channels = m_channels;
            sf_handle_in_info.samplerate = m_sample_rate;
            sf_handle_in_info.sections = false;
            sf_handle_in_info.seekable = true;

            QPointer<QBuffer> output_buf = new QBuffer(this);
            m_handle_in = sf_open_virtual(&m_virtual_io, SFM_WRITE, &sf_handle_in_info, (void *)output_buf);
            if (!m_handle_in) {
                emit error(tr("Error encoding to file: %1\n\n%2").arg(QString::fromStdString(m_file_path.string()))
                                   .arg(QString::fromStdString(sf_strerror(m_handle_in))), GkSeverity::Fatal);
            }

            emit initialize();
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
    gkEventLogger->publishEvent(msg, severity, "", true, true, false, false);
    return;
}

/**
 * @brief GkAudioEncoding::processAudioIn is executed once enough audio samples have been written to the buffer in
 * question (100 milliseconds in this instance).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>,
 * Joel Svensson <https://svenssonjoel.github.io/pages/qt-audio-input/index.html>.
 */
void GkAudioEncoding::processAudioIn()
{
    if (m_initialized) {
        record_input_buf->seek(0);
        m_buffer.append(record_input_buf->readAll());

        record_input_buf->buffer().clear();
        record_input_buf->seek(0);

        while (!m_buffer.isEmpty()) {
            if (m_chosen_codec == CodecSupport::Opus) {
                auto encode_opus_async = std::async(std::launch::async, &GkAudioEncoding::encodeOpus, this, std::ref(m_buffer));
            } else if (m_chosen_codec == CodecSupport::OggVorbis) {
                auto encode_vorbis_async = std::async(std::launch::async, &GkAudioEncoding::encodeVorbis, this, std::ref(m_buffer));
            } else if (m_chosen_codec == CodecSupport::FLAC) {
                auto encode_flac_async = std::async(std::launch::async, &GkAudioEncoding::encodeFLAC, this, std::ref(m_buffer));
            } else {
                throw std::invalid_argument(tr("The codec you have chosen is not supported as of yet, please try another!").toStdString());
            }
        }
    }

    return;
}

/**
 * @brief GkAudioEncoding::startRecBuffer
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioEncoding::startRecBuffer()
{
    gkAudioInput->setNotifyInterval(100);
    gkAudioInput->start(record_input_buf);

    return;
}

/**
 * @brief GkAudioEncoding::encodeOpus will perform an encoding with the Ogg Opus library and its parameters.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>,
 * александр дмитрыч <https://stackoverflow.com/questions/51638654/how-to-encode-and-decode-audio-data-with-opus>
 * @param data_buf the data to write to the audio encoder.
 */
void GkAudioEncoding::encodeOpus(const QByteArray &data_buf)
{
    if (!m_initialized) {
        return;
    }

    opus_int16 input_frame[AUDIO_OPUS_FRAMES_PER_BUFFER] = {};
    int err = 0;
    for (qint32 i = 0; i < AUDIO_OPUS_FRAMES_PER_BUFFER; ++i) {
        // Convert from little endian...
        for (qint32 j = 0; j < AUDIO_OPUS_FRAMES_PER_BUFFER; ++j) {
            input_frame[j] = qFromLittleEndian<opus_int16>(m_buffer.data() + j * AUDIO_OPUS_INT_SIZE);
        }

        //
        // https://stackoverflow.com/questions/46786922/how-to-confirm-opus-encode-buffer-size
        if (m_sample_rate == 48000) {
            //
            // The frame-size must therefore be 10 milliseconds for stereo!
            m_frame_size = ((48000 / 1000) * 2) * 10;
        }

        err = ope_encoder_write(m_opus_encoder, input_frame, m_frame_size);
        if (err != OPE_OK) {
            emit error(tr("Error encoding to file: %1\n\n%2").arg(QString::fromStdString(m_file_path.string()))
                               .arg(QString::fromStdString(ope_strerror(err))), GkSeverity::Fatal);

            ope_comments_destroy(m_opus_comments);
            return;
        }

        err = ope_encoder_drain(m_opus_encoder);
        if (err != OPE_OK) {
            emit error(tr("Error encoding to file: %1\n\n%2").arg(QString::fromStdString(m_file_path.string()))
                               .arg(QString::fromStdString(ope_strerror(err))), GkSeverity::Fatal);

            ope_comments_destroy(m_opus_comments);
            return;
        }

        m_buffer.remove(0, AUDIO_OPUS_FRAMES_PER_BUFFER * AUDIO_OPUS_INT_SIZE);
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodeVorbis
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data_buf the data to write to the audio encoder.
 */
void GkAudioEncoding::encodeVorbis(const QByteArray &data_buf)
{
    //
    // Ogg Vorbis
    // https://github.com/libsndfile/libsndfile/blob/master/examples/sndfilehandle.cc
    // https://github.com/libsndfile/libsndfile/blob/master/examples/sfprocess.c
    //
    QByteArray buf_tmp = data_buf;
    while (!buf_tmp.isEmpty()) {
        qint32 input_frame[AUDIO_FRAMES_PER_BUFFER] = {};
        for (qint32 i = 0; i < AUDIO_FRAMES_PER_BUFFER; ++i) {
            // Convert from little endian...
            for (qint32 j = 0; j < AUDIO_FRAMES_PER_BUFFER; ++j) {
                input_frame[j] = qFromLittleEndian<qint16>(buf_tmp.data() + j);
            }
        }

        //
        // libsndfile:
        // For writing data chunks in terms of frames.
        // The number of items actually read/written = frames * number of channels.
        //
        sf_writef_int(m_handle_in, input_frame, AUDIO_FRAMES_PER_BUFFER);
        buf_tmp.remove(0, AUDIO_FRAMES_PER_BUFFER);
    }

    return;
}

/**
 * @brief GkAudioEncoding::encodeFLAC
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data_buf the data to write to the audio encoder.
 */
void GkAudioEncoding::encodeFLAC(const QByteArray &data_buf)
{
    //
    // FLAC
    // https://github.com/libsndfile/libsndfile/blob/master/examples/sndfilehandle.cc
    // https://github.com/libsndfile/libsndfile/blob/master/examples/sfprocess.c
    //
    QByteArray buf_tmp = data_buf;
    while (!buf_tmp.isEmpty()) {
        qint32 input_frame[AUDIO_FRAMES_PER_BUFFER] = {};
        for (qint32 i = 0; i < AUDIO_FRAMES_PER_BUFFER; ++i) {
            // Convert from little endian...
            for (qint32 j = 0; j < AUDIO_FRAMES_PER_BUFFER; ++j) {
                input_frame[j] = qFromLittleEndian<qint16>(buf_tmp.data() + j);
            }
        }

        //
        // libsndfile:
        // For writing data chunks in terms of frames.
        // The number of items actually read/written = frames * number of channels.
        //
        sf_writef_int(m_handle_in, input_frame, AUDIO_FRAMES_PER_BUFFER);
        buf_tmp.remove(0, AUDIO_FRAMES_PER_BUFFER);
    }

    return;
}
