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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/file_io.hpp"
#include <QObject>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

#ifdef _WIN32
#include "src/string_funcs_windows.hpp"
#elif __linux__
#include "src/string_funcs_linux.hpp"
#endif

namespace GekkoFyre {

class AudioDevices : public QObject {
    Q_OBJECT

public:
    explicit AudioDevices(std::shared_ptr<GekkoFyre::DekodeDb> gkDb, std::shared_ptr<GekkoFyre::FileIo> filePtr, QObject *parent = nullptr);
    ~AudioDevices();

    std::vector<GekkoFyre::Database::Settings::Audio::Device> initPortAudio();
    std::vector<GekkoFyre::Database::Settings::Audio::Device> defaultAudioDevices();
    std::vector<double> enumSupportedStdSampleRates(const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters);
    std::vector<GekkoFyre::Database::Settings::Audio::Device> enumAudioDevices();
    void testSinewave(const GekkoFyre::Database::Settings::Audio::Device &device);
    void spectrogram(Database::Settings::Audio::Spectrum *spectrum, const int &pixels_width_per_sec, const float &total_secs,
                     const size_t &spectro_win_width, const size_t &spectro_win_height, const float &win_secs,
                     const float &step_secs);
    Database::Settings::Audio::Spectrum *createSpectrum(const size_t &specified_length);
    static bool is_good_speclen(size_t n);
    void volumeSetting();
    double vuMeter();

private:
    std::shared_ptr<DekodeDb> gkDekodeDb;
    std::shared_ptr<GekkoFyre::FileIo> gkFileIo;
    // std::unique_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    static std::mutex spectro_mutex;                           // Mutex for the spectrometer side of things
    static std::mutex audio_mutex;                             // Mutex for general audio device work

    static int paTestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
                              void *userData);
    static void streamFinished(void *userData);
    bool filterAudioInputEnum(const PaHostApiTypeId &host_api_type);
    bool filterAudioOutputEnum(const PaHostApiTypeId &host_api_type);
    void portAudioErr(const PaError &err);

    double calcMagnSpectrum(Database::Settings::Audio::Spectrum *spectrum);
    void destroySpectrum(Database::Settings::Audio::Spectrum *spectrum);
    void spectrumColorMap(float value, double spec_floor_db, unsigned char colour[3], bool gray_scale);
    static bool is_2357 (size_t n);
    void interpSpectrum(float *mag, int maglen, const double *spec, int speclen, const double &min_freq,
                               const double &max_freq, size_t samplerate);
    double magindex_to_specindex(int speclen, int maglen, size_t magindex, double min_freq, double max_freq, size_t samplerate, bool log_freq);

};
};