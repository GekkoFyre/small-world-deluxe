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

#pragma once

#include "src/defines.hpp"
#include <list>
#include <vector>
#include <complex>
#include <QObject>

namespace GekkoFyre {
class GkFFT : public QObject {
    Q_OBJECT

public:
    explicit GkFFT(const std::list<std::vector<float>> &spectrogramData_, const std::list<float> &waveEnvelopeMin_,
                   const std::list<float> &waveEnvelopeMax_, const std::list<float> &timeList_,
                   const size_t &numLines_, const double &deltaTime_, const double &headTime_, const double &footTime_,
                   QObject *parent = nullptr);
    ~GkFFT();

    void FFTCompute(std::complex<float> *data, unsigned int dataLength);
    void addLine(float *fourierData, unsigned int dataLength, float envMin, float envMax);

private:
    std::list<std::vector<float>> spectrogramData;
    std::list<float> waveEnvelopeMin;
    std::list<float> waveEnvelopeMax;
    std::list<float> timeList;
    size_t numLines;
    double deltaTime;
    double headTime;
    double footTime;

    void removeFoot(const size_t &numLines);

};
};
