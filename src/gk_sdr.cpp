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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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

#include "src/gk_sdr.hpp"
#include <exception>
#include <utility>
#include <QtGlobal>
#include <QEventLoop>
#include <QMessageBox>
#include <QImageReader>
#include <QMimeDatabase>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

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
using namespace GkSdr;

GkSdrDev::GkSdrDev(QObject *parent) : QObject(parent)
{
    setParent(parent);

    return;
}

GkSdrDev::~GkSdrDev()
{
    return;
}

/**
 * @brief GkSdrDev::enumSoapySdrDevs enumerates out any SDR devices found via the SoapySDR set of libraries.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param kwargs Arguments to be provided to SoapySDR itself in the enumeration of any SDR devices, if applicable.
 * @return SDR devices, if any, that have been enumerated via the SoapySDR set of libraries.
 * @see GkSdrDev::findSoapySdrDevs().
 */
QList<GkSoapySdrTableView> GkSdrDev::enumSoapySdrDevs() const
{
    try {
        //
        // Enumerate any discovered SDR devices (and applicable information)!
        QList<GkSoapySdrTableView> sdr_devs;
        sdr_devs.clear();
        const auto devsFound = findSoapySdrDevs();
        if (!devsFound.isEmpty()) {
            for (const auto &dev: devsFound) {
                if (!dev.dev_name.isEmpty()) {
                    if (filterSoapySdrDevs(dev.dev_name)) {
                        sdr_devs.push_back(dev);
                    }
                }
            }
        }

        if (!sdr_devs.isEmpty()) {
            return sdr_devs;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("Error encountered whilst enumerating SDR devices via SoapySDR!\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return QList<GkSoapySdrTableView>();
}

/**
 * @brief GkSdrDev::findSoapySdrDevs searches for any SDR devices found via the SoapySDR set of libraries.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param kwargs Arguments to be provided to SoapySDR itself in the enumeration of any SDR devices, if applicable.
 * @return SDR devices, if any, that have been found via the SoapySDR set of libraries.
 * @note SdrPlusPlus <https://github.com/AlexandreRouma/SDRPlusPlus/blob/master/source_modules/soapy_source/src/main.cpp>.
 * @see GkSdrDev::enumSoapySdrDevs().
 */
QList<GkSoapySdrTableView> GkSdrDev::findSoapySdrDevs() const
{
    try {
        QList<GkSoapySdrTableView> devsFound;
        std::string txtDevList = "";
        qint32 idx = 0;

        SoapySDR::KwargsList devList;
        devList = SoapySDR::Device::enumerate();
        for (size_t i = 0; i < devList.size(); ++i) {
            SoapySDR::Kwargs deviceArgs = devList[i];
            const auto hw_key = findSoapySdrHwKey(deviceArgs);
            if (!hw_key.isEmpty()) {
                if (hw_key != "Audio") {
                    for (auto it = deviceArgs.begin(); it != deviceArgs.end(); ++it) {
                        GkSoapySdrTableView soapySdrTableView;
                        soapySdrTableView.event_no = idx;
                        soapySdrTableView.dev_hw_key = hw_key;
                        if (it->first == "label" || it->first == "device") {
                            if (!it->second.empty()) {
                                soapySdrTableView.dev_name = QString::fromStdString(it->second);
                                void (*unmake)(SoapySDR::Device *) = &SoapySDR::Device::unmake; // Ensure to select the right overload!
                                soapySdrTableView.dev_ptr = { SoapySDR::Device::make(deviceArgs), unmake };
                            }
                        }

                        if (!soapySdrTableView.dev_name.isEmpty()) {
                            devsFound.push_back(soapySdrTableView);
                            ++idx;
                        }
                    }
                }
            }
        }

        if (!devsFound.isEmpty()) {
            return devsFound;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QList<GkSoapySdrTableView>();
}

/**
 * @brief GkSdrDev::filterSoapySdrDevs filters out any unnecessary and/or garbage data as outputted by
 * the GkSdrDev::findSoapySdrDevs() function, allowing for a clearer picture of the attached SDR devices (if any)
 * present on the end-user's computer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sdr_dev Data as outputted by the GkSdrDev::findSoapySdrDevs() function.
 * @return The filtered information. Returns, `true`, if we have a valid result but otherwise, will return, `false`.
 * @see GkSdrDev::enumSoapySdrDevs().
 */
bool GkSdrDev::filterSoapySdrDevs(const QString &sdr_dev) const
{
    if (sdr_dev.startsWith("0x", Qt::CaseInsensitive)) {
        return false;
    }

    return true;
}

/**
 * @brief GkSdrDev::findSoapySdrHwKey queries a key that uniquely identifies the hardware. This key should be meaningful
 * to the user to optimize for the underlying hardware.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param input_args Arguments to be provided to SoapySDR itself in the enumeration of any SDR devices, if applicable.
 * @return A key that uniquely identifies the hardware.
 * @see GkSdrDev::enumSoapySdrDevs().
 */
QString GkSdrDev::findSoapySdrHwKey(const QString &input_args) const
{
    try {
        SoapySDR::Device *device = SoapySDR::Device::make(input_args.toStdString());
        const std::string hw_key = device->getHardwareKey();

        SoapySDR::Device::unmake(device);
        return QString::fromStdString(hw_key);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QString();
}

/**
 * @brief GkSdrDev::findSoapySdrHwKey queries a key that uniquely identifies the hardware. This key should be meaningful
 * to the user to optimize for the underlying hardware.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param input_args Arguments to be provided to SoapySDR itself in the enumeration of any SDR devices, if applicable.
 * @return A key that uniquely identifies the hardware.
 * @see GkSdrDev::enumSoapySdrDevs().
 */
QString GkSdrDev::findSoapySdrHwKey(const SoapySDR::Kwargs &input_args) const
{
    try {
        SoapySDR::Device *device = SoapySDR::Device::make(input_args);
        const std::string hw_key = device->getHardwareKey();

        SoapySDR::Device::unmake(device);
        return QString::fromStdString(hw_key);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QString();
}

/**
 * @brief GkSdrDev::findSoapySdrHwKey queries a key that uniquely identifies the hardware. This key should be meaningful
 * to the user to optimize for the underlying hardware.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dev_ptr The pointer to the enumerated SDR device itself.
 * @return A key that uniquely identifies the hardware.
 * @see GkSdrDev::enumSoapySdrDevs().
 */
QString GkSdrDev::findSoapySdrHwKey(std::shared_ptr<SoapySDR::Device> dev_ptr) const
{
    try {
        const std::string hw_key = dev_ptr->getHardwareKey();

        return QString::fromStdString(hw_key);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QString();
}

/**
 * @brief GkSdrDev::findSoapySdrHwInfo queries a dictionary of available device information. This dictionary can any
 * number of values like vendor name, product name, revisions, serials... This information can be displayed to the user
 * to help identify the instantiated device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param input_args Arguments to be provided to SoapySDR itself in the enumeration of any SDR devices, if applicable.
 * @return Unique SDR device information gathered by querying a dictionary of available SDRs.
 * @note CubicSDR <https://github.com/cjcliffe/CubicSDR/blob/master/src/sdr/SDREnumerator.h>,
 * CubicSDR <https://github.com/cjcliffe/CubicSDR/blob/master/src/sdr/SDREnumerator.cpp>.
 * @see GkSdrDev::enumSoapySdrDevs().
 */
SoapySDR::Kwargs GkSdrDev::findSoapySdrHwInfo(const QString &input_args) const
{
    try {
        SoapySDR::Device *device = SoapySDR::Device::make(input_args.toStdString());
        const SoapySDR::Kwargs info = device->getHardwareInfo();

        SoapySDR::Device::unmake(device);
        return info;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return SoapySDR::Kwargs();
}

/**
 * @brief GkSdrDev::findSoapySdrHwInfo queries a dictionary of available device information. This dictionary can any
 * number of values like vendor name, product name, revisions, serials... This information can be displayed to the user
 * to help identify the instantiated device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param input_args Arguments to be provided to SoapySDR itself in the enumeration of any SDR devices, if applicable.
 * @return Unique SDR device information gathered by querying a dictionary of available SDRs.
 * @note CubicSDR <https://github.com/cjcliffe/CubicSDR/blob/master/src/sdr/SDREnumerator.h>,
 * CubicSDR <https://github.com/cjcliffe/CubicSDR/blob/master/src/sdr/SDREnumerator.cpp>.
 * @see GkSdrDev::enumSoapySdrDevs().
 */
SoapySDR::Kwargs GkSdrDev::findSoapySdrHwInfo(const SoapySDR::Kwargs &input_args) const
{
    try {
        SoapySDR::Device *device = SoapySDR::Device::make(input_args);
        const SoapySDR::Kwargs info = device->getHardwareInfo();

        SoapySDR::Device::unmake(device);
        return info;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return SoapySDR::Kwargs();
}

/**
 * @brief GkSdrDev::findSoapySdrHwInfo queries a dictionary of available device information. This dictionary can any
 * number of values like vendor name, product name, revisions, serials... This information can be displayed to the user
 * to help identify the instantiated device.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dev_ptr The pointer to the enumerated SDR device itself.
 * @return Unique SDR device information gathered by querying a dictionary of available SDRs.
 * @note CubicSDR <https://github.com/cjcliffe/CubicSDR/blob/master/src/sdr/SDREnumerator.h>,
 * CubicSDR <https://github.com/cjcliffe/CubicSDR/blob/master/src/sdr/SDREnumerator.cpp>.
 * @see GkSdrDev::enumSoapySdrDevs().
 */
SoapySDR::Kwargs GkSdrDev::findSoapySdrHwInfo(std::shared_ptr<SoapySDR::Device> dev_ptr) const
{
    try {
        const SoapySDR::Kwargs info = dev_ptr->getHardwareInfo();

        return info;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return SoapySDR::Kwargs();
}
