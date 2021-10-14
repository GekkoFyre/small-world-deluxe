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

#pragma once

#include "src/defines.hpp"
#include "src/gk_logger.hpp"
#include "src/audio_devices.hpp"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <memory>
#include <vector>
#include <string>
#include <QTimer>
#include <QObject>
#include <QString>
#include <QPointer>

namespace GekkoFyre {

class GkSinewaveOutput : public QObject {
    Q_OBJECT

public:
    explicit GkSinewaveOutput(const QString &output_audio_dev_name, QPointer<GekkoFyre::GkAudioDevices> audio_devs,
                              QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkSinewaveOutput() override;

public slots:
    void setPlayLength(quint32 milliseconds);
    void play();

private slots:
    void setBufferLength();
    void setSampleRate();

private:
    QString gkOutputDevName;
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    QTimer *timer;
    ALCdevice *mTestDevice;     // Audio device under test; regards OpenAL.
    ALCcontext *mTestCtx;       // Context; regards OpenAL.
    ALCboolean mTestCtxCurr;    // Current context; regards OpenAL.
    quint32 playLength;         // The amount of time for which to play the artificially created sinewave audio sample.
    quint32 bufferLength;       // The buffer size/length to use for storing the sinewave audio data.
    ALuint sampleRate;          // The preferred sample rate by the given audio device.

    quint32 calcBufferLength();
    std::vector<ALshort> generateSineWaveData();

};
};
