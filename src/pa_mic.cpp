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

#include "pa_mic.hpp"
#include <portaudiocpp/PortAudioCpp.hxx>
#include <iostream>

using namespace GekkoFyre;
using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief PaMic::PaMic handles most microphone functions via PortAudio.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
PaMic::PaMic(std::shared_ptr<AudioDevices> gkAudio, std::shared_ptr<GkLevelDb> dbPtr, QObject *parent)
    : QObject(parent)
{
    gkAudioDevices = gkAudio;
    gkDb = dbPtr;
}

PaMic::~PaMic()
{}

/**
 * @brief PaMic::recordMic Record from a selected/given microphone device on the user's computing device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * Keith Vertanen <https://www.keithv.com/software/portaudio/>
 * @param device A specifically chosen PortAudio input device.
 * @param stream A PortAudio input device stream.
 * @param buffer_sec_record The read buffer, as measured in seconds.
 * @note <https://github.com/EddieRingle/portaudio/blob/master/examples/paex_record.c>
 * <https://github.com/EddieRingle/portaudio/blob/master/test/patest_read_record.c>
 */
bool PaMic::recordInputDevice(const GkDevice &device, PaStream *stream, std::vector<short> *rec_data,
                              const int &buffer_sec_record)
{
    try {
        // Create an object that is used for recording data (i.e. buffering)
        const size_t audio_buffer_size = ((device.def_sample_rate * AUDIO_BUFFER_STREAMING_SECS) *
                                          gkDb->convertAudioChannelsInt(device.sel_channels));
        std::unique_ptr<PaAudioBuf> audioBuf = std::make_unique<PaAudioBuf>(audio_buffer_size, this); // TODO: Change these values to something more meaningful!

        std::cout << tr("Setting up PortAudio for recording from input audio device...").toStdString() << std::endl;

        portaudio::AutoSystem autoSys;
        portaudio::System &sys = portaudio::System::instance();

        std::cout << tr("Opening a recording stream on: %1").arg(device.device_info.name).toStdString() << std::endl;
        portaudio::DirectionSpecificStreamParameters inParamsRecord(sys.deviceByIndex(device.stream_parameters.device),
                                                                    device.dev_input_channel_count, gkAudioDevices->sampleFormatConvert(device.def_sample_rate),
                                                                    false, device.device_info.defaultLowInputLatency, nullptr);
        portaudio::StreamParameters paramsRecord(inParamsRecord, portaudio::DirectionSpecificStreamParameters::null(),
                                                 device.def_sample_rate, AUDIO_FRAMES_PER_BUFFER, paClipOff);
        portaudio::MemFunCallbackStream<PaAudioBuf> streamRecord(paramsRecord, *audioBuf, &PaAudioBuf::recordCallback);

        while (Pa_IsStreamActive(stream) == 1) {
            for (int j = 0; j < audioBuf->size(); ++j) {
                rec_data->push_back(audioBuf->at(j));
            }

            rec_data->clear();
        }

        return true;
    } catch (const std::exception &e) {
        throw std::runtime_error(e.what());
    }

    return false;
}
