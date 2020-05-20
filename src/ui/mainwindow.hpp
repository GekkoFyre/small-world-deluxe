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
#include "src/pa_mic.hpp"
#include "src/gk_cli.hpp"
#include "src/dek_db.hpp"
#include "src/radiolibs.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/spectro_gui.hpp"
#include "src/pa_mic_background.hpp"
#include "src/ui/dialogsettings.hpp"
#include "src/gk_audio_encoding.hpp"
#include "src/gk_audio_decoding.hpp"
#include "src/ui/gkaudioplaydialog.hpp"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/circular_buffer.hpp>
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/options.h>
#include <memory>
#include <ctime>
#include <thread>
#include <future>
#include <mutex>
#include <vector>
#include <string>
#include <QMainWindow>
#include <QPushButton>
#include <QCommandLineParser>
#include <QPointer>
#include <QSharedPointer>
#include <QPrinter>
#include <QString>
#include <QStringList>
#include <QMetaType>
#include <QDateTime>
#include <QSettings>

#ifdef _WIN32
#include "src/string_funcs_windows.hpp"
#elif __linux__
#include "src/string_funcs_linux.hpp"
#endif

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

    void on_actionShow_Waterfall_toggled(bool arg1);
    void on_actionUSB_toggled(bool arg1);
    void on_actionLSB_toggled(bool arg1);
    void on_actionAM_toggled(bool arg1);
    void on_actionFM_toggled(bool arg1);
    void on_actionSSB_toggled(bool arg1);
    void on_actionCW_toggled(bool arg1);

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
    void updateVuMeter(const double &volumePctg);
    void updateVolMeterTooltip(const int &value);
    void updateVolIndex(const int &percentage);
    void on_pushButton_radio_tune_clicked(bool checked);
    void on_verticalSlider_vol_control_valueChanged(int value);
    void on_checkBox_rx_tx_vol_toggle_stateChanged(int arg1);

    //
    // QComboBox'es
    //
    void on_comboBox_select_frequency_activated(int index);
    void on_comboBox_select_digital_mode_activated(int index);

    void infoBar();
    void uponExit();

    void stopAudioCodecRec(const bool &recording_is_started);

protected slots:
    void closeEvent(QCloseEvent *event);

public slots:
    bool stopRecordingInput(const bool &recording_is_stopped, const int &wait_time = 5000);
    void updateSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &data,
                           const std::vector<int> &raw_audio_data,
                           const int &hanning_window_size, const size_t &buffer_size);
    void updateProgressBar(const bool &enable, const size_t &min, const size_t &max);

signals:
    void refreshVuMeter(const double &volumePctg);
    void updatePaVol(const int &percentage);
    void updatePlot();
    void stopRecording(const bool &recording_is_stopped, const int &wait_time = 5000);
    void gkExitApp();
    void sendSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &values,
                         const std::vector<int> &raw_audio_data,
                         const int &hanning_window_size, const size_t &buffer_size);
    void changeFreq(const bool &radio_locked, const GekkoFyre::AmateurRadio::Control::FreqChange &freq_change);
    void changeSettings(const bool &radio_locked, const GekkoFyre::AmateurRadio::Control::SettingsChange &settings_change);

private:
    Ui::MainWindow *ui;

    leveldb::DB *db;
    std::shared_ptr<GekkoFyre::FileIo> fileIo;
    std::shared_ptr<GekkoFyre::GkLevelDb> GkDb;
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDevices;
    std::shared_ptr<GekkoFyre::PaMic> gkPaMic;
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::GkCli> gkCli;
    QPointer<GekkoFyre::RadioLibs> gkRadioLibs;
    QPointer<GekkoFyre::GkAudioEncoding> gkAudioEncoding;
    QPointer<GekkoFyre::GkAudioDecoding> gkAudioDecoding;
    QPointer<GekkoFyre::SpectroGui> gkSpectroGui;
    QPointer<GekkoFyre::paMicProcBackground> paMicProcBackground;
    QPointer<GkAudioPlayDialog> gkAudioPlayDlg;

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
    QPointer<GekkoFyre::PaAudioBuf> pref_input_audio_buf;
    QPointer<GekkoFyre::PaAudioBuf> pref_output_audio_buf;

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
    std::future<std::shared_ptr<GekkoFyre::AmateurRadio::Control::Radio>> rig_thread;

    //
    // USB & RS232
    //
    libusb_context *usb_ctx_ptr;

    //
    // Miscellaneous
    //
    std::shared_ptr<GekkoFyre::AmateurRadio::Control::Radio> gkRadioPtr;
    QPointer<QTimer> timer;

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

    void radioStats(GekkoFyre::AmateurRadio::Control::Radio *radio_dev);

    void changePushButtonColor(const QPointer<QPushButton> &push_button, const bool &green_result = true,
                               const bool &color_blind_mode = false);
    QStringList getAmateurBands();
    bool prefillAmateurBands();

    void launchSettingsWin();
    void radioInitStart(const QString &def_com_port);
    bool radioInitTest(const QString &def_com_port);

    void createStatusBar(const QString &statusMsg = "");
    bool changeStatusBarMsg(const QString &statusMsg = "");
    bool steadyTimer(const int &seconds);
};

Q_DECLARE_METATYPE(std::vector<GekkoFyre::Spectrograph::RawFFT>);
Q_DECLARE_METATYPE(GekkoFyre::AmateurRadio::Control::FreqChange);
Q_DECLARE_METATYPE(GekkoFyre::AmateurRadio::Control::SettingsChange);
Q_DECLARE_METATYPE(std::vector<short>);
Q_DECLARE_METATYPE(size_t);
