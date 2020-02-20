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
#include "src/spectro_fftw.hpp"
#include <portaudiocpp/PortAudioCpp.hxx>
#include <iostream>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief paMicProcBackground::paMicProcBackground
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_buf
 * @param pref_input_device
 * @param input_buffer_size
 * @param parent
 */
paMicProcBackground::paMicProcBackground(portaudio::System *paInit, QPointer<PaAudioBuf> audio_buf,
                                         std::shared_ptr<AudioDevices> audioDev,
                                         std::shared_ptr<GekkoFyre::StringFuncs> stringFunc,
                                         std::shared_ptr<FileIo> fileIo,
                                         QPointer<SpectroGui> spectroGui,
                                         const GkDevice &pref_input_device,
                                         const size_t input_buffer_size, QObject *parent) : QObject(parent)
{
    try {
        std::mutex pa_mic_mtx;
        std::lock_guard<std::mutex> lck_guard(pa_mic_mtx);

        gkAudioDev = audioDev;
        gkStringFuncs = stringFunc;
        gkFileIo = fileIo;
        gkSpectroGui = spectroGui;
        gkAudioBuf = audio_buf;

        sel_input_device = pref_input_device;
        audio_buffer_size = input_buffer_size;
        threads_already_open = false;

        QObject::connect(this, SIGNAL(stopRecording(const bool &, const int &)), this,
                         SLOT(abortRecording(const bool &, const int &)));

        streamRecord = nullptr; // For the receiving of microphone (audio device) input
        gkAudioDev->openRecordStream(*paInit, &gkAudioBuf, sel_input_device, &streamRecord, false);

        emit updateVolume(0);

        return;
    } catch (const std::exception &e) {
        HWND hwnd_mic_proc_background;
        gkStringFuncs->modalDlgBoxOk(hwnd_mic_proc_background, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_mic_proc_background);
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

    if (sel_input_device.dev_output_channel_count > 0 && sel_input_device.def_sample_rate > 0) {
        delete gkAudioBuf;
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

            const size_t rand_num = gkFileIo->generateRandInteger(1, buffer_size, (buffer_size - 1));
            double idx_result = audio_buf->at(rand_num);
            if (idx_result >= 0) {
                // We have a audio sample!
                double percentage = ((idx_result / 32768) * 100);
                emit updateVolume(percentage);
            }
        }
    } catch (const std::exception &e) {
        HWND hwnd_proc_vu_meter = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_proc_vu_meter, tr("Error!"), e.what(), MB_ICONERROR);
        DestroyWindow(hwnd_proc_vu_meter);
    }

    return;
}

void paMicProcBackground::spectrographCallback(PaAudioBuf *audio_buf, portaudio::MemFunCallbackStream<PaAudioBuf> *stream)
{
    try {
        std::mutex spectrograph_callback_mtx;
        std::lock_guard<std::mutex> lck_guard(spectrograph_callback_mtx);
        std::unique_ptr<GekkoFyre::SpectroFFTW> spectro_fftw = std::make_unique<GekkoFyre::SpectroFFTW>(this);
        std::vector<Spectrograph::RawFFT> fft_data;

        while (stream->isOpen()) {
            //
            // Set the y-axis of the graph (i.e. the time) to advance at the speed
            // of how often the audio buffer is filled.
            //

            std::vector<short> raw_audio_data = audio_buf->dumpMemory();
            if (!raw_audio_data.empty()) {
                Spectrograph::RawFFT waterfall_fft_data;
                std::vector<double> conv_audio_data(raw_audio_data.begin(), raw_audio_data.end());
                waterfall_fft_data = spectro_fftw->stft(&conv_audio_data, AUDIO_SIGNAL_LENGTH, gkSpectroGui->gkSpectrogram->xAxis(), FFTW_HOP_SIZE);

                // Add the calculated values to a std::vector<Spectrograph::RawFFT>().
                fft_data.push_back(waterfall_fft_data);

                if (fft_data.size() > 0) {
                    for (size_t x_axis = 0; x_axis < fft_data.size(); ++x_axis) {
                        for (size_t y_axis = 0; y_axis < fft_data.size(); ++y_axis) {
                            emit updateSpectroData(fft_data.data()->chunk_forward_0[x_axis][y_axis], stream);
                            // emit updatePlot();
                        }
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        HWND hwnd_spectro_graph_background;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_graph_background, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_graph_background);
    }

    return;
}
