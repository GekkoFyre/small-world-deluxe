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
#include "src/file_io.hpp"
#include "src/audio_devices.hpp"
#include "src/gk_cli.hpp"
#include "src/dek_db.hpp"
#include "src/radiolibs.hpp"
#include "src/ui/gkintrosetupwizard.hpp"
#include "src/ui/gkatlasdialog.hpp"
#include "src/gk_waterfall_gui.hpp"
#include "src/gk_fft_audio.hpp"
#include "src/gk_frequency_list.hpp"
#include "src/gk_xmpp_client.hpp"
#include "src/update/gk_network.hpp"
#include "src/ui/dialogsettings.hpp"
#include "src/ui/xmpp/gkxmpprosterdialog.hpp"
#include "src/ui/widgets/gk_display_image.hpp"
#include "src/ui/widgets/gk_vu_meter_widget.hpp"
#include "src/ui/weather/signal-propagation/gksolarweatherforecast.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_modem.hpp"
#include "src/gk_system.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_multimedia.hpp"
#include "src/gk_sdr.hpp"
#include <marble/MarbleWidget.h>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <sentry.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/options.h>
#include <stdexcept>
#include <exception>
#include <utility>
#include <complex>
#include <memory>
#include <thread>
#include <future>
#include <vector>
#include <string>
#include <mutex>
#include <ctime>
#include <list>
#include <QMenu>
#include <QRect>
#include <QList>
#include <QThread>
#include <QAction>
#include <QScreen>
#include <QString>
#include <QObject>
#include <QPointer>
#include <QPrinter>
#include <QMetaType>
#include <QDateTime>
#include <QWindow>
#include <QByteArray>
#include <QStringList>
#include <QMainWindow>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QCommandLineParser>

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
#include <qwt-qt5/qwt_legend_data.h>
#else
#include <qwt/qwt_legend_data.h>
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QThread gkAudioInputThread;
    QThread gkAudioOutputThread;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_actionXMPP_triggered();
    void on_actionE_xit_triggered();
    void on_action_Open_triggered();
    void on_actionCheck_for_Updates_triggered();
    void on_action_About_Dekoder_triggered();
    void on_actionSet_Offset_triggered();
    void on_actionAdjust_Volume_triggered();
    void on_actionModify_Preferences_triggered();
    void on_actionReset_triggered();
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
    void on_actionConnect_triggered();

    void on_action_Connect_triggered();
    void on_action_Disconnect_triggered();
    void on_actionShow_Waterfall_toggled(bool arg1);
    void on_actionUSB_toggled(bool arg1);
    void on_actionLSB_toggled(bool arg1);
    void on_actionAM_toggled(bool arg1);
    void on_actionFM_toggled(bool arg1);
    void on_actionCW_toggled(bool arg1);

    //
    // [ Main Menu ] General - Bookmarks
    //
    void on_comboBox_general_settings_favs_bookmark_category_currentIndexChanged(const QString &arg1);
    void on_toolButton_general_settings_favs_rename_category_triggered(QAction *arg1);
    void on_toolButton_general_settings_favs_add_category_triggered(QAction *arg1);
    void on_toolButton_general_settings_favs_delete_category_triggered(QAction *arg1);
    void on_toolButton_general_settings_favs_add_bookmark_triggered(QAction *arg1);
    void on_toolButton_general_settings_favs_delete_bookmark_triggered(QAction *arg1);
    void on_toolButton_general_settings_favs_edit_bookmark_triggered(QAction *arg1);
    void on_toolButton_general_settings_favs_apply_triggered(QAction *arg1);
    void on_toolButton_general_settings_favs_import_triggered(QAction *arg1);
    void on_toolButton_general_settings_favs_export_triggered(QAction *arg1);

    //
    // [ Main Menu ] SoapySDR - Radio
    //
    void on_comboBox_main_soapysdr_source_hw_key_currentIndexChanged(const QString &arg1);
    void on_comboBox_main_soapysdr_source_hw_id_enum_currentIndexChanged(const QString &arg1);
    void on_comboBox_main_soapysdr_source_rx_channel_currentIndexChanged(const QString &arg1);
    void on_comboBox_main_soapysdr_source_viewable_bw_activated(qint32 index);
    void on_comboBox_main_soapysdr_source_samplerate_activated(qint32 index);
    void on_comboBox_main_soapysdr_source_gain_control_mode_currentIndexChanged(const QString &arg1);
    void on_tableView_main_soapysdr_module_mgr_customContextMenuRequested(const QPoint &pos);
    void on_radioButton_soapysdr_source_modulation_nfm_clicked(bool checked);
    void on_radioButton_soapysdr_source_modulation_am_clicked(bool checked);
    void on_radioButton_soapysdr_source_modulation_usb_clicked(bool checked);
    void on_radioButton_soapysdr_source_modulation_lsb_clicked(bool checked);
    void on_radioButton_soapysdr_source_modulation_wfm_clicked(bool checked);
    void on_radioButton_soapysdr_source_modulation_dsb_clicked(bool checked);
    void on_radioButton_soapysdr_source_modulation_cw_clicked(bool checked);
    void on_radioButton_soapysdr_source_modulation_raw_clicked(bool checked);
    void repaint_soapysdr_source_modulation_radiobuttons();

    //
    // QXmpp and XMPP related
    //
    void on_actionView_Roster_triggered();
    void on_actionSign_in_triggered();
    void on_actionSign_out_triggered();
    void on_action_Register_Account_triggered();
    void on_actionOnline_triggered();
    void on_actionInvisible_triggered();
    void on_actionOffline_triggered();
    void readXmppSettings();

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
    void on_pushButton_radio_transmit_clicked();
    void on_pushButton_radio_tx_halt_clicked();
    void on_pushButton_radio_rx_halt_clicked();
    void on_pushButton_radio_monitor_clicked();

    //
    // Custom context-menu
    //
    void on_tableView_maingui_logs_customContextMenuRequested(const QPoint &pos);

    //
    // Audio/Volume related controls
    //
    void on_verticalSlider_vol_control_sliderMoved(int position);
    void on_pushButton_radio_tune_clicked(bool checked);
    void on_checkBox_rx_tx_vol_toggle_stateChanged(int arg1);
    void updateVolume(const qint32 &value);
    void procVolumeChanges(const qint32 &value);

    //
    // QComboBox'es
    //
    void on_comboBox_select_frequency_activated(qint32 index);

    void infoBar();
    void uponExit();

    //
    // QMessageBox's
    //
    void configInputAudioDevice();

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

    //
    // QXmpp and XMPP related
    //
    void launchXmppRosterDlg();

    //
    // SSTV and related
    //
    void launchSstvTab();

    //
    // Weather-forecast and signal propagation related
    //
    void on_actionSolar_Weather_Forecast_triggered();

    //
    // Settings dialog and related
    //
    void launchSettingsWin(const GekkoFyre::System::UserInterface::GkSettingsDlgTab &settingsDlgTab = GekkoFyre::System::UserInterface::GkSettingsDlgTab::GkGeneralStation);
    void actionLaunchSettingsWin();

    //
    // Mapping and atlas APIs, etc.
    //
    void on_actionView_World_Map_triggered();

    //
    // System tray icon related functions
    //
    void setIcon();

protected slots:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

    //
    // Radio, Hamlib, and SoapySDR specific functions
    //
    void procRigPort(const QString &conn_port, const GekkoFyre::AmateurRadio::GkConnMethod &conn_method);
    void findSoapySdrDevs();
    void discSoapySdrDevs(const QList<GekkoFyre::System::GkSdr::GkSoapySdrTableView> &sdr_devs);
    void updateSoapySdrBandwidthComboBoxes();
    void updateSoapySdrGainComboBoxes();

public slots:
    void restartInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &input_device);

    //
    // Radio, Hamlib, and SoapySDR specific functions
    //
    void gatherRigCapabilities(const rig_model_t &rig_model_update,
                               const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void addRigToMemory(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void disconnectRigInMemory(Rig *rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);

    //
    // VCard management for (Q)Xmpp
    //
    void updateDisplayedClientAvatar(const QByteArray &ba_img);

signals:
    void updatePaVol(const int &percentage);
    void gkExitApp();

    //
    // QSplashScreen and startup
    //
    void setStartupProgress(const qint32 &value);

    //
    // Radio, Hamlib, and SoapySDR specific functions
    //
    void changeConnPort(const QString &conn_port, const GekkoFyre::AmateurRadio::GkConnMethod &conn_method);
    void addRigInUse(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void recvRigCapabilities(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void disconnectRigInUse(Rig *rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);
    void searchSoapySdrDevs();                                                                         // Instruction to begin searching for SDR devices via SoapySDR!
    void foundSoapySdrDevs(const QList<GekkoFyre::System::GkSdr::GkSoapySdrTableView> &sdr_devs);      // Any SDR devices that have been found via SoapySDR.
    void refreshSoapySdrBandwidthComboBoxes();
    void refreshSoapySdrGainComboBoxes();

    //
    // [ Main Menu ] SoapySDR - Radio
    //
    void repaintSoapySdrRadioButtons();

    //
    // FFT & Spectrograph related
    //
    void initSpectrograph();

    //
    // Audio related
    //
    void updateAudioOut();
    void refreshVuDisplay(const qreal &rmsLevel, const qreal &peakLevel, const int &numSamples);
    void changeVolume(const qint32 &value);
    void changeGlobalVolume(const qint32 &value);

    //
    // Audio System and related
    //
    void changeInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &input_device);

    //
    // VCard management for (Q)Xmpp
    //
    void refreshDisplayedClientAvatar(const QByteArray &ba_img);

private:
    Ui::MainWindow *ui;

    //
    // Class pointers
    //
    leveldb::DB *db;
    sentry_options_t *sen_opt;
    QPointer<GekkoFyre::GkLevelDb> gkDb;
    QPointer<GekkoFyre::GkAudioDevices> gkAudioDevices;
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::GkCli> gkCli;
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::GkFrequencies> gkFreqList;
    QPointer<GekkoFyre::RadioLibs> gkRadioLibs;
    // QPointer<GekkoFyre::GkVuMeter> gkVuMeter;
    QPointer<GekkoFyre::GkModem> gkModem;
    QPointer<GekkoFyre::GkSystem> gkSystem;
    QPointer<GekkoFyre::GkMultimedia> gkMultimedia;
    QPointer<GekkoFyre::GkSdrDev> gkSdrDev;
    QPointer<GkIntroSetupWizard> gkIntroSetupWizard;
    // QPointer<GekkoFyre::GkTextToSpeech> gkTextToSpeech;

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
    // Audio System miscellaneous variables
    //
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> gkSysOutputAudioDevs;
    std::vector<GekkoFyre::Database::Settings::Audio::GkDevice> gkSysInputAudioDevs;
    QPointer<GekkoFyre::GkFFTAudio> gkFftAudio;
    GekkoFyre::GkAudioFramework::GkAudioRecordStatus gkSysOutputDevStatus;
    GekkoFyre::GkAudioFramework::GkAudioRecordStatus gkSysInputDevStatus;
    qint32 audioFrameSampleCountPerChannel;
    qint32 audioFrameSampleCountTotal;
    ALCsizei circBufSize;

    //
    // Audio sub-system
    //
    void captureAlcSamples(ALCdevice *device, std::shared_ptr<std::vector<ALshort>> deviceRecBuf, ALCsizei samples);
    double global_rx_audio_volume;
    double global_tx_audio_volume;
    quint32 m_maxAmplitude;

    //
    // Multithreading
    // https://www.boost.org/doc/libs/1_72_0/doc/html/thread/thread_management.html
    //
    std::timed_mutex btn_record_mtx;
    std::mutex gkCaptureAudioSamplesMtx;
    std::future<std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio>> rig_future;
    std::thread rig_thread;
    std::thread vu_meter_thread;
    std::thread capture_input_audio_samples;

    //
    // USB & RS232
    //
    QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> gkUsbPortMap;   // This is used for making connections to radio rigs with Hamlib!
    std::vector<GekkoFyre::Database::Settings::GkComPort> gkSerialPortMap;  // This variable is responsible for managing the COM/RS232/Serial ports!

    //
    // Radio and Hamlib related
    //
    static QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, GekkoFyre::AmateurRadio::rig_type>> gkRadioModels;
    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> gkRadioPtr;
    QList<GekkoFyre::System::GkSdr::GkSoapySdrTableView> m_sdrDevs;         // Any applicable SDR devices that have been enumerated via SoapySDR!
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
    QPointer<QTimer> changeVolTimer;

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
    bool btn_radio_rx_halt;
    bool btn_radio_tune;
    bool btn_radio_monitor;

    QStringList getAmateurBands();
    bool prefillAmateurBands();

    void launchAudioPlayerWin();
    bool radioInitStart();

    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> readRadioSettings();
    static int parseRigCapabilities(const rig_caps *caps, void *data);
    static QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, GekkoFyre::AmateurRadio::rig_type>> initRadioModelsVar();

    //
    // QFileDialog related
    //
    bool fileOverloadWarning(const int &file_count, const int &max_num_files = GK_SSTV_FILE_DLG_LOAD_IMGS_MAX_FILES_WARN);

    //
    // QXmpp and XMPP related
    //
    QPointer<GekkoFyre::GkXmppClient> m_xmppClient;
    QPointer<GkXmppRosterDialog> gkXmppRosterDlg;
    GekkoFyre::Network::GkXmpp::GkUserConn gkConnDetails;
    std::shared_ptr<QList<GekkoFyre::Network::GkXmpp::GkXmppCallsign>> m_rosterList;   // A list of all the bareJids, including the client themselves!

    //
    // Spectrograph related
    //
    QPointer<GekkoFyre::GkSpectroWaterfall> gkSpectroWaterfall;
    QVector<double> waterfall_samples_vec;
    GekkoFyre::Spectrograph::GkGraphType graph_in_use;

    //
    // Mapping and atlas APIs, etc.
    //
    QPointer<Marble::MarbleWidget> m_mapWidget;
    QPointer<GkAtlasDialog> gkAtlasDlg;

    //
    // Solar weather, forecasts, etc.
    //
    QPointer<GkSolarWeatherForecast> gkSolarWeatherForecast;

    //
    // System tray icon
    //
    QPointer<QSystemTrayIcon> m_trayIcon;
    QPointer<QMenu> m_trayIconMenu;
    QPointer<QAction> m_xmppRosterAction;
    QPointer<QAction> m_sstvAction;
    QPointer<QAction> m_settingsAction;
    QPointer<QAction> m_restoreAction;
    QPointer<QAction> m_quitAction;

    void spectroSamplesUpdated();

    void startMappingRoutines();
    void createSolarWeatherForecastDlg();

    void createStatusBar(const QString &statusMsg = "");
    bool changeStatusBarMsg(const QString &statusMsg = "");
    bool steadyTimer(const int &seconds);
    QRect findActiveScreen();

    //
    // XMPP and related
    //
    void createXmppConnection();

    //
    // System tray icon related functions
    //
    void createTrayActions();
    void createTrayIcon();

    void print_exception(const std::exception &e, const bool &displayMsgBox = false, int level = 0);

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
Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::GkBitrate);
Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::GkClearForms);
Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::CodecSupport);
Q_DECLARE_METATYPE(GekkoFyre::Database::Settings::GkAudioSource);
Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::GkAudioRecordStatus);
Q_DECLARE_METATYPE(GekkoFyre::AmateurRadio::GkFreqs);
Q_DECLARE_METATYPE(GekkoFyre::GkAudioFramework::GkAudioState);
Q_DECLARE_METATYPE(GekkoFyre::Network::GkDataState);
Q_DECLARE_METATYPE(GekkoFyre::Network::GkXmpp::GkXmppMsgTabRoster);
Q_DECLARE_METATYPE(boost::filesystem::path);
Q_DECLARE_METATYPE(std::shared_ptr<aria2::DownloadHandle>);
Q_DECLARE_METATYPE(SoapySDR::Kwargs);
Q_DECLARE_METATYPE(GekkoFyre::System::GkSdr::GkSoapySdrTableView);
Q_DECLARE_METATYPE(RIG);
Q_DECLARE_METATYPE(size_t);
Q_DECLARE_METATYPE(uint8_t);
Q_DECLARE_METATYPE(rig_model_t);
Q_DECLARE_METATYPE(QList<QwtLegendData>);
Q_DECLARE_METATYPE(std::vector<qint16>);
Q_DECLARE_METATYPE(std::vector<double>);
Q_DECLARE_METATYPE(std::vector<float>);
