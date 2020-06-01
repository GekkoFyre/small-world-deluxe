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

#include "dek_db.hpp"
#include "src/audio_devices.hpp"
#include <leveldb/cache.h>
#include <leveldb/options.h>
#include <leveldb/slice.h>
#include <leveldb/table.h>
#include <leveldb/write_batch.h>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <QMessageBox>
#include <sstream>
#include <utility>
#include <iterator>
#include <vector>
#include <ctime>
#include <QDebug>
#include <QVariant>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

namespace fs = boost::filesystem;
namespace sys = boost::system;

GkLevelDb::GkLevelDb(leveldb::DB *db_ptr, std::shared_ptr<FileIo> filePtr, QObject *parent) : QObject(parent)
{
    db = db_ptr;
    fileIo = std::move(filePtr);
}

GkLevelDb::~GkLevelDb()
{}

/**
 * @brief DekodeDb::write_rig_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
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
        case radio_cfg::ComDeviceCat:
            batch.Put("ComDeviceCat", value.toStdString());
            break;
        case radio_cfg::ComDevicePtt:
            batch.Put("ComDevicePtt", value.toStdString());
            break;
        case radio_cfg::ComBaudRate:
            batch.Put("ComBaudRate", value.toStdString());
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
 * @brief DekodeDb::write_audio_device_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 * @param is_output_device
 */
void GkLevelDb::write_audio_device_settings(const GkDevice &value, const QString &key, const bool &is_output_device)
{
    try {
        leveldb::WriteBatch batch;
        leveldb::Status status;

        if (is_output_device) {
            // Unique identifier for the chosen output audio device
            batch.Put("AudioOutputId", std::to_string(value.stream_parameters.device));
            batch.Put("AudioOutputDefSampleRate", std::to_string(value.def_sample_rate));
            batch.Put("AudioOutputChannelCount", std::to_string(value.dev_output_channel_count));
            batch.Put("AudioOutputSelChannels", std::to_string(value.sel_channels));
            batch.Put("AudioOutputDevName", value.device_info.name);

            // Determine if this is the default output device for the system and if so, convert
            // the boolean value to a std::string suitable for database storage.
            std::string is_default = boolEnum(value.default_dev);
            batch.Put("AudioOutputDefSysDevice", is_default);
        } else {
            // Unique identifier for the chosen input audio device
            batch.Put("AudioInputId", std::to_string(value.stream_parameters.device));
            batch.Put("AudioInputDefSampleRate", std::to_string(value.def_sample_rate));
            batch.Put("AudioInputChannelCount", std::to_string(value.dev_input_channel_count));
            batch.Put("AudioInputSelChannels", std::to_string(value.sel_channels));
            batch.Put("AudioInputDevName", value.device_info.name);

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
 * @brief DekodeDb::read_rig_settings
 * @param key
 * @return
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
    case radio_cfg::ComDeviceCat:
        status = db->Get(read_options, "ComDeviceCat", &value);
        break;
    case radio_cfg::ComDevicePtt:
        status = db->Get(read_options, "ComDevicePtt", &value);
        break;
    case radio_cfg::ComBaudRate:
        status = db->Get(read_options, "ComBaudRate", &value);
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
    default:
        return "";
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
int GkLevelDb::read_audio_device_settings(const bool &is_output_device)
{
    std::mutex read_audio_dev_mtx;
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value;

    std::lock_guard<std::mutex> lck_guard(read_audio_dev_mtx);
    read_options.verify_checksums = true;

    if (is_output_device) {
        // We are dealing with an audio output device
        status = db->Get(read_options, "AudioOutputId", &value);
    } else {
        // We are dealing with an audio input device
        status = db->Get(read_options, "AudioInputId", &value);
    }

    int ret_val = 0;
    if (!value.empty()) {
            ret_val = std::stoi(value);
    } else {
        ret_val = -1;
    }

    return ret_val;
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
    std::mutex read_audio_api_mtx;
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
        std::string output_def_sample_rate;
        std::string output_channel_count;
        std::string output_sel_channels;
        std::string output_def_sys_device;

        status = db->Get(read_options, "AudioOutputId", &output_id);
        status = db->Get(read_options, "AudioOutputDefSampleRate", &output_def_sample_rate);
        status = db->Get(read_options, "AudioOutputChannelCount", &output_channel_count);
        status = db->Get(read_options, "AudioOutputSelChannels", &output_sel_channels);
        status = db->Get(read_options, "AudioOutputDefSysDevice", &output_def_sys_device);

        bool def_sys_device = boolStr(output_def_sys_device);

        //
        // Test to see if the following are empty or not
        //
        if (!output_id.empty()) {
            audio_device.dev_number = std::stoi(output_id);
        } else {
            audio_device.dev_number = -1;
        }

        if (!output_def_sample_rate.empty()) {
            audio_device.def_sample_rate = std::stod(output_def_sample_rate);
        } else {
            audio_device.def_sample_rate = 0.0;
        }

        if (!output_channel_count.empty()) {
            audio_device.dev_output_channel_count = std::stoi(output_channel_count);
            audio_device.dev_input_channel_count = 0;
        } else {
            audio_device.dev_output_channel_count = 0;
            audio_device.dev_input_channel_count = 0;
        }

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
        std::string input_def_sample_rate;
        std::string input_channel_count;
        std::string input_sel_channels;
        std::string input_def_sys_device;

        status = db->Get(read_options, "AudioInputId", &input_id);
        status = db->Get(read_options, "AudioInputDefSampleRate", &input_def_sample_rate);
        status = db->Get(read_options, "AudioInputChannelCount", &input_channel_count);
        status = db->Get(read_options, "AudioInputSelChannels", &input_sel_channels);
        status = db->Get(read_options, "AudioInputDefSysDevice", &input_def_sys_device);

        bool def_sys_device = boolStr(input_def_sys_device);

        //
        // Test to see if the following are empty or not
        //
        if (!input_id.empty()) {
            audio_device.dev_number = std::stoi(input_id);
        } else {
            audio_device.dev_number = -1;
        }

        if (!input_def_sample_rate.empty()) {
            audio_device.def_sample_rate = std::stod(input_def_sample_rate);
        } else {
            audio_device.def_sample_rate = 0.0;
        }

        if (!input_channel_count.empty()) {
            audio_device.dev_input_channel_count = std::stoi(input_channel_count);
            audio_device.dev_output_channel_count = 0;
        } else {
            audio_device.dev_input_channel_count = 0;
            audio_device.dev_output_channel_count = 0;
        }

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
        ret = Mono;
        break;
    case 1:
        ret = Left;
        break;
    case 2:
        ret = Right;
        break;
    case 3:
        ret = Both;
        break;
    default:
        ret = Mono;
        break;
    }

    return ret;
}

/**
 * @brief GkLevelDb::convertAudioChannelsInt converts from an enumerator to the
 * given amounts of *actual* audio channels, as an integer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param channel_enum The channels as an enumerator, which itself is calculated by
 * a given QComboBox index.
 * @return The amount of channels, given as an integer.
 */
int GkLevelDb::convertAudioChannelsInt(const audio_channels &channel_enum) const
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
    default:
        return 1;
    }

    return -1;
}

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
    // TODO: Maybe convert to a Boost C++ tribool?

    bool ret = false;
    if (std::strcmp(is_true.c_str(), "true")) {
        ret = true;
    } else {
        ret = false;
    }

    return ret;
}
