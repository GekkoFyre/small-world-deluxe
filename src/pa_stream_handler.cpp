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
#include <QEventLoop>
#include <QByteArray>
#include <QIODevice>
#include <QBuffer>

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
                                     QPointer<GekkoFyre::GkEventLogger> eventLogger, std::shared_ptr<AudioFile<double>> audioFileLib,
                                     QObject *parent) : QThread(parent)
{
    setParent(parent);

    gkDb = std::move(database);
    gkEventLogger = std::move(eventLogger);
    gkAudioOutput = std::move(audioOutput);
    gkAudioFile = std::move(audioFileLib);

    //
    // Initialize variables
    //
    pref_output_device = output_device;
    gkPcmFileStream = std::make_unique<GkPcmFileStream>(this);

    QObject::connect(this, SIGNAL(playMedia(const boost::filesystem::path &)), this, SLOT(playMediaFile(const boost::filesystem::path &)));
    QObject::connect(this, SIGNAL(stopMedia(const boost::filesystem::path &)), this, SLOT(stopMediaFile(const boost::filesystem::path &)));

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
 * @param audioFile
 * @param loop
 * @see Ben Key <https://stackoverflow.com/questions/29249657/playing-wav-file-with-portaudio-and-sndfile>.
 */
void GkPaStreamHandler::processEvent(AudioEventType audioEventType, const fs::path &mediaFilePath, bool loop)
{
    try {
        switch (audioEventType) {
            case AudioEventType::start:
                {
                    gkSounds.insert(std::make_pair(mediaFilePath, *gkAudioFile));
                    emit playMedia(mediaFilePath);
                }

                break;
            case AudioEventType::stop:
                emit stopMedia(mediaFilePath);
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
    QObject::connect(gkAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    exec();
}

/**
 * @brief GkPaStreamHandler::playMediaFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @note alexisdm <https://stackoverflow.com/questions/10044211/how-to-use-qtmultimedia-to-play-a-wav-file>,
 * <https://github.com/sgpinkus/audio-trap/blob/master/docs/qt-audio.md>.
 */
void GkPaStreamHandler::playMediaFile(const boost::filesystem::path &media_path)
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

                gkPcmFileStream->play(QString::fromStdString(media_path.string()));
                gkAudioOutput->start(gkPcmFileStream.get());

                QEventLoop loop;
                QObject::connect(gkAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));

                do {
                    loop.exec();
                } while (gkAudioOutput->state() == QAudio::ActiveState);

                emit stopMedia(media_path);
                break;
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Error, true, true, false, false);
    }
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
            gkSounds.erase(media_path);

            break;
        }
    }
}

/*
 * @brief GkPaStreamHandler::handleStateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkPaStreamHandler::handleStateChanged(const QAudio::State changed_state)
{
    switch (changed_state) {
        case QAudio::IdleState:
            gkAudioOutput->stop();

            break;
        case QAudio::StoppedState:
            if (gkAudioOutput->error() != QAudio::NoError) { // TODO: Improve the error reporting functionality of this statement!
                gkEventLogger->publishEvent(tr("An error has been encountered during audio playback."), GkSeverity::Error, "",
                                            true, true, false, false);
            }

            break;
        default:
            break;
    }

    return;
}
