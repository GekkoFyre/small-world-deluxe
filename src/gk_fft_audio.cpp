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

#define USE_KISS_FFT 1

#include "src/gk_fft_audio.hpp"
#include "src/contrib/Gist/src/Gist.h"
#include <chrono>
#include <utility>
#include <iterator>
#include <iostream>
#include <algorithm>

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
 * @note Joel Svensson <http://svenssonjoel.github.io/pages/qt-audio-fft/index.html>,
 * Paul R <https://stackoverflow.com/questions/4675457/how-to-generate-the-audio-spectrum-using-fft-in-c>.
 */
GkFFTAudio::GkFFTAudio(QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput, const GkDevice &input_audio_device_details,
                       const GkDevice &output_audio_device_details, QPointer<GekkoFyre::GkSpectroWaterfall> spectroWaterfall,
                       QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
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

    audioStreamProc = false;
    enableAudioStreamProc = false;
    audioFileStreamProc = false;
    enableAudioFileStreamProc = false;

    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    gkSpectroWaterfall = std::move(spectroWaterfall);
    gkStringFuncs = std::move(stringFuncs);
    eventLogger = std::move(gkEventLogger);

    gkAudioInSampleRate = pref_input_audio_device.audio_device_info.preferredFormat().sampleRate();
    gkAudioInNumSamples = (gkAudioInSampleRate * (SPECTRO_Y_AXIS_SIZE / 1000));
    gkAudioBuffer = new QBuffer(this);

    QObject::connect(gkAudioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioInHandleStateChanged(QAudio::State)));
    QObject::connect(gkAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioOutHandleStateChanged(QAudio::State)));

    QObject::connect(this, SIGNAL(recordStream()), this, SLOT(recordAudioStream()));
    QObject::connect(this, SIGNAL(stopRecording()), this, SLOT(stopRecordStream()));

    QObject::connect(this, SIGNAL(recordFileStream(const fs::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)),
                     this, SLOT(recordAudioFileStream(const fs::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)));
    QObject::connect(this, SIGNAL(stopRecordingFileStream(const fs::path &)), this, SLOT(stopRecordFileStream(const fs::path &)));

    QObject::connect(gkAudioInput, SIGNAL(notify()), this, SLOT(processAudioIn()));
    QObject::connect(this, SIGNAL(refreshGraph(bool)), gkSpectroWaterfall, SLOT(replot(bool)));

    spectroRefreshTimer = new QTimer(this);
    spectroRefreshTimer->setInterval(std::chrono::milliseconds(1000));
    QObject::connect(spectroRefreshTimer, SIGNAL(timeout()), this, SLOT(refreshGraphTrue()));
    QObject::connect(this, SIGNAL(stopRecording()), spectroRefreshTimer, SLOT(stop()));
    QObject::connect(this, SIGNAL(recordStream()), spectroRefreshTimer, SLOT(start()));

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
 * @brief GkFFTAudio::audioInHandleStateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param changed_state
 */
void GkFFTAudio::audioInHandleStateChanged(QAudio::State changed_state)
{
    try {
        switch (changed_state) {
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
 * @brief GkFFTAudio::processAudioIn
 * @author Joel Svensson <http://svenssonjoel.github.io/pages/qt-audio-fft/index.html>
 */
void GkFFTAudio::processAudioIn()
{
    if (gkAudioInput->state() == QAudio::ActiveState) {
        gkAudioBuffer->seek(0);
        QByteArray ba = gkAudioBuffer->readAll();

        qint32 ba_num_samples = ba.length() / 2;
        qint32 b_pos = 0;
        for (qint32 i = 0; i < ba_num_samples; ++i) {
            int16_t s;
            s = ba.at(b_pos++);
            s |= ba.at(b_pos++) << 8;
            if (s != 0) {
                audioSamples.push_back((double)s / gkStringFuncs->getPeakValue(pref_input_audio_device.audio_device_info.preferredFormat()));
            } else {
                audioSamples.push_back(0);
            }
        }

        gkAudioBuffer->buffer().clear();
        gkAudioBuffer->seek(0);

        samplesUpdated();
    } else {
        gkEventLogger->publishEvent(tr("Audio input, %1, has changed to an interrupted state.").arg(pref_input_audio_device.audio_device_info.deviceName()), GkSeverity::Info,
                                    "", true, true, false, false);
    }

    return;
}

void GkFFTAudio::refreshGraphTrue()
{
    emit refreshGraph(true);
    return;
}

void GkFFTAudio::setAudioIo(const bool &use_input_audio)
{
    try {
        if (use_input_audio) {
            if (!gkAudioInput.isNull()) {
                //
                // Input Audio
                //
                if (audioStreamProc) {
                    enableAudioStreamProc = true;
                }

                if (audioFileStreamProc) {
                    enableAudioFileStreamProc = true;
                }

                emit stopRecording();
                if (enableAudioStreamProc) {
                    emit recordStream();
                    enableAudioStreamProc = false;
                }

                // TODO: Setup an 'emit signal' for `enableAudioFileStreamProc`...
            }
        } else {
            if (!gkAudioOutput.isNull()) {
                //
                // Output Audio
                //
                if (audioStreamProc) {
                    enableAudioStreamProc = true;
                }

                if (audioFileStreamProc) {
                    enableAudioFileStreamProc = true;
                }

                emit stopRecording();
                if (enableAudioStreamProc) {
                    emit recordStream();
                    enableAudioStreamProc = false;
                }

                // TODO: Setup an 'emit signal' for `enableAudioFileStreamProc`...
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkFFTAudio::run
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudio::run()
{
    exec();
    return;
}

/**
 * @brief GkFFTAudio::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param supported_codec
 */
void GkFFTAudio::processEvent(Spectrograph::GkFftEventType audioEventType)
{
    switch (audioEventType) {
        case Spectrograph::GkFftEventType::record:
            emit recordStream();
            break;
        case Spectrograph::GkFftEventType::stop:
            emit stopRecording();
            break;
        default:
            break;
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
void GkFFTAudio::processEvent(Spectrograph::GkFftEventType audioEventType, const GkAudioFramework::CodecSupport &supported_codec,
                              const fs::path &mediaFilePath)
{
    switch (audioEventType) {
        case Spectrograph::GkFftEventType::record:
            if (!mediaFilePath.empty()) {
                emit recordFileStream(mediaFilePath, supported_codec);
                audioFileStreamProc = true;
            }

            break;
        case Spectrograph::GkFftEventType::stop:
            if (!mediaFilePath.empty()) {
                emit stopRecordingFileStream(mediaFilePath);
                audioFileStreamProc = false;
            }

            break;
        default:
            break;
    }

    return;
}

/**
 * @brief GkFFTAudio::recordAudioStream begins the recording of an audio stream to memory, whether it be done with QAudioInput
 * or QAudioOutput, which is decided by the end-user.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param supported_codec The type of codec to use. This feature is yet to be used.
 */
void GkFFTAudio::recordAudioStream()
{
    try {
        emit stopRecording();
        gkAudioBuffer->open(QBuffer::ReadWrite);

        if (pref_input_audio_device.is_enabled) {
            //
            // Input device
            //
            if (gkAudioInput.isNull()) {
                throw std::runtime_error(tr("A memory error has been encountered whilst trying to process input audio for FFT calculations!").toStdString());
            }

            // gkAudioInput->setVolume(0.1);
            gkAudioInput->setNotifyInterval(100);
            gkAudioInput->start(gkAudioBuffer);
            audioStreamProc = true;

            // Move event processing of GkPaStreamHandler to this thread
            gkAudioInput->moveToThread(this);
        } else if (pref_output_audio_device.is_enabled) {
            //
            // Output device
            //
            if (gkAudioOutput.isNull()) {
                throw std::runtime_error(tr("A memory error has been encountered whilst trying to process output audio for FFT calculations!").toStdString());
            }

            // gkAudioOutput->setVolume(0.1);
            gkAudioOutput->setNotifyInterval(100);
            gkAudioOutput->start(gkAudioBuffer);
            audioStreamProc = true;

            // Move event processing of GkPaStreamHandler to this thread
            gkAudioOutput->moveToThread(this);
        } else {
            throw std::invalid_argument(tr("Unable to determine the desired Audio I/O in order to start recording an audio stream to memory!").toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, true, true, false, false);
    }

    return;
}

/**
 * @brief GkFFTAudio::stopRecordStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudio::stopRecordStream()
{
    audioStreamProc = false;

    try {
        //
        // Input Audio
        //
        if (gkAudioInput.isNull()) {
            throw std::runtime_error(tr("A memory error has been encountered with the input audio! Have you configured the audio devices yet?").toStdString());
        }

        if (gkAudioInput->state() == QAudio::ActiveState) { // Stop any active Audio Inputs!
            gkAudioInput->stop();
            // terminateThread();
        }

        //
        // Output Audio
        //
        if (gkAudioOutput.isNull()) {
            throw std::runtime_error(tr("A memory error has been encountered with the output audio! Have you configured the audio devices yet?").toStdString());
        }

        if (gkAudioOutput->state() == QAudio::ActiveState) { // Stop any active Audio Outputs!
            gkAudioOutput->stop();
            // terminateThread();
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
    }

    return;
}

/**
 * @brief GkFFTAudio::recordAudioFileStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 * @param supported_codec
 */
void GkFFTAudio::recordAudioFileStream(const fs::path &media_path, const GkAudioFramework::CodecSupport &supported_codec)
{
    return;
}

/**
 * @brief GkFFTAudio::stopRecordFileStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param media_path
 */
void GkFFTAudio::stopRecordFileStream(const fs::path &media_path)
{
    return;
}

/**
 * @brief GkFFTAudio::samplesUpdated
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note ixSci <https://stackoverflow.com/questions/57719723/cannot-send-events-to-objects-owned-by-a-different-thread-while-runing-a-member>.
 */
void GkFFTAudio::samplesUpdated()
{
    try {
        std::vector<double> remainder;
        auto splitAudioSamples = gkStringFuncs->chunker(audioSamples, (AUDIO_FRAMES_PER_BUFFER * 2));
        for (const auto &samples_vec: splitAudioSamples) {
            if (!samples_vec.empty()) {
                if (samples_vec.size() == (AUDIO_FRAMES_PER_BUFFER * 2)) {
                    Gist<double> fft(samples_vec.size(), gkAudioInSampleRate, WindowType::RectangularWindow);
                    fft.processAudioFrame(samples_vec);
                    magSpec = fft.getMagnitudeSpectrum();

                    double xMin, xMax;
                    size_t historyLength, layerPoints;
                    gkSpectroWaterfall->getDataDimensions(xMin, xMax, historyLength, layerPoints);
                    if (xMin != SPECTRO_X_MIN_AXIS_SIZE || xMax != SPECTRO_X_MAX_AXIS_SIZE || magSpec.size() != layerPoints || 64 != historyLength) {
                        gkSpectroWaterfall->setDataDimensions(SPECTRO_X_MIN_AXIS_SIZE, SPECTRO_X_MAX_AXIS_SIZE, 64, magSpec.size());
                    }

                    const bool bRet = gkSpectroWaterfall->addData(magSpec.data(), magSpec.size(), std::time(nullptr));
                    if (!bRet) {
                        throw std::runtime_error(tr("There has been an error with the spectrograph / waterfall.").toStdString());
                    }

                    magSpec.clear();

                    // Set the range only once (data range)...
                    // TODO: Maybe update the data-range once every <interval> as a suggestion?
                    static bool gkSetRangeOnlyOnce = true;
                    if (gkSetRangeOnlyOnce) {
                        double dataRng[2];
                        gkSpectroWaterfall->getDataRange(dataRng[0], dataRng[1]);
                        gkSpectroWaterfall->setRange(dataRng[0], dataRng[1]);
                        gkSetRangeOnlyOnce = false;
                    }
                }
            } else {
                // Do processing on the remainder...
                remainder.clear();
                if (!samples_vec.empty()) {
                    std::copy(samples_vec.begin(), samples_vec.end(), std::back_inserter(remainder));
                }
            }
        }

        audioSamples.clear();
        if (!remainder.empty()) {
            std::copy(remainder.begin(), remainder.end(), std::back_inserter(audioSamples));
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return;
}
