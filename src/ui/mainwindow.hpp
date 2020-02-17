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
#include "src/dek_db.hpp"
#include "src/audio_devices.hpp"
#include "src/pa_mic.hpp"
#include "src/radiolibs.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/spectro_gui.hpp"
#include "dialogsettings.hpp"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <memory>
#include <ctime>
#include <thread>
#include <future>
#include <mutex>
#include <QMainWindow>
#include <QPushButton>
#include <QPointer>
#include <QString>
#include <QStringList>

#ifdef __cplusplus
extern "C"
{
#endif

#include <portaudio.h>

#ifdef __cplusplus
} // extern "C"
#endif

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
    void on_actionShow_Waterfall_toggled(bool arg1);
    void on_action_Settings_triggered();
    void on_actionSave_Decoded_triggered();
    void on_actionSave_All_triggered();
    void on_actionOpen_Save_Directory_triggered();
    void on_actionDelete_all_wav_files_in_Save_Directory_triggered();
    void on_actionUSB_triggered(bool checked);
    void on_actionLSB_triggered(bool checked);
    void on_actionAM_triggered(bool checked);
    void on_actionFM_triggered(bool checked);
    void on_actionSSB_triggered(bool checked);
    void on_actionCW_triggered(bool checked);
    void on_actionRecord_triggered();
    void on_actionPlay_triggered();
    void on_actionSettings_triggered();
    void on_actionSave_Decoded_Ab_triggered();
    void on_actionView_Spectrogram_Controller_triggered();

    void infoBar();
    void uponExit();

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
    void on_verticalSlider_vol_control_sliderMoved(int position);
    void on_pushButton_radio_tune_clicked(bool checked);

    //
    // Graphing / Spectrogram / Waterfall
    //
    bool manageSpectroTiming(const int &y_axis, portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *stream);
    bool manageSpectroData(const int &x_axis, portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *stream);
    bool refreshSpectroGui();

    //
    // QComboBox'es
    //
    void on_comboBox_select_frequency_activated(int index);
    void on_comboBox_select_digital_mode_activated(int index);

    void on_action_Print_triggered();

protected slots:
    void closeEvent(QCloseEvent *event);

public slots:
    bool stopRecordingInput(const bool &recording_is_stopped, const int &wait_time = 5000);

signals:
    void updateVolume(const double &volumePctg);
    bool updateSpectroTiming(const int &y_axis, portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *stream);
    bool updateSpectroData(const int &x_axis, portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *stream);
    bool updatePlot();
    void toggleRecording(const bool &is_rec_input);
    bool stopRecording(const bool &recording_is_stopped, const int &wait_time = 5000);
    void gkExitApp();

private:
    Ui::MainWindow *ui;

    leveldb::DB *db;
    boost::filesystem::path save_db_path;
    std::shared_ptr<GekkoFyre::FileIo> fileIo;
    std::shared_ptr<GekkoFyre::GkLevelDb> GkDb;
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDevices;
    std::shared_ptr<GekkoFyre::PaMic> gkPaMic;
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::RadioLibs> gkRadioLibs;
    QPointer<GekkoFyre::SpectroGui> gkSpectroGui;

    //
    // Window Handlers for Microsoft message boxes
    //
    HWND hwnd_terminating_msg_box;

    //
    // PortAudio initialization and buffers
    //
    portaudio::AutoSystem autoSys;
    portaudio::System *gkPortAudioInit;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_output_device;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_input_device;

    //
    // Multithreading
    // https://www.boost.org/doc/libs/1_72_0/doc/html/thread/thread_management.html
    //
    std::future<GekkoFyre::AmateurRadio::Control::Radio *> rig_thread;
    boost::thread tInputDev;

    GekkoFyre::AmateurRadio::Control::Radio *radio;
    QTimer *timer;

    PaError err = paNoError;
    std::shared_ptr<PaStream> micStream;

    //
    // Mutexes
    //
    std::mutex spectrograph_callback_mtx;

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

    PaStreamCallbackResult paMicProcBackground(const GekkoFyre::Database::Settings::Audio::GkDevice &input_audio_device);
    void procVuMeter(GekkoFyre::PaAudioBuf *buffer, portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *stream);

    void changePushButtonColor(QPointer<QPushButton> push_button, const bool &green_result = true,
                               const bool &color_blind_mode = false);
    QStringList getAmateurBands();
    bool prefillAmateurBands();

    void spectrographCallback(const GekkoFyre::Database::Settings::Audio::GkDevice &device,
                              std::vector<short> *buffer_vec,
                              portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf> *stream);

    void createStatusBar(const QString &statusMsg = "");
    bool changeStatusBarMsg(const QString &statusMsg = "");
    bool steadyTimer(const int &seconds);
    void print_exception(const std::exception &e, int level = 0);
    void appTerminating();
};
