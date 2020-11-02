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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include <memory>
#include <vector>
#include <string>
#include <QTimer>
#include <QObject>
#include <QString>
#include <QPointer>
#include <QIODevice>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioDeviceInfo>

namespace GekkoFyre {

class GkSinewaveTest : public QIODevice {
    Q_OBJECT

public:
    explicit GkSinewaveTest(const GekkoFyre::Database::Settings::Audio::GkDevice &audio_dev, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                            qint32 freq, QObject *parent = nullptr);
    ~GkSinewaveTest() override;

    void setFreq(qint32 freq);
    void setDuration(qint32 ms);

    qint64 readData(char *data, qint64 max_length) Q_DECL_OVERRIDE;
    qint64 writeData(const char *data, qint64 length) Q_DECL_OVERRIDE;

private:
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    GekkoFyre::Database::Settings::Audio::GkDevice gkAudioDevice;

    qint32 frequency;
    qint32 samples;             // Samples to play for desired duration
    qint32 *end;                // The last position within the circular buffer, for faster comparisons
    qint32 *buffer;             // Sinewave buffer itself
    qint32 *send_pos;           // The current position within the circular buffer

};

class GkSinewaveOutput : public QObject {
    Q_OBJECT

public:
    explicit GkSinewaveOutput(const GekkoFyre::Database::Settings::Audio::GkDevice &audio_dev, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                              QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput, QObject *parent = nullptr);
    ~GkSinewaveOutput() override;

public slots:
    void playSound(quint32 milliseconds);

private slots:
    void writeMore();

private:
    GekkoFyre::Database::Settings::Audio::GkDevice gkAudioDevice;
    QPointer<GkSinewaveTest> gkSinewaveTest;
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;

    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QIODevice *output;
    QTimer *timer;
    char *buffer;

};
};
