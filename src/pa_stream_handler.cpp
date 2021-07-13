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

#include "src/pa_stream_handler.hpp"
#include <utility>
#include <algorithm>
#include <exception>
#include <QIODevice>
#include <QDateTime>
#include <QtEndian>
#include <QBuffer>

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
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://qtsource.wordpress.com/2011/09/12/multithreaded-audio-using-qaudiooutput/>.
 */
GkPaStreamHandler::GkPaStreamHandler(QPointer<GekkoFyre::GkLevelDb> database, const GkDevice &output_device,
                                     const GkDevice &input_device, QPointer<QAudioOutput> audioOutput,
                                     QPointer<QAudioInput> audioInput, QPointer<GekkoFyre::GkAudioEncoding> audioEncoding,
                                     QPointer<GekkoFyre::GkEventLogger> eventLogger, std::shared_ptr<AudioFile<double>> audioFileLib,
                                     QObject *parent) : QObject(parent)
{
    setParent(parent);

    gkDb = std::move(database);
    gkEventLogger = std::move(eventLogger);
    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    gkAudioEncoding = std::move(audioEncoding);
    gkAudioFile = std::move(audioFileLib);

    //
    // Initialize variables
    //
    pref_output_device = output_device;
    pref_input_device = input_device;
    gkPcmFileStream = std::make_unique<GkPcmFileStream>(this);

    QObject::connect(gkAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(playbackHandleStateChanged(QAudio::State)));
    QObject::connect(gkAudioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(recordingHandleStateChanged(QAudio::State)));

    //
    // Playing of multimedia files
    QObject::connect(this, SIGNAL(playMedia(const QFileInfo &, const GekkoFyre::GkAudioFramework::CodecSupport &)),
                     this, SLOT(playMediaFile(const QFileInfo &, const GekkoFyre::GkAudioFramework::CodecSupport &)));
    QObject::connect(this, SIGNAL(playMedia(const QDir &, const GekkoFyre::GkAudioFramework::CodecSupport &)),
                     this, SLOT(playMediaFile(const QDir &, const GekkoFyre::GkAudioFramework::CodecSupport &)));

    //
    // Stopping of either playing or recording of multimedia files
    QObject::connect(this, SIGNAL(stopMedia(const QFileInfo &)), this, SLOT(stopMediaFile(const QFileInfo &)));
    QObject::connect(this, SIGNAL(stopMedia(const QDir &)), this, SLOT(stopMediaFile(const QDir &)));

    //
    // Recording of multimedia files
    QObject::connect(this, SIGNAL(recordMedia(const QFileInfo &, const GekkoFyre::GkAudioFramework::CodecSupport &, qint32)),
                     this, SLOT(recordMediaFile(const QFileInfo &, const GekkoFyre::GkAudioFramework::CodecSupport &, qint32)));
    QObject::connect(this, SIGNAL(recordMedia(const QDir &, const GekkoFyre::GkAudioFramework::CodecSupport &, qint32)),
                     this, SLOT(recordMediaFile(const QDir &, const GekkoFyre::GkAudioFramework::CodecSupport &, qint32)));

    //
    // Changing the playback state of multimedia files (i.e. for recording, playing, etc.)
    QObject::connect(this, SIGNAL(changePlaybackState(QAudio::State)), this, SLOT(playbackHandleStateChanged(QAudio::State)));
    QObject::connect(this, SIGNAL(changeRecorderState(QAudio::State)), this, SLOT(recordingHandleStateChanged(QAudio::State)));

    //
    // Miscellaneous
    QObject::connect(this, SIGNAL(startLoopback()), this, SLOT(startMediaLoopback()));
    QObject::connect(this, SIGNAL(initEncode(const QFileInfo &, const GekkoFyre::Database::Settings::Audio::GkDevice &, const qint32 &, const GekkoFyre::GkAudioFramework::CodecSupport &, const qint32 &, const qint32 &)),
                     gkAudioEncoding, SLOT(startCaller(const QFileInfo &, const GekkoFyre::Database::Settings::Audio::GkDevice &, const qint32 &, const GekkoFyre::GkAudioFramework::CodecSupport &,
                     const qint32 &, const qint32 &)));
    return;
}

GkPaStreamHandler::~GkPaStreamHandler()
{
    if (procMediaEventLoop) {
        if (procMediaEventLoop->isRunning()) {
            procMediaEventLoop->quit();
        }
    }

    return;
}

/**
 * @brief GkPaStreamHandler::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param mediaFilePath
 * @param supported_codec
 * @param loop_media
 * @param encode_bitrate The bitrate to use when encoding/recording an audio signal, with a format like Opus or Ogg
 * Vorbis, for example. This is only used for recording purposes at the moment.
 */
void GkPaStreamHandler::processEvent(GkAudioFramework::AudioEventType audioEventType, const QFileInfo &mediaFilePath,
                                     const CodecSupport &supported_codec, bool loop_media, qint32 encode_bitrate)
{
    Q_UNUSED(loop_media);
    try {
        switch (audioEventType) {
            case GkAudioFramework::AudioEventType::start:
                {
                    if (mediaFilePath.isReadable()) {
                        gkSounds.insert(mediaFilePath.path(), *gkAudioFile);
                        emit playMedia(mediaFilePath, supported_codec);
                    }
                }

                break;
            case GkAudioFramework::AudioEventType::record:
                if (mediaFilePath.isReadable()) {
                    emit recordMedia(mediaFilePath, supported_codec, encode_bitrate);
                }

                break;
            case GkAudioFramework::AudioEventType::loopback:
                emit startLoopback();
                break;
            case GkAudioFramework::AudioEventType::stop:
                if (mediaFilePath.isReadable()) {
                    emit stopMedia(mediaFilePath);
                }

                break;
            default:
                break;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(e.what());
    }

    return;
}

/**
 * @brief GkPaStreamHandler::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param mediaFilePath
 * @param supported_codec
 * @param loop_media
 * @param encode_bitrate The bitrate to use when encoding/recording an audio signal, with a format like Opus or Ogg
 * Vorbis, for example. This is only used for recording purposes at the moment.
 */
void GkPaStreamHandler::processEvent(GkAudioFramework::AudioEventType audioEventType, const QDir &mediaFilePath,
                                     const CodecSupport &supported_codec, bool loop_media, qint32 encode_bitrate)
{
    Q_UNUSED(loop_media);
    try {
        switch (audioEventType) {
            case GkAudioFramework::AudioEventType::start:
            {
                if (mediaFilePath.isReadable()) {
                    gkSounds.insert(mediaFilePath.path(), *gkAudioFile);
                    emit playMedia(mediaFilePath, supported_codec);
                }
            }

                break;
            case GkAudioFramework::AudioEventType::record:
                if (mediaFilePath.isReadable()) {
                    emit recordMedia(mediaFilePath, supported_codec, encode_bitrate);
                }

                break;
            case GkAudioFramework::AudioEventType::loopback:
                emit startLoopback();
                break;
            case GkAudioFramework::AudioEventType::stop:
                if (mediaFilePath.isReadable()) {
                    emit stopMedia(mediaFilePath);
                }

                break;
            default:
                break;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(e.what());
    }

    return;
}

/**
 * @brief GkPaStreamHandler::playMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @param supported_codec
 */
void GkPaStreamHandler::playMediaFile(const QFileInfo &media_path, const GkAudioFramework::CodecSupport &supported_codec)
{
    playMediaFileHelper(media_path, supported_codec);
    return;
}

/**
 * @brief GkPaStreamHandler::playMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @param supported_codec
 */
void GkPaStreamHandler::playMediaFile(const QDir &media_path, const CodecSupport &supported_codec)
{
    const QFileInfo file_info_tmp(media_path.path());
    playMediaFileHelper(file_info_tmp, supported_codec);

    return;
}

/**
 * @brief GkPaStreamHandler::recordMediaFile record data to a multimedia file on the end-user's storage media of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path The multimedia file to make the recording towards.
 * @param supported_codec The codec you would like to make the recording with.
 * @param encoding_bitrate The bitrate at which to encode with (i.e. 192 kbit/sec).
 */
void GkPaStreamHandler::recordMediaFile(const QFileInfo &media_path, const GkAudioFramework::CodecSupport &supported_codec,
                                        qint32 encoding_bitrate)
{
    recordMediaFileHelper(media_path, supported_codec, encoding_bitrate);
    return;
}

/**
 * @brief GkPaStreamHandler::recordMediaFile record data to a multimedia file on the end-user's storage media of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path The multimedia file to make the recording towards.
 * @param supported_codec The codec you would like to make the recording with.
 * @param encoding_bitrate The bitrate at which to encode with (i.e. 192 kbit/sec).
 */
void GkPaStreamHandler::recordMediaFile(const QDir &media_path, const CodecSupport &supported_codec,
                                        qint32 encoding_bitrate)
{
    const QFileInfo file_info_tmp(media_path.path());
    recordMediaFileHelper(file_info_tmp, supported_codec, encoding_bitrate);

    return;
}

/**
 * @brief GkPaStreamHandler::startMediaLoopback
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkPaStreamHandler::startMediaLoopback()
{
    QPointer<QBuffer> read_buf = new QBuffer(this);
    QPointer<QBuffer> write_buf = new QBuffer(this);

    read_buf->open(QBuffer::ReadOnly);
    write_buf->open(QBuffer::WriteOnly);

    QObject::connect(write_buf, &QIODevice::bytesWritten, [write_buf, read_buf](qint64) {
        // Remove all data that was already read
        read_buf->buffer().remove(0, read_buf->pos());

        // Set the pointer towards the beginning of the unread data
        const auto res = read_buf->seek(0);
        assert(res);

        // Write out any new data
        read_buf->buffer().append(write_buf->buffer());

        // Remove all data that has already been written
        write_buf->buffer().clear();
        write_buf->seek(0);
    });

    gkAudioInput->start(write_buf);
    gkAudioOutput->start(read_buf);

    return;
}

/**
 * @brief GkPaStreamHandler::stopMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 */
void GkPaStreamHandler::stopMediaFile(const QFileInfo &media_path)
{
    for (auto media = gkSounds.begin(); media != gkSounds.end(); ++media) {
        if (media.key() == media_path.path()) {
            gkPcmFileStream->stop();

            if (gkAudioOutput->state() == QAudio::ActiveState) {
                gkAudioOutput->stop();
            }

            break;
        }
    }

    gkSounds.remove(media_path.path()); // Must be deleted outside of the loop!
}

/*
 * @brief GkPaStreamHandler::playbackHandleStateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkPaStreamHandler::playbackHandleStateChanged(QAudio::State changed_state)
{
    try {
        switch (changed_state) {
            case QAudio::IdleState:
                gkPcmFileStream->stop();

                break;
            case QAudio::StoppedState:
                if (gkAudioOutput->error() != QAudio::NoError) { // TODO: Improve the error reporting functionality of this statement!
                    throw std::runtime_error(tr("Issue with 'QAudio::StoppedState' during media playback!").toStdString());
                }

                break;
            default:
                break;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An issue has been encountered during audio playback. Error:\n\n%1").arg(QString::fromStdString(e.what())),
                                    GkSeverity::Fatal, "", true, true, false, false);
    }

    return;
}

/**
 * @brief GkPaStreamHandler::recordingHandleStateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param changed_state
 */
void GkPaStreamHandler::recordingHandleStateChanged(QAudio::State changed_state)
{
    try {
        switch (changed_state) {
            case QAudio::IdleState:
                gkAudioInput->stop();

                break;
            case QAudio::StoppedState:
                if (gkAudioInput->error() != QAudio::NoError) { // TODO: Improve the error reporting functionality of this statement!
                    throw std::runtime_error(tr("Issue with 'QAudio::StoppedState' during media recording!").toStdString());
                }

                break;
            default:
                break;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An issue has been encountered during audio recording. Error:\n\n%1").arg(QString::fromStdString(e.what())),
                                    GkSeverity::Fatal, "", true, true, false, false);
    }

    return;
}

/**
 * @brief GkPaStreamHandler::createRecordMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path The path to where the file is stored.
 * @param supported_codec The file extension to use.
 * @return
 */
QFileInfo GkPaStreamHandler::createRecordMediaFile(const QFileInfo &media_path, const CodecSupport &supported_codec)
{
    const auto randStr = gkDb->createRandomString(16);
    const auto epochSecs = QDateTime::currentSecsSinceEpoch();
    const auto extension = gkDb->convCodecFormatToFileExtension(supported_codec);
    const QFileInfo mediaRetPath = QFileInfo(media_path.path() + "/" + QString::number(epochSecs) + "_" + QString::fromStdString(randStr) + extension);

    return mediaRetPath;
}

/**
 * @brief GkPaStreamHandler::createRecordMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path The path to where the file is stored.
 * @param supported_codec The file extension to use.
 * @return
 */
QFileInfo GkPaStreamHandler::createRecordMediaFile(const QDir &media_path, const CodecSupport &supported_codec)
{
    const auto randStr = gkDb->createRandomString(16);
    const auto epochSecs = QDateTime::currentSecsSinceEpoch();
    const auto extension = gkDb->convCodecFormatToFileExtension(supported_codec);
    const QFileInfo mediaRetPath = QFileInfo(media_path.path() + "/" + QString::number(epochSecs) + "_" + QString::fromStdString(randStr) + extension);

    return mediaRetPath;
}

/**
 * @brief GkPaStreamHandler::playMediaFileHelper is a helper function for the playing of multimedia files (i.e. namely
 * audio files such as Ogg Vorbis, Ogg Opus, FLAC, PCM WAV, etc.).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @param supported_codec
 */
void GkPaStreamHandler::playMediaFileHelper(const QFileInfo &media_path, const CodecSupport &supported_codec)
{
    try {
        if (gkAudioOutput.isNull()) {
            throw std::runtime_error(tr("A memory error has been encountered whilst trying to playback the audio file, \"%1\"!")
                                             .arg(media_path.path()).toStdString());
        }

        for (auto media = gkSounds.begin(); media != gkSounds.end(); ++media) {
            if (media.key() == media_path.path()) {
                if (!gkPcmFileStream->init(pref_output_device.audio_device_info.preferredFormat())) {
                    throw std::runtime_error(tr("Error whilst initializing output audio stream!").toStdString());
                }
            }

            switch (supported_codec) {
                case GkAudioFramework::CodecSupport::PCM:
                    gkPcmFileStream->play(media.key());
                    gkAudioOutput->start(gkPcmFileStream.get());

                    break;
                case GkAudioFramework::CodecSupport::Loopback:
                    return;
                case GkAudioFramework::CodecSupport::OggVorbis:
                    break;
                case GkAudioFramework::CodecSupport::Opus:
                    break;
                case GkAudioFramework::CodecSupport::FLAC:
                    break;
                case GkAudioFramework::CodecSupport::Unsupported:
                    return;
                default:
                    return;
            }

            procMediaEventLoop = new QEventLoop(this);
            do {
                procMediaEventLoop->exec(QEventLoop::WaitForMoreEvents);
            } while (gkAudioOutput->state() == QAudio::ActiveState);

            delete procMediaEventLoop;
            emit stopMedia(media_path);
            break;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkPaStreamHandler::recordMediaFile record data to a multimedia file on the end-user's storage media of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path The multimedia file to make the recording towards.
 * @param supported_codec The codec you would like to make the recording with.
 * @param encoding_bitrate The bitrate at which to encode with (i.e. 192 kbit/sec).
 */
void GkPaStreamHandler::recordMediaFileHelper(const QFileInfo &media_path, const CodecSupport &supported_codec,
                                              qint32 encoding_bitrate)
{
    try {
        if (encoding_bitrate >= 8) {
            if (pref_input_device.is_enabled) {
                //
                // Make use of the Input Audio Device!
                if (supported_codec == CodecSupport::Opus) {
                    //
                    // Create a media file with a randomized name within the given path, for recording purposes!
                    m_mediaFile = createRecordMediaFile(media_path, CodecSupport::Opus);

                    //
                    // Use a differing FRAME SIZE for Opus!
                    emit initEncode(m_mediaFile, pref_input_device, encoding_bitrate, supported_codec, AUDIO_OPUS_MAX_FRAME_SIZE);
                } else {
                    //
                    // Create a media file with a randomized name within the given path, for recording purposes!
                    m_mediaFile = createRecordMediaFile(media_path, supported_codec);

                    //
                    // PCM, Ogg Vorbis, etc.
                    emit initEncode(m_mediaFile, pref_input_device, encoding_bitrate, supported_codec, AUDIO_FRAMES_PER_BUFFER);
                }

                procMediaEventLoop = new QEventLoop(this);
                do {
                    procMediaEventLoop->exec(QEventLoop::WaitForMoreEvents);
                } while (gkAudioInput->state() == QAudio::ActiveState);
                delete procMediaEventLoop;
            } else if (pref_output_device.is_enabled) {
                //
                // Make use of the Output Audio Device!
                if (supported_codec == CodecSupport::Opus) {
                    //
                    // Create a media file with a randomized name within the given path, for recording purposes!
                    m_mediaFile = createRecordMediaFile(media_path, CodecSupport::Opus);

                    //
                    // Use a differing FRAME SIZE for Opus!
                    emit initEncode(m_mediaFile, pref_output_device, encoding_bitrate, supported_codec, AUDIO_OPUS_MAX_FRAME_SIZE);
                } else {
                    //
                    // Create a media file with a randomized name within the given path, for recording purposes!
                    m_mediaFile = createRecordMediaFile(media_path, supported_codec);

                    //
                    // PCM, Ogg Vorbis, etc.
                    emit initEncode(m_mediaFile, pref_output_device, encoding_bitrate, supported_codec, AUDIO_FRAMES_PER_BUFFER);
                }

                procMediaEventLoop = new QEventLoop(this);
                do {
                    procMediaEventLoop->exec(QEventLoop::WaitForMoreEvents);
                } while (gkAudioInput->state() == QAudio::ActiveState);
                delete procMediaEventLoop;
            } else {
                //
                // There is no audio device that has been chosen!
                throw std::invalid_argument(tr("You must configure a suitable audio device within the Settings before continuing!").toStdString());
            }
        } else {
            throw std::invalid_argument(tr("An invalid encoding bitrate has been given! A value of %1 kBps cannot be used.")
                                                .arg(QString::number(encoding_bitrate)).toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }
}
