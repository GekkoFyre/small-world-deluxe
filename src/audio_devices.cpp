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

#define SCALE (1.0)

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
    setParent(parent);

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
 * @note <https://doc.qt.io/qt-5/audiooverview.html>.
 */
std::list<std::pair<QAudioDeviceInfo, GkDevice>> AudioDevices::enumAudioDevicesCpp(const QList<QAudioDeviceInfo> &audioDeviceInfo)
{
    try {
        std::lock_guard<std::mutex> audio_guard(enum_audio_dev_mtx);
        std::list<std::pair<QAudioDeviceInfo, GkDevice>> audio_devices_vec;

        for (const auto &device: audioDeviceInfo) {
            #if defined(_WIN32) || defined(__MINGW64__)
            if (!device.isNull() && !device.supportedSampleRates().empty() && !device.supportedSampleSizes().empty() && !device.supportedChannelCounts().empty()) {
            #elif __linux__
            if ((!device.isNull() && !device.supportedSampleRates().empty() && !device.supportedSampleSizes().empty() && !device.supportedChannelCounts().empty())
            && (device.deviceName().contains("default") || device.deviceName().contains("pulse") || device.deviceName().contains("alsa") ||
            device.deviceName().contains("qnx"))) {
            #endif
                bool at_least_mono = false; // Do we have at least a Mono channel?
                for (const auto &channel: device.supportedChannelCounts()) {
                    if (channel > 0) {
                        at_least_mono = true;
                    }
                }

                if (at_least_mono) {
                    GkDevice gkDevice;
                    gkDevice.audio_dev_str = device.deviceName();
                    gkDevice.default_input_dev = false;
                    gkDevice.default_output_dev = false;
                    gkDevice.audio_device_info = QAudioDeviceInfo(device);

                    if (gkDevice.audio_dev_str == QAudioDeviceInfo::defaultInputDevice().deviceName()) {
                        gkDevice.default_input_dev = true;
                    }

                    if (gkDevice.audio_dev_str == QAudioDeviceInfo::defaultOutputDevice().deviceName()) {
                        gkDevice.default_output_dev = true;
                    }

                    audio_devices_vec.emplace_back(std::make_pair(device, gkDevice));
                }
            }
        }

        return audio_devices_vec;
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    }

    return std::list<std::pair<QAudioDeviceInfo, GkDevice>>();
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
    // TODO: Update this so that it mentions the audio backend!
    return QCoreApplication::applicationVersion();
}

/**
 * @brief AudioDevices::rtAudioVersionText returns version-specific information about
 * PortAudio itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString AudioDevices::rtAudioVersionText()
{
    // TODO: Update this so that it mentions the audio backend!
    return QCoreApplication::applicationVersion();
}
