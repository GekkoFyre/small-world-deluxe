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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include "src/gk_circ_buffer.hpp"
#include <QObject>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <mutex>
#include <list>

namespace GekkoFyre {

class PaAudioBuf : public QObject {
    Q_OBJECT

public:
    explicit PaAudioBuf(int buffer_size, const GekkoFyre::Database::Settings::Audio::GkDevice &pref_output_device,
                        const GekkoFyre::Database::Settings::Audio::GkDevice &pref_input_device, QObject *parent = nullptr);
    virtual ~PaAudioBuf();

    PaAudioBuf operator*(const PaAudioBuf &) const;
    PaAudioBuf operator+(const PaAudioBuf &) const;

    bool is_rec_active;

    int playbackCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags);
    int recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags);
    float *dumpMemory();
    std::unique_ptr<GekkoFyre::GkCircBuffer<float *>> gkCircBuffer;

    void prepOggVorbisBuf(std::promise<std::vector<signed char>> vorbis_buf);

    virtual size_t size() const;
    virtual float at(const int &idx);
    virtual void push_back(const float &data);
    virtual bool empty() const;
    virtual bool clear() const;

public slots:
    void abortRecording(const bool &recording_is_stopped, const int &wait_time = 5000);
    void updateVuAndBuffer(const float &value);

private:
    GekkoFyre::Database::Settings::Audio::GkDevice prefInputDevice;
    GekkoFyre::Database::Settings::Audio::GkDevice prefOutputDevice;

    int circ_buffer_size;
    int maxFrameIndex;
    int frameIndex;
    float calcVolIdx;

    std::vector<float> fillVecZeros(const int &buf_size);

    portaudio::SampleDataFormat sampleFormatConvert(const unsigned long sample_rate);

};
};
