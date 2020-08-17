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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/gk_audio_decoding.hpp"
#include "src/audio_devices.hpp"
#include "src/file_io.hpp"
#include <memory>
#include <string>
#include <vector>
#include <QObject>
#include <QDialog>
#include <QString>
#include <QPointer>

namespace Ui {
class GkAudioPlayDialog;
}

class GkAudioPlayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkAudioPlayDialog(QPointer<GekkoFyre::GkLevelDb> database,
                               QPointer<GekkoFyre::GkAudioDecoding> audio_decoding,
                               std::shared_ptr<GekkoFyre::AudioDevices> audio_devices,
                               QPointer<GekkoFyre::FileIo> file_io,
                               QWidget *parent = nullptr);
    ~GkAudioPlayDialog() override;

private slots:
    void on_pushButton_reset_clicked();
    void on_pushButton_close_clicked();
    void on_pushButton_playback_stop_clicked();
    void on_pushButton_playback_browse_file_loc_clicked();
    void on_pushButton_playback_play_clicked();
    void on_pushButton_playback_record_clicked();

    void on_pushButton_playback_record_toggled(bool checked);
    void on_pushButton_playback_play_toggled(bool checked);
    void on_pushButton_playback_skip_forward_toggled(bool checked);
    void on_pushButton_playback_skip_back_toggled(bool checked);
    void on_pushButton_playback_stop_toggled(bool checked);

private:
    Ui::GkAudioPlayDialog *ui;

    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkAudioDecoding> gkAudioDecode;
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDevs;
    QPointer<GekkoFyre::FileIo> gkFileIo;

signals:
    void beginRecording(const bool &recording_is_started);

};

