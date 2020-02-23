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

#include "spectro_fftw.hpp"
#include <cmath>
#include <QMessageBox>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief SpectroFFTW::SpectroFFTW
 * @author Alex OZ9AEC <https://github.com/csete/gqrx/>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
SpectroFFTW::SpectroFFTW(std::shared_ptr<GekkoFyre::StringFuncs> stringFunc,
                         QObject *parent) : QObject(parent)
{
    gkStringFuncs = stringFunc;
}

SpectroFFTW::~SpectroFFTW()
{}

/**
 * @brief SpectroFFTW::stft performs the base Short-time Fourier transform calculations needed for the spectrographs.
 * @author Jack Schaedler <http://ofdsp.blogspot.com/2011/08/short-time-fourier-transform-with-fftw3.html>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param signal The calculated, output STFT data.
 * @param signal_length
 * @param window_size
 * @param hop_size
 * @return
 */
std::vector<Spectrograph::RawFFT> SpectroFFTW::stft(std::vector<double> *signal, int signal_length, int window_size, int hop_size)
{
    std::unique_lock<std::timed_mutex> lck_guard(calc_stft_mtx, std::defer_lock);
    std::vector<Spectrograph::RawFFT> raw_fft_vec;

    try {
        std::lock(calc_stft_mtx, calc_hanning_mtx);
        fftw_plan plan_forward;
        int i = 0;
        fftw_complex *data, *fft_result, *ifft_result;

        data = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * window_size);
        fft_result = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * window_size);
        ifft_result = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * window_size);

        plan_forward = fftw_plan_dft_1d(window_size, data, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);

        // Create a 'hamming window' of appropriate length
        double *window = new double[window_size];
        hanning(window_size, window);

        int chunkPosition = 0;
        int readIndex = 0;

        // Should we stop reading in chunks?
        bool bStop = false;
        int numChunks = 0;

        // Process each chunk of the signal
        while (chunkPosition < signal_length && !bStop) {
            // Copy the chunk into our buffer
            for (i = 0; i < window_size; i++) {
                readIndex = chunkPosition + i;

                if (readIndex < signal_length) {
                    // Note the windowing!
                    data[i][0] = (*signal)[readIndex] * window[i];
                    data[i][1] = 0.0;
                } else {
                    // We have read beyond the signal, so zero-pad it!
                    data[i][0] = 0.0;
                    data[i][1] = 0.0;

                    bStop = true;
                }
            }

            // Perform the FFT on our chunk
            fftw_execute(plan_forward);

            Spectrograph::RawFFT raw_fft;
            raw_fft.chunk_forward_0 = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * window_size);
            raw_fft.chunk_forward_1 = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * window_size);

            //
            // Copy the first ((window_size / 2) + 1) data points into your spectrogram.
            // We do this because the FFT output is mirrored about the nyquist frequency,
            // so the second half of the data is redundant. This is how Matlab's
            // spectrogram routine works.
            //
            for (i = 0; i < ((window_size / 2) + 1); i++) {
                raw_fft.chunk_forward_0[i][0] = fft_result[i][0];
                raw_fft.chunk_forward_1[i][1] = fft_result[i][1];

                raw_fft_vec.push_back(raw_fft);
            }

            chunkPosition += hop_size;
            numChunks++;
        }

        fftw_destroy_plan(plan_forward);

        fftw_free(data);
        fftw_free(fft_result);
        fftw_free(ifft_result);

        delete[] window;

        calc_stft_mtx.unlock();
        return raw_fft_vec;
    } catch (const std::exception &e) {
        HWND hwnd_stft_calc;
        gkStringFuncs->modalDlgBoxOk(hwnd_stft_calc, tr("Error!"), tr("An error has occurred during the calculation of STFT data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_stft_calc);
    }

    calc_stft_mtx.unlock();
    return raw_fft_vec;
}

float *SpectroFFTW::calcPower(const std::vector<float> &audio_samples, const size_t &buffer_size, const int &window_size, const int &feed_rate)
{
    //
    // TODO: Finish coding this!
    //
    double *frame_buffer = nullptr;
    fftw_complex *spectrum;
    fftw_plan p;
    const int no_of_windows = ((buffer_size - (window_size - feed_rate)) / feed_rate);
    float **power = new float*[no_of_windows];;

    const int fft_order = std::ceil(std::logf(window_size) / std::logf(2.0f));
    const int fft_len = 1 << fft_order;

    // Allocate buffer for calculated complex spectrum of each window
    spectrum = (fftw_complex*)fftw_malloc(fft_len * sizeof(fftw_complex));

    // The real to complex fft plan
    p = fftw_plan_dft_r2c_1d(fft_len, frame_buffer, spectrum, FFTW_ESTIMATE);

    // powerSpectrum(fft_result, fft_len);

    return *power;
}

float *SpectroFFTW::powerSpectrum(fftw_complex *spectrum, int N)
{
    float *power = nullptr;

    for (int k = 1; k < ((N + 1) / 2); ++k) {
        power[k] = (spectrum[k][0] * spectrum[k][0] + spectrum[k][1] * spectrum[k][1]);

        if (N % 2 == 0) {
            power[N/2] = (spectrum[N/2][0] * spectrum[N/2][0]); // Nyquist frequency
        }
    }

    return power;
}

/**
 * @brief SpectroFFTW::hanning
 * @author Jack Schaedler <http://ofdsp.blogspot.com/2011/08/short-time-fourier-transform-with-fftw3.html>
 * @param win_length
 * @param buffer
 */
void SpectroFFTW::hanning(int win_length, double *buffer)
{
    std::unique_lock<std::timed_mutex> lck_guard(calc_hanning_mtx, std::defer_lock);
    for (int i = 0; i < win_length; ++i) {
        buffer[i] = 0.54 - (0.46 * cos(2 * M_PI * (i / ((win_length - 1) * 1.0))));
    }

    calc_hanning_mtx.unlock();
    return;
}
