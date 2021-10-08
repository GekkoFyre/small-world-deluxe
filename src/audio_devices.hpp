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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/file_io.hpp"
#include "src/gk_frequency_list.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_system.hpp"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <mutex>
#include <vector>
#include <string>
#include <memory>
#include <future>
#include <thread>
#include <utility>
#include <QList>
#include <QObject>
#include <QString>
#include <QVector>
#include <QPointer>
#include <QAudioFormat>
#include <QAudioDeviceInfo>

#define alcCall(function, device, ...) AudioDevices::alcCallImpl(__FILE__, __LINE__, function, device, __VA_ARGS__)

namespace GekkoFyre {

class AudioDevices : public QObject {
    Q_OBJECT

public:
    explicit AudioDevices(QPointer<GekkoFyre::GkLevelDb> gkDb, QPointer<GekkoFyre::FileIo> filePtr,
                          QPointer<GekkoFyre::GkFrequencies> freqList, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                          QPointer<GekkoFyre::GkEventLogger> eventLogger, QPointer<GekkoFyre::GkSystem> systemPtr,
                          QObject *parent = nullptr);
    ~AudioDevices() override;

    std::vector<std::pair<QAudioDeviceInfo, Database::Settings::Audio::GkDevice>> enumAudioDevicesCpp(const QList<QAudioDeviceInfo> &audioDeviceInfo);

    void systemVolumeSetting();
    float vuMeter(const int &channels, const int &count, float *buffer);
    float vuMeterPeakAmplitude(const size_t &count, float *buffer);
    float vuMeterRMS(const size_t &count, float *buffer);

    float calcAudioBufferTimeNeeded(const Database::Settings::GkAudioChannels &num_channels, const size_t &fft_num_lines,
                                    const size_t &fft_samples_per_line, const size_t &audio_buf_sampling_length,
                                    const size_t &buf_size);

    static bool checkAlErrors(const std::string &filename, const std::uint_fast32_t line, ALCdevice *device);
    QList<GekkoFyre::Database::Settings::Audio::GkDevice> enumerateAudioDevices(ALCdevice *device, const ALCchar *devices, const bool &is_output_dev = false);

    QString rtAudioVersionNumber();
    QString rtAudioVersionText();

    /**
     * @author IndieGameDev.net <https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/>
     * @return
     */
    template<typename alcFunction, typename... Params>
    static auto alcCallImpl(const char *filename, const std::uint_fast32_t line, alcFunction function, ALCdevice *device,
                     Params... params)->typename std::enable_if_t<std::is_same_v<void, decltype(function(params...))>, bool> {
        function(std::forward<Params>(params)...);
        return checkAlErrors(filename, line, device);
    }

    /**
     * @author IndieGameDev.net <https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/>
     * @return
     */
    template<typename alcFunction, typename ReturnType, typename... Params>
    static auto alcCallImpl(const char *filename, const std::uint_fast32_t line, alcFunction function, ReturnType& returnValue,
                     ALCdevice *device, Params... params)->typename std::enable_if_t<!std::is_same_v<void, decltype(function(params...))>, bool>{
        returnValue = function(std::forward<Params>(params)...);
        return checkAlErrors(filename, line, device);
    }

private:
    QPointer<GkLevelDb> gkDekodeDb;
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::GkFrequencies> gkFreqList;
    QPointer<StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::GkSystem> gkSystem;

};
};
