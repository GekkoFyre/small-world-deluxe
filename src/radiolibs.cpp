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

#include "radiolibs.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <ios>
#include <list>
#include <iostream>
#include <exception>
#include <utility>
#include <cstring>
#include <sstream>
#include <QMessageBox>

#ifdef _WIN32
#include <stringapiset.h>
#elif __linux__ || __MINGW64__
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_messagebox.h>
#endif

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

RadioLibs::RadioLibs(QPointer<FileIo> filePtr, std::shared_ptr<StringFuncs> stringPtr,
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
com_baud_rates RadioLibs::convertBaudRateToEnum(const int &baud_rate_sel)
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
 * @brief RadioLibs::convertBaudRateIntToEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param baud_rate
 * @return
 */
com_baud_rates RadioLibs::convertBaudRateIntToEnum(const int &baud_rate)
{
    com_baud_rates ret = BAUD1200;
    switch (baud_rate) {
    case 1200:
        ret = BAUD1200;
        break;
    case 2400:
        ret = BAUD2400;
        break;
    case 4800:
        ret = BAUD4800;
        break;
    case 9600:
        ret = BAUD9600;
        break;
    case 19200:
        ret = BAUD19200;
        break;
    case 38400:
        ret = BAUD38400;
        break;
    case 57600:
        ret = BAUD57600;
        break;
    case 115200:
        ret = BAUD115200;
        break;
    default:
        ret = BAUD1200;
        break;
    }

    return ret;
}

/**
 * @brief RadioLibs::convertBaudRateFromEnum
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param baud_rate
 * @return
 */
int RadioLibs::convertBaudRateFromEnum(const com_baud_rates &baud_rate)
{
    int ret = 0;
    switch (baud_rate) {
    case BAUD1200:
        ret = 0;
        break;
    case BAUD2400:
        ret = 1;
        break;
    case BAUD4800:
        ret = 2;
        break;
    case BAUD9600:
        ret = 3;
        break;
    case BAUD19200:
        ret = 4;
        break;
    case BAUD38400:
        ret = 5;
        break;
    case BAUD57600:
        ret = 6;
        break;
    case BAUD115200:
        ret = 7;
        break;
    default:
        ret = 0;
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
std::list<GkComPort> RadioLibs::status_com_ports()
{
    try {
        std::list<GkComPort> com_map;
        std::vector<serial::PortInfo> devices_found = serial::list_ports();

        for (const auto &port: devices_found) {
            GkComPort com_struct;
            com_struct.port_info = port;

            if (!com_struct.port_info.port.empty()) {
                com_map.push_back(com_struct);
            }
        }

        return com_map;
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd = nullptr;
        modalDlgBoxOk(hwnd, tr("Error!"), e.what(), MB_ICONERROR);
        DestroyWindow(hwnd);
        #else
        modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), e.what());
        #endif
    }

    return std::list<GkComPort>();
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
                        for (size_t m = 0; m < usb->usb_enum.config->interface[j].altsetting[k].bNumEndpoints; ++m) {
                            // Export only audio devices!
                            if (usb->usb_enum.config->interface[j].altsetting[k].bInterfaceClass == LIBUSB_CLASS_AUDIO) {
                                //
                                // Export device information
                                // Obtain the BUS, Address and Port details
                                //
                                std::ostringstream os_bus;
                                std::ostringstream os_addr;
                                os_bus << std::dec << libusb_get_bus_number(dev);
                                os_addr << std::dec << libusb_get_device_address(dev);

                                usb->bus = gkDekodeDb->removeInvalidChars(os_bus.str());
                                usb->addr = gkDekodeDb->removeInvalidChars(os_addr.str());
                                usb->port = gkDekodeDb->removeInvalidChars(getUsbPortId(dev));

                                usb->usb_enum.vendor_id = static_cast<int>(usb->usb_enum.desc.idVendor);
                                usb->usb_enum.product_id = static_cast<int>(usb->usb_enum.desc.idProduct);
                                usb->usb_enum.conv_conf = static_cast<int>(usb->usb_enum.desc.bNumConfigurations);

                                usb->usb_vers_3.inter_desc = &usb->usb_enum.config->interface[j].altsetting[k];
                                usb->usb_vers_3.endpoint = &usb->usb_enum.config->interface[j].altsetting[k].endpoint[m];
                                libusb_get_ss_endpoint_companion_descriptor(usb->usb_enum.context, usb->usb_vers_3.endpoint, &usb->usb_vers_3.ss_desc);

                                usb->usb_vers_3.interface_number = static_cast<int>(usb->usb_enum.config->interface[j].altsetting[k].bInterfaceNumber);
                                usb->usb_vers_3.alternate_setting = static_cast<int>(usb->usb_enum.config->interface[j].altsetting[k].bAlternateSetting);

                                usb->usb_vers_3.max_packet_size = static_cast<int>(usb->usb_enum.config->interface[j].altsetting[k].endpoint[m].wMaxPacketSize);
                                usb->usb_vers_3.interval = static_cast<int>(usb->usb_enum.config->interface[j].altsetting[k].endpoint[m].bInterval);
                                usb->usb_vers_3.refresh = static_cast<int>(usb->usb_enum.config->interface[j].altsetting[k].endpoint[m].bRefresh);
                                usb->usb_vers_3.sync_address = static_cast<int>(usb->usb_enum.config->interface[j].altsetting[k].endpoint[m].bSynchAddress);

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

#if defined(_MSC_VER) && (_MSC_VER > 1900)
bool RadioLibs::modalDlgBoxOk(const HWND &hwnd, const QString &title, const QString &msgTxt, const int &icon)
{
    // TODO: Make this dialog modal
    std::mutex mtx_modal_dlg_box;
    std::lock_guard<std::mutex> lck_guard(mtx_modal_dlg_box);
    int msgBoxId = MessageBoxA(hwnd, msgTxt.toStdString().c_str(), title.toStdString().c_str(), icon | MB_OK);

    switch (msgBoxId) {
    case IDOK:
        return true;
    default:
        return false;
    }

    return false;
}
#else
bool RadioLibs::modalDlgBoxLinux(Uint32 flags, const QString &title, const QString &msgTxt)
{
    SDL_Window *sdlWindow = SDL_CreateWindow(General::productName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DLG_BOX_WINDOW_WIDTH, DLG_BOX_WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    int ret = SDL_ShowSimpleMessageBox(flags, title.toStdString().c_str(), msgTxt.toStdString().c_str(), sdlWindow);
    SDL_DestroyWindow(sdlWindow);
    return ret;
}
#endif

/**
 * @brief RadioLibs::gkInitRadioRig Initializes the struct, `GekkoFyre::AmateurRadio::Control::GkRadio`, and all of the values within,
 * along with the user's desired amateur radio rig of choice, including anything needed to power it up and enable communication between
 * computing device and the rig itself.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param radio_ptr The needed information to power up the user's desired amateur radio rig of choice.
 * @param usb_ptr A pointer which contains all the information on user's configured USB devices, if any.
 * @note Ref: HamLib <https://github.com/Hamlib/Hamlib/>. Example: <https://github.com/Hamlib/Hamlib/blob/master/tests/example.c>
 */
void RadioLibs::gkInitRadioRig(std::shared_ptr<GkRadio> radio_ptr, std::shared_ptr<GkUsbPort> usb_ptr)
{
    std::mutex mtx_init_rig;
    std::lock_guard<std::mutex> lck_guard(mtx_init_rig);

    try {
        // https://github.com/Hamlib/Hamlib/blob/master/tests/example.c
        // Set verbosity level
        rig_set_debug(radio_ptr->verbosity);

        // Instantiate the rig
        radio_ptr->rig = rig_init(radio_ptr->rig_model);

        // Setup serial port, baud rate, etc.
        int baud_rate = 9600;
        int baud_rate_tmp = convertBaudRateInt(radio_ptr->dev_baud_rate);
        if (baud_rate_tmp <= 115200 && baud_rate_tmp >= 9600) {
            baud_rate = baud_rate_tmp;
        }

        if (radio_ptr->cat_conn_type == GkConnType::RS232) {
            //
            // RS232
            //
            radio_ptr->rig->state.rigport.parm.serial.data_bits = radio_ptr->port_details.parm.serial.data_bits;
            radio_ptr->rig->state.rigport.parm.serial.stop_bits = radio_ptr->port_details.parm.serial.stop_bits;
            radio_ptr->rig->state.rigport.parm.serial.rate = baud_rate; // The BAUD Rate for the desired COM Port
            radio_ptr->port_details.type.rig = convGkConnTypeToHamlib(radio_ptr->cat_conn_type);

            // radio_ptr->rig->state.rigport.parm.serial.rts_state = radio_ptr->port_details.parm.serial.rts_state;
            radio_ptr->rig->state.rigport.parm.serial.dtr_state = radio_ptr->port_details.parm.serial.dtr_state;
            radio_ptr->rig->state.rigport.parm.serial.handshake = radio_ptr->port_details.parm.serial.handshake;
            radio_ptr->rig->state.rigport.type.ptt = radio_ptr->port_details.type.ptt;

            #if __MINGW64__
            //
            // Modify the COM Port so that it's suitable for Hamlib!
            //
            boost::replace_all(radio_ptr->cat_conn_port, "COM", "/dev/ttyS");
            boost::replace_all(radio_ptr->ptt_conn_port, "COM", "/dev/ttyS");
            #endif

            //
            // Determine the port necessary and let Hamlib know about it!
            //
            if (!radio_ptr->cat_conn_port.empty()) {
                strncpy(radio_ptr->port_details.pathname, radio_ptr->cat_conn_port.c_str(), sizeof(radio_ptr->rig->state.rigport.pathname));
                radio_ptr->port_details.pathname[radio_ptr->cat_conn_port.size()] = '\0';

                strncpy(radio_ptr->rig->state.rigport.pathname, radio_ptr->cat_conn_port.c_str(), sizeof(radio_ptr->rig->state.rigport.pathname));
                radio_ptr->rig->state.rigport.pathname[radio_ptr->cat_conn_port.size()] = '\0';
            }
        } else if (radio_ptr->cat_conn_type == GkConnType::USB) {
            //
            // USB
            //
            if (usb_ptr->bus.empty() || usb_ptr->addr.empty()) {
                throw std::runtime_error(tr("Unable to initialize radio rig with USB connection!").toStdString());
            }

            radio_ptr->rig->state.rigport.parm.usb.vid = usb_ptr->usb_enum.vendor_id;
            radio_ptr->rig->state.rigport.parm.usb.pid = usb_ptr->usb_enum.product_id;
            radio_ptr->rig->state.rigport.parm.usb.conf = usb_ptr->usb_enum.conv_conf;
            radio_ptr->rig->state.rigport.parm.usb.iface = usb_ptr->usb_vers_3.interface_number;
            radio_ptr->rig->state.rigport.parm.usb.alt = usb_ptr->usb_vers_3.alternate_setting;

            //
            // Determine the port necessary and let Hamlib know about it!
            //
            if (!radio_ptr->cat_conn_port.empty()) {
                strncpy(radio_ptr->rig->state.rigport.pathname, radio_ptr->cat_conn_port.c_str(), sizeof(radio_ptr->rig->state.rigport.pathname));
                radio_ptr->rig->state.rigport.pathname[radio_ptr->cat_conn_port.size()] = '\0';
            }
        } else if (radio_ptr->cat_conn_type == GkConnType::Parallel) {
            //
            // Parallel
            //
            radio_ptr->rig->state.rigport.parm.parallel.pin = -1; // TODO: Finish this section for Parallel connections!

            //
            // Determine the port necessary and let Hamlib know about it!
            //
            // strncpy(radio_ptr->rig->state.rigport.pathname, radio_ptr, FILPATHLEN - 1);
        } else {
            throw std::runtime_error(tr("Unable to detect connection type while initializing radio rig (i.e. 'none / unknown' was not an option)!").toStdString());
        }

        if (radio_ptr->rig_model < 1) { // No amateur radio rig has been configured and/or adequately detected!
            //
            // Probe the given communications port, whether it be RS232, USB, GPIO, etc.
            // With this information, provided a connection has been made successfully, we can infer the amateur radio
            // rig that the user is making use of!
            //
            radio_ptr->rig_model = rig_probe(&radio_ptr->port_details);
        }

        if (!radio_ptr->rig) {
            throw std::runtime_error(tr("Unknown radio rig: %1\n\nNote to developers: Please check the list of rigs.")
                                     .arg(QString::number(radio_ptr->rig_model)).toStdString());
        }

        rig_debug(radio_ptr->verbosity, "Backend version: %s, Status: %s\n\n", radio_ptr->rig->caps->version, rig_strstatus(radio_ptr->rig->caps->status));

        //
        // Open our rig in question
        //
        radio_ptr->retcode = rig_open(radio_ptr->rig);
        if (radio_ptr->retcode != RIG_OK) {
            throw std::runtime_error(tr("[ Hamlib ] Error with opening amateur radio rig:\n\n%1").arg(QString::fromStdString(rigerror(radio_ptr->retcode))).toStdString());
        } else {
            radio_ptr->is_open = true; // Set the flag that the aforementioned pointer has been initialized
        }

        while (radio_ptr->is_open) {
            //
            // IMPORTANT!!!
            // Note from Hamlib Developers: As a general practice, we should check to see if a given function
            // is within the rig's capabilities before calling it, but we are simplifying here. Also, we should
            // check each call's returned status in case of error.
            //
            if (rig_get_info(radio_ptr->rig) != nullptr) {
                radio_ptr->info_buf = rig_get_info(radio_ptr->rig);
            }

            // Main VFO frequency
            radio_ptr->status = rig_get_freq(radio_ptr->rig, RIG_VFO_CURR, &radio_ptr->freq);
            if (radio_ptr->status != RIG_OK) {
                std::cerr << tr("[ Hamlib ] Error with obtaining the primary VFO value for radio rig:\n\n%1").arg(QString::fromStdString(rigerror(radio_ptr->status))).toStdString() << std::endl;
            }

            // Current mode
            radio_ptr->status = rig_get_mode(radio_ptr->rig, RIG_VFO_CURR, &radio_ptr->mode, &radio_ptr->width);
            if (radio_ptr->status != RIG_OK) {
                std::cerr << tr("[ Hamlib ] Error with obtaining current modulation for radio rig:\n\n%1").arg(QString::fromStdString(rigerror(radio_ptr->status))).toStdString() << std::endl;
            }

            // Determine the mode of modulation that's being currently used, and output as a textual value
            radio_ptr->mm = hamlibModulEnumToStr(radio_ptr->mode).toStdString();

            // Rig power output
            radio_ptr->status = rig_get_level(radio_ptr->rig, RIG_VFO_CURR, RIG_LEVEL_RFPOWER, &radio_ptr->power);
            if (radio_ptr->status != RIG_OK) {
                std::cerr << tr("[ Hamlib ] Error with obtaining power output for radio rig:\n\n%1").arg(QString::fromStdString(rigerror(radio_ptr->status))).toStdString() << std::endl;
            }

            // Convert power reading to watts
            radio_ptr->status = rig_power2mW(radio_ptr->rig, &radio_ptr->mwpower, radio_ptr->power.f, radio_ptr->freq, radio_ptr->mode);
            if (radio_ptr->status != RIG_OK) {
                std::cerr << tr("[ Hamlib ] Error with converting signal power to watts for radio rig:\n\n%1").arg(QString::fromStdString(rigerror(radio_ptr->status))).toStdString() << std::endl;
            }

            // Raw and calibrated S-meter values
            radio_ptr->status = rig_get_level(radio_ptr->rig, RIG_VFO_CURR, RIG_LEVEL_RAWSTR, &radio_ptr->raw_strength);
            if (radio_ptr->status != RIG_OK) {
                std::cerr << tr("[ Hamlib ] Error with calibrating S-value output for radio rig:\n\n%1").arg(QString::fromStdString(rigerror(radio_ptr->status))).toStdString() << std::endl;
            }

            radio_ptr->isz = radio_ptr->rig->caps->str_cal.size; // TODO: No idea what this is for?

            radio_ptr->status = rig_get_strength(radio_ptr->rig, RIG_VFO_CURR, &radio_ptr->strength);
            if (radio_ptr->status != RIG_OK) {
                std::cerr << tr("[ Hamlib ] Error with obtaining signal strength for radio rig:\n\n%1").arg(QString::fromStdString(rigerror(radio_ptr->status))).toStdString() << std::endl;
            }
        }
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd = nullptr;
        modalDlgBoxOk(hwnd, tr("Error!"), e.what(), MB_ICONERROR);
        DestroyWindow(hwnd);
        #else
        modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), e.what());
        #endif
    }

    return;
}

/**
 * @brief RadioLibs::calibrateAudioInputSignal calibrates an audio input signal so that the obtained volume is around
 * a value of 60 decibels instead, which is much better for FFT analysis and so on.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data_buf The raw audio stream itself.
 * @return A factor for which to multiply the volume of the audio signal itself by.
 */
int16_t RadioLibs::calibrateAudioInputSignal(const int16_t *data_buf)
{
    return -1;
}

/**
 * @brief RadioLibs::translateBandsToStr will translate a given band to the equivalent QString().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param band The given amateur radio band, in meters.
 * @return The amateur radio band, in meters, provided as a QString().
 */
QString RadioLibs::translateBandsToStr(const GkFreqBands &band)
{
    switch (band) {
    case GkFreqBands::BAND160:
        return tr("None");
    case GkFreqBands::BAND80:
        return tr("80 meters");
    case GkFreqBands::BAND60:
        return tr("60 meters");
    case GkFreqBands::BAND40:
        return tr("40 meters");
    case GkFreqBands::BAND30:
        return tr("30 meters");
    case GkFreqBands::BAND20:
        return tr("20 meters");
    case GkFreqBands::BAND17:
        return tr("15 meters");
    case GkFreqBands::BAND15:
        return tr("17 meters");
    case GkFreqBands::BAND12:
        return tr("12 meters");
    case GkFreqBands::BAND10:
        return tr("10 meters");
    case GkFreqBands::BAND6:
        return tr("6 meters");
    case GkFreqBands::BAND2:
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
    case RIG_MODE_NONE:
        return tr("None");
    case RIG_MODE_AM:
        return tr("AM");
    case RIG_MODE_CW:
        return tr("CW");
    case RIG_MODE_USB:
        return tr("USB");
    case RIG_MODE_LSB:
        return tr("LSB");
    case RIG_MODE_RTTY:
        return tr("RTTY");
    case RIG_MODE_FM:
        return tr("FM");
    case RIG_MODE_WFM:
        return tr("Wide FM");
    case RIG_MODE_CWR:
        return tr("CWR");
    case RIG_MODE_RTTYR:
        return tr("RTTYR");
    case RIG_MODE_AMS:
        return tr("AMS");
    case RIG_MODE_PKTLSB:
        return tr("PKT/LSB");
    case RIG_MODE_PKTUSB:
        return tr("PKT/USB");
    case RIG_MODE_PKTFM:
        return tr("PKT/FM");
    case RIG_MODE_ECSSUSB:
        return tr("ECSS/USB");
    case RIG_MODE_ECSSLSB:
        return tr("ECSS/LSB");
    case RIG_MODE_FAX:
        return tr("FAX");
    case RIG_MODE_SAM:
        return tr("SAM");
    case RIG_MODE_SAL:
        return tr("SAL");
    case RIG_MODE_SAH:
        return tr("SAH");
    case RIG_MODE_DSB:
        return tr("DSB");
    case RIG_MODE_FMN:
        return tr("FM Narrow");
    case RIG_MODE_PKTAM:
        return tr("PKT/AM");
    case RIG_MODE_TESTS_MAX:
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
