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
#include <exception>
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
GkPaStreamHandler::GkPaStreamHandler(QPointer<GekkoFyre::GkLevelDb> database, QPointer<QAudioOutput> audioOutput,
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
    gkPcmFileStream = std::make_unique<GkPcmFileStream>(this);

    QObject::connect(gkAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(playbackHandleStateChanged(QAudio::State)));
    QObject::connect(gkAudioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(recordingHandleStateChanged(QAudio::State)));

    //
    // Playing of multimedia files
    QObject::connect(this, SIGNAL(playMedia(const QFileInfo &, const GekkoFyre::GkAudioFramework::CodecSupport &, const GekkoFyre::Database::Settings::GkAudioSource &)),
                     this, SLOT(playMediaFile(const QFileInfo &, const GekkoFyre::GkAudioFramework::CodecSupport &, const GekkoFyre::Database::Settings::GkAudioSource &)));
    QObject::connect(this, SIGNAL(playMedia(const QDir &, const GekkoFyre::GkAudioFramework::CodecSupport &, const GekkoFyre::Database::Settings::GkAudioSource &)),
                     this, SLOT(playMediaFile(const QDir &, const GekkoFyre::GkAudioFramework::CodecSupport &, const GekkoFyre::Database::Settings::GkAudioSource &)));

    //
    // Stopping of either playing or recording of multimedia files
    QObject::connect(this, SIGNAL(stopMedia(const QFileInfo &)), this, SLOT(stopMediaFile(const QFileInfo &)));
    QObject::connect(this, SIGNAL(stopMedia(const QDir &)), this, SLOT(stopMediaFile(const QDir &)));
    QObject::connect(this, SIGNAL(stopRecording(const QFileInfo &)), this, SLOT(stopRecordingFile(const QFileInfo &)));
    QObject::connect(this, SIGNAL(stopRecording(const QDir &)), this, SLOT(stopRecordingFile(const QDir &)));

    //
    // Recording of multimedia files
    QObject::connect(this, SIGNAL(recordMedia(const QDir &, const GekkoFyre::GkAudioFramework::CodecSupport &, const GekkoFyre::Database::Settings::GkAudioSource &, qint32)),
                     this, SLOT(recordMediaFile(const QDir &, const GekkoFyre::GkAudioFramework::CodecSupport &, const GekkoFyre::Database::Settings::GkAudioSource &, qint32)));

    //
    // Changing the playback state of multimedia files (i.e. for recording, playing, etc.)
    QObject::connect(this, SIGNAL(changePlaybackState(QAudio::State)), this, SLOT(playbackHandleStateChanged(QAudio::State)));
    QObject::connect(this, SIGNAL(changeRecorderState(QAudio::State)), this, SLOT(recordingHandleStateChanged(QAudio::State)));

    //
    // Miscellaneous
    QObject::connect(this, SIGNAL(startLoopback()), this, SLOT(startMediaLoopback()));
    QObject::connect(this, SIGNAL(initEncode(const QFileInfo &, const qint32 &, const GekkoFyre::GkAudioFramework::CodecSupport &, const GekkoFyre::Database::Settings::GkAudioSource &, const qint32 &, const qint32 &)),
                     gkAudioEncoding, SLOT(startCaller(const QFileInfo &, const qint32 &, const GekkoFyre::GkAudioFramework::CodecSupport &, const GekkoFyre::Database::Settings::GkAudioSource &, const qint32 &, const qint32 &)));

    return;
}

GkPaStreamHandler::~GkPaStreamHandler()
{
    return;
}

/**
 * @brief GkPaStreamHandler::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param audio_device The audio device to record from, which is either the input or output, or even a mix of the
 * aforementioned two.
 * @param mediaFilePath
 * @param supported_codec
 * @param loop_media
 * @param encode_bitrate The bitrate to use when encoding/recording an audio signal, with a format like Opus or Ogg
 * Vorbis, for example. This is only used for recording purposes at the moment.
 */
void GkPaStreamHandler::processEvent(GkAudioFramework::AudioEventType audioEventType, const GkAudioSource &audio_source,
                                     const QFileInfo &mediaFilePath, const CodecSupport &supported_codec,
                                     bool loop_media, qint32 encode_bitrate)
{
    Q_UNUSED(loop_media);
    try {
        switch (audioEventType) {
            case GkAudioFramework::AudioEventType::start:
                {
                    if (mediaFilePath.isReadable()) {
                        gkSounds.insert(mediaFilePath.path(), *gkAudioFile);
                        emit playMedia(mediaFilePath, supported_codec, audio_source);
                    }
                }

                break;
            case GkAudioFramework::AudioEventType::record:
                //
                // NOTE: Only handled with a QDir-enabled function!

                break;
            case GkAudioFramework::AudioEventType::loopback:
                emit startLoopback();
                break;
            case GkAudioFramework::AudioEventType::stop:
                if (mediaFilePath.exists()) {
                    for (const auto &file: gkAudioEvents.toStdMap()) {
                        switch (file.first) {
                            case GkAudioFramework::AudioEventType::start:
                                if (mediaFilePath == file.second) { // Path to file that is being played!
                                    emit stopMedia(mediaFilePath); // TODO: Finish coding this!
                                }

                                break;
                            case GkAudioFramework::AudioEventType::record:
                                emit stopRecording(mediaFilePath);
                                break;
                            case GkAudioFramework::AudioEventType::loopback:
                                if (mediaFilePath == file.second) {
                                    emit stopLoopback(mediaFilePath); // TODO: Finish coding this!
                                }

                                break;
                            default:
                                break;
                        }
                    }
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
 * @param audio_source The audio device to record from, which is either the input or output, or even a mix of the
 * aforementioned two.
 * @param mediaFilePath
 * @param supported_codec
 * @param loop_media
 * @param encode_bitrate The bitrate to use when encoding/recording an audio signal, with a format like Opus or Ogg
 * Vorbis, for example. This is only used for recording purposes at the moment.
 */
void GkPaStreamHandler::processEvent(GkAudioFramework::AudioEventType audioEventType, const GkAudioSource &audio_source,
                                     const QDir &mediaFilePath, const CodecSupport &supported_codec, bool loop_media,
                                     qint32 encode_bitrate)
{
    Q_UNUSED(loop_media);
    try {
        switch (audioEventType) {
            case GkAudioFramework::AudioEventType::start:
            {
                if (mediaFilePath.isReadable()) {
                    gkSounds.insert(mediaFilePath.path(), *gkAudioFile);
                    emit playMedia(mediaFilePath, supported_codec, audio_source);
                }
            }

                break;
            case GkAudioFramework::AudioEventType::record:
                if (mediaFilePath.isReadable()) {
                    emit recordMedia(mediaFilePath, supported_codec, audio_source, encode_bitrate);
                }

                break;
            case GkAudioFramework::AudioEventType::loopback:
            {
                emit startLoopback();
            }

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
void GkPaStreamHandler::playMediaFile(const QFileInfo &media_path, const GkAudioFramework::CodecSupport &supported_codec,
                                      const GkAudioSource &audio_source)
{
    try {
        playMediaFileHelper(media_path, supported_codec, audio_source);
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkPaStreamHandler::playMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @param supported_codec
 */
void GkPaStreamHandler::playMediaFile(const QDir &media_path, const CodecSupport &supported_codec,
                                      const GkAudioSource &audio_source)
{
    try {
        const QFileInfo file_info_tmp(media_path.path());
        playMediaFileHelper(file_info_tmp, supported_codec, audio_source);
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
 * @param audio_source The audio source in question, whether it is input, output, or a mix of the two.
 * @param encoding_bitrate The bitrate at which to encode with (i.e. 192 kbit/sec).
 */
void GkPaStreamHandler::recordMediaFile(const QDir &media_path, const CodecSupport &supported_codec,
                                        const GkAudioSource &audio_source, qint32 encoding_bitrate)
{
    try {
        if (audio_source == GkAudioSource::Input) {
            if (!gkAudioInput.isNull()) {
                recordMediaFileHelper(media_path, supported_codec, audio_source, encoding_bitrate);
            } else {
                throw std::invalid_argument(tr("You must configure a suitable audio device within the Settings before continuing!").toStdString());
            }
        } else if (audio_source == GkAudioSource::Output) {
            if (!gkAudioOutput.isNull()) {
                recordMediaFileHelper(media_path, supported_codec, audio_source, encoding_bitrate);
            } else {
                throw std::invalid_argument(tr("You must configure a suitable audio device within the Settings before continuing!").toStdString());
            }
        } else {
            throw std::invalid_argument(tr("An invalid or currently unsupported audio source has been chosen!").toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "",
                                    false, true, false, true, false);
    }

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
    return;
}

/**
 * @brief GkPaStreamHandler::stopMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 */
void GkPaStreamHandler::stopMediaFile(const QDir &media_path)
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
    return;
}

/**
 * @brief GkPaStreamHandler::stopRecordingFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 */
void GkPaStreamHandler::stopRecordingFile(const QFileInfo &media_path)
{
    if (gkAudioInput->state() == QAudio::ActiveState) {
        gkAudioEncoding->stopEncode();
        gkAudioInput->stop();
    }

    if (gkAudioOutput->state() == QAudio::ActiveState) {
        gkAudioEncoding->stopEncode();
        gkAudioOutput->stop();
    }

    gkAudioEvents.remove(GkAudioFramework::AudioEventType::record, media_path.absoluteFilePath());
    return;
}

/**
 * @brief GkPaStreamHandler::stopRecordingFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 */
void GkPaStreamHandler::stopRecordingFile(const QDir &media_path)
{
    if (gkAudioInput->state() == QAudio::ActiveState) {
        gkAudioEncoding->stopEncode();
        gkAudioInput->stop();
    }

    if (gkAudioOutput->state() == QAudio::ActiveState) {
        gkAudioEncoding->stopEncode();
        gkAudioOutput->stop();
    }

    gkAudioEvents.remove(GkAudioFramework::AudioEventType::record, media_path.absolutePath());
    return;
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
QFileInfo GkPaStreamHandler::createRecordMediaFile(const QDir &media_path, const CodecSupport &supported_codec)
{
    try {
        std::lock_guard<std::mutex> lock_guard(m_createRecordMediaFileDirMutex);
        const auto randStr = gkDb->createRandomString(16);
        const auto epochSecs = QDateTime::currentSecsSinceEpoch();
        const auto extension = gkDb->convCodecFormatToFileExtension(supported_codec);
        QFileInfo mediaRetPath;
        mediaRetPath.setFile(media_path.absolutePath() + "/" + QString::number(epochSecs) + "_" + QString::fromStdString(randStr) + extension);
        if (mediaRetPath.exists()) { // Retry and until we get to a folder name that does not already exist!
            mediaRetPath = createRecordMediaFile(media_path, supported_codec);
        }

        gkAudioEvents.insert(GkAudioFramework::AudioEventType::record, mediaRetPath.absoluteFilePath()); // Path to file that is being recorded!
        return mediaRetPath;
    } catch (const std::exception &e) {
        std::throw_with_nested(e.what());
    }

    return QFileInfo();
}

/**
 * @brief GkPaStreamHandler::playMediaFileHelper is a helper function for the playing of multimedia files (i.e. namely
 * audio files such as Ogg Vorbis, Ogg Opus, FLAC, PCM WAV, etc.).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @param supported_codec
 */
void GkPaStreamHandler::playMediaFileHelper(QFileInfo media_path, const CodecSupport &supported_codec,
                                            const GkAudioSource &audio_source)
{
    try {
        if (audio_source == GkAudioSource::Output) {
            if (gkAudioOutput.isNull()) {
                throw std::runtime_error(tr("A memory error has been encountered whilst trying to playback the audio file, \"%1\"!")
                                                 .arg(media_path.path()).toStdString());
            }

            for (auto media = gkSounds.begin(); media != gkSounds.end(); ++media) {
                if (media.key() == media_path.path()) {
                    if (!gkPcmFileStream->init(gkAudioOutput->format())) {
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

                emit stopMedia(media_path);
                break;
            }
        } else if (audio_source == GkAudioSource::Input) {
            if (gkAudioInput.isNull()) {
                throw std::runtime_error(tr("A memory error has been encountered whilst trying to playback the audio file, \"%1\"!")
                                                 .arg(media_path.path()).toStdString());
            }

            for (auto media = gkSounds.begin(); media != gkSounds.end(); ++media) {
                if (media.key() == media_path.path()) {
                    if (!gkPcmFileStream->init(gkAudioInput->format())) {
                        throw std::runtime_error(tr("Error whilst initializing output audio stream!").toStdString());
                    }
                }

                switch (supported_codec) {
                    case GkAudioFramework::CodecSupport::PCM:
                        gkPcmFileStream->play(media.key());
                        gkAudioInput->start(gkPcmFileStream.get());

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

                emit stopMedia(media_path);
                break;
            }
        } else {
            throw std::invalid_argument(tr("Invalid audio encoding codec specified! It is either not supported yet or an error was made.").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(e.what());
    }

    return;
}

/**
 * @brief GkPaStreamHandler::recordMediaFileHelper record data to a multimedia file on the end-user's storage media of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path The multimedia file to make the recording towards.
 * @param supported_codec The codec you would like to make the recording with.
 * @param audio_source The audio source in question, whether it is input, output, or a mix of the two.
 * @param encoding_bitrate The bitrate at which to encode with (i.e. 192 kbit/sec).
 */
void GkPaStreamHandler::recordMediaFileHelper(QDir media_path, const CodecSupport &supported_codec,
                                              const Settings::GkAudioSource &audio_source, qint32 encoding_bitrate)
{
    try {
        std::lock_guard<std::mutex> lock_guard(m_recordMediaFileHelperMutex);
        if (encoding_bitrate >= 8) {
            //
            // Make use of the Input Audio Device!
            if (supported_codec == CodecSupport::Opus) {
                //
                // Create a media file with a randomized name within the given path, for recording purposes!
                m_mediaFile = createRecordMediaFile(media_path, CodecSupport::Opus);

                //
                // Use a differing FRAME SIZE for Opus!
                // Ogg Opus only
                emit initEncode(m_mediaFile, encoding_bitrate, supported_codec, audio_source, AUDIO_OPUS_FRAMES_PER_BUFFER);
            } else {
                //
                // Create a media file with a randomized name within the given path, for recording purposes!
                m_mediaFile = createRecordMediaFile(media_path, supported_codec);

                //
                // PCM, Ogg Vorbis, etc.
                emit initEncode(m_mediaFile, encoding_bitrate, supported_codec, audio_source, AUDIO_FRAMES_PER_BUFFER);
            }
        } else {
            throw std::invalid_argument(tr("An invalid encoding bitrate has been given! A value of %1 kBps cannot be used.")
                                                .arg(QString::number(encoding_bitrate)).toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}
