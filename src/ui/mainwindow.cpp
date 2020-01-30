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
 **   Copyright (C) 2019. GekkoFyre.
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
 **   [ 1 ] - https://git.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "dialogsettings.hpp"
#include "aboutdialog.hpp"
#include <boost/exception/all.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>
#include <functional>
#include <QWidget>
#include <QResource>
#include <QMessageBox>
#include <QPointer>
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

            // Check that the database initialized okay

            dekodeDb = std::make_shared<GekkoFyre::DekodeDb>(db, fileIo, this);
            // gkAudioDevices = std::make_shared<GekkoFyre::AudioDevices>(dekodeDb, fileIo, this);
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

        // The spectrometer must listen to the chosen *output* audio device
        Device output_audio_dev;
        for (const auto &device: pref_audio_devices) {
            if (device.is_output_dev == true) {
                output_audio_dev = device;
                break;
            }
        }

        const int spect_width_per_sec = 100;
        const int spect_win_secs = 8;
        const float spect_step_secs = 4.0;
        const int spect_win_width = (spect_width_per_sec * spect_win_secs);
        const int spect_win_height = 320;

        size_t speclen = (spect_win_height * (output_audio_dev.def_sample_rate / 20 / spect_win_height + 1));

        for (size_t i = 0; ; ++i) {
            if (gkAudioDevices->is_good_speclen(speclen + i)) {
                speclen += i;
                break;
            }

            if (speclen - i >= spect_win_height && gkAudioDevices->is_good_speclen(speclen - i)) {
                speclen -= i;
                break;
            }
        }

        Spectrum *spectrum = gkAudioDevices->createSpectrum(speclen);
        spectrum->linear_floor = std::pow(10.0, AUDIO_SPEC_FLOOR_DECIBELS / 20.0);
        spectrum->mag_to_norm = 100.0;
        spectrum->max_freq = (output_audio_dev.def_sample_rate / output_audio_dev.dev_input_channel_count);

        spectrum->min_freq = 0;
        for (const auto &sample_rate: spectrum->audio_dev.supp_sample_rates) {
            if (sample_rate > 11025.0) {
                double loop_tmp = sample_rate;
                spectrum->min_freq = std::fmin(loop_tmp, spectrum->min_freq);
            }
        }

        const float spect_total_secs = ((float)AUDIO_FRAMES_PER_BUFFER / spectrum->audio_dev.def_sample_rate);
        spectro_thread = std::thread(&AudioDevices::spectrogram, gkAudioDevices, spectrum, 100, spect_total_secs, spect_win_width, spect_win_height, spect_win_secs, spect_step_secs);
        spectro_thread.detach();
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

void MainWindow::on_actionSave_Decoded_Ab_triggered()
{
    return;
}

void MainWindow::on_actionView_Graphs_triggered()
{
    return;
}
