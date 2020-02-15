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
PaAudioBuf::PaAudioBuf(int size_hint) : std::vector<short>(rec_samples)
{
    std::mutex pa_audio_buf_mtx;
    std::lock_guard<std::mutex> lck_guard(pa_audio_buf_mtx);
    if (size_hint > 0) {
        rec_samples.reserve(size_hint);
    }

    playback_iter = rec_samples.begin();
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
    short**	data_mem = (short**)output_buffer;
    unsigned long i_output = 0;
    std::mutex playback_loop_mtx;

    std::lock_guard<std::mutex> lck_guard(playback_loop_mtx);
    if (output_buffer == nullptr) {
        return paComplete;
    }

    // Output samples until we either have satified the caller, or we run out
    while (i_output < frames_per_buffer) {
        if (playback_iter == rec_samples.end()) {
            // Fill out buffer with zeros
            while (i_output < frames_per_buffer) {
                data_mem[0][i_output] = (short)0;
                i_output++;
            }

            return paComplete;
        }

        data_mem[0][i_output] = (short) *playback_iter;
        playback_iter++;
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
        rec_samples.push_back(data_mem[0][i]);
    }

    return paContinue;
}

/**
 * @brief PaAudioBuf::writeToFile
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_name
 */
short PaAudioBuf::writeToMemory(const int &idx)
{
    try {
        std::mutex mem_buffer_mtx;
        short idx_rec_sample;
        std::lock_guard<std::mutex> lck_guard(mem_buffer_mtx);

        for (size_t i = 0; i < rec_samples.size(); ++i) {
            if (i == idx) {
                idx_rec_sample = rec_samples.at(i);

                return idx_rec_sample;
            }

            continue;
        }

        return -1;
    } catch (const std::exception &e) {
        HWND hwnd_write_memory;
        dlgBoxOk(hwnd_write_memory, "Error!", e.what(), MB_ICONERROR);
        DestroyWindow(hwnd_write_memory);
    }

    return -1;
}

/**
 * @brief PaAudioBuf::resetPlayback
 */
void PaAudioBuf::resetPlayback()
{
    std::mutex reset_playback_mtx;
    std::lock_guard<std::mutex> lck_guard(reset_playback_mtx);
    playback_iter = rec_samples.begin();
    return;
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
