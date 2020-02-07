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
#include <iostream>
#include <cstdio>
#include <exception>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <QString>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>

#ifdef _WIN32
#include <stringapiset.h>

#elif __linux__
#endif

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

#define ARRAY_LEN(x) ((int) (sizeof (x) / sizeof (x [0])))

#define CHECK_HR(x) if (FAILED(x)) { goto done; }

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != NULL)        \
   {                    \
      x->Release();     \
      x = NULL;         \
   }
#endif

#ifndef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(x) \
   if(x != NULL)             \
   {                         \
      delete[] x;            \
      x = NULL;              \
   }
#endif

// https://docs.microsoft.com/en-us/windows/win32/learnwin32/learn-to-program-for-windows
// https://docs.microsoft.com/en-us/cpp/cpp/declspec?view=vs-2019

std::mutex GekkoFyre::AudioDevices::spectro_mutex;
std::mutex GekkoFyre::AudioDevices::audio_mutex;

/**
 * @brief AudioDevices::AudioDevices
 * @param parent
 * @note Core Audio APIs <https://docs.microsoft.com/en-us/windows/win32/api/_coreaudio/index>
 */
AudioDevices::AudioDevices(portaudio::System &pa_sys, std::shared_ptr<DekodeDb> gkDb, std::shared_ptr<GekkoFyre::FileIo> filePtr,
                           QObject *parent) : QObject(parent)
{
    portAudioSys = &pa_sys;
    gkDekodeDb = gkDb;
    gkFileIo = filePtr;
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
std::vector<GkDevice> AudioDevices::initPortAudio()
{
    std::vector<GkDevice> enum_devices;
    std::vector<GkDevice> device_export;

    // The number of this device; this was saved to the Google LevelDB database as the user's preference
    int chosen_output_dev = 0;
    int chosen_input_dev = 0;

    enum_devices = enumAudioDevicesCpp();
    chosen_output_dev = gkDekodeDb->read_audio_device_settings(true);
    chosen_input_dev = gkDekodeDb->read_audio_device_settings(false);

    if ((chosen_output_dev == 0) && (chosen_input_dev == 0)) {
        // No user preferences have been saved yet!
        bool output_dev_exists = false;
        bool input_dev_exists = false;
        enum_devices = defaultAudioDevices();
        for (const auto &device: enum_devices) {
            if ((device.is_output_dev == true) && (output_dev_exists == false)) {
                // We are dealing with an output device
                device_export.push_back(device);
                output_dev_exists = true;
            }

            if ((device.is_output_dev) == false && (input_dev_exists == false)) {
                // We are dealing with an input device
                device_export.push_back(device);
                input_dev_exists = true;
            }
        }
    } else {
        // User preferences have indeed been saved!
        GkDevice output_device = gkDekodeDb->read_audio_details_settings(true);
        GkDevice input_device = gkDekodeDb->read_audio_details_settings(false);

        size_t total_output_devices = 0;
        size_t total_input_devices = 0;
        size_t output_dev_counter = 0;
        size_t input_dev_counter = 0;
        size_t total_audio_devics = enum_devices.size();

        // Count the number of input and output audio devices that have been enumerated
        for (const auto &device: enum_devices) {
            if ((chosen_output_dev > 0) && (output_device.is_output_dev == true)) { // Check to make sure that the preference does exist
                ++output_dev_counter;
            }

            if ((chosen_input_dev > 0) && (input_device.is_output_dev == false)) { // Check to make sure that the preference does exist
                ++input_dev_counter;
            }
        }

        if (output_dev_counter <= total_audio_devics) {
            total_output_devices = output_dev_counter;
        } else {
            throw std::runtime_error(tr("There is a miscount in the number of output audio devices!").toStdString());
        }

        if (input_dev_counter <= total_audio_devics) {
            total_input_devices = input_dev_counter;
        } else {
            throw std::runtime_error(tr("There is a miscount in the number of input audio devices!").toStdString());
        }

        size_t loop_counter = 0;
        for (const auto &device: enum_devices) {
            if (chosen_output_dev != 0) { // Check to make sure that the preference does exist
                if ((chosen_output_dev == device.dev_number) || (total_output_devices <= loop_counter)) {
                    // Grab the saved user preference for the output audio device
                    device_export.push_back(device);
                }
            }

            if (chosen_input_dev != 0) { // Check to make sure that the preference does exist
                if ((chosen_input_dev == device.dev_number) || (total_input_devices <= loop_counter)) {
                    // Grab the saved user preference for the input audio device
                    device_export.push_back(device);
                }
            }

            ++loop_counter;

            // Exit the loop early if we have all of our audio devices
            if ((device_export.size() >= 2) || (total_output_devices >= loop_counter) || (total_input_devices >= loop_counter)) {
                break;
            }
        }
    }

    return device_export;
}

/**
 * @brief AudioDevices::defaultAudioDevices This will automatically determine the default audio devices for a
 * user's system, which is especially useful if nothing has been saved to the Google LevelDB database yet.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The default audio devices for a user's system.
 * @see GekkoFyre::AudioDevices::initPortAudio()
 */
std::vector<GkDevice> AudioDevices::defaultAudioDevices()
{
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> enum_devices;
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> exported_devices;

    enum_devices = enumAudioDevicesCpp();
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
 * @brief AudioDevices::enumAudioDevicesCpp
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
std::vector<GkDevice> AudioDevices::enumAudioDevicesCpp()
{
    try {
        std::vector<GkDevice> audio_devices_vec;                        // The vector responsible for storage all audio device sessions
        int device_number = 0;                                          // The index number for the input/output audio device in question

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
            device.device_info = const_cast<PaDeviceInfo*>(deviceInfo);

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

            #ifdef WIN32
            //
            // ASIO specific settings
            //
            if ((*i).hostApi().typeId() == paASIO) {
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
            }
            #endif // WIN32

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
 * @brief AudioDevices::testSinewave
 * @author PortAudio <http://portaudio.com/docs/v19-doxydocs/paex__sine_8c_source.html>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_device
 * @param outputParameters
 * @see GekkoFyre::AudioDevices::paTestCallback()
 * @return
 */
void AudioDevices::testSinewave(const GkDevice &device)
{
    try {
        PaStream *stream;
        PaError err;
        paTestData data;
        PaStreamParameters outputParameters;
        PaStreamParameters inputParameters;
        if (device.is_output_dev == true) {
            outputParameters = device.stream_parameters;
        } else {
            inputParameters = device.stream_parameters;
        }

        qInfo() << tr("Audio Test: output sine wave. SR = %1, BufSize = %2").arg(QString::number(device.def_sample_rate))
                   .arg(QString::number(AUDIO_FRAMES_PER_BUFFER));

        // Initialise sinusoidal wavetable
        for (int i = 0; i < AUDIO_TEST_SAMPLE_TABLE_SIZE; i++) {
            data.sine[i] = ((float)sin(((double)i/(double)AUDIO_TEST_SAMPLE_TABLE_SIZE) * M_PI * 2.));
        }

        data.left_phase = data.right_phase = 0;

        err = Pa_Initialize();
        if (err != paNoError) {
            portAudioErr(err);
        }

        if (device.is_output_dev == true) {
            if (outputParameters.device == paNoDevice) {
                std::cerr << tr("Error: No default output device has been chosen!").toStdString() << std::endl;
                portAudioErr(err);
            }

            // http://portaudio.com/docs/v19-doxydocs/portaudio_8h.html#a443ad16338191af364e3be988014cbbe
            err = Pa_OpenStream(&stream, nullptr, &outputParameters, device.def_sample_rate, paFramesPerBufferUnspecified, paClipOff, paTestCallback, &data);
            if (err != paNoError) {
                portAudioErr(err);
            }
        } else {
            if (inputParameters.device == paNoDevice) {
                std::cerr << tr("Error: No default output device has been chosen!").toStdString() << std::endl;
                portAudioErr(err);
            }

            // http://portaudio.com/docs/v19-doxydocs/portaudio_8h.html#a443ad16338191af364e3be988014cbbe
            err = Pa_OpenStream(&stream, &inputParameters, nullptr, device.def_sample_rate, paFramesPerBufferUnspecified, paClipOff, paTestCallback, &data);
            if (err != paNoError) {
                portAudioErr(err);
            }
        }

        qInfo() << QString::fromStdString(data.message);
        err = Pa_SetStreamFinishedCallback(stream, &streamFinished);
        if (err != paNoError) {
            portAudioErr(err);
        }

        err = Pa_StartStream(stream);
        if (err != paNoError) {
            portAudioErr(err);
        }

        Pa_Sleep(AUDIO_TEST_SAMPLE_LENGTH_SEC * 1000);
        err = Pa_StopStream(stream);
        if (err != paNoError) {
            portAudioErr(err);
        }

        err = Pa_CloseStream(stream);
        if (err != paNoError) {
            portAudioErr(err);
        }

        // http://portaudio.com/docs/v19-doxydocs/initializing_portaudio.html
        // Pa_Terminate();
        qInfo() << tr("Audio test sample finished.");
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
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
    double ret_vol = 0;

    /*
    HRESULT hr = S_OK;
    IMMDevice *pDevice = nullptr;
    IMMDeviceEnumerator *pEnumerator = nullptr;
    IAudioSessionControl *pSessionControl = nullptr;
    IAudioSessionControl2 *pSessionControl2 = nullptr;
    IAudioSessionManager *pSessionManager = nullptr;

    CHECK_HR(hr = CoInitialize(nullptr));

    // Create the device enumerator
    CHECK_HR(hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(MMDeviceEnumerator), (void**)&pEnumerator));

    // Get the default audio device.
    CHECK_HR(hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice));

    // Get the audio client.
    CHECK_HR(hr = pDevice->Activate(__uuidof(IID_IAudioSessionManager), CLSCTX_ALL, NULL, (void**)&pSessionManager));

    // Get a reference to the session manager.
    CHECK_HR(hr = pSessionManager->GetAudioSessionControl(GUID_NULL, FALSE, &pSessionControl));

    // Get the extended session control interface pointer.
    CHECK_HR(hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2));

    // Check whether this is a system sound.
    CHECK_HR(hr = pSessionControl2->IsSystemSoundsSession());

    // If it is a system sound, opt out of the default
    // stream attenuation experience.
    CHECK_HR(hr = pSessionControl2->SetDuckingPreference(TRUE));

    done:
    // Clean up
    SAFE_RELEASE(pSessionControl2);
    SAFE_RELEASE(pSessionControl);
    SAFE_RELEASE(pEnumerator);
    SAFE_RELEASE(pDevice);
    */

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
 * @brief AudioDevices::portAudioVersionNumber returns the actual version of PortAudio
 * itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString AudioDevices::portAudioVersionNumber()
{
    return QString::number(portAudioSys->version());
}

/**
 * @brief AudioDevices::portAudioVersionText returns version-specific information about
 * PortAudio itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString AudioDevices::portAudioVersionText()
{
    return QString(portAudioSys->versionText());
}

/**
 * @brief AudioDevices::paTestCallback This is called by the PortAudio engine when audio is needed.
 * @author PortAudio <http://portaudio.com/docs/v19-doxydocs/paex__sine_8c_source.html>
 * @param inputBuffer
 * @param outputBuffer
 * @param framesPerBuffer
 * @param timeInfo
 * @param statusFlags
 * @param userData
 * @note It may called at the interrupt level on some machines so don't do anything that could mess up
 * the system like calling malloc() or free().
 * @see GekkoFyre::AudioDevices::enumAudioDevices(), GekkoFyre::AudioDevices::streamFinished()
 * @return
 */
int AudioDevices::paTestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                                 void *userData)
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;

    (void) timeInfo; // Prevent unused variable warnings
    (void) statusFlags;
    (void) inputBuffer;

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        *out++ = data->sine[data->left_phase];  // Left channel
        *out++ = data->sine[data->right_phase]; // Right channel
        data->left_phase += 1;
        if (data->left_phase >= AUDIO_TEST_SAMPLE_TABLE_SIZE) {
            data->left_phase -= AUDIO_TEST_SAMPLE_TABLE_SIZE;
        }

        data->right_phase += 3; // Higher pitch so we can distinguish left and right

        if (data->right_phase >= AUDIO_TEST_SAMPLE_TABLE_SIZE) {
            data->right_phase -= AUDIO_TEST_SAMPLE_TABLE_SIZE;
        }
    }

    return paContinue;
}

/**
 * @brief AudioDevices::streamFinished This routine is called by portaudio when playback is done.
 * @author PortAudio <http://portaudio.com/docs/v19-doxydocs/paex__sine_8c_source.html>
 * @param userData
 * @see GekkoFyre::AudioDevices::paTestCallback()
 */
void AudioDevices::streamFinished(void *userData)
{
    paTestData *data = (paTestData *) userData;
    qInfo() << tr("Stream Completed: %1").arg(QString::fromStdString(data->message));
}

/**
 * @brief AudioDevices::filterAudioInputEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param host_api_type
 * @return
 */
bool AudioDevices::filterAudioInputEnum(const PaHostApiTypeId &host_api_type)
{
    switch (host_api_type) {
    case paInDevelopment:
        return false;
    case paDirectSound:
        return false;
    case paMME:
        return true;
    case paASIO:
        return false;
    case paSoundManager:
        return false;
    case paCoreAudio:
        return false;
    case paOSS:
        return false;
    case paALSA:
        return false;
    case paAL:
        return false;
    case paBeOS:
        return false;
    case paWDMKS:
        return false;
    case paJACK:
        return false;
    case paWASAPI:
        return false;
    case paAudioScienceHPI:
        return false;
    default:
        return false;
    }

    return false;
}

/**
 * @brief AudioDevices::filterAudioOutputEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param host_api_type
 * @return
 */
bool AudioDevices::filterAudioOutputEnum(const PaHostApiTypeId &host_api_type)
{
    switch (host_api_type) {
    case paInDevelopment:
        return true;
    case paDirectSound:
        return true;
    case paMME:
        return false;
    case paASIO:
        return true;
    case paSoundManager:
        return true;
    case paCoreAudio:
        return true;
    case paOSS:
        return true;
    case paALSA:
        return true;
    case paAL:
        return true;
    case paBeOS:
        return true;
    case paWDMKS:
        return true;
    case paJACK:
        return true;
    case paWASAPI:
        return true;
    case paAudioScienceHPI:
        return true;
    default:
        return true;
    }

    return true;
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
