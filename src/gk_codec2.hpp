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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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
#include "src/gk_string_funcs.hpp"
#include <iterator>
#include <memory>
#include <string>
#include <vector>
#include <QList>
#include <QString>
#include <QObject>
#include <QPointer>
#include <QByteArray>

#ifdef CODEC2_LIBS_ENBLD
#include <codec2/codec2.h>
#endif

namespace GekkoFyre {

class GkCodec2 : public QObject {
    Q_OBJECT

public:
    explicit GkCodec2(const Database::Settings::Codec2Mode &freedv_mode, const Database::Settings::Codec2ModeCustom &custom_mode,
                      const int &freedv_clip, const int &freedv_txbpf, QPointer<GekkoFyre::GkLevelDb> levelDb,
                      QPointer<GekkoFyre::GkEventLogger> eventLogger, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                      QObject *parent = nullptr);
    ~GkCodec2() override;

    int transmitData(const QByteArray &byte_array, const bool &play_output_sound = false, const bool &squelch_enable = false,
                     const float &squelch_thresh = -100.0f);

private:
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;

    Database::Settings::Codec2Mode gkFreeDvMode;
    Database::Settings::Codec2ModeCustom gkCustomMode;
    int gkFreeDvClip;
    int gkFreeDvTXBpf;                      // OFDM TX Filter (off by default)

    QList<QByteArray> createPayloadForTx(const QByteArray &byte_array);

    int convertFreeDvModeToInt(const Database::Settings::Codec2Mode &freedv_mode);

};
};
