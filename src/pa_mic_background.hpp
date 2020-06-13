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
 **   Small World is distributed in the hope that it will be useful,
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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/audio_devices.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/file_io.hpp"
#include "src/spectro_gui.hpp"
#include "src/dek_db.hpp"
#include "contrib/portaudio/cpp/include/portaudiocpp/PortAudioCpp.hxx"
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <QObject>
#include <QPointer>
#include <memory>
#include <mutex>
#include <thread>
#include <future>
#include <vector>
#include <list>

#ifdef _WIN32
#include "src/string_funcs_windows.hpp"
#elif __linux__
#include "src/string_funcs_linux.hpp"
#endif

namespace GekkoFyre {

class paMicProcBackground : public QObject {
    Q_OBJECT

public:
    paMicProcBackground(portaudio::System *paInit, const QPointer<GekkoFyre::PaAudioBuf> &audio_buf,
                        std::shared_ptr<GekkoFyre::AudioDevices> audioDev,
                        std::shared_ptr<GekkoFyre::StringFuncs> stringFunc,
                        std::shared_ptr<GekkoFyre::FileIo> fileIo,
                        std::shared_ptr<GkLevelDb> levelDb,
                        const GekkoFyre::Database::Settings::Audio::GkDevice &pref_input_device,
                        const size_t &input_buffer_size, const size_t &samples_per_line,
                        const size_t &num_lines, QObject *parent = nullptr);
    ~paMicProcBackground();

signals:
    void stopRecording(const bool &recording_is_stopped, const int &wait_time = 5000);
    void updateVolume(const double &volumePctg);
    void updateWaterfall(const std::vector<GekkoFyre::Spectrograph::RawFFT> &data,
                         const std::vector<int> &raw_audio_data,
                         const int &hanning_window_size, const size_t &buffer_size);
    void updateFrequencies(const float &frequency, const GekkoFyre::AmateurRadio::DigitalModes &digital_mode,
                           const GekkoFyre::AmateurRadio::IARURegions &iaru_region, const bool &remove_freq);

public slots:
    void abortRecording(const bool &recording_is_stopped, const int &wait_time = 5000);

private:
    size_t audio_buffer_size;   // The total size of the audio buffer for recording.
    bool threads_already_open;  // Whether or not the threads for computing graph data and volume information have been started or not yet.

    //
    // Class pointers
    //
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDev;
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::FileIo> gkFileIo;
    std::shared_ptr<GkLevelDb> gkDb;

    //
    // Miscellaneous Pointers
    //
    portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *streamRecord;
    GekkoFyre::PaAudioBuf *gkAudioBuf;
    GekkoFyre::Database::Settings::Audio::GkDevice sel_input_device;

    //
    // Hamlib sub-system
    //
    std::vector<float> frequencyList;

    //
    // Audio sub-system
    //
    size_t sampleRate;
    size_t sampleLength;
    size_t fftSize;

    float *waveRingBuffer;
    size_t ringBufferSize;
    size_t sampleCounter;
    size_t samplesPerLine;

    double deltaTime;

    //
    // Spectrogram-related
    //
    std::list<std::vector<float>> spectrogramData;

    //
    // Threads
    //
    boost::thread vu_meter;
    boost::thread spectro_thread;

    void initRecording();
    void procVuMeter(const size_t &buffer_size, GekkoFyre::PaAudioBuf *audio_buf,
                     portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *stream);
    void spectrographCallback(GekkoFyre::PaAudioBuf *audio_buf,
                              portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *stream,
                              float *buffer);

};
};
