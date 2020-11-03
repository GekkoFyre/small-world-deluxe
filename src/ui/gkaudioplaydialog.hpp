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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_logger.hpp"
#include "src/file_io.hpp"
#include "src/pa_audio_player.hpp"
#include <AudioFile.h>
#include <boost/filesystem.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <QFile>
#include <QObject>
#include <QDialog>
#include <QString>
#include <QPointer>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>

namespace fs = boost::filesystem;
namespace sys = boost::system;

namespace Ui {
class GkAudioPlayDialog;
}

class GkAudioPlayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkAudioPlayDialog(QPointer<GekkoFyre::GkLevelDb> database,
                               const GekkoFyre::Database::Settings::Audio::GkDevice &input_device,
                               const GekkoFyre::Database::Settings::Audio::GkDevice &output_device,
                               QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput,
                               QPointer<GekkoFyre::StringFuncs> stringFuncs,
                               QPointer<GekkoFyre::GkEventLogger> eventLogger,
                               QWidget *parent = nullptr);
    ~GkAudioPlayDialog() override;

    GekkoFyre::Database::Settings::GkAudioChannels determineAudioChannels();

private slots:
    void on_pushButton_reset_clicked();
    void on_pushButton_close_clicked();
    void on_pushButton_playback_stop_clicked();
    void on_pushButton_playback_browse_file_loc_clicked();
    void on_pushButton_playback_play_clicked();
    void on_pushButton_playback_record_clicked();
    void on_pushButton_playback_skip_back_clicked();
    void on_pushButton_playback_skip_forward_clicked();

    void on_comboBox_playback_rec_codec_currentIndexChanged(int index);
    void on_comboBox_playback_rec_bitrate_currentIndexChanged(int index);

    void resetStopButtonColor();

signals:
    void beginRecording(const bool &recording_is_started);

private:
    Ui::GkAudioPlayDialog *ui;

    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::GkPaAudioPlayer> gkPaAudioPlayer;

    //
    // QPushButtons, etc.
    //
    bool audio_out_play;
    bool audio_out_stop;
    bool audio_out_record;
    bool audio_out_skip_fwd;
    bool audio_out_skip_bck;

    //
    // QAudioSystem initialization and buffers
    //
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QAudioOutput> gkAudioOutput;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_input_device;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_output_device;

    //
    // AudioFile objects and related
    //
    std::shared_ptr<AudioFile<double>> gkAudioFile;

    QFile r_pback_audio_file;
    fs::path audio_file_path;
    GekkoFyre::GkAudioFramework::AudioFileInfo gkAudioFileInfo;

    template <typename T>
    struct gkConvertDoubleToFloat {
        template <typename U>
        T operator () (const U &x) const { return static_cast<T> (x); }
    };

};

