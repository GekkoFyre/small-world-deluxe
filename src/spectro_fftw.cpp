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
 **   Copyright (C) 2019. GekkoFyre.
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
 **   [ 1 ] - https://git.gekkofyre.io/amateur-radio/dekoder-for-morse
 **
 ****************************************************************************************************/

#include "spectro_fftw.hpp"
#include <fftw3.h>
#include <cmath>
#include <memory>

using namespace GekkoFyre;

/**
 * @brief SpectroFFTW::SpectroFFTW
 * @author Alex OZ9AEC <https://github.com/csete/gqrx/>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
SpectroFFTW::SpectroFFTW(QObject *parent) : QObject(parent)
{}

SpectroFFTW::~SpectroFFTW()
= default;

/**
 * @brief SpectroFFTW::hamming performs the calculations for the overall window size of the spectrogram.
 * @author Jack Schaedler <http://ofdsp.blogspot.com/2011/08/short-time-fourier-transform-with-fftw3.html>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param winLength The horizontal length of the window, in pixels.
 * @param buffer The ensuing buffer that's created.
 */
void SpectroFFTW::hamming(int winLength, double *buffer)
{
    for (int i = 0; i < winLength; ++i) {
        buffer[i] = 0.54 - (0.46 * cos(2 * M_PI * (i / ((winLength - 1) * 1.0))));
    }

    return;
}

/**
 * @brief SpectroFFTW::stft performs the base Short-time Fourier transform calculations needed for the spectrographs.
 * @author Jack Schaedler <http://ofdsp.blogspot.com/2011/08/short-time-fourier-transform-with-fftw3.html>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param signal The calculated, output STFT data.
 * @param signalLength
 * @param windowSize
 * @param hopSize
 */
Spectrograph::RawFFT SpectroFFTW::stft(std::vector<double> *signal, int signalLength, int windowSize, int hopSize)
{
    Spectrograph::RawFFT raw_fft;
    fftw_complex *data, *fft_result;
    fftw_plan plan_forward;
    int i;

    data = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * windowSize);
    fft_result = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * windowSize);

    plan_forward = fftw_plan_dft_1d(windowSize, data, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);

    // Create a 'hamming window' of appropriate length
    double *window = new double[windowSize];
    hamming(windowSize, window);

    int chunkPosition = 0;
    int readIndex;

    // Should we stop reading in chunks?
    int bStop = 0;

    int numChunks = 0;

    // Process each chunk of the signal
    while (chunkPosition < signalLength && !bStop) {
        // Copy the chunk into our buffer
        for (i = 0; i < windowSize; i++) {
            readIndex = chunkPosition + i;

            if (readIndex < signalLength) {
                // Note the windowing!
                data[i][0] = (*signal)[readIndex] * window[i];
                data[i][1] = 0.0;
            } else {
                // we have read beyond the signal, so zero-pad it!

                data[i][0] = 0.0;
                data[i][1] = 0.0;

                bStop = 1;
            }
        }

        // Perform the FFT on our chunk
        fftw_execute(plan_forward);

        raw_fft.chunk_forward_0 = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * windowSize);
        raw_fft.chunk_forward_1 = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * windowSize);

        for (i = 0; i < (windowSize / 2) + 1; i++) {
            raw_fft.chunk_forward_0[i][0];
            raw_fft.chunk_forward_1[i][1];
        }

        chunkPosition += hopSize;
        numChunks++;
    }

    fftw_destroy_plan(plan_forward);

    // fftw_free(data);
    // fftw_free(fft_result);
    delete[] window;

    return raw_fft;
}
