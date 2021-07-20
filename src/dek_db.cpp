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

#include "dek_db.hpp"
#include "src/audio_devices.hpp"
#include "src/contrib/rapidcsv/src/rapidcsv.h"
#include <leveldb/cache.h>
#include <leveldb/options.h>
#include <leveldb/slice.h>
#include <leveldb/table.h>
#include <leveldb/write_batch.h>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <QGuiApplication>
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>
#include <QVariant>
#include <QSysInfo>
#include <QDebug>
#include <tuple>
#include <algorithm>
#include <exception>
#include <iterator>
#include <sstream>
#include <utility>
#include <vector>
#include <random>
#include <chrono>
#include <ctime>

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Language;
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

namespace fs = boost::filesystem;
namespace sys = boost::system;

std::mutex read_audio_dev_mtx;
std::mutex read_audio_api_mtx;
std::mutex mtx_freq_already_init;

GkLevelDb::GkLevelDb(leveldb::DB *db_ptr, QPointer<FileIo> filePtr, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                     const QRect &main_win_geometry, QObject *parent) : QObject(parent)
{
    db = db_ptr;
    fileIo = std::move(filePtr);
    gkStringFuncs = std::move(stringFuncs);
    gkMainWinGeometry = main_win_geometry;
}

GkLevelDb::~GkLevelDb()
{}

/**
 * @brief GkLevelDb::writeMultipleKeys stores multiple values under the 'one' key within the Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param base_key_name The base-name of the key to start with.
 * @param values The values that are to be written under the 'one' key.
 * @param allow_empty_values If present, then whether to allow the insertion of empty values into the Google LevelDB
 * database with it's matching, strictly non-empty key.
 * @note miste...@googlemail.com <https://groups.google.com/g/leveldb/c/Bq30nafYSTY/m/NTbE0lqxEAYJ>.
 * @see GkLevelDb::readMultipleKeys().
 */
void GkLevelDb::writeMultipleKeys(const std::string &base_key_name, const std::vector<std::string> &values,
                                  const bool &allow_empty_values)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        if (!base_key_name.empty() && !values.empty()) {
            //
            // See if there are any values already pre-existing under this base-key! As we must prepend to these if
            // that's the case.
            //
            std::vector<std::string> values_modifiable;
            std::copy(values.begin(), values.end(), std::back_inserter(values_modifiable));
            auto preexisting_values = readMultipleKeys(base_key_name);

            if (!preexisting_values.empty()) {
                values_modifiable.insert(std::end(values_modifiable), std::begin(preexisting_values), std::end(preexisting_values));
                writeHashedKeys(base_key_name, preexisting_values, allow_empty_values);
                return;
            }

            writeHashedKeys(base_key_name, values_modifiable, allow_empty_values);
            return;
        } else {
            throw std::invalid_argument(tr("Empty string provided while writing key or value to Google LevelDB database!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::writeMultipleKeys stores multiple values under the 'one' key within the Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param base_key_name The base-name of the key to start with.
 * @param values The values that are to be written under the 'one' key.
 * @param allow_empty_values If present, then whether to allow the insertion of empty values into the Google LevelDB
 * database with it's matching, strictly non-empty key.
 * @note miste...@googlemail.com <https://groups.google.com/g/leveldb/c/Bq30nafYSTY/m/NTbE0lqxEAYJ>.
 * @see GkLevelDb::readMultipleKeys().
 */
void GkLevelDb::writeMultipleKeys(const std::string &base_key_name, const std::string &value, const bool &allow_empty_values)
{
    try {
        if (!base_key_name.empty() && !value.empty()) {
            //
            // See if there are any values already pre-existing under this base-key! As we must prepend to these if
            // that's the case.
            //
            //
            // See if there are any values already pre-existing under this base-key! As we must prepend to these if
            // that's the case.
            //
            std::vector<std::string> values_modifiable;
            auto preexisting_values = readMultipleKeys(base_key_name);

            preexisting_values.emplace_back(value);
            if (!preexisting_values.empty()) {
                std::copy(preexisting_values.begin(), preexisting_values.end(), std::back_inserter(values_modifiable));
            }

            writeHashedKeys(base_key_name, preexisting_values, allow_empty_values);
        } else {
            throw std::invalid_argument(tr("Empty string provided while writing key or value to Google LevelDB database!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::deleteKeyFromMultiple deletes a singular value from under the 'one' key within the Google LevelDB
 * database, which is acting as a list for multiple values.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param base_key_name The values which are to be read out from the 'single' base-key.
 * @param removed_value The value to be removed from the list of read out, multiple values.
 * @return Whether the deletion operation was a success or not.
 */
bool GkLevelDb::deleteKeyFromMultiple(const std::string &base_key_name, const std::string &removed_value, const bool &allow_empty_values)
{
    try {
        if (!base_key_name.empty() && !removed_value.empty()) {
            auto modified_values = readMultipleKeys(base_key_name);
            auto it = modified_values.begin();
            while (it != modified_values.end()) {
                if (*it == removed_value) {
                    it = modified_values.erase(it);
                } else {
                    ++it;
                }
            }

            writeMultipleKeys(base_key_name, modified_values, allow_empty_values);
        } else {
            throw std::invalid_argument(tr("Empty string provided while modifying key or value with Google LevelDB database!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return false;
}

/**
 * @brief GkLevelDb::readMultipleKeys reads out the values which have been stored under the 'one' key within a Google
 * LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param base_key_name The values which are to be read out from the 'single' base-key.
 * @return The multiple values which have been stored under the 'one' base-key.
 * @note miste...@googlemail.com <https://groups.google.com/g/leveldb/c/Bq30nafYSTY/m/NTbE0lqxEAYJ>.
 * @see GkLevelDb::writeMultipleKeys().
 */
std::vector<std::string> GkLevelDb::readMultipleKeys(const std::string &base_key_name) const
{
    try {
        if (!base_key_name.empty()) {
            leveldb::Status status;
            leveldb::ReadOptions read_options;
            std::vector<std::string> values;

            read_options.verify_checksums = true;
            leveldb::Iterator *it = db->NewIterator(read_options);
            const std::string base_key_idx = std::string(base_key_name + "!");
            const std::string base_key_rnd = std::string(base_key_name + "!~");
            for (it->Seek(base_key_idx); it->Valid() && it->key().ToString() < base_key_rnd; it->Next()) {
                if (!it->value().empty()) {
                    values.emplace_back(it->value().ToString());
                }
            }

            return values;
        } else {
            throw std::invalid_argument(tr("Empty string provided while attempting to read key and/or value with Google LevelDB database!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return std::vector<std::string>();
}

/**
 * @brief GkLevelDb::write_rig_settings writes out the given settings desired by the user to a Google LevelDB database stored
 * on their own system's storage media, either via a default storage place or through specified means as configured by the
 * user themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The information, or rather, the data itself you wish to store towards the Google LevelDB database.
 * @param key The key which is required for _later retrieving_ the desired value(s) from the Google LevelDB database itself.
 */
void GkLevelDb::write_rig_settings(const QString &value, const Database::Settings::radio_cfg &key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        using namespace Database::Settings;
        switch (key) {
        case radio_cfg::RigBrand:
            batch.Put("RigBrand", value.toStdString());
            break;
        case radio_cfg::RigModel:
            batch.Put("RigModel", value.toStdString());
            break;
        case radio_cfg::RigModelIndex:
            batch.Put("RigModelIndex", value.toStdString());
            break;
        case radio_cfg::RigVersion:
            batch.Put("RigVersion", value.toStdString());
            break;
        case radio_cfg::CatConnType:
            batch.Put("CatConnType", value.toStdString());
            break;
        case radio_cfg::PttConnType:
            batch.Put("PttConnType", value.toStdString());
            break;
        case radio_cfg::StopBits:
            batch.Put("StopBits", value.toStdString());
            break;
        case radio_cfg::DataBits:
            batch.Put("DataBits", value.toStdString());
            break;
        case radio_cfg::Handshake:
            batch.Put("Handshake", value.toStdString());
            break;
        case radio_cfg::ForceCtrlLinesDtr:
            batch.Put("ForceCtrlLinesDtr", value.toStdString());
            break;
        case radio_cfg::ForceCtrlLinesRts:
            batch.Put("ForceCtrlLinesRts", value.toStdString());
            break;
        case radio_cfg::PTTMethod:
            batch.Put("PTTMethod", value.toStdString());
            break;
        case radio_cfg::TXAudioSrc:
            batch.Put("TXAudioSrc", value.toStdString());
            break;
        case radio_cfg::PTTMode:
            batch.Put("PTTMode", value.toStdString());
            break;
        case radio_cfg::SplitOperation:
            batch.Put("SplitOperation", value.toStdString());
            break;
        case radio_cfg::PTTAdvCmd:
            batch.Put("PTTAdvCmd", value.toStdString());
            break;
        case radio_cfg::RXAudioInitStart:
            batch.Put("RXAudioInitStart", value.toStdString());
            break;
        default:
            return;
        }

        std::time_t curr_time = std::time(nullptr);
        std::stringstream ss;
        ss << curr_time;
        batch.Put("CurrTime", ss.str());

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_rig_settings_comms writes out the given settings desired by the user to a Google LevelDB database stored
 * on their own system's storage media, either via a default storage place or through specified means as configured by the
 * user themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The information, or rather, the data itself you wish to store towards the Google LevelDB database.
 * @param key The key which is required for _later retrieving_ the desired value(s) from the Google LevelDB database itself.
 * @param conn_type The type of connection that is being used, whether it be USB, RS232, GPIO, etc.
 */
void GkLevelDb::write_rig_settings_comms(const QString &value, const radio_cfg &key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
            case radio_cfg::ComDeviceCat:
                batch.Put("ComDeviceCat", value.toStdString());
                break;
            case radio_cfg::ComDevicePtt:
                batch.Put("ComDevicePtt", value.toStdString());
                break;
            case radio_cfg::ComBaudRate:
                batch.Put("ComBaudRate", value.toStdString());
                break;
            case radio_cfg::ComDeviceCatPortType:
                batch.Put("ComDeviceCatPortType", value.toStdString());
                break;
            case radio_cfg::ComDevicePttPortType:
                batch.Put("ComDevicePttPortType", value.toStdString());
                break;
            default:
                return;
        }

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_general_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void GkLevelDb::write_general_settings(const QString &value, const general_stat_cfg &key)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
            case general_stat_cfg::defCqMsg:
                batch.Put("defCqMsg", value.toStdString());
                break;
            case general_stat_cfg::myCallsign:
                batch.Put("myCallsign", value.toStdString());
                break;
            case general_stat_cfg::defReplyMsg:
                batch.Put("defReplyMsg", value.toStdString());
                break;
            case general_stat_cfg::myMaidenhead:
                batch.Put("myMaidenhead", value.toStdString());
                break;
            case general_stat_cfg::defStationInfo:
                batch.Put("defStationInfo", value.toStdString());
                break;
            case general_stat_cfg::MsgAudioNotif:
                batch.Put("MsgAudioNotif", value.toStdString());
                break;
            case general_stat_cfg::FailAudioNotif:
                batch.Put("FailAudioNotif", value.toStdString());
                break;
            default:
                return;
        }

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DekodeDb::write_audio_device_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param is_output_device
 */
void GkLevelDb::write_audio_device_settings(const GkDevice &value, const bool &is_output_device)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        if (is_output_device) {
            // Unique identifier for the chosen output audio device
            batch.Put("AudioOutputDeviceName", value.audio_dev_str.toStdString());

            // Determine if this is the default output device for the system and if so, convert
            // the boolean value to a std::string suitable for database storage.
            std::string is_default = boolEnum(value.default_output_dev);
            batch.Put("AudioOutputDefSysDevice", is_default);

            // Modifier to let SWD know if this audio input device was configured as the result of
            // user activity and not by the part of default activity/settings.
            std::string user_activity = boolEnum(value.user_config_succ);
            batch.Put("AudioOutputCfgUsrActivity", user_activity);
        } else {
            // Unique identifier for the chosen input audio device
            batch.Put("AudioInputDeviceName", value.audio_dev_str.toStdString());

            // Determine if this is the default input device for the system and if so, convert
            // the boolean value to a std::string suitable for database storage.
            std::string is_default = boolEnum(value.default_input_dev);
            batch.Put("AudioInputDefSysDevice", is_default);

            // Modifier to let SWD know if this audio input device was configured as the result of
            // user activity and not by the part of default activity/settings.
            std::string user_activity = boolEnum(value.user_config_succ);
            batch.Put("AudioInputCfgUsrActivity", user_activity);
        }

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_audio_cfg_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void GkLevelDb::write_misc_audio_settings(const QString &value, const GkAudioCfg &key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
            case GkAudioCfg::settingsDbLoc:
                batch.Put("UserProfileDbLoc", value.toStdString());
                break;
            case GkAudioCfg::LogsDirLoc:
                batch.Put("UserLogsLoc", value.toStdString());
                break;
            case GkAudioCfg::AudioRecLoc:
                batch.Put("AudioRecSaveLoc", value.toStdString());
                break;
            case GkAudioCfg::AudioInputChannels:
                batch.Put("AudioInputChannels", value.toStdString());
                break;
            case GkAudioCfg::AudioOutputChannels:
                batch.Put("AudioOutputChannels", value.toStdString());
                break;
            case GkAudioCfg::AudioInputSampleRate:
                batch.Put("AudioInputSampleRate", value.toStdString());
                break;
            case GkAudioCfg::AudioOutputSampleRate:
                batch.Put("AudioOutputSampleRate", value.toStdString());
                break;
            case GkAudioCfg::AudioInputBitrate:
                batch.Put("AudioInputBitrate", value.toStdString());
                break;
            case GkAudioCfg::AudioOutputBitrate:
                batch.Put("AudioOutputBitrate", value.toStdString());
                break;
        }

        std::time_t curr_time = std::time(nullptr);
        std::stringstream ss;
        ss << curr_time;
        batch.Put("CurrTime", ss.str());

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_event_log_settings writes out settings relevant to the custom Event Logger for Small World Deluxe, such
 * as the verbosity level as desired by the end-user themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The value to the written.
 * @param key The key that is to be written towards.
 */
void GkLevelDb::write_event_log_settings(const QString &value, const GkEventLogCfg &key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
            case GkEventLogCfg::GkLogVerbosity:
                batch.Put("GkLogVerbosity", value.toStdString());
                break;
            default:
                throw std::runtime_error(tr("Invalid key has been provided for writing Event Logger settings relating to Google LevelDB!").toStdString());
        }

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_audio_playback_dlg_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void GkLevelDb::write_audio_playback_dlg_settings(const QString &value, const AudioPlaybackDlg &key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
            case AudioPlaybackDlg::GkAudioDlgLastFolderBrowsed:
                batch.Put("GkAudioDlgLastFolderBrowsed", value.toStdString());
                break;
            case AudioPlaybackDlg::GkRecordDlgLastFolderBrowsed:
                batch.Put("GkRecordDlgLastFolderBrowsed", value.toStdString());
                break;
            case AudioPlaybackDlg::GkRecordDlgLastCodecSelected:
                batch.Put("GkRecordDlgLastCodecSelected", value.toStdString());
                break;
            default:
                throw std::runtime_error(tr("Invalid key has been provided for writing Audio Playback settings relating to Google LevelDB!").toStdString());
        }

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_firewall_is_active_settings stores settings pertaining to the default firewall built into the
 * operating system (if there is one, such as the provided firewall provided with Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The setting to be stored.
 */
void GkLevelDb::write_firewall_is_active_settings(const std::string &value)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        batch.Put("GkIsFirewallActive", value);

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::write_firewall_port_settings stores network port settings (including whether TCP and/or UDP is
 * preferred) pertaining to the default firewall built into the operating system (if there is one, such as the provided
 * firewall provided with Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key The key to store the setting under.
 * @param network_ports The network port(s) to store within the Google LevelDB database, from 0 - 65536, and whether TCP
 * and/or UDP is preferred.
 */
void GkLevelDb::write_firewall_port_settings(const std::map<qint32, Network::GkNetworkProtocol> &network_ports)
{
    try {
        std::vector<std::string> ports;
        //
        // Insert a vector of network ports along with whether TCP and/or UDP is possibly preferred into the Google
        // LevelDB database!
        //
        for (const auto &port: network_ports) {
            ports.emplace_back(std::string(std::to_string(port.first) + "," + convNetworkProtocolEnumToStr(port.second).toStdString()));
        }

        writeMultipleKeys("GkFirewallPorts", ports, false);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::write_firewall_port_settings inserts a given, singular network port setting (including whether TCP
 * and/or UDP is preferred) pertaining to the default firewall built into the operating system (if there is one, such as
 * the provided firewall provided with Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param network_port
 */
void GkLevelDb::write_firewall_port_settings(const std::pair<qint32, Network::GkNetworkProtocol> &network_port)
{
    try {
        const std::vector<std::string> read_ports = readMultipleKeys("GkFirewallPorts");
        std::stringstream ss;
        for (const auto &port: read_ports) {
            if (!port.empty()) {
                ss << port << std::endl; // Compile all the CSV values into one, big block for easier decoding by a CSV interpreter...
            }
        }

        // TODO: Finish writing this!

        //
        // Insert a singular network port along with whether TCP and/or UDP is possibly preferred into the Google
        // LevelDB database!
        //
        // writeMultipleKeys("GkFirewallPorts", ports, false);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::delete_firewall_port_settings deletes a given network port from the stored settings, pertaining to
 * the default firewall built into the operating system (if there is one, such as the provided firewall provided with
 * Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param network_port The network port to delete within the Google LevelDB database, from 0 - 65536, along with whether
 * TCP and/or UDP was preferred.
 */
void GkLevelDb::delete_firewall_port_settings(const std::pair<qint32, Network::GkNetworkProtocol> &network_port)
{
    try {
        //
        // Delete the given port from the Google LevelDB database!
        deleteKeyFromMultiple("GkFirewallPorts", std::string(std::to_string(network_port.first) + "," + convNetworkProtocolEnumToStr(network_port.second).toStdString()), false);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_firewall_app_settings writes a given network enabled application from the stored settings,
 * pertaining to the default firewall built into the operating system (if there is one, such as the provided firewall
 * provided with Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param applications The network enabled application in question to write towards the Google LevelDB database, that has
 * already being enabled in the firewall that is default to the operating system (if there is one, such as the provided
 * firewall provided with Microsoft Windows).
 */
void GkLevelDb::write_firewall_app_settings(const std::vector<std::string> &applications)
{
    try {
        //
        // Insert a vector of network enabled, software applications to the Google LevelDB database!
        writeMultipleKeys("GkFirewallApps", applications, false);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::write_firewall_app_settings inserts a given, singular network enabled application from the stored
 * settings, pertaining to the default firewall built into the operating system (if there is one, such as the provided
 * firewall provided with Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param application
 */
void GkLevelDb::write_firewall_app_settings(const std::string &application)
{
    try {
        //
        // Insert a singular, network enabled software application into the Google LevelDB database!

        // TODO: Finish writing this!
        // writeMultipleKeys("GkFirewallApps", application, false);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::delete_firewall_app_settings deletes a given network enabled application from the stored settings,
 * pertaining to the default firewall built into the operating system (if there is one, such as the provided firewall
 * provided with Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param application The network enabled application in question to delete from the Google LevelDB database, that has
 * already being disabled within the firewall that is default to the operating system (if there is one, such as the
 * provided firewall provided with Microsoft Windows).
 */
void GkLevelDb::delete_firewall_app_settings(const std::string &application)
{
    try {
        //
        // Delete the given application from the Google LevelDB database!
        deleteKeyFromMultiple("GkFirewallApps", application, false);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_lang_ui_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param lang_key
 */
void GkLevelDb::write_lang_ui_settings(const QString &value, const Settings::Language::GkUiLang &lang_key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        switch (lang_key) {
            case ChosenUiLang:
                batch.Put("GkChosenUiLang", value.toStdString());
                break;
            default:
                throw std::runtime_error(tr("Invalid key has been provided for writing UI Language settings relating to Google LevelDB!").toStdString());
        }

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::write_lang_dict_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param dict_key
 */
void GkLevelDb::write_lang_dict_settings(const QString &value, const Settings::Language::GkDictionary &dict_key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        switch (dict_key) {
            case ChosenDictLang:
                batch.Put("GkChosenDictLang", value.toStdString());
                break;
            default:
                throw std::runtime_error(tr("Invalid key has been provided for writing Nuspell dictionary settings relating to Google LevelDB!").toStdString());
        }

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::read_lang_dict_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dict_key
 * @return
 */
QString GkLevelDb::read_lang_dict_settings(const Settings::Language::GkDictionary &dict_key)
{
    try {
        leveldb::Status status;
        leveldb::ReadOptions read_options;
        std::string value = "";

        read_options.verify_checksums = true;

        switch (dict_key) {
            case ChosenDictLang:
                status = db->Get(read_options, "GkChosenDictLang", &value);
                break;
            default:
                throw std::runtime_error(tr("Invalid key has been provided for writing Hunspell dictionary settings relating to Google LevelDB!").toStdString());
        }

        return QString::fromStdString(value);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QString();
}

/**
 * @brief GkLevelDb::read_lang_ui_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param lang_key
 * @return
 */
QString GkLevelDb::read_lang_ui_settings(const Settings::Language::GkUiLang &lang_key)
{
    try {
        leveldb::Status status;
        leveldb::ReadOptions read_options;
        std::string value = "";

        read_options.verify_checksums = true;

        switch (lang_key) {
            case ChosenUiLang:
                status = db->Get(read_options, "GkChosenUiLang", &value);
                break;
            default:
                throw std::runtime_error(tr("Invalid key has been provided for writing UI Language settings relating to Google LevelDB!").toStdString());
        }

        return QString::fromStdString(value);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QString();
}

/**
 * @brief GkLevelDb::write_frequencies_db manages the storage of frequency information within Small World Deluxe, such as
 * deletion, addition, and modification.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param write_new_value The newer frequency information values to be written to Google LevelDB.
 */
void GkLevelDb::write_frequencies_db(const GkFreqs &write_new_value)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;
        leveldb::ReadOptions read_options;
        read_options.verify_checksums = true;

        // Frequency
        std::string gk_stored_freq_value = "";
        db->Get(read_options, "GkStoredFreq", &gk_stored_freq_value);
        std::string csv_stored_freq;
        csv_stored_freq = processCsvToDB("GkStoredFreq", gk_stored_freq_value, QString::number(write_new_value.frequency).toStdString()); // Frequency (CSV)
        batch.Put("GkStoredFreq", csv_stored_freq);

        // Closest matching frequency band
        std::string gk_closest_band_value = "";
        db->Get(read_options, "GkClosestBand", &gk_closest_band_value);
        std::string csv_closest_band;
        csv_closest_band = processCsvToDB("GkClosestBand", gk_closest_band_value, convBandsToStr(write_new_value.closest_freq_band).toStdString()); // Closest matching frequency band (CSV)
        batch.Put("GkClosestBand", csv_closest_band);

        // Digital mode
        std::string gk_digital_mode_value = "";
        db->Get(read_options, "GkDigitalMode", &gk_digital_mode_value);
        std::string csv_digital_mode;
        csv_digital_mode = processCsvToDB("GkDigitalMode", gk_digital_mode_value, convDigitalModesToStr(write_new_value.digital_mode).toStdString()); // Digital mode (CSV)
        batch.Put("GkDigitalMode", csv_digital_mode);

        // IARU Region
        std::string gk_iaru_region_value = "";
        db->Get(read_options, "GkIARURegion", &gk_iaru_region_value);
        std::string csv_iaru_region;
        csv_iaru_region = processCsvToDB("GkIARURegion", gk_iaru_region_value, convIARURegionToStr(write_new_value.iaru_region).toStdString()); // IARU Region (CSV)
        batch.Put("GkIARURegion", csv_iaru_region);

        remove_frequencies_db(true); // Delete all the frequencies and their component values!

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }

        return;
    } catch (const std::exception &e) { // https://en.cppreference.com/w/cpp/error/nested_exception
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::remove_frequencies_db will remove a given frequencies and all of its component values from the Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_to_remove The given frequency and its component values to remove.
 * @param del_all Whether to delete all frequencies and their component values or not.
 */
void GkLevelDb::remove_frequencies_db(const GkFreqs &freq_to_remove)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;
        leveldb::ReadOptions read_options;
        read_options.verify_checksums = true;

        //
        // Only delete the given frequency and its component values
        //

        // Frequency
        std::string gk_stored_freq_value = "";
        status = db->Get(read_options, "GkStoredFreq", &gk_stored_freq_value);
        auto gk_freq_elements = gkStringFuncs->csvSplitter(gk_stored_freq_value);
        auto gk_freq_elements_mod = gkStringFuncs->csvRemoveElement(gk_freq_elements, std::to_string(freq_to_remove.frequency));
        std::string csv_stored_freq = gkStringFuncs->csvOutputString(gk_freq_elements_mod);

        batch.Delete("GkStoredFreq"); // Delete the key before rewriting the values again!
        batch.Put("GkStoredFreq", csv_stored_freq);

        // Closest matching frequency band
        std::string gk_closest_band_value = "";
        status = db->Get(read_options, "GkClosestBand", &gk_closest_band_value);
        auto gk_closed_band_elements = gkStringFuncs->csvSplitter(gk_closest_band_value);
        auto gk_closed_band_elements_mod = gkStringFuncs->csvRemoveElement(gk_closed_band_elements, convBandsToStr(freq_to_remove.closest_freq_band).toStdString());
        std::string csv_closest_band = gkStringFuncs->csvOutputString(gk_closed_band_elements_mod);

        batch.Delete("GkClosestBand"); // Delete the key before rewriting the values again!
        batch.Put("GkClosestBand", csv_closest_band);

        // Digital mode
        std::string gk_digital_mode_value = "";
        status = db->Get(read_options, "GkDigitalMode", &gk_digital_mode_value);
        auto gk_digital_mode_elements = gkStringFuncs->csvSplitter(gk_digital_mode_value);
        auto gk_digital_mode_elements_mod = gkStringFuncs->csvRemoveElement(gk_digital_mode_elements, convDigitalModesToStr(freq_to_remove.digital_mode).toStdString());
        std::string csv_digital_mode = gkStringFuncs->csvOutputString(gk_digital_mode_elements_mod);

        batch.Delete("GkDigitalMode"); // Delete the key before rewriting the values again!
        batch.Put("GkDigitalMode", csv_digital_mode);

        // IARU Region
        std::string gk_iaru_region_value = "";
        status = db->Get(read_options, "GkIARURegion", &gk_iaru_region_value);
        auto gk_iaru_region_elements = gkStringFuncs->csvSplitter(gk_iaru_region_value);
        auto gk_iaru_region_elements_mod = gkStringFuncs->csvRemoveElement(gk_iaru_region_elements, convIARURegionToStr(freq_to_remove.iaru_region).toStdString());
        std::string csv_iaru_region = gkStringFuncs->csvOutputString(gk_iaru_region_elements_mod);

        batch.Delete("GkIARURegion"); // Delete the key before rewriting the values again!
        batch.Put("GkIARURegion", csv_iaru_region);

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) { // https://en.cppreference.com/w/cpp/error/nested_exception
        gkStringFuncs->print_exception(e);
    }

    return;
}

/**
 * @brief GkLevelDb::remove_frequencies_db deletes all the frequencies that are stored within the program's Google LevelDB database along with
 * their component values.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param del_all Whether to delete all the frequencies and their component values or not.
 */
void GkLevelDb::remove_frequencies_db(const bool &del_all)
{
    try {
        if (del_all) {
            //
            // Delete all the frequencies and their component values!
            //

            leveldb::WriteBatch batch;
            leveldb::Status status;

            // Frequency
            batch.Delete("GkStoredFreq");

            // Closest matching frequency band
            batch.Delete("GkClosestBand");

            // Digital mode
            batch.Delete("GkDigitalMode");

            // IARU Region
            batch.Delete("GkIARURegion");

            leveldb::WriteOptions write_options;
            write_options.sync = true;

            status = db->Write(write_options, &batch);

            if (!status.ok()) { // Abort because of error!
                throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
            }
        }
    } catch (const std::exception &e) { // https://en.cppreference.com/w/cpp/error/nested_exception
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::writeFreqInit
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkLevelDb::writeFreqInit()
{
    bool already_init = isFreqAlreadyInit();

    if (!already_init) {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        batch.Put("GkFreqInit", boolEnum(true));

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    }

    return;
}

/**
 * @brief GkLevelDb::isFreqAlreadyInit determines whether the Google LevelDB database has been primed with the base frequency set and
 * their component values or not. This is kind'a required for the basic functioning of Small World Deluxe, at least and until the user
 * starts making their own customizations to the software application itself.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return True or false regarding whether Google LevelDB has been primed or not.
 */
bool GkLevelDb::isFreqAlreadyInit()
{
    try {
        leveldb::Status status;
        leveldb::ReadOptions read_options;
        std::string freq_init_str = "";
        bool freq_init_bool = false;

        std::lock_guard<std::mutex> lck_guard(mtx_freq_already_init);
        read_options.verify_checksums = true;

        status = db->Get(read_options, "GkFreqInit", &freq_init_str);

        if (!status.ok()) { // Abort because of error!
            std::cout << tr("Frequencies have not yet been initialized for this user!").toStdString() << std::endl;
            return false;
        }

        if (!freq_init_str.empty()) {
            // Convert to a boolean value
            freq_init_bool = boolStr(freq_init_str);
        }

        return freq_init_bool;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return false;
}

/**
 * @brief GkLevelDb::write_sentry_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void GkLevelDb::write_sentry_settings(const bool &value, const GkSentry &key)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
        case GkSentry::AskedDialog:
            batch.Put("AskedDialog", boolEnum(value));
            break;
        case GkSentry::GivenConsent:
            batch.Put("GivenConsent", boolEnum(value));
            break;
        default:
            return;
        }

        std::time_t curr_time = std::time(nullptr);
        std::stringstream ss;
        ss << curr_time;
        batch.Put("CurrTime", ss.str());

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_optin_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void GkLevelDb::write_optin_settings(const QString &value, const GkOptIn &key)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
        case GkOptIn::UserUniqueId:
            batch.Put("UserUniqueId", value.toStdString());
            break;
        default:
            break;
        }

        std::time_t curr_time = std::time(nullptr);
        std::stringstream ss;
        ss << curr_time;
        batch.Put("CurrTime", ss.str());

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::read_sentry_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key
 * @return
 */
bool GkLevelDb::read_sentry_settings(const GkSentry &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
    case GkSentry::AskedDialog:
        status = db->Get(read_options, "AskedDialog", &value);
        break;
    case GkSentry::GivenConsent:
        status = db->Get(read_options, "GivenConsent", &value);
        break;
    default:
        break;
    }

    if (!value.empty()) {
        return boolStr(value);
    }

    return false;
}

/**
 * @brief GkLevelDb::read_optin_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key
 * @return
 */
QString GkLevelDb::read_optin_settings(const GkOptIn &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
    case GkOptIn::UserUniqueId:
        status = db->Get(read_options, "UserUniqueId", &value);
        break;
    default:
        break;
    }

    return QString::fromStdString(value);
}

/**
 * @brief GkLevelDb::capture_sys_info will not only create a Unique ID as pertaining to the user on the given, local machine for tracking of bug-reports
 * without having to rely on IP addresses, but will also capture some unique information pertaining to said local machine that will be useful for what
 * direction to take with the development of Small World Deluxe. This includes such things as screen resolution, operating system used along with
 * versioning (i.e. of the kernel and distro as well if applicable), along with a few other bits and bobs while ensuring anonymity of the user in question at
 * all times. Consent has to be given by the user upon initial program launch, of course, to send any of this information.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkLevelDb::capture_sys_info()
{
    try {
        //
        // https://docs.sentry.io/enriching-error-data/additional-data/?platform=native
        // https://docs.sentry.io/platforms/native/#capturing-events
        //
        sentry_set_level(SENTRY_LEVEL_INFO); // We are only sending informational data!
        sentry_value_t info_capture_event = sentry_value_new_event();

        std::string ret_str = "";
        QString sentry_unique_id = read_optin_settings(GkOptIn::UserUniqueId);
        if (!sentry_unique_id.isEmpty()) {
            const std::string ret_str = sentry_unique_id.toStdString(); //-V808
        } else {
            const std::string ret_str = createRandomString(24);
            write_optin_settings(QString::fromStdString(ret_str), GkOptIn::UserUniqueId);
        }

        //
        // Create a new event whereby a unique user-identity is determined!
        //
        sentry_value_t user = sentry_value_new_object();
        sentry_value_set_by_key(user, "id", sentry_value_new_string(ret_str.c_str()));

        QString callsign = read_general_settings(general_stat_cfg::myCallsign);
        QString maidenhead = read_general_settings(general_stat_cfg::myMaidenhead);

        if (!callsign.isEmpty()) {
            sentry_value_set_by_key(user, "username", sentry_value_new_string(callsign.toStdString().c_str()));
        } else {
            sentry_value_set_by_key(user, "username", sentry_value_new_string("Unknown"));
        }

        if (!maidenhead.isEmpty()) {
            sentry_value_set_by_key(user, "maidenhead", sentry_value_new_string(maidenhead.toStdString().c_str()));
        } else {
            sentry_value_set_by_key(user, "maidenhead", sentry_value_new_string("JJ00aa")); // Null value for the Maidenhead grid system!
        }

        sentry_set_user(user);

        //
        // Create a new event whereby the operating system type and version is determined!
        //
        const QString def_val = "Unknown";
        QString build_cpu_arch = def_val;
        QString curr_cpu_arch = def_val;
        QString kernel_type = def_val;
        QString kernel_vers = def_val;
        QString machine_host_name = def_val;
        QString machine_unique_id = def_val;
        QString pretty_prod_name = def_val;
        QString prod_type = def_val;
        QString prod_vers = def_val;

        sentry_value_t os_obj = sentry_value_new_object();
        sentry_value_t device_obj = sentry_value_new_object();
        sentry_value_t kernel_obj = sentry_value_new_object();
        detect_operating_system(build_cpu_arch, curr_cpu_arch, kernel_type, kernel_vers, machine_host_name, machine_unique_id,
                                pretty_prod_name, prod_type, prod_vers);

        if (!build_cpu_arch.isEmpty()) {
            // CPU Architecture used for the building of this particular version of Small World Deluxe application
            sentry_value_set_by_key(device_obj, "build_cpu_arch", sentry_value_new_string(build_cpu_arch.toStdString().c_str()));
        }

        if (!curr_cpu_arch.isEmpty()) {
            // CPU Architecture of the host operating system
            sentry_value_set_by_key(device_obj, "curr_cpu_arch", sentry_value_new_string(curr_cpu_arch.toStdString().c_str()));
            sentry_set_tag("cpu_arch", curr_cpu_arch.toStdString().c_str());
        }

        if (!kernel_type.isEmpty()) {
            // Kernel type
            sentry_value_set_by_key(kernel_obj, "type", sentry_value_new_string(kernel_type.toStdString().c_str()));
        }

        if (!kernel_vers.isEmpty()) {
            // Kernel version
            sentry_value_set_by_key(kernel_obj, "version", sentry_value_new_string(kernel_vers.toStdString().c_str()));
        }

        if (!machine_host_name.isEmpty()) {
            // Machine host name
            sentry_value_set_by_key(device_obj, "hostname", sentry_value_new_string(machine_host_name.toStdString().c_str()));
        }

        if (!machine_unique_id.isEmpty()) {
            // Machine unique ID
            sentry_value_set_by_key(device_obj, "unique_id", sentry_value_new_string(machine_unique_id.toStdString().c_str()));
        }

        if (!pretty_prod_name.isEmpty()) {
            // Operating System name (pretty version)
            sentry_value_set_by_key(os_obj, "name", sentry_value_new_string(pretty_prod_name.toStdString().c_str()));
        }

        if (!prod_type.isEmpty()) {
            // Operating System type
            sentry_value_set_by_key(os_obj, "type", sentry_value_new_string(prod_type.toStdString().c_str()));
            sentry_set_tag("os_type", prod_type.toStdString().c_str());
        }

        if (!prod_vers.isEmpty()) {
            // Operating System version
            sentry_value_set_by_key(os_obj, "version", sentry_value_new_string(prod_vers.toStdString().c_str()));
            sentry_set_tag("os_version", prod_vers.toStdString().c_str());
        }

        //
        // Kernel Information
        // https://docs.sentry.io/enriching-error-data/additional-data/?platform=native#tags
        //
        sentry_set_extra("kernel", kernel_obj);

        //
        // Device Information
        // https://docs.sentry.io/enriching-error-data/additional-data/?platform=native#tags
        //
        sentry_set_extra("device", device_obj);

        //
        // Operating System
        // https://docs.sentry.io/enriching-error-data/additional-data/?platform=native#tags
        //
        sentry_set_extra("os", os_obj);

        //
        // Grab the screen resolution of the user's desktop and create a new object for such!
        //
        const qreal width = gkMainWinGeometry.width();
        const qreal height = gkMainWinGeometry.height();

        sentry_value_t screen_res_obj = sentry_value_new_object();
        sentry_value_set_by_key(screen_res_obj, "width", sentry_value_new_double(width));
        sentry_value_set_by_key(screen_res_obj, "height", sentry_value_new_double(height));

        //
        // Screen Size information
        //
        sentry_set_extra("screen_size", screen_res_obj);

        //
        // Send the captured information to the GekkoFyre Networks' Sentry server!
        // https://docs.sentry.io/platforms/native/#capturing-events
        //
        sentry_capture_event(info_capture_event);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue was encountered with the crash-reporting subsystem; please report this to the developers. Error:\n\n%1")
        .arg(QString::fromStdString(e.what())), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_xmpp_chat_log writes the user's chat history to the Google LevelDB database, thereby removing
 * the need to download unnecessary data from a given XMPP server over the Internet.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The username associated with the archived message history in question.
 * @param messages The message history to be archived and recorded to the Google LevelDB database.
 */
void GkLevelDb::write_xmpp_chat_log(const QString &bareJid, const QList<QXmppMessage> &messages)
{
    try {
        if (!bareJid.isEmpty() && !messages.isEmpty()) {
            const std::string msg_key = QString("%1_%2").arg(bareJid).arg(General::Xmpp::GoogleLevelDb::keyToConvMsgHistory).toStdString();
            const std::string timestamp_key = QString("%1_%2").arg(bareJid).arg(General::Xmpp::GoogleLevelDb::keyToConvTimestampHistory).toStdString();
            const auto exist_msg_history = readMultipleKeys(msg_key);
            const auto exist_timestamp_history = readMultipleKeys(timestamp_key);
            std::vector<std::string> msg_body_str;
            std::vector<std::string> msg_stamp_str;

            if (!exist_msg_history.empty() && !exist_timestamp_history.empty()) {
                if (exist_msg_history.size() != exist_timestamp_history.size()) {
                    throw std::runtime_error(tr("An error has occurred with the processing of your XMPP message history!").toStdString());
                }

                //
                // We need to add the timestamp information towards, `exist_msg_history`, so that a proper comparison of its
                // variables can be made with the function, `GkLevelDb::update_xmpp_chat_log()`!
                //
                QList<QXmppMessage> recordedMsgHistory;
                for (size_t i = 0; i < exist_msg_history.size(); ++i) {
                    QXmppMessage message;
                    message.setBody(QString::fromStdString(exist_msg_history.at(i)));
                    message.setStamp(QDateTime::fromString(QString::fromStdString(exist_timestamp_history.at(i))));
                    recordedMsgHistory.insert(static_cast<qint32>(i), message);
                }

                if (!recordedMsgHistory.isEmpty()) {
                    //
                    // Process the message history for any extraneous (i.e. non-unique) data!
                    const auto proc_msg_history = update_xmpp_chat_log(bareJid, messages, recordedMsgHistory);
                    if (!proc_msg_history.isEmpty()) {
                        for (const auto &msg: proc_msg_history) {
                            if (msg.isXmppStanza() && (!msg.body().isEmpty() && msg.stamp().isValid())) {
                                msg_body_str.emplace_back(msg.body().toStdString());
                                msg_stamp_str.emplace_back(msg.stamp().toString().toStdString());
                            }
                        }

                        if (!msg_body_str.empty() && !msg_stamp_str.empty()) {
                            if (msg_body_str.size() != msg_stamp_str.size()) {
                                throw std::runtime_error(
                                        tr("An error has occurred with the processing of your XMPP message history!").toStdString());
                            }

                            writeMultipleKeys(msg_key, msg_body_str, false);
                            writeMultipleKeys(timestamp_key, msg_stamp_str, false);
                            return;
                        }
                    }
                }
            }

            for (const auto &message: messages) {
                if (message.isXmppStanza() && (!message.body().isEmpty() && message.stamp().isValid())) {
                    msg_body_str.emplace_back(message.body().toStdString());
                    msg_stamp_str.emplace_back(message.stamp().toString().toStdString());
                }
            }

            if (!msg_body_str.empty() && !msg_stamp_str.empty()) {
                if (msg_body_str.size() != msg_stamp_str.size()) {
                    throw std::runtime_error(tr("An error has occurred with the processing of your XMPP message history!").toStdString());
                }

                writeMultipleKeys(msg_key, msg_body_str, false);
                writeMultipleKeys(timestamp_key, msg_stamp_str, false);
                return;
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::update_xmpp_chat_log takes in pre-existing message histories and only keeps the information that is
 * truly needed, removing all the extraneous information in the process.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid The username associated with the archived message history in question.
 * @param original_msgs The message history that is to be processed.
 * @param comparison_msgs The message history that is to be compared against.
 * @return The processed message history, with all 'doubled-up' information removed.
 */
QList<QXmppMessage> GkLevelDb::update_xmpp_chat_log(const QString &bareJid, const QList<QXmppMessage> &original_msgs,
                                                    const QList<QXmppMessage> &comparison_msgs) const
{
    try {
        if (!original_msgs.isEmpty() && !comparison_msgs.isEmpty()) {
            QList<QXmppMessage> tmp_messages;
            std::copy(original_msgs.begin(), original_msgs.end(), std::back_inserter(tmp_messages));
            if (!tmp_messages.isEmpty()) {
                for (const auto &msg: comparison_msgs) {
                    for (auto iter = tmp_messages.begin(); iter != tmp_messages.end(); ++iter) {
                        if (iter->stamp() == msg.stamp()) {
                            iter = tmp_messages.erase(iter);
                            break;
                        }
                    }
                }

                return tmp_messages;
            }

            throw std::runtime_error(tr("An error has occurred whilst processing message history!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QList<QXmppMessage>();
}

/**
 * @brief GkLevelDb::write_xmpp_settings writes out and saves given XMPP settings to a pre-configured Google LevelDB
 * database for future reading and processing.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The actual configuration value to be saved.
 * @param key The setting in question that the value is to be saved under.
 */
void GkLevelDb::write_xmpp_settings(const QString &value, const Settings::GkXmppCfg &key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
            case Settings::GkXmppCfg::XmppAllowMsgHistory:
                batch.Put("XmppCfgAllowMsgHistory", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppAllowFileXfers:
                batch.Put("XmppCfgAllowFileXfers", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppAllowMucs:
                batch.Put("XmppCfgAllowMucs", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppAutoConnect:
                batch.Put("XmppCfgAutoConnect", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppAutoReconnect:
                batch.Put("XmppAutoReconnect", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppAutoReconnectIgnore:
                batch.Put("XmppAutoReconnectIgnore", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppUriLookupMethod:
                batch.Put("XmppUriLookupMethod", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppAvatarByteArray:
                batch.Put("XmppCfgAvatarByteArray", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppDomainUrl:
                batch.Put("XmppCfgDomainUrl", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppServerType:
                batch.Put("XmppCfgServerType", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppDomainPort:
                batch.Put("XmppCfgDomainPort", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppEnableSsl:
                batch.Put("XmppCfgEnableSsl", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppIgnoreSslErrors:
                batch.Put("XmppIgnoreSslErrors", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppUsername:
                batch.Put("XmppUsername", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppJid:
                batch.Put("XmppJid", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppPassword:
                batch.Put("XmppPassword", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppNickname:
                batch.Put("XmppNickname", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppEmailAddr:
                batch.Put("XmppEmailAddr", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppNetworkTimeout:
                batch.Put("XmppNetworkTimeout", value.toStdString());
                break;
            case Settings::GkXmppCfg::XmppLastOnlinePresence:
                batch.Put("XmppLastOnlinePresence", value.toStdString());
                break;
            default:
                return;
        }

        std::time_t curr_time = std::time(nullptr);
        std::stringstream ss;
        ss << curr_time;
        batch.Put("CurrTime", ss.str());

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_xmpp_vcard_data stores the information of users as outputted by the QXmppRosterManager upon
 * a successful connection having been made to a given XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param vcard_roster The details pertaining to all the vCards within the XMPP roster. Of the values, the first is the
 * XML Stream for the vCard(s) themselves and the second of the pair is the QByteArray for the avatar(s), if present. This
 * leaves the key, which is simply the JID and all of its pertaining information.
 * @note example_9_vCard <https://github.com/qxmpp-project/qxmpp/tree/master/examples/example_9_vCard>.
 * @see GkXmppClient::handlevCardReceived(), GkXmppClient::handleRosterReceived().
 */
void GkLevelDb::write_xmpp_vcard_data(const QMap<QString, std::pair<QByteArray, QByteArray>> &vcard_roster)
{
    try {
        if (!vcard_roster.isEmpty()) {
            auto stored_jid = readMultipleKeys(General::Xmpp::GoogleLevelDb::jidLookupKey);
            for (const auto &vcard: vcard_roster.toStdMap()) {
                if (!vcard.first.isEmpty()) {
                    for (const auto &existing_jid: stored_jid) {
                        if (vcard.first.toStdString() == existing_jid) {
                            //
                            // JID is already stored within Google LevelDB!
                            continue;
                        } else {
                            //
                            // We are dealing with a new JID entry!
                            writeMultipleKeys(General::Xmpp::GoogleLevelDb::jidLookupKey, vcard.first.toStdString(), false);
                        }

                        QString xml_stream = QTextCodec::codecForMib(1015)->toUnicode(vcard.second.first); // NOTE: An encoding other than `1015` might be needed!
                        QString img_stream = QTextCodec::codecForMib(1015)->toUnicode(vcard.second.second); // NOTE: An encoding other than `1015` might be needed!

                        writeMultipleKeys(convXmppVcardKey(vcard.first, GkVcardKeyConv::XmlStream).toStdString(), xml_stream.toStdString(), true);
                        writeMultipleKeys(convXmppVcardKey(vcard.first, GkVcardKeyConv::AvatarImg).toStdString(), img_stream.toStdString(), true);
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_xmpp_vcard_data removes the information of users as updated by the QXmppRosterManager upon
 * a successful connection having been made to a given XMPP server.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param vcard_roster The details pertaining to all the vCards within the XMPP roster. Of the values, the first is the
 * XML Stream for the vCard(s) themselves and the second of the pair is the QByteArray for the avatar(s), if present. This
 * leaves the key, which is simply the JID and all of its pertaining information.
 * @note example_9_vCard <https://github.com/qxmpp-project/qxmpp/tree/master/examples/example_9_vCard>.
 * @see GkXmppClient::handlevCardReceived(), GkXmppClient::handleRosterReceived().
 */
void GkLevelDb::remove_xmpp_vcard_data(const QMap<QString, std::pair<QByteArray, QByteArray>> &vcard_roster)
{
    try {
        if (!vcard_roster.isEmpty()) {
            auto stored_jid = readMultipleKeys(General::Xmpp::GoogleLevelDb::jidLookupKey);
            for (const auto &vcard: vcard_roster.toStdMap()) {
                if (!vcard.first.isEmpty()) {
                    for (const auto &existing_jid: stored_jid) {
                        if (vcard.first.toStdString() == existing_jid) {
                            //
                            // JID is already stored within Google LevelDB!

                            //
                            // Start by deleting the value from the Google LevelDB database...
                            QString xml_stream = QTextCodec::codecForMib(1015)->toUnicode(vcard.second.first);
                            QString img_stream = QTextCodec::codecForMib(1015)->toUnicode(vcard.second.second); // NOTE: An encoding other than `1015` might be needed!
                            deleteKeyFromMultiple(convXmppVcardKey(vcard.first, GkVcardKeyConv::XmlStream).toStdString(), xml_stream.toStdString(), true);
                            deleteKeyFromMultiple(convXmppVcardKey(vcard.first, GkVcardKeyConv::AvatarImg).toStdString(), img_stream.toStdString(), true);

                            //
                            // Now it's time to delete the key itself!
                            deleteKeyFromMultiple(General::Xmpp::GoogleLevelDb::jidLookupKey, vcard.first.toStdString(), false);
                        } else {
                            //
                            // We are dealing with a new entry, accidentally! We better add this to the database...
                        }
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::write_xmpp_alpha_notice
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 */
void GkLevelDb::write_xmpp_alpha_notice(const bool &value)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        batch.Put("XmppAlphaMsgBoxNotice", boolEnum(value));

        std::time_t curr_time = std::time(nullptr);
        std::stringstream ss;
        ss << curr_time;
        batch.Put("CurrTime", ss.str());

        leveldb::WriteOptions write_options;
        write_options.sync = true;

        status = db->Write(write_options, &batch);

        if (!status.ok()) { // Abort because of error!
            throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::read_xmpp_chat_log
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bareJid
 * @return
 */
QList<QXmppMessage> GkLevelDb::read_xmpp_chat_log(const QString &bareJid) const
{
    try {
        const std::string msg_key = QString("%1_%2").arg(bareJid).arg(General::Xmpp::GoogleLevelDb::keyToConvMsgHistory).toStdString();
        const std::string timestamp_key = QString("%1_%2").arg(bareJid).arg(General::Xmpp::GoogleLevelDb::keyToConvTimestampHistory).toStdString();
        QList<QXmppMessage> messages;
        const auto exist_msg_history = readMultipleKeys(msg_key);
        const auto exist_timestamp_history = readMultipleKeys(timestamp_key);
        if (!exist_msg_history.empty() && !exist_timestamp_history.empty()) {
            if (exist_msg_history.size() == exist_timestamp_history.size()) {
                for (size_t i = 0; i < exist_msg_history.size(); ++i) {
                    QXmppMessage message;
                    message.setStamp(QDateTime::fromString(QString::fromStdString(exist_timestamp_history.at(i))));
                    message.setBody(QString::fromStdString(exist_msg_history.at(i)));
                    messages.insert(i, message);
                }

                return messages;
            }

            throw std::runtime_error(tr("An error has occurred whilst reading recorded message history!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QList<QXmppMessage>();
}

/**
 * @brief GkLevelDb::read_xmpp_settings reads out the previously saved XMPP settings from the given Google LevelDB
 * database for further processing.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key The setting in question that the value we are trying to retrieve is saved under.
 * @return Our previously saved value from the pre-configured Google LevelDB database.
 */
QString GkLevelDb::read_xmpp_settings(const Settings::GkXmppCfg &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
        case Settings::GkXmppCfg::XmppAllowMsgHistory:
            status = db->Get(read_options, "XmppCfgAllowMsgHistory", &value);
            break;
        case Settings::GkXmppCfg::XmppAllowFileXfers:
            status = db->Get(read_options, "XmppCfgAllowFileXfers", &value);
            break;
        case Settings::GkXmppCfg::XmppAllowMucs:
            status = db->Get(read_options, "XmppCfgAllowMucs", &value);
            break;
        case Settings::GkXmppCfg::XmppAutoConnect:
            status = db->Get(read_options, "XmppCfgAutoConnect", &value);
            break;
        case Settings::GkXmppCfg::XmppAutoReconnect:
            status = db->Get(read_options, "XmppAutoReconnect", &value);
            break;
        case Settings::GkXmppCfg::XmppAutoReconnectIgnore:
            status = db->Get(read_options, "XmppAutoReconnectIgnore", &value);
            break;
        case Settings::GkXmppCfg::XmppUriLookupMethod:
            status = db->Get(read_options, "XmppUriLookupMethod", &value);
            break;
        case Settings::GkXmppCfg::XmppAvatarByteArray:
            status = db->Get(read_options, "XmppCfgAvatarByteArray", &value);
            break;
        case Settings::GkXmppCfg::XmppDomainUrl:
            status = db->Get(read_options, "XmppCfgDomainUrl", &value);
            break;
        case Settings::GkXmppCfg::XmppServerType:
            status = db->Get(read_options, "XmppCfgServerType", &value);
            break;
        case Settings::GkXmppCfg::XmppDomainPort:
            status = db->Get(read_options, "XmppCfgDomainPort", &value);
            break;
        case Settings::GkXmppCfg::XmppEnableSsl:
            status = db->Get(read_options, "XmppCfgEnableSsl", &value);
            break;
        case Settings::GkXmppCfg::XmppIgnoreSslErrors:
            status = db->Get(read_options, "XmppIgnoreSslErrors", &value);
            break;
        case Settings::GkXmppCfg::XmppUsername:
            status = db->Get(read_options, "XmppUsername", &value);
            break;
        case Settings::GkXmppCfg::XmppJid:
            status = db->Get(read_options, "XmppJid", &value);
            break;
        case Settings::GkXmppCfg::XmppPassword:
            status = db->Get(read_options, "XmppPassword", &value);
            break;
        case Settings::GkXmppCfg::XmppNickname:
            status = db->Get(read_options, "XmppNickname", &value);
            break;
        case Settings::GkXmppCfg::XmppEmailAddr:
            status = db->Get(read_options, "XmppEmailAddr", &value);
            break;
        case Settings::GkXmppCfg::XmppNetworkTimeout:
            status = db->Get(read_options, "XmppNetworkTimeout", &value);
            break;
        case Settings::GkXmppCfg::XmppLastOnlinePresence:
            status = db->Get(read_options, "XmppLastOnlinePresence", &value);
            break;
        default:
            return QString();
    }

    return QString::fromStdString(value);
}

/**
 * @brief GkLevelDb::read_xmpp_alpha_notice
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
bool GkLevelDb::read_xmpp_alpha_notice() {
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    status = db->Get(read_options, "XmppAlphaMsgBoxNotice", &value);
    return boolStr(value);
}

/**
 * @brief GkLevelDb::convXmppServerTypeFromInt
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkXmpp::GkServerType GkLevelDb::convXmppServerTypeFromInt(const qint32 &idx)
{
    switch (idx) {
        case GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX:
            return GkXmpp::GkServerType::GekkoFyre;
        case GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX:
            return GkXmpp::GkServerType::Custom;
        default:
            return GkXmpp::GkServerType::Unknown;
    }

    return GkXmpp::GkServerType::Unknown;
}

/**
 * @brief GkLevelDb::convNetworkProtocolEnumToStr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param network_protocol
 * @return
 */
QString GkLevelDb::convNetworkProtocolEnumToStr(const GkNetworkProtocol &network_protocol)
{
    switch (network_protocol) {
        case GkNetworkProtocol::TCP:
            return "TCP";
        case UDP:
            return "UDP";
        case Any:
            return "Any";
        default:
            break;
    }

    return QString();
}

/**
 * @brief GkLevelDb::writeHashedKeys
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param base_key_name The base-name of the key to start with.
 * @param values The values that are to be written under the 'one' key.
 * @param allow_empty_values If present, then whether to allow the insertion of empty values into the Google LevelDB
 * @see GkLevelDb::writeMultipleKeys().
 */
void GkLevelDb::writeHashedKeys(const std::string &base_key_name, const std::vector<std::string> &values,
                                const bool &allow_empty_values)
{
    try {
        if (!base_key_name.empty() && !values.empty()) {
            leveldb::WriteBatch batch;
            leveldb::Status status;
            std::vector<std::tuple<std::string, std::string>> key_value_map;

            qint64 curr_unix_epoch = QDateTime::currentMSecsSinceEpoch();
            for (const auto &value: values) {
                if (!value.empty()) {
                    const std::string new_key_name = std::string(base_key_name + '!' + std::to_string(curr_unix_epoch));
                    key_value_map.emplace_back(std::make_pair(new_key_name, value));
                    ++curr_unix_epoch;
                }
            }

            if (!key_value_map.empty()) {
                for (const auto &key_value: key_value_map) {
                    if (!std::get<0>(key_value).empty()) {
                        if (!std::get<1>(key_value).empty() || (allow_empty_values && std::get<1>(key_value).empty())) {
                            batch.Put(std::get<0>(key_value), std::get<1>(key_value));
                        }
                    }
                }

                leveldb::WriteOptions write_options;
                write_options.sync = true;

                status = db->Write(write_options, &batch);

                if (!status.ok()) { // Abort because of error!
                    throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
                }
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief GkLevelDb::convXmppVcardKey converts a 'base-key' to something more manageable for use with GkLevelDb::writeMultipleKeys() so
 * that multiple values maybe stored with the 'one' key instead.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param keyToConv
 * @param method
 * @return
 * @see GkLevelDb::writeMultipleKeys(), GkLevelDb::write_xmpp_vcard_data().
 */
QString GkLevelDb::convXmppVcardKey(const QString &keyToConv, const GkXmpp::GkVcardKeyConv &method)
{
    QString ret_key;
    switch (method) {
        case GkXmpp::GkVcardKeyConv::XmlStream:
            ret_key = QString(keyToConv + General::Xmpp::GoogleLevelDb::keyToConvXmlStream);
            break;
        case GkXmpp::GkVcardKeyConv::AvatarImg:
            ret_key = QString(keyToConv + General::Xmpp::GoogleLevelDb::keyToConvAvatarImg);
            break;
        default:
            break;
    }

    return ret_key;
}

/**
 * @brief GkLevelDb::detect_operating_system will detect the operating system and hopefully, version, used by the user of this
 * application, mostly for purposes needed by Sentry.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param build_cpu_arch
 * @param curr_cpu_arch
 * @param kernel_type
 * @param kernel_vers
 * @param machine_host_name
 * @param machine_unique_id
 * @param pretty_prod_name
 * @param prod_type
 * @param prod_vers
 * @note <https://doc.qt.io/qt-5/qsysinfo.html>.
 */
void GkLevelDb::detect_operating_system(QString &build_cpu_arch, QString &curr_cpu_arch, QString &kernel_type, QString &kernel_vers,
                                        QString &machine_host_name, QString &machine_unique_id, QString &pretty_prod_name,
                                        QString &prod_type, QString &prod_vers)
{
    try {
        QSysInfo sys_info;

        build_cpu_arch = sys_info.buildCpuArchitecture();
        curr_cpu_arch = sys_info.currentCpuArchitecture();
        kernel_type = sys_info.kernelType();
        kernel_vers = sys_info.kernelVersion();

        machine_host_name = sys_info.machineHostName();
        machine_unique_id = sys_info.machineUniqueId();

        pretty_prod_name = sys_info.prettyProductName();
        prod_type = sys_info.productType();
        prod_vers = sys_info.productVersion();
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief GkLevelDb::convSeverityToStr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param severity
 * @return
 */
QString GkLevelDb::convSeverityToStr(const GkSeverity &severity)
{
    switch (severity) {
    case GkSeverity::Fatal:
        return tr("Fatal");
    case GkSeverity::Error:
        return tr("Error");
    case GkSeverity::Warning:
        return tr("Warning");
    case GkSeverity::Info:
        return tr("Info");
    case GkSeverity::Debug:
        return tr("Debug");
    case GkSeverity::Verbose:
        return tr("Verbose");
    case GkSeverity::None:
        return tr("None");
    default:
        return tr("Undefined");
    }

    return tr("Error!");
}

/**
 * @brief GkLevelDb::convSeverityToEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param severity
 * @return
 */
GekkoFyre::System::Events::Logging::GkSeverity GkLevelDb::convSeverityToEnum(const QString &severity)
{
    if (severity == tr("Fatal")) {
        // Fatal
        return GkSeverity::Fatal;
    } else if (severity == tr("Error")) {
        // Error
        return GkSeverity::Error;
    } else if (severity == tr("Warning")) {
        // Warning
        return GkSeverity::Warning;
    } else if (severity == tr("Info")) {
        // Info
        return GkSeverity::Info;
    } else if (severity == tr("Debug")) {
        // Debug
        return GkSeverity::Debug;
    } else if (severity == tr("Verbose")) {
        // Verbose
        return GkSeverity::Verbose;
    } else if (severity == tr("None")) {
        // None
        return GkSeverity::None;
    } else {
        // Unknown!
        return GkSeverity::None;
    }

    return Events::Logging::None;
}

/**
 * @brief GkLevelDb::convSeverityToSentry
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param severity
 * @return
 */
sentry_level_e GkLevelDb::convSeverityToSentry(const Events::Logging::GkSeverity &severity)
{
    switch (severity) {
        case GkSeverity::Fatal:
            return SENTRY_LEVEL_FATAL;
        case GkSeverity::Error:
            return SENTRY_LEVEL_ERROR;
        case GkSeverity::Warning:
            return SENTRY_LEVEL_WARNING;
        case GkSeverity::Info:
            return SENTRY_LEVEL_INFO;
        case GkSeverity::Debug:
            return SENTRY_LEVEL_DEBUG;
        case GkSeverity::Verbose:
            return SENTRY_LEVEL_DEBUG;
        case GkSeverity::None:
            return SENTRY_LEVEL_INFO;
        default:
            return SENTRY_LEVEL_INFO;
    }

    return SENTRY_LEVEL_INFO;
}

/**
 * @brief DekodeDb::read_rig_settings reads out the stored Small World Deluxe settings which are kept within a Google LevelDB
 * database that are stored within the user's storage media, either via a default storage place or through specified means as
 * configured by the user themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key The key which is required for retrieving the desired value(s) from the Google LevelDB database itself.
 * @return The desired value from the Google LevelDB database.
 */
QString GkLevelDb::read_rig_settings(const Database::Settings::radio_cfg &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    using namespace Database::Settings;
    switch (key) {
    case radio_cfg::RigBrand:
        status = db->Get(read_options, "RigBrand", &value);
        break;
    case radio_cfg::RigModel:
        status = db->Get(read_options, "RigModel", &value);
        break;
    case radio_cfg::RigModelIndex:
        status = db->Get(read_options, "RigModelIndex", &value);
        break;
    case radio_cfg::RigVersion:
        status = db->Get(read_options, "RigVersion", &value);
        break;
    case radio_cfg::CatConnType:
        status = db->Get(read_options, "CatConnType", &value);
        break;
    case radio_cfg::PttConnType:
        status = db->Get(read_options, "PttConnType", &value);
        break;
    case radio_cfg::StopBits:
        status = db->Get(read_options, "StopBits", &value);
        break;
    case radio_cfg::DataBits:
        status = db->Get(read_options, "DataBits", &value);
        break;
    case radio_cfg::Handshake:
        status = db->Get(read_options, "Handshake", &value);
        break;
    case radio_cfg::ForceCtrlLinesDtr:
        status = db->Get(read_options, "ForceCtrlLinesDtr", &value);
        break;
    case radio_cfg::ForceCtrlLinesRts:
        status = db->Get(read_options, "ForceCtrlLinesRts", &value);
        break;
    case radio_cfg::PTTMethod:
        status = db->Get(read_options, "PTTMethod", &value);
        break;
    case radio_cfg::TXAudioSrc:
        status = db->Get(read_options, "TXAudioSrc", &value);
        break;
    case radio_cfg::PTTMode:
        status = db->Get(read_options, "PTTMode", &value);
        break;
    case radio_cfg::SplitOperation:
        status = db->Get(read_options, "SplitOperation", &value);
        break;
    case radio_cfg::PTTAdvCmd:
        status = db->Get(read_options, "PTTAdvCmd", &value);
        break;
    case radio_cfg::RXAudioInitStart:
        status = db->Get(read_options, "RXAudioInitStart", &value);
        break;
    default:
        return "";
    }

    return QString::fromStdString(value);
}

/**
 * @brief GkLevelDb::read_rig_settings_comms reads out the stored Small World Deluxe settings which are kept within a Google LevelDB
 * database that are stored within the user's storage media, either via a default storage place or through specified means as
 * configured by the user themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key The key which is required for retrieving the desired value(s) from the Google LevelDB database itself.
 * @param conn_type The type of connection that is being used, whether it be USB, RS232, GPIO, etc.
 * @return The desired value from the Google LevelDB database.
 */
QString GkLevelDb::read_rig_settings_comms(const radio_cfg &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
        case radio_cfg::ComDeviceCat:
            status = db->Get(read_options, "ComDeviceCat", &value);
            break;
        case radio_cfg::ComDevicePtt:
            status = db->Get(read_options, "ComDevicePtt", &value);
            break;
        case radio_cfg::ComBaudRate:
            status = db->Get(read_options, "ComBaudRate", &value);
            break;
        case radio_cfg::ComDeviceCatPortType:
            status = db->Get(read_options, "ComDeviceCatPortType", &value);
            break;
        case radio_cfg::ComDevicePttPortType:
            status = db->Get(read_options, "ComDevicePttPortType", &value);
            break;
        default:
            return "";
    }

    return QString::fromStdString(value);
}

/**
 * @brief GkLevelDb::read_general_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key
 * @return
 */
QString GkLevelDb::read_general_settings(const general_stat_cfg &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
        case general_stat_cfg::defCqMsg:
            status = db->Get(read_options, "defCqMsg", &value);
            break;
        case general_stat_cfg::myCallsign:
            status = db->Get(read_options, "myCallsign", &value);
            break;
        case general_stat_cfg::defReplyMsg:
            status = db->Get(read_options, "defReplyMsg", &value);
            break;
        case general_stat_cfg::myMaidenhead:
            status = db->Get(read_options, "myMaidenhead", &value);
            break;
        case general_stat_cfg::defStationInfo:
            status = db->Get(read_options, "defStationInfo", &value);
            break;
        case general_stat_cfg::MsgAudioNotif:
            status = db->Get(read_options, "MsgAudioNotif", &value);
            break;
        case general_stat_cfg::FailAudioNotif:
            status = db->Get(read_options, "FailAudioNotif", &value);
            break;
        default:
            break;
    }

    return QString::fromStdString(value);
}

/**
 * @brief DekodeDb::read_audio_device_settings Reads out captured settings about the user's preferred audial device
 * preferences.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_output_device Are we dealing with an input or output audio device (i.e. microphone, loudspeaker, etc.)?
 * @return Outputs an int that is internally referred to as the `dev_number`, regarding the
 * `GekkoFyre::Database::Settings::Audio::Device` structure in `define.hpp`.
 */
QString GkLevelDb::read_audio_device_settings(const bool &is_output_device)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value;

    std::lock_guard<std::mutex> lck_guard(read_audio_dev_mtx);
    read_options.verify_checksums = true;

    if (is_output_device) {
        status = db->Get(read_options, "AudioOutputDeviceName", &value);
    } else {
        status = db->Get(read_options, "AudioInputDeviceName", &value);
    }

    return QString::fromStdString(value);
}

/**
 * @brief
 * @param length
 * @return
 */
std::string GkLevelDb::createRandomString(const qint32 &length)
{
    const std::string char_str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, char_str.size() - 1);

    std::string random_string;
    for (std::size_t i = 0; i < length; ++i) {
        random_string += char_str[distribution(generator)];
    }

    return random_string;
}

/**
 * @brief GkLevelDb::removeInvalidChars removes invalid/illegal characters from a given std::string, making it all clean!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param string_to_modify The given std::string to modify.
 * @return The cleaned up std::string!
 */
std::string GkLevelDb::removeInvalidChars(const std::string &string_to_modify)
{
    try {
        std::string illegal_chars = "\\/:?\"<>|";
        std::string str_tmp = string_to_modify;
        std::string::iterator it;
        for (it = str_tmp.begin(); it < str_tmp.end(); ++it) {
            bool ill_found = illegal_chars.find(*it) != std::string::npos;
            if (ill_found) {
                *it = ' ';
            }
        }

        if (!str_tmp.empty()) {
            return str_tmp;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return "";
}

/**
 * @brief DekodeDb::read_audio_details_settings Reads all the settings concerning Audio Devices from the
 * Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_output_device Whether we are dealing with an output or input audio device.
 * @return The struct, `GekkoFyre::Database::Settings::Audio::Device`.
 */
GkDevice GkLevelDb::read_audio_details_settings(const bool &is_output_device)
{
    GkDevice audio_device;
    leveldb::Status status;
    leveldb::ReadOptions read_options;

    read_options.verify_checksums = true;

    if (is_output_device) {
        //
        // Output audio device
        //
        std::string output_id;
        std::string output_pa_host_idx; //-V808
        std::string output_sel_channels;
        std::string output_def_sys_device;
        std::string output_user_activity;

        status = db->Get(read_options, "AudioOutputDeviceName", &output_id);
        status = db->Get(read_options, "AudioOutputDefSysDevice", &output_def_sys_device);
        status = db->Get(read_options, "AudioOutputCfgUsrActivity", &output_user_activity);

        bool def_sys_device = boolStr(output_def_sys_device);
        bool user_activity = boolStr(output_user_activity);

        //
        // Test to see if the following are empty or not
        //
        audio_device.audio_dev_str = QString::fromStdString(output_id);

        if (!output_sel_channels.empty()) {
            audio_device.sel_channels = convertAudioChannelsEnum(std::stoi(output_sel_channels));
        } else {
            audio_device.sel_channels = GkAudioChannels::Unknown;
        }

        audio_device.default_output_dev = def_sys_device;
        audio_device.user_config_succ = user_activity;
    } else {
        //
        // Input audio device
        //
        std::string input_id;
        std::string input_pa_host_idx; //-V808
        std::string input_sel_channels;
        std::string input_def_sys_device;
        std::string input_user_activity;

        status = db->Get(read_options, "AudioInputDeviceName", &input_id);
        status = db->Get(read_options, "AudioInputDefSysDevice", &input_def_sys_device);
        status = db->Get(read_options, "AudioInputCfgUsrActivity", &input_user_activity);

        bool def_sys_device = boolStr(input_def_sys_device);
        bool user_activity = boolStr(input_user_activity);

        //
        // Test to see if the following are empty or not
        //
        audio_device.audio_dev_str = QString::fromStdString(input_id);

        if (!input_sel_channels.empty()) {
            audio_device.sel_channels = convertAudioChannelsEnum(std::stoi(input_sel_channels));
        } else {
            audio_device.sel_channels = GkAudioChannels::Unknown;
        }

        audio_device.default_input_dev = def_sys_device;
        audio_device.user_config_succ = user_activity;
    }

    return audio_device;
}

QString GkLevelDb::read_misc_audio_settings(const GkAudioCfg &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
        case GkAudioCfg::settingsDbLoc:
            status = db->Get(read_options, "UserProfileDbLoc", &value);
            break;
        case GkAudioCfg::LogsDirLoc:
            status = db->Get(read_options, "UserLogsLoc", &value);
            break;
        case GkAudioCfg::AudioRecLoc:
            status = db->Get(read_options, "AudioRecSaveLoc", &value);
            break;
        case GkAudioCfg::AudioInputChannels:
            status = db->Get(read_options, "AudioInputChannels", &value);
            break;
        case GkAudioCfg::AudioOutputChannels:
            status = db->Get(read_options, "AudioOutputChannels", &value);
            break;
        case GkAudioCfg::AudioInputSampleRate:
            status = db->Get(read_options, "AudioInputSampleRate", &value);
            break;
        case GkAudioCfg::AudioOutputSampleRate:
            status = db->Get(read_options, "AudioOutputSampleRate", &value);
            break;
        case GkAudioCfg::AudioInputBitrate:
            status = db->Get(read_options, "AudioInputBitrate", &value);
            break;
        case GkAudioCfg::AudioOutputBitrate:
            status = db->Get(read_options, "AudioOutputBitrate", &value);
            break;
    }

    return QString::fromStdString(value);
}

/**
 * @brief GkLevelDb::read_event_log_settings reads out settings relevant to the custom Event Logger for Small World Deluxe, such
 * as the verbosity level as desired by the end-user themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key The key that is to be read from within Google LevelDB.
 * @return The value itself, as related to the key.
 */
QString GkLevelDb::read_event_log_settings(const GkEventLogCfg &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
        case GkEventLogCfg::GkLogVerbosity:
            status = db->Get(read_options, "UserProfileDbLoc", &value);
            break;
        default:
            throw std::runtime_error(tr("Invalid key has been provided for reading Event Logger settings relating to Google LevelDB!").toStdString());
    }

    return QString::fromStdString(value);
}

/**
 * @brief GkLevelDb::read_audio_playback_dlg_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key
 * @return
 */
QString GkLevelDb::read_audio_playback_dlg_settings(const AudioPlaybackDlg &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
        case AudioPlaybackDlg::GkAudioDlgLastFolderBrowsed:
            status = db->Get(read_options, "GkAudioDlgLastFolderBrowsed", &value);
            break;
        case AudioPlaybackDlg::GkRecordDlgLastFolderBrowsed:
            status = db->Get(read_options, "GkRecordDlgLastFolderBrowsed", &value);
            break;
        case AudioPlaybackDlg::GkRecordDlgLastCodecSelected:
            status = db->Get(read_options, "GkRecordDlgLastCodecSelected", &value);
            break;
        default:
            throw std::runtime_error(tr("Invalid key has been provided for reading Audio Playback dialog settings relating to Google LevelDB!").toStdString());
    }

    return QString::fromStdString(value);
}

/**
 * @brief GkLevelDb::read_firewall_settings reads out stored settings pertaining to the default firewall built into the
 * operating system (if there is one, such as the provided firewall provided with Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key The key for which the supposedly stored setting is under. Returns a nullptr if nothing is stored under that
 * particular key.
 * @param comparator_value The std::string to make a comparison against; this is only applicable for some, certain keys.
 * @return The stored setting otherwise an exception is returned if there was no stored setting under a given key.
 */
bool GkLevelDb::read_firewall_settings(const Security::GkFirewallCfg &key, const std::string &comparator_value)
{
    try {
        leveldb::Status status;
        leveldb::ReadOptions read_options;
        std::string value = "";

        read_options.verify_checksums = true;
        std::vector<std::string> firewall_values;

        switch (key) {
            case Security::GkIsPortAdded:
            {
                firewall_values = readMultipleKeys("GkFirewallPorts");
                for (const auto &firewall: firewall_values) {
                    if (comparator_value == firewall) {
                        return true;
                    }
                }
            }

                break;
            case Security::GkIsAppAdded:
            {
                firewall_values = readMultipleKeys("GkFirewallApps");
                for (const auto &firewall: firewall_values) {
                    if (comparator_value == firewall) {
                        return true;
                    }
                }
            }

                break;
            case Security::GkIsFirewallActive:
            {
                status = db->Get(read_options, "GkFirewallActive", &value);
                if (!status.ok()) { // Abort because of error!
                    throw std::runtime_error(tr("Issues have been encountered while trying to write towards the user profile! Error:\n\n%1").arg(QString::fromStdString(status.ToString())).toStdString());
                }

                if (comparator_value == value) {
                    return true;
                }
            }

                break;
            default:
                throw std::runtime_error(tr("Invalid key has been provided for reading Firewall settings relating to Google LevelDB!").toStdString());
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return false;
}

/**
 * @brief GkLevelDb::read_firewall_settings_vec reads out stored settings (only regarding ports and applications in this
 * function) pertaining to the default firewall built into the operating system (if there is one, such as the provided
 * firewall provided with Microsoft Windows).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key The key for which the supposedly stored setting is under. Returns a nullptr if nothing is stored under that
 * particular key.
 * @return The stored setting, as a vector, otherwise nullptr is returned if there was no stored setting under a given key.
 */
std::vector<std::string> GkLevelDb::read_firewall_settings_vec(const Security::GkFirewallCfg &key)
{
    try {
        std::vector<std::string> firewall_values; //-V808
        switch (key) {
            case Security::GkFirewallCfg::GkReadPorts:
                return readMultipleKeys("GkFirewallPorts");
            case Security::GkReadApps:
                return readMultipleKeys("GkFirewallApps");
            default:
                break;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return std::vector<std::string>();
}

/**
 * @brief DekodeDb::convertAudioChannelsEnum converts from an index given by
 * a QComboBox and not by a specific amount of channels.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_channel_sel The index as given by a QComboBox.
 * @return The relevant channel enumerator.
 */
GkAudioChannels GkLevelDb::convertAudioChannelsEnum(const int &audio_channel_sel)
{
    GkAudioChannels ret = Mono;
    switch (audio_channel_sel) {
        case 0:
            ret = GkAudioChannels::Mono;
            break;
        case 1:
            ret = GkAudioChannels::Left;
            break;
        case 2:
            ret = GkAudioChannels::Right;
            break;
        case 3:
            ret = GkAudioChannels::Both;
            break;
        case 4:
            ret = GkAudioChannels::Surround;
            break;
        default:
            ret = GkAudioChannels::Unknown;
            break;
    }

    return ret;
}

/**
 * @brief GkLevelDb::convertAudioChannelsToCount converts an audio channel enumerator to how many audio channels there are as an
 * actual integer, so for example, `GkAudioChannels::Both` is an integer of two channels.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param channel_enum The channel enumerator.
 * @return How many channels there are as an integer.
 */
qint32 GkLevelDb::convertAudioChannelsToCount(const GkAudioChannels &channel_enum)
{
    switch (channel_enum) {
        case Mono:
            return 1;
        case Left:
            return 1;
        case Right:
            return 1;
        case Both:
            return 2;
        case Surround:
            return -2; // NOTE: This is such because it needs to be worked out via other means!
        default:
            return -1;
    }

    return -1;
}

/**
 * @brief GkLevelDb::convertAudioChannelsStr converts the audio channel count enumerator to its related QString reference.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param channel_enum The channel count enumerator.
 * @return The related QString for the given channel count enumerator.
 */
QString GkLevelDb::convertAudioChannelsStr(const GkAudioChannels &channel_enum)
{
    switch (channel_enum) {
        case Mono:
            return tr("Mono");
        case Left:
            return tr("Left");
        case Right:
            return tr("Right");
        case Both:
            return tr("Stereo");
        case Surround:
            return tr("Surround");
        default:
            return tr("Unknown");
    }

    return tr("Unknown");
}

/**
 * @brief GkLevelDb::convertAudioEnumIsStereo determines whether the current audio channel count in question is capable of
 * supporting Stereo output or not.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param channel_enum The channel count enumerator.
 * @return Whether stereo sound output is supported or not.
 */
bool GkLevelDb::convertAudioEnumIsStereo(const GkAudioChannels &channel_enum) const
{
    switch (channel_enum) {
        case Mono:
            return false;
        case Left:
            return false;
        case Right:
            return false;
        case Both:
            return true;
        case Surround:
            return false;
        default:
            return false;
    }

    return false;
}

/**
 * @brief GkLevelDb::convAudioBitRateToEnum converts a given bit-rate to the nearest, or rather, the most
 * appropriate Sample Type as per under QAudioSystem.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param bit_rate Whether we are using 8-bits, 16-bits, etc.
 * @return For 8-bit samples, you would use QAudioFormat::UnsignedInt, whilst for 16-bit samples, QAudioFormat:SignedInt is
 * the modus operandi. Anything higher then you must use QAudioFormat::Float.
 */
QAudioFormat::SampleType GkLevelDb::convAudioBitRateToEnum(const qint32 &bit_rate)
{
    switch (bit_rate) {
        case 8:
            return QAudioFormat::UnSignedInt;
        case 16:
            return QAudioFormat::SignedInt;
        case 24:
            return QAudioFormat::Float;
        case 32:
            return QAudioFormat::Float;
        default:
            return QAudioFormat::Float;
    }

    return QAudioFormat::Unknown;
}

/**
 * @brief GkLevelDb::convPttTypeToEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param ptt_type_str
 * @return
 */
ptt_type_t GkLevelDb::convPttTypeToEnum(const QString &ptt_type_str)
{
    if (ptt_type_str == tr("RIG_PTT_RIG")) {
        return RIG_PTT_RIG;
    } else if (ptt_type_str == tr("RIG_PTT_SERIAL_DTR")) {
        return RIG_PTT_SERIAL_DTR;
    } else if (ptt_type_str == tr("RIG_PTT_SERIAL_RTS")) {
        return RIG_PTT_SERIAL_RTS;
    } else if (ptt_type_str == tr("RIG_PTT_PARALLEL")) {
        return RIG_PTT_PARALLEL;
    } else if (ptt_type_str == tr("RIG_PTT_RIG_MICDATA")) {
        return RIG_PTT_RIG_MICDATA;
    } else if (ptt_type_str == tr("RIG_PTT_CM108")) {
        return RIG_PTT_CM108;
    } else if (ptt_type_str == tr("RIG_PTT_GPIO")) {
        return RIG_PTT_GPIO;
    } else {
        return RIG_PTT_NONE;
    }

    return RIG_PTT_NONE;
}

/**
 * @brief GkLevelDb::convPttTypeToStr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param ptt_type_enum
 * @return
 */
QString GkLevelDb::convPttTypeToStr(const ptt_type_t &ptt_type_enum)
{
    switch (ptt_type_enum) {
    case ptt_type_t::RIG_PTT_RIG:
        return tr("RIG_PTT_RIG");
    case ptt_type_t::RIG_PTT_SERIAL_DTR:
        return tr("RIG_PTT_SERIAL_DTR");
    case ptt_type_t::RIG_PTT_SERIAL_RTS:
        return tr("RIG_PTT_SERIAL_RTS");
    case ptt_type_t::RIG_PTT_PARALLEL:
        return tr("RIG_PTT_PARALLEL");
    case ptt_type_t::RIG_PTT_RIG_MICDATA:
        return tr("RIG_PTT_RIG_MICDATA");
    case ptt_type_t::RIG_PTT_CM108:
        return tr("RIG_PTT_CM108");
    case ptt_type_t::RIG_PTT_GPIO:
        return tr("RIG_PTT_GPIO");
    case ptt_type_t::RIG_PTT_GPION:
        return tr("RIG_PTT_GPION");
    default:
        return tr("RIG_PTT_NONE");
    }

    return tr("RIG_PTT_NONE");
}

/**
 * @brief GkLevelDb::convConnTypeToEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param conn_type
 * @return
 */
GkConnType GkLevelDb::convConnTypeToEnum(const int &conn_type)
{
    switch (conn_type) {
    case 0:
        return GkConnType::GkNone;
    case 1:
        return GkConnType::GkRS232;
    case 2:
        return GkConnType::GkUSB;
    case 3:
        return GkConnType::GkParallel;
    case 4:
        return GkConnType::GkCM108;
    case 5:
        return GkConnType::GkGPIO;
    default:
        return GkConnType::GkNone;
    }

    return GkConnType::GkNone;
}

/**
 * @brief GkLevelDb::convConnTypeToInt
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param conn_type
 * @return
 */
int GkLevelDb::convConnTypeToInt(const GkConnType &conn_type)
{
    switch (conn_type) {
    case GkConnType::GkRS232:
        return 1;
    case GkConnType::GkUSB:
        return 2;
    case GkConnType::GkParallel:
        return 3;
    case GkConnType::GkCM108:
        return 4;
    case GkConnType::GkGPIO:
        return 5;
    case GkConnType::GkNone:
        return 0;
    default:
        return -1;
    }

    return -1;
}

QString GkLevelDb::convAudioBitrateToStr(const GkAudioFramework::Bitrate &bitrate)
{
    switch (bitrate) {
    case GkAudioFramework::Bitrate::VBR:
        return tr("Variable bitrate (i.e. VBR)");
    case GkAudioFramework::Bitrate::Kbps64:
        return tr("64 Kbps");
    case GkAudioFramework::Bitrate::Kbps128:
        return tr("128 Kbps");
    case GkAudioFramework::Bitrate::Kbps192:
        return tr("192 Kbps");
    case GkAudioFramework::Bitrate::Kbps256:
        return tr("256 Kbps");
    case GkAudioFramework::Bitrate::Kbps320:
        return tr("320 Kbps");
    default:
        return tr("Variable bitrate (i.e. VBR)");
    }

    return tr("Variable bitrate (i.e. VBR)");
}

/**
 * @brief GkLevelDb::translateBandsToStr will translate a given band to the equivalent QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param band The given amateur radio band, in meters.
 * @return The amateur radio band, in meters, provided as a QString().
 */
QString GkLevelDb::convBandsToStr(const GkFreqBands &band)
{
    switch (band) {
    case GkFreqBands::BAND160:
        return tr("None");
    case GkFreqBands::BAND80:
        return tr("80 meters");
    case GkFreqBands::BAND60:
        return tr("60 meters");
    case GkFreqBands::BAND40:
        return tr("40 meters");
    case GkFreqBands::BAND30:
        return tr("30 meters");
    case GkFreqBands::BAND20:
        return tr("20 meters");
    case GkFreqBands::BAND17:
        return tr("15 meters");
    case GkFreqBands::BAND15:
        return tr("17 meters");
    case GkFreqBands::BAND12:
        return tr("12 meters");
    case GkFreqBands::BAND10:
        return tr("10 meters");
    case GkFreqBands::BAND6:
        return tr("6 meters");
    case GkFreqBands::BAND2:
        return tr("2 meters");
    default:
        return tr("Unsupported!");
    }

    return tr("Error!");
}

/**
 * @brief GkLevelDb::convDigitalModesToStr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param digital_mode
 * @return
 */
QString GkLevelDb::convDigitalModesToStr(const DigitalModes &digital_mode)
{
    switch (digital_mode) {
    case DigitalModes::WSPR:
        return tr("WSPR");
    case DigitalModes::JT65:
        return tr("JT65");
    case DigitalModes::JT9:
        return tr("JT9");
    case DigitalModes::T10:
        return tr("T10");
    case DigitalModes::FT8:
        return tr("FT8");
    case DigitalModes::FT4:
        return tr("FT4");
    case DigitalModes::Codec2:
        return tr("Codec2");
    default:
        return tr("Unsupported!");
    }

    return tr("Error!");
}

/**
 * @brief GkLevelDb::convIARURegionToStr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param iaru_region
 * @return
 */
QString GkLevelDb::convIARURegionToStr(const IARURegions &iaru_region)
{
    switch (iaru_region) {
    case IARURegions::ALL:
        return tr("ALL");
    case IARURegions::R1:
        return tr("R1");
    case IARURegions::R2:
        return tr("R2");
    case IARURegions::R3:
        return tr("R3");
    default:
        return tr("Unsupported!");
    }

    return tr("Error!");
}

/**
 * @brief GkLevelDb::convCodecSupportFromIdxToEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec_support_idx
 * @return
 */
GekkoFyre::GkAudioFramework::CodecSupport GkLevelDb::convCodecSupportFromIdxToEnum(const qint32 &codec_support_idx)
{
    switch (codec_support_idx) {
        case AUDIO_PLAYBACK_CODEC_PCM_IDX:
            return GekkoFyre::GkAudioFramework::CodecSupport::PCM;
        case AUDIO_PLAYBACK_CODEC_LOOPBACK_IDX:
            return GekkoFyre::GkAudioFramework::CodecSupport::Loopback;
        case AUDIO_PLAYBACK_CODEC_VORBIS_IDX:
            return GekkoFyre::GkAudioFramework::CodecSupport::OggVorbis;
        case AUDIO_PLAYBACK_CODEC_OPUS_IDX:
            return GekkoFyre::GkAudioFramework::CodecSupport::Opus;
        case AUDIO_PLAYBACK_CODEC_FLAC_IDX:
            return GekkoFyre::GkAudioFramework::CodecSupport::FLAC;
        default:
            return GekkoFyre::GkAudioFramework::CodecSupport::Unsupported;
    }

    return GekkoFyre::GkAudioFramework::CodecSupport::Unknown;
}

/**
 * @brief GkLevelDb::convCodecFormatToFileExtension outputs a file extension for the given codec enumerator as specified by
 * the Small World Deluxe application.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param codec_support
 * @return
 */
QString GkLevelDb::convCodecFormatToFileExtension(const CodecSupport &codec_support)
{
    switch (codec_support) {
        case CodecSupport::Opus:
            return QString(".opus");
        case CodecSupport::PCM:
            return tr(".pcm");
        case CodecSupport::Loopback:
            return tr(".loopback");
        case CodecSupport::OggVorbis:
            return QString(".ogg");
        case CodecSupport::FLAC:
            return QString(".opus");
        case CodecSupport::Unsupported:
            return tr(".unsupported");
        case CodecSupport::Unknown:
            return tr(".unknown");
    }

    return QString();
}

/**
 * @brief DekodeDb::boolEnum Will enumerate a boolean value to an std::string, ready for use within a database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_true Whether we are dealing with a true or false situation.
 * @return The enumerated boolean value.
 */
std::string GkLevelDb::boolEnum(const bool &is_true)
{
    if (is_true) {
        return "true";
    } else {
        return "false";
    }

    return "";
}

/**
 * @brief DekodeDb::boolStr Will convert a literal "true" or "false" std::string into a boolean value, ready for
 * use straight from a database!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_true Whether we are dealing with a true or false situation.
 * @return The boolean value itself.
 */
bool GkLevelDb::boolStr(const std::string &is_true)
{
    bool ret = false;
    if (is_true == "true") {
        ret = true;
    } else {
        ret = false;
    }

    return ret;
}

/**
 * @brief GkLevelDb::boolInt
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_true
 * @return
 */
qint32 GkLevelDb::boolInt(const bool &is_true)
{
    int ret = -1;
    if (is_true == true) {
        return 1;
    } else {
        return 0;
    }

    return ret;
}

/**
 * @brief GkLevelDb::intBool converts an integer to the equivalent boolean value, or otherwise, throws an exception if it
 * is out-of-range.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The corresponding integer value.
 * @return Whether a corresponding true or false was indicated.
 */
bool GkLevelDb::intBool(const qint32 &value)
{
    switch (value) {
        case 0:
            return false;
        case 1:
            return true;
        default:
            std::throw_with_nested(std::invalid_argument(tr("An invalid boolean value was provided!").toStdString()));
    }

    return false;
}

/**
 * @brief GkLevelDb::processCsvToDB takes pre-existing data and appends new values towards it, in the form of CSV. It then returns
 * all the newly made data as a CSV string. Data modifications to existing rows can be made too, which are automatically detected.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param csv_title The header/title to give the CSV list.
 * @param comma_sep_values The pre-existing CSV information.
 * @param data_to_append The information to append onto the pre-existing CSV information, if any.
 * @return The newly made data, with the appended information, as a CSV string.
 */
std::string GkLevelDb::processCsvToDB(const std::string &csv_title, const std::string &comma_sep_values, const std::string &data_to_append)
{
    try {
        if (!comma_sep_values.empty()) {
            // We are continuing with a pre-existing data set!
            if (!data_to_append.empty()) {
                auto old_csv_vals = gkStringFuncs->csvSplitter(comma_sep_values);
                auto new_csv_vals = gkStringFuncs->csvSplitter(data_to_append);
                std::stringstream ss;

                for (auto csv_old: old_csv_vals) {
                    csv_old.erase(std::remove(csv_old.begin(), csv_old.end(), '\n'), csv_old.end());
                    ss << csv_old << ',';
                }

                for (auto iter = new_csv_vals.begin(); iter != new_csv_vals.end(); ++iter) {
                    if (std::next(iter) != new_csv_vals.end()) {
                        // Perform this action for all but the last iteration!
                        ss << *iter << ',';
                    } else {
                        // Perform this action for only the last iteration...
                        ss << *iter;
                    }
                }

                std::string ret_val = ss.str();
                return ret_val;
            }
        } else {
            // We are beginning with a new data set!
            auto csv_vals = gkStringFuncs->csvSplitter(data_to_append);
            csv_vals.insert(csv_vals.begin(), csv_title);
            auto ret_val = gkStringFuncs->csvOutputString(csv_vals);

            return ret_val;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::invalid_argument(tr("An error has occurred whilst processing CSV for Google LevelDB!").toStdString()));
    }

    return "";
}

/**
 * @brief GkLevelDb::deleteCsvValForDb
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param comma_sep_values
 * @param data_to_remove
 * @return
 */
std::string GkLevelDb::deleteCsvValForDb(const std::string &comma_sep_values, const std::string &data_to_remove)
{
    //
    // TODO: Test to see if this function actually works or not!
    //
    try {
        if (!comma_sep_values.empty() && !data_to_remove.empty()) {
            std::stringstream sstream(comma_sep_values);
            rapidcsv::Document doc(sstream, rapidcsv::LabelParams(), rapidcsv::SeparatorParams(',', true, false, false, false));
            std::vector<std::string> csv_vals = doc.GetColumn<std::string>("FreqValues");

            for (size_t i = 0; i < csv_vals.size(); ++i) {
                if (csv_vals[i] == data_to_remove) {
                    // The value to be removed does indeed exist within the Google LevelDB database!
                    doc.RemoveRow(i);
                    break;
                }
            }

            std::stringstream output_stream;
            output_stream << "FreqValues" << std::endl;
            for (const auto &csv: csv_vals) {
                // Print out as a bunch of textual CSV data, ready for insertion into a Google LevelDB database!
                output_stream << csv << std::endl;
            }

            return output_stream.str();
        } // Otherwise return an empty std::string!
    } catch (const std::exception &e) {
        std::throw_with_nested(std::invalid_argument(tr("An error has occurred whilst processing CSV for Google LevelDB! Error:\n\n%1")
        .arg(QString::fromStdString(e.what())).toStdString()));
    }

    return "";
}
