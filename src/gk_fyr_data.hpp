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
#include "src/gk_logger.hpp"
#include "src/dek_db.hpp"
#include "src/gk_fft_audio.hpp"
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <vector>
#include <QFile>
#include <QTime>
#include <QBuffer>
#include <QObject>
#include <QThread>
#include <QPointer>

namespace fs = boost::filesystem;
namespace sys = boost::system;

namespace GekkoFyre {

class GkFyrFormat : public QObject {
    Q_OBJECT

public:
    explicit GkFyrFormat(QObject *parent = nullptr);
    ~GkFyrFormat() override;

    QString msgText;                                        // Any recorded chat-messages as made by the end-user.
    qint64 magSpecFrames;                                   // The total number of whole frames as calculated by, `timePassed`, divided by how often the waterfall spectrograph refreshes.
    QString rxCallsign;                                     // The callsign of the receiving party.
    QString txCallsign;                                     // The callsign of the transmitting party.
    AmateurRadio::DigitalModes digitalCodec;                // The digital mode codec used to either send or receive this message (if any).
    double snrRecvMsg;                                      // SNR information of the receiving message (if any).
    double freqOffsetRecvMsg;                               // Frequency Offset of the receiving message (if any).
    std::vector<std::vector<double>> magSpec;               // The FFT magnitude calculations for the waterfall spectrograph itself.
    GkAudioFramework::CodecSupport audioCodec;              // The audio codec to use and record with.
    QPointer<QBuffer> audioBuf;                             // A pointer for holding buffered audio data.

protected:
    QTime msgTime;                                          // The time at which a message was made by the end-user.
    QTime msgTimePassed;                                    // The amount of time passed between the last recorded, `msgText`, by the end-user and the latest updated, `magSpec`. Continues to count time regardless of the presence of a, `msgText`, or not until recording is stopped.
    QTime totalTime;                                        // The total length of time recorded overall.

private:
    void calcTotalTime();

};

class GkFyrData : public QThread {
    Q_OBJECT

public:
    explicit GkFyrData(QPointer<GekkoFyre::GkLevelDb> database, QPointer<GekkoFyre::GkFFTAudio> fftAudio,
                       QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkFyrData() override;

    void run() Q_DECL_OVERRIDE;

private:
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkFFTAudio> gkFftAudio;

    QFile m_file;
    QPointer<GkFyrFormat> m_data;

    void createFile(const fs::path &filePath);

};
};
