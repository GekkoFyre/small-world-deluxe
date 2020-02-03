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
#include "dialogsettings.hpp"
#include "aboutdialog.hpp"
#include "spectrodialog.hpp"
#include <boost/exception/all.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>
#include <functional>
#include <QWidget>
#include <QResource>
#include <QMessageBox>
#include <QDate>
#include <QTimer>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;

namespace fs = boost::filesystem;

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

    try {
        // Initialize QMainWindow to a full-screen after a single second!
        // QTimer::singleShot(0, this, SLOT(showFullScreen()));

        // Print out the current date
        std::cout << QDate::currentDate().toString().toStdString() << std::endl;

        fs::path slash = "/";
        fs::path native_slash = slash.make_preferred().native();
        fs::path resource_path = fs::path(QCoreApplication::applicationDirPath().toStdString() + native_slash.string()  + GekkoFyre::Filesystem::resourceFile);
        QResource::registerResource(QString::fromStdString(resource_path.string())); // https://doc.qt.io/qt-5/resources.html
        this->setWindowIcon(QIcon(":/resources/contrib/images/vector/Radio-04/Radio-04_rescaled.svg"));
        ui->actionRecord->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/vinyl-record.svg"));
        ui->actionPlay->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/play-rounded-flat.svg"));
        ui->actionSave_Decoded_Ab->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/clipboard-flat.svg"));
        ui->actionSettings->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/settings-flat.svg"));
        ui->actionView_Graphs->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_Graph-Decrease_378375.svg"));

        ui->action_Open->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_archive_226655.svg"));
        ui->actionE_xit->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_turn_off_on_power_181492.svg"));
        ui->action_Settings->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/settings-flat.svg"));
        ui->actionCheck_for_Updates->setIcon(QIcon(":/resources/contrib/images/vector/purchased/iconfinder_chemistry_226643.svg"));

        //
        // Initialize the default logic state on all applicable QPushButtons within QMainWindow.
        //
        btn_bridge_input_audio = false;
        btn_radio_rx = false;
        btn_radio_tx = false;
        btn_radio_tx_halt = false;
        btn_radio_tune = false;
        btn_radio_monitor = false;

        rig_load_all_backends();

        // Create path to file-database
        save_db_path = fs::path(fs::current_path().string() + native_slash.string() + Filesystem::fileName); // Path to save final database towards

        // Create class pointers
        fileIo = std::make_shared<GekkoFyre::FileIo>(this);
        gkStringFuncs = std::make_shared<GekkoFyre::StringFuncs>(this);

        // Some basic optimizations; this is the easiest way to get Google LevelDB to perform well
        leveldb::Options options;
        leveldb::Status status;
        options.create_if_missing = true;
        options.compression = leveldb::CompressionType::kSnappyCompression;
        options.paranoid_checks = true;

        if (!save_db_path.empty()) {
            status = leveldb::DB::Open(options, save_db_path.string(), &db);
            dekodeDb = std::make_shared<GekkoFyre::DekodeDb>(db, fileIo, this);
            gkRadioLibs = std::make_shared<GekkoFyre::RadioLibs>(fileIo, gkStringFuncs, dekodeDb, this);
        } else {
            throw std::runtime_error(tr("Unable to find settings database; we've lost its location! Aborting...").toStdString());
        }

        // Initialize the Hamlib 'radio' struct
        radio = new AmateurRadio::Control::Radio;
        std::string rand_file_name = fileIo->create_random_string(8);
        fs::path rig_file_path_tmp = fs::path(fs::temp_directory_path().string() + native_slash.string() + rand_file_name + GekkoFyre::Filesystem::tmpExtension);
        radio->rig_file = rig_file_path_tmp.string();

        QString def_com_port = gkRadioLibs->initComPorts();
        QString comDevice = dekodeDb->read_rig_settings(radio_cfg::ComDevice);
        if (!comDevice.isEmpty()) {
            def_com_port = comDevice;
        }

        QString model = dekodeDb->read_rig_settings(radio_cfg::RigModel);
        if (!model.isEmpty() || !model.isNull()) {
            radio->rig_model = model.toInt();
        } else {
            radio->rig_model = 1;
        }

        if (radio->rig_model <= 0) {
            radio->rig_model = 1;
        }

        QString com_baud_rate = dekodeDb->read_rig_settings(radio_cfg::ComBaudRate);
        AmateurRadio::com_baud_rates final_baud_rate = AmateurRadio::com_baud_rates::BAUD9600;
        if (!com_baud_rate.isEmpty() || !com_baud_rate.isNull()) {
            radio->dev_baud_rate = gkRadioLibs->convertBaudRateInt(com_baud_rate.toInt());
        } else {
            radio->dev_baud_rate = AmateurRadio::com_baud_rates::BAUD9600;
        }

        final_baud_rate = radio->dev_baud_rate;
        radio->verbosity = RIG_DEBUG_BUG;

        // Initialize sub-systems such as PortAudio and Hamlib!
        gkAudioDevices = std::make_shared<GekkoFyre::AudioDevices>(dekodeDb, fileIo, this);
        pref_audio_devices = gkAudioDevices->initPortAudio();
        radio->is_open = false;
        rig_thread = std::async(std::launch::async, &RadioLibs::init_rig, gkRadioLibs, radio->rig_model, def_com_port.toStdString(), final_baud_rate, radio->verbosity);

        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(infoBar()));
        timer->start(1000);

        std::thread t1(&MainWindow::infoBar, this);
        t1.detach();

        if (radio->freq >= 0.0) {
            ui->label_freq_large->setText(QString::number(radio->freq));
        } else {
            ui->label_freq_large->setText(tr("N/A"));
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(parent, tr("Error!"), e.what(), QMessageBox::Ok);
        QApplication::exit(-1);
    }
}

/**
 * @brief MainWindow::~MainWindow
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
MainWindow::~MainWindow()
{
    if (spectro_thread.joinable()) {
        spectro_thread.join();
    }

    err = Pa_CloseStream(&micStream);
    if (err != paNoError) {
        gkAudioDevices->portAudioErr(err);
    }

    delete db;
    delete timer;
    Pa_Terminate();

    if (radio != nullptr) {
        if (radio->is_open) {
            rig_close(radio->rig); // Close port
            rig_cleanup(radio->rig); // Cleanup memory
        }

        delete radio;
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
        QApplication::exit(0);
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
 * @brief MainWindow::procVuMeter
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_stream
 */
void MainWindow::procVuMeter(const Device &audio_stream)
{
    for (const auto &device: pref_audio_devices) {
        // gkAudioDevices->vuMeter(device.dev_output_channel_count, 1, 100, 3, );
        return;
    }
}

/**
 * @brief MainWindow::changePushButtonColor
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param push_button The QPushButton to be modified with the new QStyleSheet.
 * @param green_result Whether to make the QPushButton in question Green or Red.
 * @param color_blind_mode Not yet implemented!
 */
void MainWindow::changePushButtonColor(QPointer<QPushButton> push_button, const bool &green_result, const bool &color_blind_mode)
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
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_action_Settings_triggered
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void MainWindow::on_action_Settings_triggered()
{
    QPointer<DialogSettings> dlg_settings = new DialogSettings(dekodeDb, fileIo, nullptr, gkRadioLibs, this);
    dlg_settings->setWindowFlags(Qt::Window);
    dlg_settings->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(dlg_settings, SIGNAL(destroyed(QObject*)), this, SLOT(show()));
    dlg_settings->show();
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

void MainWindow::on_actionUSB_triggered(bool checked)
{
    ui->actionLSB->setChecked(false);
}

void MainWindow::on_actionLSB_triggered(bool checked)
{
    ui->actionUSB->setChecked(false);
}

void MainWindow::on_actionAM_triggered(bool checked)
{}

void MainWindow::on_actionFM_triggered(bool checked)
{}

void MainWindow::on_actionSSB_triggered(bool checked)
{}

void MainWindow::on_actionCW_triggered(bool checked)
{}

void MainWindow::on_actionRecord_triggered()
{}

void MainWindow::on_actionPlay_triggered()
{}

void MainWindow::on_actionSettings_triggered()
{
    QPointer<DialogSettings> dlg_settings = new DialogSettings(dekodeDb, fileIo, nullptr, gkRadioLibs, this);
    dlg_settings->setWindowFlags(Qt::Window);
    dlg_settings->setAttribute(Qt::WA_DeleteOnClose, true);
    QObject::connect(dlg_settings, SIGNAL(destroyed(QObject*)), this, SLOT(show()));
    dlg_settings->show();
}

/**
 * @brief MainWindow::infoBar
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://es.cppreference.com/w/cpp/io/manip/put_time>
 */
void MainWindow::infoBar()
{
    std::ostringstream oss_utc_time;
    oss_utc_time.imbue(std::locale(tr("en_US.utf8").toStdString()));
    std::time_t curr_time = std::time(nullptr);
    oss_utc_time << std::put_time(std::gmtime(&curr_time), "%T %p");

    std::ostringstream oss_utc_date;
    oss_utc_date.imbue(std::locale(tr("en_US.utf8").toStdString()));
    oss_utc_date << std::put_time(std::gmtime(&curr_time), "%F");

    QString curr_utc_time_str = QString::fromStdString(oss_utc_time.str());
    QString curr_utc_date_str = QString::fromStdString(oss_utc_date.str());
    ui->label_curr_utc_time->setText(curr_utc_time_str);
    ui->label_curr_utc_date->setText(curr_utc_date_str);
}

/**
 * @brief MainWindow::radioStats Displays statistics to the QMainWindow concerning the user's radio rig
 * of choice.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param radio_dev The structure containing all the information on the user's radio rig of choice.
 */
void MainWindow::radioStats(AmateurRadio::Control::Radio *radio_dev)
{
    return;
}

/**
 * @brief MainWindow::grabDefPaInputDevice returns either the chosen input device as designated by the user, or
 * the best possible choice as can be enumerated on the user's computing system.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return Hopefully an audio input device of some kind, let alone one that works.
 */
Device MainWindow::grabDefPaInputDevice()
{
    Device input_audio_dev;

    try {
        // The spectrometer and other functions must listen to the chosen *input* audio device
        for (const auto &device: pref_audio_devices) {
            if (device.is_output_dev == false) {
                input_audio_dev = device;
                break;
            }
        }
    } catch (const std::exception &e) {
        throw e.what();
    }

    return input_audio_dev;
}

void MainWindow::paMicProcBackground(const Device &input_audio_device, const int &numSamples)
{
    //
    // When the buffer is filled, new data will be written as starting at the very beginning, overwriting
    // any old data within the buffer itself
    // https://www.boost.org/doc/libs/1_72_0/doc/html/circular_buffer.html
    //
    boost::circular_buffer<double> input_dev_circ_buffer(numSamples);

    while (btn_radio_rx) {
        gkPaMic = std::make_shared<GekkoFyre::PaMic>(gkAudioDevices, this);
        input_dev_circ_buffer = gkPaMic->recordMic(input_audio_device, &micStream, true, AUDIO_MIC_INPUT_RECRD_SECS);
    }
}

void MainWindow::on_actionSave_Decoded_Ab_triggered()
{
    return;
}

void MainWindow::on_actionView_Graphs_triggered()
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
    if (!btn_radio_rx) {
        // Set the QPushButton to 'Green'
        changePushButtonColor(ui->pushButton_radio_receive, false);
        btn_radio_rx = true;

        auto input_audio_dev = grabDefPaInputDevice();
        int totalFrames = AUDIO_MIC_INPUT_RECRD_SECS * input_audio_dev.def_sample_rate;
        int numSamples = totalFrames * input_audio_dev.dev_input_channel_count;
        if (input_audio_dev.is_output_dev == boost::tribool::true_value
                || input_audio_dev.is_output_dev == boost::tribool::indeterminate_value) {
            paMicProcBackground(input_audio_dev, numSamples);
        } else {
            QMessageBox::warning(this, tr("Unavailable device!"), tr("You must firstly configure an appropriate **input** sound device."),
                                 QMessageBox::Ok);
        }
    } else {
        // Set the QPushButton to 'Red'
        changePushButtonColor(ui->pushButton_radio_receive, true);
        btn_radio_rx = false;
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

void MainWindow::on_pushButton_radio_tune_clicked()
{
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
