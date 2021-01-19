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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_waterfall_gui.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <kiss_fft.h>
#include <string>
#include <vector>
#include <QTimer>
#include <QString>
#include <QThread>
#include <QObject>
#include <QBuffer>
#include <QPointer>
#include <QEventLoop>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioOutput>

namespace fs = boost::filesystem;
namespace sys = boost::system;

namespace GekkoFyre {
class GkFFTAudio : public QThread {
    Q_OBJECT

public:
    explicit GkFFTAudio(const QPointer<QBuffer> &audioInputBuf, QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput,
                        const GekkoFyre::Database::Settings::Audio::GkDevice &input_audio_device_details,
                        const GekkoFyre::Database::Settings::Audio::GkDevice &output_audio_device_details,
                        QPointer<GekkoFyre::GkSpectroWaterfall> spectroWaterfall, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                        QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkFFTAudio() override;

    void run() Q_DECL_OVERRIDE;
    void processEvent(Spectrograph::GkFftEventType audioEventType);
    void processEvent(Spectrograph::GkFftEventType audioEventType, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec,
                      const fs::path &mediaFilePath);

private slots:
    void recordAudioStream();
    void stopRecordStream();

    void recordAudioFileStream(const fs::path &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec);
    void stopRecordFileStream(const fs::path &media_path);

    void refreshGraphTrue();

public slots:
    void processAudioInFft();
    void setAudioIo(const bool &use_input_audio); // True by default!

signals:
    void recordStream();
    void stopRecording();

    void recordFileStream(const fs::path &media_path, const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec);
    void stopRecordingFileStream(const fs::path &media_path);

    void refreshGraph(bool forceRepaint = false);

private:
    QPointer<GekkoFyre::GkSpectroWaterfall> gkSpectroWaterfall;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // QAudioSystem initialization and buffers
    //
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;
    QPointer<QBuffer> gkAudioInputBuf;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_input_audio_device;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_output_audio_device;
    bool audioStreamProc = false;  // Whether an audio stream recording into memory is active or not
    bool audioFileStreamProc = false; // Whether an audio stream recording into a file is active or not
    bool enableAudioStreamProc = false; // Should we re-enable audio stream recording into memory?
    bool enableAudioFileStreamProc = false; // Should we re-enable audio stream recording into a file?

    qint32 gkAudioInNumSamples = 0;
    qint32 gkAudioInSampleRate = 0;
    std::vector<double> audioSamples;

    //
    // Spectrograph
    //
    QPointer<QTimer> spectroRefreshTimer;
    std::vector<double> magSpec;

    void samplesUpdated();

};
};
