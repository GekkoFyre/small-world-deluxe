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
#include "src/pa_stream_handler.hpp"
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include <AudioFile.h>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <memory>
#include <string>
#include <QString>
#include <QObject>
#include <QPointer>
#include <QAudioInput>
#include <QAudioOutput>

namespace fs = boost::filesystem;
namespace sys = boost::system;

namespace GekkoFyre {

class GkPaAudioPlayer : public QObject {
    Q_OBJECT

public:
    explicit GkPaAudioPlayer(QPointer<GekkoFyre::GkLevelDb> database, const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                             QPointer<QAudioOutput> audioOutput, QPointer<QAudioInput> audioInput, const QPointer<GekkoFyre::GkEventLogger> &eventLogger,
                             std::shared_ptr<AudioFile<double>> audioFileLib, QObject *parent = nullptr);
    virtual ~GkPaAudioPlayer();

    void play(const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec, const fs::path &audio_file);
    void play(const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec);
    void loop(const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec, const fs::path &audio_file);
    void stop(const fs::path &audio_file);
    void loopback();

private:
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;
    QPointer<GkPaStreamHandler> streamHandler;

    //
    // AudioFile objects and related
    //
    std::shared_ptr<AudioFile<double>> gkAudioFile;

};
};
