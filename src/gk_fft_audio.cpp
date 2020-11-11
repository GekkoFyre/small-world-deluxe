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
 * @note Joel Svensson <http://svenssonjoel.github.io/pages/qt-audio-fft/index.html>,
 * Paul R <https://stackoverflow.com/questions/4675457/how-to-generate-the-audio-spectrum-using-fft-in-c>.
 */
GkFFTAudio::GkFFTAudio(QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput, const GkDevice &input_audio_device_details,
                       const GkDevice &output_audio_device_details, QPointer<GekkoFyre::GkSpectroWaterfall> spectroWaterfall,
                       QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QThread(parent)
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
    gkSpectroWaterfall = std::move(spectroWaterfall);
    eventLogger = std::move(gkEventLogger);

    gkAudioInSampleRate = pref_input_audio_device.audio_device_info.preferredFormat().sampleRate();
    gkAudioInNumSamples = (gkAudioInSampleRate * (SPECTRO_Y_AXIS_SIZE / 1000));
    gkInputBuffer = new QBuffer(this);

    for (qint32 i = 0; i < gkAudioInNumSamples; ++i) {
        mIndices.append((double)i);
        mSamples.append(0);
    }

    double freqStep = (double)gkAudioInSampleRate / (double)gkAudioInNumSamples;
    double f = SPECTRO_X_MIN_AXIS_SIZE;
    while (f < SPECTRO_X_MAX_AXIS_SIZE) {
        mFftIndices.append(f);
        f += freqStep;
    }

    // Setup the FFT plan
    mFftIn  = fftw_alloc_real(gkAudioInNumSamples);
    mFftOut = fftw_alloc_real(gkAudioInNumSamples);
    mFftPlan = fftw_plan_r2r_1d(gkAudioInNumSamples, mFftIn, mFftOut, FFTW_R2HC,FFTW_ESTIMATE);

    // Move event processing of GkPaStreamHandler to this thread
    QObject::moveToThread(this);
    gkAudioInput->moveToThread(this);

    start();

    return;
}

GkFFTAudio::~GkFFTAudio()
{
    fftw_free(mFftIn);
    fftw_free(mFftOut);
    fftw_destroy_plan(mFftPlan);

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

    QObject::connect(this, SIGNAL(recordStream(const GekkoFyre::GkAudioFramework::CodecSupport &)),
                     this, SLOT(recordAudioStream(const GekkoFyre::GkAudioFramework::CodecSupport &)));
    QObject::connect(this, SIGNAL(stopRecording()), this, SLOT(stopRecordStream()));

    QObject::connect(this, SIGNAL(recordFileStream(const fs::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)),
                     this, SLOT(recordAudioFileStream(const fs::path &, const GekkoFyre::GkAudioFramework::CodecSupport &)));
    QObject::connect(this, SIGNAL(stopRecordingFileStream(const fs::path &)), this, SLOT(stopRecordFileStream(const fs::path &)));

    QObject::connect(gkAudioInput, SIGNAL(notify()), this, SLOT(processAudioIn()));

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
    gkInputBuffer->seek(0);
    QByteArray ba = gkInputBuffer->readAll();

    qint32 ba_num_samples = ba.length() / 2;
    qint32 b_pos = 0;
    for (qint32 i = 0; i < ba_num_samples; ++i) {
        int16_t s;
        s = ba.at(b_pos++);
        s |= ba.at(b_pos++) << 8;
        if (s != 0) {
            mSamples.append((double)s / 32768.0);
        } else {
            mSamples.append(0);
        }
    }

    gkInputBuffer->buffer().clear();
    gkInputBuffer->seek(0);

    samplesUpdated();
    return;
}

/**
 * @brief GkFFTAudio::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param supported_codec
 */
void GkFFTAudio::processEvent(Spectrograph::GkFftEventType audioEventType, const GkAudioFramework::CodecSupport &supported_codec)
{
    try {
        switch (audioEventType) {
            case Spectrograph::GkFftEventType::record:
                emit recordStream(supported_codec);
                break;
            case Spectrograph::GkFftEventType::stop:
                emit stopRecording();
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
 * @brief GkFFTAudio::processEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioEventType
 * @param mediaFilePath
 * @param supported_codec
 */
void GkFFTAudio::processEvent(Spectrograph::GkFftEventType audioEventType, const GkAudioFramework::CodecSupport &supported_codec,
                              const fs::path &mediaFilePath)
{
    try {
        switch (audioEventType) {
            case Spectrograph::GkFftEventType::record:
                if (!mediaFilePath.empty()) {
                    emit recordFileStream(mediaFilePath, supported_codec);
                }

                break;
            case Spectrograph::GkFftEventType::stop:
                if (!mediaFilePath.empty()) {
                    emit stopRecordingFileStream(mediaFilePath);
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
 * @param supported_codec
 */
void GkFFTAudio::recordAudioStream(const GkAudioFramework::CodecSupport &supported_codec)
{
    try {
        if (gkAudioInput.isNull()) {
            throw std::runtime_error(tr("A memory error has been encountered whilst trying to process audio for FFT calculations!").toStdString());
        }

        gkInputBuffer->open(QBuffer::ReadWrite);
        gkAudioInput->start(gkInputBuffer);
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
    try {
        if (gkAudioInput.isNull()) {
            throw std::runtime_error(tr("A memory error has been encountered whilst trying to process audio for FFT calculations!").toStdString());
        }

        gkAudioInput->stop();
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, true, true, false, false);
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
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>,
 * Joel Svensson <http://svenssonjoel.github.io/pages/qt-audio-fft/index.html>.
 */
void GkFFTAudio::samplesUpdated()
{
    try {
        int n = mSamples.length();
        if (n > 96000) mSamples = mSamples.mid(n - gkAudioInNumSamples,-1);
        std::memcpy(mFftIn, mSamples.data(), (gkAudioInNumSamples * sizeof(double)));

        fftw_execute(mFftPlan);

        QVector<double> fftVec;
        for (int i = (gkAudioInNumSamples / gkAudioInSampleRate) * SPECTRO_X_MIN_AXIS_SIZE;
             i < (gkAudioInNumSamples / gkAudioInSampleRate) * SPECTRO_X_MAX_AXIS_SIZE;
             i ++) {
            fftVec.append(abs(mFftOut[i]));
        }

        gkSpectroWaterfall->addData(mFftIndices.mid(0, fftVec.length()).data(), fftVec.length(), time(nullptr));
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}
