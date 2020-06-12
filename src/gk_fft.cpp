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
 **   If you have downloaded the source code for "Dekoder for Morse" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020. GekkoFyre.
 **
 **   Dekoder for Morse is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Dekoder is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Dekoder for Morse.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "gk_fft.hpp"
#include <iostream>
#include <cmath>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;

/**
 * @brief GekkoFyre::GkFFT::GkFFT
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkFFT::GkFFT(QObject *parent)
{
    return;
}

GkFFT::~GkFFT()
{
    return;
}

/**
 * @brief GkFFT::FFTCompute
 * @author Ville Räisänen <https://github.com/vsr83/QSpectrogram/blob/master/spectrogram.cpp>
 * @param data
 * @param dataLength
 */
void GkFFT::FFTCompute(std::complex<float> *data, unsigned int dataLength)
{
    for (unsigned int pos = 0; pos < dataLength; ++pos) {
         unsigned int mask = dataLength;
         unsigned int mirrormask = 1;
         unsigned int target = 0;

        while (mask != 1) {
            mask >>= 1;
            if (pos & mirrormask) {
                target |= mask;
            }

            mirrormask <<= 1;
        }

        if (target > pos) {
            std::complex<float> tmp = data[pos];
            data[pos] = data[target];
            data[target] = tmp;
        }
    }

    for (unsigned int step = 1; step < dataLength; step <<= 1) {
        const unsigned int jump = step << 1;
        const float delta = M_PI / float(step);
        const float sine = std::sin(delta * 0.5);
        const std::complex<float> mult(-2.*sine*sine, std::sin(delta));
        std::complex<float> factor(1.0, 0.0);

        for (unsigned int group = 0; group < step; ++group) {
            for (unsigned int pair = group; pair < dataLength; pair += jump) {
                const unsigned int match = pair + step;
                const std::complex<float> prod(factor * data[match]);
                data[match] = data[pair] - prod;
                data[pair] += prod;
            }
            factor = mult * factor + factor;
        }
    }

    return;
}
