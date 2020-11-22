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

#include "src/gk_audio_encoding.hpp"
#include <boost/exception/all.hpp>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <QMessageBox>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef OPUS_LIBS_ENBLD
#include <opus_multistream.h>
#endif

#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
}
#endif

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

namespace fs = boost::filesystem;
namespace sys = boost::system;

#define OGG_VORBIS_READ (1024)

GkAudioEncoding::GkAudioEncoding(QPointer<FileIo> fileIo, QPointer<StringFuncs> stringFuncs,
                                 QPointer<QAudioOutput> audioOutput, QPointer<QAudioInput> audioInput,
                                 const GkDevice &output_device, const GkDevice &input_device,
                                 QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QThread(parent)
{
    setParent(parent);

    qRegisterMetaType<GekkoFyre::GkAudioFramework::Bitrate>("GekkoFyre::GkAudioFramework::Bitrate");
    qRegisterMetaType<GekkoFyre::GkAudioFramework::CodecSupport>("GekkoFyre::GkAudioFramework::CodecSupport");

    gkFileIo = std::move(fileIo);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    gkInputDev = input_device;
    gkOutputDev = output_device;

    start();

    // Move event processing of GkPaStreamHandler to this thread
    QObject::moveToThread(this);
}

GkAudioEncoding::~GkAudioEncoding()
{
    quit();
    wait();
}

/**
 * @brief GkAudioEncoding::run
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioEncoding::run()
{
    exec();
    return;
}

/**
 * @brief GkAudioEncoding::char_to_int
 * @author sehe <https://stackoverflow.com/questions/16496288/decoding-opus-audio-data>
 * @param ch
 * @return
 * @note The author suggests reading with `boost::spirit::big_dword` or similar instead.
 */
uint32_t GkAudioEncoding::char_to_int(char ch[])
{
    return static_cast<uint32_t>(static_cast<unsigned char>(ch[0])<<24) |
               static_cast<uint32_t>(static_cast<unsigned char>(ch[1])<<16) |
               static_cast<uint32_t>(static_cast<unsigned char>(ch[2])<< 8) |
            static_cast<uint32_t>(static_cast<unsigned char>(ch[3])<< 0);
}

#ifdef OPUS_LIBS_ENBLD
const char *GkAudioEncoding::OpusErrorException::what() const noexcept
{
    return opus_strerror(code);
}
#endif
