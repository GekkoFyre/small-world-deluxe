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
#include <utility>

#ifdef __cplusplus
extern "C"
{
#endif

#include "src/contrib/codec2/src/interldpc.h"
#include "src/contrib/codec2/src/gp_interleaver.h"
#include <codec2/codec2_ofdm.h>
#include <codec2/varicode.h>
#include <codec2/freedv_api.h>

#ifdef __cplusplus
} // extern "C"
#endif

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

        if ((ofdm_cfg = (struct OFDM_CONFIG *) calloc(1, sizeof (struct OFDM_CONFIG))) == nullptr) {
            throw std::runtime_error(tr("Not enough memory to initialize the OFDM libraries!").toStdString());
        }

        return;
    }  catch (const std::exception &e) {
        std::throw_with_nested(tr("Error with initializing Codec2 library! Error:\n\n%1")
                               .arg(QString::fromStdString(e.what())).toStdString());
    }

    return;
}

GkCodec2::~GkCodec2()
{
    return;
}

/**
 * @brief GkCodec2::txCodec2OfdmRawData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param gkRadio
 * @param gkAudioDevice
 * @param verbose
 * @param use_text
 * @param dpsk
 * @param rx_center
 * @param tx_center
 * @param data_bits_per_symbol
 * @param sample_frequency
 * @param num_carriers
 * @note Codec2 @ GitHub <https://github.com/drowe67/codec2/blob/master/src/ofdm_mod.c>
 */
void GkCodec2::txCodec2OfdmRawData(const GkRadio &gkRadio, const GkDevice &gkAudioDevice, const int &verbose, const int &use_text,
                                   const int &dpsk, const float &rx_center, const float &tx_center, const int &data_bits_per_symbol,
                                   int data_bits_per_frame, const int &num_sample_frames, const float &sample_frequency,
                                    const int &num_carriers, const float &ts)
{
    try {
        const int ldpc_en = convertFreeDvModeToInt(gkFreeDvMode);
        const int interleave_frames = 1;

        if (ldpc_en >= 0) {
            ofdm_cfg->fs = sample_frequency;        // Sample Frequency
            ofdm_cfg->timing_mx_thresh = 0.30f;
            ofdm_cfg->ftwindowwidth = 11;
            ofdm_cfg->bps = data_bits_per_symbol;   // Bits per Symbol
            ofdm_cfg->txtbits = 4;                  // number of auxiliary data bits
            ofdm_cfg->ns = num_sample_frames;       // Number of Symbol frames

            ofdm_cfg->tx_centre = tx_center;        // Set an optional modulation TX centre frequency (1500.0 default)
            ofdm_cfg->rx_centre = rx_center;        // Set an optional modulation RX centre frequency (1500.0 default)
            ofdm_cfg->nc = num_carriers;            // Number of Carriers (17 default, 62 max)
            ofdm_cfg->tcp = 0.002f;                 // Cyclic Prefix Duration
            ofdm_cfg->ts = 0.018f;                  // Symbol Duration

            ofdm_cfg->rs = (1.0f / ts);             // Modulating Symbol Rate

            struct OFDM *ofdm = ofdm_create(ofdm_cfg);
            Q_ASSERT(ofdm != nullptr);
            free(ofdm_cfg);

            //
            // Get a copy of the completed modem config (ofdm_create() fills in more parameters)
            //

            ofdm_cfg = ofdm_get_config_param(ofdm);

            ofdm_bitsperframe = ofdm_get_bits_per_frame(ofdm);
            ofdm_nuwbits = ((ofdm_cfg->ns - 1) * ofdm_cfg->bps - ofdm_cfg->txtbits);
            ofdm_ntxtbits = ofdm_cfg->txtbits;

            //
            // Set up default LPDC code. We could add other codes here if we like...
            //

            struct LDPC ldpc;

            int Nbitsperframe;
            int coded_bits_per_frame;

            if (ldpc_en) {
                if (ldpc_en == 1) {
                    set_up_hra_112_112(&ldpc, ofdm_cfg);
                } else {
                    set_up_hra_504_396(&ldpc, ofdm_cfg);
                }

                // Here is where we can change data bits per frame to a number smaller than LDPC code input data bits_per_frame
                if (data_bits_per_frame) {
                    set_data_bits_per_frame(&ldpc, data_bits_per_frame, ofdm_cfg->bps);
                }

                data_bits_per_frame = ldpc.data_bits_per_frame;
                coded_bits_per_frame = ldpc.coded_bits_per_frame;

                assert(data_bits_per_frame <= ldpc.ldpc_data_bits_per_frame);
                assert(coded_bits_per_frame <= ldpc.ldpc_coded_bits_per_frame);

                if (verbose > 1) {
                    gkEventLogger->publishEvent(tr("ldpc_data_bits_per_frame = %1").arg(QString::number(ldpc.ldpc_data_bits_per_frame)), GkSeverity::Debug);
                    gkEventLogger->publishEvent(tr("ldpc_coded_bits_per_frame  = %1").arg(QString::number(ldpc.ldpc_coded_bits_per_frame)), GkSeverity::Debug);
                    gkEventLogger->publishEvent(tr("data_bits_per_frame = %1").arg(QString::number(data_bits_per_frame)), GkSeverity::Debug);
                    gkEventLogger->publishEvent(tr("coded_bits_per_frame  = %1").arg(QString::number(coded_bits_per_frame)), GkSeverity::Debug);
                    gkEventLogger->publishEvent(tr("ofdm_bits_per_frame  = %1").arg(QString::number(ofdm_bitsperframe)), GkSeverity::Debug);
                    gkEventLogger->publishEvent(tr("interleave_frames: %1").arg(QString::number(interleave_frames)), GkSeverity::Debug);
                }

                assert((ofdm_nuwbits + ofdm_ntxtbits + coded_bits_per_frame) <= ofdm_bitsperframe); // Just a sanity check

                Nbitsperframe = (interleave_frames * data_bits_per_frame);
            }
        } else {}
    }  catch (const std::exception &e) {
        std::throw_with_nested(tr("Unexpected issue with the Codec2 OFDM transmission! Error:\n\n%1")
                               .arg(QString::fromStdString(e.what())).toStdString());
    }

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
