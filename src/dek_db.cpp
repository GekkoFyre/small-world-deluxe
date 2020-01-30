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
 **   Copyright (C) 2019. GekkoFyre.
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
 **   [ 1 ] - https://git.gekkofyre.io/amateur-radio/small-world-deluxe
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
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <cstring>
#include <QDebug>
#include <QMessageBox>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
namespace fs = boost::filesystem;

DekodeDb::DekodeDb(leveldb::DB *db_ptr, std::shared_ptr<FileIo> filePtr, QObject *parent) : QObject(parent)
{
    db = db_ptr;
    fileIo = filePtr;
}

DekodeDb::~DekodeDb()
{}

/**
 * @brief DekodeDb::write_rig_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void DekodeDb::write_rig_settings(const QString &value, const Database::Settings::radio_cfg &key)
{
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
    case radio_cfg::RigVersion:
        batch.Put("RigVersion", value.toStdString());
        break;
    case radio_cfg::ComDevice:
        batch.Put("ComDevice", value.toStdString());
        break;
    case radio_cfg::ComBaudRate:
        batch.Put("ComBaudRate", value.toStdString());
        break;
    case radio_cfg::StopBits:
        batch.Put("StopBits", value.toStdString());
        break;
    case radio_cfg::PollingInterval:
        batch.Put("PollingInterval", value.toStdString());
        break;
    case radio_cfg::ModeDelay:
        batch.Put("ModeDelay", value.toStdString());
        break;
    case radio_cfg::Sideband:
        batch.Put("Sideband", value.toStdString());
        break;
    case radio_cfg::CWisLSB:
        batch.Put("CWisLSB", value.toStdString());
        break;
    case radio_cfg::FlowControl:
        batch.Put("FlowControl", value.toStdString());
        break;
    case radio_cfg::PTTCommand:
        batch.Put("PTTCommand", value.toStdString());
        break;
    case radio_cfg::Retries:
        batch.Put("Retries", value.toStdString());
        break;
    case radio_cfg::RetryInterv:
        batch.Put("RetryInterv", value.toStdString());
        break;
    case radio_cfg::WriteDelay:
        batch.Put("WriteDelay", value.toStdString());
        break;
    case radio_cfg::PostWriteDelay:
        batch.Put("PostWriteDelay", value.toStdString());
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

    return;
}

/**
 * @brief DekodeDb::write_audio_device_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 * @param is_output_device
 */
void DekodeDb::write_audio_device_settings(const Device &value, const QString &key, const bool &is_output_device)
{
    leveldb::WriteBatch batch;
    leveldb::Status status;

    if (is_output_device) {
        // Unique identifier for the chosen output audio device
        batch.Put("AudioOutputId", std::to_string(value.dev_number));
        batch.Put("AudioOutputDefSampleRate", std::to_string(value.def_sample_rate));
        batch.Put("AudioOutputChannelCount", std::to_string(value.dev_output_channel_count));
        batch.Put("AudioOutputSelChannels", std::to_string(value.sel_channels));
        batch.Put("AudioOutputDevName", value.device_info->name);

        // Determine if this is the default output device for the system and if so, convert
        // the boolean value to a std::string suitable for database storage.
        std::string is_default = boolEnum(value.default_dev);
        batch.Put("AudioOutputDefSysDevice", is_default);
    } else {
        // Unique identifier for the chosen input audio device
        batch.Put("AudioInputId", std::to_string(value.dev_number));
        batch.Put("AudioInputDefSampleRate", std::to_string(value.def_sample_rate));
        batch.Put("AudioInputChannelCount", std::to_string(value.dev_input_channel_count));
        batch.Put("AudioInputSelChannels", std::to_string(value.sel_channels));
        batch.Put("AudioInputDevName", value.device_info->name);

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

    return;
}

/**
 * @brief DekodeDb::read_rig_settings
 * @param key
 * @return
 */
QString DekodeDb::read_rig_settings(const Database::Settings::radio_cfg &key)
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
    case radio_cfg::RigVersion:
        status = db->Get(read_options, "RigVersion", &value);
        break;
    case radio_cfg::ComDevice:
        status = db->Get(read_options, "ComDevice", &value);
        break;
    case radio_cfg::ComBaudRate:
        status = db->Get(read_options, "ComBaudRate", &value);
        break;
    case radio_cfg::StopBits:
        status = db->Get(read_options, "StopBits", &value);
        break;
    case radio_cfg::PollingInterval:
        status = db->Get(read_options, "PollingInterval", &value);
        break;
    case radio_cfg::ModeDelay:
        status = db->Get(read_options, "ModeDelay", &value);
        break;
    case radio_cfg::Sideband:
        status = db->Get(read_options, "Sideband", &value);
        break;
    case radio_cfg::CWisLSB:
        status = db->Get(read_options, "CWisLSB", &value);
        break;
    case radio_cfg::FlowControl:
        status = db->Get(read_options, "FlowControl", &value);
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
int DekodeDb::read_audio_device_settings(const bool &is_output_device)
{
    leveldb::Status status;
    leveldb::ReadOptions read_options;
    std::string value;

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
    }

    return ret_val;
}

/**
 * @brief DekodeDb::read_audio_details_settings Reads all the settings concerning Audio Devices from the
 * Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_output_device Whether we are dealing with an output or input audio device.
 * @return The struct, `GekkoFyre::Database::Settings::Audio::Device`.
 */
Device DekodeDb::read_audio_details_settings(const bool &is_output_device)
{
    Device audio_device;
    leveldb::Status status;
    leveldb::ReadOptions read_options;

    read_options.verify_checksums = true;

    if (is_output_device) {
        // Output audio device
        std::string output_id;
        std::string output_def_sample_rate;
        std::string output_channel_count;
        std::string output_sel_channels;
        // std::string output_dev_name;
        std::string output_def_sys_device;

        status = db->Get(read_options, "AudioOutputId", &output_id);
        status = db->Get(read_options, "AudioOutputDefSampleRate", &output_def_sample_rate);
        status = db->Get(read_options, "AudioOutputChannelCount", &output_channel_count);
        status = db->Get(read_options, "AudioOutputSelChannels", &output_sel_channels);
        // status = db->Get(read_options, "AudioOutputDevName", &output_dev_name);
        status = db->Get(read_options, "AudioOutputDefSysDevice", &output_def_sys_device);

        bool def_sys_device = boolStr(output_def_sys_device);

        audio_device.dev_number = std::stoi(output_id);
        audio_device.def_sample_rate = std::stod(output_def_sample_rate);
        audio_device.dev_output_channel_count = std::stoi(output_channel_count);
        audio_device.sel_channels = convertAudioChannelsInt(std::stoi(output_sel_channels));
        audio_device.default_dev = def_sys_device;
        // audio_device.device_info->name = output_dev_name;
    } else {
        // Input audio device
        std::string input_id;
        std::string input_def_sample_rate;
        std::string input_channel_count;
        std::string input_sel_channels;
        // std::string input_dev_name;
        std::string input_def_sys_device;

        status = db->Get(read_options, "AudioInputId", &input_id);
        status = db->Get(read_options, "AudioInputDefSampleRate", &input_def_sample_rate);
        status = db->Get(read_options, "AudioInputChannelCount", &input_channel_count);
        status = db->Get(read_options, "AudioInputSelChannels", &input_sel_channels);
        // status = db->Get(read_options, "AudioInputDevName", &input_dev_name);
        status = db->Get(read_options, "AudioInputDefSysDevice", &input_def_sys_device);

        bool def_sys_device = boolStr(input_def_sys_device);

        audio_device.dev_number = std::stoi(input_id);
        audio_device.def_sample_rate = std::stod(input_def_sample_rate);
        audio_device.dev_output_channel_count = std::stoi(input_channel_count);
        audio_device.sel_channels = convertAudioChannelsInt(std::stoi(input_sel_channels));
        audio_device.default_dev = def_sys_device;
        // audio_device.device_info->name = input_dev_name;
    }

    return audio_device;
}

/**
 * @brief DekodeDb::convertAudioChannelsInt
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_channel_sel
 * @return
 */
audio_channels DekodeDb::convertAudioChannelsInt(const int &audio_channel_sel)
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
 * @brief DekodeDb::boolEnum Will enumerate a boolean value to an std::string, ready for use within a database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_true Whether we are dealing with a true or false situation.
 * @return The enumerated boolean value.
 */
std::string DekodeDb::boolEnum(const bool &is_true)
{
    switch (is_true) {
    case true:
        return "true";
    case false:
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
bool DekodeDb::boolStr(const std::string &is_true)
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