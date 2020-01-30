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

#include "radiolibs.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <iostream>
#include <exception>
#include <QMessageBox>
#include <utility>
#include <cstring>

#ifdef __cplusplus
extern "C"
{
#endif

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#ifdef _WIN32
#elif __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#endif

#ifdef __cplusplus
}
#endif

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace AmateurRadio;
using namespace Control;
namespace fs = boost::filesystem;
namespace sys = boost::system;

RadioLibs::RadioLibs(std::shared_ptr<GekkoFyre::FileIo> filePtr, std::shared_ptr<StringFuncs> stringPtr,
        std::shared_ptr<DekodeDb> dkDb, QObject *parent) : QObject(parent)
{
    gkStringFuncs = std::move(stringPtr);
    gkDekodeDb = std::move(dkDb);
    gkFileIo = std::move(filePtr);
}

RadioLibs::~RadioLibs()
= default;

/**
 * @brief RadioLibs::convertBaudRateEnum Converts an enumerator to the given baud rate for a COM/Serial/RS-232 port.
 * @param baud_rate The enumerator to be converted to an integer.
 * @return The integer output from a converted enumerator.
 */
int RadioLibs::convertBaudRateEnum(const com_baud_rates &baud_rate)
{
    int ret = 0;
    switch (baud_rate) {
    case BAUD1200:
        ret = 1200;
        break;
    case BAUD2400:
        ret = 2400;
        break;
    case BAUD4800:
        ret = 4800;
        break;
    case BAUD9600:
        ret = 9600;
        break;
    case BAUD19200:
        ret = 19200;
        break;
    case BAUD38400:
        ret = 38400;
        break;
    case BAUD57600:
        ret = 57600;
        break;
    case BAUD115200:
        ret = 115200;
        break;
    default:
        ret = 1200;
        break;
    }

    return ret;
}

/**
 * @brief RadioLibs::convertBaudRateInt Converts a QComboBox selection to the given enumerator for
 * a COM/Serial/RS-232 port.
 * @param baud_rate_sel The QComboBox selection to be converted.
 * @return The enumerator that was converted from a QComboBox selection (as read from a Google LevelDB database).
 */
com_baud_rates RadioLibs::convertBaudRateInt(const int &baud_rate_sel)
{
    com_baud_rates ret = BAUD1200;
    switch (baud_rate_sel) {
    case 0:
        ret = BAUD1200;
        break;
    case 1:
        ret = BAUD2400;
        break;
    case 2:
        ret = BAUD4800;
        break;
    case 3:
        ret = BAUD9600;
        break;
    case 4:
        ret = BAUD19200;
        break;
    case 5:
        ret = BAUD38400;
        break;
    case 6:
        ret = BAUD57600;
        break;
    case 7:
        ret = BAUD115200;
        break;
    default:
        ret = BAUD1200;
        break;
    }

    return ret;
}

/**
 * @brief RadioLibs::initComPorts Finds a default communication's port to use, which is particularly useful if
 * the program has been started for the first time and the user hasn't set their preferences yet.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return A default communication's port to use.
 */
QString RadioLibs::initComPorts()
{
    QMap<tstring, std::pair<tstring, boost::tribool>> enum_com_ports = status_com_ports();
    QString filtered_port;

    for (const auto &port: enum_com_ports.toStdMap()) {
        if (!port.first.empty()) {
            if (port.second.second == true) {
                #ifdef _UNICODE
                filtered_port = QString::fromStdWString(port.first);
                #else
                filtered_port = QString::fromStdString(port.first);
                #endif
                break;
            }
        }
    }

    return filtered_port;
}

/**
 * @brief RadioLibs::initUsbPorts initializes the `libusb` library for use.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return Returns an array of possible USB devices on the user's system.
 */
std::vector<UsbPort> RadioLibs::initUsbPorts()
{
    std::vector<UsbPort> usb_ports = findUsbPorts();
    std::vector<UsbPort> filtered_ports;

    return filtered_ports;
}

/**
 * @brief RadioLibs::findUsbPorts
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param composite Please see the following document <https://github.com/pololu/libusbp/blob/master/examples/lsport/lsport.cpp>
 * @note <https://github.com/pololu/libusbp/blob/master/examples/port_name/port_name.cpp>
 * @return
 */
std::vector<UsbPort> RadioLibs::findUsbPorts()
{
    #ifdef _WIN32
    unsigned char usb_path[MAX_PATH + 1];
    #elif __linux__
    unsigned char usb_path[8 * 1024];
    #endif

    std::vector<UsbDev> usb_dev = enumUsbDevices();
    std::vector<UsbPort> usb_ports;
    for (const auto &usb: usb_dev) {
        UsbPort device;
        device.port = libusb_get_port_number(usb.dev);
        device.bus = libusb_get_bus_number(usb.dev);
        usb_ports.push_back(device);
    }

    std::vector<UsbPort> filtered_ports;
    for (const auto &device: usb_ports) {
        if (device.port > 0) {
            if (!device.usb_enum.product.empty()) {
                filtered_ports.push_back(device);
            }
        }
    }

    return filtered_ports;
}

/**
 * @brief RadioLibs::status_com_ports Checks the status of a COM/Serial port and whether it is active or not.
 * @author Michael Jacob Mathew <https://stackoverflow.com/questions/2674048/what-is-proper-way-to-detect-all-available-serial-ports-on-windows>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * Søren Holm <https://stackoverflow.com/questions/2530096/how-to-find-all-serial-devices-ttys-ttyusb-on-linux-without-opening-them/9914339#9914339>
 * @return A QMap where the COM/Serial port name itself is the key and the value is the Target Path plus a
 * Boost C++ triboolean that signifies whether the port is active or not.
 * @see GekkoFyre::RadioLibs::detect_com_ports()
 */
QMap<tstring, std::pair<tstring, boost::tribool>> RadioLibs::status_com_ports()
{
    QMap<tstring, std::pair<tstring, boost::tribool>> com_map;

    #ifdef _WIN32
    try {
        TCHAR lpTargetPath[5000]; // buffer to store the path of the COM ports
        DWORD test;
        bool gotPort = 0; // in case the COM port is not found

        for (int i = 0; i < 255; i++) {
            CString str;
            str.Format(_T("%d"), i);
            CString ComName = CString("COM") + CString(str); // Converting to COM0, COM1, COM2, etc.

            test = QueryDosDevice(ComName, (LPSTR)lpTargetPath, 5000);
            std::string key((LPCTSTR)ComName); // https://stackoverflow.com/questions/258050/how-to-convert-cstring-and-stdstring-stdwstring-to-each-other
            std::string targetPathStr(lpTargetPath, strlen(lpTargetPath + 1));

            // Test the return value and error if any
            if (test != 0) { // QueryDosDevice returns zero if it didn't find an object
                com_map.insert(key, std::make_pair(targetPathStr, true));
                gotPort = 1;
            }

            if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                // In-case the buffer got filled, increase size of the buffer.
                lpTargetPath[10000]; // TODO: Fix this hack!
                continue;
            }

            if (!gotPort) {
                // No active COM port found
                com_map.insert(key, std::make_pair("", false));
            }
        }
    } catch (const ATL::CAtlException &e) {
        // https://docs.microsoft.com/en-us/cpp/atl/reference/debugging-and-error-reporting-global-functions?view=vs-2019
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue was encountered whilst determining COM/Serial/RS-232 port status:\n\n%1")
                             .arg(QString::number(AtlHresultFromWin32(e.m_hr))), QMessageBox::Ok);
    }
    #elif __linux__
    try {
        // Scan through `/sys/class/tty` as it contains all the TTY-devices within the system
        sys::error_code ec;
        fs::path sys_dir = Filesystem::linux_sys_tty;
        std::list<std::string> comList;
        std::list<std::string> comList8250;
        auto dirent = gkFileIo->boost_dir_iterator(sys_dir, ec);

        if (ec) {
            throw std::runtime_error(ec.message());
        }

        for (const auto &device: dirent) {
            fs::path device_stem = device.stem();
            if (std::strcmp(device_stem.c_str(), "..") != 0) {
                if (std::strcmp(device_stem.c_str(), ".") != 0) {
                    // Construct full absolute file path
                    fs::path device_dir = sys_dir;
                    device_dir += device_stem;

                    // Register the device
                    registerComPort(comList, comList8250, device_dir);
                }
            }
        }

        // Only non-Serial-8250 has been added to comList without any further testing
        // Actual Serial-8250 devices must be probed to check for validity
        probe_serial8250_comports(comList, comList8250);

        for (const auto &port: comList) {
            com_map.insert(port, std::make_pair("", true));
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }
    #endif

    return com_map;
}

/**
 * @brief GekkoFyre::RadioLibs::hamlibStatus Acts upon the status code(s) returned by the HamLib library, throwing
 * an exception into the works where necessary.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param retcode The return code as given by the HamLib amateur radio library.
 */
void RadioLibs::hamlibStatus(const int &retcode)
{
    switch (retcode) {
        case RIG_OK:
            return;
        case RIG_EINVAL:
            throw std::runtime_error(tr("Invalid parameter!").toStdString());
        case RIG_ECONF:
            throw std::runtime_error(tr("Invalid configuration (serial,..)!").toStdString());
        case RIG_ENOMEM:
            throw std::runtime_error(tr("Memory shortage!").toStdString());
        case RIG_ENIMPL:
            throw std::runtime_error(tr("Function not implemented, but will be.").toStdString());
        case RIG_ETIMEOUT:
            throw std::runtime_error(tr("Communication timed out.").toStdString());
        case RIG_EIO:
            throw std::runtime_error(tr("I/O error, including open failed!").toStdString());
        case RIG_EINTERNAL:
            throw std::runtime_error(tr("Internal Hamlib error, huh!").toStdString());
        case RIG_EPROTO:
            throw std::runtime_error(tr("Protocol error!").toStdString());
        case RIG_ERJCTED:
            throw std::runtime_error(tr("Command rejected by the rig!").toStdString());
        case RIG_ETRUNC:
            throw std::runtime_error(tr("Command performed, but arg truncated!").toStdString());
        case RIG_ENAVAIL:
            throw std::runtime_error(tr("Function not available!").toStdString());
        case RIG_ENTARGET:
            throw std::runtime_error(tr("Unable to target VFO!").toStdString());
        case RIG_BUSERROR:
            throw std::runtime_error(tr("Error talking on the bus!").toStdString());
        case RIG_BUSBUSY:
            throw std::runtime_error(tr("Collision on the bus!").toStdString());
        case RIG_EARG:
            throw std::runtime_error(tr("NULL RIG handle or any invalid pointer parameter in get arg!").toStdString());
        case RIG_EVFO:
            throw std::runtime_error(tr("Invalid VFO!").toStdString());
        case RIG_EDOM:
            throw std::runtime_error(tr("Argument out of domain of function!").toStdString());
        default:
            return;
    }

    return;
}

/**
 * @brief RadioLibs::enumUsbDevices
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param devs
 * @param count
 * @note API Reference <http://libusb.sourceforge.net/api-1.0/>
 * @return
 */
std::vector<UsbDev> RadioLibs::enumUsbDevices()
{
    std::vector<UsbDev> usb_vec;
    try {
        libusb_context *context;
        libusb_device **devs;
        ssize_t cnt;
        int r;

        r = libusb_init(&context);
        if (r < 0) {
            throw std::runtime_error(tr("Unable to initialize `libusb` interface!").toStdString());
        }

        cnt = libusb_get_device_list(nullptr, &devs);
        if (cnt < 0) {
            throw std::runtime_error(tr("Unable to initialize `libusb` interface!").toStdString());
        }

        for (int i = 0; devs[i]; ++i) {
            UsbDev usb;
            usb.dev = devs[i];
            usb.handle = nullptr;
            unsigned char string[256];
            int ret;
            uint8_t j;

            usb.context = context;

            ret = libusb_get_device_descriptor(usb.dev, &usb.config);
            if (ret < 0) {
                throw std::runtime_error(tr("Unable to initialize `libusb` interface!").toStdString());
            }

            ret = libusb_open(usb.dev, &usb.handle);
            if (ret == LIBUSB_SUCCESS) {
                if (usb.config.iManufacturer) {
                    ret = libusb_get_string_descriptor_ascii(usb.handle, usb.config.iManufacturer, string, sizeof(string));
                    usb.mfg = std::string(reinterpret_cast<const char*>(string), sizeof(string));
                }

                if (usb.config.iProduct) {
                    ret = libusb_get_string_descriptor_ascii(usb.handle, usb.config.iProduct, string, sizeof(string));
                    usb.product = std::string(reinterpret_cast<const char*>(string), sizeof(string));
                }
            }

            usb_vec.push_back(usb);
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return usb_vec;
}

/**
 * @brief RadioLibs::getDriver While aimed at Linux systems, this filters away TTY-devices that do not contain
 * a `/device` subdirectory. An example of this is the `/sys/class/tty/console` device. Only the devices containing
 * this subdirectory are accepterd as valid ports and are therefore ouputted by the function.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * Søren Holm <https://stackoverflow.com/questions/2530096/how-to-find-all-serial-devices-ttys-ttyusb-on-linux-without-opening-them/9914339#9914339>
 * @param tty A TTY-device to be tested for the aforementioned constraints.
 * @return The 'base name' (or rather 'stem') of the TTY target.
 */
std::string RadioLibs::getDriver(const fs::path &tty)
{
    #ifdef __linux__
    struct stat st{};
    fs::path device_dir = tty;

    fs::path slash = "/";
    fs::path native_slash = slash.make_preferred().native();

    // Append `/device` to the TTY-path
    device_dir += native_slash.string() + "device";

    // Stat the `device_dir` and handle it of a `symlink` type
    if ((lstat(device_dir.string().c_str(), &st) == 0) && (S_ISLNK(st.st_mode))) {
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));

        // Append `/driver` and return just the stem of the target
        device_dir += native_slash.string() + "driver";
        if (readlink(device_dir.string().c_str(), buffer, sizeof(buffer)) > 0) {
            return fs::path(buffer).stem().string();
        }
    }
    #elif _WIN32
    Q_UNUSED(tty);
    #endif

    return "";
}

/**
 * @brief RadioLibs::probe_serial8250_comports Gathers a list of all the open TTY-devices.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * Søren Holm <https://stackoverflow.com/questions/2530096/how-to-find-all-serial-devices-ttys-ttyusb-on-linux-without-opening-them/9914339#9914339>
 * @param comList Outputted std::list<std::string> of active TTY-devices.
 * @param comList8250 A std::list<std::string> of TTY-devices to test.
 */
void RadioLibs::probe_serial8250_comports(std::list<std::string> &comList, const std::list<std::string> &comList8250)
{
    #ifdef __linux__
    struct serial_struct ser_info;
    auto it = comList8250.begin();

    // Iterate over all Serial-8250 devices
    while (it != comList8250.end()) {
        // Try and open the device
        int fd = open((*it).c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);
        if (fd >= 0) {
            // Get serial_info
            if (ioctl(fd, TIOCGSERIAL, &ser_info) == 0) {
                // If device type is not `PORT_UNKNOWN` then we accept the port
                if (ser_info.type != PORT_UNKNOWN) {
                    comList.push_back(*it);
                }
            }

            close(fd);
        }

        ++it;
    }
    #elif _WIN32
    Q_UNUSED(comList);
    Q_UNUSED(comList8250);
    #endif

    return;
}

/**
 * @brief GekkoFyre::RadioLibs::registerComPort
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * Søren Holm <https://stackoverflow.com/questions/2530096/how-to-find-all-serial-devices-ttys-ttyusb-on-linux-without-opening-them/9914339#9914339>
 * @param comList
 * @param comList8250
 * @param dir
 */
void RadioLibs::registerComPort(std::list<std::string> &comList, std::list<std::string> &comList8250,
        const fs::path &dir)
{
    // Get the driver the device is using
    std::string driver = getDriver(dir);

    // Skip devices without a driver
    if (!driver.empty()) {
        fs::path dev_file = fs::path("/dev/").string() + dir.stem().string();

        // Put Serial-8250 Devices in a separate list
        if (driver == "serial8250") {
            comList8250.push_back(dev_file.string());
        } else {
            comList.push_back(dev_file.string());
        }
    }

    return;
}

/**
 * @brief RadioLibs::init_rig Initializes the struct, `GekkoFyre::AmateurRadio::Control::radio`, and all of the values within.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param rig Holds the variables needed to initialize control of an amateur radio rig via HamLib.
 * @param rig_model The specific model of the radio rig in question.
 * @param com_baud_rate The BAUD Rate of the desired COM Port.
 * @param verbosity The kind of errors you wish for HamLib to report, whether they be at a debug level or only critical errors.
 * @note Ref: HamLib <https://github.com/Hamlib/Hamlib/>.
 */
Radio *RadioLibs::init_rig(const rig_model_t &rig_model, const std::string &com_port,
                           const com_baud_rates &com_baud_rate, const rig_debug_level_e &verbosity)
{
    Radio *radio = new Radio;

    // https://github.com/Hamlib/Hamlib/blob/master/tests/example.c
    // Set verbosity level
    rig_set_debug(verbosity);

    // Instantiate the rig
    radio->rig = rig_init(rig_model);
    rig_debug(verbosity, "Backend version: %s, Status: %s\n\n", radio->rig->caps->version, rig_strstatus(radio->rig->caps->status));

    if (radio->rig == nullptr) {
        throw std::runtime_error(tr("Unable to initialize Hamlib!").toStdString());
    }

    // Setup serial port, baud rate, etc.
    fs::path slashes = "//./";
    fs::path native_slash = slashes.make_preferred().native();
    fs::path com_port_path;
    #ifdef _WIN32
    com_port_path = fs::path(slashes.string() + com_port);
    #elif __linux__
    com_port_path = com_port;
    #endif

    radio->rig_file = com_port_path.string(); // TODO: Replace this with real values, as this is only an example for now...

    strncpy(radio->rig->state.rigport.pathname, radio->rig_file.c_str(), FILPATHLEN - 1);

    int baud_rate = 9600;
    baud_rate = convertBaudRateEnum(com_baud_rate);

    radio->rig->state.rigport.parm.serial.rate = baud_rate; // The BAUD Rate for the desired COM Port

    // Open our rig in question
    int retcode = rig_open(radio->rig);
    hamlibStatus(retcode);

    radio->is_open = true; // Set the flag that the aforementioned pointer has been initialized

    if (rig_get_info(radio->rig) != nullptr) {
        // Give me ID info, e.g., firmware version
        radio->info_buf = rig_get_info(radio->rig);
        std::cout << tr("Rig info: %1\n\n").arg(QString::fromStdString(radio->info_buf)).toStdString();
    }

    // Note from Hamlib Developers: As a general practice, we should check to see if a given function
    // is within the rig's capabilities before calling it, but we are simplifying here. Also, we should
    // check each call's returned status in case of error.

    // Main VFO frequency
    radio->status = rig_get_freq(radio->rig, RIG_VFO_CURR, &radio->freq);
    std::cout << tr("Main VFO Frequency: %1\n\n").arg(QString::number(radio->freq)).toStdString();

    // Current mode
    radio->status = rig_get_mode(radio->rig, RIG_VFO_CURR, &radio->mode, &radio->width);

    // NOTE: These are meant to be translatable!
    switch (radio->mode) {
    case RIG_MODE_USB:
        radio->mm = tr("USB").toStdString();
        break;
    case RIG_MODE_LSB:
        radio->mm = tr("LSB").toStdString();
        break;
    case RIG_MODE_CW:
        radio->mm = tr("CW").toStdString();
        break;
    case RIG_MODE_CWR:
        radio->mm = tr("CWR").toStdString();
        break;
    case RIG_MODE_AM:
        radio->mm = tr("AM").toStdString();
        break;
    case RIG_MODE_FM:
        radio->mm = tr("FM").toStdString();
        break;
    case RIG_MODE_WFM:
        radio->mm = tr("WFM").toStdString();
        break;
    case RIG_MODE_RTTY:
        radio->mm = tr("RTTY").toStdString();
        break;
    default:
        radio->mm = tr("Unrecongized").toStdString();
        break;
    }

    std::cout << tr("Current mode: %1, width is %2\n\n").arg(QString::fromStdString(radio->mm)).arg(QString::number(radio->width)).toStdString();

    // Rig power output
    radio->status = rig_get_level(radio->rig, RIG_VFO_CURR, RIG_LEVEL_RFPOWER, &radio->power);
    std::cout << tr("RF Power relative setting: %%1 (0.0 - 1.0)\n\n").arg(QString::number(radio->power.f)).toStdString();

    // Convert power reading to watts
    radio->status = rig_power2mW(radio->rig, &radio->mwpower, radio->power.f, radio->freq, radio->mode);
    std::cout << tr("RF Power calibrated: %1 watts\n\n").arg(QString::number(radio->mwpower / 1000)).toStdString();

    // Raw and calibrated S-meter values
    radio->status = rig_get_level(radio->rig, RIG_VFO_CURR, RIG_LEVEL_RAWSTR, &radio->raw_strength);
    std::cout << tr("Raw receive strength: %1\n\n").arg(QString::number(radio->raw_strength.i)).toStdString();

    radio->isz = radio->rig->caps->str_cal.size; // TODO: No idea what this is for?

    radio->status = rig_get_strength(radio->rig, RIG_VFO_CURR, &radio->strength);

    return radio;
}
