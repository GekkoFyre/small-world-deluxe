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
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/allocators.hpp>
#include <boost/iterator.hpp>
#include <memory>
#include <vector>
#include <string>

namespace GekkoFyre {

class PaAudioBuf : private boost::circular_buffer<short> {

    typedef short T;
    typedef boost::circular_buffer<short> circular_buffer;

public:
    PaAudioBuf(size_t size_hint, const bool &is_rec_active);
    virtual ~PaAudioBuf();

    PaAudioBuf operator*(const PaAudioBuf &) const;
    PaAudioBuf operator+(const PaAudioBuf &) const;

    int playbackCallback(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer,
                         const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags);
    int recordCallback(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer,
                       const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags);
    std::vector<short> dumpMemory() const;

    virtual size_t size() const;
    virtual short at(const short &idx) const;
    virtual short front() const;
    virtual short back() const;
    virtual void push_back(const short &data);
    virtual void push_front(const short &data);
    virtual void pop_front();
    virtual void pop_back();
    virtual void swap(boost::circular_buffer<short> data_idx_1) noexcept;
    virtual bool empty() const;
    virtual bool clear() const;
    virtual std::vector<short> linearize() const;
    virtual iterator begin() const;
    virtual iterator end() const;

private:
    boost::circular_buffer<short> *rec_samples_ptr;     // Contains the 16-bit mono samples
    size_t buffer_size;

    void dlgBoxOk(const HWND &hwnd, const QString &title, const QString &msgTxt, const int &icon);

};
};
