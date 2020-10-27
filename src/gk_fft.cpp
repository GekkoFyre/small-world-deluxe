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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/gk_fft.hpp"
#include <QtMath>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>
#include <cmath>

std::mutex fft_compute_mtx;
std::mutex fft_curve_mtx;

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
GkFFT::GkFFT(QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent)
{
    setParent(parent);
    eventLogger = std::move(gkEventLogger);
    m_fft = nullptr;
}

GkFFT::~GkFFT()
{
    if (m_fft) {
        free(m_fft);
    }
}

/**
 * @brief GkFFT::FFTCompute performs fast-fourier transforms on given sample data, for 'waterfall-type' spectrographs.
 * @author antonypro <https://github.com/antonypro/AudioStreaming/blob/master/AudioStreamingLib/Demos/BroadcastClient/spectrumanalyzer.cpp>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data The data on which to perform the calculations.
 * @param audioDevice The audio device for which the calculations are being performed on.
 * @param numSamples The number of audio samples.
 * @return The FFT calculations in question.
 */
std::vector<GkFFTSpectrum> GkFFT::FFTCompute(const std::vector<float> &data, const GkDevice &audioDevice, const qint32 &numSamples)
{
    try {
        std::lock_guard<std::mutex> lck_guard(fft_compute_mtx);
        if (!data.empty()) {
            m_fft = kiss_fft_alloc(numSamples, 0, nullptr, nullptr);
            m_spectrum.resize(numSamples);
            m_window.resize(numSamples);

            // Initialize the window
            for (int i = 0; i < numSamples; ++i) {
                float window = 0.5f * float(1 - qCos((2 * M_PI * i) / (numSamples - 1)));
                m_window[i] = window;
            }

            std::copy(data.begin(), data.end(), std::back_inserter(m_spectrum_buffer));
            while (m_spectrum_buffer.size() >= int(numSamples)) {
                QVector<float> middle = m_spectrum_buffer.mid(0, numSamples * sizeof(float));
                int len = middle.size();
                m_spectrum_buffer.remove(0, len);

                auto *samples = reinterpret_cast<const float *>(middle.constData());

                kiss_fft_cpx inbuf[numSamples];
                kiss_fft_cpx outbuf[numSamples];

                // Initialize data array
                for (int i = 0; i < numSamples; ++i) {
                    float realSample = samples[i];
                    float window = m_window[i];
                    float windowedSample = realSample * window;
                    inbuf[i].r = windowedSample;
                    inbuf[i].i = 0;
                }

                // Calculate the FFT
                kiss_fft(m_fft, inbuf, outbuf);

                // Analyze output to obtain amplitude and phase for each frequency
                for (int i = 2; i <= numSamples / 2; ++i) {
                    // Calculate frequency of this complex sample
                    m_spectrum[i].frequency = std::round(i * std::round(audioDevice.chosen_sample_rate) / numSamples);

                    kiss_fft_cpx cpx = outbuf[i];

                    float real = cpx.r;
                    float imag = 0;

                    if (i > 0 && i < numSamples / 2) {
                        kiss_fft_cpx cpx = outbuf[numSamples / 2 + i];
                        imag = cpx.r;
                    }

                    m_spectrum[i].magnitude = float(qSqrt(qreal(real * real + imag * imag)));
                }
            }

            return m_spectrum;
        }
    } catch (std::exception &e) {
        gkEventLogger->publishEvent(tr("An error has been encountered while calculating FFT values for the spectrograph! Error:\n\n"),
                                    GkSeverity::Fatal, QString::fromStdString(e.what()), true);
    }

    return std::vector<GkFFTSpectrum>();
}

/**
 * @brief GkFFT::FFTCurvePlot performs fast-fourier transforms on given sample data, for 2D Curve / Sinewave spectrographs.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data The data on which to perform the calculations.
 * @param audioDevice The audio device for which the calculations are being performed on.
 * @param numSamples The number of audio samples.
 * @return The FFT calculations in question.
 */
std::vector<float> GkFFT::FFTCurvePlot(const std::vector<float> &data, const GkDevice &audioDevice, const qint32 &numSamples)
{
    try {
        // The vectors must be a power of two (for FFT of course)
        std::lock_guard<std::mutex> lck_guard(fft_curve_mtx);
        qint32 size = 64;
        while (size / 2 < numSamples) {
            size = size << 1;
        }

        kiss_fftr_cfg fftConfig = kiss_fftr_alloc(size, false, nullptr,nullptr);
        kiss_fftr_cfg ifftConfig = kiss_fftr_alloc(size, true, nullptr,nullptr);
        kiss_fft_cpx inBuf[size / 2];
        kiss_fft_cpx outBuf[size / 2];

        std::vector<float> inData;
        std::vector<float> outData;

        inData.reserve(size + 1);
        outData.reserve(size + 1);

        std::copy(data.begin(), data.end(), std::back_inserter(inData));

        // Fourier transformation of the vectors
        kiss_fftr(fftConfig, inData.data(), inBuf);

        // Convolution in spacial domain is a multiplication in fourier domain. O(n).
        for (int i = 0; i < size/2; i++) {
            outBuf[i].r = (inBuf[i].r * inBuf[i].r) - (inBuf[i].i * inBuf[i].i);
            outBuf[i].i = (inBuf[i].r * inBuf[i].i) + (inBuf[i].i * inBuf[i].r);
        }

        // Inverse fast fourier tranformation to get the desired data.
        std::vector<GkFFTSpectrum> fft_spectro;
        const quint32 out_size = size + 1;

        float *outDataArr = new float[size + 1];
        kiss_fftri(ifftConfig, outBuf, outDataArr);
        std::copy(outDataArr, outDataArr + size, std::back_inserter(outData));
        delete[] outDataArr;

        // Finally we perform a cleanup!
        kiss_fftr_free(fftConfig);
        kiss_fftr_free(ifftConfig);

        return outData;
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An error has been encountered while calculating FFT values for the 2D Curve / Sinewave plot! Error:\n\n"),
                                    GkSeverity::Fatal, QString::fromStdString(e.what()), true);
    }

    return std::vector<float>();
}
