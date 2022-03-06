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

#pragma once

#include "src/defines.hpp"
#include "src/radiolibs.hpp"
#include "src/dek_db.hpp"
#include "src/audio_devices.hpp"
#include "src/gk_xmpp_client.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/ui/gkatlasdialog.hpp"
#include "src/models/tableview/gk_frequency_model.hpp"
#include <marble/MarbleWidget.h>
#include <marble/GeoDataCoordinates.h>
#include <boost/logic/tribool.hpp>
#include <list>
#include <mutex>
#include <tuple>
#include <thread>
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
#include <exception>
#include <type_traits>
#include <QMap>
#include <QDialog>
#include <QString>
#include <QVector>
#include <QPointer>
#include <QMultiMap>
#include <QComboBox>
#include <QGeoCoordinate>
#include <QSharedPointer>

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
#include <KF5/SonnetUI/Sonnet/DictionaryComboBox>
#else
#include <Sonnet/DictionaryComboBox>
#endif

namespace Ui {
class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(QPointer<GekkoFyre::GkLevelDb> dkDb,
                            QPointer<GekkoFyre::FileIo> filePtr,
                            QPointer<GekkoFyre::GkAudioDevices> audioDevices,
                            const std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> sysInputDevs,
                            const std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> sysOutputDevs,
                            QPointer<GekkoFyre::RadioLibs> radioLibs,
                            QPointer<GekkoFyre::StringFuncs> stringFuncs,
                            std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> radioPtr,
                            const std::vector<GekkoFyre::Database::Settings::GkComPort> &com_ports,
                            const QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> &usbPortMap,
                            QPointer<GekkoFyre::GkFrequencies> gkFreqList,
                            QPointer<GekkoFyre::GkFreqTableModel> freqTableModel,
                            const GekkoFyre::Network::GkXmpp::GkUserConn &connection_details,
                            QPointer<GekkoFyre::GkXmppClient> xmppClient,
                            QPointer<GekkoFyre::GkEventLogger> eventLogger,
                            QPointer<Marble::MarbleWidget> mapWidget,
                            const GekkoFyre::System::UserInterface::GkSettingsDlgTab &settingsDlgTab = GekkoFyre::System::UserInterface::GkSettingsDlgTab::GkGeneralStation,
                            QWidget *parent = nullptr);
    ~DialogSettings() override;

private slots:
    void on_pushButton_submit_config_clicked();
    void on_pushButton_cancel_config_clicked();
    void on_pushButton_db_save_loc_clicked();
    void on_pushButton_audio_save_loc_clicked();
    void on_pushButton_refresh_audio_devices_clicked();
    void on_pushButton_audio_logs_save_dir_clicked();
    void on_comboBox_brand_selection_currentIndexChanged(const QString &arg1);
    void on_comboBox_com_port_currentIndexChanged(int index = -1);
    void on_comboBox_ptt_method_port_currentIndexChanged(int index = -1);
    void on_spinBox_spectro_render_thread_settings_valueChanged(int arg1);
    void on_horizontalSlider_encoding_audio_quality_valueChanged(int value);

    void disableUsbPorts(const bool &disable);
    void disableComPorts(const bool &disable);

    //
    // Rig selection
    void on_comboBox_rig_selection_currentIndexChanged(int index = -1);

    //
    // Data Bits
    void on_radioButton_data_bits_default_clicked();
    void on_radioButton_data_bits_seven_clicked();
    void on_radioButton_data_bits_eight_clicked();

    //
    // Stop Bits
    void on_radioButton_stop_bits_default_clicked();
    void on_radioButton_stop_bits_one_clicked();
    void on_radioButton_stop_bits_two_clicked();

    //
    // Handshake
    void on_radioButton_handshake_default_clicked();
    void on_radioButton_handshake_none_clicked();
    void on_radioButton_handshake_xon_xoff_clicked();
    void on_radioButton_handshake_hardware_clicked();

    //
    // PTT Method
    void on_radioButton_ptt_method_vox_clicked();
    void on_radioButton_ptt_method_dtr_clicked();
    void on_radioButton_ptt_method_cat_clicked();
    void on_radioButton_ptt_method_rts_clicked();

    //
    // Transmit Audio Source
    void on_radioButton_tx_audio_src_rear_data_clicked();
    void on_radioButton_tx_audio_src_front_mic_clicked();

    //
    // Mode
    void on_radioButton_mode_none_clicked();
    void on_radioButton_mode_usb_clicked();
    void on_radioButton_mode_data_pkt_clicked();

    //
    // Split Operation
    void on_radioButton_split_none_clicked();
    void on_radioButton_split_rig_clicked();
    void on_radioButton_split_fake_it_clicked();

    //
    // Setting's Dialog signals
    void on_DialogSettings_rejected();

    //
    // Spectrograph & Waterfall
    void on_spinBox_spectro_min_freq_valueChanged(int arg1);
    void on_spinBox_spectro_max_freq_valueChanged(int arg1);
    void on_horizontalSlider_spectro_min_freq_sliderMoved(int position);
    void on_horizontalSlider_spectro_max_freq_sliderMoved(int position);
    void on_horizontalSlider_spectro_min_freq_valueChanged(int value);
    void on_horizontalSlider_spectro_max_freq_valueChanged(int value);

    //
    // Frequency List
    void on_pushButton_freq_list_new_clicked();
    void on_pushButton_freq_list_edit_clicked();
    void on_pushButton_freq_list_delete_clicked();
    void on_pushButton_freq_list_print_clicked();
    void on_doubleSpinBox_freq_calib_intercept_valueChanged(double arg1);
    void on_doubleSpinBox_freq_calib_slope_valueChanged(double arg1);

    //
    // General Settings
    void on_checkBox_new_msg_audio_notification_stateChanged(int arg1);
    void on_checkBox_failed_event_audio_notification_stateChanged(int arg1);

    //
    // Mapping, location, maidenhead, etc.
    void on_toolButton_rig_maidenhead_clicked();
    void on_toolButton_rig_gps_coordinates_clicked();
    void on_checkBox_rig_gps_dms_stateChanged(int arg1);
    void on_checkBox_rig_gps_dmm_stateChanged(int arg1);
    void on_checkBox_rig_gps_dd_stateChanged(int arg1);
    void on_lineEdit_rig_gps_coordinates_textEdited(const QString &arg1);
    void getGeoFocusPoint(const Marble::GeoDataCoordinates &pos);
    void saveGpsCoords(const QGeoCoordinate &geo_coords);
    void saveGpsCoords(const qreal &latitude, const qreal &longitude);
    void gpsCoordsTimerProc();

    //
    // Text-to-speech Settings
    void on_pushButton_access_stt_speak_clicked();
    void on_pushButton_access_stt_pause_clicked();
    void on_pushButton_access_stt_enable_clicked();
    void on_pushButton_access_stt_resume_clicked();
    void on_pushButton_access_stt_stop_clicked();
    void on_horizontalSlider_access_stt_volume_valueChanged(int value);
    void on_horizontalSlider_access_stt_rate_valueChanged(int value);
    void on_horizontalSlider_access_stt_pitch_valueChanged(int value);
    void on_comboBox_access_stt_engine_currentIndexChanged(int index);
    void on_comboBox_access_stt_language_currentIndexChanged(int index);
    void on_comboBox_access_stt_preset_voice_currentIndexChanged(int index);

    void ttsLocaleChanged(const QLocale &locale);
    void ttsAddLanguageItem(const QString &name, const QVariant &locale);
    void ttsAddPresetVoiceItem(const QString &name, const QVariant &locale);

    //
    // Audio System & Multimedia related
    void on_comboBox_soundcard_input_currentIndexChanged(int index);
    void on_comboBox_input_audio_dev_sample_rate_currentIndexChanged(int index);
    void on_comboBox_input_audio_dev_bitrate_currentIndexChanged(int index);
    void on_comboBox_input_audio_dev_number_channels_currentIndexChanged(int index);
    void on_pushButton_input_sound_test_clicked();
    void on_pushButton_output_sound_test_clicked();
    void on_pushButton_input_sound_configure_clicked();
    void on_pushButton_output_sound_default_clicked();

    //
    // XMPP Settings
    void on_toolButton_xmpp_upload_avatar_browse_file_clicked();
    void on_toolButton_xmpp_delete_avatar_from_server_clicked();
    void on_toolButton_xmpp_upload_avatar_to_server_clicked();
    void on_pushButton_xmpp_cfg_change_password_clicked();
    void on_pushButton_xmpp_cfg_change_email_clicked();
    void on_pushButton_xmpp_cfg_signup_clicked();
    void on_pushButton_xmpp_cfg_login_logout_clicked();
    void on_pushButton_xmpp_cfg_delete_msg_history_clicked();
    void on_pushButton_xmpp_cfg_delete_account_clicked();
    void on_comboBox_xmpp_server_type_currentIndexChanged(int index);
    void on_comboBox_xmpp_server_ssl_errors_currentIndexChanged(int index);
    void on_comboBox_xmpp_server_uri_lookup_method_currentIndexChanged(int index);
    void on_checkBox_connect_automatically_toggled(bool checked);
    void on_lineEdit_xmpp_server_url_textChanged(const QString &arg1);

    //
    // Accessibility -- Language & Dictionaries
    void on_comboBox_accessibility_lang_ui_currentIndexChanged(int index);
    void on_comboBox_accessibility_dict_currentIndexChanged(const QString &arg1);

    //
    // Accessibility -- UI & Appearance
    void on_horizontalSlider_accessibility_appearance_ui_scale_valueChanged(int value);
    void on_checkBox_accessibility_appearance_enbl_custom_font_toggled(bool checked);
    void on_fontComboBox_accessibility_appearance_custom_font_currentFontChanged(const QFont &f);

    //
    // Spell-checking, dictionaries, etc.
    //
    void spellDictDump();
    void spellDictChanged(const QString &name);
    void spellDictNameChanged(const QString &name);

signals:
    void changeSelectedTTSEngine(const QString &name);

    //
    // Hamlib and transceiver related
    void changeConnPort(const QString &conn_port, const GekkoFyre::AmateurRadio::GkConnMethod &conn_method);
    void usbPortsDisabled(const bool &active);
    void comPortsDisabled(const bool &active);
    void recvRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void addRigInUse(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);

    //
    // Audio System and related
    void changeInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &input_device);
    void changeOutputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &output_device);

    //
    // Mapping, location, maidenhead, etc.
    void setGpsCoords(const QGeoCoordinate &geo_coords);
    void setGpsCoords(const qreal &latitude, const qreal &longitude);

    //
    // XMPP and related
    void updateXmppConfig();

private:
    Ui::DialogSettings *ui;

    //
    // Converts an object, such as an `enum`, to the underlying type (i.e. an `integer` in the given example)
    template <typename E>
    constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
        return static_cast<typename std::underlying_type<E>::type>(e);
    }

    QPointer<GekkoFyre::RadioLibs> gkRadioLibs;
    QPointer<GekkoFyre::GkLevelDb> gkDekodeDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    // QPointer<GekkoFyre::GkTextToSpeech> gkTextToSpeech;
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> gkRadioPtr;

    //
    // QXmpp and XMPP related
    QPointer<GekkoFyre::GkXmppClient> m_xmppClient;
    GekkoFyre::Network::GkXmpp::GkUserConn gkConnDetails;

    //
    // Multithreading and mutexes
    std::mutex xmppUpdateCalendarMtx;
    std::thread xmppUpdateCalendarThread;

    static QComboBox *rig_comboBox;
    static QComboBox *mfg_comboBox;
    static QMultiMap<rig_model_t, std::tuple<QString, QString, GekkoFyre::AmateurRadio::rig_type>> radio_model_names; // Values: MFG, Model, Rig Type.
    static QVector<QString> unique_mfgs;

    double audio_quality_val;

    // This variable is responsible for managing the COM/RS232/Serial ports of the system!
    std::vector<GekkoFyre::Database::Settings::GkComPort> gkSerialPortMap;

    // The key is the USB devices' port number and the value is the associated struct
    QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> gkUsbPortMap;

    bool com_ports_active;
    bool usb_ports_active;

    // The key is the Product Identifier for the RS232 device in question, while the value is what's displayed in the QComboBox...
    QMap<quint16, QString> available_com_ports; // For tracking the *available* RS232, etc. device ports that the user can choose from...

    // The key is the Port Number for the USB device in question, while the value is what's displayed in the QComboBox...
    QMap<quint16, QString> available_usb_ports; // For tracking the *available* USB device ports that the user can choose from...

    //
    // Audio System and related
    // The key corresponds to the position within the QComboBoxes
    //
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> gkSysInputDevs;
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> gkSysOutputDevs;

    //
    // QXmpp and XMPP related
    void xmppUpdateCalendarDateTime(const QDateTime &startTime = QDateTime::currentDateTime().toLocalTime());

    static int prefill_rig_selection(const rig_caps *caps, void *data);
    static QMultiMap<rig_model_t, std::tuple<QString, QString, GekkoFyre::AmateurRadio::rig_type>> init_model_names();

    QPointer<GekkoFyre::GkFrequencies> gkFreqs;
    QPointer<GekkoFyre::GkFreqTableModel> gkFreqTableModel;

    GekkoFyre::System::UserInterface::GkSettingsDlgTab gkSettingsDlgTab;

    //
    // Spell-checking, dictionaries, etc.
    //
    QPointer<Sonnet::DictionaryComboBox> m_sonnetDcb;

    //
    // Mapping and atlas APIs, etc.
    //
    QPointer<Marble::MarbleWidget> m_mapWidget;
    QPointer<GkAtlasDialog> gkAtlasDlg;
    QGeoCoordinate m_coords;
    qreal m_latitude;
    qreal m_longitude;

    //
    // Date and timing, calendars, etc.
    //
    QTimer m_gpsCoordEditTimer;

    void prefill_audio_devices();
    void prefill_audio_encode_comboboxes();
    void prefill_event_logger();
    void prefill_xmpp_server_type(const GekkoFyre::Network::GkXmpp::GkServerType &server_type);
    void prefill_xmpp_ignore_ssl_errors();
    void prefill_uri_lookup_method();
    void prefill_lang_dictionaries();
    void prefill_ui_lang();
    void init_station_info();

    //
    // Mapping, location, maidenhead, etc.
    void launchAtlasDlg();
    QGeoCoordinate readGpsCoords();
    void calcGpsCoords(const QGeoCoordinate &geo_coords);

    void monitorXmppServerChange();
    void createXmppConnectionFromSettings();

    void print_exception(const std::exception &e, int level = 0);
    double convQComboBoxSampleRateToDouble(const int &combobox_idx);

    QMap<int, int> collectComboBoxIndexes(const QComboBox *combo_box);
    void prefill_rig_force_ctrl_lines(const ptt_type_t &ptt_type);
    void prefill_avail_com_ports(const std::vector<GekkoFyre::Database::Settings::GkComPort> &com_ports);
    void prefill_avail_usb_ports(const QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> &usb_devices);
    void prefill_com_baud_speed(const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate);
    void enable_device_port_options();
    GekkoFyre::AmateurRadio::GkConnType assertConnType(const bool &is_ptt = false);

    bool read_settings();
};
