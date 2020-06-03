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

#include "radiolibs.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <ios>
#include <iostream>
#include <exception>
#include <utility>
#include <cstring>
#include <sstream>
#include <QMessageBox>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libusb.h>

#if defined(_MSC_VER) && (_MSC_VER > 1900)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#elif __linux__
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
                     std::shared_ptr<GkLevelDb> dkDb, std::shared_ptr<GkRadio> radioPtr, QObject *parent) : QObject(parent)
{
    gkStringFuncs = std::move(stringPtr);
    gkDekodeDb = std::move(dkDb);
    gkFileIo = std::move(filePtr);
    gkRadioPtr = std::move(radioPtr);
}

RadioLibs::~RadioLibs()
{}

/**
 * @brief RadioLibs::convertBaudRateEnum Converts an enumerator to the given baud rate for a COM/Serial/RS-232 port.
 * @param baud_rate The enumerator to be converted to an integer.
 * @return The integer output from a converted enumerator.
 */
int RadioLibs::convertBaudRateInt(const com_baud_rates &baud_rate)
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
com_baud_rates RadioLibs::convertBaudRateEnum(const int &baud_rate_sel)
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
 * @brief RadioLibs::initUsbPorts initializes the `libusb` library for use.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return Returns an array of possible USB devices on the user's system.
 */
libusb_context *RadioLibs::initUsbLib()
{
    libusb_context *context;
    int r;

    r = libusb_init(&context);
    if (r < 0) {
        throw std::runtime_error(tr("Unable to initialize `libusb` interface!").toStdString());
    }

    return context;
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

    #if defined(_MSC_VER) && (_MSC_VER > 1900)
    #ifdef _WIN32
    try {
        TCHAR lpTargetPath[5000]; // buffer to store the path of the COM ports
        DWORD test;
        bool gotPort = false; // in case the COM port is not found

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
                gotPort = true;
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
    #endif
    #elif __linux__ || __MINGW32__
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
            if (std::strcmp(device_stem.string().c_str(), "..") != 0) {
                if (std::strcmp(device_stem.string().c_str(), ".") != 0) {
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
 * @brief RadioLibs::getUsbPortId gets the Port Identification number for the given USB Device.
 * @author Unknown <https://cpp.hotexamples.com/examples/-/-/libusb_get_bus_number/cpp-libusb_get_bus_number-function-examples.html>
 * @param usb_device The given USB Device to query.
 * @return The Port Identification number for the given USB Device.
 */
std::string RadioLibs::getUsbPortId(libusb_device *usb_device)
{
    try {
        auto usb_bus = std::to_string(libusb_get_bus_number(usb_device));

        // As per the USB 3.0 specs, the current maximum limit for the depth is 7...
        const auto max_usb_depth = 8;
        uint8_t usb_ports[max_usb_depth] = {};
        std::stringstream port_path;
        auto port_count = libusb_get_port_numbers(usb_device, usb_ports, max_usb_depth);
        auto usb_dev = std::to_string(libusb_get_device_address(usb_device));
        for (size_t i = 0; i < port_count; ++i) {
            port_path << std::to_string(usb_ports[i]) << (((i + 1) < port_count) ? "." : "");
        }

        return usb_bus + "-" + port_path.str() + "-" + usb_dev;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return "";
}

/**
 * @brief RadioLibs::enumUsbDevices will enumerate out any USB devices present/connected on the user's computer system, giving valuable
 * details for each device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param usb_ctx_ptr Required for `libusb` library (de-)initialization.
 * @note Example code file <https://github.com/libusb/libusb/blob/master/examples/testlibusb.c>
 * @return A list of USB devices present on the user's computer system, with valuable details about each device.
 */
QMap<std::string, GekkoFyre::Database::Settings::GkUsbPort> RadioLibs::enumUsbDevices(libusb_context *usb_ctx_ptr)
{
    QMap<std::string, GekkoFyre::Database::Settings::GkUsbPort> usb_hash;
    libusb_device **devices = nullptr;
    int dev_cnt = 0;
    try {
        size_t i = 0;
        dev_cnt = libusb_get_device_list(usb_ctx_ptr, &devices);
        if (dev_cnt < 0) {
            libusb_exit(usb_ctx_ptr);
            std::cout << tr("No USB devices were detected!").toStdString() << std::endl;
            return QMap<std::string, GekkoFyre::Database::Settings::GkUsbPort>();
        }

        libusb_device *dev = nullptr;
        while ((dev = devices[++i]) != nullptr) {
            GkUsbPort *usb = new GkUsbPort();

            usb->usb_enum.handle = nullptr;
            usb->usb_enum.dev = dev;
            usb->usb_enum.context = usb_ctx_ptr;

            // Get the underlying configuration details!
            int ret = libusb_get_device_descriptor(usb->usb_enum.dev, &usb->usb_enum.desc);
            if (ret < 0) {
                throw std::runtime_error(tr("Failed to get device descriptor for USB!").toStdString());
            }

            unsigned char tmp_val[32 + 1];
            // uint8_t port_numbers_len[8];

            // Open the USB device itself
            int ret_usb_open = libusb_open(dev, &usb->usb_enum.handle);
            if (ret_usb_open == LIBUSB_SUCCESS) {
                usb->usb_enum.config = new libusb_config_descriptor();
                int ret_cfg_desc = libusb_get_config_descriptor(dev, 0, &usb->usb_enum.config);
                if (ret_cfg_desc != LIBUSB_SUCCESS) {
                    std::cerr << tr("Error with enumerating `libusb` interface! Couldn't retrieve descriptors.").toStdString() << std::endl;
                }

                // https://cpp.hotexamples.com/examples/-/-/libusb_get_bus_number/cpp-libusb_get_bus_number-function-examples.html
                for (size_t j = 0; j < usb->usb_enum.config->bNumInterfaces; ++j) {
                    for (size_t k = 0; k < usb->usb_enum.config->interface[j].num_altsetting; ++k) {
                        // Export device information
                        if (usb->usb_enum.config->interface[j].altsetting[k].bInterfaceClass == LIBUSB_CLASS_AUDIO) {
                            // Export only audio devices!
                            // Obtain the BUS, Address and Port details
                            std::ostringstream os_bus;
                            std::ostringstream os_addr;
                            os_bus << std::dec << libusb_get_bus_number(dev);
                            os_addr << std::dec << libusb_get_device_address(dev);

                            usb->bus = gkDekodeDb->removeInvalidChars(os_bus.str());
                            usb->addr = gkDekodeDb->removeInvalidChars(os_addr.str());
                            usb->port = gkDekodeDb->removeInvalidChars(getUsbPortId(dev));

                            if (usb->usb_enum.desc.iManufacturer) { // Obtain the manufacturer details
                                int ret_cfg_mfg = libusb_get_string_descriptor_ascii(usb->usb_enum.handle, usb->usb_enum.desc.iManufacturer,
                                                                                     tmp_val, sizeof(tmp_val));
                                if (ret_cfg_mfg > 0) {
                                    usb->usb_enum.mfg = QString::fromLocal8Bit((char *)tmp_val, sizeof(tmp_val)).trimmed();
                                } else {
                                    throw std::runtime_error(tr("Error with enumerating `libusb` interface! Unable to obtain the manufacturer descriptor!").toStdString());
                                }

                            }
                            if (usb->usb_enum.desc.iProduct) { // Obtain the product description
                                int ret_cfg_prod = libusb_get_string_descriptor_ascii(usb->usb_enum.handle, usb->usb_enum.desc.iProduct,
                                                                                      tmp_val, sizeof(tmp_val));
                                if (ret_cfg_prod > 0) {
                                    usb->usb_enum.product = QString::fromLocal8Bit((char *)tmp_val, sizeof(tmp_val)).trimmed();
                                } else {
                                    throw std::runtime_error(tr("Error with enumerating `libusb` interface! Unable to obtain the product descriptor!").toStdString());
                                }
                            }

                            if (usb->usb_enum.desc.iSerialNumber) { // Obtain the serial number
                                int ret_cfg_serial = libusb_get_string_descriptor_ascii(usb->usb_enum.handle, usb->usb_enum.desc.iSerialNumber,
                                                                                      tmp_val, sizeof(tmp_val));
                                if (ret_cfg_serial > 0) {
                                    usb->usb_enum.serial_number = QString::fromLocal8Bit((char *)tmp_val, sizeof(tmp_val)).trimmed();
                                } else {
                                    throw std::runtime_error(tr("Error with enumerating `libusb` interface! Unable to obtain the serial number descriptor!").toStdString());
                                }
                            }

                            if (!usb_hash.contains(usb->port)) {
                                usb_hash.insert(usb->port, *usb);
                            }
                        }
                    }
                }
            }
            delete usb;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    // libusb_close(usb->usb_enum.handle);
    libusb_free_device_list(devices, dev_cnt);
    if (!usb_hash.isEmpty()) {
        return usb_hash;
    }

    return QMap<std::string, GekkoFyre::Database::Settings::GkUsbPort>();
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
std::shared_ptr<GkRadio> RadioLibs::init_rig(const rig_model_t &rig_model, const std::string &com_port,
                                             const com_baud_rates &com_baud_rate, const rig_debug_level_e &verbosity)
{
    std::mutex mtx_init_rig;
    std::lock_guard<std::mutex> lck_guard(mtx_init_rig);

    // https://github.com/Hamlib/Hamlib/blob/master/tests/example.c
    // Set verbosity level
    rig_set_debug(verbosity);

    // Setup serial port, baud rate, etc.
    fs::path slashes = "//./";
    fs::path native_slash = slashes.make_preferred().native();
    fs::path com_port_path;
    #ifdef _WIN32
    com_port_path = fs::path(slashes.string() + com_port);
    #elif __linux__
    com_port_path = com_port;
    #endif

    if (rig_model < 1) { // No amateur radio rig has been configured and/or adequately detected!
        gkRadioPtr->rig_file = com_port_path.string(); // TODO: Replace this with real values, as this is only an example for now...

        strncpy(gkRadioPtr->port_details.pathname, gkRadioPtr->rig_file.c_str(), FILPATHLEN - 1);

        int baud_rate = 9600;
        baud_rate = convertBaudRateInt(com_baud_rate);

        gkRadioPtr->port_details.parm.serial.rate = baud_rate; // The BAUD Rate for the desired COM Port
        gkRadioPtr->port_details.type.rig = convGkConnTypeToHamlib(gkRadioPtr->cat_conn_type);
        gkRadioPtr->port_details.parm.serial.rate = baud_rate;
        gkRadioPtr->port_details.parm.serial.data_bits = data_bits;
        gkRadioPtr->port_details.parm.serial.stop_bits = stop_bits;
        gkRadioPtr->port_details.parm.serial.parity = RIG_PARITY_NONE;
        gkRadioPtr->port_details.parm.serial.handshake = RIG_HANDSHAKE_NONE;

        //
        // Probe the given communications port, whether it be RS232, USB, GPIO, etc.
        // With this information, provided a connection has been made successfully, we can infer the amateur radio
        // rig that the user is making use of!
        //
        gkRadioPtr->rig_model = rig_probe(&gkRadioPtr->port_details);
    }

    // Instantiate the rig
    gkRadioPtr->rig = rig_init(rig_model);

    if (gkRadioPtr->rig == nullptr) {
        throw std::runtime_error(tr("Unable to initialize Hamlib!").toStdString());
    }

    rig_debug(verbosity, "Backend version: %s, Status: %s\n\n", gkRadioPtr->rig->caps->version, rig_strstatus(gkRadioPtr->rig->caps->status));

    // Open our rig in question
    int retcode = rig_open(gkRadioPtr->rig);
    hamlibStatus(retcode);

    gkRadioPtr->is_open = true; // Set the flag that the aforementioned pointer has been initialized

    if (rig_get_info(gkRadioPtr->rig) != nullptr) {
        // Give me ID info, e.g., firmware version
        gkRadioPtr->info_buf = rig_get_info(gkRadioPtr->rig);
        std::cout << tr("Rig info: %1\n\n").arg(QString::fromStdString(gkRadioPtr->info_buf)).toStdString();
    }

    // Note from Hamlib Developers: As a general practice, we should check to see if a given function
    // is within the rig's capabilities before calling it, but we are simplifying here. Also, we should
    // check each call's returned status in case of error.

    // Main VFO frequency
    gkRadioPtr->status = rig_get_freq(gkRadioPtr->rig, RIG_VFO_CURR, &gkRadioPtr->freq);
    std::cout << tr("Main VFO Frequency: %1\n\n").arg(QString::number(gkRadioPtr->freq)).toStdString();

    // Current mode
    gkRadioPtr->status = rig_get_mode(gkRadioPtr->rig, RIG_VFO_CURR, &gkRadioPtr->mode, &gkRadioPtr->width);

    // Determine the mode of modulation that's being currently used, and output as a textual value
    gkRadioPtr->mm = hamlibModulEnumToStr(gkRadioPtr->mode).toStdString();

    std::cout << tr("Current mode: %1, width is %2\n\n").arg(QString::fromStdString(gkRadioPtr->mm)).arg(QString::number(gkRadioPtr->width)).toStdString();

    // Rig power output
    gkRadioPtr->status = rig_get_level(gkRadioPtr->rig, RIG_VFO_CURR, RIG_LEVEL_RFPOWER, &gkRadioPtr->power);
    std::cout << tr("RF Power relative setting: %%1 (0.0 - 1.0)\n\n").arg(QString::number(gkRadioPtr->power.f)).toStdString();

    // Convert power reading to watts
    gkRadioPtr->status = rig_power2mW(gkRadioPtr->rig, &gkRadioPtr->mwpower, gkRadioPtr->power.f, gkRadioPtr->freq, gkRadioPtr->mode);
    std::cout << tr("RF Power calibrated: %1 watts\n\n").arg(QString::number(gkRadioPtr->mwpower / 1000)).toStdString();

    // Raw and calibrated S-meter values
    gkRadioPtr->status = rig_get_level(gkRadioPtr->rig, RIG_VFO_CURR, RIG_LEVEL_RAWSTR, &gkRadioPtr->raw_strength);
    std::cout << tr("Raw receive strength: %1\n\n").arg(QString::number(gkRadioPtr->raw_strength.i)).toStdString();

    gkRadioPtr->isz = gkRadioPtr->rig->caps->str_cal.size; // TODO: No idea what this is for?

    gkRadioPtr->status = rig_get_strength(gkRadioPtr->rig, RIG_VFO_CURR, &gkRadioPtr->strength);

    return gkRadioPtr;
}

/**
 * @brief RadioLibs::translateBandsToStr will translate a given band to the equivalent QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param band The given amateur radio band, in meters.
 * @return The amateur radio band, in meters, provided as a QString().
 */
QString RadioLibs::translateBandsToStr(const bands &band)
{
    switch (band) {
    case bands::BAND160:
        return tr("None");
    case bands::BAND80:
        return tr("80 meters");
    case bands::BAND60:
        return tr("60 meters");
    case bands::BAND40:
        return tr("40 meters");
    case bands::BAND30:
        return tr("30 meters");
    case bands::BAND20:
        return tr("20 meters");
    case bands::BAND17:
        return tr("15 meters");
    case bands::BAND15:
        return tr("17 meters");
    case bands::BAND12:
        return tr("12 meters");
    case bands::BAND10:
        return tr("10 meters");
    case bands::BAND6:
        return tr("6 meters");
    case bands::BAND2:
        return tr("2 meters");
    default:
        return tr("Unsupported!");
    }

    return tr("Error!");
}

/**
 * @brief RadioLibs::hamlibModulEnumToStr converts the given Hamlib enums for modulation to their relatable
 * textual value.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param modulation The given modulation, as a Hamlib enum.
 * @return The same enum, but given as a textual value.
 */
QString RadioLibs::hamlibModulEnumToStr(const rmode_t &modulation)
{
    // NOTE: These are meant to be translatable!
    switch (modulation) {
    case rmode_t::RIG_MODE_NONE:
        return tr("None");
    case rmode_t::RIG_MODE_AM:
        return tr("AM");
    case rmode_t::RIG_MODE_CW:
        return tr("CW");
    case rmode_t::RIG_MODE_USB:
        return tr("USB");
    case rmode_t::RIG_MODE_LSB:
        return tr("LSB");
    case rmode_t::RIG_MODE_RTTY:
        return tr("RTTY");
    case rmode_t::RIG_MODE_FM:
        return tr("FM");
    case rmode_t::RIG_MODE_WFM:
        return tr("Wide FM");
    case rmode_t::RIG_MODE_CWR:
        return tr("CWR");
    case rmode_t::RIG_MODE_RTTYR:
        return tr("RTTYR");
    case rmode_t::RIG_MODE_AMS:
        return tr("AMS");
    case rmode_t::RIG_MODE_PKTLSB:
        return tr("PKT/LSB");
    case rmode_t::RIG_MODE_PKTUSB:
        return tr("PKT/USB");
    case rmode_t::RIG_MODE_PKTFM:
        return tr("PKT/FM");
    case rmode_t::RIG_MODE_ECSSUSB:
        return tr("ECSS/USB");
    case rmode_t::RIG_MODE_ECSSLSB:
        return tr("ECSS/LSB");
    case rmode_t::RIG_MODE_FAX:
        return tr("FAX");
    case rmode_t::RIG_MODE_SAM:
        return tr("SAM");
    case rmode_t::RIG_MODE_SAL:
        return tr("SAL");
    case rmode_t::RIG_MODE_SAH:
        return tr("SAH");
    case rmode_t::RIG_MODE_DSB:
        return tr("DSB");
    case rmode_t::RIG_MODE_FMN:
        return tr("FM Narrow");
    case rmode_t::RIG_MODE_PKTAM:
        return tr("PKT/AM");
    case rmode_t::RIG_MODE_TESTS_MAX:
        return tr("TESTS_MAX");
    default:
        tr("N/A");
    }

    return tr("None");
}

/**
 * @brief RadioLibs::convGkConnTypeToEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param conn_type
 * @return
 */
GkConnType RadioLibs::convGkConnTypeToEnum(const QString &conn_type)
{
    if (conn_type == "RS232") {
        // RS232
        return GkConnType::RS232;
    } else if (conn_type == "USB") {
        // USB
        return GkConnType::USB;
    } else if (conn_type == "Parallel") {
        // Parallel
        return GkConnType::Parallel;
    } else if (conn_type == "CM108") {
        // CM108
        return GkConnType::CM108;
    } else if (conn_type == "GPIO") {
        // GPIO
        return GkConnType::GPIO;
    } else {
        return GkConnType::None;
    }

    return GkConnType::None;
}

/**
 * @brief RadioLibs::convGkConnTypeToHamlib converts from the pre-configured Small World Deluxe enumerators to the
 * ones which are more suitable for Hamlib.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param conn_type The desired Small World Deluxe enumerator to convert to a Hamlib one.
 * @return The desired Small World Deluxe enumerator that has been converted to a Hamlib enum.
 */
rig_port_e RadioLibs::convGkConnTypeToHamlib(const GkConnType &conn_type)
{
    switch (conn_type) {
    case GkConnType::RS232:
        return rig_port_e::RIG_PORT_SERIAL;
    case GkConnType::USB:
        return rig_port_e::RIG_PORT_USB;
    case GkConnType::Parallel:
        return rig_port_e::RIG_PORT_PARALLEL;
    case GkConnType::CM108:
        return rig_port_e::RIG_PORT_CM108;
    case GkConnType::GPIO:
        return rig_port_e::RIG_PORT_GPIO;
    default:
        return rig_port_e::RIG_PORT_NONE;
    }

    return rig_port_e::RIG_PORT_NONE;
}

/**
 * @brief RadioLibs::procFreqChange will process a frequency change request upon receiving the right signal(s).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param radio_locked Whether the transceiver rig is in a 'locked mode' or not.
 * @param freq_change The details of the frequency change request.
 */
void RadioLibs::procFreqChange(const bool &radio_locked, const FreqChange &freq_change)
{
    try {
        if (gkRadioPtr->rig == nullptr && gkRadioPtr->is_open == false) {
            throw std::runtime_error(tr("Small World Deluxe has experienced a rig control error!").toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief RadioLibs::procSettingsChange will process a change in settings regarding Hamlib upon receiving the right signal(s).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param radio_locked Whether the transceiver rig is in a 'locked mode' or not.
 * @param settings_change The details of the settings change request.
 */
void RadioLibs::procSettingsChange(const bool &radio_locked, const SettingsChange &settings_change)
{
    try {
        if (gkRadioPtr->rig == nullptr && gkRadioPtr->is_open == false) {
            throw std::runtime_error(tr("Small World Deluxe has experienced a rig control error!").toStdString());
        }

        if (!settings_change.rig_file.empty()) {
            if (settings_change.rig_file != gkRadioPtr->rig_file) {
                gkRadioPtr->rig_file = settings_change.rig_file;
            }
        }

        if (settings_change.rig_model > 0) {
            gkRadioPtr->rig_model = settings_change.rig_model;
        }

        gkRadioPtr->dev_baud_rate = settings_change.dev_baud_rate;
        gkRadioPtr->port_details = settings_change.port_details;
        gkRadioPtr->mode = settings_change.mode;
        gkRadioPtr->width = settings_change.width;

        //
        // Process the changes, if necessary, for the RS232 ports
        //
        gkRadioPtr->rig_model = rig_probe(&gkRadioPtr->port_details);
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return;
}
