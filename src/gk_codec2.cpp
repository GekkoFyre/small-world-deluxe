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

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;

GkCodec2::GkCodec2(const int &freedv_mode, const int &freedv_clip, const int &freedv_txbpf)
{
    gkFreeDvMode = freedv_mode;
    gkFreeDvClip = freedv_clip;
    gkFreeDvTXBpf = freedv_txbpf;

    freedv = freedv_open(gkFreeDvMode);
    assert(freedv != nullptr);

    freedv_set_clip(freedv, gkFreeDvClip);
    freedv_set_tx_bpf(freedv, gkFreeDvTXBpf);

    return;
}

GkCodec2::~GkCodec2()
{
    return;
}

void GkCodec2::txCodec2RawData(const GkRadio &gkRadio, const GkDevice &gkAudioDevice)
{
    return;
}