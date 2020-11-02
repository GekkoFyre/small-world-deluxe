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

#include "src/ui/gkaudioplaydialog.hpp"
#include "ui_gkaudioplaydialog.h"
#include "src/pa_audio_player.hpp"
#include <boost/exception/all.hpp>
#include <exception>
#include <utility>
#include <QIODevice>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

GkAudioPlayDialog::GkAudioPlayDialog(QPointer<GkLevelDb> database,
                                     const GkDevice &input_device, const GkDevice &output_device,
                                     QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput,
                                     QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                     QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                     QWidget *parent) : QDialog(parent), ui(new Ui::GkAudioPlayDialog)
{
    ui->setupUi(this);

    gkDb = std::move(database);
    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    //
    // Initialize variables
    //
    sndFileCallback = {};
    pref_input_device = input_device;
    pref_output_device = output_device;

    //
    // QPushButtons, etc.
    //
    audio_out_play = false;
    audio_out_stop = false;
    audio_out_record = false;
    audio_out_skip_fwd = false;
    audio_out_skip_bck = false;
}

GkAudioPlayDialog::~GkAudioPlayDialog()
{
    delete ui;
}

/**
 * @brief GkAudioPlayDialog::determineAudioChannels works out the number of audio channels to use when initializing the QAudioSystem
 * data buffer for audio playback and possibly recording as well.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_file_info The information we have on the audio playback file.
 * @return The number or type of audio channel(s) we should be initializing the QAudioSystem buffer with.
 */
GkAudioChannels GkAudioPlayDialog::determineAudioChannels()
{
    if (!r_pback_audio_file.fileName().isEmpty()) {
        // We currently have a file selected!
        if (sndFileCallback.info.channels == 1) {
            //
            // Mono
            //
            return Database::Settings::GkAudioChannels::Mono;
        } else if (sndFileCallback.info.channels == 2) {
            //
            // Stereo
            //
            return GkAudioChannels::Both;
        } else {
            if (sndFileCallback.info.channels > 2) {
                gkAudioFileInfo.num_audio_channels = Database::Settings::GkAudioChannels::Surround;
            } else {
                gkAudioFileInfo.num_audio_channels = Database::Settings::GkAudioChannels::Unknown;
                gkEventLogger->publishEvent(tr("Unable to accurately determine the number of audio channels within multimedia file, \"%1\", for unknown reasons.")
                                                    .arg(QString::fromStdString(gkAudioFileInfo.audio_file_path.filename().string())), GkSeverity::Warning, "", true,
                                            true, false, false);

                return GkAudioChannels::Unknown;
            }
        }
    }

    return GkAudioChannels::Unknown;
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

    ui->lineEdit_playback_file_location->clear();
    ui->lineEdit_playback_file_size->clear();
    ui->lineEdit_playback_file_name->clear();
    ui->lineEdit_playback_audio_codec->clear();
    ui->lineEdit_playback_bitrate->clear();
    ui->lineEdit_playback_sample_rate->clear();
    ui->progressBar_playback->setValue(ui->progressBar_playback->minimum());

    if (r_pback_audio_file.isOpen()) {
        r_pback_audio_file.close();
    }

    r_pback_audio_file.reset();
    gkAudioFileInfo = {};

    return;
}

void GkAudioPlayDialog::on_pushButton_close_clicked()
{
    this->close();

    return;
}

void GkAudioPlayDialog::on_pushButton_playback_stop_clicked()
{
    if (!audio_out_stop) {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_stop, false);
        audio_out_stop = true;
    } else {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_stop, true);
        audio_out_stop = false;
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_playback_browse_file_loc_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @see libsndfile <http://www.mega-nerd.com/libsndfile/api.html>,
 * wavtools by Matthew H. <https://github.com/hosackm/wavplayer>.
 */
void GkAudioPlayDialog::on_pushButton_playback_browse_file_loc_clicked()
{
    try {
        QFileDialog fileDialog(this, tr("Load Image for Playback"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                               tr("Audio Files (*.wav *.ogg *.opus)"));
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
        fileDialog.setViewMode(QFileDialog::Detail);

        auto remembered_path = gkDb->read_audio_playback_dlg_settings(AudioPlaybackDlg::GkAudioDlgLastFolderBrowsed);
        if (!remembered_path.isEmpty()) {
            // There has been a previously used path that the user has used, and it's been remembered by Google LevelDB!
            fileDialog.setDirectory(remembered_path);
        }

        if (fileDialog.exec()) {
            QStringList selectedFile = fileDialog.selectedFiles();
            if (!selectedFile.isEmpty()) {
                for (const auto &file: selectedFile) {
                    audio_file_path = file.toStdString();
                    r_pback_audio_file.setFileName(file);
                    r_pback_audio_file.open(QIODevice::ReadOnly);
                    gkAudioFileInfo.file_size = r_pback_audio_file.size();
                    r_pback_audio_file.close();
                    
                    gkAudioFileInfo.is_output = true;
                    gkAudioFileInfo.audio_file_path = file.toStdString();
                    sndFileCallback.file = sf_open(gkAudioFileInfo.audio_file_path.c_str(), SFM_READ, &sndFileCallback.info);
                    if (sf_error(sndFileCallback.file) != SF_ERR_NO_ERROR) {
                        gkEventLogger->publishEvent(tr("An error has been encountered whilst attempting to initialize the multimedia interface regarding file, \"%1\", with the exact error being:\n\n%2")
                        .arg(QString::fromStdString(gkAudioFileInfo.audio_file_path.filename().string())).arg(QString::fromStdString(sf_strerror(sndFileCallback.file))), GkSeverity::Warning, "", true,
                        true, false, false);
                    }

                    gkAudioFileInfo.sample_rate = sndFileCallback.info.samplerate;
                    gkAudioFileInfo.bit_depth = sndFileCallback.info.format;
                    gkAudioFileInfo.length_in_secs = (sndFileCallback.info.frames / (gkAudioFileInfo.sample_rate));
                    gkAudioFileInfo.num_samples_per_channel = sndFileCallback.info.frames;

                    gkDb->write_audio_playback_dlg_settings(QString::fromStdString(gkAudioFileInfo.audio_file_path.parent_path().string()), AudioPlaybackDlg::GkAudioDlgLastFolderBrowsed);

                    //
                    // Determine whether the audio file is Mono, Stereo, or something else in nature, and add it to the global variables
                    // for this class...
                    //
                    determineAudioChannels();
                }

                QString lengthSecs = tr("0 seconds");
                if (gkAudioFileInfo.length_in_secs > 1.0f) {
                    lengthSecs = gkStringFuncs->convSecondsToMinutes(gkAudioFileInfo.length_in_secs);
                }

                ui->lineEdit_playback_file_location->setText(QString::fromStdString(gkAudioFileInfo.audio_file_path.string()));
                ui->lineEdit_playback_file_size->setText(QString::number(gkAudioFileInfo.file_size));
                ui->lineEdit_playback_file_name->setText(tr("%1 (%2) -- %3")
                                                                 .arg(QString::fromStdString(gkAudioFileInfo.audio_file_path.filename().string()))
                                                                 .arg(lengthSecs)
                                                                 .arg(gkDb->convertAudioChannelsStr(gkAudioFileInfo.num_audio_channels)));
                ui->lineEdit_playback_bitrate->setText(QString::number(gkAudioFileInfo.bit_depth));
                ui->lineEdit_playback_sample_rate->setText(QString::number(gkAudioFileInfo.sample_rate));
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_playback_play_clicked disable any recording in progress
 * upon being clicked.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::on_pushButton_playback_play_clicked()
{
    try {
        if (!audio_out_play) {
            gkStringFuncs->changePushButtonColor(ui->pushButton_playback_play, false);
            audio_out_play = true;

            if (r_pback_audio_file.exists()) {
                std::unique_ptr<GkPaAudioPlayer> gkPaAudioPlayer = std::make_unique<GkPaAudioPlayer>(gkDb, pref_output_device, gkAudioOutput,
                                                                                                     gkEventLogger, this);
                gkPaAudioPlayer->play(audio_file_path);
            } else {
                throw std::runtime_error(tr("Error with audio playback! Does the file, \"%1\", actually exist?")
                .arg(r_pback_audio_file.fileName()).toStdString());
            }
        } else {
            gkStringFuncs->changePushButtonColor(ui->pushButton_playback_play, true);
            audio_out_play = false;
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("Issue with playback of audio file, \"%1\".\n\nError: %2").arg(r_pback_audio_file.fileName()).arg(QString::fromStdString(e.what())),
                                    GkSeverity::Fatal, "", true, true, false, false);
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_playback_record_clicked disable any playback in progress
 * upon being clicked.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::on_pushButton_playback_record_clicked()
{
    if (!audio_out_record) {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, false);
        audio_out_record = true;
    } else {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, true);
        audio_out_record = false;
    }

    return;
}

void GkAudioPlayDialog::on_pushButton_playback_skip_back_clicked()
{
    if (!audio_out_skip_bck) {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_skip_back, false);
        audio_out_skip_bck = true;
    } else {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_skip_back, true);
        audio_out_skip_bck = false;
    }

    return;
}

void GkAudioPlayDialog::on_pushButton_playback_skip_forward_clicked()
{
    if (!audio_out_skip_fwd) {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_skip_forward, false);
        audio_out_skip_fwd = true;
    } else {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_skip_forward, true);
        audio_out_skip_fwd = false;
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::on_comboBox_playback_rec_codec_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkAudioPlayDialog::on_comboBox_playback_rec_codec_currentIndexChanged(int index)
{
    return;
}

/**
 * @brief GkAudioPlayDialog::on_comboBox_playback_rec_bitrate_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void GkAudioPlayDialog::on_comboBox_playback_rec_bitrate_currentIndexChanged(int index)
{
    return;
}
