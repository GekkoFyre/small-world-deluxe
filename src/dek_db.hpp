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
#include "src/file_io.hpp"
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <memory>
#include <string>
#include <QObject>
#include <QString>
#include <QPointer>

namespace GekkoFyre {

class GkLevelDb : public QObject {
    Q_OBJECT

public:
    explicit GkLevelDb(leveldb::DB *db_ptr, QPointer<GekkoFyre::FileIo> filePtr,
                       QObject *parent = nullptr);
    ~GkLevelDb();

    void write_rig_settings(const QString &value, const Database::Settings::radio_cfg &key);
    void write_rig_settings_comms(const QString &value, const Database::Settings::radio_cfg &key,
                                  const AmateurRadio::GkConnType &conn_type = AmateurRadio::GkConnType::None);
    void write_audio_device_settings(const GekkoFyre::Database::Settings::Audio::GkDevice &value,
                                     const bool &is_output_device);
    void write_mainwindow_settings(const QString &value, const Database::Settings::general_mainwindow_cfg &key);
    void write_misc_audio_settings(const QString &value, const Database::Settings::audio_cfg &key);
    void write_frequencies_db(const AmateurRadio::GkFreqs &write_new_value,
                              const AmateurRadio::GkFreqs &write_old_value = AmateurRadio::GkFreqs());

    QString read_rig_settings(const Database::Settings::radio_cfg &key);
    QString read_rig_settings_comms(const Database::Settings::radio_cfg &key,
                                    const AmateurRadio::GkConnType &conn_type = AmateurRadio::GkConnType::None);
    int read_audio_device_settings(const bool &is_output_device);
    GekkoFyre::Database::Settings::Audio::GkDevice read_audio_details_settings(const bool &is_output_device);
    QString read_mainwindow_settings(const Database::Settings::general_mainwindow_cfg &key);
    QString read_misc_audio_settings(const GekkoFyre::Database::Settings::audio_cfg &key);

    GekkoFyre::Database::Settings::audio_channels convertAudioChannelsEnum(const int &audio_channel_sel);
    int convertAudioChannelsInt(const GekkoFyre::Database::Settings::audio_channels &channel_enum) const;
    bool convertAudioEnumIsStereo(const GekkoFyre::Database::Settings::audio_channels &channel_enum) const;

    ptt_type_t convPttTypeToEnum(const QString &ptt_type_str);
    QString convPttTypeToStr(const ptt_type_t &ptt_type_enum);
    AmateurRadio::GkConnType convConnTypeToEnum(const int &conn_type);
    int convConnTypeToInt(const AmateurRadio::GkConnType &conn_type);

    QString convAudioBitrateToStr(const GekkoFyre::GkAudioFramework::Bitrate &bitrate);

    QString convBandsToStr(const GekkoFyre::AmateurRadio::GkFreqBands &band);
    QString convDigitalModesToStr(const GekkoFyre::AmateurRadio::DigitalModes &digital_mode);
    QString convIARURegionToStr(const GekkoFyre::AmateurRadio::IARURegions &iaru_region);

    void write_audio_api_settings(const PaHostApiTypeId &interface);
    PaHostApiTypeId read_audio_api_settings();
    QString portAudioApiToStr(const PaHostApiTypeId &interface);
    PaHostApiTypeId portAudioApiToEnum(const QString &interface);

    std::string removeInvalidChars(const std::string &string_to_modify);
    std::string boolEnum(const bool &is_true);
    bool boolStr(const std::string &is_true);

private:
    QPointer<GekkoFyre::FileIo> fileIo;
    leveldb::DB *db;

    std::string processCsvToLevelDB(const std::string &comma_sep_values, const std::string &data_to_append = "", const std::string &old_value = "");

};
};
