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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/audio_devices.hpp"
#include <map>
#include <cmath>
#include <chrono>
#include <thread>
#include <utility>
#include <cstring>
#include <ostream>
#include <cstdlib>
#include <iostream>
#include <exception>
#include <algorithm>
#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QApplication>

#define SCALE (32767.0)

#define BASE_RATE 0.005
#define TIME 1.0

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

std::mutex enum_audio_dev_mtx;
std::mutex test_sinewave_mtx;
std::mutex init_port_audio_mtx;

/**
 * @brief AudioDevices::AudioDevices
 * @param parent
 * @note Core Audio APIs <https://docs.microsoft.com/en-us/windows/win32/api/_coreaudio/index>
 */
AudioDevices::AudioDevices(QPointer<GkLevelDb> gkDb, QPointer<FileIo> filePtr,
                           QPointer<GekkoFyre::GkFrequencies> freqList, QPointer<StringFuncs> stringFuncs,
                           QPointer<GekkoFyre::GkEventLogger> eventLogger, QPointer<GekkoFyre::GkSystem> systemPtr,
                           QObject *parent)
{
    gkDekodeDb = std::move(gkDb);
    gkFileIo = std::move(filePtr);
    gkFreqList = std::move(freqList);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);
    gkSystem = std::move(systemPtr);
}

AudioDevices::~AudioDevices()
{}

/**
 * @brief AudioDevices::enumAudioDevicesCpp Enumerate out all the findable audio devices within the user's computer system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The found audio devices and their statistics.
 */
GkAudioApi AudioDevices::enumAudioDevicesCpp()
{
    try {
        std::lock_guard<std::mutex> audio_guard(enum_audio_dev_mtx);
        std::vector<GkDevice> audio_devices_vec;                        // The vector responsible for storage all audio device sessions

        // Create an api map.
        std::map<RtAudio::Api, std::string> apiMap;
        apiMap[RtAudio::MACOSX_CORE] = tr("Apple Mac OS/X (Core Audio)").toStdString();
        apiMap[RtAudio::WINDOWS_ASIO] = tr("Microsoft Windows ASIO").toStdString();
        apiMap[RtAudio::WINDOWS_DS] = tr("Microsoft Windows DirectSound").toStdString();
        apiMap[RtAudio::WINDOWS_WASAPI] = tr("Microsoft Windows WASAPI").toStdString();
        apiMap[RtAudio::UNIX_JACK] = tr("Jack Client").toStdString();
        apiMap[RtAudio::LINUX_ALSA] = tr("Linux ALSA").toStdString();
        apiMap[RtAudio::LINUX_PULSE] = tr("Linux PulseAudio").toStdString();
        apiMap[RtAudio::LINUX_OSS] = tr("Linux OSS").toStdString();
        apiMap[RtAudio::RTAUDIO_DUMMY] = tr("RtAudio Dummy").toStdString();

        GkAudioApi gkAudio;

        gkAudio.api_map = apiMap;
        RtAudio::getCompiledApi(gkAudio.api_used);
        gkAudio.api_version = RtAudio::getVersion();

        for (quint32 i = 0; i < gkAudio.api_used.size(); ++i) {
            std::cout << gkAudio.api_map[gkAudio.api_used[i]] << std::endl;
        }

        for (const auto &api: gkAudio.api_used) { // Iterate over each API in the user's system, to garner all the Audio Devices!
            std::unique_ptr<RtAudio> dac = std::make_unique<RtAudio>(api);
            quint32 device_count = dac->getDeviceCount();
            std::cout << tr("Found %1 device(s) for API: \"%2\"!")
            .arg(QString::number(device_count))
            .arg(QString::fromStdString(RtAudio::getApiDisplayName(api))).toStdString() << std::endl;

            for (quint32 i = 0; i < device_count; ++i) {
                GkDevice device;
                device.device_info = dac->getDeviceInfo(i);
                device.device_id = i;
                device.audio_dev_str = QString::fromStdString(device.device_info.name);
                device.assoc_api = api;

                if (device.device_info.probed) {
                    device.dev_input_channel_count = device.device_info.inputChannels;
                    device.dev_output_channel_count = device.device_info.outputChannels;
                    device.supp_native_formats = device.device_info.nativeFormats;
                    device.default_output_dev = device.device_info.isDefaultOutput;
                    device.default_input_dev = device.device_info.isDefaultInput;

                    if (device.default_output_dev) {
                        std::cout << tr("This is the DEFAULT **output** audio device!").toStdString() << std::endl;
                    }

                    if (device.default_input_dev) {
                        std::cout << tr("This is the DEFAULT **input** audio device!").toStdString() << std::endl;
                    }

                    if (device.device_info.nativeFormats == 0) {
                        std::cout << tr("No natively supported data formats(?)!").toStdString();
                    } else {
                        std::cout << tr("Natively supported data formats:").toStdString() << std::endl;
                        if (device.device_info.nativeFormats & RTAUDIO_SINT8) {
                            std::cout << tr("  8-bit int").toStdString() << std::endl;
                        }

                        if (device.device_info.nativeFormats & RTAUDIO_SINT16) {
                            std::cout << tr("  16-bit int").toStdString() << std::endl;
                        }

                        if (device.device_info.nativeFormats & RTAUDIO_SINT24) {
                            std::cout << tr("  24-bit int").toStdString() << std::endl;
                        }

                        if (device.device_info.nativeFormats & RTAUDIO_SINT32) {
                            std::cout << tr("  32-bit int").toStdString() << std::endl;
                        }

                        if (device.device_info.nativeFormats & RTAUDIO_FLOAT32) {
                            std::cout << tr("  32-bit float").toStdString() << std::endl;
                        }

                        if (device.device_info.nativeFormats & RTAUDIO_FLOAT64) {
                            std::cout << tr("  64-bit float").toStdString() << std::endl;
                        }
                    }

                    if (device.device_info.sampleRates.size() < 1) {
                        std::cout << tr("No supported sample rates found!").toStdString() << std::endl;
                    } else {
                        std::cout << tr("Supported sample rates = ").toStdString();
                        for (quint32 j = 0; j < device.device_info.sampleRates.size(); ++j) {
                            std::cout << device.device_info.sampleRates[j] << " ";
                        }
                    }

                    std::cout << std::endl;
                    if (device.device_info.preferredSampleRate == 0) {
                        std::cout << tr("No preferred sample rate found!").toStdString() << std::endl;
                    } else {
                        std::cout << tr("Preferred sample rate = ").toStdString() << device.device_info.preferredSampleRate << std::endl;
                    }

                    std::cout << std::endl;
                    gkAudio.gkDevice.push_back(device);
                } else {
                    throw std::runtime_error(tr("Unable to probe given audio device for further details!").toStdString());
                }
            }
        }

        return gkAudio;
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    }

    return GkAudioApi();
}

/**
 * @brief AudioDevices::testSinewave Performs a sinewave test on the given input/output audio device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device
 * @param is_output_dev
 * @param stereo
 * @return
 */
void AudioDevices::testSinewave(std::shared_ptr<RtAudio> dac, const GkDevice &audio_dev, const bool &is_output_dev)
{
    try {
        std::lock_guard<std::mutex> lck_guard(test_sinewave_mtx);

        if (is_output_dev) {
            if (audio_dev.pref_output_dev) {
                RtAudio::StreamParameters oParams;
                oParams.deviceId = audio_dev.device_id;
                oParams.nChannels = audio_dev.dev_output_channel_count;
                oParams.firstChannel = 0;

                RtAudio::StreamOptions options;
                options.flags = RTAUDIO_HOG_DEVICE;
                options.flags |= RTAUDIO_SCHEDULE_REALTIME;

                GkAudioFramework::GkRtCallback sawData {};
                quint32 bufsize = audio_dev.device_info.preferredSampleRate * 10;
                dac->openStream(&oParams, nullptr, audio_dev.supp_native_formats, audio_dev.device_info.preferredSampleRate, &bufsize, &AudioDevices::playbackSaw,
                                &sawData, &options);

                sawData.nRate = audio_dev.device_info.preferredSampleRate;
                sawData.nFrame = audio_dev.device_info.preferredSampleRate;
                sawData.nChannel = audio_dev.dev_output_channel_count;
                sawData.cur = 0;
                sawData.wfTable = (float *)std::calloc(sawData.nChannel * sawData.nFrame, sizeof(float));

                if (!sawData.wfTable) {
                    dac.reset();
                }

                for (quint32 i = 0; i < sawData.nFrame; ++i) {
                    float v = std::sin(i * M_PI * 2 * 440 / sawData.nRate);
                    for (quint32 j = 0; j < sawData.nChannel; ++j) {
                        sawData.wfTable[i * sawData.nChannel + j] = v;
                    }
                }

                dac->startStream();
                std::this_thread::sleep_for(std::chrono::milliseconds(AUDIO_SINE_WAVE_PLAYBACK_SECS * 1000));
                dac->stopStream();
                dac->closeStream();

                return;
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief AudioDevices::playbackSaw
 * @author nagachika <https://gist.github.com/nagachika/165241/604224f8ec3110ecae2490efbf41681d2e4deb54>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>.
 * @param outputBuffer
 * @param inputBuffer
 * @param nBufferFrames
 * @param streamTime
 * @param status
 * @param userData
 * @return
 */
qint32 AudioDevices::playbackSaw(void *outputBuffer, void *inputBuffer, quint32 nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
    Q_UNUSED(inputBuffer);
    Q_UNUSED(streamTime);
    Q_UNUSED(status);

    auto *buf = (float *)outputBuffer;
    quint32 remainFrames;
    auto *data = (GkAudioFramework::GkRtCallback *)userData;

    remainFrames = nBufferFrames;
    while (remainFrames > 0) {
        unsigned int sz = data->nFrame - data->cur;
        if (sz > remainFrames) {
            sz = remainFrames;
        }

        std::memcpy(buf, data->wfTable + (data->cur * data->nChannel), sz * data->nChannel * sizeof(float));
        data->cur = (data->cur + sz) % data->nFrame;
        buf += sz * data->nChannel;
        remainFrames -= sz;
    }

    return 0;
}

/**
 * @brief AudioDevices::volumeSetting
 * @note Michael Satran & Mike Jacobs <https://docs.microsoft.com/en-us/windows/win32/coreaudio/endpoint-volume-controls>
 */
void AudioDevices::systemVolumeSetting()
{}

/**
 * @brief AudioDevices::vuMeter processes a raw sound buffer and outputs a possible volume level, in decibels (dB).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param channels How many audio channels there are, since they are interleaved. The first byte is the first channel, second
 * byte is the second channel, etc.
 * @param count The amount of `frames per buffer`.
 * @param buffer The raw audiological data.
 * @return The volume level, in decibels (dB).
 * @note RobertT <https://stackoverflow.com/questions/2445756/how-can-i-calculate-audio-db-level>,
 * Vassilis <https://stackoverflow.com/questions/37963115/how-to-make-smooth-levelpeak-meter-with-qt>
 */
float AudioDevices::vuMeter(const int &channels, const int &count, float *buffer)
{
    float dB_val = 0.0f;
    if (buffer != nullptr) {
        float max_val = buffer[0];

        // Find maximum!
        // Traverse the array elements from second and compare every element with current maximum...
        for (int i = 1; i < (count * channels); ++i) {
            if (buffer[i] > max_val) {
                max_val = buffer[i];
            }
        }

        float amplitude = (static_cast<float>(max_val) / gkStringFuncs->getNumericMax<qint64>());
        dB_val = (20 * std::log10(amplitude));

        // Invert from a negative to a positive!
        // dB_val *= (-1.f);
    }

    return dB_val;
}

/**
 * @brief AudioDevices::vuMeterPeakAmplitude traverses the buffered data array and compares every element with the current
 * maximum to see if there's a new maximum value to be had.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param count The size of the audio data buffer.
 * @param buffer The given audio data buffer.
 * @return The maximum, peak audio signal for a given lot of buffered data.
 */
float AudioDevices::vuMeterPeakAmplitude(const size_t &count, float *buffer)
{
    float peak_signal = 0;
    if (buffer != nullptr) {
        peak_signal = buffer[0];

        // Find maximum!
        // Traverse the array elements from second and compare every element with current maximum...
        for (size_t i = 1; i < count; ++i) {
            if (buffer[i] > peak_signal) {
                peak_signal = buffer[i];
            }
        }
    }

    return peak_signal;
}

/**
 * @brief AudioDevices::vuMeterRMS gathers averages from a given audio data buffer by doing a root-mean-square (i.e. RMS) on all the
 * given samples.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param count The size of the audio data buffer.
 * @param buffer The given audio data buffer.
 * @return The averaged RMS of a given data buffer of audio samples.
 * @note <https://stackoverflow.com/questions/8227030/how-to-find-highest-volume-level-of-a-wav-file-using-c>
 */
float AudioDevices::vuMeterRMS(const size_t &count, float *buffer)
{
    float sample = 0;
    if (buffer != nullptr) {
        for (size_t i = 0; i < count; ++i) {
            sample += buffer[i] * buffer[i];
        }
    }

    float calc_val = std::sqrt(sample / static_cast<double>(count));
    return calc_val;
}

/**
 * @brief AudioDevices::calcAudioBufferTimeNeeded will calculate the maximum time required before an update is next required
 * from a circular buffer.
 * @param num_channels The number of audio channels we are dealing with regarding the stream in question.
 * @param fft_samples_per_line The number of FFT Samples per Line.
 * @param audio_buf_sampling_length The audio buffer's sampling length.
 * @param buf_size The total size of the buffer in question.
 * @return The amount of seconds you have in total before an update is next required from the circular buffer.
 */
float AudioDevices::calcAudioBufferTimeNeeded(const GkAudioChannels &num_channels, const size_t &fft_num_lines,
                                              const size_t &fft_samples_per_line, const size_t &audio_buf_sampling_length,
                                              const size_t &buf_size)
{
    int audio_channels = 0;

    switch (num_channels) {
    case GkAudioChannels::Mono:
        audio_channels = 1;
        break;
    case GkAudioChannels::Left:
        audio_channels = 1;
        break;
    case GkAudioChannels::Right:
        audio_channels = 1;
        break;
    case GkAudioChannels::Both:
        audio_channels = 2;
        break;
    case GkAudioChannels::Unknown:
        audio_channels = 0;
        break;
    default:
        audio_channels = 0;
        break;
    }

    if (audio_channels > 0) {
        // We are dealing with an audio device we can work with!
        float seconds = (audio_buf_sampling_length - fft_samples_per_line / (fft_num_lines - 1));
        if (gkFreqList->definitelyGreaterThan(seconds, 0.000, 3)) {
            return seconds;
        }

        return 0.000;
    } else {
        throw std::invalid_argument(tr("More than zero audio channels are required!").toStdString());
    }

    return -1;
}

/**
 * @brief AudioDevices::rtAudioVersionNumber returns the actual version of PortAudio
 * itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString AudioDevices::rtAudioVersionNumber()
{
    std::string version_no = RtAudio::getVersion();
    return QString::fromStdString(version_no);
}

/**
 * @brief AudioDevices::rtAudioVersionText returns version-specific information about
 * PortAudio itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString AudioDevices::rtAudioVersionText()
{
    std::string version_no = RtAudio::getVersion();
    return QString::fromStdString(version_no);
}
