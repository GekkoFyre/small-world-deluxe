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
 * @brief GkFFT::GkFFT
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param spectrogramData_
 * @param waveEnvelopeMin_
 * @param waveEnvelopeMax_
 * @param timeList_
 * @param numLines_
 * @param deltaTime_
 * @param headTime_
 * @param footTime_
 * @param parent
 */
GkFFT::GkFFT(const std::list<std::vector<float>> &spectrogramData_, const std::list<float> &waveEnvelopeMin_,
             const std::list<float> &waveEnvelopeMax_, const std::list<float> &timeList_,
             const size_t &numLines_, const double &deltaTime_, const double &headTime_, const double &footTime_,
             QObject *parent) : QObject(parent)
{
    spectrogramData = spectrogramData_;
    waveEnvelopeMin = waveEnvelopeMin_;
    waveEnvelopeMax = waveEnvelopeMax_;
    timeList = timeList_;
    numLines = numLines_;
    deltaTime = deltaTime_;
    headTime = headTime_;
    footTime = footTime_;

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

/**
 * @brief GkFFT::addLine
 * @author Ville Räisänen <https://github.com/vsr83/QSpectrogram/>
 * @param fourierData
 * @param dataLength
 * @param envMin
 * @param envMax
 */
void GkFFT::addLine(float *fourierData, unsigned int dataLength, float envMin, float envMax)
{
    std::vector<float> fourierDataVec;

    if (spectrogramData.size() >= numLines) {
        removeFoot(spectrogramData.size() - numLines + 1);
    }
    fourierDataVec.assign(fourierData, fourierData + dataLength);
    spectrogramData.push_back(fourierDataVec);
    waveEnvelopeMax.push_back(envMax);
    waveEnvelopeMin.push_back(envMin);

    headTime += deltaTime;
    timeList.push_back(headTime);

    return;
}

/**
 * @brief GkFFT::removeFoot
 * @author Ville Räisänen <https://github.com/vsr83/QSpectrogram/>
 * @param numLines
 */
void GkFFT::removeFoot(const size_t &numLines)
{
    for (unsigned int indLine = 0; indLine < numLines; indLine++) {
        assert(!spectrogramData.empty());
        assert(!timeList.empty());
        spectrogramData.pop_front();
        timeList.pop_front();

        waveEnvelopeMin.pop_front();;
        waveEnvelopeMax.pop_front();;

        footTime += deltaTime;
    }
}
