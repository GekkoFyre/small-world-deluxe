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

#include "dialogsettings.hpp"
#include "ui_dialogsettings.h"
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <QStringList>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTableWidgetItem>
#include <utility>
#include <exception>
#include <set>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

namespace fs = boost::filesystem;
namespace sys = boost::system;

QComboBox *DialogSettings::rig_comboBox = nullptr;
QComboBox *DialogSettings::mfg_comboBox = nullptr;
QMultiMap<rig_model_t, std::tuple<QString, QString, GekkoFyre::AmateurRadio::rig_type>> DialogSettings::radio_model_names = init_model_names();
QVector<QString> DialogSettings::unique_mfgs = { "None" };

/**
 * @brief DialogSettings::DialogSettings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dkDb
 * @param filePtr
 * @param parent
 */
DialogSettings::DialogSettings(std::shared_ptr<GkLevelDb> dkDb,
                               std::shared_ptr<GekkoFyre::FileIo> filePtr,
                               std::shared_ptr<GekkoFyre::AudioDevices> audioDevices,
                               QPointer<GekkoFyre::RadioLibs> radioLibs,
                               std::shared_ptr<QSettings> settings,
                               portaudio::System *portAudioInit,
                               libusb_context *usb_lib_ctx,
                               std::shared_ptr<AmateurRadio::Control::GkRadio> radioPtr,
                               QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

    try {
        //
        // Initialize PortAudio for Settings Dialog!
        //
        gkPortAudioInit = std::move(portAudioInit);
        gkRadioLibs = std::move(radioLibs);
        gkDekodeDb = std::move(dkDb);
        gkFileIo = std::move(filePtr);
        gkAudioDevices = std::move(audioDevices);
        usb_ctx_ptr = std::move(usb_lib_ctx);
        gkRadioPtr = std::move(radioPtr);

        gkSettings = settings;
        usb_ports_active = false;
        com_ports_active = false;

        QObject::connect(this, SIGNAL(usbPortsDisabled(const bool &)), this, SLOT(disableUsbPorts(const bool &)));
        QObject::connect(this, SIGNAL(comPortsDisabled(const bool &)), this, SLOT(disableComPorts(const bool &)));

        rig_comboBox = ui->comboBox_rig_selection;
        mfg_comboBox = ui->comboBox_brand_selection;

        // Initialize the list of amateur radio transceiver models and associated manufacturers
        rig_list_foreach(prefill_rig_selection, nullptr);

        //
        // Lists all the amateur radio receivers, transmitters and transceivers by manufacturer
        //
        for (const auto &mfg: unique_mfgs) {
            mfg_comboBox->addItem(mfg);
        }

        Q_ASSERT(gkRadioLibs != nullptr);

        // Detect available COM ports on either Microsoft Windows, Linux, or even Apple Mac OS/X
        // Also detect the available USB devices of the audial type
        // NOTE: There are two functions, one each for COM/Serial/RS-232 ports and another for USB
        // devices separately. This is because within the GekkoFyre::RadioLibs namespace, there are
        // also two separate functions for enumerating out these ports!
        prefill_rig_force_ctrl_lines(ptt_type_t::RIG_PTT_SERIAL_DTR);
        prefill_rig_force_ctrl_lines(ptt_type_t::RIG_PTT_SERIAL_RTS);
        status_com_ports = gkRadioLibs->status_com_ports();
        status_usb_devices = gkRadioLibs->enumUsbDevices(usb_ctx_ptr);
        prefill_avail_com_ports(status_com_ports);
        prefill_avail_usb_ports(status_usb_devices);

        if (!status_com_ports.empty()) {
            // Select the initial COM port and load it into memory
            on_comboBox_com_port_currentIndexChanged(ui->comboBox_com_port->currentIndex());
            on_comboBox_ptt_method_port_currentIndexChanged(ui->comboBox_ptt_method_port->currentIndex());
        }

        //
        // Initialize PortAudio libraries!
        //
        std::vector<GkDevice> audio_devices = gkAudioDevices->filterPortAudioHostType(gkAudioDevices->enumAudioDevicesCpp(gkPortAudioInit));
        QVector<PaHostApiTypeId> portaudio_api_avail = gkAudioDevices->portAudioApiChooser(audio_devices);
        prefill_audio_api_avail(portaudio_api_avail);
        prefill_audio_devices(audio_devices);

        ui->label_pa_version->setText(gkAudioDevices->portAudioVersionNumber(*gkPortAudioInit));
        ui->textEdit_pa_version_text->setText(gkAudioDevices->portAudioVersionText(*gkPortAudioInit));

        prefill_com_baud_speed(GekkoFyre::AmateurRadio::com_baud_rates::BAUD1200);
        prefill_com_baud_speed(GekkoFyre::AmateurRadio::com_baud_rates::BAUD2400);
        prefill_com_baud_speed(GekkoFyre::AmateurRadio::com_baud_rates::BAUD4800);
        prefill_com_baud_speed(GekkoFyre::AmateurRadio::com_baud_rates::BAUD9600);
        prefill_com_baud_speed(GekkoFyre::AmateurRadio::com_baud_rates::BAUD19200);
        prefill_com_baud_speed(GekkoFyre::AmateurRadio::com_baud_rates::BAUD38400);
        prefill_com_baud_speed(GekkoFyre::AmateurRadio::com_baud_rates::BAUD57600);
        prefill_com_baud_speed(GekkoFyre::AmateurRadio::com_baud_rates::BAUD115200);

        init_working_freqs();
        init_station_info();

        if (gkDekodeDb.get() != nullptr) {
            read_settings();
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(this, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }
}

/**
 * @brief DialogSettings::~DialogSettings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
DialogSettings::~DialogSettings()
{
    delete rig_comboBox;
    delete mfg_comboBox;

    delete ui;
}

/**
 * @brief DialogSettings::on_pushButton_submit_config_clicked will save the values within the Setting's Dialog to a Google LevelDB
 * database and ensure everything is okay, filtered properly, validated, etc. within itself and further on functions.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_submit_config_clicked()
{
    try {
        //
        // Rig Settings
        //
        unsigned short brand = ui->comboBox_brand_selection->currentIndex();
        QVariant sel_rig = ui->comboBox_rig_selection->currentData();
        unsigned short sel_rig_index = ui->comboBox_rig_selection->currentIndex();
        unsigned short com_device_cat = ui->comboBox_com_port->currentIndex();
        unsigned short com_device_ptt = ui->comboBox_ptt_method_port->currentIndex();
        QString usb_device_cat = ui->comboBox_com_port->currentData().toString();
        QString usb_device_ptt = ui->comboBox_ptt_method_port->currentData().toString();
        int com_baud_rate = ui->comboBox_baud_rate->currentIndex();
        QString ptt_adv_cmd = ui->lineEdit_adv_ptt_cmd->text();

        //
        // Chosen PortAudio API
        //
        int chosen_pa_api = ui->comboBox_soundcard_api->currentData().toInt();

        //
        // Audio Device Channels (i.e. Mono/Stereo/etc)
        //
        int curr_input_device_channels = ui->comboBox_soundcard_input->currentData().toInt();
        int curr_output_device_channels = ui->comboBox_soundcard_output->currentData().toInt();

        //
        // RS-232 Settings
        //
        tstring chosen_com_port_cat;
        tstring chosen_com_port_ptt;
        for (const auto &sel_port: available_com_ports.toStdMap()) {
            for (const auto &avail_port: status_com_ports.toStdMap()) {
                // List out the current serial ports in use
                if (com_device_cat == sel_port.second) {
                    if (sel_port.first == avail_port.second.first) {
                        //
                        // CAT Control
                        //
                        chosen_com_port_cat = sel_port.first; // The chosen serial device uses its own name as a reference
                        break;
                    }
                }

                if (com_device_ptt == sel_port.second) {
                    if (sel_port.first == avail_port.second.first) {
                        //
                        // PTT Method
                        //
                        chosen_com_port_ptt = sel_port.first; // The chosen serial device uses its own name as a reference
                        break;
                    }
                }
            }
        }

        //
        // USB Device settings
        //
        tstring chosen_usb_port_cat;
        tstring chosen_usb_port_ptt;
        for (const auto &sel_usb_port: available_usb_ports.toStdMap()) { // The key is the Port Number whilst the value is what's displayed within the QComboBox...
            for (const auto &avail_usb_port: status_usb_devices.toStdMap()) {
                // List out the current USB devices in use
                if (usb_device_cat == sel_usb_port.first) {
                    if (QString::fromStdString(avail_usb_port.first) == sel_usb_port.first) {
                        chosen_usb_port_cat = avail_usb_port.first;
                        chosen_usb_port_ptt = avail_usb_port.first;
                        break;
                    }
                }
            }
        }

        //
        // Operating System's own Settings (e.g. the registry under Microsoft Windows)
        //
        gkFileIo->write_initial_settings(Filesystem::fileName, init_cfg::DbName);
        gkFileIo->write_initial_settings(ui->lineEdit_db_save_loc->text(), init_cfg::DbLoc);

        //
        // PortAudio API
        //
        for (const auto &pa_api_idx: avail_portaudio_api.toStdMap()) {
            if (pa_api_idx.first == chosen_pa_api) {
                gkDekodeDb->write_audio_api_settings(pa_api_idx.second);
                break;
            }
        }

        //
        // Input Device
        //
        chosen_input_audio_dev.sel_channels = gkDekodeDb->convertAudioChannelsEnum(curr_input_device_channels);
        gkDekodeDb->write_audio_device_settings(chosen_input_audio_dev, QString::number(chosen_input_audio_dev.dev_number), false);

        //
        // Output Device
        //
        chosen_output_audio_dev.sel_channels = gkDekodeDb->convertAudioChannelsEnum(curr_output_device_channels);
        gkDekodeDb->write_audio_device_settings(chosen_output_audio_dev, QString::number(chosen_output_audio_dev.dev_number), true);

        //
        // Data Bits
        //
        short enum_data_bits = 0;
        if (ui->radioButton_data_bits_default->isDown()) {
            // Default
            enum_data_bits = 0;
        } else if (ui->radioButton_data_bits_seven->isDown()) {
            // Seven (7)
            enum_data_bits = 1;
        } else {
            // Eight (8)
            enum_data_bits = 2;
        }

        //
        // Stop Bits
        //
        short enum_stop_bits = 0;
        if (ui->radioButton_stop_bits_default->isDown()) {
            // Default
            enum_stop_bits = 0;
        } else if (ui->radioButton_stop_bits_one->isDown()) {
            // One (1)
            enum_stop_bits = 1;
        } else {
            // Two (2)
            enum_stop_bits = 2;
        }

        //
        // Handshake
        //
        short enum_handshake = 0;
        if (ui->radioButton_handshake_default->isDown()) {
            // Default
            enum_handshake = 0;
        } else if (ui->radioButton_handshake_none->isDown()) {
            // None
            enum_handshake = 1;
        } else if (ui->radioButton_handshake_xon_xoff->isDown()) {
            // XON / XOFF
            enum_handshake = 2;
        } else {
            // Hardware
            enum_handshake = 3;
        }

        //
        // PTT Method
        //
        short enum_ptt_method = 0;
        if (ui->radioButton_ptt_method_vox->isDown()) {
            // VOX
            enum_ptt_method = 0;
        } else if (ui->radioButton_ptt_method_dtr->isDown()) {
            // DTR
            enum_ptt_method = 1;
        } else if (ui->radioButton_ptt_method_cat->isDown()) {
            // CAT
            enum_ptt_method = 2;
        } else {
            // RTS
            enum_ptt_method = 3;
        }

        //
        // Transmit Audio Source
        //
        short enum_tx_audio_src = 0;
        if (ui->radioButton_tx_audio_src_rear_data->isDown()) {
            // Rear / Data
            enum_tx_audio_src = 0;
        } else {
            // Front / Mic
            enum_tx_audio_src = 1;
        }

        //
        // Mode
        //
        short enum_mode = 0;
        if (ui->radioButton_mode_none->isDown()) {
            // None
            enum_mode = 0;
        } else if (ui->radioButton_mode_usb->isDown()) {
            // USB
            enum_mode = 1;
        } else {
            // Data / PKT
            enum_mode = 2;
        }

        //
        // Split Operation
        //
        short enum_split_oper = 0;
        if (ui->radioButton_split_none->isDown()) {
            // None
            enum_split_oper = 0;
        } else if (ui->radioButton_split_rig->isDown()) {
            // Rig
            enum_split_oper = 1;
        } else {
            // Fake It
            enum_split_oper = 2;
        }

        short enum_force_ctrl_lines_dtr = 0;
        short enum_force_ctrl_lines_rts = 0;
        enum_force_ctrl_lines_dtr = ui->comboBox_force_ctrl_lines_dtr->currentIndex();
        enum_force_ctrl_lines_rts = ui->comboBox_force_ctrl_lines_rts->currentIndex();

        #ifdef _UNICODE
        gkDekodeDb->write_rig_settings(QString::fromStdWString(chosen_com_port_cat), radio_cfg::ComDeviceCat);
        gkDekodeDb->write_rig_settings(QString::fromStdWString(chosen_com_port_ptt), radio_cfg::ComDevicePtt);
        #else
        gkDekodeDb->write_rig_settings(QString::fromStdString(chosen_com_port_cat), radio_cfg::ComDeviceCat);
        gkDekodeDb->write_rig_settings(QString::fromStdString(chosen_com_port_ptt), radio_cfg::ComDevicePtt);
        gkDekodeDb->write_rig_settings(QString::fromStdString(chosen_usb_port_cat), radio_cfg::UsbDeviceCat);
        gkDekodeDb->write_rig_settings(QString::fromStdString(chosen_usb_port_ptt), radio_cfg::UsbDevicePtt);
        #endif

        using namespace Database::Settings;
        gkDekodeDb->write_rig_settings(QString::number(brand), radio_cfg::RigBrand);
        gkDekodeDb->write_rig_settings(QString::number(sel_rig.toInt()), radio_cfg::RigModel);
        gkDekodeDb->write_rig_settings(QString::number(sel_rig_index), radio_cfg::RigModelIndex);
        gkDekodeDb->write_rig_settings(QString::number(com_baud_rate), radio_cfg::ComBaudRate);
        gkDekodeDb->write_rig_settings(QString::number(enum_stop_bits), radio_cfg::StopBits);
        gkDekodeDb->write_rig_settings(QString::number(enum_data_bits), radio_cfg::DataBits);
        gkDekodeDb->write_rig_settings(QString::number(enum_handshake), radio_cfg::Handshake);
        gkDekodeDb->write_rig_settings(QString::number(enum_force_ctrl_lines_dtr), radio_cfg::ForceCtrlLinesDtr);
        gkDekodeDb->write_rig_settings(QString::number(enum_force_ctrl_lines_rts), radio_cfg::ForceCtrlLinesRts);
        gkDekodeDb->write_rig_settings(QString::number(enum_ptt_method), radio_cfg::PTTMethod);
        gkDekodeDb->write_rig_settings(QString::number(enum_tx_audio_src), radio_cfg::TXAudioSrc);
        gkDekodeDb->write_rig_settings(QString::number(enum_mode), radio_cfg::PTTMode);
        gkDekodeDb->write_rig_settings(QString::number(enum_split_oper), radio_cfg::SplitOperation);
        gkDekodeDb->write_rig_settings(ptt_adv_cmd, radio_cfg::PTTAdvCmd);

        QMessageBox::information(this, tr("Thank you"), tr("Your settings have been saved."), QMessageBox::Ok);
        this->close();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_cancel_config_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_cancel_config_clicked()
{
    this->close();
}

/**
 * @brief DialogSettings::prefill_rig_selection Prefills the setting's form with the right variables
 * from the Google LevelDB database, provided it exists.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param caps
 * @param data Don't remove this, it is required for some silly reason despite not being used in any form.
 * @return
 */
int DialogSettings::prefill_rig_selection(const rig_caps *caps, void *data)
{
    try {
        switch (caps->rig_type & RIG_TYPE_MASK) {
        case RIG_TYPE_TRANSCEIVER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Transceiver));
            break;
        case RIG_TYPE_HANDHELD:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Handheld));
            break;
        case RIG_TYPE_MOBILE:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Mobile));
            break;
        case RIG_TYPE_RECEIVER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Receiver));
            break;
        case RIG_TYPE_PCRECEIVER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::PC_Receiver));
            break;
        case RIG_TYPE_SCANNER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Scanner));
            break;
        case RIG_TYPE_TRUNKSCANNER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::TrunkingScanner));
            break;
        case RIG_TYPE_COMPUTER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Computer));
            break;
        case RIG_TYPE_OTHER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Other));
            break;
        default:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Unknown));
            break;
        }

        // Sort out the amateur radio transceiver manufacturers that are unique and remove any duplicates!
        for (const auto &kv: radio_model_names.toStdMap()) {
            QString value = std::get<1>(kv.second);
            if (!value.isEmpty()) {
                if (!unique_mfgs.contains(value)) {
                    unique_mfgs.push_back(value);
                } else {
                    continue;
                }
            }
        }

        // Now sort the items alphabetically!
        std::sort(unique_mfgs.begin(), unique_mfgs.end());
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    Q_UNUSED(data);
    return 1; /* !=0, we want them all! */
}

/**
 * @brief DialogSettings::init_model_names Initializes the static variable, `GekkoFyre::DialogSettings::radio_model_names`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The value to be used for initialization.
 */
QMultiMap<rig_model_t, std::tuple<QString, QString, AmateurRadio::rig_type>> DialogSettings::init_model_names()
{
    using namespace AmateurRadio;
    QMultiMap<rig_model_t, std::tuple<QString, QString, rig_type>> mmap;
    mmap.insert(-1, std::make_tuple("", "", GekkoFyre::AmateurRadio::rig_type::Unknown));
    return mmap;
}

/**
 * @brief DialogSettings::prefill_audio_api_avail Enumerates the available operating system's sound/multimedia APIs that
 * are available to the user via PortAudio, all dependent on how Small World Deluxe and its associated libraries
 * were compiled.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param portaudio_api The pre-sorted list of sound/multimedia APIs available to the user.
 * @see AudioDevices::portAudioApiChooser(), AudioDevices::filterPortAudioHostType().
 */
void DialogSettings::prefill_audio_api_avail(const QVector<PaHostApiTypeId> &portaudio_api_vec)
{
    try {
        // Garner the list of APIs!
        if (!portaudio_api_vec.isEmpty()) {
            for (const auto &pa_api: portaudio_api_vec) {
                QString api_str_tmp = gkDekodeDb->portAudioApiToStr(pa_api);
                int underlying_api_int = to_underlying(pa_api);
                ui->comboBox_soundcard_api->insertItem(underlying_api_int, api_str_tmp, underlying_api_int);
                avail_portaudio_api.insert(underlying_api_int, pa_api);
            }
        }

        if (!avail_portaudio_api.isEmpty()) {
            for (const auto &avail_api_idx: avail_portaudio_api.toStdMap()) {
                // Set the QComboBox for the available PortAudio APIs to the first accessible item!
                ui->comboBox_soundcard_api->setCurrentIndex(avail_api_idx.first);
                return;
            }
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(this, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::prefill_audio_devices Enumerates the audio deviecs on the user's
 * computer system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_devices The available audio devices on the user's system, as a typical std::vector.
 * @see GekkoFyre::AudioDevices::enumAudioDevices(), AudioDevices::filterPortAudioHostType().
 */
void DialogSettings::prefill_audio_devices(std::vector<GkDevice> audio_devices_vec)
{
    try {
        ui->comboBox_soundcard_input->clear();
        ui->comboBox_soundcard_output->clear();
        ui->comboBox_soundcard_input->insertItem(0, tr("No audio device selected"));
        ui->comboBox_soundcard_output->insertItem(0, tr("No audio device selected"));

        if (!avail_portaudio_api.isEmpty()) {
            if (!audio_devices_vec.empty()) {
                for (const auto &device: audio_devices_vec) {
                    if (device.device_info.hostApi >= 0) {
                        if (device.is_output_dev) {
                            //
                            // Audio device is an output
                            //
                            std::string audio_dev_name = device.device_info.name;
                            if (!audio_dev_name.empty()) {
                                PaDeviceIndex dev_idx = device.stream_parameters.device;
                                avail_output_audio_devs.insert(dev_idx, device);
                            }
                        } else {
                            //
                            // Audio device is an input
                            //
                            std::string audio_dev_name = device.device_info.name;
                            if (!audio_dev_name.empty()) {
                                PaDeviceIndex dev_idx = device.stream_parameters.device;
                                avail_input_audio_devs.insert(dev_idx, device);
                            }
                        }
                    } else {
                        throw std::runtime_error(tr("Given audio devices are nullptr! Small World Deluxe may not work correctly if you continue...").toStdString());
                    }
                }

                //
                // Update the input / output audio device for the QComboBoxes
                //
                on_comboBox_soundcard_api_currentIndexChanged();

                //
                // Verify that the user's chosen and saved PortAudio API actually exist given this new instance
                // of the Setting's Dialog! If so, set the current index of the QComboBox towards it.
                //
                PaHostApiTypeId api_identifier = gkDekodeDb->read_audio_api_settings();
                if (api_identifier != PaHostApiTypeId::paInDevelopment) {
                    int actual_api_idx = to_underlying(api_identifier);
                    int counter = 0;
                    for (const auto &pa_api_idx: avail_portaudio_api.toStdMap()) {
                        if (pa_api_idx.first == actual_api_idx) {
                            ui->comboBox_soundcard_api->setCurrentIndex(counter);
                            on_comboBox_soundcard_api_currentIndexChanged();
                            break;
                        }

                        ++counter;
                    }
                }

                //
                // Input audio device for PortAudio!
                //
                int input_identifier = gkDekodeDb->read_audio_device_settings(false);
                if (input_identifier > 0) {
                    GkDevice input_device = gkAudioDevices->gatherAudioDeviceDetails(gkPortAudioInit, input_identifier);
                    for (const auto &input_idx: avail_input_audio_devs.toStdMap()) {
                        if (input_idx.first == input_identifier) {
                            chosen_input_audio_dev = input_device;
                            ui->comboBox_soundcard_input->setCurrentIndex(input_identifier);
                            break;
                        }
                    }
                }

                //
                // Output audio device for PortAudio!
                //
                int output_identifier = gkDekodeDb->read_audio_device_settings(true);
                if (output_identifier > 0) {
                    GkDevice output_device = gkAudioDevices->gatherAudioDeviceDetails(gkPortAudioInit, output_identifier);
                    for (const auto &output_idx: avail_output_audio_devs.toStdMap()) {
                        if (output_idx.first == output_identifier) {
                            chosen_output_audio_dev = output_device;
                            ui->comboBox_soundcard_output->setCurrentIndex(output_identifier);
                            break;
                        }
                    }
                }
            } else {
                //
                // No PortAudio multimedia devices were detected!
                //
                ui->comboBox_soundcard_input->clear();
                ui->comboBox_soundcard_output->clear();

                ui->comboBox_soundcard_input->insertItem(-1, tr("No available multimedia devices were detected!"), -1);
                ui->comboBox_soundcard_output->insertItem(-1, tr("No available multimedia devices were detected!"), -1);

                ui->comboBox_soundcard_input->setEnabled(false);
                ui->comboBox_soundcard_output->setEnabled(false);

                return;
            }
        } else {
            //
            // No PortAudio APIs were detected! Therefore there were no PortAudio multimedia devices detected...
            //
            ui->comboBox_soundcard_api->clear();
            ui->comboBox_soundcard_api->insertItem(-1, tr("No available APIs were detected!"), -1);
            ui->comboBox_soundcard_api->setEnabled(false);

            return;
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(this, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
}

void DialogSettings::prefill_audio_encode_comboboxes()
{
    // gkDekodeDb->convAudioBitrateToStr();

    return;
}

void DialogSettings::init_working_freqs()
{
    try {
        ui->tableWidget_working_freqs->setColumnCount(3);
        ui->tableWidget_working_freqs->setRowCount(1);

        QTableWidgetItem *header_iaru_region = new QTableWidgetItem(tr("IARU Region"));
        QTableWidgetItem *header_digital_mode = new QTableWidgetItem(tr("Mode"));
        QTableWidgetItem *header_frequency = new QTableWidgetItem(tr("Frequency"));

        header_iaru_region->setTextAlignment(Qt::AlignHCenter);
        header_digital_mode->setTextAlignment(Qt::AlignHCenter);
        header_frequency->setTextAlignment(Qt::AlignHCenter);

        ui->tableWidget_working_freqs->setItem(0, 0, header_iaru_region);
        ui->tableWidget_working_freqs->setItem(0, 1, header_digital_mode);
        ui->tableWidget_working_freqs->setItem(0, 2, header_frequency);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

void DialogSettings::init_station_info()
{
    try {
        ui->tableWidget_station_info->setColumnCount(3);
        ui->tableWidget_station_info->setRowCount(1);

        QTableWidgetItem *header_band = new QTableWidgetItem(tr("Band"));
        QTableWidgetItem *header_offset = new QTableWidgetItem(tr("Offset"));
        QTableWidgetItem *header_antenna_desc = new QTableWidgetItem(tr("Antenna Description"));

        header_band->setTextAlignment(Qt::AlignHCenter);
        header_offset->setTextAlignment(Qt::AlignHCenter);
        header_antenna_desc->setTextAlignment(Qt::AlignHCenter);

        ui->tableWidget_station_info->setItem(0, 0, header_band);
        ui->tableWidget_station_info->setItem(0, 1, header_offset);
        ui->tableWidget_station_info->setItem(0, 2, header_antenna_desc);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::collectComboBoxIndexes Collects the actual item index (i.e. GkDevice::dev_number()), and
 * returns it as a QMap<int, int>().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param combo_box The QComboBox to be processed in question.
 * @return A QMap<int, int>() with the key being the GkDevice::dev_number() and the value being the QComboBox index.
 */
QMap<int, int> DialogSettings::collectComboBoxIndexes(const QComboBox *combo_box)
{
    try {
        if (combo_box != nullptr) {
            std::mutex index_loop_mtx;
            int combo_box_size = combo_box->count();
            QMap<int, int> collected_indexes;

            for (int i = 0; i < combo_box_size; ++i) {
                std::lock_guard<std::mutex> lck_guard(index_loop_mtx);
                int actual_index = combo_box->itemData(i).toInt();
                collected_indexes.insert(actual_index, i);
            }

            return collected_indexes;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("An error was encountered whilst processing QComboBoxes:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }

    return QMap<int, int>();
}

/**
 * @brief DialogSettings::prefill_rig_force_ctrl_lines will fill out the 'Force Control Lines' section of
 * the Amateur Radio Rig's area of the Settings Dialog.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param ptt_type The PTT Type struct class required for this function to work.
 */
void DialogSettings::prefill_rig_force_ctrl_lines(const ptt_type_t &ptt_type)
{
    try {
        if (ptt_type == ptt_type_t::RIG_PTT_SERIAL_DTR) {
            ui->comboBox_force_ctrl_lines_rts->insertItem(0, tr("High"));
            ui->comboBox_force_ctrl_lines_rts->insertItem(1, tr("Low"));
        } else if (ptt_type == ptt_type_t::RIG_PTT_SERIAL_RTS) {
            ui->comboBox_force_ctrl_lines_dtr->insertItem(0, tr("High"));
            ui->comboBox_force_ctrl_lines_dtr->insertItem(1, tr("Low"));
        } else {
            return;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::prefill_avail_com_ports Finds the available COM/Serial ports within a system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param com_ports The list of returned and available COM/Serial ports. The COM/Serial port name itself
 * is the key and the value is the Target Path plus a Boost C++ triboolean that signifies whether the
 * port is active or not.
 * @see GekkoFyre::RadioLibs::detect_com_ports(), DialogSettings::prefill_avail_usb_ports()
 */
void DialogSettings::prefill_avail_com_ports(const QMap<tstring, std::pair<tstring, boost::tribool>> &com_ports)
{
    using namespace Database::Settings;

    try {
        if (!com_ports.empty()) {
            unsigned short counter_cat = 0;
            unsigned short counter_ptt = 0;
            emit comPortsDisabled(true); // Enable all GUI widgets relating to COM/Serial Ports
            for (const auto &port: com_ports.toStdMap()) {
                #ifdef _UNICODE
                switch (port.second.second.value) {
                case port.second.second.true_value:
                    //
                    // CAT Control
                    //
                    ui->comboBox_com_port->insertItem(counter_cat, QString::fromStdWString(port.first),
                                                      QString::fromStdWString(port.first));
                    available_com_ports.insert(port.second.first, counter_cat);

                    //
                    // PTT Method
                    //
                    ui->comboBox_ptt_method_port->insertItem(counter_ptt, QString::fromStdWString(port.first),
                                                             QString::fromStdWString(port.first));
                    available_com_ports.insert(port.second.first, counter_ptt);

                    ++counter_cat;
                    ++counter_ptt;
                case port.second.second.indeterminate_value:
                    continue;
                case port.second.second.false_value:
                    continue;
                }
                #else
                switch (port.second.second.value) {
                case port.second.second.true_value:
                    //
                    // CAT Control
                    //
                    ui->comboBox_com_port->insertItem(counter_cat, QString::fromStdString(port.first),
                                                      QString::fromStdString(port.first));
                    available_com_ports.insert(port.second.first, counter_cat);

                    //
                    // PTT Method
                    //
                    ui->comboBox_ptt_method_port->insertItem(counter_ptt, QString::fromStdString(port.first),
                                                             QString::fromStdString(port.first));
                    available_com_ports.insert(port.second.first, counter_ptt);

                    ++counter_cat;
                    ++counter_ptt;
                case port.second.second.indeterminate_value:
                    continue;
                case port.second.second.false_value:
                    continue;
                }
                #endif
            }

            //
            // CAT Control
            //
            QString comDeviceCat = gkDekodeDb->read_rig_settings(radio_cfg::ComDeviceCat);
            if (!comDeviceCat.isEmpty() && !available_com_ports.isEmpty()) {
                for (const auto &sel_port: available_com_ports.toStdMap()) {
                    // NOTE: The recorded setting used to identify the chosen serial device is the `Target Path`
                    if (comDeviceCat.toStdString() == sel_port.first) {
                        ui->comboBox_com_port->setCurrentIndex(sel_port.second);
                        on_comboBox_com_port_currentIndexChanged(sel_port.second);
                    }
                }
            }

            //
            // PTT Method
            //
            QString comDevicePtt = gkDekodeDb->read_rig_settings(radio_cfg::ComDevicePtt);
            if (!comDevicePtt.isEmpty() && !available_com_ports.isEmpty()) {
                for (const auto &sel_port: available_com_ports.toStdMap()) {
                    // NOTE: The recorded setting used to identify the chosen serial device is the `Target Path`
                    if (comDevicePtt.toStdString() == sel_port.first) {
                        ui->comboBox_ptt_method_port->setCurrentIndex(sel_port.second);
                        on_comboBox_ptt_method_port_currentIndexChanged(sel_port.second);
                    }
                }
            }
        } else {
            emit comPortsDisabled(false);
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::prefill_avail_usb_ports Fills out the available USB devices within the user's system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param usb_devices The available USB devices (of the audial type) within the user's system.
 * @see GekkoFyre::RadioLibs::enumUsbDevices(), DialogSettings::prefill_avail_com_ports()
 */
void DialogSettings::prefill_avail_usb_ports(const QMap<std::string, GekkoFyre::Database::Settings::GkUsbPort> usb_devices)
{
    using namespace Database::Settings;

    try {
        if (!usb_devices.empty()) {
            // USB devices are not empty!
            emit usbPortsDisabled(true);

            available_usb_ports.clear();
            for (const auto &device: usb_devices) {
                QString dev_port = QString::fromStdString(device.port);
                #ifdef _UNICODE
                QString combined_str = QString("[ #%1 ] %2").arg(QString::number(dev_port)).arg(QString::fromStdWString(device.usb_enum.product));
                available_usb_ports.insert(dev_port, combined_str.toStdWString());
                #else
                QString combined_str = QString("[ #%1 ] %2").arg(dev_port).arg(device.usb_enum.product);
                available_usb_ports.insert(dev_port, combined_str.toStdString());
                #endif

                //
                // CAT Control
                //
                ui->comboBox_com_port->addItem(combined_str, dev_port);

                //
                // PTT Method
                //
                ui->comboBox_ptt_method_port->addItem(combined_str, dev_port);

                combined_str.clear();
            }
        } else {
            // There exists no USB devices...
            emit usbPortsDisabled(false);
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::prefill_com_baud_speed
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param baud_rate
 */
void DialogSettings::prefill_com_baud_speed(const AmateurRadio::com_baud_rates &baud_rate)
{
    using namespace AmateurRadio;

    switch (baud_rate) {
    case BAUD1200:
        ui->comboBox_baud_rate->addItem(tr("1200"));
        break;
    case BAUD2400:
        ui->comboBox_baud_rate->addItem(tr("2400"));
        break;
    case BAUD4800:
        ui->comboBox_baud_rate->addItem(tr("4800"));
        break;
    case BAUD9600:
        ui->comboBox_baud_rate->addItem(tr("9600"));
        break;
    case BAUD19200:
        ui->comboBox_baud_rate->addItem(tr("19200"));
        break;
    case BAUD38400:
        ui->comboBox_baud_rate->addItem(tr("38400"));
        break;
    case BAUD57600:
        ui->comboBox_baud_rate->addItem(tr("57600"));
        break;
    case BAUD115200:
        ui->comboBox_baud_rate->addItem(tr("115200"));
        break;
    default:
        break;
    }

    return;
}

/**
 * @brief DialogSettings::enable_device_port_options
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param enable
 */
void DialogSettings::enable_device_port_options()
{
    bool widget_enable = false;
    if (usb_ports_active == false && com_ports_active == false) {
        widget_enable = false;
    } else {
        widget_enable = true;
    }

    ui->comboBox_com_port->setEnabled(widget_enable);
    ui->comboBox_baud_rate->setEnabled(widget_enable);

    ui->radioButton_data_bits_default->setEnabled(widget_enable);
    ui->radioButton_data_bits_seven->setEnabled(widget_enable);
    ui->radioButton_data_bits_eight->setEnabled(widget_enable);

    ui->radioButton_stop_bits_default->setEnabled(widget_enable);
    ui->radioButton_stop_bits_one->setEnabled(widget_enable);
    ui->radioButton_stop_bits_two->setEnabled(widget_enable);

    ui->radioButton_handshake_default->setEnabled(widget_enable);
    ui->radioButton_handshake_none->setEnabled(widget_enable);
    ui->radioButton_handshake_xon_xoff->setEnabled(widget_enable);
    ui->radioButton_handshake_hardware->setEnabled(widget_enable);

    ui->comboBox_force_ctrl_lines_dtr->setEnabled(widget_enable);
    ui->comboBox_force_ctrl_lines_rts->setEnabled(widget_enable);

    ui->lineEdit_adv_ptt_cmd->setEnabled(widget_enable);

    ui->radioButton_ptt_method_vox->setEnabled(widget_enable);
    ui->radioButton_ptt_method_dtr->setEnabled(widget_enable);
    ui->radioButton_ptt_method_rts->setEnabled(widget_enable);
    ui->radioButton_ptt_method_cat->setEnabled(widget_enable);

    ui->comboBox_ptt_method_port->setEnabled(widget_enable);
    ui->lineEdit_ptt_method_dev_path->setEnabled(widget_enable);

    ui->radioButton_tx_audio_src_rear_data->setEnabled(widget_enable);
    ui->radioButton_tx_audio_src_front_mic->setEnabled(widget_enable);

    ui->radioButton_mode_none->setEnabled(widget_enable);
    ui->radioButton_mode_usb->setEnabled(widget_enable);
    ui->radioButton_mode_data_pkt->setEnabled(widget_enable);

    ui->radioButton_split_none->setEnabled(widget_enable);
    ui->radioButton_split_rig->setEnabled(widget_enable);
    ui->radioButton_split_fake_it->setEnabled(widget_enable);

    return;
}

/**
 * @brief DialogSettings::get_device_port_details grabs details about the chosen COM/Serial/RS232 Port and sends details of it out where needed
 * within DialogSettings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param port The chosen COM/Serial/RS232 port in question.
 * @param device Further, more advanced dtails of the chosen COM/Serial/RS232 port.
 * @param baud_rate The Baud Rate of the chosen COM/Serial/RS232 port, whether it be `38400 bps` or some other value.
 */
void DialogSettings::get_device_port_details(const tstring &port, const tstring &device,
                                             const AmateurRadio::com_baud_rates &baud_rate)
{
    Q_UNUSED(port);
    Q_UNUSED(baud_rate);

    try {
        #ifdef _UNICODE
        ui->lineEdit_device_port_name->setText(QString::fromStdWString(device));
        #else
        ui->lineEdit_device_port_name->setText(QString::fromStdString(device));
        #endif
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::read_settings reads/loads out the previously saved settings from the Setting's Dialog that the user has personally
 * configured and loads them nicely into all the widgets that are present, while doing some basic filtering, error checking, etc. within
 * itself and through some external functions.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return Whether the operation was a success or not!
 */
bool DialogSettings::read_settings()
{
    try {
        using namespace Database::Settings;

        QString rigBrand = gkDekodeDb->read_rig_settings(radio_cfg::RigBrand);
        QString rigModel = gkDekodeDb->read_rig_settings(radio_cfg::RigModel);
        QString rigModelIndex = gkDekodeDb->read_rig_settings(radio_cfg::RigModelIndex);
        QString rigVers = gkDekodeDb->read_rig_settings(radio_cfg::RigVersion);
        QString comBaudRate = gkDekodeDb->read_rig_settings(radio_cfg::ComBaudRate);
        QString stopBits = gkDekodeDb->read_rig_settings(radio_cfg::StopBits);
        QString data_bits = gkDekodeDb->read_rig_settings(radio_cfg::DataBits);
        QString handshake = gkDekodeDb->read_rig_settings(radio_cfg::Handshake);
        QString force_ctrl_lines_dtr = gkDekodeDb->read_rig_settings(radio_cfg::ForceCtrlLinesDtr);
        QString force_ctrl_lines_rts = gkDekodeDb->read_rig_settings(radio_cfg::ForceCtrlLinesRts);
        QString ptt_method = gkDekodeDb->read_rig_settings(radio_cfg::PTTMethod);
        QString tx_audio_src = gkDekodeDb->read_rig_settings(radio_cfg::TXAudioSrc);
        QString ptt_mode = gkDekodeDb->read_rig_settings(radio_cfg::PTTMode);
        QString split_operation = gkDekodeDb->read_rig_settings(radio_cfg::SplitOperation);
        QString ptt_adv_cmd = gkDekodeDb->read_rig_settings(radio_cfg::PTTAdvCmd);

        QString logsDirLoc = gkDekodeDb->read_misc_audio_settings(audio_cfg::LogsDirLoc);
        QString audioRecLoc = gkDekodeDb->read_misc_audio_settings(audio_cfg::AudioRecLoc);
        QString settingsDbLoc = gkDekodeDb->read_misc_audio_settings(audio_cfg::settingsDbLoc);

        Q_UNUSED(rigModel);

        /*
        if (!rigModel.isEmpty()) {
            ui->comboBox_rig_selection->setCurrentIndex(rigModel.toInt());
        }
        */

        if (!rigBrand.isEmpty()) {
            ui->comboBox_brand_selection->setCurrentIndex(rigBrand.toInt());
        }

        if (!rigModelIndex.isEmpty()) {
            ui->comboBox_rig_selection->setCurrentIndex(rigModelIndex.toInt());
        }

        Q_UNUSED(rigVers);
        // if (!rigVers.isEmpty()) {}

        if (!comBaudRate.isEmpty()) {
            ui->comboBox_baud_rate->setCurrentIndex(comBaudRate.toInt());
        }

        if (!stopBits.isEmpty()) {
            short int_val_stop_bits = 0;
            int_val_stop_bits = stopBits.toInt();
            switch (int_val_stop_bits) {
            case 0:
                ui->radioButton_stop_bits_default->setDown(true);
                break;
            case 1:
                ui->radioButton_stop_bits_one->setDown(true);
                break;
            case 2:
                ui->radioButton_stop_bits_two->setDown(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of Stop Bits!").toStdString());
            }
        }

        if (!data_bits.isEmpty()) {
            short int_val_data_bits = 0;
            int_val_data_bits = data_bits.toInt();
            switch (int_val_data_bits) {
            case 0:
                ui->radioButton_data_bits_default->setDown(true);
                break;
            case 1:
                ui->radioButton_data_bits_seven->setDown(true);
                break;
            case 2:
                ui->radioButton_data_bits_eight->setDown(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of Data Bits!").toStdString());
            }
        }

        if (!handshake.isEmpty()) {
            short int_val_handshake = 0;
            int_val_handshake = handshake.toInt();
            switch (int_val_handshake) {
            case 0:
                ui->radioButton_handshake_default->setDown(true);
                break;
            case 1:
                ui->radioButton_handshake_none->setDown(true);
                break;
            case 2:
                ui->radioButton_handshake_xon_xoff->setDown(true);
                break;
            case 3:
                ui->radioButton_handshake_hardware->setDown(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of Stop Bits!").toStdString());
            }
        }

        if (!force_ctrl_lines_dtr.isEmpty()) {
            short int_val_force_ctrl_lines_dtr = 0;
            ui->comboBox_force_ctrl_lines_dtr->setCurrentIndex(int_val_force_ctrl_lines_dtr);
        }

        if (!force_ctrl_lines_rts.isEmpty()) {
            short int_val_force_ctrl_lines_rts = 0;
            ui->comboBox_force_ctrl_lines_rts->setCurrentIndex(int_val_force_ctrl_lines_rts);
        }

        if (!ptt_method.isEmpty()) {
            short int_val_ptt_method = 0;
            int_val_ptt_method = ptt_method.toInt();
            switch (int_val_ptt_method) {
            case 0:
                ui->radioButton_ptt_method_vox->setDown(true);
                break;
            case 1:
                ui->radioButton_ptt_method_dtr->setDown(true);
                break;
            case 2:
                ui->radioButton_ptt_method_cat->setDown(true);
                break;
            case 3:
                ui->radioButton_ptt_method_rts->setDown(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of PTT Method!").toStdString());
            }
        }

        if (!tx_audio_src.isEmpty()) {
            short int_val_tx_audio_src = 0;
            int_val_tx_audio_src = tx_audio_src.toInt();
            switch (int_val_tx_audio_src) {
            case 0:
                ui->radioButton_tx_audio_src_rear_data->setDown(true);
                break;
            case 1:
                ui->radioButton_tx_audio_src_front_mic->setDown(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of TX Audio Source!").toStdString());
            }
        }

        if (!ptt_mode.isEmpty()) {
            short int_val_ptt_mode = 0;
            int_val_ptt_mode = ptt_mode.toInt();
            switch (int_val_ptt_mode) {
            case 0:
                ui->radioButton_mode_none->setDown(true);
                break;
            case 1:
                ui->radioButton_mode_usb->setDown(true);
                break;
            case 2:
                ui->radioButton_mode_data_pkt->setDown(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of PTT Mode!").toStdString());
            }
        }

        if (!split_operation.isEmpty()) {
            short int_val_split_operation = 0;
            int_val_split_operation = split_operation.toInt();
            switch (int_val_split_operation) {
            case 0:
                ui->radioButton_split_none->setDown(true);
                break;
            case 1:
                ui->radioButton_split_rig->setDown(true);
                break;
            case 2:
                ui->radioButton_split_fake_it->setDown(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of Split Operation!").toStdString());
            }
        }

        if (!logsDirLoc.isEmpty()) {
            ui->lineEdit_audio_logs_save_dir->setText(logsDirLoc);
        } else {
            // Point to a default directory...
            ui->lineEdit_audio_logs_save_dir->setText(gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
        }

        if (!ptt_adv_cmd.isEmpty() || !ptt_adv_cmd.isNull()) {
            QString str_val_ptt_adv_cmd = ptt_adv_cmd;
            ui->lineEdit_adv_ptt_cmd->setText(str_val_ptt_adv_cmd);
        }

        if (!audioRecLoc.isEmpty()) {
            ui->lineEdit_audio_save_loc->setText(audioRecLoc);
        } else {
            // Point to a default directory...
            ui->lineEdit_audio_save_loc->setText(gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::MusicLocation)));
        }

        if (!settingsDbLoc.isEmpty()) {
            ui->lineEdit_db_save_loc->setText(settingsDbLoc);
        } else {
            // Point to a default directory...
            ui->lineEdit_db_save_loc->setText(gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
        }

        return true;
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return false;
}

/**
 * @brief DialogSettings::on_comboBox_brand_selection_currentIndexChanged list the transceivers associated with a chosen
 * device manufacturer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void DialogSettings::on_comboBox_brand_selection_currentIndexChanged(const QString &arg1)
{
    QMap<int, QString> temp_map;
    for (const auto &kv: radio_model_names.toStdMap()) {
        if (!arg1.isEmpty() && !std::get<1>(kv.second).isEmpty()) {
            if (arg1 == std::get<1>(kv.second)) {
                if (!std::get<0>(kv.second).isEmpty()) {
                    temp_map.insert(kv.first, std::get<0>(kv.second));
                }
            }
        }
    }

    rig_comboBox->clear();
    for (const auto &model: temp_map.toStdMap()) {
        rig_comboBox->addItem(model.second, model.first); // Do not modify this!
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_com_port_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void DialogSettings::on_comboBox_com_port_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    try {
        //
        // Determine whether a USB or RS232 port has been selected!
        //
        for (const auto &com_port_list: status_com_ports.toStdMap()) {
            for (const auto &usb_port_list: available_usb_ports.toStdMap()) {
                if (usb_port_list.first == ui->comboBox_com_port->currentData()) {
                    //
                    // A USB port has been chosen!
                    //
                    emit changePortType(GekkoFyre::AmateurRadio::GkConnType::USB, true);
                    return;
                } else if (QString::fromStdString(com_port_list.first) == ui->comboBox_com_port->currentData().toString()) {
                    //
                    // An RS232 port has been chosen!
                    //
                    #ifdef _UNICODE
                    ui->lineEdit_device_port_name->setText(QString::fromStdWString(com_port_list.second.first));
                    #else
                    ui->lineEdit_device_port_name->setText(QString::fromStdString(com_port_list.second.first));
                    #endif

                    emit changePortType(GekkoFyre::AmateurRadio::GkConnType::RS232, true);
                    return;
                } else {
                    continue;
                }
            }
        }

        //
        // Nothing has been chosen!
        //
        emit changePortType(GekkoFyre::AmateurRadio::GkConnType::None, true);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_ptt_method_port_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void DialogSettings::on_comboBox_ptt_method_port_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    try {
        //
        // Determine whether a USB or RS232 port has been selected!
        //
        for (const auto &com_port_list: status_com_ports.toStdMap()) {
            for (const auto &usb_port_list: available_usb_ports.toStdMap()) {
                if (usb_port_list.first == ui->comboBox_ptt_method_port->currentData()) {
                    //
                    // A USB port has been chosen!
                    //
                    emit changePortType(GekkoFyre::AmateurRadio::GkConnType::USB, false);
                    return;
                } else if (QString::fromStdString(com_port_list.first) == ui->comboBox_com_port->currentData().toString()) {
                    //
                    // An RS232 port has been chosen!
                    //
                    #ifdef _UNICODE
                    ui->lineEdit_ptt_method_dev_path->setText(QString::fromStdWString(com_port_list.second.first));
                    #else
                    ui->lineEdit_ptt_method_dev_path->setText(QString::fromStdString(com_port_list.second.first));
                    #endif

                    emit changePortType(GekkoFyre::AmateurRadio::GkConnType::RS232, false);
                    return;
                } else {
                    continue;
                }
            }
        }

        //
        // Nothing has been chosen!
        //
        emit changePortType(GekkoFyre::AmateurRadio::GkConnType::None, false);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_db_save_loc_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_db_save_loc_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose a location to save the SWD application database"),
                                                        gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), true),
                                                        QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty()) {
        ui->lineEdit_db_save_loc->setText(dirName);
        gkDekodeDb->write_misc_audio_settings(ui->lineEdit_db_save_loc->text(), audio_cfg::settingsDbLoc);
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_audio_save_loc_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_audio_save_loc_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose a location to save audio files"),
                                                        gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::MusicLocation), true),
                                                        QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty()) {
        ui->lineEdit_audio_save_loc->setText(dirName);
        gkDekodeDb->write_misc_audio_settings(ui->lineEdit_audio_save_loc->text(), audio_cfg::AudioRecLoc);
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_input_sound_test_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note paex_sine.c <http://portaudio.com/docs/v19-doxydocs/paex__sine_8c.html>
 */
void DialogSettings::on_pushButton_input_sound_test_clicked()
{
    try {
        QMessageBox::information(this, tr("Apologies"), tr("We currently do not support audio tests for input devices; this is a feature "
                                                           "that will be added in later versions of Small World Deluxe. Thanks!"), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_output_sound_test_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note paex_sine.c <http://portaudio.com/docs/v19-doxydocs/paex__sine_8c.html>
 */
void DialogSettings::on_pushButton_output_sound_test_clicked()
{
    try {
        if (ui->comboBox_soundcard_output->currentIndex() == 0) {
            QMessageBox::information(this, tr("Information"), tr("You may not perform an audio test on this dialog choice!"), QMessageBox::Ok);
            return;
        }

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Information"));
        msgBox.setText(tr("Upon accepting this informative message, a short sinusoidal audio tone will play for 3 seconds. Please "
                          "note that it can be quite loud given the nature of it, so it is advised that you turn down the volume "
                          "in advance."));
        msgBox.setStandardButtons(QMessageBox::Abort | QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Warning);
        int ret = msgBox.exec();

        if (ret == QMessageBox::Ok) {
            gkAudioDevices->testSinewave(*gkPortAudioInit, chosen_output_audio_dev, true);
            QMessageBox::information(this, tr("Finished"), tr("The audio test has now finished."), QMessageBox::Ok);
        } else if (ret == QMessageBox::Abort) {
            QMessageBox::information(this, tr("Aborted"), tr("The operation has been terminated."), QMessageBox::Ok);
        } else {
            return;
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(this, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_soundcard_input_currentIndexChanged is activated when someone makes
 * a selection within the QComboBox of the Input Audio Device. This is particularly useful regarding
 * the audio sinusoidal test functionality.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index The current index of the QComboBox.
 * @see DialogSettings::on_comboBox_soundcard_api_currentIndexChanged().
 */
void DialogSettings::on_comboBox_soundcard_input_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    try {
        //
        // Input audio devices
        //
        if (!avail_portaudio_api.isEmpty() || !avail_input_audio_devs.isEmpty()) {
            for (const auto &device: avail_input_audio_devs.toStdMap()) {
                if (device.first == ui->comboBox_soundcard_input->currentData().toInt()) {
                    GkDevice chosen_input;
                    chosen_input = gkAudioDevices->gatherAudioDeviceDetails(gkPortAudioInit, ui->comboBox_soundcard_input->currentData().toInt());
                    chosen_input_audio_dev = chosen_input;

                    return;
                }
            }
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(this, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_soundcard_output_currentIndexChanged is activated when someone makes
 * a selection within the QComboBox of the Output Audio Device. This is particularly useful regarding
 * the audio sinusoidal test functionality.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index The current index of the QComboBox.
 * @see DialogSettings::on_comboBox_soundcard_api_currentIndexChanged().
 */
void DialogSettings::on_comboBox_soundcard_output_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    try {
        //
        // Output audio devices
        //
        if (!avail_portaudio_api.isEmpty() || !avail_output_audio_devs.isEmpty()) {
            for (const auto &device: avail_output_audio_devs.toStdMap()) {
                if (device.first == ui->comboBox_soundcard_output->currentData().toInt()) {
                    GkDevice chosen_output;
                    chosen_output = gkAudioDevices->gatherAudioDeviceDetails(gkPortAudioInit, ui->comboBox_soundcard_output->currentData().toInt());
                    chosen_output_audio_dev = chosen_output;

                    return;
                }
            }
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(this, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
}

void DialogSettings::on_comboBox_soundcard_api_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    try {
        if (!avail_portaudio_api.isEmpty() || !avail_input_audio_devs.isEmpty()) {
            //
            // Input audio devices
            //
            ui->comboBox_soundcard_input->clear();
            for (const auto &pa_api: avail_portaudio_api.toStdMap()) {
                for (const auto &device: avail_input_audio_devs.toStdMap()) {
                    GkDevice input_dev = device.second;
                    if ((pa_api.first == ui->comboBox_soundcard_api->currentData().toInt()) && (input_dev.host_type_id == pa_api.second)) {
                        std::string audio_dev_name = input_dev.device_info.name;
                        if (!audio_dev_name.empty()) {
                            ui->comboBox_soundcard_input->insertItem(input_dev.stream_parameters.device, QString::fromStdString(audio_dev_name),
                                                                     input_dev.stream_parameters.device);
                        }
                    }
                }
            }
        }

        if (!avail_output_audio_devs.isEmpty()) {
            //
            // Output audio devices
            //
            ui->comboBox_soundcard_output->clear();
            for (const auto &pa_api: avail_portaudio_api.toStdMap()) {
                for (const auto &device: avail_output_audio_devs.toStdMap()) {
                    GkDevice output_dev = device.second;
                    if ((pa_api.first == ui->comboBox_soundcard_api->currentData().toInt()) && (output_dev.host_type_id == pa_api.second)) {
                        std::string audio_dev_name = output_dev.device_info.name;
                        if (!audio_dev_name.empty()) {
                            ui->comboBox_soundcard_output->insertItem(output_dev.stream_parameters.device, QString::fromStdString(audio_dev_name),
                                                                      output_dev.stream_parameters.device);
                        }
                    }
                }
            }
        }
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(this, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
}

void DialogSettings::on_spinBox_spectro_render_thread_settings_valueChanged(int arg1)
{
    Q_UNUSED(arg1);

    return;
}

void DialogSettings::on_pushButton_audio_logs_save_dir_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose a location to save data logs"),
                                                        gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), true),
                                                        QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty()) {
        ui->lineEdit_audio_logs_save_dir->setText(dirName);
        gkDekodeDb->write_misc_audio_settings(ui->lineEdit_audio_logs_save_dir->text(), audio_cfg::AudioRecLoc);
    }

    return;
}

void DialogSettings::on_pushButton_soundcard_api_reload_clicked()
{
    return;
}

void DialogSettings::on_horizontalSlider_encoding_audio_quality_valueChanged(int value)
{
    audio_quality_val = ((double)value / 10);
    std::ostringstream oss;
    oss << std::setprecision(2) << audio_quality_val;
    ui->label_encoding_audio_quality_value->setText(QString::fromStdString(oss.str()));

    return;
}

void DialogSettings::disableUsbPorts(const bool &active)
{
    usb_ports_active = active;
    enable_device_port_options();

    return;
}

void DialogSettings::disableComPorts(const bool &active)
{
    com_ports_active = active;
    enable_device_port_options();

    return;
}

void DialogSettings::on_comboBox_rig_selection_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    // unsigned short sel_rig_index = ui->comboBox_rig_selection->currentIndex();

    // gkRadioPtr->rig->caps->get_split_mode;

    return;
}

void DialogSettings::on_radioButton_data_bits_default_clicked()
{
    ui->radioButton_data_bits_default->setDown(true);
    ui->radioButton_data_bits_seven->setDown(false);
    ui->radioButton_data_bits_eight->setDown(false);

    return;
}

void DialogSettings::on_radioButton_data_bits_seven_clicked()
{
    ui->radioButton_data_bits_seven->setDown(true);
    ui->radioButton_data_bits_default->setDown(false);
    ui->radioButton_data_bits_eight->setDown(false);

    return;
}

void DialogSettings::on_radioButton_data_bits_eight_clicked()
{
    ui->radioButton_data_bits_eight->setDown(true);
    ui->radioButton_data_bits_default->setDown(false);
    ui->radioButton_data_bits_seven->setDown(false);

    return;
}

void DialogSettings::on_radioButton_stop_bits_default_clicked()
{
    ui->radioButton_stop_bits_default->setDown(true);
    ui->radioButton_stop_bits_one->setDown(false);
    ui->radioButton_stop_bits_two->setDown(false);

    return;
}

void DialogSettings::on_radioButton_stop_bits_one_clicked()
{
    ui->radioButton_stop_bits_one->setDown(true);
    ui->radioButton_stop_bits_default->setDown(false);
    ui->radioButton_stop_bits_two->setDown(false);

    return;
}

void DialogSettings::on_radioButton_stop_bits_two_clicked()
{
    ui->radioButton_stop_bits_two->setDown(true);
    ui->radioButton_stop_bits_one->setDown(false);
    ui->radioButton_stop_bits_default->setDown(false);

    return;
}

void DialogSettings::on_radioButton_handshake_default_clicked()
{
    ui->radioButton_handshake_default->setDown(true);
    ui->radioButton_handshake_none->setDown(false);
    ui->radioButton_handshake_xon_xoff->setDown(false);
    ui->radioButton_handshake_hardware->setDown(false);

    return;
}

void DialogSettings::on_radioButton_handshake_none_clicked()
{
    ui->radioButton_handshake_default->setDown(false);
    ui->radioButton_handshake_none->setDown(true);
    ui->radioButton_handshake_xon_xoff->setDown(false);
    ui->radioButton_handshake_hardware->setDown(false);

    return;
}

void DialogSettings::on_radioButton_handshake_xon_xoff_clicked()
{
    ui->radioButton_handshake_default->setDown(false);
    ui->radioButton_handshake_none->setDown(false);
    ui->radioButton_handshake_xon_xoff->setDown(true);
    ui->radioButton_handshake_hardware->setDown(false);

    return;
}

void DialogSettings::on_radioButton_handshake_hardware_clicked()
{
    ui->radioButton_handshake_default->setDown(false);
    ui->radioButton_handshake_none->setDown(false);
    ui->radioButton_handshake_xon_xoff->setDown(false);
    ui->radioButton_handshake_hardware->setDown(true);

    return;
}

void DialogSettings::on_radioButton_ptt_method_vox_clicked()
{
    ui->radioButton_ptt_method_vox->setDown(true);
    ui->radioButton_ptt_method_dtr->setDown(false);
    ui->radioButton_ptt_method_cat->setDown(false);
    ui->radioButton_ptt_method_rts->setDown(false);

    return;
}

void DialogSettings::on_radioButton_ptt_method_dtr_clicked()
{
    ui->radioButton_ptt_method_vox->setDown(false);
    ui->radioButton_ptt_method_dtr->setDown(true);
    ui->radioButton_ptt_method_cat->setDown(false);
    ui->radioButton_ptt_method_rts->setDown(false);

    return;
}

void DialogSettings::on_radioButton_ptt_method_cat_clicked()
{
    ui->radioButton_ptt_method_vox->setDown(false);
    ui->radioButton_ptt_method_dtr->setDown(false);
    ui->radioButton_ptt_method_cat->setDown(true);
    ui->radioButton_ptt_method_rts->setDown(false);

    return;
}

void DialogSettings::on_radioButton_ptt_method_rts_clicked()
{
    ui->radioButton_ptt_method_vox->setDown(false);
    ui->radioButton_ptt_method_dtr->setDown(false);
    ui->radioButton_ptt_method_cat->setDown(false);
    ui->radioButton_ptt_method_rts->setDown(true);

    return;
}

void DialogSettings::on_radioButton_tx_audio_src_rear_data_clicked()
{
    ui->radioButton_tx_audio_src_rear_data->setDown(true);
    ui->radioButton_tx_audio_src_front_mic->setDown(false);

    return;
}

void DialogSettings::on_radioButton_tx_audio_src_front_mic_clicked()
{
    ui->radioButton_tx_audio_src_rear_data->setDown(false);
    ui->radioButton_tx_audio_src_front_mic->setDown(true);

    return;
}

void DialogSettings::on_radioButton_mode_none_clicked()
{
    ui->radioButton_mode_none->setDown(true);
    ui->radioButton_mode_usb->setDown(false);
    ui->radioButton_mode_data_pkt->setDown(false);

    return;
}

void DialogSettings::on_radioButton_mode_usb_clicked()
{
    ui->radioButton_mode_none->setDown(false);
    ui->radioButton_mode_usb->setDown(true);
    ui->radioButton_mode_data_pkt->setDown(false);

    return;
}

void DialogSettings::on_radioButton_mode_data_pkt_clicked()
{
    ui->radioButton_mode_none->setDown(false);
    ui->radioButton_mode_usb->setDown(false);
    ui->radioButton_mode_data_pkt->setDown(true);

    return;
}

void DialogSettings::on_radioButton_split_none_clicked()
{
    ui->radioButton_split_none->setDown(true);
    ui->radioButton_split_rig->setDown(false);
    ui->radioButton_split_fake_it->setDown(false);

    return;
}

void DialogSettings::on_radioButton_split_rig_clicked()
{
    ui->radioButton_split_none->setDown(false);
    ui->radioButton_split_rig->setDown(true);
    ui->radioButton_split_fake_it->setDown(false);

    return;
}

void DialogSettings::on_radioButton_split_fake_it_clicked()
{
    ui->radioButton_split_none->setDown(false);
    ui->radioButton_split_rig->setDown(false);
    ui->radioButton_split_fake_it->setDown(true);

    return;
}

/**
 * @brief DialogSettings::on_DialogSettings_rejected This signal is emitted when the dialog has been rejected either by
 * the user or by calling reject() or done() with the QDialog::Rejected argument.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note QDialog <https://doc.qt.io/qt-5/qdialog.html#finished>
 */
void DialogSettings::on_DialogSettings_rejected()
{
    return;
}
