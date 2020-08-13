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

#include "gkaudioplaydialog.hpp"
#include "ui_gkaudioplaydialog.h"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <exception>
#include <QSettings>
#include <QMessageBox>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

namespace fs = boost::filesystem;
namespace sys = boost::system;

GkAudioPlayDialog::GkAudioPlayDialog(QPointer<GkLevelDb> database,
                                     QPointer<GkAudioDecoding> audio_decoding,
                                     std::shared_ptr<AudioDevices> audio_devices,
                                     QPointer<FileIo> file_io,
                                     QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GkAudioPlayDialog)
{
    ui->setupUi(this);

    gkDb = std::move(database);
    gkAudioDecode = std::move(audio_decoding);
    gkAudioDevs = std::move(audio_devices);
    gkFileIo = std::move(file_io);
}

GkAudioPlayDialog::~GkAudioPlayDialog()
{
    delete ui;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_reset_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::on_pushButton_reset_clicked()
{
    //
    // Return everything back to its previous state, or as close to it as possible!
    //

    return;
}

void GkAudioPlayDialog::on_pushButton_close_clicked()
{
    this->close();

    return;
}

void GkAudioPlayDialog::on_pushButton_playback_stop_clicked()
{
    return;
}

void GkAudioPlayDialog::on_pushButton_playback_browse_file_loc_clicked()
{
    return;
}

void GkAudioPlayDialog::on_pushButton_playback_record_toggled(bool checked)
{
    // emit beginRecording(checked);
    ui->pushButton_playback_record->setChecked(checked);

    return;
}

void GkAudioPlayDialog::on_pushButton_playback_play_toggled(bool checked)
{
    ui->pushButton_playback_play->setChecked(checked);

    return;
}

void GkAudioPlayDialog::on_pushButton_playback_skip_forward_toggled(bool checked)
{
    Q_UNUSED(checked);
    return;
}

void GkAudioPlayDialog::on_pushButton_playback_skip_back_toggled(bool checked)
{
    Q_UNUSED(checked);
    return;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_playback_play_clicked disable any recording in progress
 * upon being clicked.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::on_pushButton_playback_play_clicked()
{
    return;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_playback_record_clicked disable any playback in progress
 * upon being clicked.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::on_pushButton_playback_record_clicked()
{
    return;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_playback_stop_toggled stop all playback and recording
 * at this moment.
 * @param checked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::on_pushButton_playback_stop_toggled(bool checked)
{
    ui->pushButton_playback_stop->setChecked(checked);

    return;
}
