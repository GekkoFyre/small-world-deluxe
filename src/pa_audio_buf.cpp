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
#include <exception>
#include <mutex>
#include <algorithm>
#include <iterator>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

/**
 * @brief PaAudioBuf::PaAudioBuf processes the audio buffering and memory handling functions for PortAudio.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <http://portaudio.com/docs/v19-doxydocs-dev/pa__ringbuffer_8h.html>
 * <http://portaudio.com/docs/v19-doxydocs-dev/group__test__src.html>
 * @param size_hint
 */
PaAudioBuf::PaAudioBuf(size_t size_hint, const bool &is_rec_active)
{
    std::mutex pa_audio_buf_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_mtx);

    buffer_size = size_hint;
    rec_samples_ptr = new boost::circular_buffer<short>(buffer_size);
}

PaAudioBuf::~PaAudioBuf()
{
    delete rec_samples_ptr;
}

/**
 * @brief PaAudioBuf::playbackCallback
 * @author Keith Vertanen <https://www.keithv.com/software/portaudio/>
 * @param input_buffer
 * @param input_buffer
 * @param frames_per_buffer
 * @param time_info
 * @param status_flags
 * @return
 */
int PaAudioBuf::playbackCallback(const void *input_buffer, void *output_buffer, unsigned long frames_per_buffer,
                                 const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags)
{
    short**	data_mem = (short**)output_buffer;
    unsigned long i_output = 0;
    std::mutex playback_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(playback_loop_mtx);

    if (output_buffer == nullptr) {
        return paComplete;
    }

    // Output samples until we either have satified the caller, or we run out
    auto it = rec_samples_ptr->begin();
    while (i_output < frames_per_buffer) {
        if (it == rec_samples_ptr->end()) {
            // Fill out buffer with zeros
            while (i_output < frames_per_buffer) {
                data_mem[0][i_output] = (short)0;
                i_output++;
            }

            return paComplete;
        }

        data_mem[0][i_output] = (short) *it;
        ++it;
        i_output++;
    }

    return paContinue;
}

/**
 * @brief PaAudioBuf::recordCallback is responsible for copying PortAudio data (i.e. input audio device) to the
 * memory, asynchronously.
 * @author Keith Vertanen <https://www.keithv.com/software/portaudio/>
 * @param input_buffer
 * @param output_buffer
 * @param frames_per_buffer
 * @param time_info
 * @param status_flags
 * @param user_data
 * @return
 */
int PaAudioBuf::recordCallback(const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
                          const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags)
{
    short** data_mem = (short**)input_buffer;
    std::mutex record_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(record_loop_mtx);

    if (input_buffer == nullptr) {
        return paContinue;
    }

    for (unsigned long i = 0; i < frames_per_buffer; i++) {
        rec_samples_ptr->push_back(data_mem[0][i]);
    }

    return paContinue;
}

/**
 * @brief PaAudioBuf::dumpMemory Will dump the contents of the buffer when it reaches the target
 * size, before starting over again.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The entire contents of the buffer once having reached the target size.
 */
std::vector<short> PaAudioBuf::dumpMemory() const
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            std::mutex pa_buf_dup_mem_mtx;
            std::lock_guard<std::mutex> lck_guard(pa_buf_dup_mem_mtx);

            std::vector<short> ret_vec;
            ret_vec.reserve(buffer_size + 1);

            for (auto it = rec_samples_ptr->begin(); it != rec_samples_ptr->end(); ++it) {
                ret_vec.push_back(*it);
            }

            return ret_vec;
        }
    }

    return std::vector<short>();
}

size_t PaAudioBuf::size() const
{
    size_t rec_samples_size = 0;
    if (!rec_samples_ptr->empty()) {
        rec_samples_size = rec_samples_ptr->size();
        return rec_samples_size;
    }

    return rec_samples_size;
}

short PaAudioBuf::at(const short &idx) const
{
    short ret_value = 0;
    std::mutex pa_audio_buf_loc_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_loc_mtx);
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            if (idx <= buffer_size && idx >= 0) {
                size_t counter = 0;
                for (const auto &sample: *rec_samples_ptr) {
                    if (counter == idx) {
                        ret_value = sample;
                        break;
                    }

                    ++counter;
                }

                return ret_value;
            }
        }
    }

    return ret_value;
}

short PaAudioBuf::front() const
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            return rec_samples_ptr->front();
        }
    }

    return 0;
}

short PaAudioBuf::back() const
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            return rec_samples_ptr->back();
        }
    }

    return 0;
}

void PaAudioBuf::push_back(const short &data)
{
    if (rec_samples_ptr != nullptr) {
        rec_samples_ptr->push_back(data);
    }

    return;
}

void PaAudioBuf::push_front(const short &data)
{
    if (rec_samples_ptr != nullptr) {
        rec_samples_ptr->push_front(data);
    }

    return;
}

void PaAudioBuf::pop_front()
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            rec_samples_ptr->pop_front();
        }
    }

    return;
}

void PaAudioBuf::pop_back()
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            rec_samples_ptr->pop_back();
        }
    }

    return;
}

void PaAudioBuf::swap(boost::circular_buffer<short> data_idx_1) noexcept
{
    if (rec_samples_ptr != nullptr) {
        rec_samples_ptr->swap(data_idx_1);
    }

    return;
}

bool PaAudioBuf::empty() const
{
    if (rec_samples_ptr != nullptr) {
        if (rec_samples_ptr->empty()) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

bool PaAudioBuf::clear() const
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            rec_samples_ptr->erase(rec_samples_ptr->begin(), (rec_samples_ptr->begin() + buffer_size));

            if (rec_samples_ptr->empty()) {
                return true;
            }
        }
    }

    return false;
}

std::vector<short> PaAudioBuf::linearize() const
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            std::vector<short> ret_value;
            auto circ_ptr_temp = rec_samples_ptr->linearize();
            std::copy(circ_ptr_temp, (circ_ptr_temp + buffer_size), std::back_inserter(ret_value));
            rec_samples_ptr->erase(rec_samples_ptr->begin(), (rec_samples_ptr->begin() + buffer_size));

            return ret_value;
        }
    }

    return std::vector<short>();
}

boost::circular_buffer<short, std::allocator<short>>::iterator PaAudioBuf::begin() const
{
    if (rec_samples_ptr != nullptr) {
        return rec_samples_ptr->begin();
    }

    return boost::circular_buffer<short, std::allocator<short>>::iterator();
}

boost::circular_buffer<short, std::allocator<short>>::iterator PaAudioBuf::end() const
{
    if (rec_samples_ptr != nullptr) {
        return rec_samples_ptr->end();
    }

    return boost::circular_buffer<short, std::allocator<short>>::iterator();
}

/**
 * @brief PaAudioBuf::dlgBoxOk Creates a modal message box within the Win32 API, with an OK button.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://docs.microsoft.com/en-us/windows/win32/dlgbox/using-dialog-boxes#creating-a-modal-dialog-box>
 * <https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox>
 * @param hwnd
 * @param title
 * @param msgTxt
 * @param icon
 * @see GekkoFyre::StringFuncs::modalDlgBoxOk().
 */
void PaAudioBuf::dlgBoxOk(const HWND &hwnd, const QString &title, const QString &msgTxt, const int &icon)
{
    MessageBox(hwnd, msgTxt.toStdString().c_str(), title.toStdString().c_str(), icon | MB_OK);
    return;
}
