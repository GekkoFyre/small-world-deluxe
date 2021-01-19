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

#include "src/ui/gkaudioplaydialog.hpp"
#include "ui_gkaudioplaydialog.h"
#include <boost/exception/all.hpp>
#include <exception>
#include <utility>
#include <QTimer>
#include <QIODevice>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;
using namespace Network;
using namespace GkXmpp;

GkAudioPlayDialog::GkAudioPlayDialog(QPointer<GkLevelDb> database,
                                     const GkDevice &input_device, const GkDevice &output_device,
                                     QPointer<QAudioInput> audioInput, QPointer<QAudioOutput> audioOutput,
                                     QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                     QPointer<GekkoFyre::GkAudioEncoding> audioEncoding,
                                     QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                     QWidget *parent) : QDialog(parent), ui(new Ui::GkAudioPlayDialog)
{
    ui->setupUi(this);

    gkDb = std::move(database);
    gkAudioInput = std::move(audioInput);
    gkAudioOutput = std::move(audioOutput);
    gkStringFuncs = std::move(stringFuncs);
    gkAudioEncoding = std::move(audioEncoding);
    gkEventLogger = std::move(eventLogger);

    //
    // Initialize variables
    //
    pref_input_device = input_device;
    pref_output_device = output_device;
    m_rec_codec_chosen = CodecSupport::PCM;
    m_encode_bitrate_chosen = 8;
    gkAudioFile = std::make_shared<AudioFile<double>>();
    gkPaAudioPlayer = new GkPaAudioPlayer(gkDb, pref_output_device, pref_input_device, gkAudioOutput, gkAudioInput, gkAudioEncoding, gkEventLogger, gkAudioFile, this);

    //
    // QPushButtons, etc.
    //
    audio_out_play = false;
    audio_out_stop = false;
    audio_out_record = false;
    audio_out_skip_fwd = false;
    audio_out_skip_bck = false;

    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::OggVorbis);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::FLAC);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::Opus);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::PCM);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::Loopback);
}

GkAudioPlayDialog::~GkAudioPlayDialog()
{
    if (audio_out_play) {
        gkPaAudioPlayer->stop(audio_file_path);
        gkEventLogger->publishEvent(tr("Stopped playing audio file, \"%1\"").arg(QString::fromStdString(audio_file_path.string())), GkSeverity::Info, "", true, true, true, false);
    }

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
    //
    // TODO: This is in need of updating!
    if (!r_pback_audio_file.fileName().isEmpty()) {
        // We currently have a file selected!
        if (gkAudioFile->getNumChannels() == 1) {
            //
            // Mono
            //
            return Database::Settings::GkAudioChannels::Mono;
        } else if (gkAudioFile->getNumChannels() == 2) {
            //
            // Stereo
            //
            return GkAudioChannels::Both;
        } else {
            if (gkAudioFile->getNumChannels() > 2) {
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
        gkPaAudioPlayer->stop(audio_file_path);
        gkEventLogger->publishEvent(tr("Stopped playing audio file, \"%1\"").arg(QString::fromStdString(audio_file_path.string())), GkSeverity::Info, "", true, true, true, false);

        audio_out_stop = true;
        QTimer::singleShot(1000, this, SLOT(resetStopButtonColor()));
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
                    gkAudioFileInfo.file_size_hr = gkStringFuncs->fileSizeHumanReadable(gkAudioFileInfo.file_size);
                    r_pback_audio_file.close();
                    
                    gkAudioFileInfo.is_output = true;
                    gkAudioFileInfo.audio_file_path = file.toStdString();
                    bool loadedOk = gkAudioFile->load(gkAudioFileInfo.audio_file_path.string());
                    if (!loadedOk) {
                        gkEventLogger->publishEvent(tr("An error has been encountered whilst attempting to initialize the multimedia interface regarding file, \"%1\".")
                        .arg(QString::fromStdString(gkAudioFileInfo.audio_file_path.filename().string())), GkSeverity::Warning, "", true,
                        true, false, false);
                    }

                    gkAudioFileInfo.sample_rate = gkAudioFile->getSampleRate();
                    gkAudioFileInfo.bit_depth = gkAudioFile->getBitDepth();
                    gkAudioFileInfo.length_in_secs = gkAudioFile->getLengthInSeconds();
                    gkAudioFileInfo.num_samples_per_channel = gkAudioFile->getNumSamplesPerChannel();

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
                ui->lineEdit_playback_file_size->setText(gkAudioFileInfo.file_size_hr);
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

            GkAudioFramework::CodecSupport codec_used = gkDb->convCodecSupportFromIdxToEnum(ui->comboBox_playback_rec_codec->currentData().toInt());
            if (r_pback_audio_file.exists() && codec_used != GkAudioFramework::CodecSupport::Loopback) {
                gkPaAudioPlayer->play(codec_used, audio_file_path);
                gkEventLogger->publishEvent(
                        tr("Started playing audio file, \"%1\"").arg(QString::fromStdString(audio_file_path.string())),
                        GkSeverity::Info, "", true, true, true, false);
                ui->progressBar_playback->setFormat(tr("%p%")); // Modify the QProgressBar to display the correct text!
            } else if (codec_used == GkAudioFramework::CodecSupport::Loopback) {
                gkPaAudioPlayer->play(codec_used);
                gkEventLogger->publishEvent(
                        tr("Started audio device loopback!"), GkSeverity::Info, "", true, true, true, false);
                ui->progressBar_playback->setFormat(tr("%p%")); // Modify the QProgressBar to display the correct text!
            } else {
                throw std::runtime_error(tr("Error with audio playback! Does the file, \"%1\", actually exist?")
                .arg(r_pback_audio_file.fileName()).toStdString());
            }
        } else {
            gkPaAudioPlayer->stop(audio_file_path);
            gkEventLogger->publishEvent(tr("Stopped playing audio file, \"%1\"").arg(QString::fromStdString(audio_file_path.string())), GkSeverity::Info, "", true, true, true, false);

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
    try {
        if (!audio_out_record) {
            //
            // Do start recording!
            gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, false);
            audio_out_record = true;

            QFileDialog fileDialog(this, tr("Save Directory for Recordings"), QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
            fileDialog.setFileMode(QFileDialog::Directory);
            fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
            fileDialog.setViewMode(QFileDialog::List);

            auto remembered_path = gkDb->read_audio_playback_dlg_settings(AudioPlaybackDlg::GkRecordDlgLastFolderBrowsed);
            if (!remembered_path.isEmpty()) {
                // There has been a previously used path that the user has used, and it's been remembered by Google LevelDB!
                fileDialog.setDirectory(remembered_path);
            }

            fs::path record_dir_path;
            if (fileDialog.exec()) {
                QStringList selectedFile = fileDialog.selectedFiles();
                if (!selectedFile.isEmpty())  {
                    for (const auto &file: selectedFile) {
                        record_dir_path = file.toStdString();
                    }
                }
            } else {
                //
                // End recording state...
                gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, true);
                audio_out_record = false;
            }

            GkAudioFramework::CodecSupport codec_used = gkDb->convCodecSupportFromIdxToEnum(ui->comboBox_playback_rec_codec->currentData().toInt());
            if (fs::is_directory(record_dir_path)) {
                gkDb->write_audio_playback_dlg_settings(QString::fromStdString(record_dir_path.string()), AudioPlaybackDlg::GkRecordDlgLastFolderBrowsed);
                if (codec_used != GkAudioFramework::CodecSupport::Loopback) {
                    gkPaAudioPlayer->record(codec_used, record_dir_path);
                } else {
                    QMessageBox::information(this, tr("Not supported!"), tr("Loopback mode is unsupported during recording!"), QMessageBox::Ok);
                }
            }
        } else {
            //
            // End or pause recording...
            gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, true);
            audio_out_record = false;

            //
            // TODO: Implement the ability to STOP recording!
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
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
    switch (index) {
        case AUDIO_PLAYBACK_CODEC_PCM_IDX:
            m_rec_codec_chosen = CodecSupport::PCM;
        case AUDIO_PLAYBACK_CODEC_LOOPBACK_IDX:
            m_rec_codec_chosen = CodecSupport::Loopback;
        case AUDIO_PLAYBACK_CODEC_VORBIS_IDX:
            m_rec_codec_chosen = CodecSupport::OggVorbis;
        case AUDIO_PLAYBACK_CODEC_OPUS_IDX:
            m_rec_codec_chosen = CodecSupport::Opus;
        case AUDIO_PLAYBACK_CODEC_FLAC_IDX:
            m_rec_codec_chosen = CodecSupport::FLAC;
        default:
            break;
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::on_horizontalSlider_playback_rec_bitrate_valueChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 */
void GkAudioPlayDialog::on_horizontalSlider_playback_rec_bitrate_valueChanged(int value)
{
    const qint32 bitrate_val = value;
    ui->label_playback_rec_bitrate_kbps->setText(QString("[ %1 kBps ]").arg(QString::number(bitrate_val)));

    if (bitrate_val % 8 == 0) {
        m_encode_bitrate_chosen = bitrate_val;
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::resetStopButtonColor
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::resetStopButtonColor()
{
    gkStringFuncs->changePushButtonColor(ui->pushButton_playback_stop, true);
    audio_out_stop = false;

    gkStringFuncs->changePushButtonColor(ui->pushButton_playback_play, true);
    audio_out_play = false;

    return;
}

/**
 * @brief GkAudioPlayDialog::prefillCodecComboBoxes
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param supported_codec
 */
void GkAudioPlayDialog::prefillCodecComboBoxes(const CodecSupport &supported_codec)
{
    switch (supported_codec) {
        case CodecSupport::PCM:
            ui->comboBox_playback_rec_codec->insertItem(AUDIO_PLAYBACK_CODEC_PCM_IDX, tr("PCM"), AUDIO_PLAYBACK_CODEC_PCM_IDX);
            break;
        case CodecSupport::Loopback:
            ui->comboBox_playback_rec_codec->insertItem(AUDIO_PLAYBACK_CODEC_LOOPBACK_IDX, tr("Input/Output Loopback"), AUDIO_PLAYBACK_CODEC_LOOPBACK_IDX);
            break;
        case CodecSupport::Opus:
            ui->comboBox_playback_rec_codec->insertItem(AUDIO_PLAYBACK_CODEC_OPUS_IDX, tr("Ogg Opus"), AUDIO_PLAYBACK_CODEC_OPUS_IDX);
            break;
        case CodecSupport::OggVorbis:
            ui->comboBox_playback_rec_codec->insertItem(AUDIO_PLAYBACK_CODEC_VORBIS_IDX, tr("Ogg Vorbis"), AUDIO_PLAYBACK_CODEC_VORBIS_IDX);
            break;
        case CodecSupport::FLAC:
            ui->comboBox_playback_rec_codec->insertItem(AUDIO_PLAYBACK_CODEC_FLAC_IDX, tr("FLAC"), AUDIO_PLAYBACK_CODEC_FLAC_IDX);
            break;
        case CodecSupport::Unsupported:
            break;
        case CodecSupport::Unknown:
            break;
    }

    return;
}
