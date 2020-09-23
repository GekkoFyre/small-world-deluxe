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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "dialogsettings.hpp"
#include "ui_dialogsettings.h"
#include <hamlib/rig.h>
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
#include <iomanip>
#include <cstdlib>

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

QComboBox *DialogSettings::rig_comboBox = nullptr;
QComboBox *DialogSettings::mfg_comboBox = nullptr;
QMultiMap<rig_model_t, std::tuple<QString, QString, GekkoFyre::AmateurRadio::rig_type>> DialogSettings::radio_model_names = init_model_names();
QVector<QString> DialogSettings::unique_mfgs = { "None" };

std::mutex index_loop_mtx;

/**
 * @brief DialogSettings::DialogSettings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dkDb
 * @param filePtr
 * @param parent
 */
DialogSettings::DialogSettings(QPointer<GkLevelDb> dkDb,
                               QPointer<FileIo> filePtr,
                               std::shared_ptr<AudioDevices> audioDevices,
                               QPointer<RadioLibs> radioLibs, QPointer<StringFuncs> stringFuncs,
                               portaudio::System *portAudioInit,
                               std::shared_ptr<GkRadio> radioPtr,
                               const std::list<GekkoFyre::Database::Settings::GkComPort> &com_ports,
                               const QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> &usbPortMap,
                               QPointer<GkFrequencies> gkFreqList,
                               QPointer<GkFreqTableModel> freqTableModel,
                               QPointer<GekkoFyre::GkEventLogger> eventLogger,
                               QPointer<GekkoFyre::GkTextToSpeech> textToSpeechPtr,
                               QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

    try {
        //
        // Initialize PortAudio for Settings Dialog!
        //
        gkPortAudioInit = portAudioInit;
        gkRadioLibs = std::move(radioLibs);
        gkStringFuncs = std::move(stringFuncs);
        gkDekodeDb = std::move(dkDb);
        gkFileIo = std::move(filePtr);
        gkAudioDevices = std::move(audioDevices);
        gkRadioPtr = std::move(radioPtr);
        gkFreqs = std::move(gkFreqList);
        gkFreqTableModel = std::move(freqTableModel);
        gkEventLogger = std::move(eventLogger);
        gkTextToSpeech = std::move(textToSpeechPtr);
        status_com_ports = com_ports;

        usb_ports_active = false;
        com_ports_active = false;
        audio_quality_val = 0.0;

        //
        // The detectable and thusly, testable, PortAudio 'sample rates' for each found audio device on the user's system!
        //
        standardSampleRates = {
            8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
            44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1 /* negative terminated list */
        };

        gkFreqTableModel->populateData(gkFreqs->listOfFreqs());
        ui->tableView_working_freqs->setModel(gkFreqTableModel);
        ui->tableView_working_freqs->horizontalHeader()->setVisible(true);
        ui->tableView_working_freqs->horizontalHeader()->setSectionResizeMode(GK_FREQ_TABLEVIEW_MODEL_FREQUENCY_IDX, QHeaderView::Stretch);
        ui->tableView_working_freqs->show();

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
        status_usb_devices = usbPortMap;
        prefill_avail_com_ports(status_com_ports);
        prefill_avail_usb_ports(status_usb_devices);

        // Select the initial COM port and load it into memory
        on_comboBox_com_port_currentIndexChanged(ui->comboBox_com_port->currentIndex());
        on_comboBox_ptt_method_port_currentIndexChanged(ui->comboBox_ptt_method_port->currentIndex());

        //
        // Initialize PortAudio libraries!
        //
        std::vector<GkDevice> audio_devices = gkAudioDevices->filterPortAudioHostType(gkAudioDevices->enumAudioDevicesCpp(gkPortAudioInit));
        QVector<PaHostApiTypeId> portaudio_api_avail = gkAudioDevices->portAudioApiChooser(audio_devices);
        prefill_audio_api_avail(portaudio_api_avail);
        prefill_audio_devices(audio_devices);

        ui->label_pa_version->setText(gkAudioDevices->portAudioVersionNumber(*gkPortAudioInit));
        ui->plainTextEdit_pa_version_text->setPlainText(gkAudioDevices->portAudioVersionText(*gkPortAudioInit));

        ui->label_hamlib_api_version->setText(QString::fromStdString(hamlib_version2));
        ui->plainTextEdit_hamlib_api_version_info->setPlainText(QString::fromStdString(hamlib_copyright2));

        prefill_com_baud_speed(com_baud_rates::BAUD1200);
        prefill_com_baud_speed(com_baud_rates::BAUD2400);
        prefill_com_baud_speed(com_baud_rates::BAUD4800);
        prefill_com_baud_speed(com_baud_rates::BAUD9600);
        prefill_com_baud_speed(com_baud_rates::BAUD19200);
        prefill_com_baud_speed(com_baud_rates::BAUD38400);
        prefill_com_baud_speed(com_baud_rates::BAUD57600);
        prefill_com_baud_speed(com_baud_rates::BAUD115200);

        init_station_info();

        if (gkDekodeDb != nullptr) {
            read_settings();
        }

        //
        // Prefill information within the `General --> Event Logger` tab...
        //
        prefill_event_logger();

        //
        // Initialize Text-to-Speech GUI widgets
        //
        ui->comboBox_access_stt_engine->addItem(tr("Default"), tr("Default"));
        const auto engines = QTextToSpeech::availableEngines();
        for (const QString &engine: engines) {
            ui->comboBox_access_stt_engine->addItem(engine, engine);
        }

        ui->comboBox_access_stt_engine->setCurrentIndex(0);

        //
        // Initialize Text-to-Speech signals/slots
        //
        QObject::connect(ui->pushButton_access_stt_speak, SIGNAL(clicked()), gkTextToSpeech, SLOT(speak()));
        QObject::connect(ui->horizontalSlider_access_stt_pitch, SIGNAL(valueChanged(int)), gkTextToSpeech, SLOT(setPitch(const int &)));
        QObject::connect(ui->horizontalSlider_access_stt_rate, SIGNAL(valueChanged(int)), gkTextToSpeech, SLOT(setRate(const int &)));
        QObject::connect(ui->horizontalSlider_access_stt_volume, SIGNAL(valueChanged(int)), gkTextToSpeech, SLOT(setVolume(const int &)));
        QObject::connect(ui->comboBox_access_stt_engine, SIGNAL(currentIndexChanged(int)), gkTextToSpeech, SLOT(engineSelected(int)));
        QObject::connect(this, SIGNAL(changeSelectedTTSEngine(const QString &)), gkTextToSpeech, SLOT(engineSelected(const QString &)));
        QObject::connect(gkTextToSpeech, SIGNAL(addLangItem(const QString &, const QVariant &)), this, SLOT(ttsAddLanguageItem(const QString &, const QVariant &)));
        QObject::connect(gkTextToSpeech, SIGNAL(addVoiceItem(const QString &, const QVariant &)), this, SLOT(ttsAddPresetVoiceItem(const QString &, const QVariant &)));
        QObject::connect(gkTextToSpeech, SIGNAL(clearLangItems()), ui->comboBox_access_stt_language, SLOT(clear()));
        QObject::connect(gkTextToSpeech, SIGNAL(clearVoiceItems()), ui->comboBox_access_stt_preset_voice, SLOT(clear()));
        QObject::connect(gkTextToSpeech, SIGNAL(setVoiceCurrentIndex(int)), ui->comboBox_access_stt_preset_voice, SLOT(setCurrentIndex(int)));
        QObject::connect(gkTextToSpeech, SLOT(localeChanged(const QLocale &)), this, SLOT(ttsLocaleChanged(const QLocale &)));
        QObject::connect(ui->comboBox_access_stt_preset_voice, SIGNAL(currentIndexChanged(int)), gkTextToSpeech, SLOT(voiceSelected(int)));
        QObject::connect(ui->comboBox_access_stt_language, SIGNAL(currentIndexChanged(int)), gkTextToSpeech, SLOT(languageSelected(int)));
    } catch (const portaudio::PaException &e) {
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
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
        int brand = ui->comboBox_brand_selection->currentIndex();
        QVariant sel_rig = ui->comboBox_rig_selection->currentData();
        int sel_rig_index = ui->comboBox_rig_selection->currentIndex();
        int cat_conn_type = gkDekodeDb->convConnTypeToInt(gkRadioPtr->cat_conn_type);
        int ptt_conn_type = gkDekodeDb->convConnTypeToInt(gkRadioPtr->ptt_conn_type);
        QString com_device_cat = ui->comboBox_com_port->currentData().toString();
        QString com_device_ptt = ui->comboBox_ptt_method_port->currentData().toString();
        int com_baud_rate = ui->comboBox_baud_rate->currentIndex();
        QString ptt_adv_cmd = ui->lineEdit_adv_ptt_cmd->text();

        //
        // Audio --> Configuration
        //
        bool rx_audio_init_start = ui->checkBox_init_rx_audio_upon_start->isChecked();

        //
        // Chosen PortAudio API
        //
        int chosen_pa_api = ui->comboBox_soundcard_api->currentData().toInt();

        //
        // Audio Device Channels (i.e. Mono/Stereo/etc)
        //
        int curr_input_device_channels = ui->comboBox_soundcard_input_channels->currentIndex();
        int curr_output_device_channels = ui->comboBox_soundcard_output_channels->currentIndex();

        //
        // Operating System's own Settings (e.g. the registry under Microsoft Windows)
        //
        // gkFileIo->write_initial_settings(Filesystem::fileName, init_cfg::DbName);
        // gkFileIo->write_initial_settings(ui->lineEdit_db_save_loc->text(), init_cfg::DbLoc);

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
        chosen_input_audio_dev.chosen_audio_dev_str = ui->comboBox_soundcard_input->currentText();
        chosen_input_audio_dev.sel_channels = gkDekodeDb->convertAudioChannelsEnum(curr_input_device_channels);
        gkDekodeDb->write_audio_device_settings(chosen_input_audio_dev, false);

        //
        // Output Device
        //
        chosen_output_audio_dev.chosen_audio_dev_str = ui->comboBox_soundcard_output->currentText();
        chosen_output_audio_dev.sel_channels = gkDekodeDb->convertAudioChannelsEnum(curr_output_device_channels);
        gkDekodeDb->write_audio_device_settings(chosen_output_audio_dev, true);

        //
        // Now make the sound-device selection official throughout the running Small World Deluxe application!
        // NOTE: The order of these functions is rather semi-important! Do not change without reason!
        //
        for (const auto &input_dev: avail_input_audio_devs.toStdMap()) {
            if (chosen_input_audio_dev.chosen_audio_dev_str == input_dev.second.device_info.name) {
                emit changeInputAudioInterface(input_dev.second);
            }
        }

        for (const auto &output_dev: avail_output_audio_devs.toStdMap()) {
            if (chosen_output_audio_dev.chosen_audio_dev_str == output_dev.second.device_info.name) {
                emit changeOutputAudioInterface(output_dev.second);
            }
        }

        //
        // Audio --> Configuration
        //
        gkDekodeDb->write_rig_settings(QString::fromStdString(gkDekodeDb->boolEnum(rx_audio_init_start)), radio_cfg::RXAudioInitStart);

        //
        // General --> Event Logger
        //
        qint16 event_log_verb_idx = ui->comboBox_event_logger_general_verbosity->currentIndex();
        if (event_log_verb_idx >= 0) {
            // We have a valid result!
            gkDekodeDb->write_event_log_settings(QString::number(event_log_verb_idx), GkEventLogCfg::GkLogVerbosity);
        }

        //
        // Data Bits
        //
        qint16 enum_data_bits = 0;
        if (ui->radioButton_data_bits_default->isChecked()) {
            // Default
            enum_data_bits = 0;
        } else if (ui->radioButton_data_bits_seven->isChecked()) {
            // Seven (7)
            enum_data_bits = 7;
        } else {
            // Eight (8)
            enum_data_bits = 8;
        }

        //
        // Stop Bits
        //
        qint16 enum_stop_bits = 0;
        if (ui->radioButton_stop_bits_default->isChecked()) {
            // Default
            enum_stop_bits = 0;
        } else if (ui->radioButton_stop_bits_one->isChecked()) {
            // One (1)
            enum_stop_bits = 1;
        } else {
            // Two (2)
            enum_stop_bits = 2;
        }

        //
        // Handshake
        //
        qint16 enum_handshake = 0;
        if (ui->radioButton_handshake_default->isChecked()) {
            // Default
            enum_handshake = 0;
        } else if (ui->radioButton_handshake_none->isChecked()) {
            // None
            enum_handshake = 1;
        } else if (ui->radioButton_handshake_xon_xoff->isChecked()) {
            // XON / XOFF
            enum_handshake = 2;
        } else {
            // Hardware
            enum_handshake = 3;
        }

        //
        // PTT Method
        //
        qint16 enum_ptt_method = 0;
        if (ui->radioButton_ptt_method_vox->isChecked()) {
            // VOX
            enum_ptt_method = 0;
        } else if (ui->radioButton_ptt_method_dtr->isChecked()) {
            // DTR
            enum_ptt_method = 1;
        } else if (ui->radioButton_ptt_method_cat->isChecked()) {
            // CAT
            enum_ptt_method = 2;
        } else {
            // RTS
            enum_ptt_method = 3;
        }

        //
        // Transmit Audio Source
        //
        qint16 enum_tx_audio_src = 0;
        if (ui->radioButton_tx_audio_src_rear_data->isChecked()) {
            // Rear / Data
            enum_tx_audio_src = 0;
        } else {
            // Front / Mic
            enum_tx_audio_src = 1;
        }

        //
        // Mode
        //
        qint16 enum_mode = 0;
        if (ui->radioButton_mode_none->isChecked()) {
            // None
            enum_mode = 0;
        } else if (ui->radioButton_mode_usb->isChecked()) {
            // USB
            enum_mode = 1;
        } else {
            // Data / PKT
            enum_mode = 2;
        }

        //
        // Split Operation
        //
        qint16 enum_split_oper = 0;
        if (ui->radioButton_split_none->isChecked()) {
            // None
            enum_split_oper = 0;
        } else if (ui->radioButton_split_rig->isChecked()) {
            // Rig
            enum_split_oper = 1;
        } else {
            // Fake It
            enum_split_oper = 2;
        }

        const qint32 inputSampleRateIdx = ui->comboBox_audio_input_sample_rate->currentIndex();
        const qint32 inputChannelsIdx = ui->comboBox_soundcard_input_channels->currentIndex();
        const qint32 outputSampleRateIdx = ui->comboBox_audio_output_sample_rate->currentIndex();
        const qint32 outputChannelsIdx = ui->comboBox_soundcard_output_channels->currentIndex();

        qint16 enum_force_ctrl_lines_dtr = 0;
        qint16 enum_force_ctrl_lines_rts = 0;
        enum_force_ctrl_lines_dtr = ui->comboBox_force_ctrl_lines_dtr->currentIndex();
        enum_force_ctrl_lines_rts = ui->comboBox_force_ctrl_lines_rts->currentIndex();

        GkConnType conn_type_cat = ascertConnType(false);
        GkConnType conn_type_ptt = ascertConnType(true);

        gkDekodeDb->write_rig_settings_comms(com_device_cat, radio_cfg::ComDeviceCat);
        gkDekodeDb->write_rig_settings_comms(com_device_ptt, radio_cfg::ComDevicePtt);
        gkDekodeDb->write_rig_settings_comms(QString::number(gkDekodeDb->convConnTypeToInt(conn_type_cat)), radio_cfg::ComDeviceCatPortType);
        gkDekodeDb->write_rig_settings_comms(QString::number(gkDekodeDb->convConnTypeToInt(conn_type_ptt)), radio_cfg::ComDevicePttPortType);
        gkDekodeDb->write_rig_settings_comms(QString::number(com_baud_rate), radio_cfg::ComBaudRate);

        using namespace Database::Settings;
        gkDekodeDb->write_rig_settings(QString::number(brand), radio_cfg::RigBrand);
        gkDekodeDb->write_rig_settings(QString::number(sel_rig.toInt()), radio_cfg::RigModel);
        gkDekodeDb->write_rig_settings(QString::number(sel_rig_index), radio_cfg::RigModelIndex);
        gkDekodeDb->write_rig_settings(QString::number(cat_conn_type), radio_cfg::CatConnType);
        gkDekodeDb->write_rig_settings(QString::number(ptt_conn_type), radio_cfg::PttConnType);
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

        gkDekodeDb->write_misc_audio_settings(QString::number(inputSampleRateIdx), audio_cfg::AudioInputSampleRate);
        gkDekodeDb->write_misc_audio_settings(QString::number(outputSampleRateIdx), audio_cfg::AudioOutputSampleRate);
        gkDekodeDb->write_misc_audio_settings(QString::number(inputChannelsIdx), audio_cfg::AudioInputChannels);
        gkDekodeDb->write_misc_audio_settings(QString::number(outputChannelsIdx), audio_cfg::AudioOutputChannels);

        emit addRigInUse(ui->comboBox_rig_selection->currentData().toInt(), gkRadioPtr);

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
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::Transceiver));
            break;
        case RIG_TYPE_HANDHELD:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::Handheld));
            break;
        case RIG_TYPE_MOBILE:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::Mobile));
            break;
        case RIG_TYPE_RECEIVER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::Receiver));
            break;
        case RIG_TYPE_PCRECEIVER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::PC_Receiver));
            break;
        case RIG_TYPE_SCANNER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::Scanner));
            break;
        case RIG_TYPE_TRUNKSCANNER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::TrunkingScanner));
            break;
        case RIG_TYPE_COMPUTER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::Computer));
            break;
        case RIG_TYPE_OTHER:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::Other));
            break;
        default:
            radio_model_names.insert(caps->rig_model, std::make_tuple(caps->model_name, caps->mfg_name, rig_type::Unknown));
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
            //
            // Prefill the QComboBox responsible for displaying the PortAudio detected multimedia APIs on the user's system!
            //
            for (const auto &pa_api: portaudio_api_vec) {
                QString api_str_tmp = gkDekodeDb->portAudioApiToStr(pa_api);
                int underlying_api_int = to_underlying(pa_api);
                ui->comboBox_soundcard_api->insertItem(underlying_api_int, api_str_tmp, underlying_api_int);
                avail_portaudio_api.insert(underlying_api_int, pa_api);

                if (!avail_portaudio_api.isEmpty()) {
                    //
                    // Verify that the user's chosen and saved PortAudio API actually exist given this new instance
                    // of the Setting's Dialog! If so, set the current index of the QComboBox towards it.
                    //
                    PaHostApiTypeId api_identifier = gkDekodeDb->read_audio_api_settings();
                    if (api_identifier != PaHostApiTypeId::paInDevelopment) {
                        int actual_api_idx = to_underlying(api_identifier);
                        qint32 idx = ui->comboBox_soundcard_api->findData(api_identifier);
                        ui->comboBox_soundcard_api->setCurrentIndex(idx);

                        //
                        // Find any new audio devices
                        //
                        std::vector<GkDevice> audio_devices = gkAudioDevices->filterPortAudioHostType(gkAudioDevices->enumAudioDevicesCpp(gkPortAudioInit));
                        prefill_audio_devices(audio_devices);
                        on_comboBox_soundcard_api_currentIndexChanged();

                        //
                        // Input audio devices
                        //
                        auto soundcard_input_saved = gkDekodeDb->read_audio_device_settings(false, false);
                        if (!soundcard_input_saved.isNull() && !soundcard_input_saved.isEmpty()) {
                            for (const auto &input_dev: avail_input_audio_devs.toStdMap()) {
                                if (input_dev.second.device_info.name) { // Test that `const char *` is not empty!
                                    if (soundcard_input_saved == input_dev.second.device_info.name) {
                                        qint32 idx = ui->comboBox_soundcard_input->findData(input_dev.second.device_info.name);
                                        chosen_input_audio_dev = input_dev.second;
                                        if (idx >= 0) {
                                            ui->comboBox_soundcard_input->setCurrentIndex(idx);
                                            on_comboBox_soundcard_input_currentIndexChanged(idx);

                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        //
                        // Output audio devices
                        //
                        auto soundcard_output_saved = gkDekodeDb->read_audio_device_settings(true, false);
                        if (!soundcard_output_saved.isNull() && !soundcard_output_saved.isEmpty()) {
                            for (const auto &output_dev: avail_output_audio_devs.toStdMap()) {
                                if (output_dev.second.device_info.name) { // Test that `const char *` is not empty!
                                    if (soundcard_output_saved == output_dev.second.device_info.name) {
                                        qint32 idx = ui->comboBox_soundcard_output->findData(output_dev.second.device_info.name);
                                        chosen_output_audio_dev = output_dev.second;
                                        if (idx >= 0) {
                                            ui->comboBox_soundcard_output->setCurrentIndex(idx);
                                            on_comboBox_soundcard_output_currentIndexChanged(idx);

                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        ui->comboBox_soundcard_api->setCurrentIndex(-2);
                    }
                }
            }
        }
    } catch (const portaudio::PaException &e) {
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
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
void DialogSettings::prefill_audio_devices(const std::vector<GkDevice> &audio_devices_vec)
{
    try {
        if (!avail_portaudio_api.isEmpty()) {
            if (!audio_devices_vec.empty()) {
                quint32 output_counter = 0;
                quint32 input_counter = 0;
                for (const auto &device: audio_devices_vec) {
                    if (device.audio_src == GkAudioSource::Output) {
                        //
                        // Audio device is an output
                        //
                        std::string audio_dev_name = device.device_info.name;
                        if (!audio_dev_name.empty()) {
                            avail_output_audio_devs.insert(output_counter, device);
                            ++output_counter;
                        }
                    } else if (device.audio_src == GkAudioSource::Input) {
                        //
                        // Audio device is an input
                        //
                        std::string audio_dev_name = device.device_info.name;
                        if (!audio_dev_name.empty()) {
                            avail_input_audio_devs.insert(input_counter, device);
                            ++input_counter;
                        }
                    } else {
                        //
                        // Audio device is both input and output
                        //
                        std::string audio_dev_name = device.device_info.name;
                        if (!audio_dev_name.empty()) {
                            avail_input_audio_devs.insert(input_counter, device);
                            avail_output_audio_devs.insert(output_counter, device);

                            ++output_counter;
                            ++input_counter;
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
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    }

    return;
}

/**
 * @brief DialogSettings::prefill_audio_encode_comboboxes
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::prefill_audio_encode_comboboxes()
{
    // gkDekodeDb->convAudioBitrateToStr();

    return;
}

/**
 * @brief DialogSettings::prefill_event_logger
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::prefill_event_logger()
{
    ui->comboBox_event_logger_general_verbosity->insertItem(GK_EVENTLOG_SEVERITY_NONE_IDX, gkDekodeDb->convSeverityToStr(GkSeverity::None));
    ui->comboBox_event_logger_general_verbosity->insertItem(GK_EVENTLOG_SEVERITY_VERBOSE_IDX, gkDekodeDb->convSeverityToStr(GkSeverity::Verbose));
    ui->comboBox_event_logger_general_verbosity->insertItem(GK_EVENTLOG_SEVERITY_DEBUG_IDX, gkDekodeDb->convSeverityToStr(GkSeverity::Debug));
    ui->comboBox_event_logger_general_verbosity->insertItem(GK_EVENTLOG_SEVERITY_INFO_IDX, gkDekodeDb->convSeverityToStr(GkSeverity::Info));
    ui->comboBox_event_logger_general_verbosity->insertItem(GK_EVENTLOG_SEVERITY_WARNING_IDX, gkDekodeDb->convSeverityToStr(GkSeverity::Warning));
    ui->comboBox_event_logger_general_verbosity->insertItem(GK_EVENTLOG_SEVERITY_ERROR_IDX, gkDekodeDb->convSeverityToStr(GkSeverity::Error));
    ui->comboBox_event_logger_general_verbosity->insertItem(GK_EVENTLOG_SEVERITY_FATAL_IDX, gkDekodeDb->convSeverityToStr(GkSeverity::Fatal));

    return;
}

/**
 * @brief DialogSettings::init_station_info
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::init_station_info()
{
    try {
        ui->tableWidget_station_info->setColumnCount(3);
        ui->tableWidget_station_info->setRowCount(1);

        auto *header_band = new QTableWidgetItem(tr("Band"));
        auto *header_offset = new QTableWidgetItem(tr("Offset"));
        auto *header_antenna_desc = new QTableWidgetItem(tr("Antenna Description"));

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
 * @brief DialogSettings::print_exception
 * @param e
 * @param level
 */
void DialogSettings::print_exception(const std::exception &e, int level)
{
    QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);

    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        print_exception(e, level + 1);
    } catch (...) {}

    return;
}

/**
 * @brief DialogSettings::convQComboBoxSampleRateToDouble
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param combobox_idx
 * @return
 */
double DialogSettings::convQComboBoxSampleRateToDouble(const int &combobox_idx)
{
    switch (combobox_idx) {
    case 0:
        return 8000.0;
    case 1:
        return 9600.0;
    case 2:
        return 11025.0;
    case 3:
        return 12000.0;
    case 4:
        return 16000.0;
    case 5:
        return 22050.0;
    case 6:
        return 24000.0;
    case 7:
        return 32000.0;
    case 8:
        return 44100.0;
    case 9:
        return 48000.0;
    case 10:
        return 88200.0;
    case 11:
        return 96000.0;
    case 12:
        return 8000.0;
    case 13:
        return 192000.0;
    default:
        break;
    }

    return -1.0f;
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
void DialogSettings::prefill_avail_com_ports(const std::list<GkComPort> &com_ports)
{
    using namespace Database::Settings;

    try {
        //
        // Default values!
        //
        if (!com_ports.empty()) {
            quint16 counter = 0;
            emit comPortsDisabled(false); // Enable all GUI widgets relating to COM/Serial Ports
            for (const auto &port: com_ports) {
                ++counter;
                QString portName = port.port_info.portName();

                //
                // CAT Control
                //
                ui->comboBox_com_port->insertItem(counter, portName, portName);

                //
                // PTT Method
                //
                ui->comboBox_ptt_method_port->insertItem(counter, portName, portName);

                available_com_ports.insert(port.port_info.description(), counter);
            }

            //
            // CAT Control
            //
            QString comDeviceCat = gkDekodeDb->read_rig_settings_comms(radio_cfg::ComDeviceCat);
            if (!comDeviceCat.isEmpty() && !available_com_ports.isEmpty()) {
                for (const auto &sel_port: available_com_ports.toStdMap()) {
                    for (const auto &device: status_com_ports) {
                        if ((device.port_info.description() == sel_port.first) && (comDeviceCat == device.port_info.portName())) {
                            //
                            // COM/RS232/Serial Port
                            //

                            // NOTE: The recorded setting used to identify the chosen serial device is the COM Port name
                            ui->comboBox_com_port->setCurrentIndex(sel_port.second);
                            on_comboBox_com_port_currentIndexChanged(sel_port.second);
                        }
                    }
                }
            }

            //
            // PTT Method
            //
            QString comDevicePtt = gkDekodeDb->read_rig_settings_comms(radio_cfg::ComDevicePtt);
            if (!comDevicePtt.isEmpty() && !available_com_ports.isEmpty()) {
                for (const auto &sel_port: available_com_ports.toStdMap()) {
                    for (const auto &device: status_com_ports) {
                        if ((device.port_info.description() == sel_port.first) && (comDevicePtt == device.port_info.portName())) {
                            // NOTE: The recorded setting used to identify the chosen serial device is the COM Port name
                            ui->comboBox_ptt_method_port->setCurrentIndex(sel_port.second);
                            on_comboBox_ptt_method_port_currentIndexChanged(sel_port.second);
                        }
                    }
                }
            }
        } else {
            emit comPortsDisabled(true);
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
void DialogSettings::prefill_avail_usb_ports(const QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> &usb_devices)
{
    using namespace Database::Settings;

    try {
        if (!usb_devices.empty()) {
            // USB devices are not empty!
            emit usbPortsDisabled(false);

            available_usb_ports.clear();
            quint16 counter = 0;
            for (const auto &device: usb_devices) {
                quint16 dev_port = device.port;
                #ifdef _UNICODE
                QString combined_str = QString("[ #%1 ] %2").arg(QString::fromStdWString(device.port)).arg(device.bos_usb.lib_usb.product);
                available_usb_ports.insert(dev_port, combined_str.toStdWString());
                #else
                available_usb_ports.insert(dev_port, device.name);
                #endif

                //
                // CAT Control (via USB)
                //
                ui->comboBox_com_port->insertItem(counter, device.name, dev_port);
                if (ui->comboBox_com_port->currentData().toInt() == device.port) {
                    ui->lineEdit_device_port_name->setText(device.bos_usb.lib_usb.product);
                }

                //
                // PTT Method (via USB)
                //
                ui->comboBox_ptt_method_port->insertItem(counter, device.name, dev_port);
                if (ui->comboBox_ptt_method_port->currentData().toInt() == device.port) {
                    ui->lineEdit_ptt_method_dev_path->setText(device.bos_usb.lib_usb.product);
                }

                ++counter;
            }

            //
            // USB Port (QCombobox Index)
            //
            QString usbDeviceCat = gkDekodeDb->read_rig_settings_comms(radio_cfg::ComDeviceCat);
            QString usbDevicePtt = gkDekodeDb->read_rig_settings_comms(radio_cfg::ComDevicePtt);
            if (!available_usb_ports.isEmpty() && (!usbDeviceCat.isEmpty() || !usbDevicePtt.isEmpty())) {
                for (const auto &sel_port: available_usb_ports.toStdMap()) {
                    for (const auto &device: status_usb_devices) {
                        if (!usbDeviceCat.isEmpty()) {
                            //
                            // CAT Control
                            //
                            if ((device.port == sel_port.first) && (usbDeviceCat == device.name)) {
                                // NOTE: The recorded setting used to identify the chosen serial device is the COM Port name
                                ui->comboBox_com_port->setCurrentIndex(device.port);
                                on_comboBox_com_port_currentIndexChanged(device.port);
                            }
                        }

                        if (!usbDevicePtt.isEmpty()) {
                            //
                            // PTT Method
                            //
                            if ((device.port == sel_port.first) && (usbDevicePtt == device.name)) {
                                // NOTE: The recorded setting used to identify the chosen serial device is the COM Port name
                                ui->comboBox_ptt_method_port->setCurrentIndex(device.port);
                                on_comboBox_ptt_method_port_currentIndexChanged(device.port);
                            }
                        }
                    }
                }
            }
        } else {
            // There exists no USB devices...
            emit usbPortsDisabled(true);
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
    if (!usb_ports_active && !com_ports_active) {
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
 * @brief DialogSettings::update_sample_rates manages the sample rates for each audio device, whether it be an input or
 * output audio device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_output_device Is the device in question an output audio device?
 */
void DialogSettings::update_sample_rates(const bool &is_output_device)
{
    if (is_output_device) {
        //
        // Output device
        //
        int idx = 0;
        ui->comboBox_audio_output_sample_rate->clear();
        for (const auto &sampleRate: supportedOutputSampleRates) {
            ui->comboBox_audio_output_sample_rate->insertItem(idx, QString::number(sampleRate));
            ++idx;
        }
    } else {
        //
        // Input device
        //
        int idx = 0;
        ui->comboBox_audio_input_sample_rate->clear();
        for (const auto &sampleRate: supportedInputSampleRates) {
            ui->comboBox_audio_input_sample_rate->insertItem(idx, QString::number(sampleRate));
            ++idx;
        }
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

        const QString rigBrand = gkDekodeDb->read_rig_settings(radio_cfg::RigBrand);
        const QString rigModel = gkDekodeDb->read_rig_settings(radio_cfg::RigModel);
        const QString rigModelIndex = gkDekodeDb->read_rig_settings(radio_cfg::RigModelIndex);
        const QString rigVers = gkDekodeDb->read_rig_settings(radio_cfg::RigVersion);
        const QString comBaudRate = gkDekodeDb->read_rig_settings_comms(radio_cfg::ComBaudRate);
        const QString stopBits = gkDekodeDb->read_rig_settings(radio_cfg::StopBits);
        const QString data_bits = gkDekodeDb->read_rig_settings(radio_cfg::DataBits);
        const QString handshake = gkDekodeDb->read_rig_settings(radio_cfg::Handshake);
        const QString force_ctrl_lines_dtr = gkDekodeDb->read_rig_settings(radio_cfg::ForceCtrlLinesDtr);
        const QString force_ctrl_lines_rts = gkDekodeDb->read_rig_settings(radio_cfg::ForceCtrlLinesRts);
        const QString ptt_method = gkDekodeDb->read_rig_settings(radio_cfg::PTTMethod);
        const QString tx_audio_src = gkDekodeDb->read_rig_settings(radio_cfg::TXAudioSrc);
        const QString ptt_mode = gkDekodeDb->read_rig_settings(radio_cfg::PTTMode);
        const QString split_operation = gkDekodeDb->read_rig_settings(radio_cfg::SplitOperation);
        const QString ptt_adv_cmd = gkDekodeDb->read_rig_settings(radio_cfg::PTTAdvCmd);

        const QString logsDirLoc = gkDekodeDb->read_misc_audio_settings(audio_cfg::LogsDirLoc);
        const QString audioRecLoc = gkDekodeDb->read_misc_audio_settings(audio_cfg::AudioRecLoc);
        const QString settingsDbLoc = gkDekodeDb->read_misc_audio_settings(audio_cfg::settingsDbLoc);

        const QString msg_audio_notif = gkDekodeDb->read_general_settings(general_stat_cfg::MsgAudioNotif);
        const QString fail_event_notif = gkDekodeDb->read_general_settings(general_stat_cfg::FailAudioNotif);

        const QString inputSampleRateIdx = gkDekodeDb->read_misc_audio_settings(audio_cfg::AudioInputSampleRate);
        const QString inputChannelsIdx = gkDekodeDb->read_misc_audio_settings(audio_cfg::AudioInputChannels);
        const QString outputSampleRateIdx = gkDekodeDb->read_misc_audio_settings(audio_cfg::AudioOutputSampleRate);
        const QString outputChannelsIdx = gkDekodeDb->read_misc_audio_settings(audio_cfg::AudioOutputChannels);

        const QString eventLogVerbIdx = gkDekodeDb->read_event_log_settings(GkEventLogCfg::GkLogVerbosity);

        //
        // Audio --> Configuration
        //
        const QString rx_audio_init_start = gkDekodeDb->read_rig_settings(radio_cfg::RXAudioInitStart);

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
        } else {
            if (gkRadioPtr->capabilities != nullptr) {
                ui->comboBox_baud_rate->setCurrentIndex(gkRadioLibs->convertBaudRateFromEnum(gkRadioLibs->convertBaudRateIntToEnum(gkRadioPtr->capabilities->serial_rate_min)));
            }
        }

        if (!stopBits.isEmpty()) {
            qint16 int_val_stop_bits = 0;
            int_val_stop_bits = stopBits.toInt();
            switch (int_val_stop_bits) {
            case 0:
                ui->radioButton_stop_bits_default->setChecked(true);
                break;
            case 1:
                ui->radioButton_stop_bits_one->setChecked(true);
                break;
            case 2:
                ui->radioButton_stop_bits_two->setChecked(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of Stop Bits!").toStdString());
            }
        } else {
            if (gkRadioPtr->capabilities != nullptr) {
                switch (gkRadioPtr->capabilities->serial_stop_bits) {
                case 0:
                    ui->radioButton_stop_bits_default->setDown(true);
                    ui->radioButton_stop_bits_one->setDown(false);
                    ui->radioButton_stop_bits_two->setDown(false);
                    break;
                case 1:
                    ui->radioButton_stop_bits_one->setDown(true);
                    ui->radioButton_stop_bits_default->setDown(false);
                    ui->radioButton_stop_bits_two->setDown(false);
                    break;
                case 2:
                    ui->radioButton_stop_bits_two->setDown(true);
                    ui->radioButton_stop_bits_one->setDown(false);
                    ui->radioButton_stop_bits_default->setDown(false);
                    break;
                default:
                    throw std::invalid_argument(tr("Invalid value for amount of Stop Bits!").toStdString());
                }
            }
        }

        if (!data_bits.isEmpty()) {
            qint16 int_val_data_bits = 0;
            int_val_data_bits = data_bits.toInt();
            switch (int_val_data_bits) {
            case 0:
                ui->radioButton_data_bits_default->setChecked(true);
                break;
            case 7:
                ui->radioButton_data_bits_seven->setChecked(true);
                break;
            case 8:
                ui->radioButton_data_bits_eight->setChecked(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of Data Bits!").toStdString());
            }
        } else {
            if (gkRadioPtr->capabilities != nullptr) {
                switch (gkRadioPtr->capabilities->serial_data_bits) {
                case 0:
                    ui->radioButton_data_bits_default->setDown(true);
                    ui->radioButton_data_bits_seven->setDown(false);
                    ui->radioButton_data_bits_eight->setDown(false);
                    break;
                case 7:
                    ui->radioButton_data_bits_seven->setDown(true);
                    ui->radioButton_data_bits_default->setDown(false);
                    ui->radioButton_data_bits_eight->setDown(false);
                    break;
                case 8:
                    ui->radioButton_data_bits_eight->setDown(true);
                    ui->radioButton_data_bits_seven->setDown(false);
                    ui->radioButton_data_bits_default->setDown(false);
                    break;
                default:
                    throw std::invalid_argument(tr("Invalid value for amount of Data Bits!").toStdString());
                }
            }
        }

        if (!handshake.isEmpty()) {
            qint16 int_val_handshake = 0;
            int_val_handshake = handshake.toInt();
            switch (int_val_handshake) {
            case 0:
                ui->radioButton_handshake_default->setChecked(true);
                break;
            case 1:
                ui->radioButton_handshake_none->setChecked(true);
                break;
            case 2:
                ui->radioButton_handshake_xon_xoff->setChecked(true);
                break;
            case 3:
                ui->radioButton_handshake_hardware->setChecked(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of Stop Bits!").toStdString());
            }
        } else {
            if (gkRadioPtr->capabilities != nullptr) {
                switch (gkRadioPtr->capabilities->serial_handshake) {
                case serial_handshake_e::RIG_HANDSHAKE_NONE:
                    ui->radioButton_handshake_none->setDown(true);
                    ui->radioButton_handshake_xon_xoff->setDown(false);
                    ui->radioButton_handshake_hardware->setDown(false);
                    ui->radioButton_handshake_default->setDown(false);
                    break;
                case serial_handshake_e::RIG_HANDSHAKE_XONXOFF:
                    ui->radioButton_handshake_xon_xoff->setDown(true);
                    ui->radioButton_handshake_hardware->setDown(false);
                    ui->radioButton_handshake_default->setDown(false);
                    ui->radioButton_handshake_none->setDown(false);
                    break;
                case serial_handshake_e::RIG_HANDSHAKE_HARDWARE:
                    ui->radioButton_handshake_hardware->setDown(true);
                    ui->radioButton_handshake_default->setDown(false);
                    ui->radioButton_handshake_none->setDown(false);
                    ui->radioButton_handshake_xon_xoff->setDown(false);
                    break;
                default:
                    ui->radioButton_handshake_default->setDown(true);
                    ui->radioButton_handshake_none->setDown(false);
                    ui->radioButton_handshake_xon_xoff->setDown(false);
                    ui->radioButton_handshake_hardware->setDown(false);
                    break;
                }
            }
        }

        if (!force_ctrl_lines_dtr.isEmpty()) {
            qint16 int_val_force_ctrl_lines_dtr = 0; // TODO: Finish this critical area!
            ui->comboBox_force_ctrl_lines_dtr->setCurrentIndex(int_val_force_ctrl_lines_dtr);
        }

        if (!force_ctrl_lines_rts.isEmpty()) {
            qint16 int_val_force_ctrl_lines_rts = 0; // TODO: Finish this critical area!
            ui->comboBox_force_ctrl_lines_rts->setCurrentIndex(int_val_force_ctrl_lines_rts);
        }

        if (!ptt_method.isEmpty()) {
            qint16 int_val_ptt_method = 0;
            int_val_ptt_method = ptt_method.toInt();
            switch (int_val_ptt_method) {
            case 0:
                ui->radioButton_ptt_method_vox->setChecked(true);
                break;
            case 1:
                ui->radioButton_ptt_method_dtr->setChecked(true);
                break;
            case 2:
                ui->radioButton_ptt_method_cat->setChecked(true);
                break;
            case 3:
                ui->radioButton_ptt_method_rts->setChecked(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of PTT Method!").toStdString());
            }
        } else {
            if (gkRadioPtr->capabilities != nullptr) {
                switch (gkRadioPtr->capabilities->ptt_type) {
                case ptt_type_t::RIG_PTT_RIG_MICDATA:
                    ui->radioButton_ptt_method_vox->setDown(true);
                    ui->radioButton_ptt_method_dtr->setDown(false);
                    ui->radioButton_ptt_method_cat->setDown(false);
                    ui->radioButton_ptt_method_rts->setDown(false);
                    break;
                case ptt_type_t::RIG_PTT_SERIAL_DTR:
                    ui->radioButton_ptt_method_dtr->setDown(true);
                    ui->radioButton_ptt_method_cat->setDown(false);
                    ui->radioButton_ptt_method_rts->setDown(false);
                    ui->radioButton_ptt_method_vox->setDown(false);
                    break;
                case ptt_type_t::RIG_PTT_RIG:
                    ui->radioButton_ptt_method_cat->setDown(true);
                    ui->radioButton_ptt_method_rts->setDown(false);
                    ui->radioButton_ptt_method_vox->setDown(false);
                    ui->radioButton_ptt_method_dtr->setDown(false);
                    break;
                case ptt_type_t::RIG_PTT_SERIAL_RTS:
                    ui->radioButton_ptt_method_rts->setDown(true);
                    ui->radioButton_ptt_method_vox->setDown(false);
                    ui->radioButton_ptt_method_dtr->setDown(false);
                    ui->radioButton_ptt_method_cat->setDown(false);
                    break;
                default:
                    ui->radioButton_ptt_method_vox->setDown(true);
                    ui->radioButton_ptt_method_dtr->setDown(false);
                    ui->radioButton_ptt_method_cat->setDown(false);
                    ui->radioButton_ptt_method_rts->setDown(false);
                    break;
                }
            }
        }

        if (!tx_audio_src.isEmpty()) {
            qint16 int_val_tx_audio_src = 0;
            int_val_tx_audio_src = tx_audio_src.toInt();
            switch (int_val_tx_audio_src) {
            case 0:
                ui->radioButton_tx_audio_src_rear_data->setChecked(true);
                break;
            case 1:
                ui->radioButton_tx_audio_src_front_mic->setChecked(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of TX Audio Source!").toStdString());
            }
        }

        if (!ptt_mode.isEmpty()) {
            qint16 int_val_ptt_mode = 0;
            int_val_ptt_mode = ptt_mode.toInt();
            switch (int_val_ptt_mode) {
            case 0:
                ui->radioButton_mode_none->setChecked(true);
                break;
            case 1:
                ui->radioButton_mode_usb->setChecked(true);
                break;
            case 2:
                ui->radioButton_mode_data_pkt->setChecked(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of PTT Mode!").toStdString());
            }
        } else {
            ui->radioButton_mode_none->setDown(true);
        }

        if (!split_operation.isEmpty()) {
            qint16 int_val_split_operation = 0;
            int_val_split_operation = split_operation.toInt();
            switch (int_val_split_operation) {
            case 0:
                ui->radioButton_split_none->setChecked(true);
                break;
            case 1:
                ui->radioButton_split_rig->setChecked(true);
                break;
            case 2:
                ui->radioButton_split_fake_it->setChecked(true);
                break;
            default:
                throw std::invalid_argument(tr("Invalid value for amount of Split Operation!").toStdString());
            }
        } else {
            ui->radioButton_split_none->setDown(true);
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
            ui->lineEdit_db_save_loc->setText(gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)));
        }

        //
        // Audio --> Configuration
        //
        bool conv_rx_audio_init_start = gkDekodeDb->boolStr(rx_audio_init_start.toStdString());
        ui->checkBox_init_rx_audio_upon_start->setChecked(conv_rx_audio_init_start);

        if (!inputSampleRateIdx.isEmpty()) {
            ui->comboBox_audio_input_sample_rate->setCurrentIndex(inputSampleRateIdx.toInt());
        }

        if (!inputChannelsIdx.isEmpty()) {
            ui->comboBox_soundcard_input_channels->setCurrentIndex(inputChannelsIdx.toInt());
        }

        if (!outputSampleRateIdx.isEmpty()) {
            ui->comboBox_audio_output_sample_rate->setCurrentIndex(outputSampleRateIdx.toInt());
        }

        if (!outputChannelsIdx.isEmpty()) {
            ui->comboBox_soundcard_output_channels->setCurrentIndex(outputChannelsIdx.toInt());
        }

        //
        // General --> Event Logger
        //
        if (!eventLogVerbIdx.isEmpty()) {
            const qint32 eventLogVerbIdxInt = eventLogVerbIdx.toInt();
            if (eventLogVerbIdxInt >= 0) {
                // We have a valid result!
                ui->comboBox_event_logger_general_verbosity->setCurrentIndex(eventLogVerbIdxInt);
            }
        }

        //
        // General --> User Interface
        //
        if (!msg_audio_notif.isEmpty()) {
            const bool msg_audio_notif_bool = gkDekodeDb->boolStr(msg_audio_notif.toStdString());
            ui->checkBox_new_msg_audio_notification->setChecked(msg_audio_notif_bool);
        }

        if (!fail_event_notif.isEmpty()) {
            const bool fail_event_notif_bool = gkDekodeDb->boolStr(fail_event_notif.toStdString());
            ui->checkBox_failed_event_audio_notification->setChecked(fail_event_notif_bool);
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
    try {
        if (index > 0) { // Make sure that we haven't selected the dummy retainer item, "N/A"!
            for (const auto &com_port_list: status_com_ports) {
                for (const auto &usb_port_list: available_usb_ports.toStdMap()) {
                    if (usb_port_list.first == ui->comboBox_com_port->currentData().toInt()) {
                        // A USB port has been found!
                        emit changeConnPort(usb_port_list.second, GkConnMethod::CAT);
                    } else if (com_port_list.port_info.portName() == ui->comboBox_com_port->currentData().toString()) {
                        // An RS232/Serial port has been found!
                        QString combined_str = QString("%1 [ PID: #%2 ]")
                                .arg(com_port_list.port_info.description())
                                .arg(QString::number(com_port_list.port_info.productIdentifier()));
                        ui->lineEdit_device_port_name->setText(combined_str);
                        emit changeConnPort(com_port_list.port_info.portName(), GkConnMethod::CAT);
                    }
                }
            }
        }
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
    try {
        if (index > 0) { // Make sure that we haven't selected the dummy retainer item, "N/A"!
            for (const auto &ptt_port_list: status_com_ports) {
                for (const auto &usb_port_list: available_usb_ports.toStdMap()) {
                    if (usb_port_list.first == ui->comboBox_ptt_method_port->currentData().toInt()) {
                        // A USB port has been found!
                        emit changeConnPort(usb_port_list.second, GkConnMethod::PTT);
                    } else if (ptt_port_list.port_info.portName() == ui->comboBox_ptt_method_port->currentData().toString()) {
                        // An RS232/Serial port has been found!
                        QString combined_str = QString("%1 [ PID: #%2 ]")
                                .arg(ptt_port_list.port_info.description())
                                .arg(QString::number(ptt_port_list.port_info.productIdentifier()));
                        ui->lineEdit_device_port_name->setText(combined_str);
                        emit changeConnPort(ptt_port_list.port_info.portName(), GkConnMethod::PTT);
                    }
                }
            }
        }
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
                                                        gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation), true),
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
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_refresh_audio_devices_clicked refreshes the enumerated audio devices (both input and output) that are
 * found within the host operating system, and which are then displayed to the end-user for setting their options.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_refresh_audio_devices_clicked()
{
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
        supportedInputSampleRates.clear();
        const qint32 idx = ui->comboBox_soundcard_input->currentIndex();
        if (!avail_portaudio_api.isEmpty() || !avail_input_audio_devs.isEmpty()) {
            for (const auto &device: avail_input_audio_devs.toStdMap()) {
                if (device.first == idx) {
                    chosen_input_audio_dev = device.second;
                    supportedInputSampleRates.clear();
                    auto supported_rates = gkAudioDevices->enumSupportedStdSampleRates(&device.second.stream_parameters, standardSampleRates, false);
                    if (!supported_rates.empty()) {
                        for (const auto &sampleRate: supported_rates) {
                            const PaError support = sampleRate.second;
                            if (support == paFormatIsSupported) {
                                // This sample rate is supported!
                                supportedInputSampleRates.emplace_back(sampleRate.first);
                            }
                        }

                        chosen_input_audio_dev.supp_sample_rates = supportedInputSampleRates;
                        for (auto &input_dev: avail_input_audio_devs.toStdMap()) {
                            if (input_dev.first == idx) {
                                input_dev.second.supp_sample_rates = chosen_input_audio_dev.supp_sample_rates;
                                break;
                            }
                        }

                        update_sample_rates(false);
                    }
                }
            }

            return;
        }
    } catch (const portaudio::PaException &e) {
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
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
        supportedOutputSampleRates.clear();
        const qint32 idx = ui->comboBox_soundcard_output->currentIndex();
        if (!avail_portaudio_api.isEmpty() || !avail_output_audio_devs.isEmpty()) {
            for (const auto &device: avail_output_audio_devs.toStdMap()) {
                if (device.first == idx) {
                    chosen_output_audio_dev = device.second;
                    supportedOutputSampleRates.clear();
                    auto supported_rates = gkAudioDevices->enumSupportedStdSampleRates(&device.second.stream_parameters, standardSampleRates, true);
                    if (!supported_rates.empty()) {
                        for (const auto &sampleRate: supported_rates) {
                            const PaError support = sampleRate.second;
                            if (support == paFormatIsSupported) {
                                // This sample rate is supported!
                                supportedOutputSampleRates.emplace_back(sampleRate.first);
                            }
                        }

                        chosen_output_audio_dev.supp_sample_rates = supportedOutputSampleRates;
                        for (auto &output_dev: avail_output_audio_devs.toStdMap()) {
                            if (output_dev.first == idx) {
                                output_dev.second.supp_sample_rates = chosen_output_audio_dev.supp_sample_rates;
                                break;
                            }
                        }

                        update_sample_rates(true);
                    }
                }
            }

            return;
        }
    } catch (const portaudio::PaException &e) {
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_soundcard_api_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void DialogSettings::on_comboBox_soundcard_api_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    try {
        chosen_portaudio_underlying_api = ui->comboBox_soundcard_api->currentData().toInt();
        if (!avail_portaudio_api.isEmpty() || !avail_input_audio_devs.isEmpty()) {
            //
            // Input audio devices
            //
            ui->comboBox_soundcard_input->clear();
            quint32 input_counter = 0;
            for (const auto &pa_api: avail_portaudio_api.toStdMap()) {
                for (const auto &device: avail_input_audio_devs.toStdMap()) {
                    GkDevice input_dev = device.second;
                    if ((pa_api.first == ui->comboBox_soundcard_api->currentData().toInt()) && (input_dev.host_type_id == pa_api.second)) {
                        std::string audio_dev_name = input_dev.device_info.name;
                        if (!audio_dev_name.empty()) {
                            ui->comboBox_soundcard_input->insertItem(input_counter, QString::fromStdString(audio_dev_name),
                                                                     input_dev.device_info.name);
                            ++input_counter;
                        }
                    }
                }
            }
        }

        if (!avail_portaudio_api.isEmpty() || !avail_output_audio_devs.isEmpty()) {
            //
            // Output audio devices
            //
            ui->comboBox_soundcard_output->clear();
            quint32 output_counter = 0;
            for (const auto &pa_api: avail_portaudio_api.toStdMap()) {
                for (const auto &device: avail_output_audio_devs.toStdMap()) {
                    GkDevice output_dev = device.second;
                    if ((pa_api.first == ui->comboBox_soundcard_api->currentData().toInt()) && (output_dev.host_type_id == pa_api.second)) {
                        std::string audio_dev_name = output_dev.device_info.name;
                        if (!audio_dev_name.empty()) {
                            ui->comboBox_soundcard_output->insertItem(output_counter, QString::fromStdString(audio_dev_name),
                                                                      output_dev.device_info.name);
                            ++output_counter;
                        }
                    }
                }
            }
        }
    } catch (const portaudio::PaException &e) {
        QString error_msg = tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const portaudio::PaCppException &e) {
        QString error_msg = tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
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

void DialogSettings::on_horizontalSlider_encoding_audio_quality_valueChanged(int value)
{
    audio_quality_val = ((double)value / 10);
    std::ostringstream oss;
    oss << std::setprecision(2) << audio_quality_val;
    ui->label_encoding_audio_quality_value->setText(QString::fromStdString(oss.str()));

    return;
}

void DialogSettings::disableUsbPorts(const bool &disable)
{
    if (!disable) {
        usb_ports_active = true;
        enable_device_port_options();
    } else {
        usb_ports_active = false;
    }

    return;
}

void DialogSettings::disableComPorts(const bool &disable)
{
    if (!disable) {
        com_ports_active = true;
        enable_device_port_options();
    } else {
        com_ports_active = false;
    }

    return;
}

void DialogSettings::on_comboBox_rig_selection_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    emit recvRigCapabilities(ui->comboBox_rig_selection->currentData().toInt(), gkRadioPtr);

    if (gkRadioPtr->capabilities != nullptr) {
        ui->comboBox_baud_rate->setCurrentIndex(gkRadioLibs->convertBaudRateFromEnum(gkRadioLibs->convertBaudRateIntToEnum(gkRadioPtr->capabilities->serial_rate_min)));

        switch (gkRadioPtr->capabilities->serial_stop_bits) {
        case 0:
            ui->radioButton_stop_bits_default->setDown(true);
            ui->radioButton_stop_bits_one->setDown(false);
            ui->radioButton_stop_bits_two->setDown(false);
            break;
        case 1:
            ui->radioButton_stop_bits_one->setDown(true);
            ui->radioButton_stop_bits_default->setDown(false);
            ui->radioButton_stop_bits_two->setDown(false);
            break;
        case 2:
            ui->radioButton_stop_bits_two->setDown(true);
            ui->radioButton_stop_bits_one->setDown(false);
            ui->radioButton_stop_bits_default->setDown(false);
            break;
        default:
            throw std::invalid_argument(tr("Invalid value for amount of Stop Bits!").toStdString());
        }

        switch (gkRadioPtr->capabilities->serial_data_bits) {
        case 0:
            ui->radioButton_data_bits_default->setDown(true);
            ui->radioButton_data_bits_seven->setDown(false);
            ui->radioButton_data_bits_eight->setDown(false);
            break;
        case 7:
            ui->radioButton_data_bits_seven->setDown(true);
            ui->radioButton_data_bits_default->setDown(false);
            ui->radioButton_data_bits_eight->setDown(false);
            break;
        case 8:
            ui->radioButton_data_bits_eight->setDown(true);
            ui->radioButton_data_bits_seven->setDown(false);
            ui->radioButton_data_bits_default->setDown(false);
            break;
        default:
            throw std::invalid_argument(tr("Invalid value for amount of Data Bits!").toStdString());
        }

        switch (gkRadioPtr->capabilities->serial_handshake) {
        case serial_handshake_e::RIG_HANDSHAKE_NONE:
            ui->radioButton_handshake_none->setDown(true);
            ui->radioButton_handshake_xon_xoff->setDown(false);
            ui->radioButton_handshake_hardware->setDown(false);
            ui->radioButton_handshake_default->setDown(false);
            break;
        case serial_handshake_e::RIG_HANDSHAKE_XONXOFF:
            ui->radioButton_handshake_xon_xoff->setDown(true);
            ui->radioButton_handshake_hardware->setDown(false);
            ui->radioButton_handshake_default->setDown(false);
            ui->radioButton_handshake_none->setDown(false);
            break;
        case serial_handshake_e::RIG_HANDSHAKE_HARDWARE:
            ui->radioButton_handshake_hardware->setDown(true);
            ui->radioButton_handshake_default->setDown(false);
            ui->radioButton_handshake_none->setDown(false);
            ui->radioButton_handshake_xon_xoff->setDown(false);
            break;
        default:
            ui->radioButton_handshake_default->setDown(true);
            ui->radioButton_handshake_none->setDown(false);
            ui->radioButton_handshake_xon_xoff->setDown(false);
            ui->radioButton_handshake_hardware->setDown(false);
            break;
        }

        switch (gkRadioPtr->capabilities->ptt_type) {
        case ptt_type_t::RIG_PTT_RIG_MICDATA:
            ui->radioButton_ptt_method_vox->setDown(true);
            ui->radioButton_ptt_method_dtr->setDown(false);
            ui->radioButton_ptt_method_cat->setDown(false);
            ui->radioButton_ptt_method_rts->setDown(false);
            break;
        case ptt_type_t::RIG_PTT_SERIAL_DTR:
            ui->radioButton_ptt_method_dtr->setDown(true);
            ui->radioButton_ptt_method_cat->setDown(false);
            ui->radioButton_ptt_method_rts->setDown(false);
            ui->radioButton_ptt_method_vox->setDown(false);
            break;
        case ptt_type_t::RIG_PTT_RIG:
            ui->radioButton_ptt_method_cat->setDown(true);
            ui->radioButton_ptt_method_rts->setDown(false);
            ui->radioButton_ptt_method_vox->setDown(false);
            ui->radioButton_ptt_method_dtr->setDown(false);
            break;
        case ptt_type_t::RIG_PTT_SERIAL_RTS:
            ui->radioButton_ptt_method_rts->setDown(true);
            ui->radioButton_ptt_method_vox->setDown(false);
            ui->radioButton_ptt_method_dtr->setDown(false);
            ui->radioButton_ptt_method_cat->setDown(false);
            break;
        default:
            ui->radioButton_ptt_method_vox->setDown(true);
            ui->radioButton_ptt_method_dtr->setDown(false);
            ui->radioButton_ptt_method_cat->setDown(false);
            ui->radioButton_ptt_method_rts->setDown(false);
        }

        ui->radioButton_mode_none->setDown(true);

        ui->radioButton_split_none->setDown(true);
    }

    return;
}

void DialogSettings::on_radioButton_data_bits_default_clicked()
{
    ui->radioButton_data_bits_default->setChecked(true);
    ui->radioButton_data_bits_seven->setChecked(false);
    ui->radioButton_data_bits_eight->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_data_bits_seven_clicked()
{
    ui->radioButton_data_bits_seven->setChecked(true);
    ui->radioButton_data_bits_default->setChecked(false);
    ui->radioButton_data_bits_eight->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_data_bits_eight_clicked()
{
    ui->radioButton_data_bits_eight->setChecked(true);
    ui->radioButton_data_bits_default->setChecked(false);
    ui->radioButton_data_bits_seven->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_stop_bits_default_clicked()
{
    ui->radioButton_stop_bits_default->setChecked(true);
    ui->radioButton_stop_bits_one->setChecked(false);
    ui->radioButton_stop_bits_two->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_stop_bits_one_clicked()
{
    ui->radioButton_stop_bits_one->setChecked(true);
    ui->radioButton_stop_bits_default->setChecked(false);
    ui->radioButton_stop_bits_two->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_stop_bits_two_clicked()
{
    ui->radioButton_stop_bits_two->setChecked(true);
    ui->radioButton_stop_bits_one->setChecked(false);
    ui->radioButton_stop_bits_default->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_handshake_default_clicked()
{
    ui->radioButton_handshake_default->setChecked(true);
    ui->radioButton_handshake_none->setChecked(false);
    ui->radioButton_handshake_xon_xoff->setChecked(false);
    ui->radioButton_handshake_hardware->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_handshake_none_clicked()
{
    ui->radioButton_handshake_default->setChecked(false);
    ui->radioButton_handshake_none->setChecked(true);
    ui->radioButton_handshake_xon_xoff->setChecked(false);
    ui->radioButton_handshake_hardware->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_handshake_xon_xoff_clicked()
{
    ui->radioButton_handshake_default->setChecked(false);
    ui->radioButton_handshake_none->setChecked(false);
    ui->radioButton_handshake_xon_xoff->setChecked(true);
    ui->radioButton_handshake_hardware->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_handshake_hardware_clicked()
{
    ui->radioButton_handshake_default->setChecked(false);
    ui->radioButton_handshake_none->setChecked(false);
    ui->radioButton_handshake_xon_xoff->setChecked(false);
    ui->radioButton_handshake_hardware->setChecked(true);

    return;
}

void DialogSettings::on_radioButton_ptt_method_vox_clicked()
{
    ui->radioButton_ptt_method_vox->setChecked(true);
    ui->radioButton_ptt_method_dtr->setChecked(false);
    ui->radioButton_ptt_method_cat->setChecked(false);
    ui->radioButton_ptt_method_rts->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_ptt_method_dtr_clicked()
{
    ui->radioButton_ptt_method_vox->setChecked(false);
    ui->radioButton_ptt_method_dtr->setChecked(true);
    ui->radioButton_ptt_method_cat->setChecked(false);
    ui->radioButton_ptt_method_rts->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_ptt_method_cat_clicked()
{
    ui->radioButton_ptt_method_vox->setChecked(false);
    ui->radioButton_ptt_method_dtr->setChecked(false);
    ui->radioButton_ptt_method_cat->setChecked(true);
    ui->radioButton_ptt_method_rts->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_ptt_method_rts_clicked()
{
    ui->radioButton_ptt_method_vox->setChecked(false);
    ui->radioButton_ptt_method_dtr->setChecked(false);
    ui->radioButton_ptt_method_cat->setChecked(false);
    ui->radioButton_ptt_method_rts->setChecked(true);

    return;
}

void DialogSettings::on_radioButton_tx_audio_src_rear_data_clicked()
{
    ui->radioButton_tx_audio_src_rear_data->setChecked(true);
    ui->radioButton_tx_audio_src_front_mic->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_tx_audio_src_front_mic_clicked()
{
    ui->radioButton_tx_audio_src_rear_data->setChecked(false);
    ui->radioButton_tx_audio_src_front_mic->setChecked(true);

    return;
}

void DialogSettings::on_radioButton_mode_none_clicked()
{
    ui->radioButton_mode_none->setChecked(true);
    ui->radioButton_mode_usb->setChecked(false);
    ui->radioButton_mode_data_pkt->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_mode_usb_clicked()
{
    ui->radioButton_mode_none->setChecked(false);
    ui->radioButton_mode_usb->setChecked(true);
    ui->radioButton_mode_data_pkt->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_mode_data_pkt_clicked()
{
    ui->radioButton_mode_none->setChecked(false);
    ui->radioButton_mode_usb->setChecked(false);
    ui->radioButton_mode_data_pkt->setChecked(true);

    return;
}

void DialogSettings::on_radioButton_split_none_clicked()
{
    ui->radioButton_split_none->setChecked(true);
    ui->radioButton_split_rig->setChecked(false);
    ui->radioButton_split_fake_it->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_split_rig_clicked()
{
    ui->radioButton_split_none->setChecked(false);
    ui->radioButton_split_rig->setChecked(true);
    ui->radioButton_split_fake_it->setChecked(false);

    return;
}

void DialogSettings::on_radioButton_split_fake_it_clicked()
{
    ui->radioButton_split_none->setChecked(false);
    ui->radioButton_split_rig->setChecked(false);
    ui->radioButton_split_fake_it->setChecked(true);

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

void DialogSettings::on_spinBox_spectro_min_freq_valueChanged(int arg1)
{
    ui->horizontalSlider_spectro_min_freq->setValue(arg1);

    return;
}

void DialogSettings::on_spinBox_spectro_max_freq_valueChanged(int arg1)
{
    ui->horizontalSlider_spectro_max_freq->setValue(arg1);

    return;
}

void DialogSettings::on_horizontalSlider_spectro_min_freq_sliderMoved(int position)
{
    ui->spinBox_spectro_min_freq->setValue(position);

    return;
}

void DialogSettings::on_horizontalSlider_spectro_max_freq_sliderMoved(int position)
{
    ui->spinBox_spectro_max_freq->setValue(position);

    return;
}

void DialogSettings::on_horizontalSlider_spectro_min_freq_valueChanged(int value)
{
    ui->spinBox_spectro_min_freq->setValue(value);

    return;
}

void DialogSettings::on_horizontalSlider_spectro_max_freq_valueChanged(int value)
{
    ui->spinBox_spectro_max_freq->setValue(value);

    return;
}

void DialogSettings::on_pushButton_freq_list_new_clicked()
{
    return;
}

void DialogSettings::on_pushButton_freq_list_edit_clicked()
{
    return;
}

void DialogSettings::on_pushButton_freq_list_delete_clicked()
{
    QPointer<QItemSelectionModel> select = ui->tableView_working_freqs->selectionModel();
    if (select->hasSelection()) {
        gkFreqTableModel->removeRows(select->currentIndex().row(), 1, QModelIndex());
    }

    return;
}

void DialogSettings::on_pushButton_freq_list_print_clicked()
{
    return;
}

void DialogSettings::on_doubleSpinBox_freq_calib_intercept_valueChanged(double arg1)
{
    Q_UNUSED(arg1);

    return;
}

void DialogSettings::on_doubleSpinBox_freq_calib_slope_valueChanged(double arg1)
{
    Q_UNUSED(arg1);

    return;
}

void DialogSettings::on_checkBox_new_msg_audio_notification_stateChanged(int arg1)
{
    gkDekodeDb->write_general_settings(QString::number(arg1), general_stat_cfg::MsgAudioNotif);

    return;
}

void DialogSettings::on_checkBox_failed_event_audio_notification_stateChanged(int arg1)
{
    gkDekodeDb->write_general_settings(QString::number(arg1), general_stat_cfg::FailAudioNotif);

    return;
}

void DialogSettings::on_pushButton_access_stt_speak_clicked()
{
    return;
}

void DialogSettings::on_pushButton_access_stt_pause_clicked()
{
    return;
}

void DialogSettings::on_pushButton_access_stt_enable_clicked()
{
    return;
}

void DialogSettings::on_pushButton_access_stt_resume_clicked()
{
    return;
}

void DialogSettings::on_pushButton_access_stt_stop_clicked()
{
    return;
}

void DialogSettings::on_horizontalSlider_access_stt_volume_valueChanged(int value)
{
    return;
}

void DialogSettings::on_horizontalSlider_access_stt_rate_valueChanged(int value)
{
    return;
}

void DialogSettings::on_horizontalSlider_access_stt_pitch_valueChanged(int value)
{
    return;
}

void DialogSettings::on_comboBox_access_stt_engine_currentIndexChanged(int index)
{
    QString engineName = ui->comboBox_access_stt_engine->itemData(index).toString();
    emit changeSelectedTTSEngine(engineName);

    return;
}

void DialogSettings::on_comboBox_access_stt_language_currentIndexChanged(int index)
{
    return;
}

void DialogSettings::on_comboBox_access_stt_preset_voice_currentIndexChanged(int index)
{
    return;
}

/**
 * @brief DialogSettings::ttsLocaleChanged
 * @param locale
 */
void DialogSettings::ttsLocaleChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    ui->comboBox_access_stt_language->setCurrentIndex(ui->comboBox_access_stt_language->findData(localeVariant));
    ui->comboBox_access_stt_preset_voice->clear();

    return;
}

/**
 * @brief DialogSettings::ttsAddLanguageItem
 * @param name
 * @param locale
 */
void DialogSettings::ttsAddLanguageItem(const QString &name, const QVariant &locale)
{
    ui->comboBox_access_stt_language->addItem(name, locale);

    return;
}

/**
 * @brief DialogSettings::ttsAddPresetVoiceItem
 * @param name
 * @param locale
 */
void DialogSettings::ttsAddPresetVoiceItem(const QString &name, const QVariant &locale)
{
    Q_UNUSED(locale);
    ui->comboBox_access_stt_preset_voice->addItem(name);

    return;
}

/**
 * @brief DialogSettings::ascertConnType ascertains the connection type used whether it be USB, RS232, Serial, Parallel,
 * or something more exotic.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_ptt Is this a PTT connection type or not?
 * @return The ascertained connection type in question.
 */
GkConnType DialogSettings::ascertConnType(const bool &is_ptt)
{
    if (!is_ptt) {
        //
        // CAT
        //
        const qint32 cat_idx = ui->comboBox_com_port->currentIndex();
        const QString cat_str = ui->comboBox_com_port->currentText();
        for (const auto &port: status_com_ports) {
            if (cat_idx == port.port_info.portName()) {
                return GkConnType::GkRS232;
            }
        }

        for (const auto &usb: status_usb_devices.toStdMap()) {
            if (cat_str == usb.second.name) {
                return GkConnType::GkUSB;
            }
        }

        return GkConnType::GkNone;
    } else {
        //
        // PTT
        //
        const qint32 ptt_idx = ui->comboBox_ptt_method_port->currentIndex();
        const QString ptt_str = ui->comboBox_ptt_method_port->currentText();
        for (const auto &port: status_com_ports) {
            if (ptt_idx == port.port_info.portName()) {
                return GkConnType::GkRS232;
            }
        }

        for (const auto &usb: status_usb_devices.toStdMap()) {
            if (ptt_str == usb.second.name) {
                return GkConnType::GkUSB;
            }
        }

        return GkConnType::GkNone;
    }

    return GkUSB;
}
