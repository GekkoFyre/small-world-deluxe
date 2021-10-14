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
GkFFTAudio::GkFFTAudio(std::shared_ptr<std::vector<ALshort>> audioDevBuf, const GkDevice &audioDevDetails,
                       QPointer<GekkoFyre::GkAudioDevices> audioDevices, QPointer<GekkoFyre::GkSpectroWaterfall> spectroWaterfall,
                       QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                       QObject *parent) : QObject(parent)
{
    setParent(parent);

    //
    // Preferred multimedia settings and information for audio devices, as made and
    // configured by the end-user themselves (unless SWD has been started for the
    // first time and we are using the default, 'best guess' settings).
    //
    mAudioDevBuf = std::move(audioDevBuf);
    mAudioDevDetails = audioDevDetails;

    gkSpectroWaterfall = std::move(spectroWaterfall);
    gkAudioDevices = std::move(audioDevices);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    // gkAudioInSampleRate = pref_input_audio_device.audio_device_info.preferredFormat().sampleRate();
    gkAudioInNumSamples = (gkAudioInSampleRate * (SPECTRO_Y_AXIS_SIZE / 1000));

    spectroRefreshTimer = new QTimer(this);
    spectroRefreshTimer->setInterval(std::chrono::milliseconds(1000));
    QObject::connect(spectroRefreshTimer, SIGNAL(timeout()), this, SLOT(refreshGraphTrue()));
    QObject::connect(this, SIGNAL(stopRecording()), spectroRefreshTimer, SLOT(stop()));
    QObject::connect(this, SIGNAL(startRecording()), spectroRefreshTimer, SLOT(start()));

    //
    // Waterfall graph
    QObject::connect(this, SIGNAL(refreshGraph(bool)), gkSpectroWaterfall, SLOT(replot(bool)));

    return;
}

GkFFTAudio::~GkFFTAudio()
{}

/**
 * @brief GkFFTAudio::recordAudioStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudio::recordAudioStream()
{
    if (mAudioDevDetails.is_enabled) {
        emit startRecording();
    }

    return;
}

/**
 * @brief GkFFTAudio::stopRecordStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFFTAudio::stopRecordStream()
{
    emit stopRecording();
    return;
}

/**
 * @brief GkFFTAudio::processAudioIn
 * @author Joel Svensson <http://svenssonjoel.github.io/pages/qt-audio-fft/index.html>
 */
void GkFFTAudio::processAudioInFft()
{
    if (!mAudioDevBuf->empty()) {
        qint32 ba_num_samples = mAudioDevBuf->size() / 2;
        qint32 b_pos = 0;
        for (qint32 i = 0; i < ba_num_samples; ++i) {
            ALshort s;
            s = mAudioDevBuf->at(b_pos++);
            s |= mAudioDevBuf->at(b_pos++) << 8;
            if (s != 0) {
                audioSamples.push_back((double)s / gkAudioDevices->getPeakValue(mAudioDevDetails.pref_audio_format, GK_AUDIO_DEFAULT_BITRATE));
            } else {
                audioSamples.push_back(0);
            }
        }

        samplesUpdated();
    } else {
        // gkEventLogger->publishEvent(tr("Audio input, %1, has changed to an interrupted state.").arg(pref_input_audio_device.audio_device_info.deviceName()), GkSeverity::Info, "", true, true, false, false);
    }

    return;
}

void GkFFTAudio::refreshGraphTrue()
{
    emit refreshGraph(true);
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
        const auto splitAudioSamples = gkStringFuncs->chunker(audioSamples, (AUDIO_FRAMES_PER_BUFFER * 2));
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
