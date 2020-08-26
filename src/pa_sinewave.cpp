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
 **   Small world is distributed in the hope that it will be useful,
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

#include "src/pa_sinewave.hpp"
#include <iostream>
#include <ostream>
#include <mutex>
#include <cmath>
#include <exception>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief PaSinewave::PaSinewave creates a waveform, that of a sinewave, so that you have an easier time
 * enumerating your audio devices whether they be an input or output device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
PaSinewave::PaSinewave(int tableSize) : tableSize(tableSize), leftPhase(0), rightPhase(0)
{
    table = new float[tableSize];
    for (int i = 0; i < tableSize; ++i) {
        table[i] = (0.125f * (float)std::sin(((double)i/(double)tableSize) * M_PI * 2.));
    }
}

PaSinewave::~PaSinewave()
{
    delete[] table;
}

/**
 * @brief PaSinewave::generate
 * @param input_buffer
 * @param output_buffer
 * @param frames_per_buffer
 * @param timing_info
 * @param status_flags
 * @return
 */
int PaSinewave::generate(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer,
                         const PaStreamCallbackTimeInfo *timing_info, PaStreamCallbackFlags status_flags)
{
    Q_UNUSED(input_buffer);
    Q_UNUSED(frames_per_buffer);
    Q_UNUSED(timing_info);
    Q_UNUSED(status_flags);

    try {
        if (output_buffer != nullptr) {
            std::mutex sine_loop_mtx;
            float **out = static_cast<float **>(output_buffer);

            for (unsigned int i = 0; i < AUDIO_FRAMES_PER_BUFFER; ++i) {
                std::lock_guard<std::mutex> lck_guard(sine_loop_mtx);
                out[0][i] = table[leftPhase];
                out[1][i] = table[rightPhase];

                leftPhase += 1;
                if (leftPhase >= tableSize) {
                    leftPhase -= tableSize;
                }

                rightPhase += 3;
                if (rightPhase >= tableSize) {
                    rightPhase -= tableSize;
                }
            }

            return paContinue;
        } else {
            throw std::invalid_argument("Sinewave test input was null!");
        }
    } catch (const portaudio::PaException &e) {
        std::cerr << "A PortAudio error has occurred:\n\n" << e.paErrorText() << std::endl;
    } catch (const portaudio::PaCppException &e) {
        std::cerr << "A PortAudioCpp error has occurred:\n\n" << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "A generic exception has occurred:\n\n" << e.what() << std::endl;
    } catch (...) {
        std::cerr << "An unknown exception has occurred. There are no further details." << std::endl;
    }

    return paAbort;
}
