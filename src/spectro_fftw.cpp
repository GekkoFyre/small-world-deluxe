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
#include <utility>

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
    gkStringFuncs = std::move(stringFunc);
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
 * @note <https://dsp.stackexchange.com/questions/10062/when-should-i-calculate-psd-instead-of-plain-fft-magnitude-spectrum>
 */
void SpectroFFTW::stft(std::vector<double> *signal, int signal_length, int window_size,
                       int hop_size, const int &feed_rate,
                       std::promise<std::vector<Spectrograph::RawFFT>> ret_data_promise)
{
    Q_UNUSED(feed_rate);

    std::unique_lock<std::timed_mutex> lck_guard(calc_stft_mtx, std::defer_lock);
    std::vector<Spectrograph::RawFFT> raw_fft_vec;

    try {
        if (calc_stft_mtx.try_lock()) {
            fftw_plan plan_forward;
            int i = 0;
            fftw_complex *data, *fft_result, *ifft_result;

            // const size_t no_of_windows = ((audio_buffer_size - (window_size - feed_rate)) / feed_rate);
            // int fft_order = 0;

            // Calculate FFT length => next power of 2
            // fft_order = std::ceil(std::logf(window_size) / std::logf(2.0f));
            // const int fft_len = 1 << fft_order;

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

                    if (readIndex < signal->size()) {
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
                    // raw_fft.power = powerSpectrum(raw_fft.chunk_forward_0, fft_len);
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
        }

        ret_data_promise.set_value(raw_fft_vec);
        calc_stft_mtx.unlock();
        return;
    } catch (const std::exception &e) {
        HWND hwnd_stft_calc;
        gkStringFuncs->modalDlgBoxOk(hwnd_stft_calc, tr("Error!"), tr("An error has occurred during the calculation of STFT data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_stft_calc);
    }

    calc_stft_mtx.unlock();
    return;
}

/**
 * @brief SpectroFFTW::calcPwr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param tds
 * @param win_size
 * @param pds_data_promise
 * @see GekkoFyre::SpectroFFTW::powerSpectrum().
 */
void SpectroFFTW::calcPwr(const std::vector<Spectrograph::RawFFT> &tds, const int &win_size,
                          std::promise<std::vector<Spectrograph::RawFFT>> pds_data_promise)
{
    std::mutex calc_pwr_handler_mtx;
    std::lock_guard<std::mutex> lck_guard(calc_pwr_handler_mtx);
    std::vector<Spectrograph::RawFFT> stft_data = tds;

    if (!stft_data.empty()) {
        for (size_t i = 0; i < tds.size(); ++i) {
            stft_data.at(i).power_density_spectrum = powerSpectrum(stft_data.at(i).chunk_forward_0, win_size);
        }
    }

    pds_data_promise.set_value(stft_data);
    return;
}

/**
 * @brief SpectroFFTW::calcPwrTest For performing tests with GekkoFyre::SpectroFFTW::calcPwr() and
 * the like.
 * @author Hartmut Pfitzinger <https://stackoverflow.com/questions/24696122/calculating-the-power-spectral-density/24739037>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param num_periods The number of sinusoidal periods to generate at a given time.
 * @param win_size A window size of `512` will mean that a single sinusoidal period will have
 * exactly 512 samples, 2 => 256, 3 => 512 / 3, , 4 => 128, etc. This way, you are able to generate
 * energy at a specific spectral line.
 * @note <https://stackoverflow.com/questions/24696122/calculating-the-power-spectral-density/24739037>
 * @see GekkoFyre::SpectroFFTW::powerSpectrum(), GekkoFyre::SpectroFFTW::calcPwr().
 */
void SpectroFFTW::calcPwrTest(const size_t &num_periods, const int &win_size)
{
    for (size_t N = 0; N < win_size; ++N) {
        double value = 0;
        value = std::fabs(std::sin((2 * M_PI * num_periods * N) / win_size));
    }

    return;
}

/**
 * @brief SpectroFFTW::powerSpectrum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param tds The Time Domain Signal, as calculated originally by GekkoFyre::SpectroFFTW::stft().
 * @param win_size Be sure to keep this the same as the one used in any previous FFT-related functions. But
 * please note that this is NOT the calculated result of the hanning window.
 * @return The calculated Power Density Spectrum values.
 * @note <https://stackoverflow.com/questions/24696122/calculating-the-power-spectral-density/24739037>
 */
std::vector<double> SpectroFFTW::powerSpectrum(fftw_complex *tds, const int &win_size)
{
    std::mutex calc_pwr_spectrum_mtx;
    std::lock_guard<std::mutex> lck_guard(calc_pwr_spectrum_mtx);

    std::vector<double> power_spectrum;
    for (size_t i = 0; i < (win_size / 2); ++i) {
        power_spectrum.push_back(((tds[i][0] * tds[i][0]) + (tds[win_size - i][0] * tds[win_size - i][0])));
    }

    return power_spectrum;
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

    if (calc_hanning_mtx.try_lock()) {
        for (int i = 0; i < win_length; ++i) {
            buffer[i] = 0.54 - (0.46 * cos(2 * M_PI * (i / ((win_length - 1) * 1.0))));
        }
    }

    calc_hanning_mtx.unlock();
    return;
}
