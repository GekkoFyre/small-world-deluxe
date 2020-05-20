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
PaAudioBuf::PaAudioBuf(int size_hint, QObject *parent)
{
    std::mutex pa_audio_buf_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_mtx);

    buffer_size = size_hint;
    rec_samples_ptr = std::make_unique<boost::circular_buffer<int>>(buffer_size);
}

PaAudioBuf::~PaAudioBuf()
{}

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
    std::mutex playback_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(playback_loop_mtx);
    int**	data_mem = (int**)output_buffer;
    unsigned long i_output = 0;

    if (output_buffer == nullptr) {
        return paComplete;
    }

    // Output samples until we either have satified the caller, or we run out
    auto it = rec_samples_ptr->begin();
    while (i_output < frames_per_buffer) {
        if (it == rec_samples_ptr->end()) {
            // Fill out buffer with zeros
            while (i_output < frames_per_buffer) {
                data_mem[0][i_output] = (int)0;
                i_output++;
            }

            return paComplete;
        }

        data_mem[0][i_output] = (int) *it;
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
    std::mutex record_loop_mtx;
    std::lock_guard<std::mutex> lck_guard(record_loop_mtx);
    int** data_mem = (int**)input_buffer;

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
std::vector<int> PaAudioBuf::dumpMemory()
{
    std::mutex pa_buf_dup_mem_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_buf_dup_mem_mtx);
    std::vector<int> ret_vec;
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            // rec_samples_ptr->linearize();

            for (auto it = rec_samples_ptr->begin(); it != rec_samples_ptr->end(); ++it) {
                ret_vec.push_back(*it);
            }

            return ret_vec;
        }
    }

    ret_vec = fillVecZeros(buffer_size);
    return ret_vec;
}

/**
 * @brief PaAudioBuf::prepOggVorbisBuf prepares an audio frame within the constraints as
 * those set forth by the Ogg Vorbis library for an audio buffer, for which this function
 * exports the data as.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return A data frame suitable to be used right away by the Ogg Vorbis library for recording, with
 * only small modifications needed to fit within the constraints of its data I/O buffer afterword.
 */
void PaAudioBuf::prepOggVorbisBuf(std::promise<std::vector<signed char>> vorbis_buf)
{
    std::mutex pa_prep_vorbis_buf_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_prep_vorbis_buf_mtx);

    auto pa_buf_data = dumpMemory();
    if (!pa_buf_data.empty()) {
        std::vector<signed char> vorbis_buf_tmp;

        vorbis_buf_tmp.reserve(pa_buf_data.size());
        vorbis_buf_tmp.assign(pa_buf_data.begin(), pa_buf_data.end());

        vorbis_buf.set_value(vorbis_buf_tmp);

        return;
    } else {
        throw std::runtime_error(tr("Audio data buffer frame is empty!").toStdString());
    }

    return;
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

int PaAudioBuf::at(const int &idx)
{
    std::mutex pa_audio_buf_loc_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_loc_mtx);
    int ret_value = 0;
    if (rec_samples_ptr != nullptr && is_rec_active) {
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

int PaAudioBuf::front() const
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            return rec_samples_ptr->front();
        }
    }

    return 0;
}

int PaAudioBuf::back() const
{
    if (rec_samples_ptr != nullptr) {
        if (!rec_samples_ptr->empty()) {
            return rec_samples_ptr->back();
        }
    }

    return 0;
}

void PaAudioBuf::push_back(const int &data)
{
    if (rec_samples_ptr != nullptr) {
        rec_samples_ptr->push_back(data);
    }

    return;
}

void PaAudioBuf::push_front(const int &data)
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

void PaAudioBuf::swap(boost::circular_buffer<int> data_idx_1) noexcept
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

boost::circular_buffer<int, std::allocator<int>>::iterator PaAudioBuf::begin() const
{
    if (rec_samples_ptr != nullptr) {
        return rec_samples_ptr->begin();
    }

    return boost::circular_buffer<int, std::allocator<int>>::iterator();
}

boost::circular_buffer<int, std::allocator<int>>::iterator PaAudioBuf::end() const
{
    if (rec_samples_ptr != nullptr) {
        return rec_samples_ptr->end();
    }

    return boost::circular_buffer<int, std::allocator<int>>::iterator();
}

void PaAudioBuf::abortRecording(const bool &recording_is_stopped, const int &wait_time)
{
    std::mutex pa_audio_buf_rec_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_rec_mtx);

    if (recording_is_stopped) {
        is_rec_active = false;
    } else {
        is_rec_active = true;
    }

    return;
}

/**
 * @brief PaAudioBuf::fillVecZeros The idea of this is to fill an std::vector() with all zeroes
 * when there is nothing else to return but this, so the program doesn't output an exception
 * otherwise.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param buf_size How large to make the std::vector().
 * @return The outputted std::vector() that's now filled with zeroes.
 */
std::vector<int> PaAudioBuf::fillVecZeros(const int &buf_size)
{
    std::mutex fill_vec_zeroes_mtx;
    std::lock_guard<std::mutex> lck_guard(fill_vec_zeroes_mtx);
    std::vector<int> ret_vec;
    ret_vec.reserve(buf_size);

    for (size_t i = 0; i < buf_size; ++i) {
        ret_vec.push_back(0);
    }

    return ret_vec;
}
