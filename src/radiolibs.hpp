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
#include <boost/logic/tribool.hpp>
#include <QObject>
#include <QString>
#include <QMap>
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <mutex>
#include <list>
#include <set>

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
                       std::shared_ptr<GkLevelDb> dkDb, std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> radioPtr,
                       QObject *parent = nullptr);
    ~RadioLibs();

    static int convertBaudRateInt(const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate);
    GekkoFyre::AmateurRadio::com_baud_rates convertBaudRateToEnum(const int &baud_rate_sel);
    GekkoFyre::AmateurRadio::com_baud_rates convertBaudRateIntToEnum(const int &baud_rate);
    int convertBaudRateFromEnum(const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate);
    std::list<Database::Settings::GkComPort> status_com_ports();
    QString translateBandsToStr(const AmateurRadio::bands &band);
    QString hamlibModulEnumToStr(const rmode_t &modulation);

    GekkoFyre::AmateurRadio::GkConnType convGkConnTypeToEnum(const QString &conn_type);
    rig_port_e convGkConnTypeToHamlib(const GekkoFyre::AmateurRadio::GkConnType &conn_type);

    std::shared_ptr<AmateurRadio::Control::GkRadio> gkInitRadioRig(std::shared_ptr<AmateurRadio::Control::GkRadio> radio_ptr,
                                                                   std::shared_ptr<Database::Settings::GkUsbPort> usb_ptr);

    libusb_context *initUsbLib();
    QMap<std::string, Database::Settings::GkUsbPort> enumUsbDevices(libusb_context *usb_ctx_ptr);

signals:
    void gatherPortType(const bool &is_cat_mode);

private:
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::GkLevelDb> gkDekodeDb;
    std::shared_ptr<GekkoFyre::FileIo> gkFileIo;
    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> gkRadioPtr;

    static void hamlibStatus(const int &retcode);
    static std::string getUsbPortId(libusb_device *usb_device);

    static std::string getDriver(const boost::filesystem::path &tty);
    static void probe_serial8250_comports(std::list<std::string> &comList, const std::list<std::string> &comList8250);
    static void registerComPort(std::list<std::string> &comList, std::list<std::string> &comList8250,
                                const boost::filesystem::path &dir);

    template <class T>
    void removeDuplicates(std::vector<T> &vec) {
        std::set<T> values;
        vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const T &value) { return !values.insert(value).second; }), vec.end());
    }

};
};
