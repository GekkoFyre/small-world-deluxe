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
#include <exception>
#include <utility>
#include <QTimer>
#include <QIODevice>
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
using namespace Security;

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
    // Create SIGNALS and SLOTS
    QObject::connect(this, SIGNAL(recStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &)),
                     gkAudioEncoding, SLOT(setRecStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &)));

    //
    // Initialize variables
    //
    gkAudioFileInfo = {};
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
    emit recStatus(GkAudioRecordStatus::Defunct);
    if (audio_out_play) {
        gkPaAudioPlayer->stop(gkAudioFileInfo.audio_file_path);
        gkEventLogger->publishEvent(tr("Stopped playing audio file, \"%1\"").arg(gkAudioFileInfo.audio_file_path.fileName()), GkSeverity::Info, "", true, true, true, false);
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
    if (!gkAudioFileInfo.audio_file_path.exists()) {
        // We currently have a file selected!
        if (gkAudioFile->isMono()) {
            //
            // Mono
            //
            gkAudioFileInfo.num_audio_channels = Database::Settings::GkAudioChannels::Mono;
            return Database::Settings::GkAudioChannels::Mono;
        } else if (gkAudioFile->isStereo()) {
            //
            // Stereo
            //
            gkAudioFileInfo.num_audio_channels = Database::Settings::GkAudioChannels::Both;
            return GkAudioChannels::Both;
        } else {
            if (gkAudioFile->getNumChannels() > 2) {
                gkAudioFileInfo.num_audio_channels = Database::Settings::GkAudioChannels::Surround;
                return GkAudioChannels::Surround;
            } else {
                gkAudioFileInfo.num_audio_channels = Database::Settings::GkAudioChannels::Unknown;
                throw std::invalid_argument(tr("Unable to accurately determine the number of audio channels within multimedia file, \"%1\", for unknown reasons.")
                .arg(gkAudioFileInfo.audio_file_path.fileName()).toStdString());
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
        gkPaAudioPlayer->stop(gkAudioFileInfo.audio_file_path);
        gkEventLogger->publishEvent(tr("Stopped playing audio file, \"%1\"").arg(gkAudioFileInfo.audio_file_path.fileName()), GkSeverity::Info, "", true, true, true, false);

        audio_out_stop = true;
        QTimer::singleShot(1000, this, SLOT(resetStopButtonColor()));
    } else {
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_stop, true);
        audio_out_stop = false;
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

            auto def_path = gkDb->read_audio_playback_dlg_settings(AudioPlaybackDlg::GkAudioDlgLastFolderBrowsed); // There has been a previously used path that the user has used, and it's been remembered by Google LevelDB!
            if (def_path.isEmpty()) {
                def_path = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
            }

            //
            // Open a QFileDialog so a user may choose a multimedia to open and then thusly play!
            const QString filePath = QFileDialog::getOpenFileName(this, tr("Load Audio File for Playback"), def_path,
                                                                  tr("Audio Files (*.wav *.ogg *.opus);;All Files (*.*)"));
            gkAudioFileInfo.audio_file_path.setFile(filePath);
            gkAudioFile->load(gkAudioFileInfo.audio_file_path.filePath().toStdString());

            //
            // Write out information about the file in question!
            gkAudioFileInfo.file_size = gkAudioFileInfo.audio_file_path.size();
            gkAudioFileInfo.file_size_hr = gkStringFuncs->fileSizeHumanReadable(gkAudioFileInfo.file_size);
            gkAudioFileInfo.sample_rate = gkAudioFile->getSampleRate();
            gkAudioFileInfo.bit_depth = gkAudioFile->getBitDepth();
            gkAudioFileInfo.length_in_secs = gkAudioFile->getLengthInSeconds();
            gkAudioFileInfo.num_samples_per_channel = gkAudioFile->getNumSamplesPerChannel();

            //
            // Determine whether the audio file is Mono, Stereo, or something else in nature, and add it to the global
            // variables for this class...
            m_audioChannels = determineAudioChannels();

            QString lengthSecs = tr("0 seconds");
            if (gkAudioFileInfo.length_in_secs > 1.0f) {
                lengthSecs = gkStringFuncs->convSecondsToMinutes(gkAudioFileInfo.length_in_secs);
            }

            //
            // Write out aforementioned file information to the UI now!
            ui->lineEdit_playback_file_location->setText(gkAudioFileInfo.audio_file_path.filePath());
            ui->lineEdit_playback_file_size->setText(gkAudioFileInfo.file_size_hr);
            ui->lineEdit_playback_file_name->setText(tr("%1 (%2) -- %3")
            .arg(gkAudioFileInfo.audio_file_path.fileName(), lengthSecs,
                 gkDb->convertAudioChannelsStr(gkAudioFileInfo.num_audio_channels)));
            ui->lineEdit_playback_bitrate->setText(QString::number(gkAudioFileInfo.bit_depth));
            ui->lineEdit_playback_sample_rate->setText(QString::number(gkAudioFileInfo.sample_rate));

            GkAudioFramework::CodecSupport codec_used = gkDb->convCodecSupportFromIdxToEnum(ui->comboBox_playback_rec_codec->currentData().toInt());
            if (gkAudioFileInfo.audio_file_path.exists() && codec_used != GkAudioFramework::CodecSupport::Loopback) {
                gkPaAudioPlayer->play(codec_used, gkAudioFileInfo.audio_file_path.filePath());
                gkEventLogger->publishEvent(
                        tr("Started playing audio file, \"%1\"").arg(gkAudioFileInfo.audio_file_path.filePath()),
                        GkSeverity::Info, "", true, true, true, false);
                ui->progressBar_playback->setFormat(tr("%p%")); // Modify the QProgressBar to display the correct text!
            } else if (codec_used == GkAudioFramework::CodecSupport::Loopback) {
                gkPaAudioPlayer->play(codec_used);
                gkEventLogger->publishEvent(
                        tr("Started audio device loopback!"), GkSeverity::Info, "", true, true, true, false);
                ui->progressBar_playback->setFormat(tr("%p%")); // Modify the QProgressBar to display the correct text!
            } else {
                throw std::runtime_error(tr("Error with audio playback! Does the file, \"%1\", actually exist?")
                .arg(gkAudioFileInfo.audio_file_path.fileName()).toStdString());
            }
        } else {
            gkPaAudioPlayer->stop(gkAudioFileInfo.audio_file_path);
            gkEventLogger->publishEvent(tr("Stopped playing audio file, \"%1\"").arg(gkAudioFileInfo.audio_file_path.fileName()), GkSeverity::Info, "", true, true, true, false);

            gkStringFuncs->changePushButtonColor(ui->pushButton_playback_play, true);
            audio_out_play = false;
        }
    } catch (const std::exception &e) {
        gkStringFuncs->print_exception(e);
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

            auto def_path = gkDb->read_audio_playback_dlg_settings(AudioPlaybackDlg::GkRecordDlgLastFolderBrowsed); // There has been a previously used path that the user has used, and it's been remembered by Google LevelDB!
            if (def_path.isEmpty()) {
                def_path = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
            }

            //
            // Open a QFileDialog so a user may choose a directory to save files within!
            const QString filePath = QFileDialog::getExistingDirectory(this, tr("Save Directory for Recordings"), def_path);
            m_recordDirPath.setPath(filePath);

            //
            // End recording state...
            gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, true);
            emit recStatus(GkAudioRecordStatus::Finished);
            audio_out_record = false;

            GkAudioFramework::CodecSupport codec_used = gkDb->convCodecSupportFromIdxToEnum(ui->comboBox_playback_rec_codec->currentData().toInt());
            if (m_recordDirPath.isReadable()) { // Verify that the directory itself exists!
                ui->lineEdit_playback_file_location->setText(m_recordDirPath.path());
                gkDb->write_audio_playback_dlg_settings(m_recordDirPath.path(), AudioPlaybackDlg::GkRecordDlgLastFolderBrowsed);
                if (codec_used != GkAudioFramework::CodecSupport::Loopback) {
                    emit recStatus(GkAudioRecordStatus::Active);
                    gkPaAudioPlayer->record(codec_used, m_recordDirPath);
                } else {
                    throw std::runtime_error(tr("Loopback mode is unsupported during recording!").toStdString());
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
        gkStringFuncs->print_exception(e);
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
            return;
        case AUDIO_PLAYBACK_CODEC_LOOPBACK_IDX:
            m_rec_codec_chosen = CodecSupport::Loopback;
            return;
        case AUDIO_PLAYBACK_CODEC_VORBIS_IDX:
            m_rec_codec_chosen = CodecSupport::OggVorbis;
            return;
        case AUDIO_PLAYBACK_CODEC_OPUS_IDX:
            m_rec_codec_chosen = CodecSupport::Opus;
            return;
        case AUDIO_PLAYBACK_CODEC_FLAC_IDX:
            m_rec_codec_chosen = CodecSupport::FLAC;
            return;
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
