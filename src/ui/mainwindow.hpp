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
#include "src/file_io.hpp"
#include "src/audio_devices.hpp"
#include "src/gk_cli.hpp"
#include "src/dek_db.hpp"
#include "src/radiolibs.hpp"
#include "src/pa_audio_buf.hpp"
#include "src/spectro_gui.hpp"
#include "src/spectro_curve.hpp"
#include "src/gk_circ_buffer.hpp"
#include "src/gk_frequency_list.hpp"
#include "src/gk_audio_encoding.hpp"
#include "src/gk_audio_decoding.hpp"
#include "src/ui/dialogsettings.hpp"
#include "src/ui/gkaudioplaydialog.hpp"
#include "src/ui/widgets/gk_display_image.hpp"
#include "src/ui/widgets/gk_vu_meter_widget.hpp"
#include "src/gk_fft.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_modem.hpp"
#include "src/gk_system.hpp"
#include "src/gk_text_to_speech.hpp"
#include <sentry.h>
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
#include <QObject>
#include <QPointer>
#include <QPrinter>
#include <QMetaType>
#include <QDateTime>
#include <QByteArray>
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
    ~MainWindow() override;

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
    void on_action_Documentation_triggered();
    void on_actionSend_Report_triggered();
    void on_action_Battery_Calculator_triggered();

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

    //
    // Transmission & Digital Signalling
    //
    void msgOutgoingProcess(const QString &curr_text);

    //
    // Frequencies related
    //
    void removeFreqFromDb(const GekkoFyre::AmateurRadio::GkFreqs &freq_to_remove);
    void addFreqToDb(const GekkoFyre::AmateurRadio::GkFreqs &freq_to_add);
    void tuneActiveFreq(const quint64 &freq_tune);

    //
    // SSTV related (RX)
    //
    void on_pushButton_sstv_rx_navigate_left_clicked();
    void on_pushButton_sstv_rx_navigate_right_clicked();
    void on_pushButton_sstv_rx_save_image_clicked();
    void on_pushButton_sstv_rx_listen_rx_clicked();
    void on_pushButton_sstv_rx_saved_image_nav_left_clicked();
    void on_pushButton_sstv_rx_saved_image_nav_right_clicked();
    void on_pushButton_sstv_rx_saved_image_load_clicked();
    void on_pushButton_sstv_rx_saved_image_delete_clicked();
    void on_pushButton_sstv_rx_remove_clicked();

    //
    // SSTV related (TX)
    //
    void on_pushButton_sstv_tx_navigate_left_clicked();
    void on_pushButton_sstv_tx_navigate_right_clicked();
    void on_pushButton_sstv_tx_load_image_clicked();
    void on_pushButton_sstv_tx_send_image_clicked();
    void on_pushButton_sstv_tx_remove_clicked();

protected slots:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

    //
    // Radio and Hamlib specific functions
    //
    void procRigPort(const QString &conn_port, const GekkoFyre::AmateurRadio::GkConnMethod &conn_method);

public slots:
    void updateProgressBar(const bool &enable, const size_t &min, const size_t &max);

    //
    // Audio related
    //
    void stopRecordingInput();
    void startRecordingInput();
    void stopTransmitOutput();
    void startTransmitOutput();

    void restartInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &input_device);
    void restartOutputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &output_device);

    //
    // Radio and Hamlib specific functions
    //
    void gatherRigCapabilities(const rig_model_t &rig_model_update,
                               const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void addRigToMemory(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void disconnectRigInMemory(std::shared_ptr<Rig> rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);

    //
    // Spectrograph related
    //
    void changeGraphInUse(const GekkoFyre::Spectrograph::GkGraphType &graph_type);

signals:
    void updatePaVol(const int &percentage);
    void updatePlot();
    void gkExitApp();

    //
    // Radio and Hamlib specific functions
    //
    void changeConnPort(const QString &conn_port, const GekkoFyre::AmateurRadio::GkConnMethod &conn_method);
    void addRigInUse(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void recvRigCapabilities(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void disconnectRigInUse(std::shared_ptr<Rig> rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);

    //
    // Audio related
    //
    void refreshVuDisplay(const qreal &rmsLevel, const qreal &peakLevel, const int &numSamples);
    void changeVolume(const float &value);
    void stopRecording();
    void startRecording();
    void stopTxAudio();
    void startTxAudio();

    //
    // PortAudio and related
    //
    void changeInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &input_device);
    void changeOutputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &output_device);

    //
    // Spectrograph related
    //
    void changeGraphType(const GekkoFyre::Spectrograph::GkGraphType &graph_type);
    void refreshSpectrograph(const qint64 &latest_time_update, const qint64 &time_since);
    void onProcessFrame(const std::vector<float> &fftMagnitude);

private:
    Ui::MainWindow *ui;

    //
    // Class pointers
    //
    leveldb::DB *db;
    sentry_options_t *sen_opt;
    QPointer<GekkoFyre::GkLevelDb> GkDb;
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::GkCli> gkCli;
    std::unique_ptr<GekkoFyre::GkFFT> gkFFT;
    QPointer<GekkoFyre::FileIo> fileIo;
    QPointer<GekkoFyre::GkFrequencies> gkFreqList;
    QPointer<GekkoFyre::RadioLibs> gkRadioLibs;
    QPointer<GekkoFyre::GkAudioEncoding> gkAudioEncoding;
    QPointer<GekkoFyre::GkAudioDecoding> gkAudioDecoding;
    QPointer<GekkoFyre::GkSpectroWaterfall> gkSpectroWaterfall;
    QPointer<GekkoFyre::GkSpectroCurve> gkSpectroCurve;
    QPointer<GkAudioPlayDialog> gkAudioPlayDlg;
    QPointer<GekkoFyre::GkVuMeter> gkVuMeter;
    QPointer<GekkoFyre::GkModem> gkModem;
    QPointer<GekkoFyre::GkSystem> gkSystem;
    QPointer<GekkoFyre::GkTextToSpeech> gkTextToSpeech;

    std::shared_ptr<QCommandLineParser> gkCliParser;
    // std::shared_ptr<QPrinter> printer;

    //
    // Events logger
    //
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

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
    QMap<int, GekkoFyre::Database::Settings::Audio::GkDevice> avail_input_audio_devs;
    QMap<int, GekkoFyre::Database::Settings::Audio::GkDevice> avail_output_audio_devs;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_output_device;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_input_device;
    std::shared_ptr<GekkoFyre::PaAudioBuf<float>> input_audio_buf;
    std::shared_ptr<GekkoFyre::PaAudioBuf<float>> output_audio_buf;
    std::shared_ptr<portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf<float>>> inputAudioStream;
    std::shared_ptr<portaudio::MemFunCallbackStream<GekkoFyre::PaAudioBuf<float>>> outputAudioStream;

    //
    // Audio sub-system
    //
    bool rx_vol_control_selected;
    double global_rx_audio_volume;
    double global_tx_audio_volume;

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
    QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> gkUsbPortMap; // This is used for making connections to radio rigs with Hamlib!
    std::list<GekkoFyre::Database::Settings::GkComPort> status_com_ports; // This variable is responsible for managing the COM/RS232/Serial ports!

    //
    // Radio and Hamlib related
    //
    static QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, GekkoFyre::AmateurRadio::rig_type>> gkRadioModels;
    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> gkRadioPtr;
    QList<GekkoFyre::AmateurRadio::GkFreqs> frequencyList;

    //
    // SSTV related
    //
    QPointer<GekkoFyre::GkDisplayImage> label_sstv_tx_image;
    QPointer<GekkoFyre::GkDisplayImage> label_sstv_rx_live_image;
    QPointer<GekkoFyre::GkDisplayImage> label_sstv_rx_saved_image;
    QStringList sstv_tx_pic_files;                                      // Images loaded from the user's storage media and prepared for transmission
    QStringList sstv_rx_saved_image_files;                              // Images saved to history and then to the user's storage media
    QList<QPixmap> sstv_rx_image_pixmap;                                // Images received in real-time
    QList<QPixmap> sstv_rx_saved_image_pixmap;                          // Images saved to history
    int sstv_tx_image_idx;                                              // Images loaded and prepared for transmission
    int sstv_rx_image_idx;                                              // Images received in real-time
    int sstv_rx_saved_image_idx;                                        // Images saved to history

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
    void updateVolumeSliderLabel(const float &vol_level);

    //
    // QFileDialog related
    //
    bool fileOverloadWarning(const int &file_count, const int &max_num_files = GK_SSTV_FILE_DLG_LOAD_IMGS_MAX_FILES_WARN);

    //
    // Spectrograph related
    //
    GekkoFyre::Spectrograph::GkGraphType graph_in_use;                  // The type of graph in use and therefore displayed to the end-user as of the moment...

    void updateSpectrograph();

    void createStatusBar(const QString &statusMsg = "");
    bool changeStatusBarMsg(const QString &statusMsg = "");
    bool steadyTimer(const int &seconds);
};

Q_DECLARE_METATYPE(std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio>);
Q_DECLARE_METATYPE(GekkoFyre::Database::Settings::GkUsbPort);
Q_DECLARE_METATYPE(GekkoFyre::Spectrograph::GkFFTSpectrum);
Q_DECLARE_METATYPE(GekkoFyre::AmateurRadio::GkConnMethod);
Q_DECLARE_METATYPE(GekkoFyre::System::Events::Logging::GkEventLogging);
Q_DECLARE_METATYPE(GekkoFyre::System::Events::Logging::GkSeverity);
Q_DECLARE_METATYPE(GekkoFyre::Database::Settings::Audio::GkDevice);
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
Q_DECLARE_METATYPE(std::vector<qint16>);
Q_DECLARE_METATYPE(std::vector<double>);
Q_DECLARE_METATYPE(std::vector<float>);
