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
#include "src/dek_db.hpp"
#include "src/string_funcs_windows.hpp"
#include <fftw3.h>
#include <QObject>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>

namespace GekkoFyre {

class SpectroFFTW: public QObject {
    Q_OBJECT

public:
    explicit SpectroFFTW(std::shared_ptr<GekkoFyre::GkLevelDb> database,
                         std::shared_ptr<GekkoFyre::StringFuncs> stringFunc,
                         QObject *parent = nullptr);
    ~SpectroFFTW();

public slots:
    std::vector<Spectrograph::RawFFT> stft(std::vector<double> *signal, int signal_length, int window_size, int hop_size);

private:
    std::shared_ptr<GekkoFyre::GkLevelDb> gkDb;
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;

    std::timed_mutex calc_stft_mtx;
    std::timed_mutex calc_hanning_mtx;

    void hanning(int win_length, double *buffer);

};
};
