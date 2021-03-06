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

#include "dialogsettings.hpp"
#include "ui_dialogsettings.h"
#include "src/gk_sinewave.hpp"
#include "src/ui/xmpp/gkxmppregistrationdialog.hpp"
#include <hamlib/rig.h>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <QByteArray>
#include <QStringList>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTableWidgetItem>
#include <QRegularExpression>
#include <exception>
#include <set>
#include <iomanip>
#include <cstdlib>

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Mapping;
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
using namespace GkSdr;

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
                               QPointer<GkAudioDevices> audioDevices,
                               const std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> sysInputDevs,
                               const std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> sysOutputDevs,
                               QPointer<RadioLibs> radioLibs, QPointer<StringFuncs> stringFuncs,
                               std::shared_ptr<GkRadio> radioPtr,
                               const std::vector<GekkoFyre::Database::Settings::GkComPort> &com_ports,
                               const QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> &usbPortMap,
                               QPointer<GkFrequencies> gkFreqList,
                               QPointer<GkFreqTableModel> freqTableModel,
                               const GkUserConn &connection_details,
                               QPointer<GekkoFyre::GkXmppClient> xmppClient,
                               QPointer<GekkoFyre::GkEventLogger> eventLogger,
                               QPointer<Marble::MarbleWidget> mapWidget,
                               QPointer<GekkoFyre::GkSdrDev> gkSdrPtr,
                               const System::UserInterface::GkSettingsDlgTab &settingsDlgTab,
                               QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

    try {
        //
        // Initialize Audio System for Settings Dialog!
        //
        gkRadioLibs = std::move(radioLibs);
        gkStringFuncs = std::move(stringFuncs);
        gkDekodeDb = std::move(dkDb);
        gkFileIo = std::move(filePtr);
        gkAudioDevices = std::move(audioDevices);
        gkRadioPtr = std::move(radioPtr);
        gkFreqs = std::move(gkFreqList);
        gkFreqTableModel = std::move(freqTableModel);
        m_xmppClient = std::move(xmppClient);
        gkEventLogger = std::move(eventLogger);
        m_mapWidget = std::move(mapWidget);
        gkSdrDev = std::move(gkSdrPtr);
        gkSerialPortMap = com_ports;

        //
        // Initialize audio device information!
        gkSysInputDevs = sysInputDevs;
        gkSysOutputDevs = sysOutputDevs;

        //
        // QXmpp and XMPP related
        //
        gkConnDetails = connection_details;
        prefill_uri_lookup_method();

        //
        // Hunspell & Spelling dictionaries
        //
        prefill_lang_dictionaries();

        //
        // Mapping and atlas APIs, etc.
        ui->checkBox_rig_gps_dd->setChecked(true);
        gkAtlasDlg = new GkAtlasDialog(gkEventLogger, m_mapWidget, this);

        QObject::connect(gkAtlasDlg, SIGNAL(geoFocusPoint(const Marble::GeoDataCoordinates &)), this, SLOT(getGeoFocusPoint(const Marble::GeoDataCoordinates &)));
        QObject::connect(this, SIGNAL(setGpsCoords(const QGeoCoordinate &)), this, SLOT(saveGpsCoords(const QGeoCoordinate &)));
        QObject::connect(this, SIGNAL(setGpsCoords(const qreal &, const qreal &)), this, SLOT(saveGpsCoords(const qreal &, const qreal &)));
        QObject::connect(&m_gpsCoordEditTimer, SIGNAL(stop()), this, SLOT(gpsCoordsTimerProc()));

        //
        // Initialize any profile fields related to mapping, geography, etc.
        readGpsCoords();

        //
        // Enumerate any discovered SDR devices (and applicable information)!
        discSoapySdrDevs();

        //
        // Miscellaneous
        //
        gkSettingsDlgTab = settingsDlgTab;
        switch (gkSettingsDlgTab) {
            case System::UserInterface::GkSettingsDlgTab::GkGeneralStation:
                ui->tabWidget_config->setCurrentWidget(ui->tab_0_general);
                ui->tabWidget_config_general->setCurrentWidget(ui->tab_general_rig_station);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkGeneralFreq:
                ui->tabWidget_config->setCurrentWidget(ui->tab_0_general);
                ui->tabWidget_config_general->setCurrentWidget(ui->tab_general_ui_radio_freqs);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkGeneralUi:
                ui->tabWidget_config->setCurrentWidget(ui->tab_0_general);
                ui->tabWidget_config_general->setCurrentWidget(ui->tab_general_app_ui);
                ui->tabWidget_app_ui->setCurrentWidget(ui->tab_app_ui_basic_accessibility);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkGeneralUiBasic:
                ui->tabWidget_config->setCurrentWidget(ui->tab_0_general);
                ui->tabWidget_config_general->setCurrentWidget(ui->tab_general_app_ui);
                ui->tabWidget_app_ui->setCurrentWidget(ui->tab_app_ui_basic_accessibility);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkGeneralUiThemes:
                ui->tabWidget_config->setCurrentWidget(ui->tab_0_general);
                ui->tabWidget_config_general->setCurrentWidget(ui->tab_general_app_ui);
                ui->tabWidget_app_ui->setCurrentWidget(ui->tabWidget_app_ui); //-V678
                break;
            case System::UserInterface::GkSettingsDlgTab::GkGeneralXmpp:
                ui->tabWidget_config->setCurrentWidget(ui->tab_0_general);
                ui->tabWidget_config_general->setCurrentWidget(ui->tab_general_xmpp);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkGeneralEventLogger:
                ui->tabWidget_config->setCurrentWidget(ui->tab_0_general);
                ui->tabWidget_config_general->setCurrentWidget(ui->tab_general_event_logger);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkAudio:
                ui->tabWidget_config->setCurrentWidget(ui->tab_1_audio);
                ui->tabWidget_audio->setCurrentWidget(ui->tab_audio_config);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkAudioConfig:
                ui->tabWidget_config->setCurrentWidget(ui->tab_1_audio);
                ui->tabWidget_audio->setCurrentWidget(ui->tab_audio_config);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkAudioRecorder:
                ui->tabWidget_config->setCurrentWidget(ui->tab_1_audio);
                ui->tabWidget_audio->setCurrentWidget(ui->tab_audio_encoding_settings);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkAudioApiInfo:
                ui->tabWidget_config->setCurrentWidget(ui->tab_1_audio);
                ui->tabWidget_audio->setCurrentWidget(ui->tabWidget_audio); //-V678
                break;
            case System::UserInterface::GkSettingsDlgTab::GkRadio:
                ui->tabWidget_config->setCurrentWidget(ui->tab_2_rig);
                ui->tabWidget_radio->setCurrentWidget(ui->tab_radio_rig);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkRadioRig:
                ui->tabWidget_config->setCurrentWidget(ui->tab_2_rig);
                ui->tabWidget_radio->setCurrentWidget(ui->tab_radio_rig);
                break;
            case System::UserInterface::GkSettingsDlgTab::GkRadioApiInfo:
                ui->tabWidget_config->setCurrentWidget(ui->tab_2_rig);
                ui->tabWidget_radio->setCurrentWidget(ui->tab_radio_hamlib_api);
                break;
            default:
                ui->tabWidget_config->setCurrentWidget(ui->tab_0_general);
                ui->tabWidget_config_general->setCurrentWidget(ui->tab_general_rig_station);
                break;
        }

        //
        // Set default placeholder text, dependent on whether we are within Microsoft Windows or Linux!
        #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
        ui->lineEdit_xmpp_upload_avatar_file_browser->setPlaceholderText(tr(R"("C:\Users\ExampleUser\Pictures\avatar.png")"));
        #elif __linux__
        ui->lineEdit_xmpp_upload_avatar_file_browser->setPlaceholderText(tr(R"("/home/ExampleUser/Pictures/avatar.png")"));
        #endif

        //
        // Fill QComboBox with default items as pertaining to XMPP configurations!
        prefill_xmpp_server_type(GkXmpp::GkServerType::GekkoFyre);
        prefill_xmpp_server_type(GkXmpp::GkServerType::Custom);
        prefill_xmpp_ignore_ssl_errors();

        usb_ports_active = false;
        com_ports_active = false;
        audio_quality_val = 0.0;

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
        gkUsbPortMap = usbPortMap;
        prefill_avail_com_ports(gkSerialPortMap);
        prefill_avail_usb_ports(gkUsbPortMap);

        // Select the initial COM port and load it into memory
        on_comboBox_com_port_currentIndexChanged(ui->comboBox_com_port->currentIndex());
        on_comboBox_ptt_method_port_currentIndexChanged(ui->comboBox_ptt_method_port->currentIndex());

        //
        // Initialize Audio System libraries!
        //
        prefill_audio_encode_comboboxes();
        prefill_audio_devices();

        ui->label_pa_version->setText(gkAudioDevices->rtAudioVersionNumber());
        ui->plainTextEdit_pa_version_text->setPlainText(gkAudioDevices->rtAudioVersionText());

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
        // Initialize Text-to-Speech signals/slots
        //
        // QObject::connect(ui->pushButton_access_stt_speak, SIGNAL(clicked()), gkTextToSpeech, SLOT(speak()));
        // QObject::connect(ui->horizontalSlider_access_stt_pitch, SIGNAL(valueChanged(int)), gkTextToSpeech, SLOT(setPitch(const int &)));
        // nQObject::connect(ui->horizontalSlider_access_stt_rate, SIGNAL(valueChanged(int)), gkTextToSpeech, SLOT(setRate(const int &)));
        // QObject::connect(ui->horizontalSlider_access_stt_volume, SIGNAL(valueChanged(int)), gkTextToSpeech, SLOT(setVolume(const int &)));
        // QObject::connect(ui->comboBox_access_stt_engine, SIGNAL(currentIndexChanged(int)), gkTextToSpeech, SLOT(engineSelected(int)));
        // QObject::connect(this, SIGNAL(changeSelectedTTSEngine(const QString &)), gkTextToSpeech, SLOT(engineSelected(const QString &)));
        // QObject::connect(gkTextToSpeech, SIGNAL(addLangItem(const QString &, const QVariant &)), this, SLOT(ttsAddLanguageItem(const QString &, const QVariant &)));
        // QObject::connect(gkTextToSpeech, SIGNAL(addVoiceItem(const QString &, const QVariant &)), this, SLOT(ttsAddPresetVoiceItem(const QString &, const QVariant &)));
        // QObject::connect(gkTextToSpeech, SIGNAL(clearLangItems()), ui->comboBox_access_stt_language, SLOT(clear()));
        // QObject::connect(gkTextToSpeech, SIGNAL(clearVoiceItems()), ui->comboBox_access_stt_preset_voice, SLOT(clear()));
        // QObject::connect(gkTextToSpeech, SIGNAL(setVoiceCurrentIndex(int)), ui->comboBox_access_stt_preset_voice, SLOT(setCurrentIndex(int)));
        // QObject::connect(gkTextToSpeech, SLOT(localeChanged(const QLocale &)), this, SLOT(ttsLocaleChanged(const QLocale &)));
        // QObject::connect(ui->comboBox_access_stt_preset_voice, SIGNAL(currentIndexChanged(int)), gkTextToSpeech, SLOT(voiceSelected(int)));
        // QObject::connect(ui->comboBox_access_stt_language, SIGNAL(currentIndexChanged(int)), gkTextToSpeech, SLOT(languageSelected(int)));

        //
        // Validate inputs for the Email Address
        //
        QRegularExpression rxEmail(R"(\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,4}\b)",
                                   QRegularExpression::CaseInsensitiveOption);
        ui->lineEdit_xmpp_client_email_address->setValidator(new QRegularExpressionValidator(rxEmail, this));

        //
        // QXmpp variables and widgets
        ui->dateEdit_cfg_msg_archiving_timespan->setDisplayFormat(General::Xmpp::dateTimeFormatting);
        ui->dateEdit_cfg_msg_archiving_timespan->setDateTime(QDateTime::currentDateTime().toLocalTime());
        ui->dateEdit_cfg_msg_archiving_timespan->setMinimumDateTime(QDateTime::currentDateTime().addYears(GK_XMPP_MAM_MIN_DATETIME_YEARS).toLocalTime());
        xmppUpdateCalendarThread = std::thread(&DialogSettings::xmppUpdateCalendarDateTime, this, QDateTime::currentDateTime().toLocalTime());
        xmppUpdateCalendarThread.detach();

        //
        // Validate inputs for the Username
        //
        QRegularExpression rxUsername(R"(\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,4}\b)", QRegularExpression::CaseInsensitiveOption);
        ui->lineEdit_xmpp_client_username->setValidator(new QRegularExpressionValidator(rxUsername, this));
    } catch (const std::exception &e) {
        QString error_msg = tr("A generic exception has occurred:\n\n%1").arg(e.what());
        gkEventLogger->publishEvent(error_msg, GkSeverity::Error, "", true, true);
    } catch (...) {
        QString error_msg = tr("An unknown exception has occurred. There are no further details.");
        gkEventLogger->publishEvent(error_msg, GkSeverity::Fatal, "", false, true, false, true);
    }
}

/**
 * @brief DialogSettings::~DialogSettings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
DialogSettings::~DialogSettings()
{
    if (xmppUpdateCalendarThread.joinable()) {
        xmppUpdateCalendarThread.join();
    }

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
        // General --> User Interface
        //
        const QString user_interface_ui_lang = ui->comboBox_accessibility_lang_ui->currentText();
        const QString user_interface_hunspell_dict = ui->comboBox_accessibility_dict->currentText();

        if (!user_interface_ui_lang.isEmpty()) {
            gkDekodeDb->write_lang_ui_settings(user_interface_ui_lang, Language::GkUiLang::ChosenUiLang);
        }

        if (!user_interface_hunspell_dict.isEmpty()) {
            gkDekodeDb->write_lang_dict_settings(user_interface_hunspell_dict, Language::GkDictionary::ChosenDictLang);
        }

        const qint32 ui_scale_pctg_val = ui->horizontalSlider_accessibility_appearance_ui_scale->value();
        if (ui_scale_pctg_val >= 50) { // 50% is the most minimum value for user interface scaling!
            gkDekodeDb->write_ui_settings(QString::number(ui_scale_pctg_val), Settings::GkUiCfg::GkUiScalePctg);
        }

        //
        // General --> XMPP --> Client Settings
        //
        bool xmpp_allow_msg_history = ui->checkBox_allow_msg_history->isChecked();
        bool xmpp_allow_file_xfers = ui->checkBox_allow_file_transfers->isChecked();
        bool xmpp_allow_mucs = ui->checkBox_allow_muc_creation->isChecked();
        bool xmpp_connect_auto = ui->checkBox_connect_automatically->isChecked();
        bool xmpp_reconnect_auto = ui->checkBox_automatic_reconnect->isChecked();
        QByteArray xmpp_upload_avatar; // TODO: Finish this area of code, pronto!

        QString xmpp_client_username = ui->lineEdit_xmpp_client_username->text();
        QString xmpp_client_password = ui->lineEdit_xmpp_client_password->text();
        QString xmpp_client_email_addr = ui->lineEdit_xmpp_client_email_address->text();
        QString xmpp_client_nickname = ui->lineEdit_xmpp_server_nickname->text();
        qint32 xmpp_network_timeout = ui->spinBox_xmpp_server_network_timeout->value();

        gkDekodeDb->write_xmpp_settings(QString::fromStdString(gkDekodeDb->boolEnum(xmpp_allow_msg_history)), GkXmppCfg::XmppAllowMsgHistory);
        gkDekodeDb->write_xmpp_settings(QString::fromStdString(gkDekodeDb->boolEnum(xmpp_allow_file_xfers)), GkXmppCfg::XmppAllowFileXfers);
        gkDekodeDb->write_xmpp_settings(QString::fromStdString(gkDekodeDb->boolEnum(xmpp_allow_mucs)), GkXmppCfg::XmppAllowMucs);
        gkDekodeDb->write_xmpp_settings(QString::fromStdString(gkDekodeDb->boolEnum(xmpp_connect_auto)), GkXmppCfg::XmppAutoConnect);
        gkDekodeDb->write_xmpp_settings(QString::fromStdString(gkDekodeDb->boolEnum(xmpp_reconnect_auto)), GkXmppCfg::XmppAutoReconnect);
        gkDekodeDb->write_xmpp_settings(QString::number(xmpp_network_timeout), GkXmppCfg::XmppNetworkTimeout);

        //
        // CAUTION!!! Username, password, and e-mail address!
        //
        gkDekodeDb->write_xmpp_settings(xmpp_client_username, GkXmppCfg::XmppUsername);
        gkDekodeDb->write_xmpp_settings(xmpp_client_password, GkXmppCfg::XmppPassword);
        gkDekodeDb->write_xmpp_settings(xmpp_client_email_addr, GkXmppCfg::XmppEmailAddr);
        gkDekodeDb->write_xmpp_settings(xmpp_client_nickname, GkXmppCfg::XmppNickname);

        //
        // General --> XMPP --> Server Settings
        //
        QString xmpp_host_url = ui->lineEdit_xmpp_server_url->text();
        qint32 xmpp_server_type = ui->comboBox_xmpp_server_type->currentIndex();
        quint16 xmpp_host_tcp_port = ui->spinBox_xmpp_server_port->value();
        bool xmpp_enable_ssl = ui->checkBox_xmpp_server_ssl->isChecked();
        qint32 xmpp_ignore_ssl_errors = ui->comboBox_xmpp_server_ssl_errors->currentIndex();
        qint32 xmpp_uri_lookup_method = ui->comboBox_xmpp_server_uri_lookup_method->currentIndex();

        if (xmpp_host_url != GkXmppGekkoFyreCfg::defaultUrl) {
            gkDekodeDb->write_xmpp_settings(xmpp_host_url, GkXmppCfg::XmppDomainUrl);
            gkDekodeDb->write_xmpp_settings(QString::number(xmpp_server_type), GkXmppCfg::XmppServerType);
            gkDekodeDb->write_xmpp_settings(QString::number(xmpp_host_tcp_port), GkXmppCfg::XmppDomainPort);
            gkDekodeDb->write_xmpp_settings(QString::fromStdString(gkDekodeDb->boolEnum(xmpp_enable_ssl)), GkXmppCfg::XmppEnableSsl);
            gkDekodeDb->write_xmpp_settings(QString::number(xmpp_ignore_ssl_errors), GkXmppCfg::XmppIgnoreSslErrors);
            gkDekodeDb->write_xmpp_settings(QString::number(xmpp_uri_lookup_method), GkXmppCfg::XmppUriLookupMethod);
        }

        //
        // Audio --> Configuration
        //
        const QString input_audio_dev = ui->comboBox_soundcard_input->itemData(ui->comboBox_soundcard_input->currentIndex()).toString();
        const QString output_audio_dev = ui->comboBox_soundcard_output->itemData(ui->comboBox_soundcard_output->currentIndex()).toString();

        if (!input_audio_dev.isEmpty()) { // Save input audio device name to Google LevelDB database!
            gkDekodeDb->write_audio_device_settings(input_audio_dev, GkAudioDevice::AudioInputDeviceName); // Make sure to get the user data of the QComboBox, as the viewable text could be truncated!
        }

        if (!output_audio_dev.isEmpty()) { // Save output audio device name to Google LevelDB database!
            gkDekodeDb->write_audio_device_settings(output_audio_dev, GkAudioDevice::AudioOutputDeviceName); // Make sure to get the user data of the QComboBox, as the viewable text could be truncated!
        }

        //
        // Audio --> Recorder
        //
        if (ui->comboBox_input_audio_dev_sample_rate->count() > 0) {
            const qint32 input_audio_dev_chosen_sample_rate_data = ui->comboBox_input_audio_dev_sample_rate->currentData().toInt();
            gkDekodeDb->write_misc_audio_settings(QString::number(input_audio_dev_chosen_sample_rate_data), GkAudioCfg::AudioInputSampleRate);
        }

        if (ui->comboBox_input_audio_dev_number_channels->count() > 0) {
            const qint32 input_audio_dev_chosen_number_channels_data = ui->comboBox_input_audio_dev_number_channels->currentData().toInt();
            gkDekodeDb->write_misc_audio_settings(QString::number(input_audio_dev_chosen_number_channels_data), GkAudioCfg::AudioInputChannels);
        }

        if (ui->comboBox_input_audio_dev_bitrate->count() > 0) {
            const qint32 input_audio_dev_chosen_format_bits_data = ui->comboBox_input_audio_dev_bitrate->currentData().toInt();
            gkDekodeDb->write_misc_audio_settings(QString::number(input_audio_dev_chosen_format_bits_data), GkAudioCfg::AudioInputBitrate);
        }

        //
        // Audio Format for input device!
        for (const auto &input_dev: gkSysInputDevs) {
            if (input_dev.audio_dev_str == ui->comboBox_soundcard_input->itemData(ui->comboBox_soundcard_input->currentIndex()).toString()) {
                gkDekodeDb->write_misc_audio_settings(QString::number(input_dev.pref_audio_format), GkAudioCfg::AudioInputFormat);
                break;
            }
        }

        bool rx_audio_init_start = ui->checkBox_init_rx_audio_upon_start->isChecked();

        //
        // Operating System's own Settings (e.g. the registry under Microsoft Windows)
        //
        // gkFileIo->write_initial_settings(Filesystem::fileName, init_cfg::DbName);
        // gkFileIo->write_initial_settings(ui->lineEdit_db_save_loc->text(), init_cfg::DbLoc);

        //
        // Now make the sound-device selection official throughout the running Small World Deluxe application!
        //
        for (const auto &input_dev: gkSysInputDevs) {
            if (input_dev.isEnabled) {
                emit changeInputAudioInterface(input_dev);
            }
        }

        for (const auto &output_dev: gkSysOutputDevs) {
            if (output_dev.isEnabled) {
                emit changeOutputAudioInterface(output_dev);
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

        qint16 enum_force_ctrl_lines_dtr = 0;
        qint16 enum_force_ctrl_lines_rts = 0;
        enum_force_ctrl_lines_dtr = ui->comboBox_force_ctrl_lines_dtr->currentIndex();
        enum_force_ctrl_lines_rts = ui->comboBox_force_ctrl_lines_rts->currentIndex();

        GkConnType conn_type_cat = assertConnType(false);
        GkConnType conn_type_ptt = assertConnType(true);

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

        emit addRigInUse(ui->comboBox_rig_selection->currentData().toInt(), gkRadioPtr);
        emit updateXmppConfig();

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
 * @brief DialogSettings::xmppUpdateCalendarDateTime updates the widget, `ui->dateEdit_cfg_msg_archiving_timespan()`,
 * with the latest timing and date information.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param startTime The time and/or date at which to start the updates from.
 */
void DialogSettings::xmppUpdateCalendarDateTime(const QDateTime &startTime)
{
    std::lock_guard<std::mutex> lock_guard(xmppUpdateCalendarMtx);
    ui->dateEdit_cfg_msg_archiving_timespan->setMaximumDateTime(QDateTime::currentDateTime().toLocalTime());
    std::this_thread::sleep_for(std::chrono::milliseconds(GK_XMPP_MAN_SLEEP_DATETIME_MILLISECS));

    return;
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
 * @brief DialogSettings::prefill_audio_devices Enumerates the audio deviecs on the user's
 * computer system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_devices The available audio devices on the user's system, as a typical std::vector.
 * @see GekkoFyre::GkAudioDevices::enumAudioDevices(), GkAudioDevices::filterPortAudioHostType().
 */
void DialogSettings::prefill_audio_devices()
{
    //
    // Enumerate output audio devices!
    for (const auto &output_dev: gkSysOutputDevs) {
        if (!output_dev.audio_dev_str.isEmpty()) {
            ui->comboBox_soundcard_output->addItem(gkStringFuncs->trimStrToCharLength(output_dev.audio_dev_str, GK_AUDIO_DEVS_STR_LENGTH, true),
                                                   output_dev.audio_dev_str);
        }
    }

    //
    // Enumerate input audio devices!
    for (const auto &input_dev: gkSysInputDevs) {
        if (!input_dev.audio_dev_str.isEmpty()) {
            ui->comboBox_soundcard_input->addItem(gkStringFuncs->trimStrToCharLength(input_dev.audio_dev_str, GK_AUDIO_DEVS_STR_LENGTH, true),
                                                  input_dev.audio_dev_str);
        }
    }

    return;
}

/**
 * @brief DialogSettings::prefill_audio_encode_comboboxes
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::prefill_audio_encode_comboboxes()
{
    //
    // Prefill QComboBox for sample rates!
    ui->comboBox_input_audio_dev_sample_rate->insertItem(GK_AUDIO_SAMPLE_RATE_8000_IDX, tr("8,000"), 8000);
    ui->comboBox_input_audio_dev_sample_rate->insertItem(GK_AUDIO_SAMPLE_RATE_11025_IDX, tr("11,025"), 11025);
    ui->comboBox_input_audio_dev_sample_rate->insertItem(GK_AUDIO_SAMPLE_RATE_22050_IDX, tr("22,050"), 22050);
    ui->comboBox_input_audio_dev_sample_rate->insertItem(GK_AUDIO_SAMPLE_RATE_32000_IDX, tr("32,000"), 32000);
    ui->comboBox_input_audio_dev_sample_rate->insertItem(GK_AUDIO_SAMPLE_RATE_44100_IDX, tr("44,100"), 44100);
    ui->comboBox_input_audio_dev_sample_rate->insertItem(GK_AUDIO_SAMPLE_RATE_48000_IDX, tr("48,000"), 48000);
    ui->comboBox_input_audio_dev_sample_rate->insertItem(GK_AUDIO_SAMPLE_RATE_88200_IDX, tr("88,200"), 88200);
    ui->comboBox_input_audio_dev_sample_rate->insertItem(GK_AUDIO_SAMPLE_RATE_96000_IDX, tr("96,000"), 96000);

    //
    // Prefill QComboBox for bit-rates!
    ui->comboBox_input_audio_dev_bitrate->insertItem(GK_AUDIO_BITRATE_8_IDX, "8", 8);
    ui->comboBox_input_audio_dev_bitrate->insertItem(GK_AUDIO_BITRATE_16_IDX, "16", 16);
    ui->comboBox_input_audio_dev_bitrate->insertItem(GK_AUDIO_BITRATE_24_IDX, "24", 24);

    ui->comboBox_input_audio_dev_number_channels->insertItem(GK_AUDIO_CHANNELS_MONO, tr("Mono"), 1);
    ui->comboBox_input_audio_dev_number_channels->insertItem(GK_AUDIO_CHANNELS_STEREO, tr("Stereo"), 2);

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
 * @brief DialogSettings::prefill_xmpp_server_type
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param server_type
 */
void DialogSettings::prefill_xmpp_server_type(const GkXmpp::GkServerType &server_type)
{
    switch (server_type) {
        case GkXmpp::GkServerType::GekkoFyre:
            ui->comboBox_xmpp_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX, tr("GekkoFyre Networks (best choice!)"));
            break;
        case GkXmpp::GkServerType::Custom:
            ui->comboBox_xmpp_server_type->insertItem(GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX, tr("Custom (use with caution)"));
            break;
        default:
            std::throw_with_nested(std::invalid_argument(tr("Error encountered whilst pre-filling QComboBoxes for XMPP server-type to use!").toStdString()));
    }

    return;
}

/**
 * @brief DialogSettings::prefill_xmpp_ignore_ssl_errors
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::prefill_xmpp_ignore_ssl_errors()
{
    ui->comboBox_xmpp_server_ssl_errors->insertItem(GK_XMPP_IGNORE_SSL_ERRORS_COMBO_FALSE, tr("Don't ignore SSL errors"));
    ui->comboBox_xmpp_server_ssl_errors->insertItem(GK_XMPP_IGNORE_SSL_ERRORS_COMBO_TRUE, tr("Ignore all SSL errors"));

    return;
}

/**
 * @brief DialogSettings::prefill_uri_lookup_method
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::prefill_uri_lookup_method()
{
    ui->comboBox_xmpp_server_uri_lookup_method->insertItem(GK_XMPP_URI_LOOKUP_DNS_SRV_METHOD, tr("SRV lookup via DNS"));
    ui->comboBox_xmpp_server_uri_lookup_method->insertItem(GK_XMPP_URI_LOOKUP_MANUAL_METHOD, tr("Manual input (not recommended)"));

    return;
}

/**
 * @brief DialogSettings::prefill_lang_dictionaries prefills the combobox, `ui->comboBox_accessibility_dict()`, with all
 * the language dictionaries required for Hunspell to function properly.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::prefill_lang_dictionaries()
{
    m_sonnetDcb = new Sonnet::DictionaryComboBox(this);
    ui->horizontalLayout_56->removeWidget(ui->comboBox_accessibility_dict);
    ui->horizontalLayout_56->addWidget(m_sonnetDcb);
    m_sonnetDcb->setToolTip(tr("Choose a suitable dictionary/language for spell-checking purposes!"));

    QObject::connect(m_sonnetDcb, &Sonnet::DictionaryComboBox::dictionaryChanged, this, &DialogSettings::spellDictChanged);
    QObject::connect(m_sonnetDcb, &Sonnet::DictionaryComboBox::dictionaryNameChanged, this, &DialogSettings::spellDictNameChanged);

    return;
}

/**
 * @brief DialogSettings::prefill_ui_lang prefills the combobox, `ui->comboBox_accessibility_lang_ui()`, with all the
 * available languages for the User Interface itself of Small World Deluxe.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::prefill_ui_lang()
{
    ui->comboBox_accessibility_lang_ui->addItem(Filesystem::userInterfaceDefLang);
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
 * @brief DialogSettings::launchAtlasDlg
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::launchAtlasDlg()
{
    gkAtlasDlg->setWindowFlags(Qt::Window);
    gkAtlasDlg->show();

    return;
}

/**
 * @brief DialogSettings::readGpsCoords will read the save geographical co-ordinates from the Google LevelDB database,
 * if present.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QGeoCoordinate DialogSettings::readGpsCoords()
{
    try {
        if (ui->lineEdit_rig_gps_coordinates->text().isEmpty()) {
            const auto latitude = gkDekodeDb->read_user_loc_settings(GkUserLocSettings::UserLatitudeCoords);
            const auto longitude = gkDekodeDb->read_user_loc_settings(GkUserLocSettings::UserLongitudeCoords);
            if (!latitude.isEmpty() && !longitude.isEmpty()) {
                QGeoCoordinate coords(latitude.toDouble(), longitude.toDouble());
                return coords;
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::critical(this, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return QGeoCoordinate();
}

/**
 * @brief DialogSettings::calcGpsCoords will convert a given set of geographical co-ordinates from the regularly stored
 * values of Decimal Degrees (DD) towards either, "Degrees, minutes, and seconds (DMS)", or simply the alternative
 * instead which is, "Degrees and decimal minutes (DMM)".
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param geo_coords The latitudinal and longitudinal values to be used, as a QGeoCoordinate() object.
 * @note [ Google ] Find or enter latitude & longitude <https://support.google.com/maps/answer/18539>.
 * @see DialogSettings::readGpsCoords(), DialogSettings::launchAtlasDlg().
 */
void DialogSettings::calcGpsCoords(const QGeoCoordinate &geo_coords)
{
    if (ui->checkBox_rig_gps_dmm->isChecked()) {
        //
        // Using DMM!
        ui->lineEdit_rig_gps_coordinates->setText(geo_coords.toString(QGeoCoordinate::CoordinateFormat::DegreesMinutes));
    } else if (ui->checkBox_rig_gps_dd->isChecked()) {
        //
        // Using DD!
        ui->lineEdit_rig_gps_coordinates->setText(geo_coords.toString(QGeoCoordinate::CoordinateFormat::Degrees));
    } else {
        //
        // Using DMS (also the default option)!
        ui->lineEdit_rig_gps_coordinates->setText(geo_coords.toString(QGeoCoordinate::CoordinateFormat::DegreesMinutesSeconds));
    }

    return;
}

/**
 * @brief DialogSettings::monitorXmppServerChange Monitors for any change given in the widget, `ui->lineEdit_xmpp_server_url`, and
 * alerts the end-user if they truly wish to make that change before committing it to the Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::monitorXmppServerChange()
{
    QString xmpp_host_url = ui->lineEdit_xmpp_server_url->text();
    QString old_val = gkConnDetails.server.url;

    if (!xmpp_host_url.isEmpty() && !old_val.isEmpty()) {
        if (xmpp_host_url == old_val) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Are you sure?"));
            msgBox.setText(tr(R"(Do you truly wish to permanently disconnect from server, "%1", and connect towards, "%2", instead?)")
            .arg(old_val).arg(xmpp_host_url));
            msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Apply);
            msgBox.setDefaultButton(QMessageBox::Apply);
            msgBox.setIcon(QMessageBox::Icon::Question);
            int ret = msgBox.exec();

            switch (ret) {
                case QMessageBox::Apply:
                    gkDekodeDb->write_xmpp_settings(xmpp_host_url, GkXmppCfg::XmppDomainUrl);
                    return;
                case QMessageBox::Cancel:
                    return;
                default:
                    return;
            }
        }
    }

    return;
}

/**
 * @brief DialogSettings::createXmppConnectionFromSettings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::createXmppConnectionFromSettings()
{
    if (!m_xmppClient->isConnected()) {
        if (gkConnDetails.server.type == GkServerType::GekkoFyre) {
            QString tmp_jid = QString("%1@%2").arg(ui->lineEdit_xmpp_client_username->text()).arg(GkXmppGekkoFyreCfg::defaultUrl);
            m_xmppClient->createConnectionToServer(GkXmppGekkoFyreCfg::defaultUrl, ui->spinBox_xmpp_server_port->value(),
                                                   ui->lineEdit_xmpp_client_password->text(), tmp_jid, false);
        } else if (gkConnDetails.server.type == GkServerType::Custom) {
            QString tmp_jid = ui->lineEdit_xmpp_client_username->text();
            QString tmp_reg_domain = gkStringFuncs->getXmppHostname(tmp_jid); // Extract the URI from the given JID!
            m_xmppClient->createConnectionToServer(tmp_reg_domain, ui->spinBox_xmpp_server_port->value(),
                                                   ui->lineEdit_xmpp_client_password->text(), tmp_jid, false);
        } else {
            throw std::invalid_argument(tr("Invalid XMPP server type provided!").toStdString());
        }
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
    gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true, false);
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
void DialogSettings::prefill_avail_com_ports(const std::vector<GkComPort> &com_ports)
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
                if (!port.is_usb) {
                    ++counter;
                    QString portName = port.port_info.portName();

                    //
                    // CAT Control
                    //
                    ui->comboBox_com_port->addItem(portName, portName);

                    //
                    // PTT Method
                    //
                    ui->comboBox_ptt_method_port->addItem(portName, portName);

                    available_com_ports.insert(port.port_info.productIdentifier(), portName);
                }
            }

            //
            // CAT Control
            //
            QString comDeviceCat = gkDekodeDb->read_rig_settings_comms(radio_cfg::ComDeviceCat);
            if (!comDeviceCat.isEmpty() && !available_com_ports.isEmpty()) {
                for (const auto &sel_port: available_com_ports.toStdMap()) {
                    for (const auto &device: gkSerialPortMap) {
                        if ((device.port_info.productIdentifier() == sel_port.first) && (comDeviceCat == device.port_info.portName())) {
                            //
                            // COM/RS232/Serial Port
                            //

                            // NOTE: The recorded setting used to identify the chosen serial device is the COM Port name
                            qint32 serial_idx = ui->comboBox_com_port->findData(sel_port.second);
                            ui->comboBox_com_port->setCurrentIndex(serial_idx);
                            on_comboBox_com_port_currentIndexChanged(serial_idx);
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
                    for (const auto &device: gkSerialPortMap) {
                        if ((device.port_info.productIdentifier() == sel_port.first) && (comDevicePtt == device.port_info.portName())) {
                            // NOTE: The recorded setting used to identify the chosen serial device is the COM Port name
                            qint32 serial_idx = ui->comboBox_ptt_method_port->findData(sel_port.second);
                            ui->comboBox_ptt_method_port->setCurrentIndex(serial_idx);
                            on_comboBox_ptt_method_port_currentIndexChanged(serial_idx);
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
                #ifdef _UNICODE
                QString combined_str = QString("[ #%1 ] %2").arg(QString::fromStdWString(device.port)).arg(device.bos_usb.lib_usb.product);
                available_usb_ports.insert(dev_port, combined_str.toStdWString());
                #else
                available_usb_ports.insert(device.port, device.name);
                #endif

                //
                // CAT Control (via USB)
                //
                ui->comboBox_com_port->addItem(device.name, device.name);
                if (ui->comboBox_com_port->currentData().toString() == device.name) {
                    ui->lineEdit_device_port_name->setText(device.lib_usb.product);
                }

                //
                // PTT Method (via USB)
                //
                ui->comboBox_ptt_method_port->addItem(device.name, device.name);
                if (ui->comboBox_ptt_method_port->currentData().toString() == device.name) {
                    ui->lineEdit_ptt_method_dev_path->setText(device.lib_usb.product);
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
                    for (const auto &device: gkUsbPortMap) {
                        if (!usbDeviceCat.isEmpty()) {
                            //
                            // CAT Control
                            //
                            if ((device.port == sel_port.first) && (usbDeviceCat == device.name)) {
                                // NOTE: The recorded setting used to identify the chosen serial device is the COM Port name
                                qint32 dev_idx = ui->comboBox_com_port->findData(sel_port.second);
                                ui->comboBox_com_port->setCurrentIndex(dev_idx);
                                on_comboBox_com_port_currentIndexChanged(dev_idx);
                            }
                        }

                        if (!usbDevicePtt.isEmpty()) {
                            //
                            // PTT Method
                            //
                            if ((device.port == sel_port.first) && (usbDevicePtt == device.name)) {
                                // NOTE: The recorded setting used to identify the chosen serial device is the COM Port name
                                qint32 serial_idx = ui->comboBox_ptt_method_port->findData(sel_port.second);
                                ui->comboBox_ptt_method_port->setCurrentIndex(serial_idx);
                                on_comboBox_ptt_method_port_currentIndexChanged(serial_idx);
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

        const QString logsDirLoc = gkDekodeDb->read_misc_audio_settings(GkAudioCfg::LogsDirLoc);
        const QString audioRecLoc = gkDekodeDb->read_misc_audio_settings(GkAudioCfg::AudioRecLoc);
        const QString settingsDbLoc = gkDekodeDb->read_misc_audio_settings(GkAudioCfg::settingsDbLoc);

        //
        // General --> User Interface
        //
        const QString ui_lang = gkDekodeDb->read_lang_ui_settings(Language::GkUiLang::ChosenUiLang);
        const QString hunspell_dict = gkDekodeDb->read_lang_dict_settings(Language::GkDictionary::ChosenDictLang);
        const QString msg_audio_notif = gkDekodeDb->read_general_settings(general_stat_cfg::MsgAudioNotif);
        const QString fail_event_notif = gkDekodeDb->read_general_settings(general_stat_cfg::FailAudioNotif);

        const QString eventLogVerbIdx = gkDekodeDb->read_event_log_settings(GkEventLogCfg::GkLogVerbosity);

        const qint32 ui_scale_pctg_val = gkDekodeDb->read_ui_settings(Settings::GkUiCfg::GkUiScalePctg).toInt();
        if (ui_scale_pctg_val >= 50) { // 50% is the most minimum value for user interface scaling!
            ui->horizontalSlider_accessibility_appearance_ui_scale->setValue(ui_scale_pctg_val);
        }

        //
        // General --> XMPP --> Client Settings
        //
        QString xmpp_allow_msg_history = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppAllowMsgHistory);
        QString xmpp_allow_file_xfers = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppAllowFileXfers);
        QString xmpp_allow_mucs = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppAllowMucs);
        QString xmpp_auto_connect = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppAutoConnect);
        QString xmpp_auto_reconnect = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppAutoReconnect);
        QString xmpp_uri_lookup_method = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppUriLookupMethod);
        QString xmpp_network_timeout = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppNetworkTimeout);

        //
        // CAUTION!!! Username, password, and e-mail address!
        //
        QString xmpp_client_username = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppUsername);
        QString xmpp_client_password = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppPassword);
        QString xmpp_client_email_addr = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppEmailAddr);
        QString xmpp_client_nickname = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppNickname);

        if (!xmpp_allow_msg_history.isEmpty()) {
            ui->checkBox_allow_msg_history->setChecked(gkDekodeDb->boolStr(xmpp_allow_msg_history.toStdString()));
        } else {
            ui->checkBox_allow_msg_history->setChecked(true);
        }

        if (!xmpp_allow_file_xfers.isEmpty()) {
            ui->checkBox_allow_file_transfers->setChecked(gkDekodeDb->boolStr(xmpp_allow_file_xfers.toStdString()));
        } else {
            ui->checkBox_allow_file_transfers->setChecked(true);
        }

        if (!xmpp_allow_mucs.isEmpty()) {
            ui->checkBox_allow_muc_creation->setChecked(gkDekodeDb->boolStr(xmpp_allow_mucs.toStdString()));
        } else {
            ui->checkBox_allow_muc_creation->setChecked(true);
        }

        if (!xmpp_auto_connect.isEmpty()) {
            ui->checkBox_connect_automatically->setChecked(gkDekodeDb->boolStr(xmpp_auto_connect.toStdString()));
        } else {
            ui->checkBox_connect_automatically->setChecked(false);
        }

        if (!xmpp_auto_reconnect.isEmpty()) {
            ui->checkBox_automatic_reconnect->setChecked(gkDekodeDb->boolStr(xmpp_auto_reconnect.toStdString()));
        } else {
            ui->checkBox_automatic_reconnect->setChecked(false);
        }

        if (!xmpp_uri_lookup_method.isEmpty()) {
            ui->comboBox_xmpp_server_uri_lookup_method->setCurrentIndex(xmpp_uri_lookup_method.toInt());

            //
            // Update the index for the relevant QComboBox signal/slot!
            on_comboBox_xmpp_server_uri_lookup_method_currentIndexChanged(xmpp_uri_lookup_method.toInt());
        } else {
            ui->comboBox_xmpp_server_uri_lookup_method->setCurrentIndex(GK_XMPP_URI_LOOKUP_DNS_SRV_METHOD);

            //
            // Update the index for the relevant QComboBox signal/slot!
            on_comboBox_xmpp_server_uri_lookup_method_currentIndexChanged(GK_XMPP_URI_LOOKUP_DNS_SRV_METHOD);
        }

        if (!xmpp_client_username.isEmpty()) {
            ui->lineEdit_xmpp_client_username->setText(xmpp_client_username);
        } else {
            ui->lineEdit_xmpp_client_username->setText("");
        }

        if (!xmpp_client_password.isEmpty()) {
            ui->lineEdit_xmpp_client_password->setText(xmpp_client_password);
        } else {
            ui->lineEdit_xmpp_client_password->setText("");
        }

        if (!xmpp_client_email_addr.isEmpty()) {
            ui->lineEdit_xmpp_client_email_address->setText(xmpp_client_email_addr);
        } else {
            ui->lineEdit_xmpp_client_email_address->setText("");
        }

        if (!xmpp_client_nickname.isEmpty()) {
            ui->lineEdit_xmpp_server_nickname->setText(xmpp_client_nickname);
        } else {
            ui->lineEdit_xmpp_server_nickname->setText("");
        }

        if (!xmpp_network_timeout.isEmpty()) {
            ui->spinBox_xmpp_server_network_timeout->setValue(xmpp_network_timeout.toInt());
        } else {
            ui->spinBox_xmpp_server_network_timeout->setValue(15);
        }

        //
        // General --> XMPP --> Server Settings
        //
        QString xmpp_domain_url = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppDomainUrl);
        QString xmpp_server_type = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppServerType);
        QString xmpp_domain_port = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppDomainPort);
        QString xmpp_enable_ssl = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppEnableSsl);
        QString xmpp_ignore_ssl_errors = gkDekodeDb->read_xmpp_settings(GkXmppCfg::XmppIgnoreSslErrors);

        if (!xmpp_server_type.isEmpty()) {
            ui->comboBox_xmpp_server_type->setCurrentIndex(xmpp_server_type.toInt());

            //
            // Update the index for the relevant QComboBox signal/slot!
            on_comboBox_xmpp_server_type_currentIndexChanged(xmpp_server_type.toInt());
        } else {
            ui->comboBox_xmpp_server_type->setCurrentIndex(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX);

            //
            // Update the index for the relevant QComboBox signal/slot!
            on_comboBox_xmpp_server_type_currentIndexChanged(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX);
        }

        switch (ui->comboBox_xmpp_server_type->currentIndex()) {
            case GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX:
                ui->lineEdit_xmpp_server_url->setText(GkXmppGekkoFyreCfg::defaultUrl);
                break;
            case GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX:
                ui->lineEdit_xmpp_server_url->setText(xmpp_domain_url);
                break;
            default:
                break;
        }

        if (!xmpp_domain_port.isEmpty()) {
            ui->spinBox_xmpp_server_port->setValue(xmpp_domain_port.toInt());
        } else {
            ui->spinBox_xmpp_server_port->setValue(GK_DEFAULT_XMPP_SERVER_PORT);
        }

        if (!xmpp_enable_ssl.isEmpty()) {
            ui->checkBox_xmpp_server_ssl->setChecked(gkDekodeDb->boolStr(xmpp_enable_ssl.toStdString()));
        } else {
            ui->checkBox_xmpp_server_ssl->setChecked(true);
        }

        if (!xmpp_ignore_ssl_errors.isEmpty()) {
            ui->comboBox_xmpp_server_ssl_errors->setCurrentIndex(xmpp_ignore_ssl_errors.toInt());
        } else {
            ui->comboBox_xmpp_server_ssl_errors->setCurrentIndex(GK_XMPP_IGNORE_SSL_ERRORS_COMBO_FALSE);
        }

        //
        // Audio --> Configuration
        //
        const QString input_audio_dev = gkDekodeDb->read_audio_device_settings(GkAudioDevice::AudioInputDeviceName);
        const QString output_audio_dev = gkDekodeDb->read_audio_device_settings(GkAudioDevice::AudioOutputDeviceName);

        if (!input_audio_dev.isEmpty()) {
            const qint32 saved_input_idx = ui->comboBox_soundcard_input->findData(input_audio_dev);
            if (saved_input_idx != -1) {
                ui->comboBox_soundcard_input->setCurrentIndex(saved_input_idx);
            }
        } else {
            const ALCchar *defaultInputDeviceName = alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
            const qint32 def_input_idx = ui->comboBox_soundcard_input->findData(QString::fromStdString(defaultInputDeviceName));
            if (def_input_idx != -1) {
                ui->comboBox_soundcard_input->setCurrentIndex(def_input_idx);
            }
        }

        if (!output_audio_dev.isEmpty()) {
            const qint32 saved_output_idx = ui->comboBox_soundcard_input->findData(output_audio_dev);
            if (saved_output_idx != -1) {
                ui->comboBox_soundcard_output->setCurrentIndex(saved_output_idx);
            }
        } else {
            const ALCchar *defaultOutputDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
            const qint32 def_output_idx = ui->comboBox_soundcard_output->findData(QString::fromStdString(defaultOutputDeviceName));
            if (def_output_idx != -1) {
                ui->comboBox_soundcard_output->setCurrentIndex(def_output_idx);
            }
        }

        //
        // Audio --> Recorder
        //
        const qint32 input_audio_dev_chosen_sample_rate_data = gkDekodeDb->read_misc_audio_settings(GkAudioCfg::AudioInputSampleRate).toInt();
        const qint32 input_audio_dev_chosen_number_channels_data = gkDekodeDb->read_misc_audio_settings(GkAudioCfg::AudioInputChannels).toInt();
        const qint32 input_audio_dev_chosen_format_bits_data = gkDekodeDb->read_misc_audio_settings(GkAudioCfg::AudioInputBitrate).toInt();

        if (input_audio_dev_chosen_sample_rate_data != -1) {
            const qint32 input_audio_dev_chosen_sample_rate_idx = ui->comboBox_input_audio_dev_sample_rate->findData(QString::number(input_audio_dev_chosen_sample_rate_data));
            if (input_audio_dev_chosen_sample_rate_idx != -1) {
                ui->comboBox_input_audio_dev_sample_rate->setCurrentIndex(input_audio_dev_chosen_sample_rate_idx);
            }
        }

        if (input_audio_dev_chosen_number_channels_data != -1) {
            const qint32 input_audio_dev_chosen_channels_idx = ui->comboBox_input_audio_dev_number_channels->findData(QString::number(input_audio_dev_chosen_number_channels_data));
            if (input_audio_dev_chosen_channels_idx != -1) {
                ui->comboBox_input_audio_dev_number_channels->setCurrentIndex(input_audio_dev_chosen_channels_idx);
            }
        }

        if (input_audio_dev_chosen_format_bits_data != -1) {
            const qint32 input_audio_dev_chosen_bits_idx = ui->comboBox_input_audio_dev_bitrate->findData(QString::number(input_audio_dev_chosen_format_bits_data));
            if (input_audio_dev_chosen_bits_idx != -1) {
                ui->comboBox_input_audio_dev_bitrate->setCurrentIndex(input_audio_dev_chosen_bits_idx);
            }
        }

        //
        // Miscellaneous...
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
        if (!ui_lang.isEmpty()) {
            qint32 idx = ui->comboBox_accessibility_lang_ui->findData(ui_lang);
            if (idx >= 0) { // -1 means it is not found!
                ui->comboBox_accessibility_lang_ui->setCurrentIndex(idx);
            }
        } else {
            qint32 idx = ui->comboBox_accessibility_lang_ui->findData(Filesystem::userInterfaceDefLang);
            if (idx >= 0) { // -1 means it is not found!
                ui->comboBox_accessibility_lang_ui->setCurrentIndex(idx);
            }
        }

        if (!hunspell_dict.isEmpty()) {
            qint32 idx = ui->comboBox_accessibility_dict->findData(hunspell_dict);
            if (idx >= 0) { // -1 means it is not found!
                ui->comboBox_accessibility_dict->setCurrentIndex(idx);
            }
        } else {
            qint32 idx = ui->comboBox_accessibility_dict->findData(Filesystem::enchantSpellDefLang);
            if (idx >= 0) { // -1 means it is not found!
                ui->comboBox_accessibility_dict->setCurrentIndex(idx);
            }
        }

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
    Q_UNUSED(index);
    try {
        // TODO: This function urgently needs a cleanup...
        if (!gkSerialPortMap.empty()) { // Make sure that we haven't selected the dummy retainer item, "N/A"!
            for (const auto &com_port_list: gkSerialPortMap) {
                for (const auto &usb_port_list: available_usb_ports.toStdMap()) {
                    if (usb_port_list.second == ui->comboBox_com_port->currentData().toString()) {
                        // A USB port has been found!
                        emit changeConnPort(usb_port_list.second, GkConnMethod::CAT);
                    } else if (com_port_list.port_info.portName() == ui->comboBox_com_port->currentData().toString()) {
                        // An RS232/Serial port has been found!
                        QString combined_str = QString("%1 [ PID: #%2 ]")
                                .arg(com_port_list.port_info.productIdentifier())
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
    Q_UNUSED(index);
    try {
        // TODO: This function urgently needs a cleanup...
        if (!gkSerialPortMap.empty()) { // Make sure that we haven't selected the dummy retainer item, "N/A"!
            for (const auto &ptt_port_list: gkSerialPortMap) {
                for (const auto &usb_port_list: available_usb_ports.toStdMap()) {
                    if (usb_port_list.second == ui->comboBox_ptt_method_port->currentData().toString()) {
                        // A USB port has been found!
                        emit changeConnPort(usb_port_list.second, GkConnMethod::PTT);
                    } else if (ptt_port_list.port_info.portName() == ui->comboBox_ptt_method_port->currentData().toString()) {
                        // An RS232/Serial port has been found!
                        QString combined_str = QString("%1 [ PID: #%2 ]")
                                .arg(ptt_port_list.port_info.productIdentifier())
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
        gkDekodeDb->write_misc_audio_settings(ui->lineEdit_db_save_loc->text(), GkAudioCfg::settingsDbLoc);
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
        gkDekodeDb->write_misc_audio_settings(ui->lineEdit_audio_save_loc->text(), GkAudioCfg::AudioRecLoc);
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_soundcard_input_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void DialogSettings::on_comboBox_soundcard_input_currentIndexChanged(int index)
{
    on_comboBox_input_audio_dev_sample_rate_currentIndexChanged(ui->comboBox_input_audio_dev_sample_rate->currentIndex());
    on_comboBox_input_audio_dev_bitrate_currentIndexChanged(ui->comboBox_input_audio_dev_bitrate->currentIndex());
    on_comboBox_input_audio_dev_number_channels_currentIndexChanged(ui->comboBox_input_audio_dev_number_channels->currentIndex());

    return;
}

/**
 * @brief DialogSettings::on_comboBox_input_audio_dev_sample_rate_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void DialogSettings::on_comboBox_input_audio_dev_sample_rate_currentIndexChanged(int index)
{
    for (auto it = gkSysInputDevs.begin(), end = gkSysInputDevs.end(); it != end; ++it) {
        const QString curr_sel_input_dev = ui->comboBox_soundcard_input->itemData(ui->comboBox_soundcard_input->currentIndex()).toString();
        if (!curr_sel_input_dev.isEmpty()) {
            if (it->audio_dev_str == curr_sel_input_dev) {
                switch (index) {
                    case GK_AUDIO_SAMPLE_RATE_8000_IDX:
                        it->pref_sample_rate = 8000;
                        return;
                    case GK_AUDIO_SAMPLE_RATE_11025_IDX:
                        it->pref_sample_rate = 11025;
                        return;
                    case GK_AUDIO_SAMPLE_RATE_22050_IDX:
                        it->pref_sample_rate = 22050;
                        return;
                    case GK_AUDIO_SAMPLE_RATE_32000_IDX:
                        it->pref_sample_rate = 32000;
                        return;
                    case GK_AUDIO_SAMPLE_RATE_44100_IDX:
                        it->pref_sample_rate = 44100;
                        return;
                    case GK_AUDIO_SAMPLE_RATE_48000_IDX:
                        it->pref_sample_rate = 48000;
                        return;
                    case GK_AUDIO_SAMPLE_RATE_88200_IDX:
                        it->pref_sample_rate = 88200;
                        return;
                    case GK_AUDIO_SAMPLE_RATE_96000_IDX:
                        it->pref_sample_rate = 96000;
                        return;
                    default:
                        std::throw_with_nested(std::runtime_error(tr("ERROR: Unable to accurately determine sample rate for input audio device!").toStdString()));
                }

                break;
            }
        }
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_input_audio_dev_bitrate_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @note OpenAL Recording Example <https://github.com/kcat/openal-soft/blob/master/examples/alrecord.c>.
 */
void DialogSettings::on_comboBox_input_audio_dev_bitrate_currentIndexChanged(int index)
{
    if (index == GK_AUDIO_BITRATE_24_IDX) {
        QMessageBox::information(this, tr("Apologies"), tr("We are sincerely sorry, but 24-bit processing is not available in this version of %1!\n\nPlease check the official website for the latest updates.")
        .arg(General::productName), QMessageBox::Ok);

        //
        // Change the QComboBox to a value which *is* supported!
        ui->comboBox_input_audio_dev_bitrate->setCurrentIndex(GK_AUDIO_BITRATE_16_IDX);
        on_comboBox_input_audio_dev_bitrate_currentIndexChanged(GK_AUDIO_BITRATE_16_IDX);
    }

    if (!gkSysInputDevs.empty()) {
        for (auto it = gkSysInputDevs.begin(), end = gkSysInputDevs.end(); it != end; ++it) {
            const QString curr_sel_input_dev = ui->comboBox_soundcard_input->itemData(ui->comboBox_soundcard_input->currentIndex()).toString();
            if (!curr_sel_input_dev.isEmpty()) {
                if (it->audio_dev_str == curr_sel_input_dev) { // Reference currently selected Input Audio device!
                    on_comboBox_input_audio_dev_number_channels_currentIndexChanged(ui->comboBox_input_audio_dev_number_channels->currentIndex());
                    const auto input_audio_dev_format = gkAudioDevices->calcAudioDevFormat(it->sel_channels, index);
                    it->pref_audio_format = input_audio_dev_format;
                    return;
                }
            }
        }
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_input_audio_dev_number_channels_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void DialogSettings::on_comboBox_input_audio_dev_number_channels_currentIndexChanged(int index)
{
    if (!gkSysInputDevs.empty()) {
        for (auto it = gkSysInputDevs.begin(), end = gkSysInputDevs.end(); it != end; ++it) {
            const QString curr_sel_input_dev = ui->comboBox_soundcard_input->itemData(ui->comboBox_soundcard_input->currentIndex()).toString();
            if (!curr_sel_input_dev.isEmpty()) {
                if (it->audio_dev_str == curr_sel_input_dev) { // Reference currently selected Input Audio device!
                    switch (index) {
                        case GK_AUDIO_CHANNELS_MONO:
                        {
                            const GkAudioChannels audio_channel = GkAudioChannels::Mono;
                            it->sel_channels = audio_channel;
                            const auto input_audio_dev_format = gkAudioDevices->calcAudioDevFormat(audio_channel, ui->comboBox_input_audio_dev_bitrate->currentIndex());
                            it->pref_audio_format = input_audio_dev_format;
                        }

                            return;
                        case GK_AUDIO_CHANNELS_STEREO:
                        {
                            const GkAudioChannels audio_channel = GkAudioChannels::Stereo;
                            it->sel_channels = audio_channel;
                            const auto input_audio_dev_format = gkAudioDevices->calcAudioDevFormat(audio_channel, ui->comboBox_input_audio_dev_bitrate->currentIndex());
                            it->pref_audio_format = input_audio_dev_format;
                        }

                            return;
                        default:
                            std::throw_with_nested(std::runtime_error(tr("ERROR: Unable to accurately determine number of audio channels for input audio device!").toStdString()));
                    }

                    break;
                }
            }
        }
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_input_sound_test_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
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
 */
void DialogSettings::on_pushButton_output_sound_test_clicked()
{
    try {
        if (ui->comboBox_soundcard_output->currentIndex() < 0) {
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

        QString curr_sel_output_dev_str;
        if (ret == QMessageBox::Ok) {
            for (const auto &output_dev: gkSysOutputDevs) {
                if (output_dev.audio_dev_str == ui->comboBox_soundcard_output->itemData(ui->comboBox_soundcard_output->currentIndex()).toString()) {
                    curr_sel_output_dev_str = output_dev.audio_dev_str;
                }
            }

            QPointer<GkSinewaveOutput> gkOutputSinewaveTest = new GkSinewaveOutput(curr_sel_output_dev_str, gkAudioDevices, gkEventLogger, this);
            gkOutputSinewaveTest->setPlayLength(GK_AUDIO_SINEWAVE_TEST_PLAYBACK_SECS * 1000); // The playback length is measured in milliseconds!
            gkOutputSinewaveTest->play();

            QMessageBox::information(this, tr("Finished"), tr("The audio test has now finished."), QMessageBox::Ok);
        } else if (ret == QMessageBox::Abort) {
            QMessageBox::information(this, tr("Aborted"), tr("The operation has been terminated."), QMessageBox::Ok);
        } else {
            return;
        }
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

/**
 * @brief DialogSettings::on_pushButton_input_sound_configure_clicked navigates to a new tab window.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_input_sound_configure_clicked()
{
    ui->tabWidget_audio->setCurrentWidget(ui->tab_audio_encoding_settings);
    return;
}

/**
 * @brief DialogSettings::on_pushButton_output_sound_default_clicked chooses the default audio device applicable to the
 * output.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_pushButton_output_sound_default_clicked()
{
    const ALCchar *defaultOutputDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    const qint32 def_output_idx = ui->comboBox_soundcard_output->findData(QString::fromStdString(defaultOutputDeviceName));
    if (def_output_idx != -1) {
        ui->comboBox_soundcard_output->setCurrentIndex(def_output_idx);
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
        gkDekodeDb->write_misc_audio_settings(ui->lineEdit_audio_logs_save_dir->text(), GkAudioCfg::AudioRecLoc);
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

/**
 * @brief DialogSettings::on_toolButton_rig_maidenhead_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_toolButton_rig_maidenhead_clicked()
{
    QMessageBox::information(this, tr("Apologies"), tr("This feature is not implemented yet!"), QMessageBox::Ok);

    return;
}

/**
 * @brief DialogSettings::on_toolButton_rig_gps_coordinates_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::on_toolButton_rig_gps_coordinates_clicked()
{
    launchAtlasDlg();

    return;
}

/**
 * @brief DialogSettings::on_checkBox_rig_gps_dms_stateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void DialogSettings::on_checkBox_rig_gps_dms_stateChanged(int arg1)
{
    //
    // Using DMS!
    if (arg1 == Qt::CheckState::Checked) {
        ui->checkBox_rig_gps_dmm->setChecked(false);
        ui->checkBox_rig_gps_dd->setChecked(false);

        const auto coords = readGpsCoords();
        if (coords.isValid()) {
            ui->lineEdit_rig_gps_coordinates->setText(coords.toString(QGeoCoordinate::CoordinateFormat::DegreesMinutesSeconds));
            return;
        }

        QMessageBox::information(this, tr("Missing data!"), tr("You need to first enter and save a value in plain, decimal degrees (i.e. 'DD'), likeso but without the quotes: \"%1\"")
                .arg(General::Mapping::coordsDecDegPlaceholder), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_checkBox_rig_gps_dmm_stateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void DialogSettings::on_checkBox_rig_gps_dmm_stateChanged(int arg1)
{
    //
    // Using DMM!
    if (arg1 == Qt::CheckState::Checked) {
        ui->checkBox_rig_gps_dms->setChecked(false);
        ui->checkBox_rig_gps_dd->setChecked(false);

        const auto coords = readGpsCoords();
        if (coords.isValid()) {
            ui->lineEdit_rig_gps_coordinates->setText(coords.toString(QGeoCoordinate::CoordinateFormat::DegreesMinutes));
            return;
        }

        QMessageBox::information(this, tr("Missing data!"), tr("You need to first enter and save a value in plain, decimal degrees (i.e. 'DD'), likeso but without the quotes: \"%1\"")
                .arg(General::Mapping::coordsDecDegPlaceholder), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_checkBox_rig_gps_dd_stateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void DialogSettings::on_checkBox_rig_gps_dd_stateChanged(int arg1)
{
    //
    // Using DD!
    if (arg1 == Qt::CheckState::Checked) {
        ui->checkBox_rig_gps_dms->setChecked(false);
        ui->checkBox_rig_gps_dmm->setChecked(false);

        const auto coords = readGpsCoords();
        if (coords.isValid()) {
            ui->lineEdit_rig_gps_coordinates->setText(coords.toString(QGeoCoordinate::CoordinateFormat::Degrees));
            return;
        }

        QMessageBox::information(this, tr("Missing data!"), tr("You need to first enter and save a value in plain, decimal degrees (i.e. 'DD'), likeso but without the quotes: \"%1\"")
                .arg(General::Mapping::coordsDecDegPlaceholder), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::on_lineEdit_rig_gps_coordinates_textEdited receives a specific signal whenever the text within
 * the object, `ui->lineEdit_rig_gps_coordinates()`, is changed. Nothing happens if a change in text is performed
 * programmatically.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1 The text itself which has been changed by the end-user themselves.
 * @see DialogSettings::readGpsCoords(), DialogSettings::calcGpsCoords().
 */
void DialogSettings::on_lineEdit_rig_gps_coordinates_textEdited(const QString &arg1)
{
    ui->checkBox_rig_gps_dmm->setChecked(false);
    ui->checkBox_rig_gps_dms->setChecked(false);
    ui->checkBox_rig_gps_dd->setChecked(true);

    m_gpsCoordEditTimer.start(GK_GPS_COORDS_LINE_EDIT_TIMER);

    return;
}

/**
 * @brief DialogSettings::getGeoFocusPoint will grab the latitudinal value from the cursor's position within the class,
 * `GkAtlasDialog`, for use throughout profile settings!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void DialogSettings::getGeoFocusPoint(const Marble::GeoDataCoordinates &pos)
{
    if (pos.isValid()) {
        m_latitude = pos.latitude();
        m_longitude = pos.longitude();
        emit setGpsCoords(m_latitude, m_longitude);

        m_coords.setLatitude(m_latitude);
        m_coords.setLongitude(m_longitude);
        calcGpsCoords(m_coords);
    }

    return;
}

/**
 * @brief DialogSettings::saveGpsCoords is a function to save QGeoCoordinate() values to the Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param geo_coords The given QGeoCoordinate() to save towards the Google LevelDB database.
 */
void DialogSettings::saveGpsCoords(const QGeoCoordinate &geo_coords)
{
    try {
        if (geo_coords.isValid()) {
            gkDekodeDb->write_user_loc_settings(GkUserLocSettings::UserLatitudeCoords, QString::number(geo_coords.latitude()));
            gkDekodeDb->write_user_loc_settings(GkUserLocSettings::UserLongitudeCoords, QString::number(geo_coords.longitude()));
        }
    } catch (const std::exception &e) {
        QMessageBox::critical(this, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::saveGpsCoords is a function to save geographical values to the Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param latitude The given latitudinal value.
 * @param longitude The given longitudinal value.
 */
void DialogSettings::saveGpsCoords(const qreal &latitude, const qreal &longitude)
{
    try {
        gkDekodeDb->write_user_loc_settings(GkUserLocSettings::UserLatitudeCoords, QString::number(latitude));
        gkDekodeDb->write_user_loc_settings(GkUserLocSettings::UserLongitudeCoords, QString::number(longitude));
    } catch (const std::exception &e) {
        QMessageBox::critical(this, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief DialogSettings::gpsCoordsTimerProc processes the tail-end of m_gpsCoordEditTimer().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @see m_gpsCoordEditTimer(), ui->lineEdit_rig_gps_coordinates().
 */
void DialogSettings::gpsCoordsTimerProc()
{
    const auto dd_coords_str = ui->lineEdit_rig_gps_coordinates->text();
    if (!dd_coords_str.isEmpty()) {
        const auto values = gkStringFuncs->geoCoordsSplit(dd_coords_str);
        if (values.count() == 2) {
            qreal latitude = 0.0;
            qreal longitude = 0.0;

            //
            // Convert to the appropriate variables!
            latitude = values[0].toDouble();
            longitude = values[1].toDouble();

            QGeoCoordinate coords(latitude, longitude);
            if (coords.isValid()) {
                emit setGpsCoords(coords);
            }
        }
    }

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

/**
 * @brief DialogSettings::discSoapySdrDevs enumerates any discovered SDR devices found through the end-user's local machine
 * via SoapySDR and any provided, applicable drivers.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param kwargs Any SDR devices that have been found via SoapySDR.
 * @see MainWindow::findSoapySdrDevs().
 */
void DialogSettings::discSoapySdrDevs()
{
    try {
        //
        // Enumerate any discovered SDR devices (and applicable information)!
        m_sdrDevs.clear();
        const auto devsFound = gkSdrDev->enumSoapySdrDevs();
        if (!devsFound.isEmpty()) {
            m_sdrDevs = devsFound;
        }

        //
        // Setup any SDR table-views!
        gkSettingsDlgSdrDevsTableModel = new GkSettingsDlgSdrDevs(nullptr);
        gkSettingsDlgSdrDevsTableModel->populateData(m_sdrDevs);
        ui->tableView_radio_sdr_device_enum->setModel(gkSettingsDlgSdrDevsTableModel);
        ui->tableView_radio_sdr_device_enum->horizontalHeader()->setVisible(true);
        ui->tableView_radio_sdr_device_enum->horizontalHeader()->setSectionResizeMode(GK_TABLEVIEW_SOAPYSDR_DEVICE_NAME_IDX, QHeaderView::Stretch);
        ui->tableView_radio_sdr_device_enum->show();
    } catch (const std::exception &e) {
        print_exception(e);
    }

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
 * @brief DialogSettings::assertConnType ascertains the connection type used whether it be USB, RS232, Serial, Parallel,
 * or something more exotic.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_ptt Is this a PTT connection type or not?
 * @return The ascertained connection type in question.
 */
GkConnType DialogSettings::assertConnType(const bool &is_ptt)
{
    if (!is_ptt) {
        //
        // CAT
        //
        const QString cat_name = ui->comboBox_com_port->currentData().toString();
        for (const auto &serial: available_com_ports.toStdMap()) {
            if (cat_name == serial.second) {
                return GkConnType::GkRS232;
            }
        }

        for (const auto &usb: available_usb_ports.toStdMap()) {
            if (cat_name == usb.second) {
                return GkConnType::GkUSB;
            }
        }

        return GkConnType::GkNone;
    } else {
        //
        // PTT
        //
        const QString ptt_name = ui->comboBox_ptt_method_port->currentData().toString();
        for (const auto &serial: available_com_ports.toStdMap()) {
            if (ptt_name == serial.second) {
                return GkConnType::GkRS232;
            }
        }

        for (const auto &usb: available_usb_ports.toStdMap()) {
            if (ptt_name == usb.second) {
                return GkConnType::GkUSB;
            }
        }

        return GkConnType::GkNone;
    }

    return GkUSB;
}

void DialogSettings::on_toolButton_xmpp_upload_avatar_browse_file_clicked()
{
    QString filePath = m_xmppClient->obtainAvatarFilePath();
    ui->lineEdit_xmpp_upload_avatar_file_browser->setText(filePath);

    return;
}

void DialogSettings::on_toolButton_xmpp_delete_avatar_from_server_clicked()
{
    return;
}

void DialogSettings::on_toolButton_xmpp_upload_avatar_to_server_clicked()
{
    return;
}

void DialogSettings::on_pushButton_xmpp_cfg_change_password_clicked()
{
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountChangePassword, gkConnDetails, m_xmppClient, gkDekodeDb, gkStringFuncs, gkEventLogger, this);
    gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
    gkXmppRegistrationDlg->setAttribute(Qt::WA_DeleteOnClose, true);
    gkXmppRegistrationDlg->show();
    // this->close();

    return;
}

void DialogSettings::on_pushButton_xmpp_cfg_change_email_clicked()
{
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountChangeEmail, gkConnDetails, m_xmppClient, gkDekodeDb, gkStringFuncs, gkEventLogger, this);
    gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
    gkXmppRegistrationDlg->setAttribute(Qt::WA_DeleteOnClose, true);
    gkXmppRegistrationDlg->show();
    // this->close();

    return;
}

void DialogSettings::on_pushButton_xmpp_cfg_signup_clicked()
{
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountCreate, gkConnDetails, m_xmppClient, gkDekodeDb, gkStringFuncs, gkEventLogger, this);
    if (ui->lineEdit_xmpp_client_username->text().isEmpty() || ui->lineEdit_xmpp_client_password->text().isEmpty()) {
        //
        // Open the registration form so that the user knows what information to provide!
        gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
        gkXmppRegistrationDlg->setAttribute(Qt::WA_DeleteOnClose, true);
        gkXmppRegistrationDlg->show();
    } else {
        //
        // Register with the information already provided within the setting's dialog!
        QString tmp_jid;
        switch (gkConnDetails.server.type) {
            case GkServerType::GekkoFyre:
                tmp_jid = QString("%1@%2").arg(ui->lineEdit_xmpp_client_username->text()).arg(GkXmppGekkoFyreCfg::defaultUrl);
                break;
            case GkServerType::Custom :
                tmp_jid = ui->lineEdit_xmpp_client_username->text();
                break;
            default:
                break;
        }

        //
        // Prefill the user signup form with most of the details already provided by the user from within the Settings Dialog!
        gkXmppRegistrationDlg->prefillFormFields(tmp_jid, ui->lineEdit_xmpp_client_password->text(), ui->lineEdit_xmpp_client_email_address->text(),
                                                 ui->spinBox_xmpp_server_port->value());

        //
        // Open the registration form so that the user knows what information to provide!
        gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
        gkXmppRegistrationDlg->setAttribute(Qt::WA_DeleteOnClose, true);
        gkXmppRegistrationDlg->show();
    }

    return;
}

void DialogSettings::on_pushButton_xmpp_cfg_login_logout_clicked()
{
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountLogin, gkConnDetails, m_xmppClient, gkDekodeDb, gkStringFuncs, gkEventLogger, this);
    gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
    gkXmppRegistrationDlg->setAttribute(Qt::WA_DeleteOnClose, true);
    gkXmppRegistrationDlg->show();
    // this->close();

    return;
}

void DialogSettings::on_pushButton_xmpp_cfg_delete_account_clicked()
{
    if (!ui->lineEdit_xmpp_client_username->text().isEmpty() && !ui->lineEdit_xmpp_client_password->text().isEmpty()) {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Are you certain about deleting the XMPP account for user, \"%1\"? This action is irreversible!")
                        .arg(ui->lineEdit_xmpp_client_username->text()));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Warning);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ok:
                createXmppConnectionFromSettings();
                QObject::connect(m_xmppClient, &QXmppClient::connected, this, [=]() {
                    if (m_xmppClient->deleteUserAccount()) {
                        QMessageBox::information(nullptr, tr("Success!"), tr("User account deleted successfully from the server!"), QMessageBox::Ok);
                        return;
                    }
                });

                return;
            case QMessageBox::Cancel:
                return;
            default:
                return;
        }
    }

    return;
}

void DialogSettings::on_pushButton_xmpp_cfg_delete_msg_history_clicked()
{
    // TODO: Implement this function!
    QMessageBox::information(nullptr, tr("Not implemented!"), tr("This operation has yet to be implemented."), QMessageBox::Ok);

    return;
}

void DialogSettings::on_comboBox_xmpp_server_type_currentIndexChanged(int index)
{
    switch (index) {
        case GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX:
            // GekkoFyre Networks' servers have been selected!
            ui->lineEdit_xmpp_server_url->setEnabled(false);
            ui->spinBox_xmpp_server_port->setEnabled(false);
            ui->checkBox_xmpp_server_ssl->setEnabled(false);
            ui->comboBox_xmpp_server_ssl_errors->setEnabled(false);
            ui->lineEdit_xmpp_client_username->setPlaceholderText(tr("<username>"));

            break;
        case GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX:
            // A custom server selection has been made!
            ui->lineEdit_xmpp_server_url->setEnabled(true);
            ui->spinBox_xmpp_server_port->setEnabled(true);
            ui->checkBox_xmpp_server_ssl->setEnabled(true);
            ui->comboBox_xmpp_server_ssl_errors->setEnabled(true);
            ui->lineEdit_xmpp_client_username->setPlaceholderText(tr("<username>@<host>.<tld>"));

            break;
        default:
            break;
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_xmpp_server_ssl_errors_currentIndexChanged Present a dire warning to the end-user if
 * they so choose to ignore any and all SSL warnings, and what possible consequences their actions could have!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index The chosen item's index within the targeted QComboBox.
 */
void DialogSettings::on_comboBox_xmpp_server_ssl_errors_currentIndexChanged(int index)
{
    if (index == GK_XMPP_IGNORE_SSL_ERRORS_COMBO_FALSE) {
        return;
    } else {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Please indicate whether you are absolutely sure about disabling all warnings and "
                          "safety-checks about ignoring any future SSL warnings. This could have dire consequences "
                          "for your security online!"));
        msgBox.setStandardButtons(QMessageBox::Ignore | QMessageBox::Abort | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Abort);
        msgBox.setIcon(QMessageBox::Icon::Warning);
        int ret = msgBox.exec();

        switch (ret) {
            case QMessageBox::Ignore:
                ui->comboBox_xmpp_server_ssl_errors->setCurrentIndex(GK_XMPP_IGNORE_SSL_ERRORS_COMBO_TRUE);
                break;
            case QMessageBox::Abort:
                ui->comboBox_xmpp_server_ssl_errors->setCurrentIndex(GK_XMPP_IGNORE_SSL_ERRORS_COMBO_FALSE);
                break;
            case QMessageBox::Cancel:
                ui->comboBox_xmpp_server_ssl_errors->setCurrentIndex(GK_XMPP_IGNORE_SSL_ERRORS_COMBO_FALSE);
                break;
            default:
                ui->comboBox_xmpp_server_ssl_errors->setCurrentIndex(GK_XMPP_IGNORE_SSL_ERRORS_COMBO_FALSE);
                break;
        }
    }

    return;
}

/**
 * @brief DialogSettings::on_comboBox_xmpp_server_uri_lookup_method_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void DialogSettings::on_comboBox_xmpp_server_uri_lookup_method_currentIndexChanged(int index)
{
    return;
}

/**
 * @brief DialogSettings::on_checkBox_connect_automatically_toggled Presents a warning to the user if they check this box and
 * yet haven't configured a username, password, email, etc. yet.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param checked Whether the tickbox is checked yet or not.
 */
void DialogSettings::on_checkBox_connect_automatically_toggled(bool checked)
{
    if (checked) {
        if ((ui->lineEdit_xmpp_client_username->text().isEmpty() || ui->lineEdit_xmpp_client_password->text().isEmpty()) ||
            ui->lineEdit_xmpp_client_email_address->text().isEmpty()) {
            // Present warning to the end-user!
            QMessageBox::information(this, tr("Have you..."), tr("You have yet to configure a username, desired password, e-mail "
                                                                 "address, etc. all of which we recommend before checking this tickbox!"), QMessageBox::Ok);
        }
    } else {
        // No warning is needed at this time...
        return;
    }

    return;
}

/**
 * @brief DialogSettings::on_lineEdit_xmpp_server_url_textChanged Updates the username input in real time so that it shows
 * to the end-user how their JID appears as it does on the backend.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1 The QString as it appears within, `ui->lineEdit_xmpp_server_url()`.
 */
void DialogSettings::on_lineEdit_xmpp_server_url_textChanged(const QString &arg1)
{
    if (arg1.isEmpty()) {
        ui->lineEdit_xmpp_client_username->setPlaceholderText(QString("@%1").arg(arg1));
    }

   return;
}

/**
 * @brief DialogSettings::on_comboBox_accessibility_lang_ui_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void DialogSettings::on_comboBox_accessibility_lang_ui_currentIndexChanged(int index)
{
    return;
}

/**
 * @brief DialogSettings::on_comboBox_accessibility_dict_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void DialogSettings::on_comboBox_accessibility_dict_currentIndexChanged(const QString &arg1)
{
    return;
}

/**
 * @brief DialogSettings::on_horizontalSlider_accessibility_appearance_ui_scale_valueChanged manages the UI scale factor
 * and the change within it accordingly.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value How much to scale the UI by.
 */
void DialogSettings::on_horizontalSlider_accessibility_appearance_ui_scale_valueChanged(int value)
{
    //
    // Mention the changing percentage as a floating tooltip!
    ui->horizontalSlider_accessibility_appearance_ui_scale->setToolTip(QString("%1%").arg(QString::number(value)));

    return;
}

/**
 * @brief DialogSettings::on_checkBox_accessibility_appearance_enbl_custom_font_toggled
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param checked
 */
void DialogSettings::on_checkBox_accessibility_appearance_enbl_custom_font_toggled(bool checked)
{
    return;
}

/**
 * @brief DialogSettings::on_fontComboBox_accessibility_appearance_custom_font_currentFontChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param f
 */
void DialogSettings::on_fontComboBox_accessibility_appearance_custom_font_currentFontChanged(const QFont &f)
{
    return;
}

/**
 * @brief DialogSettings::spellDictDump
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void DialogSettings::spellDictDump()
{
    gkEventLogger->publishEvent(tr("Current spelling dictionary in use: %1").arg(m_sonnetDcb->currentDictionary()), GkSeverity::Debug,
                                "", true, true, false, false, false);
    gkEventLogger->publishEvent(tr("Human-readable name of spelling dictionary in use: %1").arg(m_sonnetDcb->currentDictionaryName()), GkSeverity::Debug,
                                "", true, true, false, false, false);

    return;
}

/**
 * @brief DialogSettings::spellDictChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param name
 */
void DialogSettings::spellDictChanged(const QString &name)
{
    gkEventLogger->publishEvent(tr("Current spelling dictionary changed: %1").arg(name), GkSeverity::Info,
                                "", true, true, false, false, false);

    return;
}

/**
 * @brief DialogSettings::spellDictNameChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param name
 */
void DialogSettings::spellDictNameChanged(const QString &name)
{
    gkEventLogger->publishEvent(tr("Human-readable name of the spelling dictionary which was changed: %1").arg(name), GkSeverity::Info,
                                "", true, true, false, false, false);

    return;
}
