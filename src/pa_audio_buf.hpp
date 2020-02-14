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

#pragma once

#include "src/defines.hpp"
#include <portaudio.h>
#include <memory>
#include <vector>
#include <string>

namespace GekkoFyre {

class PaAudioBuf : private std::vector<short> {

    typedef short T;
    typedef std::vector<short> vector;

public:
    explicit PaAudioBuf(int size_hint);
    virtual ~PaAudioBuf();

    using vector::push_back;
    using vector::operator[];
    using vector::begin;
    using vector::end;
    using vector::size;
    using vector::clear;
    PaAudioBuf operator*(const PaAudioBuf &) const;
    PaAudioBuf operator+(const PaAudioBuf &) const;
    PaAudioBuf();

    int playbackCallback(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer,
                         const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags);
    int recordCallback(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer,
                       const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags);
    void writeToFile(const std::string &file_name);
    void resetPlayback();

private:
    std::vector<short> rec_samples;
    std::vector<short>::iterator playback_iter;

};
};