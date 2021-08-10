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
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_string_funcs.hpp"
#include <sndfile.h>
#include <sndfile.hh>
#include <mutex>
#include <thread>
#include <cstdio>
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

class GkPcmWavSink : public QObject {
    Q_OBJECT

public:
    explicit GkPcmWavSink(QPointer<GekkoFyre::GkLevelDb> database, QPointer<QAudioOutput> audioOutput, QPointer<QAudioInput> audioInput, QPointer<QBuffer> audioInputBuf, QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkPcmWavSink() override;

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // QAudioSystem initialization and buffers
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;
    QPointer<QBuffer> gkAudioInputBuf;
    qint64 m_totalUncompBytesRead;
    qint64 m_totalCompBytesWritten;

    //
    // Status variables
    GekkoFyre::GkAudioFramework::GkAudioRecordStatus m_recActive;

    //
    // Encoder variables
    bool m_initialized = false;                                 // Whether an encoding operation has begun or not; therefore block other attempts until this singular one has stopped.

};
};
