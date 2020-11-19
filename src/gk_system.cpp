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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/gk_system.hpp"
#include <exception>
#include <utility>
#include <QMessageBox>
#include <QtGlobal>

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

GkSystem::GkSystem(QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger, 
                   QObject *parent) : QObject(parent)
{
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);
}

GkSystem::~GkSystem()
{
    return;
}

/**
 * @brief GkSystem::getNumCpuCores will get the number of CPU cores on the host machine and return it as an integer, in a semi-multiplatform
 * manner.
 * @author Dirk-Jan Kroon <https://stackoverflow.com/a/3006416>.
 * @return The number of CPU cores on the host machine.
 */
qint32 GkSystem::getNumCpuCores()
{
    #ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
    #elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;

    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if(count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, nullptr, 0);
        if (count < 1) { count = 1; }
    }

    return count;
    #else
    return sysconf(_SC_NPROCESSORS_ONLN);
    #endif
}

/**
 * @brief GkSystem::renameCommsDevice adds the correct operating system path/port identifiers onto a given port number. So for
 * example, under Linux, the first port for USB becomes, `/dev/ttyUSB1`. This aids with the user in identifying the correct port
 * and also with Hamlib for making the right connection.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param port The port number that's to be transformed.
 * @param conn_type The type of connection we are dealing with.
 * @return The transmogrified port identifier.
 */
QString GkSystem::renameCommsDevice(const qint32 &port, const GkConnType &conn_type)
{
    std::stringstream ss;

    #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
    ss.clear();
    #elif __linux__
    ss << "/dev/";
    #endif

    switch (conn_type) {
        case GkRS232:
        {
            #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
            ss << "COM" << port;
            #elif __linux__
            ss << "ttyS" << port;
            #endif
            break;
        }
        case GkUSB:
        {
            #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
            ss << "USB" << port;
            #elif __linux__
            ss << "ttyUSB" << port;
            #endif
            break;
        }
        case GkParallel:
        {
            #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
            ss << "LPT" << port;
            #elif __linux__
            ss << "lp" << port;
            #endif
            break;
        }
        case GkGPIO:
        {
            #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
            ss << "DIO" << port;
            #elif __linux__
            ss.clear();
            ss << "/sys/class/gpio/gpio" << port;
            #endif
            break;
        }
        case GkCM108:
            throw std::invalid_argument(tr("CM108 is currently not supported by %1!").arg(General::productName).toStdString());
        default:
            ss << "ERROR" << port;
    }

    return QString::fromStdString(ss.str());
}
