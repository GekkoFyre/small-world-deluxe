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
 **   Small world is distributed in the hope that it will be useful,
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

#include "src/gk_fft_audio.hpp"
#include <utility>

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
 * @brief GkFFTAudio::GkFFTAudio
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkFFTAudio::GkFFTAudio(QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput, const GkDevice &input_audio_device_details,
                       const GkDevice &output_audio_device_details, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                       QObject *parent) : QThread(parent)
{
    setParent(parent);

    //
    // Preferred multimedia settings and information for audio devices, as made and
    // configured by the end-user themselves (unless SWD has been started for the
    // first time and we are using the default, 'best guess' settings).
    //
    pref_input_audio_device = input_audio_device_details;
    pref_output_audio_device = output_audio_device_details;

    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    eventLogger = std::move(gkEventLogger);

    gkFftPcmStream = new GkFFTAudioPcmStream(gkAudioInput, pref_input_audio_device, this);
    start();

    // Move event processing of GkPaStreamHandler to this thread
    QObject::moveToThread(this);

    return;
}

GkFFTAudio::~GkFFTAudio()
{
    quit();
    wait();
}

/**
 * @brief GkFFTAudio::run
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudio::run()
{
    QObject::connect(gkAudioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioInHandleStateChanged(QAudio::State)));
    QObject::connect(gkAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioOutHandleStateChanged(QAudio::State)));
    QObject::connect(this, SIGNAL(recordStream(const fs::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)),
                     this, SLOT(recordAudioStream(const fs::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)));
    QObject::connect(this, SIGNAL(stopRecording(const fs::path &)), this, SLOT(stopRecordStream(const fs::path &)));

    exec();
    return;
}

/**
 * @brief GkFFTAudio::audioInHandleStateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param changed_state
 */
void GkFFTAudio::audioInHandleStateChanged(QAudio::State changed_state)
{
    try {
        switch (changed_state) {
            case QAudio::IdleState:
                gkFftPcmStream->stop();

                break;
            case QAudio::StoppedState:
                if (gkAudioInput->error() != QAudio::NoError) { // TODO: Improve the error reporting functionality of this statement!
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
 * @brief GkFFTAudio::audioOutHandleStateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param changed_state
 */
void GkFFTAudio::audioOutHandleStateChanged(QAudio::State changed_state)
{
    try {
        switch (changed_state) {
            case QAudio::IdleState:
                gkAudioOutput->stop();

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
 * @brief GkFFTAudio::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param mediaFilePath
 * @param supported_codec
 */
void GkFFTAudio::processEvent(Spectrograph::GkFftEventType audioEventType, const fs::path &mediaFilePath,
                              const GkAudioFramework::CodecSupport &supported_codec)
{
    try {
        switch (audioEventType) {
            case Spectrograph::GkFftEventType::record:
                if (!mediaFilePath.empty()) {
                    emit recordStream(mediaFilePath, supported_codec);
                }

                break;
            case Spectrograph::GkFftEventType::stop:
                if (!mediaFilePath.empty()) {
                    emit stopRecording(mediaFilePath);
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
 * @brief GkFFTAudio::recordAudioStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @param supported_codec
 */
void GkFFTAudio::recordAudioStream(const fs::path &media_path, const GkAudioFramework::CodecSupport &supported_codec)
{
    try {
        if (gkAudioInput.isNull()) {
            throw std::runtime_error(tr("A memory error has been encountered whilst trying to process audio for FFT calculations!").toStdString());
        }

        for (const auto &media: gkProcMedia) {
            if (media.first == media_path) {
                QPointer<GkFFTAudioPcmStream> fft_ptr = const_cast<GkFFTAudioPcmStream*>(&media.second);
                fft_ptr->play(QString::fromStdString(media_path.string()));

                break;
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, true, true, false, false);
    }

    return;
}

/**
 * @brief GkFFTAudio::stopRecordStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 */
void GkFFTAudio::stopRecordStream(const fs::path &media_path)
{
    for (const auto &media: gkProcMedia) {
        if (media.first == media_path) {
            QPointer<GkFFTAudioPcmStream> fft_ptr = const_cast<GkFFTAudioPcmStream*>(&media.second);
            fft_ptr->stop();
            break;
        }
    }

    gkProcMedia.erase(media_path); // Must be deleted outside of the loop!
    return;
}
