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
 **   Copyright (C) 2019. GekkoFyre.
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
 **   [ 1 ] - https://git.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "audio_devices.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <cstdio>
#include <exception>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <QDebug>

#ifdef _WIN32
#include <stringapiset.h>

/*
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <objbase.h>
#include <endpointvolume.h>
#include <CommCtrl.h>
*/

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
AudioDevices::AudioDevices(std::shared_ptr<DekodeDb> gkDb, std::shared_ptr<GekkoFyre::FileIo> filePtr,
                           QObject *parent) : QObject(parent)
{
    gkDekodeDb = gkDb;
    gkFileIo = filePtr;
    // gkStringFuncs = std::make_unique<GekkoFyre::StringFuncs>(new StringFuncs(this));
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
std::vector<Device> AudioDevices::initPortAudio()
{
    std::vector<Device> enum_devices;
    std::vector<Device> device_export;

    // The number of this device; this was saved to the Google LevelDB database as the user's preference
    int chosen_output_dev = 0;
    int chosen_input_dev = 0;

    enum_devices = enumAudioDevices();
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
        Device output_device = gkDekodeDb->read_audio_details_settings(true);
        Device input_device = gkDekodeDb->read_audio_details_settings(false);

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
std::vector<Device> AudioDevices::defaultAudioDevices()
{
    std::vector<GekkoFyre::Database::Settings::Audio::Device> enum_devices;
    std::vector<GekkoFyre::Database::Settings::Audio::Device> exported_devices;

    enum_devices = enumAudioDevices();
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
 * @brief AudioDevices::enumAudioDevices
 * @author PortAudio <http://portaudio.com/docs/v19-doxydocs/pa__devs_8c_source.html>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param inputParameters
 * @param outputParameters
 * @return
 */
std::vector<Device> AudioDevices::enumAudioDevices()
{
    std::vector<Device> audio_devices;
    std::vector<Device> audio_devices_cleaned;
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    int numDevices, defaultDisplayed;
    const PaDeviceInfo *deviceInfo;
    std::string vers_info;
    PaError err;

    err = Pa_Initialize();
    if(err != paNoError) {
        std::cerr << tr("ERROR: Pa_Initialize returned 0x%1").arg(QString::number(err)).toStdString() << std::endl;
        goto error;
    }

    numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        std::cerr << tr("ERROR: Pa_GetDeviceCount returned 0x").arg(QString::number(numDevices)).toStdString() << std::endl;
        err = numDevices;
        goto error;
    }

    std::cout << tr("Number of devices = %1").arg(QString::number(numDevices)).toStdString() << std::endl;
    for (int i = 0; i < numDevices; ++i) {
        Device device;
        deviceInfo = Pa_GetDeviceInfo(i);
        std::cout << tr("--------------------------------------- Device #%1").arg(QString::number(i)).toStdString() << std::endl;
        device.dev_number = i;
        device.default_dev = false;
        device.device_info = const_cast<PaDeviceInfo*>(deviceInfo);

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
        #ifdef PA_USE_ASIO
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
        inputParameters.sampleFormat = paFloat32;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultHighInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;

        outputParameters.device = i;
        outputParameters.channelCount = deviceInfo->maxOutputChannels;
        outputParameters.sampleFormat = paFloat32;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultHighOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        device.host_type_id = Pa_GetHostApiInfo(deviceInfo->hostApi)->type;

        // WARNING: Do not modify the settings below unless you know what you are doing!
        if (device.device_info->name[1]) {
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
        std::string audio_dev_name = device.device_info->name;
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
 * @brief AudioDevices::testSinewave
 * @author PortAudio <http://portaudio.com/docs/v19-doxydocs/paex__sine_8c_source.html>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_device
 * @param outputParameters
 * @see GekkoFyre::AudioDevices::paTestCallback()
 * @return
 */
void AudioDevices::testSinewave(const Device &device)
{
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
}

/**
 * @brief AudioDevices::createSpectrum
 * @author lincolnhard <https://github.com/lincolnhard/spectrogram-opencv>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
Spectrum *AudioDevices::createSpectrum(const size_t &specified_length)
{
    Spectrum *spec;
    spec = new Spectrum();
    if (spec == nullptr) {
        throw std::runtime_error(tr("Not enough memory!").toStdString());
    }

    std::lock_guard<std::mutex> lock(spectro_mutex);
    spec->speclen = specified_length;

    // mag_spec has values from [0..speclen] inclusive for 0 Hz to Nyquist.
    // time_domain has an extra element to be able to interpolate between
    // samples for better time precision, hoping to eliminate artifacts.
    spec->time_domain = reinterpret_cast<double *>(std::calloc(2 * specified_length + 1, sizeof(double)));
    spec->window = reinterpret_cast<double *>(std::calloc(2 * specified_length, sizeof(double)));
    spec->freq_domain = reinterpret_cast<double *>(std::calloc(2 * specified_length, sizeof(double)));
    spec->mag_spec = reinterpret_cast<double *>(std::calloc(specified_length + 1, sizeof(double)));

    if (spec->time_domain == nullptr || spec->window == nullptr || spec->freq_domain == nullptr || spec->mag_spec == nullptr) {
        throw std::runtime_error(tr("Not enough memory!").toStdString());
    }

    spec->plan = fftw_plan_r2r_1d(2 * specified_length, spec->time_domain, spec->freq_domain, FFTW_R2HC, FFTW_MEASURE | FFTW_PRESERVE_INPUT);

    if (spec->plan == nullptr) {
        throw std::runtime_error(tr("Unable to initialize FFTW mathematics library!").toStdString());
    }

    return spec;
}

void AudioDevices::spectrumColorMap(float value, double spec_floor_db, unsigned char colour[], bool gray_scale)
{
    static unsigned char map[][3] =
    {	/* These values were originally calculated for a dynamic range of 180dB. */
        {	255,	255,	255	},	/* -0dB */
        {	240,	254,	216	},	/* -10dB */
        {	242,	251,	185	},	/* -20dB */
        {	253,	245,	143	},	/* -30dB */
        {	253,	200,	102	},	/* -40dB */
        {	252,	144,	66	},	/* -50dB */
        {	252,	75,		32	},	/* -60dB */
        {	237,	28,		41	},	/* -70dB */
        {	214,	3,		64	},	/* -80dB */
        {	183,	3,		101	},	/* -90dB */
        {	157,	3,		122	},	/* -100dB */
        {	122,	3,		126	},	/* -110dB */
        {	80,		2,		110	},	/* -120dB */
        {	45,		2,		89	},	/* -130dB */
        {	19,		2,		70	},	/* -140dB */
        {	1,		3,		53	},	/* -150dB */
        {	1,		3,		37	},	/* -160dB */
        {	1,		2,		19	},	/* -170dB */
        {	0,		0,		0	},	/* -180dB */
    };

    float rem;
    int indx;

    if (gray_scale) {
        int gray; // The pixel value
        if (value <= spec_floor_db) {
            gray = 0;
        } else {
            gray = (255 - lrint(floor((value / spec_floor_db) * 255.0)));
            assert(gray >= 1 && gray <= 255);
        }

        colour[0] = colour[1] = colour[2] = gray;
        return;
    }

    if (value >= 0.0) {
        colour[0] = colour[1] = colour[2] = 255;
        return;
    }

    value = std::fabs(value * (-180.0 / spec_floor_db) * 0.1);
    indx = std::lrintf(std::floor(value));

    if (indx < 0) {
        throw std::runtime_error(tr("Error : colour map array index is %1").arg(QString::number(indx)).toStdString());
    }

    if (indx >= ARRAY_LEN(map) - 1) {
        colour[0] = colour[1] = colour[2] = 0;
        return;
    }

    rem = std::fmod(value, 1.0);

    colour[0] = std::lrintf((1.0 - rem) * map[indx][0] + rem * map[indx + 1][0]);
    colour[1] = std::lrintf((1.0 - rem) * map[indx][1] + rem * map[indx + 1][1]);
    colour[2] = std::lrintf((1.0 - rem) * map[indx][2] + rem * map[indx + 1][2]);

    return;
}

/**
 * @brief AudioDevices::is_2357 Helper function: does N have only 2, 3, 5 and 7 as its factors?
 * @author lincolnhard <https://github.com/lincolnhard/spectrogram-opencv>
 * @param n
 * @return
 */
bool AudioDevices::is_2357(size_t n)
{
    // Just eliminate all factors os 2, 3, 5 and 7 and see if 1 remains
    std::lock_guard<std::mutex> lock(spectro_mutex);
    while (n % 2 == 0) n /= 2;
    while (n % 3 == 0) n /= 3;
    while (n % 5 == 0) n /= 5;
    while (n % 7 == 0) n /= 7;
    return (n == 1);
}

/**
 * @brief AudioDevices::interpSpectrum
 * @author lincolnhard <https://github.com/lincolnhard/spectrogram-opencv>
 * @param mag
 * @param maglen
 * @param spec
 * @param speclen
 * @param min_freq
 * @param max_freq
 * @param samplerate
 */
void AudioDevices::interpSpectrum(float *mag, int maglen, const double *spec, int speclen, const double &min_freq,
                                  const double &max_freq, size_t samplerate)
{
    //
    // Map each output coordinate to where it depends on in the input array.
    // If there are more input values than output values, we need to average
    // a range of inputs.
    // If there are more output values than input values we do linear
    // interpolation between the two inputs values that a reverse-mapped
    // output value's coordinate falls between.
    //
    // spec points to an array with elements [0..speclen] inclusive
    // representing frequencies from 0 to samplerate/2 Hz. Map these to the
    // scale values min_freq to max_freq so that the bottom and top pixels
    // in the output represent the energy in the sound at min_ and max_freq Hz.
    //

    for (size_t k = 0; k < maglen; k++) {
        // Average the pixels in the range it comes from
        double current = magindex_to_specindex(speclen, maglen, k, min_freq, max_freq, samplerate, 0.0);
        double next = magindex_to_specindex(speclen, maglen, k + 1, min_freq, max_freq, samplerate, 0.0);

        // Range check: can happen if --max-freq > samplerate / 2
        if (current > speclen) {
            mag[k] = 0.0;
            return;
        }

        if (next > current + 1) {
            // The output indices are more sparse than the input indices
            // so average the range of input indices that map to this output,
            // making sure not to exceed the input array (0..speclen inclusive)
            //
            // Take a proportional part of the first sample
            double count = (1.0 - (current - std::floor(current)));
            double sum = (spec[(int)current] * count);
            while ((current += 1.0) < next && (int)current <= speclen) {
                sum += spec[(int)current];
                count += 1.0;
            }

            // And part of the last one
            if ((int)next <= speclen) {
                sum += spec[(int)next] * (next - std::floor(next));
                count += next - std::floor(next);
            }

            mag[k] = sum / count;
        } else {
            // The output indices are more densely packed than the input indices
            // so interpolate between input values to generate more output values
            //
            // Take a weighted average of the nearest values
            mag[k] = spec[(int) current] * (1.0 - (current - std::floor(current))) + spec[(int) current + 1] * (current - std::floor(current));
        }
    }

    return;
}

/**
 * @brief AudioDevices::magindex_to_specindex Helper function: Map the index for an output pixel in a column to an index into the
 * FFT result representing the same frequency.
 * magindex is from 0 to maglen-1, representing min_freq to max_freq Hz.
 * Return values from are from 0 to speclen representing frequencies from 0 to the Nyquist frequency.
 * The result is a floating point number as it may fall between elements, allowing the caller to interpolate onto the input array.
 * @author lincolnhard <https://github.com/lincolnhard/spectrogram-opencv>
 * @param speclen
 * @param maglen
 * @param magindex
 * @param min_freq
 * @param max_freq
 * @param samplerate
 * @param log_freq
 * @return
 */
double AudioDevices::magindex_to_specindex(int speclen, int maglen, size_t magindex, double min_freq, double max_freq,
                                           size_t samplerate, bool log_freq)
{
    double freq; // The frequency that this output value represents

    if (!log_freq) {
        freq = min_freq + (max_freq - min_freq) * magindex / (maglen - 1);
    } else {
        freq = min_freq * std::pow(max_freq / min_freq, (double)magindex / (maglen - 1));
    }

    return (freq * speclen / (samplerate / 2));
}

/**
 * @brief AudioDevices::is_good_speclen Helper function: is N a "fast" value for the FFT size?
 * @author lincolnhard <https://github.com/lincolnhard/spectrogram-opencv>
 * @param n
 * @return
 */
bool AudioDevices::is_good_speclen(size_t n)
{
    // It wants n, 11*n, 13*n but not (11 * 13 * n)
    // where n only has as factors 2, 3, 5 and 7
    if (n % (11 * 13) == 0) {
        return 0; // No good
    }

    return is_2357 (n) || ((n % 11 == 0) && is_2357 (n / 11)) || ((n % 13 == 0) && is_2357 (n / 13));
}

/**
 * @brief AudioDevices::destroySpectrum
 * @author lincolnhard <https://github.com/lincolnhard/spectrogram-opencv>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param spectrum
 */
void AudioDevices::destroySpectrum(Spectrum *spectrum)
{
    std::lock_guard<std::mutex> lock(spectro_mutex);
    fftw_destroy_plan(spectrum->plan);
    free(spectrum->time_domain);
    free(spectrum->window);
    free(spectrum->freq_domain);
    free(spectrum->mag_spec);
    delete spectrum;
}

/**
 * @brief AudioDevices::spectrogram
 * @author lincolnhard <https://github.com/lincolnhard/spectrogram-opencv>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Using Unique Lock <https://stackoverflow.com/questions/14709233/how-to-use-create-unique-lock-in-c>.
 */
void AudioDevices::spectrogram(Spectrum *spectrum, const int &pixels_width_per_sec,
                               const float &total_secs, const size_t &spectro_win_width,
                               const size_t &spectro_win_height, const float &win_secs,
                               const float &step_secs)
{
    std::unique_lock<std::mutex> u_lock(audio_mutex);
    cv::Mat im(spectro_win_height, spectro_win_width, CV_8UC3);
    u_lock.unlock();
    unsigned char *imdata = im.data;
    unsigned char colour[3] = {0, 0, 0};
    double max_freq = spectrum->audio_dev.def_sample_rate;
    double min_freq = 150;

    float **mag_spec = reinterpret_cast<float **>(calloc(spectro_win_width, sizeof(float *)));
    for (size_t i = 0; i < spectro_win_width; ++i) {
        if ((mag_spec[i] = reinterpret_cast<float *>(calloc(spectro_win_height, sizeof(float)))) == nullptr) {
            throw std::runtime_error(tr("Not enough memory!").toStdString());
        }
    }

    const size_t steps = (1 + (total_secs - win_secs) / step_secs);
    for (size_t i = 0; i < steps; ++i) {
        for (size_t j = 0; j < spectro_win_width; ++j) {
            std::lock_guard<std::mutex> lock(spectro_mutex);
            size_t data_len = 2 * spectrum->speclen;
            double *data = spectrum->time_domain;
            memset(data, 0, 2 * spectrum->speclen * sizeof(double));

            // TODO: Read data from PortAudio here...

            calcMagnSpectrum(spectrum);
            interpSpectrum(mag_spec[j], spectro_win_height, spectrum->mag_spec, spectrum->speclen, min_freq, max_freq, spectrum->audio_dev.def_sample_rate);
        }

        // Draw spectrogram
        for (size_t j = 0; j < spectro_win_width; ++j) {
            for (size_t k = 0; k < spectro_win_height; ++k) {
                std::lock_guard<std::mutex> lock(spectro_mutex);
                mag_spec[j][k] /= spectrum->mag_to_norm;
                mag_spec[j][k] = (mag_spec[j][k] < spectrum->linear_floor) ? AUDIO_SPEC_FLOOR_DECIBELS : 20.0 * std::log10(mag_spec[j][k]);
                spectrumColorMap(mag_spec[j][k], AUDIO_SPEC_FLOOR_DECIBELS, colour, false);
                imdata[((spectro_win_height - 1 - k) * im.cols + j) * 3] = colour[2];
                imdata[((spectro_win_height - 1 - k) * im.cols + j) * 3 + 1] = colour[1];
                imdata[((spectro_win_height - 1 - k) * im.cols + j) * 3 + 2] = colour[0];
            }
        }

        u_lock.lock();
        cv::imshow(tr("Spectrograph").toStdString(), im);
        unsigned char key = cv::waitKey(10);
        u_lock.unlock();
        if (key == 27) {
            break;
        }
    }

    u_lock.lock();
    destroySpectrum(spectrum);
    for (size_t i = 0; i < spectro_win_width; ++i) {
        free(mag_spec[i]);
    }

    free(mag_spec);
    u_lock.unlock();
    return;
}

/**
 * @brief AudioDevices::calcMagnSpectrum
 * @author lincolnhard <https://github.com/lincolnhard/spectrogram-opencv>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param spectrum
 */
double AudioDevices::calcMagnSpectrum(Spectrum *spectrum)
{
    double max_val = 0.0;
    int k = 0;
    int freqlen = 0;

    freqlen = (2 * spectrum->speclen);

    fftw_execute (spectrum->plan);

    // Convert from FFTW's "half complex" format to an array of magnitudes
    // In HC format, the values are stored:
    // r0, r1, r2 ... r(n/2), i(n+1)/2-1 .. i2, i1
    max_val = spectrum->mag_spec[0] = fabs(spectrum->freq_domain[0]);

    for (k = 1; k < spectrum->speclen; k++) {
        double re = spectrum->freq_domain[k];
        double im = spectrum->freq_domain[freqlen - k];
        spectrum->mag_spec[k] = std::sqrt(re * re + im * im);
        max_val = std::max(max_val, spectrum->mag_spec[k]);
    }

    // Lastly add the point for the Nyquist frequency
    spectrum->mag_spec[spectrum->speclen] = std::fabs(spectrum->freq_domain[spectrum->speclen]);

    return max_val;
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
    std::cerr << tr("An error occured while using the PortAudio stream").toStdString() << std::endl;
    std::cerr << tr("Error number: %1").arg(QString::number(err)).toStdString() << std::endl;
    std::cerr << tr("Error message: %1").arg(QString::fromStdString(Pa_GetErrorText(err))).toStdString() << std::endl;
    throw std::runtime_error(tr("An error occured within the audio stream:\n\n%1")
                             .arg(QString::fromStdString(Pa_GetErrorText(err))).toStdString());
}
