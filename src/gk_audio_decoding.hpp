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
#include "src/file_io.hpp"
#include "src/audio_devices.hpp"
#include "src/gk_logger.hpp"
#include <ogg/os_types.h>
#include <boost/filesystem.hpp>
#include <memory>
#include <string>
#include <vector>
#include <exception>
#include <QThread>
#include <QObject>
#include <QString>
#include <QPointer>

#ifdef __cplusplus
extern "C"
{
#endif

#include "src/contrib/opus/include/opus.h"

#ifdef __cplusplus
} // extern "C"
#endif

namespace GekkoFyre {

class GkAudioDecoding : public QThread {
    Q_OBJECT

public:
    explicit GkAudioDecoding(QPointer<GekkoFyre::FileIo> fileIo,
                             QPointer<GekkoFyre::StringFuncs> stringFuncs,
                             const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                             QPointer<GekkoFyre::GkEventLogger> eventLogger,
                             QObject *parent = nullptr);
    ~GkAudioDecoding() override;

    void run() Q_DECL_OVERRIDE;

private:
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    GekkoFyre::Database::Settings::Audio::GkDevice gkOutputDev;

    static uint32_t char_to_int(char ch[4]);

};
};
