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
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <QObject>
#include <QBuffer>
#include <QPointer>
#include <QSaveFile>
#include <QIODevice>
#include <QByteArray>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>

namespace GekkoFyre {

class GkOggVorbisSink : public QIODevice {
    Q_OBJECT

public:
    explicit GkOggVorbisSink(const QString &fileLoc, QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkOggVorbisSink() override;

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

public slots:
    void start();
    void stop();

signals:
    void volume(qint32 vol);

private:
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // QAudioSystem initialization and buffers
    qint64 m_totalUncompBytesRead;
    qint64 m_totalCompBytesWritten;

    //
    // Status variables
    GekkoFyre::GkAudioFramework::GkAudioRecordStatus m_recActive;

    //
    // Encoder variables
    bool m_initialized = false;                                 // Whether an encoding operation has begun or not; therefore block other attempts until this singular one has stopped.

    //
    // Filesystem and related
    QPointer<QSaveFile> file;                                   // The file that the encoded data is to be saved towards.
    QPointer<QSaveFile> file_pcm;                               // If the user desires so, then a PCM WAV file can be created as an adjunct too!

};
};
