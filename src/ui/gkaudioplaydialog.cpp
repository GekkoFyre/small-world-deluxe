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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <exception>
#include <utility>
#include <QTimer>
#include <QIODevice>
#include <QEventLoop>
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

GkAudioPlayDialog::GkAudioPlayDialog(QPointer<GkLevelDb> database, QPointer<GekkoFyre::GkMultimedia> multimedia,
                                     QPointer<GekkoFyre::StringFuncs> stringFuncs,
                                     QPointer<GekkoFyre::GkAudioDevices> audioDevs,
                                     QPointer<GekkoFyre::GkEventLogger> eventLogger, QWidget *parent) : QDialog(parent),
                                     ui(new Ui::GkAudioPlayDialog)
{
    try {
        ui->setupUi(this);

        gkDb = std::move(database);
        gkMultimedia = std::move(multimedia);
        gkStringFuncs = std::move(stringFuncs);
        gkAudioDevices = std::move(audioDevs);
        gkEventLogger = std::move(eventLogger);

        //
        // Signals and Slots
        QObject::connect(this, SIGNAL(updateAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &)),
                         this, SLOT(setAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &)));
        QObject::connect(this, SIGNAL(updateAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &)),
                         gkMultimedia, SLOT(setAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &)));
        QObject::connect(gkMultimedia, SIGNAL(updateAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &)),
                         this, SIGNAL(updateAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &)));
        QObject::connect(this, SIGNAL(addToPlaylist(const QFileInfo &, const GekkoFyre::GkAudioFramework::GkAudioPlaylistPriority &, const bool &)),
                         this, SLOT(playlistInsert(const QFileInfo &, const GekkoFyre::GkAudioFramework::GkAudioPlaylistPriority &, const bool &)));
        QObject::connect(this, SIGNAL(mediaAction(const GekkoFyre::GkAudioFramework::GkAudioState &, const QFileInfo &, const ALCchar *, const GekkoFyre::GkAudioFramework::CodecSupport &, const int64_t &)),
                         gkMultimedia, SLOT(mediaAction(const GekkoFyre::GkAudioFramework::GkAudioState &, const QFileInfo &, const ALCchar *, const GekkoFyre::GkAudioFramework::CodecSupport &, const int64_t &)));
        QObject::connect(this, SIGNAL(beginPlaying()), this, SLOT(startPlaying()));
        QObject::connect(this, SIGNAL(beginRecording(const QFileInfo &, const ALCchar *, const GekkoFyre::GkAudioFramework::CodecSupport &, const int64_t &)),
                         this, SLOT(startRecording(const QFileInfo &, const ALCchar *, const GekkoFyre::GkAudioFramework::CodecSupport &, const int64_t &)));
        QObject::connect(this, SIGNAL(lockSettingsUponRecord(const bool &)), this, SLOT(recordLockSettings(const bool &)));

        //
        // Initialize variables
        gkAudioFileInfo = {};
        m_rec_codec_chosen = CodecSupport::Codec2;
        m_encode_bitrate_chosen = 8;

        //
        // QPushButtons, etc.
        m_audioRecReady = false;
        audio_out_skip_fwd = false;
        audio_out_skip_bck = false;

        //
        // Miscellaneous
        ui->pushButton_playback_stop->setEnabled(false);
        initProgressBar(); // Initialize the QProgressBar with default values!
        emit updateAudioState(GkAudioState::Stopped);

        prefillCodecComboBoxes(GkAudioFramework::CodecSupport::Codec2);
        prefillCodecComboBoxes(GkAudioFramework::CodecSupport::OggVorbis);
        prefillCodecComboBoxes(GkAudioFramework::CodecSupport::AAC);
        prefillCodecComboBoxes(GkAudioFramework::CodecSupport::Opus);
        prefillCodecComboBoxes(GkAudioFramework::CodecSupport::FLAC);
        prefillCodecComboBoxes(GkAudioFramework::CodecSupport::PCM);
        prefillCodecComboBoxes(GkAudioFramework::CodecSupport::RawData);
        prefillCodecComboBoxes(GkAudioFramework::CodecSupport::Loopback);
        prefillAudioSourceComboBoxes();
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue was encountered upon initiation of audio libraries. Error: %1")
        .arg(QString::fromStdString(e.what())), QMessageBox::Ok);
    }

    return;
}

GkAudioPlayDialog::~GkAudioPlayDialog()
{
    emit recStatus(GkAudioRecordStatus::Defunct);
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
    try {
        switch (gkAudioState) {
            case GkAudioState::Playing:
                ui->pushButton_playback_play->setEnabled(true);
                break;
            case GkAudioState::Recording:
                ui->pushButton_playback_record->setEnabled(true);
                emit lockSettingsUponRecord(false);
                break;
            case GkAudioState::Stopped:
                break;
            default:
                break;
        }

        //
        // Finally, update the state to that of being Stopped, now that we don't need to know the original, previous
        // state!
        emit updateAudioState(GkAudioState::Stopped);
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_playback_play_clicked disable any recording in progress
 * upon being clicked.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Karl <https://stackoverflow.com/a/7995655>
 */
void GkAudioPlayDialog::on_pushButton_playback_play_clicked()
{
    try {
        const auto filePath = openFileBrowser(false);

        //
        // Write out information about the file in question!
        emit cleanupForms(GkClearForms::All); // Cleanup everything!

        ui->pushButton_playback_stop->setEnabled(true);
        ui->pushButton_playback_play->setEnabled(false);
        ui->pushButton_playback_record->setEnabled(false);

        emit addToPlaylist(filePath, GkAudioPlaylistPriority::Normal, true);
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::on_pushButton_playback_record_clicked disable any playback in progress
 * upon being clicked.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note High Quality Recording with FFmpeg <https://trac.ffmpeg.org/wiki/Encode/HighQualityAudio>.
 */
void GkAudioPlayDialog::on_pushButton_playback_record_clicked()
{
    try {
        const CodecSupport codec_id = gkMultimedia->convAudioCodecIdxToEnum(ui->comboBox_playback_rec_codec->currentIndex());
        const QString chosen_openal_audio_dev = ui->comboBox_playback_rec_source->currentData().toString();
        if (chosen_openal_audio_dev.isEmpty()) {
            throw std::invalid_argument(tr("An invalid audio device has been specified; please check your settings and try again.").toStdString());
        }

        if (codec_id != CodecSupport::Unsupported) {
            const auto filePath = openFileBrowser(true);
            if (!filePath.isEmpty()) {
                ui->pushButton_playback_skip_back->setEnabled(false);
                ui->pushButton_playback_skip_forward->setEnabled(false);
                ui->pushButton_playback_play->setEnabled(false);
                ui->pushButton_playback_record->setEnabled(false);
                ui->pushButton_playback_stop->setEnabled(true);

                //
                // Initiate the recording process!
                emit updateAudioState(GkAudioState::Recording);
                emit lockSettingsUponRecord(true);
                emit beginRecording(filePath, chosen_openal_audio_dev.toStdString().c_str(),
                                    codec_id, ui->horizontalSlider_playback_rec_bitrate->value());
            }

            return;
        } else {
            throw std::invalid_argument(tr("An invalid or currently unsupported codec for encoding hence-with has been selected!").toStdString());
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "",
                                    false, true, false, true, false);
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
 * @brief GkAudioPlayDialog::on_pushButton_album_art_icon_clicked modifies and/or displays the album art for the given,
 * opened audio file.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::on_pushButton_album_art_icon_clicked()
{
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
        case AUDIO_PLAYBACK_CODEC_AAC_IDX:
            m_rec_codec_chosen = CodecSupport::AAC;
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
        case AUDIO_PLAYBACK_CODEC_RAW_IDX:
            m_rec_codec_chosen = CodecSupport::RawData;
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
 * @brief GkAudioPlayDialog::playlistInsert will analyze a given audio file at a certain file path for the codec it
 * uses among other tidbits of information, such as the album name, artist(s) involved, sample rate, bitrate, file size,
 * and more, while outputting all this information to a designated form. Lastly, it will add the given multimedia file
 * to a playlist so that in turn, multiple multimedia files maybe queued for playing over time.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The file path of the given audio file in question, to be analyzed.
 * @param priority The deemed priority of the given audio file within the queue. If set to 'High', then the audio file
 * must be played right away, but otherwise if normal then simply add to the queue.
 * @param printToConsole Whether to print some interesting information about the audio file in-question to the console
 * or not.
 * @note Cornstalks <https://www.gamedev.net/forums/topic/624876-how-to-read-an-audio-file-with-ffmpeg-in-c/>.
 */
void GkAudioPlayDialog::playlistInsert(const QFileInfo &file_path, const GkAudioPlaylistPriority &priority,
                                       const bool &printToConsole)
{
    try {
        if (file_path.exists() && file_path.isFile()) {
            if (file_path.isReadable()) {
                GkAudioFileInfo audioFileInfo;
                audioFileInfo.audio_file_path = file_path;
                audioFileInfo.file_size_hr = gkStringFuncs->fileSizeHumanReadable(file_path.size());

                //
                // Setup TagLib parameters!
                GkAudioFramework::GkAudioFileMetadata meta;
                TagLib::FileRef fileRef(file_path.canonicalFilePath().toStdString().c_str());
                TagLib::Tag *tag = fileRef.tag();

                //
                // Information for gathering properties about the audio source, the codec, etc. itself with regard to
                // the file, via TagLib!
                TagLib::AudioProperties *properties = fileRef.audioProperties();

                audioFileInfo.num_audio_channels = gkAudioDevices->convAudioChannelsToEnum(properties->channels());
                audioFileInfo.info.sampleRate = properties->sampleRate(); // Measured in plain Hz!
                audioFileInfo.bit_depth = properties->bitrate();
                audioFileInfo.type_codec_str = tr("Unsupported"); // TODO: Work on this feature when the time comes!
                audioFileInfo.info.lengthInSeconds = properties->lengthInSeconds();
                audioFileInfo.info.lengthInMilliseconds = properties->lengthInMilliseconds();

                QString lengthMinutesHr = tr("0 seconds");
                if (audioFileInfo.info.lengthInSeconds > 0) {
                    lengthMinutesHr = gkStringFuncs->convSecondsToMinutes(static_cast<double>(audioFileInfo.info.lengthInSeconds));
                }

                meta.title = QString::fromWCharArray(tag->title().toCWString());
                meta.artist = QString::fromWCharArray(tag->artist().toCWString());
                meta.album = QString::fromWCharArray(tag->album().toCWString());
                meta.year_raw = tag->year();
                meta.comment = QString::fromWCharArray(tag->comment().toCWString());
                meta.track_no = tag->track();
                meta.genre = QString::fromWCharArray(tag->genre().toCWString());
                audioFileInfo.metadata = meta;

                ui->lineEdit_playback_file_location->setText(audioFileInfo.audio_file_path.canonicalFilePath());
                ui->lineEdit_playback_file_size->setText(audioFileInfo.file_size_hr);
                ui->lineEdit_playback_audio_codec->setText(audioFileInfo.type_codec_str);
                ui->lineEdit_playback_sample_rate->setText(QString::number(audioFileInfo.info.sampleRate));
                ui->lineEdit_playback_bitrate->setText(QString::number(audioFileInfo.bit_depth));

                ui->lineEdit_playback_title->setText(audioFileInfo.metadata.title);
                ui->lineEdit_playback_artist->setText(audioFileInfo.metadata.artist);
                ui->lineEdit_playback_album->setText(audioFileInfo.metadata.album);

                if (printToConsole) {
                    //
                    // Print a separator...
                    std::cout << "--------------------------------------------------" << std::endl;

                    //
                    // Print out some useful information!
                    std::cout << tr("Stream Index: #").toStdString() << audioFileInfo.info.stream_idx << std::endl;
                    std::cout << tr("Bitrate: ").toStdString() << audioFileInfo.info.bitrate << std::endl;
                    std::cout << tr("Sample rate: ").toStdString() << audioFileInfo.info.sampleRate << std::endl;
                    std::cout << tr("Channels: ").toStdString() << audioFileInfo.info.channels << std::endl;
                    std::cout << tr("Sample format: ").toStdString() << audioFileInfo.info.sample_format_str.toStdString() << std::endl;
                    std::cout << tr("Sample size: ").toStdString() << audioFileInfo.info.sample_size << std::endl;

                    //
                    // Print a separator...
                    std::cout << "--------------------------------------------------" << std::endl;
                }

                switch (priority) {
                    case GkAudioPlaylistPriority::Normal:
                        gkAudioFileInfo.push_back(audioFileInfo);
                        break;
                    case GkAudioPlaylistPriority::High:
                        gkAudioFileInfo.insert(gkAudioFileInfo.begin(), audioFileInfo); // TODO: Finish implementing this!
                        break;
                    default:
                        break;
                }

                if (gkAudioState == GkAudioState::Stopped) {
                    emit updateAudioState(GkAudioState::Playing);
                    emit beginPlaying();
                }
            }
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An issue was encountered whilst modifying the playlist. Error: %1")
        .arg(QString::fromStdString(e.what())), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::playlistRemove removes a given multimedia file from the playlist, so that it can no longer
 * be played in turn from the queue of audio files.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path
 */
void GkAudioPlayDialog::playlistRemove(const QFileInfo &file_path)
{
    return;
}

/**
 * @brief GkAudioPlayDialog::startPlaying will initiate the playing process of any multimedia files within a specific
 * queue.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note user3722440 <https://stackoverflow.com/q/50450912>.
 */
void GkAudioPlayDialog::startPlaying()
{
    try {
        for (const auto &audio_file: gkAudioFileInfo) {
            emit mediaAction(GkAudioState::Playing, audio_file.audio_file_path);
            QEventLoop loop;
            QObject::connect(gkMultimedia, SIGNAL(playingFinished()), &loop, SLOT(quit()));
            loop.exec(); // By using QEventLoop, we avoid freezing up the GUI and having to use std::thread!
        }
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An issue was encountered whilst modifying the playlist. Error: %1")
        .arg(QString::fromStdString(e.what())), GkSeverity::Fatal, "", false, true, false, true, false);
    }

    return;
}

/**
 * @brief GkAudioPlayDialog::setAudioState will update the currently available audio state so that this class knows
 * whether we are in playback mode, recording, or have stopped all processes.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::setAudioState(const GkAudioState &audioState)
{
    gkAudioState = audioState;

    return;
}

/**
 * @brief GkAudioPlayDialog::startRecording initiates the process of recording and encoding audio to a given file, with
 * audio samples gathered from the OpenAL library.
 * @param file_path The file path of where we are to send and save our buffered audio data towards.
 * @param recording_device The audio device we are to record an audio stream from.
 * @param codec_id The codec used in encoding a given audio stream.
 * @param avg_bitrate The average bitrate for encoding with. This is unused for constant quantizer encoding.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAudioPlayDialog::startRecording(const QFileInfo &file_path, const ALCchar *recording_device,
                                       const CodecSupport &codec_id, const int64_t &avg_bitrate)
{
    try {
        emit mediaAction(GkAudioState::Recording, file_path, recording_device, codec_id, avg_bitrate);
        QEventLoop loop;
        QObject::connect(gkMultimedia, SIGNAL(recordingFinished()), &loop, SLOT(quit()));
        loop.exec(); // By using QEventLoop, we avoid freezing up the GUI and having to use std::thread!
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("An issue was encountered whilst initiating the audio recording/encoding process. Error: %1")
        .arg(QString::fromStdString(e.what())), GkSeverity::Fatal, "", false, true, false, true, false);
    }

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

    ui->lineEdit_playback_artist->setReadOnly(!unlock);
    ui->lineEdit_playback_title->setReadOnly(!unlock);
    ui->lineEdit_playback_album->setReadOnly(!unlock);
    ui->pushButton_album_art_icon->setEnabled(unlock);

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
    // audio_out_stop = false;

    gkStringFuncs->changePushButtonColor(ui->pushButton_playback_play, true);
    // audio_out_play = false;

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
            ui->lineEdit_playback_title->clear();
            ui->lineEdit_playback_audio_codec->clear();
            ui->lineEdit_playback_bitrate->clear();
            ui->lineEdit_playback_sample_rate->clear();
            ui->label_playback_timer->setText(tr("-- : --"));
            ui->progressBar_playback->setFormat(tr("Waiting for user input..."));

            break;
        case GkClearForms::RecordingAudio:
            ui->lineEdit_playback_file_location->clear();
            ui->lineEdit_playback_file_size->clear();
            ui->lineEdit_playback_audio_codec->clear();
            ui->lineEdit_playback_bitrate->clear();
            ui->lineEdit_playback_sample_rate->clear();
            ui->comboBox_playback_rec_codec->setCurrentIndex(AUDIO_PLAYBACK_CODEC_CODEC2_IDX);
            ui->horizontalSlider_playback_rec_bitrate->setValue(AUDIO_RECORDING_DEF_BITRATE);
            ui->label_playback_timer->setText(tr("-- : --"));
            ui->progressBar_playback->setFormat(tr("Waiting for user input..."));

            break;
        case GkClearForms::All:
            ui->lineEdit_playback_file_location->clear();
            ui->lineEdit_playback_file_size->clear();
            ui->lineEdit_playback_title->clear();
            ui->lineEdit_playback_audio_codec->clear();
            ui->lineEdit_playback_bitrate->clear();
            ui->lineEdit_playback_sample_rate->clear();
            ui->comboBox_playback_rec_codec->setCurrentIndex(AUDIO_PLAYBACK_CODEC_CODEC2_IDX);
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
                // gkPaAudioPlayer->play(m_rec_codec_chosen, file_path, gkSysInputAudioDev.audio_src);
                break;
            case AUDIO_RECORDING_SOURCE_OUTPUT_IDX:
                // gkPaAudioPlayer->play(m_rec_codec_chosen, file_path, gkSysOutputAudioDev.audio_src);
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
        case CodecSupport::AAC:
            ui->comboBox_playback_rec_codec->insertItem(AUDIO_PLAYBACK_CODEC_AAC_IDX, tr("AAC (w/ ADTS container)"), AUDIO_PLAYBACK_CODEC_AAC_IDX);
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
        case CodecSupport::RawData:
            ui->comboBox_playback_rec_codec->insertItem(AUDIO_PLAYBACK_CODEC_RAW_IDX, tr("Raw Data"), AUDIO_PLAYBACK_CODEC_RAW_IDX);
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
    const QString audio_dev_input_desc = gkMultimedia->getInputAudioDevice().audio_dev_str;
    const QString audio_dev_output_desc = gkMultimedia->getOutputAudioDevice().audio_dev_str;

    ui->comboBox_playback_rec_source->insertItem(AUDIO_RECORDING_SOURCE_INPUT_IDX, tr("Audio Input [ %1 ]").arg(audio_dev_input_desc), AUDIO_RECORDING_SOURCE_INPUT_IDX);
    ui->comboBox_playback_rec_source->insertItem(AUDIO_RECORDING_SOURCE_OUTPUT_IDX, tr("Audio Output [ %1 ]").arg(audio_dev_output_desc), AUDIO_RECORDING_SOURCE_OUTPUT_IDX);

    ui->comboBox_playback_rec_source->setItemData(AUDIO_RECORDING_SOURCE_INPUT_IDX, audio_dev_input_desc);
    ui->comboBox_playback_rec_source->setItemData(AUDIO_RECORDING_SOURCE_OUTPUT_IDX, audio_dev_output_desc);

    return;
}

/**
 * @brief GkAudioPlayDialog::initProgressBar initializes a given QProgressBar with default values.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param min The most minimum value of the QProgressBar in question.
 * @param max The most maximum value of the QProgressBar in question.
 */
void GkAudioPlayDialog::initProgressBar(const qint32 min, const qint32 max)
{
    ui->progressBar_playback->setMinimum(0);
    ui->progressBar_playback->setMaximum(100);
    ui->progressBar_playback->setValue(0);

    return;
}

/**
 * @brief GkAudioPlayDialog::openFileBrowser will open a file dialog for either the selection of files for playback or
 * the choosing of a destination to record a given file towards.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param isRecord Are we opening a file dialog for the purposes of recording to a file, or for the purposes of playing
 * back an existing files content?
 */
QString GkAudioPlayDialog::openFileBrowser(const bool &isRecord)
{
    QString filePath;
    QString defPath = gkDb->read_audio_playback_dlg_settings(AudioPlaybackDlg::GkAudioDlgLastFolderBrowsed); // There has been a previously used path that the user has used, and it's been remembered by Google LevelDB!
    if (defPath.isEmpty()) {
        defPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }

    if (isRecord) {
        //
        // We are recording to a newly created file!
        filePath = QFileDialog::getSaveFileName(this, tr("Audio File Recording"), defPath, tr("Audio Files %1;;All Files (*.*)").arg(General::GkAudio::commonAudioFileFormats));
    } else {
        //
        // We are playing back the data of an existing file!
        filePath = QFileDialog::getOpenFileName(this, tr("Audio File Playback"), defPath, tr("Audio Files %1;;All Files (*.*)").arg(General::GkAudio::commonAudioFileFormats));
    }

    if (filePath.isEmpty()) { // No file has supposedly been chosen!
        emit cleanupForms(GkClearForms::Playback); // Cleanup just the playback section only!
        return QString();
    }

    //
    // Write out the path to the Google LevelDB database, so we can recall it in the future as history!
    gkDb->write_audio_playback_dlg_settings(filePath, AudioPlaybackDlg::GkAudioDlgLastFolderBrowsed);

    return filePath;
}

/**
 * @brief GkAudioPlayDialog::print_exception
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param e
 * @param level
 */
void GkAudioPlayDialog::print_exception(const std::exception &e, int level)
{
    gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);

    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        print_exception(e, level + 1);
    } catch (...) {}

    return;
}
