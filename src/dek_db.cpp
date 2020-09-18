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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "dek_db.hpp"
#include "src/audio_devices.hpp"
#include "src/contrib/rapidcsv/src/rapidcsv.h"
#include <sentry.h>
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
#include <QVariant>
#include <QSysInfo>
#include <QDebug>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <utility>
#include <vector>
#include <random>
#include <chrono>
#include <ctime>

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

namespace fs = boost::filesystem;
namespace sys = boost::system;

std::mutex read_audio_dev_mtx;
std::mutex read_audio_api_mtx;
std::mutex mtx_freq_already_init;

GkLevelDb::GkLevelDb(leveldb::DB *db_ptr, QPointer<FileIo> filePtr, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                     QObject *parent) : QObject(parent)
{
    db = db_ptr;
    fileIo = std::move(filePtr);
    gkStringFuncs = std::move(stringFuncs);
}

GkLevelDb::~GkLevelDb()
{}

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
            batch.Put("AudioOutputSelChannels", std::to_string(value.sel_channels));
            batch.Put("AudioOutputId", value.chosen_audio_dev_str.toStdString());
            batch.Put("AudioOutputPaHostIndex", std::to_string(value.dev_number));

            // Determine if this is the default output device for the system and if so, convert
            // the boolean value to a std::string suitable for database storage.
            std::string is_default = boolEnum(value.default_dev);
            batch.Put("AudioOutputDefSysDevice", is_default);
        } else {
            // Unique identifier for the chosen input audio device
            batch.Put("AudioInputSelChannels", std::to_string(value.sel_channels));
            batch.Put("AudioInputId", value.chosen_audio_dev_str.toStdString());
            batch.Put("AudioInputPaHostIndex", std::to_string(value.dev_number));

            // Determine if this is the default input device for the system and if so, convert
            // the boolean value to a std::string suitable for database storage.
            std::string is_default = boolEnum(value.default_dev);
            batch.Put("AudioInputDefSysDevice", is_default);
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
 * @brief DekodeDb::write_mainwindow_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void GkLevelDb::write_mainwindow_settings(const QString &value, const general_mainwindow_cfg &key)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        using namespace Database::Settings;
        switch (key) {
        case general_mainwindow_cfg::WindowMaximized:
            batch.Put("WindowMaximized", value.toStdString());
            break;
        case general_mainwindow_cfg::WindowHSize:
            batch.Put("WindowHSize", value.toStdString());
            break;
        case general_mainwindow_cfg::WindowVSize:
            batch.Put("WindowVSize", value.toStdString());
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
 * @brief GkLevelDb::write_audio_cfg_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void GkLevelDb::write_misc_audio_settings(const QString &value, const audio_cfg &key)
{
    try {
        // Put key-value
        leveldb::WriteBatch batch;
        leveldb::Status status;

        switch (key) {
        case audio_cfg::soundcardInput:
            batch.Put("SoundcardInput", value.toStdString());
            break;
        case audio_cfg::soundcardOutput:
            batch.Put("SoundcardOutput", value.toStdString());
            break;
        case audio_cfg::settingsDbLoc:
            batch.Put("UserProfileDbLoc", value.toStdString());
            break;
        case audio_cfg::LogsDirLoc:
            batch.Put("UserLogsLoc", value.toStdString());
            break;
        case audio_cfg::AudioRecLoc:
            batch.Put("AudioRecSaveLoc", value.toStdString());
            break;
        case audio_cfg::AudioInputChannels:
            batch.Put("AudioInputChannels", value.toStdString());
            break;
        case audio_cfg::AudioOutputChannels:
            batch.Put("AudioOutputChannels", value.toStdString());
            break;
        case audio_cfg::AudioInputSampleRate:
            batch.Put("AudioInputSampleRate", value.toStdString());
            break;
        case audio_cfg::AudioOutputSampleRate:
            batch.Put("AudioOutputSampleRate", value.toStdString());
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
        csv_stored_freq = processCsvToDB(gk_stored_freq_value, QString::number(write_new_value.frequency).toStdString()); // Frequency (CSV)
        batch.Put("GkStoredFreq", csv_stored_freq);

        // Closest matching frequency band
        std::string gk_closest_band_value = "";
        db->Get(read_options, "GkClosestBand", &gk_closest_band_value);
        std::string csv_closest_band;
        csv_closest_band = processCsvToDB(gk_closest_band_value, convBandsToStr(write_new_value.closest_freq_band).toStdString()); // Closest matching frequency band (CSV)
        batch.Put("GkClosestBand", csv_closest_band);

        // Digital mode
        std::string gk_digital_mode_value = "";
        db->Get(read_options, "GkDigitalMode", &gk_digital_mode_value);
        std::string csv_digital_mode;
        csv_digital_mode = processCsvToDB(gk_digital_mode_value, convDigitalModesToStr(write_new_value.digital_mode).toStdString()); // Digital mode (CSV)
        batch.Put("GkDigitalMode", csv_digital_mode);

        // IARU Region
        std::string gk_iaru_region_value = "";
        db->Get(read_options, "GkIARURegion", &gk_iaru_region_value);
        std::string csv_iaru_region;
        csv_iaru_region = processCsvToDB(gk_iaru_region_value, convIARURegionToStr(write_new_value.iaru_region).toStdString()); // IARU Region (CSV)
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
            const std::string ret_str = sentry_unique_id.toStdString();
        } else {
            const std::string ret_str = randomString(24);
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
        const QRect screen_res = detect_desktop_resolution();
        const qreal width = screen_res.width();
        const qreal height = screen_res.height();

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
 * @brief GkLevelDb::detect_desktop_resolution will detect the currently used screen resolution of the user's desktop, mostly
 * for the purposes needed by Sentry.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param horizontal
 * @param vertical
 */
QRect GkLevelDb::detect_desktop_resolution()
{
    QRect rec = QApplication::desktop()->screenGeometry();
    return rec;
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
QString GkLevelDb::read_audio_device_settings(const bool &is_output_device, const bool &index_only)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value;

    std::lock_guard<std::mutex> lck_guard(read_audio_dev_mtx);
    read_options.verify_checksums = true;

    if (!index_only) {
        if (is_output_device) {
            // We are dealing with an audio output device
            status = db->Get(read_options, "AudioOutputId", &value);
        } else {
            // We are dealing with an audio input device
            status = db->Get(read_options, "AudioInputId", &value);
        }
    } else {
        if (is_output_device) {
            status = db->Get(read_options, "AudioOutputPaHostIndex", &value);
        } else {
            status = db->Get(read_options, "AudioInputPaHostIndex", &value);
        }
    }

    return QString::fromStdString(value);
}

/**
 * @brief GkLevelDb::write_audio_api_settings Writes out the saved information concerning the user's choice of
 * decided upon PortAudio API settings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param interface The user's last chosen settings for the decided upon PortAudio API.
 */
void GkLevelDb::write_audio_api_settings(const PaHostApiTypeId &interface)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        batch.Put("AudioPortAudioAPISelection", portAudioApiToStr(interface).toStdString());

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
 * @brief GkLevelDb::read_audio_api_settings Reads the saved information concerning the user's choice of decided
 * upon PortAudio API settings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The user's last chosen settings for the decided upon PortAudio API.
 */
PaHostApiTypeId GkLevelDb::read_audio_api_settings()
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value;

    std::lock_guard<std::mutex> lck_guard(read_audio_api_mtx);
    read_options.verify_checksums = true;

    status = db->Get(read_options, "AudioPortAudioAPISelection", &value);

    if (!value.empty()) {
        // Convert from `std::string` to `enum`!
        return portAudioApiToEnum(QString::fromStdString(value));
    }

    return PaHostApiTypeId::paInDevelopment;
}

/**
 * @brief GkLevelDb::portAudioApiToStr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param interface
 * @return
 */
QString GkLevelDb::portAudioApiToStr(const PaHostApiTypeId &interface)
{
    switch (interface) {
    case PaHostApiTypeId::paDirectSound:
        return tr("DirectSound");
    case PaHostApiTypeId::paMME:
        return tr("Microsoft Multimedia Environment (MME)");
    case PaHostApiTypeId::paASIO:
        return tr("ASIO");
    case PaHostApiTypeId::paSoundManager:
        return tr("Sound Manager");
    case PaHostApiTypeId::paCoreAudio:
        return tr("Core Audio");
    case PaHostApiTypeId::paOSS:
        return tr("OSS");
    case PaHostApiTypeId::paALSA:
        return tr("ALSA");
    case PaHostApiTypeId::paAL:
        return tr("AL");
    case PaHostApiTypeId::paBeOS:
        return tr("BeOS");
    case PaHostApiTypeId::paWDMKS:
        return tr("WDM/KS");
    case PaHostApiTypeId::paJACK:
        return tr("JACK");
    case PaHostApiTypeId::paWASAPI:
        return tr("Windows Audio Session API (WASAPI)");
    case PaHostApiTypeId::paAudioScienceHPI:
        return tr("AudioScience HPI");
    case PaHostApiTypeId::paInDevelopment:
        return tr("N/A");
    default:
        return tr("Unknown");
    }

    return tr("Unknown");
}

/**
 * @brief GkLevelDb::portAudioApiToEnum converts the stored string value of the PortAudio API setting to the
 * relevant enum value, for easier computation.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param interface
 * @return The more easily computable enum value.
 */
PaHostApiTypeId GkLevelDb::portAudioApiToEnum(const QString &interface)
{
    if (!interface.isNull() && !interface.isEmpty()) {
        if (interface == tr("DirectSound")) {
            return PaHostApiTypeId::paDirectSound;
        } else if (interface == tr("Microsoft Multimedia Environment (MME)")) {
            return PaHostApiTypeId::paMME;
        } else if (interface == tr("ASIO")) {
            return PaHostApiTypeId::paASIO;
        } else if (interface == tr("Sound Manager")) {
            return PaHostApiTypeId::paSoundManager;
        } else if (interface == tr("Core Audio")) {
            return PaHostApiTypeId::paCoreAudio;
        } else if (interface == tr("OSS")) {
            return PaHostApiTypeId::paOSS;
        } else if (interface == tr("ALSA")) {
            return PaHostApiTypeId::paALSA;
        } else if (interface == tr("AL")) {
            return PaHostApiTypeId::paAL;
        } else if (interface == tr("BeOS")) {
            return PaHostApiTypeId::paBeOS;
        } else if (interface == tr("WDM/KS")) {
            return PaHostApiTypeId::paWDMKS;
        } else if (interface == tr("JACK")) {
            return PaHostApiTypeId::paJACK;
        } else if (interface == tr("Windows Audio Session API (WASAPI)")) {
            return PaHostApiTypeId::paWASAPI;
        } else if (interface == tr("AudioScience HPI")) {
            return PaHostApiTypeId::paAudioScienceHPI;
        } else if (interface == tr("N/A")) {
            return PaHostApiTypeId::paInDevelopment;
        }
    }

    return PaHostApiTypeId::paInDevelopment;
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
        std::string output_pa_host_idx;
        std::string output_sel_channels;
        std::string output_def_sys_device;

        status = db->Get(read_options, "AudioOutputId", &output_id);
        status = db->Get(read_options, "AudioOutputPaHostIndex", &output_pa_host_idx);
        status = db->Get(read_options, "AudioOutputSelChannels", &output_sel_channels);
        status = db->Get(read_options, "AudioOutputDefSysDevice", &output_def_sys_device);

        bool def_sys_device = boolStr(output_def_sys_device);

        //
        // Test to see if the following are empty or not
        //
        audio_device.chosen_audio_dev_str = QString::fromStdString(output_id);

        if (!output_sel_channels.empty()) {
            audio_device.sel_channels = convertAudioChannelsEnum(std::stoi(output_sel_channels));
        } else {
            audio_device.sel_channels = audio_channels::Unknown;
        }

        audio_device.default_dev = def_sys_device;
    } else {
        //
        // Input audio device
        //
        std::string input_id;
        std::string input_pa_host_idx;
        std::string input_sel_channels;
        std::string input_def_sys_device;

        status = db->Get(read_options, "AudioInputId", &input_id);
        status = db->Get(read_options, "AudioInputPaHostIndex", &input_pa_host_idx);
        status = db->Get(read_options, "AudioInputSelChannels", &input_sel_channels);
        status = db->Get(read_options, "AudioInputDefSysDevice", &input_def_sys_device);

        bool def_sys_device = boolStr(input_def_sys_device);

        //
        // Test to see if the following are empty or not
        //
        audio_device.chosen_audio_dev_str = QString::fromStdString(input_id);

        if (!input_sel_channels.empty()) {
            audio_device.sel_channels = convertAudioChannelsEnum(std::stoi(input_sel_channels));
        } else {
            audio_device.sel_channels = audio_channels::Unknown;
        }

        audio_device.default_dev = def_sys_device;
    }

    return audio_device;
}

/**
 * @brief DekodeDb::read_mainwindow_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key
 * @return
 */
QString GkLevelDb::read_mainwindow_settings(const general_mainwindow_cfg &key)
{
    GkDevice audio_device;
    leveldb::Status status;
    leveldb::ReadOptions read_options;

    read_options.verify_checksums = true;
    std::string output = "";

    using namespace Database::Settings;
    switch (key) {
    case general_mainwindow_cfg::WindowMaximized:
        status = db->Get(read_options, "WindowMaximized", &output);
        return QString::fromStdString(output);
    case general_mainwindow_cfg::WindowHSize:
        status = db->Get(read_options, "WindowHSize", &output);
        return QString::fromStdString(output);
    case general_mainwindow_cfg::WindowVSize:
        status = db->Get(read_options, "WindowVSize", &output);
        return QString::fromStdString(output);
    default:
        return QString::fromStdString(output);
    }

    return QString::fromStdString(output);
}

QString GkLevelDb::read_misc_audio_settings(const audio_cfg &key)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value = "";

    read_options.verify_checksums = true;

    switch (key) {
    case audio_cfg::soundcardInput:
        status = db->Get(read_options, "SoundcardInput", &value);
        break;
    case audio_cfg::soundcardOutput:
        status = db->Get(read_options, "SoundcardOutput", &value);
        break;
    case audio_cfg::settingsDbLoc:
        status = db->Get(read_options, "UserProfileDbLoc", &value);
        break;
    case audio_cfg::LogsDirLoc:
        status = db->Get(read_options, "UserLogsLoc", &value);
        break;
    case audio_cfg::AudioRecLoc:
        status = db->Get(read_options, "AudioRecSaveLoc", &value);
        break;
    case audio_cfg::AudioInputChannels:
        status = db->Get(read_options, "AudioInputChannels", &value);
        break;
    case audio_cfg::AudioOutputChannels:
        status = db->Get(read_options, "AudioOutputChannels", &value);
        break;
    case audio_cfg::AudioInputSampleRate:
        status = db->Get(read_options, "AudioInputSampleRate", &value);
        break;
    case audio_cfg::AudioOutputSampleRate:
        status = db->Get(read_options, "AudioOutputSampleRate", &value);
        break;
    }

    return QString::fromStdString(value);
}

/**
 * @brief DekodeDb::convertAudioChannelsEnum converts from an index given by
 * a QComboBox and not by a specific amount of channels.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_channel_sel The index as given by a QComboBox.
 * @return The relevant channel enumerator.
 */
audio_channels GkLevelDb::convertAudioChannelsEnum(const int &audio_channel_sel)
{
    audio_channels ret = Mono;
    switch (audio_channel_sel) {
        case 0:
            ret = audio_channels::Mono;
            break;
        case 1:
            ret = audio_channels::Left;
            break;
        case 2:
            ret = audio_channels::Right;
            break;
        case 3:
            ret = audio_channels::Both;
            break;
        case 4:
            ret = audio_channels::Surround;
            break;
        default:
            ret = audio_channels::Unknown;
            break;
    }

    return ret;
}

/**
 * @brief GkLevelDb::convertAudioChannelsStr converts the audio channel count enumerator to its related QString reference.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param channel_enum The channel count enumerator.
 * @return The related QString for the given channel count enumerator.
 */
QString GkLevelDb::convertAudioChannelsStr(const audio_channels &channel_enum)
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
bool GkLevelDb::convertAudioEnumIsStereo(const audio_channels &channel_enum) const
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
int GkLevelDb::boolInt(const bool &is_true)
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
 * @brief GkLevelDb::processCsvToDB takes pre-existing data and appends new values towards it, in the form of CSV. It then returns
 * all the newly made data as a CSV string. Data modifications to existing rows can be made too, which are automatically detected.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param comma_sep_values The pre-existing CSV information.
 * @param data_to_append The information to append onto the pre-existing CSV information, if any.
 * @return The newly made data, with the appended information, as a CSV string.
 */
std::string GkLevelDb::processCsvToDB(const std::string &comma_sep_values, const std::string &data_to_append)
{
    try {
        if (!data_to_append.empty()) {
            std::stringstream sstream;

            if (!comma_sep_values.empty()) {
                sstream << comma_sep_values << std::endl;
            } else {
                sstream << "FreqValues" << std::endl;
            }

            sstream << data_to_append;
            return sstream.str();
        } else {
            return "";
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
    try {
        if (!comma_sep_values.empty() && !data_to_remove.empty()) {
            std::stringstream sstream(comma_sep_values);
            rapidcsv::Document doc(sstream, rapidcsv::LabelParams(0, 0));
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

/**
 * @brief GkLevelDb::randomString
 * @author Carl <https://stackoverflow.com/a/12468109>
 * @return
 */
std::string GkLevelDb::randomString(const size_t &length)
{
    auto randchar = []() -> char {
        const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };

    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}
