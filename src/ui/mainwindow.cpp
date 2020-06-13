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

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "aboutdialog.hpp"
#include "spectrodialog.hpp"
#include <boost/exception/all.hpp>
#include <boost/chrono/chrono.hpp>
#include <sstream>
#include <ostream>
#include <cstring>
#include <utility>
#include <cmath>
#include <iostream>
#include <functional>
#include <chrono>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QPrintDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QResource>
#include <QMultiMap>
#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QDate>
#include <QUrl>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;

namespace fs = boost::filesystem;
namespace sys = boost::system;

//
// Statically declared members
//
QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, GekkoFyre::AmateurRadio::rig_type>> MainWindow::gkRadioModels = initRadioModelsVar();
std::list<GekkoFyre::Database::Settings::GkComPort> MainWindow::status_com_ports;

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
    qRegisterMetaType<std::vector<GekkoFyre::Spectrograph::RawFFT>>("std::vector<GekkoFyre::Spectrograph::RawFFT>");
    qRegisterMetaType<GekkoFyre::Database::Settings::GkUsbPort>("GekkoFyre::Database::Settings::GkUsbPort");
    qRegisterMetaType<GekkoFyre::AmateurRadio::GkConnType>("GekkoFyre::AmateurRadio::GkConnType");
    qRegisterMetaType<GekkoFyre::AmateurRadio::DigitalModes>("GekkoFyre::AmateurRadio::DigitalModes");
    qRegisterMetaType<GekkoFyre::AmateurRadio::IARURegions>("GekkoFyre::AmateurRadio::IARURegions");
    qRegisterMetaType<GekkoFyre::AmateurRadio::GkFreqs>("GekkoFyre::AmateurRadio::GkFreqs");
    qRegisterMetaType<RIG>("RIG");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<rig_model_t>("rig_model_t");
    qRegisterMetaType<PaHostApiTypeId>("PaHostApiTypeId");
    qRegisterMetaType<std::vector<short>>("std::vector<short>");

    try {
        // Initialize QMainWindow to a full-screen after a single second!
        // QTimer::singleShot(0, this, SLOT(showFullScreen()));

        // Print out the current date
        std::cout << QDate::currentDate().toString().toStdString() << std::endl;

        fs::path slash = "/";
        native_slash = slash.make_preferred().native();

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

        sw_settings = std::make_shared<QSettings>(QSettings::SystemScope, General::companyName,
                                                  General::productName, this);

        //
        // Initialize the list of frequencies that Small World Deluxe needs to communicate with other users
        // throughout the globe/world!
        //
        gkFreqList = new GkFreqList(this);
        QObject::connect(gkFreqList, SIGNAL(updateFrequencies(const float &, const GekkoFyre::AmateurRadio::DigitalModes &, const GekkoFyre::AmateurRadio::IARURegions &, const bool &)),
                         this, SLOT(updateFreqsInMem(const float &, const GekkoFyre::AmateurRadio::DigitalModes &, const GekkoFyre::AmateurRadio::IARURegions &, const bool &)));
        gkFreqList->publishFreqList();

        //
        // Create a status bar at the bottom of the window with a default message
        // https://doc.qt.io/qt-5/qstatusbar.html
        //
        createStatusBar();

        //
        // Configure the volume meter!
        //
        ui->progressBar_spect_vu_meter->setMinimum(0);
        ui->progressBar_spect_vu_meter->setMaximum(100);
        ui->progressBar_spect_vu_meter->setValue(0);

        //
        // Initialize the default logic state on all applicable QPushButtons within QMainWindow.
        //
        btn_bridge_input_audio = false;
        btn_radio_rx = false;
        btn_radio_tx = false;
        btn_radio_tx_halt = false;
        btn_radio_tune = false;
        btn_radio_monitor = false;

        rx_vol_control_selected = false;
        global_rx_audio_volume = 0.0;
        global_tx_audio_volume = 0.0;

        //
        // Initialize Hamlib!
        //
        rig_load_all_backends();
        rig_list_foreach(parseRigCapabilities, nullptr);

        // Create class pointers
        fileIo = std::make_shared<GekkoFyre::FileIo>(sw_settings, this);
        gkStringFuncs = std::make_shared<GekkoFyre::StringFuncs>(this);

        //
        // Settings database-related logic
        //
        QString init_settings_db_loc = fileIo->read_initial_settings(init_cfg::DbLoc);
        QString init_settings_db_name = fileIo->read_initial_settings(init_cfg::DbName);

        // Create path to file-database
        sys::error_code ec;
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
                    // Directory does not exist despite been saved as a setting, so we must create it first!
                    fs::path dir_to_append = fs::path(Filesystem::defaultDirAppend + native_slash.string() + Filesystem::fileName);
                    save_db_path = fileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                                            true, QString::fromStdString(dir_to_append.string())).toStdString(); // Path to save final database towards
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
                GkDb = std::make_shared<GekkoFyre::GkLevelDb>(db, fileIo, this);

                //
                // Initialize the ability to change Connection Type, depending on whether we are connecting to the amateur radio rig
                // by USB, RS232, GPIO, etc.!
                //
                QObject::connect(this, SIGNAL(changePortType(const GekkoFyre::AmateurRadio::GkConnType &, const bool &)),
                                 this, SLOT(selectedPortType(const GekkoFyre::AmateurRadio::GkConnType &, const bool &)));
                QObject::connect(this, SIGNAL(gatherPortType(const bool &)),
                                 this, SLOT(analyzePortType(const bool &)));

                //
                // Load some of the primary abilities to work with the Hamlib libraries!
                //
                QObject::connect(this, SIGNAL(recvRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(gatherRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
                QObject::connect(this, SIGNAL(addRigInUse(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(addRigToMemory(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
                QObject::connect(this, SIGNAL(disconnectRigInUse(RIG *, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(disconnectRigInMemory(RIG *, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
                QObject::connect(this, SIGNAL(updateRadioPtr(const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(updateRadioVarsInMem(const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
                QObject::connect(this, SIGNAL(updateFrequencies(const float &, const GekkoFyre::AmateurRadio::DigitalModes &, const GekkoFyre::AmateurRadio::IARURegions &, const bool &)),
                                 this, SLOT(updateFreqsInMem(const float &, const GekkoFyre::AmateurRadio::DigitalModes &, const GekkoFyre::AmateurRadio::IARURegions &, const bool &)));

                // Initialize the Radio Database pointer!
                if (gkRadioPtr.get() == nullptr) {
                    gkRadioPtr = std::make_shared<GkRadio>();
                }

                gkRadioPtr = readRadioSettings();
                emit addRigInUse(gkRadioPtr->rig_model, gkRadioPtr);

                // Initialize USB devices!
                gkRadioLibs = new GekkoFyre::RadioLibs(fileIo, gkStringFuncs, GkDb, gkRadioPtr, this);
                status_com_ports = gkRadioLibs->status_com_ports();
                usb_ctx_ptr = gkRadioLibs->initUsbLib();
                gkUsbPortPtr = std::make_shared<GkUsbPort>();

                //
                // Setup the SIGNALS & SLOTS for `gkRadioLibs`...
                //
                QObject::connect(gkRadioLibs, SIGNAL(gatherPortType(const bool &)),
                                 this, SLOT(analyzePortType(const bool &)));
                QObject::connect(gkRadioLibs, SIGNAL(disconnectRigInUse(RIG *, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(disconnectRigInMemory(RIG *, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
                QObject::connect(gkRadioLibs, SIGNAL(updateRadioPtr(const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(updateRadioVarsInMem(const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
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
        bool window_maximized = GkDb->read_mainwindow_settings(general_mainwindow_cfg::WindowMaximized).toInt();

        // Set the x-axis size of QMainWindow
        if (window_width >= MIN_MAIN_WINDOW_WIDTH) {
            this->window()->size().setWidth(window_width);
        }

        // Set the y-axis size of QMainWindow
        if (window_height >= MIN_MAIN_WINDOW_HEIGHT) {
            this->window()->size().setHeight(window_height);
        }

        // Whether to maximize the QMainWindow or not
        if (window_maximized == 1) {
            this->window()->showMaximized();
        } else if (window_maximized == 0) {
            this->window()->showNormal();
        } else {
            window_maximized = false;
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

        gkAudioDevices = std::make_shared<GekkoFyre::AudioDevices>(GkDb, fileIo, gkStringFuncs, this);
        auto pref_audio_devices = gkAudioDevices->initPortAudio(gkPortAudioInit);

        if (!pref_audio_devices.empty()) {
            for (const auto &device: pref_audio_devices) {
                // Now filter out what is the input and output device selectively!
                if (device.is_output_dev) {
                    // Output device
                    pref_output_device = device;
                } else {
                    // Input device
                    pref_input_device = device;
                }
            }
        }

        //
        // Initialize the Waterfall / Spectrograph
        //
        gkSpectroGui = new GekkoFyre::SpectroGui(gkStringFuncs, true, false, this);
        ui->verticalLayout_11->addWidget(gkSpectroGui);
        gkSpectroGui->setEnabled(true);
        QObject::connect(this, SIGNAL(sendSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<int> &, const int &, const size_t &)),
                         gkSpectroGui, SIGNAL(sendSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<int> &, const int &, const size_t &)));

        //
        // Sound & Audio Devices
        //
        QObject::connect(this, SIGNAL(stopRecording(const bool &, const int &)), this, SLOT(stopRecordingInput(const bool &, const int &)));
        QObject::connect(this, SIGNAL(refreshVuMeter(double)), this, SLOT(updateVuMeter(double)));

        //
        // QMainWindow widgets
        //
        prefillAmateurBands();

        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(infoBar()));
        timer->start(1000);

        const size_t fft_num_lines = SPECTRO_NUM_LINES;
        const size_t fft_samples_per_line = SPECTRO_SAMPLES_PER_LINE;

        if (!pref_audio_devices.empty()) {
            const size_t audio_input_sample_length = (pref_input_device.def_sample_rate * SPECTRO_SAMPLING_LENGTH);
            const size_t input_audio_circ_buf_size = ((fft_num_lines - 1) * fft_samples_per_line + audio_input_sample_length);

            pref_input_audio_buf = new GekkoFyre::PaAudioBuf(input_audio_circ_buf_size, pref_output_device, pref_input_device, this);
            paMicProcBackground = new GekkoFyre::paMicProcBackground(gkPortAudioInit, pref_input_audio_buf, gkAudioDevices, gkStringFuncs, fileIo, GkDb,
                                                                     pref_input_device, input_audio_circ_buf_size, fft_samples_per_line, fft_num_lines,
                                                                     nullptr);

            //
            // Spectrograph signals and slots
            //
            QObject::connect(paMicProcBackground, SIGNAL(updateFrequencies(const float &, const GekkoFyre::AmateurRadio::DigitalModes &, const GekkoFyre::AmateurRadio::IARURegions &, const bool &)),
                             this, SLOT(updateFreqsInMem(const float &, const GekkoFyre::AmateurRadio::DigitalModes &, const GekkoFyre::AmateurRadio::IARURegions &, const bool &)));
            QObject::connect(this, SIGNAL(stopRecording(const bool &, const int &)), paMicProcBackground, SLOT(abortRecording(const bool &, const int &)));
            QObject::connect(paMicProcBackground, SIGNAL(updateVolume(const double &)), this, SLOT(updateVuMeter(const double &)));
            QObject::connect(this, SIGNAL(stopRecording(const bool &, const int &)), pref_input_audio_buf, SLOT(abortRecording(const bool &, const int &)));
            QObject::connect(ui->verticalSlider_vol_control, SIGNAL(valueChanged(int)), this, SLOT(updateVolMeterTooltip(const int &)));
            QObject::connect(paMicProcBackground, SIGNAL(updateWaterfall(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<int> &, const int &, const size_t &)),
                             this, SLOT(updateSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<int> &, const int &, const size_t &)));
            QObject::connect(paMicProcBackground, SIGNAL(stopRecording(const bool &, const int &)),
                             gkSpectroGui, SIGNAL(stopSpectroRecv(const bool &, const int &)));
        }

        std::thread t1(&MainWindow::infoBar, this);
        t1.detach();

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
            const size_t audio_output_sample_length = (pref_input_device.def_sample_rate * SPECTRO_SAMPLING_LENGTH);
            const size_t output_audio_circ_buf_size = ((fft_num_lines - 1) * fft_samples_per_line + audio_output_sample_length);
            pref_output_audio_buf = new GekkoFyre::PaAudioBuf(output_audio_circ_buf_size, pref_output_device, pref_input_device, this);

            gkAudioEncoding = new GkAudioEncoding(fileIo, pref_input_audio_buf, GkDb, gkSpectroGui,
                                                  gkStringFuncs, pref_input_device, this);
            gkAudioDecoding = new GkAudioDecoding(fileIo, pref_output_audio_buf, GkDb, gkStringFuncs,
                                                  pref_output_device, this);

            //
            // Audio encoding signals and slots
            //
            gkAudioPlayDlg = new GkAudioPlayDialog(GkDb, gkAudioDecoding, gkAudioDevices, fileIo, this);
            QObject::connect(gkAudioPlayDlg, SIGNAL(beginRecording(const bool &)), this, SLOT(stopAudioCodecRec(const bool &)));
            QObject::connect(gkAudioPlayDlg, SIGNAL(beginRecording(const bool &)), gkAudioEncoding, SLOT(startRecording(const bool &)));
        }

        if (gkRadioPtr->freq > 0.0) {
            ui->label_freq_large->setText(QString::number(gkRadioPtr->freq));
        } else {
            ui->label_freq_large->setText(tr("N/A"));
        }
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
    std::mutex main_win_termination_mtx;
    std::lock_guard<std::mutex> lck_guard(main_win_termination_mtx);

    emit stopRecording(true, 5000);

    if (pref_input_device.dev_input_channel_count > 0 && pref_input_device.def_sample_rate > 0) {
        if (pref_input_audio_buf != nullptr) {
            delete pref_input_audio_buf;
        }
    }

    emit disconnectRigInUse(gkRadioPtr->rig, gkRadioPtr);

    // Free the pointer for the libusb library!
    if (usb_ctx_ptr != nullptr) {
        libusb_exit(usb_ctx_ptr);
    }

    // Free the pointer for the Google LevelDB library!
    delete db;

    // Free the pointer for the PortAudio library!
    autoSys.terminate();
    gkPortAudioInit->terminate();

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
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND160));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND80));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND60));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND40));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND30));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND20));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND17));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND15));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND12));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND10));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND6));
        bands.push_back(gkRadioLibs->translateBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND2));

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
    QPointer<DialogSettings> dlg_settings = new DialogSettings(GkDb, fileIo, gkAudioDevices, gkRadioLibs, sw_settings,
                                                               gkPortAudioInit, usb_ctx_ptr, gkRadioPtr, status_com_ports,
                                                               this);
    dlg_settings->setWindowFlags(Qt::Window);
    dlg_settings->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(dlg_settings, SIGNAL(destroyed(QObject*)), this, SLOT(show()));

    QObject::connect(dlg_settings, SIGNAL(recvRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                     this, SLOT(gatherRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
    QObject::connect(dlg_settings, SIGNAL(changePortType(const GekkoFyre::AmateurRadio::GkConnType &, const bool &)),
                     this, SLOT(selectedPortType(const GekkoFyre::AmateurRadio::GkConnType &, const bool &)));
    QObject::connect(dlg_settings, SIGNAL(gatherPortType(const bool &)),
                     this, SLOT(analyzePortType(const bool &)));
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
        std::string rand_file_name = fileIo->create_random_string(8);
        fs::path rig_file_path_tmp = fs::path(fs::temp_directory_path().string() + native_slash.string() + rand_file_name + GekkoFyre::Filesystem::tmpExtension);
        gkRadioPtr->rig_file = rig_file_path_tmp.string();

        QString model = GkDb->read_rig_settings(radio_cfg::RigModel);
        if (!model.isEmpty() || !model.isNull()) {
            gkRadioPtr->rig_model = model.toInt();
        } else {
            gkRadioPtr->rig_model = 1;
        }

        if (gkRadioPtr->rig_model <= 0) {
            gkRadioPtr->rig_model = 1;
        }

        QString com_baud_rate = GkDb->read_rig_settings(radio_cfg::ComBaudRate);
        if (!com_baud_rate.isEmpty() || !com_baud_rate.isNull()) {
            gkRadioPtr->dev_baud_rate = gkRadioLibs->convertBaudRateToEnum(com_baud_rate.toInt());
        } else {
            gkRadioPtr->dev_baud_rate = AmateurRadio::com_baud_rates::BAUD9600;
        }

        #ifdef GFYRE_HAMLIB_DBG_VERBOSITY_ENBL
        radio->verbosity = RIG_DEBUG_VERBOSE;
        #else
        gkRadioPtr->verbosity = RIG_DEBUG_BUG;
        #endif

        try {
            //
            // Initialize Hamlib!
            //
            gkRadioPtr->is_open = false;
            rig_thread = std::thread(&RadioLibs::gkInitRadioRig, gkRadioLibs, gkRadioPtr, gkUsbPortPtr);
            rig_thread.detach();

            return true;
        } catch (const std::invalid_argument &e) {
            // We wish for this to be handled silently for now!
            Q_UNUSED(e);
            return false;
        } catch (const std::runtime_error &e) {
            #if defined(_MSC_VER) && (_MSC_VER > 1900)
            HWND hwnd = nullptr;
            gkStringFuncs->modalDlgBoxOk(hwnd, tr("Error!"), tr("[ Hamlib ] %1").arg(e.what()), MB_ICONERROR);
            DestroyWindow(hwnd);
            #else
            gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("[ Hamlib ] %1").arg(e.what()));
            #endif
            return false;
        }
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

        if (!rigBrand.isNull() || !rigBrand.isEmpty()) { // The manufacturer!
            int conv_rig_brand = rigBrand.toInt();
            gk_radio_tmp->rig_brand = conv_rig_brand;
        }

        Q_UNUSED(rigModel);
        // if (!rigModel.isNull() || !rigModel.isEmpty()) {}

        if (!rigModelIndex.isNull() || !rigModelIndex.isEmpty()) { // The actual amateur radio rig itself!
            int conv_rig_model_idx = rigModelIndex.toInt();
            gk_radio_tmp->rig_model = conv_rig_model_idx;
        }

        Q_UNUSED(rigVers);
        // if (!rigVers.isNull() || !rigVers.isEmpty()) {}

        emit gatherPortType(true);
        emit gatherPortType(false);

        if (!comBaudRate.isNull() || !comBaudRate.isEmpty()) {
            int conv_com_baud_rate = comBaudRate.toInt();
            gk_radio_tmp->port_details.parm.serial.rate = gkRadioLibs->convertBaudRateInt(gkRadioLibs->convertBaudRateToEnum(conv_com_baud_rate));
        }

        if (!stopBits.isNull() || !stopBits.isEmpty()) {
            int conv_serial_stop_bits = stopBits.toInt();
            gk_radio_tmp->port_details.parm.serial.stop_bits = conv_serial_stop_bits;
        }

        if (!data_bits.isNull() || !data_bits.isEmpty()) {
            int conv_serial_data_bits = data_bits.toInt();
            gk_radio_tmp->port_details.parm.serial.data_bits = conv_serial_data_bits;
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
        }

        if (!ptt_mode.isNull() || !ptt_mode.isEmpty()) {
            int conv_ptt_mode = ptt_mode.toInt();
            switch (conv_ptt_mode) {
            case 0:
                // None
                gk_radio_tmp->mode = rmode_t::RIG_MODE_NONE;
                break;
            case 1:
                // USB
                gk_radio_tmp->mode = rmode_t::RIG_MODE_USB;
                break;
            case 2:
                // Data / PKT
                gk_radio_tmp->mode = rmode_t::RIG_MODE_PKTUSB;
                break;
            default:
                // Nothing
                gk_radio_tmp->mode = rmode_t::RIG_MODE_NONE;
                break;
            }
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
        }

        if (!ptt_adv_cmd.isNull() || !ptt_adv_cmd.isEmpty()) {
            gk_radio_tmp->adv_cmd = ptt_adv_cmd.toStdString();
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
 * @brief MainWindow::on_actionCheck_for_Updates_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionCheck_for_Updates_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_action_About_Small world_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_About_Small world_triggered()
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
        if ((gkRadioPtr.get() != nullptr) && (gkRadioPtr->rig_caps.get() != nullptr) && (gkRadioPtr->rig_caps->model_name != nullptr)) {
            msgBox.setText(tr("Are you sure you wish to disconnect from your [ %1 ] radio rig?").arg(QString::fromStdString(gkRadioPtr->rig_caps->model_name)));
        } else {
            msgBox.setText(tr("Are you sure you wish to disconnect from your radio rig?"));
        }
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Question);
        int ret = msgBox.exec();

        switch (ret) {
        case QMessageBox::Ok:
            emit disconnectRigInUse(gkRadioPtr->rig, gkRadioPtr);
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
 * @brief MainWindow::stopAudioCodecRec
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param recording_is_started
 */
void MainWindow::stopAudioCodecRec(const bool &recording_is_started)
{
    recording_in_progress = recording_is_started;

    return;
}

/**
 * @brief MainWindow::updateVuMeter updates/refreshes the widget on the left-hand side of the
 * QMainWindow which displays the volume level at any immediate time.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param volumePctg The volume percentage for the QWidget meter to be set towards.
 */
void MainWindow::updateVuMeter(const double &volumePctg)
{
    ui->progressBar_spect_vu_meter->setValue(volumePctg);
}

/**
 * @brief MainWindow::updateVolMeterTooltip updates the tooltip on the volume slider to the right-hand
 * side of QMainWindow as it is moved up/down, reflecting the actually set volume percentage.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The given volume, as a percentage.
 */
void MainWindow::updateVolMeterTooltip(const int &value)
{
    ui->verticalSlider_vol_control->setToolTip(tr("Volume: %1%").arg(QString::number(value)));

    return;
}

/**
 * @brief MainWindow::updateVolIndex controls the volume of a PortAudio stream through
 * a, '0.0 <-> 1.0', sized floating point index.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param percentage The volume percentage that's to be converted to an index.
 */
void MainWindow::updateVolIndex(const int &percentage)
{
    return;
}

/**
 * @brief MainWindow::radioStats Displays statistics to the QMainWindow concerning the user's radio rig
 * of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param radio_dev The structure containing all the information on the user's radio rig of choice.
 */
void MainWindow::radioStats(Control::GkRadio *radio_dev)
{
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
                            emit stopRecording(false);

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
            emit stopRecording(true, 5000);

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
    } else {
        // Set the QPushButton to 'Red'
        changePushButtonColor(ui->pushButton_radio_transmit, true);
        btn_radio_tx = false;
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
 * @brief MainWindow::stopRecordingInput The idea of this function is to stop recording of
 * all input audio devices related to the spectrograph / waterfall upon activation,
 * globally, across all threads.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param recording_is_stopped A toggle switch that tells the function whether to stop
 * recording of input audio devices or not.
 * @param wait_time How long to wait for the thread to terminate safely before forcing
 * a termination/interrupt.
 * @return If recording of input audio devices have actually stopped or not.
 */
bool MainWindow::stopRecordingInput(const bool &recording_is_stopped, const int &wait_time)
{
    if (recording_is_stopped) {
        btn_radio_rx = false;
    } else {
        btn_radio_rx = true;
    }

    return false;
}

/**
 * @brief MainWindow::updateSpectroData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data
 * @param raw_audio_data
 * @param hanning_window_size
 * @param buffer_size
 */
void MainWindow::updateSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &data,
                                   const std::vector<int> &raw_audio_data, const int &hanning_window_size,
                                   const size_t &buffer_size)
{
    try {
        if (!data.empty()) {
            // auto fut_spectro_gui_apply_data = std::async(std::launch::async, std::bind(&SpectroGui::applyData, gkSpectroGui, data, hanning_window_size, buffer_size));
            emit sendSpectroData(data, raw_audio_data, hanning_window_size, buffer_size);

            return;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), tr("An issue has occurred whilst updating the spectrograph:\n\n%1").arg(e.what()),
                             QMessageBox::Ok);
    }

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
 * @brief MainWindow::selectedPortType notifies Small World Deluxe as to what kind of connection you are using
 * for CAT and PTT, whether it be RS232, USB, Parallel, GPIO, etc.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param rig_conn_type The type of connection in use for CAT and PTT modes.
 * @param is_cat_mode Whether we are modifying CAT mode or PTT instead.
 */
void MainWindow::selectedPortType(const GekkoFyre::AmateurRadio::GkConnType &rig_conn_type, const bool &is_cat_mode)
{
    if (is_cat_mode) {
        QString comDeviceCat = "";
        if (rig_conn_type == GkConnType::RS232) {
            comDeviceCat = GkDb->read_rig_settings_comms(radio_cfg::ComDeviceCat, GkConnType::RS232);
        } else if (rig_conn_type == GkConnType::USB) {
            comDeviceCat = GkDb->read_rig_settings_comms(radio_cfg::UsbDeviceCat, GkConnType::USB);
        } else if (rig_conn_type == GkConnType::Parallel) {
            comDeviceCat = GkDb->read_rig_settings_comms(radio_cfg::ParallelCat, GkConnType::Parallel);
        } else {
            gkRadioPtr->cat_conn_type = GkConnType::None;
            gkRadioPtr->cat_conn_port = "";
            return;
        }

        gkRadioPtr->cat_conn_type = rig_conn_type;
        gkRadioPtr->cat_conn_port = comDeviceCat.toStdString();
    } else {
        QString comDevicePtt = "";
        if (rig_conn_type == GkConnType::RS232) {
            comDevicePtt = GkDb->read_rig_settings_comms(radio_cfg::ComDevicePtt, GkConnType::RS232);
        } else if (rig_conn_type == GkConnType::USB) {
            comDevicePtt = GkDb->read_rig_settings_comms(radio_cfg::UsbDevicePtt, GkConnType::USB);
        } else if (rig_conn_type == GkConnType::Parallel) {
            comDevicePtt = GkDb->read_rig_settings_comms(radio_cfg::ParallelPtt, GkConnType::Parallel);
        } else {
            gkRadioPtr->ptt_conn_type = GkConnType::None;
            gkRadioPtr->ptt_conn_port = "";
            return;
        }

        gkRadioPtr->ptt_conn_type = rig_conn_type;
        gkRadioPtr->ptt_conn_port = comDevicePtt.toStdString();
    }

    return;
}

/**
 * @brief MainWindow::analyzePortType
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param is_cat_mode
 */
void MainWindow::analyzePortType(const bool &is_cat_mode)
{
    std::unique_ptr<GkRadio> gk_radio_tmp = std::make_unique<GkRadio>();
    if (is_cat_mode) {
        //
        // CAT Mode
        //
        QString catConnType = GkDb->read_rig_settings(radio_cfg::CatConnType);
        if (!catConnType.isNull() || !catConnType.isEmpty()) {
            int conv_cat_conn_type = catConnType.toInt();
            gk_radio_tmp->cat_conn_type = GkDb->convConnTypeToEnum(conv_cat_conn_type);

            switch (gk_radio_tmp->cat_conn_type) {
            case GkConnType::RS232:
                emit changePortType(GekkoFyre::AmateurRadio::GkConnType::RS232, true);
                break;
            case GkConnType::USB:
                emit changePortType(GekkoFyre::AmateurRadio::GkConnType::USB, true);
                break;
            case GkConnType::Parallel:
                emit changePortType(GekkoFyre::AmateurRadio::GkConnType::Parallel, true);
                break;
            default:
                emit changePortType(GekkoFyre::AmateurRadio::GkConnType::None, true);
                break;
            }
        }
    } else {
        //
        // PTT Mode
        //
        QString pttConnType = GkDb->read_rig_settings(radio_cfg::PttConnType);
        if (!pttConnType.isNull() || !pttConnType.isEmpty()) {
            int conv_ptt_conn_type = pttConnType.toInt();
            gk_radio_tmp->ptt_conn_type = GkDb->convConnTypeToEnum(conv_ptt_conn_type);

            switch (gk_radio_tmp->ptt_conn_type) {
            case GkConnType::RS232:
                emit changePortType(GekkoFyre::AmateurRadio::GkConnType::RS232, false);
                break;
            case GkConnType::USB:
                emit changePortType(GekkoFyre::AmateurRadio::GkConnType::USB, false);
                break;
            case GkConnType::Parallel:
                emit changePortType(GekkoFyre::AmateurRadio::GkConnType::Parallel, false);
                break;
            default:
                emit changePortType(GekkoFyre::AmateurRadio::GkConnType::None, false);
                break;
            }
        }
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
                    radio_ptr->rig_caps.reset();
                    radio_ptr->rig_caps = std::make_unique<rig_caps>(*std::get<0>(model.second));

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
    if (radio_ptr->rig != nullptr) {
        emit disconnectRigInUse(radio_ptr->rig, radio_ptr);
    }

    //
    // Attempt to initialize the amateur radio rig!
    //
    emit gatherPortType(true);
    emit gatherPortType(false);
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
void MainWindow::disconnectRigInMemory(RIG *rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr)
{
    Q_UNUSED(rig_to_disconnect);

    if (gkRadioPtr.get() != nullptr) {
        if (gkRadioPtr->rig != nullptr) {
            if (gkRadioPtr->is_open) {
                // Free the pointer(s) for the Hamlib library!
                rig_close(gkRadioPtr->rig); // Close port
                // rig_cleanup(gkRadioPtr->rig); // Cleanup memory
                gkRadioPtr->is_open = false;

                for (const auto &port: status_com_ports) {
                    //
                    // CAT Port
                    //
                    if (std::strcmp(radio_ptr->cat_conn_port.c_str(), port.port_info.port.c_str()) == 0) {
                        if (port.is_open) {
                            serial::Serial(port.port_info.port).close();
                        }
                    }

                    //
                    // PTT Port
                    //
                    if (std::strcmp(radio_ptr->ptt_conn_port.c_str(), port.port_info.port.c_str()) == 0) {
                        if (port.is_open) {
                            serial::Serial(port.port_info.port).close();
                        }
                    }
                }
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::updateRadioVarsInMem
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param radio_ptr The pointer to Hamlib's radio structure and any information thereof.
 */
void MainWindow::updateRadioVarsInMem(const std::shared_ptr<GkRadio> &radio_ptr)
{
    if (radio_ptr.get() != nullptr) {
        gkRadioPtr = std::move(radio_ptr);
    }

    return;
}

/**
 * @brief MainWindow::updateFreqsInMem Update the radio rig's used frequencies within memory, either by
 * adding or removing them from the global QVector.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param frequency The frequency to update.
 * @param digital_mode What digital mode is being used, whether it be WSPR, JT65, FT8, etc.
 * @param iaru_region The IARU Region that this particular frequency applies towards, such as ALL, R1, etc.
 * @param remove_freq Whether to remove the frequency in question from the global list or not.
 */
void MainWindow::updateFreqsInMem(const float &frequency, const GekkoFyre::AmateurRadio::DigitalModes &digital_mode,
                                  const GekkoFyre::AmateurRadio::IARURegions &iaru_region, const bool &remove_freq)
{
    GkFreqs freq;
    freq.frequency = frequency;
    freq.digital_mode = digital_mode;
    freq.iaru_region = iaru_region;
    if (remove_freq) {
        //
        // We are removing the frequency from the global std::vector!
        //
        if (!frequencyList.empty()) {
            for (size_t i = 0; i < frequencyList.size(); ++i) {
                if (gkFreqList->essentiallyEqual(frequencyList[i].frequency, freq.frequency, GK_RADIO_VFO_FLOAT_PNT_PREC)) {
                    frequencyList.erase(frequencyList.begin() + i);
                    break;
                }
            }

            frequencyList.shrink_to_fit();
        }
    } else {
        //
        // We are adding the frequency to the global std::vector!
        //
        frequencyList.reserve(1);
        frequencyList.push_back(freq);
    }

    return;
}

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

void MainWindow::on_verticalSlider_vol_control_valueChanged(int value)
{
    return;
}

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
    return;
}

void MainWindow::on_actionFM_toggled(bool arg1)
{
    return;
}

void MainWindow::on_actionSSB_toggled(bool arg1)
{
    return;
}

void MainWindow::on_actionCW_toggled(bool arg1)
{
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
