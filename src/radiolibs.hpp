/**
 **     __                 _ _   __    __           _     _ 
 **    / _\_ __ ___   __ _| | | / / /\ \ \___  _ __| | __| |
 **    \ \| '_ ` _ \ / _` | | | \ \/  \/ / _ \| '__| |/ _` |
 **    _\ \ | | | | | (_| | | |  \  /\  / (_) | |  | | (_| |
 **    \__/_| |_| |_|\__,_|_|_|   \/  \/ \___/|_|  |_|\__,_|
 **                                                         
 **                  ___     _                              
 **                 /   \___| |_   ___  _____               
 **                / /\ / _ \ | | | \ \/ / _ \              
 **               / /_//  __/ | |_| |>  <  __/              
 **              /___,' \___|_|\__,_/_/\_\___|              
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
#include "src/gk_logger.hpp"
#include <boost/logic/tribool.hpp>
#include <QPointer>
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
#include <Windows.h>
#endif

namespace GekkoFyre {

class RadioLibs : public QObject {
    Q_OBJECT

public:
    explicit RadioLibs(QPointer<GekkoFyre::FileIo> filePtr, std::shared_ptr<GekkoFyre::StringFuncs> stringPtr,
                       std::shared_ptr<GkLevelDb> dkDb, std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> radioPtr,
                       QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~RadioLibs();

    static int convertBaudRateInt(const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate);
    GekkoFyre::AmateurRadio::com_baud_rates convertBaudRateToEnum(const int &baud_rate_sel);
    GekkoFyre::AmateurRadio::com_baud_rates convertBaudRateIntToEnum(const int &baud_rate);
    int convertBaudRateFromEnum(const GekkoFyre::AmateurRadio::com_baud_rates &baud_rate);
    std::list<Database::Settings::GkComPort> status_com_ports();
    QString hamlibModulEnumToStr(const rmode_t &modulation);

    GekkoFyre::AmateurRadio::GkConnType convGkConnTypeToEnum(const QString &conn_type);
    rig_port_e convGkConnTypeToHamlib(const GekkoFyre::AmateurRadio::GkConnType &conn_type);

    void gkInitRadioRig(std::shared_ptr<AmateurRadio::Control::GkRadio> radio_ptr);
    qint16 calibrateAudioInputSignal(const qint16 *data_buf);

    libusb_context *initUsbLib();
    QMap<std::string, Database::Settings::GkUsbPort> enumUsbDevices(libusb_context *usb_ctx_ptr);

signals:
    void disconnectRigInUse(std::shared_ptr<Rig> rig_to_disconnect, const std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> &radio_ptr);

private:
    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    std::shared_ptr<GekkoFyre::GkLevelDb> gkDekodeDb;
    std::shared_ptr<GekkoFyre::AmateurRadio::Control::GkRadio> gkRadioPtr;
    QPointer<GekkoFyre::FileIo> gkFileIo;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    static void hamlibStatus(const int &retcode);
    static std::string getUsbPortId(libusb_device *usb_device);

    libusb_error convLibUSBErrorToEnum(const int &error);
    void print_exception(const std::exception &e, int level = 0);

    template <class T>
    void removeDuplicates(std::vector<T> &vec) {
        std::set<T> values;
        vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const T &value) { return !values.insert(value).second; }), vec.end());
    }

};
};
