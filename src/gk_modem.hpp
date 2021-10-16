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
#include "src/audio_devices.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_string_funcs.hpp"
#include <QObject>
#include <QPointer>
#include <memory>

namespace GekkoFyre {

class GkModemQRA64 : public QObject {
    Q_OBJECT

public:
    explicit GkModemQRA64(QPointer<GekkoFyre::GkAudioDevices> gkAudio, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                          QPointer<GekkoFyre::StringFuncs> stringFuncs, const int &channel_type, const int &EbNodB,
                          const int &mode, QObject *parent = nullptr);
    ~GkModemQRA64() override;

    /*
    The QSO is counted as successfull if IV3NWV received the last message
    When mode=QRA_AUTOAP each decoder attempts to decode the message sent
    by the other station using the a-priori information derived by what
    has been already decoded in a previous phase of the QSO if decoding
    with no a-priori information has not been successful.

    Step 1) K1JT's decoder first attempts to decode msgs of type [? ? ?]
    and if this attempt fails, it attempts to decode [CQ/QRZ ? ?]  or
    [CQ/QRZ ?] msgs

    Step 2) if IV3NWV's decoder is unable to decode K1JT's without AP it
    attempts to decode messages of the type [IV3NWV ? ?] and [IV3NWV ?].

    Step 3) K1JT's decoder attempts to decode [? ? ?] and [K1JT IV3NWV ?]
    (this last decode type has been enabled by K1JT's encoder at step 2)

    Step 4) IV3NWV's decoder attempts to decode [? ? ?] and [IV3NWV K1JT
    ?] (this last decode type has been enabled by IV3NWV's encoder at step
    3)

    At each step the simulation reports if a decode was successful.  In
    this case it also reports the type of decode (see table decode_type
    above)

    When mode=QRA_NOAP, only [? ? ?] decodes are attempted and no a-priori
    information is used by the decoder

    The function returns 0 if all of the four messages have been decoded
    by their recipients (with no retries) and -1 if any of them could not
    be decoded
    */

    void encodeWithJT65(const QString &callsign_1, const QString &callsign_2, const QString &maidenhead_grid);
    QString decodeFromJT65(const QString &callsign_1, const QString &callsign_2, const QString &maidenhead_grid);

private:
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;

    int *x;
    int *y;
    int rc;
    float *rx;
    int *xdec;

    // qra64codec *codec_iv3nwv;
    // qra64codec *code_k1jt;

};

class GkModem : public QObject {
    Q_OBJECT

public:
    explicit GkModem(QPointer<GekkoFyre::GkAudioDevices> gkAudio, QPointer<GekkoFyre::GkLevelDb> dbPtr,
                     QPointer<GekkoFyre::GkEventLogger> eventLogger,
                     QPointer<GekkoFyre::StringFuncs> stringFuncs, QObject *parent = nullptr);
    ~GkModem() override;

private:
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    std::unique_ptr<GkModemQRA64> gkModemQRA64;

};
};
