/**
 **  ______  ______  ___   ___  ______  ______  ______  ______
 ** /_____/\/_____/\/___/\/__/\/_____/\/_____/\/_____/\/_____/\
 ** \:::_ \ \::::_\/\::.\ \\ \ \:::_ \ \:::_ \ \::::_\/\:::_ \ \
 **  \:\ \ \ \:\/___/\:: \/_) \ \:\ \ \ \:\ \ \ \:\/___/\:(_) ) )_
 **   \:\ \ \ \::___\/\:. __  ( (\:\ \ \ \:\ \ \ \::___\/\: __ `\ \
 **    \:\/.:| \:\____/\: \ )  \ \\:\_\ \ \:\/.:| \:\____/\ \ `\ \ \
 **     \____/_/\_____\/\__\/\__\/ \_____\/\____/_/\_____\/\_\/ \_\/
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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "gk_audio_decoding.hpp"
#include <boost/exception/all.hpp>

#ifdef __cplusplus
extern "C"
{
#endif

#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef __cplusplus
}
#endif

using namespace GekkoFyre;
namespace fs = boost::filesystem;
namespace sys = boost::system;

GkAudioDecoding::GkAudioDecoding(portaudio::System *paInit,
                                 std::shared_ptr<FileIo> fileIo,
                                 std::shared_ptr<AudioDevices> audioDevs,
                                 QPointer<PaAudioBuf> audio_buf,
                                 std::shared_ptr<StringFuncs> stringFuncs,
                                 Database::Settings::Audio::GkDevice output_device,
                                 QObject *parent)
{
    gkFileIo = fileIo;
    gkAudioDevices = audioDevs;
    gkAudioBuf = audio_buf;
    gkStringFuncs = stringFuncs;

    gkPaInit = paInit;
    gkOutputDev = output_device;
}

GkAudioDecoding::~GkAudioDecoding()
{}

std::string GkAudioDecoding::readOgg(const fs::path &filePath)
{
    auto file_contents = gkFileIo->get_file_contents(filePath);

    return 0;
}
