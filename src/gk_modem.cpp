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

#include "src/gk_modem.hpp"

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief GkModem::GkModem
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param gkAudio
 * @param dbPtr
 * @param eventLogger
 * @param parent
 */
GkModem::GkModem(QPointer<GkAudioDevices> gkAudio, QPointer<GkLevelDb> dbPtr, QPointer<GkEventLogger> eventLogger,
                 QPointer<StringFuncs> stringFuncs, QObject *parent)
    : QObject(parent)
{
    setParent(parent);

    gkAudioDevices = std::move(gkAudio);
    gkDb = std::move(dbPtr);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    //
    // Initialize the Reed Solomon encoder/decoder...
    //
    // rs = init_rs_int(6, 0x43, 3, 1, 51, 0); // These settings are applicable to JT65!

    //
    // Initialize the JT65 encoder/decoder class...
    //
    gkModemQRA64 = std::make_unique<GkModemQRA64>(gkAudioDevices, gkEventLogger, gkStringFuncs, 0, 0, 0, this);

    return;
}

GkModem::~GkModem()
{
    return;
}

/**
 * @brief GkModemQRA64::GkModemQRA64
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param gkAudio
 * @param eventLogger
 * @param channel_type
 * @param EbNodB
 * @param mode
 */
GkModemQRA64::GkModemQRA64(QPointer<GkAudioDevices> gkAudio, QPointer<GkEventLogger> eventLogger, QPointer<StringFuncs> stringFuncs,
                           const int &channel_type, const int &EbNodB, const int &mode, QObject *parent)
{
    Q_UNUSED(channel_type);
    Q_UNUSED(EbNodB);
    Q_UNUSED(parent);

    gkAudioDevices = std::move(gkAudio);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    // x = new int[QRA64_K];
    // y = new int[QRA64_N];
    // xdec = new int[QRA64_K];
    // rc = 0;

    // codec_iv3nwv = qra64_init(mode); // codec for IV3NWV
    // code_k1jt = qra64_init(mode); // codec for K1JT

    return;
}

GkModemQRA64::~GkModemQRA64()
{
    // delete[] x;
    // delete[] y;
    // delete[] xdec;

    return;
}

/**
 * @brief GkModemQRA64::encodeWithJT65 will encoded given data, which is provided by QString's, into a JT65 modulated
 * signal with the given code for this provided by the WSJT-X project <https://physics.princeton.edu/pulsar/K1JT/wsjtx.html>.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param callsign_1
 * @param callsign_2
 * @param maidenhead_grid This refers towards the Maidenhead Locator System (a.k.a. QTH Locator and IARU Locator) which is a
 * geographic co-ordinate system used by amateur radio operators to succinctly describe their locations.
 */
void GkModemQRA64::encodeWithJT65(const QString &callsign_1, const QString &callsign_2, const QString &maidenhead_grid)
{
    std::vector<int> own_callsign = gkStringFuncs->convStrToIntArray(callsign_1);
    std::vector<int> reply_callsign = gkStringFuncs->convStrToIntArray(callsign_2);
    std::vector<int> maidenhead_int = gkStringFuncs->convStrToIntArray(maidenhead_grid); // TODO: Check for the size of this that it's not over the limits! //-V808

    //
    // Determine whether we are over the symbol/character limit or not...
    //
    if ((own_callsign.size() > GK_WSJTX_JT65_CALLSIGN_SYMBOL_MAX_SIZE) && (reply_callsign.size() > GK_WSJTX_JT65_CALLSIGN_SYMBOL_MAX_SIZE)) {
        int extra_chars_own_call = GK_WSJTX_JT65_CALLSIGN_SYMBOL_MAX_SIZE - own_callsign.size();
        int extra_chars_reply_call = GK_WSJTX_JT65_CALLSIGN_SYMBOL_MAX_SIZE - reply_callsign.size();
        int result = 0;
        if (extra_chars_own_call > extra_chars_reply_call) {
            result = extra_chars_own_call;
        } else {
            result = extra_chars_reply_call;
        }

        throw std::invalid_argument(tr("Error with JT65 modem input! Callsigns are too big by an extra %1 characters!")
                                    .arg(QString::number(result)).toStdString());
    }

    //
    // Step 1a) IV3NWV makes a CQ call (with no grid)...
    //
    // encodemsg_jt65(x, *own_callsign.data(), *reply_callsign.data(), GRID_BLANK);
    // qra64_encode(); // `x` is the input buffer whilst `y` is the output buffer
    // rx = mfskchannel();

    return;
}

/**
 * @brief GkModemQRA64::decodeFromJT65 will decode any perceptible JT65 modulated signals out of a given audio stream with
 * the given code for this provided by the WSJT-X project <https://physics.princeton.edu/pulsar/K1JT/wsjtx.html>.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param callsign_1
 * @param callsign_2
 * @param maidenhead_grid This refers towards the Maidenhead Locator System (a.k.a. QTH Locator and IARU Locator) which is a
 * geographic co-ordinate system used by amateur radio operators to succinctly describe their locations.
 * @return The decoded output of the captured JT65 signal.
 */
QString GkModemQRA64::decodeFromJT65(const QString &callsign_1, const QString &callsign_2, const QString &maidenhead_grid)
{
    // qra64_decode();

    return "";
}
