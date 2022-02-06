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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_multimedia.hpp"
#include "src/gk_logger.hpp"
#include "src/file_io.hpp"
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QDialog>
#include <QString>
#include <QPointer>
#include <QFileInfo>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/codec_id.h>

#ifdef __cplusplus
} // extern "C"
#endif

namespace Ui {
class GkAudioPlayDialog;
}

class GkAudioPlayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkAudioPlayDialog(QPointer<GekkoFyre::GkLevelDb> database, QPointer<GekkoFyre::GkMultimedia> multimedia,
                               QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkAudioDevices> audioDevs,
                               QPointer<GekkoFyre::GkEventLogger> eventLogger, QWidget *parent = nullptr);
    ~GkAudioPlayDialog() override;

private slots:
    //
    // QPushButtons, etc.
    void on_pushButton_reset_clicked();
    void on_pushButton_close_clicked();
    void on_pushButton_playback_stop_clicked();
    void on_pushButton_playback_play_clicked();
    void on_pushButton_playback_record_clicked();
    void on_pushButton_playback_skip_back_clicked();
    void on_pushButton_playback_skip_forward_clicked();
    void on_pushButton_album_art_icon_clicked();

    void on_comboBox_playback_rec_codec_currentIndexChanged(int index);
    void on_horizontalSlider_playback_rec_bitrate_valueChanged(int value);

    void playlistInsert(const QFileInfo &file_path, const GekkoFyre::GkAudioFramework::GkAudioPlaylistPriority &priority, const bool &printToConsole = false);
    void playlistRemove(const QFileInfo &file_path);
    void startPlaying();
    void setAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &audioState);

    void startRecording(const QFileInfo &file_path, const ALCchar *recording_device, const AVCodecID &codec_id = AV_CODEC_ID_NONE,
                        const int64_t &avg_bitrate = 64000);

    void setBytesRead(const qint64 &bytes, const bool &uncompressed = false);

    void resetStopButtonColor();
    void clearForms(const GekkoFyre::GkAudioFramework::GkClearForms &cat);

signals:
    void recStatus(const GekkoFyre::GkAudioFramework::GkAudioRecordStatus &status); // Sets the status for when recording; whether active, stopped, or to pause...
    void cleanupForms(const GekkoFyre::GkAudioFramework::GkClearForms &cat);

    void addToPlaylist(const QFileInfo &file_path, const GekkoFyre::GkAudioFramework::GkAudioPlaylistPriority &priority, const bool &printToConsole = false);
    void removeFromPlaylist(const QFileInfo &file_path);
    void beginPlaying();
    void updateAudioState(const GekkoFyre::GkAudioFramework::GkAudioState &audioState);

    void beginRecording(const QFileInfo &file_path, const ALCchar *recording_device, const AVCodecID &codec_id = AV_CODEC_ID_NONE,
                        const int64_t &avg_bitrate = 64000);

    void mediaAction(const GekkoFyre::GkAudioFramework::GkAudioState &media_state, const QFileInfo &file_path,
                     const ALCchar *recording_device = nullptr, const AVCodecID &codec_id = AV_CODEC_ID_NONE,
                     const int64_t &avg_bitrate = 64000);

private:
    Ui::GkAudioPlayDialog *ui;

    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkMultimedia> gkMultimedia;
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // Filesystem paths and related
    QDir m_recordDirPath;

    //
    // QPushButtons, etc.
    bool m_audioRecReady;
    bool audio_out_skip_fwd;
    bool audio_out_skip_bck;

    //
    // Audio encoding related objects
    GekkoFyre::GkAudioFramework::CodecSupport m_rec_codec_chosen;
    GekkoFyre::GkAudioFramework::GkAudioState gkAudioState;
    qint32 m_encode_bitrate_chosen;

    //
    // AudioFile objects and related
    std::vector<GekkoFyre::GkAudioFramework::GkAudioFileInfo> gkAudioFileInfo;         // Information on file destined for playback!
    GekkoFyre::Database::Settings::GkAudioChannels m_audioChannels;                    // Audio channel information for both playback and recording!
    qint64 encode_compressed_bytes;
    qint64 encode_uncompressed_bytes;

    template <typename T>
    struct gkConvertDoubleToFloat {
        template <typename U>
        T operator () (const U &x) const { return static_cast<T> (x); }
    };

    void audioPlaybackHelper(const GekkoFyre::GkAudioFramework::CodecSupport &codec_used, const QString &file_path);
    void prefillCodecComboBoxes(const GekkoFyre::GkAudioFramework::CodecSupport &supported_codec);
    void prefillAudioSourceComboBoxes();
    void initProgressBar(const qint32 min = 0, const qint32 max = 100);

    [[nodiscard]] QString openFileBrowser(const bool &isRecord = false);

    void recordLockSettings(const bool &unlock = false);
    void print_exception(const std::exception &e, int level = 0);

};

