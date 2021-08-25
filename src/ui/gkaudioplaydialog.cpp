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
    QObject::connect(gkAudioEncoding, SIGNAL(bytesRead(const qint64 &, const bool &)),
                     this, SLOT(setBytesRead(const const qint64 &, const bool &)));
    QObject::connect(this, SIGNAL(cleanupForms(const GekkoFyre::GkAudioFramework::GkClearForms &)),
                     this, SLOT(clearForms(const GekkoFyre::GkAudioFramework::GkClearForms &)));

    //
    // Initialize variables
    //
    gkAudioFileInfo = {};
    pref_input_device = input_device;
    pref_output_device = output_device;
    m_rec_codec_chosen = CodecSupport::Codec2;
    m_encode_bitrate_chosen = 8;
    gkAudioFile = std::make_shared<AudioFile<double>>();
    gkPaAudioPlayer = new GkPaAudioPlayer(gkDb, gkAudioOutput, gkAudioInput, gkAudioEncoding, gkEventLogger, gkAudioFile, this);
    gkPaAudioPlayer->moveToThread(parent->thread());

    //
    // QPushButtons, etc.
    //
    audio_out_play = false;
    audio_out_stop = false;
    m_audioRecReady = false;
    audio_out_skip_fwd = false;
    audio_out_skip_bck = false;

    pref_input_device.audio_src = GkAudioSource::Input;
    pref_output_device.audio_src = GkAudioSource::Output;

    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::Codec2);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::OggVorbis);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::FLAC);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::Opus);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::PCM);
    prefillCodecComboBoxes(GkAudioFramework::CodecSupport::Loopback);
    prefillAudioSourceComboBoxes();
}

GkAudioPlayDialog::~GkAudioPlayDialog()
{
    emit recStatus(GkAudioRecordStatus::Defunct);
    if (audio_out_play) {
        if (!m_recordDirPath.isEmpty()) {
            gkPaAudioPlayer->stop(m_recordDirPath.canonicalPath(), GkAudioSource()); // Directory path to where recordings are saved!
        } else {
            gkPaAudioPlayer->stop(gkAudioFileInfo.audio_file_path, GkAudioSource()); // Path to file that is being played!
        }

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

    emit cleanupForms(GkClearForms::All);
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
        if (!m_recordDirPath.isEmpty()) {
            gkPaAudioPlayer->stop(m_recordDirPath.canonicalPath(), GkAudioSource()); // Directory path to where recordings are saved!
        } else {
            gkPaAudioPlayer->stop(gkAudioFileInfo.audio_file_path, GkAudioSource()); // Path to file that is being played!
        }
        
        gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, true);
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
            if (filePath.isEmpty()) { // No file has supposedly been chosen!
                emit cleanupForms(GkClearForms::Playback);
                return;
            }

            gkAudioFileInfo.audio_file_path.setFile(filePath);
            gkAudioFile->load(gkAudioFileInfo.audio_file_path.filePath().toStdString());

            //
            // Write out information about the file in question!
            emit cleanupForms(GkClearForms::All);
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

            if (gkAudioFileInfo.audio_file_path.exists() && m_rec_codec_chosen != GkAudioFramework::CodecSupport::Loopback) {
                audioPlaybackHelper(m_rec_codec_chosen, gkAudioFileInfo.audio_file_path.filePath());
                gkEventLogger->publishEvent(
                        tr("Started playing audio file, \"%1\"").arg(gkAudioFileInfo.audio_file_path.filePath()),
                        GkSeverity::Info, "", true, true, true, false);
                ui->progressBar_playback->setFormat(tr("%p%")); // Modify the QProgressBar to display the correct text!
            } else if (m_rec_codec_chosen == GkAudioFramework::CodecSupport::Loopback) {
                audioPlaybackHelper(m_rec_codec_chosen, gkAudioFileInfo.audio_file_path.filePath());
                gkEventLogger->publishEvent(
                        tr("Started audio device loopback!"), GkSeverity::Info, "", true, true, true, false);
                ui->progressBar_playback->setFormat(tr("%p%")); // Modify the QProgressBar to display the correct text!
            } else {
                throw std::runtime_error(tr("Error with audio playback! Does the file, \"%1\", actually exist?")
                .arg(gkAudioFileInfo.audio_file_path.fileName()).toStdString());
            }
        } else {
            gkPaAudioPlayer->stop(gkAudioFileInfo.audio_file_path, GkAudioSource());
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
        if (m_recordDirPath.path().isEmpty() || !m_audioRecReady) { // We first need to choose a destination to record towards!
            auto def_path = gkDb->read_audio_playback_dlg_settings(AudioPlaybackDlg::GkRecordDlgLastFolderBrowsed); // There has been a previously used path that the user has used, and it's been remembered by Google LevelDB!
            if (def_path.isEmpty()) {
                def_path = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
            }

            //
            // Open a QFileDialog so a user may choose a directory to save files within!
            const QString filePath = QFileDialog::getExistingDirectory(this, tr("Save Directory for Recordings"), def_path);
            gkDb->write_audio_playback_dlg_settings(QString::number(ui->comboBox_playback_rec_codec->currentIndex()),
                                                    AudioPlaybackDlg::GkRecordDlgLastCodecSelected);
            if (filePath.isEmpty()) { // No file has supposedly been chosen!
                emit cleanupForms(GkClearForms::Recording);
                return;
            }

            if (!m_recordDirPath.isReadable()) {
                emit cleanupForms(GkClearForms::Recording);
                throw std::invalid_argument(tr("Unable to use directory, \"%1\", for reading and/or writing!")
                .arg(m_recordDirPath.path()).toStdString());
            }

            //
            // The chosen directory/path is legitimate and ready-to-use!
            ui->lineEdit_playback_file_location->setText(m_recordDirPath.path());
            gkDb->write_audio_playback_dlg_settings(m_recordDirPath.path(), AudioPlaybackDlg::GkRecordDlgLastFolderBrowsed);

            //
            // Reset all of the form elements and set the designated path...
            emit cleanupForms(GkClearForms::All);
            const auto last_codec_used = gkDb->read_audio_playback_dlg_settings(AudioPlaybackDlg::GkRecordDlgLastCodecSelected);

            ui->comboBox_playback_rec_codec->setCurrentIndex(last_codec_used.toInt());
            m_recordDirPath.setPath(filePath);
            m_audioRecReady = true; // Recording state is TRUE!
            emit recStatus(GkAudioRecordStatus::Paused);

            return;
        } else if (gkAudioEncoding->getRecStatus() == GkAudioRecordStatus::Paused) { // Now that the destination has been chosen, start recording when the button is pressed again!
            //
            // Start recording; begin state...
            if (m_recordDirPath.isReadable()) { // Verify that the directory itself exists!
                //
                // Determine the codec used...
                if (m_rec_codec_chosen != GkAudioFramework::CodecSupport::Loopback) {
                    switch (ui->comboBox_playback_rec_source->currentIndex()) {
                        case AUDIO_RECORDING_SOURCE_INPUT_IDX:
                            emit recStatus(GkAudioRecordStatus::Active);
                            recordLockSettings(false);
                            gkPaAudioPlayer->record(m_rec_codec_chosen, m_recordDirPath, ui->horizontalSlider_playback_rec_bitrate->value(), pref_input_device.audio_src);
                            break;
                        case AUDIO_RECORDING_SOURCE_OUTPUT_IDX:
                            emit recStatus(GkAudioRecordStatus::Active);
                            recordLockSettings(false);
                            gkPaAudioPlayer->record(m_rec_codec_chosen, m_recordDirPath, ui->horizontalSlider_playback_rec_bitrate->value(), pref_output_device.audio_src);
                            break;
                        default:
                            throw std::invalid_argument(tr("Invalid argument provided for audio device determination, when attempting to record!").toStdString());
                    }

                    gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, false);
                } else {
                    throw std::runtime_error(tr("Loopback mode is unsupported during recording!").toStdString());
                }
            } else {
                throw std::invalid_argument(tr("Unable to use directory, \"%1\", for reading and/or writing!")
                .arg(m_recordDirPath.path()).toStdString());
            }
        } else {
            //
            // End or pause recording; end state...
            m_audioRecReady = false; // Recording state is FALSE!
            emit recStatus(GkAudioRecordStatus::Finished);
            gkStringFuncs->changePushButtonColor(ui->pushButton_playback_record, true);

            return;
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
        case AUDIO_PLAYBACK_CODEC_CODEC2_IDX:
            m_rec_codec_chosen = CodecSupport::Codec2;
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
 * @brief GkAudioPlayDialog::setBytesRead adjusts the GUI widget(s) in question to display the amount of bytes read so
 * far, whether it be for uncompressed or compressed data.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bytes The amount of data read so far, measured in bytes.
 * @param uncompressed Are we displaying measurements for uncompressed or compressed data?
 */
void GkAudioPlayDialog::setBytesRead(const qint64 &bytes, const bool &uncompressed)
{
    // const auto hr_file_size = gkStringFuncs->fileSizeHumanReadable(bytes);
    if (uncompressed) {
        encode_uncompressed_bytes = bytes;
    } else {
        encode_compressed_bytes = bytes;
    }

    if (encode_compressed_bytes <= encode_uncompressed_bytes) {
        ui->progressBar_playback->setFormat("%p%");
        ui->progressBar_playback->setValue((encode_uncompressed_bytes / encode_compressed_bytes) * 100);
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
 * @brief GkAudioPlayDialog::clearForms will clear and reset the UI forms back to their default state.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param cat The category of UI elements to clear, whether it be for example, just the Playback section, Recording, or
 * everything at once.
 */
void GkAudioPlayDialog::clearForms(const GkClearForms &cat)
{
    switch (cat) {
        case GkClearForms::Playback:
            ui->lineEdit_playback_file_location->clear();
            ui->lineEdit_playback_file_size->clear();
            ui->lineEdit_playback_file_name->clear();
            ui->lineEdit_playback_audio_codec->clear();
            ui->lineEdit_playback_bitrate->clear();
            ui->lineEdit_playback_sample_rate->clear();
            ui->label_playback_timer->setText(tr("-- : --"));
            ui->progressBar_playback->setFormat(tr("Waiting for user input..."));

            break;
        case GkClearForms::Recording:
            ui->lineEdit_playback_file_location->clear();
            ui->lineEdit_playback_file_size->clear();
            ui->lineEdit_playback_audio_codec->clear();
            ui->lineEdit_playback_bitrate->clear();
            ui->lineEdit_playback_sample_rate->clear();
            ui->comboBox_playback_rec_codec->setCurrentIndex(AUDIO_PLAYBACK_CODEC_VORBIS_IDX);
            ui->horizontalSlider_playback_rec_bitrate->setValue(AUDIO_RECORDING_DEF_BITRATE);
            ui->label_playback_timer->setText(tr("-- : --"));
            ui->progressBar_playback->setFormat(tr("Waiting for user input..."));

            break;
        case GkClearForms::All:
            ui->lineEdit_playback_file_location->clear();
            ui->lineEdit_playback_file_size->clear();
            ui->lineEdit_playback_file_name->clear();
            ui->lineEdit_playback_audio_codec->clear();
            ui->lineEdit_playback_bitrate->clear();
            ui->lineEdit_playback_sample_rate->clear();
            ui->comboBox_playback_rec_codec->setCurrentIndex(AUDIO_PLAYBACK_CODEC_VORBIS_IDX);
            ui->horizontalSlider_playback_rec_bitrate->setValue(AUDIO_RECORDING_DEF_BITRATE);
            ui->label_playback_timer->setText(tr("-- : --"));
            ui->progressBar_playback->setFormat(tr("Waiting for user input..."));

            break;
        default:
            break;
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::audioPlaybackHelper
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param m_rec_codec_chosen
 * @param file_path
 */
void GkAudioPlayDialog::audioPlaybackHelper(const CodecSupport &m_rec_codec_chosen, const QString &file_path)
{
    try {
        switch (ui->comboBox_playback_rec_source->currentIndex()) {
            case AUDIO_RECORDING_SOURCE_INPUT_IDX:
                gkPaAudioPlayer->play(m_rec_codec_chosen, file_path, pref_input_device.audio_src);
                break;
            case AUDIO_RECORDING_SOURCE_OUTPUT_IDX:
                gkPaAudioPlayer->play(m_rec_codec_chosen, file_path, pref_output_device.audio_src);
                break;
            default:
                throw std::invalid_argument(tr("Invalid argument provided for audio device determination, when attempting playback!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

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
        case CodecSupport::Codec2:
            ui->comboBox_playback_rec_codec->insertItem(AUDIO_PLAYBACK_CODEC_CODEC2_IDX, tr("Codec2"), AUDIO_PLAYBACK_CODEC_CODEC2_IDX);
            break;
        case CodecSupport::Unsupported:
            break;
        case CodecSupport::Unknown:
            break;
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::prefillAudioSourceComboBoxes
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::prefillAudioSourceComboBoxes()
{
    ui->comboBox_playback_rec_source->insertItem(AUDIO_RECORDING_SOURCE_INPUT_IDX, tr("Audio Input [ %1 ]").arg(pref_input_device.audio_dev_str), AUDIO_RECORDING_SOURCE_INPUT_IDX);
    ui->comboBox_playback_rec_source->insertItem(AUDIO_RECORDING_SOURCE_OUTPUT_IDX, tr("Audio Output [ %1 ]").arg(pref_output_device.audio_dev_str), AUDIO_RECORDING_SOURCE_OUTPUT_IDX);

    return;
}

/**
 * @brief GkAudioPlayDialog::recordLockSettings will lock the form/settings upon a recording session being initiated, with
 * the exception of the 'Stop' button and related.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::recordLockSettings(const bool &unlock)
{
    ui->comboBox_playback_rec_codec->setEnabled(unlock);
    ui->comboBox_playback_rec_source->setEnabled(unlock);
    ui->horizontalSlider_playback_rec_bitrate->setEnabled(unlock);

    ui->pushButton_playback_skip_back->setEnabled(unlock);
    ui->pushButton_playback_skip_forward->setEnabled(unlock);
    ui->pushButton_playback_play->setEnabled(unlock);

    return;
}
