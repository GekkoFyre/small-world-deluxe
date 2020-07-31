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

#include "src/radiolibs.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <QtUsb/QUsbDevice>
#include <QtUsb/QHidDevice>
#include <QtUsb/QUsbEndpoint>
#include <QtUsb/QUsbInfo>
#include <ios>
#include <list>
#include <iostream>
#include <exception>
#include <utility>
#include <cstring>
#include <sstream>
#include <QMessageBox>
#include <QSerialPortInfo>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#ifdef _WIN64
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#endif

#if __linux__
#include <sys/ioctl.h>
#include <linux/serial.h>
#endif

#ifdef __cplusplus
}
#endif

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

namespace fs = boost::filesystem;
namespace sys = boost::system;

RadioLibs::RadioLibs(QPointer<FileIo> filePtr, std::shared_ptr<StringFuncs> stringPtr,
                     std::shared_ptr<GkLevelDb> dkDb, std::shared_ptr<GkRadio> radioPtr,
                     QPointer<GkEventLogger> eventLogger, QObject *parent) : QObject(parent)
{
    gkStringFuncs = std::move(stringPtr);
    gkDekodeDb = std::move(dkDb);
    gkFileIo = std::move(filePtr);
    gkRadioPtr = std::move(radioPtr);
    gkEventLogger = std::move(eventLogger);
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
 * @brief RadioLibs::status_com_ports Checks the status of a COM/Serial port and whether it is active or not.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QList<QSerialPortInfo> RadioLibs::status_com_ports() const
{
    std::mutex mtx_status_com_ports;
    std::lock_guard<std::mutex> lck_guard(mtx_status_com_ports);

    try {
        const auto rs232_data = QSerialPortInfo::availablePorts();
        return rs232_data;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An issue was encountered whilst enumerating RS232 ports!\n\n%1")
                                                  .arg(QString::fromStdString(e.what())).toStdString()));
    }

    return QList<QSerialPortInfo>();
}

/**
 * @brief RadioLibs::filter_com_ports
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param serial_port_info
 * @return
 */
std::list<GkComPort> RadioLibs::filter_com_ports(const QList<QSerialPortInfo> &serial_port_info) const
{
    std::mutex mtx_filter_com_ports;
    std::lock_guard<std::mutex> lck_guard(mtx_filter_com_ports);

    try {
        std::list<GkComPort> com_map;
        for (const auto &info: serial_port_info) {
            if (!info.isNull()) {
                bool is_usb = false; // Are we dealing with a USB device or not? Since we have a separate library for handling such connections...
                for (const auto &port: info.availablePorts()) {
                    if (!port.isNull() && port.hasProductIdentifier()) {
                        GkComPort com_struct;
                        if (port.portName().contains(QString("USB"), Qt::CaseSensitive)) {
                            is_usb = true;
                        }

                        if (!is_usb) {
                            com_struct.port_info = info;
                            com_map.push_back(com_struct);
                            is_usb = false;
                        }
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An issue was encountered whilst enumerating RS232 ports!\n\n%1")
                                                  .arg(QString::fromStdString(e.what())).toStdString()));
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
 * @brief RadioLibs::enumUsbDevices will enumerate out any USB devices present/connected on the user's computer system, giving valuable
 * details for each device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Example code file <https://github.com/libusb/libusb/blob/master/examples/testlibusb.c>
 * @return A list of USB devices present on the user's computer system, with valuable details about each device.
 */
QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> RadioLibs::enumUsbDevices()
{
    QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort> usb_hash;
    try {
        // Enumerate USB devices!
        QUsbInfo usb_info;
        auto list = usb_info.devices();

        for (const auto &device: list) {
            GekkoFyre::Database::Settings::GkUsbPort usb;
            usb.port = (quint16)device.port;
            usb.bus = (quint16)device.bus;
            usb.pid = device.pid;
            usb.vid = device.vid;
            usb.d_class = (quint16)device.dClass;
            usb.d_sub_class = (quint16)device.dSubClass;

            QHidDevice usb_hid(new QHidDevice());
            usb_hid.open(usb.vid, usb.pid);
            usb.mfg = usb_hid.manufacturer();
            usb.product = usb_hid.product();

            #ifdef _WIN32
            // TODO: URGENT - Finish this section!
            #elif __linux__
            std::stringstream ss;
            ss << "ttyUSB" << usb.port;
            #endif

            usb.name = QString::fromStdString(ss.str());
            usb_hash.insert(usb.port, usb);
        }

        return usb_hash;
    }  catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return QMap<quint16, GekkoFyre::Database::Settings::GkUsbPort>();
}

/**
 * @brief RadioLibs::print_exception
 * @param e
 * @param level
 */
void RadioLibs::print_exception(const std::exception &e, int level)
{
    gkEventLogger->publishEvent(e.what(), GkSeverity::Warning, "", true);

    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& e) {
        print_exception(e, level+1);
    } catch(...) {}

    return;
}

/**
 * @brief RadioLibs::gkInitRadioRig Initializes the struct, `GekkoFyre::AmateurRadio::Control::GkRadio`, and all of the values within,
 * along with the user's desired amateur radio rig of choice, including anything needed to power it up and enable communication between
 * computing device and the rig itself.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param radio_ptr The needed information to power up the user's desired amateur radio rig of choice.
 * @param usb_ptr A pointer which contains all the information on user's configured USB devices, if any.
 * @note Ref: HamLib <https://github.com/Hamlib/Hamlib/>. Example: <https://github.com/Hamlib/Hamlib/blob/master/tests/example.c>
 */
void RadioLibs::gkInitRadioRig(std::shared_ptr<GkRadio> radio_ptr)
{
    std::mutex mtx_init_rig;
    std::lock_guard<std::mutex> lck_guard(mtx_init_rig);

    try {
        // https://github.com/Hamlib/Hamlib/blob/master/tests/example.c
        // Set verbosity level
        rig_set_debug(radio_ptr->verbosity);

        radio_ptr->gkRig = std::make_shared<Rig>(radio_ptr->rig_model);

        // Setup serial port, baud rate, etc.
        int baud_rate = 9600;
        int baud_rate_tmp = convertBaudRateInt(radio_ptr->dev_baud_rate);
        if (baud_rate_tmp <= 115200 && baud_rate_tmp >= 9600) {
            baud_rate = baud_rate_tmp;
        }

        //
        // Check the most up-to-date information on RS232 ports
        //
        auto recent_serial_port_info = filter_com_ports(status_com_ports());

        radio_ptr->gkRig->setConf("data_bits", std::to_string(radio_ptr->port_details.parm.serial.data_bits).c_str());
        radio_ptr->gkRig->setConf("stop_bits", std::to_string(radio_ptr->port_details.parm.serial.stop_bits).c_str());
        radio_ptr->gkRig->setConf("serial_speed", std::to_string(baud_rate).c_str());
        radio_ptr->port_details.type.rig = convGkConnTypeToHamlib(radio_ptr->cat_conn_type);

        switch (radio_ptr->ptt_type) { // TODO: Add type for 'Parallel'!
        case RIG_PTT_SERIAL_RTS:
            radio_ptr->gkRig->setConf("ptt_type", "RTS");
            radio_ptr->gkRig->setConf("rts_state", "ON");
            radio_ptr->gkRig->setConf("dtr_state", "OFF");
            break;
        case RIG_PTT_SERIAL_DTR:
            radio_ptr->gkRig->setConf("ptt_type", "DTR");
            radio_ptr->gkRig->setConf("dtr_state", "ON");
            radio_ptr->gkRig->setConf("rts_state", "OFF");
            break;
        case RIG_PTT_RIG:
            radio_ptr->gkRig->setConf("ptt_type", "RIG");
            radio_ptr->gkRig->setConf("dtr_state", "OFF");
            radio_ptr->gkRig->setConf("rts_state", "OFF");
            break;
        case RIG_PTT_RIG_MICDATA:
            radio_ptr->gkRig->setConf("ptt_type", "RIGMICDATA");
            radio_ptr->gkRig->setConf("dtr_state", "OFF");
            radio_ptr->gkRig->setConf("rts_state", "OFF");
            break;
        case RIG_PTT_NONE:
            radio_ptr->gkRig->setConf("ptt_type", "None");
            radio_ptr->gkRig->setConf("dtr_state", "OFF");
            radio_ptr->gkRig->setConf("rts_state", "OFF");
            break;
        default:
            radio_ptr->gkRig->setConf("ptt_type", "None");
            radio_ptr->gkRig->setConf("dtr_state", "OFF");
            radio_ptr->gkRig->setConf("rts_state", "OFF");
            break;
        }

        switch (radio_ptr->port_details.parm.serial.dtr_state) {
        case serial_control_state_e::RIG_SIGNAL_ON:
            // High
            radio_ptr->gkRig->setConf("dtr_state", "ON");
            break;
        case serial_control_state_e::RIG_SIGNAL_OFF:
            // Low
            radio_ptr->gkRig->setConf("dtr_state", "OFF");
            break;
        default:
            // Nothing
            radio_ptr->gkRig->setConf("dtr_state", "UNSET");
            break;
        }

        switch (radio_ptr->port_details.parm.serial.handshake) {
        case serial_handshake_e::RIG_HANDSHAKE_NONE:
            // Default
            radio_ptr->gkRig->setConf("serial_handshake", "None");
            break;
        case serial_handshake_e::RIG_HANDSHAKE_XONXOFF:
            // XON / XOFF
            radio_ptr->gkRig->setConf("serial_handshake", "XONXOFF");
            break;
        case serial_handshake_e::RIG_HANDSHAKE_HARDWARE:
            // Hardware
            radio_ptr->gkRig->setConf("serial_handshake", "Hardware");
            break;
        default:
            // Nothing
            radio_ptr->gkRig->setConf("serial_handshake", "None");
            break;
        }

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
        if (!radio_ptr->cat_conn_port.isNull() && !radio_ptr->cat_conn_port.isEmpty()) {
            radio_ptr->gkRig->setConf("ptt_pathname", radio_ptr->ptt_conn_port.toStdString().c_str());
            radio_ptr->gkRig->setConf("rig_pathname", radio_ptr->cat_conn_port.toStdString().c_str());
        }

        if (radio_ptr->rig_model < 1) { // No amateur radio rig has been configured and/or adequately detected!
            //
            // Probe the given communications port, whether it be RS232, USB, GPIO, etc.
            // With this information, provided a connection has been made successfully, we can infer the amateur radio
            // rig that the user is making use of!
            //
            radio_ptr->rig_model = rig_probe(&radio_ptr->port_details);
        }

        rig_debug(radio_ptr->verbosity, "Backend version: %s, Status: %s\n\n", radio_ptr->gkRig->caps->version, rig_strstatus(radio_ptr->gkRig->caps->status));

        //
        // Open our rig in question
        //
        radio_ptr->gkRig->open();
        radio_ptr->is_open = true; // Set the flag that the aforementioned pointer has been initialized

        while (radio_ptr->is_open) {
            //
            // IMPORTANT!!!
            // Note from Hamlib Developers: As a general practice, we should check to see if a given function
            // is within the rig's capabilities before calling it, but we are simplifying here. Also, we should
            // check each call's returned status in case of error.
            //
            if (radio_ptr->gkRig->getInfo() != nullptr) {
                radio_ptr->info_buf = radio_ptr->gkRig->getInfo();
            }

            // Main VFO frequency
            radio_ptr->freq = radio_ptr->gkRig->getFreq(RIG_VFO_CURR);

            // Current mode
            radio_ptr->mode = radio_ptr->gkRig->getMode(radio_ptr->width, RIG_VFO_CURR);

            // Determine the mode of modulation that's being currently used, and output as a textual value
            radio_ptr->mm = hamlibModulEnumToStr(radio_ptr->mode).toStdString();

            // Rig power output
            if (radio_ptr->gkRig->hasGetLevel(RIG_LEVEL_RFPOWER)) {
                radio_ptr->gkRig->getLevel(RIG_LEVEL_RFPOWER, radio_ptr->power, RIG_VFO_CURR);
            }

            // Convert power reading to watts
            radio_ptr->gkRig->power2mW(radio_ptr->power, radio_ptr->freq, radio_ptr->mode);

            // Raw and calibrated S-meter values
            if (radio_ptr->gkRig->hasGetLevel(RIG_LEVEL_RAWSTR)) {
                radio_ptr->gkRig->getLevel(RIG_LEVEL_RAWSTR, radio_ptr->raw_strength, RIG_VFO_CURR);
            }

            if (radio_ptr->gkRig->hasGetLevel(RIG_LEVEL_STRENGTH)) {
                radio_ptr->gkRig->getLevel(RIG_LEVEL_STRENGTH, radio_ptr->strength, RIG_VFO_CURR);
            }
        }
    } catch (const RigException &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("Unable to make a connection with your radio rig! Error:\n\n%1")
                             .arg(QString::fromStdString(e.message)), QMessageBox::Ok) ;
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
qint16 RadioLibs::calibrateAudioInputSignal(const qint16 *data_buf)
{
    return -1;
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
