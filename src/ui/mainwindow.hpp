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
#include "src/file_io.hpp"
#include "src/audio_devices.hpp"
#include "src/gk_cli.hpp"
#include "src/dek_db.hpp"
#include "src/radiolibs.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/spectro_gui.hpp"
#include "src/gk_circ_buffer.hpp"
#include "src/gk_frequency_list.hpp"
#include "src/ui/dialogsettings.hpp"
#include "src/gk_audio_encoding.hpp"
#include "src/gk_audio_decoding.hpp"
#include "src/gk_fft.hpp"
#include "src/ui/gkaudioplaydialog.hpp"
#include "src/ui/gk_vu_meter_widget.hpp"
#include "src/gk_string_funcs.hpp"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/options.h>
#include <stdexcept>
#include <exception>
#include <complex>
#include <memory>
#include <thread>
#include <future>
#include <vector>
#include <string>
#include <mutex>
#include <ctime>
#include <list>
#include <QList>
#include <QString>
#include <QPointer>
#include <QPrinter>
#include <QMetaType>
#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QMainWindow>
#include <QPushButton>
#include <QSharedPointer>
#include <QCommandLineParser>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionE_xit_triggered();
    void on_action_Open_triggered();
    void on_actionCheck_for_Updates_triggered();
    void on_action_About_Dekoder_triggered();
    void on_actionSet_Offset_triggered();
    void on_action_Settings_triggered();
    void on_actionSave_Decoded_triggered();
    void on_actionSave_All_triggered();
    void on_actionOpen_Save_Directory_triggered();
    void on_actionDelete_all_wav_files_in_Save_Directory_triggered();
    void on_actionPlay_triggered();
    void on_actionSettings_triggered();
    void on_actionSave_Decoded_Ab_triggered();
    void on_actionView_Spectrogram_Controller_triggered();
    void on_action_Print_triggered();
    void on_action_All_triggered();
    void on_action_Incoming_triggered();
    void on_action_Outgoing_triggered();
    void on_actionPrint_triggered();

    void on_action_Connect_triggered();
    void on_action_Disconnect_triggered();
    void on_actionShow_Waterfall_toggled(bool arg1);
    void on_actionUSB_toggled(bool arg1);
    void on_actionLSB_toggled(bool arg1);
    void on_actionAM_toggled(bool arg1);
    void on_actionFM_toggled(bool arg1);
    void on_actionSSB_toggled(bool arg1);
    void on_actionCW_toggled(bool arg1);

    //
    // Documentation
    //
    void on_action_Q_codes_triggered();

    //
    // QPushButtons that contain a logic state of some sort and are therefore displayed as
    // the color, "Green", when TRUE and the color, "Red", when FALSE. Note: May possibly
    // be different colors depending on the needs of any colorblind users for a given session.
    //
    void on_pushButton_bridge_input_audio_clicked();
    void on_pushButton_radio_receive_clicked();
    void on_pushButton_radio_transmit_clicked();
    void on_pushButton_radio_tx_halt_clicked();
    void on_pushButton_radio_monitor_clicked();

    //
    // Audio/Volume related controls
    //
    void on_verticalSlider_vol_control_valueChanged(int value);
    void on_pushButton_radio_tune_clicked(bool checked);
    void on_checkBox_rx_tx_vol_toggle_stateChanged(int arg1);

    //
    // QComboBox'es
    //
    void on_comboBox_select_frequency_activated(int index);
    void on_comboBox_select_callsign_use_currentIndexChanged(int index);

    void infoBar();
    void uponExit();

    //
    // Audio related
    //
    void updateVolume(const float &value);

protected slots:
    void closeEvent(QCloseEvent *event);

public slots:
    void updateProgressBar(const bool &enable, const size_t &min, const size_t &max);

    //
    // Audio related
    //
    void stopRecordingInput(const int &wait_time = 5000);
    void startRecordingInput(const int &wait_time = 5000);

    //
    // Radio and Hamlib specific functions
    //
    void selectedPortType(const GekkoFyre::AmateurRadio::GkConnType &rig_conn_type, const bool &is_cat_mode);
    void analyzePortType(const bool &is_cat_mode);
    void gatherRigCapabilities(const rig_model_t &rig_model_update,
                               const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void addRigToMemory(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void disconnectRigInMemory(RIG *rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void updateFreqsInMem(const float &frequency, const GekkoFyre::AmateurRadio::DigitalModes &digital_mode,
                          const GekkoFyre::AmateurRadio::IARURegions &iaru_region, const bool &remove_freq);

signals:
    void updatePaVol(const int &percentage);
    void updatePlot();
    void gkExitApp();

    //
    // Radio and Hamlib specific functions
    //
    void gatherPortType(const bool &is_cat_mode);
    void changePortType(const GekkoFyre::AmateurRadio::GkConnType &rig_conn_type, const bool &is_cat_mode);
    void addRigInUse(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void recvRigCapabilities(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void disconnectRigInUse(RIG *rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void updateFrequencies(const float &frequency, const GekkoFyre::AmateurRadio::DigitalModes &digital_mode,
                           const GekkoFyre::AmateurRadio::IARURegions &iaru_region, const bool &remove_freq);

    //
    // Audio related
    //
    void refreshVuDisplay(const qreal &rmsLevel, const qreal &peakLevel, const int &numSamples);
    void changeVolume(const float &value);
    void stopRecording(const int &wait_time = 5000);
    void startRecording(const int &wait_time = 5000);

    //
    // Spectrograph related
    //
    void refreshSpectrograph(const qint64 &latest_time_update, const qint64 &time_since);

private:
    Ui::MainWindow *ui;

    //
    // Class pointers
    //
    leveldb::DB *db;
    std::shared_ptr<GekkoFyre::GkLevelDb> GkDb;
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDevices;
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::GkCli> gkCli;
    std::unique_ptr<GekkoFyre::GkFFT> gkFFT;
    QPointer<GekkoFyre::FileIo> fileIo;
    QPointer<GekkoFyre::GkFreqList> gkFreqList;
    QPointer<GekkoFyre::RadioLibs> gkRadioLibs;
    QPointer<GekkoFyre::GkAudioEncoding> gkAudioEncoding;
    QPointer<GekkoFyre::GkAudioDecoding> gkAudioDecoding;
    QPointer<GekkoFyre::SpectroGui> gkSpectroGui;
    QPointer<GkAudioPlayDialog> gkAudioPlayDlg;
    QPointer<GekkoFyre::GkVuMeter> gkVuMeter;

    std::shared_ptr<QSettings> sw_settings;
    std::shared_ptr<QCommandLineParser> gkCliParser;
    // std::shared_ptr<QPrinter> printer;

    //
    // Filesystem
    //
    boost::filesystem::path save_db_path;
    boost::filesystem::path native_slash;

    //
    // PortAudio initialization and buffers
    //
    portaudio::AutoSystem autoSys;
    portaudio::System *gkPortAudioInit;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_output_device;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_input_device;
    std::shared_ptr<GekkoFyre::PaAudioBuf<int16_t>> input_audio_buf;
    std::shared_ptr<GekkoFyre::PaAudioBuf<int16_t>> output_audio_buf;
    portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf<int16_t>> *inputAudioStream;

    //
    // Audio sub-system
    //
    bool rx_vol_control_selected;
    double global_rx_audio_volume;
    double global_tx_audio_volume;
    bool recording_in_progress;

    //
    // Multithreading
    // https://www.boost.org/doc/libs/1_72_0/doc/html/thread/thread_management.html
    //
    std::timed_mutex btn_record_mtx;
    std::future<std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio>> rig_future;
    std::thread rig_thread;
    std::thread vu_meter_thread;
    std::thread spectro_timing_thread;

    //
    // USB & RS232
    //
    libusb_context *usb_ctx_ptr;
    std::shared_ptr<GekkoFyre::Database::Settings::GkUsbPort> gkUsbPortPtr; // This is used for making connections to radio rigs with Hamlib!
    static std::list<GekkoFyre::Database::Settings::GkComPort> status_com_ports; // This variable is responsible for managing the COM/RS232/Serial ports!

    //
    // Radio and Hamlib related
    //
    static QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, GekkoFyre::AmateurRadio::rig_type>> gkRadioModels;
    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> gkRadioPtr;
    QList<GekkoFyre::AmateurRadio::GkFreqs> frequencyList;

    //
    // Timing and date related
    //
    QPointer<QTimer> info_timer;
    qint64 gk_spectro_start_time;
    qint64 gk_spectro_latest_time;

    //
    // This sub-section contains all the boolean variables pertaining to the QPushButtons on QMainWindow that
    // possess a logic state of some kind. If the button holds a TRUE value, it'll be 'Green' in colour, otherwise
    // it'll appear 'Red' in order to display its FALSE value.
    // See: MainWindow::changePushButtonColor().
    //
    bool btn_bridge_input_audio;
    bool btn_radio_rx;
    bool btn_radio_tx;
    bool btn_radio_tx_halt;
    bool btn_radio_tune;
    bool btn_radio_monitor;

    void radioStats(GekkoFyre::AmateurRadio::Control::GkRadio *radio_dev);

    void changePushButtonColor(const QPointer<QPushButton> &push_button, const bool &green_result = true,
                               const bool &color_blind_mode = false);
    QStringList getAmateurBands();
    bool prefillAmateurBands();

    void launchSettingsWin();
    bool radioInitStart();

    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> readRadioSettings();
    static int parseRigCapabilities(const rig_caps *caps, void *data);
    static QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, GekkoFyre::AmateurRadio::rig_type>> initRadioModelsVar();

    void updateVolumeDisplayWidgets();

    //
    // Spectrograph related
    //
    void updateSpectrograph();

    void createStatusBar(const QString &statusMsg = "");
    bool changeStatusBarMsg(const QString &statusMsg = "");
    bool steadyTimer(const int &seconds);
    void print_exception(const std::exception &e, int level = 0);
};

Q_DECLARE_METATYPE(std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio>);
Q_DECLARE_METATYPE(GekkoFyre::Database::Settings::GkUsbPort);
Q_DECLARE_METATYPE(GekkoFyre::AmateurRadio::GkConnType);
Q_DECLARE_METATYPE(GekkoFyre::AmateurRadio::DigitalModes);
Q_DECLARE_METATYPE(GekkoFyre::AmateurRadio::IARURegions);
Q_DECLARE_METATYPE(GekkoFyre::Spectrograph::GkGraphType);
Q_DECLARE_METATYPE(GekkoFyre::AmateurRadio::GkFreqs);
Q_DECLARE_METATYPE(RIG);
Q_DECLARE_METATYPE(size_t);
Q_DECLARE_METATYPE(uint8_t);
Q_DECLARE_METATYPE(rig_model_t);
Q_DECLARE_METATYPE(PaHostApiTypeId);
Q_DECLARE_METATYPE(std::vector<short>);
