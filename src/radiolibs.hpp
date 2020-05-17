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
#include "src/dek_db.hpp"
#include "src/radiolibs.hpp"
#include <QObject>
#include <QString>
#include <QMap>
#include <boost/logic/tribool.hpp>
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <list>

#ifdef _WIN32
#include "src/string_funcs_windows.hpp"
#elif __linux__
#include "src/string_funcs_linux.hpp"
#endif

namespace GekkoFyre {

class RadioLibs : public QObject {
    Q_OBJECT

public:
    explicit RadioLibs(std::shared_ptr<GekkoFyre::FileIo> filePtr, std::shared_ptr<GekkoFyre::StringFuncs> stringPtr,
            std::shared_ptr<GkLevelDb> dkDb, QObject *parent = nullptr);
    ~RadioLibs() override;

    static int convertBaudRateEnum(const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate);
    GekkoFyre::AmateurRadio::com_baud_rates convertBaudRateInt(const int &baud_rate_sel);
    QString initComPorts();
    std::vector<Database::Settings::UsbPort> initUsbPorts();
    std::vector<Database::Settings::UsbPort> findUsbPorts();
    QMap<tstring, std::pair<tstring, boost::tribool>> status_com_ports();
    AmateurRadio::Control::Radio *init_rig(const rig_model_t &rig_model, const std::string &com_port,
                                           const GekkoFyre::AmateurRadio::com_baud_rates &com_baud_rate,
                                           const rig_debug_level_e &verbosity);
    QString translateBandsToStr(const AmateurRadio::bands &band);

public slots:
    void procFreqChange(const bool &radio_locked, const GekkoFyre::AmateurRadio::Control::FreqChange &freq_change);

private:
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::GkLevelDb> gkDekodeDb;
    std::shared_ptr<GekkoFyre::FileIo> gkFileIo;

    static void hamlibStatus(const int &retcode);

    std::vector<Database::Settings::UsbDev> enumUsbDevices();
    static std::string getDriver(const boost::filesystem::path &tty);
    static void probe_serial8250_comports(std::list<std::string> &comList, const std::list<std::string> &comList8250);
    static void registerComPort(std::list<std::string> &comList, std::list<std::string> &comList8250,
            const boost::filesystem::path &dir);

};
};
