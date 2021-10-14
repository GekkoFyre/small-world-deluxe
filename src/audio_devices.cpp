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
 **   Copyright (C) 2020 - 2021. GekkoFyre.
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

/**
 * @brief GkAudioDevices::GkAudioDevices
 * @param parent
 * @note Core Audio APIs <https://docs.microsoft.com/en-us/windows/win32/api/_coreaudio/index>
 */
GkAudioDevices::GkAudioDevices(QPointer<GkLevelDb> gkDb, QPointer<FileIo> filePtr,
                               QPointer<GekkoFyre::GkFrequencies> freqList, QPointer<StringFuncs> stringFuncs,
                               QPointer<GekkoFyre::GkEventLogger> eventLogger, QPointer<GekkoFyre::GkSystem> systemPtr,
                               QObject *parent) : QObject(parent)
{
    setParent(parent);

    gkDekodeDb = std::move(gkDb);
    gkFileIo = std::move(filePtr);
    gkFreqList = std::move(freqList);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);
    gkSystem = std::move(systemPtr);
}

GkAudioDevices::~GkAudioDevices()
{}

/**
 * @brief GkAudioDevices::volumeSetting
 * @note Michael Satran & Mike Jacobs <https://docs.microsoft.com/en-us/windows/win32/coreaudio/endpoint-volume-controls>
 */
void GkAudioDevices::systemVolumeSetting()
{}

/**
 * @brief GkAudioDevices::vuMeter processes a raw sound buffer and outputs a possible volume level, in decibels (dB).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param channels How many audio channels there are, since they are interleaved. The first byte is the first channel, second
 * byte is the second channel, etc.
 * @param count The amount of `frames per buffer`.
 * @param buffer The raw audiological data.
 * @return The volume level, in decibels (dB).
 * @note RobertT <https://stackoverflow.com/questions/2445756/how-can-i-calculate-audio-db-level>,
 * Vassilis <https://stackoverflow.com/questions/37963115/how-to-make-smooth-levelpeak-meter-with-qt>
 */
float GkAudioDevices::vuMeter(const int &channels, const int &count, float *buffer)
{
    float dB_val = 0.0f;
    if (buffer != nullptr) {
        float max_val = buffer[0];

        // Find maximum!
        // Traverse the array elements from second and compare every element with current maximum...
        for (int i = 1; i < (count * channels); ++i) {
            if (buffer[++i] > max_val) {
                max_val = buffer[++i];
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
 * @brief GkAudioDevices::vuMeterPeakAmplitude traverses the buffered data array and compares every element with the current
 * maximum to see if there's a new maximum value to be had.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param count The size of the audio data buffer.
 * @param buffer The given audio data buffer.
 * @return The maximum, peak audio signal for a given lot of buffered data.
 */
float GkAudioDevices::vuMeterPeakAmplitude(const size_t &count, float *buffer)
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
 * @brief GkAudioDevices::vuMeterRMS gathers averages from a given audio data buffer by doing a root-mean-square (i.e. RMS) on all the
 * given samples.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param count The size of the audio data buffer.
 * @param buffer The given audio data buffer.
 * @return The averaged RMS of a given data buffer of audio samples.
 * @note <https://stackoverflow.com/questions/8227030/how-to-find-highest-volume-level-of-a-wav-file-using-c>
 */
float GkAudioDevices::vuMeterRMS(const size_t &count, float *buffer)
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
 * @brief GkAudioDevices::calcAudioBufferTimeNeeded will calculate the maximum time required before an update is next required
 * from a circular buffer.
 * @param num_channels The number of audio channels we are dealing with regarding the stream in question.
 * @param fft_samples_per_line The number of FFT Samples per Line.
 * @param audio_buf_sampling_length The audio buffer's sampling length.
 * @param buf_size The total size of the buffer in question.
 * @return The amount of seconds you have in total before an update is next required from the circular buffer.
 */
float GkAudioDevices::calcAudioBufferTimeNeeded(const GkAudioChannels &num_channels, const size_t &fft_num_lines,
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
    case GkAudioChannels::Stereo:
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
 * @brief GkAudioDevices::checkAlErrors A function to make OpenAL error detection a little bit easier.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>,
 * IndieGameDev.net <https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/>
 * @param filename
 * @param line
 * @return
 */
bool GkAudioDevices::checkAlErrors(const std::string &filename, const std::uint_fast32_t line)
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << tr("***ERROR*** (").toStdString() << filename << ": " << line << ")" << std::endl;
        switch (error) {
            case AL_INVALID_NAME: {
                QString err_invalid_name = tr("AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function");
                std::cerr << err_invalid_name.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_invalid_name, QMessageBox::Ok);
            }

                break;
            case AL_INVALID_ENUM: {
                QString err_invalid_enum = tr("AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function");
                std::cerr << err_invalid_enum.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_invalid_enum, QMessageBox::Ok);
            }
                break;
            case AL_INVALID_VALUE: {
                QString err_invalid_value = tr("AL_INVALID_VALUE: an invalid value was passed to an OpenAL function");
                std::cerr << err_invalid_value.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_invalid_value, QMessageBox::Ok);
            }

                break;
            case AL_INVALID_OPERATION: {
                QString err_invalid_operation = tr("AL_INVALID_OPERATION: the requested operation is not valid");
                std::cerr << err_invalid_operation.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_invalid_operation, QMessageBox::Ok);
            }

                break;
            case AL_OUT_OF_MEMORY: {
                QString err_out_of_memory = tr("AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory");
                std::cerr << err_out_of_memory.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_out_of_memory, QMessageBox::Ok);
            }

                break;
            default: {
                QString err_unknown = tr("UNKNOWN AL ERROR: ");
                std::cerr << err_unknown.toStdString() << error;
                QMessageBox::warning(nullptr, tr("Error!"), err_unknown, QMessageBox::Ok);
            }

                break;
        }

        std::cerr << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief GkAudioDevices::checkAlcErrors A function to make OpenAL error detection a little bit easier.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>,
 * IndieGameDev.net <https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/>
 * @param filename
 * @param line
 * @return
 */
bool GkAudioDevices::checkAlcErrors(const std::string &filename, const std::uint_fast32_t line, ALCdevice *device)
{
    ALCenum error = alcGetError(device);
    if (error != ALC_NO_ERROR) {
        std::cerr << tr("***ERROR*** (").toStdString() << filename << ": " << line << ")" << std::endl;
        switch (error) {
            case ALC_INVALID_VALUE: {
                QString err_invalid_value = tr("ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function");
                std::cerr << err_invalid_value.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_invalid_value, QMessageBox::Ok);
            }

                break;
            case ALC_INVALID_DEVICE: {
                QString err_invalid_device = tr("ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function");
                std::cerr << err_invalid_device.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_invalid_device, QMessageBox::Ok);
            }
                break;
            case ALC_INVALID_CONTEXT: {
                QString err_invalid_context = tr("ALC_INVALID_CONTEXT: a bad context was passed to an OpenAL function");
                std::cerr << err_invalid_context.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_invalid_context, QMessageBox::Ok);
            }

                break;
            case ALC_INVALID_ENUM: {
                QString err_invalid_enum = tr("ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function");
                std::cerr << err_invalid_enum.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_invalid_enum, QMessageBox::Ok);
            }

                break;
            case ALC_OUT_OF_MEMORY: {
                QString err_out_of_memory = tr("ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function");
                std::cerr << err_out_of_memory.toStdString();
                QMessageBox::warning(nullptr, tr("Error!"), err_out_of_memory, QMessageBox::Ok);
            }

                break;
            default: {
                QString err_unknown = tr("UNKNOWN ALC ERROR: ");
                std::cerr << err_unknown.toStdString() << error;
                QMessageBox::warning(nullptr, tr("Error!"), err_unknown, QMessageBox::Ok);
            }

                break;
        }

        std::cerr << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief GkAudioDevices::enumerateAudioDevices will list/enumerate the audio devices (both input and output as specified)
 * for the given end-user's computer system, as provided by the OpenAL audio library.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param devices The OpenAL audio library pointer to use.
 * @param is_output_dev Whether we are working with output devices or input.
 * @return The enumerated list of audio devices.
 */
QList<GkDevice> GkAudioDevices::enumerateAudioDevices(const ALCenum param) {
    //
    // Prior to attempting an enumeration, OpenAL provides an extension querying mechanism which allows
    // you to know whether the runtime OpenAL implementation supports a specific extension. In our case,
    // we want to check whether OpenAL supports enumerating devices...
    //
    ALboolean enumeration;
    enumeration = alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT");
    if (enumeration == AL_FALSE) {
        std::throw_with_nested(std::runtime_error(tr("The version of OpenAL that is currently installed is not supported! Please upgrade to a later version before continuing.").toStdString()));
    }

    const char *ptr = alcGetString(nullptr, param);
    QStringList devicesList;
    do {
        devicesList.push_back(QString::fromStdString(ptr));
        ptr += devicesList.back().size() + 1;
    } while(*(ptr + 1) != '\0');

    QList<GkDevice> device_list;
    for (const auto &dev: devicesList) {
        GkDevice audio;
        if (param == ALC_DEVICE_SPECIFIER) {
            audio.audio_src = GkAudioSource::Output;
        } else if (param == ALC_CAPTURE_DEVICE_SPECIFIER) {
            audio.audio_src = GkAudioSource::Input;
        } else {
            std::throw_with_nested(std::runtime_error(tr("Unhandled error with regards to OpenAL audio device enumeration!").toStdString()));
        }

        audio.audio_dev_str = dev;
        device_list.push_back(audio);
    }

    // TODO:
    // https://github.com/kcat/openal-soft/issues/350

    return device_list;
}

/**
 * @brief GkAudioDevices::calcAudioDevFormat will calculate an audio format that's applicable to the given audio device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_channels The number of audio channels (i.e. typically either Mono or Stereo).
 * @param audio_bitrate_idx See, `ui->comboBox_input_audio_dev_bitrate()`, under, `Ui::DialogSettings()`.
 * @return The calculated audio format applicable to the given audio device.
 */
ALenum GkAudioDevices::calcAudioDevFormat(const Settings::GkAudioChannels &audio_channels, const qint32 &audio_bitrate_idx)
{
    if (audio_channels == GkAudioChannels::Mono) {
        switch (audio_bitrate_idx) {
            case GK_AUDIO_BITRATE_8_IDX:
                return AL_FORMAT_MONO8;
            case GK_AUDIO_BITRATE_16_IDX:
                return AL_FORMAT_MONO16;
            case GK_AUDIO_BITRATE_24_IDX:
                return AL_FORMAT_MONO_FLOAT32;
            default:
                std::throw_with_nested(std::runtime_error(tr("ERROR: Unable to accurately determine bit-rate for input audio device!").toStdString()));
        }
    } else if (audio_channels == GkAudioChannels::Stereo) {
        switch (audio_bitrate_idx) {
            case GK_AUDIO_BITRATE_8_IDX:
                return AL_FORMAT_STEREO8;
            case GK_AUDIO_BITRATE_16_IDX:
                return AL_FORMAT_STEREO16;
            case GK_AUDIO_BITRATE_24_IDX:
                return AL_FORMAT_STEREO_FLOAT32;
            default:
                std::throw_with_nested(std::runtime_error(tr("ERROR: Unable to accurately determine bit-rate for input audio device!").toStdString()));
        }
    }

    return 0;
}

/**
 * @brief GkAudioDevices::getAudioDevSampleRate obtains the sampling rate for the (usually output) audio device, with the
 * latter being either chosen by the end-user or appropriated from the system as the default device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device The output audio device to query in question.
 * @return The sampling rate of the given output audio device.
 */
ALCuint GkAudioDevices::getAudioDevSampleRate(ALCdevice *device)
{
    ALCint srate = 0;
    alcGetIntegerv(device, ALC_FREQUENCY, 1, &srate);

    return srate;
}

/**
 * @brief GkAudioDevices::getPeakValue
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_format
 * @param bitrate
 * @return
 * @note thibsc <https://stackoverflow.com/questions/50277132/qt-audio-file-to-wave-like-audacity>
 */
qreal GkAudioDevices::getPeakValue(const ALenum &audio_format, const qint32 &bitrate, const bool &is_signed)
{
    qreal ret(0);
    switch (audio_format) {
        case AL_FORMAT_MONO8:
            if (is_signed) {
                ret = CHAR_MAX;
            } else {
                ret = UCHAR_MAX;
            }

            break;
        case AL_FORMAT_MONO16:
            if (is_signed) {
                ret = SHRT_MAX;
            } else {
                ret = USHRT_MAX;
            }

            break;
        case AL_FORMAT_MONO_FLOAT32:
            if (bitrate != 32) {
                ret = 0;
            } else {
                ret = 1.00003;
            }

            break;
        case AL_FORMAT_STEREO8:
            if (is_signed) {
                ret = CHAR_MAX;
            } else {
                ret = UCHAR_MAX;
            }

            break;
        case AL_FORMAT_STEREO16:
            if (is_signed) {
                ret = SHRT_MAX;
            } else {
                ret = USHRT_MAX;
            }

            break;
        case AL_FORMAT_STEREO_FLOAT32:
            if (bitrate != 32) {
                ret = 0;
            } else {
                ret = 1.00003;
            }

            break;
        default:
            std::throw_with_nested(std::runtime_error(tr("Unable to determine audio format and thusly peak value!").toStdString()));
    }

    return ret;
}

/**
 * @brief GkAudioDevices::captureAlSamples for the capturing/recording of audio samples.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device
 * @param buffer
 * @param samples
 */
void GkAudioDevices::captureAlSamples(ALCdevice *device, ALshort *buffer, ALCsizei samples)
{
    alcCaptureSamples(device, buffer, samples);
    return;
}

/**
 * @brief GkAudioDevices::fwrite16le
 * @author OpenAL soft <https://github.com/kcat/openal-soft/blob/master/examples/alrecord.c>
 * @param val The value to be worked upon.
 * @param f The file handler.
 */
void GkAudioDevices::fwrite16le(ALushort val, FILE *f)
{
    ALubyte data[2];
    data[0] = (ALubyte)(val&0xff);
    data[1] = (ALubyte)(val>>8);
    fwrite(data, 1, 2, f);

    return;
}

/**
 * @brief GkAudioDevices::fwrite32le
 * @author OpenAL soft <https://github.com/kcat/openal-soft/blob/master/examples/alrecord.c>
 * @param val The value to be worked upon.
 * @param f The file handler.
 */
void GkAudioDevices::fwrite32le(ALuint val, FILE *f)
{
    ALubyte data[4];
    data[0] = (ALubyte)(val&0xff);
    data[1] = (ALubyte)((val>>8)&0xff);
    data[2] = (ALubyte)((val>>16)&0xff);
    data[3] = (ALubyte)(val>>24);
    fwrite(data, 1, 4, f);

    return;
}

/**
 * @brief GkAudioDevices::applyGain
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param buffer
 * @param buffer_size
 * @param gain_factor
 */
void GkAudioDevices::applyGain(ALshort *buffer, const quint32 &buffer_size, const qreal &gain_factor)
{
    //
    // NOTE: Clipping with regard to 16-bit boundaries!
    for (quint32 i = 0; i < buffer_size; ++i) {
        buffer[i] = qBound<ALshort>(std::numeric_limits<ALshort>::min(), qRound(buffer[i] * gain_factor), std::numeric_limits<ALshort>::max());
    }

    return;
}

/**
 * @brief GkAudioDevices::convAudioChannelsToEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param num_channels
 * @return
 */
Database::Settings::GkAudioChannels GkAudioDevices::convAudioChannelsToEnum(const qint32 &num_channels)
{
    if (num_channels == 1) {
        return GkAudioChannels::Mono;
    } else if (num_channels == 2) {
        return GkAudioChannels::Stereo;
    } else if (num_channels > 2) {
        return GkAudioChannels::Surround;
    } else {
        return GkAudioChannels::Unknown;
    }

    return GkAudioChannels::Unknown;
}

/**
 * @brief GkAudioDevices::rtAudioVersionNumber returns the actual version of PortAudio
 * itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString GkAudioDevices::rtAudioVersionNumber()
{
    // TODO: Update this so that it mentions the audio backend!
    return QCoreApplication::applicationVersion();
}

/**
 * @brief GkAudioDevices::rtAudioVersionText returns version-specific information about
 * PortAudio itself, as a QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QString GkAudioDevices::rtAudioVersionText()
{
    // TODO: Update this so that it mentions the audio backend!
    return QCoreApplication::applicationVersion();
}
