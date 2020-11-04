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

#include "src/pa_stream_handler.hpp"
#include <utility>
#include <algorithm>
#include <exception>
#include <QByteArray>
#include <QIODevice>
#include <QBuffer>
#include <QDataStream>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

/**
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://qtsource.wordpress.com/2011/09/12/multithreaded-audio-using-qaudiooutput/>.
 */
GkPaStreamHandler::GkPaStreamHandler(QPointer<GekkoFyre::GkLevelDb> database, const GkDevice &output_device, QPointer<QAudioOutput> audioOutput,
                                     QPointer<QAudioInput> audioInput, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                     std::shared_ptr<AudioFile<double>> audioFileLib, QObject *parent) : QThread(parent)
{
    setParent(parent);

    gkDb = std::move(database);
    gkEventLogger = std::move(eventLogger);
    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    gkAudioFile = std::move(audioFileLib);

    //
    // Initialize variables
    //
    pref_output_device = output_device;
    gkPcmFileStream = std::make_unique<GkPcmFileStream>(this);

    QObject::connect(this, SIGNAL(playMedia(const boost::filesystem::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)),
                     this, SLOT(playMediaFile(const boost::filesystem::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)));
    QObject::connect(this, SIGNAL(stopMedia(const boost::filesystem::path &)), this, SLOT(stopMediaFile(const boost::filesystem::path &)));
    QObject::connect(this, SIGNAL(recordMedia(const boost::filesystem::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)),
                     this, SLOT(recordMediaFile(const boost::filesystem::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)));
    QObject::connect(this, SIGNAL(startLoopback()), this, SLOT(startMediaLoopback()));
    QObject::connect(this, SIGNAL(changePlaybackState(QAudio::State)), this, SLOT(playbackHandleStateChanged(QAudio::State)));
    QObject::connect(this, SIGNAL(changeRecorderState(QAudio::State)), this, SLOT(recordingHandleStateChanged(QAudio::State)));

    start();

    // Move event processing of GkPaStreamHandler to this thread
    QObject::moveToThread(this);

    return;
}

GkPaStreamHandler::~GkPaStreamHandler()
{
    quit();
    wait();
}

/**
 * @brief GkPaStreamHandler::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param supported_codec
 * @param mediaFilePath
 * @param loop_media
 */
void GkPaStreamHandler::processEvent(GkAudioFramework::AudioEventType audioEventType, const fs::path &mediaFilePath,
                                     const GkAudioFramework::CodecSupport &supported_codec, bool loop_media)
{
    Q_UNUSED(loop_media);
    try {
        switch (audioEventType) {
            case GkAudioFramework::AudioEventType::start:
                {
                    if (!mediaFilePath.empty()) {
                        gkSounds.insert(std::make_pair(mediaFilePath, *gkAudioFile));
                        emit playMedia(mediaFilePath, supported_codec);
                    }
                }

                break;
            case GkAudioFramework::AudioEventType::record:
                if (!mediaFilePath.empty()) {
                    emit recordMedia(mediaFilePath, supported_codec);
                }

                break;
            case GkAudioFramework::AudioEventType::loopback:
                emit startLoopback();
                break;
            case GkAudioFramework::AudioEventType::stop:
                if (!mediaFilePath.empty()) {
                    emit stopMedia(mediaFilePath);
                }

                break;
            default:
                break;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Error, true, true, false, false);
    }

    return;
}

/**
 * @brief GkPaStreamHandler::run
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkPaStreamHandler::run()
{
    QObject::connect(gkAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(playbackHandleStateChanged(QAudio::State)));
    QObject::connect(gkAudioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(recordingHandleStateChanged(QAudio::State)));

    exec();
    return;
}

/**
 * @brief GkPaStreamHandler::playMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @note alexisdm <https://stackoverflow.com/questions/10044211/how-to-use-qtmultimedia-to-play-a-wav-file>,
 * <https://github.com/sgpinkus/audio-trap/blob/master/docs/qt-audio.md>.
 */
void GkPaStreamHandler::playMediaFile(const boost::filesystem::path &media_path, const GkAudioFramework::CodecSupport &supported_codec)
{
    try {
        if (!gkAudioOutput) {
            throw std::runtime_error(tr("A memory error has been encountered whilst trying to playback audio file!").toStdString());
        }

        for (const auto &media: gkSounds) {
            if (media.first == media_path) {
                if (!gkPcmFileStream->init(pref_output_device.audio_device_info.preferredFormat())) {
                    throw std::runtime_error(tr("Error whilst initializing output audio stream!").toStdString());
                }

                switch (supported_codec) {
                    case GkAudioFramework::CodecSupport::PCM:
                        gkPcmFileStream->play(QString::fromStdString(media_path.string()));
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

                loop = new QEventLoop(this);
                do {
                    loop->exec(QEventLoop::WaitForMoreEvents);
                } while (gkAudioOutput->state() == QAudio::ActiveState);
                delete loop;

                QObject::connect(gkAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(playbackHandleStateChanged(QAudio::State)));
                emit stopMedia(media_path);
                break;
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Error, true, true, false, false);
    }
}

/**
 * @brief GkPaStreamHandler::recordMediaFile record data to a multimedia file on the end-user's storage media of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path The multimedia file to make the recording towards.
 * @param supported_codec The codec you would like to make the recording with.
 */
void GkPaStreamHandler::recordMediaFile(const boost::filesystem::path &media_path, const GkAudioFramework::CodecSupport &supported_codec)
{
    record_input_buf.clear(); // Clear the pointer just in-case it has been used previously!
    record_input_buf = new QBuffer(this);
    gkAudioInput->start(record_input_buf);
    QFile write_file(QString::fromStdString(media_path.string()));

    switch (supported_codec) {
        case GkAudioFramework::CodecSupport::PCM:
            write_file.write(record_input_buf->buffer(), QIODevice::WriteOnly);
            write_file.flush();

            break;
        case GkAudioFramework::CodecSupport::Loopback:
            write_file.write(record_input_buf->buffer(), QIODevice::WriteOnly);
            write_file.flush();

            break;
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

    loop = new QEventLoop(this);
    do {
        loop->exec(QEventLoop::WaitForMoreEvents);
    } while (gkAudioInput->state() == QAudio::ActiveState);
    delete loop;

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
void GkPaStreamHandler::stopMediaFile(const boost::filesystem::path &media_path)
{
    for (const auto &media: gkSounds) {
        if (media.first == media_path) {
            gkPcmFileStream->stop();
            gkSounds.erase(media_path);

            if (gkAudioOutput->state() == QAudio::ActiveState) {
                gkAudioOutput->stop();
            }

            break;
        }
    }
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
                                    GkSeverity::Error, "", true, true, false, false);
    }

    return;
}

void GkPaStreamHandler::recordingHandleStateChanged(QAudio::State changed_state)
{
    try {
        switch (changed_state) {
            case QAudio::IdleState:
                gkAudioInput->stop();

                break;
            case QAudio::StoppedState:
                if (gkAudioOutput->error() != QAudio::NoError) { // TODO: Improve the error reporting functionality of this statement!
                    throw std::runtime_error(tr("Issue with 'QAudio::StoppedState' during media recording!").toStdString());
                }

                break;
            default:
                break;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An issue has been encountered during audio recording. Error:\n\n%1").arg(QString::fromStdString(e.what())),
                                    GkSeverity::Error, "", true, true, false, false);
    }

    return;
}
