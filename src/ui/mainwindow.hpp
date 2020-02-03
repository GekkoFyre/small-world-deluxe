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
#include <boost/filesystem.hpp>
#include <memory>
#include <ctime>
#include <thread>
#include <future>
#include <QMainWindow>
#include <QMultiMap>
#include <QChart>

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
    void on_actionView_Graphs_triggered();

    void infoBar();

private:
    Ui::MainWindow *ui;

    leveldb::DB *db;
    boost::filesystem::path save_db_path;
    std::shared_ptr<GekkoFyre::FileIo> fileIo;
    std::shared_ptr<GekkoFyre::DekodeDb> dekodeDb;
    std::shared_ptr<GekkoFyre::AudioDevices> gkAudioDevices;
    std::shared_ptr<GekkoFyre::PaMic> gkPaMic;
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::RadioLibs> gkRadioLibs;

    std::future<GekkoFyre::AmateurRadio::Control::Radio *> rig_thread;
    std::thread spectro_thread;

    std::vector<GekkoFyre::Database::Settings::Audio::Device> pref_audio_devices; // The configured audio devices for the user's system
    GekkoFyre::AmateurRadio::Control::Radio *radio;
    QTimer *timer;

    PaError err = paNoError;
    std::shared_ptr<PaStream> micStream;

    void radioStats(GekkoFyre::AmateurRadio::Control::Radio *radio_dev);
    void procVuMeter(const GekkoFyre::Database::Settings::Audio::Device &audio_stream);
};
