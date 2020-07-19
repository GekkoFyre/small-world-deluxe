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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include "src/gk_logger.hpp"
#include <memory>
#include <string>
#include <QString>
#include <QObject>
#include <QPointer>

#ifdef __cplusplus
extern "C"
{
#endif

#include "src/contrib/codec2/src/ofdm_internal.h"
#include <codec2/codec2.h>

#ifdef __cplusplus
} // extern "C"
#endif

namespace GekkoFyre {

class GkCodec2 : public QObject {
    Q_OBJECT

public:
    explicit GkCodec2(const Database::Settings::Codec2Mode &freedv_mode, const int &freedv_clip, const int &freedv_txbpf,
                      QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkCodec2();

    void txCodec2OfdmRawData(const AmateurRadio::Control::GkRadio &gkRadio, const Database::Settings::Audio::GkDevice &gkAudioDevice, const int &verbose,
                             const int &use_text, const int &dpsk, const float &rx_center, const float &tx_center, const int &data_bits_per_symbol,
                             int data_bits_per_frame, const int &num_sample_frames, const float &sample_frequency = 8000.0f,
                             const int &num_carriers = 2, const float &ts = 0.0180f);

private:
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    Database::Settings::Codec2Mode gkFreeDvMode;
    struct OFDM_CONFIG *ofdm_cfg;
    int gkFreeDvClip;
    int gkFreeDvTXBpf;                      // OFDM TX Filter (off by default)

    int convertFreeDvModeToInt(const Database::Settings::Codec2Mode &freedv_mode);

};
};
