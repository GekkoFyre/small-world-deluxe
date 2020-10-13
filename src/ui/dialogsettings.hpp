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

#pragma once

#include "src/defines.hpp"
#include "src/radiolibs.hpp"
#include "src/dek_db.hpp"
#include "src/audio_devices.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_text_to_speech.hpp"
#include "src/models/tableview/gk_frequency_model.hpp"
#include <boost/logic/tribool.hpp>
#include <list>
#include <tuple>
#include <memory>
#include <vector>
#include <string>
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
#include <QSharedPointer>

namespace Ui {
class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(QPointer<GekkoFyre::GkLevelDb> dkDb,
                            QPointer<GekkoFyre::FileIo> filePtr,
                            std::shared_ptr<GekkoFyre::AudioDevices> audioDevices,
                            QPointer<GekkoFyre::RadioLibs> radioLibs,
                            QPointer<GekkoFyre::StringFuncs> stringFuncs,
                            portaudio::System *portAudioInit,
                            std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> radioPtr,
                            const std::list<GekkoFyre::Database::Settings::GkComPort> &com_ports,
                            const QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> &usbPortMap,
                            QPointer<GekkoFyre::GkFrequencies> gkFreqList,
                            QPointer<GekkoFyre::GkFreqTableModel> freqTableModel,
                            QPointer<GekkoFyre::GkEventLogger> eventLogger,
                            QPointer<GekkoFyre::GkTextToSpeech> textToSpeechPtr,
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
    void on_comboBox_com_port_currentIndexChanged(int index);
    void on_comboBox_ptt_method_port_currentIndexChanged(int index);
    void on_spinBox_spectro_render_thread_settings_valueChanged(int arg1);
    void on_horizontalSlider_encoding_audio_quality_valueChanged(int value);

    void disableUsbPorts(const bool &disable);
    void disableComPorts(const bool &disable);

    //
    // Rig selection
    //
    void on_comboBox_rig_selection_currentIndexChanged(int index = -1);

    //
    // Data Bits
    //
    void on_radioButton_data_bits_default_clicked();
    void on_radioButton_data_bits_seven_clicked();
    void on_radioButton_data_bits_eight_clicked();

    //
    // Stop Bits
    //
    void on_radioButton_stop_bits_default_clicked();
    void on_radioButton_stop_bits_one_clicked();
    void on_radioButton_stop_bits_two_clicked();

    //
    // Handshake
    //
    void on_radioButton_handshake_default_clicked();
    void on_radioButton_handshake_none_clicked();
    void on_radioButton_handshake_xon_xoff_clicked();
    void on_radioButton_handshake_hardware_clicked();

    //
    // PTT Method
    //
    void on_radioButton_ptt_method_vox_clicked();
    void on_radioButton_ptt_method_dtr_clicked();
    void on_radioButton_ptt_method_cat_clicked();
    void on_radioButton_ptt_method_rts_clicked();

    //
    // Transmit Audio Source
    //
    void on_radioButton_tx_audio_src_rear_data_clicked();
    void on_radioButton_tx_audio_src_front_mic_clicked();

    //
    // Mode
    //
    void on_radioButton_mode_none_clicked();
    void on_radioButton_mode_usb_clicked();
    void on_radioButton_mode_data_pkt_clicked();

    //
    // Split Operation
    //
    void on_radioButton_split_none_clicked();
    void on_radioButton_split_rig_clicked();
    void on_radioButton_split_fake_it_clicked();

    //
    // Setting's Dialog signals
    //
    void on_DialogSettings_rejected();

    //
    // Spectrograph & Waterfall
    //
    void on_spinBox_spectro_min_freq_valueChanged(int arg1);
    void on_spinBox_spectro_max_freq_valueChanged(int arg1);
    void on_horizontalSlider_spectro_min_freq_sliderMoved(int position);
    void on_horizontalSlider_spectro_max_freq_sliderMoved(int position);
    void on_horizontalSlider_spectro_min_freq_valueChanged(int value);
    void on_horizontalSlider_spectro_max_freq_valueChanged(int value);

    //
    // Frequency List
    //
    void on_pushButton_freq_list_new_clicked();
    void on_pushButton_freq_list_edit_clicked();
    void on_pushButton_freq_list_delete_clicked();
    void on_pushButton_freq_list_print_clicked();
    void on_doubleSpinBox_freq_calib_intercept_valueChanged(double arg1);
    void on_doubleSpinBox_freq_calib_slope_valueChanged(double arg1);

    //
    // General Settings
    //
    void on_checkBox_new_msg_audio_notification_stateChanged(int arg1);
    void on_checkBox_failed_event_audio_notification_stateChanged(int arg1);

    //
    // Text-to-speech Settings
    //
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
    // PortAudio
    //
    void on_comboBox_soundcard_input_currentIndexChanged(int index = -1);
    void on_comboBox_soundcard_output_currentIndexChanged(int index = -1);
    void on_comboBox_soundcard_api_currentIndexChanged(int index = -1);
    void on_pushButton_input_sound_test_clicked();
    void on_pushButton_output_sound_test_clicked();
    void on_comboBox_audio_input_sample_rate_currentIndexChanged(int index);
    void on_comboBox_audio_output_sample_rate_currentIndexChanged(int index);

signals:
    void changeSelectedTTSEngine(const QString &name);

    //
    // Hamlib and transceiver related
    //
    void changeConnPort(const QString &conn_port, const GekkoFyre::AmateurRadio::GkConnMethod &conn_method);
    void usbPortsDisabled(const bool &active);
    void comPortsDisabled(const bool &active);
    void recvRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void addRigInUse(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);

    //
    // PortAudio and related
    //
    void changeInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &input_device);
    void changeOutputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &output_device);

private:
    Ui::DialogSettings *ui;

    //
    // Converts an object, such as an `enum`, to the underlying type (i.e. an `integer` in the given example)
    //
    template <typename E>
    constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
        return static_cast<typename std::underlying_type<E>::type>(e);
    }

    portaudio::System *gkPortAudioInit;
    std::vector<double> standardSampleRates;
    QMap<qint32, double> supportedInputSampleRates; // The supported sample rates for the chosen input audio device! The key corresponds to the position within the QComboBoxes...
    QMap<qint32, double> supportedOutputSampleRates; // The supported sample rates for the chosen output audio device! The key corresponds to the position within the QComboBoxes...

    QPointer<GekkoFyre::RadioLibs> gkRadioLibs;
    QPointer<GekkoFyre::GkLevelDb> gkDekodeDb;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPointer<GekkoFyre::GkTextToSpeech> gkTextToSpeech;
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDevices;
    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> gkRadioPtr;

    static QComboBox *rig_comboBox;
    static QComboBox *mfg_comboBox;
    static QMultiMap<rig_model_t, std::tuple<QString, QString, GekkoFyre::AmateurRadio::rig_type>> radio_model_names; // Values: MFG, Model, Rig Type.
    static QVector<QString> unique_mfgs;

    double audio_quality_val;

    // This variable is responsible for managing the COM/RS232/Serial ports of the system!
    std::list<GekkoFyre::Database::Settings::GkComPort> status_com_ports;

    // The key is the USB devices' port number and the value is the associated struct
    QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> status_usb_devices;

    bool com_ports_active;
    bool usb_ports_active;

    // The key is the Hardware ID for the COM/RS232 port in question, while the value is the currentIndex within the QComboBox...
    QMap<QString, int> available_com_ports; // For tracking the *available* RS232, etc. device ports that the user can choose from...

    // The key is the Port Number for the USB device in question, while the value is what's displayed in the QComboBox...
    QMap<quint16, QString> available_usb_ports; // For tracking the *available* USB device ports that the user can choose from...

    // The key corresponds to the position within the QComboBoxes
    QMap<qint32, PaHostApiTypeId> avail_portaudio_api;
    QMap<qint32, GekkoFyre::Database::Settings::Audio::GkDevice> avail_input_audio_devs;
    QMap<qint32, GekkoFyre::Database::Settings::Audio::GkDevice> avail_output_audio_devs;
    GekkoFyre::Database::Settings::Audio::GkDevice chosen_input_audio_dev;
    GekkoFyre::Database::Settings::Audio::GkDevice chosen_output_audio_dev;
    qint32 chosen_portaudio_underlying_api;

    static int prefill_rig_selection(const rig_caps *caps, void *data);
    static QMultiMap<rig_model_t, std::tuple<QString, QString, GekkoFyre::AmateurRadio::rig_type>> init_model_names();

    QPointer<GekkoFyre::GkFrequencies> gkFreqs;
    QPointer<GekkoFyre::GkFreqTableModel> gkFreqTableModel;

    void prefill_audio_api_avail(const QVector<PaHostApiTypeId> &portaudio_api_vec);
    void prefill_audio_devices(const std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> &audio_devices_vec);
    void prefill_audio_encode_comboboxes();
    void prefill_event_logger();
    void init_station_info();

    void print_exception(const std::exception &e, int level = 0);
    double convQComboBoxSampleRateToDouble(const int &combobox_idx);

    QMap<int, int> collectComboBoxIndexes(const QComboBox *combo_box);
    void prefill_rig_force_ctrl_lines(const ptt_type_t &ptt_type);
    void prefill_avail_com_ports(const std::list<GekkoFyre::Database::Settings::GkComPort> &com_ports);
    void prefill_avail_usb_ports(const QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> &usb_devices);
    void prefill_com_baud_speed(const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate);
    void enable_device_port_options();
    GekkoFyre::AmateurRadio::GkConnType ascertConnType(const bool &is_ptt = false);

    void update_sample_rates(const bool &is_output_device = false);

    bool read_settings();
};
