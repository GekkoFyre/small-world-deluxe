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

#include "src/gk_codec2.hpp"
#include "src/contrib/codec2/src/ofdm_internal.h"
#include "src/contrib/codec2/src/interldpc.h"
#include "src/contrib/codec2/src/gp_interleaver.h"
#include <codec2/codec2_ofdm.h>
#include <codec2/varicode.h>
#include <codec2/freedv_api.h>
#include <utility>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

static int ofdm_bitsperframe;
static int ofdm_nuwbits;
static int ofdm_ntxtbits;

GkCodec2::GkCodec2(const Codec2Mode &freedv_mode, const int &freedv_clip, const int &freedv_txbpf, QPointer<GkEventLogger> eventLogger,
                   QObject *parent)
{
    try {
        setParent(parent);
        gkEventLogger = std::move(eventLogger);

        gkFreeDvMode = freedv_mode;
        gkFreeDvClip = freedv_clip;
        gkFreeDvTXBpf = freedv_txbpf;

        return;
    }  catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("Error with initializing Codec2 library! Error:\n\n%1")
                             .arg(QString::fromStdString(e.what())), QMessageBox::Ok, QMessageBox::Ok);
    }

    return;
}

GkCodec2::~GkCodec2()
{
    return;
}

void GkCodec2::txCodec2OfdmRawData(const GkRadio &gkRadio, const GkDevice &gkAudioDevice)
{
    return;
}

/**
 * @brief GkCodec2::convertFreeDvModeToInt
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freedv_mode
 */
int GkCodec2::convertFreeDvModeToInt(const Codec2Mode &freedv_mode)
{
    switch (freedv_mode) {
    case Codec2Mode::freeDvMode2020:
        return FREEDV_MODE_2020;
    case Codec2Mode::freeDvMode700D:
        return FREEDV_MODE_700D;
    case Codec2Mode::freeDvMode700C:
        return FREEDV_MODE_700C;
    case Codec2Mode::freeDvMode800XA:
        return FREEDV_MODE_800XA;
    case Codec2Mode::freeDvMode2400B:
        return FREEDV_MODE_2400B;
    case Codec2Mode::freeDvMode2400A:
        return FREEDV_MODE_2400A;
    case Codec2Mode::freeDvMode1600:
        return FREEDV_MODE_1600;
    default:
        return -1;
    }

    return -1;
}
