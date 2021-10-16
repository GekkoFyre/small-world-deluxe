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

#pragma once

#include "src/defines.hpp"
#include "src/gk_logger.hpp"
#include "src/audio_devices.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_waterfall_gui.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <kiss_fft.h>
#include <string>
#include <vector>
#include <QTimer>
#include <QString>
#include <QObject>
#include <QBuffer>
#include <QPointer>

namespace fs = boost::filesystem;
namespace sys = boost::system;

namespace GekkoFyre {
class GkFFTAudio : public QObject {
    Q_OBJECT

public:
    explicit GkFFTAudio(std::shared_ptr<std::vector<ALshort>> audioDevBuf, const GekkoFyre::Database::Settings::Audio::GkDevice &audioDevDetails,
                        QPointer<GekkoFyre::GkAudioDevices> audioDevices, QPointer<GekkoFyre::GkSpectroWaterfall> spectroWaterfall,
                        QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                        QObject *parent = nullptr);
    ~GkFFTAudio() override;

private slots:
    void refreshGraphTrue();
    void processAudioInFft();

public slots:
    void recordAudioStream();
    void stopRecordStream();

signals:
    void startRecording();
    void stopRecording();
    void refreshGraph(bool forceRepaint = false);

private:
    QPointer<GekkoFyre::GkSpectroWaterfall> gkSpectroWaterfall;
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // Audio System initialization and buffers
    //
    std::shared_ptr<std::vector<ALshort>> mAudioDevBuf;
    GekkoFyre::Database::Settings::Audio::GkDevice mAudioDevDetails;

    qint32 gkAudioInNumSamples = 0;
    qint32 gkAudioInSampleRate = 0;
    std::vector<double> audioSamples;

    //
    // Spectrograph
    //
    QPointer<QTimer> spectroRefreshTimer;
    std::vector<double> magSpec;

    void samplesUpdated();

};
};
