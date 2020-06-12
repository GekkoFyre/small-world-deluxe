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

#include "pa_mic_background.hpp"
#include "spectro_cuda.h"
#include "gk_fft.hpp"
#include <iostream>
#include <utility>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief paMicProcBackground::paMicProcBackground
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param paInit
 * @param audio_buf
 * @param audioDev
 * @param stringFunc
 * @param fileIo
 * @param levelDb
 * @param pref_input_device
 * @param input_buffer_size
 * @param window_size
 * @param samples_per_line
 * @param num_lines
 * @param parent
 */
paMicProcBackground::paMicProcBackground(portaudio::System *paInit, const QPointer<PaAudioBuf> &audio_buf,
                                         std::shared_ptr<AudioDevices> audioDev,
                                         std::shared_ptr<GekkoFyre::StringFuncs> stringFunc,
                                         std::shared_ptr<FileIo> fileIo,
                                         std::shared_ptr<GekkoFyre::GkLevelDb> levelDb,
                                         const GkDevice &pref_input_device,
                                         const size_t &input_buffer_size, const size_t &samples_per_line,
                                         const size_t &num_lines, QObject *parent) : QObject(parent)
{
    try {
        std::mutex pa_mic_mtx;
        std::lock_guard<std::mutex> lck_guard(pa_mic_mtx);

        gkAudioDev = std::move(audioDev);
        gkStringFuncs = std::move(stringFunc);
        gkFileIo = std::move(fileIo);
        gkAudioBuf = audio_buf;
        gkDb = std::move(levelDb);

        sel_input_device = pref_input_device;
        audio_buffer_size = input_buffer_size;
        threads_already_open = false;

        QObject::connect(this, SIGNAL(stopRecording(const bool &, const int &)), this, SLOT(abortRecording(const bool &, const int &)));

        streamRecord = nullptr; // For the receiving of microphone (audio device) input
        gkAudioDev->openRecordStream(*paInit, &gkAudioBuf, sel_input_device, &streamRecord, gkDb->convertAudioEnumIsStereo(sel_input_device.sel_channels));

        emit updateVolume(0);

        sampleRate = sel_input_device.def_sample_rate;
        sampleLength = SPECTRO_SAMPLING_LENGTH * sampleRate;
        samplesPerLine = samples_per_line;

        fftSize = GK_FFT_SIZE;

        deltaTime = 0.0f;
        deltaTime = ((double)samplesPerLine)/((double)sampleRate);

        waveRingBuffer = new float[ringBufferSize];

        return;
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_mic_proc_background = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_mic_proc_background, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_mic_proc_background);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()));
        #endif
    }

    return;
}

paMicProcBackground::~paMicProcBackground()
{
    emit stopRecording(true, 5000);

    if (streamRecord->isOpen()) {
        streamRecord->stop();
        streamRecord->close();
    }
}

void paMicProcBackground::initRecording()
{
    if (streamRecord != nullptr && threads_already_open == false) {
        if (streamRecord->isOpen()) {
            vu_meter = boost::thread(&paMicProcBackground::procVuMeter, this, audio_buffer_size, gkAudioBuf, streamRecord);
            vu_meter.detach();

            spectro_thread = boost::thread(&paMicProcBackground::spectrographCallback, this, gkAudioBuf, streamRecord);
            spectro_thread.detach();

            threads_already_open = true;

            return;
        }
    }

    return;
}

void paMicProcBackground::abortRecording(const bool &recording_is_stopped, const int &wait_time)
{
    if (recording_is_stopped) {
        if (vu_meter.joinable()) {
            if (vu_meter.try_join_for(boost::chrono::milliseconds(wait_time))) {
                vu_meter.join();
            } else {
                vu_meter.interrupt();
            }
        }

        if (spectro_thread.joinable()) {
            if (spectro_thread.try_join_for(boost::chrono::milliseconds(wait_time))) {
                spectro_thread.join();
            } else {
                spectro_thread.interrupt();
            }
        }

        emit updateVolume(0);
        threads_already_open = false;

        return;
    } else {
        initRecording();

        return;
    }

    return;
}

void paMicProcBackground::procVuMeter(const size_t &buffer_size, PaAudioBuf *audio_buf,
                                      portaudio::MemFunCallbackStream<PaAudioBuf> *stream)
{
    try {
        std::mutex proc_vu_meter_mtx;
        std::lock_guard<std::mutex> lck_guard(proc_vu_meter_mtx);
        while (stream->isOpen()) {
            //
            // Controls how often the volume meter should update/refresh, in milliseconds!
            //
            std::this_thread::sleep_for(std::chrono::duration(std::chrono::milliseconds(AUDIO_VU_METER_UPDATE_MILLISECS)));

            if (audio_buf != nullptr) {
                const size_t rand_num = gkFileIo->generateRandInteger(1, buffer_size, (buffer_size - 1));
                double idx_result = audio_buf->at(rand_num);
                if (idx_result >= 0) {
                    // We have a audio sample!
                    double percentage = ((idx_result / 32768) * 100);
                    emit updateVolume(percentage);
                }
            }
        }
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_proc_vu_meter = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_proc_vu_meter, tr("Error!"), e.what(), MB_ICONERROR);
        DestroyWindow(hwnd_proc_vu_meter);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), e.what());
        #endif
    }

    return;
}

/**
 * @brief paMicProcBackground::spectrographCallback
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_buf
 * @param stream
 * @param buffer
 */
void paMicProcBackground::spectrographCallback(PaAudioBuf *audio_buf, portaudio::MemFunCallbackStream<PaAudioBuf> *stream, float *buffer)
{
    try {
        std::mutex spectrograph_callback_mtx;
        std::lock_guard<std::mutex> lck_guard(spectrograph_callback_mtx);

        while (stream->isOpen()) {
            float waveEnvMin = 0, waveEnvMax = 0;
            for (size_t bufferInd = 0; bufferInd < audio_buffer_size; ++bufferInd) {
                float value = buffer[bufferInd];

                if (value > waveEnvMax) {
                    waveEnvMax = value;
                }
                if (value < waveEnvMin) {
                    waveEnvMin = value;
                }

                /*
                #ifdef GK_CUDA_FFT_ENBL
                PerformCUDAFFT();
                #else
                std::unique_ptr<GkFFT> gkFFT = std::make_unique<GkFFT>(this);
                gkFFT->FFTCompute();
                #endif
                */
            }

            std::this_thread::sleep_for(std::chrono::duration(std::chrono::milliseconds(SPECTRO_REFRESH_CYCLE_MILLISECS)));
        }
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_spectro_graph_background = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_graph_background, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_graph_background);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()));
        #endif
    }

    return;
}
