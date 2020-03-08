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
#include <exception>
#include <set>
#include <QStringList>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <utility>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

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
                               std::shared_ptr<GekkoFyre::RadioLibs> radioPtr, QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

    try {
        //
        // Initialize PortAudio for Settings Dialog!
        //
        autoSys.initialize();
        gkPortAudioInit = new portaudio::System(portaudio::System::instance());

        gkRadioLibs = std::move(radioPtr);
        gkDekodeDb = std::move(dkDb);
        gkFileIo = std::move(filePtr);
        gkAudioDevices = std::move(audioDevices);

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

        Q_ASSERT(gkRadioLibs.get() != nullptr);

        // Detect available COM ports on either Microsoft Windows, Linux, or even Apple Mac OS/X
        // Also detect the available USB devices of the audial type
        // NOTE: There are two functions, one each for COM/Serial/RS-232 ports and another for USB
        // devices separately. This is because within the GekkoFyre::RadioLibs namespace, there are
        // also two separate functions for enumerating out these ports!
        status_com_ports = gkRadioLibs->status_com_ports();
        status_usb_devices = gkRadioLibs->findUsbPorts();
        prefill_avail_com_ports(status_com_ports);
        prefill_avail_usb_ports(status_usb_devices);

        if (!status_com_ports.empty()) {
            // Select the initial COM port and load it into memory
            on_comboBox_com_port_currentIndexChanged(ui->comboBox_com_port->currentIndex());
        }

        std::vector<GkDevice> audio_devices = gkAudioDevices->filterAudioDevices(gkAudioDevices->enumAudioDevicesCpp(gkPortAudioInit));
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

    autoSys.terminate();
    gkPortAudioInit->terminate();

    delete ui;
}

/**
 * @brief DialogSettings::on_pushButton_submit_config_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_submit_config_clicked()
{
    try {
        //
        // Rig Settings
        //
        int brand = ui->comboBox_brand_selection->currentIndex();
        QVariant sel_rig = ui->comboBox_rig_selection->currentData();
        int sel_rig_index = ui->comboBox_rig_selection->currentIndex();
        int com_device = ui->comboBox_com_port->currentIndex();
        int com_baud_rate = ui->comboBox_baud_rate->currentIndex();
        int stop_bits = ui->spinBox_stop_bits->value();
        int poll_interv = ui->spinBox_polling_interv->value();
        int mode_delay = ui->spinBox_mode_delay->value();
        bool cw_lsb_mode = ui->checkBox_cw_lsb_mode->checkState();
        int flow_control = ui->comboBox_flow_control->currentIndex();
        QString ptt_command = ui->lineEdit_adv_ptt_cmd->text();
        int retries = ui->spinBox_subopt_retries->value();
        int retry_interv = ui->spinBox_subopt_retry_interv->value();
        int write_delay = ui->spinBox_subopt_write_delay->value();
        int post_write_delay = ui->spinBox_subopt_post_write_delay->value();

        //
        // Audio Device Channels (i.e. Mono/Stereo/etc)
        //
        int curr_input_device_channels = ui->comboBox_soundcard_input_channels->currentIndex();
        int curr_output_device_channels = ui->comboBox_soundcard_output_channels->currentIndex();

        //
        // RS-232 Settings
        //
        tstring chosen_com_port;
        for (const auto &sel_port: available_com_ports.toStdMap()) {
            for (const auto &avail_port: status_com_ports.toStdMap()) {
                // List out the current serial ports in use
                if (com_device == sel_port.second) {
                    if (sel_port.first == avail_port.second.first) {
                        #ifdef _WIN32
                        chosen_com_port = sel_port.first; // The chosen serial device uses its own name as a reference
                        #elif __linux__
                        chosen_com_port = sel_port.first; // The chosen serial device uses a Target Path as reference
                        #endif
                        break;
                    }
                }
            }
        }

        #ifdef _UNICODE
        gkDekodeDb->write_rig_settings(QString::fromStdWString(chosen_com_port), radio_cfg::ComDevice);
        #else
        gkDekodeDb->write_rig_settings(QString::fromStdString(chosen_com_port), radio_cfg::ComDevice);
        #endif

        using namespace Database::Settings;
        gkDekodeDb->write_rig_settings(QString::number(brand), radio_cfg::RigBrand);
        gkDekodeDb->write_rig_settings(QString::number(sel_rig.toInt()), radio_cfg::RigModel);
        gkDekodeDb->write_rig_settings(QString::number(sel_rig_index), radio_cfg::RigModelIndex);
        gkDekodeDb->write_rig_settings(QString::number(com_baud_rate), radio_cfg::ComBaudRate);
        gkDekodeDb->write_rig_settings(QString::number(stop_bits), radio_cfg::StopBits);
        gkDekodeDb->write_rig_settings(QString::number(poll_interv), radio_cfg::PollingInterval);
        gkDekodeDb->write_rig_settings(QString::number(mode_delay), radio_cfg::ModeDelay);
        gkDekodeDb->write_rig_settings(QString::number(cw_lsb_mode), radio_cfg::CWisLSB);
        gkDekodeDb->write_rig_settings(QString::number(flow_control), radio_cfg::FlowControl);
        gkDekodeDb->write_rig_settings(ptt_command, radio_cfg::PTTCommand);
        gkDekodeDb->write_rig_settings(QString::number(retries), radio_cfg::Retries);
        gkDekodeDb->write_rig_settings(QString::number(retry_interv), radio_cfg::RetryInterv);
        gkDekodeDb->write_rig_settings(QString::number(write_delay), radio_cfg::WriteDelay);
        gkDekodeDb->write_rig_settings(QString::number(post_write_delay), radio_cfg::PostWriteDelay);

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
 * @brief DialogSettings::prefill_audio_devices Enumerates the audio deviecs on the user's
 * computer system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_devices The available audio devices on the user's system, as a typical std::vector.
 * @see GekkoFyre::AudioDevices::enumAudioDevices()
 */
void DialogSettings::prefill_audio_devices(std::vector<GkDevice> audio_devices_vec)
{
    try {
        ui->comboBox_soundcard_input->insertItem(0, tr("No audio device selected"));
        ui->comboBox_soundcard_output->insertItem(0, tr("No audio device selected"));

        int output_identifier = gkDekodeDb->read_audio_device_settings(true);
        int input_identifier = gkDekodeDb->read_audio_device_settings(false);

        for (const auto device: audio_devices_vec) {
            if (device.device_info.hostApi >= 0) {
                if (device.is_output_dev) {
                    //
                    // Audio device is an output
                    //
                    std::string audio_dev_name = device.device_info.name;
                    if (!audio_dev_name.empty()) {
                        ui->comboBox_soundcard_output->insertItem(device.stream_parameters.device, QString::fromStdString(audio_dev_name),
                                                                  device.stream_parameters.device);
                        avail_output_audio_devs.insert(device.stream_parameters.device, device);
                    }
                } else {
                    //
                    // Audio device is an input
                    //
                    std::string audio_dev_name = device.device_info.name;
                    if (!audio_dev_name.empty()) {
                        ui->comboBox_soundcard_input->insertItem(device.stream_parameters.device, QString::fromStdString(audio_dev_name),
                                                                 device.stream_parameters.device);
                        avail_input_audio_devs.insert(device.stream_parameters.device, device);
                    }
                }
            } else {
                throw std::runtime_error(tr("Given audio devices are nullptr! Small World Deluxe may not work correctly if you continue...").toStdString());
            }
        }

        //
        // Collect the indexes for each QComboBox, for both input/output audio devices!
        //
        QMap<int, int> input_dev_indexes = collectComboBoxIndexes(ui->comboBox_soundcard_input);
        QMap<int, int> output_dev_indexes = collectComboBoxIndexes(ui->comboBox_soundcard_output);

        // Select the user's chosen audio input devices, if one has been previously chosen
        // and saved as a setting...
        GkDevice input_device = gkAudioDevices->gatherAudioDeviceDetails(gkPortAudioInit, input_identifier);
        for (const auto &input_index: input_dev_indexes.toStdMap()) {
            if (input_device.dev_number == input_index.first) {
                ui->comboBox_soundcard_input->setCurrentIndex(input_index.second);
                chosen_input_audio_dev = input_device;
                break;
            }
        }

        // Select the user's chosen audio output devices, if one has been previously chosen
        // and saved as a setting...
        GkDevice output_device = gkAudioDevices->gatherAudioDeviceDetails(gkPortAudioInit, output_identifier);
        for (const auto &output_index: output_dev_indexes.toStdMap()) {
            if (output_device.dev_number == output_index.first) {
                ui->comboBox_soundcard_output->setCurrentIndex(output_index.second);
                chosen_output_audio_dev = output_device;
                break;
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
 * @brief DialogSettings::prefill_avail_com_ports Finds the available COM/Serial ports within a system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param ports The list of returned and available COM/Serial ports.
 * @param upperLimit The upper limit on how many COM/Serial ports to potentially return.
 * @see GekkoFyre::RadioLibs::detect_com_ports(), DialogSettings::prefill_avail_usb_ports()
 */
void DialogSettings::prefill_avail_com_ports(const QMap<tstring, std::pair<tstring, boost::tribool>> &com_ports)
{
    using namespace Database::Settings;

    try {
        if (!com_ports.empty()) {
            int counter = 0;
            enable_device_port_options(true); // Enable all GUI widgets relating to COM/Serial Ports
            for (const auto &port: com_ports.toStdMap()) {
                #ifdef _UNICODE
                if (port.second.value == port.second.true_value) {
                    ui->comboBox_com_port->addItem(QString::fromStdWString(port.first));
                    available_com_ports.insert(port.second.first, counter);
                    ++counter;
                } else if (port.second.value == port.second.indeterminate_value) {
                    ui->comboBox_com_port->addItem(QString::fromStdWString(port.first));
                    available_com_ports.insert(port.second.first, counter);
                    ++counter;
                } else {
                    continue;
                }
                #else
                if (port.second.second.value == port.second.second.true_value) {
                    ui->comboBox_com_port->insertItem(counter, QString::fromStdString(port.first));
                    available_com_ports.insert(port.second.first, counter);
                    ++counter;
                } else if (port.second.second.value == port.second.second.indeterminate_value) {
                    ui->comboBox_com_port->insertItem(counter, QString::fromStdString(port.first));
                    available_com_ports.insert(port.second.first, counter);
                    ++counter;
                } else {
                    continue;
                }
                #endif
            }

            // If a setting has been saved regarding RS-232 device port selection, then we should load
            // this up!
            // NOTE: The recorded setting used to identify the chosen serial device is the `Target Path`
            QString comDevice = gkDekodeDb->read_rig_settings(radio_cfg::ComDevice);
            if (!comDevice.isEmpty()) {
                for (const auto &sel_port: available_com_ports.toStdMap()) {
                    if (comDevice.toStdString() == sel_port.first) {
                        ui->comboBox_com_port->setCurrentIndex(sel_port.second);
                    }
                }
            }
        } else {
            enable_device_port_options(false); // Disable all GUI widgets relating to COM/Serial Ports
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
void DialogSettings::prefill_avail_usb_ports(const std::vector<UsbPort> usb_devices)
{
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
void DialogSettings::enable_device_port_options(const bool &enable)
{
    ui->comboBox_com_port->setEnabled(enable);
    ui->comboBox_baud_rate->setEnabled(enable);
    ui->spinBox_stop_bits->setEnabled(enable);
    ui->spinBox_polling_interv->setEnabled(enable);
    ui->spinBox_mode_delay->setEnabled(enable);

    ui->spinBox_subopt_retries->setEnabled(enable);
    ui->spinBox_subopt_retry_interv->setEnabled(enable);
    ui->spinBox_subopt_write_delay->setEnabled(enable);
    ui->spinBox_subopt_post_write_delay->setEnabled(enable);

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
 * @brief DialogSettings::read_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
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
        QString pollInterv = gkDekodeDb->read_rig_settings(radio_cfg::PollingInterval);
        QString modeDelay = gkDekodeDb->read_rig_settings(radio_cfg::ModeDelay);
        QString sideBand = gkDekodeDb->read_rig_settings(radio_cfg::Sideband);
        QString cwIsLSB = gkDekodeDb->read_rig_settings(radio_cfg::CWisLSB);
        QString flowControl = gkDekodeDb->read_rig_settings(radio_cfg::FlowControl);
        QString pttCommand = gkDekodeDb->read_rig_settings(radio_cfg::PTTCommand);
        QString retries = gkDekodeDb->read_rig_settings(radio_cfg::Retries);
        QString retryInterv = gkDekodeDb->read_rig_settings(radio_cfg::RetryInterv);
        QString writeDelay = gkDekodeDb->read_rig_settings(radio_cfg::WriteDelay);
        QString postWriteDelay = gkDekodeDb->read_rig_settings(radio_cfg::PostWriteDelay);

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

        if (!rigVers.isEmpty()) {}

        if (!comBaudRate.isEmpty()) {
            ui->comboBox_baud_rate->setCurrentIndex(comBaudRate.toInt());
        }

        if (!stopBits.isEmpty()) {
            ui->spinBox_stop_bits->setValue(stopBits.toInt());
        }

        if (!pollInterv.isEmpty()) {
            ui->spinBox_polling_interv->setValue(pollInterv.toInt());
        }

        if (!modeDelay.isEmpty()) {
            ui->spinBox_mode_delay->setValue(modeDelay.toInt());
        }

        if (!sideBand.isEmpty()) {}

        if (!cwIsLSB.isEmpty()) {
            ui->checkBox_cw_lsb_mode->setChecked(cwIsLSB.toInt());
        }

        if (!flowControl.isEmpty()) {
            ui->comboBox_flow_control->setCurrentIndex(flowControl.toInt());
        }

        if (!pttCommand.isEmpty()) {
            ui->lineEdit_adv_ptt_cmd->setText(pttCommand);
        }

        if (!retries.isEmpty()) {
            ui->spinBox_subopt_retries->setValue(retries.toInt());
        }

        if (!retryInterv.isEmpty()) {
            ui->spinBox_subopt_retry_interv->setValue(retryInterv.toInt());
        }

        if (!writeDelay.isEmpty()) {
            ui->spinBox_subopt_write_delay->setValue(writeDelay.toInt());
        }

        if (!postWriteDelay.isEmpty()) {
            ui->spinBox_subopt_post_write_delay->setValue(postWriteDelay.toInt());
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
    for (const auto &port_list: available_com_ports.toStdMap()) {
        if (port_list.second == index) {
            #ifdef _UNICODE
            ui->lineEdit_device_port_name->setText(QString::fromStdWString(port_list.first));
            #else
            ui->lineEdit_device_port_name->setText(QString::fromStdString(port_list.first));
            #endif
            return;
        }
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_db_save_loc_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_db_save_loc_clicked()
{}

/**
 * @brief DialogSettings::on_pushButton_audio_save_loc_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_audio_save_loc_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose a location to save audio files"),
                                                        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
                                                        QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty()) {
        ui->lineEdit_audio_save_loc->setText(dirName);
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
 */
void DialogSettings::on_comboBox_soundcard_input_currentIndexChanged(int index)
{
    try {
        int actual_index = ui->comboBox_soundcard_input->currentData().toInt();
        for (const auto &device: avail_input_audio_devs.toStdMap()) {
            if (device.first == actual_index) {
                GkDevice chosen_input;
                chosen_input = device.second;
                chosen_input = gkAudioDevices->gatherAudioDeviceDetails(gkPortAudioInit, actual_index);

                chosen_input_audio_dev = chosen_input;

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
 * @brief DialogSettings::on_comboBox_soundcard_output_currentIndexChanged is activated when someone makes
 * a selection within the QComboBox of the Output Audio Device. This is particularly useful regarding
 * the audio sinusoidal test functionality.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index The current index of the QComboBox.
 */
void DialogSettings::on_comboBox_soundcard_output_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    try {
        int actual_index = ui->comboBox_soundcard_output->currentData().toInt();
        for (const auto &device: avail_output_audio_devs.toStdMap()) {
            if (device.first == actual_index) {
                GkDevice chosen_output;
                chosen_output = device.second;
                chosen_output = gkAudioDevices->gatherAudioDeviceDetails(gkPortAudioInit, actual_index);

                chosen_output_audio_dev = chosen_output;

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

void DialogSettings::on_spinBox_spectro_render_thread_settings_valueChanged(int arg1)
{
    Q_UNUSED(arg1);

    return;
}

void DialogSettings::on_pushButton_db_save_loc_2_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose a location to save audio files"),
                                                        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
                                                        QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty()) {
        ui->lineEdit_db_save_loc_2->setText(dirName);
    }

    return;
}

void DialogSettings::on_pushButton_audio_logs_save_dir_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose a location to save audio files"),
                                                        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
                                                        QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty()) {
        ui->lineEdit_audio_logs_save_dir->setText(dirName);
    }

    return;
}
