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

#include "src/ui/mainwindow.hpp"
#include "ui_mainwindow.h"
#include "src/ui/aboutdialog.hpp"
#include "src/ui/spectrodialog.hpp"
#include "src/ui/widgets/gk_submit_msg.hpp"
#include "src/models/tableview/gk_frequency_model.hpp"
#include "src/models/tableview/gk_logger_model.hpp"
#include "src/gk_codec2.hpp"
#include <boost/exception/all.hpp>
#include <boost/chrono/chrono.hpp>
#include <cmath>
#include <chrono>
#include <sstream>
#include <ostream>
#include <cstring>
#include <utility>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <functional>
#include <QDesktopServices>
#include <QSerialPortInfo>
#include <QStandardPaths>
#include <QPrintDialog>
#include <QSerialPort>
#include <QMessageBox>
#include <QFileDialog>
#include <QIODevice>
#include <QResource>
#include <QMultiMap>
#include <QtGlobal>
#include <QWidget>
#include <QVector>
#include <QPixmap>
#include <QBuffer>
#include <QTimer>
#include <QDate>
#include <QFile>
#include <QDir>
#include <QUrl>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef GK_CUDA_FFT_ENBL
#include "./../spectro_cuda.h"
#endif

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

//
// Statically declared members
//
QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, GekkoFyre::AmateurRadio::rig_type>> MainWindow::gkRadioModels = initRadioModelsVar();

/**
 * @brief MainWindow::MainWindow
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @note main_file_input.cpp <https://github.com/lincolnhard/spectrogram-opencv/blob/master/examples/main_file_input.cpp>
 * main_mic_input.cpp <https://github.com/lincolnhard/spectrogram-opencv/blob/master/examples/main_mic_input.cpp>
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio>>("std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio>");
    qRegisterMetaType<GekkoFyre::Database::Settings::GkUsbPort>("GekkoFyre::Database::Settings::GkUsbPort");
    qRegisterMetaType<GekkoFyre::AmateurRadio::GkConnMethod>("GekkoFyre::AmateurRadio::GkConnMethod");
    qRegisterMetaType<GekkoFyre::System::Events::Logging::GkEventLogging>("GekkoFyre::System::Events::Logging::GkEventLogging");
    qRegisterMetaType<GekkoFyre::AmateurRadio::GkConnType>("GekkoFyre::AmateurRadio::GkConnType");
    qRegisterMetaType<GekkoFyre::AmateurRadio::DigitalModes>("GekkoFyre::AmateurRadio::DigitalModes");
    qRegisterMetaType<GekkoFyre::AmateurRadio::IARURegions>("GekkoFyre::AmateurRadio::IARURegions");
    qRegisterMetaType<GekkoFyre::Spectrograph::GkGraphType>("GekkoFyre::Spectrograph::GkGraphType");
    qRegisterMetaType<GekkoFyre::AmateurRadio::GkFreqs>("GekkoFyre::AmateurRadio::GkFreqs");
    qRegisterMetaType<RIG>("RIG");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<rig_model_t>("rig_model_t");
    qRegisterMetaType<PaHostApiTypeId>("PaHostApiTypeId");
    qRegisterMetaType<std::vector<qint16>>("std::vector<qint16>");

    sys::error_code ec;
    fs::path slash = "/";
    native_slash = slash.make_preferred().native();

    const fs::path dir_to_append = fs::path(Filesystem::defaultDirAppend + native_slash.string() + Filesystem::fileName);
    const fs::path swrld_save_path = fileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
                                                              true, QString::fromStdString(dir_to_append.string())).toStdString(); // Path to save final database towards

    try {
        // Print out the current date
        std::cout << QDate::currentDate().toString().toStdString() << std::endl;

        this->window()->showMaximized();; // Maximize the window!

        this->setWindowIcon(QIcon(":/resources/contrib/images/vector/purchased/2020-03/iconfinder_293_Frequency_News_Radio_5711690.svg"));
        ui->actionPlay->setIcon(QIcon(":/resources/contrib/images/vector/Kameleon/Record-Player.svg"));
        ui->actionSave_Decoded_Ab->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/clipboard-flat.svg"));
        ui->actionPrint->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/printer-rounded.svg"));
        ui->actionSettings->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/settings-flat.svg"));
        ui->actionView_Spectrogram_Controller->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_Graph-Decrease_378375.svg"));

        ui->action_Open->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_archive_226655.svg"));
        ui->action_Print->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/printer-rounded.svg"));
        ui->actionE_xit->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_turn_off_on_power_181492.svg"));
        ui->action_Settings->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/settings-flat.svg"));
        ui->actionCheck_for_Updates->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_chemistry_226643.svg"));

        //
        // Create a status bar at the bottom of the window with a default message
        // https://doc.qt.io/qt-5/qstatusbar.html
        //
        createStatusBar();

        //
        // Configure the volume meter!
        //
        gkVuMeter = new GkVuMeter(ui->frame_spect_vu_meter);
        ui->verticalLayout_8->addWidget(gkVuMeter);

        //
        // Initialize the default logic state on all applicable QPushButtons within QMainWindow.
        //
        btn_bridge_input_audio = false;
        btn_radio_rx = false;
        btn_radio_tx = false;
        btn_radio_tx_halt = false;
        btn_radio_tune = false;
        btn_radio_monitor = false;

        rx_vol_control_selected = true; // By default it is ticked!
        global_rx_audio_volume = 0.0;
        global_tx_audio_volume = 0.0;

        //
        // SSTV related
        //
        sstv_tx_image_idx = 0; // Value is otherwise '1' if an image is loaded at startup!
        sstv_rx_image_idx = 0; // Value is otherwise '1' if an image is loaded at startup!
        sstv_rx_saved_image_idx = 0; // Value is otherwise '1' if an image is loaded at startup!

        //
        // Initialize Hamlib!
        //
        rig_load_all_backends();
        rig_list_foreach(parseRigCapabilities, nullptr);

        // Create class pointers
        fileIo = new GekkoFyre::FileIo(this);
        gkStringFuncs = new GekkoFyre::StringFuncs(this);

        //
        // Settings database-related logic
        //
        QString init_settings_db_loc = fileIo->read_initial_settings(init_cfg::DbLoc);
        QString init_settings_db_name = fileIo->read_initial_settings(init_cfg::DbName);

        // Create path to file-database
        try {
            if (!init_settings_db_loc.isEmpty() && !init_settings_db_name.isEmpty()) {
                // A path has already been created, or should've been, and is stored in the QSetting's
                std::string tmp_db_loc = init_settings_db_loc.toStdString();
                std::string tmp_db_name = init_settings_db_name.toStdString();
                fs::path new_path = fs::path(tmp_db_loc + native_slash.string() + tmp_db_name); // Path to save final database towards
                if (fs::exists(new_path, ec) && fs::is_directory(tmp_db_loc, ec)) { // Verify parent directory exists and is a directory itself
                    // Directory presently exists
                    save_db_path = new_path;
                } else {
                    if (fs::exists(swrld_save_path, ec)) {
                        save_db_path = swrld_save_path;
                    } else {
                        //
                        // Directory does not exist despite the fact that it should, due to being created in a previous step!
                        //
                        throw std::invalid_argument(tr("A required directory does not exist; please report this to the developer!").toStdString());
                    }
                }
            }
        } catch (const sys::system_error &e) {
            ec = e.code();
            std::cerr << tr("An issue has been encountered whilst initializing Google LevelDB!\n\n%1")
                    .arg(QString::fromStdString(ec.message())).toStdString() << std::endl; // Because the QMessageBox doesn't always reliably display!
            QMessageBox::warning(this, tr("Error!"), tr("An issue has been encountered whilst initializing Google LevelDB!\n\n%1")
                                 .arg(QString::fromStdString(ec.message())), QMessageBox::Ok);
        }

        // Some basic optimizations; this is the easiest way to get Google LevelDB to perform well
        leveldb::Options options;
        leveldb::Status status;
        options.create_if_missing = true;
        options.compression = leveldb::CompressionType::kSnappyCompression;
        options.paranoid_checks = true;

        try {
            if (!save_db_path.empty()) {
                status = leveldb::DB::Open(options, save_db_path.string(), &db);
                GkDb = new GekkoFyre::GkLevelDb(db, fileIo, this);

                bool enableSentry = false;
                bool askSentry = GkDb->read_sentry_settings(GkSentry::AskedDialog);
                if (!askSentry) {
                    QMessageBox optInMsgBox;
                    optInMsgBox.setWindowTitle(tr("Help improve Small World!"));
                    optInMsgBox.setText(tr("With your voluntary consent, Small World Deluxe can report anonymous information that helps developers improve this "
                                           "application. This includes things like your screen resolution, along with any crashes you wish to submit."));
                    optInMsgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
                    optInMsgBox.setDefaultButton(QMessageBox::Yes);
                    optInMsgBox.setIcon(QMessageBox::Icon::Question);
                    int ret = optInMsgBox.exec();

                    switch (ret) {
                    case QMessageBox::Yes:
                        enableSentry = true;
                        GkDb->write_sentry_settings(true, GkSentry::GivenConsent);
                        break;
                    case QMessageBox::No:
                        enableSentry = false;
                        GkDb->write_sentry_settings(false, GkSentry::GivenConsent);
                        break;
                    default:
                        enableSentry = false;
                        GkDb->write_sentry_settings(false, GkSentry::GivenConsent);
                        break;
                    }

                    // We have now asked the user this question at least once!
                    GkDb->write_sentry_settings(true, GkSentry::AskedDialog);
                } else {
                    enableSentry = GkDb->read_sentry_settings(GkSentry::GivenConsent);
                }

                if (enableSentry) {
                    //
                    // Initialize Sentry!
                    // https://blog.sentry.io/2019/09/26/fixing-native-apps-with-sentry
                    // https://docs.sentry.io/platforms/native/
                    //
                    sen_opt = sentry_options_new();

                    const QString curr_path = QDir::currentPath();
                    const fs::path crashpad_handler_windows = fs::path(curr_path.toStdString() + native_slash.string() + Filesystem::gk_crashpad_handler_win);
                    const fs::path crashpad_handler_linux = fs::path(curr_path.toStdString() + native_slash.string() + Filesystem::gk_crashpad_handler_linux);
                    fs::path handler_to_use;

                    if (fs::exists(crashpad_handler_linux, ec)) {
                        // The handler exists as if we're under a Linux system!
                        handler_to_use = crashpad_handler_linux;
                    } else if (fs::exists(crashpad_handler_windows, ec)) {
                        // The handler exists as if we're under a Microsoft Windows system!
                        handler_to_use = crashpad_handler_windows;
                    } else {
                        throw std::invalid_argument(tr("Unable to find the Crashpad handler for your installation of Small World Deluxe!").toStdString());
                    }

                    // The handler is a Crashpad-specific background process
                    sentry_options_set_handler_path(sen_opt, handler_to_use.c_str());

                    const fs::path sentry_crash_dir = fs::path(Filesystem::defaultDirAppend + native_slash.string() + Filesystem::gk_sentry_dump_dir);
                    const fs::path gk_minidump = fileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
                                                                          true, QString::fromStdString(sentry_crash_dir.string())).toStdString();

                    const qint64 sentry_curr_epoch = QDateTime::currentMSecsSinceEpoch();
                    const fs::path gk_sentry_attachments = std::string(gk_minidump.string() + native_slash.string() + QString::number(sentry_curr_epoch).toStdString()
                                                                       + Filesystem::gk_sentry_dump_file_ext);

                    if (!fs::exists(gk_minidump, ec)) {
                        fs::create_directories(gk_minidump, ec);
                    }

                    // This is where Minidumps and attachments live before upload
                    sentry_options_set_database_path(sen_opt, gk_minidump.c_str());
                    sentry_options_add_attachment(sen_opt, gk_sentry_attachments.c_str());

                    // Miscellaneous settings pertaining to Sentry
                    sentry_options_set_auto_session_tracking(sen_opt, true);
                    sentry_options_set_symbolize_stacktraces(sen_opt, true);

                    // Release information
                    sentry_options_set_environment(sen_opt, General::gk_sentry_env);
                    sentry_options_set_release(sen_opt, General::appVersion);

                    // Server and URI details!
                    sentry_options_set_dsn(sen_opt, General::gk_sentry_uri);

                    // Initialize the SDK and start the Crashpad handler
                    sentry_init(sen_opt);

                    // Initialize a Unique ID for the given user on the local machine, which is much more anonymous and sanitized than
                    // dealing with IP Addresses!
                    GkDb->create_unique_id();
                }

                //
                // Load some of the primary abilities to work with the Hamlib libraries!
                //
                QObject::connect(this, SIGNAL(changeConnPort(const QString &, const GekkoFyre::AmateurRadio::GkConnMethod &)),
                                 this, SLOT(procRigPort(const QString &, const GekkoFyre::AmateurRadio::GkConnMethod &)));
                QObject::connect(this, SIGNAL(recvRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(gatherRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
                QObject::connect(this, SIGNAL(addRigInUse(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(addRigToMemory(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
                QObject::connect(this, SIGNAL(disconnectRigInUse(std::shared_ptr<Rig>, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(disconnectRigInMemory(std::shared_ptr<Rig>, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));

                //
                // Initialize the Events logger
                //
                QPointer<GkEventLoggerTableViewModel> gkEventLoggerModel = new GkEventLoggerTableViewModel(GkDb, this);
                ui->tableView_maingui_logs->setModel(gkEventLoggerModel);
                ui->tableView_maingui_logs->horizontalHeader()->setVisible(true);
                ui->tableView_maingui_logs->horizontalHeader()->setStretchLastSection(true);
                ui->tableView_maingui_logs->show();

                gkEventLogger = new GkEventLogger(this);
                QObject::connect(gkEventLogger, SIGNAL(sendEvent(const GekkoFyre::System::Events::Logging::GkEventLogging &)),
                                 gkEventLoggerModel, SLOT(insertData(const GekkoFyre::System::Events::Logging::GkEventLogging &)));
                QObject::connect(gkEventLogger, SIGNAL(removeEvent(const GekkoFyre::System::Events::Logging::GkEventLogging &)),
                                 gkEventLoggerModel, SLOT(removeData(const GekkoFyre::System::Events::Logging::GkEventLogging &)));

                gkEventLogger->publishEvent(tr("Events log initiated."), GkSeverity::Info);

                // Initialize the Radio Database pointer!
                if (gkRadioPtr.get() == nullptr) {
                    gkRadioPtr = std::make_shared<GkRadio>();
                }

                //
                // Initialize the all-important `GkRadioPtr`!
                //
                status_com_ports = gkRadioLibs->filter_com_ports(gkRadioLibs->status_com_ports());
                gkUsbPortPtr = std::make_shared<GkUsbPort>();
                gkRadioPtr = readRadioSettings();
                emit addRigInUse(gkRadioPtr->rig_model, gkRadioPtr);

                // Initialize the other radio libraries!
                gkRadioLibs = new GekkoFyre::RadioLibs(fileIo, gkStringFuncs, GkDb, gkRadioPtr, gkEventLogger, this);

                //
                // Setup the SIGNALS & SLOTS for `gkRadioLibs`...
                //
                QObject::connect(gkRadioLibs, SIGNAL(disconnectRigInUse(std::shared_ptr<Rig>, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(disconnectRigInMemory(std::shared_ptr<Rig>, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
            } else {
                throw std::runtime_error(tr("Unable to find settings database; we've lost its location!").toStdString());
            }
        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Error!"), tr("%1\n\nAborting...").arg(e.what()), QMessageBox::Ok);
            QApplication::exit(EXIT_FAILURE);
        }

        //
        // Setup the CLI parser and its settings!
        // https://doc.qt.io/qt-5/qcommandlineparser.html
        //
        gkCliParser = std::make_shared<QCommandLineParser>();
        gkCli = std::make_shared<GekkoFyre::GkCli>(gkCliParser, fileIo, GkDb, gkRadioLibs, this);

        std::unique_ptr<QString> error_msg = std::make_unique<QString>("");
        gkCli->parseCommandLine(error_msg.get());

        //
        // Load settings for QMainWindow
        //
        int window_width = GkDb->read_mainwindow_settings(general_mainwindow_cfg::WindowHSize).toInt();
        int window_height = GkDb->read_mainwindow_settings(general_mainwindow_cfg::WindowVSize).toInt();
        bool window_minimized = GkDb->read_mainwindow_settings(general_mainwindow_cfg::WindowMaximized).toInt();

        // Set the x-axis size of QMainWindow
        if (window_width >= MIN_MAIN_WINDOW_WIDTH) {
            this->window()->size().setWidth(window_width);
        }

        // Set the y-axis size of QMainWindow
        if (window_height >= MIN_MAIN_WINDOW_HEIGHT) {
            this->window()->size().setHeight(window_height);
        }

        // Whether to maximize the QMainWindow or not
        if (window_minimized == 0) {
            this->window()->showMaximized();
        } else {
            this->window()->showNormal();
        }

        //
        // Collect settings from QMainWindow, among other miscellaneous settings, upon termination of Small World Deluxe!
        //
        QObject::connect(this, SIGNAL(gkExitApp()), this, SLOT(uponExit()));

        //
        // Initialize our own PortAudio libraries and associated buffers!
        //
        PaError err;
        err = Pa_Initialize();
        if (err != paNoError) {
            throw std::runtime_error(tr("An error was encountered whilst initializing PortAudio!").toStdString());
        }

        autoSys.initialize();
        gkPortAudioInit = new portaudio::System(portaudio::System::instance());

        gkAudioDevices = std::make_shared<GekkoFyre::AudioDevices>(GkDb, fileIo, gkFreqList, gkStringFuncs, this);
        auto pref_audio_devices = gkAudioDevices->initPortAudio(gkPortAudioInit);

        if (!pref_audio_devices.empty()) {
            for (const auto &device: pref_audio_devices) {
                // Now filter out what is the input and output device selectively!
                if (device.is_output_dev) {
                    // Output device
                    pref_output_device = device;
                    pref_output_device.is_dev_active = false;
                } else {
                    // Input device
                    pref_input_device = device;
                    pref_input_device.is_dev_active = false;
                }
            }
        }

        //
        // Initialize any FFT libraries/resources
        //
        gkFFT = std::make_unique<GkFFT>();

        //
        // Initialize the Waterfall / Spectrograph
        //
        gkSpectroGui = new GekkoFyre::SpectroGui(gkStringFuncs, gkEventLogger, true, true, this);
        ui->horizontalLayout_12->addWidget(gkSpectroGui);
        gkSpectroGui->setEnabled(true);

        //
        // Sound & Audio Devices
        //
        QObject::connect(this, SIGNAL(stopRecording()), this, SLOT(stopRecordingInput()));
        QObject::connect(this, SIGNAL(startRecording()), this, SLOT(startRecordingInput()));
        QObject::connect(this, SIGNAL(stopTxAudio()), this, SLOT(stopTransmitOutput()));
        QObject::connect(this, SIGNAL(startTxAudio()), this, SLOT(startTransmitOutput()));
        QObject::connect(this, SIGNAL(refreshVuDisplay(const qreal &, const qreal &, const int &)),
                         gkVuMeter, SLOT(levelChanged(const qreal &, const qreal &, const int &)));

        //
        // QMainWindow widgets
        //
        prefillAmateurBands();

        info_timer = new QTimer(this);
        connect(info_timer, SIGNAL(timeout()), this, SLOT(infoBar()));
        info_timer->start(1000);

        QObject::connect(this, SIGNAL(refreshSpectrograph(const qint64 &, const qint64 &)),
                         gkSpectroGui, SLOT(refreshDateTime(const qint64 &, const qint64 &)));

        if (!pref_audio_devices.empty()) {
            input_audio_buf = std::make_shared<GekkoFyre::PaAudioBuf<qint16>>(AUDIO_FRAMES_PER_BUFFER, pref_output_device, pref_input_device);
        }

        //
        // QPrinter-specific options!
        // https://doc.qt.io/qt-5/qprinter.html
        // https://doc.qt.io/qt-5/qtprintsupport-index.html
        //
        QDateTime print_curr_date = QDateTime::currentDateTime();
        fs::path default_filename = fs::path(tr("(%1) Small World Deluxe").arg(print_curr_date.toString("dd-MM-yyyy")).toStdString());
        fs::path default_path = fs::path(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toStdString() + native_slash.string() + default_filename.string());

        // printer = std::make_shared<QPrinter>(QPrinter::HighResolution);
        // printer->setOutputFormat(QPrinter::NativeFormat);
        // printer->setOutputFileName(QString::fromStdString(default_path.string())); // TODO: Clean up this abomination of multiple std::string() <-> QString() conversions!

        if (!pref_audio_devices.empty()) {
            //
            // Setup the audio encoding/decoding libraries!
            //
            output_audio_buf = std::make_shared<GekkoFyre::PaAudioBuf<qint16>>(AUDIO_FRAMES_PER_BUFFER, pref_output_device, pref_input_device);

            gkAudioEncoding = new GkAudioEncoding(fileIo, input_audio_buf, GkDb, gkSpectroGui,
                                                  gkStringFuncs, pref_input_device, gkEventLogger, this);
            gkAudioDecoding = new GkAudioDecoding(fileIo, GkDb, gkStringFuncs, pref_output_device,
                                                  gkEventLogger, this);

            //
            // Audio encoding signals and slots
            //
            gkAudioPlayDlg = new GkAudioPlayDialog(GkDb, gkAudioDecoding, gkAudioDevices, fileIo, this);
            // QObject::connect(gkAudioPlayDlg, SIGNAL(beginRecording(const bool &)), gkAudioEncoding, SLOT(startRecording(const bool &)));
        }

        //
        // Initiate at startup to set any default values and/or signals/slots!
        //
        on_checkBox_rx_tx_vol_toggle_stateChanged(rx_vol_control_selected);
        QObject::connect(this, SIGNAL(changeVolume(const float &)), this, SLOT(updateVolume(const float &)));

        //
        // Set a default value for the Volume Slider and its QLabel
        //
        const int vol_slider_max_val = ui->verticalSlider_vol_control->maximum();
        const float real_vol_val = (static_cast<float>(ui->verticalSlider_vol_control->value()) / vol_slider_max_val);
        updateVolumeSliderLabel(real_vol_val);

        if (input_audio_buf.get() != nullptr) {
            if (pref_input_device.dev_number > 0 && pref_input_device.dev_input_channel_count > 0) {
                on_pushButton_radio_receive_clicked();
            }
        }

        //
        // Initialize the list of frequencies that Small World Deluxe needs to communicate with other users
        // throughout the globe/world!
        //
        gkFreqList = new GkFrequencies(GkDb, this);
        QObject::connect(gkFreqList, SIGNAL(addFreq(const GekkoFyre::AmateurRadio::GkFreqs &)),
                         this, SLOT(addFreqToDb(const GekkoFyre::AmateurRadio::GkFreqs &)));
        QObject::connect(gkFreqList, SIGNAL(removeFreq(const GekkoFyre::AmateurRadio::GkFreqs &)),
                         this, SLOT(removeFreqFromDb(const GekkoFyre::AmateurRadio::GkFreqs &)));

        gkFreqList->publishFreqList();

        //
        // Setup the SSTV sections in QMainWindow!
        //
        label_sstv_tx_image = new GkDisplayImage(Gui::sstvWindow::txSendImage, gkEventLogger, ui->scrollArea_sstv_tx_image);
        label_sstv_rx_live_image = new GkDisplayImage(Gui::sstvWindow::rxLiveImage, gkEventLogger, ui->scrollArea_sstv_rx_live_image);
        label_sstv_rx_saved_image = new GkDisplayImage(Gui::sstvWindow::rxSavedImage, gkEventLogger, ui->scrollArea_sstv_rx_saved_image);
        ui->verticalLayout_21->addWidget(label_sstv_tx_image);
        ui->verticalLayout_19->addWidget(label_sstv_rx_live_image);
        ui->verticalLayout_20->addWidget(label_sstv_rx_saved_image);
        label_sstv_tx_image->setText("");
        label_sstv_rx_live_image->setText("");
        label_sstv_rx_saved_image->setText("");

        label_sstv_rx_live_image->setPixmap(QPixmap(":/resources/contrib/images/raster/unknown-author/sstv/sstv-image-placeholder.jpg"));

        //
        // This connects `widget_mesg_outgoing` to any transmission protocols, such as Codec2!
        //
        QPointer<GkPlainTextSubmit> widget_mesg_outgoing = new GkPlainTextSubmit(ui->frame_mesg_log);
        ui->verticalLayout_3->addWidget(widget_mesg_outgoing);
        widget_mesg_outgoing->setTabChangesFocus(true);
        widget_mesg_outgoing->setPlaceholderText(tr("Enter your outgoing messages here..."));
        QObject::connect(widget_mesg_outgoing, SIGNAL(execFuncAfterEvent(const QString &)),
                         this, SLOT(msgOutgoingProcess(const QString &)));

        QPointer<GkComboBoxSubmit> widget_change_freq = new GkComboBoxSubmit(ui->frame_spect_buttons_top);
        ui->horizontalLayout_10->addWidget(widget_change_freq);
        widget_change_freq->setEditable(true);
        widget_change_freq->setToolTip(tr("Enter or fine-tune the frequency that you would like to transmit/receive with!"));
        QObject::connect(widget_change_freq, SIGNAL(execFuncAfterEvent(const quint64 &)),
                         this, SLOT(tuneActiveFreq(const quint64 &)));

        gkModem = new GekkoFyre::GkModem(gkAudioDevices, GkDb, gkEventLogger, gkStringFuncs, this);
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("An error was encountered upon launch!\n\n%1").arg(e.what()), QMessageBox::Ok);
        QApplication::exit(EXIT_FAILURE);
    }
}

/**
 * @brief MainWindow::~MainWindow
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
MainWindow::~MainWindow()
{
    emit stopRecording(); // TODO: Fix the SEGFAULT that occurs with this!
    emit disconnectRigInUse(gkRadioPtr->gkRig, gkRadioPtr);

    if (vu_meter_thread.joinable()) {
        vu_meter_thread.join();
    }

    if (spectro_timing_thread.joinable()) {
        spectro_timing_thread.join();
    }

    // Free the pointer for the Google LevelDB library!
    delete db;

    // Free the pointer for the PortAudio library!
    autoSys.terminate();
    gkPortAudioInit->terminate();

    // Clear any memory used by Sentry & Crashpad before making sure the process itself terminates!
    sentry_shutdown();

    delete ui;
}

/**
 * @brief MainWindow::on_actionE_xit_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionE_xit_triggered()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Terminate"));
    msgBox.setText(tr("Are you sure you wish to terminate Small World Deluxe?"));
    msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Icon::Question);
    int ret = msgBox.exec();

    switch (ret) {
    case QMessageBox::Ok:
        QApplication::exit(EXIT_SUCCESS);
        break;
    case QMessageBox::Cancel:
        return;
    default:
        return;
    }

    return;
}

/**
 * @brief MainWindow::on_action_Open_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_Open_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::changePushButtonColor
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param push_button The QPushButton to be modified with the new QStyleSheet.
 * @param green_result Whether to make the QPushButton in question Green or Red.
 * @param color_blind_mode Not yet implemented!
 */
void MainWindow::changePushButtonColor(const QPointer<QPushButton> &push_button, const bool &green_result,
                                       const bool &color_blind_mode)
{
    Q_UNUSED(color_blind_mode);

    if (green_result) {
        // Change QPushButton to a shade of darkish 'Green'
        push_button->setStyleSheet("QPushButton{\nbackground-color: #B80000; border: 1px solid black;\nborder-radius: 5px;\nborder-width: 1px;\npadding: 6px;\nfont: bold;\ncolor: white;\n}");
    } else {
        // Change QPushButton to a shade of darkish 'Red'
        push_button->setStyleSheet("QPushButton{\nbackground-color: #3C8C2F; border: 1px solid black;\nborder-radius: 5px;\nborder-width: 1px;\npadding: 6px;\nfont: bold;\ncolor: white;\n}");
    }

    // TODO: Implement color-blind mode!
    return;
}

/**
 * @brief MainWindow::getAmateurBands Gathers all of the requisite amateur radio bands that
 * apply to Small World Deluxe and outputs them as a QStringList().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The amateur radio bands that apply to the workings of Small World Deluxe, as a QStringList().
 */
QStringList MainWindow::getAmateurBands()
{
    try {
        QStringList bands;
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND160));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND80));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND60));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND40));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND30));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND20));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND17));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND15));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND12));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND10));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND6));
        bands.push_back(GkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND2));

        return bands;
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return QStringList();
}

/**
 * @brief MainWindow::prefillAmateurBands fills out any relevant QComboBox'es with the requisite amateur
 * radio bands that apply to Small World Deluxe.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return Whether the operation was a success or not.
 */
bool MainWindow::prefillAmateurBands()
{
    auto amateur_bands = getAmateurBands();
    if (!amateur_bands.isEmpty()) {
        ui->comboBox_select_frequency->addItems(amateur_bands);

        return true;
    } else {
        return false;
    }

    return false;
}

/**
 * @brief MainWindow::launchSettingsWin launches the Settings dialog! This is simply a helper function.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::launchSettingsWin()
{
    QPointer<GkFreqTableViewModel> gkFreqTableModel = new GkFreqTableViewModel(GkDb, this);
    QObject::connect(gkFreqTableModel, SIGNAL(addFreq(const GekkoFyre::AmateurRadio::GkFreqs &)),
                     gkFreqList, SIGNAL(addFreq(const GekkoFyre::AmateurRadio::GkFreqs &)));
    QObject::connect(gkFreqTableModel, SIGNAL(removeFreq(const GekkoFyre::AmateurRadio::GkFreqs &)),
                     gkFreqList, SIGNAL(removeFreq(const GekkoFyre::AmateurRadio::GkFreqs &)));

    QPointer<DialogSettings> dlg_settings = new DialogSettings(GkDb, fileIo, gkAudioDevices, gkRadioLibs, gkStringFuncs,
                                                               gkPortAudioInit, gkRadioPtr, status_com_ports,
                                                               gkFreqList, gkFreqTableModel, this);
    dlg_settings->setWindowFlags(Qt::Window);
    dlg_settings->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(dlg_settings, SIGNAL(destroyed(QObject*)), this, SLOT(show()));

    QObject::connect(dlg_settings, SIGNAL(changeConnPort(const QString &, const GekkoFyre::AmateurRadio::GkConnMethod &)),
                     this, SIGNAL(changeConnPort(const QString &, const GekkoFyre::AmateurRadio::GkConnMethod &)));
    QObject::connect(dlg_settings, SIGNAL(recvRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                     this, SLOT(gatherRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
    QObject::connect(dlg_settings, SIGNAL(addRigInUse(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                     this, SLOT(addRigToMemory(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));

    dlg_settings->show();

    return;
}

/**
 * @brief MainWindow::radioInitStart initializes the Hamlib library and any associated libraries/functions!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
bool MainWindow::radioInitStart()
{
    try {
        QString model = GkDb->read_rig_settings(radio_cfg::RigModel);
        if (!model.isEmpty() || !model.isNull()) {
            gkRadioPtr->rig_model = model.toInt();
        } else {
            gkRadioPtr->rig_model = 1;
        }

        if (gkRadioPtr->rig_model <= 0) {
            gkRadioPtr->rig_model = 1;
        }

        // TODO: Very important! Expand this section!
        QString com_baud_rate = GkDb->read_rig_settings_comms(radio_cfg::ComBaudRate);
        if (!com_baud_rate.isEmpty() || !com_baud_rate.isNull()) {
            gkRadioPtr->dev_baud_rate = gkRadioLibs->convertBaudRateToEnum(com_baud_rate.toInt());
        } else {
            gkRadioPtr->dev_baud_rate = AmateurRadio::com_baud_rates::BAUD9600;
        }

        #ifdef GFYRE_SWORLD_DBG_VERBOSITY
        gkRadioPtr->verbosity = RIG_DEBUG_VERBOSE;
        #else
        gkRadioPtr->verbosity = RIG_DEBUG_BUG;
        #endif

        //
        // Initialize Hamlib!
        //
        gkRadioPtr->is_open = false;
        rig_thread = std::thread(&RadioLibs::gkInitRadioRig, gkRadioLibs, gkRadioPtr);
        rig_thread.detach();

        return true;
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return false;
}

/**
 * @brief MainWindow::readRadioSettings reads out the primary settings as they relate to the user's amateur
 * radio rig, usually once provided everything has been configured within the Setting's Dialog. It also
 * detects whether the user is connecting to their radio rig by means of RS232, USB, GPIO, etc. and therefore
 * applies the correct settings accordingly.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The primary struct for dealing with the user's amateur radio rig in question and any configuration
 * settings (if automatically detected or henceforth configured within the Setting's Dialog).
 */
std::shared_ptr<GkRadio> MainWindow::readRadioSettings()
{
    try {
        QString rigBrand = GkDb->read_rig_settings(radio_cfg::RigBrand);
        QString rigModel = GkDb->read_rig_settings(radio_cfg::RigModel);
        QString rigModelIndex = GkDb->read_rig_settings(radio_cfg::RigModelIndex);
        QString rigVers = GkDb->read_rig_settings(radio_cfg::RigVersion);
        QString comBaudRate = GkDb->read_rig_settings(radio_cfg::ComBaudRate);
        QString stopBits = GkDb->read_rig_settings(radio_cfg::StopBits);
        QString data_bits = GkDb->read_rig_settings(radio_cfg::DataBits);
        QString handshake = GkDb->read_rig_settings(radio_cfg::Handshake);
        QString force_ctrl_lines_dtr = GkDb->read_rig_settings(radio_cfg::ForceCtrlLinesDtr);
        QString force_ctrl_lines_rts = GkDb->read_rig_settings(radio_cfg::ForceCtrlLinesRts);
        QString ptt_method = GkDb->read_rig_settings(radio_cfg::PTTMethod);
        QString tx_audio_src = GkDb->read_rig_settings(radio_cfg::TXAudioSrc);
        QString ptt_mode = GkDb->read_rig_settings(radio_cfg::PTTMode);
        QString split_operation = GkDb->read_rig_settings(radio_cfg::SplitOperation);
        QString ptt_adv_cmd = GkDb->read_rig_settings(radio_cfg::PTTAdvCmd);

        std::shared_ptr<GkRadio> gk_radio_tmp = std::make_shared<GkRadio>();

        //
        // CAT Port
        //
        QString comDeviceCat = GkDb->read_rig_settings_comms(radio_cfg::ComDeviceCat);
        if (!comDeviceCat.isNull() && !comDeviceCat.isEmpty()) {
            // Verify that the port still exists!
            for (const auto &port: status_com_ports) {
                if (comDeviceCat == port.port_info.portName()) {
                    // The port does indeed continue to exist!
                    gk_radio_tmp->cat_conn_port = comDeviceCat;
                    gkEventLogger->publishEvent(tr("Successfully found a port for making a CAT connection with (%1)!").arg(comDeviceCat), GkSeverity::Info);
                }
            }
        } else {
            // No ports could be found!
            gkEventLogger->publishEvent(tr("Error with enumerating out ports for CAT connections. Please check that your settings are valid."), GkSeverity::Warning);
        }

        //
        // PTT Port
        //
        QString comDevicePtt = GkDb->read_rig_settings_comms(radio_cfg::ComDevicePtt);
        if (!comDevicePtt.isNull() && !comDevicePtt.isEmpty()) {
            // Verify that the port still exists!
            for (const auto &port: status_com_ports) {
                if (comDevicePtt == port.port_info.portName()) {
                    // The port does indeed continue to exist!
                    gk_radio_tmp->ptt_conn_port = comDevicePtt;
                    gkEventLogger->publishEvent(tr("Successfully found a port for making a PTT connection with (%1)!").arg(comDevicePtt), GkSeverity::Info);
                }
            }
        } else {
            // No ports could be found!
            gkEventLogger->publishEvent(tr("Error with enumerating out ports for PTT connections. Please check that your settings are valid."), GkSeverity::Warning);
        }

        if (!rigBrand.isNull() || !rigBrand.isEmpty()) { // The manufacturer!
            int conv_rig_brand = rigBrand.toInt();
            gk_radio_tmp->rig_brand = conv_rig_brand;
        }

        Q_UNUSED(rigModel);
        // if (!rigModel.isNull() || !rigModel.isEmpty()) {}

        if (!rigModelIndex.isNull() || !rigModelIndex.isEmpty()) { // The actual amateur radio rig itself!
            int conv_rig_model_idx = rigModelIndex.toInt();
            gk_radio_tmp->rig_model = conv_rig_model_idx;
        } else {
            gk_radio_tmp->rig_model = RIG_MODEL_DUMMY;
        }

        Q_UNUSED(rigVers);
        // if (!rigVers.isNull() || !rigVers.isEmpty()) {}

        if (!comBaudRate.isNull() || !comBaudRate.isEmpty()) {
            int conv_com_baud_rate = comBaudRate.toInt();
            gk_radio_tmp->port_details.parm.serial.rate = gkRadioLibs->convertBaudRateInt(gkRadioLibs->convertBaudRateToEnum(conv_com_baud_rate));
        } else {
            gk_radio_tmp->port_details.parm.serial.rate = 38400;
        }

        gkEventLogger->publishEvent(tr("Using a serial baud rate of %1 bps.").arg(QString::number(gk_radio_tmp->port_details.parm.serial.rate)), GkSeverity::Info);

        if (!stopBits.isNull() || !stopBits.isEmpty()) {
            int conv_serial_stop_bits = stopBits.toInt();
            gk_radio_tmp->port_details.parm.serial.stop_bits = conv_serial_stop_bits;
        } else {
            gk_radio_tmp->port_details.parm.serial.stop_bits = 0;
        }

        if (!data_bits.isNull() || !data_bits.isEmpty()) {
            int conv_serial_data_bits = data_bits.toInt();
            gk_radio_tmp->port_details.parm.serial.data_bits = conv_serial_data_bits;
        } else {
            gk_radio_tmp->port_details.parm.serial.data_bits = 0;
        }

        if (!handshake.isNull() || !handshake.isEmpty()) {
            int conv_serial_handshake = handshake.toInt();
            switch (conv_serial_handshake) {
            case 0:
                // Default
                gk_radio_tmp->port_details.parm.serial.handshake = serial_handshake_e::RIG_HANDSHAKE_NONE;
                break;
            case 1:
                // None
                gk_radio_tmp->port_details.parm.serial.handshake = serial_handshake_e::RIG_HANDSHAKE_NONE;
                break;
            case 2:
                // XON / XOFF
                gk_radio_tmp->port_details.parm.serial.handshake = serial_handshake_e::RIG_HANDSHAKE_XONXOFF;
                break;
            case 3:
                // Hardware
                gk_radio_tmp->port_details.parm.serial.handshake = serial_handshake_e::RIG_HANDSHAKE_HARDWARE;
                break;
            default:
                // Nothing
                gk_radio_tmp->port_details.parm.serial.handshake = serial_handshake_e::RIG_HANDSHAKE_NONE;
                break;
            }
        } else {
            gk_radio_tmp->port_details.parm.serial.handshake = serial_handshake_e::RIG_HANDSHAKE_NONE;
        }

        if (!force_ctrl_lines_dtr.isNull() || !force_ctrl_lines_dtr.isEmpty()) {
            int conv_force_ctrl_lines_dtr = force_ctrl_lines_dtr.toInt();
            switch (conv_force_ctrl_lines_dtr) {
            case 0:
                // High
                gk_radio_tmp->port_details.parm.serial.dtr_state = serial_control_state_e::RIG_SIGNAL_ON;
                break;
            case 1:
                // Low
                gk_radio_tmp->port_details.parm.serial.dtr_state = serial_control_state_e::RIG_SIGNAL_OFF;
                break;
            default:
                // Nothing
                gk_radio_tmp->port_details.parm.serial.dtr_state = serial_control_state_e::RIG_SIGNAL_UNSET;
                break;
            }
        } else {
            gk_radio_tmp->port_details.parm.serial.dtr_state = serial_control_state_e::RIG_SIGNAL_UNSET;
        }

        if (!force_ctrl_lines_rts.isNull() || !force_ctrl_lines_rts.isEmpty()) {
            int conv_force_ctrl_lines_rts = force_ctrl_lines_rts.toInt();
            switch (conv_force_ctrl_lines_rts) {
            case 0:
                // High
                gk_radio_tmp->port_details.parm.serial.rts_state = serial_control_state_e::RIG_SIGNAL_ON;
                break;
            case 1:
                // Low
                gk_radio_tmp->port_details.parm.serial.rts_state = serial_control_state_e::RIG_SIGNAL_OFF;
                break;
            default:
                // Nothing
                gk_radio_tmp->port_details.parm.serial.rts_state = serial_control_state_e::RIG_SIGNAL_UNSET;
                break;
            }
        } else {
            gk_radio_tmp->port_details.parm.serial.rts_state = serial_control_state_e::RIG_SIGNAL_UNSET;
        }

        if (!ptt_method.isNull() || !ptt_method.isEmpty()) {
            int conv_ptt_method = ptt_method.toInt();
            switch (conv_ptt_method) {
            case 0:
                // VOX
                gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_RIG_MICDATA; // Legacy PTT (CAT PTT), supports RIG_PTT_ON_MIC/RIG_PTT_ON_DATA
                break;
            case 1:
                // DTR
                gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_SERIAL_DTR; // PTT control through serial DTR signal
                break;
            case 2:
                // CAT
                gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_RIG; // Legacy PTT (CAT PTT)
                break;
            case 3:
                // RTS
                gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_SERIAL_RTS; // PTT control through serial RTS signal
                break;
            default:
                // Nothing
                gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_NONE; // No PTT available
            }
        } else {
            gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_NONE; // Default option
        }

        if (!tx_audio_src.isNull() || !tx_audio_src.isEmpty()) {
            int conv_tx_audio_src = tx_audio_src.toInt();
            switch (conv_tx_audio_src) {
            case 0:
                // Rear / Data
                gk_radio_tmp->ptt_status = ptt_t::RIG_PTT_ON_DATA;
                break;
            case 1:
                // Front / Mic
                gk_radio_tmp->ptt_status = ptt_t::RIG_PTT_ON_MIC;
                break;
            default:
                // Nothing
                gk_radio_tmp->ptt_status = ptt_t::RIG_PTT_OFF; // TODO: Configure this so that PTT is enabled or not, as configured by the user!
                break;
            }
        } else {
            gk_radio_tmp->ptt_status = ptt_t::RIG_PTT_OFF; // Default option
        }

        if (!ptt_mode.isNull() || !ptt_mode.isEmpty()) {
            int conv_ptt_mode = ptt_mode.toInt();
            switch (conv_ptt_mode) {
            case 0:
                // None
                gk_radio_tmp->mode = RIG_MODE_NONE;
                break;
            case 1:
                // USB
                gk_radio_tmp->mode = RIG_MODE_USB;
                break;
            case 2:
                // Data / PKT
                gk_radio_tmp->mode = RIG_MODE_PKTUSB;
                break;
            default:
                // Nothing
                gk_radio_tmp->mode = RIG_MODE_NONE;
                break;
            }
        } else {
            // Default option
            gk_radio_tmp->mode = RIG_MODE_NONE;
        }

        if (!split_operation.isNull() || !split_operation.isEmpty()) {
            int conv_split_operation = split_operation.toInt();
            switch (conv_split_operation) {
            case 0:
                // None
                gk_radio_tmp->split_mode = split_t::RIG_SPLIT_OFF;
                break;
            case 1:
                // Rig
                gk_radio_tmp->split_mode = split_t::RIG_SPLIT_ON;
                break;
            case 2:
                // Fake it
                gk_radio_tmp->split_mode = split_t::RIG_SPLIT_OFF;
                break;
            default:
                // Nothing
                gk_radio_tmp->split_mode = split_t::RIG_SPLIT_OFF;
                break;
            }
        } else {
            // Default option
            gk_radio_tmp->split_mode = split_t::RIG_SPLIT_OFF;
        }

        if (!ptt_adv_cmd.isNull() || !ptt_adv_cmd.isEmpty()) {
            gk_radio_tmp->adv_cmd = ptt_adv_cmd.toStdString();
        } else {
            gk_radio_tmp->adv_cmd = "";
        }

        return gk_radio_tmp;
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return std::make_shared<GkRadio>();
}

/**
 * @brief MainWindow::parseRigCapabilities parses all the supported amateur radio rigs and other miscellaneous devices supported
 * by the Hamlib set of libraries and outputs them into a QMultiMap, which is searchable by manufacturer and device type.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param caps The given device's supported capabilities.
 * @param data This parameter's purpose is unknown but otherwise required.
 */
int MainWindow::parseRigCapabilities(const rig_caps *caps, void *data)
{
    Q_UNUSED(data);

    switch (caps->rig_type & RIG_TYPE_MASK) {
    case RIG_TYPE_TRANSCEIVER:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Transceiver));
        break;
    case RIG_TYPE_HANDHELD:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Handheld));
        break;
    case RIG_TYPE_MOBILE:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Mobile));
        break;
    case RIG_TYPE_RECEIVER:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Receiver));
        break;
    case RIG_TYPE_PCRECEIVER:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::PC_Receiver));
        break;
    case RIG_TYPE_SCANNER:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Scanner));
        break;
    case RIG_TYPE_TRUNKSCANNER:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::TrunkingScanner));
        break;
    case RIG_TYPE_COMPUTER:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Computer));
        break;
    case RIG_TYPE_OTHER:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Other));
        break;
    default:
        gkRadioModels.insert(caps->rig_model, std::make_tuple(caps, caps->mfg_name, GekkoFyre::AmateurRadio::rig_type::Unknown));
        break;
    }

    return 1; /* !=0, we want them all! */
}

QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, rig_type>> MainWindow::initRadioModelsVar()
{
    QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, rig_type>> mmap;
    mmap.insert(-1, std::make_tuple(nullptr, "", GekkoFyre::AmateurRadio::rig_type::Unknown));
    return mmap;
}

/**
 * @brief MainWindow::updateVolumeDisplayWidgets will update the volume widget within the QMainWindow of Small
 * World Deluxe and keep it updated as long as there is an incoming audio signal.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://doc.qt.io/qt-5.9/qtmultimedia-multimedia-spectrum-app-engine-cpp.html>
 */
void MainWindow::updateVolumeDisplayWidgets()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    if (inputAudioStream != nullptr && AUDIO_FRAMES_PER_BUFFER > 0) {
        while (inputAudioStream->isActive()) {
            //
            // Input audio stream is open and active!
            //
            auto audio_buf_tmp = std::make_shared<PaAudioBuf<qint16>>(*input_audio_buf);
            std::vector<qint16> recv_buf;
            while (audio_buf_tmp->size() > 0) {
                recv_buf.reserve(AUDIO_FRAMES_PER_BUFFER + 1);
                recv_buf.push_back(audio_buf_tmp->grab());
            }

            if (!recv_buf.empty()) {
                qreal peakLevel = 0;
                qreal sum = 0.0;

                const qint16 value = *reinterpret_cast<const qint16*>(recv_buf.data());
                const qreal amplitudeToReal = (static_cast<qreal>(value) / SHRT_MAX);
                peakLevel = qMax(peakLevel, amplitudeToReal);
                sum += amplitudeToReal * amplitudeToReal;

                const int numSamples = (AUDIO_FRAMES_PER_BUFFER);
                qreal rmsLevel = std::sqrt(sum / static_cast<qreal>(numSamples));

                emit refreshVuDisplay(rmsLevel, peakLevel, numSamples);
                recv_buf.clear();
                recv_buf.shrink_to_fit();
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::updateVolumeSliderLabel will update the volume QLabel UI widget, `label_vol_control_disp`.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param vol_level The actual slider value itself, derived from UI widget, `verticalSlider_vol_control`.
 */
void MainWindow::updateVolumeSliderLabel(const float &vol_level)
{
    const float vol_level_decibel = (20.0f * std::log10(vol_level));
    std::stringstream ss;
    ss << std::setprecision(3) << vol_level_decibel;
    ui->label_vol_control_disp->setText(tr("%1 dB").arg(QString::fromStdString(ss.str())));

    const float vol_multiplier = (1.0f * std::pow(10, (vol_level_decibel / 20.0f)));
    emit changeVolume(vol_multiplier);

    return;
}

/**
 * @brief MainWindow::fileOverloadWarning will warn the user about loading too many files (i.e. usually images in this case) into
 * memory and ask via QMessageBox if they really wish to proceed, despite being given all warnings about the dangers.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_count The total count of files that the user wishes to load into memory.
 * @param max_num_files The given maximum amount of files to be exceeded before displaying the QMessageBox warning.
 * @return Whether the user wishes to proceed (i.e. true) or not (i.e. false), despite being given all the warnings.
 */
bool MainWindow::fileOverloadWarning(const int &file_count, const int &max_num_files)
{
    if (file_count > max_num_files) {
        // Warn the user about the implications of loading too many files into memory all at once!
        QMessageBox msgBoxWarn;
        msgBoxWarn.setWindowTitle(tr("Warning!"));
        msgBoxWarn.setText(tr("You have selected an excessive number of files/images to load into memory (i.e. %1 files)! Proceeding with this "
                              "fact in mind can mean instability with Small World Deluxe.").arg(QString::number(file_count)));
        msgBoxWarn.setStandardButtons(QMessageBox::Cancel | QMessageBox::Abort | QMessageBox::Ok);
        msgBoxWarn.setDefaultButton(QMessageBox::Abort);
        msgBoxWarn.setIcon(QMessageBox::Icon::Warning);
        int ret = msgBoxWarn.exec();

        switch (ret) {
        case QMessageBox::Cancel:
            return false;
        case QMessageBox::Abort:
            return false;
        case QMessageBox::Ok:
            return true;
        default:
            return false;
        }
    }

    return false;
}

/**
 * @brief MainWindow::removeFreqFromDb will remove a frequency and its related values from the Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_to_remove
 */
void MainWindow::removeFreqFromDb(const GekkoFyre::AmateurRadio::GkFreqs &freq_to_remove)
{
    GkDb->remove_frequencies_db(freq_to_remove);

    return;
}

/**
 * @brief MainWindow::addFreqToDb will add a new frequency and any of its related values (such as digital mode
 * used, IARU region, etc.) to the Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_to_add
 */
void MainWindow::addFreqToDb(const GekkoFyre::AmateurRadio::GkFreqs &freq_to_add)
{
    bool freq_already_init = GkDb->isFreqAlreadyInit();
    if (!freq_already_init) {
        GkDb->write_frequencies_db(freq_to_add);
    }

    return;
}

/**
 * @brief MainWindow::tuneActiveFreq will tune the active frequency for the connected (transceiver) radio rig to the user's
 * desired value.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::tuneActiveFreq(const quint64 &freq_tune)
{
    Q_UNUSED(freq_tune);
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief MainWindow::createStatusBar creates a status bar at the bottom of the window.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://doc.qt.io/qt-5/qstatusbar.html>
 */
void MainWindow::createStatusBar(const QString &statusMsg)
{
    if (statusMsg.isEmpty()) {
        statusBar()->showMessage(tr("Ready..."), 5000);
    } else {
        statusBar()->showMessage(statusMsg, 2000);
    }
}

bool MainWindow::changeStatusBarMsg(const QString &statusMsg)
{
    if (statusBar()->currentMessage().isEmpty()) {
        if (!statusMsg.isEmpty() || !statusMsg.isNull()) {
            statusBar()->showMessage(statusMsg, 5000);
            return true;
        } else {
            return false;
        }
    } else {
        statusBar()->clearMessage();
        return changeStatusBarMsg(statusMsg);
    }

    return false;
}

/**
 * @brief MainWindow::steadyTimer is a simple duration timer that returns TRUE upon reaching a set duration that
 * is stated in milliseconds.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param milliseconds The duration of time you wish to measure.
 * @return Whether or not the duration of time has been met yet.
 * @note <http://rachelnertia.github.io/programming/2018/01/07/intro-to-std-chrono/>
 */
bool MainWindow::steadyTimer(const int &seconds)
{
    try {
        std::mutex steady_timer_mtx;
        std::lock_guard<std::mutex> lck_guard(steady_timer_mtx);
        std::chrono::steady_clock::time_point t_counter = std::chrono::steady_clock::now();
        std::chrono::duration<int> time_span;

        while (time_span.count() != seconds) {
            std::chrono::steady_clock::time_point t_duration = std::chrono::steady_clock::now();
            time_span = std::chrono::duration_cast<std::chrono::duration<int>>(t_duration - t_counter);
        }

        return true;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("There has been a timing error:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }

    return false;
}

/**
 * @brief MainWindow::print_exception
 * @param e
 * @param level
 */
void MainWindow::print_exception(const std::exception &e, int level)
{
    QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception &e) {
        print_exception(e, level + 1);
    } catch(...) {}

    return;
}

/**
 * @brief MainWindow::on_actionCheck_for_Updates_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionCheck_for_Updates_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_action_About_Dekoder_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_About_Dekoder_triggered()
{
    QPointer<AboutDialog> dlg_about = new AboutDialog(this);
    dlg_about->setWindowFlags(Qt::Window);
    dlg_about->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(dlg_about, SIGNAL(destroyed(QObject*)), this, SLOT(show()));
    dlg_about->show();
}

/**
 * @brief MainWindow::on_actionSet_Offset_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionSet_Offset_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_action_Connect_triggered Make a connection to the amateur radio rig, if at all possible.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_Connect_triggered()
{
    return;
}

/**
 * @brief MainWindow::on_action_Disconnect_triggered Disconnect any and all connections from the amateur radio rig, if
 * at all possible.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_Disconnect_triggered()
{
    try {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Disconnect"));
        if ((gkRadioPtr.get() != nullptr) && (gkRadioPtr->capabilities.get() != nullptr) && (gkRadioPtr->capabilities->model_name != nullptr)) {
            msgBox.setText(tr("Are you sure you wish to disconnect from your [ %1 ] radio rig?").arg(QString::fromStdString(gkRadioPtr->capabilities->model_name)));
        } else {
            msgBox.setText(tr("Are you sure you wish to disconnect from your radio rig?"));
        }
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Question);
        int ret = msgBox.exec();

        switch (ret) {
        case QMessageBox::Ok:
            emit disconnectRigInUse(gkRadioPtr->gkRig, gkRadioPtr);
            return;
        case QMessageBox::Cancel:
            return;
        default:
            return;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::on_actionShow_Waterfall_toggled
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void MainWindow::on_actionShow_Waterfall_toggled(bool arg1)
{
    Q_UNUSED(arg1);

    try {
        // QWT is started!
    } catch (const portaudio::PaException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudio error has occurred:\n\n%1").arg(e.paErrorText()), QMessageBox::Ok);
    } catch (const portaudio::PaCppException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A PortAudioCpp error has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("A generic exception has occurred:\n\n%1").arg(e.what()), QMessageBox::Ok);
    } catch (...) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An unknown exception has occurred. There are no further details."), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::on_action_Settings_triggered is located on the drop-down menu at the top.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_Settings_triggered()
{
    launchSettingsWin();
    return;
}

/**
 * @brief MainWindow::on_actionSave_Decoded_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionSave_Decoded_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_actionSave_All_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionSave_All_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_actionOpen_Save_Directory_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionOpen_Save_Directory_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_actionDelete_all_wav_files_in_Save_Directory_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionDelete_all_wav_files_in_Save_Directory_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

void MainWindow::on_actionPlay_triggered()
{
    gkAudioPlayDlg->setWindowFlags(Qt::Window);
    gkAudioPlayDlg->setAttribute(Qt::WA_DeleteOnClose, false);
    gkAudioPlayDlg->show();
}

/**
 * @brief MainWindow::on_actionSettings_triggered is located on the toolbar towards the top itself, where the larger icons are.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionSettings_triggered()
{
    launchSettingsWin();
}

/**
 * @brief MainWindow::infoBar
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://es.cppreference.com/w/cpp/io/manip/put_time>
 */
void MainWindow::infoBar()
{
    std::mutex info_bar_mtx;
    std::lock_guard<std::mutex> lck_guard(info_bar_mtx);

    try {
        std::ostringstream oss_utc_time;
        std::time_t curr_time = std::time(nullptr);
        oss_utc_time << std::put_time(std::gmtime(&curr_time), "%T %p");

        std::ostringstream oss_utc_date;
        oss_utc_date << std::put_time(std::gmtime(&curr_time), "%F");

        QString curr_utc_time_str = QString::fromStdString(oss_utc_time.str());
        QString curr_utc_date_str = QString::fromStdString(oss_utc_date.str());
        ui->label_curr_utc_time->setText(curr_utc_time_str);
        ui->label_curr_utc_date->setText(curr_utc_date_str);

        freq_t frequency_tmp = ((gkRadioPtr->freq / 1000) / 1000);
        if (frequency_tmp > 0.0) {
            ui->label_freq_large->setText(tr("%1 MHz").arg(QString::number(frequency_tmp)));
        } else {
            ui->label_freq_large->setText(tr("N/A"));
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::uponExit performs a number of functions upon exit of the application.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::uponExit()
{
    GkDb->write_mainwindow_settings(QString::number(this->window()->size().width()), general_mainwindow_cfg::WindowHSize);
    GkDb->write_mainwindow_settings(QString::number(this->window()->size().height()), general_mainwindow_cfg::WindowVSize);
    GkDb->write_mainwindow_settings(QString::number(this->window()->isMaximized()), general_mainwindow_cfg::WindowMaximized);

    QApplication::exit(EXIT_SUCCESS);
}

/**
 * @brief MainWindow::updateVolume will update the data within the circular buffer to the volume level as set
 * by the user themselves, usually from within QMainWindow.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The adjustment needed to the buffered data.
 * @note trukvl <https://stackoverflow.com/questions/15776390/controlling-audio-volume-in-real-time>
 */
void MainWindow::updateVolume(const float &value)
{
    if (rx_vol_control_selected) {
        //
        // Input device!
        //
        input_audio_buf->setVolume(value);
    } else {
        //
        // Output device!
        //
        output_audio_buf->setVolume(value);
    }

    return;
}

/**
 * @brief MainWindow::msgOutgoingProcess will process outgoing messages and prepare them for transmission with regards to
 * libraries such as Codec2, whilst clearing `ui->plainTextEdit_mesg_outgoing` of any text at the same time.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::msgOutgoingProcess(const QString &curr_text)
{
    Q_UNUSED(curr_text);
    QMessageBox::warning(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief MainWindow::updateSpectrograph continuously updates the embedded spectrograph(s) over a regular amount of time with
 * the latest information, in order to keep the data relevant.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::updateSpectrograph()
{
    const qint64 start_time = QDateTime::currentMSecsSinceEpoch();
    gk_spectro_start_time = start_time;
    gk_spectro_latest_time = start_time;

    #ifdef GK_CUDA_FFT_ENBL
    //
    // CUDA support is enabled!
    //
    try {
        if ((inputAudioStream != nullptr) && (pref_input_device.is_dev_active == true) && (AUDIO_FRAMES_PER_BUFFER > 0)) {
            while (inputAudioStream->isActive()) {
                std::vector<float> fftData;
                fftData.reserve(GK_FFT_SIZE + 1);
                const qint64 measure_start_time = QDateTime::currentMSecsSinceEpoch();
                while (fftData.size() < GK_FFT_SIZE) {
                    //
                    // Input audio stream is open and active!
                    //
                    auto audio_buf_tmp = std::make_shared<PaAudioBuf<qint16>>(*input_audio_buf);
                    std::vector<qint16> recv_buf;
                    while (audio_buf_tmp->size() > 0) {
                        recv_buf.reserve(AUDIO_FRAMES_PER_BUFFER + 1);
                        recv_buf.push_back(audio_buf_tmp->get());
                    }

                    if (!recv_buf.empty()) {

                        if (fftData.size() == GK_FFT_SIZE) {
                            processCUDAFFT(recv_buf.data(), fftData.data(), GK_FFT_SIZE);

                            //
                            // Perform the timing and date calculations!
                            //
                            const qint64 measure_end_time = QDateTime::currentMSecsSinceEpoch(); // The end time at the finalization of all calculations
                            const qint64 total_calc_time = measure_end_time - measure_start_time; // The total time it took to calculate everything
                            gk_spectro_latest_time = measure_end_time;

                            const qint64 spectro_time_diff = total_calc_time;
                            if (spectro_time_diff > SPECTRO_Y_AXIS_SIZE) {
                                // Stop the y-axis from growing more than `SPECTRO_Y_AXIS_SIZE` in size!
                                gk_spectro_start_time = gk_spectro_latest_time - SPECTRO_Y_AXIS_SIZE;
                            }

                            //
                            // In order to get the frequency information for each audio sample, you must:
                            // 1) Use a real-to-complex FFT of size N to generate frequency domain data.
                            // 2) Calculate the magnitude of your complex frequency domain data (i.e., `magnitude = std::sqrt(re^2 + im^2)`).
                            // 3) Optionally convert magnitude to a log scale (dB) (i.e., `magnitude_dB = 20 * std::log10(magnitude)`).
                            //

                            std::vector<double> magnitude_buf;
                            magnitude_buf.reserve(fftData.size() + 1);
                            for (const auto &calc: fftData) {
                                const double magnitude = std::sqrt(std::pow(calc, 2) + std::pow(calc, 2));
                                magnitude_buf.push_back(magnitude);
                            }

                            std::vector<float> freq_list;

                            std::vector<double> magnitude_db_buf;
                            magnitude_buf.reserve(magnitude_buf.size() + 1);
                            for (const auto &calc: magnitude_buf) {
                                const double magnitude_db = 20 * std::log10(calc);
                                magnitude_db_buf.push_back(magnitude_db);
                            }

                            QVector<double> fft_spectro_vals;
                            fft_spectro_vals.reserve(GK_FFT_SIZE + 1);
                            for (size_t i = 0; i < GK_FFT_SIZE; ++i) {
                                auto abs_val = std::abs(fftData[i]) / ((double)GK_FFT_SIZE);
                                fft_spectro_vals.push_back(abs_val);
                            }

                            gkSpectroGui->insertData(fft_spectro_vals, 1); // This is the data for the spectrograph / waterfall itself!
                            emit refreshSpectrograph(gk_spectro_latest_time, gk_spectro_start_time);

                            magnitude_buf.clear();
                            magnitude_db_buf.clear();
                            fftData.clear();
                        }
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        print_exception(e);
    }
    #else
    try {
        if ((inputAudioStream != nullptr) && (pref_input_device.is_dev_active == true) && (AUDIO_FRAMES_PER_BUFFER > 0)) {
            while (inputAudioStream->isActive()) {
                std::vector<float> fftData;
                fftData.reserve(GK_FFT_SIZE + 1);
                const qint64 measure_start_time = QDateTime::currentMSecsSinceEpoch();
                while (fftData.size() < GK_FFT_SIZE) {
                    //
                    // Input audio stream is open and active!
                    //
                    auto audio_buf_tmp = std::make_shared<PaAudioBuf<qint16>>(*input_audio_buf); // TODO: Possible SEGFAULT related to this with shutdown of Small World Deluxe...
                    std::vector<qint16> recv_buf;
                    while (audio_buf_tmp->size() > 0) {
                        recv_buf.reserve(AUDIO_FRAMES_PER_BUFFER + 1);
                        recv_buf.push_back(audio_buf_tmp->grab());
                    }

                    if (!recv_buf.empty()) {
                        fftData.push_back(*recv_buf.data());
                        recv_buf.clear();
                        recv_buf.shrink_to_fit();

                        if (fftData.size() == GK_FFT_SIZE) {
                            std::vector<GkFFTComplex> fftDataVals;
                            fftDataVals = gkFFT->FFTCompute(fftData, fftData.size(), (GK_FFT_SIZE / 2), GK_FFT_SIZE);

                            //
                            // Perform the timing and date calculations!
                            //
                            const qint64 measure_end_time = QDateTime::currentMSecsSinceEpoch(); // The end time at the finalization of all calculations
                            const qint64 total_calc_time = measure_end_time - measure_start_time; // The total time it took to calculate everything
                            gk_spectro_latest_time = measure_end_time;

                            const qint64 spectro_time_diff = total_calc_time;
                            if (spectro_time_diff > SPECTRO_Y_AXIS_SIZE) {
                                // Stop the y-axis from growing more than `SPECTRO_Y_AXIS_SIZE` in size!
                                gk_spectro_start_time = gk_spectro_latest_time - SPECTRO_Y_AXIS_SIZE;
                            }

                            //
                            // The input data are sound intensity values taken over time, equally spaced. They are
                            // said to be, appropriately enough, in the time domain. The output of the FT is said
                            // to be in the frequency domain because the horizontal axis is frequency. The vertical
                            // scale remains intensity. Although it isn't obvious from the input data, there is phase
                            // information in the input as well. Although all of the sound is sinusoidal, there is
                            // nothing that fixes the phases of the sine waves. This phase information appears in
                            // the frequency domain as the phases of the individual complex numbers, but often we
                            // don't care about it (and often we do too!). It just depends upon what you are doing!
                            //
                            // https://stackoverflow.com/questions/1679974/converting-an-fft-to-a-spectogram/10643179
                            //

                            std::vector<double> magnitude_buf;
                            magnitude_buf.reserve(fftDataVals.size() + 1);
                            for (const auto &calc: fftDataVals) {
                                const double magnitude = std::sqrt(std::pow(calc.real, 2) + std::pow(calc.imaginary, 2));
                                magnitude_buf.push_back(magnitude);
                            }

                            QVector<double> fft_spectro_vals;
                            fft_spectro_vals.reserve(GK_FFT_SAMPLE_SIZE + 1);
                            for (size_t i = 0; i < GK_FFT_SAMPLE_SIZE; ++i) {
                                fft_spectro_vals.push_back(fftDataVals[i].imaginary);
                            }

                            gkSpectroGui->insertData(fft_spectro_vals, 1); // This is the data for the spectrograph / waterfall itself!
                            emit refreshSpectrograph(gk_spectro_latest_time, gk_spectro_start_time);

                            magnitude_buf.clear();
                            fftDataVals.clear();
                        }
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        // TODO: We are using this method of reporting an exception because sometimes they are thrown upon termination of the application!
        std::cerr << tr("An error has occurred whilst undertaking calculations for the spectrograph / waterfall! Error:\n\n%1")
        .arg(QString::fromStdString(e.what())).toStdString() << std::endl;
    }

    #endif

    return;
}

/**
 * @brief MainWindow::on_verticalSlider_vol_control_valueChanged indirectly adjusts the volume of the audio data buffer itself
 * via the decibel formulae <https://en.wikipedia.org/wiki/Decibel#Acoustics>.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The value of the QSlider from the QMainWindow, ranging from 0-100 individual units.
 * @note Ansis Māliņš <https://stackoverflow.com/questions/49014440/what-is-the-correct-audio-volume-slider-formula>
 * trukvl <https://stackoverflow.com/questions/15776390/controlling-audio-volume-in-real-time>,
 */
void MainWindow::on_verticalSlider_vol_control_valueChanged(int value)
{
    //
    // The volume is a float between 0.0 and 1.0 and essentially represents a percentage of the max volume of the
    // audio. Since it will always be either 100% or less, the dB value will be either 0 (for full volume) or
    // negative. A positive dB value would mean there is amplification taking place which isn't possible with
    // simple playback.
    //
    const int vol_slider_max_val = ui->verticalSlider_vol_control->maximum();
    const float real_val = (static_cast<float>(value) / vol_slider_max_val);

    if (rx_vol_control_selected) {
        if (inputAudioStream != nullptr) {
            if (pref_input_device.is_dev_active) {
                //
                // Input audio stream is open and active!
                //
                updateVolumeSliderLabel(real_val);
            }
        }
    } else {
        if (pref_output_device.is_dev_active) {
            //
            // Output audio buffer
            //
            updateVolumeSliderLabel(real_val);
        }
    }

    return;
}

void MainWindow::on_actionSave_Decoded_Ab_triggered()
{
    return;
}

void MainWindow::on_actionView_Spectrogram_Controller_triggered()
{
    QPointer<SpectroDialog> dlg_spectro = new SpectroDialog(gkSpectroGui, this);
    dlg_spectro->setWindowFlags(Qt::Tool | Qt::Dialog);
    QObject::connect(dlg_spectro, SIGNAL(activateSpectroWaterfall(const bool &)),
                     gkSpectroGui, SLOT(showSpectrogram(const bool &)));
    dlg_spectro->show();

    return;
}

void MainWindow::on_pushButton_bridge_input_audio_clicked()
{
    if (!btn_bridge_input_audio) {
        // Set the QPushButton to 'Green'
        changePushButtonColor(ui->pushButton_bridge_input_audio, false);
        btn_bridge_input_audio = true;
    } else {
        // Set the QPushButton to 'Red'
        changePushButtonColor(ui->pushButton_bridge_input_audio, true);
        btn_bridge_input_audio = false;
    }

    return;
}

void MainWindow::on_pushButton_radio_receive_clicked()
{
    //
    // TODO: Implement a timer so this button can only be pressed every several
    // seconds, as according to the boost::thread() function...
    //

    try {
        std::unique_lock<std::timed_mutex> btn_record_lck(btn_record_mtx, std::defer_lock);
        if (!btn_radio_rx) {
            btn_record_lck.lock();
            if (pref_input_device.stream_parameters.device != paNoDevice) {
                if (pref_input_device.is_output_dev == boost::tribool::false_value) {
                    if (pref_input_device.device_info.maxInputChannels > 0) {
                        if ((pref_input_device.device_info.name != nullptr)) {
                            // Set the QPushButton to 'Green'
                            changePushButtonColor(ui->pushButton_radio_receive, false);
                            emit startRecording();

                            #ifndef GK_ENBL_VALGRIND_SUPPORT
                            spectro_timing_thread = std::thread(&MainWindow::updateSpectrograph, this);
                            spectro_timing_thread.detach();

                            vu_meter_thread = std::thread(&MainWindow::updateVolumeDisplayWidgets, this);
                            vu_meter_thread.detach();
                            #endif

                            changeStatusBarMsg(tr("Please wait! Beginning to receive audio..."));

                            return;
                        } else {
                            throw std::runtime_error(tr("The PortAudio library has not been properly initialized!").toStdString());
                        }
                    } else {
                        QMessageBox::warning(this, tr("Invalid device!"), tr("An invalid audio device has been provided. Please select another."), QMessageBox::Ok);
                        return;
                    }
                }
            } else {
                QMessageBox::warning(this, tr("Unavailable device!"), tr("No default input device! Please select one from the settings."), QMessageBox::Ok);
                return;
            }

            QMessageBox::warning(this, tr("Unavailable device!"), tr("You must firstly configure an appropriate **input** sound device. Please select one from the settings."),
                                 QMessageBox::Ok);
            return;
        } else {
            // Set the QPushButton to 'Red'
            changePushButtonColor(ui->pushButton_radio_receive, true);
            emit stopRecording();

            changeStatusBarMsg(tr("No longer receiving audio!"));

            if (!btn_record_lck.try_lock()) {
                btn_record_lck.unlock();
            }

            return;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("An issue was encountered while attempting to enter RX mode. Error: %1\n\nCancelling...").arg(e.what()),
                             QMessageBox::Ok);
    }

    return;
}

void MainWindow::on_pushButton_radio_transmit_clicked()
{
    if (!btn_radio_tx) {
        // Set the QPushButton to 'Green'
        changePushButtonColor(ui->pushButton_radio_transmit, false);
        btn_radio_tx = true;

        emit startTxAudio();
    } else {
        // Set the QPushButton to 'Red'
        changePushButtonColor(ui->pushButton_radio_transmit, true);
        btn_radio_tx = false;

        emit stopTxAudio();
    }

    return;
}

void MainWindow::on_pushButton_radio_tx_halt_clicked()
{
    if (!btn_radio_tx_halt) {
        // Set the QPushButton to 'Green'
        changePushButtonColor(ui->pushButton_radio_tx_halt, false);
        btn_radio_tx_halt = true;
    } else {
        // Set the QPushButton to 'Red'
        changePushButtonColor(ui->pushButton_radio_tx_halt, true);
        btn_radio_tx_halt = false;
    }

    return;
}

void MainWindow::on_pushButton_radio_monitor_clicked()
{
    if (!btn_radio_monitor) {
        // Set the QPushButton to 'Green'
        changePushButtonColor(ui->pushButton_radio_monitor, false);
        btn_radio_monitor = true;
    } else {
        // Set the QPushButton to 'Red'
        changePushButtonColor(ui->pushButton_radio_monitor, true);
        btn_radio_monitor = false;
    }

    return;
}

/**
 * @brief MainWindow::on_comboBox_select_frequency_activated
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void MainWindow::on_comboBox_select_frequency_activated(int index)
{
    Q_UNUSED(index);
    return;
}

/**
 * @brief MainWindow::on_comboBox_select_callsign_use_currentIndexChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void MainWindow::on_comboBox_select_callsign_use_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    return;
}

/**
 * @brief MainWindow::closeEvent is called upon having the Exit button in the extreme top-right being chosen.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    emit gkExitApp();
    event->ignore();
}

/**
 * @brief MainWindow::procRigPort changes the port used by Hamlib that's to be connected towards for communication with the
 * user's (transceiver) radio rig, whether it be a RS232/USB/Parallel/etc connection or something else entirely.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param conn_port The port itself that's to be connected towards (i.e. ttyS0, ttyUSB1, COM5, COM6, etc.).
 */
void MainWindow::procRigPort(const QString &conn_port, const GekkoFyre::AmateurRadio::GkConnMethod &conn_method)
{
    try {
        if (conn_method == GkConnMethod::CAT) {
            // CAT
            gkRadioPtr->cat_conn_port = conn_port;
        } else if (conn_method == GkConnMethod::PTT) {
            // PTT
            gkRadioPtr->ptt_conn_port = conn_port;
        } else {
            throw std::invalid_argument(tr("An error was encountered in determining the connection method used for your radio rig!").toStdString());
        }
    }  catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok, QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::stopRecordingInput The idea of this function is to stop recording of
 * all input audio devices related to the spectrograph / waterfall upon activation,
 * globally, across all threads.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param wait_time How long to wait for the thread to terminate safely before forcing
 * a termination/interrupt.
 * @return If recording of input audio devices have actually stopped or not.
 */
void MainWindow::stopRecordingInput()
{
    if (inputAudioStream != nullptr && pref_input_device.is_dev_active) {
        if (inputAudioStream->isActive()) {
            inputAudioStream->stop();
            inputAudioStream->close();

            pref_input_device.is_dev_active = false; // State that this recording device is now non-active!
        }
    }

    return;
}

/**
 * @brief MainWindow::startRecordingInput
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param wait_time How long to wait for the thread to instantiate safely before forcing
 * an abortion of the operation altogether.
 * @note PortAudio Documentation <https://app.assembla.com/spaces/portaudio/git/source/master/bindings/cpp/example/sine.cxx>.
 */
void MainWindow::startRecordingInput()
{
    emit stopRecording();

    // To minimise startup latency for this use-case (i.e. expecting StartStream() to give minimum
    // startup latency) you should use the paPrimeOutputBuffersUsingStreamCallback stream flag.
    // Otherwise the initial buffers will be zero and the time it takes for the sound to hit the
    // DACs will include playing out the buffer length of zeros (which would be around 80ms on
    // Windows WMME or DirectSound with the default PA settings).
    auto pa_stream_param = portaudio::StreamParameters(pref_input_device.cpp_stream_param, portaudio::DirectionSpecificStreamParameters::null(),
                                                       pref_input_device.def_sample_rate, AUDIO_FRAMES_PER_BUFFER,
                                                       paPrimeOutputBuffersUsingStreamCallback);
    inputAudioStream = std::make_shared<portaudio::MemFunCallbackStream<PaAudioBuf<qint16>>>(pa_stream_param, *input_audio_buf,
                                                                                             &PaAudioBuf<qint16>::recordCallback);
    inputAudioStream->start();

    pref_input_device.is_dev_active = true; // State that this recording device is now active!
    return;
}

/**
 * @brief MainWindow::stopTransmitOutput
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::stopTransmitOutput()
{
    if (outputAudioStream != nullptr && pref_output_device.is_dev_active) {
        if (outputAudioStream->isActive()) {
            outputAudioStream->stop();
            outputAudioStream->close();

            pref_output_device.is_dev_active = false; // State that this recording device is now non-active!
        }
    }

    return;
}

/**
 * @brief MainWindow::startTransmitOutput
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::startTransmitOutput()
{
    // To minimise startup latency for this use-case (i.e. expecting StartStream() to give minimum
    // startup latency) you should use the paPrimeOutputBuffersUsingStreamCallback stream flag.
    // Otherwise the initial buffers will be zero and the time it takes for the sound to hit the
    // DACs will include playing out the buffer length of zeros (which would be around 80ms on
    // Windows WMME or DirectSound with the default PA settings).
    auto pa_stream_param = portaudio::StreamParameters(portaudio::DirectionSpecificStreamParameters::null(), pref_output_device.cpp_stream_param,
                                                       pref_output_device.def_sample_rate, AUDIO_FRAMES_PER_BUFFER,
                                                       paPrimeOutputBuffersUsingStreamCallback);
    outputAudioStream = std::make_shared<portaudio::MemFunCallbackStream<PaAudioBuf<qint16>>>(pa_stream_param, *output_audio_buf,
                                                                                              &PaAudioBuf<qint16>::playbackCallback);
    outputAudioStream->start();

    return;
}

/**
 * @brief MainWindow::updateProgressBar will launch and manage a QProgressBar via signals and slots.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param enable Whether the QProgressBar should be displayed to the user or not via the GUI.
 * @param min The most minimum value possible.
 * @param max The most maximum value possible.
 */
void MainWindow::updateProgressBar(const bool &enable, const size_t &min, const size_t &max)
{
    Q_UNUSED(min);
    Q_UNUSED(max);

    try {
        if (enable) {
            // Launch the QProgressBar!
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::gatherRigCapabilities updates the capabilities of any rig that is currently in use, or about to be
 * within use.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param rig_model_update The amateur radio rig in question, which is identified by its unique integer as derived from
 * the `rig_list_foreach()` Hamlib function, that's to be updated and put into RAM for storage.
 * @param radio_ptr The pointer to Hamlib's radio structure and any information thereof.
 * @see With regards to the Hamlib libraries, see `rig_caps`. Another related function is MainWindow::parseRigCapabilities().
 */
void MainWindow::gatherRigCapabilities(const rig_model_t &rig_model_update,
                                       const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr)
{
    if (!gkRadioModels.isEmpty()) {
        if (rig_model_update > 1) {
            for (const auto &model: gkRadioModels.toStdMap()) {
                if (rig_model_update == model.first) {
                    // We have the desired amateur radio rig in question!
                    radio_ptr->capabilities.reset();
                    radio_ptr->capabilities = std::make_unique<rig_caps>(*std::get<0>(model.second));

                    return;
                }
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::addRigToMemory will add another amateur radio rig, of specific desire, to the list of those that are
 * currently within use throughout Small World Deluxe, and that are henceforth within the user's resident RAM.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param rig_model_update The amateur radio rig in question to add to the list of those that are within use throughout Small
 * World Deluxe.
 * @param radio_ptr The pointer to Hamlib's radio structure and any information thereof.
 * @see MainWindow::addRigInUse().
 */
void MainWindow::addRigToMemory(const rig_model_t &rig_model_update, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr)
{
    if (radio_ptr->gkRig.get() != nullptr) {
        emit disconnectRigInUse(radio_ptr->gkRig, radio_ptr);
    }

    //
    // Attempt to initialize the amateur radio rig!
    //
    emit recvRigCapabilities(rig_model_update, radio_ptr);
    radioInitStart();

    return;
}

/**
 * @brief MainWindow::disconnectRigInMemory will disconnect a given amateur radio rig from Small World Deluxe and perform
 * any other needed functions necessary.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param rig_to_disconnect The radio rig in question to disconnect.
 * @param radio_ptr The pointer to Hamlib's radio structure and any information thereof.
 */
void MainWindow::disconnectRigInMemory(std::shared_ptr<Rig> rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr)
{
    Q_UNUSED(rig_to_disconnect);

    if (gkRadioPtr.get() != nullptr) {
        if (gkRadioPtr->gkRig.get() != nullptr) {
            if (gkRadioPtr->is_open) {
                // Free the pointer(s) for the Hamlib library!
                rig_to_disconnect->close(); // Close port
                gkRadioPtr->is_open = false;

                for (const auto &port: status_com_ports) {
                    //
                    // CAT Port
                    //
                    if (radio_ptr->cat_conn_port == port.port_info.portName()) {
                        QSerialPort serial(port.port_info);
                        if (serial.isOpen()) {
                            serial.close();
                        }
                    }

                    //
                    // PTT Port
                    //
                    if (radio_ptr->ptt_conn_port == port.port_info.portName()) {
                        QSerialPort serial(port.port_info);
                        if (serial.isOpen()) {
                            serial.close();
                        }
                    }
                }
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::on_pushButton_radio_tune_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param checked
 */
void MainWindow::on_pushButton_radio_tune_clicked(bool checked)
{
    Q_UNUSED(checked);

    if (!btn_radio_tune) {
        // Set the QPushButton to 'Green'
        changePushButtonColor(ui->pushButton_radio_tune, false);
        btn_radio_tune = true;
    } else {
        // Set the QPushButton to 'Red'
        changePushButtonColor(ui->pushButton_radio_tune, true);
        btn_radio_tune = false;
    }

    return;
}

/**
 * @brief MainWindow::on_action_Print_triggered Controls the print(er) settings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_Print_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief MainWindow::on_checkBox_rx_tx_vol_toggle_stateChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param arg1
 */
void MainWindow::on_checkBox_rx_tx_vol_toggle_stateChanged(int arg1)
{
    rx_vol_control_selected = arg1;
    return;
}

void MainWindow::on_action_All_triggered()
{
    return;
}

void MainWindow::on_action_Incoming_triggered()
{
    return;
}

void MainWindow::on_action_Outgoing_triggered()
{
    return;
}

void MainWindow::on_actionUSB_toggled(bool arg1)
{
    ui->actionUSB->setChecked(arg1);

    return;
}

void MainWindow::on_actionLSB_toggled(bool arg1)
{
    ui->actionLSB->setChecked(arg1);

    return;
}

void MainWindow::on_actionAM_toggled(bool arg1)
{
    Q_UNUSED(arg1);

    return;
}

void MainWindow::on_actionFM_toggled(bool arg1)
{
    Q_UNUSED(arg1);

    return;
}

void MainWindow::on_actionSSB_toggled(bool arg1)
{
    Q_UNUSED(arg1);

    return;
}

void MainWindow::on_actionCW_toggled(bool arg1)
{
    Q_UNUSED(arg1);

    return;
}

/**
 * @brief MainWindow::on_action_Q_codes_triggered opens a web-browser (whichever that has been chosen as the default by
 * the user) on the user's computer and navigates to Wikipedia on all the (semi-)commonly used Q-codes by amateur radio
 * users themselves.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_Q_codes_triggered()
{
    QDesktopServices::openUrl(QUrl("https://en.wikipedia.org/wiki/Q_code#Amateur_radio"));

    return;
}

void MainWindow::on_actionPrint_triggered()
{
    /*
    try {
        QPrintDialog print_dialog(printer.get(), this);
        print_dialog.setWindowTitle(QString("Small World Deluxe - ") + tr("Print Document"));
        if (ui->textEdit_mesg_log->textCursor().hasSelection() || ui->plainTextEdit_mesg_outgoing->textCursor().hasSelection()) {
            print_dialog.addEnabledOption(QAbstractPrintDialog::PrintSelection);
        }

        if (print_dialog.exec() != QDialog::Accepted) {
            return;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("Apologies, but an issue was encountered while attempting to print:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }
    */

    return;
}

void MainWindow::on_pushButton_sstv_rx_navigate_left_clicked()
{
    //
    // Navigate to the next image!
    //

    return;
}

void MainWindow::on_pushButton_sstv_rx_navigate_right_clicked()
{
    //
    // Navigate to the previous image!
    //

    return;
}

void MainWindow::on_pushButton_sstv_rx_save_image_clicked()
{
    return;
}

void MainWindow::on_pushButton_sstv_rx_listen_rx_clicked()
{
    return;
}

void MainWindow::on_pushButton_sstv_rx_saved_image_nav_left_clicked()
{
    //
    // Navigate to the next image!
    //

    const int mem_pixmap_array_size = sstv_rx_saved_image_pixmap.size();
    if (sstv_rx_saved_image_idx < mem_pixmap_array_size) {
        sstv_rx_saved_image_idx += 1;
    } else {
        sstv_rx_saved_image_idx = 1;
    }

    const QPixmap pic(sstv_rx_saved_image_pixmap.at(sstv_rx_saved_image_idx - 1));
    label_sstv_rx_saved_image->setPixmap(pic);

    return;
}

void MainWindow::on_pushButton_sstv_rx_saved_image_nav_right_clicked()
{
    //
    // Navigate to the previous image!
    //

    const int mem_pixmap_array_size = sstv_rx_saved_image_pixmap.size();
    if (sstv_rx_saved_image_idx > 1) {
        sstv_rx_saved_image_idx -= 1;
    } else {
        sstv_rx_saved_image_idx = mem_pixmap_array_size;
    }

    const QPixmap pic(sstv_rx_saved_image_pixmap.at(sstv_rx_saved_image_idx - 1));
    label_sstv_rx_saved_image->setPixmap(pic);

    return;
}

void MainWindow::on_pushButton_sstv_rx_saved_image_load_clicked()
{
    return;
}

void MainWindow::on_pushButton_sstv_rx_saved_image_delete_clicked()
{
    return;
}

void MainWindow::on_pushButton_sstv_tx_navigate_left_clicked()
{
    //
    // Navigate to the next image!
    //

    const int files_array_size = sstv_tx_pic_files.size();
    if (sstv_tx_image_idx < files_array_size) {
        sstv_tx_image_idx += 1;
    } else {
        sstv_tx_image_idx = 1;
    }

    const QPixmap pic(sstv_tx_pic_files.at(sstv_tx_image_idx - 1));
    label_sstv_tx_image->setPixmap(pic);

    return;
}

void MainWindow::on_pushButton_sstv_tx_navigate_right_clicked()
{
    //
    // Navigate to the previous image!
    //

    const int files_array_size = sstv_tx_pic_files.size();
    if (sstv_tx_image_idx > 1) {
        sstv_tx_image_idx -= 1;
    } else {
        sstv_tx_image_idx = files_array_size;
    }

    const QPixmap pic(sstv_tx_pic_files.at(sstv_tx_image_idx - 1));
    label_sstv_tx_image->setPixmap(pic);

    return;
}

/**
 * @brief MainWindow::on_pushButton_sstv_tx_load_image_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note QFileDialog <https://doc.qt.io/qt-5/qfiledialog.html>
 */
void MainWindow::on_pushButton_sstv_tx_load_image_clicked()
{
    try {
        QFileDialog fileDialog(this, tr("Load Image for Transmission"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                               tr("Images (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"));
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
        fileDialog.setViewMode(QFileDialog::Detail);

        if (fileDialog.exec()) {
            sstv_tx_pic_files = fileDialog.selectedFiles();
            if (!sstv_tx_pic_files.isEmpty()) {
                if (sstv_tx_pic_files.size() > GK_SSTV_FILE_DLG_LOAD_IMGS_MAX_FILES_WARN) {
                    if (!fileOverloadWarning(sstv_tx_pic_files.size(), GK_SSTV_FILE_DLG_LOAD_IMGS_MAX_FILES_WARN)) {
                        return;
                    }
                } // Otherwise carry on as normal!

                qint64 total_fize_sizes = 0;
                for (const auto &data: sstv_tx_pic_files) {
                    QFile file(data);

                    file.open(QIODevice::ReadOnly);
                    total_fize_sizes += file.size();
                    file.close();
                }

                const QPixmap pic(sstv_tx_pic_files.first());
                label_sstv_tx_image->setPixmap(pic);
                sstv_tx_image_idx = 1; // TODO: Make it so that this function can load images 'in-between' other images and so on!

                // Publish an event within the event log!
                gkEventLogger->publishEvent(tr("Loaded %1 images into memory, ready for transmission. Total file size: %2 kB")
                                            .arg(QString::number(fileDialog.selectedFiles().size())).arg(QString::number(total_fize_sizes)),
                                            GkSeverity::Info);
            }

            return;
        }
    }  catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::on_pushButton_sstv_tx_send_image_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_pushButton_sstv_tx_send_image_clicked()
{
    QPixmap tx_send_img = label_sstv_tx_image->pixmap();
    if (!tx_send_img.isNull()) {
        QByteArray byte_array;
        QBuffer buffer(&byte_array);
        buffer.open(QIODevice::WriteOnly);
        tx_send_img.save(&buffer, "JPEG"); // Writes pixmap into bytes in JPEG format

        //
        // Initialize any amateur radio modems!
        //
        #ifdef CODEC2_LIBS_ENBLD
        QPointer<GkCodec2> gkCodec2 = new GkCodec2(Codec2Mode::freeDvMode2020, Codec2ModeCustom::GekkoFyreV1, 0, 0, GkDb, gkEventLogger, output_audio_buf, this);
        gkCodec2->transmitData(byte_array, true);
        #endif
    } else {
        QMessageBox::information(this, tr("No image!"), tr("Please ensure to have an image loaded before attempting to make a transmission."), QMessageBox::Ok, QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::on_pushButton_sstv_rx_remove_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_pushButton_sstv_rx_remove_clicked()
{
    return;
}

/**
 * @brief MainWindow::on_pushButton_sstv_tx_remove_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_pushButton_sstv_tx_remove_clicked()
{
    return;
}
