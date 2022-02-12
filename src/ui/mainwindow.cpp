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

#include "src/ui/mainwindow.hpp"
#include "ui_mainwindow.h"
#include "src/ui/aboutdialog.hpp"
#include "src/ui/spectrodialog.hpp"
#include "src/ui/sendreportdialog.hpp"
#include "src/ui/gkaudioplaydialog.hpp"
#include "src/ui/widgets/gk_submit_msg.hpp"
#include "src/ui/xmpp/gkxmppregistrationdialog.hpp"
#include "src/models/tableview/gk_frequency_model.hpp"
#include "src/models/tableview/gk_logger_model.hpp"
#include "src/models/tableview/gk_active_msgs_model.hpp"
#include "src/models/tableview/gk_callsign_msgs_model.hpp"
#include "src/gk_codec2.hpp"
#include <marble/AbstractFloatItem.h>
#include <marble/MarbleDirs.h>
#include <marble/GeoDataCoordinates.h>
#include <boost/chrono/chrono.hpp>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <ostream>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <functional>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QSerialPort>
#include <QMessageBox>
#include <QFileDialog>
#include <QIODevice>
#include <QMultiMap>
#include <QtGlobal>
#include <QVariant>
#include <QProcess>
#include <QWidget>
#include <QVector>
#include <QPixmap>
#include <QTimer>
#include <QtGui>
#include <QDate>
#include <QFile>
#include <QUrl>

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Mapping;
using namespace Language;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;
using namespace Network;
using namespace GkXmpp;
using namespace Security;

namespace fs = boost::filesystem;
namespace sys = boost::system;

//
// Statically declared members
//
QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, GekkoFyre::AmateurRadio::rig_type>> MainWindow::gkRadioModels = initRadioModelsVar();

std::mutex steady_timer_mtx;
std::mutex info_bar_mtx;

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
    qRegisterMetaType<GekkoFyre::Spectrograph::GkFFTSpectrum>("GekkoFyre::Spectrograph::GkFFTSpectrum");
    qRegisterMetaType<GekkoFyre::System::Events::Logging::GkEventLogging>("GekkoFyre::System::Events::Logging::GkEventLogging");
    qRegisterMetaType<GekkoFyre::System::Events::Logging::GkSeverity>("GekkoFyre::System::Events::Logging::GkSeverity");
    qRegisterMetaType<GekkoFyre::Database::Settings::Audio::GkDevice>("GekkoFyre::Database::Settings::Audio::GkDevice");
    qRegisterMetaType<GekkoFyre::AmateurRadio::GkConnType>("GekkoFyre::AmateurRadio::GkConnType");
    qRegisterMetaType<GekkoFyre::AmateurRadio::DigitalModes>("GekkoFyre::AmateurRadio::DigitalModes");
    qRegisterMetaType<GekkoFyre::AmateurRadio::IARURegions>("GekkoFyre::AmateurRadio::IARURegions");
    qRegisterMetaType<GekkoFyre::Spectrograph::GkGraphType>("GekkoFyre::Spectrograph::GkGraphType");
    qRegisterMetaType<GekkoFyre::GkAudioFramework::GkBitrate>("GekkoFyre::GkAudioFramework::Bitrate");
    qRegisterMetaType<GekkoFyre::GkAudioFramework::GkClearForms>("GekkoFyre::GkAudioFramework::GkClearForms");
    qRegisterMetaType<GekkoFyre::GkAudioFramework::CodecSupport>("GekkoFyre::GkAudioFramework::CodecSupport");
    qRegisterMetaType<GekkoFyre::Database::Settings::GkAudioSource>("GekkoFyre::Database::Settings::GkAudioSource");
    qRegisterMetaType<GekkoFyre::GkAudioFramework::GkAudioRecordStatus>("GekkoFyre::GkAudioFramework::GkAudioRecordStatus");
    qRegisterMetaType<GekkoFyre::AmateurRadio::GkFreqs>("GekkoFyre::AmateurRadio::GkFreqs");
    qRegisterMetaType<GekkoFyre::GkAudioFramework::GkAudioState>("GekkoFyre::GkAudioFramework::GkAudioState");
    qRegisterMetaType<boost::filesystem::path>("boost::filesystem::path");
    qRegisterMetaType<RIG>("RIG");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<rig_model_t>("rig_model_t");
    qRegisterMetaType<QList<QwtLegendData>>("QList<QwtLegendData>");
    qRegisterMetaType<std::vector<qint16>>("std::vector<qint16>");
    qRegisterMetaType<std::vector<double>>("std::vector<double>");
    qRegisterMetaType<std::vector<float>>("std::vector<float>");

    #ifndef GK_ENBL_VALGRIND_SUPPORT
    //
    // Initialize the QSplashScreen!
    emit setStartupProgress(0);
    #endif

    sys::error_code ec;
    fs::path slash = "/";
    native_slash = slash.make_preferred().native();

    const fs::path dir_to_append = fs::path(General::companyName + native_slash.string() + Filesystem::defaultDirAppend +
                                            native_slash.string() + Filesystem::fileName);
    const fs::path swrld_save_path = gkFileIo->defaultDirectory(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
                                                                true, QString::fromStdString(dir_to_append.string())).toStdString(); // Path to save final database towards

    try {
        // Print out the current date
        std::cout << QString("%1 v%2").arg(General::productName).arg(General::appVersion).toStdString() << std::endl;
        std::cout << tr("Time & Date: %1").arg(QDate::currentDate().toString()).toStdString() << std::endl << std::endl;

        this->window()->showMaximized();; // Maximize the window!

        //
        // Create system tray icon
        // https://doc.qt.io/qt-5/qtwidgets-desktop-systray-example.html
        //
        createTrayActions();
        createTrayIcon();
        setIcon();
        m_trayIcon->show();

        ui->actionPlay->setIcon(QIcon(":/resources/contrib/images/vector/Kameleon/Record-Player.svg"));
        ui->actionSave_Decoded_Ab->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/clipboard-flat.svg"));
        ui->actionPrint->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/printer-rounded.svg"));
        ui->actionSettings->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/settings-flat.svg"));
        ui->actionView_Spectrogram_Controller->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/graph.svg"));

        ui->action_Open->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_archive_226655.svg"));
        ui->action_Print->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/printer-rounded.svg"));
        ui->actionE_xit->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_turn_off_on_power_181492.svg"));
        ui->actionModify_Preferences->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/settings-flat.svg"));
        ui->actionCheck_for_Updates->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/update.svg"));
        ui->actionXMPP->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/xmpp.svg"));

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
        btn_radio_rx_halt = false;

        btn_radio_tune = false;
        btn_radio_monitor = false;

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
        gkFileIo = new GekkoFyre::FileIo(this);
        gkStringFuncs = new GekkoFyre::StringFuncs(this);
        gkSystem = new GkSystem(gkStringFuncs, this);

        //
        // Settings database-related logic
        //
        QString init_settings_db_loc = gkFileIo->read_initial_settings(init_cfg::DbLoc);
        QString init_settings_db_name = gkFileIo->read_initial_settings(init_cfg::DbName);

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

        #ifndef GK_ENBL_VALGRIND_SUPPORT
        emit setStartupProgress(10);
        #endif

        // Some basic optimizations; this is the easiest way to get Google LevelDB to perform well
        leveldb::Options options;
        leveldb::Status status;
        options.create_if_missing = true;
        options.compression = leveldb::CompressionType::kSnappyCompression;
        options.paranoid_checks = true;

        try {
            if (!save_db_path.empty()) {
                status = leveldb::DB::Open(options, save_db_path.string(), &db);
                const QRect main_win_coord = findActiveScreen();
                gkDb = new GekkoFyre::GkLevelDb(db, gkFileIo, gkStringFuncs, main_win_coord, this);

                bool enableSentry = false;
                bool askSentry = gkDb->read_sentry_settings(GkSentry::AskedDialog);
                if (!askSentry) {
                    QMessageBox optInMsgBox;
                    optInMsgBox.setParent(nullptr);
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
                        gkDb->write_sentry_settings(true, GkSentry::GivenConsent);
                        break;
                    case QMessageBox::No:
                        enableSentry = false;
                        gkDb->write_sentry_settings(false, GkSentry::GivenConsent);
                        break;
                    default:
                        enableSentry = false;
                        gkDb->write_sentry_settings(false, GkSentry::GivenConsent);
                        break;
                    }

                    // We have now asked the user this question at least once!
                    gkDb->write_sentry_settings(true, GkSentry::AskedDialog);
                } else {
                    enableSentry = gkDb->read_sentry_settings(GkSentry::GivenConsent);
                }

                if (enableSentry && gkSystem->isInternetAvailable()) { // Check that both Sentry is explicitly enabled and that the end-user is connected towards the Internet to begin with!
                    //
                    // Initialize Sentry!
                    // https://blog.sentry.io/2019/09/26/fixing-native-apps-with-sentry
                    // https://docs.sentry.io/platforms/native/
                    //
                    sen_opt = sentry_options_new();
                    QDir gk_minidump = QString(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" +
                            General::companyName + "/" + Filesystem::defaultDirAppend + "/" + Filesystem::gk_sentry_dump_dir);

                    const qint64 sentry_curr_epoch = QDateTime::currentMSecsSinceEpoch();
                    QDir gk_sentry_attachments = QString(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" +
                            General::companyName + "/" + Filesystem::defaultDirAppend + "/" + Filesystem::gk_sentry_dump_dir + "/" +
                            QString::number(sentry_curr_epoch) + Filesystem::gk_sentry_dump_file_ext);

                    if (!gk_minidump.exists()) {
                        gk_minidump.mkpath(QStringLiteral("."));
                    }

                    // This is where Minidumps and attachments live before upload
                    sentry_options_set_database_path(sen_opt, gk_minidump.canonicalPath().toStdString().c_str());
                    sentry_options_add_attachment(sen_opt, gk_sentry_attachments.canonicalPath().toStdString().c_str());

                    // Miscellaneous settings pertaining to Sentry
                    sentry_options_set_auto_session_tracking(sen_opt, true);
                    sentry_options_set_symbolize_stacktraces(sen_opt, true);
                    sentry_options_set_system_crash_reporter_enabled(sen_opt, true);

                    // Release information
                    sentry_options_set_environment(sen_opt, General::gk_sentry_env);
                    sentry_options_set_release(sen_opt, QString("%1@%2")
                    .arg(QString::fromStdString(General::gk_sentry_project_name))
                    .arg(QString::fromStdString(General::appVersion))
                    .toStdString().c_str());

                    // Server and URI details!
                    sentry_options_set_dsn(sen_opt, General::gk_sentry_dsn_uri);

                    // Initialize the SDK and start the Breakpad handler
                    sentry_init(sen_opt);

                    // Initialize a Unique ID for the given user on the local machine, which is much more anonymous and sanitized than
                    // dealing with IP Addresses!
                    gkDb->capture_sys_info();

                    //
                    // BUG: Workaround to fix the issue of data not uploading to Sentry server!
                    // See: https://forum.sentry.io/t/problem-with-sentry-native-c-minidumps/8878/6
                    //
                    sentry_set_transaction("init");
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
                QObject::connect(this, SIGNAL(disconnectRigInUse(Rig *, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(disconnectRigInMemory(Rig *, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));

                //
                // Initialize the Events logger
                //
                QPointer<GkEventLoggerTableViewModel> gkEventLoggerModel = new GkEventLoggerTableViewModel(gkDb, this);
                ui->tableView_maingui_logs->setModel(gkEventLoggerModel);
                ui->tableView_maingui_logs->horizontalHeader()->setVisible(true);
                ui->tableView_maingui_logs->horizontalHeader()->setSectionResizeMode(GK_EVENTLOG_TABLEVIEW_MODEL_MESSAGE_IDX, QHeaderView::Stretch);
                ui->tableView_maingui_logs->show();

                gkEventLogger = new GkEventLogger(m_trayIcon, gkStringFuncs, gkFileIo, this->winId(), this);
                QObject::connect(gkEventLogger, SIGNAL(sendEvent(const GekkoFyre::System::Events::Logging::GkEventLogging &)),
                                 gkEventLoggerModel, SLOT(insertData(const GekkoFyre::System::Events::Logging::GkEventLogging &)));
                QObject::connect(gkEventLogger, SIGNAL(removeEvent(const GekkoFyre::System::Events::Logging::GkEventLogging &)),
                                 gkEventLoggerModel, SLOT(removeData(const GekkoFyre::System::Events::Logging::GkEventLogging &)));

                gkEventLogger->publishEvent(tr("Events log initiated."), GkSeverity::Info, false, true, true, false);
                if (enableSentry) {
                    sentry_start_session();
                    sentry_reinstall_backend();
                    gkEventLogger->publishEvent(tr("Exception and crash monitoring as enabled by Sentry is now active."), GkSeverity::Info, false, true, false, false);
                }

                #ifndef GK_ENBL_VALGRIND_SUPPORT
                emit setStartupProgress(20);
                #endif

                // Initialize the Radio Database pointer!
                if (gkRadioPtr == nullptr) {
                    gkRadioPtr = std::make_shared<GkRadio>();
                }

                //
                // Initialize the all-important `GkRadioPtr`!
                //
                gkRadioLibs = new GekkoFyre::RadioLibs(gkFileIo, gkStringFuncs, gkDb, gkRadioPtr, gkEventLogger, gkSystem, this);

                //
                // Connect `GekkoFyre::GkEventLogger()` to any external log event publishing sources!
                //
                QObject::connect(gkSystem, SIGNAL(publishEventMsg(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &, const QVariant &, const bool &, const bool &, const bool &, const bool &)),
                gkEventLogger, SLOT(publishEvent(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &, const QVariant &, const bool &, const bool &, const bool &, const bool &)));
                QObject::connect(gkRadioLibs, SIGNAL(publishEventMsg(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &, const QVariant &, const bool &, const bool &, const bool &, const bool &)),
                gkEventLogger, SLOT(publishEvent(const QString &, const GekkoFyre::System::Events::Logging::GkSeverity &, const QVariant &, const bool &, const bool &, const bool &, const bool &)));

                // Initialize the other radio libraries!
                gkSerialPortMap = gkRadioLibs->filter_com_ports(gkRadioLibs->status_com_ports());
                gkUsbPortMap = gkRadioLibs->enumUsbDevices();
                gkRadioPtr = readRadioSettings();
                emit addRigInUse(gkRadioPtr->rig_model, gkRadioPtr);

                //
                // Setup the SIGNALS & SLOTS for `gkRadioLibs`...
                //
                QObject::connect(gkRadioLibs, SIGNAL(disconnectRigInUse(Rig *, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                                 this, SLOT(disconnectRigInMemory(Rig *, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
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
        gkCli = std::make_shared<GekkoFyre::GkCli>(gkCliParser, gkFileIo, gkDb, gkRadioLibs, this);

        std::unique_ptr<QString> error_msg = std::make_unique<QString>("");
        gkCli->parseCommandLine(error_msg.get());

        #ifndef GK_ENBL_VALGRIND_SUPPORT
        emit setStartupProgress(30);
        #endif

        //
        // Collect settings from QMainWindow, among other miscellaneous settings, upon termination of Small World Deluxe!
        //
        QObject::connect(this, SIGNAL(gkExitApp()), this, SLOT(uponExit()));

        //
        // Initialize the list of frequencies that Small World Deluxe needs to communicate with other users
        // throughout the globe/world!
        //
        gkFreqList = new GkFrequencies(gkDb, this);
        QObject::connect(gkFreqList, SIGNAL(addFreq(const GekkoFyre::AmateurRadio::GkFreqs &)),
                         this, SLOT(addFreqToDb(const GekkoFyre::AmateurRadio::GkFreqs &)));
        QObject::connect(gkFreqList, SIGNAL(removeFreq(const GekkoFyre::AmateurRadio::GkFreqs &)),
                         this, SLOT(removeFreqFromDb(const GekkoFyre::AmateurRadio::GkFreqs &)));

        gkFreqList->publishFreqList();
        gkAudioDevices = new GekkoFyre::GkAudioDevices(gkEventLogger, this);

        //
        // Read any previously chosen OpenAL-enumerated audio devices from the Google LevelDB database, if there are any applicable!
        const QString output_audio_device_saved = gkDb->read_audio_device_settings(GkAudioDevice::AudioOutputDeviceName);
        const QString input_audio_device_saved = gkDb->read_audio_device_settings(GkAudioDevice::AudioInputDeviceName);

        //
        // Output audio device
        std::future<void> output_audio_init = std::async(std::launch::async, [&] {
            try {
                //
                // Initialize output device!
                gkSysOutputAudioDevs = gkAudioDevices->enumerateAudioDevices(ALC_DEVICE_SPECIFIER);
                QString output_audio_dev_str;
                for (auto it = gkSysOutputAudioDevs.begin(), end = gkSysOutputAudioDevs.end(); it != end; ++it) {
                    if (!output_audio_device_saved.isEmpty()) {
                        //
                        // Enumerate output audio devices!
                        if (it->audio_dev_str == output_audio_device_saved) {
                            output_audio_dev_str = it->audio_dev_str;
                            it->pref_sample_rate = GK_AUDIO_OUTPUT_DEVICE_INIT_SAMPLE_RATE; // Use a default, hopefully universal value until we are able to initialize the device and therefore detect what is actually supported!
                            it->audio_src = GkAudioSource::Output;
                            it->al_error = 0;

                            it->alDevice = alcOpenDevice(output_audio_device_saved.toStdString().c_str());
                            if (!it->alDevice) {
                                throw std::runtime_error(tr("ERROR: Unable to initialize output audio device, \"%1\"! Out of memory?")
                                                                 .arg(output_audio_device_saved).toStdString());
                            }

                            if (gkAudioDevices->isStereoChannelSource(it->alDevice)) {
                                it->pref_audio_format = AL_FORMAT_STEREO16;
                                it->sel_channels = GkAudioChannels::Stereo;
                            } else {
                                it->pref_audio_format = AL_FORMAT_MONO16;
                                it->sel_channels = GkAudioChannels::Mono;
                            }

                            it->pref_sample_rate = gkAudioDevices->getAudioDevSampleRate(it->alDevice);
                            it->isEnabled = true;
                            it->isStreaming = false;
                        }
                    } else {
                        it->alDevice = alcOpenDevice(nullptr); // Initialize with the default audio device!
                    }

                    //
                    // Create OpenAL context for output audio device!
                    if (!alcCall(alcCreateContext, it->alDeviceCtx, it->alDevice, it->alDevice, nullptr) || !it->alDeviceCtx) {
                        it->alc_error = alcGetError(it->alDevice);
                        throw std::runtime_error(tr("ERROR: Could not create audio context for output device!").toStdString());
                    }

                    if (!alcCall(alcMakeContextCurrent, it->alDeviceCtxCurr, it->alDevice, it->alDeviceCtx) || it->alDeviceCtxCurr != ALC_TRUE) {
                        it->alc_error = alcGetError(it->alDevice);
                        throw std::runtime_error(tr("ERROR: Attempt at making the audio context current has failed for output device!").toStdString());
                    }
                }

                //
                // Start the audio output thread!
                gkAudioOutputThread.start();
                QObject::connect(&gkAudioOutputThread, &QThread::finished, gkFftAudio, &QObject::deleteLater);
                if (!output_audio_dev_str.isEmpty()) {
                    gkEventLogger->publishEvent(tr("Output audio device, \"%1\", has been initialized successfully!").arg(output_audio_dev_str), GkSeverity::Info, "", true, true, false, false, false);
                } else {
                    gkEventLogger->publishEvent(tr("The default output audio device has been initialized successfully!").arg(output_audio_dev_str), GkSeverity::Info, "", true, true, false, false, false);
                }
            } catch (const std::exception &e) {
                gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
            }
        });

        #ifndef GK_ENBL_VALGRIND_SUPPORT
        emit setStartupProgress(40);
        #endif

        //
        // Input audio device
        std::future<void> input_audio_init = std::async(std::launch::async, [&] {
            try {
                //
                // Initialize input device!
                gkSysInputAudioDevs = gkAudioDevices->enumerateAudioDevices(ALC_CAPTURE_DEVICE_SPECIFIER);

                if (!input_audio_device_saved.isEmpty()) {
                    const qint32 input_audio_dev_chosen_sample_rate = gkDb->read_misc_audio_settings(GkAudioCfg::AudioInputSampleRate).toInt();
                    const qint32 input_audio_dev_chosen_number_channels = gkDb->read_misc_audio_settings(GkAudioCfg::AudioInputChannels).toInt();
                    // const qint32 input_audio_dev_chosen_bitrate = gkDb->read_misc_audio_settings(GkAudioCfg::AudioInputBitrate).toInt();
                    const ALenum input_audio_dev_pref_audio_format = gkDb->read_misc_audio_settings(GkAudioCfg::AudioInputFormat).toInt();

                    if (input_audio_dev_chosen_sample_rate >= 8000 && input_audio_dev_chosen_sample_rate <= 192000) {
                        if (input_audio_dev_chosen_number_channels > 0) {
                            //
                            // Calculate required variables and constants!
                            const qint32 bytesPerSample = 2;
                            const qint32 safetyFactor = 2; // In order to prevent buffer overruns! Must be larger than inputBuffer to avoid the circular buffer from overwriting itself between captures...
                            audioFrameSampleCountPerChannel = GK_AUDIO_FRAME_DURATION * input_audio_dev_chosen_sample_rate / 1000;
                            audioFrameSampleCountTotal = audioFrameSampleCountPerChannel * input_audio_dev_chosen_number_channels;
                            circBufSize = audioFrameSampleCountTotal * bytesPerSample * safetyFactor;

                            //
                            // Enumerate input audio devices!
                            for (auto it = gkSysInputAudioDevs.begin(), end = gkSysInputAudioDevs.end(); it != end; ++it) {
                                if (it->audio_dev_str == input_audio_device_saved) {
                                    it->pref_sample_rate = std::abs(input_audio_dev_chosen_sample_rate); // Convert qint32 to unsigned-int!
                                    it->audio_src = GkAudioSource::Input;
                                    it->pref_audio_format = input_audio_dev_pref_audio_format;
                                    it->sel_channels = gkAudioDevices->convAudioChannelsToEnum(input_audio_dev_chosen_number_channels);
                                    it->al_error = 0;

                                    it->alDevice = alcCaptureOpenDevice(it->audio_dev_str.toStdString().c_str(), it->pref_sample_rate, it->pref_audio_format, circBufSize);
                                    if (!it->alDevice) {
                                        throw std::runtime_error(tr("ERROR: Unable to initialize input audio device, \"%1\"! Out of memory?")
                                                                         .arg(input_audio_device_saved).toStdString());
                                    }

                                    it->isEnabled = true;
                                    it->isStreaming = false;
                                } else {
                                    it->isEnabled = false;
                                    it->isStreaming = false;
                                }
                            }

                            for (auto it = gkSysInputAudioDevs.begin(), end = gkSysInputAudioDevs.end(); it != end; ++it) {
                                if (it->alDevice && it->isEnabled) {
                                    //
                                    // Begin capture of audio stream from given audio device!
                                    // NOTE: A 'context' is not required in this instance unlike an output audio device...
                                    it->alDeviceRecBuf = std::make_shared<std::vector<ALshort>>(audioFrameSampleCountTotal); // Initialize the `std::vector`!
                                    alcCaptureStart(it->alDevice);
                                    it->alc_error = alcGetError(it->alDevice);
                                    if (it->alc_error != AL_NO_ERROR) {
                                        throw std::runtime_error(tr("An issue was encountered with the capturing of data from audio device: %1")
                                                                         .arg(it->audio_dev_str).toStdString());
                                    }

                                    //
                                    // Initiate the while-loop for the capture of actual audio samples!
                                    gkSysInputDevStatus = GkAudioRecordStatus::Active;
                                    capture_input_audio_samples = std::thread(&MainWindow::captureAlcSamples, this, it->alDevice, it->alDeviceRecBuf, audioFrameSampleCountPerChannel);
                                    capture_input_audio_samples.detach();

                                    //
                                    // Start the audio input thread!
                                    QObject::connect(&gkAudioInputThread, &QThread::finished, gkFftAudio, &QObject::deleteLater);
                                    gkAudioInputThread.start();

                                    gkEventLogger->publishEvent(tr("Input audio device, \"%1\", has been initialized successfully!")
                                                                        .arg(it->audio_dev_str), GkSeverity::Info, "", true, true, false, false, false);

                                    break;
                                }
                            }
                        }
                    }
                }
            } catch (const std::exception &e) {
                gkEventLogger->publishEvent(QString::fromStdString(e.what()), GkSeverity::Fatal, "", false, true, false, true);
            }
        });

        //
        // These are blocking, therefore don't execute these until you have queued up all the needed tasks!
        output_audio_init.get();
        input_audio_init.get();

        #ifndef GK_ENBL_VALGRIND_SUPPORT
        emit setStartupProgress(50);
        #endif

        //
        // Initialize the Waterfall / Spectrograph
        //
        #ifndef ENBL_VALGRIND_SUPPORT
        gkSpectroWaterfall = new GekkoFyre::GkSpectroWaterfall(gkEventLogger, this);
        for (const auto &input_audio_dev: gkSysInputAudioDevs) {
            if (input_audio_dev.isEnabled) {
                gkFftAudio = new GekkoFyre::GkFFTAudio(input_audio_dev.alDeviceRecBuf, input_audio_dev, gkAudioDevices,
                                                       gkSpectroWaterfall, gkStringFuncs, gkEventLogger,
                                                       &gkAudioInputThread);
                break;
            }
        }

        //
        // Initialize all the SIGNALs and SLOTs for gkFftAudio!
        QObject::connect(this, SIGNAL(initSpectrograph()), gkFftAudio, SIGNAL(startRecording()), Qt::QueuedConnection);

        for (auto it = gkSysInputAudioDevs.begin(), end = gkSysInputAudioDevs.end(); it != end; ++it) {
            if (it->alDevice && it->isEnabled && !it->isStreaming) {
                //
                // Begin FFT & Spectrograph data capture!
                emit initSpectrograph();
                it->isGraphing = true;
                it->isStreaming = true;
            }
        }

        gkSpectroWaterfall->setTitle(tr("Frequency Waterfall"));
        gkSpectroWaterfall->setXLabel(tr("Frequency (kHz)"));
        gkSpectroWaterfall->setXTooltipUnit(tr("kHz"));
        gkSpectroWaterfall->setZTooltipUnit(tr("dB"));
        gkSpectroWaterfall->setYLabel(tr("Time (minutes)"), 10);
        gkSpectroWaterfall->setZLabel(tr("Signal (dB)"));
        gkSpectroWaterfall->setColorMap(ColorMaps::BlackBodyRadiation());

        //
        // Add the spectrograph / waterfall to the QMainWindow!
        ui->horizontalLayout_12->addWidget(gkSpectroWaterfall);
        #endif

        #ifndef GK_ENBL_VALGRIND_SUPPORT
        emit setStartupProgress(60);
        #endif

        //
        // Sound & Audio Devices
        //
        gkMultimedia = new GkMultimedia(gkAudioDevices, gkSysOutputAudioDevs, gkSysInputAudioDevs, gkDb, gkStringFuncs,
                                        gkEventLogger, this);
        QObject::connect(this, SIGNAL(refreshVuDisplay(const qreal &, const qreal &, const int &)),
                         gkVuMeter, SLOT(levelChanged(const qreal &, const qreal &, const int &)));
        QObject::connect(this, SIGNAL(changeInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &)),
                         this, SLOT(restartInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &)));

        //
        // Setup the volume adjustment dialog / overlay!
        QObject::connect(this, SIGNAL(changeVolume(const qint32 &)), this, SLOT(updateVolume(const qint32 &)));
        QObject::connect(this, SIGNAL(changeGlobalVolume(const qint32 &)),gkMultimedia, SLOT(changeVolume(const qint32 &)));

        //
        // Set a default value for the Volume Slider and its QLabel
        changeVolTimer = new QTimer(this);
        ui->verticalSlider_vol_control->setValue(static_cast<qint32>(GK_AUDIO_VOL_INIT_PERCENTAGE));
        emit changeVolume(ui->verticalSlider_vol_control->value());
        ui->label_vol_control_disp->setText(QString("%1%").arg(QString::number(static_cast<qint32>(GK_AUDIO_VOL_INIT_PERCENTAGE))));

        //
        // Update the volume adjustment widget from saved settings, if there are any applicable!
        const bool audio_vol_widget_checkbox_val = gkDb->intBool(gkDb->read_audio_device_settings(GkAudioDevice::AudioVolWidgetCheckboxState).toInt());
        if (audio_vol_widget_checkbox_val) {
            ui->checkBox_rx_tx_vol_toggle->setCheckState(Qt::CheckState::Checked);
            const qint32 audio_dev_input_vol = gkDb->read_audio_device_settings(GkAudioDevice::AudioInputDeviceVol).toInt();
            emit changeVolume(audio_dev_input_vol);
        } else {
            ui->checkBox_rx_tx_vol_toggle->setCheckState(Qt::CheckState::Unchecked);
            const qint32 audio_dev_output_vol = gkDb->read_audio_device_settings(GkAudioDevice::AudioOutputDeviceVol).toInt();
            emit changeVolume(audio_dev_output_vol);
        }

        //
        // QMainWindow widgets
        prefillAmateurBands();

        #ifndef GK_ENBL_VALGRIND_SUPPORT
        emit setStartupProgress(70);
        #endif

        info_timer = new QTimer(this);
        QObject::connect(info_timer, SIGNAL(timeout()), this, SLOT(infoBar()));
        info_timer->start(1000);

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

        #ifndef GK_ENBL_VALGRIND_SUPPORT
        emit setStartupProgress(80);
        #endif

        //
        // This connects `widget_mesg_outgoing` to any transmission protocols, such as Codec2!
        //
        QPointer<GkPlainTextSubmit> widget_mesg_outgoing = new GkPlainTextSubmit(ui->frame_mesg_log);
        ui->verticalLayout_3->addWidget(widget_mesg_outgoing);
        // m_spellChecker->setTextEdit(widget_mesg_outgoing);
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

        gkModem = new GkModem(gkAudioDevices, gkDb, gkEventLogger, gkStringFuncs, this);
        gkTextToSpeech = new GkTextToSpeech(gkDb, gkEventLogger, this);

        QPointer<GkActiveMsgsTableViewModel> gkActiveMsgsTableViewModel = new GkActiveMsgsTableViewModel(gkDb, this);
        QPointer<GkCallsignMsgsTableViewModel> gkCallsignMsgsTableViewModel = new GkCallsignMsgsTableViewModel(gkDb, this);
        ui->tableView_mesg_active->setModel(gkActiveMsgsTableViewModel);
        ui->tableView_mesg_callsigns->setModel(gkCallsignMsgsTableViewModel);

        //
        // Initialize mapping routines, atlas, etc.
        //
        startMappingRoutines();

        //
        // QXmpp and XMPP related
        //
        readXmppSettings();

        //
        // Initialize the QXmpp client!
        m_xmppClient = new GkXmppClient(gkConnDetails, gkDb, gkStringFuncs, gkFileIo, gkSystem, gkEventLogger, false, this);

        #ifndef GK_ENBL_VALGRIND_SUPPORT
        emit setStartupProgress(100);
        #endif
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
    if (ui->checkBox_rx_tx_vol_toggle->isChecked()) {
        //
        // Input audio device
        gkDb->write_audio_device_settings(QString::number(ui->verticalSlider_vol_control->value()), GkAudioDevice::AudioInputDeviceVol);
    } else {
        //
        // Output audio device
        gkDb->write_audio_device_settings(QString::number(ui->verticalSlider_vol_control->value()), GkAudioDevice::AudioOutputDeviceVol);
    }

    emit disconnectRigInUse(gkRadioPtr->gkRig, gkRadioPtr);
    if (gkAudioInputThread.isRunning()) {
        gkAudioInputThread.quit();
        gkAudioInputThread.wait();
    }

    if (gkAudioOutputThread.isRunning()) {
        gkAudioOutputThread.quit();
        gkAudioOutputThread.wait();
    }

    if (vu_meter_thread.joinable()) {
        vu_meter_thread.join();
    }

    if (rig_thread.joinable()) {
        rig_thread.join();
    }

    //
    // Terminate input audio (via OpenAL) and then wait a short period for it to clean itself up!
    gkSysInputDevStatus = GkAudioRecordStatus::Finished;
    std::this_thread::sleep_for(std::chrono::milliseconds(GK_EXIT_INPUT_AUDIO_TIMEOUT_MILLISECS));

    //
    // Join any input audio (via OpenAL) threads after cleanup just above!
    if (capture_input_audio_samples.joinable()) {
        capture_input_audio_samples.join();
    }

    for (auto it = gkSysOutputAudioDevs.begin(), end = gkSysOutputAudioDevs.end(); it != end; ++it) {
        if (it->alDeviceCtx && it->alc_error == AL_NO_ERROR) { // Confirm that the pointer exists and that there have been no errors otherwise!
            alcCall(alcDestroyContext, it->alDevice, it->alDeviceCtx);
        }

        if (it->alDevice && it->alc_error == AL_NO_ERROR) { // Confirm that the pointer exists and that there have been no errors otherwise!
            ALCboolean audio_output_closed;
            alcCall(alcCloseDevice, audio_output_closed, it->alDevice, it->alDevice);
        }
    }

    for (auto it = gkSysInputAudioDevs.begin(), end = gkSysInputAudioDevs.end(); it != end; ++it) {
        if (it->alDevice && it->alc_error == AL_NO_ERROR) { // Confirm that the pointer exists and that there have been no errors otherwise!
            ALCboolean audio_input_closed;
            alcCall(alcCaptureCloseDevice, audio_input_closed, it->alDevice, it->alDevice);
        }
    }

    // delete db;
    // TODO: Must fix SEGFAULT's that occur with the aforementioned line of code...

    delete ui;
}

/**
 * @brief MainWindow::on_actionXMPP_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionXMPP_triggered()
{
    try {
        launchXmppRosterDlg();
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

/**
 * @brief MainWindow::on_actionE_xit_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionE_xit_triggered()
{
    QMessageBox msgBox;
    msgBox.setParent(nullptr);
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
 * @brief MainWindow::getAmateurBands Gathers all of the requisite amateur radio bands that
 * apply to Small World Deluxe and outputs them as a QStringList().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The amateur radio bands that apply to the workings of Small World Deluxe, as a QStringList().
 */
QStringList MainWindow::getAmateurBands()
{
    try {
        QStringList bands;
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND160));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND80));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND60));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND40));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND30));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND20));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND17));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND15));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND12));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND10));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND6));
        bands.push_back(gkDb->convBandsToStr(GekkoFyre::AmateurRadio::GkFreqBands::BAND2));

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
void MainWindow::launchSettingsWin(const System::UserInterface::GkSettingsDlgTab &settingsDlgTab)
{
    QPointer<GkFreqTableModel> gkFreqTableModel = new GkFreqTableModel(gkDb, this);
    QObject::connect(gkFreqTableModel, SIGNAL(addFreq(const GekkoFyre::AmateurRadio::GkFreqs &)),
                     gkFreqList, SIGNAL(addFreq(const GekkoFyre::AmateurRadio::GkFreqs &)));
    QObject::connect(gkFreqTableModel, SIGNAL(removeFreq(const GekkoFyre::AmateurRadio::GkFreqs &)),
                     gkFreqList, SIGNAL(removeFreq(const GekkoFyre::AmateurRadio::GkFreqs &)));

    QPointer<DialogSettings> dlg_settings = new DialogSettings(gkDb, gkFileIo, gkAudioDevices,
                                                               gkSysInputAudioDevs, gkSysOutputAudioDevs, gkRadioLibs,
                                                               gkStringFuncs, gkRadioPtr, gkSerialPortMap, gkUsbPortMap,
                                                               gkFreqList, gkFreqTableModel, gkConnDetails, m_xmppClient,
                                                               gkEventLogger, m_mapWidget, gkTextToSpeech, settingsDlgTab,
                                                               this);
    dlg_settings->setWindowFlags(Qt::Window);
    dlg_settings->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(dlg_settings, SIGNAL(destroyed(QObject*)), this, SLOT(show()));

    QObject::connect(dlg_settings, SIGNAL(changeConnPort(const QString &, const GekkoFyre::AmateurRadio::GkConnMethod &)),
                     this, SIGNAL(changeConnPort(const QString &, const GekkoFyre::AmateurRadio::GkConnMethod &)));
    QObject::connect(dlg_settings, SIGNAL(recvRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                     this, SLOT(gatherRigCapabilities(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
    QObject::connect(dlg_settings, SIGNAL(addRigInUse(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)),
                     this, SLOT(addRigToMemory(const rig_model_t &, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &)));
    QObject::connect(dlg_settings, SIGNAL(updateXmppConfig()), this, SLOT(readXmppSettings()));

    //
    // Audio System related
    //
    QObject::connect(dlg_settings, SIGNAL(changeInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &)),
                     this, SLOT(restartInputAudioInterface(const GekkoFyre::Database::Settings::Audio::GkDevice &)));

    dlg_settings->show();

    return;
}

/**
 * @brief MainWindow::actionLaunchSettingsWin
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::actionLaunchSettingsWin()
{
    launchSettingsWin(System::UserInterface::GkSettingsDlgTab::GkGeneralStation);
    return;
}

/**
 * @brief MainWindow::on_actionView_World_Map_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionView_World_Map_triggered()
{
    if (!gkAtlasDlg) {
        gkAtlasDlg = new GkAtlasDialog(gkEventLogger, m_mapWidget, this);
    }

    gkAtlasDlg->setWindowFlags(Qt::Window);
    gkAtlasDlg->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(gkAtlasDlg, SIGNAL(destroyed(QObject*)), this, SLOT(show()));
    gkAtlasDlg->show();

    return;
}

/**
 * @brief MainWindow::setIcon
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 */
void MainWindow::setIcon()
{
    QIcon icon_graphic(QString(":/resources/contrib/images/vector/no-attrib/walkie-talkies.svg"));
    m_trayIcon->setIcon(icon_graphic);
    this->setWindowIcon(icon_graphic);

    return;
}

/**
 * @brief MainWindow::launchAudioPlayerWin
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::launchAudioPlayerWin()
{
    GkDevice active_output_dev;
    for (const auto &output_dev: gkSysOutputAudioDevs) {
        if (output_dev.isEnabled) {
            active_output_dev = output_dev;
        }
    }

    GkDevice active_input_dev;
    for (const auto &input_dev: gkSysInputAudioDevs) {
        if (input_dev.isEnabled) {
            active_input_dev = input_dev;
        }
    }

    //
    // Initialize the Multimedia I/O class!
    //
    QPointer<GkAudioPlayDialog> gkAudioPlayDlg = new GkAudioPlayDialog(gkDb, gkMultimedia, gkStringFuncs, gkAudioDevices, gkEventLogger, this);
    gkAudioPlayDlg->setWindowFlags(Qt::Window);
    gkAudioPlayDlg->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(gkAudioPlayDlg, SIGNAL(destroyed(QObject*)), this, SLOT(show()));

    gkAudioPlayDlg->show();
}

/**
 * @brief MainWindow::radioInitStart initializes the Hamlib library and any associated libraries/functions!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
bool MainWindow::radioInitStart()
{
    try {
        QString model = gkDb->read_rig_settings(radio_cfg::RigModel);
        if (!model.isEmpty() || !model.isNull()) {
            gkRadioPtr->rig_model = model.toInt();
        } else {
            gkRadioPtr->rig_model = 1;
        }

        if (gkRadioPtr->rig_model <= 0) {
            gkRadioPtr->rig_model = 1;
        }

        // TODO: Very important! Expand this section!
        QString com_baud_rate = gkDb->read_rig_settings_comms(radio_cfg::ComBaudRate);
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
        print_exception(e, false);
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
        const QString rigBrand = gkDb->read_rig_settings(radio_cfg::RigBrand);
        const QString rigModel = gkDb->read_rig_settings(radio_cfg::RigModel);
        const QString rigModelIndex = gkDb->read_rig_settings(radio_cfg::RigModelIndex);
        const QString rigVers = gkDb->read_rig_settings(radio_cfg::RigVersion);
        const QString comBaudRate = gkDb->read_rig_settings(radio_cfg::ComBaudRate);
        const QString stopBits = gkDb->read_rig_settings(radio_cfg::StopBits);
        const QString data_bits = gkDb->read_rig_settings(radio_cfg::DataBits);
        const QString handshake = gkDb->read_rig_settings(radio_cfg::Handshake);
        const QString force_ctrl_lines_dtr = gkDb->read_rig_settings(radio_cfg::ForceCtrlLinesDtr);
        const QString force_ctrl_lines_rts = gkDb->read_rig_settings(radio_cfg::ForceCtrlLinesRts);
        const QString ptt_method = gkDb->read_rig_settings(radio_cfg::PTTMethod);
        const QString tx_audio_src = gkDb->read_rig_settings(radio_cfg::TXAudioSrc);
        const QString ptt_mode = gkDb->read_rig_settings(radio_cfg::PTTMode);
        const QString split_operation = gkDb->read_rig_settings(radio_cfg::SplitOperation);
        const QString ptt_adv_cmd = gkDb->read_rig_settings(radio_cfg::PTTAdvCmd);

        std::shared_ptr<GkRadio> gk_radio_tmp = std::make_shared<GkRadio>();

        //
        // Setup the CAT Port
        //
        const QString comDeviceCat = gkDb->read_rig_settings_comms(radio_cfg::ComDeviceCat);
        const GkConnType catPortType = gkDb->convConnTypeToEnum(gkDb->read_rig_settings_comms(radio_cfg::ComDeviceCatPortType).toInt());
        if (!comDeviceCat.isEmpty()) {
            if (catPortType == GkConnType::GkRS232) {
                if (!gkSerialPortMap.empty()) {
                    // Verify that the port still exists!
                    for (const auto &port: gkSerialPortMap) {
                        if (comDeviceCat == port.port_info.portName()) {
                            // The port does indeed continue to exist!
                            gk_radio_tmp->cat_conn_port = port.port_info.systemLocation();
                            gkEventLogger->publishEvent(tr("Successfully found a port for making a CAT connection with (%1)!").arg(gk_radio_tmp->cat_conn_port), GkSeverity::Info);
                            break;
                        }
                    }
                }
            } else if (catPortType == GkConnType::GkUSB) {
                // USB is the connection of choice here!
                if (!gkUsbPortMap.empty()) {
                    // Verify that the port still exists!
                    for (const auto &usb: gkUsbPortMap.toStdMap()) {
                        if (comDeviceCat.toInt() == usb.first) {
                            // The port does indeed continue to exist!
                            gk_radio_tmp->cat_conn_port = gkSystem->renameCommsDevice(comDeviceCat.toInt(), GkConnType::GkUSB);
                            gk_radio_tmp->port_details.parm.usb.pid = usb.second.pid;
                            gk_radio_tmp->port_details.parm.usb.vid = usb.second.vid;

                            gkEventLogger->publishEvent(tr("Successfully found a port for making a CAT connection with (%1)!").arg(gk_radio_tmp->cat_conn_port), GkSeverity::Info);
                            break;
                        }
                    }
                }
            } else {
                // An unsupported mode, as of the moment, has been chosen so therefore issue an alert to the user!
                gkEventLogger->publishEvent(tr("Unable to find a compatible CAT connection type. Please check that your settings are valid."),
                                            GkSeverity::Warning, "", true, true);
            }
        } else {
            // No ports could be found!
            gkEventLogger->publishEvent(tr("Error with enumerating out ports for CAT connections. Please check that your settings are valid."), GkSeverity::Error);
        }

        //
        // Setup the PTT Port
        //
        QString comDevicePtt = gkDb->read_rig_settings_comms(radio_cfg::ComDevicePtt);
        GkConnType pttPortType = gkDb->convConnTypeToEnum(gkDb->read_rig_settings_comms(radio_cfg::ComDevicePttPortType).toInt());
        if (!comDevicePtt.isEmpty()) {
            if (pttPortType == GkConnType::GkRS232) {
                if (!gkSerialPortMap.empty()) {
                    // Verify that the port still exists!
                    for (const auto &port: gkSerialPortMap) {
                        if (comDevicePtt == port.port_info.portName()) {
                            // The port does indeed continue to exist!
                            gk_radio_tmp->ptt_conn_port = port.port_info.systemLocation();
                            gkEventLogger->publishEvent(tr("Successfully found a port for making a PTT connection with (%1)!").arg(gk_radio_tmp->ptt_conn_port), GkSeverity::Info);
                            break;
                        }
                    }
                }
            } else if (pttPortType == GkConnType::GkUSB) {
                // USB is the connection of choice here!
                if (!gkUsbPortMap.empty()) {
                    // Verify that the port still exists!
                    for (const auto &usb: gkUsbPortMap.toStdMap()) {
                        if (comDevicePtt.toInt() == usb.first) {
                            // The port does indeed continue to exist!
                            gk_radio_tmp->ptt_conn_port = gkSystem->renameCommsDevice(comDevicePtt.toInt(), GkConnType::GkUSB);
                            gk_radio_tmp->port_details.parm.usb.pid = usb.second.pid;
                            gk_radio_tmp->port_details.parm.usb.vid = usb.second.vid;

                            gkEventLogger->publishEvent(tr("Successfully found a port for making a PTT connection with (%1)!").arg(gk_radio_tmp->ptt_conn_port), GkSeverity::Info);
                            break;
                        }
                    }
                }
            } else {
                // An unsupported mode, as of the moment, has been chosen so therefore issue an alert to the user!
                gkEventLogger->publishEvent(tr("Unable to find a compatible PTT connection type. Please check that your settings are valid."),
                                            GkSeverity::Warning, "", true, true);
            }
        } else {
            // No ports could be found!
            gkEventLogger->publishEvent(tr("Error with enumerating out ports for PTT connections. Please check that your settings are valid."), GkSeverity::Error);
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

        if (!stopBits.isEmpty()) {
            int conv_serial_stop_bits = stopBits.toInt();
            gk_radio_tmp->port_details.parm.serial.stop_bits = conv_serial_stop_bits;
        } else {
            gk_radio_tmp->port_details.parm.serial.stop_bits = 0;
        }

        if (!data_bits.isEmpty()) {
            int conv_serial_data_bits = data_bits.toInt();
            gk_radio_tmp->port_details.parm.serial.data_bits = conv_serial_data_bits;
        } else {
            gk_radio_tmp->port_details.parm.serial.data_bits = 0;
        }

        if (!handshake.isEmpty()) {
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

        if (!force_ctrl_lines_dtr.isEmpty()) {
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

        if (!force_ctrl_lines_rts.isEmpty()) {
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

        if (!ptt_method.isEmpty()) {
            int conv_ptt_method = ptt_method.toInt();
            switch (conv_ptt_method) {
                case 0:
                    // VOX
                    gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_RIG_MICDATA; // Legacy PTT (CAT PTT), supports RIG_PTT_ON_MIC/RIG_PTT_ON_DATA
                    gk_radio_tmp->ptt_type = ptt_type_t::RIG_PTT_RIG_MICDATA;
                    break;
                case 1:
                    // DTR
                    gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_SERIAL_DTR; // PTT control through serial DTR signal
                    gk_radio_tmp->ptt_type = ptt_type_t::RIG_PTT_SERIAL_DTR;
                    break;
                case 2:
                    // CAT
                    gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_RIG; // Legacy PTT (CAT PTT)
                    gk_radio_tmp->ptt_type = ptt_type_t::RIG_PTT_RIG;
                    break;
                case 3:
                    // RTS
                    gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_SERIAL_RTS; // PTT control through serial RTS signal
                    gk_radio_tmp->ptt_type = ptt_type_t::RIG_PTT_SERIAL_RTS;
                    break;
                default:
                    // Nothing
                    gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_NONE; // No PTT available
                    gk_radio_tmp->ptt_type = ptt_type_t::RIG_PTT_NONE;
            }
        } else {
            gk_radio_tmp->port_details.type.ptt = ptt_type_t::RIG_PTT_NONE; // Default option
            gk_radio_tmp->ptt_type = ptt_type_t::RIG_PTT_NONE;
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

        if (!ptt_mode.isEmpty()) {
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

        if (!split_operation.isEmpty()) {
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

    return 1; /* != 0, we want them all! */
}

QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, rig_type>> MainWindow::initRadioModelsVar()
{
    QMultiMap<rig_model_t, std::tuple<const rig_caps *, QString, rig_type>> mmap;
    mmap.insert(-1, std::make_tuple(nullptr, "", GekkoFyre::AmateurRadio::rig_type::Unknown));
    return mmap;
}

/**
 * @brief MainWindow::captureAlcSamples for the capturing/recording of audio samples.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param device
 * @param deviceRecBuf The buffer for recording/capturing audio samples towards.
 * @param samples
 * @note Radosław Cybulski <https://stackoverflow.com/a/56651424>.
 */
void MainWindow::captureAlcSamples(ALCdevice *device, std::shared_ptr<std::vector<ALshort>> deviceRecBuf, ALCsizei samples)
{
    try {
        while (gkSysInputDevStatus == GkAudioRecordStatus::Active) {
            std::lock_guard<std::mutex> lck_guard(gkCaptureAudioSamplesMtx);
            alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &samples);
            alcCaptureSamples(device, reinterpret_cast<ALCvoid *>(deviceRecBuf->data()), samples);
        }
    } catch (const std::exception &e) {
        for (auto it = gkSysInputAudioDevs.begin(), end = gkSysInputAudioDevs.end(); it != end; ++it) {
            if (it->isStreaming) {
                gkEventLogger->publishEvent(tr("Recording from device, \"%1\", has prematurely stopped!").arg(it->audio_dev_str));
                it->isStreaming = false;
                return;
            }
        }
    }

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
        msgBoxWarn.setParent(nullptr);
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
    gkDb->remove_frequencies_db(freq_to_remove);

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
    bool freq_already_init = gkDb->isFreqAlreadyInit();
    if (!freq_already_init) {
        gkDb->write_frequencies_db(freq_to_add);
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
 * @brief
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::spectroSamplesUpdated()
{
    /*
    const qint32 n = waterfall_samples_vec.length();
    if (n > 96000) {
        waterfall_samples_vec.mid(n - gkSysInputAudioDev.audio_device_info.preferredFormat().sampleRate(), -1);
    }
     */

    return;
}

/**
 * @brief MainWindow::startMappingRoutines
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::startMappingRoutines()
{
    try {
        //
        // Configure basic settings
        Marble::MarbleDirs::setMarblePluginPath(QCoreApplication::applicationDirPath() + "/" + Filesystem::marbleDir +
                                                "/" + Filesystem::marblePlugins);
        Marble::MarbleDirs::setMarbleDataPath(QCoreApplication::applicationDirPath() + "/" + Filesystem::marbleDir +
                                              "/" + Filesystem::marbleData);

        //
        // Create the object in memory!
        m_mapWidget = new Marble::MarbleWidget();

        //
        // Load the OpenStreetMap!
        m_mapWidget->setMapThemeId(QCoreApplication::applicationDirPath() + "/" + QStringLiteral("data/maps/earth/openstreetmap/openstreetmap.dgml"));

        //
        // Enable the cloud cover and enable country borders
        m_mapWidget->setShowClouds(true);
        m_mapWidget->setShowBorders(true);

        //
        // Hide the FloatItems::Compass and FloatItems::StatusBar
        m_mapWidget->setShowOverviewMap(false);
        m_mapWidget->setShowScaleBar(false);
        m_mapWidget->setShowCompass(false);

        for (const auto &float_item: m_mapWidget->floatItems()) {
            if (float_item && float_item->nameId() == QStringLiteral("compass")) {
                //
                // Put the compass onto the LeftHand side
                float_item->setPosition(QPoint(10, 10));

                //
                // Make the content size of the compass smaller
                float_item->setContentSize(QSize(50, 50));
            }
        }

        //
        // Adjust the map and rendering quality
        m_mapWidget->setMapQualityForViewContext(Marble::HighQuality, Marble::Still);
        m_mapWidget->setMapQualityForViewContext(Marble::NormalQuality, Marble::Animation);

        // Create a horizontal zoom slider and set the default zoom
        QPointer<QSlider> zoom_slider = new QSlider(Qt::Horizontal);
        zoom_slider->setMinimum(1000);
        zoom_slider->setMaximum(2400);

        m_mapWidget->zoomView(zoom_slider->value());

        // Create a label to show the geodetic position
        QPointer<QLabel> position_label = new QLabel();
        position_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        //
        // Add all the QWidgets to the vertical layout!
        QPointer<QVBoxLayout> layout = new QVBoxLayout;
        layout->addWidget(m_mapWidget);
        layout->addWidget(zoom_slider);
        layout->addWidget(position_label);

        //
        // Centre the map onto a given position...
        Marble::GeoDataCoordinates geo_home(-60.0, -10.0, 0.0, Marble::GeoDataCoordinates::Degree);
        m_mapWidget->centerOn(geo_home);

        //
        // Connect the mapping QWidget to the position label!
        QObject::connect(m_mapWidget, SIGNAL(mouseMoveGeoPosition(QString)), position_label, SLOT(setText(QString)));
        QObject::connect(zoom_slider, SIGNAL(valueChanged(int)), m_mapWidget, SLOT(zoomView(int)));
        QObject::connect(m_mapWidget, SIGNAL(zoomChanged(int)), zoom_slider, SLOT(setValue(int)));
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

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
 * @brief MainWindow::findActiveScreen() finds the size and coordinates of the **active** screen as-is used/present by the Small
 * World Deluxe application.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The size and coordinates of the active screen that the Small World Deluxe application is presently on; this is quite
 * useful for end-user's with multi-monitor applications and we need to collect statistics for such cases.
 */
QRect MainWindow::findActiveScreen()
{
    QPointer<QScreen> screen = this->window()->windowHandle()->screen();
    return screen->availableGeometry();
}

/**
 * @brief MainWindow::createXmppConnection
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::createXmppConnection()
{
    if (!gkConnDetails.jid.isEmpty()) {
        switch (gkConnDetails.server.type) {
            case GkServerType::GekkoFyre:
                if (!gkConnDetails.password.isEmpty()) { // A password is required for GekkoFyre Networks' XMPP server!
                    m_xmppClient->createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, gkConnDetails.password, gkConnDetails.jid, false);
                }

                break;
            case GkServerType::Custom:
                if (!gkConnDetails.password.isEmpty()) { // A password is required for GekkoFyre Networks' XMPP server!
                    // Password provided
                    m_xmppClient->createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, gkConnDetails.password, gkConnDetails.jid, true);
                } else {
                    // Connecting anonymously
                    m_xmppClient->createConnectionToServer(gkConnDetails.server.url, gkConnDetails.server.port, QString(), gkConnDetails.jid, true);
                }

                break;
            default:
                break;
        }
    }

    return;
}

/**
 * @brief MainWindow::readXmppSettings reads out any saved XMPP settings that have been previously written to the
 * pre-configured Google LevelDB database.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::readXmppSettings()
{
    //
    // General --> XMPP --> Client Settings
    //
    QString xmpp_allow_msg_history = gkDb->read_xmpp_settings(GkXmppCfg::XmppAllowMsgHistory);
    QString xmpp_allow_file_xfers = gkDb->read_xmpp_settings(GkXmppCfg::XmppAllowFileXfers);
    QString xmpp_allow_mucs = gkDb->read_xmpp_settings(GkXmppCfg::XmppAllowMucs);
    QString xmpp_auto_connect = gkDb->read_xmpp_settings(GkXmppCfg::XmppAutoConnect);
    QString xmpp_auto_reconnect = gkDb->read_xmpp_settings(GkXmppCfg::XmppAutoReconnect);

    QByteArray xmpp_upload_avatar; // TODO: Finish this area of code, pronto!
    gkConnDetails.server.settings_client.upload_avatar_pixmap = xmpp_upload_avatar;

    //
    // CAUTION!!! Username, password, and e-mail address!
    //
    QString xmpp_client_username = gkDb->read_xmpp_settings(GkXmppCfg::XmppUsername);
    QString xmpp_client_password = gkDb->read_xmpp_settings(GkXmppCfg::XmppPassword);
    QString xmpp_client_email_addr = gkDb->read_xmpp_settings(GkXmppCfg::XmppEmailAddr);

    if (!xmpp_allow_msg_history.isEmpty()) {
        gkConnDetails.server.settings_client.allow_msg_history = gkDb->boolStr(xmpp_allow_msg_history.toStdString());
    } else {
        gkConnDetails.server.settings_client.allow_msg_history = true;
    }

    if (!xmpp_allow_file_xfers.isEmpty()) {
        gkConnDetails.server.settings_client.allow_file_xfers = gkDb->boolStr(xmpp_allow_file_xfers.toStdString());
    } else {
        gkConnDetails.server.settings_client.allow_file_xfers = true;
    }

    if (!xmpp_allow_mucs.isEmpty()) {
        gkConnDetails.server.settings_client.allow_mucs = gkDb->boolStr(xmpp_allow_mucs.toStdString());
    } else {
        gkConnDetails.server.settings_client.allow_mucs = true;
    }

    if (!xmpp_auto_connect.isEmpty()) {
        gkConnDetails.server.settings_client.auto_connect = gkDb->boolStr(xmpp_auto_connect.toStdString());
    } else {
        gkConnDetails.server.settings_client.auto_connect = false;
    }

    if (!xmpp_auto_reconnect.isEmpty()) {
        gkConnDetails.server.settings_client.auto_reconnect = gkDb->boolStr(xmpp_auto_reconnect.toStdString());
    } else {
        gkConnDetails.server.settings_client.auto_reconnect = false;
    }

    if (!xmpp_client_password.isEmpty()) {
        gkConnDetails.password = xmpp_client_password;
    } else {
        gkConnDetails.password = "";
    }

    if (!xmpp_client_email_addr.isEmpty()) {
        gkConnDetails.email = xmpp_client_email_addr;
    } else {
        gkConnDetails.email = "";
    }

    //
    // General --> XMPP --> Server Settings
    //
    QString xmpp_domain_url = gkDb->read_xmpp_settings(GkXmppCfg::XmppDomainUrl);
    QString xmpp_server_type = gkDb->read_xmpp_settings(GkXmppCfg::XmppServerType);
    QString xmpp_domain_port = gkDb->read_xmpp_settings(GkXmppCfg::XmppDomainPort);
    QString xmpp_enable_ssl = gkDb->read_xmpp_settings(GkXmppCfg::XmppEnableSsl);
    QString xmpp_network_timeout = gkDb->read_xmpp_settings(GkXmppCfg::XmppNetworkTimeout);
    QString xmpp_ignore_ssl_errors = gkDb->read_xmpp_settings(GkXmppCfg::XmppIgnoreSslErrors);
    QString xmpp_uri_lookup_method = gkDb->read_xmpp_settings(GkXmppCfg::XmppUriLookupMethod);

    if (xmpp_server_type.isEmpty() || xmpp_domain_url.isEmpty()) {
        gkConnDetails.server.type = gkDb->convXmppServerTypeFromInt(GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX);
        gkConnDetails.server.url = GkXmppGekkoFyreCfg::defaultUrl;
    } else {
        gkConnDetails.server.type = gkDb->convXmppServerTypeFromInt(xmpp_server_type.toInt());
        gkConnDetails.server.url = xmpp_domain_url;
    }

    if (!xmpp_client_username.isEmpty()) {
        gkConnDetails.username = xmpp_client_username;
        gkConnDetails.jid = QString(xmpp_client_username + "@" + gkConnDetails.server.url);
    } else {
        gkConnDetails.username = "";
        gkConnDetails.jid = "";
    }

    if (!xmpp_domain_port.isEmpty()) {
        gkConnDetails.server.port = xmpp_domain_port.toInt();
    } else {
        gkConnDetails.server.port = GK_DEFAULT_XMPP_SERVER_PORT;
    }

    if (!xmpp_enable_ssl.isEmpty()) {
        gkConnDetails.server.settings_client.enable_ssl = gkDb->boolStr(xmpp_enable_ssl.toStdString());
    } else {
        gkConnDetails.server.settings_client.enable_ssl = true;
    }

    if (!xmpp_network_timeout.isEmpty()) {
        gkConnDetails.server.settings_client.network_timeout = xmpp_network_timeout.toInt();
    } else {
        gkConnDetails.server.settings_client.network_timeout = 15;
    }

    if (!xmpp_ignore_ssl_errors.isEmpty()) {
        switch (xmpp_ignore_ssl_errors.toInt()) {
            case GK_XMPP_IGNORE_SSL_ERRORS_COMBO_FALSE:
                gkConnDetails.server.settings_client.ignore_ssl_errors = false;
                break;
            case GK_XMPP_IGNORE_SSL_ERRORS_COMBO_TRUE:
                gkConnDetails.server.settings_client.ignore_ssl_errors = true;
                break;
            default:
                gkConnDetails.server.settings_client.ignore_ssl_errors = false;
                break;
        }
    } else {
        gkConnDetails.server.settings_client.ignore_ssl_errors = false;
    }

    if (!xmpp_uri_lookup_method.isEmpty()) {
        switch (xmpp_uri_lookup_method.toInt()) {
            case GK_XMPP_URI_LOOKUP_DNS_SRV_METHOD:
                gkConnDetails.server.settings_client.uri_lookup_method = GkUriLookupMethod::QtDnsSrv;
                break;
            case GK_XMPP_URI_LOOKUP_MANUAL_METHOD:
                gkConnDetails.server.settings_client.uri_lookup_method = GkUriLookupMethod::Manual;
                break;
            default:
                gkConnDetails.server.settings_client.uri_lookup_method = GkUriLookupMethod::QtDnsSrv;
                break;
        }
    } else {
        gkConnDetails.server.settings_client.uri_lookup_method = GkUriLookupMethod::QtDnsSrv;
    }

    gkConnDetails.status = GkOnlineStatus::NetworkError;
    gkConnDetails.password = gkDb->read_xmpp_settings(GkXmppCfg::XmppPassword);;
    gkConnDetails.nickname = gkDb->read_xmpp_settings(GkXmppCfg::XmppNickname);;
    gkConnDetails.email = gkDb->read_xmpp_settings(GkXmppCfg::XmppEmailAddr);;

    return;
}

/**
 * @brief MainWindow::launchXmppRosterDlg launches the Roster Dialog for the XMPP side of Small World Deluxe, where end-users
 * may interact with others or even signup to the given, configured server if it's their first time connecting.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::launchXmppRosterDlg()
{
    try {
        if (!gkConnDetails.server.url.isEmpty() && !gkConnDetails.jid.isEmpty()) {
            if (!gkXmppRosterDlg) {
                m_rosterList = std::move(m_xmppClient->getRosterMap());
                gkXmppRosterDlg = new GkXmppRosterDialog(gkStringFuncs, gkConnDetails, m_xmppClient, gkDb,
                                                         gkSystem, gkEventLogger, m_rosterList, true, this);
            }

            if (gkXmppRosterDlg) { // Verify that we have successfully created the Roster dialog within memory!
                QObject::connect(this, SIGNAL(refreshDisplayedClientAvatar(const QByteArray &)),
                                 gkXmppRosterDlg, SLOT(updateDisplayedClientAvatar(const QByteArray &)));
                if (!gkXmppRosterDlg->isVisible()) { // The dialog window has not been launched yet, and whether we should show it or not!
                    gkXmppRosterDlg->setWindowFlags(Qt::Window);
                    gkXmppRosterDlg->show();
                    return;
                }
            } else {
                throw std::runtime_error(tr("Unable to create Roster dialog! Out of memory?").toStdString());
            }

            return;
        }

        launchSettingsWin(System::UserInterface::GkSettingsDlgTab::GkGeneralXmpp);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return;
}

/**
 * @brief MainWindow::createTrayActions
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::createTrayActions()
{
    m_xmppRosterAction = new QAction(tr("&XMPP"), this);
    QObject::connect(m_xmppRosterAction, &QAction::triggered, this, &MainWindow::on_actionXMPP_triggered);

    m_sstvAction = new QAction(tr("SS&TV"), this);
    QObject::connect(m_sstvAction, &QAction::triggered, this, &MainWindow::launchSstvTab);

    m_settingsAction = new QAction(tr("&Settings"), this);
    QObject::connect(m_settingsAction, &QAction::triggered, this, &MainWindow::actionLaunchSettingsWin);

    m_restoreAction = new QAction(tr("&Restore"), this);
    QObject::connect(m_restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    m_quitAction = new QAction(tr("&Quit"), this);
    QObject::connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    return;
}

/**
 * @brief MainWindow::createTrayIcon
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::createTrayIcon()
{
    m_trayIconMenu = new QMenu(this);
    m_trayIconMenu->addAction(m_xmppRosterAction);
    m_trayIconMenu->addAction(m_sstvAction);
    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_settingsAction);
    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_restoreAction);
    m_trayIconMenu->addAction(m_quitAction);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayIconMenu);

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
 * @brief MainWindow::on_actionAdjust_Volume_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionAdjust_Volume_triggered()
{
    return;
}

/**
 * @brief MainWindow::on_actionModify_Preferences_triggered is located on the drop-down menu at the top.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionModify_Preferences_triggered()
{
    launchSettingsWin(System::UserInterface::GkSettingsDlgTab::GkGeneralStation);

    return;
}

/**
 * @brief MainWindow::on_actionReset_triggered resets all the preferences back to default, after presenting a modal
 * dialog to the end-user confirming that they are sure of their decision.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionReset_triggered()
{
    try {
        QMessageBox msgBox;
        msgBox.setParent(nullptr);
        msgBox.setWindowTitle(tr("Are you sure?"));
        msgBox.setText(tr("Do you wish to reset all preferences and configuration variables back to their default values?"));
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Icon::Warning);

        int ret = msgBox.exec();
        switch (ret) {
            case QMessageBox::Ok:
            {
                QDir path_to_del = QString(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" +
                                           General::companyName + "/" + Filesystem::defaultDirAppend + "/" + Filesystem::fileName);
                if (path_to_del.exists()) {
                    //
                    // Note: need to perform the recursively remove command twice!
                    path_to_del.removeRecursively();
                    path_to_del.removeRecursively();
                    QMessageBox::information(nullptr, tr("Restart required"), tr("A restart of %1 is now required!").arg(General::productName), QMessageBox::Ok);

                    //
                    // TODO: The program doesn't seem to restart as intended, and only quits instead?
                    QString program = qApp->arguments()[0];
                    QStringList arguments = qApp->arguments().mid(1);
                    qApp->quit(); // Terminate the application!
                    QProcess::startDetached(program, arguments);
                }
            }

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
        msgBox.setParent(nullptr);
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
    launchAudioPlayerWin();
    return;
}

/**
 * @brief MainWindow::on_actionSettings_triggered is located on the toolbar towards the top itself, where the larger icons are.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionSettings_triggered()
{
    launchSettingsWin(System::UserInterface::GkSettingsDlgTab::GkGeneralStation);
    return;
}

/**
 * @brief MainWindow::infoBar
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://es.cppreference.com/w/cpp/io/manip/put_time>
 */
void MainWindow::infoBar()
{
    std::lock_guard<std::mutex> lck_guard(info_bar_mtx);

    try {
        std::ostringstream oss_utc_time;
        std::time_t curr_time = std::time(nullptr);
        oss_utc_time << std::put_time(std::gmtime(&curr_time), "%T %p");

        std::ostringstream oss_utc_date;
        oss_utc_date << std::put_time(std::gmtime(&curr_time), "%F");

        QString curr_utc_time_str = QString::fromStdString(oss_utc_time.str());
        QString curr_utc_date_str = QString::fromStdString(oss_utc_date.str());
        ui->label_curr_utc_time->setText(QString("%1 UTC+0").arg(curr_utc_time_str));
        ui->label_curr_utc_time->setToolTip(tr("Universal Time Coordinated (UTC+0)"));
        ui->label_curr_utc_date->setText(QString("%1 UTC+0").arg(curr_utc_date_str));
        ui->label_curr_utc_date->setToolTip(tr("Universal Time Coordinated (UTC+0)"));

        freq_t frequency_tmp = ((gkRadioPtr->freq / 1000) / 1000);
        if (frequency_tmp > 0.0) {
            ui->label_freq_large->setText(tr("%1 MHz").arg(QString::number(frequency_tmp)));
        } else {
            ui->label_freq_large->setText(tr("N/A"));
        }

        if (!gkRadioPtr->mode_hr.isNull() && !gkRadioPtr->mode_hr.isEmpty()) {
            ui->label_radio_rig_mode_large->setText(gkRadioPtr->mode_hr);
        }

        QString signal_strength = tr("Signal: %1 dB").arg(QString::number(gkRadioPtr->strength));
        ui->label_bandwidth_medium->setText(signal_strength);
    } catch (const std::exception &e) {
        gkEventLogger->publishEvent(tr("Error with providing radio rig statistics through the Main GUI!"),
                                    GkSeverity::Fatal, "", false, true);
    }

    return;
}

/**
 * @brief MainWindow::uponExit performs a number of functions upon exit of the application.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::uponExit()
{
    QApplication::exit(EXIT_SUCCESS);
}

/**
 * @brief MainWindow::configInputAudioDevice if no input audio device has been configured, then this function will display a QMessageBox to
 * the end-user, asking if one should be configured at the current point-in-time.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::configInputAudioDevice()
{
    QMessageBox msgBoxSettings;
    msgBoxSettings.setParent(nullptr);
    msgBoxSettings.setWindowTitle(tr("Unavailable device!"));
    msgBoxSettings.setText(tr("You must firstly configure an appropriate **input** sound device. Please select one from the settings."));
    msgBoxSettings.setStandardButtons(QMessageBox::Ok | QMessageBox::Close | QMessageBox::Cancel);
    msgBoxSettings.setDefaultButton(QMessageBox::Ok);
    msgBoxSettings.setIcon(QMessageBox::Question);
    int ret = msgBoxSettings.exec();

    switch (ret) {
        case QMessageBox::Ok:
            launchSettingsWin(System::UserInterface::GkSettingsDlgTab::GkAudioConfig);
            break;
        case QMessageBox::Close:
            QApplication::exit(EXIT_FAILURE);
            break;
        case QMessageBox::Cancel:
            break;
        default:
            break;
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
 * @brief MainWindow::on_verticalSlider_vol_control_sliderMoved adjusts the volume (or rather 'gain') of a OpenAL audio
 * source/device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param position The given volume level as determined by the QSlider itself, from a range of 0 - 100.
 * @note How to Position Audio in OpenAL <https://indiegamedev.net/2020/04/12/the-complete-guide-to-openal-with-c-part-3-positioning-sounds/>.
 */
void MainWindow::on_verticalSlider_vol_control_sliderMoved(int position)
{
    if (ui->checkBox_rx_tx_vol_toggle->isChecked()) {
        for (auto it = gkSysInputAudioDevs.begin(), end = gkSysInputAudioDevs.end(); it != end; ++it) {
            //
            // Input audio signal
            if (it->isEnabled) {
                //
                // The currently setup audio device
                it->volume = position;
                emit changeVolume(it->volume);
            }
        }
    } else {
        for (auto it = gkSysOutputAudioDevs.begin(), end = gkSysOutputAudioDevs.end(); it != end; ++it) {
            //
            // Output audio signal
            if (it->isEnabled) {
                //
                // The currently setup audio device
                it->volume = position;
                emit changeVolume(it->volume);
            }
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
    QPointer<SpectroDialog> dlg_spectro = new SpectroDialog(this);
    dlg_spectro->setWindowFlags(Qt::Tool | Qt::Dialog);
    dlg_spectro->show();

    return;
}

void MainWindow::on_pushButton_bridge_input_audio_clicked()
{
    if (!btn_bridge_input_audio) {
        // Set the QPushButton to 'Green'
        gkStringFuncs->changePushButtonColor(ui->pushButton_bridge_input_audio, false);
        btn_bridge_input_audio = true;
    } else {
        // Set the QPushButton to 'Red'
        gkStringFuncs->changePushButtonColor(ui->pushButton_bridge_input_audio, true);
        btn_bridge_input_audio = false;
    }

    return;
}

void MainWindow::on_pushButton_radio_transmit_clicked()
{
    if (!btn_radio_tx) {
        // Set the QPushButton to 'Green'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_transmit, false);
        btn_radio_tx = true;
    } else {
        // Set the QPushButton to 'Red'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_transmit, true);
        btn_radio_tx = false;
    }

    return;
}

void MainWindow::on_pushButton_radio_tx_halt_clicked()
{
    if (!btn_radio_tx_halt) {
        // Set the QPushButton to 'Green'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_tx_halt, false);
        btn_radio_tx_halt = true;
    } else {
        // Set the QPushButton to 'Red'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_tx_halt, true);
        btn_radio_tx_halt = false;
    }

    return;
}

void MainWindow::on_pushButton_radio_rx_halt_clicked()
{
    if (!btn_radio_rx_halt) {
        // Set the QPushButton to 'Green'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_rx_halt, false);
        btn_radio_rx_halt = true;
    } else {
        // Set the QPushButton to 'Red'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_rx_halt, true);
        btn_radio_rx_halt = false;
    }

    return;
}

void MainWindow::on_pushButton_radio_monitor_clicked()
{
    if (!btn_radio_monitor) {
        // Set the QPushButton to 'Green'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_monitor, false);
        btn_radio_monitor = true;
    } else {
        // Set the QPushButton to 'Red'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_monitor, true);
        btn_radio_monitor = false;
    }

    return;
}

/**
 * @brief MainWindow::on_tableView_mesg_active_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void MainWindow::on_tableView_mesg_active_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->tableView_mesg_active);
    contextMenu->exec(ui->tableView_mesg_active->mapToGlobal(pos));

    return;
}

/**
 * @brief MainWindow::on_tableView_mesg_callsigns_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void MainWindow::on_tableView_mesg_callsigns_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->tableView_mesg_callsigns);
    contextMenu->exec(ui->tableView_mesg_callsigns->mapToGlobal(pos));

    return;
}

/**
 * @brief MainWindow::on_tableView_maingui_logs_customContextMenuRequested
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pos
 */
void MainWindow::on_tableView_maingui_logs_customContextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu = std::make_unique<QMenu>(ui->tableView_maingui_logs);
    contextMenu->exec(ui->tableView_maingui_logs->mapToGlobal(pos));

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
 * @brief MainWindow::closeEvent is called upon having the Exit button in the extreme top-right being chosen.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    emit gkExitApp();
    event->accept();
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
        QMessageBox::critical(nullptr, tr("Error!"), QString::fromStdString(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::restartInputAudioInterface restarts the input audio device interface with a newly chosen audio device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param input_device The newly chosen input audio device to restart the interface with.
 */
void MainWindow::restartInputAudioInterface(const GkDevice &input_device)
{
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
                    if (std::get<0>(model.second) != nullptr) {
                        radio_ptr->capabilities.reset();
                        radio_ptr->capabilities = std::make_unique<rig_caps>(*std::get<0>(model.second));

                        return;
                    }
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
    emit disconnectRigInUse(radio_ptr->gkRig, radio_ptr);

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
void MainWindow::disconnectRigInMemory(Rig *rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr)
{
    Q_UNUSED(rig_to_disconnect);

    if (gkRadioPtr != nullptr) {
        if (gkRadioPtr->is_open) {
            // Free the pointer(s) for the Hamlib library!
            rig_to_disconnect->close(); // Close port
            gkRadioPtr->is_open = false;

            for (const auto &port: gkSerialPortMap) {
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

    return;
}

/**
 * @brief MainWindow::updateDisplayedClientAvatar updates what is displayed for the end-user's own avatar across
 * such areas as those related towards (Q)Xmpp and other, likewise areas of Small World Deluxe.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param ba_img The QByteArray data for what is to be shown/displayed on the end-user's part.
 */
void MainWindow::updateDisplayedClientAvatar(const QByteArray &ba_img)
{
    emit refreshDisplayedClientAvatar(ba_img);

    return;
}

/**
 * @brief MainWindow::launchSstvTab
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::launchSstvTab()
{
    ui->tabWidget_maingui->setCurrentWidget(ui->tab_maingui_sstv);
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
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_tune, false);
        btn_radio_tune = true;
    } else {
        // Set the QPushButton to 'Red'
        gkStringFuncs->changePushButtonColor(ui->pushButton_radio_tune, true);
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
    return;
}

/**
 * @brief MainWindow::updateVolume manages and updates any specifics regarding a change in audio volume, whether it be
 * for an input or output audio device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value The new volume value to be put into effect.
 */
void MainWindow::updateVolume(const qint32 &value)
{
    QObject::connect(changeVolTimer, &QTimer::timeout, [value, this]{ procVolumeChanges(value); });
    if (!changeVolTimer->isActive()) {
        changeVolTimer->start(GK_AUDIO_VOL_REFRESH_INTERV_DURATION);
    }

    return;
}

/**
 * @brief MainWindow::procVolumeChanges
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 */
void MainWindow::procVolumeChanges(const qint32 &value)
{
    ui->label_vol_control_disp->setText(QString("%1%").arg(QString::number(value)));
    emit changeGlobalVolume(value);

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

void MainWindow::on_actionCW_toggled(bool arg1)
{
    Q_UNUSED(arg1);

    return;
}

void MainWindow::on_actionView_Roster_triggered()
{
    try {
        launchXmppRosterDlg();
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

void MainWindow::on_actionSign_in_triggered()
{
    try {
        launchXmppRosterDlg();
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

void MainWindow::on_actionSign_out_triggered()
{
    if (m_xmppClient->isConnected()) {
        m_xmppClient->killConnectionFromServer(false);
    }

    return;
}

void MainWindow::on_action_Register_Account_triggered()
{
    QPointer<GkXmppRegistrationDialog> gkXmppRegistrationDlg = new GkXmppRegistrationDialog(GkRegUiRole::AccountCreate, gkConnDetails, m_xmppClient, gkDb, gkStringFuncs, gkEventLogger, this);
    gkXmppRegistrationDlg->setWindowFlags(Qt::Window);
    gkXmppRegistrationDlg->show();

    return;
}

void MainWindow::on_actionOnline_triggered()
{
    return;
}

void MainWindow::on_actionInvisible_triggered()
{
    return;
}

void MainWindow::on_actionOffline_triggered()
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

void MainWindow::on_pushButton_sstv_rx_navigate_left_clicked()
{
    //
    // Navigate to the next image!
    //
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

void MainWindow::on_pushButton_sstv_rx_navigate_right_clicked()
{
    //
    // Navigate to the previous image!
    //
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

void MainWindow::on_pushButton_sstv_rx_save_image_clicked()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

void MainWindow::on_pushButton_sstv_rx_listen_rx_clicked()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

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

/**
 * @brief MainWindow::on_pushButton_sstv_rx_saved_image_load_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_pushButton_sstv_rx_saved_image_load_clicked()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief MainWindow::on_pushButton_sstv_rx_saved_image_delete_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_pushButton_sstv_rx_saved_image_delete_clicked()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

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
        QPointer<GkCodec2> gkCodec2 = new GkCodec2(Codec2Mode::freeDvMode2020, Codec2ModeCustom::GekkoFyreV1, 0, 0, gkDb,
                                                   gkEventLogger, gkStringFuncs, this);
        gkCodec2->transmitData(byte_array, true);
        #endif
    } else {
        QMessageBox::information(this, tr("No image!"), tr("Please ensure to have an image loaded before attempting to make a transmission."), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::on_pushButton_sstv_rx_remove_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_pushButton_sstv_rx_remove_clicked()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief MainWindow::on_pushButton_sstv_tx_remove_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_pushButton_sstv_tx_remove_clicked()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

void MainWindow::on_action_Documentation_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

void MainWindow::on_actionSend_Report_triggered()
{
    QPointer<SendReportDialog> gkSendReport = new SendReportDialog(this);
    gkSendReport->setWindowFlags(Qt::Window);
    gkSendReport->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(gkSendReport, SIGNAL(destroyed(QObject*)), this, SLOT(show()));

    gkSendReport->show();

    return;
}

/**
 * @brief MainWindow::on_action_Battery_Calculator_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_Battery_Calculator_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief MainWindow::on_actionConnect_triggered is used for connecting to a remote instance of Small World Deluxe!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_actionConnect_triggered()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief MainWindow::print_exception
 * @param e
 * @param displayMsgBox Whether to display a QMessageBox or not to the end-user.
 * @param level
 */
void MainWindow::print_exception(const std::exception &e, const bool &displayMsgBox, int level)
{
    gkEventLogger->publishEvent(e.what(), GkSeverity::Warning, "", false);
    if (displayMsgBox) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        print_exception(e, displayMsgBox, level + 1);
    } catch (...) {}

    return;
}
