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
#include "src/file_io.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/gk_frequency_list.hpp"
#include <QObject>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <future>
#include <thread>
#include <queue>
#include <QString>
#include <QVector>
#include <QPointer>

#ifdef _WIN32
#include "src/string_funcs_windows.hpp"
#elif __linux__
#include "src/string_funcs_linux.hpp"
#endif

namespace GekkoFyre {

class AudioDevices : public QObject {
    Q_OBJECT

public:
    explicit AudioDevices(std::shared_ptr<GekkoFyre::GkLevelDb> gkDb, QPointer<GekkoFyre::FileIo> filePtr,
                          QPointer<GekkoFyre::GkFreqList> freqList, std::shared_ptr<GekkoFyre::StringFuncs> stringFuncs,
                          QObject *parent = nullptr);
    ~AudioDevices();

    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> initPortAudio(portaudio::System *portAudioSys);
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> defaultAudioDevices(portaudio::System *portAudioSys);
    std::vector<double> enumSupportedStdSampleRates(const PaStreamParameters *inputParameters,
                                                    const PaStreamParameters *outputParameters);
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> enumAudioDevices();
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> enumAudioDevicesCpp(portaudio::System *portAudioSys);
    GekkoFyre::Database::Settings::Audio::GkDevice gatherAudioDeviceDetails(portaudio::System *portAudioSys,
                                                                            const PaDeviceIndex &pa_index);
    void portAudioErr(const PaError &err);
    void volumeSetting();
    float vuMeter(const int &channels, const int &count, const int &range, const int &range_per_db, float *buffer);
    portaudio::SampleDataFormat sampleFormatConvert(const unsigned long sample_rate);

    PaStreamCallbackResult testSinewave(portaudio::System &portAudioSys, const Database::Settings::Audio::GkDevice device,
                                        const bool &is_output_dev = true);
    PaStreamCallbackResult openPlaybackStream(portaudio::System &portAudioSys, GekkoFyre::PaAudioBuf *audio_buf,
                                              const GekkoFyre::Database::Settings::Audio::GkDevice &device,
                                              const bool &stereo = true);
    PaStreamCallbackResult openRecordStream(portaudio::System &portAudioSys, std::shared_ptr<PaAudioBuf> audio_buf,
                                            const GekkoFyre::Database::Settings::Audio::GkDevice &device,
                                            portaudio::MemFunCallbackStream<PaAudioBuf> **stream_record_ptr,
                                            const bool &stereo = true);

    std::vector<Database::Settings::Audio::GkDevice> filterPortAudioHostType(const std::vector<Database::Settings::Audio::GkDevice> &audio_devices_vec);
    QVector<PaHostApiTypeId> portAudioApiChooser(const std::vector<Database::Settings::Audio::GkDevice> &audio_devices_vec);

    float calcAudioBufferTimeNeeded(const Database::Settings::audio_channels &num_channels, const size_t &fft_num_lines,
                                    const size_t &fft_samples_per_line, const size_t &audio_buf_sampling_length,
                                    const size_t &buf_size);

    QString portAudioVersionNumber(const portaudio::System &portAudioSys);
    QString portAudioVersionText(const portaudio::System &portAudioSys);

private:
    std::shared_ptr<GkLevelDb> gkDekodeDb;
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::GkFreqList> gkFreqList;
    std::shared_ptr<StringFuncs> gkStringFuncs;

    bool filterAudioEnumPreexisting(const std::vector<Database::Settings::Audio::GkDevice> &device_vec,
                                    const Database::Settings::Audio::GkDevice &device_compare);

};
};
