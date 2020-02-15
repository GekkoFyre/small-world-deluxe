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

#pragma once

#include "src/defines.hpp"
#include "src/radiolibs.hpp"
#include "src/dek_db.hpp"
#include "src/audio_devices.hpp"
#include <boost/logic/tribool.hpp>
#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <QSharedPointer>
#include <QDialog>
#include <QString>
#include <QMap>
#include <QVector>
#include <QMultiMap>
#include <QComboBox>

namespace Ui {
class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(std::shared_ptr<GekkoFyre::GkLevelDb> dkDb,
                            std::shared_ptr<GekkoFyre::FileIo> filePtr,
                            std::shared_ptr<GekkoFyre::AudioDevices> audioDevices,
                            std::shared_ptr<GekkoFyre::RadioLibs> radioPtr,
                            QWidget *parent = nullptr);
    ~DialogSettings();

private slots:
    void on_pushButton_submit_config_clicked();
    void on_pushButton_cancel_config_clicked();
    void on_comboBox_brand_selection_currentIndexChanged(const QString &arg1);
    void on_comboBox_com_port_currentIndexChanged(int index);
    void on_pushButton_db_save_loc_clicked();
    void on_pushButton_audio_save_loc_clicked();
    void on_pushButton_input_sound_test_clicked();
    void on_pushButton_output_sound_test_clicked();
    void on_comboBox_soundcard_input_currentIndexChanged(int index);
    void on_comboBox_soundcard_output_currentIndexChanged(int index);

private:
    Ui::DialogSettings *ui;

    portaudio::AutoSystem autoSys;
    portaudio::System *gkPortAudioInit;

    std::shared_ptr<GekkoFyre::RadioLibs> gkRadioLibs;
    std::shared_ptr<GekkoFyre::GkLevelDb> gkDekodeDb;
    std::shared_ptr<GekkoFyre::FileIo> gkFileIo;
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDevices;
    static QComboBox *rig_comboBox;
    static QComboBox *mfg_comboBox;
    static QMultiMap<rig_model_t, std::tuple<QString, QString, GekkoFyre::AmateurRadio::rig_type>> radio_model_names; // Values: MFG, Model, Rig Type.
    static QVector<QString> unique_mfgs;

    // A QMap where the COM/Serial port name itself is the key and the value is the Target Path plus a
    // Boost C++ triboolean that signifies whether the port is active or not
    QMap<tstring, std::pair<tstring, boost::tribool>> status_com_ports;
    std::vector<GekkoFyre::Database::Settings::UsbPort> status_usb_devices;

    // First value is the Target Path while the second is the currentIndex within the QComboBox
    QMap<tstring, int> available_com_ports; // For tracking the *available* Device Ports (i.e. COM/Serial/RS232/USB) that the user can choose from...

    // The key corresponds to the position within the QComboBoxes
    QMap<int, GekkoFyre::Database::Settings::Audio::GkDevice> avail_input_audio_devs;
    QMap<int, GekkoFyre::Database::Settings::Audio::GkDevice> avail_output_audio_devs;
    GekkoFyre::Database::Settings::Audio::GkDevice chosen_input_audio_dev;
    GekkoFyre::Database::Settings::Audio::GkDevice chosen_output_audio_dev;

    static int prefill_rig_selection(const rig_caps *caps, void *data);
    static QMultiMap<rig_model_t, std::tuple<QString, QString, GekkoFyre::AmateurRadio::rig_type>> init_model_names();

    void prefill_audio_devices(std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> audio_devices_vec);

    QMap<int, int> collectComboBoxIndexes(const QComboBox *combo_box);
    void prefill_avail_com_ports(const QMap<tstring, std::pair<tstring, boost::tribool>> &com_ports);
    void prefill_avail_usb_ports(const std::vector<GekkoFyre::Database::Settings::UsbPort> usb_devices);
    void prefill_com_baud_speed(const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate);
    void enable_device_port_options(const bool &enable);
    void get_device_port_details(const tstring &port, const tstring &device,
                                 const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate = GekkoFyre::AmateurRadio::com_baud_rates::BAUD9600);
    bool read_settings();
};
