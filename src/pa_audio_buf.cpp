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

#include "pa_audio_buf.hpp"
#include <iostream>

using namespace GekkoFyre;
using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief PaAudioBuf::PaAudioBuf processes the audio buffering and memory handling functions for PortAudio.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
PaAudioBuf::PaAudioBuf(int size_hint) : std::vector<short>(rec_samples)
{
    if (size_hint > 0)
        rec_samples.reserve(size_hint);

    playback_iter = rec_samples.begin();
}

PaAudioBuf::~PaAudioBuf()
{}

/**
 * @brief PaAudioBuf::recordCallback is responsible for copying PortAudio data (i.e. input audio device) to the
 * memory, asynchronously.
 * @author Keith Vertanen <https://www.keithv.com/software/portaudio/>
 * @param input_buffer
 * @param putput_buffer
 * @param frames_per_buffer
 * @param time_info
 * @param status_flags
 * @param user_data
 * @return
 */
int PaAudioBuf::recordCallback(const void* input_buffer, void* putput_buffer, unsigned long frames_per_buffer,
                          const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags)
{
    short** data_mem = (short**)input_buffer;
    if (input_buffer == nullptr) {
        return paContinue;
    }

    for (unsigned long i = 0; i < frames_per_buffer; i++) {
        rec_samples.push_back(data_mem[0][i]);
    }

    return paContinue;
}

/**
 * @brief PaAudioBuf::writeToFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_name
 */
void PaAudioBuf::writeToFile(const std::string &file_name)
{
    // TODO: Write the code for this section!
    return;
}

/**
 * @brief PaAudioBuf::resetPlayback
 */
void PaAudioBuf::resetPlayback()
{
    playback_iter = rec_samples.begin();
    return;
}
