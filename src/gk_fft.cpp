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
 **   Small world is distributed in the hope that it will be useful,
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

#include "src/gk_fft.hpp"
#include <fftw3.h>
#include <iostream>
#include <cmath>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

/**
 * @brief GkFFT::GkFFT
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkFFT::GkFFT()
{
    return;
}

GkFFT::~GkFFT()
{
    return;
}

/**
 * @brief GkFFT::FFTCompute performs fast-fourier transforms on given sample data.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param signal
 * @param signal_length
 * @param window_size
 * @param hop_size
 * @return
 * @note Paul R. <https://stackoverflow.com/questions/4675457/how-to-generate-the-audio-spectrum-using-fft-in-c>
 */
std::vector<GkFFTComplex> GkFFT::FFTCompute(std::vector<float> signal, int signal_length, int window_size, int hop_size)
{
    fftw_complex *data, *fft_result, *ifft_result;
    data = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * window_size);
    fft_result = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * window_size);
    ifft_result = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * window_size);

    fftw_plan plan_forward  = fftw_plan_dft_1d(window_size, data, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);

    // Create a hamming window of appropriate length
    float window[window_size];
    hamming(window_size, window);

    int chunkPosition = 0;
    int readIndex;
    int bStop = 0;
    int numChunks = 0;
    std::vector<GkFFTComplex> gkFftComplex;

    // Process each chunk of the signal
    int i = 0;
    while (chunkPosition < signal_length && !bStop) {
        // Copy the chunk into our buffer
        for(i = 0; i < window_size; i++) {
            readIndex = chunkPosition + i;
            if(readIndex < signal_length) {
                // Note the windowing!
                data[i][0] = (signal)[readIndex] * window[i];
                data[i][1] = 0.0;
            } else {
                // we have read beyond the signal, so zero-pad it!
                data[i][0] = 0.0;
                data[i][1] = 0.0;

                bStop = 1;
            }
        }

        fftw_execute(plan_forward);

        for (i = 0; i < ((window_size / 2) + 1); i++) {
            GkFFTComplex fftComplex;
            fftComplex.real = fft_result[i][0]; // Real
            fftComplex.imaginary = fft_result[i][1]; // Imaginary
            gkFftComplex.push_back(fftComplex);
        }

        chunkPosition += hop_size;
        numChunks++;
    }

    fftw_destroy_plan(plan_forward);

    fftw_free(data);
    fftw_free(fft_result);
    fftw_free(ifft_result);

    return gkFftComplex;
}

void GkFFT::hamming(int windowLength, float *buffer)
{
    for (int i = 0; i < windowLength; i++) {
        buffer[i] = 0.54 - (0.46 * cos( 2 * M_PI * (i / ((windowLength - 1) * 1.0))));
    }
}
