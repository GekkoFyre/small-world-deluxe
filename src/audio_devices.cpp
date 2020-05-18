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

#include "audio_devices.hpp"
#include "pa_sinewave.hpp"
#include <iostream>
#include <cstdio>
#include <exception>
#include <sstream>
#include <utility>
#include <cstring>
#include <ostream>
#include <algorithm>
#include <cmath>
#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QApplication>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief AudioDevices::AudioDevices
 * @param parent
 * @note Core Audio APIs <https://docs.microsoft.com/en-us/windows/win32/api/_coreaudio/index>
 */
AudioDevices::AudioDevices(std::shared_ptr<GkLevelDb> gkDb, std::shared_ptr<GekkoFyre::FileIo> filePtr,
                           std::shared_ptr<StringFuncs> stringFuncs, QObject *parent)
{
    gkDekodeDb = std::move(gkDb);
    gkFileIo = std::move(filePtr);
    gkStringFuncs = std::move(stringFuncs);
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
        std::mutex init_port_audio_mtx;

        // The number of this device; this was saved to the Google LevelDB database as the user's preference
        int chosen_output_dev = 0;
        int chosen_input_dev = 0;

        chosen_output_dev = gkDekodeDb->read_audio_device_settings(true);
        chosen_input_dev = gkDekodeDb->read_audio_device_settings(false);

        enum_devices = defaultAudioDevices(portAudioSys);
        if (chosen_output_dev < 0) {
            //
            // Output audio device
            //
            std::lock_guard<std::mutex> lck_guard(init_port_audio_mtx);
            for (const auto &device: enum_devices) {
                if (device.default_dev && device.is_output_dev) { // Is an output device!
                    device_export.push_back(device);
                }
            }
        } else {
            // Gather more details about the chosen audio device
            std::lock_guard<std::mutex> lck_guard(init_port_audio_mtx);
            PaDeviceIndex output_dev = gkDekodeDb->read_audio_device_settings(true);
            GkDevice output_dev_details = gatherAudioDeviceDetails(portAudioSys, output_dev);
            device_export.push_back(output_dev_details);
        }

        if (chosen_input_dev < 0) {
            //
            // Input audio device
            //
            std::lock_guard<std::mutex> lck_guard(init_port_audio_mtx);
            for (const auto &device: enum_devices) {
                if (device.default_dev && !device.is_output_dev) { // Is an input device!
                    device_export.push_back(device);
                }
            }
        } else {
            // Gather more details about the chosen audio device
            std::lock_guard<std::mutex> lck_guard(init_port_audio_mtx);
            PaDeviceIndex input_dev = gkDekodeDb->read_audio_device_settings(false);
            GkDevice input_dev_details = gatherAudioDeviceDetails(portAudioSys, input_dev);
            device_export.push_back(input_dev_details);
        }

        return device_export;
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
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
 * @brief AudioDevices::enumSupportedStdSampleRates
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param inputParameters
 * @param outputParameters
 * @return
 */
std::vector<double> AudioDevices::enumSupportedStdSampleRates(const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters)
{
    double standardSampleRates[] = {
        8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
        44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1 /* negative terminated  list */
    };

    int i, printCount;
    PaError err;
    std::vector<double> supSampleRate; // Supported Sample Rates

    printCount = 0;
    for (i = 0; standardSampleRates[i] > 0; ++i) {
        err = Pa_IsFormatSupported(inputParameters, outputParameters, standardSampleRates[i]);
        if(err == paFormatIsSupported) {
            if (printCount == 0) {
                std::cout << tr("\t%1").arg(QString::number(standardSampleRates[i])).toStdString();
                supSampleRate.push_back(standardSampleRates[i]);
                printCount = 1;
            } else if (printCount == 4) {
                std::cout << tr("\n\t%1").arg(QString::number(standardSampleRates[i])).toStdString();
                supSampleRate.push_back(standardSampleRates[i]);
                printCount = 1;
            } else {
                std::cout << tr(", %1").arg(QString::number(standardSampleRates[i])).toStdString();
                supSampleRate.push_back(standardSampleRates[i]);
                ++printCount;
            }
        }
    }

    if (!printCount) {
        std::cout << tr("No sample rates supported!").toStdString() << std::endl;
    } else {
        std::cout << std::endl;
    }

    return supSampleRate;
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
    std::string vers_info;
    PaError err;

    numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        std::cerr << tr("ERROR: Pa_GetDeviceCount returned 0x").arg(QString::number(numDevices)).toStdString() << std::endl;
        err = numDevices;
        goto error;
    }

    std::cout << tr("Number of devices = %1").arg(QString::number(numDevices)).toStdString() << std::endl;
    for (int i = 0; i < numDevices; ++i) {
        GkDevice device;
        deviceInfo = Pa_GetDeviceInfo(i);
        std::cout << tr("--------------------------------------- Device #%1").arg(QString::number(i)).toStdString() << std::endl;
        device.dev_number = i;
        device.default_dev = false;
        device.device_info = *const_cast<PaDeviceInfo*>(deviceInfo);

        // Mark global and API specific default devices
        defaultDisplayed = 0;
        if (i == Pa_GetDefaultInputDevice()) {
            std::cout << tr("[ Default input").toStdString();
            defaultDisplayed = 1;
        } else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultInputDevice) {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            std::string hostInfoStr = hostInfo->name;
            std::cout << tr("[ Default %1 Input").arg(QString::fromStdString(hostInfoStr)).toStdString();
            defaultDisplayed = 1;
            device.default_dev = true;
        }

        if (i == Pa_GetDefaultOutputDevice()) {
            printf((defaultDisplayed ? "," : "["));
            std::cout << tr(" Default Output").toStdString();
            defaultDisplayed = 1;
        } else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultOutputDevice) {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            printf((defaultDisplayed ? "," : "["));
            std::string hostInfoStr = hostInfo->name;
            std::cout << tr(" Default %1 Output").arg(QString::fromStdString(hostInfoStr)).toStdString();
            defaultDisplayed = 1;
            device.default_dev = true;
        }

        if (defaultDisplayed) {
            std::cout << "]" << std::endl;
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
                device.is_output_dev = false;
                device.supp_sample_rates = enumSupportedStdSampleRates(&inputParameters, nullptr);
                device.stream_parameters = inputParameters;
                // Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultInputDevice;

                audio_devices.push_back(device);
            } else if (outputParameters.channelCount > 0) {
                device.is_output_dev = true;
                device.supp_sample_rates = enumSupportedStdSampleRates(nullptr, &outputParameters);
                device.stream_parameters = outputParameters;
                // Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultOutputDevice;

                audio_devices.push_back(device);
            } else {
                device.is_output_dev = boost::tribool::indeterminate_value;
                device.supp_sample_rates = enumSupportedStdSampleRates(&inputParameters, &outputParameters);

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
        std::mutex enum_audio_dev_mtx;

        std::lock_guard<std::mutex> audio_guard(enum_audio_dev_mtx);
        for (portaudio::System::DeviceIterator i = portAudioSys->devicesBegin(); i != portAudioSys->devicesEnd(); ++i) {
            // Mark both global and API specific default audio devices
            bool default_disp = false;
            std::ostringstream audio_device_name;
            const PaDeviceInfo *deviceInfo;
            GkDevice device;
            device.default_dev = false;
            device.default_disp = false;
            device.is_output_dev = false;
            device.dev_err = paNoError;
            device.dev_number = device_number;

            if ((*i).isSystemDefaultInputDevice()) {
                audio_device_name << tr("[ Default Input").toStdString();
                default_disp = true;
            } else if ((*i).isHostApiDefaultInputDevice()) {
                audio_device_name << tr("[ Default ").toStdString() << (*i).hostApi().name() << tr(" Input").toStdString();
                default_disp = true;
            }

            if ((*i).isSystemDefaultOutputDevice()) {
                audio_device_name << (default_disp ? "," : "[");
                audio_device_name << tr(" Default Output").toStdString();
                default_disp = true;
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

            device.dev_name_formatted = audio_device_name.str();
            device.host_type_id = Pa_GetHostApiInfo(deviceInfo->hostApi)->type;

            if (Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultInputDevice) {
                //
                // Input device
                //
                device.default_dev = true;
            }

            if (Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultOutputDevice) {
                //
                // Output device
                //
                device.default_dev = true;
            }

            if (!Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultInputDevice &&
                    !Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultOutputDevice) {
                //
                // Not the default audio device
                //
                device.default_dev = false;
            }

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
                device.is_output_dev = false;
                device.stream_parameters = *inputParameters.paStreamParameters();
                device.supp_sample_rates = enumSupportedStdSampleRates(inputParameters.paStreamParameters(), nullptr);
            } else if (device.dev_output_channel_count > 0) {
                //
                // Output device
                //
                device.is_output_dev = true;
                device.stream_parameters = *outputParameters.paStreamParameters();
                device.supp_sample_rates = enumSupportedStdSampleRates(nullptr, outputParameters.paStreamParameters());
            } else {
                //
                // Unable to determine whether just input or output device, so therefore assume it's both!
                //
                device.is_output_dev = boost::tribool::indeterminate_value;
                device.stream_parameters = PaStreamParameters();
                device.supp_sample_rates = enumSupportedStdSampleRates(inputParameters.paStreamParameters(),
                                                                       outputParameters.paStreamParameters());
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

                if (device.is_output_dev) {
                    //
                    // Output device
                    //
                    device.asio_min_latency = asio_device.device().defaultLowOutputLatency();
                    device.asio_max_latency = asio_device.device().defaultHighOutputLatency();
                } else if (!device.is_output_dev) {
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
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
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
PaStreamCallbackResult AudioDevices::testSinewave(portaudio::System &portAudioSys, const GkDevice device,
                                                  const bool &is_output_dev)
{
    try {
        std::mutex test_sinewave_mtx;
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
                                                     AUDIO_FRAMES_PER_BUFFER, paClipOff);
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
                                                                           device.dev_input_channel_count, sampleFormatConvert(device.def_sample_rate), false, prefInputLatency, nullptr);
            portaudio::StreamParameters recordParams(inputParamsRecord, portaudio::DirectionSpecificStreamParameters::null(), device.def_sample_rate,
                                                     AUDIO_FRAMES_PER_BUFFER, paClipOff);

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
void AudioDevices::volumeSetting()
{}

/**
 * @brief AudioDevices::vuMeter Enumerates out the Endpoint Volume Controls
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Define a UUID for MinGW <https://stackoverflow.com/questions/23977244/how-can-i-define-an-uuid-for-a-class-and-use-uuidof-in-the-same-way-for-g>
 * @return The Endpoint Volume Control values.
 */
double AudioDevices::vuMeter()
{
    // TODO: Implement this section!
    double ret_vol = 0;

    return ret_vol;
}

/**
 * @brief AudioDevices::sampleFormatConvert converts the sample rate into a readable format
 * for PortAudio's C++ bindings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sample_rate The desired sample rate to be converted.
 * @return The converted sample rate that is now readable by PortAudio's C++ bindings.
 */
portaudio::SampleDataFormat AudioDevices::sampleFormatConvert(const unsigned long sample_rate)
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
 * @brief AudioDevices::openPlaybackStream
 * @param device
 * @param stereo
 * @return
 */
PaStreamCallbackResult AudioDevices::openPlaybackStream(portaudio::System &portAudioSys, PaAudioBuf *audio_buf,
                                                        const GkDevice &device, const bool &stereo)
{
    try {
        if (audio_buf != nullptr) {
            std::mutex playback_stream_mtx;
            std::lock_guard<std::mutex> lck_guard(playback_stream_mtx);
            portaudio::SampleDataFormat prefOutputLatency = sampleFormatConvert(portAudioSys.deviceByIndex(device.stream_parameters.device).defaultLowOutputLatency());

            //
            // Speakers output stream
            //
            portaudio::DirectionSpecificStreamParameters outputParams(portAudioSys.deviceByIndex(device.stream_parameters.device),
                                                                      device.dev_output_channel_count, portaudio::FLOAT32,
                                                                      false, prefOutputLatency, nullptr);
            portaudio::StreamParameters playbackParams(portaudio::DirectionSpecificStreamParameters::null(), outputParams, device.def_sample_rate,
                                                       AUDIO_FRAMES_PER_BUFFER, paClipOff);
            portaudio::MemFunCallbackStream<PaAudioBuf> streamPlayback(playbackParams, *audio_buf, &PaAudioBuf::playbackCallback);

            streamPlayback.start();
            while (streamPlayback.isActive()) {
                portAudioSys.sleep(100);
            }

            streamPlayback.stop();
            streamPlayback.close();

            return paContinue;
        } else {
            throw std::runtime_error(tr("You must firstly choose an output audio device within the settings!").toStdString());
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
 * @brief AudioDevices::openRecordStream
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param portAudioSys
 * @param audio_buf
 * @param device
 * @param streamRecord
 * @param stereo
 * @return
 */
PaStreamCallbackResult AudioDevices::openRecordStream(portaudio::System &portAudioSys, PaAudioBuf **audio_buf,
                                                      const GkDevice &device,
                                                      portaudio::MemFunCallbackStream<PaAudioBuf> **stream_record_ptr,
                                                      const bool &stereo)
{
    try {
        if (audio_buf != nullptr) {
            std::mutex record_stream_mtx;
            std::lock_guard<std::mutex> lck_guard(record_stream_mtx);
            portaudio::SampleDataFormat prefDataFormat = sampleFormatConvert(portAudioSys.deviceByIndex(device.stream_parameters.device).defaultLowInputLatency());

            //
            // Recording input stream
            //
            portaudio::DirectionSpecificStreamParameters inputParamsRecord(portAudioSys.deviceByIndex(device.stream_parameters.device),
                                                                           device.dev_input_channel_count, prefDataFormat,
                                                                           false, portAudioSys.defaultInputDevice().defaultLowInputLatency(), nullptr);
            portaudio::StreamParameters recordParams(inputParamsRecord, portaudio::DirectionSpecificStreamParameters::null(), device.def_sample_rate,
                                                     AUDIO_FRAMES_PER_BUFFER, paClipOff);
            portaudio::MemFunCallbackStream<PaAudioBuf> *streamRecord = new portaudio::MemFunCallbackStream<PaAudioBuf>(recordParams, **audio_buf, &PaAudioBuf::recordCallback);

            *stream_record_ptr = streamRecord;
            streamRecord->start();

            return paContinue;
        } else {
            throw std::runtime_error(tr("You must firstly choose an input audio device within the settings!").toStdString());
        }
    } catch (const portaudio::PaException &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd, tr("Error!"), tr("[ PortAudio ] %1").arg(e.paErrorText()), MB_ICONERROR);
        DestroyWindow(hwnd);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("[ PortAudio ] %1").arg(e.paErrorText()));
        #endif
    } catch (const portaudio::PaCppException &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd, tr("Error!"), tr("[ PortAudioCpp ] %1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("[ PortAudioCpp ] %1").arg(e.what()));
        #endif
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd, tr("Error!"), tr("[ Generic exception ] %1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("[ Generic exception ] %1").arg(e.what()));
        #endif
    } catch (...) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), MB_ICONERROR);
        DestroyWindow(hwnd);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("An unknown exception has occurred. There are no further details."));
        #endif
    }

    return paAbort;
}

/**
 * @brief AudioDevices::filterPortAudioHostType
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_devices_vec
 * @return
 */
std::vector<GkDevice> AudioDevices::filterPortAudioHostType(const std::vector<GkDevice> &audio_devices_vec)
{
    try {
        if (!audio_devices_vec.empty()) {
            std::vector<GkDevice> host_res;
            for (const auto &audio_device: audio_devices_vec) {
                #if _WIN32 || __MINGW32__
                if (audio_device.host_type_id == PaHostApiTypeId::paASIO) {
                    // ASIO
                    // These are the most preferred, if available at all...
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }
                } else if (audio_device.host_type_id == PaHostApiTypeId::paDirectSound) {
                    // DirectSound
                    // Second-most preferred!
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }
                } else if (audio_device.host_type_id == PaHostApiTypeId::paMME) {
                    // MME
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }
                } else if (audio_device.host_type_id == PaHostApiTypeId::paWDMKS) {
                    // WDMKS
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }
                } else if (audio_device.host_type_id == PaHostApiTypeId::paWASAPI) {
                    // WASAPI
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }
                } else {
                    // The default!
                    // We can throw away these results...
                }
                #elif __linux__ || __MINGW32__
                if (audio_device.host_type_id == PaHostApiTypeId::paALSA) {
                    // ALSA
                    // These are the most preferred, if available at all...
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }
                } else if (audio_device.host_type_id == PaHostApiTypeId::paJACK) {
                    // JACK
                    // Second-most preferred!
                    if (filterAudioEnumPreexisting(host_res, audio_device) == false) {
                        host_res.push_back(audio_device);
                    }
                } else if (audio_device.host_type_id == PaHostApiTypeId::paOSS) {
                    // OSS?
                    // It is likely that we can throw away these results, unless I'm educated upon as to what this sub-system is...
                } else {
                    // Unsupported!
                    // We can throw away these results...
                }
                #endif
            }

            if (!host_res.empty()) {
                #if _WIN32 || __MINGW32__
                std::vector<GkDevice> filt_devices;
                std::copy_if(host_res.begin(), host_res.end(), std::back_inserter(filt_devices), [](GkDevice dev) { return dev.device_info.hostApi == PaHostApiTypeId::paDirectSound; });
                return filt_devices;
                #elif __linux__
                // TODO: Add Linux support as soon as possible!
                #endif
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A fatal error was encountered while enumerating audio devices within your system:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }

    return std::vector<GkDevice>();
}

/**
 * @brief AudioDevices::portAudioApiToStr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param interface
 * @return
 */
QString AudioDevices::portAudioApiToStr(const PaHostApiTypeId &interface)
{
    switch (interface) {
    case PaHostApiTypeId::paDirectSound:
        return tr("DirectSound");
    case PaHostApiTypeId::paMME:
        return tr("Microsoft Multimedia Environment (MME)");
    case PaHostApiTypeId::paASIO:
        return tr("ASIO");
    case PaHostApiTypeId::paSoundManager:
        return tr("Sound Manager");
    case PaHostApiTypeId::paCoreAudio:
        return tr("Core Audio");
    case PaHostApiTypeId::paOSS:
        return tr("OSS");
    case PaHostApiTypeId::paALSA:
        return tr("ALSA");
    case PaHostApiTypeId::paAL:
        return tr("AL");
    case PaHostApiTypeId::paBeOS:
        return tr("BeOS");
    case PaHostApiTypeId::paWDMKS:
        return tr("WDM/KS");
    case PaHostApiTypeId::paJACK:
        return tr("JACK");
    case PaHostApiTypeId::paWASAPI:
        return tr("Windows Audio Session API (WASAPI)");
    case PaHostApiTypeId::paAudioScienceHPI:
        return tr("AudioScience HPI");
    case PaHostApiTypeId::paInDevelopment:
        return tr("N/A");
    default:
        return tr("Unknown");
    }

    return tr("Unknown");
}

/**
 * @brief AudioDevices::portAudioApiChooser will filter out and list the available operating system's sound/multimedia APIs that
 * are available to the user via PortAudio, all dependent on how Small World Deluxe and its associated libraries were compiled.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_devices_vec The group of soundcard devices to use as a base of which to filter from.
 * @return The sound/multimedia APIs available to the user, and which they'll be able to ultimately pick from within
 * the Setting's Dialog.
 */
std::vector<PaHostApiTypeId> AudioDevices::portAudioApiChooser(const std::vector<GkDevice> &audio_devices_vec)
{
    try {
        if (!audio_devices_vec.empty()) {
            std::vector<PaHostApiTypeId> api_res;
            for (const auto &audio_device: audio_devices_vec) {
                switch (audio_device.host_type_id) {
                case PaHostApiTypeId::paDirectSound:
                    api_res.push_back(PaHostApiTypeId::paDirectSound);
                    break;
                case PaHostApiTypeId::paMME:
                    api_res.push_back(PaHostApiTypeId::paMME);
                    break;
                case PaHostApiTypeId::paASIO:
                    api_res.push_back(PaHostApiTypeId::paASIO);
                    break;
                case PaHostApiTypeId::paSoundManager:
                    api_res.push_back(PaHostApiTypeId::paSoundManager);
                    break;
                case PaHostApiTypeId::paCoreAudio:
                    api_res.push_back(PaHostApiTypeId::paCoreAudio);
                    break;
                case PaHostApiTypeId::paOSS:
                    api_res.push_back(PaHostApiTypeId::paOSS);
                    break;
                case PaHostApiTypeId::paALSA:
                    api_res.push_back(PaHostApiTypeId::paALSA);
                    break;
                case PaHostApiTypeId::paAL:
                    api_res.push_back(PaHostApiTypeId::paAL);
                    break;
                case PaHostApiTypeId::paBeOS:
                    api_res.push_back(PaHostApiTypeId::paBeOS);
                    break;
                case PaHostApiTypeId::paWDMKS:
                    api_res.push_back(PaHostApiTypeId::paWDMKS);
                    break;
                case PaHostApiTypeId::paJACK:
                    api_res.push_back(PaHostApiTypeId::paJACK);
                    break;
                case PaHostApiTypeId::paWASAPI:
                    api_res.push_back(PaHostApiTypeId::paWASAPI);
                    break;
                case PaHostApiTypeId::paAudioScienceHPI:
                    api_res.push_back(PaHostApiTypeId::paAudioScienceHPI);
                    break;
                case PaHostApiTypeId::paInDevelopment:
                    api_res.push_back(PaHostApiTypeId::paInDevelopment);
                    break;
                default:
                    break;
                }
            }

            if (!api_res.empty()) {
                // Filter for duplicates!
                std::sort(api_res.begin(), api_res.end());
                // api_res.erase(std::unique(api_res.begin(), api_res.end(), api_res.end()));
                return api_res;
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A fatal error was encountered while enumerating audio devices within your system:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }

    return std::vector<PaHostApiTypeId>();
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
        QMessageBox::warning(nullptr, tr("Error!"), tr("[ PortAudio ] %1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("[ PortAudioCpp ] %1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("[ Generic exception ] %1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    QApplication::exit(EXIT_FAILURE);
    return;
}
