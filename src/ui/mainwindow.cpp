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
#include <iostream>
#include <ostream>
#include <cmath>
#include <functional>
#include <chrono>
#include <QWidget>
#include <QResource>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPrintDialog>
#include <QDate>
#include <QTimer>
#include <QPixmap>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;

namespace fs = boost::filesystem;
namespace sys = boost::system;

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
    qRegisterMetaType<std::vector<GekkoFyre::Spectrograph::RawFFT>>("std::vector<GekkoFyre::Spectrograph::RawFFT>");
    qRegisterMetaType<GekkoFyre::AmateurRadio::Control::FreqChange>("GekkoFyre::AmateurRadio::Control::FreqChange");
    qRegisterMetaType<GekkoFyre::AmateurRadio::Control::SettingsChange>("GekkoFyre::AmateurRadio::Control::SettingsChange");
    qRegisterMetaType<GekkoFyre::Database::Settings::GkUsbPort>("GekkoFyre::Database::Settings::GkUsbPort");
    qRegisterMetaType<GekkoFyre::AmateurRadio::GkConnType>("GekkoFyre::AmateurRadio::GkConnType");
    qRegisterMetaType<std::vector<short>>("std::vector<short>");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<uint8_t>("uint8_t");

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

        rig_load_all_backends();

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

        if (!save_db_path.empty()) {
            status = leveldb::DB::Open(options, save_db_path.string(), &db);
            GkDb = std::make_shared<GekkoFyre::GkLevelDb>(db, fileIo, this);
            gkRadioLibs = new GekkoFyre::RadioLibs(fileIo, gkStringFuncs, GkDb, gkRadioPtr, this);
            usb_ctx_ptr = gkRadioLibs->initUsbLib();
        } else {
            QMessageBox::warning(this, tr("Error!"), tr("Unable to find settings database; we've lost its location! Aborting..."), QMessageBox::Ok);
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
        // Initialize the ability to change Connection Type, depending on whether we are connecting to the amateur radio rig
        // by USB, RS232, GPIO, etc.!
        //
        QObject::connect(this, SIGNAL(changePortType(const GkConnType &, const bool &)),
                         this, SLOT(selectedPortType(const GkConnType &, const bool &)));

        //
        // Initialize the ability to change / modify frequencies and settings relating to Hamlib
        //
        QObject::connect(this, SIGNAL(changeFreq(const bool &, const GekkoFyre::AmateurRadio::Control::FreqChange &)),
                         gkRadioLibs, SLOT(procFreqChange(const bool &, const GekkoFyre::AmateurRadio::Control::FreqChange &)));
        QObject::connect(this, SIGNAL(changeSettings(const bool &, const GekkoFyre::AmateurRadio::Control::SettingsChange &)),
                         gkRadioLibs, SLOT(procSettingsChange(const bool &, const GekkoFyre::AmateurRadio::Control::SettingsChange &)));

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

        //
        // Initialize the Hamlib radio control library!
        //
        gkRadioPtr = std::make_shared<AmateurRadio::Control::GkRadio>();

        QString def_com_port = gkRadioLibs->initComPorts();
        QString comDevice = GkDb->read_rig_settings(radio_cfg::ComDeviceCat);
        if (!comDevice.isEmpty()) {
            def_com_port = comDevice;
        }

        radioInitStart(def_com_port);

        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(infoBar()));
        timer->start(1000);

        if (!pref_audio_devices.empty()) {
            const size_t spectro_window_size = gkSpectroGui->window()->size().rwidth();
            const size_t input_audio_buffer_size = ((pref_input_device.def_sample_rate * AUDIO_BUFFER_STREAMING_SECS) *
                                              GkDb->convertAudioChannelsInt(pref_input_device.sel_channels));
            pref_input_audio_buf = new GekkoFyre::PaAudioBuf(input_audio_buffer_size, this);
            paMicProcBackground = new GekkoFyre::paMicProcBackground(gkPortAudioInit, pref_input_audio_buf, gkAudioDevices, gkStringFuncs, fileIo, GkDb,
                                                                     pref_input_device, input_audio_buffer_size, spectro_window_size, nullptr);

            //
            // Spectrograph signals and slots
            //
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
            const size_t output_audio_buffer_size = ((pref_output_device.def_sample_rate * AUDIO_BUFFER_STREAMING_SECS) *
                                              GkDb->convertAudioChannelsInt(pref_output_device.sel_channels));
            pref_output_audio_buf = new GekkoFyre::PaAudioBuf(output_audio_buffer_size, this);

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

        radioInitTest(def_com_port);

        if (gkRadioPtr->freq >= 0.0) {
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

    // Free the pointer for the libusb library!
    if (usb_ctx_ptr != nullptr) {
        libusb_exit(usb_ctx_ptr);
    }

    // Free the pointer for the Google LevelDB library!
    delete db;

    // Free the pointer for the PortAudio library!
    autoSys.terminate();
    gkPortAudioInit->terminate();

    if (gkRadioPtr->is_open) {
        // Free the pointer(s) for the Hamlib library!
        rig_close(gkRadioPtr->rig); // Close port
        rig_cleanup(gkRadioPtr->rig); // Cleanup memory
    }

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
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND160));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND80));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND60));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND40));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND30));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND20));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND17));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND15));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND12));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND10));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND6));
        bands.push_back(gkRadioLibs->translateBandsToStr(bands::BAND2));

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
                                                               gkPortAudioInit, usb_ctx_ptr, gkRadioPtr, this);
    dlg_settings->setWindowFlags(Qt::Window);
    dlg_settings->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(dlg_settings, SIGNAL(destroyed(QObject*)), this, SLOT(show()));
    QObject::connect(dlg_settings, SIGNAL(changePortType(const GkConnType &, const bool &)),
                     this, SLOT(selectedPortType(const GkConnType &, const bool &)));
    dlg_settings->show();

    return;
}

/**
 * @brief MainWindow::radioInitStart initializes the Hamlib library and any associated libraries/functions!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::radioInitStart(const QString &def_com_port)
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
            gkRadioPtr->dev_baud_rate = gkRadioLibs->convertBaudRateEnum(com_baud_rate.toInt());
        } else {
            gkRadioPtr->dev_baud_rate = AmateurRadio::com_baud_rates::BAUD9600;
        }

        AmateurRadio::com_baud_rates final_baud_rate = gkRadioPtr->dev_baud_rate;

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
            std::future_status status;
            rig_thread = std::async(std::launch::async, &RadioLibs::init_rig, gkRadioLibs, gkRadioPtr->rig_model, def_com_port.toStdString(), final_baud_rate, gkRadioPtr->verbosity);
            do {
                status = rig_thread.wait_for(std::chrono::seconds(5));
                if (status == std::future_status::timeout) {
                    throw std::runtime_error(tr("Timed-out while attempting to prepare amateur radio rig!").toStdString());
                } else if (status == std::future_status::ready) {
                    std::cout << "Amateur radio rig is ready!" << std::endl;
                }
            } while (status != std::future_status::ready);
            gkRadioPtr = rig_thread.get();
        } catch (const std::runtime_error &e) {
            QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
            return;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::radioInitTest
 * @return
 */
bool MainWindow::radioInitTest(const QString &def_com_port)
{
    try {
        if (gkRadioPtr->rig == nullptr || !gkRadioPtr->is_open) {
            throw std::runtime_error(tr("An unknown error was encountered whilst initializing the Hamlib libraries! "
                                        "We advise you to restart Small World Deluxe for proper operation.").toStdString());
        }

        if ((gkRadioPtr->freq > 0.0 || gkRadioPtr->rig_model <= 0 || gkRadioPtr->status <= 0) ||
                (def_com_port.isNull() || def_com_port.isEmpty())) {
            QMessageBox msg_rig_error;
            msg_rig_error.setWindowTitle(tr("Error!"));
            msg_rig_error.setText(tr("Small World Deluxe has experienced a rig control error!\n\nWould you like to reconfigure your settings?"));
            msg_rig_error.setStandardButtons(QMessageBox::Ok | QMessageBox::Retry | QMessageBox::Cancel);
            msg_rig_error.setDefaultButton(QMessageBox::Ok);
            msg_rig_error.setIcon(QMessageBox::Icon::Warning);
            int msg_rig_err_ret = msg_rig_error.exec();

            switch (msg_rig_err_ret) {
            case QMessageBox::Ok:
                launchSettingsWin();
                return false;
            case QMessageBox::Retry:
                radioInitStart(def_com_port);
                return radioInitTest(def_com_port);
            case QMessageBox::Cancel:
                return false;
            default:
                return false;
            }
        }

        return true;
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return false;
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
void MainWindow::radioStats(AmateurRadio::Control::GkRadio *radio_dev)
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

void MainWindow::on_comboBox_select_frequency_activated(int index)
{
    Q_UNUSED(index);
    return;
}

void MainWindow::on_comboBox_select_digital_mode_activated(int index)
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
 * @param rig_conn_type The type of connection in use for CAT and PTT modes.
 * @param is_cat_mode Whether we are modifying CAT mode or PTT instead.
 */
void MainWindow::selectedPortType(const GkConnType &rig_conn_type, const bool &is_cat_mode)
{
    if (is_cat_mode) {
        gkRadioPtr->cat_conn_type = rig_conn_type;
    } else {
        gkRadioPtr->ptt_conn_type = rig_conn_type;
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
