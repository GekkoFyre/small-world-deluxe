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

#include "src/audio/encoding/gk_ogg_opus_sink.hpp"
#include <vector>
#include <cstring>
#include <utility>
#include <exception>
#include <QDir>
#include <QtGui>

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;
using namespace Network;
using namespace GkXmpp;

GkOggOpusSink::GkOggOpusSink(QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QIODevice(parent)
{
    setParent(parent);
    gkEventLogger = std::move(eventLogger);

    //
    // Initialize variables
    m_initialized = false;
    m_recActive = GkAudioRecordStatus::Defunct;

    return;
}

GkOggOpusSink::~GkOggOpusSink()
{}

/**
 * @brief GkCodec2Sink::readData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
qint64 GkOggOpusSink::readData(char *data, qint64 maxlen)
{
    return 0;
}

/**
 * @brief GkPcmWavSink::writeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param len
 * @return
 */
qint64 GkOggOpusSink::writeData(const char *data, qint64 len)
{
    return 0;
}

/**
 * @brief GkCodec2Sink::start
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkOggOpusSink::start()
{
    return;
}

/**
 * @brief GkPcmWavSink::stop
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkOggOpusSink::stop()
{
    return;
}
