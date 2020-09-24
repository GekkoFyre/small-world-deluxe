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
#include "src/pa_sinewave.hpp"
#include <cmath>
#include <cstdio>
#include <climits>
#include <sstream>
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
 * @brief AudioDevices::initPortAudio Initializes the PortAudio subsystem and finds default input/output sound devices
 * upon startup, provided there have been none saved in the Google LevelDB database. Otherwise it reads out what has
 * been saved, verifies that they still exist, and initializes those for execution later on in the application instead.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return A typical std::vector containing the audio device information, one each for input and output, of what soundcard
 * the application should make use of when this function is executed.
 */
std::vector<GkDevice> AudioDevices::initPortAudio(portaudio::System *portAudioSys)
{
    try {
        std::vector<GkDevice> enum_devices;
        std::vector<GkDevice> device_export;

        // The number of this device; this was saved to the Google LevelDB database as the user's preference
        PaDeviceIndex chosen_output_dev;
        PaDeviceIndex chosen_input_dev;

        chosen_output_dev = gkDekodeDb->read_audio_device_settings(true, true).toInt();
        chosen_input_dev = gkDekodeDb->read_audio_device_settings(false, true).toInt();

        enum_devices = defaultAudioDevices(portAudioSys);
        if (chosen_output_dev < 0) {
            //
            // Output audio device
            //
            std::lock_guard<std::mutex> lck_guard(init_port_audio_mtx);
            for (const auto &device: enum_devices) {
                if (device.default_dev && device.audio_src == GkAudioSource::Output) { // Is an output device!
                    device_export.push_back(device);
                }

                if (device.default_dev && device.audio_src == GkAudioSource::InputOutput) { // Is an output device!
                    device_export.push_back(device);
                }
            }
        } else {
            // Gather more details about the chosen audio device
            std::lock_guard<std::mutex> lck_guard(init_port_audio_mtx);
            GkDevice output_dev_details = gatherAudioDeviceDetails(portAudioSys, chosen_output_dev);
            device_export.push_back(output_dev_details);
        }

        if (chosen_input_dev < 0) {
            //
            // Input audio device
            //
            std::lock_guard<std::mutex> lck_guard(init_port_audio_mtx);
            for (const auto &device: enum_devices) {
                if (device.default_dev && device.audio_src == GkAudioSource::Input) { // Is an input device!
                    device_export.push_back(device);
                }

                if (device.default_dev && device.audio_src == GkAudioSource::InputOutput) { // Is an input device!
                    device_export.push_back(device);
                }
            }
        } else {
            // Gather more details about the chosen audio device
            std::lock_guard<std::mutex> lck_guard(init_port_audio_mtx);
            GkDevice input_dev_details = gatherAudioDeviceDetails(portAudioSys, chosen_input_dev);
            device_export.push_back(input_dev_details);
        }

        return device_export;
    } catch (const portaudio::PaException &e) {
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    }

    return std::vector<GkDevice>();
}

/**
 * @brief AudioDevices::defaultAudioDevices This will automatically determine the default audio devices for a
 * user's system, which is especially useful if nothing has been saved to the Google LevelDB database yet.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The default audio devices for a user's system.
 * @see GekkoFyre::AudioDevices::initPortAudio()
 */
std::vector<GkDevice> AudioDevices::defaultAudioDevices(portaudio::System *portAudioSys)
{
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> enum_devices;
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> exported_devices;

    enum_devices = filterPortAudioHostType(enumAudioDevicesCpp(portAudioSys));
    for (const auto &device: enum_devices) {
        if (device.default_dev == true) {
            // We have a default device!
            exported_devices.push_back(device);
        }
    }

    return exported_devices;
}

/**
 * @brief AudioDevices::enumSupportedStdSampleRates will test the supported sample rates of the given audio device, whether it
 * be an input or output audio device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audioParameters The necessary parameters required to make a connection to the enumerated audio device itself.
 * @param sampleRatesToTest The sample rates to test for.
 * @param isOutputDevice Whether this device in question is an output or input audio device.
 * @return The successfully supported sample rates for the given audio device.
 */
std::map<double, PaError> AudioDevices::enumSupportedStdSampleRates(const PaStreamParameters *audioParameters, const std::vector<double> &sampleRatesToTest,
                                                                    const bool &isOutputDevice)
{
    try {
        auto numCpuCores = gkSystem->getNumCpuCores();
        const size_t sample_rate_vec_size = sampleRatesToTest.size();
        size_t chunksToUse = 1;

        while (chunksToUse < (numCpuCores % sample_rate_vec_size)) {
            ++chunksToUse;
        }

        std::vector<double> vec_copy;
        vec_copy.assign(sampleRatesToTest.begin(), sampleRatesToTest.end());
        auto payload_tmp = gkStringFuncs->splitVec<double>(vec_copy, chunksToUse);
        std::map<double, PaError> mapped_data;
        std::map<double, std::future<PaError>> async_tasks;
        if (isOutputDevice) {
            //
            // Output audio device!
            //
            for (const auto &payload: payload_tmp) {
                for (const auto &sample_rate: payload) {
                    async_tasks.insert(std::make_pair(sample_rate, std::async(std::launch::async, &Pa_IsFormatSupported, nullptr, std::ref(audioParameters), std::ref(sample_rate))));
                }
            }

            for (auto &f: async_tasks) {
                PaError err;
                err = f.second.get();
                if (err == paFormatIsSupported) {
                    std::cout << tr("Sample rate of [ %1 ] is supported!").arg(QString::number(f.first)).toStdString() << std::endl;
                    mapped_data.insert(std::make_pair(f.first, err));
                }
            }
        } else {
            //
            // Input audio device!
            //
            for (const auto &payload: payload_tmp) {
                for (const auto &sample_rate: payload) {
                    async_tasks.insert(std::make_pair(sample_rate, std::async(std::launch::async, &Pa_IsFormatSupported, std::ref(audioParameters), nullptr, std::ref(sample_rate))));
                }
            }

            for (auto &f: async_tasks) {
                PaError err;
                err = f.second.get();
                if (err == paFormatIsSupported) {
                    std::cout << tr("Sample rate of [ %1 ] is supported!").arg(QString::number(f.first)).toStdString() << std::endl;
                    mapped_data.insert(std::make_pair(f.first, err));
                }
            }
        }

        return mapped_data;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("Error encountered while determining which audio device sample rates are supported! Error:\n\n%1")
        .arg(QString::fromStdString(e.what())).toStdString()));
    }

    return std::map<double, PaError>();
}

/**
 * @brief GekkoFyre::AudioDevices::AudioDevices::enumAudioDevices The non-C++ version of AudioDevices::enumAudioDevicesCpp, which
 * is useful for where the C++ bindings may not be used.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The found audio devices and their statistics.
 */
std::vector<GkDevice> AudioDevices::enumAudioDevices()
{
    std::vector<GkDevice> audio_devices;
    std::vector<GkDevice> audio_devices_cleaned;
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    int numDevices, defaultDisplayed;
    const PaDeviceInfo *deviceInfo;
    PaError err;

    numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        #ifdef GFYRE_PORTAUDIO_DBG_VERBOSITY_ENBL
        std::cerr << tr("ERROR: Pa_GetDeviceCount returned 0x").arg(QString::number(numDevices)).toStdString() << std::endl;
        #endif
        err = numDevices;
        goto error;
    }

    std::cout << tr("Number of devices = %1").arg(QString::number(numDevices)).toStdString() << std::endl;
    for (int i = 0; i < numDevices; ++i) {
        GkDevice device;
        deviceInfo = Pa_GetDeviceInfo(i);
        #ifdef GFYRE_PORTAUDIO_DBG_VERBOSITY_ENBL
        std::cout << tr("--------------------------------------- Device #%1").arg(QString::number(i)).toStdString() << std::endl;
        #endif
        device.dev_number = i;
        device.default_dev = false;
        device.device_info = *const_cast<PaDeviceInfo*>(deviceInfo);

        // Mark global and API specific default devices
        defaultDisplayed = 0;
        if (i == Pa_GetDefaultInputDevice()) {
            #ifdef GFYRE_PORTAUDIO_DBG_VERBOSITY_ENBL
            std::cout << tr("[ Default input").toStdString();
            #endif
            defaultDisplayed = 1;
        } else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultInputDevice) {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            std::string hostInfoStr = hostInfo->name;
            #ifdef GFYRE_PORTAUDIO_DBG_VERBOSITY_ENBL
            std::cout << tr("[ Default %1 Input").arg(QString::fromStdString(hostInfoStr)).toStdString();
            #endif
            defaultDisplayed = 1;
            device.default_dev = true;
        }

        if (i == Pa_GetDefaultOutputDevice()) {
            printf((defaultDisplayed ? "," : "["));
            #ifdef GFYRE_PORTAUDIO_DBG_VERBOSITY_ENBL
            std::cout << tr(" Default Output").toStdString();
            #endif
            defaultDisplayed = 1;
        } else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultOutputDevice) {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            printf((defaultDisplayed ? "," : "["));
            std::string hostInfoStr = hostInfo->name;
            #ifdef GFYRE_PORTAUDIO_DBG_VERBOSITY_ENBL
            std::cout << tr(" Default %1 Output").arg(QString::fromStdString(hostInfoStr)).toStdString();
            #endif
            defaultDisplayed = 1;
            device.default_dev = true;
        }

        if (defaultDisplayed) {
            #ifdef GFYRE_PORTAUDIO_DBG_VERBOSITY_ENBL
            std::cout << "]" << std::endl;
            #endif
            device.default_disp = true;
        } else {
            device.default_disp = false;
        }

        // Default values for ASIO specific information
        device.asio_min_latency = -1;
        device.asio_max_latency = -1;
        device.asio_granularity = -1;
        device.asio_pref_latency = -1;

        #ifdef WIN32
        #if PA_USE_ASIO
        // ASIO specific latency information
        if (Pa_GetHostApiInfo(deviceInfo->hostApi)->type == paASIO) {
            long minLatency, maxLatency, preferredLatency, granularity;
            err = PaAsio_GetAvailableLatencyValues(i, &minLatency, &maxLatency, &preferredLatency, &granularity);
            device.asio_min_latency = minLatency;
            device.asio_max_latency = maxLatency;
            device.asio_granularity = granularity;
            device.asio_pref_latency = preferredLatency;
            device.asio_err = err;
        }
        #endif // PA_USE_ASIO
        #endif // WIN32

        device.def_sample_rate = deviceInfo->defaultSampleRate;

        // Poll for standard sample rates
        // https://stackoverflow.com/questions/43068268/difference-between-paint32-paint16-paint24-pafloat32-etc
        inputParameters.device = i;
        inputParameters.channelCount = deviceInfo->maxInputChannels;
        inputParameters.sampleFormat = device.def_sample_rate;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultHighInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;

        outputParameters.device = i;
        outputParameters.channelCount = deviceInfo->maxOutputChannels;
        outputParameters.sampleFormat = device.def_sample_rate;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultHighOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        device.host_type_id = Pa_GetHostApiInfo(deviceInfo->hostApi)->type;

        // WARNING: Do not modify the settings below unless you know what you are doing!
        if (device.device_info.name[1]) {
            device.dev_output_channel_count = outputParameters.channelCount;
            device.dev_input_channel_count = inputParameters.channelCount;

            if (inputParameters.channelCount > 0) {
                device.audio_src = GkAudioSource::Input;
                device.stream_parameters = inputParameters;

                audio_devices.push_back(device);
            } else if (outputParameters.channelCount > 0) {
                device.audio_src = GkAudioSource::Output;
                device.stream_parameters = outputParameters;

                audio_devices.push_back(device);
            } else {
                device.audio_src = GkAudioSource::InputOutput;
                audio_devices.push_back(device);
            }
        }
    }

    // Clean the list of any erroneous devices that have wacky and unrealistic values
    for (const auto &device: audio_devices) {
        std::string audio_dev_name = device.device_info.name;
        if ((!audio_dev_name.empty()) && (device.default_disp)) {
            if ((device.dev_input_channel_count < AUDIO_INPUT_CHANNEL_MAX_LIMIT) && (device.dev_input_channel_count > AUDIO_INPUT_CHANNEL_MIN_LIMIT)) {
                if ((device.dev_output_channel_count < AUDIO_OUTPUT_CHANNEL_MAX_LIMIT) && (device.dev_output_channel_count > AUDIO_OUTPUT_CHANNEL_MIN_LIMIT)) {
                    audio_devices_cleaned.push_back(device);
                }
            }
        }
    }

    // http://portaudio.com/docs/v19-doxydocs/initializing_portaudio.html
    return audio_devices_cleaned;

    error:
    // Pa_Terminate();
    std::string err_msg = Pa_GetErrorText(err);
    std::cerr << tr("Error number: %1").arg(QString::number(err)).toStdString() << std::endl;
    std::cerr << tr("Error message: %1").arg(QString::fromStdString(err_msg)).toStdString() << std::endl;
    throw std::runtime_error(tr("An issue was encountered while enumerating audio devices!\n\n%1").arg(QString::fromStdString(err_msg)).toStdString());
}

/**
 * @brief AudioDevices::enumAudioDevicesCpp Enumerate out all the findable audio devices within the user's computer system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The found audio devices and their statistics.
 */
std::vector<GkDevice> AudioDevices::enumAudioDevicesCpp(portaudio::System *portAudioSys)
{
    try {
        std::vector<GkDevice> audio_devices_vec;                        // The vector responsible for storage all audio device sessions
        int device_number = 0;                                          // The index number for the input/output audio device in question

        std::lock_guard<std::mutex> audio_guard(enum_audio_dev_mtx);
        for (portaudio::System::DeviceIterator i = portAudioSys->devicesBegin(); i != portAudioSys->devicesEnd(); ++i) {
            // Mark both global and API specific default audio devices
            bool default_disp = false;
            std::ostringstream audio_device_name;
            const PaDeviceInfo *deviceInfo;
            GkDevice device;
            device.default_dev = false;
            device.default_disp = false;
            device.audio_src = GkAudioSource::Input;
            device.dev_err = paNoError;
            device.dev_number = device_number;

            if ((*i).isSystemDefaultInputDevice()) {
                audio_device_name << tr("[ Default Input").toStdString();
                default_disp = true;
                device.default_dev = true;
            } else if ((*i).isHostApiDefaultInputDevice()) {
                audio_device_name << tr("[ Default ").toStdString() << (*i).hostApi().name() << tr(" Input").toStdString();
                default_disp = true;
            }

            if ((*i).isSystemDefaultOutputDevice()) {
                audio_device_name << (default_disp ? "," : "[");
                audio_device_name << tr(" Default Output").toStdString();
                default_disp = true;
                device.default_dev = true;
            } else if ((*i).isHostApiDefaultOutputDevice()) {
                audio_device_name << (default_disp ? "," : "[");
                audio_device_name << tr(" Default ").toStdString() << (*i).hostApi().name() << tr(" Output").toStdString();
                default_disp = true;
            }

            if (default_disp) {
                audio_device_name << " ]";
            }

            //
            // Grab the unique Device Info
            //
            deviceInfo = Pa_GetDeviceInfo((*i).index());
            device.device_info = *const_cast<PaDeviceInfo*>(deviceInfo);

            device.chosen_audio_dev_str = QString::fromStdString(audio_device_name.str());
            device.host_type_id = Pa_GetHostApiInfo(deviceInfo->hostApi)->type;

            //
            // Default sample rate
            //
            device.def_sample_rate = (*i).defaultSampleRate();

            //
            // Poll for standard sample rates as associated with audio device in question
            //
            portaudio::DirectionSpecificStreamParameters inputParameters((*i), (*i).maxInputChannels(),
                                                                         sampleFormatConvert(device.def_sample_rate),
                                                                         true, 0.0, nullptr);
            portaudio::DirectionSpecificStreamParameters outputParameters((*i), (*i).maxOutputChannels(),
                                                                          sampleFormatConvert(device.def_sample_rate),
                                                                          true, 0.0, nullptr);

            if (inputParameters.numChannels() > 0) {
                device.dev_input_channel_count = inputParameters.numChannels();
            } else {
                device.dev_input_channel_count = -1;
            }

            if (outputParameters.numChannels() > 0) {
                device.dev_output_channel_count = outputParameters.numChannels();
            } else {
                device.dev_output_channel_count = -1;
            }

            if (device.dev_input_channel_count > 0) {
                //
                // Input device
                //
                device.audio_src = GkAudioSource::Input;
                device.stream_parameters = *inputParameters.paStreamParameters();
                device.cpp_stream_param = inputParameters;
            } else if (device.dev_output_channel_count > 0) {
                //
                // Output device
                //
                device.audio_src = GkAudioSource::Output;
                device.stream_parameters = *outputParameters.paStreamParameters();
                device.cpp_stream_param = outputParameters;
            } else {
                //
                // Unable to determine whether just input or output device, so therefore assume it's both!
                //
                device.audio_src = GkAudioSource::InputOutput;
                device.stream_parameters = PaStreamParameters();
            }

            // TODO: Possibly optimize this section of code?
            if (device.default_dev && audio_device_name.str() == "[ Default Input, Default Output ]") {
                device.audio_src = GkAudioSource::InputOutput;
            }

            //
            // ASIO specific settings
            //
            if ((*i).hostApi().typeId() == paASIO) {
                #ifdef WIN32
                #if PA_USE_ASIO
                portaudio::AsioDeviceAdapter asio_device(*i);
                device.asio_min_buffer_size = asio_device.minBufferSize();
                device.asio_max_buffer_size = asio_device.maxBufferSize();
                device.asio_pref_buffer_size = asio_device.preferredBufferSize();

                if (device.audio_src) {
                    //
                    // Output device
                    //
                    device.asio_min_latency = asio_device.device().defaultLowOutputLatency();
                    device.asio_max_latency = asio_device.device().defaultHighOutputLatency();
                } else if (!device.audio_src) {
                    //
                    // Input device
                    //
                    device.asio_min_latency = asio_device.device().defaultLowInputLatency();
                    device.asio_max_latency = asio_device.device().defaultHighInputLatency();
                } else {
                    //
                    // Unable to determine whether just input or output device, so therefore assume it's both!
                    //
                    device.asio_min_latency = -1; // Not an optimal value!
                    device.asio_max_latency = -1; // Not an optimal value!
                }

                if (asio_device.granularity() != -1) {
                    device.asio_granularity = asio_device.granularity();
                } else {
                    device.asio_granularity = -1;
                }
                #endif // PA_USE_ASIO
                #endif // WIN32
            }

            audio_devices_vec.push_back(device);
            ++device_number;
        }

        return audio_devices_vec;
    } catch (const portaudio::PaException &e) {
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    }

    return std::vector<GkDevice>();
}

/**
 * @brief AudioDevices::gatherAudioDeviceDetails Gathers further details on the audio device in question.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device_index The index of the audio device in question.
 * @return A GkDevice struct is returned.
 */
GkDevice AudioDevices::gatherAudioDeviceDetails(portaudio::System *portAudioSys, const PaDeviceIndex &pa_index)
{
    try {
        auto enum_devices_vec = enumAudioDevicesCpp(portAudioSys);

        for (const auto &device: enum_devices_vec) {
            if (pa_index == device.stream_parameters.device) {
                return device;
            }
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return GkDevice();
}

/**
 * @brief AudioDevices::testSinewave Performs a sinewave test on the given input/output audio device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device
 * @param is_output_dev
 * @param stereo
 * @return
 */
PaStreamCallbackResult AudioDevices::testSinewave(portaudio::System &portAudioSys, const GkDevice &device,
                                                  const bool &is_output_dev)
{
    try {
        std::lock_guard<std::mutex> lck_guard(test_sinewave_mtx);

        PaTime prefOutputLatency = portAudioSys.deviceByIndex(device.stream_parameters.device).defaultLowOutputLatency();
        PaTime prefInputLatency = portAudioSys.deviceByIndex(device.stream_parameters.device).defaultLowInputLatency();
        PaSinewave gkPaSinewave(AUDIO_TEST_SAMPLE_TABLE_SIZE);

        if (is_output_dev) {
            //
            // Speakers output stream
            //
            if (device.dev_output_channel_count <= 0) {
                throw std::invalid_argument(tr("Invalid number of output channels provided!").toStdString());
            }

            portaudio::DirectionSpecificStreamParameters outputParams(portAudioSys.deviceByIndex(device.stream_parameters.device),
                                                                   device.dev_output_channel_count, portaudio::FLOAT32, false, prefOutputLatency, nullptr);
            portaudio::StreamParameters playbackBeep(portaudio::DirectionSpecificStreamParameters::null(), outputParams, device.def_sample_rate,
                                                     AUDIO_FRAMES_PER_BUFFER, paPrimeOutputBuffersUsingStreamCallback);
            portaudio::MemFunCallbackStream<PaSinewave> streamPlaybackSine(playbackBeep, gkPaSinewave, &PaSinewave::generate);

            streamPlaybackSine.start();
            portAudioSys.sleep(AUDIO_SINE_WAVE_PLAYBACK_SECS * 1000); // Play the audio sample wave for the desired amount of seconds!
            streamPlaybackSine.stop();
            streamPlaybackSine.close();

            return paContinue;
        } else {
            //
            // Recording input stream
            //
            if (device.dev_input_channel_count <= 0) {
                throw std::invalid_argument(tr("Invalid number of input channels provided!").toStdString());
            }

            portaudio::DirectionSpecificStreamParameters inputParamsRecord(portAudioSys.deviceByIndex(device.stream_parameters.device),
                                                                           device.dev_input_channel_count,
                                                                           sampleFormatConvert(device.def_sample_rate),
                                                                           false, prefInputLatency, nullptr);
            portaudio::StreamParameters recordParams(inputParamsRecord, portaudio::DirectionSpecificStreamParameters::null(),
                                                     device.def_sample_rate,
                                                     AUDIO_FRAMES_PER_BUFFER, paNoFlag);

            portaudio::MemFunCallbackStream<PaSinewave> streamRecordSine(recordParams, gkPaSinewave, &PaSinewave::generate);

            streamRecordSine.start();
            portAudioSys.sleep(AUDIO_SINE_WAVE_PLAYBACK_SECS * 1000); // Play the audio sample wave for the desired amount of seconds!
            streamRecordSine.stop();
            streamRecordSine.close();

            return paContinue;
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return paAbort;
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
 * @brief AudioDevices::sampleFormatConvert converts the sample rate into a readable format
 * for PortAudio's C++ bindings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sample_rate The desired sample rate to be converted.
 * @return The converted sample rate that is now readable by PortAudio's C++ bindings.
 */
portaudio::SampleDataFormat AudioDevices::sampleFormatConvert(const unsigned long &sample_rate)
{
    switch (sample_rate) {
    case paFloat32:
        return portaudio::FLOAT32;
    case paInt32:
        return portaudio::INT32;
    case paInt24:
        return portaudio::INT24;
    case paInt16:
        return portaudio::INT16;
    case paInt8:
        return portaudio::INT8;
    case paUInt8:
        return portaudio::UINT8;
    default:
        return portaudio::INT16;
    }

    return portaudio::INT16;
}

/**
 * @brief AudioDevices::filterPortAudioHostType filters out the audio/multimedia devices on the user's system as they relate
 * to their associated operating system's API, via PortAudio. This is all dependent on how Small World Deluxe and its
 * associated libraries were compiled.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_devices_vec The group of soundcard/multimedia devices to use as a base of which to filter from.
 * @return The sorted soundcard/multimedia devices, filtered as per their API in relation to the operating system
 * via PortAudio.
 */
std::vector<GkDevice> AudioDevices::filterPortAudioHostType(const std::vector<GkDevice> &audio_devices_vec)
{
    try {
        if (!audio_devices_vec.empty()) {
            std::vector<GkDevice> host_res;
            for (const auto &audio_device: audio_devices_vec) {
                switch (audio_device.host_type_id) {
                #if defined(_WIN32) || defined(__MINGW64__)
                case PaHostApiTypeId::paDirectSound:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paMME:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paASIO:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paWDMKS:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paWASAPI:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                #elif __linux__
                case PaHostApiTypeId::paSoundManager:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paCoreAudio:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paJACK:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paALSA:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paAL:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paBeOS:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paOSS:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                #endif
                case PaHostApiTypeId::paAudioScienceHPI:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                case PaHostApiTypeId::paInDevelopment:
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }

                    break;
                default:
                    break;
                }
            }

            if (!host_res.empty()) {
                return host_res;
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A fatal error was encountered while enumerating audio devices within your system:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }

    return std::vector<GkDevice>();
}

/**
 * @brief AudioDevices::portAudioApiChooser will filter out and list the available operating system's sound/multimedia APIs that
 * are available to the user via PortAudio, all dependent on how Small World Deluxe and its associated libraries were compiled.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_devices_vec The group of soundcard/multimedia devices to use as a base of which to filter from.
 * @return The sound/multimedia APIs available to the user, and which they'll be able to ultimately pick from within
 * the Setting's Dialog.
 * @see AudioDevices::filterPortAudioHostType().
 */
QVector<PaHostApiTypeId> AudioDevices::portAudioApiChooser(const std::vector<GkDevice> &audio_devices_vec)
{
    try {
        if (!audio_devices_vec.empty()) {
            QVector<PaHostApiTypeId> api_res;
            for (const auto &audio_device: audio_devices_vec) {
                switch (audio_device.host_type_id) {
                case PaHostApiTypeId::paDirectSound:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paDirectSound);
                    }
                    break;
                case PaHostApiTypeId::paMME:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paMME);
                    }
                    break;
                case PaHostApiTypeId::paASIO:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paASIO);
                    }
                    break;
                case PaHostApiTypeId::paSoundManager:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paSoundManager);
                    }
                    break;
                case PaHostApiTypeId::paCoreAudio:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paCoreAudio);
                    }
                    break;
                case PaHostApiTypeId::paOSS:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paOSS);
                    }
                    break;
                case PaHostApiTypeId::paALSA:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paALSA);
                    }
                    break;
                case PaHostApiTypeId::paAL:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paAL);
                    }
                    break;
                case PaHostApiTypeId::paBeOS:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paBeOS);
                    }
                    break;
                case PaHostApiTypeId::paWDMKS:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paWDMKS);
                    }
                    break;
                case PaHostApiTypeId::paJACK:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paJACK);
                    }
                    break;
                case PaHostApiTypeId::paWASAPI:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paWASAPI);
                    }
                    break;
                case PaHostApiTypeId::paAudioScienceHPI:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paAudioScienceHPI);
                    }
                    break;
                case PaHostApiTypeId::paInDevelopment:
                    if (!api_res.contains(audio_device.host_type_id)) {
                        api_res.push_back(PaHostApiTypeId::paInDevelopment);
                    }
                    break;
                default:
                    break;
                }
            }

            if (!api_res.empty()) {
                // Filter for duplicates!
                std::sort(api_res.begin(), api_res.end());
                return api_res;
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A fatal error was encountered while enumerating audio devices within your system:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }

    return QVector<PaHostApiTypeId>();
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
 * @brief AudioDevices::portAudioVersionNumber returns the actual version of PortAudio
 * itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString AudioDevices::portAudioVersionNumber(const portaudio::System &portAudioSys)
{
    int version_ret = portAudioSys.version();
    return QString::number(version_ret);
}

/**
 * @brief AudioDevices::portAudioVersionText returns version-specific information about
 * PortAudio itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString AudioDevices::portAudioVersionText(const portaudio::System &portAudioSys)
{
    std::string version_text_ret = portAudioSys.versionText();
    return QString::fromStdString(version_text_ret);
}

/**
 * @brief AudioDevices::filterAudioEnumPreexisting A simple function for seeing if a new addition to the `std::vector<GkDevice>` will
 * add another value that is the exact same as a previous entry or at least, very similar.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device_vec The enumerated audio devices to compare against.
 * @param device_compare The comparison value to be used.
 * @return Whether there was a comparison match or not.
 */
bool AudioDevices::filterAudioEnumPreexisting(const std::vector<GkDevice> &device_vec, const GkDevice &device_compare)
{
    try {
        //
        // TODO: I have an idea to reimplement this with maybe using a points system? Dunno... need to ask a more seasoned coder.
        //
        if (!device_vec.empty()) {
            for (const auto &audio_device: device_vec) {
                if (std::strcmp(audio_device.device_info.name, device_compare.device_info.name) == 0) {
                    return true;
                }
            }
        }

        return false;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A fatal error was encountered while enumerating audio devices within your system:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }

    return false;
}

/**
 * @brief AudioDevices::portAudioErr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param err
 */
void AudioDevices::portAudioErr(const PaError &err)
{
    try {
        std::cerr << tr("An error occured while using the PortAudio stream").toStdString() << std::endl;
        std::cerr << tr("Error number: %1").arg(QString::number(err)).toStdString() << std::endl;
        std::cerr << tr("Error message: %1").arg(QString::fromStdString(Pa_GetErrorText(err))).toStdString() << std::endl;
        throw std::runtime_error(tr("An error occured within the audio stream:\n\n%1")
                                 .arg(QString::fromStdString(Pa_GetErrorText(err))).toStdString());
    } catch (const portaudio::PaException &e) {
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    }

    QApplication::exit(EXIT_FAILURE);
    return;
}
